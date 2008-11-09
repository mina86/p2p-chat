/** \file
 * XML Tree structures definition.
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

#ifndef H_XMLNODE_HPP
#define H_XMLNODE_HPP

#include "xml-parser.hpp"

namespace ppc{

namespace xml{

/** Type for holding list of attribtues. */
typedef Parser2::Attributes Attributes;

struct CDataNode;
struct ElementNode;

/** Abstract structure of XML tree's node. */
struct Node {

	/** Returns name of node. */
	const std::string &getName() const { return name; }

	/** Returns pointer to parent of node. */
	ElementNode *getParent() { return parent; }

	/** Returns const pointer to parent of node. */
	const ElementNode *getParent() const { return parent; }

	/** Returns pointer to next node on the same level. */
	Node *getNextSibling() { return nextSibling; }

	/** Returns const pointer to next node on the same level. */
	const Node *getNextSibling() const { return nextSibling; }

	/** Sets node's pointer to next node on the same level. */
	void setNextSibling(Node *next) { nextSibling = next; }

	/** Returns \c true iff node is a root node (ie. have no parent). */
	bool isRoot() const { return !parent; }

	/** Returns \c true iff node is a CDataNode. */
	bool isCData() const { return name.empty(); }

	/** Returns \c true iff node is an ElementNode. */
	bool isElement() const { return !name.empty(); }

	/**
	 * Outputs given node to stream donated by \a out.  Data is
	 * (obviously) saved in XML format.  If this node is an Element
	 * node opening tag and closing tags are written and all elements
	 * are printed inside (yes, this is obvious as well).
	 *
	 * \param out output stream to write data to.
	 */
	virtual void printNode(FILE* out) const = 0;

	/**
	 * Cleans tree from this node down.  Does not affect siblings.
	 * Does not delete this node either.
	 */
	virtual void clearNode() = 0;

	/** Returns object casted to CDataNode type. */
	inline CDataNode &cdataNode();

	/** Returns object casted to CDataNode type. */
	inline const CDataNode &cdataNode() const;

	/** Returns object casted to ElementNode type. */
	inline ElementNode &elementNode();

	/** Returns object casted to ElementNode type. */
	inline const ElementNode &elementNode() const;


	/** Destructor. */
	virtual ~Node() { }


protected:
	/**
	 * Constructor.
	 * \param n name of node.
	 * \param p parent node.
	 * \param n next node on that level.
	 */
	Node(const std::string &n, ElementNode &p, Node *next = 0)
		: name(n), parent(&p), nextSibling(next) { }

	/** Constructor. Used to construct root. */
	Node() : name("<root>"), parent(0), nextSibling(0) { }


private:
	/** No copying allowed. */
	Node(const Node &n) { (void)n; }

	/** Name of node. */
	std::string name;

	/** Pointer to parent node. */
	ElementNode *parent;

	/** Pointer to next node on the same level. */
	Node *nextSibling;
};


/** Node holding CData information. */
struct CDataNode : public Node {
	/**
	 * Constructor.
	 * \param p parent node.
	 * \param str CData information.
	 * \param n next node on that level.
	 */
	CDataNode(ElementNode &p, const std::string &str = std::string(),
	          Node *n = 0) : Node(std::string(), p, n), data(str) { }

	/** Returns CData hold by the object. */
	const std::string &getCData() const { return data; }

	/** Returns CData hold by the object. */
	std::string &getCData() { return data; }

	/**
	 * Sets CData hold by the object.
	 * \param str string to set as value.
	 */
	void setCData(const std::string &str) { data = str; }


	virtual void printNode(FILE* stout) const;
	virtual void clearNode() {}

private:
	/** CData value. */
	std::string data;
};


/** Node to hold XML element information and make tree. */
struct ElementNode : public Node {
	/**
	 * Constructor.
	 * \param p parent node.
	 * \param n node's name.
	 * \param attrs_ node's attributes.
	 */
	ElementNode(const std::string &n, ElementNode &p,
	           const Attributes &attrs_ = Attributes())
		: Node(n, p), attrs(attrs_), firstChild(0) {}

	/** Constructor.  Used to construct root. */
	ElementNode() : Node() { }

	/** Destructor. */
	virtual ~ElementNode() {
		clearNode();
	}

	virtual void printNode(FILE *fd) const;
	virtual void clearNode();


	/**
	 * Adds child (ElementNode)to node.
	 * \param n name of node.
	 * \param attrs_ attributes of new node.
	 * \return pointer to new node.
	 */
	ElementNode* addChild(const std::string &n, const Attributes &attrs_);

	/**
	 * Adds CDataNode to node.
	 * \param n name of node.
	 * \param attrs_ attributes of new node.
	 */
	void addCData(const std::string &cleanData);

	/**
	 * Returns pointer to CDataNde inside element or 0 if it wasn't found.
	 */
	const CDataNode* findCData() const;

	/**
	 * Returns pointer to CDataNde inside element or 0 if it wasn't found.
	 */
	CDataNode* findCData() {
		return const_cast<CDataNode*>(const_cast<const ElementNode*>(this)->findCData());
	}

	/**
	 * Modifies CDataNode of node.
	 * \param newCData new value to set as CData.
	 */
	void modifyCData(const std::string &newCData);

	/**
	 * Finds node in tree.
	 * \param path path to node (e.g. /foo/bar or foo/bar ).
	 * \return pointer to founded node or 0.
	 */
	ElementNode* findNode(const std::string &path);

	/**
	 * Finds node in tree.  Creates it when if it doesn't exist.
	 * \param path path to node (e.g. /foo/bar or foo/bar ).
	 * \return pointer to founded node.
	 */
	ElementNode* modifyNode(const std::string &path);

	/** Returns node's attributes. */
	Attributes &getAttrs() {
		return attrs;
	}

	/** Returns node's attributes. */
	const Attributes &getAttrs() const {
		return attrs;
	}

	/**
	 * Returns attribute's value.
	 * \param attr name od attribute to get.
	 * \param def default value to return if attribute doesn't exist.
	 * \return value of attribute or \a def if it doesn't exist.
	 */
	const std::string &getAttr(const std::string &attr,
	                           const std::string &def = std::string()) {
		Attributes::iterator it = attrs.find(attr);
		return it == attrs.end() ? def : it->second;
	}

	/**
	 * Set attribute's value.
	 * \param attr name od attribute to get.
	 * \param val value to set as attribute.
	 */
	const void setAttr(const std::string& attr, const std::string& val){
		attrs[attr] = val;
	}

	/**
	 * Removes attribute.
	 * \param attr name od attribute to get.
	 */
	const void removeAttr(const std::string &attr){
		attrs.erase(attrs.find(attr));
	}


	/** Return's node's first child. */
	Node* getFirstChild() { return firstChild; }

	/** Return's node's first child. */
	const Node* getFirstChild() const { return firstChild; }


private:
	/**
	 * Finds node's child which name is n.
	 * \param n name of wanted node.
	 * \return pointer to wanted node or 0 if it doesn't exist.
	 */
	ElementNode *findChild(const std::string& n);

	/**
	 * Finds node's child which name is n.  Creates it if it doesn't
	 * exist.
	 * \param n name of wanted node.
	 * \return pointer to wanted node.
	 */
	ElementNode *modifyChild(const std::string& n);


	/** Attributes of node */
	Attributes attrs;

	/* First child of node */
	Node *firstChild;
};



CDataNode &Node::cdataNode() {
	return *dynamic_cast<CDataNode*>(this);
}
const CDataNode &Node::cdataNode() const {
	return *dynamic_cast<const CDataNode*>(this);
}
ElementNode &Node::elementNode() {
	return *dynamic_cast<ElementNode*>(this);
}
const ElementNode &Node::elementNode() const {
	return *dynamic_cast<const ElementNode*>(this);
}



/**
 * Parser to read files with configuration. It extends Parser2 class
 * by adding XMLTree pointer and replacing construcotr and open(),
 * close() and cdata() methods.
 */
struct Reader : public xml::Parser2 {
	/**
	 * Constructor.
	 * \param tree structure where element will be hold.
	 */
	Reader(xml::ElementNode &root) : node(&root) { }


protected:
	virtual void open(const std::string &name, const Attributes &attrs);
	virtual void close(const std::string &name);
	virtual void cdata(const std::string &data);


private:
	/** Pointer to structure where elements are hold. */
	xml::ElementNode *node;
};


}

}
#endif
