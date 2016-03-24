/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


#include <cstdio>

#include "CoinPragma.hpp"
#include "CoinHelperFunctions.hpp"

#include "ClpInterior.hpp"
#include "myPdco.hpp"
#include "ClpDummyMatrix.hpp"
#include "ClpMessage.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor
//-------------------------------------------------------------------
myPdco::myPdco()
     : ClpPdcoBase(),
       rowIndex_(NULL),
       numlinks_(0),
       numnodes_(0)
{
     setType(11);
}

// Constructor from stuff
myPdco::myPdco(double d1, double d2,
               int numnodes, int numlinks)
     : ClpPdcoBase(),
       rowIndex_(NULL),
       numlinks_(numlinks),
       numnodes_(numnodes)
{
     d1_ = d1;
     d2_ = d2;
     setType(11);
}
//-------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------
myPdco::myPdco(const myPdco & rhs)
     : ClpPdcoBase(rhs),
       numlinks_(rhs.numlinks_),
       numnodes_(rhs.numnodes_)
{
     rowIndex_ = ClpCopyOfArray(rhs.rowIndex_, 2 * (numlinks_ + 2 * numnodes_));
}

//-------------------------------------------------------------------
// Destructor
//-------------------------------------------------------------------
myPdco::~myPdco()
{
     delete [] rowIndex_;
}

//----------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------
myPdco &
myPdco::operator= (const myPdco & rhs)
{
     if (this != &rhs) {
          ClpPdcoBase::operator= (rhs);
          numlinks_ = rhs.numlinks_;
          numnodes_ = rhs.numnodes_;
          rowIndex_ = ClpCopyOfArray(rhs.rowIndex_, 2 * (numlinks_ + 2 * numnodes_));
     }
     return *this;
}
//-------------------------------------------------------------------
// Clone
//-------------------------------------------------------------------
ClpPdcoBase * myPdco::clone() const
{
     return new myPdco(*this);
}

void myPdco::matVecMult(ClpInterior * model,  int mode, double* x_elts, double* y_elts) const
{
     int nrow = model->numberRows();
     if (mode == 1) {
          double y_sum = 0.0;
          for (int k = 0; k < numlinks_; k++) {
               y_sum += y_elts[k];
               int i1 = rowIndex_[2*k];
               assert(i1 >= 0);
               x_elts[i1] += y_elts[k];
               int i2 = rowIndex_[2*k+1];
               assert(i2 >= 0);
               x_elts[i2] -= y_elts[k];
          }
          double y_suma = 0.0;
          double y_sumb = 0.0;
          for (int k = 0; k < numnodes_; k++) {
               y_suma -= y_elts[numlinks_ + k];
               y_sumb += y_elts[numlinks_ + numnodes_ + k];
               x_elts[k] += y_elts[numlinks_ + k];
               x_elts[k] -= y_elts[numlinks_ + numnodes_ + k];
          }
          x_elts[nrow-3] += y_suma;
          x_elts[nrow-2] += y_sumb;
          x_elts[nrow-1] += (y_sum - y_suma + y_sumb);
     } else {
          for (int k = 0; k < numlinks_; k++) {
               x_elts[k] += y_elts[nrow-1];
               int i1 = rowIndex_[2*k];
               x_elts[k] += y_elts[i1];
               int i2 = rowIndex_[2*k+1];
               x_elts[k] -= y_elts[i2];
          }
          for (int k = 0; k < numnodes_; k++) {
               x_elts[numlinks_ + k] += (y_elts[k] - y_elts[nrow-3] + y_elts[nrow-1]);
               x_elts[numlinks_ + numnodes_ + k] +=
                    (y_elts[nrow-2] - y_elts[k] + y_elts[nrow-1]);
          }
     }
     return;
}

void myPdco::matPrecon(ClpInterior * model, double delta, double* x_elts, double* y_elts) const
{
     double y_sum = 0.0;
     int ncol = model->numberColumns();
     int nrow = model->numberRows();
     double *ysq = new double[ncol];
     for (int k = 0; k < nrow; k++)
          x_elts[k] = 0.0;
     for (int k = 0; k < ncol; k++)
          ysq[k] = y_elts[k] * y_elts[k];

     for (int k = 0; k < numlinks_; k++) {
          y_sum += ysq[k];
          int i1 = rowIndex_[2*k];
          x_elts[i1] += ysq[k];
          int i2 = rowIndex_[2*k+1];
          x_elts[i2] += ysq[k];
     }
     double y_suma = 0.0;
     double y_sumb = 0.0;
     for (int k = 0; k < numnodes_; k++) {
          y_suma += ysq[numlinks_ + k];
          y_sumb += ysq[numlinks_ + numnodes_ + k];
          x_elts[k] += ysq[numlinks_ + k];
          x_elts[k] += ysq[numlinks_ + numnodes_ + k];
     }
     x_elts[nrow-3] += y_suma;
     x_elts[nrow-2] += y_sumb;
     x_elts[nrow-1] += (y_sum + y_suma + y_sumb);
     delete [] ysq;
     double delsq = delta * delta;
     for (int k = 0; k < nrow; k++)
          x_elts[k] = 1.0 / (sqrt(x_elts[k] + delsq));
     return;
}
double myPdco::getObj(ClpInterior * model, CoinDenseVector<double> &x) const
{
     double obj = 0;
     double *x_elts = x.getElements();
     int ncol = model->numberColumns();
     for (int k = 0; k < ncol; k++)
          obj += x_elts[k] * log(x_elts[k]);
     return obj;
}

void myPdco::getGrad(ClpInterior * model, CoinDenseVector<double> &x, CoinDenseVector<double> &g) const
{
     double *x_elts = x.getElements();
     double *g_elts = g.getElements();
     int ncol = model->numberColumns();
     for (int k = 0; k < ncol; k++)
          g_elts[k] = 1.0 + log(x_elts[k]);
     return;
}

void myPdco::getHessian(ClpInterior * model, CoinDenseVector<double> &x, CoinDenseVector<double> &H) const
{
     double *x_elts = x.getElements();
     double *H_elts = H.getElements();
     int ncol = model->numberColumns();
     for (int k = 0; k < ncol; k++)
          H_elts[k] = 1.0 / x_elts[k];
     return;
}
myPdco::myPdco(ClpInterior & model, FILE * fpData, FILE * fpParam)
{
     int nrow;
     int ncol;
     int numelts;
     double  *rowUpper;
     double  *rowLower;
     double  *colUpper;
     double  *colLower;
     double  *rhs;
     double  *x;
     double  *y;
     double  *dj;
     int ipair[2], igparm[4], maxrows, maxlinks;
     // Read max array sizes and allocate them
     fscanf(fpParam, "%d %d", &maxrows, &maxlinks);
     int *ir = new int[2*maxlinks + 5];
     /********************** alpha parameter hrdwired here ***********/
     double alpha = 0.9;

     int kct = 0;
     int imax = 0;
     int imin = 0x7fffffff;
     int *ifrom = &ipair[0];
     int *ito = &ipair[1];
     int nonzpt = 0;
     numlinks_ = 0;
     while (fscanf(fpData, "%d %d", ifrom, ito) && kct++ < maxlinks) {
          //  while(fread(ipair, 4,2, fpData) && kct++ < maxlinks){
          if ((*ifrom) < 0) {
               printf("Bad link  %d  %d\n", *ifrom, *ito);
               continue;
          }
          ipair[0]--;
          ipair[1]--;
          //assert(*ifrom>=0&&*ifrom<maxrows);
          //assert(*ito>=0&&*ifrom<maxrows);
          if (*ifrom < 0 || *ifrom >= maxrows || *ito < 0 || *ito >= maxrows) {
               printf("bad link %d %d\n", *ifrom, *ito);
               continue;
          }
          numlinks_++;
          ir[nonzpt++] = *ifrom;
          ir[nonzpt++] = *ito;
          imax = CoinMax(imax, *ifrom);
          imax = CoinMax(imax, *ito);
          imin = CoinMin(imin, *ifrom);
          imin = CoinMin(imin, *ito);
     }
     fclose(fpData);
     fclose(fpParam);
     printf("imax %d  imin %d\n", imax, imin);
     // Set model size
     numnodes_ = imax + 1;
     nrow = numnodes_ + 3;
     ncol = numlinks_ + 2 * numnodes_;
     numelts = 3 * ncol;

     rowIndex_ = ir;

     d1_ = 1.0e-3;
     d2_ = 1.0e-3;

     double* rhs_def = new double[nrow];
     for (int k = 0; k < nrow; k++)
          rhs_def[k] = 0.0;
     rhs_def[nrow-3] = alpha - 1.0;
     rhs_def[nrow-2] = 1.0 - alpha;
     rhs_def[nrow-1] = 1.0;
     // rhs_ etc should not be public
     rhs = rhs_def;
     rowUpper = rhs_def;
     rowLower = rhs_def;
     double *x_def = new double[ncol];
     double *U_def = new double[ncol];
     double *L_def = new double[ncol];
     for (int k = 0; k < ncol; k++) {
          //    x_def[k] = 10.0/ncol;
          x_def[k] = 1.0 / ncol;
          U_def[k] = 1e20;;
          L_def[k] = 0.0;
     }
     x = x_def;
     colUpper = U_def;
     colLower = L_def;
     // We have enough to create a model
     ClpDummyMatrix dummy(ncol, nrow, numelts);
     model.loadProblem(dummy,
                       colLower, colUpper, NULL, rowLower, rowUpper);
     double *y_def = new double[nrow];
     for (int k = 0; k < nrow; k++)
          y_def[k] = 0.0;
     y = y_def;
     double *dj_def = new double[ncol];
     for (int k = 0; k < ncol; k++)
          dj_def[k] = 1.0;
     dj = dj_def;
     // delete arrays
     delete [] U_def;
     delete [] L_def;
     // Should be sets
     model.rhs_ = rhs;
     model.x_ = x;
     model.y_ = y;
     model.dj_ = dj;
     model.xsize_ = 50 / ncol;
     model.xsize_ = CoinMin(model.xsize_, 1.0);
     model.zsize_ = 1;
}
