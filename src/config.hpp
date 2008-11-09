/** \file
 * Config structure definition.
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

#ifndef H_CONFIG_HPP
#define H_CONFIG_HPP

#include "xml-node.hpp"

namespace ppc {

/**
 * Class maintaining program configuration.
 */
struct Config {
	/** Default Constructor. */
	Config() : root(*new xml::ElementNode()), autoDelete(true) { }

	/**
	 * Constructor.  It allows attaching Configure class inside of an
	 * XML tree creating a "fake root" element.  This may be handy if
	 * several configurations are kept in a tree and we want to feed
	 * some function with one of them.
	 *
	 * \param r reference to node which will be root.
	 * \param aDelete bool value to set autodeleting root in destructor
	 */
	Config(xml::ElementNode &r, bool aDelete = false)
		: root(r), autoDelete(aDelete) { }

	/** Destructor. */
	~Config() {
		if (autoDelete) {
			delete &root;
		}
	}


	/**
	 * Returns the whole list of attributes.
	 * \param path path to element (e.g. /foo/bar).
	 * \return pointer to Attributes or 0 when node wasn't found.
	 */
	xml::Attributes *getAttrs(const std::string& path) {
		xml::ElementNode *node = root.findNode(path);
		return node ? &node->getAttrs() : 0;
	}

	/**
	 * Returns the whole list of attributes..
	 * \param path path to element (e.g. /foo/bar).
	 * \return pointer to Attributes or 0 when node wasn't found.
	 */
	const xml::Attributes *getAttrs(const std::string& path) const {
		const xml::ElementNode *node = root.findNode(path);
		return node ? &node->getAttrs() : 0;
	}

	/**
	 * Returns value (CData) or attribute of element as string.
	 * \param path path to element we want to get (e.g. /foo/bar to
	 *             get bar cdata and /foo/bar#atr to get attribute atr
	 *             value).
	 * \param def default value to return when node wasn't found.
	 * \return value (CData) or attribute of element as string.
	 */
	const std::string& getString(const std::string &path,
			const std::string &def = std::string()) const;

	/**
	 * Returns value (CData) or attribute of element as unsigned long.
	 * \param path path to element we want to get (e.g. /foo/bar to
	 *             get bar cdata and /foo/bar#atr to get attribute atr
	 *             value).
	 * \param def default value to return when node wasn't found.
	 * \return value (CData) or attribute of element as unsigned long.
	 */
	unsigned long getUnsigned(const std::string &path,
	                          unsigned long def = 0) const;

	/**
	 * Returns value (CData) or attribute of element as long.
	 * \param path path to element we want to get (e.g. /foo/bar to
	 *             get bar cdata and /foo/bar#atr to get attribute atr
	 *             value).
	 * \param def default value to return when node wasn't found.
	 * \return value (CData) or attribute of element as long.
	 */
	long getInteger(const std::string &path, long def = 0) const;

	/**
	 * Returns value (CData) or attribute of element as double.
	 * \param path path to element we want to get (e.g. /foo/bar to
	 *             get bar cdata and /foo/bar#atr to get attribute atr
	 *             value).
	 * \param def default value to return when node wasn't found.
	 * \return value (CData) or attribute of element as double.
	 */
	double getReal(const std::string &path, double def = 0) const;

	/**
	 * Sets value (CData) or attribute of element from string.
	 * Creates element if it doesn't exist before.
	 * \param path path to element we want to get (e.g. /foo/bar to
	 *        set bar cdata and /foo/bar@atr to set attribute atr
	 *        value).
	 * \param val value to set.
	 */
	void setString(const std::string &path, const std::string &val);

	/**
	 * Sets value (CData) or attribute of element from unsigned long.
	 * Creates element if it doesn't exist before.
	 * \param path path to element we want to get (e.g. /foo/bar to
	 *        set bar cdata and /foo/bar#atr to set attribute atr
	 *        value).
	 * \param val value to set.
	 */
	void setUnsigned(const std::string &path, unsigned long val);

	/**
	 * Sets value (CData) or attribute of element from long.  Creates
	 * element if it doesn't exist before.
	 * \param path path to element we want to get (e.g. /foo/bar to
	 *        set bar cdata and /foo/bar#atr to set attribute atr
	 *        value).
	 * \param val value to set.
	 */
	void setInteger(const std::string &path, long val);

	/**
	 * Sets value (CData) or attribute of element from double.
	 * Creates element if it doesn't exist before.
	 * \param path path to element we want to get (e.g. /foo/bar to
	 *        set bar cdata and /foo/bar@atr to set attribute atr
	 *        value).
	 * \param val value to set.
	 */
	void setReal(const std::string &path, double val);


protected:
	/** Returns reference to root element. */
	xml::ElementNode &getRoot() { return root; }
	/** Returns const reference to root element. */
	const xml::ElementNode &getRoot() const { return root; }


private:
	/** Structure with configuration. */
	xml::ElementNode &root;
	/** Whether we shall delete root. */
	bool autoDelete;
};



/**
 * Class maintaining reading from and writing to files
 * with program configuration.
 */
struct ConfigFile : public Config {

	/** Constructor. */
	ConfigFile() {}

	/**
	 * Constructor which set configuration file name.  It
	 * automatically reads configuration from this file and allows to
	 * use loadConfig() and saveConfig() methods without arguments.
	 * \param fileName name of file with configuration
	 */
	ConfigFile(const std::string& fileName)
		: configFile(fileName) {
		loadConfig();
	}

	/**
	 * Loads configuration from previously entered XML file.
	 * \return 0 if everything is ok, 1 if there was problem
	 * with opening file or 2 if problem was with closing file
	 */
	int loadConfig(){
		return loadConfig(configFile);
	}
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
	int loadConfig(const std::string& fileName);

	/**
	 * Saves configuration to XML file named as fileName.
	 * Don't sets this file name as default name for next
	 * loadConfig() and saveConfig() methods calls.
	 * \param fileName name of file to save configuration
	 * \return 0 if everything is ok, 1 if there was problem
	 * with opening file or 2 if problem was with closing file
	 */
	int saveConfig(const std::string& fileName);

private:
	/** Name of configuragtion file loaded the last time. */
	std::string configFile;
};

}

#endif
