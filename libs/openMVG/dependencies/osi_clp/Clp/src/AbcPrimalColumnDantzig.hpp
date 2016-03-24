/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others, Copyright (C) 2012, FasterCoin.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef AbcPrimalColumnDantzig_H
#define AbcPrimalColumnDantzig_H

#include "AbcPrimalColumnPivot.hpp"

//#############################################################################

/** Primal Column Pivot Dantzig Algorithm Class

This is simplest choice - choose largest infeasibility

*/

class AbcPrimalColumnDantzig : public AbcPrimalColumnPivot {

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
     virtual int pivotColumn(CoinPartitionedVector * updates,
                             CoinPartitionedVector * spareRow2,
                             CoinPartitionedVector * spareColumn1);

     /// Just sets model
     virtual void saveWeights(AbcSimplex * model, int) {
          model_ = model;
     }
     //@}


     ///@name Constructors and destructors
     //@{
     /// Default Constructor
     AbcPrimalColumnDantzig();

     /// Copy constructor
     AbcPrimalColumnDantzig(const AbcPrimalColumnDantzig &);

     /// Assignment operator
     AbcPrimalColumnDantzig & operator=(const AbcPrimalColumnDantzig& rhs);

     /// Destructor
     virtual ~AbcPrimalColumnDantzig ();

     /// Clone
     virtual AbcPrimalColumnPivot * clone(bool copyData = true) const;

     //@}

     //---------------------------------------------------------------------------

private:
     ///@name Private member data
     //@}
};

#endif
