/* $Id: CoinParam.cpp 1424 2011-05-02 08:02:28Z stefan $ */
// Copyright (C) 2006, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <string>
#include <cassert>
#include <iostream>

#include "CoinPragma.hpp"
#include "CoinParam.hpp"

/*
  Constructors and destructors

  There's a generic constructor and one for integer, double, keyword, string,
  and action parameters.
*/

/*
  Default constructor.
*/
CoinParam::CoinParam () 
  : type_(coinParamInvalid),
    name_(),
    lengthName_(0),
    lengthMatch_(0),
    lowerDblValue_(0.0),
    upperDblValue_(0.0),
    dblValue_(0.0),
    lowerIntValue_(0),
    upperIntValue_(0),
    intValue_(0),
    strValue_(),
    definedKwds_(),
    currentKwd_(-1),
    pushFunc_(0),
    pullFunc_(0),
    shortHelp_(),
    longHelp_(),
    display_(false)
{
  /* Nothing to be done here */
}


/*
  Constructor for double parameter
*/
CoinParam::CoinParam (std::string name, std::string help,
		      double lower, double upper, double dflt, bool display)
  : type_(coinParamDbl),
    name_(name),
    lengthName_(0),
    lengthMatch_(0),
    lowerDblValue_(lower),
    upperDblValue_(upper),
    dblValue_(dflt),
    lowerIntValue_(0),
    upperIntValue_(0),
    intValue_(0),
    strValue_(),
    definedKwds_(),
    currentKwd_(-1),
    pushFunc_(0),
    pullFunc_(0),
    shortHelp_(help),
    longHelp_(),
    display_(display)
{
  processName() ;
}

/*
  Constructor for integer parameter
*/
CoinParam::CoinParam (std::string name, std::string help,
		      int lower, int upper, int dflt, bool display)
  : type_(coinParamInt),
    name_(name),
    lengthName_(0),
    lengthMatch_(0),
    lowerDblValue_(0.0),
    upperDblValue_(0.0),
    dblValue_(0.0),
    lowerIntValue_(lower),
    upperIntValue_(upper),
    intValue_(dflt),
    strValue_(),
    definedKwds_(),
    currentKwd_(-1),
    pushFunc_(0),
    pullFunc_(0),
    shortHelp_(help),
    longHelp_(),
    display_(display)
{
  processName() ;
}

/*
  Constructor for keyword parameter.
*/
CoinParam::CoinParam (std::string name, std::string help,
		      std::string firstValue, int dflt, bool display)
  : type_(coinParamKwd),
    name_(name),
    lengthName_(0),
    lengthMatch_(0),
    lowerDblValue_(0.0),
    upperDblValue_(0.0),
    dblValue_(0.0),
    lowerIntValue_(0),
    upperIntValue_(0),
    intValue_(0),
    strValue_(),
    definedKwds_(),
    currentKwd_(dflt),
    pushFunc_(0),
    pullFunc_(0),
    shortHelp_(help),
    longHelp_(),
    display_(display)
{
  processName() ;
  definedKwds_.push_back(firstValue) ;
}

/*
  Constructor for string parameter.
*/
CoinParam::CoinParam (std::string name, std::string help,
	 	      std::string dflt, bool display)
  : type_(coinParamStr),
    name_(name),
    lengthName_(0),
    lengthMatch_(0),
    lowerDblValue_(0.0),
    upperDblValue_(0.0),
    dblValue_(0.0),
    lowerIntValue_(0),
    upperIntValue_(0),
    intValue_(0),
    strValue_(dflt),
    definedKwds_(),
    currentKwd_(0),
    pushFunc_(0),
    pullFunc_(0),
    shortHelp_(help),
    longHelp_(),
    display_(display)
{
  processName() ;
}

/*
  Constructor for action parameter.
*/
CoinParam::CoinParam (std::string name, std::string help, bool display)
  : type_(coinParamAct),
    name_(name),
    lengthName_(0),
    lengthMatch_(0),
    lowerDblValue_(0.0),
    upperDblValue_(0.0),
    dblValue_(0.0),
    lowerIntValue_(0),
    upperIntValue_(0),
    intValue_(0),
    strValue_(),
    definedKwds_(),
    currentKwd_(0),
    pushFunc_(0),
    pullFunc_(0),
    shortHelp_(help),
    longHelp_(),
    display_(display)
{
  processName() ;
}

/*
  Copy constructor.
*/
CoinParam::CoinParam (const CoinParam &orig)
  : type_(orig.type_),
    lengthName_(orig.lengthName_),
    lengthMatch_(orig.lengthMatch_),
    lowerDblValue_(orig.lowerDblValue_),
    upperDblValue_(orig.upperDblValue_),
    dblValue_(orig.dblValue_),
    lowerIntValue_(orig.lowerIntValue_),
    upperIntValue_(orig.upperIntValue_),
    intValue_(orig.intValue_),
    currentKwd_(orig.currentKwd_),
    pushFunc_(orig.pushFunc_),
    pullFunc_(orig.pullFunc_),
    display_(orig.display_)
{
  name_ = orig.name_ ;
  strValue_ = orig.strValue_ ;
  definedKwds_ = orig.definedKwds_ ;
  shortHelp_ = orig.shortHelp_ ;
  longHelp_ = orig.longHelp_ ;
}

/*
  Clone
*/

CoinParam *CoinParam::clone ()
{
  return (new CoinParam(*this)) ;
}

CoinParam &CoinParam::operator= (const CoinParam &rhs)
{
  if (this != &rhs)
  { type_ = rhs.type_ ;
    name_ = rhs.name_ ;
    lengthName_ = rhs.lengthName_ ;
    lengthMatch_ = rhs.lengthMatch_ ;
    lowerDblValue_ = rhs.lowerDblValue_ ;
    upperDblValue_ = rhs.upperDblValue_ ;
    dblValue_ = rhs.dblValue_ ;
    lowerIntValue_ = rhs.lowerIntValue_ ;
    upperIntValue_ = rhs.upperIntValue_ ;
    intValue_ = rhs.intValue_ ;
    strValue_ = rhs.strValue_ ;
    definedKwds_ = rhs.definedKwds_ ;
    currentKwd_ = rhs.currentKwd_ ;
    pushFunc_ = rhs.pushFunc_ ;
    pullFunc_ = rhs.pullFunc_ ;
    shortHelp_ = rhs.shortHelp_ ;
    longHelp_ = rhs.longHelp_ ;
    display_ = rhs.display_ ; }

  return *this ; }

/*
  Destructor
*/
CoinParam::~CoinParam ()
{ /* Nothing more to do */ }


/*
  Methods to manipulate a CoinParam object.
*/

/*
  Process the parameter name.
  
  Process the name for efficient matching: determine if an `!' is present. If
  so, locate and record the position and remove the `!'.
*/

void CoinParam::processName()

{ std::string::size_type shriekPos = name_.find('!') ;
  lengthName_ = name_.length() ;
  if (shriekPos == std::string::npos)
  { lengthMatch_ = lengthName_ ; }
  else
  { lengthMatch_ = shriekPos ;
    name_ = name_.substr(0,shriekPos)+name_.substr(shriekPos+1) ;
    lengthName_-- ; }

  return ; }

/*
  Check an input string to see if it matches the parameter name. The whole
  input string must match, and the length of the match must exceed the
  minimum match length. A match is impossible if the string is longer than
  the name.

  Returns: 0 for no match, 1 for a successful match, 2 if the match is short
*/
int CoinParam::matches (std::string input) const
{
  size_t inputLen = input.length() ;
  if (inputLen <= lengthName_)
  { size_t i ;
    for (i = 0 ; i < inputLen ; i++)
    { if (tolower(name_[i]) != tolower(input[i])) 
	break ; }
    if (i < inputLen)
    { return (0) ; }
    else
    if (i >= lengthMatch_)
    { return (1) ; }
    else
    { return (2) ; } }
  
  return (0) ;
}


/*
  Return the parameter name, formatted to indicate how it'll be matched.
  E.g., some!Name will come back as some(Name).
*/
std::string CoinParam::matchName () const
{ 
  if (lengthMatch_ == lengthName_) 
  { return name_ ; }
  else
  { return name_.substr(0,lengthMatch_)+"("+name_.substr(lengthMatch_)+")" ; }
}


/*
  Print the long help message and a message about appropriate values.
*/
void CoinParam::printLongHelp() const
{
  if (longHelp_ != "")
  { CoinParamUtils::printIt(longHelp_.c_str()) ; }
  else
  if (shortHelp_ != "")
  { CoinParamUtils::printIt(shortHelp_.c_str()) ; }
  else
  { CoinParamUtils::printIt("No help provided.") ; }

  switch (type_)
  { case coinParamDbl:
    { std::cout << "<Range of values is " << lowerDblValue_ << " to "
		<< upperDblValue_ << ";\n\tcurrent " << dblValue_ << ">"
		<< std::endl ;
      assert (upperDblValue_>lowerDblValue_) ;
      break ; }
    case coinParamInt:
    { std::cout << "<Range of values is " << lowerIntValue_ << " to "
		<< upperIntValue_ << ";\n\tcurrent " << intValue_ << ">"
		<< std::endl ;
      assert (upperIntValue_>lowerIntValue_) ;
      break ; }
    case coinParamKwd:
    { printKwds() ;
      break ; }
    case coinParamStr:
    { std::cout << "<Current value is " ;
      if (strValue_ == "")
      { std::cout << "(unset)>" ; }
      else
      { std::cout << "`" << strValue_ << "'>" ; }
      std::cout << std::endl ;
      break ; }
    case coinParamAct:
    { break ; }
    default:
    { std::cout << "!! invalid parameter type !!" << std::endl ;
      assert (false) ; } }
}


/*
  Methods to manipulate the value of a parameter.
*/

/*
  Methods to manipulate the values associated with a keyword parameter.
*/

/*
  Add a keyword to the list for a keyword parameter.
*/
void CoinParam::appendKwd (std::string kwd)
{ 
  assert (type_ == coinParamKwd) ;

  definedKwds_.push_back(kwd) ;
}

/*
  Scan the keywords of a keyword parameter and return the integer index of
  the keyword matching the input, or -1 for no match.
*/
int CoinParam::kwdIndex (std::string input) const
{
  assert (type_ == coinParamKwd) ;

  int whichItem = -1 ;
  size_t numberItems = definedKwds_.size() ;
  if (numberItems > 0)
  { size_t inputLen = input.length() ;
    size_t it ;
/*
  Open a loop to check each keyword against the input string. We don't record
  the match length for keywords, so we need to check each one for an `!' and
  do the necessary preprocessing (record position and elide `!') before
  checking for a match of the required length.
*/
    for (it = 0 ; it < numberItems ; it++)
    { std::string kwd = definedKwds_[it] ;
      std::string::size_type shriekPos = kwd.find('!') ;
      size_t kwdLen = kwd.length() ;
      size_t matchLen = kwdLen ;
      if (shriekPos != std::string::npos)
      { matchLen = shriekPos ;
	kwd = kwd.substr(0,shriekPos)+kwd.substr(shriekPos+1) ;
	kwdLen = kwd.length() ; }
/*
  Match is possible only if input is shorter than the keyword. The entire input
  must match and the match must exceed the minimum length.
*/
      if (inputLen <= kwdLen)
      { unsigned int i ;
	for (i = 0 ; i < inputLen ; i++)
	{ if (tolower(kwd[i]) != tolower(input[i])) 
	    break ; }
	if (i >= inputLen && i >= matchLen)
	{ whichItem = static_cast<int>(it) ;
	  break ; } } } }

  return (whichItem) ;
}

/*
  Set current value for a keyword parameter using a string.
*/
void CoinParam::setKwdVal (const std::string value)
{
  assert (type_ == coinParamKwd) ;

  int action = kwdIndex(value) ;
  if (action >= 0)
  { currentKwd_ = action ; }
}

/*
  Set current value for keyword parameter using an integer. Echo the new value
  to cout if requested.
*/
void CoinParam::setKwdVal (int value, bool printIt)
{
  assert (type_ == coinParamKwd) ;
  assert (value >= 0 && unsigned(value) < definedKwds_.size()) ;

  if (printIt && value != currentKwd_)
  { std::cout << "Option for " << name_ << " changed from "
              << definedKwds_[currentKwd_] << " to "
              << definedKwds_[value] << std::endl ; }

  currentKwd_ = value ;
}

/*
  Return the string corresponding to the current value.
*/
std::string CoinParam::kwdVal() const
{
  assert (type_ == coinParamKwd) ;
  
  return (definedKwds_[currentKwd_]) ;
}

/*
  Print the keywords for a keyword parameter, formatted to indicate how they'll
  be matched. (E.g., some!Name prints as some(Name).). Follow with current
  value.
*/
void CoinParam::printKwds () const
{
  assert (type_ == coinParamKwd) ;

  std::cout << "Possible options for " << name_ << " are:" ;
  unsigned int it ;
  int maxAcross = 5 ;
  for (it = 0 ; it < definedKwds_.size() ; it++)
  { std::string kwd = definedKwds_[it] ;
    std::string::size_type shriekPos = kwd.find('!') ;
    if (shriekPos != std::string::npos)
    { kwd = kwd.substr(0,shriekPos)+"("+kwd.substr(shriekPos+1)+")" ; }
    if (it%maxAcross == 0)
    { std::cout << std::endl ; }
    std::cout << "  " << kwd ; }
  std::cout << std::endl ;

  assert (currentKwd_ >= 0 && unsigned(currentKwd_) < definedKwds_.size()) ;

  std::string current = definedKwds_[currentKwd_] ;
  std::string::size_type  shriekPos = current.find('!') ;
  if (shriekPos != std::string::npos)
  { current = current.substr(0,shriekPos)+
			"("+current.substr(shriekPos+1)+")" ; }
  std::cout << "  <current: " << current << ">" << std::endl ;
}


/*
  Methods to manipulate the value of a string parameter.
*/

void CoinParam::setStrVal (std::string value)
{ 
  assert (type_ == coinParamStr) ;

  strValue_ = value ;
}

std::string CoinParam::strVal () const
{
  assert (type_ == coinParamStr) ;

  return (strValue_) ;
}


/*
  Methods to manipulate the value of a double parameter.
*/

void CoinParam::setDblVal (double value)
{ 
  assert (type_ == coinParamDbl) ;

  dblValue_ = value ;
}

double CoinParam::dblVal () const
{
  assert (type_ == coinParamDbl) ;

  return (dblValue_) ;
}


/*
  Methods to manipulate the value of an integer parameter.
*/

void CoinParam::setIntVal (int value)
{ 
  assert (type_ == coinParamInt) ;

  intValue_ = value ;
}

int CoinParam::intVal () const
{
  assert (type_ == coinParamInt) ;

  return (intValue_) ;
}

/*
  A print function (friend of the class)
*/

std::ostream &operator<< (std::ostream &s, const CoinParam &param)
{
  switch (param.type())
  { case CoinParam::coinParamDbl:
    { return (s << param.dblVal()) ; }
    case CoinParam::coinParamInt:
    { return (s << param.intVal()) ; }
    case CoinParam::coinParamKwd:
    { return (s << param.kwdVal()) ; }
    case CoinParam::coinParamStr:
    { return (s << param.strVal()) ; }
    case CoinParam::coinParamAct:
    { return (s << "<evokes action>") ; }
    default:
    { return (s << "!! invalid parameter type !!") ; } }
}
