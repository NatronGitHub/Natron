// Copyright (C) 2001, International Business Machines
// Corporation and others.  All Rights Reserved.
// This file is licensed under the terms of Eclipse Public License (EPL).

// this is a copy of OsiVolSolverInterface (trunk rev. 1466) renamed to OsiTestSolverInterface

#include "CoinPragma.hpp"

#include <cstdlib>
#include <cmath>

#include "CoinHelperFunctions.hpp"
#include "CoinMpsIO.hpp"
#include "OsiTestSolverInterface.hpp"

//#############################################################################

void
OsiTestSolverInterface::initFromRlbRub(const int rownum,
				      const double* rowlb,
				      const double* rowub)
{
   if (maxNumrows_ > 0) {
      rowRimAllocator_();
      if (rowub) {
	 CoinDisjointCopyN(rowub, rownum, rowupper_);
      } else {
	 CoinFillN(rowupper_, rownum, getInfinity());
      }
      if (rowlb) {
	 CoinDisjointCopyN(rowlb, rownum, rowlower_);
      } else {
	 CoinFillN(rowlower_, rownum, -getInfinity());
      }
      // Set the initial dual solution
      CoinFillN(rowprice_, rownum, 0.0);
      convertBoundsToSenses_();
   }
}

//#############################################################################

void
OsiTestSolverInterface::initFromRhsSenseRange(const int rownum,
					     const char* rowsen,
					     const double* rowrhs,   
					     const double* rowrng)
{
   if (maxNumrows_ > 0) {
      rowRimAllocator_();
      if (rowsen) {
	 CoinDisjointCopyN(rowsen, rownum, rowsense_);
      } else {
	 CoinFillN(rowsense_, rownum, 'G');
      }
      if (rowrhs) {
	 CoinDisjointCopyN(rowrhs, rownum, rhs_);
      } else {
	 CoinFillN(rhs_, rownum, 0.0);
      }
      if (rowrng) {
	 CoinDisjointCopyN(rowrng, rownum, rowrange_);
      } else {
	 CoinFillN(rowrange_, rownum, 0.0);
      }
      // Set the initial dual solution
      CoinFillN(rowprice_, rownum, 0.0);
      convertSensesToBounds_();
   }
}
//#############################################################################

void
OsiTestSolverInterface::initFromClbCubObj(const int colnum,
                                         const double* collb,
                                         const double* colub,   
                                         const double* obj)
{
  if (maxNumcols_ > 0) {
    colRimAllocator_();
    if (colub) {
      CoinDisjointCopyN(colub, colnum, colupper_);
    } else {
      CoinFillN(colupper_, colnum, getInfinity());
    }
    if (collb) {
      CoinDisjointCopyN(collb, colnum, collower_);
    } else {
      CoinFillN(collower_, colnum, 0.0);
    }
    CoinFillN(continuous_,colnum,true);
    if (obj) {
      CoinDisjointCopyN(obj, colnum, objcoeffs_);
    } else {
      CoinFillN(objcoeffs_, colnum, 0.0);
    }
    int c;
    for ( c=0; c<colnum; c++ ) {
      if ( fabs(collower_[c]) < fabs(colupper_[c]) ) {
        colsol_[c] = collower_[c];
      }
      else {
        colsol_[c] = colupper_[c];
      }
    }
  }
}

//#############################################################################
// Problem input methods
//#############################################################################

void
OsiTestSolverInterface::loadProblem(const CoinPackedMatrix& matrix,
				   const double* collb, const double* colub,   
				   const double* obj,
				   const double* rowlb, const double* rowub)
{
   gutsOfDestructor_();
   const int rownum = matrix.getNumRows();
   const int colnum = matrix.getNumCols();

   if (matrix.isColOrdered()) {
      colMatrix_.setExtraGap(matrix.getExtraGap());
      colMatrix_.setExtraMajor(matrix.getExtraMajor());
      colMatrix_ = matrix;
      colMatrixCurrent_ = true;
      rowMatrixCurrent_ = false;
      maxNumcols_ = colMatrix_.getMaxMajorDim();
      maxNumrows_ = static_cast<int>((1+colMatrix_.getExtraGap()) *
				     colMatrix_.getMinorDim());
   } else {
      rowMatrix_.setExtraGap(matrix.getExtraGap());
      rowMatrix_.setExtraMajor(matrix.getExtraMajor());
      rowMatrix_ = matrix;
      rowMatrixCurrent_ = true;
      colMatrixCurrent_ = false;
      maxNumcols_ = static_cast<int>((1+rowMatrix_.getExtraGap()) *
				     rowMatrix_.getMinorDim());
      maxNumrows_ = rowMatrix_.getMaxMajorDim();
   }

   initFromRlbRub(rownum, rowlb, rowub);
   initFromClbCubObj(colnum, collb, colub, obj);
}

//-----------------------------------------------------------------------

void
OsiTestSolverInterface::assignProblem(CoinPackedMatrix*& matrix,
				     double*& collb, double*& colub,
				     double*& obj,
				     double*& rowlb, double*& rowub)
{
   gutsOfDestructor_();
   const int rownum = matrix->getNumRows();
   const int colnum = matrix->getNumCols();
   maxNumcols_ = colnum;
   maxNumrows_ = rownum;

   if (matrix->isColOrdered()) {
      colMatrix_.swap(*matrix);
      colMatrixCurrent_ = true;
      rowMatrixCurrent_ = false;
   } else {
      rowMatrix_.swap(*matrix);
      rowMatrixCurrent_ = true;
      colMatrixCurrent_ = false;
   }
   delete matrix; matrix = 0;
      
   rowupper_  = rowub;     rowub  = 0;
   rowlower_  = rowlb;     rowlb  = 0;
   colupper_  = colub;     colub  = 0;
   collower_  = collb;     collb  = 0;
   objcoeffs_ = obj;       obj    = 0;

   if (maxNumrows_ > 0) {
      if (!rowupper_) {
	 rowupper_ = new double[maxNumrows_];
	 CoinFillN(rowupper_, rownum, getInfinity());
      }
      if (!rowlower_) {
	 rowlower_ = new double[maxNumrows_];
	 CoinFillN(rowlower_, rownum, -getInfinity());
      }
      rowsense_ = new char[maxNumrows_];
      rhs_      = new double[maxNumrows_];
      rowrange_ = new double[maxNumrows_];
      rowprice_ = new double[maxNumrows_];
      lhs_      = new double[maxNumrows_];
      // Set the initial dual solution
      CoinFillN(rowprice_, rownum, 0.0);
      convertBoundsToSenses_();
   }
   if (maxNumcols_ > 0) {
      if (!colupper_) {
	 colupper_ = new double[maxNumcols_];
	 CoinFillN(colupper_, colnum, getInfinity());
      }
      if (!collower_) {
	 collower_ = new double[maxNumcols_];
	 CoinFillN(collower_, colnum, -getInfinity());
      }
      if (!objcoeffs_) {
	 objcoeffs_ = new double[maxNumcols_];
	 CoinFillN(objcoeffs_, colnum, -getInfinity());
      }

      colsol_    = new double[maxNumcols_];
      int c;
      for ( c=0; c<colnum; c++ ) {
	if ( fabs(collower_[c]) < fabs(colupper_[c]) ) {
	  colsol_[c] = collower_[c];
	}
	else {
	  colsol_[c] = colupper_[c];
	}
      }

      rc_        = new double[maxNumcols_];
      continuous_ = new bool[maxNumcols_];
   }
}

//-----------------------------------------------------------------------

void
OsiTestSolverInterface::loadProblem(const CoinPackedMatrix& matrix,
				   const double* collb, const double* colub,
				   const double* obj,
				   const char* rowsen, const double* rowrhs,   
				   const double* rowrng)
{
   gutsOfDestructor_();
   const int rownum = matrix.getNumRows();
   const int colnum = matrix.getNumCols();

   if (matrix.isColOrdered()) {
      colMatrix_ = matrix;
      colMatrixCurrent_ = true;
      rowMatrixCurrent_ = false;
      maxNumcols_ = colMatrix_.getMaxMajorDim();
      maxNumrows_ = static_cast<int>((1+colMatrix_.getExtraGap()) *
				     colMatrix_.getMinorDim());
   } else {
      rowMatrix_ = matrix;
      rowMatrixCurrent_ = true;
      colMatrixCurrent_ = false;
      maxNumcols_ = static_cast<int>((1+rowMatrix_.getExtraGap()) *
				     rowMatrix_.getMinorDim());
      maxNumrows_ = rowMatrix_.getMaxMajorDim();
   }

   initFromRhsSenseRange(rownum, rowsen, rowrhs, rowrng);
   initFromClbCubObj(colnum, collb, colub, obj);
}

//-----------------------------------------------------------------------

void
OsiTestSolverInterface::assignProblem(CoinPackedMatrix*& matrix,
				     double*& collb, double*& colub,
				     double*& obj,
				     char*& rowsen, double*& rowrhs,
				     double*& rowrng)
{
   gutsOfDestructor_();
   const int rownum = matrix->getNumRows();
   const int colnum = matrix->getNumCols();
   maxNumcols_ = colnum;
   maxNumrows_ = rownum;

   if (matrix->isColOrdered()) {
      colMatrix_.swap(*matrix);
      colMatrixCurrent_ = true;
      rowMatrixCurrent_ = false;
   } else {
      rowMatrix_.swap(*matrix);
      rowMatrixCurrent_ = true;
      colMatrixCurrent_ = false;
   }
   delete matrix; matrix = 0;
      
   rowsense_  = rowsen;   rowsen = 0;
   rhs_       = rowrhs;   rowrhs = 0;
   rowrange_  = rowrng;   rowrng = 0;
   colupper_  = colub;    colub  = 0;
   collower_  = collb;    collb  = 0;
   objcoeffs_ = obj;      obj    = 0;

   if (maxNumrows_ > 0) {
      if (!rowsense_) {
	 rowsense_ = new char[maxNumrows_];
	 CoinFillN(rowsense_, rownum, 'G');
      }
      if (!rhs_) {
	 rhs_ = new double[maxNumrows_];
	 CoinFillN(rhs_, rownum, 0.0);
      }
      if (!rowrange_) {
	 rowrange_ = new double[maxNumrows_];
	 CoinFillN(rowrange_, rownum, 0.0);
      }
      rowlower_ = new double[maxNumrows_];
      rowupper_ = new double[maxNumrows_];
      rowprice_ = new double[maxNumrows_];
      lhs_      = new double[maxNumrows_];
      // Set the initial dual solution
      CoinFillN(rowprice_, rownum, 0.0);
      convertSensesToBounds_();
   }
   if (maxNumcols_ > 0) {
      if (!colupper_) {
	 colupper_ = new double[maxNumcols_];
	 CoinFillN(colupper_, colnum, getInfinity());
      }
      if (!collower_) {
	 collower_ = new double[maxNumcols_];
	 CoinFillN(collower_, colnum, -getInfinity());
      }
      if (!objcoeffs_) {
	 objcoeffs_ = new double[maxNumcols_];
	 CoinFillN(objcoeffs_, colnum, -getInfinity());
      }

      colsol_    = new double[maxNumcols_];
      int c;
      for ( c=0; c<colnum; c++ ) {
	if ( fabs(collower_[c]) < fabs(colupper_[c]) ) {
	  colsol_[c] = collower_[c];
	}
	else {
	  colsol_[c] = colupper_[c];
	}
      }

      rc_        = new double[maxNumcols_];
      continuous_ = new bool[maxNumcols_];
   }
}

//-----------------------------------------------------------------------

void
OsiTestSolverInterface::loadProblem(const int numcols, const int numrows,
				   const int* start, const int* index,
				   const double* value,
				   const double* collb, const double* colub,
				   const double* obj,
				   const double* rowlb, const double* rowub)
{
   gutsOfDestructor_();

   colMatrix_.copyOf(true, numrows, numcols, start[numcols],
		     value, index, start, 0);
   colMatrixCurrent_ = true;
   rowMatrixCurrent_ = false;
   maxNumcols_ = colMatrix_.getMaxMajorDim();
   maxNumrows_ = static_cast<int>((1+colMatrix_.getExtraGap()) *
				  colMatrix_.getMinorDim());

   initFromRlbRub(numrows, rowlb, rowub);
   initFromClbCubObj(numcols, collb, colub, obj);
}

//-----------------------------------------------------------------------

void
OsiTestSolverInterface::loadProblem(const int numcols, const int numrows,
				   const int* start, const int* index,
				   const double* value,
				   const double* collb, const double* colub,
				   const double* obj,
				   const char* rowsen, const double* rowrhs,   
				   const double* rowrng)
{
   gutsOfDestructor_();

   colMatrix_.copyOf(true, numrows, numcols, start[numcols],
		     value, index, start, 0);
   colMatrixCurrent_ = true;
   rowMatrixCurrent_ = false;
   maxNumcols_ = colMatrix_.getMaxMajorDim();
   maxNumrows_ = static_cast<int>((1+colMatrix_.getExtraGap()) *
				  colMatrix_.getMinorDim());

   initFromRhsSenseRange(numrows, rowsen, rowrhs, rowrng);
   initFromClbCubObj(numcols, collb, colub, obj);
}

//-----------------------------------------------------------------------


int 
OsiTestSolverInterface::readMps(const char *filename, const char *extension)
{
   CoinMpsIO reader;
   reader.setInfinity(getInfinity());
   int retVal = reader.readMps(filename, extension);
   if (retVal == 0) {
     loadProblem(*reader.getMatrixByCol(),
	         reader.getColLower(),reader.getColUpper(),
	         reader.getObjCoefficients(),
	         reader.getRowLower(),reader.getRowUpper());
     int nc = getNumCols();
     assert (continuous_);
     CoinFillN(continuous_, nc, true);
   }
   return retVal;
}


//-----------------------------------------------------------------------

void 
OsiTestSolverInterface::writeMps(const char *filename,
				const char *extension,
				double /*objSense*/) const
{
   CoinMpsIO writer;
   writer.setMpsData(*getMatrixByCol(), getInfinity(),
		     getColLower(), getColUpper(),
		     getObjCoefficients(), 
		     reinterpret_cast<const char *> (NULL) /*integrality*/,
		     getRowLower(), getRowUpper(),
		     reinterpret_cast<const char **> (NULL) /*colnam*/, 
		     reinterpret_cast<const char **> (NULL) /*rownam*/);
   std::string fname = filename;
   if (extension)
   { if (extension[0] != '\0' && extension[0] != '.')
     fname += "." ; }
   fname += extension;
   writer.writeMps(fname.c_str());
}
