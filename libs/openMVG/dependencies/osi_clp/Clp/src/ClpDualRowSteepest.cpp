/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"
#include "ClpSimplex.hpp"
#include "ClpDualRowSteepest.hpp"
#include "CoinIndexedVector.hpp"
#include "ClpFactorization.hpp"
#include "CoinHelperFunctions.hpp"
#include <cstdio>
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################
//#define CLP_DEBUG 4
//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpDualRowSteepest::ClpDualRowSteepest (int mode)
     : ClpDualRowPivot(),
       state_(-1),
       mode_(mode),
       persistence_(normal),
       weights_(NULL),
       infeasible_(NULL),
       alternateWeights_(NULL),
       savedWeights_(NULL),
       dubiousWeights_(NULL)
{
     type_ = 2 + 64 * mode;
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpDualRowSteepest::ClpDualRowSteepest (const ClpDualRowSteepest & rhs)
     : ClpDualRowPivot(rhs)
{
     state_ = rhs.state_;
     mode_ = rhs.mode_;
     persistence_ = rhs.persistence_;
     model_ = rhs.model_;
     if ((model_ && model_->whatsChanged() & 1) != 0) {
          int number = model_->numberRows();
          if (rhs.savedWeights_)
               number = CoinMin(number, rhs.savedWeights_->capacity());
          if (rhs.infeasible_) {
               infeasible_ = new CoinIndexedVector(rhs.infeasible_);
          } else {
               infeasible_ = NULL;
          }
          if (rhs.weights_) {
               weights_ = new double[number];
               ClpDisjointCopyN(rhs.weights_, number, weights_);
          } else {
               weights_ = NULL;
          }
          if (rhs.alternateWeights_) {
               alternateWeights_ = new CoinIndexedVector(rhs.alternateWeights_);
          } else {
               alternateWeights_ = NULL;
          }
          if (rhs.savedWeights_) {
               savedWeights_ = new CoinIndexedVector(rhs.savedWeights_);
          } else {
               savedWeights_ = NULL;
          }
          if (rhs.dubiousWeights_) {
               assert(model_);
               int number = model_->numberRows();
               dubiousWeights_ = new int[number];
               ClpDisjointCopyN(rhs.dubiousWeights_, number, dubiousWeights_);
          } else {
               dubiousWeights_ = NULL;
          }
     } else {
          infeasible_ = NULL;
          weights_ = NULL;
          alternateWeights_ = NULL;
          savedWeights_ = NULL;
          dubiousWeights_ = NULL;
     }
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpDualRowSteepest::~ClpDualRowSteepest ()
{
     delete [] weights_;
     delete [] dubiousWeights_;
     delete infeasible_;
     delete alternateWeights_;
     delete savedWeights_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpDualRowSteepest &
ClpDualRowSteepest::operator=(const ClpDualRowSteepest& rhs)
{
     if (this != &rhs) {
          ClpDualRowPivot::operator=(rhs);
          state_ = rhs.state_;
          mode_ = rhs.mode_;
          persistence_ = rhs.persistence_;
          model_ = rhs.model_;
          delete [] weights_;
          delete [] dubiousWeights_;
          delete infeasible_;
          delete alternateWeights_;
          delete savedWeights_;
          assert(model_);
          int number = model_->numberRows();
          if (rhs.savedWeights_)
               number = CoinMin(number, rhs.savedWeights_->capacity());
          if (rhs.infeasible_ != NULL) {
               infeasible_ = new CoinIndexedVector(rhs.infeasible_);
          } else {
               infeasible_ = NULL;
          }
          if (rhs.weights_ != NULL) {
               weights_ = new double[number];
               ClpDisjointCopyN(rhs.weights_, number, weights_);
          } else {
               weights_ = NULL;
          }
          if (rhs.alternateWeights_ != NULL) {
               alternateWeights_ = new CoinIndexedVector(rhs.alternateWeights_);
          } else {
               alternateWeights_ = NULL;
          }
          if (rhs.savedWeights_ != NULL) {
               savedWeights_ = new CoinIndexedVector(rhs.savedWeights_);
          } else {
               savedWeights_ = NULL;
          }
          if (rhs.dubiousWeights_) {
               assert(model_);
               int number = model_->numberRows();
               dubiousWeights_ = new int[number];
               ClpDisjointCopyN(rhs.dubiousWeights_, number, dubiousWeights_);
          } else {
               dubiousWeights_ = NULL;
          }
     }
     return *this;
}
// Fill most values
void
ClpDualRowSteepest::fill(const ClpDualRowSteepest& rhs)
{
     state_ = rhs.state_;
     mode_ = rhs.mode_;
     persistence_ = rhs.persistence_;
     assert (model_->numberRows() == rhs.model_->numberRows());
     model_ = rhs.model_;
     assert(model_);
     int number = model_->numberRows();
     if (rhs.savedWeights_)
          number = CoinMin(number, rhs.savedWeights_->capacity());
     if (rhs.infeasible_ != NULL) {
          if (!infeasible_)
               infeasible_ = new CoinIndexedVector(rhs.infeasible_);
          else
               *infeasible_ = *rhs.infeasible_;
     } else {
          delete infeasible_;
          infeasible_ = NULL;
     }
     if (rhs.weights_ != NULL) {
          if (!weights_)
               weights_ = new double[number];
          ClpDisjointCopyN(rhs.weights_, number, weights_);
     } else {
          delete [] weights_;
          weights_ = NULL;
     }
     if (rhs.alternateWeights_ != NULL) {
          if (!alternateWeights_)
               alternateWeights_ = new CoinIndexedVector(rhs.alternateWeights_);
          else
               *alternateWeights_ = *rhs.alternateWeights_;
     } else {
          delete alternateWeights_;
          alternateWeights_ = NULL;
     }
     if (rhs.savedWeights_ != NULL) {
          if (!savedWeights_)
               savedWeights_ = new CoinIndexedVector(rhs.savedWeights_);
          else
               *savedWeights_ = *rhs.savedWeights_;
     } else {
          delete savedWeights_;
          savedWeights_ = NULL;
     }
     if (rhs.dubiousWeights_) {
          assert(model_);
          int number = model_->numberRows();
          if (!dubiousWeights_)
               dubiousWeights_ = new int[number];
          ClpDisjointCopyN(rhs.dubiousWeights_, number, dubiousWeights_);
     } else {
          delete [] dubiousWeights_;
          dubiousWeights_ = NULL;
     }
}
// Returns pivot row, -1 if none
int
ClpDualRowSteepest::pivotRow()
{
     assert(model_);
     int i, iRow;
     double * infeas = infeasible_->denseVector();
     double largest = 0.0;
     int * index = infeasible_->getIndices();
     int number = infeasible_->getNumElements();
     const int * pivotVariable = model_->pivotVariable();
     int chosenRow = -1;
     int lastPivotRow = model_->pivotRow();
     assert (lastPivotRow < model_->numberRows());
     double tolerance = model_->currentPrimalTolerance();
     // we can't really trust infeasibilities if there is primal error
     // this coding has to mimic coding in checkPrimalSolution
     double error = CoinMin(1.0e-2, model_->largestPrimalError());
     // allow tolerance at least slightly bigger than standard
     tolerance = tolerance +  error;
     // But cap
     tolerance = CoinMin(1000.0, tolerance);
     tolerance *= tolerance; // as we are using squares
     bool toleranceChanged = false;
     double * solution = model_->solutionRegion();
     double * lower = model_->lowerRegion();
     double * upper = model_->upperRegion();
     // do last pivot row one here
     //#define CLP_DUAL_FIXED_COLUMN_MULTIPLIER 10.0
     if (lastPivotRow >= 0 && lastPivotRow < model_->numberRows()) {
#ifdef CLP_DUAL_COLUMN_MULTIPLIER
          int numberColumns = model_->numberColumns();
#endif
          int iPivot = pivotVariable[lastPivotRow];
          double value = solution[iPivot];
          double lower = model_->lower(iPivot);
          double upper = model_->upper(iPivot);
          if (value > upper + tolerance) {
               value -= upper;
               value *= value;
#ifdef CLP_DUAL_COLUMN_MULTIPLIER
               if (iPivot < numberColumns)
                    value *= CLP_DUAL_COLUMN_MULTIPLIER; // bias towards columns
#endif
               // store square in list
               if (infeas[lastPivotRow])
                    infeas[lastPivotRow] = value; // already there
               else
                    infeasible_->quickAdd(lastPivotRow, value);
          } else if (value < lower - tolerance) {
               value -= lower;
               value *= value;
#ifdef CLP_DUAL_COLUMN_MULTIPLIER
               if (iPivot < numberColumns)
                    value *= CLP_DUAL_COLUMN_MULTIPLIER; // bias towards columns
#endif
               // store square in list
               if (infeas[lastPivotRow])
                    infeas[lastPivotRow] = value; // already there
               else
                    infeasible_->add(lastPivotRow, value);
          } else {
               // feasible - was it infeasible - if so set tiny
               if (infeas[lastPivotRow])
                    infeas[lastPivotRow] = COIN_INDEXED_REALLY_TINY_ELEMENT;
          }
          number = infeasible_->getNumElements();
     }
     if(model_->numberIterations() < model_->lastBadIteration() + 200) {
          // we can't really trust infeasibilities if there is dual error
          if (model_->largestDualError() > model_->largestPrimalError()) {
               tolerance *= CoinMin(model_->largestDualError() / model_->largestPrimalError(), 1000.0);
               toleranceChanged = true;
          }
     }
     int numberWanted;
     if (mode_ < 2 ) {
          numberWanted = number + 1;
     } else if (mode_ == 2) {
          numberWanted = CoinMax(2000, number / 8);
     } else {
          int numberElements = model_->factorization()->numberElements();
          double ratio = static_cast<double> (numberElements) /
                         static_cast<double> (model_->numberRows());
          numberWanted = CoinMax(2000, number / 8);
          if (ratio < 1.0) {
               numberWanted = CoinMax(2000, number / 20);
          } else if (ratio > 10.0) {
               ratio = number * (ratio / 80.0);
               if (ratio > number)
                    numberWanted = number + 1;
               else
                    numberWanted = CoinMax(2000, static_cast<int> (ratio));
          }
     }
     if (model_->largestPrimalError() > 1.0e-3)
          numberWanted = number + 1; // be safe
     int iPass;
     // Setup two passes
     int start[4];
     start[1] = number;
     start[2] = 0;
     double dstart = static_cast<double> (number) * model_->randomNumberGenerator()->randomDouble();
     start[0] = static_cast<int> (dstart);
     start[3] = start[0];
     //double largestWeight=0.0;
     //double smallestWeight=1.0e100;
     for (iPass = 0; iPass < 2; iPass++) {
          int end = start[2*iPass+1];
          for (i = start[2*iPass]; i < end; i++) {
               iRow = index[i];
               double value = infeas[iRow];
               if (value > tolerance) {
                    //#define OUT_EQ
#ifdef OUT_EQ
                    {
                         int iSequence = pivotVariable[iRow];
                         if (upper[iSequence] == lower[iSequence])
                              value *= 2.0;
                    }
#endif
                    double weight = CoinMin(weights_[iRow], 1.0e50);
                    //largestWeight = CoinMax(largestWeight,weight);
                    //smallestWeight = CoinMin(smallestWeight,weight);
                    //double dubious = dubiousWeights_[iRow];
                    //weight *= dubious;
                    //if (value>2.0*largest*weight||(value>0.5*largest*weight&&value*largestWeight>dubious*largest*weight)) {
                    if (value > largest * weight) {
                         // make last pivot row last resort choice
                         if (iRow == lastPivotRow) {
                              if (value * 1.0e-10 < largest * weight)
                                   continue;
                              else
                                   value *= 1.0e-10;
                         }
                         int iSequence = pivotVariable[iRow];
                         if (!model_->flagged(iSequence)) {
                              //#define CLP_DEBUG 3
#ifdef CLP_DEBUG
                              double value2 = 0.0;
                              if (solution[iSequence] > upper[iSequence] + tolerance)
                                   value2 = solution[iSequence] - upper[iSequence];
                              else if (solution[iSequence] < lower[iSequence] - tolerance)
                                   value2 = solution[iSequence] - lower[iSequence];
                              assert(fabs(value2 * value2 - infeas[iRow]) < 1.0e-8 * CoinMin(value2 * value2, infeas[iRow]));
#endif
                              if (solution[iSequence] > upper[iSequence] + tolerance ||
                                        solution[iSequence] < lower[iSequence] - tolerance) {
                                   chosenRow = iRow;
                                   largest = value / weight;
                                   //largestWeight = dubious;
                              }
                         } else {
                              // just to make sure we don't exit before got something
                              numberWanted++;
                         }
                    }
                    numberWanted--;
                    if (!numberWanted)
                         break;
               }
          }
          if (!numberWanted)
               break;
     }
     //printf("smallest %g largest %g\n",smallestWeight,largestWeight);
     if (chosenRow < 0 && toleranceChanged) {
          // won't line up with checkPrimalSolution - do again
          double saveError = model_->largestDualError();
          model_->setLargestDualError(0.0);
          // can't loop
          chosenRow = pivotRow();
          model_->setLargestDualError(saveError);
     }
     if (chosenRow < 0 && lastPivotRow < 0) {
          int nLeft = 0;
          for (int i = 0; i < number; i++) {
               int iRow = index[i];
               if (fabs(infeas[iRow]) > 1.0e-50) {
                    index[nLeft++] = iRow;
               } else {
                    infeas[iRow] = 0.0;
               }
          }
          infeasible_->setNumElements(nLeft);
          model_->setNumberPrimalInfeasibilities(nLeft);
     }
     return chosenRow;
}
#if 0
static double ft_count = 0.0;
static double up_count = 0.0;
static double ft_count_in = 0.0;
static double up_count_in = 0.0;
static int xx_count = 0;
#endif
/* Updates weights and returns pivot alpha.
   Also does FT update */
double
ClpDualRowSteepest::updateWeights(CoinIndexedVector * input,
                                  CoinIndexedVector * spare,
                                  CoinIndexedVector * spare2,
                                  CoinIndexedVector * updatedColumn)
{
     //#define CLP_DEBUG 3
#if CLP_DEBUG>2
     // Very expensive debug
     {
          int numberRows = model_->numberRows();
          CoinIndexedVector * temp = new CoinIndexedVector();
          temp->reserve(numberRows +
                        model_->factorization()->maximumPivots());
          double * array = alternateWeights_->denseVector();
          int * which = alternateWeights_->getIndices();
          alternateWeights_->clear();
          int i;
          for (i = 0; i < numberRows; i++) {
               double value = 0.0;
               array[i] = 1.0;
               which[0] = i;
               alternateWeights_->setNumElements(1);
               model_->factorization()->updateColumnTranspose(temp,
                         alternateWeights_);
               int number = alternateWeights_->getNumElements();
               int j;
               for (j = 0; j < number; j++) {
                    int iRow = which[j];
                    value += array[iRow] * array[iRow];
                    array[iRow] = 0.0;
               }
               alternateWeights_->setNumElements(0);
               double w = CoinMax(weights_[i], value) * .1;
               if (fabs(weights_[i] - value) > w) {
                    printf("%d old %g, true %g\n", i, weights_[i], value);
                    weights_[i] = value; // to reduce printout
               }
               //else
               //printf("%d matches %g\n",i,value);
          }
          delete temp;
     }
#endif
     assert (input->packedMode());
     if (!updatedColumn->packedMode()) {
          // I think this means empty
#ifdef COIN_DEVELOP
          printf("updatedColumn not packed mode ClpDualRowSteepest::updateWeights\n");
#endif
          return 0.0;
     }
     double alpha = 0.0;
     if (!model_->factorization()->networkBasis()) {
          // clear other region
          alternateWeights_->clear();
          double norm = 0.0;
          int i;
          double * work = input->denseVector();
          int numberNonZero = input->getNumElements();
          int * which = input->getIndices();
          double * work2 = spare->denseVector();
          int * which2 = spare->getIndices();
          // ftran
          //permute and move indices into index array
          //also compute norm
          //int *regionIndex = alternateWeights_->getIndices (  );
          const int *permute = model_->factorization()->permute();
          //double * region = alternateWeights_->denseVector();
          if (permute) {
               for ( i = 0; i < numberNonZero; i ++ ) {
                    int iRow = which[i];
                    double value = work[i];
                    norm += value * value;
                    iRow = permute[iRow];
                    work2[iRow] = value;
                    which2[i] = iRow;
               }
          } else {
               for ( i = 0; i < numberNonZero; i ++ ) {
                    int iRow = which[i];
                    double value = work[i];
                    norm += value * value;
                    //iRow = permute[iRow];
                    work2[iRow] = value;
                    which2[i] = iRow;
               }
          }
          spare->setNumElements ( numberNonZero );
          // Do FT update
#if 0
          ft_count_in += updatedColumn->getNumElements();
          up_count_in += spare->getNumElements();
#endif
          if (permute || true) {
#if CLP_DEBUG>2
               printf("REGION before %d els\n", spare->getNumElements());
               spare->print();
#endif
               model_->factorization()->updateTwoColumnsFT(spare2, updatedColumn,
                         spare, permute != NULL);
#if CLP_DEBUG>2
               printf("REGION after %d els\n", spare->getNumElements());
               spare->print();
#endif
          } else {
               // Leave as old way
               model_->factorization()->updateColumnFT(spare2, updatedColumn);
               model_->factorization()->updateColumn(spare2, spare, false);
          }
#undef CLP_DEBUG
#if 0
          ft_count += updatedColumn->getNumElements();
          up_count += spare->getNumElements();
          xx_count++;
          if ((xx_count % 1000) == 0)
               printf("zz %d ft %g up %g (in %g %g)\n", xx_count, ft_count, up_count,
                      ft_count_in, up_count_in);
#endif
          numberNonZero = spare->getNumElements();
          // alternateWeights_ should still be empty
          int pivotRow = model_->pivotRow();
#ifdef CLP_DEBUG
          if ( model_->logLevel (  ) > 4  &&
                    fabs(norm - weights_[pivotRow]) > 1.0e-3 * (1.0 + norm))
               printf("on row %d, true weight %g, old %g\n",
                      pivotRow, sqrt(norm), sqrt(weights_[pivotRow]));
#endif
          // could re-initialize here (could be expensive)
          norm /= model_->alpha() * model_->alpha();
          assert(model_->alpha());
          assert(norm);
          // pivot element
          alpha = 0.0;
          double multiplier = 2.0 / model_->alpha();
          // look at updated column
          work = updatedColumn->denseVector();
          numberNonZero = updatedColumn->getNumElements();
          which = updatedColumn->getIndices();

          int nSave = 0;
          double * work3 = alternateWeights_->denseVector();
          int * which3 = alternateWeights_->getIndices();
          const int * pivotColumn = model_->factorization()->pivotColumn();
          for (i = 0; i < numberNonZero; i++) {
               int iRow = which[i];
               double theta = work[i];
               if (iRow == pivotRow)
                    alpha = theta;
               double devex = weights_[iRow];
               work3[nSave] = devex; // save old
               which3[nSave++] = iRow;
               // transform to match spare
               int jRow = permute ? pivotColumn[iRow] : iRow;
               double value = work2[jRow];
               devex +=  theta * (theta * norm + value * multiplier);
               if (devex < DEVEX_TRY_NORM)
                    devex = DEVEX_TRY_NORM;
               weights_[iRow] = devex;
          }
          alternateWeights_->setPackedMode(true);
          alternateWeights_->setNumElements(nSave);
          if (norm < DEVEX_TRY_NORM)
               norm = DEVEX_TRY_NORM;
          // Try this to make less likely will happen again and stop cycling
          //norm *= 1.02;
          weights_[pivotRow] = norm;
          spare->clear();
#ifdef CLP_DEBUG
          spare->checkClear();
#endif
     } else {
          // Do FT update
          model_->factorization()->updateColumnFT(spare, updatedColumn);
          // clear other region
          alternateWeights_->clear();
          double norm = 0.0;
          int i;
          double * work = input->denseVector();
          int number = input->getNumElements();
          int * which = input->getIndices();
          double * work2 = spare->denseVector();
          int * which2 = spare->getIndices();
          for (i = 0; i < number; i++) {
               int iRow = which[i];
               double value = work[i];
               norm += value * value;
               work2[iRow] = value;
               which2[i] = iRow;
          }
          spare->setNumElements(number);
          // ftran
#ifndef NDEBUG
          alternateWeights_->checkClear();
#endif
          model_->factorization()->updateColumn(alternateWeights_, spare);
          // alternateWeights_ should still be empty
#ifndef NDEBUG
          alternateWeights_->checkClear();
#endif
          int pivotRow = model_->pivotRow();
#ifdef CLP_DEBUG
          if ( model_->logLevel (  ) > 4  &&
                    fabs(norm - weights_[pivotRow]) > 1.0e-3 * (1.0 + norm))
               printf("on row %d, true weight %g, old %g\n",
                      pivotRow, sqrt(norm), sqrt(weights_[pivotRow]));
#endif
          // could re-initialize here (could be expensive)
          norm /= model_->alpha() * model_->alpha();

          assert(norm);
          //if (norm < DEVEX_TRY_NORM)
          //norm = DEVEX_TRY_NORM;
          // pivot element
          alpha = 0.0;
          double multiplier = 2.0 / model_->alpha();
          // look at updated column
          work = updatedColumn->denseVector();
          number = updatedColumn->getNumElements();
          which = updatedColumn->getIndices();

          int nSave = 0;
          double * work3 = alternateWeights_->denseVector();
          int * which3 = alternateWeights_->getIndices();
          for (i = 0; i < number; i++) {
               int iRow = which[i];
               double theta = work[i];
               if (iRow == pivotRow)
                    alpha = theta;
               double devex = weights_[iRow];
               work3[nSave] = devex; // save old
               which3[nSave++] = iRow;
               double value = work2[iRow];
               devex +=  theta * (theta * norm + value * multiplier);
               if (devex < DEVEX_TRY_NORM)
                    devex = DEVEX_TRY_NORM;
               weights_[iRow] = devex;
          }
          if (!alpha) {
               // error - but carry on
               alpha = 1.0e-50;
          }
          alternateWeights_->setPackedMode(true);
          alternateWeights_->setNumElements(nSave);
          if (norm < DEVEX_TRY_NORM)
               norm = DEVEX_TRY_NORM;
          weights_[pivotRow] = norm;
          spare->clear();
     }
#ifdef CLP_DEBUG
     spare->checkClear();
#endif
     return alpha;
}

/* Updates primal solution (and maybe list of candidates)
   Uses input vector which it deletes
   Computes change in objective function
*/
void
ClpDualRowSteepest::updatePrimalSolution(
     CoinIndexedVector * primalUpdate,
     double primalRatio,
     double & objectiveChange)
{
     double * COIN_RESTRICT work = primalUpdate->denseVector();
     int number = primalUpdate->getNumElements();
     int * COIN_RESTRICT which = primalUpdate->getIndices();
     int i;
     double changeObj = 0.0;
     double tolerance = model_->currentPrimalTolerance();
     const int * COIN_RESTRICT pivotVariable = model_->pivotVariable();
     double * COIN_RESTRICT infeas = infeasible_->denseVector();
     double * COIN_RESTRICT solution = model_->solutionRegion();
     const double * COIN_RESTRICT costModel = model_->costRegion();
     const double * COIN_RESTRICT lowerModel = model_->lowerRegion();
     const double * COIN_RESTRICT upperModel = model_->upperRegion();
#ifdef CLP_DUAL_COLUMN_MULTIPLIER
     int numberColumns = model_->numberColumns();
#endif
     if (primalUpdate->packedMode()) {
          for (i = 0; i < number; i++) {
               int iRow = which[i];
               int iPivot = pivotVariable[iRow];
               double value = solution[iPivot];
               double cost = costModel[iPivot];
               double change = primalRatio * work[i];
               work[i] = 0.0;
               value -= change;
               changeObj -= change * cost;
               double lower = lowerModel[iPivot];
               double upper = upperModel[iPivot];
               solution[iPivot] = value;
               if (value < lower - tolerance) {
                    value -= lower;
                    value *= value;
#ifdef CLP_DUAL_COLUMN_MULTIPLIER
                    if (iPivot < numberColumns)
                         value *= CLP_DUAL_COLUMN_MULTIPLIER; // bias towards columns
#endif
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
                    if (lower == upper)
                         value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
                    // store square in list
                    if (infeas[iRow])
                         infeas[iRow] = value; // already there
                    else
                         infeasible_->quickAdd(iRow, value);
               } else if (value > upper + tolerance) {
                    value -= upper;
                    value *= value;
#ifdef CLP_DUAL_COLUMN_MULTIPLIER
                    if (iPivot < numberColumns)
                         value *= CLP_DUAL_COLUMN_MULTIPLIER; // bias towards columns
#endif
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
                    if (lower == upper)
                         value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
                    // store square in list
                    if (infeas[iRow])
                         infeas[iRow] = value; // already there
                    else
                         infeasible_->quickAdd(iRow, value);
               } else {
                    // feasible - was it infeasible - if so set tiny
                    if (infeas[iRow])
                         infeas[iRow] = COIN_INDEXED_REALLY_TINY_ELEMENT;
               }
          }
     } else {
          for (i = 0; i < number; i++) {
               int iRow = which[i];
               int iPivot = pivotVariable[iRow];
               double value = solution[iPivot];
               double cost = costModel[iPivot];
               double change = primalRatio * work[iRow];
               value -= change;
               changeObj -= change * cost;
               double lower = lowerModel[iPivot];
               double upper = upperModel[iPivot];
               solution[iPivot] = value;
               if (value < lower - tolerance) {
                    value -= lower;
                    value *= value;
#ifdef CLP_DUAL_COLUMN_MULTIPLIER
                    if (iPivot < numberColumns)
                         value *= CLP_DUAL_COLUMN_MULTIPLIER; // bias towards columns
#endif
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
                    if (lower == upper)
                         value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
                    // store square in list
                    if (infeas[iRow])
                         infeas[iRow] = value; // already there
                    else
                         infeasible_->quickAdd(iRow, value);
               } else if (value > upper + tolerance) {
                    value -= upper;
                    value *= value;
#ifdef CLP_DUAL_COLUMN_MULTIPLIER
                    if (iPivot < numberColumns)
                         value *= CLP_DUAL_COLUMN_MULTIPLIER; // bias towards columns
#endif
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
                    if (lower == upper)
                         value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
                    // store square in list
                    if (infeas[iRow])
                         infeas[iRow] = value; // already there
                    else
                         infeasible_->quickAdd(iRow, value);
               } else {
                    // feasible - was it infeasible - if so set tiny
                    if (infeas[iRow])
                         infeas[iRow] = COIN_INDEXED_REALLY_TINY_ELEMENT;
               }
               work[iRow] = 0.0;
          }
     }
     // Do pivot row
     {
          int iRow = model_->pivotRow();
          // feasible - was it infeasible - if so set tiny
          //assert (infeas[iRow]);
          if (infeas[iRow])
               infeas[iRow] = COIN_INDEXED_REALLY_TINY_ELEMENT;
     }
     primalUpdate->setNumElements(0);
     objectiveChange += changeObj;
}
/* Saves any weights round factorization as pivot rows may change
   1) before factorization
   2) after factorization
   3) just redo infeasibilities
   4) restore weights
*/
void
ClpDualRowSteepest::saveWeights(ClpSimplex * model, int mode)
{
     // alternateWeights_ is defined as indexed but is treated oddly
     model_ = model;
     int numberRows = model_->numberRows();
     int numberColumns = model_->numberColumns();
     const int * pivotVariable = model_->pivotVariable();
     int i;
     if (mode == 1) {
          if(weights_) {
               // Check if size has changed
               if (infeasible_->capacity() == numberRows) {
                    alternateWeights_->clear();
                    // change from row numbers to sequence numbers
                    int * which = alternateWeights_->getIndices();
                    for (i = 0; i < numberRows; i++) {
                         int iPivot = pivotVariable[i];
                         which[i] = iPivot;
                    }
                    state_ = 1;
               } else {
                    // size has changed - clear everything
                    delete [] weights_;
                    weights_ = NULL;
                    delete [] dubiousWeights_;
                    dubiousWeights_ = NULL;
                    delete infeasible_;
                    infeasible_ = NULL;
                    delete alternateWeights_;
                    alternateWeights_ = NULL;
                    delete savedWeights_;
                    savedWeights_ = NULL;
                    state_ = -1;
               }
          }
     } else if (mode == 2 || mode == 4 || mode >= 5) {
          // restore
          if (!weights_ || state_ == -1 || mode == 5) {
               // initialize weights
               delete [] weights_;
               delete alternateWeights_;
               weights_ = new double[numberRows];
               alternateWeights_ = new CoinIndexedVector();
               // enough space so can use it for factorization
               alternateWeights_->reserve(numberRows +
                                          model_->factorization()->maximumPivots());
               if (mode_ != 1 || mode == 5) {
                    // initialize to 1.0 (can we do better?)
                    for (i = 0; i < numberRows; i++) {
                         weights_[i] = 1.0;
                    }
               } else {
                    CoinIndexedVector * temp = new CoinIndexedVector();
                    temp->reserve(numberRows +
                                  model_->factorization()->maximumPivots());
                    double * array = alternateWeights_->denseVector();
                    int * which = alternateWeights_->getIndices();
                    for (i = 0; i < numberRows; i++) {
                         double value = 0.0;
                         array[0] = 1.0;
                         which[0] = i;
                         alternateWeights_->setNumElements(1);
                         alternateWeights_->setPackedMode(true);
                         model_->factorization()->updateColumnTranspose(temp,
                                   alternateWeights_);
                         int number = alternateWeights_->getNumElements();
                         int j;
                         for (j = 0; j < number; j++) {
                              value += array[j] * array[j];
                              array[j] = 0.0;
                         }
                         alternateWeights_->setNumElements(0);
                         weights_[i] = value;
                    }
                    delete temp;
               }
               // create saved weights (not really indexedvector)
               savedWeights_ = new CoinIndexedVector();
               savedWeights_->reserve(numberRows);

               double * array = savedWeights_->denseVector();
               int * which = savedWeights_->getIndices();
               for (i = 0; i < numberRows; i++) {
                    array[i] = weights_[i];
                    which[i] = pivotVariable[i];
               }
          } else if (mode != 6) {
               int * which = alternateWeights_->getIndices();
               CoinIndexedVector * rowArray3 = model_->rowArray(3);
               rowArray3->clear();
               int * back = rowArray3->getIndices();
               // In case something went wrong
               for (i = 0; i < numberRows + numberColumns; i++)
                    back[i] = -1;
               if (mode != 4) {
                    // save
                    CoinMemcpyN(which,	numberRows, savedWeights_->getIndices());
                    CoinMemcpyN(weights_,	numberRows, savedWeights_->denseVector());
               } else {
                    // restore
                    //memcpy(which,savedWeights_->getIndices(),
                    //     numberRows*sizeof(int));
                    //memcpy(weights_,savedWeights_->denseVector(),
                    //     numberRows*sizeof(double));
                    which = savedWeights_->getIndices();
               }
               // restore (a bit slow - but only every re-factorization)
               double * array = savedWeights_->denseVector();
               for (i = 0; i < numberRows; i++) {
                    int iSeq = which[i];
                    back[iSeq] = i;
               }
               for (i = 0; i < numberRows; i++) {
                    int iPivot = pivotVariable[i];
                    iPivot = back[iPivot];
                    if (iPivot >= 0) {
                         weights_[i] = array[iPivot];
                         if (weights_[i] < DEVEX_TRY_NORM)
                              weights_[i] = DEVEX_TRY_NORM; // may need to check more
                    } else {
                         // odd
                         weights_[i] = 1.0;
                    }
               }
          } else {
               // mode 6 - scale back weights as primal errors
               double primalError = model_->largestPrimalError();
               double allowed ;
               if (primalError > 1.0e3)
                    allowed = 10.0;
               else if (primalError > 1.0e2)
                    allowed = 50.0;
               else if (primalError > 1.0e1)
                    allowed = 100.0;
               else
                    allowed = 1000.0;
               double allowedInv = 1.0 / allowed;
               for (i = 0; i < numberRows; i++) {
                    double value = weights_[i];
                    if (value < allowedInv)
                         value = allowedInv;
                    else if (value > allowed)
                         value = allowed;
                    weights_[i] = allowed;
               }
          }
          state_ = 0;
          // set up infeasibilities
          if (!infeasible_) {
               infeasible_ = new CoinIndexedVector();
               infeasible_->reserve(numberRows);
          }
     }
     if (mode >= 2) {
          // Get dubious weights
          //if (!dubiousWeights_)
          //dubiousWeights_=new int[numberRows];
          //model_->factorization()->getWeights(dubiousWeights_);
          infeasible_->clear();
          int iRow;
          const int * pivotVariable = model_->pivotVariable();
          double tolerance = model_->currentPrimalTolerance();
          for (iRow = 0; iRow < numberRows; iRow++) {
               int iPivot = pivotVariable[iRow];
               double value = model_->solution(iPivot);
               double lower = model_->lower(iPivot);
               double upper = model_->upper(iPivot);
               if (value < lower - tolerance) {
                    value -= lower;
                    value *= value;
#ifdef CLP_DUAL_COLUMN_MULTIPLIER
                    if (iPivot < numberColumns)
                         value *= CLP_DUAL_COLUMN_MULTIPLIER; // bias towards columns
#endif
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
                    if (lower == upper)
                         value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
                    // store square in list
                    infeasible_->quickAdd(iRow, value);
               } else if (value > upper + tolerance) {
                    value -= upper;
                    value *= value;
#ifdef CLP_DUAL_COLUMN_MULTIPLIER
                    if (iPivot < numberColumns)
                         value *= CLP_DUAL_COLUMN_MULTIPLIER; // bias towards columns
#endif
#ifdef CLP_DUAL_FIXED_COLUMN_MULTIPLIER
                    if (lower == upper)
                         value *= CLP_DUAL_FIXED_COLUMN_MULTIPLIER; // bias towards taking out fixed variables
#endif
                    // store square in list
                    infeasible_->quickAdd(iRow, value);
               }
          }
     }
}
// Gets rid of last update
void
ClpDualRowSteepest::unrollWeights()
{
     double * saved = alternateWeights_->denseVector();
     int number = alternateWeights_->getNumElements();
     int * which = alternateWeights_->getIndices();
     int i;
     if (alternateWeights_->packedMode()) {
          for (i = 0; i < number; i++) {
               int iRow = which[i];
               weights_[iRow] = saved[i];
               saved[i] = 0.0;
          }
     } else {
          for (i = 0; i < number; i++) {
               int iRow = which[i];
               weights_[iRow] = saved[iRow];
               saved[iRow] = 0.0;
          }
     }
     alternateWeights_->setNumElements(0);
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpDualRowPivot * ClpDualRowSteepest::clone(bool CopyData) const
{
     if (CopyData) {
          return new ClpDualRowSteepest(*this);
     } else {
          return new ClpDualRowSteepest();
     }
}
// Gets rid of all arrays
void
ClpDualRowSteepest::clearArrays()
{
     if (persistence_ == normal) {
          delete [] weights_;
          weights_ = NULL;
          delete [] dubiousWeights_;
          dubiousWeights_ = NULL;
          delete infeasible_;
          infeasible_ = NULL;
          delete alternateWeights_;
          alternateWeights_ = NULL;
          delete savedWeights_;
          savedWeights_ = NULL;
     }
     state_ = -1;
}
// Returns true if would not find any row
bool
ClpDualRowSteepest::looksOptimal() const
{
     int iRow;
     const int * pivotVariable = model_->pivotVariable();
     double tolerance = model_->currentPrimalTolerance();
     // we can't really trust infeasibilities if there is primal error
     // this coding has to mimic coding in checkPrimalSolution
     double error = CoinMin(1.0e-2, model_->largestPrimalError());
     // allow tolerance at least slightly bigger than standard
     tolerance = tolerance +  error;
     // But cap
     tolerance = CoinMin(1000.0, tolerance);
     int numberRows = model_->numberRows();
     int numberInfeasible = 0;
     for (iRow = 0; iRow < numberRows; iRow++) {
          int iPivot = pivotVariable[iRow];
          double value = model_->solution(iPivot);
          double lower = model_->lower(iPivot);
          double upper = model_->upper(iPivot);
          if (value < lower - tolerance) {
               numberInfeasible++;
          } else if (value > upper + tolerance) {
               numberInfeasible++;
          }
     }
     return (numberInfeasible == 0);
}
// Called when maximum pivots changes
void
ClpDualRowSteepest::maximumPivotsChanged()
{
     if (alternateWeights_ &&
               alternateWeights_->capacity() != model_->numberRows() +
               model_->factorization()->maximumPivots()) {
          delete alternateWeights_;
          alternateWeights_ = new CoinIndexedVector();
          // enough space so can use it for factorization
          alternateWeights_->reserve(model_->numberRows() +
                                     model_->factorization()->maximumPivots());
     }
}

