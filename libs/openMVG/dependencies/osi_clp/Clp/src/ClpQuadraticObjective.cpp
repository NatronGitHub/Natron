/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinIndexedVector.hpp"
#include "ClpFactorization.hpp"
#include "ClpSimplex.hpp"
#include "ClpQuadraticObjective.hpp"
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################
//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpQuadraticObjective::ClpQuadraticObjective ()
     : ClpObjective()
{
     type_ = 2;
     objective_ = NULL;
     quadraticObjective_ = NULL;
     gradient_ = NULL;
     numberColumns_ = 0;
     numberExtendedColumns_ = 0;
     activated_ = 0;
     fullMatrix_ = false;
}

//-------------------------------------------------------------------
// Useful Constructor
//-------------------------------------------------------------------
ClpQuadraticObjective::ClpQuadraticObjective (const double * objective ,
          int numberColumns,
          const CoinBigIndex * start,
          const int * column, const double * element,
          int numberExtendedColumns)
     : ClpObjective()
{
     type_ = 2;
     numberColumns_ = numberColumns;
     if (numberExtendedColumns >= 0)
          numberExtendedColumns_ = CoinMax(numberColumns_, numberExtendedColumns);
     else
          numberExtendedColumns_ = numberColumns_;
     if (objective) {
          objective_ = new double [numberExtendedColumns_];
          CoinMemcpyN(objective, numberColumns_, objective_);
          memset(objective_ + numberColumns_, 0, (numberExtendedColumns_ - numberColumns_)*sizeof(double));
     } else {
          objective_ = new double [numberExtendedColumns_];
          memset(objective_, 0, numberExtendedColumns_ * sizeof(double));
     }
     if (start)
          quadraticObjective_ = new CoinPackedMatrix(true, numberColumns, numberColumns,
                    start[numberColumns], element, column, start, NULL);
     else
          quadraticObjective_ = NULL;
     gradient_ = NULL;
     activated_ = 1;
     fullMatrix_ = false;
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpQuadraticObjective::ClpQuadraticObjective (const ClpQuadraticObjective & rhs,
          int type)
     : ClpObjective(rhs)
{
     numberColumns_ = rhs.numberColumns_;
     numberExtendedColumns_ = rhs.numberExtendedColumns_;
     fullMatrix_ = rhs.fullMatrix_;
     if (rhs.objective_) {
          objective_ = new double [numberExtendedColumns_];
          CoinMemcpyN(rhs.objective_, numberExtendedColumns_, objective_);
     } else {
          objective_ = NULL;
     }
     if (rhs.gradient_) {
          gradient_ = new double [numberExtendedColumns_];
          CoinMemcpyN(rhs.gradient_, numberExtendedColumns_, gradient_);
     } else {
          gradient_ = NULL;
     }
     if (rhs.quadraticObjective_) {
          // see what type of matrix wanted
          if (type == 0) {
               // just copy
               quadraticObjective_ = new CoinPackedMatrix(*rhs.quadraticObjective_);
          } else if (type == 1) {
               // expand to full symmetric
               fullMatrix_ = true;
               const int * columnQuadratic1 = rhs.quadraticObjective_->getIndices();
               const CoinBigIndex * columnQuadraticStart1 = rhs.quadraticObjective_->getVectorStarts();
               const int * columnQuadraticLength1 = rhs.quadraticObjective_->getVectorLengths();
               const double * quadraticElement1 = rhs.quadraticObjective_->getElements();
               CoinBigIndex * columnQuadraticStart2 = new CoinBigIndex [numberExtendedColumns_+1];
               int * columnQuadraticLength2 = new int [numberExtendedColumns_];
               int iColumn;
               int numberColumns = rhs.quadraticObjective_->getNumCols();
               int numberBelow = 0;
               int numberAbove = 0;
               int numberDiagonal = 0;
               CoinZeroN(columnQuadraticLength2, numberExtendedColumns_);
               for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                    for (CoinBigIndex j = columnQuadraticStart1[iColumn];
                              j < columnQuadraticStart1[iColumn] + columnQuadraticLength1[iColumn]; j++) {
                         int jColumn = columnQuadratic1[j];
                         if (jColumn > iColumn) {
                              numberBelow++;
                              columnQuadraticLength2[jColumn]++;
                              columnQuadraticLength2[iColumn]++;
                         } else if (jColumn == iColumn) {
                              numberDiagonal++;
                              columnQuadraticLength2[iColumn]++;
                         } else {
                              numberAbove++;
                         }
                    }
               }
               if (numberAbove > 0) {
                    if (numberAbove == numberBelow) {
                         // already done
                         quadraticObjective_ = new CoinPackedMatrix(*rhs.quadraticObjective_);
                         delete [] columnQuadraticStart2;
                         delete [] columnQuadraticLength2;
                    } else {
                         printf("number above = %d, number below = %d, error\n",
                                numberAbove, numberBelow);
                         abort();
                    }
               } else {
                    int numberElements = numberDiagonal + 2 * numberBelow;
                    int * columnQuadratic2 = new int [numberElements];
                    double * quadraticElement2 = new double [numberElements];
                    columnQuadraticStart2[0] = 0;
                    numberElements = 0;
                    for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                         int n = columnQuadraticLength2[iColumn];
                         columnQuadraticLength2[iColumn] = 0;
                         numberElements += n;
                         columnQuadraticStart2[iColumn+1] = numberElements;
                    }
                    for (iColumn = 0; iColumn < numberColumns; iColumn++) {
                         for (CoinBigIndex j = columnQuadraticStart1[iColumn];
                                   j < columnQuadraticStart1[iColumn] + columnQuadraticLength1[iColumn]; j++) {
                              int jColumn = columnQuadratic1[j];
                              if (jColumn > iColumn) {
                                   // put in two places
                                   CoinBigIndex put = columnQuadraticLength2[jColumn] + columnQuadraticStart2[jColumn];
                                   columnQuadraticLength2[jColumn]++;
                                   quadraticElement2[put] = quadraticElement1[j];
                                   columnQuadratic2[put] = iColumn;
                                   put = columnQuadraticLength2[iColumn] + columnQuadraticStart2[iColumn];
                                   columnQuadraticLength2[iColumn]++;
                                   quadraticElement2[put] = quadraticElement1[j];
                                   columnQuadratic2[put] = jColumn;
                              } else if (jColumn == iColumn) {
                                   CoinBigIndex put = columnQuadraticLength2[iColumn] + columnQuadraticStart2[iColumn];
                                   columnQuadraticLength2[iColumn]++;
                                   quadraticElement2[put] = quadraticElement1[j];
                                   columnQuadratic2[put] = iColumn;
                              } else {
                                   abort();
                              }
                         }
                    }
                    // Now create
                    quadraticObjective_ =
                         new CoinPackedMatrix (true,
                                               rhs.numberExtendedColumns_,
                                               rhs.numberExtendedColumns_,
                                               numberElements,
                                               quadraticElement2,
                                               columnQuadratic2,
                                               columnQuadraticStart2,
                                               columnQuadraticLength2, 0.0, 0.0);
                    delete [] columnQuadraticStart2;
                    delete [] columnQuadraticLength2;
                    delete [] columnQuadratic2;
                    delete [] quadraticElement2;
               }
          } else {
               fullMatrix_ = false;
               abort(); // code when needed
          }

     } else {
          quadraticObjective_ = NULL;
     }
}
/* Subset constructor.  Duplicates are allowed
   and order is as given.
*/
ClpQuadraticObjective::ClpQuadraticObjective (const ClpQuadraticObjective &rhs,
          int numberColumns,
          const int * whichColumn)
     : ClpObjective(rhs)
{
     fullMatrix_ = rhs.fullMatrix_;
     objective_ = NULL;
     int extra = rhs.numberExtendedColumns_ - rhs.numberColumns_;
     numberColumns_ = 0;
     numberExtendedColumns_ = numberColumns + extra;
     if (numberColumns > 0) {
          // check valid lists
          int numberBad = 0;
          int i;
          for (i = 0; i < numberColumns; i++)
               if (whichColumn[i] < 0 || whichColumn[i] >= rhs.numberColumns_)
                    numberBad++;
          if (numberBad)
               throw CoinError("bad column list", "subset constructor",
                               "ClpQuadraticObjective");
          numberColumns_ = numberColumns;
          objective_ = new double[numberExtendedColumns_];
          for (i = 0; i < numberColumns_; i++)
               objective_[i] = rhs.objective_[whichColumn[i]];
          CoinMemcpyN(rhs.objective_ + rhs.numberColumns_,	(numberExtendedColumns_ - numberColumns_),
                      objective_ + numberColumns_);
          if (rhs.gradient_) {
               gradient_ = new double[numberExtendedColumns_];
               for (i = 0; i < numberColumns_; i++)
                    gradient_[i] = rhs.gradient_[whichColumn[i]];
               CoinMemcpyN(rhs.gradient_ + rhs.numberColumns_,	(numberExtendedColumns_ - numberColumns_),
                           gradient_ + numberColumns_);
          } else {
               gradient_ = NULL;
          }
     } else {
          gradient_ = NULL;
          objective_ = NULL;
     }
     if (rhs.quadraticObjective_) {
          quadraticObjective_ = new CoinPackedMatrix(*rhs.quadraticObjective_,
                    numberColumns, whichColumn,
                    numberColumns, whichColumn);
     } else {
          quadraticObjective_ = NULL;
     }
}


//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpQuadraticObjective::~ClpQuadraticObjective ()
{
     delete [] objective_;
     delete [] gradient_;
     delete quadraticObjective_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpQuadraticObjective &
ClpQuadraticObjective::operator=(const ClpQuadraticObjective& rhs)
{
     if (this != &rhs) {
          fullMatrix_ = rhs.fullMatrix_;
          delete quadraticObjective_;
          quadraticObjective_ = NULL;
          delete [] objective_;
          delete [] gradient_;
          ClpObjective::operator=(rhs);
          numberColumns_ = rhs.numberColumns_;
          numberExtendedColumns_ = rhs.numberExtendedColumns_;
          if (rhs.objective_) {
               objective_ = new double [numberExtendedColumns_];
               CoinMemcpyN(rhs.objective_, numberExtendedColumns_, objective_);
          } else {
               objective_ = NULL;
          }
          if (rhs.gradient_) {
               gradient_ = new double [numberExtendedColumns_];
               CoinMemcpyN(rhs.gradient_, numberExtendedColumns_, gradient_);
          } else {
               gradient_ = NULL;
          }
          if (rhs.quadraticObjective_) {
               quadraticObjective_ = new CoinPackedMatrix(*rhs.quadraticObjective_);
          } else {
               quadraticObjective_ = NULL;
          }
     }
     return *this;
}

// Returns gradient
double *
ClpQuadraticObjective::gradient(const ClpSimplex * model,
                                const double * solution, double & offset, bool refresh,
                                int includeLinear)
{
     offset = 0.0;
     bool scaling = false;
     if (model && (model->rowScale() ||
                   model->objectiveScale() != 1.0 || model->optimizationDirection() != 1.0))
          scaling = true;
     const double * cost = NULL;
     if (model)
          cost = model->costRegion();
     if (!cost) {
          // not in solve
          cost = objective_;
          scaling = false;
     }
     if (!scaling) {
          if (!quadraticObjective_ || !solution || !activated_) {
               return objective_;
          } else {
               if (refresh || !gradient_) {
                    if (!gradient_)
                         gradient_ = new double[numberExtendedColumns_];
                    const int * columnQuadratic = quadraticObjective_->getIndices();
                    const CoinBigIndex * columnQuadraticStart = quadraticObjective_->getVectorStarts();
                    const int * columnQuadraticLength = quadraticObjective_->getVectorLengths();
                    const double * quadraticElement = quadraticObjective_->getElements();
                    offset = 0.0;
                    // use current linear cost region
                    if (includeLinear == 1)
                         CoinMemcpyN(cost, numberExtendedColumns_, gradient_);
                    else if (includeLinear == 2)
                         CoinMemcpyN(objective_, numberExtendedColumns_, gradient_);
                    else
                         memset(gradient_, 0, numberExtendedColumns_ * sizeof(double));
                    if (activated_) {
                         if (!fullMatrix_) {
                              int iColumn;
                              for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                                   double valueI = solution[iColumn];
                                   CoinBigIndex j;
                                   for (j = columnQuadraticStart[iColumn];
                                             j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                                        int jColumn = columnQuadratic[j];
                                        double valueJ = solution[jColumn];
                                        double elementValue = quadraticElement[j];
                                        if (iColumn != jColumn) {
                                             offset += valueI * valueJ * elementValue;
                                             //if (fabs(valueI*valueJ*elementValue)>1.0e-12)
                                             //printf("%d %d %g %g %g -> %g\n",
                                             //       iColumn,jColumn,valueI,valueJ,elementValue,
                                             //       valueI*valueJ*elementValue);
                                             double gradientI = valueJ * elementValue;
                                             double gradientJ = valueI * elementValue;
                                             gradient_[iColumn] += gradientI;
                                             gradient_[jColumn] += gradientJ;
                                        } else {
                                             offset += 0.5 * valueI * valueI * elementValue;
                                             //if (fabs(valueI*valueI*elementValue)>1.0e-12)
                                             //printf("XX %d %g %g -> %g\n",
                                             //       iColumn,valueI,elementValue,
                                             //       0.5*valueI*valueI*elementValue);
                                             double gradientI = valueI * elementValue;
                                             gradient_[iColumn] += gradientI;
                                        }
                                   }
                              }
                         } else {
                              // full matrix
                              int iColumn;
                              offset *= 2.0;
                              for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                                   CoinBigIndex j;
                                   double value = 0.0;
                                   double current = gradient_[iColumn];
                                   for (j = columnQuadraticStart[iColumn];
                                             j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                                        int jColumn = columnQuadratic[j];
                                        double valueJ = solution[jColumn] * quadraticElement[j];
                                        value += valueJ;
                                   }
                                   offset += value * solution[iColumn];
                                   gradient_[iColumn] = current + value;
                              }
                              offset *= 0.5;
                         }
                    }
               }
               if (model)
                    offset *= model->optimizationDirection() * model->objectiveScale();
               return gradient_;
          }
     } else {
          // do scaling
          assert (solution);
          // for now only if half
          assert (!fullMatrix_);
          if (refresh || !gradient_) {
               if (!gradient_)
                    gradient_ = new double[numberExtendedColumns_];
               double direction = model->optimizationDirection() * model->objectiveScale();
               // direction is actually scale out not scale in
               //if (direction)
               //direction = 1.0/direction;
               const int * columnQuadratic = quadraticObjective_->getIndices();
               const CoinBigIndex * columnQuadraticStart = quadraticObjective_->getVectorStarts();
               const int * columnQuadraticLength = quadraticObjective_->getVectorLengths();
               const double * quadraticElement = quadraticObjective_->getElements();
               int iColumn;
               const double * columnScale = model->columnScale();
               // use current linear cost region (already scaled)
               if (includeLinear == 1) {
                    CoinMemcpyN(model->costRegion(), numberExtendedColumns_, gradient_);
               }	else if (includeLinear == 2) {
                    memset(gradient_ + numberColumns_, 0, (numberExtendedColumns_ - numberColumns_)*sizeof(double));
                    if (!columnScale) {
                         for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                              gradient_[iColumn] = objective_[iColumn] * direction;
                         }
                    } else {
                         for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                              gradient_[iColumn] = objective_[iColumn] * direction * columnScale[iColumn];
                         }
                    }
               } else {
                    memset(gradient_, 0, numberExtendedColumns_ * sizeof(double));
               }
               if (!columnScale) {
                    if (activated_) {
                         for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                              double valueI = solution[iColumn];
                              CoinBigIndex j;
                              for (j = columnQuadraticStart[iColumn];
                                        j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                                   int jColumn = columnQuadratic[j];
                                   double valueJ = solution[jColumn];
                                   double elementValue = quadraticElement[j];
                                   elementValue *= direction;
                                   if (iColumn != jColumn) {
                                        offset += valueI * valueJ * elementValue;
                                        double gradientI = valueJ * elementValue;
                                        double gradientJ = valueI * elementValue;
                                        gradient_[iColumn] += gradientI;
                                        gradient_[jColumn] += gradientJ;
                                   } else {
                                        offset += 0.5 * valueI * valueI * elementValue;
                                        double gradientI = valueI * elementValue;
                                        gradient_[iColumn] += gradientI;
                                   }
                              }
                         }
                    }
               } else {
                    // scaling
                    if (activated_) {
                         for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                              double valueI = solution[iColumn];
                              double scaleI = columnScale[iColumn] * direction;
                              CoinBigIndex j;
                              for (j = columnQuadraticStart[iColumn];
                                        j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                                   int jColumn = columnQuadratic[j];
                                   double valueJ = solution[jColumn];
                                   double elementValue = quadraticElement[j];
                                   double scaleJ = columnScale[jColumn];
                                   elementValue *= scaleI * scaleJ;
                                   if (iColumn != jColumn) {
                                        offset += valueI * valueJ * elementValue;
                                        double gradientI = valueJ * elementValue;
                                        double gradientJ = valueI * elementValue;
                                        gradient_[iColumn] += gradientI;
                                        gradient_[jColumn] += gradientJ;
                                   } else {
                                        offset += 0.5 * valueI * valueI * elementValue;
                                        double gradientI = valueI * elementValue;
                                        gradient_[iColumn] += gradientI;
                                   }
                              }
                         }
                    }
               }
          }
          if (model)
               offset *= model->optimizationDirection();
          return gradient_;
     }
}

//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpObjective * ClpQuadraticObjective::clone() const
{
     return new ClpQuadraticObjective(*this);
}
/* Subset clone.  Duplicates are allowed
   and order is as given.
*/
ClpObjective *
ClpQuadraticObjective::subsetClone (int numberColumns,
                                    const int * whichColumns) const
{
     return new ClpQuadraticObjective(*this, numberColumns, whichColumns);
}
// Resize objective
void
ClpQuadraticObjective::resize(int newNumberColumns)
{
     if (numberColumns_ != newNumberColumns) {
          int newExtended = newNumberColumns + (numberExtendedColumns_ - numberColumns_);
          int i;
          double * newArray = new double[newExtended];
          if (objective_)
               CoinMemcpyN(objective_,	CoinMin(newExtended, numberExtendedColumns_), newArray);
          delete [] objective_;
          objective_ = newArray;
          for (i = numberColumns_; i < newNumberColumns; i++)
               objective_[i] = 0.0;
          if (gradient_) {
               newArray = new double[newExtended];
               if (gradient_)
                    CoinMemcpyN(gradient_,	CoinMin(newExtended, numberExtendedColumns_), newArray);
               delete [] gradient_;
               gradient_ = newArray;
               for (i = numberColumns_; i < newNumberColumns; i++)
                    gradient_[i] = 0.0;
          }
          if (quadraticObjective_) {
               if (newNumberColumns < numberColumns_) {
                    int * which = new int[numberColumns_-newNumberColumns];
                    int i;
                    for (i = newNumberColumns; i < numberColumns_; i++)
                         which[i-newNumberColumns] = i;
                    quadraticObjective_->deleteRows(numberColumns_ - newNumberColumns, which);
                    quadraticObjective_->deleteCols(numberColumns_ - newNumberColumns, which);
                    delete [] which;
               } else {
                    quadraticObjective_->setDimensions(newNumberColumns, newNumberColumns);
               }
          }
          numberColumns_ = newNumberColumns;
          numberExtendedColumns_ = newExtended;
     }

}
// Delete columns in  objective
void
ClpQuadraticObjective::deleteSome(int numberToDelete, const int * which)
{
     int newNumberColumns = numberColumns_ - numberToDelete;
     int newExtended = numberExtendedColumns_ - numberToDelete;
     if (objective_) {
          int i ;
          char * deleted = new char[numberColumns_];
          int numberDeleted = 0;
          memset(deleted, 0, numberColumns_ * sizeof(char));
          for (i = 0; i < numberToDelete; i++) {
               int j = which[i];
               if (j >= 0 && j < numberColumns_ && !deleted[j]) {
                    numberDeleted++;
                    deleted[j] = 1;
               }
          }
          newNumberColumns = numberColumns_ - numberDeleted;
          newExtended = numberExtendedColumns_ - numberDeleted;
          double * newArray = new double[newExtended];
          int put = 0;
          for (i = 0; i < numberColumns_; i++) {
               if (!deleted[i]) {
                    newArray[put++] = objective_[i];
               }
          }
          delete [] objective_;
          objective_ = newArray;
          delete [] deleted;
          CoinMemcpyN(objective_ + numberColumns_,	(numberExtendedColumns_ - numberColumns_),
                      objective_ + newNumberColumns);
     }
     if (gradient_) {
          int i ;
          char * deleted = new char[numberColumns_];
          int numberDeleted = 0;
          memset(deleted, 0, numberColumns_ * sizeof(char));
          for (i = 0; i < numberToDelete; i++) {
               int j = which[i];
               if (j >= 0 && j < numberColumns_ && !deleted[j]) {
                    numberDeleted++;
                    deleted[j] = 1;
               }
          }
          newNumberColumns = numberColumns_ - numberDeleted;
          newExtended = numberExtendedColumns_ - numberDeleted;
          double * newArray = new double[newExtended];
          int put = 0;
          for (i = 0; i < numberColumns_; i++) {
               if (!deleted[i]) {
                    newArray[put++] = gradient_[i];
               }
          }
          delete [] gradient_;
          gradient_ = newArray;
          delete [] deleted;
          CoinMemcpyN(gradient_ + numberColumns_,	(numberExtendedColumns_ - numberColumns_),
                      gradient_ + newNumberColumns);
     }
     numberColumns_ = newNumberColumns;
     numberExtendedColumns_ = newExtended;
     if (quadraticObjective_) {
          quadraticObjective_->deleteCols(numberToDelete, which);
          quadraticObjective_->deleteRows(numberToDelete, which);
     }
}

// Load up quadratic objective
void
ClpQuadraticObjective::loadQuadraticObjective(const int numberColumns, const CoinBigIndex * start,
          const int * column, const double * element, int numberExtended)
{
     fullMatrix_ = false;
     delete quadraticObjective_;
     quadraticObjective_ = new CoinPackedMatrix(true, numberColumns, numberColumns,
               start[numberColumns], element, column, start, NULL);
     numberColumns_ = numberColumns;
     if (numberExtended > numberExtendedColumns_) {
          if (objective_) {
               // make correct size
               double * newArray = new double[numberExtended];
               CoinMemcpyN(objective_, numberColumns_, newArray);
               delete [] objective_;
               objective_ = newArray;
               memset(objective_ + numberColumns_, 0, (numberExtended - numberColumns_)*sizeof(double));
          }
          if (gradient_) {
               // make correct size
               double * newArray = new double[numberExtended];
               CoinMemcpyN(gradient_, numberColumns_, newArray);
               delete [] gradient_;
               gradient_ = newArray;
               memset(gradient_ + numberColumns_, 0, (numberExtended - numberColumns_)*sizeof(double));
          }
          numberExtendedColumns_ = numberExtended;
     } else {
          numberExtendedColumns_ = numberColumns_;
     }
}
void
ClpQuadraticObjective::loadQuadraticObjective (  const CoinPackedMatrix& matrix)
{
     delete quadraticObjective_;
     quadraticObjective_ = new CoinPackedMatrix(matrix);
}
// Get rid of quadratic objective
void
ClpQuadraticObjective::deleteQuadraticObjective()
{
     delete quadraticObjective_;
     quadraticObjective_ = NULL;
}
/* Returns reduced gradient.Returns an offset (to be added to current one).
 */
double
ClpQuadraticObjective::reducedGradient(ClpSimplex * model, double * region,
                                       bool useFeasibleCosts)
{
     int numberRows = model->numberRows();
     int numberColumns = model->numberColumns();

     //work space
     CoinIndexedVector  * workSpace = model->rowArray(0);

     CoinIndexedVector arrayVector;
     arrayVector.reserve(numberRows + 1);

     int iRow;
#ifdef CLP_DEBUG
     workSpace->checkClear();
#endif
     double * array = arrayVector.denseVector();
     int * index = arrayVector.getIndices();
     int number = 0;
     const double * costNow = gradient(model, model->solutionRegion(), offset_,
                                       true, useFeasibleCosts ? 2 : 1);
     double * cost = model->costRegion();
     const int * pivotVariable = model->pivotVariable();
     for (iRow = 0; iRow < numberRows; iRow++) {
          int iPivot = pivotVariable[iRow];
          double value;
          if (iPivot < numberColumns)
               value = costNow[iPivot];
          else if (!useFeasibleCosts)
               value = cost[iPivot];
          else
               value = 0.0;
          if (value) {
               array[iRow] = value;
               index[number++] = iRow;
          }
     }
     arrayVector.setNumElements(number);

     // Btran basic costs
     model->factorization()->updateColumnTranspose(workSpace, &arrayVector);
     double * work = workSpace->denseVector();
     ClpFillN(work, numberRows, 0.0);
     // now look at dual solution
     double * rowReducedCost = region + numberColumns;
     double * dual = rowReducedCost;
     const double * rowCost = cost + numberColumns;
     for (iRow = 0; iRow < numberRows; iRow++) {
          dual[iRow] = array[iRow];
     }
     double * dj = region;
     ClpDisjointCopyN(costNow, numberColumns, dj);

     model->transposeTimes(-1.0, dual, dj);
     for (iRow = 0; iRow < numberRows; iRow++) {
          // slack
          double value = dual[iRow];
          value += rowCost[iRow];
          rowReducedCost[iRow] = value;
     }
     return offset_;
}
/* Returns step length which gives minimum of objective for
   solution + theta * change vector up to maximum theta.

   arrays are numberColumns+numberRows
*/
double
ClpQuadraticObjective::stepLength(ClpSimplex * model,
                                  const double * solution,
                                  const double * change,
                                  double maximumTheta,
                                  double & currentObj,
                                  double & predictedObj,
                                  double & thetaObj)
{
     const double * cost = model->costRegion();
     bool inSolve = true;
     if (!cost) {
          // not in solve
          cost = objective_;
          inSolve = false;
     }
     double delta = 0.0;
     double linearCost = 0.0;
     int numberRows = model->numberRows();
     int numberColumns = model->numberColumns();
     int numberTotal = numberColumns;
     if (inSolve)
          numberTotal += numberRows;
     currentObj = 0.0;
     thetaObj = 0.0;
     for (int iColumn = 0; iColumn < numberTotal; iColumn++) {
          delta += cost[iColumn] * change[iColumn];
          linearCost += cost[iColumn] * solution[iColumn];
     }
     if (!activated_ || !quadraticObjective_) {
          currentObj = linearCost;
          thetaObj = currentObj + delta * maximumTheta;
          if (delta < 0.0) {
               return maximumTheta;
          } else {
	    COIN_DETAIL_PRINT(printf("odd linear direction %g\n", delta));
               return 0.0;
          }
     }
     assert (model);
     bool scaling = false;
     if ((model->rowScale() ||
               model->objectiveScale() != 1.0 || model->optimizationDirection() != 1.0) && inSolve)
          scaling = true;
     const int * columnQuadratic = quadraticObjective_->getIndices();
     const CoinBigIndex * columnQuadraticStart = quadraticObjective_->getVectorStarts();
     const int * columnQuadraticLength = quadraticObjective_->getVectorLengths();
     const double * quadraticElement = quadraticObjective_->getElements();
     double a = 0.0;
     double b = delta;
     double c = 0.0;
     if (!scaling) {
          if (!fullMatrix_) {
               int iColumn;
               for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                    double valueI = solution[iColumn];
                    double changeI = change[iColumn];
                    CoinBigIndex j;
                    for (j = columnQuadraticStart[iColumn];
                              j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                         int jColumn = columnQuadratic[j];
                         double valueJ = solution[jColumn];
                         double changeJ = change[jColumn];
                         double elementValue = quadraticElement[j];
                         if (iColumn != jColumn) {
                              a += changeI * changeJ * elementValue;
                              b += (changeI * valueJ + changeJ * valueI) * elementValue;
                              c += valueI * valueJ * elementValue;
                         } else {
                              a += 0.5 * changeI * changeI * elementValue;
                              b += changeI * valueI * elementValue;
                              c += 0.5 * valueI * valueI * elementValue;
                         }
                    }
               }
          } else {
               // full matrix stored
               int iColumn;
               for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                    double valueI = solution[iColumn];
                    double changeI = change[iColumn];
                    CoinBigIndex j;
                    for (j = columnQuadraticStart[iColumn];
                              j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                         int jColumn = columnQuadratic[j];
                         double valueJ = solution[jColumn];
                         double changeJ = change[jColumn];
                         double elementValue = quadraticElement[j];
                         valueJ *= elementValue;
                         a += changeI * changeJ * elementValue;
                         b += changeI * valueJ;
                         c += valueI * valueJ;
                    }
               }
               a *= 0.5;
               c *= 0.5;
          }
     } else {
          // scaling
          // for now only if half
          assert (!fullMatrix_);
          const double * columnScale = model->columnScale();
          double direction = model->optimizationDirection() * model->objectiveScale();
          // direction is actually scale out not scale in
          if (direction)
               direction = 1.0 / direction;
          if (!columnScale) {
               for (int iColumn = 0; iColumn < numberColumns_; iColumn++) {
                    double valueI = solution[iColumn];
                    double changeI = change[iColumn];
                    CoinBigIndex j;
                    for (j = columnQuadraticStart[iColumn];
                              j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                         int jColumn = columnQuadratic[j];
                         double valueJ = solution[jColumn];
                         double changeJ = change[jColumn];
                         double elementValue = quadraticElement[j];
                         elementValue *= direction;
                         if (iColumn != jColumn) {
                              a += changeI * changeJ * elementValue;
                              b += (changeI * valueJ + changeJ * valueI) * elementValue;
                              c += valueI * valueJ * elementValue;
                         } else {
                              a += 0.5 * changeI * changeI * elementValue;
                              b += changeI * valueI * elementValue;
                              c += 0.5 * valueI * valueI * elementValue;
                         }
                    }
               }
          } else {
               // scaling
               for (int iColumn = 0; iColumn < numberColumns_; iColumn++) {
                    double valueI = solution[iColumn];
                    double changeI = change[iColumn];
                    double scaleI = columnScale[iColumn] * direction;
                    CoinBigIndex j;
                    for (j = columnQuadraticStart[iColumn];
                              j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                         int jColumn = columnQuadratic[j];
                         double valueJ = solution[jColumn];
                         double changeJ = change[jColumn];
                         double elementValue = quadraticElement[j];
                         elementValue *= scaleI * columnScale[jColumn];
                         if (iColumn != jColumn) {
                              a += changeI * changeJ * elementValue;
                              b += (changeI * valueJ + changeJ * valueI) * elementValue;
                              c += valueI * valueJ * elementValue;
                         } else {
                              a += 0.5 * changeI * changeI * elementValue;
                              b += changeI * valueI * elementValue;
                              c += 0.5 * valueI * valueI * elementValue;
                         }
                    }
               }
          }
     }
     double theta;
     //printf("Current cost %g\n",c+linearCost);
     currentObj = c + linearCost;
     thetaObj = currentObj + a * maximumTheta * maximumTheta + b * maximumTheta;
     // minimize a*x*x + b*x + c
     if (a <= 0.0) {
          theta = maximumTheta;
     } else {
          theta = -0.5 * b / a;
     }
     predictedObj = currentObj + a * theta * theta + b * theta;
     if (b > 0.0) {
          if (model->messageHandler()->logLevel() & 32)
               printf("a %g b %g c %g => %g\n", a, b, c, theta);
          b = 0.0;
     }
     return CoinMin(theta, maximumTheta);
}
// Return objective value (without any ClpModel offset) (model may be NULL)
double
ClpQuadraticObjective::objectiveValue(const ClpSimplex * model, const double * solution) const
{
     bool scaling = false;
     if (model && (model->rowScale() ||
                   model->objectiveScale() != 1.0))
          scaling = true;
     const double * cost = NULL;
     if (model)
          cost = model->costRegion();
     if (!cost) {
          // not in solve
          cost = objective_;
          scaling = false;
     }
     double linearCost = 0.0;
     int numberColumns = model->numberColumns();
     int numberTotal = numberColumns;
     double currentObj = 0.0;
     for (int iColumn = 0; iColumn < numberTotal; iColumn++) {
          linearCost += cost[iColumn] * solution[iColumn];
     }
     if (!activated_ || !quadraticObjective_) {
          currentObj = linearCost;
          return currentObj;
     }
     assert (model);
     const int * columnQuadratic = quadraticObjective_->getIndices();
     const CoinBigIndex * columnQuadraticStart = quadraticObjective_->getVectorStarts();
     const int * columnQuadraticLength = quadraticObjective_->getVectorLengths();
     const double * quadraticElement = quadraticObjective_->getElements();
     double c = 0.0;
     if (!scaling) {
          if (!fullMatrix_) {
               int iColumn;
               for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                    double valueI = solution[iColumn];
                    CoinBigIndex j;
                    for (j = columnQuadraticStart[iColumn];
                              j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                         int jColumn = columnQuadratic[j];
                         double valueJ = solution[jColumn];
                         double elementValue = quadraticElement[j];
                         if (iColumn != jColumn) {
                              c += valueI * valueJ * elementValue;
                         } else {
                              c += 0.5 * valueI * valueI * elementValue;
                         }
                    }
               }
          } else {
               // full matrix stored
               int iColumn;
               for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
                    double valueI = solution[iColumn];
                    CoinBigIndex j;
                    for (j = columnQuadraticStart[iColumn];
                              j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                         int jColumn = columnQuadratic[j];
                         double valueJ = solution[jColumn];
                         double elementValue = quadraticElement[j];
                         valueJ *= elementValue;
                         c += valueI * valueJ;
                    }
               }
               c *= 0.5;
          }
     } else {
          // scaling
          // for now only if half
          assert (!fullMatrix_);
          const double * columnScale = model->columnScale();
          double direction = model->objectiveScale();
          // direction is actually scale out not scale in
          if (direction)
               direction = 1.0 / direction;
          if (!columnScale) {
               for (int iColumn = 0; iColumn < numberColumns_; iColumn++) {
                    double valueI = solution[iColumn];
                    CoinBigIndex j;
                    for (j = columnQuadraticStart[iColumn];
                              j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                         int jColumn = columnQuadratic[j];
                         double valueJ = solution[jColumn];
                         double elementValue = quadraticElement[j];
                         elementValue *= direction;
                         if (iColumn != jColumn) {
                              c += valueI * valueJ * elementValue;
                         } else {
                              c += 0.5 * valueI * valueI * elementValue;
                         }
                    }
               }
          } else {
               // scaling
               for (int iColumn = 0; iColumn < numberColumns_; iColumn++) {
                    double valueI = solution[iColumn];
                    double scaleI = columnScale[iColumn] * direction;
                    CoinBigIndex j;
                    for (j = columnQuadraticStart[iColumn];
                              j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
                         int jColumn = columnQuadratic[j];
                         double valueJ = solution[jColumn];
                         double elementValue = quadraticElement[j];
                         elementValue *= scaleI * columnScale[jColumn];
                         if (iColumn != jColumn) {
                              c += valueI * valueJ * elementValue;
                         } else {
                              c += 0.5 * valueI * valueI * elementValue;
                         }
                    }
               }
          }
     }
     currentObj = c + linearCost;
     return currentObj;
}
// Scale objective
void
ClpQuadraticObjective::reallyScale(const double * columnScale)
{
     const int * columnQuadratic = quadraticObjective_->getIndices();
     const CoinBigIndex * columnQuadraticStart = quadraticObjective_->getVectorStarts();
     const int * columnQuadraticLength = quadraticObjective_->getVectorLengths();
     double * quadraticElement = quadraticObjective_->getMutableElements();
     for (int iColumn = 0; iColumn < numberColumns_; iColumn++) {
          double scaleI = columnScale[iColumn];
          objective_[iColumn] *= scaleI;
          CoinBigIndex j;
          for (j = columnQuadraticStart[iColumn];
                    j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
               int jColumn = columnQuadratic[j];
               quadraticElement[j] *= scaleI * columnScale[jColumn];
          }
     }
}
/* Given a zeroed array sets nonlinear columns to 1.
   Returns number of nonlinear columns
*/
int
ClpQuadraticObjective::markNonlinear(char * which)
{
     int iColumn;
     const int * columnQuadratic = quadraticObjective_->getIndices();
     const CoinBigIndex * columnQuadraticStart = quadraticObjective_->getVectorStarts();
     const int * columnQuadraticLength = quadraticObjective_->getVectorLengths();
     for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
          CoinBigIndex j;
          for (j = columnQuadraticStart[iColumn];
                    j < columnQuadraticStart[iColumn] + columnQuadraticLength[iColumn]; j++) {
               int jColumn = columnQuadratic[j];
               which[jColumn] = 1;
               which[iColumn] = 1;
          }
     }
     int numberNonLinearColumns = 0;
     for (iColumn = 0; iColumn < numberColumns_; iColumn++) {
          if(which[iColumn])
               numberNonLinearColumns++;
     }
     return numberNonLinearColumns;
}
