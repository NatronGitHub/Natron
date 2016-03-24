/* $Id: CoinMpsIO.hpp 1419 2011-04-29 17:39:07Z stefan $ */
// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinMpsIO_H
#define CoinMpsIO_H

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <vector>
#include <string>

#include "CoinUtilsConfig.h"
#include "CoinPackedMatrix.hpp"
#include "CoinMessageHandler.hpp"
#include "CoinFileIO.hpp"
class CoinModel;

/// The following lengths are in decreasing order (for 64 bit etc)
/// Large enough to contain element index
/// This is already defined as CoinBigIndex
/// Large enough to contain column index
typedef int COINColumnIndex;

/// Large enough to contain row index (or basis)
typedef int COINRowIndex;

// We are allowing free format - but there is a limit!
// User can override by using CXXFLAGS += -DCOIN_MAX_FIELD_LENGTH=nnn
#ifndef COIN_MAX_FIELD_LENGTH
#define COIN_MAX_FIELD_LENGTH 160
#endif
#define MAX_CARD_LENGTH 5*COIN_MAX_FIELD_LENGTH+80

enum COINSectionType { COIN_NO_SECTION, COIN_NAME_SECTION, COIN_ROW_SECTION,
		       COIN_COLUMN_SECTION,
		       COIN_RHS_SECTION, COIN_RANGES_SECTION, COIN_BOUNDS_SECTION,
		       COIN_ENDATA_SECTION, COIN_EOF_SECTION, COIN_QUADRATIC_SECTION, 
		       COIN_CONIC_SECTION,COIN_QUAD_SECTION,COIN_SOS_SECTION, 
		       COIN_BASIS_SECTION,COIN_UNKNOWN_SECTION
};

enum COINMpsType { COIN_N_ROW, COIN_E_ROW, COIN_L_ROW, COIN_G_ROW,
  COIN_BLANK_COLUMN, COIN_S1_COLUMN, COIN_S2_COLUMN, COIN_S3_COLUMN,
  COIN_INTORG, COIN_INTEND, COIN_SOSEND, COIN_UNSET_BOUND,
  COIN_UP_BOUND, COIN_FX_BOUND, COIN_LO_BOUND, COIN_FR_BOUND,
                   COIN_MI_BOUND, COIN_PL_BOUND, COIN_BV_BOUND, COIN_UI_BOUND, COIN_LI_BOUND,
		   COIN_SC_BOUND, COIN_S1_BOUND, COIN_S2_BOUND,
		   COIN_BS_BASIS, COIN_XL_BASIS, COIN_XU_BASIS,
		   COIN_LL_BASIS, COIN_UL_BASIS, COIN_UNKNOWN_MPS_TYPE
};
class CoinMpsIO;
/// Very simple code for reading MPS data
class CoinMpsCardReader {

public:

  /**@name Constructor and destructor */
  //@{
  /// Constructor expects file to be open 
  /// This one takes gzFile if fp null
  CoinMpsCardReader ( CoinFileInput *input, CoinMpsIO * reader );

  /// Destructor
  ~CoinMpsCardReader (  );
  //@}


  /**@name card stuff */
  //@{
  /// Read to next section
  COINSectionType readToNextSection (  );
  /// Gets next field and returns section type e.g. COIN_COLUMN_SECTION
  COINSectionType nextField (  );
  /** Gets next field for .gms file and returns type.
      -1 - EOF
      0 - what we expected (and processed so pointer moves past)
      1 - not what we expected
      leading blanks always ignored
      input types 
      0 - anything - stops on non blank card
      1 - name (in columnname)
      2 - value
      3 - value name pair
      4 - equation type
      5 - ;
  */
  int nextGmsField ( int expectedType );
  /// Returns current section type
  inline COINSectionType whichSection (  ) const {
    return section_;
  }
  /// Sets current section type
  inline void setWhichSection(COINSectionType section  ) {
    section_=section;
  }
  /// Sees if free format. 
  inline bool freeFormat() const
  { return freeFormat_;}
  /// Sets whether free format.  Mainly for blank RHS etc
  inline void setFreeFormat(bool yesNo) 
  { freeFormat_=yesNo;}
  /// Only for first field on card otherwise BLANK_COLUMN
  /// e.g. COIN_E_ROW
  inline COINMpsType mpsType (  ) const {
    return mpsType_;
  }
  /// Reads and cleans card - taking out trailing blanks - return 1 if EOF
  int cleanCard();
  /// Returns row name of current field
  inline const char *rowName (  ) const {
    return rowName_;
  }
  /// Returns column name of current field
  inline const char *columnName (  ) const {
    return columnName_;
  }
  /// Returns value in current field
  inline double value (  ) const {
    return value_;
  }
  /// Returns value as string in current field
  inline const char *valueString (  ) const {
    return valueString_;
  }
  /// Whole card (for printing)
  inline const char *card (  ) const {
    return card_;
  }
  /// Whole card - so we look at it (not const so nextBlankOr will work for gms reader)
  inline char *mutableCard (  ) {
    return card_;
  }
  /// set position (again so gms reader will work)
  inline void setPosition(char * position)
  { position_=position;}
  /// get position (again so gms reader will work)
  inline char * getPosition() const
  { return position_;}
  /// Returns card number
  inline CoinBigIndex cardNumber (  ) const {
    return cardNumber_;
  }
  /// Returns file input
  inline CoinFileInput * fileInput (  ) const {
    return input_;
  }
  /// Sets whether strings allowed
  inline void setStringsAllowed()
  { stringsAllowed_=true;}
  //@}

////////////////// data //////////////////
protected:

  /**@name data */
  //@{
  /// Current value
  double value_;
  /// Current card image
  char card_[MAX_CARD_LENGTH];
  /// Current position within card image
  char *position_;
  /// End of card
  char *eol_;
  /// Current COINMpsType
  COINMpsType mpsType_;
  /// Current row name
  char rowName_[COIN_MAX_FIELD_LENGTH];
  /// Current column name
  char columnName_[COIN_MAX_FIELD_LENGTH];
  /// File input
  CoinFileInput *input_;
  /// Which section we think we are in
  COINSectionType section_;
  /// Card number
  CoinBigIndex cardNumber_;
  /// Whether free format.  Just for blank RHS etc
  bool freeFormat_;
  /// Whether IEEE - 0 no, 1 INTEL, 2 not INTEL
  int ieeeFormat_;
  /// If all names <= 8 characters then allow embedded blanks
  bool eightChar_;
  /// MpsIO
  CoinMpsIO * reader_;
  /// Message handler
  CoinMessageHandler * handler_;
  /// Messages
  CoinMessages messages_;
  /// Current element as characters (only if strings allowed) 
  char valueString_[COIN_MAX_FIELD_LENGTH];
  /// Whether strings allowed
  bool stringsAllowed_;
  //@}
public:
  /**@name methods */
  //@{
  /// type - 0 normal, 1 INTEL IEEE, 2 other IEEE
  double osi_strtod(char * ptr, char ** output, int type);
  /// remove blanks 
  static void strcpyAndCompress ( char *to, const char *from );
  ///
  static char * nextBlankOr ( char *image );
  /// For strings
  double osi_strtod(char * ptr, char ** output);
  //@}

};

//#############################################################################
#ifdef USE_SBB
class SbbObject;
class SbbModel;
#endif
/// Very simple class for containing data on set
class CoinSet {

public:

  /**@name Constructor and destructor */
  //@{
  /// Default constructor 
  CoinSet ( );
  /// Constructor 
  CoinSet ( int numberEntries, const int * which);

  /// Copy constructor 
  CoinSet (const CoinSet &);
  
  /// Assignment operator 
  CoinSet & operator=(const CoinSet& rhs);  
  
  /// Destructor
  virtual ~CoinSet (  );
  //@}


  /**@name gets */
  //@{
  /// Returns number of entries
  inline int numberEntries (  ) const 
  { return numberEntries_;  }
  /// Returns type of set - 1 =SOS1, 2 =SOS2
  inline int setType (  ) const 
  { return setType_;  }
  /// Returns list of variables
  inline const int * which (  ) const 
  { return which_;  }
  /// Returns weights
  inline const double * weights (  ) const 
  { return weights_;  }
  //@}

#ifdef USE_SBB
  /**@name Use in sbb */
  //@{
  /// returns an object of type SbbObject
  virtual SbbObject * sbbObject(SbbModel * model) const 
  { return NULL;}
  //@}
#endif

////////////////// data //////////////////
protected:

  /**@name data */
  //@{
  /// Number of entries
  int numberEntries_;
  /// type of set
  int setType_;
  /// Which variables are in set
  int * which_;
  /// Weights
  double * weights_;
  //@}
};

//#############################################################################
/// Very simple class for containing SOS set
class CoinSosSet : public CoinSet{

public:

  /**@name Constructor and destructor */
  //@{
  /// Constructor 
  CoinSosSet ( int numberEntries, const int * which, const double * weights, int type);

  /// Destructor
  virtual ~CoinSosSet (  );
  //@}


#ifdef USE_SBB
  /**@name Use in sbb */
  //@{
  /// returns an object of type SbbObject
  virtual SbbObject * sbbObject(SbbModel * model) const ;
  //@}
#endif

////////////////// data //////////////////
protected:

  /**@name data */
  //@{
  //@}
};

//#############################################################################

/** MPS IO Interface

    This class can be used to read in mps files without a solver.  After
    reading the file, the CoinMpsIO object contains all relevant data, which
    may be more than a particular OsiSolverInterface allows for.  Items may
    be deleted to allow for flexibility of data storage.

    The implementation makes the CoinMpsIO object look very like a dummy solver,
    as the same conventions are used.
*/

class CoinMpsIO {
   friend void CoinMpsIOUnitTest(const std::string & mpsDir);

public:

/** @name Methods to retrieve problem information

   These methods return information about the problem held by the CoinMpsIO
   object.
   
   Querying an object that has no data associated with it result in zeros for
   the number of rows and columns, and NULL pointers from the methods that
   return vectors.  Const pointers returned from any data-query method are
   always valid
*/
//@{
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
	Put another way, only range constraints have a nontrivial value for
	rowrange.
    */
    const double * getRowRange() const;

    /// Get pointer to array[getNumRows()] of row lower bounds
    const double * getRowLower() const;

    /// Get pointer to array[getNumRows()] of row upper bounds
    const double * getRowUpper() const;

    /// Get pointer to array[getNumCols()] of objective function coefficients
    const double * getObjCoefficients() const;

    /// Get pointer to row-wise copy of the coefficient matrix
    const CoinPackedMatrix * getMatrixByRow() const;

    /// Get pointer to column-wise copy of the coefficient matrix
    const CoinPackedMatrix * getMatrixByCol() const;

    /// Return true if column is a continuous variable
    bool isContinuous(int colNumber) const;

    /** Return true if a column is an integer variable

        Note: This function returns true if the the column
        is a binary or general integer variable.
    */
    bool isInteger(int columnNumber) const;
  
    /** Returns array[getNumCols()] specifying if a variable is integer.

	At present, simply coded as zero (continuous) and non-zero (integer)
	May be extended at a later date.
    */
    const char * integerColumns() const;

    /** Returns the row name for the specified index.

	Returns 0 if the index is out of range.
    */
    const char * rowName(int index) const;

    /** Returns the column name for the specified index.

	Returns 0 if the index is out of range.
    */
    const char * columnName(int index) const;

    /** Returns the index for the specified row name
  
	Returns -1 if the name is not found.
        Returns numberRows for the objective row and > numberRows for
	dropped free rows.
    */
    int rowIndex(const char * name) const;

    /** Returns the index for the specified column name
  
	Returns -1 if the name is not found.
    */
    int columnIndex(const char * name) const;

    /** Returns the (constant) objective offset
    
	This is the RHS entry for the objective row
    */
    double objectiveOffset() const;
    /// Set objective offset
    inline void setObjectiveOffset(double value)
    { objectiveOffset_=value;}

    /// Return the problem name
    const char * getProblemName() const;

    /// Return the objective name
    const char * getObjectiveName() const;

    /// Return the RHS vector name
    const char * getRhsName() const;

    /// Return the range vector name
    const char * getRangeName() const;

    /// Return the bound vector name
    const char * getBoundName() const;
    /// Number of string elements
    inline int numberStringElements() const
    { return numberStringElements_;}
    /// String element
    inline const char * stringElement(int i) const
    { return stringElements_[i];}
//@}


/** @name Methods to set problem information

    Methods to load a problem into the CoinMpsIO object.
*/
//@{
  
    /// Set the problem data
    void setMpsData(const CoinPackedMatrix& m, const double infinity,
		     const double* collb, const double* colub,
		     const double* obj, const char* integrality,
		     const double* rowlb, const double* rowub,
		     char const * const * const colnames,
		     char const * const * const rownames);
    void setMpsData(const CoinPackedMatrix& m, const double infinity,
		     const double* collb, const double* colub,
		     const double* obj, const char* integrality,
		     const double* rowlb, const double* rowub,
		     const std::vector<std::string> & colnames,
		     const std::vector<std::string> & rownames);
    void setMpsData(const CoinPackedMatrix& m, const double infinity,
		     const double* collb, const double* colub,
		     const double* obj, const char* integrality,
		     const char* rowsen, const double* rowrhs,
		     const double* rowrng,
		     char const * const * const colnames,
		     char const * const * const rownames);
    void setMpsData(const CoinPackedMatrix& m, const double infinity,
		     const double* collb, const double* colub,
		     const double* obj, const char* integrality,
		     const char* rowsen, const double* rowrhs,
		     const double* rowrng,
		     const std::vector<std::string> & colnames,
		     const std::vector<std::string> & rownames);

    /** Pass in an array[getNumCols()] specifying if a variable is integer.

	At present, simply coded as zero (continuous) and non-zero (integer)
	May be extended at a later date.
    */
    void copyInIntegerInformation(const char * integerInformation);

    /// Set problem name
    void setProblemName(const char *name) ;

    /// Set objective name
    void setObjectiveName(const char *name) ;

//@}

/** @name Parameter set/get methods

  Methods to set and retrieve MPS IO parameters.
*/

//@{
    /// Set infinity
    void setInfinity(double value);

    /// Get infinity
    double getInfinity() const;

    /// Set default upper bound for integer variables
    void setDefaultBound(int value);

    /// Get default upper bound for integer variables
    int getDefaultBound() const;
    /// Whether to allow string elements
    inline int allowStringElements() const
    { return allowStringElements_;}
    /// Whether to allow string elements (0 no, 1 yes, 2 yes and try flip)
    inline void setAllowStringElements(int yesNo)
    { allowStringElements_ = yesNo;}
    /** Small element value - elements less than this set to zero on input
        default is 1.0e-14 */
    inline double getSmallElementValue() const
    { return smallElement_;}
    inline void setSmallElementValue(double value)
    { smallElement_=value;} 
//@}


/** @name Methods for problem input and output

  Methods to read and write MPS format problem files.
   
  The read and write methods return the number of errors that occurred during
  the IO operation, or -1 if no file is opened.

  \note
  If the CoinMpsIO class was compiled with support for libz then
  readMps will automatically try to append .gz to the file name and open it as
  a compressed file if the specified file name cannot be opened.
  (Automatic append of the .bz2 suffix when libbz is used is on the TODO list.)

  \todo
  Allow for file pointers and positioning
*/

//@{
    /// Set the current file name for the CoinMpsIO object
    void setFileName(const char * name);

    /// Get the current file name for the CoinMpsIO object
    const char * getFileName() const;

    /** Read a problem in MPS format from the given filename.

      Use "stdin" or "-" to read from stdin.
    */
    int readMps(const char *filename, const char *extension = "mps");

    /** Read a problem in MPS format from the given filename.

      Use "stdin" or "-" to read from stdin.
      But do sets as well
    */
     int readMps(const char *filename, const char *extension ,
        int & numberSets, CoinSet **& sets);

    /** Read a problem in MPS format from a previously opened file

      More precisely, read a problem using a CoinMpsCardReader object already
      associated with this CoinMpsIO object.

      \todo
      Provide an interface that will allow a client to associate a
      CoinMpsCardReader object with a CoinMpsIO object by setting the
      cardReader_ field.
    */
    int readMps();
    /// and
    int readMps(int & numberSets, CoinSet **& sets);
    /** Read a basis in MPS format from the given filename.
	If VALUES on NAME card and solution not NULL fills in solution
	status values as for CoinWarmStartBasis (but one per char)
	-1 file error, 0 normal, 1 has solution values

      Use "stdin" or "-" to read from stdin.

      If sizes of names incorrect - read without names
    */
    int readBasis(const char *filename, const char *extension ,
		  double * solution, unsigned char *rowStatus, unsigned char *columnStatus,
		  const std::vector<std::string> & colnames,int numberColumns,
		  const std::vector<std::string> & rownames, int numberRows);

    /** Read a problem in GAMS format from the given filename.

      Use "stdin" or "-" to read from stdin.
      if convertObjective then massages objective column
    */
    int readGms(const char *filename, const char *extension = "gms",bool convertObjective=false);

    /** Read a problem in GAMS format from the given filename.

      Use "stdin" or "-" to read from stdin.
      But do sets as well
    */
     int readGms(const char *filename, const char *extension ,
        int & numberSets, CoinSet **& sets);

    /** Read a problem in GAMS format from a previously opened file

      More precisely, read a problem using a CoinMpsCardReader object already
      associated with this CoinMpsIO object.

    */
    // Not for now int readGms();
    /// and
    int readGms(int & numberSets, CoinSet **& sets);
    /** Read a problem in GMPL (subset of AMPL)  format from the given filenames.
    */
    int readGMPL(const char *modelName, const char * dataName=NULL, bool keepNames=false);

    /** Write the problem in MPS format to a file with the given filename.

	\param compression can be set to three values to indicate what kind
	of file should be written
	<ul>
	  <li> 0: plain text (default)
	  <li> 1: gzip compressed (.gz is appended to \c filename)
	  <li> 2: bzip2 compressed (.bz2 is appended to \c filename) (TODO)
	</ul>
	If the library was not compiled with the requested compression then
	writeMps falls back to writing a plain text file.

	\param formatType specifies the precision to used for values in the
	MPS file
	<ul>
	  <li> 0: normal precision (default)
	  <li> 1: extra accuracy
	  <li> 2: IEEE hex
	</ul>

	\param numberAcross specifies whether 1 or 2 (default) values should be
	specified on every data line in the MPS file.

	\param quadratic specifies quadratic objective to be output
    */
    int writeMps(const char *filename, int compression = 0,
		 int formatType = 0, int numberAcross = 2,
		 CoinPackedMatrix * quadratic = NULL,
		 int numberSOS=0,const CoinSet * setInfo=NULL) const;

    /// Return card reader object so can see what last card was e.g. QUADOBJ
    inline const CoinMpsCardReader * reader() const
    { return cardReader_;}
  
    /** Read in a quadratic objective from the given filename.

      If filename is NULL (or the same as the currently open file) then
      reading continues from the current file.
      If not, the file is closed and the specified file is opened.
      
      Code should be added to
      general MPS reader to read this if QSECTION
      Data is assumed to be Q and objective is c + 1/2 xT Q x
      No assumption is made for symmetry, positive definite, etc.
      No check is made for duplicates or non-triangular if checkSymmetry==0.
      If 1 checks lower triangular (so off diagonal should be 2*Q)
      if 2 makes lower triangular and assumes full Q (but adds off diagonals)
      
      Arrays should be deleted by delete []

      Returns number of errors:
      <ul>
	<li> -1: bad file
	<li> -2: no Quadratic section
	<li> -3: an empty section
        <li> +n: then matching errors etc (symmetry forced)
        <li> -4: no matching errors but fails triangular test
		 (triangularity forced)
      </ul>
      columnStart is numberColumns+1 long, others numberNonZeros
    */
    int readQuadraticMps(const char * filename,
			 int * &columnStart, int * &column, double * &elements,
			 int checkSymmetry);

    /** Read in a list of cones from the given filename.  

      If filename is NULL (or the same as the currently open file) then
      reading continues from the current file.
      If not, the file is closed and the specified file is opened.

      Code should be added to
      general MPS reader to read this if CSECTION
      No checking is done that in unique cone

      Arrays should be deleted by delete []

      Returns number of errors, -1 bad file, -2 no conic section,
      -3 empty section

      columnStart is numberCones+1 long, other number of columns in matrix
    */
    int readConicMps(const char * filename,
		     int * &columnStart, int * &column, int & numberCones);
    /// Set whether to move objective from matrix
    inline void setConvertObjective(bool trueFalse)
    { convertObjective_=trueFalse;}
  /// copies in strings from a CoinModel - returns number
  int copyStringElements(const CoinModel * model);
  //@}

/** @name Constructors and destructors */
//@{
    /// Default Constructor
    CoinMpsIO(); 
      
    /// Copy constructor 
    CoinMpsIO (const CoinMpsIO &);
  
    /// Assignment operator 
    CoinMpsIO & operator=(const CoinMpsIO& rhs);
  
    /// Destructor 
    ~CoinMpsIO ();
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


/**@name Methods to release storage

  These methods allow the client to reduce the storage used by the CoinMpsIO
  object be selectively releasing unneeded problem information.
*/
//@{
    /** Release all information which can be re-calculated.
    
	E.g., row sense, copies of rows, hash tables for names.
    */
    void releaseRedundantInformation();

    /// Release all row information (lower, upper)
    void releaseRowInformation();

    /// Release all column information (lower, upper, objective)
    void releaseColumnInformation();

    /// Release integer information
    void releaseIntegerInformation();

    /// Release row names
    void releaseRowNames();

    /// Release column names
    void releaseColumnNames();

    /// Release matrix information
    void releaseMatrixInformation();
  //@}

protected:
  
/**@name Miscellaneous helper functions */
  //@{

    /// Utility method used several times to implement public methods
    void
    setMpsDataWithoutRowAndColNames(
		      const CoinPackedMatrix& m, const double infinity,
		      const double* collb, const double* colub,
		      const double* obj, const char* integrality,
		      const double* rowlb, const double* rowub);
    void
    setMpsDataColAndRowNames(
		      const std::vector<std::string> & colnames,
		      const std::vector<std::string> & rownames);
    void
    setMpsDataColAndRowNames(
		      char const * const * const colnames,
		      char const * const * const rownames);

  
    /// Does the heavy lifting for destruct and assignment.
    void gutsOfDestructor();

    /// Does the heavy lifting for copy and assignment.
    void gutsOfCopy(const CoinMpsIO &);
  
    /// Clears problem data from the CoinMpsIO object.
    void freeAll();


    /** A quick inlined function to convert from lb/ub style constraint
	definition to sense/rhs/range style */
    inline void
    convertBoundToSense(const double lower, const double upper,
			char& sense, double& right, double& range) const;
    /** A quick inlined function to convert from sense/rhs/range stryle
	constraint definition to lb/ub style */
    inline void
    convertSenseToBound(const char sense, const double right,
			const double range,
			double& lower, double& upper) const;

  /** Deal with a filename
  
    As the name says.
    Returns +1 if the file name is new, 0 if it's the same as before
    (i.e., matches fileName_), and -1 if there's an error and the file
    can't be opened.
    Handles automatic append of .gz suffix when compiled with libz.

    \todo
    Add automatic append of .bz2 suffix when compiled with libbz.
  */

  int dealWithFileName(const char * filename,  const char * extension,
		       CoinFileInput * &input); 
  /** Add string to list
      iRow==numberRows is objective, nr+1 is lo, nr+2 is up
      iColumn==nc is rhs (can't cope with ranges at present)
  */
  void addString(int iRow,int iColumn, const char * value);
  /// Decode string
  void decodeString(int iString, int & iRow, int & iColumn, const char * & value) const;
  //@}

  
  // for hashing
  typedef struct {
    int index, next;
  } CoinHashLink;

  /**@name Hash table methods */
  //@{
  /// Creates hash list for names (section = 0 for rows, 1 columns)
  void startHash ( char **names, const int number , int section );
  /// This one does it when names are already in
  void startHash ( int section ) const;
  /// Deletes hash storage
  void stopHash ( int section );
  /// Finds match using hash,  -1 not found
  int findHash ( const char *name , int section ) const;
  //@}

    /**@name Cached problem information */
    //@{
      /// Problem name
      char * problemName_;

      /// Objective row name
      char * objectiveName_;

      /// Right-hand side vector name
      char * rhsName_;

      /// Range vector name
      char * rangeName_;

      /// Bounds vector name
      char * boundName_;

      /// Number of rows
      int numberRows_;

      /// Number of columns
      int numberColumns_;

      /// Number of coefficients
      CoinBigIndex numberElements_;

      /// Pointer to dense vector of row sense indicators
      mutable char    *rowsense_;
  
      /// Pointer to dense vector of row right-hand side values
      mutable double  *rhs_;
  
      /** Pointer to dense vector of slack variable upper bounds for range 
          constraints (undefined for non-range rows)
      */
      mutable double  *rowrange_;
   
      /// Pointer to row-wise copy of problem matrix coefficients.
      mutable CoinPackedMatrix *matrixByRow_;  

      /// Pointer to column-wise copy of problem matrix coefficients.
      CoinPackedMatrix *matrixByColumn_;  

      /// Pointer to dense vector of row lower bounds
      double * rowlower_;

      /// Pointer to dense vector of row upper bounds
      double * rowupper_;

      /// Pointer to dense vector of column lower bounds
      double * collower_;

      /// Pointer to dense vector of column upper bounds
      double * colupper_;

      /// Pointer to dense vector of objective coefficients
      double * objective_;

      /// Constant offset for objective value (i.e., RHS value for OBJ row)
      double objectiveOffset_;


      /** Pointer to dense vector specifying if a variable is continuous
	  (0) or integer (1).
      */
      char * integerType_;

      /** Row and column names
	  Linked to hash table sections (0 - row names, 1 column names)
      */
      char **names_[2];
    //@}

    /** @name Hash tables */
    //@{
      /// Current file name
      char * fileName_;

      /// Number of entries in a hash table section
      int numberHash_[2];

      /// Hash tables (two sections, 0 - row names, 1 - column names)
      mutable CoinHashLink *hash_[2];
    //@}

    /** @name CoinMpsIO object parameters */
    //@{
      /// Upper bound when no bounds for integers
      int defaultBound_; 

      /// Value to use for infinity
      double infinity_;
      /// Small element value
      double smallElement_;

      /// Message handler
      CoinMessageHandler * handler_;
      /** Flag to say if the message handler is the default handler.

          If true, the handler will be destroyed when the CoinMpsIO
	  object is destroyed; if false, it will not be destroyed.
      */
      bool defaultHandler_;
      /// Messages
      CoinMessages messages_;
      /// Card reader
      CoinMpsCardReader * cardReader_;
      /// If .gms file should it be massaged to move objective
      bool convertObjective_;
      /// Whether to allow string elements
      int allowStringElements_;
      /// Maximum number of string elements
      int maximumStringElements_;
      /// Number of string elements
      int numberStringElements_;
      /// String elements
      char ** stringElements_;
    //@}

};

//#############################################################################
/** A function that tests the methods in the CoinMpsIO class. The
    only reason for it not to be a member method is that this way it doesn't
    have to be compiled into the library. And that's a gain, because the
    library should be compiled with optimization on, but this method should be
    compiled with debugging. Also, if this method is compiled with
    optimization, the compilation takes 10-15 minutes and the machine pages
    (has 256M core memory!)... */
void
CoinMpsIOUnitTest(const std::string & mpsDir);
// Function to return number in most efficient way
// section is 0 for columns, 1 for rhs,ranges and 2 for bounds
/* formatType is
   0 - normal and 8 character names
   1 - extra accuracy
   2 - IEEE hex - INTEL
   3 - IEEE hex - not INTEL
*/
void
CoinConvertDouble(int section, int formatType, double value, char outputValue[24]);

#endif
