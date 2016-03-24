/* $Id: CoinModelUseful.hpp 1416 2011-04-17 09:57:29Z stefan $ */
// Copyright (C) 2005, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinModelUseful_H
#define CoinModelUseful_H


#include <cstdlib>
#include <cmath>
#include <cassert>
#include <cfloat>
#include <cstring>
#include <cstdio>
#include <iostream>


#include "CoinPragma.hpp"

/**
   This is for various structures/classes needed by CoinModel.

   CoinModelLink
   CoinModelLinkedList
   CoinModelHash
*/
/// for going through row or column

class CoinModelLink {
  
public:
  /**@name Constructors, destructor */
   //@{
   /** Default constructor. */
   CoinModelLink();
   /** Destructor */
   ~CoinModelLink();
   //@}

   /**@name Copy method */
   //@{
   /** The copy constructor. */
   CoinModelLink(const CoinModelLink&);
  /// =
   CoinModelLink& operator=(const CoinModelLink&);
   //@}

   /**@name Sets and gets method */
   //@{
  /// Get row
  inline int row() const
  { return row_;}
  /// Get column
  inline int column() const
  { return column_;}
  /// Get value
  inline double value() const
  { return value_;}
  /// Get value
  inline double element() const
  { return value_;}
  /// Get position
  inline int position() const
  { return position_;}
  /// Get onRow
  inline bool onRow() const
  { return onRow_;}
  /// Set row
  inline void setRow(int row)
  { row_=row;}
  /// Set column
  inline void setColumn(int column)
  { column_=column;}
  /// Set value
  inline void setValue(double value)
  { value_=value;}
  /// Set value
  inline void setElement(double value)
  { value_=value;}
  /// Set position
  inline void setPosition(int position)
  { position_=position;}
  /// Set onRow
  inline void setOnRow(bool onRow)
  { onRow_=onRow;}
   //@}

private:
  /**@name Data members */
  //@{
  /// Row
  int row_;
  /// Column
  int column_;
  /// Value as double
  double value_;
  /// Position in data
  int position_;
  /// If on row chain
  bool onRow_;
  //@}
};

/// for linked lists
// for specifying triple
typedef struct {
  // top bit is nonzero if string
  // rest is row
  unsigned int row;
  //CoinModelRowIndex row;
  int column;
  double value; // If string then index into strings
} CoinModelTriple;
inline int rowInTriple(const CoinModelTriple & triple)
{ return triple.row&0x7fffffff;}
inline void setRowInTriple(CoinModelTriple & triple,int iRow)
{ triple.row = iRow|(triple.row&0x80000000);}
inline bool stringInTriple(const CoinModelTriple & triple)
{ return (triple.row&0x80000000)!=0;}
inline void setStringInTriple(CoinModelTriple & triple,bool string)
{ triple.row = (string ? 0x80000000 : 0)|(triple.row&0x7fffffff);}
inline void setRowAndStringInTriple(CoinModelTriple & triple,
				    int iRow,bool string)
{ triple.row = (string ? 0x80000000 : 0)|iRow;}
/// for names and hashing
// for hashing
typedef struct {
  int index, next;
} CoinModelHashLink;

/* Function type.  */
typedef double (*func_t) (double);

/// For string evaluation
/* Data type for links in the chain of symbols.  */
struct symrec
{
  char *name;  /* name of symbol */
  int type;    /* type of symbol: either VAR or FNCT */
  union
  {
    double var;      /* value of a VAR */
    func_t fnctptr;  /* value of a FNCT */
  } value;
  struct symrec *next;  /* link field */
};
     
typedef struct symrec symrec;

class CoinYacc {
private:
  CoinYacc(const CoinYacc& rhs);
  CoinYacc& operator=(const CoinYacc& rhs);

public:
  CoinYacc() : symtable(NULL), symbuf(NULL), length(0), unsetValue(0) {}
  ~CoinYacc()
  {
    if (length) {
      free(symbuf);
      symbuf = NULL;
    }
    symrec* s = symtable;
    while (s) {
      free(s->name);
      symtable = s;
      s = s->next;
      free(symtable);
    }
  }
    
public:
  symrec * symtable;
  char * symbuf;
  int length;
  double unsetValue;
};

class CoinModelHash {
  
public:
  /**@name Constructors, destructor */
  //@{
  /** Default constructor. */
  CoinModelHash();
  /** Destructor */
  ~CoinModelHash();
  //@}
  
  /**@name Copy method */
  //@{
  /** The copy constructor. */
  CoinModelHash(const CoinModelHash&);
  /// =
  CoinModelHash& operator=(const CoinModelHash&);
  //@}

  /**@name sizing (just increases) */
  //@{
  /// Resize hash (also re-hashs)
  void resize(int maxItems,bool forceReHash=false);
  /// Number of items i.e. rows if just row names
  inline int numberItems() const
  { return numberItems_;}
  /// Set number of items
  void setNumberItems(int number);
  /// Maximum number of items
  inline int maximumItems() const
  { return maximumItems_;}
  /// Names
  inline const char *const * names() const
  { return names_;}
  //@}

  /**@name hashing */
  //@{
  /// Returns index or -1
  int hash(const char * name) const;
  /// Adds to hash
  void addHash(int index, const char * name);
  /// Deletes from hash
  void deleteHash(int index);
  /// Returns name at position (or NULL)
  const char * name(int which) const;
  /// Returns non const name at position (or NULL)
  char * getName(int which) const;
  /// Sets name at position (does not create)
  void setName(int which,char * name ) ;
  /// Validates
  void validateHash() const;
private:
  /// Returns a hash value
  int hashValue(const char * name) const;
public:
  //@}
private:
  /**@name Data members */
  //@{
  /// Names
  char ** names_;
  /// hash
  CoinModelHashLink * hash_;
  /// Number of items 
  int numberItems_;
  /// Maximum number of items
  int maximumItems_;
  /// Last slot looked at
  int lastSlot_;
  //@}
};
/// For int,int hashing
class CoinModelHash2 {
  
public:
  /**@name Constructors, destructor */
  //@{
  /** Default constructor. */
  CoinModelHash2();
  /** Destructor */
  ~CoinModelHash2();
  //@}
  
  /**@name Copy method */
  //@{
  /** The copy constructor. */
  CoinModelHash2(const CoinModelHash2&);
  /// =
  CoinModelHash2& operator=(const CoinModelHash2&);
  //@}

  /**@name sizing (just increases) */
  //@{
  /// Resize hash (also re-hashs)
  void resize(int maxItems, const CoinModelTriple * triples,bool forceReHash=false);
  /// Number of items
  inline int numberItems() const
  { return numberItems_;}
  /// Set number of items
  void setNumberItems(int number);
  /// Maximum number of items
  inline int maximumItems() const
  { return maximumItems_;}
  //@}

  /**@name hashing */
  //@{
  /// Returns index or -1
  int hash(int row, int column, const CoinModelTriple * triples) const;
  /// Adds to hash
  void addHash(int index, int row, int column, const CoinModelTriple * triples);
  /// Deletes from hash
  void deleteHash(int index, int row, int column);
private:
  /// Returns a hash value
  int hashValue(int row, int column) const;
public:
  //@}
private:
  /**@name Data members */
  //@{
  /// hash
  CoinModelHashLink * hash_;
  /// Number of items 
  int numberItems_;
  /// Maximum number of items
  int maximumItems_;
  /// Last slot looked at
  int lastSlot_;
  //@}
};
class CoinModelLinkedList {
  
public:
  /**@name Constructors, destructor */
  //@{
  /** Default constructor. */
  CoinModelLinkedList();
  /** Destructor */
  ~CoinModelLinkedList();
  //@}
  
  /**@name Copy method */
  //@{
  /** The copy constructor. */
  CoinModelLinkedList(const CoinModelLinkedList&);
  /// =
  CoinModelLinkedList& operator=(const CoinModelLinkedList&);
  //@}

  /**@name sizing (just increases) */
  //@{
  /** Resize list - for row list maxMajor is maximum rows.
  */
  void resize(int maxMajor,int maxElements);
  /** Create list - for row list maxMajor is maximum rows.
      type 0 row list, 1 column list
  */
  void create(int maxMajor,int maxElements,
              int numberMajor, int numberMinor,
              int type,
              int numberElements, const CoinModelTriple * triples);
  /// Number of major items i.e. rows if just row links
  inline int numberMajor() const
  { return numberMajor_;}
  /// Maximum number of major items i.e. rows if just row links
  inline int maximumMajor() const
  { return maximumMajor_;}
  /// Number of elements
  inline int numberElements() const
  { return numberElements_;}
  /// Maximum number of elements
  inline int maximumElements() const
  { return maximumElements_;}
  /// First on free chain
  inline int firstFree() const
  { return first_[maximumMajor_];}
  /// Last on free chain
  inline int lastFree() const
  { return last_[maximumMajor_];}
  /// First on  chain
  inline int first(int which) const
  { return first_[which];}
  /// Last on  chain
  inline int last(int which) const
  { return last_[which];}
  /// Next array
  inline const int * next() const
  { return next_;}
  /// Previous array
  inline const int * previous() const
  { return previous_;}
  //@}

  /**@name does work */
  //@{
  /** Adds to list - easy case i.e. add row to row list
      Returns where chain starts
  */
  int addEasy(int majorIndex, int numberOfElements, const int * indices,
              const double * elements, CoinModelTriple * triples,
              CoinModelHash2 & hash);
  /** Adds to list - hard case i.e. add row to column list
  */
  void addHard(int minorIndex, int numberOfElements, const int * indices,
               const double * elements, CoinModelTriple * triples,
               CoinModelHash2 & hash);
  /** Adds to list - hard case i.e. add row to column list
      This is when elements have been added to other copy
  */
  void addHard(int first, const CoinModelTriple * triples,
               int firstFree, int lastFree,const int * nextOther);
  /** Deletes from list - same case i.e. delete row from row list
  */
  void deleteSame(int which, CoinModelTriple * triples,
                 CoinModelHash2 & hash, bool zapTriples);
  /** Deletes from list - other case i.e. delete row from column list
      This is when elements have been deleted from other copy
  */
  void updateDeleted(int which, CoinModelTriple * triples,
                     CoinModelLinkedList & otherList);
  /** Deletes one element from Row list
  */
  void deleteRowOne(int position, CoinModelTriple * triples,
                 CoinModelHash2 & hash);
  /** Update column list for one element when
      one element deleted from row copy
  */
  void updateDeletedOne(int position, const CoinModelTriple * triples);
  /// Fills first,last with -1
  void fill(int first,int last);
  /** Puts in free list from other list */
  void synchronize(CoinModelLinkedList & other);
  /// Checks that links are consistent
  void validateLinks(const CoinModelTriple * triples) const;
  //@}
private:
  /**@name Data members */
  //@{
  /// Previous - maximumElements long
  int * previous_;
  /// Next - maximumElements long
  int * next_;
  /// First - maximumMajor+1 long (last free element chain)
  int * first_;
  /// Last - maximumMajor+1 long (last free element chain)
  int * last_;
  /// Number of major items i.e. rows if just row links
  int numberMajor_;
  /// Maximum number of major items i.e. rows if just row links
  int maximumMajor_;
  /// Number of elements
  int numberElements_;
  /// Maximum number of elements
  int maximumElements_;
  /// 0 row list, 1 column list
  int type_;
  //@}
};

#endif
