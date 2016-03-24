/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

// @TODO the ifdef WSSMP_BARRIER currently does not work - the define is never set
// best would be if one could tell ClpModel to use the wssmp factorization code, if available, without having to check for some define

#include "ClpInterior.hpp"
#include "ClpSimplex.hpp"
#ifdef WSSMP_BARRIER
#include "ClpCholeskyWssmp.hpp"
#endif
#include "ClpCholeskyDense.hpp"
int main(int argc, const char *argv[])
{
     ClpInterior  model;
     int status;
     if (argc < 2) {
#if defined(SAMPLEDIR)
          status = model.readMps(SAMPLEDIR "/p0033.mps", true);
#else
          fprintf(stderr, "Do not know where to find sample MPS files.\n");
          exit(1);
#endif
     } else
          status = model.readMps(argv[1]);
     if (status) {
          printf("errors on input\n");
          exit(77);
     }
     // ** note this does not have presolve
#ifdef WSSMP_BARRIER
     ClpCholeskyWssmp * cholesky = new ClpCholeskyWssmp();
#else
     ClpCholeskyDense * cholesky = new ClpCholeskyDense();
#endif
     model.setCholesky(cholesky);
     model.primalDual();
     // Do crossover
     ClpSimplex model2(model);
     // make sure no status left
     model2.createStatus();
     model2.primal(1);
     return 0;
}
