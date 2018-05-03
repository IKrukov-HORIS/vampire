
/*
 * File SubstitutionTree_Nodes.cpp.
 *
 * This file is part of the source code of the software program
 * Vampire. It is protected by applicable
 * copyright laws.
 *
 * This source code is distributed under the licence found here
 * https://vprover.github.io/license.html
 * and in the source directory
 *
 * In summary, you are allowed to use Vampire for non-commercial
 * purposes but not allowed to distribute, modify, copy, create derivatives,
 * or use in competitions. 
 * For other uses of Vampire please contact developers for a different
 * licence, which we will make an effort to provide. 
 */
/**
 * @file SubstitutionTree_Nodes.cpp
 * Different SubstitutionTree Node implementations.
 */

#include "Lib/DHMultiset.hpp"
#include "Lib/Exception.hpp"
#include "Lib/List.hpp"
#include "Lib/Metaiterators.hpp"
#include "Lib/SkipList.hpp"
#include "Lib/VirtualIterator.hpp"

#include "Index.hpp"
#include "SubstitutionTree.hpp"

namespace Indexing
{

class SubstitutionTree::UListLeaf
: public Leaf
{
public:
  inline
  UListLeaf() : _children(0), _size(0) {}
  inline
  UListLeaf(TermList ts) : Leaf(ts), _children(0), _size(0) {}
  virtual ~UListLeaf()
  {
    LDList::destroy(_children);
  }

  inline
  NodeAlgorithm algorithm() const { return UNSORTED_LIST; }
  inline
  bool isEmpty() const { return !_children; }
  inline
  int size() const { return _size; }
  inline
  bool isHigherOrder() const { return false; } 
  inline
  LDIterator allChildren()
  {
    return pvi( LDList::RefIterator(_children) );
  }
  inline
  void insert(LeafData ld)
  {
    LDList::push(ld, _children);
    _size++;
  }
  inline
  void remove(LeafData ld)
  {
    _children = LDList::remove(ld, _children);
    _size--;
  }

  CLASS_NAME(SubstitutionTree::UListLeaf);
  USE_ALLOCATOR(UListLeaf);
protected:
  typedef List<LeafData> LDList;
  LDList* _children;
  int _size;

  friend class SubstitutionTree;
};


class SubstitutionTree::HoUListLeaf
: public UListLeaf
{
public:

  inline
  HoUListLeaf(TermList ts) : UListLeaf(ts) { 
    ASS(ts.isTerm());
    Term* t = ts.term();
    ASS(t->hasVarHead());
    termType = env.signature->getVarType(t->functor());
  }
  HoUListLeaf(const UListLeaf* leaf) : UListLeaf(leaf->term) { 
    TermList ts = leaf->term;
    ASS(ts.isTerm());
    Term* t = ts.term();
    ASS(t->hasVarHead());
    termType = env.signature->getVarType(t->functor());
    _children = leaf->_children;
    _size = leaf->_size;
  }
  ~HoUListLeaf()
  {
  }

  inline
  bool isHigherOrder() const { return true; } 
  inline
  virtual OperatorType* getType() { return termType; }

  CLASS_NAME(SubstitutionTree::HoUListLeaf);
  USE_ALLOCATOR(HoUListLeaf);
private:
  OperatorType* termType;
};

class SubstitutionTree::SListLeaf
: public Leaf
{
public:
  SListLeaf() {}
  SListLeaf(TermList ts) : Leaf(ts) {}

  static SListLeaf* assimilate(Leaf* orig, bool ho = false);

  inline
  NodeAlgorithm algorithm() const { return SKIP_LIST; }
  inline
  bool isEmpty() const { return _children.isEmpty(); }
  inline
  bool isHigherOrder() const { return false; } 
#if VDEBUG
  inline
  int size() const { return _children.size(); }
#endif
  inline
  LDIterator allChildren()
  {
    return pvi( LDSkipList::RefIterator(_children) );
  }
  void insert(LeafData ld) { _children.insert(ld); }
  void remove(LeafData ld) { _children.remove(ld); }

  CLASS_NAME(SubstitutionTree::SListLeaf);
  USE_ALLOCATOR(SListLeaf);
protected:
  typedef SkipList<LeafData,LDComparator> LDSkipList;
  LDSkipList _children;

  friend class SubstitutionTree;
};


class SubstitutionTree::HoSListLeaf
: public SListLeaf
{
public:
  HoSListLeaf() {}
  HoSListLeaf(TermList ts) : SListLeaf(ts) {
    ASS(ts.isTerm());
    Term* t = ts.term();
    ASS(t->hasVarHead());
    termType = env.signature->getVarType(t->functor());
  }
  HoSListLeaf(const SListLeaf* leaf) : SListLeaf(leaf->term) {
    TermList ts = leaf->term;
    ASS(ts.isTerm());
    Term* t = ts.term();
    ASS(t->hasVarHead());
    termType = env.signature->getVarType(t->functor());
    _children.insertFromIterator(LDSkipList::Iterator(leaf->_children));
  }

  inline
  virtual OperatorType* getType() { return termType; }
  inline
  bool isHigherOrder() const { return true; } 

  CLASS_NAME(SubstitutionTree::HoSListLeaf);
  USE_ALLOCATOR(HoSListLeaf);
private:
  OperatorType* termType;
};


SubstitutionTree::Leaf* SubstitutionTree::createLeaf()
{
  return new UListLeaf();
}

SubstitutionTree::Leaf* SubstitutionTree::createLeaf(TermList ts, bool ho)
{
  if(ho){ return new HoUListLeaf(ts); }
  return new UListLeaf(ts);
}

SubstitutionTree::IntermediateNode* SubstitutionTree::createIntermediateNode(unsigned childVar,bool useC)
{
  CALL("SubstitutionTree::createIntermediateNode/2");
  if(useC){ return new UArrIntermediateNodeWithSorts(childVar); }
  return new UArrIntermediateNode(childVar);
}

SubstitutionTree::IntermediateNode* SubstitutionTree::createIntermediateNode(TermList ts, unsigned childVar,bool useC, bool ho)
{
  CALL("SubstitutionTree::createIntermediateNode/3");
  if(ho){ return new HoUArrIntermediateNode(ts, childVar); }
  if(useC){ return new UArrIntermediateNodeWithSorts(ts, childVar); }
  return new UArrIntermediateNode(ts, childVar);
}

SubstitutionTree::IntermediateNode* SubstitutionTree::convertToHigherOrder(IntermediateNode* node)
{
  ASS(!node->isHigherOrder())
  IntermediateNode* hoNode;
  if(node->algorithm() == SKIP_LIST){
    hoNode = new HoSListIntermediateNode(static_cast<SListIntermediateNode*>(node));
  } else {
    hoNode = new HoUArrIntermediateNode(static_cast<UArrIntermediateNode*>(node));
  }
  return hoNode;
}

SubstitutionTree::Leaf* SubstitutionTree::convertToHigherOrder(Leaf* leaf)
{
  ASS(!leaf->isHigherOrder())
  Leaf* hoLeaf;
  if(leaf->algorithm() == SKIP_LIST){
    hoLeaf = new HoSListLeaf(static_cast<SListLeaf*>(leaf));
  } else {
    hoLeaf = new HoUListLeaf(static_cast<UListLeaf*>(leaf));
  }
  return hoLeaf;
}

void SubstitutionTree::IntermediateNode::destroyChildren()
{
  static Stack<Node*> toDelete;
  toDelete.reset();
  toDelete.push(this);
  while(toDelete.isNonEmpty()) {
    Node* n=toDelete.pop();
    if(!n->isLeaf()) {
      IntermediateNode* in=static_cast<IntermediateNode*>(n);
      NodeIterator children=in->allChildren();
      while(children.hasNext()) {
	toDelete.push(*children.next());
      }
      in->removeAllChildren();
    }
    if(n!=this) {
      delete n;
    }
  }
}

SubstitutionTree::Node** SubstitutionTree::UArrIntermediateNode::
	childByTop(TermList t, bool canCreate)
{
  CALL("SubstitutionTree::UArrIntermediateNode::childByTop");

  for(int i=0;i<_size;i++) {
    if(TermList::sameTop(t, _nodes[i]->term)) {
      return &_nodes[i];
    }
  }
  if(canCreate) {
    mightExistAsTop(t);
    ASS_L(_size,UARR_INTERMEDIATE_NODE_MAX_SIZE);
    ASS_EQ(_nodes[_size],0);
    _nodes[++_size]=0;
    return &_nodes[_size-1];
  }
  return 0;
}

SubstitutionTree::Node** SubstitutionTree::HoUArrIntermediateNode::
  varHeadChildByType(TermList t, bool canCreate)
{
  CALL("SubstitutionTree::UArrIntermediateNode::varHeadChildByType");

  ASS(t.isTerm());
  Term* searchTerm = t.term();
  OperatorType* searchType;

  if(searchTerm->hasVarHead()){
    searchType = env.signature->getVarType(searchTerm->functor());
  } else {
    searchType = env.signature->getFunction(searchTerm->functor())->fnType();
  }

  for(int i=0;i<_varHeadChildrenSize;i++) {
    //cout << " currently checking for type equality with term " + _hoVarNodes[i]->term.toString() << endl;
    if(searchType == _hoVarNodes[i]->getType()){
      return &_hoVarNodes[i];
    }
  }
  if(canCreate) {
    ASS_L(_varHeadChildrenSize,UARR_INTERMEDIATE_NODE_MAX_SIZE);
    ASS_EQ(_hoVarNodes[_varHeadChildrenSize],0);
    _hoVarNodes[++_varHeadChildrenSize]=0;
    return &_hoVarNodes[_varHeadChildrenSize-1];
  }
  return 0;
}

void SubstitutionTree::UArrIntermediateNode::remove(TermList t)
{
  CALL("SubstitutionTree::UArrIntermediateNode::remove");

  for(int i=0;i<_size;i++) {
    if(TermList::sameTop(t, _nodes[i]->term)) {
      _size--;
      _nodes[i]=_nodes[_size];
      _nodes[_size]=0;
      return;
    }
  }
  ASSERTION_VIOLATION;
}

void SubstitutionTree::HoUArrIntermediateNode::remove(TermList t)
{
  CALL("SubstitutionTree::UArrIntermediateNode::remove");

  if(t.isTerm() && t.term()->hasVarHead()){
    Term* searchTerm = t.term();
    OperatorType* searchType = env.signature->getVarType(searchTerm->functor());
    for(int i=0;i<_varHeadChildrenSize;i++) {
      if(searchType == _hoVarNodes[i]->getType()){
        _varHeadChildrenSize--;
        _hoVarNodes[i]=_hoVarNodes[_varHeadChildrenSize];
        _hoVarNodes[_varHeadChildrenSize]=0;
        return;
      }
    }
    ASSERTION_VIOLATION;   
  }
  
  for(int i=0;i<_size;i++) {
    if(TermList::sameTop(t, _nodes[i]->term)) {
      _size--;
      _nodes[i]=_nodes[_size];
      _nodes[_size]=0;
      return;
    }
  }
  ASSERTION_VIOLATION;
}

/**
 * Take an IntermediateNode, destroy it, and return
 * SListIntermediateNode with the same content.
 */
SubstitutionTree::IntermediateNode* SubstitutionTree::SListIntermediateNode
	::assimilate(IntermediateNode* orig, bool ho)
{
  CALL("SubstitutionTree::SListIntermediateNode::assimilate");

  IntermediateNode* res= 0;
  if(ho){
    res = new HoSListIntermediateNode(orig->term, orig->childVar);
  }else if(orig->withSorts()){
    res = new SListIntermediateNodeWithSorts(orig->term, orig->childVar);
  }else{
    res = new SListIntermediateNode(orig->term, orig->childVar);
  }
  res->loadChildren(orig->allChildren());
  orig->makeEmpty();
  delete orig;
  return res;
}

/**
 * Take a Leaf, destroy it, and return SListLeaf
 * with the same content.
 */
SubstitutionTree::SListLeaf* SubstitutionTree::SListLeaf::assimilate(Leaf* orig, bool ho)
{
  CALL("SubstitutionTree::SListLeaf::assimilate");

  SListLeaf* res= 0;
  if(ho){
    res = new HoSListLeaf(orig->term);
  } else {
    res = new SListLeaf(orig->term);
  }
  res->loadChildren(orig->allChildren());
  orig->makeEmpty();
  delete orig;
  return res;
}

void SubstitutionTree::ensureLeafEfficiency(Leaf** leaf, bool ho)
{
  CALL("SubstitutionTree::ensureLeafEfficiency");

  if( (*leaf)->algorithm()==UNSORTED_LIST && (*leaf)->size()>5 ) {
    *leaf=SListLeaf::assimilate(*leaf, ho);
  }
}

void SubstitutionTree::ensureIntermediateNodeEfficiency(IntermediateNode** inode, bool ho)
{
  CALL("SubstitutionTree::ensureIntermediateNodeEfficiency");

  if( (*inode)->algorithm()==UNSORTED_LIST && (*inode)->size()>3 ) {
    cout << "ENSURING EFFICIENCECY OF NODE WITH TERM " + (*inode)->term.toString() + " and it is a higher-order node: " << ho << endl; 
    *inode=SListIntermediateNode::assimilate(*inode, ho);
  }
}

}
