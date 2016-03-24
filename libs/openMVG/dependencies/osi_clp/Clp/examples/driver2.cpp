/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "ClpSimplex.hpp"
#include "ClpPresolve.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinTime.hpp"
#include "ClpDualRowSteepest.hpp"
#include "ClpPrimalColumnSteepest.hpp"
#include <iomanip>
// This driver shows how to trap messages - this is just as in unitTest.cpp
// ****** THis code is similar to MyMessageHandler.hpp and MyMessagehandler.cpp
#include "CoinMessageHandler.hpp"

/** This just adds a model to CoinMessage and a void pointer so
    user can trap messages and do useful stuff.
    This is used in Clp/Test/unitTest.cpp

    The file pointer is just there as an example of user stuff.

*/
class ClpSimplex;

class MyMessageHandler : public CoinMessageHandler {

public:
     /**@name Overrides */
     //@{
     virtual int print();
     //@}
     /**@name set and get */
     //@{
     /// Model
     const ClpSimplex * model() const;
     void setModel(ClpSimplex * model);
     //@}

     /**@name Constructors, destructor */
     //@{
     /** Default constructor. */
     MyMessageHandler();
     /// Constructor with pointer to model
     MyMessageHandler(ClpSimplex * model,
                      FILE * userPointer = NULL);
     /** Destructor */
     virtual ~MyMessageHandler();
     //@}

     /**@name Copy method */
     //@{
     /** The copy constructor. */
     MyMessageHandler(const MyMessageHandler&);
     /** The copy constructor from an CoinSimplexMessageHandler. */
     MyMessageHandler(const CoinMessageHandler&);

     MyMessageHandler& operator= (const MyMessageHandler&);
     /// Clone
     virtual CoinMessageHandler * clone() const ;
     //@}


protected:
     /**@name Data members
        The data members are protected to allow access for derived classes. */
     //@{
     /// Pointer back to model
     ClpSimplex * model_;
     //@}
};


//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
MyMessageHandler::MyMessageHandler()
     : CoinMessageHandler(),
       model_(NULL)
{
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
MyMessageHandler::MyMessageHandler(const MyMessageHandler & rhs)
     : CoinMessageHandler(rhs),
       model_(rhs.model_)
{
}

MyMessageHandler::MyMessageHandler(const CoinMessageHandler & rhs)
     : CoinMessageHandler(),
       model_(NULL)
{
}

// Constructor with pointer to model
MyMessageHandler::MyMessageHandler(ClpSimplex * model,
                                   FILE * userPointer)
     : CoinMessageHandler(),
       model_(model)
{
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
MyMessageHandler::~MyMessageHandler()
{
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
MyMessageHandler &
MyMessageHandler::operator= (const MyMessageHandler & rhs)
{
     if (this != &rhs) {
          CoinMessageHandler::operator= (rhs);
          model_ = rhs.model_;
     }
     return *this;
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
CoinMessageHandler * MyMessageHandler::clone() const
{
     return new MyMessageHandler(*this);
}
// Print out values from first 20 messages
static int times = 0;
int
MyMessageHandler::print()
{
     // You could have added a callback flag if you had wanted - see Clp_C_Interface.c
     times++;
     if (times <= 20) {
          int messageNumber = currentMessage().externalNumber();
          if (currentSource() != "Clp")
               messageNumber += 1000000;
          int i;
          int nDouble = numberDoubleFields();
          printf("%d doubles - ", nDouble);
          for (i = 0; i < nDouble; i++)
               printf("%g ", doubleValue(i));
          printf("\n");;
          int nInt = numberIntFields();
          printf("%d ints - ", nInt);
          for (i = 0; i < nInt; i++)
               printf("%d ", intValue(i));
          printf("\n");;
          int nString = numberStringFields();
          printf("%d strings - ", nString);
          for (i = 0; i < nString; i++)
               printf("%s ", stringValue(i).c_str());
          printf("\n");;
     }
     return CoinMessageHandler::print();
}
const ClpSimplex *
MyMessageHandler::model() const
{
     return model_;
}
void
MyMessageHandler::setModel(ClpSimplex * model)
{
     model_ = model;
}

int main(int argc, const char *argv[])
{
     ClpSimplex  model;
     // Message handler
     MyMessageHandler messageHandler(&model);
     std::cout << "Testing derived message handler" << std::endl;
     model.passInMessageHandler(&messageHandler);
     int status;
     // Keep names when reading an mps file
     if (argc < 2) {
#if defined(SAMPLEDIR)
          status = model.readMps(SAMPLEDIR "/p0033.mps", true);
#else
          fprintf(stderr, "Do not know where to find sample MPS files.\n");
          exit(1);
#endif
     } else
          status = model.readMps(argv[1], true);

     if (status) {
          fprintf(stderr, "Bad readMps %s\n", argv[1]);
          fprintf(stdout, "Bad readMps %s\n", argv[1]);
          exit(1);
     }

     double time1 = CoinCpuTime();
     /*
       This driver shows how to do presolve.by hand (rather than with initialSolve)
     */
     ClpSimplex * model2;
     ClpPresolve pinfo;
     int numberPasses = 5; // can change this
     /* Use a tolerance of 1.0e-8 for feasibility, treat problem as
        not being integer, do "numberpasses" passes and throw away names
        in presolved model */
     model2 = pinfo.presolvedModel(model, 1.0e-8, false, numberPasses, false);
     if (!model2) {
          fprintf(stderr, "ClpPresolve says %s is infeasible with tolerance of %g\n",
                  argv[1], 1.0e-8);
          fprintf(stdout, "ClpPresolve says %s is infeasible with tolerance of %g\n",
                  argv[1], 1.0e-8);
          // model was infeasible - maybe try again with looser tolerances
          model2 = pinfo.presolvedModel(model, 1.0e-7, false, numberPasses, false);
          if (!model2) {
               fprintf(stderr, "ClpPresolve says %s is infeasible with tolerance of %g\n",
                       argv[1], 1.0e-7);
               fprintf(stdout, "ClpPresolve says %s is infeasible with tolerance of %g\n",
                       argv[1], 1.0e-7);
               exit(2);
          }
     }
     // change factorization frequency from 200
     model2->setFactorizationFrequency(100 + model2->numberRows() / 50);
     if (argc < 3 || !strstr(argv[2], "primal")) {
          // Use the dual algorithm unless user said "primal"
          /* faster if bounds tightened as then dual can flip variables
          to other bound to stay dual feasible.  We can trash the bounds as
          this model is going to be thrown away
          */
          int numberInfeasibilities = model2->tightenPrimalBounds();
          if (numberInfeasibilities)
               std::cout << "** Analysis indicates model infeasible"
                         << std::endl;
          model2->crash(1000.0, 2);
          ClpDualRowSteepest steep(1);
          model2->setDualRowPivotAlgorithm(steep);
          model2->dual();
     } else {
          ClpPrimalColumnSteepest steep(1);
          model2->setPrimalColumnPivotAlgorithm(steep);
          model2->primal();
     }
     pinfo.postsolve(true);

     int numberIterations = model2->numberIterations();;
     delete model2;
     /* After this postsolve model should be optimal.
        We can use checkSolution and test feasibility */
     model.checkSolution();
     if (model.numberDualInfeasibilities() ||
               model.numberPrimalInfeasibilities())
          printf("%g dual %g(%d) Primal %g(%d)\n",
                 model.objectiveValue(),
                 model.sumDualInfeasibilities(),
                 model.numberDualInfeasibilities(),
                 model.sumPrimalInfeasibilities(),
                 model.numberPrimalInfeasibilities());
     // But resolve for safety
     model.primal(1);

     numberIterations += model.numberIterations();;
     // for running timing tests
     std::cout << argv[1] << " Objective " << model.objectiveValue() << " took " <<
               numberIterations << " iterations and " <<
               CoinCpuTime() - time1 << " seconds" << std::endl;

     std::string modelName;
     model.getStrParam(ClpProbName, modelName);
     std::cout << "Model " << modelName << " has " << model.numberRows() << " rows and " <<
               model.numberColumns() << " columns" << std::endl;

     // remove this to print solution

     exit(0);

     /*
       Now to print out solution.  The methods used return modifiable
       arrays while the alternative names return const pointers -
       which is of course much more virtuous.

       This version just does non-zero columns

      */
#if 0
     int numberRows = model.numberRows();

     // Alternatively getRowActivity()
     double * rowPrimal = model.primalRowSolution();
     // Alternatively getRowPrice()
     double * rowDual = model.dualRowSolution();
     // Alternatively getRowLower()
     double * rowLower = model.rowLower();
     // Alternatively getRowUpper()
     double * rowUpper = model.rowUpper();
     // Alternatively getRowObjCoefficients()
     double * rowObjective = model.rowObjective();

     // If we have not kept names (parameter to readMps) this will be 0
     assert(model.lengthNames());

     // Row names
     const std::vector<std::string> * rowNames = model.rowNames();


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
#endif
     std::cout << "--------------------------------------" << std::endl;

     // Columns

     int numberColumns = model.numberColumns();

     // Alternatively getColSolution()
     double * columnPrimal = model.primalColumnSolution();
     // Alternatively getReducedCost()
     double * columnDual = model.dualColumnSolution();
     // Alternatively getColLower()
     double * columnLower = model.columnLower();
     // Alternatively getColUpper()
     double * columnUpper = model.columnUpper();
     // Alternatively getObjCoefficients()
     double * columnObjective = model.objective();

     // If we have not kept names (parameter to readMps) this will be 0
     assert(model.lengthNames());

     // Column names
     const std::vector<std::string> * columnNames = model.columnNames();


     int iColumn;

     std::cout << "                       Primal          Dual         Lower         Upper          Cost"
               << std::endl;

     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          double value;
          value = columnPrimal[iColumn];
          if (fabs(value) > 1.0e-8) {
               std::cout << std::setw(6) << iColumn << " " << std::setw(8) << (*columnNames)[iColumn];
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
     }
     std::cout << "--------------------------------------" << std::endl;

     return 0;
}
