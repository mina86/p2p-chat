/** \file
 * XML Tree structures definition.
 * $Id: xmltree.hpp,v 1.2 2008/01/17 11:31:36 mina86 Exp $
 */

#ifndef H_XMLTREE_HPP
#define H_XMLTREE_HPP

#include <string>
#include <map>
#include <sstream>

#include "xml-parser.hpp"


namespace ppc {

namespace xml {


/** Type for holding list of attribtues. */
typedef Parser2::Attributes Attributes;


/**
 * Node in Tree. It's representing one element from configuration
 * file
 */
struct Node {
	/**
	 * Constructor of node which has only name yet
	 * \param name name of element
	 * \param upNode_ pointer to parent
	 */
	Node(const std::string &name, struct Node *upNode_)
		: tagName(name), upNode(upNode_), downNode(0), nextNode(0) { }

	/**
	 * Constructor of node which has name and attributes
	 * \param name name of element
	 * \param attrs_ list of attributes
	 * \param upNode_ pointer to parent
	 */
	Node(const std::string &name, const Attributes &attrs_,
	     struct Node *upNode_)
		: tagName(name), attrs(attrs_), upNode(upNode_), downNode(0),
		  nextNode(0) { }

	friend struct Tree;


private:
	/** Name of element */
	std::string tagName;

	/** CData of element */
	std::string tagVal;

	/** List of attribtues. */
	Attributes attrs;

	/** Pointer to parent */
	struct Node *upNode;

	/** Pointer to the first on childen's list */
	struct Node *downNode;

	/** Pointer to next on the list of one parent children */
	struct Node *nextNode;
};


/**
 * Simple structure to hold configuration parameters
 */
struct Tree {
	/**
	 * Constructor of empty tree (with guard at root).
	 */
	Tree() {
		root = new Node("root", 0);
		temp = root;
	}

	/**
	 * Destructor (deletes every elements of tree at first).
	 */
	~Tree() {
		clear();
		delete root;
	}

	/**
	* Prepares tree to build, should be called when you want
	* to load new configuration file.
	*/
	void reset() {
		temp = root;
		clear();
	}

	/**
	 * Called when ConfigParser want to add new tree node at the
	 * same element as the last time.
	 * \param name  element name.
	 * \param attrs list of attributes.
	 */
	void openNode(const std::string &name, const Attributes &attrs);

	/**
	 * Called when ConfigParser want to add any CData to
	 * the last element.
	 * \param value CData.
	 */
	void fillNode(const std::string &value) {
		temp->tagVal = value;
	}

	/**
	 * Called when ConfigParser already added all children and data
	 * to the last element.
	 */
	void closeNode() {
		temp = temp->upNode;
	}

	/**
	 * Lets to get value (CData) of element
	 * \param name path to element we want to get (e.g. /foo/bar)
	 * \param val returns found value (CData) here
	 * \return 0 when element was found or 1 when wasn't
	 */
	int getValue(const std::string& name, std::string &val);

	/**
	 * Lets to set value (CData) of element
	 * \param name path to element we want to get (e.g. /foo/bar)
	 * \param val value (CData) to set
	 */
	void setValue(const std::string& name, const std::string &val);

	/**
	 * Lets to get attribute of element
	 * \param name path to element and attribute (e.g. /foo/bar:attr)
	 * \param val returns found attribute value here
	 * \return 0 when attribute was found, 1 when there is no element
	 *  with this name or 2 when there was no attribute
	 */
	int getAttr(const std::string& name, std::string &val);

	/**
	 * Lets to set attribute of element
	 * \param name path to element and attribute (e.g. /foo/bar:attr)
	 * \param val attribute value to set
	 */
	void setAttr(const std::string& name, const std::string &val);

	/**
	 * Lets to get the whole list of attributes at one time.
	 * \param name path to element (e.g. /foo/bar).
	 * \param attrs returns found list of attributes here.
	 * \return 0 when attribute was found, 1 when wasn't.
	 */
	int getAttrs(const std::string& name, Attributes &attrs);

	/**
	 * Returns the whole the as a string (in XML format) which can
	 * be saved on file.
	 * \return well-formated tree to print.
	 */
	std::string& printTree() {
		std::ostringstream stout;
		printNode(root->downNode, &stout);
		return *(new std::string(stout.str()));
	}


private:
	/** Pointer to the root of Tree */
	Node *root;
	/** Extra pointer used in building tree */
	Node *temp;

	/**
	 * Returns pointer to element (from the tree with node
	 * as a root), which tagName is equal name or 0 when there
	 * is no element with this name
	 * \param node root of tree which we are looking for in
	 * \param name path to element we are looking for (e.g. /foo/bar)
	 * \return pointer to found elemenent or 0
	 */
	Node* findNode(Node* node, std::string name);

	/**
	 * Returns pointer to element (from the list of node
	 * children), which tagName is equal _tagName or 0 when
	 * there is no element with this name
	 * \param node parent of element's list which we are looking for in
	 * \param _tagName name of element we are looking for (e.g. /bar)
	 * \return pointer to found elemenent or 0
	 */
	Node* findByName(Node* node, const std::string& _tagName);

	/**
	 * Returns pointer to element (from the tree with node
	 * as a root), which tagName is equal name. Makes that
	 * element if there is no one with this name
	 * \param node root of tree which we are looking for in
	 * \param name path to element we are looking for (e.g. /foo/bar)
	 * \return pointer to element to modify
	 */
	Node* modifyNode(Node* node, std::string name);

	/**
	 * Returns pointer to element (from the list of node
	 * children), which tagName is equal _tagName. Makes that
	 * element if there is no one with this name
	 * \param node parent of element's list which we are looking for in
	 * \param _tagName name of element we are looking for (e.g. /bar)
	 *  \return pointer to element to modify
	 */
	Node* modifyByName(Node* node, const std::string& _tagName);

	/**
	 * Clears the tree - deletes every elements
	 */
	void clear() {
		clearNode(root->downNode);
		root->downNode = 0;
	}

	/**
	 * Clears the tree with parent as a root
	 * \param parent root of tree to clear
	 */
	void clearNode(Node* parent);

	/**
	 * Prints formatted elements of tree with node as a root
	 * on stout
	 * \param node root of tree to print out
	 * \param stout stringstream used as an out
	 */
	void printNode(Node *node, std::ostringstream* stout);

	/**
	 * Prints formatted list of attributes on stout
	 * \param attrs list of attributes to print out
	 * \param stout stringstream used as an out
	 */
	void printAttributes(const Attributes &attrs, std::ostringstream* stout);
};


}

}

#endif
