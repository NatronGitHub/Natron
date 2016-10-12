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

#include "Knob.h"
#include "KnobImpl.h"
#include "KnobGetValueImpl.h"
#include "KnobSetValueImpl.h"

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
#include "Engine/TrackerContext.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#include "Serialization/CurveSerialization.h"
#include "Serialization/KnobSerialization.h"

#include "Engine/EngineFwd.h"

SERIALIZATION_NAMESPACE_USING

NATRON_NAMESPACE_ENTER;

using std::make_pair; using std::pair;

KnobSignalSlotHandler::KnobSignalSlotHandler(const KnobIPtr& knob)
    : QObject()
    , k(knob)
{
}

void
KnobSignalSlotHandler::onMasterCurveAnimationChanged(const std::list<double>& /*keysAdded*/,
                                                     const std::list<double>& /*keysRemoved*/,
                                                     ViewIdx view,
                                                     int dimension,
                                                     CurveChangeReason reason)
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );
    if (!handler) {
        return;
    }
    KnobIPtr master = handler->getKnob();
    if (!master) {
        return;
    }
    CurvePtr masterCurve = master->getCurve(view, dimension);
    if (!masterCurve) {
        return;
    }

    StringAnimationManagerPtr stringAnim = master->getStringAnimation();

    getKnob()->cloneCurve(view, dimension, *masterCurve, 0 /*offset*/, 0 /*range*/, stringAnim.get(), reason);
}



bool
KnobI::slaveTo(int dimension,
               const KnobIPtr & other,
               int otherDimension,
               bool ignoreMasterPersistence)
{
    return slaveToInternal(dimension, other, otherDimension, eValueChangedReasonNatronInternalEdited, ignoreMasterPersistence);
}

void
KnobI::onKnobSlavedTo(int dimension,
                      const KnobIPtr &  other,
                      int otherDimension)
{
    slaveToInternal(dimension, other, otherDimension, eValueChangedReasonUserEdited, false);
}

void
KnobI::unSlave(int dimension,
               bool copyState)
{
    unSlaveInternal(dimension, eValueChangedReasonNatronInternalEdited, copyState);
}

void
KnobI::onKnobUnSlaved(int dimension)
{
    unSlaveInternal(dimension, eValueChangedReasonUserEdited, true);
}



KnobPagePtr
KnobI::getTopLevelPage() const
{
    KnobIPtr parentKnob = getParentKnob();
    KnobIPtr parentKnobTmp = parentKnob;

    while (parentKnobTmp) {
        KnobIPtr parent = parentKnobTmp->getParentKnob();
        if (!parent) {
            break;
        } else {
            parentKnobTmp = parent;
        }
    }

    ////find in which page the knob should be
    KnobPagePtr isTopLevelParentAPage = toKnobPage(parentKnobTmp);

    return isTopLevelParentAPage;
}

/***********************************KNOB HELPER******************************************/

///for each dimension, the dimension of the master this dimension is linked to, and a pointer to the master
typedef std::map<ViewIdx, MasterKnobLink> PerViewMasterLink;
typedef std::vector<PerViewMasterLink> PerDimensionMasterVec;

///a curve for each dimension
typedef std::map<ViewIdx, CurvePtr> PerViewCurveMap;
typedef std::vector<PerViewCurveMap> PerDimensionCurveVec;

typedef std::map<ViewIdx, AnimationLevelEnum> PerViewAnimLevelMap;
typedef std:vector<PerViewAnimLevelMap> PerDimensionAnimLevelVec;

struct Expr
{
    std::string expression; //< the one modified by Natron
    std::string originalExpression; //< the one input by the user
    std::string exprInvalid;
    bool hasRet;

    ///The list of pair<knob, dimension> dpendencies for an expression
    std::list< std::pair<KnobIWPtr, int> > dependencies;

    //PyObject* code;

    Expr()
        : expression(), originalExpression(), exprInvalid(), hasRet(false) /*, code(0)*/ {}
};


typedef std::map<ViewIdx, Expr> ExprPerViewMap;
typedef std::vector<ExprPerViewMap> ExprPerDimensionVec;

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
    std::vector<bool> enabled;

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

    // True if the knob was dynamically created by the user (either via python or via the gui)
    bool dynamicallyCreated;

    // True if it was created by the user and should be put into the "User" page
    bool userKnob;

    // Pointer to the ofx param overlay interact for ofx parameter which have a custom interact
    // This is only supported OpenFX-wise
    OfxParamOverlayInteractPtr customInteract;

    // Pointer to the knobGui interface if it has any
    KnobGuiIWPtr gui;

    // Protects mustCloneGuiCurves & mustCloneInternalCurves
    mutable QMutex mustCloneGuiCurvesMutex;

    // Set to true if gui curves were modified by the user instead of the real internal curves.
    // If true then when finished rendering, the knob should clone the guiCurves into the internal curves.
    std::vector<bool> mustCloneGuiCurves;

    // Set to true if the internal curves were modified and we should update the gui curves
    std::vector<bool> mustCloneInternalCurves;

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
    mutable std::vector<bool> hasModifications;

    // Protects valueChangedBlocked & listenersNotificationBlocked
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
                      int dimension_,
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
        , enabled(dimension_)
        , CanUndo(true)
        , evaluateOnChange(true)
        , IsPersistent(true)
        , tooltipHint()
        , hintIsMarkdown(false)
        , isAnimationEnabled(true)
        , dimension(dimension_)
        , curves(dimension_)
        , mastersMutex()
        , masters(dimension_)
        , ignoreMasterPersistence(false)
        , slaveForAlias()
        , listeners()
        , animationLevelMutex()
        , animationLevel(dimension_)
        , declaredByPlugin(declaredByPlugin_)
        , dynamicallyCreated(false)
        , userKnob(false)
        , customInteract()
        , gui()
        , mustCloneGuiCurvesMutex()
        , mustCloneGuiCurves()
        , mustCloneInternalCurves()
        , ofxParamHandle(0)
        , dimensionNames(dimension_)
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

        mustCloneGuiCurves.resize(dimension);
        mustCloneInternalCurves.resize(dimension);
        expressions.resize(dimension);
        hasModifications.resize(dimension);
        for (int i = 0; i < dimension_; ++i) {
            mustCloneGuiCurves[i] = false;
            mustCloneInternalCurves[i] = false;
            hasModifications[i] = false;
        }
    }

    void parseListenersFromExpression(int dimension);

    std::string declarePythonVariables(bool addTab, int dimension);

    bool shouldUseGuiCurve() const
    {
        if (!holder.lock()) {
            return false;
        }

        return !holder.lock()->isSetValueCurrentlyPossible() && gui.lock();
    }
};

KnobHelper::KnobHelper(const KnobHolderPtr& holder,
                       const std::string &label,
                       int nDims,
                       bool declaredByPlugin)
    : _signalSlotHandler()
    , _imp( new KnobHelperPrivate(this, holder, nDims, label, declaredByPlugin) )
{
    if (holder) {
        setHashParent(holder);
    }
}

KnobHelper::~KnobHelper()
{
}

void
KnobHelper::setHolder(const KnobHolderPtr& holder)
{
    _imp->holder = holder;
}

void
KnobHelper::incrementExpressionRecursionLevel() const
{
    KnobDataTLSPtr tls = _imp->tlsData->getOrCreateTLSData();

    assert(tls);
    ++tls->expressionRecursionLevel;
}

void
KnobHelper::decrementExpressionRecursionLevel() const
{
    KnobDataTLSPtr tls = _imp->tlsData->getTLSData();

    assert(tls);
    assert(tls->expressionRecursionLevel > 0);
    --tls->expressionRecursionLevel;
}

int
KnobHelper::getExpressionRecursionLevel() const
{
    KnobDataTLSPtr tls = _imp->tlsData->getTLSData();

    if (!tls) {
        return 0;
    }

    return tls->expressionRecursionLevel;
}

void
KnobHelper::setHashingStrategy(KnobFrameViewHashingStrategyEnum strategy)
{
    _imp->cacheInvalidationStrategy = strategy;
}

KnobFrameViewHashingStrategyEnum
KnobHelper::getHashingStrategy() const
{
    return _imp->cacheInvalidationStrategy;
}

void
KnobHelper::deleteKnob()
{
    KnobI::ListenerDimsMap listenersCpy = _imp->listeners;

    for (ListenerDimsMap::iterator it = listenersCpy.begin(); it != listenersCpy.end(); ++it) {
        KnobIPtr knob = it->first.lock();
        if (!knob) {
            continue;
        }
        KnobIPtr aliasKnob = knob->getAliasMaster();
        if (aliasKnob.get() == this) {
            knob->setKnobAsAliasOfThis(aliasKnob, false);
        }
        for (int i = 0; i < knob->getNDimensions(); ++i) {
            knob->setExpressionInvalid( i, ViewSpec::all(), false, tr("%1: parameter does not exist").arg( QString::fromUtf8( getName().c_str() ) ).toStdString() );
            knob->unSlave(i, false);
        }
    }

    KnobHolderPtr holder = getHolder();

    if ( holder && holder->getApp() ) {
        holder->getApp()->recheckInvalidExpressions();
    }

    for (int i = 0; i < getNDimensions(); ++i) {
        clearExpression(i, true);
    }

    resetParent();


    if (holder) {

        // For containers also delete children.
        KnobGroup* isGrp =  dynamic_cast<KnobGroup*>(this);
        KnobPage* isPage = dynamic_cast<KnobPage*>(this);
        if (isGrp)     {
            KnobsVec children = isGrp->getChildren();
            for (KnobsVec::iterator it = children.begin(); it != children.end(); ++it) {
                holder->deleteKnob(*it, true);
            }
        } else if (isPage) {
            KnobsVec children = isPage->getChildren();
            for (KnobsVec::iterator it = children.begin(); it != children.end(); ++it) {
                holder->deleteKnob(*it, true);
            }
        }

        EffectInstancePtr effect = toEffectInstance(holder);
        if (effect) {
            NodePtr node = effect->getNode();
            if (node) {
                if ( useHostOverlayHandle() ) {
                    node->removePositionHostOverlay( shared_from_this() );
                }
                node->removeParameterFromPython( getName() );
            }
        }
    }
    _signalSlotHandler.reset();
} // KnobHelper::deleteKnob

void
KnobHelper::setKnobGuiPointer(const KnobGuiIPtr& ptr)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->gui = ptr;
}

KnobGuiIPtr
KnobHelper::getKnobGuiPointer() const
{
    return _imp->gui.lock();
}

bool
KnobHelper::getAllDimensionVisible() const
{
    if ( getKnobGuiPointer() ) {
        return getKnobGuiPointer()->getAllDimensionsVisible();
    }

    return true;
}

#ifdef DEBUG
void
KnobHelper::debugHook()
{
    assert(true);
}

#endif

void
KnobHelper::setDeclaredByPlugin(bool b)
{
    _imp->declaredByPlugin = b;
}

bool
KnobHelper::isDeclaredByPlugin() const
{
    return _imp->declaredByPlugin;
}

void
KnobHelper::setDynamicallyCreated()
{
    _imp->dynamicallyCreated = true;
}

bool
KnobHelper::isDynamicallyCreated() const
{
    return _imp->dynamicallyCreated;
}

void
KnobHelper::setAsUserKnob(bool b)
{
    _imp->userKnob = b;
    _imp->dynamicallyCreated = b;
}

bool
KnobHelper::isUserKnob() const
{
    return _imp->userKnob;
}

void
KnobHelper::populate()
{
    KnobIPtr thisKnob = shared_from_this();
    boost::shared_ptr<KnobSignalSlotHandler> handler( new KnobSignalSlotHandler(thisKnob) );

    setSignalSlotHandler(handler);

    if (!isAnimatedByDefault()) {
        _imp->isAnimationEnabled = false;
    }

    KnobColor* isColor = dynamic_cast<KnobColor*>(this);
    KnobSeparator* isSep = dynamic_cast<KnobSeparator*>(this);
    KnobPage* isPage = dynamic_cast<KnobPage*>(this);
    KnobGroup* isGrp = dynamic_cast<KnobGroup*>(this);
    if (isPage || isGrp) {
        _imp->evaluateOnChange = false;
    }
    if (isSep) {
        _imp->IsPersistent = false;
    }
    for (int i = 0; i < _imp->dimension; ++i) {
        _imp->enabled[i] = true;
        if ( canAnimate() ) {
            _imp->curves[i][ViewIdx(0)].reset( new Curve(shared_from_this(), i) );
        }
        _imp->animationLevel[i] = eAnimationLevelNone;


        if (!isColor) {
            switch (i) {
            case 0:
                _imp->dimensionNames[i] = "x";
                break;
            case 1:
                _imp->dimensionNames[i] = "y";
                break;
            case 2:
                _imp->dimensionNames[i] = "z";
                break;
            case 3:
                _imp->dimensionNames[i] = "w";
                break;
            default:
                break;
            }
        } else {
            switch (i) {
            case 0:
                _imp->dimensionNames[i] = "r";
                break;
            case 1:
                _imp->dimensionNames[i] = "g";
                break;
            case 2:
                _imp->dimensionNames[i] = "b";
                break;
            case 3:
                _imp->dimensionNames[i] = "a";
                break;
            default:
                break;
            }
        }
    }
} // KnobHelper::populate

std::string
KnobHelper::getDimensionName(int dimension) const
{
    assert( dimension < (int)_imp->dimensionNames.size() && dimension >= 0);

    return _imp->dimensionNames[dimension];
}

void
KnobHelper::setDimensionName(int dim,
                             const std::string & name)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->dimensionNames[dim] = name;
    _signalSlotHandler->s_dimensionNameChanged(dim);
}

template <typename T>
const std::string &
Knob<T>::typeName() const
{
    static std::string knobNoTypeName("NoType");

    return knobNoTypeName;
}

template <typename T>
bool
Knob<T>::canAnimate() const
{
    return false;
}

void
KnobHelper::setSignalSlotHandler(const boost::shared_ptr<KnobSignalSlotHandler> & handler)
{
    _signalSlotHandler = handler;
}

void
KnobHelper::deleteValuesAtTime(CurveChangeReason curveChangeReason,
                               const std::list<double>& times,
                               ViewSpec view,
                               int dimension)
{
    if ( ( dimension > (int)_imp->curves.size() ) || (dimension < 0) ) {
        throw std::invalid_argument("KnobHelper::deleteValueAtTime(): Dimension out of range");
    }

    if ( times.empty() ) {
        return;
    }

    // If not animated, don't bother
    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return;
    }

    // Figure out which curve to use depending on whether we are rendering or not
    bool mustRefreshGuiCurve;
#pragma message WARN("deleteValuesAtTime does not support multi-view yet")
    CurvePtr curve = getCurveToOperateOn(ViewIdx(view.value()), dimension, true, &mustRefreshGuiCurve);

    int nKeys = curve->getKeyFramesCount();

    // We are about to remove the last keyframe, ensure that the internal value of the knob is the one of the animation
    if (nKeys - times.size() == 0) {
        copyValuesFromCurve(dimension);
    }

    // Update internal curve
    std::list<double> keysRemoved;
    try {
        for (std::list<double>::const_iterator it = times.begin(); it != times.end(); ++it) {
            curve->removeKeyFrameWithTime(*it);
            keysRemoved.push_back(*it);
        }
    } catch (const std::exception & /*e*/) {
    }

    // If we are directly editing the internal curve, also update the gui curve
    if (mustRefreshGuiCurve) {
        CurvePtr guiCurve = getKnobGuiPointer()->getCurve(view, dimension);
        assert(guiCurve);
        try {
            for (std::list<double>::const_iterator it = times.begin(); it != times.end(); ++it) {
                guiCurve->removeKeyFrameWithTime(*it);
            }
        } catch (...) {

        }
    }
    

    // virtual portion
    onKeyframesRemoved(times, view, dimension, curveChangeReason);


    // Refresh the has animation flag on the holder
    {
        KnobHolderPtr holder = getHolder();
        if (holder) {
            holder->updateHasAnimation();
        }
    }
    

    // Now refresh animation level and notify we changed to the holder...
    // If we are editing the gui curve it will be taken care of in the dequeueValues action
    ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited;
    if (mustRefreshGuiCurve) {
        refreshAnimationLevel(view, dimension);
        evaluateValueChange(dimension, *times.begin(), view, reason);
    }

    _signalSlotHandler->s_redrawGuiCurve(curveChangeReason, view, dimension);

    std::list<double> keysAdded;
    _signalSlotHandler->s_curveAnimationChanged(keysAdded, keysRemoved, view, dimension, curveChangeReason);

} // deleteValuesAtTime

void
KnobHelper::deleteAnimationConditional(double time,
                                       ViewSpec view,
                                       int dimension,
                                       CurveChangeReason reason,
                                       bool before)
{
    if (!_imp->curves[dimension]) {
        return;
    }
    assert( 0 <= dimension && dimension < getNDimensions() );

    // Figure out which curve to use depending on whether we are rendering or not
    bool mustRefreshGuiCurve;
#pragma message WARN("deleteAnimationConditional does not support multi-view yet")
    CurvePtr curve = getCurveToOperateOn(ViewIdx(view.value()), dimension, true, &mustRefreshGuiCurve);

    std::list<double> keysRemoved;
    if (before) {
        curve->removeKeyFramesBeforeTime(time, &keysRemoved);
    } else {
        curve->removeKeyFramesAfterTime(time, &keysRemoved);
    }

    if (mustRefreshGuiCurve) {
        refreshAnimationLevel(view, dimension);
        guiCurveCloneInternalCurve(reason, view, dimension, eValueChangedReasonNatronInternalEdited);
        evaluateValueChange(dimension, time, view, eValueChangedReasonNatronInternalEdited);
    }

    std::list<double> keysAdded;
    _signalSlotHandler->s_curveAnimationChanged(keysAdded, keysRemoved, view, dimension, reason);

} // deleteAnimationConditional

void
KnobHelper::deleteAnimationBeforeTime(double time,
                                      ViewSpec view,
                                      int dimension,
                                      CurveChangeReason reason)
{
    deleteAnimationConditional(time, view, dimension, reason, true);
}

void
KnobHelper::deleteAnimationAfterTime(double time,
                                     ViewSpec view,
                                     int dimension,
                                     CurveChangeReason reason)
{
    deleteAnimationConditional(time, view, dimension, reason, false);
}


bool
KnobHelper::warpValuesAtTime(CurveChangeReason reason, const std::list<double>& times, ViewSpec view,  int dimension, const Curve::KeyFrameWarp& warp, bool allowReplacingExistingKeys, std::vector<KeyFrame>* outKeys)
{
    if ( ( dimension > (int)_imp->curves.size() ) || (dimension < 0) ) {
        throw std::invalid_argument("KnobHelper::warpValuesAtTimeInternal(): Dimension out of range");
    }
    if (times.empty()) {
        return true;
    }

    // Don't bother if not animated
    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return true;
    }


    // Figure out which curve to use depending on whether we are rendering or not
    bool mustRefreshGuiCurve;
#pragma message WARN("warpValuesAtTime does not support multi-view yet")
    CurvePtr curve = getCurveToOperateOn(ViewIdx(view.value()), dimension, true, &mustRefreshGuiCurve);


    double firstKeyTime = 0.;

    // Make sure string animation follows up
    AnimatingKnobStringHelper* isString = dynamic_cast<AnimatingKnobStringHelper*>(this);

    if (outKeys) {
        outKeys->clear();
    }

    std::list<double> keysAdded, keysRemoved;

    std::vector<std::string> oldStringValues;
    if (isString) {
        oldStringValues.resize(times.size());
        int i = 0;
        for (std::list<double>::const_iterator it = times.begin(); it!=times.end(); ++it, ++i) {
            isString->stringFromInterpolatedValue(*it, view, &oldStringValues[i]);
        }
    }
    // This may fail if we cannot find a keyframe at the given time
    std::vector<KeyFrame> newKeys;
    if ( !curve->transformKeyframesValueAndTime(times, warp, allowReplacingExistingKeys, &newKeys, &keysAdded, &keysAdded) ) {
        return false;
    }
    if (outKeys) {
        *outKeys = newKeys;
    }

    // If we are updating internal curve, also update gui curve
    if (mustRefreshGuiCurve) {
        CurvePtr guiCurve = getKnobGuiPointer()->getCurve(view, dimension);
        assert(guiCurve);
        guiCurve->transformKeyframesValueAndTime(times, warp, allowReplacingExistingKeys);
    }

    // update string animation
    if (isString) {
        onKeyframesRemoved(times, view, dimension, reason);
        assert(newKeys.size() == oldStringValues.size());
        for (std::size_t i = 0; i < newKeys.size(); ++i) {
            double ret;
            isString->stringToKeyFrameValue(newKeys[i].getTime(), ViewIdx(view.value()), oldStringValues[i], &ret);
        }
    }



    // Now refresh animation level and notify we changed to the holder...
    // If we are editing the gui curve it will be taken care of in the dequeueValues action
    if (mustRefreshGuiCurve) {
        evaluateValueChange(dimension, firstKeyTime, view, eValueChangedReasonNatronInternalEdited);

    }

    _signalSlotHandler->s_curveAnimationChanged(keysAdded, keysRemoved, view, dimension, reason);

    return true;

} // warpValuesAtTime



CurvePtr
KnobHelper::getCurveToOperateOn(ViewIdx view, int dimension, bool throwIfNotPresent, bool* mustRefreshGuiCurve)
{
    // Figure out whether to use gui curves or internal curves, depending on whether we are rendering
    KnobGuiIPtr hasGui = getKnobGuiPointer();
    bool useGuiCurve = _imp->shouldUseGuiCurve();

    CurvePtr ret;
    if (!useGuiCurve) {
        // Not rendering, work directly on internal curve
        ret = _imp->curves[dimension];
    } else {
        // Rendering, work on gui curve and flag that we need to copy the gui curve when rendering is done (i.e: in dequeueActions)
        assert(hasGui);
        ret = hasGui->getCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension, true);
    }

    // hmm no curve, this parameter isn't animated?
    if (!ret && throwIfNotPresent) {
        throw std::runtime_error("KnobHelper::getCurveToOperateOn: curve is NULL");
    }

    // If we work the internal curve, we must update
    // the gui curve immediately after the changes made to the curve
    if (hasGui && !useGuiCurve) {
        *mustRefreshGuiCurve = true;
    } else {
        *mustRefreshGuiCurve = false;
    }
    return ret;
}

bool
KnobHelper::cloneCurve(ViewIdx view,
                       int dimension,
                       const Curve& curve,
                       double offset, const RangeD* range,
                       const StringAnimationManager* stringAnimation,
                       CurveChangeReason reason)
{
    assert( dimension >= 0 && dimension < (int)_imp->curves.size() );
    if (dimension < 0 || dimension >= (int)_imp->curves.size()) {
        throw std::invalid_argument("KnobHelper::cloneCurve: invalid dimension index");
    }

    // Figure out whether to use gui curves or internal curves, depending on whether we are rendering
    bool mustRefreshGuiCurve;
    CurvePtr thisCurve = getCurveToOperateOn(view, dimension, false, &mustRefreshGuiCurve);
    if (!thisCurve) {
        return false;
    }

    KnobGuiIPtr hasGui = getKnobGuiPointer();

    // Only track keyframe changes if there is a knob gui
    KeyFrameSet oldKeys;
    if (hasGui) {
        oldKeys = thisCurve->getKeyFrames_mt_safe();
    }

    // When there's no gui, we don't have to update keyframes on gui, so just clone fast without checking for changes
    bool hasChanged ;
    if (!hasGui) {
        if (range) {
            thisCurve->clone(curve, offset, range);
        } else {
            thisCurve->clone(curve);
        }
        hasChanged = true;
    } else {
        // Check for changes, slower but may avoid computing diff of keyframes
        hasChanged = thisCurve->cloneAndCheckIfChanged(curve, offset, range);
    }

    // Clone string animation if necesssary
    if (stringAnimation) {
        StringAnimationManagerPtr thisStringAnimation = getStringAnimation();
        if (thisStringAnimation) {
            thisStringAnimation->cloneAndCheckIfChanged(*stringAnimation);
        }
    }

    // If we are editing the gui curve it will be taken care of in the dequeueValues action
    if (mustRefreshGuiCurve) {
        evaluateValueChange(dimension, getCurrentTime(), view,  eValueChangedReasonNatronInternalEdited);
        guiCurveCloneInternalCurve(eCurveChangeReasonInternal, view, dimension, eValueChangedReasonNatronInternalEdited);
    }

    if (hasGui && hasChanged) {
        KeyFrameSet keys = thisCurve->getKeyFrames_mt_safe();

        std::list<double> keysRemoved, keysAdded;
        for (KeyFrameSet::iterator it = keys.begin(); it != keys.end(); ++it) {
            KeyFrameSet::iterator foundInOldKeys = Curve::findWithTime(oldKeys, oldKeys.end(), it->getTime());
            if (foundInOldKeys == oldKeys.end()) {
                keysAdded.push_back(it->getTime());
            } else {
                oldKeys.erase(foundInOldKeys);
            }
        }

        for (KeyFrameSet::iterator it = oldKeys.begin(); it != oldKeys.end(); ++it) {
            KeyFrameSet::iterator foundInNextKeys = Curve::findWithTime(keys, keys.end(), it->getTime());
            if (foundInNextKeys == keys.end()) {
                keysRemoved.push_back(it->getTime());
            }
        }

        if (!keysAdded.empty() || !keysRemoved.empty()) {
            _signalSlotHandler->s_curveAnimationChanged(keysAdded, keysRemoved, view, dimension, reason);
        }
    }
    return hasChanged;
} // cloneCurve

bool
KnobHelper::cloneCurves(const KnobIPtr& other,
                        ViewIdx view,
                        double offset, const RangeD* range,
                        int dimension,
                        int otherDimension)
{
    assert( (dimension == otherDimension) || (dimension != -1) );
    assert(other);
    assert( (dimension == otherDimension) || (dimension != -1) );
    assert(other);
    bool hasChanged = false;
    if (dimension == -1) {
        int dimMin = std::min( getNDimensions(), other->getNDimensions() );
        for (int i = 0; i < dimMin; ++i) {
            CurvePtr otherCurve = other->getCurve(ViewIdx(0), i);
            if (!otherCurve) {
                continue;
            }
            StringAnimationManagerPtr stringAnim = other->getStringAnimation();
            hasChanged |= cloneCurve(view, i, *otherCurve, offset, range, stringAnim.get(), eCurveChangeReasonInternal);
        }
    } else {
        if (otherDimension == -1) {
            otherDimension = dimension;
        }
        assert( dimension >= 0 && dimension < getNDimensions() &&
               otherDimension >= 0 && otherDimension < getNDimensions() );
        CurvePtr otherCurve = other->getCurve(ViewIdx(0), otherDimension);
        if (!otherCurve) {
            return false;
        }
        StringAnimationManagerPtr stringAnim = other->getStringAnimation();
        hasChanged |= cloneCurve(view, dimension, *otherCurve, offset, range, stringAnim.get(), eCurveChangeReasonInternal);
    }
    
    return hasChanged;
}

void
KnobHelper::setInterpolationAtTimes(CurveChangeReason reason, ViewSpec view, int dimension, const std::list<double>& times, KeyframeTypeEnum interpolation, std::vector<KeyFrame>* newKeys)
{
    assert( QThread::currentThread() == qApp->thread() );
    assert( dimension >= 0 && dimension < (int)_imp->curves.size() );

    if (times.empty()) {
        return;
    }
    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return;
    }

    bool mustRefreshGuiCurve;
#pragma message WARN("setInterpolationAtTimes does not support multi-view yet")
    CurvePtr curve = getCurveToOperateOn(ViewIdx(view.value()), dimension, false, &mustRefreshGuiCurve);

    for (std::list<double>::const_iterator it = times.begin(); it!=times.end(); ++it) {
        KeyFrame k;
        if (curve->setKeyFrameInterpolation(interpolation, *it, &k)) {
            if (newKeys) {
                newKeys->push_back(k);
            }
        }
    }
    double time = getCurrentTime();

    if (mustRefreshGuiCurve) {
        CurvePtr guiCurve = getKnobGuiPointer()->getCurve(view, dimension);
        assert(guiCurve);
        guiCurveCloneInternalCurve(reason, view, dimension, eValueChangedReasonNatronInternalEdited);
        evaluateValueChange(dimension, time, view, eValueChangedReasonNatronInternalEdited);

    }


    _signalSlotHandler->s_keyFrameInterpolationChanged(time, view, dimension, reason);

}

bool
KnobHelper::setLeftAndRightDerivativesAtTime(CurveChangeReason reason,
                                             ViewSpec view,
                                             int dimension,
                                             double time,
                                             double left,
                                             double right)
{
    assert( QThread::currentThread() == qApp->thread() );
    if ( ( dimension > (int)_imp->curves.size() ) || (dimension < 0) ) {
        throw std::invalid_argument("KnobHelper::setInterpolationAtTime(): Dimension out of range");
    }

    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return false;
    }

    bool mustRefreshGuiCurve;
#pragma message WARN("setLeftAndRightDerivativesAtTime does not support multi-view yet")
    CurvePtr curve = getCurveToOperateOn(ViewIdx(view.value()), dimension, false, &mustRefreshGuiCurve);

    int keyIndex = curve->keyFrameIndex(time);
    if (keyIndex == -1) {
        return false;
    }

    curve->setKeyFrameInterpolation(eKeyframeTypeFree, keyIndex);
    curve->setKeyFrameDerivatives(left, right, keyIndex);

    if (mustRefreshGuiCurve) {
        CurvePtr guiCurve = getKnobGuiPointer()->getCurve(view, dimension);
        assert(guiCurve);
        guiCurve->setKeyFrameInterpolation(eKeyframeTypeFree, keyIndex);
        guiCurve->setKeyFrameDerivatives(left, right, keyIndex);
        evaluateValueChange(dimension, time, view, eValueChangedReasonNatronInternalEdited);

    }


    _signalSlotHandler->s_derivativeMoved(time, view, dimension, reason);

    return true;
} // KnobHelper::moveDerivativesAtTime

bool
KnobHelper::setDerivativeAtTime(CurveChangeReason reason,
                                 ViewSpec view,
                                 int dimension,
                                 double time,
                                 double derivative,
                                 bool isLeft)
{
    assert( QThread::currentThread() == qApp->thread() );
    if ( ( dimension > (int)_imp->curves.size() ) || (dimension < 0) ) {
        throw std::invalid_argument("KnobHelper::setInterpolationAtTime(): Dimension out of range");
    }

    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return false;
    }

    bool mustRefreshGuiCurve;
#pragma message WARN("setDerivativeAtTime does not support multi-view yet")
    CurvePtr curve = getCurveToOperateOn(ViewIdx(view.value()), dimension, false, &mustRefreshGuiCurve);

    int keyIndex = curve->keyFrameIndex(time);
    if (keyIndex == -1) {
        return false;
    }

    curve->setKeyFrameInterpolation(eKeyframeTypeBroken, keyIndex);
    if (isLeft) {
        curve->setKeyFrameLeftDerivative(derivative, keyIndex);
    } else {
        curve->setKeyFrameRightDerivative(derivative, keyIndex);
    }

    if (mustRefreshGuiCurve) {
        CurvePtr guiCurve = getKnobGuiPointer()->getCurve(view, dimension);
        assert(guiCurve);
        guiCurve->setKeyFrameInterpolation(eKeyframeTypeBroken, keyIndex);
        if (isLeft) {
            guiCurve->setKeyFrameLeftDerivative(derivative, keyIndex);
        } else {
            guiCurve->setKeyFrameRightDerivative(derivative, keyIndex);
        }
        evaluateValueChange(dimension, time, view, eValueChangedReasonNatronInternalEdited);

    }

    _signalSlotHandler->s_derivativeMoved(time, view, dimension, reason);

    return true;
} // KnobHelper::moveDerivativeAtTime


void
KnobHelper::removeAnimation(ViewSpec view, DimSpec dimensions, CurveChangeReason reason)
{
    if (dimensions.empty()) {
        return;
    }

    if (!canAnimate()) {
        return;
    }

    int dim_i = dimensions.value();
    int nDims = getNDimensions();

    bool animationEnabled = isAnimationEnabled();
    for (int i = 0; i < nDims; ++i) {
        if (dimension != DimSpec::all().value() && i != dimension) {
            continue;
        }
        if (animationEnabled && !isAnimated(i, view)) {
            continue;
        }

        bool mustRefreshGuiCurve;
#pragma message WARN("removeAnimationAcrossDimensions does not support multi-view yet")
        CurvePtr curve = getCurveToOperateOn(ViewIdx(view.value()), i, false, &mustRefreshGuiCurve);


        // Ensure the underlying values are the values of the current curve at the current time
        copyValuesFromCurve(i);

        // Notify the user of the keyframes that were removed
        std::list<double> timesRemoved;

        assert(curve);
        if (curve) {
            KeyFrameSet keys = curve->getKeyFrames_mt_safe();
            for (KeyFrameSet::const_iterator it = keys.begin(); it!=keys.end(); ++it) {
                timesRemoved.push_back(it->getTime());
            }
            curve->clearKeyFrames();
        }

        // Nothing changed
        if (timesRemoved.empty()) {
            continue;
        }

        onKeyframesRemoved(timesRemoved, view, i, reason);


        if (mustRefreshGuiCurve) {
            evaluateValueChange(i, getCurrentTime(), view, eValueChangedReasonNatronInternalEdited);
            guiCurveCloneInternalCurve(eCurveChangeReasonInternal, view, i, eValueChangedReasonNatronInternalEdited);
        }
    
        
        std::list<double> timesAdded;
        _signalSlotHandler->s_curveAnimationChanged(timesAdded, timesRemoved, view, dimension, reason);

    } // for dimensions


    {
        KnobHolderPtr holder = getHolder();
        if (holder) {
            holder->updateHasAnimation();
        }
    }
}


void
KnobHelper::cloneInternalCurvesIfNeeded(std::map<int, ValueChangedReasonEnum>& modifiedDimensions)
{
    QMutexLocker k(&_imp->mustCloneGuiCurvesMutex);

    for (int i = 0; i < getNDimensions(); ++i) {
        if (_imp->mustCloneInternalCurves[i]) {
            guiCurveCloneInternalCurve(eCurveChangeReasonInternal, ViewIdx(0), i, eValueChangedReasonNatronInternalEdited);
            _imp->mustCloneInternalCurves[i] = false;
            modifiedDimensions.insert( std::make_pair(i, eValueChangedReasonNatronInternalEdited) );
        }
    }
}

void
KnobHelper::setInternalCurveHasChanged(ViewSpec /*view*/,
                                       int dimension,
                                       bool changed)
{
    QMutexLocker k(&_imp->mustCloneGuiCurvesMutex);

    _imp->mustCloneInternalCurves[dimension] = changed;
}

void
KnobHelper::cloneGuiCurvesIfNeeded(std::map<int, ValueChangedReasonEnum>& modifiedDimensions)
{
    if ( !canAnimate() ) {
        return;
    }

    bool hasChanged = false;
    QMutexLocker k(&_imp->mustCloneGuiCurvesMutex);
    for (int i = 0; i < getNDimensions(); ++i) {
        if (_imp->mustCloneGuiCurves[i]) {
            hasChanged = true;
            CurvePtr curve = _imp->curves[i];
            assert(curve);
            KnobGuiIPtr hasGui = getKnobGuiPointer();
            CurvePtr guicurve;
            if (hasGui) {
                guicurve = hasGui->getCurve(ViewIdx(0), i);
                assert(guicurve);
                curve->clone(*guicurve);
            }

            _imp->mustCloneGuiCurves[i] = false;

            modifiedDimensions.insert( std::make_pair(i, eValueChangedReasonUserEdited) );
        }
    }
    if (hasChanged) {
        KnobHolderPtr holder = getHolder();
        if (holder) {
            holder->updateHasAnimation();
        }
    }
}

void
KnobHelper::guiCurveCloneInternalCurve(CurveChangeReason curveChangeReason,
                                       ViewSpec view,
                                       int dimension,
                                       ValueChangedReasonEnum reason)
{
    if ( !canAnimate() ) {
        return;
    }
    KnobGuiIPtr hasGui = getKnobGuiPointer();
    if (hasGui) {
        CurvePtr guicurve = hasGui->getCurve(view, dimension);
        assert(guicurve);
        guicurve->clone( *(_imp->curves[dimension]) );
        if ( _signalSlotHandler && (reason != eValueChangedReasonUserEdited) ) {
            _signalSlotHandler->s_redrawGuiCurve(curveChangeReason, view, dimension);
        }
    }
}

CurvePtr
KnobHelper::getGuiCurve(ViewSpec view,
                        int dimension,
                        bool byPassMaster) const
{
    if ( !canAnimate() ) {
        return CurvePtr();
    }

    std::pair<int, KnobIPtr > master = getMaster(dimension);
    if (!byPassMaster && master.second) {
        return master.second->getGuiCurve(view, master.first);
    }

    KnobGuiIPtr hasGui = getKnobGuiPointer();
    if (hasGui) {
        return hasGui->getCurve(view, dimension);
    } else {
        return CurvePtr();
    }
}

void
KnobHelper::setGuiCurveHasChanged(ViewSpec /*view*/,
                                  int dimension,
                                  bool changed)
{
    assert( dimension < (int)_imp->mustCloneGuiCurves.size() );
    QMutexLocker k(&_imp->mustCloneGuiCurvesMutex);
    _imp->mustCloneGuiCurves[dimension] = changed;
}

bool
KnobHelper::hasGuiCurveChanged(ViewSpec /*view*/,
                               int dimension) const
{
    assert( dimension < (int)_imp->mustCloneGuiCurves.size() );
    QMutexLocker k(&_imp->mustCloneGuiCurvesMutex);

    return _imp->mustCloneGuiCurves[dimension];
}

CurvePtr KnobHelper::getCurve(ViewIdx view,
                             int dimension,
                             bool byPassMaster) const
{
    if ( (dimension < 0) || ( dimension >= (int)_imp->curves.size() ) ) {
        return CurvePtr();
    }

    std::pair<int, KnobIPtr > master = getMaster(dimension);
    if (!byPassMaster && master.second) {
        return master.second->getCurve(view, master.first);
    }

    QMutexLocker k(&_imp->curvesMutex);
    PerViewCurveMap::const_iterator foundView = _imp->curves[dimension].find(view);
    if (foundView == _imp->curves[dimension].end()) {
        return CurvePtr();
    }

    return foundView->second;
}

CurvePtr
KnobHelper::getAnimationCurve(ViewIdx idx, int dimension) const
{
    return getCurve(idx, dimension);
}

bool
KnobHelper::isAnimated(int dimension,
                       ViewSpec view) const
{
    if ( !canAnimate() ) {
        return false;
    }
    assert(!view.isAll());
    if (view.isAll()) {
        throw std::invalid_argument("Knob::isAnimated: Invalid ViewSpec: all views has no meaning in this function");
    }
    if (dimension < 0 || dimension >= (int)_imp->dimension) {
        return false;
    }

    ViewIdx view_i(0);
    if (hasViewsSplitOff(dimension)) {
        if (view.isCurrent()) {
            view_i = getCurrentView();
        } else {
            assert(view.isViewIdx());
            view_i = ViewIdx(view.value());
        }
    }
    CurvePtr curve = getCurve(view_i, dimension);
    return curve ? curve->isAnimated() : false;
}

bool
KnobHelper::hasViewsSplitOff(int dimension) const
{
    QMutexLocker k(&_imp->curvesMutex);
    assert(dimension >= 0 && dimension < (int)_imp->curves.size());
    return _imp->curves[dimension].size() > 1;
}

int
KnobHelper::getNDimensions() const
{
    return _imp->dimension;
}

void
KnobHelper::beginChanges()
{
    KnobHolderPtr holder = getHolder();
    if (holder) {
        holder->beginChanges();
    }
}

void
KnobHelper::endChanges()
{
    KnobHolderPtr holder = getHolder();
    if (holder) {
        holder->endChanges();
    }
}

void
KnobHelper::blockValueChanges()
{
    QMutexLocker k(&_imp->valueChangedBlockedMutex);

    ++_imp->valueChangedBlocked;
}

void
KnobHelper::unblockValueChanges()
{
    QMutexLocker k(&_imp->valueChangedBlockedMutex);

    --_imp->valueChangedBlocked;
}

bool
KnobHelper::isValueChangesBlocked() const
{
    QMutexLocker k(&_imp->valueChangedBlockedMutex);

    return _imp->valueChangedBlocked > 0;
}

void
KnobHelper::blockListenersNotification()
{
    QMutexLocker k(&_imp->valueChangedBlockedMutex);

    ++_imp->listenersNotificationBlocked;
}

void
KnobHelper::unblockListenersNotification()
{
    QMutexLocker k(&_imp->valueChangedBlockedMutex);

    --_imp->listenersNotificationBlocked;
}

bool
KnobHelper::isListenersNotificationBlocked() const
{
    QMutexLocker k(&_imp->valueChangedBlockedMutex);

    return _imp->listenersNotificationBlocked > 0;
}


void
KnobHelper::setAutoKeyingEnabled(bool enabled)
{
    QMutexLocker k(&_imp->valueChangedBlockedMutex);
    if (enabled) {
        ++_imp->autoKeyingDisabled;
    } else {
        --_imp->autoKeyingDisabled;
    }
}

bool
KnobHelper::isAutoKeyingEnabled(int dimension, ValueChangedReasonEnum reason) const
{
    if (dimension < 0 || dimension >= _imp->dimension) {
        return false;
    }

    // The knob cannot animate
    if (!isAnimationEnabled()) {
        return false;
    }

    // The knob doesn't have any animation don't start keying automatically
    if (getAnimationLevel(dimension) == eAnimationLevelNone) {
        return false;
    }

    // Knobs without an effect cannot auto-key
    KnobHolderPtr holder = getHolder();
    if (!holder) {
        return false;
    }

    // Hmm this is a custom Knob used somewhere, don't allow auto-keying
    if (!holder->getApp()) {
        return false;
    }

    // Check for reason appropriate for auto-keying
    if ( (reason != eValueChangedReasonUserEdited) &&
        (reason != eValueChangedReasonPluginEdited) &&
        (reason != eValueChangedReasonNatronGuiEdited) &&
        (reason != eValueChangedReasonNatronInternalEdited)) {
        return false;
    }

    // Finally return the value set to setAutoKeyingEnabled
    QMutexLocker k(&_imp->valueChangedBlockedMutex);
    return _imp->autoKeyingDisabled;
}

bool
KnobHelper::evaluateValueChangeInternal(int dimension,
                                        double time,
                                        ViewSpec view,
                                        ValueChangedReasonEnum reason,
                                        ValueChangedReasonEnum originalReason)
{
    // If this assert triggers, that is that the knob have had its function deleteKnob() called but the object was not really deleted.
    // You should investigate why the Knob was not deleted.
    assert(_signalSlotHandler);

    AppInstancePtr app;
    KnobHolderPtr holder = getHolder();
    if (holder) {
        app = holder->getApp();
    }

    /// For eValueChangedReasonTimeChanged we never call the instanceChangedAction and evaluate otherwise it would just throttle
    /// the application responsiveness
    onInternalValueChanged(dimension, time, view);

    bool ret = false;
    if ( ( (originalReason != eValueChangedReasonTimeChanged) || evaluateValueChangeOnTimeChange() ) && holder ) {
        holder->beginChanges();
        KnobIPtr thisShared = shared_from_this();
        assert(thisShared);
        holder->appendValueChange(thisShared, dimension, time, view, originalReason, reason);
        ret |= holder->endChanges();
    }


    if (!holder) {
        computeHasModifications();
        _signalSlotHandler->s_valueChanged(view, dimension, (int)reason);
        if ( !isListenersNotificationBlocked() ) {
            refreshListenersAfterValueChange(view, originalReason, dimension);
        }
        refreshAnimationLevel(view, dimension);
    }

    return ret;
}

bool
KnobHelper::evaluateValueChange(int dimension,
                                double time,
                                ViewSpec view,
                                ValueChangedReasonEnum reason)
{
    return evaluateValueChangeInternal(dimension, time, view, reason, reason);
}

void
KnobHelper::setAddNewLine(bool newLine)
{
    _imp->newLine = newLine;
}

bool
KnobHelper::isNewLineActivated() const
{
    return _imp->newLine;
}

void
KnobHelper::setAddSeparator(bool addSep)
{
    _imp->addSeparator = addSep;
}

bool
KnobHelper::isSeparatorActivated() const
{
    return _imp->addSeparator;
}

void
KnobHelper::setSpacingBetweenItems(int spacing)
{
    _imp->itemSpacing = spacing;
}

int
KnobHelper::getSpacingBetweenitems() const
{
    return _imp->itemSpacing;
}

std::string
KnobHelper::getInViewerContextLabel() const
{
    QMutexLocker k(&_imp->labelMutex);

    return _imp->inViewerContextLabel;
}

void
KnobHelper::setInViewerContextLabel(const QString& label)
{
    {
        QMutexLocker k(&_imp->labelMutex);

        _imp->inViewerContextLabel = label.toStdString();
    }
    _signalSlotHandler->s_inViewerContextLabelChanged();
}

std::string
KnobHelper::getInViewerContextIconFilePath(bool checked) const
{
    QMutexLocker k(&_imp->labelMutex);
    int idx = !checked ? 0 : 1;

    if ( !_imp->inViewerContextIconFilePath[idx].empty() ) {
        return _imp->inViewerContextIconFilePath[idx];
    }
    int otherIdx = !checked ? 1 : 0;

    return _imp->inViewerContextIconFilePath[otherIdx];
}

void
KnobHelper::setInViewerContextIconFilePath(const std::string& icon, bool checked)
{
    QMutexLocker k(&_imp->labelMutex);
    int idx = !checked ? 0 : 1;

    _imp->inViewerContextIconFilePath[idx] = icon;
}

void
KnobHelper::setInViewerContextCanHaveShortcut(bool haveShortcut)
{
    _imp->inViewerContextHasShortcut = haveShortcut;
}

bool
KnobHelper::getInViewerContextHasShortcut() const
{
    return _imp->inViewerContextHasShortcut;
}

void
KnobHelper::addInViewerContextShortcutsReference(const std::string& actionID)
{
    _imp->additionalShortcutsInTooltip.push_back(actionID);
}

const std::list<std::string>&
KnobHelper::getInViewerContextAdditionalShortcuts() const
{
    return _imp->additionalShortcutsInTooltip;
}

void
KnobHelper::setInViewerContextItemSpacing(int spacing)
{
    _imp->inViewerContextItemSpacing = spacing;
}

int
KnobHelper::getInViewerContextItemSpacing() const
{
    return _imp->inViewerContextItemSpacing;
}

void
KnobHelper::setInViewerContextLayoutType(ViewerContextLayoutTypeEnum layoutType)
{
    _imp->inViewerContextLayoutType = layoutType;
}

ViewerContextLayoutTypeEnum
KnobHelper::getInViewerContextLayoutType() const
{
    return _imp->inViewerContextLayoutType;
}

void
KnobHelper::setInViewerContextSecret(bool secret)
{
    {
        QMutexLocker k(&_imp->stateMutex);
        _imp->inViewerContextSecret = secret;
    }
    _signalSlotHandler->s_viewerContextSecretChanged();
}

bool
KnobHelper::getInViewerContextSecret() const
{
    QMutexLocker k(&_imp->stateMutex);

    return _imp->inViewerContextSecret;
}

void
KnobHelper::setEnabled(bool b,
                       DimSpec dimension)
{
    {
        QMutexLocker k(&_imp->stateMutex);
        if (dimension.isAll()) {
            for (std::size_t i = 0; i < _imp->enabled.size(); ++i) {
                _imp->enabled[i] = b;
            }
        } else {
            _imp->enabled[dimension] = b;
        }
    }
    _signalSlotHandler->s_enabledChanged();
}


void
KnobHelper::setSecret(bool b)
{
    {
        QMutexLocker k(&_imp->stateMutex);
        if (_imp->IsSecret == b) {
            return;
        }
        _imp->IsSecret = b;
    }

    ///the knob was revealed , refresh its gui to the current time
    if (!b) {
        KnobHolderPtr holder = getHolder();
        if (holder) {
            AppInstancePtr app = holder->getApp();
            if (app) {
                onTimeChanged( false, app->getTimeLine()->currentFrame() );
            }
        }
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_secretChanged();
    }
}

int
KnobHelper::determineHierarchySize() const
{
    int ret = 0;
    KnobIPtr current = getParentKnob();

    while (current) {
        ++ret;
        current = current->getParentKnob();
    }

    return ret;
}

std::string
KnobHelper::getLabel() const
{
    QMutexLocker k(&_imp->labelMutex);

    return _imp->label;
}

void
KnobHelper::setLabel(const std::string& label)
{
    {
        QMutexLocker k(&_imp->labelMutex);
        _imp->label = label;
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_labelChanged();
    }
}

void
KnobHelper::setIconLabel(const std::string& iconFilePath,
                         bool checked,
                         bool alsoSetViewerUIIcon)
{
    {
        QMutexLocker k(&_imp->labelMutex);
        int idx = !checked ? 0 : 1;

        _imp->iconFilePath[idx] = iconFilePath;
    }
    if (alsoSetViewerUIIcon) {
        setInViewerContextIconFilePath(iconFilePath, checked);
    }
}

const std::string&
KnobHelper::getIconLabel(bool checked) const
{
    QMutexLocker k(&_imp->labelMutex);
    int idx = !checked ? 0 : 1;

    if ( !_imp->iconFilePath[idx].empty() ) {
        return _imp->iconFilePath[idx];
    }
    int otherIdx = !checked ? 1 : 0;

    return _imp->iconFilePath[otherIdx];
}

bool
KnobHelper::hasAnimation() const
{
    QMutexLocker k(&_imp->curvesMutex);
    QMutexLocker k2(&_imp->expressionMutex);
    for (int i = 0; i < getNDimensions(); ++i) {
        const PerViewCurveMap& curvesPerView = _imp->curves[i];
        for (PerViewCurveMap::const_iterator it = curvesPerView.begin(); it!=curvesPerView.end(); ++it) {
            if (it->second->getKeyFramesCount() > 0) {
                return true;
            }
        }
        const ExprPerViewMap& exprPerView = _imp->expressions[i];
        for (ExprPerViewMap::const_iterator it = exprPerView.begin(); it!=exprPerView.end(); ++it) {
            if (!it->second.originalExpression.empty()) {
                return true;
            }
        }
    }

    return false;
}

static std::size_t
getMatchingParenthesisPosition(std::size_t openingParenthesisPos,
                               char openingChar,
                               char closingChar,
                               const std::string& str)
{
    assert(openingParenthesisPos < str.size() && str.at(openingParenthesisPos) == openingChar);

    int noOpeningParenthesisFound = 0;
    int i = openingParenthesisPos + 1;

    while ( i < (int)str.size() ) {
        if (str.at(i) == closingChar) {
            if (noOpeningParenthesisFound == 0) {
                break;
            } else {
                --noOpeningParenthesisFound;
            }
        } else if (str.at(i) == openingChar) {
            ++noOpeningParenthesisFound;
        }
        ++i;
    }
    if ( i >= (int)str.size() ) {
        return std::string::npos;
    }

    return i;
}

static void
extractParameters(std::size_t startParenthesis,
                  std::size_t endParenthesis,
                  const std::string& str,
                  std::vector<std::string>* params)
{
    std::size_t i = startParenthesis + 1;
    int insideParenthesis = 0;

    while (i < endParenthesis || insideParenthesis < 0) {
        std::string curParam;
        if (str.at(i) == '(') {
            ++insideParenthesis;
        } else if (str.at(i) == ')') {
            --insideParenthesis;
        }
        while ( i < str.size() && (str.at(i) != ',' || insideParenthesis > 0) ) {
            curParam.push_back( str.at(i) );
            ++i;
            if (str.at(i) == '(') {
                ++insideParenthesis;
            } else if (str.at(i) == ')') {
                if (insideParenthesis > 0) {
                    --insideParenthesis;
                } else {
                    break;
                }
            }
        }
        params->push_back(curParam);
    }
}

static bool
parseTokenFrom(int fromDim,
               int dimensionParamPos,
               bool returnsTuple,
               const std::string& str,
               const std::string& token,
               std::size_t inputPos,
               std::size_t *tokenStart,
               std::string* output)
{
    std::size_t pos;

//    std::size_t tokenSize;
    bool foundMatchingToken = false;

    while (!foundMatchingToken) {
        pos = str.find(token, inputPos);
        if (pos == std::string::npos) {
            return false;
        }

        *tokenStart = pos;
        pos += token.size();

        ///Find nearest opening parenthesis
        for (; pos < str.size(); ++pos) {
            if (str.at(pos) == '(') {
                foundMatchingToken = true;
                break;
            } else if (str.at(pos) != ' ') {
                //We didn't find a good token
                break;
            }
        }

        if ( pos >= str.size() ) {
            throw std::invalid_argument("Invalid expr");
        }

        if (!foundMatchingToken) {
            inputPos = pos;
        }
    }

    std::size_t endingParenthesis = getMatchingParenthesisPosition(pos, '(', ')',  str);
    if (endingParenthesis == std::string::npos) {
        throw std::invalid_argument("Invalid expr");
    }

    std::vector<std::string> params;
    ///If the function returns a tuple like get()[dimension], do not extract parameters
    if (!returnsTuple) {
        extractParameters(pos, endingParenthesis, str, &params);
    } else {
        //try to find the tuple
        std::size_t it = endingParenthesis + 1;
        while (it < str.size() && str.at(it) == ' ') {
            ++it;
        }
        if ( ( it < str.size() ) && (str.at(it) == '[') ) {
            ///we found a tuple
            std::size_t endingBracket = getMatchingParenthesisPosition(it, '[', ']',  str);
            if (endingBracket == std::string::npos) {
                throw std::invalid_argument("Invalid expr");
            }
            params.push_back( str.substr(it + 1, endingBracket - it - 1) );
        }
    }


    //The get() function does not always returns a tuple
    if ( params.empty() ) {
        params.push_back("-1");
    }
    if (dimensionParamPos == -1) {
        ++dimensionParamPos;
    }


    if ( (dimensionParamPos < 0) || ( (int)params.size() <= dimensionParamPos ) ) {
        throw std::invalid_argument("Invalid expr");
    }

    std::stringstream ss;
    /*
       When replacing the getValue() (or similar function) call by addAsDepdendencyOf
       the parameter prefixing the addAsDepdendencyOf will register itself its dimension params[dimensionParamPos] as a dependency of the expression
       at the "fromDim" dimension of thisParam
     */
    ss << ".addAsDependencyOf(" << fromDim << ",thisParam," <<  params[dimensionParamPos] << ")\n";
    std::string toInsert = ss.str();

    // tokenSize = endingParenthesis - tokenStart + 1;
    if ( (*tokenStart < 2) || (str[*tokenStart - 1] != '.') ) {
        throw std::invalid_argument("Invalid expr");
    }

    std::locale loc;
    //Find the start of the symbol
    int i = (int)*tokenStart - 2;
    int nClosingParenthesisMet = 0;
    while (i >= 0) {
        if (str[i] == ')') {
            ++nClosingParenthesisMet;
        }
        if ( std::isspace(str[i], loc) ||
             ( str[i] == '=') ||
             ( str[i] == '\n') ||
             ( str[i] == '\t') ||
             ( str[i] == '+') ||
             ( str[i] == '-') ||
             ( str[i] == '*') ||
             ( str[i] == '/') ||
             ( str[i] == '%') ||
             ( ( str[i] == '(') && !nClosingParenthesisMet ) ) {
            break;
        }
        --i;
    }
    ++i;
    std::string varName = str.substr(i, *tokenStart - 1 - i);
    output->append(varName + toInsert);

    //assert(*tokenSize > 0);
    return true;
} // parseTokenFrom

static bool
extractAllOcurrences(const std::string& str,
                     const std::string& token,
                     bool returnsTuple,
                     int dimensionParamPos,
                     int fromDim,
                     std::string *outputScript)
{
    std::size_t tokenStart;
    bool couldFindToken;

    try {
        couldFindToken = parseTokenFrom(fromDim, dimensionParamPos, returnsTuple, str, token, 0, &tokenStart, outputScript);
    } catch (...) {
        return false;
    }

    while (couldFindToken) {
        try {
            couldFindToken = parseTokenFrom(fromDim, dimensionParamPos, returnsTuple, str, token, tokenStart + 1, &tokenStart, outputScript);
        } catch (...) {
            return false;
        }
    }

    return true;
}

std::string
KnobHelperPrivate::declarePythonVariables(bool addTab,
                                          int dim)
{
    KnobHolderPtr h = holder.lock();
    if (!h) {
        throw std::runtime_error("This parameter cannot have an expression");
    }

    EffectInstancePtr effect = toEffectInstance(h);
    if (!effect) {
        throw std::runtime_error("This parameter cannot have an expression");
    }

    NodePtr node = effect->getNode();
    assert(node);

    NodeCollectionPtr collection = node->getGroup();
    if (!collection) {
        throw std::runtime_error("This parameter cannot have an expression");
    }
    NodeGroupPtr isParentGrp = toNodeGroup(collection);
    std::string appID = node->getApp()->getAppIDString();
    std::string tabStr = addTab ? "    " : "";
    std::stringstream ss;
    if (appID != "app") {
        ss << tabStr << "app = " << appID << "\n";
    }


    //Define all nodes reachable through expressions in the scope


    //Define all nodes in the same group reachable by their bare script-name
    NodesList siblings = collection->getNodes();
    std::string collectionScriptName;
    if (isParentGrp) {
        collectionScriptName = isParentGrp->getNode()->getFullyQualifiedName();
    } else {
        collectionScriptName = appID;
    }
    for (NodesList::iterator it = siblings.begin(); it != siblings.end(); ++it) {
        if ((*it)->isActivated()) {
            std::string scriptName = (*it)->getScriptName_mt_safe();
            std::string fullName = appID + "." + (*it)->getFullyQualifiedName();
            ss << tabStr << "if hasattr(";
            if (isParentGrp) {
                ss << appID << ".";
            }
            ss << collectionScriptName << ",\"" <<  scriptName << "\"):\n";

            ss << tabStr << "    " << scriptName << " = " << fullName << "\n";
        }
    }

    if (isParentGrp) {
        ss << tabStr << "thisGroup = " << appID << "." << collectionScriptName << "\n";
    } else {
        ss << tabStr << "thisGroup = " << appID << "\n";
    }
    ss << tabStr << "thisNode = " << node->getScriptName_mt_safe() <<  "\n";

    ///Now define the variables in the scope
    ss << tabStr << "thisParam = thisNode." << name << "\n";
    ss << tabStr << "random = thisParam.random\n";
    ss << tabStr << "randomInt = thisParam.randomInt\n";
    ss << tabStr << "curve = thisParam.curve\n";
    if (dimension != -1) {
        ss << tabStr << "dimension = " << dim << "\n";
    }

    //If this node is a group, also define all nodes inside the group, though they will be referencable via
    //thisNode.childname but also with <NodeName.childname>
    /*NodeGroupPtr isHolderGrp = toNodeGroup(effect);
    if (isHolderGrp) {
        NodesList children = isHolderGrp->getNodes();
        for (NodesList::iterator it = children.begin(); it != children.end(); ++it) {
            if ( (*it)->isActivated() && (*it)->isPersistent() ) {
                std::string scriptName = (*it)->getScriptName_mt_safe();
                std::string fullName = (*it)->getFullyQualifiedName();

                std::string nodeFullName = appID + "." + fullName;
                bool isAttrDefined;
                PyObject* obj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(nodeFullName, appPTR->getMainModule(), &isAttrDefined);
                Q_UNUSED(obj);
                if (isAttrDefined) {
                    ss << tabStr << "if hasattr(" << node->getScriptName_mt_safe() << ",\"" << scriptName << "\"):\n";
                    ss << tabStr << "    " << holderScriptName << "." << scriptName << " = " << nodeFullName << "\n";
                }
            }
        }
    }*/


    return ss.str();
} // KnobHelperPrivate::declarePythonVariables

void
KnobHelperPrivate::parseListenersFromExpression(int dimension)
{
    //Extract pointers to knobs referred to by the expression
    //Our heuristic is quite simple: we replace in the python code all calls to:
    // - getValue
    // - getValueAtTime
    // - getDerivativeAtTime
    // - getIntegrateFromTimeToTime
    // - get
    // And replace them by addAsDependencyOf(thisParam) which will register the parameters as a dependency of this parameter

    std::string expressionCopy;

    {
        QMutexLocker k(&expressionMutex);
        expressionCopy = expressions[dimension].originalExpression;
    }
    std::string script;

    if  ( !extractAllOcurrences(expressionCopy, "getValue", false, 0, dimension, &script) ) {
        return;
    }

    if ( !extractAllOcurrences(expressionCopy, "getValueAtTime", false, 1,  dimension, &script) ) {
        return;
    }

    if ( !extractAllOcurrences(expressionCopy, "getDerivativeAtTime", false, 1,  dimension, &script) ) {
        return;
    }

    if ( !extractAllOcurrences(expressionCopy, "getIntegrateFromTimeToTime", false, 2, dimension, &script) ) {
        return;
    }

    if ( !extractAllOcurrences(expressionCopy, "get", true, -1, dimension, &script) ) {
        return;
    }

    std::string declarations = declarePythonVariables(false, dimension);
    std::stringstream ss;
    ss << "frame=0\n";
    ss << "view=0\n";
    ss << declarations << '\n';
    ss << expressionCopy << '\n';
    ss << script;
    script = ss.str();
    ///This will register the listeners
    std::string error;
    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, NULL);
    if ( !error.empty() ) {
        qDebug() << error.c_str();
    }
    assert(ok);
    if (!ok) {
        throw std::runtime_error("KnobHelperPrivate::parseListenersFromExpression(): interpretPythonScript(" + script + ") failed!");
    }
} // KnobHelperPrivate::parseListenersFromExpression

std::string
KnobHelper::validateExpression(const std::string& expression,
                               int dimension,
                               bool hasRetVariable,
                               std::string* resultAsString)
{

#ifdef NATRON_RUN_WITHOUT_PYTHON
    throw std::invalid_argument("NATRON_RUN_WITHOUT_PYTHON is defined");
#endif
    PythonGILLocker pgl;

    if ( expression.empty() ) {
        throw std::invalid_argument("empty expression");;
    }


    std::string exprCpy = expression;

    //if !hasRetVariable the expression is expected to be single-line
    if (!hasRetVariable) {
        std::size_t foundNewLine = expression.find("\n");
        if (foundNewLine != std::string::npos) {
            throw std::invalid_argument("unexpected new line character \'\\n\'");
        }
        //preprend the line with "ret = ..."
        std::string toInsert("    ret = ");
        exprCpy.insert(0, toInsert);
    } else {
        exprCpy.insert(0, "    ");
        std::size_t foundNewLine = exprCpy.find("\n");
        while (foundNewLine != std::string::npos) {
            exprCpy.insert(foundNewLine + 1, "    ");
            foundNewLine = exprCpy.find("\n", foundNewLine + 1);
        }
    }

    KnobHolderPtr holder = getHolder();
    if (!holder) {
        throw std::runtime_error("This parameter cannot have an expression");
    }

    EffectInstancePtr effect = toEffectInstance(holder);
    if (!effect) {
        throw std::runtime_error("This parameter cannot have an expression");
    }

    NodePtr node = effect->getNode();
    assert(node);
    std::string appID = holder->getApp()->getAppIDString();
    std::string nodeName = node->getFullyQualifiedName();
    std::string nodeFullName = appID + "." + nodeName;
    std::string exprFuncPrefix = nodeFullName + "." + getName() + ".";
    std::string exprFuncName;
    {
        std::stringstream tmpSs;
        tmpSs << "expression" << dimension;
        exprFuncName = tmpSs.str();
    }

    exprCpy.append("\n    return ret\n");

    ///Now define the thisNode variable

    std::stringstream ss;
    ss << "def "  << exprFuncName << "(frame, view):\n";
    ss << _imp->declarePythonVariables(true, dimension);


    std::string script = ss.str();
    script.append(exprCpy);
    script.append(exprFuncPrefix + exprFuncName + " = " + exprFuncName);

    ///Try to compile the expression and evaluate it, if it doesn't have a good syntax, throw an exception
    ///with the error.
    std::string error;
    std::string funcExecScript = "ret = " + exprFuncPrefix + exprFuncName;

    {
        EXPR_RECURSION_LEVEL();

        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, 0) ) {
            throw std::runtime_error(error);
        }

        std::stringstream ss;
        ss << funcExecScript << '(' << getCurrentTime() << ", " <<  getCurrentView() << ")\n";
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(ss.str(), &error, 0) ) {
            throw std::runtime_error(error);
        }

        PyObject *ret = PyObject_GetAttrString(NATRON_PYTHON_NAMESPACE::getMainModule(), "ret"); //get our ret variable created above

        if ( !ret || PyErr_Occurred() ) {
#ifdef DEBUG
            PyErr_Print();
#endif
            throw std::runtime_error("return value must be assigned to the \"ret\" variable");
        }


        KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>(this);
        KnobIntBase* isInt = dynamic_cast<KnobIntBase*>(this);
        KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>(this);
        KnobStringBase* isString = dynamic_cast<KnobStringBase*>(this);
        if (isDouble) {
            double r = isDouble->pyObjectToType<double>(ret);
            *resultAsString = QString::number(r).toStdString();
        } else if (isInt) {
            int r = isInt->pyObjectToType<int>(ret);
            *resultAsString = QString::number(r).toStdString();
        } else if (isBool) {
            bool r = isBool->pyObjectToType<bool>(ret);
            *resultAsString = r ? "True" : "False";
        } else {
            assert(isString);
            *resultAsString = isString->pyObjectToType<std::string>(ret);
        }
    }


    return funcExecScript;
} // KnobHelper::validateExpression

bool
KnobHelper::checkInvalidExpressions()
{
    int ndims = getNDimensions();
    std::vector<std::pair<std::string, bool> > exprToReapply(ndims);
    {
        QMutexLocker k(&_imp->expressionMutex);
        for (int i = 0; i < ndims; ++i) {
            if ( !_imp->expressions[i].exprInvalid.empty() ) {
                exprToReapply[i] = std::make_pair(_imp->expressions[i].originalExpression, _imp->expressions[i].hasRet);
            }
        }
    }
    bool isInvalid = false;

    for (int i = 0; i < ndims; ++i) {
        if ( !exprToReapply[i].first.empty() ) {
            setExpressionInternal(i, exprToReapply[i].first, exprToReapply[i].second, true, false);
        }
        std::string err;
        if ( !isExpressionValid(i, &err) ) {
            isInvalid = true;
        }
    }

    return !isInvalid;
}

bool
KnobHelper::isExpressionValid(int dimension,
                              std::string* error) const
{
    int ndims = getNDimensions();

    if ( (dimension < 0) || (dimension >= ndims) ) {
        return false;
    }
    {
        QMutexLocker k(&_imp->expressionMutex);
        if (error) {
            *error = _imp->expressions[dimension].exprInvalid;
        }

        return _imp->expressions[dimension].exprInvalid.empty();
    }
}

void
KnobHelper::setExpressionInvalid(int dimension,
                                 bool valid,
                                 const std::string& error)
{
    int ndims = getNDimensions();

    if ( (dimension < 0) || (dimension >= ndims) ) {
        return;
    }
    bool wasValid;
    {
        QMutexLocker k(&_imp->expressionMutex);
        wasValid = _imp->expressions[dimension].exprInvalid.empty();
        _imp->expressions[dimension].exprInvalid = error;
    }
    KnobHolderPtr holder = getHolder();
    if ( holder && holder->getApp() ) {
        if (wasValid && !valid) {
            getHolder()->getApp()->addInvalidExpressionKnob( boost::const_pointer_cast<KnobI>( shared_from_this() ) );
            if (_signalSlotHandler) {
                _signalSlotHandler->s_expressionChanged(dimension);
            }
        } else if (!wasValid && valid) {
            bool haveOtherExprInvalid = false;
            {
                QMutexLocker k(&_imp->expressionMutex);
                for (int i = 0; i < ndims; ++i) {
                    if (i != dimension) {
                        if ( !_imp->expressions[dimension].exprInvalid.empty() ) {
                            haveOtherExprInvalid = true;
                            break;
                        }
                    }
                }
            }
            if (!haveOtherExprInvalid) {
                holder->getApp()->removeInvalidExpressionKnob( shared_from_this() );
            }
            if (_signalSlotHandler) {
                _signalSlotHandler->s_expressionChanged(dimension);
            }
        }
    }
} // KnobHelper::setExpressionInvalid

void
KnobHelper::setExpressionInternal(int dimension,
                                  const std::string& expression,
                                  bool hasRetVariable,
                                  bool clearResults,
                                  bool failIfInvalid)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    assert( dimension >= 0 && dimension < getNDimensions() );

    PythonGILLocker pgl;

    ///Clear previous expr
    clearExpression(dimension, clearResults);

    if ( expression.empty() ) {
        return;
    }

    std::string exprResult;
    std::string exprCpy;
    std::string exprInvalid;
    try {
        exprCpy = validateExpression(expression, dimension, hasRetVariable, &exprResult);
    } catch (const std::exception &e) {
        exprInvalid = e.what();
        exprCpy = expression;
        if (failIfInvalid) {
            throw std::invalid_argument(exprInvalid);
        }
    }

    //Set internal fields

    {
        QMutexLocker k(&_imp->expressionMutex);
        _imp->expressions[dimension].hasRet = hasRetVariable;
        _imp->expressions[dimension].expression = exprCpy;
        _imp->expressions[dimension].originalExpression = expression;
        _imp->expressions[dimension].exprInvalid = exprInvalid;

        ///This may throw an exception upon failure
        //NATRON_PYTHON_NAMESPACE::compilePyScript(exprCpy, &_imp->expressions[dimension].code);
    }

    KnobHolderPtr holder = getHolder();
    if (holder) {
        //Parse listeners of the expression, to keep track of dependencies to indicate them to the user.

        if ( exprInvalid.empty() ) {
            EXPR_RECURSION_LEVEL();
            _imp->parseListenersFromExpression(dimension);
        } else {
            AppInstancePtr app = holder->getApp();
            if (app) {
                app->addInvalidExpressionKnob( shared_from_this() );
            }
        }
    }


    //Notify the expr. has changed
    expressionChanged(dimension);
} // KnobHelper::setExpressionInternal

void
KnobHelper::replaceNodeNameInExpression(int dimension,
                                        const std::string& oldName,
                                        const std::string& newName)
{
    assert(dimension >= 0 && dimension < _imp->dimension);
    KnobHolderPtr holder = getHolder();
    if (!holder) {
        return;
    }
    EffectInstancePtr isEffect = toEffectInstance(holder);
    if (!isEffect) {
        return;
    }

    isEffect->beginChanges();
    std::string hasExpr = getExpression(dimension);
    if ( hasExpr.empty() ) {
        return;
    }
    bool hasRetVar = isExpressionUsingRetVariable(dimension);
    try {
        //Change in expressions the script-name
        QString estr = QString::fromUtf8( hasExpr.c_str() );
        estr.replace( QString::fromUtf8( oldName.c_str() ), QString::fromUtf8( newName.c_str() ) );
        hasExpr = estr.toStdString();
        setExpression(dimension, hasExpr, hasRetVar, false);
    } catch (...) {
    }

    isEffect->endChanges(true);
}

bool
KnobHelper::isExpressionUsingRetVariable(int dimension) const
{
    QMutexLocker k(&_imp->expressionMutex);

    return _imp->expressions[dimension].hasRet;
}

bool
KnobHelper::getExpressionDependencies(int dimension,
                                      std::list<std::pair<KnobIWPtr, int> >& dependencies) const
{
    QMutexLocker k(&_imp->expressionMutex);

    if ( !_imp->expressions[dimension].expression.empty() ) {
        dependencies = _imp->expressions[dimension].dependencies;

        return true;
    }

    return false;
}

void
KnobHelper::clearExpression(int dimension,
                            bool clearResults)
{
    PythonGILLocker pgl;
    bool hadExpression;
    {
        QMutexLocker k(&_imp->expressionMutex);
        hadExpression = !_imp->expressions[dimension].originalExpression.empty();
        _imp->expressions[dimension].expression.clear();
        _imp->expressions[dimension].originalExpression.clear();
        _imp->expressions[dimension].exprInvalid.clear();
        //Py_XDECREF(_imp->expressions[dimension].code); //< new ref
        //_imp->expressions[dimension].code = 0;
    }
    KnobIPtr thisShared = shared_from_this();
    {
        std::list<std::pair<KnobIWPtr, int> > dependencies;
        {
            QWriteLocker kk(&_imp->mastersMutex);
            dependencies = _imp->expressions[dimension].dependencies;
            _imp->expressions[dimension].dependencies.clear();
        }
        for (std::list<std::pair<KnobIWPtr, int> >::iterator it = dependencies.begin();
             it != dependencies.end(); ++it) {
            KnobIPtr otherKnob = it->first.lock();
            KnobHelper* other = dynamic_cast<KnobHelper*>( otherKnob.get() );
            if (!other) {
                continue;
            }
            ListenerDimsMap otherListeners;
            {
                QReadLocker otherMastersLocker(&other->_imp->mastersMutex);
                otherListeners = other->_imp->listeners;
            }

            for (ListenerDimsMap::iterator it = otherListeners.begin(); it != otherListeners.end(); ++it) {
                KnobIPtr knob = it->first.lock();
                if (knob.get() == this) {
                    //erase from the dimensions vector
                    assert( dimension < (int)it->second.size() );
                    it->second[dimension].isListening = false;

                    //if it has no longer has a reference to this knob, erase it
                    bool hasReference = false;
                    for (std::size_t d = 0; d < it->second.size(); ++d) {
                        if (it->second[d].isListening) {
                            hasReference = true;
                            break;
                        }
                    }
                    if (!hasReference) {
                        otherListeners.erase(it);
                    }

                    break;
                }
            }

            {
                KnobHolderPtr holder = getHolder();
                if (holder) {
                    holder->onKnobSlaved(thisShared, otherKnob, dimension, false );
                }
            }


            {
                QWriteLocker otherMastersLocker(&other->_imp->mastersMutex);
                other->_imp->listeners = otherListeners;
            }
        }
    }

    if (clearResults) {
        clearExpressionsResults(dimension);
    }

    if (hadExpression) {
        expressionChanged(dimension);
    }
} // KnobHelper::clearExpression

void
KnobHelper::expressionChanged(int dimension)
{
    KnobHolderPtr holder = getHolder();
    if (holder) {
        holder->updateHasAnimation();
    }

    if (_signalSlotHandler) {
        _signalSlotHandler->s_expressionChanged(dimension);
    }
    computeHasModifications();
}

static bool
catchErrors(PyObject* mainModule,
            std::string* error)
{
    if ( PyErr_Occurred() ) {
        PyErr_Print();
        ///Gui session, do stdout, stderr redirection
        if ( PyObject_HasAttrString(mainModule, "catchErr") ) {
            PyObject* errCatcher = PyObject_GetAttrString(mainModule, "catchErr"); //get our catchOutErr created above, new ref
            PyObject *errorObj = 0;
            if (errCatcher) {
                errorObj = PyObject_GetAttrString(errCatcher, "value"); //get the  stderr from our catchErr object, new ref
                assert(errorObj);
                *error = NATRON_PYTHON_NAMESPACE::PyStringToStdString(errorObj);
                PyObject* unicode = PyUnicode_FromString("");
                PyObject_SetAttrString(errCatcher, "value", unicode);
                Py_DECREF(errorObj);
                Py_DECREF(errCatcher);
            }
        }

        return false;
    }

    return true;
}

bool
KnobHelper::executeExpression(double time,
                              ViewIdx view,
                              int dimension,
                              PyObject** ret,
                              std::string* error) const
{
    std::string expr;
    {
        QMutexLocker k(&_imp->expressionMutex);
        expr = _imp->expressions[dimension].expression;
    }

    //returns a new ref, this function's documentation is not clear onto what it returns...
    //https://docs.python.org/2/c-api/veryhigh.html
    PyObject* mainModule = NATRON_PYTHON_NAMESPACE::getMainModule();
    PyObject* globalDict = PyModule_GetDict(mainModule);
    std::stringstream ss;

    ss << expr << '(' << time << ", " <<  view << ")\n";
    std::string script = ss.str();
    PyObject* v = PyRun_String(script.c_str(), Py_file_input, globalDict, 0);
    Py_XDECREF(v);

    *ret = 0;

    if ( !catchErrors(mainModule, error) ) {
        return false;
    }
    *ret = PyObject_GetAttrString(mainModule, "ret"); //get our ret variable created above
    if (!*ret) {
        *error = "Missing ret variable";

        return false;
    }
    if ( !catchErrors(mainModule, error) ) {
        return false;
    }

    return true;
}

std::string
KnobHelper::getExpression(int dimension) const
{
    if (dimension == -1) {
        dimension = 0;
    }
    QMutexLocker k(&_imp->expressionMutex);

    return _imp->expressions[dimension].originalExpression;
}

KnobHolderPtr
KnobHelper::getHolder() const
{
    return _imp->holder.lock();
}

void
KnobHelper::setAnimationEnabled(bool val)
{
    if ( !canAnimate() ) {
        return;
    }
    KnobHolderPtr holder = getHolder();
    if (holder && !holder->canKnobsAnimate() ) {
        return;
    }
    _imp->isAnimationEnabled = val;
}

bool
KnobHelper::isAnimationEnabled() const
{
    return canAnimate() && _imp->isAnimationEnabled;
}

void
KnobHelper::setName(const std::string & name,
                    bool throwExceptions)
{
    _imp->originalName = name;
    _imp->name = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(name);
    KnobHolderPtr holder = getHolder();

    if (!holder) {
        return;
    }
    //Try to find a duplicate
    int no = 1;
    bool foundItem;
    std::string finalName;
    do {
        std::stringstream ss;
        ss << _imp->name;
        if (no > 1) {
            ss << no;
        }
        finalName = ss.str();
        if ( holder->getOtherKnobByName( finalName, shared_from_this() ) ) {
            foundItem = true;
        } else {
            foundItem = false;
        }
        ++no;
    } while (foundItem);


    EffectInstancePtr effect = toEffectInstance(holder);
    if (effect) {
        NodePtr node = effect->getNode();
        std::string effectScriptName = node->getScriptName_mt_safe();
        if ( !effectScriptName.empty() ) {
            std::string newPotentialQualifiedName = node->getApp()->getAppIDString() +  node->getFullyQualifiedName();
            newPotentialQualifiedName += '.';
            newPotentialQualifiedName += finalName;

            bool isAttrDefined = false;
            PyObject* obj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(newPotentialQualifiedName, appPTR->getMainModule(), &isAttrDefined);
            Q_UNUSED(obj);
            if (isAttrDefined) {
                QString message = tr("A Python attribute with the name %1 already exists.").arg(QString::fromUtf8(newPotentialQualifiedName.c_str()));
                if (throwExceptions) {
                    throw std::runtime_error( message.toStdString() );
                } else {
                    appPTR->writeToErrorLog_mt_safe(QString::fromUtf8(getName().c_str()), QDateTime::currentDateTime(), message );
                    std::cerr << message.toStdString() << std::endl;

                    return;
                }
            }
        }
    }
    _imp->name = finalName;
} // KnobHelper::setName

const std::string &
KnobHelper::getName() const
{
    return _imp->name;
}

const std::string &
KnobHelper::getOriginalName() const
{
    return _imp->originalName;
}

void
KnobHelper::resetParent()
{
    KnobIPtr parent = _imp->parentKnob.lock();

    if (parent) {
        KnobGroupPtr isGrp =  toKnobGroup(parent);
        KnobPagePtr isPage = toKnobPage(parent);
        if (isGrp) {
            isGrp->removeKnob( shared_from_this() );
        } else if (isPage) {
            isPage->removeKnob( shared_from_this() );
        } else {
            assert(false);
        }
        _imp->parentKnob.reset();
    }
}

void
KnobHelper::setParentKnob(KnobIPtr knob)
{
    _imp->parentKnob = knob;
}

KnobIPtr
KnobHelper::getParentKnob() const
{
    return _imp->parentKnob.lock();
}

bool
KnobHelper::getIsSecret() const
{
    QMutexLocker k(&_imp->stateMutex);

    return _imp->IsSecret;
}

bool
KnobHelper::getIsSecretRecursive() const
{
    if ( getIsSecret() ) {
        return true;
    }
    KnobIPtr parent = getParentKnob();
    if (parent) {
        return parent->getIsSecretRecursive();
    }

    return false;
}

void
KnobHelper::setIsFrozen(bool frozen)
{
    if (_signalSlotHandler) {
        _signalSlotHandler->s_setFrozen(frozen);
    }
}

bool
KnobHelper::isEnabled(int dimension) const
{
    assert( 0 <= dimension && dimension < getNDimensions() );

    QMutexLocker k(&_imp->stateMutex);

    return _imp->enabled[dimension];
}


void
KnobHelper::setDirty(bool d)
{
    _signalSlotHandler->s_setDirty(d);
}

void
KnobHelper::setEvaluateOnChange(bool b)
{
    KnobPage* isPage = dynamic_cast<KnobPage*>(this);
    KnobGroup* isGrp = dynamic_cast<KnobGroup*>(this);

    if (isPage || isGrp) {
        b = false;
    }
    {
        QMutexLocker k(&_imp->stateMutex);
        _imp->evaluateOnChange = b;
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_evaluateOnChangeChanged(b);
    }
}

bool
KnobHelper::getIsPersistent() const
{
    return _imp->IsPersistent;
}

void
KnobHelper::setIsPersistent(bool b)
{
    _imp->IsPersistent = b;
}

void
KnobHelper::setCanUndo(bool val)
{
    _imp->CanUndo = val;
}

bool
KnobHelper::getCanUndo() const
{
    return _imp->CanUndo;
}

void
KnobHelper::setIsMetadataSlave(bool slave)
{
    _imp->isClipPreferenceSlave = slave;
}

bool
KnobHelper::getIsMetadataSlave() const
{
    return _imp->isClipPreferenceSlave;
}

bool
KnobHelper::getEvaluateOnChange() const
{
    QMutexLocker k(&_imp->stateMutex);

    return _imp->evaluateOnChange;
}

void
KnobHelper::setHintToolTip(const std::string & hint)
{
    _imp->tooltipHint = hint;
    if (_signalSlotHandler) {
        _signalSlotHandler->s_helpChanged();
    }
}

const std::string &
KnobHelper::getHintToolTip() const
{
    return _imp->tooltipHint;
}

bool
KnobHelper::isHintInMarkdown() const
{
    return _imp->hintIsMarkdown;
}

void
KnobHelper::setHintIsMarkdown(bool b)
{
    _imp->hintIsMarkdown = b;
}

void
KnobHelper::setCustomInteract(const OfxParamOverlayInteractPtr & interactDesc)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->customInteract = interactDesc;
}

OfxParamOverlayInteractPtr KnobHelper::getCustomInteract() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->customInteract;
}

void
KnobHelper::swapOpenGLBuffers()
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        hasGui->swapOpenGLBuffers();
    }
}

void
KnobHelper::redraw()
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        hasGui->redraw();
    }
}

void
KnobHelper::getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();
    if (hasGui) {
        hasGui->getOpenGLContextFormat(depthPerComponents, hasAlpha);
    } else {
        *depthPerComponents = 8;
        *hasAlpha = false;
    }
}

void
KnobHelper::getViewportSize(double &width,
                            double &height) const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        hasGui->getViewportSize(width, height);
    } else {
        width = 0;
        height = 0;
    }
}

void
KnobHelper::getPixelScale(double & xScale,
                          double & yScale) const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        hasGui->getPixelScale(xScale, yScale);
    } else {
        xScale = 0;
        yScale = 0;
    }
}

void
KnobHelper::getBackgroundColour(double &r,
                                double &g,
                                double &b) const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        hasGui->getBackgroundColour(r, g, b);
    } else {
        r = 0;
        g = 0;
        b = 0;
    }
}

int
KnobHelper::getWidgetFontHeight() const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        return hasGui->getWidgetFontHeight();
    }

    return 0;
}

int
KnobHelper::getStringWidthForCurrentFont(const std::string& string) const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        return hasGui->getStringWidthForCurrentFont(string);
    }

    return 0;
}

void
KnobHelper::toWidgetCoordinates(double *x,
                                double *y) const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        hasGui->toWidgetCoordinates(x, y);
    }
}

void
KnobHelper::toCanonicalCoordinates(double *x,
                                   double *y) const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        hasGui->toCanonicalCoordinates(x, y);
    }
}

RectD
KnobHelper::getViewportRect() const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        return hasGui->getViewportRect();
    } else {
        return RectD();
    }
}

void
KnobHelper::getCursorPosition(double& x,
                              double& y) const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        return hasGui->getCursorPosition(x, y);
    } else {
        x = 0;
        y = 0;
    }
}

void
KnobHelper::saveOpenGLContext()
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        hasGui->saveOpenGLContext();
    }
}

void
KnobHelper::restoreOpenGLContext()
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        hasGui->restoreOpenGLContext();
    }
}

bool
KnobI::shouldDrawOverlayInteract() const
{
    // If there is one dimension disabled, don't draw it
    int nDims = getNDimensions();
    for (int i = 0; i < nDims; ++i) {
        if (!isEnabled(i)) {
            return false;
        }
    }

    // If this knob is secret, don't draw it
    if (getIsSecretRecursive()) {
        return false;
    }
    
    KnobPagePtr page = getTopLevelPage();
    if (!page) {
        return false;
    }
    // Only draw overlays for knobs in the current page
    return page->isEnabled(0);
}

void
KnobHelper::setOfxParamHandle(void* ofxParamHandle)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->ofxParamHandle = ofxParamHandle;
}

void*
KnobHelper::getOfxParamHandle() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->ofxParamHandle;
}

bool
KnobHelper::isMastersPersistenceIgnored() const
{
    QReadLocker l(&_imp->mastersMutex);

    return _imp->ignoreMasterPersistence;
}

void
KnobHelper::copyAnimationToClipboard() const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        hasGui->copyAnimationToClipboard(-1);
    }
}

bool
KnobHelper::slaveToInternal(int dimension,
                            const KnobIPtr & other,
                            int otherDimension,
                            ValueChangedReasonEnum reason,
                            bool ignoreMasterPersistence)
{
    assert(other.get() != this);
    if (dimension < 0 || dimension >= (int)_imp->masters.size()) {
        return false;
    }
    assert( 0 <= dimension && dimension < (int)_imp->masters.size() );

    if (other->getMaster(otherDimension).second.get() == this) {
        //avoid recursion
        return false;
    }
    {
        QWriteLocker l(&_imp->mastersMutex);
        if ( _imp->masters[dimension].second.lock() ) {
            return false;
        }
        _imp->ignoreMasterPersistence = ignoreMasterPersistence;
        _imp->masters[dimension].second = other;
        _imp->masters[dimension].first = otherDimension;
    }

    KnobHelper* masterKnob = dynamic_cast<KnobHelper*>( other.get() );
    assert(masterKnob);
    if (!masterKnob) {
        return false;
    }

    if (masterKnob->_signalSlotHandler && _signalSlotHandler) {
        QObject::connect( masterKnob->_signalSlotHandler.get(), SIGNAL(curveAnimationChanged(std::list<double>, std::list<double>,ViewIdx,int,CurveChangeReason)),
                         _signalSlotHandler.get(), SLOT(onMasterCurveAnimationChanged(std::list<double>, std::list<double>,ViewIdx,int,CurveChangeReason)), Qt::UniqueConnection );
    }

    bool hasChanged = cloneAndCheckIfChanged(other, dimension, otherDimension);

    //Do not disable buttons when they are slaved
    KnobButton* isBtn = dynamic_cast<KnobButton*>(this);
    if (!isBtn) {
        setEnabled(dimension, false);
    }

    if (_signalSlotHandler) {
        ///Notify we want to refresh
        if (reason == eValueChangedReasonNatronInternalEdited) {
            _signalSlotHandler->s_knobSlaved(dimension, true);
        }
    }

    if (hasChanged) {
        evaluateValueChange(dimension, getCurrentTime(), ViewIdx(0), reason);
    } else {
        //just refresh the hasModifications flag so it gets serialized
        computeHasModifications();
        if (isBtn) {
            //force the aliased parameter to be persistent if it's not, otherwise it will not be saved
            isBtn->setIsPersistent(true);
        }
    }


    ///Register this as a listener of the master
    if (masterKnob) {
        masterKnob->addListener( false, dimension, otherDimension, shared_from_this() );
    }

    return true;
} // KnobHelper::slaveTo

bool
KnobHelper::getMaster(DimIdx dimension, MasterKnobLink* masterLink) const
{
    assert( dimension >= 0 && dimension < (int)_imp->masters.size() );
    QReadLocker l(&_imp->mastersMutex);
    std::pair<int, KnobIPtr > ret = std::make_pair( _imp->masters[dimension].first, _imp->masters[dimension].second.lock() );

    return ret;
}

void
KnobHelper::resetMaster(DimIdx dimension, ViewIdx view)
{
    assert(dimension >= 0);
    _imp->masters[dimension].second.reset();
    _imp->masters[dimension].first = -1;
    _imp->ignoreMasterPersistence = false;
}

bool
KnobHelper::isSlave(int dimension) const
{
    assert(dimension >= 0);
    QReadLocker l(&_imp->mastersMutex);

    return bool( _imp->masters[dimension].second.lock() );
}

void
KnobHelper::refreshAnimationLevel(ViewSpec view,
                                int dimension)
{
    bool changed = false;
    KnobHolderPtr holder = getHolder();

    for (int i = 0; i < _imp->dimension; ++i) {
        if ( (i == dimension) || (dimension == -1) ) {
            AnimationLevelEnum level = eAnimationLevelNone;


            if ( canAnimate() &&
                 isAnimated(i, view) &&
                 getExpression(i).empty() &&
                 holder && holder->getApp() ) {
                CurvePtr c = getCurve(view, i);
                double time = holder->getApp()->getTimeLine()->currentFrame();
                if (c->getKeyFramesCount() > 0) {
                    KeyFrame kf;
                    int nKeys = c->getNKeyFramesInRange(time, time + 1);
                    if (nKeys > 0) {
                        level = eAnimationLevelOnKeyframe;
                    } else {
                        level = eAnimationLevelInterpolatedValue;
                    }
                } else {
                    level = eAnimationLevelNone;
                }
            } else {
                level = eAnimationLevelNone;
            }
            {
                QMutexLocker l(&_imp->animationLevelMutex);
                assert( dimension < (int)_imp->animationLevel.size() );
                if (_imp->animationLevel[i] != level) {
                    changed = true;
                    _imp->animationLevel[i] = level;
                }
            }
        }
    }

    KnobGuiIPtr hasGui = getKnobGuiPointer();
    if ( changed && _signalSlotHandler && hasGui && !hasGui->isGuiFrozenForPlayback() ) {
        _signalSlotHandler->s_animationLevelChanged(view, dimension );
    }
}

AnimationLevelEnum
KnobHelper::getAnimationLevel(int dimension) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int, KnobIPtr > master = getMaster(dimension);

    if (master.second) {
        //Make sure it is refreshed
        master.second->refreshAnimationLevel(ViewSpec(0), master.first);

        return master.second->getAnimationLevel(master.first);
    }

    QMutexLocker l(&_imp->animationLevelMutex);
    if ( dimension > (int)_imp->animationLevel.size() ) {
        throw std::invalid_argument("Knob::getAnimationLevel(): Dimension out of range");
    }

    return _imp->animationLevel[dimension];
}

bool
KnobHelper::getKeyFrameTime(ViewSpec view,
                            int index,
                            int dimension,
                            double* time) const
{
    assert( 0 <= dimension && dimension < getNDimensions() );
    if ( !isAnimated(dimension, view) ) {
        return false;
    }
    CurvePtr curve = getCurve(view, dimension); //< getCurve will return the master's curve if any
    assert(curve);
    KeyFrame kf;
    bool ret = curve->getKeyFrameWithIndex(index, &kf);
    if (ret) {
        *time = kf.getTime();
    }

    return ret;
}

bool
KnobHelper::getLastKeyFrameTime(ViewSpec view,
                                int dimension,
                                double* time) const
{
    assert( 0 <= dimension && dimension < getNDimensions() );
    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return false;
    }

    CurvePtr curve = getCurve(view, dimension);  //< getCurve will return the master's curve if any
    assert(curve);
    *time = curve->getMaximumTimeCovered();

    return true;
}

bool
KnobHelper::getFirstKeyFrameTime(ViewSpec view,
                                 int dimension,
                                 double* time) const
{
    return getKeyFrameTime(view, 0, dimension, time);
}

int
KnobHelper::getKeyFramesCount(ViewSpec view,
                              int dimension) const
{
    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return 0;
    }

    CurvePtr curve = getCurve(view, dimension);  //< getCurve will return the master's curve if any
    assert(curve);

    return curve->getKeyFramesCount();   //< getCurve will return the master's curve if any
}

bool
KnobHelper::getNearestKeyFrameTime(ViewSpec view,
                                   int dimension,
                                   double time,
                                   double* nearestTime) const
{
    assert( 0 <= dimension && dimension < getNDimensions() );
    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return false;
    }

    CurvePtr curve = getCurve(view, dimension);  //< getCurve will return the master's curve if any
    assert(curve);
    KeyFrame kf;
    bool ret = curve->getNearestKeyFrameWithTime(time, &kf);
    if (ret) {
        *nearestTime = kf.getTime();
    }

    return ret;
}

int
KnobHelper::getKeyFrameIndex(ViewSpec view,
                             int dimension,
                             double time) const
{
    assert( 0 <= dimension && dimension < getNDimensions() );
    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return -1;
    }

    CurvePtr curve = getCurve(view, dimension);  //< getCurve will return the master's curve if any
    assert(curve);

    return curve->keyFrameIndex(time);
}

void
KnobHelper::refreshListenersAfterValueChange(ViewSpec view,
                                             ValueChangedReasonEnum reason,
                                             int dimension)
{
    ListenerDimsMap listeners;

    getListeners(listeners);

    if ( listeners.empty() ) {
        return;
    }

    double time = getCurrentTime();
    for (ListenerDimsMap::iterator it = listeners.begin(); it != listeners.end(); ++it) {
        KnobHelper* slaveKnob = dynamic_cast<KnobHelper*>( it->first.lock().get() );
        if (!slaveKnob) {
            continue;
        }


        std::set<int> dimensionsToEvaluate;
        bool mustClone = false;
        for (std::size_t i = 0; i < it->second.size(); ++i) {
            if ( it->second[i].isListening && ( (it->second[i].targetDim == dimension) || (it->second[i].targetDim == -1) || (dimension == -1) ) ) {
                dimensionsToEvaluate.insert(i);
                if (!it->second[i].isExpr) {
                    mustClone = true;
                }
            }
        }

        if ( dimensionsToEvaluate.empty() ) {
            continue;
        }

        int dimChanged;
        if (dimensionsToEvaluate.size() > 1) {
            dimChanged = -1;
        } else {
            dimChanged = *dimensionsToEvaluate.begin();
        }

        if (mustClone) {
            ///We still want to clone the master's dimension because otherwise we couldn't edit the curve e.g in the curve editor
            ///For example we use it for roto knobs where selected beziers have their knobs slaved to the gui knobs
            slaveKnob->clone(shared_from_this(), dimChanged);
        }

        slaveKnob->evaluateValueChangeInternal(dimChanged, time, view, eValueChangedReasonSlaveRefresh, reason);

        //call recursively
        if ( !slaveKnob->isListenersNotificationBlocked() ) {
            slaveKnob->refreshListenersAfterValueChange(view, reason, dimChanged);
        }
    } // for all listeners
} // KnobHelper::refreshListenersAfterValueChange

bool
KnobHelper::cloneExpressions(const KnobIPtr& other,
                             ViewIdx view,
                             int dimension,
                             int otherDimension)
{
    assert( (int)_imp->expressions.size() == getNDimensions() );
    assert( (dimension == otherDimension) || (dimension != -1) );
    assert(other);
    bool ret = false;
    try {
        int dims = std::min( getNDimensions(), other->getNDimensions() );
        for (int i = 0; i < dims; ++i) {
            if ( (i == dimension) || (dimension == -1) ) {
                std::string expr = other->getExpression(i, view);
                bool hasRet = other->isExpressionUsingRetVariable(i);
                if ( !expr.empty() && ( (expr != _imp->expressions[i].originalExpression) || (hasRet != _imp->expressions[i].hasRet) ) ) {
                    setExpression(i, expr, hasRet, false);
                    cloneExpressionsResults(other, i, otherDimension);
                    ret = true;
                }
            }
        }
    } catch (...) {
        ///ignore errors
    }

    return ret;
}

bool
KnobHelper::cloneExpressionsAndCheckIfChanged(const KnobIPtr& other,
                                              int dimension,
                                              int otherDimension)
{

}


//The knob in parameter will "listen" to this knob. Hence this knob is a dependency of the knob in parameter.
void
KnobHelper::addListener(const bool isExpression,
                        const int listenerDimension,
                        const int listenedToDimension,
                        const KnobIPtr& listener)
{
    assert( listenedToDimension == -1 || (listenedToDimension >= 0 && listenedToDimension < _imp->dimension) );
    KnobHelper* listenerIsHelper = dynamic_cast<KnobHelper*>( listener.get() );
    assert(listenerIsHelper);
    if (!listenerIsHelper) {
        return;
    }
    KnobIPtr thisShared = shared_from_this();
    if (listenerIsHelper->_signalSlotHandler && _signalSlotHandler) {
        //Notify the holder one of its knob is now slaved
        KnobHolderPtr holder = listenerIsHelper->getHolder();
        if (holder) {
            holder->onKnobSlaved(listener, thisShared, listenerDimension, true );
        }
    }

    // If this knob is already a dependency of the knob, add it to its dimension vector
    {
        QWriteLocker l(&_imp->mastersMutex);
        ListenerDimsMap::iterator foundListening = _imp->listeners.find(listener);
        if ( foundListening != _imp->listeners.end() ) {
            foundListening->second[listenerDimension].isListening = true;
            foundListening->second[listenerDimension].isExpr = isExpression;
            foundListening->second[listenerDimension].targetDim = listenedToDimension;
        } else {
            std::vector<ListenerDim>& dims = _imp->listeners[listener];
            dims.resize( listener->getNDimensions() );
            dims[listenerDimension].isListening = true;
            dims[listenerDimension].isExpr = isExpression;
            dims[listenerDimension].targetDim = listenedToDimension;
        }
    }

    if (isExpression) {
        QMutexLocker k(&listenerIsHelper->_imp->expressionMutex);
        assert(listenerDimension >= 0 && listenerDimension < listenerIsHelper->_imp->dimension);
        listenerIsHelper->_imp->expressions[listenerDimension].dependencies.push_back( std::make_pair(thisShared, listenedToDimension) );
    }
}

void
KnobHelper::removeListener(const KnobIPtr& listener,
                           int listenerDimension)
{
    KnobHelperPtr listenerHelper = boost::dynamic_pointer_cast<KnobHelper>(listener);

    assert(listenerHelper);

    QWriteLocker l(&_imp->mastersMutex);
    for (ListenerDimsMap::iterator it = _imp->listeners.begin(); it != _imp->listeners.end(); ++it) {
        if (it->first.lock() == listener) {
            it->second[listenerDimension].isListening = false;

            bool hasDimensionListening = false;
            for (std::size_t i = 0; i < it->second.size(); ++i) {
                if (it->second[listenerDimension].isListening) {
                    hasDimensionListening = true;
                    break;
                }
            }
            if (!hasDimensionListening) {
                _imp->listeners.erase(it);
            }
            break;
        }
    }
}

void
KnobHelper::getListeners(KnobI::ListenerDimsMap & listeners) const
{
    QReadLocker l(&_imp->mastersMutex);

    listeners = _imp->listeners;
}

double
KnobHelper::getCurrentTime() const
{
    KnobHolderPtr holder = getHolder();

    return holder && holder->getApp() ? holder->getCurrentTime() : 0;
}

ViewIdx
KnobHelper::getCurrentView() const
{
    KnobHolderPtr holder = getHolder();

    return ( holder && holder->getApp() ) ? holder->getCurrentView() : ViewIdx(0);
}


double
KnobHelper::random(double time,
                   unsigned int seed) const
{
    randomSeed(time, seed);

    return random();
}

double
KnobHelper::random(double min,
                   double max) const
{
    QMutexLocker k(&_imp->lastRandomHashMutex);

    _imp->lastRandomHash = hashFunction(_imp->lastRandomHash);

    return ( (double)_imp->lastRandomHash / (double)0x100000000LL ) * (max - min)  + min;
}

int
KnobHelper::randomInt(double time,
                      unsigned int seed) const
{
    randomSeed(time, seed);

    return randomInt();
}

int
KnobHelper::randomInt(int min,
                      int max) const
{
    return (int)random( (double)min, (double)max );
}

struct alias_cast_float
{
    alias_cast_float()
        : raw(0)
    {
    };                          // initialize to 0 in case sizeof(T) < 8

    union
    {
        U32 raw;
        float data;
    };
};

void
KnobHelper::randomSeed(double time,
                       unsigned int seed) const
{
    // Make the hash vary from seed
    U32 hash32 = seed;

    // Make the hash vary from time
    {
        alias_cast_float ac;
        ac.data = (float)time;
        hash32 += ac.raw;
    }

    QMutexLocker k(&_imp->lastRandomHashMutex);
    _imp->lastRandomHash = hash32;
}

bool
KnobHelper::hasModifications() const
{
    QMutexLocker k(&_imp->hasModificationsMutex);

    for (int i = 0; i < _imp->dimension; ++i) {
        if (_imp->hasModifications[i]) {
            return true;
        }
    }

    return false;
}

bool
KnobHelper::hasModifications(int dimension) const
{
    if ( (dimension < 0) || (dimension >= _imp->dimension) ) {
        throw std::invalid_argument("KnobHelper::hasModifications: Dimension out of range");
    }
    QMutexLocker k(&_imp->hasModificationsMutex);

    return _imp->hasModifications[dimension];
}

bool
KnobHelper::setHasModifications(int dimension,
                                bool value,
                                bool lock)
{
    assert(dimension >= 0 && dimension < _imp->dimension);
    bool ret;
    if (lock) {
        QMutexLocker k(&_imp->hasModificationsMutex);
        ret = _imp->hasModifications[dimension] != value;
        _imp->hasModifications[dimension] = value;
    } else {
        assert( !_imp->hasModificationsMutex.tryLock() );
        ret = _imp->hasModifications[dimension] != value;
        _imp->hasModifications[dimension] = value;
    }

    return ret;
}

void
KnobHolder::getUserPages(std::list<KnobPagePtr>& userPages) const {
    const KnobsVec& knobs = getKnobs();

    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ( (*it)->isUserKnob() ) {
            KnobPagePtr isPage = toKnobPage(*it);
            if (isPage) {
                userPages.push_back(isPage);
            }
        }
    }
}

KnobIPtr
KnobHelper::createDuplicateOnHolder(const KnobHolderPtr& otherHolder,
                                    const KnobPagePtr& page,
                                    const KnobGroupPtr& group,
                                    int indexInParent,
                                    bool makeAlias,
                                    const std::string& newScriptName,
                                    const std::string& newLabel,
                                    const std::string& newToolTip,
                                    bool refreshParams,
                                    bool isUserKnob)
{
    ///find-out to which node that master knob belongs to
    KnobHolderPtr holder = getHolder();
    if ( !holder || !holder->getApp() ) {
        return KnobIPtr();
    }

    EffectInstancePtr otherIsEffect = toEffectInstance(otherHolder);
    EffectInstancePtr isEffect = toEffectInstance(holder);
    KnobBool* isBool = dynamic_cast<KnobBool*>(this);
    KnobInt* isInt = dynamic_cast<KnobInt*>(this);
    KnobDouble* isDbl = dynamic_cast<KnobDouble*>(this);
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>(this);
    KnobColor* isColor = dynamic_cast<KnobColor*>(this);
    KnobString* isString = dynamic_cast<KnobString*>(this);
    KnobFile* isFile = dynamic_cast<KnobFile*>(this);
    KnobPath* isPath = dynamic_cast<KnobPath*>(this);
    KnobGroup* isGrp = dynamic_cast<KnobGroup*>(this);
    KnobPage* isPage = dynamic_cast<KnobPage*>(this);
    KnobButton* isBtn = dynamic_cast<KnobButton*>(this);
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>(this);


    //Ensure the group user page is created
    KnobPagePtr destPage;

    if (page) {
        destPage = page;
    } else {
        if (otherIsEffect) {
            std::list<KnobPagePtr> userPages;
            otherIsEffect->getUserPages(userPages);
            if (userPages.empty()) {
                destPage = otherIsEffect->getOrCreateUserPageKnob();
            } else {
                destPage = userPages.front();
            }

        }
    }

    KnobIPtr output;
    if (isBool) {
        KnobBoolPtr newKnob = otherHolder->createBoolKnob(newScriptName, newLabel, isUserKnob);
        output = newKnob;
    } else if (isInt) {
        KnobIntPtr newKnob = otherHolder->createIntKnob(newScriptName, newLabel, getNDimensions(), isUserKnob);
        newKnob->setMinimumsAndMaximums( isInt->getMinimums(), isInt->getMaximums() );
        newKnob->setDisplayMinimumsAndMaximums( isInt->getDisplayMinimums(), isInt->getDisplayMaximums() );
        if ( isInt->isSliderDisabled() ) {
            newKnob->disableSlider();
        }
        output = newKnob;
    } else if (isDbl) {
        KnobDoublePtr newKnob = otherHolder->createDoubleKnob(newScriptName, newLabel, getNDimensions(), isUserKnob);
        newKnob->setSpatial( isDbl->getIsSpatial() );
        if ( isDbl->isRectangle() ) {
            newKnob->setAsRectangle();
        }
        for (int i = 0; i < getNDimensions(); ++i) {
            newKnob->setValueIsNormalized( i, isDbl->getValueIsNormalized(i) );
        }
        if ( isDbl->isSliderDisabled() ) {
            newKnob->disableSlider();
        }
        newKnob->setMinimumsAndMaximums( isDbl->getMinimums(), isDbl->getMaximums() );
        newKnob->setDisplayMinimumsAndMaximums( isDbl->getDisplayMinimums(), isDbl->getDisplayMaximums() );
        output = newKnob;
    } else if (isChoice) {
        KnobChoicePtr newKnob = otherHolder->createChoiceKnob(newScriptName, newLabel, isUserKnob);
        if (!makeAlias) {
            newKnob->populateChoices( isChoice->getEntries_mt_safe(), isChoice->getEntriesHelp_mt_safe() );
        }
        output = newKnob;
    } else if (isColor) {
        KnobColorPtr newKnob = otherHolder->createColorKnob(newScriptName, newLabel, getNDimensions(), isUserKnob);
        newKnob->setMinimumsAndMaximums( isColor->getMinimums(), isColor->getMaximums() );
        newKnob->setDisplayMinimumsAndMaximums( isColor->getDisplayMinimums(), isColor->getDisplayMaximums() );
        output = newKnob;
    } else if (isString) {
        KnobStringPtr newKnob = otherHolder->createStringKnob(newScriptName, newLabel, isUserKnob);
        if ( isString->isLabel() ) {
            newKnob->setAsLabel();
        }
        if ( isString->isCustomKnob() ) {
            newKnob->setAsCustom();
        }
        if ( isString->isMultiLine() ) {
            newKnob->setAsMultiLine();
        }
        if ( isString->usesRichText() ) {
            newKnob->setUsesRichText(true);
        }
        output = newKnob;
    } else if (isFile) {
        KnobFilePtr newKnob = otherHolder->createFileKnob(newScriptName, newLabel, isUserKnob);
        newKnob->setDialogType(isFile->getDialogType());
        newKnob->setDialogFilters(isFile->getDialogFilters());
        output = newKnob;
    } else if (isPath) {
        KnobPathPtr newKnob = otherHolder->createPathKnob(newScriptName, newLabel, isUserKnob);
        if ( isPath->isMultiPath() ) {
            newKnob->setMultiPath(true);
        }
        output = newKnob;
    } else if (isGrp) {
        KnobGroupPtr newKnob = otherHolder->createGroupKnob(newScriptName, newLabel, isUserKnob);
        if ( isGrp->isTab() ) {
            newKnob->setAsTab();
        }
        output = newKnob;
    } else if (isPage) {
        KnobPagePtr newKnob = otherHolder->createPageKnob(newScriptName, newLabel, isUserKnob);
        output = newKnob;
    } else if (isBtn) {
        KnobButtonPtr newKnob = otherHolder->createButtonKnob(newScriptName, newLabel, isUserKnob);
        output = newKnob;
    } else if (isParametric) {
        KnobParametricPtr newKnob = otherHolder->createParametricKnob(newScriptName, newLabel, isParametric->getNDimensions(), isUserKnob);
        output = newKnob;
    }
    if (!output) {
        return KnobIPtr();
    }
    for (int i = 0; i < getNDimensions(); ++i) {
        output->setDimensionName( i, getDimensionName(i) );
    }
    output->setName(newScriptName, true);
    output->cloneDefaultValues( shared_from_this() );
    output->clone( shared_from_this() );
    if ( canAnimate() ) {
        output->setAnimationEnabled( isAnimationEnabled() );
    }
    output->setEvaluateOnChange( getEvaluateOnChange() );
    output->setHintToolTip(newToolTip);
    output->setAddNewLine(true);
    output->setHashingStrategy(getHashingStrategy());
    if (group) {
        if (indexInParent == -1) {
            group->addKnob(output);
        } else {
            group->insertKnob(indexInParent, output);
        }
    } else if (destPage) {
        if (indexInParent == -1) {
            destPage->addKnob(output);
        } else {
            destPage->insertKnob(indexInParent, output);
        }
    }
    if (isUserKnob && otherIsEffect) {
        otherIsEffect->getNode()->declarePythonKnobs();
    }
    if (!makeAlias && otherIsEffect && isEffect) {
        NodeCollectionPtr collec;
        collec = isEffect->getNode()->getGroup();

        NodeGroupPtr isCollecGroup = toNodeGroup(collec);
        std::stringstream ss;
        if (isCollecGroup) {
            ss << "thisGroup." << newScriptName;
        } else {
            ss << "app." << otherIsEffect->getNode()->getFullyQualifiedName() << "." << newScriptName;
        }
        if (output->getNDimensions() > 1) {
            ss << ".get()[dimension]";
        } else {
            ss << ".get()";
        }

        try {
            std::string script = ss.str();
            for (int i = 0; i < getNDimensions(); ++i) {
                clearExpression(i, true);
                setExpression(i, script, false, false);
            }
        } catch (...) {
        }
    } else {
        setKnobAsAliasOfThis(output, true);
    }
    if (refreshParams) {
        otherHolder->recreateUserKnobs(true);
    }

    return output;
} // KnobHelper::createDuplicateOnNode

bool
KnobI::areTypesCompatibleForSlave(const KnobIPtr& lhs,
                                  const KnobIPtr& rhs)
{
    if ( lhs->typeName() == rhs->typeName() ) {
        return true;
    }

    //These are compatible types
    KnobIntPtr lhsIsInt = toKnobInt(lhs);
    KnobIntPtr rhsIsInt = toKnobInt(rhs);
    KnobDoublePtr lhsIsDouble = toKnobDouble(lhs);
    KnobColorPtr lhsIsColor = toKnobColor(lhs);
    KnobDoublePtr rhsIsDouble = toKnobDouble(rhs);
    KnobColorPtr rhsIsColor = toKnobColor(rhs);

    //Knobs containing doubles are compatibles with knobs containing integers because the user might want to link values
    //stored in a double parameter to a int parameter and vice versa
    if ( (lhsIsDouble || lhsIsColor || lhsIsInt) && (rhsIsColor || rhsIsDouble || rhsIsInt) ) {
        return true;
    }

    /*  KnobChoicePtr lhsIsChoice = toKnobChoice(lhs);
       KnobChoicePtr rhsIsChoice = toKnobChoice(rhs);
       if (lhsIsChoice || rhsIsChoice) {
          return false;
       }
     */


    return false;
}

bool
KnobHelper::setKnobAsAliasOfThis(const KnobIPtr& master,
                                 bool doAlias)
{
    //Sanity check
    if ( !master || ( master->getNDimensions() != getNDimensions() ) ||
         ( master->typeName() != typeName() ) ) {
        return false;
    }

    /*
       For choices, copy exactly the menu entries because they have to be the same
     */
    if (doAlias) {
        master->onKnobAboutToAlias( shared_from_this() );
    }
    beginChanges();
    for (int i = 0; i < getNDimensions(); ++i) {
        if ( isSlave(i) ) {
            unSlave(i, false);
        }
        if (doAlias) {
            //master->clone(this, i);
            bool ok = slaveToInternal(i, master, i, eValueChangedReasonNatronInternalEdited, false);
            assert(ok);
            Q_UNUSED(ok);
        }
    }
    for (int i = 0; i < getNDimensions(); ++i) {
        master->setDimensionName( i, getDimensionName(i) );
    }
    handleSignalSlotsForAliasLink(master, doAlias);

    endChanges();

    QWriteLocker k(&_imp->mastersMutex);
    if (doAlias) {
        _imp->slaveForAlias = master;
    } else {
        _imp->slaveForAlias.reset();
    }
    master->computeHasModifications();

    return true;
}

KnobIPtr
KnobHelper::getAliasMaster()  const
{
    QReadLocker k(&_imp->mastersMutex);

    return _imp->slaveForAlias.lock();
}

void
KnobHelper::getAllExpressionDependenciesRecursive(std::set<NodePtr >& nodes) const
{
    std::set<KnobIPtr> deps;
    {
        QMutexLocker k(&_imp->expressionMutex);
        for (int i = 0; i < _imp->dimension; ++i) {
            for (std::list< std::pair<KnobIWPtr, int> >::const_iterator it = _imp->expressions[i].dependencies.begin();
                 it != _imp->expressions[i].dependencies.end(); ++it) {
                KnobIPtr knob = it->first.lock();
                if (knob) {
                    deps.insert(knob);
                }
            }
        }
    }
    {
        QReadLocker k(&_imp->mastersMutex);
        for (int i = 0; i < _imp->dimension; ++i) {
            KnobIPtr master = _imp->masters[i].second.lock();
            if (master) {
                if ( std::find(deps.begin(), deps.end(), master) == deps.end() ) {
                    deps.insert(master);
                }
            }
        }
    }
    std::list<KnobIPtr> knobsToInspectRecursive;

    for (std::set<KnobIPtr>::iterator it = deps.begin(); it != deps.end(); ++it) {
        EffectInstancePtr effect  = toEffectInstance( (*it)->getHolder() );
        if (effect) {
            NodePtr node = effect->getNode();

            nodes.insert(node);
            knobsToInspectRecursive.push_back(*it);
        }
    }


    for (std::list<KnobIPtr>::iterator it = knobsToInspectRecursive.begin(); it != knobsToInspectRecursive.end(); ++it) {
        (*it)->getAllExpressionDependenciesRecursive(nodes);
    }
}



static void
initializeValueSerializationStorage(const KnobIPtr& knob, const int dimension, ValueSerialization* serialization)
{
    serialization->_expression = knob->getExpression(dimension);
    serialization->_expresionHasReturnVariable = knob->isExpressionUsingRetVariable(dimension);

    bool gotValue = !serialization->_expression.empty();

    CurvePtr curve = knob->getCurve(ViewSpec::current(), dimension);
    if (curve && !gotValue) {
        curve->toSerialization(&serialization->_animationCurve);
        if (!serialization->_animationCurve.keys.empty()) {
            gotValue = true;
        }
    }

    if (!gotValue) {
        EffectInstancePtr isHolderEffect = toEffectInstance(knob->getHolder());
        bool isEffectCloned = false;
        if (isHolderEffect) {
            isEffectCloned = isHolderEffect->getNode()->getMasterNode().get() != 0;
        }

        std::pair< int, KnobIPtr > master = knob->getMaster(dimension);

        // Only serialize master link if:
        // - it exists and
        // - the knob wants the slave/master link to be persistent and
        // - the effect is not a clone of another one OR the master knob is an alias of this one
        if ( master.second && !knob->isMastersPersistenceIgnored() && (!isEffectCloned || knob->getAliasMaster())) {
            if (master.second->getNDimensions() > 1) {
                serialization->_slaveMasterLink.masterDimensionName = master.second->getDimensionName(master.first);
            }
            serialization->_slaveMasterLink.hasLink = true;
            gotValue = true;
            if (master.second != knob) {
                NamedKnobHolderPtr holder = boost::dynamic_pointer_cast<NamedKnobHolder>( master.second->getHolder() );
                assert(holder);

                TrackMarkerPtr isMarker = toTrackMarker(holder);
                if (isMarker) {
                    if (isMarker) {
                        serialization->_slaveMasterLink.masterTrackName = isMarker->getScriptName_mt_safe();
                        if (isMarker->getContext()->getNode()->getEffectInstance() != holder) {
                            serialization->_slaveMasterLink.masterNodeName = isMarker->getContext()->getNode()->getScriptName_mt_safe();
                        }
                    }
                } else {
                    // coverity[dead_error_line]
                    if (holder && holder != knob->getHolder()) {
                        // If the master knob is on  the group containing the node holding this knob
                        // then don't serialize the node name

                        EffectInstancePtr thisHolderIsEffect = toEffectInstance(knob->getHolder());
                        if (thisHolderIsEffect) {
                            NodeGroupPtr isGrp = toNodeGroup(thisHolderIsEffect->getNode()->getGroup());
                            if (isGrp && isGrp == holder) {
                                serialization->_slaveMasterLink.masterNodeName = kKnobMasterNodeIsGroup;
                            }
                        }
                        if (serialization->_slaveMasterLink.masterNodeName.empty()) {
                            serialization->_slaveMasterLink.masterNodeName = holder->getScriptName_mt_safe();
                        }
                    }
                }
                serialization->_slaveMasterLink.masterKnobName = master.second->getName();
            }
        }
    }

    KnobBoolBasePtr isBoolBase = toKnobBoolBase(knob);
    KnobIntPtr isInt = toKnobInt(knob);
    KnobBoolPtr isBool = toKnobBool(knob);
    KnobButtonPtr isButton = toKnobButton(knob);
    KnobDoubleBasePtr isDoubleBase = toKnobDoubleBase(knob);
    KnobDoublePtr isDouble = toKnobDouble(knob);
    KnobColorPtr isColor = toKnobColor(knob);
    KnobChoicePtr isChoice = toKnobChoice(knob);
    KnobStringBasePtr isStringBase = toKnobStringBase(knob);
    KnobParametricPtr isParametric = toKnobParametric(knob);
    KnobPagePtr isPage = toKnobPage(knob);
    KnobGroupPtr isGrp = toKnobGroup(knob);
    KnobSeparatorPtr isSep = toKnobSeparator(knob);
    KnobButtonPtr btn = toKnobButton(knob);


    if (isInt) {
        serialization->_type = ValueSerialization::eSerializationValueVariantTypeInteger;
        serialization->_defaultValue.isInt = isInt->getDefaultValue(dimension);
        serialization->_serializeDefaultValue = isInt->hasDefaultValueChanged(dimension);
    } else if (isBool || isGrp || isButton) {
        serialization->_type = ValueSerialization::eSerializationValueVariantTypeBoolean;
        serialization->_defaultValue.isBool = isBoolBase->getDefaultValue(dimension);
        serialization->_serializeDefaultValue = isBoolBase->hasDefaultValueChanged(dimension);
    } else if (isColor || isDouble) {
        serialization->_type = ValueSerialization::eSerializationValueVariantTypeDouble;
        serialization->_defaultValue.isDouble = isDoubleBase->getDefaultValue(dimension);
        serialization->_serializeDefaultValue = isDoubleBase->hasDefaultValueChanged(dimension);
    } else if (isStringBase) {
        serialization->_type = ValueSerialization::eSerializationValueVariantTypeString;
        serialization->_defaultValue.isString = isStringBase->getDefaultValue(dimension);
        serialization->_serializeDefaultValue = isStringBase->hasDefaultValueChanged(dimension);

    } else if (isChoice) {
        serialization->_type = ValueSerialization::eSerializationValueVariantTypeString;
        //serialization->_defaultValue.isString
        std::vector<std::string> entries = isChoice->getEntries_mt_safe();
        int defIndex = isChoice->getDefaultValue(dimension);
        std::string defValue;
        if (defIndex >= 0 && defIndex < (int)entries.size()) {
            defValue = entries[defIndex];
        }
        serialization->_defaultValue.isString = defValue;
        serialization->_serializeDefaultValue = isChoice->hasDefaultValueChanged(dimension);

    }

    serialization->_serializeValue = false;

    if (!gotValue) {

        if (isInt) {
            serialization->_value.isInt = isInt->getValue(dimension);
            serialization->_serializeValue = (serialization->_value.isInt != serialization->_defaultValue.isInt);
        } else if (isBool || isGrp || isButton) {
            serialization->_value.isBool = isBoolBase->getValue(dimension);
            serialization->_serializeValue = (serialization->_value.isBool != serialization->_defaultValue.isBool);
        } else if (isColor || isDouble) {
            serialization->_value.isDouble = isDoubleBase->getValue(dimension);
            serialization->_serializeValue = (serialization->_value.isDouble != serialization->_defaultValue.isDouble);
        } else if (isStringBase) {
            serialization->_value.isString = isStringBase->getValue(dimension);
            serialization->_serializeValue = (serialization->_value.isString != serialization->_defaultValue.isString);

        } else if (isChoice) {
            serialization->_value.isString = isChoice->getActiveEntryText_mt_safe();
            serialization->_serializeValue = (serialization->_value.isString != serialization->_defaultValue.isString);
        }
    }
    // Check if we need to serialize this dimension
    serialization->_mustSerialize = true;

    if (serialization->_expression.empty() && !serialization->_slaveMasterLink.hasLink && serialization->_animationCurve.keys.empty()  && !serialization->_serializeValue && !serialization->_serializeDefaultValue) {
        serialization->_mustSerialize = false;
    }

} // initializeValueSerializationStorage

void
KnobHelper::restoreValueFromSerialization(const SERIALIZATION_NAMESPACE::ValueSerialization& obj, int targetDimension, bool restoreDefaultValue)
{
    KnobIPtr thisShared = shared_from_this();
    KnobBoolBasePtr isBoolBase = toKnobBoolBase(thisShared);
    KnobIntPtr isInt = toKnobInt(thisShared);
    KnobBoolPtr isBool = toKnobBool(thisShared);
    KnobButtonPtr isButton = toKnobButton(thisShared);
    KnobDoubleBasePtr isDoubleBase = toKnobDoubleBase(thisShared);
    KnobDoublePtr isDouble = toKnobDouble(thisShared);
    KnobColorPtr isColor = toKnobColor(thisShared);
    KnobChoicePtr isChoice = toKnobChoice(thisShared);
    KnobStringBasePtr isStringBase = toKnobStringBase(thisShared);
    KnobPagePtr isPage = toKnobPage(thisShared);
    KnobGroupPtr isGrp = toKnobGroup(thisShared);
    KnobSeparatorPtr isSep = toKnobSeparator(thisShared);
    KnobButtonPtr btn = toKnobButton(thisShared);

    // We do the opposite of what is done in initializeValueSerializationStorage()
    if (isInt) {
        if (obj._serializeValue) {
            isInt->setValue(obj._value.isInt, ViewSpec::all(), targetDimension, eValueChangedReasonNatronInternalEdited, 0);
        }
        if (restoreDefaultValue) {
            if (obj._serializeValue) {
                isInt->setDefaultValueWithoutApplying(obj._defaultValue.isInt, targetDimension);
            } else {
                isInt->setDefaultValue(obj._defaultValue.isInt, targetDimension);
            }
        }

    } else if (isBool || isGrp || isButton) {
        assert(isBoolBase);
        if (obj._serializeValue) {
            isBoolBase->setValue(obj._value.isBool, ViewSpec::all(), targetDimension, eValueChangedReasonNatronInternalEdited, 0);
        }
        if (restoreDefaultValue) {
            if (obj._serializeValue) {
                isBoolBase->setDefaultValueWithoutApplying(obj._defaultValue.isBool, targetDimension);
            } else {
                isBoolBase->setDefaultValue(obj._defaultValue.isBool, targetDimension);
            }
        }
    } else if (isColor || isDouble) {
        assert(isDoubleBase);

        if (obj._serializeValue) {
            isDoubleBase->setValue(obj._value.isDouble, ViewSpec::all(), targetDimension, eValueChangedReasonNatronInternalEdited, 0);
        }
        if (restoreDefaultValue) {
            if (obj._serializeValue) {
                isDoubleBase->setDefaultValueWithoutApplying(obj._defaultValue.isDouble, targetDimension);
            } else {
                isDoubleBase->setDefaultValue(obj._defaultValue.isDouble, targetDimension);
            }
        }

    } else if (isStringBase) {

        if (obj._serializeValue) {
            isStringBase->setValue(obj._value.isString, ViewSpec::all(), targetDimension, eValueChangedReasonNatronInternalEdited, 0);
        }
        if (restoreDefaultValue) {
            if (obj._serializeValue) {
                isStringBase->setDefaultValueWithoutApplying(obj._defaultValue.isString, targetDimension);
            } else {
                isStringBase->setDefaultValue(obj._defaultValue.isString, targetDimension);
            }
        }
    } else if (isChoice) {
        int foundValue = -1;
        int foundDefault = -1;
        std::vector<std::string> entries = isChoice->getEntries_mt_safe();
        for (std::size_t i = 0; i < entries.size(); ++i) {
            if (foundValue == -1 && obj._serializeValue && boost::iequals(entries[i], obj._value.isString) ) {
                foundValue = i;
            } else if (foundDefault == -1 && restoreDefaultValue && boost::iequals(entries[i], obj._defaultValue.isString)) {
                foundDefault = i;
            }
        }

        if (obj._serializeValue) {
            if (foundValue == -1) {
                // Just remember the active entry if not found
                isChoice->setActiveEntry(obj._value.isString);
            } else {
                isChoice->setValue(foundValue, ViewSpec::all(), targetDimension, eValueChangedReasonNatronInternalEdited, 0);
            }
         }
        if (foundDefault != -1) {
            if (obj._serializeValue) {
                isChoice->setDefaultValueWithoutApplying(foundDefault);
            } else {
                isChoice->setDefaultValue(foundDefault);
            }
        }
    }

}

void
KnobHelper::toSerialization(SerializationObjectBase* serializationBase)
{

    SERIALIZATION_NAMESPACE::KnobSerialization* serialization = dynamic_cast<SERIALIZATION_NAMESPACE::KnobSerialization*>(serializationBase);
    SERIALIZATION_NAMESPACE::GroupKnobSerialization* groupSerialization = dynamic_cast<SERIALIZATION_NAMESPACE::GroupKnobSerialization*>(serializationBase);
    assert(serialization || groupSerialization);
    if (!serialization && !groupSerialization) {
        return;
    }

    if (groupSerialization) {
        KnobGroup* isGrp = dynamic_cast<KnobGroup*>(this);
        KnobPage* isPage = dynamic_cast<KnobPage*>(this);

        assert(isGrp || isPage);

        groupSerialization->_typeName = typeName();
        groupSerialization->_name = getName();
        groupSerialization->_label = getLabel();
        groupSerialization->_secret = getIsSecret();

        if (isGrp) {
            groupSerialization->_isSetAsTab = isGrp->isTab();
            groupSerialization->_isOpened = isGrp->getValue();
        }

        KnobsVec children;

        if (isGrp) {
            children = isGrp->getChildren();
        } else if (isPage) {
            children = isPage->getChildren();
        }
        for (std::size_t i = 0; i < children.size(); ++i) {
            const KnobIPtr& child = children[i];
            if (isPage) {
                // If page, check that the child is a top level child and not child of a sub-group
                // otherwise let the sub group register the child
                KnobIPtr parent = child->getParentKnob();
                if (parent.get() != isPage) {
                    continue;
                }
            }
            KnobGroupPtr isGrp = toKnobGroup(child);
            if (isGrp) {
                boost::shared_ptr<GroupKnobSerialization> childSer( new GroupKnobSerialization );
                isGrp->toSerialization(childSer.get());
                groupSerialization->_children.push_back(childSer);
            } else {
                //KnobChoicePtr isChoice = toKnobChoice(children[i].get());
                //bool copyKnob = false;//isChoice != NULL;
                KnobSerializationPtr childSer( new KnobSerialization );
                child->toSerialization(childSer.get());
                assert(childSer->_isUserKnob);
                groupSerialization->_children.push_back(childSer);
            }
        }
    } else {


        KnobIPtr thisShared = shared_from_this();

        serialization->_typeName = typeName();
        serialization->_dimension = getNDimensions();
        serialization->_scriptName = getName();

        serialization->_isUserKnob = isUserKnob() && !isDeclaredByPlugin();

        bool isFullRecoverySave = appPTR->getCurrentSettings()->getIsFullRecoverySaveModeEnabled();


        // Values bits
        serialization->_values.resize(serialization->_dimension);
        for (int i = 0; i < serialization->_dimension; ++i) {
            serialization->_values[i]._serialization = serialization;
            serialization->_values[i]._dimension = i;
            initializeValueSerializationStorage(thisShared, i, &serialization->_values[i]);

            // Force default value serialization in those cases
            if (serialization->_isUserKnob || isFullRecoverySave) {
                serialization->_values[i]._serializeDefaultValue = true;
                serialization->_values[i]._mustSerialize = true;
            }
        }

        serialization->_masterIsAlias = getAliasMaster().get() != 0;

        // User knobs bits
        serialization->_label = getLabel();
        serialization->_triggerNewLine = isNewLineActivated();
        serialization->_evaluatesOnChange = getEvaluateOnChange();
        serialization->_isPersistent = getIsPersistent();
        serialization->_animatesChanged = (isAnimationEnabled() != isAnimatedByDefault());
        serialization->_tooltip = getHintToolTip();
        serialization->_iconFilePath[0] = getIconLabel(false);
        serialization->_iconFilePath[1] = getIconLabel(true);

        if (serialization->_isUserKnob) {
            serialization->_isSecret = getIsSecret();

            int nDimsDisabled = 0;
            for (int i = 0; i < serialization->_dimension; ++i) {
                // If the knob is slaved, it will be disabled so do not take it into account
                if (!isSlave(i) && (!isEnabled(i))) {
                    ++nDimsDisabled;
                }
            }
            if (nDimsDisabled == serialization->_dimension) {
                serialization->_disabled = true;
            }
        }


        // Viewer UI context bits
        if (getHolder()) {
            if (getHolder()->getInViewerContextKnobIndex(thisShared) != -1) {
                serialization->_hasViewerInterface = true;
                serialization->_inViewerContextItemSpacing = getInViewerContextItemSpacing();
                ViewerContextLayoutTypeEnum layout = getInViewerContextLayoutType();
                switch (layout) {
                    case eViewerContextLayoutTypeAddNewLine:
                        serialization->_inViewerContextItemLayout = kInViewerContextItemLayoutNewLine;
                        break;
                    case eViewerContextLayoutTypeSeparator:
                        serialization->_inViewerContextItemLayout = kInViewerContextItemLayoutAddSeparator;
                        break;
                    case eViewerContextLayoutTypeStretchAfter:
                        serialization->_inViewerContextItemLayout = kInViewerContextItemLayoutStretchAfter;
                        break;
                    case eViewerContextLayoutTypeStretchBefore:
                        serialization->_inViewerContextItemLayout = kInViewerContextItemLayoutStretchBefore;
                        break;
                    case eViewerContextLayoutTypeSpacing:
                        serialization->_inViewerContextItemLayout.clear();
                        break;
                }
                serialization->_inViewerContextSecret = getInViewerContextSecret();
                if (serialization->_isUserKnob) {
                    serialization->_inViewerContextLabel = getInViewerContextLabel();
                    serialization->_inViewerContextIconFilePath[0] = getInViewerContextIconFilePath(false);
                    serialization->_inViewerContextIconFilePath[1] = getInViewerContextIconFilePath(true);

                }
            }
        }

        // Per-type specific data
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>(this);
        if (isChoice) {
            ChoiceExtraData* extraData = new ChoiceExtraData;
            extraData->_entries = isChoice->getEntries_mt_safe();
            extraData->_helpStrings = isChoice->getEntriesHelp_mt_safe();
            serialization->_extraData.reset(extraData);
        }
        KnobParametric* isParametric = dynamic_cast<KnobParametric*>(this);
        if (isParametric) {
            ParametricExtraData* extraData = new ParametricExtraData;
            isParametric->saveParametricCurves(&extraData->parametricCurves);
            serialization->_extraData.reset(extraData);
        }
        KnobString* isString = dynamic_cast<KnobString*>(this);
        if (isString) {
            TextExtraData* extraData = new TextExtraData;
            isString->getStringAnimation()->save(&extraData->keyframes);
            serialization->_extraData.reset(extraData);
            extraData->fontFamily = isString->getFontFamily();
            extraData->fontSize = isString->getFontSize();
            isString->getFontColor(&extraData->fontColor[0], &extraData->fontColor[1], &extraData->fontColor[2]);
            extraData->italicActivated = isString->getItalicActivated();
            extraData->boldActivated = isString->getBoldActivated();
        }
        if (serialization->_isUserKnob) {
            if (isString) {
                TextExtraData* extraData = dynamic_cast<TextExtraData*>(serialization->_extraData.get());
                assert(extraData);
                extraData->label = isString->isLabel();
                extraData->multiLine = isString->isMultiLine();
                extraData->richText = isString->usesRichText();
            }
            KnobDouble* isDbl = dynamic_cast<KnobDouble*>(this);
            KnobInt* isInt = dynamic_cast<KnobInt*>(this);
            KnobColor* isColor = dynamic_cast<KnobColor*>(this);
            if (isDbl || isInt || isColor) {
                ValueExtraData* extraData = new ValueExtraData;
                if (isDbl) {
                    extraData->useHostOverlayHandle = serialization->_dimension == 2 && isDbl->getHasHostOverlayHandle();
                    extraData->min = isDbl->getMinimum();
                    extraData->max = isDbl->getMaximum();
                    extraData->dmin = isDbl->getDisplayMinimum();
                    extraData->dmax = isDbl->getDisplayMaximum();
                } else if (isInt) {
                    extraData->min = isInt->getMinimum();
                    extraData->max = isInt->getMaximum();
                    extraData->dmin = isInt->getDisplayMinimum();
                    extraData->dmax = isInt->getDisplayMaximum();
                } else if (isColor) {
                    extraData->min = isColor->getMinimum();
                    extraData->max = isColor->getMaximum();
                    extraData->dmin = isColor->getDisplayMinimum();
                    extraData->dmax = isColor->getDisplayMaximum();
                }
                serialization->_extraData.reset(extraData);
            }

            KnobFile* isFile = dynamic_cast<KnobFile*>(this);
            if (isFile) {
                FileExtraData* extraData = new FileExtraData;
                extraData->useSequences = isFile->getDialogType() == KnobFile::eKnobFileDialogTypeOpenFileSequences ||
                isFile->getDialogType() == KnobFile::eKnobFileDialogTypeSaveFileSequences;
                serialization->_extraData.reset(extraData);
            }

            KnobPath* isPath = dynamic_cast<KnobPath*>(this);
            if (isPath) {
                PathExtraData* extraData = new PathExtraData;
                extraData->multiPath = isPath->isMultiPath();
                serialization->_extraData.reset(extraData);
            }
        }

        // Check if we need to serialize this knob
        serialization->_mustSerialize = true;
        if (!serialization->_isUserKnob && !serialization->_masterIsAlias && !serialization->_hasViewerInterface) {
            bool mustSerialize = false;
            for (std::size_t i = 0; i < serialization->_values.size(); ++i) {
                mustSerialize |= serialization->_values[i]._mustSerialize;
            }

            if (!mustSerialize) {
                // Check if there are extra data
                {
                    const TextExtraData* data = dynamic_cast<const TextExtraData*>(serialization->_extraData.get());
                    if (data) {
                        if (!data->keyframes.empty() || data->fontFamily != NATRON_FONT || data->fontSize != KnobString::getDefaultFontPointSize() || data->fontColor[0] != 0 || data->fontColor[1] != 0 || data->fontColor[2] != 0) {
                            mustSerialize = true;
                        }
                    }
                }
                {
                    const ParametricExtraData* data = dynamic_cast<const ParametricExtraData*>(serialization->_extraData.get());
                    if (data) {
                        if (!data->parametricCurves.empty()) {
                            mustSerialize = true;
                        }
                    }
                }

            }
            serialization->_mustSerialize = mustSerialize;
        }
    } // groupSerialization
} // KnobHelper::toSerialization


void
KnobHelper::fromSerialization(const SerializationObjectBase& serializationBase)
{
    // We allow non persistent knobs to be loaded if we found a valid serialization for them
    const SERIALIZATION_NAMESPACE::KnobSerialization* serialization = dynamic_cast<const SERIALIZATION_NAMESPACE::KnobSerialization*>(&serializationBase);
    assert(serialization);
    if (!serialization) {
        return;
    }

    // Block any instance change action call when loading a knob
    blockValueChanges();
    beginChanges();



    // Restore extra datas
    KnobFile* isInFile = dynamic_cast<KnobFile*>(this);
    KnobString* isString = dynamic_cast<KnobString*>(this);
    if (isString) {
        const TextExtraData* data = dynamic_cast<const TextExtraData*>(serialization->_extraData.get());
        if (data) {
            isString->loadAnimation(data->keyframes);
            isString->setFontColor(data->fontColor[0], data->fontColor[1], data->fontColor[2]);
            isString->setFontFamily(data->fontFamily);
            isString->setFontSize(std::max(data->fontSize,1));
            isString->setItalicActivated(data->italicActivated);
            isString->setBoldActivated(data->boldActivated);
        }

    }

    // Load parametric parameter's curves
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>(this);
    if (isParametric) {
        const ParametricExtraData* data = dynamic_cast<const ParametricExtraData*>(serialization->_extraData.get());
        if (data) {
            isParametric->loadParametricCurves(data->parametricCurves);
        }
    }


    // Restore user knobs bits
    if (serialization->_isUserKnob) {
        setAsUserKnob(true);
        if (serialization->_isSecret) {
            setSecret(true);
        }
        // Restore enabled state
        if (serialization->_disabled) {
            setAllDimensionsEnabled(false);
        }
        setIsPersistent(serialization->_isPersistent);
        if (serialization->_animatesChanged) {
            setAnimationEnabled(!isAnimatedByDefault());
        }
        setEvaluateOnChange(serialization->_evaluatesOnChange);
        setName(serialization->_scriptName);
        setHintToolTip(serialization->_tooltip);
        setAddNewLine(serialization->_triggerNewLine);
        setIconLabel(serialization->_iconFilePath[0], false);
        setIconLabel(serialization->_iconFilePath[1], true);

        KnobInt* isInt = dynamic_cast<KnobInt*>(this);
        KnobDouble* isDouble = dynamic_cast<KnobDouble*>(this);
        KnobColor* isColor = dynamic_cast<KnobColor*>(this);
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>(this);
        KnobPath* isPath = dynamic_cast<KnobPath*>(this);

        int nDims = std::min( getNDimensions(), serialization->_dimension );

        if (isInt) {
            const ValueExtraData* data = dynamic_cast<const ValueExtraData*>(serialization->_extraData.get());
            assert(data);
            if (data) {
                std::vector<int> minimums, maximums, dminimums, dmaximums;
                for (int i = 0; i < nDims; ++i) {
                    minimums.push_back(data->min);
                    maximums.push_back(data->max);
                    dminimums.push_back(data->dmin);
                    dmaximums.push_back(data->dmax);
                }
                isInt->setMinimumsAndMaximums(minimums, maximums);
                isInt->setDisplayMinimumsAndMaximums(dminimums, dmaximums);
            }
        } else if (isDouble) {
            const ValueExtraData* data = dynamic_cast<const ValueExtraData*>(serialization->_extraData.get());
            assert(data);
            if (data) {
                std::vector<double> minimums, maximums, dminimums, dmaximums;
                for (int i = 0; i < nDims; ++i) {
                    minimums.push_back(data->min);
                    maximums.push_back(data->max);
                    dminimums.push_back(data->dmin);
                    dmaximums.push_back(data->dmax);
                }
                isDouble->setMinimumsAndMaximums(minimums, maximums);
                isDouble->setDisplayMinimumsAndMaximums(dminimums, dmaximums);
                if (data->useHostOverlayHandle) {
                    isDouble->setHasHostOverlayHandle(true);
                }
            }

        } else if (isChoice) {
            const ChoiceExtraData* data = dynamic_cast<const ChoiceExtraData*>(serialization->_extraData.get());
            if (data) {
                isChoice->populateChoices(data->_entries, data->_helpStrings);
            }
        } else if (isColor) {
            const ValueExtraData* data = dynamic_cast<const ValueExtraData*>(serialization->_extraData.get());
            if (data) {
                std::vector<double> minimums, maximums, dminimums, dmaximums;
                for (int i = 0; i < nDims; ++i) {
                    minimums.push_back(data->min);
                    maximums.push_back(data->max);
                    dminimums.push_back(data->dmin);
                    dmaximums.push_back(data->dmax);
                }
                isColor->setMinimumsAndMaximums(minimums, maximums);
                isColor->setDisplayMinimumsAndMaximums(dminimums, dmaximums);
            }
        } else if (isString) {
            const TextExtraData* data = dynamic_cast<const TextExtraData*>(serialization->_extraData.get());
            if (data) {
                if (data->label) {
                    isString->setAsLabel();
                } else if (data->multiLine) {
                    isString->setAsMultiLine();
                    if (data->richText) {
                        isString->setUsesRichText(true);
                    }
                }
            }

        } else if (isInFile) {
            const FileExtraData* data = dynamic_cast<const FileExtraData*>(serialization->_extraData.get());
            if (data) {
                if (data->useExistingFiles) {
                    if (data->useSequences) {
                        isInFile->setDialogType(KnobFile::eKnobFileDialogTypeOpenFileSequences);
                    } else {
                        isInFile->setDialogType(KnobFile::eKnobFileDialogTypeOpenFile);
                    }
                } else {
                    if (data->useSequences) {
                        isInFile->setDialogType(KnobFile::eKnobFileDialogTypeSaveFileSequences);
                    } else {
                        isInFile->setDialogType(KnobFile::eKnobFileDialogTypeSaveFile);
                    }
                }
                isInFile->setDialogFilters(data->filters);
            }
        } else if (isPath) {
            const PathExtraData* data = dynamic_cast<const PathExtraData*>(serialization->_extraData.get());
            if (data && data->multiPath) {
                isPath->setMultiPath(true);
            }
        }

    } // isUserKnob


    // There is a case where the dimension of a parameter might have changed between versions, e.g:
    // the size parameter of the Blur node was previously a Double1D and has become a Double2D to control
    // both dimensions.
    // For compatibility, we do not load only the first dimension, otherwise the result wouldn't be the same,
    // instead we replicate the last dimension of the serialized knob to all other remaining dimensions to fit the
    // knob's dimensions.
    for (std::size_t d = 0; d < serialization->_values.size(); ++d) {
        int dimensionIndex = serialization->_values[d]._dimension;

        // Clone animation
        if (!serialization->_values[d]._animationCurve.keys.empty()) {
            CurvePtr curve = getCurve(ViewIdx(0), dimensionIndex);
            if (curve) {
                curve->fromSerialization(serialization->_values[d]._animationCurve);
            }
        } else if (serialization->_values[d]._expression.empty() && !serialization->_values[d]._slaveMasterLink.hasLink) {
            restoreValueFromSerialization(serialization->_values[d], dimensionIndex, serialization->_values[d]._serializeDefaultValue);
        }
        
    } // for all dims


    // Restore viewer UI context
    if (serialization->_hasViewerInterface) {
        setInViewerContextItemSpacing(serialization->_inViewerContextItemSpacing);
        ViewerContextLayoutTypeEnum layoutType = eViewerContextLayoutTypeSpacing;
        if (serialization->_inViewerContextItemLayout == kInViewerContextItemLayoutNewLine) {
            layoutType = eViewerContextLayoutTypeAddNewLine;
        } else if (serialization->_inViewerContextItemLayout == kInViewerContextItemLayoutStretchAfter) {
            layoutType = eViewerContextLayoutTypeStretchAfter;
        } else if (serialization->_inViewerContextItemLayout == kInViewerContextItemLayoutStretchBefore) {
            layoutType = eViewerContextLayoutTypeStretchBefore;
        } else if (serialization->_inViewerContextItemLayout == kInViewerContextItemLayoutAddSeparator) {
            layoutType = eViewerContextLayoutTypeSeparator;
        }
        setInViewerContextLayoutType(layoutType);
        setInViewerContextSecret(serialization->_inViewerContextSecret);
        if (isUserKnob()) {
            setInViewerContextLabel(QString::fromUtf8(serialization->_inViewerContextLabel.c_str()));
            setInViewerContextIconFilePath(serialization->_inViewerContextIconFilePath[0], false);
            setInViewerContextIconFilePath(serialization->_inViewerContextIconFilePath[1], true);
        }
    }
    
    // Allow changes again
    endChanges();
    unblockValueChanges();
    computeHasModifications();
} // KnobHelper::fromSerialization



/***************************KNOB HOLDER******************************************/

struct KnobHolder::KnobHolderPrivate
{
    AppInstanceWPtr app;
    QMutex knobsMutex;
    std::vector< KnobIPtr > knobs;
    bool knobsInitialized;
    bool isInitializingKnobs;
    bool isSlave;
    std::vector<KnobIWPtr> knobsWithViewerUI;

    ///Count how many times an overlay needs to be redrawn for the instanceChanged/penMotion/penDown etc... actions
    ///to just redraw it once when the recursion level is back to 0
    QMutex overlayRedrawStackMutex;
    int overlayRedrawStack;
    bool isDequeingValuesSet;
    mutable QMutex paramsEditLevelMutex;

    struct MultipleParamsEditData {
        std::string commandName;
        int nActionsInBracket;

        MultipleParamsEditData()
        : commandName()
        , nActionsInBracket(0)
        {

        }
    };

    // We use a stack in case user calls it recursively
    std::list<MultipleParamsEditData> paramsEditStack;

    mutable QMutex evaluationBlockedMutex;
    int evaluationBlocked;

    //Set in the begin/endChanges block
    bool canCurrentlySetValue;
    KnobChanges knobChanged;
    int nbSignificantChangesDuringEvaluationBlock;
    int nbChangesDuringEvaluationBlock;
    int nbChangesRequiringMetadataRefresh;
    QMutex knobsFrozenMutex;
    bool knobsFrozen;

    // Protects hasAnimation
    mutable QMutex hasAnimationMutex;

    // True if one knob held by this object is animated.
    // This is held here for fast access, otherwise it requires looping over all knobs
    bool hasAnimation;
    DockablePanelI* settingsPanel;

    std::list<KnobIWPtr> overlaySlaves;

    // A knobs table owned by the holder
    KnobItemsTablePtr knobsTable;

    // The script-name of the knob right before where the table should be inserted in the gui
    std::string knobsTableParamBefore;


    KnobHolderPrivate(const AppInstancePtr& appInstance_)
        : app(appInstance_)
        , knobsMutex()
        , knobs()
        , knobsInitialized(false)
        , isInitializingKnobs(false)
        , isSlave(false)
        , overlayRedrawStackMutex()
        , overlayRedrawStack(0)
        , isDequeingValuesSet(false)
        , evaluationBlockedMutex(QMutex::Recursive)
        , evaluationBlocked(0)
        , canCurrentlySetValue(true)
        , knobChanged()
        , nbSignificantChangesDuringEvaluationBlock(0)
        , nbChangesDuringEvaluationBlock(0)
        , nbChangesRequiringMetadataRefresh(0)
        , knobsFrozenMutex()
        , knobsFrozen(false)
        , hasAnimationMutex()
        , hasAnimation(false)
        , settingsPanel(0)
        , overlaySlaves()
        , knobsTable()
        , knobsTableParamBefore()

    {
    }

    KnobHolderPrivate(const KnobHolderPrivate& other)
    : app(other.app)
    , knobsMutex()
    , knobs(other.knobs)
    , knobsInitialized(other.knobsInitialized)
    , isInitializingKnobs(other.isInitializingKnobs)
    , isSlave(other.isSlave)
    , overlayRedrawStackMutex()
    , overlayRedrawStack(0)
    , isDequeingValuesSet(other.isDequeingValuesSet)
    , evaluationBlockedMutex(QMutex::Recursive)
    , evaluationBlocked(0)
    , canCurrentlySetValue(other.canCurrentlySetValue)
    , knobChanged()
    , nbSignificantChangesDuringEvaluationBlock(0)
    , nbChangesDuringEvaluationBlock(0)
    , nbChangesRequiringMetadataRefresh(0)
    , knobsFrozenMutex()
    , knobsFrozen(false)
    , hasAnimationMutex()
    , hasAnimation(other.hasAnimation)
    , settingsPanel(other.settingsPanel)
    {

    }
};

KnobHolder::KnobHolder(const AppInstancePtr& appInstance)
    : QObject()
    , _imp( new KnobHolderPrivate(appInstance) )
{
    QObject::connect( this, SIGNAL(doEndChangesOnMainThread()), this, SLOT(onDoEndChangesOnMainThreadTriggered()) );
    QObject::connect( this, SIGNAL(doEvaluateOnMainThread(bool,bool)), this,
                      SLOT(onDoEvaluateOnMainThread(bool,bool)) );
    QObject::connect( this, SIGNAL(doValueChangeOnMainThread(KnobIPtr,int,double,ViewSpec,bool)), this,
                      SLOT(onDoValueChangeOnMainThread(KnobIPtr,int,double,ViewSpec,bool)) );
}

KnobHolder::KnobHolder(const KnobHolder& other)
    : QObject()
    , boost::enable_shared_from_this<KnobHolder>()
    , _imp (new KnobHolderPrivate(*other._imp))
{
    QObject::connect( this, SIGNAL( doEndChangesOnMainThread() ), this, SLOT( onDoEndChangesOnMainThreadTriggered() ) );
    QObject::connect( this, SIGNAL( doEvaluateOnMainThread(bool, bool) ), this,
                     SLOT( onDoEvaluateOnMainThread(bool, bool) ) );
    QObject::connect( this, SIGNAL( doValueChangeOnMainThread(KnobIPtr, int, double, ViewSpec, bool) ), this,
                     SLOT( onDoValueChangeOnMainThread(KnobIPtr, int, double, ViewSpec, bool) ) );
}

KnobHolder::~KnobHolder()
{
    for (std::size_t i = 0; i < _imp->knobs.size(); ++i) {
        KnobHelperPtr helper = boost::dynamic_pointer_cast<KnobHelper>(_imp->knobs[i]);
        assert(helper);
        if (helper) {
            // Make sure nobody is referencing this
            helper->_imp->holder.reset();
            helper->deleteKnob();

        }
    }
}

void
KnobHolder::setItemsTable(const KnobItemsTablePtr& table, const std::string& paramScriptNameBefore)
{
    assert(!paramScriptNameBefore.empty());
    _imp->knobsTableParamBefore = paramScriptNameBefore;
    _imp->knobsTable = table;
}

KnobItemsTablePtr
KnobHolder::getItemsTable() const
{
    return _imp->knobsTable;
}

std::string
KnobHolder::getItemsTablePreviousKnobScriptName() const
{
    return _imp->knobsTableParamBefore;
}

void
KnobHolder::setViewerUIKnobs(const KnobsVec& knobs)
{
    QMutexLocker k(&_imp->knobsMutex);
    _imp->knobsWithViewerUI.clear();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        _imp->knobsWithViewerUI.push_back(*it);
    }

}

void
KnobHolder::addKnobToViewerUI(const KnobIPtr& knob)
{
    QMutexLocker k(&_imp->knobsMutex);
    _imp->knobsWithViewerUI.push_back(knob);
}

void
KnobHolder::insertKnobToViewerUI(const KnobIPtr& knob, int index)
{
    QMutexLocker k(&_imp->knobsMutex);
    if (index < 0 || index >= (int)_imp->knobsWithViewerUI.size()) {
        _imp->knobsWithViewerUI.push_back(knob);
    } else {
        std::vector<KnobIWPtr>::iterator it = _imp->knobsWithViewerUI.begin();
        std::advance(it, index);
        _imp->knobsWithViewerUI.insert(it, knob);
    }
}

void
KnobHolder::removeKnobViewerUI(const KnobIPtr& knob)
{
    QMutexLocker k(&_imp->knobsMutex);
    for (std::vector<KnobIWPtr>::iterator it = _imp->knobsWithViewerUI.begin(); it!=_imp->knobsWithViewerUI.end(); ++it) {
        KnobIPtr p = it->lock();
        if (p == knob) {
            _imp->knobsWithViewerUI.erase(it);
            return;
        }
    }

}

int
KnobHolder::getInViewerContextKnobIndex(const KnobIConstPtr& knob) const
{
    QMutexLocker k(&_imp->knobsMutex);
    int i = 0;
    for (std::vector<KnobIWPtr>::const_iterator it = _imp->knobsWithViewerUI.begin(); it!=_imp->knobsWithViewerUI.end(); ++it, ++i) {
        KnobIPtr p = it->lock();
        if (p == knob) {
            return i;
        }
    }
    return -1;
}

KnobsVec
KnobHolder::getViewerUIKnobs() const
{
    QMutexLocker k(&_imp->knobsMutex);
    KnobsVec ret;
    for (std::vector<KnobIWPtr>::const_iterator it = _imp->knobsWithViewerUI.begin(); it != _imp->knobsWithViewerUI.end(); ++it) {
        KnobIPtr k = it->lock();
        if (k) {
            ret.push_back(k);
        }
    }

    return ret;
}

void
KnobHolder::setIsInitializingKnobs(bool b)
{
    QMutexLocker k(&_imp->knobsMutex);

    _imp->isInitializingKnobs = b;
}

bool
KnobHolder::isInitializingKnobs() const
{
    QMutexLocker k(&_imp->knobsMutex);

    return _imp->isInitializingKnobs;
}

void
KnobHolder::addKnob(const KnobIPtr& k)
{
    assert( QThread::currentThread() == qApp->thread() );
    QMutexLocker kk(&_imp->knobsMutex);
    for (KnobsVec::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        if (*it == k) {
            return;
        }
    }
    _imp->knobs.push_back(k);
}

void
KnobHolder::insertKnob(int index,
                       const KnobIPtr& k)
{
    if (index < 0) {
        return;
    }
    QMutexLocker kk(&_imp->knobsMutex);
    for (KnobsVec::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        if (*it == k) {
            return;
        }
    }
    if ( index >= (int)_imp->knobs.size() ) {
        _imp->knobs.push_back(k);
    } else {
        KnobsVec::iterator it = _imp->knobs.begin();
        std::advance(it, index);
        _imp->knobs.insert(it, k);
    }
}

void
KnobHolder::removeKnobFromList(const KnobIConstPtr& knob)
{
    QMutexLocker kk(&_imp->knobsMutex);

    for (KnobsVec::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        if (*it == knob) {
            _imp->knobs.erase(it);

            return;
        }
    }
}

void
KnobHolder::setPanelPointer(DockablePanelI* gui)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->settingsPanel = gui;
}

void
KnobHolder::discardPanelPointer()
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->settingsPanel = 0;
}

void
KnobHolder::recreateUserKnobs(bool keepCurPageIndex)
{
    assert( QThread::currentThread() == qApp->thread() );
    if (_imp->settingsPanel) {
        _imp->settingsPanel->recreateUserKnobs(keepCurPageIndex);
        EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
        if (isEffect) {
            isEffect->getNode()->declarePythonKnobs();
        }
    }
}

void
KnobHolder::recreateKnobs(bool keepCurPageIndex)
{
    assert( QThread::currentThread() == qApp->thread() );
    if (_imp->settingsPanel) {
        _imp->settingsPanel->refreshGuiForKnobsChanges(keepCurPageIndex);
        EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
        if (isEffect) {
            isEffect->getNode()->declarePythonKnobs();
        }
    }
}

void
KnobHolder::deleteKnob(const KnobIPtr& knob,
                       bool alsoDeleteGui)
{
    assert( QThread::currentThread() == qApp->thread() );

    KnobsVec knobs;
    {
        QMutexLocker k(&_imp->knobsMutex);
        knobs = _imp->knobs;
    }
    KnobIPtr sharedKnob;
    for (KnobsVec::iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if (*it == knob) {
            (*it)->deleteKnob();
            sharedKnob = *it;
            break;
        }
    }

    {
        QMutexLocker k(&_imp->knobsMutex);
        for (KnobsVec::iterator it2 = _imp->knobs.begin(); it2 != _imp->knobs.end(); ++it2) {
            if (*it2 == knob) {
                _imp->knobs.erase(it2);
                break;
            }
        }
    }

    if (sharedKnob && alsoDeleteGui && _imp->settingsPanel) {
        _imp->settingsPanel->deleteKnobGui(sharedKnob);
    }
}

void
KnobHolder::addOverlaySlaveParam(const KnobIPtr& knob)
{
    _imp->overlaySlaves.push_back(knob);
}

bool
KnobHolder::isOverlaySlaveParam(const KnobIConstPtr& knob) const
{
    for (std::list<KnobIWPtr >::const_iterator it = _imp->overlaySlaves.begin(); it != _imp->overlaySlaves.end(); ++it) {
        KnobIPtr k = it->lock();
        if (!k) {
            continue;
        }
        if (k == knob) {
            return true;
        }
    }

    return false;
}

void
KnobHolder::redrawOverlayInteract()
{
    if ( isDoingInteractAction() ) {
        getApp()->queueRedrawForAllViewers();
    } else {
        getApp()->redrawAllViewers();
    }
}


bool
KnobHolder::moveViewerUIKnobOneStepUp(const KnobIPtr& knob)
{
    QMutexLocker k(&_imp->knobsMutex);
    for (std::size_t i = 0; i < _imp->knobsWithViewerUI.size(); ++i) {
        if (_imp->knobsWithViewerUI[i].lock() == knob) {
            if (i == 0) {
                return false;
            }
            std::swap(_imp->knobsWithViewerUI[i - 1], _imp->knobsWithViewerUI[i]);
            return true;
        }
    }
    return false;
}

bool
KnobHolder::moveViewerUIOneStepDown(const KnobIPtr& knob)
{
    QMutexLocker k(&_imp->knobsMutex);
    for (std::size_t i = 0; i < _imp->knobsWithViewerUI.size(); ++i) {
        if (_imp->knobsWithViewerUI[i].lock() == knob) {
            if (i == _imp->knobsWithViewerUI.size() - 1) {
                return false;
            }
            std::swap(_imp->knobsWithViewerUI[i + 1], _imp->knobsWithViewerUI[i]);
            return true;
        }
    }
    return false;
}

bool
KnobHolder::moveKnobOneStepUp(const KnobIPtr& knob)
{
    if ( !knob->isUserKnob() && !toKnobPage(knob) ) {
        return false;
    }
    KnobIPtr parent = knob->getParentKnob();
    KnobGroupPtr parentIsGrp = toKnobGroup(parent);
    KnobPagePtr parentIsPage = toKnobPage(parent);

    //the knob belongs to a group/page , change its index within the group instead
    bool moveOk = false;
    if (!parent) {
        moveOk = true;
    }
    try {
        if (parentIsGrp) {
            moveOk = parentIsGrp->moveOneStepUp(knob);
        } else if (parentIsPage) {
            moveOk = parentIsPage->moveOneStepUp(knob);
        }
    } catch (const std::exception& e) {
        qDebug() << e.what();
        assert(false);

        return false;
    }

    if (moveOk) {
        QMutexLocker k(&_imp->knobsMutex);
        int prevInPage = -1;
        if (parent) {
            for (U32 i = 0; i < _imp->knobs.size(); ++i) {
                if (_imp->knobs[i] == knob) {
                    if (prevInPage != -1) {
                        KnobIPtr tmp = _imp->knobs[prevInPage];
                        _imp->knobs[prevInPage] = _imp->knobs[i];
                        _imp->knobs[i] = tmp;
                    }
                    break;
                } else {
                    if ( _imp->knobs[i]->isUserKnob() && (_imp->knobs[i]->getParentKnob() == parent) ) {
                        prevInPage = i;
                    }
                }
            }
        } else {
            bool foundPrevPage = false;
            for (U32 i = 0; i < _imp->knobs.size(); ++i) {
                if (_imp->knobs[i] == knob) {
                    if (prevInPage != -1) {
                        KnobIPtr tmp = _imp->knobs[prevInPage];
                        _imp->knobs[prevInPage] = _imp->knobs[i];
                        _imp->knobs[i] = tmp;
                        foundPrevPage = true;
                    }
                    break;
                } else {
                    if ( !_imp->knobs[i]->getParentKnob() ) {
                        prevInPage = i;
                    }
                }
            }
            if (!foundPrevPage) {
                moveOk = false;
            }
        }
    }

    return moveOk;
} // KnobHolder::moveKnobOneStepUp

bool
KnobHolder::moveKnobOneStepDown(const KnobIPtr& knob)
{
    if ( !knob->isUserKnob() && !toKnobPage(knob) ) {
        return false;
    }
    KnobIPtr parent = knob->getParentKnob();
    KnobGroupPtr parentIsGrp = toKnobGroup(parent);
    KnobPagePtr parentIsPage = toKnobPage(parent);

    //the knob belongs to a group/page , change its index within the group instead
    bool moveOk = false;
    if (!parent) {
        moveOk = true;
    }
    try {
        if (parentIsGrp) {
            moveOk = parentIsGrp->moveOneStepDown(knob);
        } else if (parentIsPage) {
            moveOk = parentIsPage->moveOneStepDown(knob);
        }
    } catch (const std::exception& e) {
        qDebug() << e.what();
        assert(false);

        return false;
    }

    QMutexLocker k(&_imp->knobsMutex);
    int foundIndex = -1;
    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        if (_imp->knobs[i] == knob) {
            foundIndex = i;
            break;
        }
    }
    assert(foundIndex != -1);
    if (foundIndex < 0) {
        return false;
    }
    if (moveOk) {
        //The knob (or page) could be moved inside the group/page, just move it down
        if (parent) {
            for (int i = foundIndex + 1; i < (int)_imp->knobs.size(); ++i) {
                if ( _imp->knobs[i]->isUserKnob() && (_imp->knobs[i]->getParentKnob() == parent) ) {
                    KnobIPtr tmp = _imp->knobs[foundIndex];
                    _imp->knobs[foundIndex] = _imp->knobs[i];
                    _imp->knobs[i] = tmp;
                    break;
                }
            }
        } else {
            bool foundNextPage = false;
            for (int i = foundIndex + 1; i < (int)_imp->knobs.size(); ++i) {
                if ( !_imp->knobs[i]->getParentKnob() ) {
                    KnobIPtr tmp = _imp->knobs[foundIndex];
                    _imp->knobs[foundIndex] = _imp->knobs[i];
                    _imp->knobs[i] = tmp;
                    foundNextPage = true;
                    break;
                }
            }

            if (!foundNextPage) {
                moveOk = false;
            }
        }
    }

    return moveOk;
} // KnobHolder::moveKnobOneStepDown

KnobPagePtr
KnobHolder::getUserPageKnob() const
{
    {
        QMutexLocker k(&_imp->knobsMutex);
        for (KnobsVec::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
            if (!(*it)->isUserKnob()) {
                continue;
            }
            KnobPagePtr isPage = boost::dynamic_pointer_cast<KnobPage>(*it);
            if (isPage) {
                return isPage;
            }
        }
    }

    return KnobPagePtr();
}

KnobPagePtr
KnobHolder::getOrCreateUserPageKnob()
{
    KnobPagePtr ret = getUserPageKnob();

    if (ret) {
        return ret;
    }
    ret = AppManager::createKnob<KnobPage>(shared_from_this(), tr(NATRON_USER_MANAGED_KNOBS_PAGE_LABEL), 1, false);
    ret->setName(NATRON_USER_MANAGED_KNOBS_PAGE);
    onUserKnobCreated(ret, true);
    return ret;
}

void
KnobHolder::onUserKnobCreated(const KnobIPtr& knob, bool isUserKnob)
{

    knob->setAsUserKnob(isUserKnob);
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect) {
        if (isEffect->getNode()->isPyPlug() && getApp()->isCreatingNode()) {
            knob->setDeclaredByPlugin(true);
        }
        if (isUserKnob) {
            isEffect->getNode()->declarePythonKnobs();
        }
    }

}

KnobIntPtr
KnobHolder::createIntKnob(const std::string& name,
                          const std::string& label,
                          int dimension,
                          bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobInt(existingKnob);
    }
    KnobIntPtr ret = AppManager::createKnob<KnobInt>(shared_from_this(), label, dimension, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);
    return ret;
}

KnobDoublePtr
KnobHolder::createDoubleKnob(const std::string& name,
                             const std::string& label,
                             int dimension,
                             bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobDouble(existingKnob);
    }
    KnobDoublePtr ret = AppManager::createKnob<KnobDouble>(shared_from_this(), label, dimension, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);
    return ret;
}

KnobColorPtr
KnobHolder::createColorKnob(const std::string& name,
                            const std::string& label,
                            int dimension,
                            bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobColor(existingKnob);
    }
    KnobColorPtr ret = AppManager::createKnob<KnobColor>(shared_from_this(), label, dimension, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

KnobBoolPtr
KnobHolder::createBoolKnob(const std::string& name,
                           const std::string& label,
                           bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobBool(existingKnob);
    }
    KnobBoolPtr ret = AppManager::createKnob<KnobBool>(shared_from_this(), label, 1, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

KnobChoicePtr
KnobHolder::createChoiceKnob(const std::string& name,
                             const std::string& label,
                             bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobChoice(existingKnob);
    }
    KnobChoicePtr ret = AppManager::createKnob<KnobChoice>(shared_from_this(), label, 1, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

KnobButtonPtr
KnobHolder::createButtonKnob(const std::string& name,
                             const std::string& label,
                             bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobButton(existingKnob);
    }
    KnobButtonPtr ret = AppManager::createKnob<KnobButton>(shared_from_this(), label, 1, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

KnobSeparatorPtr
KnobHolder::createSeparatorKnob(const std::string& name,
                                const std::string& label,
                                bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobSeparator(existingKnob);
    }
    KnobSeparatorPtr ret = AppManager::createKnob<KnobSeparator>(shared_from_this(), label, 1, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

//Type corresponds to the Type enum defined in StringParamBase in Parameter.h
KnobStringPtr
KnobHolder::createStringKnob(const std::string& name,
                             const std::string& label,
                             bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobString(existingKnob);
    }
    KnobStringPtr ret = AppManager::createKnob<KnobString>(shared_from_this(), label, 1, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);
    return ret;
}

KnobFilePtr
KnobHolder::createFileKnob(const std::string& name,
                           const std::string& label,
                           bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobFile(existingKnob);
    }
    KnobFilePtr ret = AppManager::createKnob<KnobFile>(shared_from_this(), label, 1, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);

    return ret;
}



KnobPathPtr
KnobHolder::createPathKnob(const std::string& name,
                           const std::string& label,
                           bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobPath(existingKnob);
    }
    KnobPathPtr ret = AppManager::createKnob<KnobPath>(shared_from_this(), label, 1, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);
    return ret;
}

KnobGroupPtr
KnobHolder::createGroupKnob(const std::string& name,
                            const std::string& label,
                            bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobGroup(existingKnob);
    }
    KnobGroupPtr ret = AppManager::createKnob<KnobGroup>(shared_from_this(), label, 1, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

KnobPagePtr
KnobHolder::createPageKnob(const std::string& name,
                           const std::string& label,
                           bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobPage(existingKnob);
    }
    KnobPagePtr ret = AppManager::createKnob<KnobPage>(shared_from_this(), label, 1, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);
    return ret;
}

KnobParametricPtr
KnobHolder::createParametricKnob(const std::string& name,
                                 const std::string& label,
                                 int nbCurves,
                                 bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return toKnobParametric(existingKnob);
    }
    KnobParametricPtr ret = AppManager::createKnob<KnobParametric>(shared_from_this(), label, nbCurves, false);
    ret->setName(name);
    onUserKnobCreated(ret, userKnob);

    return ret;
}

void
KnobHolder::onDoEvaluateOnMainThread(bool significant,
                                     bool refreshMetadata)
{
    assert( QThread::currentThread() == qApp->thread() );
    evaluate(significant, refreshMetadata);
}

void
KnobHolder::invalidateCacheHashAndEvaluate(bool isSignificant,
                                bool refreshMetadatas)
{
    onSignificantEvaluateAboutToBeCalled(KnobIPtr(), eValueChangedReasonNatronInternalEdited, -1, 0, ViewSpec(0));
    evaluate(isSignificant, refreshMetadatas);
}

void
KnobHolder::onDoEndChangesOnMainThreadTriggered()
{
    assert( QThread::currentThread() == qApp->thread() );
    endChanges();
}

bool
KnobHolder::endChanges(bool discardRendering)
{
    bool isMT = QThread::currentThread() == qApp->thread();

    if ( !isMT && !canHandleEvaluateOnChangeInOtherThread() ) {
        Q_EMIT doEndChangesOnMainThread();

        return true;
    }


    bool thisChangeSignificant = false;
    bool thisBracketHadChange = false;
    KnobChanges knobChanged;
    {
        QMutexLocker l(&_imp->evaluationBlockedMutex);

        knobChanged = _imp->knobChanged;
        for (KnobChanges::iterator it = knobChanged.begin(); it != knobChanged.end(); ++it) {
            if ( it->knob->getEvaluateOnChange() ) {
                thisChangeSignificant = true;
            }

            if ( !it->valueChangeBlocked && it->knob->getIsMetadataSlave() ) {
                ++_imp->nbChangesRequiringMetadataRefresh;
            }
        }
        if (thisChangeSignificant) {
            ++_imp->nbSignificantChangesDuringEvaluationBlock;
        }
        if ( !knobChanged.empty() ) {
            ++_imp->nbChangesDuringEvaluationBlock;
            thisBracketHadChange = true;
        }

        _imp->knobChanged.clear();
    }
    KnobIPtr firstKnobChanged;
    ValueChangedReasonEnum firstKnobReason = eValueChangedReasonNatronGuiEdited;
    int firstKnobDimension = -1;
    ViewSpec firstKnobView(0);
    double firstKnobTime = 0;
    if ( !knobChanged.empty() ) {
        KnobChange& first = knobChanged.front();
        firstKnobChanged = first.knob;
        firstKnobReason = first.reason;
        if (!first.dimensionChanged.empty()) {
            firstKnobDimension = *(first.dimensionChanged.begin());
        }
        firstKnobTime = first.time;
        firstKnobView = first.view;
    }
    bool isChangeDueToTimeChange = firstKnobReason == eValueChangedReasonTimeChanged;
    bool isLoadingProject = false;
    if ( getApp() ) {
        isLoadingProject = getApp()->getProject()->isLoadingProject();
    }

    // If the node is currently modifying its input, do not ask for a render
    // because at then end of the inputChanged handler, it will ask for a refresh
    // and a rebuild of the inputs tree.
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    bool duringInputChangeAction = false;
    if (isEffect) {
        NodePtr node = isEffect->getNode();
        if ( isMT && node->duringInputChangedAction() ) {
            duringInputChangeAction = true;
        }
    }


    // Increment hash only if significant
    if (thisChangeSignificant && thisBracketHadChange && !isLoadingProject && !duringInputChangeAction && !isChangeDueToTimeChange) {
        onSignificantEvaluateAboutToBeCalled(firstKnobChanged, firstKnobReason, firstKnobDimension, firstKnobTime, firstKnobView);
    }

    bool guiFrozen = firstKnobChanged ? getApp() && firstKnobChanged->getKnobGuiPointer() && firstKnobChanged->getKnobGuiPointer()->isGuiFrozenForPlayback() : false;

    // Call instanceChanged on each knob
    bool ret = false;
    for (KnobChanges::iterator it = knobChanged.begin(); it != knobChanged.end(); ++it) {

        it->knob->computeHasModifications();

        if (it->knob && !it->valueChangeBlocked && !isLoadingProject) {
            if ( !it->originatedFromMainThread && !canHandleEvaluateOnChangeInOtherThread() ) {
                Q_EMIT doValueChangeOnMainThread(it->knob, it->originalReason, it->time, it->view, it->originatedFromMainThread);
            } else {
                ret |= onKnobValueChanged_public(it->knob, it->originalReason, it->time, it->view, it->originatedFromMainThread);
            }
        }


        int dimension = -1;
        if (it->dimensionChanged.size() == 1) {
            dimension = *it->dimensionChanged.begin();
        }

        if (!guiFrozen) {
            boost::shared_ptr<KnobSignalSlotHandler> handler = it->knob->getSignalSlotHandler();
            if (handler) {
#pragma message WARN("Don't update GUI if value changes are blocked")
                handler->s_valueChanged(it->view, dimension, it->reason);
            }
            it->knob->refreshAnimationLevel(it->view, dimension);
        }

        if ( !it->valueChangeBlocked && !it->knob->isListenersNotificationBlocked() && firstKnobReason != eValueChangedReasonTimeChanged) {
            it->knob->refreshListenersAfterValueChange(it->view, it->originalReason, dimension);
        }
    }

    int evaluationBlocked;
    bool hasHadSignificantChange = false;
    bool hasHadAnyChange = false;
    bool mustRefreshMetadatas = false;

    {
        QMutexLocker l(&_imp->evaluationBlockedMutex);
        if (_imp->evaluationBlocked > 0) {
            --_imp->evaluationBlocked;
        }
        evaluationBlocked = _imp->evaluationBlocked;
        if (evaluationBlocked == 0) {
            if (_imp->nbSignificantChangesDuringEvaluationBlock) {
                hasHadSignificantChange = true;
            }
            if (_imp->nbChangesRequiringMetadataRefresh) {
                mustRefreshMetadatas = true;
            }
            if (_imp->nbChangesDuringEvaluationBlock) {
                hasHadAnyChange = true;
            }
            _imp->nbSignificantChangesDuringEvaluationBlock = 0;
            _imp->nbChangesDuringEvaluationBlock = 0;
            _imp->nbChangesRequiringMetadataRefresh = 0;
        }
    }


    // Call getClipPreferences & render
    if ( hasHadAnyChange && !discardRendering && !isLoadingProject && !duringInputChangeAction && !isChangeDueToTimeChange && (evaluationBlocked == 0) ) {
        endKnobsValuesChanged_public(firstKnobReason);
        if (!isMT) {
            Q_EMIT doEvaluateOnMainThread(hasHadSignificantChange, mustRefreshMetadatas);
        } else {
            evaluate(hasHadSignificantChange, mustRefreshMetadatas);
        }

    }

    return ret;
} // KnobHolder::endChanges

void
KnobHolder::onDoValueChangeOnMainThread(const KnobIPtr& knob,
                                        int reason,
                                        double time,
                                        ViewSpec view,
                                        bool originatedFromMT)
{
    assert( QThread::currentThread() == qApp->thread() );
    onKnobValueChanged_public(knob, (ValueChangedReasonEnum)reason, time, view, originatedFromMT);
}

void
KnobHolder::appendValueChange(const KnobIPtr& knob,
                              int dimension,
                              double time,
                              ViewSpec view,
                              ValueChangedReasonEnum originalReason,
                              ValueChangedReasonEnum reason)
{
    if ( isInitializingKnobs() ) {
        return;
    }
    {
        QMutexLocker l(&_imp->evaluationBlockedMutex);

        if (_imp->nbChangesDuringEvaluationBlock == 0) {
            // This is the first change, call begin action
            beginKnobsValuesChanged_public(originalReason);
        }

        KnobChange* foundChange = 0;
        for (KnobChanges::iterator it = _imp->knobChanged.begin(); it != _imp->knobChanged.end(); ++it) {
            if (it->knob == knob) {
                foundChange = &*it;
                break;
            }
        }
        if (!foundChange) {
            KnobChange p;
            _imp->knobChanged.push_back(p);
            foundChange = &_imp->knobChanged.back();
        }
        assert(foundChange);

        foundChange->reason = reason;
        foundChange->originalReason = originalReason;
        foundChange->originatedFromMainThread = QThread::currentThread() == qApp->thread();
        foundChange->time = time;
        foundChange->view = view;
        foundChange->knob = knob;
        foundChange->valueChangeBlocked = knob->isValueChangesBlocked();
        if (dimension == -1) {
            for (int i = 0; i < knob->getNDimensions(); ++i) {
                foundChange->dimensionChanged.insert(i);
                // Make sure expressions are invalidated
                knob->clearExpressionsResults(i);
            }
        } else {
            foundChange->dimensionChanged.insert(dimension);

            // Make sure expressions are invalidated
            knob->clearExpressionsResults(dimension);
        }

        if ( !foundChange->valueChangeBlocked && knob->getIsMetadataSlave() ) {
            ++_imp->nbChangesRequiringMetadataRefresh;
        }

        if ( knob->getEvaluateOnChange() ) {
            ++_imp->nbSignificantChangesDuringEvaluationBlock;
        }
        ++_imp->nbChangesDuringEvaluationBlock;

    }
} // KnobHolder::appendValueChange

void
KnobHolder::beginChanges()
{
    /*
     * Start a begin/end block, actually blocking all evaluations (renders) but not value changed callback.
     */
    bool canSet = canSetValue();
    QMutexLocker l(&_imp->evaluationBlockedMutex);

    ++_imp->evaluationBlocked;
    if (_imp->evaluationBlocked == 1) {
        _imp->canCurrentlySetValue = canSet;
    }
    //std::cout <<"INCR: " << _imp->evaluationBlocked << std::endl;
}

bool
KnobHolder::isEvaluationBlocked() const
{
    QMutexLocker l(&_imp->evaluationBlockedMutex);

    return _imp->evaluationBlocked > 0;
}

bool
KnobHolder::isSetValueCurrentlyPossible() const
{
    {
        QMutexLocker l(&_imp->evaluationBlockedMutex);
        if (_imp->evaluationBlocked > 0) {
            return _imp->canCurrentlySetValue;
        }
    }

    return canSetValue();
}

void
KnobHolder::getAllExpressionDependenciesRecursive(std::set<NodePtr >& nodes) const
{
    QMutexLocker k(&_imp->knobsMutex);

    for (KnobsVec::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        (*it)->getAllExpressionDependenciesRecursive(nodes);
    }
}


void
KnobHolder::beginMultipleEdits(const std::string& commandName)
{
    {
        QMutexLocker l(&_imp->paramsEditLevelMutex);
        KnobHolderPrivate::MultipleParamsEditData data;
        data.commandName = commandName;
        _imp->paramsEditStack.push_back(data);
    }
    beginChanges();
}

KnobHolder::MultipleParamsEditEnum
KnobHolder::getMultipleEditsLevel() const
{
    QMutexLocker l(&_imp->paramsEditLevelMutex);
    if (_imp->paramsEditStack.empty()) {
        return eMultipleParamsEditOff;
    }
    const KnobHolderPrivate::MultipleParamsEditData& last = _imp->paramsEditStack.back();
    if (last.nActionsInBracket > 0) {
        return eMultipleParamsEditOn;
    } else {
        return eMultipleParamsEditOnCreateNewCommand;
    }
}

std::string
KnobHolder::getCurrentMultipleEditsCommandName() const
{
    QMutexLocker l(&_imp->paramsEditLevelMutex);
    if (_imp->paramsEditStack.empty()) {
        return std::string();
    }
    return _imp->paramsEditStack.back().commandName;
}

void
KnobHolder::endMultipleEdits()
{
    bool mustCallEndChanges = false;
    {
        QMutexLocker l(&_imp->paramsEditLevelMutex);
        if (_imp->paramsEditStack.empty()) {
            qDebug() << "[BUG]: Call to endMultipleEdits without a matching call to beginMultipleEdits";
            return;
        }
        _imp->paramsEditStack.pop_back();
        mustCallEndChanges = _imp->paramsEditStack.empty();
    }
    if (mustCallEndChanges) {
        endChanges();
    }
}

AppInstancePtr
KnobHolder::getApp() const
{
    return _imp->app.lock();
}

void
KnobHolder::initializeKnobsPublic()
{
    {
        InitializeKnobsFlag_RAII __isInitializingKnobsFlag__( shared_from_this() );
        initializeKnobs();
    }
    _imp->knobsInitialized = true;
}

void
KnobHolder::refreshAfterTimeChange(bool isPlayback,
                                   double time)
{
    assert( QThread::currentThread() == qApp->thread() );
    AppInstancePtr app = getApp();
    if ( !app || app->isGuiFrozen() ) {
        return;
    }
    for (std::size_t i = 0; i < _imp->knobs.size(); ++i) {
        _imp->knobs[i]->onTimeChanged(isPlayback, time);
    }
    refreshExtraStateAfterTimeChanged(isPlayback, time);
}

void
KnobHolder::refreshAfterTimeChangeOnlyKnobsWithTimeEvaluation(double time)
{
    assert( QThread::currentThread() == qApp->thread() );
    for (std::size_t i = 0; i < _imp->knobs.size(); ++i) {
        if ( _imp->knobs[i]->evaluateValueChangeOnTimeChange() ) {
            _imp->knobs[i]->onTimeChanged(false, time);
        }
    }
}


KnobIPtr
KnobHolder::getKnobByName(const std::string & name) const
{
    QMutexLocker k(&_imp->knobsMutex);

    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        if (_imp->knobs[i]->getName() == name) {
            return _imp->knobs[i];
        }
    }

    return KnobIPtr();
}

// Same as getKnobByName expect that if we find the caller, we skip it
KnobIPtr
KnobHolder::getOtherKnobByName(const std::string & name,
                               const KnobIConstPtr& caller) const
{
    QMutexLocker k(&_imp->knobsMutex);

    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        if (_imp->knobs[i] == caller) {
            continue;
        }
        if (_imp->knobs[i]->getName() == name) {
            return _imp->knobs[i];
        }
    }

    return KnobIPtr();
}

const std::vector< KnobIPtr > &
KnobHolder::getKnobs() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->knobs;
}

std::vector< KnobIPtr >
KnobHolder::getKnobs_mt_safe() const
{
    QMutexLocker k(&_imp->knobsMutex);

    return _imp->knobs;
}

void
KnobHolder::slaveAllKnobs(const KnobHolderPtr& other)
{
    assert( QThread::currentThread() == qApp->thread() );
    if (_imp->isSlave) {
        return;
    }
    ///Call it prior to slaveTo: it will set the master pointer as pointing to other
    onAllKnobsSlaved(true, other);

    ///When loading a project, we don't need to slave all knobs here because the serialization of each knob separatly
    ///will reslave it correctly if needed
    beginChanges();

    const KnobsVec & otherKnobs = other->getKnobs();
    const KnobsVec & thisKnobs = getKnobs();
    for (U32 i = 0; i < otherKnobs.size(); ++i) {
        if ( otherKnobs[i]->isDeclaredByPlugin() || otherKnobs[i]->isUserKnob() ) {
            KnobIPtr foundKnob;
            for (U32 j = 0; j < thisKnobs.size(); ++j) {
                if ( thisKnobs[j]->getName() == otherKnobs[i]->getName() ) {
                    foundKnob = thisKnobs[j];
                    break;
                }
            }
            assert(foundKnob);
            if (!foundKnob) {
                continue;
            }
            int dims = foundKnob->getNDimensions();
            for (int j = 0; j < dims; ++j) {
                foundKnob->slaveTo(j, otherKnobs[i], j);
            }
        }
    }
    endChanges();

    _imp->isSlave = true;
}

bool
KnobHolder::isSlave() const
{
    return _imp->isSlave;
}

void
KnobHolder::unslaveAllKnobs()
{
    if (!_imp->isSlave) {
        return;
    }
    const KnobsVec & thisKnobs = getKnobs();
    beginChanges();
    for (U32 i = 0; i < thisKnobs.size(); ++i) {
        int dims = thisKnobs[i]->getNDimensions();
        for (int j = 0; j < dims; ++j) {
            if ( thisKnobs[i]->isSlave(j) ) {
                thisKnobs[i]->unSlave(j, true);
            }
        }
    }
    endChanges();
    _imp->isSlave = false;
}

void
KnobHolder::beginKnobsValuesChanged_public(ValueChangedReasonEnum reason)
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );

    RECURSIVE_ACTION();
    beginKnobsValuesChanged(reason);
}

void
KnobHolder::endKnobsValuesChanged_public(ValueChangedReasonEnum reason)
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );

    RECURSIVE_ACTION();
    endKnobsValuesChanged(reason);
}

bool
KnobHolder::onKnobValueChanged_public(const KnobIPtr& k,
                                      ValueChangedReasonEnum reason,
                                      double time,
                                      ViewSpec view,
                                      bool originatedFromMainThread)
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );
    if (!_imp->knobsInitialized) {
        return false;
    }
    RECURSIVE_ACTION();

    return onKnobValueChanged(k, reason, time, view, originatedFromMainThread);
}

void
KnobHolder::checkIfRenderNeeded()
{
    ///cannot run in another thread.
    assert( QThread::currentThread() == qApp->thread() );
    if ( (getRecursionLevel() == 0) ) {
        endChanges();
    }
}

void
KnobHolder::incrementRedrawNeededCounter()
{
    {
        QMutexLocker k(&_imp->overlayRedrawStackMutex);
        ++_imp->overlayRedrawStack;
    }
}

bool
KnobHolder::checkIfOverlayRedrawNeeded()
{
    {
        QMutexLocker k(&_imp->overlayRedrawStackMutex);
        bool ret = _imp->overlayRedrawStack > 0;
        _imp->overlayRedrawStack = 0;

        return ret;
    }
}


void
KnobHolder::setKnobsFrozen(bool frozen)
{
    {
        QMutexLocker l(&_imp->knobsFrozenMutex);
        if (frozen == _imp->knobsFrozen) {
            return;
        }
        _imp->knobsFrozen = frozen;
    }
    KnobsVec knobs = getKnobs_mt_safe();

    for (U32 i = 0; i < knobs.size(); ++i) {
        knobs[i]->setIsFrozen(frozen);
    }
}

bool
KnobHolder::areKnobsFrozen() const
{
    QMutexLocker l(&_imp->knobsFrozenMutex);

    return _imp->knobsFrozen;
}

bool
KnobHolder::isDequeueingValuesSet() const
{
    {
        QMutexLocker k(&_imp->overlayRedrawStackMutex);

        return _imp->isDequeingValuesSet;
    }
}

bool
KnobHolder::dequeueValuesSet()
{
    assert( QThread::currentThread() == qApp->thread() );
    beginChanges();
    {
        QMutexLocker k(&_imp->overlayRedrawStackMutex);
        _imp->isDequeingValuesSet = true;
    }
    bool ret = false;
    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        ret |= _imp->knobs[i]->dequeueValuesSet(false);
    }
    {
        QMutexLocker k(&_imp->overlayRedrawStackMutex);
        _imp->isDequeingValuesSet = false;
    }
    endChanges();

    return ret;
}

double
KnobHolder::getCurrentTime() const
{
    return getApp() ? getApp()->getTimeLine()->currentFrame() : 0;
}

int
KnobHolder::getPageIndex(const KnobPagePtr page) const
{
    QMutexLocker k(&_imp->knobsMutex);
    int pageIndex = 0;

    for (std::size_t i = 0; i < _imp->knobs.size(); ++i) {
        KnobPagePtr ispage = toKnobPage(_imp->knobs[i]);
        if (ispage) {
            if (page == ispage) {
                return pageIndex;
            } else {
                ++pageIndex;
            }
        }
    }

    return -1;
}

bool
KnobHolder::getHasAnimation() const
{
    QMutexLocker k(&_imp->hasAnimationMutex);

    return _imp->hasAnimation;
}

void
KnobHolder::setHasAnimation(bool hasAnimation)
{
    QMutexLocker k(&_imp->hasAnimationMutex);

    _imp->hasAnimation = hasAnimation;
}

void
KnobHolder::updateHasAnimation()
{
    bool hasAnimation = false;
    {
        QMutexLocker l(&_imp->knobsMutex);

        for (KnobsVec::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
            if ( (*it)->hasAnimation() ) {
                hasAnimation = true;
                break;
            }
        }
    }
    QMutexLocker k(&_imp->hasAnimationMutex);

    _imp->hasAnimation = hasAnimation;
}

void
KnobHolder::appendToHash(double time, ViewIdx view, Hash64* hash)
{
    KnobsVec knobs = getKnobs_mt_safe();
    for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
        if (!(*it)->getEvaluateOnChange()) {
            continue;
        }
        U64 knobHash = (*it)->computeHash(time, view);
        hash->append(knobHash);

    }

} // appendToHash



/***************************STRING ANIMATION******************************************/



bool
AnimatingKnobStringHelper::cloneExtraData(const KnobIPtr& other,
                                          ViewIdx view,
                                          double offset,
                                          const RangeD* range,
                                          int /*dimension*/,
                                          int /*otherDimension*/)
{
    AnimatingKnobStringHelperPtr isAnimatedString = boost::dynamic_pointer_cast<AnimatingKnobStringHelper>(other);

    if (isAnimatedString) {
        return _animation->clone( *isAnimatedString->getStringAnimation(), view, offset, range );
    }

    return false;

}

AnimatingKnobStringHelper::AnimatingKnobStringHelper(const KnobHolderPtr& holder,
                                                     const std::string &description,
                                                     int dimension,
                                                     bool declaredByPlugin)
    : KnobStringBase(holder, description, dimension, declaredByPlugin)
    , _animation()
{

}

AnimatingKnobStringHelper::~AnimatingKnobStringHelper()
{
}

void
AnimatingKnobStringHelper::populate()
{
    KnobStringBase::populate();
    _animation.reset(new StringAnimationManager( shared_from_this() ) );
}

void
AnimatingKnobStringHelper::stringToKeyFrameValue(double time,
                                                 ViewIdx /*view*/,
                                                 const std::string & v,
                                                 double* returnValue)
{
    _animation->insertKeyFrame(time, v, returnValue);
}

void
AnimatingKnobStringHelper::stringFromInterpolatedValue(double interpolated,
                                                       ViewSpec view,
                                                       std::string* returnValue) const
{
    assert( !view.isAll() );
    Q_UNUSED(view);
    _animation->stringFromInterpolatedIndex(interpolated, returnValue);
}

void
AnimatingKnobStringHelper::onKeyframesRemoved( const std::list<double>& keysRemoved,
                                          ViewSpec /*view*/,
                                          int /*dimension*/,
                                          CurveChangeReason /*reason*/)
{
    _animation->removeKeyframes(keysRemoved);
}

std::string
AnimatingKnobStringHelper::getStringAtTime(double time,
                                           ViewSpec view,
                                           int dimension)
{
    std::string ret;

    bool succeeded = false;
    if ( _animation->hasCustomInterp() ) {
        try {
            succeeded = _animation->customInterpolation(time, &ret);
        } catch (...) {
        }

    }
    if (!succeeded) {
        ret = getValue(dimension, view);
    }

    return ret;
}

void
AnimatingKnobStringHelper::setCustomInterpolation(customParamInterpolationV1Entry_t func,
                                                  void* ofxParamHandle)
{
    _animation->setCustomInterpolation(func, ofxParamHandle);
}

void
AnimatingKnobStringHelper::loadAnimation(const std::map<int, std::string> & keyframes)
{
    _animation->load(keyframes);
}

void
AnimatingKnobStringHelper::saveAnimation(std::map<int, std::string>* keyframes) const
{
    _animation->save(keyframes);
}

/***************************KNOB EXPLICIT TEMPLATE INSTANTIATION******************************************/


template class Knob<int>;
template class Knob<double>;
template class Knob<bool>;
template class Knob<std::string>;

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_Knob.cpp"
