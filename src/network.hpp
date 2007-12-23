/** \file
 * Network module definition.
 * $Id: network.hpp,v 1.2 2007/12/23 01:13:22 mina86 Exp $
 */

#ifndef H_NETWORK_HPP
#define H_NETWORK_HPP

#include "application.hpp"
#include "netio.hpp"
#include "user.hpp"
#include "unordered-vector.hpp"


namespace ppc {


struct NetworkConnection;


/**
 * Module maintaining network communication.
 */
struct Network : public Module {
	/**
	 * Creates new Network module.  This only opens TCP and UDP
	 * listening sockets but does not send any data (like \c st
	 * packet).
	 *
	 * \param core core module.
	 * \param addr ppcp network's address.
	 * \param nick our nick in ppcp network.
	 * \throw NetException if error while creating sockets occured.
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
	typedef unordered_vector<NetworkConnection> TCPSockets;

	/** A sequence used to give each module unique name. */
	static unsigned network_id;

	/**
	 * accept()s connections from listening socket and adds them to
	 * tcpSockets vector.
	 */
	void acceptConnections();

	/** Reads data from udpSocket and parses it. */
	void recieveUDPDatagrams();

	/**
	 * Reads data from given TCP connection and parses it.
	 * \param conn connection to handle.
	 */
	void readFromTCPConnection(NetworkConnection &conn);


	/** Network's address. */
	Address address;

	/** TCP listening socket. */
	TCPListeningSocket *tcpListeningSocket;

	/** UDP socket. */
	UDPSocket *udpSocket;

	/** Vector of TCP sockets. */
	TCPSockets tcpSockets;


	/** "Pointer" to map of all users connected to network. */
	shared_obj<sig::UsersListData> users;

	/** Reference to ourUser field from users. */
	User &ourUser;
};


}

#endif
