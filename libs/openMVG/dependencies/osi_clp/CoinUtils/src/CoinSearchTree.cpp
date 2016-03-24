/* $Id: CoinSearchTree.cpp 1590 2013-04-10 16:48:33Z stefan $ */
// Copyright (C) 2006, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <cstdio>
#include "CoinSearchTree.hpp"

BitVector128::BitVector128()
{
  bits_[0] = 0;
  bits_[1] = 0;
  bits_[2] = 0;
  bits_[3] = 0;
}

BitVector128::BitVector128(unsigned int bits[4])
{
   set(bits);
}

void
BitVector128::set(unsigned int bits[4])
{
   bits_[0] = bits[0];
   bits_[1] = bits[1];
   bits_[2] = bits[2];
   bits_[3] = bits[3];
}

void
BitVector128::setBit(int i)
{
  int byte = i >> 5;
  int bit = i & 31;
  bits_[byte] |= (1 << bit);
}
  
void
BitVector128::clearBit(int i)
{
  int byte = i >> 5;
  int bit = i & 31;
  bits_[byte] &= ~(1 << bit);
}

std::string
BitVector128::str() const
{
  char output[33];
  output[32] = 0;
  sprintf(output, "%08X%08X%08X%08X",
	  bits_[3], bits_[2], bits_[1], bits_[0]);
  return output;
}
  
bool
operator<(const BitVector128& b0, const BitVector128& b1)
{
  if (b0.bits_[3] < b1.bits_[3])
    return true;
  if (b0.bits_[3] > b1.bits_[3])
    return false;
  if (b0.bits_[2] < b1.bits_[2])
    return true;
  if (b0.bits_[2] > b1.bits_[2])
    return false;
  if (b0.bits_[1] < b1.bits_[1])
    return true;
  if (b0.bits_[1] > b1.bits_[1])
    return false;
  return (b0.bits_[0] < b1.bits_[0]);
}

void
CoinSearchTreeManager::newSolution(double solValue)
{
    ++numSolution;
    hasUB_ = true;
    CoinTreeNode* top = candidates_->top();
    const double q = top ? top->getQuality() : solValue;
    const bool switchToDFS = fabs(q) < 1e-3 ?
	(fabs(solValue) < 0.005) : ((solValue-q)/fabs(q) < 0.005);
    if (switchToDFS &&
	dynamic_cast<CoinSearchTree<CoinSearchTreeCompareDepth>*>(candidates_) == NULL) {
	CoinSearchTree<CoinSearchTreeCompareDepth>* cands =
	    new CoinSearchTree<CoinSearchTreeCompareDepth>(*candidates_);
	delete candidates_;
	candidates_ = cands;
    }
}

void
CoinSearchTreeManager::reevaluateSearchStrategy()
{
    const int n = candidates_->numInserted() % 1000;
    /* the tests below ensure that even if this method is not invoked after
       every push(), the search strategy will be reevaluated when n is ~500 */
    if (recentlyReevaluatedSearchStrategy_) {
	if (n > 250 && n <= 500) {
	    recentlyReevaluatedSearchStrategy_ = false;
	}
    } else {
	if (n > 500) {
	    recentlyReevaluatedSearchStrategy_ = true;
	    /* we can reevaluate things... */
	}
    }
}
