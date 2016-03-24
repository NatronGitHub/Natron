// Copyright (c) 2006, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

/*
  A brief test of CoinMessageHandler. Tests that we can print basic messages,
  and checks that we can handle messages with parts. Does a few more subtle
  tests involving cloning and handlers with no associated messages.

  Does not attempt to test any of the various enbryonic features.
*/

#include "CoinPragma.hpp"
#include "CoinMessageHandler.hpp"
#include <cstring>

namespace { // begin file-local namespace

/*
  Define a set of test messages.
*/
enum COIN_TestMessage
{ COIN_TST_NOFIELDS,
  COIN_TST_INT,
  COIN_TST_DBL,
  COIN_TST_DBLFMT,
  COIN_TST_CHAR,
  COIN_TST_STRING,
  COIN_TST_MULTIPART,
  COIN_TST_NOCODES,
  COIN_TST_END
} ;

/*
  Convenient structure for doing static initialisation. Essentially, we want
  something that'll allow us to set up an array of structures with the
  information required by CoinOneMessage. Then we can easily walk the array,
  create a CoinOneMessage structure for each message, and add each message to
  a CoinMessages structure.
*/
typedef struct
{ COIN_TestMessage internalNumber;
  int externalNumber;
  char detail;
  const char * message;
} MsgDefn ;

MsgDefn us_tstmsgs[] =
{ {COIN_TST_NOFIELDS,1,1,"This message has no parts and no fields."},
  {COIN_TST_INT,3,1,"This message has an integer field: (%d)"},
  {COIN_TST_DBL,4,1,"This message has a double field: (%g)"},
  {COIN_TST_DBLFMT,4,1,
      "This message has an explicit precision .3: (%.3g)"},
  {COIN_TST_CHAR,5,1,"This message has a char field: (%c)"},
  {COIN_TST_STRING,6,1,"This message has a string field: (%s)"},
  {COIN_TST_MULTIPART,7,1,
    "Prefix%? Part 1%? Part 2 with integer in hex %#.4x%? Part 3%? suffix."},
  {COIN_TST_NOCODES,8,1,""},
  {COIN_TST_END,7,1,"This is the dummy end marker."}
} ;


/*
  Tests that don't use formatted messages.
*/
void testsWithoutMessages (int &errs)

{ CoinMessageHandler hdl ;
/*
  format_ must be null in order for on-the-fly message construction to work
  properly.
*/
  hdl.message()
    << "This should print if the constructor sets format_ to null."
    << CoinMessageEol ;
/*
  Log level should have no effect by default, so set it to 0 to prove the
  point.
*/
  hdl.message()
    << "By default, the log level has no effect for on-the-fly messages."
    << CoinMessageEol ;
  hdl.setLogLevel(0) ;
  if (hdl.logLevel() != 0)
  { std::cout
      << "Cannot set/get log level of 0!" << std::endl ;
    errs++ ; }
  hdl.message()
    << "Log level is now" << hdl.logLevel() << "." << CoinMessageEol ;
/*
  But we can specify a log level and it will be effective. What's more, each
  message is completely independent, so the default behaviour should return
  after an explicit log level suppressed printing.
*/
  hdl.message()
    << "But we can specify a log level and have it take effect."
    << CoinMessageEol ;
  hdl.message(1)
    << "This message should not print!" << CoinMessageEol ;
  hdl.message()
    << "If you saw a message that said 'should not print', there's a problem."
    << CoinMessageEol ;
/*
  This next sequence exposed a subtle bug in cloning. Failure here may well
  cause a core dump. Here's the scenario: Since we haven't used any messages,
  rhs.format_ is null and rhs.currentMessage_ is set to the default for
  CoinOneMessage, which sets the format string to the null string "\0".
  Cloning assumed that rhs.format_ was a pointer into the format string from
  currentMessage_, and proceeded to set up format_ in the clone to be a
  pointer into the cloned currentMessage_, allowing for the fact that we
  might be somewhere in the middle of the message at the time of cloning.
  Net result was that format_ was non-null in the clone, but god only knows
  where it pointed to. When the code tried to write to *format_, the result
  was a core dump.
*/
  hdl.message()
    << "A core dump here indicates a cloning failure." << CoinMessageEol ;
  CoinMessageHandler hdlClone(hdl) ;
  hdlClone.message()
    << "This should print if cloning sets format_ to null."
    << CoinMessageEol ;

  return ; }


/*
  Basic functionality for printing messages. Check that supported parameter
  types work, and that we can selectively suppress portions of a message.
*/
void basicTestsWithMessages (const CoinMessages &testMessages, int &errs)
{
  CoinMessageHandler hdl ;
  hdl.setLogLevel(1) ;
  if (hdl.logLevel() != 1)
  { std::cout
      << "Cannot set/get log level of 1!" << std::endl ;
    errs++ ; }
/*
  Simple tests of one piece messages.
*/
  hdl.message(COIN_TST_NOFIELDS,testMessages) << CoinMessageEol ;
  hdl.message(COIN_TST_INT,testMessages) << 42 << CoinMessageEol ;
  hdl.message(COIN_TST_DBL,testMessages) << (42.42+1.0/7.0) << CoinMessageEol ;
  std::cout << "Changing to 10 significant digits." << std::endl ;
  int savePrecision = hdl.precision() ;
  hdl.setPrecision(10) ;
  hdl.message(COIN_TST_DBL,testMessages) << (42.42+1.0/7.0) << CoinMessageEol ;
  std::cout
    << "And back to " << savePrecision
    << " significant digits." << std::endl ;
  hdl.setPrecision(savePrecision) ;
  hdl.message(COIN_TST_DBL,testMessages) << (42.42+1.0/7.0) << CoinMessageEol ;
  hdl.message(COIN_TST_DBLFMT,testMessages)
       << (42.42+1.0/7.0) << CoinMessageEol ;

  hdl.message(COIN_TST_CHAR,testMessages) << 'w' << CoinMessageEol ;
  hdl.message(COIN_TST_STRING,testMessages) << "forty-two" << CoinMessageEol ;
/*
  A multipart message, consisting of prefix, three optional parts, and a
  suffix. Note that we need four calls to printing() in order to process the
  four `%?' codes.
*/
  hdl.message(COIN_TST_MULTIPART,testMessages) ;
  hdl.printing(true) ;
  hdl.printing(true) << 42 ;
  hdl.printing(true) ;
  hdl.printing(true) << CoinMessageEol ;
  hdl.message(COIN_TST_MULTIPART,testMessages) ;
  hdl.printing(false) ;
  hdl.printing(false) << 42 ;
  hdl.printing(false) ;
  hdl.printing(true) << CoinMessageEol ;
  hdl.message(COIN_TST_MULTIPART,testMessages) ;
  hdl.printing(true) ;
  hdl.printing(false) << 42 ;
  hdl.printing(false) ;
  hdl.printing(true) << CoinMessageEol ;
  hdl.message(COIN_TST_MULTIPART,testMessages) ;
  hdl.printing(false) ;
  hdl.printing(true) << 42 ;
  hdl.printing(false) ;
  hdl.printing(true) << CoinMessageEol ;
  hdl.message(COIN_TST_MULTIPART,testMessages) ;
  hdl.printing(false) ;
  hdl.printing(false) << 42 ;
  hdl.printing(true) ;
  hdl.printing(true) << CoinMessageEol ;
/*
  Construct a message from scratch given an empty message. Parameters are
  printed with default format codes.
*/
  hdl.message(COIN_TST_NOCODES,testMessages) ;
  hdl.message() << "Standardised prefix, free form remainder:" ;
  hdl.message() << "An int" << 42
   << "A double" << 4.2
   << "a new line" << CoinMessageNewline
   << "and done." << CoinMessageEol ;
/*
  Construct a message from scratch given nothing at all. hdl.finish is
  equivalent to CoinMessageEol (more accurately, processing CoinMessagEol
  consists of a call to finish).
*/
  hdl.message() << "No standardised prefix, free form reminder: integer ("
    << 42 << ")." ;
  hdl.finish() ;
/*
  And the transition mechanism, where we just dump the string we're given.
  It's not possible to augment this message, as printStatus_ is set to 2,
  which prevents the various operator<< methods from contributing to the
  output buffer, with the exception of operator<<(CoinMessageMarker).
*/
  hdl.message(27,"Tran","Transition message.",'I') << CoinMessageEol ;

  return ; }


/*
  More difficult tests. Clone a handler in mid-message. Why? Because we can.
*/
void advTestsWithMessages (const CoinMessages &testMessages, int &errs)
{
  CoinMessageHandler hdl ;
/*
  A multipart message, consisting of prefix, three optional parts, and a
  suffix. Note that we need four calls to printing() in order to process the
  four `%?' codes.
*/
  hdl.message() << "Trying a clone in mid-message." << CoinMessageEol ;

  hdl.message(COIN_TST_MULTIPART,testMessages) ;
  hdl.printing(true) ;
  hdl.printing(true) ;
  CoinMessageHandler hdl2 ;
  hdl2 = hdl ;
  hdl2.message() << 42 ;
  hdl2.printing(true) ;
  hdl2.printing(true) << CoinMessageEol ;

  hdl.message() << 0x42 ;
  hdl.printing(false) ;
  hdl.printing(false) << CoinMessageEol ;

  hdl.message()
    << "The second copy should be missing Part 3 and suffix."
    << CoinMessageEol ;

  return ; }

} // end file-local namespace

bool CoinMessageHandlerUnitTest ()

{ int errs = 0 ;

/*
  Create a CoinMessages object to hold our messages. 
*/
  CoinMessages testMessages(sizeof(us_tstmsgs)/sizeof(MsgDefn)) ;
  strcpy(testMessages.source_,"Test") ;
/*
  Load with messages. This involves creating successive messages
  (CoinOneMessage) and loading them into the array. This is the usual copy
  operation; client is responsible for disposing of the original message
  (accomplished here by keeping the CoinOneMessage internal to the loop
  body).
*/
  MsgDefn *msgDef = us_tstmsgs ;
  while (msgDef->internalNumber != COIN_TST_END)
  { CoinOneMessage msg(msgDef->externalNumber,msgDef->detail,msgDef->message) ;
    testMessages.addMessage(msgDef->internalNumber,msg) ;
    msgDef++ ; }
/*
  Run some tests on a message handler without messages.
*/
  testsWithoutMessages(errs) ;
/*
  Basic tests with messages. 
*/
  basicTestsWithMessages(testMessages,errs) ;
/*
  Advanced tests with messages. 
*/
  advTestsWithMessages(testMessages,errs) ;
/*
  Did we make it without error?
*/
  if (errs)
  { std::cout
      << "ERROR! CoinMessageHandlerTest reports "
      << errs << " errors." << std::endl ; }
  else
  { std::cout
      << "CoinMessageHandlerTest completed without error." << std::endl ; }

  return (errs == 0) ; }
