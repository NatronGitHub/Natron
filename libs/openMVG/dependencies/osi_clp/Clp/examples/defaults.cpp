/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "ClpSimplex.hpp"
#include "ClpPrimalColumnSteepest.hpp"
#include "ClpDualRowSteepest.hpp"
#include <iomanip>

int main(int argc, const char *argv[])
{
     ClpSimplex  model;
     int status;
     // Keep names
     if (argc < 2) {
#if defined(SAMPLEDIR)
          status = model.readMps(SAMPLEDIR "/p0033.mps", true);
#else
          fprintf(stderr, "Do not know where to find sample MPS files.\n");
          exit(1);
#endif
     } else
          status = model.readMps(argv[1], true);
     /*
       This driver is similar to minimum.cpp, but it
       sets all parameter values to their defaults.  The purpose of this
       is to give a list of most of the methods that the end user will need.

       There are also some more methods as for OsiSolverInterface e.g.
       some loadProblem methods and deleteRows and deleteColumns.

       Often two methods do the same thing, one having a name I like
       while the other adheres to OsiSolverInterface standards
     */

     // Use exact devex ( a variant of steepest edge)
     ClpPrimalColumnSteepest primalSteepest;
     model.setPrimalColumnPivotAlgorithm(primalSteepest);


     int integerValue;
     double value;

     // Infeasibility cost
     value = model.infeasibilityCost();
     std::cout << "Default value of infeasibility cost is " << value << std::endl;
     model.setInfeasibilityCost(value);

     if (!status) {
          model.primal();
     }
     // Number of rows and columns - also getNumRows, getNumCols
     std::string modelName;
     model.getStrParam(ClpProbName, modelName);
     std::cout << "Model " << modelName << " has " << model.numberRows() << " rows and " <<
               model.numberColumns() << " columns" << std::endl;

     /* Some parameters as in OsiSolverParameters.  ObjectiveLimits
        are not active yet.  dualTolerance, setDualTolerance,
        primalTolerance and setPrimalTolerance may be used as well */

     model.getDblParam(ClpDualObjectiveLimit, value);
     std::cout << "Value of ClpDualObjectiveLimit is " << value << std::endl;
     model.getDblParam(ClpPrimalObjectiveLimit, value);
     std::cout << "Value of ClpPrimalObjectiveLimit is " << value << std::endl;
     model.getDblParam(ClpDualTolerance, value);
     std::cout << "Value of ClpDualTolerance is " << value << std::endl;
     model.getDblParam(ClpPrimalTolerance, value);
     std::cout << "Value of ClpPrimalTolerance is " << value << std::endl;
     model.getDblParam(ClpObjOffset, value);
     std::cout << "Value of ClpObjOffset is " << value << std::endl;

     // setDblParam(ClpPrimalTolerance) is same as this
     model.getDblParam(ClpPrimalTolerance, value);
     model.setPrimalTolerance(value);

     model.setDualTolerance(model.dualTolerance()) ;

     // Other Param stuff

     // Can also use maximumIterations
     model.getIntParam(ClpMaxNumIteration, integerValue);
     std::cout << "Value of ClpMaxNumIteration is " << integerValue << std::endl;
     model.setMaximumIterations(integerValue);

     // Not sure this works yet
     model.getIntParam(ClpMaxNumIterationHotStart, integerValue);
     std::cout << "Value of ClpMaxNumIterationHotStart is "
               << integerValue << std::endl;

     // Can also use getIterationCount and getObjValue
     /* Status of problem:
         0 - optimal
         1 - primal infeasible
         2 - dual infeasible
         3 - stopped on iterations etc
         4 - stopped due to errors
     */
     std::cout << "Model status is " << model.status() << " after "
               << model.numberIterations() << " iterations - objective is "
               << model.objectiveValue() << std::endl;

     assert(!model.isAbandoned());
     assert(model.isProvenOptimal());
     assert(!model.isProvenPrimalInfeasible());
     assert(!model.isProvenDualInfeasible());
     assert(!model.isPrimalObjectiveLimitReached());
     assert(!model.isDualObjectiveLimitReached());
     assert(!model.isIterationLimitReached());


     // Things to help you determine if optimal
     assert(model.primalFeasible());
     assert(!model.numberPrimalInfeasibilities());
     assert(model.sumPrimalInfeasibilities() < 1.0e-7);
     assert(model.dualFeasible());
     assert(!model.numberDualInfeasibilities());
     assert(model.sumDualInfeasibilities() < 1.0e-7);

     // Save warm start and set to all slack
     unsigned char * basis1 = model.statusCopy();
     model.createStatus();

     // Now create another model and do hot start
     ClpSimplex model2 = model;
     model2.copyinStatus(basis1);
     delete [] basis1;

     // Check model has not got basis (should iterate)
     model.dual();

     // Can use getObjSense
     model2.setOptimizationDirection(model.optimizationDirection());

     // Can use scalingFlag() to check if scaling on
     // But set up scaling
     model2.scaling();

     // Could play with sparse factorization on/off
     model2.setSparseFactorization(model.sparseFactorization());

     // Sets row pivot choice algorithm in dual
     ClpDualRowSteepest dualSteepest;
     model2.setDualRowPivotAlgorithm(dualSteepest);

     // Dual bound (i.e. dual infeasibility cost)
     value = model.dualBound();
     std::cout << "Default value of dual bound is " << value << std::endl;
     model.setDualBound(value);

     // Do some deafult message handling
     // To see real use - see OsiOslSolverInterfaceTest.cpp
     CoinMessageHandler handler;
     model2.passInMessageHandler(& handler);
     model2.newLanguage(CoinMessages::us_en);

     //Increase level of detail
     model2.setLogLevel(4);

     // solve
     model2.dual();
     // flip direction twice and solve
     model2.setOptimizationDirection(-1);
     model2.dual();
     model2.setOptimizationDirection(1);
     //Decrease level of detail
     model2.setLogLevel(1);
     model2.dual();

     /*
       Now for getting at information.  This will not deal with:

       ClpMatrixBase * rowCopy() and ClpMatrixbase * clpMatrix()

       nor with

       double * infeasibilityRay() and double * unboundedRay()
       (NULL returned if none/wrong)
       Up to user to use delete [] on these arrays.

      */


     /*
       Now to print out solution.  The methods used return modifiable
       arrays while the alternative names return const pointers -
       which is of course much more virtuous

      */

     int numberRows = model2.numberRows();

     // Alternatively getRowActivity()
     double * rowPrimal = model2.primalRowSolution();
     // Alternatively getRowPrice()
     double * rowDual = model2.dualRowSolution();
     // Alternatively getRowLower()
     double * rowLower = model2.rowLower();
     // Alternatively getRowUpper()
     double * rowUpper = model2.rowUpper();
     // Alternatively getRowObjCoefficients()
     double * rowObjective = model2.rowObjective();

     // If we have not kept names (parameter to readMps) this will be 0
     assert(model2.lengthNames());

     // Row names
     const std::vector<std::string> * rowNames = model2.rowNames();


     int iRow;

     std::cout << "                       Primal          Dual         Lower         Upper        (Cost)"
               << std::endl;

     for (iRow = 0; iRow < numberRows; iRow++) {
          double value;
          std::cout << std::setw(6) << iRow << " " << std::setw(8) << (*rowNames)[iRow];
          value = rowPrimal[iRow];
          if (fabs(value) < 1.0e5)
               std::cout << std::setiosflags(std::ios::fixed | std::ios::showpoint) << std::setw(14) << value;
          else
               std::cout << std::setiosflags(std::ios::scientific) << std::setw(14) << value;
          value = rowDual[iRow];
          if (fabs(value) < 1.0e5)
               std::cout << std::setiosflags(std::ios::fixed | std::ios::showpoint) << std::setw(14) << value;
          else
               std::cout << std::setiosflags(std::ios::scientific) << std::setw(14) << value;
          value = rowLower[iRow];
          if (fabs(value) < 1.0e5)
               std::cout << std::setiosflags(std::ios::fixed | std::ios::showpoint) << std::setw(14) << value;
          else
               std::cout << std::setiosflags(std::ios::scientific) << std::setw(14) << value;
          value = rowUpper[iRow];
          if (fabs(value) < 1.0e5)
               std::cout << std::setiosflags(std::ios::fixed | std::ios::showpoint) << std::setw(14) << value;
          else
               std::cout << std::setiosflags(std::ios::scientific) << std::setw(14) << value;
          if (rowObjective) {
               value = rowObjective[iRow];
               if (fabs(value) < 1.0e5)
                    std::cout << std::setiosflags(std::ios::fixed | std::ios::showpoint) << std::setw(14) << value;
               else
                    std::cout << std::setiosflags(std::ios::scientific) << std::setw(14) << value;
          }
          std::cout << std::endl;
     }
     std::cout << "--------------------------------------" << std::endl;

     // Columns

     int numberColumns = model2.numberColumns();

     // Alternatively getColSolution()
     double * columnPrimal = model2.primalColumnSolution();
     // Alternatively getReducedCost()
     double * columnDual = model2.dualColumnSolution();
     // Alternatively getColLower()
     double * columnLower = model2.columnLower();
     // Alternatively getColUpper()
     double * columnUpper = model2.columnUpper();
     // Alternatively getObjCoefficients()
     double * columnObjective = model2.objective();

     // If we have not kept names (parameter to readMps) this will be 0
     assert(model2.lengthNames());

     // Column names
     const std::vector<std::string> * columnNames = model2.columnNames();


     int iColumn;

     std::cout << "                       Primal          Dual         Lower         Upper          Cost"
               << std::endl;

     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          double value;
          std::cout << std::setw(6) << iColumn << " " << std::setw(8) << (*columnNames)[iColumn];
          value = columnPrimal[iColumn];
          if (fabs(value) < 1.0e5)
               std::cout << std::setiosflags(std::ios::fixed | std::ios::showpoint) << std::setw(14) << value;
          else
               std::cout << std::setiosflags(std::ios::scientific) << std::setw(14) << value;
          value = columnDual[iColumn];
          if (fabs(value) < 1.0e5)
               std::cout << std::setiosflags(std::ios::fixed | std::ios::showpoint) << std::setw(14) << value;
          else
               std::cout << std::setiosflags(std::ios::scientific) << std::setw(14) << value;
          value = columnLower[iColumn];
          if (fabs(value) < 1.0e5)
               std::cout << std::setiosflags(std::ios::fixed | std::ios::showpoint) << std::setw(14) << value;
          else
               std::cout << std::setiosflags(std::ios::scientific) << std::setw(14) << value;
          value = columnUpper[iColumn];
          if (fabs(value) < 1.0e5)
               std::cout << std::setiosflags(std::ios::fixed | std::ios::showpoint) << std::setw(14) << value;
          else
               std::cout << std::setiosflags(std::ios::scientific) << std::setw(14) << value;
          value = columnObjective[iColumn];
          if (fabs(value) < 1.0e5)
               std::cout << std::setiosflags(std::ios::fixed | std::ios::showpoint) << std::setw(14) << value;
          else
               std::cout << std::setiosflags(std::ios::scientific) << std::setw(14) << value;

          std::cout << std::endl;
     }
     std::cout << "--------------------------------------" << std::endl;

     std::cout << std::resetiosflags(std::ios::fixed | std::ios::showpoint | std::ios::scientific);

     // Now matrix
     CoinPackedMatrix * matrix = model2.matrix();

     const double * element = matrix->getElements();
     const int * row = matrix->getIndices();
     const int * start = matrix->getVectorStarts();
     const int * length = matrix->getVectorLengths();

     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          std::cout << "Column " << iColumn;
          int j;
          for (j = start[iColumn]; j < start[iColumn] + length[iColumn]; j++)
               std::cout << " ( " << row[j] << ", " << element[j] << ")";
          std::cout << std::endl;
     }
     return 0;
}
