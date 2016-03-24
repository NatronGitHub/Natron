/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpPresolve_H
#define ClpPresolve_H
#include "ClpSimplex.hpp"

class CoinPresolveAction;
#include "CoinPresolveMatrix.hpp"
/** This is the Clp interface to CoinPresolve

*/
class ClpPresolve {
public:
     /**@name Main Constructor, destructor */
     //@{
     /// Default constructor
     ClpPresolve();

     /// Virtual destructor
     virtual ~ClpPresolve();
     //@}
     /**@name presolve - presolves a model, transforming the model
      * and saving information in the ClpPresolve object needed for postsolving.
      * This underlying (protected) method is virtual; the idea is that in the future,
      * one could override this method to customize how the various
      * presolve techniques are applied.

      This version of presolve returns a pointer to a new presolved
         model.  NULL if infeasible or unbounded.
         This should be paired with postsolve
         below.  The advantage of going back to original model is that it
         will be exactly as it was i.e. 0.0 will not become 1.0e-19.
         If keepIntegers is true then bounds may be tightened in
         original.  Bounds will be moved by up to feasibilityTolerance
         to try and stay feasible.
         Names will be dropped in presolved model if asked
     */
     ClpSimplex * presolvedModel(ClpSimplex & si,
                                 double feasibilityTolerance = 0.0,
                                 bool keepIntegers = true,
                                 int numberPasses = 5,
                                 bool dropNames = false,
                                 bool doRowObjective = false,
				 const char * prohibitedRows=NULL,
				 const char * prohibitedColumns=NULL);
#ifndef CLP_NO_STD
     /** This version saves data in a file.  The passed in model
         is updated to be presolved model.  
         Returns non-zero if infeasible*/
     int presolvedModelToFile(ClpSimplex &si, std::string fileName,
                              double feasibilityTolerance = 0.0,
                              bool keepIntegers = true,
                              int numberPasses = 5,
			      bool dropNames = false,
                              bool doRowObjective = false);
#endif
     /** Return pointer to presolved model,
         Up to user to destroy */
     ClpSimplex * model() const;
     /// Return pointer to original model
     ClpSimplex * originalModel() const;
     /// Set pointer to original model
     void setOriginalModel(ClpSimplex * model);

     /// return pointer to original columns
     const int * originalColumns() const;
     /// return pointer to original rows
     const int * originalRows() const;
     /** "Magic" number. If this is non-zero then any elements with this value
         may change and so presolve is very limited in what can be done
         to the row and column.  This is for non-linear problems.
     */
     inline void setNonLinearValue(double value) {
          nonLinearValue_ = value;
     }
     inline double nonLinearValue() const {
          return nonLinearValue_;
     }
     /// Whether we want to do dual part of presolve
     inline bool doDual() const {
          return (presolveActions_ & 1) == 0;
     }
     inline void setDoDual(bool doDual) {
          if (doDual) presolveActions_  &= ~1;
          else presolveActions_ |= 1;
     }
     /// Whether we want to do singleton part of presolve
     inline bool doSingleton() const {
          return (presolveActions_ & 2) == 0;
     }
     inline void setDoSingleton(bool doSingleton) {
          if (doSingleton) presolveActions_  &= ~2;
          else presolveActions_ |= 2;
     }
     /// Whether we want to do doubleton part of presolve
     inline bool doDoubleton() const {
          return (presolveActions_ & 4) == 0;
     }
     inline void setDoDoubleton(bool doDoubleton) {
          if (doDoubleton) presolveActions_  &= ~4;
          else presolveActions_ |= 4;
     }
     /// Whether we want to do tripleton part of presolve
     inline bool doTripleton() const {
          return (presolveActions_ & 8) == 0;
     }
     inline void setDoTripleton(bool doTripleton) {
          if (doTripleton) presolveActions_  &= ~8;
          else presolveActions_ |= 8;
     }
     /// Whether we want to do tighten part of presolve
     inline bool doTighten() const {
          return (presolveActions_ & 16) == 0;
     }
     inline void setDoTighten(bool doTighten) {
          if (doTighten) presolveActions_  &= ~16;
          else presolveActions_ |= 16;
     }
     /// Whether we want to do forcing part of presolve
     inline bool doForcing() const {
          return (presolveActions_ & 32) == 0;
     }
     inline void setDoForcing(bool doForcing) {
          if (doForcing) presolveActions_  &= ~32;
          else presolveActions_ |= 32;
     }
     /// Whether we want to do impliedfree part of presolve
     inline bool doImpliedFree() const {
          return (presolveActions_ & 64) == 0;
     }
     inline void setDoImpliedFree(bool doImpliedfree) {
          if (doImpliedfree) presolveActions_  &= ~64;
          else presolveActions_ |= 64;
     }
     /// Whether we want to do dupcol part of presolve
     inline bool doDupcol() const {
          return (presolveActions_ & 128) == 0;
     }
     inline void setDoDupcol(bool doDupcol) {
          if (doDupcol) presolveActions_  &= ~128;
          else presolveActions_ |= 128;
     }
     /// Whether we want to do duprow part of presolve
     inline bool doDuprow() const {
          return (presolveActions_ & 256) == 0;
     }
     inline void setDoDuprow(bool doDuprow) {
          if (doDuprow) presolveActions_  &= ~256;
          else presolveActions_ |= 256;
     }
     /// Whether we want to do singleton column part of presolve
     inline bool doSingletonColumn() const {
          return (presolveActions_ & 512) == 0;
     }
     inline void setDoSingletonColumn(bool doSingleton) {
          if (doSingleton) presolveActions_  &= ~512;
          else presolveActions_ |= 512;
     }
     /// Whether we want to do gubrow part of presolve
     inline bool doGubrow() const {
          return (presolveActions_ & 1024) == 0;
     }
     inline void setDoGubrow(bool doGubrow) {
          if (doGubrow) presolveActions_  &= ~1024;
          else presolveActions_ |= 1024;
     }
     /// Whether we want to do twoxtwo part of presolve
     inline bool doTwoxTwo() const {
          return (presolveActions_ & 2048) != 0;
     }
     inline void setDoTwoxtwo(bool doTwoxTwo) {
          if (!doTwoxTwo) presolveActions_  &= ~2048;
          else presolveActions_ |= 2048;
     }
     /// Set whole group
     inline int presolveActions() const {
          return presolveActions_ & 0xffff;
     }
     inline void setPresolveActions(int action) {
          presolveActions_  = (presolveActions_ & 0xffff0000) | (action & 0xffff);
     }
     /// Substitution level
     inline void setSubstitution(int value) {
          substitution_ = value;
     }
     /// Asks for statistics
     inline void statistics() {
          presolveActions_ |= 0x80000000;
     }
     /// Return presolve status (0,1,2)
     int presolveStatus() const;

     /**@name postsolve - postsolve the problem.  If the problem
       has not been solved to optimality, there are no guarantees.
      If you are using an algorithm like simplex that has a concept
      of "basic" rows/cols, then set updateStatus

      Note that if you modified the original problem after presolving,
      then you must ``undo'' these modifications before calling postsolve.
     This version updates original*/
     virtual void postsolve(bool updateStatus = true);

     /// Gets rid of presolve actions (e.g.when infeasible)
     void destroyPresolve();

     /**@name private or protected data */
private:
     /// Original model - must not be destroyed before postsolve
     ClpSimplex * originalModel_;

     /// ClpPresolved model - up to user to destroy by deleteClpPresolvedModel
     ClpSimplex * presolvedModel_;
     /** "Magic" number. If this is non-zero then any elements with this value
         may change and so presolve is very limited in what can be done
         to the row and column.  This is for non-linear problems.
         One could also allow for cases where sign of coefficient is known.
     */
     double nonLinearValue_;
     /// Original column numbers
     int * originalColumn_;
     /// Original row numbers
     int * originalRow_;
     /// Row objective
     double * rowObjective_;
     /// The list of transformations applied.
     const CoinPresolveAction *paction_;

     /// The postsolved problem will expand back to its former size
     /// as postsolve transformations are applied.
     /// It is efficient to allocate data structures for the final size
     /// of the problem rather than expand them as needed.
     /// These fields give the size of the original problem.
     int ncols_;
     int nrows_;
     CoinBigIndex nelems_;
     /// Number of major passes
     int numberPasses_;
     /// Substitution level
     int substitution_;
#ifndef CLP_NO_STD
     /// Name of saved model file
     std::string saveFile_;
#endif
     /** Whether we want to skip dual part of presolve etc.
         512 bit allows duplicate column processing on integer columns
         and dual stuff on integers
     */
     int presolveActions_;
protected:
     /// If you want to apply the individual presolve routines differently,
     /// or perhaps add your own to the mix,
     /// define a derived class and override this method
     virtual const CoinPresolveAction *presolve(CoinPresolveMatrix *prob);

     /// Postsolving is pretty generic; just apply the transformations
     /// in reverse order.
     /// You will probably only be interested in overriding this method
     /// if you want to add code to test for consistency
     /// while debugging new presolve techniques.
     virtual void postsolve(CoinPostsolveMatrix &prob);
     /** This is main part of Presolve */
     virtual ClpSimplex * gutsOfPresolvedModel(ClpSimplex * originalModel,
               double feasibilityTolerance,
               bool keepIntegers,
               int numberPasses,
               bool dropNames,
					       bool doRowObjective,
					       const char * prohibitedRows=NULL,
					       const char * prohibitedColumns=NULL);
};
#endif
