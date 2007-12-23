/** \file
 * Network module implementation.
 * $Id: network.cpp,v 1.2 2007/12/23 01:13:22 mina86 Exp $
 */

#include <stdio.h>
#include <string.h>

#include "network.hpp"
#include "ppcp-parser.hpp"



namespace ppc {


/** Structure holding data associated with TCP connection. */
struct NetworkConnection {
	/**
	 * ID of user this connection is to.  \a id's nick may be an empty
	 * string which means that we don't know sender's nick name yet.
	 */
	User::ID id;

	/** A TCP socket. */
	TCPSocket *tcpSocket;

	/** Tokenizer used to parse packets. */
	ppcp::StandAloneTokenizer *tokenizer;

	/**
	 * Constructor.
	 * \param sock    TCP socket.
	 * \param i       remote user ID.
	 * \param ourNick our nick name.
	 */
	NetworkConnection(TCPSocket *sock, const User::ID &i,
	                  const std::string &ourNick)
		: id(i), tcpSocket(sock),
		  tokenizer(new ppcp::StandAloneTokenizer(ourNick)) { }
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
	unordered_vector<NetworkConnection *> connections;
	/** User age in ticks. */
	unsigned age;
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
		delete it->tcpSocket;
		delete it->tokenizer;
	}
	tcpSockets.clear();
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
		FD_SET(fd = it->tcpSocket->getFD(), rd);
		if (it->tcpSocket->hasDataToWrite()) {
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
			recieveUDPDatagrams();
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
		int fd = it->tcpSocket->getFD();
		if (FD_ISSET(fd, rd)) {
			++handled, --nfds;
		}
		if (FD_ISSET(fd, wr)) {
			++handled, --nfds;
		}

		try {
			if (FD_ISSET(fd, rd)) {
				readFromTCPConnection(*it);
			}
			if (FD_ISSET(it->tcpSocket->getFD(), wr)) {
				it->tcpSocket->write();
			}
			++it;
		}
		catch (const Exception &e) {
			sendSignal("/ui/msg/error", "/ui/",
			           new sig::StringData("TCP socket error: " +
			                               e.getMessage()));
			delete it->tcpSocket;
			delete it->tokenizer;
			it = tcpSockets.erase(it);
			end = tcpSockets.end();
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
		tcpSockets.push_back(NetworkConnection(sock,
		                                       User::ID(sock->getAddress()),
		                                       ourUser.id.nick));
	}
}


void Network::recieveUDPDatagrams() {
	ppcp::Tokenizer::Token token;
	ppcp::Tokenizer ppcpTokenizer(ourUser.id.nick);
	xml::Tokenizer xmlTokenizer;
	std::string data;
	Address addr;

	while (!(data = udpSocket->read(addr)).empty()) {
		try {
			xmlTokenizer.init();
			xmlTokenizer.feed(data);
			ppcpTokenizer.init();
			while ((token = ppcpTokenizer.nextToken(xmlTokenizer))) {
				/* FIXME: TODO token consumption */
			}
		}
		catch (const Exception &e) {
			(void)e;
			/* ignore datagrams which are not valid PPCP */
		}
	}
}


void Network::readFromTCPConnection(NetworkConnection &conn) {
	ppcp::Tokenizer::Token token;
	std::string data;

	while (!(data = conn.tcpSocket->read()).empty()) {
		conn.tokenizer->feed(data);

		try {
			while ((token = conn.tokenizer->nextToken())) {
				/* FIXME: TODO token consumption */
			}
		} catch (const Exception &e) {
			(void)e;
			/* FIXME: TODO close connection */
		}
	}
}


}
