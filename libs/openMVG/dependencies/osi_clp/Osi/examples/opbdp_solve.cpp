// Copyright (C) 2006, International Business Machines
// Corporation and others.  All Rights Reserved.

#include "OsiConfig.h"

#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cmath>
#include <cfloat>
#include <string>
#include <iostream>

#include "CoinPragma.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPackedVector.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinTime.hpp"

#include "opbdp_solve.hpp"
#include "PBCS.h"
// Not threadsafe
static unsigned int ** opbdp_solution=NULL;
static int opbdp_number_solutions=0;
static int opbdp_maximum_solutions=0;
static int size_solution=0;
static PBCS *save_pbcs=NULL;
void opbdp_save_solution(OrdInt & sol)
{
  if (opbdp_number_solutions==opbdp_maximum_solutions) {
    opbdp_maximum_solutions = opbdp_maximum_solutions*2+100;
    unsigned int ** temp = new unsigned int * [opbdp_maximum_solutions];
    CoinMemcpyN(opbdp_solution,opbdp_number_solutions,temp);
    CoinZeroN(temp+opbdp_number_solutions,opbdp_maximum_solutions-opbdp_number_solutions);
    delete [] opbdp_solution;
    opbdp_solution=temp;
  }
  unsigned int * array = new unsigned int [size_solution];
  CoinZeroN(array,size_solution);
  opbdp_solution[opbdp_number_solutions++]=array;
  sol.first();
  while(!sol.last()) {
    int var = sol.next();
    if (var > 0)
      setAtOne(var-1,array,true);
  }
  OrdInt Fixed = save_pbcs->get_fixed();
  Fixed.first();
  while(!Fixed.last()) {
    int flit = Fixed.next();
    if (flit > 0)
      setAtOne(flit-1,array,true);
  }
  // add hidden ones due to equations
  save_pbcs->Eq.first();
  while(!save_pbcs->Eq.last()) {
    int l = save_pbcs->Eq.next();
    int r = save_pbcs->Eq(l);
    if (sol.member(r) || Fixed.member(r)) 
      setAtOne(l-1,array,true);
    else if (sol.member(-r) || Fixed.member(-r))
      setAtOne(l-1,array,true);
    // just positives are added
  }
}
//-------------------------------------------------------------------
// Returns the greatest common denominator of two 
// positive integers, a and b, found using Euclid's algorithm 
//-------------------------------------------------------------------
static int gcd(int a, int b) 
{
  int remainder = -1;
  // make sure a<=b (will always remain so)
  if(a > b) {
    // Swap a and b
    int temp = a;
    a = b;
    b = temp;
  }
  // if zero then gcd is nonzero (zero may occur in rhs of packed)
  if (!a) {
    if (b) {
      return b;
    } else {
      printf("**** gcd given two zeros!!\n");
      abort();
    }
  }
  while (remainder) {
    remainder = b % a;
    b = a;
    a = remainder;
  }
  return b;
}
static int solve(const OsiSolverInterface * model,PBCS & pbcs, OrdInt & sol)
{
  int numberColumns = model->getNumCols();
  size_solution = (numberColumns+31)/32;
  int numberRows = model->getNumRows();
  bool all01=true;
  int i;
  const double * objective = model->getObjCoefficients();
  double infinity = model->getInfinity();
  const CoinPackedMatrix * matrix = model->getMatrixByRow();
  const double * columnLower = model->getColLower();
  const double * columnUpper = model->getColUpper();
  const double * rowLower = model->getRowLower();
  const double * rowUpper = model->getRowUpper();
  for (i = 0; i < numberColumns; i++) {
    if (!model->isInteger(i)||columnLower[i]<0.0||columnUpper[i]>1.0) {
      all01=false;
      break;
    }
  }
  if (!all01)
    return -1;
  // Get scale factors to make integral
  const int * column = matrix->getIndices();
  const int * rowLength = matrix->getVectorLengths();
  const CoinBigIndex * rowStart = matrix->getVectorStarts();
  const double * elementByRow = matrix->getElements();
  // objective not too important
  double maximumObjElement = 0.0 ;
  for (i = 0 ; i < numberColumns ; i++) 
    maximumObjElement = CoinMax(maximumObjElement,fabs(objective[i])) ;
  int objGood = 0 ;
  double objMultiplier = 2520.0 ;
  bool good=true;
  if (maximumObjElement) {
    while (10.0*objMultiplier*maximumObjElement < 1.0e9)
      objMultiplier *= 10.0 ;
    if (maximumObjElement*2520>=1.0e9)
      objMultiplier=1.0;
    double tolerance = 1.0e-9*objMultiplier;
    for (i = 0 ; i < numberColumns ; i++) {
      double value=fabs(objective[i])*objMultiplier ;
      if (!value)
        continue;
      int nearest = (int) floor(value+0.5) ;
      if (fabs(value-floor(value+0.5)) > tolerance) {
        // just take for now
        objGood = 1 ;
        good=false;
        break ;
      } else if (!objGood) {
        objGood = nearest ;
      } else {
        objGood = gcd(objGood,nearest) ;
      }
    }
    objMultiplier /= objGood;
    if (!good) {
      printf("Unable to scale objective correctly - maximum %g\n",
             maximumObjElement);
      objMultiplier = 1.0e7/maximumObjElement;
    }
  } else {
    objMultiplier=0.0; // no objective
  }
  double maximumElement=0.0;
  // now real stuff
  for (i=0;i<numberRows;i++) {
    if (rowLower[i]!=-infinity)
      maximumElement = CoinMax(maximumElement,fabs(rowLower[i])) ;
    if (rowUpper[i]!=infinity)
      maximumElement = CoinMax(maximumElement,fabs(rowUpper[i])) ;
    for (CoinBigIndex j=rowStart[i];j<rowStart[i]+rowLength[i];j++) 
      maximumElement = CoinMax(maximumElement,fabs(elementByRow[j])) ;
  }
  assert (maximumElement);
  int elGood = 0 ;
  double elMultiplier = 2520.0 ;
  while (10.0*elMultiplier*maximumElement < 1.0e8)
    elMultiplier *= 10.0 ;
  if (maximumElement*2520>=1.0e8)
      elMultiplier=1.0;
  double tolerance = 1.0e-8*elMultiplier;
  good=true;
  for (i=0;i<numberRows;i++) {
    if (rowLower[i]!=-infinity) {
      double value=fabs(rowLower[i])*elMultiplier ;
      if (value) {
        int nearest = (int) floor(value+0.5) ;
        if (fabs(value-floor(value+0.5)) > tolerance) {
          elGood = 0 ;
          good=false;
          break ;
        } else if (!elGood) {
          elGood = nearest ;
        } else {
          elGood = gcd(elGood,nearest) ;
        }
      }
    }
    if (rowUpper[i]!=infinity) {
      double value=fabs(rowUpper[i])*elMultiplier ;
      if (value) {
        int nearest = (int) floor(value+0.5) ;
        if (fabs(value-floor(value+0.5)) > tolerance) {
          elGood = 0 ;
          good=false;
          break ;
        } else if (!elGood) {
          elGood = nearest ;
        } else {
          elGood = gcd(elGood,nearest) ;
        }
      }
    }
    for (CoinBigIndex j=rowStart[i];j<rowStart[i]+rowLength[i];j++) {
      double value=fabs(elementByRow[j])*elMultiplier ;
      if (!value)
        continue;
      int nearest = (int) floor(value+0.5) ;
      if (fabs(value-floor(value+0.5)) > tolerance) {
        elGood = 0 ;
        good=false;
        break ;
      } else if (!elGood) {
        elGood = nearest ;
      } else {
        elGood = gcd(elGood,nearest) ;
      }
    }
  }
  if (!good)
    return -1; // no good
  // multiplier
  elMultiplier /= elGood;
  double objOffset=0.0;
  model->getDblParam(OsiObjOffset,objOffset);
  int doMax= (model->getObjSense()<0.0) ? 1 : 0;
  printf("Objective multiplier is %g, element mutiplier %g, offset %g\n",
         objMultiplier,elMultiplier,objOffset);

  Products objf;
  Atoms atoms;  
  save_pbcs = & pbcs;
  pbcs.set_atoms(&atoms);
  pbcs.set_enum_heuristic(0);
  for (i=0;i<numberColumns;i++) {
    char name[9];
    sprintf(name,"C%7.7d",i);
    SimpleString thisName(name);
    int literal = atoms(thisName);
    assert (literal==i+1);
    int value = (int) floor(objective[i]*objMultiplier+0.5);
    objf.add(value,literal);
  }
  LPBIneqSet equations;
  for (i=0;i<numberRows;i++) {
    Products row;
    if (rowLower[i]==-infinity||rowUpper[i]==infinity) {
      for (CoinBigIndex j=rowStart[i];j<rowStart[i]+rowLength[i];j++) {
        int value = (int) floor(elementByRow[j]*elMultiplier+0.5);
        row.add(value,column[j]+1);
      }
      Coefficient rhs;
      if (rowLower[i]==-infinity) {
        int value = (int) floor(rowUpper[i]*elMultiplier+0.5);
        rhs=value;
        row.set_constant(row.get_constant() - rhs);
        Coefficient rhs2 = -row.get_constant();
        row.set_constant(0);
        row.mult(-1);
        row.set_constant(row.get_constant() + rhs2);
      } else {
        int value = (int) floor(rowLower[i]*elMultiplier+0.5);
        rhs=value;
        row.set_constant(row.get_constant() - rhs);
      }
      rhs = -row.get_constant();
      row.set_constant(0);
      LPBIneq lpbi(row,rhs);
      pbcs.add(lpbi);
    } else {
      // put in both
      {
        Products row;
        for (CoinBigIndex j=rowStart[i];j<rowStart[i]+rowLength[i];j++) {
          int value = (int) floor(elementByRow[j]*elMultiplier+0.5);
          row.add(value,column[j]+1);
        }
        Coefficient rhs;
        int value = (int) floor(rowUpper[i]*elMultiplier+0.5);
        rhs=value;
        row.set_constant(row.get_constant() - rhs);
        Coefficient rhs2 = -row.get_constant();
        row.set_constant(0);
        row.mult(-1);
        row.set_constant(row.get_constant() + rhs2);
        rhs = -row.get_constant();
        row.set_constant(0);
        LPBIneq lpbi(row,rhs);
        pbcs.add(lpbi);
      }
      {
        Products row;
        for (CoinBigIndex j=rowStart[i];j<rowStart[i]+rowLength[i];j++) {
          int value = (int) floor(elementByRow[j]*elMultiplier+0.5);
          row.add(value,column[j]+1);
        }
        Coefficient rhs;
        int value = (int) floor(rowLower[i]*elMultiplier+0.5);
        rhs=value;
        row.set_constant(row.get_constant() - rhs);
        rhs = -row.get_constant();
        row.set_constant(0);
        LPBIneq lpbi(row,rhs);
        pbcs.add(lpbi);
      }
    }
  }
  // Add extra rows to fix
  int numberFixed=0;
  for (i=0;i<numberColumns;i++) {
    if (columnLower[i]) {
      numberFixed++;
      Products row;
      row.add(1,i+1);
      Coefficient rhs;
      row.set_constant(row.get_constant() - 1);
      rhs = -row.get_constant();
      row.set_constant(0);
      LPBIneq lpbi(row,rhs);
      pbcs.add(lpbi);
    } else if (!columnUpper[i]) {
      numberFixed++;
      Products row;
      row.add(1,i+1);
      Coefficient rhs;
      rhs=0;
      row.set_constant(row.get_constant() - rhs);
      Coefficient rhs2 = -row.get_constant();
      row.set_constant(0);
      row.mult(-1);
      row.set_constant(row.get_constant() + rhs2);
      rhs = -row.get_constant();
      row.set_constant(0);
      LPBIneq lpbi(row,rhs);
      pbcs.add(lpbi);
    }
  }
  Coefficient value;
  int consistent=0;
  pbcs.Enum.stat_reset();
  // FM preprocessing
  if (numberFixed<numberColumns) {
    OrdInt fixed_lits = pbcs.fixing();
    if (!fixed_lits.empty())
      {
        int lit;
        std::cout << "Fixing : ";
        fixed_lits.first();
        while(!fixed_lits.last())
          {
            objf.fix(lit = fixed_lits.next());
          }
        if (pbcs.is_unsatisfiable())
          std::cout << "\n'fixing' detected inconsistency"<<std::endl;		
        else if (fixed_lits.length() == 1)		
          std::cout << "\n1 Literal has been fixed by 'fixing' "<<std::endl;
        else
          std::cout << "\n" << fixed_lits.length() << " Literals have been fixed by 'fixing' "<<std::endl;	  
      }
  }
  if (numberFixed<numberColumns) {
    int changed = pbcs.coefficient_reduction_bigM();
    if (changed > 1)	    
      std::cout << "Big M coefficient reduction changed " << changed << " coefficients"<<std::endl;
    else if (changed == 1)
      std::cout << "Big M coefficient reduction changed " << changed << " coefficient"<<std::endl;
    std::cout.flush();
  }
  pbcs.Enum.stat_reset();
  if (pbcs.is_unsatisfiable())
    {
      value = 0;
      consistent = 0;
    }
  else if (pbcs.is_empty())
    {
      value = doMax ? objf.get_upperbound() : objf.get_lowerbound();
      consistent = 1;
    }
  else
    {
      ////////////////////////////////
      value = pbcs.Enum.optimize(objf,consistent,sol,doMax,0);
      ////////////////////////////////
    }
  if (consistent == 0)
    return 0;
  else
    return 1;
}
/* Solve pure 0-1 with integral coefficients etc (or can be made by scaling) using opdbp
    The solution will be set in model
    If no solution then returns 0, if not suitable returns -1
 */
int 
solveOpbdp(OsiSolverInterface * model)
{
  PBCS pbcs;
  OrdInt sol;
  pbcs.set_enum_verbosity(3);
  pbcs.set_verbosity(3);
  double time1 = CoinCpuTime();
  int numberFound=solve(model,pbcs,sol);
  if (numberFound>0) {
    const double * objective = model->getObjCoefficients();
    double objOffset=0.0;
    model->getDblParam(OsiObjOffset,objOffset);
    int numberColumns = model->getNumCols();
    double * solution = new double [numberColumns];
    CoinZeroN(solution,numberColumns);
    sol.first();
    while(!sol.last()) {
      int var = sol.next();
      if (var > 0)
        solution[var-1]=1.0;
    }
    OrdInt Fixed = pbcs.get_fixed();
    Fixed.first();
    while(!Fixed.last()) {
      int flit = Fixed.next();
      if (flit > 0)
        solution[flit-1]=1.0;
    }
    // add hidden ones due to equations
    pbcs.Eq.first();
    while(!pbcs.Eq.last()) {
      int l = pbcs.Eq.next();
      int r = pbcs.Eq(l);
      if (sol.member(r) || Fixed.member(r)) 
        solution[l-1]=1.0;
      else if (sol.member(-r) || Fixed.member(-r))
        solution[l-1]=1.0;
      // just positives are added
    }
    numberFound=1;
    double objValue = - objOffset;
    for (int i=0;i<numberColumns;i++)
      objValue += solution[i]*objective[i];
    std::cout<<"Global "<<(model->getObjSense()>0 ? "minimum" : "maximum")<<" of "<<objValue
             <<" took "<< CoinCpuTime()-time1<<" seconds"<<std::endl;
    std::cout << "Explored Nodes: " << pbcs.Enum.stat_nodes();
    std::cout << "   Max. Depth: " << pbcs.Enum.stat_maxdepth();
    std::cout << "   Fixed Literals: " << pbcs.Enum.stat_fixedlits() << ""<<std::endl; 
    //model->setObjValue(objValue);
    model->setColSolution(solution);
    delete [] solution;
  } else if (!numberFound) {
    std::cout << "Constraint Set is unsatisfiable"<<std::endl;
  } else {
    std::cout<<"Problem in incorrect format"<<std::endl;
  }
  return numberFound;
}
/* Find all solutions of a pure 0-1 with integral coefficients etc (or can be made by scaling) using opdbp
   Returns an array of bit solution vectors.
   i is 1 if bit set (see below) 
   If no solution then numberFound will be 0, if not suitable -1
*/
unsigned int ** 
solveOpbdp(const OsiSolverInterface * model,int & numberFound)
{
  PBCS pbcs;
  OrdInt sol;
  pbcs.set_enum_verbosity(-1);
  pbcs.set_verbosity(-1);
  numberFound=solve(model,pbcs,sol);
  if (numberFound>0||opbdp_number_solutions) {
    // all solutions
    numberFound = opbdp_number_solutions;
    return opbdp_solution;
  } else {
    // no solution
    return NULL;
  }
}
