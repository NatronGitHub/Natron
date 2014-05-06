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


#ifndef ROTOUNDOCOMMAND_H
#define ROTOUNDOCOMMAND_H

#include <list>
#include <QUndoCommand>
#include <boost/shared_ptr.hpp>

#include "Global/Macros.h"

class BezierCP;
class MoveControlPointsUndoCommand : public QUndoCommand
{
public:
    
    MoveControlPointsUndoCommand();
    
    virtual ~MoveControlPointsUndoCommand();
    
    virtual void undo() OVERRIDE FINAL;
    
    virtual void redo() OVERRIDE FINAL;
    
    virtual int id() const OVERRIDE FINAL;
    
    virtual bool mergeWith(const QUndoCommand *other) OVERRIDE FINAL;
    
private:
    
    std::list< std::pair<boost::shared_ptr<BezierCP> ,boost::shared_ptr<BezierCP> > > selectedPoints;
};

#endif // ROTOUNDOCOMMAND_H
