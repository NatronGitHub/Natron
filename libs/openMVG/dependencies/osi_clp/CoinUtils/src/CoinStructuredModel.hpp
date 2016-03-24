/* $Id: CoinStructuredModel.hpp 1372 2011-01-03 23:31:00Z lou $ */
// Copyright (C) 2008, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinStructuredModel_H
#define CoinStructuredModel_H

#include "CoinModel.hpp"
#include <vector>

/** 
    This is a model which is made up of Coin(Structured)Model blocks.
*/
  typedef struct CoinModelInfo2 {
    int rowBlock; // Which row block
    int columnBlock; // Which column block
    char matrix; // nonzero if matrix exists
    char rhs; // nonzero if non default rhs exists
    char rowName; // nonzero if row names exists
    char integer; // nonzero if integer information exists
    char bounds; // nonzero if non default bounds/objective exists
    char columnName; // nonzero if column names exists
    CoinModelInfo2() : 
      rowBlock(0),
      columnBlock(0),
      matrix(0),
      rhs(0),
      rowName(0),
      integer(0),
      bounds(0),
      columnName(0)
    {}
} CoinModelBlockInfo;

class CoinStructuredModel : public CoinBaseModel {
  
public:
  /**@name Useful methods for building model */
  //@{
  /** add a block from a CoinModel using names given as parameters 
      returns number of errors (e.g. both have objectives but not same)
   */
  int addBlock(const std::string & rowBlock,
		const std::string & columnBlock,
		const CoinBaseModel & block);
  /** add a block from a CoinModel with names in model
      returns number of errors (e.g. both have objectives but not same)
 */
  int addBlock(const CoinBaseModel & block);
  /** add a block from a CoinModel using names given as parameters 
      returns number of errors (e.g. both have objectives but not same)
      This passes in block - structured model takes ownership
   */
  int addBlock(const std::string & rowBlock,
		const std::string & columnBlock,
		CoinBaseModel * block);
  /** add a block using names 
   */
  int addBlock(const std::string & rowBlock,
	       const std::string & columnBlock,
	       const CoinPackedMatrix & matrix,
	       const double * rowLower, const double * rowUpper,
	       const double * columnLower, const double * columnUpper,
	       const double * objective);

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
  
  not const as may change model e.g. fill in default bounds
  */
  int writeMps(const char *filename, int compression = 0,
               int formatType = 0, int numberAcross = 2, bool keepStrings=false) ;
  /** Decompose a CoinModel
      1 - try D-W
      2 - try Benders
      3 - try Staircase
      Returns number of blocks or zero if no structure
  */
  int decompose(const CoinModel &model,int type,
		int maxBlocks=50);
  /** Decompose a model specified as arrays + CoinPackedMatrix
      1 - try D-W
      2 - try Benders
      3 - try Staircase
      Returns number of blocks or zero if no structure
  */
  int decompose(const CoinPackedMatrix & matrix,
		const double * rowLower, const double * rowUpper,
		const double * columnLower, const double * columnUpper,
		const double * objective, int type,int maxBlocks=50,
		double objectiveOffset=0.0);
  
   //@}


  /**@name For getting information */
   //@{
   /// Return number of row blocks
  inline int numberRowBlocks() const
  { return numberRowBlocks_;}
   /// Return number of column blocks
  inline int numberColumnBlocks() const
  { return numberColumnBlocks_;}
   /// Return number of elementBlocks
  inline CoinBigIndex numberElementBlocks() const
  { return numberElementBlocks_;}
   /// Return number of elements
  CoinBigIndex numberElements() const;
  /// Return the i'th row block name
  inline const std::string & getRowBlock(int i) const
  { return rowBlockNames_[i];}
  /// Set i'th row block name
  inline void setRowBlock(int i,const std::string &name) 
  { rowBlockNames_[i] = name;}
  /// Add or check a row block name and number of rows
  int addRowBlock(int numberRows,const std::string &name) ;
  /// Return a row block index given a row block name
  int rowBlock(const std::string &name) const;
  /// Return i'th the column block name
  inline const std::string & getColumnBlock(int i) const
  { return columnBlockNames_[i];}
  /// Set i'th column block name
  inline void setColumnBlock(int i,const std::string &name) 
  { columnBlockNames_[i] = name;}
  /// Add or check a column block name and number of columns
  int addColumnBlock(int numberColumns,const std::string &name) ;
  /// Return a column block index given a column block name
  int columnBlock(const std::string &name) const;
  /// Return i'th block type
  inline const CoinModelBlockInfo &  blockType(int i) const
  { return blockType_[i];}
  /// Return i'th block
  inline CoinBaseModel * block(int i) const
  { return blocks_[i];}
  /// Return block corresponding to row and column
  const CoinBaseModel *  block(int row,int column) const;
  /// Return i'th block as CoinModel (or NULL)
  CoinModel * coinBlock(int i) const;
  /// Return block corresponding to row and column as CoinModel
  const CoinBaseModel *  coinBlock(int row,int column) const;
  /// Return block number corresponding to row and column
  int  blockIndex(int row,int column) const;
  /** Return model as a CoinModel block
      and fill in info structure and update counts
      */
  CoinModel * coinModelBlock(CoinModelBlockInfo & info) ;
  /// Sets given block into coinModelBlocks_
  void setCoinModel(CoinModel * block, int iBlock);
  /// Refresh info in blockType_
  void refresh(int iBlock);
  /** Fill pointers corresponding to row and column */

  CoinModelBlockInfo block(int row,int column,
	     const double * & rowLower, const double * & rowUpper,
	     const double * & columnLower, const double * & columnUpper,
	     const double * & objective) const;
  /// Direction of optimization (1 - minimize, -1 - maximize, 0 - ignore
  inline double optimizationDirection() const {
    return  optimizationDirection_;
  }
  /// Set direction of optimization (1 - minimize, -1 - maximize, 0 - ignore
  inline void setOptimizationDirection(double value)
  { optimizationDirection_=value;}
   //@}

  /**@name Constructors, destructor */
   //@{
   /** Default constructor. */
  CoinStructuredModel();
  /** Read a problem in MPS format from the given filename.
      May try and decompose
   */
  CoinStructuredModel(const char *fileName,int decompose=0,
		      int maxBlocks=50);
   /** Destructor */
   virtual ~CoinStructuredModel();
   //@}

   /**@name Copy method */
   //@{
   /** The copy constructor. */
   CoinStructuredModel(const CoinStructuredModel&);
  /// =
   CoinStructuredModel& operator=(const CoinStructuredModel&);
  /// Clone
  virtual CoinBaseModel * clone() const;
   //@}

private:

  /** Fill in info structure and update counts
      Returns number of inconsistencies on border
  */
  int fillInfo(CoinModelBlockInfo & info,const CoinModel * block);
  /** Fill in info structure and update counts
  */
  void fillInfo(CoinModelBlockInfo & info,const CoinStructuredModel * block);
  /**@name Data members */
   //@{
  /// Current number of row blocks
  int numberRowBlocks_;
  /// Current number of column blocks
  int numberColumnBlocks_;
  /// Current number of element blocks
  int numberElementBlocks_;
  /// Maximum number of element blocks
  int maximumElementBlocks_;
  /// Rowblock name
  std::vector<std::string> rowBlockNames_;
  /// Columnblock name
  std::vector<std::string> columnBlockNames_;
  /// Blocks
  CoinBaseModel ** blocks_;
  /// CoinModel copies of blocks or NULL if original CoinModel
  CoinModel ** coinModelBlocks_;
  /// Which parts of model are set in block
  CoinModelBlockInfo * blockType_;
   //@}
};
#endif
