/* $Id: CoinParam.hpp 1493 2011-11-01 16:56:07Z tkr $ */
#ifndef CoinParam_H
#define CoinParam_H

/*
  Copyright (C) 2002, International Business Machines
  Corporation and others.  All Rights Reserved.

  This code is licensed under the terms of the Eclipse Public License (EPL).
*/

/*! \file CoinParam.hpp
    \brief Declaration of a class for command line parameters.
*/

#include <vector>
#include <string>
#include <cstdio>

/*! \class CoinParam
    \brief A base class for `keyword value' command line parameters.

  The underlying paradigm is that a parameter specifies an action to be
  performed on a target object. The base class provides two function
  pointers, a `push' function and a `pull' function.  By convention, a push
  function will set some value in the target object or perform some action
  using the target object.  A `pull' function will retrieve some value from
  the target object.  This is only a convention, however; CoinParam and
  associated utilities make no use of these functions and have no hardcoded
  notion of how they should be used.

  The action to be performed, and the target object, will be specific to a
  particular application. It is expected that users will derive
  application-specific parameter classes from this base class. A derived
  class will typically add fields and methods to set/get a code for the
  action to be performed (often, an enum class) and the target object (often,
  a pointer or reference).

  Facilities provided by the base class and associated utility routines
  include:
  <ul>
    <li> Support for common parameter types with numeric, string, or
	 keyword values.
    <li> Support for short and long help messages.
    <li> Pointers to `push' and `pull' functions as described above.
    <li> Command line parsing and keyword matching.
  </ul>
  All utility routines are declared in the #CoinParamUtils namespace.

  The base class recognises five types of parameters: actions (which require
  no value); numeric parameters with integer or real (double) values; keyword
  parameters, where the value is one of a defined set of value-keywords;
  and string parameters (where the value is a string).
  The base class supports the definition of a valid range, a default value,
  and short and long help messages for a parameter.

  As defined by the #CoinParamFunc typedef, push and pull functions
  should take a single parameter, a pointer to a CoinParam. Typically this
  object will actually be a derived class as described above, and the
  implementation function will have access to all capabilities of CoinParam and
  of the derived class.

  When specified as command line parameters, the expected syntax is `-keyword
  value' or `-keyword=value'. You can also use the Gnu double-dash style,
  `--keyword'. Spaces around the `=' will \e not work.

  The keyword (name) for a parameter can be defined with an `!' to mark the
  minimal match point. For example, allow!ableGap will be considered matched
  by the strings `allow', `allowa', `allowab', \e etc. Similarly, the
  value-keyword strings for keyword parameters can be defined with `!' to
  mark the minimal match point.  Matching of keywords and value-keywords is
  \e not case sensitive.
*/

class CoinParam
{
 
public:

/*! \name Subtypes */
//@{

  /*! \brief Enumeration for the types of parameters supported by CoinParam

    CoinParam provides support for several types of parameters:
    <ul>
      <li> Action parameters, which require no value.
      <li> Integer and double numeric parameters, with upper and lower bounds.
      <li> String parameters that take an arbitrary string value.
      <li> Keyword parameters that take a defined set of string (value-keyword)
	   values. Value-keywords are associated with integers in the order in
	   which they are added, starting from zero.
    </ul>
  */
  typedef enum { coinParamInvalid = 0,
		 coinParamAct, coinParamInt, coinParamDbl,
		 coinParamStr, coinParamKwd } CoinParamType ;

  /*! \brief Type declaration for push and pull functions.

    By convention, a return code of  0 indicates execution without error, >0
    indicates nonfatal error, and <0 indicates fatal error. This is only
    convention, however; the base class makes no use of the push and pull
    functions and has no hardcoded interpretation of the return code.
  */
  typedef int (*CoinParamFunc)(CoinParam *param) ;

//@}

/*! \name Constructors and Destructors

  Be careful how you specify parameters for the constructors! Some compilers
  are entirely too willing to convert almost anything to bool.
*/
//@{

  /*! \brief Default constructor */

  CoinParam() ;

  /*! \brief Constructor for a parameter with a double value
  
    The default value is 0.0. Be careful to clearly indicate that \p lower and
    \p upper are real (double) values to distinguish this constructor from the
    constructor for an integer parameter.
  */
  CoinParam(std::string name, std::string help,
	    double lower, double upper, double dflt = 0.0,
	    bool display = true) ;

  /*! \brief Constructor for a parameter with an integer value
  
    The default value is 0.
  */
  CoinParam(std::string name, std::string help,
	    int lower, int upper, int dflt = 0,
	    bool display = true) ;

  /*! \brief Constructor for a parameter with keyword values

    The string supplied as \p firstValue becomes the first value-keyword.
    Additional value-keywords can be added using appendKwd().  It's necessary
    to specify both the first value-keyword (\p firstValue) and the default
    value-keyword index (\p dflt) in order to distinguish this constructor
    from the constructors for string and action parameters.

    Value-keywords are associated with an integer, starting with zero and
    increasing as each keyword is added.  The value-keyword given as \p
    firstValue will be associated with the integer zero. The integer supplied
    for \p dflt can be any value, as long as it will be valid once all
    value-keywords have been added.
  */
  CoinParam(std::string name, std::string help,
	    std::string firstValue, int dflt, bool display = true) ;

  /*! \brief Constructor for a string parameter

    For some compilers, the default value (\p dflt) must be specified
    explicitly with type std::string to distinguish the constructor for a
    string parameter from the constructor for an action parameter. For
    example, use std::string("default") instead of simply "default", or use a
    variable of type std::string.
  */
  CoinParam(std::string name, std::string help,
	    std::string dflt, bool display = true) ;

  /*! \brief Constructor for an action parameter */

  CoinParam(std::string name, std::string help,
	    bool display = true) ;

  /*! \brief Copy constructor */

  CoinParam(const CoinParam &orig) ;

  /*! \brief Clone */

  virtual CoinParam *clone() ;

  /*! \brief Assignment */
  
    CoinParam &operator=(const CoinParam &rhs) ;

  /*! \brief  Destructor */

  virtual ~CoinParam() ;

//@}

/*! \name Methods to query and manipulate the value(s) of a parameter */
//@{

  /*! \brief Add an additional value-keyword to a keyword parameter */

  void appendKwd(std::string kwd) ;

  /*! \brief Return the integer associated with the specified value-keyword
  
    Returns -1 if no value-keywords match the specified string.
  */
  int kwdIndex(std::string kwd) const ;

  /*! \brief Return the value-keyword that is the current value of the
	     keyword parameter
  */
  std::string kwdVal() const ;

  /*! \brief Set the value of the keyword parameter using the integer
	     associated with a value-keyword.
  
    If \p printIt is true, the corresponding value-keyword string will be
    echoed to std::cout.
  */
  void setKwdVal(int value, bool printIt = false) ;

  /*! \brief Set the value of the keyword parameter using a value-keyword
	     string.
  
    The given string will be tested against the set of value-keywords for
    the parameter using the shortest match rules.
  */
  void setKwdVal(const std::string value ) ;

  /*! \brief Prints the set of value-keywords defined for this keyword
	     parameter
  */
  void printKwds() const ;


  /*! \brief Set the value of a string parameter */

  void setStrVal(std::string value) ;

  /*! \brief Get the value of a string parameter */

  std::string strVal() const ;


  /*! \brief Set the value of a double parameter */

  void setDblVal(double value) ;

  /*! \brief Get the value of a double parameter */

  double dblVal() const ;


  /*! \brief Set the value of a integer parameter */

  void setIntVal(int value) ;

  /*! \brief Get the value of a integer parameter */

  int intVal() const ;


  /*! \brief Add a short help string to a parameter */

  inline void setShortHelp(const std::string help) { shortHelp_ = help ; } 

  /*! \brief Retrieve the short help string */

  inline std::string shortHelp() const { return (shortHelp_) ; } 

  /*! \brief Add a long help message to a parameter
  
    See printLongHelp() for a description of how messages are broken into
    lines.
  */
  inline void setLongHelp(const std::string help) { longHelp_ = help ; } 

  /*! \brief Retrieve the long help message */

  inline std::string longHelp() const { return (longHelp_) ; } 

  /*! \brief  Print long help

    Prints the long help string, plus the valid range and/or keywords if
    appropriate. The routine makes a best effort to break the message into
    lines appropriate for an 80-character line. Explicit line breaks in the
    message will be observed. The short help string will be used if
    long help is not available.
  */
  void printLongHelp() const ;

//@}

/*! \name Methods to query and manipulate a parameter object */
//@{

  /*! \brief Return the type of the parameter */

  inline CoinParamType type() const { return (type_) ; } 

  /*! \brief Set the type of the parameter */

  inline void setType(CoinParamType type) { type_ = type ; } 

  /*! \brief Return the parameter keyword (name) string */

  inline std::string  name() const { return (name_) ; } 

  /*! \brief Set the parameter keyword (name) string */

  inline void setName(std::string name) { name_ = name ; processName() ; } 

  /*! \brief Check if the specified string matches the parameter keyword (name)
	     string
  
    Returns 1 if the string matches and meets the minimum match length,
    2 if the string matches but doesn't meet the minimum match length,
    and 0 if the string doesn't match. Matches are \e not case-sensitive.
  */
  int matches (std::string input) const ;

  /*! \brief Return the parameter keyword (name) string formatted to show
	     the minimum match length
  
    For example, if the parameter name was defined as allow!ableGap, the
    string returned by matchName would be allow(ableGap).
  */
  std::string matchName() const ;

  /*! \brief Set visibility of parameter

    Intended to control whether the parameter is shown when a list of
    parameters is processed. Used by CoinParamUtils::printHelp when printing
    help messages for a list of parameters.
  */
  inline void setDisplay(bool display) { display_ = display ; } 

  /*! \brief Get visibility of parameter */

  inline bool display() const { return (display_) ; } 

  /*! \brief Get push function */

  inline CoinParamFunc pushFunc() { return (pushFunc_) ; } 

  /*! \brief Set push function */

  inline void setPushFunc(CoinParamFunc func) { pushFunc_ = func ; }  

  /*! \brief Get pull function */

  inline CoinParamFunc pullFunc() { return (pullFunc_) ; } 

  /*! \brief Set pull function */

  inline void setPullFunc(CoinParamFunc func) { pullFunc_ = func ; } 

//@}

private:

/*! \name Private methods */
//@{

  /*! Process a name for efficient matching */
  void processName() ;

//@}

/*! \name Private parameter data */
//@{
  /// Parameter type (see #CoinParamType)
  CoinParamType type_ ;

  /// Parameter name
  std::string name_ ;

  /// Length of parameter name
  size_t lengthName_ ;

  /*! \brief  Minimum length required to declare a match for the parameter
	      name.
  */
  size_t lengthMatch_ ;

  /// Lower bound on value for a double parameter
  double lowerDblValue_ ;

  /// Upper bound on value for a double parameter
  double upperDblValue_ ;

  /// Double parameter - current value
  double dblValue_ ;

  /// Lower bound on value for an integer parameter
  int lowerIntValue_ ;

  /// Upper bound on value for an integer parameter
  int upperIntValue_ ;

  /// Integer parameter - current value
  int intValue_ ;

  /// String parameter - current value
  std::string strValue_ ;

  /// Set of valid value-keywords for a keyword parameter
  std::vector<std::string> definedKwds_ ;

  /*! \brief Current value for a keyword parameter (index into #definedKwds_)
  */
  int currentKwd_ ;

  /// Push function
  CoinParamFunc pushFunc_ ;

  /// Pull function
  CoinParamFunc pullFunc_ ;

  /// Short help
  std::string shortHelp_ ;

  /// Long help
  std::string longHelp_ ;

  /// Display when processing lists of parameters?
  bool display_ ;
//@}

} ;

/*! \relatesalso CoinParam
    \brief A type for a parameter vector.
*/
typedef std::vector<CoinParam*> CoinParamVec ;

/*! \relatesalso CoinParam
    \brief A stream output function for a CoinParam object.
*/
std::ostream &operator<< (std::ostream &s, const CoinParam &param) ;

/*
  Bring in the utility functions for parameter handling (CbcParamUtils).
*/

/*! \brief Utility functions for processing CoinParam parameters.

  The functions in CoinParamUtils support command line or interactive
  parameter processing and a help facility. Consult the `Related Functions'
  section of the CoinParam class documentation for individual function
  documentation.
*/
namespace CoinParamUtils {
  /*! \relatesalso CoinParam
      \brief Take command input from the file specified by src.

      Use stdin for \p src to specify interactive prompting for commands.
  */
  void setInputSrc(FILE *src) ;

  /*! \relatesalso CoinParam
      \brief Returns true if command line parameters are being processed.
  */
  bool isCommandLine() ;

  /*! \relatesalso CoinParam
      \brief Returns true if parameters are being obtained from stdin.
  */
  bool isInteractive() ;

  /*! \relatesalso CoinParam
      \brief Attempt to read a string from the input.
      
      \p argc and \p argv are used only if isCommandLine() would return true.
      If \p valid is supplied, it will be set to 0 if a string is parsed
      without error, 2 if no field is present.
  */
  std::string getStringField(int argc, const char *argv[], int *valid) ;

  /*! \relatesalso CoinParam
      \brief Attempt to read an integer from the input.
      
      \p argc and \p argv are used only if isCommandLine() would return true.
      If \p valid is supplied, it will be set to 0 if an integer is parsed
      without error, 1 if there's a parse error, and 2 if no field is present.
  */
  int getIntField(int argc, const char *argv[], int *valid) ;

  /*! \relatesalso CoinParam
      \brief Attempt to read a real (double) from the input.
      
      \p argc and \p argv are used only if isCommandLine() would return true.
      If \p valid is supplied, it will be set to 0 if a real number is parsed
      without error, 1 if there's a parse error, and 2 if no field is present.
  */
  double getDoubleField(int argc, const char *argv[], int *valid) ;

  /*! \relatesalso CoinParam
      \brief Scan a parameter vector for parameters whose keyword (name) string
	     matches \p name using minimal match rules.
      
       \p matchNdx is set to the index of the last parameter that meets the
       minimal match criteria (but note there should be at most one matching
       parameter if the parameter vector is properly configured). \p shortCnt
       is set to the number of short matches (should be zero for a properly
       configured parameter vector if a minimal match is found). The return
       value is the number of matches satisfying the minimal match requirement
       (should be 0 or 1 in a properly configured vector).
  */
  int matchParam(const CoinParamVec &paramVec, std::string name,
		 int &matchNdx, int &shortCnt) ;

  /*! \relatesalso CoinParam
      \brief Get the next command keyword (name)

    To be precise, return the next field from the current command input
    source, after a bit of processing. In command line mode (isCommandLine()
    returns true) the next field will normally be of the form `-keyword' or
    `--keyword' (\e i.e., a parameter keyword), and the string returned would
    be `keyword'. In interactive mode (isInteractive() returns true), the
    user will be prompted if necessary.  It is assumed that the user knows
    not to use the `-' or `--' prefixes unless specifying parameters on the
    command line.

    There are a number of special cases if we're in command line mode. The
    order of processing of the raw string goes like this:
    <ul>
      <li> A stand-alone `-' is forced to `stdin'.
      <li> A stand-alone '--' is returned as a word; interpretation is up to
	   the client.
      <li> A prefix of '-' or '--' is stripped from the string.
    </ul>
    If the result is the string `stdin', command processing shifts to
    interactive mode and the user is immediately prompted for a new command.

    Whatever results from the above sequence is returned to the user as the
    return value of the function. An empty string indicates end of input.

    \p prompt will be used only if it's necessary to prompt the user in
    interactive mode.
  */

  std::string getCommand(int argc, const char *argv[],
			 const std::string prompt, std::string *pfx = 0) ;

  /*! \relatesalso CoinParam
      \brief Look up the command keyword (name) in the parameter vector.
      	     Print help if requested.

    In the most straightforward use, \p name is a string without `?', and the
    value returned is the index in \p paramVec of the single parameter that
    matched \p name. One or more '?' characters at the end of \p name is a
    query for information. The routine prints short (one '?') or long (more
    than one '?') help messages for a query.  Help is also printed in the case
    where the name is ambiguous (some of the matches did not meet the minimal
    match length requirement).

    Note that multiple matches meeting the minimal match requirement is a
    configuration error. The mimimal match length for the parameters
    involved is too short.

    If provided as parameters, on return
    <ul>
      <li> \p matchCnt will be set to the number of matches meeting the
	   minimal match requirement
      <li> \p shortCnt will be set to the number of matches that did not
	   meet the miminal match requirement
      <li> \p queryCnt will be set to the number of '?' characters at the
	   end of the name
    </ul>

    The return values are:
    <ul>
      <li> >0: index in \p paramVec of the single unique match for \p name
      <li> -1: a query was detected (one or more '?' characters at the end
	       of \p name
      <li> -2: one or more short matches, not a query
      <li> -3: no matches, not a query
      <li> -4: multiple matches meeting the minimal match requirement
	       (configuration error)
    </ul>
  */
  int lookupParam(std::string name, CoinParamVec &paramVec, 
		  int *matchCnt = 0, int *shortCnt = 0, int *queryCnt = 0) ;

  /*! \relatesalso CoinParam
      \brief Utility to print a long message as filled lines of text

      The routine makes a best effort to break lines without exceeding the
      standard 80 character line length. Explicit newlines in \p msg will
      be obeyed.
  */
  void printIt(const char *msg) ;

  /*! \relatesalso CoinParam
      \brief Utility routine to print help given a short match or explicit
	     request for help.

      The two really are related, in that a query (a string that ends with
      one or more `?' characters) will often result in a short match. The
      routine expects that \p name matches a single parameter, and does not
      look for multiple matches.
      
      If called with \p matchNdx < 0, the routine will look up \p name in \p
      paramVec and print the full name from the parameter. If called with \p
      matchNdx > 0, it just prints the name from the specified parameter.  If
      the name is a query, short (one '?') or long (more than one '?') help
      is printed.

  */ void shortOrHelpOne(CoinParamVec &paramVec,int matchNdx, std::string
  name, int numQuery) ;

  /*! \relatesalso CoinParam
      \brief Utility routine to print help given multiple matches.

      If the name is not a query, or asks for short help (\e i.e., contains
      zero or one '?' characters), the list of matching names is printed. If
      the name asks for long help (contains two or more '?' characters),
      short help is printed for each matching name.
  */
  void shortOrHelpMany(CoinParamVec &paramVec,
		       std::string name, int numQuery) ;

  /*! \relatesalso CoinParam
      \brief Print a generic `how to use the command interface' help message.

    The message is hard coded to match the behaviour of the parsing utilities.
  */
  void printGenericHelp() ;

  /*! \relatesalso CoinParam
      \brief Utility routine to print help messages for one or more
	     parameters.
    
    Intended as a utility to implement explicit `help' commands. Help will be
    printed for all parameters in \p paramVec from \p firstParam to \p
    lastParam, inclusive. If \p shortHelp is true, short help messages will
    be printed. If \p longHelp is true, long help messages are printed. \p
    shortHelp overrules \p longHelp. If neither is true, only command
    keywords are printed. \p prefix is printed before each line; it's an
    imperfect attempt at indentation.
  */
  void printHelp(CoinParamVec &paramVec, int firstParam, int lastParam,
		 std::string prefix,
		 bool shortHelp, bool longHelp, bool hidden) ;
}


#endif	/* CoinParam_H */

