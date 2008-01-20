/** \file
 * XML Tree structures implementation.
 * $Id: xml-node.cpp,v 1.2 2008/01/20 17:53:57 mina86 Exp $
 */

#include "xml-node.hpp"

namespace ppc {

namespace xml {

void CDataNode::printNode(FILE *out) {
	fputs(xml::escape(data).c_str(), out);
}

void ElementNode::printNode(FILE* out){
	fprintf(out, "<%s", getName().c_str());

	Attributes::const_iterator it = attrs.begin(), end = attrs.end();
	for (; it != end; ++it) {
		fprintf(out, " %s=\"%s\"", it->first.c_str(),
		        xml::escape(it->second).c_str());
	}

	if (!firstChild) {
		fputs("/>", out);
		return;
	}

	putc('>', out);
	Node *node = firstChild;
	do {
		node->printNode(out);
	} while ((node = node->getNextSibling()));
	fprintf(out, "</%s>", getName().c_str());
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
		ElementNode *next = new ElementNode(n, *this, attrs_);
		Node* temp = firstChild;
		while (temp->getNextSibling()) {
			temp = temp->getNextSibling();
		}
		temp->setNextSibling(next);
		return next;
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
	} else {
		Node* temp = firstChild;
		while((temp->getNextSibling() != 0)){
			if (temp->isCData()) {
				temp->cdataNode().setCData(newCData);
				return;
			}
			temp = temp->getNextSibling();
		}
		if (temp->isCData()) {
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
	if(firstChild == 0){
		ElementNode *newNode = new ElementNode(n, *this);
		firstChild = newNode;
		return newNode;
	} else {
		Node *node = firstChild;

		while (node->getNextSibling()) {
			if(node->getName() == n){
				return &(node->elementNode());
			}
			node = node->getNextSibling();
		}
		if(node->getName() == n){
			return &(node->elementNode());
		}
		ElementNode *newNode = new ElementNode(n, *this);
		node->setNextSibling(newNode);
		return newNode;
	}
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
