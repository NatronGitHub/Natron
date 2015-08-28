/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef CURVEEDITORUNDOREDO_H
#define CURVEEDITORUNDOREDO_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>
#include <vector>
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QUndoCommand> // in QtGui on Qt4, in QtWidgets on Qt5
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif
#include "Engine/Curve.h"

namespace Transform
{
    struct Matrix3x3;
}
class CurveGui;
class KnobGui;
class Curve;
class CurveWidget;
class NodeCurveEditorElement;

struct SelectedKey
{
    boost::shared_ptr<CurveGui> curve;
    KeyFrame key;
    std::pair<double,double> leftTan, rightTan;

    SelectedKey()
        : curve(), key()
    {
    }

    SelectedKey(const boost::shared_ptr<CurveGui>& c,
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

    typedef std::map< boost::shared_ptr<CurveGui>, std::vector<KeyFrame> > KeysToAddList;

    AddKeysCommand(CurveWidget *editor,
                   const KeysToAddList & keys,
                   QUndoCommand *parent = 0);

    AddKeysCommand(CurveWidget *editor,
                   const boost::shared_ptr<CurveGui>& curve,
                   const std::vector<KeyFrame> & keys,
                   QUndoCommand *parent = 0);

    virtual ~AddKeysCommand() OVERRIDE
    {
    }
    

protected:
    
    void addOrRemoveKeyframe(bool isSetKeyCommand, bool add);

    
    virtual void undo() OVERRIDE;
    virtual void redo() OVERRIDE;


private:
    KeysToAddList _keys;
    CurveWidget *_curveWidget;
};



class SetKeysCommand
: public AddKeysCommand
{
public:
    
    
    SetKeysCommand(CurveWidget *editor,
                   const AddKeysCommand::KeysToAddList & keys,
                   QUndoCommand *parent = 0);
    
    SetKeysCommand(CurveWidget *editor,
                   const boost::shared_ptr<CurveGui>& curve,
                   const std::vector<KeyFrame> & keys,
                   QUndoCommand *parent = 0);
    
    virtual ~SetKeysCommand() OVERRIDE
    {
    }
    
private:
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    
private:
    boost::shared_ptr<CurveGui> _guiCurve;
    boost::shared_ptr<Curve> _oldCurve;
};

//////////////////////////////REMOVE  MULTIPLE KEYS COMMAND//////////////////////////////////////////////

class RemoveKeysCommand
    : public QUndoCommand
{
public:
    RemoveKeysCommand(CurveWidget* editor,
                      const std::map<boost::shared_ptr<CurveGui> ,std::vector<KeyFrame> > & curveEditorElement
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
    std::map<boost::shared_ptr<CurveGui> ,std::vector<KeyFrame> > _keys;
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
