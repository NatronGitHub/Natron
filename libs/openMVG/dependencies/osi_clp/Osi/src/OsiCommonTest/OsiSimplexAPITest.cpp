/*
  Copyright (C) 2000 -- 2010, Lou Hafer, International Business Machines,
  and others.  All Rights Reserved.

  This code is licensed under the terms of the Eclipse Public License (EPL).
*/

#include "CoinPragma.hpp"

#include "OsiUnitTests.hpp"

#include "OsiConfig.h"

/*
  #include "CoinTime.hpp"
  #include <cstdlib>
  #include <cassert>
  #include <vector>
  #include <iostream>
  #include <iomanip>
  #include <sstream>
  #include <cstdio>
*/

#include "CoinHelperFunctions.hpp"
#include "CoinFinite.hpp"
#include "CoinFloatEqual.hpp"
#include "CoinPackedVector.hpp"
#include "CoinPackedMatrix.hpp"
#include "OsiSolverInterface.hpp"

using namespace OsiUnitTest ;

/*
  Helper methods for the OSI simplex API test.
*/
namespace {

/*
  Test that the given vector is a unit vector with a 1 in the specified index
  position
*/
bool isUnitVector (int /* ndx */, int len, double *vec)
{ bool retval = false ;

  CoinAbsFltEq fltEq ;

  int nzCount = 0 ;
  int oneCount = 0 ;
  int onePosn = -1 ;

  for (int j = 0 ; j < len ; j++)
  { if (!fltEq(vec[j],0.0))
    { nzCount++ ;
      if (fltEq(vec[j],1.0))
      { oneCount++ ;
	onePosn = j ; } } }
  if (nzCount == 1 && oneCount == 1 && onePosn >= 0)
  { retval = true ; }
  
  if (OsiUnitTest::verbosity >= 2 && !retval)
  { if (nzCount > oneCount)
    { std::cout
	<< "    Vector contains " << nzCount-oneCount
	<< " elements that are neither 1.0 or 0.0." << std::endl ; }
    if (oneCount > 1)
    { std::cout
	<< "    Vector contains " << oneCount
	<< " elements that are 1.0." << std::endl ; }
    if (oneCount < 1)
    { std::cout
	<< "    Vector contains no elements that are 1.0." << std::endl ; } }

  return (retval) ; }
  

/*
  Build a constraint system in basis order. Assumes that enableFactorization
  has been called. Indirect test of getBasics, because if the basis columns are
  loaded incorrectly, checks that B inv(B) = I will surely fail.
*/
CoinPackedMatrix *buildBasisMatrix (const OsiSolverInterface *si)
{ std::string solverName ;
  si->getStrParam(OsiSolverName,solverName) ;

  CoinPackedMatrix *basisMtx = new CoinPackedMatrix() ;
  const CoinPackedMatrix *mtx = si->getMatrixByCol() ;

  int m = si->getNumRows() ;
  int n = si->getNumCols() ;
  int *basicIndices = new int[m] ;
  si->getBasics(basicIndices) ;

  for (int i = 0 ; i < m ; i++)
  { int j = basicIndices[i] ;
    if (j < n)
    { if (OsiUnitTest::verbosity >= 2)
      { std::cout
	  << "  Retrieving column " << j << " for basis pos'n " << i << "."
	  << std::endl ; }
      CoinShallowPackedVector col = mtx->getVector(j) ;
      basisMtx->appendCol(col) ; }
    else
    { j -= n ;
      if (OsiUnitTest::verbosity >= 2)
      { std::cout
	  << "  Fabricating e<" << j << "> for basis pos'n " << i << "."
	  << std::endl ; }
      CoinPackedVector ei = CoinPackedVector(1,&j,1.0) ;
      basisMtx->appendCol(ei) ; } }

  return (basisMtx) ; }


/*
  Test columns beta<k> = inv(B)e<k> of the basis inverse by calculating
  B beta<k> and checking that the result is the appropriate unit vector.
  Also tests getBasics, because we use it to build the basis matrix.

  It's assumed that the si passed as a parameter is ready for tableau
  queries: a problem has been loaded and solved to optimality and
  enableFactorization has been called.
*/
void testBInvCol (const OsiSolverInterface *si)
{ std::string solverName ;
  si->getStrParam(OsiSolverName,solverName) ;

  int m = si->getNumRows() ;

  std::cout << "  Testing getBInvCol ... " << std::endl;

  CoinPackedMatrix *basisMtx = buildBasisMatrix(si) ;
/*
  Fetch beta<k>, calculate B beta<k>, k = 0, ..., m-1, and check that we have
  the appropriate unit vector.
*/
  double *betak = new double[m] ;
  double *ek = new double[m] ;
  for (int k = 0 ; k < m ; k++)
  { 
    CoinFillN(betak,m,COIN_DBL_MAX) ;
    CoinFillN(ek,m,COIN_DBL_MAX) ;
    OSIUNITTEST_CATCH_ERROR(si->getBInvCol(k,betak), {}, solverName, "testBInvCol");

    basisMtx->times(betak,ek) ;
    OSIUNITTEST_ASSERT_ERROR(isUnitVector(k,m,ek),
    	if (OsiUnitTest::verbosity >= 1)
    	{ std::cout << "  " << "B beta<" << k << "> != e<" << k << ">." << std::endl ; },
      solverName, "testBInvCol");
  }
  delete[] betak ;
  delete[] ek ;
  delete basisMtx ;
}

/*
  Test rows beta<i> = e<i>inv(B) of the basis inverse by calculating beta<i>B
  and checking that the result is the appropriate unit vector. Also tests
  getBasics, because we need it to build the basis matrix.

  It's assumed that the si passed as a parameter is ready for tableau queries:
  a problem has been loaded and solved to optimality and enableFactorization
  has been called.
*/
void testBInvRow (const OsiSolverInterface *si)
{ std::string solverName ;
  si->getStrParam(OsiSolverName,solverName) ;

  int m = si->getNumRows() ;

  std::cout << "  Testing getBInvRow ... " << std::endl;

/*
  Should construct in row-major form for transposeTimes, but efficiency is
  not all that big an issue here.
*/
  CoinPackedMatrix *basisMtx = buildBasisMatrix(si) ;
/*
  Fetch beta<i>, calculate beta<i> B, i = 0, ..., m-1, and check that we have
  the appropriate unit vector.
*/
  double *betai = new double[m] ;
  double *ei = new double[m] ;
  for (int i = 0 ; i < m ; i++)
  { CoinFillN(betai,m,COIN_DBL_MAX) ;
    CoinFillN(ei,m,COIN_DBL_MAX) ;
    OSIUNITTEST_CATCH_ERROR(si->getBInvRow(i,betai), {}, solverName, "testBInvRow");

    basisMtx->transposeTimes(betai,ei) ;
    OSIUNITTEST_ASSERT_ERROR(isUnitVector(i,m,ei),
    	if (OsiUnitTest::verbosity >= 1)
      { std::cout << "  " << "beta<" << i << ">B != e<" << i << ">." << std::endl ; },
      solverName, "testBInvRow");
  }
  delete[] betai ;
  delete[] ei ;
  delete basisMtx ;
}

/*
  Test columns abar<j> = inv(B) a<j> by checking that B abar<j> = a<j>.

  It's assumed that the si passed as a parameter is ready for tableau
  queries: a problem has been loaded and solved to optimality and
  enableFactorization has been called.
*/
void testBInvACol (const OsiSolverInterface *si)
{ std::string solverName ;
  si->getStrParam(OsiSolverName,solverName) ;

  int n = si->getNumCols() ;
  int m = si->getNumRows() ;

  std::cout << "  Testing getBInvACol ... " << std::endl;

  CoinPackedMatrix *basisMtx = buildBasisMatrix(si) ;
  const CoinPackedMatrix *mtx = si->getMatrixByCol() ;
/*
  Fetch abar<j>, calculate B abar<j>, k = 0, ..., n-1, and check that the
  result is a<j>.
*/
  double *abarj = new double[m] ;
  double *aj = new double[m] ;
  for (int j = 0 ; j < n ; j++)
  { CoinFillN(abarj,m,COIN_DBL_MAX) ;
    CoinFillN(aj,m,COIN_DBL_MAX) ;
    OSIUNITTEST_CATCH_ERROR(si->getBInvACol(j,abarj), {}, solverName, "testBInvACol");

    basisMtx->times(abarj,aj) ;
    const CoinShallowPackedVector pv = mtx->getVector(j) ;
    OSIUNITTEST_ASSERT_ERROR(isEquivalent(pv,m,aj),
    	if (OsiUnitTest::verbosity >= 1)
      { std::cout << "  " << "B abar<" << j << "> != a<" << j << ">." << std::endl ; },
      solverName, "testBInvACol");
  }
  delete[] abarj ;
  delete[] aj ;
  delete basisMtx ;
}

/*
  Test rows abar<i> = e<i>(inv(B)(A I)). This is an awkward thing to check,
  because there's no analog to the column identity B abar<j> = a<j>. Go with
  brute force: Build inv(B)A row by row with getBInvARow and check it against
  inv(B)A built column by column with getBInvACol. (Clearly, testBInvACol
  should run first.)
  
  e<i>(inv(B)I) is of course beta<i> = e<i>inv(B), so we can just check that
  beta<i>B = e<i>.
  
  It's assumed that the si passed as a parameter is ready for tableau
  queries: a problem has been loaded and solved to optimality and
  enableFactorization has been called.
*/
void testBInvARow (const OsiSolverInterface *si)
{ std::string solverName ;
  si->getStrParam(OsiSolverName,solverName) ;

  int n = si->getNumCols() ;
  int m = si->getNumRows() ;

  std::cout << "  Testing getBInvARow ... " << std::endl;

  CoinPackedMatrix *basisMtx = buildBasisMatrix(si) ;
/*
  Construct inv(B)A by column, then change over to row-major ordering so we
  can compare with the version build from tableau rows.
  
  Interesting quirk here: Turns out p0033's tableau has no nonzeros in rows
  with index 6 or 15. Because of that, when abarj is converted to row-major,
  it has only 15 rows, and that causes isEquivalent2 to fail. So force the
  full size.
*/
  CoinPackedMatrix abarjMtx ;
  double *abarj = new double[m] ;
  for (int j = 0 ; j < n ; j++)
  { si->getBInvACol(j,abarj) ;
    CoinPackedVector pkv ;
    pkv.setFullNonZero(m,abarj) ;
    abarjMtx.appendCol(pkv) ; }
  delete[] abarj ;
  abarjMtx.reverseOrdering() ;
  abarjMtx.setDimensions(m,n) ;
  if (OsiUnitTest::verbosity >= 1)
  { std::cout
      << "  Col-major tableau is " << abarjMtx.getNumRows() << " x "
      << abarjMtx.getNumCols() << " with " << abarjMtx.getNumElements()
      << " elements." << std::endl ; }
/*
  Construct inv(B)A by row. Check the vectors returned for inv(B)I = beta<i>
  as we go.
*/
  CoinPackedMatrix abariMtx ;
  abariMtx.reverseOrdering() ;
  double *abari = new double[n] ;
  double *betai = new double[m] ;
  double *ei = new double[m] ;
  for (int i = 0 ; i < m ; i++)
  { CoinFillN(abari,n,COIN_DBL_MAX) ;
    CoinFillN(betai,m,COIN_DBL_MAX) ;
    OSIUNITTEST_CATCH_ERROR(si->getBInvARow(i,abari,betai), {}, solverName, "testBInvARow");
    CoinPackedVector pkv ;
    pkv.setFullNonZero(n,abari) ;
    if (OsiUnitTest::verbosity >= 2)
    { std::cout << "  Adding" ;
      const int *indices = pkv.getIndices() ;
      for (int v = 0 ; v < pkv.getNumElements() ; v++)
      { std::cout << " (" << i << "," << indices[v] << ")" ; }
      std::cout << std::endl ;
      if (!isEquivalent(pkv,n,abari))
        std::cout << "  !! packed abari != full abari !!" << std::endl ; }
    abariMtx.appendRow(pkv) ;
    basisMtx->transposeTimes(betai,ei) ;
    OSIUNITTEST_ASSERT_ERROR(isUnitVector(i,m,ei),
    	if (OsiUnitTest::verbosity >= 1)
      { std::cout << "  " << "beta<" << i << ">B != e<" << i << ">." << std::endl ; },
      solverName, "testBInvARow");
  }
  abariMtx.setDimensions(m,n) ;
  if (OsiUnitTest::verbosity >= 2)
  { std::cout
      << "  Row-major tableau is " << abariMtx.getNumRows() << " x "
      << abariMtx.getNumCols() << " with " << abariMtx.getNumElements()
      << " elements." << std::endl ; }
  delete[] abari ;
  delete[] betai ;
  delete[] ei ;
  delete basisMtx ;
/*
  Check that the two matrices are equivalent. isEquivalent2 will report
  differences, but we won't get a good count. But then, one error is all we
  need to report.
*/
  OSIUNITTEST_ASSERT_ERROR(abariMtx.isEquivalent2(abarjMtx), {}, solverName, "testBInvARow: tableaus built by rows and columns match");
}

/*
  Test the row and column duals returned by getReducedGradient.

  ** This method requires that the solver have an optimal solution in hand. **

  The method checks the values returned by getReducedGradient against the
  values held in the solver (getRowPrice, getReducedCost) and tests that
  the sign of the reduced costs matches the status of the architectural
  variables. None of these are guaranteed unless the problem has been solved
  to optimality. The validity of the test hinges on the assumption that an
  implementor will just do the calculations for the given c rather than try
  to determine if the answer held in the solver is valid for the given c.

  * For row duals, test that y = c<B>inv(B), using getBInvCol (already
    tested) to obtain the columns of the basis inverse. Also check that
    we agree with getRowPrice.
  * For column duals (aka reduced costs), test that cbar = c<N> - yN,
    using the duals we've just checked. Also check that we agree with
    getReducedCost. Cross-check the sign of the reduced costs against the
    status vector returned by getBasisStatus.

  It's assumed that the si passed as a parameter is ready for tableau
  queries: a problem has been loaded and solved to optimality and
  enableFactorization has been called.
*/
void testReducedGradient (const OsiSolverInterface *si)
{ std::string solverName ;
  si->getStrParam(OsiSolverName,solverName) ;

  int n = si->getNumCols() ;
  int m = si->getNumRows() ;

  double objSense = si->getObjSense() ;
  std::cout
    << "  Testing getReducedGradient ... " ;
/*
  Acquire the solver's notion of current duals and reduced costs before we do
  anything else.
*/
  const double *yGold = si->getRowPrice() ;
  const double *cbarGold = si->getReducedCost() ;
/*
  Acquire the basis and build c<B>, the vector of basic cost coefficients. For
  logicals (basicIndices[k] >= n) assume a zero coefficient.
*/
  int *basicIndices = new int[m] ;
  si->getBasics(basicIndices) ;
  const double *c = si->getObjCoefficients() ;
  double *cB = new double[m] ;
  for (int k = 0 ; k < m ; k++)
  { int j = basicIndices[k] ;
    if (j < n)
    { if (OsiUnitTest::verbosity >= 2)
      { std::cout
	  << "  Retrieving c<" << j << "> = " << c[j]
	  << " for basis pos'n " << k << "." << std::endl ; }
    cB[k] = c[j] ; }
    else
    { if (OsiUnitTest::verbosity >= 2)
      { std::cout
	  << "  Assuming c<n+" << n-j << "> = " << 0.0
	  << " for basis pos'n " << k << "." << std::endl ; }
      cB[k] = 0.0 ; } }
  delete[] basicIndices ;
/*
  Call getReducedGradient to get duals and reduced costs.
*/
  double *cbar = new double[n] ;
  double *y = new double[m] ;
  OSIUNITTEST_CATCH_SEVERITY_EXPECTED(si->getReducedGradient(cbar,y,c),
  	delete[] cbar;
  	delete[] y;
  	return,
  	solverName, "testReducedGradient", TestOutcome::ERROR, solverName == "cplex");

/*
  Run through the columns of the basis. Retrieve beta<j>, calculate
  dot(c<B>,beta<j>) and check that all three sources of y<k> agree.
*/
  double *betaj = new double[m] ;
  CoinRelFltEq eq ;
  for (int k = 0 ; k < m ; k++)
  { double yk = 0.0 ;
    si->getBInvCol(k,betaj) ;
    for (int i = 0 ; i < m ; i++)
    { yk += cB[i]*betaj[i] ; }
    OSIUNITTEST_ASSERT_ERROR(eq(y[k],yGold[k]),
      if (OsiUnitTest::verbosity >= 1)
      	std::cout << "  " << y[k] << " = y<" << k << "> != yGold<" << k << "> = " << yGold[k] << ", diff = " << y[k]-yGold[k] << "." << std::endl,
    	solverName, "testReducedGradient");
    OSIUNITTEST_ASSERT_ERROR(eq(y[k],yk),
      if (OsiUnitTest::verbosity >= 1)
      	std::cout << "  " << y[k] << " = y<" << k << "> != c<B>beta<" << k << "> = " << yk << ", diff = " << y[k]-yk << "." << std::endl,
    	solverName, "testReducedGradient");
  }
  delete[] cB ;
  delete[] betaj ;
/*
  Now that we're confident the duals are correct, use them to calculate cbar
  as c-yN and check that all sources for cbar agree. Also check that the sign
  is correct given the status of the variable. There's no need to
  differentiate basic and nonbasic columns here.
*/
  int *archStatus = new int[n] ;
  int *logStatus = new int[m] ;
  si->getBasisStatus(archStatus,logStatus) ;
  double dualTol ;
  si->getDblParam(OsiDualTolerance,dualTol) ;

  const int OsiSimplex_isFree = 0 ;
  const int OsiSimplex_basic = 1 ;
  const int OsiSimplex_nbub = 2 ;
  const int OsiSimplex_nblb = 3 ;
  std::string statNames[] = { "NBFR", "B", "NBUB", "NBLB" } ;

  const CoinPackedMatrix *mtx = si->getMatrixByCol() ;
  double *cbarCalc = new double[n] ;
  mtx->transposeTimes(y,cbarCalc) ;
  std::transform(c,c+n,cbarCalc,cbarCalc,std::minus<double>()) ;

  for (int j = 1 ; j < n ; j++)
  { double cbarj = cbar[j] ;
    int statj = archStatus[j] ;
    if (OsiUnitTest::verbosity >= 2)
      std::cout << "  x<" << j << "> " << statNames[statj] << ", cbar<" << j << "> = " << cbarj << "." << std::endl ;
    OSIUNITTEST_ASSERT_ERROR(eq(cbarj,cbarGold[j]),
      if (OsiUnitTest::verbosity >= 1)
      	std::cout << "  " << cbarj << " = cbar<" << j << "> != cbarGold<" << j << "> = " << cbarGold[j] << ", diff = " << cbarj-cbarGold[j] << "." << std::endl,
    	solverName, "testReducedGradient");
    OSIUNITTEST_ASSERT_ERROR(eq(cbarj,cbarCalc[j]),
      if (OsiUnitTest::verbosity >= 1)
    	  std::cout << "  " << cbarj << " = cbar<" << j << "> != c<" << j << "> - ya<" << j << "> = " << cbarCalc[j] << ", diff = " << cbarj-cbarCalc[j] << "." << std::endl,
    	solverName, "testReducedGradient");
    double testcbarj = objSense*cbarj ;
    switch (statj)
    { case OsiSimplex_nbub:
      {
        OSIUNITTEST_ASSERT_ERROR(testcbarj <= dualTol,
          if (OsiUnitTest::verbosity >= 1)
          	std::cout << "  cbar<" << j << "> = " << cbarj << " has the wrong sign for a NBUB variable." << std::endl,
        	solverName, "testReducedGradient");
        break ; }
      case OsiSimplex_nblb:
      {
        OSIUNITTEST_ASSERT_ERROR(testcbarj >= -dualTol,
          if (OsiUnitTest::verbosity >= 1)
          	std::cout << "  cbar<" << j << "> = " << cbarj << " has the wrong sign for a NBLB variable." << std::endl,
        	solverName, "testReducedGradient");
        break ; }
      case OsiSimplex_isFree:
      {
        OSIUNITTEST_ASSERT_ERROR(CoinAbs(testcbarj) <= dualTol,
          if (OsiUnitTest::verbosity >= 1)
          	std::cout  << "  cbar<" << j << "> = " << cbarj << " should be zero for a NBFR variable." << std::endl,
        	solverName, "testReducedGradient");
        break ; }
      case OsiSimplex_basic:
      {
        OSIUNITTEST_ASSERT_ERROR(CoinAbs(testcbarj) <= dualTol,
          if (OsiUnitTest::verbosity >= 1)
          	std::cout  << "  cbar<" << j << "> = " << cbarj << " should be zero for a basic variable." << std::endl,
        	solverName, "testReducedGradient");
        break ; }
      default:
      { break ; } } }

  delete[] y ;
  delete[] cbar ;
  delete[] cbarCalc ;
  delete[] archStatus ;
  delete[] logStatus ;
}

/*
  Test the mode 2 portion of the simplex API.
  Solve an lp by hand
*/
void testSimplexMode2 (const OsiSolverInterface *emptySi, std::string sampleDir)
{ OsiSolverInterface * si = emptySi->clone();
  std::string solverName;
  si->getStrParam(OsiSolverName,solverName);
  std::string fn = sampleDir+"p0033";
  si->readMps(fn.c_str(),"mps");
  si->setObjSense(-1.0);
  si->initialSolve();
  si->setObjSense(1.0);
  // enable special mode
  si->enableSimplexInterface(true);
  // we happen to know that variables are 0-1 and rows are L
  int numberIterations=0;
  int numberColumns = si->getNumCols();
  int numberRows = si->getNumRows();
  double * fakeCost = new double[numberColumns];
  double * duals = new double [numberRows];
  double * djs = new double [numberColumns];
  const double * solution = si->getColSolution();
  memcpy(fakeCost,si->getObjCoefficients(),numberColumns*sizeof(double));
  while (1) {
    const double * dj;
    const double * dual;
    if ((numberIterations&1)==0) {
      // use given ones
      dj = si->getReducedCost();
      dual = si->getRowPrice();
    } else {
      // create
      dj = djs;
      dual = duals;
      si->getReducedGradient(djs,duals,fakeCost);
    }
    int i;
    int colIn=9999;
    int direction=1;
    double best=1.0e-6;
    // find most negative reduced cost
    // Should check basic - but should be okay on this problem
    for (i=0;i<numberRows;i++) {
      double value=dual[i];
      if (value>best) {
      	direction=-1;
      	best=value;
      	colIn=-i-1;
      }
    }
    for (i=0;i<numberColumns;i++) {
      double value=dj[i];
      if (value<-best&&solution[i]<1.0e-6) {
      	direction=1;
      	best=-value;
      	colIn=i;
      } else if (value>best&&solution[i]>1.0-1.0e-6) {
      	direction=-1;
      	best=value;
      	colIn=i;
      }
    }
    if (colIn==9999)
      break; // should be optimal
    int colOut;
    int outStatus;
    double theta;
    OSIUNITTEST_ASSERT_ERROR(!si->primalPivotResult(colIn,direction,colOut,outStatus,theta,NULL), break, solverName, "testSimplexMode2");
    printf("out %d, direction %d theta %g\n",
	   colOut,outStatus,theta);
    numberIterations++;
  }
  delete [] fakeCost;
  delete [] duals;
  delete [] djs;
  // exit special mode
  si->disableSimplexInterface();
  si->resolve();
  OSIUNITTEST_ASSERT_ERROR(!si->getIterationCount(), {}, solverName, "testSimplexMode2: resolve after disable simplex interface");
  si->setObjSense(-1.0);
  si->initialSolve();
  std::cout<<solverName<<" passed OsiSimplexInterface test"<<std::endl;
  delete si;
}

/*
  Test Simplex API mode 1 (tableau access) methods.
*/
void testSimplexMode1 (const OsiSolverInterface *emptySi, std::string sampleDir)
{ OsiSolverInterface * si = emptySi->clone();
  std::string solverName;
  si->getStrParam(OsiSolverName,solverName);
  si->setHintParam(OsiDoReducePrint,true,OsiHintDo) ;
/*
  Read p0033 and check that there's no optimal basis prior to solving.
*/
  std::string fn = sampleDir+"p0033";
  si->readMps(fn.c_str(),"mps");

  OSIUNITTEST_ASSERT_ERROR(!si->basisIsAvailable(),
    if (OsiUnitTest::verbosity >= 1)
      std::cout << "  " << solverName << " shows no optimal basis before initial solve." << std::endl,
    *si, "testSimplexMode1: basis before solve");
/*
  Solve as minimisation problem.
*/
  si->setObjSense(1.0) ;
  si->initialSolve();
  OSIUNITTEST_ASSERT_ERROR(si->isProvenOptimal(), return, *si, "testSimplexMode1: solve p0033");
  if (OsiUnitTest::verbosity >= 1)
  { std::cout
      << "  " << solverName << " solved p0033 z = " << si->getObjValue()
      << "." << std::endl ; }
/*
  Now get tough. Resolve, first as maximisation, then minimisation. Enable the
  tableau interface and check the methods.
*/
  double minmax[] = { -1.0,  1.0 } ;
  for (int ndx = 0 ; ndx < 2 ; ndx++)
  { si->setObjSense(minmax[ndx]) ;
    std::cout
      << "  " << ((minmax[ndx] < 0)?"maximisation ...":"minimisation")
      << " ..." << std::endl ;
    si->resolve() ;
    OSIUNITTEST_ASSERT_ERROR(si->isProvenOptimal(), return, *si, "testSimplexMode1: resolve p0033");
    if (OsiUnitTest::verbosity >= 1)
    { std::cout << "  " << solverName	<< ((si->getObjSense() < 0)?" maximised":" minimised") << " p0033 z = " << si->getObjValue() << "." << std::endl ; }
    OSIUNITTEST_ASSERT_ERROR(si->basisIsAvailable(), {}, *si, "testSimplexMode1: basis available after resolve");
    if (OsiUnitTest::verbosity >= 1 && si->basisIsAvailable())
    { std::cout << "  " << solverName << " shows optimal basis after resolve." << std::endl ; }
/*
  Enable simplex mode 1.
*/
    si->enableFactorization() ;
/*
  Test the various methods.
*/
    testBInvCol(si) ;
    testBInvRow(si) ;
    testBInvACol(si) ;
    testBInvARow(si) ;
    testReducedGradient(si) ;
/*
  Disable simplex mode 1.
*/
    si->disableFactorization() ; }
/*
  Trash this solver and we're finished.
*/
  delete si ;
}

} // end file-local namespace

namespace OsiUnitTest {

/*
  Test a solver's implementation of the OSI simplex API.
*/

void testSimplexAPI (const OsiSolverInterface *emptySi, const std::string &sampleDir)
{ OsiSolverInterface *si = emptySi->clone() ;
  std::string solverName;
  si->getStrParam(OsiSolverName,solverName);
/*
  Do the tests only if the solver implements the simplex API.
*/
  if (si->canDoSimplexInterface() == 0)
  { OSIUNITTEST_ADD_OUTCOME(solverName, "testSimplexAPI", "skipped test", OsiUnitTest::TestOutcome::NOTE, true);
  	std::cout << solverName << " has no OsiSimplex API." << std::endl ;
    return; }
/*
  Test the mode 1 (tableau access) portion of the API.
*/
  if (si->canDoSimplexInterface() >= 1)
  { std::cout << "Testing Simplex API mode 1 for " << solverName << " ... " << std::endl ;
    testSimplexMode1(emptySi,sampleDir) ;
  }
/*
  Test the mode 2 (pivot-by-pivot control) portion of the API.
*/
  if (si->canDoSimplexInterface() >= 2)
  { std::cout << "Testing Simplex API mode 2 for " << solverName << " ... " << std::endl ;
    testSimplexMode2(emptySi,sampleDir) ;
  }
  else
  { OSIUNITTEST_ADD_OUTCOME(solverName, "testSimplexAPI mode 2", "skipped test", OsiUnitTest::TestOutcome::NOTE, true);
  	std::cout << solverName << " does not implement Simplex API mode 2." << std::endl ; }
}

} /* namespace OsiUnitTest */
