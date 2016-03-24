/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef ClpPrimalColumnDantzig_H
#define ClpPrimalColumnDantzig_H

#include "ClpPrimalColumnPivot.hpp"

//#############################################################################

/** Primal Column Pivot Dantzig Algorithm Class

This is simplest choice - choose largest infeasibility

*/

class ClpPrimalColumnDantzig : public ClpPrimalColumnPivot {

public:

     ///@name Algorithmic methods
     //@{

     /** Returns pivot column, -1 if none.
         Lumbers over all columns - slow
         The Packed CoinIndexedVector updates has cost updates - for normal LP
         that is just +-weight where a feasibility changed.  It also has
         reduced cost from last iteration in pivot row
         Can just do full price if you really want to be slow
     */
     virtual int pivotColumn(CoinIndexedVector * updates,
                             CoinIndexedVector * spareRow1,
                             CoinIndexedVector * spareRow2,
                             CoinIndexedVector * spareColumn1,
                             CoinIndexedVector * spareColumn2);

     /// Just sets model
     virtual void saveWeights(ClpSimplex * model, int) {
          model_ = model;
     }
     //@}


     ///@name Constructors and destructors
     //@{
     /// Default Constructor
     ClpPrimalColumnDantzig();

     /// Copy constructor
     ClpPrimalColumnDantzig(const ClpPrimalColumnDantzig &);

     /// Assignment operator
     ClpPrimalColumnDantzig & operator=(const ClpPrimalColumnDantzig& rhs);

     /// Destructor
     virtual ~ClpPrimalColumnDantzig ();

     /// Clone
     virtual ClpPrimalColumnPivot * clone(bool copyData = true) const;

     //@}

     //---------------------------------------------------------------------------

private:
     ///@name Private member data
     //@}
};

#endif
