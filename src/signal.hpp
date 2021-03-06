/** \file
 * Signal definitions.
 * Copyright 2008 by Michal Nazarewicz (mina86/AT/mina86.com)
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef H_SIGNAL_HPP
#define H_SIGNAL_HPP

#include <assert.h>

#include <string>
#include <algorithm>
#include <map>

#include "user.hpp"
#include "shared-obj.hpp"


namespace ppc {


struct Module;



/**
 * Signals are the main way of inter Module communication.  Signal may
 * be addressed to single module (when ppc::Module::reciever does not
 * end with a slash) or to a group of modules (when it ends with
 * a slash; signal will be delivered to all modules that
 * ppc::Module::reciever is prefix of names of).
 *
 * Signals are identified by their type which is a string resambling
 * unix path name but with limitation that each component may consist
 * of lower case letters, digits and hypens only.  There are following
 * main classes:
 *
 * <ul>
 *   <li>\c /core used to maintain application.  This group includes
 *     requests to add or remove modules.</li>
 *   <li>\c /ui used to send information to users.</li>
 *   <li>\c /net refferring to network and internet communication.
 *     This includes requests to send message and information that
 *     message was recieved.</li>
 * </ul>
 *
 * Signals usually have data associated with them.  Signals keep
 * pointer to abstract structure Signal::Data which should be cast to
 * another structure whitch hold actual data for the signal.  The type
 * shall be deducted from signal's type (it's not very elegant
 * sollution but it will hopefully work).
 *
 * \c /core signals include:
 * <ul>
 *   <li>\c /core/tick sent by core module every second.</li>
 *   <li>\c /core/module/new sent by core module to all modules when
 *     new module is added; its argument is a sig::StringData
 *     object.</li>
 *   <li>\c /core/sig/num sent by core module each time unix signal
 *     number \c num (other then \c SIGALRM) is recieved (\c num in
 *     signal's name is replaced by signal's number); it has no
 *     argument.</li>
 *   <li>\c /core/module/removed sent by core module to all modules
 *     when module exits and is removed from list; its argument is
 *     a sig::StringData object.</li>
 *   <li>\c /core/module/exits sent to core module when sender exits
 *     and should be removed from list. Note that this signal is sort
 *     of magic -- it doesn't matter who you address it to it will
 *     always be handled by \c /core (but still address it properly to
 *     \c /core as this (invalid) behaviour may change); it has no
 *     argument.</li>
 *   <li>\c /core/module/quit sent to inform module to exit.  It is
 *     mostly used to (i) disconnect from given network (then signal
 *     is sent to given network) or to (ii) stop all modules before
 *     application exits; it has no argument.</li>
 *   <li>\c /core/module/start sent to core module to create new
 *     module; argument is not yet defined.</li>
 *   <li>\c /core/module/kill sent to core module to make it "kill"
 *     module or set of modules.  Killing means that
 *     a /core/module/quit signal will be sent to that module and if
 *     it won't exit during one minute it will be deleted; signal's
 *     argument is a sig::StringData object where data is pattern
 *     matching modules names.</li>
 * </ul>
 *
 * \c /ui signals include \c /ui/msg/debug, \c /ui/msg/info, \c
 * /ui/msg/notice and \c /ui/msg/error signals which give certain
 * messages that user interface may disyplay; their argument is
 * sig::StringData object.
 *
 * \c /net signals include:
 * <ul>
 *   <li>\c /net/status/changed sent by network module to all \c /ui/
 *     modules when user (including "our" user) changes status or
 *     display name; it is also used to inform that user went
 *     offline; its argument is sig::UserData object.</li>
 *   <li>\c /net/status/change sent to network module to request status
 *     or display name change; its argument is sig::UserData object
 *     (but some fields are ignored).</li>
 *   <li>\c /net/status/rq sent to network module to make it send
 *     a request to given user to reply with that user's status; its
 *     argument is sig::MessageData object but \a flags and \a data
 *     fields are ignored.</li>
 *   <li>\c /net/msg/got sent by network module to all \c /ui/ modules
 *     when new message is recieved; its argument is
 *     sig::MessageData object.</li>
 *   <li>\c /net/msg/send sent to network module to request message to
 *     be send to user; it's argument is sig::rMessageData
 *     object.</li>
 *   <li>\c /net/msg/sent sent by network module to all \c /ui/
 *     modules when message has been sent (the truth is that it
 *     doesn't really mean that the message was sent but only that it
 *     was added to buffer of pendig data); it's argument is
 *     sig::MessageData object.</li>
 *   <li>\c /net/conn/connected sent by network module to all \c /ui/
 *     modules when new connection has been made; its argument is
 *     sig::UsersListData object (which see for more
 *     information).</li>
 *   <li>\c /net/conn/are-you-connected sent to network module and if
 *     network module (that is connected) recieves taht signel it
 *     replies with a \c /net/conn/connected signal to sender; it has
 *     no argument.</li>
 *   <li>\c /net/conn/disconnecting sent by netowrk module to all \c
 *     /ui/ modules when module starts disconnecting from network --
 *     from this point network module will ignore most of (all)
 *     signals and won't send any packets but may send some signals
 *     (like /net/status/changed or /net/msg/got); it has no
 *     arguments.</li>
 * </ul>
 *
 * It may be tempting to use pointers or references when passing data
 * with singlas.  This is not good sollution however because pointed
 * object may be deleted when singal is delivered.  The only exception
 * is \c /net/conn/connected signals which is protected by using shared_ptr.
 */
struct Signal {
	/** Base type for data objects. */
	typedef shared_obj_base        Data;
	/** Shared pointer to data object. */
	typedef shared_obj<const Data> data_ptr;


	/**
	 * Constructor sets signal's type and reciever.
	 * \param t signal's type.
	 * \param s signal's sender.
	 * \param r reciever name pattern.
	 * \param d signal's data.
	 */
	Signal(const std::string &t = "", const std::string &s = "",
	       const std::string &r = "", const data_ptr &d = data_ptr())
		: data(d), type(t), sender(s), reciever(r) { }

	/**
	 * Constructor sets signal's type and reciever.
	 * \param t signal's type.
	 * \param s signal's sender.
	 * \param r reciever name pattern.
	 * \param d signal to get data from.
	 */
	Signal(const std::string &t, const std::string &s,
	       const std::string &r, const Signal &d)
		: data(d.data), type(t), sender(s), reciever(r) { }


	/** Returns signal's type. */
	const std::string &getType() const { return type; }

	/** Returns signal's sender. */
	const std::string &getSender() const { return sender; }

	/** Returns signal's reciever pattern. */
	const std::string &getReciever() const { return reciever; }

	/** Returns signal's data. */
	const Data *getData() const { return data.get(); }

	/**
	 * Returns signal's data cast to given type.  This method simply
	 * casts what getData() method returns to a pointer to const \a T.
	 * This comes in handy since casting value returned by getData()
	 * is common operation and it is simpler to type
	 * <tt>signal.getData<type>()</tt> then <tt>dynamic_cast<const
	 * type*>(signal.getData())</tt>.
	 *
	 * This method uses a dynamic_cast so it checks if we are casting
	 * to object of a proper type.  If not \c NULL is returned thus
	 * dereferencing will cause a \c SIGSEGV.
	 */
	template<class T>
	const T *getData() const {
		const T *const ret = dynamic_cast<const T*>(data.get());
		assert(ret == data.get());
		return ret;
	}

	/** Returns signal's data. */
	const Data *operator->() const { return data.get(); }

	/** Returns signal's data. */
	const Data &operator*() const { return *data.get(); }

	/** Returns signal's data shared pointer. */
	const data_ptr &getDataPointer() const { return data; }


	/** Zeroes all fields. */
	void clear() {
		type.clear();
		reciever = sender = type;
		data = (Data*)0;
	}


private:
	/** Shared pointer to signal data object. */
	data_ptr data;

	/** Signal's type. */
	std::string type;

	/** Signal's sender. */
	std::string sender;

	/** Signal's reciever pattern. */
	std::string reciever;
};



/** Namespace with signal's data definition. */
namespace sig {


/**
 * Signal data with single string.  Used by all the signals that pass
 * a single string as its data.
 */
struct StringData : public Signal::Data {
	/**
	 * Sets data.
	 * \param str string to set.
	 */
	StringData(const std::string &str) : data(str) { }

	/** Signal's string data. */
	std::string data;
};


/**
 * Signal data with user's status and name.  Used with modules that
 * inform about status or display name changes or requests such
 * changes.
 */
struct UserData : public Signal::Data {
	/**
	 * Sets data.
	 * \param u  user to set.
	 * \param fl what was accualy changed.
	 */
	UserData(User u, unsigned fl) : user(u), flags(fl) { }

	/**
	 * User data.  If user's IP address is zero it represents "our"
	 * user.
	 */
	User user;

	/** Flags specifying what have changed/need to be changed. */
	unsigned flags;

	/** Flags which specify what to change or what was changed. */
	enum {
		STATE        =  1,  /**< User's state was changed. */
		MESSAGE      =  2,  /**< User's status message was changed. */
		NAME         =  4,  /**< User's display name was changed. */
		CONNECTED    =  8,  /**< User's just connected. */
		DISCONNECTED = 16,  /**< User's just disconnected. */
		REQUEST      = 32   /**< Send \c rq element along with \c st. */
	};
};


/**
 * Signal data with recieved or (to be) sent message.  In former case
 * \a user field specifies who sent message (it may be our user), in
 * the later case \a user specifies who is message's reciever.
 */
struct MessageData : public StringData {
	/**
	 * Sets data.
	 * \param i   message's sender/reciever's id.
	 * \param msg message's text.
	 * \param fl  message's flags.
	 */
	MessageData(const User::ID &i, const std::string &msg, unsigned fl = 0)
		: StringData(msg), id(i), flags(fl) { }

	/**
	 * Message's sender's or reciever's (depending on signal) ID.
	 *
	 * This field does not need to be fully set (but only if \c
	 * ALLOW_UDP flag is set).  If IP address is set but port number
	 * is not set data will be sent to given IP address to given user
	 * using UDP unicast datagram.  If IP address is not set message
	 * will be sent to given user (actually all users with given nick
	 * name) using UDP multicast datagram.  If in those two scenarios
	 * nick name is empty a \c to:n attribute of sent packet won't be
	 * set and thus message will be sent to all users which listen at
	 * given address or all users in network.
	 */
	User::ID id;

	/** Possible message's flags. */
	enum {
		ACTION    =  1, /**< This is an action. */
		MESSAGE   =  2, /**< This is a message. */
		ALLOW_UDP =  4, /**< Message may be sent through UDP socket. */
		RAW       =  8, /**< \a data is a raw XML data to send, ACTION
		                      and MESSAGE flags are ignored. */
		VALIDATE = 16   /**< Message is sent onlt if user specified by
		                     \a id exists. */
	};

	/** Combination fo \c ACTION and \c MESSAGE flags. */
	unsigned flags;
};


/**
 * Signal data with users list connected to network.  This object
 * usually refers to real list maintained by network module, i.e. it
 * is not a copy.  Becase this is a shared object it is deleted when
 * all references to this object are removed thus if one module uses
 * this data (through a shared_obj class) it can continue even if
 * corresponding network module have exited, however remember to
 * operate on shared_obj not on pointer to this object itself.
 */
struct UsersListData : public Signal::Data {
	/** List of users connected to network. */
	typedef std::map<User::ID, User *> Users;

	/** Our user. */
	User ourUser;

	/** List of users connected to network. */
	Users users;

	/**
	 * Constructor.
	 * \param our our user.
	 */
	UsersListData(const User &our) : ourUser(our) { }

	/**
	 * Constructor.
	 * \param nick our user's nick name.
	 * \param port our TCP listening port.
	 */
	UsersListData(const std::string &nick, Port port)
		: ourUser(nick, Address(0, port)) { }


	/** Deletes all users in users map. */
	virtual ~UsersListData() {
		if (!users.empty()) {
			std::map<User::ID, User *>::iterator it = users.begin();
			std::map<User::ID, User *>::iterator end = users.end();
			do {
				delete it->second;
			} while (++it != end);
			users.clear();
		}
	}
};



}


}

#endif
