// Copyright (C) 2010
// All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

/*! \file OsiUnitTests.hpp

  Utility methods for OSI unit tests.
*/

#ifndef OSISOLVERINTERFACETEST_HPP_
#define OSISOLVERINTERFACETEST_HPP_

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <map>

class OsiSolverInterface;
class CoinPackedVectorBase;

/** A function that tests that a lot of problems given in MPS files (mostly the NETLIB problems) solve properly with all the specified solvers.
 *
 * The routine creates a vector of NetLib problems (problem name, objective,
 * various other characteristics), and a vector of solvers to be tested.
 *
 * Each solver is run on each problem. The run is deemed successful if the
 * solver reports the correct problem size after loading and returns the
 * correct objective value after optimization.

 * If multiple solvers are available, the results are compared pairwise against
 * the results reported by adjacent solvers in the solver vector. Due to
 * limitations of the volume solver, it must be the last solver in vecEmptySiP.
 */
void OsiSolverInterfaceMpsUnitTest
  (const std::vector<OsiSolverInterface*> & vecEmptySiP,
   const std::string& mpsDir);

/** A function that tests the methods in the OsiSolverInterface class.
 * Some time ago, if this method is compiled with optimization,
 * the compilation took 10-15 minutes and the machine pages (has 256M core memory!)...
 */
void OsiSolverInterfaceCommonUnitTest
  (const OsiSolverInterface* emptySi,
   const std::string& mpsDir,
   const std::string& netlibDir);

/** A function that tests the methods in the OsiColCut class. */
void OsiColCutUnitTest
  (const OsiSolverInterface * baseSiP,
   const std::string & mpsDir);

/** A function that tests the methods in the OsiRowCut class. */
void OsiRowCutUnitTest
  (const OsiSolverInterface * baseSiP,
   const std::string & mpsDir);

/** A function that tests the methods in the OsiRowCutDebugger class. */
void OsiRowCutDebuggerUnitTest
  (const OsiSolverInterface * siP,
   const std::string & mpsDir);

/** A function that tests the methods in the OsiCuts class. */
void OsiCutsUnitTest();

/// A namespace so we can define a few `global' variables to use during tests.
namespace OsiUnitTest {

class TestOutcomes;

/*! \brief Verbosity level of unit tests

 0 (default) for minimal output; larger numbers produce more output
*/
extern unsigned int verbosity;

/*! \brief Behaviour on failing a test

 - 0 (= default) continue
 - 1 press any key to continue
 - 2 stop with abort()
*/
extern unsigned int haltonerror;

/*! \brief Test outcomes

  A global TestOutcomes object to store test outcomes during the run of the unit test
  for an OSI.
 */
extern TestOutcomes outcomes;

/*! \brief Print an error message

  Formatted as "XxxSolverInterface testing issue: message" where Xxx is the string
  provided as \p solverName.

  Flushes std::cout before printing to std::cerr.
*/
void failureMessage(const std::string &solverName,
		    const std::string &message) ;
/// \overload
void failureMessage(const OsiSolverInterface &si,
		    const std::string &message) ;

/*! \brief Print an error message, specifying the test name and condition

  Formatted as "XxxSolverInterface testing issue: testname failed: testcond" where
  Xxx is the OsiStrParam::OsiSolverName parameter of the \p si.
  Flushes std::cout before printing to std::cerr.
*/
void failureMessage(const std::string &solverName,
		    const std::string &testname, const std::string &testcond) ;

/// \overload
void failureMessage(const OsiSolverInterface &si,
		    const std::string &testname, const std::string &testcond) ;

/*! \brief Print a message.

  Prints the message as given. Flushes std::cout before printing to std::cerr.
*/
void testingMessage(const char *const msg) ;

/*! \brief Utility method to check equality

  Tests for equality using CoinRelFltEq with tolerance \p tol. Understands the
  notion of solver infinity and obtains the value for infinity from the solver
  interfaces supplied as parameters.
*/
bool equivalentVectors(const OsiSolverInterface * si1,
		       const OsiSolverInterface * si2,
		       double tol, const double * v1, const double * v2, int size) ;

/*! \brief Compare two problems for equality

  Compares the problems held in the two solvers: constraint matrix, row and column
  bounds, column type, and objective. Rows are checked using upper and lower bounds
  and using sense, bound, and range.
*/
bool compareProblems(OsiSolverInterface *osi1, OsiSolverInterface *osi2) ;

/*! \brief Compare a packed vector with an expanded vector

  Checks that all values present in the packed vector are present in the full vector
  and checks that there are no extra entries in the full vector. Uses CoinRelFltEq
  with the default tolerance.
*/
bool isEquivalent(const CoinPackedVectorBase &pv, int n, const double *fv) ;

/*! \brief Process command line parameters.

 An unrecognised keyword which is not in the \p ignorekeywords map will trigger the
 help message and a return value of false. For each keyword in \p ignorekeywords, you
 can specify the number of following parameters that should be ignored.

 This should be replaced with the one of the standard CoinUtils parameter mechanisms.
 */
bool processParameters (int argc, const char **argv,
			std::map<std::string,std::string>& parms,
      const std::map<std::string,int>& ignorekeywords = std::map<std::string,int>());

/// A single test outcome record.
class TestOutcome {
  public:
    /// Test result
    typedef enum {
	    NOTE     = 0,
	    PASSED   = 1,
	    WARNING  = 2,
	    ERROR    = 3,
	    LAST     = 4
    } SeverityLevel;
    /// Print strings for SeverityLevel
    static std::string SeverityLevelName[LAST];
    /// Name of component under test
    std::string     component;
    /// Name of test
    std::string     testname;
    /// Condition being tested
    std::string     testcond;
    /// Test result
    SeverityLevel   severity;
    /// Set to true if problem is expected
    bool            expected;
    /// Name of code file where test executed
    std::string     filename;
    /// Line number in code file where test executed
    int             linenumber;
    /// Standard constructor
    TestOutcome(const std::string& comp, const std::string& tst,
    		const char* cond, SeverityLevel sev,
		const char* file, int line, bool exp = false)
      : component(comp),testname(tst),testcond(cond),severity(sev),
        expected(exp),filename(file),linenumber(line)
    { }
    /// Print the test outcome
    void print() const;
};

/// Utility class to maintain a list of test outcomes.
class TestOutcomes : public std::list<TestOutcome> {
  public:
    /// Add an outcome to the list
    void add(std::string comp, std::string tst, const char* cond,
    	     TestOutcome::SeverityLevel sev, const char* file, int line,
	     bool exp = false)
    { push_back(TestOutcome(comp,tst,cond,sev,file,line,exp)); }

    /*! \brief Add an outcome to the list

      Get the component name from the solver interface.
    */
    void add(const OsiSolverInterface& si, std::string tst, const char* cond,
    	     TestOutcome::SeverityLevel sev, const char* file, int line,
	     bool exp = false);
    /// Print the list of outcomes
    void print() const;
    /*! \brief Count total and expected outcomes at given severity level
    
      Given a severity level, walk the list of outcomes and count the total number
      of outcomes at this severity level and the number expected.
    */
    void getCountBySeverity(TestOutcome::SeverityLevel sev,
    			    int& total, int& expected) const;
};

/// Convert parameter to a string (stringification)
#define OSIUNITTEST_QUOTEME_(x) #x
/// Convert to string with one level of expansion of the parameter
#define OSIUNITTEST_QUOTEME(x) OSIUNITTEST_QUOTEME_(x)

template <typename Component>
bool OsiUnitTestAssertSeverityExpected(
    bool condition, const char * condition_str, const char *filename,
    int line, const Component& component, const std::string& testname,
    TestOutcome::SeverityLevel severity, bool expected)
{
  if (condition) {
    OsiUnitTest::outcomes.add(component, testname, condition_str,
        OsiUnitTest::TestOutcome::PASSED, filename, line, false);
    if (OsiUnitTest::verbosity >= 2) {
      std::ostringstream successmsg;
      successmsg << __FILE__ << ":" << __LINE__ << ": " << testname
          << " (condition \'" << condition_str << "\') passed.\n";
      OsiUnitTest::testingMessage(successmsg.str().c_str());
    }
    return true;
  }
  OsiUnitTest::outcomes.add(component, testname, condition_str,
      severity, filename, line, expected);
  OsiUnitTest::failureMessage(component, testname, condition_str);
  switch (OsiUnitTest::haltonerror) {
    case 2:
    { if (severity >= OsiUnitTest::TestOutcome::ERROR ) std::abort(); break; }
    case 1:
    { std::cout << std::endl << "press any key to continue..." << std::endl;
      std::getchar();
      break ; }
    default: ;
  }
  return false;
}

/// Add a test outcome to the list held in OsiUnitTest::outcomes
#define OSIUNITTEST_ADD_OUTCOME(component,testname,testcondition,severity,expected) \
    OsiUnitTest::outcomes.add(component,testname,testcondition,severity,\
    __FILE__,__LINE__,expected)
/*! \brief Test for a condition and record the result

  Test \p condition and record the result in OsiUnitTest::outcomes.
  If it succeeds, record the result as OsiUnitTest::TestOutcome::PASSED and print
  a message for OsiUnitTest::verbosity >= 2.
  If it fails, record the test as failed with \p severity and \p expected and
  react as specified by OsiUnitTest::haltonerror.

  \p failurecode is executed when failure is not fatal.
*/
#define OSIUNITTEST_ASSERT_SEVERITY_EXPECTED(condition,failurecode,component,\
					     testname, severity, expected) \
{ \
  if (!OsiUnitTestAssertSeverityExpected(condition, #condition, \
      __FILE__, __LINE__, component, testname, severity, expected)) { \
    failurecode; \
  } \
}

/*! \brief Perform a test with severity OsiUnitTest::TestOutcome::ERROR, failure not
 	   expected.
*/
#define OSIUNITTEST_ASSERT_ERROR(condition, failurecode, component, testname) \
  OSIUNITTEST_ASSERT_SEVERITY_EXPECTED(condition,failurecode,component,testname,\
  				       OsiUnitTest::TestOutcome::ERROR,false)

/*! \brief Perform a test with severity OsiUnitTest::TestOutcome::WARNING, failure
	   not expected.
*/
#define OSIUNITTEST_ASSERT_WARNING(condition, failurecode, component, testname) \
  OSIUNITTEST_ASSERT_SEVERITY_EXPECTED(condition,failurecode,component,testname,\
  				       OsiUnitTest::TestOutcome::WARNING,false)

/*! \brief Perform a test surrounded by a try/catch block

  \p trycode is executed in a try/catch block; if there's no throw the test is deemed
  to have succeeded and is recorded in OsiUnitTest::outcomes with status
  OsiUnitTest::TestOutcome::PASSED. If the \p trycode throws a CoinError, the failure
  is recorded with status \p severity and \p expected and the value of
  OsiUnitTest::haltonerror is consulted. If the failure is not fatal, \p catchcode is
  executed. If any other error is thrown, the failure is recorded as for a CoinError
  and \p catchcode is executed (haltonerror is not consulted).
*/
#define OSIUNITTEST_CATCH_SEVERITY_EXPECTED(trycode, catchcode, component, testname,\
					    severity, expected) \
{ \
  try { \
    trycode; \
    OSIUNITTEST_ADD_OUTCOME(component,testname,#trycode " did not throw exception",\
			    OsiUnitTest::TestOutcome::PASSED,false); \
    if (OsiUnitTest::verbosity >= 2) { \
      std::string successmsg( __FILE__ ":" OSIUNITTEST_QUOTEME(__LINE__) ": "); \
      successmsg = successmsg + testname; \
      successmsg = successmsg + " (code \'" #trycode "\') did not throw exception"; \
      successmsg = successmsg + ".\n" ; \
      OsiUnitTest::testingMessage(successmsg.c_str()); \
    } \
  } catch (CoinError& e) { \
    std::stringstream errmsg; \
    errmsg << #trycode " threw CoinError: " << e.message(); \
    if (e.className().length() > 0) \
      errmsg << " in " << e.className(); \
    if (e.methodName().length() > 0) \
      errmsg << " in " << e.methodName(); \
    if (e.lineNumber() >= 0) \
      errmsg << " at " << e.fileName() << ":" << e.lineNumber(); \
    OSIUNITTEST_ADD_OUTCOME(component,testname,errmsg.str().c_str(),\
    			    severity,expected); \
    OsiUnitTest::failureMessage(component,testname,errmsg.str().c_str()); \
    switch(OsiUnitTest::haltonerror) { \
      case 2: \
      { if (severity >= OsiUnitTest::TestOutcome::ERROR) abort(); break; } \
      case 1: \
      { std::cout << std::endl << "press any key to continue..." << std::endl; \
        getchar(); \
	break ; } \
      default: ; \
    } \
    catchcode; \
  } catch (...) { \
    std::string errmsg; \
    errmsg = #trycode; \
    errmsg = errmsg + " threw unknown exception"; \
    OSIUNITTEST_ADD_OUTCOME(component,testname,errmsg.c_str(),severity,false); \
    OsiUnitTest::failureMessage(component,testname,errmsg.c_str()); \
    catchcode; \
  } \
}

/*! \brief Perform a try/catch test with severity OsiUnitTest::TestOutcome::ERROR,
	   failure not expected.
*/
#define OSIUNITTEST_CATCH_ERROR(trycode, catchcode, component, testname) \
	OSIUNITTEST_CATCH_SEVERITY_EXPECTED(trycode, catchcode, component, testname, OsiUnitTest::TestOutcome::ERROR, false)

/*! \brief Perform a try/catch test with severity OsiUnitTest::TestOutcome::WARNING,
	   failure not expected.
*/
#define OSIUNITTEST_CATCH_WARNING(trycode, catchcode, component, testname) \
	OSIUNITTEST_CATCH_SEVERITY_EXPECTED(trycode, catchcode, component, testname, OsiUnitTest::TestOutcome::WARNING, false)

} // end namespace OsiUnitTest

#endif /*OSISOLVERINTERFACETEST_HPP_*/
