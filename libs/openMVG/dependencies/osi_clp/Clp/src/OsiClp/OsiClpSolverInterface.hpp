// $Id$
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


#ifndef OsiClpSolverInterface_H
#define OsiClpSolverInterface_H

#include <string>
#include <cfloat>
#include <map>

#include "ClpSimplex.hpp"
#include "ClpLinearObjective.hpp"
#include "CoinPackedMatrix.hpp"
#include "OsiSolverInterface.hpp"
#include "CoinWarmStartBasis.hpp"
#include "ClpEventHandler.hpp"
#include "ClpNode.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinFinite.hpp"

class OsiRowCut;
class OsiClpUserSolver;
class OsiClpDisasterHandler;
class CoinSet;
static const double OsiClpInfinity = COIN_DBL_MAX;

//#############################################################################

/** Clp Solver Interface
    
Instantiation of OsiClpSolverInterface for the Model Algorithm.

*/

class OsiClpSolverInterface :
  virtual public OsiSolverInterface {
  friend void OsiClpSolverInterfaceUnitTest(const std::string & mpsDir, const std::string & netlibDir);
  
public:
  //---------------------------------------------------------------------------
  /**@name Solve methods */
  //@{
  /// Solve initial LP relaxation 
  virtual void initialSolve();
  
  /// Resolve an LP relaxation after problem modification
  virtual void resolve();
  
  /// Resolve an LP relaxation after problem modification (try GUB)
  virtual void resolveGub(int needed);
  
  /// Invoke solver's built-in enumeration algorithm
  virtual void branchAndBound();

  /** Solve when primal column and dual row solutions are near-optimal
      options - 0 no presolve (use primal and dual)
                1 presolve (just use primal)
		2 no presolve (just use primal)
      basis -   0 use all slack basis
                1 try and put some in basis
  */
  void crossover(int options,int basis);
  //@}
  
  /*! @name OsiSimplexInterface methods
      \brief Methods for the Osi Simplex API.

    The current implementation should work for both minimisation and
    maximisation in mode 1 (tableau access). In mode 2 (single pivot), only
    minimisation is supported as of 100907.
  */
  //@{
  /** \brief Simplex API capability.
  
    Returns
     - 0 if no simplex API
     - 1 if can just do getBInv etc
     - 2 if has all OsiSimplex methods
  */
  virtual int canDoSimplexInterface() const;

  /*! \brief Enables simplex mode 1 (tableau access)
  
    Tells solver that calls to getBInv etc are about to take place.
    Underlying code may need mutable as this may be called from 
    CglCut::generateCuts which is const.  If that is too horrific then
    each solver e.g. BCP or CBC will have to do something outside
    main loop.
  */
  virtual void enableFactorization() const;

  /*! \brief Undo any setting changes made by #enableFactorization */
  virtual void disableFactorization() const;
  
  /** Returns true if a basis is available
      AND problem is optimal.  This should be used to see if
      the BInvARow type operations are possible and meaningful. 
  */
  virtual bool basisIsAvailable() const;
  
  /** The following two methods may be replaced by the
      methods of OsiSolverInterface using OsiWarmStartBasis if:
      1. OsiWarmStartBasis resize operation is implemented
      more efficiently and
      2. It is ensured that effects on the solver are the same
      
      Returns a basis status of the structural/artificial variables 
      At present as warm start i.e 0 free, 1 basic, 2 upper, 3 lower
      
      NOTE  artificials are treated as +1 elements so for <= rhs
      artificial will be at lower bound if constraint is tight
      
      This means that Clpsimplex flips artificials as it works
      in terms of row activities
  */
  virtual void getBasisStatus(int* cstat, int* rstat) const;
  
  /** Set the status of structural/artificial variables and
      factorize, update solution etc 
      
      NOTE  artificials are treated as +1 elements so for <= rhs
      artificial will be at lower bound if constraint is tight
      
      This means that Clpsimplex flips artificials as it works
      in terms of row activities
      Returns 0 if OK, 1 if problem is bad e.g. duplicate elements, too large ...
  */
  virtual int setBasisStatus(const int* cstat, const int* rstat);
  
  ///Get the reduced gradient for the cost vector c 
  virtual void getReducedGradient(double* columnReducedCosts, 
				  double * duals,
				  const double * c) const ;
  
  ///Get a row of the tableau (slack part in slack if not NULL)
  virtual void getBInvARow(int row, double* z, double * slack=NULL) const;
  
  /** Get a row of the tableau (slack part in slack if not NULL)
      If keepScaled is true then scale factors not applied after so
      user has to use coding similar to what is in this method
  */
  virtual void getBInvARow(int row, CoinIndexedVector * z, CoinIndexedVector * slack=NULL,
			   bool keepScaled=false) const;
  
  ///Get a row of the basis inverse
  virtual void getBInvRow(int row, double* z) const;
  
  ///Get a column of the tableau
  virtual void getBInvACol(int col, double* vec) const ;
  
  ///Get a column of the tableau
  virtual void getBInvACol(int col, CoinIndexedVector * vec) const ;
  
  /** Update (i.e. ftran) the vector passed in.
      Unscaling is applied after - can't be applied before
  */
  
  virtual void getBInvACol(CoinIndexedVector * vec) const ;
  
  ///Get a column of the basis inverse
  virtual void getBInvCol(int col, double* vec) const ;
  
  /** Get basic indices (order of indices corresponds to the
      order of elements in a vector retured by getBInvACol() and
      getBInvCol()).
  */
  virtual void getBasics(int* index) const;

  /*! \brief Enables simplex mode 2 (individual pivot control)

     This method is supposed to ensure that all typical things (like
     reduced costs, etc.) are updated when individual pivots are executed
     and can be queried by other methods.
  */
  virtual void enableSimplexInterface(bool doingPrimal);
  /// Copy across enabled stuff from one solver to another
  void copyEnabledSuff(OsiClpSolverInterface & rhs);
  
  /*! \brief Undo setting changes made by #enableSimplexInterface */
  virtual void disableSimplexInterface();
  /// Copy across enabled stuff from one solver to another
  void copyEnabledStuff(ClpSimplex & rhs);

  /** Perform a pivot by substituting a colIn for colOut in the basis. 
      The status of the leaving variable is given in statOut. Where
      1 is to upper bound, -1 to lower bound
      Return code is 0 for okay,
      1 if inaccuracy forced re-factorization (should be okay) and
      -1 for singular factorization
  */
  virtual int pivot(int colIn, int colOut, int outStatus);
  
  /** Obtain a result of the primal pivot 
      Outputs: colOut -- leaving column, outStatus -- its status,
      t -- step size, and, if dx!=NULL, *dx -- primal ray direction.
      Inputs: colIn -- entering column, sign -- direction of its change (+/-1).
      Both for colIn and colOut, artificial variables are index by
      the negative of the row index minus 1.
      Return code (for now): 0 -- leaving variable found, 
      -1 -- everything else?
      Clearly, more informative set of return values is required 
      Primal and dual solutions are updated
  */
  virtual int primalPivotResult(int colIn, int sign, 
				int& colOut, int& outStatus, 
				double& t, CoinPackedVector* dx);
  
  /** Obtain a result of the dual pivot (similar to the previous method)
      Differences: entering variable and a sign of its change are now
      the outputs, the leaving variable and its statuts -- the inputs
      If dx!=NULL, then *dx contains dual ray
      Return code: same
  */
  virtual int dualPivotResult(int& colIn, int& sign, 
			      int colOut, int outStatus, 
			      double& t, CoinPackedVector* dx);
  
  
  //@}
  //---------------------------------------------------------------------------
  /**@name Parameter set/get methods
     
  The set methods return true if the parameter was set to the given value,
  false otherwise. There can be various reasons for failure: the given
  parameter is not applicable for the solver (e.g., refactorization
  frequency for the clp algorithm), the parameter is not yet implemented
  for the solver or simply the value of the parameter is out of the range
  the solver accepts. If a parameter setting call returns false check the
  details of your solver.
  
  The get methods return true if the given parameter is applicable for the
  solver and is implemented. In this case the value of the parameter is
  returned in the second argument. Otherwise they return false.
  */
  //@{
  // Set an integer parameter
  bool setIntParam(OsiIntParam key, int value);
  // Set an double parameter
  bool setDblParam(OsiDblParam key, double value);
  // Set a string parameter
  bool setStrParam(OsiStrParam key, const std::string & value);
  // Get an integer parameter
  bool getIntParam(OsiIntParam key, int& value) const;
  // Get an double parameter
  bool getDblParam(OsiDblParam key, double& value) const;
  // Get a string parameter
  bool getStrParam(OsiStrParam key, std::string& value) const;
  // Set a hint parameter - overrides OsiSolverInterface
  virtual bool setHintParam(OsiHintParam key, bool yesNo=true,
                            OsiHintStrength strength=OsiHintTry,
                            void * otherInformation=NULL);
  //@}
  
  //---------------------------------------------------------------------------
  ///@name Methods returning info on how the solution process terminated
  //@{
  /// Are there a numerical difficulties?
  virtual bool isAbandoned() const;
  /// Is optimality proven?
  virtual bool isProvenOptimal() const;
  /// Is primal infeasiblity proven?
  virtual bool isProvenPrimalInfeasible() const;
  /// Is dual infeasiblity proven?
  virtual bool isProvenDualInfeasible() const;
  /// Is the given primal objective limit reached?
  virtual bool isPrimalObjectiveLimitReached() const;
  /// Is the given dual objective limit reached?
  virtual bool isDualObjectiveLimitReached() const;
  /// Iteration limit reached?
  virtual bool isIterationLimitReached() const;
  //@}
  
  //---------------------------------------------------------------------------
  /**@name WarmStart related methods */
  //@{
  
  /*! \brief Get an empty warm start object
    
  This routine returns an empty CoinWarmStartBasis object. Its purpose is
  to provide a way to give a client a warm start basis object of the
  appropriate type, which can resized and modified as desired.
  */
  
  virtual CoinWarmStart *getEmptyWarmStart () const;
  
  /// Get warmstarting information
  virtual CoinWarmStart* getWarmStart() const;
  /// Get warmstarting information
  inline CoinWarmStartBasis* getPointerToWarmStart() 
  { return &basis_;}
  /// Get warmstarting information
  inline const CoinWarmStartBasis* getConstPointerToWarmStart() const 
  { return &basis_;}
  /** Set warmstarting information. Return true/false depending on whether
      the warmstart information was accepted or not. */
  virtual bool setWarmStart(const CoinWarmStart* warmstart);
  /** \brief Get warm start information.
      
      Return warm start information for the current state of the solver
      interface. If there is no valid warm start information, an empty warm
      start object wil be returned.  This does not necessarily create an 
      object - may just point to one.  must Delete set true if user
      should delete returned object.
      OsiClp version always returns pointer and false.
  */
  virtual CoinWarmStart* getPointerToWarmStart(bool & mustDelete) ;

  //@}
  
  //---------------------------------------------------------------------------
  /**@name Hotstart related methods (primarily used in strong branching).
     The user can create a hotstart (a snapshot) of the optimization process
     then reoptimize over and over again always starting from there.<br>
     <strong>NOTE</strong>: between hotstarted optimizations only
     bound changes are allowed. */
  //@{
  /// Create a hotstart point of the optimization process
  virtual void markHotStart();
  /// Optimize starting from the hotstart
  virtual void solveFromHotStart();
  /// Delete the snapshot
  virtual void unmarkHotStart();
  /** Start faster dual - returns negative if problems 1 if infeasible,
      Options to pass to solver
      1 - create external reduced costs for columns
      2 - create external reduced costs for rows
      4 - create external row activity (columns always done)
      Above only done if feasible
      When set resolve does less work
  */
  int startFastDual(int options);
  /// Stop fast dual
  void stopFastDual();
  /// Sets integer tolerance and increment
  void setStuff(double tolerance,double increment);
  //@}
  
  //---------------------------------------------------------------------------
  /**@name Problem information methods
     
  These methods call the solver's query routines to return
  information about the problem referred to by the current object.
  Querying a problem that has no data associated with it result in
  zeros for the number of rows and columns, and NULL pointers from
  the methods that return vectors.
  
  Const pointers returned from any data-query method are valid as
  long as the data is unchanged and the solver is not called.
  */
  //@{
  /**@name Methods related to querying the input data */
  //@{
  /// Get number of columns
  virtual int getNumCols() const {
    return modelPtr_->numberColumns(); }
  
  /// Get number of rows
  virtual int getNumRows() const {
    return modelPtr_->numberRows(); }
  
  /// Get number of nonzero elements
  virtual int getNumElements() const {
    int retVal = 0;
    const CoinPackedMatrix * matrix =modelPtr_->matrix();
    if ( matrix != NULL ) retVal=matrix->getNumElements();
    return retVal; }

    /// Return name of row if one exists or Rnnnnnnn
    /// maxLen is currently ignored and only there to match the signature from the base class! 
    virtual std::string getRowName(int rowIndex,
				   unsigned maxLen = static_cast<unsigned>(std::string::npos)) const;
    
    /// Return name of column if one exists or Cnnnnnnn
    /// maxLen is currently ignored and only there to match the signature from the base class! 
    virtual std::string getColName(int colIndex,
				   unsigned maxLen = static_cast<unsigned>(std::string::npos)) const;
    
  
  /// Get pointer to array[getNumCols()] of column lower bounds
  virtual const double * getColLower() const { return modelPtr_->columnLower(); }
  
  /// Get pointer to array[getNumCols()] of column upper bounds
  virtual const double * getColUpper() const { return modelPtr_->columnUpper(); }
  
  /** Get pointer to array[getNumRows()] of row constraint senses.
      <ul>
      <li>'L' <= constraint
      <li>'E' =  constraint
      <li>'G' >= constraint
      <li>'R' ranged constraint
      <li>'N' free constraint
      </ul>
  */
  virtual const char * getRowSense() const;
  
  /** Get pointer to array[getNumRows()] of rows right-hand sides
      <ul>
      <li> if rowsense()[i] == 'L' then rhs()[i] == rowupper()[i]
      <li> if rowsense()[i] == 'G' then rhs()[i] == rowlower()[i]
      <li> if rowsense()[i] == 'R' then rhs()[i] == rowupper()[i]
      <li> if rowsense()[i] == 'N' then rhs()[i] == 0.0
      </ul>
  */
  virtual const double * getRightHandSide() const ;
  
  /** Get pointer to array[getNumRows()] of row ranges.
      <ul>
      <li> if rowsense()[i] == 'R' then
      rowrange()[i] == rowupper()[i] - rowlower()[i]
      <li> if rowsense()[i] != 'R' then
      rowrange()[i] is undefined
      </ul>
  */
  virtual const double * getRowRange() const ;
  
  /// Get pointer to array[getNumRows()] of row lower bounds
  virtual const double * getRowLower() const { return modelPtr_->rowLower(); }
  
  /// Get pointer to array[getNumRows()] of row upper bounds
  virtual const double * getRowUpper() const { return modelPtr_->rowUpper(); }
  
  /// Get pointer to array[getNumCols()] of objective function coefficients
  virtual const double * getObjCoefficients() const 
  { if (fakeMinInSimplex_)
      return linearObjective_ ;
    else
      return modelPtr_->objective(); }
  
  /// Get objective function sense (1 for min (default), -1 for max)
  virtual double getObjSense() const 
  { return ((fakeMinInSimplex_)?-modelPtr_->optimizationDirection():
  				 modelPtr_->optimizationDirection()); }
  
  /// Return true if column is continuous
  virtual bool isContinuous(int colNumber) const;
  /// Return true if variable is binary
  virtual bool isBinary(int colIndex) const;
  
  /** Return true if column is integer.
      Note: This function returns true if the the column
      is binary or a general integer.
  */
  virtual bool isInteger(int colIndex) const;
  
  /// Return true if variable is general integer
  virtual bool isIntegerNonBinary(int colIndex) const;
  
  /// Return true if variable is binary and not fixed at either bound
  virtual bool isFreeBinary(int colIndex) const; 
  /**  Return array of column length
       0 - continuous
       1 - binary (may get fixed later)
       2 - general integer (may get fixed later)
  */
  virtual const char * getColType(bool refresh=false) const;
  
  /** Return true if column is integer but does not have to
      be declared as such.
      Note: This function returns true if the the column
      is binary or a general integer.
  */
  bool isOptionalInteger(int colIndex) const;
  /** Set the index-th variable to be an optional integer variable */
  void setOptionalInteger(int index);
  
  /// Get pointer to row-wise copy of matrix
  virtual const CoinPackedMatrix * getMatrixByRow() const;
  
  /// Get pointer to column-wise copy of matrix
  virtual const CoinPackedMatrix * getMatrixByCol() const;
  
  /// Get pointer to mutable column-wise copy of matrix
  virtual CoinPackedMatrix * getMutableMatrixByCol() const;
  
    /// Get solver's value for infinity
  virtual double getInfinity() const { return OsiClpInfinity; }
  //@}
  
  /**@name Methods related to querying the solution */
  //@{
  /// Get pointer to array[getNumCols()] of primal solution vector
  virtual const double * getColSolution() const; 
  
  /// Get pointer to array[getNumRows()] of dual prices
  virtual const double * getRowPrice() const;
  
  /// Get a pointer to array[getNumCols()] of reduced costs
  virtual const double * getReducedCost() const; 
  
  /** Get pointer to array[getNumRows()] of row activity levels (constraint
      matrix times the solution vector */
  virtual const double * getRowActivity() const; 
  
  /// Get objective function value
  virtual double getObjValue() const;
  
  /** Get how many iterations it took to solve the problem (whatever
      "iteration" mean to the solver. */
  virtual int getIterationCount() const 
  { return modelPtr_->numberIterations(); }
  
  /** Get as many dual rays as the solver can provide. (In case of proven
      primal infeasibility there should be at least one.)

      The first getNumRows() ray components will always be associated with
      the row duals (as returned by getRowPrice()). If \c fullRay is true,
      the final getNumCols() entries will correspond to the ray components
      associated with the nonbasic variables. If the full ray is requested
      and the method cannot provide it, it will throw an exception.

      <strong>NOTE for implementers of solver interfaces:</strong> <br>
      The double pointers in the vector should point to arrays of length
      getNumRows() and they should be allocated via new[]. <br>
      
      <strong>NOTE for users of solver interfaces:</strong> <br>
      It is the user's responsibility to free the double pointers in the
      vector using delete[].
  */
  virtual std::vector<double*> getDualRays(int maxNumRays,
					   bool fullRay = false) const;
  /** Get as many primal rays as the solver can provide. (In case of proven
      dual infeasibility there should be at least one.)
      
      <strong>NOTE for implementers of solver interfaces:</strong> <br>
      The double pointers in the vector should point to arrays of length
      getNumCols() and they should be allocated via new[]. <br>
      
      <strong>NOTE for users of solver interfaces:</strong> <br>
      It is the user's responsibility to free the double pointers in the
      vector using delete[].
  */
  virtual std::vector<double*> getPrimalRays(int maxNumRays) const;
  
  //@}
  //@}
  
  //---------------------------------------------------------------------------
  
  /**@name Problem modifying methods */
  //@{
  //-------------------------------------------------------------------------
  /**@name Changing bounds on variables and constraints */
  //@{
  /** Set an objective function coefficient */
  virtual void setObjCoeff( int elementIndex, double elementValue );
  
  /** Set a single column lower bound<br>
      Use -DBL_MAX for -infinity. */
  virtual void setColLower( int elementIndex, double elementValue );
  
  /** Set a single column upper bound<br>
      Use DBL_MAX for infinity. */
  virtual void setColUpper( int elementIndex, double elementValue );
  
  /** Set a single column lower and upper bound */
  virtual void setColBounds( int elementIndex,
                             double lower, double upper );
  
  /** Set the bounds on a number of columns simultaneously<br>
      The default implementation just invokes setColLower() and
      setColUpper() over and over again.
      @param indexFirst,indexLast pointers to the beginning and after the
      end of the array of the indices of the variables whose
      <em>either</em> bound changes
      @param boundList the new lower/upper bound pairs for the variables
  */
  virtual void setColSetBounds(const int* indexFirst,
                               const int* indexLast,
                               const double* boundList);
  
  /** Set a single row lower bound<br>
      Use -DBL_MAX for -infinity. */
  virtual void setRowLower( int elementIndex, double elementValue );
  
  /** Set a single row upper bound<br>
      Use DBL_MAX for infinity. */
  virtual void setRowUpper( int elementIndex, double elementValue ) ;
  
  /** Set a single row lower and upper bound */
  virtual void setRowBounds( int elementIndex,
                             double lower, double upper ) ;
  
  /** Set the type of a single row<br> */
  virtual void setRowType(int index, char sense, double rightHandSide,
                          double range);
  
  /** Set the bounds on a number of rows simultaneously<br>
      The default implementation just invokes setRowLower() and
      setRowUpper() over and over again.
      @param indexFirst,indexLast pointers to the beginning and after the
      end of the array of the indices of the constraints whose
      <em>either</em> bound changes
      @param boundList the new lower/upper bound pairs for the constraints
  */
  virtual void setRowSetBounds(const int* indexFirst,
                               const int* indexLast,
                               const double* boundList);
  
  /** Set the type of a number of rows simultaneously<br>
      The default implementation just invokes setRowType()
      over and over again.
      @param indexFirst,indexLast pointers to the beginning and after the
      end of the array of the indices of the constraints whose
      <em>any</em> characteristics changes
      @param senseList the new senses
      @param rhsList   the new right hand sides
      @param rangeList the new ranges
  */
  virtual void setRowSetTypes(const int* indexFirst,
                              const int* indexLast,
                              const char* senseList,
                              const double* rhsList,
                              const double* rangeList);
    /** Set the objective coefficients for all columns
	array [getNumCols()] is an array of values for the objective.
        This defaults to a series of set operations and is here for speed.
    */
    virtual void setObjective(const double * array);

    /** Set the lower bounds for all columns
	array [getNumCols()] is an array of values for the objective.
        This defaults to a series of set operations and is here for speed.
    */
    virtual void setColLower(const double * array);

    /** Set the upper bounds for all columns
	array [getNumCols()] is an array of values for the objective.
        This defaults to a series of set operations and is here for speed.
    */
    virtual void setColUpper(const double * array);

//    using OsiSolverInterface::setRowName ;
    /// Set name of row
//    virtual void setRowName(int rowIndex, std::string & name) ;
    virtual void setRowName(int rowIndex, std::string  name) ;
    
//    using OsiSolverInterface::setColName ;
    /// Set name of column
//    virtual void setColName(int colIndex, std::string & name) ;
    virtual void setColName(int colIndex, std::string  name) ;
    
  //@}
  
  //-------------------------------------------------------------------------
  /**@name Integrality related changing methods */
  //@{
  /** Set the index-th variable to be a continuous variable */
  virtual void setContinuous(int index);
  /** Set the index-th variable to be an integer variable */
  virtual void setInteger(int index);
  /** Set the variables listed in indices (which is of length len) to be
      continuous variables */
  virtual void setContinuous(const int* indices, int len);
  /** Set the variables listed in indices (which is of length len) to be
      integer variables */
  virtual void setInteger(const int* indices, int len);
  /// Number of SOS sets
  inline int numberSOS() const
  { return numberSOS_;}
  /// SOS set info
  inline const CoinSet * setInfo() const
  { return setInfo_;}
  /** \brief Identify integer variables and SOS and create corresponding objects.
  
    Record integer variables and create an OsiSimpleInteger object for each
    one.  All existing OsiSimpleInteger objects will be destroyed.
    If the solver supports SOS then do the same for SOS.
     If justCount then no objects created and we just store numberIntegers_
    Returns number of SOS
  */

  virtual int findIntegersAndSOS(bool justCount);
  //@}
  
  //-------------------------------------------------------------------------
  /// Set objective function sense (1 for min (default), -1 for max,)
  virtual void setObjSense(double s ) 
  { modelPtr_->setOptimizationDirection( s < 0 ? -1 : 1); }
  
  /** Set the primal solution column values
      
  colsol[numcols()] is an array of values of the problem column
  variables. These values are copied to memory owned by the
  solver object or the solver.  They will be returned as the
  result of colsol() until changed by another call to
  setColsol() or by a call to any solver routine.  Whether the
  solver makes use of the solution in any way is
  solver-dependent. 
  */
  virtual void setColSolution(const double * colsol);
  
  /** Set dual solution vector
      
  rowprice[numrows()] is an array of values of the problem row
  dual variables. These values are copied to memory owned by the
  solver object or the solver.  They will be returned as the
  result of rowprice() until changed by another call to
  setRowprice() or by a call to any solver routine.  Whether the
  solver makes use of the solution in any way is
  solver-dependent. 
  */
  virtual void setRowPrice(const double * rowprice);
  
  //-------------------------------------------------------------------------
  /**@name Methods to expand a problem.<br>
     Note that if a column is added then by default it will correspond to a
     continuous variable. */
  //@{

  //using OsiSolverInterface::addCol ;
  /** */
  virtual void addCol(const CoinPackedVectorBase& vec,
                      const double collb, const double colub,   
                      const double obj);
  /*! \brief Add a named column (primal variable) to the problem.
  */
  virtual void addCol(const CoinPackedVectorBase& vec,
		      const double collb, const double colub,   
		      const double obj, std::string name) ;
  /** Add a column (primal variable) to the problem. */
  virtual void addCol(int numberElements, const int * rows, const double * elements,
                      const double collb, const double colub,   
                      const double obj) ;
  /*! \brief Add a named column (primal variable) to the problem.
   */
  virtual void addCol(int numberElements,
		      const int* rows, const double* elements,
		      const double collb, const double colub,   
		      const double obj, std::string name) ;
  /** */
  virtual void addCols(const int numcols,
                       const CoinPackedVectorBase * const * cols,
                       const double* collb, const double* colub,   
                       const double* obj);
  /**  */
  virtual void addCols(const int numcols,
		       const int * columnStarts, const int * rows, const double * elements,
		       const double* collb, const double* colub,   
		       const double* obj);
  /** */
  virtual void deleteCols(const int num, const int * colIndices);
  
  /** */
  virtual void addRow(const CoinPackedVectorBase& vec,
                      const double rowlb, const double rowub);
  /** */
    /*! \brief Add a named row (constraint) to the problem.
    
      The default implementation adds the row, then changes the name. This
      can surely be made more efficient within an OsiXXX class.
    */
    virtual void addRow(const CoinPackedVectorBase& vec,
			const double rowlb, const double rowub,
			std::string name) ;
  virtual void addRow(const CoinPackedVectorBase& vec,
                      const char rowsen, const double rowrhs,   
                      const double rowrng);
  /** Add a row (constraint) to the problem. */
  virtual void addRow(int numberElements, const int * columns, const double * element,
		      const double rowlb, const double rowub) ;
    /*! \brief Add a named row (constraint) to the problem.
    */
    virtual void addRow(const CoinPackedVectorBase& vec,
			const char rowsen, const double rowrhs,   
			const double rowrng, std::string name) ;
  /** */
  virtual void addRows(const int numrows,
                       const CoinPackedVectorBase * const * rows,
                       const double* rowlb, const double* rowub);
  /** */
  virtual void addRows(const int numrows,
                       const CoinPackedVectorBase * const * rows,
                       const char* rowsen, const double* rowrhs,   
                       const double* rowrng);

  /** */
  virtual void addRows(const int numrows,
		       const int * rowStarts, const int * columns, const double * element,
		       const double* rowlb, const double* rowub);
  ///
  void modifyCoefficient(int row, int column, double newElement,
			bool keepZero=false)
	{modelPtr_->modifyCoefficient(row,column,newElement, keepZero);}

  /** */
  virtual void deleteRows(const int num, const int * rowIndices);
  /**  If solver wants it can save a copy of "base" (continuous) model here
   */
  virtual void saveBaseModel() ;
  /**  Strip off rows to get to this number of rows.
       If solver wants it can restore a copy of "base" (continuous) model here
  */
  virtual void restoreBaseModel(int numberRows);
  
  //-----------------------------------------------------------------------
  /** Apply a collection of row cuts which are all effective.
      applyCuts seems to do one at a time which seems inefficient.
  */
  virtual void applyRowCuts(int numberCuts, const OsiRowCut * cuts);
  /** Apply a collection of row cuts which are all effective.
      applyCuts seems to do one at a time which seems inefficient.
      This uses array of pointers
  */
  virtual void applyRowCuts(int numberCuts, const OsiRowCut ** cuts);
    /** Apply a collection of cuts.

	Only cuts which have an <code>effectiveness >= effectivenessLb</code>
	are applied.
	<ul>
	  <li> ReturnCode.getNumineffective() -- number of cuts which were
	       not applied because they had an
	       <code>effectiveness < effectivenessLb</code>
	  <li> ReturnCode.getNuminconsistent() -- number of invalid cuts
	  <li> ReturnCode.getNuminconsistentWrtIntegerModel() -- number of
	       cuts that are invalid with respect to this integer model
	  <li> ReturnCode.getNuminfeasible() -- number of cuts that would
	       make this integer model infeasible
	  <li> ReturnCode.getNumApplied() -- number of integer cuts which
	       were applied to the integer model
	  <li> cs.size() == getNumineffective() +
			    getNuminconsistent() +
			    getNuminconsistentWrtIntegerModel() +
			    getNuminfeasible() +
			    getNumApplied()
	</ul>
    */
    virtual ApplyCutsReturnCode applyCuts(const OsiCuts & cs,
					  double effectivenessLb = 0.0);

  //@}
  //@}
  
  //---------------------------------------------------------------------------
  
public:
  
  /**@name Methods to input a problem */
  //@{
  /** Load in an problem by copying the arguments (the constraints on the
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
  virtual void loadProblem(const CoinPackedMatrix& matrix,
                           const double* collb, const double* colub,   
                           const double* obj,
                           const double* rowlb, const double* rowub);
  
  /** Load in an problem by assuming ownership of the arguments (the
      constraints on the rows are given by lower and upper bounds). For
      default values see the previous method.  <br>
      <strong>WARNING</strong>: The arguments passed to this method will be
      freed using the C++ <code>delete</code> and <code>delete[]</code>
      functions. 
  */
  virtual void assignProblem(CoinPackedMatrix*& matrix,
    			     double*& collb, double*& colub, double*& obj,
    			     double*& rowlb, double*& rowub);
  
  /** Load in an problem by copying the arguments (the constraints on the
      rows are given by sense/rhs/range triplets). If a pointer is NULL then the
      following values are the default:
      <ul>
      <li> <code>colub</code>: all columns have upper bound infinity
      <li> <code>collb</code>: all columns have lower bound 0 
      <li> <code>obj</code>: all variables have 0 objective coefficient
      <li> <code>rowsen</code>: all rows are >=
      <li> <code>rowrhs</code>: all right hand sides are 0
      <li> <code>rowrng</code>: 0 for the ranged rows
      </ul>
  */
  virtual void loadProblem(const CoinPackedMatrix& matrix,
    			   const double* collb, const double* colub,
    			   const double* obj,
    			   const char* rowsen, const double* rowrhs,   
    			   const double* rowrng);
  
  /** Load in an problem by assuming ownership of the arguments (the
      constraints on the rows are given by sense/rhs/range triplets). For
      default values see the previous method. <br>
      <strong>WARNING</strong>: The arguments passed to this method will be
      freed using the C++ <code>delete</code> and <code>delete[]</code>
      functions. 
  */
  virtual void assignProblem(CoinPackedMatrix*& matrix,
    			     double*& collb, double*& colub, double*& obj,
    			     char*& rowsen, double*& rowrhs,
    			     double*& rowrng);
  
  /** Just like the other loadProblem() methods except that the matrix is
      given as a ClpMatrixBase. */
  virtual void loadProblem(const ClpMatrixBase& matrix,
			   const double* collb, const double* colub,
			   const double* obj,
			   const double* rowlb, const double* rowub) ;

  /** Just like the other loadProblem() methods except that the matrix is
      given in a standard column major ordered format (without gaps). */
  virtual void loadProblem(const int numcols, const int numrows,
                           const CoinBigIndex * start, const int* index,
                           const double* value,
                           const double* collb, const double* colub,   
                           const double* obj,
                           const double* rowlb, const double* rowub);
  
  /** Just like the other loadProblem() methods except that the matrix is
      given in a standard column major ordered format (without gaps). */
  virtual void loadProblem(const int numcols, const int numrows,
                           const CoinBigIndex * start, const int* index,
                           const double* value,
                           const double* collb, const double* colub,   
                           const double* obj,
                           const char* rowsen, const double* rowrhs,   
                           const double* rowrng);
  /// This loads a model from a coinModel object - returns number of errors
  virtual int loadFromCoinModel (  CoinModel & modelObject, bool keepSolution=false);

  using OsiSolverInterface::readMps ;
  /** Read an mps file from the given filename (defaults to Osi reader) - returns
      number of errors (see OsiMpsReader class) */
  virtual int readMps(const char *filename,
                      const char *extension = "mps") ;
  /** Read an mps file from the given filename returns
      number of errors (see OsiMpsReader class) */
  int readMps(const char *filename,bool keepNames,bool allowErrors);
  /// Read an mps file
  virtual int readMps (const char *filename, const char*extension,
			int & numberSets, CoinSet ** & sets);
  
  /** Write the problem into an mps file of the given filename.
      If objSense is non zero then -1.0 forces the code to write a
      maximization objective and +1.0 to write a minimization one.
      If 0.0 then solver can do what it wants */
  virtual void writeMps(const char *filename,
                        const char *extension = "mps",
                        double objSense=0.0) const;
  /** Write the problem into an mps file of the given filename,
      names may be null.  formatType is
      0 - normal
      1 - extra accuracy 
      2 - IEEE hex (later)
      
      Returns non-zero on I/O error
  */
  virtual int writeMpsNative(const char *filename, 
                             const char ** rowNames, const char ** columnNames,
                             int formatType=0,int numberAcross=2,
                             double objSense=0.0) const ;
  /// Read file in LP format (with names)
  virtual int readLp(const char *filename, const double epsilon = 1e-5);
  /** Write the problem into an Lp file of the given filename.
      If objSense is non zero then -1.0 forces the code to write a
      maximization objective and +1.0 to write a minimization one.
      If 0.0 then solver can do what it wants.
      This version calls writeLpNative with names */
  virtual void writeLp(const char *filename,
                       const char *extension = "lp",
                       double epsilon = 1e-5,
                       int numberAcross = 10,
                       int decimals = 5,
                       double objSense = 0.0,
                       bool useRowNames = true) const;
  /** Write the problem into the file pointed to by the parameter fp. 
      Other parameters are similar to 
      those of writeLp() with first parameter filename.
  */
  virtual void writeLp(FILE *fp,
               double epsilon = 1e-5,
               int numberAcross = 10,
               int decimals = 5,
               double objSense = 0.0,
	       bool useRowNames = true) const;
  /**
     I (JJF) am getting annoyed because I can't just replace a matrix.
     The default behavior of this is do nothing so only use where that would not matter
     e.g. strengthening a matrix for MIP
  */
  virtual void replaceMatrixOptional(const CoinPackedMatrix & matrix);
  /// And if it does matter (not used at present)
  virtual void replaceMatrix(const CoinPackedMatrix & matrix) ;
  //@}
  
  /**@name Message handling (extra for Clp messages).
     Normally I presume you would want the same language.
     If not then you could use underlying model pointer */
  //@{
  /** Pass in a message handler
      
      It is the client's responsibility to destroy a message handler installed
      by this routine; it will not be destroyed when the solver interface is
      destroyed. 
  */
  virtual void passInMessageHandler(CoinMessageHandler * handler);
  /// Set language
  void newLanguage(CoinMessages::Language language);
  void setLanguage(CoinMessages::Language language)
  {newLanguage(language);}
  /// Set log level (will also set underlying solver's log level)
  void setLogLevel(int value);
  /// Create C++ lines to get to current state
  void generateCpp( FILE * fp);
  //@}
  //---------------------------------------------------------------------------
  
  /**@name Clp specific public interfaces */
  //@{
  /// Get pointer to Clp model
  ClpSimplex * getModelPtr() const ;
  /// Set pointer to Clp model and return old
  inline ClpSimplex * swapModelPtr(ClpSimplex * newModel)
  { ClpSimplex * model = modelPtr_; modelPtr_=newModel;return model;}
  /// Get special options
  inline unsigned int specialOptions() const
  { return specialOptions_;}
  void setSpecialOptions(unsigned int value);
  /// Last algorithm used , 1 = primal, 2 = dual other unknown
  inline int lastAlgorithm() const
  { return lastAlgorithm_;}
  /// Set last algorithm used , 1 = primal, 2 = dual other unknown
  inline void setLastAlgorithm(int value)
  { lastAlgorithm_ = value;}
  /// Get scaling action option
  inline int cleanupScaling() const
  { return cleanupScaling_;}
  /** Set Scaling option
      When scaling is on it is possible that the scaled problem
      is feasible but the unscaled is not.  Clp returns a secondary
      status code to that effect.  This option allows for a cleanup.
      If you use it I would suggest 1.
      This only affects actions when scaled optimal
      0 - no action
      1 - clean up using dual if primal infeasibility
      2 - clean up using dual if dual infeasibility
      3 - clean up using dual if primal or dual infeasibility
      11,12,13 - as 1,2,3 but use primal
  */
  inline void setCleanupScaling(int value)
  { cleanupScaling_=value;}
  /** Get smallest allowed element in cut.
      If smaller than this then ignored */
  inline double smallestElementInCut() const
  { return smallestElementInCut_;}
  /** Set smallest allowed element in cut.
      If smaller than this then ignored */
  inline void setSmallestElementInCut(double value)
  { smallestElementInCut_=value;}
  /** Get smallest change in cut.
      If (upper-lower)*element < this then element is
      taken out and cut relaxed. 
      (upper-lower) is taken to be at least 1.0 and
      this is assumed >= smallestElementInCut_
  */
  inline double smallestChangeInCut() const
  { return smallestChangeInCut_;}
  /** Set smallest change in cut.
      If (upper-lower)*element < this then element is
      taken out and cut relaxed. 
      (upper-lower) is taken to be at least 1.0 and
      this is assumed >= smallestElementInCut_
  */
  inline void setSmallestChangeInCut(double value)
  { smallestChangeInCut_=value;}
  /// Pass in initial solve options
  inline void setSolveOptions(const ClpSolve & options)
  { solveOptions_ = options;}
  /** Tighten bounds - lightweight or very lightweight
      0 - normal, 1 lightweight but just integers, 2 lightweight and all
  */
  virtual int tightenBounds(int lightweight=0);
  /// Return number of entries in L part of current factorization
  virtual CoinBigIndex getSizeL() const;
  /// Return number of entries in U part of current factorization
  virtual CoinBigIndex getSizeU() const;
  /// Get disaster handler
  const OsiClpDisasterHandler * disasterHandler() const
  { return disasterHandler_;}
  /// Pass in disaster handler
  void passInDisasterHandler(OsiClpDisasterHandler * handler);
  /// Get fake objective
  ClpLinearObjective * fakeObjective() const
  { return fakeObjective_;}
  /// Set fake objective (and take ownership)
  void setFakeObjective(ClpLinearObjective * fakeObjective);
  /// Set fake objective
  void setFakeObjective(double * fakeObjective);
  /*! \brief Set up solver for repeated use by Osi interface.

    The normal usage does things like keeping factorization around so can be
    used.  Will also do things like keep scaling and row copy of matrix if
    matrix does not change.

    \p senseOfAdventure:
    - 0 - safe stuff as above
    - 1 - will take more risks - if it does not work then bug which will be
          fixed
    - 2 - don't bother doing most extreme termination checks e.g. don't bother
	  re-factorizing if less than 20 iterations.
    - 3 - Actually safer than 1 (mainly just keeps factorization)
      
    \p printOut
    - -1 always skip round common messages instead of doing some work
    -  0 skip if normal defaults
    -  1 leaves
  */
  void setupForRepeatedUse(int senseOfAdventure=0, int printOut=0);
  /// Synchronize model (really if no cuts in tree)
  virtual void synchronizeModel();
  /*! \brief Set special options in underlying clp solver.

    Safe as const because #modelPtr_ is mutable.
  */
  void setSpecialOptionsMutable(unsigned int value) const;

  //@}
  
  //---------------------------------------------------------------------------
  
  /**@name Constructors and destructors */
  //@{
  /// Default Constructor
  OsiClpSolverInterface ();
  
  /// Clone
  virtual OsiSolverInterface * clone(bool copyData = true) const;
  
  /// Copy constructor 
  OsiClpSolverInterface (const OsiClpSolverInterface &);
  
  /// Borrow constructor - only delete one copy
  OsiClpSolverInterface (ClpSimplex * rhs, bool reallyOwn=false);
  
  /// Releases so won't error
  void releaseClp();
  
  /// Assignment operator 
  OsiClpSolverInterface & operator=(const OsiClpSolverInterface& rhs);
  
  /// Destructor 
  virtual ~OsiClpSolverInterface ();
  
  /// Resets as if default constructor
  virtual void reset();
  //@}
  
  //---------------------------------------------------------------------------
  
protected:
  ///@name Protected methods
  //@{
  /** Apply a row cut (append to constraint matrix). */
  virtual void applyRowCut(const OsiRowCut& rc);
  
  /** Apply a column cut (adjust one or more bounds). */
  virtual void applyColCut(const OsiColCut& cc);
  //@}
  
  //---------------------------------------------------------------------------
  
protected:
  /**@name Protected methods */
  //@{
  /// The real work of a copy constructor (used by copy and assignment)
  void gutsOfDestructor();
  
  /// Deletes all mutable stuff
  void freeCachedResults() const;
  
  /// Deletes all mutable stuff for row ranges etc
  void freeCachedResults0() const;
  
  /// Deletes all mutable stuff for matrix etc
  void freeCachedResults1() const;
  
  /// A method that fills up the rowsense_, rhs_ and rowrange_ arrays
  void extractSenseRhsRange() const;
  
  ///
  void fillParamMaps();
  /** Warm start
      
  NOTE  artificials are treated as +1 elements so for <= rhs
  artificial will be at lower bound if constraint is tight
  
  This means that Clpsimplex flips artificials as it works
  in terms of row activities
  */
  CoinWarmStartBasis getBasis(ClpSimplex * model) const;
  /** Sets up working basis as a copy of input
      
  NOTE  artificials are treated as +1 elements so for <= rhs
  artificial will be at lower bound if constraint is tight
  
  This means that Clpsimplex flips artificials as it works
  in terms of row activities
  */
  void setBasis( const CoinWarmStartBasis & basis, ClpSimplex * model);
  /// Crunch down problem a bit
  void crunch();
  /// Extend scale factors
  void redoScaleFactors(int numberRows,const CoinBigIndex * starts,
			const int * indices, const double * elements);
public:
  /** Sets up working basis as a copy of input and puts in as basis
  */
  void setBasis( const CoinWarmStartBasis & basis);
  /// Just puts current basis_ into ClpSimplex model
  inline void setBasis( )
  { setBasis(basis_,modelPtr_);}
  /// Warm start difference from basis_ to statusArray
  CoinWarmStartDiff * getBasisDiff(const unsigned char * statusArray) const ;
  /// Warm start from statusArray
  CoinWarmStartBasis * getBasis(const unsigned char * statusArray) const ;
  /// Delete all scale factor stuff and reset option
  void deleteScaleFactors();
  /// If doing fast hot start then ranges are computed
  inline const double * upRange() const
  { return rowActivity_;}
  inline const double * downRange() const
  { return columnActivity_;}
  /// Pass in range array
  inline void passInRanges(int * array)
  { whichRange_=array;}
  /// Pass in sos stuff from AMPl
  void setSOSData(int numberSOS,const char * type,
		  const int * start,const int * indices, const double * weights=NULL);
  /// Compute largest amount any at continuous away from bound
  void computeLargestAway();
  /// Get largest amount continuous away from bound
  inline double largestAway() const
  { return largestAway_;}
  /// Set largest amount continuous away from bound
  inline void setLargestAway(double value)
  { largestAway_ = value;}
  /// Sort of lexicographic resolve
  void lexSolve();
  //@}
  
protected:
  /**@name Protected member data */
  //@{
  /// Clp model represented by this class instance
  mutable ClpSimplex * modelPtr_;
  //@}
  /**@name Cached information derived from the OSL model */
  //@{
  /// Pointer to dense vector of row sense indicators
  mutable char    *rowsense_;
  
  /// Pointer to dense vector of row right-hand side values
  mutable double  *rhs_;
  
  /** Pointer to dense vector of slack upper bounds for range 
      constraints (undefined for non-range rows)
  */
  mutable double  *rowrange_;
  
  /** A pointer to the warmstart information to be used in the hotstarts.
      This is NOT efficient and more thought should be given to it... */
  mutable CoinWarmStartBasis* ws_;
  /** also save row and column information for hot starts
      only used in hotstarts so can be casual */
  mutable double * rowActivity_;
  mutable double * columnActivity_;
  /// Stuff for fast dual
  ClpNodeStuff stuff_;
  /// Number of SOS sets
  int numberSOS_;
  /// SOS set info
  CoinSet * setInfo_;
  /// Alternate model (hot starts) - but also could be permanent and used for crunch
  ClpSimplex * smallModel_;
  /// factorization for hot starts
  ClpFactorization * factorization_;
  /** Smallest allowed element in cut.
      If smaller than this then ignored */
  double smallestElementInCut_;
  /** Smallest change in cut.
      If (upper-lower)*element < this then element is
      taken out and cut relaxed. */
  double smallestChangeInCut_;
  /// Largest amount continuous away from bound
  double largestAway_;
  /// Arrays for hot starts
  char * spareArrays_;
  /** Warmstart information to be used in resolves. */
  CoinWarmStartBasis basis_;
  /** The original iteration limit before hotstarts started. */
  int itlimOrig_;
  
  /*! \brief Last algorithm used
  
    Coded as
    -    0 invalid
    -    1 primal
    -    2 dual
    - -911 disaster in the algorithm that was attempted
    -  999 current solution no longer optimal due to change in problem or
           basis
  */
  mutable int lastAlgorithm_;
  
  /// To say if destructor should delete underlying model
  bool notOwned_;
  
  /// Pointer to row-wise copy of problem matrix coefficients.
  mutable CoinPackedMatrix *matrixByRow_;  
  
  /// Pointer to row-wise copy of continuous problem matrix coefficients.
  CoinPackedMatrix *matrixByRowAtContinuous_;  
  
  /// Pointer to integer information
  char * integerInformation_;
  
  /** Pointer to variables for which we want range information
      The number is in [0]
      memory is not owned by OsiClp
  */
  int * whichRange_;

  //std::map<OsiIntParam, ClpIntParam> intParamMap_;
  //std::map<OsiDblParam, ClpDblParam> dblParamMap_;
  //std::map<OsiStrParam, ClpStrParam> strParamMap_;
  
  /*! \brief Faking min to get proper dual solution signs in simplex API */
  mutable bool fakeMinInSimplex_ ;
  /*! \brief Linear objective
  
    Normally a pointer to the linear coefficient array in the clp objective.
    An independent copy when #fakeMinInSimplex_ is true, because we need
    something permanent to point to when #getObjCoefficients is called.
  */
  mutable double *linearObjective_;

  /// To save data in OsiSimplex stuff
  mutable ClpDataSave saveData_;
  /// Options for initialSolve
  ClpSolve solveOptions_;
  /** Scaling option
      When scaling is on it is possible that the scaled problem
      is feasible but the unscaled is not.  Clp returns a secondary
      status code to that effect.  This option allows for a cleanup.
      If you use it I would suggest 1.
      This only affects actions when scaled optimal
      0 - no action
      1 - clean up using dual if primal infeasibility
      2 - clean up using dual if dual infeasibility
      3 - clean up using dual if primal or dual infeasibility
      11,12,13 - as 1,2,3 but use primal
  */
  int cleanupScaling_;
  /** Special options
      0x80000000 off
      0 simple stuff for branch and bound
      1 try and keep work regions as much as possible
      2 do not use any perturbation
      4 allow exit before re-factorization
      8 try and re-use factorization if no cuts
      16 use standard strong branching rather than clp's
      32 Just go to first factorization in fast dual
      64 try and tighten bounds in crunch
      128 Model will only change in column bounds
      256 Clean up model before hot start
      512 Give user direct access to Clp regions in getBInvARow etc (i.e.,
          do not unscale, and do not return result in getBInv parameters;
	  you have to know where to look for the answer)
      1024 Don't "borrow" model in initialSolve
      2048 Don't crunch
      4096 quick check for optimality
      Bits above 8192 give where called from in Cbc
      At present 0 is normal, 1 doing fast hotstarts, 2 is can do quick check
      65536 Keep simple i.e. no  crunch etc
      131072 Try and keep scaling factors around
      262144 Don't try and tighten bounds (funny global cuts)
      524288 Fake objective and 0-1
      1048576 Don't recompute ray after crunch
      2097152 
  */
  mutable unsigned int specialOptions_;
  /// Copy of model when option 131072 set
  ClpSimplex * baseModel_;
  /// Number of rows when last "scaled"
  int lastNumberRows_;
  /// Continuous model
  ClpSimplex * continuousModel_;
  /// Possible disaster handler
  OsiClpDisasterHandler * disasterHandler_ ;
  /// Fake objective
  ClpLinearObjective * fakeObjective_;
  /// Row scale factors (has inverse at end)
  CoinDoubleArrayWithLength rowScale_; 
  /// Column scale factors (has inverse at end)
  CoinDoubleArrayWithLength columnScale_; 
  //@}
};
  
class OsiClpDisasterHandler : public ClpDisasterHandler {
public:
  /**@name Virtual methods that the derived classe should provide.
  */
  //@{
  /// Into simplex
  virtual void intoSimplex();
  /// Checks if disaster
  virtual bool check() const ;
  /// saves information for next attempt
  virtual void saveInfo();
  /// Type of disaster 0 can fix, 1 abort
  virtual int typeOfDisaster();
  //@}
  
  
  /**@name Constructors, destructor */

  //@{
  /** Default constructor. */
  OsiClpDisasterHandler(OsiClpSolverInterface * model = NULL);
  /** Destructor */
  virtual ~OsiClpDisasterHandler();
  // Copy
  OsiClpDisasterHandler(const OsiClpDisasterHandler&);
  // Assignment
  OsiClpDisasterHandler& operator=(const OsiClpDisasterHandler&);
  /// Clone
  virtual ClpDisasterHandler * clone() const;

  //@}
  
  /**@name Sets/gets */

  //@{
  /** set model. */
  void setOsiModel(OsiClpSolverInterface * model);
  /// Get model
  inline OsiClpSolverInterface * osiModel() const
  { return osiModel_;}
  /// Set where from
  inline void setWhereFrom(int value)
  { whereFrom_=value;}
  /// Get where from
  inline int whereFrom() const
  { return whereFrom_;}
  /// Set phase 
  inline void setPhase(int value)
  { phase_=value;}
  /// Get phase 
  inline int phase() const
  { return phase_;}
  /// are we in trouble
  inline bool inTrouble() const
  { return inTrouble_;}
  
  //@}
  
  
protected:
  /**@name Data members
     The data members are protected to allow access for derived classes. */
  //@{
  /// Pointer to model
  OsiClpSolverInterface * osiModel_;
  /** Where from 
      0 dual (resolve)
      1 crunch
      2 primal (resolve)
      4 dual (initialSolve)
      6 primal (initialSolve)
  */
  int whereFrom_;
  /** phase
      0 initial
      1 trying continuing with back in and maybe different perturb
      2 trying continuing with back in and different scaling
      3 trying dual from all slack
      4 trying primal from previous stored basis
  */
  int phase_;
  /// Are we in trouble
  bool inTrouble_;
  //@}
};
// So unit test can find out if NDEBUG set
bool OsiClpHasNDEBUG();
//#############################################################################
/** A function that tests the methods in the OsiClpSolverInterface class. */
void OsiClpSolverInterfaceUnitTest(const std::string & mpsDir, const std::string & netlibDir);
#endif
