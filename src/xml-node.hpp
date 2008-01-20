#include <string>
#include <map>
#include <sstream>
#include "xml-parser.hpp"

namespace ppc{

namespace xml{

/** Type for holding list of attribtues. */
typedef Parser2::Attributes Attributes;

struct CDataNode;
struct ElementNode;

struct Node {
	const std::string &getName() const { return name; }
	ElementNode *getParent() { return parent; }
	const ElementNode *getParent() const { return parent; }
	Node *getNextSibling() { return nextSibling; }
	const Node *getNextSibling() const { return nextSibling; }	
	void setNextSibling(Node *next) {nextSibling = next; }
	
	bool isRoot() const { return !parent; }
	bool isCData() const { return name.empty(); }
	bool isElement() const { return !name.empty(); }

	virtual void printNode(std::ostringstream* stout) = 0;
	virtual void clearNode() = 0;
	virtual void clearTree() = 0;

	inline CDataNode &cdataNode();
	inline const CDataNode &cdataNode() const;
	inline ElementNode &elementNode();
	inline const ElementNode &elementNode() const;

  protected:
	Node(const std::string &n, ElementNode &p) : name(n), parent(&p), nextSibling(0) { }
	Node(const std::string &n, ElementNode &p, Node *next) 
		: name(n), parent(&p), nextSibling(next) { }
	Node() : name("<root>"), parent(0), nextSibling(0) { }
	virtual ~Node() { }
	
  private:
    /** No copying allowed. */
    Node(const Node &n) { (void)n; }
  
	std::string name;
	ElementNode *parent;
	Node *nextSibling;
};


struct CDataNode : public Node {
	CDataNode(ElementNode &p, const std::string &str = std::string(), Node *n = 0) 
		: Node(std::string(), p, n), data(str) { }
	virtual ~CDataNode() { }	
	const std::string &getCData() const { return data; }
	std::string &getCData() { return data; }
	void setCData(const std::string &str) { data = str; }
	void completeCData(const std::string &cleanData) { data.append(cleanData); }
	virtual void clearNode() {}
	virtual void clearTree() {};
	virtual void printNode(std::ostringstream* stout);
	
 private:
	std::string data;
};

struct ElementNode : public Node {
	ElementNode(const std::string &n, ElementNode &p, 
		const Attributes &attrs_) 
		: Node(n, p), attrs(attrs_), firstChild(0) {}
		
	ElementNode(const std::string &n, ElementNode &p) 
		: Node(n, p), firstChild(0) {}
	
	ElementNode() : Node() { }
	virtual ~ElementNode() {
		clearTree();
	}

	virtual void clearNode();
	virtual void clearTree();

	ElementNode* addChild(const std::string &n, const Attributes &attrs_);
	void addCData(const std::string &cleanData);
	void modifyCData(const std::string &newCData);
	ElementNode* closeNode();

	/**
	 * Returns the whole the as a string (in XML format) which can
	 * be saved on file.
	 * \return well-formated tree to print.
	 */
	void printNode(FILE *fd) {
		if (firstChild != 0){
			std::ostringstream stout;
			firstChild->printNode(&stout);
			std::string strout(stout.str());
			fwrite(&strout[0], sizeof(char), strout.size(), fd);
		}
	}

	/**
	 * Prints formatted elements of tree with node as a root
	 * on stout
	 * \param node root of tree to print out
	 * \param stout stringstream used as an out
	 */
	virtual void printNode(std::ostringstream* stout);

	/**
	 * Prints formatted list of attributes on stout
	 * \param attrs list of attributes to print out
	 * \param stout stringstream used as an out
	 */
	void printAttributes(const Attributes &attrs, std::ostringstream* stout);

	ElementNode* findNode(std::string n);
	ElementNode* findChild(const std::string& n);
	

ElementNode* modifyNode(const std::string& path);

ElementNode* modifyChild(const std::string& n);
	
	
	const std::string& getAttr(const std::string& attr, const std::string& def) {
		if( attrs.find(attr) == attrs.end() ){
			return def;
		}
		return attrs[attr];
	}
	
	const void setAttr(const std::string& attr, const std::string& val){
		attrs[attr] = val;
	}
	
	CDataNode* findCData(){
		Node* node = this;
		if(node != 0 && node->isElement() ){
			node = node->elementNode().firstChild;
			while(node != 0){
				if( node->isCData() ){
					return &(node->cdataNode());
				}
				node = node->getNextSibling();
			}
		}
		return 0;
	}
	
 private:
 	Attributes attrs;
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
	virtual void open(const std::string &name, const Attributes &attrs);
	virtual void close(const std::string &name);
	virtual void cdata(const std::string &data);

private:
	/** Pointer to structure where elements are hold. */
	xml::ElementNode *node;
};



}

}

