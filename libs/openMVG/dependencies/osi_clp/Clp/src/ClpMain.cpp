/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cfloat>
#include <string>
#include <iostream>

int boundary_sort = 1000;
int boundary_sort2 = 1000;
int boundary_sort3 = 10000;

#include "CoinPragma.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinSort.hpp"
// History since 1.0 at end
#include "ClpConfig.h"
#include "CoinMpsIO.hpp"
#include "CoinFileIO.hpp"
#ifdef COIN_HAS_GLPK
#include "glpk.h"
extern glp_tran* cbc_glp_tran;
extern glp_prob* cbc_glp_prob;
#else
#define GLP_UNDEF 1
#define GLP_FEAS 2
#define GLP_INFEAS 3
#define GLP_NOFEAS 4
#define GLP_OPT 5
#endif

#include "AbcCommon.hpp"
#include "ClpFactorization.hpp"
#include "CoinTime.hpp"
#include "ClpSimplex.hpp"
#include "ClpSimplexOther.hpp"
#include "ClpSolve.hpp"
#include "ClpMessage.hpp"
#include "ClpPackedMatrix.hpp"
#include "ClpPlusMinusOneMatrix.hpp"
#include "ClpNetworkMatrix.hpp"
#include "ClpDualRowSteepest.hpp"
#include "ClpDualRowDantzig.hpp"
#include "ClpLinearObjective.hpp"
#include "ClpPrimalColumnSteepest.hpp"
#include "ClpPrimalColumnDantzig.hpp"
#include "ClpPresolve.hpp"
#include "CbcOrClpParam.hpp"
#include "CoinSignal.hpp"
#include "CoinWarmStartBasis.hpp"
#ifdef ABC_INHERIT
#include "AbcSimplex.hpp"
#include "AbcSimplexFactorization.hpp"
#include "AbcDualRowSteepest.hpp"
#include "AbcDualRowDantzig.hpp"
#endif
//#define COIN_HAS_ASL
#ifdef COIN_HAS_ASL
#include "Clp_ampl.h"
#endif
#ifdef DMALLOC
#include "dmalloc.h"
#endif
#if defined(COIN_HAS_WSMP) || defined(COIN_HAS_AMD) || defined(COIN_HAS_CHOLMOD) || defined(TAUCS_BARRIER) || defined(COIN_HAS_MUMPS)
#define FOREIGN_BARRIER
#endif

static double totalTime = 0.0;
static bool maskMatches(const int * starts, char ** masks,
                        std::string & check);
#ifndef ABC_INHERIT
static ClpSimplex * currentModel = NULL;
#else
static AbcSimplex * currentModel = NULL;
#endif

extern "C" {
     static void
#if defined(_MSC_VER)
     __cdecl
#endif // _MSC_VER
     signal_handler(int /*whichSignal*/)
     {
          if (currentModel != NULL)
               currentModel->setMaximumIterations(0); // stop at next iterations
          return;
     }
}

//#############################################################################

#ifdef NDEBUG
#undef NDEBUG
#endif

#ifndef ABC_INHERIT
int mainTest (int argc, const char *argv[], int algorithm,
              ClpSimplex empty, ClpSolve solveOptions, int switchOff, bool doVector);
#else
int mainTest (int argc, const char *argv[], int algorithm,
              AbcSimplex empty, ClpSolve solveOptions, int switchOff, bool doVector);
#endif
static void statistics(ClpSimplex * originalModel, ClpSimplex * model);
static void generateCode(const char * fileName, int type);
// Returns next valid field
int CbcOrClpRead_mode = 1;
FILE * CbcOrClpReadCommand = stdin;
extern int CbcOrClpEnvironmentIndex;
#ifdef CLP_USER_DRIVEN1
/* Returns true if variable sequenceOut can leave basis when
   model->sequenceIn() enters.
   This function may be entered several times for each sequenceOut.  
   The first time realAlpha will be positive if going to lower bound
   and negative if going to upper bound (scaled bounds in lower,upper) - then will be zero.
   currentValue is distance to bound.
   currentTheta is current theta.
   alpha is fabs(pivot element).
   Variable will change theta if currentValue - currentTheta*alpha < 0.0
*/
bool userChoiceValid1(const ClpSimplex * model,
		      int sequenceOut,
		      double currentValue,
		      double currentTheta,
		      double alpha,
		      double realAlpha)
{
  return true;
}
/* This returns true if chosen in/out pair valid.
   The main thing to check would be variable flipping bounds may be
   OK.  This would be signaled by reasonable theta_ and valueOut_.
   If you return false sequenceIn_ will be flagged as ineligible.
*/
bool userChoiceValid2(const ClpSimplex * model)
{
  return true;
}
/* If a good pivot then you may wish to unflag some variables.
 */
void userChoiceWasGood(ClpSimplex * model)
{
}
#endif
//#define CILK_TEST
#ifdef CILK_TEST
static void cilkTest();
#endif
int
#if defined(_MSC_VER)
__cdecl
#endif // _MSC_VER
main (int argc, const char *argv[])
{
#ifdef CILK_TEST
  cilkTest();
#endif
     // next {} is just to make sure all memory should be freed - for debug
     {
          double time1 = CoinCpuTime(), time2;
          // Set up all non-standard stuff
          //int numberModels=1;
#ifndef ABC_INHERIT
          ClpSimplex * models = new ClpSimplex[1];
#else
          AbcSimplex * models = new AbcSimplex[1];
#endif

          // default action on import
          int allowImportErrors = 0;
          int keepImportNames = 1;
          int doIdiot = -1;
          int outputFormat = 2;
          int slpValue = -1;
          int cppValue = -1;
          int printOptions = 0;
          int printMode = 0;
          int presolveOptions = 0;
          int doCrash = 0;
          int doVector = 0;
          int doSprint = -1;
          // set reasonable defaults
#ifdef ABC_INHERIT
#define DEFAULT_PRESOLVE_PASSES 20
#else
#define DEFAULT_PRESOLVE_PASSES 5
#endif
          int preSolve = DEFAULT_PRESOLVE_PASSES;
          bool preSolveFile = false;
          models->setPerturbation(50);
          models->messageHandler()->setPrefix(false);
          const char dirsep =  CoinFindDirSeparator();
          std::string directory;
          std::string dirSample;
          std::string dirNetlib;
          std::string dirMiplib;
          if (dirsep == '/') {
               directory = "./";
               dirSample = "../../Data/Sample/";
               dirNetlib = "../../Data/Netlib/";
               dirMiplib = "../../Data/miplib3/";
          } else {
               directory = ".\\";
#              ifdef COIN_MSVS
	       // Visual Studio builds are deeper
               dirSample = "..\\..\\..\\..\\Data\\Sample\\";
               dirNetlib = "..\\..\\..\\..\\Data\\Netlib\\";
               dirMiplib = "..\\..\\..\\..\\Data\\miplib3\\";
#              else
               dirSample = "..\\..\\Data\\Sample\\";
               dirNetlib = "..\\..\\Data\\Netlib\\";
               dirMiplib = "..\\..\\Data\\miplib3\\";
#              endif
          }
          std::string defaultDirectory = directory;
          std::string importFile = "";
          std::string exportFile = "default.mps";
          std::string importBasisFile = "";
          int basisHasValues = 0;
          int substitution = 3;
          int dualize = 3;  // dualize if looks promising
          std::string exportBasisFile = "default.bas";
          std::string saveFile = "default.prob";
          std::string restoreFile = "default.prob";
          std::string solutionFile = "stdout";
          std::string solutionSaveFile = "solution.file";
          std::string printMask = "";
          CbcOrClpParam parameters[CBCMAXPARAMETERS];
          int numberParameters ;
          establishParams(numberParameters, parameters) ;
          parameters[whichParam(CLP_PARAM_ACTION_BASISIN, numberParameters, parameters)].setStringValue(importBasisFile);
          parameters[whichParam(CLP_PARAM_ACTION_BASISOUT, numberParameters, parameters)].setStringValue(exportBasisFile);
          parameters[whichParam(CLP_PARAM_ACTION_PRINTMASK, numberParameters, parameters)].setStringValue(printMask);
          parameters[whichParam(CLP_PARAM_ACTION_DIRECTORY, numberParameters, parameters)].setStringValue(directory);
          parameters[whichParam(CLP_PARAM_ACTION_DIRSAMPLE, numberParameters, parameters)].setStringValue(dirSample);
          parameters[whichParam(CLP_PARAM_ACTION_DIRNETLIB, numberParameters, parameters)].setStringValue(dirNetlib);
          parameters[whichParam(CBC_PARAM_ACTION_DIRMIPLIB, numberParameters, parameters)].setStringValue(dirMiplib);
          parameters[whichParam(CLP_PARAM_DBL_DUALBOUND, numberParameters, parameters)].setDoubleValue(models->dualBound());
          parameters[whichParam(CLP_PARAM_DBL_DUALTOLERANCE, numberParameters, parameters)].setDoubleValue(models->dualTolerance());
          parameters[whichParam(CLP_PARAM_ACTION_EXPORT, numberParameters, parameters)].setStringValue(exportFile);
          parameters[whichParam(CLP_PARAM_INT_IDIOT, numberParameters, parameters)].setIntValue(doIdiot);
          parameters[whichParam(CLP_PARAM_ACTION_IMPORT, numberParameters, parameters)].setStringValue(importFile);
          parameters[whichParam(CLP_PARAM_INT_SOLVERLOGLEVEL, numberParameters, parameters)].setIntValue(models->logLevel());
          parameters[whichParam(CLP_PARAM_INT_MAXFACTOR, numberParameters, parameters)].setIntValue(models->factorizationFrequency());
          parameters[whichParam(CLP_PARAM_INT_MAXITERATION, numberParameters, parameters)].setIntValue(models->maximumIterations());
          parameters[whichParam(CLP_PARAM_INT_OUTPUTFORMAT, numberParameters, parameters)].setIntValue(outputFormat);
          parameters[whichParam(CLP_PARAM_INT_PRESOLVEPASS, numberParameters, parameters)].setIntValue(preSolve);
          parameters[whichParam(CLP_PARAM_INT_PERTVALUE, numberParameters, parameters)].setIntValue(models->perturbation());
          parameters[whichParam(CLP_PARAM_DBL_PRIMALTOLERANCE, numberParameters, parameters)].setDoubleValue(models->primalTolerance());
          parameters[whichParam(CLP_PARAM_DBL_PRIMALWEIGHT, numberParameters, parameters)].setDoubleValue(models->infeasibilityCost());
          parameters[whichParam(CLP_PARAM_ACTION_RESTORE, numberParameters, parameters)].setStringValue(restoreFile);
          parameters[whichParam(CLP_PARAM_ACTION_SAVE, numberParameters, parameters)].setStringValue(saveFile);
          parameters[whichParam(CLP_PARAM_DBL_TIMELIMIT, numberParameters, parameters)].setDoubleValue(models->maximumSeconds());
          parameters[whichParam(CLP_PARAM_ACTION_SOLUTION, numberParameters, parameters)].setStringValue(solutionFile);
          parameters[whichParam(CLP_PARAM_ACTION_SAVESOL, numberParameters, parameters)].setStringValue(solutionSaveFile);
          parameters[whichParam(CLP_PARAM_INT_SPRINT, numberParameters, parameters)].setIntValue(doSprint);
          parameters[whichParam(CLP_PARAM_INT_SUBSTITUTION, numberParameters, parameters)].setIntValue(substitution);
          parameters[whichParam(CLP_PARAM_INT_DUALIZE, numberParameters, parameters)].setIntValue(dualize);
          parameters[whichParam(CLP_PARAM_DBL_PRESOLVETOLERANCE, numberParameters, parameters)].setDoubleValue(1.0e-8);
          int verbose = 0;

          // total number of commands read
          int numberGoodCommands = 0;
          bool * goodModels = new bool[1];
          goodModels[0] = false;
#ifdef COIN_HAS_ASL
	  ampl_info info;
	  int usingAmpl=0;
	  CoinMessageHandler * generalMessageHandler = models->messageHandler();
	  generalMessageHandler->setPrefix(false);
	  CoinMessages generalMessages = models->messages();
	  char generalPrint[10000];
	  {
	    bool noPrinting_=false;
            memset(&info, 0, sizeof(info));
            if (argc > 2 && !strcmp(argv[2], "-AMPL")) {
	      usingAmpl = 1;
	      // see if log in list
	      noPrinting_ = true;
	      for (int i = 1; i < argc; i++) {
                    if (!strncmp(argv[i], "log", 3)) {
                        const char * equals = strchr(argv[i], '=');
                        if (equals && atoi(equals + 1) > 0) {
			    noPrinting_ = false;
                            info.logLevel = atoi(equals + 1);
                            int log = whichParam(CLP_PARAM_INT_LOGLEVEL, numberParameters, parameters);
                            parameters[log].setIntValue(info.logLevel);
                            // mark so won't be overWritten
                            info.numberRows = -1234567;
                            break;
                        }
                    }
                }

                union {
                    void * voidModel;
                    CoinModel * model;
                } coinModelStart;
                coinModelStart.model = NULL;
                int returnCode = readAmpl(&info, argc, const_cast<char **>(argv), & coinModelStart.voidModel);
                if (returnCode)
                    return returnCode;
                CbcOrClpRead_mode = 2; // so will start with parameters
                // see if log in list (including environment)
                for (int i = 1; i < info.numberArguments; i++) {
                    if (!strcmp(info.arguments[i], "log")) {
                        if (i < info.numberArguments - 1 && atoi(info.arguments[i+1]) > 0)
                            noPrinting_ = false;
                        break;
                    }
                }
                if (noPrinting_) {
                    models->messageHandler()->setLogLevel(0);
                    setCbcOrClpPrinting(false);
                }
                if (!noPrinting_)
                    printf("%d rows, %d columns and %d elements\n",
                           info.numberRows, info.numberColumns, info.numberElements);
                    models->loadProblem(info.numberColumns, info.numberRows, info.starts,
                                        info.rows, info.elements,
                                        info.columnLower, info.columnUpper, info.objective,
                                        info.rowLower, info.rowUpper);
                // If we had a solution use it
                if (info.primalSolution) {
                    models->setColSolution(info.primalSolution);
                }
                // status
                if (info.rowStatus) {
                    unsigned char * statusArray = models->statusArray();
                    int i;
                    for (i = 0; i < info.numberColumns; i++)
                        statusArray[i] = static_cast<unsigned char>(info.columnStatus[i]);
                    statusArray += info.numberColumns;
                    for (i = 0; i < info.numberRows; i++)
                        statusArray[i] = static_cast<unsigned char>(info.rowStatus[i]);
                }
                freeArrays1(&info);
                // modify objective if necessary
                models->setOptimizationDirection(info.direction);
                models->setObjectiveOffset(info.offset);
                if (info.offset) {
                    sprintf(generalPrint, "Ampl objective offset is %g",
                            info.offset);
                    generalMessageHandler->message(CLP_GENERAL, generalMessages)
                    << generalPrint
                    << CoinMessageEol;
                }
                goodModels[0] = true;
                // change argc etc
                argc = info.numberArguments;
                argv = const_cast<const char **>(info.arguments);
            }
        }
#endif

          // Hidden stuff for barrier
          int choleskyType = 0;
          int gamma = 0;
          parameters[whichParam(CLP_PARAM_STR_BARRIERSCALE, numberParameters, parameters)].setCurrentOption(2);
          int scaleBarrier = 2;
          int doKKT = 0;
          int crossover = 2; // do crossover unless quadratic

          int iModel = 0;
          //models[0].scaling(1);
          //models[0].setDualBound(1.0e6);
          //models[0].setDualTolerance(1.0e-7);
          //ClpDualRowSteepest steep;
          //models[0].setDualRowPivotAlgorithm(steep);
#ifdef ABC_INHERIT
          models[0].setDualTolerance(1.0e-6);
          models[0].setPrimalTolerance(1.0e-6);
#endif
          //ClpPrimalColumnSteepest steepP;
          //models[0].setPrimalColumnPivotAlgorithm(steepP);
          std::string field;
          std::cout << "Coin LP version " << CLP_VERSION
                    << ", build " << __DATE__ << std::endl;
          // Print command line
          if (argc > 1) {
               printf("command line - ");
               for (int i = 0; i < argc; i++)
                    printf("%s ", argv[i]);
               printf("\n");
          }

          while (1) {
               // next command
               field = CoinReadGetCommand(argc, argv);

               // exit if null or similar
               if (!field.length()) {
                    if (numberGoodCommands == 1 && goodModels[0]) {
                         // we just had file name - do dual or primal
                         field = "either";
                    } else if (!numberGoodCommands) {
                         // let's give the sucker a hint
                         std::cout
                                   << "Clp takes input from arguments ( - switches to stdin)"
                                   << std::endl
                                   << "Enter ? for list of commands or help" << std::endl;
                         field = "-";
                    } else {
                         break;
                    }
               }

               // see if ? at end
               int numberQuery = 0;
               if (field != "?" && field != "???") {
                    size_t length = field.length();
                    size_t i;
                    for (i = length - 1; i > 0; i--) {
                         if (field[i] == '?')
                              numberQuery++;
                         else
                              break;
                    }
                    field = field.substr(0, length - numberQuery);
               }
               // find out if valid command
               int iParam;
               int numberMatches = 0;
               int firstMatch = -1;
               for ( iParam = 0; iParam < numberParameters; iParam++ ) {
                    int match = parameters[iParam].matches(field);
                    if (match == 1) {
                         numberMatches = 1;
                         firstMatch = iParam;
                         break;
                    } else {
                         if (match && firstMatch < 0)
                              firstMatch = iParam;
                         numberMatches += match >> 1;
                    }
               }
	       ClpSimplex * thisModel=models+iModel;
               if (iParam < numberParameters && !numberQuery) {
                    // found
                    CbcOrClpParam found = parameters[iParam];
                    CbcOrClpParameterType type = found.type();
                    int valid;
                    numberGoodCommands++;
                    if (type == CBC_PARAM_GENERALQUERY) {
                         std::cout << "In argument list keywords have leading - "
                                   ", -stdin or just - switches to stdin" << std::endl;
                         std::cout << "One command per line (and no -)" << std::endl;
                         std::cout << "abcd? gives list of possibilities, if only one + explanation" << std::endl;
                         std::cout << "abcd?? adds explanation, if only one fuller help" << std::endl;
                         std::cout << "abcd without value (where expected) gives current value" << std::endl;
                         std::cout << "abcd value sets value" << std::endl;
                         std::cout << "Commands are:" << std::endl;
                         int maxAcross = 10;
                         bool evenHidden = false;
                         int printLevel =
                              parameters[whichParam(CLP_PARAM_STR_ALLCOMMANDS,
                                                    numberParameters, parameters)].currentOptionAsInteger();
                         int convertP[] = {2, 1, 0};
                         printLevel = convertP[printLevel];
                         if ((verbose & 8) != 0) {
                              // even hidden
                              evenHidden = true;
                              verbose &= ~8;
                         }
#ifdef COIN_HAS_ASL
			 if (verbose < 4 && usingAmpl)
			   verbose += 4;
#endif
                         if (verbose)
                              maxAcross = 1;
                         int limits[] = {1, 101, 201, 301, 401};
                         std::vector<std::string> types;
                         types.push_back("Double parameters:");
                         types.push_back("Int parameters:");
                         types.push_back("Keyword parameters:");
                         types.push_back("Actions or string parameters:");
                         int iType;
                         for (iType = 0; iType < 4; iType++) {
                              int across = 0;
                              int lengthLine = 0;
                              if ((verbose % 4) != 0)
                                   std::cout << std::endl;
                              std::cout << types[iType] << std::endl;
                              if ((verbose & 2) != 0)
                                   std::cout << std::endl;
                              for ( iParam = 0; iParam < numberParameters; iParam++ ) {
                                   int type = parameters[iParam].type();
                                   //printf("%d type %d limits %d %d display %d\n",iParam,
                                   //   type,limits[iType],limits[iType+1],parameters[iParam].displayThis());
                                   if ((parameters[iParam].displayThis() >= printLevel || evenHidden) &&
                                             type >= limits[iType]
                                             && type < limits[iType+1]) {
                                        if (!across) {
                                             if ((verbose & 2) != 0)
                                                  std::cout << "Command ";
                                        }
                                        int length = parameters[iParam].lengthMatchName() + 1;
                                        if (lengthLine + length > 80) {
                                             std::cout << std::endl;
                                             across = 0;
                                             lengthLine = 0;
                                        }
                                        std::cout << " " << parameters[iParam].matchName();
                                        lengthLine += length ;
                                        across++;
                                        if (across == maxAcross) {
                                             across = 0;
                                             if (verbose) {
                                                  // put out description as well
                                                  if ((verbose & 1) != 0)
                                                       std::cout << parameters[iParam].shortHelp();
                                                  std::cout << std::endl;
                                                  if ((verbose & 2) != 0) {
                                                       std::cout << "---- description" << std::endl;
                                                       parameters[iParam].printLongHelp();
                                                       std::cout << "----" << std::endl << std::endl;
                                                  }
                                             } else {
                                                  std::cout << std::endl;
                                             }
                                        }
                                   }
                              }
                              if (across)
                                   std::cout << std::endl;
                         }
                    } else if (type == CBC_PARAM_FULLGENERALQUERY) {
                         std::cout << "Full list of commands is:" << std::endl;
                         int maxAcross = 5;
                         int limits[] = {1, 101, 201, 301, 401};
                         std::vector<std::string> types;
                         types.push_back("Double parameters:");
                         types.push_back("Int parameters:");
                         types.push_back("Keyword parameters and others:");
                         types.push_back("Actions:");
                         int iType;
                         for (iType = 0; iType < 4; iType++) {
                              int across = 0;
                              std::cout << types[iType] << std::endl;
                              for ( iParam = 0; iParam < numberParameters; iParam++ ) {
                                   int type = parameters[iParam].type();
                                   if (type >= limits[iType]
                                             && type < limits[iType+1]) {
                                        if (!across)
                                             std::cout << "  ";
                                        std::cout << parameters[iParam].matchName() << "  ";
                                        across++;
                                        if (across == maxAcross) {
                                             std::cout << std::endl;
                                             across = 0;
                                        }
                                   }
                              }
                              if (across)
                                   std::cout << std::endl;
                         }
                    } else if (type < 101) {
                         // get next field as double
                         double value = CoinReadGetDoubleField(argc, argv, &valid);
                         if (!valid) {
                              parameters[iParam].setDoubleParameter(thisModel, value);
                         } else if (valid == 1) {
                              std::cout << " is illegal for double parameter " << parameters[iParam].name() << " value remains " <<
                                        parameters[iParam].doubleValue() << std::endl;
                         } else {
                              std::cout << parameters[iParam].name() << " has value " <<
                                        parameters[iParam].doubleValue() << std::endl;
                         }
                    } else if (type < 201) {
                         // get next field as int
                         int value = CoinReadGetIntField(argc, argv, &valid);
                         if (!valid) {
                              if (parameters[iParam].type() == CLP_PARAM_INT_PRESOLVEPASS)
                                   preSolve = value;
                              else if (parameters[iParam].type() == CLP_PARAM_INT_IDIOT)
                                   doIdiot = value;
                              else if (parameters[iParam].type() == CLP_PARAM_INT_SPRINT)
                                   doSprint = value;
                              else if (parameters[iParam].type() == CLP_PARAM_INT_OUTPUTFORMAT)
                                   outputFormat = value;
                              else if (parameters[iParam].type() == CLP_PARAM_INT_SLPVALUE)
                                   slpValue = value;
                              else if (parameters[iParam].type() == CLP_PARAM_INT_CPP)
                                   cppValue = value;
                              else if (parameters[iParam].type() == CLP_PARAM_INT_PRESOLVEOPTIONS)
                                   presolveOptions = value;
                              else if (parameters[iParam].type() == CLP_PARAM_INT_PRINTOPTIONS)
                                   printOptions = value;
                              else if (parameters[iParam].type() == CLP_PARAM_INT_SUBSTITUTION)
                                   substitution = value;
                              else if (parameters[iParam].type() == CLP_PARAM_INT_DUALIZE)
                                   dualize = value;
                              else if (parameters[iParam].type() == CLP_PARAM_INT_VERBOSE)
                                   verbose = value;
                              parameters[iParam].setIntParameter(thisModel, value);
                         } else if (valid == 1) {
                              std::cout << " is illegal for integer parameter " << parameters[iParam].name() << " value remains " <<
                                        parameters[iParam].intValue() << std::endl;
                         } else {
                              std::cout << parameters[iParam].name() << " has value " <<
                                        parameters[iParam].intValue() << std::endl;
                         }
                    } else if (type < 301) {
                         // one of several strings
                         std::string value = CoinReadGetString(argc, argv);
                         int action = parameters[iParam].parameterOption(value);
                         if (action < 0) {
                              if (value != "EOL") {
                                   // no match
                                   parameters[iParam].printOptions();
                              } else {
                                   // print current value
                                   std::cout << parameters[iParam].name() << " has value " <<
                                             parameters[iParam].currentOption() << std::endl;
                              }
                         } else {
                              parameters[iParam].setCurrentOption(action);
                              // for now hard wired
                              switch (type) {
                              case CLP_PARAM_STR_DIRECTION:
				if (action == 0) {
                                        models[iModel].setOptimizationDirection(1);
#ifdef ABC_INHERIT
                                        thisModel->setOptimizationDirection(1);
#endif
				}  else if (action == 1) {
                                        models[iModel].setOptimizationDirection(-1);
#ifdef ABC_INHERIT
                                        thisModel->setOptimizationDirection(-1);
#endif
				}  else {
                                        models[iModel].setOptimizationDirection(0);
#ifdef ABC_INHERIT
                                        thisModel->setOptimizationDirection(0);
#endif
				}
                                   break;
                              case CLP_PARAM_STR_DUALPIVOT:
                                   if (action == 0) {
                                        ClpDualRowSteepest steep(3);
                                        thisModel->setDualRowPivotAlgorithm(steep);
#ifdef ABC_INHERIT
                                        AbcDualRowSteepest steep2(3);
                                        models[iModel].setDualRowPivotAlgorithm(steep2);
#endif
                                   } else if (action == 1) {
                                        //ClpDualRowDantzig dantzig;
                                        ClpDualRowDantzig dantzig;
                                        thisModel->setDualRowPivotAlgorithm(dantzig);
#ifdef ABC_INHERIT
                                        AbcDualRowDantzig dantzig2;
                                        models[iModel].setDualRowPivotAlgorithm(dantzig2);
#endif
                                   } else if (action == 2) {
                                        // partial steep
                                        ClpDualRowSteepest steep(2);
                                        thisModel->setDualRowPivotAlgorithm(steep);
#ifdef ABC_INHERIT
                                        AbcDualRowSteepest steep2(2);
                                        models[iModel].setDualRowPivotAlgorithm(steep2);
#endif
                                   } else {
                                        ClpDualRowSteepest steep;
                                        thisModel->setDualRowPivotAlgorithm(steep);
#ifdef ABC_INHERIT
                                        AbcDualRowSteepest steep2;
                                        models[iModel].setDualRowPivotAlgorithm(steep2);
#endif
                                   }
                                   break;
                              case CLP_PARAM_STR_PRIMALPIVOT:
                                   if (action == 0) {
                                        ClpPrimalColumnSteepest steep(3);
                                        thisModel->setPrimalColumnPivotAlgorithm(steep);
                                   } else if (action == 1) {
                                        ClpPrimalColumnSteepest steep(0);
                                        thisModel->setPrimalColumnPivotAlgorithm(steep);
                                   } else if (action == 2) {
                                        ClpPrimalColumnDantzig dantzig;
                                        thisModel->setPrimalColumnPivotAlgorithm(dantzig);
                                   } else if (action == 3) {
                                        ClpPrimalColumnSteepest steep(4);
                                        thisModel->setPrimalColumnPivotAlgorithm(steep);
                                   } else if (action == 4) {
                                        ClpPrimalColumnSteepest steep(1);
                                        thisModel->setPrimalColumnPivotAlgorithm(steep);
                                   } else if (action == 5) {
                                        ClpPrimalColumnSteepest steep(2);
                                        thisModel->setPrimalColumnPivotAlgorithm(steep);
                                   } else if (action == 6) {
                                        ClpPrimalColumnSteepest steep(10);
                                        thisModel->setPrimalColumnPivotAlgorithm(steep);
                                   }
                                   break;
                              case CLP_PARAM_STR_SCALING:
                                   thisModel->scaling(action);
                                   break;
                              case CLP_PARAM_STR_AUTOSCALE:
                                   thisModel->setAutomaticScaling(action != 0);
                                   break;
                              case CLP_PARAM_STR_SPARSEFACTOR:
                                   thisModel->setSparseFactorization((1 - action) != 0);
                                   break;
                              case CLP_PARAM_STR_BIASLU:
                                   thisModel->factorization()->setBiasLU(action);
                                   break;
                              case CLP_PARAM_STR_PERTURBATION:
                                   if (action == 0)
                                        thisModel->setPerturbation(50);
                                   else
                                        thisModel->setPerturbation(100);
                                   break;
                              case CLP_PARAM_STR_ERRORSALLOWED:
                                   allowImportErrors = action;
                                   break;
                              case CLP_PARAM_STR_ABCWANTED:
                                   models[iModel].setAbcState(action);
                                   break;
                              case CLP_PARAM_STR_INTPRINT:
                                   printMode = action;
                                   break;
                              case CLP_PARAM_STR_KEEPNAMES:
                                   keepImportNames = 1 - action;
                                   break;
                              case CLP_PARAM_STR_PRESOLVE:
                                   if (action == 0)
                                        preSolve = DEFAULT_PRESOLVE_PASSES;
                                   else if (action == 1)
                                        preSolve = 0;
                                   else if (action == 2)
                                        preSolve = 10;
                                   else
                                        preSolveFile = true;
                                   break;
                              case CLP_PARAM_STR_PFI:
                                   thisModel->factorization()->setForrestTomlin(action == 0);
                                   break;
                              case CLP_PARAM_STR_FACTORIZATION:
                                   models[iModel].factorization()->forceOtherFactorization(action);
#ifdef ABC_INHERIT
                                   thisModel->factorization()->forceOtherFactorization(action);
#endif
                                   break;
                              case CLP_PARAM_STR_CRASH:
                                   doCrash = action;
                                   break;
                              case CLP_PARAM_STR_VECTOR:
                                   doVector = action;
                                   break;
                              case CLP_PARAM_STR_MESSAGES:
                                   models[iModel].messageHandler()->setPrefix(action != 0);
#ifdef ABC_INHERIT
                                   thisModel->messageHandler()->setPrefix(action != 0);
#endif
                                   break;
                              case CLP_PARAM_STR_CHOLESKY:
                                   choleskyType = action;
                                   break;
                              case CLP_PARAM_STR_GAMMA:
                                   gamma = action;
                                   break;
                              case CLP_PARAM_STR_BARRIERSCALE:
                                   scaleBarrier = action;
                                   break;
                              case CLP_PARAM_STR_KKT:
                                   doKKT = action;
                                   break;
                              case CLP_PARAM_STR_CROSSOVER:
                                   crossover = action;
                                   break;
                              default:
                                   //abort();
                                   break;
                              }
                         }
                    } else {
                         // action
		         if (type == CLP_PARAM_ACTION_EXIT) {
#ifdef COIN_HAS_ASL
			   if (usingAmpl) {
			     writeAmpl(&info);
			     freeArrays2(&info);
			     freeArgs(&info);
			   }
#endif
			   break; // stop all
			 }
                         switch (type) {
                         case CLP_PARAM_ACTION_DUALSIMPLEX:
                         case CLP_PARAM_ACTION_PRIMALSIMPLEX:
                         case CLP_PARAM_ACTION_EITHERSIMPLEX:
                         case CLP_PARAM_ACTION_BARRIER:
                              // synonym for dual
                         case CBC_PARAM_ACTION_BAB:
                              if (goodModels[iModel]) {
				if (type==CLP_PARAM_ACTION_EITHERSIMPLEX||
				    type==CBC_PARAM_ACTION_BAB)
				  models[iModel].setMoreSpecialOptions(16384|
								       models[iModel].moreSpecialOptions());
                                   double objScale =
                                        parameters[whichParam(CLP_PARAM_DBL_OBJSCALE2, numberParameters, parameters)].doubleValue();
                                   if (objScale != 1.0) {
                                        int iColumn;
                                        int numberColumns = models[iModel].numberColumns();
                                        double * dualColumnSolution =
                                             models[iModel].dualColumnSolution();
                                        ClpObjective * obj = models[iModel].objectiveAsObject();
                                        assert(dynamic_cast<ClpLinearObjective *> (obj));
                                        double offset;
                                        double * objective = obj->gradient(NULL, NULL, offset, true);
                                        for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                                             dualColumnSolution[iColumn] *= objScale;
                                             objective[iColumn] *= objScale;;
                                        }
                                        int iRow;
                                        int numberRows = models[iModel].numberRows();
                                        double * dualRowSolution =
                                             models[iModel].dualRowSolution();
                                        for (iRow = 0; iRow < numberRows; iRow++)
                                             dualRowSolution[iRow] *= objScale;
                                        models[iModel].setObjectiveOffset(objScale * models[iModel].objectiveOffset());
                                   }
                                   ClpSolve::SolveType method;
                                   ClpSolve::PresolveType presolveType;
                                   ClpSolve solveOptions;
#ifndef ABC_INHERIT
                                   ClpSimplex * model2 = models + iModel;
#else
                                   AbcSimplex * model2 = models + iModel;
				   if (type==CLP_PARAM_ACTION_EITHERSIMPLEX||
				       type==CBC_PARAM_ACTION_BAB)
				     solveOptions.setSpecialOption(3,0); // allow +-1
#endif
				   if (dualize==4) { 
				     solveOptions.setSpecialOption(4, 77);
				     dualize=0;
				   }
                                   if (dualize) {
                                        bool tryIt = true;
                                        double fractionColumn = 1.0;
                                        double fractionRow = 1.0;
                                        if (dualize == 3) {
                                             dualize = 1;
                                             int numberColumns = model2->numberColumns();
                                             int numberRows = model2->numberRows();
#ifndef ABC_INHERIT
                                             if (numberRows < 50000 || 5 * numberColumns > numberRows) {
#else
                                             if (numberRows < 500 || 4 * numberColumns > numberRows) {
#endif
                                                  tryIt = false;
                                             } else {
                                                  fractionColumn = 0.1;
                                                  fractionRow = 0.3;
                                             }
                                        }
                                        if (tryIt) {
					  ClpSimplex * thisModel=model2;
                                             thisModel = static_cast<ClpSimplexOther *> (thisModel)->dualOfModel(fractionRow, fractionColumn);
                                             if (thisModel) {
                                                  printf("Dual of model has %d rows and %d columns\n",
                                                         thisModel->numberRows(), thisModel->numberColumns());
                                                  thisModel->setOptimizationDirection(1.0);
#ifndef ABC_INHERIT
						  model2=thisModel;
#else
						  int abcState=model2->abcState();
						  model2=new AbcSimplex(*thisModel);
						  model2->setAbcState(abcState);
						  delete thisModel;
#endif
                                             } else {
                                                  thisModel = models + iModel;
                                                  dualize = 0;
					     }
                                        } else {
                                             dualize = 0;
                                        }
                                   }
                                   if (preSolveFile)
                                        presolveOptions |= 0x40000000;
                                   solveOptions.setPresolveActions(presolveOptions);
                                   solveOptions.setSubstitution(substitution);
                                   if (preSolve != DEFAULT_PRESOLVE_PASSES && preSolve) {
                                        presolveType = ClpSolve::presolveNumber;
                                        if (preSolve < 0) {
                                             preSolve = - preSolve;
                                             if (preSolve <= 100) {
                                                  presolveType = ClpSolve::presolveNumber;
                                                  printf("Doing %d presolve passes - picking up non-costed slacks\n",
                                                         preSolve);
                                                  solveOptions.setDoSingletonColumn(true);
                                             } else {
                                                  preSolve -= 100;
                                                  presolveType = ClpSolve::presolveNumberCost;
                                                  printf("Doing %d presolve passes - picking up costed slacks\n",
                                                         preSolve);
                                             }
                                        }
                                   } else if (preSolve) {
                                        presolveType = ClpSolve::presolveOn;
                                   } else {
                                        presolveType = ClpSolve::presolveOff;
                                   }
                                   solveOptions.setPresolveType(presolveType, preSolve);
                                   if (type == CLP_PARAM_ACTION_DUALSIMPLEX ||
                                             type == CBC_PARAM_ACTION_BAB) {
                                        method = ClpSolve::useDual;
                                   } else if (type == CLP_PARAM_ACTION_PRIMALSIMPLEX) {
                                        method = ClpSolve::usePrimalorSprint;
                                   } else if (type == CLP_PARAM_ACTION_EITHERSIMPLEX) {
                                        method = ClpSolve::automatic;
                                   } else {
                                        method = ClpSolve::useBarrier;
#ifdef ABC_INHERIT
                                        if (doIdiot > 0) 
                                             solveOptions.setSpecialOption(1, 2, doIdiot); // dense threshold
#endif
                                        if (crossover == 1) {
                                             method = ClpSolve::useBarrierNoCross;
                                        } else if (crossover == 2) {
                                             ClpObjective * obj = models[iModel].objectiveAsObject();
                                             if (obj->type() > 1) {
                                                  method = ClpSolve::useBarrierNoCross;
                                                  presolveType = ClpSolve::presolveOff;
                                                  solveOptions.setPresolveType(presolveType, preSolve);
                                             }
                                        }
                                   }
                                   solveOptions.setSolveType(method);
                                   solveOptions.setSpecialOption(5, printOptions & 1);
                                   if (doVector) {
                                        ClpMatrixBase * matrix = models[iModel].clpMatrix();
                                        if (dynamic_cast< ClpPackedMatrix*>(matrix)) {
                                             ClpPackedMatrix * clpMatrix = dynamic_cast< ClpPackedMatrix*>(matrix);
                                             clpMatrix->makeSpecialColumnCopy();
                                        }
                                   }
                                   if (method == ClpSolve::useDual) {
                                        // dual
                                        if (doCrash)
                                             solveOptions.setSpecialOption(0, 1, doCrash); // crash
                                        else if (doIdiot)
                                             solveOptions.setSpecialOption(0, 2, doIdiot); // possible idiot
                                   } else if (method == ClpSolve::usePrimalorSprint) {
                                        // primal
                                        // if slp turn everything off
                                        if (slpValue > 0) {
                                             doCrash = false;
                                             doSprint = 0;
                                             doIdiot = -1;
                                             solveOptions.setSpecialOption(1, 10, slpValue); // slp
                                             method = ClpSolve::usePrimal;
                                        }
                                        if (doCrash) {
                                             solveOptions.setSpecialOption(1, 1, doCrash); // crash
                                        } else if (doSprint > 0) {
                                             // sprint overrides idiot
                                             solveOptions.setSpecialOption(1, 3, doSprint); // sprint
                                        } else if (doIdiot > 0) {
                                             solveOptions.setSpecialOption(1, 2, doIdiot); // idiot
                                        } else if (slpValue <= 0) {
                                             if (doIdiot == 0) {
                                                  if (doSprint == 0)
                                                       solveOptions.setSpecialOption(1, 4); // all slack
                                                  else
                                                       solveOptions.setSpecialOption(1, 9); // all slack or sprint
                                             } else {
                                                  if (doSprint == 0)
                                                       solveOptions.setSpecialOption(1, 8); // all slack or idiot
                                                  else
                                                       solveOptions.setSpecialOption(1, 7); // initiative
                                             }
                                        }
                                        if (basisHasValues == -1)
                                             solveOptions.setSpecialOption(1, 11); // switch off values
                                   } else if (method == ClpSolve::useBarrier || method == ClpSolve::useBarrierNoCross) {
                                        int barrierOptions = choleskyType;
                                        if (scaleBarrier) {
                                             if ((scaleBarrier & 1) != 0)
                                                  barrierOptions |= 8;
                                             barrierOptions |= 2048 * (scaleBarrier >> 1);
                                        }
                                        if (doKKT)
                                             barrierOptions |= 16;
                                        if (gamma)
                                             barrierOptions |= 32 * gamma;
                                        if (crossover == 3)
                                             barrierOptions |= 256; // try presolve in crossover
                                        solveOptions.setSpecialOption(4, barrierOptions);
                                   }
                                   int status;
                                   if (cppValue >= 0) {
                                        // generate code
                                        FILE * fp = fopen("user_driver.cpp", "w");
                                        if (fp) {
                                             // generate enough to do solveOptions
                                             model2->generateCpp(fp);
                                             solveOptions.generateCpp(fp);
                                             fclose(fp);
                                             // now call generate code
                                             generateCode("user_driver.cpp", cppValue);
                                        } else {
                                             std::cout << "Unable to open file user_driver.cpp" << std::endl;
                                        }
                                   }
#ifdef CLP_MULTIPLE_FACTORIZATIONS
                                   int denseCode = parameters[whichParam(CBC_PARAM_INT_DENSE, numberParameters, parameters)].intValue();
				   if (denseCode!=-1)
				     model2->factorization()->setGoDenseThreshold(denseCode);
                                   int smallCode = parameters[whichParam(CBC_PARAM_INT_SMALLFACT, numberParameters, parameters)].intValue();
				   if (smallCode!=-1)
				     model2->factorization()->setGoSmallThreshold(smallCode);
                                   model2->factorization()->goDenseOrSmall(model2->numberRows());
#endif
                                   try {
                                        status = model2->initialSolve(solveOptions);
#ifndef NDEBUG
					// if infeasible check ray
					if (model2->status()==1) {
					  ClpSimplex * simplex = model2;
					  if(simplex->ray()) {
					    // make sure we use non-scaled versions
					    ClpPackedMatrix * saveMatrix = simplex->swapScaledMatrix(NULL);
					    double * saveScale = simplex->swapRowScale(NULL);
					    // could use existing arrays
					    int numberRows=simplex->numberRows();
					    int numberColumns=simplex->numberColumns();
					    double * farkas = new double [2*numberColumns+numberRows];
					    double * bound = farkas + numberColumns;
					    double * effectiveRhs = bound + numberColumns;
					    // get ray as user would
					    double * ray = simplex->infeasibilityRay();
					    // get farkas row
					    memset(farkas,0,(2*numberColumns+numberRows)*sizeof(double));
					    simplex->transposeTimes(-1.0,ray,farkas);
					    // Put nonzero bounds in bound
					    const double * columnLower = simplex->columnLower();
					    const double * columnUpper = simplex->columnUpper();
					    int numberBad=0;
					    for (int i=0;i<numberColumns;i++) {
					      double value = farkas[i];
					      double boundValue=0.0;
					      if (simplex->getStatus(i)==ClpSimplex::basic) {
						// treat as zero if small
						if (fabs(value)<1.0e-8) {
						  value=0.0;
						  farkas[i]=0.0;
						}
						if (value) {
						  //printf("basic %d direction %d farkas %g\n",
						  //	   i,simplex->directionOut(),value);
						  if (value<0.0) 
						    boundValue=columnLower[i];
						  else
						    boundValue=columnUpper[i];
						}
					      } else if (fabs(value)>1.0e-10) {
						if (value<0.0) 
						  boundValue=columnLower[i];
						else
						  boundValue=columnUpper[i];
					      }
					      bound[i]=boundValue;
					      if (fabs(boundValue)>1.0e10)
						numberBad++;
					    }
					    const double * rowLower = simplex->rowLower();
					    const double * rowUpper = simplex->rowUpper();
					    //int pivotRow = simplex->spareIntArray_[3];
					    //bool badPivot=pivotRow<0;
					    for (int i=0;i<numberRows;i++) {
					      double value = ray[i];
					      double rhsValue=0.0;
					      if (simplex->getRowStatus(i)==ClpSimplex::basic) {
						// treat as zero if small
						if (fabs(value)<1.0e-8) {
						  value=0.0;
						  ray[i]=0.0;
						}
						if (value) {
						  //printf("row basic %d direction %d ray %g\n",
						  //	   i,simplex->directionOut(),value);
						  if (value<0.0) 
						    rhsValue=rowLower[i];
						  else
						    rhsValue=rowUpper[i];
						}
					      } else if (fabs(value)>1.0e-10) {
						if (value<0.0) 
						  rhsValue=rowLower[i];
						else
						  rhsValue=rowUpper[i];
					      }
					      effectiveRhs[i]=rhsValue;
					      if (fabs(effectiveRhs[i])>1.0e10)
						printf("Large rhs row %d %g\n",
						       i,effectiveRhs[i]);
					    }
					    simplex->times(-1.0,bound,effectiveRhs);
					    double bSum=0.0;
					    for (int i=0;i<numberRows;i++) {
					      bSum += effectiveRhs[i]*ray[i];
					      if (fabs(effectiveRhs[i])>1.0e10)
						printf("Large rhs row %d %g after\n",
						       i,effectiveRhs[i]);
					    }
					    if (numberBad||bSum>1.0e-6) {
					      printf("Bad infeasibility ray %g  - %d bad\n",
						     bSum,numberBad);
					    } else {
					      //printf("Good ray - infeasibility %g\n",
					      //     -bSum);
					    }
					    delete [] ray;
					    delete [] farkas;
					    simplex->swapRowScale(saveScale);
					    simplex->swapScaledMatrix(saveMatrix);
					  } else {
					    //printf("No dual ray\n");
					  }
					}
#endif
                                   } catch (CoinError e) {
                                        e.print();
                                        status = -1;
                                   }
                                   if (dualize) {
				     ClpSimplex * thisModel=models+iModel;
                                        int returnCode = static_cast<ClpSimplexOther *> (thisModel)->restoreFromDual(model2);
                                        if (model2->status() == 3)
                                             returnCode = 0;
                                        delete model2;
                                        if (returnCode && dualize != 2) {
                                             currentModel = models + iModel;
                                             // register signal handler
                                             signal(SIGINT, signal_handler);
                                             thisModel->primal(1);
                                             currentModel = NULL;
                                        }
					// switch off (user can switch back on)
					parameters[whichParam(CLP_PARAM_INT_DUALIZE, 
							      numberParameters, parameters)].setIntValue(dualize);
                                   }
                                   if (status >= 0)
                                        basisHasValues = 1;
                              } else {
                                   std::cout << "** Current model not valid" << std::endl;
                              }
                              break;
                         case CLP_PARAM_ACTION_STATISTICS:
                              if (goodModels[iModel]) {
                                   // If presolve on look at presolved
                                   bool deleteModel2 = false;
                                   ClpSimplex * model2 = models + iModel;
                                   if (preSolve) {
                                        ClpPresolve pinfo;
                                        int presolveOptions2 = presolveOptions&~0x40000000;
                                        if ((presolveOptions2 & 0xffff) != 0)
                                             pinfo.setPresolveActions(presolveOptions2);
                                        pinfo.setSubstitution(substitution);
                                        if ((printOptions & 1) != 0)
                                             pinfo.statistics();
                                        double presolveTolerance =
                                             parameters[whichParam(CLP_PARAM_DBL_PRESOLVETOLERANCE, numberParameters, parameters)].doubleValue();
                                        model2 =
                                             pinfo.presolvedModel(models[iModel], presolveTolerance,
                                                                  true, preSolve);
                                        if (model2) {
                                             printf("Statistics for presolved model\n");
                                             deleteModel2 = true;
                                        } else {
                                             printf("Presolved model looks infeasible - will use unpresolved\n");
                                             model2 = models + iModel;
                                        }
                                   } else {
                                        printf("Statistics for unpresolved model\n");
                                        model2 =  models + iModel;
                                   }
                                   statistics(models + iModel, model2);
                                   if (deleteModel2)
                                        delete model2;
                              } else {
                                   std::cout << "** Current model not valid" << std::endl;
                              }
                              break;
                         case CLP_PARAM_ACTION_TIGHTEN:
                              if (goodModels[iModel]) {
                                   int numberInfeasibilities = models[iModel].tightenPrimalBounds();
                                   if (numberInfeasibilities)
                                        std::cout << "** Analysis indicates model infeasible" << std::endl;
                              } else {
                                   std::cout << "** Current model not valid" << std::endl;
                              }
                              break;
                         case CLP_PARAM_ACTION_PLUSMINUS:
                              if (goodModels[iModel]) {
                                   ClpMatrixBase * saveMatrix = models[iModel].clpMatrix();
                                   ClpPackedMatrix* clpMatrix =
                                        dynamic_cast< ClpPackedMatrix*>(saveMatrix);
                                   if (clpMatrix) {
                                        ClpPlusMinusOneMatrix * newMatrix = new ClpPlusMinusOneMatrix(*(clpMatrix->matrix()));
                                        if (newMatrix->getIndices()) {
                                             models[iModel].replaceMatrix(newMatrix);
                                             delete saveMatrix;
                                             std::cout << "Matrix converted to +- one matrix" << std::endl;
                                        } else {
                                             std::cout << "Matrix can not be converted to +- 1 matrix" << std::endl;
                                        }
                                   } else {
                                        std::cout << "Matrix not a ClpPackedMatrix" << std::endl;
                                   }
                              } else {
                                   std::cout << "** Current model not valid" << std::endl;
                              }
                              break;
                         case CLP_PARAM_ACTION_NETWORK:
                              if (goodModels[iModel]) {
                                   ClpMatrixBase * saveMatrix = models[iModel].clpMatrix();
                                   ClpPackedMatrix* clpMatrix =
                                        dynamic_cast< ClpPackedMatrix*>(saveMatrix);
                                   if (clpMatrix) {
                                        ClpNetworkMatrix * newMatrix = new ClpNetworkMatrix(*(clpMatrix->matrix()));
                                        if (newMatrix->getIndices()) {
                                             models[iModel].replaceMatrix(newMatrix);
                                             delete saveMatrix;
                                             std::cout << "Matrix converted to network matrix" << std::endl;
                                        } else {
                                             std::cout << "Matrix can not be converted to network matrix" << std::endl;
                                        }
                                   } else {
                                        std::cout << "Matrix not a ClpPackedMatrix" << std::endl;
                                   }
                              } else {
                                   std::cout << "** Current model not valid" << std::endl;
                              }
                              break;
                         case CLP_PARAM_ACTION_IMPORT: {
                              // get next field
                              field = CoinReadGetString(argc, argv);
                              if (field == "$") {
                                   field = parameters[iParam].stringValue();
                              } else if (field == "EOL") {
                                   parameters[iParam].printString();
                                   break;
                              } else {
                                   parameters[iParam].setStringValue(field);
                              }
                              std::string fileName;
                              bool canOpen = false;
                              // See if gmpl file
                              int gmpl = 0;
                              std::string gmplData;
                              if (field == "-") {
                                   // stdin
                                   canOpen = true;
                                   fileName = "-";
                              } else {
                                   // See if .lp
                                   {
                                        const char * c_name = field.c_str();
                                        size_t length = strlen(c_name);
                                        if (length > 3 && !strncmp(c_name + length - 3, ".lp", 3))
                                             gmpl = -1; // .lp
                                   }
                                   bool absolutePath;
                                   if (dirsep == '/') {
                                        // non Windows (or cygwin)
                                        absolutePath = (field[0] == '/');
                                   } else {
                                        //Windows (non cycgwin)
                                        absolutePath = (field[0] == '\\');
                                        // but allow for :
                                        if (strchr(field.c_str(), ':'))
                                             absolutePath = true;
                                   }
                                   if (absolutePath) {
                                        fileName = field;
                                        size_t length = field.size();
                                        size_t percent = field.find('%');
                                        if (percent < length && percent > 0) {
                                             gmpl = 1;
                                             fileName = field.substr(0, percent);
                                             gmplData = field.substr(percent + 1);
                                             if (percent < length - 1)
                                                  gmpl = 2; // two files
                                             printf("GMPL model file %s and data file %s\n",
                                                    fileName.c_str(), gmplData.c_str());
                                        }
                                   } else if (field[0] == '~') {
                                        char * environVar = getenv("HOME");
                                        if (environVar) {
                                             std::string home(environVar);
                                             field = field.erase(0, 1);
                                             fileName = home + field;
                                        } else {
                                             fileName = field;
                                        }
                                   } else {
                                        fileName = directory + field;
                                        // See if gmpl (model & data) - or even lp file
                                        size_t length = field.size();
                                        size_t percent = field.find('%');
                                        if (percent < length && percent > 0) {
                                             gmpl = 1;
                                             fileName = directory + field.substr(0, percent);
                                             gmplData = directory + field.substr(percent + 1);
                                             if (percent < length - 1)
                                                  gmpl = 2; // two files
                                             printf("GMPL model file %s and data file %s\n",
                                                    fileName.c_str(), gmplData.c_str());
                                        }
                                   }
                                   std::string name = fileName;
                                   if (fileCoinReadable(name)) {
                                        // can open - lets go for it
                                        canOpen = true;
                                        if (gmpl == 2) {
                                             FILE *fp;
                                             fp = fopen(gmplData.c_str(), "r");
                                             if (fp) {
                                                  fclose(fp);
                                             } else {
                                                  canOpen = false;
                                                  std::cout << "Unable to open file " << gmplData << std::endl;
                                             }
                                        }
                                   } else {
                                        std::cout << "Unable to open file " << fileName << std::endl;
                                   }
                              }
                              if (canOpen) {
                                   int status;
                                   if (!gmpl)
                                        status = models[iModel].readMps(fileName.c_str(),
                                                                        keepImportNames != 0,
                                                                        allowImportErrors != 0);
                                   else if (gmpl > 0)
                                        status = models[iModel].readGMPL(fileName.c_str(),
                                                                         (gmpl == 2) ? gmplData.c_str() : NULL,
                                                                         keepImportNames != 0);
                                   else
#ifdef KILL_ZERO_READLP
				     status = models[iModel].readLp(fileName.c_str(), models[iModel].getSmallElementValue());
#else
				     status = models[iModel].readLp(fileName.c_str(), 1.0e-12);
#endif
                                   if (!status || (status > 0 && allowImportErrors)) {
                                        goodModels[iModel] = true;
                                        // sets to all slack (not necessary?)
                                        thisModel->createStatus();
                                        time2 = CoinCpuTime();
                                        totalTime += time2 - time1;
                                        time1 = time2;
                                        // Go to canned file if just input file
                                        if (CbcOrClpRead_mode == 2 && argc == 2) {
                                             // only if ends .mps
                                             char * find = const_cast<char *>(strstr(fileName.c_str(), ".mps"));
                                             if (find && find[4] == '\0') {
                                                  find[1] = 'p';
                                                  find[2] = 'a';
                                                  find[3] = 'r';
                                                  FILE *fp = fopen(fileName.c_str(), "r");
                                                  if (fp) {
                                                       CbcOrClpReadCommand = fp; // Read from that file
                                                       CbcOrClpRead_mode = -1;
                                                  }
                                             }
                                        }
                                   } else {
                                        // errors
                                        std::cout << "There were " << status <<
                                                  " errors on input" << std::endl;
                                   }
                              }
                         }
                         break;
                         case CLP_PARAM_ACTION_EXPORT:
                              if (goodModels[iModel]) {
                                   double objScale =
                                        parameters[whichParam(CLP_PARAM_DBL_OBJSCALE2, numberParameters, parameters)].doubleValue();
                                   if (objScale != 1.0) {
                                        int iColumn;
                                        int numberColumns = models[iModel].numberColumns();
                                        double * dualColumnSolution =
                                             models[iModel].dualColumnSolution();
                                        ClpObjective * obj = models[iModel].objectiveAsObject();
                                        assert(dynamic_cast<ClpLinearObjective *> (obj));
                                        double offset;
                                        double * objective = obj->gradient(NULL, NULL, offset, true);
                                        for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                                             dualColumnSolution[iColumn] *= objScale;
                                             objective[iColumn] *= objScale;;
                                        }
                                        int iRow;
                                        int numberRows = models[iModel].numberRows();
                                        double * dualRowSolution =
                                             models[iModel].dualRowSolution();
                                        for (iRow = 0; iRow < numberRows; iRow++)
                                             dualRowSolution[iRow] *= objScale;
                                        models[iModel].setObjectiveOffset(objScale * models[iModel].objectiveOffset());
                                   }
                                   // get next field
                                   field = CoinReadGetString(argc, argv);
                                   if (field == "$") {
                                        field = parameters[iParam].stringValue();
                                   } else if (field == "EOL") {
                                        parameters[iParam].printString();
                                        break;
                                   } else {
                                        parameters[iParam].setStringValue(field);
                                   }
                                   std::string fileName;
                                   bool canOpen = false;
                                   if (field[0] == '/' || field[0] == '\\') {
                                        fileName = field;
                                   } else if (field[0] == '~') {
                                        char * environVar = getenv("HOME");
                                        if (environVar) {
                                             std::string home(environVar);
                                             field = field.erase(0, 1);
                                             fileName = home + field;
                                        } else {
                                             fileName = field;
                                        }
                                   } else {
                                        fileName = directory + field;
                                   }
                                   FILE *fp = fopen(fileName.c_str(), "w");
                                   if (fp) {
                                        // can open - lets go for it
                                        fclose(fp);
                                        canOpen = true;
                                   } else {
                                        std::cout << "Unable to open file " << fileName << std::endl;
                                   }
                                   if (canOpen) {
                                        // If presolve on then save presolved
                                        bool deleteModel2 = false;
                                        ClpSimplex * model2 = models + iModel;
                                        if (dualize && dualize < 3) {
                                             model2 = static_cast<ClpSimplexOther *> (model2)->dualOfModel();
                                             printf("Dual of model has %d rows and %d columns\n",
                                                    model2->numberRows(), model2->numberColumns());
                                             model2->setOptimizationDirection(1.0);
                                             preSolve = 0; // as picks up from model
                                        }
                                        if (preSolve) {
                                             ClpPresolve pinfo;
                                             int presolveOptions2 = presolveOptions&~0x40000000;
                                             if ((presolveOptions2 & 0xffff) != 0)
                                                  pinfo.setPresolveActions(presolveOptions2);
                                             pinfo.setSubstitution(substitution);
                                             if ((printOptions & 1) != 0)
                                                  pinfo.statistics();
                                             double presolveTolerance =
                                                  parameters[whichParam(CLP_PARAM_DBL_PRESOLVETOLERANCE, numberParameters, parameters)].doubleValue();
                                             model2 =
                                                  pinfo.presolvedModel(models[iModel], presolveTolerance,
                                                                       true, preSolve, false, false);
                                             if (model2) {
                                                  printf("Saving presolved model on %s\n",
                                                         fileName.c_str());
                                                  deleteModel2 = true;
                                             } else {
                                                  printf("Presolved model looks infeasible - saving original on %s\n",
                                                         fileName.c_str());
                                                  deleteModel2 = false;
                                                  model2 = models + iModel;

                                             }
                                        } else {
                                             printf("Saving model on %s\n",
                                                    fileName.c_str());
                                        }
#if 0
                                        // Convert names
                                        int iRow;
                                        int numberRows = model2->numberRows();
                                        int iColumn;
                                        int numberColumns = model2->numberColumns();

                                        char ** rowNames = NULL;
                                        char ** columnNames = NULL;
                                        if (model2->lengthNames()) {
                                             rowNames = new char * [numberRows];
                                             for (iRow = 0; iRow < numberRows; iRow++) {
                                                  rowNames[iRow] =
                                                       CoinStrdup(model2->rowName(iRow).c_str());
#ifdef STRIPBLANKS
                                                  char * xx = rowNames[iRow];
                                                  int i;
                                                  int length = strlen(xx);
                                                  int n = 0;
                                                  for (i = 0; i < length; i++) {
                                                       if (xx[i] != ' ')
                                                            xx[n++] = xx[i];
                                                  }
                                                  xx[n] = '\0';
#endif
                                             }

                                             columnNames = new char * [numberColumns];
                                             for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                                                  columnNames[iColumn] =
                                                       CoinStrdup(model2->columnName(iColumn).c_str());
#ifdef STRIPBLANKS
                                                  char * xx = columnNames[iColumn];
                                                  int i;
                                                  int length = strlen(xx);
                                                  int n = 0;
                                                  for (i = 0; i < length; i++) {
                                                       if (xx[i] != ' ')
                                                            xx[n++] = xx[i];
                                                  }
                                                  xx[n] = '\0';
#endif
                                             }
                                        }
                                        CoinMpsIO writer;
                                        writer.setMpsData(*model2->matrix(), COIN_DBL_MAX,
                                                          model2->getColLower(), model2->getColUpper(),
                                                          model2->getObjCoefficients(),
                                                          (const char*) 0 /*integrality*/,
                                                          model2->getRowLower(), model2->getRowUpper(),
                                                          columnNames, rowNames);
                                        // Pass in array saying if each variable integer
                                        writer.copyInIntegerInformation(model2->integerInformation());
                                        writer.setObjectiveOffset(model2->objectiveOffset());
                                        writer.writeMps(fileName.c_str(), 0, 1, 1);
                                        if (rowNames) {
                                             for (iRow = 0; iRow < numberRows; iRow++) {
                                                  free(rowNames[iRow]);
                                             }
                                             delete [] rowNames;
                                             for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                                                  free(columnNames[iColumn]);
                                             }
                                             delete [] columnNames;
                                        }
#else
                                        model2->writeMps(fileName.c_str(), (outputFormat - 1) / 2, 1 + ((outputFormat - 1) & 1));
#endif
                                        if (deleteModel2)
                                             delete model2;
                                        time2 = CoinCpuTime();
                                        totalTime += time2 - time1;
                                        time1 = time2;
                                   }
                              } else {
                                   std::cout << "** Current model not valid" << std::endl;
                              }
                              break;
                         case CLP_PARAM_ACTION_BASISIN:
                              if (goodModels[iModel]) {
                                   // get next field
                                   field = CoinReadGetString(argc, argv);
                                   if (field == "$") {
                                        field = parameters[iParam].stringValue();
                                   } else if (field == "EOL") {
                                        parameters[iParam].printString();
                                        break;
                                   } else {
                                        parameters[iParam].setStringValue(field);
                                   }
                                   std::string fileName;
                                   bool canOpen = false;
                                   if (field == "-") {
                                        // stdin
                                        canOpen = true;
                                        fileName = "-";
                                   } else {
                                        if (field[0] == '/' || field[0] == '\\') {
                                             fileName = field;
                                        } else if (field[0] == '~') {
                                             char * environVar = getenv("HOME");
                                             if (environVar) {
                                                  std::string home(environVar);
                                                  field = field.erase(0, 1);
                                                  fileName = home + field;
                                             } else {
                                                  fileName = field;
                                             }
                                        } else {
                                             fileName = directory + field;
                                        }
                                        FILE *fp = fopen(fileName.c_str(), "r");
                                        if (fp) {
                                             // can open - lets go for it
                                             fclose(fp);
                                             canOpen = true;
                                        } else {
                                             std::cout << "Unable to open file " << fileName << std::endl;
                                        }
                                   }
                                   if (canOpen) {
                                        int values = thisModel->readBasis(fileName.c_str());
                                        if (values == 0)
                                             basisHasValues = -1;
                                        else
                                             basisHasValues = 1;
                                   }
                              } else {
                                   std::cout << "** Current model not valid" << std::endl;
                              }
                              break;
                         case CLP_PARAM_ACTION_PRINTMASK:
                              // get next field
                         {
                              std::string name = CoinReadGetString(argc, argv);
                              if (name != "EOL") {
                                   parameters[iParam].setStringValue(name);
                                   printMask = name;
                              } else {
                                   parameters[iParam].printString();
                              }
                         }
                         break;
                         case CLP_PARAM_ACTION_BASISOUT:
                              if (goodModels[iModel]) {
                                   // get next field
                                   field = CoinReadGetString(argc, argv);
                                   if (field == "$") {
                                        field = parameters[iParam].stringValue();
                                   } else if (field == "EOL") {
                                        parameters[iParam].printString();
                                        break;
                                   } else {
                                        parameters[iParam].setStringValue(field);
                                   }
                                   std::string fileName;
                                   bool canOpen = false;
                                   if (field[0] == '/' || field[0] == '\\') {
                                        fileName = field;
                                   } else if (field[0] == '~') {
                                        char * environVar = getenv("HOME");
                                        if (environVar) {
                                             std::string home(environVar);
                                             field = field.erase(0, 1);
                                             fileName = home + field;
                                        } else {
                                             fileName = field;
                                        }
                                   } else {
                                        fileName = directory + field;
                                   }
                                   FILE *fp = fopen(fileName.c_str(), "w");
                                   if (fp) {
                                        // can open - lets go for it
                                        fclose(fp);
                                        canOpen = true;
                                   } else {
                                        std::cout << "Unable to open file " << fileName << std::endl;
                                   }
                                   if (canOpen) {
                                        ClpSimplex * model2 = models + iModel;
                                        model2->writeBasis(fileName.c_str(), outputFormat > 1, outputFormat - 2);
                                        time2 = CoinCpuTime();
                                        totalTime += time2 - time1;
                                        time1 = time2;
                                   }
                              } else {
                                   std::cout << "** Current model not valid" << std::endl;
                              }
                              break;
                         case CLP_PARAM_ACTION_PARAMETRICS:
                              if (goodModels[iModel]) {
                                   // get next field
                                   field = CoinReadGetString(argc, argv);
                                   if (field == "$") {
                                        field = parameters[iParam].stringValue();
                                   } else if (field == "EOL") {
                                        parameters[iParam].printString();
                                        break;
                                   } else {
                                        parameters[iParam].setStringValue(field);
                                   }
                                   std::string fileName;
                                   //bool canOpen = false;
                                   if (field[0] == '/' || field[0] == '\\') {
                                        fileName = field;
                                   } else if (field[0] == '~') {
                                        char * environVar = getenv("HOME");
                                        if (environVar) {
                                             std::string home(environVar);
                                             field = field.erase(0, 1);
                                             fileName = home + field;
                                        } else {
                                             fileName = field;
                                        }
                                   } else {
                                        fileName = directory + field;
                                   }
				   ClpSimplex * model2 = models + iModel;
				   static_cast<ClpSimplexOther *> (model2)->parametrics(fileName.c_str());
				   time2 = CoinCpuTime();
				   totalTime += time2 - time1;
				   time1 = time2;
                              } else {
                                   std::cout << "** Current model not valid" << std::endl;
                              }
                              break;
                         case CLP_PARAM_ACTION_SAVE: {
                              // get next field
                              field = CoinReadGetString(argc, argv);
                              if (field == "$") {
                                   field = parameters[iParam].stringValue();
                              } else if (field == "EOL") {
                                   parameters[iParam].printString();
                                   break;
                              } else {
                                   parameters[iParam].setStringValue(field);
                              }
                              std::string fileName;
                              bool canOpen = false;
                              if (field[0] == '/' || field[0] == '\\') {
                                   fileName = field;
                              } else if (field[0] == '~') {
                                   char * environVar = getenv("HOME");
                                   if (environVar) {
                                        std::string home(environVar);
                                        field = field.erase(0, 1);
                                        fileName = home + field;
                                   } else {
                                        fileName = field;
                                   }
                              } else {
                                   fileName = directory + field;
                              }
                              FILE *fp = fopen(fileName.c_str(), "wb");
                              if (fp) {
                                   // can open - lets go for it
                                   fclose(fp);
                                   canOpen = true;
                              } else {
                                   std::cout << "Unable to open file " << fileName << std::endl;
                              }
                              if (canOpen) {
                                   int status;
                                   // If presolve on then save presolved
                                   bool deleteModel2 = false;
                                   ClpSimplex * model2 = models + iModel;
                                   if (preSolve) {
                                        ClpPresolve pinfo;
                                        double presolveTolerance =
                                             parameters[whichParam(CLP_PARAM_DBL_PRESOLVETOLERANCE, numberParameters, parameters)].doubleValue();
                                        model2 =
                                             pinfo.presolvedModel(models[iModel], presolveTolerance,
                                                                  false, preSolve);
                                        if (model2) {
                                             printf("Saving presolved model on %s\n",
                                                    fileName.c_str());
                                             deleteModel2 = true;
                                        } else {
                                             printf("Presolved model looks infeasible - saving original on %s\n",
                                                    fileName.c_str());
                                             deleteModel2 = false;
                                             model2 = models + iModel;

                                        }
                                   } else {
                                        printf("Saving model on %s\n",
                                               fileName.c_str());
                                   }
                                   status = model2->saveModel(fileName.c_str());
                                   if (deleteModel2)
                                        delete model2;
                                   if (!status) {
                                        goodModels[iModel] = true;
                                        time2 = CoinCpuTime();
                                        totalTime += time2 - time1;
                                        time1 = time2;
                                   } else {
                                        // errors
                                        std::cout << "There were errors on output" << std::endl;
                                   }
                              }
                         }
                         break;
                         case CLP_PARAM_ACTION_RESTORE: {
                              // get next field
                              field = CoinReadGetString(argc, argv);
                              if (field == "$") {
                                   field = parameters[iParam].stringValue();
                              } else if (field == "EOL") {
                                   parameters[iParam].printString();
                                   break;
                              } else {
                                   parameters[iParam].setStringValue(field);
                              }
                              std::string fileName;
                              bool canOpen = false;
                              if (field[0] == '/' || field[0] == '\\') {
                                   fileName = field;
                              } else if (field[0] == '~') {
                                   char * environVar = getenv("HOME");
                                   if (environVar) {
                                        std::string home(environVar);
                                        field = field.erase(0, 1);
                                        fileName = home + field;
                                   } else {
                                        fileName = field;
                                   }
                              } else {
                                   fileName = directory + field;
                              }
                              FILE *fp = fopen(fileName.c_str(), "rb");
                              if (fp) {
                                   // can open - lets go for it
                                   fclose(fp);
                                   canOpen = true;
                              } else {
                                   std::cout << "Unable to open file " << fileName << std::endl;
                              }
                              if (canOpen) {
                                   int status = models[iModel].restoreModel(fileName.c_str());
                                   if (!status) {
                                        goodModels[iModel] = true;
                                        time2 = CoinCpuTime();
                                        totalTime += time2 - time1;
                                        time1 = time2;
                                   } else {
                                        // errors
                                        std::cout << "There were errors on input" << std::endl;
                                   }
                              }
                         }
                         break;
                         case CLP_PARAM_ACTION_MAXIMIZE:
                              models[iModel].setOptimizationDirection(-1);
#ifdef ABC_INHERIT
                              thisModel->setOptimizationDirection(-1);
#endif
                              break;
                         case CLP_PARAM_ACTION_MINIMIZE:
                              models[iModel].setOptimizationDirection(1);
#ifdef ABC_INHERIT
                              thisModel->setOptimizationDirection(1);
#endif
                              break;
                         case CLP_PARAM_ACTION_ALLSLACK:
                              thisModel->allSlackBasis(true);
#ifdef ABC_INHERIT
                              models[iModel].allSlackBasis();
#endif
                              break;
                         case CLP_PARAM_ACTION_REVERSE:
                              if (goodModels[iModel]) {
                                   int iColumn;
                                   int numberColumns = models[iModel].numberColumns();
                                   double * dualColumnSolution =
                                        models[iModel].dualColumnSolution();
                                   ClpObjective * obj = models[iModel].objectiveAsObject();
                                   assert(dynamic_cast<ClpLinearObjective *> (obj));
                                   double offset;
                                   double * objective = obj->gradient(NULL, NULL, offset, true);
                                   for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                                        dualColumnSolution[iColumn] = -dualColumnSolution[iColumn];
                                        objective[iColumn] = -objective[iColumn];
                                   }
                                   int iRow;
                                   int numberRows = models[iModel].numberRows();
                                   double * dualRowSolution =
                                        models[iModel].dualRowSolution();
                                   for (iRow = 0; iRow < numberRows; iRow++) {
                                        dualRowSolution[iRow] = -dualRowSolution[iRow];
                                   }
                                   models[iModel].setObjectiveOffset(-models[iModel].objectiveOffset());
                              }
                              break;
                         case CLP_PARAM_ACTION_DIRECTORY: {
                              std::string name = CoinReadGetString(argc, argv);
                              if (name != "EOL") {
                                   size_t length = name.length();
                                   if (length > 0 && name[length-1] == dirsep) {
                                        directory = name;
                                   } else {
                                        directory = name + dirsep;
                                   }
                                   parameters[iParam].setStringValue(directory);
                              } else {
                                   parameters[iParam].printString();
                              }
                         }
                         break;
                         case CLP_PARAM_ACTION_DIRSAMPLE: {
                              std::string name = CoinReadGetString(argc, argv);
                              if (name != "EOL") {
                                   size_t length = name.length();
                                   if (length > 0 && name[length-1] == dirsep) {
                                        dirSample = name;
                                   } else {
                                        dirSample = name + dirsep;
                                   }
                                   parameters[iParam].setStringValue(dirSample);
                              } else {
                                   parameters[iParam].printString();
                              }
                         }
                         break;
                         case CLP_PARAM_ACTION_DIRNETLIB: {
                              std::string name = CoinReadGetString(argc, argv);
                              if (name != "EOL") {
                                   size_t length = name.length();
                                   if (length > 0 && name[length-1] == dirsep) {
                                        dirNetlib = name;
                                   } else {
                                        dirNetlib = name + dirsep;
                                   }
                                   parameters[iParam].setStringValue(dirNetlib);
                              } else {
                                   parameters[iParam].printString();
                              }
                         }
                         break;
                         case CBC_PARAM_ACTION_DIRMIPLIB: {
                              std::string name = CoinReadGetString(argc, argv);
                              if (name != "EOL") {
                                   size_t length = name.length();
                                   if (length > 0 && name[length-1] == dirsep) {
                                        dirMiplib = name;
                                   } else {
                                        dirMiplib = name + dirsep;
                                   }
                                   parameters[iParam].setStringValue(dirMiplib);
                              } else {
                                   parameters[iParam].printString();
                              }
                         }
                         break;
                         case CLP_PARAM_ACTION_STDIN:
                              CbcOrClpRead_mode = -1;
                              break;
                         case CLP_PARAM_ACTION_NETLIB_DUAL:
                         case CLP_PARAM_ACTION_NETLIB_EITHER:
                         case CLP_PARAM_ACTION_NETLIB_BARRIER:
                         case CLP_PARAM_ACTION_NETLIB_PRIMAL:
                         case CLP_PARAM_ACTION_NETLIB_TUNE: {
                              // create fields for unitTest
                              const char * fields[4];
                              int nFields = 4;
                              fields[0] = "fake main from unitTest";
                              std::string mpsfield = "-dirSample=";
                              mpsfield += dirSample.c_str();
                              fields[1] = mpsfield.c_str();
                              std::string netfield = "-dirNetlib=";
                              netfield += dirNetlib.c_str();
                              fields[2] = netfield.c_str();
                              fields[3] = "-netlib";
                              int algorithm;
                              if (type == CLP_PARAM_ACTION_NETLIB_DUAL) {
                                   std::cerr << "Doing netlib with dual algorithm" << std::endl;
                                   algorithm = 0;
				   models[iModel].setMoreSpecialOptions(models[iModel].moreSpecialOptions()|32768);
                              } else if (type == CLP_PARAM_ACTION_NETLIB_BARRIER) {
                                   std::cerr << "Doing netlib with barrier algorithm" << std::endl;
                                   algorithm = 2;
                              } else if (type == CLP_PARAM_ACTION_NETLIB_EITHER) {
                                   std::cerr << "Doing netlib with dual or primal algorithm" << std::endl;
                                   algorithm = 3;
                              } else if (type == CLP_PARAM_ACTION_NETLIB_TUNE) {
                                   std::cerr << "Doing netlib with best algorithm!" << std::endl;
                                   algorithm = 5;
                                   // uncomment next to get active tuning
                                   // algorithm=6;
                              } else {
                                   std::cerr << "Doing netlib with primal algorithm" << std::endl;
                                   algorithm = 1;
                              }
                              //int specialOptions = models[iModel].specialOptions();
                              models[iModel].setSpecialOptions(0);
                              ClpSolve solveOptions;
                              ClpSolve::PresolveType presolveType;
                              if (preSolve)
                                   presolveType = ClpSolve::presolveOn;
                              else
                                   presolveType = ClpSolve::presolveOff;
                              solveOptions.setPresolveType(presolveType, 5);
                              if (doSprint >= 0 || doIdiot >= 0) {
                                   if (doSprint > 0) {
                                        // sprint overrides idiot
                                        solveOptions.setSpecialOption(1, 3, doSprint); // sprint
                                   } else if (doIdiot > 0) {
                                        solveOptions.setSpecialOption(1, 2, doIdiot); // idiot
                                   } else {
                                        if (doIdiot == 0) {
                                             if (doSprint == 0)
                                                  solveOptions.setSpecialOption(1, 4); // all slack
                                             else
                                                  solveOptions.setSpecialOption(1, 9); // all slack or sprint
                                        } else {
                                             if (doSprint == 0)
                                                  solveOptions.setSpecialOption(1, 8); // all slack or idiot
                                             else
                                                  solveOptions.setSpecialOption(1, 7); // initiative
                                        }
                                   }
                              }
#if FACTORIZATION_STATISTICS
			      {
				extern int loSizeX;
				extern int hiSizeX;
				for (int i=0;i<argc;i++) {
				  if (!strcmp(argv[i],"-losize")) {
				    int size=atoi(argv[i+1]);
				    if (size>0)
				      loSizeX=size;
				  }
				  if (!strcmp(argv[i],"-hisize")) {
				    int size=atoi(argv[i+1]);
				    if (size>loSizeX)
				      hiSizeX=size;
				  }
				}
				if (loSizeX!=-1||hiSizeX!=1000000)
				  printf("Solving problems %d<= and <%d\n",loSizeX,hiSizeX);
			      }
#endif
			      // for moment then back to models[iModel]
#ifndef ABC_INHERIT
                              int specialOptions = models[iModel].specialOptions();
                              mainTest(nFields, fields, algorithm, *thisModel,
                                       solveOptions, specialOptions, doVector != 0);
#else
			      //if (!processTune) {
			      //specialOptions=0;
			      //models->setSpecialOptions(models->specialOptions()&~65536);
			      // }
                              mainTest(nFields, fields, algorithm, *models,
                                       solveOptions, 0, doVector != 0);
#endif
                         }
                         break;
                         case CLP_PARAM_ACTION_UNITTEST: {
                              // create fields for unitTest
                              const char * fields[2];
                              int nFields = 2;
                              fields[0] = "fake main from unitTest";
                              std::string dirfield = "-dirSample=";
                              dirfield += dirSample.c_str();
                              fields[1] = dirfield.c_str();
                              int specialOptions = models[iModel].specialOptions();
                              models[iModel].setSpecialOptions(0);
                              int algorithm = -1;
                              if (models[iModel].numberRows())
                                   algorithm = 7;
                              ClpSolve solveOptions;
                              ClpSolve::PresolveType presolveType;
                              if (preSolve)
                                   presolveType = ClpSolve::presolveOn;
                              else
                                   presolveType = ClpSolve::presolveOff;
                              solveOptions.setPresolveType(presolveType, 5);
#ifndef ABC_INHERIT
                              mainTest(nFields, fields, algorithm, *thisModel,
                                       solveOptions, specialOptions, doVector != 0);
#else
                              mainTest(nFields, fields, algorithm, *models,
                                       solveOptions, specialOptions, doVector != 0);
#endif
                         }
                         break;
                         case CLP_PARAM_ACTION_FAKEBOUND:
                              if (goodModels[iModel]) {
                                   // get bound
                                   double value = CoinReadGetDoubleField(argc, argv, &valid);
                                   if (!valid) {
                                        std::cout << "Setting " << parameters[iParam].name() <<
                                                  " to DEBUG " << value << std::endl;
                                        int iRow;
                                        int numberRows = models[iModel].numberRows();
                                        double * rowLower = models[iModel].rowLower();
                                        double * rowUpper = models[iModel].rowUpper();
                                        for (iRow = 0; iRow < numberRows; iRow++) {
                                             // leave free ones for now
                                             if (rowLower[iRow] > -1.0e20 || rowUpper[iRow] < 1.0e20) {
                                                  rowLower[iRow] = CoinMax(rowLower[iRow], -value);
                                                  rowUpper[iRow] = CoinMin(rowUpper[iRow], value);
                                             }
                                        }
                                        int iColumn;
                                        int numberColumns = models[iModel].numberColumns();
                                        double * columnLower = models[iModel].columnLower();
                                        double * columnUpper = models[iModel].columnUpper();
                                        for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                                             // leave free ones for now
                                             if (columnLower[iColumn] > -1.0e20 ||
                                                       columnUpper[iColumn] < 1.0e20) {
                                                  columnLower[iColumn] = CoinMax(columnLower[iColumn], -value);
                                                  columnUpper[iColumn] = CoinMin(columnUpper[iColumn], value);
                                             }
                                        }
                                   } else if (valid == 1) {
                                        abort();
                                   } else {
                                        std::cout << "enter value for " << parameters[iParam].name() <<
                                                  std::endl;
                                   }
                              }
                              break;
                         case CLP_PARAM_ACTION_REALLY_SCALE:
                              if (goodModels[iModel]) {
                                   ClpSimplex newModel(models[iModel],
                                                       models[iModel].scalingFlag());
                                   printf("model really really scaled\n");
                                   models[iModel] = newModel;
                              }
                              break;
                         case CLP_PARAM_ACTION_USERCLP:
                              // Replace the sample code by whatever you want
                              if (goodModels[iModel]) {
                                   ClpSimplex * thisModel = &models[iModel];
                                   printf("Dummy user code - model has %d rows and %d columns\n",
                                          thisModel->numberRows(), thisModel->numberColumns());
                              }
                              break;
                         case CLP_PARAM_ACTION_HELP:
                              std::cout << "Coin LP version " << CLP_VERSION
                                        << ", build " << __DATE__ << std::endl;
                              std::cout << "Non default values:-" << std::endl;
                              std::cout << "Perturbation " << models[0].perturbation() << " (default 100)"
                                        << std::endl;
                              CoinReadPrintit(
                                   "Presolve being done with 5 passes\n\
Dual steepest edge steep/partial on matrix shape and factorization density\n\
Clpnnnn taken out of messages\n\
If Factorization frequency default then done on size of matrix\n\n\
(-)unitTest, (-)netlib or (-)netlibp will do standard tests\n\n\
You can switch to interactive mode at any time so\n\
clp watson.mps -scaling off -primalsimplex\nis the same as\n\
clp watson.mps -\nscaling off\nprimalsimplex"
                              );
                              break;
                         case CLP_PARAM_ACTION_SOLUTION:
			 case CLP_PARAM_ACTION_GMPL_SOLUTION:
                              if (goodModels[iModel]) {
                                   // get next field
                                   field = CoinReadGetString(argc, argv);
				   bool append = false;
				   if (field == "append$") {
				     field = "$";
				     append = true;
				   }
                                   if (field == "$") {
                                        field = parameters[iParam].stringValue();
                                   } else if (field == "EOL") {
                                        parameters[iParam].printString();
                                        break;
                                   } else {
                                        parameters[iParam].setStringValue(field);
                                   }
                                   std::string fileName;
                                   FILE *fp = NULL;
                                   if (field == "-" || field == "EOL" || field == "stdout") {
                                        // stdout
                                        fp = stdout;
                                        fprintf(fp, "\n");
                                   } else if (field == "stderr") {
                                        // stderr
                                        fp = stderr;
                                        fprintf(fp, "\n");
                                   } else {
                                        if (field[0] == '/' || field[0] == '\\') {
                                             fileName = field;
                                        } else if (field[0] == '~') {
                                             char * environVar = getenv("HOME");
                                             if (environVar) {
                                                  std::string home(environVar);
                                                  field = field.erase(0, 1);
                                                  fileName = home + field;
                                             } else {
                                                  fileName = field;
                                             }
                                        } else {
                                             fileName = directory + field;
                                        }
					if (!append)
					  fp = fopen(fileName.c_str(), "w");
					else
					  fp = fopen(fileName.c_str(), "a");
                                   }
                                   if (fp) {
				     // See if Glpk 
				     if (type == CLP_PARAM_ACTION_GMPL_SOLUTION) {
				       int numberRows = models[iModel].getNumRows();
				       int numberColumns = models[iModel].getNumCols();
				       int numberGlpkRows=numberRows+1;
#ifdef COIN_HAS_GLPK
				       if (cbc_glp_prob) {
					 // from gmpl
					 numberGlpkRows=glp_get_num_rows(cbc_glp_prob);
					 if (numberGlpkRows!=numberRows)
					   printf("Mismatch - cbc %d rows, glpk %d\n",
						  numberRows,numberGlpkRows);
				       }
#endif
				       fprintf(fp,"%d %d\n",numberGlpkRows,
					       numberColumns);
				       int iStat = models[iModel].status();
				       int iStat2 = GLP_UNDEF;
				       if (iStat == 0) {
					 // optimal
					 iStat2 = GLP_FEAS;
				       } else if (iStat == 1) {
					 // infeasible
					 iStat2 = GLP_NOFEAS;
				       } else if (iStat == 2) {
					 // unbounded
					 // leave as 1
				       } else if (iStat >= 3 && iStat <= 5) {
					 iStat2 = GLP_FEAS;
				       }
				       double objValue = models[iModel].getObjValue() 
					 * models[iModel].getObjSense();
				       fprintf(fp,"%d 2 %g\n",iStat2,objValue);
				       if (numberGlpkRows > numberRows) {
					 // objective as row
					 fprintf(fp,"4 %g 1.0\n",objValue);
				       }
				       int lookup[6]=
					 {4,1,3,2,4,5};
				       const double * primalRowSolution =
					 models[iModel].primalRowSolution();
				       const double * dualRowSolution =
					 models[iModel].dualRowSolution();
				       for (int i=0;i<numberRows;i++) {
					 fprintf(fp,"%d %g %g\n",lookup[models[iModel].getRowStatus(i)],
						 primalRowSolution[i],dualRowSolution[i]);
				       }
				       const double * primalColumnSolution =
					 models[iModel].primalColumnSolution();
				       const double * dualColumnSolution =
					 models[iModel].dualColumnSolution();
				       for (int i=0;i<numberColumns;i++) {
					 fprintf(fp,"%d %g %g\n",lookup[models[iModel].getColumnStatus(i)],
						 primalColumnSolution[i],dualColumnSolution[i]);
				       }
				       fclose(fp);
#ifdef COIN_HAS_GLPK
				       if (cbc_glp_prob) {
					 glp_read_sol(cbc_glp_prob,fileName.c_str());
					 glp_mpl_postsolve(cbc_glp_tran,
							   cbc_glp_prob,
							   GLP_SOL);
					 // free up as much as possible
					 glp_free(cbc_glp_prob);
					 glp_mpl_free_wksp(cbc_glp_tran);
					 cbc_glp_prob = NULL;
					 cbc_glp_tran = NULL;
					 //gmp_free_mem();
					 /* check that no memory blocks are still allocated */
					 glp_free_env();
				       }
#endif
				       break;
				     }
                                        // Write solution header (suggested by Luigi Poderico)
                                        double objValue = models[iModel].getObjValue() * models[iModel].getObjSense();
                                        int iStat = models[iModel].status();
                                        if (iStat == 0) {
                                             fprintf(fp, "optimal\n" );
                                        } else if (iStat == 1) {
                                             // infeasible
                                             fprintf(fp, "infeasible\n" );
                                        } else if (iStat == 2) {
                                             // unbounded
                                             fprintf(fp, "unbounded\n" );
                                        } else if (iStat == 3) {
                                             fprintf(fp, "stopped on iterations or time\n" );
                                        } else if (iStat == 4) {
                                             fprintf(fp, "stopped on difficulties\n" );
                                        } else if (iStat == 5) {
                                             fprintf(fp, "stopped on ctrl-c\n" );
                                        } else {
                                             fprintf(fp, "status unknown\n" );
                                        }
                                        fprintf(fp, "Objective value %15.8g\n", objValue);
					if (printMode==9) {
					  // just statistics
					  int numberRows = models[iModel].numberRows();
					  double * dualRowSolution = models[iModel].dualRowSolution();
					  double * primalRowSolution =
					    models[iModel].primalRowSolution();
					  double * rowLower = models[iModel].rowLower();
					  double * rowUpper = models[iModel].rowUpper();
					  double highestPrimal;
					  double lowestPrimal;
					  double highestDual;
					  double lowestDual;
					  double largestAway;
					  int numberAtLower;
					  int numberAtUpper;
					  int numberBetween;
					  highestPrimal=-COIN_DBL_MAX;
					  lowestPrimal=COIN_DBL_MAX;
					  highestDual=-COIN_DBL_MAX;
					  lowestDual=COIN_DBL_MAX;
					  largestAway=0.0;;
					  numberAtLower=0;
					  numberAtUpper=0;
					  numberBetween=0;
					  for (int iRow=0;iRow<numberRows;iRow++) {
					    double primal=primalRowSolution[iRow];
					    double lower=rowLower[iRow];
					    double upper=rowUpper[iRow];
					    double dual=dualRowSolution[iRow];
					    highestPrimal=CoinMax(highestPrimal,primal);
					    lowestPrimal=CoinMin(lowestPrimal,primal);
					    highestDual=CoinMax(highestDual,dual);
					    lowestDual=CoinMin(lowestDual,dual);
					    if (primal<lower+1.0e-6) {
					      numberAtLower++;
					    } else if (primal>upper-1.0e-6) {
					      numberAtUpper++;
					    } else {
					      numberBetween++;
					      largestAway=CoinMax(largestAway,
								  CoinMin(primal-lower,upper-primal));
					    }
					  }
					  printf("For rows %d at lower, %d between, %d at upper - lowest %g, highest %g most away %g - highest dual %g lowest %g\n",
						 numberAtLower,numberBetween,numberAtUpper,
						 lowestPrimal,highestPrimal,largestAway,
						 lowestDual,highestDual);
					  int numberColumns = models[iModel].numberColumns();
					  double * dualColumnSolution = models[iModel].dualColumnSolution();
					  double * primalColumnSolution =
					    models[iModel].primalColumnSolution();
					  double * columnLower = models[iModel].columnLower();
					  double * columnUpper = models[iModel].columnUpper();
					  highestPrimal=-COIN_DBL_MAX;
					  lowestPrimal=COIN_DBL_MAX;
					  highestDual=-COIN_DBL_MAX;
					  lowestDual=COIN_DBL_MAX;
					  largestAway=0.0;;
					  numberAtLower=0;
					  numberAtUpper=0;
					  numberBetween=0;
					  for (int iColumn=0;iColumn<numberColumns;iColumn++) {
					    double primal=primalColumnSolution[iColumn];
					    double lower=columnLower[iColumn];
					    double upper=columnUpper[iColumn];
					    double dual=dualColumnSolution[iColumn];
					    highestPrimal=CoinMax(highestPrimal,primal);
					    lowestPrimal=CoinMin(lowestPrimal,primal);
					    highestDual=CoinMax(highestDual,dual);
					    lowestDual=CoinMin(lowestDual,dual);
					    if (primal<lower+1.0e-6) {
					      numberAtLower++;
					    } else if (primal>upper-1.0e-6) {
					      numberAtUpper++;
					    } else {
					      numberBetween++;
					      largestAway=CoinMax(largestAway,
								  CoinMin(primal-lower,upper-primal));
					    }
					  }
					  printf("For columns %d at lower, %d between, %d at upper - lowest %g, highest %g most away %g - highest dual %g lowest %g\n",
						 numberAtLower,numberBetween,numberAtUpper,
						 lowestPrimal,highestPrimal,largestAway,
						 lowestDual,highestDual);
					  break;
					}
                                        // make fancy later on
                                        int iRow;
                                        int numberRows = models[iModel].numberRows();
                                        int lengthName = models[iModel].lengthNames(); // 0 if no names
                                        int lengthPrint = CoinMax(lengthName, 8);
                                        // in general I don't want to pass around massive
                                        // amounts of data but seems simpler here
                                        std::vector<std::string> rowNames =
                                             *(models[iModel].rowNames());
                                        std::vector<std::string> columnNames =
                                             *(models[iModel].columnNames());

                                        double * dualRowSolution = models[iModel].dualRowSolution();
                                        double * primalRowSolution =
                                             models[iModel].primalRowSolution();
                                        double * rowLower = models[iModel].rowLower();
                                        double * rowUpper = models[iModel].rowUpper();
                                        double primalTolerance = models[iModel].primalTolerance();
                                        bool doMask = (printMask != "" && lengthName);
                                        int * maskStarts = NULL;
                                        int maxMasks = 0;
                                        char ** masks = NULL;
                                        if (doMask) {
                                             int nAst = 0;
                                             const char * pMask2 = printMask.c_str();
                                             char pMask[100];
                                             size_t lengthMask = strlen(pMask2);
                                             assert (lengthMask < 100);
                                             if (*pMask2 == '"') {
                                                  if (pMask2[lengthMask-1] != '"') {
                                                       printf("mismatched \" in mask %s\n", pMask2);
                                                       break;
                                                  } else {
                                                       strcpy(pMask, pMask2 + 1);
                                                       *strchr(pMask, '"') = '\0';
                                                  }
                                             } else if (*pMask2 == '\'') {
                                                  if (pMask2[lengthMask-1] != '\'') {
                                                       printf("mismatched ' in mask %s\n", pMask2);
                                                       break;
                                                  } else {
                                                       strcpy(pMask, pMask2 + 1);
                                                       *strchr(pMask, '\'') = '\0';
                                                  }
                                             } else {
                                                  strcpy(pMask, pMask2);
                                             }
                                             if (lengthMask > static_cast<size_t>(lengthName)) {
                                                  printf("mask %s too long - skipping\n", pMask);
                                                  break;
                                             }
                                             maxMasks = 1;
                                             for (size_t iChar = 0; iChar < lengthMask; iChar++) {
                                                  if (pMask[iChar] == '*') {
                                                       nAst++;
                                                       maxMasks *= (lengthName + 1);
                                                  }
                                             }
                                             int nEntries = 1;
                                             maskStarts = new int[lengthName+2];
                                             masks = new char * [maxMasks];
                                             char ** newMasks = new char * [maxMasks];
                                             int i;
                                             for (i = 0; i < maxMasks; i++) {
                                                  masks[i] = new char[lengthName+1];
                                                  newMasks[i] = new char[lengthName+1];
                                             }
                                             strcpy(masks[0], pMask);
                                             for (int iAst = 0; iAst < nAst; iAst++) {
                                                  int nOldEntries = nEntries;
                                                  nEntries = 0;
                                                  for (int iEntry = 0; iEntry < nOldEntries; iEntry++) {
                                                       char * oldMask = masks[iEntry];
                                                       char * ast = strchr(oldMask, '*');
                                                       assert (ast);
                                                       size_t length = strlen(oldMask) - 1;
                                                       size_t nBefore = ast - oldMask;
                                                       size_t nAfter = length - nBefore;
                                                       // and add null
                                                       nAfter++;
                                                       for (size_t i = 0; i <= lengthName - length; i++) {
                                                            char * maskOut = newMasks[nEntries];
                                                            CoinMemcpyN(oldMask, static_cast<int>(nBefore), maskOut);
                                                            for (size_t k = 0; k < i; k++)
                                                                 maskOut[k+nBefore] = '?';
                                                            CoinMemcpyN(ast + 1, static_cast<int>(nAfter), maskOut + nBefore + i);
                                                            nEntries++;
                                                            assert (nEntries <= maxMasks);
                                                       }
                                                  }
                                                  char ** temp = masks;
                                                  masks = newMasks;
                                                  newMasks = temp;
                                             }
                                             // Now extend and sort
                                             int * sort = new int[nEntries];
                                             for (i = 0; i < nEntries; i++) {
                                                  char * maskThis = masks[i];
                                                  size_t length = strlen(maskThis);
                                                  while (length > 0 && maskThis[length-1] == ' ')
                                                       length--;
                                                  maskThis[length] = '\0';
                                                  sort[i] = static_cast<int>(length);
                                             }
                                             CoinSort_2(sort, sort + nEntries, masks);
                                             int lastLength = -1;
                                             for (i = 0; i < nEntries; i++) {
                                                  int length = sort[i];
                                                  while (length > lastLength)
                                                       maskStarts[++lastLength] = i;
                                             }
                                             maskStarts[++lastLength] = nEntries;
                                             delete [] sort;
                                             for (i = 0; i < maxMasks; i++)
                                                  delete [] newMasks[i];
                                             delete [] newMasks;
                                        }
					if (printMode > 5) {
					  int numberColumns = models[iModel].numberColumns();
					  // column length unless rhs ranging
					  int number = numberColumns;
					  switch (printMode) {
					    // bound ranging
					    case 6:
					      fprintf(fp,"Bound ranging");
					      break;
					    // rhs ranging
					    case 7:
					      fprintf(fp,"Rhs ranging");
					      number = numberRows;
					      break;
					    // objective ranging
					    case 8:
					      fprintf(fp,"Objective ranging");
					      break;
					  }
					  if (lengthName)
					    fprintf(fp,",name");
					  fprintf(fp,",increase,variable,decrease,variable\n");
					  int * which = new int [ number];
					  if (printMode != 7) {
					    if (!doMask) {
					      for (int i = 0; i < number;i ++)
						which[i]=i;
					    } else {
					      int n = 0;
					      for (int i = 0; i < number;i ++) {
						if (maskMatches(maskStarts,masks,columnNames[i]))
						  which[n++]=i;
					      }
					      if (n) {
						number=n;
					      } else {
						printf("No names match - doing all\n");
						for (int i = 0; i < number;i ++)
						  which[i]=i;
					      }
					    }
					  } else {
					    if (!doMask) {
					      for (int i = 0; i < number;i ++)
						which[i]=i+numberColumns;
					    } else {
					      int n = 0;
					      for (int i = 0; i < number;i ++) {
						if (maskMatches(maskStarts,masks,rowNames[i]))
						  which[n++]=i+numberColumns;
					      }
					      if (n) {
						number=n;
					      } else {
						printf("No names match - doing all\n");
						for (int i = 0; i < number;i ++)
						  which[i]=i+numberColumns;
					      }
					    }
					  }
					  double * valueIncrease = new double [ number];
					  int * sequenceIncrease = new int [ number];
					  double * valueDecrease = new double [ number];
					  int * sequenceDecrease = new int [ number];
					  switch (printMode) {
					    // bound or rhs ranging
					    case 6:
					    case 7:
					      models[iModel].primalRanging(numberRows,
									   which, valueIncrease, sequenceIncrease,
									   valueDecrease, sequenceDecrease);
					      break;
					    // objective ranging
					    case 8:
					      models[iModel].dualRanging(number,
									   which, valueIncrease, sequenceIncrease,
									   valueDecrease, sequenceDecrease);
					      break;
					  }
					  for (int i = 0; i < number; i++) {
					    int iWhich = which[i];
					    fprintf(fp, "%d,", (iWhich<numberColumns) ? iWhich : iWhich-numberColumns);
					    if (lengthName) {
					      const char * name = (printMode==7) ? rowNames[iWhich-numberColumns].c_str() : columnNames[iWhich].c_str();
					      fprintf(fp,"%s,",name);
					    }
					    if (valueIncrease[i]<1.0e30) {
					      fprintf(fp, "%.10g,", valueIncrease[i]);
					      int outSequence = sequenceIncrease[i];
					      if (outSequence<numberColumns) {
						if (lengthName)
						  fprintf(fp,"%s,",columnNames[outSequence].c_str());
						else
						  fprintf(fp,"C%7.7d,",outSequence);
					      } else {
						outSequence -= numberColumns;
						if (lengthName)
						  fprintf(fp,"%s,",rowNames[outSequence].c_str());
						else
						  fprintf(fp,"R%7.7d,",outSequence);
					      }
					    } else {
					      fprintf(fp,"1.0e100,,");
					    }
					    if (valueDecrease[i]<1.0e30) {
					      fprintf(fp, "%.10g,", valueDecrease[i]);
					      int outSequence = sequenceDecrease[i];
					      if (outSequence<numberColumns) {
						if (lengthName)
						  fprintf(fp,"%s",columnNames[outSequence].c_str());
						else
						  fprintf(fp,"C%7.7d",outSequence);
					      } else {
						outSequence -= numberColumns;
						if (lengthName)
						  fprintf(fp,"%s",rowNames[outSequence].c_str());
						else
						  fprintf(fp,"R%7.7d",outSequence);
					      }
					    } else {
					      fprintf(fp,"1.0e100,");
					    }
					    fprintf(fp,"\n");
					  }
					  if (fp != stdout)
					    fclose(fp);
					  delete [] which;
					  delete [] valueIncrease;
					  delete [] sequenceIncrease;
					  delete [] valueDecrease;
					  delete [] sequenceDecrease;
					  if (masks) {
					    delete [] maskStarts;
					    for (int i = 0; i < maxMasks; i++)
					      delete [] masks[i];
					    delete [] masks;
					  }
					  break;
					}
                                        if (printMode > 2) {
                                             for (iRow = 0; iRow < numberRows; iRow++) {
                                                  int type = printMode - 3;
                                                  if (primalRowSolution[iRow] > rowUpper[iRow] + primalTolerance ||
                                                            primalRowSolution[iRow] < rowLower[iRow] - primalTolerance) {
                                                       fprintf(fp, "** ");
                                                       type = 2;
                                                  } else if (fabs(primalRowSolution[iRow]) > 1.0e-8) {
                                                       type = 1;
                                                  } else if (numberRows < 50) {
                                                       type = 3;
                                                  }
                                                  if (doMask && !maskMatches(maskStarts, masks, rowNames[iRow]))
                                                       type = 0;
                                                  if (type) {
                                                       fprintf(fp, "%7d ", iRow);
                                                       if (lengthName) {
                                                            const char * name = rowNames[iRow].c_str();
                                                            size_t n = strlen(name);
                                                            size_t i;
                                                            for (i = 0; i < n; i++)
                                                                 fprintf(fp, "%c", name[i]);
                                                            for (; i < static_cast<size_t>(lengthPrint); i++)
                                                                 fprintf(fp, " ");
                                                       }
                                                       fprintf(fp, " %15.8g        %15.8g\n", primalRowSolution[iRow],
                                                               dualRowSolution[iRow]);
                                                  }
                                             }
                                        }
                                        int iColumn;
                                        int numberColumns = models[iModel].numberColumns();
                                        double * dualColumnSolution =
                                             models[iModel].dualColumnSolution();
                                        double * primalColumnSolution =
                                             models[iModel].primalColumnSolution();
                                        double * columnLower = models[iModel].columnLower();
                                        double * columnUpper = models[iModel].columnUpper();
                                        for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                                             int type = (printMode > 3) ? 1 : 0;
                                             if (primalColumnSolution[iColumn] > columnUpper[iColumn] + primalTolerance ||
                                                       primalColumnSolution[iColumn] < columnLower[iColumn] - primalTolerance) {
                                                  fprintf(fp, "** ");
                                                  type = 2;
                                             } else if (fabs(primalColumnSolution[iColumn]) > 1.0e-8) {
                                                  type = 1;
                                             } else if (numberColumns < 50) {
                                                  type = 3;
                                             }
                                             if (doMask && !maskMatches(maskStarts, masks,
                                                                        columnNames[iColumn]))
                                                  type = 0;
                                             if (type) {
                                                  fprintf(fp, "%7d ", iColumn);
                                                  if (lengthName) {
                                                       const char * name = columnNames[iColumn].c_str();
                                                       size_t n = strlen(name);
                                                       size_t i;
                                                       for (i = 0; i < n; i++)
                                                            fprintf(fp, "%c", name[i]);
                                                       for (; i < static_cast<size_t>(lengthPrint); i++)
                                                            fprintf(fp, " ");
                                                  }
                                                  fprintf(fp, " %15.8g        %15.8g\n",
                                                          primalColumnSolution[iColumn],
                                                          dualColumnSolution[iColumn]);
                                             }
                                        }
                                        if (fp != stdout)
                                             fclose(fp);
                                        if (masks) {
                                             delete [] maskStarts;
                                             for (int i = 0; i < maxMasks; i++)
                                                  delete [] masks[i];
                                             delete [] masks;
                                        }
                                   } else {
                                        std::cout << "Unable to open file " << fileName << std::endl;
                                   }
                              } else {
                                   std::cout << "** Current model not valid" << std::endl;

                              }

                              break;
                         case CLP_PARAM_ACTION_SAVESOL:
                              if (goodModels[iModel]) {
                                   // get next field
                                   field = CoinReadGetString(argc, argv);
                                   if (field == "$") {
                                        field = parameters[iParam].stringValue();
                                   } else if (field == "EOL") {
                                        parameters[iParam].printString();
                                        break;
                                   } else {
                                        parameters[iParam].setStringValue(field);
                                   }
                                   std::string fileName;
                                   if (field[0] == '/' || field[0] == '\\') {
                                        fileName = field;
                                   } else if (field[0] == '~') {
                                        char * environVar = getenv("HOME");
                                        if (environVar) {
                                             std::string home(environVar);
                                             field = field.erase(0, 1);
                                             fileName = home + field;
                                        } else {
                                             fileName = field;
                                        }
                                   } else {
                                        fileName = directory + field;
                                   }
                                   saveSolution(models + iModel, fileName);
                              } else {
                                   std::cout << "** Current model not valid" << std::endl;

                              }
                              break;
                         case CLP_PARAM_ACTION_ENVIRONMENT:
                              CbcOrClpEnvironmentIndex = 0;
                              break;
                         default:
                              abort();
                         }
                    }
               } else if (!numberMatches) {
                    std::cout << "No match for " << field << " - ? for list of commands"
                              << std::endl;
               } else if (numberMatches == 1) {
                    if (!numberQuery) {
                         std::cout << "Short match for " << field << " - completion: ";
                         std::cout << parameters[firstMatch].matchName() << std::endl;
                    } else if (numberQuery) {
                         std::cout << parameters[firstMatch].matchName() << " : ";
                         std::cout << parameters[firstMatch].shortHelp() << std::endl;
                         if (numberQuery >= 2)
                              parameters[firstMatch].printLongHelp();
                    }
               } else {
                    if (!numberQuery)
                         std::cout << "Multiple matches for " << field << " - possible completions:"
                                   << std::endl;
                    else
                         std::cout << "Completions of " << field << ":" << std::endl;
                    for ( iParam = 0; iParam < numberParameters; iParam++ ) {
                         int match = parameters[iParam].matches(field);
                         if (match && parameters[iParam].displayThis()) {
                              std::cout << parameters[iParam].matchName();
                              if (numberQuery >= 2)
                                   std::cout << " : " << parameters[iParam].shortHelp();
                              std::cout << std::endl;
                         }
                    }
               }
          }
          delete [] models;
          delete [] goodModels;
     }
#ifdef COIN_HAS_GLPK
     if (cbc_glp_prob) {
       // free up as much as possible
       glp_free(cbc_glp_prob);
       glp_mpl_free_wksp(cbc_glp_tran);
       glp_free_env(); 
       cbc_glp_prob = NULL;
       cbc_glp_tran = NULL;
     }
#endif
     // By now all memory should be freed
#ifdef DMALLOC
     dmalloc_log_unfreed();
     dmalloc_shutdown();
#endif
     return 0;
}
static void breakdown(const char * name, int numberLook, const double * region)
{
     double range[] = {
          -COIN_DBL_MAX,
          -1.0e15, -1.0e11, -1.0e8, -1.0e5, -1.0e4, -1.0e3, -1.0e2, -1.0e1,
          -1.0,
          -1.0e-1, -1.0e-2, -1.0e-3, -1.0e-4, -1.0e-5, -1.0e-8, -1.0e-11, -1.0e-15,
          0.0,
          1.0e-15, 1.0e-11, 1.0e-8, 1.0e-5, 1.0e-4, 1.0e-3, 1.0e-2, 1.0e-1,
          1.0,
          1.0e1, 1.0e2, 1.0e3, 1.0e4, 1.0e5, 1.0e8, 1.0e11, 1.0e15,
          COIN_DBL_MAX
     };
     int nRanges = static_cast<int> (sizeof(range) / sizeof(double));
     int * number = new int[nRanges];
     memset(number, 0, nRanges * sizeof(int));
     int * numberExact = new int[nRanges];
     memset(numberExact, 0, nRanges * sizeof(int));
     int i;
     for ( i = 0; i < numberLook; i++) {
          double value = region[i];
          for (int j = 0; j < nRanges; j++) {
               if (value == range[j]) {
                    numberExact[j]++;
                    break;
               } else if (value < range[j]) {
                    number[j]++;
                    break;
               }
          }
     }
     printf("\n%s has %d entries\n", name, numberLook);
     for (i = 0; i < nRanges; i++) {
          if (number[i])
               printf("%d between %g and %g", number[i], range[i-1], range[i]);
          if (numberExact[i]) {
               if (number[i])
                    printf(", ");
               printf("%d exactly at %g", numberExact[i], range[i]);
          }
          if (number[i] + numberExact[i])
               printf("\n");
     }
     delete [] number;
     delete [] numberExact;
}
void sortOnOther(int * column,
                 const CoinBigIndex * rowStart,
                 int * order,
                 int * other,
                 int nRow,
                 int nInRow,
                 int where)
{
     if (nRow < 2 || where >= nInRow)
          return;
     // do initial sort
     int kRow;
     int iRow;
     for ( kRow = 0; kRow < nRow; kRow++) {
          iRow = order[kRow];
          other[kRow] = column[rowStart[iRow] + where];
     }
     CoinSort_2(other, other + nRow, order);
     int first = 0;
     iRow = order[0];
     int firstC = column[rowStart[iRow] + where];
     kRow = 1;
     while (kRow < nRow) {
          int lastC = 9999999;;
          for (; kRow < nRow + 1; kRow++) {
               if (kRow < nRow) {
                    iRow = order[kRow];
                    lastC = column[rowStart[iRow] + where];
               } else {
                    lastC = 9999999;
               }
               if (lastC > firstC)
                    break;
          }
          // sort
          sortOnOther(column, rowStart, order + first, other, kRow - first,
                      nInRow, where + 1);
          firstC = lastC;
          first = kRow;
     }
}
static void statistics(ClpSimplex * originalModel, ClpSimplex * model)
{
     int numberColumns = originalModel->numberColumns();
     const char * integerInformation  = originalModel->integerInformation();
     const double * columnLower = originalModel->columnLower();
     const double * columnUpper = originalModel->columnUpper();
     int numberIntegers = 0;
     int numberBinary = 0;
     int iRow, iColumn;
     if (integerInformation) {
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               if (integerInformation[iColumn]) {
                    if (columnUpper[iColumn] > columnLower[iColumn]) {
                         numberIntegers++;
                         if (columnLower[iColumn] == 0.0 && columnUpper[iColumn] == 1)
                              numberBinary++;
                    }
               }
          }
	  printf("Original problem has %d integers (%d of which binary)\n",
		 numberIntegers,numberBinary);
     }
     numberColumns = model->numberColumns();
     int numberRows = model->numberRows();
     columnLower = model->columnLower();
     columnUpper = model->columnUpper();
     const double * rowLower = model->rowLower();
     const double * rowUpper = model->rowUpper();
     const double * objective = model->objective();
     if (model->integerInformation()) {
       const char * integerInformation  = model->integerInformation();
       int numberIntegers = 0;
       int numberBinary = 0;
       double * obj = new double [numberColumns];
       int * which = new int [numberColumns];
       for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
	 if (columnUpper[iColumn] > columnLower[iColumn]) {
	   if (integerInformation[iColumn]) {
	     numberIntegers++;
	     if (columnLower[iColumn] == 0.0 && columnUpper[iColumn] == 1)
	       numberBinary++;
	   }
	 }
       }
       if(numberColumns != originalModel->numberColumns())
	 printf("Presolved problem has %d integers (%d of which binary)\n",
		numberIntegers,numberBinary);
       for (int ifInt=0;ifInt<2;ifInt++) {
	 for (int ifAbs=0;ifAbs<2;ifAbs++) {
	   int numberSort=0;
	   int numberZero=0;
	   int numberDifferentObj=0;
	   for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
	     if (columnUpper[iColumn] > columnLower[iColumn]) {
	       if (!ifInt||integerInformation[iColumn]) {
		 obj[numberSort]=(ifAbs) ? fabs(objective[iColumn]) :
		   objective[iColumn];
		 which[numberSort++]=iColumn;
		 if (!objective[iColumn])
		   numberZero++;
	       }
	     }
	   }
	   CoinSort_2(obj,obj+numberSort,which);
	   double last=obj[0];
	   for (int jColumn = 1; jColumn < numberSort; jColumn++) {
	     if (fabs(obj[jColumn]-last)>1.0e-12) {
	       numberDifferentObj++;
	       last=obj[jColumn];
	     }
	   }
	   numberDifferentObj++;
	   printf("==== ");
	   if (ifInt)
	     printf("for integers ");
	   if (!ifAbs)
	     printf("%d zero objective ",numberZero);
	   else
	     printf("absolute objective values ");
	   printf("%d different\n",numberDifferentObj);
	   bool saveModel=false;
	   int target=model->logLevel();
	   if (target>10000) {
	     if (ifInt&&!ifAbs)
	       saveModel=true;
	     target-=10000;
	   }

	   if (target<=100)
	     target=12;
	   else
	     target-=100;
	   if (numberDifferentObj<target) {
	     int iLast=0;
	     double last=obj[0];
	     for (int jColumn = 1; jColumn < numberSort; jColumn++) {
	       if (fabs(obj[jColumn]-last)>1.0e-12) {
		 printf("%d variables have objective of %g\n",
			jColumn-iLast,last);
		 iLast=jColumn;
		 last=obj[jColumn];
	       }
	     }
	     printf("%d variables have objective of %g\n",
		    numberSort-iLast,last);
	     if (saveModel) {
	       int spaceNeeded=numberSort+numberDifferentObj;
	       int * columnAdd = new int[spaceNeeded+numberDifferentObj+1];
	       double * elementAdd = new double[spaceNeeded];
	       int * rowAdd = new int[2*numberDifferentObj+1];
	       int * newIsInteger = rowAdd+numberDifferentObj+1;
	       double * objectiveNew = new double[3*numberDifferentObj];
	       double * lowerNew = objectiveNew+numberDifferentObj;
	       double * upperNew = lowerNew+numberDifferentObj;
	       memset(columnAdd+spaceNeeded,0,
		      (numberDifferentObj+1)*sizeof(int));
	       ClpSimplex tempModel=*model;
	       int iLast=0;
	       double last=obj[0];
	       numberDifferentObj=0;
	       int numberElements=0;
	       rowAdd[0]=0;
	       double * objective = tempModel.objective();
	       for (int jColumn = 1; jColumn < numberSort+1; jColumn++) {
		 if (jColumn==numberSort||fabs(obj[jColumn]-last)>1.0e-12) {
		   // not if just one
		   if (jColumn-iLast>1) {
		     bool allInteger=integerInformation!=NULL;
		     int iColumn=which[iLast];
		     objectiveNew[numberDifferentObj]=objective[iColumn];
		     double lower=0.0;
		     double upper=0.0;
		     for (int kColumn=iLast;kColumn<jColumn;kColumn++) {
		       iColumn=which[kColumn];
		       objective[iColumn]=0.0;
		       double lowerValue=columnLower[iColumn];
		       double upperValue=columnUpper[iColumn];
		       double elementValue=-1.0;
		       if (objectiveNew[numberDifferentObj]*objective[iColumn]<0.0) {
			 lowerValue=-columnUpper[iColumn];
			 upperValue=-columnLower[iColumn];
			 elementValue=1.0;
		       }
		       columnAdd[numberElements]=iColumn;
		       elementAdd[numberElements++]=elementValue;
		       if (integerInformation&&!integerInformation[iColumn])
			 allInteger=false;
		       if (lower!=-COIN_DBL_MAX) {
			 if (lowerValue!=-COIN_DBL_MAX)
			   lower += lowerValue;
			 else
			   lower=-COIN_DBL_MAX;
		       }
		       if (upper!=COIN_DBL_MAX) {
			 if (upperValue!=COIN_DBL_MAX)
			   upper += upperValue;
			 else
			   upper=COIN_DBL_MAX;
		       }
		     }
		     columnAdd[numberElements]=numberColumns+numberDifferentObj;
		     elementAdd[numberElements++]=1.0;
		     newIsInteger[numberDifferentObj]= (allInteger) ? 1 : 0;
		     lowerNew[numberDifferentObj]=lower;
		     upperNew[numberDifferentObj]=upper;
		     numberDifferentObj++;
		     rowAdd[numberDifferentObj]=numberElements;
		   }
		   iLast=jColumn;
		   last=obj[jColumn];
		 }
	       }
	       // add columns
	       tempModel.addColumns(numberDifferentObj, lowerNew, upperNew,
				    objectiveNew,
				    columnAdd+spaceNeeded, NULL, NULL);
	       // add constraints and make integer if all integer in group
	       for (int iObj=0; iObj < numberDifferentObj; iObj++) {
		 lowerNew[iObj]=0.0;
		 upperNew[iObj]=0.0;
		 if (newIsInteger[iObj])
		   tempModel.setInteger(numberColumns+iObj);
	       }
	       tempModel.addRows(numberDifferentObj, lowerNew, upperNew,
				 rowAdd,columnAdd,elementAdd);
	       delete [] columnAdd;
	       delete [] elementAdd;
	       delete [] rowAdd;
	       delete [] objectiveNew;
	       // save
	       std::string tempName = model->problemName();
	       if (ifInt)
		 tempName += "_int";
	       if (ifAbs)
		 tempName += "_abs";
	       tempName += ".mps";
	       tempModel.writeMps(tempName.c_str());
	     }
	   }
	 }
       }
       delete [] which;
       delete [] obj;
       printf("===== end objective counts\n");
     }
     CoinPackedMatrix * matrix = model->matrix();
     CoinBigIndex numberElements = matrix->getNumElements();
     const int * columnLength = matrix->getVectorLengths();
     //const CoinBigIndex * columnStart = matrix->getVectorStarts();
     const double * elementByColumn = matrix->getElements();
     int * number = new int[numberRows+1];
     memset(number, 0, (numberRows + 1)*sizeof(int));
     int numberObjSingletons = 0;
     /* cType
        0 0/inf, 1 0/up, 2 lo/inf, 3 lo/up, 4 free, 5 fix, 6 -inf/0, 7 -inf/up,
        8 0/1
     */
     int cType[9];
     std::string cName[] = {"0.0->inf,", "0.0->up,", "lo->inf,", "lo->up,", "free,", "fixed,", "-inf->0.0,",
                            "-inf->up,", "0.0->1.0"
                           };
     int nObjective = 0;
     memset(cType, 0, sizeof(cType));
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          int length = columnLength[iColumn];
          if (length == 1 && objective[iColumn])
               numberObjSingletons++;
          number[length]++;
          if (objective[iColumn])
               nObjective++;
          if (columnLower[iColumn] > -1.0e20) {
               if (columnLower[iColumn] == 0.0) {
                    if (columnUpper[iColumn] > 1.0e20)
                         cType[0]++;
                    else if (columnUpper[iColumn] == 1.0)
                         cType[8]++;
                    else if (columnUpper[iColumn] == 0.0)
                         cType[5]++;
                    else
                         cType[1]++;
               } else {
                    if (columnUpper[iColumn] > 1.0e20)
                         cType[2]++;
                    else if (columnUpper[iColumn] == columnLower[iColumn])
                         cType[5]++;
                    else
                         cType[3]++;
               }
          } else {
               if (columnUpper[iColumn] > 1.0e20)
                    cType[4]++;
               else if (columnUpper[iColumn] == 0.0)
                    cType[6]++;
               else
                    cType[7]++;
          }
     }
     /* rType
        0 E 0, 1 E 1, 2 E -1, 3 E other, 4 G 0, 5 G 1, 6 G other,
        7 L 0,  8 L 1, 9 L other, 10 Range 0/1, 11 Range other, 12 free
     */
     int rType[13];
     std::string rName[] = {"E 0.0,", "E 1.0,", "E -1.0,", "E other,", "G 0.0,", "G 1.0,", "G other,",
                            "L 0.0,", "L 1.0,", "L other,", "Range 0.0->1.0,", "Range other,", "Free"
                           };
     memset(rType, 0, sizeof(rType));
     for (iRow = 0; iRow < numberRows; iRow++) {
          if (rowLower[iRow] > -1.0e20) {
               if (rowLower[iRow] == 0.0) {
                    if (rowUpper[iRow] > 1.0e20)
                         rType[4]++;
                    else if (rowUpper[iRow] == 1.0)
                         rType[10]++;
                    else if (rowUpper[iRow] == 0.0)
                         rType[0]++;
                    else
                         rType[11]++;
               } else if (rowLower[iRow] == 1.0) {
                    if (rowUpper[iRow] > 1.0e20)
                         rType[5]++;
                    else if (rowUpper[iRow] == rowLower[iRow])
                         rType[1]++;
                    else
                         rType[11]++;
               } else if (rowLower[iRow] == -1.0) {
                    if (rowUpper[iRow] > 1.0e20)
                         rType[6]++;
                    else if (rowUpper[iRow] == rowLower[iRow])
                         rType[2]++;
                    else
                         rType[11]++;
               } else {
                    if (rowUpper[iRow] > 1.0e20)
                         rType[6]++;
                    else if (rowUpper[iRow] == rowLower[iRow])
                         rType[3]++;
                    else
                         rType[11]++;
               }
          } else {
               if (rowUpper[iRow] > 1.0e20)
                    rType[12]++;
               else if (rowUpper[iRow] == 0.0)
                    rType[7]++;
               else if (rowUpper[iRow] == 1.0)
                    rType[8]++;
               else
                    rType[9]++;
          }
     }
     // Basic statistics
     printf("\n\nProblem has %d rows, %d columns (%d with objective) and %d elements\n",
            numberRows, numberColumns, nObjective, numberElements);
     if (number[0] + number[1]) {
          printf("There are ");
          if (numberObjSingletons)
               printf("%d singletons with objective ", numberObjSingletons);
          int numberNoObj = number[1] - numberObjSingletons;
          if (numberNoObj)
               printf("%d singletons with no objective ", numberNoObj);
          if (number[0])
               printf("** %d columns have no entries", number[0]);
          printf("\n");
     }
     printf("Column breakdown:\n");
     int k;
     for (k = 0; k < static_cast<int> (sizeof(cType) / sizeof(int)); k++) {
          printf("%d of type %s ", cType[k], cName[k].c_str());
          if (((k + 1) % 3) == 0)
               printf("\n");
     }
     if ((k % 3) != 0)
          printf("\n");
     printf("Row breakdown:\n");
     for (k = 0; k < static_cast<int> (sizeof(rType) / sizeof(int)); k++) {
          printf("%d of type %s ", rType[k], rName[k].c_str());
          if (((k + 1) % 3) == 0)
               printf("\n");
     }
     if ((k % 3) != 0)
          printf("\n");
     //#define SYM
#ifndef SYM
     if (model->logLevel() < 2)
          return ;
#endif
     int kMax = model->logLevel() > 3 ? 1000000 : 10;
     k = 0;
     for (iRow = 1; iRow <= numberRows; iRow++) {
          if (number[iRow]) {
               k++;
               printf("%d columns have %d entries\n", number[iRow], iRow);
               if (k == kMax)
                    break;
          }
     }
     if (k < numberRows) {
          int kk = k;
          k = 0;
          for (iRow = numberRows; iRow >= 1; iRow--) {
               if (number[iRow]) {
                    k++;
                    if (k == kMax)
                         break;
               }
          }
          if (k > kk) {
               printf("\n    .........\n\n");
               iRow = k;
               k = 0;
               for (; iRow < numberRows; iRow++) {
                    if (number[iRow]) {
                         k++;
                         printf("%d columns have %d entries\n", number[iRow], iRow);
                         if (k == kMax)
                              break;
                    }
               }
          }
     }
     delete [] number;
     printf("\n\n");
     if (model->logLevel() == 63
#ifdef SYM
               || true
#endif
        ) {
          // get column copy
          CoinPackedMatrix columnCopy = *matrix;
          const int * columnLength = columnCopy.getVectorLengths();
          number = new int[numberRows+1];
          memset(number, 0, (numberRows + 1)*sizeof(int));
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               int length = columnLength[iColumn];
               number[length]++;
          }
          k = 0;
          for (iRow = 1; iRow <= numberRows; iRow++) {
               if (number[iRow]) {
                    k++;
               }
          }
          int * row = columnCopy.getMutableIndices();
          const CoinBigIndex * columnStart = columnCopy.getVectorStarts();
          double * element = columnCopy.getMutableElements();
          int * order = new int[numberColumns];
          int * other = new int[numberColumns];
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               int length = columnLength[iColumn];
               order[iColumn] = iColumn;
               other[iColumn] = length;
               CoinBigIndex start = columnStart[iColumn];
               CoinSort_2(row + start, row + start + length, element + start);
          }
          CoinSort_2(other, other + numberColumns, order);
          int jColumn = number[0] + number[1];
          for (iRow = 2; iRow <= numberRows; iRow++) {
               if (number[iRow]) {
                    printf("XX %d columns have %d entries\n", number[iRow], iRow);
                    int kColumn = jColumn + number[iRow];
                    sortOnOther(row, columnStart,
                                order + jColumn, other, number[iRow], iRow, 0);
                    // Now print etc
                    if (iRow < 500000) {
                         for (int lColumn = jColumn; lColumn < kColumn; lColumn++) {
                              iColumn = order[lColumn];
                              CoinBigIndex start = columnStart[iColumn];
                              if (model->logLevel() == 63) {
                                   printf("column %d %g <= ", iColumn, columnLower[iColumn]);
                                   for (CoinBigIndex i = start; i < start + iRow; i++)
                                        printf("( %d, %g) ", row[i], element[i]);
                                   printf("<= %g\n", columnUpper[iColumn]);
                              }
                         }
                    }
                    jColumn = kColumn;
               }
          }
          delete [] order;
          delete [] other;
          delete [] number;
     }
     // get row copy
     CoinPackedMatrix rowCopy = *matrix;
     rowCopy.reverseOrdering();
     const int * rowLength = rowCopy.getVectorLengths();
     number = new int[numberColumns+1];
     memset(number, 0, (numberColumns + 1)*sizeof(int));
     if (model->logLevel() > 3) {
       // get column copy
       CoinPackedMatrix columnCopy = *matrix;
       const int * columnLength = columnCopy.getVectorLengths();
       const int * row = columnCopy.getIndices();
       const CoinBigIndex * columnStart = columnCopy.getVectorStarts();
       const double * element = columnCopy.getElements();
       const double * elementByRow = rowCopy.getElements();
       const int * rowStart = rowCopy.getVectorStarts();
       const int * column = rowCopy.getIndices();
       int nPossibleZeroCost=0;
       int nPossibleNonzeroCost=0;
       for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
	 int length = columnLength[iColumn];
	 if (columnLower[iColumn]<-1.0e30&&columnUpper[iColumn]>1.0e30) {
	   if (length==1) {
	     printf("Singleton free %d - cost %g\n",iColumn,objective[iColumn]);
	   } else if (length==2) {
	     int iRow0=row[columnStart[iColumn]];
	     int iRow1=row[columnStart[iColumn]+1];
	     double element0=element[columnStart[iColumn]];
	     double element1=element[columnStart[iColumn]+1];
	     int n0=rowLength[iRow0];
	     int n1=rowLength[iRow1];
	     printf("Doubleton free %d - cost %g - %g in %srow with %d entries and %g in %srow with %d entries\n",
		    iColumn,objective[iColumn],element0,(rowLower[iRow0]==rowUpper[iRow0]) ? "==" : "",n0,
		    element1,(rowLower[iRow1]==rowUpper[iRow1]) ? "==" : "",n1);

	   }
	 }
	 if (length==1) {
	   int iRow=row[columnStart[iColumn]];
	   double value=COIN_DBL_MAX;
	   for (int i=rowStart[iRow];i<rowStart[iRow]+rowLength[iRow];i++) {
	     int jColumn=column[i];
	     if (jColumn!=iColumn) {
	       if (value!=elementByRow[i]) {
		 if (value==COIN_DBL_MAX) {
		   value=elementByRow[i];
		 } else {
		   value = -COIN_DBL_MAX;
		   break;
		 }
	       }
	     }
	   }
	   if (!objective[iColumn]) {
	     if (model->logLevel() > 4) 
	     printf("Singleton %d with no objective in row with %d elements - rhs %g,%g\n",iColumn,rowLength[iRow],rowLower[iRow],rowUpper[iRow]);
	     nPossibleZeroCost++;
	   } else if (value!=-COIN_DBL_MAX) {
	     if (model->logLevel() > 4) 
	     printf("Singleton %d with objective in row with %d equal elements - rhs %g,%g\n",iColumn,rowLength[iRow],rowLower[iRow],rowUpper[iRow]);
	     nPossibleNonzeroCost++;
	   }
	 }
       }
       if (nPossibleZeroCost||nPossibleNonzeroCost)
	 printf("%d singletons with zero cost, %d with valid cost\n",
		nPossibleZeroCost,nPossibleNonzeroCost);
       // look for DW
       int * blockStart = new int [2*(numberRows+numberColumns)+1+numberRows];
       int * columnBlock = blockStart+numberRows;
       int * nextColumn = columnBlock+numberColumns;
       int * blockCount = nextColumn+numberColumns;
       int * blockEls = blockCount+numberRows+1;
       int direction[2]={-1,1};
       int bestBreak=-1;
       double bestValue=0.0;
       int iPass=0;
       int halfway=(numberRows+1)/2;
       int firstMaster=-1;
       int lastMaster=-2;
       while (iPass<2) {
	 int increment=direction[iPass];
	 int start= increment>0 ? 0 : numberRows-1;
	 int stop=increment>0 ? numberRows : -1;
	 int numberBlocks=0;
	 int thisBestBreak=-1;
	 double thisBestValue=COIN_DBL_MAX;
	 int numberRowsDone=0;
	 int numberMarkedColumns=0;
	 int maximumBlockSize=0;
	 for (int i=0;i<numberRows+2*numberColumns;i++) 
	   blockStart[i]=-1;
	 for (int i=0;i<numberRows+1;i++)
	   blockCount[i]=0;
	 for (int iRow=start;iRow!=stop;iRow+=increment) {
	   int iBlock = -1;
	   for (CoinBigIndex j=rowStart[iRow];j<rowStart[iRow]+rowLength[iRow];j++) {
	     int iColumn=column[j];
	     int whichColumnBlock=columnBlock[iColumn];
	     if (whichColumnBlock>=0) {
	       // column marked
	       if (iBlock<0) {
		 // put row in that block
		 iBlock=whichColumnBlock;
	       } else if (iBlock!=whichColumnBlock) {
		 // merge
		 blockCount[iBlock]+=blockCount[whichColumnBlock];
		 blockCount[whichColumnBlock]=0;
		 int jColumn=blockStart[whichColumnBlock];
		 while (jColumn>=0) {
		   columnBlock[jColumn]=iBlock;
		   iColumn=jColumn;
		   jColumn=nextColumn[jColumn];
		 }
		 nextColumn[iColumn]=blockStart[iBlock];
		 blockStart[iBlock]=blockStart[whichColumnBlock];
		 blockStart[whichColumnBlock]=-1;
	       }
	     }
	   }
	   int n=numberMarkedColumns;
	   if (iBlock<0) {
	     //new block
	     if (rowLength[iRow]) {
	       numberBlocks++;
	       iBlock=numberBlocks;
	       int jColumn=column[rowStart[iRow]];
	       columnBlock[jColumn]=iBlock;
	       blockStart[iBlock]=jColumn;
	       numberMarkedColumns++;
	       for (CoinBigIndex j=rowStart[iRow]+1;j<rowStart[iRow]+rowLength[iRow];j++) {
		 int iColumn=column[j];
		 columnBlock[iColumn]=iBlock;
		 numberMarkedColumns++;
		 nextColumn[jColumn]=iColumn;
		 jColumn=iColumn;
	       }
	       blockCount[iBlock]=numberMarkedColumns-n;
	     } else {
	       // empty
	       iBlock=numberRows;
	     }
	   } else {
	     // put in existing block
	     int jColumn=blockStart[iBlock];
	     for (CoinBigIndex j=rowStart[iRow];j<rowStart[iRow]+rowLength[iRow];j++) {
	       int iColumn=column[j];
	       assert (columnBlock[iColumn]<0||columnBlock[iColumn]==iBlock);
	       if (columnBlock[iColumn]<0) {
		 columnBlock[iColumn]=iBlock;
		 numberMarkedColumns++;
		 nextColumn[iColumn]=jColumn;
		 jColumn=iColumn;
	       }
	     }
	     blockStart[iBlock]=jColumn;
	     blockCount[iBlock]+=numberMarkedColumns-n;
	   }
	   maximumBlockSize=CoinMax(maximumBlockSize,blockCount[iBlock]);
	   numberRowsDone++;
	   if (thisBestValue*numberRowsDone > maximumBlockSize&&numberRowsDone>halfway) { 
	     thisBestBreak=iRow;
	     thisBestValue=static_cast<double>(maximumBlockSize)/
	       static_cast<double>(numberRowsDone);
	   }
	 }
	 if (thisBestBreak==stop)
	   thisBestValue=COIN_DBL_MAX;
	 iPass++;
	 if (iPass==1) {
	   bestBreak=thisBestBreak;
	   bestValue=thisBestValue;
	 } else {
	   if (bestValue<thisBestValue) {
	     firstMaster=0;
	     lastMaster=bestBreak;
	   } else {
	     firstMaster=thisBestBreak; // ? +1
	     lastMaster=numberRows;
	   }
	 }
       }
       if (firstMaster<lastMaster) {
	 printf("%d master rows %d <= < %d\n",lastMaster-firstMaster,
		firstMaster,lastMaster);
	 for (int i=0;i<numberRows+2*numberColumns;i++) 
	   blockStart[i]=-1;
	 for (int i=firstMaster;i<lastMaster;i++)
	   blockStart[i]=-2;
	 int firstRow=0;
	 int numberBlocks=-1;
	 while (true) {
	   for (;firstRow<numberRows;firstRow++) {
	     if (blockStart[firstRow]==-1)
	       break;
	   }
	   if (firstRow==numberRows)
	     break;
	   int nRows=0;
	   numberBlocks++;
	   int numberStack=1;
	   blockCount[0] = firstRow;
	   while (numberStack) {
	     int iRow=blockCount[--numberStack];
	     for (CoinBigIndex j=rowStart[iRow];j<rowStart[iRow]+rowLength[iRow];j++) {
	       int iColumn=column[j];
	       int iBlock=columnBlock[iColumn];
	       if (iBlock<0) {
		 columnBlock[iColumn]=numberBlocks;
		 for (CoinBigIndex k=columnStart[iColumn];
		      k<columnStart[iColumn]+columnLength[iColumn];k++) {
		   int jRow=row[k];
		   int rowBlock=blockStart[jRow];
		   if (rowBlock==-1) {
		     nRows++;
		     blockStart[jRow]=numberBlocks;
		     blockCount[numberStack++]=jRow;
		   }
		 }
	       }
	     }
	   }
	   if (!nRows) {
	     // empty!!
	     numberBlocks--;
	   }
	   firstRow++;
	 }
	 // adjust 
	 numberBlocks++;
	 for (int i=0;i<numberBlocks;i++) {
	   blockCount[i]=0;
	   nextColumn[i]=0;
	 }
	 int numberEmpty=0;
	 int numberMaster=0;
	 memset(blockEls,0,numberBlocks*sizeof(int));
	 for (int iRow = 0; iRow < numberRows; iRow++) {
	   int iBlock=blockStart[iRow];
	   if (iBlock>=0) {
	     blockCount[iBlock]++;
	     blockEls[iBlock]+=rowLength[iRow];
	   } else {
	     if (iBlock==-2)
	       numberMaster++;
	     else
	       numberEmpty++;
	   }
	 }
	 int numberEmptyColumns=0;
	 int numberMasterColumns=0;
	 for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
	   int iBlock=columnBlock[iColumn];
	   if (iBlock>=0) {
	     nextColumn[iBlock]++;
	   } else {
	     if (columnLength[iColumn])
	       numberMasterColumns++;
	     else
	       numberEmptyColumns++;
	   }
	 }
	 int largestRows=0;
	 int largestColumns=0;
	 for (int i=0;i<numberBlocks;i++) {
	   if (blockCount[i]+nextColumn[i]>largestRows+largestColumns) {
	     largestRows=blockCount[i];
	     largestColumns=nextColumn[i];
	   }
	 }
	 bool useful=true;
	 if (numberMaster>halfway||largestRows*3>numberRows)
	   useful=false;
	 printf("%s %d blocks (largest %d,%d), %d master rows (%d empty) out of %d, %d master columns (%d empty) out of %d\n",
		useful ? "**Useful" : "NoGood",
		numberBlocks,largestRows,largestColumns,numberMaster,numberEmpty,numberRows,
		numberMasterColumns,numberEmptyColumns,numberColumns);
	 for (int i=0;i<numberBlocks;i++) 
	   printf("Block %d has %d rows and %d columns (%d elements)\n",
		  i,blockCount[i],nextColumn[i],blockEls[i]);
	 if (model->logLevel() == 17) {
	   int * whichRows=new int[numberRows+numberColumns];
	   int * whichColumns=whichRows+numberRows;
	   char name[20];
	   for (int iBlock=0;iBlock<numberBlocks;iBlock++) {
	     sprintf(name,"block%d.mps",iBlock);
	     int nRows=0;
	     for (int iRow=0;iRow<numberRows;iRow++) {
	       if (blockStart[iRow]==iBlock)
		 whichRows[nRows++]=iRow;
	     }
	     int nColumns=0;
	     for (int iColumn=0;iColumn<numberColumns;iColumn++) {
	       if (columnBlock[iColumn]==iBlock)
		 whichColumns[nColumns++]=iColumn;
	     }
	     ClpSimplex subset(model,nRows,whichRows,nColumns,whichColumns);
	     subset.writeMps(name,0,1);
	   }
	   delete [] whichRows;
	 } 
       }
       delete [] blockStart;
     }
     for (iRow = 0; iRow < numberRows; iRow++) {
          int length = rowLength[iRow];
          number[length]++;
     }
     if (number[0])
          printf("** %d rows have no entries\n", number[0]);
     k = 0;
     for (iColumn = 1; iColumn <= numberColumns; iColumn++) {
          if (number[iColumn]) {
               k++;
               printf("%d rows have %d entries\n", number[iColumn], iColumn);
               if (k == kMax)
                    break;
          }
     }
     if (k < numberColumns) {
          int kk = k;
          k = 0;
          for (iColumn = numberColumns; iColumn >= 1; iColumn--) {
               if (number[iColumn]) {
                    k++;
                    if (k == kMax)
                         break;
               }
          }
          if (k > kk) {
               printf("\n    .........\n\n");
               iColumn = k;
               k = 0;
               for (; iColumn < numberColumns; iColumn++) {
                    if (number[iColumn]) {
                         k++;
                         printf("%d rows have %d entries\n", number[iColumn], iColumn);
                         if (k == kMax)
                              break;
                    }
               }
          }
     }
     if (model->logLevel() == 63
#ifdef SYM
               || true
#endif
        ) {
          int * column = rowCopy.getMutableIndices();
          const CoinBigIndex * rowStart = rowCopy.getVectorStarts();
          double * element = rowCopy.getMutableElements();
          int * order = new int[numberRows];
          int * other = new int[numberRows];
          for (iRow = 0; iRow < numberRows; iRow++) {
               int length = rowLength[iRow];
               order[iRow] = iRow;
               other[iRow] = length;
               CoinBigIndex start = rowStart[iRow];
               CoinSort_2(column + start, column + start + length, element + start);
          }
          CoinSort_2(other, other + numberRows, order);
          int jRow = number[0] + number[1];
          double * weight = new double[numberRows];
          double * randomColumn = new double[numberColumns+1];
          double * randomRow = new double [numberRows+1];
          int * sortRow = new int [numberRows];
          int * possibleRow = new int [numberRows];
          int * backRow = new int [numberRows];
          int * stackRow = new int [numberRows];
          int * sortColumn = new int [numberColumns];
          int * possibleColumn = new int [numberColumns];
          int * backColumn = new int [numberColumns];
          int * backColumn2 = new int [numberColumns];
          int * mapRow = new int [numberRows];
          int * mapColumn = new int [numberColumns];
          int * stackColumn = new int [numberColumns];
          double randomLower = CoinDrand48();
          double randomUpper = CoinDrand48();
          double randomInteger = CoinDrand48();
          int * startAdd = new int[numberRows+1];
          int * columnAdd = new int [2*numberElements];
          double * elementAdd = new double[2*numberElements];
          int nAddRows = 0;
          startAdd[0] = 0;
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               randomColumn[iColumn] = CoinDrand48();
               backColumn2[iColumn] = -1;
          }
          for (iColumn = 2; iColumn <= numberColumns; iColumn++) {
               if (number[iColumn]) {
                    printf("XX %d rows have %d entries\n", number[iColumn], iColumn);
                    int kRow = jRow + number[iColumn];
                    sortOnOther(column, rowStart,
                                order + jRow, other, number[iColumn], iColumn, 0);
                    // Now print etc
                    if (iColumn < 500000) {
                         int nLook = 0;
                         for (int lRow = jRow; lRow < kRow; lRow++) {
                              iRow = order[lRow];
                              CoinBigIndex start = rowStart[iRow];
                              if (model->logLevel() == 63) {
                                   printf("row %d %g <= ", iRow, rowLower[iRow]);
                                   for (CoinBigIndex i = start; i < start + iColumn; i++)
                                        printf("( %d, %g) ", column[i], element[i]);
                                   printf("<= %g\n", rowUpper[iRow]);
                              }
                              int first = column[start];
                              double sum = 0.0;
                              for (CoinBigIndex i = start; i < start + iColumn; i++) {
                                   int jColumn = column[i];
                                   double value = element[i];
                                   jColumn -= first;
                                   assert (jColumn >= 0);
                                   sum += value * randomColumn[jColumn];
                              }
                              if (rowLower[iRow] > -1.0e30 && rowLower[iRow])
                                   sum += rowLower[iRow] * randomLower;
                              else if (!rowLower[iRow])
                                   sum += 1.234567e-7 * randomLower;
                              if (rowUpper[iRow] < 1.0e30 && rowUpper[iRow])
                                   sum += rowUpper[iRow] * randomUpper;
                              else if (!rowUpper[iRow])
                                   sum += 1.234567e-7 * randomUpper;
                              sortRow[nLook] = iRow;
                              randomRow[nLook++] = sum;
                              // best way is to number unique elements and bounds and use
                              if (fabs(sum) > 1.0e4)
                                   sum *= 1.0e-6;
                              weight[iRow] = sum;
                         }
                         assert (nLook <= numberRows);
                         CoinSort_2(randomRow, randomRow + nLook, sortRow);
                         randomRow[nLook] = COIN_DBL_MAX;
                         double last = -COIN_DBL_MAX;
                         int iLast = -1;
                         for (int iLook = 0; iLook < nLook + 1; iLook++) {
                              if (randomRow[iLook] > last) {
                                   if (iLast >= 0) {
                                        int n = iLook - iLast;
                                        if (n > 1) {
                                             //printf("%d rows possible?\n",n);
                                        }
                                   }
                                   iLast = iLook;
                                   last = randomRow[iLook];
                              }
                         }
                    }
                    jRow = kRow;
               }
          }
          CoinPackedMatrix columnCopy = *matrix;
          const int * columnLength = columnCopy.getVectorLengths();
          const int * row = columnCopy.getIndices();
          const CoinBigIndex * columnStart = columnCopy.getVectorStarts();
          const double * elementByColumn = columnCopy.getElements();
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               int length = columnLength[iColumn];
               CoinBigIndex start = columnStart[iColumn];
               double sum = objective[iColumn];
               if (columnLower[iColumn] > -1.0e30 && columnLower[iColumn])
                    sum += columnLower[iColumn] * randomLower;
               else if (!columnLower[iColumn])
                    sum += 1.234567e-7 * randomLower;
               if (columnUpper[iColumn] < 1.0e30 && columnUpper[iColumn])
                    sum += columnUpper[iColumn] * randomUpper;
               else if (!columnUpper[iColumn])
                    sum += 1.234567e-7 * randomUpper;
               if (model->isInteger(iColumn))
                    sum += 9.87654321e-6 * randomInteger;
               for (CoinBigIndex i = start; i < start + length; i++) {
                    int iRow = row[i];
                    sum += elementByColumn[i] * weight[iRow];
               }
               sortColumn[iColumn] = iColumn;
               randomColumn[iColumn] = sum;
          }
          {
               CoinSort_2(randomColumn, randomColumn + numberColumns, sortColumn);
               for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                    int i = sortColumn[iColumn];
                    backColumn[i] = iColumn;
               }
               randomColumn[numberColumns] = COIN_DBL_MAX;
               double last = -COIN_DBL_MAX;
               int iLast = -1;
               for (int iLook = 0; iLook < numberColumns + 1; iLook++) {
                    if (randomColumn[iLook] > last) {
                         if (iLast >= 0) {
                              int n = iLook - iLast;
                              if (n > 1) {
                                   //printf("%d columns possible?\n",n);
                              }
                              for (int i = iLast; i < iLook; i++) {
                                   possibleColumn[sortColumn[i]] = n;
                              }
                         }
                         iLast = iLook;
                         last = randomColumn[iLook];
                    }
               }
               for (iRow = 0; iRow < numberRows; iRow++) {
                    CoinBigIndex start = rowStart[iRow];
                    double sum = 0.0;
                    int length = rowLength[iRow];
                    for (CoinBigIndex i = start; i < start + length; i++) {
                         int jColumn = column[i];
                         double value = element[i];
                         jColumn = backColumn[jColumn];
                         sum += value * randomColumn[jColumn];
                         //if (iColumn==23089||iRow==23729)
                         //printf("row %d cola %d colb %d value %g rand %g sum %g\n",
                         //   iRow,jColumn,column[i],value,randomColumn[jColumn],sum);
                    }
                    sortRow[iRow] = iRow;
                    randomRow[iRow] = weight[iRow];
                    randomRow[iRow] = sum;
               }
               CoinSort_2(randomRow, randomRow + numberRows, sortRow);
               for (iRow = 0; iRow < numberRows; iRow++) {
                    int i = sortRow[iRow];
                    backRow[i] = iRow;
               }
               randomRow[numberRows] = COIN_DBL_MAX;
               last = -COIN_DBL_MAX;
               iLast = -1;
               // Do backward indices from order
               for (iRow = 0; iRow < numberRows; iRow++) {
                    other[order[iRow]] = iRow;
               }
               for (int iLook = 0; iLook < numberRows + 1; iLook++) {
                    if (randomRow[iLook] > last) {
                         if (iLast >= 0) {
                              int n = iLook - iLast;
                              if (n > 1) {
                                   //printf("%d rows possible?\n",n);
                                   // Within group sort as for original "order"
                                   for (int i = iLast; i < iLook; i++) {
                                        int jRow = sortRow[i];
                                        order[i] = other[jRow];
                                   }
                                   CoinSort_2(order + iLast, order + iLook, sortRow + iLast);
                              }
                              for (int i = iLast; i < iLook; i++) {
                                   possibleRow[sortRow[i]] = n;
                              }
                         }
                         iLast = iLook;
                         last = randomRow[iLook];
                    }
               }
               // Temp out
               for (int iLook = 0; iLook < numberRows - 1000000; iLook++) {
                    iRow = sortRow[iLook];
                    CoinBigIndex start = rowStart[iRow];
                    int length = rowLength[iRow];
                    int numberPossible = possibleRow[iRow];
                    for (CoinBigIndex i = start; i < start + length; i++) {
                         int jColumn = column[i];
                         if (possibleColumn[jColumn] != numberPossible)
                              numberPossible = -1;
                    }
                    int n = numberPossible;
                    if (numberPossible > 1) {
                         //printf("pppppossible %d\n",numberPossible);
                         for (int jLook = iLook + 1; jLook < iLook + numberPossible; jLook++) {
                              int jRow = sortRow[jLook];
                              CoinBigIndex start2 = rowStart[jRow];
                              assert (numberPossible == possibleRow[jRow]);
                              assert(length == rowLength[jRow]);
                              for (CoinBigIndex i = start2; i < start2 + length; i++) {
                                   int jColumn = column[i];
                                   if (possibleColumn[jColumn] != numberPossible)
                                        numberPossible = -1;
                              }
                         }
                         if (numberPossible < 2) {
                              // switch off
                              for (int jLook = iLook; jLook < iLook + n; jLook++)
                                   possibleRow[sortRow[jLook]] = -1;
                         }
                         // skip rest
                         iLook += n - 1;
                    } else {
                         possibleRow[iRow] = -1;
                    }
               }
               for (int iLook = 0; iLook < numberRows; iLook++) {
                    iRow = sortRow[iLook];
                    int numberPossible = possibleRow[iRow];
                    // Only if any integers
                    int numberIntegers = 0;
                    CoinBigIndex start = rowStart[iRow];
                    int length = rowLength[iRow];
                    for (CoinBigIndex i = start; i < start + length; i++) {
                         int jColumn = column[i];
                         if (model->isInteger(jColumn))
                              numberIntegers++;
                    }
                    if (numberPossible > 1 && !numberIntegers) {
                         //printf("possible %d - but no integers\n",numberPossible);
                    }
                    if (numberPossible > 1 && (numberIntegers || false)) {
                         //
                         printf("possible %d - %d integers\n", numberPossible, numberIntegers);
                         int lastLook = iLook;
                         int nMapRow = -1;
                         for (int jLook = iLook + 1; jLook < iLook + numberPossible; jLook++) {
                              // stop if too many failures
                              if (jLook > iLook + 10 && nMapRow < 0)
                                   break;
                              // Create identity mapping
                              int i;
                              for (i = 0; i < numberRows; i++)
                                   mapRow[i] = i;
                              for (i = 0; i < numberColumns; i++)
                                   mapColumn[i] = i;
                              int offset = jLook - iLook;
                              int nStackC = 0;
                              // build up row and column mapping
                              int nStackR = 1;
                              stackRow[0] = iLook;
                              bool good = true;
                              while (nStackR) {
                                   nStackR--;
                                   int look1 = stackRow[nStackR];
                                   int look2 = look1 + offset;
                                   assert (randomRow[look1] == randomRow[look2]);
                                   int row1 = sortRow[look1];
                                   int row2 = sortRow[look2];
                                   assert (mapRow[row1] == row1);
                                   assert (mapRow[row2] == row2);
                                   mapRow[row1] = row2;
                                   mapRow[row2] = row1;
                                   CoinBigIndex start1 = rowStart[row1];
                                   CoinBigIndex offset2 = rowStart[row2] - start1;
                                   int length = rowLength[row1];
                                   assert( length == rowLength[row2]);
                                   for (CoinBigIndex i = start1; i < start1 + length; i++) {
                                        int jColumn1 = column[i];
                                        int jColumn2 = column[i+offset2];
                                        if (randomColumn[backColumn[jColumn1]] !=
                                                  randomColumn[backColumn[jColumn2]]) {
                                             good = false;
                                             break;
                                        }
                                        if (mapColumn[jColumn1] == jColumn1) {
                                             // not touched
                                             assert (mapColumn[jColumn2] == jColumn2);
                                             if (jColumn1 != jColumn2) {
                                                  // Put on stack
                                                  mapColumn[jColumn1] = jColumn2;
                                                  mapColumn[jColumn2] = jColumn1;
                                                  stackColumn[nStackC++] = jColumn1;
                                             }
                                        } else {
                                             if (mapColumn[jColumn1] != jColumn2 ||
                                                       mapColumn[jColumn2] != jColumn1) {
                                                  // bad
                                                  good = false;
                                                  printf("bad col\n");
                                                  break;
                                             }
                                        }
                                   }
                                   if (!good)
                                        break;
                                   while (nStackC) {
                                        nStackC--;
                                        int iColumn = stackColumn[nStackC];
                                        int iColumn2 = mapColumn[iColumn];
                                        assert (iColumn != iColumn2);
                                        int length = columnLength[iColumn];
                                        assert (length == columnLength[iColumn2]);
                                        CoinBigIndex start = columnStart[iColumn];
                                        CoinBigIndex offset2 = columnStart[iColumn2] - start;
                                        for (CoinBigIndex i = start; i < start + length; i++) {
                                             int iRow = row[i];
                                             int iRow2 = row[i+offset2];
                                             if (mapRow[iRow] == iRow) {
                                                  // First (but be careful)
                                                  if (iRow != iRow2) {
                                                       //mapRow[iRow]=iRow2;
                                                       //mapRow[iRow2]=iRow;
                                                       int iBack = backRow[iRow];
                                                       int iBack2 = backRow[iRow2];
                                                       if (randomRow[iBack] == randomRow[iBack2] &&
                                                                 iBack2 - iBack == offset) {
                                                            stackRow[nStackR++] = iBack;
                                                       } else {
                                                            //printf("randomRow diff - weights %g %g\n",
                                                            //     weight[iRow],weight[iRow2]);
                                                            // bad
                                                            good = false;
                                                            break;
                                                       }
                                                  }
                                             } else {
                                                  if (mapRow[iRow] != iRow2 ||
                                                            mapRow[iRow2] != iRow) {
                                                       // bad
                                                       good = false;
                                                       printf("bad row\n");
                                                       break;
                                                  }
                                             }
                                        }
                                        if (!good)
                                             break;
                                   }
                              }
                              // then check OK
                              if (good) {
                                   for (iRow = 0; iRow < numberRows; iRow++) {
                                        CoinBigIndex start = rowStart[iRow];
                                        int length = rowLength[iRow];
                                        if (mapRow[iRow] == iRow) {
                                             for (CoinBigIndex i = start; i < start + length; i++) {
                                                  int jColumn = column[i];
                                                  backColumn2[jColumn] = i - start;
                                             }
                                             for (CoinBigIndex i = start; i < start + length; i++) {
                                                  int jColumn = column[i];
                                                  if (mapColumn[jColumn] != jColumn) {
                                                       int jColumn2 = mapColumn[jColumn];
                                                       CoinBigIndex i2 = backColumn2[jColumn2];
                                                       if (i2 < 0) {
                                                            good = false;
                                                       } else if (element[i] != element[i2+start]) {
                                                            good = false;
                                                       }
                                                  }
                                             }
                                             for (CoinBigIndex i = start; i < start + length; i++) {
                                                  int jColumn = column[i];
                                                  backColumn2[jColumn] = -1;
                                             }
                                        } else {
                                             int row2 = mapRow[iRow];
                                             assert (iRow = mapRow[row2]);
                                             if (rowLower[iRow] != rowLower[row2] ||
                                                       rowLower[row2] != rowLower[iRow])
                                                  good = false;
                                             CoinBigIndex offset2 = rowStart[row2] - start;
                                             for (CoinBigIndex i = start; i < start + length; i++) {
                                                  int jColumn = column[i];
                                                  double value = element[i];
                                                  int jColumn2 = column[i+offset2];
                                                  double value2 = element[i+offset2];
                                                  if (value != value2 || mapColumn[jColumn] != jColumn2 ||
                                                            mapColumn[jColumn2] != jColumn)
                                                       good = false;
                                             }
                                        }
                                   }
                                   if (good) {
                                        // check rim
                                        for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                                             if (mapColumn[iColumn] != iColumn) {
                                                  int iColumn2 = mapColumn[iColumn];
                                                  if (objective[iColumn] != objective[iColumn2])
                                                       good = false;
                                                  if (columnLower[iColumn] != columnLower[iColumn2])
                                                       good = false;
                                                  if (columnUpper[iColumn] != columnUpper[iColumn2])
                                                       good = false;
                                                  if (model->isInteger(iColumn) != model->isInteger(iColumn2))
                                                       good = false;
                                             }
                                        }
                                   }
                                   if (good) {
                                        // temp
                                        if (nMapRow < 0) {
                                             //const double * solution = model->primalColumnSolution();
                                             // find mapped
                                             int nMapColumn = 0;
                                             for (int i = 0; i < numberColumns; i++) {
                                                  if (mapColumn[i] > i)
                                                       nMapColumn++;
                                             }
                                             nMapRow = 0;
                                             int kRow = -1;
                                             for (int i = 0; i < numberRows; i++) {
                                                  if (mapRow[i] > i) {
                                                       nMapRow++;
                                                       kRow = i;
                                                  }
                                             }
                                             printf("%d columns, %d rows\n", nMapColumn, nMapRow);
                                             if (nMapRow == 1) {
                                                  CoinBigIndex start = rowStart[kRow];
                                                  int length = rowLength[kRow];
                                                  printf("%g <= ", rowLower[kRow]);
                                                  for (CoinBigIndex i = start; i < start + length; i++) {
                                                       int jColumn = column[i];
                                                       if (mapColumn[jColumn] != jColumn)
                                                            printf("* ");
                                                       printf("%d,%g ", jColumn, element[i]);
                                                  }
                                                  printf("<= %g\n", rowUpper[kRow]);
                                             }
                                        }
                                        // temp
                                        int row1 = sortRow[lastLook];
                                        int row2 = sortRow[jLook];
                                        lastLook = jLook;
                                        CoinBigIndex start1 = rowStart[row1];
                                        CoinBigIndex offset2 = rowStart[row2] - start1;
                                        int length = rowLength[row1];
                                        assert( length == rowLength[row2]);
                                        CoinBigIndex put = startAdd[nAddRows];
                                        double multiplier = length < 11 ? 2.0 : 1.125;
                                        double value = 1.0;
                                        for (CoinBigIndex i = start1; i < start1 + length; i++) {
                                             int jColumn1 = column[i];
                                             int jColumn2 = column[i+offset2];
                                             columnAdd[put] = jColumn1;
                                             elementAdd[put++] = value;
                                             columnAdd[put] = jColumn2;
                                             elementAdd[put++] = -value;
                                             value *= multiplier;
                                        }
                                        nAddRows++;
                                        startAdd[nAddRows] = put;
                                   } else {
                                        printf("ouch - did not check out as good\n");
                                   }
                              }
                         }
                         // skip rest
                         iLook += numberPossible - 1;
                    }
               }
          }
          if (nAddRows) {
               double * lower = new double [nAddRows];
               double * upper = new double[nAddRows];
               int i;
               //const double * solution = model->primalColumnSolution();
               for (i = 0; i < nAddRows; i++) {
                    lower[i] = 0.0;
                    upper[i] = COIN_DBL_MAX;
               }
               printf("Adding %d rows with %d elements\n", nAddRows,
                      startAdd[nAddRows]);
               //ClpSimplex newModel(*model);
               //newModel.addRows(nAddRows,lower,upper,startAdd,columnAdd,elementAdd);
               //newModel.writeMps("modified.mps");
               delete [] lower;
               delete [] upper;
          }
          delete [] startAdd;
          delete [] columnAdd;
          delete [] elementAdd;
          delete [] order;
          delete [] other;
          delete [] randomColumn;
          delete [] weight;
          delete [] randomRow;
          delete [] sortRow;
          delete [] backRow;
          delete [] possibleRow;
          delete [] sortColumn;
          delete [] backColumn;
          delete [] backColumn2;
          delete [] possibleColumn;
          delete [] mapRow;
          delete [] mapColumn;
          delete [] stackRow;
          delete [] stackColumn;
     }
     delete [] number;
     // Now do breakdown of ranges
     breakdown("Elements", numberElements, elementByColumn);
     breakdown("RowLower", numberRows, rowLower);
     breakdown("RowUpper", numberRows, rowUpper);
     breakdown("ColumnLower", numberColumns, columnLower);
     breakdown("ColumnUpper", numberColumns, columnUpper);
     breakdown("Objective", numberColumns, objective);
}
static bool maskMatches(const int * starts, char ** masks,
                        std::string & check)
{
     // back to char as I am old fashioned
     const char * checkC = check.c_str();
     size_t length = strlen(checkC);
     while (checkC[length-1] == ' ')
          length--;
     for (int i = starts[length]; i < starts[length+1]; i++) {
          char * thisMask = masks[i];
          size_t k;
          for ( k = 0; k < length; k++) {
               if (thisMask[k] != '?' && thisMask[k] != checkC[k])
                    break;
          }
          if (k == length)
               return true;
     }
     return false;
}
static void clean(char * temp)
{
     char * put = temp;
     while (*put >= ' ')
          put++;
     *put = '\0';
}
static void generateCode(const char * fileName, int type)
{
     FILE * fp = fopen(fileName, "r");
     assert (fp);
     int numberLines = 0;
#define MAXLINES 500
#define MAXONELINE 200
     char line[MAXLINES][MAXONELINE];
     while (fgets(line[numberLines], MAXONELINE, fp)) {
          assert (numberLines < MAXLINES);
          clean(line[numberLines]);
          numberLines++;
     }
     fclose(fp);
     // add in actual solve
     strcpy(line[numberLines], "5  clpModel->initialSolve(clpSolve);");
     numberLines++;
     fp = fopen(fileName, "w");
     assert (fp);
     char apo = '"';
     char backslash = '\\';

     fprintf(fp, "#include %cClpSimplex.hpp%c\n", apo, apo);
     fprintf(fp, "#include %cClpSolve.hpp%c\n", apo, apo);

     fprintf(fp, "\nint main (int argc, const char *argv[])\n{\n");
     fprintf(fp, "  ClpSimplex  model;\n");
     fprintf(fp, "  int status=1;\n");
     fprintf(fp, "  if (argc<2)\n");
     fprintf(fp, "    fprintf(stderr,%cPlease give file name%cn%c);\n",
             apo, backslash, apo);
     fprintf(fp, "  else\n");
     fprintf(fp, "    status=model.readMps(argv[1],true);\n");
     fprintf(fp, "  if (status) {\n");
     fprintf(fp, "    fprintf(stderr,%cBad readMps %%s%cn%c,argv[1]);\n",
             apo, backslash, apo);
     fprintf(fp, "    exit(1);\n");
     fprintf(fp, "  }\n\n");
     fprintf(fp, "  // Now do requested saves and modifications\n");
     fprintf(fp, "  ClpSimplex * clpModel = & model;\n");
     int wanted[9];
     memset(wanted, 0, sizeof(wanted));
     wanted[0] = wanted[3] = wanted[5] = wanted[8] = 1;
     if (type > 0)
          wanted[1] = wanted[6] = 1;
     if (type > 1)
          wanted[2] = wanted[4] = wanted[7] = 1;
     std::string header[9] = { "", "Save values", "Redundant save of default values", "Set changed values",
                               "Redundant set default values", "Solve", "Restore values", "Redundant restore values", "Add to model"
                             };
     for (int iType = 0; iType < 9; iType++) {
          if (!wanted[iType])
               continue;
          int n = 0;
          int iLine;
          for (iLine = 0; iLine < numberLines; iLine++) {
               if (line[iLine][0] == '0' + iType) {
                    if (!n)
                         fprintf(fp, "\n  // %s\n\n", header[iType].c_str());
                    n++;
                    fprintf(fp, "%s\n", line[iLine] + 1);
               }
          }
     }
     fprintf(fp, "\n  // Now you would use solution etc etc\n\n");
     fprintf(fp, "  return 0;\n}\n");
     fclose(fp);
     printf("C++ file written to %s\n", fileName);
}
/*
  Version 1.00.00 October 13 2004.
  1.00.01 October 18.  Added basis handling helped/prodded by Thorsten Koch.
  Also modifications to make faster with sbb (I hope I haven't broken anything).
  1.00.02 March 21 2005.  Redid ClpNonLinearCost to save memory also redid
  createRim to try and improve cache characteristics.
  1.00.03 April 8 2005.  Added Volume algorithm as crash and made code more
  robust on testing.  Also added "either" and "tune" algorithm.
  1.01.01 April 12 2005.  Decided to go to different numbering.  Backups will
  be last 2 digits while middle 2 are for improvements.  Still take a long
  time to get to 2.00.01
  1.01.02 May 4 2005.  Will be putting in many changes - so saving stable version
  1.02.01 May 6 2005.  Lots of changes to try and make faster and more stable in
  branch and cut.
  1.02.02 May 19 2005.  Stuff for strong branching and some improvements to simplex
  1.03.01 May 24 2006.  Lots done but I can't remember what!
  1.03.03 June 13 2006.  For clean up after dual perturbation
  1.04.01 June 26 2007.  Lots of changes but I got lazy
  1.05.00 June 27 2007.  This is trunk so when gets to stable will be 1.5
  1.11.00 November 5 2009 (Guy Fawkes) - OSL factorization and better ordering
 */
#ifdef CILK_TEST
// -*- C++ -*-

/*
 * cilk-for.cilk
 *
 * Copyright (c) 2007-2008 Cilk Arts, Inc.  55 Cambridge Street,
 * Burlington, MA 01803.  Patents pending.  All rights reserved. You may
 * freely use the sample code to guide development of your own works,
 * provided that you reproduce this notice in any works you make that
 * use the sample code.  This sample code is provided "AS IS" without
 * warranty of any kind, either express or implied, including but not
 * limited to any implied warranty of non-infringement, merchantability
 * or fitness for a particular purpose.  In no event shall Cilk Arts,
 * Inc. be liable for any direct, indirect, special, or consequential
 * damages, or any other damages whatsoever, for any use of or reliance
 * on this sample code, including, without limitation, any lost
 * opportunity, lost profits, business interruption, loss of programs or
 * data, even if expressly advised of or otherwise aware of the
 * possibility of such damages, whether in an action of contract,
 * negligence, tort, or otherwise.
 *
 * This file demonstrates a Cilk++ for loop
 */

#include <cilk/cilk.h>
//#include <cilk/cilkview.h>
#include <cilk/reducer_max.h>
#include <cstdlib>
#include <iostream>

// cilk_for granularity.
#define CILK_FOR_GRAINSIZE 128

double dowork(double i)
{
    // Waste time:
    int j;
    double k = i;
    for (j = 0; j < 50000; ++j) {
        k += k / ((j + 1) * (k + 1));
    }

    return k;
}
static void doSomeWork(double * a,int low, int high)
{
  if (high-low>300) {
    int mid=(high+low)>>1;
    cilk_spawn doSomeWork(a,low,mid);
    doSomeWork(a,mid,high);
    cilk_sync;
  } else {
    for(int i = low; i < high; ++i) {
      a[i] = dowork(a[i]);
    }
  }
}

void cilkTest()
{
    unsigned int n = 10000;
    //cilk::cilkview cv;


    double* a = new double[n];

    for(unsigned int i = 0; i < n; i++) {
        // Populate A 
        a[i] = (double) ((i * i) % 1024 + 512) / 512;
    }

    std::cout << "Iterating over " << n << " integers" << std::endl;

    //cv.start();
#if 1
    //#pragma cilk_grainsize=CILK_FOR_GRAINSIZE
    cilk_for(unsigned int i = 0; i < n; ++i) {
        a[i] = dowork(a[i]);
    }
#else
    doSomeWork(a,0,n);
#endif
    int * which =new int[n];
    unsigned int n2=n>>1;
    for (int i=0;i<n2;i++) 
      which[i]=n-2*i;
    cilk::reducer_max_index<int,double> maximumIndex(-1,0.0);
    cilk_for(unsigned int i = 0; i < n2; ++i) {
      int iWhich=which[i];
      maximumIndex.calc_max(iWhich,a[iWhich]);
    }
    int bestIndex=maximumIndex.get_index();
    int bestIndex2=-1;
    double largest=0.0;
    cilk_for(unsigned int i = 0; i < n2; ++i) {
      int iWhich=which[i];
      if (a[iWhich]>largest) {
	bestIndex2=iWhich;
	largest=a[iWhich];
      }
    }
    assert (bestIndex==bestIndex2);
    //cv.stop();
    //cv.dump("cilk-for-results", false);

    //std::cout << cv.accumulated_milliseconds() / 1000.f << " seconds" << std::endl;

    exit(0);
}
#endif
