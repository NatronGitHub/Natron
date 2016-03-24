/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifdef NDEBUG
#undef NDEBUG
#endif

#include "ClpConfig.h"
#include "CoinPragma.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cfloat>
#include <string>
#include <iostream>

#include "CoinMpsIO.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinPackedVector.hpp"
#include "CoinStructuredModel.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinTime.hpp"
#include "CoinFloatEqual.hpp"

#if CLP_HAS_ABC
#include "CoinAbcCommon.hpp"
#endif
#ifdef ABC_INHERIT
#include "CoinAbcFactorization.hpp"
#endif
#include "ClpFactorization.hpp"
#include "ClpSimplex.hpp"
#include "ClpSimplexOther.hpp"
#include "ClpSimplexNonlinear.hpp"
#include "ClpInterior.hpp"
#include "ClpLinearObjective.hpp"
#include "ClpDualRowSteepest.hpp"
#include "ClpDualRowDantzig.hpp"
#include "ClpPrimalColumnSteepest.hpp"
#include "ClpPrimalColumnDantzig.hpp"
#include "ClpParameters.hpp"
#include "ClpNetworkMatrix.hpp"
#include "ClpPlusMinusOneMatrix.hpp"
#include "MyMessageHandler.hpp"
#include "MyEventHandler.hpp"

#include "ClpPresolve.hpp"
#include "Idiot.hpp"
#if FACTORIZATION_STATISTICS
extern double ftranTwiddleFactor1X;
extern double ftranTwiddleFactor2X;
extern double ftranFTTwiddleFactor1X;
extern double ftranFTTwiddleFactor2X;
extern double btranTwiddleFactor1X;
extern double btranTwiddleFactor2X;
extern double ftranFullTwiddleFactor1X;
extern double ftranFullTwiddleFactor2X;
extern double btranFullTwiddleFactor1X;
extern double btranFullTwiddleFactor2X;
extern double denseThresholdX;
extern double twoThresholdX;
extern double minRowsSparse;
extern double largeRowsSparse;
extern double mediumRowsDivider;
extern double mediumRowsMinCount;
extern double largeRowsCount;
#endif

//#############################################################################


// Function Prototypes. Function definitions is in this file.
void testingMessage( const char * const msg );
#if defined(COIN_HAS_AMD) || defined(COIN_HAS_CHOLMOD) || defined(COIN_HAS_GLPK)
static int barrierAvailable = 1;
static std::string nameBarrier = "barrier-UFL";
#elif COIN_HAS_WSMP
static int barrierAvailable = 2;
static std::string nameBarrier = "barrier-WSSMP";
#elif defined(COIN_HAS_MUMPS)
static int barrierAvailable = 3;
static std::string nameBarrier = "barrier-MUMPS";
#else
static int barrierAvailable = 0;
static std::string nameBarrier = "barrier-slow";
#endif
#define NUMBER_ALGORITHMS 12
// If you just want a subset then set some to 1
static int switchOff[NUMBER_ALGORITHMS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
// shortName - 0 no , 1 yes
ClpSolve setupForSolve(int algorithm, std::string & nameAlgorithm,
                       int shortName)
{
     ClpSolve solveOptions;
     /* algorithms are
        0 barrier
        1 dual with volumne crash
        2,3 dual with and without crash
        4,5 primal with and without
        6,7 automatic with and without
        8,9 primal with idiot 1 and 5
        10,11 primal with 70, dual with volume
     */
     switch (algorithm) {
     case 0:
          if (shortName)
               nameAlgorithm = "ba";
          else
               nameAlgorithm = "nameBarrier";
          solveOptions.setSolveType(ClpSolve::useBarrier);
          if (barrierAvailable == 1)
               solveOptions.setSpecialOption(4, 4);
          else if (barrierAvailable == 2)
               solveOptions.setSpecialOption(4, 2);
          break;
     case 1:
#ifdef COIN_HAS_VOL
          if (shortName)
               nameAlgorithm = "du-vol-50";
          else
               nameAlgorithm = "dual-volume-50";
          solveOptions.setSolveType(ClpSolve::useDual);
          solveOptions.setSpecialOption(0, 2, 50); // volume
#else
          solveOptions.setSolveType(ClpSolve::notImplemented);
#endif
          break;
     case 2:
          if (shortName)
               nameAlgorithm = "du-cr";
          else
               nameAlgorithm = "dual-crash";
          solveOptions.setSolveType(ClpSolve::useDual);
          solveOptions.setSpecialOption(0, 1);
          break;
     case 3:
          if (shortName)
               nameAlgorithm = "du";
          else
               nameAlgorithm = "dual";
          solveOptions.setSolveType(ClpSolve::useDual);
          break;
     case 4:
          if (shortName)
               nameAlgorithm = "pr-cr";
          else
               nameAlgorithm = "primal-crash";
          solveOptions.setSolveType(ClpSolve::usePrimal);
          solveOptions.setSpecialOption(1, 1);
          break;
     case 5:
          if (shortName)
               nameAlgorithm = "pr";
          else
               nameAlgorithm = "primal";
          solveOptions.setSolveType(ClpSolve::usePrimal);
          break;
     case 6:
          if (shortName)
               nameAlgorithm = "au-cr";
          else
               nameAlgorithm = "either-crash";
          solveOptions.setSolveType(ClpSolve::automatic);
          solveOptions.setSpecialOption(1, 1);
          break;
     case 7:
          if (shortName)
               nameAlgorithm = "au";
          else
               nameAlgorithm = "either";
          solveOptions.setSolveType(ClpSolve::automatic);
          break;
     case 8:
          if (shortName)
               nameAlgorithm = "pr-id-1";
          else
               nameAlgorithm = "primal-idiot-1";
          solveOptions.setSolveType(ClpSolve::usePrimalorSprint);
          solveOptions.setSpecialOption(1, 2, 1); // idiot
          break;
     case 9:
          if (shortName)
               nameAlgorithm = "pr-id-5";
          else
               nameAlgorithm = "primal-idiot-5";
          solveOptions.setSolveType(ClpSolve::usePrimalorSprint);
          solveOptions.setSpecialOption(1, 2, 5); // idiot
          break;
     case 10:
          if (shortName)
               nameAlgorithm = "pr-id-70";
          else
               nameAlgorithm = "primal-idiot-70";
          solveOptions.setSolveType(ClpSolve::usePrimalorSprint);
          solveOptions.setSpecialOption(1, 2, 70); // idiot
          break;
     case 11:
#ifdef COIN_HAS_VOL
          if (shortName)
               nameAlgorithm = "du-vol";
          else
               nameAlgorithm = "dual-volume";
          solveOptions.setSolveType(ClpSolve::useDual);
          solveOptions.setSpecialOption(0, 2, 3000); // volume
#else
          solveOptions.setSolveType(ClpSolve::notImplemented);
#endif
          break;
     default:
          abort();
     }
     if (shortName) {
          // can switch off
          if (switchOff[algorithm])
               solveOptions.setSolveType(ClpSolve::notImplemented);
     }
     return solveOptions;
}
static void printSol(ClpSimplex & model)
{
     int numberRows = model.numberRows();
     int numberColumns = model.numberColumns();

     double * rowPrimal = model.primalRowSolution();
     double * rowDual = model.dualRowSolution();
     double * rowLower = model.rowLower();
     double * rowUpper = model.rowUpper();

     int iRow;
     double objValue = model.getObjValue();
     printf("Objvalue %g Rows (%d)\n", objValue, numberRows);
     for (iRow = 0; iRow < numberRows; iRow++) {
          printf("%d primal %g dual %g low %g up %g\n",
                 iRow, rowPrimal[iRow], rowDual[iRow],
                 rowLower[iRow], rowUpper[iRow]);
     }
     double * columnPrimal = model.primalColumnSolution();
     double * columnDual = model.dualColumnSolution();
     double * columnLower = model.columnLower();
     double * columnUpper = model.columnUpper();
     double offset;
     //const double * gradient = model.objectiveAsObject()->gradient(&model,
     //                                                       columnPrimal,offset,true,1);
     const double * gradient = model.objective(columnPrimal, offset);
     int iColumn;
     objValue = -offset - model.objectiveOffset();
     printf("offset %g (%g)\n", offset, model.objectiveOffset());
     printf("Columns (%d)\n", numberColumns);
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          printf("%d primal %g dual %g low %g up %g\n",
                 iColumn, columnPrimal[iColumn], columnDual[iColumn],
                 columnLower[iColumn], columnUpper[iColumn]);
          objValue += columnPrimal[iColumn] * gradient[iColumn];
          if (fabs(columnPrimal[iColumn]*gradient[iColumn]) > 1.0e-8)
               printf("obj -> %g gradient %g\n", objValue, gradient[iColumn]);
     }
     printf("Computed objective %g\n", objValue);
}

void usage(const std::string& key)
{
     std::cerr
               << "Undefined parameter \"" << key << "\".\n"
               << "Correct usage: \n"
               << "  clp [-dirSample=V1] [-dirNetlib=V2] [-netlib]\n"
               << "  where:\n"
               << "    -dirSample: directory containing mps test files\n"
               << "        Default value V1=\"../../Data/Sample\"\n"
               << "    -dirNetlib: directory containing netlib files\"\n"
               << "        Default value V2=\"../../Data/Netlib\"\n"
               << "    -netlib\n"
               << "        If specified, then netlib testset run as well as the nitTest.\n";
}
#if FACTORIZATION_STATISTICS
int loSizeX=-1;
int hiSizeX=1000000;
#endif
//----------------------------------------------------------------
#ifndef ABC_INHERIT
#define AnySimplex ClpSimplex
#else
#include "AbcSimplex.hpp"
#define AnySimplex AbcSimplex
#endif
int mainTest (int argc, const char *argv[], int algorithm,
              AnySimplex empty, ClpSolve solveOptionsIn,
              int switchOffValue, bool doVector)
{
     int i;

     if (switchOffValue > 0) {
          // switch off some
          int iTest;
          for (iTest = 0; iTest < NUMBER_ALGORITHMS; iTest++) {
#ifndef NDEBUG
               int bottom = switchOffValue % 10;
               assert (bottom == 0 || bottom == 1);
#endif
               switchOffValue /= 10;
               switchOff[iTest] = 0;
          }
     }
     int numberFailures=0;
     // define valid parameter keywords
     std::set<std::string> definedKeyWords;
     definedKeyWords.insert("-dirSample");
     definedKeyWords.insert("-dirNetlib");
     definedKeyWords.insert("-netlib");

     // Create a map of parameter keys and associated data
     std::map<std::string, std::string> parms;
     for ( i = 1; i < argc; i++ ) {
          std::string parm(argv[i]);
          std::string key, value;
          std::string::size_type  eqPos = parm.find('=');

          // Does parm contain and '='
          if ( eqPos == std::string::npos ) {
               //Parm does not contain '='
               key = parm;
          } else {
               key = parm.substr(0, eqPos);
               value = parm.substr(eqPos + 1);
          }

          // Is specifed key valid?
          if ( definedKeyWords.find(key) == definedKeyWords.end() ) {
               // invalid key word.
               // Write help text
               usage(key);
               return 1;
          }
          parms[key] = value;
     }

     const char dirsep =  CoinFindDirSeparator();
     // Set directory containing mps data files.
     std::string dirSample;
     if (parms.find("-dirSample") != parms.end())
          dirSample = parms["-dirSample"];
     else
          dirSample = dirsep == '/' ? "../../Data/Sample/" : "..\\..\\Data\\Sample\\";

     // Set directory containing netlib data files.
     std::string dirNetlib;
     if (parms.find("-dirNetlib") != parms.end())
          dirNetlib = parms["-dirNetlib"];
     else
          dirNetlib = dirsep == '/' ? "../../Data/Netlib/" : "..\\..\\Data\\Netlib\\";
#if 0 //FACTORIZATION_STATISTICS==0
     if (!empty.numberRows()) {
          testingMessage( "Testing ClpSimplex\n" );
          ClpSimplexUnitTest(dirSample);
     }
#endif
     if (parms.find("-netlib") != parms.end() || empty.numberRows()) {
          unsigned int m;
	  std::string sizeLoHi;
#if FACTORIZATION_STATISTICS
	  double ftranTwiddleFactor1XChoice[3]={0.9,1.0,1.1};
	  double ftranTwiddleFactor2XChoice[3]={0.9,1.0,1.1};
	  double ftranFTTwiddleFactor1XChoice[3]={0.9,1.0,1.1};
	  double ftranFTTwiddleFactor2XChoice[3]={0.9,1.0,1.1};
	  double btranTwiddleFactor1XChoice[3]={0.9,1.0,1.1};
	  double btranTwiddleFactor2XChoice[3]={0.9,1.0,1.1};
	  double ftranFullTwiddleFactor1XChoice[3]={0.9,1.0,1.1};
	  double ftranFullTwiddleFactor2XChoice[3]={0.9,1.0,1.1};
	  double btranFullTwiddleFactor1XChoice[3]={0.9,1.0,1.1};
	  double btranFullTwiddleFactor2XChoice[3]={0.9,1.0,1.1};
	  double denseThresholdXChoice[3]={2,20,40};
	  double twoThresholdXChoice[3]={800,1000,1200};
	  double minRowsSparseChoice[3]={250,300,350};
	  double largeRowsSparseChoice[3]={8000,10000,12000};
	  double mediumRowsDividerChoice[3]={5,6,7};
	  double mediumRowsMinCountChoice[3]={300,500,600};
	  double largeRowsCountChoice[3]={700,1000,1300};
	  double times[3*3*3*3*3];
	  memset(times,0,sizeof(times));
#define whichParam(za,zname) const char * choice##za=#zname; \
       const double * use##za=zname##Choice;		     \
       double * external##za=&zname
	  whichParam(A,denseThresholdX);
	  whichParam(B,twoThresholdX);
	  whichParam(C,minRowsSparse);
	  whichParam(D,mediumRowsDivider);
	  whichParam(E,mediumRowsMinCount);
#endif
          // Define test problems:
          //   mps names,
          //   maximization or minimization,
          //   Number of rows and columns in problem, and
          //   objective function value
          std::vector<std::string> mpsName;
          std::vector<bool> min;
          std::vector<int> nRows;
          std::vector<int> nCols;
          std::vector<double> objValue;
          std::vector<double> objValueTol;
          // 100 added means no presolve
          std::vector<int> bestStrategy;
          if(empty.numberRows()) {
               std::string alg;
               for (int iTest = 0; iTest < NUMBER_ALGORITHMS; iTest++) {
                    ClpSolve solveOptions = setupForSolve(iTest, alg, 0);
                    printf("%d %s ", iTest, alg.c_str());
                    if (switchOff[iTest])
                         printf("skipped by user\n");
                    else if(solveOptions.getSolveType() == ClpSolve::notImplemented)
                         printf("skipped as not available\n");
                    else
                         printf("will be tested\n");
               }
          }
          if (!empty.numberRows()) {
#if 1
               mpsName.push_back("25fv47");
               min.push_back(true);
               nRows.push_back(822);
               nCols.push_back(1571);
               objValueTol.push_back(1.E-10);
               objValue.push_back(5.5018458883E+03);
               bestStrategy.push_back(0);
               mpsName.push_back("80bau3b");
               min.push_back(true);
               nRows.push_back(2263);
               nCols.push_back(9799);
               objValueTol.push_back(1.e-8);
               objValue.push_back(9.8722419241E+05);
               bestStrategy.push_back(3);
               mpsName.push_back("blend");
               min.push_back(true);
               nRows.push_back(75);
               nCols.push_back(83);
               objValueTol.push_back(1.e-8);
               objValue.push_back(-3.0812149846e+01);
               bestStrategy.push_back(3);
               mpsName.push_back("pilotnov");
               min.push_back(true);
               nRows.push_back(976);
               nCols.push_back(2172);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-4.4972761882e+03);
               bestStrategy.push_back(3);
               mpsName.push_back("maros-r7");
               min.push_back(true);
               nRows.push_back(3137);
               nCols.push_back(9408);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.4971851665e+06);
               bestStrategy.push_back(2);
               mpsName.push_back("pilot");
               min.push_back(true);
               nRows.push_back(1442);
               nCols.push_back(3652);
               objValueTol.push_back(1.e-5);
               objValue.push_back(/*-5.5740430007e+02*/ -557.48972927292);
               bestStrategy.push_back(3);
               mpsName.push_back("pilot4");
               min.push_back(true);
               nRows.push_back(411);
               nCols.push_back(1000);
               objValueTol.push_back(5.e-5);
               objValue.push_back(-2.5811392641e+03);
               bestStrategy.push_back(3);
#endif
               mpsName.push_back("pilot87");
               min.push_back(true);
               nRows.push_back(2031);
               nCols.push_back(4883);
               objValueTol.push_back(1.e-4);
               objValue.push_back(3.0171072827e+02);
               bestStrategy.push_back(0);
#if 1
               mpsName.push_back("adlittle");
               min.push_back(true);
               nRows.push_back(57);
               nCols.push_back(97);
               objValueTol.push_back(1.e-10);
               objValue.push_back(2.2549496316e+05);
               bestStrategy.push_back(3);
               mpsName.push_back("afiro");
               min.push_back(true);
               nRows.push_back(28);
               nCols.push_back(32);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-4.6475314286e+02);
               bestStrategy.push_back(3);
               mpsName.push_back("agg");
               min.push_back(true);
               nRows.push_back(489);
               nCols.push_back(163);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-3.5991767287e+07);
               bestStrategy.push_back(3);
               mpsName.push_back("agg2");
               min.push_back(true);
               nRows.push_back(517);
               nCols.push_back(302);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-2.0239252356e+07);
               bestStrategy.push_back(3);
               mpsName.push_back("agg3");
               min.push_back(true);
               nRows.push_back(517);
               nCols.push_back(302);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.0312115935e+07);
               bestStrategy.push_back(4);
               mpsName.push_back("bandm");
               min.push_back(true);
               nRows.push_back(306);
               nCols.push_back(472);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-1.5862801845e+02);
               bestStrategy.push_back(2);
               mpsName.push_back("beaconfd");
               min.push_back(true);
               nRows.push_back(174);
               nCols.push_back(262);
               objValueTol.push_back(1.e-10);
               objValue.push_back(3.3592485807e+04);
               bestStrategy.push_back(0);
               mpsName.push_back("bnl1");
               min.push_back(true);
               nRows.push_back(644);
               nCols.push_back(1175);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.9776295615E+03);
               bestStrategy.push_back(3);
               mpsName.push_back("bnl2");
               min.push_back(true);
               nRows.push_back(2325);
               nCols.push_back(3489);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.8112365404e+03);
               bestStrategy.push_back(3);
               mpsName.push_back("boeing1");
               min.push_back(true);
               nRows.push_back(/*351*/352);
               nCols.push_back(384);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-3.3521356751e+02);
               bestStrategy.push_back(3);
               mpsName.push_back("boeing2");
               min.push_back(true);
               nRows.push_back(167);
               nCols.push_back(143);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-3.1501872802e+02);
               bestStrategy.push_back(3);
               mpsName.push_back("bore3d");
               min.push_back(true);
               nRows.push_back(234);
               nCols.push_back(315);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.3730803942e+03);
               bestStrategy.push_back(3);
               mpsName.push_back("brandy");
               min.push_back(true);
               nRows.push_back(221);
               nCols.push_back(249);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.5185098965e+03);
               bestStrategy.push_back(3);
               mpsName.push_back("capri");
               min.push_back(true);
               nRows.push_back(272);
               nCols.push_back(353);
               objValueTol.push_back(1.e-10);
               objValue.push_back(2.6900129138e+03);
               bestStrategy.push_back(3);
               mpsName.push_back("cycle");
               min.push_back(true);
               nRows.push_back(1904);
               nCols.push_back(2857);
               objValueTol.push_back(1.e-9);
               objValue.push_back(-5.2263930249e+00);
               bestStrategy.push_back(3);
               mpsName.push_back("czprob");
               min.push_back(true);
               nRows.push_back(930);
               nCols.push_back(3523);
               objValueTol.push_back(1.e-10);
               objValue.push_back(2.1851966989e+06);
               bestStrategy.push_back(3);
               mpsName.push_back("d2q06c");
               min.push_back(true);
               nRows.push_back(2172);
               nCols.push_back(5167);
               objValueTol.push_back(1.e-7);
               objValue.push_back(122784.21557456);
               bestStrategy.push_back(0);
               mpsName.push_back("d6cube");
               min.push_back(true);
               nRows.push_back(416);
               nCols.push_back(6184);
               objValueTol.push_back(1.e-7);
               objValue.push_back(3.1549166667e+02);
               bestStrategy.push_back(3);
               mpsName.push_back("degen2");
               min.push_back(true);
               nRows.push_back(445);
               nCols.push_back(534);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-1.4351780000e+03);
               bestStrategy.push_back(3);
               mpsName.push_back("degen3");
               min.push_back(true);
               nRows.push_back(1504);
               nCols.push_back(1818);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-9.8729400000e+02);
               bestStrategy.push_back(2);
               mpsName.push_back("dfl001");
               min.push_back(true);
               nRows.push_back(6072);
               nCols.push_back(12230);
               objValueTol.push_back(1.e-5);
               objValue.push_back(1.1266396047E+07);
               bestStrategy.push_back(5);
               mpsName.push_back("e226");
               min.push_back(true);
               nRows.push_back(224);
               nCols.push_back(282);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-1.8751929066e+01 + 7.113);
               bestStrategy.push_back(3); // The correct answer includes -7.113 term. This is a constant in the objective function. See line 1683 of the mps file.
               mpsName.push_back("etamacro");
               min.push_back(true);
               nRows.push_back(401);
               nCols.push_back(688);
               objValueTol.push_back(1.e-6);
               objValue.push_back(-7.5571521774e+02 );
               bestStrategy.push_back(3);
               mpsName.push_back("fffff800");
               min.push_back(true);
               nRows.push_back(525);
               nCols.push_back(854);
               objValueTol.push_back(1.e-6);
               objValue.push_back(5.5567961165e+05);
               bestStrategy.push_back(3);
               mpsName.push_back("finnis");
               min.push_back(true);
               nRows.push_back(498);
               nCols.push_back(614);
               objValueTol.push_back(1.e-6);
               objValue.push_back(1.7279096547e+05);
               bestStrategy.push_back(3);
               mpsName.push_back("fit1d");
               min.push_back(true);
               nRows.push_back(25);
               nCols.push_back(1026);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-9.1463780924e+03);
               bestStrategy.push_back(3 + 100);
               mpsName.push_back("fit1p");
               min.push_back(true);
               nRows.push_back(628);
               nCols.push_back(1677);
               objValueTol.push_back(1.e-10);
               objValue.push_back(9.1463780924e+03);
               bestStrategy.push_back(5 + 100);
               mpsName.push_back("fit2d");
               min.push_back(true);
               nRows.push_back(26);
               nCols.push_back(10500);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-6.8464293294e+04);
               bestStrategy.push_back(3 + 100);
               mpsName.push_back("fit2p");
               min.push_back(true);
               nRows.push_back(3001);
               nCols.push_back(13525);
               objValueTol.push_back(1.e-9);
               objValue.push_back(6.8464293232e+04);
               bestStrategy.push_back(5 + 100);
               mpsName.push_back("forplan");
               min.push_back(true);
               nRows.push_back(162);
               nCols.push_back(421);
               objValueTol.push_back(1.e-6);
               objValue.push_back(-6.6421873953e+02);
               bestStrategy.push_back(3);
               mpsName.push_back("ganges");
               min.push_back(true);
               nRows.push_back(1310);
               nCols.push_back(1681);
               objValueTol.push_back(1.e-5);
               objValue.push_back(-1.0958636356e+05);
               bestStrategy.push_back(3);
               mpsName.push_back("gfrd-pnc");
               min.push_back(true);
               nRows.push_back(617);
               nCols.push_back(1092);
               objValueTol.push_back(1.e-10);
               objValue.push_back(6.9022359995e+06);
               bestStrategy.push_back(3);
               mpsName.push_back("greenbea");
               min.push_back(true);
               nRows.push_back(2393);
               nCols.push_back(5405);
               objValueTol.push_back(1.e-10);
               objValue.push_back(/*-7.2462405908e+07*/ -72555248.129846);
               bestStrategy.push_back(3);
               mpsName.push_back("greenbeb");
               min.push_back(true);
               nRows.push_back(2393);
               nCols.push_back(5405);
               objValueTol.push_back(1.e-10);
               objValue.push_back(/*-4.3021476065e+06*/ -4302260.2612066);
               bestStrategy.push_back(3);
               mpsName.push_back("grow15");
               min.push_back(true);
               nRows.push_back(301);
               nCols.push_back(645);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-1.0687094129e+08);
               bestStrategy.push_back(4 + 100);
               mpsName.push_back("grow22");
               min.push_back(true);
               nRows.push_back(441);
               nCols.push_back(946);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-1.6083433648e+08);
               bestStrategy.push_back(4 + 100);
               mpsName.push_back("grow7");
               min.push_back(true);
               nRows.push_back(141);
               nCols.push_back(301);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-4.7787811815e+07);
               bestStrategy.push_back(4 + 100);
               mpsName.push_back("israel");
               min.push_back(true);
               nRows.push_back(175);
               nCols.push_back(142);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-8.9664482186e+05);
               bestStrategy.push_back(2);
               mpsName.push_back("kb2");
               min.push_back(true);
               nRows.push_back(44);
               nCols.push_back(41);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-1.7499001299e+03);
               bestStrategy.push_back(3);
               mpsName.push_back("lotfi");
               min.push_back(true);
               nRows.push_back(154);
               nCols.push_back(308);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-2.5264706062e+01);
               bestStrategy.push_back(3);
               mpsName.push_back("maros");
               min.push_back(true);
               nRows.push_back(847);
               nCols.push_back(1443);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-5.8063743701e+04);
               bestStrategy.push_back(3);
               mpsName.push_back("modszk1");
               min.push_back(true);
               nRows.push_back(688);
               nCols.push_back(1620);
               objValueTol.push_back(1.e-10);
               objValue.push_back(3.2061972906e+02);
               bestStrategy.push_back(3);
               mpsName.push_back("nesm");
               min.push_back(true);
               nRows.push_back(663);
               nCols.push_back(2923);
               objValueTol.push_back(1.e-5);
               objValue.push_back(1.4076073035e+07);
               bestStrategy.push_back(2);
               mpsName.push_back("perold");
               min.push_back(true);
               nRows.push_back(626);
               nCols.push_back(1376);
               objValueTol.push_back(1.e-6);
               objValue.push_back(-9.3807580773e+03);
               bestStrategy.push_back(3);
               //mpsName.push_back("qap12");min.push_back(true);nRows.push_back(3193);nCols.push_back(8856);objValueTol.push_back(1.e-6);objValue.push_back(5.2289435056e+02);bestStrategy.push_back(3);
               //mpsName.push_back("qap15");min.push_back(true);nRows.push_back(6331);nCols.push_back(22275);objValueTol.push_back(1.e-10);objValue.push_back(1.0409940410e+03);bestStrategy.push_back(3);
               mpsName.push_back("recipe");
               min.push_back(true);
               nRows.push_back(92);
               nCols.push_back(180);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-2.6661600000e+02);
               bestStrategy.push_back(3);
               mpsName.push_back("sc105");
               min.push_back(true);
               nRows.push_back(106);
               nCols.push_back(103);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-5.2202061212e+01);
               bestStrategy.push_back(3);
               mpsName.push_back("sc205");
               min.push_back(true);
               nRows.push_back(206);
               nCols.push_back(203);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-5.2202061212e+01);
               bestStrategy.push_back(3);
               mpsName.push_back("sc50a");
               min.push_back(true);
               nRows.push_back(51);
               nCols.push_back(48);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-6.4575077059e+01);
               bestStrategy.push_back(3);
               mpsName.push_back("sc50b");
               min.push_back(true);
               nRows.push_back(51);
               nCols.push_back(48);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-7.0000000000e+01);
               bestStrategy.push_back(3);
               mpsName.push_back("scagr25");
               min.push_back(true);
               nRows.push_back(472);
               nCols.push_back(500);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-1.4753433061e+07);
               bestStrategy.push_back(3);
               mpsName.push_back("scagr7");
               min.push_back(true);
               nRows.push_back(130);
               nCols.push_back(140);
               objValueTol.push_back(1.e-6);
               objValue.push_back(-2.3313892548e+06);
               bestStrategy.push_back(3);
               mpsName.push_back("scfxm1");
               min.push_back(true);
               nRows.push_back(331);
               nCols.push_back(457);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.8416759028e+04);
               bestStrategy.push_back(3);
               mpsName.push_back("scfxm2");
               min.push_back(true);
               nRows.push_back(661);
               nCols.push_back(914);
               objValueTol.push_back(1.e-10);
               objValue.push_back(3.6660261565e+04);
               bestStrategy.push_back(3);
               mpsName.push_back("scfxm3");
               min.push_back(true);
               nRows.push_back(991);
               nCols.push_back(1371);
               objValueTol.push_back(1.e-10);
               objValue.push_back(5.4901254550e+04);
               bestStrategy.push_back(3);
               mpsName.push_back("scorpion");
               min.push_back(true);
               nRows.push_back(389);
               nCols.push_back(358);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.8781248227e+03);
               bestStrategy.push_back(3);
               mpsName.push_back("scrs8");
               min.push_back(true);
               nRows.push_back(491);
               nCols.push_back(1169);
               objValueTol.push_back(1.e-5);
               objValue.push_back(9.0429998619e+02);
               bestStrategy.push_back(2);
               mpsName.push_back("scsd1");
               min.push_back(true);
               nRows.push_back(78);
               nCols.push_back(760);
               objValueTol.push_back(1.e-10);
               objValue.push_back(8.6666666743e+00);
               bestStrategy.push_back(3 + 100);
               mpsName.push_back("scsd6");
               min.push_back(true);
               nRows.push_back(148);
               nCols.push_back(1350);
               objValueTol.push_back(1.e-10);
               objValue.push_back(5.0500000078e+01);
               bestStrategy.push_back(3 + 100);
               mpsName.push_back("scsd8");
               min.push_back(true);
               nRows.push_back(398);
               nCols.push_back(2750);
               objValueTol.push_back(1.e-10);
               objValue.push_back(9.0499999993e+02);
               bestStrategy.push_back(1 + 100);
               mpsName.push_back("sctap1");
               min.push_back(true);
               nRows.push_back(301);
               nCols.push_back(480);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.4122500000e+03);
               bestStrategy.push_back(3);
               mpsName.push_back("sctap2");
               min.push_back(true);
               nRows.push_back(1091);
               nCols.push_back(1880);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.7248071429e+03);
               bestStrategy.push_back(3);
               mpsName.push_back("sctap3");
               min.push_back(true);
               nRows.push_back(1481);
               nCols.push_back(2480);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.4240000000e+03);
               bestStrategy.push_back(3);
               mpsName.push_back("seba");
               min.push_back(true);
               nRows.push_back(516);
               nCols.push_back(1028);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.5711600000e+04);
               bestStrategy.push_back(3);
               mpsName.push_back("share1b");
               min.push_back(true);
               nRows.push_back(118);
               nCols.push_back(225);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-7.6589318579e+04);
               bestStrategy.push_back(3);
               mpsName.push_back("share2b");
               min.push_back(true);
               nRows.push_back(97);
               nCols.push_back(79);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-4.1573224074e+02);
               bestStrategy.push_back(3);
               mpsName.push_back("shell");
               min.push_back(true);
               nRows.push_back(537);
               nCols.push_back(1775);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.2088253460e+09);
               bestStrategy.push_back(3);
               mpsName.push_back("ship04l");
               min.push_back(true);
               nRows.push_back(403);
               nCols.push_back(2118);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.7933245380e+06);
               bestStrategy.push_back(3);
               mpsName.push_back("ship04s");
               min.push_back(true);
               nRows.push_back(403);
               nCols.push_back(1458);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.7987147004e+06);
               bestStrategy.push_back(3);
               mpsName.push_back("ship08l");
               min.push_back(true);
               nRows.push_back(779);
               nCols.push_back(4283);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.9090552114e+06);
               bestStrategy.push_back(3);
               mpsName.push_back("ship08s");
               min.push_back(true);
               nRows.push_back(779);
               nCols.push_back(2387);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.9200982105e+06);
               bestStrategy.push_back(2);
               mpsName.push_back("ship12l");
               min.push_back(true);
               nRows.push_back(1152);
               nCols.push_back(5427);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.4701879193e+06);
               bestStrategy.push_back(3);
               mpsName.push_back("ship12s");
               min.push_back(true);
               nRows.push_back(1152);
               nCols.push_back(2763);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.4892361344e+06);
               bestStrategy.push_back(2);
               mpsName.push_back("sierra");
               min.push_back(true);
               nRows.push_back(1228);
               nCols.push_back(2036);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.5394362184e+07);
               bestStrategy.push_back(3);
               mpsName.push_back("stair");
               min.push_back(true);
               nRows.push_back(357);
               nCols.push_back(467);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-2.5126695119e+02);
               bestStrategy.push_back(3);
               mpsName.push_back("standata");
               min.push_back(true);
               nRows.push_back(360);
               nCols.push_back(1075);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.2576995000e+03);
               bestStrategy.push_back(3);
               //mpsName.push_back("standgub");min.push_back(true);nRows.push_back(362);nCols.push_back(1184);objValueTol.push_back(1.e-10);objValue.push_back(1257.6995); bestStrategy.push_back(3);
               mpsName.push_back("standmps");
               min.push_back(true);
               nRows.push_back(468);
               nCols.push_back(1075);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.4060175000E+03);
               bestStrategy.push_back(3);
               mpsName.push_back("stocfor1");
               min.push_back(true);
               nRows.push_back(118);
               nCols.push_back(111);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-4.1131976219E+04);
               bestStrategy.push_back(3);
               mpsName.push_back("stocfor2");
               min.push_back(true);
               nRows.push_back(2158);
               nCols.push_back(2031);
               objValueTol.push_back(1.e-10);
               objValue.push_back(-3.9024408538e+04);
               bestStrategy.push_back(3);
               //mpsName.push_back("stocfor3");min.push_back(true);nRows.push_back(16676);nCols.push_back(15695);objValueTol.push_back(1.e-10);objValue.push_back(-3.9976661576e+04);bestStrategy.push_back(3);
               //mpsName.push_back("truss");min.push_back(true);nRows.push_back(1001);nCols.push_back(8806);objValueTol.push_back(1.e-10);objValue.push_back(4.5881584719e+05);bestStrategy.push_back(3);
               mpsName.push_back("tuff");
               min.push_back(true);
               nRows.push_back(334);
               nCols.push_back(587);
               objValueTol.push_back(1.e-10);
               objValue.push_back(2.9214776509e-01);
               bestStrategy.push_back(3);
               mpsName.push_back("vtpbase");
               min.push_back(true);
               nRows.push_back(199);
               nCols.push_back(203);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.2983146246e+05);
               bestStrategy.push_back(3);
               mpsName.push_back("wood1p");
               min.push_back(true);
               nRows.push_back(245);
               nCols.push_back(2594);
               objValueTol.push_back(5.e-5);
               objValue.push_back(1.4429024116e+00);
               bestStrategy.push_back(3);
               mpsName.push_back("woodw");
               min.push_back(true);
               nRows.push_back(1099);
               nCols.push_back(8405);
               objValueTol.push_back(1.e-10);
               objValue.push_back(1.3044763331E+00);
               bestStrategy.push_back(3);
#endif
          } else {
               // Just testing one
               mpsName.push_back(empty.problemName());
               min.push_back(true);
               nRows.push_back(-1);
               nCols.push_back(-1);
               objValueTol.push_back(1.e-10);
               objValue.push_back(0.0);
               bestStrategy.push_back(0);
               int iTest;
               std::string alg;
               for (iTest = 0; iTest < NUMBER_ALGORITHMS; iTest++) {
                    ClpSolve solveOptions = setupForSolve(iTest, alg, 0);
                    printf("%d %s ", iTest, alg.c_str());
                    if (switchOff[iTest])
                         printf("skipped by user\n");
                    else if(solveOptions.getSolveType() == ClpSolve::notImplemented)
                         printf("skipped as not available\n");
                    else
                         printf("will be tested\n");
               }
          }

          double timeTaken = 0.0;
          if( !barrierAvailable)
               switchOff[0] = 1;
          // Loop once for each Mps File
          for (m = 0; m < mpsName.size(); m++ ) {
#if FACTORIZATION_STATISTICS
	       if (nRows[m]<loSizeX||nRows[m]>=hiSizeX) {
		 std::cerr << "  skipping mps file: " << mpsName[m]
			   <<" as "<<nRows[m]
                         << " (" << m + 1 << " out of " << mpsName.size() << ")" << std::endl;
		 continue;
	       }
#endif
               std::cerr << "  processing mps file: " << mpsName[m]
                         << " (" << m + 1 << " out of " << mpsName.size() << ")" << std::endl;
               AnySimplex solutionBase = empty;
               std::string fn = dirNetlib + mpsName[m];
               if (!empty.numberRows() || algorithm < 6) {
                    // Read data mps file,
                    CoinMpsIO mps;
                    int nerrors = mps.readMps(fn.c_str(), "mps");
                    if (nerrors) {
                         std::cerr << "Error " << nerrors << " when reading model from "
                                   << fn.c_str() << "! Aborting tests.\n";
                         return 1;
                    }
                    solutionBase.loadProblem(*mps.getMatrixByCol(), mps.getColLower(),
                                             mps.getColUpper(),
                                             mps.getObjCoefficients(),
                                             mps.getRowLower(), mps.getRowUpper());

                    solutionBase.setDblParam(ClpObjOffset, mps.objectiveOffset());
               }

               // Runs through strategies
               if (algorithm == 6 || algorithm == 7) {
                    // algorithms tested are at top of file
                    double testTime[NUMBER_ALGORITHMS];
                    std::string alg[NUMBER_ALGORITHMS];
                    int iTest;
                    for (iTest = 0; iTest < NUMBER_ALGORITHMS; iTest++) {
                         ClpSolve solveOptions = setupForSolve(iTest, alg[iTest], 1);
                         if (solveOptions.getSolveType() != ClpSolve::notImplemented) {
                              double time1 = CoinCpuTime();
                              ClpSimplex solution = solutionBase;
                              if (solution.maximumSeconds() < 0.0)
                                   solution.setMaximumSeconds(120.0);
                              if (doVector) {
                                   ClpMatrixBase * matrix = solution.clpMatrix();
                                   if (dynamic_cast< ClpPackedMatrix*>(matrix)) {
                                        ClpPackedMatrix * clpMatrix = dynamic_cast< ClpPackedMatrix*>(matrix);
                                        clpMatrix->makeSpecialColumnCopy();
                                   }
                              }
                              solution.initialSolve(solveOptions);
                              double time2 = CoinCpuTime() - time1;
                              testTime[iTest] = time2;
                              printf("Finished %s Took %g seconds (%d iterations) - status %d\n", 
				     mpsName[m].c_str(),time2, solution.problemStatus(),solution.numberIterations());
                              if (solution.problemStatus())
                                   testTime[iTest] = 1.0e20;
                         } else {
                              testTime[iTest] = 1.0e30;
                         }
                    }
                    int iBest = -1;
                    double dBest = 1.0e10;
                    printf("%s", fn.c_str());
                    for (iTest = 0; iTest < NUMBER_ALGORITHMS; iTest++) {
                         if (testTime[iTest] < 1.0e30) {
                              printf(" %s %g",
                                     alg[iTest].c_str(), testTime[iTest]);
                              if (testTime[iTest] < dBest) {
                                   dBest = testTime[iTest];
                                   iBest = iTest;
                              }
                         }
                    }
                    printf("\n");
                    if (iBest >= 0)
                         printf("Best strategy for %s is %s (%d) which takes %g seconds\n",
                                fn.c_str(), alg[iBest].c_str(), iBest, testTime[iBest]);
                    else
                         printf("No strategy finished in time limit\n");
                    continue;
               }
               double time1 = CoinCpuTime();
               AnySimplex solution = solutionBase;
#if 0
               solution.setOptimizationDirection(-1);
               {
                    int j;
                    double * obj = solution.objective();
                    int n = solution.numberColumns();
                    for (j = 0; j < n; j++)
                         obj[j] *= -1.0;
               }
#endif
               ClpSolve::SolveType method;
               ClpSolve solveOptions = solveOptionsIn;
               std::string nameAlgorithm;
               if (algorithm != 5) {
                    if (algorithm == 0) {
                         method = ClpSolve::useDual;
                         nameAlgorithm = "dual";
                    } else if (algorithm == 1) {
                         method = ClpSolve::usePrimalorSprint;
                         nameAlgorithm = "primal";
                    } else if (algorithm == 3) {
                         method = ClpSolve::automatic;
                         nameAlgorithm = "either";
                    } else {
                         nameAlgorithm = "barrier-slow";
#if defined(COIN_HAS_AMD) || defined(COIN_HAS_CHOLMOD) || defined(COIN_HAS_GLPK)
                         solveOptions.setSpecialOption(4, 4);
                         nameAlgorithm = "barrier-UFL";
#endif
#ifdef COIN_HAS_WSMP
                         solveOptions.setSpecialOption(4, 2);
                         nameAlgorithm = "barrier-WSSMP";
#endif
#ifdef COIN_HAS_MUMPS
                         solveOptions.setSpecialOption(4, 6);
                         nameAlgorithm = "barrier-MUMPS";
#endif
                         method = ClpSolve::useBarrier;
                    }
                    solveOptions.setSolveType(method);
               } else {
                    int iAlg = bestStrategy[m];
                    int presolveOff = iAlg / 100;
                    iAlg = iAlg % 100;
                    if( !barrierAvailable && iAlg == 0) {
                         if (nRows[m] != 2172)
                              iAlg = 5; // try primal
                         else
                              iAlg = 3; // d2q06c
                    }
                    solveOptions = setupForSolve(iAlg, nameAlgorithm, 0);
                    if (presolveOff)
                         solveOptions.setPresolveType(ClpSolve::presolveOff);
               }
               if (doVector) {
                    ClpMatrixBase * matrix = solution.clpMatrix();
                    if (dynamic_cast< ClpPackedMatrix*>(matrix)) {
                         ClpPackedMatrix * clpMatrix = dynamic_cast< ClpPackedMatrix*>(matrix);
                         clpMatrix->makeSpecialColumnCopy();
                    }
               }
#if FACTORIZATION_STATISTICS
	       double timesOne[3*3*3*3*3];
	       memset(timesOne,0,sizeof(timesOne));
	       int iterationsOne[3*3*3*3*3];
	       memset(iterationsOne,0,sizeof(iterationsOne));
	       AnySimplex saveModel(solution);
	       double time2;
#if 0
               solution.initialSolve(solveOptions);
               time2 = CoinCpuTime() - time1;
               timeTaken += time2;
               printf("%s took %g seconds using algorithm %s\n", fn.c_str(), time2, nameAlgorithm.c_str());
#endif
#define loA 1
#define loB 0
#define loC 0
#define loD 1
#define loE 1
#define hiA 2
#define hiB 1
#define hiC 1
#define hiD 2
#define hiE 2
               time2 = CoinCpuTime();
	       for (int iA=loA;iA<hiA;iA++) {
		 *externalA=useA[iA];
		 for (int iB=loB;iB<hiB;iB++) {
		   *externalB=useB[iB];
		   for (int iC=loC;iC<hiC;iC++) {
		     *externalC=useC[iC];
		     for (int iD=loD;iD<hiD;iD++) {
		       *externalD=useD[iD];
		       for (int iE=loE;iE<hiE;iE++) {
			 *externalE=useE[iE];
			 solution=saveModel;
			 solution.initialSolve(solveOptions);
			 double time3=CoinCpuTime();
			 printf("Finished %s Took %g seconds (%d iterations) - status %d\n", 
				mpsName[m].c_str(),time3-time2,solution.numberIterations(), solution.problemStatus());
			 iterationsOne[iA+3*iB+9*iC+27*iD+81*iE]=solution.numberIterations();
			 timesOne[iA+3*iB+9*iC+27*iD+81*iE]=time3-time2;
			 times[iA+3*iB+9*iC+27*iD+81*iE]+=time3-time2;
			 time2=time3;
		       }
		     }
		   }
		 }
	       }
	       double bestTime=1.0e100;
	       int iBestTime=-1;
	       double bestTimePer=1.0e100;
	       int iBestTimePer=-1;
	       int bestIterations=1000000;
	       int iBestIterations=-1;
	       printf("TTimes ");
	       for (int i=0;i<3*3*3*3*3;i++) {
		 // skip if not done
		 if (!iterationsOne[i])
		   continue;
		 double average=timesOne[i]/static_cast<double>(iterationsOne[i]);
		 if (timesOne[i]<bestTime) {
		   bestTime=timesOne[i];
		   iBestTime=i;
		 }
		 if (average<bestTimePer) {
		   bestTimePer=average;
		   iBestTimePer=i;
		 }
		 if (iterationsOne[i]<bestIterations) {
		   bestIterations=iterationsOne[i];
		   iBestIterations=i;
		 }
		 printf("%.2f ",timesOne[i]);
	       }
	       printf("\n");
	       int iA,iB,iC,iD,iE;
	       iA=iBestTime;
	       iE=iA/81;
	       iA-=81*iE;
	       iD=iA/27;
	       iA-=27*iD;
	       iC=iA/9;
	       iA-=9*iC;
	       iB=iA/3;
	       iA-=3*iB;
	       printf("Best time %.2f %s=%g %s=%g %s=%g %s=%g %s=%g\n",bestTime,choiceA,useA[iA],
		      choiceB,useB[iB],choiceC,useC[iC],choiceD,useD[iD],choiceE,useE[iE]);
	       iA=iBestTimePer;
	       iE=iA/81;
	       iA-=81*iE;
	       iD=iA/27;
	       iA-=27*iD;
	       iC=iA/9;
	       iA-=9*iC;
	       iB=iA/3;
	       iA-=3*iB;
	       printf("Best time per iteration %g %s=%g %s=%g %s=%g %s=%g %s=%g\n",bestTimePer,choiceA,useA[iA],
		      choiceB,useB[iB],choiceC,useC[iC],choiceD,useD[iD],choiceE,useE[iE]);
	       iA=iBestIterations;
	       iE=iA/81;
	       iA-=81*iE;
	       iD=iA/27;
	       iA-=27*iD;
	       iC=iA/9;
	       iA-=9*iC;
	       iB=iA/3;
	       iA-=3*iB;
	       printf("Best iterations %d %s=%g %s=%g %s=%g %s=%g %s=%g\n",bestIterations,choiceA,useA[iA],
		      choiceB,useB[iB],choiceC,useC[iC],choiceD,useD[iD],choiceE,useE[iE]);
#else
               solution.initialSolve(solveOptions);
               double time2 = CoinCpuTime() - time1;
               timeTaken += time2;
               printf("%s took %g seconds using algorithm %s\n", fn.c_str(), time2, nameAlgorithm.c_str());
#endif
               // Test objective solution value
               {
		    if (!solution.isProvenOptimal()) {
		      std::cerr <<"** NOT OPTIMAL ";
		      numberFailures++;
		    }
                    double soln = solution.objectiveValue();
                    CoinRelFltEq eq(objValueTol[m]);
                    std::cerr << soln << ",  " << objValue[m] << " diff " <<
                              soln - objValue[m] << std::endl;
                    if(!eq(soln, objValue[m])) {
                         printf("** difference fails\n");
			 numberFailures++;
		    }
               }
          }
          printf("Total time %g seconds\n", timeTaken);
#if FACTORIZATION_STATISTICS
	  double bestTime=1.0e100;
	  int iBestTime=-1;
	  for (int i=0;i<3*3*3*3*3;i++) {
	    if (times[i]&&times[i]<bestTime) {
	      bestTime=times[i];
	      iBestTime=i;
	    }
	  }
	  for (int iA=0;iA<hiA;iA++) {
	    for (int iB=0;iB<hiB;iB++) {
	      for (int iC=0;iC<hiC;iC++) {
		for (int iD=loD;iD<hiD;iD++) {
		  for (int iE=loE;iE<hiE;iE++) {
		    int k=iA+3*iB+9*iC+27*iD+81*iE;
		    double thisTime=times[k];
		    if (thisTime) {
		      if (k==iBestTime)
			printf("** ");
		      printf("%s=%g %s=%g %s=%g %s=%g %s=%g - time %.2f\n",
			     choiceA,useA[iA],
			     choiceB,useB[iB],choiceC,useC[iC],
			     choiceD,useD[iD],choiceE,useE[iE],thisTime);
		    }
		  }
		}
	      }
	    }
	  }
	  //exit(0);
#endif
     } else {
          testingMessage( "***Skipped Testing on netlib    ***\n" );
          testingMessage( "***use -netlib to test class***\n" );
     }
     if (!numberFailures)
       testingMessage( "All tests completed successfully\n" );
     else
       testingMessage( "Some tests failed\n" );
     return 0;
}


// Display message on stdout and stderr
void testingMessage( const char * const msg )
{
     std::cerr << msg;
     //cout <<endl <<"*****************************************"
     //     <<endl <<msg <<endl;
}

//--------------------------------------------------------------------------
// test factorization methods and simplex method and simple barrier
void
ClpSimplexUnitTest(const std::string & dirSample)
{

     CoinRelFltEq eq(0.000001);

     {
          ClpSimplex solution;

          // matrix data
          //deliberate hiccup of 2 between 0 and 1
          CoinBigIndex start[5] = {0, 4, 7, 8, 9};
          int length[5] = {2, 3, 1, 1, 1};
          int rows[11] = {0, 2, -1, -1, 0, 1, 2, 0, 1, 2};
          double elements[11] = {7.0, 2.0, 1.0e10, 1.0e10, -2.0, 1.0, -2.0, 1, 1, 1};
          CoinPackedMatrix matrix(true, 3, 5, 8, elements, rows, start, length);

          // rim data
          double objective[7] = { -4.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0};
          double rowLower[5] = {14.0, 3.0, 3.0, 1.0e10, 1.0e10};
          double rowUpper[5] = {14.0, 3.0, 3.0, -1.0e10, -1.0e10};
          double colLower[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
          double colUpper[7] = {100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0};

          // basis 1
          int rowBasis1[3] = { -1, -1, -1};
          int colBasis1[5] = {1, 1, -1, -1, 1};
          solution.loadProblem(matrix, colLower, colUpper, objective,
                               rowLower, rowUpper);
          int i;
          solution.createStatus();
          for (i = 0; i < 3; i++) {
               if (rowBasis1[i] < 0) {
                    solution.setRowStatus(i, ClpSimplex::atLowerBound);
               } else {
                    solution.setRowStatus(i, ClpSimplex::basic);
               }
          }
          for (i = 0; i < 5; i++) {
               if (colBasis1[i] < 0) {
                    solution.setColumnStatus(i, ClpSimplex::atLowerBound);
               } else {
                    solution.setColumnStatus(i, ClpSimplex::basic);
               }
          }
          solution.setLogLevel(3 + 4 + 8 + 16 + 32);
          solution.primal();
          for (i = 0; i < 3; i++) {
               if (rowBasis1[i] < 0) {
                    solution.setRowStatus(i, ClpSimplex::atLowerBound);
               } else {
                    solution.setRowStatus(i, ClpSimplex::basic);
               }
          }
          for (i = 0; i < 5; i++) {
               if (colBasis1[i] < 0) {
                    solution.setColumnStatus(i, ClpSimplex::atLowerBound);
               } else {
                    solution.setColumnStatus(i, ClpSimplex::basic);
               }
          }
          // intricate stuff does not work with scaling
          solution.scaling(0);
#ifndef NDEBUG
          int returnCode = solution.factorize ( );
          assert(!returnCode);
#else
          solution.factorize ( );
#endif
          const double * colsol = solution.primalColumnSolution();
          const double * rowsol = solution.primalRowSolution();
          solution.getSolution(rowsol, colsol);
#ifndef NDEBUG
          double colsol1[5] = {20.0 / 7.0, 3.0, 0.0, 0.0, 23.0 / 7.0};
          for (i = 0; i < 5; i++) {
               assert(eq(colsol[i], colsol1[i]));
          }
          // now feed in again without actually doing factorization
#endif
          ClpFactorization factorization2 = *solution.factorization();
          ClpSimplex solution2 = solution;
          solution2.setFactorization(factorization2);
          solution2.createStatus();
          for (i = 0; i < 3; i++) {
               if (rowBasis1[i] < 0) {
                    solution2.setRowStatus(i, ClpSimplex::atLowerBound);
               } else {
                    solution2.setRowStatus(i, ClpSimplex::basic);
               }
          }
          for (i = 0; i < 5; i++) {
               if (colBasis1[i] < 0) {
                    solution2.setColumnStatus(i, ClpSimplex::atLowerBound);
               } else {
                    solution2.setColumnStatus(i, ClpSimplex::basic);
               }
          }
          // intricate stuff does not work with scaling
          solution2.scaling(0);
          solution2.getSolution(rowsol, colsol);
          colsol = solution2.primalColumnSolution();
          rowsol = solution2.primalRowSolution();
          for (i = 0; i < 5; i++) {
               assert(eq(colsol[i], colsol1[i]));
          }
          solution2.setDualBound(0.1);
          solution2.dual();
          objective[2] = -1.0;
          objective[3] = -0.5;
          objective[4] = 10.0;
          solution.dual();
          for (i = 0; i < 3; i++) {
               rowLower[i] = -1.0e20;
               colUpper[i+2] = 0.0;
          }
          solution.setLogLevel(3);
          solution.dual();
          double rowObjective[] = {1.0, 0.5, -10.0};
          solution.loadProblem(matrix, colLower, colUpper, objective,
                               rowLower, rowUpper, rowObjective);
          solution.dual();
          solution.loadProblem(matrix, colLower, colUpper, objective,
                               rowLower, rowUpper, rowObjective);
          solution.primal();
     }
#ifndef COIN_NO_CLP_MESSAGE
     {
          CoinMpsIO m;
          std::string fn = dirSample + "exmip1";
          if (m.readMps(fn.c_str(), "mps") == 0) {
               ClpSimplex solution;
               solution.loadProblem(*m.getMatrixByCol(), m.getColLower(), m.getColUpper(),
                                    m.getObjCoefficients(),
                                    m.getRowLower(), m.getRowUpper());
               solution.dual();
               // Test event handling
               MyEventHandler handler;
               solution.passInEventHandler(&handler);
               int numberRows = solution.numberRows();
               // make sure values pass has something to do
               for (int i = 0; i < numberRows; i++)
                    solution.setRowStatus(i, ClpSimplex::basic);
               solution.primal(1);
               assert (solution.secondaryStatus() == 102); // Came out at end of pass
          } else {
               std::cerr << "Error reading exmip1 from sample data. Skipping test." << std::endl;
          }
     }
     // Test Message handler
     {
          CoinMpsIO m;
          std::string fn = dirSample + "exmip1";
          //fn = "Test/subGams4";
          if (m.readMps(fn.c_str(), "mps") == 0) {
               ClpSimplex model;
               model.loadProblem(*m.getMatrixByCol(), m.getColLower(), m.getColUpper(),
                                 m.getObjCoefficients(),
                                 m.getRowLower(), m.getRowUpper());
               // Message handler
               MyMessageHandler messageHandler(&model);
               std::cout << "Testing derived message handler" << std::endl;
               model.passInMessageHandler(&messageHandler);
               model.messagesPointer()->setDetailMessage(1, 102);
               model.setFactorizationFrequency(10);
               model.primal();
               model.primal(0, 3);
               model.setObjCoeff(3, -2.9473684210526314);
               model.primal(0, 3);
               // Write saved solutions
               int nc = model.getNumCols();
               size_t s;
               std::deque<StdVectorDouble> fep = messageHandler.getFeasibleExtremePoints();
               size_t numSavedSolutions = fep.size();
               for ( s = 0; s < numSavedSolutions; ++s ) {
                    const StdVectorDouble & solnVec = fep[s];
                    for ( int c = 0; c < nc; ++c ) {
                         if (fabs(solnVec[c]) > 1.0e-8)
                              std::cout << "Saved Solution: " << s << " ColNum: " << c << " Value: " << solnVec[c] << std::endl;
                    }
               }
               // Solve again without scaling
               // and maximize then minimize
               messageHandler.clearFeasibleExtremePoints();
               model.scaling(0);
               model.setOptimizationDirection(-1);
               model.primal();
               model.setOptimizationDirection(1);
               model.primal();
               fep = messageHandler.getFeasibleExtremePoints();
               numSavedSolutions = fep.size();
               for ( s = 0; s < numSavedSolutions; ++s ) {
                    const StdVectorDouble & solnVec = fep[s];
                    for ( int c = 0; c < nc; ++c ) {
                         if (fabs(solnVec[c]) > 1.0e-8)
                              std::cout << "Saved Solution: " << s << " ColNum: " << c << " Value: " << solnVec[c] << std::endl;
                    }
               }
          } else {
               std::cerr << "Error reading exmip1 from sample data. Skipping test." << std::endl;
          }
     }
#endif
     // Test dual ranging
     {
          CoinMpsIO m;
          std::string fn = dirSample + "exmip1";
          if (m.readMps(fn.c_str(), "mps") == 0) {
               ClpSimplex model;
               model.loadProblem(*m.getMatrixByCol(), m.getColLower(), m.getColUpper(),
                                 m.getObjCoefficients(),
                                 m.getRowLower(), m.getRowUpper());
               model.primal();
               int which[13] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
               double costIncrease[13];
               int sequenceIncrease[13];
               double costDecrease[13];
               int sequenceDecrease[13];
               // ranging
               model.dualRanging(13, which, costIncrease, sequenceIncrease,
                                 costDecrease, sequenceDecrease);
               int i;
               for ( i = 0; i < 13; i++)
                    printf("%d increase %g %d, decrease %g %d\n",
                           i, costIncrease[i], sequenceIncrease[i],
                           costDecrease[i], sequenceDecrease[i]);
               assert (fabs(costDecrease[3]) < 1.0e-4);
               assert (fabs(costIncrease[7] - 1.0) < 1.0e-4);
               model.setOptimizationDirection(-1);
               {
                    int j;
                    double * obj = model.objective();
                    int n = model.numberColumns();
                    for (j = 0; j < n; j++)
                         obj[j] *= -1.0;
               }
               double costIncrease2[13];
               int sequenceIncrease2[13];
               double costDecrease2[13];
               int sequenceDecrease2[13];
               // ranging
               model.dualRanging(13, which, costIncrease2, sequenceIncrease2,
                                 costDecrease2, sequenceDecrease2);
               for (i = 0; i < 13; i++) {
                    assert (fabs(costIncrease[i] - costDecrease2[i]) < 1.0e-6);
                    assert (fabs(costDecrease[i] - costIncrease2[i]) < 1.0e-6);
                    assert (sequenceIncrease[i] == sequenceDecrease2[i]);
                    assert (sequenceDecrease[i] == sequenceIncrease2[i]);
               }
               // Now delete all rows and see what happens
               model.deleteRows(model.numberRows(), which);
               model.primal();
               // ranging
               if (!model.dualRanging(8, which, costIncrease, sequenceIncrease,
                                      costDecrease, sequenceDecrease)) {
                    for (i = 0; i < 8; i++) {
                         printf("%d increase %g %d, decrease %g %d\n",
                                i, costIncrease[i], sequenceIncrease[i],
                                costDecrease[i], sequenceDecrease[i]);
                    }
               }
          } else {
               std::cerr << "Error reading exmip1 from sample data. Skipping test." << std::endl;
          }
     }
     // Test primal ranging
     {
          CoinMpsIO m;
          std::string fn = dirSample + "exmip1";
          if (m.readMps(fn.c_str(), "mps") == 0) {
               ClpSimplex model;
               model.loadProblem(*m.getMatrixByCol(), m.getColLower(), m.getColUpper(),
                                 m.getObjCoefficients(),
                                 m.getRowLower(), m.getRowUpper());
               model.primal();
               int which[13] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
               double valueIncrease[13];
               int sequenceIncrease[13];
               double valueDecrease[13];
               int sequenceDecrease[13];
               // ranging
               model.primalRanging(13, which, valueIncrease, sequenceIncrease,
                                   valueDecrease, sequenceDecrease);
               int i;
               for ( i = 0; i < 13; i++)
                    printf("%d increase %g %d, decrease %g %d\n",
                           i, valueIncrease[i], sequenceIncrease[i],
                           valueDecrease[i], sequenceDecrease[i]);
               assert (fabs(valueIncrease[3] - 0.642857) < 1.0e-4);
               assert (fabs(valueIncrease[8] - 2.95113) < 1.0e-4);
          } else {
               std::cerr << "Error reading exmip1 from sample data. Skipping test." << std::endl;
          }
#if 0
          // out until I find optimization bug
          // Test parametrics
          ClpSimplexOther * model2 = (ClpSimplexOther *) (&model);
          double rhs[] = { 1.0, 2.0, 3.0, 4.0, 5.0};
          double endingTheta = 1.0;
          model2->scaling(0);
          model2->setLogLevel(63);
          model2->parametrics(0.0, endingTheta, 0.1,
                              NULL, NULL, rhs, rhs, NULL);
#endif
     }
     // Test binv etc
     {
          /*
             Wolsey : Page 130
             max 4x1 -  x2
             7x1 - 2x2    <= 14
             x2    <= 3
             2x1 - 2x2    <= 3
             x1 in Z+, x2 >= 0

             note slacks are -1 in Clp so signs may be different
          */

          int n_cols = 2;
          int n_rows = 3;

          double obj[2] = { -4.0, 1.0};
          double collb[2] = {0.0, 0.0};
          double colub[2] = {COIN_DBL_MAX, COIN_DBL_MAX};
          double rowlb[3] = { -COIN_DBL_MAX, -COIN_DBL_MAX, -COIN_DBL_MAX};
          double rowub[3] = {14.0, 3.0, 3.0};

          int rowIndices[5] =  {0,     2,    0,    1,    2};
          int colIndices[5] =  {0,     0,    1,    1,    1};
          double elements[5] = {7.0, 2.0, -2.0,  1.0, -2.0};
          CoinPackedMatrix M(true, rowIndices, colIndices, elements, 5);

          ClpSimplex model;
          model.loadProblem(M, collb, colub, obj, rowlb, rowub);
          model.dual(0, 1); // keep factorization

          //check that the tableau matches wolsey (B-1 A)
          // slacks in second part of binvA
          double * binvA = reinterpret_cast<double*> (malloc((n_cols + n_rows) * sizeof(double)));

          printf("B-1 A by row\n");
          int i;
          for( i = 0; i < n_rows; i++) {
               model.getBInvARow(i, binvA, binvA + n_cols);
               printf("row: %d -> ", i);
               for(int j = 0; j < n_cols + n_rows; j++) {
                    printf("%g, ", binvA[j]);
               }
               printf("\n");
          }
          // See if can re-use factorization AND arrays
          model.primal(0, 3 + 4); // keep factorization
          // And do by column
          printf("B-1 A by column\n");
          for( i = 0; i < n_rows + n_cols; i++) {
               model.getBInvACol(i, binvA);
               printf("column: %d -> ", i);
               for(int j = 0; j < n_rows; j++) {
                    printf("%g, ", binvA[j]);
               }
               printf("\n");
          }
          /* Do twice -
             without and with scaling
          */
          // set scaling off
          model.scaling(0);
          for (int iPass = 0; iPass < 2; iPass++) {
               model.primal(0, 3 + 4); // keep factorization
               const double * rowScale = model.rowScale();
               const double * columnScale = model.columnScale();
               if (!iPass)
                    assert (!rowScale);
               else
                    assert (rowScale); // only true for this example
               /* has to be exactly correct as in OsiClpsolverInterface.cpp
                  (also redo each pass as may change
               */
               printf("B-1 A");
               for( i = 0; i < n_rows; i++) {
                    model.getBInvARow(i, binvA, binvA + n_cols);
                    printf("\nrow: %d -> ", i);
                    int j;
                    // First columns
                    for(j = 0; j < n_cols; j++) {
                         if (binvA[j]) {
                              printf("(%d %g), ", j, binvA[j]);
                         }
                    }
                    // now rows
                    for(j = 0; j < n_rows; j++) {
                         if (binvA[j+n_cols]) {
                              printf("(%d %g), ", j + n_cols, binvA[j+n_cols]);
                         }
                    }
               }
               printf("\n");
               printf("And by column (trickier)");
               const int * pivotVariable = model.pivotVariable();
               for( i = 0; i < n_cols + n_rows; i++) {
                    model.getBInvACol(i, binvA);
                    printf("\ncolumn: %d -> ", i);
                    for(int j = 0; j < n_rows; j++) {
                         if (binvA[j]) {
                              // need to know pivot variable for +1/-1 (slack) and row/column scaling
                              int pivot = pivotVariable[j];
                              if (pivot < n_cols) {
                                   // scaled coding is in just in case
                                   if (!columnScale) {
                                        printf("(%d %g), ", j, binvA[j]);
                                   } else {
                                        printf("(%d %g), ", j, binvA[j]*columnScale[pivot]);
                                   }
                              } else {
                                   if (!rowScale) {
                                        printf("(%d %g), ", j, binvA[j]);
                                   } else {
                                        printf("(%d %g), ", j, binvA[j] / rowScale[pivot-n_cols]);
                                   }
                              }
                         }
                    }
               }
               printf("\n");
               printf("binvrow");
               for( i = 0; i < n_rows; i++) {
                    model.getBInvRow(i, binvA);
                    printf("\nrow: %d -> ", i);
                    int j;
                    for (j = 0; j < n_rows; j++) {
                         if (binvA[j])
                              printf("(%d %g), ", j, binvA[j]);
                    }
               }
               printf("\n");
               printf("And by column ");
               for( i = 0; i < n_rows; i++) {
                    model.getBInvCol(i, binvA);
                    printf("\ncol: %d -> ", i);
                    int j;
                    for (j = 0; j < n_rows; j++) {
                         if (binvA[j])
                              printf("(%d %g), ", j, binvA[j]);
                    }
               }
               printf("\n");
               // now deal with next pass
               if (!iPass) {
                    // get scaling for testing
                    model.scaling(1);
               }
          }
          free(binvA);
          model.setColUpper(1, 2.0);
          model.dual(0, 2 + 4); // use factorization and arrays
          model.dual(0, 2); // hopefully will not use factorization
          model.primal(0, 3 + 4); // keep factorization
          // but say basis has changed
          model.setWhatsChanged(model.whatsChanged()&(~512));
          model.dual(0, 2); // hopefully will not use factorization
     }
     // test steepest edge
     {
          CoinMpsIO m;
          std::string fn = dirSample + "finnis";
          int returnCode = m.readMps(fn.c_str(), "mps");
          if (returnCode) {
               // probable cause is that gz not there
               fprintf(stderr, "Unable to open finnis.mps in %s\n", dirSample.c_str());
               fprintf(stderr, "Most probable cause is that sample data is not available, or finnis.mps is gzipped i.e. finnis.mps.gz and libz has not been activated\n");
               fprintf(stderr, "Either gunzip files or edit Makefiles/Makefile.location to get libz\n");
          } else {
               ClpModel model;
               model.loadProblem(*m.getMatrixByCol(), m.getColLower(),
                                 m.getColUpper(),
                                 m.getObjCoefficients(),
                                 m.getRowLower(), m.getRowUpper());
               ClpSimplex solution(model);

               solution.scaling(1);
               solution.setDualBound(1.0e8);
               //solution.factorization()->maximumPivots(1);
               //solution.setLogLevel(3);
               solution.setDualTolerance(1.0e-7);
               // set objective sense,
               ClpDualRowSteepest steep;
               solution.setDualRowPivotAlgorithm(steep);
               solution.setDblParam(ClpObjOffset, m.objectiveOffset());
               solution.dual();
          }
     }
     // test normal solution
     {
          CoinMpsIO m;
          std::string fn = dirSample + "afiro";
          if (m.readMps(fn.c_str(), "mps") == 0) {
               ClpSimplex solution;
               ClpModel model;
               // do twice - without and with scaling
               int iPass;
               for (iPass = 0; iPass < 2; iPass++) {
                    // explicit row objective for testing
                    int nr = m.getNumRows();
                    double * rowObj = new double[nr];
                    CoinFillN(rowObj, nr, 0.0);
                    model.loadProblem(*m.getMatrixByCol(), m.getColLower(), m.getColUpper(),
                                      m.getObjCoefficients(),
                                      m.getRowLower(), m.getRowUpper(), rowObj);
                    delete [] rowObj;
                    solution = ClpSimplex(model);
                    if (iPass) {
                         solution.scaling();
                    }
                    solution.dual();
                    solution.dual();
                    // test optimal
                    assert (solution.status() == 0);
                    int numberColumns = solution.numberColumns();
                    int numberRows = solution.numberRows();
                    CoinPackedVector colsol(numberColumns, solution.primalColumnSolution());
                    double * objective = solution.objective();
#ifndef NDEBUG
                    double objValue = colsol.dotProduct(objective);
#endif
                    CoinRelFltEq eq(1.0e-8);
                    assert(eq(objValue, -4.6475314286e+02));
                    solution.dual();
                    assert(eq(solution.objectiveValue(), -4.6475314286e+02));
                    double * lower = solution.columnLower();
                    double * upper = solution.columnUpper();
                    double * sol = solution.primalColumnSolution();
                    double * result = new double[numberColumns];
                    CoinFillN ( result, numberColumns, 0.0);
                    solution.matrix()->transposeTimes(solution.dualRowSolution(), result);
                    int iRow , iColumn;
                    // see if feasible and dual feasible
                    for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                         double value = sol[iColumn];
                         assert(value < upper[iColumn] + 1.0e-8);
                         assert(value > lower[iColumn] - 1.0e-8);
                         value = objective[iColumn] - result[iColumn];
                         assert (value > -1.0e-5);
                         if (sol[iColumn] > 1.0e-5)
                              assert (value < 1.0e-5);
                    }
                    delete [] result;
                    result = new double[numberRows];
                    CoinFillN ( result, numberRows, 0.0);
                    solution.matrix()->times(colsol, result);
                    lower = solution.rowLower();
                    upper = solution.rowUpper();
                    sol = solution.primalRowSolution();
#ifndef NDEBUG
                    for (iRow = 0; iRow < numberRows; iRow++) {
                         double value = result[iRow];
                         assert(eq(value, sol[iRow]));
                         assert(value < upper[iRow] + 1.0e-8);
                         assert(value > lower[iRow] - 1.0e-8);
                    }
#endif
                    delete [] result;
                    // test row objective
                    double * rowObjective = solution.rowObjective();
                    CoinDisjointCopyN(solution.dualRowSolution(), numberRows, rowObjective);
                    CoinDisjointCopyN(solution.dualColumnSolution(), numberColumns, objective);
                    // this sets up all slack basis
                    solution.createStatus();
                    solution.dual();
                    CoinFillN(rowObjective, numberRows, 0.0);
                    CoinDisjointCopyN(m.getObjCoefficients(), numberColumns, objective);
                    solution.dual();
               }
          } else {
               std::cerr << "Error reading afiro from sample data. Skipping test." << std::endl;
          }
     }
     // test unbounded
     {
          CoinMpsIO m;
          std::string fn = dirSample + "brandy";
          if (m.readMps(fn.c_str(), "mps") == 0) {
               ClpSimplex solution;
               // do twice - without and with scaling
               int iPass;
               for (iPass = 0; iPass < 2; iPass++) {
                    solution.loadProblem(*m.getMatrixByCol(), m.getColLower(), m.getColUpper(),
                                         m.getObjCoefficients(),
                                         m.getRowLower(), m.getRowUpper());
                    if (iPass)
                         solution.scaling();
                    solution.setOptimizationDirection(-1);
                    // test unbounded and ray
#ifdef DUAL
                    solution.setDualBound(100.0);
                    solution.dual();
#else
                    solution.primal();
#endif
                    assert (solution.status() == 2);
                    int numberColumns = solution.numberColumns();
                    int numberRows = solution.numberRows();
                    double * lower = solution.columnLower();
                    double * upper = solution.columnUpper();
                    double * sol = solution.primalColumnSolution();
                    double * ray = solution.unboundedRay();
                    double * objective = solution.objective();
                    double objChange = 0.0;
                    int iRow , iColumn;
                    // make sure feasible and columns form ray
                    for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                         double value = sol[iColumn];
                         assert(value < upper[iColumn] + 1.0e-8);
                         assert(value > lower[iColumn] - 1.0e-8);
                         value = ray[iColumn];
                         if (value > 0.0)
                              assert(upper[iColumn] > 1.0e30);
                         else if (value < 0.0)
                              assert(lower[iColumn] < -1.0e30);
                         objChange += value * objective[iColumn];
                    }
                    // make sure increasing objective
                    assert(objChange > 0.0);
                    double * result = new double[numberRows];
                    CoinFillN ( result, numberRows, 0.0);
                    solution.matrix()->times(sol, result);
                    lower = solution.rowLower();
                    upper = solution.rowUpper();
                    sol = solution.primalRowSolution();
#ifndef NDEBUG
                    for (iRow = 0; iRow < numberRows; iRow++) {
                         double value = result[iRow];
                         assert(eq(value, sol[iRow]));
                         assert(value < upper[iRow] + 2.0e-8);
                         assert(value > lower[iRow] - 2.0e-8);
                    }
#endif
                    CoinFillN ( result, numberRows, 0.0);
                    solution.matrix()->times(ray, result);
                    // there may be small differences (especially if scaled)
                    for (iRow = 0; iRow < numberRows; iRow++) {
                         double value = result[iRow];
                         if (value > 1.0e-8)
                              assert(upper[iRow] > 1.0e30);
                         else if (value < -1.0e-8)
                              assert(lower[iRow] < -1.0e30);
                    }
                    delete [] result;
                    delete [] ray;
               }
          } else {
               std::cerr << "Error reading brandy from sample data. Skipping test." << std::endl;
          }
     }
     // test infeasible
     {
          CoinMpsIO m;
          std::string fn = dirSample + "brandy";
          if (m.readMps(fn.c_str(), "mps") == 0) {
               ClpSimplex solution;
               // do twice - without and with scaling
               int iPass;
               for (iPass = 0; iPass < 2; iPass++) {
                    solution.loadProblem(*m.getMatrixByCol(), m.getColLower(), m.getColUpper(),
                                         m.getObjCoefficients(),
                                         m.getRowLower(), m.getRowUpper());
                    if (iPass)
                         solution.scaling();
                    // test infeasible and ray
                    solution.columnUpper()[0] = 0.0;
#ifdef DUAL
                    solution.setDualBound(100.0);
                    solution.dual();
#else
                    solution.primal();
#endif
                    assert (solution.status() == 1);
                    int numberColumns = solution.numberColumns();
                    int numberRows = solution.numberRows();
                    double * lower = solution.rowLower();
                    double * upper = solution.rowUpper();
                    double * ray = solution.infeasibilityRay();
                    assert(ray);
                    // construct proof of infeasibility
                    int iRow , iColumn;
                    double lo = 0.0, up = 0.0;
                    int nl = 0, nu = 0;
                    for (iRow = 0; iRow < numberRows; iRow++) {
                         if (lower[iRow] > -1.0e20) {
                              lo += ray[iRow] * lower[iRow];
                         } else {
                              if (ray[iRow] > 1.0e-8)
                                   nl++;
                         }
                         if (upper[iRow] < 1.0e20) {
                              up += ray[iRow] * upper[iRow];
                         } else {
                              if (ray[iRow] > 1.0e-8)
                                   nu++;
                         }
                    }
                    if (nl)
                         lo = -1.0e100;
                    if (nu)
                         up = 1.0e100;
                    double * result = new double[numberColumns];
                    double lo2 = 0.0, up2 = 0.0;
                    CoinFillN ( result, numberColumns, 0.0);
                    solution.matrix()->transposeTimes(ray, result);
                    lower = solution.columnLower();
                    upper = solution.columnUpper();
                    nl = nu = 0;
                    for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                         if (result[iColumn] > 1.0e-8) {
                              if (lower[iColumn] > -1.0e20)
                                   lo2 += result[iColumn] * lower[iColumn];
                              else
                                   nl++;
                              if (upper[iColumn] < 1.0e20)
                                   up2 += result[iColumn] * upper[iColumn];
                              else
                                   nu++;
                         } else if (result[iColumn] < -1.0e-8) {
                              if (lower[iColumn] > -1.0e20)
                                   up2 += result[iColumn] * lower[iColumn];
                              else
                                   nu++;
                              if (upper[iColumn] < 1.0e20)
                                   lo2 += result[iColumn] * upper[iColumn];
                              else
                                   nl++;
                         }
                    }
                    if (nl)
                         lo2 = -1.0e100;
                    if (nu)
                         up2 = 1.0e100;
                    // make sure inconsistency
                    assert(lo2 > up || up2 < lo);
                    delete [] result;
                    delete [] ray;
               }
          } else {
               std::cerr << "Error reading brandy from sample data. Skipping test." << std::endl;
          }
     }
     // test delete and add
     {
          CoinMpsIO m;
          std::string fn = dirSample + "brandy";
          if (m.readMps(fn.c_str(), "mps") == 0) {
               ClpSimplex solution;
               solution.loadProblem(*m.getMatrixByCol(), m.getColLower(), m.getColUpper(),
                                    m.getObjCoefficients(),
                                    m.getRowLower(), m.getRowUpper());
               solution.dual();
               CoinRelFltEq eq(1.0e-8);
               assert(eq(solution.objectiveValue(), 1.5185098965e+03));

               int numberColumns = solution.numberColumns();
               int numberRows = solution.numberRows();
               double * saveObj = new double [numberColumns];
               double * saveLower = new double[numberRows+numberColumns];
               double * saveUpper = new double[numberRows+numberColumns];
               int * which = new int [numberRows+numberColumns];

               int numberElements = m.getMatrixByCol()->getNumElements();
               int * starts = new int[numberRows+numberColumns];
               int * index = new int[numberElements];
               double * element = new double[numberElements];

               const CoinBigIndex * startM;
               const int * lengthM;
               const int * indexM;
               const double * elementM;

               int n, nel;

               // delete non basic columns
               n = 0;
               nel = 0;
               int iRow , iColumn;
               const double * lower = m.getColLower();
               const double * upper = m.getColUpper();
               const double * objective = m.getObjCoefficients();
               startM = m.getMatrixByCol()->getVectorStarts();
               lengthM = m.getMatrixByCol()->getVectorLengths();
               indexM = m.getMatrixByCol()->getIndices();
               elementM = m.getMatrixByCol()->getElements();
               starts[0] = 0;
               for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                    if (solution.getColumnStatus(iColumn) != ClpSimplex::basic) {
                         saveObj[n] = objective[iColumn];
                         saveLower[n] = lower[iColumn];
                         saveUpper[n] = upper[iColumn];
                         int j;
                         for (j = startM[iColumn]; j < startM[iColumn] + lengthM[iColumn]; j++) {
                              index[nel] = indexM[j];
                              element[nel++] = elementM[j];
                         }
                         which[n++] = iColumn;
                         starts[n] = nel;
                    }
               }
               solution.deleteColumns(n, which);
               solution.dual();
               // Put back
               solution.addColumns(n, saveLower, saveUpper, saveObj,
                                   starts, index, element);
               solution.dual();
               assert(eq(solution.objectiveValue(), 1.5185098965e+03));
               // Delete all columns and add back
               n = 0;
               nel = 0;
               starts[0] = 0;
               lower = m.getColLower();
               upper = m.getColUpper();
               objective = m.getObjCoefficients();
               for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                    saveObj[n] = objective[iColumn];
                    saveLower[n] = lower[iColumn];
                    saveUpper[n] = upper[iColumn];
                    int j;
                    for (j = startM[iColumn]; j < startM[iColumn] + lengthM[iColumn]; j++) {
                         index[nel] = indexM[j];
                         element[nel++] = elementM[j];
                    }
                    which[n++] = iColumn;
                    starts[n] = nel;
               }
               solution.deleteColumns(n, which);
               solution.dual();
               // Put back
               solution.addColumns(n, saveLower, saveUpper, saveObj,
                                   starts, index, element);
               solution.dual();
               assert(eq(solution.objectiveValue(), 1.5185098965e+03));

               // reload with original
               solution.loadProblem(*m.getMatrixByCol(), m.getColLower(), m.getColUpper(),
                                    m.getObjCoefficients(),
                                    m.getRowLower(), m.getRowUpper());
               // delete half rows
               n = 0;
               nel = 0;
               lower = m.getRowLower();
               upper = m.getRowUpper();
               startM = m.getMatrixByRow()->getVectorStarts();
               lengthM = m.getMatrixByRow()->getVectorLengths();
               indexM = m.getMatrixByRow()->getIndices();
               elementM = m.getMatrixByRow()->getElements();
               starts[0] = 0;
               for (iRow = 0; iRow < numberRows; iRow++) {
                    if ((iRow & 1) == 0) {
                         saveLower[n] = lower[iRow];
                         saveUpper[n] = upper[iRow];
                         int j;
                         for (j = startM[iRow]; j < startM[iRow] + lengthM[iRow]; j++) {
                              index[nel] = indexM[j];
                              element[nel++] = elementM[j];
                         }
                         which[n++] = iRow;
                         starts[n] = nel;
                    }
               }
               solution.deleteRows(n, which);
               solution.dual();
               // Put back
               solution.addRows(n, saveLower, saveUpper,
                                starts, index, element);
               solution.dual();
               assert(eq(solution.objectiveValue(), 1.5185098965e+03));
               solution.writeMps("yy.mps");
               // Delete all rows
               n = 0;
               nel = 0;
               lower = m.getRowLower();
               upper = m.getRowUpper();
               starts[0] = 0;
               for (iRow = 0; iRow < numberRows; iRow++) {
                    saveLower[n] = lower[iRow];
                    saveUpper[n] = upper[iRow];
                    int j;
                    for (j = startM[iRow]; j < startM[iRow] + lengthM[iRow]; j++) {
                         index[nel] = indexM[j];
                         element[nel++] = elementM[j];
                    }
                    which[n++] = iRow;
                    starts[n] = nel;
               }
               solution.deleteRows(n, which);
               solution.dual();
               // Put back
               solution.addRows(n, saveLower, saveUpper,
                                starts, index, element);
               solution.dual();
               solution.writeMps("xx.mps");
               assert(eq(solution.objectiveValue(), 1.5185098965e+03));
               // Zero out status array to give some interest
               memset(solution.statusArray() + numberColumns, 0, numberRows);
               solution.primal(1);
               assert(eq(solution.objectiveValue(), 1.5185098965e+03));
               // Delete all columns and rows
               n = 0;
               for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                    which[n++] = iColumn;
                    starts[n] = nel;
               }
               solution.deleteColumns(n, which);
               n = 0;
               for (iRow = 0; iRow < numberRows; iRow++) {
                    which[n++] = iRow;
                    starts[n] = nel;
               }
               solution.deleteRows(n, which);

               delete [] saveObj;
               delete [] saveLower;
               delete [] saveUpper;
               delete [] which;
               delete [] starts;
               delete [] index;
               delete [] element;
          } else {
               std::cerr << "Error reading brandy from sample data. Skipping test." << std::endl;
          }
     }
#if 1
     // Test barrier
     {
          CoinMpsIO m;
          std::string fn = dirSample + "exmip1";
          if (m.readMps(fn.c_str(), "mps") == 0) {
               ClpInterior solution;
               solution.loadProblem(*m.getMatrixByCol(), m.getColLower(), m.getColUpper(),
                                    m.getObjCoefficients(),
                                    m.getRowLower(), m.getRowUpper());
               solution.primalDual();
          } else {
               std::cerr << "Error reading exmip1 from sample data. Skipping test." << std::endl;
          }
     }
#endif
     // test network
#define QUADRATIC
     if (1) {
          std::string fn = dirSample + "input.130";
          int numberColumns;
          int numberRows;

          FILE * fp = fopen(fn.c_str(), "r");
          if (!fp) {
               // Try in Data/Sample
               fn = "Data/Sample/input.130";
               fp = fopen(fn.c_str(), "r");
          }
          if (!fp) {
               fprintf(stderr, "Unable to open file input.130 in dirSample or Data/Sample directory\n");
          } else {
               int problem;
               char temp[100];
               // read and skip
               int x = fscanf(fp, "%s", temp);
               if (x < 0)
                    throw("bad fscanf");
               assert (!strcmp(temp, "BEGIN"));
               x = fscanf(fp, "%*s %*s %d %d %*s %*s %d %*s", &problem, &numberRows,
                          &numberColumns);
               if (x < 0)
                    throw("bad fscanf");
               // scan down to SUPPLY
               while (fgets(temp, 100, fp)) {
                    if (!strncmp(temp, "SUPPLY", 6))
                         break;
               }
               if (strncmp(temp, "SUPPLY", 6)) {
                    fprintf(stderr, "Unable to find SUPPLY\n");
                    exit(2);
               }
               // get space for rhs
               double * lower = new double[numberRows];
               double * upper = new double[numberRows];
               int i;
               for (i = 0; i < numberRows; i++) {
                    lower[i] = 0.0;
                    upper[i] = 0.0;
               }
               // ***** Remember to convert to C notation
               while (fgets(temp, 100, fp)) {
                    int row;
                    int value;
                    if (!strncmp(temp, "ARCS", 4))
                         break;
                    sscanf(temp, "%d %d", &row, &value);
                    upper[row-1] = -value;
                    lower[row-1] = -value;
               }
               if (strncmp(temp, "ARCS", 4)) {
                    fprintf(stderr, "Unable to find ARCS\n");
                    exit(2);
               }
               // number of columns may be underestimate
               int * head = new int[2*numberColumns];
               int * tail = new int[2*numberColumns];
               int * ub = new int[2*numberColumns];
               int * cost = new int[2*numberColumns];
               // ***** Remember to convert to C notation
               numberColumns = 0;
               while (fgets(temp, 100, fp)) {
                    int iHead;
                    int iTail;
                    int iUb;
                    int iCost;
                    if (!strncmp(temp, "DEMAND", 6))
                         break;
                    sscanf(temp, "%d %d %d %d", &iHead, &iTail, &iCost, &iUb);
                    iHead--;
                    iTail--;
                    head[numberColumns] = iHead;
                    tail[numberColumns] = iTail;
                    ub[numberColumns] = iUb;
                    cost[numberColumns] = iCost;
                    numberColumns++;
               }
               if (strncmp(temp, "DEMAND", 6)) {
                    fprintf(stderr, "Unable to find DEMAND\n");
                    exit(2);
               }
               // ***** Remember to convert to C notation
               while (fgets(temp, 100, fp)) {
                    int row;
                    int value;
                    if (!strncmp(temp, "END", 3))
                         break;
                    sscanf(temp, "%d %d", &row, &value);
                    upper[row-1] = value;
                    lower[row-1] = value;
               }
               if (strncmp(temp, "END", 3)) {
                    fprintf(stderr, "Unable to find END\n");
                    exit(2);
               }
               printf("Problem %d has %d rows and %d columns\n", problem,
                      numberRows, numberColumns);
               fclose(fp);
               ClpSimplex  model;
               // now build model

               double * objective = new double[numberColumns];
               double * lowerColumn = new double[numberColumns];
               double * upperColumn = new double[numberColumns];

               double * element = new double [2*numberColumns];
               CoinBigIndex * start = new CoinBigIndex [numberColumns+1];
               int * row = new int[2*numberColumns];
               start[numberColumns] = 2 * numberColumns;
               for (i = 0; i < numberColumns; i++) {
                    start[i] = 2 * i;
                    element[2*i] = -1.0;
                    element[2*i+1] = 1.0;
                    row[2*i] = head[i];
                    row[2*i+1] = tail[i];
                    lowerColumn[i] = 0.0;
                    upperColumn[i] = ub[i];
                    objective[i] = cost[i];
               }
               // Create Packed Matrix
               CoinPackedMatrix matrix;
               int * lengths = NULL;
               matrix.assignMatrix(true, numberRows, numberColumns,
                                   2 * numberColumns, element, row, start, lengths);
               // load model
               model.loadProblem(matrix,
                                 lowerColumn, upperColumn, objective,
                                 lower, upper);
               model.factorization()->maximumPivots(200 + model.numberRows() / 100);
               model.createStatus();
               double time1 = CoinCpuTime();
               model.dual();
               std::cout << "Network problem, ClpPackedMatrix took " << CoinCpuTime() - time1 << " seconds" << std::endl;
               ClpPlusMinusOneMatrix * plusMinus = new ClpPlusMinusOneMatrix(matrix);
               assert (plusMinus->getIndices()); // would be zero if not +- one
               //ClpPlusMinusOneMatrix *plusminus_matrix;

               //plusminus_matrix = new ClpPlusMinusOneMatrix;

               //plusminus_matrix->passInCopy(numberRows, numberColumns, true, plusMinus->getMutableIndices(),
               //                         plusMinus->startPositive(),plusMinus->startNegative());
               model.loadProblem(*plusMinus,
                                 lowerColumn, upperColumn, objective,
                                 lower, upper);
               //model.replaceMatrix( plusminus_matrix , true);
               delete plusMinus;
               //model.createStatus();
               //model.initialSolve();
               //model.writeMps("xx.mps");

               model.factorization()->maximumPivots(200 + model.numberRows() / 100);
               model.createStatus();
               time1 = CoinCpuTime();
               model.dual();
               std::cout << "Network problem, ClpPlusMinusOneMatrix took " << CoinCpuTime() - time1 << " seconds" << std::endl;
               ClpNetworkMatrix network(numberColumns, head, tail);
               model.loadProblem(network,
                                 lowerColumn, upperColumn, objective,
                                 lower, upper);

               model.factorization()->maximumPivots(200 + model.numberRows() / 100);
               model.createStatus();
               time1 = CoinCpuTime();
               model.dual();
               std::cout << "Network problem, ClpNetworkMatrix took " << CoinCpuTime() - time1 << " seconds" << std::endl;
               delete [] lower;
               delete [] upper;
               delete [] head;
               delete [] tail;
               delete [] ub;
               delete [] cost;
               delete [] objective;
               delete [] lowerColumn;
               delete [] upperColumn;
          }
     }
#ifdef QUADRATIC
     // Test quadratic to solve linear
     if (1) {
          CoinMpsIO m;
          std::string fn = dirSample + "exmip1";
          if (m.readMps(fn.c_str(), "mps") == 0) {
               ClpSimplex solution;
               solution.loadProblem(*m.getMatrixByCol(), m.getColLower(), m.getColUpper(),
                                    m.getObjCoefficients(),
                                    m.getRowLower(), m.getRowUpper());
               //solution.dual();
               // get quadratic part
               int numberColumns = solution.numberColumns();
               int * start = new int [numberColumns+1];
               int * column = new int[numberColumns];
               double * element = new double[numberColumns];
               int i;
               start[0] = 0;
               int n = 0;
               int kk = numberColumns - 1;
               int kk2 = numberColumns - 1;
               for (i = 0; i < numberColumns; i++) {
                    if (i >= kk) {
                         column[n] = i;
                         if (i >= kk2)
                              element[n] = 1.0e-1;
                         else
                              element[n] = 0.0;
                         n++;
                    }
                    start[i+1] = n;
               }
               // Load up objective
               solution.loadQuadraticObjective(numberColumns, start, column, element);
               delete [] start;
               delete [] column;
               delete [] element;
               //solution.quadraticSLP(50,1.0e-4);
               CoinRelFltEq eq(1.0e-4);
               //assert(eq(objValue,820.0));
               //solution.setLogLevel(63);
               solution.primal();
               printSol(solution);
               //assert(eq(objValue,3.2368421));
               //exit(77);
          } else {
               std::cerr << "Error reading exmip1 from sample data. Skipping test." << std::endl;
          }
     }
     // Test quadratic
     if (1) {
          CoinMpsIO m;
          std::string fn = dirSample + "share2qp";
          //fn = "share2qpb";
          if (m.readMps(fn.c_str(), "mps") == 0) {
               ClpSimplex model;
               model.loadProblem(*m.getMatrixByCol(), m.getColLower(), m.getColUpper(),
                                 m.getObjCoefficients(),
                                 m.getRowLower(), m.getRowUpper());
               model.dual();
               // get quadratic part
               int * start = NULL;
               int * column = NULL;
               double * element = NULL;
               m.readQuadraticMps(NULL, start, column, element, 2);
               int column2[200];
               double element2[200];
               int start2[80];
               int j;
               start2[0] = 0;
               int nel = 0;
               bool good = false;
               for (j = 0; j < 79; j++) {
                    if (start[j] == start[j+1]) {
                         column2[nel] = j;
                         element2[nel] = 0.0;
                         nel++;
                    } else {
                         int i;
                         for (i = start[j]; i < start[j+1]; i++) {
                              column2[nel] = column[i];
                              element2[nel++] = element[i];
                         }
                    }
                    start2[j+1] = nel;
               }
               // Load up objective
               if (good)
                    model.loadQuadraticObjective(model.numberColumns(), start2, column2, element2);
               else
                    model.loadQuadraticObjective(model.numberColumns(), start, column, element);
               delete [] start;
               delete [] column;
               delete [] element;
               int numberColumns = model.numberColumns();
               model.scaling(0);
#if 0
               model.nonlinearSLP(50, 1.0e-4);
#else
               // Get feasible
               ClpObjective * saveObjective = model.objectiveAsObject()->clone();
               ClpLinearObjective zeroObjective(NULL, numberColumns);
               model.setObjective(&zeroObjective);
               model.dual();
               model.setObjective(saveObjective);
               delete saveObjective;
#endif
               //model.setLogLevel(63);
               //exit(77);
               model.setFactorizationFrequency(10);
               model.primal();
               printSol(model);
#ifndef NDEBUG
               double objValue = model.getObjValue();
#endif
               CoinRelFltEq eq(1.0e-4);
               assert(eq(objValue, -400.92));
               // and again for barrier
               model.barrier(false);
               //printSol(model);
               model.allSlackBasis();
               model.primal();
               //printSol(model);
          } else {
               std::cerr << "Error reading share2qp from sample data. Skipping test." << std::endl;
          }
     }
     if (0) {
          CoinMpsIO m;
          std::string fn = "./beale";
          //fn = "./jensen";
          if (m.readMps(fn.c_str(), "mps") == 0) {
               ClpSimplex solution;
               solution.loadProblem(*m.getMatrixByCol(), m.getColLower(), m.getColUpper(),
                                    m.getObjCoefficients(),
                                    m.getRowLower(), m.getRowUpper());
               solution.setDblParam(ClpObjOffset, m.objectiveOffset());
               solution.dual();
               // get quadratic part
               int * start = NULL;
               int * column = NULL;
               double * element = NULL;
               m.readQuadraticMps(NULL, start, column, element, 2);
               // Load up objective
               solution.loadQuadraticObjective(solution.numberColumns(), start, column, element);
               delete [] start;
               delete [] column;
               delete [] element;
               solution.primal(1);
               solution.nonlinearSLP(50, 1.0e-4);
               double objValue = solution.getObjValue();
               CoinRelFltEq eq(1.0e-4);
               assert(eq(objValue, 0.5));
               solution.primal();
               objValue = solution.getObjValue();
               assert(eq(objValue, 0.5));
          } else {
               std::cerr << "Error reading beale.mps. Skipping test." << std::endl;
          }
     }
#endif
     // Test CoinStructuredModel
     {

          // Sub block
          CoinModel sub;
          {
               // matrix data
               //deliberate hiccup of 2 between 0 and 1
               CoinBigIndex start[5] = {0, 4, 7, 8, 9};
               int length[5] = {2, 3, 1, 1, 1};
               int rows[11] = {0, 2, -1, -1, 0, 1, 2, 0, 1, 2};
               double elements[11] = {7.0, 2.0, 1.0e10, 1.0e10, -2.0, 1.0, -2.0, 1, 1, 1};
               CoinPackedMatrix matrix(true, 3, 5, 8, elements, rows, start, length);
               // by row
               matrix.reverseOrdering();
               const double * element = matrix.getElements();
               const int * column = matrix.getIndices();
               const CoinBigIndex * rowStart = matrix.getVectorStarts();
               const int * rowLength = matrix.getVectorLengths();

               // rim data
               //double objective[5]={-4.0,1.0,0.0,0.0,0.0};
               double rowLower[3] = {14.0, 3.0, 3.0};
               double rowUpper[3] = {14.0, 3.0, 3.0};
               //double columnLower[5]={0.0,0.0,0.0,0.0,0.0};
               //double columnUpper[5]={100.0,100.0,100.0,100.0,100.0};

               for (int i = 0; i < 3; i++) {
                    sub.addRow(rowLength[i], column + rowStart[i],
                               element + rowStart[i], rowLower[i], rowUpper[i]);
               }
               //for (int i=0;i<5;i++) {
               //sub.setColumnBounds(i,columnLower[i],columnUpper[i]);
               //sub.setColumnObjective(i,objective[i]);
               //}
               sub.convertMatrix();
          }
          // Top
          CoinModel top;
          {
               // matrix data
               CoinBigIndex start[5] = {0, 2, 4, 6, 8};
               int length[5] = {2, 2, 2, 2, 2};
               int rows[10] = {0, 1, 0, 1, 0, 1, 0, 1, 0, 1};
               double elements[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
               CoinPackedMatrix matrix(true, 2, 5, 8, elements, rows, start, length);
               // by row
               matrix.reverseOrdering();
               const double * element = matrix.getElements();
               const int * column = matrix.getIndices();
               const CoinBigIndex * rowStart = matrix.getVectorStarts();
               const int * rowLength = matrix.getVectorLengths();

               // rim data
               double objective[5] = { -4.0, 1.0, 0.0, 0.0, 0.0};
               //double rowLower[3]={14.0,3.0,3.0};
               //double rowUpper[3]={14.0,3.0,3.0};
               double columnLower[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
               double columnUpper[5] = {100.0, 100.0, 100.0, 100.0, 100.0};

               for (int i = 0; i < 2; i++) {
                    top.addRow(rowLength[i], column + rowStart[i],
                               element + rowStart[i],
                               -COIN_DBL_MAX, COIN_DBL_MAX);
               }
               for (int i = 0; i < 5; i++) {
                    top.setColumnBounds(i, columnLower[i], columnUpper[i]);
                    top.setColumnObjective(i, objective[i]);
               }
               top.convertMatrix();
          }
          // Create a structured model
          CoinStructuredModel structured;
          int numberBlocks = 5;
          for (int i = 0; i < numberBlocks; i++) {
               std::string topName = "row_master";
               std::string blockName = "block_";
               char bName = static_cast<char>('a' + static_cast<char>(i));
               blockName.append(1, bName);
               structured.addBlock(topName, blockName, top);
               structured.addBlock(blockName, blockName, sub);
          }
          // Set rhs on first block
          CoinModel * first = structured.coinBlock(0);
          for (int i = 0; i < 2; i++) {
               first->setRowLower(i, 0.0);
               first->setRowUpper(i, 100.0);
          }
          // Refresh whats set
          structured.refresh(0);
          // Could perturb stuff, but for first go don't bother
          ClpSimplex fullModel;
          // There is no original stuff set - think
          fullModel.loadProblem(structured, false);
          fullModel.dual();
          fullModel.dropNames();
          fullModel.writeMps("test.mps");
          // Make up very simple nested model - not realistic
          // Create a structured model
          CoinStructuredModel structured2;
          numberBlocks = 3;
          for (int i = 0; i < numberBlocks; i++) {
               std::string blockName = "block_";
               char bName = static_cast<char>('a' + static_cast<char>(i));
               blockName.append(1, bName);
               structured2.addBlock(blockName, blockName, structured);
          }
          fullModel.loadProblem(structured2, false);
          fullModel.dual();
          fullModel.dropNames();
          fullModel.writeMps("test2.mps");
     }
}
