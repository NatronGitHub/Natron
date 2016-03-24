/* $Id: CoinModelUseful.cpp 1585 2013-04-06 20:42:02Z stefan $ */
// Copyright (C) 2005, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"

#include <cstdlib>
#include <cmath>
#include <cassert>
#include <cfloat>
#include <string>
#include <cstdio>
#include <iostream>

#include "CoinHelperFunctions.hpp"

#include "CoinModelUseful.hpp"


//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
CoinModelLink::CoinModelLink () 
 : row_(-1),
   column_(-1),
   value_(0.0),
   position_(-1),
   onRow_(true)
{
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
CoinModelLink::CoinModelLink (const CoinModelLink & rhs) 
  : row_(rhs.row_),
    column_(rhs.column_),
    value_(rhs.value_),
    position_(rhs.position_),
    onRow_(rhs.onRow_)
{
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
CoinModelLink::~CoinModelLink ()
{
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
CoinModelLink &
CoinModelLink::operator=(const CoinModelLink& rhs)
{
  if (this != &rhs) {
    row_ = rhs.row_;
    column_ = rhs.column_;
    value_ = rhs.value_;
    position_ = rhs.position_;
    onRow_ = rhs.onRow_;
  }
  return *this;
}

namespace {
 const int mmult[] = {
    262139, 259459, 256889, 254291, 251701, 249133, 246709, 244247,
    241667, 239179, 236609, 233983, 231289, 228859, 226357, 223829,
    221281, 218849, 216319, 213721, 211093, 208673, 206263, 203773,
    201233, 198637, 196159, 193603, 191161, 188701, 186149, 183761,
    181303, 178873, 176389, 173897, 171469, 169049, 166471, 163871,
    161387, 158941, 156437, 153949, 151531, 149159, 146749, 144299,
    141709, 139369, 136889, 134591, 132169, 129641, 127343, 124853,
    122477, 120163, 117757, 115361, 112979, 110567, 108179, 105727,
    103387, 101021, 98639, 96179, 93911, 91583, 89317, 86939, 84521,
    82183, 79939, 77587, 75307, 72959, 70793, 68447, 66103
  };
  const int lengthMult = static_cast<int> (sizeof(mmult) / sizeof(int));
}

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
CoinModelHash::CoinModelHash () 
  : names_(NULL),
    hash_(NULL),
    numberItems_(0),
    maximumItems_(0),
    lastSlot_(-1)
{
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
CoinModelHash::CoinModelHash (const CoinModelHash & rhs) 
  : names_(NULL),
    hash_(NULL),
    numberItems_(rhs.numberItems_),
    maximumItems_(rhs.maximumItems_),
    lastSlot_(rhs.lastSlot_)
{
  if (maximumItems_) {
    names_ = new char * [maximumItems_];
    for (int i=0;i<maximumItems_;i++) {
      names_[i]=CoinStrdup(rhs.names_[i]);
    }
    hash_ = CoinCopyOfArray(rhs.hash_,4*maximumItems_);
  }
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
CoinModelHash::~CoinModelHash ()
{
  for (int i=0;i<maximumItems_;i++) 
    free(names_[i]);
  delete [] names_;
  delete [] hash_;
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
CoinModelHash &
CoinModelHash::operator=(const CoinModelHash& rhs)
{
  if (this != &rhs) {
    for (int i=0;i<maximumItems_;i++) 
      free(names_[i]);
    delete [] names_;
    delete [] hash_;
    numberItems_ = rhs.numberItems_;
    maximumItems_ = rhs.maximumItems_;
    lastSlot_ = rhs.lastSlot_;
    if (maximumItems_) {
      names_ = new char * [maximumItems_];
      for (int i=0;i<maximumItems_;i++) {
        names_[i]=CoinStrdup(rhs.names_[i]);
      }
      hash_ = CoinCopyOfArray(rhs.hash_,4*maximumItems_);
    } else {
      names_ = NULL;
      hash_ = NULL;
    }
  }
  return *this;
}
// Set number of items
void 
CoinModelHash::setNumberItems(int number)
{
  assert (number>=0&&number<=numberItems_);
  numberItems_=number;
}
// Resize hash (also re-hashs)
void 
CoinModelHash::resize(int maxItems,bool forceReHash)
{
  assert (numberItems_<=maximumItems_);
  if (maxItems<=maximumItems_&&!forceReHash)
    return;
  int n=maximumItems_;
  maximumItems_=maxItems;
  char ** names = new char * [maximumItems_];
  int i;
  for ( i=0;i<n;i++) 
    names[i]=names_[i];
  for ( ;i<maximumItems_;i++) 
    names[i]=NULL;
  delete [] names_;
  names_ = names;
  delete [] hash_;
  int maxHash = 4 * maximumItems_;
  hash_ = new CoinModelHashLink [maxHash];
  int ipos;

  for ( i = 0; i < maxHash; i++ ) {
    hash_[i].index = -1;
    hash_[i].next = -1;
  }

  /*
   * Initialize the hash table.  Only the index of the first name that
   * hashes to a value is entered in the table; subsequent names that
   * collide with it are not entered.
   */
  for ( i = 0; i < numberItems_; ++i ) {
    if (names_[i]) {
      ipos = hashValue ( names_[i]);
      if ( hash_[ipos].index == -1 ) {
        hash_[ipos].index = i;
      }
    }
  }

  /*
   * Now take care of the names that collided in the preceding loop,
   * by finding some other entry in the table for them.
   * Since there are as many entries in the table as there are names,
   * there must be room for them.
   */
  lastSlot_ = -1;
  for ( i = 0; i < numberItems_; ++i ) {
    if (!names_[i]) 
      continue;
    char *thisName = names[i];
    ipos = hashValue ( thisName);

    while ( true ) {
      int j1 = hash_[ipos].index;

      if ( j1 == i )
	break;
      else {
	char *thisName2 = names[j1];

	if ( strcmp ( thisName, thisName2 ) == 0 ) {
	  printf ( "** duplicate name %s\n", names[i] );
          abort();
	  break;
	} else {
	  int k = hash_[ipos].next;

	  if ( k == -1 ) {
	    while ( true ) {
	      ++lastSlot_;
	      if ( lastSlot_ > numberItems_ ) {
		printf ( "** too many names\n" );
                abort();
		break;
	      }
	      if ( hash_[lastSlot_].index == -1 ) {
		break;
	      }
	    }
	    hash_[ipos].next = lastSlot_;
	    hash_[lastSlot_].index = i;
	    break;
	  } else {
	    ipos = k;
	    /* nothing worked - try it again */
	  }
	}
      }
    }
  }
  
}
// validate
void 
CoinModelHash::validateHash() const
{
  for (int i = 0; i < numberItems_; ++i ) {
    if (names_[i]) {
      assert (hash( names_[i])>=0);
    }
  }
}
// Returns index or -1
int 
CoinModelHash::hash(const char * name) const
{
  int found = -1;

  int ipos;

  /* default if we don't find anything */
  if ( !numberItems_ )
    return -1;

  ipos = hashValue ( name );
  while ( true ) {
    int j1 = hash_[ipos].index;

    if ( j1 >= 0 ) {
      char *thisName2 = names_[j1];

      if ( strcmp ( name, thisName2 ) != 0 ) {
	int k = hash_[ipos].next;

	if ( k != -1 )
	  ipos = k;
	else
	  break;
      } else {
	found = j1;
	break;
      }
    } else {
      int k = hash_[ipos].next;
      
      if ( k != -1 )
        ipos = k;
      else
        break;
    }
  }
  return found;
}
// Adds to hash
void 
CoinModelHash::addHash(int index, const char * name)
{
  // resize if necessary
  if (numberItems_>=maximumItems_) 
    resize(1000+3*numberItems_/2);
  assert (!names_[index]);
  names_[index]=CoinStrdup(name);
  int ipos = hashValue ( name);
  numberItems_ = CoinMax(numberItems_,index+1);
  if ( hash_[ipos].index <0 ) {
    hash_[ipos].index = index;
  } else {
    while ( true ) {
      int j1 = hash_[ipos].index;
      
      if ( j1 == index )
	break; // duplicate?
      else {
        if (j1>=0) {
          char *thisName2 = names_[j1];
          if ( strcmp ( name, thisName2 ) == 0 ) {
            printf ( "** duplicate name %s\n", names_[index] );
            abort();
            break;
          } else {
            int k = hash_[ipos].next;
            
            if ( k == -1 ) {
              while ( true ) {
                ++lastSlot_;
                if ( lastSlot_ > numberItems_ ) {
                  printf ( "** too many names\n" );
                  abort();
                  break;
                }
                if ( hash_[lastSlot_].index <0 && hash_[lastSlot_].next<0) {
                  break;
                }
              }
              hash_[ipos].next = lastSlot_;
              hash_[lastSlot_].index = index;
              hash_[lastSlot_].next=-1;
              break;
            } else {
              ipos = k;
            }
          }
        } else {
          //slot available
          hash_[ipos].index = index;
        }
      }
    }
  }
}
// Deletes from hash
void 
CoinModelHash::deleteHash(int index)
{
  if (index<numberItems_&&names_[index]) {
    
    int ipos = hashValue ( names_[index] );

    while ( ipos>=0 ) {
      int j1 = hash_[ipos].index;
      if ( j1!=index) {
        ipos = hash_[ipos].next;
      } else {
        hash_[ipos].index=-1; // available
        break;
      }
    }
    assert (ipos>=0);
    free(names_[index]);
    names_[index]=NULL;
  }
}
// Returns name at position (or NULL)
const char * 
CoinModelHash::name(int which) const
{
  if (which<numberItems_)
    return names_[which];
  else
    return NULL;
}
// Returns non const name at position (or NULL)
char * 
CoinModelHash::getName(int which) const
{
  if (which<numberItems_)
    return names_[which];
  else
    return NULL;
}
// Sets name at position (does not create)
void 
CoinModelHash::setName(int which,char * name ) 
{
  if (which<numberItems_)
    names_[which]=name;
}
// Returns a hash value
int 
CoinModelHash::hashValue(const char * name) const
{
  
  int n = 0;
  int j;
  int length =  static_cast<int> (strlen(name));
  // may get better spread with unsigned
  const unsigned char * name2 = reinterpret_cast<const unsigned char *> (name);
  while (length) {
    int length2 = CoinMin( length,lengthMult);
    for ( j = 0; j < length2; ++j ) {
      n += mmult[j] * name2[j];
    }
    name+=length2;
    length-=length2;
  }
  int maxHash = 4 * maximumItems_;
  return ( abs ( n ) % maxHash );	/* integer abs */
}
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################
//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
CoinModelHash2::CoinModelHash2 () 
 :   hash_(NULL),
    numberItems_(0),
    maximumItems_(0),
    lastSlot_(-1)
{
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
CoinModelHash2::CoinModelHash2 (const CoinModelHash2 & rhs) 
  : hash_(NULL),
    numberItems_(rhs.numberItems_),
    maximumItems_(rhs.maximumItems_),
    lastSlot_(rhs.lastSlot_)
{
  if (maximumItems_) {
    hash_ = CoinCopyOfArray(rhs.hash_,4*maximumItems_);
  }
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
CoinModelHash2::~CoinModelHash2 ()
{
  delete [] hash_;
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
CoinModelHash2 &
CoinModelHash2::operator=(const CoinModelHash2& rhs)
{
  if (this != &rhs) {
    delete [] hash_;
    numberItems_ = rhs.numberItems_;
    maximumItems_ = rhs.maximumItems_;
    lastSlot_ = rhs.lastSlot_;
    if (maximumItems_) {
      hash_ = CoinCopyOfArray(rhs.hash_,4*maximumItems_);
    } else {
      hash_ = NULL;
    }
  }
  return *this;
}
// Set number of items
void 
CoinModelHash2::setNumberItems(int number)
{
  assert (number>=0&&(number<=numberItems_||!numberItems_));
  numberItems_=number;
}
// Resize hash (also re-hashs)
void 
CoinModelHash2::resize(int maxItems, const CoinModelTriple * triples,bool forceReHash)
{
  assert (numberItems_<=maximumItems_||!maximumItems_);
  if (maxItems<=maximumItems_&&!forceReHash)
    return;
  if (maxItems>maximumItems_) {
    maximumItems_=maxItems;
    delete [] hash_;
    hash_ = new CoinModelHashLink [4*maximumItems_];
  }
  int maxHash = 4 * maximumItems_;
  int ipos;
  int i;
  for ( i = 0; i < maxHash; i++ ) {
    hash_[i].index = -1;
    hash_[i].next = -1;
  }

  /*
   * Initialize the hash table.  Only the index of the first name that
   * hashes to a value is entered in the table; subsequent names that
   * collide with it are not entered.
   */
  for ( i = 0; i < numberItems_; ++i ) {
    int row = static_cast<int> (rowInTriple(triples[i]));
    int column = triples[i].column;
    if (column>=0) {
      ipos = hashValue ( row, column);
      if ( hash_[ipos].index == -1 ) {
        hash_[ipos].index = i;
      }
    }
  }

  /*
   * Now take care of the entries that collided in the preceding loop,
   * by finding some other entry in the table for them.
   * Since there are as many entries in the table as there are entries,
   * there must be room for them.
   */
  lastSlot_ = -1;
  for ( i = 0; i < numberItems_; ++i ) {
    int row = static_cast<int> (rowInTriple(triples[i]));
    int column = triples[i].column;
    if (column>=0) {
      ipos = hashValue ( row, column);

      while ( true ) {
        int j1 = hash_[ipos].index;
        
        if ( j1 == i )
          break;
        else {
          int row2 = static_cast<int> (rowInTriple(triples[j1]));
          int column2 = triples[j1].column;
          if ( row==row2&&column==column2 ) {
            printf ( "** duplicate entry %d %d\n", row,column );
            abort();
            break;
          } else {
            int k = hash_[ipos].next;
            
            if ( k == -1 ) {
              while ( true ) {
                ++lastSlot_;
                if ( lastSlot_ > numberItems_ ) {
                  printf ( "** too many entries\n" );
                  abort();
                  break;
                }
                if ( hash_[lastSlot_].index == -1 ) {
                  break;
                }
              }
              hash_[ipos].next = lastSlot_;
              hash_[lastSlot_].index = i;
              break;
            } else {
              ipos = k;
            }
          }
	}
      }
    }
  }
  
}
// Returns index or -1
int 
CoinModelHash2::hash(int row, int column, const CoinModelTriple * triples) const
{
  int found = -1;

  int ipos;

  /* default if we don't find anything */
  if ( !numberItems_ )
    return -1;

  ipos = hashValue ( row, column );
  while ( true ) {
    int j1 = hash_[ipos].index;

    if ( j1 >= 0 ) {
      int row2 = static_cast<int> (rowInTriple(triples[j1]));
      int column2 = triples[j1].column;
      if ( row!=row2||column!=column2 ) {
	int k = hash_[ipos].next;
        
	if ( k != -1 )
	  ipos = k;
	else
	  break;
      } else {
	found = j1;
	break;
      }
    } else {
      int k = hash_[ipos].next;
      
      if ( k != -1 )
        ipos = k;
      else
        break;
    }
  }
  return found;
}
// Adds to hash
void 
CoinModelHash2::addHash(int index, int row, int column, const CoinModelTriple * triples)
{
  // resize if necessary
  if (numberItems_>=maximumItems_||index+1>=maximumItems_) 
    resize(CoinMax(1000+3*numberItems_/2,index+1), triples);
  int ipos = hashValue ( row, column);
  numberItems_ = CoinMax(numberItems_,index+1);
  assert (numberItems_<=maximumItems_);
  if ( hash_[ipos].index <0 ) {
    hash_[ipos].index = index;
  } else {
    while ( true ) {
      int j1 = hash_[ipos].index;
      
      if ( j1 == index ) {
	break; // duplicate??
      } else {
        if (j1 >=0 ) {
          int row2 = static_cast<int> (rowInTriple(triples[j1]));
          int column2 = triples[j1].column;
          if ( row==row2&&column==column2 ) {
            printf ( "** duplicate entry %d %d\n", row, column );
            abort();
            break;
          } else {
            int k = hash_[ipos].next;
            
            if ( k ==-1 ) {
              while ( true ) {
                ++lastSlot_;
                if ( lastSlot_ > numberItems_ ) {
                  printf ( "** too many entrys\n" );
                  abort();
                  break;
                }
                if ( hash_[lastSlot_].index <0 ) {
                  break;
                }
              }
              hash_[ipos].next = lastSlot_;
              hash_[lastSlot_].index = index;
              hash_[lastSlot_].next=-1;
              break;
            } else {
              ipos = k;
            }
          }
        } else {
          // slot available
          hash_[ipos].index = index;
        }
      }
    }
  }
}
// Deletes from hash
void 
CoinModelHash2::deleteHash(int index,int row, int column)
{
  if (index<numberItems_) {
    
    int ipos = hashValue ( row, column );

    while ( ipos>=0 ) {
      int j1 = hash_[ipos].index;
      if ( j1!=index) {
        ipos = hash_[ipos].next;
      } else {
        hash_[ipos].index=-1; // available
        break;
      }
    }
  }
}
namespace {
  const int mmult2[] = {
    262139, 259459, 256889, 254291, 251701, 249133, 246709, 244247,
    241667, 239179, 236609, 233983, 231289, 228859, 226357, 223829
  };
}
// Returns a hash value
int 
CoinModelHash2::hashValue(int row, int column) const
{
  
  // Optimizer should take out one side of if
  if (sizeof(int)==4*sizeof(char)) {
    unsigned char tempChar[4];
    
    unsigned int n = 0;
    int * temp = reinterpret_cast<int *> (tempChar);
    *temp=row;
    n += mmult2[0] * tempChar[0];
    n += mmult2[1] * tempChar[1];
    n += mmult2[2] * tempChar[2];
    n += mmult[3] * tempChar[3];
    *temp=column;
    n += mmult2[0+8] * tempChar[0];
    n += mmult2[1+8] * tempChar[1];
    n += mmult2[2+8] * tempChar[2];
    n += mmult2[3+8] * tempChar[3];
    return n % (maximumItems_<<1);
  } else {
    // ints are 8
    unsigned char tempChar[8];
    
    int n = 0;
    unsigned int j;
    int * temp = reinterpret_cast<int *> (tempChar);
    *temp=row;
    for ( j = 0; j < sizeof(int); ++j ) {
      int itemp = tempChar[j];
      n += mmult2[j] * itemp;
    }
    *temp=column;
    for ( j = 0; j < sizeof(int); ++j ) {
      int itemp = tempChar[j];
      n += mmult2[j+8] * itemp;
    }
    int maxHash = 4 * maximumItems_;
    int absN = abs(n);
    int returnValue = absN % maxHash;
    return returnValue;
  }
}
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
CoinModelLinkedList::CoinModelLinkedList () 
  : previous_(NULL),
    next_(NULL),
    first_(NULL),
    last_(NULL),
    numberMajor_(0),
    maximumMajor_(0),
    numberElements_(0),
    maximumElements_(0),
    type_(-1)
{
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
CoinModelLinkedList::CoinModelLinkedList (const CoinModelLinkedList & rhs) 
  : numberMajor_(rhs.numberMajor_),
    maximumMajor_(rhs.maximumMajor_),
    numberElements_(rhs.numberElements_),
    maximumElements_(rhs.maximumElements_),
    type_(rhs.type_)
{
  if (maximumMajor_) {
    previous_ = CoinCopyOfArray(rhs.previous_,maximumElements_);
    next_ = CoinCopyOfArray(rhs.next_,maximumElements_);
    first_ = CoinCopyOfArray(rhs.first_,maximumMajor_+1);
    last_ = CoinCopyOfArray(rhs.last_,maximumMajor_+1);
  } else {
    previous_ = NULL;
    next_ = NULL;
    first_ = NULL;
    last_ = NULL;
  }
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
CoinModelLinkedList::~CoinModelLinkedList ()
{
  delete [] previous_;
  delete [] next_;
  delete [] first_;
  delete [] last_;
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
CoinModelLinkedList &
CoinModelLinkedList::operator=(const CoinModelLinkedList& rhs)
{
  if (this != &rhs) {
    delete [] previous_;
    delete [] next_;
    delete [] first_;
    delete [] last_;
    numberMajor_ = rhs.numberMajor_;
    maximumMajor_ = rhs.maximumMajor_;
    numberElements_ = rhs.numberElements_;
    maximumElements_ = rhs.maximumElements_;
    type_ = rhs.type_;
    if (maximumMajor_) {
      previous_ = CoinCopyOfArray(rhs.previous_,maximumElements_);
      next_ = CoinCopyOfArray(rhs.next_,maximumElements_);
      first_ = CoinCopyOfArray(rhs.first_,maximumMajor_+1);
      last_ = CoinCopyOfArray(rhs.last_,maximumMajor_+1);
    } else {
      previous_ = NULL;
      next_ = NULL;
      first_ = NULL;
      last_ = NULL;
    }
  }
  return *this;
}
// Resize list - for row list maxMajor is maximum rows
void 
CoinModelLinkedList::resize(int maxMajor,int maxElements)
{
  maxMajor=CoinMax(maxMajor,maximumMajor_);
  maxElements=CoinMax(maxElements,maximumElements_);
  if (maxMajor>maximumMajor_) {
    int * first = new int [maxMajor+1];
    int free;
    if (maximumMajor_) {
      CoinMemcpyN(first_,maximumMajor_,first);
#     ifdef ZEROFAULT
      memset(first+maximumMajor_,0,(maxMajor-maximumMajor_)*sizeof(int)) ;
#     endif
      free = first_[maximumMajor_];
      first[maximumMajor_]=-1;
    } else {
      free=-1;
    }
    first[maxMajor]=free;
    delete [] first_;
    first_=first;
    int * last = new int [maxMajor+1];
    if (maximumMajor_) {
      CoinMemcpyN(last_,maximumMajor_,last);
#     ifdef ZEROFAULT
      memset(last+maximumMajor_,0,(maxMajor-maximumMajor_)*sizeof(int)) ;
#     endif
      free = last_[maximumMajor_];
      last[maximumMajor_]=-1;
    } else {
      free=-1;
    }
    last[maxMajor]=free;
    delete [] last_;
    last_=last;
    maximumMajor_ = maxMajor;
  }
  if ( maxElements>maximumElements_) {
    int * previous = new int [maxElements];
    CoinMemcpyN(previous_,numberElements_,previous);
#   ifdef ZEROFAULT
    memset(previous+numberElements_,0,
	   (maxElements-numberElements_)*sizeof(int)) ;
#   endif
    delete [] previous_;
    previous_=previous;
    int * next = new int [maxElements];
    CoinMemcpyN(next_,numberElements_,next);
#   ifdef ZEROFAULT
    memset(next+numberElements_,0,(maxElements-numberElements_)*sizeof(int)) ;
#   endif
    delete [] next_;
    next_=next;
    maximumElements_=maxElements;
  }
}
// Create list - for row list maxMajor is maximum rows
void 
CoinModelLinkedList::create(int maxMajor,int maxElements,
                            int numberMajor,int /*numberMinor*/, int type,
                            int numberElements, const CoinModelTriple * triples)
{
  maxMajor=CoinMax(maxMajor,maximumMajor_);
  maxMajor=CoinMax(maxMajor,numberMajor);
  maxElements=CoinMax(maxElements,maximumElements_);
  maxElements=CoinMax(maxElements,numberElements);
  type_=type;
  assert (!previous_);
  previous_ = new int [maxElements];
  next_ = new int [maxElements];
  maximumElements_=maxElements;
  assert (maxElements>=numberElements);
  assert (maxMajor>0&&!maximumMajor_);
  first_ = new int[maxMajor+1];
  last_ = new int[maxMajor+1];
# ifdef ZEROFAULT
  memset(previous_,0,maxElements*sizeof(int)) ;
  memset(next_,0,maxElements*sizeof(int)) ;
  memset(first_,0,(maxMajor+1)*sizeof(int)) ;
  memset(last_,0,(maxMajor+1)*sizeof(int)) ;
# endif
  assert (numberElements>=0);
  numberElements_=numberElements;
  maximumMajor_ = maxMajor;
  // do lists
  int i;
  for (i=0;i<numberMajor;i++) {
    first_[i]=-1;
    last_[i]=-1;
  }
  first_[maximumMajor_]=-1;
  last_[maximumMajor_]=-1;
  int freeChain=-1;
  for (i=0;i<numberElements;i++) {
    if (triples[i].column>=0) {
      int iMajor;
      if (!type_) {
        // for rows
        iMajor=static_cast<int> (rowInTriple(triples[i]));
      } else {
        iMajor=triples[i].column;
      }
      assert (iMajor<numberMajor);
      if (first_[iMajor]>=0) {
        // not first
        int j=last_[iMajor];
        next_[j]=i;
        previous_[i]=j;
      } else {
        // first
        first_[iMajor]=i;
        previous_[i]=-1;
      }
      last_[iMajor]=i;
    } else {
      // on deleted list
      if (freeChain>=0) {
        next_[freeChain]=i;
        previous_[i]=freeChain;
      } else {
        first_[maximumMajor_]=i;
        previous_[i]=-1;
      }
      freeChain=i;
    }
  }
  // Now clean up
  if (freeChain>=0) {
    next_[freeChain]=-1;
    last_[maximumMajor_]=freeChain;
  }
  for (i=0;i<numberMajor;i++) {
    int k=last_[i];
    if (k>=0) {
      next_[k]=-1;
      last_[i]=k;
    }
  }
  numberMajor_=numberMajor;
}
/* Adds to list - easy case i.e. add row to row list
   Returns where chain starts
*/
int 
CoinModelLinkedList::addEasy(int majorIndex, int numberOfElements, const int * indices,
                             const double * elements, CoinModelTriple * triples,
                             CoinModelHash2 & hash)
{
  assert (majorIndex<maximumMajor_);
  if (numberOfElements+numberElements_>maximumElements_) {
    resize(maximumMajor_,(3*(numberElements_+numberOfElements))/2+1000);
  }
  int first=-1;
  if (majorIndex>=numberMajor_) {
    for (int i=numberMajor_;i<=majorIndex;i++) {
      first_[i]=-1;
      last_[i]=-1;
    }
  }
  if (numberOfElements) {
    bool doHash = hash.maximumItems()!=0;
    int lastFree=last_[maximumMajor_];
    int last=last_[majorIndex];
    for (int i=0;i<numberOfElements;i++) {
      int put;
      if (lastFree>=0) {
        put=lastFree;
        lastFree=previous_[lastFree];
      } else {
        put=numberElements_;
        assert (put<maximumElements_);
        numberElements_++;
      }
      if (type_==0) {
        // row
	setRowAndStringInTriple(triples[put],majorIndex,false);
        triples[put].column=indices[i];
      } else {
        // column
	setRowAndStringInTriple(triples[put],indices[i],false);
        triples[put].column=majorIndex;
      }
      triples[put].value=elements[i];
      if (doHash)
        hash.addHash(put,static_cast<int> (rowInTriple(triples[put])),triples[put].column,triples);
      if (last>=0) {
        next_[last]=put;
      } else {
        first_[majorIndex]=put;
      }
      previous_[put]=last;
      last=put;
    }
    next_[last]=-1;
    if (last_[majorIndex]<0) {
      // first in row
      first = first_[majorIndex];
    } else {
      first = next_[last_[majorIndex]];
    }
    last_[majorIndex]=last;
    if (lastFree>=0) {
      next_[lastFree]=-1;
      last_[maximumMajor_]=lastFree;
    } else {
      first_[maximumMajor_]=-1;
      last_[maximumMajor_]=-1;
    }
  }
  numberMajor_=CoinMax(numberMajor_,majorIndex+1);
  return first;
}
/* Adds to list - hard case i.e. add row to column list
 */
void 
CoinModelLinkedList::addHard(int minorIndex, int numberOfElements, const int * indices,
                             const double * elements, CoinModelTriple * triples,
                             CoinModelHash2 & hash)
{
  int lastFree=last_[maximumMajor_];
  bool doHash = hash.maximumItems()!=0;
  for (int i=0;i<numberOfElements;i++) {
    int put;
    if (lastFree>=0) {
      put=lastFree;
      lastFree=previous_[lastFree];
    } else {
      put=numberElements_;
      assert (put<maximumElements_);
      numberElements_++;
    }
    int other=indices[i];
    if (type_==0) {
      // row
      setRowAndStringInTriple(triples[put],other,false);
      triples[put].column=minorIndex;
    } else {
      // column
      setRowAndStringInTriple(triples[put],minorIndex,false);
      triples[put].column=other;
    }
    triples[put].value=elements[i];
    if (doHash)
      hash.addHash(put,static_cast<int> (rowInTriple(triples[put])),triples[put].column,triples);
    if (other>=numberMajor_) {
      // Need to fill in null values
      fill(numberMajor_,other+1);
      numberMajor_=other+1;
    }
    int last=last_[other];
    if (last>=0) {
      next_[last]=put;
    } else {
      first_[other]=put;
    }
    previous_[put]=last;
    next_[put]=-1;
    last_[other]=put;
  }
  if (lastFree>=0) {
    next_[lastFree]=-1;
    last_[maximumMajor_]=lastFree;
  } else {
    first_[maximumMajor_]=-1;
    last_[maximumMajor_]=-1;
  }
}
/* Adds to list - hard case i.e. add row to column list
   This is when elements have been added to other copy
*/
void 
CoinModelLinkedList::addHard(int first, const CoinModelTriple * triples,
                             int firstFree, int lastFree,const int * next)
{
  first_[maximumMajor_]=firstFree;
  last_[maximumMajor_]=lastFree;
  int put=first;
  int minorIndex=-1;
  while (put>=0) {
    assert (put<maximumElements_);
    numberElements_=CoinMax(numberElements_,put+1);
    int other;
    if (type_==0) {
      // row
      other=rowInTriple(triples[put]);
      if (minorIndex>=0)
        assert(triples[put].column==minorIndex);
      else
        minorIndex=triples[put].column;
    } else {
      // column
      other=triples[put].column;
      if (minorIndex>=0)
        assert(static_cast<int> (rowInTriple(triples[put]))==minorIndex);
      else
        minorIndex=rowInTriple(triples[put]);
    }
    assert (other<maximumMajor_);
    if (other>=numberMajor_) {
      // Need to fill in null values
      fill (numberMajor_,other+1);
      numberMajor_=other+1;
    }
    int last=last_[other];
    if (last>=0) {
      next_[last]=put;
    } else {
      first_[other]=put;
    }
    previous_[put]=last;
    next_[put]=-1;
    last_[other]=put;
    put = next[put];
  }
}
/* Deletes from list - same case i.e. delete row from row list
*/
void 
CoinModelLinkedList::deleteSame(int which, CoinModelTriple * triples,
                                CoinModelHash2 & hash, bool zapTriples)
{
  assert (which>=0);
  if (which<numberMajor_) {
    int lastFree=last_[maximumMajor_];
    int put=first_[which];
    first_[which]=-1;
    while (put>=0) {
      if (hash.numberItems()) {
        // take out of hash
        hash.deleteHash(put, static_cast<int> (rowInTriple(triples[put])),triples[put].column);
      }
      if (zapTriples) {
        triples[put].column=-1;
        triples[put].value=0.0;
      }
      if (lastFree>=0) {
        next_[lastFree]=put;
      } else {
        first_[maximumMajor_]=put;
      }
      previous_[put]=lastFree;
      lastFree=put;
      put=next_[put];
    }
    if (lastFree>=0) {
      next_[lastFree]=-1;
      last_[maximumMajor_]=lastFree;
    } else {
      assert (last_[maximumMajor_]==-1);
    }
    last_[which]=-1;
  }
}
/* Deletes from list - other case i.e. delete row from column list
   This is when elements have been deleted from other copy
*/
void 
CoinModelLinkedList::updateDeleted(int /*which*/, CoinModelTriple * triples,
                                   CoinModelLinkedList & otherList)
{
  int firstFree = otherList.firstFree();
  int lastFree = otherList.lastFree();
  const int * previousOther = otherList.previous();
  assert (maximumMajor_);
  if (lastFree>=0) {
    // First free should be same
    if (first_[maximumMajor_]>=0)
      assert (firstFree==first_[maximumMajor_]);
    int last = last_[maximumMajor_];
    first_[maximumMajor_]=firstFree;
    // Maybe nothing to do
    if (last_[maximumMajor_]==lastFree)
      return;
    last_[maximumMajor_]=lastFree;
    int iMajor;
    if (!type_) {
      // for rows
      iMajor=static_cast<int> (rowInTriple(triples[lastFree]));
    } else {
      iMajor=triples[lastFree].column;
    }
    if (first_[iMajor]>=0) {
      // take out
      int previousThis = previous_[lastFree];
      int nextThis = next_[lastFree];
      if (previousThis>=0&&previousThis!=last) {
        next_[previousThis]=nextThis;
#ifndef NDEBUG
        int iTest;
        if (!type_) {
          // for rows
          iTest=static_cast<int> (rowInTriple(triples[previousThis]));
        } else {
          iTest=triples[previousThis].column;
        }
        assert (triples[previousThis].column>=0);
        assert (iTest==iMajor);
#endif
      } else {
        first_[iMajor]=nextThis;
      }
      if (nextThis>=0) {
        previous_[nextThis]=previousThis;
#ifndef NDEBUG
        int iTest;
        if (!type_) {
          // for rows
          iTest=static_cast<int> (rowInTriple(triples[nextThis]));
        } else {
          iTest=triples[nextThis].column;
        }
        assert (triples[nextThis].column>=0);
        assert (iTest==iMajor);
#endif
      } else {
        last_[iMajor]=previousThis;
      }
    }
    triples[lastFree].column=-1;
    triples[lastFree].value=0.0;
    // Do first (by which I mean last)
    next_[lastFree]=-1;
    int previous = previousOther[lastFree];
    while (previous!=last) {
      if (previous>=0) {
        if (!type_) {
          // for rows
          iMajor=static_cast<int> (rowInTriple(triples[previous]));
        } else {
          iMajor=triples[previous].column;
        }
        if (first_[iMajor]>=0) {
          // take out
          int previousThis = previous_[previous];
          int nextThis = next_[previous];
          if (previousThis>=0&&previousThis!=last) {
            next_[previousThis]=nextThis;
#ifndef NDEBUG
            int iTest;
            if (!type_) {
              // for rows
              iTest=static_cast<int> (rowInTriple(triples[previousThis]));
            } else {
              iTest=triples[previousThis].column;
            }
            assert (triples[previousThis].column>=0);
            assert (iTest==iMajor);
#endif
          } else {
            first_[iMajor]=nextThis;
          }
          if (nextThis>=0) {
            previous_[nextThis]=previousThis;
#ifndef NDEBUG
            int iTest;
            if (!type_) {
              // for rows
              iTest=static_cast<int> (rowInTriple(triples[nextThis]));
            } else {
              iTest=triples[nextThis].column;
            }
            assert (triples[nextThis].column>=0);
            assert (iTest==iMajor);
#endif
          } else {
            last_[iMajor]=previousThis;
          }
        }
        triples[previous].column=-1;
        triples[previous].value=0.0;
        next_[previous]=lastFree;
      } else {
        assert (lastFree==firstFree);
      }
      previous_[lastFree]=previous;
      lastFree=previous;
      previous = previousOther[lastFree];
    }
    if (last>=0) {
      next_[previous]=lastFree;
    } else {
      assert (firstFree==lastFree);
    }
    previous_[lastFree]=previous;
  }
}
/* Deletes one element from Row list
 */
void 
CoinModelLinkedList::deleteRowOne(int position, CoinModelTriple * triples,
                                  CoinModelHash2 & hash)
{
  int row=rowInTriple(triples[position]);
  assert (row<numberMajor_);
  if (hash.numberItems()) {
    // take out of hash
    hash.deleteHash(position, static_cast<int> (rowInTriple(triples[position])),triples[position].column);
  }
  int previous = previous_[position];
  int next = next_[position];
  // put on free list
  int lastFree=last_[maximumMajor_];
  if (lastFree>=0) {
    next_[lastFree]=position;
  } else {
    first_[maximumMajor_]=position;
    assert (last_[maximumMajor_]==-1);
  }
  last_[maximumMajor_]=position;
  previous_[position]=lastFree;
  next_[position]=-1;
  // Now take out of row
  if (previous>=0) {
    next_[previous]=next;
  } else {
    first_[row]=next;
  }
  if (next>=0) {
    previous_[next]=previous;
  } else {
    last_[row]=previous;
  }
}
/* Update column list for one element when
   one element deleted from row copy
*/
void 
CoinModelLinkedList::updateDeletedOne(int position, const CoinModelTriple * triples)
{
  assert (maximumMajor_);
  int column=triples[position].column;
  assert (column>=0&&column<numberMajor_);
  int previous = previous_[position];
  int next = next_[position];
  // put on free list
  int lastFree=last_[maximumMajor_];
  if (lastFree>=0) {
    next_[lastFree]=position;
  } else {
    first_[maximumMajor_]=position;
    assert (last_[maximumMajor_]==-1);
  }
  last_[maximumMajor_]=position;
  previous_[position]=lastFree;
  next_[position]=-1;
  // Now take out of column
  if (previous>=0) {
    next_[previous]=next;
  } else {
    first_[column]=next;
  }
  if (next>=0) {
    previous_[next]=previous;
  } else {
    last_[column]=previous;
  }
}
// Fills first,last with -1
void 
CoinModelLinkedList::fill(int first,int last)
{
  for (int i=first;i<last;i++) {
    first_[i]=-1;
    last_[i]=-1;
  }
}
/* Puts in free list from other list */
void 
CoinModelLinkedList::synchronize(CoinModelLinkedList & other)
{
  int first = other.first_[other.maximumMajor_];
  first_[maximumMajor_]=first;
  int last = other.last_[other.maximumMajor_];
  last_[maximumMajor_]=last;
  int put=first;
  while (put>=0) {
    previous_[put]=other.previous_[put];
    next_[put]=other.next_[put];
    put = next_[put];
  }
}
// Checks that links are consistent
void 
CoinModelLinkedList::validateLinks(const CoinModelTriple * triples) const
{
  char * mark = new char[maximumElements_];
  memset(mark,0,maximumElements_);
  int lastElement=-1;
  int i;
  for ( i=0;i<numberMajor_;i++) {
    int position = first_[i];
#ifndef NDEBUG
    int lastPosition=-1;
#endif
    while (position>=0) {
      assert (position==first_[i] || next_[previous_[position]]==position);
      assert (type_ || i == static_cast<int> (rowInTriple(triples[position])));  // i == iMajor for rows
      assert (!type_ || i == triples[position].column);  // i == iMajor
      assert (triples[position].column>=0);
      mark[position]=1;
      lastElement = CoinMax(lastElement,position);
#ifndef NDEBUG
      lastPosition=position;
#endif
      position = next_[position];
    }
    assert (lastPosition==last_[i]);
  }
  for (i=0;i<=lastElement;i++) {
    if (!mark[i])
      assert(triples[i].column==-1);
  }
  delete [] mark;
}
