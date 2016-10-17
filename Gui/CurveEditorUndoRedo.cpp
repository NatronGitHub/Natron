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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "CurveEditorUndoRedo.h"

#include <cmath>
#include <stdexcept>

#include <QtCore/QDebug>

#include "Global/GlobalDefines.h"

#include "Gui/CurveEditor.h"
#include "Gui/CurveGui.h"
#include "Gui/KnobGui.h"
#include "Gui/CurveWidget.h"

#include "Engine/Bezier.h"
#include "Engine/Knob.h"
#include "Engine/Curve.h"
#include "Engine/RotoContext.h"
#include "Engine/KnobTypes.h"
#include "Engine/Transform.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER;


template <typename T>
static void convertVariantTimeValuePairToTypedList(const std::list<VariantTimeValuePair>& inList,
                                                   std::list<TimeValuePair<T> >* outList)
{
    for (std::list<VariantTimeValuePair>::const_iterator it = inList.begin(); it!=inList.end(); ++it) {
        TimeValuePair<T> p(it->time, variantToType<T>(it->value));
        outList->push_back(p);
    }
}

void
AddOrRemoveKeysCommand::addOrRemoveKeyframe(bool add)
{
    for (ObjectKeysToAddMap::const_iterator it = _keys.begin(); it != _keys.end(); ++it) {

        AnimatingObjectIPtr obj = it->first.lock();
        if (!obj) {
            continue;
        }


        if (!add) {
            for (std::size_t i = 0; i < it->second.keyframesPerDim.size(); ++i) {
                std::list<double> keyTimes;
                for (std::list<VariantTimeValuePair> ::const_iterator it2 = it->second.keyframesPerDim[i].begin(); it2 != it->second.keyframesPerDim[i].end(); ++it2) {
                    keyTimes.push_back(it2->time);
                }
                obj->deleteValuesAtTime(eCurveChangeReasonInternal, keyTimes, ViewSpec::all(), i + it->second.dimensionStartIndex);
            }
        } else {

            AnimatingObjectI::KeyframeDataTypeEnum dataType = obj->getKeyFrameDataType();
            switch (dataType) {
                case AnimatingObjectI::eKeyframeDataTypeNone:
                case AnimatingObjectI::eKeyframeDataTypeDouble:
                {
                    std::vector<std::list<DoubleTimeValuePair> > keyframes(it->second.keyframesPerDim.size());
                    for (std::size_t i = 0; i < it->second.keyframesPerDim.size(); ++i) {
                        convertVariantTimeValuePairToTypedList<double>(it->second.keyframesPerDim[i], &keyframes[i]);
                    }
                    obj->setMultipleDoubleValueAtTimeAcrossDimensions(keyframes, it->second.dimensionStartIndex, ViewSpec::all(), eValueChangedReasonUserEdited);
                }   break;
                case AnimatingObjectI::eKeyframeDataTypeBool:
                {
                    std::vector<std::list<BoolTimeValuePair> > keyframes(it->second.keyframesPerDim.size());
                    for (std::size_t i = 0; i < it->second.keyframesPerDim.size(); ++i) {
                        convertVariantTimeValuePairToTypedList<bool>(it->second.keyframesPerDim[i], &keyframes[i]);
                    }
                    obj->setMultipleBoolValueAtTimeAcrossDimensions(keyframes, it->second.dimensionStartIndex, ViewSpec::all(), eValueChangedReasonUserEdited);

                }   break;
                case AnimatingObjectI::eKeyframeDataTypeString:
                {
                    std::vector<std::list<StringTimeValuePair> > keyframes(it->second.keyframesPerDim.size());
                    for (std::size_t i = 0; i < it->second.keyframesPerDim.size(); ++i) {
                        convertVariantTimeValuePairToTypedList<std::string>(it->second.keyframesPerDim[i], &keyframes[i]);
                    }
                    obj->setMultipleStringValueAtTimeAcrossDimensions(keyframes, it->second.dimensionStartIndex, ViewSpec::all(), eValueChangedReasonUserEdited);

                }   break;
                case AnimatingObjectI::eKeyframeDataTypeInt:
                {
                    std::vector<std::list<IntTimeValuePair> > keyframes(it->second.keyframesPerDim.size());
                    for (std::size_t i = 0; i < it->second.keyframesPerDim.size(); ++i) {
                        convertVariantTimeValuePairToTypedList<int>(it->second.keyframesPerDim[i], &keyframes[i]);
                    }
                    obj->setMultipleIntValueAtTimeAcrossDimensions(keyframes, it->second.dimensionStartIndex, ViewSpec::all(), eValueChangedReasonUserEdited);
                    
                }   break;
            } // end switch
        } // !add
    } // for all objects

} // addOrRemoveKeyframe


void
AddOrRemoveKeysCommand::undo()
{
    addOrRemoveKeyframe(!_initialCommandIsAdd);
}

void
AddOrRemoveKeysCommand::redo()
{

    addOrRemoveKeyframe(_initialCommandIsAdd);
}


void
SetKeysCommand::undo()
{
    for (ObjectKeysToSetMap::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        AnimatingObjectIPtr obj = it->first.lock();
        if (!obj) {
            continue;
        }

        std::vector<int> dims(it->second.keyframesPerDim.size());
        for (std::size_t i = 0; i < it->second.keyframesPerDim.size(); ++i) {
            dims[i] = i + it->second.dimensionStartIndex;
        }

        for (std::size_t i = 0; i < dims.size(); ++i) {
            CurvePtr curve = obj->getAnimationCurve(ViewGetSpec(0), dims[i]);
            if (!curve) {
                continue;
            }

            // Clone the old curve state
            obj->cloneCurve(ViewSpec::all(), dims[i], *it->second.keyframesPerDim[i].oldCurveState, 0 /*offset*/, 0 /*range*/, 0 /*stringAnimation*/, eCurveChangeReasonInternal);
        }
    }
}

void
SetKeysCommand::redo()
{
    for (ObjectKeysToSetMap::iterator it = _keys.begin(); it != _keys.end(); ++it) {
        AnimatingObjectIPtr obj = it->first.lock();
        if (!obj) {
            continue;
        }

        std::vector<int> dims(it->second.keyframesPerDim.size());
        for (std::size_t i = 0; i < it->second.keyframesPerDim.size(); ++i) {
            dims[i] = i + it->second.dimensionStartIndex;
        }

        // The first time clone previous animation
        if (_isFirstRedo) {
            for (std::size_t i = 0; i < dims.size(); ++i) {
                CurvePtr curve = obj->ViewGetSpec(ViewIdx(0), dims[i]);
                if (!curve) {
                    continue;
                }
                it->second.keyframesPerDim[i].oldCurveState.reset(new Curve);
                it->second.keyframesPerDim[i].oldCurveState->clone(*curve);
            }
        }

        // Remove all existing animation
        obj->removeAnimationAcrossDimensions(ViewSpec::all(), dims, eCurveChangeReasonInternal);

        // Add new keyframes
        AnimatingObjectI::KeyframeDataTypeEnum dataType = obj->getKeyFrameDataType();
        switch (dataType) {
            case AnimatingObjectI::eKeyframeDataTypeNone:
            case AnimatingObjectI::eKeyframeDataTypeDouble:
            {
                std::vector<std::list<DoubleTimeValuePair> > keyframes(it->second.keyframesPerDim.size());
                for (std::size_t i = 0; i < it->second.keyframesPerDim.size(); ++i) {
                    convertVariantTimeValuePairToTypedList<double>(it->second.keyframesPerDim[i].newKeys, &keyframes[i]);
                }
                obj->setMultipleDoubleValueAtTimeAcrossDimensions(keyframes, it->second.dimensionStartIndex, ViewSpec::all(), eValueChangedReasonUserEdited);
            }   break;
            case AnimatingObjectI::eKeyframeDataTypeBool:
            {
                std::vector<std::list<BoolTimeValuePair> > keyframes(it->second.keyframesPerDim.size());
                for (std::size_t i = 0; i < it->second.keyframesPerDim.size(); ++i) {
                    convertVariantTimeValuePairToTypedList<bool>(it->second.keyframesPerDim[i].newKeys, &keyframes[i]);
                }
                obj->setMultipleBoolValueAtTimeAcrossDimensions(keyframes, it->second.dimensionStartIndex, ViewSpec::all(), eValueChangedReasonUserEdited);

            }   break;
            case AnimatingObjectI::eKeyframeDataTypeString:
            {
                std::vector<std::list<StringTimeValuePair> > keyframes(it->second.keyframesPerDim.size());
                for (std::size_t i = 0; i < it->second.keyframesPerDim.size(); ++i) {
                    convertVariantTimeValuePairToTypedList<std::string>(it->second.keyframesPerDim[i].newKeys, &keyframes[i]);
                }
                obj->setMultipleStringValueAtTimeAcrossDimensions(keyframes, it->second.dimensionStartIndex, ViewSpec::all(), eValueChangedReasonUserEdited);

            }   break;
            case AnimatingObjectI::eKeyframeDataTypeInt:
            {
                std::vector<std::list<IntTimeValuePair> > keyframes(it->second.keyframesPerDim.size());
                for (std::size_t i = 0; i < it->second.keyframesPerDim.size(); ++i) {
                    convertVariantTimeValuePairToTypedList<int>(it->second.keyframesPerDim[i].newKeys, &keyframes[i]);
                }
                obj->setMultipleIntValueAtTimeAcrossDimensions(keyframes, it->second.dimensionStartIndex, ViewSpec::all(), eValueChangedReasonUserEdited);


            }   break;
        } // end switch

    } // for all objects
} // redo



//////////////////////////////MOVE MULTIPLE KEYS COMMAND//////////////////////////////////////////////
WarpKeysCommand::WarpKeysCommand(const ObjectKeysToMoveMap &keys,
                                 double dt,
                                 double dv,
                                 QUndoCommand *parent )
: QUndoCommand(parent)
, _keys(keys)
{
    _warp.reset(new Curve::TranslationKeyFrameWarp(dt, dv));
    setText( tr("Move keyframes") );
}

WarpKeysCommand::WarpKeysCommand(const ObjectKeysToMoveMap& keys,
                const Transform::Matrix3x3& matrix,
                QUndoCommand *parent)
: QUndoCommand(parent)
, _keys(keys)
{
    _warp.reset(new Curve::AffineKeyFrameWarp(matrix));
    setText( tr("Transform keyframes") );
}

void
WarpKeysCommand::warpKeys()
{
    for (ObjectKeysToMoveMap::const_iterator it = _keys.begin(); it!=_keys.end();++it) {
        AnimatingObjectIPtr obj = it->first.lock();
        if (!obj) {
            continue;
        }
        for (std::size_t i = 0; i < it->second.keyframesPerDim.size(); ++i) {
            obj->warpValuesAtTime(eCurveChangeReasonInternal, it->second.keyframesPerDim[i].keysToMove, ViewSpec::all(), it->second.dimensionStartIndex + i, *_warp, _warp->allowReplacingExistingKeyframes());
        }

    } // for all objects
}

void
WarpKeysCommand::undo()
{
    _warp->setWarpInverted(true);
    warpKeys();
}

void
WarpKeysCommand::redo()
{
    _warp->setWarpInverted(false);
    warpKeys();
}

bool
WarpKeysCommand::mergeWith(const QUndoCommand * command)
{
    const WarpKeysCommand* cmd = dynamic_cast<const WarpKeysCommand*>(command);
    if (!cmd || cmd->id() != id()) {
        return false;
    }
    if ( cmd->_keys.size() != _keys.size() ) {
        return false;
    }

    ObjectKeysToMoveMap::const_iterator itother = cmd->_keys.begin();
    for (ObjectKeysToMoveMap::const_iterator it = _keys.begin(); it != _keys.end(); ++it, ++itother) {
        if (itother->first.lock() != it->first.lock()) {
            return false;
        }

        if ( itother->second.keyframesPerDim.size() != it->second.keyframesPerDim.size() ) {
            return false;
        }

        for (std::size_t i = 0; i < it->second.keyframesPerDim.size(); ++i) {
            if (it->second.keyframesPerDim[i].keysToMove.size() != itother->second.keyframesPerDim[i].keysToMove.size()) {
                return false;
            }
            std::list<double>::const_iterator it2 = it->second.keyframesPerDim[i].keysToMove.begin();
            std::list<double>::const_iterator itother2 = itother->second.keyframesPerDim[i].keysToMove.begin();
            for (; it2 != it->second.keyframesPerDim[i].keysToMove.end(); ++it2, ++itother2) {
                if (*it2 != *itother2) {
                    return false;
                }
            }
        }
    }

    return _warp->mergeWith(*cmd->_warp);
}

int
WarpKeysCommand::id() const
{
    return kCurveEditorMoveMultipleKeysCommandCompressionID;
}


void
SetKeysInterpolationCommand::setNewInterpolation(bool undo)
{
    for (ObjectKeyFramesDataMap::const_iterator it = _keys.begin(); it!=_keys.end(); ++it) {
        AnimatingObjectIPtr obj = it->first.lock();
        if (!obj) {
            continue;
        }
        for (std::size_t i = 0; i < it->second.keyframesPerDim.size(); ++i) {

            KeyframeTypeEnum type = undo ? it->second.keyframesPerDim[i].oldInterp : it->second.keyframesPerDim[i].newInterp;
            obj->setInterpolationAtTimes(eCurveChangeReasonInternal, ViewSpec::all(), it->second.dimensionStartIndex + i, it->second.keyframesPerDim[i].keysToChange, type);
        }
    } // for all objects
} // SetKeysInterpolationCommand::setNewInterpolation

void
SetKeysInterpolationCommand::undo()
{
    setNewInterpolation(true);
}

void
SetKeysInterpolationCommand::redo()
{
    setNewInterpolation(false);
}


MoveTangentCommand::MoveTangentCommand(SelectedTangentEnum deriv,
                                       const AnimatingObjectIPtr& object,
                                       int dimension,
                                       const KeyFrame& k,
                                       double dx,
                                       double dy,
                                       QUndoCommand *parent)
: QUndoCommand(parent)
, _object(object)
, _dimension(dimension)
, _keyTime(k.getTime())
, _deriv(deriv)
, _oldInterp( k.getInterpolation() )
, _oldLeft( k.getLeftDerivative() )
, _oldRight( k.getRightDerivative() )
, _setBoth(false)
, _firstRedoCalled(false)
{
    CurvePtr curve = object->getAnimationCurve(ViewGetSpec(0), dimension);
    assert(curve);

    KeyFrameSet keys = curve->getKeyFrames_mt_safe();
    KeyFrameSet::const_iterator cur = keys.find(k);

    assert( cur != keys.end() );

    //find next and previous keyframes
    KeyFrameSet::const_iterator prev = cur;
    if ( prev != keys.begin() ) {
        --prev;
    } else {
        prev = keys.end();
    }
    KeyFrameSet::const_iterator next = cur;
    if ( next != keys.end() ) {
        ++next;
    }

    // handle first and last keyframe correctly:
    // - if their interpolation was eKeyframeTypeCatmullRom or eKeyframeTypeCubic, then it becomes eKeyframeTypeFree
    // - in all other cases it becomes eKeyframeTypeBroken
    KeyframeTypeEnum interp = k.getInterpolation();
    bool keyframeIsFirstOrLast = ( prev == keys.end() || next == keys.end() );
    bool interpIsNotBroken = (interp != eKeyframeTypeBroken);
    bool interpIsCatmullRomOrCubicOrFree = (interp == eKeyframeTypeCatmullRom ||
                                            interp == eKeyframeTypeCubic ||
                                            interp == eKeyframeTypeFree);
    _setBoth = keyframeIsFirstOrLast ? interpIsCatmullRomOrCubicOrFree  || internalCurve->isCurvePeriodic() : interpIsNotBroken;

    bool isLeft;
    if (deriv == eSelectedTangentLeft) {
        //if dx is not of the good sign it would make the curve uncontrollable
        if (dx <= 0) {
            dx = 0.0001;
        }
        isLeft = true;
    } else {
        //if dx is not of the good sign it would make the curve uncontrollable
        if (dx >= 0) {
            dx = -0.0001;
        }
        isLeft = false;
    }
    double derivative = dy / dx;

    if (_setBoth) {
        _newInterp = eKeyframeTypeFree;
        _newLeft = derivative;
        _newRight = derivative;
    } else {
        if (isLeft) {
            _newLeft = derivative;
            _newRight = _oldRight;
        } else {
            _newLeft = _oldLeft;
            _newRight = derivative;
        }
        _newInterp = eKeyframeTypeBroken;
    }

    setText( tr("Move keyframe slope") );

}

MoveTangentCommand::MoveTangentCommand(SelectedTangentEnum deriv,
                                       const AnimatingObjectIPtr& object,
                                       int dimension,
                                       const KeyFrame& k,
                                       double derivative,
                                       QUndoCommand *parent)
    : QUndoCommand(parent)
    , _object(object)
    , _dimension(dimension)
    , _keyTime(k.getTime())
    , _deriv(deriv)
    , _oldInterp( k.getInterpolation() )
    , _oldLeft( k.getLeftDerivative() )
    , _oldRight( k.getRightDerivative() )
    , _setBoth(true)
    , _firstRedoCalled(false)
{
    _newInterp = _oldInterp == eKeyframeTypeBroken ? eKeyframeTypeBroken : eKeyframeTypeFree;
    _setBoth = _newInterp == eKeyframeTypeFree;

    switch (deriv) {
    case eSelectedTangentLeft:
        _newLeft = derivative;
        if (_newInterp == eKeyframeTypeBroken) {
            _newRight = _oldRight;
        } else {
            _newRight = derivative;
        }
        break;
    case eSelectedTangentRight:
        _newRight = derivative;
        if (_newInterp == eKeyframeTypeBroken) {
            _newLeft = _oldLeft;
        } else {
            _newLeft = derivative;
        }
    default:
        break;
    }
    setText( tr("Move keyframe slope") );

}

void
MoveTangentCommand::setNewDerivatives(bool undo)
{
    AnimatingObjectIPtr obj = _object.lock();
    if (!obj) {
        return;
    }

    double left = undo ? _oldLeft : _newLeft;
    double right = undo ? _oldRight : _newRight;
    KeyframeTypeEnum interp = undo ? _oldInterp : _newInterp;
    if (_setBoth) {
        obj->setLeftAndRightDerivativesAtTime(eCurveChangeReasonInternal, ViewSpec::all(), _dimension, _keyTime, left, right);
    } else {
        bool isLeft = _deriv == eSelectedTangentLeft;
        obj->setDerivativeAtTime(eCurveChangeReasonInternal, ViewSpec::all(), _dimension, _keyTime, isLeft ? left : right, isLeft);
    }
    obj->setInterpolationAtTime(eCurveChangeReasonInternal, ViewSpec::all(), _dimension, _keyTime, interp);

}

void
MoveTangentCommand::undo()
{
    setNewDerivatives(true);
}

void
MoveTangentCommand::redo()
{
    setNewDerivatives(false);
    _firstRedoCalled = true;
}

int
MoveTangentCommand::id() const
{
    return kCurveEditorMoveTangentsCommandCompressionID;
}

bool
MoveTangentCommand::mergeWith(const QUndoCommand * command)
{
    const MoveTangentCommand* cmd = dynamic_cast<const MoveTangentCommand*>(command);
    if (!cmd || cmd->id() == id()) {
        return false;
    }

    if (cmd->_object.lock() != _object.lock() || cmd->_dimension != _dimension || cmd->_keyTime != _keyTime) {
        return false;
    }


    _newInterp = cmd->_newInterp;
    _newLeft = cmd->_newLeft;
    _newRight = cmd->_newRight;

    return true;

}

NATRON_NAMESPACE_EXIT;
