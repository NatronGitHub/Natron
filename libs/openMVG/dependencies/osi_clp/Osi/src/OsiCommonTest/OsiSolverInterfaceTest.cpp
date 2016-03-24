// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

/*
		!!  MAINTAINERS  PLEASE  READ  !!

The OSI unit test is gradually undergoing a conversion. Over time, the goal is
to factor the original monolith into separate files and routines. Individual
routines should collect test outcomes in the global OsiUnitTest::outcomes
object using the OSIUNITTEST_* macros defined in OsiUnitTests.hpp.
Ideally, the implementor of an OsiXXX could
indicated expected failures, to avoid the current practice of modifying the
unit test to avoid attempting the test for a particular OsiXXX.

The original approach was to use asserts in tests; the net effect is that the
unit test chokes and dies as soon as something goes wrong. The current
approach is to soldier on until something has gone wrong which makes further
testing pointless if OsiUnitTest::haltonerror is set to 0, to hold and ask the
user for pressing a key if OsiUnitTest::haltonerror is set to 1, and to stop
immediately if OsiUnitTest::haltonerror is set to 2 (but only in case of an error,
not for warnings). The general idea is to return the maximum amount of useful
information with each run. The OsiUnitTest::verbosity variable should be used
to decide on the amount of information to be printed. At level 0, only minimal
output should be printed, at level 1, more information about failed tests
should be printed, while at level 2, also information on passed tests and other
details should be printed.

If you work on this code, please keep these conventions in mind:

  * Tests should be encapsulated in subroutines. If you have a moment, factor
    something out of the main routine --- it'd be nice to get it down under
    500 lines.

  * All local helper routines should be defined in the file-local namespace.

  * This unit test is meant as a certification that OsiXXX correctly implements
    the OSI API specification. Don't step around it!
  
    If OsiXXX is not capable of meeting a particular requirement and you edit
    the unit test code to avoid the test, don't just sweep it under the rug!
    Print a failure message saying the test has been skipped, or something
    else informative.
    
    OsiVol is the worst offender for this (the underlying algorithm is not
    simplex and imposes serious limitations on the type of lp that vol can
    handle). Any simplex-oriented solver should *NOT* be exempted from any
    test. If it's pointless to even try, print a failure message.

  -- lh, 08.01.07, 10.08.26 --
*/

#include "CoinPragma.hpp"
#include "OsiConfig.h"

#include <cstdlib>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdio>

/*
  A utility definition which allows for easy suppression of unused variable
  warnings from GCC. Handy in this environment, where we're constantly def'ing
  things in and out.
*/
#ifndef UNUSED
# if defined(__GNUC__)
#   define UNUSED __attribute__((unused))
# else
#   define UNUSED
# endif
#endif

#include "OsiUnitTests.hpp"
#include "OsiSolverInterface.hpp"
#include "CoinTime.hpp"
#include "CoinFloatEqual.hpp"
#include "CoinPackedVector.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinWarmStartBasis.hpp"
#include "OsiRowCut.hpp"
#include "OsiCuts.hpp"
#include "OsiPresolve.hpp"

/*
  Define helper routines in the file-local namespace.
*/

namespace OsiUnitTest {
extern
void testSimplexAPI(const OsiSolverInterface* emptySi, const std::string& mpsDir);
}

using namespace OsiUnitTest ;

namespace {

//#############################################################################
// A routine to build a CoinPackedMatrix matching the exmip1 example.
//#############################################################################

const CoinPackedMatrix *BuildExmip1Mtx ()
/*
  Simple function to build a packed matrix for the exmip1 example used in
  tests. The function exists solely to hide the intermediate variables.
  Probably could be written as an initialised declaration.
  See COIN/Mps/Sample/exmip1.mps for a human-readable presentation.

  Don't forget to dispose of the matrix when you're done with it.

  Ordered triples seem easiest. They're listed in row-major order.
*/

{ int rowndxs[] = { 0, 0, 0, 0, 0,
		    1, 1,
		    2, 2,
		    3, 3,
		    4, 4, 4 } ;
  int colndxs[] = { 0, 1, 3, 4, 7,
		    1, 2,
		    2, 5,
		    3, 6,
		    0, 4, 7 } ;
  double coeffs[] = { 3.0, 1.0, -2.0, -1.0, -1.0,
		      2.0, 1.1,
		      1.0, 1.0,
		      2.8, -1.2,
		      5.6, 1.0, 1.9 } ;

  CoinPackedMatrix *mtx =
        new CoinPackedMatrix(true,&rowndxs[0],&colndxs[0],&coeffs[0],14) ;

  return (mtx) ; }

//#############################################################################
// Short tests contributed by Vivian DeSmedt. Thanks!
//#############################################################################

/*
  DeSmedt Problem #1

  Initially, max 3*x1 +   x2			x* = [  5 0 ]
		 2*x1 +   x2 <= 10	   row_act = [ 10 5 ]
		   x1 + 3*x2 <= 15

  Test for solver status, expected solution, row activity. Then change
  objective to [1 1], resolve, check again for solver status, expected
  solution [3 4] and expected row activity [10 15].
*/
bool test1VivianDeSmedt(OsiSolverInterface *s)
{
	bool ret = true;

	double inf = s->getInfinity();

	CoinPackedMatrix m;

	m.transpose();

	CoinPackedVector r0;
	r0.insert(0, 2);
	r0.insert(1, 1);
	m.appendRow(r0);

	CoinPackedVector r1;
	r1.insert(0, 1);
	r1.insert(1, 3);
	m.appendRow(r1);

	int numcol = 2;

	double *obj = new double[numcol];
	obj[0] = 3;
	obj[1] = 1;

	double *collb = new double[numcol];
	collb[0] = 0;
	collb[1] = 0;

	double *colub = new double[numcol];
	colub[0] = inf;
	colub[1] = inf;

	int numrow = 2;

	double *rowlb = new double[numrow];
	rowlb[0] = -inf;
	rowlb[1] = -inf;

	double *rowub = new double[numrow];
	rowub[0] = 10;
	rowub[1] = 15;

	s->loadProblem(m, collb, colub, obj, rowlb, rowub);

	delete [] obj;
	delete [] collb;
	delete [] colub;

	delete [] rowlb;
	delete [] rowub;

	s->setObjSense(-1);

	s->initialSolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution1[] = {5, 0};
	ret = ret && equivalentVectors(s,s,0.0001, s->getColSolution(), solution1, 2);

	const double activity1[] = {10, 5};
	ret = ret && equivalentVectors(s,s,0.0001, s->getRowActivity(), activity1, 2);

	s->setObjCoeff(0, 1);
	s->setObjCoeff(1, 1);

	s->resolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution2[] = {3, 4};
	ret = ret && equivalentVectors(s,s,0.0001, s->getColSolution(), solution2, 2);

	const double activity2[] = {10, 15};
	ret = ret && equivalentVectors(s,s,0.0001, s->getRowActivity(), activity2, 2);

	return ret;
}

//--------------------------------------------------------------------------

bool test2VivianDeSmedt(OsiSolverInterface *s)
{
	bool ret = true;

	double inf = s->getInfinity();

	CoinPackedMatrix m;

	m.transpose();

	CoinPackedVector r0;
	r0.insert(0, 2);
	r0.insert(1, 1);
	m.appendRow(r0);

	CoinPackedVector r1;
	r1.insert(0, 1);
	r1.insert(1, 3);
	m.appendRow(r1);

	CoinPackedVector r2;
	r2.insert(0, 1);
	r2.insert(1, 1);
	m.appendRow(r2);

	int numcol = 2;

	double *obj = new double[numcol];
	obj[0] = 3;
	obj[1] = 1;

	double *collb = new double[numcol];
	collb[0] = 0;
	collb[1] = 0;

	double *colub = new double[numcol];
	colub[0] = inf;
	colub[1] = inf;

	int numrow = 3;

	double *rowlb = new double[numrow];
	rowlb[0] = -inf;
	rowlb[1] = -inf;
	rowlb[2] = 1;

	double *rowub = new double[numrow];
	rowub[0] = 10;
	rowub[1] = 15;
	rowub[2] = inf;

	s->loadProblem(m, collb, colub, obj, rowlb, rowub);

	delete [] obj;
	delete [] collb;
	delete [] colub;

	delete [] rowlb;
	delete [] rowub;

	s->setObjSense(-1);

	s->initialSolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution1[] = {5, 0};
	ret = ret && equivalentVectors(s,s,0.0001, s->getColSolution(), solution1, 2);

	const double activity1[] = {10, 5, 5};
	ret = ret && equivalentVectors(s,s,0.0001, s->getRowActivity(), activity1, 3);

	s->setObjCoeff(0, 1);
	s->setObjCoeff(1, 1);

	s->resolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution2[] = {3, 4};
	ret = ret && equivalentVectors(s,s,0.0001, s->getColSolution(), solution2, 2);

	const double activity2[] = {10, 15, 7};
	ret = ret && equivalentVectors(s,s,0.0001, s->getRowActivity(), activity2, 3);

  return ret;
}

//--------------------------------------------------------------------------

bool test3VivianDeSmedt(OsiSolverInterface *s)
{
	bool ret = true;

	//double inf = s->getInfinity();

	CoinPackedVector empty;

	s->addCol(empty, 0, 10, 3);
	s->addCol(empty, 0, 10, 1);

	CoinPackedVector r0;
	r0.insert(0, 2);
	r0.insert(1, 1);
	s->addRow(r0, 0, 10);

	CoinPackedVector r1;
	r1.insert(0, 1);
	r1.insert(1, 3);
	s->addRow(r1, 0, 15);

	s->setObjSense(-1);

	s->writeMps("test");

	s->initialSolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution1[] = {5, 0};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution1, 2);

	const double activity1[] = {10, 5};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity1, 2);

	s->setObjCoeff(0, 1);
	s->setObjCoeff(1, 1);

	s->resolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution2[] = {3, 4};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution2, 2);

	const double activity2[] = {10, 15};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity2, 2);

	return ret;
}

//--------------------------------------------------------------------------

bool test4VivianDeSmedt(OsiSolverInterface *s)
{
	bool ret = true;

	double inf = s->getInfinity();

	CoinPackedVector empty;

	s->addCol(empty, 0, inf, 3);
	s->addCol(empty, 0, inf, 1);

	CoinPackedVector r0;
	r0.insert(0, 2);
	r0.insert(1, 1);
	s->addRow(r0, -inf, 10);

	CoinPackedVector r1;
	r1.insert(0, 1);
	r1.insert(1, 3);
	s->addRow(r1, -inf, 15);

	s->setObjSense(-1);

	s->writeMps("test");

	s->initialSolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution1[] = {5, 0};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution1, 2);

	const double activity1[] = {10, 5};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity1, 2);

	s->setObjCoeff(0, 1);
	s->setObjCoeff(1, 1);

	s->resolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution2[] = {3, 4};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution2, 2);

	const double activity2[] = {10, 15};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity2, 2);

	return ret;
}

//--------------------------------------------------------------------------

/*
  Constructs the system

     max    3x1 +  x2

    -inf <= 2x1 +  x2 <= 10
    -inf <=  x1 + 3x2 <= 15

  The optimal solution is unbounded. Objective is then changed to

     max     x1 +  x2

  which has a bounded optimum at x1 = 3, x2 = 4.
*/

bool test5VivianDeSmedt(OsiSolverInterface *s)
{
	bool ret = true;

	double inf = s->getInfinity();

	CoinPackedVector empty;

	s->addCol(empty, -inf, inf, 3);
	s->addCol(empty, -inf, inf, 1);

	CoinPackedVector r0;
	r0.insert(0, 2);
	r0.insert(1, 1);
	s->addRow(r0, -inf, 10);

	CoinPackedVector r1;
	r1.insert(0, 1);
	r1.insert(1, 3);
	s->addRow(r1, -inf, 15);

	s->setObjSense(-1);

	s->writeMps("test");
        s->initialSolve();

	ret = ret && !s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && s->isProvenDualInfeasible();

	s->setObjCoeff(0, 1);
	s->setObjCoeff(1, 1);

	s->resolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution2[] = {3, 4};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution2, 2);

	const double activity2[] = {10, 15};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity2, 2);

	return ret;
}

//--------------------------------------------------------------------------

bool test6VivianDeSmedt(OsiSolverInterface *s)
{
	bool ret = true;

	double inf = s->getInfinity();

	CoinPackedVector empty;

	s->addCol(empty, 0, inf, 3);
	s->addCol(empty, 0, inf, 1);

	CoinPackedVector r0;
	r0.insert(0, 2);
	r0.insert(1, 1);
	s->addRow(r0, 0, 10);

	CoinPackedVector r1;
	r1.insert(0, 1);
	r1.insert(1, 3);
	s->addRow(r1, 0, 15);

	s->setObjSense(-1);

	s->writeMps("test");

	s->initialSolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution1[] = {5, 0};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution1, 2);

	const double activity1[] = {10, 5};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity1, 2);

	s->setObjCoeff(0, 1);
	s->setObjCoeff(1, 1);

	s->resolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution2[] = {3, 4};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution2, 2);

	const double activity2[] = {10, 15};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity2, 2);

	return ret;
}

//--------------------------------------------------------------------------

bool test7VivianDeSmedt(OsiSolverInterface *s)
{
	bool ret = true;

	double inf = s->getInfinity();

	CoinPackedVector empty;

	s->addCol(empty, 4, inf, 3);
	s->addCol(empty, 3, inf, 1);

	CoinPackedVector r0;
	r0.insert(0, 2);
	r0.insert(1, 1);
	s->addRow(r0, 0, 10);

	CoinPackedVector r1;
	r1.insert(0, 1);
	r1.insert(1, 3);
	s->addRow(r1, 0, 15);

	s->setObjSense(-1);

	s->writeMps("test");

	s->initialSolve();

	ret = ret && !s->isProvenOptimal();
	ret = ret && s->isProvenPrimalInfeasible();

	s->setObjCoeff(0, 1);
	s->setObjCoeff(1, 1);

	s->resolve();

	ret = ret && !s->isProvenOptimal();
	ret = ret && s->isProvenPrimalInfeasible();

	return ret;
}

//--------------------------------------------------------------------------

bool test8VivianDeSmedt(OsiSolverInterface *s)
{
	bool ret = true;

	double inf = s->getInfinity();

	CoinPackedVector empty;

	s->addCol(empty, -inf, inf, 3);
	s->addCol(empty, -inf, inf, 1);

	CoinPackedVector r0;
	r0.insert(0, 2);
	r0.insert(1, 1);
	s->addRow(r0, 0, 10);

	CoinPackedVector r1;
	r1.insert(0, 1);
	r1.insert(1, 3);
	s->addRow(r1, 0, 15);

	s->setObjSense(-1);

	s->writeMps("test");

	s->initialSolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution1[] = {6, -2};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution1, 2);

	const double activity1[] = {10, 0};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity1, 2);

	s->setObjCoeff(0, 1);
	s->setObjCoeff(1, 1);

	s->resolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution2[] = {3, 4};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution2, 2);

	const double activity2[] = {10, 15};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity2, 2);

	return ret;
}

//--------------------------------------------------------------------------

bool test9VivianDeSmedt(OsiSolverInterface *s)
{
	bool ret = true;

	double inf = s->getInfinity();

	CoinPackedVector empty;

	s->addCol(empty, -inf, inf, 3);
	s->addCol(empty, -inf, inf, 1);

	CoinPackedVector r0;
	r0.insert(0, 2);
	r0.insert(1, 1);
	s->addRow(r0, 0, 10);

	CoinPackedVector r1;
	r1.insert(0, 1);
	r1.insert(1, 3);
	s->addRow(r1, 0, 15);

	CoinPackedVector r2;
	r2.insert(0, 1);
	r2.insert(1, 4);
	s->addRow(r2, 12, inf);

	s->setObjSense(-1);

	s->writeMps("test");

	s->initialSolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution1[] = {4, 2};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution1, 2);

	const double activity1[] = {10, 10, 12};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity1, 3);

	s->setObjCoeff(0, 1);
	s->setObjCoeff(1, 1);

	s->resolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution2[] = {3, 4};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution2, 2);

	const double activity2[] = {10, 15, 19};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity2, 3);

	return ret;
}

//--------------------------------------------------------------------------

bool test10VivianDeSmedt(OsiSolverInterface *s)
{
	bool ret = true;

	double inf = s->getInfinity();

	int numcols = 2;
	int numrows = 2;
	const CoinBigIndex start[] = {0, 2, 4};
	const int index[] = {0, 1, 0, 1};
	const double value[] = {4, 1, 2, 3};
	const double collb[] = {0, 0};
	const double colub[] = {inf, inf};
	double obj[] = {3, 1};
	char rowsen[] = {'R', 'R'};
	double rowrhs[] = {20, 15};
	double rowrng[] = {20, 15};

	s->loadProblem(numcols, numrows, start, index, value, collb, colub, obj, rowsen, rowrhs, rowrng);

	s->setObjSense(-1);

	s->writeMps("test");

	s->initialSolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution1[] = {5, 0};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution1, 2);

	const double activity1[] = {20, 5};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity1, 2);

	s->setObjCoeff(0, 1);
	s->setObjCoeff(1, 1);

	s->resolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution2[] = {3, 4};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution2, 2);

	const double activity2[] = {20, 15};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity2, 2);

	return ret;
}

//--------------------------------------------------------------------------

bool test11VivianDeSmedt(OsiSolverInterface *s)
{
	bool ret = true;

	double inf = s->getInfinity();

	int numcols = 2;
	int numrows = 2;
	const CoinBigIndex start[] = {0, 2, 4};
	const int index[] = {0, 1, 0, 1};
	const double value[] = {4, 1, 2, 3};
	const double collb[] = {0, 0};
	const double colub[] = {inf, inf};
	double obj[] = {3, 1};
	double rowlb[] = {0, 0};
	double rowub[] = {20, 15};

	s->loadProblem(numcols, numrows, start, index, value, collb, colub, obj, rowlb, rowub);

	s->setObjSense(-1);

	s->writeMps("test");

	s->initialSolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution1[] = {5, 0};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution1, 2);

	const double activity1[] = {20, 5};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity1, 2);

	s->setObjCoeff(0, 1);
	s->setObjCoeff(1, 1);

	s->resolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution2[] = {3, 4};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution2, 2);

	const double activity2[] = {20, 15};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity2, 2);

	return ret;
}

//--------------------------------------------------------------------------

bool test12VivianDeSmedt(OsiSolverInterface *s)
{
	bool ret = true;

	double inf = s->getInfinity();

	CoinPackedMatrix m;

	m.transpose();

	CoinPackedVector r0;
	r0.insert(0, 4);
	r0.insert(1, 2);
	m.appendRow(r0);

	CoinPackedVector r1;
	r1.insert(0, 1);
	r1.insert(1, 3);
	m.appendRow(r1);

	int numcol = 2;

	double *obj = new double[numcol];
	obj[0] = 3;
	obj[1] = 1;

	double *collb = new double[numcol];
	collb[0] = 0;
	collb[1] = 0;

	double *colub = new double[numcol];
	colub[0] = inf;
	colub[1] = inf;

	int numrow = 2;

	double *rowlb = new double[numrow];
	rowlb[0] = 0;
	rowlb[1] = 0;

	double *rowub = new double[numrow];
	rowub[0] = 20;
	rowub[1] = 15;

	s->loadProblem(m, collb, colub, obj, rowlb, rowub);

	delete [] obj;
	delete [] collb;
	delete [] colub;

	delete [] rowlb;
	delete [] rowub;

	s->setObjSense(-1);

	s->initialSolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution1[] = {5, 0};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution1, 2);

	const double activity1[] = {20, 5};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity1, 2);

	s->setObjCoeff(0, 1);
	s->setObjCoeff(1, 1);

	s->resolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution2[] = {3, 4};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution2, 2);

	const double activity2[] = {20, 15};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity2, 2);

	return ret;
}

//--------------------------------------------------------------------------

bool test13VivianDeSmedt(OsiSolverInterface *s)
{
	bool ret = true;

	double inf = s->getInfinity();

	CoinPackedMatrix m;

	CoinPackedVector c0;
	c0.insert(0, 4);
	c0.insert(1, 1);
	m.appendCol(c0);

	CoinPackedVector c1;
	c1.insert(0, 2);
	c1.insert(1, 3);
	m.appendCol(c1);

	int numcol = 2;

	double *obj = new double[numcol];
	obj[0] = 3;
	obj[1] = 1;

	double *collb = new double[numcol];
	collb[0] = 0;
	collb[1] = 0;

	double *colub = new double[numcol];
	colub[0] = inf;
	colub[1] = inf;

	int numrow = 2;

	double *rowlb = new double[numrow];
	rowlb[0] = 0;
	rowlb[1] = 0;

	double *rowub = new double[numrow];
	rowub[0] = 20;
	rowub[1] = 15;

	s->loadProblem(m, collb, colub, obj, rowlb, rowub);

	delete [] obj;
	delete [] collb;
	delete [] colub;

	delete [] rowlb;
	delete [] rowub;

	s->setObjSense(-1);

	s->initialSolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution1[] = {5, 0};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution1, 2);

	const double activity1[] = {20, 5};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity1, 2);

	s->setObjCoeff(0, 1);
	s->setObjCoeff(1, 1);

	s->resolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution2[] = {3, 4};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution2, 2);

	const double activity2[] = {20, 15};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity2, 2);

	return ret;
}

//--------------------------------------------------------------------------

bool test14VivianDeSmedt(OsiSolverInterface *s)
{
	bool ret = true;

	double inf = s->getInfinity();

	CoinPackedVector empty;

	s->addCol(empty, 0, inf, 3);
	s->addCol(empty, 0, inf, 1);

	CoinPackedVector r0;
	r0.insert(0, 4);
	r0.insert(1, 2);
	s->addRow(r0, 0, 20);

	CoinPackedVector r1;
	r1.insert(0, 1);
	r1.insert(1, 3);
	s->addRow(r1, 0, 15);

	s->setObjSense(-1);

	s->writeMps("test");

	s->initialSolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution1[] = {5, 0};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution1, 2);

	const double activity1[] = {20, 5};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity1, 2);

	s->setObjCoeff(0, 1);
	s->setObjCoeff(1, 1);

	s->resolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution2[] = {3, 4};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution2, 2);

	const double activity2[] = {20, 15};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity2, 2);

	return ret;
}

//--------------------------------------------------------------------------

bool test15VivianDeSmedt(OsiSolverInterface *s)
{
	bool ret = true;

	double inf = s->getInfinity();

	CoinPackedVector empty;

	s->addRow(empty, 0, 20);
	s->addRow(empty, 0, 15);

	CoinPackedVector c0;
	c0.insert(0, 4);
	c0.insert(1, 1);
	s->addCol(c0, 0, inf, 3);

	CoinPackedVector c1;
	c1.insert(0, 2);
	c1.insert(1, 3);
	s->addCol(c1, 0, inf, 1);

	s->setObjSense(-1);

	s->writeMps("test");

	s->initialSolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution1[] = {5, 0};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution1, 2);

	const double activity1[] = {20, 5};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity1, 2);

	s->setObjCoeff(0, 1);
	s->setObjCoeff(1, 1);

	s->resolve();

	ret = ret && s->isProvenOptimal();
	ret = ret && !s->isProvenPrimalInfeasible();
	ret = ret && !s->isProvenDualInfeasible();

	const double solution2[] = {3, 4};
	ret = ret && equivalentVectors(s,s,0.0001,s->getColSolution(), solution2, 2);

	const double activity2[] = {20, 15};
	ret = ret && equivalentVectors(s,s,0.0001,s->getRowActivity(), activity2, 2);

	return ret;
}

/*
  Another test case submitted by Vivian De Smedt.  The test is to modify the
  objective function and check that the solver's optimum point tracks
  correctly. The initial problem is

  max  3*x1 +   x2

  s.t. 2*x1 +   x2 <= 10
	 x1 + 3*x2 <= 15

  with optimum z* = 15 at (x1,x2) = (5,0). Then the objective is changed to
  x1 + x2, with new optimum z* = 7 at (3,4).

  The volume algorithm doesn't return exact solution values, so relax the
  test for correctness when we're checking the solution.
*/

void changeObjAndResolve (const OsiSolverInterface *emptySi)

{ OsiSolverInterface *s = emptySi->clone() ;
  double dEmpty = 0 ;
  int iEmpty = 0 ;
  CoinBigIndex iEmpty2 = 0 ;

/*
  Establish an empty problem. Establish empty columns with bounds and objective
  coefficient only. Finally, insert constraint coefficients and set for
  maximisation.
*/
  s->loadProblem(0,0,&iEmpty2,&iEmpty,&dEmpty,
		 &dEmpty,&dEmpty,&dEmpty,&dEmpty,&dEmpty) ;

  CoinPackedVector c ;
  s->addCol(c,0,10,3) ;
  s->addCol(c,0,10,1) ;

  double inf = s->getInfinity() ;
  CoinPackedVector r1 ;
  r1.insert(0,2) ;
  r1.insert(1,1) ;
  s->addRow(r1,-inf,10) ;

  r1.clear() ;
  r1.insert(0,1) ;
  r1.insert(1,3) ;
  s->addRow(r1,-inf,15) ;

  s->setObjSense(-1) ;
/*
  Optimise for 3*x1 + x2 and check for correctness.
*/
  s->initialSolve() ;

  const double *colSol = s->getColSolution() ;
  OSIUNITTEST_ASSERT_ERROR(colSol[0] >= 4.5, {}, *s, "changeObjAndResolve");
  OSIUNITTEST_ASSERT_ERROR(colSol[1] <= 0.5, {}, *s, "changeObjAndResolve");
/*
  Set objective to x1 + x2 and reoptimise.
*/
  s->setObjCoeff(0,1) ;
  s->setObjCoeff(1,1) ;

  s->resolve() ;

  colSol = s->getColSolution() ;
  OSIUNITTEST_ASSERT_ERROR(colSol[0] >= 2.3 && colSol[0] <= 3.7, {}, *s, "changeObjAndResolve");
  OSIUNITTEST_ASSERT_ERROR(colSol[1] >= 3.5 && colSol[1] <= 4.5, {}, *s, "changeObjAndResolve");

  delete s ;
}

/*
  This code is taken from some bug reports of Sebastian Nowozin. It
  demonstrates some issues he had with OsiClp.

  https://projects.coin-or.org/Osi/ticket/54
  https://projects.coin-or.org/Osi/ticket/55
  https://projects.coin-or.org/Osi/ticket/56
  https://projects.coin-or.org/Osi/ticket/58

  The short summary is that enabling/disabling the level 2 simplex interface
  (controllable pivoting) with an empty constraint matrix caused problems.
  For solvers that don't support simplex level 2, all we're testing is
  constraint system mods and resolve.

  Query (lh): The original comments with this code referred to use of Binv to
  generate cuts Binv et al. are level 1 simplex interface routines. Perhaps
  the test should use level 1.
*/

bool test16SebastianNowozin(OsiSolverInterface *si)

{ CoinAbsFltEq fltEq ;

  CoinPackedMatrix* matrix = new CoinPackedMatrix(false,0,0) ;
  matrix->setDimensions(0,4) ;

  double objective[] = { 0.1, 0.2, -0.1, -0.2 } ;
  double varLB[] = { 0.0, 0.0, 0.0, 0.0 } ;
  double varUB[] = { 1.0, 1.0, 1.0, 1.0 } ;

  si->loadProblem(*matrix, varLB, varUB, objective, NULL, NULL) ;
  delete matrix ;

/*
  Set objective sense prior to objective, just to catch the unwary.
*/
  si->setObjSense(1) ;
  si->setObjective(objective) ;
/*
  The code provided with ticket 54 is illegal --- this first call cannot be
  resolve(). The first solve must be an initialSolve, for the benefit of
  solvers which use it for initialisation.  -- lh, 080903 --
*/
  si->initialSolve() ;
  OSIUNITTEST_ASSERT_ERROR(si->isProvenOptimal(), return false, *si, "test16SebastianNowozin initial solve");
  OSIUNITTEST_ASSERT_ERROR(fltEq(si->getObjValue(),-0.3), return false, *si, "test16SebastianNowozin initial solve");
/*
  Expected: primal = [ 0 0 1 1 ]
*/
  OSIUNITTEST_ASSERT_ERROR(si->getColSolution() != NULL, return false, *si, "test16SebastianNowozin initial solve");
/*
  Simulate a constraint generation interval that will require the simplex
  interface.  Enable, then disable level 2 simplex interface (controllable
  pivoting), if the solver has it.
*/
  if (si->canDoSimplexInterface() >= 2)
  { OSIUNITTEST_CATCH_ERROR(si->enableFactorization(),        return false, *si, "test16SebastianNowozin initial solve");
  	OSIUNITTEST_CATCH_ERROR(si->enableSimplexInterface(true), return false, *si, "test16SebastianNowozin initial solve");
//  	try
//    { si->enableFactorization() ;
//      si->enableSimplexInterface(true) ; }
//    catch (CoinError e)
//    { std::string errmsg ;
//      errmsg = "first enableFactorization or enableSimplexInterface" ;
//      errmsg = errmsg + " threw CoinError: " + e.message() ;
//      OSIUNITTEST_ADD_OUTCOME(*si, "test16SebastianNowozin first enableFactorization or enableSimplexInterface", errmsg.c_str(), TestOutcome::ERROR, false);
//      failureMessage(*si,errmsg) ;
//      return (false) ; }
//    OSIUNITTEST_ADD_OUTCOME(*si, "test16SebastianNowozin first enableFactorization or enableSimplexInterface", "no exception", TestOutcome::PASSED, false);
    // (...) constraint generation here
    si->disableFactorization() ; }

/*
  Add two constraints and resolve
*/
  CoinPackedVector row1 ;	// x_2 + x_3 - x_0 <= 0
  row1.insert(0,-1.0) ;
  row1.insert(2,1.0) ;
  row1.insert(3,1.0) ;
  si->addRow(row1,-si->getInfinity(),0.0) ;

  CoinPackedVector row2 ;	// x_0 + x_1 - x_3 <= 0
  row2.insert(0,1.0) ;
  row2.insert(1,1.0) ;
  row2.insert(3,-1.0) ;
  si->addRow(row2,-si->getInfinity(),0.0) ;

  si->resolve() ;
  OSIUNITTEST_ASSERT_ERROR(si->isProvenOptimal(),         return false, *si, "test16SebastianNowozin first resolve");
  OSIUNITTEST_ASSERT_ERROR(fltEq(si->getObjValue(),-0.1), return false, *si, "test16SebastianNowozin first resolve");
/*
  Expected: primal = [ 1 0 0 1 ]
*/
  OSIUNITTEST_ASSERT_ERROR(si->getColSolution() != NULL, return false, *si, "test16SebastianNowozin first resolve");
/*
  Simulate another constraint generation run.
*/
  if (si->canDoSimplexInterface() >= 2)
  { OSIUNITTEST_CATCH_ERROR(si->enableFactorization(),        return false, *si, "test16SebastianNowozin first resolve");
	  OSIUNITTEST_CATCH_ERROR(si->enableSimplexInterface(true), return false, *si, "test16SebastianNowozin first resolve");
//	try
//    { si->enableFactorization() ;
//      si->enableSimplexInterface(true) ; }
//    catch (CoinError e)
//    { std::string errmsg ;
//      errmsg = "second enableFactorization or enableSimplexInterface" ;
//      errmsg = errmsg + " threw CoinError: " + e.message() ;
//      failureMessage(*si,errmsg) ;
//      OSIUNITTEST_ADD_OUTCOME(*si, "test16SebastianNowozin second enableFactorization or enableSimplexInterface", errmsg.c_str(), TestOutcome::ERROR, false);
//      return (false) ; }
//    OSIUNITTEST_ADD_OUTCOME(*si, "test16SebastianNowozin second enableFactorization or enableSimplexInterface", "no exception", TestOutcome::PASSED, false);
    // (...) constraint generation here
    si->disableFactorization() ; }
/*
  Remove a constraint, add .15 to the objective coefficients, and resolve.
*/
  int rows_to_delete_arr[] = { 0 } ;
  si->deleteRows(1,rows_to_delete_arr) ;

  std::transform(objective,objective+4,objective,
		 std::bind2nd(std::plus<double>(),0.15)) ;
  si->setObjective(objective) ;
  si->resolve() ;
  OSIUNITTEST_ASSERT_ERROR(si->isProvenOptimal(),          return false, *si, "test16SebastianNowozin second resolve");
  OSIUNITTEST_ASSERT_ERROR(fltEq(si->getObjValue(),-0.05), return false, *si, "test16SebastianNowozin second resolve");
/*
  Expected: obj = [ .25 .35 .05 -.05], primal = [ 0 0 0 1 ]
*/
  OSIUNITTEST_ASSERT_ERROR(si->getColSolution() != NULL, return false, *si, "test16SebastianNowozin second resolve");

  return true;
}


/*
  This code checks an issue reported by Sebastian Nowozin in OsiClp ticket
  57.  He said that OsiClpSolverInterface::getReducedGradient() requires a
  prior call to both enableSimplexInterface(true) and enableFactorization(),
  but only checks/asserts the simplex interface.

  Same comment as test16 --- would simplex level 1 suffice?
*/

bool test17SebastianNowozin(OsiSolverInterface *si)

{ if (si->canDoSimplexInterface() < 2)
  { return (true) ; }

  CoinPackedMatrix *matrix = new CoinPackedMatrix(false,0,0) ;
  matrix->setDimensions(0,4) ;

  double objective[] = { 0.1, 0.2, -0.1, -0.2, } ;
  double varLB[] = { 0.0, 0.0, 0.0, 0.0, } ;
  double varUB[] = { 1.0, 1.0, 1.0, 1.0, } ;

  si->loadProblem(*matrix,varLB,varUB,objective,NULL,NULL) ;
  si->setObjSense(1) ;
	
  delete matrix ;

  CoinPackedVector row1 ;	// x_2 + x_3 - x_0 <= 0
  row1.insert(0,-1.0) ;
  row1.insert(2,1.0) ;
  row1.insert(3,1.0) ;
  si->addRow(row1,-si->getInfinity(),0.0) ;

  si->initialSolve() ;
  OSIUNITTEST_ASSERT_ERROR(si->isProvenOptimal(), return false, *si, "test17SebastianNowozin");
  if (!si->isProvenOptimal())
  	return false;
/*
  Unlike test16, here we do not call si->enableFactorization() first.
*/
 	OSIUNITTEST_CATCH_ERROR(si->enableSimplexInterface(true), return false, *si, "test17SebastianNowozin");
/*
  Now check that getReducedGradient works.
*/
 	double dummy[4] = { 1., 1., 1., 1.} ;
 	OSIUNITTEST_CATCH_ERROR(si->getReducedGradient(dummy,dummy,dummy), return false, *si, "test17SebastianNowozin");

  return true; }


//#############################################################################
// Routines to test various feature groups
//#############################################################################

/*! \brief Test row and column name manipulation

  emptySi should be an empty solver interface, fn the path to the exmpi1
  example.
*/

void testNames (const OsiSolverInterface *emptySi, std::string fn)
{
  bool recognisesOsiNames = true ;
  bool ok ;

  OsiSolverInterface *si = emptySi->clone() ;

  std::string exmip1ObjName = "OBJ" ;
  OsiSolverInterface::OsiNameVec exmip1RowNames(0) ;
  exmip1RowNames.push_back("ROW01") ;
  exmip1RowNames.push_back("ROW02") ;
  exmip1RowNames.push_back("ROW03") ;
  exmip1RowNames.push_back("ROW04") ;
  exmip1RowNames.push_back("ROW05") ;
  OsiSolverInterface::OsiNameVec exmip1ColNames(0) ;
  exmip1ColNames.push_back("COL01") ;
  exmip1ColNames.push_back("COL02") ;
  exmip1ColNames.push_back("COL03") ;
  exmip1ColNames.push_back("COL04") ;
  exmip1ColNames.push_back("COL05") ;
  exmip1ColNames.push_back("COL06") ;
  exmip1ColNames.push_back("COL07") ;
  exmip1ColNames.push_back("COL08") ;

  testingMessage("Testing row/column name handling ...") ;
/*
  Try to get the solver name, but don't immediately abort.
*/
  std::string solverName = "Unknown solver" ;
  OSIUNITTEST_ASSERT_WARNING(si->getStrParam(OsiSolverName,solverName) == true, {}, solverName, "testNames: getStrParam(OsiSolverName)");
/*
  Checking default names. dfltRowColName is pretty liberal about indices, but
  they should never be negative. Since default row/column names are a letter
  plus n digits, asking for a length of 5 on the objective gets you a leading
  'O' plus five more letters.
*/
  std::string dfltName = si->dfltRowColName('o',0,5) ;
  std::string expName = "OBJECT" ;
  OSIUNITTEST_ASSERT_WARNING(dfltName == expName, {}, solverName, "testNames: default objective name");

  dfltName = si->dfltRowColName('r',-1,5) ;
  expName = "!!invalid Row -1!!" ;
  OSIUNITTEST_ASSERT_WARNING(dfltName == expName, {}, solverName, "testNames: default name for invalid row");

  dfltName = si->dfltRowColName('c',-1,5) ;
  expName = "!!invalid Col -1!!" ;
  OSIUNITTEST_ASSERT_WARNING(dfltName == expName, {}, solverName, "testNames: default name for invalid col");
/*
  Start by telling the SI to use lazy names and see if it comes up with
  the right names from the MPS file. There's no point in proceeding further
  if we can't read an MPS file.
*/
  // std::cout << "Testing lazy names from MPS input file." << std::endl ;
  OSIUNITTEST_ASSERT_SEVERITY_EXPECTED(si->setIntParam(OsiNameDiscipline, 1) == true, recognisesOsiNames = false, solverName, "testNames: switch to lazy names", TestOutcome::NOTE, false);

  OSIUNITTEST_ASSERT_ERROR(si->readMps(fn.c_str(),"mps") == 0, delete si; return, solverName, "testNames: read MPS");

  OsiSolverInterface::OsiNameVec rowNames ;
  int rowNameCnt ;
  OsiSolverInterface::OsiNameVec colNames ;
  int colNameCnt ;

  int m = si->getNumRows() ;

  if (recognisesOsiNames)
  { std::string objName = si->getObjName() ;
    OSIUNITTEST_ASSERT_WARNING(objName == exmip1ObjName, {}, solverName, "testNames lazy names: objective name");
    OSIUNITTEST_ASSERT_WARNING(si->getRowName(m) == exmip1ObjName, {}, solverName, "testNames lazy names: name of one after last row is objective name");

    rowNames = si->getRowNames() ;
    rowNameCnt = static_cast<int>(rowNames.size()) ;
    OSIUNITTEST_ASSERT_WARNING(rowNameCnt == static_cast<int>(exmip1RowNames.size()), {}, solverName, "testNames lazy names: row names count");
    ok = true ;
    for (int i = 0 ; i < rowNameCnt ; i++)
    { if (rowNames[i] != exmip1RowNames[i])
      { ok = false ;
        std::cout << "ERROR! " ;
        std::cout << "Row " << i << " is \"" << rowNames[i] << "\" expected \"" << exmip1RowNames[i] << "\"." << std::endl; } }
    OSIUNITTEST_ASSERT_WARNING(ok == true, {}, solverName, "testNames lazy names: row names");

    colNames = si->getColNames() ;
    colNameCnt = static_cast<int>(colNames.size()) ;
    OSIUNITTEST_ASSERT_WARNING(colNameCnt == static_cast<int>(exmip1ColNames.size()), {}, solverName, "testNames lazy names: column names count");
    ok = true ;
    for (int j = 0 ; j < colNameCnt ; j++)
    { if (colNames[j] != exmip1ColNames[j])
      { ok = false ;
        std::cout << "ERROR! " ;
        std::cout << "Column " << j << " is " << colNames[j] << "\" expected \"" << exmip1ColNames[j] << "\"." << std::endl ; } }
    OSIUNITTEST_ASSERT_WARNING(ok == true, {}, solverName, "testNames lazy names: column names");
/*
  Switch back to name discipline 0. We should revert to default names. Failure
  to switch back to discipline 0 after successfully switching to discipline 1
  is some sort of internal confusion in the Osi; abort the test.
*/
    // std::cout << "Switching to no names (aka default names)." << std::endl ;
    OSIUNITTEST_ASSERT_WARNING(si->setIntParam(OsiNameDiscipline, 0) == true, delete si; return, solverName, "testNames: switch to no names");
  }
/*
  This block of tests for default names should pass even if the underlying
  Osi doesn't recognise OsiNameDiscipline. When using default names, name
  vectors are not necessary, hence should have size zero.
*/
  rowNames = si->getRowNames() ;
  OSIUNITTEST_ASSERT_WARNING(rowNames.size() == 0, {}, solverName, "testNames no names: row names count");
  ok = true ;
  for (int i = 0 ; i < m ; i++)
  { if (si->getRowName(i) != si->dfltRowColName('r',i))
    { ok = false ;
      std::cout << "ERROR! " ;
      std::cout
      << "Row " << i << " is \"" << si->getRowName(i)
      << "\" expected \"" << si->dfltRowColName('r',i)
      << "\"." << std::endl ; } }
  OSIUNITTEST_ASSERT_WARNING(ok == true, {}, solverName, "testNames no names: row names");

  colNames = si->getColNames() ;
  OSIUNITTEST_ASSERT_WARNING(colNames.size() == 0, {}, solverName, "testNames no names: column names count");
  int n = si->getNumCols() ;
  ok = true ;
  for (int j = 0 ; j < n ; j++)
  { if (si->getColName(j) != si->dfltRowColName('c',j))
    { ok = false ;
      std::cout << "ERROR! " ;
      std::cout
      << "Column " << j << " is \"" << si->getColName(j)
      << "\" expected \"" << si->dfltRowColName('c',j)
      << "\"." << std::endl ; } }
  OSIUNITTEST_ASSERT_WARNING(ok == true, {}, solverName, "testNames no names: column names");
/*
  This is as much as we can ask if the underlying solver doesn't recognise
  OsiNameDiscipline. Return if that's the case.
*/
  if (!recognisesOsiNames)
  { delete si ;
    return; }
/*
  Switch back to lazy names. The previous names should again be available.
*/
  // std::cout << "Switching back to lazy names." << std::endl ;
  OSIUNITTEST_ASSERT_WARNING(si->setIntParam(OsiNameDiscipline,1) == true, delete si; return, solverName, "testNames: switch to lazy names");
  rowNames = si->getRowNames() ;
  rowNameCnt = static_cast<int>(rowNames.size()) ;
  OSIUNITTEST_ASSERT_WARNING(rowNameCnt == static_cast<int>(exmip1RowNames.size()), {}, solverName, "testNames lazy names: row names count");
  ok = true ;
  for (int i = 0 ; i < rowNameCnt ; i++)
  { if (rowNames[i] != exmip1RowNames[i])
    { ok = false ;
      std::cout << "ERROR! " ;
      std::cout << "Row " << i << " is \"" << rowNames[i] << "\" expected \"" << exmip1RowNames[i] << "\"." << std::endl ; } }
  OSIUNITTEST_ASSERT_WARNING(ok == true, {}, solverName, "testNames lazy names: row names");

  colNames = si->getColNames() ;
  colNameCnt = static_cast<int>(colNames.size()) ;
  OSIUNITTEST_ASSERT_WARNING(colNameCnt == static_cast<int>(exmip1ColNames.size()), {}, solverName, "testNames lazy names: column names count");
  ok = true ;
  for (int j = 0 ; j < colNameCnt ; j++)
  { if (colNames[j] != exmip1ColNames[j])
    { ok = false ;
      std::cout << "ERROR! " ;
      std::cout << "Column " << j << " is " << colNames[j] << "\" expected \"" << exmip1ColNames[j] << "\"." << std::endl ; } }
  OSIUNITTEST_ASSERT_WARNING(ok == true, {}, solverName, "testNames lazy names: column names");
/*
  Add a row. We should see no increase in the size of the row name vector,
  and asking for the name of the new row should return a default name.
*/
  int nels = 5 ;
  int indices[5] = { 0, 2, 3, 5, 7 } ;
  double els[5] = { 1.0, 3.0, 4.0, 5.0, 42.0 } ;
  CoinPackedVector newRow(nels,indices,els) ;
  si->addRow(newRow,-4.2, .42) ;
  OSIUNITTEST_ASSERT_WARNING(si->getNumRows() == m+1, delete si; return, solverName, "testNames lazy names: added a row");
  rowNames = si->getRowNames() ;
  rowNameCnt = static_cast<int>(rowNames.size()) ;
  OSIUNITTEST_ASSERT_WARNING(rowNameCnt == m, {}, solverName, "testNames lazy names: row names count after row addition");
  OSIUNITTEST_ASSERT_WARNING(si->getRowName(m) == si->dfltRowColName('r',m), {}, solverName, "testNames lazy names: default name for added row");
/*
  Now set a name for the row.
*/
  std::string newRowName = "NewRow" ;
  si->setRowName(m,newRowName) ;
  OSIUNITTEST_ASSERT_WARNING(si->getRowName(m) == newRowName, {}, solverName, "testNames lazy names: setting new row name");
/*
  Ok, who are we really talking with? Delete row 0 and see if the names
  change appropriately. Since deleteRows is pure virtual, the names will
  change only if the underlying OsiXXX supports names (i.e., it must make
  a call to deleteRowNames).
*/
  // std::cout << "Testing row deletion." << std::endl ;
  si->deleteRows(1,indices) ;
  rowNames = si->getRowNames() ;
  rowNameCnt = static_cast<int>(rowNames.size()) ;
  OSIUNITTEST_ASSERT_WARNING(rowNameCnt == m, {}, solverName, "testNames lazy names: row names count after deleting row");
  ok = true ;
  for (int i = 0 ; i < rowNameCnt ; i++)
  { std::string expected ;
    if (i != m-1)
    { expected = exmip1RowNames[i+1] ; }
    else
    { expected = newRowName ; }
    if (rowNames[i] != expected)
    { ok = false ;
      std::cout << "ERROR! " ;
      std::cout << "Row " << i << " is \"" << rowNames[i] << "\" expected \"" << expected << "\"." << std::endl ; } }
  OSIUNITTEST_ASSERT_WARNING(ok == true, {}, solverName, "testNames lazy names: row names after deleting row");

/*
  Add/delete a column and do the same tests. Expected results as above.
*/
  nels = 3 ;
  indices[0] = 0 ;
  indices[1] = 2 ;
  indices[2] = 4 ;
  els[0] = 1.0 ;
  els[1] = 4.0 ;
  els[2] = 24.0 ;
  CoinPackedVector newCol(nels,indices,els) ;
  si->addCol(newCol,-4.2, .42, 42.0) ;
  OSIUNITTEST_ASSERT_ERROR(si->getNumCols() == n+1, delete si; return, solverName, "testNames lazy names: columns count after adding column");
  colNames = si->getColNames() ;
  colNameCnt = static_cast<int>(colNames.size()) ;
  OSIUNITTEST_ASSERT_WARNING(colNameCnt == n, {}, solverName, "testNames lazy names: columns names count after adding column");
  OSIUNITTEST_ASSERT_WARNING(si->getColName(n) == si->dfltRowColName('c',n), {}, solverName, "testNames lazy names default column name after adding column");
  std::string newColName = "NewCol" ;
  si->setColName(n,newColName) ;
  OSIUNITTEST_ASSERT_WARNING(si->getColName(n) == newColName, {}, solverName, "testNames lazy names: setting column name");
  // std::cout << "Testing column deletion." << std::endl ;
  si->deleteCols(1,indices) ;
  colNames = si->getColNames() ;
  colNameCnt = static_cast<int>(colNames.size()) ;
  OSIUNITTEST_ASSERT_WARNING(colNameCnt == n, {}, solverName, "testNames lazy names: column names count after deleting column");
  ok = true ;
  for (int j = 0 ; j < colNameCnt ; j++)
  { std::string expected ;
    if (j != n-1)
    { expected = exmip1ColNames[j+1] ; }
    else
    { expected = newColName ; }
    if (colNames[j] != expected)
    { ok = false ;
      std::cout << "ERROR! " ;
      std::cout << "Column " << j << " is \"" << colNames[j] << "\" expected \"" << expected << "\"." << std::endl ; } }
  OSIUNITTEST_ASSERT_WARNING(ok == true, {}, solverName, "testNames lazy names: column names after deleting column");
/*
  Interchange row and column names.
*/
  // std::cout << "Testing bulk replacement of names." << std::endl ;
  si->setRowNames(exmip1ColNames,0,3,2) ;
  rowNames = si->getRowNames() ;
  rowNameCnt = static_cast<int>(rowNames.size()) ;
  OSIUNITTEST_ASSERT_WARNING(rowNameCnt == m, {}, solverName, "testNames lazy names: row names count after bulk replace");
  ok = true ;
  for (int i = 0 ; i < rowNameCnt ; i++)
  { std::string expected ;
    if (i < 2)
    { expected = exmip1RowNames[i+1] ; }
    else
    if (i >= 2 && i <= 4)
    { expected = exmip1ColNames[i-2] ; }
    else
    { expected = newRowName ; }
    if (rowNames[i] != expected)
    { ok = false ;
      std::cout << "ERROR! " ;
      std::cout << "Row " << i << " is \"" << rowNames[i]	<< "\" expected \"" << expected << "\"." << std::endl ; } }
  OSIUNITTEST_ASSERT_WARNING(ok == true, {}, solverName, "testNames lazy names: row names after bulk replace");

  si->setColNames(exmip1RowNames,3,2,0) ;
  colNames = si->getColNames() ;
  colNameCnt = static_cast<int>(colNames.size()) ;
  OSIUNITTEST_ASSERT_WARNING(colNameCnt == n, {}, solverName, "testNames lazy names: column names count after bulk replace");
  ok = true ;
  for (int j = 0 ; j < colNameCnt ; j++)
  { std::string expected ;
    if (j < 2)
    { expected = exmip1RowNames[j+3] ; }
    else
    if (j >= 2 && j <= 6)
    { expected = exmip1ColNames[j+1] ; }
    else
    { expected = newColName ; }
    if (colNames[j] != expected)
    { ok = false ;
      std::cout << "ERROR! " ;
      std::cout << "Column " << j << " is \"" << colNames[j] << "\" expected \"" << expected << "\"." << std::endl ; } }
  OSIUNITTEST_ASSERT_WARNING(ok == true, {}, solverName, "testNames lazy names: column names after bulk replace");
/*
  Delete a few row and column names (directly, as opposed to deleting rows or
  columns). Names should shift downward.
*/
  // std::cout << "Testing name deletion." << std::endl ;
  si->deleteRowNames(0,2) ;
  rowNames = si->getRowNames() ;
  rowNameCnt = static_cast<int>(rowNames.size()) ;
  OSIUNITTEST_ASSERT_WARNING(rowNameCnt == m-2, {}, solverName, "testNames lazy names: row names count after deleting 2 rows");
  ok = true ;
  for (int i = 0 ; i < rowNameCnt ; i++)
  { std::string expected ;
    if (i < rowNameCnt)
    { expected = exmip1ColNames[i] ; }
    if (rowNames[i] != expected)
    { ok = false ;
      std::cout << "ERROR! " ;
      std::cout << "Row " << i << " is \"" << rowNames[i] << "\" expected \"" << expected << "\"." << std::endl ; } }
  OSIUNITTEST_ASSERT_WARNING(ok == true, {}, solverName, "testNames lazy names: row names after deleting 2 rows");

  si->deleteColNames(5,3) ;
  colNames = si->getColNames() ;
  colNameCnt = static_cast<int>(colNames.size()) ;
  OSIUNITTEST_ASSERT_WARNING(colNameCnt == n-3, {}, solverName, "testNames lazy names: column names count after deleting 3 columns");
  ok = true ;
  for (int j = 0 ; j < colNameCnt ; j++)
  { std::string expected ;
    if (j < 2)
    { expected = exmip1RowNames[j+3] ; }
    else
    if (j >= 2 && j < colNameCnt)
    { expected = exmip1ColNames[j+1] ; }
    if (colNames[j] != expected)
    { ok = false ;
      std::cout << "ERROR! " ;
      std::cout << "Column " << j << " is \"" << colNames[j] << "\" expected \"" << expected << "\"." << std::endl ; } }
  OSIUNITTEST_ASSERT_WARNING(ok == true, {}, solverName, "testNames lazy names: column names after deleting 3 columns");
/*
  Finally, switch to full names, and make sure we retrieve full length
  vectors.
*/
  // std::cout << "Switching to full names." << std::endl ;
  OSIUNITTEST_ASSERT_WARNING(si->setIntParam(OsiNameDiscipline,2), delete si; return, solverName, "testNames lazy names: change name discipline");
  m = si->getNumRows() ;
  rowNames = si->getRowNames() ;
  rowNameCnt = static_cast<int>(rowNames.size()) ;
  OSIUNITTEST_ASSERT_WARNING(rowNameCnt == m+1, {}, solverName, "testNames full names: row names count");
  OSIUNITTEST_ASSERT_WARNING(rowNames[m] == exmip1ObjName, {}, solverName, "testNames full names: objective name");
  ok = true ;
  for (int i = 0 ; i < rowNameCnt-1 ; i++)
  { std::string expected ;
    if (i < 3)
    { expected = exmip1ColNames[i] ; }
    else
    { expected = si->dfltRowColName('r',i) ; }
    if (rowNames[i] != expected)
    { ok = false ;
      std::cout << "ERROR! " ;
      std::cout << "Row " << i << " is \"" << rowNames[i] << "\" expected \"" << expected << "\"." << std::endl ; } }
  OSIUNITTEST_ASSERT_WARNING(ok == true, {}, solverName, "testNames full names: row names");

  n = si->getNumCols() ;
  colNames = si->getColNames() ;
  colNameCnt = static_cast<int>(colNames.size()) ;
  OSIUNITTEST_ASSERT_WARNING(colNameCnt == n, {}, solverName, "testNames full names: column names count");
  ok = true ;
  for (int j = 0 ; j < colNameCnt ; j++)
  { std::string expected ;
    if (j < 2)
    { expected = exmip1RowNames[j+3] ; }
    else
    if (j >= 2 && j <= 4)
    { expected = exmip1ColNames[j+1] ; }
    else
    { expected = si->dfltRowColName('c',j) ; }
    if (colNames[j] != expected)
    { ok = false ;
      std::cout << "ERROR! " ;
      std::cout << "Column " << j << " is " << colNames[j] << "\" expected \"" << expected << "\"." << std::endl ; } }
  OSIUNITTEST_ASSERT_WARNING(ok == true, {}, solverName, "testNames full names: column names");

  delete si ;
}

//--------------------------------------------------------------------------

/*! \brief Tests for a solution imposed by the user.

  Checks the routines setColSolution (primal variables) and setRowSolution
  (dual variables). Goes on to check that getReducedCost and getRowActivity
  use the imposed solution.

  The prototype OSI supplied as the parameter should be loaded with a smallish
  problem.
*/
void testSettingSolutions (OsiSolverInterface &proto)

{ OsiSolverInterface *si = proto.clone() ;
  bool allOK = true ;
  int i ;
  int m = si->getNumRows() ;
  int n = si->getNumCols() ;
  double mval,cval,rval ;
  const double *rowVec,*colVec,*objVec ;
  double *colShouldBe = new double [m] ;
  double *rowShouldBe = new double [n] ;

  CoinAbsFltEq fltEq ;

  testingMessage("Checking that solver can set row and column solutions ...") ;

/*
  Create dummy solution vectors.
*/
  double *dummyColSol = new double[n] ;
  for (i = 0 ; i < n ; i++)
  { dummyColSol[i] = i + .5 ; }

  double *dummyRowSol = new double[m] ;
  for (i = 0 ; i < m ; i++ )
  { dummyRowSol[i] = i - .5 ; }

/*
  First the values we can set directly: primal (column) and dual (row)
  solutions. The osi should copy the vector, hence the pointer we get back
  should not be the pointer we supply. But it's reasonable to expect exact
  equality, as no arithmetic should be performed.
*/
  si->setColSolution(dummyColSol) ;
  OSIUNITTEST_ASSERT_ERROR(dummyColSol != si->getColSolution(), allOK = false, *si, "setting solutions: solver should not return original pointer");
  rowVec = si->getColSolution() ;

  bool ok = true ;
  for (i = 0 ; i < n ;  i++)
  { mval = rowVec[i] ;
    rval = dummyColSol[i] ;
    if (mval != rval)
    { ok = false ;
      std::cout
        << "x<" << i << "> = " << mval
        << ", expecting " << rval
        << ", |error| = " << (mval-rval)
        << "." << std::endl ; } }
  OSIUNITTEST_ASSERT_ERROR(ok == true, allOK = false, *si, "setting solutions: solver stored column solution correctly");

  si->setRowPrice(dummyRowSol) ;
  OSIUNITTEST_ASSERT_ERROR(dummyRowSol != si->getRowPrice(), allOK = false, *si, "setting solutions: solver should not return original pointer");
  colVec = si->getRowPrice() ;

  if( colVec != NULL )
  {
    ok = true ;
    for (i = 0 ; i < m ; i++)
    { mval = colVec[i] ;
    cval = dummyRowSol[i] ;
    if (mval != cval)
    { ok = false ;
    std::cout
    << "y<" << i << "> = " << mval
    << ", expecting " << cval
    << ", |error| = " << (mval-cval)
    << "." << std::endl ; } }
  }
  else
    ok = false;
  OSIUNITTEST_ASSERT_ERROR(ok == true, allOK = false, *si, "setting solutions: solver stored row price correctly");
/*
  Now let's get serious. Check that reduced costs and row activities match
  the values we just specified for row and column solutions. Absolute
  equality cannot be assumed here.

  Reduced costs first: c - yA
*/
  rowVec = si->getReducedCost() ;
  objVec = si->getObjCoefficients() ;
  const CoinPackedMatrix *mtx = si->getMatrixByCol() ;
  mtx->transposeTimes(dummyRowSol,rowShouldBe) ;
  if( rowVec != NULL )
  { ok = true ;
    for (i = 0 ; i < n ; i++)
    { mval = rowVec[i] ;
      rval = objVec[i] - rowShouldBe[i] ;
      if (!fltEq(mval,rval))
      { ok = false ;
        std::cout
        << "cbar<" << i << "> = " << mval
        << ", expecting " << rval
        << ", |error| = " << (mval-rval)
        << "." << std::endl ; } }
  } else
    ok = false;
  OSIUNITTEST_ASSERT_WARNING(ok == true, allOK = false, *si, "setting solutions: reduced costs from solution set with setRowPrice");

/*
  Row activity: Ax
*/
  colVec = si->getRowActivity() ;
  mtx->times(dummyColSol,colShouldBe) ;
  ok = true ;
  for (i = 0 ; i < m ; i++)
  { mval = colVec[i] ;
    cval = colShouldBe[i] ;
    if (!fltEq(mval,cval))
    { ok = false ;
      std::cout
        << "lhs<" << i << "> = " << mval
        << ", expecting " << cval
        << ", |error| = " << (mval-cval)
        << "." << std::endl ; } }
  OSIUNITTEST_ASSERT_WARNING(ok == true, allOK = false, *si, "setting solutions: row activity from solution set with setColSolution");

  if (allOK)
  { testingMessage(" ok.\n") ; }
  else
  { failureMessage(*si,"Errors handling imposed column/row solutions.") ; }

  delete [] dummyColSol ;
  delete [] colShouldBe ;
  delete [] dummyRowSol ;
  delete [] rowShouldBe ;

  delete si ;

  return ; }


//--------------------------------------------------------------------------

/*! \brief Helper routines to test OSI parameters.

  A set of helper routines to test integer, double, and hint parameter
  set/get routines.
*/

bool testIntParam(OsiSolverInterface * si, int k, int val)
{
  int i = 123456789, orig = 123456789;
  bool ret;
  OsiIntParam key = static_cast<OsiIntParam>(k);
  si->getIntParam(key, orig);
  if (si->setIntParam(key, val)) {
    ret = (si->getIntParam(key, i) == true) && (i == val);
  } else {
    ret = (si->getIntParam(key, i) == true) && (i == orig);
  }
  return ret;
}

bool testDblParam(OsiSolverInterface * si, int k, double val)
{
  double d = 123456789.0, orig = 123456789.0;
  bool ret;
  OsiDblParam key = static_cast<OsiDblParam>(k);
  si->getDblParam(key, orig);
  if (si->setDblParam(key, val)) {
    ret = (si->getDblParam(key, d) == true) && (d == val);
  } else {
    ret = (si->getDblParam(key, d) == true) && (d == orig);
  }
  return ret;
}

bool testHintParam(OsiSolverInterface * si, int k, bool sense,
			  OsiHintStrength strength, int *throws)
/*
  Tests for proper behaviour of [set,get]HintParam methods. The initial get
  tests the return value to see if the hint is implemented; the values
  returned for sense and strength are not checked.

  If the hint is implemented, a pair of set/get calls is performed at the
  strength specified by the parameter. The set can return true or, at
  strength OsiForceDo, throw an exception if the solver cannot comply. The
  rationale would be that only OsiForceDo must be obeyed, so anything else
  should return true regardless of whether the solver followed the hint.

  The test checks that the value and strength returned by getHintParam matches
  the previous call to setHintParam. This is arguably wrong --- one can argue
  that it should reflect the solver's ability to comply with the hint. But
  that's how the OSI interface standard has evolved up to now.

  If the hint is not implemented, attempting to set the hint should return
  false, or throw an exception at strength OsiForceDo.

  The testing code which calls testHintParam is set up so that a successful
  return is defined as true if the hint is implemented, false if it is not.
  Information printing is suppressed; uncomment and recompile if you want it.
*/
{ bool post_sense ;
  OsiHintStrength post_strength ;
  bool ret ;
  OsiHintParam key = static_cast<OsiHintParam>(k) ;

  if (si->getHintParam(key,post_sense,post_strength))
  { ret = false ;
  	std::ostringstream tstname;
    tstname << "testHintParam: hint " << static_cast<int>(key) << " sense " << sense << " strength " << static_cast<int>(strength);
    if( strength == OsiForceDo )
    {
    	try {
      	if (si->setHintParam(key,sense,strength)) {
      		ret = (si->getHintParam(key,post_sense,post_strength) == true) && (post_strength == strength) && (post_sense == sense);
      	}
    	} catch( CoinError& e ) {
    		std::cout << tstname.str() << " catched CoinError exception " << e.message() << std::endl;
    		ret = true;
      	(*throws)++;
    	}
    } else {
    	OSIUNITTEST_CATCH_WARNING(
    			if (si->setHintParam(key,sense,strength)) {
    				ret = (si->getHintParam(key,post_sense,post_strength) == true) && (post_strength == strength) && (post_sense == sense);
    			},
    			(*throws)++;
    			ret = (strength == OsiForceDo),
    			*si, tstname.str()
    	);
    }
  }
  else
  { ret = true ;
  	std::ostringstream tstname;
  	tstname << "testHintParam: hint " << static_cast<int>(key) << " sense " << sense << " strength " << static_cast<int>(strength);
  	OSIUNITTEST_CATCH_WARNING(ret = si->setHintParam(key,sense,strength),	(*throws)++; ret = !(strength == OsiForceDo), *si, tstname.str());
  }

  return ret ; }


/*
  Test functionality related to the objective function:
    * Does the solver properly handle a constant offset?
    * Does the solver properly handle primal and dual objective limits? This
      routine only checks for the correct answers. It does not check whether
      the solver stops early due to objective limits.
    * Does the solver properly handle minimisation / maximisation via
      setObjSense?

  The return value is the number of failures recorded by the routine.
*/

void testObjFunctions (const OsiSolverInterface *emptySi,
		       const std::string &mpsDir)

{ OsiSolverInterface *si = emptySi->clone() ;
  CoinRelFltEq eq ;
  int i ;

  std::cout
    << "Testing functionality related to the objective." << std::endl ;

  std::string solverName = "Unknown solver" ;
  si->getStrParam(OsiSolverName,solverName) ;
/*
  Check for default objective sense. This should be minimisation.
*/
  OSIUNITTEST_ASSERT_ERROR(si->getObjSense() == 1.0 || si->getObjSense() == -1.0, {}, solverName, "testObjFunctions: default objective sense is determinant value");
  OSIUNITTEST_ASSERT_WARNING(si->getObjSense() == 1.0, {}, solverName, "testObjFunctions: default objective sense is minimization");

/*
  Read in e226; chosen because it has an offset defined in the mps file.
  We can't continue if we can't read the test problem.
*/
  std::string fn = mpsDir+"e226" ;
  OSIUNITTEST_ASSERT_ERROR(si->readMps(fn.c_str(),"mps") == 0, delete si; return, solverName, "testObjFunctions: read MPS");
/*
  Solve and test for the correct objective value.
*/
  si->initialSolve() ;
  double objValue = si->getObjValue() ;
  double objNoOffset = -18.751929066 ;
  double objOffset = +7.113 ;
  OSIUNITTEST_ASSERT_ERROR(eq(objValue,(objNoOffset+objOffset)), {}, solverName, "testObjFunctions: getObjValue with constant in objective");
/*
  Test objective limit methods. If no limit has been specified, they should
  return false.
*/
  OSIUNITTEST_ASSERT_ERROR(!si->isPrimalObjectiveLimitReached(), {}, solverName, "testObjFunctions: isPrimalObjectiveLimitReached without limit (min)");
  OSIUNITTEST_ASSERT_ERROR(!si->isDualObjectiveLimitReached(), {}, solverName, "testObjFunctions: isDualObjectiveLimitReached without limit (min)");
/* One could think that also no limit should be reached in case of maximization.
 * However, by default primal and dual limits are not unset, but set to plus or minus infinity.
 * So, if we change the objective sense, we need to remember to set also the limits to a nonlimiting value.
 */
  si->setObjSense(-1.0) ;
  si->setDblParam(OsiPrimalObjectiveLimit, COIN_DBL_MAX);
  si->setDblParam(OsiDualObjectiveLimit,  -COIN_DBL_MAX);
  OSIUNITTEST_ASSERT_ERROR(!si->isPrimalObjectiveLimitReached(), {}, solverName, "testObjFunctions: isPrimalObjectiveLimitReached without limit (max)");
  OSIUNITTEST_ASSERT_ERROR(!si->isDualObjectiveLimitReached(), {}, solverName, "testObjFunctions: isDualObjectiveLimitReached without limit (max)");
  si->setObjSense(1.0) ;
  si->setDblParam(OsiPrimalObjectiveLimit, -COIN_DBL_MAX);
  si->setDblParam(OsiDualObjectiveLimit,    COIN_DBL_MAX);
/*
  Test objective limit methods. There's no attempt to see if the solver stops
  early when given a limit that's tighter than the optimal objective.  All
  we're doing here is checking that the routines return the correct value
  when the limits are exceeded. For minimisation (maximisation) the primal
  limit represents an acceptable level of `goodness'; to be true, we should
  be below (above) it. The dual limit represents an unacceptable level of
  `badness'; to be true, we should be above (below) it.

  The loop performs two iterations, first for maximisation, then for
  minimisation. For maximisation, z* = 111.65096. The second iteration is
  sort of redundant, but it does test the ability to switch back to
  minimisation.
*/
  double expectedObj[2] = { 111.650960689, objNoOffset+objOffset } ;
  double primalObjLim[2] = { 100.0, -5.0 } ;
  double dualObjLim[2] = { 120.0, -15.0 } ;
  double optSense[2] = { -1.0, 1.0 } ;
  for (i = 0 ; i <= 1 ; i++)
  { si->setObjSense(optSense[i]) ;

    // reset objective limits to infinity
    si->setDblParam(OsiPrimalObjectiveLimit, -optSense[i] * COIN_DBL_MAX);
    si->setDblParam(OsiDualObjectiveLimit,    optSense[i] * COIN_DBL_MAX);

    si->initialSolve() ;
    objValue = si->getObjValue() ;
    OSIUNITTEST_ASSERT_ERROR(eq(objValue,expectedObj[i]), {}, solverName, "testObjFunctions: optimal value during max/min switch");

    si->setDblParam(OsiPrimalObjectiveLimit,primalObjLim[i]) ;
    si->setDblParam(OsiDualObjectiveLimit,dualObjLim[i]) ;
    OSIUNITTEST_ASSERT_WARNING(si->isPrimalObjectiveLimitReached(), {}, solverName, "testObjFunctions: primal objective limit");
    OSIUNITTEST_ASSERT_WARNING(si->isDualObjectiveLimitReached(), {}, solverName, "testObjFunctions: dual objective limit");
  }

  delete si ;
  si = 0 ;

/*
  Finally, check that the objective sense is treated as a parameter of the
  solver, not a property of the problem. The test clones emptySi, inverts the
  default objective sense, clones a second solver, then loads and optimises
  e226.
*/
  si = emptySi->clone() ;
  double dfltSense = si->getObjSense() ;
  dfltSense = -dfltSense ;
  si->setObjSense(dfltSense) ;
  OsiSolverInterface *si2 = si->clone() ;
  delete si ;
  si = NULL ;
  OSIUNITTEST_ASSERT_ERROR(si2->getObjSense() == dfltSense, {}, solverName, "testObjFunctions: objective sense preserved by clone");
  OSIUNITTEST_ASSERT_ERROR(si2->readMps(fn.c_str(),"mps") == 0, delete si; return, solverName, "testObjFunctions: 2nd read MPS");
  OSIUNITTEST_ASSERT_ERROR(si2->getObjSense() == dfltSense, {}, solverName, "testObjFunctions: objective sense preserved by problem load");
  si2->initialSolve() ;
  if (dfltSense < 0)
  { i = 0 ; }
  else
  { i = 1 ; }
  objValue = si2->getObjValue() ;
  OSIUNITTEST_ASSERT_ERROR(eq(objValue,expectedObj[i]), {}, solverName, "testObjFunctions: optimal value of load problem after set objective sense");
  
  delete si2 ;
}


/*
  Check that solver returns the proper status for artificial variables. The OSI
  convention is that this status should be reported as if the artificial uses a
  positive coefficient. Specifically:

  ax <= b  ==>  ax + s = b,       0 <= s <= infty
  ax >= b  ==>  ax + s = b,  -infty <= s <= 0

  If the constraint is tight at optimum, then the status should be
  atLowerBound for a <= constraint, atUpperBound for a >= constraint. The
  test problem is

      x1      >= -5	(c0)
      x1      <=  2	(c1)
           x2 >= 44	(c2)
	   x2 <= 51	(c3)

  This is evaluated for two objectives, so that we have all combinations of
  tight constraints under minimisation and maximisation.

		max x1-x2	min x1-x2

	  obj	  -42		  -56

	  c0	  basic		  upper
	  c1	  lower		  basic
	  c2	  upper		  basic
	  c3	  basic		  lower
  
*/

void testArtifStatus (const OsiSolverInterface *emptySi)

{ OsiSolverInterface *si = emptySi->clone() ;
  double infty = si->getInfinity() ;

  testingMessage("Testing status for artificial variables.\n") ;
/*
  Set up the example problem in packed column-major vector format and load it
  into the solver.
*/
  int colCnt = 2 ;
  int rowCnt = 4 ;
  int indices[] = {0, 1, 2, 3} ;
  double coeffs[] = {1.0, 1.0, 1.0, 1.0} ;
  CoinBigIndex starts[] = {0, 2, 4} ;
  double obj[] = {1.0, -1.0} ;

  double vubs[] = {  infty,  infty } ;
  double vlbs[] = { -infty, -infty } ;

  double rubs[] = {  infty, 2.0,  infty, 51.0 } ;
  double rlbs[] = { -5.0, -infty, 44.0,  -infty } ;
  std::string contype[] = { ">=", "<=", ">=", "<=" } ;
  std::string statCode[] = { "isFree", "basic",
			     "atUpperBound", "atLowerBound" } ;
  std::string sense[] = { "maximise", "minimise" } ;

  si->loadProblem(colCnt,rowCnt,
		  starts,indices,coeffs,vlbs,vubs,obj,rlbs,rubs) ;
/*
  Vectors for objective sense and correct answers. Maximise first.
*/
  double objSense[] = { -1.0, 1.0 } ;
  double zopt[] = { -42.0, -56 } ;
  CoinWarmStartBasis::Status goodStatus[] =
      { CoinWarmStartBasis::basic,
	CoinWarmStartBasis::atLowerBound,
	CoinWarmStartBasis::atUpperBound,
	CoinWarmStartBasis::basic,
	CoinWarmStartBasis::atUpperBound,
	CoinWarmStartBasis::basic,
	CoinWarmStartBasis::basic,
	CoinWarmStartBasis::atLowerBound } ;
/*
  Get to work. Open a loop, set the objective sense, solve the problem, and
  then check the results: We should have an optimal solution, with the correct
  objective. We should be able to ask for a warm start basis, and it should
  show the correct status.
*/
  CoinRelFltEq eq ;

  for (int iter = 0 ; iter <= 1 ; iter++)
  { si->setObjSense(objSense[iter]) ;
    si->initialSolve() ;
    OSIUNITTEST_ASSERT_ERROR(si->isProvenOptimal(), {}; continue, *si, "testArtifStatus: initial solve");
    OSIUNITTEST_ASSERT_ERROR(eq(si->getObjValue(),zopt[iter]), {}; continue, *si, "testArtifStatus: initial solve optimal value");

    CoinWarmStart *ws = si->getWarmStart() ;
    CoinWarmStartBasis *wsb = dynamic_cast<CoinWarmStartBasis *>(ws) ;
    OSIUNITTEST_ASSERT_ERROR(wsb != NULL, {}; continue, *si, "testArtifStatus: initial solve warm start basis");

    CoinWarmStartBasis::Status stati ;

    bool ok = true;
    for (int i = 0 ; i < rowCnt ; i++)
    { stati = wsb->getArtifStatus(i) ;
      if (stati != goodStatus[iter*rowCnt+i])
      { ok = false;
      	std::cout << "Incorrect status " << statCode[stati] << " for " << contype[i] << " constraint c" << i << " (" << sense[iter] << "), expected " << statCode[goodStatus[iter*rowCnt+i]] << "." << std::endl;
      } }
  	OSIUNITTEST_ASSERT_ERROR(ok == true, {}, *si, "testArtifStatus: artifical variable status");
    
    delete ws ; }
/*
  Clean up.
*/
  delete si ;
}


/*
  This method checks [cbar<B> cbar<N>] = [c<B>-yB c<N>-yN] = [0 (c<N> - yN)]
  for the architectural variables. (But note there's no need to discriminate
  between basic and nonbasic columns in the tests below.)  This provides a
  moderately strong check on the correctness of y (getRowPrice) and cbar
  (getReducedCosts). The method also checks that the sign of cbar<N> is
  appropriate for the status of the variables.
*/

void testReducedCosts (const OsiSolverInterface *emptySi,
		       const std::string &sampleDir)

{
  OsiSolverInterface *si = emptySi->clone() ;
  std::string solverName;
  si->getStrParam(OsiSolverName,solverName);
  si->setHintParam(OsiDoReducePrint,true,OsiHintDo) ;

  std::cout << "Testing duals and reduced costs ... " ;
/*
  Read p0033 and solve to optimality (minimisation).
*/
  std::string fn = sampleDir+"p0033";
  si->readMps(fn.c_str(),"mps");
  si->setObjSense(1.0) ;
  si->initialSolve();
	OSIUNITTEST_ASSERT_ERROR(si->isProvenOptimal(), return, solverName, "testReducedCosts: solving p0033");
  if (OsiUnitTest::verbosity >= 1)
  { std::cout
      << "  " << solverName << " solved p0033 z = " << si->getObjValue()
      << "." << std::endl ; }
/*
  Get the unchanging components: size, matrix, objective.
*/
  int n = si->getNumCols() ;
  double *cbarCalc = new double[n] ;
  double dualTol ;
  si->getDblParam(OsiDualTolerance,dualTol) ;
  CoinRelFltEq eq ;
  std::string statNames[] = { "NBFR", "B", "NBUB", "NBLB" } ;
/*
  Resolve, first as maximisation, then minimisation, and do the tests.
*/
  double minmax[] = { -1.0,  1.0 } ;
  for (int ndx = 0 ; ndx < 2 ; ndx++)
  { si->setObjSense(minmax[ndx]) ;
    si->resolve() ;
  	OSIUNITTEST_ASSERT_ERROR(si->isProvenOptimal(), return, solverName, "testReducedCosts: solving p0033 after changing objective sense");
    if (OsiUnitTest::verbosity >= 1)
    { std::cout
	<< "  " << solverName
	<< ((si->getObjSense() < 0)?" maximised":" minimised")
	<< " p0033 z = " << si->getObjValue()
	<< "." << std::endl ; }
/*
  Retrieve status, duals, and reduced costs. Calculate c - yA.
*/
    const CoinPackedMatrix *mtx = si->getMatrixByCol() ;
    const double *c = si->getObjCoefficients() ;
    const CoinWarmStartBasis *wsb = 
      dynamic_cast<CoinWarmStartBasis *>(si->getWarmStart()) ;
    double dir = si->getObjSense() ;
    const double *y = si->getRowPrice() ;
    const double *cbar = si->getReducedCost() ;
    mtx->transposeTimes(y,cbarCalc) ;
    std::transform(c,c+n,cbarCalc,cbarCalc,std::minus<double>()) ;
/*
  Walk the architecturals and check that cbar<j> = c<j> - ya<j> and has the
  correct sign given the status and objective sense (max/min).
*/
    bool cbarCalcj_ok = true;
    bool testcbarj_ok = true;
    for (int j = 0 ; j < n ; j++)
    { CoinWarmStartBasis::Status statj = wsb->getStructStatus(j) ;
      double cbarj = cbar[j] ;
      double cbarCalcj = cbarCalc[j] ;
    
      if (OsiUnitTest::verbosity >= 1)
      { std::cout << "  x<" << j << "> " << statNames[statj] << ", cbar<" << j << "> = " << cbarj << "." << std::endl ; }

      if (!eq(cbarj,cbarCalcj))
      { cbarCalcj_ok = false;
      	if (OsiUnitTest::verbosity >= 1)
      	{ std::cout
      		<< "  " << cbarj << " = cbar<" << j << "> != c<"
      		<< j << "> - ya<" << j << "> = "
      		<< cbarCalcj << ", diff = "
      		<< cbarj-cbarCalcj << "." << std::endl ; } }

      double testcbarj = dir*cbarj ;
      switch (statj)
      { case CoinWarmStartBasis::atUpperBound:
      	{ if (testcbarj > dualTol)
      		{ testcbarj_ok = false;
      			if (OsiUnitTest::verbosity >= 1)
      			{ std::cout
      				<< "  cbar<" << j << "> = " << cbarj
      				<< " has the wrong sign for a NBUB variable."
      				<< std::endl ; } }
      		break ; }
      	case CoinWarmStartBasis::atLowerBound:
      	{ if (testcbarj < -dualTol)
      		{ testcbarj_ok = false;
      			if (OsiUnitTest::verbosity >= 1)
      			{ std::cout
      				<< "  cbar<" << j << "> = " << cbarj
      				<< " has the wrong sign for a NBLB variable."
      				<< std::endl ; } }
      		break ; }
      	case CoinWarmStartBasis::isFree:
      	{ if (CoinAbs(testcbarj) > dualTol)
      		{ testcbarj_ok = false;
      			if (OsiUnitTest::verbosity >= 1)
      			{ std::cout
      				<< "  cbar<" << j << "> = " << cbarj
      				<< " should be zero for a NBFR variable."
      				<< std::endl ; } }
      		break ; }
      	case CoinWarmStartBasis::basic:
      	{ if (CoinAbs(testcbarj) > dualTol)
      		{ testcbarj_ok = false;
      			if (OsiUnitTest::verbosity >= 1)
      			{ std::cout
      				<< "  cbar<" << j << "> = " << cbarj
      				<< " should be zero for a basic variable."
      				<< std::endl ; } }
      		break ; }
      	default:
      	{ break ; } } }
  	OSIUNITTEST_ASSERT_ERROR(cbarCalcj_ok == true, {}, solverName, "testReducedCosts: reduced costs");
  	OSIUNITTEST_ASSERT_ERROR(testcbarj_ok == true, {}, solverName, "testReducedCosts: basis status of structural variable");

    delete wsb ; }

  delete[] cbarCalc ;
}




/*
  Test the writeMps and writeMpsNative functions by loading a problem,
  writing it out to a file, reloading it, and solving.
  
  Implicitly assumes readMps has already been tested.

  fn should be the path to exmip1.
*/

void testWriteMps (const OsiSolverInterface *emptySi, std::string fn)

{
  testingMessage("Testing writeMps and writeMpsNative.\n") ;

  CoinRelFltEq eq(1.0e-8) ;

  OsiSolverInterface *si1 = emptySi->clone();
  OsiSolverInterface *si2 = emptySi->clone();
  OsiSolverInterface *si3 = emptySi->clone();
/*
  Sanity test. Read in exmip1 and do an initialSolve.
*/
  OSIUNITTEST_ASSERT_ERROR(si1->readMps(fn.c_str(),"mps") == 0, return, *si1, "testWriteMps: read MPS");

  bool solved = true;
  OSIUNITTEST_CATCH_SEVERITY_EXPECTED(si1->initialSolve(), solved = false, *si1, "testWriteMps: solving LP",
  		TestOutcome::ERROR, e.className() == "OsiVolSolverInterface" || e.className() == "OsiTestSolverInterface");
  double soln = si1->getObjValue();
/*
  Write a test output file with writeMpsNative, then read and solve. See if
  we get the right answer.
  
  FIXME: Really, this test should verify values --- Vol could participate in
  that (lh, 070726).
*/
  si1->writeMpsNative("test.out",NULL,NULL);
  OSIUNITTEST_ASSERT_ERROR(si2->readMps("test.out","") == 0, return, *si1, "testWriteMps: read LP written by writeMpsNative");
  if (solved) {
    OSIUNITTEST_CATCH_ERROR(si2->initialSolve(), return, *si1, "testWriteMps: solving LP written by writeMpsNative");
    OSIUNITTEST_ASSERT_ERROR(eq(soln,si2->getObjValue()), return, *si1, "testWriteMps: solving LP written by writeMpsNative");
  }
/*
  Repeat with writeMps.
*/
  si1->writeMps("test2","out");
  OSIUNITTEST_ASSERT_ERROR(si3->readMps("test2.out","") == 0, return, *si1, "testWriteMps: read LP written by writeMps");
  if (solved) {
    OSIUNITTEST_CATCH_ERROR(si3->initialSolve(), return, *si1, "testWriteMps: solving LP written by writeMps");
    OSIUNITTEST_ASSERT_ERROR(eq(soln,si3->getObjValue()), return, *si1, "testWriteMps: solving LP written by writeMps");
  }
/*
  Clean up.
*/
  delete si1;
  delete si2;
  delete si3;
}


/*
  Test writeLp and writeLpNative. Same sequence as for testWriteMps, above.
  Implicitly assumes readLp has been tested, but in fact that's not the case at
  present (lh, 070726).
*/
void testWriteLp (const OsiSolverInterface *emptySi, std::string fn)

{
  testingMessage("Testing writeLp and writeLpNative.\n") ;

  CoinRelFltEq eq(1.0e-8) ;

  OsiSolverInterface * si1 = emptySi->clone();
  OsiSolverInterface * si2 = emptySi->clone();
  OsiSolverInterface * si3 = emptySi->clone();

  OSIUNITTEST_ASSERT_ERROR(si1->readMps(fn.c_str(),"mps") == 0, return, *si1, "testWriteLp: read MPS");
  bool solved = true;
  OSIUNITTEST_CATCH_SEVERITY_EXPECTED(si1->initialSolve(), solved = false, *si1, "testWriteLp: solving LP",
  		TestOutcome::ERROR, e.className() == "OsiVolSolverInterface" || e.className() == "OsiTestSolverInterface");
  double soln = si1->getObjValue();

  si1->writeLpNative("test.lp",NULL,NULL,1.0e-9,10,8);

  OSIUNITTEST_ASSERT_ERROR(si2->readLp("test.lp") == 0, return, *si1, "testWriteLp: read LP written by writeLpNative");
  if (solved) {
    OSIUNITTEST_CATCH_ERROR(si2->initialSolve(), return, *si1, "testWriteLp: solving LP written by writeLpNative");
    OSIUNITTEST_ASSERT_ERROR(eq(soln,si2->getObjValue()), return, *si1, "testWriteLp: solving LP written by writeLpNative");
  }

  si1->writeLp("test2");
  OSIUNITTEST_ASSERT_ERROR(si3->readLp("test2.lp") == 0, return, *si1, "testWriteLp: read LP written by writeLp");
  if (solved) {
    OSIUNITTEST_CATCH_ERROR(si3->initialSolve(), return, *si1, "testWriteLp: solving LP written by writeLp");
    OSIUNITTEST_ASSERT_ERROR(eq(soln,si3->getObjValue()), return, *si1, "testWriteLp: solving LP written by writeLp");
  }

  delete si1;
  delete si2;
  delete si3;
}

/*
  Test load and assign problem. The first batch of tests loads up eight
  solvers, using each variant of loadProblem and assignProblem, runs
  initialSolve for all, then checks all values for all variants.
*/

void testLoadAndAssignProblem (const OsiSolverInterface *emptySi,
			       const OsiSolverInterface *exmip1Si)

{
  CoinRelFltEq eq(1.0e-8) ;
  std::string solverName;
  if( !emptySi->getStrParam(OsiSolverName, solverName) )
     solverName == "unknown";
/*
  Test each variant of loadProblem and assignProblem. Clone a whack of solvers
  and use one for each variant. Then run initialSolve() on each solver. Then
  check that all values are as they should be.

  Note that we are not testing the variants that supply the matrix as a set
  of vectors (row/col starts, col/row indices, coefficients). To be really
  thorough, we should do another eight ...
*/
  {
    testingMessage("Testing loadProblem and assignProblem methods.\n") ;
    OsiSolverInterface * base = exmip1Si->clone();
    OsiSolverInterface *  si1 = emptySi->clone();
    OsiSolverInterface *  si2 = emptySi->clone();
    OsiSolverInterface *  si3 = emptySi->clone();
    OsiSolverInterface *  si4 = emptySi->clone();
    OsiSolverInterface *  si5 = emptySi->clone();
    OsiSolverInterface *  si6 = emptySi->clone();
    OsiSolverInterface *  si7 = emptySi->clone();
    OsiSolverInterface *  si8 = emptySi->clone();

    si1->loadProblem(*base->getMatrixByCol(),
		     base->getColLower(),base->getColUpper(),
		     base->getObjCoefficients(),
		     base->getRowSense(),base->getRightHandSide(),
		     base->getRowRange());
    si2->loadProblem(*base->getMatrixByRow(),
		     base->getColLower(),base->getColUpper(),
		     base->getObjCoefficients(),
		     base->getRowSense(),base->getRightHandSide(),
		     base->getRowRange());
    si3->loadProblem(*base->getMatrixByCol(),
		     base->getColLower(),base->getColUpper(),
		     base->getObjCoefficients(),
		     base->getRowLower(),base->getRowUpper() );
    si4->loadProblem(*base->getMatrixByCol(),
		     base->getColLower(),base->getColUpper(),
		     base->getObjCoefficients(),
		     base->getRowLower(),base->getRowUpper() );
    {
      double objOffset;
      base->getDblParam(OsiObjOffset,objOffset);
      si1->setDblParam(OsiObjOffset,objOffset);
      si2->setDblParam(OsiObjOffset,objOffset);
      si3->setDblParam(OsiObjOffset,objOffset);
      si4->setDblParam(OsiObjOffset,objOffset);
      si5->setDblParam(OsiObjOffset,objOffset);
      si6->setDblParam(OsiObjOffset,objOffset);
      si7->setDblParam(OsiObjOffset,objOffset);
      si8->setDblParam(OsiObjOffset,objOffset);
    }
/*
  Assign methods should set their parameters to NULL, so check for that.
*/
    CoinPackedMatrix * pm = new CoinPackedMatrix(*base->getMatrixByCol());
    double * clb = new double[base->getNumCols()];
    std::copy(base->getColLower(),
	      base->getColLower()+base->getNumCols(),clb);
    double * cub = new double[base->getNumCols()];
    std::copy(base->getColUpper(),
	      base->getColUpper()+base->getNumCols(),cub);
    double * objc = new double[base->getNumCols()];
    std::copy(base->getObjCoefficients(),
	      base->getObjCoefficients()+base->getNumCols(),objc);
    double * rlb = new double[base->getNumRows()];
    std::copy(base->getRowLower(),
	      base->getRowLower()+base->getNumRows(),rlb);
    double * rub = new double[base->getNumRows()];
    std::copy(base->getRowUpper(),
	      base->getRowUpper()+base->getNumRows(),rub);
    si5->assignProblem(pm,clb,cub,objc,rlb,rub);
    OSIUNITTEST_ASSERT_ERROR(pm   == NULL, return, solverName, "testLoadAndAssignProblem: si5 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(clb  == NULL, return, solverName, "testLoadAndAssignProblem: si5 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(cub  == NULL, return, solverName, "testLoadAndAssignProblem: si5 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(objc == NULL, return, solverName, "testLoadAndAssignProblem: si5 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(rlb  == NULL, return, solverName, "testLoadAndAssignProblem: si5 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(rub  == NULL, return, solverName, "testLoadAndAssignProblem: si5 assignProblem should set parameters to NULL");

    pm = new CoinPackedMatrix(*base->getMatrixByRow());
    clb = new double[base->getNumCols()];
    std::copy(base->getColLower(),
	      base->getColLower()+base->getNumCols(),clb);
    cub = new double[base->getNumCols()];
    std::copy(base->getColUpper(),
	      base->getColUpper()+base->getNumCols(),cub);
    objc = new double[base->getNumCols()];
    std::copy(base->getObjCoefficients(),
	      base->getObjCoefficients()+base->getNumCols(),objc);
    rlb = new double[base->getNumRows()];
    std::copy(base->getRowLower(),
	      base->getRowLower()+base->getNumRows(),rlb);
    rub = new double[base->getNumRows()];
    std::copy(base->getRowUpper(),
	      base->getRowUpper()+base->getNumRows(),rub);
    si6->assignProblem(pm,clb,cub,objc,rlb,rub);
    OSIUNITTEST_ASSERT_ERROR(pm   == NULL, return, solverName, "testLoadAndAssignProblem: si6 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(clb  == NULL, return, solverName, "testLoadAndAssignProblem: si6 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(cub  == NULL, return, solverName, "testLoadAndAssignProblem: si6 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(objc == NULL, return, solverName, "testLoadAndAssignProblem: si6 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(rlb  == NULL, return, solverName, "testLoadAndAssignProblem: si6 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(rub  == NULL, return, solverName, "testLoadAndAssignProblem: si6 assignProblem should set parameters to NULL");

    pm = new CoinPackedMatrix(*base->getMatrixByCol());
    clb = new double[base->getNumCols()];
    std::copy(base->getColLower(),
	      base->getColLower()+base->getNumCols(),clb);
    cub = new double[base->getNumCols()];
    std::copy(base->getColUpper(),
	      base->getColUpper()+base->getNumCols(),cub);
    objc = new double[base->getNumCols()];
    std::copy(base->getObjCoefficients(),
	      base->getObjCoefficients()+base->getNumCols(),objc);
    char * rsen = new char[base->getNumRows()];
    std::copy(base->getRowSense(),
	      base->getRowSense()+base->getNumRows(),rsen);
    double * rhs = new double[base->getNumRows()];
    std::copy(base->getRightHandSide(),
	      base->getRightHandSide()+base->getNumRows(),rhs);
    double * rng = new double[base->getNumRows()];
    std::copy(base->getRowRange(),
	      base->getRowRange()+base->getNumRows(),rng);
    si7->assignProblem(pm,clb,cub,objc,rsen,rhs,rng);
    OSIUNITTEST_ASSERT_ERROR(pm   == NULL, return, solverName, "testLoadAndAssignProblem: si7 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(clb  == NULL, return, solverName, "testLoadAndAssignProblem: si7 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(cub  == NULL, return, solverName, "testLoadAndAssignProblem: si7 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(objc == NULL, return, solverName, "testLoadAndAssignProblem: si7 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(rsen == NULL, return, solverName, "testLoadAndAssignProblem: si7 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(rhs  == NULL, return, solverName, "testLoadAndAssignProblem: si7 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(rng  == NULL, return, solverName, "testLoadAndAssignProblem: si7 assignProblem should set parameters to NULL");

    pm = new CoinPackedMatrix(*base->getMatrixByCol());
    clb = new double[base->getNumCols()];
    std::copy(base->getColLower(),
	      base->getColLower()+base->getNumCols(),clb);
    cub = new double[base->getNumCols()];
    std::copy(base->getColUpper(),
	      base->getColUpper()+base->getNumCols(),cub);
    objc = new double[base->getNumCols()];
    std::copy(base->getObjCoefficients(),
	      base->getObjCoefficients()+base->getNumCols(),objc);
    rsen = new char[base->getNumRows()];
    std::copy(base->getRowSense(),
	      base->getRowSense()+base->getNumRows(),rsen);
    rhs = new double[base->getNumRows()];
    std::copy(base->getRightHandSide(),
	      base->getRightHandSide()+base->getNumRows(),rhs);
    rng = new double[base->getNumRows()];
    std::copy(base->getRowRange(),
	      base->getRowRange()+base->getNumRows(),rng);
    si8->assignProblem(pm,clb,cub,objc,rsen,rhs,rng);
    OSIUNITTEST_ASSERT_ERROR(pm   == NULL, return, solverName, "testLoadAndAssignProblem: si8 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(clb  == NULL, return, solverName, "testLoadAndAssignProblem: si8 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(cub  == NULL, return, solverName, "testLoadAndAssignProblem: si8 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(objc == NULL, return, solverName, "testLoadAndAssignProblem: si8 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(rsen == NULL, return, solverName, "testLoadAndAssignProblem: si8 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(rhs  == NULL, return, solverName, "testLoadAndAssignProblem: si8 assignProblem should set parameters to NULL");
    OSIUNITTEST_ASSERT_ERROR(rng  == NULL, return, solverName, "testLoadAndAssignProblem: si8 assignProblem should set parameters to NULL");

    // Create an indices vector
    CoinPackedVector basePv,pv;
    OSIUNITTEST_ASSERT_ERROR(base->getNumCols()<10, return, solverName, "testLoadAndAssignProblem");
    OSIUNITTEST_ASSERT_ERROR(base->getNumRows()<10, return, solverName, "testLoadAndAssignProblem");
    int indices[10];
    int i;
    for (i=0; i<10; i++) indices[i]=i;

    // Test solve methods.
    // Vol solver interface is expected to throw
    // an error if the data has a ranged row.
    // Prepare test that there is non-zero range
    basePv.setFull(base->getNumRows(),base->getRowRange());
    pv.setConstant( base->getNumRows(), indices, 0.0 );

    OSIUNITTEST_CATCH_SEVERITY_EXPECTED(base->initialSolve(), return, *base, "testLoadAndAssignProblem: base initialSolve",
    		TestOutcome::ERROR, solverName == "vol" && !basePv.isEquivalent(pv));
    OSIUNITTEST_CATCH_SEVERITY_EXPECTED(si1->initialSolve(), return, *base, "testLoadAndAssignProblem: si1 initialSolve",
    		TestOutcome::ERROR, solverName == "vol" && !basePv.isEquivalent(pv));
    OSIUNITTEST_CATCH_SEVERITY_EXPECTED(si2->initialSolve(), return, *base, "testLoadAndAssignProblem: si2 initialSolve",
    		TestOutcome::ERROR, solverName == "vol" && !basePv.isEquivalent(pv));
    OSIUNITTEST_CATCH_SEVERITY_EXPECTED(si3->initialSolve(), return, *base, "testLoadAndAssignProblem: si3 initialSolve",
    		TestOutcome::ERROR, solverName == "vol" && !basePv.isEquivalent(pv));
    OSIUNITTEST_CATCH_SEVERITY_EXPECTED(si4->initialSolve(), return, *base, "testLoadAndAssignProblem: si4 initialSolve",
    		TestOutcome::ERROR, solverName == "vol" && !basePv.isEquivalent(pv));
    OSIUNITTEST_CATCH_SEVERITY_EXPECTED(si5->initialSolve(), return, *base, "testLoadAndAssignProblem: si5 initialSolve",
    		TestOutcome::ERROR, solverName == "vol" && !basePv.isEquivalent(pv));
    OSIUNITTEST_CATCH_SEVERITY_EXPECTED(si6->initialSolve(), return, *base, "testLoadAndAssignProblem: si6 initialSolve",
    		TestOutcome::ERROR, solverName == "vol" && !basePv.isEquivalent(pv));
    OSIUNITTEST_CATCH_SEVERITY_EXPECTED(si7->initialSolve(), return, *base, "testLoadAndAssignProblem: si7 initialSolve",
    		TestOutcome::ERROR, solverName == "vol" && !basePv.isEquivalent(pv));
    OSIUNITTEST_CATCH_SEVERITY_EXPECTED(si8->initialSolve(), return, *base, "testLoadAndAssignProblem: si8 initialSolve",
    		TestOutcome::ERROR, solverName == "vol" && !basePv.isEquivalent(pv));

    // Test collower
    basePv.setVector(base->getNumCols(),indices,base->getColLower());
    pv.setVector( si1->getNumCols(),indices, si1->getColLower());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si1 column lower bounds");
    pv.setVector( si2->getNumCols(),indices, si2->getColLower());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si2 column lower bounds");
    pv.setVector( si3->getNumCols(),indices, si3->getColLower());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si3 column lower bounds");
    pv.setVector( si4->getNumCols(),indices, si4->getColLower());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si4 column lower bounds");
    pv.setVector( si5->getNumCols(),indices, si5->getColLower());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si5 column lower bounds");
    pv.setVector( si6->getNumCols(),indices, si6->getColLower());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si6 column lower bounds");
    pv.setVector( si7->getNumCols(),indices, si7->getColLower());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si7 column lower bounds");
    pv.setVector( si8->getNumCols(),indices, si8->getColLower());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si8 column lower bounds");

    // Test colupper
    basePv.setVector(base->getNumCols(),indices,base->getColUpper());
    pv.setVector( si1->getNumCols(),indices, si1->getColUpper());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si1 column upper bounds");
    pv.setVector( si2->getNumCols(),indices, si2->getColUpper());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si2 column upper bounds");
    pv.setVector( si3->getNumCols(),indices, si3->getColUpper());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si3 column upper bounds");
    pv.setVector( si4->getNumCols(),indices, si4->getColUpper());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si4 column upper bounds");
    pv.setVector( si5->getNumCols(),indices, si5->getColUpper());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si5 column upper bounds");
    pv.setVector( si6->getNumCols(),indices, si6->getColUpper());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si6 column upper bounds");
    pv.setVector( si7->getNumCols(),indices, si7->getColUpper());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si7 column upper bounds");
    pv.setVector( si8->getNumCols(),indices, si8->getColUpper());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si8 column upper bounds");

    // Test getObjCoefficients
    basePv.setVector(base->getNumCols(),indices,base->getObjCoefficients());
    pv.setVector( si1->getNumCols(),indices, si1->getObjCoefficients());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si1 objective coefficients");
    pv.setVector( si2->getNumCols(),indices, si2->getObjCoefficients());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si2 objective coefficients");
    pv.setVector( si3->getNumCols(),indices, si3->getObjCoefficients());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si3 objective coefficients");
    pv.setVector( si4->getNumCols(),indices, si4->getObjCoefficients());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si4 objective coefficients");
    pv.setVector( si5->getNumCols(),indices, si5->getObjCoefficients());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si5 objective coefficients");
    pv.setVector( si6->getNumCols(),indices, si6->getObjCoefficients());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si6 objective coefficients");
    pv.setVector( si7->getNumCols(),indices, si7->getObjCoefficients());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si7 objective coefficients");
    pv.setVector( si8->getNumCols(),indices, si8->getObjCoefficients());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si8 objective coefficients");
	
    // Test rowrhs
    basePv.setFull(base->getNumRows(),base->getRightHandSide());
    pv.setFull( si1->getNumRows(), si1->getRightHandSide());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si1 right hand side");
    pv.setFull( si2->getNumRows(), si2->getRightHandSide());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si2 right hand side");
    pv.setFull( si3->getNumRows(), si3->getRightHandSide());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si3 right hand side");
    pv.setFull( si4->getNumRows(), si4->getRightHandSide());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si4 right hand side");
    pv.setFull( si5->getNumRows(), si5->getRightHandSide());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si5 right hand side");
    pv.setFull( si6->getNumRows(), si6->getRightHandSide());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si6 right hand side");
    pv.setFull( si7->getNumRows(), si7->getRightHandSide());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si7 right hand side");
    pv.setFull( si8->getNumRows(), si8->getRightHandSide());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si8 right hand side");

    // Test rowrange
    basePv.setFull(base->getNumRows(),base->getRowRange());
    pv.setFull( si1->getNumRows(), si1->getRowRange());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si1 row range");
    pv.setFull( si2->getNumRows(), si2->getRowRange());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si2 row range");
    pv.setFull( si3->getNumRows(), si3->getRowRange());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si3 row range");
    pv.setFull( si4->getNumRows(), si4->getRowRange());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si4 row range");
    pv.setFull( si5->getNumRows(), si5->getRowRange());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si5 row range");
    pv.setFull( si6->getNumRows(), si6->getRowRange());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si6 row range");
    pv.setFull( si7->getNumRows(), si7->getRowRange());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si7 row range");
    pv.setFull( si8->getNumRows(), si8->getRowRange());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si8 row range");

    // Test row sense
    {
      const char * cb = base->getRowSense();
      const char * c1 = si1->getRowSense();
      const char * c2 = si2->getRowSense();
      const char * c3 = si3->getRowSense();
      const char * c4 = si4->getRowSense();
      const char * c5 = si5->getRowSense();
      const char * c6 = si6->getRowSense();
      const char * c7 = si7->getRowSense();
      const char * c8 = si8->getRowSense();
      int nr = base->getNumRows();
      bool rowsense_ok1 = true;
      bool rowsense_ok2 = true;
      bool rowsense_ok3 = true;
      bool rowsense_ok4 = true;
      bool rowsense_ok5 = true;
      bool rowsense_ok6 = true;
      bool rowsense_ok7 = true;
      bool rowsense_ok8 = true;
      for ( i=0; i<nr; i++ ) {
      	rowsense_ok1 &= cb[i]==c1[i];
      	rowsense_ok2 &= cb[i]==c2[i];
      	rowsense_ok3 &= cb[i]==c3[i];
      	rowsense_ok4 &= cb[i]==c4[i];
      	rowsense_ok5 &= cb[i]==c5[i];
      	rowsense_ok6 &= cb[i]==c6[i];
      	rowsense_ok7 &= cb[i]==c7[i];
      	rowsense_ok8 &= cb[i]==c8[i];
      }
      OSIUNITTEST_ASSERT_ERROR(rowsense_ok1, return, solverName, "testLoadAndAssignProblem: si1 row sense");
      OSIUNITTEST_ASSERT_ERROR(rowsense_ok2, return, solverName, "testLoadAndAssignProblem: si2 row sense");
      OSIUNITTEST_ASSERT_ERROR(rowsense_ok3, return, solverName, "testLoadAndAssignProblem: si3 row sense");
      OSIUNITTEST_ASSERT_ERROR(rowsense_ok4, return, solverName, "testLoadAndAssignProblem: si4 row sense");
      OSIUNITTEST_ASSERT_ERROR(rowsense_ok5, return, solverName, "testLoadAndAssignProblem: si5 row sense");
      OSIUNITTEST_ASSERT_ERROR(rowsense_ok6, return, solverName, "testLoadAndAssignProblem: si6 row sense");
      OSIUNITTEST_ASSERT_ERROR(rowsense_ok7, return, solverName, "testLoadAndAssignProblem: si7 row sense");
      OSIUNITTEST_ASSERT_ERROR(rowsense_ok8, return, solverName, "testLoadAndAssignProblem: si8 row sense");
    }

    // Test rowlower
    basePv.setVector(base->getNumRows(),indices,base->getRowLower());
    pv.setVector( si1->getNumRows(),indices, si1->getRowLower());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si1 row lower bounds");
    pv.setVector( si2->getNumRows(),indices, si2->getRowLower());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si2 row lower bounds");
    pv.setVector( si3->getNumRows(),indices, si3->getRowLower());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si3 row lower bounds");
    pv.setVector( si4->getNumRows(),indices, si4->getRowLower());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si4 row lower bounds");
    pv.setVector( si5->getNumRows(),indices, si5->getRowLower());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si5 row lower bounds");
    pv.setVector( si6->getNumRows(),indices, si6->getRowLower());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si6 row lower bounds");
    pv.setVector( si7->getNumRows(),indices, si7->getRowLower());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si7 row lower bounds");
    pv.setVector( si8->getNumRows(),indices, si8->getRowLower());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si8 row lower bounds");

    // Test rowupper
    basePv.setVector(base->getNumRows(),indices,base->getRowUpper());
    pv.setVector( si1->getNumRows(),indices, si1->getRowUpper());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si1 row upper bounds");
    pv.setVector( si2->getNumRows(),indices, si2->getRowUpper());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si2 row upper bounds");
    pv.setVector( si3->getNumRows(),indices, si3->getRowUpper());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si3 row upper bounds");
    pv.setVector( si4->getNumRows(),indices, si4->getRowUpper());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si4 row upper bounds");
    pv.setVector( si5->getNumRows(),indices, si5->getRowUpper());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si5 row upper bounds");
    pv.setVector( si6->getNumRows(),indices, si6->getRowUpper());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si6 row upper bounds");
    pv.setVector( si7->getNumRows(),indices, si7->getRowUpper());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si7 row upper bounds");
    pv.setVector( si8->getNumRows(),indices, si8->getRowUpper());
    OSIUNITTEST_ASSERT_ERROR(basePv.isEquivalent(pv), return, solverName, "testLoadAndAssignProblem: si8 row upper bounds");

    // Test Constraint Matrix
    OSIUNITTEST_ASSERT_ERROR(base->getMatrixByCol()->isEquivalent(*si1->getMatrixByCol()), return, solverName, "testLoadAndAssignProblem: si1 constraint matrix");
    OSIUNITTEST_ASSERT_ERROR(base->getMatrixByRow()->isEquivalent(*si1->getMatrixByRow()), return, solverName, "testLoadAndAssignProblem: si1 constraint matrix");
    OSIUNITTEST_ASSERT_ERROR(base->getMatrixByCol()->isEquivalent(*si2->getMatrixByCol()), return, solverName, "testLoadAndAssignProblem: si2 constraint matrix");
    OSIUNITTEST_ASSERT_ERROR(base->getMatrixByRow()->isEquivalent(*si2->getMatrixByRow()), return, solverName, "testLoadAndAssignProblem: si2 constraint matrix");
    OSIUNITTEST_ASSERT_ERROR(base->getMatrixByCol()->isEquivalent(*si3->getMatrixByCol()), return, solverName, "testLoadAndAssignProblem: si3 constraint matrix");
    OSIUNITTEST_ASSERT_ERROR(base->getMatrixByRow()->isEquivalent(*si3->getMatrixByRow()), return, solverName, "testLoadAndAssignProblem: si3 constraint matrix");
    OSIUNITTEST_ASSERT_ERROR(base->getMatrixByCol()->isEquivalent(*si4->getMatrixByCol()), return, solverName, "testLoadAndAssignProblem: si4 constraint matrix");
    OSIUNITTEST_ASSERT_ERROR(base->getMatrixByRow()->isEquivalent(*si4->getMatrixByRow()), return, solverName, "testLoadAndAssignProblem: si4 constraint matrix");
    OSIUNITTEST_ASSERT_ERROR(base->getMatrixByCol()->isEquivalent(*si5->getMatrixByCol()), return, solverName, "testLoadAndAssignProblem: si5 constraint matrix");
    OSIUNITTEST_ASSERT_ERROR(base->getMatrixByRow()->isEquivalent(*si5->getMatrixByRow()), return, solverName, "testLoadAndAssignProblem: si5 constraint matrix");
    OSIUNITTEST_ASSERT_ERROR(base->getMatrixByCol()->isEquivalent(*si6->getMatrixByCol()), return, solverName, "testLoadAndAssignProblem: si6 constraint matrix");
    OSIUNITTEST_ASSERT_ERROR(base->getMatrixByRow()->isEquivalent(*si6->getMatrixByRow()), return, solverName, "testLoadAndAssignProblem: si6 constraint matrix");
    OSIUNITTEST_ASSERT_ERROR(base->getMatrixByCol()->isEquivalent(*si7->getMatrixByCol()), return, solverName, "testLoadAndAssignProblem: si7 constraint matrix");
    OSIUNITTEST_ASSERT_ERROR(base->getMatrixByRow()->isEquivalent(*si7->getMatrixByRow()), return, solverName, "testLoadAndAssignProblem: si7 constraint matrix");
    OSIUNITTEST_ASSERT_ERROR(base->getMatrixByCol()->isEquivalent(*si8->getMatrixByCol()), return, solverName, "testLoadAndAssignProblem: si8 constraint matrix");
    OSIUNITTEST_ASSERT_ERROR(base->getMatrixByRow()->isEquivalent(*si8->getMatrixByRow()), return, solverName, "testLoadAndAssignProblem: si8 constraint matrix");

    // Test Objective Value
    OSIUNITTEST_ASSERT_ERROR(eq(base->getObjValue(),si1->getObjValue()), return, solverName, "testLoadAndAssignProblem: si1 objective value");
    OSIUNITTEST_ASSERT_ERROR(eq(base->getObjValue(),si2->getObjValue()), return, solverName, "testLoadAndAssignProblem: si2 objective value");
    OSIUNITTEST_ASSERT_ERROR(eq(base->getObjValue(),si3->getObjValue()), return, solverName, "testLoadAndAssignProblem: si3 objective value");
    OSIUNITTEST_ASSERT_ERROR(eq(base->getObjValue(),si4->getObjValue()), return, solverName, "testLoadAndAssignProblem: si4 objective value");
    OSIUNITTEST_ASSERT_ERROR(eq(base->getObjValue(),si5->getObjValue()), return, solverName, "testLoadAndAssignProblem: si5 objective value");
    OSIUNITTEST_ASSERT_ERROR(eq(base->getObjValue(),si6->getObjValue()), return, solverName, "testLoadAndAssignProblem: si6 objective value");
    OSIUNITTEST_ASSERT_ERROR(eq(base->getObjValue(),si7->getObjValue()), return, solverName, "testLoadAndAssignProblem: si7 objective value");
    OSIUNITTEST_ASSERT_ERROR(eq(base->getObjValue(),si8->getObjValue()), return, solverName, "testLoadAndAssignProblem: si8 objective value");

    // Clean-up
    delete si8;
    delete si7;
    delete si6;
    delete si5;
    delete si4;
    delete si3;
    delete si2;
    delete si1;
    delete base;
  }
/*
  The OSI interface spec says any of the parameters to loadProblem can default
  to null. Let's see if that works. Test the rowub, rowlb and sense, rhs,
  range variants. Arguably we should check all variants again, but let's
  hope that OSI implementors carry things over from one variant to another.

  For Gurobi, this does not work. Since Gurobi does not know about free rows
  (rowtype 'N'), OsiGrb translates free rows into 'L' (lower-equal) rows
  with a -infty right-hand side.  This makes some of the tests below fail.

  Ok, gurobi has quirks. As do other solvers. But those quirks should be
  hidden from users of OsiGrb -- i.e., the translation should be done within
  OsiGrb, and should not be visible to the user (or this test). That's the
  point of OSI.  It's an implementation failure and should not be swept
  under the rug here.  -- lh, 100826 --

  OsiGrb does quite some attempts to hide this translation from the user,
  need to check further what could be done. -- sv, 110306 --
  ps: it's just coincidence that I actually see your messages here...
*/

  if (solverName != "gurobi")
  {
    int i ;

    OsiSolverInterface * si1 = emptySi->clone();
    OsiSolverInterface * si2 = emptySi->clone();
      
    si1->loadProblem(*exmip1Si->getMatrixByCol(),NULL,NULL,NULL,NULL,NULL);
    si2->loadProblem(*exmip1Si->getMatrixByCol(),NULL,NULL,NULL,NULL,NULL,NULL);
      
    // Test column settings
    OSIUNITTEST_ASSERT_ERROR(si1->getNumCols() == exmip1Si->getNumCols(), return, solverName, "testLoadAndAssignProblem: si1 loadProblem with matrix only");
    bool collower_ok = true;
    bool colupper_ok = true;
    bool colobjcoef_ok = true;
    for ( i=0; i<si1->getNumCols(); i++ ) {
      collower_ok &= eq(si1->getColLower()[i],0.0);
      colupper_ok &= eq(si1->getColUpper()[i],si1->getInfinity());
      colobjcoef_ok &= eq(si1->getObjCoefficients()[i],0.0);
    }
    OSIUNITTEST_ASSERT_ERROR(collower_ok   == true, return, solverName, "testLoadAndAssignProblem: si1 loadProblem with matrix only");
    OSIUNITTEST_ASSERT_ERROR(colupper_ok   == true, return, solverName, "testLoadAndAssignProblem: si1 loadProblem with matrix only");
    OSIUNITTEST_ASSERT_ERROR(colobjcoef_ok == true, return, solverName, "testLoadAndAssignProblem: si1 loadProblem with matrix only");

    // Test row settings
    OSIUNITTEST_ASSERT_ERROR(si1->getNumRows() == exmip1Si->getNumRows(), return, solverName, "testLoadAndAssignProblem: si1 loadProblem with matrix only");
    const double * rh = si1->getRightHandSide();
    const double * rr = si1->getRowRange();
    const char * rs = si1->getRowSense();
    const double * rl = si1->getRowLower();
    const double * ru = si1->getRowUpper();
    bool rowrhs_ok = true;
    bool rowrange_ok = true;
    bool rowsense_ok = true;
    bool rowlower_ok = true;
    bool rowupper_ok = true;
    for ( i=0; i<si1->getNumRows(); i++ ) {
      rowrhs_ok &= eq(rh[i],0.0);
      rowrange_ok &= eq(rr[i],0.0);
      rowsense_ok &= 'N'==rs[i];
      rowlower_ok &= eq(rl[i],-si1->getInfinity());
      rowupper_ok &= eq(ru[i], si1->getInfinity());
    }
    OSIUNITTEST_ASSERT_ERROR(rowrhs_ok   == true, return, solverName, "testLoadAndAssignProblem: si1 loadProblem with matrix only");
    OSIUNITTEST_ASSERT_ERROR(rowrange_ok == true, return, solverName, "testLoadAndAssignProblem: si1 loadProblem with matrix only");
    OSIUNITTEST_ASSERT_ERROR(rowsense_ok == true, return, solverName, "testLoadAndAssignProblem: si1 loadProblem with matrix only");
    OSIUNITTEST_ASSERT_ERROR(rowlower_ok == true, return, solverName, "testLoadAndAssignProblem: si1 loadProblem with matrix only");
    OSIUNITTEST_ASSERT_ERROR(rowupper_ok == true, return, solverName, "testLoadAndAssignProblem: si1 loadProblem with matrix only");

    // And repeat for si2
    OSIUNITTEST_ASSERT_ERROR(si2->getNumCols() == exmip1Si->getNumCols(), return, solverName, "testLoadAndAssignProblem: si2 loadProblem with matrix only");
    collower_ok = true;
    colupper_ok = true;
    colobjcoef_ok = true;
    for ( i=0; i<si2->getNumCols(); i++ ) {
      collower_ok &= eq(si2->getColLower()[i],0.0);
      colupper_ok &= eq(si2->getColUpper()[i],si2->getInfinity());
      colobjcoef_ok &= eq(si2->getObjCoefficients()[i],0.0);
    }
    OSIUNITTEST_ASSERT_ERROR(collower_ok   == true, return, solverName, "testLoadAndAssignProblem: si2 loadProblem with matrix only");
    OSIUNITTEST_ASSERT_ERROR(colupper_ok   == true, return, solverName, "testLoadAndAssignProblem: si2 loadProblem with matrix only");
    OSIUNITTEST_ASSERT_ERROR(colobjcoef_ok == true, return, solverName, "testLoadAndAssignProblem: si2 loadProblem with matrix only");
    //
    OSIUNITTEST_ASSERT_ERROR(si2->getNumRows() == exmip1Si->getNumRows(), return, solverName, "testLoadAndAssignProblem: si2 loadProblem with matrix only");
    rh = si2->getRightHandSide();
    rr = si2->getRowRange();
    rs = si2->getRowSense();
    rl = si2->getRowLower();
    ru = si2->getRowUpper();
    rowrhs_ok = true;
    rowrange_ok = true;
    rowsense_ok = true;
    rowlower_ok = true;
    rowupper_ok = true;
    for ( i=0; i<si2->getNumRows(); i++ ) {
      rowrhs_ok   &= eq(rh[i],0.0);
      rowrange_ok &= eq(rr[i],0.0);
      rowsense_ok &= 'G'==rs[i];
      rowlower_ok &= eq(rl[i],0.0);
      rowupper_ok &= eq(ru[i],si2->getInfinity());
    }
    OSIUNITTEST_ASSERT_ERROR(rowrhs_ok   == true, return, solverName, "testLoadAndAssignProblem: si2 loadProblem with matrix only");
    OSIUNITTEST_ASSERT_ERROR(rowrange_ok == true, return, solverName, "testLoadAndAssignProblem: si2 loadProblem with matrix only");
    OSIUNITTEST_ASSERT_ERROR(rowsense_ok == true, return, solverName, "testLoadAndAssignProblem: si2 loadProblem with matrix only");
    OSIUNITTEST_ASSERT_ERROR(rowlower_ok == true, return, solverName, "testLoadAndAssignProblem: si2 loadProblem with matrix only");
    OSIUNITTEST_ASSERT_ERROR(rowupper_ok == true, return, solverName, "testLoadAndAssignProblem: si2 loadProblem with matrix only");
      
    delete si1;
    delete si2;
  }
  else
  {
    failureMessage(solverName, "OsiGrb exposes inability to handle 'N' constraints (expected).") ;
    OSIUNITTEST_ADD_OUTCOME(solverName, "testLoadAndAssignProblem", "ability to handle 'N' constraints", TestOutcome::ERROR, true);
  }
/*
  Load problem with row rhs, sense and range, but leave column bounds and
  objective at defaults. A belt-and-suspenders kind of test. Arguably we should
  have the symmetric case, with column bounds valid and row values at default.
*/
  {
    int i ;

    OsiSolverInterface *  si = emptySi->clone();
      
    si->loadProblem(*exmip1Si->getMatrixByRow(),
		    NULL,NULL,NULL,
		    exmip1Si->getRowSense(),
		    exmip1Si->getRightHandSide(),
		    exmip1Si->getRowRange());
    // Test column settings
    OSIUNITTEST_ASSERT_ERROR(si->getNumCols()==exmip1Si->getNumCols(), return, solverName, "testLoadAndAssignProblem: loadProblem with matrix and row bounds only");
    bool collower_ok = true;
    bool colupper_ok = true;
    bool colobjcoef_ok = true;
    for ( i=0; i<si->getNumCols(); i++ ) {
      collower_ok &= eq(si->getColLower()[i],0.0);
      colupper_ok &= eq(si->getColUpper()[i],si->getInfinity());
      colobjcoef_ok &= eq(si->getObjCoefficients()[i],0.0);
    }
    OSIUNITTEST_ASSERT_ERROR(collower_ok == true, return, solverName, "testLoadAndAssignProblem: loadProblem with matrix and row bounds only");
    OSIUNITTEST_ASSERT_ERROR(colupper_ok == true, return, solverName, "testLoadAndAssignProblem: loadProblem with matrix and row bounds only");
    OSIUNITTEST_ASSERT_ERROR(colobjcoef_ok == true, return, solverName, "testLoadAndAssignProblem: loadProblem with matrix and row bounds only");
    // Test row settings
    OSIUNITTEST_ASSERT_ERROR(si->getNumRows()==exmip1Si->getNumRows(), return, solverName, "testLoadAndAssignProblem: loadProblem with matrix and row bounds only");
    bool rowrhs_ok = true;
    bool rowrange_ok = true;
    bool rowsense_ok = true;
    bool rowlower_ok = true;
    bool rowupper_ok = true;
    for ( i=0; i<si->getNumRows(); i++ ) {
    	rowrhs_ok &= eq(si->getRightHandSide()[i], exmip1Si->getRightHandSide()[i]);
    	rowrange_ok &= eq(si->getRowRange()[i], exmip1Si->getRowRange()[i]);
    	rowsense_ok &= si->getRowSense()[i]==exmip1Si->getRowSense()[i];

    	char s = si->getRowSense()[i];
    	if ( s=='G' ) {
    		rowlower_ok &= eq(si->getRowLower()[i], exmip1Si->getRightHandSide()[i]);
    		rowupper_ok &= eq(si->getRowUpper()[i], si->getInfinity());
    	}
    	else if ( s=='L' ) {
    		rowlower_ok &= eq(si->getRowLower()[i], -si->getInfinity());
    		rowupper_ok &= eq(si->getRowUpper()[i], exmip1Si->getRightHandSide()[i]);
    	}
    	else if ( s=='E' ) {
    		rowlower_ok &= eq(si->getRowLower()[i], si->getRowUpper()[i]);
    		rowupper_ok &= eq(si->getRowUpper()[i],	exmip1Si->getRightHandSide()[i]);
    	}
    	else if ( s=='N' ) {
    		rowlower_ok &= eq(si->getRowLower()[i], -si->getInfinity());
    		rowupper_ok &= eq(si->getRowUpper()[i],  si->getInfinity());
    	}
    	else if ( s=='R' ) {
    		rowlower_ok &= eq(si->getRowLower()[i], exmip1Si->getRightHandSide()[i] - exmip1Si->getRowRange()[i]);
    		rowupper_ok &= eq(si->getRowUpper()[i], exmip1Si->getRightHandSide()[i]);
    	}
    }
    OSIUNITTEST_ASSERT_ERROR(rowrhs_ok   == true, return, solverName, "testLoadAndAssignProblem: loadProblem with matrix and row bounds only");
    OSIUNITTEST_ASSERT_ERROR(rowrange_ok == true, return, solverName, "testLoadAndAssignProblem: loadProblem with matrix and row bounds only");
    OSIUNITTEST_ASSERT_ERROR(rowsense_ok == true, return, solverName, "testLoadAndAssignProblem: loadProblem with matrix and row bounds only");
    OSIUNITTEST_ASSERT_ERROR(rowlower_ok == true, return, solverName, "testLoadAndAssignProblem: loadProblem with matrix and row bounds only");
    OSIUNITTEST_ASSERT_ERROR(rowupper_ok == true, return, solverName, "testLoadAndAssignProblem: loadProblem with matrix and row bounds only");
      
    delete si;
  }

  return ;
}

/*
  Test adding rows and columns to an empty constraint system.
*/
void testAddToEmptySystem (const OsiSolverInterface *emptySi,
			   bool volSolverInterface)

{
  CoinRelFltEq eq(1.0e-7) ;

  std::string solverName = "Unknown solver" ;
  emptySi->getStrParam(OsiSolverName,solverName) ;
/*
  Add rows to an empty system. Begin by creating empty columns, then add some
  rows.
*/
  {
    OsiSolverInterface *  si = emptySi->clone();
    int i;

    //Matrix
    int column[]={0,1,2};
    double row1E[]={4.0,7.0,5.0};
    double row2E[]={7.0,4.0,5.0};
    CoinPackedVector row1(3,column,row1E);
    CoinPackedVector row2(3,column,row2E);

    double objective[]={5.0,6.0,5.5};

    {
      // Add empty columns
      for (i=0;i<3;i++)
      { const CoinPackedVector reqdBySunCC ;
      	si->addCol(reqdBySunCC,0.0,10.0,objective[i]) ; }

      // Add rows
      si->addRow(row1,2.0,100.0);
      si->addRow(row2,2.0,100.0);

      // Vol can not solve problem of this form
      if ( !volSolverInterface ) {
      	// solve
      	si->initialSolve();
        OSIUNITTEST_ASSERT_ERROR(eq(si->getObjValue(),2.0), {}, solverName, "testAddToEmptySystem: getObjValue after adding empty columns");
      }
    }

    delete si;
  }
  // Test adding rows to NULL - alternative row vector format
  {
    OsiSolverInterface *  si = emptySi->clone();
    int i;

    //Matrix
    int column[]={0,1,2,0,1,2};
    double row1E[]={4.0,7.0,5.0};
    double row2E[]={7.0,4.0,5.0};
    double row12E[]={4.0,7.0,5.0,7.0,4.0,5.0};
    int starts[]={0,3,6};
    double ub[]={100.0,100.0};

    double objective[]={5.0,6.0,5.5};

    {
      // Add empty columns
      for (i=0;i<3;i++)
      { const CoinPackedVector reqdBySunCC ;
      	si->addCol(reqdBySunCC,0.0,10.0,objective[i]) ; }
      
      // Add rows
      si->addRows(2,starts,column,row12E,NULL,ub);
      // and again
      si->addRow(3,column,row1E,2.0,100.0);
      si->addRow(3,column,row2E,2.0,100.0);
      
      // Vol can not solve problem of this form
      if ( !volSolverInterface ) {
      	// solve
      	si->initialSolve();
        OSIUNITTEST_ASSERT_ERROR(eq(si->getObjValue(),2.0), {}, solverName, "testAddToEmptySystem: getObjValue after adding empty columns and then rows");
      }
    }

    delete si;
  }
/*
  Add columns to an empty system. Start by creating empty rows, then add
  some columns.
*/
  {
    OsiSolverInterface *  si = emptySi->clone();
    int i;

    //Matrix
    int row[]={0,1};
    double col1E[]={4.0,7.0};
    double col2E[]={7.0,4.0};
    double col3E[]={5.0,5.0};
    CoinPackedVector col1(2,row,col1E);
    CoinPackedVector col2(2,row,col2E);
    CoinPackedVector col3(2,row,col3E);

    double objective[]={5.0,6.0,5.5};
    {
      // Add empty rows
      for (i=0;i<2;i++)
      { const CoinPackedVector reqdBySunCC ;
      	si->addRow(reqdBySunCC,2.0,100.0) ; }

      // Add columns
      if ( volSolverInterface ) {
      	// FIXME: this test could be done w/ the volume, but the rows must not be ranged.
      	OSIUNITTEST_ADD_OUTCOME(solverName, "testAddToEmptySystem", "addCol adds columns to NULL", TestOutcome::WARNING, true);
      	failureMessage(solverName,"addCol add columns to null");
      }
      else {
      	si->addCol(col1,0.0,10.0,objective[0]);
      	si->addCol(col2,0.0,10.0,objective[1]);
      	si->addCol(col3,0.0,10.0,objective[2]);

      	// solve
      	si->initialSolve();

      	CoinRelFltEq eq(1.0e-7) ;
        OSIUNITTEST_ASSERT_ERROR(eq(si->getObjValue(),2.0), {}, solverName, "testAddToEmptySystem: getObjValue after adding empty rows and then columns");
      }
    }
    delete si;
  }
  // Test adding columns to NULL - alternative column vector format
  {
    OsiSolverInterface *  si = emptySi->clone();
    int i;

    //Matrix
    int row[]={0,1};
    double col1E[]={4.0,7.0};
    double col23E[]={7.0,4.0,5.0,5.0};
    int row23E[]={0,1,0,1};
    int start23E[]={0,2,4};
    double ub23E[]={10.0,10.0};

    double objective[]={5.0,6.0,5.5};
    {
      // Add empty rows
      for (i=0;i<2;i++)
      { const CoinPackedVector reqdBySunCC ;
      	si->addRow(reqdBySunCC,2.0,100.0) ; }
      
      // Add columns
      if ( volSolverInterface ) {
      	// FIXME: this test could be done w/ the volume, but the rows must not be ranged.
      	OSIUNITTEST_ADD_OUTCOME(solverName, "testAddToEmptySystem", "addCol adds columns to NULL", TestOutcome::WARNING, true);
      }
      else {
      	si->addCols(2,start23E,row23E,col23E,NULL,ub23E,objective+1);
      	si->addCol(2,row,col1E,0.0,10.0,objective[0]);

      	// solve
      	si->initialSolve();
        OSIUNITTEST_ASSERT_ERROR(eq(si->getObjValue(),2.0), {}, solverName, "testAddToEmptySystem: getObjValue after adding empty rows and then columns (alternative format)");
      }
    }
    delete si;
  }
}

/*
  OsiPresolve has the property that it will report the correct (untransformed)
  objective for the presolved problem.

  Test OsiPresolve by checking the objective that we get by optimising the
  presolved problem. Then postsolve to get back to the original problem
  statement and check that we have the same objective without further
  iterations. This is much more a check on OsiPresolve than on the OsiXXX
  under test. OsiPresolve simply calls the underlying OsiXXX when it needs to
  solve a model. All the work involved with presolve and postsolve transforms
  is handled in OsiPresolve.
  
  The problems are a selection of problems from Data/Sample. In particular,
  e226 is in the list by virtue of having a constant offset (7.113) defined
  for the objective, and p0201 is in the list because presolve (as of 071015)
  finds no reductions.

  The objective for finnis (1.7279106559e+05) is not the same as the
  objective used by Netlib (1.7279096547e+05), but solvers clp, dylp, glpk,
  and cplex agree that it's correct.

  This test could be made stronger, but more brittle, by checking for the
  expected size of the constraint system after presolve. It would also be good
  to add a maximisation problem and check for signs of reduced costs and duals.

  Returns the number of errors encountered.
*/
int testOsiPresolve (const OsiSolverInterface *emptySi,
		   const std::string &sampleDir)

{ typedef std::pair<std::string,double> probPair ;
  std::vector<probPair> sampleProbs ;

  sampleProbs.push_back(probPair("brandy",1.5185098965e+03)) ;
  sampleProbs.push_back(probPair("e226",(-18.751929066+7.113))) ;
//#ifdef COIN_HAS_GRB
//  // for the demo license of Gurobi, model "finnis" is too large, so we skip it in this case
//  if( dynamic_cast<const OsiGrbSolverInterface*>(emptySi) && dynamic_cast<const OsiGrbSolverInterface*>(emptySi)->isDemoLicense() )
//    std::cout << "Skip model finnis in test of OsiPresolve with Gurobi, since we seem to have only a demo license of Gurobi." << std::endl;
//  else
//#endif
  sampleProbs.push_back(probPair("finnis",1.7279106559e+05)) ;
  sampleProbs.push_back(probPair("p0201",6875)) ;

  CoinRelFltEq eq(1.0e-8) ;

  int errs = 0 ;
  int warnings = 0;

  std::string solverName = "Unknown solver" ;
  OSIUNITTEST_ASSERT_ERROR(emptySi->getStrParam(OsiSolverName,solverName) == true, ++errs, solverName, "testOsiPresolve: getStrParam(OsiSolverName)");

  std::cout << "Testing OsiPresolve ... " << std::endl ;

  for (unsigned i = 0 ; i < sampleProbs.size() ; i++)
  { OsiSolverInterface * si = emptySi->clone();
    if (!si->setIntParam(OsiNameDiscipline,1))
      std::cout << "  attempt to switch to lazy names failed." ;

    std::string mpsName = sampleProbs[i].first ;
    double correctObj = sampleProbs[i].second ;

    std::cout << "  testing presolve on " << mpsName << "." << std::endl ;

    std::string fn = sampleDir+mpsName ;
    OSIUNITTEST_ASSERT_ERROR(si->readMps(fn.c_str(),"mps") == 0, delete si; ++errs; continue, solverName, "testOsiPresolve: read MPS");
/*
  Set up for presolve. Allow very slight (1.0e-8) bound relaxation to retain
  feasibility. Discard integrality information (false) and limit the number of
  presolve passes to 5.
*/
    OsiSolverInterface *presolvedModel ;
    OsiPresolve pinfo ;
    presolvedModel = pinfo.presolvedModel(*si,1.0e-8,false,5) ;
    OSIUNITTEST_ASSERT_ERROR(presolvedModel != NULL, delete si; ++errs; continue, solverName, "testOsiPresolve");
/*
  Optimise the presolved model and check the objective.  We need to turn off
  any native presolve, which may or may not affect the objective.
*/
    presolvedModel->setHintParam(OsiDoPresolveInInitial,false,OsiHintDo) ;
    presolvedModel->initialSolve() ;
    OSIUNITTEST_ASSERT_ERROR(eq(correctObj,presolvedModel->getObjValue()), delete si; ++errs; continue, solverName, "testOsiPresolve");
/*
  Postsolve to return to the original formulation. The presolvedModel should
  no longer be needed once we've executed postsolve. Check that we get the
  correct objective without iterations. As before, turn off any native
  presolve.
*/
    pinfo.postsolve(true) ;
    delete presolvedModel ;
    si->setHintParam(OsiDoPresolveInResolve,false,OsiHintDo) ;
    si->resolve() ;
    OSIUNITTEST_ASSERT_ERROR(eq(correctObj,si->getObjValue()), ++errs, solverName, "testOsiPresolve: postsolve objective value");
    OSIUNITTEST_ASSERT_WARNING(si->getIterationCount() == 0, ++warnings, solverName, "testOsiPresolve: postsolve number of iterations");

    delete si ; }

  if (errs == 0)
  { std::cout << "OsiPresolve test ok with " << warnings << " warnings." << std::endl ; }
  else
  { failureMessage(solverName,"errors during OsiPresolve test.") ; }

  return (errs) ; }

/*
  Test the values returned by an empty solver interface.
*/
void testEmptySi (const OsiSolverInterface *emptySi)

{ std::string solverName;
  const OsiSolverInterface *si = emptySi->clone() ;

  std::cout << "Testing empty solver interface ... " << std::endl ;

  si->getStrParam(OsiSolverName,solverName) ;

  OSIUNITTEST_ASSERT_ERROR(si->getNumRows()         == 0,    {}, solverName, "testEmptySi");
  OSIUNITTEST_ASSERT_ERROR(si->getNumCols()         == 0,    {}, solverName, "testEmptySi");
  OSIUNITTEST_ASSERT_ERROR(si->getNumElements()     == 0,    {}, solverName, "testEmptySi");
  OSIUNITTEST_ASSERT_ERROR(si->getColLower()        == NULL, {}, solverName, "testEmptySi");
  OSIUNITTEST_ASSERT_ERROR(si->getColUpper()        == NULL, {}, solverName, "testEmptySi");
  OSIUNITTEST_ASSERT_ERROR(si->getColSolution()     == NULL, {}, solverName, "testEmptySi");
  OSIUNITTEST_ASSERT_ERROR(si->getObjCoefficients() == NULL, {}, solverName, "testEmptySi");
  OSIUNITTEST_ASSERT_ERROR(si->getRowRange()        == NULL, {}, solverName, "testEmptySi");
  OSIUNITTEST_ASSERT_ERROR(si->getRightHandSide()   == NULL, {}, solverName, "testEmptySi");
  OSIUNITTEST_ASSERT_ERROR(si->getRowSense()        == NULL, {}, solverName, "testEmptySi");
  OSIUNITTEST_ASSERT_ERROR(si->getRowLower()        == NULL, {}, solverName, "testEmptySi");
  OSIUNITTEST_ASSERT_ERROR(si->getRowUpper()        == NULL, {}, solverName, "testEmptySi");

  delete si ;
}


/*
  This routine uses the problem galenet (included in Data/Sample) to check
  getDualRays. Galenet is a primal infeasible flow problem:

  s1: t14 <= 20
  s2: t24 + t25 <= 20
  s3: t35 <= 20
  n4: t14 + t24 - t46 - t47 = 0
  n5: t25 + t35 - t57 - t58 = 0
  d6: t46 >= 10
  d7: t47 + t57 >= 20
  d8: t58 >= 30

  t14,t58 <= 30    tt24, t57 <= 20    t25, t35, t46 <= 10    t47 <= 2

  Galenet is the original form, with mixed explicit constraints and implicit
  bound constraints. Galenetbnds is the same problem, but with implicit
  bounds converted to explicit constraints and all constraints converted to
  inequalities so that the algebraic test still works.

  The routine doesn't actually test for specific dual rays; rather, it tests
  for rA >= 0 and rb < 0, on the assumption that the dual constraint system
  matches the canonical form min yb  yA >= c. (More accurately, on the
  assumption that the sign convention of the ray is correct for the canonical
  form.)

  The strategy is to check first for the ability to return a ray with row and
  column components, then a ray with row components only. If both of these
  result in a throw, conclude that the solver does not implement getDualRays.
*/

void testDualRays (const OsiSolverInterface *emptySi,
		  const std::string &sampleDir)

{ unsigned int rayNdx,raysReturned ;
  bool hasGetDualRays = false ;

  std::string solverName ;
  OsiSolverInterface *si = 0 ;

  std::vector<double *> rays ;
  const int raysRequested = 5 ;
  const std::string mpsNames[] = { "galenet", "galenetbnds" } ;
  const bool rayTypes[] = { true, false } ;

  std::cout << "Testing getDualRays ..." << std::endl ;
/*
  Figure out what we can test. getDualRays only makes sense after solving a
  problem, so the strategy is to solve galenet and try for full rays. If that
  fails, solve galenetbnds and try for row-component rays. If that fails,
  conclude that the solver doesn't implement getDualRays.
*/
  for (int iter = 0 ; iter <= 1 ; iter++)
  { const bool fullRay = rayTypes[iter] ;
    const std::string mpsName = mpsNames[iter] ;
    const std::string fn = sampleDir+mpsName ;
    
    si = emptySi->clone() ;
    si->getStrParam(OsiSolverName,solverName) ;
    std::cout
      << "  checking if " << solverName << " implements getDualRays(maxRays"
      << ((fullRay == true)?",true":"") << ") ... " ;

    si->setIntParam(OsiNameDiscipline,1) ;

    OSIUNITTEST_ASSERT_ERROR(si->readMps(fn.c_str(),"mps") == 0, delete si; return, solverName, "testDualRays: read MPS");
/*
  Solve and report the result. We should be primal infeasible, and not optimal.
  Specify maximisation just for kicks.
*/
    si->setObjSense(-1.0) ;
    si->setHintParam(OsiDoPresolveInInitial,false,OsiHintDo) ;
    si->setHintParam(OsiDoReducePrint,true,OsiHintDo) ;
    si->initialSolve() ;
    OSIUNITTEST_ASSERT_ERROR(!si->isProvenOptimal(), {}, solverName, "testDualRays: infeasible instance not proven optimal");
    OSIUNITTEST_ASSERT_ERROR( si->isProvenPrimalInfeasible(), {}, solverName, "testDualRays: recognize infeasiblity of instance");
/*
  Try a call to getDualRays. If the call throws, abort this iteration and
  try again.
*/
    try
    { rays = si->getDualRays(raysRequested,fullRay) ;
      hasGetDualRays = true ;
      std::cout << "yes." << std::endl ; }
    catch (CoinError& err)
    { std::cout << "no." << std::endl ;
      delete si ;
      si = 0 ;
      continue ; }
/*
  We have rays. Check to see how many. There should be at least one, and no
  more than the number requested. If there are none, bail out now.
*/
    raysReturned = static_cast<unsigned int>(rays.size()) ;
    OSIUNITTEST_ASSERT_ERROR(raysReturned >= 1, break, solverName, "testDualRays: number of returned rays");
    OSIUNITTEST_ASSERT_WARNING(static_cast<int>(raysReturned) <= raysRequested, {}, solverName, "testDualRays: number of returned rays");
/*
  Do a bit of setup before checking each ray. If we're dealing with a full
  ray, we'll need variable bounds, solution value, and status. Acquire the
  bounds arrays, and acquire a warm start object so we can ask for column
  status. Failure to retrieve a warm start aborts the test.
*/

    unsigned int m,n,i,j ;
    m = si->getNumRows() ;
    n = si->getNumCols() ;
    unsigned int rayLen = m ;
    CoinWarmStartBasis *wsb = 0 ;
    const double *vlbs= 0 ;
    const double *vubs = 0 ;
    const double *xvals = 0 ;
    const double *rhs = si->getRightHandSide() ;
    const char* sense = si->getRowSense();
    if (fullRay == true)
    { rayLen += n ;
      wsb = dynamic_cast<CoinWarmStartBasis *>(si->getWarmStart()) ;
      OSIUNITTEST_ASSERT_ERROR(wsb != NULL, break, solverName, "testDualRays: get warmstart basis");
       vlbs = si->getColLower() ;
       vubs = si->getColUpper() ;
       xvals = si->getColSolution() ; }

    double tol ;
    si->getDblParam(OsiDualTolerance,tol) ;

    double *rA = new double[rayLen] ;
/*
  Open a loop to check each ray for validity.
*/
    for (rayNdx = 0 ; rayNdx < raysReturned ; rayNdx++)
    { double *ray = rays[rayNdx] ;

      if (OsiUnitTest::verbosity >= 2)
      { std::cout << "  Ray[" << rayNdx << "]: " << std::endl ;
        for (i = 0 ; i < m ; i++)
        { if (fabs(ray[i]) > tol)
          { std::cout << "    " << si->getRowName(i) << " [" << i << "]: " << ray[i] << "\t rhs: " << rhs[i] << "\t sense: " << sense[i] << std::endl ; } }
        if (fullRay == true)
        { for (j = 0 ; j < n ; j++)
          { if (fabs(ray[m+j]) > tol)
            { std::cout << "    " << si->getColName(j) << " [" << j << "]: " << ray[m+j] << std::endl ; } } } }
/*
  Check that the ray is not identically zero.
*/
      for (i = 0 ; i < rayLen ; i++)
      { if (fabs(ray[i]) > tol) break ; }
      OSIUNITTEST_ASSERT_ERROR(i < rayLen, continue, solverName, "testDualRays: ray should not be zero");
/*
  Check that dot(r,b) < 0. For the first m components this is a
  straightforward dot product. If we're dealing with column components, we
  need to synthesize the coefficient on-the-fly. There can be at most one
  nonzero associated with an out-of-bound basic primal, which corresponds to
  the nonbasic dual that's driving the ray.
*/
      double rdotb = 0.0 ;
      int nzoobCnt = 0 ;
      for (i = 0 ; i < m ; i++)
      { rdotb += rhs[i]*ray[i] ; }
      if (fullRay == true)
      { CoinWarmStartBasis::Status statj ;
	      for (j = 0 ; j < n ; j++)
	      { statj = wsb->getStructStatus(j) ;
	        switch (statj)
	        { case CoinWarmStartBasis::atUpperBound:
	          { rdotb += vubs[j]*ray[m+j] ;
	            break ; }
	          case CoinWarmStartBasis::atLowerBound:
	          { rdotb += (-vlbs[j])*ray[m+j] ;
	            break ; }
	          case CoinWarmStartBasis::basic:
	          { if (ray[m+j] != 0)
	            { nzoobCnt++ ;
  	            OSIUNITTEST_ASSERT_ERROR(xvals[j] > vubs[j] || xvals[j] < vlbs[j], break, solverName, "testDualRays: xval outside bounds for nonzero ray entry");
		            if (xvals[j] > vubs[j])
		            { rdotb += vubs[j]*ray[m+j] ; }
                else if (xvals[j] < vlbs[j])
		            { rdotb += (-vlbs[j])*ray[m+j] ; }
	            }
	            break ; }
	          default:
	          {
	            OSIUNITTEST_ASSERT_ERROR(fabs(ray[i]) <= tol, {}, solverName, "testDualRays: zero ray entry for basic variables");
	            break ; } } }
          OSIUNITTEST_ASSERT_ERROR(nzoobCnt <= 1, {}, solverName, "testDualRays: at most one nonzero ray entry for basic variables");
      }
      if (OsiUnitTest::verbosity >= 2)
        std::cout << "dot(r,b) = " << rdotb << std::endl;
      OSIUNITTEST_ASSERT_ERROR(rdotb < 0, {}, solverName, "testDualRays: ray points into right direction");
/*
  On to rA >= 0. As with dot(r,b), it's trivially easy to do the calculation
  for explicit constraints, but we have to synthesize the coefficients
  corresponding to bounded variables on-the-fly. Upper bounds look like
  x<j> <= u<j>, lower bounds -x<j> <= -l<j>. No need to repeat the ray
  coefficient tests.
*/
      CoinFillN(rA,m,0.0) ;
      si->getMatrixByCol()->transposeTimes(ray,rA) ;
      if (fullRay == true)
      { CoinWarmStartBasis::Status statj ;
        for (j = 0 ; j < n ; j++)
        { statj = wsb->getStructStatus(j) ;
          switch (statj)
          { case CoinWarmStartBasis::atUpperBound:
            { rA[j] += ray[m+j] ;
              break ; }
            case CoinWarmStartBasis::atLowerBound:
            { rA[j] += -ray[m+j] ;
              break ; }
            case CoinWarmStartBasis::basic:
            { if (ray[m+j] != 0)
              { if (xvals[j] > vubs[j])
                  rA[j] += ray[m+j] ;
                else if (xvals[j] < vlbs[j])
      	          rA[j] += -ray[m+j] ; }
              break ; }
            default:
            { break ; } } } }

      bool badVal = false ;
      for (j = 0 ; j < n ; j++)
      { if (rA[j] < -tol)
	      { std::cout << "  " << solverName << ": ray[" << rayNdx << "] fails rA >= 0 for column " << j << " with value " << rA[j] << "." << std::endl;
	        badVal = true ; } }
      OSIUNITTEST_ASSERT_ERROR(badVal == false, {}, solverName, "testDualRays: rA >= 0");
      if (badVal == true && OsiUnitTest::verbosity >= 2)
      { std::cout << "  Ray[" << rayNdx << "]: " << std::endl ;
	      for (i = 0 ; i < m ; i++)
	      { if (fabs(ray[i]) > tol)
	        { std::cout << "    [" << i << "]: " << ray[i] << std::endl ; } } } }
/*
  Clean up.
*/
    delete [] rA ;
    for (rayNdx = 0 ; rayNdx < raysReturned ; rayNdx++)
    { delete [] rays[rayNdx] ; }
    delete si ; }
/*
  Report the result and we're done.
*/
	OSIUNITTEST_ASSERT_SEVERITY_EXPECTED(hasGetDualRays, {}, solverName, "testDualRays: getDualRays is implemented", TestOutcome::NOTE, false);
  if (hasGetDualRays == false)
  { testingMessage("  *** WARNING *** getDualRays is unimplemented.\n") ; }
}


}	// end file-local namespace


//#############################################################################
// The main event
//#############################################################################

/*
  The order of tests should be examined. As it stands, we test immediately
  for the ability to read an mps file and bail if we can't do it. But quite a
  few tests could be performed without reading an mps file.  -- lh, 080107 --

  Gradually, oh so gradually, the Osi unit test is converting to produce some
  information about failed tests, and this routine now returns a count.
  Whenever you revise a test, please take the time to produce a count of
  errors.
*/

void
OsiSolverInterfaceCommonUnitTest(const OsiSolverInterface* emptySi,
				 const std::string & mpsDir,
				 const std::string & /* netlibDir */)
{

  CoinRelFltEq eq ;

/*
  Test if the si knows its name. The name will be used for displaying messages
  when testing.
*/
  std::string solverName ;
  {
    OsiSolverInterface *si = emptySi->clone() ;
    solverName = "Unknown Solver" ;
    OSIUNITTEST_ASSERT_ERROR(si->getStrParam(OsiSolverName,solverName), {}, solverName, "getStrParam(OsiSolverName) supported");
    OSIUNITTEST_ASSERT_ERROR(solverName != "Unknown Solver", {}, solverName, "solver knows its name");
    delete si ;
  }
  { std::string temp = ": running common unit tests.\n" ;
    temp = solverName + temp ;
    testingMessage(temp.c_str()) ; }

/*
  Set a variable so we can easily determine which solver interface we're
  testing. This is so that we can easily decide to omit a test when it's
  beyond the capability of a solver.
*/
  bool volSolverInterface UNUSED = (solverName == "vol");
  bool dylpSolverInterface UNUSED = (solverName == "dylp");
  bool glpkSolverInterface UNUSED = (solverName == "glpk");
  bool xprSolverInterface UNUSED = (solverName == "xpress");
  bool symSolverInterface UNUSED = (solverName == "sym");
  bool grbSolverInterface UNUSED = (solverName == "gurobi");
  bool cpxSolverInterface UNUSED = (solverName == "cplex");
  bool spxSolverInterface UNUSED = (solverName == "soplex");
  
/*
  Test values returned by an empty solver interface.
*/
  testEmptySi(emptySi) ;
/*
  See if we can read an MPS file. We're dead in the water if we can't do this.
*/
  std::string fn = mpsDir+"exmip1" ;
  OsiSolverInterface *exmip1Si = emptySi->clone() ;
  OSIUNITTEST_ASSERT_ERROR(exmip1Si->readMps(fn.c_str(),"mps") == 0, return, *exmip1Si, "read MPS file");
/*
  Test that the solver correctly handles row and column names.
*/
  testNames(emptySi,fn) ;
/*
  Test constants in objective function, dual and primal objective limit
  functions, objective sense (max/min).
  Do not perform test if Vol solver, because it requires problems of a
  special form and can not solve netlib e226.
*/
  if ( !volSolverInterface ) {
    testObjFunctions(emptySi,mpsDir) ;
  } else {
    OSIUNITTEST_ADD_OUTCOME(solverName, "testObjFunctions", "skipped test for OsiVol", OsiUnitTest::TestOutcome::NOTE, true);
  }

  // Test that problem was loaded correctly

  {
    int nc = exmip1Si->getNumCols();
    int nr = exmip1Si->getNumRows();
    OSIUNITTEST_ASSERT_ERROR(nc == 8, return, *exmip1Si, "problem read correctly: number of columns");
    OSIUNITTEST_ASSERT_ERROR(nr == 5, return, *exmip1Si, "problem read correctly: number of rows");

  	const char   * exmip1Sirs  = exmip1Si->getRowSense();
    OSIUNITTEST_ASSERT_ERROR(exmip1Sirs[0]=='G', {}, *exmip1Si, "problem read correctly: row sense");
    OSIUNITTEST_ASSERT_ERROR(exmip1Sirs[1]=='L', {}, *exmip1Si, "problem read correctly: row sense");
    OSIUNITTEST_ASSERT_ERROR(exmip1Sirs[2]=='E', {}, *exmip1Si, "problem read correctly: row sense");
    OSIUNITTEST_ASSERT_ERROR(exmip1Sirs[3]=='R', {}, *exmip1Si, "problem read correctly: row sense");
    OSIUNITTEST_ASSERT_ERROR(exmip1Sirs[4]=='R', {}, *exmip1Si, "problem read correctly: row sense");

    const double * exmip1Sirhs = exmip1Si->getRightHandSide();
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirhs[0],2.5), {}, *exmip1Si, "problem read correctly: row rhs");
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirhs[1],2.1), {}, *exmip1Si, "problem read correctly: row rhs");
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirhs[2],4.0), {}, *exmip1Si, "problem read correctly: row rhs");
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirhs[3],5.0), {}, *exmip1Si, "problem read correctly: row rhs");
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirhs[4],15.), {}, *exmip1Si, "problem read correctly: row rhs");

    const double * exmip1Sirr  = exmip1Si->getRowRange();
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirr[0],0.0), {}, *exmip1Si, "problem read correctly: row range");
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirr[1],0.0), {}, *exmip1Si, "problem read correctly: row range");
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirr[2],0.0), {}, *exmip1Si, "problem read correctly: row range");
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirr[3],5.0-1.8), {}, *exmip1Si, "problem read correctly: row range");
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirr[4],15.0-3.0), {}, *exmip1Si, "problem read correctly: row range");

    const CoinPackedMatrix *goldByCol = BuildExmip1Mtx() ;
    CoinPackedMatrix goldmtx ;
    goldmtx.reverseOrderedCopyOf(*goldByCol) ;
    delete goldByCol ;
    
    CoinPackedMatrix pm;
    pm.setExtraGap(0.0);
    pm.setExtraMajor(0.0);
    pm = *exmip1Si->getMatrixByRow();
    pm.removeGaps();
    OSIUNITTEST_ASSERT_ERROR(goldmtx.isEquivalent(pm), {}, *exmip1Si, "problem read correctly: matrix by row");

    const double * cl = exmip1Si->getColLower();
    OSIUNITTEST_ASSERT_ERROR(eq(cl[0],2.5), {}, *exmip1Si, "problem read correctly: columns lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cl[1],0.0), {}, *exmip1Si, "problem read correctly: columns lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cl[2],0.0), {}, *exmip1Si, "problem read correctly: columns lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cl[3],0.0), {}, *exmip1Si, "problem read correctly: columns lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cl[4],0.5), {}, *exmip1Si, "problem read correctly: columns lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cl[5],0.0), {}, *exmip1Si, "problem read correctly: columns lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cl[6],0.0), {}, *exmip1Si, "problem read correctly: columns lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cl[7],0.0), {}, *exmip1Si, "problem read correctly: columns lower bounds");

    const double * cu = exmip1Si->getColUpper();
    OSIUNITTEST_ASSERT_ERROR(eq(cu[0],exmip1Si->getInfinity()), {}, *exmip1Si, "problem read correctly: columns upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cu[1],4.1), {}, *exmip1Si, "problem read correctly: columns upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cu[2],1.0), {}, *exmip1Si, "problem read correctly: columns upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cu[3],1.0), {}, *exmip1Si, "problem read correctly: columns upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cu[4],4.0), {}, *exmip1Si, "problem read correctly: columns upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cu[5],exmip1Si->getInfinity()), {}, *exmip1Si, "problem read correctly: columns upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cu[6],exmip1Si->getInfinity()), {}, *exmip1Si, "problem read correctly: columns upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cu[7],4.3), {}, *exmip1Si, "problem read correctly: columns upper bounds");

    const double * rl = exmip1Si->getRowLower();
    OSIUNITTEST_ASSERT_ERROR(eq(rl[0],2.5), {}, *exmip1Si, "problem read correctly: rows lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(rl[1],-exmip1Si->getInfinity()), {}, *exmip1Si, "problem read correctly: rows lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(rl[2],4.0), {}, *exmip1Si, "problem read correctly: rows lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(rl[3],1.8), {}, *exmip1Si, "problem read correctly: rows lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(rl[4],3.0), {}, *exmip1Si, "problem read correctly: rows lower bounds");

    const double * ru = exmip1Si->getRowUpper();
    OSIUNITTEST_ASSERT_ERROR(eq(ru[0],exmip1Si->getInfinity()), {}, *exmip1Si, "problem read correctly: rows upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(ru[1],2.1), {}, *exmip1Si, "problem read correctly: rows upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(ru[2],4.0), {}, *exmip1Si, "problem read correctly: rows upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(ru[3],5.0), {}, *exmip1Si, "problem read correctly: rows upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(ru[4],15.), {}, *exmip1Si, "problem read correctly: rows upper bounds");

    const double * objCoef = exmip1Si->getObjCoefficients();
    OSIUNITTEST_ASSERT_ERROR(eq(objCoef[0], 1.0), {}, *exmip1Si, "problem read correctly: objective coefficients");
    OSIUNITTEST_ASSERT_ERROR(eq(objCoef[1], 0.0), {}, *exmip1Si, "problem read correctly: objective coefficients");
    OSIUNITTEST_ASSERT_ERROR(eq(objCoef[2], 0.0), {}, *exmip1Si, "problem read correctly: objective coefficients");
    OSIUNITTEST_ASSERT_ERROR(eq(objCoef[3], 0.0), {}, *exmip1Si, "problem read correctly: objective coefficients");
    OSIUNITTEST_ASSERT_ERROR(eq(objCoef[4], 2.0), {}, *exmip1Si, "problem read correctly: objective coefficients");
    OSIUNITTEST_ASSERT_ERROR(eq(objCoef[5], 0.0), {}, *exmip1Si, "problem read correctly: objective coefficients");
    OSIUNITTEST_ASSERT_ERROR(eq(objCoef[6], 0.0), {}, *exmip1Si, "problem read correctly: objective coefficients");
    OSIUNITTEST_ASSERT_ERROR(eq(objCoef[7],-1.0), {}, *exmip1Si, "problem read correctly: objective coefficients");

    // make sure col solution is something reasonable,
    // that is between upper and lower bounds
    const double * cs = exmip1Si->getColSolution();
    int c;
    bool okColSol=true;
    //double inf = exmip1Si->getInfinity();
    for ( c=0; c<nc; c++ ) {
      // if colSol is not between column bounds then
      // colSol is unreasonable.
      if ( !(cl[c]<=cs[c] && cs[c]<=cu[c]) ) okColSol=false;
      // if at least one column bound is not infinite,
      // then it is unreasonable to have colSol as infinite
      // FIXME: temporarily commented out pending some group thought on the
      //	semantics of this test. -- lh, 03.04.29 --
      // if ( (cl[c]<inf || cu[c]<inf) && cs[c]>=inf ) okColSol=false;
    }
    OSIUNITTEST_ASSERT_WARNING(okColSol, {}, *exmip1Si, "column solution before solve");

    // Test that objective value is correct
    // FIXME: the test checks the primal value. vol fails this, because vol
    // considers the dual value to be the objective value
    /*
       gurobi fails this, because gurobi does not have a solution before a
       model is solved (which makes sense, I (SV) think)

       Eh, well, you can argue the point, but the current OSI spec requires
       that there be a valid solution from the point that the problem is
       loaded. Nothing says it needs to be a good solution. -- lh, 100826 --
    */
    double correctObjValue = CoinPackedVector(nc,objCoef).dotProduct(cs);
    double siObjValue = exmip1Si->getObjValue();
    OSIUNITTEST_ASSERT_SEVERITY_EXPECTED(eq(correctObjValue,siObjValue), {}, *exmip1Si, "solution value before solve", TestOutcome::WARNING, solverName == "Vol");
  }

  // Test matrixByCol method
  {
    const CoinPackedMatrix *goldmtx = BuildExmip1Mtx() ;
    OsiSolverInterface & si = *exmip1Si->clone();
    CoinPackedMatrix sm = *si.getMatrixByCol();
    sm.removeGaps();
    OSIUNITTEST_ASSERT_ERROR(goldmtx->isEquivalent(sm), {}, solverName, "getMatrixByCol");
    delete goldmtx ;

    // Test getting and setting of objective offset
    double objOffset;
    OSIUNITTEST_ASSERT_ERROR(si.getDblParam(OsiObjOffset,objOffset), {}, solverName, "getDblParam(OsiObjOffset)");
    OSIUNITTEST_ASSERT_ERROR(eq(objOffset, 0.0), {}, solverName, "objective offset 0 for exmip1");
    OSIUNITTEST_ASSERT_ERROR(si.setDblParam(OsiObjOffset,3.21), {}, solverName, "setDblParam(OsiObjOffset)");
    si.getDblParam(OsiObjOffset,objOffset);
    OSIUNITTEST_ASSERT_ERROR(eq(objOffset, 3.21), {}, solverName, "storing objective offset");

    delete &si;
  }

  // Test clone
  {
    OsiSolverInterface * si2;
    int ad = 13579;
    {
      OsiSolverInterface * si1 = exmip1Si->clone();
      int ad = 13579;
      si1->setApplicationData(&ad);
      OSIUNITTEST_ASSERT_ERROR(*(static_cast<int *>(si1->getApplicationData())) == ad, {}, solverName, "storing application data");
      si2 = si1->clone();
      delete si1;
    }
    OSIUNITTEST_ASSERT_ERROR(*(static_cast<int *>(si2->getApplicationData())) == ad, {}, solverName, "cloning of application data");

    int nc = si2->getNumCols();
    int nr = si2->getNumRows();
    OSIUNITTEST_ASSERT_ERROR(nc == 8, return, *exmip1Si, "problem cloned: number of columns");
    OSIUNITTEST_ASSERT_ERROR(nr == 5, return, *exmip1Si, "problem cloned: number of rows");

    const char   * exmip1Sirs  = si2->getRowSense();
    OSIUNITTEST_ASSERT_ERROR(exmip1Sirs[0]=='G', {}, solverName, "problem cloned: row sense");
    OSIUNITTEST_ASSERT_ERROR(exmip1Sirs[1]=='L', {}, solverName, "problem cloned: row sense");
    OSIUNITTEST_ASSERT_ERROR(exmip1Sirs[2]=='E', {}, solverName, "problem cloned: row sense");
    OSIUNITTEST_ASSERT_ERROR(exmip1Sirs[3]=='R', {}, solverName, "problem cloned: row sense");
    OSIUNITTEST_ASSERT_ERROR(exmip1Sirs[4]=='R', {}, solverName, "problem cloned: row sense");

    const double * exmip1Sirhs = si2->getRightHandSide();
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirhs[0],2.5), {}, solverName, "problem cloned: row rhs");
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirhs[1],2.1), {}, solverName, "problem cloned: row rhs");
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirhs[2],4.0), {}, solverName, "problem cloned: row rhs");
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirhs[3],5.0), {}, solverName, "problem cloned: row rhs");
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirhs[4],15.), {}, solverName, "problem cloned: row rhs");

    const double * exmip1Sirr  = si2->getRowRange();
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirr[0],0.0), {}, solverName, "problem cloned: row range");
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirr[1],0.0), {}, solverName, "problem cloned: row range");
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirr[2],0.0), {}, solverName, "problem cloned: row range");
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirr[3],5.0-1.8), {}, solverName, "problem cloned: row range");
    OSIUNITTEST_ASSERT_ERROR(eq(exmip1Sirr[4],15.0-3.0), {}, solverName, "problem cloned: row range");

    const CoinPackedMatrix *goldByCol = BuildExmip1Mtx() ;
    CoinPackedMatrix goldmtx ;
    goldmtx.reverseOrderedCopyOf(*goldByCol) ;
    CoinPackedMatrix pm;
    pm.setExtraGap(0.0);
    pm.setExtraMajor(0.0);
    pm = *si2->getMatrixByRow();
    OSIUNITTEST_ASSERT_ERROR(goldmtx.isEquivalent(pm), {}, solverName, "problem cloned: matrix by row");
	  delete goldByCol ;

    const double * cl = si2->getColLower();
    OSIUNITTEST_ASSERT_ERROR(eq(cl[0],2.5), {}, solverName, "problem cloned: columns lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cl[1],0.0), {}, solverName, "problem cloned: columns lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cl[2],0.0), {}, solverName, "problem cloned: columns lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cl[3],0.0), {}, solverName, "problem cloned: columns lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cl[4],0.5), {}, solverName, "problem cloned: columns lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cl[5],0.0), {}, solverName, "problem cloned: columns lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cl[6],0.0), {}, solverName, "problem cloned: columns lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cl[7],0.0), {}, solverName, "problem cloned: columns lower bounds");

    const double * cu = si2->getColUpper();
    OSIUNITTEST_ASSERT_ERROR(eq(cu[0],exmip1Si->getInfinity()), {}, solverName, "problem cloned: columns upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cu[1],4.1), {}, solverName, "problem cloned: columns upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cu[2],1.0), {}, solverName, "problem cloned: columns upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cu[3],1.0), {}, solverName, "problem cloned: columns upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cu[4],4.0), {}, solverName, "problem cloned: columns upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cu[5],exmip1Si->getInfinity()), {}, solverName, "problem cloned: columns upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cu[6],exmip1Si->getInfinity()), {}, solverName, "problem cloned: columns upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(cu[7],4.3), {}, solverName, "problem cloned: columns upper bounds");

    const double * rl = si2->getRowLower();
    OSIUNITTEST_ASSERT_ERROR(eq(rl[0],2.5), {}, solverName, "problem cloned: rows lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(rl[1],-exmip1Si->getInfinity()), {}, solverName, "problem cloned: rows lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(rl[2],4.0), {}, solverName, "problem cloned: rows lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(rl[3],1.8), {}, solverName, "problem cloned: rows lower bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(rl[4],3.0), {}, solverName, "problem cloned: rows lower bounds");

    const double * ru = si2->getRowUpper();
    OSIUNITTEST_ASSERT_ERROR(eq(ru[0],exmip1Si->getInfinity()), {}, solverName, "problem cloned: rows upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(ru[1],2.1), {}, solverName, "problem cloned: rows upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(ru[2],4.0), {}, solverName, "problem cloned: rows upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(ru[3],5.0), {}, solverName, "problem cloned: rows upper bounds");
    OSIUNITTEST_ASSERT_ERROR(eq(ru[4],15.), {}, solverName, "problem cloned: rows upper bounds");

    const double * objCoef = exmip1Si->getObjCoefficients();
    OSIUNITTEST_ASSERT_ERROR(eq(objCoef[0], 1.0), {}, solverName, "problem cloned: objective coefficients");
    OSIUNITTEST_ASSERT_ERROR(eq(objCoef[1], 0.0), {}, solverName, "problem cloned: objective coefficients");
    OSIUNITTEST_ASSERT_ERROR(eq(objCoef[2], 0.0), {}, solverName, "problem cloned: objective coefficients");
    OSIUNITTEST_ASSERT_ERROR(eq(objCoef[3], 0.0), {}, solverName, "problem cloned: objective coefficients");
    OSIUNITTEST_ASSERT_ERROR(eq(objCoef[4], 2.0), {}, solverName, "problem cloned: objective coefficients");
    OSIUNITTEST_ASSERT_ERROR(eq(objCoef[5], 0.0), {}, solverName, "problem cloned: objective coefficients");
    OSIUNITTEST_ASSERT_ERROR(eq(objCoef[6], 0.0), {}, solverName, "problem cloned: objective coefficients");
    OSIUNITTEST_ASSERT_ERROR(eq(objCoef[7],-1.0), {}, solverName, "problem cloned: objective coefficients");

    // make sure col solution is something reasonable,
    // that is between upper and lower bounds
    const double * cs = exmip1Si->getColSolution();
    int c;
    bool okColSol=true;
    //double inf = exmip1Si->getInfinity();
    for ( c=0; c<nc; c++ ) {
      // if colSol is not between column bounds then
      // colSol is unreasonable.
      if( !(cl[c]<=cs[c] && cs[c]<=cu[c]) ) okColSol=false;
      // if at least one column bound is not infinite,
      // then it is unreasonable to have colSol as infinite
      // FIXME: temporarily commented out pending some group thought on the
      //	semantics of this test. -- lh, 03.04.29 --
      // if ( (cl[c]<inf || cu[c]<inf) && cs[c]>=inf ) okColSol=false;
    }
    OSIUNITTEST_ASSERT_WARNING(okColSol, {}, solverName, "problem cloned: column solution before solve");

    // Test getting of objective offset
    double objOffset;
    OSIUNITTEST_ASSERT_ERROR(si2->getDblParam(OsiObjOffset,objOffset), {}, solverName, "problem cloned: getDblParam(OsiObjOffset)");
    OSIUNITTEST_ASSERT_ERROR(eq(objOffset, 0.0), {}, solverName, "problem cloned: objective offset 0.0");
    delete si2;
  }
  // end of clone testing

  // Test apply cuts method
  {
    OsiSolverInterface & im = *(exmip1Si->clone());
    OsiCuts cuts;

    // Generate some cuts
    {
      // Get number of rows and columns in model
      int nr = im.getNumRows();
      int nc = im.getNumCols();
      OSIUNITTEST_ASSERT_ERROR(nr == 5, return, solverName, "apply cuts: number of rows");
      OSIUNITTEST_ASSERT_ERROR(nc == 8, return, solverName, "apply cuts: number of columns");

      // Generate a valid row cut from thin air
      int c;
      {
        int *inx = new int[nc];
        for (c=0;c<nc;c++) inx[c]=c;
        double *el = new double[nc];
        for (c=0;c<nc;c++)
          el[c]=(static_cast<double>(c))*(static_cast<double>(c));

        OsiRowCut rc;
        rc.setRow(nc,inx,el);
        rc.setLb(-100.);
        rc.setUb(100.);
        rc.setEffectiveness(22);

        cuts.insert(rc);
        delete[]el;
        delete[]inx;
      }

      // Generate valid col cut from thin air
      {
        const double * oslColLB = im.getColLower();
        const double * oslColUB = im.getColUpper();
        int *inx = new int[nc];
        for (c=0;c<nc;c++) inx[c]=c;
        double *lb = new double[nc];
        double *ub = new double[nc];
        for (c=0;c<nc;c++) lb[c]=oslColLB[c]+0.001;
        for (c=0;c<nc;c++) ub[c]=oslColUB[c]-0.001;

        OsiColCut cc;
        cc.setLbs(nc,inx,lb);
        cc.setUbs(nc,inx,ub);

        cuts.insert(cc);
        delete [] ub;
        delete [] lb;
        delete [] inx;
      }

      {
        // Generate a row and column cut which are ineffective
        OsiRowCut * rcP= new OsiRowCut;
        rcP->setEffectiveness(-1.);
        cuts.insert(rcP);
        OSIUNITTEST_ASSERT_ERROR(rcP == NULL, {}, solverName, "apply cuts: insert row cut keeps pointer");

        OsiColCut * ccP= new OsiColCut;
        ccP->setEffectiveness(-12.);
        cuts.insert(ccP);
        OSIUNITTEST_ASSERT_ERROR(ccP == NULL, {}, solverName, "apply cuts: insert column cut keeps pointer");
      }
      {
        //Generate inconsistent Row cut
        OsiRowCut rc;
        const int ne=1;
        int inx[ne]={-10};
        double el[ne]={2.5};
        rc.setRow(ne,inx,el);
        rc.setLb(3.);
        rc.setUb(4.);
        OSIUNITTEST_ASSERT_ERROR(!rc.consistent(), {}, solverName, "apply cuts: inconsistent row cut");
        cuts.insert(rc);
      }
      {
        //Generate inconsistent col cut
        OsiColCut cc;
        const int ne=1;
        int inx[ne]={-10};
        double el[ne]={2.5};
        cc.setUbs(ne,inx,el);
        OSIUNITTEST_ASSERT_ERROR(!cc.consistent(), {}, solverName, "apply cuts: inconsistent column cut");
        cuts.insert(cc);
      }
      {
        // Generate row cut which is inconsistent for model m
        OsiRowCut rc;
        const int ne=1;
        int inx[ne]={10};
        double el[ne]={2.5};
        rc.setRow(ne,inx,el);
        OSIUNITTEST_ASSERT_ERROR( rc.consistent(),   {}, solverName, "apply cuts: row cut inconsistent for model only");
        OSIUNITTEST_ASSERT_ERROR(!rc.consistent(im), {}, solverName, "apply cuts: row cut inconsistent for model only");
        cuts.insert(rc);
      }
      {
        // Generate col cut which is inconsistent for model m
        OsiColCut cc;
        const int ne=1;
        int inx[ne]={30};
        double el[ne]={2.0};
        cc.setLbs(ne,inx,el);
        OSIUNITTEST_ASSERT_ERROR( cc.consistent(),   {}, solverName, "apply cuts: column cut inconsistent for model only");
        OSIUNITTEST_ASSERT_ERROR(!cc.consistent(im), {}, solverName, "apply cuts: column cut inconsistent for model only");
        cuts.insert(cc);
      }
      {
        // Generate col cut which is infeasible
        OsiColCut cc;
        const int ne=1;
        int inx[ne]={0};
        double el[ne]={2.0};
        cc.setUbs(ne,inx,el);
        cc.setEffectiveness(1000.);
        OSIUNITTEST_ASSERT_ERROR(cc.consistent(),   {}, solverName, "apply cuts: column cut infeasible for model");
        OSIUNITTEST_ASSERT_ERROR(cc.consistent(im), {}, solverName, "apply cuts: column cut infeasible for model");
        OSIUNITTEST_ASSERT_ERROR(cc.infeasible(im), {}, solverName, "apply cuts: column cut infeasible for model");
        cuts.insert(cc);
      }
    }
    OSIUNITTEST_ASSERT_ERROR(cuts.sizeRowCuts() == 4, {}, solverName, "apply cuts: number of stored row cuts");
    OSIUNITTEST_ASSERT_ERROR(cuts.sizeColCuts() == 5, {}, solverName, "apply cuts: number of stored column cuts");

   {
      OsiSolverInterface::ApplyCutsReturnCode rc = im.applyCuts(cuts);
      OSIUNITTEST_ASSERT_ERROR(rc.getNumIneffective() == 2, {}, solverName, "apply cuts: number of row cuts found ineffective");
      OSIUNITTEST_ASSERT_ERROR(rc.getNumApplied() == 2, {}, solverName, "apply cuts: number of row cuts applied");
      OSIUNITTEST_ASSERT_ERROR(rc.getNumInfeasible() == 1, {}, solverName, "apply cuts: number of row cuts found infeasible");
      OSIUNITTEST_ASSERT_ERROR(rc.getNumInconsistentWrtIntegerModel() == 2, {}, solverName, "apply cuts: number of row cuts found inconsistent wr.t. integer model");
      OSIUNITTEST_ASSERT_ERROR(rc.getNumInconsistent() == 2, {}, solverName, "apply cuts: number of row cuts found inconsistent");
      OSIUNITTEST_ASSERT_ERROR(cuts.sizeCuts() == rc.getNumIneffective() + rc.getNumApplied() + rc.getNumInfeasible() + rc.getNumInconsistentWrtIntegerModel() + rc.getNumInconsistent(), {}, solverName, "apply cuts: consistent count of row cuts");
    }

    delete &im;
  }
  // end of apply cut method testing


/*
  Test setting primal (column) and row (dual) solutions, and test that reduced
  cost and row activity match.

  GUROBI does not support setting solutions (only basis can be set), so we
  skip this test.

  This is a failure of the implementation of OsiGrb. Most solvers do not
  allow you to simply set a solution by giving primal values, it needs to be
  handled in the OsiXXX. That's what OsiGrb should do.  See longer rant where
  OsiGrb exposes Gurobi's inability to handle 'N' constraints. Here, run the
  tests and see the error messages. Shouldn't cause failure because we're not
  yet properly counting errors.
  -- lh, 100826 --
*/
  testSettingSolutions(*exmip1Si) ;

  // Test column type methods
  // skip for vol since it does not support this function

  if ( volSolverInterface ) {
    OSIUNITTEST_ADD_OUTCOME(solverName, "testing column type methods", "skipped test for OsiVol", TestOutcome::NOTE, true);
  } else {
    OsiSolverInterface & fim = *(emptySi->clone());
    std::string fn = mpsDir+"exmip1";
    fim.readMps(fn.c_str(),"mps");

    // exmip1.mps has 2 integer variables with index 2 & 3
    OSIUNITTEST_ASSERT_ERROR( fim.getNumIntegers() == 2, {}, solverName, "column type methods: number of integers");

    OSIUNITTEST_ASSERT_ERROR( fim.isContinuous(0), {}, solverName, "column type methods: isContinuous");
    OSIUNITTEST_ASSERT_ERROR( fim.isContinuous(1), {}, solverName, "column type methods: isContinuous");
    OSIUNITTEST_ASSERT_ERROR(!fim.isContinuous(2), {}, solverName, "column type methods: isContinuous");
    OSIUNITTEST_ASSERT_ERROR(!fim.isContinuous(3), {}, solverName, "column type methods: isContinuous");
    OSIUNITTEST_ASSERT_ERROR( fim.isContinuous(4), {}, solverName, "column type methods: isContinuous");

    OSIUNITTEST_ASSERT_ERROR(!fim.isInteger(0), {}, solverName, "column type methods: isInteger");
    OSIUNITTEST_ASSERT_ERROR(!fim.isInteger(1), {}, solverName, "column type methods: isInteger");
    OSIUNITTEST_ASSERT_ERROR( fim.isInteger(2), {}, solverName, "column type methods: isInteger");
    OSIUNITTEST_ASSERT_ERROR( fim.isInteger(3), {}, solverName, "column type methods: isInteger");
    OSIUNITTEST_ASSERT_ERROR(!fim.isInteger(4), {}, solverName, "column type methods: isInteger");

    OSIUNITTEST_ASSERT_ERROR(!fim.isBinary(0), {}, solverName, "column type methods: isBinary");
    OSIUNITTEST_ASSERT_ERROR(!fim.isBinary(1), {}, solverName, "column type methods: isBinary");
    OSIUNITTEST_ASSERT_ERROR( fim.isBinary(2), {}, solverName, "column type methods: isBinary");
    OSIUNITTEST_ASSERT_ERROR( fim.isBinary(3), {}, solverName, "column type methods: isBinary");
    OSIUNITTEST_ASSERT_ERROR(!fim.isBinary(4), {}, solverName, "column type methods: isBinary");

    OSIUNITTEST_ASSERT_ERROR(!fim.isIntegerNonBinary(0), {}, solverName, "column type methods: isIntegerNonBinary");
    OSIUNITTEST_ASSERT_ERROR(!fim.isIntegerNonBinary(1), {}, solverName, "column type methods: isIntegerNonBinary");
    OSIUNITTEST_ASSERT_ERROR(!fim.isIntegerNonBinary(2), {}, solverName, "column type methods: isIntegerNonBinary");
    OSIUNITTEST_ASSERT_ERROR(!fim.isIntegerNonBinary(3), {}, solverName, "column type methods: isIntegerNonBinary");
    OSIUNITTEST_ASSERT_ERROR(!fim.isIntegerNonBinary(4), {}, solverName, "column type methods: isIntegerNonBinary");

    // Test fractionalIndices
    do {
      double sol[]={1.0, 2.0, 2.9, 3.0, 4.0,0.0,0.0,0.0};
      fim.setColSolution(sol);
      OsiVectorInt fi = fim.getFractionalIndices(1e-5);
      OSIUNITTEST_ASSERT_ERROR(fi.size() == 1, break, solverName, "column type methods: getFractionalIndices");
      OSIUNITTEST_ASSERT_ERROR(fi[0] == 2, {}, solverName, "column type methods: getFractionalIndices");

      // Set integer variables very close to integer values
      sol[2]=5 + .00001/2.;
      sol[3]=8 - .00001/2.;
      fim.setColSolution(sol);
      fi = fim.getFractionalIndices(1e-5);
      OSIUNITTEST_ASSERT_ERROR(fi.size() == 0, {}, solverName, "column type methods: getFractionalIndices");

      // Set integer variables close, but beyond tolerances
      sol[2]=5 + .00001*2.;
      sol[3]=8 - .00001*2.;
      fim.setColSolution(sol);
      fi = fim.getFractionalIndices(1e-5);
      OSIUNITTEST_ASSERT_ERROR(fi.size() == 2, break, solverName, "column type methods: getFractionalIndices");
      OSIUNITTEST_ASSERT_ERROR(fi[0] == 2, {}, solverName, "column type methods: getFractionalIndices");
      OSIUNITTEST_ASSERT_ERROR(fi[1] == 3, {}, solverName, "column type methods: getFractionalIndices");
    } while(false);

    // Change data so column 2 & 3 are integerNonBinary
    fim.setColUpper(2,5.0);
    fim.setColUpper(3,6.0);
    OSIUNITTEST_ASSERT_ERROR(eq(fim.getColUpper()[2],5.0), {}, solverName, "column type methods: convert binary to integer variable");
    OSIUNITTEST_ASSERT_ERROR(eq(fim.getColUpper()[3],6.0), {}, solverName, "column type methods: convert binary to integer variable");
    OSIUNITTEST_ASSERT_ERROR(!fim.isBinary(0), {}, solverName, "column type methods: convert binary to integer variable");
    OSIUNITTEST_ASSERT_ERROR(!fim.isBinary(1), {}, solverName, "column type methods: convert binary to integer variable");
    OSIUNITTEST_ASSERT_ERROR(!fim.isBinary(2), {}, solverName, "column type methods: convert binary to integer variable");
    OSIUNITTEST_ASSERT_ERROR(!fim.isBinary(3), {}, solverName, "column type methods: convert binary to integer variable");
    OSIUNITTEST_ASSERT_ERROR(!fim.isBinary(4), {}, solverName, "column type methods: convert binary to integer variable");
    OSIUNITTEST_ASSERT_ERROR(fim.getNumIntegers() == 2, {}, solverName, "column type methods: convert binary to integer variable");
    OSIUNITTEST_ASSERT_ERROR(!fim.isIntegerNonBinary(0), {}, solverName, "column type methods: convert binary to integer variable");
    OSIUNITTEST_ASSERT_ERROR(!fim.isIntegerNonBinary(1), {}, solverName, "column type methods: convert binary to integer variable");
    OSIUNITTEST_ASSERT_ERROR( fim.isIntegerNonBinary(2), {}, solverName, "column type methods: convert binary to integer variable");
    OSIUNITTEST_ASSERT_ERROR( fim.isIntegerNonBinary(3), {}, solverName, "column type methods: convert binary to integer variable");
    OSIUNITTEST_ASSERT_ERROR(!fim.isIntegerNonBinary(4), {}, solverName, "column type methods: convert binary to integer variable");

    delete &fim;
  }

/*
  Test load and assign methods, and do an initialSolve while we have the
  problem loaded. This routine also puts some stress on cloning --- it creates
  nine simultaneous clones of the OSI under test.
*/
  testLoadAndAssignProblem(emptySi,exmip1Si) ;
  testAddToEmptySystem(emptySi,volSolverInterface) ;
/*
  Test write methods.
*/
  testWriteMps(emptySi,fn) ;
  testWriteLp(emptySi,fn) ;
/*
  Test the simplex portion of the OSI interface.
*/
  testSimplexAPI(emptySi,mpsDir) ;

  // Add a Laci suggested test case
  // Load in a problem as column ordered matrix,
  // extract the row ordered copy,
  // add a row,
  // extract the row ordered copy again and test whether it's ok.
  // (the same can be done with reversing the role
  //  of row and column ordered.)
  {
    OsiSolverInterface *  si = emptySi->clone();

    si->loadProblem(
		    *(exmip1Si->getMatrixByCol()),
		    exmip1Si->getColLower(),
		    exmip1Si->getColUpper(),
		    exmip1Si->getObjCoefficients(),
		    exmip1Si->getRowSense(),
		    exmip1Si->getRightHandSide(),
		    exmip1Si->getRowRange() );

    CoinPackedMatrix pm1 = *(si->getMatrixByRow());

    // Get a row of the matrix to make a cut
    const CoinShallowPackedVector neededBySunCC =
				exmip1Si->getMatrixByRow()->getVector(1) ;
    CoinPackedVector pv = neededBySunCC ;

    pv.setElement(0,3.14*pv.getElements()[0]);

    OsiRowCut rc;
    rc.setRow( pv );
    rc.setLb( exmip1Si->getRowLower()[1]-0.5 );
    rc.setUb( exmip1Si->getRowUpper()[1]-0.5 );

    OsiCuts cuts;
    cuts.insert(rc);

    si->applyCuts(cuts);

    CoinPackedMatrix pm2 = *(si->getMatrixByRow());

    OSIUNITTEST_ASSERT_ERROR(pm1.getNumRows()==pm2.getNumRows()-1, {}, solverName, "switching from column to row ordering: added row");
    int i;
    for( i=0; i<pm1.getNumRows(); ++i ) {
      const CoinShallowPackedVector neededBySunCC1 = pm1.getVector(i) ;
      const CoinShallowPackedVector neededBySunCC2 = pm2.getVector(i) ;
      if (neededBySunCC1 != neededBySunCC2)
      	break;
    }
    OSIUNITTEST_ASSERT_ERROR(i == pm1.getNumRows(), {}, solverName, "switching from column to row ordering: matrix ok");
    OSIUNITTEST_ASSERT_ERROR(pm2.getVector(pm2.getNumRows()-1).isEquivalent(pv), {}, solverName, "switching from column to row ordering: last row equals added cut");

    delete si;
  }
  {
    OsiSolverInterface *  si = emptySi->clone();

    si->loadProblem(
		    *(exmip1Si->getMatrixByRow()),
		    exmip1Si->getColLower(),
		    exmip1Si->getColUpper(),
		    exmip1Si->getObjCoefficients(),
		    exmip1Si->getRowLower(),
		    exmip1Si->getRowUpper() );

    CoinPackedMatrix pm1 = *(si->getMatrixByCol());

    // Get a row of the matrix to make a cut
    const CoinShallowPackedVector neededBySunCC =
				exmip1Si->getMatrixByRow()->getVector(1) ;
    CoinPackedVector pv = neededBySunCC ;
    pv.setElement(0,3.14*pv.getElements()[0]);

    OsiRowCut rc;
    rc.setRow( pv );
    rc.setLb( exmip1Si->getRowLower()[1]-0.5 );
    rc.setUb( exmip1Si->getRowUpper()[1]-0.5 );

    OsiCuts cuts;
    cuts.insert(rc);

    si->applyCuts(cuts);

    CoinPackedMatrix pm2 = *(si->getMatrixByCol());

    OSIUNITTEST_ASSERT_ERROR(pm1.isColOrdered(), {}, solverName, "switching from row to column ordering");
    OSIUNITTEST_ASSERT_ERROR(pm2.isColOrdered(), {}, solverName, "switching from row to column ordering");
    OSIUNITTEST_ASSERT_ERROR(pm1.getNumRows()==pm2.getNumRows()-1, {}, solverName, "switching from row to column ordering: added row");

    CoinPackedMatrix pm1ByRow;
    pm1ByRow.reverseOrderedCopyOf(pm1);
    CoinPackedMatrix pm2ByRow;
    pm2ByRow.reverseOrderedCopyOf(pm2);

    OSIUNITTEST_ASSERT_ERROR(!pm1ByRow.isColOrdered(), {}, solverName, "switching from row to column ordering");
    OSIUNITTEST_ASSERT_ERROR(!pm2ByRow.isColOrdered(), {}, solverName, "switching from row to column ordering");
    OSIUNITTEST_ASSERT_ERROR(pm1ByRow.getNumRows() == pm2ByRow.getNumRows()-1, {}, solverName, "switching from row to column ordering");
    OSIUNITTEST_ASSERT_ERROR(pm1.getNumRows() == pm1ByRow.getNumRows(), {}, solverName, "switching from row to column ordering");
    OSIUNITTEST_ASSERT_ERROR(pm2.getNumRows() == pm2ByRow.getNumRows(), {}, solverName, "switching from row to column ordering");

    int i;
    for( i=0; i<pm1ByRow.getNumRows(); ++i ) {
      const CoinShallowPackedVector neededBySunCC1 = pm1ByRow.getVector(i) ;
      const CoinShallowPackedVector neededBySunCC2 = pm2ByRow.getVector(i) ;
      if (neededBySunCC1 != neededBySunCC2)
      	break;
    }
    OSIUNITTEST_ASSERT_ERROR(i == pm1ByRow.getNumRows(), {}, solverName, "switching from row to column ordering: matrix ok");
    OSIUNITTEST_ASSERT_ERROR(pm2ByRow.getVector(pm2ByRow.getNumRows()-1).isEquivalent(pv), {}, solverName, "switching from row to column ordering: last row is added cut");

    delete si;
  }

  delete exmip1Si;

  {
    // Testing parameter settings
    OsiSolverInterface *  si = emptySi->clone();
    int i;
    int ival;
    double dval;
    bool hint;
    OsiHintStrength hintStrength;
    OSIUNITTEST_ASSERT_ERROR(si->getIntParam (OsiLastIntParam,  ival) == false, {}, solverName, "parameter methods: retrieve last int param");
    OSIUNITTEST_ASSERT_ERROR(si->getDblParam (OsiLastDblParam,  dval) == false, {}, solverName, "parameter methods: retrieve last double param");
    OSIUNITTEST_ASSERT_ERROR(si->getHintParam(OsiLastHintParam, hint) == false, {}, solverName, "parameter methods: retrieve last hint param");
    OSIUNITTEST_ASSERT_ERROR(si->setIntParam (OsiLastIntParam,  0)     == false, {}, solverName, "parameter methods: set last int param");
    OSIUNITTEST_ASSERT_ERROR(si->setDblParam (OsiLastDblParam,  0.0)   == false, {}, solverName, "parameter methods: set last double param");
    OSIUNITTEST_ASSERT_ERROR(si->setHintParam(OsiLastHintParam, false) == false, {}, solverName, "parameter methods: set last hint param");

    bool param_ok = true;
    for (i = 0; i < OsiLastIntParam; ++i) {
      const bool exists = si->getIntParam(static_cast<OsiIntParam>(i), ival);
      // existence and test should result in the same
      param_ok &= (!exists ^ testIntParam(si, i, -1));
      param_ok &= (!exists ^ testIntParam(si, i, 0));
      param_ok &= (!exists ^ testIntParam(si, i, 1));
      param_ok &= (!exists ^ testIntParam(si, i, 9999999));
      param_ok &= (!exists ^ testIntParam(si, i, COIN_INT_MAX));
      if (exists)
      	param_ok &= (si->getIntParam(static_cast<OsiIntParam>(i), ival));
    }
    OSIUNITTEST_ASSERT_ERROR(param_ok == true, {}, solverName, "parameter methods: test intparam");

    param_ok = true;
    for (i = 0; i < OsiLastDblParam; ++i) {
      const bool exists = si->getDblParam(static_cast<OsiDblParam>(i), dval);
      // existence and test should result in the same
      param_ok &= (!exists ^ testDblParam(si, i, -1e50));
      param_ok &= (!exists ^ testDblParam(si, i, -1e10));
      param_ok &= (!exists ^ testDblParam(si, i, -1));
      param_ok &= (!exists ^ testDblParam(si, i, -1e-4));
      param_ok &= (!exists ^ testDblParam(si, i, -1e-15));
      param_ok &= (!exists ^ testDblParam(si, i, 1e50));
      param_ok &= (!exists ^ testDblParam(si, i, 1e10));
      param_ok &= (!exists ^ testDblParam(si, i, 1));
      param_ok &= (!exists ^ testDblParam(si, i, 1e-4));
      param_ok &= (!exists ^ testDblParam(si, i, 1e-15));
      if (exists)
      	param_ok &= (si->setDblParam(static_cast<OsiDblParam>(i), dval));
    }
    OSIUNITTEST_ASSERT_ERROR(param_ok == true, {}, solverName, "parameter methods: test dblparam");

    // test hints --- see testHintParam for detailed explanation.
    { int throws = 0 ;
      param_ok = true;
      for (i = 0 ; i < OsiLastHintParam ; ++i)
      { const bool exists = si->getHintParam(static_cast<OsiHintParam>(i),hint,hintStrength) ;
        param_ok &= (!exists ^ testHintParam(si,i,true,OsiHintIgnore,&throws)) ;
        param_ok &= (!exists ^ testHintParam(si,i,true,OsiHintTry,&throws)) ;
        param_ok &= (!exists ^ testHintParam(si,i,false,OsiHintTry,&throws)) ;
        param_ok &= (!exists ^ testHintParam(si,i,true,OsiHintDo,&throws)) ;
        param_ok &= (!exists ^ testHintParam(si,i,false,OsiHintDo,&throws)) ;
        param_ok &= (!exists ^ testHintParam(si,i,true,OsiForceDo,&throws)) ;
        param_ok &= (!exists ^ testHintParam(si,i,false,OsiForceDo,&throws)) ; }
      std::cout.flush() ;
      std::cerr << "Checked " << static_cast<int>(OsiLastHintParam) << " hints x (true, false) at strength OsiForceDo; " << throws << " throws." << std::endl ;
      OSIUNITTEST_ASSERT_ERROR(param_ok == true, {}, solverName, "parameter methods: test hintparam");
    }

    delete si;
  }
/*
  A test to see if resolve gets the correct answer after changing the
  objective. Safe for Vol, as the result is checked by testing an interval on
  the primal solution.
*/
  changeObjAndResolve(emptySi) ;
/*
  Test OsiPresolve. This is a `bolt on' presolve, distinct from any presolve
  that might be innate to the solver.
*/
  if ( !volSolverInterface && !symSolverInterface )
  { testOsiPresolve(emptySi,mpsDir) ; }
  else
  { OSIUNITTEST_ADD_OUTCOME(solverName, "testOsiPresolved", "skipped test", OsiUnitTest::TestOutcome::NOTE, true); }
/*
  Do a check to see if the solver returns the correct status for artificial
  variables. See the routine for detailed comments. Vol has no basis, hence no
  status.
*/
  if (!volSolverInterface && !symSolverInterface)
    testArtifStatus(emptySi) ;
  else
  	OSIUNITTEST_ADD_OUTCOME(solverName, "testArtifStatus", "skipped test", OsiUnitTest::TestOutcome::NOTE, true);

  // Perform tests that are embodied in functions
  if ( !volSolverInterface && !symSolverInterface)
  {
    typedef bool (*TestFunction)(OsiSolverInterface*);
    std::vector<std::pair<TestFunction, const char*> > test_functions;
    test_functions.push_back(std::pair<TestFunction, const char*>(&test1VivianDeSmedt, "test1VivianDeSmedt"));
    test_functions.push_back(std::pair<TestFunction, const char*>(&test2VivianDeSmedt, "test2VivianDeSmedt"));
    test_functions.push_back(std::pair<TestFunction, const char*>(&test3VivianDeSmedt, "test3VivianDeSmedt"));
    test_functions.push_back(std::pair<TestFunction, const char*>(&test4VivianDeSmedt, "test4VivianDeSmedt"));
    test_functions.push_back(std::pair<TestFunction, const char*>(&test5VivianDeSmedt, "test5VivianDeSmedt"));
    test_functions.push_back(std::pair<TestFunction, const char*>(&test6VivianDeSmedt, "test6VivianDeSmedt"));
    test_functions.push_back(std::pair<TestFunction, const char*>(&test7VivianDeSmedt, "test7VivianDeSmedt"));
    test_functions.push_back(std::pair<TestFunction, const char*>(&test8VivianDeSmedt, "test8VivianDeSmedt"));
    test_functions.push_back(std::pair<TestFunction, const char*>(&test9VivianDeSmedt, "test9VivianDeSmedt"));
    test_functions.push_back(std::pair<TestFunction, const char*>(&test10VivianDeSmedt,"test10VivianDeSmedt"));
    test_functions.push_back(std::pair<TestFunction, const char*>(&test11VivianDeSmedt,"test11VivianDeSmedt"));
    test_functions.push_back(std::pair<TestFunction, const char*>(&test12VivianDeSmedt,"test12VivianDeSmedt"));
    test_functions.push_back(std::pair<TestFunction, const char*>(&test13VivianDeSmedt,"test13VivianDeSmedt"));
    test_functions.push_back(std::pair<TestFunction, const char*>(&test14VivianDeSmedt,"test14VivianDeSmedt"));
    test_functions.push_back(std::pair<TestFunction, const char*>(&test15VivianDeSmedt,"test15VivianDeSmedt"));
    test_functions.push_back(std::pair<TestFunction, const char*>(&test16SebastianNowozin,"test16SebastianNowozin"));
    test_functions.push_back(std::pair<TestFunction, const char*>(&test17SebastianNowozin,"test17SebastianNowozin"));

    unsigned int i;
    for (i = 0; i < test_functions.size(); ++i) {
      OsiSolverInterface *s = emptySi->clone();
      const char * testName = test_functions[i].second;
      {
        bool test = test_functions[i].first(s);
        OSIUNITTEST_ASSERT_ERROR(test == true, {}, solverName, testName);
      }
      delete s;
    }
  }
  else
  	OSIUNITTEST_ADD_OUTCOME(solverName, "test*VivianDeSmedt and test*SebastianNowozin", "skipped test", OsiUnitTest::TestOutcome::NOTE, true);
/*
  Test duals and reduced costs, then dual rays. Vol doesn't react well to
  either test.
*/
  if (!volSolverInterface && !symSolverInterface) {
    testReducedCosts(emptySi,mpsDir) ;
    testDualRays(emptySi,mpsDir) ;
  } else {
  	OSIUNITTEST_ADD_OUTCOME(solverName, "testReducedCosts", "skipped test", OsiUnitTest::TestOutcome::NOTE, true);
  	OSIUNITTEST_ADD_OUTCOME(solverName, "testDualRays", "skipped test", OsiUnitTest::TestOutcome::NOTE, true);
  }
}


  /*
    Orphan comment? If anyone happens to poke at the code that this belongs
    to, move it. My (lh) guess is it should go somewhere in the deSmedt tests.
    I just haven't made time to check them all.

    And I really do want to find this. It'd be a great test case for the dual
    ray routine.

    With this matrix we have a primal/dual infeas problem. Leaving the first
    row makes it primal feas, leaving the first col makes it dual feas.
    All vars are >= 0

    obj: -1  2 -3  4 -5 (min)

          0 -1  0  0 -2  >=  1
          1  0 -3  0  4  >= -2
          0  3  0 -5  0  >=  3
          0  0  5  0 -6  >= -4
          2 -4  0  6  0  >=  5
  */
