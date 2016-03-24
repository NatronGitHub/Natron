/* $Id$ */
// Copyright (C) 2008, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpNode_H
#define ClpNode_H

#include "CoinPragma.hpp"

// This implements all stuff for Clp fathom
/** This contains what is in a Clp "node"

 */

class ClpFactorization;
class ClpDualRowSteepest;
class ClpNodeStuff;
class ClpNode {

public:
     /**@name Useful methods */
     //@{
     /** Applies node to model
         0 - just tree bounds
         1 - tree bounds and basis etc
         2 - saved bounds and basis etc
     */
     void applyNode(ClpSimplex * model, int doBoundsEtc );
     /// Choose a new variable
     void chooseVariable(ClpSimplex * model, ClpNodeStuff * info);
     /// Fix on reduced costs
     int fixOnReducedCosts(ClpSimplex * model);
     /// Create odd arrays
     void createArrays(ClpSimplex * model);
     /// Clean up as crunch is different model
     void cleanUpForCrunch();
     //@}

     /**@name Gets and sets */
     //@{
     /// Objective value
     inline double objectiveValue() const {
          return objectiveValue_;
     }
     /// Set objective value
     inline void setObjectiveValue(double value) {
          objectiveValue_ = value;
     }
     /// Primal solution
     inline const double * primalSolution() const {
          return primalSolution_;
     }
     /// Dual solution
     inline const double * dualSolution() const {
          return dualSolution_;
     }
     /// Initial value of integer variable
     inline double branchingValue() const {
          return branchingValue_;
     }
     /// Sum infeasibilities
     inline double sumInfeasibilities() const {
          return sumInfeasibilities_;
     }
     /// Number infeasibilities
     inline int numberInfeasibilities() const {
          return numberInfeasibilities_;
     }
     /// Relative depth
     inline int depth() const {
          return depth_;
     }
     /// Estimated solution value
     inline double estimatedSolution() const {
          return estimatedSolution_;
     }
     /** Way for integer variable -1 down , +1 up */
     int way() const;
     /// Return true if branch exhausted
     bool fathomed() const;
     /// Change state of variable i.e. go other way
     void changeState();
     /// Sequence number of integer variable (-1 if none)
     inline int sequence() const {
          return sequence_;
     }
     /// If odd arrays exist
     inline bool oddArraysExist() const {
          return lower_ != NULL;
     }
     /// Status array
     inline const unsigned char * statusArray() const {
          return status_;
     }
     //@}

     /**@name Constructors, destructor */
     //@{
     /** Default constructor. */
     ClpNode();
     /// Constructor from model
     ClpNode (ClpSimplex * model, const ClpNodeStuff * stuff, int depth);
     /// Does work of constructor (partly so gdb will work)
     void gutsOfConstructor(ClpSimplex * model, const ClpNodeStuff * stuff,
                            int arraysExist, int depth);
     /** Destructor */
     virtual ~ClpNode();
     //@}

     /**@name Copy methods (at present illegal - will abort) */
     //@{
     /** The copy constructor. */
     ClpNode(const ClpNode&);
     /// Operator =
     ClpNode& operator=(const ClpNode&);
     //@}

protected:
// For state of branch
     typedef struct {
          unsigned int firstBranch: 1; //  nonzero if first branch on variable is up
          unsigned int branch: 2; //  0 means do first branch next, 1 second, 2 finished
          unsigned int spare: 29;
     } branchState;
     /**@name Data */
     //@{
     /// Initial value of integer variable
     double branchingValue_;
     /// Value of objective
     double objectiveValue_;
     /// Sum of infeasibilities
     double sumInfeasibilities_;
     /// Estimated solution value
     double estimatedSolution_;
     /// Factorization
     ClpFactorization * factorization_;
     /// Steepest edge weights
     ClpDualRowSteepest * weights_;
     /// Status vector
     unsigned char * status_;
     /// Primal solution
     double * primalSolution_;
     /// Dual solution
     double * dualSolution_;
     /// Integer lower bounds (only used in fathomMany)
     int * lower_;
     /// Integer upper bounds (only used in fathomMany)
     int * upper_;
     /// Pivot variables for factorization
     int * pivotVariables_;
     /// Variables fixed by reduced costs (at end of branch) 0x10000000 added if fixed to UB
     int * fixed_;
     /// State of branch
     branchState branchState_;
     /// Sequence number of integer variable (-1 if none)
     int sequence_;
     /// Number of infeasibilities
     int numberInfeasibilities_;
     /// Relative depth
     int depth_;
     /// Number fixed by reduced cost
     int numberFixed_;
     /// Flags - 1 duals scaled
     int flags_;
     /// Maximum number fixed by reduced cost
     int maximumFixed_;
     /// Maximum rows so far
     int maximumRows_;
     /// Maximum columns so far
     int maximumColumns_;
     /// Maximum Integers so far
     int maximumIntegers_;
     //@}
};
class ClpNodeStuff {

public:
     /**@name Constructors, destructor */
     //@{
     /** Default constructor. */
     ClpNodeStuff();
     /** Destructor */
     virtual ~ClpNodeStuff();
     //@}

     /**@name Copy methods (only copies ints etc, nulls arrays) */
     //@{
     /** The copy constructor. */
     ClpNodeStuff(const ClpNodeStuff&);
     /// Operator =
     ClpNodeStuff& operator=(const ClpNodeStuff&);
     /// Zaps stuff 1 - arrays, 2 ints, 3 both
     void zap(int type);
     //@}


     /**@name Fill methods */
     //@{
     /** Fill with pseudocosts */
     void fillPseudoCosts(const double * down, const double * up,
                          const int * priority,
                          const int * numberDown, const int * numberUp,
                          const int * numberDownInfeasible, const int * numberUpInfeasible,
                          int number);
     /// Update pseudo costs
     void update(int way, int sequence, double change, bool feasible);
     /// Return maximum number of nodes
     int maximumNodes() const;
     /// Return maximum space for nodes
     int maximumSpace() const;
     //@}

public:
     /**@name Data */
     //@{
     /// Integer tolerance
     double integerTolerance_;
     /// Integer increment
     double integerIncrement_;
     /// Small change in branch
     double smallChange_;
     /// Down pseudo costs
     double * downPseudo_;
     /// Up pseudo costs
     double * upPseudo_;
     /// Priority
     int * priority_;
     /// Number of times down
     int * numberDown_;
     /// Number of times up
     int * numberUp_;
     /// Number of times down infeasible
     int * numberDownInfeasible_;
     /// Number of times up infeasible
     int * numberUpInfeasible_;
     /// Copy of costs (local)
     double * saveCosts_;
     /// Array of ClpNodes
     ClpNode ** nodeInfo_;
     /// Large model if crunched
     ClpSimplex * large_;
     /// Which rows in large model
     int * whichRow_;
     /// Which columns in large model
     int * whichColumn_;
#ifndef NO_FATHOM_PRINT
     /// Cbc's message handler
     CoinMessageHandler * handler_;
#endif
     /// Number bounds in large model
     int nBound_;
     /// Save of specialOptions_ (local)
     int saveOptions_;
     /** Options to pass to solver
         1 - create external reduced costs for columns
         2 - create external reduced costs for rows
         4 - create external row activity (columns always done)
         Above only done if feasible
         32 - just create up to nDepth_+1 nodes
         65536 - set if activated
     */
     int solverOptions_;
     /// Maximum number of nodes to do
     int maximumNodes_;
     /// Number before trust from CbcModel
     int numberBeforeTrust_;
     /// State of search from CbcModel
     int stateOfSearch_;
     /// Number deep
     int nDepth_;
     /// Number nodes returned (-1 if fathom aborted)
     int nNodes_;
     /// Number of nodes explored
     int numberNodesExplored_;
     /// Number of iterations
     int numberIterations_;
     /// Type of presolve - 0 none, 1 crunch
     int presolveType_;
#ifndef NO_FATHOM_PRINT
     /// Depth passed in
     int startingDepth_;
     /// Node at which called
     int nodeCalled_;
#endif
     //@}
};
class ClpHashValue {

public:
     /**@name Useful methods */
     //@{
     /// Return index or -1 if not found
     int index(double value) const;
     /// Add value to list and return index
     int addValue(double value) ;
     /// Number of different entries
     inline int numberEntries() const {
          return numberHash_;
     }
     //@}

     /**@name Constructors, destructor */
     //@{
     /** Default constructor. */
     ClpHashValue();
     /** Useful constructor. */
     ClpHashValue(ClpSimplex * model);
     /** Destructor */
     virtual ~ClpHashValue();
     //@}

     /**@name Copy method */
     //@{
     /** The copy constructor. */
     ClpHashValue(const ClpHashValue&);
     /// =
     ClpHashValue& operator=(const ClpHashValue&);
     //@}
private:
     /**@name private stuff */
     //@{
     /** returns hash */
     int hash(double value) const;
     /// Resizes
     void resize(bool increaseMax);
     //@}

protected:
     /**@name Data members
        The data members are protected to allow access for derived classes. */
     //@{
     /// Data
     // for hashing
     typedef struct {
          double value;
          int index, next;
     } CoinHashLink;
     /// Hash table
     mutable CoinHashLink *hash_;
     /// Number of entries in hash table
     int numberHash_;
     /// Maximum number of entries in hash table i.e. size
     int maxHash_;
     /// Last used space
     int lastUsed_;
     //@}
};
#endif
