/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).
/*
   Authors

   John Tomlin

 */
#ifndef ClpPdco_H
#define ClpPdco_H

#include "ClpInterior.hpp"

/** This solves problems in Primal Dual Convex Optimization

    It inherits from ClpInterior.  It has no data of its own and
    is never created - only cast from a ClpInterior object at algorithm time.

*/
class ClpPdco : public ClpInterior {

public:

     /**@name Description of algorithm */
     //@{
     /** Pdco algorithm

         Method


     */

     int pdco();
     // ** Temporary version
     int  pdco( ClpPdcoBase * stuff, Options &options, Info &info, Outfo &outfo);

     //@}

     /**@name Functions used in pdco */
     //@{
     /// LSQR
     void lsqr();

     void matVecMult( int, double *, double *);

     void matVecMult( int, CoinDenseVector<double> &, double *);

     void matVecMult( int, CoinDenseVector<double> &, CoinDenseVector<double> &);

     void matVecMult( int, CoinDenseVector<double> *, CoinDenseVector<double> *);

     void getBoundTypes( int *, int *, int *, int**);

     void getGrad(CoinDenseVector<double> &x, CoinDenseVector<double> &grad);

     void getHessian(CoinDenseVector<double> &x, CoinDenseVector<double> &H);

     double getObj(CoinDenseVector<double> &x);

     void matPrecon( double, double *, double *);

     void matPrecon( double, CoinDenseVector<double> &, double *);

     void matPrecon( double, CoinDenseVector<double> &, CoinDenseVector<double> &);

     void matPrecon( double, CoinDenseVector<double> *, CoinDenseVector<double> *);
     //@}

};
#endif
