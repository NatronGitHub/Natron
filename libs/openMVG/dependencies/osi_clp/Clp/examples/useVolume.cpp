/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "ClpSimplex.hpp"
#include "ClpFactorization.hpp"
#include "VolVolume.hpp"

//#############################################################################

class lpHook : public VOL_user_hooks {
private:
     lpHook(const lpHook&);
     lpHook& operator= (const lpHook&);
private:
     /// Pointer to dense vector of structural variable upper bounds
     double  *colupper_;
     /// Pointer to dense vector of structural variable lower bounds
     double  *collower_;
     /// Pointer to dense vector of objective coefficients
     double  *objcoeffs_;
     /// Pointer to dense vector of right hand sides
     double  *rhs_;
     /// Pointer to dense vector of senses
     char    *sense_;

     /// The problem matrix in a row ordered form
     CoinPackedMatrix rowMatrix_;
     /// The problem matrix in a column ordered form
     CoinPackedMatrix colMatrix_;

public:
     lpHook(const double* clb, const double* cub, const double* obj,
            const double* rhs, const char* sense, const CoinPackedMatrix& mat);
     virtual ~lpHook();

public:
     // for all hooks: return value of -1 means that volume should quit
     /** compute reduced costs
         @param u (IN) the dual variables
         @param rc (OUT) the reduced cost with respect to the dual values
     */
     virtual int compute_rc(const VOL_dvector& u, VOL_dvector& rc);

     /** Solve the subproblem for the subgradient step.
         @param dual (IN) the dual variables
         @param rc (IN) the reduced cost with respect to the dual values
         @param lcost (OUT) the lagrangean cost with respect to the dual values
         @param x (OUT) the primal result of solving the subproblem
         @param v (OUT) b-Ax for the relaxed constraints
         @param pcost (OUT) the primal objective value of <code>x</code>
     */
     virtual int solve_subproblem(const VOL_dvector& dual, const VOL_dvector& rc,
                                  double& lcost, VOL_dvector& x, VOL_dvector& v,
                                  double& pcost);
     /** Starting from the primal vector x, run a heuristic to produce
         an integer solution
         @param x (IN) the primal vector
         @param heur_val (OUT) the value of the integer solution (return
         <code>DBL_MAX</code> here if no feas sol was found
     */
     virtual int heuristics(const VOL_problem& p,
                            const VOL_dvector& x, double& heur_val) {
          return 0;
     }
};

//#############################################################################

lpHook::lpHook(const double* clb, const double* cub, const double* obj,
               const double* rhs, const char* sense,
               const CoinPackedMatrix& mat)
{
     const int colnum = mat.getNumCols();
     const int rownum = mat.getNumRows();

     colupper_ = new double[colnum];
     collower_ = new double[colnum];
     objcoeffs_ = new double[colnum];
     rhs_ = new double[rownum];
     sense_ = new char[rownum];

     std::copy(clb, clb + colnum, collower_);
     std::copy(cub, cub + colnum, colupper_);
     std::copy(obj, obj + colnum, objcoeffs_);
     std::copy(rhs, rhs + rownum, rhs_);
     std::copy(sense, sense + rownum, sense_);

     if (mat.isColOrdered()) {
          colMatrix_.copyOf(mat);
          rowMatrix_.reverseOrderedCopyOf(mat);
     } else {
          rowMatrix_.copyOf(mat);
          colMatrix_.reverseOrderedCopyOf(mat);
     }
}

//-----------------------------------------------------------------------------

lpHook::~lpHook()
{
     delete[] colupper_;
     delete[] collower_;
     delete[] objcoeffs_;
     delete[] rhs_;
     delete[] sense_;
}

//#############################################################################

int
lpHook::compute_rc(const VOL_dvector& u, VOL_dvector& rc)
{
     rowMatrix_.transposeTimes(u.v, rc.v);
     const int psize = rowMatrix_.getNumCols();

     for (int i = 0; i < psize; ++i)
          rc[i] = objcoeffs_[i] - rc[i];
     return 0;
}

//-----------------------------------------------------------------------------

int
lpHook::solve_subproblem(const VOL_dvector& dual, const VOL_dvector& rc,
                         double& lcost, VOL_dvector& x, VOL_dvector& v,
                         double& pcost)
{
     int i;
     const int psize = x.size();
     const int dsize = v.size();

     // compute the lagrangean solution corresponding to the reduced costs
     for (i = 0; i < psize; ++i)
          x[i] = (rc[i] >= 0.0) ? collower_[i] : colupper_[i];

     // compute the lagrangean value (rhs*dual + primal*rc)
     lcost = 0;
     for (i = 0; i < dsize; ++i)
          lcost += rhs_[i] * dual[i];
     for (i = 0; i < psize; ++i)
          lcost += x[i] * rc[i];

     // compute the rhs - lhs
     colMatrix_.times(x.v, v.v);
     for (i = 0; i < dsize; ++i)
          v[i] = rhs_[i] - v[i];

     // compute the lagrangean primal objective
     pcost = 0;
     for (i = 0; i < psize; ++i)
          pcost += x[i] * objcoeffs_[i];

     return 0;
}

//#############################################################################

int main(int argc, const char *argv[])
{
     ClpSimplex  model;
     int status;

     if (argc < 2) {
#if defined(SAMPLEDIR)
          status = model.readMps(SAMPLEDIR "/p0033.mps", true);
#else
          fprintf(stderr, "Do not know where to find sample MPS files.\n");
          exit(1);
#endif
     } else
          status = model.readMps(argv[1], true);
     if( status != 0 )
     {
        printf("Error %d reading MPS file\n", status);
        return status;
     }
     /*
       This driver uses volume algorithm
       then does dual - after adjusting costs
       then solves real problem
     */

     // do volume for a bit
     VOL_problem volprob;
     const CoinPackedMatrix* mat = model.matrix();
     const int psize = mat->getNumCols();
     const int dsize = mat->getNumRows();
     char * sense = new char[dsize];
     double * rhs = new double[dsize];
     const double * rowLower = model.rowLower();
     const double * rowUpper = model.rowUpper();
     // Set the lb/ub on the duals
     volprob.dsize = dsize;
     volprob.psize = psize;
     volprob.dual_lb.allocate(dsize);
     volprob.dual_ub.allocate(dsize);
     volprob.dsol.allocate(dsize);
     int i;
     for (i = 0; i < dsize; ++i) {
          if (rowUpper[i] == rowLower[i]) {
               // 'E':
               volprob.dual_lb[i] = -1.0e31;
               volprob.dual_ub[i] = 1.0e31;
               rhs[i] = rowUpper[i];
               sense[i] = 'E';
          } else if (rowLower[i] < -0.99e10 && rowUpper[i] < 0.99e10) {
               // 'L':
               volprob.dual_lb[i] = -1.0e31;
               volprob.dual_ub[i] = 0.0;
               rhs[i] = rowUpper[i];
               sense[i] = 'L';
          } else if (rowLower[i] > -0.99e10 && rowUpper[i] > 0.99e10) {
               // 'G':
               volprob.dual_lb[i] = 0.0;
               volprob.dual_ub[i] = 1.0e31;
               rhs[i] = rowLower[i];
               sense[i] = 'G';
          } else {
               printf("Volume Algorithm can't work if there is a non ELG row\n");
               abort();
          }
     }
     // Can't use read_param as private
     // anyway I want automatic use - so maybe this is problem
#if 0
     FILE* infile = fopen("parameters", "r");
     if (!infile) {
          printf("Failure to open parameter file\n");
     } else {
          volprob.read_params("parameters");
     }
#endif
#if 0
     // should save and restore bounds
     model.tightenPrimalBounds();
#else
     double * colUpper = model.columnUpper();
     for (i = 0; i < psize; i++)
          colUpper[i] = 1.0;
#endif
     lpHook myHook(model.getColLower(), model.getColUpper(),
                   model.getObjCoefficients(),
                   rhs, sense, *mat);
     // move duals
     double * pi = model.dualRowSolution();
     memcpy(volprob.dsol.v, pi, dsize * sizeof(double));
     volprob.solve(myHook,  false /* not warmstart */);
     // For now stop as not doing any good
     exit(77);
     // create objectives
     int numberRows = model.numberRows();
     int numberColumns = model.numberColumns();
     memcpy(pi, volprob.dsol.v, numberRows * sizeof(double));
#define MODIFYCOSTS
#ifdef MODIFYCOSTS
     double * saveObj = new double[numberColumns];
     memcpy(saveObj, model.objective(), numberColumns * sizeof(double));
     memcpy(model.dualColumnSolution(), model.objective(),
            numberColumns * sizeof(double));
     model.clpMatrix()->transposeTimes(-1.0, pi, model.dualColumnSolution());
     memcpy(model.objective(), model.dualColumnSolution(),
            numberColumns * sizeof(double));
     const double * rowsol = model.primalRowSolution();
     //const double * rowLower = model.rowLower();
     //const double * rowUpper = model.rowUpper();
     double offset = 0.0;
     for (i = 0; i < numberRows; i++) {
          offset += pi[i] * rowsol[i];
     }
     double value2;
     model.getDblParam(ClpObjOffset, value2);
     printf("Offset %g %g\n", offset, value2);
     model.setRowObjective(pi);
     // zero out pi
     memset(pi, 0, numberRows * sizeof(double));
#endif
     // Could put some in basis - only partially tested
     model.allSlackBasis();
     model.factorization()->maximumPivots(1000);
     //model.setLogLevel(63);
     // solve
     model.dual(1);
     //model.primal(1);
#ifdef MODIFYCOSTS
     memcpy(model.objective(), saveObj, numberColumns * sizeof(double));
     // zero out pi
     memset(pi, 0, numberRows * sizeof(double));
     model.setRowObjective(pi);
     delete [] saveObj;
     model.primal();
#endif

     return 0;
}
