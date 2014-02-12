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

#ifndef CURVEEDITORUNDOREDO_H
#define CURVEEDITORUNDOREDO_H

#include <vector>
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QUndoCommand> // in QtGui on Qt4, in QtWidgets on Qt5
CLANG_DIAG_ON(deprecated)

#include "Engine/Curve.h"

class CurveGui;
class KnobGui;
class CurveWidget;
class NodeCurveEditorElement;

//////////////////////////////ADD MULTIPLE KEYS COMMAND//////////////////////////////////////////////

class AddKeysCommand : public QUndoCommand {
public:

    AddKeysCommand(CurveWidget *editor,CurveGui* curve, const std::vector<KeyFrame> &keys, QUndoCommand *parent = 0);

    virtual ~AddKeysCommand() OVERRIDE {}

private:
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

    void addOrRemoveKeyframe(bool add);

private:
    CurveGui* _curve;
    std::vector<KeyFrame> _keys;
    CurveWidget *_curveWidget;
};



//////////////////////////////REMOVE  MULTIPLE KEYS COMMAND//////////////////////////////////////////////

class RemoveKeysCommand : public QUndoCommand{
public:
    RemoveKeysCommand(CurveWidget* editor,const std::vector< std::pair<CurveGui*,KeyFrame > >& curveEditorElement
                              ,QUndoCommand *parent = 0);
    virtual ~RemoveKeysCommand() OVERRIDE {}

private:
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

    void addOrRemoveKeyframe(bool add);
    
private:
    std::vector<std::pair<CurveGui*,KeyFrame > > _keys;
    CurveWidget* _curveWidget;
};

//////////////////////////////MOVE KEY COMMAND//////////////////////////////////////////////
struct KeyMove{
    CurveGui* curve;
    KeyFrame key;
    
    KeyMove(CurveGui* c,const KeyFrame& k)
    : curve(c)
    , key(k)
    {
        
    }
};


typedef std::vector< KeyMove > KeyMoveV;

class MoveKeysCommand : public QUndoCommand{



public:

    MoveKeysCommand(CurveWidget* editor,const KeyMoveV& keys,double dt,double dv,QUndoCommand *parent = 0);
    virtual ~MoveKeysCommand() OVERRIDE {}

private:
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual int id() const OVERRIDE FINAL;
    virtual bool mergeWith(const QUndoCommand * command) OVERRIDE FINAL;

    void move(double dt, double dv, bool isundo);

private:
    bool _merge;
    double _dt;
    double _dv;
    KeyMoveV _keys;
    CurveWidget* _curveWidget;
};


//////////////////////////////SET MULTIPLE KEYS INTERPOLATION COMMAND//////////////////////////////////////////////

struct KeyInterpolationChange{
    Natron::KeyframeType oldInterp;
    Natron::KeyframeType newInterp;
    CurveGui* curve;
    KeyFrame key;
    
    KeyInterpolationChange(Natron::KeyframeType oldType,Natron::KeyframeType newType,CurveGui* c,const KeyFrame& k)
    : oldInterp(oldType)
    , newInterp(newType)
    , curve(c)
    , key(k)
    {
        
    }
};



class SetKeysInterpolationCommand : public QUndoCommand{

public:

    SetKeysInterpolationCommand(CurveWidget* editor,const std::vector< KeyInterpolationChange >& keys,QUndoCommand *parent = 0);
    virtual ~SetKeysInterpolationCommand() OVERRIDE {}
private:
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;


    void setNewInterpolation(bool undo);
    
private:
    std::vector< KeyInterpolationChange > _oldInterp;
    CurveWidget* _curveWidget;
};

#endif // CURVEEDITORUNDOREDO_H
