/* $Id: CoinSearchTree.hpp 1590 2013-04-10 16:48:33Z stefan $ */
// Copyright (C) 2006, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinSearchTree_H
#define CoinSearchTree_H

#include <vector>
#include <algorithm>
#include <cmath>
#include <string>

#include "CoinFinite.hpp"
#include "CoinHelperFunctions.hpp"

// #define DEBUG_PRINT

//#############################################################################

class BitVector128 {
  friend bool operator<(const BitVector128& b0, const BitVector128& b1);
private:
  unsigned int bits_[4];
public:
  BitVector128();
  BitVector128(unsigned int bits[4]);
  ~BitVector128() {}
  void set(unsigned int bits[4]);
  void setBit(int i);
  void clearBit(int i);
  std::string str() const;
};

bool operator<(const BitVector128& b0, const BitVector128& b1);

//#############################################################################

/** A class from which the real tree nodes should be derived from. Some of the
    data that undoubtedly exist in the real tree node is replicated here for
    fast access. This class is used in the various comparison functions. */
class CoinTreeNode {
protected:
    CoinTreeNode() :
	depth_(-1),
	fractionality_(-1),
	quality_(-COIN_DBL_MAX),
	true_lower_bound_(-COIN_DBL_MAX),
	preferred_() {}
    CoinTreeNode(int d,
		 int f = -1,
		 double q = -COIN_DBL_MAX,
		 double tlb = -COIN_DBL_MAX,
		 BitVector128 p = BitVector128()) :
	depth_(d),
	fractionality_(f),
	quality_(q),
	true_lower_bound_(tlb),
	preferred_(p) {}
    CoinTreeNode(const CoinTreeNode& x) :
	depth_(x.depth_),
	fractionality_(x.fractionality_),
	quality_(x.quality_),
	true_lower_bound_(x.true_lower_bound_),
	preferred_(x.preferred_) {}
    CoinTreeNode& operator=(const CoinTreeNode& x) {
        if (this != &x) {
	  depth_ = x.depth_;
	  fractionality_ = x.fractionality_;
	  quality_ = x.quality_;
	  true_lower_bound_ = x.true_lower_bound_;
	  preferred_ = x.preferred_;
	}
	return *this;
    }
private:
    /// The depth of the node in the tree
    int depth_;
    /** A measure of fractionality, e.g., the number of unsatisfied
	integrality requirements */
    int fractionality_;
    /** Some quality for the node. For normal branch-and-cut problems the LP
	relaxation value will do just fine. It is probably an OK approximation
	even if column generation is done. */
    double quality_;
    /** A true lower bound on the node. May be -infinity. For normal
	branch-and-cut problems the LP relaxation value is OK. It is different
	when column generation is done. */
    double true_lower_bound_;
    /** */
    BitVector128 preferred_;
public:
    virtual ~CoinTreeNode() {}

    inline int          getDepth()         const { return depth_; }
    inline int          getFractionality() const { return fractionality_; }
    inline double       getQuality()       const { return quality_; }
    inline double       getTrueLB()        const { return true_lower_bound_; }
    inline BitVector128 getPreferred()     const { return preferred_; }

    inline void setDepth(int d)              { depth_ = d; }
    inline void setFractionality(int f)      { fractionality_ = f; }
    inline void setQuality(double q)         { quality_ = q; }
    inline void setTrueLB(double tlb)        { true_lower_bound_ = tlb; }
    inline void setPreferred(BitVector128 p) { preferred_ = p; }
};

//==============================================================================

class CoinTreeSiblings {
private:
    CoinTreeSiblings();
    CoinTreeSiblings& operator=(const CoinTreeSiblings&);
private:
    int current_;
    int numSiblings_;
    CoinTreeNode** siblings_;
public:
    CoinTreeSiblings(const int n, CoinTreeNode** nodes) :
	current_(0), numSiblings_(n), siblings_(new CoinTreeNode*[n])
    {
	CoinDisjointCopyN(nodes, n, siblings_);
    }
    CoinTreeSiblings(const CoinTreeSiblings& s) :
	current_(s.current_),
	numSiblings_(s.numSiblings_),
	siblings_(new CoinTreeNode*[s.numSiblings_])
    {
	CoinDisjointCopyN(s.siblings_, s.numSiblings_, siblings_);
    }
    ~CoinTreeSiblings() { delete[] siblings_; }
    inline CoinTreeNode* currentNode() const { return siblings_[current_]; }
    /** returns false if cannot be advanced */
    inline bool advanceNode() { return ++current_ != numSiblings_; }
    inline int toProcess() const { return numSiblings_ - current_; }
    inline int size() const { return numSiblings_; }
    inline void printPref() const {
      for (int i = 0; i < numSiblings_; ++i) {
	std::string pref = siblings_[i]->getPreferred().str();
	printf("prefs of sibligs: sibling[%i]: %s\n", i, pref.c_str());
      }
    }
};

//#############################################################################

/** Function objects to compare search tree nodes. The comparison function
    must return true if the first argument is "better" than the second one,
    i.e., it should be processed first. */
/*@{*/
/** Depth First Search. */
struct CoinSearchTreeComparePreferred {
  static inline const char* name() { return "CoinSearchTreeComparePreferred"; }
  inline bool operator()(const CoinTreeSiblings* x,
			 const CoinTreeSiblings* y) const {
    const CoinTreeNode* xNode = x->currentNode();
    const CoinTreeNode* yNode = y->currentNode();
    const BitVector128 xPref = xNode->getPreferred();
    const BitVector128 yPref = yNode->getPreferred();
    bool retval = true;
    if (xPref < yPref) {
      retval = true;
    } else if (yPref < xPref) {
      retval = false;
    } else {
      retval = xNode->getQuality() < yNode->getQuality();
    }
#ifdef DEBUG_PRINT
    printf("Comparing xpref (%s) and ypref (%s) : %s\n",
	   xpref.str().c_str(), ypref.str().c_str(), retval ? "T" : "F");
#endif
    return retval;
  }
};

//-----------------------------------------------------------------------------
/** Depth First Search. */
struct CoinSearchTreeCompareDepth {
  static inline const char* name() { return "CoinSearchTreeCompareDepth"; }
  inline bool operator()(const CoinTreeSiblings* x,
			 const CoinTreeSiblings* y) const {
#if 1
    return x->currentNode()->getDepth() >= y->currentNode()->getDepth();
#else
    if(x->currentNode()->getDepth() > y->currentNode()->getDepth())
      return 1;
    if(x->currentNode()->getDepth() == y->currentNode()->getDepth() &&
       x->currentNode()->getQuality() <= y->currentNode()->getQuality())
      return 1;
    return 0;
#endif
  }
};

//-----------------------------------------------------------------------------
/* Breadth First Search */
struct CoinSearchTreeCompareBreadth {
  static inline const char* name() { return "CoinSearchTreeCompareBreadth"; }
  inline bool operator()(const CoinTreeSiblings* x,
			 const CoinTreeSiblings* y) const {
    return x->currentNode()->getDepth() < y->currentNode()->getDepth();
  }
};

//-----------------------------------------------------------------------------
/** Best first search */
struct CoinSearchTreeCompareBest {
  static inline const char* name() { return "CoinSearchTreeCompareBest"; }
  inline bool operator()(const CoinTreeSiblings* x,
			 const CoinTreeSiblings* y) const {
    return x->currentNode()->getQuality() < y->currentNode()->getQuality();
  }
};

//#############################################################################

class CoinSearchTreeBase
{
private:
    CoinSearchTreeBase(const CoinSearchTreeBase&);
    CoinSearchTreeBase& operator=(const CoinSearchTreeBase&);

protected:
    std::vector<CoinTreeSiblings*> candidateList_;
    int numInserted_;
    int size_;

protected:
    CoinSearchTreeBase() : candidateList_(), numInserted_(0), size_(0) {}

    virtual void realpop() = 0;
    virtual void realpush(CoinTreeSiblings* s) = 0;
    virtual void fixTop() = 0;

public:
    virtual ~CoinSearchTreeBase() {}
    virtual const char* compName() const = 0;

    inline const std::vector<CoinTreeSiblings*>& getCandidates() const {
	return candidateList_;
    }
    inline bool empty() const { return candidateList_.empty(); }
    inline int size() const { return size_; }
    inline int numInserted() const { return numInserted_; }
    inline CoinTreeNode* top() const {
      if (size_ == 0)
	return NULL;
#ifdef DEBUG_PRINT
      char output[44];
      output[43] = 0;
      candidateList_.front()->currentNode()->getPreferred().print(output);
      printf("top's pref: %s\n", output);
#endif
      return candidateList_.front()->currentNode();
    }
    /** pop will advance the \c next pointer among the siblings on the top and
	then moves the top to its correct position. #realpop is the method
	that actually removes the element from the heap */
    inline void pop() {
	CoinTreeSiblings* s = candidateList_.front();
	if (!s->advanceNode()) {
	    realpop();
	    delete s;
	} else {
	    fixTop();
	}
	--size_;
    }
    inline void push(int numNodes, CoinTreeNode** nodes,
		     const bool incrInserted = true) {
	CoinTreeSiblings* s = new CoinTreeSiblings(numNodes, nodes);
	realpush(s);
	if (incrInserted) {
	    numInserted_ += numNodes;
	}
	size_ += numNodes;
    }
    inline void push(const CoinTreeSiblings& sib,
		     const bool incrInserted = true) {
	CoinTreeSiblings* s = new CoinTreeSiblings(sib);
#ifdef DEBUG_PRINT
	s->printPref();
#endif
	realpush(s);
	if (incrInserted) {
	    numInserted_ += sib.toProcess();
	}
	size_ += sib.size();
    }
};

//#############################################################################

// #define CAN_TRUST_STL_HEAP
#ifdef CAN_TRUST_STL_HEAP

template <class Comp>
class CoinSearchTree : public CoinSearchTreeBase
{
private:
    Comp comp_;
protected:
    virtual void realpop() {
	candidateList_.pop_back();
    }
    virtual void fixTop() {
	CoinTreeSiblings* s = top();
	realpop();
	push(s, false);
    }
    virtual void realpush(CoinTreeSiblings* s) {
	nodes_.push_back(s);
	std::push_heap(candidateList_.begin(), candidateList_.end(), comp_);
    }
public:
    CoinSearchTree() : CoinSearchTreeBase(), comp_() {}
    CoinSearchTree(const CoinSearchTreeBase& t) :
	CoinSearchTreeBase(), comp_() {
	candidateList_ = t.getCandidates();
	std::make_heap(candidateList_.begin(), candidateList_.end(), comp_);
	numInserted_ = t.numInserted_;
	size_ = t.size_;
    }
    ~CoinSearchTree() {}
    const char* compName() const { return Comp::name(); }
};

#else

template <class Comp>
class CoinSearchTree : public CoinSearchTreeBase
{
private:
    Comp comp_;

protected:
    virtual void realpop() {
	candidateList_[0] = candidateList_.back();
	candidateList_.pop_back();
	fixTop();
    }
    /** After changing data in the top node, fix the heap */
    virtual void fixTop() {
	const size_t size = candidateList_.size();
	if (size > 1) {
	    CoinTreeSiblings** candidates = &candidateList_[0];
	    CoinTreeSiblings* s = candidates[0];
	    --candidates;
	    size_t pos = 1;
	    size_t ch;
	    for (ch = 2; ch < size; pos = ch, ch *= 2) {
		if (comp_(candidates[ch+1], candidates[ch]))
		    ++ch;
		if (comp_(s, candidates[ch]))
		    break;
		candidates[pos] = candidates[ch];
	    }
	    if (ch == size) {
		if (comp_(candidates[ch], s)) {
		    candidates[pos] = candidates[ch];
		    pos = ch;
		}
	    }
	    candidates[pos] = s;
	}
    }
    virtual void realpush(CoinTreeSiblings* s) {
	candidateList_.push_back(s);
	CoinTreeSiblings** candidates = &candidateList_[0];
	--candidates;
	size_t pos = candidateList_.size();
	size_t ch;
	for (ch = pos/2; ch != 0; pos = ch, ch /= 2) {
	    if (comp_(candidates[ch], s))
		break;
	    candidates[pos] = candidates[ch];
	}
	candidates[pos] = s;
    }

public:
    CoinSearchTree() : CoinSearchTreeBase(), comp_() {}
    CoinSearchTree(const CoinSearchTreeBase& t) :
	CoinSearchTreeBase(), comp_() {
	candidateList_ = t.getCandidates();
	std::sort(candidateList_.begin(), candidateList_.end(), comp_);
	numInserted_ = t.numInserted();
	size_ = t.size();
    }
    virtual ~CoinSearchTree() {}
    const char* compName() const { return Comp::name(); }
};

#endif

//#############################################################################

enum CoinNodeAction {
    CoinAddNodeToCandidates,
    CoinTestNodeForDiving,
    CoinDiveIntoNode
};

class CoinSearchTreeManager
{
private:
    CoinSearchTreeManager(const CoinSearchTreeManager&);
    CoinSearchTreeManager& operator=(const CoinSearchTreeManager&);
private:
    CoinSearchTreeBase* candidates_;
    int numSolution;
    /** Whether there is an upper bound or not. The upper bound may have come
	as input, not necessarily from a solution */
    bool hasUB_;

    /** variable used to test whether we need to reevaluate search strategy */
    bool recentlyReevaluatedSearchStrategy_;

public:
    CoinSearchTreeManager() :
	candidates_(NULL),
	numSolution(0),
	recentlyReevaluatedSearchStrategy_(true)
    {}
    virtual ~CoinSearchTreeManager() {
	delete candidates_;
    }

    inline void setTree(CoinSearchTreeBase* t) {
	delete candidates_;
	candidates_ = t;
    }
    inline CoinSearchTreeBase* getTree() const {
	return candidates_;
    }

    inline bool empty() const { return candidates_->empty(); }
    inline size_t size() const { return candidates_->size(); }
    inline size_t numInserted() const { return candidates_->numInserted(); }
    inline CoinTreeNode* top() const { return candidates_->top(); }
    inline void pop() { candidates_->pop(); }
    inline void push(CoinTreeNode* node, const bool incrInserted = true) {
	candidates_->push(1, &node, incrInserted);
    }
    inline void push(const CoinTreeSiblings& s, const bool incrInserted=true) {
	candidates_->push(s, incrInserted);
    }
    inline void push(const int n, CoinTreeNode** nodes,
		     const bool incrInserted = true) {
	candidates_->push(n, nodes, incrInserted);
    }

    inline CoinTreeNode* bestQualityCandidate() const {
	return candidates_->top();
    }
    inline double bestQuality() const {
	return candidates_->top()->getQuality();
    }
    void newSolution(double solValue);
    void reevaluateSearchStrategy();
};

//#############################################################################

#endif
