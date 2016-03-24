/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).


// It also tests pdco

// This reads a network problem created by netgen which can be
// downloaded from www.netlib.org/lp/generators/netgen
// This is a generator due to Darwin Klingman

#include "ClpInterior.hpp"
#include "myPdco.hpp"
#include <stdio.h>
#include <assert.h>
#include <cmath>

// easy way to allow compiling all sources for this example within one file,
// so no need to do something special in Makefile
#include "myPdco.cpp"

int main(int argc, const char *argv[])
{

     // Get model in some way
     ClpInterior model;
     // Open graph and parameter files
     //FILE *fpin = fopen("./g.graph","r");
     //FILE *fpp = fopen("./gparm","r");
     FILE *fpin = fopen("./g.tiny", "r");
     FILE *fpp = fopen("./gparm.tiny", "r");
     assert(fpin);
     assert(fpp);
     myPdco stuff(model, fpin, fpp);
     Info info;
     Outfo outfo;
     Options options;


     /*
      *     Set the input parameters for LSQR.
      */
     options.gamma = stuff.getD1();
     options.delta = stuff.getD2();
     options.MaxIter = 40;
     options.FeaTol = 5.0e-4;
     options.OptTol = 5.0e-4;
     options.StepTol = 0.99;
     //  options.x0min = 10.0/num_cols;
     options.x0min = 0.01;
     options.z0min = 0.01;
     options.mu0 = 1.0e-6;
     options.LSmethod = 3;   // 1=Cholesky    2=QR    3=LSQR
     options.LSproblem = 1;  // See below
     options.LSQRMaxIter = 999;
     options.LSQRatol1 = 1.0e-3; // Initial  atol
     options.LSQRatol2 = 1.0e-6; // Smallest atol (unless atol1 is smaller)
     options.LSQRconlim = 1.0e12;
     info.atolmin = options.LSQRatol2;
     info.LSdamp = 0.0;
     // These are already set?
     model.xsize_ = 50.0 / (model.numberColumns());
     model.xsize_ = CoinMin(1.0, model.xsize_);

     /*
      *     Solve the test problem
      */
     model.pdco(&stuff, options, info, outfo);

     /*
      *     Examine the results.
      *     Print the residual norms RNORM and ARNORM given by LSQR, and then compute
      */
     return 0;
}
