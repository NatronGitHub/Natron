/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpModel_H
#define ClpModel_H

#include "ClpConfig.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <string>
//#ifndef COIN_USE_CLP
//#define COIN_USE_CLP
//#endif
#include "ClpPackedMatrix.hpp"
#include "CoinMessageHandler.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinTypes.hpp"
#include "CoinFinite.hpp"
#include "ClpParameters.hpp"
#include "ClpObjective.hpp"
class ClpEventHandler;
/** This is the base class for Linear and quadratic Models
    This knows nothing about the algorithm, but it seems to
    have a reasonable amount of information

    I would welcome suggestions for what should be in this and
    how it relates to OsiSolverInterface.  Some methods look
    very similar.

*/
class CoinBuild;
class CoinModel;
class ClpModel {

public:

     /**@name Constructors and destructor
        Note - copy methods copy ALL data so can chew up memory
        until other copy is freed
      */
     //@{
     /// Default constructor
     ClpModel (bool emptyMessages = false  );

     /** Copy constructor. May scale depending on mode
         -1 leave mode as is
         0 -off, 1 equilibrium, 2 geometric, 3, auto, 4 auto-but-as-initialSolve-in-bab
     */
     ClpModel(const ClpModel & rhs, int scalingMode = -1);
     /// Assignment operator. This copies the data
     ClpModel & operator=(const ClpModel & rhs);
     /** Subproblem constructor.  A subset of whole model is created from the
         row and column lists given.  The new order is given by list order and
         duplicates are allowed.  Name and integer information can be dropped
     */
     ClpModel (const ClpModel * wholeModel,
               int numberRows, const int * whichRows,
               int numberColumns, const int * whichColumns,
               bool dropNames = true, bool dropIntegers = true);
     /// Destructor
     ~ClpModel (  );
     //@}

     /**@name Load model - loads some stuff and initializes others */
     //@{
     /** Loads a problem (the constraints on the
         rows are given by lower and upper bounds). If a pointer is 0 then the
         following values are the default:
         <ul>
           <li> <code>colub</code>: all columns have upper bound infinity
           <li> <code>collb</code>: all columns have lower bound 0
           <li> <code>rowub</code>: all rows have upper bound infinity
           <li> <code>rowlb</code>: all rows have lower bound -infinity
       <li> <code>obj</code>: all variables have 0 objective coefficient
         </ul>
     */
     void loadProblem (  const ClpMatrixBase& matrix,
                         const double* collb, const double* colub,
                         const double* obj,
                         const double* rowlb, const double* rowub,
                         const double * rowObjective = NULL);
     void loadProblem (  const CoinPackedMatrix& matrix,
                         const double* collb, const double* colub,
                         const double* obj,
                         const double* rowlb, const double* rowub,
                         const double * rowObjective = NULL);

     /** Just like the other loadProblem() method except that the matrix is
       given in a standard column major ordered format (without gaps). */
     void loadProblem (  const int numcols, const int numrows,
                         const CoinBigIndex* start, const int* index,
                         const double* value,
                         const double* collb, const double* colub,
                         const double* obj,
                         const double* rowlb, const double* rowub,
                         const double * rowObjective = NULL);
     /** This loads a model from a coinModel object - returns number of errors.

         modelObject not const as may be changed as part of process
         If tryPlusMinusOne then will try adding as +-1 matrix
     */
     int loadProblem (  CoinModel & modelObject, bool tryPlusMinusOne = false);
     /// This one is for after presolve to save memory
     void loadProblem (  const int numcols, const int numrows,
                         const CoinBigIndex* start, const int* index,
                         const double* value, const int * length,
                         const double* collb, const double* colub,
                         const double* obj,
                         const double* rowlb, const double* rowub,
                         const double * rowObjective = NULL);
     /** Load up quadratic objective.  This is stored as a CoinPackedMatrix */
     void loadQuadraticObjective(const int numberColumns,
                                 const CoinBigIndex * start,
                                 const int * column, const double * element);
     void loadQuadraticObjective (  const CoinPackedMatrix& matrix);
     /// Get rid of quadratic objective
     void deleteQuadraticObjective();
     /// This just loads up a row objective
     void setRowObjective(const double * rowObjective);
     /// Read an mps file from the given filename
     int readMps(const char *filename,
                 bool keepNames = false,
                 bool ignoreErrors = false);
     /// Read GMPL files from the given filenames
     int readGMPL(const char *filename, const char * dataName,
                  bool keepNames = false);
     /// Copy in integer informations
     void copyInIntegerInformation(const char * information);
     /// Drop integer informations
     void deleteIntegerInformation();
     /** Set the index-th variable to be a continuous variable */
     void setContinuous(int index);
     /** Set the index-th variable to be an integer variable */
     void setInteger(int index);
     /** Return true if the index-th variable is an integer variable */
     bool isInteger(int index) const;
     /// Resizes rim part of model
     void resize (int newNumberRows, int newNumberColumns);
     /// Deletes rows
     void deleteRows(int number, const int * which);
     /// Add one row
     void addRow(int numberInRow, const int * columns,
                 const double * elements, double rowLower = -COIN_DBL_MAX,
                 double rowUpper = COIN_DBL_MAX);
     /// Add rows
     void addRows(int number, const double * rowLower,
                  const double * rowUpper,
                  const CoinBigIndex * rowStarts, const int * columns,
                  const double * elements);
     /// Add rows
     void addRows(int number, const double * rowLower,
                  const double * rowUpper,
                  const CoinBigIndex * rowStarts, const int * rowLengths,
                  const int * columns,
                  const double * elements);
#ifndef CLP_NO_VECTOR
     void addRows(int number, const double * rowLower,
                  const double * rowUpper,
                  const CoinPackedVectorBase * const * rows);
#endif
     /** Add rows from a build object.
         If tryPlusMinusOne then will try adding as +-1 matrix
         if no matrix exists.
         Returns number of errors e.g. duplicates
     */
     int addRows(const CoinBuild & buildObject, bool tryPlusMinusOne = false,
                 bool checkDuplicates = true);
     /** Add rows from a model object.  returns
         -1 if object in bad state (i.e. has column information)
         otherwise number of errors.

         modelObject non const as can be regularized as part of build
         If tryPlusMinusOne then will try adding as +-1 matrix
         if no matrix exists.
     */
     int addRows(CoinModel & modelObject, bool tryPlusMinusOne = false,
                 bool checkDuplicates = true);

     /// Deletes columns
     void deleteColumns(int number, const int * which);
     /// Deletes rows AND columns (keeps old sizes)
     void deleteRowsAndColumns(int numberRows, const int * whichRows,
			       int numberColumns, const int * whichColumns);
     /// Add one column
     void addColumn(int numberInColumn,
                    const int * rows,
                    const double * elements,
                    double columnLower = 0.0,
                    double  columnUpper = COIN_DBL_MAX,
                    double  objective = 0.0);
     /// Add columns
     void addColumns(int number, const double * columnLower,
                     const double * columnUpper,
                     const double * objective,
                     const CoinBigIndex * columnStarts, const int * rows,
                     const double * elements);
     void addColumns(int number, const double * columnLower,
                     const double * columnUpper,
                     const double * objective,
                     const CoinBigIndex * columnStarts, const int * columnLengths,
                     const int * rows,
                     const double * elements);
#ifndef CLP_NO_VECTOR
     void addColumns(int number, const double * columnLower,
                     const double * columnUpper,
                     const double * objective,
                     const CoinPackedVectorBase * const * columns);
#endif
     /** Add columns from a build object
         If tryPlusMinusOne then will try adding as +-1 matrix
         if no matrix exists.
         Returns number of errors e.g. duplicates
     */
     int addColumns(const CoinBuild & buildObject, bool tryPlusMinusOne = false,
                    bool checkDuplicates = true);
     /** Add columns from a model object.  returns
         -1 if object in bad state (i.e. has row information)
         otherwise number of errors
         modelObject non const as can be regularized as part of build
         If tryPlusMinusOne then will try adding as +-1 matrix
         if no matrix exists.
     */
     int addColumns(CoinModel & modelObject, bool tryPlusMinusOne = false,
                    bool checkDuplicates = true);
     /// Modify one element of a matrix
     inline void modifyCoefficient(int row, int column, double newElement,
                                   bool keepZero = false) {
          matrix_->modifyCoefficient(row, column, newElement, keepZero);
     }
     /** Change row lower bounds */
     void chgRowLower(const double * rowLower);
     /** Change row upper bounds */
     void chgRowUpper(const double * rowUpper);
     /** Change column lower bounds */
     void chgColumnLower(const double * columnLower);
     /** Change column upper bounds */
     void chgColumnUpper(const double * columnUpper);
     /** Change objective coefficients */
     void chgObjCoefficients(const double * objIn);
     /** Borrow model.  This is so we don't have to copy large amounts
         of data around.  It assumes a derived class wants to overwrite
         an empty model with a real one - while it does an algorithm */
     void borrowModel(ClpModel & otherModel);
     /** Return model - nulls all arrays so can be deleted safely
         also updates any scalars */
     void returnModel(ClpModel & otherModel);

     /// Create empty ClpPackedMatrix
     void createEmptyMatrix();
     /** Really clean up matrix (if ClpPackedMatrix).
         a) eliminate all duplicate AND small elements in matrix
         b) remove all gaps and set extraGap_ and extraMajor_ to 0.0
         c) reallocate arrays and make max lengths equal to lengths
         d) orders elements
         returns number of elements eliminated or -1 if not ClpPackedMatrix
     */
     int cleanMatrix(double threshold = 1.0e-20);
     /// Copy contents - resizing if necessary - otherwise re-use memory
     void copy(const ClpMatrixBase * from, ClpMatrixBase * & to);
#ifndef CLP_NO_STD
     /// Drops names - makes lengthnames 0 and names empty
     void dropNames();
     /// Copies in names
     void copyNames(const std::vector<std::string> & rowNames,
                    const std::vector<std::string> & columnNames);
     /// Copies in Row names - modifies names first .. last-1
     void copyRowNames(const std::vector<std::string> & rowNames, int first, int last);
     /// Copies in Column names - modifies names first .. last-1
     void copyColumnNames(const std::vector<std::string> & columnNames, int first, int last);
     /// Copies in Row names - modifies names first .. last-1
     void copyRowNames(const char * const * rowNames, int first, int last);
     /// Copies in Column names - modifies names first .. last-1
     void copyColumnNames(const char * const * columnNames, int first, int last);
     /// Set name of row
     void setRowName(int rowIndex, std::string & name) ;
     /// Set name of col
     void setColumnName(int colIndex, std::string & name) ;
#endif
     /** Find a network subset.
         rotate array should be numberRows.  On output
         -1 not in network
          0 in network as is
          1 in network with signs swapped
         Returns number of network rows
     */
     int findNetwork(char * rotate, double fractionNeeded = 0.75);
     /** This creates a coinModel object
     */
     CoinModel * createCoinModel() const;

     /** Write the problem in MPS format to the specified file.

     Row and column names may be null.
     formatType is
     <ul>
       <li> 0 - normal
       <li> 1 - extra accuracy
       <li> 2 - IEEE hex
     </ul>

     Returns non-zero on I/O error
     */
     int writeMps(const char *filename,
                  int formatType = 0, int numberAcross = 2,
                  double objSense = 0.0) const ;
     //@}
     /**@name gets and sets */
     //@{
     /// Number of rows
     inline int numberRows() const {
          return numberRows_;
     }
     inline int getNumRows() const {
          return numberRows_;
     }
     /// Number of columns
     inline int getNumCols() const {
          return numberColumns_;
     }
     inline int numberColumns() const {
          return numberColumns_;
     }
     /// Primal tolerance to use
     inline double primalTolerance() const {
          return dblParam_[ClpPrimalTolerance];
     }
     void setPrimalTolerance( double value) ;
     /// Dual tolerance to use
     inline double dualTolerance() const  {
          return dblParam_[ClpDualTolerance];
     }
     void setDualTolerance( double value) ;
     /// Primal objective limit
     inline double primalObjectiveLimit() const {
          return dblParam_[ClpPrimalObjectiveLimit];
     }
     void setPrimalObjectiveLimit(double value);
     /// Dual objective limit
     inline double dualObjectiveLimit() const {
          return dblParam_[ClpDualObjectiveLimit];
     }
     void setDualObjectiveLimit(double value);
     /// Objective offset
     inline double objectiveOffset() const {
          return dblParam_[ClpObjOffset];
     }
     void setObjectiveOffset(double value);
     /// Presolve tolerance to use
     inline double presolveTolerance() const {
          return dblParam_[ClpPresolveTolerance];
     }
#ifndef CLP_NO_STD
     inline const std::string & problemName() const {
          return strParam_[ClpProbName];
     }
#endif
     /// Number of iterations
     inline int numberIterations() const  {
          return numberIterations_;
     }
     inline int getIterationCount() const {
          return numberIterations_;
     }
     inline void setNumberIterations(int numberIterationsNew) {
          numberIterations_ = numberIterationsNew;
     }
     /** Solve type - 1 simplex, 2 simplex interface, 3 Interior.*/
     inline int solveType() const {
          return solveType_;
     }
     inline void setSolveType(int type) {
          solveType_ = type;
     }
     /// Maximum number of iterations
     inline int maximumIterations() const {
          return intParam_[ClpMaxNumIteration];
     }
     void setMaximumIterations(int value);
     /// Maximum time in seconds (from when set called)
     inline double maximumSeconds() const {
          return dblParam_[ClpMaxSeconds];
     }
     void setMaximumSeconds(double value);
     void setMaximumWallSeconds(double value);
     /// Returns true if hit maximum iterations (or time)
     bool hitMaximumIterations() const;
     /** Status of problem:
         -1 - unknown e.g. before solve or if postSolve says not optimal
         0 - optimal
         1 - primal infeasible
         2 - dual infeasible
         3 - stopped on iterations or time
         4 - stopped due to errors
         5 - stopped by event handler (virtual int ClpEventHandler::event())
     */
     inline int status() const            {
          return problemStatus_;
     }
     inline int problemStatus() const            {
          return problemStatus_;
     }
     /// Set problem status
     inline void setProblemStatus(int problemStatusNew) {
          problemStatus_ = problemStatusNew;
     }
     /** Secondary status of problem - may get extended
         0 - none
         1 - primal infeasible because dual limit reached OR (probably primal
         infeasible but can't prove it  - main status was 4)
         2 - scaled problem optimal - unscaled problem has primal infeasibilities
         3 - scaled problem optimal - unscaled problem has dual infeasibilities
         4 - scaled problem optimal - unscaled problem has primal and dual infeasibilities
         5 - giving up in primal with flagged variables
         6 - failed due to empty problem check
         7 - postSolve says not optimal
         8 - failed due to bad element check
         9 - status was 3 and stopped on time
	 10 - status was 3 and can't use objective as lb
         100 up - translation of enum from ClpEventHandler
     */
     inline int secondaryStatus() const            {
          return secondaryStatus_;
     }
     inline void setSecondaryStatus(int newstatus) {
          secondaryStatus_ = newstatus;
     }
     /// Are there a numerical difficulties?
     inline bool isAbandoned() const             {
          return problemStatus_ == 4;
     }
     /// Is optimality proven?
     inline bool isProvenOptimal() const         {
          return problemStatus_ == 0;
     }
     /// Is primal infeasiblity proven?
     inline bool isProvenPrimalInfeasible() const {
          return problemStatus_ == 1;
     }
     /// Is dual infeasiblity proven?
     inline bool isProvenDualInfeasible() const  {
          return problemStatus_ == 2;
     }
     /// Is the given primal objective limit reached?
     bool isPrimalObjectiveLimitReached() const ;
     /// Is the given dual objective limit reached?
     bool isDualObjectiveLimitReached() const ;
     /// Iteration limit reached?
     inline bool isIterationLimitReached() const {
          return problemStatus_ == 3;
     }
     /// Direction of optimization (1 - minimize, -1 - maximize, 0 - ignore
     inline double optimizationDirection() const {
          return  optimizationDirection_;
     }
     inline double getObjSense() const    {
          return optimizationDirection_;
     }
     void setOptimizationDirection(double value);
     /// Primal row solution
     inline double * primalRowSolution() const    {
          return rowActivity_;
     }
     inline const double * getRowActivity() const {
          return rowActivity_;
     }
     /// Primal column solution
     inline double * primalColumnSolution() const {
          return columnActivity_;
     }
     inline const double * getColSolution() const {
          return columnActivity_;
     }
     inline void setColSolution(const double * input) {
          memcpy(columnActivity_, input, numberColumns_ * sizeof(double));
     }
     /// Dual row solution
     inline double * dualRowSolution() const      {
          return dual_;
     }
     inline const double * getRowPrice() const    {
          return dual_;
     }
     /// Reduced costs
     inline double * dualColumnSolution() const   {
          return reducedCost_;
     }
     inline const double * getReducedCost() const {
          return reducedCost_;
     }
     /// Row lower
     inline double* rowLower() const              {
          return rowLower_;
     }
     inline const double* getRowLower() const     {
          return rowLower_;
     }
     /// Row upper
     inline double* rowUpper() const              {
          return rowUpper_;
     }
     inline const double* getRowUpper() const     {
          return rowUpper_;
     }
     //-------------------------------------------------------------------------
     /**@name Changing bounds on variables and constraints */
     //@{
     /** Set an objective function coefficient */
     void setObjectiveCoefficient( int elementIndex, double elementValue );
     /** Set an objective function coefficient */
     inline void setObjCoeff( int elementIndex, double elementValue ) {
          setObjectiveCoefficient( elementIndex, elementValue);
     }

     /** Set a single column lower bound<br>
         Use -DBL_MAX for -infinity. */
     void setColumnLower( int elementIndex, double elementValue );

     /** Set a single column upper bound<br>
         Use DBL_MAX for infinity. */
     void setColumnUpper( int elementIndex, double elementValue );

     /** Set a single column lower and upper bound */
     void setColumnBounds( int elementIndex,
                           double lower, double upper );

     /** Set the bounds on a number of columns simultaneously<br>
         The default implementation just invokes setColLower() and
         setColUpper() over and over again.
         @param indexFirst,indexLast pointers to the beginning and after the
            end of the array of the indices of the variables whose
        <em>either</em> bound changes
         @param boundList the new lower/upper bound pairs for the variables
     */
     void setColumnSetBounds(const int* indexFirst,
                             const int* indexLast,
                             const double* boundList);

     /** Set a single column lower bound<br>
         Use -DBL_MAX for -infinity. */
     inline void setColLower( int elementIndex, double elementValue ) {
          setColumnLower(elementIndex, elementValue);
     }
     /** Set a single column upper bound<br>
         Use DBL_MAX for infinity. */
     inline void setColUpper( int elementIndex, double elementValue ) {
          setColumnUpper(elementIndex, elementValue);
     }

     /** Set a single column lower and upper bound */
     inline void setColBounds( int elementIndex,
                               double lower, double upper ) {
          setColumnBounds(elementIndex, lower, upper);
     }

     /** Set the bounds on a number of columns simultaneously<br>
         @param indexFirst,indexLast pointers to the beginning and after the
            end of the array of the indices of the variables whose
        <em>either</em> bound changes
         @param boundList the new lower/upper bound pairs for the variables
     */
     inline void setColSetBounds(const int* indexFirst,
                                 const int* indexLast,
                                 const double* boundList) {
          setColumnSetBounds(indexFirst, indexLast, boundList);
     }

     /** Set a single row lower bound<br>
         Use -DBL_MAX for -infinity. */
     void setRowLower( int elementIndex, double elementValue );

     /** Set a single row upper bound<br>
         Use DBL_MAX for infinity. */
     void setRowUpper( int elementIndex, double elementValue ) ;

     /** Set a single row lower and upper bound */
     void setRowBounds( int elementIndex,
                        double lower, double upper ) ;

     /** Set the bounds on a number of rows simultaneously<br>
         @param indexFirst,indexLast pointers to the beginning and after the
            end of the array of the indices of the constraints whose
        <em>either</em> bound changes
         @param boundList the new lower/upper bound pairs for the constraints
     */
     void setRowSetBounds(const int* indexFirst,
                          const int* indexLast,
                          const double* boundList);

     //@}
     /// Scaling
     inline const double * rowScale() const {
          return rowScale_;
     }
     inline const double * columnScale() const {
          return columnScale_;
     }
     inline const double * inverseRowScale() const {
          return inverseRowScale_;
     }
     inline const double * inverseColumnScale() const {
          return inverseColumnScale_;
     }
     inline double * mutableRowScale() const {
          return rowScale_;
     }
     inline double * mutableColumnScale() const {
          return columnScale_;
     }
     inline double * mutableInverseRowScale() const {
          return inverseRowScale_;
     }
     inline double * mutableInverseColumnScale() const {
          return inverseColumnScale_;
     }
     inline double * swapRowScale(double * newScale) {
          double * oldScale = rowScale_;
	  rowScale_ = newScale;
          return oldScale;
     }
     void setRowScale(double * scale) ;
     void setColumnScale(double * scale);
     /// Scaling of objective
     inline double objectiveScale() const {
          return objectiveScale_;
     }
     inline void setObjectiveScale(double value) {
          objectiveScale_ = value;
     }
     /// Scaling of rhs and bounds
     inline double rhsScale() const {
          return rhsScale_;
     }
     inline void setRhsScale(double value) {
          rhsScale_ = value;
     }
     /// Sets or unsets scaling, 0 -off, 1 equilibrium, 2 geometric, 3 auto, 4 auto-but-as-initialSolve-in-bab
     void scaling(int mode = 1);
     /** If we constructed a "really" scaled model then this reverses the operation.
         Quantities may not be exactly as they were before due to rounding errors */
     void unscale();
     /// Gets scalingFlag
     inline int scalingFlag() const {
          return scalingFlag_;
     }
     /// Objective
     inline double * objective() const {
          if (objective_) {
               double offset;
               return objective_->gradient(NULL, NULL, offset, false);
          } else {
               return NULL;
          }
     }
     inline double * objective(const double * solution, double & offset, bool refresh = true) const {
          offset = 0.0;
          if (objective_) {
               return objective_->gradient(NULL, solution, offset, refresh);
          } else {
               return NULL;
          }
     }
     inline const double * getObjCoefficients() const {
          if (objective_) {
               double offset;
               return objective_->gradient(NULL, NULL, offset, false);
          } else {
               return NULL;
          }
     }
     /// Row Objective
     inline double * rowObjective() const         {
          return rowObjective_;
     }
     inline const double * getRowObjCoefficients() const {
          return rowObjective_;
     }
     /// Column Lower
     inline double * columnLower() const          {
          return columnLower_;
     }
     inline const double * getColLower() const    {
          return columnLower_;
     }
     /// Column Upper
     inline double * columnUpper() const          {
          return columnUpper_;
     }
     inline const double * getColUpper() const    {
          return columnUpper_;
     }
     /// Matrix (if not ClpPackedmatrix be careful about memory leak
     inline CoinPackedMatrix * matrix() const {
          if ( matrix_ == NULL ) return NULL;
          else return matrix_->getPackedMatrix();
     }
     /// Number of elements in matrix
     inline int getNumElements() const {
          return matrix_->getNumElements();
     }
     /** Small element value - elements less than this set to zero,
        default is 1.0e-20 */
     inline double getSmallElementValue() const {
          return smallElement_;
     }
     inline void setSmallElementValue(double value) {
          smallElement_ = value;
     }
     /// Row Matrix
     inline ClpMatrixBase * rowCopy() const       {
          return rowCopy_;
     }
     /// Set new row matrix
     void setNewRowCopy(ClpMatrixBase * newCopy);
     /// Clp Matrix
     inline ClpMatrixBase * clpMatrix() const     {
          return matrix_;
     }
     /// Scaled ClpPackedMatrix
     inline ClpPackedMatrix * clpScaledMatrix() const     {
          return scaledMatrix_;
     }
     /// Sets pointer to scaled ClpPackedMatrix
     inline void setClpScaledMatrix(ClpPackedMatrix * scaledMatrix) {
          delete scaledMatrix_;
          scaledMatrix_ = scaledMatrix;
     }
     /// Swaps pointer to scaled ClpPackedMatrix
     inline ClpPackedMatrix * swapScaledMatrix(ClpPackedMatrix * scaledMatrix) {
          ClpPackedMatrix * oldMatrix = scaledMatrix_;
          scaledMatrix_ = scaledMatrix;
	  return oldMatrix;
     }
     /** Replace Clp Matrix (current is not deleted unless told to
         and new is used)
         So up to user to delete current.  This was used where
         matrices were being rotated. ClpModel takes ownership.
     */
     void replaceMatrix(ClpMatrixBase * matrix, bool deleteCurrent = false);
     /** Replace Clp Matrix (current is not deleted unless told to
         and new is used) So up to user to delete current.  This was used where
         matrices were being rotated.  This version changes CoinPackedMatrix
         to ClpPackedMatrix.  ClpModel takes ownership.
     */
     inline void replaceMatrix(CoinPackedMatrix * newmatrix,
                               bool deleteCurrent = false) {
          replaceMatrix(new ClpPackedMatrix(newmatrix), deleteCurrent);
     }
     /// Objective value
     inline double objectiveValue() const {
          return objectiveValue_ * optimizationDirection_ - dblParam_[ClpObjOffset];
     }
     inline void setObjectiveValue(double value) {
          objectiveValue_ = (value + dblParam_[ClpObjOffset]) / optimizationDirection_;
     }
     inline double getObjValue() const {
          return objectiveValue_ * optimizationDirection_ - dblParam_[ClpObjOffset];
     }
     /// Integer information
     inline char * integerInformation() const     {
          return integerType_;
     }
     /** Infeasibility/unbounded ray (NULL returned if none/wrong)
         Up to user to use delete [] on these arrays.  */
     double * infeasibilityRay(bool fullRay=false) const;
     double * unboundedRay() const;
     /// For advanced users - no need to delete - sign not changed
     inline double * ray() const
     { return ray_;}
     /// just test if infeasibility or unbounded Ray exists
     inline bool rayExists() const {
         return (ray_!=NULL);
     }
     /// just delete ray if exists
     inline void deleteRay() {
         delete [] ray_;
         ray_=NULL;
     }
	 /// Access internal ray storage. Users should call infeasibilityRay() or unboundedRay() instead.
	 inline const double * internalRay() const {
		 return ray_;
	 }
     /// See if status (i.e. basis) array exists (partly for OsiClp)
     inline bool statusExists() const {
          return (status_ != NULL);
     }
     /// Return address of status (i.e. basis) array (char[numberRows+numberColumns])
     inline unsigned char *  statusArray() const {
          return status_;
     }
     /** Return copy of status (i.e. basis) array (char[numberRows+numberColumns]),
         use delete [] */
     unsigned char *  statusCopy() const;
     /// Copy in status (basis) vector
     void copyinStatus(const unsigned char * statusArray);

     /// User pointer for whatever reason
     inline void setUserPointer (void * pointer) {
          userPointer_ = pointer;
     }
     inline void * getUserPointer () const {
          return userPointer_;
     }
     /// Trusted user pointer
     inline void setTrustedUserPointer (ClpTrustedData * pointer) {
          trustedUserPointer_ = pointer;
     }
     inline ClpTrustedData * getTrustedUserPointer () const {
          return trustedUserPointer_;
     }
     /// What has changed in model (only for masochistic users)
     inline int whatsChanged() const {
          return whatsChanged_;
     }
     inline void setWhatsChanged(int value) {
          whatsChanged_ = value;
     }
     /// Number of threads (not really being used)
     inline int numberThreads() const {
          return numberThreads_;
     }
     inline void setNumberThreads(int value) {
          numberThreads_ = value;
     }
     //@}
     /**@name Message handling */
     //@{
     /// Pass in Message handler (not deleted at end)
     void passInMessageHandler(CoinMessageHandler * handler);
     /// Pass in Message handler (not deleted at end) and return current
     CoinMessageHandler * pushMessageHandler(CoinMessageHandler * handler,
                                             bool & oldDefault);
     /// back to previous message handler
     void popMessageHandler(CoinMessageHandler * oldHandler, bool oldDefault);
     /// Set language
     void newLanguage(CoinMessages::Language language);
     inline void setLanguage(CoinMessages::Language language) {
          newLanguage(language);
     }
     /// Overrides message handler with a default one
     void setDefaultMessageHandler();
     /// Return handler
     inline CoinMessageHandler * messageHandler() const       {
          return handler_;
     }
     /// Return messages
     inline CoinMessages messages() const                     {
          return messages_;
     }
     /// Return pointer to messages
     inline CoinMessages * messagesPointer()                  {
          return & messages_;
     }
     /// Return Coin messages
     inline CoinMessages coinMessages() const                  {
          return coinMessages_;
     }
     /// Return pointer to Coin messages
     inline CoinMessages * coinMessagesPointer()                  {
          return & coinMessages_;
     }
     /** Amount of print out:
         0 - none
         1 - just final
         2 - just factorizations
         3 - as 2 plus a bit more
         4 - verbose
         above that 8,16,32 etc just for selective debug
     */
     inline void setLogLevel(int value)    {
          handler_->setLogLevel(value);
     }
     inline int logLevel() const           {
          return handler_->logLevel();
     }
     /// Return true if default handler
     inline bool defaultHandler() const {
          return defaultHandler_;
     }
     /// Pass in Event handler (cloned and deleted at end)
     void passInEventHandler(const ClpEventHandler * eventHandler);
     /// Event handler
     inline ClpEventHandler * eventHandler() const {
          return eventHandler_;
     }
     /// Thread specific random number generator
     inline CoinThreadRandom * randomNumberGenerator() {
          return &randomNumberGenerator_;
     }
     /// Thread specific random number generator
     inline CoinThreadRandom & mutableRandomNumberGenerator() {
          return randomNumberGenerator_;
     }
     /// Set seed for thread specific random number generator
     inline void setRandomSeed(int value) {
          randomNumberGenerator_.setSeed(value);
     }
     /// length of names (0 means no names0
     inline int lengthNames() const {
          return lengthNames_;
     }
#ifndef CLP_NO_STD
     /// length of names (0 means no names0
     inline void setLengthNames(int value) {
          lengthNames_ = value;
     }
     /// Row names
     inline const std::vector<std::string> * rowNames() const {
          return &rowNames_;
     }
     inline const std::string& rowName(int iRow) const {
          return rowNames_[iRow];
     }
     /// Return name or Rnnnnnnn
     std::string getRowName(int iRow) const;
     /// Column names
     inline const std::vector<std::string> * columnNames() const {
          return &columnNames_;
     }
     inline const std::string& columnName(int iColumn) const {
          return columnNames_[iColumn];
     }
     /// Return name or Cnnnnnnn
     std::string getColumnName(int iColumn) const;
#endif
     /// Objective methods
     inline ClpObjective * objectiveAsObject() const {
          return objective_;
     }
     void setObjective(ClpObjective * objective);
     inline void setObjectivePointer(ClpObjective * newobjective) {
          objective_ = newobjective;
     }
     /** Solve a problem with no elements - return status and
         dual and primal infeasibilites */
     int emptyProblem(int * infeasNumber = NULL, double * infeasSum = NULL, bool printMessage = true);

     //@}

     /**@name Matrix times vector methods
        They can be faster if scalar is +- 1
        These are covers so user need not worry about scaling
        Also for simplex I am not using basic/non-basic split */
     //@{
     /** Return <code>y + A * x * scalar</code> in <code>y</code>.
         @pre <code>x</code> must be of size <code>numColumns()</code>
         @pre <code>y</code> must be of size <code>numRows()</code> */
     void times(double scalar,
                const double * x, double * y) const;
     /** Return <code>y + x * scalar * A</code> in <code>y</code>.
         @pre <code>x</code> must be of size <code>numRows()</code>
         @pre <code>y</code> must be of size <code>numColumns()</code> */
     void transposeTimes(double scalar,
                         const double * x, double * y) const ;
     //@}


     //---------------------------------------------------------------------------
     /**@name Parameter set/get methods

        The set methods return true if the parameter was set to the given value,
        false otherwise. There can be various reasons for failure: the given
        parameter is not applicable for the solver (e.g., refactorization
        frequency for the volume algorithm), the parameter is not yet implemented
        for the solver or simply the value of the parameter is out of the range
        the solver accepts. If a parameter setting call returns false check the
        details of your solver.

        The get methods return true if the given parameter is applicable for the
        solver and is implemented. In this case the value of the parameter is
        returned in the second argument. Otherwise they return false.

        ** once it has been decided where solver sits this may be redone
     */
     //@{
     /// Set an integer parameter
     bool setIntParam(ClpIntParam key, int value) ;
     /// Set an double parameter
     bool setDblParam(ClpDblParam key, double value) ;
#ifndef CLP_NO_STD
     /// Set an string parameter
     bool setStrParam(ClpStrParam key, const std::string & value);
#endif
     // Get an integer parameter
     inline bool getIntParam(ClpIntParam key, int& value) const {
          if (key < ClpLastIntParam) {
               value = intParam_[key];
               return true;
          } else {
               return false;
          }
     }
     // Get an double parameter
     inline bool getDblParam(ClpDblParam key, double& value) const {
          if (key < ClpLastDblParam) {
               value = dblParam_[key];
               return true;
          } else {
               return false;
          }
     }
#ifndef CLP_NO_STD
     // Get a string parameter
     inline bool getStrParam(ClpStrParam key, std::string& value) const {
          if (key < ClpLastStrParam) {
               value = strParam_[key];
               return true;
          } else {
               return false;
          }
     }
#endif
     /// Create C++ lines to get to current state
     void generateCpp( FILE * fp);
     /** For advanced options
         1 - Don't keep changing infeasibility weight
         2 - Keep nonLinearCost round solves
         4 - Force outgoing variables to exact bound (primal)
         8 - Safe to use dense initial factorization
         16 -Just use basic variables for operation if column generation
         32 -Create ray even in BAB
         64 -Treat problem as feasible until last minute (i.e. minimize infeasibilities)
         128 - Switch off all matrix sanity checks
         256 - No row copy
         512 - If not in values pass, solution guaranteed, skip as much as possible
         1024 - In branch and bound
         2048 - Don't bother to re-factorize if < 20 iterations
         4096 - Skip some optimality checks
         8192 - Do Primal when cleaning up primal
         16384 - In fast dual (so we can switch off things)
         32768 - called from Osi
         65536 - keep arrays around as much as possible (also use maximumR/C)
         131072 - transposeTimes is -1.0 and can skip basic and fixed
         262144 - extra copy of scaled matrix
         524288 - Clp fast dual
         1048576 - don't need to finish dual (can return 3)
	 2097152 - zero costs!
         NOTE - many applications can call Clp but there may be some short cuts
                which are taken which are not guaranteed safe from all applications.
                Vetted applications will have a bit set and the code may test this
                At present I expect a few such applications - if too many I will
                have to re-think.  It is up to application owner to change the code
                if she/he needs these short cuts.  I will not debug unless in Coin
                repository.  See COIN_CLP_VETTED comments.
         0x01000000 is Cbc (and in branch and bound)
         0x02000000 is in a different branch and bound
     */
     inline unsigned int specialOptions() const {
          return specialOptions_;
     }
     void setSpecialOptions(unsigned int value);
#define COIN_CBC_USING_CLP 0x01000000
     inline bool inCbcBranchAndBound() const {
          return (specialOptions_ & COIN_CBC_USING_CLP) != 0;
     }
     //@}

     /**@name private or protected methods */
     //@{
protected:
     /// Does most of deletion (0 = all, 1 = most)
     void gutsOfDelete(int type);
     /** Does most of copying
         If trueCopy 0 then just points to arrays
         If -1 leaves as much as possible */
     void gutsOfCopy(const ClpModel & rhs, int trueCopy = 1);
     /// gets lower and upper bounds on rows
     void getRowBound(int iRow, double& lower, double& upper) const;
     /// puts in format I like - 4 array matrix - may make row copy
     void gutsOfLoadModel ( int numberRows, int numberColumns,
                            const double* collb, const double* colub,
                            const double* obj,
                            const double* rowlb, const double* rowub,
                            const double * rowObjective = NULL);
     /// Does much of scaling
     void gutsOfScaling();
     /// Objective value - always minimize
     inline double rawObjectiveValue() const {
          return objectiveValue_;
     }
     /// If we are using maximumRows_ and Columns_
     inline bool permanentArrays() const {
          return (specialOptions_ & 65536) != 0;
     }
     /// Start using maximumRows_ and Columns_
     void startPermanentArrays();
     /// Stop using maximumRows_ and Columns_
     void stopPermanentArrays();
     /// Create row names as char **
     const char * const * rowNamesAsChar() const;
     /// Create column names as char **
     const char * const * columnNamesAsChar() const;
     /// Delete char * version of names
     void deleteNamesAsChar(const char * const * names, int number) const;
     /// On stopped - sets secondary status
     void onStopped();
     //@}


////////////////// data //////////////////
protected:

     /**@name data */
     //@{
     /// Direction of optimization (1 - minimize, -1 - maximize, 0 - ignore
     double optimizationDirection_;
     /// Array of double parameters
     double dblParam_[ClpLastDblParam];
     /// Objective value
     double objectiveValue_;
     /// Small element value
     double smallElement_;
     /// Scaling of objective
     double objectiveScale_;
     /// Scaling of rhs and bounds
     double rhsScale_;
     /// Number of rows
     int numberRows_;
     /// Number of columns
     int numberColumns_;
     /// Row activities
     double * rowActivity_;
     /// Column activities
     double * columnActivity_;
     /// Duals
     double * dual_;
     /// Reduced costs
     double * reducedCost_;
     /// Row lower
     double* rowLower_;
     /// Row upper
     double* rowUpper_;
     /// Objective
     ClpObjective * objective_;
     /// Row Objective (? sign)  - may be NULL
     double * rowObjective_;
     /// Column Lower
     double * columnLower_;
     /// Column Upper
     double * columnUpper_;
     /// Packed matrix
     ClpMatrixBase * matrix_;
     /// Row copy if wanted
     ClpMatrixBase * rowCopy_;
     /// Scaled packed matrix
     ClpPackedMatrix * scaledMatrix_;
     /// Infeasible/unbounded ray
     double * ray_;
     /// Row scale factors for matrix
     double * rowScale_;
     /// Column scale factors
     double * columnScale_;
     /// Inverse row scale factors for matrix (end of rowScale_)
     double * inverseRowScale_;
     /// Inverse column scale factors for matrix (end of columnScale_)
     double * inverseColumnScale_;
     /** Scale flag, 0 none, 1 equilibrium, 2 geometric, 3, auto, 4 dynamic,
         5 geometric on rows */
     int scalingFlag_;
     /** Status (i.e. basis) Region.  I know that not all algorithms need a status
         array, but it made sense for things like crossover and put
         all permanent stuff in one place.  No assumption is made
         about what is in status array (although it might be good to reserve
         bottom 3 bits (i.e. 0-7 numeric) for classic status).  This
         is number of columns + number of rows long (in that order).
     */
     unsigned char * status_;
     /// Integer information
     char * integerType_;
     /// User pointer for whatever reason
     void * userPointer_;
     /// Trusted user pointer e.g. for heuristics
     ClpTrustedData * trustedUserPointer_;
     /// Array of integer parameters
     int intParam_[ClpLastIntParam];
     /// Number of iterations
     int numberIterations_;
     /** Solve type - 1 simplex, 2 simplex interface, 3 Interior.*/
     int solveType_;
     /** Whats changed since last solve.  This is a work in progress
         It is designed so careful people can make go faster.
         It is only used when startFinishOptions used in dual or primal.
         Bit 1 - number of rows/columns has not changed (so work arrays valid)
             2 - matrix has not changed
             4 - if matrix has changed only by adding rows
             8 - if matrix has changed only by adding columns
            16 - row lbs not changed
            32 - row ubs not changed
            64 - column objective not changed
           128 - column lbs not changed
           256 - column ubs not changed
       512 - basis not changed (up to user to set this to 0)
             top bits may be used internally
       shift by 65336 is 3 all same, 1 all except col bounds
     */
#define ROW_COLUMN_COUNTS_SAME 1
#define MATRIX_SAME 2
#define MATRIX_JUST_ROWS_ADDED 4
#define MATRIX_JUST_COLUMNS_ADDED 8
#define ROW_LOWER_SAME 16
#define ROW_UPPER_SAME 32
#define OBJECTIVE_SAME 64 
#define COLUMN_LOWER_SAME 128
#define COLUMN_UPPER_SAME 256
#define BASIS_SAME 512
#define ALL_SAME 65339
#define ALL_SAME_EXCEPT_COLUMN_BOUNDS 65337
     unsigned int whatsChanged_;
     /// Status of problem
     int problemStatus_;
     /// Secondary status of problem
     int secondaryStatus_;
     /// length of names (0 means no names)
     int lengthNames_;
     /// Number of threads (not very operational)
     int numberThreads_;
     /** For advanced options
         See get and set for meaning
     */
     unsigned int specialOptions_;
     /// Message handler
     CoinMessageHandler * handler_;
     /// Flag to say if default handler (so delete)
     bool defaultHandler_;
     /// Thread specific random number generator
     CoinThreadRandom randomNumberGenerator_;
     /// Event handler
     ClpEventHandler * eventHandler_;
#ifndef CLP_NO_STD
     /// Row names
     std::vector<std::string> rowNames_;
     /// Column names
     std::vector<std::string> columnNames_;
#endif
     /// Messages
     CoinMessages messages_;
     /// Coin messages
     CoinMessages coinMessages_;
     /// Maximum number of columns in model
     int maximumColumns_;
     /// Maximum number of rows in model
     int maximumRows_;
     /// Maximum number of columns (internal arrays) in model
     int maximumInternalColumns_;
     /// Maximum number of rows (internal arrays) in model
     int maximumInternalRows_;
     /// Base packed matrix
     CoinPackedMatrix baseMatrix_;
     /// Base row copy
     CoinPackedMatrix baseRowCopy_;
     /// Saved row scale factors for matrix
     double * savedRowScale_;
     /// Saved column scale factors
     double * savedColumnScale_;
#ifndef CLP_NO_STD
     /// Array of string parameters
     std::string strParam_[ClpLastStrParam];
#endif
     //@}
};
/** This is a tiny class where data can be saved round calls.
 */
class ClpDataSave {

public:
     /**@name Constructors and destructor
      */
     //@{
     /// Default constructor
     ClpDataSave (  );

     /// Copy constructor.
     ClpDataSave(const ClpDataSave &);
     /// Assignment operator. This copies the data
     ClpDataSave & operator=(const ClpDataSave & rhs);
     /// Destructor
     ~ClpDataSave (  );

     //@}

////////////////// data //////////////////
public:

     /**@name data - with same names as in other classes*/
     //@{
     double dualBound_;
     double infeasibilityCost_;
     double pivotTolerance_;
     double zeroFactorizationTolerance_;
     double zeroSimplexTolerance_;
     double acceptablePivot_;
     double objectiveScale_;
     int sparseThreshold_;
     int perturbation_;
     int forceFactorization_;
     int scalingFlag_;
     unsigned int specialOptions_;
     //@}
};

#endif
