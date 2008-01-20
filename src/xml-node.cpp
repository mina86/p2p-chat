/** \file
 * XML Tree structures implementation.
 * $Id: xml-node.cpp,v 1.1 2008/01/20 16:50:39 jwawer Exp $
 */

#include "xml-node.hpp"

namespace ppc {

namespace xml {

void CDataNode::printNode(std::ostringstream* stout){
	*stout << escape(data)  << "\n";
	if(getNextSibling() != 0){
		getNextSibling()->printNode(stout);
	}
}

void ElementNode::printNode(std::ostringstream* stout){
	*stout << "<" << getName();
	if (!attrs.empty() ){ 
		printAttributes(attrs, stout);
	}
	*stout << ">\n";
	if(firstChild != 0){
		firstChild->printNode(stout);
	}
	*stout << "</" << getName() << ">\n";
	if(getNextSibling() != 0){
		getNextSibling()->printNode(stout);
	}
}

void ElementNode::printAttributes(const Attributes &attrs,
                       std::ostringstream* stout){                  
	Attributes::const_iterator it = attrs.begin(), end = attrs.end();
	for (; it != end; ++it) {
		*stout << " " << it->first << "=\"" << it->second << "\"";
	}
}

void ElementNode::clearTree(){
		if(firstChild != 0){
			firstChild->clearNode();
			firstChild = 0;
		}
		if(getNextSibling() != 0){
			getNextSibling()->clearNode();
			setNextSibling(0);
		}
}

void ElementNode::clearNode(){
		if(firstChild != 0){
			firstChild->clearNode();
		}
		if(getNextSibling() != 0){
			getNextSibling()->clearNode();
		}
		delete this;
}

ElementNode* ElementNode::addChild(const std::string &n, 
									const Attributes &attrs_){
	if (firstChild == 0){
		ElementNode *node = new ElementNode(n, *this, attrs_);
		firstChild = node;
		return node;
	}
	else{
		Node* temp = firstChild;
		while(temp->getNextSibling() != 0){
			temp = temp->getNextSibling();
		}
		temp->setNextSibling(new ElementNode(n, *this, attrs_));
		return &(temp->getNextSibling()->elementNode());
	}
}

void ElementNode::addCData(const std::string &cleanData){
	if (firstChild == 0){
		CDataNode *node = new CDataNode(*this, cleanData);
		firstChild = node;
	}
	else{
		Node* temp = firstChild;
		while((temp->getNextSibling() != 0)){
			if ( temp->isCData() ){
				temp->cdataNode().completeCData(cleanData);
				return;
			}
			temp = temp->getNextSibling();
		}
		if ( temp->isCData() ){
			temp->cdataNode().completeCData(cleanData);
			return;
		}
		temp->setNextSibling(new CDataNode(*this, cleanData));
	}

}

void ElementNode::modifyCData(const std::string &newCData){
	if (firstChild == 0){
		CDataNode *node = new CDataNode(*this, newCData);
		firstChild = node;
	}
	else{
		Node* temp = firstChild;
		while((temp->getNextSibling() != 0)){
			if ( temp->isCData() ){
				temp->cdataNode().setCData(newCData);
				return;
			}
			temp = temp->getNextSibling();
		}
		if ( temp->isCData() ){
			temp->cdataNode().setCData(newCData);
			return;
		}
		temp->setNextSibling(new CDataNode(*this, newCData));
	}

}

ElementNode* ElementNode::closeNode(){
	return &(getParent()->elementNode());	
}


ElementNode* ElementNode::findNode(std::string n){
	ElementNode* node = this;
	std::string tempName;
	size_t index;
	
	n = n.substr(1);
	while(1){
		index = n.find_first_of("/");
		tempName = n.substr(0, index);
		n = n.substr(index+1);
		node = node->findChild(tempName);
		if((node == 0) || (index == std::string::npos)){
			break;	
		}
	}
	return node;
}

ElementNode* ElementNode::findChild(const std::string& n){
	Node* node = this;
	if(node != 0){
		node = node->elementNode().firstChild;
	}
	while(node != 0){
		if(node->getName() == n){
			return &(node->elementNode());
		}
		node = node->getNextSibling();
	}
	return 0;
}


ElementNode* ElementNode::modifyNode(const std::string& path){
	std::string nodeName, tempPath;
	ElementNode* node = this;
	size_t index;
	
	tempPath = path.substr(1);
	while(1){
		index = tempPath.find_first_of("/");
		nodeName = tempPath.substr(0, index);
		tempPath = tempPath.substr(index+1);
		node = node->modifyChild(nodeName);
		if(index == std::string::npos){
			break;
		}
	}
	return node;
}

ElementNode* ElementNode::modifyChild(const std::string& n){
	Node* node = this;
	ElementNode* parent = this;

	if(firstChild == 0){
		firstChild = new ElementNode(n, *parent);
		return &(firstChild->elementNode());
	}
	else{
		node = firstChild;
		while(node->getNextSibling() != 0){
			if(node->getName() == n){
				return &(node->elementNode());
			}
			node = node->getNextSibling();
		}
		if(node->getName() == n){
			return &(node->elementNode());
		}	
		node->setNextSibling(new ElementNode(n, *parent) );
		return &(node->getNextSibling()->elementNode());
	}
}

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


void Reader::open(const std::string &name, const Attributes &attrs){
	node = node->addChild(name, attrs);
}

void Reader::cdata(const std::string &data){
	std::string dirt("\n \t");
	size_t start = data.find_first_not_of(dirt);
	if (start == std::string::npos){
		return;
	}
	size_t lenght = data.find_last_not_of(dirt) - start +1;
	std::string cleanData = data.substr(start, lenght);
	node->addCData(cleanData);
}

void Reader::close(const std::string &name){
	(void)name;
	node = node->closeNode();
}


}

}
