/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"
#include "CoinTime.hpp"
#include "CbcOrClpParam.hpp"

#include <string>
#include <iostream>
#include <cassert>
#ifdef COIN_HAS_CBC
#ifdef COIN_HAS_CLP
#include "OsiClpSolverInterface.hpp"
#include "ClpSimplex.hpp"
#endif
#include "CbcModel.hpp"
#endif
#include "CoinHelperFunctions.hpp"
#ifdef COIN_HAS_CLP
#include "ClpSimplex.hpp"
#include "ClpFactorization.hpp"
#endif
#if COIN_INT_MAX==0
#undef COIN_INT_MAX
#define COIN_INT_MAX 2147483647
#endif
#ifdef COIN_HAS_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif
#ifdef COIN_HAS_CBC
// from CoinSolve
static char coin_prompt[] = "Coin:";
#else
static char coin_prompt[] = "Clp:";
#endif
#ifdef CLP_CILK
#ifndef CBC_THREAD
#define CBC_THREAD
#endif
#endif
#if defined(COIN_HAS_WSMP) && ! defined(USE_EKKWSSMP)
#ifndef CBC_THREAD
#define CBC_THREAD
#endif
#endif
#include "ClpConfig.h"
#ifdef CLP_HAS_ABC
#include "AbcCommon.hpp"
#endif
static bool doPrinting = true;
std::string afterEquals = "";
static char printArray[200];
void setCbcOrClpPrinting(bool yesNo)
{
     doPrinting = yesNo;
}
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
CbcOrClpParam::CbcOrClpParam ()
     : type_(CBC_PARAM_NOTUSED_INVALID),
       lowerDoubleValue_(0.0),
       upperDoubleValue_(0.0),
       lowerIntValue_(0),
       upperIntValue_(0),
       lengthName_(0),
       lengthMatch_(0),
       definedKeyWords_(),
       name_(),
       shortHelp_(),
       longHelp_(),
       action_(CBC_PARAM_NOTUSED_INVALID),
       currentKeyWord_(-1),
       display_(0),
       intValue_(-1),
       doubleValue_(-1.0),
       stringValue_(""),
       whereUsed_(7)
{
}
// Other constructors
CbcOrClpParam::CbcOrClpParam (std::string name, std::string help,
                              double lower, double upper, CbcOrClpParameterType type,
                              int display)
     : type_(type),
       lowerIntValue_(0),
       upperIntValue_(0),
       definedKeyWords_(),
       name_(name),
       shortHelp_(help),
       longHelp_(),
       action_(type),
       currentKeyWord_(-1),
       display_(display),
       intValue_(-1),
       doubleValue_(-1.0),
       stringValue_(""),
       whereUsed_(7)
{
     lowerDoubleValue_ = lower;
     upperDoubleValue_ = upper;
     gutsOfConstructor();
}
CbcOrClpParam::CbcOrClpParam (std::string name, std::string help,
                              int lower, int upper, CbcOrClpParameterType type,
                              int display)
     : type_(type),
       lowerDoubleValue_(0.0),
       upperDoubleValue_(0.0),
       definedKeyWords_(),
       name_(name),
       shortHelp_(help),
       longHelp_(),
       action_(type),
       currentKeyWord_(-1),
       display_(display),
       intValue_(-1),
       doubleValue_(-1.0),
       stringValue_(""),
       whereUsed_(7)
{
     gutsOfConstructor();
     lowerIntValue_ = lower;
     upperIntValue_ = upper;
}
// Other strings will be added by append
CbcOrClpParam::CbcOrClpParam (std::string name, std::string help,
                              std::string firstValue,
                              CbcOrClpParameterType type, int whereUsed,
                              int display)
     : type_(type),
       lowerDoubleValue_(0.0),
       upperDoubleValue_(0.0),
       lowerIntValue_(0),
       upperIntValue_(0),
       definedKeyWords_(),
       name_(name),
       shortHelp_(help),
       longHelp_(),
       action_(type),
       currentKeyWord_(0),
       display_(display),
       intValue_(-1),
       doubleValue_(-1.0),
       stringValue_(""),
       whereUsed_(whereUsed)
{
     gutsOfConstructor();
     definedKeyWords_.push_back(firstValue);
}
// Action
CbcOrClpParam::CbcOrClpParam (std::string name, std::string help,
                              CbcOrClpParameterType type, int whereUsed,
                              int display)
     : type_(type),
       lowerDoubleValue_(0.0),
       upperDoubleValue_(0.0),
       lowerIntValue_(0),
       upperIntValue_(0),
       definedKeyWords_(),
       name_(name),
       shortHelp_(help),
       longHelp_(),
       action_(type),
       currentKeyWord_(-1),
       display_(display),
       intValue_(-1),
       doubleValue_(-1.0),
       stringValue_("")
{
     whereUsed_ = whereUsed;
     gutsOfConstructor();
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
CbcOrClpParam::CbcOrClpParam (const CbcOrClpParam & rhs)
{
     type_ = rhs.type_;
     lowerDoubleValue_ = rhs.lowerDoubleValue_;
     upperDoubleValue_ = rhs.upperDoubleValue_;
     lowerIntValue_ = rhs.lowerIntValue_;
     upperIntValue_ = rhs.upperIntValue_;
     lengthName_ = rhs.lengthName_;
     lengthMatch_ = rhs.lengthMatch_;
     definedKeyWords_ = rhs.definedKeyWords_;
     name_ = rhs.name_;
     shortHelp_ = rhs.shortHelp_;
     longHelp_ = rhs.longHelp_;
     action_ = rhs.action_;
     currentKeyWord_ = rhs.currentKeyWord_;
     display_ = rhs.display_;
     intValue_ = rhs.intValue_;
     doubleValue_ = rhs.doubleValue_;
     stringValue_ = rhs.stringValue_;
     whereUsed_ = rhs.whereUsed_;
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
CbcOrClpParam::~CbcOrClpParam ()
{
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
CbcOrClpParam &
CbcOrClpParam::operator=(const CbcOrClpParam & rhs)
{
     if (this != &rhs) {
          type_ = rhs.type_;
          lowerDoubleValue_ = rhs.lowerDoubleValue_;
          upperDoubleValue_ = rhs.upperDoubleValue_;
          lowerIntValue_ = rhs.lowerIntValue_;
          upperIntValue_ = rhs.upperIntValue_;
          lengthName_ = rhs.lengthName_;
          lengthMatch_ = rhs.lengthMatch_;
          definedKeyWords_ = rhs.definedKeyWords_;
          name_ = rhs.name_;
          shortHelp_ = rhs.shortHelp_;
          longHelp_ = rhs.longHelp_;
          action_ = rhs.action_;
          currentKeyWord_ = rhs.currentKeyWord_;
          display_ = rhs.display_;
          intValue_ = rhs.intValue_;
          doubleValue_ = rhs.doubleValue_;
          stringValue_ = rhs.stringValue_;
          whereUsed_ = rhs.whereUsed_;
     }
     return *this;
}
void
CbcOrClpParam::gutsOfConstructor()
{
     std::string::size_type  shriekPos = name_.find('!');
     lengthName_ = static_cast<unsigned int>(name_.length());
     if ( shriekPos == std::string::npos ) {
          //does not contain '!'
          lengthMatch_ = lengthName_;
     } else {
          lengthMatch_ = static_cast<unsigned int>(shriekPos);
          name_ = name_.substr(0, shriekPos) + name_.substr(shriekPos + 1);
          lengthName_--;
     }
}
// Returns length of name for printing
int
CbcOrClpParam::lengthMatchName (  ) const
{
     if (lengthName_ == lengthMatch_)
          return lengthName_;
     else
          return lengthName_ + 2;
}
// Insert string (only valid for keywords)
void
CbcOrClpParam::append(std::string keyWord)
{
     definedKeyWords_.push_back(keyWord);
}

int
CbcOrClpParam::matches (std::string input) const
{
     // look up strings to do more elegantly
     if (input.length() > lengthName_) {
          return 0;
     } else {
          unsigned int i;
          for (i = 0; i < input.length(); i++) {
               if (tolower(name_[i]) != tolower(input[i]))
                    break;
          }
          if (i < input.length()) {
               return 0;
          } else if (i >= lengthMatch_) {
               return 1;
          } else {
               // matched but too short
               return 2;
          }
     }
}
// Returns name which could match
std::string
CbcOrClpParam::matchName (  ) const
{
     if (lengthMatch_ == lengthName_)
          return name_;
     else
          return name_.substr(0, lengthMatch_) + "(" + name_.substr(lengthMatch_) + ")";
}

// Returns parameter option which matches (-1 if none)
int
CbcOrClpParam::parameterOption ( std::string check ) const
{
     int numberItems = static_cast<int>(definedKeyWords_.size());
     if (!numberItems) {
          return -1;
     } else {
          int whichItem = 0;
          unsigned int it;
          for (it = 0; it < definedKeyWords_.size(); it++) {
               std::string thisOne = definedKeyWords_[it];
               std::string::size_type  shriekPos = thisOne.find('!');
               size_t length1 = thisOne.length();
               size_t length2 = length1;
               if ( shriekPos != std::string::npos ) {
                    //contains '!'
                    length2 = shriekPos;
                    thisOne = thisOne.substr(0, shriekPos) +
                              thisOne.substr(shriekPos + 1);
                    length1 = thisOne.length();
               }
               if (check.length() <= length1 && length2 <= check.length()) {
                    unsigned int i;
                    for (i = 0; i < check.length(); i++) {
                         if (tolower(thisOne[i]) != tolower(check[i]))
                              break;
                    }
                    if (i < check.length()) {
                         whichItem++;
                    } else if (i >= length2) {
                         break;
                    }
               } else {
                    whichItem++;
               }
          }
          if (whichItem < numberItems)
               return whichItem;
          else
               return -1;
     }
}
// Prints parameter options
void
CbcOrClpParam::printOptions (  ) const
{
     std::cout << "<Possible options for " << name_ << " are:";
     unsigned int it;
     for (it = 0; it < definedKeyWords_.size(); it++) {
          std::string thisOne = definedKeyWords_[it];
          std::string::size_type  shriekPos = thisOne.find('!');
          if ( shriekPos != std::string::npos ) {
               //contains '!'
               thisOne = thisOne.substr(0, shriekPos) +
                         "(" + thisOne.substr(shriekPos + 1) + ")";
          }
          std::cout << " " << thisOne;
     }
     assert (currentKeyWord_ >= 0 && currentKeyWord_ < static_cast<int>(definedKeyWords_.size()));
     std::string current = definedKeyWords_[currentKeyWord_];
     std::string::size_type  shriekPos = current.find('!');
     if ( shriekPos != std::string::npos ) {
          //contains '!'
          current = current.substr(0, shriekPos) +
                    "(" + current.substr(shriekPos + 1) + ")";
     }
     std::cout << ";\n\tcurrent  " << current << ">" << std::endl;
}
// Print action and string
void
CbcOrClpParam::printString() const
{
     if (name_ == "directory")
          std::cout << "Current working directory is " << stringValue_ << std::endl;
     else if (name_.substr(0, 6) == "printM")
          std::cout << "Current value of printMask is " << stringValue_ << std::endl;
     else
          std::cout << "Current default (if $ as parameter) for " << name_
                    << " is " << stringValue_ << std::endl;
}
void CoinReadPrintit(const char * input)
{
     int length = static_cast<int>(strlen(input));
     char temp[101];
     int i;
     int n = 0;
     for (i = 0; i < length; i++) {
          if (input[i] == '\n') {
               temp[n] = '\0';
               std::cout << temp << std::endl;
               n = 0;
          } else if (n >= 65 && input[i] == ' ') {
               temp[n] = '\0';
               std::cout << temp << std::endl;
               n = 0;
          } else if (n || input[i] != ' ') {
               temp[n++] = input[i];
          }
     }
     if (n) {
          temp[n] = '\0';
          std::cout << temp << std::endl;
     }
}
// Print Long help
void
CbcOrClpParam::printLongHelp() const
{
     if (type_ >= 1 && type_ < 400) {
          CoinReadPrintit(longHelp_.c_str());
          if (type_ < CLP_PARAM_INT_SOLVERLOGLEVEL) {
               printf("<Range of values is %g to %g;\n\tcurrent %g>\n", lowerDoubleValue_, upperDoubleValue_, doubleValue_);
               assert (upperDoubleValue_ > lowerDoubleValue_);
          } else if (type_ < CLP_PARAM_STR_DIRECTION) {
               printf("<Range of values is %d to %d;\n\tcurrent %d>\n", lowerIntValue_, upperIntValue_, intValue_);
               assert (upperIntValue_ > lowerIntValue_);
          } else if (type_ < CLP_PARAM_ACTION_DIRECTORY) {
               printOptions();
          }
     }
}
#ifdef COIN_HAS_CBC
int
CbcOrClpParam::setDoubleParameter (OsiSolverInterface * model, double value)
{
     int returnCode;
     setDoubleParameterWithMessage(model, value, returnCode);
     if (doPrinting && strlen(printArray))
          std::cout << printArray << std::endl;
     return returnCode;
}
// Sets double parameter and returns printable string and error code
const char *
CbcOrClpParam::setDoubleParameterWithMessage ( OsiSolverInterface * model, double  value , int & returnCode)
{
     if (value < lowerDoubleValue_ || value > upperDoubleValue_) {
          sprintf(printArray, "%g was provided for %s - valid range is %g to %g",
                  value, name_.c_str(), lowerDoubleValue_, upperDoubleValue_);
          std::cout << value << " was provided for " << name_ <<
                    " - valid range is " << lowerDoubleValue_ << " to " <<
                    upperDoubleValue_ << std::endl;
          returnCode = 1;
     } else {
          double oldValue = doubleValue_;
          doubleValue_ = value;
          switch (type_) {
          case CLP_PARAM_DBL_DUALTOLERANCE:
               model->getDblParam(OsiDualTolerance, oldValue);
               model->setDblParam(OsiDualTolerance, value);
               break;
          case CLP_PARAM_DBL_PRIMALTOLERANCE:
               model->getDblParam(OsiPrimalTolerance, oldValue);
               model->setDblParam(OsiPrimalTolerance, value);
               break;
          default:
               break;
          }
          sprintf(printArray, "%s was changed from %g to %g",
                  name_.c_str(), oldValue, value);
          returnCode = 0;
     }
     return printArray;
}
#endif
#ifdef COIN_HAS_CLP
int
CbcOrClpParam::setDoubleParameter (ClpSimplex * model, double value)
{
     int returnCode;
     setDoubleParameterWithMessage(model, value, returnCode);
     if (doPrinting && strlen(printArray))
          std::cout << printArray << std::endl;
     return returnCode;
}
// Sets int parameter and returns printable string and error code
const char *
CbcOrClpParam::setDoubleParameterWithMessage ( ClpSimplex * model, double value , int & returnCode)
{
     double oldValue = doubleValue_;
     if (value < lowerDoubleValue_ || value > upperDoubleValue_) {
          sprintf(printArray, "%g was provided for %s - valid range is %g to %g",
                  value, name_.c_str(), lowerDoubleValue_, upperDoubleValue_);
          returnCode = 1;
     } else {
          sprintf(printArray, "%s was changed from %g to %g",
                  name_.c_str(), oldValue, value);
          returnCode = 0;
          doubleValue_ = value;
          switch (type_) {
          case CLP_PARAM_DBL_DUALTOLERANCE:
               model->setDualTolerance(value);
               break;
          case CLP_PARAM_DBL_PRIMALTOLERANCE:
               model->setPrimalTolerance(value);
               break;
          case CLP_PARAM_DBL_ZEROTOLERANCE:
               model->setSmallElementValue(value);
               break;
          case CLP_PARAM_DBL_DUALBOUND:
               model->setDualBound(value);
               break;
          case CLP_PARAM_DBL_PRIMALWEIGHT:
               model->setInfeasibilityCost(value);
               break;
#ifndef COIN_HAS_CBC
          case CLP_PARAM_DBL_TIMELIMIT:
               model->setMaximumSeconds(value);
               break;
#endif
          case CLP_PARAM_DBL_OBJSCALE:
               model->setObjectiveScale(value);
               break;
          case CLP_PARAM_DBL_RHSSCALE:
               model->setRhsScale(value);
               break;
          case CLP_PARAM_DBL_PRESOLVETOLERANCE:
               model->setDblParam(ClpPresolveTolerance, value);
               break;
          default:
               break;
          }
     }
     return printArray;
}
double
CbcOrClpParam::doubleParameter (ClpSimplex * model) const
{
     double value;
     switch (type_) {
#ifndef COIN_HAS_CBC
     case CLP_PARAM_DBL_DUALTOLERANCE:
          value = model->dualTolerance();
          break;
     case CLP_PARAM_DBL_PRIMALTOLERANCE:
          value = model->primalTolerance();
          break;
#endif
     case CLP_PARAM_DBL_ZEROTOLERANCE:
          value = model->getSmallElementValue();
          break;
     case CLP_PARAM_DBL_DUALBOUND:
          value = model->dualBound();
          break;
     case CLP_PARAM_DBL_PRIMALWEIGHT:
          value = model->infeasibilityCost();
          break;
#ifndef COIN_HAS_CBC
     case CLP_PARAM_DBL_TIMELIMIT:
          value = model->maximumSeconds();
          break;
#endif
     case CLP_PARAM_DBL_OBJSCALE:
          value = model->objectiveScale();
          break;
     case CLP_PARAM_DBL_RHSSCALE:
          value = model->rhsScale();
          break;
     default:
          value = doubleValue_;
          break;
     }
     return value;
}
int
CbcOrClpParam::setIntParameter (ClpSimplex * model, int value)
{
     int returnCode;
     setIntParameterWithMessage(model, value, returnCode);
     if (doPrinting && strlen(printArray))
          std::cout << printArray << std::endl;
     return returnCode;
}
// Sets int parameter and returns printable string and error code
const char *
CbcOrClpParam::setIntParameterWithMessage ( ClpSimplex * model, int value , int & returnCode)
{
     int oldValue = intValue_;
     if (value < lowerIntValue_ || value > upperIntValue_) {
          sprintf(printArray, "%d was provided for %s - valid range is %d to %d",
                  value, name_.c_str(), lowerIntValue_, upperIntValue_);
          returnCode = 1;
     } else {
          intValue_ = value;
          sprintf(printArray, "%s was changed from %d to %d",
                  name_.c_str(), oldValue, value);
          returnCode = 0;
          switch (type_) {
          case CLP_PARAM_INT_SOLVERLOGLEVEL:
               model->setLogLevel(value);
               if (value > 2)
                    model->factorization()->messageLevel(8);
               else
                    model->factorization()->messageLevel(0);
               break;
          case CLP_PARAM_INT_MAXFACTOR:
               model->factorization()->maximumPivots(value);
               break;
          case CLP_PARAM_INT_PERTVALUE:
               model->setPerturbation(value);
               break;
          case CLP_PARAM_INT_MAXITERATION:
               model->setMaximumIterations(value);
               break;
          case CLP_PARAM_INT_SPECIALOPTIONS:
               model->setSpecialOptions(value);
               break;
          case CLP_PARAM_INT_RANDOMSEED:
	    {
	      if (value==0) {
		double time = fabs(CoinGetTimeOfDay());
		while (time>=COIN_INT_MAX)
		  time *= 0.5;
		value = static_cast<int>(time);
		sprintf(printArray, "using time of day %s was changed from %d to %d",
                  name_.c_str(), oldValue, value);
	      }
	      model->setRandomSeed(value);
	    }
               break;
          case CLP_PARAM_INT_MORESPECIALOPTIONS:
               model->setMoreSpecialOptions(value);
	       break;
#ifndef COIN_HAS_CBC
#ifdef CBC_THREAD
          case CBC_PARAM_INT_THREADS:
               model->setNumberThreads(value);
               break;
#endif
#endif
          default:
               break;
          }
     }
     return printArray;
}
int
CbcOrClpParam::intParameter (ClpSimplex * model) const
{
     int value;
     switch (type_) {
#ifndef COIN_HAS_CBC
     case CLP_PARAM_INT_SOLVERLOGLEVEL:
          value = model->logLevel();
          break;
#endif
     case CLP_PARAM_INT_MAXFACTOR:
          value = model->factorization()->maximumPivots();
          break;
          break;
     case CLP_PARAM_INT_PERTVALUE:
          value = model->perturbation();
          break;
     case CLP_PARAM_INT_MAXITERATION:
          value = model->maximumIterations();
          break;
     case CLP_PARAM_INT_SPECIALOPTIONS:
          value = model->specialOptions();
          break;
     case CLP_PARAM_INT_RANDOMSEED:
          value = model->randomNumberGenerator()->getSeed();
          break;
     case CLP_PARAM_INT_MORESPECIALOPTIONS:
          value = model->moreSpecialOptions();
          break;
#ifndef COIN_HAS_CBC
#ifdef CBC_THREAD
     case CBC_PARAM_INT_THREADS:
          value = model->numberThreads();
	  break;
#endif
#endif
     default:
          value = intValue_;
          break;
     }
     return value;
}
#endif
int
CbcOrClpParam::checkDoubleParameter (double value) const
{
     if (value < lowerDoubleValue_ || value > upperDoubleValue_) {
          std::cout << value << " was provided for " << name_ <<
                    " - valid range is " << lowerDoubleValue_ << " to " <<
                    upperDoubleValue_ << std::endl;
          return 1;
     } else {
          return 0;
     }
}
#ifdef COIN_HAS_CBC
double
CbcOrClpParam::doubleParameter (OsiSolverInterface *
#ifndef NDEBUG
                                model
#endif
                               ) const
{
     double value = 0.0;
     switch (type_) {
     case CLP_PARAM_DBL_DUALTOLERANCE:
          assert(model->getDblParam(OsiDualTolerance, value));
          break;
     case CLP_PARAM_DBL_PRIMALTOLERANCE:
          assert(model->getDblParam(OsiPrimalTolerance, value));
          break;
     default:
          return doubleValue_;
          break;
     }
     return value;
}
int
CbcOrClpParam::setIntParameter (OsiSolverInterface * model, int value)
{
     int returnCode;
     setIntParameterWithMessage(model, value, returnCode);
     if (doPrinting && strlen(printArray))
          std::cout << printArray << std::endl;
     return returnCode;
}
// Sets int parameter and returns printable string and error code
const char *
CbcOrClpParam::setIntParameterWithMessage ( OsiSolverInterface * model, int  value , int & returnCode)
{
     if (value < lowerIntValue_ || value > upperIntValue_) {
          sprintf(printArray, "%d was provided for %s - valid range is %d to %d",
                  value, name_.c_str(), lowerIntValue_, upperIntValue_);
          returnCode = 1;
     } else {
          int oldValue = intValue_;
          intValue_ = oldValue;
          switch (type_) {
          case CLP_PARAM_INT_SOLVERLOGLEVEL:
               model->messageHandler()->setLogLevel(value);
               break;
          default:
               break;
          }
          sprintf(printArray, "%s was changed from %d to %d",
                  name_.c_str(), oldValue, value);
          returnCode = 0;
     }
     return printArray;
}
int
CbcOrClpParam::intParameter (OsiSolverInterface * model) const
{
     int value = 0;
     switch (type_) {
     case CLP_PARAM_INT_SOLVERLOGLEVEL:
          value = model->messageHandler()->logLevel();
          break;
     default:
          value = intValue_;
          break;
     }
     return value;
}
int
CbcOrClpParam::setDoubleParameter (CbcModel &model, double value)
{
     int returnCode;
     setDoubleParameterWithMessage(model, value, returnCode);
     if (doPrinting && strlen(printArray))
          std::cout << printArray << std::endl;
     return returnCode;
}
// Sets double parameter and returns printable string and error code
const char *
CbcOrClpParam::setDoubleParameterWithMessage ( CbcModel & model, double  value , int & returnCode)
{
     if (value < lowerDoubleValue_ || value > upperDoubleValue_) {
          sprintf(printArray, "%g was provided for %s - valid range is %g to %g",
                  value, name_.c_str(), lowerDoubleValue_, upperDoubleValue_);
          returnCode = 1;
     } else {
          double oldValue = doubleValue_;
          doubleValue_ = value;
          switch (type_) {
          case CBC_PARAM_DBL_INFEASIBILITYWEIGHT:
               oldValue = model.getDblParam(CbcModel::CbcInfeasibilityWeight);
               model.setDblParam(CbcModel::CbcInfeasibilityWeight, value);
               break;
          case CBC_PARAM_DBL_INTEGERTOLERANCE:
               oldValue = model.getDblParam(CbcModel::CbcIntegerTolerance);
               model.setDblParam(CbcModel::CbcIntegerTolerance, value);
               break;
          case CBC_PARAM_DBL_INCREMENT:
               oldValue = model.getDblParam(CbcModel::CbcCutoffIncrement);
               model.setDblParam(CbcModel::CbcCutoffIncrement, value);
          case CBC_PARAM_DBL_ALLOWABLEGAP:
               oldValue = model.getDblParam(CbcModel::CbcAllowableGap);
               model.setDblParam(CbcModel::CbcAllowableGap, value);
               break;
          case CBC_PARAM_DBL_GAPRATIO:
               oldValue = model.getDblParam(CbcModel::CbcAllowableFractionGap);
               model.setDblParam(CbcModel::CbcAllowableFractionGap, value);
               break;
          case CBC_PARAM_DBL_CUTOFF:
               oldValue = model.getCutoff();
               model.setCutoff(value);
               break;
          case CBC_PARAM_DBL_TIMELIMIT_BAB:
               oldValue = model.getDblParam(CbcModel::CbcMaximumSeconds) ;
               {
                    //OsiClpSolverInterface * clpSolver = dynamic_cast< OsiClpSolverInterface*> (model.solver());
                    //ClpSimplex * lpSolver = clpSolver->getModelPtr();
                    //lpSolver->setMaximumSeconds(value);
                    model.setDblParam(CbcModel::CbcMaximumSeconds, value) ;
               }
               break ;
          case CLP_PARAM_DBL_DUALTOLERANCE:
          case CLP_PARAM_DBL_PRIMALTOLERANCE:
               setDoubleParameter(model.solver(), value);
               return 0; // to avoid message
          default:
               break;
          }
          sprintf(printArray, "%s was changed from %g to %g",
                  name_.c_str(), oldValue, value);
          returnCode = 0;
     }
     return printArray;
}
double
CbcOrClpParam::doubleParameter (CbcModel &model) const
{
     double value;
     switch (type_) {
     case CBC_PARAM_DBL_INFEASIBILITYWEIGHT:
          value = model.getDblParam(CbcModel::CbcInfeasibilityWeight);
          break;
     case CBC_PARAM_DBL_INTEGERTOLERANCE:
          value = model.getDblParam(CbcModel::CbcIntegerTolerance);
          break;
     case CBC_PARAM_DBL_INCREMENT:
          value = model.getDblParam(CbcModel::CbcCutoffIncrement);
          break;
     case CBC_PARAM_DBL_ALLOWABLEGAP:
          value = model.getDblParam(CbcModel::CbcAllowableGap);
          break;
     case CBC_PARAM_DBL_GAPRATIO:
          value = model.getDblParam(CbcModel::CbcAllowableFractionGap);
          break;
     case CBC_PARAM_DBL_CUTOFF:
          value = model.getCutoff();
          break;
     case CBC_PARAM_DBL_TIMELIMIT_BAB:
          value = model.getDblParam(CbcModel::CbcMaximumSeconds) ;
          break ;
     case CLP_PARAM_DBL_DUALTOLERANCE:
     case CLP_PARAM_DBL_PRIMALTOLERANCE:
          value = doubleParameter(model.solver());
          break;
     default:
          value = doubleValue_;
          break;
     }
     return value;
}
int
CbcOrClpParam::setIntParameter (CbcModel &model, int value)
{
     int returnCode;
     setIntParameterWithMessage(model, value, returnCode);
     if (doPrinting && strlen(printArray))
          std::cout << printArray << std::endl;
     return returnCode;
}
// Sets int parameter and returns printable string and error code
const char *
CbcOrClpParam::setIntParameterWithMessage ( CbcModel & model, int value , int & returnCode)
{
     if (value < lowerIntValue_ || value > upperIntValue_) {
          sprintf(printArray, "%d was provided for %s - valid range is %d to %d",
                  value, name_.c_str(), lowerIntValue_, upperIntValue_);
          returnCode = 1;
     } else {
          int oldValue = intValue_;
          intValue_ = value;
          switch (type_) {
          case CLP_PARAM_INT_LOGLEVEL:
               oldValue = model.messageHandler()->logLevel();
               model.messageHandler()->setLogLevel(CoinAbs(value));
               break;
          case CLP_PARAM_INT_SOLVERLOGLEVEL:
               oldValue = model.solver()->messageHandler()->logLevel();
               model.solver()->messageHandler()->setLogLevel(value);
               break;
          case CBC_PARAM_INT_MAXNODES:
               oldValue = model.getIntParam(CbcModel::CbcMaxNumNode);
               model.setIntParam(CbcModel::CbcMaxNumNode, value);
               break;
          case CBC_PARAM_INT_MAXSOLS:
               oldValue = model.getIntParam(CbcModel::CbcMaxNumSol);
               model.setIntParam(CbcModel::CbcMaxNumSol, value);
               break;
          case CBC_PARAM_INT_MAXSAVEDSOLS:
               oldValue = model.maximumSavedSolutions();
               model.setMaximumSavedSolutions(value);
               break;
          case CBC_PARAM_INT_STRONGBRANCHING:
               oldValue = model.numberStrong();
               model.setNumberStrong(value);
               break;
          case CBC_PARAM_INT_NUMBERBEFORE:
               oldValue = model.numberBeforeTrust();
               model.setNumberBeforeTrust(value);
               break;
          case CBC_PARAM_INT_NUMBERANALYZE:
               oldValue = model.numberAnalyzeIterations();
               model.setNumberAnalyzeIterations(value);
               break;
          case CBC_PARAM_INT_CUTPASSINTREE:
               oldValue = model.getMaximumCutPasses();
               model.setMaximumCutPasses(value);
               break;
          case CBC_PARAM_INT_CUTPASS:
               oldValue = model.getMaximumCutPassesAtRoot();
               model.setMaximumCutPassesAtRoot(value);
               break;
#ifdef COIN_HAS_CBC
#ifdef CBC_THREAD
          case CBC_PARAM_INT_THREADS:
               oldValue = model.getNumberThreads();
               model.setNumberThreads(value);
               break;
#endif
          case CBC_PARAM_INT_RANDOMSEED:
               oldValue = model.getRandomSeed();
               model.setRandomSeed(value);
               break;
#endif
          default:
               break;
          }
          sprintf(printArray, "%s was changed from %d to %d",
                  name_.c_str(), oldValue, value);
          returnCode = 0;
     }
     return printArray;
}
int
CbcOrClpParam::intParameter (CbcModel &model) const
{
     int value;
     switch (type_) {
     case CLP_PARAM_INT_LOGLEVEL:
          value = model.messageHandler()->logLevel();
          break;
     case CLP_PARAM_INT_SOLVERLOGLEVEL:
          value = model.solver()->messageHandler()->logLevel();
          break;
     case CBC_PARAM_INT_MAXNODES:
          value = model.getIntParam(CbcModel::CbcMaxNumNode);
          break;
     case CBC_PARAM_INT_MAXSOLS:
          value = model.getIntParam(CbcModel::CbcMaxNumSol);
          break;
     case CBC_PARAM_INT_MAXSAVEDSOLS:
          value = model.maximumSavedSolutions();
	  break;
     case CBC_PARAM_INT_STRONGBRANCHING:
          value = model.numberStrong();
          break;
     case CBC_PARAM_INT_NUMBERBEFORE:
          value = model.numberBeforeTrust();
          break;
     case CBC_PARAM_INT_NUMBERANALYZE:
          value = model.numberAnalyzeIterations();
          break;
     case CBC_PARAM_INT_CUTPASSINTREE:
          value = model.getMaximumCutPasses();
          break;
     case CBC_PARAM_INT_CUTPASS:
          value = model.getMaximumCutPassesAtRoot();
          break;
#ifdef COIN_HAS_CBC
#ifdef CBC_THREAD
     case CBC_PARAM_INT_THREADS:
          value = model.getNumberThreads();
#endif
     case CBC_PARAM_INT_RANDOMSEED:
          value = model.getRandomSeed();
          break;
#endif
     default:
          value = intValue_;
          break;
     }
     return value;
}
#endif
// Sets current parameter option using string
void
CbcOrClpParam::setCurrentOption ( const std::string value )
{
     int action = parameterOption(value);
     if (action >= 0)
          currentKeyWord_ = action;
}
// Sets current parameter option
void
CbcOrClpParam::setCurrentOption ( int value , bool printIt)
{
     if (printIt && value != currentKeyWord_)
          std::cout << "Option for " << name_ << " changed from "
                    << definedKeyWords_[currentKeyWord_] << " to "
                    << definedKeyWords_[value] << std::endl;

     currentKeyWord_ = value;
}
// Sets current parameter option and returns printable string
const char *
CbcOrClpParam::setCurrentOptionWithMessage ( int value )
{
     if (value != currentKeyWord_) {
          sprintf(printArray, "Option for %s changed from %s to %s",
                  name_.c_str(), definedKeyWords_[currentKeyWord_].c_str(),
                  definedKeyWords_[value].c_str());

          currentKeyWord_ = value;
     } else {
          printArray[0] = '\0';
     }
     return printArray;
}
void
CbcOrClpParam::setIntValue ( int value )
{
     if (value < lowerIntValue_ || value > upperIntValue_) {
          std::cout << value << " was provided for " << name_ <<
                    " - valid range is " << lowerIntValue_ << " to " <<
                    upperIntValue_ << std::endl;
     } else {
          intValue_ = value;
     }
}
void
CbcOrClpParam::setDoubleValue ( double value )
{
     if (value < lowerDoubleValue_ || value > upperDoubleValue_) {
          std::cout << value << " was provided for " << name_ <<
                    " - valid range is " << lowerDoubleValue_ << " to " <<
                    upperDoubleValue_ << std::endl;
     } else {
          doubleValue_ = value;
     }
}
void
CbcOrClpParam::setStringValue ( std::string value )
{
     stringValue_ = value;
}
static char line[1000];
static char * where = NULL;
extern int CbcOrClpRead_mode;
int CbcOrClpEnvironmentIndex = -1;
static size_t fillEnv()
{
#if defined(_MSC_VER) || defined(__MSVCRT__)
     return 0;
#else
     // Don't think it will work on Windows
     char * environ = getenv("CBC_CLP_ENVIRONMENT");
     size_t length = 0;
     if (environ) {
          length = strlen(environ);
          if (CbcOrClpEnvironmentIndex < static_cast<int>(length)) {
               // find next non blank
               char * whereEnv = environ + CbcOrClpEnvironmentIndex;
               // munch white space
               while (*whereEnv == ' ' || *whereEnv == '\t' || *whereEnv < ' ')
                    whereEnv++;
               // copy
               char * put = line;
               while ( *whereEnv != '\0' ) {
                    if ( *whereEnv == ' ' || *whereEnv == '\t' || *whereEnv < ' ' ) {
                         break;
                    }
                    *put = *whereEnv;
                    put++;
                    assert (put - line < 1000);
                    whereEnv++;
               }
               CbcOrClpEnvironmentIndex = static_cast<int>(whereEnv - environ);
               *put = '\0';
               length = strlen(line);
          } else {
               length = 0;
          }
     }
     if (!length)
          CbcOrClpEnvironmentIndex = -1;
     return length;
#endif
}
extern FILE * CbcOrClpReadCommand;
// Simple read stuff
std::string
CoinReadNextField()
{
     std::string field;
     if (!where) {
          // need new line
#ifdef COIN_HAS_READLINE
          if (CbcOrClpReadCommand == stdin) {
               // Get a line from the user.
               where = readline (coin_prompt);

               // If the line has any text in it, save it on the history.
               if (where) {
                    if ( *where)
                         add_history (where);
                    strcpy(line, where);
                    free(where);
               }
          } else {
               where = fgets(line, 1000, CbcOrClpReadCommand);
          }
#else
          if (CbcOrClpReadCommand == stdin) {
	       fputs(coin_prompt,stdout);
               fflush(stdout);
          }
          where = fgets(line, 1000, CbcOrClpReadCommand);
#endif
          if (!where)
               return field; // EOF
          where = line;
          // clean image
          char * lastNonBlank = line - 1;
          while ( *where != '\0' ) {
               if ( *where != '\t' && *where < ' ' ) {
                    break;
               } else if ( *where != '\t' && *where != ' ') {
                    lastNonBlank = where;
               }
               where++;
          }
          where = line;
          *(lastNonBlank + 1) = '\0';
     }
     // munch white space
     while (*where == ' ' || *where == '\t')
          where++;
     char * saveWhere = where;
     while (*where != ' ' && *where != '\t' && *where != '\0')
          where++;
     if (where != saveWhere) {
          char save = *where;
          *where = '\0';
          //convert to string
          field = saveWhere;
          *where = save;
     } else {
          where = NULL;
          field = "EOL";
     }
     return field;
}

std::string
CoinReadGetCommand(int argc, const char *argv[])
{
     std::string field = "EOL";
     // say no =
     afterEquals = "";
     while (field == "EOL") {
          if (CbcOrClpRead_mode > 0) {
               if ((CbcOrClpRead_mode < argc && argv[CbcOrClpRead_mode]) ||
                         CbcOrClpEnvironmentIndex >= 0) {
                    if (CbcOrClpEnvironmentIndex < 0) {
                         field = argv[CbcOrClpRead_mode++];
                    } else {
                         if (fillEnv()) {
                              field = line;
                         } else {
                              // not there
                              continue;
                         }
                    }
                    if (field == "-") {
                         std::cout << "Switching to line mode" << std::endl;
                         CbcOrClpRead_mode = -1;
                         field = CoinReadNextField();
                    } else if (field[0] != '-') {
                         if (CbcOrClpRead_mode != 2) {
                              // now allow std::cout<<"skipping non-command "<<field<<std::endl;
                              // field="EOL"; // skip
                         } else if (CbcOrClpEnvironmentIndex < 0) {
                              // special dispensation - taken as -import name
                              CbcOrClpRead_mode--;
                              field = "import";
                         }
                    } else {
                         if (field != "--") {
                              // take off -
                              field = field.substr(1);
                         } else {
                              // special dispensation - taken as -import --
                              CbcOrClpRead_mode--;
                              field = "import";
                         }
                    }
               } else {
                    field = "";
               }
          } else {
               field = CoinReadNextField();
          }
     }
     // if = then modify and save
     std::string::size_type found = field.find('=');
     if (found != std::string::npos) {
          afterEquals = field.substr(found + 1);
          field = field.substr(0, found);
     }
     //std::cout<<field<<std::endl;
     return field;
}
std::string
CoinReadGetString(int argc, const char *argv[])
{
     std::string field = "EOL";
     if (afterEquals == "") {
          if (CbcOrClpRead_mode > 0) {
               if (CbcOrClpRead_mode < argc || CbcOrClpEnvironmentIndex >= 0) {
                    if (CbcOrClpEnvironmentIndex < 0) {
                         if (argv[CbcOrClpRead_mode][0] != '-') {
                              field = argv[CbcOrClpRead_mode++];
                         } else if (!strcmp(argv[CbcOrClpRead_mode], "--")) {
                              field = argv[CbcOrClpRead_mode++];
                              // -- means import from stdin
                              field = "-";
                         }
                    } else {
                         fillEnv();
                         field = line;
                    }
               }
          } else {
               field = CoinReadNextField();
          }
     } else {
          field = afterEquals;
          afterEquals = "";
     }
     //std::cout<<field<<std::endl;
     return field;
}
// valid 0 - okay, 1 bad, 2 not there
int
CoinReadGetIntField(int argc, const char *argv[], int * valid)
{
     std::string field = "EOL";
     if (afterEquals == "") {
          if (CbcOrClpRead_mode > 0) {
               if (CbcOrClpRead_mode < argc || CbcOrClpEnvironmentIndex >= 0) {
                    if (CbcOrClpEnvironmentIndex < 0) {
                         // may be negative value so do not check for -
                         field = argv[CbcOrClpRead_mode++];
                    } else {
                         fillEnv();
                         field = line;
                    }
               }
          } else {
               field = CoinReadNextField();
          }
     } else {
          field = afterEquals;
          afterEquals = "";
     }
     long int value = 0;
     //std::cout<<field<<std::endl;
     if (field != "EOL") {
          const char * start = field.c_str();
          char * endPointer = NULL;
          // check valid
          value =  strtol(start, &endPointer, 10);
          if (*endPointer == '\0') {
               *valid = 0;
          } else {
               *valid = 1;
               std::cout << "String of " << field;
          }
     } else {
          *valid = 2;
     }
     return static_cast<int>(value);
}
double
CoinReadGetDoubleField(int argc, const char *argv[], int * valid)
{
     std::string field = "EOL";
     if (afterEquals == "") {
          if (CbcOrClpRead_mode > 0) {
               if (CbcOrClpRead_mode < argc || CbcOrClpEnvironmentIndex >= 0) {
                    if (CbcOrClpEnvironmentIndex < 0) {
                         // may be negative value so do not check for -
                         field = argv[CbcOrClpRead_mode++];
                    } else {
                         fillEnv();
                         field = line;
                    }
               }
          } else {
               field = CoinReadNextField();
          }
     } else {
          field = afterEquals;
          afterEquals = "";
     }
     double value = 0.0;
     //std::cout<<field<<std::endl;
     if (field != "EOL") {
          const char * start = field.c_str();
          char * endPointer = NULL;
          // check valid
          value =  strtod(start, &endPointer);
          if (*endPointer == '\0') {
               *valid = 0;
          } else {
               *valid = 1;
               std::cout << "String of " << field;
          }
     } else {
          *valid = 2;
     }
     return value;
}
/*
  Subroutine to establish the cbc parameter array. See the description of
  class CbcOrClpParam for details. Pulled from C..Main() for clarity.
*/
void
establishParams (int &numberParameters, CbcOrClpParam *const parameters)
{
     numberParameters = 0;
     parameters[numberParameters++] =
          CbcOrClpParam("?", "For help", CBC_PARAM_GENERALQUERY, 7, 0);
     parameters[numberParameters++] =
          CbcOrClpParam("???", "For help", CBC_PARAM_FULLGENERALQUERY, 7, 0);
     parameters[numberParameters++] =
          CbcOrClpParam("-", "From stdin",
                        CLP_PARAM_ACTION_STDIN, 3, 0);
#ifdef ABC_INHERIT
      parameters[numberParameters++] =
          CbcOrClpParam("abc", "Whether to visit Aboca",
                        "off", CLP_PARAM_STR_ABCWANTED, 7, 0);
     parameters[numberParameters-1].append("one");
     parameters[numberParameters-1].append("two");
     parameters[numberParameters-1].append("three");
     parameters[numberParameters-1].append("four");
     parameters[numberParameters-1].append("five");
     parameters[numberParameters-1].append("six");
     parameters[numberParameters-1].append("seven");
     parameters[numberParameters-1].append("eight");
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("decide");
     parameters[numberParameters-1].setLonghelp
     (
          "Decide whether to use A Basic Optimization Code (Accelerated?) \
and whether to try going parallel!"
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("allC!ommands", "Whether to print less used commands",
                        "no", CLP_PARAM_STR_ALLCOMMANDS);
     parameters[numberParameters-1].append("more");
     parameters[numberParameters-1].append("all");
     parameters[numberParameters-1].setLonghelp
     (
          "For the sake of your sanity, only the more useful and simple commands \
are printed out on ?."
     );
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("allow!ableGap", "Stop when gap between best possible and \
best less than this",
                        0.0, 1.0e20, CBC_PARAM_DBL_ALLOWABLEGAP);
     parameters[numberParameters-1].setDoubleValue(0.0);
     parameters[numberParameters-1].setLonghelp
     (
          "If the gap between best solution and best possible solution is less than this \
then the search will be terminated.  Also see ratioGap."
     );
#endif
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("allS!lack", "Set basis back to all slack and reset solution",
                        CLP_PARAM_ACTION_ALLSLACK, 3);
     parameters[numberParameters-1].setLonghelp
     (
          "Mainly useful for tuning purposes.  Normally the first dual or primal will be using an all slack \
basis anyway."
     );
#endif
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("artif!icialCost", "Costs >= this treated as artificials in feasibility pump",
                        0.0, COIN_DBL_MAX, CBC_PARAM_DBL_ARTIFICIALCOST, 1);
     parameters[numberParameters-1].setDoubleValue(0.0);
     parameters[numberParameters-1].setLonghelp
     (
          "0.0 off - otherwise variables with costs >= this are treated as artificials and fixed to lower bound in feasibility pump"
     );
#endif
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("auto!Scale", "Whether to scale objective, rhs and bounds of problem if they look odd",
                        "off", CLP_PARAM_STR_AUTOSCALE, 7, 0);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].setLonghelp
     (
          "If you think you may get odd objective values or large equality rows etc then\
 it may be worth setting this true.  It is still experimental and you may prefer\
 to use objective!Scale and rhs!Scale."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("barr!ier", "Solve using primal dual predictor corrector algorithm",
                        CLP_PARAM_ACTION_BARRIER);
     parameters[numberParameters-1].setLonghelp
     (
          "This command solves the current model using the  primal dual predictor \
corrector algorithm.  You may want to link in an alternative \
ordering and factorization.  It will also solve models \
with quadratic objectives."

     );
     parameters[numberParameters++] =
          CbcOrClpParam("basisI!n", "Import basis from bas file",
                        CLP_PARAM_ACTION_BASISIN, 3);
     parameters[numberParameters-1].setLonghelp
     (
          "This will read an MPS format basis file from the given file name.  It will use the default\
 directory given by 'directory'.  A name of '$' will use the previous value for the name.  This\
 is initialized to '', i.e. it must be set.  If you have libz then it can read compressed\
 files 'xxxxxxxx.gz' or xxxxxxxx.bz2."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("basisO!ut", "Export basis as bas file",
                        CLP_PARAM_ACTION_BASISOUT);
     parameters[numberParameters-1].setLonghelp
     (
          "This will write an MPS format basis file to the given file name.  It will use the default\
 directory given by 'directory'.  A name of '$' will use the previous value for the name.  This\
 is initialized to 'default.bas'."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("biasLU", "Whether factorization biased towards U",
                        "UU", CLP_PARAM_STR_BIASLU, 2, 0);
     parameters[numberParameters-1].append("UX");
     parameters[numberParameters-1].append("LX");
     parameters[numberParameters-1].append("LL");
     parameters[numberParameters-1].setCurrentOption("LX");
#endif
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("branch!AndCut", "Do Branch and Cut",
                        CBC_PARAM_ACTION_BAB);
     parameters[numberParameters-1].setLonghelp
     (
          "This does branch and cut.  There are many parameters which can affect the performance.  \
First just try with default settings and look carefully at the log file.  Did cuts help?  Did they take too long?  \
Look at output to see which cuts were effective and then do some tuning.  You will see that the \
options for cuts are off, on, root and ifmove, forceon.  Off is \
obvious, on means that this cut generator will be tried in the branch and cut tree (you can fine tune using \
'depth').  Root means just at the root node while 'ifmove' means that cuts will be used in the tree if they \
look as if they are doing some good and moving the objective value.  Forceon is same as on but forces code to use \
cut generator at every node.  For probing forceonbut just does fixing probing in tree - not strengthening etc.  \
If pre-processing reduced the size of the \
problem or strengthened many coefficients then it is probably wise to leave it on.  Switch off heuristics \
which did not provide solutions.  The other major area to look at is the search.  Hopefully good solutions \
were obtained fairly early in the search so the important point is to select the best variable to branch on.  \
See whether strong branching did a good job - or did it just take a lot of iterations.  Adjust the strongBranching \
and trustPseudoCosts parameters.  If cuts did a good job, then you may wish to \
have more rounds of cuts - see passC!uts and passT!ree."
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("bscale", "Whether to scale in barrier (and ordering speed)",
                        "off", CLP_PARAM_STR_BARRIERSCALE, 7, 0);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("off1");
     parameters[numberParameters-1].append("on1");
     parameters[numberParameters-1].append("off2");
     parameters[numberParameters-1].append("on2");
     parameters[numberParameters++] =
          CbcOrClpParam("chol!esky", "Which cholesky algorithm",
                        "native", CLP_PARAM_STR_CHOLESKY, 7);
     parameters[numberParameters-1].append("dense");
     //#ifdef FOREIGN_BARRIER
#ifdef COIN_HAS_WSMP
     parameters[numberParameters-1].append("fudge!Long");
     parameters[numberParameters-1].append("wssmp");
#else
     parameters[numberParameters-1].append("fudge!Long_dummy");
     parameters[numberParameters-1].append("wssmp_dummy");
#endif
#if defined(COIN_HAS_AMD) || defined(COIN_HAS_CHOLMOD) || defined(COIN_HAS_GLPK)
     parameters[numberParameters-1].append("Uni!versityOfFlorida");
#else
     parameters[numberParameters-1].append("Uni!versityOfFlorida_dummy");
#endif
#ifdef TAUCS_BARRIER
     parameters[numberParameters-1].append("Taucs");
#else
     parameters[numberParameters-1].append("Taucs_dummy");
#endif
#ifdef COIN_HAS_MUMPS
     parameters[numberParameters-1].append("Mumps");
#else
     parameters[numberParameters-1].append("Mumps_dummy");
#endif
     parameters[numberParameters-1].setLonghelp
     (
          "For a barrier code to be effective it needs a good Cholesky ordering and factorization.  \
The native ordering and factorization is not state of the art, although acceptable.  \
You may want to link in one from another source.  See Makefile.locations for some \
possibilities."
     );
     //#endif
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("clique!Cuts", "Whether to use Clique cuts",
                        "off", CBC_PARAM_STR_CLIQUECUTS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("root");
     parameters[numberParameters-1].append("ifmove");
     parameters[numberParameters-1].append("forceOn");
     parameters[numberParameters-1].append("onglobal");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on clique cuts (either at root or in entire tree) \
See branchAndCut for information on options."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("combine!Solutions", "Whether to use combine solution heuristic",
                        "off", CBC_PARAM_STR_COMBINE);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on a heuristic which does branch and cut on the problem given by just \
using variables which have appeared in one or more solutions. \
It obviously only tries after two or more solutions. \
See Rounding for meaning of on,both,before"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("combine2!Solutions", "Whether to use crossover solution heuristic",
                        "off", CBC_PARAM_STR_CROSSOVER2);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on a heuristic which does branch and cut on the problem given by \
fixing variables which have same value in two or more solutions. \
It obviously only tries after two or more solutions. \
See Rounding for meaning of on,both,before"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("constraint!fromCutoff", "Whether to use cutoff as constraint",
                        "off", CBC_PARAM_STR_CUTOFF_CONSTRAINT);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].setLonghelp
     (
          "This adds the objective as a constraint with best solution as RHS"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("cost!Strategy", "How to use costs as priorities",
                        "off", CBC_PARAM_STR_COSTSTRATEGY);
     parameters[numberParameters-1].append("pri!orities");
     parameters[numberParameters-1].append("column!Order?");
     parameters[numberParameters-1].append("01f!irst?");
     parameters[numberParameters-1].append("01l!ast?");
     parameters[numberParameters-1].append("length!?");
     parameters[numberParameters-1].append("singletons");
     parameters[numberParameters-1].append("nonzero");
     parameters[numberParameters-1].setLonghelp
     (
          "This orders the variables in order of their absolute costs - with largest cost ones being branched on \
first.  This primitive strategy can be surprsingly effective.  The column order\
 option is obviously not on costs but easy to code here."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("cplex!Use", "Whether to use Cplex!",
                        "off", CBC_PARAM_STR_CPX);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].setLonghelp
     (
          " If the user has Cplex, but wants to use some of Cbc's heuristics \
then you can!  If this is on, then Cbc will get to the root node and then \
hand over to Cplex.  If heuristics find a solution this can be significantly \
quicker.  You will probably want to switch off Cbc's cuts as Cplex thinks \
they are genuine constraints.  It is also probable that you want to switch \
off preprocessing, although for difficult problems it is worth trying \
both."
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("cpp!Generate", "Generates C++ code",
                        -1, 50000, CLP_PARAM_INT_CPP, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "Once you like what the stand-alone solver does then this allows \
you to generate user_driver.cpp which approximates the code.  \
0 gives simplest driver, 1 generates saves and restores, 2 \
generates saves and restores even for variables at default value. \
4 bit in cbc generates size dependent code rather than computed values.  \
This is now deprecated as you can call stand-alone solver - see \
Cbc/examples/driver4.cpp."
     );
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("crash", "Whether to create basis for problem",
                        "off", CLP_PARAM_STR_CRASH);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("so!low_halim");
     parameters[numberParameters-1].append("lots");
#ifdef ABC_INHERIT
     parameters[numberParameters-1].append("dual");
     parameters[numberParameters-1].append("dw");
     parameters[numberParameters-1].append("idiot");
#endif
     parameters[numberParameters-1].setLonghelp
     (
          "If crash is set on and there is an all slack basis then Clp will flip or put structural\
 variables into basis with the aim of getting dual feasible.  On the whole dual seems to be\
 better without it and there are alternative types of 'crash' for primal e.g. 'idiot' or 'sprint'. \
I have also added a variant due to Solow and Halim which is as on but just flip.");
     parameters[numberParameters++] =
          CbcOrClpParam("cross!over", "Whether to get a basic solution after barrier",
                        "on", CLP_PARAM_STR_CROSSOVER);
     parameters[numberParameters-1].append("off");
     parameters[numberParameters-1].append("maybe");
     parameters[numberParameters-1].append("presolve");
     parameters[numberParameters-1].setLonghelp
     (
          "Interior point algorithms do not obtain a basic solution (and \
the feasibility criterion is a bit suspect (JJF)).  This option will crossover \
to a basic solution suitable for ranging or branch and cut.  With the current state \
of quadratic it may be a good idea to switch off crossover for quadratic (and maybe \
presolve as well) - the option maybe does this."
     );
#endif
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("csv!Statistics", "Create one line of statistics",
                        CLP_PARAM_ACTION_CSVSTATISTICS, 2, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "This appends statistics to given file name.  It will use the default\
 directory given by 'directory'.  A name of '$' will use the previous value for the name.  This\
 is initialized to '', i.e. it must be set.  Adds header if file empty or does not exist."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("cutD!epth", "Depth in tree at which to do cuts",
                        -1, 999999, CBC_PARAM_INT_CUTDEPTH);
     parameters[numberParameters-1].setLonghelp
     (
          "Cut generators may be - off, on only at root, on if they look possible \
and on.  If they are done every node then that is that, but it may be worth doing them \
every so often.  The original method was every so many nodes but it is more logical \
to do it whenever depth in tree is a multiple of K.  This option does that and defaults \
to -1 (off -> code decides)."
     );
     parameters[numberParameters-1].setIntValue(-1);
     parameters[numberParameters++] =
          CbcOrClpParam("cutL!ength", "Length of a cut",
                        -1, COIN_INT_MAX, CBC_PARAM_INT_CUTLENGTH);
     parameters[numberParameters-1].setLonghelp
     (
          "At present this only applies to Gomory cuts. -1 (default) leaves as is. \
Any value >0 says that all cuts <= this length can be generated both at \
root node and in tree. 0 says to use some dynamic lengths.  If value >=10,000,000 \
then the length in tree is value%10000000 - so 10000100 means unlimited length \
at root and 100 in tree."
     );
     parameters[numberParameters-1].setIntValue(-1);
     parameters[numberParameters++] =
          CbcOrClpParam("cuto!ff", "All solutions must be better than this",
                        -1.0e60, 1.0e60, CBC_PARAM_DBL_CUTOFF);
     parameters[numberParameters-1].setDoubleValue(1.0e50);
     parameters[numberParameters-1].setLonghelp
     (
          "All solutions must be better than this value (in a minimization sense).  \
This is also set by code whenever it obtains a solution and is set to value of \
objective for solution minus cutoff increment."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("cuts!OnOff", "Switches all cuts on or off",
                        "off", CBC_PARAM_STR_CUTSSTRATEGY);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("root");
     parameters[numberParameters-1].append("ifmove");
     parameters[numberParameters-1].append("forceOn");
     parameters[numberParameters-1].setLonghelp
     (
          "This can be used to switch on or off all cuts (apart from Reduce and Split).  Then you can do \
individual ones off or on \
See branchAndCut for information on options."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("debug!In", "read valid solution from file",
                        CLP_PARAM_ACTION_DEBUG, 7, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "This will read a solution file from the given file name.  It will use the default\
 directory given by 'directory'.  A name of '$' will use the previous value for the name.  This\
 is initialized to '', i.e. it must be set.\n\n\
If set to create it will create a file called debug.file  after search.\n\n\
The idea is that if you suspect a bad cut generator \
you can do a good run with debug set to 'create' and then switch on the cuts you suspect and \
re-run with debug set to 'debug.file'  The create case has same effect as saveSolution."
     );
#endif
#ifdef COIN_HAS_CLP
#if CLP_MULTIPLE_FACTORIZATIONS >0
     parameters[numberParameters++] =
          CbcOrClpParam("dense!Threshold", "Whether to use dense factorization",
                        -1, 10000, CBC_PARAM_INT_DENSE, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "If processed problem <= this use dense factorization"
     );
     parameters[numberParameters-1].setIntValue(-1);
#endif
#endif
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("depth!MiniBab", "Depth at which to try mini BAB",
                        -COIN_INT_MAX, COIN_INT_MAX, CBC_PARAM_INT_DEPTHMINIBAB);
     parameters[numberParameters-1].setIntValue(-1);
     parameters[numberParameters-1].setLonghelp
     (
          "Rather a complicated parameter but can be useful. -1 means off for large problems but on as if -12 for problems where rows+columns<500, -2 \
means use Cplex if it is linked in.  Otherwise if negative then go into depth first complete search fast branch and bound when depth>= -value-2 (so -3 will use this at depth>=1).  This mode is only switched on after 500 nodes.  If you really want to switch it off for small problems then set this to -999.  If >=0 the value doesn't matter very much.  The code will do approximately 100 nodes of fast branch and bound every now and then at depth>=5.  The actual logic is too twisted to describe here."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("dextra3", "Extra double parameter 3",
                        -COIN_DBL_MAX, COIN_DBL_MAX, CBC_PARAM_DBL_DEXTRA3, 0);
     parameters[numberParameters-1].setDoubleValue(0.0);
     parameters[numberParameters++] =
          CbcOrClpParam("dextra4", "Extra double parameter 4",
                        -COIN_DBL_MAX, COIN_DBL_MAX, CBC_PARAM_DBL_DEXTRA4, 0);
     parameters[numberParameters-1].setDoubleValue(0.0);
     parameters[numberParameters++] =
          CbcOrClpParam("dextra5", "Extra double parameter 5",
                        -COIN_DBL_MAX, COIN_DBL_MAX, CBC_PARAM_DBL_DEXTRA5, 0);
     parameters[numberParameters-1].setDoubleValue(0.0);
     parameters[numberParameters++] =
          CbcOrClpParam("Dins", "Whether to try Distance Induced Neighborhood Search",
                        "off", CBC_PARAM_STR_DINS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters-1].append("often");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on Distance induced neighborhood Search. \
See Rounding for meaning of on,both,before"
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("direction", "Minimize or Maximize",
                        "min!imize", CLP_PARAM_STR_DIRECTION);
     parameters[numberParameters-1].append("max!imize");
     parameters[numberParameters-1].append("zero");
     parameters[numberParameters-1].setLonghelp
     (
          "The default is minimize - use 'direction maximize' for maximization.\n\
You can also use the parameters 'maximize' or 'minimize'."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("directory", "Set Default directory for import etc.",
                        CLP_PARAM_ACTION_DIRECTORY);
     parameters[numberParameters-1].setLonghelp
     (
          "This sets the directory which import, export, saveModel, restoreModel etc will use.\
  It is initialized to './'"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("dirSample", "Set directory where the COIN-OR sample problems are.",
                        CLP_PARAM_ACTION_DIRSAMPLE, 7, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "This sets the directory where the COIN-OR sample problems reside. It is\
 used only when -unitTest is passed to clp. clp will pick up the test problems\
 from this directory.\
 It is initialized to '../../Data/Sample'"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("dirNetlib", "Set directory where the netlib problems are.",
                        CLP_PARAM_ACTION_DIRNETLIB, 7, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "This sets the directory where the netlib problems reside. One can get\
 the netlib problems from COIN-OR or from the main netlib site. This\
 parameter is used only when -netlib is passed to clp. clp will pick up the\
 netlib problems from this directory. If clp is built without zlib support\
 then the problems must be uncompressed.\
 It is initialized to '../../Data/Netlib'"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("dirMiplib", "Set directory where the miplib 2003 problems are.",
                        CBC_PARAM_ACTION_DIRMIPLIB, 7, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "This sets the directory where the miplib 2003 problems reside. One can\
 get the miplib problems from COIN-OR or from the main miplib site. This\
 parameter is used only when -miplib is passed to cbc. cbc will pick up the\
 miplib problems from this directory. If cbc is built without zlib support\
 then the problems must be uncompressed.\
 It is initialized to '../../Data/miplib3'"
     );
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("diveO!pt", "Diving options",
                        -1, 200000, CBC_PARAM_INT_DIVEOPT, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "If >2 && <8 then modify diving options - \
	 \n\t3 only at root and if no solution,  \
	 \n\t4 only at root and if this heuristic has not got solution, \
	 \n\t5 only at depth <4, \
	 \n\t6 decay, \
	 \n\t7 run up to 2 times if solution found 4 otherwise."
     );
     parameters[numberParameters-1].setIntValue(-1);
     parameters[numberParameters++] =
          CbcOrClpParam("DivingS!ome", "Whether to try Diving heuristics",
                        "off", CBC_PARAM_STR_DIVINGS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on a random diving heuristic at various times. \
C - Coefficient, F - Fractional, G - Guided, L - LineSearch, P - PseudoCost, V - VectorLength. \
You may prefer to use individual on/off \
See Rounding for meaning of on,both,before"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("DivingC!oefficient", "Whether to try DiveCoefficient",
                        "off", CBC_PARAM_STR_DIVINGC);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters++] =
          CbcOrClpParam("DivingF!ractional", "Whether to try DiveFractional",
                        "off", CBC_PARAM_STR_DIVINGF);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters++] =
          CbcOrClpParam("DivingG!uided", "Whether to try DiveGuided",
                        "off", CBC_PARAM_STR_DIVINGG);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters++] =
          CbcOrClpParam("DivingL!ineSearch", "Whether to try DiveLineSearch",
                        "off", CBC_PARAM_STR_DIVINGL);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters++] =
          CbcOrClpParam("DivingP!seudoCost", "Whether to try DivePseudoCost",
                        "off", CBC_PARAM_STR_DIVINGP);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters++] =
          CbcOrClpParam("DivingV!ectorLength", "Whether to try DiveVectorLength",
                        "off", CBC_PARAM_STR_DIVINGV);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters++] =
          CbcOrClpParam("doH!euristic", "Do heuristics before any preprocessing",
                        CBC_PARAM_ACTION_DOHEURISTIC, 3);
     parameters[numberParameters-1].setLonghelp
     (
          "Normally heuristics are done in branch and bound.  It may be useful to do them outside. \
Only those heuristics with 'both' or 'before' set will run.  \
Doing this may also set cutoff, which can help with preprocessing."
     );
#endif
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("dualB!ound", "Initially algorithm acts as if no \
gap between bounds exceeds this value",
                        1.0e-20, 1.0e12, CLP_PARAM_DBL_DUALBOUND);
     parameters[numberParameters-1].setLonghelp
     (
          "The dual algorithm in Clp is a single phase algorithm as opposed to a two phase\
 algorithm where you first get feasible then optimal.  If a problem has both upper and\
 lower bounds then it is trivial to get dual feasible by setting non basic variables\
 to correct bound.  If the gap between the upper and lower bounds of a variable is more\
 than the value of dualBound Clp introduces fake bounds so that it can make the problem\
 dual feasible.  This has the same effect as a composite objective function in the\
 primal algorithm.  Too high a value may mean more iterations, while too low a bound means\
 the code may go all the way and then have to increase the bounds.  OSL had a heuristic to\
 adjust bounds, maybe we need that here."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("dualize", "Solves dual reformulation",
                        0, 4, CLP_PARAM_INT_DUALIZE, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "Don't even think about it."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("dualP!ivot", "Dual pivot choice algorithm",
                        "auto!matic", CLP_PARAM_STR_DUALPIVOT, 7, 1);
     parameters[numberParameters-1].append("dant!zig");
     parameters[numberParameters-1].append("partial");
     parameters[numberParameters-1].append("steep!est");
     parameters[numberParameters-1].setLonghelp
     (
          "Clp can use any pivot selection algorithm which the user codes as long as it\
 implements the features in the abstract pivot base class.  The Dantzig method is implemented\
 to show a simple method but its use is deprecated.  Steepest is the method of choice and there\
 are two variants which keep all weights updated but only scan a subset each iteration.\
 Partial switches this on while automatic decides at each iteration based on information\
 about the factorization."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("dualS!implex", "Do dual simplex algorithm",
                        CLP_PARAM_ACTION_DUALSIMPLEX);
     parameters[numberParameters-1].setLonghelp
     (
          "This command solves the continuous relaxation of the current model using the dual steepest edge algorithm.\
The time and iterations may be affected by settings such as presolve, scaling, crash\
 and also by dual pivot method, fake bound on variables and dual and primal tolerances."
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("dualT!olerance", "For an optimal solution \
no dual infeasibility may exceed this value",
                        1.0e-20, 1.0e12, CLP_PARAM_DBL_DUALTOLERANCE);
     parameters[numberParameters-1].setLonghelp
     (
          "Normally the default tolerance is fine, but you may want to increase it a\
 bit if a dual run seems to be having a hard time.  One method which can be faster is \
to use a large tolerance e.g. 1.0e-4 and dual and then clean up problem using primal and the \
correct tolerance (remembering to switch off presolve for this final short clean up phase)."
     );
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("either!Simplex", "Do dual or primal simplex algorithm",
                        CLP_PARAM_ACTION_EITHERSIMPLEX);
     parameters[numberParameters-1].setLonghelp
     (
          "This command solves the continuous relaxation of the current model using the dual or primal algorithm,\
 based on a dubious analysis of model."
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("end", "Stops clp execution",
                        CLP_PARAM_ACTION_EXIT);
     parameters[numberParameters-1].setLonghelp
     (
          "This stops execution ; end, exit, quit and stop are synonyms"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("environ!ment", "Read commands from environment",
                        CLP_PARAM_ACTION_ENVIRONMENT, 7, 0);
     parameters[numberParameters-1].setLonghelp
     (
          "This starts reading from environment variable CBC_CLP_ENVIRONMENT."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("error!sAllowed", "Whether to allow import errors",
                        "off", CLP_PARAM_STR_ERRORSALLOWED, 3);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].setLonghelp
     (
          "The default is not to use any model which had errors when reading the mps file.\
  Setting this to 'on' will allow all errors from which the code can recover\
 simply by ignoring the error.  There are some errors from which the code can not recover \
e.g. no ENDATA.  This has to be set before import i.e. -errorsAllowed on -import xxxxxx.mps."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("exit", "Stops clp execution",
                        CLP_PARAM_ACTION_EXIT);
     parameters[numberParameters-1].setLonghelp
     (
          "This stops the execution of Clp, end, exit, quit and stop are synonyms"
     );
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("exper!iment", "Whether to use testing features",
                        -1, 200, CBC_PARAM_INT_EXPERIMENT, 0);
     parameters[numberParameters-1].setLonghelp
     (
          "Defines how adventurous you want to be in using new ideas. \
0 then no new ideas, 1 fairly sensible, 2 a bit dubious, 3 you are on your own!"
     );
     parameters[numberParameters-1].setIntValue(0);
     parameters[numberParameters++] =
          CbcOrClpParam("expensive!Strong", "Whether to do even more strong branching",
                        0, COIN_INT_MAX, CBC_PARAM_INT_STRONG_STRATEGY, 0);
     parameters[numberParameters-1].setLonghelp
     (
      "Strategy for extra strong branching \n\
\n\t0 - normal\n\
\n\twhen to do all fractional\n\
\n\t1 - root node\n\
\n\t2 - depth less than modifier\n\
\n\t4 - if objective == best possible\n\
\n\t6 - as 2+4\n\
\n\twhen to do all including satisfied\n\
\n\t10 - root node etc.\n\
\n\tIf >=100 then do when depth <= strategy/100 (otherwise 5)"
     );
     parameters[numberParameters-1].setIntValue(0);
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("export", "Export model as mps file",
                        CLP_PARAM_ACTION_EXPORT);
     parameters[numberParameters-1].setLonghelp
     (
          "This will write an MPS format file to the given file name.  It will use the default\
 directory given by 'directory'.  A name of '$' will use the previous value for the name.  This\
 is initialized to 'default.mps'.  \
It can be useful to get rid of the original names and go over to using Rnnnnnnn and Cnnnnnnn.  This can be done by setting 'keepnames' off before importing mps file."
     );
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("extra1", "Extra integer parameter 1",
                        -COIN_INT_MAX, COIN_INT_MAX, CBC_PARAM_INT_EXTRA1, 0);
     parameters[numberParameters-1].setIntValue(-1);
     parameters[numberParameters++] =
          CbcOrClpParam("extra2", "Extra integer parameter 2",
                        -100, COIN_INT_MAX, CBC_PARAM_INT_EXTRA2, 0);
     parameters[numberParameters-1].setIntValue(-1);
     parameters[numberParameters++] =
          CbcOrClpParam("extra3", "Extra integer parameter 3",
                        -1, COIN_INT_MAX, CBC_PARAM_INT_EXTRA3, 0);
     parameters[numberParameters-1].setIntValue(-1);
     parameters[numberParameters++] =
          CbcOrClpParam("extra4", "Extra integer parameter 4",
                        -1, COIN_INT_MAX, CBC_PARAM_INT_EXTRA4, 0);
     parameters[numberParameters-1].setIntValue(-1);
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on yet more special options!! \
The bottom digit is a strategy when to used shadow price stuff e.g. 3 \
means use until a solution is found.  The next two digits say what sort \
of dual information to use.  After that it goes back to powers of 2 so -\n\
\n\t1000 - switches on experimental hotstart\n\
\n\t2,4,6000 - switches on experimental methods of stopping cuts\n\
\n\t8000 - increase minimum drop gradually\n\
\n\t16000 - switches on alternate gomory criterion"
     );
     parameters[numberParameters++] =
       CbcOrClpParam("extraV!ariables", "Allow creation of extra integer variables",
		     -COIN_INT_MAX, COIN_INT_MAX, CBC_PARAM_INT_EXTRA_VARIABLES, 0);
     parameters[numberParameters-1].setIntValue(0);
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on creation of extra integer variables \
to gather all variables with same cost."
     );
#endif
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("fact!orization", "Which factorization to use",
                        "normal", CLP_PARAM_STR_FACTORIZATION);
     parameters[numberParameters-1].append("dense");
     parameters[numberParameters-1].append("simple");
     parameters[numberParameters-1].append("osl");
     parameters[numberParameters-1].setLonghelp
     (
#ifndef ABC_INHERIT
          "The default is to use the normal CoinFactorization, but \
other choices are a dense one, osl's or one designed for small problems."
#else
          "Normally the default is to use the normal CoinFactorization, but \
other choices are a dense one, osl's or one designed for small problems. \
However if at Aboca then the default is CoinAbcFactorization and other choices are \
a dense one, one designed for small problems or if enabled a long factorization."
#endif
     );
     parameters[numberParameters++] =
          CbcOrClpParam("fakeB!ound", "All bounds <= this value - DEBUG",
                        1.0, 1.0e15, CLP_PARAM_ACTION_FAKEBOUND, 0);
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("feas!ibilityPump", "Whether to try Feasibility Pump",
                        "off", CBC_PARAM_STR_FPUMP);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on feasibility pump heuristic at root. This is due to Fischetti, Lodi and Glover \
and uses a sequence of Lps to try and get an integer feasible solution. \
Some fine tuning is available by passFeasibilityPump and also pumpTune. \
See Rounding for meaning of on,both,before"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("fix!OnDj", "Try heuristic based on fixing variables with \
reduced costs greater than this",
                        -1.0e20, 1.0e20, CBC_PARAM_DBL_DJFIX, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "If this is set integer variables with reduced costs greater than this will be fixed \
before branch and bound - use with extreme caution!"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("flow!CoverCuts", "Whether to use Flow Cover cuts",
                        "off", CBC_PARAM_STR_FLOWCUTS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("root");
     parameters[numberParameters-1].append("ifmove");
     parameters[numberParameters-1].append("forceOn");
     parameters[numberParameters-1].append("onglobal");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on flow cover cuts (either at root or in entire tree) \
See branchAndCut for information on options."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("force!Solution", "Whether to use given solution as crash for BAB",
                        -1, 20000000, CLP_PARAM_INT_USESOLUTION);
     parameters[numberParameters-1].setIntValue(-1);
     parameters[numberParameters-1].setLonghelp
     (
          "-1 off.  If 1 then tries to branch to solution given by AMPL or priorities file. \
If 0 then just tries to set as best solution \
If >1 then also does that many nodes on fixed problem."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("fraction!forBAB", "Fraction in feasibility pump",
                        1.0e-5, 1.1, CBC_PARAM_DBL_SMALLBAB, 1);
     parameters[numberParameters-1].setDoubleValue(0.5);
     parameters[numberParameters-1].setLonghelp
     (
          "After a pass in feasibility pump, variables which have not moved \
about are fixed and if the preprocessed model is small enough a few nodes \
of branch and bound are done on reduced problem.  Small problem has to be less than this fraction of original."
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("gamma!(Delta)", "Whether to regularize barrier",
                        "off", CLP_PARAM_STR_GAMMA, 7, 1);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("gamma");
     parameters[numberParameters-1].append("delta");
     parameters[numberParameters-1].append("onstrong");
     parameters[numberParameters-1].append("gammastrong");
     parameters[numberParameters-1].append("deltastrong");
#endif
#ifdef COIN_HAS_CBC
      parameters[numberParameters++] =
          CbcOrClpParam("GMI!Cuts", "Whether to use alternative Gomory cuts",
                        "off", CBC_PARAM_STR_GMICUTS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("root");
     parameters[numberParameters-1].append("ifmove");
     parameters[numberParameters-1].append("forceOn");
     parameters[numberParameters-1].append("endonly");
     parameters[numberParameters-1].append("long");
     parameters[numberParameters-1].append("longroot");
     parameters[numberParameters-1].append("longifmove");
     parameters[numberParameters-1].append("forceLongOn");
     parameters[numberParameters-1].append("longendonly");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on an alternative Gomory cut generator (either at root or in entire tree) \
This version is by Giacomo Nannicini and may be more robust \
See branchAndCut for information on options."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("GMI!Cuts", "Whether to use alternative Gomory cuts",
                        "off", CBC_PARAM_STR_GMICUTS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("root");
     parameters[numberParameters-1].append("ifmove");
     parameters[numberParameters-1].append("forceOn");
     parameters[numberParameters-1].append("endonly");
     parameters[numberParameters-1].append("long");
     parameters[numberParameters-1].append("longroot");
     parameters[numberParameters-1].append("longifmove");
     parameters[numberParameters-1].append("forceLongOn");
     parameters[numberParameters-1].append("longendonly");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on an alternative Gomory cut generator (either at root or in entire tree) \
This version is by Giacomo Nannicini and may be more robust \
See branchAndCut for information on options."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("gomory!Cuts", "Whether to use Gomory cuts",
                        "off", CBC_PARAM_STR_GOMORYCUTS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("root");
     parameters[numberParameters-1].append("ifmove");
     parameters[numberParameters-1].append("forceOn");
     parameters[numberParameters-1].append("onglobal");
     parameters[numberParameters-1].append("forceandglobal");
     parameters[numberParameters-1].append("forceLongOn");
     parameters[numberParameters-1].append("long");
     parameters[numberParameters-1].setLonghelp
     (
          "The original cuts - beware of imitations!  Having gone out of favor, they are now more \
fashionable as LP solvers are more robust and they interact well with other cuts.  They will almost always \
give cuts (although in this executable they are limited as to number of variables in cut).  \
However the cuts may be dense so it is worth experimenting (Long allows any length). \
See branchAndCut for information on options."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("greedy!Heuristic", "Whether to use a greedy heuristic",
                        "off", CBC_PARAM_STR_GREEDY);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     //parameters[numberParameters-1].append("root");
     parameters[numberParameters-1].setLonghelp
     (
          "Switches on a greedy heuristic which will try and obtain a solution.  It may just fix a \
percentage of variables and then try a small branch and cut run. \
See Rounding for meaning of on,both,before"
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("gsolu!tion", "Puts glpk solution to file",
                        CLP_PARAM_ACTION_GMPL_SOLUTION);
     parameters[numberParameters-1].setLonghelp
     (
          "Will write a glpk solution file to the given file name.  It will use the default\
 directory given by 'directory'.  A name of '$' will use the previous value for the name.  This\
 is initialized to 'stdout' (this defaults to ordinary solution if stdout). \
If problem created from gmpl model - will do any reports."
     );
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("heur!isticsOnOff", "Switches most heuristics on or off",
                        "off", CBC_PARAM_STR_HEURISTICSTRATEGY);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].setLonghelp
     (
          "This can be used to switch on or off all heuristics.  Then you can do \
individual ones off or on.  CbcTreeLocal is not included as it dramatically \
alters search."
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("help", "Print out version, non-standard options and some help",
                        CLP_PARAM_ACTION_HELP, 3);
     parameters[numberParameters-1].setLonghelp
     (
          "This prints out some help to get user started.  If you have printed this then \
you should be past that stage:-)"
     );
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("hOp!tions", "Heuristic options",
                        -9999999, 9999999, CBC_PARAM_INT_HOPTIONS, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "1 says stop heuristic immediately allowable gap reached. \
Others are for feasibility pump - \
2 says do exact number of passes given, \
4 only applies if initial cutoff given and says relax after 50 passes, \
while 8 will adapt cutoff rhs after first solution if it looks as if code is stalling."
     );
     parameters[numberParameters-1].setIntValue(0);
     parameters[numberParameters++] =
          CbcOrClpParam("hot!StartMaxIts", "Maximum iterations on hot start",
                        0, COIN_INT_MAX, CBC_PARAM_INT_MAXHOTITS);
#endif
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("idiot!Crash", "Whether to try idiot crash",
                        -1, 99999999, CLP_PARAM_INT_IDIOT);
     parameters[numberParameters-1].setLonghelp
     (
          "This is a type of 'crash' which works well on some homogeneous problems.\
 It works best on problems with unit elements and rhs but will do something to any model.  It should only be\
 used before primal.  It can be set to -1 when the code decides for itself whether to use it,\
 0 to switch off or n > 0 to do n passes."
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("import", "Import model from mps file",
                        CLP_PARAM_ACTION_IMPORT, 3);
     parameters[numberParameters-1].setLonghelp
     (
          "This will read an MPS format file from the given file name.  It will use the default\
 directory given by 'directory'.  A name of '$' will use the previous value for the name.  This\
 is initialized to '', i.e. it must be set.  If you have libgz then it can read compressed\
 files 'xxxxxxxx.gz' or 'xxxxxxxx.bz2'.  \
If 'keepnames' is off, then names are dropped -> Rnnnnnnn and Cnnnnnnn."
     );
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("inc!rement", "A valid solution must be at least this \
much better than last integer solution",
                        -1.0e20, 1.0e20, CBC_PARAM_DBL_INCREMENT);
     parameters[numberParameters-1].setLonghelp
     (
          "Whenever a solution is found the bound on solutions is set to solution (in a minimization\
sense) plus this.  If it is not set then the code will try and work one out e.g. if \
all objective coefficients are multiples of 0.01 and only integer variables have entries in \
objective then this can be set to 0.01.  Be careful if you set this negative!"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("inf!easibilityWeight", "Each integer infeasibility is expected \
to cost this much",
                        0.0, 1.0e20, CBC_PARAM_DBL_INFEASIBILITYWEIGHT, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "A primitive way of deciding which node to explore next.  Satisfying each integer infeasibility is \
expected to cost this much."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("initialS!olve", "Solve to continuous",
                        CLP_PARAM_ACTION_SOLVECONTINUOUS);
     parameters[numberParameters-1].setLonghelp
     (
          "This just solves the problem to continuous - without adding any cuts"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("integerT!olerance", "For an optimal solution \
no integer variable may be this away from an integer value",
                        1.0e-20, 0.5, CBC_PARAM_DBL_INTEGERTOLERANCE);
     parameters[numberParameters-1].setLonghelp
     (
          "Beware of setting this smaller than the primal tolerance."
     );
#endif
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("keepN!ames", "Whether to keep names from import",
                        "on", CLP_PARAM_STR_KEEPNAMES);
     parameters[numberParameters-1].append("off");
     parameters[numberParameters-1].setLonghelp
     (
          "It saves space to get rid of names so if you need to you can set this to off.  \
This needs to be set before the import of model - so -keepnames off -import xxxxx.mps."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("KKT", "Whether to use KKT factorization",
                        "off", CLP_PARAM_STR_KKT, 7, 1);
     parameters[numberParameters-1].append("on");
#endif
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("knapsack!Cuts", "Whether to use Knapsack cuts",
                        "off", CBC_PARAM_STR_KNAPSACKCUTS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("root");
     parameters[numberParameters-1].append("ifmove");
     parameters[numberParameters-1].append("forceOn");
     parameters[numberParameters-1].append("onglobal");
     parameters[numberParameters-1].append("forceandglobal");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on knapsack cuts (either at root or in entire tree) \
See branchAndCut for information on options."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("lagomory!Cuts", "Whether to use Lagrangean Gomory cuts",
                        "off", CBC_PARAM_STR_LAGOMORYCUTS);
     parameters[numberParameters-1].append("endonlyroot");
     parameters[numberParameters-1].append("endcleanroot");
     parameters[numberParameters-1].append("endbothroot");
     parameters[numberParameters-1].append("endonly");
     parameters[numberParameters-1].append("endclean");
     parameters[numberParameters-1].append("endboth");
     parameters[numberParameters-1].append("onlyaswell");
     parameters[numberParameters-1].append("cleanaswell");
     parameters[numberParameters-1].append("bothaswell");
     parameters[numberParameters-1].append("onlyinstead");
     parameters[numberParameters-1].append("cleaninstead");
     parameters[numberParameters-1].append("bothinstead");
     parameters[numberParameters-1].setLonghelp
     (
          "This is a gross simplification of 'A Relax-and-Cut Framework for Gomory's Mixed-Integer Cuts' \
by Matteo Fischetti & Domenico Salvagnin.  This simplification \
just uses original constraints while modifying objective using other cuts. \
So you don't use messy constraints generated by Gomory etc. \
A variant is to allow non messy cuts e.g. clique cuts. \
So 'only' does this while clean also allows integral valued cuts.  \
'End' is recommended which waits until other cuts have finished and then \
does a few passes. \
The length options for gomory cuts are used."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("latwomir!Cuts", "Whether to use Lagrangean TwoMir cuts",
                        "off", CBC_PARAM_STR_LATWOMIRCUTS);
     parameters[numberParameters-1].append("endonlyroot");
     parameters[numberParameters-1].append("endcleanroot");
     parameters[numberParameters-1].append("endbothroot");
     parameters[numberParameters-1].append("endonly");
     parameters[numberParameters-1].append("endclean");
     parameters[numberParameters-1].append("endboth");
     parameters[numberParameters-1].append("onlyaswell");
     parameters[numberParameters-1].append("cleanaswell");
     parameters[numberParameters-1].append("bothaswell");
     parameters[numberParameters-1].append("onlyinstead");
     parameters[numberParameters-1].append("cleaninstead");
     parameters[numberParameters-1].append("bothinstead");
     parameters[numberParameters-1].setLonghelp
     (
          "This is a lagrangean relaxation for TwoMir cuts.  See \
lagomoryCuts for description of options."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("lift!AndProjectCuts", "Whether to use Lift and Project cuts",
                        "off", CBC_PARAM_STR_LANDPCUTS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("root");
     parameters[numberParameters-1].append("ifmove");
     parameters[numberParameters-1].append("forceOn");
     parameters[numberParameters-1].setLonghelp
     (
          "Lift and project cuts. \
May be slow \
See branchAndCut for information on options."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("local!TreeSearch", "Whether to use local treesearch",
                        "off", CBC_PARAM_STR_LOCALTREE);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on a local search algorithm when a solution is found.  This is from \
Fischetti and Lodi and is not really a heuristic although it can be used as one. \
When used from Coin solve it has limited functionality.  It is not switched on when \
heuristics are switched on."
     );
#endif
#ifndef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("log!Level", "Level of detail in Solver output",
                        -1, 999999, CLP_PARAM_INT_SOLVERLOGLEVEL);
#else
     parameters[numberParameters++] =
          CbcOrClpParam("log!Level", "Level of detail in Coin branch and Cut output",
                        -63, 63, CLP_PARAM_INT_LOGLEVEL);
     parameters[numberParameters-1].setIntValue(1);
#endif
     parameters[numberParameters-1].setLonghelp
     (
          "If 0 then there should be no output in normal circumstances.  1 is probably the best\
 value for most uses, while 2 and 3 give more information."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("max!imize", "Set optimization direction to maximize",
                        CLP_PARAM_ACTION_MAXIMIZE, 7);
     parameters[numberParameters-1].setLonghelp
     (
          "The default is minimize - use 'maximize' for maximization.\n\
You can also use the parameters 'direction maximize'."
     );
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("maxF!actor", "Maximum number of iterations between \
refactorizations",
                        1, 999999, CLP_PARAM_INT_MAXFACTOR);
     parameters[numberParameters-1].setLonghelp
     (
          "If this is at its initial value of 200 then in this executable clp will guess at a\
 value to use.  Otherwise the user can set a value.  The code may decide to re-factorize\
 earlier for accuracy."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("maxIt!erations", "Maximum number of iterations before \
stopping",
                        0, 2147483647, CLP_PARAM_INT_MAXITERATION);
     parameters[numberParameters-1].setLonghelp
     (
          "This can be used for testing purposes.  The corresponding library call\n\
      \tsetMaximumIterations(value)\n can be useful.  If the code stops on\
 seconds or by an interrupt this will be treated as stopping on maximum iterations.  This is ignored in branchAndCut - use maxN!odes."
     );
#endif
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("maxN!odes", "Maximum number of nodes to do",
                        -1, 2147483647, CBC_PARAM_INT_MAXNODES);
     parameters[numberParameters-1].setLonghelp 
     (
          "This is a repeatable way to limit search.  Normally using time is easier \
but then the results may not be repeatable."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("maxSaved!Solutions", "Maximum number of solutions to save",
                        0, 2147483647, CBC_PARAM_INT_MAXSAVEDSOLS);
     parameters[numberParameters-1].setLonghelp
     (
          "Number of solutions to save."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("maxSo!lutions", "Maximum number of solutions to get",
                        1, 2147483647, CBC_PARAM_INT_MAXSOLS);
     parameters[numberParameters-1].setLonghelp
     (
          "You may want to stop after (say) two solutions or an hour.  \
This is checked every node in tree, so it is possible to get more solutions from heuristics."
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("min!imize", "Set optimization direction to minimize",
                        CLP_PARAM_ACTION_MINIMIZE, 7);
     parameters[numberParameters-1].setLonghelp
     (
          "The default is minimize - use 'maximize' for maximization.\n\
This should only be necessary if you have previously set maximization \
You can also use the parameters 'direction minimize'."
     );
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("mipO!ptions", "Dubious options for mip",
                        0, COIN_INT_MAX, CBC_PARAM_INT_MIPOPTIONS, 0);
     parameters[numberParameters++] =
          CbcOrClpParam("more!MipOptions", "More dubious options for mip",
                        -1, COIN_INT_MAX, CBC_PARAM_INT_MOREMIPOPTIONS, 0);
     parameters[numberParameters++] =
          CbcOrClpParam("mixed!IntegerRoundingCuts", "Whether to use Mixed Integer Rounding cuts",
                        "off", CBC_PARAM_STR_MIXEDCUTS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("root");
     parameters[numberParameters-1].append("ifmove");
     parameters[numberParameters-1].append("forceOn");
     parameters[numberParameters-1].append("onglobal");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on mixed integer rounding cuts (either at root or in entire tree) \
See branchAndCut for information on options."
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("mess!ages", "Controls if Clpnnnn is printed",
                        "off", CLP_PARAM_STR_MESSAGES);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].setLonghelp
     ("The default behavior is to put out messages such as:\n\
   Clp0005 2261  Objective 109.024 Primal infeas 944413 (758)\n\
but this program turns this off to make it look more friendly.  It can be useful\
 to turn them back on if you want to be able to 'grep' for particular messages or if\
 you intend to override the behavior of a particular message.  This only affects Clp not Cbc."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("miplib", "Do some of miplib test set",
                        CBC_PARAM_ACTION_MIPLIB, 3, 1);
#ifdef COIN_HAS_CBC
      parameters[numberParameters++] =
          CbcOrClpParam("mips!tart", "reads an initial feasible solution from file",
                        CBC_PARAM_ACTION_MIPSTART);
     parameters[numberParameters-1].setLonghelp
     ("\
The MIPStart allows one to enter an initial integer feasible solution \
to CBC. Values of the main decision variables which are active (have \
non-zero values) in this solution are specified in a text  file. The \
text file format used is the same of the solutions saved by CBC, but \
not all fields are required to be filled. First line may contain the \
solution status and will be ignored, remaining lines contain column \
indexes, names and values as in this example:\n\
\n\
Stopped on iterations - objective value 57597.00000000\n\
      0  x(1,1,2,2)               1 \n\
      1  x(3,1,3,2)               1 \n\
      5  v(5,1)                   2 \n\
      33 x(8,1,5,2)               1 \n\
      ...\n\
\n\
Column indexes are also ignored since pre-processing can change them. \
There is no need to include values for continuous or integer auxiliary \
variables, since they can be computed based on main decision variables. \
Starting CBC with an integer feasible solution can dramatically improve \
its performance: several MIP heuristics (e.g. RINS) rely on having at \
least one feasible solution available and can start immediately if the \
user provides one. Feasibility Pump (FP) is a heuristic which tries to \
overcome the problem of taking too long to find feasible solution (or \
not finding at all), but it not always succeeds. If you provide one \
starting solution you will probably save some time by disabling FP. \
\n\n\
Knowledge specific to your problem can be considered to write an \
external module to quickly produce an initial feasible solution - some \
alternatives are the implementation of simple greedy heuristics or the \
solution (by CBC for example) of a simpler model created just to find \
a feasible solution. \
\n\n\
Question and suggestions regarding MIPStart can be directed to\n\
haroldo.santos@gmail.com.\
");
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("moreS!pecialOptions", "Yet more dubious options for Simplex - see ClpSimplex.hpp",
                        0, COIN_INT_MAX, CLP_PARAM_INT_MORESPECIALOPTIONS, 0);
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("moreT!une", "Yet more dubious ideas for feasibility pump",
                        0, 100000000, CBC_PARAM_INT_FPUMPTUNE2, 0);
     parameters[numberParameters-1].setLonghelp
     (
          "Yet more ideas for Feasibility Pump \n\
\t/100000 == 1 use box constraints and original obj in cleanup\n\
\t/1000 == 1 Pump will run twice if no solution found\n\
\t/1000 == 2 Pump will only run after root cuts if no solution found\n\
\t/1000 >10 as above but even if solution found\n\
\t/100 == 1,3.. exact 1.0 for objective values\n\
\t/100 == 2,3.. allow more iterations per pass\n\
\t n fix if value of variable same for last n iterations."
     );
     parameters[numberParameters-1].setIntValue(0);
     parameters[numberParameters++] =
          CbcOrClpParam("multiple!RootPasses", "Do multiple root passes to collect cuts and solutions",
                        0, 100000000, CBC_PARAM_INT_MULTIPLEROOTS, 0);
     parameters[numberParameters-1].setIntValue(0);
     parameters[numberParameters-1].setLonghelp
     (
          "Do (in parallel if threads enabled) the root phase this number of times \
 and collect all solutions and cuts generated.  The actual format is aabbcc \
where aa is number of extra passes, if bb is non zero \
then it is number of threads to use (otherwise uses threads setting) and \
cc is number of times to do root phase.  Yet another one from the Italian idea factory \
(This time - Andrea Lodi , Matteo Fischetti , Michele Monaci , Domenico Salvagnin , \
and Andrea Tramontani). \
The solvers do not interact with each other.  However if extra passes are specified \
then cuts are collected and used in later passes - so there is interaction there." 
     );
     parameters[numberParameters++] =
          CbcOrClpParam("naive!Heuristics", "Whether to try some stupid heuristic",
                        "off", CBC_PARAM_STR_NAIVE, 7, 1);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters-1].setLonghelp
     (
          "Really silly stuff e.g. fix all integers with costs to zero!. \
Doh option does heuristic before preprocessing"     );
#endif
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("netlib", "Solve entire netlib test set",
                        CLP_PARAM_ACTION_NETLIB_EITHER, 3, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "This exercises the unit test for clp and then solves the netlib test set using dual or primal.\
The user can set options before e.g. clp -presolve off -netlib"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("netlibB!arrier", "Solve entire netlib test set with barrier",
                        CLP_PARAM_ACTION_NETLIB_BARRIER, 3, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "This exercises the unit test for clp and then solves the netlib test set using barrier.\
The user can set options before e.g. clp -kkt on -netlib"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("netlibD!ual", "Solve entire netlib test set (dual)",
                        CLP_PARAM_ACTION_NETLIB_DUAL, 3, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "This exercises the unit test for clp and then solves the netlib test set using dual.\
The user can set options before e.g. clp -presolve off -netlib"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("netlibP!rimal", "Solve entire netlib test set (primal)",
                        CLP_PARAM_ACTION_NETLIB_PRIMAL, 3, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "This exercises the unit test for clp and then solves the netlib test set using primal.\
The user can set options before e.g. clp -presolve off -netlibp"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("netlibT!une", "Solve entire netlib test set with 'best' algorithm",
                        CLP_PARAM_ACTION_NETLIB_TUNE, 3, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "This exercises the unit test for clp and then solves the netlib test set using whatever \
works best.  I know this is cheating but it also stresses the code better by doing a \
mixture of stuff.  The best algorithm was chosen on a Linux ThinkPad using native cholesky \
with University of Florida ordering."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("network", "Tries to make network matrix",
                        CLP_PARAM_ACTION_NETWORK, 7, 0);
     parameters[numberParameters-1].setLonghelp
     (
          "Clp will go faster if the matrix can be converted to a network.  The matrix\
 operations may be a bit faster with more efficient storage, but the main advantage\
 comes from using a network factorization.  It will probably not be as fast as a \
specialized network code."
     );
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("nextB!estSolution", "Prints next best saved solution to file",
                        CLP_PARAM_ACTION_NEXTBESTSOLUTION);
     parameters[numberParameters-1].setLonghelp
     (
          "To write best solution, just use solution.  This prints next best (if exists) \
 and then deletes it. \
 This will write a primitive solution file to the given file name.  It will use the default\
 directory given by 'directory'.  A name of '$' will use the previous value for the name.  This\
 is initialized to 'stdout'.  The amount of output can be varied using printi!ngOptions or printMask."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("node!Strategy", "What strategy to use to select nodes",
                        "hybrid", CBC_PARAM_STR_NODESTRATEGY);
     parameters[numberParameters-1].append("fewest");
     parameters[numberParameters-1].append("depth");
     parameters[numberParameters-1].append("upfewest");
     parameters[numberParameters-1].append("downfewest");
     parameters[numberParameters-1].append("updepth");
     parameters[numberParameters-1].append("downdepth");
     parameters[numberParameters-1].setLonghelp
     (
          "Normally before a solution the code will choose node with fewest infeasibilities. \
You can choose depth as the criterion.  You can also say if up or down branch must \
be done first (the up down choice will carry on after solution). \
Default has now been changed to hybrid which is breadth first on small depth nodes then fewest."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("numberA!nalyze", "Number of analysis iterations",
                        -COIN_INT_MAX, COIN_INT_MAX, CBC_PARAM_INT_NUMBERANALYZE, 0);
     parameters[numberParameters-1].setLonghelp
     (
          "This says how many iterations to spend at root node analyzing problem. \
This is a first try and will hopefully become more sophisticated."
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("objective!Scale", "Scale factor to apply to objective",
                        -1.0e20, 1.0e20, CLP_PARAM_DBL_OBJSCALE, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "If the objective function has some very large values, you may wish to scale them\
 internally by this amount.  It can also be set by autoscale.  It is applied after scaling.  You are unlikely to need this."
     );
     parameters[numberParameters-1].setDoubleValue(1.0);
#endif
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("outDup!licates", "takes duplicate rows etc out of integer model",
                        CLP_PARAM_ACTION_OUTDUPROWS, 7, 0);
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("output!Format", "Which output format to use",
                        1, 6, CLP_PARAM_INT_OUTPUTFORMAT);
     parameters[numberParameters-1].setLonghelp
     (
          "Normally export will be done using normal representation for numbers and two values\
 per line.  You may want to do just one per line (for grep or suchlike) and you may wish\
 to save with absolute accuracy using a coded version of the IEEE value. A value of 2 is normal.\
 otherwise odd values gives one value per line, even two.  Values 1,2 give normal format, 3,4\
 gives greater precision, while 5,6 give IEEE values.  When used for exporting a basis 1 does not save \
values, 2 saves values, 3 with greater accuracy and 4 in IEEE."
     );
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("para!metrics", "Import data from file and do parametrics",
                        CLP_PARAM_ACTION_PARAMETRICS, 3);
     parameters[numberParameters-1].setLonghelp
     (
          "This will read a file with parametric data from the given file name \
and then do parametrics.  It will use the default\
 directory given by 'directory'.  A name of '$' will use the previous value for the name.  This\
 is initialized to '', i.e. it must be set.  This can not read from compressed files. \
File is in modified csv format - a line ROWS will be followed by rows data \
while a line COLUMNS will be followed by column data.  The last line \
should be ENDATA. The ROWS line must exist and is in the format \
ROWS, inital theta, final theta, interval theta, n where n is 0 to get \
CLPI0062 message at interval or at each change of theta \
and 1 to get CLPI0063 message at each iteration.  If interval theta is 0.0 \
or >= final theta then no interval reporting.  n may be missed out when it is \
taken as 0.  If there is Row data then \
there is a headings line with allowed headings - name, number, \
lower(rhs change), upper(rhs change), rhs(change).  Either the lower and upper \
fields should be given or the rhs field. \
The optional COLUMNS line is followed by a headings line with allowed \
headings - name, number, objective(change), lower(change), upper(change). \
 Exactly one of name and number must be given for either section and \
missing ones have value 0.0."
     );
#endif
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("passC!uts", "Number of cut passes at root node",
                        -9999999, 9999999, CBC_PARAM_INT_CUTPASS);
     parameters[numberParameters-1].setLonghelp
     (
          "The default is 100 passes if less than 500 columns, 100 passes (but \
stop if drop small if less than 5000 columns, 20 otherwise"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("passF!easibilityPump", "How many passes in feasibility pump",
                        0, 10000, CBC_PARAM_INT_FPUMPITS);
     parameters[numberParameters-1].setLonghelp
     (
          "This fine tunes Feasibility Pump by doing more or fewer passes."
     );
     parameters[numberParameters-1].setIntValue(20);
#endif
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("passP!resolve", "How many passes in presolve",
                        -200, 100, CLP_PARAM_INT_PRESOLVEPASS, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "Normally Presolve does 5 passes but you may want to do less to make it\
 more lightweight or do more if improvements are still being made.  As Presolve will return\
 if nothing is being taken out, you should not normally need to use this fine tuning."
     );
#endif
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("passT!reeCuts", "Number of cut passes in tree",
                        -9999999, 9999999, CBC_PARAM_INT_CUTPASSINTREE);
     parameters[numberParameters-1].setLonghelp
     (
          "The default is one pass"
     );
#endif
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("pertV!alue", "Method of perturbation",
                        -5000, 102, CLP_PARAM_INT_PERTVALUE, 1);
     parameters[numberParameters++] =
          CbcOrClpParam("perturb!ation", "Whether to perturb problem",
                        "on", CLP_PARAM_STR_PERTURBATION);
     parameters[numberParameters-1].append("off");
     parameters[numberParameters-1].setLonghelp
     (
          "Perturbation helps to stop cycling, but Clp uses other measures for this.\
  However large problems and especially ones with unit elements and unit rhs or costs\
 benefit from perturbation.  Normally Clp tries to be intelligent, but you can switch this off.\
  The Clp library has this off by default.  This program has it on by default."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("PFI", "Whether to use Product Form of Inverse in simplex",
                        "off", CLP_PARAM_STR_PFI, 7, 0);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].setLonghelp
     (
          "By default clp uses Forrest-Tomlin L-U update.  If you are masochistic you can switch it off."
     );
#endif
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("pivotAndC!omplement", "Whether to try Pivot and Complement heuristic",
                        "off", CBC_PARAM_STR_PIVOTANDCOMPLEMENT);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters-1].setLonghelp
     (
          "stuff needed. \
Doh option does heuristic before preprocessing"     );
     parameters[numberParameters++] =
          CbcOrClpParam("pivotAndF!ix", "Whether to try Pivot and Fix heuristic",
                        "off", CBC_PARAM_STR_PIVOTANDFIX);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters-1].setLonghelp
     (
          "stuff needed. \
Doh option does heuristic before preprocessing"     );
#endif
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("plus!Minus", "Tries to make +- 1 matrix",
                        CLP_PARAM_ACTION_PLUSMINUS, 7, 0);
     parameters[numberParameters-1].setLonghelp
     (
          "Clp will go slightly faster if the matrix can be converted so that the elements are\
 not stored and are known to be unit.  The main advantage is memory use.  Clp may automatically\
 see if it can convert the problem so you should not need to use this."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("pO!ptions", "Dubious print options",
                        0, COIN_INT_MAX, CLP_PARAM_INT_PRINTOPTIONS, 1);
     parameters[numberParameters-1].setIntValue(0);
     parameters[numberParameters-1].setLonghelp
     (
          "If this is > 0 then presolve will give more information and branch and cut will give statistics"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("preO!pt", "Presolve options",
                        0, COIN_INT_MAX, CLP_PARAM_INT_PRESOLVEOPTIONS, 0);
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("presolve", "Whether to presolve problem",
                        "on", CLP_PARAM_STR_PRESOLVE);
     parameters[numberParameters-1].append("off");
     parameters[numberParameters-1].append("more");
     parameters[numberParameters-1].append("file");
     parameters[numberParameters-1].setLonghelp
     (
          "Presolve analyzes the model to find such things as redundant equations, equations\
 which fix some variables, equations which can be transformed into bounds etc etc.  For the\
 initial solve of any problem this is worth doing unless you know that it will have no effect.  \
on will normally do 5 passes while using 'more' will do 10.  If the problem is very large you may need \
to write the original to file using 'file'."
     );
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("preprocess", "Whether to use integer preprocessing",
                        "off", CBC_PARAM_STR_PREPROCESS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("save");
     parameters[numberParameters-1].append("equal");
     parameters[numberParameters-1].append("sos");
     parameters[numberParameters-1].append("trysos");
     parameters[numberParameters-1].append("equalall");
     parameters[numberParameters-1].append("strategy");
     parameters[numberParameters-1].append("aggregate");
     parameters[numberParameters-1].append("forcesos");
     parameters[numberParameters-1].setLonghelp
     (
          "This tries to reduce size of model in a similar way to presolve and \
it also tries to strengthen the model - this can be very useful and is worth trying. \
 Save option saves on file presolved.mps.  equal will turn <= cliques into \
==.  sos will create sos sets if all 0-1 in sets (well one extra is allowed) \
and no overlaps.  trysos is same but allows any number extra.  equalall will turn all \
valid inequalities into equalities with integer slacks.  strategy is as \
on but uses CbcStrategy."
     );
#endif
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("preT!olerance", "Tolerance to use in presolve",
                        1.0e-20, 1.0e12, CLP_PARAM_DBL_PRESOLVETOLERANCE);
     parameters[numberParameters-1].setLonghelp
     (
          "The default is 1.0e-8 - you may wish to try 1.0e-7 if presolve says the problem is \
infeasible and you have awkward numbers and you are sure the problem is really feasible."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("primalP!ivot", "Primal pivot choice algorithm",
                        "auto!matic", CLP_PARAM_STR_PRIMALPIVOT, 7, 1);
     parameters[numberParameters-1].append("exa!ct");
     parameters[numberParameters-1].append("dant!zig");
     parameters[numberParameters-1].append("part!ial");
     parameters[numberParameters-1].append("steep!est");
     parameters[numberParameters-1].append("change");
     parameters[numberParameters-1].append("sprint");
     parameters[numberParameters-1].setLonghelp
     (
          "Clp can use any pivot selection algorithm which the user codes as long as it\
 implements the features in the abstract pivot base class.  The Dantzig method is implemented\
 to show a simple method but its use is deprecated.  Exact devex is the method of choice and there\
 are two variants which keep all weights updated but only scan a subset each iteration.\
 Partial switches this on while change initially does dantzig until the factorization\
 becomes denser.  This is still a work in progress."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("primalS!implex", "Do primal simplex algorithm",
                        CLP_PARAM_ACTION_PRIMALSIMPLEX);
     parameters[numberParameters-1].setLonghelp
     (
          "This command solves the continuous relaxation of the current model using the primal algorithm.\
  The default is to use exact devex.\
 The time and iterations may be affected by settings such as presolve, scaling, crash\
 and also by column selection  method, infeasibility weight and dual and primal tolerances."
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("primalT!olerance", "For an optimal solution \
no primal infeasibility may exceed this value",
                        1.0e-20, 1.0e12, CLP_PARAM_DBL_PRIMALTOLERANCE);
     parameters[numberParameters-1].setLonghelp
     (
          "Normally the default tolerance is fine, but you may want to increase it a\
 bit if a primal run seems to be having a hard time"
     );
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("primalW!eight", "Initially algorithm acts as if it \
costs this much to be infeasible",
                        1.0e-20, 1.0e20, CLP_PARAM_DBL_PRIMALWEIGHT);
     parameters[numberParameters-1].setLonghelp
     (
          "The primal algorithm in Clp is a single phase algorithm as opposed to a two phase\
 algorithm where you first get feasible then optimal.  So Clp is minimizing this weight times\
 the sum of primal infeasibilities plus the true objective function (in minimization sense).\
  Too high a value may mean more iterations, while too low a bound means\
 the code may go all the way and then have to increase the weight in order to get feasible.\
  OSL had a heuristic to\
 adjust bounds, maybe we need that here."
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("printi!ngOptions", "Print options",
                        "normal", CLP_PARAM_STR_INTPRINT, 3);
     parameters[numberParameters-1].append("integer");
     parameters[numberParameters-1].append("special");
     parameters[numberParameters-1].append("rows");
     parameters[numberParameters-1].append("all");
     parameters[numberParameters-1].append("csv");
     parameters[numberParameters-1].append("bound!ranging");
     parameters[numberParameters-1].append("rhs!ranging");
     parameters[numberParameters-1].append("objective!ranging");
     parameters[numberParameters-1].append("stats");
     parameters[numberParameters-1].append("boundsint");
     parameters[numberParameters-1].append("boundsall");
     parameters[numberParameters-1].setLonghelp
     (
          "This changes the amount and format of printing a solution:\nnormal - nonzero column variables \n\
integer - nonzero integer column variables\n\
special - in format suitable for OsiRowCutDebugger\n\
rows - nonzero column variables and row activities\n\
all - all column variables and row activities.\n\
\nFor non-integer problems 'integer' and 'special' act like 'normal'.  \
Also see printMask for controlling output."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("printM!ask", "Control printing of solution on a  mask",
                        CLP_PARAM_ACTION_PRINTMASK, 3);
     parameters[numberParameters-1].setLonghelp
     (
          "If set then only those names which match mask are printed in a solution. \
'?' matches any character and '*' matches any set of characters. \
 The default is '' i.e. unset so all variables are printed. \
This is only active if model has names."
     );
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("prio!rityIn", "Import priorities etc from file",
                        CBC_PARAM_ACTION_PRIORITYIN, 3);
     parameters[numberParameters-1].setLonghelp
     (
          "This will read a file with priorities from the given file name.  It will use the default\
 directory given by 'directory'.  A name of '$' will use the previous value for the name.  This\
 is initialized to '', i.e. it must be set.  This can not read from compressed files. \
File is in csv format with allowed headings - name, number, priority, direction, up, down, solution.  Exactly one of\
 name and number must be given."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("probing!Cuts", "Whether to use Probing cuts",
                        "off", CBC_PARAM_STR_PROBINGCUTS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("root");
     parameters[numberParameters-1].append("ifmove");
     parameters[numberParameters-1].append("forceOn");
     parameters[numberParameters-1].append("onglobal");
     parameters[numberParameters-1].append("forceonglobal");
     parameters[numberParameters-1].append("forceOnBut");
     parameters[numberParameters-1].append("forceOnStrong");
     parameters[numberParameters-1].append("forceOnButStrong");
     parameters[numberParameters-1].append("strongRoot");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on probing cuts (either at root or in entire tree) \
See branchAndCut for information on options. \
but strong options do more probing"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("proximity!Search", "Whether to do proximity search heuristic",
                        "off", CBC_PARAM_STR_PROXIMITY);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters-1].append("10");
     parameters[numberParameters-1].append("100");
     parameters[numberParameters-1].append("300");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on a heuristic which looks for a solution close \
to incumbent solution (Fischetti and Monaci). \
See Rounding for meaning of on,both,before. \
The ones at end have different maxNode settings (and are 'on'(on==30))."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("pumpC!utoff", "Fake cutoff for use in feasibility pump",
                        -COIN_DBL_MAX, COIN_DBL_MAX, CBC_PARAM_DBL_FAKECUTOFF);
     parameters[numberParameters-1].setDoubleValue(0.0);
     parameters[numberParameters-1].setLonghelp
     (
          "0.0 off - otherwise add a constraint forcing objective below this value\
 in feasibility pump"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("pumpI!ncrement", "Fake increment for use in feasibility pump",
                        -COIN_DBL_MAX, COIN_DBL_MAX, CBC_PARAM_DBL_FAKEINCREMENT, 1);
     parameters[numberParameters-1].setDoubleValue(0.0);
     parameters[numberParameters-1].setLonghelp
     (
          "0.0 off - otherwise use as absolute increment to cutoff \
when solution found in feasibility pump"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("pumpT!une", "Dubious ideas for feasibility pump",
                        0, 100000000, CBC_PARAM_INT_FPUMPTUNE);
     parameters[numberParameters-1].setLonghelp
     (
          "This fine tunes Feasibility Pump \n\
\t>=10000000 use as objective weight switch\n\
\t>=1000000 use as accumulate switch\n\
\t>=1000 use index+1 as number of large loops\n\
\t==100 use objvalue +0.05*fabs(objvalue) as cutoff OR fakeCutoff if set\n\
\t%100 == 10,20 affects how each solve is done\n\
\t1 == fix ints at bounds, 2 fix all integral ints, 3 and continuous at bounds. \
If accumulate is on then after a major pass, variables which have not moved \
are fixed and a small branch and bound is tried."
     );
     parameters[numberParameters-1].setIntValue(0);
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("quit", "Stops clp execution",
                        CLP_PARAM_ACTION_EXIT);
     parameters[numberParameters-1].setLonghelp
     (
          "This stops the execution of Clp, end, exit, quit and stop are synonyms"
     );
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("randomC!bcSeed", "Random seed for Cbc",
                        -1, COIN_INT_MAX, CBC_PARAM_INT_RANDOMSEED);
     parameters[numberParameters-1].setLonghelp
     (
          "This sets a random seed for Cbc \
- 0 says use time of day, -1 is as now."
     );
     parameters[numberParameters-1].setIntValue(-1);
     parameters[numberParameters++] =
          CbcOrClpParam("randomi!zedRounding", "Whether to try randomized rounding heuristic",
                        "off", CBC_PARAM_STR_RANDROUND);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters-1].setLonghelp
     (
          "stuff needed. \
Doh option does heuristic before preprocessing"     );
#endif
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("randomS!eed", "Random seed for Clp",
                        0, COIN_INT_MAX, CLP_PARAM_INT_RANDOMSEED);
     parameters[numberParameters-1].setLonghelp
     (
          "This sets a random seed for Clp \
- 0 says use time of day."
     );
     parameters[numberParameters-1].setIntValue(1234567);
#endif
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("ratio!Gap", "Stop when gap between best possible and \
best less than this fraction of larger of two",
                        0.0, 1.0e20, CBC_PARAM_DBL_GAPRATIO);
     parameters[numberParameters-1].setDoubleValue(0.0);
     parameters[numberParameters-1].setLonghelp
     (
          "If the gap between best solution and best possible solution is less than this fraction \
of the objective value at the root node then the search will terminate.  See 'allowableGap' for a \
way of using absolute value rather than fraction."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("readS!tored", "Import stored cuts from file",
                        CLP_PARAM_ACTION_STOREDFILE, 3, 0);
#endif
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("reallyO!bjectiveScale", "Scale factor to apply to objective in place",
                        -1.0e20, 1.0e20, CLP_PARAM_DBL_OBJSCALE2, 0);
     parameters[numberParameters-1].setLonghelp
     (
          "You can set this to -1.0 to test maximization or other to stress code"
     );
     parameters[numberParameters-1].setDoubleValue(1.0);
     parameters[numberParameters++] =
          CbcOrClpParam("reallyS!cale", "Scales model in place",
                        CLP_PARAM_ACTION_REALLY_SCALE, 7, 0);
#endif
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("reduce!AndSplitCuts", "Whether to use Reduce-and-Split cuts",
                        "off", CBC_PARAM_STR_REDSPLITCUTS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("root");
     parameters[numberParameters-1].append("ifmove");
     parameters[numberParameters-1].append("forceOn");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on reduce and split  cuts (either at root or in entire tree). \
May be slow \
See branchAndCut for information on options."
     );
      parameters[numberParameters++] =
          CbcOrClpParam("reduce2!AndSplitCuts", "Whether to use Reduce-and-Split cuts - style 2",
                        "off", CBC_PARAM_STR_REDSPLIT2CUTS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("root");
     parameters[numberParameters-1].append("longOn");
     parameters[numberParameters-1].append("longRoot");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on reduce and split  cuts (either at root or in entire tree) \
This version is by Giacomo Nannicini based on Francois Margot's version \
Standard setting only uses rows in tableau <=256, long uses all \
May be slow \
See branchAndCut for information on options."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("reduce2!AndSplitCuts", "Whether to use Reduce-and-Split cuts - style 2",
                        "off", CBC_PARAM_STR_REDSPLIT2CUTS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("root");
     parameters[numberParameters-1].append("longOn");
     parameters[numberParameters-1].append("longRoot");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on reduce and split  cuts (either at root or in entire tree) \
This version is by Giacomo Nannicini based on Francois Margot's version \
Standard setting only uses rows in tableau <=256, long uses all \
See branchAndCut for information on options."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("residual!CapacityCuts", "Whether to use Residual Capacity cuts",
                        "off", CBC_PARAM_STR_RESIDCUTS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("root");
     parameters[numberParameters-1].append("ifmove");
     parameters[numberParameters-1].append("forceOn");
     parameters[numberParameters-1].setLonghelp
     (
          "Residual capacity cuts. \
See branchAndCut for information on options."
     );
#endif
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("restore!Model", "Restore model from binary file",
                        CLP_PARAM_ACTION_RESTORE, 7, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "This reads data save by saveModel from the given file.  It will use the default\
 directory given by 'directory'.  A name of '$' will use the previous value for the name.  This\
 is initialized to 'default.prob'."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("reverse", "Reverses sign of objective",
                        CLP_PARAM_ACTION_REVERSE, 7, 0);
     parameters[numberParameters-1].setLonghelp
     (
          "Useful for testing if maximization works correctly"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("rhs!Scale", "Scale factor to apply to rhs and bounds",
                        -1.0e20, 1.0e20, CLP_PARAM_DBL_RHSSCALE, 0);
     parameters[numberParameters-1].setLonghelp
     (
          "If the rhs or bounds have some very large meaningful values, you may wish to scale them\
 internally by this amount.  It can also be set by autoscale.  This should not be needed."
     );
     parameters[numberParameters-1].setDoubleValue(1.0);
#endif
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("Rens", "Whether to try Relaxation Enforced Neighborhood Search",
                        "off", CBC_PARAM_STR_RENS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters-1].append("200");
     parameters[numberParameters-1].append("1000");
     parameters[numberParameters-1].append("10000");
     parameters[numberParameters-1].append("dj");
     parameters[numberParameters-1].append("djbefore");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on Relaxation enforced neighborhood Search. \
on just does 50 nodes \
200 or 1000 does that many nodes. \
Doh option does heuristic before preprocessing"     );
     parameters[numberParameters++] =
          CbcOrClpParam("Rins", "Whether to try Relaxed Induced Neighborhood Search",
                        "off", CBC_PARAM_STR_RINS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters-1].append("often");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on Relaxed induced neighborhood Search. \
Doh option does heuristic before preprocessing"     );
     parameters[numberParameters++] =
          CbcOrClpParam("round!ingHeuristic", "Whether to use Rounding heuristic",
                        "off", CBC_PARAM_STR_ROUNDING);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on a simple (but effective) rounding heuristic at each node of tree.  \
On means do in solve i.e. after preprocessing, \
Before means do if doHeuristics used, off otherwise, \
and both means do if doHeuristics and in solve."
     );

#endif
     parameters[numberParameters++] =
          CbcOrClpParam("saveM!odel", "Save model to binary file",
                        CLP_PARAM_ACTION_SAVE, 7, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "This will save the problem to the given file name for future use\
 by restoreModel.  It will use the default\
 directory given by 'directory'.  A name of '$' will use the previous value for the name.  This\
 is initialized to 'default.prob'."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("saveS!olution", "saves solution to file",
                        CLP_PARAM_ACTION_SAVESOL);
     parameters[numberParameters-1].setLonghelp
     (
          "This will write a binary solution file to the given file name.  It will use the default\
 directory given by 'directory'.  A name of '$' will use the previous value for the name.  This\
 is initialized to 'solution.file'.  To read the file use fread(int) twice to pick up number of rows \
and columns, then fread(double) to pick up objective value, then pick up row activities, row duals, column \
activities and reduced costs - see bottom of CbcOrClpParam.cpp for code that reads or writes file. \
If name contains '_fix_read_' then does not write but reads and will fix all variables"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("scal!ing", "Whether to scale problem",
                        "off", CLP_PARAM_STR_SCALING);
     parameters[numberParameters-1].append("equi!librium");
     parameters[numberParameters-1].append("geo!metric");
     parameters[numberParameters-1].append("auto!matic");
     parameters[numberParameters-1].append("dynamic");
     parameters[numberParameters-1].append("rows!only");
     parameters[numberParameters-1].setLonghelp
     (
          "Scaling can help in solving problems which might otherwise fail because of lack of\
 accuracy.  It can also reduce the number of iterations.  It is not applied if the range\
 of elements is small.  When unscaled it is possible that there may be small primal and/or\
 infeasibilities."
     );
     parameters[numberParameters-1].setCurrentOption(3); // say auto
#ifndef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("sec!onds", "Maximum seconds",
                        -1.0, 1.0e12, CLP_PARAM_DBL_TIMELIMIT);
     parameters[numberParameters-1].setLonghelp
     (
          "After this many seconds clp will act as if maximum iterations had been reached \
(if value >=0)."
     );
#else
     parameters[numberParameters++] =
          CbcOrClpParam("sec!onds", "maximum seconds",
                        -1.0, 1.0e12, CBC_PARAM_DBL_TIMELIMIT_BAB);
     parameters[numberParameters-1].setLonghelp
     (
          "After this many seconds coin solver will act as if maximum nodes had been reached."
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("sleep", "for debug",
                        CLP_PARAM_ACTION_DUMMY, 7, 0);
     parameters[numberParameters-1].setLonghelp
     (
          "If passed to solver fom ampl, then ampl will wait so that you can copy .nl file for debug."
     );
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("slow!cutpasses", "Maximum number of tries for slower cuts",
                        -1, COIN_INT_MAX, CBC_PARAM_INT_MAX_SLOW_CUTS);
     parameters[numberParameters-1].setLonghelp
     (
          "Some cut generators are fairly slow - this limits the number of times they are tried."
     );
     parameters[numberParameters-1].setIntValue(10);
#endif
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("slp!Value", "Number of slp passes before primal",
                        -1, 50000, CLP_PARAM_INT_SLPVALUE, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "If you are solving a quadratic problem using primal then it may be helpful to do some \
sequential Lps to get a good approximate solution."
     );
#if CLP_MULTIPLE_FACTORIZATIONS > 0
     parameters[numberParameters++] =
          CbcOrClpParam("small!Factorization", "Whether to use small factorization",
                        -1, 10000, CBC_PARAM_INT_SMALLFACT, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "If processed problem <= this use small factorization"
     );
     parameters[numberParameters-1].setIntValue(-1);
#endif
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("solu!tion", "Prints solution to file",
                        CLP_PARAM_ACTION_SOLUTION);
     parameters[numberParameters-1].setLonghelp
     (
          "This will write a primitive solution file to the given file name.  It will use the default\
 directory given by 'directory'.  A name of '$' will use the previous value for the name.  This\
 is initialized to 'stdout'.  The amount of output can be varied using printi!ngOptions or printMask."
     );
#ifdef COIN_HAS_CLP
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("solv!e", "Solve problem",
                        CBC_PARAM_ACTION_BAB);
     parameters[numberParameters-1].setLonghelp
     (
          "If there are no integer variables then this just solves LP.  If there are integer variables \
this does branch and cut."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("sos!Options", "Whether to use SOS from AMPL",
                        "off", CBC_PARAM_STR_SOS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].setCurrentOption("on");
     parameters[numberParameters-1].setLonghelp
     (
          "Normally if AMPL says there are SOS variables they should be used, but sometime sthey should\
 be turned off - this does so."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("slog!Level", "Level of detail in (LP) Solver output",
                        -1, 63, CLP_PARAM_INT_SOLVERLOGLEVEL);
     parameters[numberParameters-1].setLonghelp
     (
          "If 0 then there should be no output in normal circumstances.  1 is probably the best\
 value for most uses, while 2 and 3 give more information.  This parameter is only used inside MIP - for Clp use 'log'"
     );
#else
     // allow solve as synonym for possible dual
     parameters[numberParameters++] =
       CbcOrClpParam("solv!e", "Solve problem using dual simplex (probably)",
                        CLP_PARAM_ACTION_EITHERSIMPLEX);
     parameters[numberParameters-1].setLonghelp
     (
          "Just so can use solve for clp as well as in cbc"
     );
#endif
#endif
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("spars!eFactor", "Whether factorization treated as sparse",
                        "on", CLP_PARAM_STR_SPARSEFACTOR, 7, 0);
     parameters[numberParameters-1].append("off");
     parameters[numberParameters++] =
          CbcOrClpParam("special!Options", "Dubious options for Simplex - see ClpSimplex.hpp",
                        0, COIN_INT_MAX, CLP_PARAM_INT_SPECIALOPTIONS, 0);
     parameters[numberParameters++] =
          CbcOrClpParam("sprint!Crash", "Whether to try sprint crash",
                        -1, 5000000, CLP_PARAM_INT_SPRINT);
     parameters[numberParameters-1].setLonghelp
     (
          "For long and thin problems this program may solve a series of small problems\
 created by taking a subset of the columns.  I introduced the idea as 'Sprint' after\
 an LP code of that name of the 60's which tried the same tactic (not totally successfully).\
  Cplex calls it 'sifting'.  -1 is automatic choice, 0 is off, n is number of passes"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("stat!istics", "Print some statistics",
                        CLP_PARAM_ACTION_STATISTICS);
     parameters[numberParameters-1].setLonghelp
     (
          "This command prints some statistics for the current model.\
 If log level >1 then more is printed.\
 These are for presolved model if presolve on (and unscaled)."
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("stop", "Stops clp execution",
                        CLP_PARAM_ACTION_EXIT);
     parameters[numberParameters-1].setLonghelp
     (
          "This stops the execution of Clp, end, exit, quit and stop are synonyms"
     );
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("strat!egy", "Switches on groups of features",
                        0, 2, CBC_PARAM_INT_STRATEGY);
     parameters[numberParameters-1].setLonghelp
     (
          "This turns on newer features. \
Use 0 for easy problems, 1 is default, 2 is aggressive. \
1 uses Gomory cuts using tolerance of 0.01 at root, \
does a possible restart after 100 nodes if can fix many \
and activates a diving and RINS heuristic and makes feasibility pump \
more aggressive. \
This does not apply to unit tests (where 'experiment' may have similar effects)."
     );
     parameters[numberParameters-1].setIntValue(1);
#ifdef CBC_KEEP_DEPRECATED
     parameters[numberParameters++] =
          CbcOrClpParam("strengthen", "Create strengthened problem",
                        CBC_PARAM_ACTION_STRENGTHEN, 3);
     parameters[numberParameters-1].setLonghelp
     (
          "This creates a new problem by applying the root node cuts.  All tight constraints \
will be in resulting problem"
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("strong!Branching", "Number of variables to look at in strong branching",
                        0, COIN_INT_MAX, CBC_PARAM_INT_STRONGBRANCHING);
     parameters[numberParameters-1].setLonghelp
     (
          "In order to decide which variable to branch on, the code will choose up to this number \
of unsatisfied variables to do mini up and down branches on.  Then the most effective one is chosen. \
If a variable is branched on many times then the previous average up and down costs may be used - \
see number before trust."
     );
#endif
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("subs!titution", "How long a column to substitute for in presolve",
                        0, 10000, CLP_PARAM_INT_SUBSTITUTION, 0);
     parameters[numberParameters-1].setLonghelp
     (
          "Normally Presolve gets rid of 'free' variables when there are no more than 3 \
 variables in column.  If you increase this the number of rows may decrease but number of \
 elements may increase."
     );
#endif
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("testO!si", "Test OsiObject stuff",
                        -1, COIN_INT_MAX, CBC_PARAM_INT_TESTOSI, 0);
#endif
#ifdef CBC_THREAD
     parameters[numberParameters++] =
          CbcOrClpParam("thread!s", "Number of threads to try and use",
                        -100, 100000, CBC_PARAM_INT_THREADS, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "To use multiple threads, set threads to number wanted.  It may be better \
to use one or two more than number of cpus available.  If 100+n then n threads and \
search is repeatable (maybe be somewhat slower), \
if 200+n use threads for root cuts, 400+n threads used in sub-trees."
     );
#endif
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("tighten!Factor", "Tighten bounds using this times largest \
activity at continuous solution",
                        1.0e-3, 1.0e20, CBC_PARAM_DBL_TIGHTENFACTOR, 0);
     parameters[numberParameters-1].setLonghelp
     (
          "This sleazy trick can help on some problems."
     );
#endif
#ifdef COIN_HAS_CLP
     parameters[numberParameters++] =
          CbcOrClpParam("tightLP", "Poor person's preSolve for now",
                        CLP_PARAM_ACTION_TIGHTEN, 7, 0);
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("timeM!ode", "Whether to use CPU or elapsed time",
                        "cpu", CLP_PARAM_STR_TIME_MODE);
     parameters[numberParameters-1].append("elapsed");
     parameters[numberParameters-1].setLonghelp
     (
          "cpu uses CPU time for stopping, while elapsed uses elapsed time. \
(On Windows, elapsed time is always used)."
     );
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("trust!PseudoCosts", "Number of branches before we trust pseudocosts",
                        -3, 2000000000, CBC_PARAM_INT_NUMBERBEFORE);
     parameters[numberParameters-1].setLonghelp
     (
          "Using strong branching computes pseudo-costs.  After this many times for a variable we just \
trust the pseudo costs and do not do any more strong branching."
     );
#endif
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("tune!PreProcess", "Dubious tuning parameters",
                        0, 20000000, CLP_PARAM_INT_PROCESSTUNE, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "For making equality cliques this is minimumsize.  Also for adding \
integer slacks.  May be used for more later \
If <10000 that is what it does.  If <1000000 - numberPasses is (value/10000)-1 and tune is tune %10000. \
If >= 1000000! - numberPasses is (value/1000000)-1 and tune is tune %1000000.  In this case if tune is now still >=10000 \
numberPassesPerInnerLoop is changed from 10 to (tune-10000)-1 and tune becomes tune % 10000!!!!! - happy? - \
so to keep normal limit on cliques of 5, do 3 major passes (include presolves) but only doing one tightening pass per major pass - \
you would use 3010005 (I think)"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("two!MirCuts", "Whether to use Two phase Mixed Integer Rounding cuts",
                        "off", CBC_PARAM_STR_TWOMIRCUTS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("root");
     parameters[numberParameters-1].append("ifmove");
     parameters[numberParameters-1].append("forceOn");
     parameters[numberParameters-1].append("onglobal");
     parameters[numberParameters-1].append("forceandglobal");
     parameters[numberParameters-1].append("forceLongOn");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on two phase mixed integer rounding  cuts (either at root or in entire tree) \
See branchAndCut for information on options."
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("unitTest", "Do unit test",
                        CLP_PARAM_ACTION_UNITTEST, 3, 1);
     parameters[numberParameters-1].setLonghelp
     (
          "This exercises the unit test for clp"
     );
     parameters[numberParameters++] =
          CbcOrClpParam("userClp", "Hand coded Clp stuff",
                        CLP_PARAM_ACTION_USERCLP, 0, 0);
     parameters[numberParameters-1].setLonghelp
     (
          "There are times e.g. when using AMPL interface when you may wish to do something unusual.  \
Look for USERCLP in main driver and modify sample code."
     );
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("userCbc", "Hand coded Cbc stuff",
                        CBC_PARAM_ACTION_USERCBC, 0, 0);
     parameters[numberParameters-1].setLonghelp
     (
          "There are times e.g. when using AMPL interface when you may wish to do something unusual.  \
Look for USERCBC in main driver and modify sample code. \
It is possible you can get same effect by using example driver4.cpp."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("Vnd!VariableNeighborhoodSearch", "Whether to try Variable Neighborhood Search",
                        "off", CBC_PARAM_STR_VND);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("both");
     parameters[numberParameters-1].append("before");
     parameters[numberParameters-1].append("intree");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on variable neighborhood Search. \
Doh option does heuristic before preprocessing"     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("vector", "Whether to use vector? Form of matrix in simplex",
                        "off", CLP_PARAM_STR_VECTOR, 7, 0);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].setLonghelp
     (
          "If this is on ClpPackedMatrix uses extra column copy in odd format."
     );
     parameters[numberParameters++] =
          CbcOrClpParam("verbose", "Switches on longer help on single ?",
                        0, 31, CLP_PARAM_INT_VERBOSE, 0);
     parameters[numberParameters-1].setLonghelp
     (
          "Set to 1 to get short help with ? list, 2 to get long help, 3 for both.  (add 4 to just get ampl ones)."
     );
     parameters[numberParameters-1].setIntValue(0);
#ifdef COIN_HAS_CBC
     parameters[numberParameters++] =
          CbcOrClpParam("vub!heuristic", "Type of vub heuristic",
                        -2, 20, CBC_PARAM_INT_VUBTRY, 0);
     parameters[numberParameters-1].setLonghelp
     (
          "If set will try and fix some integer variables"
     );
     parameters[numberParameters-1].setIntValue(-1);
     parameters[numberParameters++] =
          CbcOrClpParam("zero!HalfCuts", "Whether to use zero half cuts",
                        "off", CBC_PARAM_STR_ZEROHALFCUTS);
     parameters[numberParameters-1].append("on");
     parameters[numberParameters-1].append("root");
     parameters[numberParameters-1].append("ifmove");
     parameters[numberParameters-1].append("forceOn");
     parameters[numberParameters-1].append("onglobal");
     parameters[numberParameters-1].setLonghelp
     (
          "This switches on zero-half cuts (either at root or in entire tree) \
See branchAndCut for information on options.  This implementation was written by \
Alberto Caprara."
     );
#endif
     parameters[numberParameters++] =
          CbcOrClpParam("zeroT!olerance", "Kill all coefficients \
whose absolute value is less than this value",
                        1.0e-100, 1.0e-5, CLP_PARAM_DBL_ZEROTOLERANCE);
     parameters[numberParameters-1].setLonghelp
     (
          "This applies to reading mps files (and also lp files \
if KILL_ZERO_READLP defined)"
     );
     parameters[numberParameters-1].setDoubleValue(1.0e-20);
     assert(numberParameters < CBCMAXPARAMETERS);
}
// Given a parameter type - returns its number in list
int whichParam (CbcOrClpParameterType name,
                int numberParameters, CbcOrClpParam *const parameters)
{
     int i;
     for (i = 0; i < numberParameters; i++) {
          if (parameters[i].type() == name)
               break;
     }
     assert (i < numberParameters);
     return i;
}
#ifdef COIN_HAS_CLP
/* Restore a solution from file.
   mode 0 normal, 1 swap rows and columns and primal and dual
   if 2 set then also change signs
*/
void restoreSolution(ClpSimplex * lpSolver, std::string fileName, int mode)
{
     FILE * fp = fopen(fileName.c_str(), "rb");
     if (fp) {
          int numberRows = lpSolver->numberRows();
          int numberColumns = lpSolver->numberColumns();
          int numberRowsFile;
          int numberColumnsFile;
          double objectiveValue;
          size_t nRead;
          nRead = fread(&numberRowsFile, sizeof(int), 1, fp);
          if (nRead != 1)
               throw("Error in fread");
          nRead = fread(&numberColumnsFile, sizeof(int), 1, fp);
          if (nRead != 1)
               throw("Error in fread");
          nRead = fread(&objectiveValue, sizeof(double), 1, fp);
          if (nRead != 1)
               throw("Error in fread");
          double * dualRowSolution = lpSolver->dualRowSolution();
          double * primalRowSolution = lpSolver->primalRowSolution();
          double * dualColumnSolution = lpSolver->dualColumnSolution();
          double * primalColumnSolution = lpSolver->primalColumnSolution();
          if (mode) {
               // swap
               int k = numberRows;
               numberRows = numberColumns;
               numberColumns = k;
               double * temp;
               temp = dualRowSolution;
               dualRowSolution = primalColumnSolution;
               primalColumnSolution = temp;
               temp = dualColumnSolution;
               dualColumnSolution = primalRowSolution;
               primalRowSolution = temp;
          }
          if (numberRows > numberRowsFile || numberColumns > numberColumnsFile) {
               std::cout << "Mismatch on rows and/or columns - giving up" << std::endl;
          } else {
               lpSolver->setObjectiveValue(objectiveValue);
               if (numberRows == numberRowsFile && numberColumns == numberColumnsFile) {
                    nRead = fread(primalRowSolution, sizeof(double), numberRows, fp);
                    if (nRead != static_cast<size_t>(numberRows))
                         throw("Error in fread");
                    nRead = fread(dualRowSolution, sizeof(double), numberRows, fp);
                    if (nRead != static_cast<size_t>(numberRows))
                         throw("Error in fread");
                    nRead = fread(primalColumnSolution, sizeof(double), numberColumns, fp);
                    if (nRead != static_cast<size_t>(numberColumns))
                         throw("Error in fread");
                    nRead = fread(dualColumnSolution, sizeof(double), numberColumns, fp);
                    if (nRead != static_cast<size_t>(numberColumns))
                         throw("Error in fread");
               } else {
                    std::cout << "Mismatch on rows and/or columns - truncating" << std::endl;
                    double * temp = new double [CoinMax(numberRowsFile, numberColumnsFile)];
                    nRead = fread(temp, sizeof(double), numberRowsFile, fp);
                    if (nRead != static_cast<size_t>(numberRowsFile))
                         throw("Error in fread");
                    CoinMemcpyN(temp, numberRows, primalRowSolution);
                    nRead = fread(temp, sizeof(double), numberRowsFile, fp);
                    if (nRead != static_cast<size_t>(numberRowsFile))
                         throw("Error in fread");
                    CoinMemcpyN(temp, numberRows, dualRowSolution);
                    nRead = fread(temp, sizeof(double), numberColumnsFile, fp);
                    if (nRead != static_cast<size_t>(numberColumnsFile))
                         throw("Error in fread");
                    CoinMemcpyN(temp, numberColumns, primalColumnSolution);
                    nRead = fread(temp, sizeof(double), numberColumnsFile, fp);
                    if (nRead != static_cast<size_t>(numberColumnsFile))
                         throw("Error in fread");
                    CoinMemcpyN(temp, numberColumns, dualColumnSolution);
                    delete [] temp;
			   }
               if (mode == 3) {
                    int i;
                    for (i = 0; i < numberRows; i++) {
                         primalRowSolution[i] = -primalRowSolution[i];
                         dualRowSolution[i] = -dualRowSolution[i];
                    }
                    for (i = 0; i < numberColumns; i++) {
                         primalColumnSolution[i] = -primalColumnSolution[i];
                         dualColumnSolution[i] = -dualColumnSolution[i];
                    }
               }
          }
          fclose(fp);
     } else {
          std::cout << "Unable to open file " << fileName << std::endl;
     }
}
// Dump a solution to file
void saveSolution(const ClpSimplex * lpSolver, std::string fileName)
{
     if (strstr(fileName.c_str(), "_fix_read_")) {
          FILE * fp = fopen(fileName.c_str(), "rb");
          if (fp) {
               ClpSimplex * solver = const_cast<ClpSimplex *>(lpSolver);
               restoreSolution(solver, fileName, 0);
               // fix all
               int logLevel = solver->logLevel();
               int iColumn;
               int numberColumns = solver->numberColumns();
               double * primalColumnSolution =
                    solver->primalColumnSolution();
               double * columnLower = solver->columnLower();
               double * columnUpper = solver->columnUpper();
               for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                    double value = primalColumnSolution[iColumn];
                    if (value > columnUpper[iColumn]) {
                         if (value > columnUpper[iColumn] + 1.0e-6 && logLevel > 1)
                              printf("%d value of %g - bounds %g %g\n",
                                     iColumn, value, columnLower[iColumn], columnUpper[iColumn]);
                         value = columnUpper[iColumn];
                    } else if (value < columnLower[iColumn]) {
                         if (value < columnLower[iColumn] - 1.0e-6 && logLevel > 1)
                              printf("%d value of %g - bounds %g %g\n",
                                     iColumn, value, columnLower[iColumn], columnUpper[iColumn]);
                         value = columnLower[iColumn];
                    }
                    columnLower[iColumn] = value;
                    columnUpper[iColumn] = value;
               }
               return;
          }
     }
     FILE * fp = fopen(fileName.c_str(), "wb");
     if (fp) {
          int numberRows = lpSolver->numberRows();
          int numberColumns = lpSolver->numberColumns();
          double objectiveValue = lpSolver->objectiveValue();
          size_t nWrite;
          nWrite = fwrite(&numberRows, sizeof(int), 1, fp);
          if (nWrite != 1)
               throw("Error in fwrite");
          nWrite = fwrite(&numberColumns, sizeof(int), 1, fp);
          if (nWrite != 1)
               throw("Error in fwrite");
          nWrite = fwrite(&objectiveValue, sizeof(double), 1, fp);
          if (nWrite != 1)
               throw("Error in fwrite");
          double * dualRowSolution = lpSolver->dualRowSolution();
          double * primalRowSolution = lpSolver->primalRowSolution();
          nWrite = fwrite(primalRowSolution, sizeof(double), numberRows, fp);
          if (nWrite != static_cast<size_t>(numberRows))
               throw("Error in fwrite");
          nWrite = fwrite(dualRowSolution, sizeof(double), numberRows, fp);
          if (nWrite != static_cast<size_t>(numberRows))
               throw("Error in fwrite");
          double * dualColumnSolution = lpSolver->dualColumnSolution();
          double * primalColumnSolution = lpSolver->primalColumnSolution();
          nWrite = fwrite(primalColumnSolution, sizeof(double), numberColumns, fp);
          if (nWrite != static_cast<size_t>(numberColumns))
               throw("Error in fwrite");
          nWrite = fwrite(dualColumnSolution, sizeof(double), numberColumns, fp);
          if (nWrite != static_cast<size_t>(numberColumns))
               throw("Error in fwrite");
          fclose(fp);
     } else {
          std::cout << "Unable to open file " << fileName << std::endl;
     }
}
#endif
