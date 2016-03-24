// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>

#include "CoinLpIO.hpp"
#include "CoinFloatEqual.hpp"
#include <string.h>
//#############################################################################

//--------------------------------------------------------------------------
// test import methods
void
CoinLpIOUnitTest(const std::string& lpDir)
{
   // Test default constructor
   {
      CoinLpIO m;
      assert( m.rowsense_ == NULL );
      assert( m.rhs_ == NULL );
      assert( m.rowrange_ == NULL );
      assert( m.matrixByRow_ == NULL );
      assert( m.matrixByColumn_ == NULL );
      assert( m.integerType_ == NULL);
      assert( m.fileName_ == NULL );
      assert( !strcmp( m.problemName_ , ""));
      assert( m.objName_ == NULL);
   }
   {
      CoinRelFltEq eq;
      CoinLpIO m;
      std::string fn = lpDir + "exmip1.lp";
      m.readLp(fn.c_str());
      assert( !strcmp( m.problemName_ , ""));
      assert( !strcmp( m.objName_ , "OBJ"));
      // Test language and re-use
      m.newLanguage(CoinMessages::it);
      m.messageHandler()->setPrefix(false);
      m.readLp(fn.c_str());
      // Test copy constructor and assignment operator
      {
         CoinLpIO lhs;
         {
            CoinLpIO im(m);
            CoinLpIO imC1(im);
            assert( imC1.getNumCols() == im.getNumCols() );
            assert( imC1.getNumRows() == im.getNumRows() );

            for (int i = 0; i < im.numberHash_[0]; i++) {
               // check the row name
               assert(!strcmp(im.names_[0][i], imC1.names_[0][i]));
            }

            for (int i = 0; i < im.numberHash_[1]; i++) {
               // check the column name
               assert(!strcmp(im.names_[1][i], imC1.names_[1][i]));
            }

            for (int i = 0; i < im.maxHash_[0]; i++) {
               // check hash value for row name
               assert(im.hash_[0][i].next  == imC1.hash_[0][i].next);
               assert(im.hash_[0][i].index == imC1.hash_[0][i].index);
            }

            for (int i = 0; i < im.maxHash_[1]; i++) {
               // check hash value for column name
               assert(im.hash_[1][i].next == imC1.hash_[1][i].next);
               assert(im.hash_[1][i].index == imC1.hash_[1][i].index);
            }

            CoinLpIO imC2(im);
            assert( imC2.getNumCols() == im.getNumCols() );
            assert( imC2.getNumRows() == im.getNumRows() );
            lhs = imC2;
            assert( lhs.getNumCols() == imC2.getNumCols() );
            assert( lhs.getNumRows() == imC2.getNumRows() );

            for (int i = 0; i < imC2.numberHash_[0]; i++) {
               // check the row name
               assert(!strcmp(lhs.names_[0][i], imC2.names_[0][i]));
            }

            for (int i = 0; i < imC2.numberHash_[1]; i++) {
               // check the column name
               assert(!strcmp(lhs.names_[1][i], imC2.names_[1][i]));
            }

            for (int i = 0; i < imC2.maxHash_[0]; i++) {
               // check hash value for row name
               assert(lhs.hash_[0][i].next  == imC2.hash_[0][i].next);
               assert(lhs.hash_[0][i].index == imC2.hash_[0][i].index);
            }

            for (int i = 0; i < im.maxHash_[1]; i++) {
               // check hash value for column name
               assert(lhs.hash_[1][i].next == imC2.hash_[1][i].next);
               assert(lhs.hash_[1][i].index == imC2.hash_[1][i].index);
            }
         }
         // Test that lhs has correct values even though rhs has gone out of scope
         assert( lhs.getNumCols() == m.getNumCols() );
         assert( lhs.getNumRows() == m.getNumRows() );
      }
      {
         CoinLpIO dumSi(m);
         int nc = dumSi.getNumCols();
         int nr = dumSi.getNumRows();
         const double* cl = dumSi.getColLower();
         const double* cu = dumSi.getColUpper();
         const double* rl = dumSi.getRowLower();
         const double* ru = dumSi.getRowUpper();
         assert( nc == 10 );
         assert( nr == 5 );
         assert( eq(cl[0], 2.5) );
         assert( eq(cl[1], 0.5) );
         assert( eq(cu[1], 4) );
         assert( eq(cu[2], 4.3) );
         assert( eq(rl[0], 2.5) );
         assert( eq(rl[4], 15.0) );
         assert( eq(ru[1], 2.1) );
         assert( eq(ru[4], 15.0) );
         assert( !eq(cl[3], 1.2345) );
         assert( !eq(cu[4], 10.2345) );
         assert( eq( dumSi.getObjCoefficients()[0],  1.0) );
         assert( eq( dumSi.getObjCoefficients()[1],  2.0) );
         assert( eq( dumSi.getObjCoefficients()[2],  -1.0) );
         assert( eq( dumSi.getObjCoefficients()[3],  0.0) );
         assert( eq( dumSi.getObjCoefficients()[4],  0.0) );
         assert( eq( dumSi.getObjCoefficients()[5],  0.0) );
         assert( eq( dumSi.getObjCoefficients()[6],  0.0) );
         assert( eq( dumSi.getObjCoefficients()[7],  0.0) );
         assert( eq( dumSi.getObjCoefficients()[8],  0.0) );
         assert( eq( dumSi.getObjCoefficients()[9],  0.0) );
         dumSi.writeLp("CoinLpIoTest.lp", true);
      }
      // Read just written file
      {
         CoinLpIO dumSi;
         dumSi.readLp("CoinLpIoTest.lp");
         int nc = dumSi.getNumCols();
         int nr = dumSi.getNumRows();
         const double* cl = dumSi.getColLower();
         const double* cu = dumSi.getColUpper();
         const double* rl = dumSi.getRowLower();
         const double* ru = dumSi.getRowUpper();
         assert( nc == 10 );
         assert( nr == 5 );
         assert( eq(cl[0], 2.5) );
         assert( eq(cl[1], 0.5) );
         assert( eq(cu[1], 4) );
         assert( eq(cu[2], 4.3) );
         assert( eq(rl[0], 2.5) );
         assert( eq(rl[4], 15.0) );
         assert( eq(ru[1], 2.1) );
         assert( eq(ru[4], 15.0) );
         assert( !eq(cl[3], 1.2345) );
         assert( !eq(cu[4], 10.2345) );
         assert( eq( dumSi.getObjCoefficients()[0],  1.0) );
         assert( eq( dumSi.getObjCoefficients()[1],  2.0) );
         assert( eq( dumSi.getObjCoefficients()[2], -1.0) );
         assert( eq( dumSi.getObjCoefficients()[3],  0.0) );
         assert( eq( dumSi.getObjCoefficients()[4],  0.0) );
         assert( eq( dumSi.getObjCoefficients()[5],  0.0) );
         assert( eq( dumSi.getObjCoefficients()[6],  0.0) );
         assert( eq( dumSi.getObjCoefficients()[7],  0.0) );
      }
      // Test matrixByRow method
      {
         const CoinLpIO si(m);
         const CoinPackedMatrix* smP = si.getMatrixByRow();
         CoinRelFltEq eq;
         const double* ev = smP->getElements();
         assert( eq(ev[0],   3.0) );
         assert( eq(ev[1],   1.0) );
         assert( eq(ev[2],  -2.0) );
         assert( eq(ev[3],  -1.0) );
         assert( eq(ev[4],  -1.0) );
         assert( eq(ev[5],   2.0) );
         assert( eq(ev[6],   1.1) );
         assert( eq(ev[7],   1.0) );
         assert( eq(ev[8],   1.0) );
         assert( eq(ev[9],   2.8) );
         assert( eq(ev[10], -1.2) );
         assert( eq(ev[11], -1.0 ) );
         assert( eq(ev[12],  5.6) );
         assert( eq(ev[13],  1.0) );
         assert( eq(ev[14],  1.9) );
         assert( eq(ev[15], -1.0) );
         const CoinBigIndex* mi = smP->getVectorStarts();
         assert( mi[0] == 0 );
         assert( mi[1] == 5 );
         assert( mi[2] == 7 );
         assert( mi[3] == 9 );
         assert( mi[4] == 12 );
         assert( mi[5] == 16 );
         const int* ei = smP->getIndices();
         assert( ei[0]  ==  0 );
         assert( ei[1]  ==  3 );
         assert( ei[2]  ==  4 );
         assert( ei[3]  ==  1 );
         assert( ei[4]  ==  2 );
         assert( ei[5]  ==  3 );
         assert( ei[6]  ==  5 );
         assert( ei[7]  ==  5 );
         assert( ei[8]  ==  6 );
         assert( ei[9]  ==  4 );
         assert( ei[10] ==  7 );
         assert( ei[11] ==  8 );
         assert( ei[12] ==  0 );
         assert( ei[13] ==  1 );
         assert( ei[14] ==  2 );
         assert( ei[15] ==  9 );
         assert( smP->getMajorDim() == 5 );
         assert( smP->getNumElements() == 16 );
      }
      // Test matrixByCol method
      {
         const CoinLpIO si(m);
         const CoinPackedMatrix* smP = si.getMatrixByCol();
         CoinRelFltEq eq;
         const double* ev = smP->getElements();
         assert( eq(ev[0],   3.0) );
         assert( eq(ev[1],   5.6) );
         assert( eq(ev[2],   -1.0) );
         assert( eq(ev[3],   1.0) );
         assert( eq(ev[4],   -1.0) );
         assert( eq(ev[5],   1.9) );
         assert( eq(ev[6],   1.0) );
         assert( eq(ev[7],   2.0) );
         assert( eq(ev[8],   -2.0) );
         assert( eq(ev[9],   2.8) );
         assert( eq(ev[10],  1.1) );
         assert( eq(ev[11],  1.0) );
         assert( eq(ev[12],  1.0) );
         assert( eq(ev[13],  -1.2) );
         assert( eq(ev[14],  -1.0) );
         assert( eq(ev[15],  -1.0) );
         const CoinBigIndex* mi = smP->getVectorStarts();
         assert( mi[0] == 0 );
         assert( mi[1] == 2 );
         assert( mi[2] == 4 );
         assert( mi[3] == 6 );
         assert( mi[4] == 8 );
         assert( mi[5] == 10 );
         assert( mi[6] == 12 );
         assert( mi[7] == 13 );
         assert( mi[8] == 14 );
         assert( mi[9] == 15 );
         assert( mi[10] == 16 );
         const int* ei = smP->getIndices();
         assert( ei[0]  ==  0 );
         assert( ei[1]  ==  4 );
         assert( ei[2]  ==  0 );
         assert( ei[3]  ==  4 );
         assert( ei[4]  ==  0  );
         assert( ei[5]  ==  4 );
         assert( ei[6]  ==  0 );
         assert( ei[7]  ==  1 );
         assert( ei[8]  ==  0 );
         assert( ei[9]  ==  3 );
         assert( ei[10] ==  1 );
         assert( ei[11] ==  2 );
         assert( ei[12] ==  2 );
         assert( ei[13] ==  3 );
         assert( ei[14] ==  3 );
         assert( ei[15] ==  4 );
         assert( smP->getMajorDim() == 10 );
         assert( smP->getNumElements() == 16 );
         assert( smP->getSizeVectorStarts() == 11 );
         assert( smP->getMinorDim() == 5 );
      }
      //--------------
      // Test rowsense, rhs, rowrange, matrixByRow
      {
         CoinLpIO lhs;
         {
            assert( lhs.rowrange_ == NULL );
            assert( lhs.rowsense_ == NULL );
            assert( lhs.rhs_ == NULL );
            assert( lhs.matrixByColumn_ == NULL );
            CoinLpIO siC1(m);
            assert( siC1.rowrange_ != NULL );
            assert( siC1.rowsense_ != NULL );
            assert( siC1.rhs_ != NULL );
            assert( siC1.matrixByRow_ != NULL );
            const char*    siC1rs  = siC1.getRowSense();
            assert( siC1rs[0] == 'G' );
            assert( siC1rs[1] == 'L' );
            assert( siC1rs[2] == 'E' );
            assert( siC1rs[3] == 'E' );
            assert( siC1rs[4] == 'E' );
            const double* siC1rhs = siC1.getRightHandSide();
            assert( eq(siC1rhs[0], 2.5) );
            assert( eq(siC1rhs[1], 2.1) );
            assert( eq(siC1rhs[2], 4.0) );
            assert( eq(siC1rhs[3], 1.8) );
            assert( eq(siC1rhs[4], 15.) );
            const double* siC1rr  = siC1.getRowRange();
            assert( eq(siC1rr[0], 0.0) );
            assert( eq(siC1rr[1], 0.0) );
            assert( eq(siC1rr[2], 0.0) );
            assert( eq(siC1rr[3], 0.0) );
            assert( eq(siC1rr[4], 0.0) );
            const CoinPackedMatrix* siC1mbr = siC1.getMatrixByRow();
            assert( siC1mbr != NULL );
            const double* ev = siC1mbr->getElements();
            assert( eq(ev[0],   3.0) );
            assert( eq(ev[1],   1.0) );
            assert( eq(ev[2],  -2.0) );
            assert( eq(ev[3],  -1.0) );
            assert( eq(ev[4],  -1.0) );
            assert( eq(ev[5],   2.0) );
            assert( eq(ev[6],   1.1) );
            assert( eq(ev[7],   1.0) );
            assert( eq(ev[8],   1.0) );
            assert( eq(ev[9],   2.8) );
            assert( eq(ev[10], -1.2) );
            assert( eq(ev[11], -1.0) );
            assert( eq(ev[12],  5.6) );
            assert( eq(ev[13],  1.0) );
            assert( eq(ev[14],  1.9) );
            assert( eq(ev[15], -1.0) );
            const CoinBigIndex* mi = siC1mbr->getVectorStarts();
            assert( mi[0] == 0 );
            assert( mi[1] == 5 );
            assert( mi[2] == 7 );
            assert( mi[3] == 9 );
            assert( mi[4] == 12 );
            assert( mi[5] == 16 );
            const int* ei = siC1mbr->getIndices();
            assert( ei[0]  ==  0 );
            assert( ei[1]  ==  3 );
            assert( ei[2]  ==  4 );
            assert( ei[3]  ==  1 );
            assert( ei[4]  ==  2 );
            assert( ei[5]  ==  3 );
            assert( ei[6]  ==  5 );
            assert( ei[7]  ==  5 );
            assert( ei[8]  ==  6 );
            assert( ei[9]  ==  4 );
            assert( ei[10] ==  7 );
            assert( ei[11] ==  8 );
            assert( ei[12] ==  0 );
            assert( ei[13] ==  1 );
            assert( ei[14] ==  2 );
            assert( ei[15] ==  9 );
            assert( siC1mbr->getMajorDim() == 5 );
            assert( siC1mbr->getNumElements() == 16 );
            assert( siC1rs  == siC1.getRowSense() );
            assert( siC1rhs == siC1.getRightHandSide() );
            assert( siC1rr  == siC1.getRowRange() );
         }
      }
   }
}
