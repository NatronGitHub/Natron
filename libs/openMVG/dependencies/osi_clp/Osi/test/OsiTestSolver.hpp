// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// This file is licensed under the terms of Eclipse Public License (EPL).

// this is a copy of VolVolume (stable/1.1 rev. 233)

#ifndef __OSITESTSOLVER_HPP__
#define __OSITESTSOLVER_HPP__

#include <cfloat>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <cstdlib>

#include "CoinFinite.hpp"

#ifndef VOL_DEBUG
// When VOL_DEBUG is 1, we check vector indices
#define VOL_DEBUG 0
#endif

template <class T> static inline T
VolMax(register const T x, register const T y) {
   return ((x) > (y)) ? (x) : (y);
}

template <class T> static inline T
VolAbs(register const T x) {
   return ((x) > 0) ? (x) : -(x);
}

//############################################################################

#if defined(VOL_DEBUG) && (VOL_DEBUG != 0)
#define VOL_TEST_INDEX(i, size)			\
{						\
   if ((i) < 0 || (i) >= (size)) {		\
      printf("bad VOL_?vector index\n");	\
      abort();					\
   }						\
}
#define VOL_TEST_SIZE(size)			\
{						\
   if (s <= 0) {				\
      printf("bad VOL_?vector size\n");		\
      abort();					\
   }						\
}
#else
#define VOL_TEST_INDEX(i, size)
#define VOL_TEST_SIZE(size)
#endif
      
//############################################################################

class VOL_dvector;
class VOL_ivector;
class VOL_primal;
class VOL_dual;
class VOL_swing;
class VOL_alpha_factor;
class VOL_vh;
class VOL_indc;
class VOL_user_hooks;
class VOL_problem;

//############################################################################

/**
   This class contains the parameters controlling the Volume Algorithm 
*/
struct VOL_parms {
   /** initial value of lambda */
   double lambdainit; 
   /** initial value of alpha */
   double alphainit; 
   /** minimum value for alpha */
   double alphamin;
   /** when little progress is being done, we multiply alpha by alphafactor */
   double alphafactor;

   /** initial upper bound of the value of an integer solution */
   double ubinit;

   /** accept if max abs viol is less than this */
   double primal_abs_precision;
   /** accept if abs gap is less than this */
   double gap_abs_precision; 
   /** accept if rel gap is less than this */
   double gap_rel_precision;  
   /** terminate if best_ub - lcost < granularity */
   double granularity;

   /** terminate if the relative increase in lcost through
       <code>ascent_check_invl</code> steps is less than this */
   double minimum_rel_ascent;
   /** when to check for sufficient relative ascent the first time */
   int    ascent_first_check;
   /** through how many iterations does the relative ascent have to reach a
       minimum */
   int    ascent_check_invl;
   
   /** maximum number of iterations  */
   int    maxsgriters; 

   /** controls the level of printing.
       The flag should the the 'OR'-d value of the following options:
       <ul>
       <li> 0 - print nothing
       <li> 1 - print iteration information
       <li> 2 - add lambda information
       <li> 4 - add number of Red, Yellow, Green iterations
       </ul>
       Default: 3
   */
   int    printflag; 
   /** controls how often do we print */
   int    printinvl; 
   /** controls how often we run the primal heuristic */
   int    heurinvl; 

   /** how many consecutive green iterations are allowed before changing
       lambda */
   int greentestinvl; 
   /** how many consecutive yellow iterations are allowed before changing
       lambda */
   int yellowtestinvl; 
   /** how many consecutive red iterations are allowed before changing
       lambda */
   int redtestinvl;

   /** number of iterations before we check if alpha should be decreased */
   int    alphaint; 

   /** name of file for saving dual solution */
   char* temp_dualfile;
};

//############################################################################

/** vector of doubles. It is used for most vector operations.

    Note: If <code>VOL_DEBUG</code> is <code>#defined</code> to be 1 then each
    time an entry is accessed in the vector the index of the entry is tested
    for nonnegativity and for being less than the size of the vector. It's
    good to turn this on while debugging, but in final runs it should be
    turned off (beause of the performance hit).
*/
class VOL_dvector {
public:
   /** The array holding the vector */
   double* v;
   /** The size of the vector */
   int sz;

public:
   /** Construct a vector of size s. The content of the vector is undefined. */
   VOL_dvector(const int s) {
      VOL_TEST_SIZE(s);
      v = new double[sz = s];
   }
   /** Default constructor creates a vector of size 0. */
   VOL_dvector() : v(0), sz(0) {}
   /** Copy constructor makes a replica of x. */
   VOL_dvector(const VOL_dvector& x) : v(0), sz(0) {
      sz = x.sz;
      if (sz > 0) {
	 v = new double[sz];
	 std::copy(x.v, x.v + sz, v);
      }
   }
   /** The destructor deletes the data array. */
   ~VOL_dvector() { delete[] v; }

   /** Return the size of the vector. */
   inline int size() const {return sz;}

   /** Return a reference to the <code>i</code>-th entry. */
   inline double& operator[](const int i) {
      VOL_TEST_INDEX(i, sz);
      return v[i];
   }

   /** Return the <code>i</code>-th entry. */
   inline double operator[](const int i) const {
      VOL_TEST_INDEX(i, sz);
      return v[i];
   }

   /** Delete the content of the vector and replace it with a vector of length
       0. */
   inline void clear() {
      delete[] v;
      v = 0;
      sz = 0;
   }
   /** Convex combination. Replace the current vector <code>v</code> with 
       <code>v = (1-gamma) v + gamma w</code>. */
   inline void cc(const double gamma, const VOL_dvector& w) {
      if (sz != w.sz) {
	 printf("bad VOL_dvector sizes\n");
	 abort();
      }
      double * p_v = v - 1;
      const double * p_w = w.v - 1;
      const double * const p_e = v + sz;
      const double one_gamma = 1.0 - gamma;
      while ( ++p_v != p_e ){
	 *p_v = one_gamma * (*p_v) + gamma * (*++p_w);
      }
   }

   /** delete the current vector and allocate space for a vector of size 
       <code>s</code>. */
   inline void allocate(const int s) {
      VOL_TEST_SIZE(s);
      delete[] v;                 
      v = new double[sz = s];
   }

   /** swaps the vector with <code>w</code>. */
   inline void swap(VOL_dvector& w) {
      std::swap(v, w.v);
      std::swap(sz, w.sz);
   }

   /** Copy <code>w</code> into the vector. */
   VOL_dvector& operator=(const VOL_dvector& w);
   /** Replace every entry in the vector with <code>w</code>. */
   VOL_dvector& operator=(const double w);
};

//-----------------------------------------------------------------------------
/** vector of ints. It's used to store indices, it has similar
    functions as VOL_dvector.

    Note: If <code>VOL_DEBUG</code> is <code>#defined</code> to be 1 then each
    time an entry is accessed in the vector the index of the entry is tested
    for nonnegativity and for being less than the size of the vector. It's
    good to turn this on while debugging, but in final runs it should be
    turned off (beause of the performance hit).
*/
class VOL_ivector {
public:
   /** The array holding the vector. */
   int* v;
   /** The size of the vector. */
   int sz;
public:
   /** Construct a vector of size s. The content of the vector is undefined. */
   VOL_ivector(const int s) {
      VOL_TEST_SIZE(s);
      v = new int[sz = s];
   }
   /** Default constructor creates a vector of size 0. */
   VOL_ivector() : v(0), sz(0) {}
   /** Copy constructor makes a replica of x. */
   VOL_ivector(const VOL_ivector& x) {
      sz = x.sz;
      if (sz > 0) {
	 v = new int[sz];
	 std::copy(x.v, x.v + sz, v);
      }
   }
   /** The destructor deletes the data array. */
   ~VOL_ivector(){
      delete [] v;
   }

   /** Return the size of the vector. */
   inline int size() const { return sz; }
   /** Return a reference to the <code>i</code>-th entry. */
   inline int& operator[](const int i) {
      VOL_TEST_INDEX(i, sz);
      return v[i];
   }

   /** Return the <code>i</code>-th entry. */
   inline int operator[](const int i) const {
      VOL_TEST_INDEX(i, sz);
      return v[i];
   }

   /** Delete the content of the vector and replace it with a vector of length
       0. */
   inline void clear() {
      delete[] v;
      v = 0;
      sz = 0;
   }

   /** delete the current vector and allocate space for a vector of size 
       <code>s</code>. */
   inline void allocate(const int s) {
      VOL_TEST_SIZE(s);
      delete[] v;
      v = new int[sz = s];
   }

   /** swaps the vector with <code>w</code>. */
   inline void swap(VOL_ivector& w) {
      std::swap(v, w.v);
      std::swap(sz, w.sz);
   }

   /** Copy <code>w</code> into the vector. */
   VOL_ivector& operator=(const VOL_ivector& v);      
   /** Replace every entry in the vector with <code>w</code>. */
   VOL_ivector& operator=(const int w);
};

//############################################################################
// A class describing a primal solution. This class is used only internally 
class VOL_primal {
public: 
   // objective value of this primal solution 
   double value;
   // the largest of the v[i]'s
   double viol;  
   // primal solution  
   VOL_dvector x;
   // v=b-Ax, for the relaxed constraints
   VOL_dvector v; 

   VOL_primal(const int psize, const int dsize) : x(psize), v(dsize) {}
   VOL_primal(const VOL_primal& primal) :
      value(primal.value), viol(primal.viol), x(primal.x), v(primal.v) {}
   ~VOL_primal() {}
   inline VOL_primal& operator=(const VOL_primal& p) {
      if (this == &p) 
	 return *this;
      value = p.value;
      viol = p.viol;
      x = p.x;
      v = p.v;
      return *this;
   }

   // convex combination. data members in this will be overwritten
   // convex combination between two primal solutions
   // x <-- alpha x + (1 - alpha) p.x
   // v <-- alpha v + (1 - alpha) p.v
   inline void cc(const double alpha, const VOL_primal& p) {
      value = alpha * p.value + (1.0 - alpha) * value;
      x.cc(alpha, p.x);
      v.cc(alpha, p.v);
   }
   // find maximum of v[i]
   void find_max_viol(const VOL_dvector& dual_lb, 
		      const VOL_dvector& dual_ub);
};

//-----------------------------------------------------------------------------
// A class describing a dual solution. This class is used only internally 
class VOL_dual {
public:
   // lagrangian value
   double lcost; 
   // reduced costs * (pstar-primal)
   double xrc;
   // this information is only printed
   // dual vector
   VOL_dvector u; 

   VOL_dual(const int dsize) : u(dsize) { u = 0.0;}
   VOL_dual(const VOL_dual& dual) :
      lcost(dual.lcost), xrc(dual.xrc), u(dual.u) {}
   ~VOL_dual() {}
   inline VOL_dual& operator=(const VOL_dual& p) {
      if (this == &p) 
	 return *this;
      lcost = p.lcost;
      xrc = p.xrc;
      u = p.u;
      return *this;
   }
   // dual step
   void   step(const double target, const double lambda,
	       const VOL_dvector& dual_lb, const VOL_dvector& dual_ub,
	       const VOL_dvector& v);
   double ascent(const VOL_dvector& v, const VOL_dvector& last_u) const;
   void   compute_xrc(const VOL_dvector& pstarx, const VOL_dvector& primalx,
		      const VOL_dvector& rc);

};


//############################################################################
/* here we check whether an iteration is green, yellow or red. Also according
   to this information we decide whether lambda should be changed */
class VOL_swing {
private:
   VOL_swing(const VOL_swing&);
   VOL_swing& operator=(const VOL_swing&);
public:
   enum condition {green, yellow, red} lastswing;
   int lastgreeniter, lastyellowiter, lastrediter;
   int ngs, nrs, nys;
   int rd;
   
   VOL_swing() {
      lastgreeniter = lastyellowiter = lastrediter = 0;
      ngs = nrs = nys = 0;
   }
   ~VOL_swing(){}

   inline void cond(const VOL_dual& dual, 
		    const double lcost, const double ascent, const int iter) {
      double eps = 1.e-3;

      if (ascent > 0.0  &&  lcost > dual.lcost + eps) {
	 lastswing = green;
	 lastgreeniter = iter;
	 ++ngs;
	 rd = 0;
      } else { 
	 if (ascent <= 0  &&  lcost > dual.lcost) {
	    lastswing = yellow;
	    lastyellowiter = iter;
	    ++nys;
	    rd = 0;
	 } else {
	    lastswing = red;
	    lastrediter = iter;
	    ++nrs;
	    rd = 1;
	 }
      }
   }

   inline double
   lfactor(const VOL_parms& parm, const double lambda, const int iter) {
      double lambdafactor = 1.0;
      double eps = 5.e-4;
      int cons;

      switch (lastswing) {
       case green:
	 cons = iter - VolMax(lastyellowiter, lastrediter);
	 if (parm.printflag & 4)
	    printf("      G: Consecutive Gs = %3d\n\n", cons);
	 if (cons >= parm.greentestinvl && lambda < 2.0) {
	    lastgreeniter = lastyellowiter = lastrediter = iter;
	    lambdafactor = 2.0;
	    if (parm.printflag & 2)
	       printf("\n ---- increasing lamda to %g ----\n\n",
		      lambda * lambdafactor); 
	 }
	 break;
      
       case yellow:
	 cons = iter - VolMax(lastgreeniter, lastrediter);
	 if (parm.printflag & 4)
	    printf("      Y: Consecutive Ys = %3d\n\n", cons);
	 if (cons >= parm.yellowtestinvl) {
	    lastgreeniter = lastyellowiter = lastrediter = iter;
	    lambdafactor = 1.1;
	    if (parm.printflag & 2)
	       printf("\n **** increasing lamda to %g *****\n\n",
		      lambda * lambdafactor);
	 }
	 break;
      
       case red:
	 cons = iter - VolMax(lastgreeniter, lastyellowiter);
	 if (parm.printflag & 4)
	    printf("      R: Consecutive Rs = %3d\n\n", cons);
	 if (cons >= parm.redtestinvl && lambda > eps) {
	    lastgreeniter = lastyellowiter = lastrediter = iter;
	    lambdafactor = 0.67;
	    if (parm.printflag & 2)
	       printf("\n **** decreasing lamda to %g *****\n\n",
		      lambda * lambdafactor);
	 } 
	 break;
      }
      return lambdafactor;
   }

   inline void
   print() {
      printf("**** G= %i, Y= %i, R= %i ****\n", ngs, nys, nrs);
      ngs = nrs = nys = 0;  
   }
};

//############################################################################
/* alpha should be decreased if after some number of iterations the objective
   has increased less that 1% */
class VOL_alpha_factor {
private:
   VOL_alpha_factor(const VOL_alpha_factor&);
   VOL_alpha_factor& operator=(const VOL_alpha_factor&);
public:
   double lastvalue;

   VOL_alpha_factor() {lastvalue = -COIN_DBL_MAX;}
   ~VOL_alpha_factor() {}

   inline double factor(const VOL_parms& parm, const double lcost,
			const double alpha) {
      if (alpha < parm.alphamin)
	 return 1.0;
      register const double ll = VolAbs(lcost);
      const double x = ll > 10 ? (lcost-lastvalue)/ll : (lcost-lastvalue);
      lastvalue = lcost;
      return (x <= 0.01) ? parm.alphafactor : 1.0;
   }
};

//############################################################################
/* here we compute the norm of the conjugate direction -hh-, the norm of the
   subgradient -norm-, the inner product between the subgradient and the 
   last conjugate direction -vh-, and the inner product between the new
   conjugate direction and the subgradient */
class VOL_vh {
private:
   VOL_vh(const VOL_vh&);
   VOL_vh& operator=(const VOL_vh&);
public:
   double hh;
   double norm;
   double vh;
   double asc;

   VOL_vh(const double alpha,
	  const VOL_dvector& dual_lb, const VOL_dvector& dual_ub,
	  const VOL_dvector& v, const VOL_dvector& vstar,
	  const VOL_dvector& u);
   ~VOL_vh(){}
};

//############################################################################
/* here we compute different parameter to be printed. v2 is the square of 
   the norm of the subgradient. vu is the inner product between the dual
   variables and the subgradient. vabs is the maximum absolute value of
   the violations of pstar. asc is the inner product between the conjugate
   direction and the subgradient */
class VOL_indc {
private:
   VOL_indc(const VOL_indc&);
   VOL_indc& operator=(const VOL_indc&);
public:
   double v2;
   double vu;
   double vabs;
   double asc;

public:
   VOL_indc(const VOL_dvector& dual_lb, const VOL_dvector& dual_ub,
	    const VOL_primal& primal, const VOL_primal& pstar,
	    const VOL_dual& dual);
   ~VOL_indc() {}
};

//#############################################################################

/** The user hooks should be overridden by the user to provide the
    problem specific routines for the volume algorithm. The user
    should derive a class ... 

    for all hooks: return value of -1 means that volume should quit
*/
class VOL_user_hooks {
public:
   virtual ~VOL_user_hooks() {}
public:
   // for all hooks: return value of -1 means that volume should quit
   /** compute reduced costs    
       @param u (IN) the dual variables
       @param rc (OUT) the reduced cost with respect to the dual values
   */
   virtual int compute_rc(const VOL_dvector& u, VOL_dvector& rc) = 0;

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
				double& pcost) = 0;
   /** Starting from the primal vector x, run a heuristic to produce
       an integer solution  
       @param x (IN) the primal vector
       @param heur_val (OUT) the value of the integer solution (return 
       <code>COIN_DBL_MAX</code> here if no feas sol was found
   */
   virtual int heuristics(const VOL_problem& p, 
			  const VOL_dvector& x, double& heur_val) = 0;
};

//#############################################################################

/** This class holds every data for the Volume Algorithm and its 
    <code>solve</code> method must be invoked to solve the problem.

    The INPUT fields must be filled out completely before <code>solve</code> 
    is invoked. <code>dsol</code> have to be filled out if and only if the 
    last argument to <code>solve</code> is <code>true</code>.
*/

class VOL_problem {
private:
   VOL_problem(const VOL_problem&);
   VOL_problem& operator=(const VOL_problem&);
   void set_default_parm();
   // ############ INPUT fields ########################
public: 
   /**@name Constructors and destructor */
   //@{
   /** Default constructor. */
   VOL_problem();
   /** Create a a <code>VOL_problem</code> object and read in the parameters
       from <code>filename</code>. */
   VOL_problem(const char *filename);
   /** Destruct the object. */
   ~VOL_problem();
   //@}

   /**@name Method to solve the problem. */
   //@{
   /** Solve the problem using the <code>hooks</code>. Any information needed 
       in the hooks must be stored in the structure <code>user_data</code> 
       points to. */
   int solve(VOL_user_hooks& hooks, const bool use_preset_dual = false);
   //@}

private: 
   /**@name Internal data (may be inquired for) */
   //@{
   /** value of alpha */
   double alpha_; 
   /** value of lambda */
   double lambda_;
   // This union is here for padding (so that data members would be
   // double-aligned on x86 CPU
   union {
      /** iteration number */
      int iter_;
      double __pad0;
   };
   //@}

public:
  
   /**@name External data (containing the result after solve) */
   //@{
   /** final lagrangian value (OUTPUT) */
   double value;
   /** final dual solution (INPUT/OUTPUT) */
   VOL_dvector dsol;
   /** final primal solution (OUTPUT) */
   VOL_dvector psol;
   /** violations (b-Ax) for the relaxed constraints */
   VOL_dvector viol;
   //@}

   /**@name External data (may be changed by the user before calling solve) */
   //@{
   /** The parameters controlling the Volume Algorithm (INPUT) */
   VOL_parms parm;
   /** length of primal solution (INPUT) */
   int psize;        
   /** length of dual solution (INPUT) */
   int dsize;      
   /** lower bounds for the duals (if 0 length, then filled with -inf) (INPUT)
    */
   VOL_dvector dual_lb;
   /** upper bounds for the duals (if 0 length, then filled with +inf) (INPUT)
    */
   VOL_dvector dual_ub;
   //@}

public:
   /**@name Methods returning final data */
   //@{
   /** returns the iteration number */
   int    iter() const { return iter_; }
   /** returns the value of alpha */
   double alpha() const { return alpha_; }
   /** returns the value of lambda */
   double lambda() const { return lambda_; }
   //@}

private:
   /**@name Private methods used internally */
   //@{
   /** Read in the parameters from the file <code>filename</code>. */
   void read_params(const char* filename);

   /** initializes duals, bounds for the duals, alpha, lambda */
   int initialize(const bool use_preset_dual);

   /** print volume info every parm.printinvl iterations */
   void print_info(const int iter,
		   const VOL_primal& primal, const VOL_primal& pstar,
		   const VOL_dual& dual);

   /** Checks if lcost is close to the target, if so it increases the target.
       Close means that we got within 5% of the target. */
   double readjust_target(const double oldtarget, const double lcost) const;

   /** Here we decide the value of alpha1 to be used in the convex
       combination. The new pstar will be computed as <br>
       pstar = alpha1 * pstar + (1 - alpha1) * primal <br>
       More details of this are in doc.ps. <br>
       IN:  alpha, primal, pstar, dual <br>
       @return alpha1
   */
   double power_heur(const VOL_primal& primal, const VOL_primal& pstar,
		     const VOL_dual& dual) const;
   //@}
};

#endif
