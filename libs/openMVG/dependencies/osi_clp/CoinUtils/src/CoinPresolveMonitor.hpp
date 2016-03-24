
#ifndef CoinPresolveMonitor_H
#define CoinPresolveMonitor_H

/*!
  \brief Monitor a row or column for modification

  The purpose of this class is to monitor a row or column for modifications
  during presolve and postsolve. Each object can monitor one row or
  column. The initial copy of the row or column is loaded by the constructor.
  Each subsequent call to checkAndTell() compares the current state of the row
  or column with the stored state and reports any modifications.

  Internally the row or column is held as a CoinPackedVector so that it's
  possible to follow a row or column through presolve (CoinPresolveMatrix)
  and postsolve (CoinPostsolveMatrix).

  Do not underestimate the amount of work required here. Extracting a row from
  the CoinPostsolve matrix requires a scan of every element in the matrix.
  That's one scan by the constructor and one scan with every call to modify.
  But that's precisely why it's virtually impossible to debug presolve without
  aids.

  Parameter overloads for CoinPresolveMatrix and CoinPostsolveMatrix are a
  little clumsy, but not a problem in use. The alternative is to add methods
  to the CoinPresolveMatrix and CoinPostsolveMatrix classes that will only be
  used for debugging. That's not too attractive either.
*/
class CoinPresolveMonitor
{
  public:

  /*! \brief Default constructor

    Creates an empty monitor.
  */
  CoinPresolveMonitor() ;

  /*! \brief Initialise from a CoinPresolveMatrix

    Load the initial row or column from a CoinPresolveMatrix. Set \p isRow
    true for a row, false for a column.
  */
  CoinPresolveMonitor(const CoinPresolveMatrix *mtx, bool isRow, int k) ;

  /*! \brief Initialise from a CoinPostsolveMatrix

    Load the initial row or column from a CoinPostsolveMatrix. Set \p isRow
    true for a row, false for a column.
  */
  CoinPresolveMonitor(const CoinPostsolveMatrix *mtx, bool isRow, int k) ;

  /*! \brief Compare the present row or column against the stored copy and
  	     report differences.

    Load the current row or column from a CoinPresolveMatrix and compare.
    Differences are printed to std::cout.
  */
  void checkAndTell(const CoinPresolveMatrix *mtx) ;

  /*! \brief Compare the present row or column against the stored copy and
  	     report differences.

    Load the current row or column from a CoinPostsolveMatrix and compare.
    Differences are printed to std::cout.
  */
  void checkAndTell(const CoinPostsolveMatrix *mtx) ;

  private:

  /// Extract a row from a CoinPresolveMatrix
  CoinPackedVector *extractRow(int i, const CoinPresolveMatrix *mtx) const ;

  /// Extract a column from a CoinPresolveMatrix
  CoinPackedVector *extractCol(int j, const CoinPresolveMatrix *mtx) const ;

  /// Extract a row from a CoinPostsolveMatrix
  CoinPackedVector *extractRow(int i, const CoinPostsolveMatrix *mtx) const ;

  /// Extract a column from a CoinPostsolveMatrix
  CoinPackedVector *extractCol(int j, const CoinPostsolveMatrix *mtx) const ;

  /// Worker method underlying the public checkAndTell methods.
  void checkAndTell(CoinPackedVector *curVec, double lb, double ub) ;

  /// True to monitor a row, false to monitor a column
  bool isRow_ ;

  /// Row or column index
  int ndx_ ;

  /*! The original row or column

    Sorted in increasing order of indices.
  */
  CoinPackedVector *origVec_ ;

  /// Original row or column lower bound
  double lb_ ;

  /// Original row or column upper bound
  double ub_ ;
} ;

#endif
