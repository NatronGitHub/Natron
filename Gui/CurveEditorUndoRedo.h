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


#include <QtGui/QUndoCommand>


#include "Engine/Curve.h"

class CurveGui;
class KnobGui;
class CurveWidget;
class NodeCurveEditorElement;


//////////////////////////////ADD KEY COMMAND//////////////////////////////////////////////
class AddKeyCommand : public QUndoCommand {
public:

    AddKeyCommand(CurveWidget *editor, NodeCurveEditorElement *curveEditorElement, const std::string& actionName,
                  const KeyFrame& key, QUndoCommand *parent = 0);

    virtual void undo();
    virtual void redo();

private:

    std::string _actionName;
    KeyFrame _key;
    NodeCurveEditorElement* _element;
    CurveWidget *_curveWidget;
};

//////////////////////////////ADD MULTIPLE KEYS COMMAND//////////////////////////////////////////////

class PasteKeysCommand : public QUndoCommand {
public:

    PasteKeysCommand(CurveWidget *editor,NodeCurveEditorElement* element, const std::vector<KeyFrame> &keys, QUndoCommand *parent = 0);

    virtual ~PasteKeysCommand() { _keys.clear() ;}
    virtual void undo();
    virtual void redo();
private:

    std::string _actionName;
    NodeCurveEditorElement* _element;
    std::vector<KeyFrame> _keys;
    CurveWidget *_curveWidget;
};


//////////////////////////////REMOVE KEY COMMAND//////////////////////////////////////////////

class RemoveKeyCommand : public QUndoCommand{
public:

    RemoveKeyCommand(CurveWidget* editor,NodeCurveEditorElement* curveEditorElement
                     ,const KeyFrame& key,QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();

private:

    NodeCurveEditorElement* _element;
    KeyFrame _key;
    CurveWidget* _curveWidget;
};


//////////////////////////////REMOVE  MULTIPLE KEYS COMMAND//////////////////////////////////////////////

class RemoveMultipleKeysCommand : public QUndoCommand{
public:
    RemoveMultipleKeysCommand(CurveWidget* editor,const std::vector< std::pair<NodeCurveEditorElement*,KeyFrame > >& curveEditorElement
                              ,QUndoCommand *parent = 0);
    virtual ~RemoveMultipleKeysCommand() { _keys.clear(); }
    virtual void undo();
    virtual void redo();

private:

    std::vector<std::pair<NodeCurveEditorElement*,KeyFrame > > _keys;
    CurveWidget* _curveWidget;
};

//////////////////////////////MOVE KEY COMMAND//////////////////////////////////////////////
struct KeyMove{
    CurveGui* curve;
    KnobGui* knob;
    KeyFrame _old,_new;
};


class MoveKeyCommand : public QUndoCommand{

public:

    MoveKeyCommand(CurveWidget* editor,const KeyMove& move,QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();
    virtual int id() const ;
    virtual bool mergeWith(const QUndoCommand * command);

private:

    KeyMove _move;
    CurveWidget* _curveWidget;
};


//////////////////////////////MOVE MULTIPLE KEYS COMMAND//////////////////////////////////////////////

class MoveMultipleKeysCommand : public QUndoCommand{



public:

    MoveMultipleKeysCommand(CurveWidget* editor,const std::vector< KeyMove >& keys,QUndoCommand *parent = 0);
    virtual ~MoveMultipleKeysCommand(){ _keys.clear(); }
    virtual void undo();
    virtual void redo();
    virtual int id() const ;
    virtual bool mergeWith(const QUndoCommand * command);

private:

    std::vector< KeyMove > _keys;
    CurveWidget* _curveWidget;
};


//////////////////////////////SET KEY INTERPOLATION COMMAND//////////////////////////////////////////////

struct KeyInterpolationChange{
    Natron::KeyframeType oldInterp;
    Natron::KeyframeType newInterp;
    CurveGui* curve;
    KeyFrame key;
    KnobGui* knob;
};

class SetKeyInterpolationCommand : public QUndoCommand{

public:

    SetKeyInterpolationCommand(CurveWidget* editor,
                               const KeyInterpolationChange& change,
                               QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();

private:

    KeyInterpolationChange _change;
    CurveWidget* _curveWidget;
};



//////////////////////////////SET MULTIPLE KEYS INTERPOLATION COMMAND//////////////////////////////////////////////

class SetMultipleKeysInterpolationCommand : public QUndoCommand{

public:

    SetMultipleKeysInterpolationCommand(CurveWidget* editor,const std::vector< KeyInterpolationChange >& keys,QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();

private:

    std::vector< KeyInterpolationChange > _oldInterp;
    CurveWidget* _curveWidget;
};

#endif // CURVEEDITORUNDOREDO_H
