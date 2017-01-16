/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#ifndef ENGINE_KNOBPRIVATE_H
#define ENGINE_KNOBPRIVATE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Knob.h"

#include <algorithm> // min, max
#include <cassert>
#include <stdexcept>

#include <QtCore/QDataStream>
#include <QtCore/QDateTime>
#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QDebug>

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/algorithm/string/predicate.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include "Global/GlobalDefines.h"

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Curve.h"
#include "Engine/Cache.h"
#include "Engine/CacheEntryBase.h"
#include "Engine/CacheEntryKeyBase.h"
#include "Engine/DockablePanelI.h"
#include "Engine/Hash64.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/StringAnimationManager.h"
#include "Engine/Settings.h"
#include "Engine/TLSHolder.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/TrackMarker.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#include "Serialization/CurveSerialization.h"
#include "Serialization/KnobSerialization.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

#define EXPR_RECURSION_LEVEL() ExprRecursionLevel_RAII __recursionLevelIncrementer__(this)

typedef std::map<ViewIdx, KnobDimViewBasePtr> PerViewKnobDataMap;
typedef std::vector<PerViewKnobDataMap> PerDimensionKnobDataMap;

typedef std::map<ViewIdx, KnobI::Expr> ExprPerViewMap;
typedef std::vector<ExprPerViewMap> ExprPerDimensionVec;

struct RedirectionLink
{
    KnobDimViewBasePtr savedData;
};
typedef std::map<ViewIdx, RedirectionLink> PerViewSavedDataMap;
typedef std::vector<PerViewSavedDataMap> PerDimensionSavedDataVec;


typedef std::map<ViewIdx, bool> PerViewHasModificationMap;
typedef std::vector<PerViewHasModificationMap> PerDimensionModificationMap;

typedef std::map<ViewIdx, bool> PerViewEnabledMap;
typedef std::vector<PerViewEnabledMap> PerDimensionEnabledMap;

typedef std::map<ViewIdx, bool> PerViewAllDimensionsVisible;


struct KnobHelperPrivate
{
    // Ptr to the public class, can not be a smart ptr
    KnobHelper* publicInterface;

    // The holder containing this knob. This may be null if the knob is not in a collection
    KnobHolderWPtr holder;

    KnobFrameViewHashingStrategyEnum cacheInvalidationStrategy;

    // Protects the label
    mutable QMutex labelMutex;

    // The text label that will be displayed  on the GUI
    std::string label;

    // An icon to replace the label (one when checked, one when unchecked, for toggable buttons)
    std::string iconFilePath[2];

    // The script-name of the knob as available to python
    std::string name;

    // The original name passed to setName() by the user. The name might be different to comply to Python
    std::string originalName;

    // Should we add a new line after this parameter in the settings panel
    bool newLine;

    // Should we add a horizontal separator after this parameter
    bool addSeparator;

    // How much spacing in pixels should we add after this parameter. Only relevant if newLine is false
    int itemSpacing;

    // The spacing in pixels after this knob in the Viewer UI
    int inViewerContextItemSpacing;

    // The layout type in the viewer UI
    ViewerContextLayoutTypeEnum inViewerContextLayoutType;

    // The label in the viewer UI
    std::string inViewerContextLabel;

    // The icon in the viewer UI
    std::string inViewerContextIconFilePath[2];

    // Should this knob be available in the ShortCut editor by default?
    bool inViewerContextHasShortcut;

    // This is a list of script-names of knob shortcuts one can reference in the tooltip help.
    // See ViewerNode.cpp for an example.
    std::list<std::string> additionalShortcutsInTooltip;

    // A weak ptr to the parent knob containing this one. Each knob should be at least in a KnobPage
    // except the KnobPage itself.
    KnobIWPtr parentKnob;

    // Protects IsSecret, defaultIsSecret, enabled, inViewerContextSecret, defaultEnabled, evaluateOnChange
    mutable QMutex stateMutex;

    // Tells whether the knob is secret
    bool IsSecret;

    // Tells whether the knob is secret in the viewer. By default it is always visible in the viewer (if it has a viewer UI)
    bool inViewerContextSecret;

    // Is this parameter enabled
    bool enabled;

    // True if this knob can use the undo/redo stack
    bool CanUndo;

    // If true, a value change will never trigger an evaluation (render)
    bool evaluateOnChange;

    // If false this knob is not serialized into the project
    bool IsPersistent;

    // The hint tooltip displayed when hovering the mouse on the parameter
    std::string tooltipHint;

    // True if the hint contains markdown encoded data
    bool hintIsMarkdown;

    // True if this knob can receive animation curves
    bool isAnimationEnabled;

    // The number of dimensions in this knob (e.g: an RGBA KnobColor is 4-dimensional)
    int dimension;

    // For each view, a boolean indicating whether all dimensions are controled at once
    // protected by stateMutex
    PerViewAllDimensionsVisible allDimensionsVisible;

    // When true, autoFoldDimensions can be called to check if dimensions can be folded or not.
    bool autoFoldEnabled;

    // When true, autoAdjustFoldExpandDimensions can be called to automatically fold or expand dimensions
    bool autoAdjustFoldExpandEnabled;

    // protects perDimViewData and perDimViewSavedData
    mutable QMutex perDimViewDataMutex;
    
    // For each dimension and view the value stuff
    PerDimensionKnobDataMap perDimViewData;

    // When a dimension/view is linked to another knob, we save it so it can be restored
    // further on
    PerDimensionSavedDataVec perDimViewSavedData;

    // Was the knob declared by a plug-in or added by Natron?
    bool declaredByPlugin;

    // True if it was created by the user and should be put into the "User" page
    bool userKnob;

    // Pointer to the ofx param overlay interact for ofx parameter which have a custom interact
    // This is only supported OpenFX-wise
    OfxParamOverlayInteractPtr customInteract;

    // Pointer to the knobGui interface if it has any
    KnobGuiIWPtr gui;

    // A blind handle to the ofx param, needed for custom OpenFX interpolation
    void* ofxParamHandle;

    // Protects expressions
    mutable QMutex expressionMutex;

    // For each dimension its expression
    ExprPerDimensionVec expressions;

    // Used to prevent expressions from creating infinite loops
    // It doesn't have to be thread-local even if getValue can be called on multiple threads:
    // the evaluation of expressions is locking-out all other threads anyway, so really a single
    // thread is using this variable at a time anyway.
    int expressionRecursionLevel;

    // Protects expressionRecursionLevel
    mutable QMutex expressionRecursionLevelMutex;

    // For each dimension, the label displayed on the interface (e.g: "R" "G" "B" "A")
    std::vector<std::string> dimensionNames;

    // Protects lastRandomHash
    mutable QMutex lastRandomHashMutex;

    // The last return value of random to preserve its state
    mutable U32 lastRandomHash;

    // Protects hasModifications
    mutable QMutex hasModificationsMutex;

    // For each dimension tells whether the knob is considered to have modification or not
    mutable PerDimensionModificationMap hasModifications;

    // Protects valueChangedBlocked
    mutable QMutex valueChangedBlockedMutex;

    // Recursive counter to prevent calls to knobChanged callback
    int valueChangedBlocked;

    // Recursive counter to prevent autokeying in setValue
    int autoKeyingDisabled;

    // If true, when this knob change, it is required to refresh the meta-data on a Node
    bool isClipPreferenceSlave;

    // When enabled the keyframes can be displayed on the timeline if the knob is visible
    // protected by stateMutex
    bool keyframeTrackingEnabled;

    KnobHelperPrivate(KnobHelper* publicInterface_,
                      const KnobHolderPtr& holder_,
                      int nDims,
                      const std::string & label_,
                      bool declaredByPlugin_)
    : publicInterface(publicInterface_)
    , holder(holder_)
    , cacheInvalidationStrategy(eKnobHashingStrategyValue)
    , labelMutex()
    , label(label_)
    , iconFilePath()
    , name( label_.c_str() )
    , originalName( label_.c_str() )
    , newLine(true)
    , addSeparator(false)
    , itemSpacing(0)
    , inViewerContextItemSpacing(5)
    , inViewerContextLayoutType(eViewerContextLayoutTypeSpacing)
    , inViewerContextLabel()
    , inViewerContextIconFilePath()
    , inViewerContextHasShortcut(false)
    , parentKnob()
    , stateMutex()
    , IsSecret(false)
    , inViewerContextSecret(false)
    , enabled(true)
    , CanUndo(true)
    , evaluateOnChange(true)
    , IsPersistent(true)
    , tooltipHint()
    , hintIsMarkdown(false)
    , isAnimationEnabled(true)
    , dimension(nDims)
    , allDimensionsVisible()
    , autoFoldEnabled(true)
    , autoAdjustFoldExpandEnabled(true)
    , perDimViewDataMutex()
    , perDimViewData()
    , perDimViewSavedData()
    , declaredByPlugin(declaredByPlugin_)
    , userKnob(false)
    , customInteract()
    , gui()
    , ofxParamHandle(0)
    , expressionMutex()
    , expressions()
    , expressionRecursionLevel(0)
    , expressionRecursionLevelMutex(QMutex::Recursive)
    , dimensionNames()
    , lastRandomHash(0)
    , hasModificationsMutex()
    , hasModifications()
    , valueChangedBlockedMutex()
    , valueChangedBlocked(0)
    , autoKeyingDisabled(0)
    , isClipPreferenceSlave(false)
    , keyframeTrackingEnabled(true)
    {
        {
            KnobHolderPtr h = holder.lock();
            if ( h && !h->canKnobsAnimate() ) {
                isAnimationEnabled = false;
            }
        }

        dimensionNames.resize(dimension);
        hasModifications.resize(dimension);
        allDimensionsVisible[ViewIdx(0)] = true;
        perDimViewData.resize(dimension);
        expressions.resize(dimension);
        perDimViewSavedData.resize(dimension);
        for (int i = 0; i < nDims; ++i) {
            hasModifications[i][ViewIdx(0)] = false;
        }
    }

    void parseListenersFromExpression(DimIdx dimension, ViewIdx view);

    /**
     * @brief Returns a string to append to the expression script declaring all Python attributes referencing nodes, knobs etc...
     * that can be reached through the expression at the given dimension/view.
     * @param addTab, if true, the script should be indented by one tab
     **/
    std::string getReachablePythonAttributesForExpression(bool addTab, DimIdx dimension, ViewIdx view);


};


class KnobExpressionKey : public CacheEntryKeyBase
{
public:

    KnobExpressionKey(U64 nodeTimeInvariantHash,
                      int dimension,
                      TimeValue time,
                      ViewIdx view,
                      const std::string& knobScriptName)
    : _nodeTimeInvariantHash(nodeTimeInvariantHash)
    , _dimension(dimension)
    , _time(time)
    , _view(view)
    , _knobScriptName(knobScriptName)
    {

    }

    virtual ~KnobExpressionKey()
    {

    }

    virtual void copy(const CacheEntryKeyBase& other) OVERRIDE FINAL
    {
        CacheEntryKeyBase::copy(other);
        const KnobExpressionKey* o = dynamic_cast<const KnobExpressionKey*>(&other);
        if (!o) {
            return;
        }
        _nodeTimeInvariantHash = o->_nodeTimeInvariantHash;
        _dimension = o->_dimension;
        _time = o->_time;
        _view = o->_view;
        _knobScriptName = o->_knobScriptName;
    }

    virtual bool equals(const CacheEntryKeyBase& other) OVERRIDE FINAL
    {
        const KnobExpressionKey* o = dynamic_cast<const KnobExpressionKey*>(&other);
        if (!o) {
            return false;
        }
        if (_nodeTimeInvariantHash != o->_nodeTimeInvariantHash) {
            return false;
        }
        if (_dimension != o->_dimension) {
            return false;
        }
        if (_time != o->_time) {
            return false;
        }
        if (_view != o->_view) {
            return false;
        }
        if (_knobScriptName != o->_knobScriptName) {
            return false;
        }
        return true;
    }

    virtual int getUniqueID() const OVERRIDE FINAL
    {
        return kCacheKeyUniqueIDExpressionResult;
    }
    
private:



    virtual void appendToHash(Hash64* hash) const OVERRIDE FINAL
    {
        hash->append(_nodeTimeInvariantHash);
        hash->append(_dimension);
        hash->append((double)_time);
        hash->append((int)_view);
        Hash64::appendQString(QString::fromUtf8(_knobScriptName.c_str()), hash);
    }

    U64 _nodeTimeInvariantHash;
    int _dimension;
    TimeValue _time;
    ViewIdx _view;
    std::string _knobScriptName;
};


class KnobExpressionResult : public CacheEntryBase
{
    KnobExpressionResult();

public:

    static KnobExpressionResultPtr create(const KnobExpressionKeyPtr& key)
    {
        KnobExpressionResultPtr ret(new KnobExpressionResult());
        ret->setKey(key);
        return ret;
    }

    virtual ~KnobExpressionResult()
    {

    }

    // This is thread-safe and doesn't require a mutex:
    // The thread computing this entry and calling the setter is guaranteed
    // to be the only one interacting with this object. Then all objects
    // should call the getter.
    //
    void getResult(double* value, std::string* valueAsString) const
    {
        if (value) {
            *value = _valueResult;
        }
        if (valueAsString) {
            *valueAsString = _stringResult;
        }
    }

    void setResult(double value, const std::string& valueAsString)
    {
        _stringResult = valueAsString;
        _valueResult = value;
    }

    virtual std::size_t getSize() const OVERRIDE FINAL
    {
        return 0;
    }

private:
    
    std::string _stringResult;
    double _valueResult;

};


NATRON_NAMESPACE_EXIT

#endif // KNOBPRIVATE_H
