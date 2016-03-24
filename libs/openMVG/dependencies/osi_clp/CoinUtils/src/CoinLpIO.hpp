/* $Id: CoinLpIO.hpp 1602 2013-07-12 12:15:37Z tkr $ */
// Last edit: 11/5/08
//
// Name:     CoinLpIO.hpp; Support for Lp files
// Author:   Francois Margot
//           Tepper School of Business
//           Carnegie Mellon University, Pittsburgh, PA 15213
//           email: fmargot@andrew.cmu.edu
// Date:     12/28/03
//-----------------------------------------------------------------------------
// Copyright (C) 2003, Francois Margot, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinLpIO_H
#define CoinLpIO_H

#include <cstdio>

#include "CoinPackedMatrix.hpp"
#include "CoinMessage.hpp"

typedef int COINColumnIndex;

  /** Class to read and write Lp files 

 Lp file format: 

/ this is a comment <BR>
\ this too <BR>
 Min<BR>
  obj: x0 + x1 + 3 x2 - 4.5 xyr + 1 <BR>
 s.t. <BR>
 cons1: x0 - x2 - 2.3 x4 <= 4.2   / this is another comment <BR>
 c2: x1 + x2 >= 1 <BR>
 cc: x1 + x2 + xyr = 2 <BR>
 Bounds <BR>
 0 <= x1 <= 3 <BR>
 1 >= x2 <BR>
 x3 = 1 <BR>
 -2 <= x4 <= Inf <BR>
 xyr free <BR>
 Integers <BR>
 x0 <BR>
 Generals <BR>
 x1 xyr <BR>
 Binaries <BR>
 x2 <BR>
 End

Notes: <UL>
 <LI> Keywords are: Min, Max, Minimize, Maximize, s.t., Subject To, 
      Bounds, Integers, Generals, Binaries, End, Free, Inf. 
 <LI> Keywords are not case sensitive and may be in plural or singular form.
      They should not be used as objective, row or column names.
 <LI> Bounds, Integers, Generals, Binaries sections are optional.
 <LI> Generals and Integers are synonymous.
 <LI> Bounds section (if any) must come before Integers, Generals, and 
      Binaries sections.
 <LI> Row names must be followed by ':' without blank space.
      Row names are optional. If row names are present, 
      they must be distinct (if the k-th constraint has no given name, its name
      is set automatically to "consk" for k=0,...,).
      For valid row names, see the method is_invalid_name(). 
 <LI> Column names must be followed by a blank space. They must be distinct. 
      For valid column names, see the method is_invalid_name(). 
 <LI> The objective function name must be followed by ':' without blank space.
      Objective function name is optional (if no objective function name
      is given, it is set to "obj" by default).
      For valid objective function names, see the method is_invalid_name(). 
 <LI> Ranged constraints are written as two constraints.
      If a name is given for a ranged constraint, the upper bound constraint 
      has that name and the lower bound constraint has that name with "_low" 
      as suffix. This should be kept in mind when assigning names to ranged
      constraint, as the resulting name must be distinct from all the other
      names and be considered valid by the method is_invalid_name().
 <LI> At most one constant term may appear in the objective function; 
      if present, it must appear last. 
 <LI> Default bounds are 0 for lower bound and +infinity for upper bound.
 <LI> Free variables get default lower bound -infinity and 
      default upper bound +infinity. Writing "x0 Free" in an
      LP file means "set lower bound on x0 to -infinity".
 <LI> If more than one upper (resp. lower) bound on a variable appears in 
      the Bounds section, the last one is the one taken into 
      account. The bounds for a binary variable are set to 0/1 only if this 
      bound is stronger than the bound obtained from the Bounds section. 
 <LI> Numbers larger than DBL_MAX (or larger than 1e+400) in the input file
      might crash the code.
 <LI> A comment must start with '\' or '/'. That symbol must either be
      the first character of a line or be preceded by a blank space. The 
      comment ends at the end of the 
      line. Comments are skipped while reading an Lp file and they may be
      inserted anywhere.
</UL>
*/
class CoinLpIO {
      friend void CoinLpIOUnitTest(const std::string & lpDir); 
public:

  /**@name Constructor and Destructor */
  //@{
  /// Default Constructor
  CoinLpIO(); 
  
  /// Does the heavy lifting for destruct and assignment.
  void gutsOfDestructor(); 
 
  /// Does the heavy lifting for copy and assignment
  void gutsOfCopy(const CoinLpIO &); 
 
  /// assignment operator
  CoinLpIO & operator = (const CoinLpIO& rhs) ; 
 
  /// Copy constructor 
  CoinLpIO (const CoinLpIO &); 

  /// Destructor 
  ~CoinLpIO();

  /** Free the vector previous_names_[section] and set 
      card_previous_names_[section] to 0.
      section = 0 for row names, 
      section = 1 for column names.  
  */
  void freePreviousNames(const int section);

  /// Free all memory (except memory related to hash tables and objName_).
  void freeAll();
  //@}

    /** A quick inlined function to convert from lb/ub style constraint
	definition to sense/rhs/range style */
    inline void
    convertBoundToSense(const double lower, const double upper,
			char& sense, double& right, double& range) const;

  /**@name Queries */
  //@{

  /// Get the problem name
  const char * getProblemName() const;

  /// Set problem name
  void setProblemName(const char *name);

  /// Get number of columns
  int getNumCols() const;

  /// Get number of rows
  int getNumRows() const;

  /// Get number of nonzero elements
  int getNumElements() const;
  
  /// Get pointer to array[getNumCols()] of column lower bounds
  const double * getColLower() const;

  /// Get pointer to array[getNumCols()] of column upper bounds
  const double * getColUpper() const;

  /// Get pointer to array[getNumRows()] of row lower bounds
  const double * getRowLower() const;
  
  /// Get pointer to array[getNumRows()] of row upper bounds
  const double * getRowUpper() const;
      /** Get pointer to array[getNumRows()] of constraint senses.
	<ul>
	<li>'L': <= constraint
	<li>'E': =  constraint
	<li>'G': >= constraint
	<li>'R': ranged constraint
	<li>'N': free constraint
	</ul>
      */
  const char * getRowSense() const;
  
  /** Get pointer to array[getNumRows()] of constraint right-hand sides.
      
  Given constraints with upper (rowupper) and/or lower (rowlower) bounds,
  the constraint right-hand side (rhs) is set as
  <ul>
  <li> if rowsense()[i] == 'L' then rhs()[i] == rowupper()[i]
  <li> if rowsense()[i] == 'G' then rhs()[i] == rowlower()[i]
  <li> if rowsense()[i] == 'R' then rhs()[i] == rowupper()[i]
  <li> if rowsense()[i] == 'N' then rhs()[i] == 0.0
	</ul>
  */
  const double * getRightHandSide() const;
  
  /** Get pointer to array[getNumRows()] of row ranges.
      
  Given constraints with upper (rowupper) and/or lower (rowlower) bounds, 
  the constraint range (rowrange) is set as
  <ul>
  <li> if rowsense()[i] == 'R' then
  rowrange()[i] == rowupper()[i] - rowlower()[i]
  <li> if rowsense()[i] != 'R' then
  rowrange()[i] is 0.0
  </ul>
  Put another way, only ranged constraints have a nontrivial value for
  rowrange.
  */
  const double * getRowRange() const;

  /// Get pointer to array[getNumCols()] of objective function coefficients
  const double * getObjCoefficients() const;
  
  /// Get pointer to row-wise copy of the coefficient matrix
  const CoinPackedMatrix * getMatrixByRow() const;

  /// Get pointer to column-wise copy of the coefficient matrix
  const CoinPackedMatrix * getMatrixByCol() const;

  /// Get objective function name
  const char * getObjName() const;
  
  /// Get pointer to array[*card_prev] of previous row names.
  /// The value of *card_prev might be different than getNumRows()+1 if 
  /// non distinct
  /// row names were present or if no previous names were saved or if
  /// the object was holding a different problem before.
  void getPreviousRowNames(char const * const * prev, 
			   int *card_prev) const;

  /// Get pointer to array[*card_prev] of previous column names.
  /// The value of *card_prev might be different than getNumCols() if non
  /// distinct column names were present of if no previous names were saved, 
  /// or if the object was holding a different problem before. 
  void getPreviousColNames(char const * const * prev, 
			   int *card_prev) const;

  /// Get pointer to array[getNumRows()+1] of row names, including
  /// objective function name as last entry.
  char const * const * getRowNames() const;
  
  /// Get pointer to array[getNumCols()] of column names
  char const * const *getColNames() const;
  
  /// Return the row name for the specified index.
  /// Return the objective function name if index = getNumRows().
  /// Return 0 if the index is out of range or if row names are not defined.
  const char * rowName(int index) const;

  /// Return the column name for the specified index.
  /// Return 0 if the index is out of range or if column names are not 
  /// defined.
  const char * columnName(int index) const;

  /// Return the index for the specified row name.
  /// Return getNumRows() for the objective function name.
  /// Return -1 if the name is not found.
  int rowIndex(const char * name) const;

  /// Return the index for the specified column name.
  /// Return -1 if the name is not found.
  int columnIndex(const char * name) const;

  ///Returns the (constant) objective offset
  double objectiveOffset() const;
  
  /// Set objective offset
  inline void setObjectiveOffset(double value)
  { objectiveOffset_ = value;}
  
  /// Return true if a column is an integer (binary or general 
  /// integer) variable
  bool isInteger(int columnNumber) const;
  
  /// Get characteristic vector of integer variables
  const char * integerColumns() const;
  //@}
  
  /**@name Parameters */
  //@{
  /// Get infinity
  double getInfinity() const;

  /// Set infinity. Any number larger is considered infinity.
  /// Default: DBL_MAX
  void setInfinity(const double);

  /// Get epsilon
  double getEpsilon() const;

  /// Set epsilon.
  /// Default: 1e-5.
  void setEpsilon(const double);

  /// Get numberAcross, the number of monomials to be printed per line
  int getNumberAcross() const;

  /// Set numberAcross.
  /// Default: 10.
  void setNumberAcross(const int);

  /// Get decimals, the number of digits to write after the decimal point
  int getDecimals() const;

  /// Set decimals.
  /// Default: 5
  void setDecimals(const int);
  //@}

  /**@name Public methods */
  //@{
  /** Set the data of the object.
      Set it from the coefficient matrix m, the lower bounds
      collb,  the upper bounds colub, objective function obj_coeff, 
      integrality vector integrality, lower/upper bounds on the constraints.
      The sense of optimization of the objective function is assumed to be 
      a minimization. 
      Numbers larger than DBL_MAX (or larger than 1e+400) 
      might crash the code.
  */
  void setLpDataWithoutRowAndColNames(
			      const CoinPackedMatrix& m,
			      const double* collb, const double* colub,
			      const double* obj_coeff, 
			      const char* integrality,
			      const double* rowlb, const double* rowub);

  /** Return 0 if buff is a valid name for a row, a column or objective
      function, return a positive number otherwise.
      If parameter ranged = true, the name is intended for a ranged 
      constraint. <BR>
      Return 1 if the name has more than 100 characters (96 characters
      for a ranged constraint name, as "_low" will be added to the name).<BR>
      Return 2 if the name starts with a number.<BR>
      Return 3 if the name is not built with 
      the letters a to z, A to Z, the numbers 0 to 9 or the characters
      " ! # $ % & ( ) . ; ? @ _ ' ` { } ~ <BR>
      Return 4 if the name is a keyword.<BR>
      Return 5 if the name is empty or NULL. */
  int is_invalid_name(const char *buff, const bool ranged) const;
  
  /** Return 0 if each of the card_vnames entries of vnames is a valid name, 
      return a positive number otherwise. The return value, if not 0, is the 
      return value of is_invalid_name() for the last invalid name
      in vnames. If check_ranged = true, the names are row names and 
      names for ranged constaints must be checked for additional restrictions
      since "_low" will be added to the name if an Lp file is written.
      When check_ranged = true, card_vnames must have getNumRows()+1 entries, 
      with entry vnames[getNumRows()] being the
      name of the objective function.
      For a description of valid names and return values, see the method 
      is_invalid_name(). 

      This method must not be called with check_ranged = true before 
      setLpDataWithoutRowAndColNames() has been called, since access
      to the indices of all the ranged constraints is required.
  */
  int are_invalid_names(char const * const *vnames, 
				  const int card_vnames,
				  const bool check_ranged) const;
  
  /// Set objective function name to the default "obj" and row 
  /// names to the default "cons0", "cons1", ...
  void setDefaultRowNames();

  /// Set column names to the default "x0", "x1", ...
  void setDefaultColNames();

  /** Set the row and column names.
      The array rownames must either be NULL or have exactly getNumRows()+1 
      distinct entries,
      each of them being a valid name (see is_invalid_name()) and the
      last entry being the intended name for the objective function.
      If rownames is NULL, existing row names and objective function
      name are not changed.
      If rownames is deemed invalid, default row names and objective function
      name are used (see setDefaultRowNames()). The memory location of 
      array rownames (or its entries) should not be related
      to the memory location of the array (or entries) obtained from 
      getRowNames() or getPreviousRowNames(), as the call to 
      setLpDataRowAndColNames() modifies the corresponding arrays. 
      Unpredictable results
      are obtained if this requirement is ignored.

      Similar remarks apply to the array colnames, which must either be
      NULL or have exactly getNumCols() entries.
  */
  void setLpDataRowAndColNames(char const * const * const rownames,
			       char const * const * const colnames);

  /** Write the data in Lp format in the file with name filename.
      Coefficients with value less than epsilon away from an integer value
      are written as integers.
      Write at most numberAcross monomials on a line.
      Write non integer numbers with decimals digits after the decimal point.
      Write objective function name and row names if useRowNames = true.

      Ranged constraints are written as two constraints.
      If row names are used, the upper bound constraint has the
      name of the original ranged constraint and the
      lower bound constraint has for name the original name with 
      "_low" as suffix. If doing so creates two identical row names,
      default row names are used (see setDefaultRowNames()).
  */
  int writeLp(const char *filename, 
	      const double epsilon, 
	      const int numberAcross,
	      const int decimals,
	      const bool useRowNames = true);

  /** Write the data in Lp format in the file pointed to by the paramater fp.
      Coefficients with value less than epsilon away from an integer value
      are written as integers.
      Write at most numberAcross monomials on a line.
      Write non integer numbers with decimals digits after the decimal point.
      Write objective function name and row names if useRowNames = true.

      Ranged constraints are written as two constraints.
      If row names are used, the upper bound constraint has the
      name of the original ranged constraint and the
      lower bound constraint has for name the original name with 
      "_low" as suffix. If doing so creates two identical row names,
      default row names are used (see setDefaultRowNames()).
  */
  int writeLp(FILE *fp, 
	      const double epsilon, 
	      const int numberAcross,
	      const int decimals,
	      const bool useRowNames = true);

  /// Write the data in Lp format in the file with name filename.
  /// Write objective function name and row names if useRowNames = true.
  int writeLp(const char *filename, const bool useRowNames = true);

  /// Write the data in Lp format in the file pointed to by the parameter fp.
  /// Write objective function name and row names if useRowNames = true.
  int writeLp(FILE *fp, const bool useRowNames = true);

  /// Read the data in Lp format from the file with name filename, using
  /// the given value for epsilon. If the original problem is
  /// a maximization problem, the objective function is immediadtly 
  /// flipped to get a minimization problem.
  void readLp(const char *filename, const double epsilon);

  /// Read the data in Lp format from the file with name filename.
  /// If the original problem is
  /// a maximization problem, the objective function is immediadtly 
  /// flipped to get a minimization problem.  
  void readLp(const char *filename);

  /// Read the data in Lp format from the file stream, using
  /// the given value for epsilon.
  /// If the original problem is
  /// a maximization problem, the objective function is immediadtly 
  /// flipped to get a minimization problem.  
  void readLp(FILE *fp, const double epsilon);

  /// Read the data in Lp format from the file stream.
  /// If the original problem is
  /// a maximization problem, the objective function is immediadtly 
  /// flipped to get a minimization problem.  
  void readLp(FILE *fp);

  /// Dump the data. Low level method for debugging.
  void print() const;
  //@}
/**@name Message handling */
//@{
  /** Pass in Message handler
  
      Supply a custom message handler. It will not be destroyed when the
      CoinMpsIO object is destroyed.
  */
  void passInMessageHandler(CoinMessageHandler * handler);

  /// Set the language for messages.
  void newLanguage(CoinMessages::Language language);

  /// Set the language for messages.
  inline void setLanguage(CoinMessages::Language language) {newLanguage(language);}

  /// Return the message handler
  inline CoinMessageHandler * messageHandler() const {return handler_;}

  /// Return the messages
  inline CoinMessages messages() {return messages_;}
  /// Return the messages pointer
  inline CoinMessages * messagesPointer() {return & messages_;}
//@}

protected:
  /// Problem name
  char * problemName_;

  /// Message handler
  CoinMessageHandler * handler_;
  /** Flag to say if the message handler is the default handler.
      
      If true, the handler will be destroyed when the CoinMpsIO
      object is destroyed; if false, it will not be destroyed.
  */
  bool defaultHandler_;
  /// Messages
  CoinMessages messages_;

  /// Number of rows
  int numberRows_;
  
  /// Number of columns
  int numberColumns_;
  
  /// Number of elements
  int numberElements_;
  
  /// Pointer to column-wise copy of problem matrix coefficients.
  mutable CoinPackedMatrix *matrixByColumn_;  
  
  /// Pointer to row-wise copy of problem matrix coefficients.
  CoinPackedMatrix *matrixByRow_;  
  
  /// Pointer to dense vector of row lower bounds
  double * rowlower_;
  
  /// Pointer to dense vector of row upper bounds
  double * rowupper_;
  
  /// Pointer to dense vector of column lower bounds
  double * collower_;
  
  /// Pointer to dense vector of column upper bounds
  double * colupper_;
  
  /// Pointer to dense vector of row rhs
  mutable double * rhs_;
  
  /** Pointer to dense vector of slack variable upper bounds for ranged 
      constraints (undefined for non-ranged constraints)
  */
  mutable double  *rowrange_;

  /// Pointer to dense vector of row senses
  mutable char * rowsense_;
  
  /// Pointer to dense vector of objective coefficients
  double * objective_;
  
  /// Constant offset for objective value
  double objectiveOffset_;
  
  /// Pointer to dense vector specifying if a variable is continuous
  /// (0) or integer (1).
  char * integerType_;
  
  /// Current file name
  char * fileName_;
  
  /// Value to use for infinity
  double infinity_;

  /// Value to use for epsilon
  double epsilon_;

  /// Number of monomials printed in a row
  int numberAcross_;

  /// Number of decimals printed for coefficients
  int decimals_;

  /// Objective function name
  char *objName_;

  /** Row names (including objective function name) 
      and column names when stopHash() for the corresponding 
      section was last called or for initial names (deemed invalid) 
      read from a file.<BR>
      section = 0 for row names, 
      section = 1 for column names.  */
  char **previous_names_[2];

  /// card_previous_names_[section] holds the number of entries in the vector 
  /// previous_names_[section].
  /// section = 0 for row names, 
  /// section = 1 for column names. 
  int card_previous_names_[2];

  /// Row names (including objective function name) 
  /// and column names (linked to Hash tables).
  /// section = 0 for row names, 
  /// section = 1 for column names. 
  char **names_[2];

  typedef struct {
    int index, next;
  } CoinHashLink;

  /// Maximum number of entries in a hash table section.
  /// section = 0 for row names, 
  /// section = 1 for column names. 
  int maxHash_[2];

  /// Number of entries in a hash table section.
  /// section = 0 for row names, 
  /// section = 1 for column names. 
  int numberHash_[2];

  /// Hash tables with two sections.
  /// section = 0 for row names (including objective function name), 
  /// section = 1 for column names. 
  mutable CoinHashLink *hash_[2];

  /// Build the hash table for the given names. The parameter number is
  /// the cardinality of parameter names. Remove duplicate names. 
  ///
  /// section = 0 for row names, 
  /// section = 1 for column names. 
  void startHash(char const * const * const names, 
		 const COINColumnIndex number, 
		 int section);

  /// Delete hash storage. If section = 0, it also frees objName_.
  /// section = 0 for row names, 
  /// section = 1 for column names. 
  void stopHash(int section);

  /// Return the index of the given name, return -1 if the name is not found.
  /// Return getNumRows() for the objective function name.
  /// section = 0 for row names (including objective function name), 
  /// section = 1 for column names. 
  COINColumnIndex findHash(const char *name, int section) const;

  /// Insert thisName in the hash table if not present yet; does nothing
  /// if the name is already in.
  /// section = 0 for row names, 
  /// section = 1 for column names. 
  void insertHash(const char *thisName, int section);

  /// Write a coefficient.
  /// print_1 = 0 : do not print the value 1.
  void out_coeff(FILE *fp, double v, int print_1) const;

  /// Locate the objective function. 
  /// Return 1 if found the keyword "Minimize" or one of its variants, 
  /// -1 if found keyword "Maximize" or one of its variants.
  int find_obj(FILE *fp) const;

  /// Return an integer indicating if the keyword "subject to" or one
  /// of its variants has been read.
  /// Return 1 if buff is the keyword "s.t" or one of its variants.
  /// Return 2 if buff is the keyword "subject" or one of its variants.
  /// Return 0 otherwise.
  int is_subject_to(const char *buff) const;

  /// Return 1 if the first character of buff is a number.
  /// Return 0 otherwise.
  int first_is_number(const char *buff) const;

  /// Return 1 if the first character of buff is '/' or '\'.
  /// Return 0 otherwise.
  int is_comment(const char *buff) const;

  /// Read the file fp until buff contains an end of line
  void skip_comment(char *buff, FILE *fp) const;

  /// Put in buff the next string that is not part of a comment
  void scan_next(char *buff, FILE *fp) const;

  /// Return 1 if buff is the keyword "free" or one of its variants.
  /// Return 0 otherwise.
  int is_free(const char *buff) const;
  
  /// Return 1 if buff is the keyword "inf" or one of its variants.
  /// Return 0 otherwise.
  int is_inf(const char *buff) const;
  
  /// Return an integer indicating the inequality sense read.
  /// Return 0 if buff is '<='.
  /// Return 1 if buff is '='.
  /// Return 2 if buff is '>='.
  /// Return -1 otherwise.
  int is_sense(const char *buff) const;

  /// Return an integer indicating if one of the keywords "Bounds", "Integers",
  /// "Generals", "Binaries", "End", or one
  /// of their variants has been read.
  /// Return 1 if buff is the keyword "Bounds" or one of its variants.
  /// Return 2 if buff is the keyword "Integers" or "Generals" or one of their 
  /// variants.
  /// Return 3 if buff is the keyword "Binaries" or one of its variants.
  /// Return 4 if buff is the keyword "End" or one of its variants.
  /// Return 0 otherwise.
  int is_keyword(const char *buff) const;

  /// Read a monomial of the objective function.
  /// Return 1 if "subject to" or one of its variants has been read.
  int read_monom_obj(FILE *fp, double *coeff, char **name, int *cnt, 
		     char **obj_name);

  /// Read a monomial of a constraint.
  /// Return a positive number if the sense of the inequality has been 
  /// read (see method is_sense() for the return code).
  /// Return -1 otherwise.
  int read_monom_row(FILE *fp, char *start_str, double *coeff, char **name, 
		     int cnt_coeff) const;

  /// Reallocate vectors related to number of coefficients.
  void realloc_coeff(double **coeff, char ***colNames, int *maxcoeff) const;

  /// Reallocate vectors related to rows.
  void realloc_row(char ***rowNames, int **start, double **rhs, 
		   double **rowlow, double **rowup, int *maxrow) const;
    
  /// Reallocate vectors related to columns.
  void realloc_col(double **collow, double **colup, char **is_int,
		   int *maxcol) const;

  /// Read a constraint.
  void read_row(FILE *fp, char *buff, double **pcoeff, char ***pcolNames, 
		int *cnt_coeff, int *maxcoeff,
		     double *rhs, double *rowlow, double *rowup, 
		     int *cnt_row, double inf) const;

  /** Check that current objective name and all row names are distinct
      including row names obtained by adding "_low" for ranged constraints.
      If there is a conflict in the names, they are replaced by default 
      row names (see setDefaultRowNames()).

      This method must not be called before 
      setLpDataWithoutRowAndColNames() has been called, since access
      to the indices of all the ranged constraints is required.

      This method must not be called before 
      setLpDataRowAndColNames() has been called, since access
      to all the row names is required.
  */
  void checkRowNames();

  /** Check that current column names are distinct.
      If not, they are replaced by default 
      column names (see setDefaultColNames()).

      This method must not be called before 
      setLpDataRowAndColNames() has been called, since access
      to all the column names is required.
  */
  void checkColNames();

};

void
CoinLpIOUnitTest(const std::string& lpDir);


#endif 