#ifndef H_XMLNODE_HPP
#define H_XMLNODE_HPP

#include "xml-parser.hpp"

namespace ppc{

namespace xml{

/** Type for holding list of attribtues. */
typedef Parser2::Attributes Attributes;

struct CDataNode;
struct ElementNode;

/** Abstract structure of configuration tree's node. */
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

	/** 
	 * Allows to check if node is root.
	 * \return 1 if it's root and 0 otherwise.
	 */
	bool isRoot() const { return !parent; }
	
	/** 
	 * Allows to check if node is CDataNode.
	 * \return 1 if it's root and 0 otherwise.
	 */	
	bool isCData() const { return name.empty(); }
	
	/**
	 * Allows to check if node is ElementNode.
	 * \return 1 if it's root and 0 otherwise.
	 */	
	bool isElement() const { return !name.empty(); }

	/** Virtual method to print tree from this node. */
	virtual void printNode(FILE* out) = 0;
	
	/** Virtual method to clean tree from this node. */
	virtual void clearNode() = 0;

	/** Method to cast node to CDataNode. */
	inline CDataNode &cdataNode();
	
	/** Method to cast node to CDataNode. */
	inline const CDataNode &cdataNode() const;
	
	/** Method to cast node to ElementNode. */
	inline ElementNode &elementNode();
	
	/** Method to cast node to ElementNode. */
	inline const ElementNode &elementNode() const;

protected:

	/**
	 * Constructor.
	 * \param n name of node.
	 * \param p parent node.
	 * \param n next node on that level. 
	 */
	Node(const std::string &n, ElementNode &p, Node *next = 0)
		: name(n), parent(&p), nextSibling(next) { }
	
	/**
	 * Constructor.
	 * Uses to construct root.
	 */
	Node() : name("<root>"), parent(0), nextSibling(0) { }
	
	/** Destructor. */
	virtual ~Node() { }

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

/** Node to hold CData information. */
struct CDataNode : public Node {

	/**
	 * Constructor.
	 * \param p parent node.
	 * \param str CData information.	 
	 * \param n next node on that level. 
	 */
	CDataNode(ElementNode &p, const std::string &str = std::string(),
	          Node *n = 0) : Node(std::string(), p, n), data(str) { }

	/** Destructor. */
	virtual ~CDataNode() { }

	/** Allows to get CData. */
	const std::string &getCData() const { return data; }

	/** Allows to get CData. */
	std::string &getCData() { return data; }

	/** 
	 * Allows to set CData. 
	 * \param str string to set as value.
	 */
	void setCData(const std::string &str) { data = str; }
	
	/** 
	 * Allows to complete CData. 
	 * \param cleanData string to add to value.
	 */
	void completeCData(const std::string &cleanData) {
		data.append(cleanData);
	}

	/** Virtual method to print tree from this node. */	
	virtual void printNode(FILE* stout);
		
	/** Virtual method to clean tree from this node. */
	virtual void clearNode() {}

private:
	/** CData value. */
	std::string data;
};

/** Node to hold xml element information and make tree. */
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

	/**
	 * Constructor.
	 * Uses to construct root.
	 */
	ElementNode() : Node() { }

	/** Destructor. */
	virtual ~ElementNode() {
		clearTree();
	}

	/** Deletes all nodes of tree from this node (without it). */
	void clearTree();

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
	
	/** Closes node and returns pointer to parent node. */
	ElementNode* closeNode();

	/**
	 * Prints the whole tree from this node (in XML format)
	 * to file.
	 * \param fd file descriptor.
	 */
	virtual void printNode(FILE *fd);

	/**
	 * Finds CDataNode of node.
	 * \return pointer to CDataNde or 0 if it wasn't found.
	 */
	CDataNode* findCData();
	
	/**
	 * Modifies CDataNode of node.
	 * \param newCData new value to set as CData.
	 */
	void modifyCData(const std::string &newCData);
	
	/**
	 * Finds node in tree.
	 * \param path path to node (e.g. /foo/bar ).
	 * \return pointer to founded node.
	 */
	ElementNode* findNode(const std::string& path);
	
	/**
	 * Finds node in tree. Makes it when it doesn't exist.
	 * \param path path to node (e.g. /foo/bar ).
	 * \return pointer to founded node.
	 */
	ElementNode* modifyNode(const std::string& path);

	/** Allows to get attributes od node. */
	Attributes* getAttrs() {
		return &attrs;
	}

	/**
	 * Allows to get value of attribute.
	 * \param attr name od attribute to get.
	 * \param def default value to return if attribute doesn't exist.
	 * \return value of attribute or def if it doesn't exist.
	 */
	const std::string& getAttr(const std::string& attr,
	                           const std::string& def = std::string()) {
		if( attrs.find(attr) == attrs.end() ){
			return def;
		}
		return attrs[attr];
	}

	/**
	 * Allows to set value of attribute.
	 * \param attr name od attribute to get.
	 * \param val value to set as attribute.
	 */
	const void setAttr(const std::string& attr, const std::string& val){
		attrs[attr] = val;
	}
	
	/** Gets firstChild of node. */
	Node* getFirstChild() { return firstChild; }
	
private:

	/**
	 * Finds node which name is n from children.
	 * \param n name of wanted node.
	 * \return pointer to wanted node or 0 if it doesn't exist.
	 */
	ElementNode* findChild(const std::string& n);
	
	/**
	 * Finds node which name is n from children. Makes it
	 * if it doesn't exist.
	 * \param n name of wanted node.
	 * \return pointer to wanted node.
	 */
	ElementNode* modifyChild(const std::string& n);
	
	/** Deletes all nodes of tree from this node. */
	virtual void clearNode();

	/** Attributes of node */
	Attributes attrs;
	
	/* First child of node */
	Node *firstChild;
};



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
	Reader(xml::ElementNode *root) : node(root) {
		node->clearTree();
	}


protected:

	/**
	 * Called when open tag is encountered.
	 * \param name  element name.
	 * \param attrs list of attributes.
	 */
	virtual void open(const std::string &name, const Attributes &attrs);

	/**
	 * Called when element is being closed.
	 * \param name element's name.
	 */
	virtual void close(const std::string &name);
	
	/** 
	 * Called when there is some CData to add.
	 * \param data CData.
	 */
	virtual void cdata(const std::string &data);

private:

	/** Pointer to structure where elements are hold. */
	xml::ElementNode *node;
};


}

}
#endif
