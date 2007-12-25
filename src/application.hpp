/** \file
 * Basic modules definitions.
 * $Id: application.hpp,v 1.5 2007/12/25 01:32:28 mina86 Exp $
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

	/**
	 * Returns number of ticks since Core::run() was called.  This was
	 * introduced because: (i) calling time() is believed to take more
	 * time, (ii) current systam time may be changed so we cannot
	 * realy on it to grow at a rate one per second and (iii) it's
	 * easier (and faster) to save "times" some events occured and
	 * then calculate how far in the past it happend using a single
	 * variable which is increased once per second then to increase
	 * all those variables every second.
	 */
	inline unsigned long getTicks() const;
};



/**
 * Core module that select()s file descriptors, deliveres signals and
 * maintains madules list.
 */
struct Core : protected Module {
	/**
	 * Initialises core module.
	 *
	 * \param cfg  application configuration.
	 */
	Core(Config &cfg)
		: Module(*this, "/core"), config(cfg), running(true) {
		modules.insert(std::make_pair(moduleName,static_cast<Module*>(this)));
	}

	/**
	 * Sets main module.  This is a module that will run through the
	 * execution of application.  When core module notices that main
	 * module have exited it will unconditionally terminate.  Main
	 * module will be automatically added to modules list, no need to
	 * call addModule.  If module with given name already exists \c
	 * false is returned and no action taken.
	 *
	 * \param main main module.
	 * \return \c true iff module with given name does not yet exist.
	 */
	bool setMainModule(Module &main) {
		if (!addModule(main)) return false;
		mainModule = &main;
		return true;
	}


	/**
	 * Adds module to modules list.
	 * \param module Module to add.
	 * \return \c true iff module with given name does not yet exist.
	 */
	bool addModule(Module &module);


	/** Runs core, returns exit status. */
	int run();


protected:
	virtual int setFDSets(fd_set *rd, fd_set *wr, fd_set *ex);
	virtual int doFDs(int nfds, const fd_set *rd, const fd_set *wr,
	                  const fd_set *ex);
	virtual void recievedSignal(const Signal &sig);

	/* Those HAVE TO BE OVERWRITTEN, otherwise will in trouble. */
	void sendSignal(const std::string &type, const std::string &reciever,
	                Signal::Data *sigData) {
		signals.push(Signal(type, moduleName, reciever, sigData));
	}
	const Modules getModules() const { return modules; }
	const Config &getConfig() const { return config; }
	unsigned long getTicks() const { return ticks; }


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

	/** Seconds since run() was called. */
	unsigned long ticks;


	/** Whether application should still run. */
	bool running;


	/** Delivers signals to modules. */
	void deliverSignals();


	friend struct Module;
};





/* Those method below work because compiler knows core is object of
   Core structure thus it will call Core's specific method which in
   turn does what we want. */

void Module::sendSignal(const std::string &type, const std::string &reciever,
                         Signal::Data *sigData) {
	core.sendSignal(type, reciever, sigData);
}

const Module::Modules Module::getModules() const {
	return core.getModules();
}

const Config &Module::getConfig() const {
	return core.getConfig();
}

unsigned long Module::getTicks() const {
	return core.getTicks();
}


}

#endif
