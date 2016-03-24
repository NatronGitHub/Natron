// Copyright (C) 2006, International Business Machines
// Corporation and others.  All Rights Reserved.
#ifndef OsiOpbdpSolve_H
#define OsiOpbdpSolve_H

#include <string>

#include "OsiSolverInterface.hpp"

/** Solve pure 0-1 with integral coefficients etc (or can be made by scaling) using opdbp
    The solution will be set in model
    If no solution then returns 0, if not suitable returns -1
 */
int solveOpbdp(OsiSolverInterface * model);
/** Find all solutions of a pure 0-1 with integral coefficients etc (or can be made by scaling) using opdbp
    Returns an array of bit solution vectors.
    i is 1 if bit set (see below) 
    If no solution then numberFound will be 0, if not suitable -1

    This needs the line at about 206 of EnumerateOpt.cpp

      local_maximum = value + 1; // try to reach that one!

    replaced by

      if (verbosity!=-1) {
        local_maximum = value + 1; // try to reach that one!
      } else {
        local_maximum = 0;
        void opbdp_save_solution(OrdInt & sol);
        OrdInt sol = last_solution.to_OrdInt();
        opbdp_save_solution(sol); // save solution
      }
 */
unsigned int ** solveOpbdp(const OsiSolverInterface * model,int & numberFound);

inline bool atOne(int i,unsigned int * array) {
  return ((array[i>>5]>>(i&31))&1)!=0;
}
inline void setAtOne(int i,unsigned int * array,bool trueFalse) {
  unsigned int & value = array[i>>5];
  int bit = i&31;
  if (trueFalse)
    value |= (1<<bit);
  else
    value &= ~(1<<bit);
}
#endif
