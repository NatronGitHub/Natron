// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

// Test individual classes or groups of classes

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <iostream>

#include "CoinPragma.hpp"
#include "CoinFinite.hpp"
#include "CoinError.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinSort.hpp"
#include "CoinShallowPackedVector.hpp"
#include "CoinPackedVector.hpp"
#include "CoinDenseVector.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinMpsIO.hpp"
#include "CoinLpIO.hpp"
#include "CoinMessageHandler.hpp"
void CoinModelUnitTest(const std::string & mpsDir,
                       const std::string & netlibDir, const std::string & testModel);
// Function Prototypes. Function definitions is in this file.
void testingMessage( const char * const msg );

//----------------------------------------------------------------
// unitTest [-mpsDir=V1] [-netlibDir=V2] [-testModel=V3]
// 
// where (unix defaults):
//   -mpsDir: directory containing mps test files
//       Default value V1="../../Data/Sample"    
//   -netlibDir: directory containing netlib files
//       Default value V2="../../Data/Netlib"
//   -testModel: name of model in netlibdir for testing CoinModel
//       Default value V3="25fv47.mps"
//
// All parameters are optional.
//----------------------------------------------------------------

int main (int argc, const char *argv[])
{
  /*
    Set default location for Data directory, assuming traditional
    package layout.
  */
  const char dirsep =  CoinFindDirSeparator();
  std::string dataDir ;
  if (dirsep == '/')
    dataDir = "../../Data" ;
  else
    dataDir = "..\\..\\Data" ;
# ifdef COIN_MSVS
    // Visual Studio builds are deeper.
    dataDir = "..\\..\\"+dataDir ;
# endif
  // define valid parameter keywords
  std::set<std::string> definedKeyWords;
  // Really should be sampleDir, but let's not rock the boat.
  definedKeyWords.insert("-mpsDir");
  // Directory for netlib problems.
  definedKeyWords.insert("-netlibDir");
  // Allow for large named model for CoinModel
  definedKeyWords.insert("-testModel");
  /*
    Set parameter defaults.
  */
  std::string mpsDir = dataDir + dirsep + "Sample" + dirsep ;
  std::string netlibDir = dataDir + dirsep + "Netlib" + dirsep ;
  std::string testModel = "p0033.mps" ;
  /*
    Process command line parameters. Assume params of the
    form 'keyword' or 'keyword=value'.
  */
  std::map<std::string,std::string> parms;
  for (int i = 1 ;  i < argc ; i++) {
    std::string parm(argv[i]);
    std::string key,value;
    std::string::size_type eqPos = parm.find('=');
    if (eqPos == std::string::npos) {
      key = parm ;
      value = "" ;
    }
    else {
      key = parm.substr(0,eqPos) ;
      value = parm.substr(eqPos+1) ;
    }
  /*
    Check for valid keyword. Print help and exit on failure.
  */
    if (definedKeyWords.find(key) == definedKeyWords.end()) {
      std::cerr
	  << "Undefined parameter \"" << key << "\".\n"
	  << "Correct usage: \n"
	  << "  unitTest [-mpsDir=V1] [-netlibDir=V2] [-testModel=V3]\n"
	  << "where:\n"
	  << "  -mpsDir: directory containing mps test files\n"
	  << "        Default value V1=\"" << mpsDir << "\"\n"
	  << "  -netlibDir: directory containing netlib files\n"
	  << "        Default value V2=\"" << netlibDir << "\"\n"
	  << "  -testModel: name of model testing CoinModel\n"
	  << "        Default value V3=\"" << testModel << "\"\n";
      return 1 ;
    }
    parms[key] = value ;
  }
  // Deal with any values given on the command line 
  if (parms.find("-mpsDir") != parms.end())
    mpsDir = parms["-mpsDir"] + dirsep;
  if (parms.find("-netlibDir") != parms.end())
    netlibDir = parms["-netlibDir"] + dirsep;
  if (parms.find("-testModel") != parms.end())
    testModel = parms["-testModel"] ;

  bool allOK = true ;

  // *FIXME* : these tests should be written... 
  //  testingMessage( "Testing CoinHelperFunctions\n" );
  //  CoinHelperFunctionsUnitTest();
  //  testingMessage( "Testing CoinSort\n" );
  //  tripleCompareUnitTest();

/*
  Check that finite and isnan are working.
*/
  double finiteVal = 1.0 ;
  double zero = 0.0 ;
  double checkVal ;

  testingMessage( "Testing CoinFinite ... " ) ;
# ifdef COIN_C_FINITE
  checkVal = finiteVal/zero ;
# else
  checkVal = COIN_DBL_MAX ;
# endif
  testingMessage( " finite value: " ) ;
  if (CoinFinite(finiteVal))
  { testingMessage( "ok" ) ; }
  else
  { allOK = false ;
    testingMessage( "ERROR" ) ; }
  testingMessage( "; infinite value: " ) ;
  if (!CoinFinite(checkVal))
  { testingMessage( "ok.\n" ) ; }
  else
  { allOK = false ;
    testingMessage( "ERROR.\n" ) ; }

# ifdef COIN_C_ISNAN
  testingMessage( "Testing CoinIsnan ... " ) ;
  testingMessage( " finite value: " ) ;
  if (!CoinIsnan(finiteVal))
  { testingMessage( "ok" ) ; }
  else
  { allOK = false ;
    testingMessage( "ERROR" ) ; }
  testingMessage( "; NaN value: " ) ;
  checkVal = checkVal/checkVal ;
  if (CoinIsnan(checkVal))
  { testingMessage( "ok.\n" ) ; }
  else
  { allOK = false ;
    testingMessage( "ERROR.\n" ) ; }
# else
  allOK = false ;
  testingMessage( "ERROR: No functional CoinIsnan.\n" ) ;
# endif


  testingMessage( "Testing CoinModel\n" );
  CoinModelUnitTest(mpsDir,netlibDir,testModel);

  testingMessage( "Testing CoinError\n" );
  CoinErrorUnitTest();

  testingMessage( "Testing CoinShallowPackedVector\n" );
  CoinShallowPackedVectorUnitTest();

  testingMessage( "Testing CoinPackedVector\n" );
  CoinPackedVectorUnitTest();

  testingMessage( "Testing CoinIndexedVector\n" );
  CoinIndexedVectorUnitTest();

  testingMessage( "Testing CoinPackedMatrix\n" );
  CoinPackedMatrixUnitTest();

// At moment CoinDenseVector is not compiling with MS V C++ V6
#if 1
  testingMessage( "Testing CoinDenseVector\n" );
  //CoinDenseVectorUnitTest<int>(0);
  CoinDenseVectorUnitTest<double>(0.0);
  CoinDenseVectorUnitTest<float>(0.0f);
#endif

  testingMessage( "Testing CoinMpsIO\n" );
  CoinMpsIOUnitTest(mpsDir);

  testingMessage( "Testing CoinLpIO\n" );
  CoinLpIOUnitTest(mpsDir);

  testingMessage( "Testing CoinMessageHandler\n" );
  if (!CoinMessageHandlerUnitTest())
  { allOK = false ; }

  if (allOK)
  { testingMessage( "All tests completed successfully.\n" );
    return (0) ; }
  else
  { testingMessage( "\nERROR: " ) ;
    testingMessage(
  	"Errors occurred during testing; please check the output.\n\n");
    return (1) ; }
}

 
// Display message on stdout and stderr
void testingMessage( const char * const msg )
{
  std::cerr <<msg;
  //cout <<endl <<"*****************************************"
  //     <<endl <<msg <<endl;
}

