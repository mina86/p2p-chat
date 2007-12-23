/** \file
 * Network module definition.
 * $Id: network.hpp,v 1.1 2007/12/23 00:53:19 mina86 Exp $
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
	typedef unordered_vector<NetworkConnection> TCPSockets;

	static unsigned network_id;

	void acceptConnections();
	void recieveUDPDatagrams();
	void readFromTCPConnection(NetworkConnection &conn);

	Address address;
	TCPListeningSocket *tcpListeningSocket;
	UDPSocket *udpSocket;
	TCPSockets tcpSockets;

	shared_obj<sig::UsersListData> users;
	User &ourUser;
};

}

#endif
