/** \file
 * Config structure definition.
 * $Id: config.hpp,v 1.5 2008/01/20 17:58:18 mina86 Exp $
 */

#ifndef H_CONFIG_HPP
#define H_CONFIG_HPP

#include "xml-node.hpp"


namespace ppc {

/**
 * Class maintaining program configuration.
 */
struct Config {
	/** Constructor. */
	Config() : root(*new xml::ElementNode()), autoDelete(true) { }

	/** Constructor. */
	Config(xml::ElementNode &r, bool aDelete = false)
		: root(r), autoDelete(aDelete) { }

	/** Destructor. */
	~Config() {
		if (autoDelete) {
			delete &root;
		}
	}

	xml::ElementNode &getRoot() { return root; }
	const xml::ElementNode &getRoot() const { return root; }

	/**
	 * Lets to get the whole list of attributes at one time.
	 * Where: Attributes is std::map<std::string, std::string>.
	 * \param name path to element (e.g. /foo/bar).
	 * \param attrs returns found list of attributes here.
	 * \return 0 when attribute was found, 1 when wasn't.
	 */
	/* FIXME
	xml::Attributes &getAttrs(const std::string& name) const {
	//	return tree->getAttrs(name, attrs);
	}
	*/

	const std::string& getString(const std::string &path,
			const std::string &def = std::string()) const;

	unsigned long getUnsigned(const std::string &path,
	                          unsigned long def = 0) const;
	long getInteger(const std::string &path, long def = 0) const;
	double getReal(const std::string &path, double def = 0) const;

	void setString(const std::string &path, const std::string &val) {
		size_t index = path.find_first_of(":");
		std::string nodePath = path.substr(0, index);
		std::string attr;
		if(index != std::string::npos){
			attr = path.substr(index+1);
		}

		xml::ElementNode *node = root.modifyNode(nodePath);

		if (attr.empty()) {
			node->modifyCData(val);
		} else {
			node->setAttr(attr, val);
		}
	}

	void setUnsigned(const std::string &path, unsigned long val) {
		char buffer[100];
		sprintf(buffer, "%lu", val);
		setString(path, buffer);
	}

	void setInteger(const std::string &path, long val) {
		char buffer[100];
		sprintf(buffer, "%ld", val);
		setString(path, buffer);
	}

	void setReal(const std::string &path, double val) {
		char buffer[100];
		sprintf(buffer, "%f", val);
		setString(path, buffer);
	}

private:
	/** Structure with configuration. */
	xml::ElementNode &root;
	/** Whether we shall delete root. */
	bool autoDelete;
};


struct ConfigFile : public Config {

	/** Constructor. */
	ConfigFile() : autoSave(false) {}

	/** Constructor which set configuration file name. It
	 * automatically reads configuration from this file and
	 * allows to use loadConfig() and saveConfig() methods
	 * without arguments.
	 * \param fileName name of file with configuration
	 */
	ConfigFile(const std::string& fileName)
		: configFile(fileName), autoSave(true) {
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

	/**
	 * Changes value of automating configuration option.
	 * If it's true configuration is saved to file after
	 * every change of configuration parametr.
	 * \param value to set
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


private:
	/** Name of configuragtion file loaded the last time. */
	std::string configFile;
	/** Variable to turn on and off autosaving. */
	bool autoSave;
};

}

#endif
