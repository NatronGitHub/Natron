/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "ClpSimplex.hpp"
int main(int argc, const char *argv[])
{
     ClpSimplex  model;
     int status;
     if (argc < 2) {
#if defined(SAMPLEDIR)
/*
  SAMPLEDIR should "path/to/Data/Sample"
  Include the quotes and final path separator.
*/
          status = model.readMps(SAMPLEDIR "/p0033.mps", true);
#else
          fprintf(stderr, "Do not know where to find sample MPS files.\n");
          exit(1);
#endif
     } else
          status = model.readMps(argv[1]);
     if (!status) {
          model.primal();
     }
     return 0;
}
