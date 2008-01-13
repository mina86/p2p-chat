/** \file
 * Network module definition.
 * $Id: network.hpp,v 1.12 2008/01/13 11:53:10 mina86 Exp $
 */

#ifndef H_NETWORK_HPP
#define H_NETWORK_HPP

#include "application.hpp"
#include "netio.hpp"
#include "user.hpp"
#include "unordered-vector.hpp"
#include "ppcp-parser.hpp"
#include "ppcp-packets.hpp"


/**
 * Intervfal between checking timeouts.  This may have value greater
 * then one to save some CPU time as Network will investigate all
 * users and all connections once every HZ_DIVIDER seconds instead of
 * every second.
 */
#define PPC_NETWORK_HZ_DIVIDER       10


namespace ppc {


struct NetworkConnection;
struct NetworkUser;


/**
 * Module maintaining network communication.
 */
struct Network : public Module {
	/**
	 * Creates new Network module.  This only opens TCP and UDP
	 * listening sockets but does not send any data (like \c st
	 * packet).  Network modules are named \c /net/ppcp/number where
	 * number is a sequence number starting from zero.
	 *
	 * \param core core module.
	 * \param addr ppcp network's address.
	 * \param nick our nick in ppcp network.
	 * \throw IOException if error while creating sockets occured.
	 * \throw InvalidNick  if \a nick is invalid.
	 */
	Network(Core &core, Address addr, const std::string &nick);

	/** Frees all resources and closes connections. */
	~Network();

	virtual int setFDSets(fd_set *rd, fd_set *wr, fd_set *ex);
	virtual int doFDs(int nfds, const fd_set *rd, const fd_set *wr,
	                  const fd_set *ex);
	virtual void recievedSignal(const Signal &sig);


private:
	/** Vector of all opened TCP connections. */
	typedef unordered_vector<NetworkConnection*> Connections;


	/**
	 * accept()s connections from listening socket and adds them to
	 * tcpSockets vector.
	 * \throw IOException if error while accepting connection occured.
	 */
	void acceptConnections();

	/**
	 * Reads data from udpSocket and parses it.
	 * \throw IOException if error while recieving data from UDP
	 *                     socket occured.
	 * \throw xml::Error when data feed to tokenizer was not valid XML.
	 */
	void readFromUDPSocket();

	/**
	 * Reads data from given TCP connection and parses it.
	 * \param conn connection to handle.
	 * \throw IOException if error while recieving data from TCP
	 *                     socket occured.
	 * \throw xml::Error   if data was misformatted XML stream.
	 */
	void readFromTCPConnection(NetworkConnection &conn);

	/**
	 * Handles single token from TCP connection or UDP datagram.
	 * \param user  user given token was sent from.
	 * \param token the token.
	 */
	void handleToken(NetworkUser &user, const ppcp::Tokenizer::Token &token);

	/**
	 * Writes pending data to given TCP connection and parses it.
	 * \param conn connection to handle.
	 * \throw IOException if error while sending data from TCP
	 *                     socket occured.
	 */
	void writeToTCPConnection(NetworkConnection &conn);


	/**
	 * Removes all users and closes all connections that age exceeded
	 * max age.
	 */
	void performTick();


	/**
	 * Sends given string to given user or to whole network.
	 * \param user user to send packet to.
	 * \param str  string to send.
	 * \param udp  whether data may be send through UDP multicast.
	 */
	void send(NetworkUser &user, const std::string &str, bool udp = false);


	/**
	 * Sends given string to given user or to whole network.
	 * \param id   user's ID to send packet to..
	 * \param str  string to send.
	 * \param udp  whether data may be send through UDP multicast.
	 */
	void send(const User::ID &id, const std::string &str, bool udp = false);


	/**
	 * Sends given string to all users (using multicast UDP).
	 * \param str  string to send.
	 */
	void send(const std::string &str) {
		udpSocket->push(ppcp::ppcpOpen(ourUser) + str + ppcp::ppcpClose(),
		                address);
	}


	/**
	 * Returns user with given ID.  If such user does not exist
	 * creates him/her.
	 * \param id   user's ID
	 * \param name user's display name to use if it is new user.
	 * \return Networkuser with given ID.
	 */
	NetworkUser &getUser(const User::ID &id,
	                     const std::string &name = std::string());



	/** Network's address. */
	Address address;

	/** TCP listening socket. */
	TCPListeningSocket *tcpListeningSocket;

	/** UDP socket. */
	UDPSocket *udpSocket;

	/** Vector of TCP sockets. */
	Connections connections;

	/** Number of ticks Network did not check if users/connections got old. */
	unsigned missedTicks;

	/** Last time status was sent. */
	unsigned lastStatus;

	/** If \c true we are disconnecting; many signals are ignored. */
	bool disconnecting;

	/** "Pointer" to map of all users connected to network. */
	shared_obj<sig::UsersListData> users;

	/** Reference to ourUser field from users. */
	User &ourUser;
};


}

#endif
