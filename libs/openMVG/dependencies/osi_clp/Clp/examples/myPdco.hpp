/* $Id$ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef myPdco_H
#define myPdco_H


#include "CoinPragma.hpp"

#include "ClpPdcoBase.hpp"

/** This implements a simple network matrix as derived from ClpMatrixBase.

If you want more sophisticated version then you could inherit from this.
Also you might want to allow networks with gain */

class myPdco : public ClpPdcoBase {

public:
     /**@name Useful methods */
     //@{
     virtual void matVecMult(ClpInterior * model, int mode, double * x, double * y) const;

     virtual void getGrad(ClpInterior * model, CoinDenseVector<double> &x, CoinDenseVector<double> &grad) const;

     virtual void getHessian(ClpInterior * model, CoinDenseVector<double> &x, CoinDenseVector<double> &H) const;

     virtual double getObj(ClpInterior * model, CoinDenseVector<double> &x) const;

     virtual void matPrecon(ClpInterior * model,  double delta, double * x, double * y) const ;
     //@}


     /**@name Constructors, destructor */
     //@{
     /** Default constructor. */
     myPdco();
     /** Constructor from Stuff */
     myPdco(double d1, double d2,
            int numnodes, int numlinks);
     /// Also reads a model
     myPdco(ClpInterior & model, FILE * fpData, FILE * fpParam);
     /** Destructor */
     virtual ~myPdco();
     //@}

     /**@name Copy method */
     //@{
     /** The copy constructor. */
     myPdco(const myPdco&);

     myPdco& operator=(const myPdco&);
     /// Clone
     virtual ClpPdcoBase * clone() const ;
     //@}


protected:
     /**@name Data members
        The data members are protected to allow access for derived classes. */
     //@{
     int * rowIndex_;
     int numlinks_;
     int numnodes_;

     //@}
};

#endif
