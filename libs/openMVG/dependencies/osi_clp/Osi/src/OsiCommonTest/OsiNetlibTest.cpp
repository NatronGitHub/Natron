/*
  Copyright (C) 2000 -- 2010, Lou Hafer, International Business Machines,
  and others.  All Rights Reserved.

  This code is licensed under the terms of the Eclipse Public License (EPL).
*/

#include "CoinPragma.hpp"

#include "OsiUnitTests.hpp"

#include "OsiConfig.h"

#include "CoinTime.hpp"
#include "CoinFloatEqual.hpp"

/*
  #include <cstdlib>
  #include <cassert>
  #include <vector>
  #include <iostream>
  #include <iomanip>
  #include <sstream>
  #include <cstdio>
*/

#include "OsiSolverInterface.hpp"

/*! \brief Run solvers on NetLib problems.

  The routine creates a vector of NetLib problems (problem name, objective,
  various other characteristics), and a vector of solvers to be tested.

  Each solver is run on each problem. The run is deemed successful if the
  solver reports the correct problem size after loading and returns the
  correct objective value after optimization.

  If multiple solvers are available, the results are compared pairwise against
  the results reported by adjacent solvers in the solver vector. Due to
  limitations of the volume solver, it must be the last solver in vecEmptySiP.
*/

void OsiSolverInterfaceMpsUnitTest
  (const std::vector<OsiSolverInterface*> & vecEmptySiP,
   const std::string & mpsDir)

{ int i ;
  unsigned int m ;

/*
  Vectors to hold test problem names and characteristics. The objective value
  after optimization (objValue) must agree to the specified tolerance
  (objValueTol).
*/
  std::vector<std::string> mpsName ;
  std::vector<bool> minObj ;
  std::vector<int> nRows ;
  std::vector<int> nCols ;
  std::vector<double> objValue ;
  std::vector<double> objValueTol ;
/*
  And a macro to make the vector creation marginally readable.
*/
#define PUSH_MPS(zz_mpsName_zz,zz_minObj_zz,\
		 zz_nRows_zz,zz_nCols_zz,zz_objValue_zz,zz_objValueTol_zz) \
  mpsName.push_back(zz_mpsName_zz) ; \
  minObj.push_back(zz_minObj_zz) ; \
  nRows.push_back(zz_nRows_zz) ; \
  nCols.push_back(zz_nCols_zz) ; \
  objValueTol.push_back(zz_objValueTol_zz) ; \
  objValue.push_back(zz_objValue_zz) ;

/*
  Load up the problem vector. Note that the row counts here include the
  objective function.
*/
  PUSH_MPS("25fv47",true,822,1571,5.5018458883E+03,1.0e-10)
  PUSH_MPS("80bau3b",true,2263,9799,9.8722419241E+05,1.e-10)
  PUSH_MPS("adlittle",true,57,97,2.2549496316e+05,1.e-10)
  PUSH_MPS("afiro",true,28,32,-4.6475314286e+02,1.e-10)
  PUSH_MPS("agg",true,489,163,-3.5991767287e+07,1.e-10)
  PUSH_MPS("agg2",true,517,302,-2.0239252356e+07,1.e-10)
  PUSH_MPS("agg3",true,517,302,1.0312115935e+07,1.e-10)
  PUSH_MPS("bandm",true,306,472,-1.5862801845e+02,1.e-10)
  PUSH_MPS("beaconfd",true,174,262,3.3592485807e+04,1.e-10)
  PUSH_MPS("blend",true,75,83,-3.0812149846e+01,1.e-10)
  PUSH_MPS("bnl1",true,644,1175,1.9776295615E+03,1.e-10)
  PUSH_MPS("bnl2",true,2325,3489,1.8112365404e+03,1.e-10)
  PUSH_MPS("boeing1",true,/*351*/352,384,-3.3521356751e+02,1.e-10)
  PUSH_MPS("boeing2",true,167,143,-3.1501872802e+02,1.e-10)
  PUSH_MPS("bore3d",true,234,315,1.3730803942e+03,1.e-10)
  PUSH_MPS("brandy",true,221,249,1.5185098965e+03,1.e-10)
  PUSH_MPS("capri",true,272,353,2.6900129138e+03,1.e-10)
  PUSH_MPS("cycle",true,1904,2857,-5.2263930249e+00,1.e-9)
  PUSH_MPS("czprob",true,930,3523,2.1851966989e+06,1.e-10)
  PUSH_MPS("d2q06c",true,2172,5167,122784.21557456,1.e-7)
  PUSH_MPS("d6cube",true,416,6184,3.1549166667e+02,1.e-8)
  PUSH_MPS("degen2",true,445,534,-1.4351780000e+03,1.e-10)
  PUSH_MPS("degen3",true,1504,1818,-9.8729400000e+02,1.e-10)
  PUSH_MPS("dfl001",true,6072,12230,1.1266396047E+07,1.e-5)
  PUSH_MPS("e226",true,224,282,(-18.751929066+7.113),1.e-10) // NOTE: Objective function has constant of 7.113
  PUSH_MPS("etamacro",true,401,688,-7.5571521774e+02 ,1.e-6)
  PUSH_MPS("fffff800",true,525,854,5.5567961165e+05,1.e-6)
  PUSH_MPS("finnis",true,498,614,1.7279096547e+05,1.e-6)
  PUSH_MPS("fit1d",true,25,1026,-9.1463780924e+03,1.e-10)
  PUSH_MPS("fit1p",true,628,1677,9.1463780924e+03,1.e-10)
  PUSH_MPS("fit2d",true,26,10500,-6.8464293294e+04,1.e-10)
  PUSH_MPS("fit2p",true,3001,13525,6.8464293232e+04,1.e-9)
  PUSH_MPS("forplan",true,162,421,-6.6421873953e+02,1.e-6)
  PUSH_MPS("ganges",true,1310,1681,-1.0958636356e+05,1.e-5)
  PUSH_MPS("gfrd-pnc",true,617,1092,6.9022359995e+06,1.e-10)
  PUSH_MPS("greenbea",true,2393,5405,/*-7.2462405908e+07*/-72555248.129846,1.e-10)
  PUSH_MPS("greenbeb",true,2393,5405,/*-4.3021476065e+06*/-4302260.2612066,1.e-10)
  PUSH_MPS("grow15",true,301,645,-1.0687094129e+08,1.e-10)
  PUSH_MPS("grow22",true,441,946,-1.6083433648e+08,1.e-10)
  PUSH_MPS("grow7",true,141,301,-4.7787811815e+07,1.e-10)
  PUSH_MPS("israel",true,175,142,-8.9664482186e+05,1.e-10)
  PUSH_MPS("kb2",true,44,41,-1.7499001299e+03,1.e-10)
  PUSH_MPS("lotfi",true,154,308,-2.5264706062e+01,1.e-10)
  PUSH_MPS("maros",true,847,1443,-5.8063743701e+04,1.e-10)
  PUSH_MPS("maros-r7",true,3137,9408,1.4971851665e+06,1.e-10)
  PUSH_MPS("modszk1",true,688,1620,3.2061972906e+02,1.e-10)
  PUSH_MPS("nesm",true,663,2923,1.4076073035e+07,1.e-5)
  PUSH_MPS("perold",true,626,1376,-9.3807580773e+03,1.e-6)
  PUSH_MPS("pilot",true,1442,3652,/*-5.5740430007e+02*/-557.48972927292,5.e-5)
  PUSH_MPS("pilot4",true,411,1000,-2.5811392641e+03,1.e-6)
  PUSH_MPS("pilot87",true,2031,4883,3.0171072827e+02,1.e-4)
  PUSH_MPS("pilotnov",true,976,2172,-4.4972761882e+03,1.e-10)
  // ?? PUSH_MPS("qap8",true,913,1632,2.0350000000e+02,1.e-10)
  // ?? PUSH_MPS("qap12",true,3193,8856,5.2289435056e+02,1.e-10)
  // ?? PUSH_MPS("qap15",true,6331,22275,1.0409940410e+03,1.e-10)
  PUSH_MPS("recipe",true,92,180,-2.6661600000e+02,1.e-10)
  PUSH_MPS("sc105",true,106,103,-5.2202061212e+01,1.e-10)
  PUSH_MPS("sc205",true,206,203,-5.2202061212e+01,1.e-10)
  PUSH_MPS("sc50a",true,51,48,-6.4575077059e+01,1.e-10)
  PUSH_MPS("sc50b",true,51,48,-7.0000000000e+01,1.e-10)
  PUSH_MPS("scagr25",true,472,500,-1.4753433061e+07,1.e-10)
  PUSH_MPS("scagr7",true,130,140,-2.3313892548e+06,1.e-6)
  PUSH_MPS("scfxm1",true,331,457,1.8416759028e+04,1.e-10)
  PUSH_MPS("scfxm2",true,661,914,3.6660261565e+04,1.e-10)
  PUSH_MPS("scfxm3",true,991,1371,5.4901254550e+04,1.e-10)
  PUSH_MPS("scorpion",true,389,358,1.8781248227e+03,1.e-10)
  PUSH_MPS("scrs8",true,491,1169,9.0429998619e+02,1.e-5)
  PUSH_MPS("scsd1",true,78,760,8.6666666743e+00,1.e-10)
  PUSH_MPS("scsd6",true,148,1350,5.0500000078e+01,1.e-10)
  PUSH_MPS("scsd8",true,398,2750,9.0499999993e+02,1.e-8)
  PUSH_MPS("sctap1",true,301,480,1.4122500000e+03,1.e-10)
  PUSH_MPS("sctap2",true,1091,1880,1.7248071429e+03,1.e-10)
  PUSH_MPS("sctap3",true,1481,2480,1.4240000000e+03,1.e-10)
  PUSH_MPS("seba",true,516,1028,1.5711600000e+04,1.e-10)
  PUSH_MPS("share1b",true,118,225,-7.6589318579e+04,1.e-10)
  PUSH_MPS("share2b",true,97,79,-4.1573224074e+02,1.e-10)
  PUSH_MPS("shell",true,537,1775,1.2088253460e+09,1.e-10)
  PUSH_MPS("ship04l",true,403,2118,1.7933245380e+06,1.e-10)
  PUSH_MPS("ship04s",true,403,1458,1.7987147004e+06,1.e-10)
  PUSH_MPS("ship08l",true,779,4283,1.9090552114e+06,1.e-10)
  PUSH_MPS("ship08s",true,779,2387,1.9200982105e+06,1.e-10)
  PUSH_MPS("ship12l",true,1152,5427,1.4701879193e+06,1.e-10)
  PUSH_MPS("ship12s",true,1152,2763,1.4892361344e+06,1.e-10)
  PUSH_MPS("sierra",true,1228,2036,1.5394362184e+07,1.e-10)
  PUSH_MPS("stair",true,357,467,-2.5126695119e+02,1.e-10)
  PUSH_MPS("standata",true,360,1075,1.2576995000e+03,1.e-10)
  // GUB PUSH_MPS("standgub",true,362,1184,1257.6995,1.e-10)
  PUSH_MPS("standmps",true,468,1075,1.4060175000E+03,1.e-10)
  PUSH_MPS("stocfor1",true,118,111,-4.1131976219E+04,1.e-10)
  PUSH_MPS("stocfor2",true,2158,2031,-3.9024408538e+04,1.e-10)
  // ?? PUSH_MPS("stocfor3",true,16676,15695,-3.9976661576e+04,1.e-10)
  // ?? PUSH_MPS("truss",true,1001,8806,4.5881584719e+05,1.e-10)
  PUSH_MPS("tuff",true,334,587,2.9214776509e-01,1.e-10)
  PUSH_MPS("vtpbase",true,199,203,1.2983146246e+05,1.e-10)
  PUSH_MPS("wood1p",true,245,2594,1.4429024116e+00,5.e-5)
  PUSH_MPS("woodw",true,1099,8405,1.3044763331E+00,1.e-10)

#undef PUSH_MPS

  const unsigned int numProblems = static_cast<unsigned int>(mpsName.size()) ;

/*
  Create vectors to hold solver interfaces, the name of each solver interface,
  the current state of processing, and statistics about the number of problems
  solved and the time taken.
*/
  const int numSolvers = static_cast<int>(vecEmptySiP.size()) ;
  std::vector<OsiSolverInterface*> vecSiP(numSolvers) ;
  std::vector<std::string> siName(numSolvers) ;
  std::vector<int> siStage(numSolvers) ;
  std::vector<int> numProbSolved(numSolvers) ;
  std::vector<double> timeTaken(numSolvers) ;
  for (i = 0 ; i < numSolvers ; i++)
  { siName[i] = "unknown" ;
    numProbSolved[i] = 0 ;
    timeTaken[i] = 0.0 ; }
/*
  For each problem, create a fresh clone of the `empty' solvers
  from vecEmptySiP, then proceed in stages: read the MPS file, solve the
  problem, check the solution. If there are multiple solvers in vecSiP,
  the results of each solver are compared with its neighbors in the vector.
*/
  for (m = 0 ; m < numProblems ; m++) {
    std::cout << "  processing mps file: " << mpsName[m]
      << " (" << m+1 << " out of " << numProblems << ")" << std::endl ;
/*
  Stage 0: Create fresh solver clones.
*/
    int solversReadMpsFile = 0 ;
    for (i = numSolvers-1 ; i >= 0 ; --i) {
      vecSiP[i] = vecEmptySiP[i]->clone() ;
      vecSiP[i]->getStrParam(OsiSolverName,siName[i]) ;
      siStage[i] = 0 ;
    }
/*
  Stage 1: Read the MPS file into each solver interface.  As a basic check,
  make sure the size of the constraint matrix is correct.
*/
    for (i = 0 ; i < numSolvers ; i++) {
      std::string fn = mpsDir+mpsName[m] ;
      vecSiP[i]->readMps(fn.c_str(),"mps") ;
      if (minObj[m])
        vecSiP[i]->setObjSense(1.0) ;
      else
        vecSiP[i]->setObjSense(-1.0) ;
      int nr = vecSiP[i]->getNumRows() ;
      int nc = vecSiP[i]->getNumCols() ;
      if (nr == nRows[m]-1 && nc == nCols[m])
      { siStage[i] = 1 ;
        solversReadMpsFile++ ; }
    }
/*
  If more than one solver succeeded, compare representations.
*/
    if (solversReadMpsFile > 0) {
      // Find an initial pair to compare
      int s1 ;
      for (s1 = 0 ; s1 < numSolvers-1 && siStage[s1] < 1 ; s1++) ;
      int s2 ;
      for (s2 = s1+1 ; s2 < numSolvers && siStage[s2] < 1 ; s2++) ;
      while (s2 < numSolvers) {
        std::cout << "  comparing problem representation for " << siName[s1] << " and " << siName[s2] << " ..." ;
        if (OsiUnitTest::compareProblems(vecSiP[s1],vecSiP[s2]))
        	std::cout << " ok." << std::endl;
        s1 = s2 ;
        for (s2++ ; s2 < numSolvers && siStage[s2] < 1 ; s2++) ;
      }
    }
/*
  Stage 2: Ask each solver that successfully read the problem to solve it,
  then check the return code and objective.
*/
    for (i = 0 ; i < numSolvers ; ++i)
    { if (siStage[i] < 1) continue ;

      double startTime = CoinCpuTime() ;
      OSIUNITTEST_CATCH_ERROR(vecSiP[i]->initialSolve(), continue, *vecSiP[i], "netlib " + mpsName[m]);

      double timeOfSolution = CoinCpuTime()-startTime;
      OSIUNITTEST_ASSERT_ERROR(vecSiP[i]->isProvenOptimal(), {}, *vecSiP[i], "netlib " + mpsName[m]);
      if (vecSiP[i]->isProvenOptimal())
      { double soln = vecSiP[i]->getObjValue();
        CoinRelFltEq eq(objValueTol[m]) ;
        OSIUNITTEST_ASSERT_ERROR(eq(soln,objValue[m]),
      	    std::cerr << soln << " != " << objValue[m] << "; error = " << fabs(objValue[m]-soln),
            *vecSiP[i], "netlib " + mpsName[m]);
        if (eq(soln,objValue[m])) {
        	std::cout << "  " << siName[i] << " "	<< soln << " = " << objValue[m] << ", "	<< vecSiP[i]->getIterationCount() << " iters; okay" ;
        	numProbSolved[i]++ ; }
        std::cout << " - took " << timeOfSolution << " seconds." << std::endl;
        timeTaken[i] += timeOfSolution;
      }
      else
      { std::cout.flush() ;
        std::cerr << "  " << siName[i] << "; error " ;
        if (vecSiP[i]->isProvenPrimalInfeasible())
        	std::cerr << "primal infeasible" ;
        else if (vecSiP[i]->isIterationLimitReached())
        	std::cerr << "iteration limit" ;
        else if (vecSiP[i]->isAbandoned())
        	std::cerr << "abandoned" ;
        else
        	std::cerr << "unknown" ; } }
/*
  Delete the used solver interfaces so we can reload fresh clones for the
  next problem.
*/
    for (i = 0 ; i < numSolvers ; i++) delete vecSiP[i] ; }
/*
  Print a summary for each solver.
*/
  for (i = 0 ; i < numSolvers ; i++) {
    std::cout
      << siName[i] << " solved "
      << numProbSolved[i] << " out of "
      << numProblems << " and took " << timeTaken[i] << " seconds."
      << std::endl ;
  }
}
