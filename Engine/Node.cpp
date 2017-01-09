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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Node.h"

#include <limits>
#include <locale>
#include <algorithm> // min, max
#include <bitset>
#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream

#include "Global/Macros.h"

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QReadWriteLock>
#include <QtCore/QCoreApplication>
#include <QtCore/QWaitCondition>
#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QtCore/QRegExp>

#include <ofxNatron.h>

#include <SequenceParsing.h>

#include "Global/MemoryInfo.h"

#include "Engine/NodePrivate.h"
#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Backdrop.h"
#include "Engine/Cache.h"
#include "Engine/Curve.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/DiskCacheNode.h"
#include "Engine/Dot.h"
#include "Engine/EffectInstance.h"
#include "Engine/EffectInstanceTLSData.h"
#include "Engine/Format.h"
#include "Engine/FileSystemModel.h"
#include "Engine/FStreamsSupport.h"
#include "Engine/GroupInput.h"
#include "Engine/GroupOutput.h"
#include "Engine/GenericSchedulerThreadWatcher.h"
#include "Engine/Hash64.h"
#include "Engine/Image.h"
#include "Engine/Knob.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/OneViewNode.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Log.h"
#include "Engine/Lut.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeGraphI.h"
#include "Engine/NodeGuiI.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxHost.h"
#include "Engine/OpenGLViewerI.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Plugin.h"
#include "Engine/PrecompNode.h"
#include "Engine/Project.h"
#include "Engine/ReadNode.h"
#include "Engine/RenderValuesCache.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/StubNode.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Timer.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerNode.h"
#include "Engine/TrackerHelper.h"
#include "Engine/TreeRenderNodeArgs.h"
#include "Engine/TLSHolder.h"
#include "Engine/UndoCommand.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerNode.h"
#include "Engine/WriteNode.h"



///The flickering of edges/nodes in the nodegraph will be refreshed
///at most every...
#define NATRON_RENDER_GRAPHS_HINTS_REFRESH_RATE_SECONDS 1


NATRON_NAMESPACE_ENTER;

Node::Node(const AppInstancePtr& app,
           const NodeCollectionPtr& group,
           const PluginPtr& plugin)
    : QObject()
    , _imp( new NodePrivate(this, app, group, plugin) )
{
    QObject::connect(this, SIGNAL(refreshIdentityStateRequested()), this, SLOT(onRefreshIdentityStateRequestReceived()), Qt::QueuedConnection);

    if (plugin && QString::fromUtf8(plugin->getPluginID().c_str()).startsWith(QLatin1String("com.FXHOME.HitFilm"))) {
        _imp->requiresGLFinishBeforeRender = true;
    }
}


Node::~Node()
{
    destroyNode(true, false);
}

bool
Node::isGLFinishRequiredBeforeRender() const
{
    return _imp->requiresGLFinishBeforeRender;
}

bool
Node::isPersistent() const
{
    return _imp->isPersistent;
}


PluginPtr
Node::getPlugin() const
{
    if (isPyPlug()) {
        return _imp->pyPlugHandle.lock();
    } else {
        return _imp->plugin.lock();
    }
}

PluginPtr
Node::getPyPlugPlugin() const
{
    return _imp->pyPlugHandle.lock();
}

PluginPtr
Node::getOriginalPlugin() const
{
    return _imp->plugin.lock();
}

void
Node::setPrecompNode(const PrecompNodePtr& precomp)
{
    //QMutexLocker k(&_imp->pluginsPropMutex);
    _imp->precomp = precomp;
}

PrecompNodePtr
Node::isPartOfPrecomp() const
{
    //QMutexLocker k(&_imp->pluginsPropMutex);
    return _imp->precomp.lock();
}

std::string
Node::getCurrentNodePresets() const
{
    QMutexLocker k(&_imp->nodePresetMutex);
    return _imp->initialNodePreset;
}


void
Node::setRenderThreadSafety(RenderSafetyEnum safety)
{
    QMutexLocker k(&_imp->pluginsPropMutex);

    _imp->currentThreadSafety = safety;
}

RenderSafetyEnum
Node::getCurrentRenderThreadSafety() const
{
    if ( !isMultiThreadingSupportEnabledForPlugin() ) {
        return eRenderSafetyInstanceSafe;
    }
    QMutexLocker k(&_imp->pluginsPropMutex);

    return _imp->currentThreadSafety;
}

void
Node::revertToPluginThreadSafety()
{
    QMutexLocker k(&_imp->pluginsPropMutex);

    _imp->currentThreadSafety = _imp->pluginSafety;
}


RenderSafetyEnum
Node::getPluginRenderThreadSafety() const
{
    QMutexLocker k(&_imp->pluginsPropMutex);
    return _imp->pluginSafety;
}

void
Node::setCurrentOpenGLRenderSupport(PluginOpenGLRenderSupport support)
{
    QMutexLocker k(&_imp->pluginsPropMutex);

    _imp->currentSupportOpenGLRender = support;
}

PluginOpenGLRenderSupport
Node::getCurrentOpenGLRenderSupport() const
{
    PluginPtr plugin = getPlugin();
    if (plugin) {
        PluginOpenGLRenderSupport pluginProp = (PluginOpenGLRenderSupport)plugin->getProperty<int>(kNatronPluginPropOpenGLSupport);
        if (pluginProp != ePluginOpenGLRenderSupportYes) {
            return pluginProp;
        }
    }

    if (!getApp()->getProject()->isGPURenderingEnabledInProject()) {
        return ePluginOpenGLRenderSupportNone;
    }

    // Ok still turned on, check the value of the opengl support knob in the Node page
    KnobChoicePtr openglSupportKnob = _imp->openglRenderingEnabledKnob.lock();
    if (openglSupportKnob) {
        int index = openglSupportKnob->getValue();
        if (index == 1) {
            return ePluginOpenGLRenderSupportNone;
        } else if (index == 2 && getApp()->isBackground()) {
            return ePluginOpenGLRenderSupportNone;
        }
    }

    // Descriptor returned that it supported OpenGL, let's see if it turned off/on in the instance the OpenGL rendering
    QMutexLocker k(&_imp->pluginsPropMutex);

    return _imp->currentSupportOpenGLRender;
}

void
Node::setCurrentSequentialRenderSupport(SequentialPreferenceEnum support)
{
    QMutexLocker k(&_imp->pluginsPropMutex);

    _imp->currentSupportSequentialRender = support;
}

SequentialPreferenceEnum
Node::getCurrentSequentialRenderSupport() const
{
    QMutexLocker k(&_imp->pluginsPropMutex);

    return _imp->currentSupportSequentialRender;
}

void
Node::setCurrentSupportTiles(bool support)
{
    QMutexLocker k(&_imp->pluginsPropMutex);

    _imp->currentSupportTiles = support;
}

bool
Node::getCurrentSupportTiles() const
{
    QMutexLocker k(&_imp->pluginsPropMutex);

    return _imp->currentSupportTiles;
}

void
Node::setCurrentSupportRenderScale(bool support)
{
    QMutexLocker k(&_imp->pluginsPropMutex);

    _imp->currentSupportsRenderScale = support;
}

bool
Node::getCurrentSupportRenderScale() const
{
    QMutexLocker k(&_imp->pluginsPropMutex);

    return _imp->currentSupportsRenderScale;
}

void
Node::setCurrentCanDistort(bool support)
{
    QMutexLocker k(&_imp->pluginsPropMutex);

    _imp->currentCanDistort = support;
}

bool
Node::getCurrentCanDistort() const
{
    QMutexLocker k(&_imp->pluginsPropMutex);

    return _imp->currentCanDistort;
}

void
Node::refreshDynamicProperties()
{
    PluginOpenGLRenderSupport pluginGLSupport = ePluginOpenGLRenderSupportNone;
    PluginPtr plugin = getPlugin();
    if (plugin) {
        pluginGLSupport = (PluginOpenGLRenderSupport)plugin->getProperty<int>(kNatronPluginPropOpenGLSupport);
        if (plugin->isOpenGLEnabled() && pluginGLSupport == ePluginOpenGLRenderSupportYes) {
            // Ok the plug-in supports OpenGL, figure out now if can be turned on/off by the instance
            pluginGLSupport = _imp->effect->getCurrentOpenGLSupport();
        }
    }


    setCurrentOpenGLRenderSupport(pluginGLSupport);
    bool tilesSupported = _imp->effect->supportsTiles();
    bool supportsRenderScale = _imp->effect->supportsRenderScale();
    bool multiResSupported = _imp->effect->supportsMultiResolution();
    bool canDistort = _imp->effect->getCanDistort();
    _imp->pluginSafety = _imp->effect->getCurrentRenderThreadSafety();
    
    if (!tilesSupported && _imp->pluginSafety == eRenderSafetyFullySafeFrame) {
        // an effect which does not support tiles cannot support host frame threading
        setRenderThreadSafety(eRenderSafetyFullySafe);
    } else {
        setRenderThreadSafety(_imp->pluginSafety);
    }

    setCurrentSupportTiles(multiResSupported && tilesSupported);
    setCurrentSupportRenderScale(supportsRenderScale);
    setCurrentSequentialRenderSupport( _imp->effect->getSequentialPreference() );
    setCurrentCanDistort(canDistort);
}

bool
Node::isRenderScaleSupportEnabledForPlugin() const
{
    PluginPtr plugin = getPlugin();
    return plugin ? plugin->isRenderScaleEnabled() : true;
}

bool
Node::isMultiThreadingSupportEnabledForPlugin() const
{
    PluginPtr plugin = getPlugin();
    return plugin ? plugin->isMultiThreadingEnabled() : true;
}


bool
Node::isDuringPaintStrokeCreation() const
{
    // We should render only
    RotoStrokeItemPtr attachedStroke = toRotoStrokeItem(getAttachedRotoItem());
    if (!attachedStroke) {
        return false;
    }
    return attachedStroke->isCurrentlyDrawing();
}


void
Node::refreshAcceptedBitDepths()
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->supportedDepths.clear();
    _imp->effect->addSupportedBitDepth(&_imp->supportedDepths);
    if ( _imp->supportedDepths.empty() ) {
        //From the spec:
        //The default for a plugin is to have none set, the plugin must define at least one in its describe action.
        throw std::runtime_error(tr("This plug-in does not support any of 8-bit, 16-bit or 32-bit floating point image processing").toStdString());
    }

}

bool
Node::isNodeCreated() const
{
    return _imp->nodeCreated;
}

void
Node::setProcessChannelsValues(bool doR,
                               bool doG,
                               bool doB,
                               bool doA)
{
    KnobBoolPtr eR = _imp->enabledChan[0].lock();

    if (eR) {
        eR->setValue(doR);
    }
    KnobBoolPtr eG = _imp->enabledChan[1].lock();
    if (eG) {
        eG->setValue(doG);
    }
    KnobBoolPtr eB = _imp->enabledChan[2].lock();
    if (eB) {
        eB->setValue(doB);
    }
    KnobBoolPtr eA = _imp->enabledChan[3].lock();
    if (eA) {
        eA->setValue(doA);
    }
}

bool
Node::setStreamWarningInternal(StreamWarningEnum warning,
                               const QString& message)
{
    assert( QThread::currentThread() == qApp->thread() );
    std::map<Node::StreamWarningEnum, QString>::iterator found = _imp->streamWarnings.find(warning);
    if ( found == _imp->streamWarnings.end() ) {
        _imp->streamWarnings.insert( std::make_pair(warning, message) );

        return true;
    } else {
        if (found->second != message) {
            found->second = message;

            return true;
        }
    }

    return false;
}

void
Node::setStreamWarning(StreamWarningEnum warning,
                       const QString& message)
{
    if ( setStreamWarningInternal(warning, message) ) {
        Q_EMIT streamWarningsChanged();
    }
}

void
Node::setStreamWarnings(const std::map<StreamWarningEnum, QString>& warnings)
{
    bool changed = false;

    for (std::map<StreamWarningEnum, QString>::const_iterator it = warnings.begin(); it != warnings.end(); ++it) {
        changed |= setStreamWarningInternal(it->first, it->second);
    }
    if (changed) {
        Q_EMIT streamWarningsChanged();
    }
}



void
Node::getStreamWarnings(std::map<StreamWarningEnum, QString>* warnings) const
{
    assert( QThread::currentThread() == qApp->thread() );
    *warnings = _imp->streamWarnings;
}




NodeCollectionPtr
Node::getGroup() const
{
    QMutexLocker k(&_imp->groupMutex);
    return _imp->group.lock();
}


void
Node::removeAllImagesFromCache()
{
    AppInstancePtr app = getApp();

    if (!app) {
        return;
    }
    ProjectPtr proj = app->getProject();

    if ( proj->isProjectClosing() || proj->isLoadingProject() ) {
        return;
    }
    appPTR->getCache()->removeAllEntriesForPlugin(getPluginID(), false);
}




void
Node::loadKnob(const KnobIPtr & knob,
               const SERIALIZATION_NAMESPACE::KnobSerializationList & knobsValues)
{
    // Try to find a serialized value for this knob

    for (SERIALIZATION_NAMESPACE::KnobSerializationList::const_iterator it = knobsValues.begin(); it != knobsValues.end(); ++it) {
        if ( (*it)->getName() == knob->getName() ) {
            knob->fromSerialization(**it);
            break;
        }
    }

} // Node::loadKnob

bool
Node::linkToNode(const NodePtr& other)
{
    // Only link same plug-in
    if (!other || other->getPluginID() != getPluginID()) {
        return false;
    }
    const KnobsVec& knobs = getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
        {
            KnobButtonPtr isButton = toKnobButton(*it);
            if (isButton && !isButton->getIsCheckable()) {
                // Don't link trigger buttons
                continue;
            }
        }
       
        KnobIPtr masterKnob = other->getKnobByName((*it)->getName());
        if (!masterKnob) {
            continue;
        }
        (*it)->linkTo(masterKnob);
    }
    return true;
}

void
Node::unlinkAllKnobs()
{
    const KnobsVec& knobs = getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        (*it)->unlink(DimSpec::all(), ViewSetSpec::all(), true);
    }
}

void
Node::getLinkedNodes(std::list<std::pair<NodePtr, bool> >* nodes) const
{
    // For each knob we keep track of the master nodes we find.
    // If the number of knobs refering the node is equal to the number of knobs dimension/view pairs
    // then the node is considered a clone.
    std::map<NodePtr, int> masterNodes;
    int nVisitedKnobDimensionView = 0;
    const KnobsVec& knobs = getKnobs();

    // For Groups, since a PyPlug may have by default some parameters linked,
    // we do not count the links to internal nodes to figure out if we need to appear linked or not.
    NodeGroupPtr isNodeGroup = isEffectNodeGroup();
    NodesList groupNodes;
    if (isNodeGroup) {
        groupNodes = isNodeGroup->getNodes();
    }
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {

        // Should we consider this knob to determine if the node is a clone or not ?
        bool countKnobAmongstVisited = true;
        {
            KnobButtonPtr isButton = toKnobButton(*it);
            if (isButton && !isButton->getIsCheckable()) {
                // Ignore trigger buttons
                countKnobAmongstVisited = false;
            }
        }

        if (!(*it)->getEvaluateOnChange()) {
            countKnobAmongstVisited = false;
        }

        int nDims = (*it)->getNDimensions();
        std::list<ViewIdx> views = (*it)->getViewsList();
        for (std::list<ViewIdx>::const_iterator it2 = views.begin(); it2 != views.end(); ++it2) {
            for (int i = 0; i < nDims; ++i) {

                if (countKnobAmongstVisited) {
                    ++nVisitedKnobDimensionView;
                }


                KnobDimViewKeySet dimViewSharedKnobs;
                (*it)->getSharedValues(DimIdx(i), *it2, &dimViewSharedKnobs);

                {
                    KnobDimViewKeySet expressionDependencies;
                    if ((*it)->getExpressionDependencies(DimIdx(i), *it2, expressionDependencies)) {

                    }
                    dimViewSharedKnobs.insert(expressionDependencies.begin(), expressionDependencies.end());
                }

                // insert all shared knobs for this dimension/view to the global sharedKnobs set
                for (KnobDimViewKeySet::const_iterator it3 = dimViewSharedKnobs.begin(); it3 != dimViewSharedKnobs.end() ;++it3) {
                    KnobIPtr sharedKnob = it3->knob.lock();
                    if (!sharedKnob) {
                        continue;
                    }

                    // Appear linked if there's at least one shared knob.
                    EffectInstancePtr sharedHolderIsEffect = toEffectInstance(sharedKnob->getHolder());
                    KnobTableItemPtr sharedHolderIsItem = toKnobTableItem(sharedKnob->getHolder());
                    if (sharedHolderIsItem) {
                        sharedHolderIsEffect = sharedHolderIsItem->getModel()->getNode()->getEffectInstance();
                    }
                    if (!sharedHolderIsEffect) {
                        continue;
                    }

                    // Increment the number of references to that node
                    NodePtr sharedNode = sharedHolderIsEffect->getNode();

                    // If the shared node is a child of this group, don't count as link
                    if (std::find(groupNodes.begin(), groupNodes.end(), sharedNode) != groupNodes.end()) {
                        continue;
                    }
                    if (sharedNode.get() != this) {
                        std::map<NodePtr, int>::iterator found = masterNodes.find(sharedNode);
                        if (found == masterNodes.end()) {
                            if (countKnobAmongstVisited) {
                                masterNodes[sharedNode] = 1;
                            } else {
                                masterNodes[sharedNode] = 0;
                            }
                        } else {
                            if (countKnobAmongstVisited) {
                                ++found->second;
                            }
                        }
                    }
                } // for all shared knobs
            } // for all dimensions
        } // for all views

    } // for all knobs

    for (std::map<NodePtr, int>::const_iterator it = masterNodes.begin(); it!=masterNodes.end(); ++it) {
        bool isCloneLink = it->second >= nVisitedKnobDimensionView;
        nodes->push_back(std::make_pair(it->first, isCloneLink));

    }

} // getLinkedNodes

void
Node::getCloneLinkedNodes(std::list<NodePtr>* clones) const
{
    std::list<std::pair<NodePtr, bool> > links;
    getLinkedNodes(&links);
    for (std::list<std::pair<NodePtr, bool> >::const_iterator it = links.begin(); it!=links.end(); ++it) {
        if (it->second) {
            clones->push_back(it->first);
        }
    }
}

bool
Node::areAllProcessingThreadsQuit() const
{
    {
        QMutexLocker locker(&_imp->mustQuitPreviewMutex);
        if (_imp->computingPreview) {
            return false;
        }
    }

    //If this effect has a RenderEngine, make sure it is finished
    if (_imp->renderEngine) {
        if ( _imp->renderEngine->hasThreadsAlive() ) {
            return false;
        }
    }

    TrackerNodePtr isTracker = toTrackerNode(_imp->effect);
    if (isTracker) {
        TrackerHelperPtr tracker = isTracker->getTracker();
        if ( tracker && !tracker->hasTrackerThreadQuit() ) {
            return false;
        }
    }

    return true;
}

void
Node::quitAnyProcessing_non_blocking()
{
    //If this effect has a RenderEngine, make sure it is finished

    if (_imp->renderEngine) {
        _imp->renderEngine->quitEngine(true);
    }

    //Returns when the preview is done computign
    _imp->abortPreview_non_blocking();


    TrackerNodePtr isTracker = toTrackerNode(_imp->effect);
    if (isTracker) {
        TrackerHelperPtr tracker = isTracker->getTracker();
        if (tracker) {
            tracker->quitTrackerThread_non_blocking();

        }
    }
}

void
Node::quitAnyProcessing_blocking(bool allowThreadsToRestart)
{


    //If this effect has a RenderEngine, make sure it is finished

    if (_imp->renderEngine) {
        _imp->renderEngine->quitEngine(allowThreadsToRestart);
        _imp->renderEngine->waitForEngineToQuit_enforce_blocking();
    }

    //Returns when the preview is done computign
    _imp->abortPreview_blocking(allowThreadsToRestart);

    TrackerNodePtr isTracker = toTrackerNode(_imp->effect);
    if (isTracker) {
        TrackerHelperPtr tracker = isTracker->getTracker();
        if (tracker) {
            tracker->quitTrackerThread_blocking(allowThreadsToRestart);
        }
    }
}

void
Node::abortAnyProcessing_non_blocking()
{

    if (_imp->renderEngine) {
        getApp()->abortAllViewers(false);

    }

    TrackerNodePtr isTracker = toTrackerNode(_imp->effect);
    if (isTracker) {
        TrackerHelperPtr tracker = isTracker->getTracker();
        if (tracker) {
            tracker->abortTracking();
        }
    }

    _imp->abortPreview_non_blocking();
}

void
Node::abortAnyProcessing_blocking()
{
    if (_imp->renderEngine) {
        _imp->renderEngine->abortRenderingNoRestart();
        _imp->renderEngine->waitForAbortToComplete_enforce_blocking();
    }

    TrackerNodePtr isTracker = toTrackerNode(_imp->effect);
    if (isTracker) {
        TrackerHelperPtr tracker = isTracker->getTracker();
        if (tracker) {
            tracker->abortTracking_blocking();
        }
    }
    _imp->abortPreview_blocking(false);
}



AppInstancePtr
Node::getApp() const
{
    return _imp->app.lock();
}

bool
Node::isActivated() const
{
    QMutexLocker l(&_imp->activatedMutex);

    return _imp->activated;
}


bool
Node::isKeepInAnimationModuleButtonDown() const
{
    KnobButtonPtr b = _imp->keepInAnimationModuleKnob.lock();
    if (!b) {
        return false;
    }
    return b->getValue();
}

bool
Node::isForceCachingEnabled() const
{
    KnobBoolPtr b = _imp->forceCaching.lock();

    return b ? b->getValue() : false;
}

void
Node::setForceCachingEnabled(bool value)
{
    KnobBoolPtr b = _imp->forceCaching.lock();
    if (!b) {
        return;
    }
    b->setValue(value);
}


void
Node::beginEditKnobs()
{
    _imp->effect->beginEditKnobs_public();
}


EffectInstancePtr
Node::getEffectInstance() const
{
    ///Thread safe as it never changes
    return _imp->effect;
}


int
Node::getMajorVersion() const
{

    PluginPtr plugin = getPlugin();
    if (!plugin) {
        return 0;
    }

    return (int)plugin->getProperty<unsigned int>(kNatronPluginPropVersion, 0);
}

int
Node::getMinorVersion() const
{
    ///Thread safe as it never changes
    PluginPtr plugin = getPlugin();
    if (!plugin) {
        return 0;
    }

    return (int)plugin->getProperty<unsigned int>(kNatronPluginPropVersion, 1);
}

void
Node::clearLastRenderedImage()
{
    {
        QMutexLocker k(&_imp->lastRenderedImageMutex);
        _imp->lastRenderedImage.reset();
    }
    _imp->effect->clearLastRenderedImage();
}

void
Node::setLastRenderedImage(const ImagePtr& lastRenderedImage)
{
#pragma message WARN("Check if we still neeed this ?")

    ImagePtr curLastRenderedImage;
    {
        QMutexLocker k(&_imp->lastRenderedImageMutex);
        curLastRenderedImage = _imp->lastRenderedImage;
        _imp->lastRenderedImage = lastRenderedImage;
    }
    // Ensure it is not destroyed while under the mutex, this could lead to a deadlock if the OpenGL context
    // switches during the texture destruction.
    curLastRenderedImage.reset();
}

ImagePtr
Node::getLastRenderedImage() const
{
    {
        QMutexLocker k(&_imp->lastRenderedImageMutex);
        return _imp->lastRenderedImage;
    }
}

KnobIPtr
Node::getKnobByName(const std::string & name) const
{
    ///MT-safe, never changes
    if (!_imp->effect) {
        return KnobIPtr();
    }

    return _imp->effect->getKnobByName(name);
}

bool
Node::isInputNode() const
{
    ///MT-safe, never changes
    return _imp->effect->isGenerator();
}

bool
Node::isOutputNode() const
{   ///MT-safe, never changes
    return _imp->effect->isOutput();
}

RenderEnginePtr
Node::getRenderEngine() const
{
    return _imp->renderEngine;
}


bool
Node::isDoingSequentialRender() const
{
    return _imp->renderEngine ? _imp->renderEngine->isDoingSequentialRender() : false;
}


ViewerNodePtr
Node::isEffectViewerNode() const
{
    return toViewerNode(_imp->effect);
}

ViewerInstancePtr
Node::isEffectViewerInstance() const
{
    return toViewerInstance(_imp->effect);
}

NodeGroupPtr
Node::isEffectNodeGroup() const
{
    return toNodeGroup(_imp->effect);
}

StubNodePtr
Node::isEffectStubNode() const
{
    return toStubNode(_imp->effect);
}


PrecompNodePtr
Node::isEffectPrecompNode() const
{
    return toPrecompNode(_imp->effect);
}

GroupInputPtr
Node::isEffectGroupInput() const
{
    return toGroupInput(_imp->effect);
}

GroupOutputPtr
Node::isEffectGroupOutput() const
{
    return toGroupOutput(_imp->effect);
}

ReadNodePtr
Node::isEffectReadNode() const
{
    return toReadNode(_imp->effect);
}

WriteNodePtr
Node::isEffectWriteNode() const
{
    return toWriteNode(_imp->effect);
}

BackdropPtr
Node::isEffectBackdrop() const
{
    return toBackdrop(_imp->effect);
}

OneViewNodePtr
Node::isEffectOneViewNode() const
{
    return toOneViewNode(_imp->effect);
}


const KnobsVec &
Node::getKnobs() const
{
    ///MT-safe from EffectInstance::getKnobs()
    return _imp->effect->getKnobs();
}


std::string
Node::getPluginIconFilePath() const
{
    PluginPtr plugin = getPlugin();
    if (!plugin) {
        return std::string();
    }

    std::string resourcesPath = plugin->getProperty<std::string>(kNatronPluginPropResourcesPath);
    std::string filename = plugin->getProperty<std::string>(kNatronPluginPropIconFilePath);
    if (filename.empty()) {
        return filename;
    }
    return resourcesPath + filename;
}

bool
Node::isPyPlug() const
{
    if (!_imp->isPyPlug) {
        return false;
    }
    NodeGroupPtr isGrp = toNodeGroup(_imp->effect);
    if (!isGrp) {
        return false;
    }
    return !isGrp->isSubGraphEditedByUser();
}

std::string
Node::getPluginID() const
{
    PluginPtr plugin = getPlugin();
    if (!plugin) {
        return std::string();
    }

    return plugin->getPluginID();
}

std::string
Node::getPluginLabel() const
{
    PluginPtr plugin = getPlugin();
    if (!plugin) {
        return std::string();
    }
    return plugin->getPluginLabel();
}

std::string
Node::getPluginResourcesPath() const
{
    PluginPtr plugin = getPlugin();
    if (!plugin) {
        return std::string();
    }
    return plugin->getProperty<std::string>(kNatronPluginPropResourcesPath);
}

std::string
Node::getPluginDescription() const
{
    PluginPtr plugin = getPlugin();
    if (!plugin) {
        return std::string();
    }
    return plugin->getProperty<std::string>(kNatronPluginPropDescription);
}

void
Node::getPluginGrouping(std::vector<std::string>* grouping) const
{
    PluginPtr plugin = getPlugin();
    if (!plugin) {
        return;
    }
    *grouping = plugin->getPropertyN<std::string>(kNatronPluginPropGrouping);
}

static std::string removeTrailingDigits(const std::string& str)
{
    if (str.empty()) {
        return std::string();
    }
    std::size_t i = str.size() - 1;
    while (i > 0 && std::isdigit(str[i])) {
        --i;
    }

    if (i == 0) {
        // Name only consists of digits
        return std::string();
    }

    return str.substr(0, i + 1);
}




bool
Node::message(MessageTypeEnum type,
              const std::string & content) const
{
    ///If the node was aborted, don't transmit any message because we could cause a deadlock
    if ( _imp->effect->aborted() || (!_imp->nodeCreated && _imp->wasCreatedSilently) || _imp->restoringDefaults) {
        return false;
    }

    // See https://github.com/MrKepzie/Natron/issues/1313
    // Messages posted from a separate thread should be logged and not show a pop-up
    if ( QThread::currentThread() != qApp->thread() ) {

        LogEntry::LogEntryColor c;
        if (getColor(&c.r, &c.g, &c.b)) {
            c.colorSet = true;
        }
        appPTR->writeToErrorLog_mt_safe(QString::fromUtf8( getLabel_mt_safe().c_str() ), QDateTime::currentDateTime(), QString::fromUtf8( content.c_str() ), false, c);
        if (type == eMessageTypeError) {
            appPTR->showErrorLog();
        }
        return true;
    }

    NodePtr ioContainer = getIOContainer();
    if (ioContainer) {
        ioContainer->message(type, content);
        return true;
    }
    // Some nodes may be hidden from the user but may still report errors (such that the group is in fact hidden to the user)
    if ( !isPersistent() && getGroup() ) {
        NodeGroup* isGroup = dynamic_cast<NodeGroup*>( getGroup().get() );
        if (isGroup) {
            isGroup->message(type, content);
        }
    }

    switch (type) {
    case eMessageTypeInfo:
        Dialogs::informationDialog(getLabel_mt_safe(), content);

        return true;
    case eMessageTypeWarning:
        Dialogs::warningDialog(getLabel_mt_safe(), content);

        return true;
    case eMessageTypeError:
        Dialogs::errorDialog(getLabel_mt_safe(), content);

        return true;
    case eMessageTypeQuestion:

        return Dialogs::questionDialog(getLabel_mt_safe(), content, false) == eStandardButtonYes;
    default:

        return false;
    }
}

void
Node::setPersistentMessage(MessageTypeEnum type,
                           const std::string & content)
{
    if (!_imp->nodeCreated && _imp->wasCreatedSilently) {
        return;
    }

    if ( !appPTR->isBackground() ) {
        //if the message is just an information, display a popup instead.
        NodePtr ioContainer = getIOContainer();
        if (ioContainer) {
            ioContainer->setPersistentMessage(type, content);

            return;
        }
        // Some nodes may be hidden from the user but may still report errors (such that the group is in fact hidden to the user)
        if ( !isPersistent() && getGroup() ) {
            NodeGroupPtr isGroup = toNodeGroup(getGroup());
            if (isGroup) {
                isGroup->setPersistentMessage(type, content);
            }
        }

        if (type == eMessageTypeInfo) {
            message(type, content);

            return;
        }

        {
            QMutexLocker k(&_imp->persistentMessageMutex);
            QString mess = QString::fromUtf8( content.c_str() );
            if (mess == _imp->persistentMessage) {
                return;
            }
            _imp->persistentMessageType = (int)type;
            _imp->persistentMessage = mess;
        }
        Q_EMIT persistentMessageChanged();
    } else {
        std::cout << "Persistent message: " << content << std::endl;
    }
}

bool
Node::hasPersistentMessage() const
{
    QMutexLocker k(&_imp->persistentMessageMutex);

    return !_imp->persistentMessage.isEmpty();
}

void
Node::getPersistentMessage(QString* message,
                           int* type,
                           bool prefixLabelAndType) const
{
    QMutexLocker k(&_imp->persistentMessageMutex);

    *type = _imp->persistentMessageType;

    if ( prefixLabelAndType && !_imp->persistentMessage.isEmpty() ) {
        message->append( QString::fromUtf8( getLabel_mt_safe().c_str() ) );
        if (*type == eMessageTypeError) {
            message->append( QString::fromUtf8(" error: ") );
        } else if (*type == eMessageTypeWarning) {
            message->append( QString::fromUtf8(" warning: ") );
        }
    }
    message->append(_imp->persistentMessage);
}

void
Node::clearPersistentMessageRecursive(std::list<NodePtr>& markedNodes)
{
    if ( std::find(markedNodes.begin(), markedNodes.end(), shared_from_this()) != markedNodes.end() ) {
        return;
    }
    markedNodes.push_back(shared_from_this());
    clearPersistentMessageInternal();

    int nInputs = getMaxInputCount();
    ///No need to lock, inputs is only written to by the main-thread
    for (int i = 0; i < nInputs; ++i) {
        NodePtr input = getInput(i);
        if (input) {
            input->clearPersistentMessageRecursive(markedNodes);
        }
    }
}

void
Node::clearPersistentMessageInternal()
{
    bool changed;
    {
        QMutexLocker k(&_imp->persistentMessageMutex);
        changed = !_imp->persistentMessage.isEmpty();
        if (changed) {
            _imp->persistentMessage.clear();
        }
    }

    if (changed) {
        Q_EMIT persistentMessageChanged();
    }
}

void
Node::clearPersistentMessage(bool recurse)
{
    AppInstancePtr app = getApp();

    if (!app) {
        return;
    }
    if ( app->isBackground() ) {
        return;
    }
    if (recurse) {
        std::list<NodePtr> markedNodes;
        clearPersistentMessageRecursive(markedNodes);
    } else {
        clearPersistentMessageInternal();
    }
}


bool
Node::notifyInputNIsRendering(int inputNb)
{
    if ( !getApp() || getApp()->isGuiFrozen() ) {
        return false;
    }

    timeval now;


    gettimeofday(&now, 0);

    QMutexLocker l(&_imp->lastRenderStartedMutex);
    double t =  now.tv_sec  - _imp->lastInputNRenderStartedSlotCallTime.tv_sec +
               (now.tv_usec - _imp->lastInputNRenderStartedSlotCallTime.tv_usec) * 1e-6f;

    assert( inputNb >= 0 && inputNb < (int)_imp->inputIsRenderingCounter.size() );

    if ( (t > NATRON_RENDER_GRAPHS_HINTS_REFRESH_RATE_SECONDS) || (_imp->inputIsRenderingCounter[inputNb] == 0) ) {
        _imp->lastInputNRenderStartedSlotCallTime = now;
        ++_imp->inputIsRenderingCounter[inputNb];

        l.unlock();

        Q_EMIT inputNIsRendering(inputNb);

        return true;
    }

    return false;
}

void
Node::notifyInputNIsFinishedRendering(int inputNb)
{
    {
        QMutexLocker l(&_imp->lastRenderStartedMutex);
        assert( inputNb >= 0 && inputNb < (int)_imp->inputIsRenderingCounter.size() );
        --_imp->inputIsRenderingCounter[inputNb];
    }
    Q_EMIT inputNIsFinishedRendering(inputNb);
}

bool
Node::notifyRenderingStarted()
{
    if ( !getApp() || getApp()->isGuiFrozen() ) {
        return false;
    }

    timeval now;

    gettimeofday(&now, 0);


    QMutexLocker l(&_imp->lastRenderStartedMutex);
    double t =  now.tv_sec  - _imp->lastRenderStartedSlotCallTime.tv_sec +
               (now.tv_usec - _imp->lastRenderStartedSlotCallTime.tv_usec) * 1e-6f;

    if ( (t > NATRON_RENDER_GRAPHS_HINTS_REFRESH_RATE_SECONDS) || (_imp->renderStartedCounter == 0) ) {
        _imp->lastRenderStartedSlotCallTime = now;
        ++_imp->renderStartedCounter;


        l.unlock();

        Q_EMIT renderingStarted();

        return true;
    }

    return false;
}

void
Node::notifyRenderingEnded()
{
    {
        QMutexLocker l(&_imp->lastRenderStartedMutex);
        --_imp->renderStartedCounter;
    }
    Q_EMIT renderingEnded();
}

int
Node::getIsInputNRenderingCounter(int inputNb) const
{
    QMutexLocker l(&_imp->lastRenderStartedMutex);

    assert( inputNb >= 0 && inputNb < (int)_imp->inputIsRenderingCounter.size() );

    return _imp->inputIsRenderingCounter[inputNb];
}

int
Node::getIsNodeRenderingCounter() const
{
    QMutexLocker l(&_imp->lastRenderStartedMutex);

    return _imp->renderStartedCounter;
}


QMutex &
Node::getRenderInstancesSharedMutex()
{
    return _imp->renderInstancesSharedMutex;
}


NodePtr
Node::getIOContainer() const
{
    return _imp->ioContainer.lock();
}



void
Node::getOriginalFrameRangeForReader(const std::string& pluginID,
                                     const std::string& canonicalFileName,
                                     int* firstFrame,
                                     int* lastFrame)
{
    if (pluginID == PLUGINID_OFX_READFFMPEG) {
        ///If the plug-in is a video, only ffmpeg may know how many frames there are
        *firstFrame = INT_MIN;
        *lastFrame = INT_MAX;
    } else {
        SequenceParsing::SequenceFromPattern seq;
        FileSystemModel::filesListFromPattern(canonicalFileName, &seq);
        if ( seq.empty() || (seq.size() == 1) ) {
            *firstFrame = 1;
            *lastFrame = 1;
        } else if (seq.size() > 1) {
            *firstFrame = seq.begin()->first;
            *lastFrame = seq.rbegin()->first;
        }
    }
}

void
Node::computeFrameRangeForReader(const KnobIPtr& fileKnob, bool setFrameRange)
{
    /*
       We compute the original frame range of the sequence for the plug-in
       because the plug-in does not have access to the exact original pattern
       hence may not exactly end-up with the same file sequence as what the user
       selected from the file dialog.
     */
    ReadNodePtr isReadNode = isEffectReadNode();
    std::string pluginID;

    if (isReadNode) {
        NodePtr embeddedPlugin = isReadNode->getEmbeddedReader();
        if (embeddedPlugin) {
            pluginID = embeddedPlugin->getPluginID();
        }
    } else {
        pluginID = getPluginID();
    }

    int leftBound = INT_MIN;
    int rightBound = INT_MAX;
    ///Set the originalFrameRange parameter of the reader if it has one.
    KnobIntPtr originalFrameRangeKnob = toKnobInt(getKnobByName(kReaderParamNameOriginalFrameRange));
    KnobIntPtr firstFrameKnob = toKnobInt(getKnobByName(kReaderParamNameFirstFrame));
    KnobIntPtr lastFrameKnob = toKnobInt(getKnobByName(kReaderParamNameLastFrame));
    if (originalFrameRangeKnob) {
        if (originalFrameRangeKnob->getNDimensions() == 2){
            KnobFilePtr isFile = toKnobFile(fileKnob);
            assert(isFile);
            if (!isFile) {
                throw std::logic_error("Node::computeFrameRangeForReader");
            }

            if (pluginID == PLUGINID_OFX_READFFMPEG) {
                ///If the plug-in is a video, only ffmpeg may know how many frames there are
                std::vector<int> frameRange(2);
                frameRange[0] = INT_MIN;
                frameRange[1] = INT_MAX;
                originalFrameRangeKnob->setValueAcrossDimensions(frameRange);
            } else {
                std::string pattern = isFile->getRawFileName();
                getApp()->getProject()->canonicalizePath(pattern);
                SequenceParsing::SequenceFromPattern seq;
                FileSystemModel::filesListFromPattern(pattern, &seq);
                if ( seq.empty() || (seq.size() == 1) ) {
                    leftBound = 1;
                    rightBound = 1;
                } else if (seq.size() > 1) {
                    leftBound = seq.begin()->first;
                    rightBound = seq.rbegin()->first;
                }
                std::vector<int> frameRange(2);
                frameRange[0] = leftBound;
                frameRange[1] = rightBound;
                originalFrameRangeKnob->setValueAcrossDimensions(frameRange);

                if (setFrameRange) {
                    if (firstFrameKnob) {
                        firstFrameKnob->setValue(leftBound);
                        firstFrameKnob->setRange(leftBound, rightBound);
                    }
                    if (lastFrameKnob) {
                        lastFrameKnob->setValue(rightBound);
                        lastFrameKnob->setRange(leftBound, rightBound);
                    }
                }
            }
        }
    }

}

bool
Node::isSubGraphEditedByUser() const
{
    
    NodeGroupPtr isGroup = isEffectNodeGroup();
    if (!isGroup) {
        return false;
    }
    return isGroup->isSubGraphEditedByUser();
}




const std::vector<std::string>&
Node::getCreatedViews() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->createdViews;
}

void
Node::refreshCreatedViews()
{
    KnobIPtr knob = getKnobByName(kReadOIIOAvailableViewsKnobName);

    if (knob) {
        refreshCreatedViews(knob);
    }
}

void
Node::refreshCreatedViews(const KnobIPtr& knob)
{
    assert( QThread::currentThread() == qApp->thread() );

    KnobStringPtr availableViewsKnob = toKnobString(knob);
    if (!availableViewsKnob) {
        return;
    }
    QString value = QString::fromUtf8( availableViewsKnob->getValue().c_str() );
    QStringList views = value.split( QLatin1Char(',') );

    bool isSingleView = views.size() == 1 && views.front().compare(QString::fromUtf8("Main"), Qt::CaseInsensitive) == 0;

    _imp->createdViews.clear();

    const std::vector<std::string>& projectViews = getApp()->getProject()->getProjectViewNames();
    QStringList qProjectViews;
    for (std::size_t i = 0; i < projectViews.size(); ++i) {
        qProjectViews.push_back( QString::fromUtf8( projectViews[i].c_str() ) );
    }

    QStringList missingViews;
    if (!isSingleView) {
        for (QStringList::Iterator it = views.begin(); it != views.end(); ++it) {
            if ( !qProjectViews.contains(*it, Qt::CaseInsensitive) && !it->isEmpty() ) {
                missingViews.push_back(*it);
            }
            _imp->createdViews.push_back( it->toStdString() );
        }
    }

    if ( !missingViews.isEmpty() ) {
        KnobIPtr fileKnob = getKnobByName(kOfxImageEffectFileParamName);
        KnobFilePtr inputFileKnob = toKnobFile(fileKnob);
        if (inputFileKnob) {
            std::string filename = inputFileKnob->getValue();
            std::stringstream ss;
            for (int i = 0; i < missingViews.size(); ++i) {
                ss << missingViews[i].toStdString();
                if (i < missingViews.size() - 1) {
                    ss << ", ";
                }
            }
            ss << std::endl;
            ss << std::endl;
            ss << tr("These views are in %1 but do not exist in the project.\nWould you like to create them?").arg( QString::fromUtf8( filename.c_str() ) ).toStdString();
            std::string question  = ss.str();
            StandardButtonEnum rep = Dialogs::questionDialog(tr("Views available").toStdString(), question, false, StandardButtons(eStandardButtonYes | eStandardButtonNo), eStandardButtonYes);
            if (rep == eStandardButtonYes) {
                std::vector<std::string> viewsToCreate;
                for (QStringList::Iterator it = missingViews.begin(); it != missingViews.end(); ++it) {
                    viewsToCreate.push_back( it->toStdString() );
                }
                getApp()->getProject()->createProjectViews(viewsToCreate);
            }
        }
    }

    Q_EMIT availableViewsChanged();
} // Node::refreshCreatedViews

bool
Node::getHideInputsKnobValue() const
{
    KnobBoolPtr k = _imp->hideInputs.lock();

    if (!k) {
        return false;
    }

    return k->getValue();
}

void
Node::setHideInputsKnobValue(bool hidden)
{
    KnobBoolPtr k = _imp->hideInputs.lock();

    if (!k) {
        return;
    }
    k->setValue(hidden);
}

void
Node::onRefreshIdentityStateRequestReceived()
{
#pragma message WARN("Do this in NodeGui instead")
    if ( !_imp->guiPointer.lock() ) {
        return;
    }

    assert( QThread::currentThread() == qApp->thread() );
    int refreshStateCount;
    {
        QMutexLocker k(&_imp->refreshIdentityStateRequestsCountMutex);
        refreshStateCount = _imp->refreshIdentityStateRequestsCount;
        _imp->refreshIdentityStateRequestsCount = 0;
    }
    if ( (refreshStateCount == 0) || !_imp->effect ) {
        //was already processed
        return;
    }

    ProjectPtr project = getApp()->getProject();
    if (project->isLoadingProject()) {
        return;
    }
    TimeValue time = project->currentFrame();
    RenderScale scale(1.);
    double inputTime = 0;
    U64 hash = 0;
    bool viewAware =  _imp->effect->isViewAware();
    int nViews = !viewAware ? 1 : project->getProjectViewsCount();

    RectI format = _imp->effect->getOutputFormat();

    //The one view node might report it is identity, but we do not want it to display it


    bool isIdentity = false;
    int inputNb = -1;
    OneViewNodePtr isOneView = isEffectOneViewNode();
    if (!isOneView) {
        for (int i = 0; i < nViews; ++i) {
            int identityInputNb = -1;
            ViewIdx identityView;
            bool isViewIdentity = _imp->effect->isIdentity_public(true, hash, time, scale, format, ViewIdx(i), &inputTime, &identityView, &identityInputNb);
            if ( (i > 0) && ( (isViewIdentity != isIdentity) || (identityInputNb != inputNb) || (identityView.value() != i) ) ) {
                isIdentity = false;
                inputNb = -1;
                break;
            }
            isIdentity |= isViewIdentity;
            inputNb = identityInputNb;
            if (!isIdentity) {
                break;
            }
        }
    }

    //Check for consistency across views or then say the effect is not identity since the UI cannot display 2 different states
    //depending on the view


    NodeGuiIPtr nodeUi = _imp->guiPointer.lock();
    if (nodeUi) {
        nodeUi->onIdentityStateChanged(isIdentity ? inputNb : -1);
    }
} // Node::onRefreshIdentityStateRequestReceived

void
Node::refreshIdentityState()
{


    //Post a new request
    {
        QMutexLocker k(&_imp->refreshIdentityStateRequestsCountMutex);
        ++_imp->refreshIdentityStateRequestsCount;
    }
    Q_EMIT refreshIdentityStateRequested();
}

KnobStringPtr
Node::getExtraLabelKnob() const
{
    return _imp->nodeLabelKnob.lock();
}

KnobStringPtr
Node::getOFXSubLabelKnob() const
{
    return _imp->ofxSubLabelKnob.lock();
}


KnobBoolPtr
Node::getDisabledKnob() const
{
    return _imp->disableNodeKnob.lock();
}

bool
Node::isLifetimeActivated(int *firstFrame,
                          int *lastFrame) const
{
    KnobBoolPtr enableLifetimeKnob = _imp->enableLifeTimeKnob.lock();

    if (!enableLifetimeKnob) {
        return false;
    }
    if ( !enableLifetimeKnob->getValue() ) {
        return false;
    }
    KnobIntPtr lifetimeKnob = _imp->lifeTimeKnob.lock();
    *firstFrame = lifetimeKnob->getValue(DimIdx(0));
    *lastFrame = lifetimeKnob->getValue(DimIdx(1));

    return true;
}

KnobBoolPtr
Node::getLifeTimeEnabledKnob() const
{
    return _imp->enableLifeTimeKnob.lock();
}

KnobIntPtr
Node::getLifeTimeKnob() const
{
    return _imp->lifeTimeKnob.lock();
}

bool
Node::isNodeDisabledForFrame(TimeValue time, ViewIdx view) const
{
    // Check disabled knob
    KnobBoolPtr b = _imp->disableNodeKnob.lock();
    if (b && b->getValueAtTime(time, DimIdx(0), view)) {
        return true;
    }

    NodeGroupPtr isContainerGrp = toNodeGroup( getGroup() );

    // If within a group, check the group's disabled knob
    if (isContainerGrp) {
        if (isContainerGrp->getNode()->isNodeDisabledForFrame(time, view)) {
            return true;
        }
    }

    // If an internal IO node, check the main meta-node disabled knob
    NodePtr ioContainer = getIOContainer();
    if (ioContainer) {
        return ioContainer->isNodeDisabledForFrame(time, view);
    }

    RotoDrawableItemPtr attachedItem = getAttachedRotoItem();
    if (attachedItem && !attachedItem->isActivated(time, view)) {
        return true;
    }

    // Check lifetime
    int lifeTimeFirst, lifeTimeEnd;
    bool lifeTimeEnabled = isLifetimeActivated(&lifeTimeFirst, &lifeTimeEnd);
    bool enabled = ( !lifeTimeEnabled || (time >= lifeTimeFirst && time <= lifeTimeEnd) );
    return !enabled;
} // isNodeDisabledForFrame

bool
Node::getDisabledKnobValue() const
{
    // Check disabled knob
    KnobBoolPtr b = _imp->disableNodeKnob.lock();
    if (b && b->getValue()) {
        return true;
    }

    NodeGroupPtr isContainerGrp = toNodeGroup( getGroup() );

    // If within a group, check the group's disabled knob
    if (isContainerGrp) {
        if (isContainerGrp->getNode()->getDisabledKnobValue()) {
            return true;
        }
    }

    // If an internal IO node, check the main meta-node disabled knob
    NodePtr ioContainer = getIOContainer();
    if (ioContainer) {
        return ioContainer->getDisabledKnobValue();
    }


    int lifeTimeFirst, lifeTimeEnd;
    bool lifeTimeEnabled = isLifetimeActivated(&lifeTimeFirst, &lifeTimeEnd);
    double curFrame = _imp->effect->getCurrentTime_TLS();
    bool enabled = ( !lifeTimeEnabled || (curFrame >= lifeTimeFirst && curFrame <= lifeTimeEnd) );

    return !enabled;
}

void
Node::setNodeDisabled(bool disabled)
{
    KnobBoolPtr b = _imp->disableNodeKnob.lock();

    if (b) {
        b->setValue(disabled);
    }
}


ImageBitDepthEnum
Node::getClosestSupportedBitDepth(ImageBitDepthEnum depth)
{
    bool foundShort = false;
    bool foundByte = false;

    for (std::list<ImageBitDepthEnum>::const_iterator it = _imp->supportedDepths.begin(); it != _imp->supportedDepths.end(); ++it) {
        if (*it == depth) {
            return depth;
        } else if (*it == eImageBitDepthFloat) {
            return eImageBitDepthFloat;
        } else if (*it == eImageBitDepthShort) {
            foundShort = true;
        } else if (*it == eImageBitDepthByte) {
            foundByte = true;
        }
    }
    if (foundShort) {
        return eImageBitDepthShort;
    } else if (foundByte) {
        return eImageBitDepthByte;
    } else {
        ///The plug-in doesn't support any bitdepth, the program shouldn't even have reached here.
        assert(false);

        return eImageBitDepthNone;
    }
}

ImageBitDepthEnum
Node::getBestSupportedBitDepth() const
{
    bool foundShort = false;
    bool foundByte = false;

    for (std::list<ImageBitDepthEnum>::const_iterator it = _imp->supportedDepths.begin(); it != _imp->supportedDepths.end(); ++it) {
        switch (*it) {
        case eImageBitDepthByte:
            foundByte = true;
            break;

        case eImageBitDepthShort:
            foundShort = true;
            break;

        case eImageBitDepthHalf:
            break;

        case eImageBitDepthFloat:

            return eImageBitDepthFloat;

        case eImageBitDepthNone:
            break;
        }
    }

    if (foundShort) {
        return eImageBitDepthShort;
    } else if (foundByte) {
        return eImageBitDepthByte;
    } else {
        ///The plug-in doesn't support any bitdepth, the program shouldn't even have reached here.
        assert(false);

        return eImageBitDepthNone;
    }
}

bool
Node::isSupportedBitDepth(ImageBitDepthEnum depth) const
{
    return std::find(_imp->supportedDepths.begin(), _imp->supportedDepths.end(), depth) != _imp->supportedDepths.end();
}

std::string
Node::getNodeExtraLabel() const
{
    KnobStringPtr s = _imp->nodeLabelKnob.lock();

    if (s) {
        return s->getValue();
    } else {
        return std::string();
    }
}



bool
Node::isBackdropNode() const
{
    return getPluginID() == PLUGINID_NATRON_BACKDROP;
}

void
Node::updateEffectSubLabelKnob(const QString & name)
{
    if (!_imp->effect) {
        return;
    }
    KnobStringPtr strKnob = _imp->ofxSubLabelKnob.lock();
    if (strKnob) {
        strKnob->setValue( name.toStdString() );
    }
}




void
Node::onNodeMetadatasRefreshedOnMainThread(const NodeMetadata& meta)
{

    // Refresh warnings
    _imp->refreshMetadaWarnings(meta);

    // Refresh identity state
    refreshIdentityState();

    // Refresh default layer & mask channel selectors
    refreshChannelSelectors();

    // Premult might have changed, check for warnings
    checkForPremultWarningAndCheckboxes();

    // Refresh channel checkbox and layer selectors visibility
    refreshLayersSelectorsVisibility();
}



void
Node::setPosition(double x,
                  double y)
{
    NodeGuiIPtr gui = _imp->guiPointer.lock();

    if (gui) {
        gui->setPosition(x, y);
    } else {
        onNodeUIPositionChanged(x,y);
    }
}

void
Node::getPosition(double *x,
                  double *y) const
{
    QMutexLocker k(&_imp->nodeUIDataMutex);
    *x = _imp->nodePositionCoords[0];
    *y = _imp->nodePositionCoords[1];
}

void
Node::onNodeUIPositionChanged(double x, double y)
{
    QMutexLocker k(&_imp->nodeUIDataMutex);
    _imp->nodePositionCoords[0] = x;
    _imp->nodePositionCoords[1] = y;
}

void
Node::onNodeUISizeChanged(double w,
              double h)
{
    QMutexLocker k(&_imp->nodeUIDataMutex);
    _imp->nodeSize[0] = w;
    _imp->nodeSize[1] = h;
}


void
Node::setSize(double w,
              double h)
{
    NodeGuiIPtr gui = _imp->guiPointer.lock();

    if (gui) {
        gui->setSize(w, h);
    } else {
        onNodeUISizeChanged(w,h);
    }
}


void
Node::getSize(double* w,
              double* h) const
{
    QMutexLocker k(&_imp->nodeUIDataMutex);
    *w = _imp->nodeSize[0];
    *h = _imp->nodeSize[1];
}

bool
Node::getColor(double* r,
               double *g,
               double* b) const
{
    QMutexLocker k(&_imp->nodeUIDataMutex);
    *r = _imp->nodeColor[0];
    *g = _imp->nodeColor[1];
    *b = _imp->nodeColor[2];
    return true;
}

void
Node::getDefaultColor(double* r, double *g, double* b) const
{
    std::vector<std::string> grouping;
    getPluginGrouping(&grouping);
    std::string majGroup = grouping.empty() ? "" : grouping.front();
    float defR, defG, defB;

    SettingsPtr settings = appPTR->getCurrentSettings();

    if ( _imp->effect->isReader() ) {
        settings->getReaderColor(&defR, &defG, &defB);
    } else if ( _imp->effect->isWriter() ) {
        settings->getWriterColor(&defR, &defG, &defB);
    } else if ( _imp->effect->isGenerator() ) {
        settings->getGeneratorColor(&defR, &defG, &defB);
    } else if (majGroup == PLUGIN_GROUP_COLOR) {
        settings->getColorGroupColor(&defR, &defG, &defB);
    } else if (majGroup == PLUGIN_GROUP_FILTER) {
        settings->getFilterGroupColor(&defR, &defG, &defB);
    } else if (majGroup == PLUGIN_GROUP_CHANNEL) {
        settings->getChannelGroupColor(&defR, &defG, &defB);
    } else if (majGroup == PLUGIN_GROUP_KEYER) {
        settings->getKeyerGroupColor(&defR, &defG, &defB);
    } else if (majGroup == PLUGIN_GROUP_MERGE) {
        settings->getMergeGroupColor(&defR, &defG, &defB);
    } else if (majGroup == PLUGIN_GROUP_PAINT) {
        settings->getDrawGroupColor(&defR, &defG, &defB);
    } else if (majGroup == PLUGIN_GROUP_TIME) {
        settings->getTimeGroupColor(&defR, &defG, &defB);
    } else if (majGroup == PLUGIN_GROUP_TRANSFORM) {
        settings->getTransformGroupColor(&defR, &defG, &defB);
    } else if (majGroup == PLUGIN_GROUP_MULTIVIEW) {
        settings->getViewsGroupColor(&defR, &defG, &defB);
    } else if (majGroup == PLUGIN_GROUP_DEEP) {
        settings->getDeepGroupColor(&defR, &defG, &defB);
    } else if (dynamic_cast<Backdrop*>(_imp->effect.get())) {
        settings->getDefaultBackdropColor(&defR, &defG, &defB);
    } else {
        settings->getDefaultNodeColor(&defR, &defG, &defB);
    }
    *r = defR;
    *g = defG;
    *b = defB;

}

bool
Node::hasColorChangedSinceDefault() const
{
    double defR, defG, defB;
    getDefaultColor(&defR, &defG, &defB);
    if ( (std::abs(_imp->nodeColor[0] - defR) > 0.01) || (std::abs(_imp->nodeColor[1] - defG) > 0.01) || (std::abs(_imp->nodeColor[2] - defB) > 0.01) ) {
        return true;
    }

    return false;
}

void
Node::onNodeUIColorChanged(double r,
                           double g,
                           double b)
{
    QMutexLocker k(&_imp->nodeUIDataMutex);
    _imp->nodeColor[0] = r;
    _imp->nodeColor[1] = g;
    _imp->nodeColor[2] = b;

}


void
Node::setColor(double r,
               double g,
               double b)
{
    NodeGuiIPtr gui = _imp->guiPointer.lock();

    if (gui) {
        gui->setColor(r, g, b);
    } else {
        onNodeUIColorChanged(r,g,b);
    }
}



void
Node::onNodeUISelectionChanged(bool isSelected)
{
    QMutexLocker k(&_imp->nodeUIDataMutex);
    _imp->nodeIsSelected = isSelected;
}

bool
Node::getNodeIsSelected() const
{
    QMutexLocker k(&_imp->nodeUIDataMutex);
    return _imp->nodeIsSelected;
}



void
Node::setNodeGuiPointer(const NodeGuiIPtr& gui)
{
    assert( !_imp->guiPointer.lock() );
    assert( QThread::currentThread() == qApp->thread() );
    _imp->guiPointer = gui;
}

NodeGuiIPtr
Node::getNodeGui() const
{
    return _imp->guiPointer.lock();
}

bool
Node::isUserSelected() const
{
    NodeGuiIPtr gui = _imp->guiPointer.lock();

    if (!gui) {
        return false;
    }

    return gui->isUserSelected();
}

bool
Node::isSettingsPanelMinimized() const
{
    NodeGuiIPtr gui = _imp->guiPointer.lock();

    if (!gui) {
        return false;
    }

    return gui->isSettingsPanelMinimized();
}

bool
Node::isSettingsPanelVisibleInternal(std::set<NodeConstPtr>& recursionList) const
{
    NodeGuiIPtr gui = _imp->guiPointer.lock();

    if (!gui) {
        return false;
    }

    NodeConstPtr thisShared = shared_from_this();
    if ( recursionList.find(thisShared) != recursionList.end() ) {
        return false;
    }
    recursionList.insert(thisShared);

    return gui->isSettingsPanelVisible();
}

bool
Node::isSettingsPanelVisible() const
{
    std::set<NodeConstPtr> tmplist;

    return isSettingsPanelVisibleInternal(tmplist);
}

void
Node::attachRotoItem(const RotoDrawableItemPtr& stroke)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->paintStroke = stroke;
    setProcessChannelsValues(true, true, true, true);
}

RotoDrawableItemPtr
Node::getOriginalAttachedItem() const
{
    return _imp->paintStroke.lock();
}

RotoDrawableItemPtr
Node::getAttachedRotoItem() const
{
    EffectInstancePtr effect = getEffectInstance();
    if (!effect) {
        return RotoDrawableItemPtr();
    }
    RotoDrawableItemPtr thisItem = _imp->paintStroke.lock();
    if (!thisItem) {
        return thisItem;
    }
    // On a render thread, use the local thread copy
    ParallelRenderArgsPtr tls = effect->getParallelRenderArgsTLS();
    if (tls && thisItem->isRenderCloneNeeded()) {
        return thisItem->getCachedDrawable(tls->abortInfo.lock());
    }
    return thisItem;
}



KnobBoolPtr
Node::getProcessAllLayersKnob() const
{
    return _imp->processAllLayersKnob.lock();
}

void
Node::checkForPremultWarningAndCheckboxes()
{
    if ( isOutputNode() || _imp->effect->isGenerator() || _imp->effect->isReader() ) {
        return;
    }
    KnobBoolPtr chans[4];
    KnobStringPtr premultWarn = _imp->premultWarning.lock();
    if (!premultWarn) {
        return;
    }
    NodePtr prefInput = getPreferredInputNode();

    if ( !prefInput ) {
        //No input, do not warn
        premultWarn->setSecret(true);

        return;
    }
    for (int i = 0; i < 4; ++i) {
        chans[i] = _imp->enabledChan[i].lock();

        //No checkboxes
        if (!chans[i]) {
            premultWarn->setSecret(true);

            return;
        }

        //not RGBA
        if ( chans[i]->getIsSecret() ) {
            return;
        }
    }

    ImagePremultiplicationEnum premult = _imp->effect->getPremult();

    //not premult
    if (premult != eImagePremultiplicationPremultiplied) {
        premultWarn->setSecret(true);

        return;
    }

    bool checked[4];
    checked[3] = chans[3]->getValue();

    //alpha unchecked
    if (!checked[3]) {
        premultWarn->setSecret(true);

        return;
    }
    for (int i = 0; i < 3; ++i) {
        checked[i] = chans[i]->getValue();
        if (!checked[i]) {
            premultWarn->setSecret(false);

            return;
        }
    }

    //RGB checked
    premultWarn->setSecret(true);
} // Node::checkForPremultWarningAndCheckboxes



double
Node::getHostMixingValue(TimeValue time,
                         ViewIdx view) const
{
    KnobDoublePtr mix = _imp->mixWithSource.lock();

    return mix ? mix->getValueAtTime(time, DimIdx(0), view) : 1.;
}


NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_Node.cpp"
