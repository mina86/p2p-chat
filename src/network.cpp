/** \file
 * Network module implementation.
 * $Id: network.cpp,v 1.3 2007/12/24 12:34:01 mina86 Exp $
 */

#include <stdio.h>
#include <string.h>

#include "network.hpp"
#include "ppcp-parser.hpp"
#include "ppcp-packets.hpp"



namespace ppc {


/** Structure holding data associated with TCP connection. */
struct NetworkConnection {
	/** Possible flags. */
	enum Flags {
		/** Remote side have opened \c ppcp element and thus we have
		 * recieved remote user's nick name. */
		KNOW_WHO      = 0x01,

		/** Remote side have closed \c ppcp element.  We should do the
		 * same.  All data read from strem must be ignored. */
		REMOTE_CLOSED = 0x02,

		/** We have pushed \c ppcp closing tag yet it was not sent
		 * yet.  We must not writa data through this stream. */
		LOCAL_CLOSING = 0x04,

		/** We have closed \c ppcp element.  We must not write data
		 * through this stream.  */
		LOCAL_CLOSED  = 0x08,

		/** Both sides have closed \c ppcp element.  Connection shall
		 * be closed and removed from list. */
		BOTH_CLOSED   = 0x0A
	};


	/**
	 * ID of user this connection is to.  \a id's nick may be an empty
	 * string which means that we don't know sender's nick name yet.
	 */
	User::ID id;

	/** A TCP socket. */
	TCPSocket *tcpSocket;

	/** Tokenizer used to parse packets. */
	ppcp::StandAloneTokenizer tokenizer;

	/* Connection flags. */
	unsigned short flags;


	/**
	 * Constructor.
	 * \param sock    TCP socket.
	 * \param i       remote user ID.
	 * \param ourNick our nick name.
	 */
	NetworkConnection(TCPSocket *sock, const User::ID &i,
	                  const std::string &ourNick)
		: id(i), tcpSocket(sock), tokenizer(ourNick), flags(0) { }

	/** Deletes a tcpSocket. */
	~NetworkConnection() {
		delete tcpSocket;
	}
};


/**
 * Specialisation of User class used to store additional attributes.
 * Network object has shared_obj which points to sig::UsersListData
 * object.  This object contains a map of pointers to User objects.
 * Network object really holds there pointers to NetworkUser object
 * (which extends User so there should be no problem as long as
 * casting is used when deleting users and no one outside Network
 * touches those User objects).
 */
struct NetworkUser : public User {
	/** Active connections to user. */
	typedef unordered_vector<NetworkConnection *> Connections;

	/** Active connections to user. */
	Connections connections;
	/** User age in ticks. */
	unsigned age;
};


/**
 * Specialisation of sig::UsersListData class which knows that we
 * really keep pointers to NetworkUser objects in \a users map.
 * sig::UsersListData has a virtual destructor hence we are sure that
 * NetworkUsersList destructor will be called.  This also prevents
 * making User class having virtual destructor (hence speeds things
 * up).
 */
struct NetworkUsersList : sig::UsersListData {
	/**
	 * Deletes all users in users map knowing that they are really
	 * NetworkUser objects not User objects.
	 */
	virtual ~NetworkUsersList() {
		if (users.empty()) {
			return;
		}

		std::map<User::ID, User *>::iterator it = users.begin();
		std::map<User::ID, User *>::iterator end = users.end();
		for (; it != end; ++it) {
			delete (NetworkUser*)it->second;
		}
		users.clear();
	}
};


unsigned Network::network_id = 0;



Network::Network(Core &c, Address addr, const std::string &nick)
	: Module(c, "/net/ppc/" + network_id++), address(addr),
	  users(new sig::UsersListData(nick)), ourUser(users->ourUser) {
	tcpListeningSocket = TCPListeningSocket::bind(Address(0, addr.port));
	udpSocket = UDPSocket::bind(addr);
}



Network::~Network() {
	delete tcpListeningSocket;
	tcpListeningSocket = 0;
	delete udpSocket;
	udpSocket = 0;
	TCPSockets::iterator it = tcpSockets.begin(), end = tcpSockets.end();
	for (; it != end; ++it) {
		delete *it;
	}
	tcpSockets.clear();
	/* NetworkUser objects kept in \a users may reference already
	   deleted TCP sockets but this doesn't really matter since only
	   we knew that User object stored there are really
	   NetworkUser objects. */
}



int Network::setFDSets(fd_set *rd, fd_set *wr, fd_set *ex) {
	int max, fd;
	(void)ex;

	FD_SET(max = tcpListeningSocket->getFD(), rd);
	FD_SET(fd = udpSocket->getFD(), rd);
	if (udpSocket->hasDataToWrite()) {
		FD_SET(fd, wr);
	}
	if (fd > max) {
		max = fd;
	}

	TCPSockets::iterator it = tcpSockets.begin(), end = tcpSockets.end();
	for (; it != end; ++it) {
		FD_SET(fd = (*it)->tcpSocket->getFD(), rd);
		if ((*it)->tcpSocket->hasDataToWrite()) {
			FD_SET(fd, wr);
		}
		if (fd > max) {
			max = fd;
		}
	}

	return max + 1;
}



int Network::doFDs(int nfds, const fd_set *rd, const fd_set *wr,
                   const fd_set *ex) {
	int handled = 0;
	(void)ex;


	/* Listening socekt */
	if (FD_ISSET(tcpListeningSocket->getFD(), rd)) {
		++handled, --nfds;
		try {
			acceptConnections();
		}
		catch (const Exception &e) {
			sendSignal("/ui/msg/error", "/ui/",
			           new sig::StringData("TCP listening socket error: " +
			                               e.getMessage()));
		}
	}


	/* Read from UDP socket */
	if (FD_ISSET(udpSocket->getFD(), rd)) {
		++handled, --nfds;
		try {
			readFromUDPSocket();
		}
		catch (const Exception &e) {
			sendSignal("/ui/msg/error", "/ui/",
			           new sig::StringData("UDP socket error: " +
			                               e.getMessage()));
		}
	}


	/* Write to UDP socket */
	if (FD_ISSET(udpSocket->getFD(), wr)) {
		++handled, --nfds;
		try {
			udpSocket->write();
		}
		catch (const Exception &e) {
			sendSignal("/ui/msg/error", "/ui/",
			           new sig::StringData("UDP socket error: " +
			                               e.getMessage()));
		}
	}


	/* If there is nothing more exit */
	if (nfds <= 0) {
		return handled;
	}


	/* TCP sockets */
	TCPSockets::iterator it = tcpSockets.begin(), end = tcpSockets.end();
	while (nfds > 0 && it != end) {
		int fd = (*it)->tcpSocket->getFD();
		if (FD_ISSET(fd, rd)) {
			++handled, --nfds;
		}
		if (FD_ISSET(fd, wr)) {
			++handled, --nfds;
		}

		try {
			if (FD_ISSET(fd, rd)) readFromTCPConnection(**it);
			if (FD_ISSET(fd, wr)) writeToTCPConnection (**it);
		}
		catch (const Exception &e) {
			sendSignal("/ui/msg/error", "/ui/",
			           new sig::StringData("TCP socket error: " +
			                               e.getMessage()));
			(*it)->flags |= NetworkConnection::BOTH_CLOSED;
		}

		/* !(~a & b)  is the same thing as  (a & b) == b */
		if (!(~(*it)->flags & NetworkConnection::BOTH_CLOSED)) {
			closeConnection(**it);
			it = tcpSockets.erase(it);
			end = tcpSockets.end();
		} else {
			++it;
		}
	}


	return handled;
}



void Network::recievedSignal(const Signal &sig) {
	(void)sig;
	/* FIXME: TODO */
}



void Network::acceptConnections() {
	TCPSocket *sock;
	while ((sock = tcpListeningSocket->accept())) {
		NetworkConnection *conn;
		conn = new NetworkConnection(sock, User::ID(sock->getAddress()),
		                             ourUser.id.nick);
		sock->push(ppcp::ppcpOpen(ourUser.id.nick));
		tcpSockets.push_back(conn);
	}
}



void Network::readFromUDPSocket() {
	ppcp::StandAloneTokenizer tokenizer(ourUser.id.nick, true);
	ppcp::Tokenizer::Token token;
	std::string data;
	Address addr;

	while (!(data = udpSocket->read(addr)).empty()) {
		NetworkUser *user = 0;

		if (addr.port != address.port) {
			continue;
		}

		tokenizer.init();
		tokenizer.feed(data);

		while ((token = tokenizer.nextToken())) {
			switch (token.type) {
			case ppcp::Tokenizer::END: /* dead code */
				break;

			case ppcp::Tokenizer::IGNORE:
			case ppcp::Tokenizer::PPCP_CLOSE:
				return;

			case ppcp::Tokenizer::PPCP_OPEN:
				user = getUser(User::ID(token.data, addr.ip));
				break;

			case ppcp::Tokenizer::ST:
				gotStatus(*user, User::Status((User::State)token.flags,
				                              token.data));
				break;

			case ppcp::Tokenizer::RQ:
				sendStatus(user);
				break;

			case ppcp::Tokenizer::M:
				gotMessage(*user, token.data, token.flags);
				break;
			}
		}
	}
}



void Network::readFromTCPConnection(NetworkConnection &conn) {
	ppcp::Tokenizer::Token token;
	std::string data;
	NetworkUser *user = conn.flags & NetworkConnection::KNOW_WHO
		? getUser(conn.id) : 0;

	while (!(data = conn.tcpSocket->read()).empty()) {
		if (conn.flags & NetworkConnection::REMOTE_CLOSED) {
			continue;
		}

		conn.tokenizer.feed(data);

		/* Blah... 4 levels of indention and even more!  But making
		   this loop a separate function does not make much more sense
		   then using so many levels of indention as it would require
		   passing tons of arguments to that new function or
		   reconstructing objects several times.  The other solution
		   would bo to first read all data from connection and feed
		   tokenizer with it and then consume tokens but that would
		   require more memory. */

		while ((token = conn.tokenizer.nextToken())) {
			switch (token.type) {
			case ppcp::Tokenizer::IGNORE:
			case ppcp::Tokenizer::PPCP_CLOSE:
				if (!(conn.flags & NetworkConnection::LOCAL_CLOSING)) {
					conn.tcpSocket->push(ppcp::ppcpClose());
				}
				conn.flags |= NetworkConnection::REMOTE_CLOSED;
				conn.flags |= NetworkConnection::LOCAL_CLOSED;
			case ppcp::Tokenizer::END:
				break;

			case ppcp::Tokenizer::PPCP_OPEN:
				if (user) break;
				user = getUser(User::ID(token.data,
				                        conn.tcpSocket->getAddress().ip));
				conn.id = user->id;
				conn.flags |= NetworkConnection::KNOW_WHO;
				user->connections.push_back(&conn);
				break;

			case ppcp::Tokenizer::ST:
				if (!user) break;
				gotStatus(*user, User::Status((User::State)token.flags,
				                              token.data));
				break;

			case ppcp::Tokenizer::RQ:
				if (!user) break;
				sendStatus(user);
				break;

			case ppcp::Tokenizer::M:
				if (!user) break;
				gotMessage(*user, token.data, token.flags);
				break;
			}
		}
	}
}


void Network::writeToTCPConnection(NetworkConnection &conn) {
	conn.tcpSocket->write();
	if ((conn.flags & NetworkConnection::LOCAL_CLOSING) &&
	    !conn.tcpSocket->hasDataToWrite()) {
		conn.flags |= NetworkConnection::LOCAL_CLOSED;
	}
}


void Network::closeConnection(NetworkConnection &conn) {
	delete conn.tcpSocket;

	sig::UsersListData::Users::iterator uit = users->users.find(conn.id);
	if (uit == users->users.end()) {
		return;
	}

	NetworkUser &u = *static_cast<NetworkUser*>(uit->second);
	NetworkUser::Connections::iterator it = u.connections.find(&conn);
	if (it != u.connections.end()) {
		u.connections.erase(it);
		u.age = 0;
	}
}

}
