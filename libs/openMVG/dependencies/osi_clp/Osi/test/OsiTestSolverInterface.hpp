// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This file is licensed under the terms of Eclipse Public License (EPL).

// this is a copy of OsiVolSolverInterface (trunk rev. 1466) renamed to OsiTestSolverInterface

#ifndef OsiTestSolverInterface_H
#define OsiTestSolverInterface_H

#include <string>

#include "OsiTestSolver.hpp"

#include "CoinPackedMatrix.hpp"

#include "OsiSolverInterface.hpp"

static const double OsiTestInfinity = 1.0e31;

//#############################################################################

/** Vol(ume) Solver Interface

    Instantiation of OsiTestSolverInterface for the Volume Algorithm
*/

class OsiTestSolverInterface :
   virtual public OsiSolverInterface, public VOL_user_hooks {
   friend void OsiTestSolverInterfaceUnitTest(const std::string & mpsDir, const std::string & netlibDir);

private:
  class OsiVolMatrixOneMinusOne_ {
    int majorDim_;
    int minorDim_;

    int plusSize_;
    int * plusInd_;
    int * plusStart_;
    int * plusLength_;

    int minusSize_;
    int * minusInd_;
    int * minusStart_;
    int * minusLength_;

  public:
    OsiVolMatrixOneMinusOne_(const CoinPackedMatrix& m);
    ~OsiVolMatrixOneMinusOne_();
    void timesMajor(const double* x, double* y) const;
  };

public:
  //---------------------------------------------------------------------------
  /**@name Solve methods */
  //@{
    /// Solve initial LP relaxation 
    virtual void initialSolve();

    /// Resolve an LP relaxation after problem modification
    virtual void resolve();

    /// Invoke solver's built-in enumeration algorithm
    virtual void branchAndBound() {
      throw CoinError("Sorry, the Volume Algorithm doesn't implement B&B",
		     "branchAndBound", "OsiTestSolverInterface");
    }
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
      
      This routine returns an empty warm start object. Its purpose is
      to provide a way to give a client a warm start object of the
      appropriate type, which can resized and modified as desired.
    */
    virtual CoinWarmStart *getEmptyWarmStart () const ;

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
      virtual int getNumCols() const {
        return rowMatrixCurrent_?
  	rowMatrix_.getNumCols() : colMatrix_.getNumCols(); }
  
      /// Get number of rows
      virtual int getNumRows() const {
        return rowMatrixCurrent_?
  	rowMatrix_.getNumRows() : colMatrix_.getNumRows(); }
  
      /// Get number of nonzero elements
      virtual int getNumElements() const {
        return rowMatrixCurrent_?
  	rowMatrix_.getNumElements() : colMatrix_.getNumElements(); }
  
      /// Get pointer to array[getNumCols()] of column lower bounds
      virtual const double * getColLower() const { return collower_; }
  
      /// Get pointer to array[getNumCols()] of column upper bounds
      virtual const double * getColUpper() const { return colupper_; }
  
      /** Get pointer to array[getNumRows()] of row constraint senses.
  	<ul>
  	<li>'L' <= constraint
  	<li>'E' =  constraint
  	<li>'G' >= constraint
  	<li>'R' ranged constraint
  	<li>'N' free constraint
  	</ul>
      */
      virtual const char * getRowSense() const { return rowsense_; }
  
      /** Get pointer to array[getNumRows()] of rows right-hand sides
          <ul>
  	  <li> if rowsense()[i] == 'L' then rhs()[i] == rowupper()[i]
  	  <li> if rowsense()[i] == 'G' then rhs()[i] == rowlower()[i]
  	  <li> if rowsense()[i] == 'R' then rhs()[i] == rowupper()[i]
  	  <li> if rowsense()[i] == 'N' then rhs()[i] == 0.0
          </ul>
      */
      virtual const double * getRightHandSide() const { return rhs_; }
  
      /** Get pointer to array[getNumRows()] of row ranges.
  	<ul>
            <li> if rowsense()[i] == 'R' then
                    rowrange()[i] == rowupper()[i] - rowlower()[i]
            <li> if rowsense()[i] != 'R' then
                    rowrange()[i] is undefined
          </ul>
      */
      virtual const double * getRowRange() const { return rowrange_; }
  
      /// Get pointer to array[getNumRows()] of row lower bounds
      virtual const double * getRowLower() const { return rowlower_; }
  
      /// Get pointer to array[getNumRows()] of row upper bounds
      virtual const double * getRowUpper() const { return rowupper_; }
  
      /// Get pointer to array[getNumCols()] of objective function coefficients
      virtual const double * getObjCoefficients() const { return objcoeffs_; }
  
      /// Get objective function sense (1 for min (default), -1 for max)
      virtual double getObjSense() const { return objsense_; }
  
       /// Return true if column is continuous
      virtual bool isContinuous(int colNumber) const;
  
#if 0
      /// Return true if column is binary
      virtual bool isBinary(int colNumber) const;
  
      /** Return true if column is integer.
          Note: This function returns true if the the column
          is binary or a general integer.
      */
      virtual bool isInteger(int colNumber) const;
  
      /// Return true if column is general integer
      virtual bool isIntegerNonBinary(int colNumber) const;
  
      /// Return true if column is binary and not fixed at either bound
      virtual bool isFreeBinary(int colNumber) const;
#endif
  
      /// Get pointer to row-wise copy of matrix
      virtual const CoinPackedMatrix * getMatrixByRow() const;
  
      /// Get pointer to column-wise copy of matrix
      virtual const CoinPackedMatrix * getMatrixByCol() const;
  
      /// Get solver's value for infinity
      virtual double getInfinity() const { return OsiTestInfinity; }
    //@}
    
    /**@name Methods related to querying the solution */
    //@{
      /// Get pointer to array[getNumCols()] of primal solution vector
      virtual const double * getColSolution() const { return colsol_; }
  
      /// Get pointer to array[getNumRows()] of dual prices
      virtual const double * getRowPrice() const { return rowprice_; }
  
      /// Get a pointer to array[getNumCols()] of reduced costs
      virtual const double * getReducedCost() const { return rc_; }
  
      /** Get pointer to array[getNumRows()] of row activity levels (constraint
  	matrix times the solution vector */
      virtual const double * getRowActivity() const { return lhs_; }
  
      /// Get objective function value
      virtual double getObjValue() const { 
#if 1
        // This does not pass unitTest if getObjValue is called before solve
        return lagrangeanCost_;
#else
        return OsiSolverInterface::getObjValue();
#endif
      }
  
      /** Get how many iterations it took to solve the problem (whatever
	  "iteration" mean to the solver. */
      virtual int getIterationCount() const { return volprob_.iter(); }
  
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
  
#if 0
      /** Get indices of solution vector which are integer variables 
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
      virtual void setObjCoeff( int elementIndex, double elementValue ) {
	objcoeffs_[elementIndex] = elementValue;
      }

      using OsiSolverInterface::setColLower ;
      /** Set a single column lower bound<br>
    	  Use -COIN_DBL_MAX for -infinity. */
      virtual void setColLower( int elementIndex, double elementValue ) {
	collower_[elementIndex] = elementValue;
      }
      
      using OsiSolverInterface::setColUpper ;
      /** Set a single column upper bound<br>
    	  Use COIN_DBL_MAX for infinity. */
      virtual void setColUpper( int elementIndex, double elementValue ) {
	colupper_[elementIndex] = elementValue;
      }

      /** Set a single column lower and upper bound */
      virtual void setColBounds( int elementIndex,
    				 double lower, double upper ) {
	collower_[elementIndex] = lower;
	colupper_[elementIndex] = upper;
      }

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
      virtual void setRowLower( int elementIndex, double elementValue ) {
	rowlower_[elementIndex] = elementValue;
	convertBoundToSense(elementValue, rowupper_[elementIndex],
			    rowsense_[elementIndex], rhs_[elementIndex],
			    rowrange_[elementIndex]);
      }
      
      /** Set a single row upper bound<br>
    	  Use COIN_DBL_MAX for infinity. */
      virtual void setRowUpper( int elementIndex, double elementValue ) {
	rowupper_[elementIndex] = elementValue;
	convertBoundToSense(rowlower_[elementIndex], elementValue,
			    rowsense_[elementIndex], rhs_[elementIndex],
			    rowrange_[elementIndex]);
      }
    
      /** Set a single row lower and upper bound */
      virtual void setRowBounds( int elementIndex,
    				 double lower, double upper ) {
	rowlower_[elementIndex] = lower;
	rowupper_[elementIndex] = upper;
	convertBoundToSense(lower, upper,
			    rowsense_[elementIndex], rhs_[elementIndex],
			    rowrange_[elementIndex]);
      }
    
      /** Set the type of a single row<br> */
      virtual void setRowType(int index, char sense, double rightHandSide,
    			      double range) {
	rowsense_[index] = sense;
	rhs_[index] = rightHandSide;
	rowrange_[index] = range;
	convertSenseToBound(sense, rightHandSide, range,
			    rowlower_[index], rowupper_[index]);
      }
    
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
    virtual void setObjSense(double s ) { objsense_ = s < 0 ? -1.0 : 1.0; }
    
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

      using OsiSolverInterface::addCol ;
      /** */
      virtual void addCol(const CoinPackedVectorBase& vec,
    			     const double collb, const double colub,   
    			     const double obj);

      using OsiSolverInterface::addCols ;
      /** */
      virtual void addCols(const int numcols,
			   const CoinPackedVectorBase * const * cols,
			   const double* collb, const double* colub,   
			   const double* obj);
#if 0
      /** */
      virtual void addCols(const CoinPackedMatrix& matrix,
			   const double* collb, const double* colub,   
			   const double* obj);
#endif
      /** */
      virtual void deleteCols(const int num, const int * colIndices);
    
      using OsiSolverInterface::addRow ;
      /** */
      virtual void addRow(const CoinPackedVectorBase& vec,
    			  const double rowlb, const double rowub);
      /** */
      virtual void addRow(const CoinPackedVectorBase& vec,
    			  const char rowsen, const double rowrhs,   
    			  const double rowrng);

      using OsiSolverInterface::addRows ;
      /** */
      virtual void addRows(const int numrows,
			   const CoinPackedVectorBase * const * rows,
			   const double* rowlb, const double* rowub);
      /** */
      virtual void addRows(const int numrows,
			   const CoinPackedVectorBase * const * rows,
    			   const char* rowsen, const double* rowrhs,   
    			   const double* rowrng);
#if 0
      /** */
      virtual void addRows(const CoinPackedMatrix& matrix,
    			   const double* rowlb, const double* rowub);
      /** */
      virtual void addRows(const CoinPackedMatrix& matrix,
    			   const char* rowsen, const double* rowrhs,   
    			   const double* rowrng);
#endif
      /** */
      virtual void deleteRows(const int num, const int * rowIndices);
    
      //-----------------------------------------------------------------------
#if 0
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
#endif
    //@}
  //@}

  //---------------------------------------------------------------------------

protected:
  /**@name Helper methods for problem input */
  void initFromRlbRub(const int rownum,
		      const double* rowlb, const double* rowub);
  void initFromRhsSenseRange(const int rownum, const char* rowsen,
			     const double* rowrhs, const double* rowrng);
  void initFromClbCubObj(const int colnum, const double* collb,
			 const double* colub, const double* obj);
public:
   
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
        default values see the previous method.  <br>
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
    
    using OsiSolverInterface::readMps ;
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

  //---------------------------------------------------------------------------

  /**@name OSL specific public interfaces */
  //@{
    /// Get pointer to Vol model
    VOL_problem* volprob() { return &volprob_; }
  //@}

  //---------------------------------------------------------------------------

  /**@name Constructors and destructors */
  //@{
    /// Default Constructor
    OsiTestSolverInterface ();
    
    /// Clone
    virtual OsiSolverInterface * clone(bool copyData = true) const;
    
    /// Copy constructor 
    OsiTestSolverInterface (const OsiTestSolverInterface &);
    
    /// Assignment operator 
    OsiTestSolverInterface & operator=(const OsiTestSolverInterface& rhs);
    
    /// Destructor 
    virtual ~OsiTestSolverInterface ();
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

private:
  /**@name Methods of <code>VOL_user_hooks</code> */
  //@{
    /// compute reduced costs    
    virtual int compute_rc(const VOL_dvector& u, VOL_dvector& rc);
    /// Solve the subproblem for the subgradient step.
    virtual int solve_subproblem(const VOL_dvector& dual,
				 const VOL_dvector& rc,
				 double& lcost, VOL_dvector& x, VOL_dvector& v,
				 double& pcost);
    /** Starting from the primal vector x, run a heuristic to produce
	an integer solution. This is not done in LP solving. */
  virtual int heuristics(const VOL_problem& /*p*/, 
			 const VOL_dvector& /*x*/, double& heur_val) {
      heur_val = COIN_DBL_MAX;
      return 0;
    }
  //@}

  //---------------------------------------------------------------------------

private:
  /**@name Private helper methods */
  //@{
    /** Update the row ordered matrix from the column ordered one */
    void updateRowMatrix_() const;
    /** Update the column ordered matrix from the row ordered one */
    void updateColMatrix_() const;

    /** Test whether the Volume Algorithm can be applied to the given problem.
     */
    void checkData_() const;
    /** Compute the reduced costs (<code>rc</code>) with respect to the dual
        values given in <code>u</code>. */
    void compute_rc_(const double* u, double* rc) const;
    /** A method deleting every member data */
    void gutsOfDestructor_();

    /** A method allocating sufficient space for the rim vectors corresponding
        to the rows. */
    void rowRimAllocator_();
    /** A method allocating sufficient space for the rim vectors corresponding
        to the columns. */
    void colRimAllocator_();

    /** Reallocate the rim arrays corresponding to the rows. */
    void rowRimResize_(const int newSize);
    /** Reallocate the rim arrays corresponding to the columns. */
    void colRimResize_(const int newSize);

    /** For each row convert LB/UB style row constraints to sense/rhs style. */
    void convertBoundsToSenses_();
    /** For each row convert sense/rhs style row constraints to LB/UB style. */
    void convertSensesToBounds_();

    /** test whether the given matrix is 0/1/-1 entries only. */
    bool test_zero_one_minusone_(const CoinPackedMatrix& m) const;
  //@}

  //---------------------------------------------------------------------------

private:
  
  //---------------------------------------------------------------------------
  /**@name The problem matrix in row and column ordered forms <br>
     Note that at least one of the matrices is always current. */
  //@{
    /// A flag indicating whether the row ordered matrix is up-to-date
    mutable bool rowMatrixCurrent_;
    /// The problem matrix in a row ordered form
    mutable CoinPackedMatrix rowMatrix_;
    /// A flag indicating whether the column ordered matrix is up-to-date
    mutable bool colMatrixCurrent_;
    /// The problem matrix in a column ordered form
    mutable CoinPackedMatrix colMatrix_;
  //@}

  //---------------------------------------------------------------------------
  /**@name Data members used when 0/1/-1 matrix is detected */
  //@{
    /// An indicator whether the matrix is 0/1/-1
    bool isZeroOneMinusOne_;
    /// The row ordered matrix without the elements
    OsiVolMatrixOneMinusOne_* rowMatrixOneMinusOne_;
    /// The column ordered matrix without the elements
    OsiVolMatrixOneMinusOne_* colMatrixOneMinusOne_;
  //@}

  //---------------------------------------------------------------------------
  /**@name The rim vectors */
  //@{
    /// Pointer to dense vector of structural variable upper bounds
    double  *colupper_;
    /// Pointer to dense vector of structural variable lower bounds
    double  *collower_;
    /// Pointer to dense vector of bool to indicate if column is continuous
    bool    *continuous_;
    /// Pointer to dense vector of slack variable upper bounds
    double  *rowupper_;
    /// Pointer to dense vector of slack variable lower bounds
    double  *rowlower_;
    /// Pointer to dense vector of row sense indicators
    char    *rowsense_;
    /// Pointer to dense vector of row right-hand side values
    double  *rhs_;
    /** Pointer to dense vector of slack upper bounds for range 
        constraints (undefined for non-range rows). */
    double  *rowrange_;
    /// Pointer to dense vector of objective coefficients
    double  *objcoeffs_;
  //@}

  //---------------------------------------------------------------------------
  /// Sense of objective (1 for min; -1 for max)
  double  objsense_;

  //---------------------------------------------------------------------------
  /**@name The solution */
  //@{
    /// Pointer to dense vector of primal structural variable values
    double  *colsol_;
    /// Pointer to dense vector of dual row variable values
    double  *rowprice_;
    /// Pointer to dense vector of reduced costs
    double  *rc_;
    /// Pointer to dense vector of left hand sides (row activity levels)
    double  *lhs_;
    /// The Lagrangean cost, a lower bound on the objective value
    double   lagrangeanCost_;
  //@}

  //---------------------------------------------------------------------------
  /** An array to store the hotstart information between solveHotStart()
	calls */
  double  *rowpriceHotStart_;

  /// allocated size of the row related rim vectors
  int maxNumrows_;
  /// allocated size of the column related rim vectors
  int maxNumcols_;

  /// The volume solver
  VOL_problem volprob_;
};

/** A function that tests the methods in the OsiTestSolverInterface class.  */
void
OsiTestSolverInterfaceUnitTest(const std::string & mpsDir, const std::string & netlibDir);

#endif
