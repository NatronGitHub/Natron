/* $Id: CoinMessage.hpp 1411 2011-03-30 11:40:33Z forrest $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinMessage_H
#define CoinMessage_H

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

/*! \file

    This file contains the enum for the standard set of Coin messages and a
    class definition whose sole purpose is to supply a constructor. The text
    ot the messages is defined in CoinMessage.cpp,

    CoinMessageHandler.hpp contains the generic facilities for message
    handling.
*/

#include "CoinMessageHandler.hpp"

/*! \brief Symbolic names for the standard set of COIN messages */

enum COIN_Message
{
  COIN_MPS_LINE=0,
  COIN_MPS_STATS,
  COIN_MPS_ILLEGAL,
  COIN_MPS_BADIMAGE,
  COIN_MPS_DUPOBJ,
  COIN_MPS_DUPROW,
  COIN_MPS_NOMATCHROW,
  COIN_MPS_NOMATCHCOL,
  COIN_MPS_FILE,
  COIN_MPS_BADFILE1,
  COIN_MPS_BADFILE2,
  COIN_MPS_EOF,
  COIN_MPS_RETURNING,
  COIN_MPS_CHANGED,
  COIN_SOLVER_MPS,
  COIN_PRESOLVE_COLINFEAS,
  COIN_PRESOLVE_ROWINFEAS,
  COIN_PRESOLVE_COLUMNBOUNDA,
  COIN_PRESOLVE_COLUMNBOUNDB,
  COIN_PRESOLVE_NONOPTIMAL,
  COIN_PRESOLVE_STATS,
  COIN_PRESOLVE_INFEAS,
  COIN_PRESOLVE_UNBOUND,
  COIN_PRESOLVE_INFEASUNBOUND,
  COIN_PRESOLVE_INTEGERMODS,
  COIN_PRESOLVE_POSTSOLVE,
  COIN_PRESOLVE_NEEDS_CLEANING,
  COIN_PRESOLVE_PASS,
# if PRESOLVE_DEBUG
  COIN_PRESOLDBG_FIRSTCHECK,
  COIN_PRESOLDBG_RCOSTACC,
  COIN_PRESOLDBG_RCOSTSTAT,
  COIN_PRESOLDBG_STATSB,
  COIN_PRESOLDBG_DUALSTAT,
# endif
  COIN_GENERAL_INFO,
  COIN_GENERAL_WARNING,
  COIN_DUMMY_END
};


/*! \class CoinMessage
    \brief The standard set of Coin messages

    This class provides convenient access to the standard set of Coin messages.
    In a nutshell, it's a CoinMessages object with a constructor that
    preloads the standard Coin messages.
*/

class CoinMessage : public CoinMessages {

public:

  /**@name Constructors etc */
  //@{
  /*! \brief Constructor
  
    Build a CoinMessages object and load it with the standard set of
    Coin messages.
  */
  CoinMessage(Language language=us_en);
  //@}

};

#endif
