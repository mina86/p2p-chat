/**
 * Basic modules definitions.
 * $Id: application.hpp,v 1.1 2007/12/03 14:46:33 mina86 Exp $
 */

#ifndef H_APPLICATION_HPP
#define H_APPLICATION_HPP

#include <string.h>
#include <sys/select.h>
#include <map>
#include <string>

#include "vector-queue.hpp"


namespace ppc {

struct Config;
struct Core;
struct Module;




struct Signal {
	/**
	 * Data associated with a signal.  It is an abstract class for
	 * real data containers to extend.
	 */
	struct Data {
		/** Virtual destructor. */
		virtual ~Data() { };

	protected:
		/** Constructor, zeroes reference counter. */
		Data() : references(0) { }
		/** Constructor, zeroes reference counter.
		 * \param data ignored. */
		Data(const Data &data) : references(0) { (void)data; }

	private:
		/** Reference counter. */
		unsigned references;

		/** Decreases reference counter and deletes object if it
		    reaches zero. */
		void decreaseReferences() {
			if (!--references) delete this;
		}

		/** Increases reference counter. */
		void increaseReferences() { ++references; }

		friend struct Signal;
	};



	/**
	 * Constructor sets signal's type and reciever.
	 * \param t signal's type.
	 * \param s signal's sender.
	 * \param r reciever name pattern.
	 * \param d signal's data.
	 */
	Signal(const std::string &t = "", const std::string &s = "",
	       const std::string &r = "", Data *d = 0)
		: type(t), sender(s), reciever(r), data(d) {
		if (d) d->increaseReferences();
	}


	/**
	 * Copy constructor.
	 * \param s signal to copy.
	 */
	Signal(const Signal &s)
		: type(s.type), sender(s.sender), reciever(s.reciever),
		  data(s.data) {
		if (data) data->increaseReferences();
	}


	/** Destructor. */
	~Signal() {
		if (data) data->decreaseReferences();
	}


	/**
	 * Assigment operator
	 * \param s signal to copy.
	 */
	Signal &operator=(const Signal &s) {
		if (this != &s) {
			type = s.type;
			sender = s.sender;
			reciever = s.reciever;
			if (data) data->decreaseReferences();
			data = s.data;
			if (data) data->increaseReferences();
		}
		return *this;
	}


	/** Returns signal's type. */
	const std::string &getType() const { return type; }

	/** Returns signal's sender. */
	const std::string &getSender() const { return sender; }

	/** Returns signal's reciever pattern. */
	const std::string &getReciever() const { return reciever; }

	/** Returns signal's data. */
	const Data *getData() const { return  data; }


	/**
	 * Tests whether module matches reciever pattern.
	 * \param m module to check name.
	 * \return \c true iff module's name matches pattern.
	 */
	inline bool matchReciever(const Module &m) const;


private:
	/** Signal's type. */
	std::string type;

	/** Signal's sender. */
	std::string sender;

	/** Signal's reciever pattern. */
	std::string reciever;

	/** Signal's data. */
	Data *data;
};



/**
 * An abstract class representing single module.  Each module has its
 * name and can send signals to other modules.  Module can maintain
 * a list of file descriptors witch will be select()ed in core module.
 *
 * Module's name must be unique.  Names resamble an unix absolute path
 * name but only lower case letters, digits and hypens are allowed.
 * Currently there are three types of names: <tt>/core</tt>,
 * <tt>/net/<i>proto</i>/<i>id</i></tt> (where <i>proto</i> may be
 * only <tt>ppc</tt>) and <tt>/ui/<i>type</i>/<i>id</i></tt>.
 */
struct Module {
	/** Module's name, */
	const std::string moduleName;


	/**
	 * Initialises basic variables.
	 *
	 * \param c core module.
	 * \param name module's name.
	 */
	Module(Core &c, const std::string &name) : moduleName(name), core(c) { }


	/** Destructor. */
	virtual ~Module() { }


	/**
	 * Sets bits in file descriptors sets to use in select().  Method
	 * may not clear any bits in those sets.  If given condition for
	 * a file descriptor is met doFDs() method will be called.
	 *
	 * Method must return integer one greater then biggest file
	 * descriptor number used in any set or zero if no bits were set.
	 *
	 * \param rd file descriptors to watch to see if characters become
	 *           available for reading (more precisely, to see if
	 *           a read will not block; in particular, a file
	 *           descriptor is also ready on end-of-file).
	 * \param wr file descriptors to watch to see if a write will not
	 *           block.
	 * \param ex file descriptors to watch for exceptions.
	 * \return integer one greater then biggest file descriptor number
	 *         used in any set or zero if no bits were set.
	 */
	virtual int setFDSets(fd_set *rd, fd_set *wr, fd_set *ex) = 0;

	/**
	 * Perferms operations on files in sets.  Note that set may
	 * contain file descriptors module is not interested in.  Method
	 * must return number of operations it took care of.
	 *
	 * \param rd file descriptors ready for reading.
	 * \param wr file descriptors ready for writtitg.
	 * \param ex file descriptors where exception occured.
	 * \return number of operations it took care of.
	 */
	virtual int doFDs(const fd_set *rd, const fd_set *wr, const fd_set *ex)=0;


	/**
	 * A signal has been delivered to module.
	 * \param sig delivered signal.
	 */
	virtual void recievedSignal(const Signal &sig) = 0;


	/**
	 * Tests whether module's name matches \a pattern.  If \a pattern
	 * ends with <tt>"/\*"</tt> name matches iff \a pattern with
	 * asterix stripped off is name's prefix, otherwise it matches iff
	 * name and \a pattern are equal.
	 *
	 * \param pattern pattern to match name against.
	 * \return whether name matches \a pattern.
	 */
	bool matchName(const std::string &pattern) const {
		const std::string::size_type len = pattern.length();
		return len > 1 && len <= moduleName.length() &&
			(pattern[len-1] == '*' && pattern[len-2] == '/'
			 ? !memcmp(pattern.data(), moduleName.data(), len-1)
			 : (pattern == moduleName));
	}


protected:
	/** A list of modules. */
	typedef std::map<std::string, Module*> Modules;

	/** Core module. */
	Core &core;

	/**
	 * Sends a signal.  Signal is added to core module's signal queue
	 * and will be delivered later on.
	 *
	 * \param type     signal's type.
	 * \param reciever signal's reciever.
	 * \param sigData  signal data.
	 */
	inline void sendSignal(const std::string &type,
	                       const std::string &reciever,
	                       Signal::Data *sigData);

	/** Returns list of modules. */
	inline const Modules getModules() const;

	/** Returns configuration. */
	inline const Config &getConfig() const;
};



/**
 * Core module that select()s file descriptors, deliveres signals and
 * maintains madules list.
 */
struct Core : protected Module {
	/**
	 * Initialises core module.  \a main is a main madule whitch must
	 * be running whole the time application is running.  If it stops
	 * core will exit.
	 *
	 * \param cfg  application configuration.
	 * \param main main module.
	 */
	Core(Config &cfg, Module &main)
		: Module(*this, "/core"), mainModule(&main), config(cfg) {
		modules.insert(std::make_pair(moduleName,static_cast<Module*>(this)));
		modules.insert(std::make_pair(main.moduleName, &main));
	}


	/** Runs core, returns exit status. */
	int run();


protected:
	virtual int setFDSets(fd_set *rd, fd_set *wr, fd_set *ex);
	virtual int doFDs(const fd_set *rd, const fd_set *wr, const fd_set *ex);
	virtual void recievedSignal(const Signal &sig);

	void sendSignal(const std::string &type, const std::string &reciever,
	                Signal::Data *sigData) {
		signals.push(Signal(type, moduleName, reciever, sigData));
	}
	const Modules getModules() const { return modules; }
	const Config &getConfig() const { return config; }


private:
	/** Signals queue. */
	typedef std::queue<Signal, std::vector<Signal> > Queue;

	/** Modules list. */
	Modules modules;

	/** Main module, if it stops core stops. */
	Module *mainModule;

	/** Signals queue. */
	Queue signals;

	/** Application configuration. */
	Config &config;


	friend struct Module;
};




bool Signal::matchReciever(const Module &m) const {
	return m.matchName(reciever);
}


void Module::sendSignal(const std::string &type, const std::string &reciever,
                         Signal::Data *sigData) {
	core.signals.push(Signal(type, moduleName, reciever, sigData));
}

const Module::Modules Module::getModules() const {
	return core.modules;
}

const Config &Module::getConfig() const {
	return core.config;
}


}

#endif
