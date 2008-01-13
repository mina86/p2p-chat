/** \file
 * XML Tree structures implementation.
 * $Id: xmltree.cpp,v 1.1 2008/01/13 13:30:54 mina86 Exp $
 */

#include "xmltree.hpp"


namespace ppc {

namespace xml {


void Tree::openNode(const std::string &name, const Attributes &attrs){
	struct Node* parent = temp;

	if(temp->downNode == 0){
		temp->downNode = new Node(name, attrs, parent);
		temp = temp->downNode;
	}
	else{
		temp = temp->downNode;
		while(temp->nextNode != 0){
			temp = temp->nextNode;
		}
		temp->nextNode = new Node(name, attrs, parent);
		temp = temp->nextNode;
	}
}


Node* Tree::findNode(Node* node, std::string name){
	std::string tempName;
	size_t index;

	name = name.substr(1);
	while(1){
		index = name.find_first_of("/");
		tempName = name.substr(0, index);
		name = name.substr(index+1);
		node = findByName(node->downNode, tempName);
		if((node == 0) || (index == std::string::npos)){
			break;
		}
	}
	return node;
}


Node* Tree::modifyNode(Node* node, std::string name){
	std::string tempName;
	size_t index;

	name = name.substr(1);
	while(1){
		index = name.find_first_of("/");
		tempName = name.substr(0, index);
		name = name.substr(index+1);
		node = modifyByName(node, tempName);
		if(index == std::string::npos){
			break;
		}
	}
	return node;
}


Node* Tree::findByName(Node* node, const std::string& tagName){
	while(node != 0){
		if(node->tagName == tagName){
			return node;
		}
		node = node->nextNode;
	}
	return 0;
}


Node* Tree::modifyByName(Node* node, const std::string& tagName){
	struct Node* parent = node;

	if(node->downNode == 0){
		node->downNode = new Node(tagName, parent);
		node = node->downNode;
	}
	else{
		node = node->downNode;
		while(node->nextNode != 0){
			if(node->tagName == tagName){
				return node;
			}
			node = node->nextNode;
		}
		if(node->tagName == tagName){
			return node;
		}
		node->nextNode = new Node(tagName, parent);
		node = node->nextNode;
	}
	return node;
}


int Tree::getValue(const std::string& name, std::string &val){
	Node* tempNode;

	if( (tempNode = findNode(root, name)) == 0){
		return 1;
	}
	val = tempNode->tagVal;
	return 0;
}


void Tree::setValue(const std::string& name, const std::string &val){
	Node* tempNode = modifyNode(root, name);
	tempNode->tagVal = val;
}


int Tree::getAttr(const std::string& name, std::string &val){
	Node* tempNode;

	int index = name.find_first_of(":");
	std::string tagName = name.substr(0, index);
	std::string attrName = name.substr(index+1);

	if( (tempNode = findNode(root, tagName)) == 0){
		return 1;
	}

	if( tempNode->attrs.find(attrName) == tempNode->attrs.end() ){
		return 2;
	}

	val = tempNode->attrs[attrName];
	return 0;
}


void Tree::setAttr(const std::string& name, const std::string &val){
	int index = name.find_first_of(":");
	std::string tagName(name, 0, index);
	std::string attrName(name, index + 1);
	Node* tempNode = modifyNode(root, tagName);
	tempNode->attrs[attrName] = val;
}


int Tree::getAttrs(const std::string& name, Attributes &attrs){
	Node* tempNode;

	if( (tempNode = findNode(root, name)) == 0){
		return 1;
	}
	attrs = tempNode->attrs;
	return 0;
}


void Tree::printAttributes(const Attributes &attrs,
                           std::ostringstream* stout){
	Attributes::const_iterator it = attrs.begin(), end = attrs.end();
	for (; it != end; ++it) {
		*stout << " " << it->first << "=\"" << it->second << "\"";
	}
}


void Tree::printNode(Node *node, std::ostringstream* stout){
	if(node != 0){
		*stout << "<" << node->tagName;
		printAttributes(node->attrs, stout);
		*stout << ">\n";
		printNode(node->downNode, stout);
		*stout << node->tagVal << "\n";
		*stout << "</" << node->tagName << ">\n";
		printNode(node->nextNode, stout);
	}
}


void Tree::clearNode(Node* parent){
	if (parent != 0){
		clearNode(parent->downNode);
		clearNode(parent->nextNode);
		delete parent;
	}
}


}

}
