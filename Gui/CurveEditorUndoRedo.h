/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "Global/Macros.h"

#include <list>
#include <vector>
#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QCoreApplication>
#include <QUndoCommand> // in QtGui on Qt4, in QtWidgets on Qt5
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/AnimatingObjectI.h"
#include "Engine/Curve.h"
#include "Engine/Transform.h"
#include "Engine/Variant.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;


/**
 * @class An undo command that can add or remove keyframes on multiple curves at once
 **/
class AddOrRemoveKeysCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(AddKeysCommand)

public:

    struct ObjectKeyFramesData
    {
        std::vector<std::list<VariantTimeValuePair> > keyframesPerDim;
        int dimensionStartIndex;

    };


    typedef std::map<AnimatingObjectIWPtr, ObjectKeyFramesData > ObjectKeysToAddMap;

    AddOrRemoveKeysCommand(const ObjectKeysToAddMap & keys,
                           bool initialCommandIsAdd,
                           QUndoCommand *parent)
    : QUndoCommand(parent)
    , _initialCommandIsAdd(initialCommandIsAdd)
    , _keys(keys)
    {
        setText( tr("Add keyframe(s)") );
    }

    virtual ~AddOrRemoveKeysCommand() OVERRIDE
    {
    }

protected:

    /**
     * @brief Add or remove keyframes.
     * @param clearAndSet If true, this will clear existing keyframes before adding them
     * @param add True if operation is add, false if it should remove
     **/
    void addOrRemoveKeyframe(bool add);


    virtual void undo() OVERRIDE;
    virtual void redo() OVERRIDE;

private:

    bool _initialCommandIsAdd;
    ObjectKeysToAddMap _keys;
};

/**
 * @class An undo command that can set all keyframes on multiple curves at once. Existing keyframes are deleted (and restored in undo)
 **/
class SetKeysCommand : public QUndoCommand
{

    Q_DECLARE_TR_FUNCTIONS(SetKeysCommand)

public:

    struct PerDimensionObjectKeyframes
    {
        std::list<VariantTimeValuePair> newKeys;
        CurvePtr oldCurveState;
    };

    struct SetObjectKeyFramesData
    {

        std::vector<PerDimensionObjectKeyframes> keyframesPerDim;
        int dimensionStartIndex;

    };


    typedef std::map<AnimatingObjectIWPtr, SetObjectKeyFramesData > ObjectKeysToSetMap;


    SetKeysCommand(const ObjectKeysToSetMap & keys,
                   QUndoCommand *parent = 0)
    : QUndoCommand(parent)
    , _isFirstRedo(true)
    , _keys(keys)
    {
        setText( tr("Set keyframe(s)") );
    }


    virtual ~SetKeysCommand() OVERRIDE
    {
    }

private:


    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    bool _isFirstRedo;
    ObjectKeysToSetMap _keys;
};


/**
 * @class An undo command that can warp keyframes of multiple curves
 **/
class WarpKeysCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(MoveKeysCommand)

public:

    struct PerDimensionObjectKeyframes
    {
        std::list<double> keysToMove;
    };

    struct WarpObjectKeyFramesData
    {
        std::vector<PerDimensionObjectKeyframes> keyframesPerDim;
        int dimensionStartIndex;
    };

    typedef std::map<AnimatingObjectIWPtr, WarpObjectKeyFramesData > ObjectKeysToMoveMap;

    WarpKeysCommand(const ObjectKeysToMoveMap& keys,
                    double dt,
                    double dv,
                    QUndoCommand *parent = 0);

    WarpKeysCommand(const ObjectKeysToMoveMap& keys,
                    const Transform::Matrix3x3& matrix,
                    QUndoCommand *parent = 0);
    
    virtual ~WarpKeysCommand() OVERRIDE
    {
    }

private:

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    virtual int id() const OVERRIDE FINAL;
    virtual bool mergeWith(const QUndoCommand * command) OVERRIDE FINAL;
    void warpKeys();

private:
    boost::scoped_ptr<Curve::KeyFrameWarp> _warp;
    ObjectKeysToMoveMap _keys;
};


//////////////////////////////SET MULTIPLE KEYS INTERPOLATION COMMAND//////////////////////////////////////////////

/**
 * @class An undo command that can set the interpolation type of multiple keyframes of multiple curves
 **/
class SetKeysInterpolationCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(SetKeysInterpolationCommand)

public:



    struct PerDimensionObjectKeyframes
    {
        std::list<double> keysToChange;
        KeyframeTypeEnum oldInterp, newInterp;
    };

    struct InterpolationObjectKeyFramesData
    {
        std::vector<PerDimensionObjectKeyframes> keyframesPerDim;
        int dimensionStartIndex;
    };

    typedef std::map<AnimatingObjectIWPtr, InterpolationObjectKeyFramesData> ObjectKeyFramesDataMap;

    SetKeysInterpolationCommand(const ObjectKeyFramesDataMap & keys,
                                QUndoCommand *parent)
    : QUndoCommand(parent)
    , _keys(keys)
    {
        setText( tr("Set keyframes interpolation") );

    }

    virtual ~SetKeysInterpolationCommand() OVERRIDE
    {
    }

private:
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;


    void setNewInterpolation(bool undo);

private:
   ObjectKeyFramesDataMap _keys;
};

/**
 * @class An undo command that can move derivatives of a control point of a curve
 **/
class MoveTangentCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(MoveTangentCommand)

public:

    enum SelectedTangentEnum
    {
        eSelectedTangentLeft = 0,
        eSelectedTangentRight
    };


    MoveTangentCommand(SelectedTangentEnum deriv,
                       const AnimatingObjectIPtr& object,
                       int dimension,
                       const KeyFrame& k,
                       double dx,
                       double dy,
                       QUndoCommand *parent = 0);

    MoveTangentCommand(SelectedTangentEnum deriv,
                       const AnimatingObjectIPtr& object,
                       int dimension,
                       const KeyFrame& k,
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

    AnimatingObjectIWPtr _object;
    int _dimension;
    double _keyTime;
    SelectedTangentEnum _deriv;
    KeyframeTypeEnum _oldInterp, _newInterp;
    double _oldLeft, _oldRight, _newLeft, _newRight;
    bool _setBoth;
    bool _firstRedoCalled;
};

NATRON_NAMESPACE_EXIT;

#endif // CURVEEDITORUNDOREDO_H
