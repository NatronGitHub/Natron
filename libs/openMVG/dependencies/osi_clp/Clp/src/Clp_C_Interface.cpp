// $Id$
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


#include "CoinPragma.hpp"

#include <cmath>
#include <cstring>

#include "CoinHelperFunctions.hpp"
#include "ClpConfig.h"
#include "ClpSimplex.hpp"
#include "ClpInterior.hpp"
#ifndef SLIM_CLP
#include "Idiot.hpp"
#endif
#include <cfloat>
// Get C stuff but with extern C
#define CLP_EXTERN_C
#include "Coin_C_defines.h"

/// To allow call backs
class CMessageHandler : public CoinMessageHandler {

public:
     /**@name Overrides */
     //@{
     virtual int print();
     //@}
     /**@name set and get */
     //@{
     /// Model
     const Clp_Simplex * model() const;
     void setModel(Clp_Simplex * model);
     /// Call back
     void setCallBack(clp_callback callback);
     //@}

     /**@name Constructors, destructor */
     //@{
     /** Default constructor. */
     CMessageHandler();
     /// Constructor with pointer to model
     CMessageHandler(Clp_Simplex * model,
                     FILE * userPointer = NULL);
     /** Destructor */
     virtual ~CMessageHandler();
     //@}

     /**@name Copy method */
     //@{
     /** The copy constructor. */
     CMessageHandler(const CMessageHandler&);
     /** The copy constructor from an CoinSimplexMessageHandler. */
     CMessageHandler(const CoinMessageHandler&);

     CMessageHandler& operator=(const CMessageHandler&);
     /// Clone
     virtual CoinMessageHandler * clone() const ;
     //@}


protected:
     /**@name Data members
        The data members are protected to allow access for derived classes. */
     //@{
     /// Pointer back to model
     Clp_Simplex * model_;
     /// call back
     clp_callback callback_;
     //@}
};


//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
CMessageHandler::CMessageHandler ()
     : CoinMessageHandler(),
       model_(NULL),
       callback_(NULL)
{
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
CMessageHandler::CMessageHandler (const CMessageHandler & rhs)
     : CoinMessageHandler(rhs),
       model_(rhs.model_),
       callback_(rhs.callback_)
{
}

CMessageHandler::CMessageHandler (const CoinMessageHandler & rhs)
     : CoinMessageHandler(rhs),
       model_(NULL),
       callback_(NULL)
{
}

// Constructor with pointer to model
CMessageHandler::CMessageHandler(Clp_Simplex * model,
                                 FILE * )
     : CoinMessageHandler(),
       model_(model),
       callback_(NULL)
{
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
CMessageHandler::~CMessageHandler ()
{
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
CMessageHandler &
CMessageHandler::operator=(const CMessageHandler& rhs)
{
     if (this != &rhs) {
          CoinMessageHandler::operator=(rhs);
          model_ = rhs.model_;
          callback_ = rhs.callback_;
     }
     return *this;
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
CoinMessageHandler * CMessageHandler::clone() const
{
     return new CMessageHandler(*this);
}

int
CMessageHandler::print()
{
     if (callback_) {
          int messageNumber = currentMessage().externalNumber();
          if (currentSource() != "Clp")
               messageNumber += 1000000;
          int i;
          int nDouble = numberDoubleFields();
          assert (nDouble <= 10);
          double vDouble[10];
          for (i = 0; i < nDouble; i++)
               vDouble[i] = doubleValue(i);
          int nInt = numberIntFields();
          assert (nInt <= 10);
          int vInt[10];
          for (i = 0; i < nInt; i++)
               vInt[i] = intValue(i);
          int nString = numberStringFields();
          assert (nString <= 10);
          char * vString[10];
          for (i = 0; i < nString; i++) {
               std::string value = stringValue(i);
               vString[i] = CoinStrdup(value.c_str());
          }
          callback_(model_, messageNumber,
                    nDouble, vDouble,
                    nInt, vInt,
                    nString, vString);
          for (i = 0; i < nString; i++)
               free(vString[i]);

     }
     return CoinMessageHandler::print();
}
const Clp_Simplex *
CMessageHandler::model() const
{
     return model_;
}
void
CMessageHandler::setModel(Clp_Simplex * model)
{
     model_ = model;
}
// Call back
void
CMessageHandler::setCallBack(clp_callback callback)
{
     callback_ = callback;
}

#include "Clp_C_Interface.h"
#include <string>
#include <stdio.h>
#include <iostream>

#if defined(__MWERKS__)
#pragma export on
#endif


COINLIBAPI const char* COINLINKAGE
Clp_Version(void)
{
   return CLP_VERSION;
}
COINLIBAPI int COINLINKAGE
Clp_VersionMajor(void)
{
   return CLP_VERSION_MAJOR;
}
COINLIBAPI int COINLINKAGE Clp_VersionMinor(void)
{
   return CLP_VERSION_MINOR;
}
COINLIBAPI int COINLINKAGE Clp_VersionRelease(void)
{
   return CLP_VERSION_RELEASE;
}

/* Default constructor */
COINLIBAPI Clp_Simplex *  COINLINKAGE
Clp_newModel()
{
     Clp_Simplex * model = new Clp_Simplex;
     model->model_ = new ClpSimplex();
     model->handler_ = NULL;
     return model;
}
/* Destructor */
COINLIBAPI void COINLINKAGE
Clp_deleteModel(Clp_Simplex * model)
{
     delete model->model_;
     delete model->handler_;
     delete model;
}

/* Loads a problem (the constraints on the
    rows are given by lower and upper bounds). If a pointer is NULL then the
    following values are the default:
    <ul>
    <li> <code>colub</code>: all columns have upper bound infinity
    <li> <code>collb</code>: all columns have lower bound 0
    <li> <code>rowub</code>: all rows have upper bound infinity
    <li> <code>rowlb</code>: all rows have lower bound -infinity
    <li> <code>obj</code>: all variables have 0 objective coefficient
    </ul>
*/
/* Just like the other loadProblem() method except that the matrix is
   given in a standard column major ordered format (without gaps). */
COINLIBAPI void COINLINKAGE
Clp_loadProblem (Clp_Simplex * model,  const int numcols, const int numrows,
                 const CoinBigIndex * start, const int* index,
                 const double* value,
                 const double* collb, const double* colub,
                 const double* obj,
                 const double* rowlb, const double* rowub)
{
     const char prefix[] = "Clp_c_Interface::Clp_loadProblem(): ";
     const int  verbose = 0;
     if (verbose > 1) {
          printf("%s numcols = %i, numrows = %i\n",
                 prefix, numcols, numrows);
          printf("%s model = %p, start = %p, index = %p, value = %p\n",
                 prefix, reinterpret_cast<const void *>(model), reinterpret_cast<const void *>(start), reinterpret_cast<const void *>(index), reinterpret_cast<const void *>(value));
          printf("%s collb = %p, colub = %p, obj = %p, rowlb = %p, rowub = %p\n",
                 prefix, reinterpret_cast<const void *>(collb), reinterpret_cast<const void *>(colub), reinterpret_cast<const void *>(obj), reinterpret_cast<const void *>(rowlb), reinterpret_cast<const void *>(rowub));
     }
     model->model_->loadProblem(numcols, numrows, start, index, value,
                                collb, colub, obj, rowlb, rowub);
}
/* read quadratic part of the objective (the matrix part) */
COINLIBAPI void COINLINKAGE
Clp_loadQuadraticObjective(Clp_Simplex * model,
                           const int numberColumns,
                           const CoinBigIndex * start,
                           const int * column,
                           const double * element)
{

     model->model_->loadQuadraticObjective(numberColumns,
                                           start, column, element);

}
/* Read an mps file from the given filename */
COINLIBAPI int COINLINKAGE
Clp_readMps(Clp_Simplex * model, const char *filename,
            int keepNames,
            int ignoreErrors)
{
     return model->model_->readMps(filename, keepNames != 0, ignoreErrors != 0);
}
/* Copy in integer informations */
COINLIBAPI void COINLINKAGE
Clp_copyInIntegerInformation(Clp_Simplex * model, const char * information)
{
     model->model_->copyInIntegerInformation(information);
}
/* Drop integer informations */
COINLIBAPI void COINLINKAGE
Clp_deleteIntegerInformation(Clp_Simplex * model)
{
     model->model_->deleteIntegerInformation();
}
/* Resizes rim part of model  */
COINLIBAPI void COINLINKAGE
Clp_resize (Clp_Simplex * model, int newNumberRows, int newNumberColumns)
{
     model->model_->resize(newNumberRows, newNumberColumns);
}
/* Deletes rows */
COINLIBAPI void COINLINKAGE
Clp_deleteRows(Clp_Simplex * model, int number, const int * which)
{
     model->model_->deleteRows(number, which);
}
/* Add rows */
COINLIBAPI void COINLINKAGE
Clp_addRows(Clp_Simplex * model, int number, const double * rowLower,
            const double * rowUpper,
            const int * rowStarts, const int * columns,
            const double * elements)
{
     model->model_->addRows(number, rowLower, rowUpper, rowStarts, columns, elements);
}

/* Deletes columns */
COINLIBAPI void COINLINKAGE
Clp_deleteColumns(Clp_Simplex * model, int number, const int * which)
{
     model->model_->deleteColumns(number, which);
}
/* Add columns */
COINLIBAPI void COINLINKAGE
Clp_addColumns(Clp_Simplex * model, int number, const double * columnLower,
               const double * columnUpper,
               const double * objective,
               const int * columnStarts, const int * rows,
               const double * elements)
{
     model->model_->addColumns(number, columnLower, columnUpper, objective,
                               columnStarts, rows, elements);
}
/* Change row lower bounds */
COINLIBAPI void COINLINKAGE
Clp_chgRowLower(Clp_Simplex * model, const double * rowLower)
{
     model->model_->chgRowLower(rowLower);
}
/* Change row upper bounds */
COINLIBAPI void COINLINKAGE
Clp_chgRowUpper(Clp_Simplex * model, const double * rowUpper)
{
     model->model_->chgRowUpper(rowUpper);
}
/* Change column lower bounds */
COINLIBAPI void COINLINKAGE
Clp_chgColumnLower(Clp_Simplex * model, const double * columnLower)
{
     model->model_->chgColumnLower(columnLower);
}
/* Change column upper bounds */
COINLIBAPI void COINLINKAGE
Clp_chgColumnUpper(Clp_Simplex * model, const double * columnUpper)
{
     model->model_->chgColumnUpper(columnUpper);
}
/* Change objective coefficients */
COINLIBAPI void COINLINKAGE
Clp_chgObjCoefficients(Clp_Simplex * model, const double * objIn)
{
     model->model_->chgObjCoefficients(objIn);
}
/* Drops names - makes lengthnames 0 and names empty */
COINLIBAPI void COINLINKAGE
Clp_dropNames(Clp_Simplex * model)
{
     model->model_->dropNames();
}
/* Copies in names */
COINLIBAPI void COINLINKAGE
Clp_copyNames(Clp_Simplex * model, const char * const * rowNamesIn,
              const char * const * columnNamesIn)
{
     int iRow;
     std::vector<std::string> rowNames;
     int numberRows = model->model_->numberRows();
     rowNames.reserve(numberRows);
     for (iRow = 0; iRow < numberRows; iRow++) {
          rowNames.push_back(rowNamesIn[iRow]);
     }

     int iColumn;
     std::vector<std::string> columnNames;
     int numberColumns = model->model_->numberColumns();
     columnNames.reserve(numberColumns);
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          columnNames.push_back(columnNamesIn[iColumn]);
     }
     model->model_->copyNames(rowNames, columnNames);
}

/* Number of rows */
COINLIBAPI int COINLINKAGE
Clp_numberRows(Clp_Simplex * model)
{
     return model->model_->numberRows();
}
/* Number of columns */
COINLIBAPI int COINLINKAGE
Clp_numberColumns(Clp_Simplex * model)
{
     return model->model_->numberColumns();
}
/* Primal tolerance to use */
COINLIBAPI double COINLINKAGE
Clp_primalTolerance(Clp_Simplex * model)
{
     return model->model_->primalTolerance();
}
COINLIBAPI void COINLINKAGE
Clp_setPrimalTolerance(Clp_Simplex * model,  double value)
{
     model->model_->setPrimalTolerance(value);
}
/* Dual tolerance to use */
COINLIBAPI double COINLINKAGE
Clp_dualTolerance(Clp_Simplex * model)
{
     return model->model_->dualTolerance();
}
COINLIBAPI void COINLINKAGE
Clp_setDualTolerance(Clp_Simplex * model,  double value)
{
     model->model_->setDualTolerance(value);
}
/* Dual objective limit */
COINLIBAPI double COINLINKAGE
Clp_dualObjectiveLimit(Clp_Simplex * model)
{
     return model->model_->dualObjectiveLimit();
}
COINLIBAPI void COINLINKAGE
Clp_setDualObjectiveLimit(Clp_Simplex * model, double value)
{
     model->model_->setDualObjectiveLimit(value);
}
/* Objective offset */
COINLIBAPI double COINLINKAGE
Clp_objectiveOffset(Clp_Simplex * model)
{
     return model->model_->objectiveOffset();
}
COINLIBAPI void COINLINKAGE
Clp_setObjectiveOffset(Clp_Simplex * model, double value)
{
     model->model_->setObjectiveOffset(value);
}
/* Fills in array with problem name  */
COINLIBAPI void COINLINKAGE
Clp_problemName(Clp_Simplex * model, int maxNumberCharacters, char * array)
{
     std::string name = model->model_->problemName();
     maxNumberCharacters = CoinMin(maxNumberCharacters,
     				   ((int) strlen(name.c_str()))+1) ;
     strncpy(array, name.c_str(), maxNumberCharacters - 1);
     array[maxNumberCharacters-1] = '\0';
}
/* Sets problem name.  Must have \0 at end.  */
COINLIBAPI int COINLINKAGE
Clp_setProblemName(Clp_Simplex * model, int /*maxNumberCharacters*/, char * array)
{
     return model->model_->setStrParam(ClpProbName, array);
}
/* Number of iterations */
COINLIBAPI int COINLINKAGE
Clp_numberIterations(Clp_Simplex * model)
{
     return model->model_->numberIterations();
}
COINLIBAPI void COINLINKAGE
Clp_setNumberIterations(Clp_Simplex * model, int numberIterations)
{
     model->model_->setNumberIterations(numberIterations);
}
/* Maximum number of iterations */
COINLIBAPI int maximumIterations(Clp_Simplex * model)
{
     return model->model_->maximumIterations();
}
COINLIBAPI void COINLINKAGE
Clp_setMaximumIterations(Clp_Simplex * model, int value)
{
     model->model_->setMaximumIterations(value);
}
/* Maximum time in seconds (from when set called) */
COINLIBAPI double COINLINKAGE
Clp_maximumSeconds(Clp_Simplex * model)
{
     return model->model_->maximumSeconds();
}
COINLIBAPI void COINLINKAGE
Clp_setMaximumSeconds(Clp_Simplex * model, double value)
{
     model->model_->setMaximumSeconds(value);
}
/* Returns true if hit maximum iteratio`ns (or time) */
COINLIBAPI int COINLINKAGE
Clp_hitMaximumIterations(Clp_Simplex * model)
{
     return model->model_->hitMaximumIterations() ? 1 : 0;
}
/* Status of problem:
   0 - optimal
   1 - primal infeasible
   2 - dual infeasible
   3 - stopped on iterations etc
   4 - stopped due to errors
*/
COINLIBAPI int COINLINKAGE
Clp_status(Clp_Simplex * model)
{
     return model->model_->status();
}
/* Set problem status */
COINLIBAPI void COINLINKAGE
Clp_setProblemStatus(Clp_Simplex * model, int problemStatus)
{
     model->model_->setProblemStatus(problemStatus);
}
/* Secondary status of problem - may get extended
   0 - none
   1 - primal infeasible because dual limit reached
   2 - scaled problem optimal - unscaled has primal infeasibilities
   3 - scaled problem optimal - unscaled has dual infeasibilities
   4 - scaled problem optimal - unscaled has both dual and primal infeasibilities
*/
COINLIBAPI int COINLINKAGE
Clp_secondaryStatus(Clp_Simplex * model)
{
     return model->model_->secondaryStatus();
}
COINLIBAPI void COINLINKAGE
Clp_setSecondaryStatus(Clp_Simplex * model, int status)
{
     model->model_->setSecondaryStatus(status);
}
/* Direction of optimization (1 - minimize, -1 - maximize, 0 - ignore */
COINLIBAPI double COINLINKAGE
Clp_optimizationDirection(Clp_Simplex * model)
{
     return model->model_->optimizationDirection();
}
COINLIBAPI void COINLINKAGE
Clp_setOptimizationDirection(Clp_Simplex * model, double value)
{
     model->model_->setOptimizationDirection(value);
}
/* Primal row solution */
COINLIBAPI double * COINLINKAGE
Clp_primalRowSolution(Clp_Simplex * model)
{
     return model->model_->primalRowSolution();
}
/* Primal column solution */
COINLIBAPI double * COINLINKAGE
Clp_primalColumnSolution(Clp_Simplex * model)
{
     return model->model_->primalColumnSolution();
}
/* Dual row solution */
COINLIBAPI double * COINLINKAGE
Clp_dualRowSolution(Clp_Simplex * model)
{
     return model->model_->dualRowSolution();
}
/* Reduced costs */
COINLIBAPI double * COINLINKAGE
Clp_dualColumnSolution(Clp_Simplex * model)
{
     return model->model_->dualColumnSolution();
}
/* Row lower */
COINLIBAPI double* COINLINKAGE
Clp_rowLower(Clp_Simplex * model)
{
     return model->model_->rowLower();
}
/* Row upper  */
COINLIBAPI double* COINLINKAGE
Clp_rowUpper(Clp_Simplex * model)
{
     return model->model_->rowUpper();
}
/* Objective */
COINLIBAPI double * COINLINKAGE
Clp_objective(Clp_Simplex * model)
{
     return model->model_->objective();
}
/* Column Lower */
COINLIBAPI double * COINLINKAGE
Clp_columnLower(Clp_Simplex * model)
{
     return model->model_->columnLower();
}
/* Column Upper */
COINLIBAPI double * COINLINKAGE
Clp_columnUpper(Clp_Simplex * model)
{
     return model->model_->columnUpper();
}
/* Number of elements in matrix */
COINLIBAPI int COINLINKAGE
Clp_getNumElements(Clp_Simplex * model)
{
     return model->model_->getNumElements();
}
// Column starts in matrix
COINLIBAPI const CoinBigIndex * COINLINKAGE Clp_getVectorStarts(Clp_Simplex * model)
{
     CoinPackedMatrix * matrix;
     matrix = model->model_->matrix();
     return (matrix == NULL) ? NULL : matrix->getVectorStarts();
}

// Row indices in matrix
COINLIBAPI const int * COINLINKAGE Clp_getIndices(Clp_Simplex * model)
{
     CoinPackedMatrix * matrix = model->model_->matrix();
     return (matrix == NULL) ? NULL : matrix->getIndices();
}

// Column vector lengths in matrix
COINLIBAPI const int * COINLINKAGE Clp_getVectorLengths(Clp_Simplex * model)
{
     CoinPackedMatrix * matrix = model->model_->matrix();
     return (matrix == NULL) ? NULL : matrix->getVectorLengths();
}

// Element values in matrix
COINLIBAPI const double * COINLINKAGE Clp_getElements(Clp_Simplex * model)
{
     CoinPackedMatrix * matrix = model->model_->matrix();
     return (matrix == NULL) ? NULL : matrix->getElements();
}
/* Objective value */
COINLIBAPI double COINLINKAGE
Clp_objectiveValue(Clp_Simplex * model)
{
     return model->model_->objectiveValue();
}
/* Integer information */
COINLIBAPI char * COINLINKAGE
Clp_integerInformation(Clp_Simplex * model)
{
     return model->model_->integerInformation();
}
/* Infeasibility/unbounded ray (NULL returned if none/wrong)
   Up to user to use free() on these arrays.  */
COINLIBAPI double * COINLINKAGE
Clp_infeasibilityRay(Clp_Simplex * model)
{
     const double * ray = model->model_->internalRay();
     double * array = NULL;
     int numberRows = model->model_->numberRows(); 
     int status = model->model_->status();
     if (status == 1 && ray) {
          array = static_cast<double*>(malloc(numberRows*sizeof(double)));
          memcpy(array,ray,numberRows*sizeof(double));
#ifdef PRINT_RAY_METHOD
	  printf("Infeasibility ray obtained by algorithm %s\n",model->model_->algorithm()>0 ?
	      "primal" : "dual");
#endif
     }
     return array;
}
COINLIBAPI double * COINLINKAGE
Clp_unboundedRay(Clp_Simplex * model)
{
     const double * ray = model->model_->internalRay();
     double * array = NULL;
     int numberColumns = model->model_->numberColumns(); 
     int status = model->model_->status();
     if (status == 2 && ray) {
          array = static_cast<double*>(malloc(numberColumns*sizeof(double)));
          memcpy(array,ray,numberColumns*sizeof(double));
     }
     return array;
}
COINLIBAPI void COINLINKAGE
Clp_freeRay(Clp_Simplex * model, double * ray)
{
     free(ray);
}
/* See if status array exists (partly for OsiClp) */
COINLIBAPI int COINLINKAGE
Clp_statusExists(Clp_Simplex * model)
{
     return model->model_->statusExists() ? 1 : 0;
}
/* Return address of status array (char[numberRows+numberColumns]) */
COINLIBAPI unsigned char *  COINLINKAGE
Clp_statusArray(Clp_Simplex * model)
{
     return model->model_->statusArray();
}
/* Copy in status vector */
COINLIBAPI void COINLINKAGE
Clp_copyinStatus(Clp_Simplex * model, const unsigned char * statusArray)
{
     model->model_->copyinStatus(statusArray);
}

/* User pointer for whatever reason */
COINLIBAPI void COINLINKAGE
Clp_setUserPointer (Clp_Simplex * model, void * pointer)
{
     model->model_->setUserPointer(pointer);
}
COINLIBAPI void * COINLINKAGE
Clp_getUserPointer (Clp_Simplex * model)
{
     return model->model_->getUserPointer();
}
/* Pass in Callback function */
COINLIBAPI void COINLINKAGE
Clp_registerCallBack(Clp_Simplex * model,
                     clp_callback userCallBack)
{
     // Will be copy of users one
     delete model->handler_;
     model->handler_ = new CMessageHandler(*(model->model_->messageHandler()));
     model->handler_->setCallBack(userCallBack);
     model->handler_->setModel(model);
     model->model_->passInMessageHandler(model->handler_);
}
/* Unset Callback function */
COINLIBAPI void COINLINKAGE
Clp_clearCallBack(Clp_Simplex * model)
{
     delete model->handler_;
     model->handler_ = NULL;
}
/* Amount of print out:
   0 - none
   1 - just final
   2 - just factorizations
   3 - as 2 plus a bit more
   4 - verbose
   above that 8,16,32 etc just for selective debug
*/
COINLIBAPI void COINLINKAGE
Clp_setLogLevel(Clp_Simplex * model, int value)
{
     model->model_->setLogLevel(value);
}
COINLIBAPI int COINLINKAGE
Clp_logLevel(Clp_Simplex * model)
{
     return model->model_->logLevel();
}
/* length of names (0 means no names0 */
COINLIBAPI int COINLINKAGE
Clp_lengthNames(Clp_Simplex * model)
{
     return model->model_->lengthNames();
}
/* Fill in array (at least lengthNames+1 long) with a row name */
COINLIBAPI void COINLINKAGE
Clp_rowName(Clp_Simplex * model, int iRow, char * name)
{
     std::string rowName = model->model_->rowName(iRow);
     strcpy(name, rowName.c_str());
}
/* Fill in array (at least lengthNames+1 long) with a column name */
COINLIBAPI void COINLINKAGE
Clp_columnName(Clp_Simplex * model, int iColumn, char * name)
{
     std::string columnName = model->model_->columnName(iColumn);
     strcpy(name, columnName.c_str());
}

/* General solve algorithm which can do presolve.
   See  ClpSolve.hpp for options
*/
COINLIBAPI int COINLINKAGE
Clp_initialSolve(Clp_Simplex * model)
{
     return model->model_->initialSolve();
}
/* Pass solve options. (Exception to direct analogue rule) */
COINLIBAPI int COINLINKAGE
Clp_initialSolveWithOptions(Clp_Simplex * model, Clp_Solve * s)
{
     return model->model_->initialSolve(s->options);
}
/* Barrier initial solve */
COINLIBAPI int COINLINKAGE
Clp_initialBarrierSolve(Clp_Simplex * model0)
{
     ClpSimplex *model = model0->model_;

     return model->initialBarrierSolve();

}
/* Barrier initial solve */
COINLIBAPI int COINLINKAGE
Clp_initialBarrierNoCrossSolve(Clp_Simplex * model0)
{
     ClpSimplex *model = model0->model_;

     return model->initialBarrierNoCrossSolve();

}
/* Dual initial solve */
COINLIBAPI int COINLINKAGE
Clp_initialDualSolve(Clp_Simplex * model)
{
     return model->model_->initialDualSolve();
}
/* Primal initial solve */
COINLIBAPI int COINLINKAGE
Clp_initialPrimalSolve(Clp_Simplex * model)
{
     return model->model_->initialPrimalSolve();
}
/* Dual algorithm - see ClpSimplexDual.hpp for method */
COINLIBAPI int COINLINKAGE
Clp_dual(Clp_Simplex * model, int ifValuesPass)
{
     return model->model_->dual(ifValuesPass);
}
/* Primal algorithm - see ClpSimplexPrimal.hpp for method */
COINLIBAPI int COINLINKAGE
Clp_primal(Clp_Simplex * model, int ifValuesPass)
{
     return model->model_->primal(ifValuesPass);
}
/* Sets or unsets scaling, 0 -off, 1 equilibrium, 2 geometric, 3, auto, 4 dynamic(later) */
COINLIBAPI void COINLINKAGE
Clp_scaling(Clp_Simplex * model, int mode)
{
     model->model_->scaling(mode);
}
/* Gets scalingFlag */
COINLIBAPI int COINLINKAGE
Clp_scalingFlag(Clp_Simplex * model)
{
     return model->model_->scalingFlag();
}
/* Crash - at present just aimed at dual, returns
   -2 if dual preferred and crash basis created
   -1 if dual preferred and all slack basis preferred
   0 if basis going in was not all slack
   1 if primal preferred and all slack basis preferred
   2 if primal preferred and crash basis created.

   if gap between bounds <="gap" variables can be flipped

   If "pivot" is
   0 No pivoting (so will just be choice of algorithm)
   1 Simple pivoting e.g. gub
   2 Mini iterations
*/
COINLIBAPI int COINLINKAGE
Clp_crash(Clp_Simplex * model, double gap, int pivot)
{
     return model->model_->crash(gap, pivot);
}
/* If problem is primal feasible */
COINLIBAPI int COINLINKAGE
Clp_primalFeasible(Clp_Simplex * model)
{
     return model->model_->primalFeasible() ? 1 : 0;
}
/* If problem is dual feasible */
COINLIBAPI int COINLINKAGE
Clp_dualFeasible(Clp_Simplex * model)
{
     return model->model_->dualFeasible() ? 1 : 0;
}
/* Dual bound */
COINLIBAPI double COINLINKAGE
Clp_dualBound(Clp_Simplex * model)
{
     return model->model_->dualBound();
}
COINLIBAPI void COINLINKAGE
Clp_setDualBound(Clp_Simplex * model, double value)
{
     model->model_->setDualBound(value);
}
/* Infeasibility cost */
COINLIBAPI double COINLINKAGE
Clp_infeasibilityCost(Clp_Simplex * model)
{
     return model->model_->infeasibilityCost();
}
COINLIBAPI void COINLINKAGE
Clp_setInfeasibilityCost(Clp_Simplex * model, double value)
{
     model->model_->setInfeasibilityCost(value);
}
/* Perturbation:
   50  - switch on perturbation
   100 - auto perturb if takes too long (1.0e-6 largest nonzero)
   101 - we are perturbed
   102 - don't try perturbing again
   default is 100
   others are for playing
*/
COINLIBAPI int COINLINKAGE
Clp_perturbation(Clp_Simplex * model)
{
     return model->model_->perturbation();
}
COINLIBAPI void COINLINKAGE
Clp_setPerturbation(Clp_Simplex * model, int value)
{
     model->model_->setPerturbation(value);
}
/* Current (or last) algorithm */
COINLIBAPI int COINLINKAGE
Clp_algorithm(Clp_Simplex * model)
{
     return model->model_->algorithm();
}
/* Set algorithm */
COINLIBAPI void COINLINKAGE
Clp_setAlgorithm(Clp_Simplex * model, int value)
{
     model->model_->setAlgorithm(value);
}
/* Sum of dual infeasibilities */
COINLIBAPI double COINLINKAGE
Clp_sumDualInfeasibilities(Clp_Simplex * model)
{
     return model->model_->sumDualInfeasibilities();
}
/* Number of dual infeasibilities */
COINLIBAPI int COINLINKAGE
Clp_numberDualInfeasibilities(Clp_Simplex * model)
{
     return model->model_->numberDualInfeasibilities();
}
/* Sum of primal infeasibilities */
COINLIBAPI double COINLINKAGE
Clp_sumPrimalInfeasibilities(Clp_Simplex * model)
{
     return model->model_->sumPrimalInfeasibilities();
}
/* Number of primal infeasibilities */
COINLIBAPI int COINLINKAGE
Clp_numberPrimalInfeasibilities(Clp_Simplex * model)
{
     return model->model_->numberPrimalInfeasibilities();
}
/* Save model to file, returns 0 if success.  This is designed for
   use outside algorithms so does not save iterating arrays etc.
   It does not save any messaging information.
   Does not save scaling values.
   It does not know about all types of virtual functions.
*/
COINLIBAPI int COINLINKAGE
Clp_saveModel(Clp_Simplex * model, const char * fileName)
{
     return model->model_->saveModel(fileName);
}
/* Restore model from file, returns 0 if success,
   deletes current model */
COINLIBAPI int COINLINKAGE
Clp_restoreModel(Clp_Simplex * model, const char * fileName)
{
     return model->model_->restoreModel(fileName);
}

/* Just check solution (for external use) - sets sum of
   infeasibilities etc */
COINLIBAPI void COINLINKAGE
Clp_checkSolution(Clp_Simplex * model)
{
     model->model_->checkSolution();
}
/* Number of rows */
COINLIBAPI int COINLINKAGE
Clp_getNumRows(Clp_Simplex * model)
{
     return model->model_->getNumRows();
}
/* Number of columns */
COINLIBAPI int COINLINKAGE
Clp_getNumCols(Clp_Simplex * model)
{
     return model->model_->getNumCols();
}
/* Number of iterations */
COINLIBAPI int COINLINKAGE
Clp_getIterationCount(Clp_Simplex * model)
{
     return model->model_->getIterationCount();
}
/* Are there a numerical difficulties? */
COINLIBAPI int COINLINKAGE
Clp_isAbandoned(Clp_Simplex * model)
{
     return model->model_->isAbandoned() ? 1 : 0;
}
/* Is optimality proven? */
COINLIBAPI int COINLINKAGE
Clp_isProvenOptimal(Clp_Simplex * model)
{
     return model->model_->isProvenOptimal() ? 1 : 0;
}
/* Is primal infeasiblity proven? */
COINLIBAPI int COINLINKAGE
Clp_isProvenPrimalInfeasible(Clp_Simplex * model)
{
     return model->model_->isProvenPrimalInfeasible() ? 1 : 0;
}
/* Is dual infeasiblity proven? */
COINLIBAPI int COINLINKAGE
Clp_isProvenDualInfeasible(Clp_Simplex * model)
{
     return model->model_->isProvenDualInfeasible() ? 1 : 0;
}
/* Is the given primal objective limit reached? */
COINLIBAPI int COINLINKAGE
Clp_isPrimalObjectiveLimitReached(Clp_Simplex * model)
{
     return model->model_->isPrimalObjectiveLimitReached() ? 1 : 0;
}
/* Is the given dual objective limit reached? */
COINLIBAPI int COINLINKAGE
Clp_isDualObjectiveLimitReached(Clp_Simplex * model)
{
     return model->model_->isDualObjectiveLimitReached() ? 1 : 0;
}
/* Iteration limit reached? */
COINLIBAPI int COINLINKAGE
Clp_isIterationLimitReached(Clp_Simplex * model)
{
     return model->model_->isIterationLimitReached() ? 1 : 0;
}
/* Direction of optimization (1 - minimize, -1 - maximize, 0 - ignore */
COINLIBAPI double COINLINKAGE
Clp_getObjSense(Clp_Simplex * model)
{
     return model->model_->getObjSense();
}
/* Direction of optimization (1 - minimize, -1 - maximize, 0 - ignore */
COINLIBAPI void COINLINKAGE
Clp_setObjSense(Clp_Simplex * model, double objsen)
{
     model->model_->setOptimizationDirection(objsen);
}
/* Primal row solution */
COINLIBAPI const double * COINLINKAGE
Clp_getRowActivity(Clp_Simplex * model)
{
     return model->model_->getRowActivity();
}
/* Primal column solution */
COINLIBAPI const double * COINLINKAGE
Clp_getColSolution(Clp_Simplex * model)
{
     return model->model_->getColSolution();
}
COINLIBAPI void COINLINKAGE
Clp_setColSolution(Clp_Simplex * model, const double * input)
{
     model->model_->setColSolution(input);
}
/* Dual row solution */
COINLIBAPI const double * COINLINKAGE
Clp_getRowPrice(Clp_Simplex * model)
{
     return model->model_->getRowPrice();
}
/* Reduced costs */
COINLIBAPI const double * COINLINKAGE
Clp_getReducedCost(Clp_Simplex * model)
{
     return model->model_->getReducedCost();
}
/* Row lower */
COINLIBAPI const double* COINLINKAGE
Clp_getRowLower(Clp_Simplex * model)
{
     return model->model_->getRowLower();
}
/* Row upper  */
COINLIBAPI const double* COINLINKAGE
Clp_getRowUpper(Clp_Simplex * model)
{
     return model->model_->getRowUpper();
}
/* Objective */
COINLIBAPI const double * COINLINKAGE
Clp_getObjCoefficients(Clp_Simplex * model)
{
     return model->model_->getObjCoefficients();
}
/* Column Lower */
COINLIBAPI const double * COINLINKAGE
Clp_getColLower(Clp_Simplex * model)
{
     return model->model_->getColLower();
}
/* Column Upper */
COINLIBAPI const double * COINLINKAGE
Clp_getColUpper(Clp_Simplex * model)
{
     return model->model_->getColUpper();
}
/* Objective value */
COINLIBAPI double COINLINKAGE
Clp_getObjValue(Clp_Simplex * model)
{
     return model->model_->getObjValue();
}
/* Get variable basis info */
COINLIBAPI int COINLINKAGE
Clp_getColumnStatus(Clp_Simplex * model, int sequence)
{
     return (int) model->model_->getColumnStatus(sequence);
}
/* Get row basis info */
COINLIBAPI int COINLINKAGE
Clp_getRowStatus(Clp_Simplex * model, int sequence)
{
     return (int) model->model_->getRowStatus(sequence);
}
/* Set variable basis info */
COINLIBAPI void COINLINKAGE
Clp_setColumnStatus(Clp_Simplex * model, int sequence, int value)
{
     if (value >= 0 && value <= 5) {
          model->model_->setColumnStatus(sequence, (ClpSimplex::Status) value );
          if (value == 3 || value == 5)
               model->model_->primalColumnSolution()[sequence] =
                    model->model_->columnLower()[sequence];
          else if (value == 2)
               model->model_->primalColumnSolution()[sequence] =
                    model->model_->columnUpper()[sequence];
     }
}
/* Set row basis info */
COINLIBAPI void COINLINKAGE
Clp_setRowStatus(Clp_Simplex * model, int sequence, int value)
{
     if (value >= 0 && value <= 5) {
          model->model_->setRowStatus(sequence, (ClpSimplex::Status) value );
          if (value == 3 || value == 5)
               model->model_->primalRowSolution()[sequence] =
                    model->model_->rowLower()[sequence];
          else if (value == 2)
               model->model_->primalRowSolution()[sequence] =
                    model->model_->rowUpper()[sequence];
     }
}
/* Small element value - elements less than this set to zero,
   default is 1.0e-20 */
COINLIBAPI double COINLINKAGE
Clp_getSmallElementValue(Clp_Simplex * model)
{
     return model->model_->getSmallElementValue();
}
COINLIBAPI void COINLINKAGE
Clp_setSmallElementValue(Clp_Simplex * model, double value)
{
     model->model_->setSmallElementValue(value);
}
/* Print model */
COINLIBAPI void COINLINKAGE
Clp_printModel(Clp_Simplex * model, const char * prefix)
{
     ClpSimplex *clp_simplex = model->model_;
     int numrows    = clp_simplex->numberRows();
     int numcols    = clp_simplex->numberColumns();
     int numelem    = clp_simplex->getNumElements();
     const CoinBigIndex *start = clp_simplex->matrix()->getVectorStarts();
     const int *index     = clp_simplex->matrix()->getIndices();
     const double *value  = clp_simplex->matrix()->getElements();
     const double *collb  = model->model_->columnLower();
     const double *colub  = model->model_->columnUpper();
     const double *obj    = model->model_->objective();
     const double *rowlb  = model->model_->rowLower();
     const double *rowub  = model->model_->rowUpper();
     printf("%s numcols = %i, numrows = %i, numelem = %i\n",
            prefix, numcols, numrows, numelem);
     printf("%s model = %p, start = %p, index = %p, value = %p\n",
            prefix, reinterpret_cast<const void *>(model), reinterpret_cast<const void *>(start), reinterpret_cast<const void *>(index), reinterpret_cast<const void *>(value));
     clp_simplex->matrix()->dumpMatrix(NULL);
     {
          int i;
          for (i = 0; i <= numcols; i++)
               printf("%s start[%i] = %i\n", prefix, i, start[i]);
          for (i = 0; i < numelem; i++)
               printf("%s index[%i] = %i, value[%i] = %g\n",
                      prefix, i, index[i], i, value[i]);
     }

     printf("%s collb = %p, colub = %p, obj = %p, rowlb = %p, rowub = %p\n",
            prefix, reinterpret_cast<const void *>(collb), reinterpret_cast<const void *>(colub), reinterpret_cast<const void *>(obj), reinterpret_cast<const void *>(rowlb), reinterpret_cast<const void *>(rowub));
     printf("%s optimization direction = %g\n", prefix, Clp_optimizationDirection(model));
     printf("  (1 - minimize, -1 - maximize, 0 - ignore)\n");
     {
          int i;
          for (i = 0; i < numcols; i++)
               printf("%s collb[%i] = %g, colub[%i] = %g, obj[%i] = %g\n",
                      prefix, i, collb[i], i, colub[i], i, obj[i]);
          for (i = 0; i < numrows; i++)
               printf("%s rowlb[%i] = %g, rowub[%i] = %g\n",
                      prefix, i, rowlb[i], i, rowub[i]);
     }
}

#ifndef SLIM_CLP
/** Solve the problem with the idiot code */
/* tryhard values:
   tryhard & 7:
      0: NOT lightweight, 105 iterations within a pass (when mu stays fixed)
      1: lightweight, but focus more on optimality (mu is high)
         (23 iters in a pass)
      2: lightweight, but focus more on feasibility (11 iters in a pass)
      3: lightweight, but focus more on feasibility (23 iters in a pass, so it
         goes closer to opt than option 2)
   tryhard >> 3:
      number of passes, the larger the number the closer it gets to optimality
*/
COINLIBAPI void COINLINKAGE
Clp_idiot(Clp_Simplex * model, int tryhard)
{
     ClpSimplex *clp = model->model_;
     Idiot info(*clp);
     int numberpass = tryhard >> 3;
     int lightweight = tryhard & 7;
     info.setLightweight(lightweight);
     info.crash(numberpass, clp->messageHandler(), clp->messagesPointer(), false);
}
#endif

COINLIBAPI Clp_Solve * COINLINKAGE 
ClpSolve_new() 
{ 
    return new Clp_Solve(); 
}

COINLIBAPI void COINLINKAGE 
ClpSolve_delete(Clp_Solve * solve) 
{ 
    delete solve; 
}

// space- and error-saving macros
#define ClpSolveGetIntProperty(prop) \
COINLIBAPI int COINLINKAGE \
ClpSolve_ ## prop (Clp_Solve *s) \
{ \
    return s->options.prop(); \
}

#define ClpSolveSetIntProperty(prop) \
COINLIBAPI void COINLINKAGE \
ClpSolve_ ## prop (Clp_Solve *s, int val) \
{ \
    s->options.prop(val); \
}

COINLIBAPI void COINLINKAGE 
ClpSolve_setSpecialOption(Clp_Solve * s, int which, int value, int extraInfo) 
{
    s->options.setSpecialOption(which,value,extraInfo);
}

COINLIBAPI int COINLINKAGE 
ClpSolve_getSpecialOption(Clp_Solve * s, int which)
{
    return s->options.getSpecialOption(which);
}

COINLIBAPI void COINLINKAGE 
ClpSolve_setSolveType(Clp_Solve * s, int method, int extraInfo)
{
    s->options.setSolveType(static_cast<ClpSolve::SolveType>(method), extraInfo);
}

ClpSolveGetIntProperty(getSolveType)

COINLIBAPI void COINLINKAGE ClpSolve_setPresolveType(Clp_Solve * s, int amount, int extraInfo)
{
    s->options.setPresolveType(static_cast<ClpSolve::PresolveType>(amount),extraInfo);
}

ClpSolveGetIntProperty(getPresolveType)

ClpSolveGetIntProperty(getPresolvePasses)


COINLIBAPI int COINLINKAGE 
ClpSolve_getExtraInfo(Clp_Solve * s, int which) {
     return s->options.getExtraInfo(which);
}

ClpSolveSetIntProperty(setInfeasibleReturn)
ClpSolveGetIntProperty(infeasibleReturn)

ClpSolveGetIntProperty(doDual)
ClpSolveSetIntProperty(setDoDual)

ClpSolveGetIntProperty(doSingleton)
ClpSolveSetIntProperty(setDoSingleton)

ClpSolveGetIntProperty(doDoubleton)
ClpSolveSetIntProperty(setDoDoubleton)

ClpSolveGetIntProperty(doTripleton)
ClpSolveSetIntProperty(setDoTripleton)

ClpSolveGetIntProperty(doTighten)
ClpSolveSetIntProperty(setDoTighten)

ClpSolveGetIntProperty(doForcing)
ClpSolveSetIntProperty(setDoForcing)

ClpSolveGetIntProperty(doImpliedFree)
ClpSolveSetIntProperty(setDoImpliedFree)

ClpSolveGetIntProperty(doDupcol)
ClpSolveSetIntProperty(setDoDupcol)

ClpSolveGetIntProperty(doDuprow)
ClpSolveSetIntProperty(setDoDuprow)

ClpSolveGetIntProperty(doSingletonColumn)
ClpSolveSetIntProperty(setDoSingletonColumn)

ClpSolveGetIntProperty(presolveActions)
ClpSolveSetIntProperty(setPresolveActions)

ClpSolveGetIntProperty(substitution)
ClpSolveSetIntProperty(setSubstitution)

#if defined(__MWERKS__)
#pragma export off
#endif

