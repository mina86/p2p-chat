/** \file
 * XML Tree structures implementation.
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

#include "xml-node.hpp"

namespace ppc {

namespace xml {


void CDataNode::printNode(FILE *out) const {
	fputs(xml::escape(data).c_str(), out);
}

void ElementNode::printNode(FILE* out) const {
	fprintf(out, "<%s", getName().c_str());

	Attributes::const_iterator it = attrs.begin(), end = attrs.end();
	for (; it != end; ++it) {
		fprintf(out, " %s=\"%s\"", it->first.c_str(),
		        xml::escape(it->second).c_str());
	}

	if (!firstChild) {
		fputs("/>\n", out);
		return;
	}

	putc('>', out);
	Node *node = firstChild;
	do {
		node->printNode(out);
	} while ((node = node->getNextSibling()));
	fprintf(out, "</%s>\n", getName().c_str());
}

void ElementNode::clearNode(){
	Node *child, *next = firstChild;
	while ((child = next)) {
		next = child->getNextSibling();
		child->clearNode();
		delete child;
	}
	attrs.clear();
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
	} else {
		Node* temp = firstChild;
		while (!temp->isCData() && temp->getNextSibling()) {
			temp = temp->getNextSibling();
		}
		if (temp->isCData()){
			temp->cdataNode().getCData().append(cleanData);
		} else {
			temp->setNextSibling(new CDataNode(*this, cleanData));
		}
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

const CDataNode* ElementNode::findCData() const {
	const Node *node = firstChild;
	while (node && !node->isCData()) {
		node = node->getNextSibling();
	}
	return node ? &(node->cdataNode()) : 0;
}

ElementNode* ElementNode::findNode(const std::string& path){
	ElementNode* node = this;
	std::string nodeName, tempPath;
	size_t index;

	index = path.find_first_of("/");
	if (index == 0){
		tempPath = path.substr(1);
		while( node->getParent() ){
			node = node->getParent();
		}
	}
	else {
		tempPath = path;
	}
	while(1){
		index = tempPath.find_first_of("/");
		nodeName = tempPath.substr(0, index);
		tempPath = tempPath.substr(index+1);
		node = node->findChild(nodeName);
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

	index = path.find_first_of("/");
	if (index == 0){
		tempPath = path.substr(1);
		while( node->getParent() ){
			node = node->getParent();
		}
	}
	else {
		tempPath = path;
	}
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
	node = node->getParent();
}


}

}
