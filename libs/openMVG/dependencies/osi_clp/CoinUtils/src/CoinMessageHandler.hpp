/* $Id: CoinMessageHandler.hpp 1514 2011-12-10 23:35:23Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinMessageHandler_H
#define CoinMessageHandler_H

#include "CoinUtilsConfig.h"
#include "CoinPragma.hpp"

#include <iostream>
#include <cstdio>
#include <string>
#include <vector>

/** \file CoinMessageHandler.hpp
    \brief This is a first attempt at a message handler.

 The COIN Project is in favo(u)r of multi-language support. This implementation
 of a message handler tries to make it as lightweight as possible in the sense
 that only a subset of messages need to be defined --- the rest default to US
 English.

 The default handler at present just prints to stdout or to a FILE pointer

 \todo
 This needs to be worked over for correct operation with ISO character codes.
*/

/*
 I (jjf) am strongly in favo(u)r of language support for an open
 source project, but I have tried to make it as lightweight as
 possible in the sense that only a subset of messages need to be
 defined - the rest default to US English.  There will be different
 sets of messages for each component - so at present there is a
 Clp component and a Coin component.

 Because messages are only used in a controlled environment and have no
 impact on code and are tested by other tests I have included tests such
 as language and derivation in other unit tests.
*/
/*
  Where there are derived classes I (jjf) have started message numbers at 1001.
*/


/** \brief Class for one massaged message.

  A message consists of a text string with formatting codes (#message_),
  an integer identifier (#externalNumber_) which also determines the severity
  level (#severity_) of the message, and a detail (logging) level (#detail_).

  CoinOneMessage is just a container to hold this information. The
  interpretation is set by CoinMessageHandler, which see.
 */

class CoinOneMessage {

public:
  /**@name Constructors etc */
  //@{
  /** Default constructor. */
  CoinOneMessage();
  /** Normal constructor */
  CoinOneMessage(int externalNumber, char detail,
		const char * message);
  /** Destructor */
  ~CoinOneMessage();
  /** The copy constructor */
  CoinOneMessage(const CoinOneMessage&);
  /** assignment operator. */
  CoinOneMessage& operator=(const CoinOneMessage&);
  //@}

  /**@name Useful stuff */
  //@{
  /// Replace message text (<i>e.g.</i>, text in a different language)
  void replaceMessage(const char * message);
  //@}

  /**@name Get and set methods */
  //@{
  /** Get message ID number */
  inline int externalNumber() const
  {return externalNumber_;}
  /** \brief Set message ID number
  
    In the default CoinMessageHandler, this number is printed in the message
    prefix and is used to determine the message severity level.
  */
  inline void setExternalNumber(int number) 
  {externalNumber_=number;}
  /// Severity
  inline char severity() const
  {return severity_;}
  /// Set detail level
  inline void setDetail(int level)
  {detail_=static_cast<char> (level);}
  /// Get detail level
  inline int detail() const
  {return detail_;}
  /// Return the message text
  inline char * message() const 
  {return message_;}
  //@}

  /**@name member data */
  //@{
  /// number to print out (also determines severity)
    int externalNumber_;
  /// Will only print if detail matches
    char detail_;
  /// Severity
    char severity_;
  /// Messages (in correct language) (not all 400 may exist)
  mutable char message_[400];
  //@}
};

/** \brief Class to hold and manipulate an array of massaged messages.

  Note that the message index used to reference a message in the array of
  messages is completely distinct from the external ID number stored with the
  message.
*/

class CoinMessages {

public:
  /** \brief Supported languages
  
    These are the languages that are supported.  At present only
    us_en is serious and the rest are for testing.
  */
  enum Language {
    us_en = 0,
    uk_en,
    it
  };

  /**@name Constructors etc */
  //@{
  /** Constructor with number of messages. */
  CoinMessages(int numberMessages=0);
  /** Destructor */
  ~CoinMessages();
  /** The copy constructor */
  CoinMessages(const CoinMessages&);
  /** assignment operator. */
  CoinMessages& operator=(const CoinMessages&);
  //@}

  /**@name Useful stuff */
  //@{
  /*! \brief Installs a new message in the specified index position

    Any existing message is replaced, and a copy of the specified message is
    installed.
  */
  void addMessage(int messageNumber, const CoinOneMessage & message);
  /*! \brief Replaces the text of the specified message

    Any existing text is deleted and the specified text is copied into the
    specified message.
  */
  void replaceMessage(int messageNumber, const char * message);
  /** Language.  Need to think about iso codes */
  inline Language language() const
  {return language_;}
  /** Set language */
  void setLanguage(Language newlanguage)
  {language_ = newlanguage;}
  /// Change detail level for one message
  void setDetailMessage(int newLevel, int messageNumber);
  /** \brief Change detail level for several messages

    messageNumbers is expected to contain the indices of the messages to be
    changed.
    If numberMessages >= 10000 or messageNumbers is NULL, the detail level
    is changed on all messages.
  */
  void setDetailMessages(int newLevel, int numberMessages,
			 int * messageNumbers);
  /** Change detail level for all messages with low <= ID number < high */
  void setDetailMessages(int newLevel, int low, int high);

  /// Returns class
  inline int getClass() const
  { return class_;}
  /// Moves to compact format
  void toCompact();
  /// Moves from compact format
  void fromCompact();
  //@}

  /**@name member data */
  //@{
  /// Number of messages
  int numberMessages_;
  /// Language 
  Language language_;
  /// Source (null-terminated string, maximum 4 characters).
  char source_[5];
  /// Class - see later on before CoinMessageHandler
  int class_;
  /** Length of fake CoinOneMessage array.
      First you get numberMessages_ pointers which point to stuff
  */
  int lengthMessages_;
  /// Messages
  CoinOneMessage ** message_;
  //@}
};

// for convenience eol
enum CoinMessageMarker {
  CoinMessageEol = 0,
  CoinMessageNewline = 1
};

/** Base class for message handling

    The default behavior is described here: messages are printed, and (if the
    severity is sufficiently high) execution will be aborted. Inherit and
    redefine the methods #print and #checkSeverity to augment the behaviour.

    Messages can be printed with or without a prefix; the prefix will consist
    of a source string, the external ID number, and a letter code,
    <i>e.g.</i>, Clp6024W.
    A prefix makes the messages look less nimble but is very useful
    for "grep" <i>etc</i>.

    <h3> Usage </h3>

    The general approach to using the COIN messaging facility is as follows:
    <ul>
      <li> Define your messages. For each message, you must supply an external
	 ID number, a log (detail) level, and a format string. Typically, you
	 define a convenience structure for this, something that's easy to
	 use to create an array of initialised message definitions at compile
	 time.
      <li> Create a CoinMessages object, sized to accommodate the number of
        messages you've defined. (Incremental growth will happen if
	necessary as messages are loaded, but it's inefficient.)
      <li> Load the messages into the CoinMessages object. Typically this
	entails creating a CoinOneMessage object for each message and
	passing it as a parameter to CoinMessages::addMessage(). You specify
	the message's internal ID as the other parameter to addMessage.
      <li> Create and use a CoinMessageHandler object to print messages.
    </ul>
    See, for example, CoinMessage.hpp and CoinMessage.cpp for an example of
    the first three steps. `Format codes' below has a simple example of
    printing a message.

    <h3> External ID numbers and severity </h3>

    CoinMessageHandler assumes the following relationship between the
    external ID number of a message and the severity of the message:
    \li <3000 are informational ('I')
    \li <6000 warnings ('W')
    \li <9000 non-fatal errors ('E')
    \li >=9000 aborts the program (after printing the message) ('S')

    <h3> Log (detail) levels </h3>

    The default behaviour is that a message will print if its detail level
    is less than or equal to the handler's log level.  If all you want to
    do is set a single log level for the handler, use #setLogLevel(int).

    If you want to get fancy, here's how it really works: There's an array,
    #logLevels_, which you can manipulate with #setLogLevel(int,int). Each
    entry logLevels_[i] specifies the log level for messages of class i (see
    CoinMessages::class_). If logLevels_[0] is set to the magic number -1000
    you get the simple behaviour described above, whatever the class of the
    messages. If logLevels_[0] is set to a valid log level (>= 0), then
    logLevels_[i] really is the log level for messages of class i.

    <h3> Format codes </h3>

    CoinMessageHandler can print integers (normal, long, and long long),
    doubles, characters, and strings. See the descriptions of the
    various << operators.
    
    When processing a standard message with a format string, the formatting
    codes specified in the format string will be passed to the sprintf
    function, along with the argument. When generating a message with no
    format string, each << operator uses a simple format code appropriate for
    its argument.  Consult the documentation for the standard printf facility
    for further information on format codes.

    The special format code `%?' provides a hook to enable or disable
    printing.  For each `%?' code, there must be a corresponding call to
    printing(bool).  This provides a way to define optional parts in
    messages, delineated by the code `%?' in the format string.  Printing can
    be suppressed for these optional parts, but any operands must still be
    supplied. For example, given the message string
    \verbatim
    "A message with%? an optional integer %d and%? a double %g."
    \endverbatim
    installed in CoinMessages \c exampleMsgs with index 5, and
    \c CoinMessageHandler \c hdl, the code
    \code
    hdl.message(5,exampleMsgs) ;
    hdl.printing(true) << 42 ;
    hdl.printing(true) << 53.5 << CoinMessageEol ;
    \endcode
    will print
    \verbatim
    A message with an optional integer 42 and a double 53.5.
    \endverbatim
    while
    \code
    hdl.message(5,exampleMsgs) ;
    hdl.printing(false) << 42 ;
    hdl.printing(true) << 53.5 << CoinMessageEol ;
    \endcode
    will print
    \verbatim
    A message with a double 53.5.
    \endverbatim

    For additional examples of usage, see CoinMessageHandlerUnitTest in
    CoinMessageHandlerTest.cpp.
*/

class CoinMessageHandler  {

friend bool CoinMessageHandlerUnitTest () ;

public:
   /**@name Virtual methods that the derived classes may provide */
   //@{
  /** Print message, return 0 normally.
  */
  virtual int print() ;
  /** Check message severity - if too bad then abort
  */
  virtual void checkSeverity() ;
   //@}

  /**@name Constructors etc */
  //@{
  /// Constructor
  CoinMessageHandler();
  /// Constructor to put to file pointer (won't be closed)
  CoinMessageHandler(FILE *fp);
  /** Destructor */
  virtual ~CoinMessageHandler();
  /** The copy constructor */
  CoinMessageHandler(const CoinMessageHandler&);
  /** Assignment operator. */
  CoinMessageHandler& operator=(const CoinMessageHandler&);
  /// Clone
  virtual CoinMessageHandler * clone() const;
  //@}
   /**@name Get and set methods */
   //@{
  /// Get detail level of a message.
  inline int detail(int messageNumber, const CoinMessages &normalMessage) const
  { return normalMessage.message_[messageNumber]->detail();}
  /** Get current log (detail) level. */
  inline int logLevel() const
          { return logLevel_;}
  /** \brief Set current log (detail) level.

    If the log level is equal or greater than the detail level of a message,
    the message will be printed. A rough convention for the amount of output
    expected is
    - 0 - none
    - 1 - minimal
    - 2 - normal low
    - 3 - normal high
    - 4 - verbose

    Please assign log levels to messages accordingly. Log levels of 8 and
    above (8,16,32, <i>etc</i>.) are intended for selective debugging.
    The logical AND of the log level specified in the message and the current
    log level is used to determine if the message is printed. (In other words,
    you're using individual bits to determine which messages are printed.)
  */
  void setLogLevel(int value);
  /** Get alternative log level. */
  inline int logLevel(int which) const
  { return logLevels_[which];}
  /*! \brief Set alternative log level value.

    Can be used to store alternative log level information within the handler.
  */
  void setLogLevel(int which, int value);

  /// Set the number of significant digits for printing floating point numbers
  void setPrecision(unsigned int new_precision);
  /// Current number of significant digits for printing floating point numbers
  inline int precision() { return (g_precision_) ; }

  /// Switch message prefix on or off.
  void setPrefix(bool yesNo);
  /// Current setting for printing message prefix.
  bool  prefix() const;
  /*! \brief Values of double fields already processed.

    As the parameter for a double field is processed, the value is saved
    and can be retrieved using this function.
  */
  inline double doubleValue(int position) const
  { return doubleValue_[position];}
  /*! \brief Number of double fields already processed.

    Incremented each time a field of type double is processed.
  */
  inline int numberDoubleFields() const
  {return static_cast<int>(doubleValue_.size());}
  /*! \brief Values of integer fields already processed.

    As the parameter for a integer field is processed, the value is saved
    and can be retrieved using this function.
  */
  inline int intValue(int position) const
  { return longValue_[position];}
  /*! \brief Number of integer fields already processed.

    Incremented each time a field of type integer is processed.
  */
  inline int numberIntFields() const
  {return static_cast<int>(longValue_.size());}
  /*! \brief Values of char fields already processed.

    As the parameter for a char field is processed, the value is saved
    and can be retrieved using this function.
  */
  inline char charValue(int position) const
  { return charValue_[position];}
  /*! \brief Number of char fields already processed.

    Incremented each time a field of type char is processed.
  */
  inline int numberCharFields() const
  {return static_cast<int>(charValue_.size());}
  /*! \brief Values of string fields already processed.

    As the parameter for a string field is processed, the value is saved
    and can be retrieved using this function.
  */
  inline std::string stringValue(int position) const
  { return stringValue_[position];}
  /*! \brief Number of string fields already processed.

    Incremented each time a field of type string is processed.
  */
  inline int numberStringFields() const
  {return static_cast<int>(stringValue_.size());}

  /// Current message
  inline CoinOneMessage  currentMessage() const
  {return currentMessage_;}
  /// Source of current message
  inline std::string currentSource() const
  {return source_;}
  /// Output buffer
  inline const char * messageBuffer() const
  {return messageBuffer_;}
  /// Highest message number (indicates any errors)
  inline int highestNumber() const
  {return highestNumber_;}
  /// Get current file pointer
  inline FILE * filePointer() const
  { return fp_;}
  /// Set new file pointer
  inline void setFilePointer(FILE * fp)
  { fp_ = fp;}
  //@}
  
  /**@name Actions to create a message  */
  //@{
  /*! \brief Start a message

    Look up the specified message. A prefix will be generated if enabled.
    The message will be printed if the current log level is equal or greater
    than the log level of the message.
  */
  CoinMessageHandler &message(int messageNumber,
			      const CoinMessages &messages) ;

  /*! \brief Start or continue a message

    With detail = -1 (default), does nothing except return a reference to the
    handler. (I.e., msghandler.message() << "foo" is precisely equivalent
    to msghandler << "foo".) If \p msgDetail is >= 0, is will be used
    as the detail level to determine whether the message should print
    (assuming class 0).

    This can be used with any of the << operators. One use is to start
    a message which will be constructed entirely from scratch. Another
    use is continuation of a message after code that interrupts the usual
    sequence of << operators.
  */
  CoinMessageHandler & message(int detail = -1) ;

  /*! \brief Print a complete message
  
    Generate a standard prefix and append \c msg `as is'. This is intended as
    a transition mechanism.  The standard prefix is generated (if enabled),
    and \c msg is appended. The message must be ended with a CoinMessageEol
    marker. Attempts to add content with << will have no effect.

    The default value of \p detail will not change printing status. If
    \p detail is >= 0, it will be used as the detail level to determine
    whether the message should print (assuming class 0).

  */
  CoinMessageHandler &message(int externalNumber, const char *source,
			      const char *msg,
			      char severity, int detail = -1) ;

  /*! \brief Process an integer parameter value.

    The default format code is `%d'.
  */
  CoinMessageHandler & operator<< (int intvalue);
#if COIN_BIG_INDEX==1
  /*! \brief Process a long integer parameter value.

    The default format code is `%ld'.
  */
  CoinMessageHandler & operator<< (long longvalue);
#endif
#if COIN_BIG_INDEX==2
  /*! \brief Process a long long integer parameter value.

    The default format code is `%ld'.
  */
  CoinMessageHandler & operator<< (long long longvalue);
#endif
  /*! \brief Process a double parameter value.

    The default format code is `%d'.
  */
  CoinMessageHandler & operator<< (double doublevalue);
  /*! \brief Process a STL string parameter value.

    The default format code is `%g'.
  */
  CoinMessageHandler & operator<< (const std::string& stringvalue);
  /*! \brief Process a char parameter value.

    The default format code is `%s'.
  */
  CoinMessageHandler & operator<< (char charvalue);
  /*! \brief Process a C-style string parameter value.

    The default format code is `%c'.
  */
  CoinMessageHandler & operator<< (const char *stringvalue);
  /*! \brief Process a marker.

    The default format code is `%s'.
  */
  CoinMessageHandler & operator<< (CoinMessageMarker);
  /** Finish (and print) the message.
  
    Equivalent to using the CoinMessageEol marker.
  */
  int finish();
  /*! \brief Enable or disable printing of an optional portion of a message.

    Optional portions of a message are delimited by `%?' markers, and
    printing processes one %? marker. If \c onOff is true, the subsequent
    portion of the message (to the next %? marker or the end of the format
    string) will be printed. If \c onOff is false, printing is suppressed.
    Parameters must still be supplied, whether printing is suppressed or not.
    See the class documentation for an example.
  */
  CoinMessageHandler & printing(bool onOff);

  //@}

  /** Log levels will be by type and will then use type
      given in CoinMessage::class_

    - 0 - Branch and bound code or similar
    - 1 - Solver
    - 2 - Stuff in Coin directory
    - 3 - Cut generators
  */
#define COIN_NUM_LOG 4
/// Maximum length of constructed message (characters)
#define COIN_MESSAGE_HANDLER_MAX_BUFFER_SIZE 1000
protected:
  /**@name Protected member data */
  //@{
  /// values in message
  std::vector<double> doubleValue_;
  std::vector<int> longValue_;
  std::vector<char> charValue_;
  std::vector<std::string> stringValue_;
  /// Log level
  int logLevel_;
  /// Log levels
  int logLevels_[COIN_NUM_LOG];
  /// Whether we want prefix (may get more subtle so is int)
  int prefix_;
  /// Current message
  CoinOneMessage  currentMessage_;
  /// Internal number for use with enums
  int internalNumber_;
  /// Format string for message (remainder)
  char * format_;
  /// Output buffer
  char messageBuffer_[COIN_MESSAGE_HANDLER_MAX_BUFFER_SIZE];
  /// Position in output buffer
  char * messageOut_;
  /// Current source of message
  std::string source_;
  /** 0 - Normal.
      1 - Put in values, move along format, but don't print.
      2 - A complete message was provided; nothing more to do but print
          when CoinMessageEol is processed. Any << operators are treated
	  as noops.
      3 - do nothing except look for CoinMessageEol (i.e., the message
          detail level was not sufficient to cause it to print).
  */
  int printStatus_;
  /// Highest message number (indicates any errors)
  int highestNumber_;
  /// File pointer
  FILE * fp_;
  /// Current format for floating point numbers
  char g_format_[8];
  /// Current number of significant digits for floating point numbers
  int g_precision_ ;
   //@}

private:

  /** The body of the copy constructor and the assignment operator */
  void gutsOfCopy(const CoinMessageHandler &rhs) ;

  /*! \brief Internal function to locate next format code.

    Intended for internal use. Side effects modify the format string.
  */
  char *nextPerCent(char *start, const bool initial = false) ;

  /*! \brief Internal printing function.
  
    Makes it easier to split up print into clean, print and check severity
  */
  int internalPrint() ;

  /// Decide if this message should print.
  void calcPrintStatus(int msglvl, int msgclass) ;
    

};

//#############################################################################
/** A function that tests the methods in the CoinMessageHandler class. The
    only reason for it not to be a member method is that this way it doesn't
    have to be compiled into the library. And that's a gain, because the
    library should be compiled with optimization on, but this method should be
    compiled with debugging. */
bool
CoinMessageHandlerUnitTest();

#endif
