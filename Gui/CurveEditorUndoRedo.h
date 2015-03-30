//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef CURVEEDITORUNDOREDO_H
#define CURVEEDITORUNDOREDO_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <list>
#include <vector>
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QUndoCommand> // in QtGui on Qt4, in QtWidgets on Qt5
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Curve.h"

namespace Transform
{
    struct Matrix3x3;
}
class CurveGui;
class KnobGui;
class CurveWidget;
class NodeCurveEditorElement;

struct SelectedKey
{
    CurveGui* curve;
    KeyFrame key;
    std::pair<double,double> leftTan, rightTan;

    SelectedKey()
        : curve(NULL), key()
    {
    }

    SelectedKey(CurveGui* c,
                const KeyFrame & k)
        : curve(c)
          , key(k)
    {
    }

    SelectedKey(const SelectedKey & o)
        : curve(o.curve)
          , key(o.key)
          , leftTan(o.leftTan)
          , rightTan(o.rightTan)
    {
    }
};

struct SelectedKey_compare_time
{
    bool operator() (const SelectedKey & lhs,
                     const SelectedKey & rhs) const
    {
        return lhs.key.getTime() < rhs.key.getTime();
    }
};


typedef boost::shared_ptr<SelectedKey> KeyPtr;
typedef std::list< KeyPtr > SelectedKeys;


//////////////////////////////ADD MULTIPLE KEYS COMMAND//////////////////////////////////////////////

class AddKeysCommand
    : public QUndoCommand
{
public:

    struct KeysForCurve
    {
        CurveGui* curve;
        std::vector<KeyFrame> keys;
    };

    typedef std::list< boost::shared_ptr<KeysForCurve> > KeysToAddList;

    AddKeysCommand(CurveWidget *editor,
                   const KeysToAddList & keys,
                   QUndoCommand *parent = 0);

    AddKeysCommand(CurveWidget *editor,
                   CurveGui* curve,
                   const std::vector<KeyFrame> & keys,
                   QUndoCommand *parent = 0);

    virtual ~AddKeysCommand() OVERRIDE
    {
    }

private:
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

    void addOrRemoveKeyframe(bool add);

private:
    KeysToAddList _keys;
    CurveWidget *_curveWidget;
};


//////////////////////////////REMOVE  MULTIPLE KEYS COMMAND//////////////////////////////////////////////

class RemoveKeysCommand
    : public QUndoCommand
{
public:
    RemoveKeysCommand(CurveWidget* editor,
                      const std::vector< std::pair<CurveGui*,KeyFrame > > & curveEditorElement
                      ,
                      QUndoCommand *parent = 0);
    virtual ~RemoveKeysCommand() OVERRIDE
    {
    }

private:
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

    void addOrRemoveKeyframe(bool add);

private:
    std::vector<std::pair<CurveGui*,KeyFrame > > _keys;
    CurveWidget* _curveWidget;
};

//////////////////////////////MOVE KEY COMMAND//////////////////////////////////////////////


class MoveKeysCommand
    : public QUndoCommand
{
public:

    MoveKeysCommand(CurveWidget* widget,
                    const SelectedKeys & keys,
                    double dt,
                    double dv,
                    bool updateOnFirstRedo,
                    QUndoCommand *parent = 0);
    virtual ~MoveKeysCommand() OVERRIDE
    {
    }

private:
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual int id() const OVERRIDE FINAL;
    virtual bool mergeWith(const QUndoCommand * command) OVERRIDE FINAL;

    void move(double dt, double dv);

private:
    bool _firstRedoCalled;
    bool _updateOnFirstRedo;
    double _dt;
    double _dv;
    SelectedKeys _keys;
    CurveWidget* _widget;
};


//////////////////////////////SET MULTIPLE KEYS INTERPOLATION COMMAND//////////////////////////////////////////////

struct KeyInterpolationChange
{
    Natron::KeyframeTypeEnum oldInterp;
    Natron::KeyframeTypeEnum newInterp;
    KeyPtr key;


    KeyInterpolationChange(Natron::KeyframeTypeEnum oldType,
                           Natron::KeyframeTypeEnum newType,
                           const KeyPtr & k)
        : oldInterp(oldType)
          , newInterp(newType)
          , key(k)
    {
    }
};


class SetKeysInterpolationCommand
    : public QUndoCommand
{
public:

    SetKeysInterpolationCommand(CurveWidget* widget,
                                const std::list< KeyInterpolationChange > & keys,
                                QUndoCommand *parent = 0);
    virtual ~SetKeysInterpolationCommand() OVERRIDE
    {
    }

private:
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;


    void setNewInterpolation(bool undo);

private:
    std::list< KeyInterpolationChange > _keys;
    CurveWidget* _widget;
};

class MoveTangentCommand
: public QUndoCommand
{
public:
    
    enum SelectedTangentEnum
    {
        eSelectedTangentLeft = 0,
        eSelectedTangentRight
    };

    
    MoveTangentCommand(CurveWidget* widget,
                       SelectedTangentEnum deriv,
                       const KeyPtr& key,
                       double dx,double dy,
                       bool updateOnFirstRedo,
                       QUndoCommand *parent = 0);
    
    MoveTangentCommand(CurveWidget* widget,
                       SelectedTangentEnum deriv,
                       const KeyPtr& key,
                       double derivative,
                       QUndoCommand *parent = 0);
    
    virtual ~MoveTangentCommand() OVERRIDE
    {
    }
    
private:
    
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual int id() const OVERRIDE FINAL;
    virtual bool mergeWith(const QUndoCommand * command) OVERRIDE FINAL;
    
    void setNewDerivatives(bool undo);
    
private:
    
    CurveWidget* _widget;
    KeyPtr _key;
    SelectedTangentEnum _deriv;
    Natron::KeyframeTypeEnum _oldInterp,_newInterp;
    double _oldLeft,_oldRight,_newLeft,_newRight;
    bool _setBoth;
    bool _updateOnFirstRedo;
    bool _firstRedoCalled;
};

class TransformKeysCommand
: public QUndoCommand
{
public:
    
    TransformKeysCommand(CurveWidget* widget,
                         const SelectedKeys & keys,
                         double centerX,
                         double centerY,
                         double tx,
                         double ty,
                         double sx,
                         double sy,
                         bool updateOnFirstRedo,
                         QUndoCommand *parent = 0);
    virtual ~TransformKeysCommand();
    
private:
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual int id() const OVERRIDE FINAL;
    virtual bool mergeWith(const QUndoCommand * command) OVERRIDE FINAL;
    
    void transform(const KeyPtr& k);
    
private:
    bool _firstRedoCalled;
    bool _updateOnFirstRedo;
    SelectedKeys _keys;
    CurveWidget* _widget;
    std::list<std::pair<boost::shared_ptr<Curve>,boost::shared_ptr<Curve> > > _curves;
    boost::shared_ptr<Transform::Matrix3x3> _matrix;
};


#endif // CURVEEDITORUNDOREDO_H
