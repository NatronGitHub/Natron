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

#include "OsiSolverInterface.hpp"
#include "CoinFloatEqual.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPackedMatrix.hpp"
/*
  #include "CoinPackedVector.hpp"
  #include "CoinWarmStartBasis.hpp"
  #include "OsiRowCut.hpp"
  #include "OsiCuts.hpp"
  #include "OsiPresolve.hpp"
*/


namespace OsiUnitTest {

unsigned int verbosity = 0;

unsigned int haltonerror = 0;

TestOutcomes outcomes;

//#############################################################################
// Helper routines for messages.
//#############################################################################

/*
  If anyone is feeling ambitious, it'd be a really good idea to handle i/o for
  the unittest by way of a standard CoinMessageHandler. Might require a bit of
  tweaking in CoinMessageHandler.
*/

// A helper function to write out a message about a test failure
void failureMessage( const std::string & solverName,
                     const std::string & message )
{
    std::string messageText;
    messageText = "*** ";
    messageText += solverName + "SolverInterface testing issue: ";
    messageText += message;
    // flush stdout so that error messages are properly interleaved.
    std::cout.flush() ;
    std::cerr << messageText.c_str() << std::endl;
}

void failureMessage( const OsiSolverInterface & si,
                     const std::string & message )
{
    std::string solverName;
    si.getStrParam(OsiSolverName, solverName);
    failureMessage(solverName, message);
}

void failureMessage( const std::string & solverName, const std::string &testname, const std::string &testcond)
{
    std::string messageText;
    messageText = "*** ";
    messageText += solverName + "SolverInterface testing issue: ";
    messageText += testname + " failed: " + testcond;
    // flush stdout so that error messages are properly interleaved.
    std::cout.flush() ;
    std::cerr << messageText.c_str() << std::endl;
}

void failureMessage( const OsiSolverInterface & si, const std::string &testname, const std::string &testcond)
{
    std::string solverName;
    si.getStrParam(OsiSolverName, solverName);
    failureMessage(solverName, testname, testcond);
}

/*
  Display message on stderr. Flush cout buffer before printing the message,
  so that output comes out in order in spite of buffered cout.
*/
void testingMessage( const char * const msg )
{
    std::cout.flush() ;
    std::cerr << msg;
}
//#############################################################################
// Vector comparison utility.
//#############################################################################

// A helper function to compare the equivalence of two vectors
bool equivalentVectors (const OsiSolverInterface * si1,
                        const OsiSolverInterface * si2,
                        double tol,
                        const double * v1,
                        const double * v2,
                        int size)
{
    bool retVal = true;
    double infty1 = si1->getInfinity();
    double infty2 = si2->getInfinity();
    CoinRelFltEq eq(tol) ;
    int i;
    /*
      Both values must be the same infinity or equal within the specified
      tolerance. Otherwise we have failure.
    */
    for (i = 0 ; i < size ; i++ ) {
        if (!(v1[i] <= -infty1 && v2[i] <= -infty2) &&
                !(v1[i] >=  infty1 && v2[i] >=  infty2) &&
                !eq(v1[i], v2[i]) ) {
            std::cout.flush() ;
            std::cerr << "eq " << i << " " << v1[i] << " " << v2[i] << std::endl;
            retVal = false;
            break;
        }
    }
    return retVal;
}

/*
  Check a packed vector for equality with a full vector. The algorithm is to
  first confirm that the elements of the packed vector are present in the full
  vector, then scan the full vector to make sure there are no additional
  nonzeros.
*/
bool isEquivalent (const CoinPackedVectorBase &pv, int n, const double *fv)
{
    int pvCnt = pv.getNumElements() ;
    const int *indices = pv.getIndices() ;
    const double *elems = pv.getElements() ;

    bool retval = true ;
    CoinRelFltEq eq ;

    for (int v = 0 ; v < pvCnt ; v++) {
        int k = indices[v] ;
        if (!eq(elems[v], fv[k])) {
            retval = false ;
            break ;
        }
    }
    if (retval == true) {
        int fvCnt = 0 ;
        for (int k = 0 ; k < n ; k++) {
            if (!eq(fv[k], 0.0))
                fvCnt++ ;
        }
        if (fvCnt != pvCnt)
            retval = false ;
    }

    return (retval) ;
}


/*
  Method to compare the problem representation held by a pair of solver
  interfaces.
*/
bool compareProblems (OsiSolverInterface *osi1, OsiSolverInterface *osi2)
{

    bool areEquiv = true ;
    std::string si1Name, si2Name ;
    osi1->getStrParam(OsiSolverName, si1Name) ;
    osi2->getStrParam(OsiSolverName, si2Name) ;

    // Compare row and column counts
    int colCnt = 0 ;
    if (osi1->getNumCols() != osi2->getNumCols()) {
        std::cerr
                << "  Unequal column count, "
                << si1Name << " vs. " << si2Name << std::endl ;
        return (false) ;
    } else {
        colCnt = osi1->getNumCols() ;
    }

    int rowCnt = 0 ;
    if (osi1->getNumRows() != osi2->getNumRows()) {
        std::cerr
                << "  Unequal row count, "
                << si1Name << " vs. " << si2Name << std::endl ;
        return (false) ;
    } else {
        rowCnt = osi1->getNumRows() ;
    }

    // Compare column bounds
    areEquiv = equivalentVectors(osi1, osi2, 1.e-10,
                                 osi1->getColLower(), osi2->getColLower(), colCnt) ;
    if (areEquiv == false) {
        std::cerr
                << "  Unequal column lower bounds, "
                << si1Name << " vs. " << si2Name << std::endl ;
        return (false) ;
    }
    areEquiv = equivalentVectors(osi1, osi2, 1.e-10,
                                 osi1->getColUpper(), osi2->getColUpper(), colCnt) ;
    if (areEquiv == false) {
        std::cerr
                << "  Unequal column upper bounds, "
                << si1Name << " vs. " << si2Name << std::endl ;
        return (false) ;
    }

    // Compare row bounds
    areEquiv = equivalentVectors(osi1, osi2, 1.e-10,
                                 osi1->getRowLower(), osi2->getRowLower(), rowCnt) ;
    if (areEquiv == false) {
        std::cerr
                << "  Unequal row lower bounds, "
                << si1Name << " vs. " << si2Name << std::endl ;
        return (false) ;
    }
    areEquiv = equivalentVectors(osi1, osi2, 1.e-10,
                                 osi1->getRowUpper(), osi2->getRowUpper(), rowCnt) ;
    if (areEquiv == false) {
        std::cerr
                << "  Unequal row lower bounds, "
                << si1Name << " vs. " << si2Name << std::endl ;
        return (false) ;
    }

    // Compare row sense
    {
        const char *rowSense1 = osi1->getRowSense() ;
        const char *rowSense2 = osi2->getRowSense() ;
        areEquiv = true ;
        for (int r = 0 ; r < rowCnt && areEquiv == true ; r++) {
            if (rowSense1[r] != rowSense2[r]) {
                areEquiv = false ;
            }
        }
        if (areEquiv == false) {
            std::cerr
                    << "  Unequal row sense, "
                    << si1Name << " vs. " << si2Name << std::endl ;
            return (false) ;
        }
    }

    // Compare row rhs
    areEquiv = equivalentVectors(osi1, osi2, 1.e-10,
                                 osi1->getRightHandSide(), osi2->getRightHandSide(), rowCnt) ;
    if (areEquiv == false) {
        std::cerr
                << "  Unequal right-hand-side, "
                << si1Name << " vs. " << si2Name << std::endl ;
        return (false) ;
    }

    // Compare range
    areEquiv = equivalentVectors(osi1, osi2, 1.e-10,
                                 osi1->getRowRange(), osi2->getRowRange(), rowCnt) ;
    if (areEquiv == false) {
        std::cerr
                << "  Unequal row range, "
                << si1Name << " vs. " << si2Name << std::endl ;
        return (false) ;
    }

    // Compare objective sense
    if (osi1->getObjSense() != osi2->getObjSense()) {
        std::cerr
                << "  Unequal objective sense, "
                << si1Name << " vs. " << si2Name << std::endl ;
        return (false) ;
    }

    // Compare objective coefficients
    areEquiv = equivalentVectors(osi1, osi2, 1.e-10,
                                 osi1->getObjCoefficients(), osi2->getObjCoefficients(), colCnt) ;
    if (areEquiv == false) {
        std::cerr
                << "  Unequal objective coefficients, "
                << si1Name << " vs. " << si2Name << std::endl ;
        return (false) ;
    }

    // Compare number of elements
    if (osi1->getNumElements() != osi2->getNumElements()) {
        std::cerr
                << "  Unequal number of constraint matrix coefficients, "
                << si1Name << " vs. " << si2Name << std::endl ;
        return (false) ;
    }

    // Compare constraint matrix, for both row-major and column-major orderings
    {
        const CoinPackedMatrix *rmm1 = osi1->getMatrixByRow() ;
        const CoinPackedMatrix *rm  = osi2->getMatrixByRow() ;
        if (!rmm1->isEquivalent(*rm)) {
            std::cerr
                    << "  Unequal constraint matrix, row-major ordering, "
                    << si1Name << " vs. " << si2Name << std::endl ;
            return (false) ;
        }
        const CoinPackedMatrix *cmm1 = osi1->getMatrixByCol() ;
        const CoinPackedMatrix *cm  = osi2->getMatrixByCol() ;
        if (!cmm1->isEquivalent(*cm)) {
            std::cerr
                    << "  Unequal constraint matrix, column-major ordering, "
                    << si1Name << " vs. " << si2Name << std::endl ;
            return (false) ;
        }
    }
    // Check column types
    {
        areEquiv = true ;
        for (int j = 0 ; j < colCnt && areEquiv == true ; j++) {
            if (osi1->isContinuous(j) != osi2->isContinuous(j))
                areEquiv = false ;
            if (osi1->isBinary(j) != osi2->isBinary(j))
                areEquiv = false ;
            if (osi1->isIntegerNonBinary(j) != osi2->isIntegerNonBinary(j))
                areEquiv = false ;
            if (osi1->isFreeBinary(j) != osi2->isFreeBinary(j))
                areEquiv = false ;
            if (osi1->isInteger(j) != osi2->isInteger(j))
                areEquiv = false ;
        }
        if (areEquiv == false) {
            std::cerr
                    << "  Unequal variable type, "
                    << si1Name << " vs. " << si2Name << std::endl ;
            return (false) ;
        }
    }
    return (true) ;
}

bool processParameters (int argc, const char **argv, std::map<std::string, std::string>& parms, const std::map<std::string, int>& ignorekeywords)
{
    /*
      Initialise the parameter keywords.
    */
    std::set<std::string> definedKeyWords;
    definedKeyWords.insert("-cerr2cout");
    definedKeyWords.insert("-mpsDir");
    definedKeyWords.insert("-netlibDir");
    definedKeyWords.insert("-miplib3Dir");
    definedKeyWords.insert("-testOsiSolverInterface");
    definedKeyWords.insert("-nobuf");
    definedKeyWords.insert("-cutsOnly");
    definedKeyWords.insert("-verbosity");
    definedKeyWords.insert("-onerror");

    /*
      Set default values for data directories.
     */
    const char dirsep =  CoinFindDirSeparator() ;
    std::string pathTmp ;
    pathTmp = ".." ;
    pathTmp += dirsep ;
    pathTmp += ".." ;
    pathTmp += dirsep ;
    pathTmp += "Data" ;
    pathTmp += dirsep ;
# ifdef COIN_MSVS
    // Visual Studio builds are deeper
    pathTmp = "..\\..\\" + pathTmp ;
# endif

    parms["-mpsDir"] = pathTmp + "Sample"  ;
    parms["-netlibDir"] = pathTmp + "Netlib" ;
    parms["-miplib3Dir"] = pathTmp + "miplib3" ;

    /*
      Read the command line parameters and fill a map of parameter keys and
      associated data. The parser allows for parameters which are only a keyword,
      or parameters of the form keyword=value (no spaces).
     */
    for (int i = 1 ; i < argc ; i++) {
        std::string parm(argv[i]) ;
        std::string key, value ;
        std::string::size_type eqPos = parm.find('=');

        if (eqPos == std::string::npos) {
            key = parm ;
        } else {
            key = parm.substr(0, eqPos) ;
            value = parm.substr(eqPos + 1) ;
        }

        /*
         * Should the specified key be ignored?
         */
        if (ignorekeywords.find(key) != ignorekeywords.end()) {
            assert(ignorekeywords.find(key)->second >= 0);
            i += ignorekeywords.find(key)->second;
            continue;
        }

        /*
          Is the specified key valid?
         */
        if (definedKeyWords.find(key) == definedKeyWords.end()) {
            if( key != "-usage" && key != "-help" )
                std::cerr << "Undefined parameter \"" << key << "\"." << std::endl ;
            std::cerr << "Usage: unitTest [-nobuf] [-mpsDir=V1] [-netlibDir=V2] [-miplibDir=V3] [-testOsiSolverInterface] [-cutsOnly] [-verbosity=num]" << std::endl ;
            std::cerr << "  where:" << std::endl ;
            std::cerr << "  -cerr2cout: redirect cerr to cout; sometimes useful to synchronise cout & cerr." << std::endl;
            std::cerr << "  -mpsDir: directory containing mps test files." << std::endl
                      << "       Default value V1=\"../../Data/Sample\"" << std::endl;
            std::cerr << "  -netlibDir: directory containing netlib files." << std::endl
                      << "       Default value V2=\"../../Data/Netlib\"" << std::endl;
            std::cerr << "  -miplib3Dir: directory containing miplib3 files." << std::endl
                      << "       Default value V3=\"../../Data/miplib3\"" << std::endl;
            std::cerr << "  -testOsiSolverInterface: run each OSI on the netlib problem set." << std::endl
                      << "       Default is to not run the netlib problem set." << std::endl;
            std::cerr << "  -cutsOnly: If specified, only OsiCut tests are run." << std::endl;
            std::cerr << "  -nobuf: use unbuffered output." << std::endl
                      << "       Default is buffered output." << std::endl;
            std::cerr << "  -verbosity: verbosity level of tests output (0-2)." << std::endl
                      << "       Default is 0 (minimal output)." << std::endl;
            std::cerr << "  -onerror: behaviour in case of failing test (continue, wait, stop)." << std::endl
                      << "       Default is continue." << std::endl;
            return false;
        }

        /*
          Valid keyword; stash the value for later reference.
         */
        parms[key] = value;
    }

    /*
      Tack the directory separator onto the data directories so we don't have to
      worry about it later.
     */
    if( parms["-mpsDir"].length() > 0 )
        parms["-mpsDir"] += dirsep ;
    if( parms["-netlibDir"].length() > 0 )
        parms["-netlibDir"] += dirsep ;
    if( parms["-miplib3"].length() > 0 )
        parms["-miplib3Dir"] += dirsep ;

    /*
      Did the user request unbuffered i/o? It seems we need to go after this
      through stdio --- using pubsetbuf(0,0) on the C++ streams has no
      discernible affect. Nor, for that matter, did setting the unitbuf flag on
      the streams. Why? At a guess, sync_with_stdio connects the streams to the
      stdio buffers, and the C++ side isn't programmed to change them?
     */
    if (parms.find("-nobuf") != parms.end()) {
        // std::streambuf *coutBuf, *cerrBuf ;
        // coutBuf = std::cout.rdbuf() ;
        // coutBuf->pubsetbuf(0,0) ;
        // cerrBuf = std::cerr.rdbuf() ;
        // cerrBuf->pubsetbuf(0,0) ;
        setbuf(stderr, 0);
        setbuf(stdout, 0);
    }

    /*
      Did the user request a redirect for cerr? This must occur before any i/o is
      performed.
     */
    if (parms.find("-cerr2cout") != parms.end())
        std::cerr.rdbuf(std::cout.rdbuf());

    /*
     * Did the user set a verbosity level?
     */
    if (parms.find("-verbosity") != parms.end()) {
        char* endptr;
        std::string verbstring = parms["-verbosity"];
        unsigned long verblevel = strtoul(verbstring.c_str(), &endptr, 10);

        if( *endptr != '\0' || verblevel < 0) {
            std::cerr << "verbosity level must be a nonnegative number" << std::endl;
            return false;
        }

        OsiUnitTest::verbosity = static_cast<unsigned int>(verblevel);
    }

    /*
     * Did the user specify what to do in case of a failure?
     */
    if (parms.find("-onerror") != parms.end()) {
        std::string onerror = parms["-onerror"];

        if( onerror == "continue" )
            OsiUnitTest::haltonerror = 0;
        else if( onerror == "wait" )
            OsiUnitTest::haltonerror = 1;
        else if( onerror == "stop" )
            OsiUnitTest::haltonerror = 2;
        else {
            std::cerr << "-onerror must be specified with either 'continue', 'wait', or 'stop'" << std::endl;
            return false;
        }
    }

    return true;
}

std::string TestOutcome::SeverityLevelName[LAST] = {
    "NOTE", "PASSED", "WARNING", "ERROR"
};

void TestOutcome::print() const
{
    printf("%-10s", SeverityLevelName[severity].c_str());
    printf("%-10s", component.c_str());
    printf("%s", testname.c_str());
    printf("\n");

    if( expected )
        printf(" (expected)         ");
    else
        printf("                    ");
    printf("%s\n", testcond.c_str());

    printf("                    ");
    printf("%s:%d\n", filename.c_str(), linenumber);

//	printf("\n");
}

void TestOutcomes::add(const OsiSolverInterface& si, std::string tst, const char* cond, TestOutcome::SeverityLevel sev, const char* file, int line, bool exp)
{
    std::string solverName;
    si.getStrParam(OsiSolverName, solverName);
    push_back(TestOutcome(solverName, tst, cond, sev, file, line, exp));
}

void TestOutcomes::print() const
{
    int count[TestOutcome::LAST];
    int expected[TestOutcome::LAST];
    for( int i = 0; i < TestOutcome::LAST; ++i ) {
        count[i] = 0;
        expected[i] = 0;
    }
    /*
      Walk the list, counting entries at each severity level. Print if verbosity
      level is high enough.
    */
    for (const_iterator it(begin()); it != end(); ++it) {
        ++count[it->severity];
        if (it->expected) ++expected[it->severity];
        if ((it->severity != TestOutcome::PASSED || OsiUnitTest::verbosity >= 2) &&
                (it->severity != TestOutcome::NOTE || OsiUnitTest::verbosity >= 1))
            it->print();
    }
    /// Print summary counts
    for (int i = 0; i < TestOutcome::LAST; ++i)
        printf("Severity %-10s: %4d  thereof expected: %4d\n",
               TestOutcome::SeverityLevelName[i].c_str(), count[i], expected[i]);
}

void TestOutcomes::getCountBySeverity(TestOutcome::SeverityLevel sev,
                                      int& total, int& expected) const
{
    assert(sev >= 0);
    assert(sev < TestOutcome::LAST);

    total = 0;
    expected = 0;

    for( const_iterator it(begin()); it != end(); ++it ) {
        if( it->severity != sev ) continue;
        ++total;
        if( it->expected ) ++expected;
    }
}

} // end OsiUnitTest namespace
