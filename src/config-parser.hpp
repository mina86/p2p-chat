/** \file
 * Config parser definition.
 * $Id: config-parser.hpp,v 1.1 2008/01/13 13:30:54 mina86 Exp $
 */

#ifndef H_CONFIG_PARSER_HPP
#define H_CONFIG_PARSER_HPP

#include "xmltree.hpp"
#include "xml-parser.hpp"

namespace ppc {

/**
 * Parser to read files with configuration. It extends Parser2 class
 * by adding XMLTree pointer and replacing construcotr and open(),
 * close() and cdata() methods.
 */
struct ConfigParser : public xml::Parser2 {
	/**
	 * Constructor.
	 * \param tree structure where element will be hold.
	 */
	ConfigParser(xml::Tree *tree_) : tree(tree_) { };


protected:
	virtual void open(const std::string &name, const Attributes &attrs);
	virtual void close(const std::string &name);
	virtual void cdata(const std::string &data);


private:
	/** Pointer to structure where elements are hold. */
	xml::Tree *tree;
};

}

#endif
