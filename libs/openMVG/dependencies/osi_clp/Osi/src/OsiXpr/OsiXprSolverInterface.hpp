// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef OsiXprSolverInterface_H
#define OsiXprSolverInterface_H

#include <string>
#include <cstdio>

#include "OsiSolverInterface.hpp"

typedef struct xo_prob_struct* XPRSprob;

//#############################################################################

/** XPRESS-MP Solver Interface

    Instantiation of OsiSolverInterface for XPRESS-MP
 */
class OsiXprSolverInterface : virtual public OsiSolverInterface {
   friend void OsiXprSolverInterfaceUnitTest(const std::string & mpsDir, const std::string & netlibDir);
public:
  /**@name Solve methods */
  //@{
    /// Solve initial LP relaxation 
    virtual void initialSolve();

    /// Resolve an LP relaxation after problem modification
    virtual void resolve();

    /// Invoke solver's built-in enumeration algorithm
    virtual void branchAndBound();
  //@}
  
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
    // Set mipstart option (pass column solution to XPRESS before MIP start)
    void setMipStart(bool value) { domipstart = value; }
    // Get mipstart option value
    bool getMipStart() const { return domipstart; }
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
    /// Get empty warm start object
    CoinWarmStart *getEmptyWarmStart () const;
    /// Get warmstarting information
    virtual CoinWarmStart* getWarmStart() const;
    /** Set warmstarting information. Return true/false depending on whether
	the warmstart information was accepted or not. */
    virtual bool setWarmStart(const CoinWarmStart* warmstart);
  //@}

  //---------------------------------------------------------------------------
  /**@name Hotstart related methods (primarily used in strong branching). <br>
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
      virtual int getNumCols() const;
  
      /// Get number of rows
      virtual int getNumRows() const;
  
      /// Get number of nonzero elements
      virtual int getNumElements() const;
  
      /// Get pointer to array[getNumCols()] of column lower bounds
      virtual const double * getColLower() const;
  
      /// Get pointer to array[getNumCols()] of column upper bounds
      virtual const double * getColUpper() const;
  
      /** Get pointer to array[getNumRows()] of row constraint senses.
  	<ul>
  	<li>'L': <= constraint
  	<li>'E': =  constraint
  	<li>'G': >= constraint
  	<li>'R': ranged constraint
  	<li>'N': free constraint
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
      virtual const double * getRightHandSide() const;
  
      /** Get pointer to array[getNumRows()] of row ranges.
  	<ul>
            <li> if rowsense()[i] == 'R' then
                    rowrange()[i] == rowupper()[i] - rowlower()[i]
            <li> if rowsense()[i] != 'R' then
                    rowrange()[i] is 0.0
          </ul>
      */
      virtual const double * getRowRange() const;
  
      /// Get pointer to array[getNumRows()] of row lower bounds
      virtual const double * getRowLower() const;
  
      /// Get pointer to array[getNumRows()] of row upper bounds
      virtual const double * getRowUpper() const;
  
      /// Get pointer to array[getNumCols()] of objective function coefficients
      virtual const double * getObjCoefficients() const;
  
      /// Get objective function sense (1 for min (default), -1 for max)
      virtual double getObjSense() const;
  
      /// Return true if variable is continuous
      virtual bool isContinuous(int colIndex) const;
  
#if 0
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
#endif
      /// Get pointer to row-wise copy of matrix
      virtual const CoinPackedMatrix * getMatrixByRow() const;
  
      /// Get pointer to column-wise copy of matrix
      virtual const CoinPackedMatrix * getMatrixByCol() const;
  
      /// Get solver's value for infinity
      virtual double getInfinity() const;
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
      virtual int getIterationCount() const;
  
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
					       bool fullRay=false) const;
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
  
#if 0
      /** Get vector of indices of solution which are integer variables 
  	presently at fractional values */
      virtual OsiVectorInt getFractionalIndices(const double etol=1.e-05)
	const;
#endif
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
    	  Use -COIN_DBL_MAX for -infinity. */
      virtual void setColLower( int elementIndex, double elementValue );
      
      /** Set a single column upper bound<br>
    	  Use COIN_DBL_MAX for infinity. */
      virtual void setColUpper( int elementIndex, double elementValue );
      
      /** Set a single column lower and upper bound<br>
    	  The default implementation just invokes setColLower() and
    	  setColUpper() */
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
    	  Use -COIN_DBL_MAX for -infinity. */
      virtual void setRowLower( int elementIndex, double elementValue );
      
      /** Set a single row upper bound<br>
    	  Use COIN_DBL_MAX for infinity. */
      virtual void setRowUpper( int elementIndex, double elementValue );
    
      /** Set a single row lower and upper bound<br>
    	  The default implementation just invokes setRowLower() and
    	  setRowUpper() */
      virtual void setRowBounds( int elementIndex,
    				 double lower, double upper );
    
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
    //@}
    
    //-------------------------------------------------------------------------
    /// Set objective function sense (1 for min (default), -1 for max,)
    virtual void setObjSense(double s);
    
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
      /** */
      virtual void addCol(const CoinPackedVectorBase& vec,
			  const double collb, const double colub,   
			  const double obj);
      /** */
      virtual void addCols(const int numcols,
			   const CoinPackedVectorBase * const * cols,
			   const double* collb, const double* colub,   
			   const double* obj);
      /** */
      virtual void deleteCols(const int num, const int * colIndices);
    
      /** */
      virtual void addRow(const CoinPackedVectorBase& vec,
    			  const double rowlb, const double rowub);
      /** */
      virtual void addRow(const CoinPackedVectorBase& vec,
    			  const char rowsen, const double rowrhs,   
    			  const double rowrng);
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
      virtual void deleteRows(const int num, const int * rowIndices);
#if 0
      //-----------------------------------------------------------------------
      /** Apply a collection of cuts.<br>
    	  Only cuts which have an <code>effectiveness >= effectivenessLb</code>
    	  are applied.
    	  <ul>
    	    <li> ReturnCode.numberIneffective() -- number of cuts which were
                 not applied because they had an
    	         <code>effectiveness < effectivenessLb</code>
    	    <li> ReturnCode.numberInconsistent() -- number of invalid cuts
    	    <li> ReturnCode.numberInconsistentWrtIntegerModel() -- number of
                 cuts that are invalid with respect to this integer model
            <li> ReturnCode.numberInfeasible() -- number of cuts that would
    	         make this integer model infeasible
            <li> ReturnCode.numberApplied() -- number of integer cuts which
    	         were applied to the integer model
            <li> cs.size() == numberIneffective() +
                              numberInconsistent() +
    			      numberInconsistentWrtIntegerModel() +
    			      numberInfeasible() +
    			      nubmerApplied()
          </ul>
      */
      virtual ApplyCutsReturnCode applyCuts(const OsiCuts & cs,
    					    double effectivenessLb = 0.0);
    //@}
  //@}
#endif
  //---------------------------------------------------------------------------

  /**@name Methods to input a problem */
  //@{
    /** Load in an problem by copying the arguments (the constraints on the
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
    virtual void loadProblem(const CoinPackedMatrix& matrix,
			     const double* collb, const double* colub,   
			     const double* obj,
			     const double* rowlb, const double* rowub);
			    
    /** Load in an problem by assuming ownership of the arguments (the
        constraints on the rows are given by lower and upper bounds). For
        default values see the previous method. <br>
	<strong>WARNING</strong>: The arguments passed to this method will be
	freed using the C++ <code>delete</code> and <code>delete[]</code>
	functions. 
    */
    virtual void assignProblem(CoinPackedMatrix*& matrix,
			       double*& collb, double*& colub, double*& obj,
			       double*& rowlb, double*& rowub);

    /** Load in an problem by copying the arguments (the constraints on the
	rows are given by sense/rhs/range triplets). If a pointer is 0 then the
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
	given in a standard column major ordered format (without gaps). */
    virtual void loadProblem(const int numcols, const int numrows,
			     const int* start, const int* index,
			     const double* value,
			     const double* collb, const double* colub,   
			     const double* obj,
			     const double* rowlb, const double* rowub);

    /** Just like the other loadProblem() methods except that the matrix is
	given in a standard column major ordered format (without gaps). */
    virtual void loadProblem(const int numcols, const int numrows,
			     const int* start, const int* index,
			     const double* value,
			     const double* collb, const double* colub,   
			     const double* obj,
			     const char* rowsen, const double* rowrhs,   
			     const double* rowrng);


    /** Read an mps file from the given filename */
    virtual int readMps(const char *filename,
			 const char *extension = "mps");


    /** Write the problem into an mps file of the given filename.
     If objSense is non zero then -1.0 forces the code to write a
    maximization objective and +1.0 to write a minimization one.
    If 0.0 then solver can do what it wants */
    virtual void writeMps(const char *filename,
			  const char *extension = "mps",
			  double objSense=0.0) const;
  //@}

    /**@name Message handling */
    //@{
    /** Pass in a message handler
        It is the client's responsibility to destroy a message handler installed
        by this routine; it will not be destroyed when the solver interface is
        destroyed. 
     */
    void passInMessageHandler(CoinMessageHandler * handler);
    //@}

  //---------------------------------------------------------------------------

  /**@name XPRESS specific public interfaces */
  //@{
    /**@name Static instance counter methods */
    //@{
      /** XPRESS has a context that must be created prior to all other XPRESS
	  calls.
        This method:
        <ul>
        <li>Increments by 1 the number of uses of the XPRESS environment.
        <li>Creates the XPRESS context when the number of uses is changed to 1
	    from 0. 
        </ul>
      */
      static  void incrementInstanceCounter();

      /** XPRESS has a context that should be deleted after XPRESS calls.
        This method:
        <ul>
        <li>Decrements by 1 the number of uses of the XPRESS environment.
        <li>Deletes the XPRESS context when the number of uses is change to
        0 from 1.
        </ul>
      */
      static  void decrementInstanceCounter();

      /** Return the number of instances of instantiated objects using XPRESS
	  services. */
      static  unsigned int getNumInstances();
      
      /** Return a pointer to the XPRESS problem */
      XPRSprob getLpPtr() { return prob_; }
    //@}

    /// Return XPRESS-MP Version number
    static int version();

    /**@name Log File  */
    //@{
    static int iXprCallCount_;


      /// Get logfile FILE *
      static FILE * getLogFilePtr();
      /**Set logfile name. The logfile is an attempt to 
	 capture the calls to Xpress functions for debugging. */
      static void setLogFileName( const char * filename );
    //@}
  //@}

  /**@name Constructors and destructors */
  //@{
    /// Default Constructor
    OsiXprSolverInterface (int newrows = 50, int newnz = 100);

    /// Clone
    virtual OsiSolverInterface * clone(bool copyData = true) const;

    /// Copy constructor 
    OsiXprSolverInterface (const OsiXprSolverInterface &);

    /// Assignment operator 
    OsiXprSolverInterface & operator=(const OsiXprSolverInterface& rhs);

    /// Destructor 
    virtual ~OsiXprSolverInterface ();
  //@}

protected:

  /**@name Protected methods */
  //@{
    /// Apply a row cut.  Return true if cut was applied.
    virtual void applyRowCut( const OsiRowCut & rc );

    /** Apply a column cut (bound adjustment). 
      Return true if cut was applied.
    */
    virtual void applyColCut( const OsiColCut & cc );
  //@}

private:  
  
  /**@name Private static class data */
  //@{
    /// Name of the logfile
    static const char * logFileName_;

    /// The FILE* to the logfile
    static FILE * logFilePtr_;

    /// Number of live problem instances
    static  unsigned int numInstances_;

    /// Counts calls to incrementInstanceCounter()
    static  unsigned int osiSerial_;

  //@}

  /**@name Private methods */
  //@{
    /// The real work of a copy constructor (used by copy and assignment)
    void gutsOfCopy( const OsiXprSolverInterface & source );

    /// The real work of a constructor (used by construct and assignment)
    void gutsOfConstructor(); 

    /// The real work of a destructor (used by copy and assignment)
    void gutsOfDestructor(); 

    /// Destroy cached copy of solution data (whenever it changes)
    void freeSolution();

    /** Destroy cached copies of problem and solution data (whenever they
	change) */
    void freeCachedResults();

    /// Number of integer variables in the problem
    int getNumIntVars() const;

    /**@name Methods to support for XPRESS-MP multiple matrix facility */
    //@{
      /// Build cached copy of variable types
      void getVarTypes() const;

      /** Save the current problem in XPRESS (if necessary) and 
          make this problem current (restore if necessary).
      */
      void    activateMe() const;

      /** Save and restore are necessary if there is data associated with
          this problem.  Also, queries to a problem with no data should 
          respond sensibly; XPRESS query results are undefined.
      */
      bool    isDataLoaded() const;
    //@}
  //@}

  /**@name Private member data */
  //@{

    /**@name Data to support for XPRESS-MP multiple matrix facility */
    //@{
    
    mutable XPRSprob prob_;
    
      /// XPRESS problem name (should be unique for each saved problem)
      mutable std::string  xprProbname_;
    //@}

    /**@name Cached copies of XPRESS-MP problem data */
    //@{
      /** Pointer to row-wise copy of problem matrix coefficients.<br>
          Note that XPRESS keeps the objective row in the 
          problem matrix, so row indices and counts are adjusted 
          accordingly.
      */
      mutable CoinPackedMatrix *matrixByRow_;
      mutable CoinPackedMatrix *matrixByCol_;

      /// Pointer to dense vector of structural variable upper bounds
      mutable double  *colupper_;

      /// Pointer to dense vector of structural variable lower bounds
      mutable double  *collower_;

      /// Pointer to dense vector of slack variable upper bounds
      mutable double  *rowupper_;

      /// Pointer to dense vector of slack variable lower bounds
      mutable double  *rowlower_;

      /// Pointer to dense vector of row sense indicators
      mutable char    *rowsense_;

      /// Pointer to dense vector of row right-hand side values
      mutable double  *rhs_;

      /** Pointer to dense vector of slack upper bounds for range 
          constraints (undefined for non-range rows)
      */
      mutable double  *rowrange_;

      /// Pointer to dense vector of objective coefficients
      mutable double  *objcoeffs_;

      /// Sense of objective (1 for min; -1 for max)
      mutable double  objsense_;

      /// Pointer to dense vector of primal structural variable values
      mutable double  *colsol_;

      /// Pointer to dense vector of primal slack variable values
      mutable double  *rowsol_;

      /// Pointer to dense vector of primal slack variable values
      mutable double  *rowact_;

      /// Pointer to dense vector of dual row variable values
      mutable double  *rowprice_;

      /// Pointer to dense vector of dual column variable values
      mutable double  *colprice_;

      /// Pointer to list of indices of XPRESS "global" variables
      mutable int     *ivarind_;

      /** Pointer to list of global variable types:
      <ul>
      <li>'B': binary variable
      <li>'I': general integer variable (but might have 0-1  bounds)
      <li>'P': partial integer variable (not currently supported)
      <li>'S': sem-continuous variable (not currently supported)
      </ul>
      */
      mutable char    *ivartype_;

      /** Pointer to dense vector of variable types 
          (as above, or 'C' for continuous)
      */
      mutable char    *vartype_;

      /** Indicates whether the last solve was for a MIP or an LP. */
      mutable bool lastsolvewasmip;
    //@}
  //@}

      /// Whether to pass a column solution to XPRESS before starting MIP solve (loadmipsol)
      bool            domipstart;
};

//#############################################################################
/** A function that tests the methods in the OsiXprSolverInterface class. */
void OsiXprSolverInterfaceUnitTest(const std::string & mpsDir, const std::string & netlibDir);

#endif
