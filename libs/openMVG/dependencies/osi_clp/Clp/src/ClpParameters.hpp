/* $Id$ */
// Copyright (C) 2000, 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef _ClpParameters_H
#define _ClpParameters_H

/** This is where to put any useful stuff.

*/
enum ClpIntParam {
     /** The maximum number of iterations Clp can execute in the simplex methods
      */
     ClpMaxNumIteration = 0,
     /** The maximum number of iterations Clp can execute in hotstart before
         terminating */
     ClpMaxNumIterationHotStart,
     /** The name discipline; specifies how the solver will handle row and
         column names.
       - 0: Auto names: Names cannot be set by the client. Names of the form
        Rnnnnnnn or Cnnnnnnn are generated on demand when a name for a
        specific row or column is requested; nnnnnnn is derived from the row
        or column index. Requests for a vector of names return a vector with
        zero entries.
       - 1: Lazy names: Names supplied by the client are retained. Names of the
        form Rnnnnnnn or Cnnnnnnn are generated on demand if no name has been
        supplied by the client. Requests for a vector of names return a
        vector sized to the largest index of a name supplied by the client;
        some entries in the vector may be null strings.
       - 2: Full names: Names supplied by the client are retained. Names of the
        form Rnnnnnnn or Cnnnnnnn are generated on demand if no name has been
        supplied by the client. Requests for a vector of names return a
        vector sized to match the constraint system, and all entries will
        contain either the name specified by the client or a generated name.
     */
     ClpNameDiscipline,
     /** Just a marker, so that we can allocate a static sized array to store
         parameters. */
     ClpLastIntParam
};

enum ClpDblParam {
     /** Set Dual objective limit. This is to be used as a termination criteria
         in methods where the dual objective monotonically changes (dual
         simplex). */
     ClpDualObjectiveLimit,
     /** Primal objective limit. This is to be used as a termination
         criteria in methods where the primal objective monotonically changes
         (e.g., primal simplex) */
     ClpPrimalObjectiveLimit,
     /** The maximum amount the dual constraints can be violated and still be
         considered feasible. */
     ClpDualTolerance,
     /** The maximum amount the primal constraints can be violated and still be
         considered feasible. */
     ClpPrimalTolerance,
     /** Objective function constant. This the value of the constant term in
         the objective function. */
     ClpObjOffset,
     /// Maximum time in seconds - after, this action is as max iterations
     ClpMaxSeconds,
     /// Maximum wallclock running time in seconds - after, this action is as max iterations
     ClpMaxWallSeconds,
     /// Tolerance to use in presolve
     ClpPresolveTolerance,
     /** Just a marker, so that we can allocate a static sized array to store
         parameters. */
     ClpLastDblParam
};


enum ClpStrParam {
     /** Name of the problem. This is the found on the Name card of
         an mps file. */
     ClpProbName = 0,
     /** Just a marker, so that we can allocate a static sized array to store
         parameters. */
     ClpLastStrParam
};

/// Copy (I don't like complexity of Coin version)
template <class T> inline void
ClpDisjointCopyN( const T * array, const int size, T * newArray)
{
     memcpy(reinterpret_cast<void *> (newArray), array, size * sizeof(T));
}
/// And set
template <class T> inline void
ClpFillN( T * array, const int size, T value)
{
     int i;
     for (i = 0; i < size; i++)
          array[i] = value;
}
/// This returns a non const array filled with input from scalar or actual array
template <class T> inline T*
ClpCopyOfArray( const T * array, const int size, T value)
{
     T * arrayNew = new T[size];
     if (array)
          ClpDisjointCopyN(array, size, arrayNew);
     else
          ClpFillN ( arrayNew, size, value);
     return arrayNew;
}

/// This returns a non const array filled with actual array (or NULL)
template <class T> inline T*
ClpCopyOfArray( const T * array, const int size)
{
     if (array) {
          T * arrayNew = new T[size];
          ClpDisjointCopyN(array, size, arrayNew);
          return arrayNew;
     } else {
          return NULL;
     }
}
/// For a structure to be used by trusted code
typedef struct {
     int typeStruct; // allocated as 1,2 etc
     int typeCall;
     void * data;
} ClpTrustedData;
#endif
