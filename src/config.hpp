/** \file
 * Config structure definition.
 * $Id: config.hpp,v 1.2 2008/01/17 11:31:36 mina86 Exp $
 */

#ifndef H_CONFIG_HPP
#define H_CONFIG_HPP

#include "config-parser.hpp"

namespace ppc {

/**
 * Class maintaining program configuration.
 */
struct Config{
	/** Constructor. */
	Config() : autoSave(false) {
		tree = new xml::Tree();
		parser = new ConfigParser(tree);
	}

	/** Constructor which set configuration file name. It
	 * automatically reads configuration from this file and
	 * allows to use loadConfig() and saveConfig() methods
	 * without arguments.
	 * \param fileName name of file with configuration
	 */
	Config(const std::string& fileName)
		: configFile(fileName), autoSave(true) {
		tree = new xml::Tree();
		parser = new ConfigParser(tree);
		loadConfig();
	}


	/** Destructor. */
	~Config() {
		delete tree;
		delete parser;
	}

	/**
	 * Loads configuration from previously entered XML file.
	 * \return 0 if everything is ok, 1 if there was problem
	 * with opening file or 2 if problem was with closing file
	 */
	int loadConfig();

	/**
	 * Saves configuration in XML file, which was used for load
	 * the last time.
	 * \return 0 if everything is ok, 1 if there was problem
	 * with opening file or 2 if problem was with closing file
	 */
	int saveConfig() {
		return saveConfig(configFile);
	}

	/**
	 * Loads configuration from XML file named as fileName.
	 * Sets this file name as default name for next loadConfig()
	 * and saveConfig() methods calls.
	 * \param fileName name of file to read configuration from
	 * \return 0 if everything is ok, 1 if there was problem
	 * with opening file or 2 if problem was with closing file
	 */
	int loadConfig(const std::string& fileName) {
		configFile = fileName;
		return loadConfig();
	}

	/**
	 * Saves configuration to XML file named as fileName.
	 * Don't sets this file name as default name for next
	 * loadConfig() and saveConfig() methods calls.
	 * \param fileName name of file to save configuration
	 * \return 0 if everything is ok, 1 if there was problem
	 * with opening file or 2 if problem was with closing file
	 */
	int saveConfig(const std::string& fileName);

	/**
	 * Changes value of automating configuration option.
	 * If it's true configuration is saved to file after
	 * every change of configuration parametr.
	 * \param set value to set
	 */
	void setAutoSave(bool set) {
		autoSave = set;
	}

	/**
	 * Checks value of automating configuration option.
	 * If it's true configuration is saved to file after
	 * every change of configuration parametr.
	 * \return present value
	 */
	bool getAutoSave() const {
		return autoSave;
	}

	/**
	 * Lets to get value (CData) of element
	 * \param name path to element we want to get (e.g. /foo/bar)
	 * \param val returns found value (CData) here
	 * \return 0 when element was found or 1 when wasn't
	 */
	int getValue(const std::string& name, std::string &val) const {
		return tree->getValue(name, val);
	}

	/**
	 * Lets to set value (CData) of element
	 * \param name path to element we want to get (e.g. /foo/bar)
	 * \param val value (CData) to set
	 */
	void setValue(const std::string& name, const std::string &val) {
		tree->setValue(name, val);
	}

	/**
	 * Lets to get attribute value of element
	 * \param name path to element and attribute (e.g. /foo/bar:attr)
	 * \param val returns found attribute value here
	 * \return 0 when attribute was found, 1 when there is no element
	 *  with this name or 2 when there was no attribute
	 */
	int getAttr(const std::string& name, std::string &val) const {
		return tree->getAttr(name, val);
	}

	/**
	 * Lets to set attribute of element
	 * \param name path to element and attribute (e.g. /foo/bar:attr)
	 * \param val attribute value to set
	 */
	void setAttr(const std::string& name, const std::string &val) {
		tree->setAttr(name, val);
	}

	/**
	 * Lets to get the whole list of attributes at one time.
	 * Where: Attributes is std::map<std::string, std::string>.
	 * \param name path to element (e.g. /foo/bar).
	 * \param attrs returns found list of attributes here.
	 * \return 0 when attribute was found, 1 when wasn't.
	 */
	int getAttrs(const std::string& name, xml::Attributes &attrs) const {
		return tree->getAttrs(name, attrs);
	}


private:
	/** Structure with configuration. */
	xml::Tree *tree;
	/** Parser used for reading configuration from file. */
	ConfigParser *parser;
	/** Name of configuragtion file loaded the last time. */
	std::string configFile;
	/** Variable to turn on and off autosaving. */
	bool autoSave;

	/**
	 * Simple method used for maintaining opening files.
	 * \param fileName name of file to open.
	 * \param mode mode used to opening file (r(ead) or w(rite)).
	 * \return opened file descriptor or 0 if there was any problem
	 */
	FILE *openFile(const std::string& fileName, const char *mode);
};

}

#endif
