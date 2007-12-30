/** \file
 * Network module implementation.
 * $Id: network.cpp,v 1.13 2007/12/30 15:21:10 mina86 Exp $
 */

#include <stdio.h>
#include <string.h>

#include "network.hpp"
#include "ppcp-parser.hpp"
#include "ppcp-packets.hpp"



namespace ppc {


struct NetworkUser;
struct NetworkConnection;



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


	/**
	 * Initialises User object.
	 *
	 * \param i  user's ID -- nick name, IP address pair
	 * \param n  user's display name or empty string.
	 * \param st user's status.
	 * \throw InvalidNick if \a n is invalid display name.
	 */
	NetworkUser(ID i, const std::string &n, const Status &st = Status())
		: User(i, n, st), lastAccessed(Core::getTicks()) { }

	/**
	 * Initialises User object.  User's display name is set from
	 * <tt>i.name</tt>.
	 *
	 * \param i  user's ID -- nick name, IP address pair
	 * \param st user's status.
	 */
	explicit NetworkUser(ID i, const Status &st = Status())
		: User(i, st), lastAccessed(Core::getTicks()) { }

	/**
	 * Initialises User object.  User's ID is set from \a n and \a
	 * addr.
	 *
	 * \param n    user's display name.
	 * \param addr user's address (IP, port pair).
	 * \param st   user's status.
	 * \throw InvalidNick if \a n is invalid display name.
	 */
	NetworkUser(const std::string &n, Address addr, const Status &st=Status())
		: User(n, addr, st) { }

	/**
	 * Returns time in ticks since last access or \c 0 if there is at
	 * least one connection associated with given user.
	 */
	unsigned long age() const {
		return connections.empty() ? Core::getTicks() - lastAccessed : 0;
	}

	/** Returns first connection we can send data through or \c NULL. */
	inline NetworkConnection *getConnection();

	/** Updates last access time. */
	void accessed() {
		lastAccessed = Core::getTicks();
	}


private:
	/** Active connections to user. */
	Connections connections;

	/** Moment user did some activity last time. */
	unsigned long lastAccessed;


	friend struct NetworkConnection;
};



/** Structure holding data associated with TCP connection. */
struct NetworkConnection {
	/** Possible flags. */
	enum {
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


	/** Connection flags -- combination of flags defined in enum above. */
	unsigned short flags;


	/**
	 * Constructor.
	 * \param sock    TCP socket.
	 * \param ourNick our nick name.
	 */
	NetworkConnection(TCPSocket &sock, const std::string &ourNick)
		: tcpSocket(sock), tokenizer(ourNick),
		  lastAccessed(Core::getTicks()) { }

	/** Deletes a tcpSocket. */
	~NetworkConnection() {
		delete &tcpSocket;
		if (user) {
			user->connections.erase(user->connections.find(this));
			user->lastAccessed = Core::getTicks();
			user = 0;
		}
	}

	/**
	 * Attaches connection to given user.
	 * \param u user to attach connection to.
	 */
	void attachTo(NetworkUser &u) {
		user = &u;
		u.connections.push_back(this);
	}

	/** Returns \c true iff connection is attached to some user. */
	bool isAttached() const {
		return user;
	}

	/** Returns user connection is attached to or \c NULL. */
	NetworkUser *getUser() { return user; }

	/** Returns time in ticks since last access. */
	unsigned long age() const {
		return Core::getTicks() - lastAccessed;
	}

	/**
	 * Returns next token from tokenizer.
	 * \throw xml::Error if data read from socket was invalid XML.
	 */
	ppcp::Tokenizer::Token nextToken() {
		return tokenizer.nextToken();
	}

	/**
	 * Reads data from socket and feeds data to tokenizer.  Returns \c
	 * true iff data was sucesfully read from socket.
	 * \throw IOException if error while reading data from socket occured.
	 */
	bool feed() {
		std::string data = read();
		if (!data.empty()) {
			tokenizer.feed(data);
		}
		return !data.empty();
	}

	/**
	 * Reads data from socket.  Returns read data or empty string.  If
	 * data was read updates \a lastAccessed field.
	 * \throw IOException if error while reading data from socket occured.
	 */
	std::string read() {
		std::string data = tcpSocket.read();
		if (!data.empty()) {
			lastAccessed = Core::getTicks();
		}
		return data;
	}

	/**
	 * Writes data to socket and updates \c lastAccessed field.
	 * \throw IOException if error while writting data from socket occured.
	 */
	void write() {
		tcpSocket.write();
		lastAccessed = Core::getTicks();
	}

	/**
	 * Pushes data to buffer to send it later on.
	 * \param str string to append to buffer.
	 */
	void push(const std::string &str) {
		tcpSocket.push(str);
	}

	/** Returns whether socket has pending data to write. */
	bool hasDataToWrite() const {
		return tcpSocket.hasDataToWrite();
	}

	/** Returns socket's file descriptor number. */
	int getFD() {
		return tcpSocket.fd;
	}

	/** Returns address socket is connected to. */
	Address getAddress() const {
		return tcpSocket.address;
	}

	/** Returns end of file flag of TCP connection. */
	bool isEOF() const {
		return tcpSocket.isEOF();
	}


private:
	/** A TCP socket. */
	TCPSocket &tcpSocket;

	/** User this connection is associated with or \c NULL. */
	NetworkUser *user;

	/** Tokenizer used to parse packets. */
	ppcp::StandAloneTokenizer tokenizer;

	/** Last moment there was activity on connection. */
	unsigned long lastAccessed;
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


/**
 * A private (file-scope) variable to make sequential numbers in
 * module names.  Each file implementing each module (should) have its
 * own \a seq variable.
 */
static unsigned long seq = 0;


Network::Network(Core &c, Address addr, const std::string &nick)
	: Module(c, "/net/ppc/", seq++), address(addr),
	  lastStatus(Core::getTicks()), ourUser(users->ourUser) {
	tcpListeningSocket = TCPListeningSocket::bind(Address());
	udpSocket = UDPSocket::bind(addr);
	users = new sig::UsersListData(nick, tcpListeningSocket->address.port);
	sendSignal("/net/conn/connected", "/ui/", users.get());
}



Network::~Network() {
	delete tcpListeningSocket;
	tcpListeningSocket = 0;
	delete udpSocket;
	udpSocket = 0;
	Connections::iterator it = connections.begin(), end = connections.end();
	for (; it != end; ++it) {
		delete *it;
	}
	connections.clear();
	/* NetworkUser objects kept in \a users may reference already
	   deleted TCP sockets but this doesn't really matter since only
	   we knew that User object stored there are really
	   NetworkUser objects. */
	sendSignal("/net/conn/disconnected", "/ui/", 0);
}



int Network::setFDSets(fd_set *rd, fd_set *wr, fd_set *ex) {
	int max, fd;
	(void)ex;

	FD_SET(max = tcpListeningSocket->fd, rd);
	FD_SET(fd = udpSocket->fd, rd);
	if (udpSocket->hasDataToWrite()) {
		FD_SET(fd, wr);
	}
	if (fd > max) {
		max = fd;
	}

	Connections::iterator it = connections.begin(), end = connections.end();
	for (; it != end; ++it) {
		fd = (*it)->getFD();
		if (!(*it)->isEOF()) {
			FD_SET(fd, rd);
		}
		if ((*it)->hasDataToWrite()) {
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
	if (FD_ISSET(tcpListeningSocket->fd, rd)) {
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
	if (FD_ISSET(udpSocket->fd, rd)) {
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
	if (FD_ISSET(udpSocket->fd, wr)) {
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
	Connections::iterator it = connections.begin(), end = connections.end();
	while (nfds > 0 && it != end) {
		int fd = (*it)->getFD();
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
			delete *it;
			it = connections.erase(it);
			end = connections.end();
		} else {
			++it;
		}
	}


	return handled;
}



void Network::recievedSignal(const Signal &sig) {
	if (sig.getType() == "/core/tick") {
		missedTicks = (missedTicks + 1) % HZ_DIVIDER;
		if (!missedTicks) {
			performTick();
		}

	} else if (sig.getType() == "/net/status/change") {
		const sig::UserData &data =
			*static_cast<const sig::UserData*>(sig.getData());
		if (!data.flags) {
			return;
		}

		if (data.flags & sig::UserData::STATE) {
			ourUser.status.state = data.user.status.state;
		}
		if (data.flags & sig::UserData::MESSAGE) {
			ourUser.status.message = data.user.status.message;
		}
		if (data.flags & sig::UserData::NAME) {
			ourUser.name = data.user.name;
		}

		sendSignal("/net/status/changed", "/ui/",
		           new sig::UserData(ourUser, data.flags));
		send(data.flags & sig::UserData::REQUEST
		     ? ppcp::st(ourUser)+ppcp::rq() : ppcp::st(ourUser));
		lastStatus = Core::getTicks();

	} else if (sig.getType() == "/net/conn/are-you-connected") {
		sendSignal("/net/conn/connected", "/ui/", users.get());

	} else if (sig.getType() == "/net/msg/send") {
		const sig::MessageData &data =
			*static_cast<const sig::MessageData*>(sig.getData());
		send(data.id,
		     data.flags & sig::MessageData::RAW ? data.data : ppcp::m(data),
		     data.flags & sig::MessageData::ALLOW_UDP);
		sendSignal("/net/msg/sent", "/ui/", sig);

	} else if (sig.getType() == "/net/status/rq") {
		const sig::MessageData &data =
			*static_cast<const sig::MessageData*>(sig.getData());
		send(data.id, ppcp::rq(), true);
	}
}



void Network::acceptConnections() {
	TCPSocket *sock;
	while ((sock = tcpListeningSocket->accept())) {
		NetworkConnection *conn;
		conn = new NetworkConnection(*sock, ourUser.id.nick);
		sock->push(ppcp::ppcpOpen(ourUser));
		connections.push_back(conn);
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
			case ppcp::Tokenizer::IGNORE:
			case ppcp::Tokenizer::PPCP_CLOSE:
				goto nextDatagram;

			case ppcp::Tokenizer::PPCP_OPEN:
				user = &getUser(User::ID(token.data,
				                         Address(addr.ip, token.flags)),
				                token.data2);
			case ppcp::Tokenizer::END: /* dead code */
				break;

			default:
				if (user) handleToken(*user, token);
			}
		}

	nextDatagram: ;
	}
}



void Network::readFromTCPConnection(NetworkConnection &conn) {
	ppcp::Tokenizer::Token token;

	if (conn.flags & NetworkConnection::REMOTE_CLOSED) {
	ignore:
		while (!conn.read().empty());
		return;
	}

	for(;;){
		switch (token.type) {
		case ppcp::Tokenizer::END:
			if (!conn.feed()) {
				if (conn.isEOF()) {
					throw IOException("Unexpected end of file.");
				}
				return;
			}
			break;

		case ppcp::Tokenizer::IGNORE:
		case ppcp::Tokenizer::PPCP_CLOSE:
			if (~conn.flags & NetworkConnection::LOCAL_CLOSING) {
				conn.push(ppcp::ppcpClose());
			}
			conn.flags |= NetworkConnection::REMOTE_CLOSED;
			conn.flags |= NetworkConnection::LOCAL_CLOSING;
			goto ignore;

		case ppcp::Tokenizer::PPCP_OPEN:
			if (conn.isAttached()) break;
			conn.attachTo(getUser(User::ID(token.data,
			                               Address(conn.getAddress().ip,
			                                       token.flags)),
			                      token.data2));
			break;

		default:
			if (conn.isAttached()) handleToken(*conn.getUser(), token);
		}

		token = conn.nextToken();
	}
}



void Network::handleToken(NetworkUser &user,
                          const ppcp::Tokenizer::Token &token) {
	switch (token.type) {
	case ppcp::Tokenizer::ST: {
		unsigned flags = 0;
		if (user.status.state != (User::State)token.flags) {
			user.status.state = (User::State)token.flags;
			flags |= sig::UserData::STATE;
		}
		if (user.status.message != token.data) {
			user.status.message = token.data;
			flags |= sig::UserData::MESSAGE;
		}
		if (user.name != token.data2) {
			user.name = token.data2;
			flags |= sig::UserData::NAME;
		}
		if (flags) {
			sendSignal("/net/status/changed", "/ui/",
			           new sig::UserData(user, flags));
		}
		break;
	}

	case ppcp::Tokenizer::RQ:
		if (Core::getTicks() - lastStatus + 10 >= STATUS_RESEND) {
			send(ppcp::st(ourUser));
			lastStatus = Core::getTicks();
		} else {
			send(user, ppcp::st(ourUser));
		}
		break;

	case ppcp::Tokenizer::M:
		sendSignal("/net/msg/got", "/ui/",
		           new sig::MessageData(user.id, token.data, token.flags));
		break;

	default: /* dead code */
		;
	}
}



void Network::writeToTCPConnection(NetworkConnection &conn) {
	conn.write();
	if ((conn.flags & NetworkConnection::LOCAL_CLOSING) &&
	    !conn.hasDataToWrite()) {
		conn.flags |= NetworkConnection::LOCAL_CLOSED;
	}
}



void Network::performTick() {
	if (Core::getTicks() - lastStatus >= STATUS_RESEND) {
		send(ppcp::st(ourUser));
		lastStatus = Core::getTicks();
	}

	/* Handle connections */
	Connections::iterator c    = connections.begin();
	Connections::iterator cend = connections.end();
	while (c != cend) {
		if ((*c)->flags & NetworkConnection::LOCAL_CLOSING) {
			if((*c)->age() < CONNECTION_CLOSING_TIMEOUT) goto next;
			delete *c;
			c = connections.erase(c);
			cend = connections.end();
			continue;
		} else {
			if ((*c)->age() < CONNECTION_MAX_AGE) goto next;
			(*c)->flags |= NetworkConnection::LOCAL_CLOSING;
			(*c)->push(ppcp::ppcpClose());
		}
	next:
		++c;
	}

	/* Handle users */
	sig::UsersListData::Users::iterator u = users->users.begin();
	sig::UsersListData::Users::iterator uend = users->users.end();
	while (u != uend) {
		NetworkUser &user = *static_cast<NetworkUser*>(u->second);
		if (user.age() < (user.status.state == User::OFFLINE
		                  ? OFFLINE_USER_MAX_AGE : ONLINE_USER_MAX_AGE)) {
			++u;
			continue;
		}

		sendSignal("/net/usr/changed", "/ui/",
		           new sig::UserData(user, sig::UserData::DISCONNECTED));
		User::ID id = user.id;
		delete u->second;
		users->users.erase(u);
		u = users->users.lower_bound(id);
		uend = users->users.end();
	}
}


NetworkUser &Network::getUser(const User::ID &id, const std::string &name) {
	std::pair<sig::UsersListData::Users::iterator, bool> ret;
	ret = users->users.insert(std::make_pair(id, (User*)0));
	if (ret.second) {
		static_cast<NetworkUser*>(ret.first->second)->accessed();
	} else {
		ret.first->second = new NetworkUser(id, name);
		sendSignal("/net/status/changed", "/ui/",
		           new sig::UserData(*ret.first->second,
		                             sig::UserData::CONNECTED));
	}
	return *static_cast<NetworkUser*>(ret.first->second);
}



void Network::send(NetworkUser &user, const std::string &str, bool udp) {
	NetworkConnection *conn = user.getConnection();

	if (!conn && udp) {
		udpSocket->push(ppcp::ppcpOpen(ourUser, user.id.nick) + str +
		                ppcp::ppcpClose(),
		                Address(user.id.address.ip, address.port));
	} else {
		TCPSocket *sock;
		try {
			sock = TCPSocket::connect(user.id.address);
		}
		catch (const IOException &e) {
			sendSignal("/ui/msg/error", "/ui/",
			           new sig::StringData("Error connecting to user: " +
			                               e.getMessage()));
			return;
		}
		conn = new NetworkConnection(*sock, ourUser.id.nick);
		conn->attachTo(user);
		sock->push(ppcp::ppcpOpen(ourUser, user.id.nick));
	}

	conn->push(str);
}


void Network::send(const User::ID &id, const std::string &str, bool udp) {
	if (id.address.ip && id.address.port) {
		send(getUser(id), str, udp);
	} else if (!udp) {
		/* nothing */
	} else {
		udpSocket->push(ppcp::ppcpOpen(ourUser, id.nick) + str +
		                ppcp::ppcpClose(),
		                id.address.ip
		                ? Address(id.address.ip, address.port) : address);
	}
}


NetworkConnection *NetworkUser::getConnection() {
	Connections::iterator it = connections.begin(), end = connections.end();
	while (it != end && ~(*it)->flags & NetworkConnection::LOCAL_CLOSING) {
		++it;
	}
	return it == end ? 0 : *it;
}

}
