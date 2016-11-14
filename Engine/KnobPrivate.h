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


#ifndef ENGINE_KNOBPRIVATE_H
#define ENGINE_KNOBPRIVATE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <algorithm> // min, max
#include <cassert>
#include <stdexcept>

#include "Knob.h"

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

#define EXPR_RECURSION_LEVEL() KnobHelper::ExprRecursionLevel_RAII __recursionLevelIncrementer__(this)


NATRON_NAMESPACE_ENTER

///for each dimension, the dimension of the master this dimension is linked to, and a pointer to the master
typedef std::map<ViewIdx, MasterKnobLink> PerViewMasterLink;
typedef std::vector<PerViewMasterLink> PerDimensionMasterVec;

///a curve for each dimension
typedef std::map<ViewIdx, CurvePtr> PerViewCurveMap;
typedef std::vector<PerViewCurveMap> PerDimensionCurveVec;

typedef std::map<ViewIdx, AnimationLevelEnum> PerViewAnimLevelMap;
typedef std::vector<PerViewAnimLevelMap> PerDimensionAnimLevelVec;



typedef std::map<ViewIdx, KnobI::Expr> ExprPerViewMap;
typedef std::vector<ExprPerViewMap> ExprPerDimensionVec;

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

    // For each dimension tells whether the knob is enabled
    PerDimensionEnabledMap enabled;

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

    // When true, setAllDimensionsVisible will be called by the KnobGui according to the knob
    // values.
    bool autoFoldAllDimensionsEnabled;

    // Protect curves
    mutable QMutex curvesMutex;

    // For each dimension and view an animation curve
    PerDimensionCurveVec curves;

    // Read/Write lock protecting _masters & ignoreMasterPersistence & listeners
    mutable QReadWriteLock mastersMutex;

    // For each dimension and view, tells to which knob and the dimension in that knob it is slaved to
    PerDimensionMasterVec masters;

    // When true masters will not be serialized
    bool ignoreMasterPersistence;

    // Used when this knob is an alias of another knob. The other knob is set in "slaveForAlias"
    KnobIWPtr slaveForAlias;

    // This is a list of all the knobs that have expressions/links refering to this knob.
    // For each knob, a ListenerDim struct associated to each of its dimension informs as to the nature of the link (i.e: slave/master link or expression link)
    KnobI::ListenerDimsMap listeners;

    // Protects animationLevel
    mutable QMutex animationLevelMutex;

    // Indicates for each dimension whether it is static/interpolated/onkeyframe
    PerDimensionAnimLevelVec animationLevel;

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

    // For each dimension, the label displayed on the interface (e.g: "R" "G" "B" "A")
    std::vector<std::string> dimensionNames;

    // Protects expressions
    mutable QMutex expressionMutex;

    // For each dimension its expression
    ExprPerDimensionVec expressions;

    // Protects lastRandomHash
    mutable QMutex lastRandomHashMutex;

    // The last return value of random to preserve its state
    mutable U32 lastRandomHash;

    // TLS data for the knob
    boost::shared_ptr<TLSHolder<KnobHelper::KnobTLSData> > tlsData;

    // Protects hasModifications
    mutable QMutex hasModificationsMutex;

    // For each dimension tells whether the knob is considered to have modification or not
    mutable PerDimensionModificationMap hasModifications;

    // Protects valueChangedBlocked & listenersNotificationBlocked & guiRefreshBlocked
    mutable QMutex valueChangedBlockedMutex;

    // Recursive counter to prevent calls to knobChanged callback
    int valueChangedBlocked;

    // Recursive counter to prevent calls to knobChanged callback for listeners knob (i.e: knobs that refer to this one)
    int listenersNotificationBlocked;

    // Recursive counter to prevent autokeying in setValue
    int autoKeyingDisabled;

    // If true, when this knob change, it is required to refresh the meta-data on a Node
    bool isClipPreferenceSlave;

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
    , enabled()
    , CanUndo(true)
    , evaluateOnChange(true)
    , IsPersistent(true)
    , tooltipHint()
    , hintIsMarkdown(false)
    , isAnimationEnabled(true)
    , dimension(nDims)
    , allDimensionsVisible()
    , autoFoldAllDimensionsEnabled(true)
    , curves()
    , mastersMutex()
    , masters()
    , ignoreMasterPersistence(false)
    , slaveForAlias()
    , listeners()
    , animationLevelMutex()
    , animationLevel()
    , declaredByPlugin(declaredByPlugin_)
    , userKnob(false)
    , customInteract()
    , gui()
    , ofxParamHandle(0)
    , dimensionNames()
    , expressionMutex()
    , expressions()
    , lastRandomHash(0)
    , tlsData( new TLSHolder<KnobHelper::KnobTLSData>() )
    , hasModificationsMutex()
    , hasModifications()
    , valueChangedBlockedMutex()
    , valueChangedBlocked(0)
    , listenersNotificationBlocked(0)
    , autoKeyingDisabled(0)
    , isClipPreferenceSlave(false)
    {
        {
            KnobHolderPtr h = holder.lock();
            if ( h && !h->canKnobsAnimate() ) {
                isAnimationEnabled = false;
            }
        }

        enabled.resize(dimension);
        curves.resize(dimension);
        masters.resize(dimension);
        animationLevel.resize(dimension);
        expressions.resize(dimension);
        dimensionNames.resize(dimension);
        hasModifications.resize(dimension);
        allDimensionsVisible[ViewIdx(0)] = true;
        for (int i = 0; i < nDims; ++i) {
            enabled[i][ViewIdx(0)] = true;
            curves[i][ViewIdx(0)] = CurvePtr();
            masters[i][ViewIdx(0)] = MasterKnobLink();
            animationLevel[i][ViewIdx(0)] = eAnimationLevelNone;
            expressions[i][ViewIdx(0)] = KnobI::Expr();
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


NATRON_NAMESPACE_EXIT

#endif // KNOBPRIVATE_H
