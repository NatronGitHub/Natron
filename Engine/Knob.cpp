/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include <algorithm> // min, max
#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream
#include <cctype> // isspace

#include <QtCore/QDataStream>
#include <QtCore/QDateTime>
#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QDebug>

#include "Global/GlobalDefines.h"

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Curve.h"
#include "Engine/DockablePanelI.h"
#include "Engine/Hash64.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobGuiI.h"
#include "Engine/KnobSerialization.h"
#include "Engine/KnobTypes.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/StringAnimationManager.h"
#include "Engine/TLSHolder.h"
#include "Engine/TimeLine.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerContext.h"
#include "Engine/Transform.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

using std::make_pair; using std::pair;

KnobSignalSlotHandler::KnobSignalSlotHandler(const KnobIPtr& knob)
    : QObject()
    , k(knob)
{
}

void
KnobSignalSlotHandler::onAnimationRemoved(ViewSpec view,
                                          int dimension)
{
    getKnob()->onAnimationRemoved(view, dimension);
}

void
KnobSignalSlotHandler::onMasterKeyFrameSet(double time,
                                           ViewSpec view,
                                           int dimension,
                                           int reason,
                                           bool added)
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );

    assert(handler);
    KnobIPtr master = handler->getKnob();

    getKnob()->clone(master.get(), dimension);
    Q_EMIT keyFrameSet(time, view, dimension, reason, added);
}

void
KnobSignalSlotHandler::onMasterKeyFrameRemoved(double time,
                                               ViewSpec view,
                                               int dimension,
                                               int reason)
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );

    assert(handler);
    KnobIPtr master = handler->getKnob();

    getKnob()->clone(master.get(), dimension);
    Q_EMIT keyFrameRemoved(time, view, dimension, reason);
}

void
KnobSignalSlotHandler::onMasterKeyFrameMoved(ViewSpec view,
                                             int dimension,
                                             double oldTime,
                                             double newTime)
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );

    assert(handler);
    KnobIPtr master = handler->getKnob();

    getKnob()->clone(master.get(), dimension);
    Q_EMIT keyFrameMoved(view, dimension, oldTime, newTime);
}

void
KnobSignalSlotHandler::onMasterAnimationRemoved(ViewSpec view,
                                                int dimension)
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>( sender() );

    assert(handler);
    KnobIPtr master = handler->getKnob();

    getKnob()->clone(master.get(), dimension);
    Q_EMIT animationRemoved(view, dimension);
}

/***************** KNOBI**********************/

bool
KnobI::slaveTo(int dimension,
               const KnobIPtr & other,
               int otherDimension,
               bool ignoreMasterPersistence)
{
    return slaveToInternal(dimension, other, otherDimension, eValueChangedReasonNatronInternalEdited, ignoreMasterPersistence);
}

static KnobIPtr
findMaster(const KnobIPtr & knob,
           const NodesList & allNodes,
           const std::string& masterNodeNameFull,
           const std::string& masterNodeName,
           const std::string& masterTrackName,
           const std::string& masterKnobName,
           const std::map<std::string, std::string>& oldNewScriptNamesMapping)
{
    ///we need to cycle through all the nodes of the project to find the real master
    NodePtr masterNode;

    qDebug() << "Link slave/master for" << knob->getName().c_str() << "restoring linkage" << masterNodeNameFull.c_str() << '|' << masterNodeName.c_str()  << '/' << masterTrackName.c_str() << '/' << masterKnobName.c_str();

    /*
       When copy pasting, the new node copied has a script-name different from what is inside the serialization because 2
       nodes cannot co-exist with the same script-name. We keep in the map the script-names mapping.
       oldNewScriptNamesMapping is set in NodeGraphPrivate::pasteNode()
     */
    std::string masterNodeNameToFind = masterNodeName;
    std::string masterNodeNameFullToFind = masterNodeNameFull;
    {
        std::map<std::string, std::string>::const_iterator foundMapping = oldNewScriptNamesMapping.find(masterNodeName);
        if ( foundMapping != oldNewScriptNamesMapping.end() ) {
            // found it!
            // first, replace the last past of the full name
            if (!masterNodeNameFull.empty()) {
                std::size_t found = masterNodeNameFullToFind.find_last_of('.');
                if (found != std::string::npos && masterNodeNameFullToFind.substr(found+1) == masterNodeName) {
                    masterNodeNameFullToFind = masterNodeNameFullToFind.substr(0, found+1) + foundMapping->second;
                }
            }
            // then update masterNodeNameToFind
            masterNodeNameToFind = foundMapping->second;
        }
    }
    // The following is probably useless: NodeGraphPrivate::pasteNode() only uses the node name, not the fully qualified name.
    // Let us still do that check.
    {
        std::map<std::string, std::string>::const_iterator foundMapping = oldNewScriptNamesMapping.find(masterNodeNameFull.empty() ? masterNodeName : masterNodeNameFull);
        if ( foundMapping != oldNewScriptNamesMapping.end() ) {
            masterNodeNameFullToFind = foundMapping->second;
        }
    }
    if (masterNodeNameFullToFind.empty()) {
        masterNodeNameFullToFind = masterNodeNameToFind;
    }
    for (NodesList::const_iterator it2 = allNodes.begin(); it2 != allNodes.end(); ++it2) {
        qDebug() << "Trying node" << (*it2)->getScriptName().c_str() << '/' << (*it2)->getFullyQualifiedName().c_str();
        if ( (*it2)->getFullyQualifiedName() == masterNodeNameFullToFind ) {
            masterNode = *it2;
            break;
        }
    }
    if (!masterNode) {
        qDebug() << "Link slave/master for" << knob->getName().c_str() << "could not find master node" << masterNodeNameFullToFind.c_str() << "trying without the group name (Natron project files before 2.3.16)...";

        for (NodesList::const_iterator it2 = allNodes.begin(); it2 != allNodes.end(); ++it2) {
            qDebug() << "Trying node" << (*it2)->getScriptName().c_str() << '/' << (*it2)->getFullyQualifiedName().c_str();
            if ( (*it2)->getScriptName() == masterNodeNameToFind ) {
                masterNode = *it2;
                qDebug() << "Got node" << (*it2)->getFullyQualifiedName().c_str();
                break;
            }
        }
    }
    if (!masterNode) {
        qDebug() << "Link slave/master for" << knob->getName().c_str() << "could not find master node" << masterNodeNameFullToFind.c_str() << '|' << masterNodeNameToFind.c_str();

        return KnobIPtr();
    }

    if ( !masterTrackName.empty() ) {
        TrackerContextPtr context = masterNode->getTrackerContext();
        if (context) {
            TrackMarkerPtr masterTrack = context->getMarkerByName(masterTrackName);
            if (!masterTrack) {
                qDebug() << "Link slave/master for" << knob->getName().c_str() << "could not find track" << masterNodeNameToFind.c_str() << '/' << masterTrackName.c_str();
            } else {
                KnobIPtr masterKnob = masterTrack->getKnobByName(masterKnobName);
                if (!masterKnob) {
                    qDebug() << "Link slave/master for" << knob->getName().c_str() << "cound not find knob in track" << masterNodeNameToFind.c_str()  << '/' << masterTrackName.c_str() << '/' << masterKnobName.c_str();
                }
                return masterKnob;
            }
        }
    } else {
        ///now that we have the master node, find the corresponding knob
        const std::vector<KnobIPtr> & otherKnobs = masterNode->getKnobs();
        for (std::size_t j = 0; j < otherKnobs.size(); ++j) {
            qDebug() << "Trying knob" << otherKnobs[j]->getName().c_str();
            // The other knob doesn't need to be persistent (it may be a pushbutton)
            if (otherKnobs[j]->getName() == masterKnobName) {
                return otherKnobs[j];
            }
        }
        qDebug() << "Link slave/master for" << knob->getName().c_str() << "cound not find knob" << masterNodeNameToFind.c_str()  << '/' << masterTrackName.c_str() << '/' << masterKnobName.c_str();
    }

    qDebug() << "Link slave/master for" << knob->getName().c_str() << "failed to restore linkage" << masterNodeNameFullToFind.c_str() << '|' << masterNodeNameToFind.c_str()  << '/' << masterTrackName.c_str() << '/' << masterKnobName.c_str();

    return KnobIPtr();
}

void
KnobI::restoreLinks(const NodesList & allNodes,
                    const std::map<std::string, std::string>& oldNewScriptNamesMapping,
                    bool throwOnFailure)
{
    KnobIPtr thisKnob = shared_from_this();
    for (std::vector<Link>::const_iterator l = _links.begin(); l != _links.end(); ++l) {
        KnobIPtr linkedKnob = findMaster(thisKnob, allNodes, l->nodeNameFull, l->nodeName, l->trackName, l->knobName, oldNewScriptNamesMapping);
        if (!linkedKnob) {
            if (throwOnFailure) {
                throw std::runtime_error(std::string("KnobI::restoreLinks(): Could not restore link to node/knob/track ") + l->nodeName + '/' + l->knobName + '/' + l->trackName);
            } else {
                continue;
            }
        }
        if (linkedKnob == thisKnob) {
            qDebug() << "Link for" << getName().c_str()
            << "failed to restore the following linkage:"
            << "node=" << l->nodeName.c_str()
            << "knob=" << l->knobName.c_str()
            << "track=" << l->trackName.c_str()
            << "because it returned the same Knob. Probably a pre-2.3.16 project with Group clones";
            if (throwOnFailure) {
                throw std::runtime_error(std::string("KnobI::restoreLinks(): Could not restore link to node/knob/track ") + l->nodeName + '/' + l->knobName + '/' + l->trackName + " because it returned the same Knob. Probably a pre-2.3.16 project with Group clones.");
            } else {
                continue;
            }
       } else if (l->dimension == -1) {
            setKnobAsAliasOfThis(linkedKnob, true);
        } else {
            assert(l->slaveDimension >= 0);
            slaveTo(l->slaveDimension, linkedKnob, l->dimension);
        }
    }
    _links.clear();
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

void
KnobI::removeAnimation(ViewSpec view,
                       int dimension)
{
    if ( canAnimate() ) {
        removeAnimationWithReason(view, dimension, eValueChangedReasonNatronInternalEdited);
    }
}

void
KnobI::onAnimationRemoved(ViewSpec view,
                          int dimension)
{
    if ( canAnimate() ) {
        removeAnimationWithReason(view, dimension, eValueChangedReasonUserEdited);
    }
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
    KnobPagePtr isTopLevelParentAPage = boost::dynamic_pointer_cast<KnobPage>(parentKnobTmp);

    return isTopLevelParentAPage;
}

/***********************************KNOB HELPER******************************************/

///for each dimension, the dimension of the master this dimension is linked to, and a pointer to the master
typedef std::vector<std::pair<int, KnobIWPtr> > MastersMap;

///a curve for each dimension
typedef std::vector<CurvePtr> CurvesMap;

struct Expr
{
    std::string expression; //< the one modified by Natron
    std::string originalExpression; //< the one input by the user
    std::string exprInvalid;
    bool hasRet;

    ///The list of pair<knob, dimension> dpendencies for an expression
    std::list<std::pair<KnobIWPtr, int> > dependencies;

    //PyObject* code;

    Expr()
        : expression(), originalExpression(), exprInvalid(), hasRet(false) /*, code(0)*/ {}
};

struct KnobHelperPrivate
{
    KnobHelper* publicInterface;

#ifdef DEBUG
#pragma message WARN("This should be a weak_ptr")
#endif
    KnobHolder* holder;
    mutable QMutex labelMutex;
    std::string label; //< the text label that will be displayed  on the GUI
    std::string iconFilePath[2]; //< an icon to replace the label (one when checked, one when unchecked, for toggable buttons)
    std::string name; //< the knob can have a name different than the label displayed on GUI.
    //By default this is the same as label but can be set by calling setName().
    std::string originalName; //< the original name passed to setName() by the user

    // Gui related stuff
    bool newLine;
    bool addSeparator;
    int itemSpacing;

    // If this knob is supposed to be visible in the Viewer UI, this is the index at which it should be positioned
    int inViewerContextAddSeparator;
    int inViewerContextItemSpacing;
    int inViewerContextAddNewLine;
    std::string inViewerContextLabel;
    bool inViewerContextHasShortcut;
    KnobIWPtr parentKnob;
    mutable QMutex stateMutex; // protects IsSecret defaultIsSecret enabled
    bool IsSecret, defaultIsSecret, inViewerContextSecret;
    std::vector<bool> enabled, defaultEnabled;
    bool CanUndo;
    QMutex evaluateOnChangeMutex;
    bool evaluateOnChange; //< if true, a value change will never trigger an evaluation
    bool IsPersistent; //will it be serialized?
    std::string tooltipHint;
    bool hintIsMarkdown;
    bool isAnimationEnabled;
    int dimension;
    /* the keys for a specific dimension*/
    CurvesMap curves;

    ////curve links
    ///A slave link CANNOT be master at the same time (i.e: if _slaveLinks[i] != NULL  then _masterLinks[i] == NULL )
    mutable QReadWriteLock mastersMutex; //< protects _masters & ignoreMasterPersistence & listeners
    MastersMap masters; //from what knob is slaved each curve if any
    bool ignoreMasterPersistence; //< when true masters will not be serialized

    //Used when this knob is an alias of another knob. The other knob is set in "slaveForAlias"
    KnobIPtr slaveForAlias;

    ///This is a list of all the knobs that have expressions/links to this knob.
    KnobI::ListenerDimsMap listeners;
    mutable QMutex animationLevelMutex;
    std::vector<AnimationLevelEnum> animationLevel; //< indicates for each dimension whether it is static/interpolated/onkeyframe
    bool declaredByPlugin; //< was the knob declared by a plug-in or added by Natron
    bool dynamicallyCreated; //< true if the knob was dynamically created by the user (either via python or via the gui)
    bool userKnob; //< true if it was created by the user and should be put into the "User" page

    ///Pointer to the ofx param overlay interact
    OfxParamOverlayInteractPtr customInteract;

    ///Pointer to the knobGui interface if it has any
    KnobGuiIWPtr gui;
    mutable QMutex mustCloneGuiCurvesMutex;
    /// Set to true if gui curves were modified by the user instead of the real internal curves.
    /// If true then when finished rendering, the knob should clone the guiCurves into the internal curves.
    std::vector<bool> mustCloneGuiCurves;
    std::vector<bool> mustCloneInternalCurves;

    ///Used by deQueueValuesSet to know whether we should clear expressions results or not
    std::vector<bool> mustClearExprResults;

    ///A blind handle to the ofx param, needed for custom overlay interacts
    void* ofxParamHandle;

    ///This is to deal with multi-instance effects such as the Tracker: instance specifics knobs are
    ///not shared between instances whereas non instance specifics are shared.
    bool isInstanceSpecific;
    std::vector<std::string> dimensionNames;
    mutable QMutex expressionMutex;
    std::vector<Expr> expressions;
    mutable QMutex lastRandomHashMutex;
    mutable U32 lastRandomHash;

    ///Used to prevent recursive calls for expressions
    boost::shared_ptr<TLSHolder<KnobHelper::KnobTLSData> > tlsData;
    mutable QMutex hasModificationsMutex;
    std::vector<bool> hasModifications;
    mutable QMutex valueChangedBlockedMutex;
    int valueChangedBlocked; // protected by valueChangedBlockedMutex
    int listenersNotificationBlocked; // protected by valueChangedBlockedMutex
    bool isClipPreferenceSlave;

    KnobHelperPrivate(KnobHelper* publicInterface_,
                      KnobHolder*  holder_,
                      int dimension_,
                      const std::string & label_,
                      bool declaredByPlugin_)
        : publicInterface(publicInterface_)
        , holder(holder_)
        , labelMutex()
        , label(label_)
        , iconFilePath()
        , name( label_.c_str() )
        , originalName( label_.c_str() )
        , newLine(true)
        , addSeparator(false)
        , itemSpacing(0)
        , inViewerContextAddSeparator(false)
        , inViewerContextItemSpacing(5)
        , inViewerContextAddNewLine(false)
        , inViewerContextLabel()
        , inViewerContextHasShortcut(false)
        , parentKnob()
        , stateMutex()
        , IsSecret(false)
        , defaultIsSecret(false)
        , inViewerContextSecret(false)
        , enabled(dimension_)
        , defaultEnabled(dimension_)
        , CanUndo(true)
        , evaluateOnChangeMutex()
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
        , mustClearExprResults()
        , ofxParamHandle(0)
        , isInstanceSpecific(false)
        , dimensionNames(dimension_)
        , expressionMutex()
        , expressions()
        , lastRandomHash(0)
        , tlsData()
        , hasModificationsMutex()
        , hasModifications()
        , valueChangedBlockedMutex()
        , valueChangedBlocked(0)
        , listenersNotificationBlocked(0)
        , isClipPreferenceSlave(false)
    {
        tlsData = boost::make_shared<TLSHolder<KnobHelper::KnobTLSData> >();
        if ( holder && !holder->canKnobsAnimate() ) {
            isAnimationEnabled = false;
        }

        mustCloneGuiCurves.resize(dimension);
        mustCloneInternalCurves.resize(dimension);
        mustClearExprResults.resize(dimension);
        expressions.resize(dimension);
        hasModifications.resize(dimension);
        for (int i = 0; i < dimension_; ++i) {
            defaultEnabled[i] = enabled[i] = true;
            mustCloneGuiCurves[i] = false;
            mustCloneInternalCurves[i] = false;
            mustClearExprResults[i] = false;
            hasModifications[i] = false;
        }
    }

    void parseListenersFromExpression(int dimension);

    std::string declarePythonVariables(bool addTab, int dimension);

    bool shouldUseGuiCurve() const
    {
        if (!holder) {
            return false;
        }

        return !holder->isSetValueCurrentlyPossible() && gui.lock();
    }
};

KnobHelper::KnobHelper(KnobHolder* holder,
                       const std::string &label,
                       int dimension,
                       bool declaredByPlugin)
    : _signalSlotHandler()
    , _imp( new KnobHelperPrivate(this, holder, dimension, label, declaredByPlugin) )
{
}

KnobHelper::~KnobHelper()
{
}

void
KnobHelper::setHolder(KnobHolder* holder)
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
        for (int i = 0; i < knob->getDimension(); ++i) {
            knob->setExpressionInvalid( i, false, tr("%1: parameter does not exist").arg( QString::fromUtf8( getName().c_str() ) ).toStdString() );
            knob->unSlave(i, false);
        }
    }

    KnobHolder* holder = getHolder();
    if ( holder && holder->getApp() ) {
        holder->getApp()->recheckInvalidExpressions();
    }

    for (int i = 0; i < getDimension(); ++i) {
        clearExpression(i, true);
    }

    resetParent();

    if (holder) {
        KnobGroup* isGrp =  dynamic_cast<KnobGroup*>(this);
        KnobPage* isPage = dynamic_cast<KnobPage*>(this);
        if (isGrp) {
            KnobsVec children = isGrp->getChildren();
            for (KnobsVec::iterator it = children.begin(); it != children.end(); ++it) {
                _imp->holder->deleteKnob(it->get(), true);
            }
        } else if (isPage) {
            KnobsVec children = isPage->getChildren();
            for (KnobsVec::iterator it = children.begin(); it != children.end(); ++it) {
                _imp->holder->deleteKnob(it->get(), true);
            }
        }

        EffectInstance* effect = dynamic_cast<EffectInstance*>(holder);
        if (effect) {
            if ( useHostOverlayHandle() ) {
                effect->getNode()->removePositionHostOverlay(this);
            }
            effect->getNode()->removeParameterFromPython( getName() );
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

bool
KnobHelper::isDeclaredByPlugin() const
{
    return _imp->declaredByPlugin;
}

void
KnobHelper::setAsInstanceSpecific()
{
    _imp->isInstanceSpecific = true;
}

bool
KnobHelper::isInstanceSpecific() const
{
    return _imp->isInstanceSpecific;
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
    KnobSignalSlotHandlerPtr handler = boost::make_shared<KnobSignalSlotHandler>(thisKnob);

    setSignalSlotHandler(handler);

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
            _imp->curves[i] = boost::make_shared<Curve>(this, i);
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
    if (_signalSlotHandler) {
        _signalSlotHandler->s_dimensionNameChanged(dim);
    }
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
KnobHelper::setSignalSlotHandler(const KnobSignalSlotHandlerPtr & handler)
{
    _signalSlotHandler = handler;
}

void
KnobHelper::deleteValuesAtTime(CurveChangeReason curveChangeReason,
                               const std::list<double>& times,
                               ViewSpec view,
                               int dimension, bool copyCurveValueAtTimeToInternalValue)
{
    if ( ( dimension > (int)_imp->curves.size() ) || (dimension < 0) ) {
        throw std::invalid_argument("KnobHelper::deleteValueAtTime(): Dimension out of range");
    }

    if ( times.empty() ) {
        return;
    }

    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return;
    }
    CurvePtr curve;
    bool useGuiCurve = _imp->shouldUseGuiCurve();
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (!useGuiCurve) {
        curve = _imp->curves[dimension];
    } else {
        curve = hasGui->getCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension, true);
    }

    assert(curve);

    // We are about to remove the last keyframe, ensure that the internal value of the knob is the one of the animation
    if (copyCurveValueAtTimeToInternalValue) {
        copyValuesFromCurve(dimension);
    }

    try {
        for (std::list<double>::const_iterator it = times.begin(); it != times.end(); ++it) {
            curve->removeKeyFrameWithTime(*it);
        }
    } catch (const std::exception & e) {
        //qDebug() << e.what();
    }

    if (!useGuiCurve && hasGui) {
        CurvePtr guiCurve = hasGui->getCurve(view, dimension);
        assert(guiCurve);
        for (std::list<double>::const_iterator it = times.begin(); it != times.end(); ++it) {
            guiCurve->removeKeyFrameWithTime(*it);
        }
    }


    //virtual portion
    for (std::list<double>::const_iterator it = times.begin(); it != times.end(); ++it) {
        keyframeRemoved_virtual(dimension, *it);
    }


    if (_imp->holder) {
        _imp->holder->updateHasAnimation();
    }


    ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited;

    if (!useGuiCurve) {
        checkAnimationLevel(view, dimension);
        evaluateValueChange(dimension, *times.begin(), view, reason);
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_redrawGuiCurve(curveChangeReason, view, dimension);
    }


    if (_signalSlotHandler /* && reason != eValueChangedReasonUserEdited*/) {
        _signalSlotHandler->s_multipleKeyFramesRemoved(times, view, dimension, (int)reason);
    }
} // KnobHelper::deleteValuesAtTime

void
KnobHelper::deleteValueAtTime(CurveChangeReason curveChangeReason,
                              double time,
                              ViewSpec view,
                              int dimension, bool copyCurveValueAtTimeToInternalValue)
{
    if ( ( dimension > (int)_imp->curves.size() ) || (dimension < 0) ) {
        throw std::invalid_argument("KnobHelper::deleteValueAtTime(): Dimension out of range");
    }

    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return;
    }

    CurvePtr curve;
    bool useGuiCurve = _imp->shouldUseGuiCurve();
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (!useGuiCurve) {
        curve = _imp->curves[dimension];
    } else {
        curve = hasGui->getCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension, true);
    }

    assert(curve);

    // We are about to remove the last keyframe, ensure that the internal value of the knob is the one of the animation
    if (copyCurveValueAtTimeToInternalValue) {
        copyValuesFromCurve(dimension);
    }


    try {
        curve->removeKeyFrameWithTime(time);
    } catch (const std::exception & e) {
        //qDebug() << e.what();
    }

    if (!useGuiCurve && hasGui) {
        CurvePtr guiCurve = hasGui->getCurve(view, dimension);
        assert(guiCurve);
        guiCurve->removeKeyFrameWithTime(time);
    }


    //virtual portion
    keyframeRemoved_virtual(dimension, time);


    if (_imp->holder) {
        _imp->holder->updateHasAnimation();
    }


    ValueChangedReasonEnum reason = eValueChangedReasonNatronInternalEdited;

    if (!useGuiCurve) {
        checkAnimationLevel(view, dimension);
        evaluateValueChange(dimension, time, view, reason);
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_redrawGuiCurve(curveChangeReason, view, dimension);
    }


    if (_signalSlotHandler /* && reason != eValueChangedReasonUserEdited*/) {
        _signalSlotHandler->s_keyFrameRemoved(time, view, dimension, (int)reason);
    }
} // KnobHelper::deleteValueAtTime

void
KnobHelper::onKeyFrameRemoved(double time,
                              ViewSpec view,
                              int dimension,
                              bool copyCurveValueAtTimeToInternalValue)
{
    deleteValueAtTime(eCurveChangeReasonInternal, time, view, dimension, copyCurveValueAtTimeToInternalValue);
}

bool
KnobHelper::moveValuesAtTime(CurveChangeReason reason,
                             ViewSpec view,
                             int dimension,
                             double dt,
                             double dv,
                             std::vector<KeyFrame>* keyframes)
{
    assert(keyframes);
    assert( QThread::currentThread() == qApp->thread() );
    assert( dimension >= 0 && dimension < (int)_imp->curves.size() );

    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return false;
    }

    /*
       We write on the "GUI" curve if the engine is either:
       - using it
       - is still marked as different from the "internal" curve
     */

    bool useGuiCurve = _imp->shouldUseGuiCurve();

    for (std::size_t i = 0; i < keyframes->size(); ++i) {
        if ( !moveValueAtTimeInternal(useGuiCurve, (*keyframes)[i].getTime(), view, dimension, dt, dv, &(*keyframes)[i]) ) {
            return false;
        }
    }


    if (!useGuiCurve) {
        evaluateValueChange(dimension, getCurrentTime(), view, eValueChangedReasonNatronInternalEdited);
    } else {
        setGuiCurveHasChanged(view, dimension, true);
    }
    //notify that the gui curve has changed to redraw it
    if (_signalSlotHandler) {
        _signalSlotHandler->s_redrawGuiCurve(reason, view, dimension);
    }

    return true;
}

bool
KnobHelper::moveValueAtTimeInternal(bool useGuiCurve,
                                    double time,
                                    ViewSpec view,
                                    int dimension,
                                    double dt,
                                    double dv,
                                    KeyFrame* newKey)
{
    CurvePtr curve;
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (!useGuiCurve) {
        curve = _imp->curves[dimension];
    } else {
        assert(hasGui);
        curve = hasGui->getCurve(view, dimension);
    }
    assert(curve);

    if ( !curve->moveKeyFrameValueAndTime(time, dt, dv, newKey) ) {
        return false;
    }
    if (!useGuiCurve && hasGui) {
        CurvePtr guiCurve = hasGui->getCurve(view, dimension);
        assert(guiCurve);
        guiCurve->moveKeyFrameValueAndTime(time, dt, dv, 0);
    }


    ///Make sure string animation follows up
    AnimatingKnobStringHelper* isString = dynamic_cast<AnimatingKnobStringHelper*>(this);
    std::string v;
    if (isString) {
        isString->stringFromInterpolatedValue(time, view, &v);
    }
    keyframeRemoved_virtual(dimension, time);
    if (isString) {
        double ret;
        isString->stringToKeyFrameValue(newKey->getTime(), view, v, &ret);
    }


    if (_signalSlotHandler) {
        _signalSlotHandler->s_keyFrameMoved( view, dimension, time, newKey->getTime() );
    }

    return true;
}

bool
KnobHelper::moveValueAtTime(CurveChangeReason reason,
                            double time,
                            ViewSpec view,
                            int dimension,
                            double dt,
                            double dv,
                            KeyFrame* newKey)
{
    assert( QThread::currentThread() == qApp->thread() );
    assert( dimension >= 0 && dimension < (int)_imp->curves.size() );

    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return false;
    }

    /*
       We write on the "GUI" curve if the engine is either:
       - using it
       - is still marked as different from the "internal" curve
     */
    bool useGuiCurve = _imp->shouldUseGuiCurve();

    if ( !moveValueAtTimeInternal(useGuiCurve, time, view, dimension, dt, dv, newKey) ) {
        return false;
    }

    if (!useGuiCurve) {
        evaluateValueChange(dimension, newKey->getTime(), view, eValueChangedReasonNatronInternalEdited);
    } else {
        setGuiCurveHasChanged(view, dimension, true);
    }
    //notify that the gui curve has changed to redraw it
    if (_signalSlotHandler) {
        _signalSlotHandler->s_redrawGuiCurve(reason, view, dimension);
    }

    return true;
}

bool
KnobHelper::transformValuesAtTime(CurveChangeReason curveChangeReason,
                                  ViewSpec view,
                                  int dimension,
                                  const Transform::Matrix3x3& matrix,
                                  std::vector<KeyFrame>* keyframes)
{
    assert( QThread::currentThread() == qApp->thread() );
    assert( dimension >= 0 && dimension < (int)_imp->curves.size() );

    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return false;
    }

    bool useGuiCurve = _imp->shouldUseGuiCurve();

    for (std::size_t i = 0; i < keyframes->size(); ++i) {
        if ( !transformValueAtTimeInternal(useGuiCurve, (*keyframes)[i].getTime(), view, dimension, matrix, &(*keyframes)[i]) ) {
            return false;
        }
    }

    if (_signalSlotHandler) {
        _signalSlotHandler->s_redrawGuiCurve(curveChangeReason, view, dimension);
    }

    if (!useGuiCurve) {
        evaluateValueChange(dimension, getCurrentTime(), view,  eValueChangedReasonNatronInternalEdited);
    } else {
        setGuiCurveHasChanged(view, dimension, true);
    }

    return true;
}

bool
KnobHelper::transformValueAtTimeInternal(bool useGuiCurve,
                                         double time,
                                         ViewSpec view,
                                         int dimension,
                                         const Transform::Matrix3x3& matrix,
                                         KeyFrame* newKey)
{
    CurvePtr curve;
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (!useGuiCurve) {
        curve = _imp->curves[dimension];
    } else {
        assert(hasGui);
        curve = hasGui->getCurve(view, dimension);
    }
    assert(curve);

    KeyFrame k;
    int keyindex = curve->keyFrameIndex(time);
    if (keyindex == -1) {
        return false;
    }

    bool gotKey = curve->getKeyFrameWithIndex(keyindex, &k);
    if (!gotKey) {
        return false;
    }

    Transform::Point3D p;
    p.x = k.getTime();
    p.y = k.getValue();
    p.z = 1;

    p = Transform::matApply(matrix, p);


    if ( curve->areKeyFramesValuesClampedToIntegers() ) {
        p.y = std::floor(p.y + 0.5);
    } else if ( curve->areKeyFramesValuesClampedToBooleans() ) {
        p.y = p.y < 0.5 ? 0 : 1;
    }

    ///Make sure string animation follows up
    AnimatingKnobStringHelper* isString = dynamic_cast<AnimatingKnobStringHelper*>(this);
    std::string v;
    if (isString) {
        isString->stringFromInterpolatedValue(k.getValue(), view, &v);
    }
    keyframeRemoved_virtual(dimension, time);
    if (isString) {
        double ret;
        isString->stringToKeyFrameValue(p.x, view, v, &ret);
    }


    try {
        *newKey = curve->setKeyFrameValueAndTime(p.x, p.y, keyindex, NULL);
    } catch (...) {
        return false;
    }

    if (!useGuiCurve && hasGui) {
        CurvePtr guiCurve = hasGui->getCurve(view, dimension);
        try {
            guiCurve->setKeyFrameValueAndTime(p.x, p.y, keyindex, NULL);
        } catch (...) {
            return false;
        }
    }


    if (_signalSlotHandler) {
        _signalSlotHandler->s_keyFrameMoved(view, dimension, time, p.x);
    }

    return true;
} // KnobHelper::transformValueAtTimeInternal

bool
KnobHelper::transformValueAtTime(CurveChangeReason curveChangeReason,
                                 double time,
                                 ViewSpec view,
                                 int dimension,
                                 const Transform::Matrix3x3& matrix,
                                 KeyFrame* newKey)
{
    assert( QThread::currentThread() == qApp->thread() );
    assert( dimension >= 0 && dimension < (int)_imp->curves.size() );

    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return false;
    }


    bool useGuiCurve = _imp->shouldUseGuiCurve();

    if ( !transformValueAtTimeInternal(useGuiCurve, time, view, dimension, matrix, newKey) ) {
        return false;
    }

    if (_signalSlotHandler) {
        _signalSlotHandler->s_redrawGuiCurve(curveChangeReason, view, dimension);
    }

    if (!useGuiCurve) {
        evaluateValueChange(dimension, getCurrentTime(), view,  eValueChangedReasonNatronInternalEdited);
    } else {
        setGuiCurveHasChanged(view, dimension, true);
    }

    return true;
}

void
KnobHelper::cloneCurve(ViewSpec view,
                       int dimension,
                       const Curve& curve)
{
    assert( dimension >= 0 && dimension < (int)_imp->curves.size() );
    CurvePtr thisCurve;
    KnobGuiIPtr hasGui = getKnobGuiPointer();
    bool useGuiCurve = _imp->shouldUseGuiCurve();
    if (!useGuiCurve) {
        thisCurve = _imp->curves[dimension];
    } else {
        assert(hasGui);
        thisCurve = hasGui->getCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension, true);
    }
    assert(thisCurve);

    if (_signalSlotHandler) {
        _signalSlotHandler->s_animationAboutToBeRemoved(view, dimension);
        _signalSlotHandler->s_animationRemoved(view, dimension);
    }
    animationRemoved_virtual(dimension);
    thisCurve->clone(curve);
    if (!useGuiCurve) {
        evaluateValueChange(dimension, getCurrentTime(), view,  eValueChangedReasonNatronInternalEdited);
        guiCurveCloneInternalCurve(eCurveChangeReasonInternal, view, dimension, eValueChangedReasonNatronInternalEdited);
    }

    if (_signalSlotHandler) {
        std::list<double> keysList;
        KeyFrameSet keys = thisCurve->getKeyFrames_mt_safe();
        for (KeyFrameSet::iterator it = keys.begin(); it != keys.end(); ++it) {
            keysList.push_back( it->getTime() );
        }
        if ( !keysList.empty() ) {
            _signalSlotHandler->s_multipleKeyFramesSet(keysList, view, dimension, (int)eValueChangedReasonNatronInternalEdited);
        }
    }
}

bool
KnobHelper::setInterpolationAtTime(CurveChangeReason reason,
                                   ViewSpec view,
                                   int dimension,
                                   double time,
                                   KeyframeTypeEnum interpolation,
                                   KeyFrame* newKey)
{
    assert( QThread::currentThread() == qApp->thread() );
    assert( dimension >= 0 && dimension < (int)_imp->curves.size() );

    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return false;
    }

    CurvePtr curve;
    bool useGuiCurve = _imp->shouldUseGuiCurve();
    KnobGuiIPtr hasGui = getKnobGuiPointer();
    if (!useGuiCurve) {
        curve = _imp->curves[dimension];
    } else {
        assert(hasGui);
        curve = hasGui->getCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension, true);
    }
    assert(curve);

    int keyIndex = curve->keyFrameIndex(time);
    if (keyIndex == -1) {
        return false;
    }

    *newKey = curve->setKeyFrameInterpolation(interpolation, keyIndex);

    if (!useGuiCurve && hasGui) {
        CurvePtr guiCurve = hasGui->getCurve(view, dimension);
        assert(guiCurve);
        guiCurve->setKeyFrameInterpolation(interpolation, keyIndex);
    }

    if (!useGuiCurve) {
        evaluateValueChange(dimension, time, view, eValueChangedReasonNatronInternalEdited);
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_redrawGuiCurve(reason, view, dimension);
    }

    if (_signalSlotHandler) {
        _signalSlotHandler->s_keyFrameInterpolationChanged(time, view, dimension);
    }

    return true;
}

bool
KnobHelper::moveDerivativesAtTime(CurveChangeReason reason,
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

    CurvePtr curve;
    KnobGuiIPtr hasGui = getKnobGuiPointer();
    bool useGuiCurve = _imp->shouldUseGuiCurve();

    if (!useGuiCurve) {
        curve = _imp->curves[dimension];
    } else {
        assert(hasGui);
        curve = hasGui->getCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension, true);
    }

    assert(curve);

    int keyIndex = curve->keyFrameIndex(time);
    if (keyIndex == -1) {
        return false;
    }

    curve->setKeyFrameInterpolation(eKeyframeTypeFree, keyIndex);
    curve->setKeyFrameDerivatives(left, right, keyIndex);

    if (!useGuiCurve && hasGui) {
        CurvePtr guiCurve = hasGui->getCurve(view, dimension);
        assert(guiCurve);
        guiCurve->setKeyFrameInterpolation(eKeyframeTypeFree, keyIndex);
        guiCurve->setKeyFrameDerivatives(left, right, keyIndex);
    }

    if (!useGuiCurve) {
        evaluateValueChange(dimension, time, view, eValueChangedReasonNatronInternalEdited);
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_redrawGuiCurve(reason, view, dimension);
    }

    if (_signalSlotHandler) {
        _signalSlotHandler->s_derivativeMoved(time, view, dimension);
    }

    return true;
} // KnobHelper::moveDerivativesAtTime

bool
KnobHelper::moveDerivativeAtTime(CurveChangeReason reason,
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

    CurvePtr curve;
    bool useGuiCurve = _imp->shouldUseGuiCurve();
    KnobGuiIPtr hasGui = getKnobGuiPointer();
    if (!useGuiCurve) {
        curve = _imp->curves[dimension];
    } else {
        assert(hasGui);
        curve = hasGui->getCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension, true);
    }
    assert(curve);

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

    if (!useGuiCurve && hasGui) {
        CurvePtr guiCurve = hasGui->getCurve(view, dimension);
        assert(guiCurve);
        guiCurve->setKeyFrameInterpolation(eKeyframeTypeBroken, keyIndex);
        if (isLeft) {
            guiCurve->setKeyFrameLeftDerivative(derivative, keyIndex);
        } else {
            guiCurve->setKeyFrameRightDerivative(derivative, keyIndex);
        }
    }


    if (!useGuiCurve) {
        evaluateValueChange(dimension, time, view, eValueChangedReasonNatronInternalEdited);
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_redrawGuiCurve(reason, view, dimension);
    }

    if (_signalSlotHandler) {
        _signalSlotHandler->s_derivativeMoved(time, view, dimension);
    }

    return true;
} // KnobHelper::moveDerivativeAtTime

void
KnobHelper::removeAnimationWithReason(ViewSpec view,
                                      int dimension,
                                      ValueChangedReasonEnum reason)
{
    assert(0 <= dimension);
    if ( (dimension < 0) || ( (int)_imp->curves.size() <= dimension ) ) {
        throw std::invalid_argument("KnobHelper::removeAnimationWithReason(): Dimension out of range");
    }


    if ( !canAnimate() || !isAnimated(dimension, view) ) {
        return;
    }

    CurvePtr curve;
    KnobGuiIPtr hasGui = getKnobGuiPointer();
    bool useGuiCurve = _imp->shouldUseGuiCurve();

    if (!useGuiCurve) {
        curve = _imp->curves[dimension];
    } else {
        assert(hasGui);
        curve = hasGui->getCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension, true);
    }

    if ( _signalSlotHandler && (reason != eValueChangedReasonUserEdited) ) {
        _signalSlotHandler->s_animationAboutToBeRemoved(view, dimension);
    }

    copyValuesFromCurve(dimension);

    assert(curve);
    if (curve) {
        curve->clearKeyFrames();
    }

    if ( _signalSlotHandler && (reason != eValueChangedReasonUserEdited) ) {
        _signalSlotHandler->s_animationRemoved(view, dimension);
    }

    animationRemoved_virtual(dimension);

    if (_imp->holder) {
        _imp->holder->updateHasAnimation();
    }


    if (!useGuiCurve) {
        //virtual portion
        evaluateValueChange(dimension, getCurrentTime(), view, reason);
        guiCurveCloneInternalCurve(eCurveChangeReasonInternal, view, dimension, reason);
    } else {
        if (_signalSlotHandler) {
            _signalSlotHandler->s_redrawGuiCurve(eCurveChangeReasonInternal, view, dimension);
        }
    }
} // KnobHelper::removeAnimationWithReason

void
KnobHelper::clearExpressionsResultsIfNeeded(std::map<int, ValueChangedReasonEnum>& modifiedDimensions)
{
    QMutexLocker k(&_imp->mustCloneGuiCurvesMutex);

    for (int i = 0; i < getDimension(); ++i) {
        if (_imp->mustClearExprResults[i]) {
            clearExpressionsResults(i);
            _imp->mustClearExprResults[i] = false;
            modifiedDimensions.insert( std::make_pair(i, eValueChangedReasonNatronInternalEdited) );
        }
    }
}

void
KnobHelper::cloneInternalCurvesIfNeeded(std::map<int, ValueChangedReasonEnum>& modifiedDimensions)
{
    QMutexLocker k(&_imp->mustCloneGuiCurvesMutex);

    for (int i = 0; i < getDimension(); ++i) {
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
    for (int i = 0; i < getDimension(); ++i) {
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
    if (hasChanged && _imp->holder) {
        _imp->holder->updateHasAnimation();
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

    std::pair<int, KnobIPtr> master = getMaster(dimension);
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

CurvePtr KnobHelper::getCurve(ViewSpec view,
                                              int dimension,
                                              bool byPassMaster) const
{
    if ( (dimension < 0) || ( dimension >= (int)_imp->curves.size() ) ) {
        return CurvePtr();
    }

    std::pair<int, KnobIPtr> master = getMaster(dimension);
    if (!byPassMaster && master.second) {
        return master.second->getCurve(view, master.first);
    }

    return _imp->curves[dimension];
}

bool
KnobHelper::isAnimated(int dimension,
                       ViewSpec view) const
{
    if ( !canAnimate() ) {
        return false;
    }
    CurvePtr curve = getCurve(view, dimension);
    assert(curve);

    return curve->isAnimated();
}

const std::vector<CurvePtr> &
KnobHelper::getCurves() const
{
    return _imp->curves;
}

int
KnobHelper::getDimension() const
{
    return _imp->dimension;
}

void
KnobHelper::beginChanges()
{
    if (_imp->holder) {
        _imp->holder->beginChanges();
    }
}

void
KnobHelper::endChanges()
{
    if (_imp->holder) {
        _imp->holder->endChanges();
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

bool
KnobHelper::evaluateValueChangeInternal(int dimension,
                                        double time,
                                        ViewSpec view,
                                        ValueChangedReasonEnum reason,
                                        ValueChangedReasonEnum originalReason)
{
    AppInstancePtr app;

    if (_imp->holder) {
        app = _imp->holder->getApp();
    }
    KnobGuiIPtr hasGui = getKnobGuiPointer();
    bool refreshWidget = !app || hasAnimation() || time == app->getTimeLine()->currentFrame();

    /// For eValueChangedReasonTimeChanged we never call the instanceChangedAction and evaluate otherwise it would just throttle
    /// the application responsiveness
    onInternalValueChanged(dimension, time, view);

    bool ret = false;
    if ( ( (originalReason != eValueChangedReasonTimeChanged) || evaluateValueChangeOnTimeChange() ) && _imp->holder ) {
        _imp->holder->beginChanges();
        KnobIPtr thisShared = shared_from_this();
        assert(thisShared);
        _imp->holder->appendValueChange(thisShared, dimension, refreshWidget, time, view, originalReason, reason);
        ret |= _imp->holder->endChanges();
    }


    if (!_imp->holder) {
        computeHasModifications();
        if (refreshWidget && _signalSlotHandler) {
            _signalSlotHandler->s_valueChanged(view, dimension, (int)reason);
        }
        if ( !isListenersNotificationBlocked() ) {
            refreshListenersAfterValueChange(view, originalReason, dimension);
        }
        checkAnimationLevel(view, dimension);
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
    if (_signalSlotHandler) {
        _signalSlotHandler->s_inViewerContextLabelChanged();
    }
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
KnobHelper::setInViewerContextAddSeparator(bool addSeparator)
{
    _imp->inViewerContextAddSeparator = addSeparator;
}

bool
KnobHelper::getInViewerContextAddSeparator() const
{
    return _imp->inViewerContextAddSeparator;
}

void
KnobHelper::setInViewerContextNewLineActivated(bool activated)
{
    _imp->inViewerContextAddNewLine = activated;
}

bool
KnobHelper::getInViewerContextNewLineActivated() const
{
    return _imp->inViewerContextAddNewLine;
}

void
KnobHelper::setInViewerContextSecret(bool secret)
{
    {
        QMutexLocker k(&_imp->stateMutex);
        _imp->inViewerContextSecret = secret;
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_viewerContextSecretChanged();
    }
}

bool
KnobHelper::getInViewerContextSecret() const
{
    QMutexLocker k(&_imp->stateMutex);

    return _imp->inViewerContextSecret;
}

void
KnobHelper::setEnabled(int dimension,
                       bool b)
{
    {
        QMutexLocker k(&_imp->stateMutex);
        _imp->enabled[dimension] = b;
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_enabledChanged();
    }
}

void
KnobHelper::setDefaultEnabled(int dimension,
                              bool b)
{
    {
        QMutexLocker k(&_imp->stateMutex);
        _imp->defaultEnabled[dimension] = b;
    }
    setEnabled(dimension, b);
}

void
KnobHelper::setAllDimensionsEnabled(bool b)
{
    bool changed = false;
    {
        QMutexLocker k(&_imp->stateMutex);
        for (U32 i = 0; i < _imp->enabled.size(); ++i) {
            if (b != _imp->enabled[i]) {
                _imp->enabled[i] = b;
                changed = true;
            }
        }
    }

    if (changed && _signalSlotHandler) {
        _signalSlotHandler->s_enabledChanged();
    }
}

void
KnobHelper::setDefaultAllDimensionsEnabled(bool b)
{
    {
        QMutexLocker k(&_imp->stateMutex);
        for (U32 i = 0; i < _imp->enabled.size(); ++i) {
            _imp->defaultEnabled[i] = b;
        }
    }
    setAllDimensionsEnabled(b);
}

void
KnobHelper::setSecretByDefault(bool b)
{
    {
        QMutexLocker k(&_imp->stateMutex);
        _imp->defaultIsSecret = b;
    }
    setSecret(b);
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
    if ( !b && _imp->holder && _imp->holder->getApp() ) {
        onTimeChanged( false, _imp->holder->getApp()->getTimeLine()->currentFrame() );
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
                         bool checked)
{
    QMutexLocker k(&_imp->labelMutex);
    int idx = !checked ? 0 : 1;

    _imp->iconFilePath[idx] = iconFilePath;
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
    for (int i = 0; i < getDimension(); ++i) {
        if (getKeyFramesCount(ViewIdx(0), i) > 0) {
            return true;
        }
        if ( !getExpression(i).empty() ) {
            return true;
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

// Extract a list of parameters between two parentheses.
// This code looks very fragile.
// Whitespace handling was added, but it seems like anything that looks like a more
// complicated expression with operators would break (see https://github.com/NatronGitHub/Natron/issues/448).
static void
extractParameters(std::size_t startParenthesis,
                  std::size_t endParenthesis,
                  const std::string& str,
                  std::vector<std::string>* params)
{
    std::size_t i = startParenthesis + 1;
    int insideParenthesis = 0;

    while ( i < str.size() && (i < endParenthesis || insideParenthesis < 0) ) {
        std::string curParam;
        std::size_t prev_i = i;
        while ( i < str.size() && std::isspace( str.at(i) ) ) {
            ++i;
        }
        if ( i >= str.size() ) {
            break;
        }
        if (str.at(i) == '(') {
            ++insideParenthesis;
            ++i;
        } else if (str.at(i) == ')') {
            --insideParenthesis;
            ++i;
        }
        while ( i < str.size() && std::isspace( str.at(i) ) ) {
            ++i;
        }
        if ( i >= str.size() ) {
            break;
        }
        while ( i < str.size() && ((!std::isspace( str.at(i) ) && str.at(i) != ',') || insideParenthesis > 0) ) {
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
        while ( i < str.size() && std::isspace( str.at(i) ) ) {
            ++i;
        }
        if ( i >= str.size() ) {
            break;
        }
        if (str.at(i) == ',') {
            ++i;
        }
        if (i == prev_i) {
            assert(false && "this should not happen");
            // We didn't move, this is probably a bug.
            break;
        }
        if ( !curParam.empty() ) {
            params->push_back(curParam);
        }
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

    //Find the start of the symbol
    int i = (int)*tokenStart - 2;
    int nClosingParenthesisMet = 0;
    while (i >= 0) {
        if (str[i] == ')') {
            ++nClosingParenthesisMet;
        }
        if ( std::isspace(str[i]) ||
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
    if (!holder) {
        throw std::runtime_error("This parameter cannot have an expression");
    }

    EffectInstance* effect = dynamic_cast<EffectInstance*>(holder);
    if (!effect) {
        throw std::runtime_error("This parameter cannot have an expression");
    }

    NodePtr node = effect->getNode();
    assert(node);

    NodeCollectionPtr collection = node->getGroup();
    if (!collection) {
        throw std::runtime_error("This parameter cannot have an expression");
    }
    NodeGroup* isParentGrp = dynamic_cast<NodeGroup*>( collection.get() );
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
        if ( (*it)->isActivated() && !(*it)->getParentMultiInstance() ) {
            std::string scriptName = (*it)->getScriptName_mt_safe();
            if ( NATRON_PYTHON_NAMESPACE::isKeyword(scriptName) ) {
                throw std::runtime_error(scriptName + " is a Python keyword");
            }
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
    /*NodeGroup* isHolderGrp = dynamic_cast<NodeGroup*>(effect);
    if (isHolderGrp) {
        NodesList children = isHolderGrp->getNodes();
        for (NodesList::iterator it = children.begin(); it != children.end(); ++it) {
            if ( (*it)->isActivated() && !(*it)->getParentMultiInstance() && (*it)->isPartOfProject() ) {
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

    std::string error;
#if 0
    // the following is OK to detect that there's no "return" in the expression,
    // but fails if the expression contains e.g. "thisGroup"
    // see https://github.com/NatronGitHub/Natron/issues/294

    ///Try to compile the expression and evaluate it, if it doesn't have a good syntax, throw an exception
    ///with the error.
    {
        // first check that the expression does not contain return
        EXPR_RECURSION_LEVEL();

        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(expression, &error, 0) ) {
            throw std::runtime_error(error);
        }
    }
#endif

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

    KnobHolder* holder = getHolder();
    if (!holder) {
        throw std::runtime_error("This parameter cannot have an expression");
    }

    EffectInstance* effect = dynamic_cast<EffectInstance*>(holder);
    if (!effect) {
        throw std::runtime_error("This parameter cannot have an expression");
    }

    NodePtr node = effect->getNode();
    assert(node);
    std::string appID = getHolder()->getApp()->getAppIDString();
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

    std::string funcExecScript = "ret = " + exprFuncPrefix + exprFuncName;

    ///Try to compile the expression and evaluate it, if it doesn't have a good syntax, throw an exception
    ///with the error.
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
            if (PyUnicode_Check(ret) || PyString_Check(ret)) {
                *resultAsString = isString->pyObjectToType<std::string>(ret);
            } else {
                int index = 0;
                if ( PyFloat_Check(ret) ) {
                    index = std::floor( (double)PyFloat_AsDouble(ret) + 0.5 );
                } else if ( PyLong_Check(ret) ) {
                    index = (int)PyInt_AsLong(ret);
                } else if (PyObject_IsTrue(ret) == 1) {
                    index = 1;
                }

                const AnimatingKnobStringHelper* isStringAnimated = dynamic_cast<const AnimatingKnobStringHelper* >(this);
                if (!isStringAnimated) {
                    return std::string();
                }
                isStringAnimated->stringFromInterpolatedValue(index, ViewSpec::current(), resultAsString);
            }
        }
    }


    return funcExecScript;
} // KnobHelper::validateExpression

bool
KnobHelper::checkInvalidExpressions()
{
    int ndims = getDimension();
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
    int ndims = getDimension();

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
    int ndims = getDimension();

    if ( (dimension < 0) || (dimension >= ndims) ) {
        return;
    }
    bool wasValid;
    {
        QMutexLocker k(&_imp->expressionMutex);
        wasValid = _imp->expressions[dimension].exprInvalid.empty();
        _imp->expressions[dimension].exprInvalid = error;
    }
    if ( getHolder() && getHolder()->getApp() ) {
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
                getHolder()->getApp()->removeInvalidExpressionKnob(this);
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
    assert( dimension >= 0 && dimension < getDimension() );

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
            throw e;
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

    if ( getHolder() ) {
        //Parse listeners of the expression, to keep track of dependencies to indicate them to the user.

        if ( exprInvalid.empty() ) {
            EXPR_RECURSION_LEVEL();
            _imp->parseListenersFromExpression(dimension);
        } else {
            AppInstancePtr app = getHolder()->getApp();
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
    KnobHolder* holder = getHolder();
    if (!holder) {
        return;
    }
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
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
            assert(other);
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

            if ( getHolder() ) {
                getHolder()->onKnobSlaved(thisShared, otherKnob, dimension, false );
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
    if (_imp->holder) {
        _imp->holder->updateHasAnimation();
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
    PythonGILLocker pgl;

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
    std::stringstream ss;

    ss << expr << '(' << time << ", " <<  view << ")\n";

    return executeExpression(ss.str(), ret, error);
}


bool
KnobHelper::executeExpression(const std::string& expr,
                              PyObject** ret,
                              std::string* error)
{
    PythonGILLocker pgl;

    //returns a new ref, this function's documentation is not clear onto what it returns...
    //https://docs.python.org/2/c-api/veryhigh.html
    PyObject* mainModule = NATRON_PYTHON_NAMESPACE::getMainModule();
    PyObject* globalDict = PyModule_GetDict(mainModule);

    PyErr_Clear();

    PyObject* v = PyRun_String(expr.c_str(), Py_file_input, globalDict, 0);
    Py_XDECREF(v);

    *ret = 0;

    if ( !catchErrors(mainModule, error) ) {
        return false;
    }
    *ret = PyObject_GetAttrString(mainModule, "ret"); //get our ret variable created above
    if (!*ret) {
        // Do not forget to empty the error stream using catchError, even if we know the error,
        // for subsequent expression evaluations.
        if ( catchErrors(mainModule, error) ) {
            *error = "Missing 'ret' attribute";
        }

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

KnobHolder*
KnobHelper::getHolder() const
{
    return _imp->holder;
}

void
KnobHelper::setAnimationEnabled(bool val)
{
    if ( !canAnimate() ) {
        return;
    }
    if ( _imp->holder && !_imp->holder->canKnobsAnimate() ) {
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

    if ( !getHolder() ) {
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
        if ( getHolder()->getOtherKnobByName(finalName, this) ) {
            foundItem = true;
        } else {
            foundItem = false;
        }
        ++no;
    } while (foundItem);


    EffectInstance* effect = dynamic_cast<EffectInstance*>(_imp->holder);
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
        KnobGroup* isGrp =  dynamic_cast<KnobGroup*>( parent.get() );
        KnobPage* isPage = dynamic_cast<KnobPage*>( parent.get() );
        if (isGrp) {
            isGrp->removeKnob(this);
        } else if (isPage) {
            isPage->removeKnob(this);
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

bool
KnobHelper::getDefaultIsSecret() const
{
    QMutexLocker k(&_imp->stateMutex);

    return _imp->defaultIsSecret;
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
    assert( 0 <= dimension && dimension < getDimension() );

    QMutexLocker k(&_imp->stateMutex);

    return _imp->enabled[dimension];
}

bool
KnobHelper::isDefaultEnabled(int dimension) const
{
    assert( 0 <= dimension && dimension < getDimension() );

    QMutexLocker k(&_imp->stateMutex);

    return _imp->defaultEnabled[dimension];
}

void
KnobHelper::setDirty(bool d)
{
    if (_signalSlotHandler) {
        _signalSlotHandler->s_setDirty(d);
    }
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
        QMutexLocker k(&_imp->evaluateOnChangeMutex);
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
    QMutexLocker k(&_imp->evaluateOnChangeMutex);

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

#ifdef OFX_EXTENSIONS_NATRON
double
KnobHelper::getScreenPixelRatio() const
{
    KnobGuiIPtr hasGui = getKnobGuiPointer();

    if (hasGui) {
        return hasGui->getScreenPixelRatio();
    } else {
        return 1.;
    }
}
#endif

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
    int nDims = getDimension();
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
        QObject::connect( masterKnob->_signalSlotHandler.get(), SIGNAL(keyFrameSet(double,ViewSpec,int,int,bool)),
                          _signalSlotHandler.get(), SLOT(onMasterKeyFrameSet(double,ViewSpec,int,int,bool)), Qt::UniqueConnection );
        QObject::connect( masterKnob->_signalSlotHandler.get(), SIGNAL(keyFrameRemoved(double,ViewSpec,int,int)),
                          _signalSlotHandler.get(), SLOT(onMasterKeyFrameRemoved(double,ViewSpec,int,int)), Qt::UniqueConnection );
        QObject::connect( masterKnob->_signalSlotHandler.get(), SIGNAL(keyFrameMoved(ViewSpec,int,double,double)),
                          _signalSlotHandler.get(), SLOT(onMasterKeyFrameMoved(ViewSpec,int,double,double)), Qt::UniqueConnection );
        QObject::connect( masterKnob->_signalSlotHandler.get(), SIGNAL(animationRemoved(ViewSpec,int)),
                          _signalSlotHandler.get(), SLOT(onMasterAnimationRemoved(ViewSpec,int)), Qt::UniqueConnection );
    }

    bool hasChanged = cloneAndCheckIfChanged(other.get(), dimension, otherDimension);

    //Do not disable buttons when they are slaved
    KnobButton* isBtn = dynamic_cast<KnobButton*>(this);
    if (!isBtn) {
        setEnabled(dimension, false);
    }

    if (_signalSlotHandler) {
        ///Notify we want to refresh
        if (reason == eValueChangedReasonNatronInternalEdited && _signalSlotHandler) {
            _signalSlotHandler->s_knobSlaved(dimension, true);
        }
    }

    if (hasChanged) {
        evaluateValueChange(dimension, getCurrentTime(), ViewIdx(0), reason);
    } else if (isBtn) {
        //For buttons, don't evaluate or the instanceChanged action of the button will be called,
        //just refresh the hasModifications flag so it gets serialized
        isBtn->computeHasModifications();
        //force the aliased parameter to be persistent if it's not, otherwise it will not be saved
        isBtn->setIsPersistent(true);
    }

    ///Register this as a listener of the master
    if (masterKnob) {
        masterKnob->addListener( false, dimension, otherDimension, shared_from_this() );
    }

    return true;
} // KnobHelper::slaveTo

std::pair<int, KnobIPtr> KnobHelper::getMaster(int dimension) const
{
    assert( dimension >= 0 && dimension < (int)_imp->masters.size() );
    QReadLocker l(&_imp->mastersMutex);
    std::pair<int, KnobIPtr> ret = std::make_pair( _imp->masters[dimension].first, _imp->masters[dimension].second.lock() );

    return ret;
}

void
KnobHelper::resetMaster(int dimension)
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
KnobHelper::checkAnimationLevel(ViewSpec view,
                                int dimension)
{
    bool changed = false;

    for (int i = 0; i < _imp->dimension; ++i) {
        if ( (i == dimension) || (dimension == -1) ) {
            AnimationLevelEnum level = eAnimationLevelNone;


            if ( canAnimate() &&
                 isAnimated(i, view) &&
                 getExpression(i).empty() &&
                 getHolder() && getHolder()->getApp() ) {
                CurvePtr c = getCurve(view, i);
                double time = getHolder()->getApp()->getTimeLine()->currentFrame();
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
    std::pair<int, KnobIPtr> master = getMaster(dimension);

    if (master.second) {
        //Make sure it is refreshed
        master.second->checkAnimationLevel(ViewSpec(0), master.first);

        return master.second->getAnimationLevel(master.first);
    }

    QMutexLocker l(&_imp->animationLevelMutex);
    if ( dimension > (int)_imp->animationLevel.size() ) {
        throw std::invalid_argument("Knob::getAnimationLevel(): Dimension out of range");
    }

    return _imp->animationLevel[dimension];
}

void
KnobHelper::deleteAnimationConditional(double time,
                                       ViewSpec view,
                                       int dimension,
                                       ValueChangedReasonEnum reason,
                                       bool before)
{
    if (!_imp->curves[dimension]) {
        return;
    }
    assert( 0 <= dimension && dimension < getDimension() );

    CurvePtr curve;
    KnobGuiIPtr hasGui = getKnobGuiPointer();
    bool useGuiCurve = _imp->shouldUseGuiCurve();

    if (!useGuiCurve) {
        curve = _imp->curves[dimension];
    } else {
        assert(hasGui);
        curve = hasGui->getCurve(view, dimension);
        setGuiCurveHasChanged(view, dimension, true);
    }

    std::list<int> keysRemoved;
    if (before) {
        curve->removeKeyFramesBeforeTime(time, &keysRemoved);
    } else {
        curve->removeKeyFramesAfterTime(time, &keysRemoved);
    }

    if (!useGuiCurve) {
        checkAnimationLevel(view, dimension);
        guiCurveCloneInternalCurve(eCurveChangeReasonInternal, view, dimension, reason);
        evaluateValueChange(dimension, time, view, reason);
    }

    KnobHolder* holder = getHolder();
    if ( holder && holder->getApp() ) {
        holder->getApp()->removeMultipleKeyframeIndicator(keysRemoved, true);
    }
}

void
KnobHelper::deleteAnimationBeforeTime(double time,
                                      ViewSpec view,
                                      int dimension,
                                      ValueChangedReasonEnum reason)
{
    deleteAnimationConditional(time, view, dimension, reason, true);
}

void
KnobHelper::deleteAnimationAfterTime(double time,
                                     ViewSpec view,
                                     int dimension,
                                     ValueChangedReasonEnum reason)
{
    deleteAnimationConditional(time, view, dimension, reason, false);
}

bool
KnobHelper::getKeyFrameTime(ViewSpec view,
                            int index,
                            int dimension,
                            double* time) const
{
    assert( 0 <= dimension && dimension < getDimension() );
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
    assert( 0 <= dimension && dimension < getDimension() );
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
    assert( 0 <= dimension && dimension < getDimension() );
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
    assert( 0 <= dimension && dimension < getDimension() );
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
        for (std::size_t i = 0; i < it->second.size(); ++i) {
            if ( it->second[i].isListening && ( (it->second[i].targetDim == dimension) || (it->second[i].targetDim == -1) || (dimension == -1) ) ) {
                dimensionsToEvaluate.insert(i);
                if (!it->second[i].isExpr) {
                    ///We still want to clone the master's dimension because otherwise we couldn't edit the curve e.g in the curve editor
                    ///For example we use it for roto knobs where selected beziers have their knobs slaved to the gui knobs
                    slaveKnob->clone(this, i, it->second[i].targetDim);
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

        for (int i = 0; i < slaveKnob->getDimension(); ++i) {
            slaveKnob->clearExpressionsResults(i);
        }


        slaveKnob->evaluateValueChangeInternal(dimChanged, time, view, eValueChangedReasonSlaveRefresh, reason);

        //call recursively
        if ( !slaveKnob->isListenersNotificationBlocked() ) {
            slaveKnob->refreshListenersAfterValueChange(view, reason, dimChanged);
        }
    } // for all listeners
} // KnobHelper::refreshListenersAfterValueChange

void
KnobHelper::cloneExpressions(KnobI* other,
                             int dimension,
                             int otherDimension)
{
    assert( (int)_imp->expressions.size() == getDimension() );
    assert( (dimension == otherDimension) || (dimension != -1) );
    try {
        if (dimension == -1) {
            int dims = std::min( getDimension(), other->getDimension() );
            for (int i = 0; i < dims; ++i) {
                std::string expr = other->getExpression(i);
                bool hasRet = other->isExpressionUsingRetVariable(i);
                if ( !expr.empty() ) {
                    setExpression(i, expr, hasRet, false);
                    cloneExpressionsResults(other, i, i);
                }
            }
        } else {
            if (otherDimension == -1) {
                otherDimension = dimension;
            }
            assert( dimension >= 0 && dimension < getDimension() &&
                    otherDimension >= 0 && otherDimension < other->getDimension() );
            std::string expr = other->getExpression(otherDimension);
            bool hasRet = other->isExpressionUsingRetVariable(otherDimension);
            if ( !expr.empty() ) {
                setExpression(dimension, expr, hasRet, false);
                cloneExpressionsResults(other, dimension, otherDimension);
            }
        }
    } catch (...) {
        ///ignore errors
    }
}

bool
KnobHelper::cloneExpressionsAndCheckIfChanged(KnobI* other,
                                              int dimension,
                                              int otherDimension)
{
    assert( (int)_imp->expressions.size() == getDimension() );
    assert( (dimension == otherDimension) || (dimension != -1) );
    assert(other);
    bool ret = false;
    try {
        int dims = std::min( getDimension(), other->getDimension() );
        for (int i = 0; i < dims; ++i) {
            if ( (i == dimension) || (dimension == -1) ) {
                std::string expr = other->getExpression(i);
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

void
KnobHelper::cloneOneCurve(KnobI* other,
                          int offset,
                          const RangeD* range,
                          int dimension,
                          int otherDimension)
{
    assert( dimension >= 0 && dimension < getDimension() &&
            otherDimension >= 0 && otherDimension < other->getDimension() );
    CurvePtr thisCurve = getCurve(ViewIdx(0), dimension, true);
    CurvePtr otherCurve = other->getCurve(ViewIdx(0), otherDimension, true);
    if (thisCurve && otherCurve) {
        if (!range) {
            thisCurve->clone(*otherCurve);
        } else {
            thisCurve->clone(*otherCurve, offset, range);
        }
    }
    CurvePtr guiCurve = getGuiCurve(ViewIdx(0), dimension);
    CurvePtr otherGuiCurve = other->getGuiCurve(ViewIdx(0), otherDimension);
    if (guiCurve) {
        if (otherGuiCurve) {
            if (!range) {
                guiCurve->clone(*otherGuiCurve);
            } else {
                guiCurve->clone(*otherGuiCurve, offset, range);
            }
        } else {
            if (otherCurve) {
                if (!range) {
                    guiCurve->clone(*otherCurve);
                } else {
                    guiCurve->clone(*otherCurve, offset, range);
                }
            }
        }
    }
    checkAnimationLevel(ViewIdx(0), dimension);
}

void
KnobHelper::cloneCurves(KnobI* other,
                        int offset,
                        const RangeD* range,
                        int dimension,
                        int otherDimension)
{
    assert( (dimension == otherDimension) || (dimension != -1) );
    assert(other);
    if (dimension == -1) {
        int dimMin = std::min( getDimension(), other->getDimension() );
        for (int i = 0; i < dimMin; ++i) {
            cloneOneCurve(other, offset, range, i, i);
        }
    } else {
        if (otherDimension == -1) {
            otherDimension = dimension;
        }
        cloneOneCurve(other, offset, range, dimension, otherDimension);
    }
}

bool
KnobHelper::cloneOneCurveAndCheckIfChanged(KnobI* other,
                                           bool updateGui,
                                           int dimension,
                                           int otherDimension)
{
    assert( dimension >= 0 && dimension < getDimension() &&
            otherDimension >= 0 && otherDimension < other->getDimension() );
    bool cloningCurveChanged = false;
    CurvePtr thisCurve = getCurve(ViewIdx(0), dimension, true);
    CurvePtr otherCurve = other->getCurve(ViewIdx(0), otherDimension, true);
    KeyFrameSet oldKeys;
    if (thisCurve && otherCurve) {
        if (updateGui) {
            oldKeys = thisCurve->getKeyFrames_mt_safe();
        }
        cloningCurveChanged |= thisCurve->cloneAndCheckIfChanged(*otherCurve);
    }

    CurvePtr guiCurve = getGuiCurve(ViewIdx(0), dimension);
    CurvePtr otherGuiCurve = other->getGuiCurve(ViewIdx(0), otherDimension);
    if (guiCurve) {
        if (otherGuiCurve) {
            cloningCurveChanged |= guiCurve->cloneAndCheckIfChanged(*otherGuiCurve);
        } else {
            cloningCurveChanged |= guiCurve->cloneAndCheckIfChanged(*otherCurve);
        }
    }

    if (updateGui && cloningCurveChanged) {
        // Indicate that old keyframes are removed

        std::list<double> oldKeysList;
        for (KeyFrameSet::iterator it = oldKeys.begin(); it != oldKeys.end(); ++it) {
            oldKeysList.push_back( it->getTime() );
        }
        if ( !oldKeysList.empty() && _signalSlotHandler) {
            _signalSlotHandler->s_multipleKeyFramesRemoved(oldKeysList, ViewSpec::all(), dimension, (int)eValueChangedReasonNatronInternalEdited);
        }

        // Indicate new keyframes

        std::list<double> keysList;
        KeyFrameSet keys;
        if (thisCurve) {
            keys = thisCurve->getKeyFrames_mt_safe();
        }
        for (KeyFrameSet::iterator it = keys.begin(); it != keys.end(); ++it) {
            keysList.push_back( it->getTime() );
        }
        if ( !keysList.empty() && _signalSlotHandler) {
            _signalSlotHandler->s_multipleKeyFramesSet(keysList, ViewSpec::all(), dimension, (int)eValueChangedReasonNatronInternalEdited);
        }
        checkAnimationLevel(ViewIdx(0), dimension);
    } else if (!updateGui) {
        checkAnimationLevel(ViewIdx(0), dimension);
    }

    return cloningCurveChanged;
} // KnobHelper::cloneOneCurveAndCheckIfChanged

bool
KnobHelper::cloneCurvesAndCheckIfChanged(KnobI* other,
                                         bool updateGui,
                                         int dimension,
                                         int otherDimension)
{
    assert( (dimension == otherDimension) || (dimension != -1) );
    assert(other);
    assert( (dimension == otherDimension) || (dimension != -1) );
    assert(other);
    bool hasChanged = false;
    if (dimension == -1) {
        int dimMin = std::min( getDimension(), other->getDimension() );
        for (int i = 0; i < dimMin; ++i) {
            hasChanged |= cloneOneCurveAndCheckIfChanged(other, updateGui, i, i);
        }
    } else {
        if (otherDimension == -1) {
            otherDimension = dimension;
        }
        assert( dimension >= 0 && dimension < getDimension() &&
                otherDimension >= 0 && otherDimension < other->getDimension() );
        hasChanged |= cloneOneCurveAndCheckIfChanged(other, updateGui, dimension, otherDimension);
    }

    return hasChanged;
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
        if ( listenerIsHelper->getHolder() ) {
            listenerIsHelper->getHolder()->onKnobSlaved(listener, thisShared, listenerDimension, true );
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
            dims.resize( listener->getDimension() );
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
KnobHelper::removeListener(KnobI* listener,
                           int listenerDimension)
{
    KnobHelper* listenerHelper = dynamic_cast<KnobHelper*>(listener);

    assert(listenerHelper);
    Q_UNUSED(listenerHelper);

    QWriteLocker l(&_imp->mastersMutex);
    for (ListenerDimsMap::iterator it = _imp->listeners.begin(); it != _imp->listeners.end(); ++it) {
        if (it->first.lock().get() == listener) {
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
    KnobHolder* holder = getHolder();

    return holder && holder->getApp() ? holder->getCurrentTime() : 0;
}

ViewIdx
KnobHelper::getCurrentView() const
{
    KnobHolder* holder = getHolder();

    return ( holder && holder->getApp() ) ? holder->getCurrentView() : ViewIdx(0);
}

double
KnobHelper::random(double min,
                   double max,
                   double time,
                   unsigned int seed) const
{
    randomSeed(time, seed);

    return random(min, max);
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
KnobHelper::randomInt(int min,
                      int max,
                      double time,
                      unsigned int seed) const
{
    randomSeed(time, seed);

    return randomInt(min, max);
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
    U64 hash = 0;
    KnobHolder* holder = getHolder();

    if (holder) {
        EffectInstance* effect = dynamic_cast<EffectInstance*>(holder);
        if (effect) {
            hash = effect->getHash();
        }
    }
    U32 hash32 = (U32)hash;
    hash32 += seed;

    alias_cast_float ac;
    ac.data = (float)time;
    hash32 += ac.raw;

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
KnobHelper::hasModificationsForSerialization() const
{
    bool enabledChanged = false;
    bool defValueChanged = false;
    for (int i = 0; i < getDimension(); ++i) {
        if ( isEnabled(i) != isDefaultEnabled(i) ) {
            enabledChanged = true;
        }
        if (hasDefaultValueChanged(i)) {
            defValueChanged = true;
        }
    }

    return hasModifications() ||
           getIsSecret() != getDefaultIsSecret() || enabledChanged || defValueChanged;
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
KnobHolder::getUserPages(std::list<KnobPage*>& userPages) const {
    const KnobsVec& knobs = getKnobs();

    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ( (*it)->isUserKnob() ) {
            KnobPage* isPage = dynamic_cast<KnobPage*>( it->get() );
            if (isPage) {
                userPages.push_back(isPage);
            }
        }
    }
}

KnobIPtr
KnobHelper::createDuplicateOnHolder(KnobHolder* otherHolder,
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
    if ( !otherHolder || !getHolder()->getApp() ) {
        return KnobIPtr();
    }

    KnobHolder* holder = getHolder();
    EffectInstance* otherIsEffect = dynamic_cast<EffectInstance*>(otherHolder);
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
    KnobBool* isBool = dynamic_cast<KnobBool*>(this);
    KnobInt* isInt = dynamic_cast<KnobInt*>(this);
    KnobDouble* isDbl = dynamic_cast<KnobDouble*>(this);
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>(this);
    KnobColor* isColor = dynamic_cast<KnobColor*>(this);
    KnobString* isString = dynamic_cast<KnobString*>(this);
    KnobFile* isFile = dynamic_cast<KnobFile*>(this);
    KnobOutputFile* isOutputFile = dynamic_cast<KnobOutputFile*>(this);
    KnobPath* isPath = dynamic_cast<KnobPath*>(this);
    KnobGroup* isGrp = dynamic_cast<KnobGroup*>(this);
    KnobPage* isPage = dynamic_cast<KnobPage*>(this);
    KnobButton* isBtn = dynamic_cast<KnobButton*>(this);
    KnobParametric* isParametric = dynamic_cast<KnobParametric*>(this);


    //Ensure there is a user page
    KnobPagePtr destPage;
    if (page) {
        destPage = page;
    } else {
        if (otherIsEffect) {
            std::list<KnobPage*> userPages;
            otherIsEffect->getUserPages(userPages);
            if (userPages.empty()) {
                destPage = otherIsEffect->getOrCreateUserPageKnob();
            } else {
                destPage = boost::dynamic_pointer_cast<KnobPage>(userPages.front()->shared_from_this());
            }

        }
    }

    KnobIPtr output;
    if (isBool) {
        KnobBoolPtr newKnob = otherHolder->createBoolKnob(newScriptName, newLabel, isUserKnob);
        output = newKnob;
    } else if (isInt) {
        KnobIntPtr newKnob = otherHolder->createIntKnob(newScriptName, newLabel, getDimension(), isUserKnob);
        newKnob->setMinimumsAndMaximums( isInt->getMinimums(), isInt->getMaximums() );
        newKnob->setDisplayMinimumsAndMaximums( isInt->getDisplayMinimums(), isInt->getDisplayMaximums() );
        if ( isInt->isSliderDisabled() ) {
            newKnob->disableSlider();
        }
        output = newKnob;
    } else if (isDbl) {
        KnobDoublePtr newKnob = otherHolder->createDoubleKnob(newScriptName, newLabel, getDimension(), isUserKnob);
        newKnob->setSpatial( isDbl->getIsSpatial() );
        if ( isDbl->isRectangle() ) {
            newKnob->setAsRectangle();
        }
        for (int i = 0; i < getDimension(); ++i) {
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
            newKnob->populateChoices( isChoice->getEntries_mt_safe() );
        }
        output = newKnob;
    } else if (isColor) {
        KnobColorPtr newKnob = otherHolder->createColorKnob(newScriptName, newLabel, getDimension(), isUserKnob);
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
        if ( isFile->isInputImageFile() ) {
            newKnob->setAsInputImage();
        }
        output = newKnob;
    } else if (isOutputFile) {
        KnobOutputFilePtr newKnob = otherHolder->createOuptutFileKnob(newScriptName, newLabel, isUserKnob);
        if ( isOutputFile->isOutputImageFile() ) {
            newKnob->setAsOutputImageFile();
        }
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
        KnobParametricPtr newKnob = otherHolder->createParametricKnob(newScriptName, newLabel, isParametric->getDimension(), isUserKnob);
        output = newKnob;
        newKnob->setMinimumsAndMaximums( isParametric->getMinimums(), isParametric->getMaximums() );
        newKnob->setDisplayMinimumsAndMaximums( isParametric->getDisplayMinimums(), isParametric->getDisplayMaximums() );
    }
    if (!output) {
        return KnobIPtr();
    }
    for (int i = 0; i < getDimension(); ++i) {
        output->setDimensionName( i, getDimensionName(i) );
    }
    output->setName(newScriptName, true);
    output->cloneDefaultValues(this);
    output->clone(this);
    if ( canAnimate() ) {
        output->setAnimationEnabled( isAnimationEnabled() );
    }
    output->setEvaluateOnChange( getEvaluateOnChange() );
    output->setHintToolTip(newToolTip);
    output->setAddNewLine(true);
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
        otherIsEffect->getNode()->declarePythonFields();
    }
    if (!makeAlias && otherIsEffect && isEffect) {
        NodeCollectionPtr collec;
        collec = isEffect->getNode()->getGroup();

        NodeGroup* isCollecGroup = dynamic_cast<NodeGroup*>( collec.get() );
        std::stringstream ss;
        if (isCollecGroup) {
            ss << "thisGroup." << newScriptName;
        } else {
            ss << "app." << otherIsEffect->getNode()->getFullyQualifiedName() << "." << newScriptName;
        }
        if (output->getDimension() > 1) {
            ss << ".get()[dimension]";
        } else {
            ss << ".get()";
        }

        try {
            std::string script = ss.str();
            for (int i = 0; i < getDimension(); ++i) {
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
KnobI::areTypesCompatibleForSlave(KnobI* lhs,
                                  KnobI* rhs)
{
    if ( lhs->typeName() == rhs->typeName() ) {
        return true;
    }

    //These are compatible types
    KnobInt* lhsIsInt = dynamic_cast<KnobInt*>(lhs);
    KnobInt* rhsIsInt = dynamic_cast<KnobInt*>(rhs);
    KnobDouble* lhsIsDouble = dynamic_cast<KnobDouble*>(lhs);
    KnobColor* lhsIsColor = dynamic_cast<KnobColor*>(lhs);
    KnobDouble* rhsIsDouble = dynamic_cast<KnobDouble*>(rhs);
    KnobColor* rhsIsColor = dynamic_cast<KnobColor*>(rhs);
    if ( (lhsIsDouble || lhsIsColor || lhsIsInt) && (rhsIsColor || rhsIsDouble || rhsIsInt) ) {
        return true;
    }

    /*  KnobChoice* lhsIsChoice = dynamic_cast<KnobChoice*>(lhs);
       KnobChoice* rhsIsChoice = dynamic_cast<KnobChoice*>(rhs);
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
    if ( !master || ( master->getDimension() != getDimension() ) ||
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
    for (int i = 0; i < getDimension(); ++i) {
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
    for (int i = 0; i < getDimension(); ++i) {
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

    return true;
}

KnobIPtr
KnobHelper::getAliasMaster()  const
{
    QReadLocker k(&_imp->mastersMutex);

    return _imp->slaveForAlias;
}

void
KnobHelper::getAllExpressionDependenciesRecursive(std::set<NodePtr>& nodes) const
{
    std::set<KnobIPtr> deps;
    {
        QMutexLocker k(&_imp->expressionMutex);
        for (int i = 0; i < _imp->dimension; ++i) {
            for (std::list<std::pair<KnobIWPtr, int> >::const_iterator it = _imp->expressions[i].dependencies.begin();
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
        EffectInstance* effect  = dynamic_cast<EffectInstance*>( (*it)->getHolder() );
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

/***************************KNOB HOLDER******************************************/

struct KnobHolder::KnobHolderPrivate
{
    AppInstanceWPtr app;
    QMutex knobsMutex;
    std::vector<KnobIPtr> knobs;
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
    KnobHolder::MultipleParamsEditEnum paramsEditLevel;
    int paramsEditRecursionLevel;
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
    mutable QMutex hasAnimationMutex;
    bool hasAnimation;
    DockablePanelI* settingsPanel;

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
        , paramsEditLevel(eMultipleParamsEditOff)
        , paramsEditRecursionLevel(0)
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
    , paramsEditLevel(other.paramsEditLevel)
    , paramsEditRecursionLevel(other.paramsEditRecursionLevel)
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
    QObject::connect( this, SIGNAL(doValueChangeOnMainThread(KnobI*,int,double,ViewSpec,bool)), this,
                      SLOT(onDoValueChangeOnMainThread(KnobI*,int,double,ViewSpec,bool)) );
}

KnobHolder::KnobHolder(const KnobHolder& other)
: QObject()
, _imp (new KnobHolderPrivate(*other._imp))
{
    QObject::connect( this, SIGNAL( doEndChangesOnMainThread() ), this, SLOT( onDoEndChangesOnMainThreadTriggered() ) );
    QObject::connect( this, SIGNAL( doEvaluateOnMainThread(bool, bool) ), this,
                     SLOT( onDoEvaluateOnMainThread(bool, bool) ) );
    QObject::connect( this, SIGNAL( doValueChangeOnMainThread(KnobI*, int, double, ViewSpec, bool) ), this,
                     SLOT( onDoValueChangeOnMainThread(KnobI*, int, double, ViewSpec, bool) ) );
}

KnobHolder::~KnobHolder()
{
    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        KnobHelper* helper = dynamic_cast<KnobHelper*>( _imp->knobs[i].get() );
        assert(helper);
        if ( helper && (helper->_imp->holder == this) ) {
            helper->_imp->holder = 0;
        }
    }
}

void
KnobHolder::addKnobToViewerUI(const KnobIPtr& knob)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->knobsWithViewerUI.push_back(knob);
}

bool
KnobHolder::isInViewerUIKnob(const KnobIPtr& knob) const
{
    for (std::vector<KnobIWPtr>::const_iterator it = _imp->knobsWithViewerUI.begin(); it!=_imp->knobsWithViewerUI.end(); ++it) {
        KnobIPtr p = it->lock();
        if (p == knob) {
            return true;
        }
    }
    return false;
}

KnobsVec
KnobHolder::getViewerUIKnobs() const
{
    assert( QThread::currentThread() == qApp->thread() );
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
KnobHolder::removeKnobFromList(const KnobI* knob)
{
    QMutexLocker kk(&_imp->knobsMutex);

    for (KnobsVec::iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        if (it->get() == knob) {
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
            isEffect->getNode()->declarePythonFields();
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
            isEffect->getNode()->declarePythonFields();
        }
    }
}

void
KnobHolder::deleteKnob(KnobI* knob,
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
        if (it->get() == knob) {
            (*it)->deleteKnob();
            sharedKnob = *it;
            break;
        }
    }

    {
        QMutexLocker k(&_imp->knobsMutex);
        for (KnobsVec::iterator it2 = _imp->knobs.begin(); it2 != _imp->knobs.end(); ++it2) {
            if (it2->get() == knob) {
                _imp->knobs.erase(it2);
                break;
            }
        }
    }

    if (alsoDeleteGui && _imp->settingsPanel) {
        _imp->settingsPanel->deleteKnobGui(sharedKnob);
    }
}

bool
KnobHolder::moveKnobOneStepUp(KnobI* knob)
{
    if ( !knob->isUserKnob() && !dynamic_cast<KnobPage*>(knob) ) {
        return false;
    }
    KnobIPtr parent = knob->getParentKnob();
    KnobGroup* parentIsGrp = dynamic_cast<KnobGroup*>( parent.get() );
    KnobPage* parentIsPage = dynamic_cast<KnobPage*>( parent.get() );

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
                if (_imp->knobs[i].get() == knob) {
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
                if (_imp->knobs[i].get() == knob) {
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
KnobHolder::moveKnobOneStepDown(KnobI* knob)
{
    if ( !knob->isUserKnob() && !dynamic_cast<KnobPage*>(knob) ) {
        return false;
    }
    KnobIPtr parent = knob->getParentKnob();
    KnobGroup* parentIsGrp = dynamic_cast<KnobGroup*>( parent.get() );
    KnobPage* parentIsPage = dynamic_cast<KnobPage*>( parent.get() );

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
        if (_imp->knobs[i].get() == knob) {
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
    ret = AppManager::createKnob<KnobPage>(this, tr(NATRON_USER_MANAGED_KNOBS_PAGE_LABEL), 1, false);
    ret->setAsUserKnob(true);
    ret->setName(NATRON_USER_MANAGED_KNOBS_PAGE);


    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect) {
        isEffect->getNode()->declarePythonFields();
    }

    return ret;
}

KnobIntPtr
KnobHolder::createIntKnob(const std::string& name,
                          const std::string& label,
                          int dimension,
                          bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobInt>(existingKnob);
    }
    KnobIntPtr ret = AppManager::createKnob<KnobInt>(this, label, dimension, false);
    ret->setName(name);
    ret->setAsUserKnob(userKnob);
    /*KnobPagePtr pageknob = getOrCreateUserPageKnob();
       Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect && userKnob) {
        isEffect->getNode()->declarePythonFields();
    }

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
        return boost::dynamic_pointer_cast<KnobDouble>(existingKnob);
    }
    KnobDoublePtr ret = AppManager::createKnob<KnobDouble>(this, label, dimension, false);
    ret->setName(name);
    ret->setAsUserKnob(userKnob);
    /*KnobPagePtr pageknob = getOrCreateUserPageKnob();
       Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect && userKnob) {
        isEffect->getNode()->declarePythonFields();
    }

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
        return boost::dynamic_pointer_cast<KnobColor>(existingKnob);
    }
    KnobColorPtr ret = AppManager::createKnob<KnobColor>(this, label, dimension, false);
    ret->setName(name);
    ret->setAsUserKnob(userKnob);
    /*KnobPagePtr pageknob = getOrCreateUserPageKnob();
       Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect && userKnob) {
        isEffect->getNode()->declarePythonFields();
    }

    return ret;
}

KnobBoolPtr
KnobHolder::createBoolKnob(const std::string& name,
                           const std::string& label,
                           bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobBool>(existingKnob);
    }
    KnobBoolPtr ret = AppManager::createKnob<KnobBool>(this, label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob(userKnob);
    /*KnobPagePtr pageknob = getOrCreateUserPageKnob();
       Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect && userKnob) {
        isEffect->getNode()->declarePythonFields();
    }

    return ret;
}

KnobChoicePtr
KnobHolder::createChoiceKnob(const std::string& name,
                             const std::string& label,
                             bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobChoice>(existingKnob);
    }
    KnobChoicePtr ret = AppManager::createKnob<KnobChoice>(this, label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob(userKnob);
    /*KnobPagePtr pageknob = getOrCreateUserPageKnob();
       Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect && userKnob) {
        isEffect->getNode()->declarePythonFields();
    }

    return ret;
}

KnobButtonPtr
KnobHolder::createButtonKnob(const std::string& name,
                             const std::string& label,
                             bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobButton>(existingKnob);
    }
    KnobButtonPtr ret = AppManager::createKnob<KnobButton>(this, label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob(userKnob);
    /*KnobPagePtr pageknob = getOrCreateUserPageKnob();
       Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect && userKnob) {
        isEffect->getNode()->declarePythonFields();
    }

    return ret;
}

KnobSeparatorPtr
KnobHolder::createSeparatorKnob(const std::string& name,
                                const std::string& label,
                                bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobSeparator>(existingKnob);
    }
    KnobSeparatorPtr ret = AppManager::createKnob<KnobSeparator>(this, label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob(userKnob);
    /*KnobPagePtr pageknob = getOrCreateUserPageKnob();
       Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect && userKnob) {
        isEffect->getNode()->declarePythonFields();
    }

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
        return boost::dynamic_pointer_cast<KnobString>(existingKnob);
    }
    KnobStringPtr ret = AppManager::createKnob<KnobString>(this, label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob(userKnob);
    /*KnobPagePtr pageknob = getOrCreateUserPageKnob();
       Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect && userKnob) {
        isEffect->getNode()->declarePythonFields();
    }

    return ret;
}

KnobFilePtr
KnobHolder::createFileKnob(const std::string& name,
                           const std::string& label,
                           bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobFile>(existingKnob);
    }
    KnobFilePtr ret = AppManager::createKnob<KnobFile>(this, label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob(userKnob);
    /*KnobPagePtr pageknob = getOrCreateUserPageKnob();
       Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect && userKnob) {
        isEffect->getNode()->declarePythonFields();
    }

    return ret;
}

KnobOutputFilePtr
KnobHolder::createOuptutFileKnob(const std::string& name,
                                 const std::string& label,
                                 bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobOutputFile>(existingKnob);
    }
    KnobOutputFilePtr ret = AppManager::createKnob<KnobOutputFile>(this, label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob(userKnob);
    /*KnobPagePtr pageknob = getOrCreateUserPageKnob();
       Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect && userKnob) {
        isEffect->getNode()->declarePythonFields();
    }

    return ret;
}

KnobPathPtr
KnobHolder::createPathKnob(const std::string& name,
                           const std::string& label,
                           bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobPath>(existingKnob);
    }
    KnobPathPtr ret = AppManager::createKnob<KnobPath>(this, label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob(userKnob);
    /*KnobPagePtr pageknob = getOrCreateUserPageKnob();
       Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect && userKnob) {
        isEffect->getNode()->declarePythonFields();
    }

    return ret;
}

KnobGroupPtr
KnobHolder::createGroupKnob(const std::string& name,
                            const std::string& label,
                            bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobGroup>(existingKnob);
    }
    KnobGroupPtr ret = AppManager::createKnob<KnobGroup>(this, label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob(userKnob);
    /*KnobPagePtr pageknob = getOrCreateUserPageKnob();
       Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect && userKnob) {
        isEffect->getNode()->declarePythonFields();
    }

    return ret;
}

KnobPagePtr
KnobHolder::createPageKnob(const std::string& name,
                           const std::string& label,
                           bool userKnob)
{
    KnobIPtr existingKnob = getKnobByName(name);

    if (existingKnob) {
        return boost::dynamic_pointer_cast<KnobPage>(existingKnob);
    }
    KnobPagePtr ret = AppManager::createKnob<KnobPage>(this, label, 1, false);
    ret->setName(name);
    ret->setAsUserKnob(userKnob);
    /*KnobPagePtr pageknob = getOrCreateUserPageKnob();
       Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect && userKnob) {
        isEffect->getNode()->declarePythonFields();
    }

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
        return boost::dynamic_pointer_cast<KnobParametric>(existingKnob);
    }
    KnobParametricPtr ret = AppManager::createKnob<KnobParametric>(this, label, nbCurves, false);
    ret->setName(name);
    ret->setAsUserKnob(userKnob);
    /*KnobPagePtr pageknob = getOrCreateUserPageKnob();
       Q_UNUSED(pageknob);*/
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(this);
    if (isEffect && userKnob) {
        isEffect->getNode()->declarePythonFields();
    }

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
KnobHolder::incrHashAndEvaluate(bool isSignificant,
                                bool refreshMetadata)
{
    onSignificantEvaluateAboutToBeCalled(0);
    evaluate(isSignificant, refreshMetadata);
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
    if ( !knobChanged.empty() ) {
        firstKnobChanged = knobChanged.begin()->knob;
        firstKnobReason = knobChanged.begin()->reason;
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
        onSignificantEvaluateAboutToBeCalled( firstKnobChanged.get() );
    }

    bool guiFrozen = firstKnobChanged ? getApp() && firstKnobChanged->getKnobGuiPointer() && firstKnobChanged->getKnobGuiPointer()->isGuiFrozenForPlayback() : false;

    // Call instanceChanged on each knob
    bool ret = false;
    for (KnobChanges::iterator it = knobChanged.begin(); it != knobChanged.end(); ++it) {
        if (it->knob && !it->valueChangeBlocked && !isLoadingProject) {
            if ( !it->originatedFromMainThread && !canHandleEvaluateOnChangeInOtherThread() ) {
                Q_EMIT doValueChangeOnMainThread(it->knob.get(), it->originalReason, it->time, it->view, it->originatedFromMainThread);
            } else {
                ret |= onKnobValueChanged_public(it->knob.get(), it->originalReason, it->time, it->view, it->originatedFromMainThread);
            }
        }

        it->knob->computeHasModifications();

        int dimension = -1;
        if (it->dimensionChanged.size() == 1) {
            dimension = *it->dimensionChanged.begin();
        }
        if (!guiFrozen) {
            KnobSignalSlotHandlerPtr handler = it->knob->getSignalSlotHandler();
            if (handler) {
                handler->s_valueChanged(it->view, dimension, it->reason);
            }
            it->knob->checkAnimationLevel(it->view, dimension);
        }

        if ( !it->valueChangeBlocked && !it->knob->isListenersNotificationBlocked() && firstKnobReason != eValueChangedReasonTimeChanged) {
            it->knob->refreshListenersAfterValueChange(it->view, it->originalReason, dimension);
        }
    }

    int evaluationBlocked;
    bool hasHadSignificantChange = false;
    bool hasHadAnyChange = false;
    bool mustRefreshMetadata = false;

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
                mustRefreshMetadata = true;
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
        if (!isMT) {
            Q_EMIT doEvaluateOnMainThread(hasHadSignificantChange, mustRefreshMetadata);
        } else {
            evaluate(hasHadSignificantChange, mustRefreshMetadata);
        }
    }

    return ret;
} // KnobHolder::endChanges

void
KnobHolder::onDoValueChangeOnMainThread(KnobI* knob,
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
                              bool refreshGui,
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
        foundChange->refreshGui |= refreshGui;
        foundChange->time = time;
        foundChange->view = view;
        foundChange->knob = knob;
        foundChange->valueChangeBlocked = knob->isValueChangesBlocked();
        if (dimension == -1) {
            for (int i = 0; i < knob->getDimension(); ++i) {
                foundChange->dimensionChanged.insert(i);
            }
        } else {
            foundChange->dimensionChanged.insert(dimension);
        }

        if ( !foundChange->valueChangeBlocked && knob->getIsMetadataSlave() ) {
            ++_imp->nbChangesRequiringMetadataRefresh;
        }

        if ( knob->getEvaluateOnChange() ) {
            ++_imp->nbSignificantChangesDuringEvaluationBlock;
        }
        ++_imp->nbChangesDuringEvaluationBlock;

        //We do not call instanceChanged now since the hash did not change!
        //Make sure to call it after

        /*if (reason == eValueChangedReasonTimeChanged) {
            return;
           }*/
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
KnobHolder::getAllExpressionDependenciesRecursive(std::set<NodePtr>& nodes) const
{
    QMutexLocker k(&_imp->knobsMutex);

    for (KnobsVec::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
        (*it)->getAllExpressionDependenciesRecursive(nodes);
    }
}

KnobHolder::MultipleParamsEditEnum
KnobHolder::getMultipleParamsEditLevel() const
{
    QMutexLocker l(&_imp->paramsEditLevelMutex);

    return _imp->paramsEditLevel;
}

void
KnobHolder::setMultipleParamsEditLevel(KnobHolder::MultipleParamsEditEnum level)
{
    QMutexLocker l(&_imp->paramsEditLevelMutex);

    if ( appPTR->isBackground() ) {
        _imp->paramsEditLevel = KnobHolder::eMultipleParamsEditOff;
    } else {
        if (level == KnobHolder::eMultipleParamsEditOff) {
            if (_imp->paramsEditRecursionLevel > 0) {
                --_imp->paramsEditRecursionLevel;
            }
            if (_imp->paramsEditRecursionLevel == 0) {
                _imp->paramsEditLevel = KnobHolder::eMultipleParamsEditOff;
            }
            endChanges();
        } else if (level == KnobHolder::eMultipleParamsEditOn) {
            _imp->paramsEditLevel = level;
        } else {
            assert(level == KnobHolder::eMultipleParamsEditOnCreateNewCommand);
            beginChanges();
            if (_imp->paramsEditLevel == KnobHolder::eMultipleParamsEditOff) {
                _imp->paramsEditLevel = KnobHolder::eMultipleParamsEditOnCreateNewCommand;
            }
            ++_imp->paramsEditRecursionLevel;
        }
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
        InitializeKnobsFlag_RAII __isInitializingKnobsFlag__(this);
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

void
KnobHolder::refreshInstanceSpecificKnobsOnly(bool isPlayback,
                                             double time)
{
    assert( QThread::currentThread() == qApp->thread() );
    if ( !getApp() || getApp()->isGuiFrozen() ) {
        return;
    }
    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        if ( _imp->knobs[i]->isInstanceSpecific() ) {
            _imp->knobs[i]->onTimeChanged(isPlayback, time);
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
                               const KnobI* caller) const
{
    QMutexLocker k(&_imp->knobsMutex);

    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        if (_imp->knobs[i].get() == caller) {
            continue;
        }
        if (_imp->knobs[i]->getName() == name) {
            return _imp->knobs[i];
        }
    }

    return KnobIPtr();
}

const std::vector<KnobIPtr> &
KnobHolder::getKnobs() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->knobs;
}

std::vector<KnobIPtr>
KnobHolder::getKnobs_mt_safe() const
{
    QMutexLocker k(&_imp->knobsMutex);

    return _imp->knobs;
}

void
KnobHolder::slaveAllKnobs(KnobHolder* other,
                          bool restore)
{
    assert( QThread::currentThread() == qApp->thread() );
    if (_imp->isSlave) {
        return;
    }
    ///Call it prior to slaveTo: it will set the master pointer as pointing to other
    onAllKnobsSlaved(true, other);

    ///When loading a project, we don't need to slave all knobs here because the serialization of each knob separately
    ///will reslave it correctly if needed
    if (!restore) {
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
                int dims = foundKnob->getDimension();
                for (int j = 0; j < dims; ++j) {
                    foundKnob->slaveTo(j, otherKnobs[i], j);
                }
            }
        }
        endChanges();
    }
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
        int dims = thisKnobs[i]->getDimension();
        for (int j = 0; j < dims; ++j) {
            if ( thisKnobs[i]->isSlave(j) ) {
                thisKnobs[i]->unSlave(j, true);
            }
        }
    }
    endChanges();
    _imp->isSlave = false;
    onAllKnobsSlaved(false, (KnobHolder*)NULL);
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
KnobHolder::onKnobValueChanged_public(KnobI* k,
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
KnobHolder::restoreDefaultValues()
{
    assert( QThread::currentThread() == qApp->thread() );

    aboutToRestoreDefaultValues();

    beginChanges();

    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        KnobButton* isBtn = dynamic_cast<KnobButton*>( _imp->knobs[i].get() );
        KnobSeparator* isSeparator = dynamic_cast<KnobSeparator*>( _imp->knobs[i].get() );

        ///Don't restore buttons and the node label
        if ( ( !isBtn || isBtn->getIsCheckable() ) && !isSeparator && (_imp->knobs[i]->getName() != kUserLabelKnobName) ) {
            for (int d = 0; d < _imp->knobs[i]->getDimension(); ++d) {
                _imp->knobs[i]->resetToDefaultValue(d);
            }
        }
    }
    endChanges();
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
KnobHolder::getPageIndex(const KnobPage* page) const
{
    QMutexLocker k(&_imp->knobsMutex);
    int pageIndex = 0;

    for (std::size_t i = 0; i < _imp->knobs.size(); ++i) {
        KnobPage* ispage = dynamic_cast<KnobPage*>( _imp->knobs[i].get() );
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

/***************************STRING ANIMATION******************************************/
void
AnimatingKnobStringHelper::cloneExtraData(KnobI* other,
                                          int /*dimension*/,
                                          int /*otherDimension*/)
{
    AnimatingKnobStringHelper* isAnimatedString = dynamic_cast<AnimatingKnobStringHelper*>(other);

    if (isAnimatedString) {
        _animation->clone( isAnimatedString->getAnimation() );
    }
}

bool
AnimatingKnobStringHelper::cloneExtraDataAndCheckIfChanged(KnobI* other,
                                                           int /*dimension*/,
                                                           int /*otherDimension*/)
{
    AnimatingKnobStringHelper* isAnimatedString = dynamic_cast<AnimatingKnobStringHelper*>(other);

    if (isAnimatedString) {
        return _animation->cloneAndCheckIfChanged( isAnimatedString->getAnimation() );
    }

    return false;
}

void
AnimatingKnobStringHelper::cloneExtraData(KnobI* other,
                                          double offset,
                                          const RangeD* range,
                                          int /*dimension*/,
                                          int /*otherDimension*/)
{
    AnimatingKnobStringHelper* isAnimatedString = dynamic_cast<AnimatingKnobStringHelper*>(other);

    if (isAnimatedString) {
        _animation->clone(isAnimatedString->getAnimation(), offset, range);
    }
}

AnimatingKnobStringHelper::AnimatingKnobStringHelper(KnobHolder* holder,
                                                     const std::string &description,
                                                     int dimension,
                                                     bool declaredByPlugin)
    : KnobStringBase(holder, description, dimension, declaredByPlugin)
    , _animation( new StringAnimationManager(this) ) // scoped_ptr
{
}

AnimatingKnobStringHelper::~AnimatingKnobStringHelper()
{
}

void
AnimatingKnobStringHelper::stringToKeyFrameValue(double time,
                                                 ViewSpec /*view*/,
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
AnimatingKnobStringHelper::animationRemoved_virtual(int /*dimension*/)
{
    _animation->clearKeyFrames();
}

void
AnimatingKnobStringHelper::keyframeRemoved_virtual(int /*dimension*/,
                                                   double time)
{
    _animation->removeKeyFrame(time);
}

std::string
AnimatingKnobStringHelper::getStringAtTime(double time,
                                           ViewSpec view,
                                           int dimension)
{
    std::string ret;

    // assert(!view.isAll());
    // assert(!view.isCurrent()); // not yet implemented
    if ( _animation->hasCustomInterp() ) {
        bool succeeded = false;
        try {
            succeeded = _animation->customInterpolation(time, &ret);
        } catch (...) {
        }

        if (!succeeded) {
            return getValue(dimension, view);
        } else {
            return ret;
        }
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

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_Knob.cpp"
