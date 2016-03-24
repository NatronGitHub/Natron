/* $Id: CoinParamUtils.cpp 1468 2011-09-03 17:19:13Z stefan $ */
// Copyright (C) 2007, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <cassert>
#include <cerrno>
#include <iostream>

#include "CoinUtilsConfig.h"
#include "CoinParam.hpp"
#include <cstdlib>
#include <cstring>
#include <cstdio>

#ifdef COIN_HAS_READLINE     
#include <readline/readline.h>
#include <readline/history.h>
#endif

/* Unnamed local namespace */
namespace
{

/*
  cmdField: The index of the current command line field. Forced to -1 when
	    accepting commands from stdin (interactive) or a command file.
  readSrc:  Current input source.

  pendingVal: When the form param=value is encountered, both keyword and value
	    form one command line field. We need to return `param' as the
	    field and somehow keep the value around for the upcoming call
	    that'll request it. That's the purpose of pendingVal.
*/

int cmdField = 1 ;
FILE *readSrc = stdin ;
std::string pendingVal = "" ;


/*
  Get next command or field in command. When in interactive mode, prompt the
  user and read the resulting line of input.
*/
std::string nextField (const char *prompt)
{
  static char line[1000] ;
  static char *where = NULL ;
  std::string field ;
  const char *dflt_prompt = "Eh? " ;

  if (prompt == 0)
  { prompt = dflt_prompt ; }
/*
  Do we have a line at the moment? If not, acquire one. When we're done,
  line holds the input line and where points to the start of the line. If we're
  using the readline library, add non-empty lines to the history list.
*/
  if (!where) {
#ifdef COIN_HAS_READLINE
    if (readSrc == stdin)
    { where = readline(prompt) ;
      if (where)
      { if (*where)
	  add_history (where) ;
	strcpy(line,where) ;
	free(where) ;
	where = line ; } }
    else
    { where = fgets(line,1000,readSrc) ; }
#else
    if (readSrc == stdin)
      { fprintf(stdout,"%s",prompt) ;
      fflush(stdout) ; }
    where = fgets(line,1000,readSrc) ;
#endif
/*
  If where is NULL, we have EOF. Return a null string.
*/
    if (!where)
      return field ;
/*
  Clean the image. Trailing junk first. The line will be cut off at the last
  non-whitespace character, but we need to scan until we find the end of the
  string or some other non-printing character to make sure we don't miss a
  printing character after whitespace.
*/
    char *lastNonBlank = line-1 ;
    for (where = line ; *where != '\0' ; where++)
    { if (*where != '\t' && *where < ' ')
      { break ; }
      if (*where != '\t' && *where != ' ')
      { lastNonBlank = where ; } }
    *(lastNonBlank+1) = '\0' ;
    where = line ; }
/*
  Munch through leading white space.
*/
  while (*where == ' ' || *where == '\t')
    where++ ;
/*
  See if we can separate a field; if so, copy it over into field for return.
  If we're out of line, return the string "EOL".
*/
  char *saveWhere = where ;
  while (*where != ' ' && *where != '\t' && *where!='\0')
    where++ ;
  if (where != saveWhere)
  { char save = *where ;
    *where = '\0' ;
    field = saveWhere ;
    *where = save ; }
  else
  { where = NULL ;
    field = "EOL" ; }

  return (field) ; }

}


/* Visible functions */

namespace CoinParamUtils
{

/*
  As mentioned above, cmdField set to -1 is the indication that we're reading
  from stdin or a file.
*/
void setInputSrc (FILE *src)

{ if (src != 0)
  { cmdField = -1 ;
    readSrc = src ; } }

/*
  A utility to allow clients to determine if we're processing parameters from
  the comand line or otherwise.
*/
bool isCommandLine ()

{ assert(cmdField != 0) ;
  
  if (cmdField > 0)
  { return (true) ; }
  else
  { return (false) ; } }

/*
  A utility to allow clients to determine if we're accepting parameters
  interactively.
*/
bool isInteractive ()

{ assert(cmdField != 0) ;
  
  if (cmdField < 0 && readSrc == stdin)
  { return (true) ; }
  else
  { return (false) ; } }

/*
  Utility functions for acquiring input.
*/


/*
  Return the next field (word) from the current command line. Generally, this
  is expected to be of the form `-param' or `--param', with special cases as
  set out below.

  If we're in interactive mode (cmdField == -1), nextField does all the work
  to prompt the user and return the next field from the resulting input. It is
  assumed that the user knows not to use `-' or `--' prefixes in interactive
  mode.

  If we're in command line mode (cmdField > 0), cmdField indicates the
  current command line word. The order of processing goes like this:
    * A stand-alone `-' is converted to `stdin'
    * A stand-alone '--' is returned as a word; interpretation is up to the
      client.
    * A prefix of '-' or '--' is stripped from the field.
  If the result is `stdin', it's assumed we're switching to interactive mode
  and the user is prompted for another command.

  Whatever results from the above sequence is returned to the client as the
  next field. An empty string indicates end of input.

  Prompt will be used by nextField if it's necessary to prompt the user for
  a command (only when reading from stdin).

  If provided, pfx is set to the prefix ("-", "--", or "") stripped from the
  field. Lack of prefix is not necessarily an error because of the following
  scenario:  To read a file, the verbose command might be "foo -import
  myfile". But we might want to allow a short form, "foo myfile". And we'd
  like "foo import" to be interpreted as "foo -import import" (i.e., import the
  file named `import').
*/

std::string getCommand (int argc, const char *argv[],
			const std::string prompt, std::string *pfx)

{ std::string field = "EOL" ;
  pendingVal = "" ;
  int pfxlen ;

  if (pfx != 0)
  { (*pfx) = "" ; }
/*
  Acquire the next field, and convert as outlined above if we're processing
  command line parameters.
*/
  while (field == "EOL")
  { pfxlen = 0 ;
    if (cmdField > 0)
    { if (cmdField < argc)
      { field = argv[cmdField++] ;
	if (field == "-")
	{ field = "stdin" ; }
	else
	if (field == "--")
	{ /* Prevent `--' from being eaten by next case. */ }
	else
	{ if (field[0] == '-')
	  { pfxlen = 1 ;
	    if (field[1] == '-')
	      pfxlen = 2 ;
	    if (pfx != 0)
	      (*pfx) = field.substr(0,pfxlen) ;
	    field = field.substr(pfxlen) ; } } }
      else
      { field = "" ; } }
    else
    { field = nextField(prompt.c_str()) ; }
    if (field == "stdin")
    { std::cout << "Switching to line mode" << std::endl ;
      cmdField = -1 ;
      field = nextField(prompt.c_str()) ; } }
/*
  Are we left with something of the form param=value? If so, separate the
  pieces, returning `param' and saving `value' for later use as per comments
  at the head of the file.
*/
  std::string::size_type found = field.find('=');
  if (found != std::string::npos)
  { pendingVal = field.substr(found+1) ;
    field = field.substr(0,found) ; }

  return (field) ; }


/*
  Function to look up a parameter keyword (name) in the parameter vector and
  deal with the result. The keyword may end in one or more `?' characters;
  this is a query for information about matching parameters.

  If we have a single match satisfying the minimal match requirements, and
  there's no query, we simply return the index of the matching parameter in
  the parameter vector. If there are no matches, and no query, the return
  value will be -3. No matches on a query returns -1.

  A single short match, or a single match of any length with a query, will
  result in a short help message

  If present, these values are set as follows:
    * matchCntp is set to the number of parameters that matched.
    * shortCntp is set to the number of matches that failed to meet the minimum
      match requirement.
    * queryCntp is set to the number of trailing `?' characters at the end
      of name.

  Return values:
    >0:	index of the single unique match for the name
    -1: query present
    -2: no query, one or more short matches
    -3: no query, no match
    -4: multiple full matches (indicates configuration error)

  The final three parameters (matchCnt, shortCnt, queryCnt) are optional and
  default to null. Use them if you want more detail on the match.
*/

int lookupParam (std::string name, CoinParamVec &paramVec,
		 int *matchCntp, int *shortCntp, int *queryCntp)

{
  int retval = -3 ;

  if (matchCntp != 0)
  { *matchCntp = 0 ; }
  if (shortCntp != 0)
  { *shortCntp = 0 ; }
  if (queryCntp != 0)
  { *queryCntp = 0 ; }
/*
  Is there anything here at all? 
*/
  if (name.length() == 0)
  { return (retval) ; }
/*
  Scan the parameter name to see if it ends in one or more `?' characters. If
  so, take it as a request to return a list of parameters that match name up
  to the first `?'.  The strings '?' and '???' are considered to be valid
  parameter names (short and long help, respectively) and are handled as
  special cases: If the whole string is `?'s, one and three are commands as
  is, while 2 and 4 or more are queries about `?' or `???'.
*/
  int numQuery = 0 ;
  { int length = static_cast<int>(name.length()) ;
    int i ;
    for (i = length-1 ; i >= 0 && name[i] == '?' ; i--)
    { numQuery++ ; }
    if (numQuery == length)
    { switch (length)
      { case 1:
	case 3:
	{ numQuery = 0 ;
	  break ; }
	case 2:
	{ numQuery -= 1 ;
	  break ; }
        default:
	{ numQuery -= 3 ;
	  break ; } } }
    name = name.substr(0,length-numQuery) ;
    if (queryCntp != 0)
    { *queryCntp = numQuery ; } }
/*
  See if we can match the parameter name. On return, matchNdx is set to the
  last match satisfying the minimal match criteria, or -1 if there's no
  match.  matchCnt is the number of matches satisfying the minimum match
  length, and shortCnt is possible matches that were short of the minimum
  match length,
*/
  int matchNdx = -1 ;
  int shortCnt = 0 ;
  int matchCnt = CoinParamUtils::matchParam(paramVec,name,matchNdx,shortCnt) ;
/*
  Set up return values before we get into further processing.
*/
  if (matchCntp != 0)
  { *matchCntp = matchCnt ; }
  if (shortCntp != 0)
  { *shortCntp = shortCnt ; }
  if (numQuery > 0)
  { retval = -1 ; }
  else
  { if (matchCnt+shortCnt == 0)
    { retval = -3 ; }
    else
    if (matchCnt > 1)
    { retval = -4 ; }
    else
    { retval = -2 ; } }
/*
  No matches? Nothing more to be done here.
*/
  if (matchCnt+shortCnt == 0)
  { return (retval) ; }
/*
  A unique match and no `?' in the name says we have our parameter. Return
  the result.
*/
  if (matchCnt == 1 && shortCnt == 0 && numQuery == 0)
  { assert (matchNdx >= 0 && matchNdx < static_cast<int>(paramVec.size())) ;
    return (matchNdx) ; }
/*
  A single match? There are two possibilities:
    * The string specified is shorter than the match length requested by the
      parameter. (Useful for avoiding inadvertent execution of commands that
      the client might regret.)
    * The string specified contained a `?', in which case we print the help.
      The match may or may not be short.
*/
  if (matchCnt+shortCnt == 1)
  { CoinParamUtils::shortOrHelpOne(paramVec,matchNdx,name,numQuery) ;
    return (retval) ; }
/*
  The final case: multiple matches. Most commonly this will be multiple short
  matches. If we have multiple matches satisfying the minimal length
  criteria, we have a configuration problem.  The other question is whether
  the user wanted help information. Two question marks gets short help.
*/
  if (matchCnt > 1)
  { std::cout
    << "Configuration error! `" << name
    <<"' was fully matched " << matchCnt << " times!"
    << std::endl ; }
  std::cout
    << "Multiple matches for `" << name << "'; possible completions:"
    << std::endl ;
  CoinParamUtils::shortOrHelpMany(paramVec,name,numQuery) ;

  return (retval) ; }


/*
  Utility functions to acquire parameter values from the command line. For
  all of these, a pendingVal is consumed if it exists.
*/


/*
  Read a string and return a pointer to the string. Set valid to indicate the
  result of parsing: 0: okay, 1: <unused>, 2: not present.
*/

std::string getStringField (int argc, const char *argv[], int *valid)

{ std::string field ;

  if (pendingVal != "")
  { field = pendingVal ;
    pendingVal = "" ; }
  else
  { field = "EOL" ;
    if (cmdField > 0)
    { if (cmdField < argc)
      { field = argv[cmdField++] ; } }
    else
    { field = nextField(0) ; } }

  if (valid != 0)
  { if (field != "EOL")
    { *valid = 0 ; }
    else
    { *valid = 2 ; } }

  return (field) ; }

/*
  Read an int and return the value. Set valid to indicate the result of
  parsing: 0: okay, 1: parse error, 2: not present.
*/

int getIntField (int argc, const char *argv[], int *valid)

{ std::string field ;

  if (pendingVal != "")
  { field = pendingVal ;
    pendingVal = "" ; }
  else
  { field = "EOL" ;
    if (cmdField > 0)
    { if (cmdField < argc)
      { field = argv[cmdField++] ; } }
    else
    { field = nextField(0) ; } }
/*
  The only way to check for parse error here is to set the system variable
  errno to 0 and then see if it's nonzero after we try to convert the string
  to integer.
*/
  int value = 0 ;
  errno = 0 ;
  if (field != "EOL")
  { value =  atoi(field.c_str()) ; }

  if (valid != 0)
  { if (field != "EOL")
    { if (errno == 0)
      { *valid = 0 ; }
      else
      { *valid = 1 ; } }
    else
    { *valid = 2 ; } }

  return (value) ; }


/*
  Read a double and return the value. Set valid to indicate the result of
  parsing: 0: okay, 1: bad parse, 2: not present. But we'll never return
  valid == 1 because atof gives us no way to tell.)
*/

double getDoubleField (int argc, const char *argv[], int *valid)

{ std::string field ;

  if (pendingVal != "")
  { field = pendingVal ;
    pendingVal = "" ; }
  else
  { field = "EOL" ;
    if (cmdField > 0)
    { if (cmdField < argc)
      { field = argv[cmdField++] ; } }
    else
    { field = nextField(0) ; } }
/*
  The only way to check for parse error here is to set the system variable
  errno to 0 and then see if it's nonzero after we try to convert the string
  to integer.
*/
  double value = 0.0 ;
  errno = 0 ;
  if (field != "EOL")
  { value = atof(field.c_str()) ; }

  if (valid != 0)
  { if (field != "EOL")
    { if (errno == 0)
      { *valid = 0 ; }
      else
      { *valid = 1 ; } }
    else
    { *valid = 2 ; } }

  return (value) ; }


/*
  Utility function to scan a parameter vector for matches. Sets matchNdx to
  the index of the last parameter that meets the minimal match criteria (but
  note there should be at most one such parameter if the parameter vector is
  properly configured). Sets shortCnt to the number of short matches (should
  be zero in a properly configured vector if a minimal match is found).
  Returns the number of matches satisfying the minimal match requirement
  (should be 0 or 1 in a properly configured vector).

  The routine allows for the possibility of null entries in the parameter
  vector.

  In order to handle `?' and `???', there's nothing to it but to force a
  unique match if we match `?' exactly. (This is another quirk of clp/cbc
  parameter parsing, which we need to match for historical reasons.)
*/

int matchParam (const CoinParamVec &paramVec, std::string name,
		int &matchNdx, int &shortCnt)

{ 
  int vecLen = static_cast<int>(paramVec.size()) ;
  int matchCnt = 0 ;

  matchNdx = -1 ;
  shortCnt = 0 ;

  for (int i = 0 ; i < vecLen  ; i++)
  { CoinParam *param =  paramVec[i] ;
    if (param == 0) continue ;
    int match = paramVec[i]->matches(name) ;
    if (match == 1)
    { matchNdx = i ;
      matchCnt++ ;
      if (name == "?")
      { matchCnt = 1 ;
	break ; } }
    else
    { shortCnt += match>>1 ; } }

  return (matchCnt) ;
}

/*
  Now a bunch of routines that are useful in the context of generating help
  messages.
*/

/*
  Simple formatting routine for long messages. Used to print long help for
  parameters. Lines are broken at the first white space after 65 characters,
  or when an explicit return (`\n') character is scanned. Leading spaces are
  suppressed.
*/

void printIt (const char *msg)

{ int length = static_cast<int>(strlen(msg)) ;
  char temp[101] ;
  int i ;
  int n = 0 ;
  for (i = 0 ; i < length ; i++)
  { if (msg[i] == '\n' ||
	(n >= 65 && (msg[i] == ' ' || msg[i] == '\t')))
    { temp[n] = '\0' ;
      std::cout << temp << std::endl ;
      n = 0 ; }
    else
    if (n || msg[i] != ' ')
    { temp[n++] = msg[i] ; } }
  if (n > 0)
  { temp[n] = '\0' ;
    std::cout << temp << std::endl ; }

  return ; }


/*
  Utility function for the case where a name matches a single parameter, but
  either it's short, or the user wanted help, or both.

  The routine allows for the possibility that there are null entries in the
  parameter vector, but matchNdx should point to a valid entry if it's >= 0.
*/

void shortOrHelpOne (CoinParamVec &paramVec,
		     int matchNdx, std::string name, int numQuery)

{ int i ;
  int numParams = static_cast<int>(paramVec.size()) ;
  int lclNdx = -1 ;
/*
  For a short match, we need to look up the parameter again. This should find
  a short match, given the conditions where this routine is called. But be
  prepared to find a full match.
  
  If matchNdx >= 0, just use the index we're handed.
*/
  if (matchNdx < 0) 
  { int match = 0 ;
    for (i = 0 ; i < numParams ; i++)
    { CoinParam *param =  paramVec[i] ;
      if (param == 0) continue ;
      int match = param->matches(name) ;
      if (match != 0)
      { lclNdx = i ;
	break ; } }

    assert (lclNdx >= 0) ;

    if (match == 1)
    { std::cout
	<< "Match for '" << name << "': "
	<< paramVec[matchNdx]->matchName() << "." ; }
    else
    { std::cout
      << "Short match for '" << name << "'; possible completion: "
      << paramVec[lclNdx]->matchName() << "." ; } }
  else
  { assert(matchNdx >= 0 && matchNdx < static_cast<int>(paramVec.size())) ;
    std::cout << "Match for `" << name << "': "
	      << paramVec[matchNdx]->matchName() ;
    lclNdx = matchNdx ; }
/*
  Print some help, if there was a `?' in the name. `??' gets the long help.
*/
  if (numQuery > 0)
  { std::cout << std::endl ;
    if (numQuery == 1)
    { std::cout << paramVec[lclNdx]->shortHelp() ; }
    else
    { paramVec[lclNdx]->printLongHelp() ; } }
  std::cout << std::endl ;

  return ; }

/*
  Utility function for the case where a name matches multiple parameters.
  Zero or one `?' gets just the matching names, while `??' gets short help
  with each match.

  The routine allows for the possibility that there are null entries in the
  parameter vector.
*/

void shortOrHelpMany (CoinParamVec &paramVec, std::string name, int numQuery)

{ int numParams = static_cast<int>(paramVec.size()) ;
/*
  Scan the parameter list. For each match, print just the name, or the name
  and short help.
*/
  int lineLen = 0 ;
  bool printed = false ;
  for (int i = 0 ; i < numParams ; i++)
  { CoinParam *param = paramVec[i] ;
    if (param == 0) continue ;
    int match = param->matches(name) ;
    if (match > 0)
    { std::string nme = param->matchName() ;
      int len = static_cast<int>(nme.length()) ;
      if (numQuery >= 2) 
      { std::cout << nme << " : " << param->shortHelp() ;
	std::cout << std::endl ; }
      else
      { lineLen += 2+len ;
	if (lineLen > 80)
	{ std::cout << std::endl ;
	  lineLen = 2+len ; }
	std::cout << "  " << nme ;
	printed = true ; } } }

  if (printed)
  { std::cout << std::endl ; }

  return ; }


/*
  A generic help message that explains the basic operation of parameter
  parsing.
*/

void printGenericHelp ()

{ std::cout << std::endl ;
  std::cout
    << "For command line arguments, keywords have a leading `-' or '--'; "
    << std::endl ;
  std::cout
    << "-stdin or just - switches to stdin with a prompt."
    << std::endl ;
  std::cout
    << "When prompted, one command per line, without the leading `-'."
    << std::endl ;
  std::cout
    << "abcd value sets abcd to value."
    << std::endl ;
  std::cout
    << "abcd without a value (where one is expected) gives the current value."
    << std::endl ;
  std::cout
    << "abcd? gives a list of possible matches; if there's only one, a short"
    << std::endl ;
  std::cout
    << "help message is printed."
    << std::endl ;
  std::cout
    << "abcd?? prints the short help for all matches; if there's only one"
    << std::endl ;
  std::cout
    << "match, a longer help message and current value are printed."
    << std::endl ;
  
  return ; }


/*
  Utility function for various levels of `help' command. The entries between
  paramVec[firstParam] and paramVec[lastParam], inclusive, will be printed.
  If shortHelp is true, the short help message will be printed for each
  parameter. If longHelp is true, the long help message will be printed for
  each parameter.  If hidden is true, even parameters with display = false
  will be printed. Each line is prefaced with the specified prefix.

  The routine allows for the possibility that there are null entries in the
  parameter vector.
*/

void printHelp (CoinParamVec &paramVec, int firstParam, int lastParam,
		std::string prefix,
		bool shortHelp, bool longHelp, bool hidden)

{ bool noHelp = !(shortHelp || longHelp) ;
  int i ;
  int pfxLen = static_cast<int>(prefix.length()) ;
  bool printed = false ;

  if (noHelp)
  { int lineLen = 0 ;
    for (i = firstParam ; i <= lastParam ; i++)
    { CoinParam *param = paramVec[i] ;
      if (param == 0) continue ;
      if (param->display() || hidden)
      { std::string nme = param->matchName() ;
	int len = static_cast<int>(nme.length()) ;
	if (!printed)
	{ std::cout << std::endl << prefix ;
	  lineLen += pfxLen ;
	  printed = true ; }
	lineLen += 2+len ;
	if (lineLen > 80)
	{ std::cout << std::endl << prefix ;
	  lineLen = pfxLen+2+len ; }
        std::cout << "  " << nme ; } }
    if (printed)
    { std::cout << std::endl ; } }
  else
  if (shortHelp)
  { for (i = firstParam ; i <= lastParam ; i++)
    { CoinParam *param = paramVec[i] ;
      if (param == 0) continue ;
      if (param->display() || hidden)
      { std::cout << std::endl << prefix ;
	std::cout << param->matchName() ;
	std::cout << ": " ;
	std::cout << param->shortHelp() ; } }
      std::cout << std::endl ; }
  else
  if (longHelp)
  { for (i = firstParam ; i <= lastParam ; i++)
    { CoinParam *param = paramVec[i] ;
      if (param == 0) continue ;
      if (param->display() || hidden)
      { std::cout << std::endl << prefix ;
	std::cout << "Command: " << param->matchName() ;
        std::cout << std::endl << prefix ;
	std::cout << "---- description" << std::endl ;
	printIt(param->longHelp().c_str()) ;
	std::cout << prefix << "----" << std::endl ; } } }

  std::cout << std::endl ;

  return ; }   

} // end namespace CoinParamUtils
