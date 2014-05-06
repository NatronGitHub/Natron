//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/


#include "RotoUndoCommand.h"
#include "Global/GlobalDefines.h"
#include "Engine/RotoContext.h"

MoveControlPointsUndoCommand::MoveControlPointsUndoCommand()
: QUndoCommand()
{
    
}

MoveControlPointsUndoCommand::~MoveControlPointsUndoCommand()
{
    
}

void MoveControlPointsUndoCommand::undo()
{
    
}

void MoveControlPointsUndoCommand::redo()
{
    
}

int MoveControlPointsUndoCommand::id() const
{
    return kRotoMoveControlPointsCompressionID;
}

bool MoveControlPointsUndoCommand::mergeWith(const QUndoCommand *other)
{
    const MoveControlPointsUndoCommand* mvCmd = dynamic_cast<const MoveControlPointsUndoCommand*>(other);
    if (!mvCmd) {
        return false;
    }
    
    
}