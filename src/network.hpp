/** \file
 * Network module definition.
 * $Id: network.hpp,v 1.3 2007/12/24 12:31:51 mina86 Exp $
 */

#ifndef H_NETWORK_HPP
#define H_NETWORK_HPP

#include "application.hpp"
#include "netio.hpp"
#include "user.hpp"
#include "unordered-vector.hpp"


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
	/** Certain constants. */
	enum {
		/**
		 * Time after which user (with state different then offline)
		 * is considered to disconnected.
		 */
		ONLINE_USER_MAX_AGE  = 750,

		/**
		 * Time after which user (with offline state) is considered to
		 * disconnect.  Normally, if user sends a packet with offline
		 * status (s)he is treated as if (s)he has disconnected, yet
		 * if user sends us a message (s)he apperas with offline
		 * status and this time specifis how long shall it be kept
		 * without any activity.
		 */
		OFFLINE_USER_MAX_AGE = 400,

		/**
		 * Status sending interval.
		 */
		STATUS_RESEND        = 300
	};


	/** Vector of all opened TCP connections. */
	typedef unordered_vector<NetworkConnection*> TCPSockets;

	/** A sequence used to give each module unique name. */
	static unsigned network_id;


	/**
	 * accept()s connections from listening socket and adds them to
	 * tcpSockets vector.
	 * \throw NetException if error while accepting connection occured.
	 */
	void acceptConnections();


	/**
	 * Reads data from udpSocket and parses it.
	 * \throw NetException if error while recieving data from UDP
	 *                     socket occured.
	 * \throw xml::Error when data feed to tokenizer was not valid XML.
	 */
	void readFromUDPSocket();



	/**
	 * Reads data from given TCP connection and parses it.
	 * \param conn connection to handle.
	 * \throw NetException if error while recieving data from TCP
	 *                     socket occured.
	 * \throw xml::Error   if data was misformatted XML stream.
	 */
	void readFromTCPConnection(NetworkConnection &conn);

	/**
	 * Writes pending data to given TCP connection and parses it.
	 * \param conn connection to handle.
	 * \throw NetException if error while sending data from TCP
	 *                     socket occured.
	 */
	void writeToTCPConnection(NetworkConnection &conn);

	/**
	 * Closes TCP connection and removes all references to users.
	 * \param conn connection to close.
	 */
	void closeConnection(NetworkConnection &conn);



	/**
	 * User changed status.
	 * \param user   user that did this.
	 * \param status new state.
	 */
	void gotStatus(NetworkUser &user, User::Status status);

	/**
	 * Sends status to given user or whole network.
	 * \param user user to send status to.
	 */
	void sendStatus(NetworkUser *user);

	/**
	 * User sent a message.
	 * \param user  user who sent message.
	 * \param data  message's text.
	 * \param flags combination of sig::MessageData::ACTION and
	 *              sig::MessageData::MESSAGE flags.
	 */
	void gotMessage(NetworkUser &user, const std::string &data,
	                unsigned flags);


	NetworkUser *getUser(const User::ID &id);


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
