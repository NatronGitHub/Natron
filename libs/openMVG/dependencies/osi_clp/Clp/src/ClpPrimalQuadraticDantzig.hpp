/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpPrimalQuadraticDantzig_H
#define ClpPrimalQuadraticDantzig_H

#include "ClpPrimalColumnPivot.hpp"
class ClpSimplexPrimalQuadratic;
class ClpQuadraticInfo;
//#############################################################################

/** Primal Column Pivot Dantzig Algorithm Class

This is simplest choice - choose largest infeasibility

*/

class ClpPrimalQuadraticDantzig : public ClpPrimalColumnPivot {

public:

     ///@name Algorithmic methods
     //@{

     /** Returns pivot column, -1 if none.
         Lumbers over all columns - slow
         updateArray has cost updates (also use pivotRow_ from last iteration)
         Can just do full price if you really want to be slow
     */
     virtual int pivotColumn(CoinIndexedVector * updates,
                             CoinIndexedVector * spareRow1,
                             CoinIndexedVector * spareRow2,
                             CoinIndexedVector * spareColumn1,
                             CoinIndexedVector * spareColumn2);

     /// Just sets model
     virtual void saveWeights(ClpSimplex * model, int mode) {
          model_ = model;
     }
     //@}


     ///@name Constructors and destructors
     //@{
     /// Default Constructor
     ClpPrimalQuadraticDantzig();

     /// Copy constructor
     ClpPrimalQuadraticDantzig(const ClpPrimalQuadraticDantzig &);

     /// Constructor from model
     ClpPrimalQuadraticDantzig(ClpSimplexPrimalQuadratic * model,
                               ClpQuadraticInfo * info);

     /// Assignment operator
     ClpPrimalQuadraticDantzig & operator=(const ClpPrimalQuadraticDantzig& rhs);

     /// Destructor
     virtual ~ClpPrimalQuadraticDantzig ();

     /// Clone
     virtual ClpPrimalColumnPivot * clone(bool copyData = true) const;

     //@}

     //---------------------------------------------------------------------------

private:
     ///@name Private member data
     /// Pointer to info
     ClpQuadraticInfo * quadraticInfo_;
     //@}
};

#endif
