/* $Id$ */
// Copyright (C) 2008, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "ClpSimplex.hpp"
#include "CoinStructuredModel.hpp"
#include <iomanip>

int main(int argc, const char *argv[])
{
     /* Create a structured model by reading mps file and trying
        Dantzig-Wolfe decomposition (that's the 1 parameter)
     */
     // At present D-W rows are hard coded - will move stuff from OSL
#if defined(NETLIBDIR)
     CoinStructuredModel model((argc < 2) ? NETLIBDIR "/czprob.mps"
                               : argv[1], 1);
#else
     if (argc<2) {
          fprintf(stderr, "Do not know where to find netlib MPS files.\n");
          return 1;
     }
     CoinStructuredModel model(argv[1], 1);
#endif
     if (!model.numberRows())
          exit(10);
     // Get default solver - could change stuff
     ClpSimplex solver;
     /*
       This driver does a simple Dantzig Wolfe decomposition
     */
     solver.solve(&model);
     // Double check
     solver.primal(1);
     return 0;
}
