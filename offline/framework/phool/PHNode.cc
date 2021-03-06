//  Author: Matthias Messer

#include "PHNode.h"

#include <cstdlib>
#include <iostream>

using namespace std;

/* to keep backward compatibility, default type of stored object is PHTable */
PHNode::PHNode() : 
  parent(NULL),
  persistent(true),
  type("PHNode"),
  objecttype("PHTable"),
  name(""),
  reset_able(true)
{
  return;
}

PHNode::PHNode(const string& n) : 
  parent(NULL),
  persistent(true),
  type("PHNode"),
  objecttype("PHTable"),
  name(n),
  reset_able(true)
{
  return;
}

PHNode::PHNode(const string &n, const string &objtype ) : 
  parent(NULL),
  persistent(true),
  type("PHNode"),
  objecttype(objtype),
  name(n),
  reset_able(true)
{
  return;
}

PHNode::~PHNode() 
{
   if (parent)
     {
       parent->forgetMe(this);
     }
}

PHNode::PHNode(const PHNode &phn):
  parent(NULL),
  persistent(phn.persistent),
  type(phn.type),
  objecttype(phn.objecttype),
  name(phn.name),
  reset_able(phn.reset_able)
{
  cout << "copy ctor not implemented because of pointer to parent" << endl;
  cout << "which needs implementing for this to be reasonable" << endl;
  exit(1);
}

PHNode &
PHNode::operator=(const PHNode&)
{
  cout << "= operator not implemented because of pointer to parent" << endl;
  cout << "which needs implementing for this to be reasonable" << endl;
  exit(1);
}

void
PHNode::setResetFlag(const int val)
{
  reset_able = (val) ? true : false;
}

PHBoolean  
PHNode::getResetFlag() const
{
  return reset_able;
}

// Implementation of external functions.
std::ostream & 
operator << (std::ostream & stream, const PHNode & node)
{
   stream << node.getType() << " : " << node.getName();

   return stream;
}

