
/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifdef USE_CBCCONFIG
# include "CbcConfig.h"
#else
# include "ClpConfig.h"
#endif

#ifndef CbcOrClpParam_H
#define CbcOrClpParam_H
/**
   This has parameter handling stuff which can be shared between Cbc and Clp (and Dylp etc).

   This (and .cpp) should be copied so that it is the same in Cbc/Test and Clp/Test.
   I know this is not elegant but it seems simplest.

   It uses COIN_HAS_CBC for parameters wanted by CBC
   It uses COIN_HAS_CLP for parameters wanted by CLP (or CBC using CLP)
   It could use COIN_HAS_DYLP for parameters wanted by DYLP
   It could use COIN_HAS_DYLP_OR_CLP for parameters wanted by DYLP or CLP etc etc

 */
class OsiSolverInterface;
class CbcModel;
class ClpSimplex;
/*! \brief Parameter codes

  Parameter type ranges are allocated as follows
  <ul>
    <li>   1 -- 100	double parameters
    <li> 101 -- 200	integer parameters
    <li> 201 -- 250	string parameters
    <li> 251 -- 300	cuts etc(string but broken out for clarity)
    <li> 301 -- 400	`actions'
  </ul>

  `Actions' do not necessarily invoke an immediate action; it's just that they
  don't fit neatly into the parameters array.

  This coding scheme is in flux.
*/

enum CbcOrClpParameterType

{
     CBC_PARAM_GENERALQUERY = -100,
     CBC_PARAM_FULLGENERALQUERY,

     CLP_PARAM_DBL_PRIMALTOLERANCE = 1,
     CLP_PARAM_DBL_DUALTOLERANCE,
     CLP_PARAM_DBL_TIMELIMIT,
     CLP_PARAM_DBL_DUALBOUND,
     CLP_PARAM_DBL_PRIMALWEIGHT,
     CLP_PARAM_DBL_OBJSCALE,
     CLP_PARAM_DBL_RHSSCALE,
     CLP_PARAM_DBL_ZEROTOLERANCE,

     CBC_PARAM_DBL_INFEASIBILITYWEIGHT = 51,
     CBC_PARAM_DBL_CUTOFF,
     CBC_PARAM_DBL_INTEGERTOLERANCE,
     CBC_PARAM_DBL_INCREMENT,
     CBC_PARAM_DBL_ALLOWABLEGAP,
     CBC_PARAM_DBL_TIMELIMIT_BAB,
     CBC_PARAM_DBL_GAPRATIO,

     CBC_PARAM_DBL_DJFIX = 81,
     CBC_PARAM_DBL_TIGHTENFACTOR,
     CLP_PARAM_DBL_PRESOLVETOLERANCE,
     CLP_PARAM_DBL_OBJSCALE2,
     CBC_PARAM_DBL_FAKEINCREMENT,
     CBC_PARAM_DBL_FAKECUTOFF,
     CBC_PARAM_DBL_ARTIFICIALCOST,
     CBC_PARAM_DBL_DEXTRA3,
     CBC_PARAM_DBL_SMALLBAB,
     CBC_PARAM_DBL_DEXTRA4,
     CBC_PARAM_DBL_DEXTRA5,

     CLP_PARAM_INT_SOLVERLOGLEVEL = 101,
#ifndef COIN_HAS_CBC
     CLP_PARAM_INT_LOGLEVEL = 101,
#endif
     CLP_PARAM_INT_MAXFACTOR,
     CLP_PARAM_INT_PERTVALUE,
     CLP_PARAM_INT_MAXITERATION,
     CLP_PARAM_INT_PRESOLVEPASS,
     CLP_PARAM_INT_IDIOT,
     CLP_PARAM_INT_SPRINT,
     CLP_PARAM_INT_OUTPUTFORMAT,
     CLP_PARAM_INT_SLPVALUE,
     CLP_PARAM_INT_PRESOLVEOPTIONS,
     CLP_PARAM_INT_PRINTOPTIONS,
     CLP_PARAM_INT_SPECIALOPTIONS,
     CLP_PARAM_INT_SUBSTITUTION,
     CLP_PARAM_INT_DUALIZE,
     CLP_PARAM_INT_VERBOSE,
     CLP_PARAM_INT_CPP,
     CLP_PARAM_INT_PROCESSTUNE,
     CLP_PARAM_INT_USESOLUTION,
     CLP_PARAM_INT_RANDOMSEED,
     CLP_PARAM_INT_MORESPECIALOPTIONS,

     CBC_PARAM_INT_STRONGBRANCHING = 151,
     CBC_PARAM_INT_CUTDEPTH,
     CBC_PARAM_INT_MAXNODES,
     CBC_PARAM_INT_NUMBERBEFORE,
     CBC_PARAM_INT_NUMBERANALYZE,
     CBC_PARAM_INT_MIPOPTIONS,
     CBC_PARAM_INT_MOREMIPOPTIONS,
     CBC_PARAM_INT_MAXHOTITS,
     CBC_PARAM_INT_FPUMPITS,
     CBC_PARAM_INT_MAXSOLS,
     CBC_PARAM_INT_FPUMPTUNE,
     CBC_PARAM_INT_TESTOSI,
     CBC_PARAM_INT_EXTRA1,
     CBC_PARAM_INT_EXTRA2,
     CBC_PARAM_INT_EXTRA3,
     CBC_PARAM_INT_EXTRA4,
     CBC_PARAM_INT_DEPTHMINIBAB,
     CBC_PARAM_INT_CUTPASSINTREE,
     CBC_PARAM_INT_THREADS,
     CBC_PARAM_INT_CUTPASS,
     CBC_PARAM_INT_VUBTRY,
     CBC_PARAM_INT_DENSE,
     CBC_PARAM_INT_EXPERIMENT,
     CBC_PARAM_INT_DIVEOPT,
     CBC_PARAM_INT_STRATEGY,
     CBC_PARAM_INT_SMALLFACT,
     CBC_PARAM_INT_HOPTIONS,
     CBC_PARAM_INT_CUTLENGTH,
     CBC_PARAM_INT_FPUMPTUNE2,
#ifdef COIN_HAS_CBC
     CLP_PARAM_INT_LOGLEVEL ,
#endif
     CBC_PARAM_INT_MAXSAVEDSOLS,
     CBC_PARAM_INT_RANDOMSEED,
     CBC_PARAM_INT_MULTIPLEROOTS,
     CBC_PARAM_INT_STRONG_STRATEGY,
     CBC_PARAM_INT_EXTRA_VARIABLES,
     CBC_PARAM_INT_MAX_SLOW_CUTS,

     CLP_PARAM_STR_DIRECTION = 201,
     CLP_PARAM_STR_DUALPIVOT,
     CLP_PARAM_STR_SCALING,
     CLP_PARAM_STR_ERRORSALLOWED,
     CLP_PARAM_STR_KEEPNAMES,
     CLP_PARAM_STR_SPARSEFACTOR,
     CLP_PARAM_STR_PRIMALPIVOT,
     CLP_PARAM_STR_PRESOLVE,
     CLP_PARAM_STR_CRASH,
     CLP_PARAM_STR_BIASLU,
     CLP_PARAM_STR_PERTURBATION,
     CLP_PARAM_STR_MESSAGES,
     CLP_PARAM_STR_AUTOSCALE,
     CLP_PARAM_STR_CHOLESKY,
     CLP_PARAM_STR_KKT,
     CLP_PARAM_STR_BARRIERSCALE,
     CLP_PARAM_STR_GAMMA,
     CLP_PARAM_STR_CROSSOVER,
     CLP_PARAM_STR_PFI,
     CLP_PARAM_STR_INTPRINT,
     CLP_PARAM_STR_VECTOR,
     CLP_PARAM_STR_FACTORIZATION,
     CLP_PARAM_STR_ALLCOMMANDS,
     CLP_PARAM_STR_TIME_MODE,
     CLP_PARAM_STR_ABCWANTED,

     CBC_PARAM_STR_NODESTRATEGY = 251,
     CBC_PARAM_STR_BRANCHSTRATEGY,
     CBC_PARAM_STR_CUTSSTRATEGY,
     CBC_PARAM_STR_HEURISTICSTRATEGY,
     CBC_PARAM_STR_GOMORYCUTS,
     CBC_PARAM_STR_PROBINGCUTS,
     CBC_PARAM_STR_KNAPSACKCUTS,
     CBC_PARAM_STR_REDSPLITCUTS,
     CBC_PARAM_STR_ROUNDING,
     CBC_PARAM_STR_SOLVER,
     CBC_PARAM_STR_CLIQUECUTS,
     CBC_PARAM_STR_COSTSTRATEGY,
     CBC_PARAM_STR_FLOWCUTS,
     CBC_PARAM_STR_MIXEDCUTS,
     CBC_PARAM_STR_TWOMIRCUTS,
     CBC_PARAM_STR_PREPROCESS,
     CBC_PARAM_STR_FPUMP,
     CBC_PARAM_STR_GREEDY,
     CBC_PARAM_STR_COMBINE,
     CBC_PARAM_STR_PROXIMITY,
     CBC_PARAM_STR_LOCALTREE,
     CBC_PARAM_STR_SOS,
     CBC_PARAM_STR_LANDPCUTS,
     CBC_PARAM_STR_RINS,
     CBC_PARAM_STR_RESIDCUTS,
     CBC_PARAM_STR_RENS,
     CBC_PARAM_STR_DIVINGS,
     CBC_PARAM_STR_DIVINGC,
     CBC_PARAM_STR_DIVINGF,
     CBC_PARAM_STR_DIVINGG,
     CBC_PARAM_STR_DIVINGL,
     CBC_PARAM_STR_DIVINGP,
     CBC_PARAM_STR_DIVINGV,
     CBC_PARAM_STR_DINS,
     CBC_PARAM_STR_PIVOTANDFIX,
     CBC_PARAM_STR_RANDROUND,
     CBC_PARAM_STR_NAIVE,
     CBC_PARAM_STR_ZEROHALFCUTS,
     CBC_PARAM_STR_CPX,
     CBC_PARAM_STR_CROSSOVER2,
     CBC_PARAM_STR_PIVOTANDCOMPLEMENT,
     CBC_PARAM_STR_VND,
     CBC_PARAM_STR_LAGOMORYCUTS,
     CBC_PARAM_STR_LATWOMIRCUTS,
     CBC_PARAM_STR_REDSPLIT2CUTS,
     CBC_PARAM_STR_GMICUTS,
     CBC_PARAM_STR_CUTOFF_CONSTRAINT,

     CLP_PARAM_ACTION_DIRECTORY = 301,
     CLP_PARAM_ACTION_DIRSAMPLE,
     CLP_PARAM_ACTION_DIRNETLIB,
     CBC_PARAM_ACTION_DIRMIPLIB,
     CLP_PARAM_ACTION_IMPORT,
     CLP_PARAM_ACTION_EXPORT,
     CLP_PARAM_ACTION_RESTORE,
     CLP_PARAM_ACTION_SAVE,
     CLP_PARAM_ACTION_DUALSIMPLEX,
     CLP_PARAM_ACTION_PRIMALSIMPLEX,
     CLP_PARAM_ACTION_EITHERSIMPLEX,
     CLP_PARAM_ACTION_MAXIMIZE,
     CLP_PARAM_ACTION_MINIMIZE,
     CLP_PARAM_ACTION_EXIT,
     CLP_PARAM_ACTION_STDIN,
     CLP_PARAM_ACTION_UNITTEST,
     CLP_PARAM_ACTION_NETLIB_EITHER,
     CLP_PARAM_ACTION_NETLIB_DUAL,
     CLP_PARAM_ACTION_NETLIB_PRIMAL,
     CLP_PARAM_ACTION_SOLUTION,
     CLP_PARAM_ACTION_SAVESOL,
     CLP_PARAM_ACTION_TIGHTEN,
     CLP_PARAM_ACTION_FAKEBOUND,
     CLP_PARAM_ACTION_HELP,
     CLP_PARAM_ACTION_PLUSMINUS,
     CLP_PARAM_ACTION_NETWORK,
     CLP_PARAM_ACTION_ALLSLACK,
     CLP_PARAM_ACTION_REVERSE,
     CLP_PARAM_ACTION_BARRIER,
     CLP_PARAM_ACTION_NETLIB_BARRIER,
     CLP_PARAM_ACTION_NETLIB_TUNE,
     CLP_PARAM_ACTION_REALLY_SCALE,
     CLP_PARAM_ACTION_BASISIN,
     CLP_PARAM_ACTION_BASISOUT,
     CLP_PARAM_ACTION_SOLVECONTINUOUS,
     CLP_PARAM_ACTION_CLEARCUTS,
     CLP_PARAM_ACTION_VERSION,
     CLP_PARAM_ACTION_STATISTICS,
     CLP_PARAM_ACTION_DEBUG,
     CLP_PARAM_ACTION_DUMMY,
     CLP_PARAM_ACTION_PRINTMASK,
     CLP_PARAM_ACTION_OUTDUPROWS,
     CLP_PARAM_ACTION_USERCLP,
     CLP_PARAM_ACTION_MODELIN,
     CLP_PARAM_ACTION_CSVSTATISTICS,
     CLP_PARAM_ACTION_STOREDFILE,
     CLP_PARAM_ACTION_ENVIRONMENT,
     CLP_PARAM_ACTION_PARAMETRICS,
     CLP_PARAM_ACTION_GMPL_SOLUTION,

     CBC_PARAM_ACTION_BAB = 351,
     CBC_PARAM_ACTION_MIPLIB,
     CBC_PARAM_ACTION_STRENGTHEN,
     CBC_PARAM_ACTION_PRIORITYIN,
     CBC_PARAM_ACTION_MIPSTART,
     CBC_PARAM_ACTION_USERCBC,
     CBC_PARAM_ACTION_DOHEURISTIC,
     CLP_PARAM_ACTION_NEXTBESTSOLUTION,

     CBC_PARAM_NOTUSED_OSLSTUFF = 401,
     CBC_PARAM_NOTUSED_CBCSTUFF,

     CBC_PARAM_NOTUSED_INVALID = 1000
} ;
#include <vector>
#include <string>

/// Very simple class for setting parameters

class CbcOrClpParam {
public:
     /**@name Constructor and destructor */
     //@{
     /// Constructors
     CbcOrClpParam (  );
     CbcOrClpParam (std::string name, std::string help,
                    double lower, double upper, CbcOrClpParameterType type, int display = 2);
     CbcOrClpParam (std::string name, std::string help,
                    int lower, int upper, CbcOrClpParameterType type, int display = 2);
     // Other strings will be added by insert
     CbcOrClpParam (std::string name, std::string help, std::string firstValue,
                    CbcOrClpParameterType type, int whereUsed = 7, int display = 2);
     // Action
     CbcOrClpParam (std::string name, std::string help,
                    CbcOrClpParameterType type, int whereUsed = 7, int display = 2);
     /// Copy constructor.
     CbcOrClpParam(const CbcOrClpParam &);
     /// Assignment operator. This copies the data
     CbcOrClpParam & operator=(const CbcOrClpParam & rhs);
     /// Destructor
     ~CbcOrClpParam (  );
     //@}

     /**@name stuff */
     //@{
     /// Insert string (only valid for keywords)
     void append(std::string keyWord);
     /// Adds one help line
     void addHelp(std::string keyWord);
     /// Returns name
     inline std::string  name(  ) const {
          return name_;
     }
     /// Returns short help
     inline std::string  shortHelp(  ) const {
          return shortHelp_;
     }
     /// Sets a double parameter (nonzero code if error)
     int setDoubleParameter(CbcModel & model, double value) ;
     /// Sets double parameter and returns printable string and error code
     const char * setDoubleParameterWithMessage ( CbcModel & model, double  value , int & returnCode);
     /// Gets a double parameter
     double doubleParameter(CbcModel & model) const;
     /// Sets a int parameter (nonzero code if error)
     int setIntParameter(CbcModel & model, int value) ;
     /// Sets int parameter and returns printable string and error code
     const char * setIntParameterWithMessage ( CbcModel & model, int value , int & returnCode);
     /// Gets a int parameter
     int intParameter(CbcModel & model) const;
     /// Sets a double parameter (nonzero code if error)
     int setDoubleParameter(ClpSimplex * model, double value) ;
     /// Gets a double parameter
     double doubleParameter(ClpSimplex * model) const;
     /// Sets double parameter and returns printable string and error code
     const char * setDoubleParameterWithMessage ( ClpSimplex * model, double  value , int & returnCode);
     /// Sets a int parameter (nonzero code if error)
     int setIntParameter(ClpSimplex * model, int value) ;
     /// Sets int parameter and returns printable string and error code
     const char * setIntParameterWithMessage ( ClpSimplex * model, int  value , int & returnCode);
     /// Gets a int parameter
     int intParameter(ClpSimplex * model) const;
     /// Sets a double parameter (nonzero code if error)
     int setDoubleParameter(OsiSolverInterface * model, double value) ;
     /// Sets double parameter and returns printable string and error code
     const char * setDoubleParameterWithMessage ( OsiSolverInterface * model, double  value , int & returnCode);
     /// Gets a double parameter
     double doubleParameter(OsiSolverInterface * model) const;
     /// Sets a int parameter (nonzero code if error)
     int setIntParameter(OsiSolverInterface * model, int value) ;
     /// Sets int parameter and returns printable string and error code
     const char * setIntParameterWithMessage ( OsiSolverInterface * model, int  value , int & returnCode);
     /// Gets a int parameter
     int intParameter(OsiSolverInterface * model) const;
     /// Checks a double parameter (nonzero code if error)
     int checkDoubleParameter(double value) const;
     /// Returns name which could match
     std::string matchName (  ) const;
     /// Returns length of name for ptinting
     int lengthMatchName (  ) const;
     /// Returns parameter option which matches (-1 if none)
     int parameterOption ( std::string check ) const;
     /// Prints parameter options
     void printOptions (  ) const;
     /// Returns current parameter option
     inline std::string currentOption (  ) const {
          return definedKeyWords_[currentKeyWord_];
     }
     /// Sets current parameter option
     void setCurrentOption ( int value , bool printIt = false);
     /// Sets current parameter option and returns printable string
     const char * setCurrentOptionWithMessage ( int value );
     /// Sets current parameter option using string
     void setCurrentOption (const std::string value );
     /// Returns current parameter option position
     inline int currentOptionAsInteger (  ) const {
          return currentKeyWord_;
     }
     /// Sets int value
     void setIntValue ( int value );
     inline int intValue () const {
          return intValue_;
     }
     /// Sets double value
     void setDoubleValue ( double value );
     inline double doubleValue () const {
          return doubleValue_;
     }
     /// Sets string value
     void setStringValue ( std::string value );
     inline std::string stringValue () const {
          return stringValue_;
     }
     /// Returns 1 if matches minimum, 2 if matches less, 0 if not matched
     int matches (std::string input) const;
     /// type
     inline CbcOrClpParameterType type() const {
          return type_;
     }
     /// whether to display
     inline int displayThis() const {
          return display_;
     }
     /// Set Long help
     inline void setLonghelp(const std::string help) {
          longHelp_ = help;
     }
     /// Print Long help
     void printLongHelp() const;
     /// Print action and string
     void printString() const;
     /** 7 if used everywhere,
         1 - used by clp
         2 - used by cbc
         4 - used by ampl
     */
     inline int whereUsed() const {
          return whereUsed_;
     }

private:
     /// gutsOfConstructor
     void gutsOfConstructor();
     //@}
////////////////// data //////////////////
private:

     /**@name data
      We might as well throw all type data in - could derive?
     */
     //@{
     // Type see CbcOrClpParameterType
     CbcOrClpParameterType type_;
     /// If double == okay
     double lowerDoubleValue_;
     double upperDoubleValue_;
     /// If int == okay
     int lowerIntValue_;
     int upperIntValue_;
     // Length of name
     unsigned int lengthName_;
     // Minimum match
     unsigned int lengthMatch_;
     /// set of valid strings
     std::vector<std::string> definedKeyWords_;
     /// Name
     std::string name_;
     /// Short help
     std::string shortHelp_;
     /// Long help
     std::string longHelp_;
     /// Action
     CbcOrClpParameterType action_;
     /// Current keyWord (if a keyword parameter)
     int currentKeyWord_;
     /// Display on ?
     int display_;
     /// Integer parameter - current value
     int intValue_;
     /// Double parameter - current value
     double doubleValue_;
     /// String parameter - current value
     std::string stringValue_;
     /** 7 if used everywhere,
         1 - used by clp
         2 - used by cbc
         4 - used by ampl
     */
     int whereUsed_;
     //@}
};
/// Simple read stuff
std::string CoinReadNextField();

std::string CoinReadGetCommand(int argc, const char *argv[]);
std::string CoinReadGetString(int argc, const char *argv[]);
// valid 0 - okay, 1 bad, 2 not there
int CoinReadGetIntField(int argc, const char *argv[], int * valid);
double CoinReadGetDoubleField(int argc, const char *argv[], int * valid);
void CoinReadPrintit(const char * input);
void setCbcOrClpPrinting(bool yesNo);
#define CBCMAXPARAMETERS 250
/*
  Subroutine to establish the cbc parameter array. See the description of
  class CbcOrClpParam for details. Pulled from C..Main() for clarity.
*/
void establishParams (int &numberParameters, CbcOrClpParam *const parameters);
// Given a parameter type - returns its number in list
int whichParam (CbcOrClpParameterType name,
                int numberParameters, CbcOrClpParam *const parameters);
// Dump a solution to file
void saveSolution(const ClpSimplex * lpSolver, std::string fileName);
#endif	/* CbcOrClpParam_H */
