/** \file
 * Basic modules definitions.
 * $Id: application.hpp,v 1.3 2007/12/08 18:00:58 mina86 Exp $
 */

#ifndef H_APPLICATION_HPP
#define H_APPLICATION_HPP

#include <string.h>
#include <sys/select.h>
#include <map>
#include <string>

#include "signal.hpp"
#include "vector-queue.hpp"


namespace ppc {

struct Config;
struct Core;



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
	 * \param nfds number of bits set.
	 * \param rd   file descriptors ready for reading.
	 * \param wr   file descriptors ready for writtitg.
	 * \param ex   file descriptors where exception occured.
	 * \return number of operations it took care of.
	 */
	virtual int doFDs(int nfds, const fd_set *rd, const fd_set *wr,
	                  const fd_set *ex) = 0;


	/**
	 * A signal has been delivered to module.
	 * \param sig delivered signal.
	 */
	virtual void recievedSignal(const Signal &sig) = 0;


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
		: Module(*this, "/core"), mainModule(&main), config(cfg),
		  running(true) {
		modules.insert(std::make_pair(moduleName,static_cast<Module*>(this)));
		modules.insert(std::make_pair(main.moduleName, &main));
	}


	/** Runs core, returns exit status. */
	int run();


protected:
	virtual int setFDSets(fd_set *rd, fd_set *wr, fd_set *ex);
	virtual int doFDs(int nfds, const fd_set *rd, const fd_set *wr,
	                  const fd_set *ex);
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

	/** Whether application should still run. */
	bool running;


	friend struct Module;
};





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
