/* $Id$ */
// Copyright (C) 2008, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinPragma.hpp"
#include "ClpSimplex.hpp"
#include "ClpNode.hpp"
#include "ClpFactorization.hpp"
#include "ClpDualRowSteepest.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpNode::ClpNode () :
     branchingValue_(0.5),
     objectiveValue_(0.0),
     sumInfeasibilities_(0.0),
     estimatedSolution_(0.0),
     factorization_(NULL),
     weights_(NULL),
     status_(NULL),
     primalSolution_(NULL),
     dualSolution_(NULL),
     lower_(NULL),
     upper_(NULL),
     pivotVariables_(NULL),
     fixed_(NULL),
     sequence_(1),
     numberInfeasibilities_(0),
     depth_(0),
     numberFixed_(0),
     flags_(0),
     maximumFixed_(0),
     maximumRows_(0),
     maximumColumns_(0),
     maximumIntegers_(0)
{
     branchState_.firstBranch = 0;
     branchState_.branch = 0;
}
//-------------------------------------------------------------------
// Useful Constructor from model
//-------------------------------------------------------------------
ClpNode::ClpNode (ClpSimplex * model, const ClpNodeStuff * stuff, int depth) :
     branchingValue_(0.5),
     objectiveValue_(0.0),
     sumInfeasibilities_(0.0),
     estimatedSolution_(0.0),
     factorization_(NULL),
     weights_(NULL),
     status_(NULL),
     primalSolution_(NULL),
     dualSolution_(NULL),
     lower_(NULL),
     upper_(NULL),
     pivotVariables_(NULL),
     fixed_(NULL),
     sequence_(1),
     numberInfeasibilities_(0),
     depth_(0),
     numberFixed_(0),
     flags_(0),
     maximumFixed_(0),
     maximumRows_(0),
     maximumColumns_(0),
     maximumIntegers_(0)
{
     branchState_.firstBranch = 0;
     branchState_.branch = 0;
     gutsOfConstructor(model, stuff, 0, depth);
}

//-------------------------------------------------------------------
// Most of work of constructor from model
//-------------------------------------------------------------------
void
ClpNode::gutsOfConstructor (ClpSimplex * model, const ClpNodeStuff * stuff,
                            int arraysExist, int depth)
{
     int numberRows = model->numberRows();
     int numberColumns = model->numberColumns();
     int numberTotal = numberRows + numberColumns;
     int maximumTotal = maximumRows_ + maximumColumns_;
     depth_ = depth;
     // save stuff
     objectiveValue_ = model->objectiveValue() * model->optimizationDirection();
     estimatedSolution_ = objectiveValue_;
     flags_ = 1; //say scaled
     if (!arraysExist) {
          maximumRows_ = CoinMax(maximumRows_, numberRows);
          maximumColumns_ = CoinMax(maximumColumns_, numberColumns);
          maximumTotal = maximumRows_ + maximumColumns_;
          assert (!factorization_);
          factorization_ = new ClpFactorization(*model->factorization(), numberRows);
          status_ = CoinCopyOfArrayPartial(model->statusArray(), maximumTotal, numberTotal);
          primalSolution_ = CoinCopyOfArrayPartial(model->solutionRegion(), maximumTotal, numberTotal);
          dualSolution_ = CoinCopyOfArrayPartial(model->djRegion(), maximumTotal, numberTotal); //? has duals as well?
          pivotVariables_ = CoinCopyOfArrayPartial(model->pivotVariable(), maximumRows_, numberRows);
          ClpDualRowSteepest* pivot =
               dynamic_cast< ClpDualRowSteepest*>(model->dualRowPivot());
          if (pivot) {
               assert (!weights_);
               weights_ = new ClpDualRowSteepest(*pivot);
          }
     } else {
          if (arraysExist == 2)
               assert(lower_);
          if (numberRows <= maximumRows_ && numberColumns <= maximumColumns_) {
               CoinMemcpyN(model->statusArray(), numberTotal, status_);
               if (arraysExist == 1) {
                    *factorization_ = *model->factorization();
                    CoinMemcpyN(model->solutionRegion(), numberTotal, primalSolution_);
                    CoinMemcpyN(model->djRegion(), numberTotal, dualSolution_); //? has duals as well?
                    ClpDualRowSteepest* pivot =
                         dynamic_cast< ClpDualRowSteepest*>(model->dualRowPivot());
                    if (pivot) {
                         if (weights_) {
                              //if (weights_->numberRows()==pivot->numberRows()) {
                              weights_->fill(*pivot);
                              //} else {
                              //delete weights_;
                              //weights_ = new ClpDualRowSteepest(*pivot);
                              //}
                         } else {
                              weights_ = new ClpDualRowSteepest(*pivot);
                         }
                    }
                    CoinMemcpyN(model->pivotVariable(), numberRows, pivotVariables_);
               } else {
                    CoinMemcpyN(model->primalColumnSolution(), numberColumns, primalSolution_);
                    CoinMemcpyN(model->dualColumnSolution(), numberColumns, dualSolution_);
                    flags_ = 0;
                    CoinMemcpyN(model->dualRowSolution(), numberRows, dualSolution_ + numberColumns);
               }
          } else {
               // size has changed
               maximumRows_ = CoinMax(maximumRows_, numberRows);
               maximumColumns_ = CoinMax(maximumColumns_, numberColumns);
               maximumTotal = maximumRows_ + maximumColumns_;
               delete weights_;
               weights_ = NULL;
               delete [] status_;
               delete [] primalSolution_;
               delete [] dualSolution_;
               delete [] pivotVariables_;
               status_ = CoinCopyOfArrayPartial(model->statusArray(), maximumTotal, numberTotal);
               primalSolution_ = new double [maximumTotal*sizeof(double)];
               dualSolution_ = new double [maximumTotal*sizeof(double)];
               if (arraysExist == 1) {
                    *factorization_ = *model->factorization(); // I think this is OK
                    CoinMemcpyN(model->solutionRegion(), numberTotal, primalSolution_);
                    CoinMemcpyN(model->djRegion(), numberTotal, dualSolution_); //? has duals as well?
                    ClpDualRowSteepest* pivot =
                         dynamic_cast< ClpDualRowSteepest*>(model->dualRowPivot());
                    if (pivot) {
                         assert (!weights_);
                         weights_ = new ClpDualRowSteepest(*pivot);
                    }
               } else {
                    CoinMemcpyN(model->primalColumnSolution(), numberColumns, primalSolution_);
                    CoinMemcpyN(model->dualColumnSolution(), numberColumns, dualSolution_);
                    flags_ = 0;
                    CoinMemcpyN(model->dualRowSolution(), numberRows, dualSolution_ + numberColumns);
               }
               pivotVariables_ = new int [maximumRows_];
               if (model->pivotVariable() && model->numberRows() == numberRows)
                    CoinMemcpyN(model->pivotVariable(), numberRows, pivotVariables_);
               else
                    CoinFillN(pivotVariables_, numberRows, -1);
          }
     }
     numberFixed_ = 0;
     const double * lower = model->columnLower();
     const double * upper = model->columnUpper();
     const double * solution = model->primalColumnSolution();
     const char * integerType = model->integerInformation();
     const double * columnScale = model->columnScale();
     if (!flags_)
          columnScale = NULL; // as duals correct
     int iColumn;
     sequence_ = -1;
     double integerTolerance = stuff->integerTolerance_;
     double mostAway = 0.0;
     int bestPriority = COIN_INT_MAX;
     sumInfeasibilities_ = 0.0;
     numberInfeasibilities_ = 0;
     int nFix = 0;
     double gap = CoinMax(model->dualObjectiveLimit() - objectiveValue_, 1.0e-4);
#define PSEUDO 3
#if PSEUDO==1||PSEUDO==2
     // Column copy of matrix
     ClpPackedMatrix * matrix = model->clpScaledMatrix();
     const double *objective = model->costRegion() ;
     if (!objective) {
          objective = model->objective();
          //if (!matrix)
          matrix = dynamic_cast< ClpPackedMatrix*>(model->clpMatrix());
     } else if (!matrix) {
          matrix = dynamic_cast< ClpPackedMatrix*>(model->clpMatrix());
     }
     const double * element = matrix->getElements();
     const int * row = matrix->getIndices();
     const CoinBigIndex * columnStart = matrix->getVectorStarts();
     const int * columnLength = matrix->getVectorLengths();
     double direction = model->optimizationDirection();
     const double * dual = dualSolution_ + numberColumns;
#if PSEUDO==2
     double * activeWeight = new double [numberRows];
     const double * rowLower = model->rowLower();
     const double * rowUpper = model->rowUpper();
     const double * rowActivity = model->primalRowSolution();
     double tolerance = 1.0e-6;
     for (int iRow = 0; iRow < numberRows; iRow++) {
          // could use pi to see if active or activity
          if (rowActivity[iRow] > rowUpper[iRow] - tolerance
                    || rowActivity[iRow] < rowLower[iRow] + tolerance) {
               activeWeight[iRow] = 0.0;
          } else {
               activeWeight[iRow] = -1.0;
          }
     }
     for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
          if (integerType[iColumn]) {
               double value = solution[iColumn];
               if (fabs(value - floor(value + 0.5)) > 1.0e-6) {
                    CoinBigIndex start = columnStart[iColumn];
                    CoinBigIndex end = start + columnLength[iColumn];
                    for (CoinBigIndex j = start; j < end; j++) {
                         int iRow = row[j];
                         if (activeWeight[iRow] >= 0.0)
                              activeWeight[iRow] += 1.0;
                    }
               }
          }
     }
     for (int iRow = 0; iRow < numberRows; iRow++) {
          if (activeWeight[iRow] > 0.0) {
               // could use pi
               activeWeight[iRow] = 1.0 / activeWeight[iRow];
          } else {
               activeWeight[iRow] = 0.0;
          }
     }
#endif
#endif
     const double * downPseudo = stuff->downPseudo_;
     const int * numberDown = stuff->numberDown_;
     const int * numberDownInfeasible = stuff->numberDownInfeasible_;
     const double * upPseudo = stuff->upPseudo_;
     const int * priority = stuff->priority_;
     const int * numberUp = stuff->numberUp_;
     const int * numberUpInfeasible = stuff->numberUpInfeasible_;
     int numberBeforeTrust = stuff->numberBeforeTrust_;
     int stateOfSearch = stuff->stateOfSearch_;
     int iInteger = 0;
     // weight at 1.0 is max min (CbcBranch was 0.8,0.1) (ClpNode was 0.9,0.9)
#define WEIGHT_AFTER 0.9
#define WEIGHT_BEFORE 0.2
     //Stolen from Constraint Integer Programming book (with epsilon change)
#define WEIGHT_PRODUCT
#ifdef WEIGHT_PRODUCT
     double smallChange = stuff->smallChange_;
#endif
#ifndef INFEAS_MULTIPLIER 
#define INFEAS_MULTIPLIER 1.0
#endif
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          if (integerType[iColumn]) {
               double value = solution[iColumn];
               value = CoinMax(value, static_cast<double> (lower[iColumn]));
               value = CoinMin(value, static_cast<double> (upper[iColumn]));
               double nearest = floor(value + 0.5);
               if (fabs(value - nearest) > integerTolerance) {
                    numberInfeasibilities_++;
                    sumInfeasibilities_ += fabs(value - nearest);
#if PSEUDO==1 || PSEUDO ==2
                    double upValue = 0.0;
                    double downValue = 0.0;
                    double value2 = direction * objective[iColumn];
                    //double dj2=value2;
                    if (value2) {
                         if (value2 > 0.0)
                              upValue += 1.5 * value2;
                         else
                              downValue -= 1.5 * value2;
                    }
                    CoinBigIndex start = columnStart[iColumn];
                    CoinBigIndex end = columnStart[iColumn] + columnLength[iColumn];
                    for (CoinBigIndex j = start; j < end; j++) {
                         int iRow = row[j];
                         value2 = -dual[iRow];
                         if (value2) {
                              value2 *= element[j];
                              //dj2 += value2;
#if PSEUDO==2
                              assert (activeWeight[iRow] > 0.0 || fabs(dual[iRow]) < 1.0e-6);
                              value2 *= activeWeight[iRow];
#endif
                              if (value2 > 0.0)
                                   upValue += value2;
                              else
                                   downValue -= value2;
                         }
                    }
                    //assert (fabs(dj2)<1.0e-4);
                    int nUp = numberUp[iInteger];
                    double upValue2 = (upPseudo[iInteger] / (1.0 + nUp));
                    // Extra for infeasible branches
                    if (nUp) {
                         double ratio = 1.0 + INFEAS_MULTIPLIER*static_cast<double>(numberUpInfeasible[iInteger]) /
                                        static_cast<double>(nUp);
                         upValue2 *= ratio;
                    }
                    int nDown = numberDown[iInteger];
                    double downValue2 = (downPseudo[iInteger] / (1.0 + nDown));
                    if (nDown) {
                         double ratio = 1.0 + INFEAS_MULTIPLIER*static_cast<double>(numberDownInfeasible[iInteger]) /
                                        static_cast<double>(nDown);
                         downValue2 *= ratio;
                    }
                    //printf("col %d - downPi %g up %g, downPs %g up %g\n",
                    //     iColumn,upValue,downValue,upValue2,downValue2);
                    upValue = CoinMax(0.1 * upValue, upValue2);
                    downValue = CoinMax(0.1 * downValue, downValue2);
                    //upValue = CoinMax(upValue,1.0e-8);
                    //downValue = CoinMax(downValue,1.0e-8);
                    upValue *= ceil(value) - value;
                    downValue *= value - floor(value);
                    double infeasibility;
                    //if (depth>1000)
                    //infeasibility = CoinMax(upValue,downValue)+integerTolerance;
                    //else
                    if (stateOfSearch <= 2) {
                         // no solution
                         infeasibility = (1.0 - WEIGHT_BEFORE) * CoinMax(upValue, downValue) +
                                         WEIGHT_BEFORE * CoinMin(upValue, downValue) + integerTolerance;
                    } else {
#ifndef WEIGHT_PRODUCT
                         infeasibility = (1.0 - WEIGHT_AFTER) * CoinMax(upValue, downValue) +
                                         WEIGHT_AFTER * CoinMin(upValue, downValue) + integerTolerance;
#else
                         infeasibility = CoinMax(CoinMax(upValue, downValue), smallChange) *
                                         CoinMax(CoinMin(upValue, downValue), smallChange);
#endif
                    }
                    estimatedSolution_ += CoinMin(upValue2, downValue2);
#elif PSEUDO==3
                    int nUp = numberUp[iInteger];
                    int nDown = numberDown[iInteger];
                    // Extra 100% for infeasible branches
                    double upValue = (ceil(value) - value) * (upPseudo[iInteger] /
                                     (1.0 + nUp));
                    if (nUp) {
                         double ratio = 1.0 + INFEAS_MULTIPLIER*static_cast<double>(numberUpInfeasible[iInteger]) /
                                        static_cast<double>(nUp);
                         upValue *= ratio;
                    }
                    double downValue = (value - floor(value)) * (downPseudo[iInteger] /
                                       (1.0 + nDown));
                    if (nDown) {
                         double ratio = 1.0 + INFEAS_MULTIPLIER*static_cast<double>(numberDownInfeasible[iInteger]) /
                                        static_cast<double>(nDown);
                         downValue *= ratio;
                    }
                    if (nUp < numberBeforeTrust || nDown < numberBeforeTrust) {
                         upValue *= 10.0;
                         downValue *= 10.0;
                    }

                    double infeasibility;
                    //if (depth>1000)
                    //infeasibility = CoinMax(upValue,downValue)+integerTolerance;
                    //else
                    if (stateOfSearch <= 2) {
                         // no solution
                         infeasibility = (1.0 - WEIGHT_BEFORE) * CoinMax(upValue, downValue) +
                                         WEIGHT_BEFORE * CoinMin(upValue, downValue) + integerTolerance;
                    } else {
#ifndef WEIGHT_PRODUCT
                         infeasibility = (1.0 - WEIGHT_AFTER) * CoinMax(upValue, downValue) +
                                         WEIGHT_AFTER * CoinMin(upValue, downValue) + integerTolerance;
#else
                    infeasibility = CoinMax(CoinMax(upValue, downValue), smallChange) *
                                    CoinMax(CoinMin(upValue, downValue), smallChange);
                    //infeasibility += CoinMin(upValue,downValue)*smallChange;
#endif
                    }
                    //infeasibility = 0.1*CoinMax(upValue,downValue)+
                    //0.9*CoinMin(upValue,downValue) + integerTolerance;
                    estimatedSolution_ += CoinMin(upValue, downValue);
#else
                    double infeasibility = fabs(value - nearest);
#endif
                    assert (infeasibility > 0.0);
                    if (priority[iInteger] < bestPriority) {
                         mostAway = 0.0;
                         bestPriority = priority[iInteger];
                    } else if (priority[iInteger] > bestPriority) {
                         infeasibility = 0.0;
                    }
                    if (infeasibility > mostAway) {
                         mostAway = infeasibility;
                         sequence_ = iColumn;
                         branchingValue_ = value;
                         branchState_.branch = 0;
#if PSEUDO>0
                         if (upValue <= downValue)
                              branchState_.firstBranch = 1; // up
                         else
                              branchState_.firstBranch = 0; // down
#else
                         if (value <= nearest)
                              branchState_.firstBranch = 1; // up
                         else
                              branchState_.firstBranch = 0; // down
#endif
                    }
               } else if (model->getColumnStatus(iColumn) == ClpSimplex::atLowerBound) {
                    bool fix = false;
                    if (columnScale) {
                         if (dualSolution_[iColumn] > gap * columnScale[iColumn])
                              fix = true;
                    } else {
                         if (dualSolution_[iColumn] > gap)
                              fix = true;
                    }
                    if (fix) {
                         nFix++;
                         //printf("fixed %d to zero gap %g dj %g %g\n",iColumn,
                         // gap,dualSolution_[iColumn], columnScale ? columnScale[iColumn]:1.0);
                         model->setColumnStatus(iColumn, ClpSimplex::isFixed);
                    }
               } else if (model->getColumnStatus(iColumn) == ClpSimplex::atUpperBound) {
                    bool fix = false;
                    if (columnScale) {
                         if (-dualSolution_[iColumn] > gap * columnScale[iColumn])
                              fix = true;
                    } else {
                         if (-dualSolution_[iColumn] > gap)
                              fix = true;
                    }
                    if (fix) {
                         nFix++;
                         //printf("fixed %d to one gap %g dj %g %g\n",iColumn,
                         // gap,dualSolution_[iColumn], columnScale ? columnScale[iColumn]:1.0);
                         model->setColumnStatus(iColumn, ClpSimplex::isFixed);
                    }
               }
               iInteger++;
          }
     }
     //printf("Choosing %d inf %g pri %d\n",
     // sequence_,mostAway,bestPriority);
#if PSEUDO == 2
     delete [] activeWeight;
#endif
     if (lower_) {
          // save bounds
          if (iInteger > maximumIntegers_) {
               delete [] lower_;
               delete [] upper_;
               maximumIntegers_ = iInteger;
               lower_ = new int [maximumIntegers_];
               upper_ = new int [maximumIntegers_];
          }
          iInteger = 0;
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               if (integerType[iColumn]) {
                    lower_[iInteger] = static_cast<int> (lower[iColumn]);
                    upper_[iInteger] = static_cast<int> (upper[iColumn]);
                    iInteger++;
               }
          }
     }
     // Could omit save of fixed if doing full save of bounds
     if (sequence_ >= 0 && nFix) {
          if (nFix > maximumFixed_) {
               delete [] fixed_;
               fixed_ = new int [nFix];
               maximumFixed_ = nFix;
          }
          numberFixed_ = 0;
          unsigned char * status = model->statusArray();
          for (iColumn = 0; iColumn < numberColumns; iColumn++) {
               if (status[iColumn] != status_[iColumn]) {
                    if (solution[iColumn] <= lower[iColumn] + 2.0 * integerTolerance) {
                         model->setColumnUpper(iColumn, lower[iColumn]);
                         fixed_[numberFixed_++] = iColumn;
                    } else {
                         assert (solution[iColumn] >= upper[iColumn] - 2.0 * integerTolerance);
                         model->setColumnLower(iColumn, upper[iColumn]);
                         fixed_[numberFixed_++] = iColumn | 0x10000000;
                    }
               }
          }
          //printf("%d fixed\n",numberFixed_);
     }
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpNode::ClpNode (const ClpNode & )
{
     printf("ClpNode copy not implemented\n");
     abort();
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpNode::~ClpNode ()
{
     delete factorization_;
     delete weights_;
     delete [] status_;
     delete [] primalSolution_;
     delete [] dualSolution_;
     delete [] lower_;
     delete [] upper_;
     delete [] pivotVariables_;
     delete [] fixed_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpNode &
ClpNode::operator=(const ClpNode& rhs)
{
     if (this != &rhs) {
          printf("ClpNode = not implemented\n");
          abort();
     }
     return *this;
}
// Create odd arrays
void
ClpNode::createArrays(ClpSimplex * model)
{
     int numberColumns = model->numberColumns();
     const char * integerType = model->integerInformation();
     int iColumn;
     int numberIntegers = 0;
     for (iColumn = 0; iColumn < numberColumns; iColumn++) {
          if (integerType[iColumn])
               numberIntegers++;
     }
     if (numberIntegers > maximumIntegers_ || !lower_) {
          delete [] lower_;
          delete [] upper_;
          maximumIntegers_ = numberIntegers;
          lower_ = new int [numberIntegers];
          upper_ = new int [numberIntegers];
     }
}
// Clean up as crunch is different model
void
ClpNode::cleanUpForCrunch()
{
     delete weights_;
     weights_ = NULL;
}
/* Applies node to model
   0 - just tree bounds
   1 - tree bounds and basis etc
   2 - saved bounds and basis etc
*/
void
ClpNode::applyNode(ClpSimplex * model, int doBoundsEtc )
{
     int numberColumns = model->numberColumns();
     const double * lower = model->columnLower();
     const double * upper = model->columnUpper();
     if (doBoundsEtc < 2) {
          // current bound
          int way = branchState_.firstBranch;
          if (branchState_.branch > 0)
               way = 1 - way;
          if (!way) {
               // This should also do underlying internal bound
               model->setColumnUpper(sequence_, floor(branchingValue_));
          } else {
               // This should also do underlying internal bound
               model->setColumnLower(sequence_, ceil(branchingValue_));
          }
          // apply dj fixings
          for (int i = 0; i < numberFixed_; i++) {
               int iColumn = fixed_[i];
               if ((iColumn & 0x10000000) != 0) {
                    iColumn &= 0xfffffff;
                    model->setColumnLower(iColumn, upper[iColumn]);
               } else {
                    model->setColumnUpper(iColumn, lower[iColumn]);
               }
          }
     } else {
          // restore bounds
          assert (lower_);
          int iInteger = -1;
          const char * integerType = model->integerInformation();
          for (int iColumn = 0; iColumn < numberColumns; iColumn++) {
               if (integerType[iColumn]) {
                    iInteger++;
                    if (lower_[iInteger] != static_cast<int> (lower[iColumn]))
                         model->setColumnLower(iColumn, lower_[iInteger]);
                    if (upper_[iInteger] != static_cast<int> (upper[iColumn]))
                         model->setColumnUpper(iColumn, upper_[iInteger]);
               }
          }
     }
     if (doBoundsEtc && doBoundsEtc < 3) {
          //model->copyFactorization(*factorization_);
          model->copyFactorization(*factorization_);
          ClpDualRowSteepest* pivot =
               dynamic_cast< ClpDualRowSteepest*>(model->dualRowPivot());
          if (pivot && weights_) {
               pivot->fill(*weights_);
          }
          int numberRows = model->numberRows();
          int numberTotal = numberRows + numberColumns;
          CoinMemcpyN(status_, numberTotal, model->statusArray());
          if (doBoundsEtc < 2) {
               CoinMemcpyN(primalSolution_, numberTotal, model->solutionRegion());
               CoinMemcpyN(dualSolution_, numberTotal, model->djRegion());
               CoinMemcpyN(pivotVariables_, numberRows, model->pivotVariable());
               CoinMemcpyN(dualSolution_ + numberColumns, numberRows, model->dualRowSolution());
          } else {
               CoinMemcpyN(primalSolution_, numberColumns, model->primalColumnSolution());
               CoinMemcpyN(dualSolution_, numberColumns, model->dualColumnSolution());
               CoinMemcpyN(dualSolution_ + numberColumns, numberRows, model->dualRowSolution());
               if (model->columnScale()) {
                    // See if just primal will work
                    double * solution = model->primalColumnSolution();
                    const double * columnScale = model->columnScale();
                    int i;
                    for (i = 0; i < numberColumns; i++) {
                         solution[i] *= columnScale[i];
                    }
               }
          }
          model->setObjectiveValue(objectiveValue_);
     }
}
// Choose a new variable
void
ClpNode::chooseVariable(ClpSimplex * , ClpNodeStuff * /*info*/)
{
#if 0
     int way = branchState_.firstBranch;
     if (branchState_.branch > 0)
          way = 1 - way;
     assert (!branchState_.branch);
     // We need to use pseudo costs to choose a variable
     int numberColumns = model->numberColumns();
#endif
}
// Fix on reduced costs
int
ClpNode::fixOnReducedCosts(ClpSimplex * )
{

     return 0;
}
/* Way for integer variable -1 down , +1 up */
int
ClpNode::way() const
{
     int way = branchState_.firstBranch;
     if (branchState_.branch > 0)
          way = 1 - way;
     return way == 0 ? -1 : +1;
}
// Return true if branch exhausted
bool
ClpNode::fathomed() const
{
     return branchState_.branch >= 1
            ;
}
// Change state of variable i.e. go other way
void
ClpNode::changeState()
{
     branchState_.branch++;
     assert (branchState_.branch <= 2);
}
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpNodeStuff::ClpNodeStuff () :
     integerTolerance_(1.0e-7),
     integerIncrement_(1.0e-8),
     smallChange_(1.0e-8),
     downPseudo_(NULL),
     upPseudo_(NULL),
     priority_(NULL),
     numberDown_(NULL),
     numberUp_(NULL),
     numberDownInfeasible_(NULL),
     numberUpInfeasible_(NULL),
     saveCosts_(NULL),
     nodeInfo_(NULL),
     large_(NULL),
     whichRow_(NULL),
     whichColumn_(NULL),
#ifndef NO_FATHOM_PRINT
     handler_(NULL),
#endif
     nBound_(0),
     saveOptions_(0),
     solverOptions_(0),
     maximumNodes_(0),
     numberBeforeTrust_(0),
     stateOfSearch_(0),
     nDepth_(-1),
     nNodes_(0),
     numberNodesExplored_(0),
     numberIterations_(0),
     presolveType_(0)
#ifndef NO_FATHOM_PRINT
     ,startingDepth_(-1),
     nodeCalled_(-1)
#endif
{

}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpNodeStuff::ClpNodeStuff (const ClpNodeStuff & rhs)
     : integerTolerance_(rhs.integerTolerance_),
       integerIncrement_(rhs.integerIncrement_),
       smallChange_(rhs.smallChange_),
       downPseudo_(NULL),
       upPseudo_(NULL),
       priority_(NULL),
       numberDown_(NULL),
       numberUp_(NULL),
       numberDownInfeasible_(NULL),
       numberUpInfeasible_(NULL),
       saveCosts_(NULL),
       nodeInfo_(NULL),
       large_(NULL),
       whichRow_(NULL),
       whichColumn_(NULL),
#ifndef NO_FATHOM_PRINT
       handler_(rhs.handler_),
#endif
       nBound_(0),
       saveOptions_(rhs.saveOptions_),
       solverOptions_(rhs.solverOptions_),
       maximumNodes_(rhs.maximumNodes_),
       numberBeforeTrust_(rhs.numberBeforeTrust_),
       stateOfSearch_(rhs.stateOfSearch_),
       nDepth_(rhs.nDepth_),
       nNodes_(rhs.nNodes_),
       numberNodesExplored_(rhs.numberNodesExplored_),
       numberIterations_(rhs.numberIterations_),
       presolveType_(rhs.presolveType_)
#ifndef NO_FATHOM_PRINT
     ,startingDepth_(rhs.startingDepth_),
       nodeCalled_(rhs.nodeCalled_)
#endif
{
}
//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpNodeStuff &
ClpNodeStuff::operator=(const ClpNodeStuff& rhs)
{
     if (this != &rhs) {
          integerTolerance_ = rhs.integerTolerance_;
          integerIncrement_ = rhs.integerIncrement_;
          smallChange_ = rhs.smallChange_;
          downPseudo_ = NULL;
          upPseudo_ = NULL;
          priority_ = NULL;
          numberDown_ = NULL;
          numberUp_ = NULL;
          numberDownInfeasible_ = NULL;
          numberUpInfeasible_ = NULL;
          saveCosts_ = NULL;
          nodeInfo_ = NULL;
          large_ = NULL;
          whichRow_ = NULL;
          whichColumn_ = NULL;
          nBound_ = 0;
          saveOptions_ = rhs.saveOptions_;
          solverOptions_ = rhs.solverOptions_;
          maximumNodes_ = rhs.maximumNodes_;
          numberBeforeTrust_ = rhs.numberBeforeTrust_;
          stateOfSearch_ = rhs.stateOfSearch_;
          int n = maximumNodes();
          if (n) {
               for (int i = 0; i < n; i++)
                    delete nodeInfo_[i];
          }
          delete [] nodeInfo_;
          nodeInfo_ = NULL;
          nDepth_ = rhs.nDepth_;
          nNodes_ = rhs.nNodes_;
          numberNodesExplored_ = rhs.numberNodesExplored_;
          numberIterations_ = rhs.numberIterations_;
          presolveType_ = rhs.presolveType_;
#ifndef NO_FATHOM_PRINT
	  handler_ = rhs.handler_;
	  startingDepth_ = rhs.startingDepth_;
	  nodeCalled_ = rhs.nodeCalled_;
#endif
     }
     return *this;
}
// Zaps stuff 1 - arrays, 2 ints, 3 both
void
ClpNodeStuff::zap(int type)
{
     if ((type & 1) != 0) {
          downPseudo_ = NULL;
          upPseudo_ = NULL;
          priority_ = NULL;
          numberDown_ = NULL;
          numberUp_ = NULL;
          numberDownInfeasible_ = NULL;
          numberUpInfeasible_ = NULL;
          saveCosts_ = NULL;
          nodeInfo_ = NULL;
          large_ = NULL;
          whichRow_ = NULL;
          whichColumn_ = NULL;
     }
     if ((type & 2) != 0) {
          nBound_ = 0;
          saveOptions_ = 0;
          solverOptions_ = 0;
          maximumNodes_ = 0;
          numberBeforeTrust_ = 0;
          stateOfSearch_ = 0;
          nDepth_ = -1;
          nNodes_ = 0;
          presolveType_ = 0;
          numberNodesExplored_ = 0;
          numberIterations_ = 0;
     }
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpNodeStuff::~ClpNodeStuff ()
{
     delete [] downPseudo_;
     delete [] upPseudo_;
     delete [] priority_;
     delete [] numberDown_;
     delete [] numberUp_;
     delete [] numberDownInfeasible_;
     delete [] numberUpInfeasible_;
     int n = maximumNodes();
     if (n) {
          for (int i = 0; i < n; i++)
               delete nodeInfo_[i];
     }
     delete [] nodeInfo_;
#ifdef CLP_INVESTIGATE
     // Should be NULL - find out why not?
     assert (!saveCosts_);
#endif
     delete [] saveCosts_;
}
// Return maximum number of nodes
int
ClpNodeStuff::maximumNodes() const
{
     int n = 0;
#if 0
     if (nDepth_ != -1) {
          if ((solverOptions_ & 32) == 0)
               n = (1 << nDepth_);
          else if (nDepth_)
               n = 1;
     }
     assert (n == maximumNodes_ - (1 + nDepth_) || n == 0);
#else
     if (nDepth_ != -1) {
          n = maximumNodes_ - (1 + nDepth_);
          assert (n > 0);
     }
#endif
     return n;
}
// Return maximum space for nodes
int
ClpNodeStuff::maximumSpace() const
{
     return maximumNodes_;
}
/* Fill with pseudocosts */
void
ClpNodeStuff::fillPseudoCosts(const double * down, const double * up,
                              const int * priority,
                              const int * numberDown, const int * numberUp,
                              const int * numberDownInfeasible,
                              const int * numberUpInfeasible,
                              int number)
{
     delete [] downPseudo_;
     delete [] upPseudo_;
     delete [] priority_;
     delete [] numberDown_;
     delete [] numberUp_;
     delete [] numberDownInfeasible_;
     delete [] numberUpInfeasible_;
     downPseudo_ = CoinCopyOfArray(down, number);
     upPseudo_ = CoinCopyOfArray(up, number);
     priority_ = CoinCopyOfArray(priority, number);
     numberDown_ = CoinCopyOfArray(numberDown, number);
     numberUp_ = CoinCopyOfArray(numberUp, number);
     numberDownInfeasible_ = CoinCopyOfArray(numberDownInfeasible, number);
     numberUpInfeasible_ = CoinCopyOfArray(numberUpInfeasible, number);
     // scale
     for (int i = 0; i < number; i++) {
          int n;
          n = numberDown_[i];
          if (n)
               downPseudo_[i] *= n;
          n = numberUp_[i];
          if (n)
               upPseudo_[i] *= n;
     }
}
// Update pseudo costs
void
ClpNodeStuff::update(int way, int sequence, double change, bool feasible)
{
     assert (numberDown_[sequence] >= numberDownInfeasible_[sequence]);
     assert (numberUp_[sequence] >= numberUpInfeasible_[sequence]);
     if (way < 0) {
          numberDown_[sequence]++;
          if (!feasible)
               numberDownInfeasible_[sequence]++;
          downPseudo_[sequence] += CoinMax(change, 1.0e-12);
     } else {
          numberUp_[sequence]++;
          if (!feasible)
               numberUpInfeasible_[sequence]++;
          upPseudo_[sequence] += CoinMax(change, 1.0e-12);
     }
}
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
ClpHashValue::ClpHashValue () :
     hash_(NULL),
     numberHash_(0),
     maxHash_(0),
     lastUsed_(-1)
{
}
//-------------------------------------------------------------------
// Useful Constructor from model
//-------------------------------------------------------------------
ClpHashValue::ClpHashValue (ClpSimplex * model) :
     hash_(NULL),
     numberHash_(0),
     maxHash_(0),
     lastUsed_(-1)
{
     maxHash_ = 1000;
     int numberColumns = model->numberColumns();
     const double * columnLower = model->columnLower();
     const double * columnUpper = model->columnUpper();
     int numberRows = model->numberRows();
     const double * rowLower = model->rowLower();
     const double * rowUpper = model->rowUpper();
     const double * objective = model->objective();
     CoinPackedMatrix * matrix = model->matrix();
     const int * columnLength = matrix->getVectorLengths();
     const CoinBigIndex * columnStart = matrix->getVectorStarts();
     const double * elementByColumn = matrix->getElements();
     int i;
     int ipos;

     hash_ = new CoinHashLink[maxHash_];

     for ( i = 0; i < maxHash_; i++ ) {
          hash_[i].value = -1.0e-100;
          hash_[i].index = -1;
          hash_[i].next = -1;
     }
     // Put in +0
     hash_[0].value = 0.0;
     hash_[0].index = 0;
     numberHash_ = 1;
     /*
      * Initialize the hash table.  Only the index of the first value that
      * hashes to a value is entered in the table; subsequent values that
      * collide with it are not entered.
      */
     for ( i = 0; i < numberColumns; i++ ) {
          int length = columnLength[i];
          CoinBigIndex start = columnStart[i];
          for (CoinBigIndex i = start; i < start + length; i++) {
               double value = elementByColumn[i];
               ipos = hash ( value);
               if ( hash_[ipos].index == -1 ) {
                    hash_[ipos].index = numberHash_;
                    numberHash_++;
                    hash_[ipos].value = elementByColumn[i];
               }
          }
     }

     /*
      * Now take care of the values that collided in the preceding loop,
      * Also do other stuff
      */
     for ( i = 0; i < numberRows; i++ ) {
          if (numberHash_ * 2 > maxHash_)
               resize(true);
          double value;
          value = rowLower[i];
          ipos = index(value);
          if (ipos < 0)
               addValue(value);
          value = rowUpper[i];
          ipos = index(value);
          if (ipos < 0)
               addValue(value);
     }
     for ( i = 0; i < numberColumns; i++ ) {
          int length = columnLength[i];
          CoinBigIndex start = columnStart[i];
          if (numberHash_ * 2 > maxHash_)
               resize(true);
          double value;
          value = objective[i];
          ipos = index(value);
          if (ipos < 0)
               addValue(value);
          value = columnLower[i];
          ipos = index(value);
          if (ipos < 0)
               addValue(value);
          value = columnUpper[i];
          ipos = index(value);
          if (ipos < 0)
               addValue(value);
          for (CoinBigIndex j = start; j < start + length; j++) {
               if (numberHash_ * 2 > maxHash_)
                    resize(true);
               value = elementByColumn[j];
               ipos = index(value);
               if (ipos < 0)
                    addValue(value);
          }
     }
     resize(false);
}

//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
ClpHashValue::ClpHashValue (const ClpHashValue & rhs) :
     hash_(NULL),
     numberHash_(rhs.numberHash_),
     maxHash_(rhs.maxHash_),
     lastUsed_(rhs.lastUsed_)
{
     if (maxHash_) {
          CoinHashLink * newHash = new CoinHashLink[maxHash_];
          int i;
          for ( i = 0; i < maxHash_; i++ ) {
               newHash[i].value = rhs.hash_[i].value;
               newHash[i].index = rhs.hash_[i].index;
               newHash[i].next = rhs.hash_[i].next;
          }
     }
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
ClpHashValue::~ClpHashValue ()
{
     delete [] hash_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
ClpHashValue &
ClpHashValue::operator=(const ClpHashValue& rhs)
{
     if (this != &rhs) {
          numberHash_ = rhs.numberHash_;
          maxHash_ = rhs.maxHash_;
          lastUsed_ = rhs.lastUsed_;
          delete [] hash_;
          if (maxHash_) {
               CoinHashLink * newHash = new CoinHashLink[maxHash_];
               int i;
               for ( i = 0; i < maxHash_; i++ ) {
                    newHash[i].value = rhs.hash_[i].value;
                    newHash[i].index = rhs.hash_[i].index;
                    newHash[i].next = rhs.hash_[i].next;
               }
          } else {
               hash_ = NULL;
          }
     }
     return *this;
}
// Return index or -1 if not found
int
ClpHashValue::index(double value) const
{
     if (!value)
          return 0;
     int ipos = hash ( value);
     int returnCode = -1;
     while ( hash_[ipos].index >= 0 ) {
          if (value == hash_[ipos].value) {
               returnCode = hash_[ipos].index;
               break;
          } else {
               int k = hash_[ipos].next;
               if ( k == -1 ) {
                    break;
               } else {
                    ipos = k;
               }
          }
     }
     return returnCode;
}
// Add value to list and return index
int
ClpHashValue::addValue(double value)
{
     int ipos = hash ( value);

     assert (value != hash_[ipos].value);
     if (hash_[ipos].index == -1) {
          // can put in here
          hash_[ipos].index = numberHash_;
          numberHash_++;
          hash_[ipos].value = value;
          return numberHash_ - 1;
     }
     int k = hash_[ipos].next;
     while (k != -1) {
          ipos = k;
          k = hash_[ipos].next;
     }
     while ( true ) {
          ++lastUsed_;
          assert (lastUsed_ <= maxHash_);
          if ( hash_[lastUsed_].index == -1 ) {
               break;
          }
     }
     hash_[ipos].next = lastUsed_;
     hash_[lastUsed_].index = numberHash_;
     numberHash_++;
     hash_[lastUsed_].value = value;
     return numberHash_ - 1;
}

namespace {
  /*
    Originally a local static variable in ClpHashValue::hash.
	Local static variables are a problem when building DLLs on Windows, but
	file-local constants seem to be ok.  -- lh, 101016 --
  */
  const int mmult_for_hash[] = {
          262139, 259459, 256889, 254291, 251701, 249133, 246709, 244247,
          241667, 239179, 236609, 233983, 231289, 228859, 226357, 223829,
          221281, 218849, 216319, 213721, 211093, 208673, 206263, 203773,
          201233, 198637, 196159, 193603, 191161, 188701, 186149, 183761,
          181303, 178873, 176389, 173897, 171469, 169049, 166471, 163871,
          161387, 158941, 156437, 153949, 151531, 149159, 146749, 144299,
          141709, 139369, 136889, 134591, 132169, 129641, 127343, 124853,
          122477, 120163, 117757, 115361, 112979, 110567, 108179, 105727,
          103387, 101021, 98639, 96179, 93911, 91583, 89317, 86939, 84521,
          82183, 79939, 77587, 75307, 72959, 70793, 68447, 66103
     };
}
int
ClpHashValue::hash ( double value) const
{
     
     union {
          double d;
          char c[8];
     } v1;
     assert (sizeof(double) == 8);
     v1.d = value;
     int n = 0;
     int j;

     for ( j = 0; j < 8; ++j ) {
          int ichar = v1.c[j];
          n += mmult_for_hash[j] * ichar;
     }
     return ( abs ( n ) % maxHash_ );	/* integer abs */
}
void
ClpHashValue::resize(bool increaseMax)
{
     int newSize = increaseMax ? ((3 * maxHash_) >> 1) + 1000 : maxHash_;
     CoinHashLink * newHash = new CoinHashLink[newSize];
     int i;
     for ( i = 0; i < newSize; i++ ) {
          newHash[i].value = -1.0e-100;
          newHash[i].index = -1;
          newHash[i].next = -1;
     }
     // swap
     CoinHashLink * oldHash = hash_;
     hash_ = newHash;
     int oldSize = maxHash_;
     maxHash_ = newSize;
     /*
      * Initialize the hash table.  Only the index of the first value that
      * hashes to a value is entered in the table; subsequent values that
      * collide with it are not entered.
      */
     int ipos;
     int n = 0;
     for ( i = 0; i < oldSize; i++ ) {
          if (oldHash[i].index >= 0) {
               ipos = hash ( oldHash[i].value);
               if ( hash_[ipos].index == -1 ) {
                    hash_[ipos].index = n;
                    n++;
                    hash_[ipos].value = oldHash[i].value;
                    // unmark
                    oldHash[i].index = -1;
               }
          }
     }
     /*
      * Now take care of the values that collided in the preceding loop,
      * by finding some other entry in the table for them.
      * Since there are as many entries in the table as there are values,
      * there must be room for them.
      */
     lastUsed_ = -1;
     for ( i = 0; i < oldSize; ++i ) {
          if (oldHash[i].index >= 0) {
               double value = oldHash[i].value;
               ipos = hash ( value);
               int k;
               while ( true ) {
                    assert (value != hash_[ipos].value);
                    k = hash_[ipos].next;
                    if ( k == -1 ) {
                         while ( true ) {
                              ++lastUsed_;
                              assert (lastUsed_ <= maxHash_);
                              if ( hash_[lastUsed_].index == -1 ) {
                                   break;
                              }
                         }
                         hash_[ipos].next = lastUsed_;
                         hash_[lastUsed_].index = n;
                         n++;
                         hash_[lastUsed_].value = value;
                         break;
                    } else {
                         ipos = k;
                    }
               }
          }
     }
     assert (n == numberHash_);
     delete [] oldHash;
}
