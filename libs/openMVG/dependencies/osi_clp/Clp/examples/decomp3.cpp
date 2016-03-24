/* $Id$ */
// Copyright (C) 2008, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "ClpSimplex.hpp"
#include "ClpPresolve.hpp"
#include "CoinStructuredModel.hpp"
#include "CoinTime.hpp"
#include <iomanip>

int main(int argc, const char *argv[])
{
     /* Create a structured model by reading mps file and trying
        Dantzig-Wolfe or Benders decomposition
     */
     // Get maximum number of blocks
     int maxBlocks = 50;
     if (argc > 2)
          maxBlocks = atoi(argv[2]);
     int decompose = 1;
     if (maxBlocks < 0) {
          maxBlocks = -maxBlocks;
          decompose = 2;
     }
     if (maxBlocks < 2) {
          printf("Second parameters is maximum number of blocks >=2)\n");
          exit(5);
     } else {
          printf("Allowing at most %d blocks\n", maxBlocks);
     }
     //#define PRESOLVE
#ifndef PRESOLVE
#if defined(NETLIBDIR)
     CoinStructuredModel model((argc < 2) ? NETLIBDIR "/czprob.mps"
                               : argv[1], decompose, maxBlocks);
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
     // change factorization frequency from 200
     solver.setFactorizationFrequency(100 + model.numberRows() / 50);
     /*
       This driver does a simple Dantzig Wolfe decomposition
     */
     double time1 = CoinCpuTime() ;
     solver.solve(&model);
     std::cout << "model took " << CoinCpuTime() - time1 << " seconds" << std::endl;
     // Double check
     solver.primal(1);
#else
     ClpSimplex  model;
#if defined(NETLIBDIR)
     int status = model.readMps((argc < 2) ? NETLIBDIR "/czprob.mps"
                                : argv[1]);
#else
     if (argc<2) {
          fprintf(stderr, "Do not know where to find netlib MPS files.\n");
          return 1;
     }
     int status = model.readMps(argv[1]);
#endif
     if (status) {
          fprintf(stdout, "Bad readMps %s\n", argv[1]);
          exit(1);
     }
     ClpSimplex * model2 = &model;

     CoinStructuredModel modelB;
     modelB.decompose(*model2->matrix(),
                      model2->rowLower(), model2->rowUpper(),
                      model2->columnLower(), model2->columnUpper(),
                      model2->objective(), 1, maxBlocks,
                      model2->objectiveOffset());
     // change factorization frequency from 200
     model2->setFactorizationFrequency(100 + modelB.numberRows() / 50);
     /*
       This driver does a simple Dantzig Wolfe decomposition
     */
     double time1 = CoinCpuTime() ;
     model2->solve(&modelB);
     std::cout << "model took " << CoinCpuTime() - time1 << " seconds" << std::endl;
     // But resolve for safety
     model.primal(1);
#endif
     return 0;
     ClpSimplex solver2;
#if defined(NETLIBDIR)
     solver2.readMps((argc < 2) ? NETLIBDIR "/czprob.mps" : argv[1]);
#else
     if (argc<2) {
          fprintf(stderr, "Do not know where to find netlib MPS files.\n");
          return 1;
     }
     solver2.readMps(argv[1]);
#endif
     time1 = CoinCpuTime() ;
     solver2.dual();
     std::cout << "second try took " << CoinCpuTime() - time1 << " seconds" << std::endl;
     return 0;
}
