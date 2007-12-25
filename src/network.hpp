/** \file
 * Network module definition.
 * $Id: network.hpp,v 1.4 2007/12/25 01:36:27 mina86 Exp $
 */

#ifndef H_NETWORK_HPP
#define H_NETWORK_HPP

#include "application.hpp"
#include "netio.hpp"
#include "user.hpp"
#include "unordered-vector.hpp"
#include "ppcp-parser.hpp"


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
		STATUS_RESEND        = 300,

		/**
		 * Interval after which unused TCP connection will be closed.
		 */
		CONNECTION_MAX_AGE   = 600,

		/**
		 * Interval after which connection which is being closed (that
		 * is we are traying to send ppcp close tag or waiting for
		 * that tag from the other side) should be closed without
		 * waiting for proper ppcp closing tags.
		 */
		CONNECTION_CLOSING_TIMEOUT = 60,

		/**
		 * Intervfal between checking timeouts.  This may have value
		 * greater then one to save some CPU time as Network will
		 * investigate all users and all connections once every
		 * HZ_DIVIDER seconds instead of every second.
		 */
		HZ_DIVIDER           =  10
	};


	/** Vector of all opened TCP connections. */
	typedef unordered_vector<NetworkConnection*> Connections;

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
	 * Handles single token from TCP connection or UDP datagram.
	 * \param user  user given token was sent from.
	 * \param token the token.
	 */
	void handleToken(NetworkUser &user, const ppcp::Tokenizer::Token &token);

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
	 * Removes all users and closes all connections that age exceeded
	 * max age.
	 */
	void performTick();


	/**
	 * Sends status to given user or whole network.
	 * \param user user to send status to.
	 */
	void sendStatus(NetworkUser *user);


	/**
	 * Returns user with given ID.  If such user does not exist
	 * creates him/her.
	 * \param id user's ID
	 * \return Networkuser with given ID.
	 */
	NetworkUser *getUser(const User::ID &id);



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


	/** "Pointer" to map of all users connected to network. */
	shared_obj<sig::UsersListData> users;

	/** Reference to ourUser field from users. */
	User &ourUser;
};


}

#endif
