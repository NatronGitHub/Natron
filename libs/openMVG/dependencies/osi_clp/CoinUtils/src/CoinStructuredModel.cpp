/* $Id: CoinStructuredModel.cpp 1585 2013-04-06 20:42:02Z stefan $ */
// Copyright (C) 2008, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


#include "CoinUtilsConfig.h"
#include "CoinHelperFunctions.hpp"
#include "CoinStructuredModel.hpp"
#include "CoinSort.hpp"
#include "CoinMpsIO.hpp"
#include "CoinFloatEqual.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
CoinStructuredModel::CoinStructuredModel () 
  :  CoinBaseModel(),
     numberRowBlocks_(0),
     numberColumnBlocks_(0),
     numberElementBlocks_(0),
     maximumElementBlocks_(0),
     blocks_(NULL),
     coinModelBlocks_(NULL),
     blockType_(NULL)
{
}
/* Read a problem in MPS or GAMS format from the given filename.
 */
CoinStructuredModel::CoinStructuredModel(const char *fileName, 
					 int decomposeType,
					 int maxBlocks)
  :  CoinBaseModel(),
     numberRowBlocks_(0),
     numberColumnBlocks_(0),
     numberElementBlocks_(0),
     maximumElementBlocks_(0),
     blocks_(NULL),
     coinModelBlocks_(NULL),
     blockType_(NULL)
{
  CoinModel coinModel(fileName,false);
  if (coinModel.numberRows()) {
    problemName_ = coinModel.getProblemName();
    optimizationDirection_ = coinModel.optimizationDirection();
    objectiveOffset_ = coinModel.objectiveOffset();
    if (!decomposeType) {
      addBlock("row_master","column_master",coinModel);
    } else {
      const CoinPackedMatrix * matrix = coinModel.packedMatrix();
      if (!matrix) 
	coinModel.convertMatrix();
      decompose(coinModel,decomposeType,maxBlocks);
      //addBlock("row_master","column_master",coinModel);
    }
  }
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
CoinStructuredModel::CoinStructuredModel (const CoinStructuredModel & rhs) 
  : CoinBaseModel(rhs),
    numberRowBlocks_(rhs.numberRowBlocks_),
    numberColumnBlocks_(rhs.numberColumnBlocks_),
    numberElementBlocks_(rhs.numberElementBlocks_),
    maximumElementBlocks_(rhs.maximumElementBlocks_)
{
  if (maximumElementBlocks_) {
    blocks_ = CoinCopyOfArray(rhs.blocks_,maximumElementBlocks_);
    for (int i=0;i<numberElementBlocks_;i++)
      blocks_[i]= rhs.blocks_[i]->clone();
    blockType_ = CoinCopyOfArray(rhs.blockType_,maximumElementBlocks_);
    if (rhs.coinModelBlocks_) {
      coinModelBlocks_ = CoinCopyOfArray(rhs.coinModelBlocks_,
					 maximumElementBlocks_);
      for (int i=0;i<numberElementBlocks_;i++)
	coinModelBlocks_[i]= new CoinModel(*rhs.coinModelBlocks_[i]);
    } else {
      coinModelBlocks_ = NULL;
    }
  } else {
    blocks_ = NULL;
    blockType_ = NULL;
    coinModelBlocks_ = NULL;
  }
  rowBlockNames_ = rhs.rowBlockNames_;
  columnBlockNames_ = rhs.columnBlockNames_;
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
CoinStructuredModel::~CoinStructuredModel ()
{
  for (int i=0;i<numberElementBlocks_;i++)
    delete blocks_[i];
  delete [] blocks_;
  delete [] blockType_;
  if (coinModelBlocks_) {
    for (int i=0;i<numberElementBlocks_;i++)
      delete coinModelBlocks_[i];
    delete [] coinModelBlocks_;
  }
}

// Clone
CoinBaseModel *
CoinStructuredModel::clone() const
{
  return new CoinStructuredModel(*this);
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
CoinStructuredModel &
CoinStructuredModel::operator=(const CoinStructuredModel& rhs)
{
  if (this != &rhs) {
    CoinBaseModel::operator=(rhs);
    for (int i=0;i<numberElementBlocks_;i++)
      delete blocks_[i];
    delete [] blocks_;
    delete [] blockType_;
    if (coinModelBlocks_) {
      for (int i=0;i<numberElementBlocks_;i++)
	delete coinModelBlocks_[i];
      delete [] coinModelBlocks_;
    }
    numberRowBlocks_ = rhs.numberRowBlocks_;
    numberColumnBlocks_ = rhs.numberColumnBlocks_;
    numberElementBlocks_ = rhs.numberElementBlocks_;
    maximumElementBlocks_ = rhs.maximumElementBlocks_;
    if (maximumElementBlocks_) {
      blocks_ = CoinCopyOfArray(rhs.blocks_,maximumElementBlocks_);
      for (int i=0;i<numberElementBlocks_;i++)
	blocks_[i]= rhs.blocks_[i]->clone();
      blockType_ = CoinCopyOfArray(rhs.blockType_,maximumElementBlocks_);
      if (rhs.coinModelBlocks_) {
	coinModelBlocks_ = CoinCopyOfArray(rhs.coinModelBlocks_,
					   maximumElementBlocks_);
	for (int i=0;i<numberElementBlocks_;i++)
	  coinModelBlocks_[i]= new CoinModel(*rhs.coinModelBlocks_[i]);
      } else {
	coinModelBlocks_ = NULL;
      }
    } else {
      blocks_ = NULL;
      blockType_ = NULL;
      coinModelBlocks_ = NULL;
    }
    rowBlockNames_ = rhs.rowBlockNames_;
    columnBlockNames_ = rhs.columnBlockNames_;
  }
  return *this;
}
static bool sameValues(const double * a, const double *b, int n)
{
  int i;
  for ( i=0;i<n;i++) {
    if (a[i]!=b[i])
      break;
  }
  return (i==n);
}
static bool sameValues(const int * a, const int *b, int n)
{
  int i;
  for ( i=0;i<n;i++) {
    if (a[i]!=b[i])
      break;
  }
  return (i==n);
}
static bool sameValues(const CoinModel * a, const CoinModel *b, bool doRows)
{
  int i=0;
  int n=0;
  if (doRows) {
    n = a->numberRows();
    for ( i=0;i<n;i++) {
      const char * aName = a->getRowName(i);
      const char * bName = b->getRowName(i);
      bool good=true;
      if (aName) {
	if (!bName||strcmp(aName,bName))
	  good=false;
      } else if (bName) {
	good=false;
      }
      if (!good)
	break;
    }
  } else {
    n = a->numberColumns();
    for ( i=0;i<n;i++) {
      const char * aName = a->getColumnName(i);
      const char * bName = b->getColumnName(i);
      bool good=true;
      if (aName) {
	if (!bName||strcmp(aName,bName))
	  good=false;
      } else if (bName) {
	good=false;
      }
      if (!good)
	break;
    }
  }
  return (i==n);
}
// Add a row block name and number of rows
int
CoinStructuredModel::addRowBlock(int numberRows,const std::string &name) 
{
  int iRowBlock;
  for (iRowBlock=0;iRowBlock<numberRowBlocks_;iRowBlock++) {
    if (name==rowBlockNames_[iRowBlock])
      break;
  }
  if (iRowBlock==numberRowBlocks_) {
    rowBlockNames_.push_back(name);
    numberRowBlocks_++;
    numberRows_ += numberRows;
  }
  return iRowBlock;
}
// Return a row block index given a row block name
int 
CoinStructuredModel::rowBlock(const std::string &name) const
{
  int iRowBlock;
  for (iRowBlock=0;iRowBlock<numberRowBlocks_;iRowBlock++) {
    if (name==rowBlockNames_[iRowBlock])
      break;
  }
  if (iRowBlock==numberRowBlocks_) 
    iRowBlock=-1;
  return iRowBlock;
}
// Add a column block name and number of columns
int
CoinStructuredModel::addColumnBlock(int numberColumns,const std::string &name) 
{
  int iColumnBlock;
  for (iColumnBlock=0;iColumnBlock<numberColumnBlocks_;iColumnBlock++) {
    if (name==columnBlockNames_[iColumnBlock])
      break;
  }
  if (iColumnBlock==numberColumnBlocks_) {
    columnBlockNames_.push_back(name);
    numberColumnBlocks_++;
    numberColumns_ += numberColumns;
  }
  return iColumnBlock;
}
// Return a column block index given a column block name
int 
CoinStructuredModel::columnBlock(const std::string &name) const
{
  int iColumnBlock;
  for (iColumnBlock=0;iColumnBlock<numberColumnBlocks_;iColumnBlock++) {
    if (name==columnBlockNames_[iColumnBlock])
      break;
  }
  if (iColumnBlock==numberColumnBlocks_) 
    iColumnBlock=-1;
  return iColumnBlock;
}
// Return number of elements
CoinBigIndex 
CoinStructuredModel::numberElements() const
{
  CoinBigIndex numberElements=0;
  for (int iBlock=0;iBlock<numberElementBlocks_;iBlock++) {
    numberElements += blocks_[iBlock]->numberElements();
  }
  return numberElements;
}
// Return i'th block as CoinModel (or NULL)
CoinModel * 
CoinStructuredModel::coinBlock(int i) const
{ 
  CoinModel * block = dynamic_cast<CoinModel *>(blocks_[i]);
  if (block) 
    return block;
  else if (coinModelBlocks_)
    return coinModelBlocks_[i];
  else
    return NULL;
}

/* Return block as a CoinModel block
   and fill in info structure and update counts
*/
CoinModel *  
CoinStructuredModel::coinModelBlock(CoinModelBlockInfo & info)
{
  // CoinStructuredModel
  int numberRows=this->numberRows();
  int numberColumns =this-> numberColumns();
  int numberRowBlocks=this->numberRowBlocks();
  int numberColumnBlocks =this-> numberColumnBlocks();
  int numberElementBlocks =this-> numberElementBlocks();
  CoinBigIndex numberElements=this->numberElements();
  // See what is needed
  double * rowLower = NULL;
  double * rowUpper = NULL;
  double * columnLower = NULL;
  double * columnUpper = NULL;
  double * objective = NULL;
  int * integerType = NULL;
  info = CoinModelBlockInfo();
  CoinModel ** blocks = new CoinModel * [numberElementBlocks];
  for (int iBlock=0;iBlock<numberElementBlocks;iBlock++) {
    CoinModelBlockInfo thisInfo=blockType_[iBlock];
    CoinStructuredModel * subModel = dynamic_cast<CoinStructuredModel *>(blocks_[iBlock]);
    CoinModel * thisBlock;
    if (subModel) {
      thisBlock = subModel->coinModelBlock(thisInfo);
      fillInfo(thisInfo,subModel);
      setCoinModel(thisBlock,iBlock);
    } else {
      thisBlock = dynamic_cast<CoinModel *>(blocks_[iBlock]);
      assert (thisBlock);
      fillInfo(thisInfo,thisBlock);
    }
    blocks[iBlock]=thisBlock;
    if (thisInfo.rhs&&!info.rhs) {
      info.rhs=1;
      rowLower = new double [numberRows];
      rowUpper = new double [numberRows];
      CoinFillN(rowLower,numberRows,-COIN_DBL_MAX);
      CoinFillN(rowUpper,numberRows,COIN_DBL_MAX);
    }
    if (thisInfo.bounds&&!info.bounds) {
      info.bounds=1;
      columnLower = new double [numberColumns];
      columnUpper = new double [numberColumns];
      objective = new double [numberColumns];
      CoinFillN(columnLower,numberColumns,0.0);
      CoinFillN(columnUpper,numberColumns,COIN_DBL_MAX);
      CoinFillN(objective,numberColumns,0.0);
    }
    if (thisInfo.integer&&!info.integer) {
      info.integer=1;
      integerType = new int [numberColumns];
      CoinFillN(integerType,numberColumns,0);
    }
    if (thisInfo.rowName&&!info.rowName) {
      info.rowName=1;
    }
    if (thisInfo.columnName&&!info.columnName) {
      info.columnName=1;
    }
  }
  // Space for elements
  int * row = new int[numberElements];
  int * column = new int[numberElements];
  double * element = new double[numberElements];
  numberElements=0;
  // Bases for blocks
  int * rowBase = new int[numberRowBlocks];
  CoinFillN(rowBase,numberRowBlocks,-1);
  CoinModelBlockInfo * rowBlockInfo = 
    new CoinModelBlockInfo [numberRowBlocks];
  int * columnBase = new int[numberColumnBlocks];
  CoinFillN(columnBase,numberColumnBlocks,-1);
  CoinModelBlockInfo * columnBlockInfo = 
    new CoinModelBlockInfo [numberColumnBlocks];
  for (int iBlock=0;iBlock<numberElementBlocks;iBlock++) {
    int iRowBlock = rowBlock(blocks[iBlock]->getRowBlock());
    assert (iRowBlock>=0&&iRowBlock<numberRowBlocks);
    if (rowBase[iRowBlock]==-1) 
      rowBase[iRowBlock]=blocks[iBlock]->numberRows();
    else
      assert (rowBase[iRowBlock]==blocks[iBlock]->numberRows());
    int iColumnBlock = columnBlock(blocks[iBlock]->getColumnBlock());
    assert (iColumnBlock>=0&&iColumnBlock<numberColumnBlocks);
    if (columnBase[iColumnBlock]==-1) 
      columnBase[iColumnBlock]=blocks[iBlock]->numberColumns();
    else
      assert (columnBase[iColumnBlock]==blocks[iBlock]->numberColumns());
  }
  int n=0;
  for (int iBlock=0;iBlock<numberRowBlocks;iBlock++) {
    int k = rowBase[iBlock];
    rowBase[iBlock]=n;
    assert (k>=0);
    n+=k;
  }
  assert (n==numberRows);
  n=0;
  for (int iBlock=0;iBlock<numberColumnBlocks;iBlock++) {
    int k = columnBase[iBlock];
    columnBase[iBlock]=n;
    assert (k>=0);
    n+=k;
  }
  assert (n==numberColumns);
  for (int iBlock=0;iBlock<numberElementBlocks;iBlock++) {
    CoinModelBlockInfo thisInfo=blockType_[iBlock];
    CoinModel * thisBlock = blocks[iBlock];
    int iRowBlock = rowBlock(blocks[iBlock]->getRowBlock());
    int iRowBase = rowBase[iRowBlock];
    int nRows = thisBlock->numberRows();
    // could check if identical before error
    if (thisInfo.rhs) { 
      assert (!rowBlockInfo[iRowBlock].rhs);
      rowBlockInfo[iRowBlock].rhs=1;
      memcpy(rowLower+iRowBase,thisBlock->rowLowerArray(),
	     nRows*sizeof(double));
      memcpy(rowUpper+iRowBase,thisBlock->rowUpperArray(),
	     nRows*sizeof(double));
    }
    int iColumnBlock = columnBlock(blocks[iBlock]->getColumnBlock());
    int iColumnBase = columnBase[iColumnBlock];
    int nColumns = thisBlock->numberColumns();
    if (thisInfo.bounds) {
      assert (!columnBlockInfo[iColumnBlock].bounds);
      columnBlockInfo[iColumnBlock].bounds=1;
      memcpy(columnLower+iColumnBase,thisBlock->columnLowerArray(),
	     nColumns*sizeof(double));
      memcpy(columnUpper+iColumnBase,thisBlock->columnUpperArray(),
	     nColumns*sizeof(double));
      memcpy(objective+iColumnBase,thisBlock->objectiveArray(),
	     nColumns*sizeof(double));
    }
    if (thisInfo.integer) {
      assert (!columnBlockInfo[iColumnBlock].integer);
      columnBlockInfo[iColumnBlock].integer=1;
      memcpy(integerType+iColumnBase,thisBlock->integerTypeArray(),
	     nColumns*sizeof(int));
    }
    const CoinPackedMatrix * elementBlock = thisBlock->packedMatrix();
    // get matrix data pointers
    const int * row2 = elementBlock->getIndices();
    const CoinBigIndex * columnStart = elementBlock->getVectorStarts();
    const double * elementByColumn = elementBlock->getElements();
    const int * columnLength = elementBlock->getVectorLengths(); 
    int n = elementBlock->getNumCols();
    assert (elementBlock->isColOrdered());
    for (int iColumn=0;iColumn<n;iColumn++) {
      CoinBigIndex j;
      int jColumn = iColumn+iColumnBase;
      for (j=columnStart[iColumn];
	   j<columnStart[iColumn]+columnLength[iColumn];j++) {
	row[numberElements]=row2[j]+iRowBase;
	column[numberElements]=jColumn;
	element[numberElements++]=elementByColumn[j];
	}
    }
  }
  delete [] rowBlockInfo;
  delete [] columnBlockInfo;
  CoinPackedMatrix matrix(true,row,column,element,numberElements);
  if (numberElements)
    info.matrix=1;
  delete [] row;
  delete [] column;
  delete [] element;
  CoinModel * block = new CoinModel(numberRows, numberColumns,
			&matrix,
			rowLower, rowUpper, 
			columnLower, columnUpper, objective);
  delete [] rowLower;
  delete [] rowUpper;
  delete [] columnLower;
  delete [] columnUpper;
  delete [] objective;
  // Do integers if wanted
  if (integerType) {
    for (int iColumn=0;iColumn<numberColumns;iColumn++) {
      block->setColumnIsInteger(iColumn,integerType[iColumn]!=0);
    }
  }
  delete [] integerType;
  block->setObjectiveOffset(objectiveOffset());
  if (info.rowName||info.columnName) {
    for (int iBlock=0;iBlock<numberElementBlocks;iBlock++) {
      CoinModelBlockInfo thisInfo;
      CoinModel * thisBlock = blocks[iBlock];
      int iRowBlock = rowBlock(thisBlock->getRowBlock());
      int iRowBase = rowBase[iRowBlock];
      if (thisInfo.rowName) {
	int numberItems = thisBlock->rowNames()->numberItems();
	assert( thisBlock->numberRows()>=numberItems);
	if (numberItems) {
	  const char *const * rowNames=thisBlock->rowNames()->names();
	  for (int i=0;i<numberItems;i++) {
	    std::string name = rowNames[i];
	    block->setRowName(i+iRowBase,name.c_str());
	  }
	}
      }
      int iColumnBlock = columnBlock(thisBlock->getColumnBlock());
      int iColumnBase = columnBase[iColumnBlock];
      if (thisInfo.columnName) {
	int numberItems = thisBlock->columnNames()->numberItems();
	assert( thisBlock->numberColumns()>=numberItems);
	if (numberItems) {
	  const char *const * columnNames=thisBlock->columnNames()->names();
	  for (int i=0;i<numberItems;i++) {
	    std::string name = columnNames[i];
	    block->setColumnName(i+iColumnBase,name.c_str());
	  }
	}
      }
    }
  }
  delete [] rowBase;
  delete [] columnBase;
  for (int iBlock=0;iBlock<numberElementBlocks;iBlock++) {
    if (static_cast<CoinBaseModel *> (blocks[iBlock])!=
	static_cast<CoinBaseModel *> (blocks_[iBlock]))
      delete blocks[iBlock];
  }
  delete [] blocks;
  return block;
}
// Sets given block into coinModelBlocks_
void 
CoinStructuredModel::setCoinModel(CoinModel * block, int iBlock)
{
 if (!coinModelBlocks_) {
    coinModelBlocks_ = new CoinModel * [maximumElementBlocks_];
    CoinZeroN(coinModelBlocks_,maximumElementBlocks_);
  }
  delete coinModelBlocks_[iBlock];
  coinModelBlocks_[iBlock]=block;
 }
// Refresh info in blockType_
void 
CoinStructuredModel::refresh(int iBlock)
{
  fillInfo(blockType_[iBlock],coinBlock(iBlock));
}
/* Fill in info structure and update counts
   Returns number of inconsistencies on border
*/
int 
CoinStructuredModel::fillInfo(CoinModelBlockInfo & info,
			      const CoinModel * block) 
{
  int whatsSet = block->whatIsSet();
  info.matrix = static_cast<char>(((whatsSet&1)!=0) ? 1 : 0);
  info.rhs = static_cast<char>(((whatsSet&2)!=0) ? 1 : 0);
  info.rowName = static_cast<char>(((whatsSet&4)!=0) ? 1 : 0);
  info.integer = static_cast<char>(((whatsSet&32)!=0) ? 1 : 0);
  info.bounds = static_cast<char>(((whatsSet&8)!=0) ? 1 : 0);
  info.columnName = static_cast<char>(((whatsSet&16)!=0) ? 1 : 0);
  int numberRows = block->numberRows();
  int numberColumns = block->numberColumns();
  // Which block
  int iRowBlock=addRowBlock(numberRows,block->getRowBlock());
  info.rowBlock=iRowBlock;
  int iColumnBlock=addColumnBlock(numberColumns,block->getColumnBlock());
  info.columnBlock=iColumnBlock;
  int numberErrors=0;
  CoinModelBlockInfo sumInfo=blockType_[numberElementBlocks_-1];
  int iRhs=(sumInfo.rhs) ? numberElementBlocks_-1 : -1;
  int iRowName=(sumInfo.rowName) ? numberElementBlocks_-1 : -1;
  int iBounds=(sumInfo.bounds) ? numberElementBlocks_-1 : -1;
  int iColumnName=(sumInfo.columnName) ? numberElementBlocks_-1 : -1;
  int iInteger=(sumInfo.integer) ? numberElementBlocks_-1 : -1;
  for (int i=0;i<numberElementBlocks_-1;i++) {
    if (iRowBlock==blockType_[i].rowBlock) {
      if (numberRows!=blocks_[i]->numberRows())
	numberErrors+=1000;
      if (blockType_[i].rhs) {
	if (iRhs<0) {
	  iRhs=i;
	} else {
	  // check
	  const double * a = static_cast<CoinModel *>(blocks_[iRhs])->rowLowerArray();
	  const double * b = static_cast<CoinModel *>(blocks_[i])->rowLowerArray();
	  if (!sameValues(a,b,numberRows))
	    numberErrors++;
	  a = static_cast<CoinModel *>(blocks_[iRhs])->rowUpperArray();
	  b = static_cast<CoinModel *>(blocks_[i])->rowUpperArray();
	  if (!sameValues(a,b,numberRows))
	    numberErrors++;
	}
      }
      if (blockType_[i].rowName) {
	if (iRowName<0) {
	  iRowName=i;
	} else {
	  // check
	  if (!sameValues(static_cast<CoinModel *>(blocks_[iRowName]),
			  static_cast<CoinModel *>(blocks_[i]),true))
	    numberErrors++;
	}
      }
    }
    if (iColumnBlock==blockType_[i].columnBlock) {
      if (numberColumns!=blocks_[i]->numberColumns())
	numberErrors+=1000;
      if (blockType_[i].bounds) {
	if (iBounds<0) {
	  iBounds=i;
	} else {
	  // check
	  const double * a = static_cast<CoinModel *>(blocks_[iBounds])->columnLowerArray();
	  const double * b = static_cast<CoinModel *>(blocks_[i])->columnLowerArray();
	  if (!sameValues(a,b,numberColumns))
	    numberErrors++;
	  a = static_cast<CoinModel *>(blocks_[iBounds])->columnUpperArray();
	  b = static_cast<CoinModel *>(blocks_[i])->columnUpperArray();
	  if (!sameValues(a,b,numberColumns))
	    numberErrors++;
	  a = static_cast<CoinModel *>(blocks_[iBounds])->objectiveArray();
	  b = static_cast<CoinModel *>(blocks_[i])->objectiveArray();
	  if (!sameValues(a,b,numberColumns))
	    numberErrors++;
	}
      }
      if (blockType_[i].columnName) {
	if (iColumnName<0) {
	  iColumnName=i;
	} else {
	  // check
	  if (!sameValues(static_cast<CoinModel *>(blocks_[iColumnName]),
			  static_cast<CoinModel *>(blocks_[i]),false))
	    numberErrors++;
	}
      }
      if (blockType_[i].integer) {
	if (iInteger<0) {
	  iInteger=i;
	} else {
	  // check
	  const int * a = static_cast<CoinModel *>(blocks_[iInteger])->integerTypeArray();
	  const int * b = static_cast<CoinModel *>(blocks_[i])->integerTypeArray();
	  if (!sameValues(a,b,numberColumns))
	    numberErrors++;
	}
      }
    }
  }
  return numberErrors;
}
/* Fill in info structure and update counts
*/
void
CoinStructuredModel::fillInfo(CoinModelBlockInfo & info,
			      const CoinStructuredModel * block) 
{
  int numberRows = block->numberRows();
  int numberColumns = block->numberColumns();
  // Which block
  int iRowBlock=addRowBlock(numberRows,block->rowBlockName_);
  info.rowBlock=iRowBlock;
  int iColumnBlock=addColumnBlock(numberColumns,block->columnBlockName_);
  info.columnBlock=iColumnBlock;
}
/* add a block from a CoinModel without names*/
int 
CoinStructuredModel::addBlock(const std::string & rowBlock,
			      const std::string & columnBlock,
			      const CoinBaseModel & block)
{
  CoinBaseModel * block2 = block.clone();
  return addBlock(rowBlock,columnBlock,block2);
}
/* add a block using names 
 */
int 
CoinStructuredModel::addBlock(const std::string & rowBlock,
			      const std::string & columnBlock,
			      const CoinPackedMatrix & matrix,
			      const double * rowLower, const double * rowUpper,
			      const double * columnLower, const double * columnUpper,
			      const double * objective)
{
  CoinModel * block = new CoinModel();
  block->loadBlock(matrix,columnLower,columnUpper,objective,
		   rowLower,rowUpper);
  return addBlock(rowBlock,columnBlock,block);
}
/* add a block from a CoinModel without names*/
int 
CoinStructuredModel::addBlock(const std::string & rowBlock,
			      const std::string & columnBlock,
			      CoinBaseModel * block)
{
  if (numberElementBlocks_==maximumElementBlocks_) {
    maximumElementBlocks_ = 3*(maximumElementBlocks_+10)/2;
    CoinBaseModel ** temp = new CoinBaseModel * [maximumElementBlocks_];
    memcpy(temp,blocks_,numberElementBlocks_*sizeof(CoinBaseModel *));
    delete [] blocks_;
    blocks_ = temp;
    CoinModelBlockInfo * temp2 = new CoinModelBlockInfo [maximumElementBlocks_];
    memcpy(temp2,blockType_,numberElementBlocks_*sizeof(CoinModelBlockInfo));
    delete [] blockType_;
    blockType_ = temp2;
    if (coinModelBlocks_) {
      CoinModel ** temp = new CoinModel * [maximumElementBlocks_];
      CoinZeroN(temp,maximumElementBlocks_);
      memcpy(temp,coinModelBlocks_,numberElementBlocks_*sizeof(CoinModel *));
      delete [] coinModelBlocks_;
      coinModelBlocks_ = temp;
    }
  }
  blocks_[numberElementBlocks_++]=block;
  block->setRowBlock(rowBlock);
  block->setColumnBlock(columnBlock);
  int numberErrors=0;
  CoinModel * coinBlock = dynamic_cast<CoinModel *>(block);
  if (coinBlock) {
    // Convert matrix
    if (coinBlock->type()!=3)
      coinBlock->convertMatrix();
    numberErrors=fillInfo(blockType_[numberElementBlocks_-1],coinBlock);
  } else {
    CoinStructuredModel * subModel = dynamic_cast<CoinStructuredModel *>(block);
    assert (subModel);
    CoinModel * blockX = 
      subModel->coinModelBlock(blockType_[numberElementBlocks_-1]);
    fillInfo(blockType_[numberElementBlocks_-1],subModel);
    setCoinModel(blockX,numberElementBlocks_-1);
  }
  return numberErrors;
}

/* add a block from a CoinModel with names*/
int
CoinStructuredModel::addBlock(const CoinBaseModel & block)
{
  
  //inline const std::string & getRowBlock() const
  //abort();
  return addBlock(block.getRowBlock(),block.getColumnBlock(),
		  block);
}
/* Decompose a model specified as arrays + CoinPackedMatrix
   1 - try D-W
   2 - try Benders
   3 - try Staircase
   Returns number of blocks or zero if no structure
*/
int 
CoinStructuredModel::decompose(const CoinPackedMatrix & matrix,
			       const double * rowLower, const double * rowUpper,
			       const double * columnLower, const double * columnUpper,
			       const double * objective, int type,int maxBlocks,
			       double objectiveOffset)
{
  setObjectiveOffset(objectiveOffset);
  int numberBlocks=0;
  if (type==1) {
    // get row copy
    CoinPackedMatrix rowCopy = matrix;
    rowCopy.reverseOrdering();
    const int * row = matrix.getIndices();
    const int * columnLength = matrix.getVectorLengths();
    const CoinBigIndex * columnStart = matrix.getVectorStarts();
    //const double * elementByColumn = matrix.getElements();
    const int * column = rowCopy.getIndices();
    const int * rowLength = rowCopy.getVectorLengths();
    const CoinBigIndex * rowStart = rowCopy.getVectorStarts();
    //const double * elementByRow = rowCopy.getElements();
    int numberRows = matrix.getNumRows();
    int * rowBlock = new int[numberRows+1];
    int iRow;
    // Row counts (maybe look at long rows for master)
    CoinZeroN(rowBlock,numberRows+1);
    for (iRow=0;iRow<numberRows;iRow++) {
      int length = rowLength[iRow];
      rowBlock[length]++;
    }
    for (iRow=0;iRow<numberRows+1;iRow++) {
      if (rowBlock[iRow])
	printf("%d rows have %d elements\n",rowBlock[iRow],iRow);
    }
    bool newWay=true;
    // to say if column looked at
    int numberColumns = matrix.getNumCols();
    int * columnBlock = new int[numberColumns];
    int iColumn;
    int * whichRow = new int [numberRows];
    int * whichColumn = new int [numberColumns];
    int * stack = new int [numberRows];
    if (newWay) {
      //double best2[3]={COIN_DBL_MAX,COIN_DBL_MAX,COIN_DBL_MAX};
      double best2[3]={0.0,0.0,0.0};
      int row2[3]={-1,-1,-1};
      // try forward and backward and sorted
      for (int iWay=0;iWay<3;iWay++) {
	if (iWay==0) {
	  // forwards
	  for (int i=0;i<numberRows;i++)
	    stack[i]=i;
	} else if (iWay==1) {
	  // backwards
	  for (int i=0;i<numberRows;i++)
	    stack[i]=numberRows-1-i;
	} else {
	  // sparsest first
	  for (int i=0;i<numberRows;i++) {
	    rowBlock[i]=rowLength[i];
	    stack[i]=i;
	  }
	  CoinSort_2(rowBlock,rowBlock+numberRows,stack);
	}
	CoinFillN(rowBlock,numberRows,-1);
	rowBlock[numberRows]=0;
	CoinFillN(whichRow,numberRows,-1);
	CoinFillN(columnBlock,numberColumns,-1);
	CoinFillN(whichColumn,numberColumns,-1);
	int numberMarkedColumns = 0;
	numberBlocks=0;
	int bestRow = -1;
	int maximumInBlock = 0;
	int rowsDone=0;
	int checkAfterRows = (5*numberRows)/10+1;
#define OSL_WAY
#ifdef OSL_WAY
	double best = COIN_DBL_MAX;
	int bestRowsDone=-1;
#else
	double best = 0.0; //COIN_DBL_MAX;
#endif
	int numberGoodBlocks=0;
	
	for (int kRow=0;kRow<numberRows;kRow++) {
	  iRow = stack[kRow];
	  CoinBigIndex start = rowStart[iRow];
	  CoinBigIndex end = start+rowLength[iRow];
	  int iBlock=-1;
	  for (CoinBigIndex j=start;j<end;j++) {
	    int iColumn = column[j];
	    if (columnBlock[iColumn]>=0) {
	      // already marked
	      if (iBlock<0) {
		iBlock = columnBlock[iColumn];
	      } else if (iBlock != columnBlock[iColumn]) {
		// join two blocks
		int jBlock = columnBlock[iColumn];
		numberGoodBlocks--;
		// Increase count of iBlock
		rowBlock[iBlock] += rowBlock[jBlock];
		rowBlock[jBlock]=0;
		// First column of block jBlock
		int jColumn = whichRow[jBlock];
		while (jColumn>=0) {
		  columnBlock[jColumn]=iBlock;
		  iColumn = jColumn;
		  jColumn = whichColumn[jColumn];
		}
		whichColumn[iColumn] = whichRow[iBlock];
		whichRow[iBlock] = whichRow[jBlock];
		whichRow[jBlock]=-1;
	      }
	    }
	  }
	  int n=end-start;
	  // If not in block - then start one
	  if (iBlock<0) {
	    // unless null row
	    if (n) {
	      iBlock = numberBlocks;
	      numberBlocks++;
	      numberGoodBlocks++;
	      int jColumn = column[start];
	      columnBlock[jColumn]=iBlock;
	      whichRow[iBlock]=jColumn;
	      numberMarkedColumns += n;
	      rowBlock[iBlock] = n;
	      for (CoinBigIndex j=start+1;j<end;j++) {
		int iColumn = column[j];
		columnBlock[iColumn]=iBlock;
		whichColumn[jColumn]=iColumn;
		jColumn=iColumn;
	      }
	    } else {
	      rowBlock[numberRows]++;
	    }
	  } else {
	    // add all to this block if not already in
	    int jColumn = whichRow[iBlock];
	    for (CoinBigIndex j=start;j<end;j++) {
	      int iColumn = column[j];
	      if (columnBlock[iColumn]<0) {
		numberMarkedColumns++;
		rowBlock[iBlock]++;
		columnBlock[iColumn]=iBlock;
		whichColumn[iColumn]=jColumn;
		jColumn=iColumn;
	      }
	    }
	    whichRow[iBlock]=jColumn;
	  }
#if 0
	  {
	    int nn=0;
	    int * temp = new int [numberRows];
	    CoinZeroN(temp,numberRows);
	    for (int i=0;i<numberColumns;i++) {
	      int iBlock = columnBlock[i];
	      if (iBlock>=0) {
		nn++;
		assert (iBlock<numberBlocks);
		temp[iBlock]++;
	      }
	    }
	    for (int i=0;i<numberBlocks;i++)
	      assert (temp[i]==rowBlock[i]);
	    assert (nn==numberMarkedColumns);
	    for (int i=0;i<numberBlocks;i++) {
	      // First column of block i
	      int jColumn = whichRow[i];
	      int n=0;
	      while (jColumn>=0) {
		n++;
		jColumn = whichColumn[jColumn];
	      }
	      assert (n==temp[i]);
	    }
	    delete [] temp;
	  }
#endif
	  rowsDone++;
	  if (iBlock>=0) 
	    maximumInBlock = CoinMax(maximumInBlock,rowBlock[iBlock]);
	  if (rowsDone>=checkAfterRows) {
	    assert (numberGoodBlocks>0);
	    double averageSize = static_cast<double>(numberMarkedColumns)/
	      static_cast<double>(numberGoodBlocks);
#ifndef OSL_WAY
	    double notionalBlocks = static_cast<double>(numberMarkedColumns)/
	      averageSize;
	    if (maximumInBlock<3*averageSize&&numberGoodBlocks>2) {
	      if(best*(numberRows-rowsDone) < notionalBlocks) {
		best = notionalBlocks/
		  static_cast<double> (numberRows-rowsDone); 
		bestRow = kRow;
	      }
	    }
#else
	    if (maximumInBlock*10<numberColumns*11&&numberGoodBlocks>1) {
	      double test = maximumInBlock + 0.0*averageSize;
	      if(best*static_cast<double>(rowsDone) > test) {
		best = test/static_cast<double> (rowsDone); 
		bestRow = kRow;
		bestRowsDone=rowsDone;
	      }
	    }
#endif
	  }	      
	}
#ifndef OSL_WAY
	best2[iWay]=best;
#else
	if (bestRowsDone<numberRows)
	  best2[iWay]=-(numberRows-bestRowsDone);
	else
	  best2[iWay]=-numberRows;
#endif
	row2[iWay]=bestRow;
      }
      // mark rows
      int nMaster;
      CoinFillN(rowBlock,numberRows,-2);
      if (best2[2]<best2[0]||best2[2]<best2[1]) {
	int iRow1;
	int iRow2;
	if (best2[0]>best2[1]) {
	  // Bottom rows in master
	  iRow1=row2[0]+1;
	  iRow2=numberRows;
	} else {
	  // Top rows in master
	  iRow1=0;
	  iRow2=numberRows-row2[1];
	}
	nMaster = iRow2-iRow1;
	CoinFillN(rowBlock+iRow1,nMaster,-1);
      } else {
	// sorted
	// Bottom rows in master (in order)
	int iRow1=row2[2]+1;
	nMaster = numberRows-iRow1;
	for (int i=iRow1;i<numberRows;i++)
	  rowBlock[stack[i]]=-1;
      }
      if (nMaster*2>numberRows) {
	printf("%d rows out of %d would be in master - no good\n",
	       nMaster,numberRows);
	delete [] rowBlock;
	delete [] columnBlock;
	delete [] whichRow;
	delete [] whichColumn;
	delete [] stack;
	CoinModel model(numberRows,numberColumns,&matrix, rowLower, rowUpper,
			columnLower,columnUpper,objective);
	model.setObjectiveOffset(objectiveOffset);
	addBlock("row_master","column_master",model);
	return 0;
      }
    } else {
      for (iRow=0;iRow<numberRows;iRow++)
	rowBlock[iRow]=-2;
      // these are master rows
      if (numberRows==105127) {
	// ken-18
	for (iRow=104976;iRow<numberRows;iRow++)
	  rowBlock[iRow]=-1;
      } else if (numberRows==2426) {
	// ken-7
	for (iRow=2401;iRow<numberRows;iRow++)
	  rowBlock[iRow]=-1;
      } else if (numberRows==810) {
	for (iRow=81;iRow<84;iRow++)
	  rowBlock[iRow]=-1;
      } else if (numberRows==5418) {
	for (iRow=564;iRow<603;iRow++)
	  rowBlock[iRow]=-1;
      } else if (numberRows==10280) {
	// osa-60
	for (iRow=10198;iRow<10280;iRow++)
	  rowBlock[iRow]=-1;
      } else if (numberRows==1503) {
	// degen3
	for (iRow=0;iRow<561;iRow++)
	  rowBlock[iRow]=-1;
      } else if (numberRows==929) {
	// czprob
	for (iRow=0;iRow<39;iRow++)
	  rowBlock[iRow]=-1;
      } else if (numberRows==33874) {
	// pds-20
	for (iRow=31427;iRow<33874;iRow++)
	  rowBlock[iRow]=-1;
      } else if (numberRows==24902) {
	// allgrade
	int kRow=818;
	for (iRow=0;iRow<kRow;iRow++)
	  rowBlock[iRow]=-1;
      }
    }
    numberBlocks=0;
    CoinFillN(columnBlock,numberColumns,-2);
    for (iColumn=0;iColumn<numberColumns;iColumn++) {
      int kstart = columnStart[iColumn];
      int kend = columnStart[iColumn]+columnLength[iColumn];
      if (columnBlock[iColumn]==-2) {
	// column not allocated
	int j;
	int nstack=0;
	for (j=kstart;j<kend;j++) {
	  int iRow= row[j];
	  if (rowBlock[iRow]!=-1) {
	    assert(rowBlock[iRow]==-2);
	    rowBlock[iRow]=numberBlocks; // mark
	    stack[nstack++] = iRow;
	  }
	}
	if (nstack) {
	  // new block - put all connected in
	  numberBlocks++;
	  columnBlock[iColumn]=numberBlocks-1;
	  while (nstack) {
	    int iRow = stack[--nstack];
	    int k;
	    for (k=rowStart[iRow];k<rowStart[iRow]+rowLength[iRow];k++) {
	      int iColumn = column[k];
	      int kkstart = columnStart[iColumn];
	      int kkend = kkstart + columnLength[iColumn];
	      if (columnBlock[iColumn]==-2) {
		columnBlock[iColumn]=numberBlocks-1; // mark
		// column not allocated
		int jj;
		for (jj=kkstart;jj<kkend;jj++) {
		  int jRow= row[jj];
		  if (rowBlock[jRow]==-2) {
		    rowBlock[jRow]=numberBlocks-1;
		    stack[nstack++]=jRow;
		  }
		}
	      } else {
		assert (columnBlock[iColumn]==numberBlocks-1);
	      }
	    }
	  }
	} else {
	  // Only in master
	  columnBlock[iColumn]=-1;
	}
      }
    }
    delete [] stack;
    int numberMasterRows=0;
    for (iRow=0;iRow<numberRows;iRow++) {
      int iBlock = rowBlock[iRow];
      if (iBlock==-1)
	numberMasterRows++;
    }
    int numberMasterColumns=0;
    for (iColumn=0;iColumn<numberColumns;iColumn++) {
      int iBlock = columnBlock[iColumn];
      if (iBlock==-1)
	numberMasterColumns++;
    }
    if (numberBlocks<=maxBlocks)
      printf("%d blocks found - %d rows, %d columns in master\n",
	     numberBlocks,numberMasterRows,numberMasterColumns);
    else
      printf("%d blocks found (reduced to %d) - %d rows, %d columns in master\n",
	     numberBlocks,maxBlocks,numberMasterRows,numberMasterColumns);
    if (numberBlocks) {
      if (numberBlocks>maxBlocks) {
	int iBlock;
	for (iRow=0;iRow<numberRows;iRow++) {
	  iBlock = rowBlock[iRow];
	  if (iBlock>=0)
	    rowBlock[iRow] = iBlock%maxBlocks;
	}
	for (iColumn=0;iColumn<numberColumns;iColumn++) {
	  iBlock = columnBlock[iColumn];
	  if (iBlock>=0)
	    columnBlock[iColumn] = iBlock%maxBlocks;
	}
	numberBlocks=maxBlocks;
      }
    }
    // make up problems
    // Create all sub problems
    // Space for creating
    double * obj = new double [numberColumns];
    double * columnLo = new double [numberColumns];
    double * columnUp = new double [numberColumns];
    double * rowLo = new double [numberRows];
    double * rowUp = new double [numberRows];
    // Counts
    int * rowCount = reinterpret_cast<int *>(rowLo);
    CoinZeroN(rowCount,numberBlocks);
    for (int i=0;i<numberRows;i++) {
      int iBlock=rowBlock[i];
      if (iBlock>=0)
	rowCount[iBlock]++;
    }
    // allocate empty rows
    for (int i=0;i<numberRows;i++) {
      int iBlock=rowBlock[i];
      if (iBlock==-2) {
	// find block with smallest count
	int iSmall=-1;
	int smallest = numberRows;
	for (int j=0;j<numberBlocks;j++) {
	  if (rowCount[j]<smallest) {
	    iSmall=j;
	    smallest=rowCount[j];
	  }
	}
	rowBlock[i]=iSmall;
	rowCount[iSmall]++;
      }
    }
    int * columnCount = reinterpret_cast<int *>(rowUp);
    CoinZeroN(columnCount,numberBlocks);
    for (int i=0;i<numberColumns;i++) {
      int iBlock=columnBlock[i];
      if (iBlock>=0)
	columnCount[iBlock]++;
    }
    int maximumSize=0;
    for (int i=0;i<numberBlocks;i++) {
      printf("Block %d has %d rows and %d columns\n",i,
	     rowCount[i],columnCount[i]);
      int k=2*rowCount[i]+columnCount[i];
      maximumSize = CoinMax(maximumSize,k);
    }
    if (maximumSize*10>4*(2*numberRows+numberColumns)) {
      // No good
      printf("Doesn't look good\n");
      delete [] rowBlock;
      delete [] columnBlock;
      delete [] whichRow;
      delete [] whichColumn;
      delete [] obj ; 
      delete [] columnLo ; 
      delete [] columnUp ; 
      delete [] rowLo ; 
      delete [] rowUp ; 
      CoinModel model(numberRows,numberColumns,&matrix, rowLower, rowUpper,
		      columnLower,columnUpper,objective);
      model.setObjectiveOffset(objectiveOffset);
      addBlock("row_master","column_master",model);
      return 0;
    }
    // Name for master so at top
    addRowBlock(numberMasterRows,"row_master");
    // Arrays
    // get full matrix
    CoinPackedMatrix fullMatrix = matrix;
    int numberRow2,numberColumn2;
    int iBlock;
    for (iBlock=0;iBlock<numberBlocks;iBlock++) {
      char rowName[20];
      sprintf(rowName,"row_%d",iBlock);
      char columnName[20];
      sprintf(columnName,"column_%d",iBlock);
      numberRow2=0;
      numberColumn2=0;
      for (iRow=0;iRow<numberRows;iRow++) {
	if (iBlock==rowBlock[iRow]) {
	  rowLo[numberRow2]=rowLower[iRow];
	  rowUp[numberRow2]=rowUpper[iRow];
	  whichRow[numberRow2++]=iRow;
	}
      }
      for (iColumn=0;iColumn<numberColumns;iColumn++) {
	if (iBlock==columnBlock[iColumn]) {
	  obj[numberColumn2]=objective[iColumn];
	  columnLo[numberColumn2]=columnLower[iColumn];
	  columnUp[numberColumn2]=columnUpper[iColumn];
	  whichColumn[numberColumn2++]=iColumn;
	}
      }
      // Diagonal block
      CoinPackedMatrix mat(fullMatrix,
			   numberRow2,whichRow,
			   numberColumn2,whichColumn);
      // make sure correct dimensions
      mat.setDimensions(numberRow2,numberColumn2);
      CoinModel * block = new CoinModel(numberRow2,numberColumn2,&mat,
					rowLo,rowUp,NULL,NULL,NULL);
      block->setOriginalIndices(whichRow,whichColumn);
      addBlock(rowName,columnName,block); // takes ownership
      // and top block
      numberRow2=0;
      // get top matrix
      for (iRow=0;iRow<numberRows;iRow++) {
	int iBlock = rowBlock[iRow];
	if (iBlock==-1) {
	  whichRow[numberRow2++]=iRow;
	}
      }
      CoinPackedMatrix top(fullMatrix,
			   numberRow2,whichRow,
			   numberColumn2,whichColumn);
      // make sure correct dimensions
      top.setDimensions(numberRow2,numberColumn2);
      block = new CoinModel(numberMasterRows,numberColumn2,&top,
			    NULL,NULL,columnLo,columnUp,obj);
      block->setOriginalIndices(whichRow,whichColumn);
      addBlock("row_master",columnName,block); // takes ownership
    }
    // and master
    numberRow2=0;
    numberColumn2=0;
    for (iRow=0;iRow<numberRows;iRow++) {
      int iBlock = rowBlock[iRow];
      if (iBlock==-1) {
	rowLo[numberRow2]=rowLower[iRow];
	rowUp[numberRow2]=rowUpper[iRow];
	whichRow[numberRow2++]=iRow;
      }
    }
    for (iColumn=0;iColumn<numberColumns;iColumn++) {
      int iBlock = columnBlock[iColumn];
      if (iBlock<0) {
	obj[numberColumn2]=objective[iColumn];
	columnLo[numberColumn2]=columnLower[iColumn];
	columnUp[numberColumn2]=columnUpper[iColumn];
	whichColumn[numberColumn2++]=iColumn;
      }
    }
    delete [] rowBlock;
    delete [] columnBlock;
    CoinPackedMatrix top(fullMatrix,
			 numberRow2,whichRow,
			 numberColumn2,whichColumn);
    // make sure correct dimensions
    top.setDimensions(numberRow2,numberColumn2);
    CoinModel * block = new CoinModel(numberRow2,numberColumn2,&top,
				      rowLo,rowUp,
				      columnLo,columnUp,obj);
    block->setOriginalIndices(whichRow,whichColumn);
    addBlock("row_master","column_master",block); // takes ownership
    delete [] whichRow;
    delete [] whichColumn;
    delete [] obj ; 
    delete [] columnLo ; 
    delete [] columnUp ; 
    delete [] rowLo ; 
    delete [] rowUp ; 
  } else if (type==2) {
    // get row copy
    CoinPackedMatrix rowCopy = matrix;
    rowCopy.reverseOrdering();
    const int * row = matrix.getIndices();
    const int * columnLength = matrix.getVectorLengths();
    const CoinBigIndex * columnStart = matrix.getVectorStarts();
    //const double * elementByColumn = matrix.getElements();
    const int * column = rowCopy.getIndices();
    const int * rowLength = rowCopy.getVectorLengths();
    const CoinBigIndex * rowStart = rowCopy.getVectorStarts();
    //const double * elementByRow = rowCopy.getElements();
    int numberColumns = matrix.getNumCols();
    int * columnBlock = new int[numberColumns+1];
    int iColumn;
    // Column counts (maybe look at long columns for master)
    CoinZeroN(columnBlock,numberColumns+1);
    for (iColumn=0;iColumn<numberColumns;iColumn++) {
      int length = columnLength[iColumn];
      columnBlock[length]++;
    }
    for (iColumn=0;iColumn<numberColumns+1;iColumn++) {
      if (columnBlock[iColumn])
	printf("%d columns have %d elements\n",columnBlock[iColumn],iColumn);
    }
    bool newWay=false;
    // to say if row looked at
    int numberRows = matrix.getNumRows();
    int * rowBlock = new int[numberRows];
    int iRow;
    int * whichRow = new int [numberRows];
    int * whichColumn = new int [numberColumns];
    int * stack = new int [numberColumns];
    if (newWay) {
      //double best2[3]={COIN_DBL_MAX,COIN_DBL_MAX,COIN_DBL_MAX};
      double best2[3]={0.0,0.0,0.0};
      int column2[3]={-1,-1,-1};
      // try forward and backward and sorted
      for (int iWay=0;iWay<3;iWay++) {
	if (iWay==0) {
	  // forwards
	  for (int i=0;i<numberColumns;i++)
	    stack[i]=i;
	} else if (iWay==1) {
	  // backwards
	  for (int i=0;i<numberColumns;i++)
	    stack[i]=numberColumns-1-i;
	} else {
	  // sparsest first
	  for (int i=0;i<numberColumns;i++) {
	    columnBlock[i]=columnLength[i];
	    stack[i]=i;
	  }
	  CoinSort_2(columnBlock,columnBlock+numberColumns,stack);
	}
	CoinFillN(columnBlock,numberColumns,-1);
	columnBlock[numberColumns]=0;
	CoinFillN(whichColumn,numberColumns,-1);
	CoinFillN(rowBlock,numberRows,-1);
	CoinFillN(whichRow,numberRows,-1);
	int numberMarkedRows = 0;
	numberBlocks=0;
	int bestColumn = -1;
	int maximumInBlock = 0;
	int columnsDone=0;
	int checkAfterColumns = (5*numberColumns)/10+1;
#ifdef OSL_WAY
	double best = COIN_DBL_MAX;
	int bestColumnsDone=-1;
#else
	double best = 0.0; //COIN_DBL_MAX;
#endif
	int numberGoodBlocks=0;
	
	for (int kColumn=0;kColumn<numberColumns;kColumn++) {
	  iColumn = stack[kColumn];
	  CoinBigIndex start = columnStart[iColumn];
	  CoinBigIndex end = start+columnLength[iColumn];
	  int iBlock=-1;
	  for (CoinBigIndex j=start;j<end;j++) {
	    int iRow = row[j];
	    if (rowBlock[iRow]>=0) {
	      // already marked
	      if (iBlock<0) {
		iBlock = rowBlock[iRow];
	      } else if (iBlock != rowBlock[iRow]) {
		// join two blocks
		int jBlock = rowBlock[iRow];
		numberGoodBlocks--;
		// Increase count of iBlock
		columnBlock[iBlock] += columnBlock[jBlock];
		columnBlock[jBlock]=0;
		// First row of block jBlock
		int jRow = whichColumn[jBlock];
		while (jRow>=0) {
		  rowBlock[jRow]=iBlock;
		  iRow = jRow;
		  jRow = whichRow[jRow];
		}
		whichRow[iRow] = whichColumn[iBlock];
		whichColumn[iBlock] = whichColumn[jBlock];
		whichColumn[jBlock]=-1;
	      }
	    }
	  }
	  int n=end-start;
	  // If not in block - then start one
	  if (iBlock<0) {
	    // unless null column
	    if (n) {
	      iBlock = numberBlocks;
	      numberBlocks++;
	      numberGoodBlocks++;
	      int jRow = row[start];
	      rowBlock[jRow]=iBlock;
	      whichColumn[iBlock]=jRow;
	      numberMarkedRows += n;
	      columnBlock[iBlock] = n;
	      for (CoinBigIndex j=start+1;j<end;j++) {
		int iRow = row[j];
		rowBlock[iRow]=iBlock;
		whichRow[jRow]=iRow;
		jRow=iRow;
	      }
	    } else {
	      columnBlock[numberColumns]++;
	    }
	  } else {
	    // add all to this block if not already in
	    int jRow = whichColumn[iBlock];
	    for (CoinBigIndex j=start;j<end;j++) {
	      int iRow = row[j];
	      if (rowBlock[iRow]<0) {
		numberMarkedRows++;
		columnBlock[iBlock]++;
		rowBlock[iRow]=iBlock;
		whichRow[iRow]=jRow;
		jRow=iRow;
	      }
	    }
	    whichColumn[iBlock]=jRow;
	  }
	  columnsDone++;
	  if (iBlock>=0) 
	    maximumInBlock = CoinMax(maximumInBlock,columnBlock[iBlock]);
	  if (columnsDone>=checkAfterColumns) {
	    assert (numberGoodBlocks>0);
	    double averageSize = static_cast<double>(numberMarkedRows)/
	      static_cast<double>(numberGoodBlocks);
#ifndef OSL_WAY
	    double notionalBlocks = static_cast<double>(numberMarkedRows)/
	      averageSize;
	    if (maximumInBlock<3*averageSize&&numberGoodBlocks>2) {
	      if(best*(numberColumns-columnsDone) < notionalBlocks) {
		best = notionalBlocks/
		  static_cast<double> (numberColumns-columnsDone); 
		bestColumn = kColumn;
	      }
	    }
#else
	    if (maximumInBlock*10<numberRows*11&&numberGoodBlocks>1) {
	      double test = maximumInBlock + 0.0*averageSize;
	      if(best*static_cast<double>(columnsDone) > test) {
		best = test/static_cast<double> (columnsDone); 
		bestColumn = kColumn;
		bestColumnsDone=columnsDone;
	      }
	    }
#endif
	  }	      
	}
#ifndef OSL_WAY
	best2[iWay]=best;
#else
	if (bestColumnsDone<numberColumns)
	  best2[iWay]=-(numberColumns-bestColumnsDone);
	else
	  best2[iWay]=-numberColumns;
#endif
	column2[iWay]=bestColumn;
      }
      // mark columns
      int nMaster;
      CoinFillN(columnBlock,numberColumns,-2);
      if (best2[2]<best2[0]||best2[2]<best2[1]) {
	int iColumn1;
	int iColumn2;
	if (best2[0]>best2[1]) {
	  // End columns in master
	  iColumn1=column2[0]+1;
	  iColumn2=numberColumns;
	} else {
	  // Beginning columns in master
	  iColumn1=0;
	  iColumn2=numberColumns-column2[1];
	}
	nMaster = iColumn2-iColumn1;
	CoinFillN(columnBlock+iColumn1,nMaster,-1);
      } else {
	// sorted
	// End columns in master (in order)
	int iColumn1=column2[2]+1;
	nMaster = numberColumns-iColumn1;
	for (int i=iColumn1;i<numberColumns;i++)
	  columnBlock[stack[i]]=-1;
      }
      if (nMaster*2>numberColumns) {
	printf("%d columns out of %d would be in master - no good\n",
	       nMaster,numberColumns);
	delete [] rowBlock;
	delete [] columnBlock;
	delete [] whichRow;
	delete [] whichColumn;
	delete [] stack;
	CoinModel model(numberRows,numberColumns,&matrix, rowLower, rowUpper,
			columnLower,columnUpper,objective);
	model.setObjectiveOffset(objectiveOffset);
	addBlock("row_master","column_master",model);
	return 0;
      }
    } else {
      for (iColumn=0;iColumn<numberColumns;iColumn++)
	columnBlock[iColumn]=-2;
      // these are master columns
      if (numberColumns==2426) {
	// ken-7 dual
	for (iColumn=2401;iColumn<numberColumns;iColumn++)
	  columnBlock[iColumn]=-1;
      }
    }
    numberBlocks=0;
    CoinFillN(rowBlock,numberRows,-2);
    for (iRow=0;iRow<numberRows;iRow++) {
      int kstart = rowStart[iRow];
      int kend = rowStart[iRow]+rowLength[iRow];
      if (rowBlock[iRow]==-2) {
	// row not allocated
	int j;
	int nstack=0;
	for (j=kstart;j<kend;j++) {
	  int iColumn= column[j];
	  if (columnBlock[iColumn]!=-1) {
	    assert(columnBlock[iColumn]==-2);
	    columnBlock[iColumn]=numberBlocks; // mark
	    stack[nstack++] = iColumn;
	  }
	}
	if (nstack) {
	  // new block - put all connected in
	  numberBlocks++;
	  rowBlock[iRow]=numberBlocks-1;
	  while (nstack) {
	    int iColumn = stack[--nstack];
	    int k;
	    for (k=columnStart[iColumn];k<columnStart[iColumn]+columnLength[iColumn];k++) {
	      int iRow = row[k];
	      int kkstart = rowStart[iRow];
	      int kkend = kkstart + rowLength[iRow];
	      if (rowBlock[iRow]==-2) {
		rowBlock[iRow]=numberBlocks-1; // mark
		// row not allocated
		int jj;
		for (jj=kkstart;jj<kkend;jj++) {
		  int jColumn= column[jj];
		  if (columnBlock[jColumn]==-2) {
		    columnBlock[jColumn]=numberBlocks-1;
		    stack[nstack++]=jColumn;
		  }
		}
	      } else {
		assert (rowBlock[iRow]==numberBlocks-1);
	      }
	    }
	  }
	} else {
	  // Only in master
	  rowBlock[iRow]=-1;
	}
      }
    }
    delete [] stack;
    int numberMasterColumns=0;
    for (iColumn=0;iColumn<numberColumns;iColumn++) {
      int iBlock = columnBlock[iColumn];
      if (iBlock==-1)
	numberMasterColumns++;
    }
    int numberMasterRows=0;
    for (iRow=0;iRow<numberRows;iRow++) {
      int iBlock = rowBlock[iRow];
      if (iBlock==-1)
	numberMasterRows++;
    }
    if (numberBlocks<=maxBlocks)
      printf("%d blocks found - %d columns, %d rows in master\n",
	     numberBlocks,numberMasterColumns,numberMasterRows);
    else
      printf("%d blocks found (reduced to %d) - %d columns, %d rows in master\n",
	     numberBlocks,maxBlocks,numberMasterColumns,numberMasterRows);
    if (numberBlocks) {
      if (numberBlocks>maxBlocks) {
	int iBlock;
	for (iColumn=0;iColumn<numberColumns;iColumn++) {
	  iBlock = columnBlock[iColumn];
	  if (iBlock>=0)
	    columnBlock[iColumn] = iBlock%maxBlocks;
	}
	for (iRow=0;iRow<numberRows;iRow++) {
	  iBlock = rowBlock[iRow];
	  if (iBlock>=0)
	    rowBlock[iRow] = iBlock%maxBlocks;
	}
	numberBlocks=maxBlocks;
      }
    }
    // make up problems
    // Create all sub problems
    // Space for creating
    double * obj = new double [numberColumns];
    double * rowLo = new double [numberRows];
    double * rowUp = new double [numberRows];
    double * columnLo = new double [numberColumns];
    double * columnUp = new double [numberColumns];
    // Counts
    int * columnCount = reinterpret_cast<int *>(columnLo);
    CoinZeroN(columnCount,numberBlocks);
    for (int i=0;i<numberColumns;i++) {
      int iBlock=columnBlock[i];
      if (iBlock>=0)
	columnCount[iBlock]++;
    }
    // allocate empty columns
    for (int i=0;i<numberColumns;i++) {
      int iBlock=columnBlock[i];
      if (iBlock==-2) {
	// find block with smallest count
	int iSmall=-1;
	int smallest = numberColumns;
	for (int j=0;j<numberBlocks;j++) {
	  if (columnCount[j]<smallest) {
	    iSmall=j;
	    smallest=columnCount[j];
	  }
	}
	columnBlock[i]=iSmall;
	columnCount[iSmall]++;
      }
    }
    int * rowCount = reinterpret_cast<int *>(columnUp);
    CoinZeroN(rowCount,numberBlocks);
    for (int i=0;i<numberRows;i++) {
      int iBlock=rowBlock[i];
      if (iBlock>=0)
	rowCount[iBlock]++;
    }
    int maximumSize=0;
    for (int i=0;i<numberBlocks;i++) {
      printf("Block %d has %d columns and %d rows\n",i,
	     columnCount[i],rowCount[i]);
      int k=2*columnCount[i]+rowCount[i];
      maximumSize = CoinMax(maximumSize,k);
    }
    if (maximumSize*10>4*(2*numberColumns+numberRows)) {
      // No good
      printf("Doesn't look good\n");
      delete [] rowBlock;
      delete [] columnBlock;
      delete [] whichRow;
      delete [] whichColumn;
      delete [] obj ; 
      delete [] columnLo ; 
      delete [] columnUp ; 
      delete [] rowLo ; 
      delete [] rowUp ; 
      CoinModel model(numberRows,numberColumns,&matrix, rowLower, rowUpper,
		      columnLower,columnUpper,objective);
      model.setObjectiveOffset(objectiveOffset);
      addBlock("row_master","column_master",model);
      return 0;
    }
    // Name for master so at beginning
    addColumnBlock(numberMasterColumns,"column_master");
    // Arrays
    // get full matrix
    CoinPackedMatrix fullMatrix = matrix;
    int numberRow2,numberColumn2;
    int iBlock;
    for (iBlock=0;iBlock<numberBlocks;iBlock++) {
      char rowName[20];
      sprintf(rowName,"row_%d",iBlock);
      char columnName[20];
      sprintf(columnName,"column_%d",iBlock);
      numberRow2=0;
      numberColumn2=0;
      for (iColumn=0;iColumn<numberColumns;iColumn++) {
	if (iBlock==columnBlock[iColumn]) {
	  obj[numberColumn2]=objective[iColumn];
	  columnLo[numberColumn2]=columnLower[iColumn];
	  columnUp[numberColumn2]=columnUpper[iColumn];
	  whichColumn[numberColumn2++]=iColumn;
	}
      }
      for (iRow=0;iRow<numberRows;iRow++) {
	if (iBlock==rowBlock[iRow]) {
	  rowLo[numberRow2]=rowLower[iRow];
	  rowUp[numberRow2]=rowUpper[iRow];
	  whichRow[numberRow2++]=iRow;
	}
      }
      // Diagonal block
      CoinPackedMatrix mat(fullMatrix,
			   numberRow2,whichRow,
			   numberColumn2,whichColumn);
      // make sure correct dimensions
      mat.setDimensions(numberRow2,numberColumn2);
      CoinModel * block = new CoinModel(numberRow2,numberColumn2,&mat,
					rowLo,rowUp,columnLo,columnUp,obj);
      block->setOriginalIndices(whichRow,whichColumn);
      addBlock(rowName,columnName,block); // takes ownership
      // and beginning block
      numberColumn2=0;
      // get beginning matrix
      for (iColumn=0;iColumn<numberColumns;iColumn++) {
	int iBlock = columnBlock[iColumn];
	if (iBlock==-1) {
	  whichColumn[numberColumn2++]=iColumn;
	}
      }
      CoinPackedMatrix beginning(fullMatrix,
			   numberRow2,whichRow,
			   numberColumn2,whichColumn);
      // make sure correct dimensions *********
      beginning.setDimensions(numberRow2,numberColumn2);
      block = new CoinModel(numberRow2,numberMasterColumns,&beginning,
			    NULL,NULL,NULL,NULL,NULL);
      block->setOriginalIndices(whichRow,whichColumn);
      addBlock(rowName,"column_master",block); // takes ownership
    }
    // and master
    numberRow2=0;
    numberColumn2=0;
    for (iColumn=0;iColumn<numberColumns;iColumn++) {
      int iBlock = columnBlock[iColumn];
      if (iBlock==-1) {
	obj[numberColumn2]=objective[iColumn];
	columnLo[numberColumn2]=columnLower[iColumn];
	columnUp[numberColumn2]=columnUpper[iColumn];
	whichColumn[numberColumn2++]=iColumn;
      }
    }
    for (iRow=0;iRow<numberRows;iRow++) {
      int iBlock = rowBlock[iRow];
      if (iBlock<0) {
	rowLo[numberRow2]=rowLower[iRow];
	rowUp[numberRow2]=rowUpper[iRow];
	whichRow[numberRow2++]=iRow;
      }
    }
    delete [] rowBlock;
    delete [] columnBlock;
    CoinPackedMatrix beginning(fullMatrix,
			 numberRow2,whichRow,
			 numberColumn2,whichColumn);
    // make sure correct dimensions
    beginning.setDimensions(numberRow2,numberColumn2);
    CoinModel * block = new CoinModel(numberRow2,numberColumn2,&beginning,
				      rowLo,rowUp,
				      columnLo,columnUp,obj);
    block->setOriginalIndices(whichRow,whichColumn);
    addBlock("row_master","column_master",block); // takes ownership
    delete [] whichRow;
    delete [] whichColumn;
    delete [] obj ; 
    delete [] columnLo ; 
    delete [] columnUp ; 
    delete [] rowLo ; 
    delete [] rowUp ; 
  } else {
    abort();
  }
  return numberBlocks;
}
/* Decompose a CoinModel
   1 - try D-W
   2 - try Benders
   3 - try Staircase
   Returns number of blocks or zero if no structure
*/
int 
CoinStructuredModel::decompose(const CoinModel & coinModel, int type,
			       int maxBlocks)
{
  const CoinPackedMatrix * matrix = coinModel.packedMatrix();
  assert (matrix!=NULL);
  // Arrays
  const double * objective = coinModel.objectiveArray();
  const double * columnLower = coinModel.columnLowerArray();
  const double * columnUpper = coinModel.columnUpperArray();
  const double * rowLower = coinModel.rowLowerArray();
  const double * rowUpper = coinModel.rowUpperArray();
  return decompose(*matrix,
		   rowLower,  rowUpper,
		   columnLower,  columnUpper,
		   objective, type,maxBlocks,
		   coinModel.objectiveOffset());
}
// Return block corresponding to row and column
const CoinBaseModel *  
CoinStructuredModel::block(int row,int column) const
{
  const CoinBaseModel * block = NULL;
  if (blockType_) {
    for (int iBlock=0;iBlock<numberElementBlocks_;iBlock++) {
      if (blockType_[iBlock].rowBlock==row&&
	  blockType_[iBlock].columnBlock==column) {
	block = blocks_[iBlock];
	break;
      }
    }
  }
  return block;
}
// Return block corresponding to row and column as CoinModel
const CoinBaseModel *  
CoinStructuredModel::coinBlock(int row,int column) const
{
  const CoinModel * block = NULL;
  if (blockType_) {
    for (int iBlock=0;iBlock<numberElementBlocks_;iBlock++) {
      if (blockType_[iBlock].rowBlock==row&&
	  blockType_[iBlock].columnBlock==column) {
	block = dynamic_cast<CoinModel *>(blocks_[iBlock]);
	assert (block);
	break;
      }
    }
  }
  return block;
}
/* Fill pointers corresponding to row and column.
   False if any missing */
CoinModelBlockInfo 
CoinStructuredModel::block(int row,int column,
			   const double * & rowLower, const double * & rowUpper,
			   const double * & columnLower, const double * & columnUpper,
			   const double * & objective) const
{
  CoinModelBlockInfo info;
  //memset(&info,0,sizeof(info));
  rowLower=NULL;
  rowUpper=NULL;
  columnLower=NULL;
  columnUpper=NULL;
  objective=NULL;
  if (blockType_) {
    for (int iBlock=0;iBlock<numberElementBlocks_;iBlock++) {
      CoinModel * thisBlock = coinBlock(iBlock);
      if (blockType_[iBlock].rowBlock==row) {
	if (blockType_[iBlock].rhs) {
	  info.rhs=1;
	  rowLower = thisBlock->rowLowerArray();
	  rowUpper = thisBlock->rowUpperArray();
	}
      }
      if (blockType_[iBlock].columnBlock==column) {
	if (blockType_[iBlock].bounds) {
	  info.bounds=1;
	  columnLower = thisBlock->columnLowerArray();
	  columnUpper = thisBlock->columnUpperArray();
	  objective = thisBlock->objectiveArray();
	}
      }
    }
  }
  return info;
}
// Return block number corresponding to row and column
int  
CoinStructuredModel::blockIndex(int row,int column) const
{
  int block=-1;
  if (blockType_) {
    for (int iBlock=0;iBlock<numberElementBlocks_;iBlock++) {
      if (blockType_[iBlock].rowBlock==row&&
	  blockType_[iBlock].columnBlock==column) {
	block = iBlock;
	break;
      }
    }
  }
  return block;
}
