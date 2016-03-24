/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).
/*
   Authors

   John Forrest

 */
#ifndef ClpSolve_H
#define ClpSolve_H

/**
    This is a very simple class to guide algorithms.  It is used to tidy up
    passing parameters to initialSolve and maybe for output from that

*/

class ClpSolve  {

public:

     /** enums for solve function */
     enum SolveType {
          useDual = 0,
          usePrimal,
          usePrimalorSprint,
          useBarrier,
          useBarrierNoCross,
          automatic,
          notImplemented
     };
     enum PresolveType {
          presolveOn = 0,
          presolveOff,
          presolveNumber,
          presolveNumberCost
     };

     /**@name Constructors and destructor and copy */
     //@{
     /// Default constructor
     ClpSolve (  );
     /// Constructor when you really know what you are doing
     ClpSolve ( SolveType method, PresolveType presolveType,
                int numberPasses, int options[6],
                int extraInfo[6], int independentOptions[3]);
     /// Generates code for above constructor
     void generateCpp(FILE * fp);
     /// Copy constructor.
     ClpSolve(const ClpSolve &);
     /// Assignment operator. This copies the data
     ClpSolve & operator=(const ClpSolve & rhs);
     /// Destructor
     ~ClpSolve (  );
     //@}

     /**@name Functions most useful to user */
     //@{
     /** Special options - bits
     0      4 - use crash (default allslack in dual, idiot in primal)
         8 - all slack basis in primal
     2      16 - switch off interrupt handling
     3      32 - do not try and make plus minus one matrix
         64 - do not use sprint even if problem looks good
      */
     /** which translation is:
         which:
         0 - startup in Dual  (nothing if basis exists).:
                      0 - no basis
       	   1 - crash
       	   2 - use initiative about idiot! but no crash
         1 - startup in Primal (nothing if basis exists):
                      0 - use initiative
       	   1 - use crash
       	   2 - use idiot and look at further info
       	   3 - use sprint and look at further info
       	   4 - use all slack
       	   5 - use initiative but no idiot
       	   6 - use initiative but no sprint
       	   7 - use initiative but no crash
                      8 - do allslack or idiot
                      9 - do allslack or sprint
       	   10 - slp before
       	   11 - no nothing and primal(0)
         2 - interrupt handling - 0 yes, 1 no (for threadsafe)
         3 - whether to make +- 1matrix - 0 yes, 1 no
         4 - for barrier
                      0 - dense cholesky
       	   1 - Wssmp allowing some long columns
       	   2 - Wssmp not allowing long columns
       	   3 - Wssmp using KKT
                      4 - Using Florida ordering
       	   8 - bit set to do scaling
       	   16 - set to be aggressive with gamma/delta?
                      32 - Use KKT
         5 - for presolve
                      1 - switch off dual stuff
         6 - for detailed printout (initially just presolve)
                      1 - presolve statistics
     */
     void setSpecialOption(int which, int value, int extraInfo = -1);
     int getSpecialOption(int which) const;

     /// Solve types
     void setSolveType(SolveType method, int extraInfo = -1);
     SolveType getSolveType();

     // Presolve types
     void setPresolveType(PresolveType amount, int extraInfo = -1);
     PresolveType getPresolveType();
     int getPresolvePasses() const;
     /// Extra info for idiot (or sprint)
     int getExtraInfo(int which) const;
     /** Say to return at once if infeasible,
         default is to solve */
     void setInfeasibleReturn(bool trueFalse);
     inline bool infeasibleReturn() const {
          return independentOptions_[0] != 0;
     }
     /// Whether we want to do dual part of presolve
     inline bool doDual() const {
          return (independentOptions_[1] & 1) == 0;
     }
     inline void setDoDual(bool doDual_) {
          if (doDual_) independentOptions_[1]  &= ~1;
          else independentOptions_[1] |= 1;
     }
     /// Whether we want to do singleton part of presolve
     inline bool doSingleton() const {
          return (independentOptions_[1] & 2) == 0;
     }
     inline void setDoSingleton(bool doSingleton_) {
          if (doSingleton_) independentOptions_[1]  &= ~2;
          else independentOptions_[1] |= 2;
     }
     /// Whether we want to do doubleton part of presolve
     inline bool doDoubleton() const {
          return (independentOptions_[1] & 4) == 0;
     }
     inline void setDoDoubleton(bool doDoubleton_) {
          if (doDoubleton_) independentOptions_[1]  &= ~4;
          else independentOptions_[1] |= 4;
     }
     /// Whether we want to do tripleton part of presolve
     inline bool doTripleton() const {
          return (independentOptions_[1] & 8) == 0;
     }
     inline void setDoTripleton(bool doTripleton_) {
          if (doTripleton_) independentOptions_[1]  &= ~8;
          else independentOptions_[1] |= 8;
     }
     /// Whether we want to do tighten part of presolve
     inline bool doTighten() const {
          return (independentOptions_[1] & 16) == 0;
     }
     inline void setDoTighten(bool doTighten_) {
          if (doTighten_) independentOptions_[1]  &= ~16;
          else independentOptions_[1] |= 16;
     }
     /// Whether we want to do forcing part of presolve
     inline bool doForcing() const {
          return (independentOptions_[1] & 32) == 0;
     }
     inline void setDoForcing(bool doForcing_) {
          if (doForcing_) independentOptions_[1]  &= ~32;
          else independentOptions_[1] |= 32;
     }
     /// Whether we want to do impliedfree part of presolve
     inline bool doImpliedFree() const {
          return (independentOptions_[1] & 64) == 0;
     }
     inline void setDoImpliedFree(bool doImpliedfree) {
          if (doImpliedfree) independentOptions_[1]  &= ~64;
          else independentOptions_[1] |= 64;
     }
     /// Whether we want to do dupcol part of presolve
     inline bool doDupcol() const {
          return (independentOptions_[1] & 128) == 0;
     }
     inline void setDoDupcol(bool doDupcol_) {
          if (doDupcol_) independentOptions_[1]  &= ~128;
          else independentOptions_[1] |= 128;
     }
     /// Whether we want to do duprow part of presolve
     inline bool doDuprow() const {
          return (independentOptions_[1] & 256) == 0;
     }
     inline void setDoDuprow(bool doDuprow_) {
          if (doDuprow_) independentOptions_[1]  &= ~256;
          else independentOptions_[1] |= 256;
     }
     /// Whether we want to do singleton column part of presolve
     inline bool doSingletonColumn() const {
          return (independentOptions_[1] & 512) == 0;
     }
     inline void setDoSingletonColumn(bool doSingleton_) {
          if (doSingleton_) independentOptions_[1]  &= ~512;
          else independentOptions_[1] |= 512;
     }
     /// Whether we want to kill small substitutions
     inline bool doKillSmall() const {
          return (independentOptions_[1] & 1024) == 0;
     }
     inline void setDoKillSmall(bool doKill) {
          if (doKill) independentOptions_[1]  &= ~1024;
          else independentOptions_[1] |= 1024;
     }
     /// Set whole group
     inline int presolveActions() const {
          return independentOptions_[1] & 0xffff;
     }
     inline void setPresolveActions(int action) {
          independentOptions_[1]  = (independentOptions_[1] & 0xffff0000) | (action & 0xffff);
     }
     /// Largest column for substitution (normally 3)
     inline int substitution() const {
          return independentOptions_[2];
     }
     inline void setSubstitution(int value) {
          independentOptions_[2] = value;
     }
     //@}

////////////////// data //////////////////
private:

     /**@name data.
     */
     //@{
     /// Solve type
     SolveType method_;
     /// Presolve type
     PresolveType presolveType_;
     /// Amount of presolve
     int numberPasses_;
     /// Options - last is switch for OsiClp
     int options_[7];
     /// Extra information
     int extraInfo_[7];
     /** Extra algorithm dependent options
         0 - if set return from clpsolve if infeasible
         1 - To be copied over to presolve options
         2 - max substitution level
     */
     int independentOptions_[3];
     //@}
};

/// For saving extra information to see if looping.
class ClpSimplexProgress {

public:


     /**@name Constructors and destructor and copy */
     //@{
     /// Default constructor
     ClpSimplexProgress (  );

     /// Constructor from model
     ClpSimplexProgress ( ClpSimplex * model );

     /// Copy constructor.
     ClpSimplexProgress(const ClpSimplexProgress &);

     /// Assignment operator. This copies the data
     ClpSimplexProgress & operator=(const ClpSimplexProgress & rhs);
     /// Destructor
     ~ClpSimplexProgress (  );
     /// Resets as much as possible
     void reset();
     /// Fill from model
     void fillFromModel ( ClpSimplex * model );

     //@}

     /**@name Check progress */
     //@{
     /** Returns -1 if okay, -n+1 (n number of times bad) if bad but action taken,
         >=0 if give up and use as problem status
     */
     int looping (  );
     /// Start check at beginning of whileIterating
     void startCheck();
     /// Returns cycle length in whileIterating
     int cycle(int in, int out, int wayIn, int wayOut);

     /// Returns previous objective (if -1) - current if (0)
     double lastObjective(int back = 1) const;
     /// Set real primal infeasibility and move back
     void setInfeasibility(double value);
     /// Returns real primal infeasibility (if -1) - current if (0)
     double lastInfeasibility(int back = 1) const;
     /// Modify objective e.g. if dual infeasible in dual
     void modifyObjective(double value);
     /// Returns previous iteration number (if -1) - current if (0)
     int lastIterationNumber(int back = 1) const;
     /// clears all iteration numbers (to switch off panic)
     void clearIterationNumbers();
     /// Odd state
     inline void newOddState() {
          oddState_ = - oddState_ - 1;
     }
     inline void endOddState() {
          oddState_ = abs(oddState_);
     }
     inline void clearOddState() {
          oddState_ = 0;
     }
     inline int oddState() const {
          return oddState_;
     }
     /// number of bad times
     inline int badTimes() const {
          return numberBadTimes_;
     }
     inline void clearBadTimes() {
          numberBadTimes_ = 0;
     }
     /// number of really bad times
     inline int reallyBadTimes() const {
          return numberReallyBadTimes_;
     }
     inline void incrementReallyBadTimes() {
          numberReallyBadTimes_++;
     }
     /// number of times flagged
     inline int timesFlagged() const {
          return numberTimesFlagged_;
     }
     inline void clearTimesFlagged() {
          numberTimesFlagged_ = 0;
     }
     inline void incrementTimesFlagged() {
          numberTimesFlagged_++;
     }

     //@}
     /**@name Data  */
#define CLP_PROGRESS 5
     //#define CLP_PROGRESS_WEIGHT 10
     //@{
     /// Objective values
     double objective_[CLP_PROGRESS];
     /// Sum of infeasibilities for algorithm
     double infeasibility_[CLP_PROGRESS];
     /// Sum of real primal infeasibilities for primal
     double realInfeasibility_[CLP_PROGRESS];
#ifdef CLP_PROGRESS_WEIGHT
     /// Objective values for weights
     double objectiveWeight_[CLP_PROGRESS_WEIGHT];
     /// Sum of infeasibilities for algorithm for weights
     double infeasibilityWeight_[CLP_PROGRESS_WEIGHT];
     /// Sum of real primal infeasibilities for primal for weights
     double realInfeasibilityWeight_[CLP_PROGRESS_WEIGHT];
     /// Drop  for weights
     double drop_;
     /// Best? for weights
     double best_;
#endif
     /// Initial weight for weights
     double initialWeight_;
#define CLP_CYCLE 12
     /// For cycle checking
     //double obj_[CLP_CYCLE];
     int in_[CLP_CYCLE];
     int out_[CLP_CYCLE];
     char way_[CLP_CYCLE];
     /// Pointer back to model so we can get information
     ClpSimplex * model_;
     /// Number of infeasibilities
     int numberInfeasibilities_[CLP_PROGRESS];
     /// Iteration number at which occurred
     int iterationNumber_[CLP_PROGRESS];
#ifdef CLP_PROGRESS_WEIGHT
     /// Number of infeasibilities for weights
     int numberInfeasibilitiesWeight_[CLP_PROGRESS_WEIGHT];
     /// Iteration number at which occurred for weights
     int iterationNumberWeight_[CLP_PROGRESS_WEIGHT];
#endif
     /// Number of times checked (so won't stop too early)
     int numberTimes_;
     /// Number of times it looked like loop
     int numberBadTimes_;
     /// Number really bad times
     int numberReallyBadTimes_;
     /// Number of times no iterations as flagged
     int numberTimesFlagged_;
     /// If things are in an odd state
     int oddState_;
     //@}
};

#include "ClpConfig.h"
#if CLP_HAS_ABC
#include "AbcCommon.hpp"
/// For saving extra information to see if looping.
class AbcSimplexProgress : public ClpSimplexProgress {

public:


     /**@name Constructors and destructor and copy */
     //@{
     /// Default constructor
     AbcSimplexProgress (  );

     /// Constructor from model
     AbcSimplexProgress ( ClpSimplex * model );

     /// Copy constructor.
     AbcSimplexProgress(const AbcSimplexProgress &);

     /// Assignment operator. This copies the data
     AbcSimplexProgress & operator=(const AbcSimplexProgress & rhs);
     /// Destructor
     ~AbcSimplexProgress (  );

     //@}

     /**@name Check progress */
     //@{
     /** Returns -1 if okay, -n+1 (n number of times bad) if bad but action taken,
         >=0 if give up and use as problem status
     */
     int looping (  );

     //@}
     /**@name Data  */
     //@}
};
#endif
#endif
