// Copyright (C) 2007, Lou Hafer and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <sstream>
#include <iomanip>

#include "OsiSolverInterface.hpp"
#include "CoinLpIO.hpp"
#include "CoinMpsIO.hpp"
#include "CoinModel.hpp"

/*
  These routines support three name disciplines:

    0: No names: No name information is retained. rowNames_ and colNames_ are
       always vectors of length 0. Requests for individual names will return
       a name of the form RowNNN or ColNNN, generated on request.

    1: Lazy names: Name information supplied by the client is retained.
       rowNames_ and colNames_ are sized to be large enough to hold names
       supplied by the client, and no larger. If the client has left holes,
       those entries will contain a null string. Requests for individual names
       will return the name supplied by the client, or a generated name.
       Requests for a vector of names will return a reference to rowNames_ or
       colNames_, with no modification.

       This mode is intended for applications like branch-and-cut, where the
       client is only interested in the original constraint system and could
       care less about names for generated constraints and variables. The
       various read[Mps,GMPL,Lp] routines capture the names that are part of
       the input format. If the client is building from scratch, they'll need
       to supply the names as they install constraints and variables.

    2: Full names: From the client's point of view, this looks exactly like
       lazy names, except that when a vector of names is requested, the vector
       is always sized to match the constraint system and all entries have
       names (either supplied or generated). Internally, full names looks just
       like lazy names, with the exception that if the client requests one of
       the name vectors, we generate the full version on the spot and keep it
       around for further use.

       This approach sidesteps some ugly implementation issues. The base
       routines to add a row or column, or load a problem from matrices, are
       pure virtual. There's just no way to guarantee we can keep the name
       vectors up-to-date with each modification. Nor do we want to be in the
       business of generating a name as soon as the row or column appears, only
       to have our generated name immediately overridden by the client.

  Arguably these magic numbers should be an enum, but that's not the current
  OSI style.
*/

namespace {

/*
  Generate a `name' that's really an error message. A separate routine
  strictly for standardisation and ease of use.

  The 'u' code is intended to be used when a routine is expecting one of 'r' or
  'c' and saw something else.
*/
std::string invRowColName (char rcd, int ndx)

{ std::ostringstream buildName ;

  buildName << "!!invalid " ;
  switch (rcd)
  { case 'r':
    { buildName << "Row " << ndx << "!!" ;
      break ; }
    case 'c':
    { buildName << "Col " << ndx << "!!" ;
      break ; }
    case 'd':
    { buildName << "Discipline " << ndx << "!!" ;
      break ; }
    case 'u':
    { buildName << "Row/Col " << ndx << "!!" ;
      break ; }
    default:
    { buildName << "!!Internal Confusion!!" ;
      break ; } }

  return (buildName.str()) ; }

/*
  Adjust the allocated capacity of the name vectors, if they're sufficiently
  far off (more than 1000 elements). Use this routine only if you don't need
  the current contents of the vectors. The `assignment & swap' bit is a trick
  lifted from Stroustrop 16.3.8 to make sure we really give back some space.
*/
void reallocRowColNames (OsiSolverInterface::OsiNameVec &rowNames, int m,
			 OsiSolverInterface::OsiNameVec &colNames, int n)

{ int rowCap = static_cast<int>(rowNames.capacity()) ;
  int colCap = static_cast<int>(colNames.capacity()) ;

  if (rowCap-m > 1000)
  { rowNames.resize(m) ;
    OsiSolverInterface::OsiNameVec tmp = rowNames ;
    rowNames.swap(tmp) ; }
  else
  if (rowCap < m)
  { rowNames.reserve(m) ; }
  assert(rowNames.capacity() >= static_cast<unsigned>(m)) ;

  if (colCap-n > 1000)
  { colNames.resize(n) ;
    OsiSolverInterface::OsiNameVec tmp = colNames ;
    colNames.swap(tmp) ; }
  else
  if (colCap < n)
  { colNames.reserve(n) ; }
  assert(colNames.capacity() >= static_cast<unsigned>(n)) ;

  return ; }


/*
  It's handy to have a 0-length name vector hanging around to use as a return
  value when the name discipline = auto. Then we don't have to worry
  about what's actually occupying rowNames_ or colNames_.
*/

const OsiSolverInterface::OsiNameVec zeroLengthNameVec(0) ;

}

/*
  Generate the default RNNNNNNN/CNNNNNNN. This is a separate routine so that
  it's available to generate names for new rows and columns. If digits is
  nonzero, it's used, otherwise the default is 7 digits after the initial R
  or C.

  If rc = 'o', return the default name for the objective function. Yes, it's
  true that ndx = m is (by definition) supposed to refer to the objective
  function. But ... we're often calling this routine to get a name *before*
  installing the row. Unless you want all your rows to be named OBJECTIVE,
  something different is needed here.
*/

std::string
OsiSolverInterface::dfltRowColName (char rc, int ndx, unsigned digits) const

{ std::ostringstream buildName ;

  if (!(rc == 'r' || rc == 'c' || rc == 'o'))
  { return (invRowColName('u',ndx)) ; }
  if (ndx < 0)
  { return (invRowColName(rc,ndx)) ; }
  
  if (digits <= 0)
  { digits = 7 ; }

  if (rc == 'o')
  { std::string dfltObjName = "OBJECTIVE" ;
    buildName << dfltObjName.substr(0,digits+1) ; }
  else
  { buildName << ((rc == 'r')?"R":"C") ;
    buildName << std::setw(digits) << std::setfill('0') ;
    buildName << ndx ; }

  return buildName.str() ; }

/*
  Return the name of the objective function.
*/
std::string OsiSolverInterface::getObjName (unsigned maxLen) const
{ std::string name ;
  
  if (objName_.length() == 0)
  { name = dfltRowColName('o',0,maxLen) ; }
  else
  { name = objName_.substr(0,maxLen) ; }

  return (name) ; }

/*
  Return a row name, according to the current name discipline, truncated if
  necessary. If the row index is out of range, the name becomes an error
  message. By definition, ndx = getNumRows() (i.e., one greater than the
  largest valid row index) is the objective function.
*/
std::string OsiSolverInterface::getRowName (int ndx, unsigned maxLen) const

{ int nameDiscipline ;
  std::string name ;
/*
  Check for valid row index.
*/
  int m = getNumRows() ;
  if (ndx < 0 || ndx > m)
  { name = invRowColName('r',ndx) ;
    return (name) ; }
/*
  The objective is kept separately, so we don't always have an entry at
  index m in the names vector. If no name is set, return the default.
*/
  if (ndx == m)
  { return (getObjName(maxLen)) ; }
/*
  Determine how we're handling names. It's possible that the underlying solver
  has overridden getIntParam, but doesn't recognise OsiNameDiscipline. In that
  case, we want to default to auto names
*/
  bool recognisesOsiNames = getIntParam(OsiNameDiscipline,nameDiscipline) ;
  if (recognisesOsiNames == false)
  { nameDiscipline = 0 ; }
/*
  Find/generate the proper name, based on discipline.
*/
  switch (nameDiscipline)
  { case 0:
    { name = dfltRowColName('r',ndx) ;
      break ; }
    case 1:
    case 2:
    { name = "" ;
      if (static_cast<unsigned>(ndx) < rowNames_.size())
	name = rowNames_[ndx] ;
      if (name.length() == 0)
	name = dfltRowColName('r',ndx) ;
      break ; }
    default:
    { name = invRowColName('d',nameDiscipline) ;
      return (name) ; } }
/*
  Return the (possibly truncated) substring. The default for maxLen is npos
  (no truncation).
*/
  return (name.substr(0,maxLen)) ; }


/*
  Return the vector of row names. The vector we need depends on the name
  discipline:
    0: return a vector of length 0
    1: return rowNames_, no tweaking required
    2: Check that rowNames_ is complete. Generate a complete vector on
       the spot if we need it.
*/
const OsiSolverInterface::OsiNameVec &OsiSolverInterface::getRowNames ()

{ int nameDiscipline ;
/*
  Determine how we're handling names. It's possible that the underlying solver
  has overridden getIntParam, but doesn't recognise OsiNameDiscipline. In that
  case, we want to default to auto names
*/
  bool recognisesOsiNames = getIntParam(OsiNameDiscipline,nameDiscipline) ;
  if (recognisesOsiNames == false)
  { nameDiscipline = 0 ; }
/*
  Return the proper vector, as described at the head of the routine. If we
  need to generate a full vector, resize the existing vector and scan, filling
  in entries as required.
*/
  switch (nameDiscipline)
  { case 0:
    { return (zeroLengthNameVec) ; }
    case 1:
    { return (rowNames_) ; }
    case 2:
    { int m = getNumRows() ;
      if (rowNames_.size() < static_cast<unsigned>(m+1))
      { rowNames_.resize(m+1) ; }
      for (int i = 0 ; i < m ; i++)
      { if (rowNames_[i].length() == 0)
	{ rowNames_[i] = dfltRowColName('r',i) ; } }
      if (rowNames_[m].length() == 0)
      { rowNames_[m] = getObjName() ; }
      return (rowNames_) ; }
    default:
    { /* quietly fail */
      return (zeroLengthNameVec) ; } }
/*
  We should never reach here.
*/
  assert(false) ;

  return (zeroLengthNameVec) ; }
  

/*
  Return a column name, according to the current name discipline, truncated if
  necessary. If the column index is out of range, the name becomes an error
  message.
*/
std::string OsiSolverInterface::getColName (int ndx, unsigned maxLen) const

{ int nameDiscipline ;
  std::string name ;
/*
  Check for valid column index.
*/
  if (ndx < 0 || ndx >= getNumCols())
  { name = invRowColName('c',ndx) ;
    return (name) ; }
/*
  Determine how we're handling names. It's possible that the underlying solver
  has overridden getIntParam, but doesn't recognise OsiNameDiscipline. In that
  case, we want to default to auto names
*/
  bool recognisesOsiNames = getIntParam(OsiNameDiscipline,nameDiscipline) ;
  if (recognisesOsiNames == false)
  { nameDiscipline = 0 ; }
/*
  Find/generate the proper name, based on discipline.
*/
  switch (nameDiscipline)
  { case 0:
    { name = dfltRowColName('c',ndx) ;
      break ; }
    case 1:
    case 2:
    { name = "" ;
      if (static_cast<unsigned>(ndx) < colNames_.size())
	name = colNames_[ndx] ;
      if (name.length() == 0)
	name = dfltRowColName('c',ndx) ;
      break ; }
    default:
    { name = invRowColName('d',nameDiscipline) ;
      return (name) ; } }
/*
  Return the (possibly truncated) substring. The default for maxLen is npos
  (no truncation).
*/
  return (name.substr(0,maxLen)) ; }


/*
  Return the vector of column names. The vector we need depends on the name
  discipline:
    0: return a vector of length 0
    1: return colNames_, no tweaking required
    2: Check that colNames_ is complete. Generate a complete vector on
       the spot if we need it.
*/
const OsiSolverInterface::OsiNameVec &OsiSolverInterface::getColNames ()

{ int nameDiscipline ;
/*
  Determine how we're handling names. It's possible that the underlying solver
  has overridden getIntParam, but doesn't recognise OsiNameDiscipline. In that
  case, we want to default to auto names
*/
  bool recognisesOsiNames = getIntParam(OsiNameDiscipline,nameDiscipline) ;
  if (recognisesOsiNames == false)
  { nameDiscipline = 0 ; }
/*
  Return the proper vector, as described at the head of the routine. If we
  need to generate a full vector, resize the existing vector and scan, filling
  in entries as required.
*/
  switch (nameDiscipline)
  { case 0:
    { return (zeroLengthNameVec) ; }
    case 1:
    { return (colNames_) ; }
    case 2:
    { int n = getNumCols() ;
      if (colNames_.size() < static_cast<unsigned>(n))
      { colNames_.resize(n) ; }
      for (int j = 0 ; j < n ; j++)
      { if (colNames_[j].length() == 0)
	{ colNames_[j] = dfltRowColName('c',j) ; } }
      return (colNames_) ; }
    default:
    { /* quietly fail */
      return (zeroLengthNameVec) ; } }
/*
  We should never reach here.
*/
  assert(false) ;

  return (zeroLengthNameVec) ; }
  

/*
  Set a single row name. Quietly does nothing if the index or name discipline
  is invalid.
*/
void OsiSolverInterface::setRowName (int ndx, std::string name)

{ int nameDiscipline ;
/*
  Quietly do nothing if the index is out of bounds. This should be changed,
  but what's our error convention, eh? There's no precedent in
  OsiSolverInterface.cpp.
*/
  if (ndx < 0 || ndx >= getNumRows())
  { return ; }
/*
  Determine how we're handling names. It's possible that the underlying solver
  has overridden getIntParam, but doesn't recognise OsiNameDiscipline. In that
  case, we want to default to auto names
*/
  bool recognisesOsiNames = getIntParam(OsiNameDiscipline,nameDiscipline) ;
  if (recognisesOsiNames == false)
  { nameDiscipline = 0 ; }
/*
  Do the right thing, according to the discipline.
*/
  switch (nameDiscipline)
  { case 0:
    { break ; }
    case 1:
    case 2:
    { if (static_cast<unsigned>(ndx) > rowNames_.capacity())
      { rowNames_.resize(ndx+1) ; }
      else
      if (static_cast<unsigned>(ndx) >= rowNames_.size())
      { rowNames_.resize(ndx+1) ; }
      rowNames_[ndx] = name ;
      break ; }
    default:
    { break ; } }

  return ; }

/*
  Set a run of row names. Quietly fail if the specified indices are invalid.

  On the target side, [tgtStart, tgtStart+len-1] must be in the range
  [0,m-1], where m is the number of rows.  On the source side, srcStart must
  be zero or greater. If we run off the end of srcNames, we just generate
  default names.
*/
void OsiSolverInterface::setRowNames (OsiNameVec &srcNames,
				      int srcStart, int len, int tgtStart)

{ int nameDiscipline ;
/*
  Determine how we're handling names. It's possible that the underlying solver
  has overridden getIntParam, but doesn't recognise OsiNameDiscipline. In that
  case, we want to default to auto names
*/
  bool recognisesOsiNames = getIntParam(OsiNameDiscipline,nameDiscipline) ;
  if (recognisesOsiNames == false)
  { nameDiscipline = 0 ; }
/*
  If the name discipline is auto, we're already done.
*/
  if (nameDiscipline == 0)
  { return ; }
/*
  A little self-protection. Check that we're within [0,m-1] on the target side,
  and that srcStart is zero or greater. Quietly fail if the indices don't fit.
*/
  int m = getNumRows() ;
  if (tgtStart < 0 || tgtStart+len > m)
  { return ; }
  if (srcStart < 0)
  { return ; }
  int srcLen = static_cast<int>(srcNames.size()) ;
/*
  Load 'em up.
*/
  int srcNdx = srcStart ;
  int tgtNdx = tgtStart ;
  for ( ; tgtNdx < tgtStart+len ; srcNdx++,tgtNdx++)
  { if (srcNdx < srcLen)
    { setRowName(tgtNdx,srcNames[srcNdx]) ; }
    else
    { setRowName(tgtNdx,dfltRowColName('r',tgtNdx)) ; } }

  return ; }

/*
  Delete one or more row names.
*/
void OsiSolverInterface::deleteRowNames (int tgtStart, int len)

{ int nameDiscipline ;
/*
  Determine how we're handling names. It's possible that the underlying solver
  has overridden getIntParam, but doesn't recognise OsiNameDiscipline. In that
  case, we want to default to auto names
*/
  bool recognisesOsiNames = getIntParam(OsiNameDiscipline,nameDiscipline) ;
  if (recognisesOsiNames == false)
  { nameDiscipline = 0 ; }
/*
  If the name discipline is auto, we're done.
*/
  if (nameDiscipline == 0)
  { return ; }
/*
  Trim the range to names that exist in the name vector. If we're doing lazy
  names, it's quite likely that we don't need to do any work.
*/
  int lastNdx = static_cast<int>(rowNames_.size()) ;
  if (tgtStart < 0 || tgtStart >= lastNdx)
  { return ; }
  if (tgtStart+len > lastNdx)
  { len = lastNdx-tgtStart ; }
/*
  Erase the names.
*/
  OsiNameVec::iterator firstIter,lastIter ;
  firstIter = rowNames_.begin()+tgtStart ;
  lastIter = firstIter+len ;
  rowNames_.erase(firstIter,lastIter) ;

  return ; }
  

/*
  Set a single column name. Quietly does nothing if the index or name
  discipline is invalid.
*/
void OsiSolverInterface::setColName (int ndx, std::string name)

{ int nameDiscipline ;
/*
  Quietly do nothing if the index is out of bounds. This should be changed,
  but what's our error convention, eh? There's no precedent in
  OsiSolverInterface.cpp.
*/
  if (ndx < 0 || ndx >= getNumCols())
  { return ; }
/*
  Determine how we're handling names. It's possible that the underlying solver
  has overridden getIntParam, but doesn't recognise OsiNameDiscipline. In that
  case, we want to default to auto names
*/
  bool recognisesOsiNames = getIntParam(OsiNameDiscipline,nameDiscipline) ;
  if (recognisesOsiNames == false)
  { nameDiscipline = 0 ; }
/*
  Do the right thing, according to the discipline.
*/
  switch (nameDiscipline)
  { case 0:
    { break ; }
    case 1:
    case 2:
    { if (static_cast<unsigned>(ndx) > colNames_.capacity())
      { colNames_.resize(ndx+1) ; }
      else
      if (static_cast<unsigned>(ndx) >= colNames_.size())
      { colNames_.resize(ndx+1) ; }
      colNames_[ndx] = name ;
      break ; }
    default:
    { break ; } }

  return ; }

/*
  Set a run of column names. Quietly fail if the specified indices are
  invalid.

  On the target side, [tgtStart, tgtStart+len-1] must be in the range
  [0,n-1], where n is the number of columns.  On the source side, srcStart
  must be zero or greater. If we run off the end of srcNames, we just
  generate default names.
*/
void OsiSolverInterface::setColNames (OsiNameVec &srcNames,
				      int srcStart, int len, int tgtStart)

{ int nameDiscipline ;
/*
  Determine how we're handling names. It's possible that the underlying solver
  has overridden getIntParam, but doesn't recognise OsiNameDiscipline. In that
  case, we want to default to auto names
*/
  bool recognisesOsiNames = getIntParam(OsiNameDiscipline,nameDiscipline) ;
  if (recognisesOsiNames == false)
  { nameDiscipline = 0 ; }
/*
  If the name discipline is auto, we're already done.
*/
  if (nameDiscipline == 0)
  { return ; }
/*
  A little self-protection. Check that we're within [0,m-1] on the target side,
  and that srcStart is zero or greater. Quietly fail if the indices don't fit.
*/
  int n = getNumCols() ;
  if (tgtStart < 0 || tgtStart+len > n)
  { return ; }
  if (srcStart < 0)
  { return ; }
  int srcLen = static_cast<int>(srcNames.size()) ;
/*
  Load 'em up.
*/
  int srcNdx = srcStart ;
  int tgtNdx = tgtStart ;
  for ( ; tgtNdx < tgtStart+len ; srcNdx++,tgtNdx++)
  { if (srcNdx < srcLen)
    { setColName(tgtNdx,srcNames[srcNdx]) ; }
    else
    { setColName(tgtNdx,dfltRowColName('c',tgtNdx)) ; } }

  return ; }

/*
  Delete one or more column names. Quietly fail if firstNdx is less than zero
  or if firstNdx+len is greater than the number of columns.
*/
void OsiSolverInterface::deleteColNames (int tgtStart, int len)

{ int nameDiscipline ;
/*
  Determine how we're handling names. It's possible that the underlying solver
  has overridden getIntParam, but doesn't recognise OsiNameDiscipline. In that
  case, we want to default to auto names
*/
  bool recognisesOsiNames = getIntParam(OsiNameDiscipline,nameDiscipline) ;
  if (recognisesOsiNames == false)
  { nameDiscipline = 0 ; }
/*
  If the name discipline is auto, we're done.
*/
  if (nameDiscipline == 0)
  { return ; }
/*
  Trim the range to names that exist in the name vector. If we're doing lazy
  names, it's quite likely that we don't need to do any work.
*/
  int lastNdx = static_cast<int>(colNames_.size()) ;
  if (tgtStart < 0 || tgtStart >= lastNdx)
  { return ; }
  if (tgtStart+len > lastNdx)
  { len = lastNdx-tgtStart ; }
/*
  Erase the names.
*/
  OsiNameVec::iterator firstIter,lastIter ;
  firstIter = colNames_.begin()+tgtStart ;
  lastIter = firstIter+len ;
  colNames_.erase(firstIter,lastIter) ;

  return ; }

/*
  Install the name information from a CoinMpsIO object.
*/
void OsiSolverInterface::setRowColNames (const CoinMpsIO &mps)

{ int nameDiscipline,m,n ;
/*
  Determine how we're handling names. It's possible that the underlying solver
  has overridden getIntParam, but doesn't recognise OsiNameDiscipline. In that
  case, we want to default to auto names
*/
  bool recognisesOsiNames = getIntParam(OsiNameDiscipline,nameDiscipline) ;
  if (recognisesOsiNames == false)
  { nameDiscipline = 0 ; }
/*
  Whatever happens, we're about to clean out the current name vectors. Decide
  on an appropriate size and call reallocRowColNames to adjust capacity.
*/
  if (nameDiscipline == 0)
  { m = 0 ;
    n = 0 ; }
  else
  { m = mps.getNumRows() ;
    n = mps.getNumCols() ; }
  reallocRowColNames(rowNames_,m,colNames_,n) ;
/*
  If name discipline is auto, we're done already. Otherwise, load 'em
  up. If I understand MPS correctly, names are required.
*/
  if (nameDiscipline != 0)
  { rowNames_.resize(m) ;
    for (int i = 0 ; i < m ; i++)
    { rowNames_[i] = mps.rowName(i) ; }
    objName_ = mps.getObjectiveName() ;
    colNames_.resize(n) ;
    for (int j = 0 ; j < n ; j++)
    { colNames_[j] = mps.columnName(j) ; } }

  return ; }


/*
  Install the name information from a CoinModel object. CoinModel does not
  maintain a name for the objective function (in fact, it has no concept of
  objective function).
*/
void OsiSolverInterface::setRowColNames (CoinModel &mod)

{ int nameDiscipline,m,n ;
/*
  Determine how we're handling names. It's possible that the underlying solver
  has overridden getIntParam, but doesn't recognise OsiNameDiscipline. In that
  case, we want to default to auto names
*/
  bool recognisesOsiNames = getIntParam(OsiNameDiscipline,nameDiscipline) ;
  if (recognisesOsiNames == false)
  { nameDiscipline = 0 ; }
/*
  Whatever happens, we're about to clean out the current name vectors. Decide
  on an appropriate size and call reallocRowColNames to adjust capacity.
*/
  if (nameDiscipline == 0)
  { m = 0 ;
    n = 0 ; }
  else
  { m = mod.rowNames()->numberItems() ;
    n = mod.columnNames()->numberItems() ; }
  reallocRowColNames(rowNames_,m,colNames_,n) ;
/*
  If name discipline is auto, we're done already. Otherwise, load 'em
  up. As best I can see, there's no guarantee that we'll have names for all
  rows and columns, so we need to pay attention.
*/
  if (nameDiscipline != 0)
  { int maxRowNdx=-1, maxColNdx=-1 ;
    const char *const *names = mod.rowNames()->names() ;
    rowNames_.resize(m) ;
    for (int i = 0 ; i < m ; i++)
    { std::string nme = names[i] ;
      if (nme.length() == 0)
      { if (nameDiscipline == 2)
	{ nme = dfltRowColName('r',i) ; } }
      if (nme.length() > 0)
      { maxRowNdx = i ; }
      rowNames_[i] = nme ; }
    rowNames_.resize(maxRowNdx+1) ;
    names = mod.columnNames()->names() ;
    colNames_.resize(n) ;
    for (int j = 0 ; j < n ; j++)
    { std::string nme = names[j] ;
      if (nme.length() == 0)
      { if (nameDiscipline == 2)
	{ nme = dfltRowColName('c',j) ; } }
      if (nme.length() > 0)
      { maxColNdx = j ; }
      colNames_[j] = nme ; }
    colNames_.resize(maxColNdx+1) ; }
/*
  And we're done.
*/
  return ; }



/*
  Install the name information from a CoinLpIO object. Nearly identical to the
  previous routine, but we start from a different object.
*/
void OsiSolverInterface::setRowColNames (CoinLpIO &mod)

{ int nameDiscipline,m,n ;
/*
  Determine how we're handling names. It's possible that the underlying solver
  has overridden getIntParam, but doesn't recognise OsiNameDiscipline. In that
  case, we want to default to auto names
*/
  bool recognisesOsiNames = getIntParam(OsiNameDiscipline,nameDiscipline) ;
  if (recognisesOsiNames == false)
  { nameDiscipline = 0 ; }
/*
  Whatever happens, we're about to clean out the current name vectors. Decide
  on an appropriate size and call reallocRowColNames to adjust capacity.
*/
  if (nameDiscipline == 0)
  { m = 0 ;
    n = 0 ; }
  else
  { m = mod.getNumRows() ;
    n = mod.getNumCols() ; }
  reallocRowColNames(rowNames_,m,colNames_,n) ;
/*
  If name discipline is auto, we're done already. Otherwise, load 'em
  up. I have no idea whether we can guarantee valid names for all rows and
  columns, so we need to pay attention.
*/
  if (nameDiscipline != 0)
  { int maxRowNdx=-1, maxColNdx=-1 ;
    const char *const *names = mod.getRowNames() ;
    rowNames_.resize(m) ;
    for (int i = 0 ; i < m ; i++)
    { std::string nme = names[i] ;
      if (nme.length() == 0)
      { if (nameDiscipline == 2)
	{ nme = dfltRowColName('r',i) ; } }
      if (nme.length() > 0)
      { maxRowNdx = i ; }
      rowNames_[i] = nme ; }
    rowNames_.resize(maxRowNdx+1) ;
    objName_ = mod.getObjName() ;
    names = mod.getColNames() ;
    colNames_.resize(n) ;
    for (int j = 0 ; j < n ; j++)
    { std::string nme = names[j] ;
      if (nme.length() == 0)
      { if (nameDiscipline == 2)
	{ nme = dfltRowColName('c',j) ; } }
      if (nme.length() > 0)
      { maxColNdx = j ; }
      colNames_[j] = nme ; }
    colNames_.resize(maxColNdx+1) ; }
/*
  And we're done.
*/
  return ; }

