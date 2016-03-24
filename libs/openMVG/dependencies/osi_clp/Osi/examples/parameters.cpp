// Example of using COIN-OR OSI
// Demonstrates some problem and solution query methods 
// Also demonstrates some parameter setting

#include <iostream>
#include OSIXXXhpp

int
main(void)
{
  // Create a problem pointer.  We use the base class here.
  OsiSolverInterface *si;

  // When we instantiate the object, we need a specific derived class.
  si = new OSIXXX;

   // Read in an mps file.  This one's from the MIPLIB library.
#if defined(SAMPLEDIR)
   si->readMps(SAMPLEDIR "/p0033");
#else
   fprintf(stderr, "Do not know where to find sample MPS files.\n");
   exit(1);
#endif

  // Display some information about the instance
  int nrows = si->getNumRows();
  int ncols = si->getNumCols();
  int nelem = si->getNumElements();
  std::cout << "This problem has " << nrows << " rows, " 
	    << ncols << " columns, and " << nelem << " nonzeros." 
	    << std::endl;

  double const * upper_bounds = si->getColUpper();
  std::cout << "The upper bound on the first column is " << upper_bounds[0]
	    << std::endl; 
  // All information about the instance is available with similar methods


  // Before solving, indicate some parameters
  si->setIntParam( OsiMaxNumIteration, 10);
  si->setDblParam( OsiPrimalTolerance, 0.001 );

  // Can also read parameters
  std::string solver;
  si->getStrParam( OsiSolverName, solver );
  std::cout << "About to solve with: " << solver << std::endl;

  // Solve the (relaxation of the) problem
  si->initialSolve();

  // Check the solution
  if ( si->isProvenOptimal() ) { 
     std::cout << "Found optimal solution!" << std::endl; 
     std::cout << "Objective value is " << si->getObjValue() << std::endl;

     // Examine solution
     int n = si->getNumCols();
     const double *solution;
     solution = si->getColSolution();

     std::cout << "Solution: ";
     for (int i = 0; i < n; i++)
	std::cout << solution[i] << " ";
     std::cout << std::endl;

     std::cout << "It took " << si->getIterationCount() << " iterations"
	       << " to solve." << std::endl;
  } else {
     std::cout << "Didn't find optimal solution." << std::endl;

     // Check other status functions.  What happened?
     if (si->isProvenPrimalInfeasible())
	std::cout << "Problem is proven to be infeasible." << std::endl;
     if (si->isProvenDualInfeasible())
	std::cout << "Problem is proven dual infeasible." << std::endl;
     if (si->isIterationLimitReached())
	std::cout << "Reached iteration limit." << std::endl;
  }

  return 0;
}
