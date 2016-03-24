/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

// "Idiot" as the name of this algorithm is copylefted.  If you want to change
// the name then it should be something equally stupid (but not "Stupid") or
// even better something witty.

#ifndef Idiot_H
#define Idiot_H
#ifndef OSI_IDIOT
#include "ClpSimplex.hpp"
#define OsiSolverInterface ClpSimplex
#else
#include "OsiSolverInterface.hpp"
typedef int CoinBigIndex;
#endif
class CoinMessageHandler;
class CoinMessages;
/// for use internally
typedef struct {
     double infeas;
     double objval;
     double dropThis;
     double weighted;
     double sumSquared;
     double djAtBeginning;
     double djAtEnd;
     int iteration;
} IdiotResult;
/** This class implements a very silly algorithm.  It has no merit
    apart from the fact that it gets an approximate solution to
    some classes of problems.  Better if vaguely homogeneous.
    It works on problems where volume algorithm works and often
    gets a better primal solution but it has no dual solution.

    It can also be used as a "crash" to get a problem started.  This
    is probably its most useful function.

    It is based on the idea that algorithms with terrible convergence
    properties may be okay at first.  Throw in some random dubious tricks
    and the resulting code may be worth keeping as long as you don't
    look at it.

*/

class Idiot {

public:

     /**@name Constructors and destructor
        Just a pointer to model is kept
      */
     //@{
     /// Default constructor
     Idiot (  );
     /// Constructor with model
     Idiot ( OsiSolverInterface & model );

     /// Copy constructor.
     Idiot(const Idiot &);
     /// Assignment operator. This copies the data
     Idiot & operator=(const Idiot & rhs);
     /// Destructor
     ~Idiot (  );
     //@}


     /**@name Algorithmic calls
      */
     //@{
     /// Get an approximate solution with the idiot code
     void solve();
     /// Lightweight "crash"
     void crash(int numberPass, CoinMessageHandler * handler,
                const CoinMessages * messages, bool doCrossover = true);
     /** Use simplex to get an optimal solution
         mode is how many steps the simplex crossover should take to
         arrive to an extreme point:
         0 - chosen,all ever used, all
         1 - chosen, all
         2 - all
         3 - do not do anything  - maybe basis
         + 16 do presolves
     */
     void crossOver(int mode);
     //@}


     /**@name Gets and sets of most useful data
      */
     //@{
     /** Starting weight - small emphasizes feasibility,
         default 1.0e-4 */
     inline double getStartingWeight() const {
          return mu_;
     }
     inline void setStartingWeight(double value) {
          mu_ = value;
     }
     /** Weight factor - weight multiplied by this when changes,
         default 0.333 */
     inline double getWeightFactor() const {
          return muFactor_;
     }
     inline void setWeightFactor(double value) {
          muFactor_ = value;
     }
     /** Feasibility tolerance - problem essentially feasible if
         individual infeasibilities less than this.
         default 0.1 */
     inline double getFeasibilityTolerance() const {
          return smallInfeas_;
     }
     inline void setFeasibilityTolerance(double value) {
          smallInfeas_ = value;
     }
     /** Reasonably feasible.  Dubious method concentrates more on
         objective when sum of infeasibilities less than this.
         Very dubious default value of (Number of rows)/20 */
     inline double getReasonablyFeasible() const {
          return reasonableInfeas_;
     }
     inline void setReasonablyFeasible(double value) {
          reasonableInfeas_ = value;
     }
     /** Exit infeasibility - exit if sum of infeasibilities less than this.
         Default -1.0 (i.e. switched off) */
     inline double getExitInfeasibility() const {
          return exitFeasibility_;
     }
     inline void setExitInfeasibility(double value) {
          exitFeasibility_ = value;
     }
     /** Major iterations.  stop after this number.
         Default 30.  Use 2-5 for "crash" 50-100 for serious crunching */
     inline int getMajorIterations() const {
          return majorIterations_;
     }
     inline void setMajorIterations(int value) {
          majorIterations_ = value;
     }
     /** Minor iterations.  Do this number of tiny steps before
         deciding whether to change weights etc.
         Default - dubious sqrt(Number of Rows).
         Good numbers 105 to 405 say (5 is dubious method of making sure
         idiot is not trying to be clever which it may do every 10 minor
         iterations) */
     inline int getMinorIterations() const {
          return maxIts2_;
     }
     inline void setMinorIterations(int value) {
          maxIts2_ = value;
     }
     // minor iterations for first time
     inline int getMinorIterations0() const {
          return maxIts_;
     }
     inline void setMinorIterations0(int value) {
          maxIts_ = value;
     }
     /** Reduce weight after this many major iterations.  It may
         get reduced before this but this is a maximum.
         Default 3.  3-10 plausible. */
     inline int getReduceIterations() const {
          return maxBigIts_;
     }
     inline void setReduceIterations(int value) {
          maxBigIts_ = value;
     }
     /// Amount of information - default of 1 should be okay
     inline int getLogLevel() const {
          return logLevel_;
     }
     inline void setLogLevel(int value) {
          logLevel_ = value;
     }
     /// How lightweight - 0 not, 1 yes, 2 very lightweight
     inline int getLightweight() const {
          return lightWeight_;
     }
     inline void setLightweight(int value) {
          lightWeight_ = value;
     }
     /// strategy
     inline int getStrategy() const {
          return strategy_;
     }
     inline void setStrategy(int value) {
          strategy_ = value;
     }
     /// Fine tuning - okay if feasibility drop this factor
     inline double getDropEnoughFeasibility() const {
          return dropEnoughFeasibility_;
     }
     inline void setDropEnoughFeasibility(double value) {
          dropEnoughFeasibility_ = value;
     }
     /// Fine tuning - okay if weighted obj drop this factor
     inline double getDropEnoughWeighted() const {
          return dropEnoughWeighted_;
     }
     inline void setDropEnoughWeighted(double value) {
          dropEnoughWeighted_ = value;
     }
     //@}


/// Stuff for internal use
private:

     /// Does actual work
     // allow public!
public:
     void solve2(CoinMessageHandler * handler, const CoinMessages *messages);
private:
     IdiotResult IdiSolve(
          int nrows, int ncols, double * rowsol , double * colsol,
          double * pi, double * djs, const double * origcost ,
          double * rowlower,
          double * rowupper, const double * lower,
          const double * upper, const double * element,
          const int * row, const CoinBigIndex * colcc,
          const int * length, double * lambda,
          int maxIts, double mu, double drop,
          double maxmin, double offset,
          int strategy, double djTol, double djExit, double djFlag,
          CoinThreadRandom * randomNumberGenerator);
     int dropping(IdiotResult result,
                  double tolerance,
                  double small,
                  int *nbad);
     IdiotResult objval(int nrows, int ncols, double * rowsol , double * colsol,
                        double * pi, double * djs, const double * cost ,
                        const double * rowlower,
                        const double * rowupper, const double * lower,
                        const double * upper, const double * elemnt,
                        const int * row, const CoinBigIndex * columnStart,
                        const int * length, int extraBlock, int * rowExtra,
                        double * solExtra, double * elemExtra, double * upperExtra,
                        double * costExtra, double weight);
     // Deals with whenUsed and slacks
     int cleanIteration(int iteration, int ordinaryStart, int ordinaryEnd,
                        double * colsol, const double * lower, const double * upper,
                        const double * rowLower, const double * rowUpper,
                        const double * cost, const double * element, double fixTolerance, double & objChange,
                        double & infChange);
private:
     /// Underlying model
     OsiSolverInterface * model_;

     double djTolerance_;
     double mu_;  /* starting mu */
     double drop_; /* exit if drop over 5 checks less than this */
     double muFactor_; /* reduce mu by this */
     double stopMu_; /* exit if mu gets smaller than this */
     double smallInfeas_; /* feasibility tolerance */
     double reasonableInfeas_; /* use lambdas if feasibility less than this */
     double exitDrop_; /* candidate for stopping after a major iteration */
     double muAtExit_; /* mu on exit */
     double exitFeasibility_; /* exit if infeasibility less than this */
     double dropEnoughFeasibility_; /* okay if feasibility drop this factor */
     double dropEnoughWeighted_; /* okay if weighted obj drop this factor */
     int * whenUsed_; /* array to say what was used */
     int maxBigIts_; /* always reduce mu after this */
     int maxIts_; /* do this many iterations on first go */
     int majorIterations_;
     int logLevel_;
     int logFreq_;
     int checkFrequency_; /* can exit after 5 * this iterations (on drop) */
     int lambdaIterations_; /* do at least this many lambda iterations */
     int maxIts2_; /* do this many iterations on subsequent goes */
     int strategy_;   /* 0 - default strategy
		     1 - do accelerator step but be cautious
		     2 - do not do accelerator step
		     4 - drop, exitDrop and djTolerance all relative
		     8 - keep accelerator step to theta=10.0

                    32 - Scale
		   512 - crossover
                  2048 - keep lambda across mu change
		  4096 - return best solution (not last found)
		  8192 - always do a presolve in crossover
		 16384 - costed slacks found - so whenUsed_ longer */
     int lightWeight_; // 0 - normal, 1 lightweight
};
#endif
