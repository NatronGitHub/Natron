/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpPdcoBase_H
#define ClpPdcoBase_H

#include "CoinPragma.hpp"

#include "CoinPackedMatrix.hpp"
#include "CoinDenseVector.hpp"
class ClpInterior;

/** Abstract base class for tailoring everything for Pcdo

    Since this class is abstract, no object of this type can be created.

    If a derived class provides all methods then all ClpPcdo algorithms
    should work.

    Eventually we should be able to use ClpObjective and ClpMatrixBase.
*/

class ClpPdcoBase  {

public:
     /**@name Virtual methods that the derived classes must provide */
     //@{
     virtual void matVecMult(ClpInterior * model, int mode, double * x, double * y) const = 0;

     virtual void getGrad(ClpInterior * model, CoinDenseVector<double> &x, CoinDenseVector<double> &grad) const = 0;

     virtual void getHessian(ClpInterior * model, CoinDenseVector<double> &x, CoinDenseVector<double> &H) const = 0;

     virtual double getObj(ClpInterior * model, CoinDenseVector<double> &x) const = 0;

     virtual void matPrecon(ClpInterior * model,  double delta, double * x, double * y) const = 0;

     //@}
     //@{
     ///@name Other
     /// Clone
     virtual ClpPdcoBase * clone() const = 0;
     /// Returns type
     inline int type() const {
          return type_;
     };
     /// Sets type
     inline void setType(int type) {
          type_ = type;
     };
     /// Returns size of d1
     inline int sizeD1() const {
          return 1;
     };
     /// Returns d1 as scalar
     inline double getD1() const {
          return d1_;
     };
     /// Returns size of d2
     inline int sizeD2() const {
          return 1;
     };
     /// Returns d2 as scalar
     inline double getD2() const {
          return d2_;
     };
     //@}


protected:

     /**@name Constructors, destructor<br>
        <strong>NOTE</strong>: All constructors are protected. There's no need
        to expose them, after all, this is an abstract class. */
     //@{
     /** Default constructor. */
     ClpPdcoBase();
     /** Destructor (has to be public) */
public:
     virtual ~ClpPdcoBase();
protected:
     // Copy
     ClpPdcoBase(const ClpPdcoBase&);
     // Assignment
     ClpPdcoBase& operator=(const ClpPdcoBase&);
     //@}


protected:
     /**@name Data members
        The data members are protected to allow access for derived classes. */
     //@{
     /// Should be dense vectors
     double d1_;
     double d2_;
     /// type (may be useful)
     int type_;
     //@}
};

#endif
