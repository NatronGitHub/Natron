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

#include "Node.h"

#include <limits>
#include <locale>
#include <algorithm> // min, max
#include <bitset>
#include <cassert>
#include <stdexcept>

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

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
#include "Engine/AbortableRenderInfo.h"
#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Backdrop.h"
#include "Engine/Curve.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/DiskCacheNode.h"
#include "Engine/Dot.h"
#include "Engine/EffectInstance.h"
#include "Engine/Format.h"
#include "Engine/FileSystemModel.h"
#include "Engine/FStreamsSupport.h"
#include "Engine/GroupInput.h"
#include "Engine/GroupOutput.h"
#include "Engine/GenericSchedulerThreadWatcher.h"
#include "Engine/Hash64.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
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
#include "Engine/Plugin.h"
#include "Engine/PrecompNode.h"
#include "Engine/Project.h"
#include "Engine/ReadNode.h"
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
#include "Engine/TLSHolder.h"
#include "Engine/UndoCommand.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerNode.h"
#include "Engine/WriteNode.h"

#include "Serialization/KnobSerialization.h"
#include "Serialization/NodeSerialization.h"
#include "Serialization/NodeClipBoard.h"
#include "Serialization/SerializationIO.h"

///The flickering of edges/nodes in the nodegraph will be refreshed
///at most every...
#define NATRON_RENDER_GRAPHS_HINTS_REFRESH_RATE_SECONDS 1


NATRON_NAMESPACE_ENTER;

using std::make_pair;
using std::cout; using std::endl;
using boost::shared_ptr;


/**
 *@brief Actually converting to ARGB... but it is called BGRA by
   the texture format GL_UNSIGNED_INT_8_8_8_8_REV
 **/
static unsigned int toBGRA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) WARN_UNUSED_RETURN;
unsigned int
toBGRA(unsigned char r,
       unsigned char g,
       unsigned char b,
       unsigned char a)
{
    return (a << 24) | (r << 16) | (g << 8) | b;
}

Node::Node(const AppInstancePtr& app,
           const NodeCollectionPtr& group,
           const PluginPtr& plugin)
    : QObject()
    , _imp( new NodePrivate(this, app, group, plugin) )
{
    QObject::connect( this, SIGNAL(pluginMemoryUsageChanged(qint64)), appPTR, SLOT(onNodeMemoryRegistered(qint64)) );
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

static bool findAvailableName(const std::string &baseName, const NodeCollectionPtr& group, const NodePtr& node, std::string* name)
{
    *name = baseName;
    int no = 1;
    do {
        if (no > 1) {
            std::stringstream ss;
            ss << baseName;
            ss << '_';
            ss << no;
            *name = ss.str();
        }
        ++no;
    } while ( group && group->checkIfNodeNameExists(*name, node) );

    // When no == 2 the name was available
    return no == 2;
}

void
Node::initNodeNameFallbackOnPluginDefault()
{
    NodeCollectionPtr group = getGroup();
    std::string name;
    std::string pluginLabel;
    AppManager::AppTypeEnum appType = appPTR->getAppType();
    PluginPtr plugin = getPlugin();
    if ( plugin &&
        ( ( appType == AppManager::eAppTypeBackground) ||
         ( appType == AppManager::eAppTypeGui) ||
         ( appType == AppManager::eAppTypeInterpreter) ) ) {
            pluginLabel = plugin->getLabelWithoutSuffix();
        } else {
            pluginLabel = plugin->getPluginLabel();
        }
    try {
        if (group) {
            group->initNodeName(plugin->getPluginID(), pluginLabel, &name);
        } else {
            name = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(pluginLabel);
        }
    } catch (...) {
    }

    setNameInternal(name.c_str(), false);
}

void
Node::initNodeScriptName(const SERIALIZATION_NAMESPACE::NodeSerialization* serialization, const QString& fixedName)
{
    // If the serialization is not null, we are either pasting a node or loading it from a project.
    if (!fixedName.isEmpty()) {

        NodeCollectionPtr group = getGroup();
        std::string name;
        findAvailableName(fixedName.toStdString(), group, shared_from_this(), &name);

        //This version of setScriptName will not error if the name is invalid or already taken
        setScriptName_no_error_check(name);

        setLabel(name);


    } else if (serialization) {

        if (serialization->_nodeScriptName.empty()) {
            // The serialized script name may be empty in the case we are loading from a PyPlug/Preset directly
            initNodeNameFallbackOnPluginDefault();
        } else {
            NodeCollectionPtr group = getGroup();
            std::string name;
            bool nameAvailable = findAvailableName(serialization->_nodeScriptName, group, shared_from_this(), &name);

            // This version of setScriptName will not error if the name is invalid or already taken
            setScriptName_no_error_check(name);


            // If the script name was not available, give the same script name to the label, most likely this is a copy/paste operation.
            if (!nameAvailable) {
                setLabel(name);
            } else {
                setLabel(serialization->_nodeLabel);
            }
        }

    } else {
        initNodeNameFallbackOnPluginDefault();
    }



}

void
Node::createNodeGuiInternal(const CreateNodeArgsPtr& args)
{

    // The container group UI should have been created so far
    NodePtr thisShared = shared_from_this();
    NodeCollectionPtr group = getGroup();
    NodeGraphI* graph_i = group->getNodeGraph();
    if (graph_i) {
        graph_i->createNodeGui(thisShared, *args);

        // The gui pointer is set in the constructor of NodeGui
        if (!_imp->guiPointer.lock()) {
            throw std::runtime_error(tr("Could not create GUI for node %1").arg(QString::fromUtf8(getScriptName_mt_safe().c_str())).toStdString());
        }
    }
}

void
Node::load(const CreateNodeArgsPtr& args)
{
    // Called from the main thread. MT-safe
    assert( QThread::currentThread() == qApp->thread() );

    // Cannot load twice
    assert(!_imp->effect);

    // Should this node be persistent
    _imp->isPersistent = !args->getProperty<bool>(kCreateNodeArgsPropVolatile);

    // For Readers & Writers this is a hack to enable the internal decoder/encoder node to have a pointer to the main node the user sees
    _imp->ioContainer = args->getProperty<NodePtr>(kCreateNodeArgsPropMetaNodeContainer);

    NodeCollectionPtr group = getGroup();
    assert(group);
    if (!group) {
        throw std::logic_error("Node::load no container group!");
    }

    NodePtr thisShared = shared_from_this();

    // Add the node to the group before initializing anything else
    group->addNode(thisShared);

    // Should we report errors if load fails ?
    _imp->wasCreatedSilently = args->getProperty<bool>(kCreateNodeArgsPropSilent);

    // If this is a pyplug, load its properties
    std::string pyPlugID = args->getProperty<std::string>(kCreateNodeArgsPropPyPlugID);
    if (!pyPlugID.empty()) {
        _imp->pyPlugHandle = appPTR->getPluginBinary(QString::fromUtf8(pyPlugID.c_str()), -1, -1, false);
        _imp->isPyPlug = true;
    }


    // Any serialization from project load or copy/paste ?
    SERIALIZATION_NAMESPACE::NodeSerializationPtr serialization = args->getProperty<SERIALIZATION_NAMESPACE::NodeSerializationPtr >(kCreateNodeArgsPropNodeSerialization);

    // Should we load a preset ?
    std::string presetLabel = args->getProperty<std::string>(kCreateNodeArgsPropPreset);
    if (!presetLabel.empty()) {
        // If there's a preset specified, load serialization from preset

        // Figure out the plugin to use. We cannot use getPlugin() now because the effect is not yet created
        PluginPtr plugin = _imp->pyPlugHandle.lock();
        if (!plugin) {
            plugin = _imp->plugin.lock();
        }
        const std::vector<PluginPresetDescriptor>& presets = plugin->getPresetFiles();
        for (std::vector<PluginPresetDescriptor>::const_iterator it = presets.begin(); it!=presets.end(); ++it) {
            if (it->presetLabel.toStdString() == presetLabel) {

                // We found a matching preset
                _imp->initialNodePreset = presetLabel;
                break;
            }
        }
    } else if (serialization) {
        // The serialization had a preset
        _imp->initialNodePreset = serialization->_presetInstanceLabel;
    }




    std::string argFixedName = args->getProperty<std::string>(kCreateNodeArgsPropNodeInitialName);

    PluginPtr pluginPtr = _imp->plugin.lock();

     // Get the function pointer to create the plug-in instance
    EffectBuilder createFunc = (EffectBuilder)pluginPtr->getProperty<void*>(kNatronPluginPropCreateFunc);
    assert(createFunc);
    if (!createFunc) {
        throw std::invalid_argument("Node::load: No kNatronPluginPropCreateFunc property set on plug-in!");
    }
    _imp->effect = createFunc(thisShared);
    assert(_imp->effect);
    if (!_imp->effect) {
        throw std::runtime_error(tr("Could not create instance of %1").arg(QString::fromUtf8(getPluginID().c_str())).toStdString());
    }

    // Hack for Reader/Writer node
    NodePtr ioContainer = _imp->ioContainer.lock();
    if (ioContainer) {
        ReadNodePtr isReader = ioContainer->isEffectReadNode();
        if (isReader) {
            isReader->setEmbeddedReader(thisShared);
        } else {
            WriteNodePtr isWriter = ioContainer->isEffectWriteNode();
            assert(isWriter);
            if (isWriter) {
                isWriter->setEmbeddedWriter(thisShared);
            }
        }
    }

    bool argsNoNodeGui = args->getProperty<bool>(kCreateNodeArgsPropNoNodeGUI);


    // Make sure knobs initialization does not attempt to call knobChanged or trigger a render.
    _imp->effect->beginChanges();

    // For OpenFX this calls describe & describeInContext if neede dand then creates parameters and clips
    _imp->effect->initializeDataAfterCreate();

    // Set the node name
    initNodeScriptName(serialization.get(), QString::fromUtf8(argFixedName.c_str()));

    // Set plug-in accepted bitdepths and set default metadata
    refreshAcceptedBitDepths();

    // Load inputs
    initializeInputs();

    // Create knobs
    initializeKnobs(serialization.get() != 0, !argsNoNodeGui);

    // If this node is a group and we are in gui mode, create the node graph right now before creating any other
    // subnodes (in restoreNodeToDefaultState). This is so that the nodes get a proper position
    {
        NodeGroupPtr isGroupNode = toNodeGroup(_imp->effect);
        if (isGroupNode && isGroupNode->isSubGraphUserVisible() ) {
            getApp()->createGroupGui(thisShared, *args);
        }
    }

    // Restore the node to its default state including internal node graph and such for groups
    restoreNodeToDefaultState(args);

    // if we have initial values set for Knobs in the CreateNodeArgs object, deserialize them now
    setValuesFromSerialization(*args);

    // Setup default-metadata
    _imp->effect->setDefaultMetadata();

    // For OpenFX we create the image effect now
    _imp->effect->createInstanceAction();

    // For readers, set their original frame range when creating them
    if ( !serialization && ( _imp->effect->isReader() || _imp->effect->isWriter() ) ) {
        KnobIPtr filenameKnob = getKnobByName(kOfxImageEffectFileParamName);
        if (filenameKnob) {
            onFileNameParameterChanged(filenameKnob);
        }
    }

    // Check if there is any overlay
    _imp->effect->initializeOverlayInteract();

    // Refresh render scale support
    _imp->effect->refreshRenderScaleSupport();

    // Refresh dynamic props such as tiles support, OpenGL support, multi-thread etc...
    refreshDynamicProperties();

    // Ensure the OpenGL support knob has a consistant state according to the project
    onOpenGLEnabledKnobChangedOnProject(getApp()->getProject()->isOpenGLRenderActivated());

    // Get the sub-label knob
    restoreSublabel();

    // If this plug-in create views (ReadOIIO only) then refresh them
    refreshCreatedViews();

    // Notify the container group we added this node
    group->notifyNodeActivated(thisShared);

    // Create gui if needed. For groups this will also create the GUI of all internal nodes
    // unless they are not created yet
    if (!argsNoNodeGui && !getApp()->isBackground()) {
        createNodeGuiInternal(args);
    }

    // Callback to the effect notifying everything is setup.
    // Generally used by Group derivatives class to initialize internal nodes
    // unless there is a serialization that was loaded before
    _imp->effect->onEffectCreated(*args);

    // This node is now considered created
    _imp->nodeCreated = true;

    // Refresh page order so that serialization does not save it if it did not change
    _imp->refreshDefaultPagesOrder();

    // Refresh knobs Viewer UI order so that serialization does not save it if it did not change
    _imp->refreshDefaultViewerKnobsOrder();

    // Refresh data that depend on inputs
    if ( !getApp()->isCreatingNodeTree() ) {
        refreshAllInputRelatedData(!serialization);
    }

    // Run the Python callback
    _imp->runOnNodeCreatedCB(!serialization);

    // If needed compute a preview for this node
    computePreviewImage( getApp()->getTimeLine()->currentFrame() );

    // Resume knobChanged calls
    _imp->effect->endChanges();
} // load

std::string
Node::getCurrentNodePresets() const
{
    QMutexLocker k(&_imp->nodePresetMutex);
    return _imp->initialNodePreset;
}


void
Node::setWhileCreatingPaintStroke(bool creating)
{
    QMutexLocker k(&_imp->lastStrokeMovementMutex);

    _imp->duringPaintStrokeCreation = creating;
}

bool
Node::isDuringPaintStrokeCreation() const
{
    QMutexLocker k(&_imp->lastStrokeMovementMutex);

    return _imp->duringPaintStrokeCreation;
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
Node::setCurrentCanTransform(bool support)
{
    QMutexLocker k(&_imp->pluginsPropMutex);

    _imp->currentCanTransform = support;
}

bool
Node::getCurrentCanTransform() const
{
    QMutexLocker k(&_imp->pluginsPropMutex);

    return _imp->currentCanTransform;
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
    bool multiResSupported = _imp->effect->supportsMultiResolution();
    bool canTransform = _imp->effect->getCanTransform();
    _imp->pluginSafety = _imp->effect->getCurrentRenderThreadSafety();
    setRenderThreadSafety(_imp->pluginSafety);
    setCurrentSupportTiles(multiResSupported && tilesSupported);
    setCurrentSequentialRenderSupport( _imp->effect->getSequentialPreference() );
    setCurrentCanTransform(canTransform);
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

void
Node::prepareForNextPaintStrokeRender()
{
    {
        QMutexLocker k(&_imp->lastStrokeMovementMutex);
        _imp->strokeBitmapCleared = false;
    }
    _imp->effect->clearActionsCache();
}

void
Node::setLastPaintStrokeDataNoRotopaint()
{
    {
        QMutexLocker k(&_imp->lastStrokeMovementMutex);
        _imp->strokeBitmapCleared = false;
        _imp->duringPaintStrokeCreation = true;
    }
    _imp->effect->setDuringPaintStrokeCreationThreadLocal(true);
}

ImagePtr
Node::getPaintBuffer() const
{
    QMutexLocker k(&_imp->lastStrokeMovementMutex);
    return _imp->paintBuffer;
}

void
Node::setPaintBuffer(const ImagePtr& image)
{
    QMutexLocker k(&_imp->lastStrokeMovementMutex);
    _imp->paintBuffer = image;
}

void
Node::invalidateLastPaintStrokeDataNoRotopaint()
{
    {
        QMutexLocker k(&_imp->lastStrokeMovementMutex);
        _imp->duringPaintStrokeCreation = false;
    }
}

RectD
Node::getPaintStrokeRoD_duringPainting() const
{
    return getApp()->getPaintStrokeWholeBbox();
}

void
Node::getPaintStrokeRoD(double time,
                        ViewIdx view,
                        RectD* bbox) const
{
    bool duringPaintStroke = _imp->effect->isDuringPaintStrokeCreationThreadLocal();
    QMutexLocker k(&_imp->lastStrokeMovementMutex);

    if (duringPaintStroke) {
        *bbox = getPaintStrokeRoD_duringPainting();
    } else {
        RotoDrawableItemPtr item = _imp->paintStroke.lock();
        if (!item) {
            throw std::logic_error("");
        }
        *bbox = item->getBoundingBox(time, view);
    }
}

bool
Node::isLastPaintStrokeBitmapCleared() const
{
    QMutexLocker k(&_imp->lastStrokeMovementMutex);

    return _imp->strokeBitmapCleared;
}

void
Node::clearLastPaintStrokeRoD()
{
    QMutexLocker k(&_imp->lastStrokeMovementMutex);

    _imp->strokeBitmapCleared = true;
}

void
Node::getLastPaintStrokePoints(double time,
                               ViewIdx view,
                               unsigned int mipmapLevel,
                               std::list<std::list<std::pair<Point, double> > >* strokes,
                               int* strokeIndex) const
{
    bool duringPaintStroke;
    {
        QMutexLocker k(&_imp->lastStrokeMovementMutex);
        duringPaintStroke = _imp->duringPaintStrokeCreation;
    }

    if (duringPaintStroke) {
        getApp()->getLastPaintStrokePoints(strokes, strokeIndex);
        //adapt to mipmaplevel if needed
        if (mipmapLevel == 0) {
            return;
        }
        int pot = 1 << mipmapLevel;
        for (std::list<std::list<std::pair<Point, double> > >::iterator it = strokes->begin(); it != strokes->end(); ++it) {
            for (std::list<std::pair<Point, double> >::iterator it2 = it->begin(); it2 != it->end(); ++it2) {
                std::pair<Point, double> &p = *it2;
                p.first.x /= pot;
                p.first.y /= pot;
            }
        }
    } else {
        RotoDrawableItemPtr item = _imp->paintStroke.lock();
        RotoStrokeItemPtr stroke = toRotoStrokeItem(item);
        assert(stroke);
        if (!stroke) {
            throw std::logic_error("");
        }
        stroke->evaluateStroke(mipmapLevel, time, view, strokes);
        *strokeIndex = 0;
    }
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
        throw std::runtime_error("Plug-in does not support 8bits, 16bits or 32bits floating point image processing.");
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
Node::clearStreamWarning(StreamWarningEnum warning)
{
    assert( QThread::currentThread() == qApp->thread() );
    std::map<Node::StreamWarningEnum, QString>::iterator found = _imp->streamWarnings.find(warning);
    if ( ( found == _imp->streamWarnings.end() ) || found->second.isEmpty() ) {
        return;
    }
    found->second.clear();
    Q_EMIT streamWarningsChanged();
}

void
Node::getStreamWarnings(std::map<StreamWarningEnum, QString>* warnings) const
{
    assert( QThread::currentThread() == qApp->thread() );
    *warnings = _imp->streamWarnings;
}



void
Node::declareTablePythonFields()
{
    KnobItemsTablePtr table = _imp->effect->getItemsTable();
    if (!table) {
        return;
    }
    if (getScriptName_mt_safe().empty()) {
        return;
    }

    table->declareItemsToPython();
}

NodeCollectionPtr
Node::getGroup() const
{
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
    appPTR->removeAllCacheEntriesForPlugin(getPluginID());
}


void
Node::setValuesFromSerialization(const CreateNodeArgs& args)
{
    
    std::vector<std::string> params = args.getPropertyN<std::string>(kCreateNodeArgsPropNodeInitialParamValues);
    
    assert( QThread::currentThread() == qApp->thread() );
    const std::vector< KnobIPtr > & nodeKnobs = getKnobs();

    for (std::size_t i = 0; i < params.size(); ++i) {
        for (U32 j = 0; j < nodeKnobs.size(); ++j) {
            if (nodeKnobs[j]->getName() == params[i]) {
                
                KnobBoolBasePtr isBool = toKnobBoolBase(nodeKnobs[j]);
                KnobIntBasePtr isInt = toKnobIntBase(nodeKnobs[j]);
                KnobDoubleBasePtr isDbl = toKnobDoubleBase(nodeKnobs[j]);
                KnobStringBasePtr isStr = toKnobStringBase(nodeKnobs[j]);
                int nDims = nodeKnobs[j]->getNDimensions();

                std::string propName = kCreateNodeArgsPropParamValue;
                propName += "_";
                propName += params[i];
                if (isBool) {
                    std::vector<bool> v = args.getPropertyN<bool>(propName);
                    nDims = std::min((int)v.size(), nDims);
                    for (int d = 0; d < nDims; ++d) {
                        isBool->setValue(v[d]);
                    }
                } else if (isInt) {
                    std::vector<int> v = args.getPropertyN<int>(propName);
                    nDims = std::min((int)v.size(), nDims);
                    for (int d = 0; d < nDims; ++d) {
                        isInt->setValue(v[d]);
                    }
                } else if (isDbl) {
                    std::vector<double> v = args.getPropertyN<double>(propName);
                    nDims = std::min((int)v.size(), nDims);
                    for (int d = 0; d < nDims; ++d) {
                        isDbl->setValue(v[d]);
                    }
                } else if (isStr) {
                    std::vector<std::string> v = args.getPropertyN<std::string>(propName);
                    nDims = std::min((int)v.size(), nDims);
                    for (int d = 0; d < nDims; ++d) {
                        isStr->setValue(v[d]);
                    }
                }
                break;
            }
        }
    }
    
    
}



static KnobIPtr
findMasterKnob(const KnobIPtr & knob,
           const NodesList & allNodes,
           const std::string& masterKnobName,
           const std::string& masterNodeName,
           const std::string& masterItemName)
{
    ///we need to cycle through all the nodes of the project to find the real master
    NodePtr masterNode;

    if (masterNodeName == kKnobMasterNodeIsGroup) {
        EffectInstancePtr thisEffect = toEffectInstance(knob->getHolder());
        if (thisEffect) {
            NodeGroupPtr thisGroup = toNodeGroup(thisEffect->getNode()->getGroup());
            if (thisGroup) {
                masterNode = thisGroup->getNode();
            }
        }
        assert(masterNode);
    } else {

        for (NodesList::const_iterator it2 = allNodes.begin(); it2 != allNodes.end(); ++it2) {
            if ( (*it2)->getScriptName() == masterNodeName ) {
                masterNode = *it2;
                break;
            }
        }
    }
    if (!masterNode) {
        qDebug() << "Link slave/master for " << knob->getName().c_str() <<   " failed to restore the following linkage: " << masterNodeName.c_str();

        return KnobIPtr();
    }

    if ( !masterItemName.empty() ) {
        KnobItemsTablePtr table = masterNode->getEffectInstance()->getItemsTable();
        if (table) {
            KnobTableItemPtr item = table->getItemByFullyQualifiedScriptName(masterItemName);
            if (item) {
                return item->getKnobByName(masterKnobName);
            }
        }
    } else {
        ///now that we have the master node, find the corresponding knob
        const std::vector< KnobIPtr > & otherKnobs = masterNode->getKnobs();
        for (std::size_t j = 0; j < otherKnobs.size(); ++j) {
            if ( (otherKnobs[j]->getName() == masterKnobName) ) {
                return otherKnobs[j];
            }
        }
    }

    qDebug() << "Link slave/master for " << knob->getName().c_str() <<   " failed to restore the following linkage: " << masterNodeName.c_str();

    return KnobIPtr();
}


void
Node::restoreKnobLinks(const boost::shared_ptr<SERIALIZATION_NAMESPACE::KnobSerializationBase>& serialization,
                       const NodesList & allNodes)
{

    SERIALIZATION_NAMESPACE::KnobSerialization* isKnobSerialization = dynamic_cast<SERIALIZATION_NAMESPACE::KnobSerialization*>(serialization.get());
    SERIALIZATION_NAMESPACE::GroupKnobSerialization* isGroupKnobSerialization = dynamic_cast<SERIALIZATION_NAMESPACE::GroupKnobSerialization*>(serialization.get());

    if (isGroupKnobSerialization) {
        for (std::list <boost::shared_ptr<SERIALIZATION_NAMESPACE::KnobSerializationBase> >::const_iterator it = isGroupKnobSerialization->_children.begin(); it != isGroupKnobSerialization->_children.end(); ++it) {
            try {
                restoreKnobLinks(*it, allNodes);
            } catch (const std::exception& e) {
                LogEntry::LogEntryColor c;
                if (getColor(&c.r, &c.g, &c.b)) {
                    c.colorSet = true;
                }
                appPTR->writeToErrorLog_mt_safe(QString::fromUtf8(getScriptName_mt_safe().c_str() ), QDateTime::currentDateTime(), QString::fromUtf8(e.what()), false, c);


            }
        }
    } else if (isKnobSerialization) {
        KnobIPtr knob =  getKnobByName(isKnobSerialization->_scriptName);
        if (!knob) {
            throw std::invalid_argument(tr("Could not find a parameter named \"%1\"").arg( QString::fromUtf8( isKnobSerialization->_scriptName.c_str() ) ).toStdString());

            return;
        }


        // Restore slave/master links first
        {
            if (isKnobSerialization->_masterIsAlias) {
                if (!isKnobSerialization->_values.empty()) {
                    const SERIALIZATION_NAMESPACE::ValueSerialization value = isKnobSerialization->_values.begin()->second[0];
                    const std::string& aliasKnobName = value._slaveMasterLink.masterKnobName;
                    const std::string& aliasNodeName = value._slaveMasterLink.masterNodeName;
                    const std::string& masterTableItemName  = value._slaveMasterLink.masterTableItemName;
                    KnobIPtr alias = findMasterKnob(knob, allNodes, aliasKnobName, aliasNodeName, masterTableItemName);
                    if (alias) {
                        knob->setKnobAsAliasOfThis(alias, true);
                    }
                }
            } else {
                const std::vector<std::string>& projectViews = getApp()->getProject()->getProjectViewNames();
                for (SERIALIZATION_NAMESPACE::KnobSerialization::PerViewValueSerializationMap::const_iterator it = isKnobSerialization->_values.begin();
                     it != isKnobSerialization->_values.end(); ++it) {

                    // Find a matching view name
                    ViewIdx view_i(0);
                    Project::getViewIndex(projectViews, it->first, &view_i);

                    for (std::size_t i = 0; i < it->second.size(); ++i) {
                        if (!it->second[i]._slaveMasterLink.hasLink) {
                            continue;
                        }

                        std::string masterKnobName, masterNodeName, masterTableItemName;
                        if (it->second[i]._slaveMasterLink.masterNodeName.empty()) {
                            // Node name empty, assume this is the same node
                            masterNodeName = getScriptName_mt_safe();
                        }

                        if (it->second[i]._slaveMasterLink.masterKnobName.empty()) {
                            // Knob name empty, assume this is the same knob unless it has a single dimension
                            if (knob->getNDimensions() == 1) {
                                continue;
                            }
                            masterKnobName = knob->getName();
                        }

                        masterTableItemName = it->second[i]._slaveMasterLink.masterTableItemName;
                        KnobIPtr master = findMasterKnob(knob,
                                                         allNodes,
                                                         masterKnobName,
                                                         masterNodeName,
                                                         masterTableItemName);
                        if (master) {
                            // Find dimension in master by name
                            int otherDimIndex = -1;
                            if (master->getNDimensions() == 1) {
                                otherDimIndex = 0;
                            } else {
                                for (int d = 0; d < master->getNDimensions(); ++d) {
                                    if ( boost::iequals(master->getDimensionName(DimIdx(d)), it->second[i]._slaveMasterLink.masterDimensionName) ) {
                                        otherDimIndex = d;
                                        break;
                                    }
                                }
                                if (otherDimIndex == -1) {
                                    // Before Natron 2.2 we serialized the dimension index. Try converting to an int
                                    otherDimIndex = QString::fromUtf8(it->second[i]._slaveMasterLink.masterDimensionName.c_str()).toInt();
                                }
                            }
                            ViewIdx otherView(0);
                            Project::getViewIndex(projectViews, it->second[i]._slaveMasterLink.masterViewName, &otherView);

                            if (otherDimIndex >=0 && otherDimIndex < master->getNDimensions()) {
                                knob->slaveTo(master, DimIdx(it->second[i]._dimension), DimIdx(otherDimIndex), view_i, otherView);
                            } else {
                                throw std::invalid_argument(tr("Could not find a dimension named \"%1\" in \"%2\"").arg(QString::fromUtf8(it->second[i]._slaveMasterLink.masterDimensionName.c_str())).arg( QString::fromUtf8( it->second[i]._slaveMasterLink.masterKnobName.c_str() ) ).toStdString());
                            }
                        }

                    } // for each dimensions
                } // for each view
            } // isAlias
        }

        // Restore expressions
        {
            const std::vector<std::string>& projectViews = getApp()->getProject()->getProjectViewNames();
            for (SERIALIZATION_NAMESPACE::KnobSerialization::PerViewValueSerializationMap::const_iterator it = isKnobSerialization->_values.begin();
                 it != isKnobSerialization->_values.end(); ++it) {
                // Find a matching view name
                ViewIdx view_i(0);
                Project::getViewIndex(projectViews, it->first, &view_i);
                
                for (std::size_t i = 0; i < it->second.size(); ++i) {
                    try {
                        if ( !it->second[i]._expression.empty() ) {
                            knob->restoreExpression(DimIdx(it->second[i]._dimension), view_i,  it->second[i]._expression, it->second[i]._expresionHasReturnVariable);
                        }
                    } catch (const std::exception& e) {
                        QString err = QString::fromUtf8("Failed to restore expression: %1").arg( QString::fromUtf8( e.what() ) );
                        appPTR->writeToErrorLog_mt_safe(QString::fromUtf8( knob->getName().c_str() ), QDateTime::currentDateTime(), err);
                    }
                } // for all dimensions
            } // for all views
        }
    }
}


void
Node::restoreUserKnob(const KnobGroupPtr& group,
                      const KnobPagePtr& page,
                      const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase,
                      unsigned int recursionLevel)
{
    
    const SERIALIZATION_NAMESPACE::KnobSerialization* serialization = dynamic_cast<const SERIALIZATION_NAMESPACE::KnobSerialization*>(&serializationBase);
    const SERIALIZATION_NAMESPACE::GroupKnobSerialization* groupSerialization = dynamic_cast<const SERIALIZATION_NAMESPACE::GroupKnobSerialization*>(&serializationBase);
    assert(serialization || groupSerialization);
    if (!serialization && !groupSerialization) {
        return;
    }

    if (groupSerialization) {
        KnobIPtr found = getKnobByName(groupSerialization->_name);

        bool isPage = false;
        bool isGroup = false;
        if (groupSerialization->_typeName == KnobPage::typeNameStatic()) {
            isPage = true;
        } else if (groupSerialization->_typeName == KnobGroup::typeNameStatic()) {
            isGroup = true;
        } else {
            if (recursionLevel == 0) {
                // Recursion level is 0, so we are a page since pages all knobs must live in a page.
                // We use it because in the past we didn't serialize the typename so we could not know if this was
                // a page or a group.
                isPage = true;
            } else {
                isGroup = true;
            }
        }
        assert(isPage != isGroup);
        if (isPage) {
            KnobPagePtr page;
            if (!found) {
                page = AppManager::createKnob<KnobPage>(_imp->effect, groupSerialization->_label, 1, false);
                page->setAsUserKnob(true);
                page->setName(groupSerialization->_name);
            } else {
                page = toKnobPage(found);
            }
            if (!page) {
                return;
            }
            if (!found && _imp->isLoadingPreset) {
                _imp->presetKnobs.push_back(page);
            }
            for (std::list<boost::shared_ptr<SERIALIZATION_NAMESPACE::KnobSerializationBase> >::const_iterator it = groupSerialization->_children.begin(); it != groupSerialization->_children.end(); ++it) {
                restoreUserKnob(KnobGroupPtr(), page, **it, recursionLevel + 1);
            }

        } else if (isGroup) { //!ispage
            KnobGroupPtr grp;
            if (!found) {
                grp = AppManager::createKnob<KnobGroup>(_imp->effect, groupSerialization->_label, 1, false);
            } else {
                grp = toKnobGroup(found);

            }
            if (!grp) {
                return;
            }
            grp->setAsUserKnob(true);
            grp->setName(groupSerialization->_name);
            if (groupSerialization->_isSetAsTab) {
                grp->setAsTab();
            }
            assert(page);
            if (page) {
                page->addKnob(grp);
            }
            if (group) {
                group->addKnob(grp);
            }
            if (!found && _imp->isLoadingPreset) {
                _imp->presetKnobs.push_back(group);
            }
            grp->setValue(groupSerialization->_isOpened);
            for (std::list<boost::shared_ptr<SERIALIZATION_NAMESPACE::KnobSerializationBase> >::const_iterator it = groupSerialization->_children.begin(); it != groupSerialization->_children.end(); ++it) {
                restoreUserKnob(grp, page, **it, recursionLevel + 1);
            }
        } // ispage

    } else {



        assert(serialization->_isUserKnob);
        if (!serialization->_isUserKnob) {
            return;
        }


        bool isFile = serialization->_typeName == KnobFile::typeNameStatic();
        bool isPath = serialization->_typeName == KnobPath::typeNameStatic();
        bool isString = serialization->_typeName == KnobString::typeNameStatic();
        bool isParametric = serialization->_typeName == KnobParametric::typeNameStatic();
        bool isChoice = serialization->_typeName == KnobChoice::typeNameStatic();
        bool isDouble = serialization->_typeName == KnobDouble::typeNameStatic();
        bool isColor = serialization->_typeName == KnobColor::typeNameStatic();
        bool isInt = serialization->_typeName == KnobInt::typeNameStatic();
        bool isBool = serialization->_typeName == KnobBool::typeNameStatic();
        bool isSeparator = serialization->_typeName == KnobSeparator::typeNameStatic();
        bool isButton = serialization->_typeName == KnobButton::typeNameStatic();

        assert(isInt || isDouble || isBool || isChoice || isColor || isString || isFile || isPath || isButton || isSeparator || isParametric);

        KnobIPtr knob;
        KnobIPtr found = getKnobByName(serialization->_scriptName);
        if (found) {
            knob = found;
        } else {
            if (isInt) {
                knob = AppManager::createKnob<KnobInt>(_imp->effect, serialization->_label, serialization->_dimension, false);
            } else if (isDouble) {
                knob = AppManager::createKnob<KnobDouble>(_imp->effect, serialization->_label, serialization->_dimension, false);
            } else if (isBool) {
                knob = AppManager::createKnob<KnobBool>(_imp->effect, serialization->_label, serialization->_dimension, false);
            } else if (isChoice) {
                knob = AppManager::createKnob<KnobChoice>(_imp->effect, serialization->_label, serialization->_dimension, false);
            } else if (isColor) {
                knob = AppManager::createKnob<KnobColor>(_imp->effect, serialization->_label, serialization->_dimension, false);
            } else if (isString) {
                knob = AppManager::createKnob<KnobString>(_imp->effect, serialization->_label, serialization->_dimension, false);
            } else if (isFile) {
                knob = AppManager::createKnob<KnobFile>(_imp->effect, serialization->_label, serialization->_dimension, false);
            } else if (isPath) {
                knob = AppManager::createKnob<KnobPath>(_imp->effect, serialization->_label, serialization->_dimension, false);
            } else if (isButton) {
                knob = AppManager::createKnob<KnobButton>(_imp->effect, serialization->_label, serialization->_dimension, false);
            } else if (isSeparator) {
                knob = AppManager::createKnob<KnobSeparator>(_imp->effect, serialization->_label, serialization->_dimension, false);
            } else if (isParametric) {
                knob = AppManager::createKnob<KnobParametric>(_imp->effect, serialization->_label, serialization->_dimension, false);
            }
            if (!found && _imp->isLoadingPreset) {
                _imp->presetKnobs.push_back(knob);
            }
        } // found


        assert(knob);
        if (!knob) {
            return;
        }

        knob->fromSerialization(*serialization);

        if (group) {
            group->addKnob(knob);
        } else if (page) {
            page->addKnob(knob);
        }
    } // groupSerialization

} // restoreUserKnob

std::string
Node::getContainerGroupFullyQualifiedName() const
{
    NodeCollectionPtr collection = getGroup();
    NodeGroupPtr containerIsGroup = toNodeGroup(collection);
    if (containerIsGroup) {
        return  containerIsGroup->getNode()->getFullyQualifiedName();
    }
    return std::string();
}

void
Node::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase)
{

    SERIALIZATION_NAMESPACE::NodeSerialization* serialization = dynamic_cast<SERIALIZATION_NAMESPACE::NodeSerialization*>(serializationBase);
    assert(serialization);
    if (!serialization) {
        return;
    }

    // All this code is MT-safe as it runs in the serialization thread

    OfxEffectInstancePtr isOfxEffect = boost::dynamic_pointer_cast<OfxEffectInstance>(getEffectInstance());

    if (isOfxEffect) {
        // For OpenFX nodes, we call the sync private data action now to let a chance to the plug-in to synchronize it's
        // private data to parameters that will be saved with the project.
        isOfxEffect->syncPrivateData_other_thread();
    }


    // Check if pages ordering changed, if not do not serialize
    bool pageOrderChanged = true;
    if (serialization->_encodeType == SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationTypeRegular) {
        pageOrderChanged = hasPageOrderChangedSinceDefault();
    }

    bool isFullSaveMode = appPTR->getCurrentSettings()->getIsFullRecoverySaveModeEnabled();

    bool subGraphEdited = isSubGraphEditedByUser();

    KnobPagePtr pyPlugPage = _imp->pyPlugPage.lock();

    KnobsVec knobs = getEffectInstance()->getKnobs_mt_safe();
    std::list<KnobIPtr > userPages;
    for (std::size_t i  = 0; i < knobs.size(); ++i) {
        KnobGroupPtr isGroup = toKnobGroup(knobs[i]);
        KnobPagePtr isPage = toKnobPage(knobs[i]);

        // For pages, check if it is a user knob, if so serialialize user knobs recursively
        if (isPage) {
            // Don t save empty pages
            if (isPage->getChildren().empty()) {
                continue;
            }
            if (!isPage->getIsSecret() && (pageOrderChanged || subGraphEdited)) {
                serialization->_pagesIndexes.push_back( knobs[i]->getName() );
            }
            if ( knobs[i]->isUserKnob() && !knobs[i]->isDeclaredByPlugin() ) {
                userPages.push_back(knobs[i]);
            }
            continue;
        }

        bool hasExpr = false;
        {
            std::list<ViewIdx> views = knobs[i]->getViewsList();
            for (int d = 0; d < knobs[i]->getNDimensions(); ++d) {
                for (std::list<ViewIdx>::const_iterator itV = views.begin(); itV != views.end(); ++itV) {
                    if (!knobs[i]->getExpression(DimIdx(d), *itV).empty()) {
                        hasExpr = true;
                        break;
                    }
                    MasterKnobLink linkData;
                    if (knobs[i]->getMaster(DimIdx(d), *itV, &linkData)) {
                        hasExpr = true;
                        break;
                    }
                }
                if (hasExpr) {
                    break;
                }
            }
        }
        if (!knobs[i]->getIsPersistent() && !hasExpr) {
            // Don't serialize non persistant knobs
            continue;
        }

        if (knobs[i]->isUserKnob() && !knobs[i]->isDeclaredByPlugin()) {
            // Don't serialize user knobs, its taken care of by user pages
            continue;
        }

        if (isGroup || isPage) {
            // Don't serialize these, they don't hold anything
            continue;
        }


        if (!isFullSaveMode && !knobs[i]->hasModifications()) {
            // This knob was not modified by the user, don't serialize it
            continue;
        }

        // If the knob is in the PyPlug page, only serialize if the PyPlug page is visible
        if (pyPlugPage && pyPlugPage->getIsSecret() && knobs[i]->getTopLevelPage() == pyPlugPage) {
            continue;
        }

        SERIALIZATION_NAMESPACE::KnobSerializationPtr newKnobSer( new SERIALIZATION_NAMESPACE::KnobSerialization );
        knobs[i]->toSerialization(newKnobSer.get());
        if (newKnobSer->_mustSerialize) {
            serialization->_knobsValues.push_back(newKnobSer);
        }

    }

    // Serialize user pages now
    for (std::list<KnobIPtr>::const_iterator it = userPages.begin(); it != userPages.end(); ++it) {
        boost::shared_ptr<SERIALIZATION_NAMESPACE::GroupKnobSerialization> s( new SERIALIZATION_NAMESPACE::GroupKnobSerialization );
        (*it)->toSerialization(s.get());
        serialization->_userPages.push_back(s);
    }


    serialization->_groupFullyQualifiedScriptName = getContainerGroupFullyQualifiedName();

    serialization->_nodeLabel = getLabel_mt_safe();

    serialization->_nodeScriptName = getScriptName_mt_safe();

    serialization->_pluginID = getPluginID();


    {
        QMutexLocker k(&_imp->nodePresetMutex);
        serialization->_presetInstanceLabel = _imp->initialNodePreset;
    }

    serialization->_pluginMajorVersion = getMajorVersion();

    serialization->_pluginMinorVersion = getMinorVersion();

    // Only serialize inputs for regular serialization
    if (serialization->_encodeType == SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationTypeRegular) {
        getInputNames(serialization->_inputs);
    }


    NodePtr masterNode = getMasterNode();
    if (masterNode) {
        serialization->_masterNodecriptName = masterNode->getScriptName_mt_safe();
    }

    KnobItemsTablePtr table = _imp->effect->getItemsTable();
    if (table && table->getNumTopLevelItems() > 0) {
        serialization->_tableModel.reset(new SERIALIZATION_NAMESPACE::KnobItemsTableSerialization);
        table->toSerialization(serialization->_tableModel.get());
    }

    // For groups, serialize its children if the graph was edited
    NodeGroupPtr isGrp = isEffectNodeGroup();
    if (isGrp && subGraphEdited) {
        NodesList nodes;
        isGrp->getActiveNodes(&nodes);

        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if ( (*it)->isPersistent() ) {
                
                SERIALIZATION_NAMESPACE::NodeSerializationPtr state;
                StubNodePtr isStub = toStubNode((*it)->getEffectInstance());
                if (isStub) {
                    state = isStub->getNodeSerialization();
                    if (!state) {
                        continue;
                    }
                } else {
                    state.reset( new SERIALIZATION_NAMESPACE::NodeSerialization );
                    (*it)->toSerialization(state.get());
                }
                
                serialization->_children.push_back(state);
            }
        }
    }

    // User created components
    std::list<ImageComponents> userComps;
    getUserCreatedComponents(&userComps);
    for (std::list<ImageComponents>::iterator it = userComps.begin(); it!=userComps.end(); ++it) {
        SERIALIZATION_NAMESPACE::ImageComponentsSerialization s;
        s.layerName = it->getLayerName();
        s.globalCompsName = it->getComponentsGlobalName();
        s.channelNames = it->getComponentsNames();
        serialization->_userComponents.push_back(s);
    }

    getPosition(&serialization->_nodePositionCoords[0], &serialization->_nodePositionCoords[1]);

    // Only save the size for backdrops, that's the only node where the user can resize
    if (isEffectBackdrop()) {
        getSize(&serialization->_nodeSize[0], &serialization->_nodeSize[1]);
    }

    if (hasColorChangedSinceDefault()) {
        getColor(&serialization->_nodeColor[0], &serialization->_nodeColor[1], &serialization->_nodeColor[2]);
    }
    getOverlayColor(&serialization->_overlayColor[0], &serialization->_overlayColor[1], &serialization->_overlayColor[2]);

    // Only serialize viewer UI knobs order if it has changed

    bool serializeViewerKnobs = serialization->_encodeType != SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationTypeRegular;
    KnobsVec viewerUIKnobs = getEffectInstance()->getViewerUIKnobs();
    if (!serializeViewerKnobs) {
        if (viewerUIKnobs.size() != _imp->defaultViewerKnobsOrder.size()) {
            std::list<std::string>::const_iterator it2 = _imp->defaultViewerKnobsOrder.begin();
            bool hasChanged = false;
            for (KnobsVec::iterator it = viewerUIKnobs.begin(); it!=viewerUIKnobs.end(); ++it, ++it2) {
                if ((*it)->getName() != *it2) {
                    hasChanged = true;
                    break;
                }
            }
            serializeViewerKnobs |= hasChanged;
        }
    }
    if (serializeViewerKnobs) {
        for (KnobsVec::iterator it = viewerUIKnobs.begin(); it!=viewerUIKnobs.end(); ++it) {
            serialization->_viewerUIKnobsOrder.push_back((*it)->getName());
        }
    }

} // Node::toSerialization

void
Node::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase)
{
    const SERIALIZATION_NAMESPACE::NodeSerialization* serialization = dynamic_cast<const SERIALIZATION_NAMESPACE::NodeSerialization*>(&serializationBase);
    assert(serialization);
    if (!serialization) {
        return;
    }

    // Load all knobs as well as user knobs and roto/tracking data
    loadKnobsFromSerialization(*serialization);

    // Remember the UI
    {
        QMutexLocker k(&_imp->nodeUIDataMutex);
        _imp->nodePositionCoords[0] = serialization->_nodePositionCoords[0];
        _imp->nodePositionCoords[1] = serialization->_nodePositionCoords[1];
        _imp->nodeSize[0] = serialization->_nodeSize[0];
        _imp->nodeSize[1] = serialization->_nodeSize[1];
        _imp->nodeColor[0] = serialization->_nodeColor[0];
        _imp->nodeColor[1] = serialization->_nodeColor[1];
        _imp->nodeColor[2] = serialization->_nodeColor[2];
        _imp->overlayColor[0] = serialization->_overlayColor[0];
        _imp->overlayColor[1] = serialization->_overlayColor[1];
        _imp->overlayColor[2] = serialization->_overlayColor[2];
    }

} // fromSerialization

void
Node::loadInternalNodeGraph(bool initialSetupAllowed,
                            const SERIALIZATION_NAMESPACE::NodeSerialization* projectSerialization,
                            const SERIALIZATION_NAMESPACE::NodeSerialization* pyPlugSerialization)
{
    NodeGroupPtr isGrp = isEffectNodeGroup();
    if (!isGrp) {
        return;
    }

    // Only do this when loading the node the first time
    assert(!isNodeCreated());

    {
        PluginPtr pyPlug = _imp->pyPlugHandle.lock();
        // For old PyPlugs based on Python scripts, the nodes are created by the Python script after the Group itself
        // gets created. So don't do anything
        bool isPythonScriptPyPlug = pyPlug && pyPlug->getProperty<bool>(kNatronPluginPropPyPlugIsPythonScript);
        if (isPythonScriptPyPlug) {
            return;
        }
    }

    // PyPlug serialization is only for pyplugs
    assert(!_imp->isPyPlug || pyPlugSerialization);


    // If we are creating the node in the standard way or loading a project and the internal node graph was not edited, initialize the sub-graph.
    // For a standard Group it creates the Input and Output nodes.
    if ((!projectSerialization || projectSerialization->_children.empty()) && !_imp->isPyPlug && initialSetupAllowed) {
        isGrp->setupInitialSubGraphState();
    }


    // Call the nodegroup derivative that is the only one to know what to do
    isGrp->loadSubGraph(projectSerialization, pyPlugSerialization);

}


void
Node::loadKnobsFromSerialization(const SERIALIZATION_NAMESPACE::NodeSerialization& serialization)
{

    _imp->effect->beginChanges();
    _imp->effect->onKnobsAboutToBeLoaded(serialization);

    {
        QMutexLocker k(&_imp->createdComponentsMutex);
        for (std::list<SERIALIZATION_NAMESPACE::ImageComponentsSerialization>::const_iterator it = serialization._userComponents.begin(); it!=serialization._userComponents.end(); ++it) {
            ImageComponents s(it->layerName, it->globalCompsName, it->channelNames);
            _imp->createdComponents.push_back(s);
        }
    }
  
    {
        // Load all knobs

        for (SERIALIZATION_NAMESPACE::KnobSerializationList::const_iterator it = serialization._knobsValues.begin(); it!=serialization._knobsValues.end(); ++it) {
            KnobIPtr knob = getKnobByName((*it)->_scriptName);
            if (!knob) {
                continue;
            }
            knob->fromSerialization(**it);

        }


    }

    KnobIPtr filenameParam = getKnobByName(kOfxImageEffectFileParamName);
    if (filenameParam) {
        computeFrameRangeForReader(filenameParam, false);
    }

    // now restore the roto context if the node has a roto context
    KnobItemsTablePtr table = _imp->effect->getItemsTable();
    if (serialization._tableModel && table) {
        table->resetModel(eTableChangeReasonInternal);
        table->fromSerialization(*serialization._tableModel);
        table->declareItemsToPython();
    }

    {
        for (std::list<boost::shared_ptr<SERIALIZATION_NAMESPACE::GroupKnobSerialization> >::const_iterator it = serialization._userPages.begin(); it != serialization._userPages.end(); ++it) {
            restoreUserKnob(KnobGroupPtr(), KnobPagePtr(), **it, 0);
        }
    }

    declarePythonKnobs();

    if (!serialization._pagesIndexes.empty()) {
        setPagesOrder( serialization._pagesIndexes );
    }

    if (!serialization._viewerUIKnobsOrder.empty()) {
        KnobsVec viewerUIknobs;
        for (std::list<std::string>::const_iterator it = serialization._viewerUIKnobsOrder.begin(); it!=serialization._viewerUIKnobsOrder.end(); ++it) {
            KnobIPtr knob = getKnobByName(*it);
            if (knob) {
                viewerUIknobs.push_back(knob);
            }
        }
        _imp->effect->setViewerUIKnobs(viewerUIknobs);
    }

    // Force update of user knobs on the GUI if we are calling this in restoreNodeDefaults
    _imp->effect->recreateUserKnobs(false);

    _imp->effect->onKnobsLoaded();
    _imp->effect->endChanges();

} // loadKnobsFromSerialization

void
Node::clearPresetFlag()
{
    bool isEmpty;
    {
        QMutexLocker k(&_imp->nodePresetMutex);
        isEmpty = _imp->initialNodePreset.empty();
        _imp->initialNodePreset.clear();
    }
    if (!isEmpty) {
        Q_EMIT nodePresetsChanged();
    }
}

void
Node::loadPresets(const std::string& presetsLabel)
{

    assert(QThread::currentThread() == qApp->thread());
    {
        QMutexLocker k(&_imp->nodePresetMutex);
        _imp->initialNodePreset = presetsLabel;
    }
    restoreNodeToDefaultState(CreateNodeArgsPtr());
    Q_EMIT nodePresetsChanged();
}

void
Node::loadPresetsFromFile(const std::string& presetsFile)
{

    assert(QThread::currentThread() == qApp->thread());

    SERIALIZATION_NAMESPACE::NodeSerializationPtr serialization(new SERIALIZATION_NAMESPACE::NodeSerialization);

    // Throws on failure
    getNodeSerializationFromPresetFile(presetsFile, serialization.get());

    {
        QMutexLocker k(&_imp->nodePresetMutex);
        _imp->initialNodePreset = serialization->_presetInstanceLabel;
    }
    restoreNodeToDefaultState(CreateNodeArgsPtr());
    Q_EMIT nodePresetsChanged();
}

void
Node::getNodeSerializationFromPresetFile(const std::string& presetFile, SERIALIZATION_NAMESPACE::NodeSerialization* serialization)
{
    assert(serialization);
    FStreamsSupport::ifstream ifile;
    FStreamsSupport::open(&ifile, presetFile);
    if (!ifile || presetFile.empty()) {
        std::string message = tr("Failed to open file: ").toStdString() + presetFile;
        throw std::runtime_error(message);
    }

    SERIALIZATION_NAMESPACE::read(ifile,serialization);
}



void
Node::getNodeSerializationFromPresetName(const std::string& presetName, SERIALIZATION_NAMESPACE::NodeSerialization* serialization)
{
    PluginPtr plugin = getPlugin();
    if (!plugin) {
        throw std::invalid_argument("Invalid plug-in");
    }

    const std::vector<PluginPresetDescriptor>& presets = plugin->getPresetFiles();
    for (std::vector<PluginPresetDescriptor>::const_iterator it = presets.begin() ;it!=presets.end(); ++it) {
        if (it->presetLabel.toStdString() == presetName) {
            getNodeSerializationFromPresetFile(it->presetFilePath.toStdString(), serialization);
            assert(presetName == serialization->_presetsIdentifierLabel);
            return;
        }
    }


    std::string message = tr("Cannot find loaded preset named %1").arg(QString::fromUtf8(presetName.c_str())).toStdString();
    throw std::invalid_argument(message);
}

void
Node::loadPresetsInternal(const SERIALIZATION_NAMESPACE::NodeSerializationPtr& serialization,bool setKnobsDefault)
{
    assert(QThread::currentThread() == qApp->thread());


    loadKnobsFromSerialization(*serialization);

    if (setKnobsDefault) {
        // set non animated knobs to be their default values
        const KnobsVec& knobs = getKnobs();
        for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
            KnobButtonPtr isBtn = toKnobButton(*it);
            KnobPagePtr isPage = toKnobPage(*it);
            KnobSeparatorPtr isSeparator = toKnobSeparator(*it);
            if ( (isBtn && !isBtn->getIsCheckable())  || isPage || isSeparator) {
                continue;
            }

            if ((*it)->getIsPersistent()) {
                KnobIntBasePtr isInt = toKnobIntBase(*it);
                KnobBoolBasePtr isBool = toKnobBoolBase(*it);
                KnobStringBasePtr isString = toKnobStringBase(*it);
                KnobDoubleBasePtr isDouble = toKnobDoubleBase(*it);
                if ((*it)->hasAnimation()) {
                    continue;
                }
                for (int d = 0; d < (*it)->getNDimensions(); ++d) {
                    if (isInt) {
                        isInt->setDefaultValue(isInt->getValue(DimIdx(d)), DimIdx(d));
                    } else if (isBool) {
                        isBool->setDefaultValue(isBool->getValue(DimIdx(d)), DimIdx(d));
                    } else if (isString) {
                        isString->setDefaultValue(isString->getValue(DimIdx(d)), DimIdx(d));
                    } else if (isDouble) {
                        isDouble->setDefaultValue(isDouble->getValue(DimIdx(d)), DimIdx(d));
                    }
                }
            }
        }
    }

} // Node::loadPresetsInternal

void
Node::exportNodeToPyPlug(const std::string& filePath)
{
    // Only groups can export to PyPlug
    if (!isEffectNodeGroup()) {
        return;
    }
    FStreamsSupport::ofstream ofile;
    FStreamsSupport::open(&ofile, filePath);
    if (!ofile || filePath.empty()) {
        std::string message = tr("Failed to open file: ").toStdString() + filePath;
        throw std::runtime_error(message);
    }

    // Perform checks before writing the file
    {
        std::string pyPlugID = _imp->pyPlugIDKnob.lock()->getValue();
        if (pyPlugID.empty()) {
            std::string message = tr("The plug-in ID cannot be empty").toStdString();
            throw std::runtime_error(message);
        }
    }
    {
        std::string pyPlugLabel = _imp->pyPlugLabelKnob.lock()->getValue();
        if (pyPlugLabel.empty()) {
            std::string message = tr("The plug-in label cannot be empty").toStdString();
            throw std::runtime_error(message);
        }
    }

    // Make sure the file paths are relative to the pyplug script directory
    std::string pyPlugDirectoryPath;
    {
        std::size_t foundSlash = filePath.find_last_of('/');
        if (foundSlash != std::string::npos) {
            pyPlugDirectoryPath = filePath.substr(0, foundSlash + 1);
        }
    }

    {
        std::string iconFilePath = _imp->pyPlugIconKnob.lock()->getValue();
        std::string path;
        std::size_t foundSlash = iconFilePath.find_last_of('/');
        if (foundSlash != std::string::npos) {
            path = iconFilePath.substr(0, foundSlash + 1);
        }
        if (!path.empty() && path != pyPlugDirectoryPath) {
            std::string message = tr("The plug-in icon file should be located in the same directory as the PyPlug script (%1)").arg(QString::fromUtf8(pyPlugDirectoryPath.c_str())).toStdString();
            throw std::runtime_error(message);
        }
    }
    {
        std::string callbacksFilePath = _imp->pyPlugExtPythonScript.lock()->getValue();
        std::string path;
        std::size_t foundSlash = callbacksFilePath.find_last_of('/');
        if (foundSlash != std::string::npos) {
            path = callbacksFilePath.substr(0, foundSlash + 1);
        }
        if (!path.empty() && path != pyPlugDirectoryPath) {
            std::string message = tr("The Python callbacks file should be located in the same directory as the PyPlug script (%1)").arg(QString::fromUtf8(pyPlugDirectoryPath.c_str())).toStdString();
            throw std::runtime_error(message);
        }
    }


    // Check that the directory where the file will be is registered in Natron search paths.
    if (!getApp()->isBackground()) {
        bool foundInPath = false;
        QStringList groupSearchPath = appPTR->getAllNonOFXPluginsPaths();
        for (QStringList::iterator it = groupSearchPath.begin(); it != groupSearchPath.end(); ++it) {
            std::string thisPath = it->toStdString();

            // pyPlugDirectoryPath ends with a separator, so ensure this one has one too
            if (!thisPath.empty() && thisPath[thisPath.size() - 1] != '/') {
                thisPath.push_back('/');
            }
            if (thisPath == pyPlugDirectoryPath) {
                foundInPath = true;
                break;
            }
        }

        if (!foundInPath) {
            QString message = tr("Directory \"%1\" is not in the plug-in search path, would you like to add it?").arg(QString::fromUtf8(pyPlugDirectoryPath.c_str()));
            StandardButtonEnum rep = Dialogs::questionDialog(tr("Plug-in path").toStdString(),
                                                             message.toStdString(), false);

            if  (rep == eStandardButtonYes) {
                appPTR->getCurrentSettings()->appendPythonGroupsPath(pyPlugDirectoryPath);
            }
        }
    }


    SERIALIZATION_NAMESPACE::NodeSerializationPtr serialization(new SERIALIZATION_NAMESPACE::NodeSerialization);
    serialization->_encodeType = SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationTypePyPlug;
    toSerialization(serialization.get());

    SERIALIZATION_NAMESPACE::NodeClipBoard cb;
    cb.nodes.push_back(serialization);
    
    SERIALIZATION_NAMESPACE::write(ofile, cb);
}

void
Node::exportNodeToPresets(const std::string& filePath,
                          const std::string& presetsLabel,
                          const std::string& iconFilePath,
                          int shortcutSymbol,
                          int shortcutModifiers)
{
    FStreamsSupport::ofstream ofile;
    FStreamsSupport::open(&ofile, filePath);
    if (!ofile || filePath.empty()) {
        std::string message = tr("Failed to open file: ").toStdString() + filePath;
        throw std::runtime_error(message);
    }

    // Make sure the file paths are relative to the presets script directory
    std::string pyPlugDirectoryPath;
    {
        std::size_t foundSlash = filePath.find_last_of('/');
        if (foundSlash != std::string::npos) {
            pyPlugDirectoryPath = filePath.substr(0, foundSlash + 1);
        }
    }
    {
        std::string path;
        std::size_t foundSlash = iconFilePath.find_last_of('/');
        if (foundSlash != std::string::npos) {
            path = iconFilePath.substr(0, foundSlash + 1);
        }
        if (!path.empty() && path != pyPlugDirectoryPath) {
            std::string message = tr("The preset icon file should be located in the same directory as the preset script (%1)").arg(QString::fromUtf8(pyPlugDirectoryPath.c_str())).toStdString();
            throw std::runtime_error(message);
        }
    }


    SERIALIZATION_NAMESPACE::NodeSerializationPtr serialization(new SERIALIZATION_NAMESPACE::NodeSerialization);
    serialization->_encodeType = SERIALIZATION_NAMESPACE::NodeSerialization::eNodeSerializationTypePresets;
    serialization->_presetsIdentifierLabel = presetsLabel;
    serialization->_presetsIconFilePath = iconFilePath;
    serialization->_presetShortcutSymbol = shortcutSymbol;
    serialization->_presetShortcutPresetModifiers = shortcutModifiers;

    toSerialization(serialization.get());

    SERIALIZATION_NAMESPACE::NodeClipBoard cb;
    cb.nodes.push_back(serialization);

    SERIALIZATION_NAMESPACE::write(ofile, cb);
}



void
Node::restoreNodeToDefaultState(const CreateNodeArgsPtr& args)
{
    assert(QThread::currentThread() == qApp->thread());

    FlagSetter setter(true, &_imp->restoringDefaults);

    // Make sure the instance does not receive knobChanged now
    _imp->effect->beginChanges();

    // If the node is not yet created (i.e: this is called in the load() function) then some stuff here doesn't need to be done
    bool nodeCreated = isNodeCreated();
    if (nodeCreated) {
        // Purge any cache when reseting to defaults
        _imp->effect->purgeCaches();
    }

    // Check if there is any serialization from presets/pyplug
    std::string nodePreset = getCurrentNodePresets();
    SERIALIZATION_NAMESPACE::NodeSerializationPtr presetSerialization, pyPlugSerialization, projectSerialization;
    if (args) {
        projectSerialization = args->getProperty<SERIALIZATION_NAMESPACE::NodeSerializationPtr >(kCreateNodeArgsPropNodeSerialization);
    }
    if (!nodePreset.empty()) {
        try {
            presetSerialization.reset(new SERIALIZATION_NAMESPACE::NodeSerialization);
            getNodeSerializationFromPresetName(nodePreset, presetSerialization.get());
        } catch (...) {

        }
    }

    if (_imp->isPyPlug) {
        PluginPtr pyPlugHandle = _imp->pyPlugHandle.lock();
        if (pyPlugHandle) {
            bool isPythonScriptPyPlug = pyPlugHandle->getProperty<bool>(kNatronPluginPropPyPlugIsPythonScript);
            if (!isPythonScriptPyPlug) {
                std::string filePath = pyPlugHandle->getProperty<std::string>(kNatronPluginPropPyPlugScriptAbsoluteFilePath);
                pyPlugSerialization.reset(new SERIALIZATION_NAMESPACE::NodeSerialization);
                getNodeSerializationFromPresetFile(filePath, pyPlugSerialization.get());
            }
        }
    }
    // Reset all knobs to default first, block value changes and do them all afterwards because the node state can only be restored
    // if all parameters are actually to the good value
    if (nodeCreated) {


        // Restore knob defaults
        const KnobsVec& knobs = getKnobs();
        for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
            if (!(*it)->getIsPersistent() ) {
                continue;
            }
            (*it)->blockValueChanges();
            (*it)->unSplitAllViews();
            (*it)->resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());
            (*it)->unblockValueChanges();
        }
    }


    // If this is a pyplug, load the node state (and its internal subgraph)
    if (pyPlugSerialization) {
        loadPresetsInternal(pyPlugSerialization, false);
    }

    if (presetSerialization) {

        // If any preset knob already exist, remove it
        {
            for (std::list<KnobIWPtr>::iterator it = _imp->presetKnobs.begin(); it != _imp->presetKnobs.end(); ++it) {
                KnobIPtr knob = it->lock();
                if (!knob) {
                    continue;
                }
                _imp->effect->deleteKnob(knob, true);
            }
            _imp->presetKnobs.clear();
        }

        _imp->isLoadingPreset = true;
        // Load presets from serialization if any
        loadPresetsInternal(presetSerialization, true);
        _imp->isLoadingPreset = false;
    } else {

        // Reset knob default values to their initial default value if we had a different preset before
        if (nodeCreated) {
            const KnobsVec& knobs = getKnobs();
            for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
                KnobButtonPtr isBtn = toKnobButton(*it);
                KnobPagePtr isPage = toKnobPage(*it);
                KnobSeparatorPtr isSeparator = toKnobSeparator(*it);
                if ( (isBtn && !isBtn->getIsCheckable())  || isPage || isSeparator) {
                    continue;
                }
                if ((*it)->hasAnimation()) {
                    continue;
                }

                if ((*it)->getIsPersistent()) {
                    KnobIntBasePtr isInt = toKnobIntBase(*it);
                    KnobBoolBasePtr isBool = toKnobBoolBase(*it);
                    KnobStringBasePtr isString = toKnobStringBase(*it);
                    KnobDoubleBasePtr isDouble = toKnobDoubleBase(*it);
                    for (int d = 0; d < (*it)->getNDimensions(); ++d) {
                        if (isInt) {
                            isInt->setDefaultValue(isInt->getInitialDefaultValue(DimIdx(d)), DimIdx(d));
                        } else if (isBool) {
                            isBool->setDefaultValue(isBool->getInitialDefaultValue(DimIdx(d)), DimIdx(d));
                        } else if (isString) {
                            isString->setDefaultValue(isString->getInitialDefaultValue(DimIdx(d)), DimIdx(d));
                        } else if (isDouble) {
                            isDouble->setDefaultValue(isDouble->getInitialDefaultValue(DimIdx(d)), DimIdx(d));
                        }
                    }
                }
            }
        }
    }

    // Load serialization before loading internal nodegraph as restoring parameters of the sub-nodegraph could reference user knobs
    // on this node
    if (projectSerialization) {
        fromSerialization(*projectSerialization);
    }

    if (!nodeCreated) {
        bool initialSubGraphSetupAllowed = false;
        if (args) {
            initialSubGraphSetupAllowed = !args->getProperty<bool>(kCreateNodeArgsPropNodeGroupDisableCreateInitialNodes);
        }

        loadInternalNodeGraph(initialSubGraphSetupAllowed, projectSerialization.get(), pyPlugSerialization.get());
    }


    // If there was a serialization, we most likely removed or created user parameters, so refresh Python knobs
    declarePythonKnobs();

    if (nodeCreated) {
        // Ensure the state of the node is consistent with what the plug-in expects
        int time = getApp()->getTimeLine()->currentFrame();
        {
            const KnobsVec& knobs = getKnobs();
            for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
                _imp->effect->onKnobValueChanged_public(*it, eValueChangedReasonRestoreDefault, time, ViewIdx(0), true);
            }
        }
    }

    _imp->effect->endChanges();

    // Refresh hash & meta-data and trigger a render
    _imp->effect->invalidateCacheHashAndEvaluate(true, true);

} // restoreNodeToDefaultState

void
Node::restoreSublabel()
{

    KnobIPtr sublabelKnob = getKnobByName(kNatronOfxParamStringSublabelName);
    if (!sublabelKnob) {
        return;
    }
    
    // Make sure the knob is not persistent
    sublabelKnob->setIsPersistent(false);
    
    KnobStringPtr sublabelKnobIsString = toKnobString(sublabelKnob);
    _imp->ofxSubLabelKnob = sublabelKnobIsString;

    NodePtr ioContainer = getIOContainer();
    if (ioContainer) {
        ioContainer->_imp->ofxSubLabelKnob = sublabelKnobIsString;
        Q_EMIT ioContainer->nodeExtraLabelChanged();
    }
 

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


void
Node::restoreKnobsLinks(const SERIALIZATION_NAMESPACE::NodeSerialization & serialization,
                        const NodesList & allNodes)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );


    const std::string & masterNodeName = serialization._masterNodecriptName;
    if ( !masterNodeName.empty() ) {

        // In the past the script-name contained the fully qualified name , e.g: Group1.Blur1
        // This leads to issues when restoring the graph in another Group name.
        // Ensure the name is only the script-name by removing the prefix
        NodePtr masterNode;
        std::size_t foundDot = masterNodeName.find_last_of(".");
        if (foundDot != std::string::npos) {
            masterNode = getGroup()->getNodeByName(masterNodeName.substr(foundDot + 1));
        } else {
            masterNode = getGroup()->getNodeByName(masterNodeName);
        }



        if (!masterNode) {
            LogEntry::LogEntryColor c;
            if (getColor(&c.r, &c.g, &c.b)) {
                c.colorSet = true;
            }

            appPTR->writeToErrorLog_mt_safe( QString::fromUtf8(getScriptName_mt_safe().c_str() ), QDateTime::currentDateTime(),
                                            tr("Cannot restore the link between %1 and %2.")
                                            .arg( QString::fromUtf8( serialization._nodeScriptName.c_str() ) )
                                            .arg( QString::fromUtf8( masterNodeName.c_str() ) ) );
        } else {
            _imp->effect->slaveAllKnobs( masterNode->getEffectInstance() );
        }
        return;
    }


    const SERIALIZATION_NAMESPACE::KnobSerializationList & knobsValues = serialization._knobsValues;
    ///try to find a serialized value for this knob
    for (SERIALIZATION_NAMESPACE::KnobSerializationList::const_iterator it = knobsValues.begin(); it != knobsValues.end(); ++it) {
        try {
            restoreKnobLinks(*it, allNodes);
        } catch (const std::exception& e) {
            // For stub nodes don't report errors
            if (!isEffectStubNode()) {
                LogEntry::LogEntryColor c;
                if (getColor(&c.r, &c.g, &c.b)) {
                    c.colorSet = true;
                }
                appPTR->writeToErrorLog_mt_safe(QString::fromUtf8(getScriptName_mt_safe().c_str() ), QDateTime::currentDateTime(), QString::fromUtf8(e.what()), false, c);
            }
        }
    }

    const std::list<boost::shared_ptr<SERIALIZATION_NAMESPACE::GroupKnobSerialization> >& userKnobs = serialization._userPages;
    for (std::list<boost::shared_ptr<SERIALIZATION_NAMESPACE::GroupKnobSerialization > >::const_iterator it = userKnobs.begin(); it != userKnobs.end(); ++it) {
        try {
            restoreKnobLinks(*it, allNodes);
        } catch (const std::exception& e) {
            LogEntry::LogEntryColor c;
            if (getColor(&c.r, &c.g, &c.b)) {
                c.colorSet = true;
            }
            appPTR->writeToErrorLog_mt_safe(QString::fromUtf8(getScriptName_mt_safe().c_str() ), QDateTime::currentDateTime(), QString::fromUtf8(e.what()), false, c);

        }
    }
}

void
Node::setPagesOrder(const std::list<std::string>& pages)
{
    //re-order the pages
    std::vector<KnobIPtr> pagesOrdered(pages.size());

    KnobsVec knobs = getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobPagePtr isPage = toKnobPage(*it);
        if (!isPage) {
            continue;
        }

        // Look for it in the pages
        int i = 0;
        bool found = false;
        for (std::list<std::string>::const_iterator it2 = pages.begin(); it2 != pages.end(); ++it2, ++i) {
            if (*it2 == isPage->getName()) {
                pagesOrdered[i] = *it;
                _imp->effect->removeKnobFromList(isPage);
                isPage->setSecret(false);
                found = true;
                break;
            }
        }
        if (!found && isPage->isUserKnob()) {
            isPage->setSecret(true);
        }
    }
    int index = 0;
    for (std::vector<KnobIPtr >::iterator it =  pagesOrdered.begin(); it != pagesOrdered.end(); ++it, ++index) {
        if (*it) {
            _imp->effect->insertKnob(index, *it);
        }
    }
}

void
Node::refreshDefaultPagesOrder()
{
    _imp->refreshDefaultPagesOrder();
}


std::list<std::string>
Node::getPagesOrder() const
{
    KnobsVec knobs = _imp->effect->getKnobs_mt_safe();
    std::list<std::string> ret;

    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobPagePtr ispage = toKnobPage(*it);
        if (ispage && !ispage->getChildren().empty() && !ispage->getIsSecret()) {
            ret.push_back( ispage->getName() );
        }
    }

    return ret;
}

bool
Node::hasPageOrderChangedSinceDefault() const
{
    std::list<std::string> pagesOrder = getPagesOrder();
    if (pagesOrder.size() != _imp->defaultPagesOrder.size()) {
        return true;
    }
    std::list<std::string>::const_iterator it2 = _imp->defaultPagesOrder.begin();
    for (std::list<std::string>::const_iterator it = pagesOrder.begin(); it!=pagesOrder.end(); ++it, ++it2) {
        if (*it != *it2) {
            return true;
        }
    }
    return false;
}

bool
Node::isRenderingPreview() const
{
    QMutexLocker l(&_imp->computingPreviewMutex);

    return _imp->computingPreview;
}

bool
Node::hasOverlay() const
{
    if (!_imp->effect) {
        return false;
    }

    NodeGuiIPtr nodeGui = getNodeGui();
    if (nodeGui) {
        if ( nodeGui->hasHostOverlay() ) {
            return true;
        }
    }

    return _imp->effect->hasOverlay();
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
    OutputEffectInstancePtr isOutput = isEffectOutput();

    if (isOutput) {
        if ( isOutput->getRenderEngine()->hasThreadsAlive() ) {
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
    OutputEffectInstancePtr isOutput = isEffectOutput();

    if (isOutput) {
        isOutput->getRenderEngine()->quitEngine(true);
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
    OutputEffectInstancePtr isOutput = isEffectOutput();

    if (isOutput) {
        RenderEnginePtr engine = isOutput->getRenderEngine();
        if (engine) {
            engine->quitEngine(allowThreadsToRestart);
            engine->waitForEngineToQuit_enforce_blocking();
        }
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
    OutputEffectInstancePtr isOutput = isEffectOutput();

    if (isOutput) {
        isOutput->getRenderEngine()->abortRenderingNoRestart();
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
    OutputEffectInstancePtr isOutput = isEffectOutput();

    if (isOutput) {
        RenderEnginePtr engine = isOutput->getRenderEngine();
        assert(engine);
        engine->abortRenderingNoRestart();
        engine->waitForAbortToComplete_enforce_blocking();
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



const std::vector<std::string> &
Node::getInputLabels() const
{
    assert(_imp->inputsInitialized);
    ///MT-safe as it never changes.
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->inputLabels;
}

const NodesWList &
Node::getOutputs() const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->outputs;
}

void
Node::getOutputs_mt_safe(NodesWList& outputs) const
{
    QMutexLocker l(&_imp->outputsMutex);

    outputs =  _imp->outputs;
}

void
Node::getInputNames(std::map<std::string, std::string> & inputNames) const
{
    // This is called by the serialization thread.
    // We use the inputs because we want to serialize exactly how the tree was to the user

    QMutexLocker l(&_imp->inputsLabelsMutex);
    assert( _imp->inputs.size() == _imp->inputLabels.size() );
    for (std::size_t i = 0; i < _imp->inputs.size(); ++i) {
        NodePtr input = _imp->inputs[i].lock();
        if (input) {
            inputNames.insert( std::make_pair( _imp->inputLabels[i], input->getScriptName_mt_safe() ) );
        }
    }
}

int
Node::getPreferredInputInternal(bool connected) const
{
    int nInputs = getMaxInputCount();

    if (nInputs == 0) {
        return -1;
    }
    std::vector<NodePtr> inputs(nInputs);
    std::vector<std::string> inputLabels(nInputs);
    int inputA = -1;
    int inputB = -1;
    {
        // fill input labels, and if one is called "Source", return it
        // if it's "A" or "B", keep the index.
        for (int i = 0; i < nInputs; ++i) {
            std::string inputLabel = getInputLabel(i);
            //printf("%d->%s\n", i, inputLabel.c_str());
            if (inputLabel == kOfxImageEffectSimpleSourceClipName) {
                inputs[i] = getInput(i);
                if ( (connected && inputs[i]) || (!connected && !inputs[i]) ) {
                    return i;
                }
            } else if (inputLabel == "A") {
                inputA = i;
            } else if (inputLabel == "B") {
                inputB = i;
            }
        }
    }
    bool useInputA = false;
    if (!connected) {
        // For the merge node, use the preference (only when not connected)
        useInputA = appPTR->getCurrentSettings()->isMergeAutoConnectingToAInput();
    }

    ///Find an input named A
    int inputToFind = -1, foundOther = -1;
    if ( useInputA || (getPluginID() == PLUGINID_OFX_SHUFFLE) ) {
        inputToFind = inputA;
        foundOther = inputB;
    } else {
        inputToFind = inputB;
        foundOther = inputA;
    }
    if (inputToFind != -1) {
        inputs[inputToFind] = getInput(inputToFind);
        if ( (connected && inputs[inputToFind]) || (!connected && !inputs[inputToFind]) ) {
            return inputToFind;
        }
    }
    if (foundOther != -1) {
        inputs[foundOther] = getInput(foundOther);
        if ( (connected && inputs[foundOther]) || (!connected && !inputs[foundOther]) ) {
            return foundOther;
        }
    }


    for (int i = 0; i < nInputs; ++i) {
        if (!inputs[i]) {
            inputs[i] = getInput(i);
        }
    }


    ///we return the first non-optional empty input
    int firstNonOptionalEmptyInput = -1;
    std::list<int> optionalEmptyInputs;
    std::list<int> optionalEmptyMasks;

    for (int i = 0; i < nInputs; ++i) {
        if ( (connected && inputs[i]) || (!connected && !inputs[i]) ) {
            if ( !_imp->effect->isInputOptional(i) ) {
                if (firstNonOptionalEmptyInput == -1) {
                    firstNonOptionalEmptyInput = i;
                    break;
                }
            } else {
                if ( _imp->effect->isInputMask(i) ) {
                    optionalEmptyMasks.push_back(i);
                } else {
                    optionalEmptyInputs.push_back(i);
                }
            }
        }
    }


    ///Default to the first non optional empty input
    if (firstNonOptionalEmptyInput != -1) {
        return firstNonOptionalEmptyInput;
    }  else {
        if ( !optionalEmptyInputs.empty() ) {
            //We return the first optional empty input
            std::list<int>::iterator first = optionalEmptyInputs.begin();

            if ( first == optionalEmptyInputs.end() ) {
                return -1;
            } else {
                return *first;
            }
        } else if ( !optionalEmptyMasks.empty() ) {
            return optionalEmptyMasks.front();
        } else {
            return -1;
        }
    }
} // Node::getPreferredInputInternal

int
Node::getPreferredInput() const
{
    return getPreferredInputInternal(true);
}

int
Node::getPreferredInputForConnection() const
{
    return getPreferredInputInternal(false);
}

NodePtr
Node::getPreferredInputNode() const
{
    GroupInputPtr isInput = isEffectGroupInput();
    PrecompNodePtr isPrecomp = isEffectPrecompNode();

    if (isInput) {
        NodeGroupPtr isGroup = toNodeGroup(getGroup());
        assert(isGroup);
        if (!isGroup) {
            return NodePtr();
        }
        int inputNb = -1;
        std::vector<NodePtr> groupInputs;
        isGroup->getInputs(&groupInputs);
        for (std::size_t i = 0; i < groupInputs.size(); ++i) {
            if (groupInputs[i].get() == this) {
                inputNb = i;
                break;
            }
        }
        if (inputNb != -1) {
            NodePtr input = isGroup->getNode()->getInput(inputNb);

            return input;
        }
    } else if (isPrecomp) {
        return isPrecomp->getOutputNode();
    } else {
        int idx = getPreferredInput();
        if (idx != -1) {
            return getInput(idx);
        }
    }

    return NodePtr();
}

void
Node::getOutputsConnectedToThisNode(std::map<NodePtr, int>* outputs)
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    NodePtr thisSHared = shared_from_this();
    for (NodesWList::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
        NodePtr output = it->lock();
        if (!output) {
            continue;
        }

        int indexOfThis = output->inputIndex(thisSHared);
        assert(indexOfThis != -1);
        if (indexOfThis >= 0) {
            outputs->insert( std::make_pair(output, indexOfThis) );
        }
    }
}

const std::string &
Node::getScriptName() const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    QMutexLocker l(&_imp->nameMutex);

    return _imp->scriptName;
}

std::string
Node::getScriptName_mt_safe() const
{
    QMutexLocker l(&_imp->nameMutex);

    return _imp->scriptName;
}

static void
prependGroupNameRecursive(const NodePtr& group,
                          std::string& name)
{
    name.insert(0, ".");
    name.insert( 0, group->getScriptName_mt_safe() );
    NodeCollectionPtr hasParentGroup = group->getGroup();
    NodeGroupPtr isGrp = toNodeGroup(hasParentGroup);
    if (isGrp) {
        prependGroupNameRecursive(isGrp->getNode(), name);
    }
}

std::string
Node::getFullyQualifiedNameInternal(const std::string& scriptName) const
{
    std::string ret = scriptName;


    NodeCollectionPtr hasParentGroup = getGroup();
    NodeGroupPtr isGrp = toNodeGroup(hasParentGroup);
    if (isGrp) {
        NodePtr grpNode = isGrp->getNode();
        if (grpNode) {
            prependGroupNameRecursive(grpNode, ret);
        }
    }


    return ret;
}

std::string
Node::getFullyQualifiedName() const
{
    return getFullyQualifiedNameInternal( getScriptName_mt_safe() );
}

void
Node::setLabel(const std::string& label)
{
    assert( QThread::currentThread() == qApp->thread() );


    {
        QMutexLocker k(&_imp->nameMutex);
        if (label == _imp->label) {
            return;
        }
        _imp->label = label;
    }
    NodeCollectionPtr collection = getGroup();
    if (collection) {
        collection->notifyNodeLabelChanged( shared_from_this() );
    }
    Q_EMIT labelChanged( QString::fromUtf8( label.c_str() ) );
}

const std::string&
Node::getLabel() const
{
    assert( QThread::currentThread() == qApp->thread() );
    QMutexLocker k(&_imp->nameMutex);

    return _imp->label;
}

std::string
Node::getLabel_mt_safe() const
{
    QMutexLocker k(&_imp->nameMutex);

    return _imp->label;
}

void
Node::setScriptName_no_error_check(const std::string & name)
{
    setNameInternal(name, false);
}

static void
insertDependenciesRecursive(Node* node,
                            KnobI::ListenerDimsMap* dependencies)
{
    const KnobsVec & knobs = node->getKnobs();

    for (std::size_t i = 0; i < knobs.size(); ++i) {
        KnobI::ListenerDimsMap dimDeps;
        knobs[i]->getListeners(dimDeps);
        for (KnobI::ListenerDimsMap::iterator it = dimDeps.begin(); it != dimDeps.end(); ++it) {
            dependencies->insert(*it);
        }
    }

    NodeGroupPtr isGroup = node->isEffectNodeGroup();
    if (isGroup) {
        NodesList nodes = isGroup->getNodes();
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            insertDependenciesRecursive(it->get(), dependencies);
        }
    }
}

void
Node::setNameInternal(const std::string& name,
                      bool throwErrors)
{
    std::string oldName = getScriptName_mt_safe();
    std::string fullOldName = getFullyQualifiedName();
    std::string newName = name;
    bool onlySpaces = true;

    for (std::size_t i = 0; i < name.size(); ++i) {
        if (name[i] != '_') {
            onlySpaces = false;
            break;
        }
    }
    if (onlySpaces) {
        QString err = tr("The name must at least contain a character");
        if (throwErrors) {
            throw std::runtime_error(err.toStdString());
        } else {
            LogEntry::LogEntryColor c;
            if (getColor(&c.r, &c.g, &c.b)) {
                c.colorSet = true;
            }
            appPTR->writeToErrorLog_mt_safe(QString::fromUtf8(getFullyQualifiedName().c_str()), QDateTime::currentDateTime(), err, false, c);
            std::cerr << err.toStdString() << std::endl;

            return;
        }
    }

    NodeCollectionPtr collection = getGroup();
    if (collection) {
        if (throwErrors) {
            try {
                collection->checkNodeName(shared_from_this(), name, false, false, &newName);
            } catch (const std::exception& e) {
                LogEntry::LogEntryColor c;
                if (getColor(&c.r, &c.g, &c.b)) {
                    c.colorSet = true;
                }
                appPTR->writeToErrorLog_mt_safe(QString::fromUtf8(getFullyQualifiedName().c_str()), QDateTime::currentDateTime(), QString::fromUtf8( e.what() ), false, c );
                std::cerr << e.what() << std::endl;

                return;
            }
        } else {
            collection->checkNodeName(shared_from_this(), name, false, false, &newName);
        }
    }


    if (oldName == newName) {
        return;
    }


    if ( !newName.empty() ) {
        bool isAttrDefined = false;
        std::string newPotentialQualifiedName = getApp()->getAppIDString() + "." + getFullyQualifiedNameInternal(newName);
        PyObject* obj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(newPotentialQualifiedName, appPTR->getMainModule(), &isAttrDefined);
        Q_UNUSED(obj);
        if (isAttrDefined) {
            std::stringstream ss;
            ss << "A Python attribute with the same name (" << newPotentialQualifiedName << ") already exists.";
            if (throwErrors) {
                throw std::runtime_error( ss.str() );
            } else {
                std::string err = ss.str();
                LogEntry::LogEntryColor c;
                if (getColor(&c.r, &c.g, &c.b)) {
                    c.colorSet = true;
                }
                appPTR->writeToErrorLog_mt_safe(QString::fromUtf8(oldName.c_str()), QDateTime::currentDateTime(), QString::fromUtf8( err.c_str() ), false, c );
                std::cerr << err << std::endl;

                return;
            }
        }
    }

    {
        QMutexLocker l(&_imp->nameMutex);
        _imp->scriptName = newName;
        ///Set the label at the same time if the label is empty
        if ( _imp->label.empty() ) {
            _imp->label = newName;
        }
    }
    std::string fullySpecifiedName = getFullyQualifiedName();


    if (collection) {
        if ( !oldName.empty() ) {
            if (fullOldName != fullySpecifiedName) {
                try {
                    setNodeVariableToPython(fullOldName, fullySpecifiedName);
                } catch (const std::exception& e) {
                    qDebug() << e.what();
                }
            }
        } else { //if (!oldName.empty()) {
            declareNodeVariableToPython(fullySpecifiedName);
        }

        if (_imp->nodeCreated) {
            ///For all knobs that have listeners, change in the expressions of listeners this knob script-name
            KnobI::ListenerDimsMap dependencies;
            insertDependenciesRecursive(this, &dependencies);
            for (KnobI::ListenerDimsMap::iterator it = dependencies.begin(); it != dependencies.end(); ++it) {
                KnobIPtr listener = it->first.lock();
                if (!listener) {
                    continue;
                }
                for (std::list<KnobI::ListenerLink>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                    if (it2->isExpr) {
                        listener->replaceNodeNameInExpression(it2->listenerDimension, it2->listenerView, oldName, newName);
                    }
                }
            }
        }
    }

    QString qnewName = QString::fromUtf8( newName.c_str() );
    Q_EMIT scriptNameChanged(qnewName);
    Q_EMIT labelChanged(qnewName);
} // Node::setNameInternal


void
Node::setScriptName(const std::string& name)
{
    std::string newName;

    if ( getGroup() ) {
        getGroup()->checkNodeName(shared_from_this(), name, false, true, &newName);
    } else {
        newName = name;
    }
    //We do not allow setting the script-name of output nodes because we rely on it with NatronRenderer
    if ( isEffectGroupOutput() ) {
        throw std::runtime_error( tr("Changing the script-name of an Output node is not a valid operation.").toStdString() );

        return;
    }


    setNameInternal(newName, true);
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


std::string
Node::makeInfoForInput(int inputNumber) const
{
    if ( (inputNumber < -1) || ( inputNumber >= getMaxInputCount() ) ) {
        return "";
    }
    EffectInstancePtr input;
    if (inputNumber != -1) {
        input = _imp->effect->getInput(inputNumber);
        /*if (input) {
            input = input->getNearestNonIdentity( getApp()->getTimeLine()->currentFrame() );
        }*/
    } else {
        input = _imp->effect;
    }

    if (!input) {
        return "";
    }


    ImageBitDepthEnum depth = _imp->effect->getBitDepth(inputNumber);
    double time = getApp()->getTimeLine()->currentFrame();
    std::stringstream ss;
    { // input name
        QString inputName;
        if (inputNumber != -1) {
            inputName = QString::fromUtf8( getInputLabel(inputNumber).c_str() );
        } else {
            inputName = tr("Output");
        }
        ss << "<b><font color=\"orange\">" << tr("%1:").arg(inputName).toStdString() << "</font></b><br />";
    }
    { // image format
        ss << "<b>" << tr("Image planes:").toStdString() << "</b> <font color=#c8c8c8>";
        EffectInstance::ComponentsAvailableMap availableComps;
        input->getComponentsAvailable(true, true, time, &availableComps);
        EffectInstance::ComponentsAvailableMap::iterator next = availableComps.begin();
        if ( next != availableComps.end() ) {
            ++next;
        }
        for (EffectInstance::ComponentsAvailableMap::iterator it = availableComps.begin(); it != availableComps.end(); ++it) {
            NodePtr origin = it->second.lock();
            if ( (origin.get() != this) || (inputNumber == -1) ) {
                if (origin) {
                    ss << Image::getFormatString(it->first, depth);
                    if (inputNumber != -1) {
                        ss << " " << tr("(from %1)").arg( QString::fromUtf8( origin->getLabel_mt_safe().c_str() ) ).toStdString();
                    }
                }
            }
            if ( next != availableComps.end() ) {
                if (origin) {
                    if ( (origin.get() != this) || (inputNumber == -1) ) {
                        ss << ", ";
                    }
                }
                ++next;
            }
        }
        ss << "</font><br />";
    }
    { // premult
        ImagePremultiplicationEnum premult = input->getPremult();
        QString premultStr = tr("unknown");
        switch (premult) {
        case eImagePremultiplicationOpaque:
            premultStr = tr("opaque");
            break;
        case eImagePremultiplicationPremultiplied:
            premultStr = tr("premultiplied");
            break;
        case eImagePremultiplicationUnPremultiplied:
            premultStr = tr("unpremultiplied");
            break;
        }
        ss << "<b>" << tr("Alpha premultiplication:").toStdString() << "</b> <font color=#c8c8c8>" << premultStr.toStdString() << "</font><br />";
    }
    { // par
        double par = input->getAspectRatio(-1);
        ss << "<b>" << tr("Pixel aspect ratio:").toStdString() << "</b> <font color=#c8c8c8>" << par << "</font><br />";
    }
    { // fps
        double fps = input->getFrameRate();
        ss << "<b>" << tr("Frame rate:").toStdString() << "</b> <font color=#c8c8c8>" << tr("%1fps").arg(fps).toStdString() << "</font><br />";
    }
    {
        double first = 1., last = 1.;
        input->getFrameRange_public(0, &first, &last);
        ss << "<b>" << tr("Frame range:").toStdString() << "</b> <font color=#c8c8c8>" << first << " - " << last << "</font><br />";
    }
    {
        RenderScale scale(1.);
        RectD rod;
        StatusEnum stat = input->getRegionOfDefinition_public(0,
                                                              time,
                                                              scale, ViewIdx(0), &rod);
        if (stat != eStatusFailed) {
            ss << "<b>" << tr("Region of Definition (at t=%1):").arg(time).toStdString() << "</b> <font color=#c8c8c8>";
            ss << tr("left = %1 bottom = %2 right = %3 top = %4").arg(rod.x1).arg(rod.y1).arg(rod.x2).arg(rod.y2).toStdString() << "</font><br />";
        }
    }

    return ss.str();
} // Node::makeInfoForInput

void
Node::findPluginFormatKnobs()
{
    findPluginFormatKnobs(getKnobs(), true);
}

void
Node::findRightClickMenuKnob(const KnobsVec& knobs)
{
    for (std::size_t i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->getName() == kNatronOfxParamRightClickMenu) {
            KnobIPtr rightClickKnob = knobs[i];
            KnobChoicePtr isChoice = toKnobChoice(rightClickKnob);
            if (isChoice) {
                QObject::connect( isChoice.get(), SIGNAL(populated()), this, SIGNAL(rightClickMenuKnobPopulated()) );
            }
            break;
        }
    }
}

void
Node::findPluginFormatKnobs(const KnobsVec & knobs,
                            bool loadingSerialization)
{
    ///Try to find a format param and hijack it to handle it ourselves with the project's formats
    KnobIPtr formatKnob;

    for (std::size_t i = 0; i < knobs.size(); ++i) {
        if (knobs[i]->getName() == kNatronParamFormatChoice) {
            formatKnob = knobs[i];
            break;
        }
    }
    if (formatKnob) {
        KnobIPtr formatSize;
        for (std::size_t i = 0; i < knobs.size(); ++i) {
            if (knobs[i]->getName() == kNatronParamFormatSize) {
                formatSize = knobs[i];
                break;
            }
        }
        KnobIPtr formatPar;
        for (std::size_t i = 0; i < knobs.size(); ++i) {
            if (knobs[i]->getName() == kNatronParamFormatPar) {
                formatPar = knobs[i];
                break;
            }
        }
        if (formatSize && formatPar) {
            _imp->pluginFormatKnobs.formatChoice = toKnobChoice(formatKnob);
            formatSize->setEvaluateOnChange(false);
            formatPar->setEvaluateOnChange(false);
            formatSize->setSecret(true);
            formatPar->setSecret(true);
            _imp->pluginFormatKnobs.size = toKnobInt(formatSize);
            _imp->pluginFormatKnobs.par = toKnobDouble(formatPar);

            std::vector<std::string> formats;
            int defValue;
            getApp()->getProject()->getProjectFormatEntries(&formats, &defValue);
            refreshFormatParamChoice(formats, defValue, loadingSerialization);
        }
    }
}

void
Node::createNodePage(const KnobPagePtr& settingsPage)
{
    KnobBoolPtr hideInputs = AppManager::createKnob<KnobBool>(_imp->effect, tr("Hide inputs"), 1, false);

    hideInputs->setName("hideInputs");
    hideInputs->setDefaultValue(false);
    hideInputs->setAnimationEnabled(false);
    hideInputs->setAddNewLine(false);
    hideInputs->setIsPersistent(true);
    hideInputs->setEvaluateOnChange(false);
    hideInputs->setHintToolTip( tr("When checked, the input arrows of the node in the nodegraph will be hidden") );
    _imp->hideInputs = hideInputs;
    settingsPage->addKnob(hideInputs);


    KnobBoolPtr fCaching = AppManager::createKnob<KnobBool>(_imp->effect, tr("Force caching"), 1, false);
    fCaching->setName("forceCaching");
    fCaching->setDefaultValue(false);
    fCaching->setAnimationEnabled(false);
    fCaching->setAddNewLine(false);
    fCaching->setIsPersistent(true);
    fCaching->setEvaluateOnChange(false);
    fCaching->setHintToolTip( tr("When checked, the output of this node will always be kept in the RAM cache for fast access of already computed "
                                 "images.") );
    _imp->forceCaching = fCaching;
    settingsPage->addKnob(fCaching);

    KnobBoolPtr previewEnabled = AppManager::createKnob<KnobBool>(_imp->effect, tr("Preview"), 1, false);
    assert(previewEnabled);
    previewEnabled->setDefaultValue( makePreviewByDefault() );
    previewEnabled->setName(kEnablePreviewKnobName);
    previewEnabled->setAnimationEnabled(false);
    previewEnabled->setAddNewLine(false);
    previewEnabled->setIsPersistent(false);
    previewEnabled->setEvaluateOnChange(false);
    previewEnabled->setHintToolTip( tr("Whether to show a preview on the node box in the node-graph.") );
    settingsPage->addKnob(previewEnabled);
    _imp->previewEnabledKnob = previewEnabled;

    KnobBoolPtr disableNodeKnob = AppManager::createKnob<KnobBool>(_imp->effect, tr("Disable"), 1, false);
    assert(disableNodeKnob);
    disableNodeKnob->setAnimationEnabled(false);
    disableNodeKnob->setIsMetadataSlave(true);
    disableNodeKnob->setName(kDisableNodeKnobName);
    disableNodeKnob->setAddNewLine(false);
    _imp->effect->addOverlaySlaveParam(disableNodeKnob);
    disableNodeKnob->setHintToolTip( tr("When disabled, this node acts as a pass through.") );
    settingsPage->addKnob(disableNodeKnob);
    _imp->disableNodeKnob = disableNodeKnob;



    KnobBoolPtr useFullScaleImagesWhenRenderScaleUnsupported = AppManager::createKnob<KnobBool>(_imp->effect, tr("Render high def. upstream"), 1, false);
    useFullScaleImagesWhenRenderScaleUnsupported->setAnimationEnabled(false);
    useFullScaleImagesWhenRenderScaleUnsupported->setDefaultValue(false);
    useFullScaleImagesWhenRenderScaleUnsupported->setName("highDefUpstream");
    useFullScaleImagesWhenRenderScaleUnsupported->setHintToolTip( tr("This node does not support rendering images at a scale lower than 1, it "
                                                                     "can only render high definition images. When checked this parameter controls "
                                                                     "whether the rest of the graph upstream should be rendered with a high quality too or at "
                                                                     "the most optimal resolution for the current viewer's viewport. Typically checking this "
                                                                     "means that an image will be slow to be rendered, but once rendered it will stick in the cache "
                                                                     "whichever zoom level you are using on the Viewer, whereas when unchecked it will be much "
                                                                     "faster to render but will have to be recomputed when zooming in/out in the Viewer.") );
    if ( ( isRenderScaleSupportEnabledForPlugin() ) && (getEffectInstance()->supportsRenderScaleMaybe() == EffectInstance::eSupportsYes) ) {
        useFullScaleImagesWhenRenderScaleUnsupported->setSecret(true);
    }
    settingsPage->addKnob(useFullScaleImagesWhenRenderScaleUnsupported);
    _imp->useFullScaleImagesWhenRenderScaleUnsupported = useFullScaleImagesWhenRenderScaleUnsupported;


    KnobIntPtr lifeTimeKnob = AppManager::createKnob<KnobInt>(_imp->effect, tr("Lifetime Range"), 2, false);
    assert(lifeTimeKnob);
    lifeTimeKnob->setAnimationEnabled(false);
    lifeTimeKnob->setIsMetadataSlave(true);
    lifeTimeKnob->setName(kLifeTimeNodeKnobName);
    lifeTimeKnob->setAddNewLine(false);
    lifeTimeKnob->setHintToolTip( tr("This is the frame range during which the node will be active if Enable Lifetime is checked") );
    settingsPage->addKnob(lifeTimeKnob);
    _imp->lifeTimeKnob = lifeTimeKnob;


    KnobBoolPtr enableLifetimeNodeKnob = AppManager::createKnob<KnobBool>(_imp->effect, tr("Enable Lifetime"), 1, false);
    assert(enableLifetimeNodeKnob);
    enableLifetimeNodeKnob->setAnimationEnabled(false);
    enableLifetimeNodeKnob->setDefaultValue(false);
    enableLifetimeNodeKnob->setIsMetadataSlave(true);
    enableLifetimeNodeKnob->setName(kEnableLifeTimeNodeKnobName);
    enableLifetimeNodeKnob->setHintToolTip( tr("When checked, the node is only active during the specified frame range by the Lifetime Range parameter. "
                                               "Outside of this frame range, it behaves as if the Disable parameter is checked") );
    settingsPage->addKnob(enableLifetimeNodeKnob);
    _imp->enableLifeTimeKnob = enableLifetimeNodeKnob;

    PluginOpenGLRenderSupport glSupport = ePluginOpenGLRenderSupportNone;

    PluginPtr plugin = getPlugin();
    if (plugin && plugin->isOpenGLEnabled()) {
        glSupport = (PluginOpenGLRenderSupport)plugin->getProperty<int>(kNatronPluginPropOpenGLSupport);
    }
    // The Roto node needs to have a "GPU enabled" knob to control the nodes internally
    if (glSupport != ePluginOpenGLRenderSupportNone || dynamic_cast<RotoPaint*>(_imp->effect.get())) {
        getOrCreateOpenGLEnabledKnob();
    }


    KnobStringPtr knobChangedCallback = AppManager::createKnob<KnobString>(_imp->effect, tr("After param changed callback"), 1, false);
    knobChangedCallback->setHintToolTip( tr("Set here the name of a function defined in Python which will be called for each  "
                                            "parameter change. Either define this function in the Script Editor "
                                            "or in the init.py script or even in the script of a Python group plug-in.\n"
                                            "The signature of the callback is: callback(thisParam, thisNode, thisGroup, app, userEdited) where:\n"
                                            "- thisParam: The parameter which just had its value changed\n"
                                            "- userEdited: A boolean informing whether the change was due to user interaction or "
                                            "because something internally triggered the change.\n"
                                            "- thisNode: The node holding the parameter\n"
                                            "- app: points to the current application instance\n"
                                            "- thisGroup: The group holding thisNode (only if thisNode belongs to a group)") );
    knobChangedCallback->setAnimationEnabled(false);
    knobChangedCallback->setName("onParamChanged");
    settingsPage->addKnob(knobChangedCallback);
    _imp->knobChangedCallback = knobChangedCallback;

    KnobStringPtr inputChangedCallback = AppManager::createKnob<KnobString>(_imp->effect, tr("After input changed callback"), 1, false);
    inputChangedCallback->setHintToolTip( tr("Set here the name of a function defined in Python which will be called after "
                                             "each connection is changed for the inputs of the node. "
                                             "Either define this function in the Script Editor "
                                             "or in the init.py script or even in the script of a Python group plug-in.\n"
                                             "The signature of the callback is: callback(inputIndex, thisNode, thisGroup, app):\n"
                                             "- inputIndex: the index of the input which changed, you can query the node "
                                             "connected to the input by calling the getInput(...) function.\n"
                                             "- thisNode: The node holding the parameter\n"
                                             "- app: points to the current application instance\n"
                                             "- thisGroup: The group holding thisNode (only if thisNode belongs to a group)") );

    inputChangedCallback->setAnimationEnabled(false);
    inputChangedCallback->setName("onInputChanged");
    settingsPage->addKnob(inputChangedCallback);
    _imp->inputChangedCallback = inputChangedCallback;

    NodeGroupPtr isGroup = isEffectNodeGroup();
    if (isGroup) {
        KnobStringPtr onNodeCreated = AppManager::createKnob<KnobString>(_imp->effect, tr("After Node Created"), 1, false);
        onNodeCreated->setName("afterNodeCreated");
        onNodeCreated->setHintToolTip( tr("Add here the name of a Python-defined function that will be called each time a node "
                                          "is created in the group. This will be called in addition to the After Node Created "
                                          " callback of the project for the group node and all nodes within it (not recursively).\n"
                                          "The boolean variable userEdited will be set to True if the node was created "
                                          "by the user or False otherwise (such as when loading a project, or pasting a node).\n"
                                          "The signature of the callback is: callback(thisNode, app, userEdited) where:\n"
                                          "- thisNode: the node which has just been created\n"
                                          "- userEdited: a boolean indicating whether the node was created by user interaction or from "
                                          "a script/project load/copy-paste\n"
                                          "- app: points to the current application instance.") );
        onNodeCreated->setAnimationEnabled(false);
        _imp->nodeCreatedCallback = onNodeCreated;
        settingsPage->addKnob(onNodeCreated);

        KnobStringPtr onNodeDeleted = AppManager::createKnob<KnobString>(_imp->effect, tr("Before Node Removal"), 1, false);
        onNodeDeleted->setName("beforeNodeRemoval");
        onNodeDeleted->setHintToolTip( tr("Add here the name of a Python-defined function that will be called each time a node "
                                          "is about to be deleted. This will be called in addition to the Before Node Removal "
                                          " callback of the project for the group node and all nodes within it (not recursively).\n"
                                          "This function will not be called when the project is closing.\n"
                                          "The signature of the callback is: callback(thisNode, app) where:\n"
                                          "- thisNode: the node about to be deleted\n"
                                          "- app: points to the current application instance.") );
        onNodeDeleted->setAnimationEnabled(false);
        _imp->nodeRemovalCallback = onNodeDeleted;
        settingsPage->addKnob(onNodeDeleted);
    }
    if (_imp->effect->getItemsTable()) {
        KnobStringPtr param = AppManager::createKnob<KnobString>(_imp->effect, tr("After Items Selection Changed"), 1, false);
        param->setName("afterItemsSelectionChanged");
        param->setHintToolTip( tr("Add here the name of a Python-defined function that will be called each time the "
                                  "selection in the table changes. "
                                  "The variable \"reason\" will be set to a value of type NatronEngine.Natron.TableChangeReasonEnum "
                                  "depending on where the selection was made from. If reason is "
                                  "NatronEngine.Natron.TableChangeReasonEnum.eTableChangeReasonViewer then the selection was made "
                                  "from the viewer. If reason is NatronEngine.Natron.TableChangeReasonEnum.eTableChangeReasonPanel "
                                  "then the selection was made from the settings panel. Otherwise the selection was not changed "
                                  "by the user directly and results from an internal A.P.I call.\n"
                                  "The signature of the callback is: callback(thisNode, app, deselected, selected, reason) where:\n"
                                  "- thisNode: the node holding the items table\n"
                                  "- app: points to the current application instance\n"
                                  "- deselected: a sequence of items that were removed from the selection\n"
                                  "- selected: a sequence of items that were added to the selection\n"
                                  "- reason: a value of type NatronEngine.Natron.TableChangeReasonEnum") );
        param->setAnimationEnabled(false);
        _imp->tableSelectionChangedCallback = param;
        settingsPage->addKnob(param);

    }
} // Node::createNodePage

void
Node::createInfoPage()
{
    KnobPagePtr infoPage = AppManager::createKnob<KnobPage>(_imp->effect, tr(kInfoPageParamLabel), 1, false);
    infoPage->setName(kInfoPageParamName);
    _imp->infoPage = infoPage;

    KnobStringPtr nodeInfos = AppManager::createKnob<KnobString>(_imp->effect, std::string(), 1, false);
    nodeInfos->setName("nodeInfos");
    nodeInfos->setAnimationEnabled(false);
    nodeInfos->setIsPersistent(false);
    nodeInfos->setAsMultiLine();
    nodeInfos->setAsCustomHTMLText(true);
    nodeInfos->setEvaluateOnChange(false);
    nodeInfos->setHintToolTip( tr("Input and output informations, press Refresh to update them with current values") );
    infoPage->addKnob(nodeInfos);
    _imp->nodeInfos = nodeInfos;


    KnobButtonPtr refreshInfoButton = AppManager::createKnob<KnobButton>(_imp->effect, tr("Refresh Info"), 1, false);
    refreshInfoButton->setName("refreshButton");
    refreshInfoButton->setEvaluateOnChange(false);
    infoPage->addKnob(refreshInfoButton);
    _imp->refreshInfoButton = refreshInfoButton;
}

void
Node::createPyPlugExportGroup()
{
    // Create the knobs in either page since they are hidden anyway
    KnobPagePtr mainPage;
    const KnobsVec& knobs = getKnobs();
    for (std::size_t i = 0; i < knobs.size(); ++i) {
        KnobPagePtr p = toKnobPage(knobs[i]);
        if (p) {
            mainPage = p;
            break;
        }
    }
    assert(mainPage);
    KnobGroupPtr group;
    {
        KnobGroupPtr param = AppManager::createKnob<KnobGroup>( _imp->effect, tr(kNatronNodeKnobExportPyPlugGroupLabel), 1, false );
        group = param;
        param->setName(kNatronNodeKnobExportPyPlugGroup);
        param->setSecret(true);
        param->setEvaluateOnChange(false);
        param->setDefaultValue(false);
        param->setIsPersistent(false);
        param->setAsDialog(true);
        if (mainPage) {
            mainPage->addKnob(param);
        }
        _imp->pyPlugExportDialog = param;
    }

    {
        KnobFilePtr param = AppManager::createKnob<KnobFile>(_imp->effect, tr(kNatronNodeKnobExportDialogFilePathLabel), 1, false);
        param->setName(kNatronNodeKnobExportDialogFilePath);
        param->setDialogType(KnobFile::eKnobFileDialogTypeSaveFile);
        {
            std::vector<std::string> filters;
            filters.push_back(NATRON_PRESETS_FILE_EXT);
            param->setDialogFilters(filters);
        }
        param->setEvaluateOnChange(false);
        param->setIsPersistent(false);
        param->setHintToolTip(tr(kNatronNodeKnobExportDialogFilePathHint));
        group->addKnob(param);
        _imp->pyPlugExportDialogFile = param;
    }
    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( _imp->effect, tr(kNatronNodeKnobExportDialogOkButtonLabel), 1, false );
        param->setName(kNatronNodeKnobExportDialogOkButton);
        param->setSecret(true);
        param->setAddNewLine(false);
        param->setEvaluateOnChange(false);
        param->setSpacingBetweenItems(3);
        param->setIsPersistent(false);
        group->addKnob(param);
        _imp->pyPlugExportDialogOkButton = param;
    }
    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( _imp->effect, tr(kNatronNodeKnobExportDialogCancelButtonLabel), 1, false );
        param->setName(kNatronNodeKnobExportDialogCancelButton);
        param->setSecret(true);
        param->setEvaluateOnChange(false);
        param->setSpacingBetweenItems(3);
        param->setIsPersistent(false);
        group->addKnob(param);
        _imp->pyPlugExportDialogCancelButton = param;
    }
}

void
Node::createPyPlugPage()
{
    PluginPtr pyPlug = _imp->pyPlugHandle.lock();
    KnobPagePtr page = AppManager::createKnob<KnobPage>(_imp->effect, tr(kPyPlugPageParamLabel), 1, false);
    page->setName(kPyPlugPageParamName);
    page->setSecret(true);
    _imp->pyPlugPage = page;

    {
        KnobStringPtr param = AppManager::createKnob<KnobString>(_imp->effect, tr(kNatronNodeKnobPyPlugPluginIDLabel), 1, false);
        param->setName(kNatronNodeKnobPyPlugPluginID);
        if (pyPlug) {
            param->setValue(pyPlug->getPluginID());
        }
        param->setEvaluateOnChange(false);
        param->setHintToolTip(tr(kNatronNodeKnobPyPlugPluginIDHint));
        page->addKnob(param);
        _imp->pyPlugIDKnob = param;
    }
    {
        KnobStringPtr param = AppManager::createKnob<KnobString>(_imp->effect, tr(kNatronNodeKnobPyPlugPluginLabelLabel), 1, false);
        param->setName(kNatronNodeKnobPyPlugPluginLabel);
        if (pyPlug) {
            param->setValue(pyPlug->getPluginLabel());
        }
        param->setEvaluateOnChange(false);
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginLabelHint));
        page->addKnob(param);
        _imp->pyPlugLabelKnob = param;
    }
    {
        KnobStringPtr param = AppManager::createKnob<KnobString>(_imp->effect, tr(kNatronNodeKnobPyPlugPluginGroupingLabel), 1, false);
        param->setName(kNatronNodeKnobPyPlugPluginGrouping);
        if (pyPlug) {
            param->setValue(pyPlug->getGroupingString());
        } else {
            param->setValue("PyPlugs");
        }
        param->setEvaluateOnChange(false);
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginGroupingHint));
        page->addKnob(param);
        _imp->pyPlugGroupingKnob = param;
    }
    {
        KnobStringPtr param = AppManager::createKnob<KnobString>(_imp->effect, tr(kNatronNodeKnobPyPlugPluginDescriptionLabel), 1, false);
        param->setName(kNatronNodeKnobPyPlugPluginDescription);
        param->setEvaluateOnChange(false);
        param->setAsMultiLine();
        if (pyPlug) {
            param->setValue(pyPlug->getProperty<std::string>(kNatronPluginPropDescription));
        }
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginDescriptionHint));
        param->setAddNewLine(false);
        page->addKnob(param);
        _imp->pyPlugDescKnob = param;
    }
    {
        KnobBoolPtr param = AppManager::createKnob<KnobBool>(_imp->effect, tr(kNatronNodeKnobPyPlugPluginDescriptionIsMarkdownLabel), 1, false);
        param->setName(kNatronNodeKnobPyPlugPluginDescriptionIsMarkdown);
        param->setEvaluateOnChange(false);
        if (pyPlug) {
            param->setValue(pyPlug->getProperty<bool>(kNatronPluginPropDescriptionIsMarkdown));
        }
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginDescriptionIsMarkdownHint));
        page->addKnob(param);
        _imp->pyPlugDescIsMarkdownKnob = param;
    }
    {
        KnobIntPtr param = AppManager::createKnob<KnobInt>(_imp->effect, tr(kNatronNodeKnobPyPlugPluginVersionLabel), 2, false);
        param->setName(kNatronNodeKnobPyPlugPluginVersion);
        param->setEvaluateOnChange(false);
        param->setDimensionName(DimIdx(0), "Major");
        param->setDimensionName(DimIdx(1), "Minor");
        if (pyPlug) {
            param->setValue((int)pyPlug->getProperty<unsigned int>(kNatronPluginPropVersion, 0));
            param->setValue((int)pyPlug->getProperty<unsigned int>(kNatronPluginPropVersion, 1), ViewSetSpec::all(), DimSpec(1));
        }
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginVersionHint));
        page->addKnob(param);
        _imp->pyPlugVersionKnob = param;
    }
    {
        KnobIntPtr param = AppManager::createKnob<KnobInt>(_imp->effect, tr(kNatronNodeKnobPyPlugPluginShortcutLabel), 2, false);
        param->setName(kNatronNodeKnobPyPlugPluginShortcut);
        param->setEvaluateOnChange(false);
        param->setAsShortcutKnob(true);
        if (pyPlug) {
            param->setValue(pyPlug->getProperty<int>(kNatronPluginPropShortcut, 0));
            param->setValue(pyPlug->getProperty<int>(kNatronPluginPropShortcut, 1), ViewSetSpec::all(), DimSpec(1));
        }
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginShortcutHint));
        page->addKnob(param);
        _imp->pyPlugShortcutKnob = param;
    }
    {
        KnobFilePtr param = AppManager::createKnob<KnobFile>(_imp->effect, tr(kNatronNodeKnobPyPlugPluginCallbacksPythonScriptLabel), 1, false);
        param->setName(kNatronNodeKnobPyPlugPluginCallbacksPythonScript);
        param->setEvaluateOnChange(false);
        if (pyPlug) {
            param->setValue(pyPlug->getProperty<std::string>(kNatronPluginPropPyPlugExtScriptFile));
        }
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginCallbacksPythonScriptHint));
        page->addKnob(param);
        _imp->pyPlugExtPythonScript = param;
    }
    {
        KnobFilePtr param = AppManager::createKnob<KnobFile>(_imp->effect, tr(kNatronNodeKnobPyPlugPluginIconFileLabel), 1, false);
        param->setName(kNatronNodeKnobPyPlugPluginIconFile);
        param->setEvaluateOnChange(false);
        if (pyPlug) {
            param->setValue(pyPlug->getProperty<std::string>(kNatronPluginPropIconFilePath));
        }
        param->setHintToolTip( tr(kNatronNodeKnobPyPlugPluginIconFileHint));
        page->addKnob(param);
        _imp->pyPlugIconKnob = param;
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>(_imp->effect, tr(kNatronNodeKnobExportPyPlugButtonLabel), 1, false);
        param->setName(kNatronNodeKnobExportPyPlugButton);
        param->setEvaluateOnChange(false);
        param->setHintToolTip( tr("Click to export this group to a PyPlug file (.%1)").arg(QLatin1String(NATRON_PRESETS_FILE_EXT)));
        page->addKnob(param);
        _imp->pyPlugExportButtonKnob = param;
    }

}

void
Node::createPythonPage()
{
    KnobPagePtr pythonPage = AppManager::createKnob<KnobPage>(_imp->effect, tr("Python"), 1, false);
    KnobStringPtr beforeFrameRender =  AppManager::createKnob<KnobString>(_imp->effect, tr("Before frame render"), 1, false);

    beforeFrameRender->setName("beforeFrameRender");
    beforeFrameRender->setAnimationEnabled(false);
    beforeFrameRender->setHintToolTip( tr("Add here the name of a Python defined function that will be called before rendering "
                                          "any frame.\n "
                                          "The signature of the callback is: callback(frame, thisNode, app) where:\n"
                                          "- frame: the frame to be rendered\n"
                                          "- thisNode: points to the writer node\n"
                                          "- app: points to the current application instance") );
    pythonPage->addKnob(beforeFrameRender);
    _imp->beforeFrameRender = beforeFrameRender;

    KnobStringPtr beforeRender =  AppManager::createKnob<KnobString>(_imp->effect, tr("Before render"), 1, false);
    beforeRender->setName("beforeRender");
    beforeRender->setAnimationEnabled(false);
    beforeRender->setHintToolTip( tr("Add here the name of a Python defined function that will be called once when "
                                     "starting rendering.\n "
                                     "The signature of the callback is: callback(thisNode, app) where:\n"
                                     "- thisNode: points to the writer node\n"
                                     "- app: points to the current application instance") );
    pythonPage->addKnob(beforeRender);
    _imp->beforeRender = beforeRender;

    KnobStringPtr afterFrameRender =  AppManager::createKnob<KnobString>(_imp->effect, tr("After frame render"), 1, false);
    afterFrameRender->setName("afterFrameRender");
    afterFrameRender->setAnimationEnabled(false);
    afterFrameRender->setHintToolTip( tr("Add here the name of a Python defined function that will be called after rendering "
                                         "any frame.\n "
                                         "The signature of the callback is: callback(frame, thisNode, app) where:\n"
                                         "- frame: the frame that has been rendered\n"
                                         "- thisNode: points to the writer node\n"
                                         "- app: points to the current application instance") );
    pythonPage->addKnob(afterFrameRender);
    _imp->afterFrameRender = afterFrameRender;

    KnobStringPtr afterRender =  AppManager::createKnob<KnobString>(_imp->effect, tr("After render"), 1, false);
    afterRender->setName("afterRender");
    afterRender->setAnimationEnabled(false);
    afterRender->setHintToolTip( tr("Add here the name of a Python defined function that will be called once when the rendering "
                                    "is finished.\n "
                                    "The signature of the callback is: callback(aborted, thisNode, app) where:\n"
                                    "- aborted: True if the render ended because it was aborted, False upon completion\n"
                                    "- thisNode: points to the writer node\n"
                                    "- app: points to the current application instance") );
    pythonPage->addKnob(afterRender);
    _imp->afterRender = afterRender;
} // Node::createPythonPage

KnobDoublePtr
Node::getOrCreateHostMixKnob(const KnobPagePtr& mainPage)
{
    KnobDoublePtr mixKnob = AppManager::checkIfKnobExistsWithNameOrCreate<KnobDouble>(_imp->effect, kHostMixingKnobName, tr("Mix"));
    mixKnob->setDeclaredByPlugin(false);
    mixKnob->setName(kHostMixingKnobName);
    mixKnob->setHintToolTip( tr("Mix between the source image at 0 and the full effect at 1.") );
    mixKnob->setRange(0., 1.);
    mixKnob->setDefaultValue(1.);
    if (mainPage) {
        mainPage->addKnob(mixKnob);
    }
    _imp->mixWithSource = mixKnob;
    return mixKnob;
}

void
Node::createMaskSelectors(const std::vector<std::pair<bool, bool> >& hasMaskChannelSelector,
                          const std::vector<std::string>& inputLabels,
                          const KnobPagePtr& mainPage,
                          bool addNewLineOnLastMask,
                          KnobIPtr* lastKnobCreated)
{
    assert( hasMaskChannelSelector.size() == inputLabels.size() );

    for (std::size_t i = 0; i < hasMaskChannelSelector.size(); ++i) {
        if (!hasMaskChannelSelector[i].first) {
            continue;
        }


        MaskSelector sel;
        KnobBoolPtr enabled = AppManager::createKnob<KnobBool>(_imp->effect, inputLabels[i], 1, false);

        enabled->setDefaultValue(false);
        enabled->setAddNewLine(false);
        if (hasMaskChannelSelector[i].second) {
            std::string enableMaskName(std::string(kEnableMaskKnobName) + "_" + inputLabels[i]);
            enabled->setName(enableMaskName);
            enabled->setHintToolTip( tr("Enable the mask to come from the channel named by the choice parameter on the right. "
                                        "Turning this off will act as though the mask was disconnected.") );
        } else {
            std::string enableMaskName(std::string(kEnableInputKnobName) + "_" + inputLabels[i]);
            enabled->setName(enableMaskName);
            enabled->setHintToolTip( tr("Enable the image to come from the channel named by the choice parameter on the right. "
                                        "Turning this off will act as though the input was disconnected.") );
        }
        enabled->setAnimationEnabled(false);
        if (mainPage) {
            mainPage->addKnob(enabled);
        }


        sel.enabled = enabled;

        KnobChoicePtr channel = AppManager::createKnob<KnobChoice>(_imp->effect, std::string(), 1, false);
        std::vector<std::string> choices;
        choices.push_back("None");

        const ImageComponents& rgba = ImageComponents::getRGBAComponents();
        const std::string& rgbaCompname = rgba.getComponentsGlobalName();
        const std::vector<std::string>& rgbaChannels = rgba.getComponentsNames();
        for (std::size_t c = 0; c < rgbaChannels.size(); ++c) {
            std::string option = rgbaCompname + '.' + rgbaChannels[c];
            choices.push_back(option);
        }

        channel->populateChoices(choices);
        channel->setDefaultValue(choices.size() - 1);
        channel->setAnimationEnabled(false);
        channel->setHintToolTip( tr("Use this channel from the original input to mix the output with the original input. "
                                    "Setting this to None is the same as disconnecting the input.") );
        if (hasMaskChannelSelector[i].second) {
            std::string channelMaskName(std::string(kMaskChannelKnobName) + "_" + inputLabels[i]);
            channel->setName(channelMaskName);
        } else {
            std::string channelMaskName(std::string(kInputChannelKnobName) + "_" + inputLabels[i]);
            channel->setName(channelMaskName);
        }
        sel.channel = channel;
        if (mainPage) {
            mainPage->addKnob(channel);
        }

        //Make sure the first default param in the vector is MaskInvert

        if (!addNewLineOnLastMask) {
            //If there is a MaskInvert parameter, make it on the same line as the Mask channel parameter
            channel->setAddNewLine(false);
        }
        if (!*lastKnobCreated) {
            *lastKnobCreated = enabled;
        }

        _imp->maskSelectors[i] = sel;
    } // for (int i = 0; i < inputsCount; ++i) {
} // Node::createMaskSelectors


KnobPagePtr
Node::getOrCreateMainPage()
{
    const KnobsVec & knobs = _imp->effect->getKnobs();
    KnobPagePtr mainPage;

    for (std::size_t i = 0; i < knobs.size(); ++i) {
        KnobPagePtr p = toKnobPage(knobs[i]);
        if ( p && (p->getName() != kInfoPageParamName) &&
             (p->getName() != kNodePageParamName) ) {
            mainPage = p;
            break;
        }
    }
    if (!mainPage) {
        mainPage = AppManager::createKnob<KnobPage>( _imp->effect, tr("Settings") );
    }

    return mainPage;
}

void
Node::createLabelKnob(const KnobPagePtr& settingsPage,
                      const std::string& label)
{
    KnobStringPtr nodeLabel = AppManager::createKnob<KnobString>(_imp->effect, label, 1, false);

    assert(nodeLabel);
    nodeLabel->setName(kUserLabelKnobName);
    nodeLabel->setAnimationEnabled(false);
    nodeLabel->setEvaluateOnChange(false);
    nodeLabel->setAsMultiLine();
    nodeLabel->setUsesRichText(true);
    nodeLabel->setHintToolTip( tr("This label gets appended to the node name on the node graph.") );
    settingsPage->addKnob(nodeLabel);
    _imp->nodeLabelKnob = nodeLabel;
}

void
Node::findOrCreateChannelEnabled()
{
    //Try to find R,G,B,A parameters on the plug-in, if found, use them, otherwise create them
    static const std::string channelLabels[4] = {kNatronOfxParamProcessRLabel, kNatronOfxParamProcessGLabel, kNatronOfxParamProcessBLabel, kNatronOfxParamProcessALabel};
    static const std::string channelNames[4] = {kNatronOfxParamProcessR, kNatronOfxParamProcessG, kNatronOfxParamProcessB, kNatronOfxParamProcessA};
    static const std::string channelHints[4] = {kNatronOfxParamProcessRHint, kNatronOfxParamProcessGHint, kNatronOfxParamProcessBHint, kNatronOfxParamProcessAHint};
    KnobBoolPtr foundEnabled[4];
    const KnobsVec & knobs = _imp->effect->getKnobs();

    for (int i = 0; i < 4; ++i) {
        KnobBoolPtr enabled;
        for (std::size_t j = 0; j < knobs.size(); ++j) {
            if (knobs[j]->getOriginalName() == channelNames[i]) {
                foundEnabled[i] = toKnobBool(knobs[j]);
                break;
            }
        }
    }

    bool foundAll = foundEnabled[0] && foundEnabled[1] && foundEnabled[2] && foundEnabled[3];
    bool isWriter = _imp->effect->isWriter();

    KnobPagePtr mainPage;
    if (foundAll) {
        for (int i = 0; i < 4; ++i) {
            // Writers already have their checkboxes places correctly
            if (!isWriter) {
                if (!mainPage) {
                    mainPage = getOrCreateMainPage();
                }
                if (foundEnabled[i]->getParentKnob() == mainPage) {
                    //foundEnabled[i]->setAddNewLine(i == 3);
                    mainPage->removeKnob(foundEnabled[i]);
                    mainPage->insertKnob(i, foundEnabled[i]);
                }
            }
            _imp->enabledChan[i] = foundEnabled[i];
        }
    }

    bool pluginDefaultPref[4];
    _imp->hostChannelSelectorEnabled = _imp->effect->isHostChannelSelectorSupported(&pluginDefaultPref[0], &pluginDefaultPref[1], &pluginDefaultPref[2], &pluginDefaultPref[3]);


    if (_imp->hostChannelSelectorEnabled) {
        if (foundAll) {
            std::cerr << getScriptName_mt_safe() << ": WARNING: property " << kNatronOfxImageEffectPropChannelSelector << " is different of " << kOfxImageComponentNone << " but uses its own checkboxes" << std::endl;
        } else {
            if (!mainPage) {
                mainPage = getOrCreateMainPage();
            }

            //Create the selectors
            for (int i = 0; i < 4; ++i) {
                foundEnabled[i] =  AppManager::createKnob<KnobBool>(_imp->effect, channelLabels[i], 1, false);
                foundEnabled[i]->setName(channelNames[i]);
                foundEnabled[i]->setAnimationEnabled(false);
                foundEnabled[i]->setAddNewLine(i == 3);
                foundEnabled[i]->setDefaultValue(pluginDefaultPref[i]);
                foundEnabled[i]->setHintToolTip(channelHints[i]);
                mainPage->insertKnob(i, foundEnabled[i]);
                _imp->enabledChan[i] = foundEnabled[i];
            }
            foundAll = true;
        }
    }
    if ( !isWriter && foundAll && !getApp()->isBackground() ) {
        if (!mainPage) {
            mainPage = getOrCreateMainPage();
        }
        _imp->enabledChan[3].lock()->setAddNewLine(false);
        KnobStringPtr premultWarning = AppManager::createKnob<KnobString>(_imp->effect, std::string(), 1, false);
        premultWarning->setName("premultWarningKnob");
        premultWarning->setIconLabel("dialog-warning");
        premultWarning->setSecret(true);
        premultWarning->setAsLabel();
        premultWarning->setEvaluateOnChange(false);
        premultWarning->setIsPersistent(false);
        premultWarning->setHintToolTip( tr("The alpha checkbox is checked and the RGB "
                                           "channels in output are alpha-premultiplied. Any of the unchecked RGB channel "
                                           "may be incorrect because the alpha channel changed but their value did not. "
                                           "To fix this, either check all RGB channels (or uncheck alpha) or unpremultiply the "
                                           "input image first.").toStdString() );
        mainPage->insertKnob(4, premultWarning);
        _imp->premultWarning = premultWarning;
    }
} // Node::findOrCreateChannelEnabled

void
Node::createChannelSelectors(const std::vector<std::pair<bool, bool> >& hasMaskChannelSelector,
                             const std::vector<std::string>& inputLabels,
                             const KnobPagePtr& mainPage,
                             KnobIPtr* lastKnobBeforeAdvancedOption)
{
    ///Create input layer selectors
    for (std::size_t i = 0; i < inputLabels.size(); ++i) {
        if (!hasMaskChannelSelector[i].first) {
            _imp->createChannelSelector(i, inputLabels[i], false, mainPage, lastKnobBeforeAdvancedOption);
        }
    }
    ///Create output layer selectors
    _imp->createChannelSelector(-1, "Output", true, mainPage, lastKnobBeforeAdvancedOption);
}

void
Node::initializeDefaultKnobs(bool loadingSerialization, bool hasGUI)
{
    //Readers and Writers don't have default knobs since these knobs are on the ReadNode/WriteNode itself
    NodePtr ioContainer = getIOContainer();

    //Add the "Node" page
    KnobPagePtr settingsPage = AppManager::checkIfKnobExistsWithNameOrCreate<KnobPage>(_imp->effect, kNodePageParamName, tr(kNodePageParamLabel));
    settingsPage->setDeclaredByPlugin(false);

    //Create the "Label" knob
    BackdropPtr isBackdropNode = isEffectBackdrop();
    QString labelKnobLabel = isBackdropNode ? tr("Name label") : tr("Label");
    createLabelKnob( settingsPage, labelKnobLabel.toStdString() );

    if (isBackdropNode || isEffectStubNode()) {
        //backdrops just have a label
        return;
    }


    ///find in all knobs a page param to set this param into
    const KnobsVec & knobs = _imp->effect->getKnobs();

    findPluginFormatKnobs(knobs, loadingSerialization);
    findRightClickMenuKnob(knobs);

    KnobPagePtr mainPage;


    // Scan all inputs to find masks and get inputs labels
    //Pair hasMaskChannelSelector, isMask
    int inputsCount = getMaxInputCount();
    std::vector<std::pair<bool, bool> > hasMaskChannelSelector(inputsCount);
    std::vector<std::string> inputLabels(inputsCount);
    for (int i = 0; i < inputsCount; ++i) {
        inputLabels[i] = _imp->effect->getInputLabel(i);

        assert( i < (int)_imp->inputsComponents.size() );
        const std::list<ImageComponents>& inputSupportedComps = _imp->inputsComponents[i];
        bool isMask = _imp->effect->isInputMask(i);
        bool supportsOnlyAlpha = inputSupportedComps.size() == 1 && inputSupportedComps.front().getNumComponents() == 1;

        hasMaskChannelSelector[i].first = false;
        hasMaskChannelSelector[i].second = isMask;

        if ( isMask || supportsOnlyAlpha ) {
            hasMaskChannelSelector[i].first = true;
            if (!mainPage) {
                mainPage = getOrCreateMainPage();
            }
        }
    }


    // Create the Output Layer choice if needed plus input layers selectors
    KnobIPtr lastKnobBeforeAdvancedOption;
    bool requiresLayerShuffle = _imp->effect->getCreateChannelSelectorKnob();
    if (requiresLayerShuffle) {
        if (!mainPage) {
            mainPage = getOrCreateMainPage();
        }
        createChannelSelectors(hasMaskChannelSelector, inputLabels, mainPage, &lastKnobBeforeAdvancedOption);
    }


    findOrCreateChannelEnabled();

    ///Find in the plug-in the Mask/Mix related parameter to re-order them so it is consistent across nodes
    std::vector<std::pair<std::string, KnobIPtr > > foundPluginDefaultKnobsToReorder;
    foundPluginDefaultKnobsToReorder.push_back( std::make_pair( kOfxMaskInvertParamName, KnobIPtr() ) );
    foundPluginDefaultKnobsToReorder.push_back( std::make_pair( kOfxMixParamName, KnobIPtr() ) );
    ///Insert auto-added knobs before mask invert if found
    for (std::size_t i = 0; i < knobs.size(); ++i) {
        for (std::size_t j = 0; j < foundPluginDefaultKnobsToReorder.size(); ++j) {
            if (knobs[i]->getName() == foundPluginDefaultKnobsToReorder[j].first) {
                foundPluginDefaultKnobsToReorder[j].second = knobs[i];
            }

        }
    }



    assert(foundPluginDefaultKnobsToReorder.size() > 0 && foundPluginDefaultKnobsToReorder[0].first == kOfxMaskInvertParamName);

    createMaskSelectors(hasMaskChannelSelector, inputLabels, mainPage, !foundPluginDefaultKnobsToReorder[0].second.get(), &lastKnobBeforeAdvancedOption);


    //Create the host mix if needed
    if ( _imp->effect->isHostMixingEnabled() ) {
        if (!mainPage) {
            mainPage = getOrCreateMainPage();
        }
        getOrCreateHostMixKnob(mainPage);
    }


    /*
     * Reposition the MaskInvert and Mix parameters declared by the plug-in
     */

    for (std::size_t i = 0; i < foundPluginDefaultKnobsToReorder.size(); ++i) {
        if (foundPluginDefaultKnobsToReorder[i].second) {
            if (!mainPage) {
                mainPage = getOrCreateMainPage();
            }
            if (foundPluginDefaultKnobsToReorder[i].second->getParentKnob() == mainPage) {
                mainPage->removeKnob( foundPluginDefaultKnobsToReorder[i].second);
                mainPage->addKnob(foundPluginDefaultKnobsToReorder[i].second);
            }
        }
    }


    if (lastKnobBeforeAdvancedOption && mainPage) {
        
        KnobsVec mainPageChildren = mainPage->getChildren();
        int i = 0;
        for (KnobsVec::iterator it = mainPageChildren.begin(); it != mainPageChildren.end(); ++it, ++i) {
            if (*it == lastKnobBeforeAdvancedOption) {
                if (i > 0) {
                    KnobsVec::iterator prev = it;
                    --prev;
                    if ( !toKnobSeparator(*prev) ) {
                        KnobSeparatorPtr sep = AppManager::createKnob<KnobSeparator>(_imp->effect, std::string(), 1, false);
                        sep->setName("advancedSep");
                        mainPage->insertKnob(i, sep);
                    }
                }

                break;
            }
        }
    }


    createNodePage(settingsPage);

    NodeGroupPtr isGrpNode = isEffectNodeGroup();
    if (!isGrpNode) {
        createInfoPage();
    } else {
        if (isGrpNode->isSubGraphUserVisible()) {
            createPyPlugPage();

            if (hasGUI) {
                createPyPlugExportGroup();
            }
        }
    }


    if (_imp->effect->isWriter()
        && !ioContainer) {
        //Create a frame step parameter for writers, and control it in OutputSchedulerThread.cpp

        if (!mainPage) {
            mainPage = getOrCreateMainPage();
        }

        KnobButtonPtr renderButton = AppManager::createKnob<KnobButton>(_imp->effect, tr("Render"), 1, false);
        renderButton->setHintToolTip( tr("Starts rendering the specified frame range.") );
        renderButton->setAsRenderButton();
        renderButton->setName("startRender");
        renderButton->setEvaluateOnChange(false);
        _imp->renderButton = renderButton;
        mainPage->addKnob(renderButton);

        createPythonPage();
    }
} // Node::initializeDefaultKnobs

void
Node::initializeKnobs(bool loadingSerialization, bool hasGUI)
{
    ////Only called by the main-thread


    _imp->effect->beginChanges();

    assert( QThread::currentThread() == qApp->thread() );

    InitializeKnobsFlag_RAII __isInitializingKnobsFlag__(getEffectInstance());

    //Initialize plug-in knobs
    _imp->effect->initializeKnobsPublic();

    if ( _imp->effect->getMakeSettingsPanel() ) {
        //initialize default knobs added by Natron
        initializeDefaultKnobs(loadingSerialization, hasGUI);
    }

    declarePythonKnobs();

    KnobItemsTablePtr table = _imp->effect->getItemsTable();
    if (table) {
        table->declareItemsToPython();
    }
    
    _imp->effect->endChanges();

    Q_EMIT knobsInitialized();
} // initializeKnobs



int
Node::getFrameStepKnobValue() const
{
    KnobIPtr knob = getKnobByName(kNatronWriteParamFrameStep);
    if (!knob) {
        return 1;
    }
    KnobIntPtr k = toKnobInt(knob);

    if (!k) {
        return 1;
    } else {
        int v = k->getValue();

        return std::max(1, v);
    }
}

bool
Node::handleFormatKnob(const KnobIPtr& knob)
{
    KnobChoicePtr choice = _imp->pluginFormatKnobs.formatChoice.lock();

    if (!choice) {
        return false;
    }

    if (knob != choice) {
        return false;
    }
    if (choice->getIsSecret()) {
        return true;
    }
    int curIndex = choice->getValue();
    Format f;
    if ( !getApp()->getProject()->getProjectFormatAtIndex(curIndex, &f) ) {
        assert(false);

        return true;
    }

    KnobIntPtr size = _imp->pluginFormatKnobs.size.lock();
    KnobDoublePtr par = _imp->pluginFormatKnobs.par.lock();
    assert(size && par);

    _imp->effect->beginChanges();
    size->blockValueChanges();
    std::vector<int> values(2);
    values[0] = f.width();
    values[1] = f.height();
    size->setValueAcrossDimensions(values);
    size->unblockValueChanges();
    par->blockValueChanges();
    par->setValue( f.getPixelAspectRatio() );
    par->unblockValueChanges();

    _imp->effect->endChanges();

    return true;
}

void
Node::refreshFormatParamChoice(const std::vector<std::string>& entries,
                               int defValue,
                               bool loadingProject)
{
    KnobChoicePtr choice = _imp->pluginFormatKnobs.formatChoice.lock();

    if (!choice) {
        return;
    }
    int curIndex = choice->getValue();
    choice->populateChoices(entries);
    choice->beginChanges();

    choice->setDefaultValueWithoutApplying(defValue);
    // We don't want to serialize the default value even if it changed, because it will be restored by the project
    choice->setCurrentDefaultValueAsInitialValue();

    if (!loadingProject) {
        //changedKnob was not called because we are initializing knobs
        handleFormatKnob(choice);
    } else {
        if ( curIndex < (int)entries.size() ) {
            choice->setValue(curIndex);
        }
    }

    choice->endChanges();
}

void
Node::refreshPreviewsAfterProjectLoad()
{
    computePreviewImage( getApp()->getTimeLine()->currentFrame() );
    Q_EMIT s_refreshPreviewsAfterProjectLoadRequested();
}

QString
Node::makeDocumentation(bool genHTML) const
{
    QString ret;
    QString markdown;
    QTextStream ts(&ret);
    QTextStream ms(&markdown);

    QString pluginID, pluginLabel, pluginDescription, pluginIcon;
    int majorVersion = getMajorVersion();
    int minorVersion = getMinorVersion();
    std::vector<std::string> pluginGroup;
    bool pluginDescriptionIsMarkdown = false;
    QVector<QStringList> inputs;
    QVector<QStringList> items;

    {
        PluginPtr plugin = getPlugin();
        assert(plugin);

        pluginID = QString::fromUtf8(plugin->getPluginID().c_str());
        pluginLabel =  QString::fromUtf8(plugin->getPluginLabel().c_str());
        pluginDescription =  QString::fromUtf8( plugin->getProperty<std::string>(kNatronPluginPropDescription).c_str() );
        pluginIcon = QString::fromUtf8(plugin->getProperty<std::string>(kNatronPluginPropIconFilePath).c_str());
        pluginGroup = plugin->getPropertyN<std::string>(kNatronPluginPropGrouping);
        pluginDescriptionIsMarkdown = plugin->getProperty<bool>(kNatronPluginPropDescriptionIsMarkdown);


        for (int i = 0; i < _imp->effect->getMaxInputCount(); ++i) {
            QStringList input;
            QString optional = _imp->effect->isInputOptional(i) ? tr("Yes") : tr("No");
            input << QString::fromStdString( _imp->effect->getInputLabel(i) ) << QString::fromStdString( _imp->effect->getInputHint(i) ) << optional;
            inputs.push_back(input);

            // Don't show more than doc for 4 inputs otherwise it will just clutter the page
            if (i == 3) {
                break;
            }
        }
    }

    // check for plugin icon
    QString pluginIconUrl;
    if ( !pluginIcon.isEmpty() ) {
        QFile iconFile(pluginIcon);
        if ( iconFile.exists() ) {
            if (genHTML) {
                pluginIconUrl.append( QString::fromUtf8("/LOCAL_FILE/") );
                pluginIconUrl.append(pluginIcon);
                pluginIconUrl.replace( QString::fromUtf8("\\"), QString::fromUtf8("/") );
            } else {
                pluginIconUrl.append(pluginID);
                pluginIconUrl.append(QString::fromUtf8(".png"));
            }
        }
    }

    // check for extra markdown file
    QString extraMarkdown;
    QString pluginMD = pluginIcon;
    pluginMD.replace( QString::fromUtf8(".png"), QString::fromUtf8(".md") );
    QFile pluginMarkdownFile(pluginMD);
    if ( pluginMarkdownFile.exists() ) {
        if ( pluginMarkdownFile.open(QIODevice::ReadOnly | QIODevice::Text) ) {
            extraMarkdown = QString::fromUtf8( pluginMarkdownFile.readAll() );
            pluginMarkdownFile.close();
        }
    }

    // generate knobs info
    KnobsVec knobs = getEffectInstance()->getKnobs_mt_safe();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {

        if ( (*it)->getIsSecret() ) {
            continue;
        }

        if (!(*it)->isDeclaredByPlugin()) {
            continue;
        }
        QString knobScriptName = QString::fromUtf8( (*it)->getName().c_str() );
        QString knobLabel = QString::fromUtf8( (*it)->getLabel().c_str() );
        QString knobHint = QString::fromUtf8( (*it)->getHintToolTip().c_str() );

        QString defValuesStr, knobType;
        std::vector<std::pair<QString, QString> > dimsDefaultValueStr;
        KnobIntPtr isInt = toKnobInt(*it);
        KnobChoicePtr isChoice = toKnobChoice(*it);
        KnobBoolPtr isBool = toKnobBool(*it);
        KnobDoublePtr isDbl = toKnobDouble(*it);
        KnobStringPtr isString = toKnobString(*it);
        KnobSeparatorPtr isSep = toKnobSeparator(*it);
        KnobButtonPtr isBtn = toKnobButton(*it);
        KnobParametricPtr isParametric = toKnobParametric(*it);
        KnobGroupPtr isGroup = toKnobGroup(*it);
        KnobPagePtr isPage = toKnobPage(*it);
        KnobColorPtr isColor = toKnobColor(*it);

        if (isInt) {
            knobType = tr("Integer");
        } else if (isChoice) {
            knobType = tr("Choice");
        } else if (isBool) {
            knobType = tr("Boolean");
        } else if (isDbl) {
            knobType = tr("Double");
        } else if (isString) {
            knobType = tr("String");
        } else if (isSep) {
            knobType = tr("Seperator");
        } else if (isBtn) {
            knobType = tr("Button");
        } else if (isParametric) {
            knobType = tr("Parametric");
        } else if (isGroup) {
            knobType = tr("Group");
        } else if (isPage) {
            knobType = tr("Page");
        } else if (isColor) {
            knobType = tr("Color");
        } else {
            knobType = tr("N/A");
        }

        if (!isGroup && !isPage) {
            for (int i = 0; i < (*it)->getNDimensions(); ++i) {
                QString valueStr;

                if (!isBtn && !isSep && !isParametric) {
                    if (isChoice) {
                        int index = isChoice->getDefaultValue(DimIdx(i));
                        std::vector<std::string> entries = isChoice->getEntries();
                        if ( (index >= 0) && ( index < (int)entries.size() ) ) {
                            valueStr = QString::fromUtf8( entries[index].c_str() );
                        }
                        std::vector<std::string> entriesHelp = isChoice->getEntriesHelp();
                        if ( entries.size() == entriesHelp.size() ) {
                            knobHint.append( QString::fromUtf8("\n\n") );
                            for (size_t i = 0; i < entries.size(); i++) {
                                QString entry = QString::fromUtf8( entries[i].c_str() );
                                QString entryHelp = QString::fromUtf8( entriesHelp[i].c_str() );
                                if (!entry.isEmpty() && !entryHelp.isEmpty() ) {
                                    knobHint.append( QString::fromUtf8("**%1**: %2\n").arg(entry).arg(entryHelp) );
                                }
                            }
                        }
                    } else if (isInt) {
                        valueStr = QString::number( isInt->getDefaultValue(DimIdx(i)) );
                    } else if (isDbl) {
                        valueStr = QString::number( isDbl->getDefaultValue(DimIdx(i)) );
                    } else if (isBool) {
                        valueStr = isBool->getDefaultValue(DimIdx(i)) ? tr("On") : tr("Off");
                    } else if (isString) {
                        valueStr = QString::fromUtf8( isString->getDefaultValue(DimIdx(i)).c_str() );
                    } else if (isColor) {
                        valueStr = QString::number( isColor->getDefaultValue(DimIdx(i)) );
                    }
                }

                dimsDefaultValueStr.push_back( std::make_pair(QString::fromUtf8( (*it)->getDimensionName(DimIdx(i)).c_str() ), valueStr) );
            }

            for (std::size_t i = 0; i < dimsDefaultValueStr.size(); ++i) {
                if ( !dimsDefaultValueStr[i].second.isEmpty() ) {
                    if (dimsDefaultValueStr.size() > 1) {
                        defValuesStr.append(dimsDefaultValueStr[i].first);
                        defValuesStr.append( QString::fromUtf8(": ") );
                    }
                    defValuesStr.append(dimsDefaultValueStr[i].second);
                    if (i < dimsDefaultValueStr.size() - 1) {
                        defValuesStr.append( QString::fromUtf8(" ") );
                    }
                }
            }
            if ( defValuesStr.isEmpty() ) {
                defValuesStr = tr("N/A");
            }
        }

        if (!isPage && !isSep && !isGroup) {
            QStringList row;
            row << knobLabel << knobScriptName << knobType << defValuesStr << knobHint;
            items.append(row);
        }
    } // for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {


    // generate plugin info
    ms << pluginLabel << "\n==========\n\n";
    if (!pluginIconUrl.isEmpty()) {
        ms << "![](" << pluginIconUrl << ")\n\n";
    }
    ms << tr("*This documentation is for version %2.%3 of %1.*").arg(pluginLabel).arg(majorVersion).arg(minorVersion) << "\n\n";

    if (!pluginDescriptionIsMarkdown) {
        if (genHTML) {
            pluginDescription = NATRON_NAMESPACE::convertFromPlainText(pluginDescription, NATRON_NAMESPACE::WhiteSpaceNormal);

            // replace URLs with links
            QRegExp re( QString::fromUtf8("((http|ftp|https)://([\\w_-]+(?:(?:\\.[\\w_-]+)+))([\\w.,@?^=%&:/~+#-]*[\\w@?^=%&/~+#-])?)") );
            pluginDescription.replace( re, QString::fromUtf8("<a href=\"\\1\">\\1</a>") );
        } else {
            pluginDescription.replace( QString::fromUtf8("\n"), QString::fromUtf8("\n\n") );
        }
    }

    ms << pluginDescription << "\n\n";

    // create markdown table
    ms << tr("Inputs") << "\n----------\n\n";
    ms << tr("Input") << " | " << tr("Description") << " | " << tr("Optional") << "\n";
    ms << "--- | --- | ---\n";
    if (inputs.size() > 0) {
        Q_FOREACH(const QStringList &input, inputs) {
            QString inputName = input.at(0);
            inputName.replace(QString::fromUtf8("\n"),QString::fromUtf8("<br />"));
            if (inputName.isEmpty()) {
                inputName = QString::fromUtf8("&nbsp;");
            }

            QString inputDesc = input.at(1);
            inputDesc.replace(QString::fromUtf8("\n"),QString::fromUtf8("<br />"));
            if (inputDesc.isEmpty()) {
                inputDesc = QString::fromUtf8("&nbsp;");
            }

            QString inputOpt = input.at(2);
            if (inputOpt.isEmpty()) {
                inputOpt = QString::fromUtf8("&nbsp;");
            }

            ms << inputName << " | " << inputDesc << " | " << inputOpt << "\n";
        }
    }
    ms << tr("Controls") << "\n----------\n\n";
    ms << tr("Label (UI Name)") << " | " << tr("Script-Name") << " | " <<tr("Type") << " | " << tr("Default-Value") << " | " << tr("Function") << "\n";
    ms << "--- | --- | --- | --- | ---\n";
    if (items.size() > 0) {
        Q_FOREACH(const QStringList &item, items) {
            QString itemLabel = item.at(0);
            itemLabel.replace(QString::fromUtf8("\n"),QString::fromUtf8("<br />"));
            if (itemLabel.isEmpty()) {
                itemLabel = QString::fromUtf8("&nbsp;");
            }

            QString itemScript = item.at(1);
            itemScript.replace(QString::fromUtf8("\n"),QString::fromUtf8("<br />"));
            if (itemScript.isEmpty()) {
                itemScript = QString::fromUtf8("&nbsp;");
            }

            QString itemType = item.at(2);
            if (itemType.isEmpty()) {
                itemType = QString::fromUtf8("&nbsp;");
            }

            QString itemDefault = item.at(3);
            itemDefault.replace(QString::fromUtf8("\n"),QString::fromUtf8("<br />"));
            if (itemDefault.isEmpty()) {
                itemDefault = QString::fromUtf8("&nbsp;");
            }

            QString itemFunction = item.at(4);
            itemFunction.replace(QString::fromUtf8("\n"),QString::fromUtf8("<br />"));
            if (itemFunction.isEmpty()) {
                itemFunction = QString::fromUtf8("&nbsp;");
            }

            ms << itemLabel << " | " << itemScript << " | " << itemType << " | " << itemDefault << " | " << itemFunction << "\n";
        }
    }

    // add extra markdown if available
    if ( !extraMarkdown.isEmpty() ) {
        ms << "\n\n";
        ms << extraMarkdown;
    }

    // output
    if (genHTML) {
        ts << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">";
        ts << "<html><head>";
        ts << "<title>" << pluginLabel << " - NATRON_DOCUMENTATION</title>";
        ts << "<link rel=\"stylesheet\" href=\"_static/default.css\" type=\"text/css\" /><link rel=\"stylesheet\" href=\"_static/style.css\" type=\"text/css\" /><script type=\"text/javascript\" src=\"_static/jquery.js\"></script><script type=\"text/javascript\" src=\"_static/dropdown.js\"></script>";
        ts << "</head><body>";
        ts << "<div class=\"related\"><h3>" << tr("Navigation") << "</h3><ul>";
        ts << "<li><a href=\"/index.html\">NATRON_DOCUMENTATION</a> &raquo;</li>";
        ts << "<li><a href=\"/_group.html\">" << tr("Reference Guide") << "</a> &raquo;</li>";
        if ( !pluginGroup.empty() ) {
            const QString group = QString::fromUtf8(pluginGroup[0].c_str());
            if (!group.isEmpty()) {
                ts << "<li><a href=\"/_group.html?id=" << group << "\">" << group << "</a> &raquo;</li>";
            }
        }
        ts << "</ul></div>";
        ts << "<div class=\"document\"><div class=\"documentwrapper\"><div class=\"body\"><div class=\"section\">";
        QString html = Markdown::convert2html(markdown);
        ts << Markdown::fixNodeHTML(html);
        ts << "</div></div></div><div class=\"clearer\"></div></div><div class=\"footer\"></div></body></html>";
    } else {
        ts << markdown;
    }

    return ret;
} // Node::makeDocumentation

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
Node::onSetSupportRenderScaleMaybeSet(int support)
{
    if ( (EffectInstance::SupportsEnum)support == EffectInstance::eSupportsYes ) {
        KnobBoolPtr b = _imp->useFullScaleImagesWhenRenderScaleUnsupported.lock();
        if (b) {
            b->setSecret(true);
        }
    }
}

bool
Node::useScaleOneImagesWhenRenderScaleSupportIsDisabled() const
{
    KnobBoolPtr b =  _imp->useFullScaleImagesWhenRenderScaleUnsupported.lock();

    return b ? b->getValue() : false;
}

void
Node::beginEditKnobs()
{
    _imp->effect->beginEditKnobs();
}


EffectInstancePtr
Node::getEffectInstance() const
{
    ///Thread safe as it never changes
    return _imp->effect;
}

void
Node::hasViewersConnectedInternal(std::list<ViewerInstancePtr >* viewers,
                                 std::list<const Node*>* markedNodes) const
{

    if (std::find(markedNodes->begin(), markedNodes->end(), this) != markedNodes->end()) {
        return;
    }

    markedNodes->push_back(this);
    ViewerInstancePtr thisViewer = toViewerInstance( _imp->effect);

    if (thisViewer) {
        viewers->push_back(thisViewer);
    } else {
        NodesList outputs;
        getOutputsWithGroupRedirection(outputs);

        for (NodesList::iterator it = outputs.begin(); it != outputs.end(); ++it) {
            assert(*it);
            (*it)->hasViewersConnectedInternal(viewers, markedNodes);
        }
    }
}

void
Node::hasOutputNodesConnectedInternal(std::list<OutputEffectInstancePtr >* writers,
                                     std::list<const Node*>* markedNodes) const
{
    if (std::find(markedNodes->begin(), markedNodes->end(), this) != markedNodes->end()) {
        return;
    }

    markedNodes->push_back(this);

    OutputEffectInstancePtr thisWriter = boost::dynamic_pointer_cast<OutputEffectInstance>(_imp->effect);

    if ( thisWriter && thisWriter->isOutput() && !dynamic_cast<GroupOutput*>(thisWriter.get()) ) {
        writers->push_back(thisWriter);
    } else {
        NodesList outputs;
        getOutputsWithGroupRedirection(outputs);

        for (NodesList::iterator it = outputs.begin(); it != outputs.end(); ++it) {
            assert(*it);
            (*it)->hasOutputNodesConnectedInternal(writers, markedNodes);
        }
    }
}

void
Node::hasOutputNodesConnected(std::list<OutputEffectInstancePtr >* writers) const
{
    std::list<const Node*> m;
    hasOutputNodesConnectedInternal(writers, &m);
}

void
Node::hasViewersConnected(std::list<ViewerInstancePtr >* viewers) const
{

    std::list<const Node*> m;
    hasViewersConnectedInternal(viewers, &m);

}

/**
 * @brief Resolves links of the graph in the case of containers (that do not do any rendering but only contain nodes inside)
 * so that algorithms that cycle the tree from bottom to top
 * properly visit all nodes in the correct order
 **/
static NodePtr
applyNodeRedirectionsUpstream(const NodePtr& node)
{
    if (!node) {
        return node;
    }
    NodeGroupPtr isGrp = node->isEffectNodeGroup();
    if (isGrp) {
        //The node is a group, instead jump directly to the output node input of the  group
        return applyNodeRedirectionsUpstream(isGrp->getOutputNodeInput());
    }

    PrecompNodePtr isPrecomp = node->isEffectPrecompNode();
    if (isPrecomp) {
        //The node is a precomp, instead jump directly to the output node of the precomp
        return applyNodeRedirectionsUpstream(isPrecomp->getOutputNode());
    }

    GroupInputPtr isInput = node->isEffectGroupInput();
    if (isInput) {
        //The node is a group input,  jump to the corresponding input of the group
        NodeCollectionPtr collection = node->getGroup();
        assert(collection);
        isGrp = toNodeGroup(collection);
        if (isGrp) {
            return applyNodeRedirectionsUpstream(isGrp->getRealInputForInput(node));
        }
    }

    return node;
}

/**
 * @brief Resolves links of the graph in the case of containers (that do not do any rendering but only contain nodes inside)
 * so that algorithms that cycle the tree from top to bottom
 * properly visit all nodes in the correct order. Note that one node may translate to several nodes since multiple nodes
 * may be connected to the same node.
 **/
static void
applyNodeRedirectionsDownstream(int recurseCounter,
                                const NodePtr& node,
                                NodesList& translated)
{
    NodeGroupPtr isGrp = node->isEffectNodeGroup();

    if (isGrp) {
        //The node is a group, meaning it should not be taken into account, instead jump directly to the input nodes output of the group
        NodesList inputNodes;
        isGrp->getInputsOutputs(&inputNodes);
        for (NodesList::iterator it2 = inputNodes.begin(); it2 != inputNodes.end(); ++it2) {
            //Call recursively on them
            applyNodeRedirectionsDownstream(recurseCounter + 1, *it2, translated);
        }

        return;
    }

    GroupOutputPtr isOutput = node->isEffectGroupOutput();
    if (isOutput) {
        //The node is the output of a group, its outputs are the outputs of the group
        NodeCollectionPtr collection = isOutput->getNode()->getGroup();
        if (!collection) {
            return;
        }
        isGrp = toNodeGroup(collection);
        if (isGrp) {
            NodesWList groupOutputs;

            NodePtr grpNode = isGrp->getNode();
            if (grpNode) {
                grpNode->getOutputs_mt_safe(groupOutputs);
            }

            for (NodesWList::iterator it2 = groupOutputs.begin(); it2 != groupOutputs.end(); ++it2) {
                //Call recursively on them
                NodePtr output = it2->lock();
                if (output) {
                    applyNodeRedirectionsDownstream(recurseCounter + 1, output, translated);
                }
            }
        }

        return;
    }

    PrecompNodePtr isInPrecomp = node->isPartOfPrecomp();
    if ( isInPrecomp && (isInPrecomp->getOutputNode() == node) ) {
        //This node is the output of the precomp, its outputs are the outputs of the precomp node
        NodesWList groupOutputs;

        isInPrecomp->getNode()->getOutputs_mt_safe(groupOutputs);

        for (NodesWList::iterator it2 = groupOutputs.begin(); it2 != groupOutputs.end(); ++it2) {
            //Call recursively on them
            NodePtr output = it2->lock();
            if (output) {
                applyNodeRedirectionsDownstream(recurseCounter + 1, output, translated);
            }
        }

        return;
    }

    //Base case: return this node
    if (recurseCounter > 0) {
        translated.push_back(node);
    }
} // applyNodeRedirectionsDownstream

void
Node::getOutputsWithGroupRedirection(NodesList& outputs) const
{
    NodesList redirections;
    NodePtr thisShared = boost::const_pointer_cast<Node>( shared_from_this() );

    applyNodeRedirectionsDownstream(0, thisShared, redirections);
    if ( !redirections.empty() ) {
        outputs.insert( outputs.begin(), redirections.begin(), redirections.end() );
    } else {
        QMutexLocker l(&_imp->outputsMutex);
        for (NodesWList::const_iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it) {
            NodePtr output = it->lock();
            if (output) {
                outputs.push_back(output);
            }
        }
    }
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
Node::initializeInputs()
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    const int inputCount = getMaxInputCount();
    InputsV oldInputs;
    {
        QMutexLocker k(&_imp->inputsLabelsMutex);
        _imp->inputLabels.resize(inputCount);
        _imp->inputHints.resize(inputCount);
        for (int i = 0; i < inputCount; ++i) {
            _imp->inputLabels[i] = _imp->effect->getInputLabel(i);
            _imp->inputHints[i] = _imp->effect->getInputHint(i);
        }
    }
    {
        QMutexLocker l(&_imp->lastRenderStartedMutex);
        _imp->inputIsRenderingCounter.resize(inputCount);
    }
    {
        QMutexLocker l(&_imp->inputsMutex);
        oldInputs = _imp->inputs;

        std::vector<bool> oldInputsVisibility = _imp->inputsVisibility;
        _imp->inputIsRenderingCounter.resize(inputCount);
        _imp->inputs.resize(inputCount);
        _imp->inputsVisibility.resize(inputCount);
        ///if we added inputs, just set to NULL the new inputs, and add their label to the labels map
        for (int i = 0; i < inputCount; ++i) {
            if ( i < (int)oldInputs.size() ) {
                _imp->inputs[i] = oldInputs[i];
            } else {
                _imp->inputs[i].reset();
            }
            if (i < (int) oldInputsVisibility.size()) {
                _imp->inputsVisibility[i] = oldInputsVisibility[i];
            } else {
                _imp->inputsVisibility[i] = true;
            }
        }


        ///Set the components the plug-in accepts
        _imp->inputsComponents.resize(inputCount);
        for (int i = 0; i < inputCount; ++i) {
            _imp->inputsComponents[i].clear();
            if ( _imp->effect->isInputMask(i) ) {
                //Force alpha for masks
                _imp->inputsComponents[i].push_back( ImageComponents::getAlphaComponents() );
            } else {
                _imp->effect->addAcceptedComponents(i, &_imp->inputsComponents[i]);
            }
        }
        _imp->outputComponents.clear();
        _imp->effect->addAcceptedComponents(-1, &_imp->outputComponents);
    }
    _imp->inputsInitialized = true;

    Q_EMIT inputsInitialized();
} // Node::initializeInputs

NodePtr
Node::getInput(int index) const
{
    return getInputInternal(true, index);
}

NodePtr
Node::getInputInternal(bool useGroupRedirections,
                       int index) const
{
    if (!_imp->inputsInitialized) {
        qDebug() << "Node::getInput(): inputs not initialized";
    }
    QMutexLocker l(&_imp->inputsMutex);
    if ( ( index >= (int)_imp->inputs.size() ) || (index < 0) ) {
        return NodePtr();
    }

    NodePtr ret =  _imp->inputs[index].lock();
    if (ret && useGroupRedirections) {
        ret = applyNodeRedirectionsUpstream(ret);
    }

    return ret;
}

NodePtr
Node::getRealInput(int index) const
{
    return getInputInternal(false, index);
}

int
Node::getInputIndex(const NodeConstPtr& node) const
{
    QMutexLocker l(&_imp->inputsMutex);

    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i].lock() == node) {
            return i;
        }
    }

    return -1;
}

const std::vector<NodeWPtr > &
Node::getInputs() const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);
    return _imp->inputs;
}


std::vector<NodeWPtr >
Node::getInputs_copy() const
{
    assert(_imp->inputsInitialized);

    QMutexLocker l(&_imp->inputsMutex);

    return _imp->inputs;
}

std::string
Node::getInputLabel(int inputNb) const
{
    assert(_imp->inputsInitialized);

    QMutexLocker l(&_imp->inputsLabelsMutex);
    if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputLabels.size() ) ) {
        throw std::invalid_argument("Index out of range");
    }

    return _imp->inputLabels[inputNb];
}

std::string
Node::getInputHint(int inputNb) const
{
    assert(_imp->inputsInitialized);

    QMutexLocker l(&_imp->inputsLabelsMutex);
    if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputHints.size() ) ) {
        throw std::invalid_argument("Index out of range");
    }

    return _imp->inputHints[inputNb];
}

void
Node::setInputLabel(int inputNb, const std::string& label)
{
    {
        QMutexLocker l(&_imp->inputsLabelsMutex);
        if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputLabels.size() ) ) {
            throw std::invalid_argument("Index out of range");
        }
        _imp->inputLabels[inputNb] = label;
    }
    std::map<int, MaskSelector>::iterator foundMask = _imp->maskSelectors.find(inputNb);
    if (foundMask != _imp->maskSelectors.end()) {
        foundMask->second.channel.lock()->setLabel(label);
    }

    std::map<int, ChannelSelector>::iterator foundChannel = _imp->channelsSelectors.find(inputNb);
    if (foundChannel != _imp->channelsSelectors.end()) {
        foundChannel->second.layer.lock()->setLabel(label + std::string(" Layer"));
    }

    Q_EMIT inputEdgeLabelChanged(inputNb, QString::fromUtf8(label.c_str()));
}

void
Node::setInputHint(int inputNb, const std::string& hint)
{
    {
        QMutexLocker l(&_imp->inputsLabelsMutex);
        if ( (inputNb < 0) || ( inputNb >= (int)_imp->inputHints.size() ) ) {
            throw std::invalid_argument("Index out of range");
        }
        _imp->inputHints[inputNb] = hint;
    }
}

bool
Node::isInputVisible(int inputNb) const
{
    QMutexLocker k(&_imp->inputsMutex);
    if (inputNb >= 0 && inputNb < (int)_imp->inputsVisibility.size()) {
        return _imp->inputsVisibility[inputNb];
    } else {
        throw std::invalid_argument("Index out of range");
    }
    return false;
}

void
Node::setInputVisible(int inputNb, bool visible)
{
    {
        QMutexLocker k(&_imp->inputsMutex);
        if (inputNb >= 0 && inputNb < (int)_imp->inputsVisibility.size()) {
            _imp->inputsVisibility[inputNb] = visible;
        } else {
            throw std::invalid_argument("Index out of range");
        }
    }
    Q_EMIT inputVisibilityChanged(inputNb);
}


int
Node::getInputNumberFromLabel(const std::string& inputLabel) const
{
    assert(_imp->inputsInitialized);
    QMutexLocker l(&_imp->inputsLabelsMutex);
    for (U32 i = 0; i < _imp->inputLabels.size(); ++i) {
        if (_imp->inputLabels[i] == inputLabel) {
            return i;
        }
    }

    return -1;
}

bool
Node::isInputConnected(int inputNb) const
{
    assert(_imp->inputsInitialized);

    return getInput(inputNb) != NULL;
}

bool
Node::hasInputConnected() const
{
    assert(_imp->inputsInitialized);

    QMutexLocker l(&_imp->inputsMutex);
    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if ( _imp->inputs[i].lock() ) {
            return true;
        }
    }


    return false;
}

bool
Node::hasMandatoryInputDisconnected() const
{
    QMutexLocker l(&_imp->inputsMutex);

    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if ( !_imp->inputs[i].lock() && !_imp->effect->isInputOptional(i) ) {
            return true;
        }
    }

    return false;
}

bool
Node::hasAllInputsConnected() const
{
    QMutexLocker l(&_imp->inputsMutex);

    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if ( !_imp->inputs[i].lock() ) {
            return false;
        }
    }

    return true;
}

bool
Node::hasOutputConnected() const
{
    ////Only called by the main-thread

    if ( isOutputNode() ) {
        return true;
    }
    if ( QThread::currentThread() == qApp->thread() ) {
        if (_imp->outputs.size() > 0) {
            return true;
        }
    } else {
        QMutexLocker l(&_imp->outputsMutex);
        if (_imp->outputs.size() > 0) {
            return true;
        }
    }

    return false;
}

bool
Node::checkIfConnectingInputIsOk(const NodePtr& input) const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    if (!input || input.get() == this) {
        return false;
    }
    bool found = input->isNodeUpstream(shared_from_this());
    return !found;
}

bool
Node::isNodeUpstreamInternal(const NodeConstPtr& input, std::list<const Node*>& markedNodes) const
{
    if (!input) {
        return false;
    }

    if (std::find(markedNodes.begin(), markedNodes.end(), this) != markedNodes.end()) {
        return false;
    }

    markedNodes.push_back(this);

    // No need to lock inputs is only written to by the main-thread
    for (std::size_t i = 0; i  < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i].lock() == input) {
            return true;
        }
    }

    for (std::size_t i = 0; i  < _imp->inputs.size(); ++i) {
        NodePtr in = _imp->inputs[i].lock();
        if (in) {
            if (in->isNodeUpstreamInternal(input, markedNodes)) {
                return true;
            }
        }
    }
    return false;
}

bool
Node::isNodeUpstream(const NodeConstPtr& input) const
{
    ////Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    std::list<const Node*> markedNodes;
    return isNodeUpstreamInternal(input, markedNodes);
}


static Node::CanConnectInputReturnValue
checkCanConnectNoMultiRes(const Node* output,
                          const NodePtr& input)
{
    //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsMultiResolution
    //Check that the input has the same RoD that another input and that its rod is set to 0,0
    RenderScale scale(1.);
    RectD rod;
    U64 inputHash;
    bool gotHash = input->getEffectInstance()->getRenderHash(input->getEffectInstance()->getCurrentTime(), ViewIdx(0), &inputHash);
    (void)gotHash;
    StatusEnum stat = input->getEffectInstance()->getRegionOfDefinition_public(inputHash,
                                                                               output->getApp()->getTimeLine()->currentFrame(),
                                                                               scale,
                                                                               ViewIdx(0),
                                                                               &rod);

    if ( (stat == eStatusFailed) && !rod.isNull() ) {
        return Node::eCanConnectInput_givenNodeNotConnectable;
    }
    if ( (rod.x1 != 0) || (rod.y1 != 0) ) {
        return Node::eCanConnectInput_multiResNotSupported;
    }

    // Commented-out: Some Furnace plug-ins from The Foundry (e.g F_Steadiness) are not supporting multi-resolution but actually produce an output
    // with a RoD different from the input

    /*RectD outputRod;
       stat = output->getEffectInstance()->getRegionOfDefinition_public(output->getCacheID(), output->getApp()->getTimeLine()->currentFrame(), scale, ViewIdx(0), &outputRod);
       Q_UNUSED(stat);

       if ( !outputRod.isNull() && (rod != outputRod) ) {
        return Node::eCanConnectInput_multiResNotSupported;
       }*/

    for (int i = 0; i < output->getMaxInputCount(); ++i) {
        NodePtr inputNode = output->getInput(i);
        if (inputNode) {
            RectD inputRod;
            stat = inputNode->getEffectInstance()->getRegionOfDefinition_public(0,
                                                                                output->getApp()->getTimeLine()->currentFrame(),
                                                                                scale,
                                                                                ViewIdx(0),
                                                                                &inputRod);
            if ( (stat == eStatusFailed) && !inputRod.isNull() ) {
                return Node::eCanConnectInput_givenNodeNotConnectable;
            }
            if (inputRod != rod) {
                return Node::eCanConnectInput_multiResNotSupported;
            }
        }
    }

    return Node::eCanConnectInput_ok;
}

Node::CanConnectInputReturnValue
Node::canConnectInput(const NodePtr& input,
                      int inputNumber) const
{
    ///No-one is allowed to connect to the other node
    if ( !input || !input->canOthersConnectToThisNode() ) {
        return eCanConnectInput_givenNodeNotConnectable;
    }

    ///Check for invalid index
    {
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNumber < 0) || ( inputNumber >= (int)_imp->inputs.size() ) ) {
            return eCanConnectInput_indexOutOfRange;
        }
        if ( _imp->inputs[inputNumber].lock() ) {
            return eCanConnectInput_inputAlreadyConnected;
        }
    }

    NodeGroupPtr isGrp = input->isEffectNodeGroup();
    if ( isGrp && !isGrp->getOutputNode() ) {
        return eCanConnectInput_groupHasNoOutput;
    }


    ///Applying this connection would create cycles in the graph
    if ( !checkIfConnectingInputIsOk( input ) ) {
        return eCanConnectInput_graphCycles;
    }


    if ( !_imp->effect->supportsMultiResolution() ) {
        CanConnectInputReturnValue ret = checkCanConnectNoMultiRes(this, input);
        if (ret != eCanConnectInput_ok) {
            return ret;
        }
    }

    {
        ///Check for invalid pixel aspect ratio if the node doesn't support multiple clip PARs

        double inputPAR = input->getEffectInstance()->getAspectRatio(-1);
        double inputFPS = input->getEffectInstance()->getFrameRate();
        QMutexLocker l(&_imp->inputsMutex);

        for (InputsV::const_iterator it = _imp->inputs.begin(); it != _imp->inputs.end(); ++it) {
            NodePtr node = it->lock();
            if (node) {
                if ( !_imp->effect->supportsMultipleClipPARs() ) {
                    if (node->getEffectInstance()->getAspectRatio(-1) != inputPAR) {
                        return eCanConnectInput_differentPars;
                    }
                }

                if (std::abs(node->getEffectInstance()->getFrameRate() - inputFPS) > 0.01) {
                    return eCanConnectInput_differentFPS;
                }
            }
        }
    }

    return eCanConnectInput_ok;
} // Node::canConnectInput

bool
Node::connectInput(const NodePtr & input,
                   int inputNumber)
{
    assert(_imp->inputsInitialized);
    assert(input);

    ///Check for cycles: they are forbidden in the graph
    if ( !checkIfConnectingInputIsOk( input ) ) {
        return false;
    }

    ///For effects that do not support multi-resolution, make sure the input effect is correct
    ///otherwise the rendering might crash
    if ( !_imp->effect->supportsMultiResolution() && !getApp()->getProject()->isLoadingProject() ) {
        CanConnectInputReturnValue ret = checkCanConnectNoMultiRes(this, input);
        if (ret != eCanConnectInput_ok) {
            return false;
        }
    }

    _imp->effect->abortAnyEvaluation();

    {
        ///Check for invalid index
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNumber < 0) ||
             ( inputNumber >= (int)_imp->inputs.size() ) ||
             _imp->inputs[inputNumber].lock()) {
            return false;
        }

        _imp->inputs[inputNumber] = input;

        input->connectOutput(shared_from_this() );
    }

    getApp()->recheckInvalidExpressions();

    ///Get notified when the input name has changed
    QObject::connect( input.get(), SIGNAL(labelChanged(QString)), this, SLOT(onInputLabelChanged(QString)) );

    ///Notify the GUI
    Q_EMIT inputChanged(inputNumber);
    bool mustCallEnd = false;

    ///Call the instance changed action with a reason clip changed
    beginInputEdition();
    mustCallEnd = true;
    onInputChanged(inputNumber);


    bool creatingNodeTree = getApp()->isCreatingNodeTree();
    if (!creatingNodeTree) {
        _imp->effect->invalidateHashCache();
    }

    std::string inputChangedCB = getInputChangedCallback();
    if ( !inputChangedCB.empty() ) {
        _imp->runInputChangedCallback(inputNumber, inputChangedCB);
    }

    if (mustCallEnd) {
        endInputEdition(true);
    }

    return true;
} // Node::connectInput


bool
Node::replaceInputInternal(const NodePtr& input, int inputNumber)
{
    assert(_imp->inputsInitialized);
    assert(input);
    ///Check for cycles: they are forbidden in the graph
    if ( !checkIfConnectingInputIsOk( input ) ) {
        return false;
    }


    ///For effects that do not support multi-resolution, make sure the input effect is correct
    ///otherwise the rendering might crash
    if ( !_imp->effect->supportsMultiResolution() ) {
        CanConnectInputReturnValue ret = checkCanConnectNoMultiRes(this, input);
        if (ret != eCanConnectInput_ok) {
            return false;
        }
    }

    {
        ///Check for invalid index
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNumber < 0) || ( inputNumber > (int)_imp->inputs.size() ) ) {
            return false;
        }
    }


    {
        QMutexLocker l(&_imp->inputsMutex);
        ///Set the input

        NodePtr curIn = _imp->inputs[inputNumber].lock();
        if (curIn) {
            QObject::connect( curIn.get(), SIGNAL(labelChanged(QString)), this, SLOT(onInputLabelChanged(QString)) );
            curIn->disconnectOutput(this);
        }
        _imp->inputs[inputNumber] = input;

        input->connectOutput(shared_from_this() );
    }

    ///Get notified when the input name has changed
    QObject::connect( input.get(), SIGNAL(labelChanged(QString)), this, SLOT(onInputLabelChanged(QString)) );

    ///Notify the GUI
    Q_EMIT inputChanged(inputNumber);
    bool mustCallEnd = false;
    beginInputEdition();
    mustCallEnd = true;
    ///Call the instance changed action with a reason clip changed
    onInputChanged(inputNumber);


    bool creatingNodeTree = getApp()->isCreatingNodeTree();
    if (!creatingNodeTree) {
        // Notify cache
        _imp->effect->invalidateHashCache();
    }


    std::string inputChangedCB = getInputChangedCallback();
    if ( !inputChangedCB.empty() ) {
        _imp->runInputChangedCallback(inputNumber, inputChangedCB);
    }

    if (mustCallEnd) {
        endInputEdition(true);
    }
    
    return true;
}

bool
Node::replaceInput(const NodePtr& input,
                   int inputNumber)
{


    _imp->effect->abortAnyEvaluation();
    return replaceInputInternal(input, inputNumber);
} // Node::replaceInput

void
Node::switchInput0And1()
{
    assert(_imp->inputsInitialized);
    int maxInputs = getMaxInputCount();
    if (maxInputs < 2) {
        return;
    }
    ///get the first input number to switch
    int inputAIndex = -1;
    for (int i = 0; i < maxInputs; ++i) {
        if ( !_imp->effect->isInputMask(i) ) {
            inputAIndex = i;
            break;
        }
    }

    ///There's only a mask ??
    if (inputAIndex == -1) {
        return;
    }

    ///get the second input number to switch
    int inputBIndex = -1;
    int firstMaskInput = -1;
    for (int j = 0; j < maxInputs; ++j) {
        if (j == inputAIndex) {
            continue;
        }
        if ( !_imp->effect->isInputMask(j) ) {
            inputBIndex = j;
            break;
        } else {
            firstMaskInput = j;
        }
    }
    if (inputBIndex == -1) {
        ///if there's a mask use it as input B for the switch
        if (firstMaskInput != -1) {
            inputBIndex = firstMaskInput;
        }
    }

    _imp->effect->abortAnyEvaluation();

    {
        QMutexLocker l(&_imp->inputsMutex);
        assert( inputAIndex < (int)_imp->inputs.size() && inputBIndex < (int)_imp->inputs.size() );
        NodePtr input0, input1;

        input0 = _imp->inputs[inputAIndex].lock();
        input1 = _imp->inputs[inputBIndex].lock();
        _imp->inputs[inputAIndex] = _imp->inputs[inputBIndex];
        _imp->inputs[inputBIndex] = input0;


    }
    Q_EMIT inputChanged(inputAIndex);
    Q_EMIT inputChanged(inputBIndex);

    beginInputEdition();
    onInputChanged(inputAIndex);
    onInputChanged(inputBIndex);

    bool creatingNodeTree = getApp()->isCreatingNodeTree();
    if (!creatingNodeTree) {
        // Notify cache
        _imp->effect->invalidateHashCache();

    }

    std::string inputChangedCB = getInputChangedCallback();
    if ( !inputChangedCB.empty() ) {
        _imp->runInputChangedCallback(inputAIndex, inputChangedCB);
        _imp->runInputChangedCallback(inputBIndex, inputChangedCB);
    }


    endInputEdition(true);
} // switchInput0And1

void
Node::onInputLabelChanged(const QString & name)
{
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);
    Node* inp = dynamic_cast<Node*>( sender() );
    assert(inp);
    if (!inp) {
        // coverity[dead_error_line]
        return;
    }
    int inputNb = -1;
    ///No need to lock, inputs is only written to by the mainthread

    for (U32 i = 0; i < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i].lock().get() == inp) {
            inputNb = i;
            break;
        }
    }

    if (inputNb != -1) {
        Q_EMIT inputLabelChanged(inputNb, name);
    }
}

void
Node::connectOutput(const NodePtr& output)
{
    assert(output);

    {
        QMutexLocker l(&_imp->outputsMutex);
        _imp->outputs.push_back(output);
    }
    Q_EMIT outputsChanged();
}

int
Node::disconnectInput(int inputNumber)
{
    assert(_imp->inputsInitialized);

    NodePtr inputShared;
    bool destroyed;
    {
        QMutexLocker k(&_imp->isBeingDestroyedMutex);
        destroyed = _imp->isBeingDestroyed;
    }
    if (!destroyed) {
        _imp->effect->abortAnyEvaluation();
    }

    {
        QMutexLocker l(&_imp->inputsMutex);
        if ( (inputNumber < 0) ||
             ( inputNumber > (int)_imp->inputs.size() ) ||
             (!_imp->inputs[inputNumber].lock() )) {
            return -1;
        }
        inputShared = _imp->inputs[inputNumber].lock();
    }


    QObject::disconnect( inputShared.get(), SIGNAL(labelChanged(QString)), this, SLOT(onInputLabelChanged(QString)) );
    inputShared->disconnectOutput(this);

    {
        QMutexLocker l(&_imp->inputsMutex);
        _imp->inputs[inputNumber].reset();

    }

    {
        QMutexLocker k(&_imp->isBeingDestroyedMutex);
        if (_imp->isBeingDestroyed) {
            return -1;
        }
    }

    Q_EMIT inputChanged(inputNumber);

    beginInputEdition();
    onInputChanged(inputNumber);

    bool creatingNodeTree = getApp()->isCreatingNodeTree();
    if (!creatingNodeTree) {
        // Notify cache
        _imp->effect->invalidateHashCache();
    }


    std::string inputChangedCB = getInputChangedCallback();
    if ( !inputChangedCB.empty() ) {
        _imp->runInputChangedCallback(inputNumber, inputChangedCB);
    }
    endInputEdition(true);

    return inputNumber;
} // Node::disconnectInput

int
Node::disconnectInputInternal(const NodePtr& input)
{
    assert(_imp->inputsInitialized);
    int found = -1;
    NodePtr inputShared;
    {
        QMutexLocker l(&_imp->inputsMutex);
        for (std::size_t i = 0; i < _imp->inputs.size(); ++i) {
            NodePtr curInput = _imp->inputs[i].lock();
            if (curInput == input) {
                inputShared = curInput;
                found = (int)i;
                break;
            }
        }

    }
    if (found != -1) {
        {
            QMutexLocker l(&_imp->inputsMutex);
            _imp->inputs[found].reset();
        }
        input->disconnectOutput(this);
        Q_EMIT inputChanged(found);
        beginInputEdition();
        onInputChanged(found);

        bool creatingNodeTree = getApp()->isCreatingNodeTree();
        if (!creatingNodeTree) {
            ///Recompute the hash
            if ( !getApp()->getProject()->isProjectClosing() ) {
                _imp->effect->invalidateHashCache();
            }
        }



        std::string inputChangedCB = getInputChangedCallback();
        if ( !inputChangedCB.empty() ) {
            _imp->runInputChangedCallback(found, inputChangedCB);
        }

        endInputEdition(true);

        return found;
    }
    
    return -1;
}

int
Node::disconnectInput(const NodePtr& input)
{

    _imp->effect->abortAnyEvaluation();
    return disconnectInputInternal(input);
} // Node::disconnectInput

int
Node::disconnectOutput(const Node* output)
{
    assert(output);
    int ret = -1;
    {
        QMutexLocker l(&_imp->outputsMutex);
        int ret = 0;
        for (NodesWList::iterator it = _imp->outputs.begin(); it != _imp->outputs.end(); ++it, ++ret) {
            if (it->lock().get() == output) {
                _imp->outputs.erase(it);
                break;
            }
        }
    }

    //Will just refresh the gui
    Q_EMIT outputsChanged();

    return ret;
}

int
Node::inputIndex(const NodePtr& n) const
{
    if (!n) {
        return -1;
    }

    ///Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->inputsInitialized);

    ///No need to lock this is only called by the main-thread
    for (std::size_t i = 0; i < _imp->inputs.size(); ++i) {
        if (_imp->inputs[i].lock() == n) {
            return i;
        }
    }


    return -1;
}

void
Node::clearLastRenderedImage()
{
    _imp->effect->clearLastRenderedImage();
}

void
Node::deactivate(const std::list< NodePtr > & outputsToDisconnect,
                 bool disconnectAll,
                 bool reconnect,
                 bool hideGui,
                 bool triggerRender,
                 bool unslaveKnobs)
{

    if ( !_imp->effect || !isActivated() ) {
        return;
    }
    //first tell the gui to clear any persistent message linked to this node
    clearPersistentMessage(false);



    bool beingDestroyed;
    {
        QMutexLocker k(&_imp->isBeingDestroyedMutex);
        beingDestroyed = _imp->isBeingDestroyed;
    }


    ///kill any thread it could have started
    ///Commented-out: If we were to undo the deactivate we don't want all threads to be
    ///exited, just exit them when the effect is really deleted instead
    //quitAnyProcessing();
    if (!beingDestroyed) {
        _imp->effect->abortAnyEvaluation(false /*keepOldestRender*/);
        _imp->abortPreview_non_blocking();
    }

    NodeCollectionPtr parentCol = getGroup();


    if (unslaveKnobs) {
        ///For all knobs that have listeners, invalidate expressions
        NodeGroupPtr isParentGroup = toNodeGroup(parentCol);
        KnobsVec knobs = _imp->effect->getKnobs_mt_safe();
        for (U32 i = 0; i < knobs.size(); ++i) {
            KnobI::ListenerDimsMap listeners;
            knobs[i]->getListeners(listeners);
            for (KnobI::ListenerDimsMap::iterator it = listeners.begin(); it != listeners.end(); ++it) {
                KnobIPtr listener = it->first.lock();
                if (!listener) {
                    continue;
                }
                KnobHolderPtr holder = listener->getHolder();
                if (!holder) {
                    continue;
                }
                if ( ( holder == _imp->effect ) || (holder == isParentGroup) ) {
                    continue;
                }

                EffectInstancePtr isEffect = toEffectInstance(holder);
                if (!isEffect) {
                    continue;
                }

                NodePtr effectNode = isEffect->getNode();
                if (!effectNode) {
                    continue;
                }
                NodeCollectionPtr effectParent = effectNode->getGroup();
                if (!effectParent) {
                    continue;
                }
                NodeGroupPtr isEffectParentGroup = toNodeGroup(effectParent);
                if ( isEffectParentGroup && (isEffectParentGroup == _imp->effect) ) {
                    continue;
                }

                isEffect->beginChanges();
                std::list<ViewIdx> views = listener->getViewsList();
                for (int dim = 0; dim < listener->getNDimensions(); ++dim) {
                    for (std::list<ViewIdx>::const_iterator it2 = views.begin(); it2 != views.end(); ++it2) {
                        MasterKnobLink linkData;
                        if (listener->getMaster(DimIdx(dim), *it2, &linkData)) {
                            KnobIPtr masterKnob = linkData.masterKnob.lock();
                            if (masterKnob == knobs[i]) {
                                listener->unSlave(DimIdx(dim), *it2, true);
                            }
                        }

                        std::string hasExpr = listener->getExpression(DimIdx(dim), *it2);
                        if ( !hasExpr.empty() ) {
                            std::stringstream ss;
                            ss << tr("Missing node ").toStdString();
                            ss << getFullyQualifiedName();
                            ss << ' ';
                            ss << tr("in expression.").toStdString();
                            listener->setExpressionInvalid( DimIdx(dim), *it2, false, ss.str() );
                        }

                    }

                }
                isEffect->endChanges(true);
            }
        }
    }

    ///if the node has 1 non-optional input, attempt to connect the outputs to the input of the current node
    ///this node is the node the outputs should attempt to connect to
    NodePtr inputToConnectTo;
    NodePtr firstOptionalInput;
    int firstNonOptionalInput = -1;
    if (reconnect) {
        bool hasOnlyOneInputConnected = false;

        ///No need to lock inputs is only written to by the mainthread
        for (std::size_t i = 0; i < _imp->inputs.size(); ++i) {
            NodePtr input = _imp->inputs[i].lock();
            if (input) {
                if ( !_imp->effect->isInputOptional(i) ) {
                    if (firstNonOptionalInput == -1) {
                        firstNonOptionalInput = i;
                        hasOnlyOneInputConnected = true;
                    } else {
                        hasOnlyOneInputConnected = false;
                    }
                } else if (!firstOptionalInput) {
                    firstOptionalInput = input;
                    if (hasOnlyOneInputConnected) {
                        hasOnlyOneInputConnected = false;
                    } else {
                        hasOnlyOneInputConnected = true;
                    }
                }
            }
        }

        if (hasOnlyOneInputConnected) {
            if (firstNonOptionalInput != -1) {
                inputToConnectTo = getRealInput(firstNonOptionalInput);
            } else if (firstOptionalInput) {
                inputToConnectTo = firstOptionalInput;
            }
        }
    }
    /*Removing this node from the output of all inputs*/
    _imp->deactivatedState.clear();


    std::vector<NodePtr > inputsQueueCopy;


    if (hideGui) {
        for (std::size_t i = 0; i < _imp->inputs.size(); ++i) {
            NodePtr input = _imp->inputs[i].lock();
            if (input) {
                input->disconnectOutput(this);
            }
        }
    }


    ///For each output node we remember that the output node  had its input number inputNb connected
    ///to this node
    NodesWList outputsQueueCopy;
    {
        QMutexLocker l(&_imp->outputsMutex);
        outputsQueueCopy = _imp->outputs;
    }


    for (NodesWList::iterator it = outputsQueueCopy.begin(); it != outputsQueueCopy.end(); ++it) {
        NodePtr output = it->lock();
        if (!output) {
            continue;
        }
        bool dc = false;
        if (disconnectAll) {
            dc = true;
        } else {
            for (NodesList::const_iterator found = outputsToDisconnect.begin(); found != outputsToDisconnect.end(); ++found) {
                if (*found == output) {
                    dc = true;
                    break;
                }
            }
        }
        if (dc) {
            int inputNb = output->getInputIndex(shared_from_this());
            if (inputNb != -1) {
                _imp->deactivatedState.insert( make_pair(*it, inputNb) );

                ///reconnect if inputToConnectTo is not null
                if (inputToConnectTo) {
                    output->replaceInputInternal(inputToConnectTo, inputNb);
                } else {
                    ignore_result( output->disconnectInputInternal(shared_from_this()) );
                }
            }
        }
    }

    // If the effect was doing OpenGL rendering and had context(s) bound, dettach them.
    _imp->effect->dettachAllOpenGLContexts();


    ///Free all memory used by the plug-in.

    ///COMMENTED-OUT: Don't do this, the node may still be rendering here since the abort call is not blocking.
    ///_imp->effect->clearPluginMemoryChunks();
    clearLastRenderedImage();

    if (parentCol && !beingDestroyed) {
        parentCol->notifyNodeDeactivated( shared_from_this() );
    }

    if (hideGui && !beingDestroyed) {
        Q_EMIT deactivated(triggerRender);
    }
    {
        QMutexLocker l(&_imp->activatedMutex);
        _imp->activated = false;
    }


    ///If the node is a group, deactivate all nodes within the group
    NodeGroupPtr isGrp = isEffectNodeGroup();
    if (isGrp) {
        isGrp->setIsDeactivatingGroup(true);
        NodesList nodes = isGrp->getNodes();
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            (*it)->deactivate(std::list< NodePtr >(), false, false, true, false);
        }
        isGrp->setIsDeactivatingGroup(false);
    }


    AppInstancePtr app = getApp();
    if ( app && !app->getProject()->isProjectClosing() ) {
        _imp->runOnNodeDeleteCB();
    }

    deleteNodeVariableToPython( getFullyQualifiedName() );
} // deactivate

void
Node::activate(const std::list< NodePtr > & outputsToRestore,
               bool restoreAll,
               bool triggerRender)
{
    ///Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !_imp->effect || isActivated() ) {
        return;
    }


    ///No need to lock, inputs is only written to by the main-thread
    NodePtr thisShared = shared_from_this();

    ///for all inputs, reconnect their output to this node
    for (std::size_t i = 0; i < _imp->inputs.size(); ++i) {
        NodePtr input = _imp->inputs[i].lock();
        if (input) {
            input->connectOutput(thisShared);
        }
    }


    ///Restore all outputs that was connected to this node
    for (std::map<NodeWPtr, int >::iterator it = _imp->deactivatedState.begin();
         it != _imp->deactivatedState.end(); ++it) {
        NodePtr output = it->first.lock();
        if (!output) {
            continue;
        }

        bool restore = false;
        if (restoreAll) {
            restore = true;
        } else {
            for (NodesList::const_iterator found = outputsToRestore.begin(); found != outputsToRestore.end(); ++found) {
                if (*found == output) {
                    restore = true;
                    break;
                }
            }
        }

        if (restore) {
            ///before connecting the outputs to this node, disconnect any link that has been made
            ///between the outputs by the user. This should normally never happen as the undo/redo
            ///stack follow always the same order.
            NodePtr outputHasInput = output->getInput(it->second);
            if (outputHasInput) {
                bool ok = getApp()->getProject()->disconnectNodes(outputHasInput, output);
                assert(ok);
                Q_UNUSED(ok);
            }

            ///and connect the output to this node
            output->connectInput(thisShared, it->second);
        }
    }

    {
        QMutexLocker l(&_imp->activatedMutex);
        _imp->activated = true; //< flag it true before notifying the GUI because the gui rely on this flag (espcially the Viewer)
    }

    NodeCollectionPtr group = getGroup();
    if (group) {
        group->notifyNodeActivated( shared_from_this() );
    }
    Q_EMIT activated(triggerRender);


    declareAllPythonAttributes();
    getApp()->recheckInvalidExpressions();


    ///If the node is a group, activate all nodes within the group first
    NodeGroupPtr isGrp = isEffectNodeGroup();
    if (isGrp) {
        isGrp->setIsActivatingGroup(true);
        NodesList nodes = isGrp->getNodes();
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            (*it)->activate(std::list< NodePtr >(), false, false);
        }
        isGrp->setIsActivatingGroup(false);
    }


    _imp->runOnNodeCreatedCB(true);
} // activate

class NodeDestroyNodeInternalArgs
    : public GenericWatcherCallerArgs
{
public:

    bool autoReconnect;

    NodeDestroyNodeInternalArgs()
        : GenericWatcherCallerArgs()
        , autoReconnect(false)
    {}

    virtual ~NodeDestroyNodeInternalArgs() {}
};

void
Node::onProcessingQuitInDestroyNodeInternal(int taskID,
                                            const WatcherCallerArgsPtr& args)
{
    assert(_imp->renderWatcher);
    assert(taskID == (int)NodeRenderWatcher::eBlockingTaskQuitAnyProcessing);
    Q_UNUSED(taskID);
    assert(args);
    NodeDestroyNodeInternalArgs* thisArgs = dynamic_cast<NodeDestroyNodeInternalArgs*>( args.get() );
    assert(thisArgs);
    doDestroyNodeInternalEnd(thisArgs ? thisArgs->autoReconnect : false);
    _imp->renderWatcher.reset();
}

void
Node::doDestroyNodeInternalEnd(bool autoReconnect)
{
    //if (doDeactivate) {
        deactivate(NodesList(),
                   true,
                   autoReconnect,
                   true,
                   false);
    //}

    {
        NodeGuiIPtr guiPtr = _imp->guiPointer.lock();
        if (guiPtr) {
            guiPtr->destroyGui();
        }
    }

    ///If its a group, clear its nodes
    NodeGroupPtr isGrp = isEffectNodeGroup();
    if (isGrp) {
        isGrp->clearNodesBlocking();
    }


    ///Quit any rendering
    {
        OutputEffectInstancePtr isOutput = isEffectOutput();
        if (isOutput) {
            isOutput->getRenderEngine()->quitEngine(true);
        }
    }
    ///Remove all images in the cache associated to this node
    ///This will not remove from the disk cache if the project is closing
    removeAllImagesFromCache();

    AppInstancePtr app = getApp();
    if (app) {
        app->recheckInvalidExpressions();
    }

    ///Remove the Python node
    deleteNodeVariableToPython( getFullyQualifiedName() );


    ///Kill the effect
    if (_imp->effect) {
        _imp->effect->clearPluginMemoryChunks();
    }

    ///If inside the group, remove it from the group
    ///the use_count() after the call to removeNode should be 2 and should be the shared_ptr held by the caller and the
    ///thisShared ptr

    NodeCollectionPtr thisGroup = getGroup();
    if ( thisGroup ) {
        thisGroup->removeNode(this);
    }

    _imp->effect.reset();


} // doDestroyNodeInternalEnd

void
Node::destroyNode(bool blockingDestroy,
                          bool autoReconnect)

{
    if (!_imp->effect) {
        return;
    }

    {
        QMutexLocker k(&_imp->activatedMutex);
        _imp->isBeingDestroyed = true;
    }

    bool allProcessingQuit = areAllProcessingThreadsQuit();
    if (allProcessingQuit || blockingDestroy) {
        if (!allProcessingQuit) {
            quitAnyProcessing_blocking(false);
        }
        doDestroyNodeInternalEnd(false);
    } else {
        NodeGroupPtr isGrp = isEffectNodeGroup();

        NodesList nodesToWatch;
        nodesToWatch.push_back( shared_from_this() );
        if (isGrp) {
            isGrp->getNodes_recursive(nodesToWatch, false);
        }
        _imp->renderWatcher.reset( new NodeRenderWatcher(nodesToWatch) );
        QObject::connect( _imp->renderWatcher.get(), SIGNAL(taskFinished(int,WatcherCallerArgsPtr)), this, SLOT(onProcessingQuitInDestroyNodeInternal(int,WatcherCallerArgsPtr)) );
        boost::shared_ptr<NodeDestroyNodeInternalArgs> args( new NodeDestroyNodeInternalArgs() );
        args->autoReconnect = autoReconnect;
        _imp->renderWatcher->scheduleBlockingTask(NodeRenderWatcher::eBlockingTaskQuitAnyProcessing, args);
    }
} // Node::destroyNodeInternal

KnobIPtr
Node::getKnobByName(const std::string & name) const
{
    ///MT-safe, never changes
    if (!_imp->effect) {
        return KnobIPtr();
    }

    return _imp->effect->getKnobByName(name);
}

NATRON_NAMESPACE_ANONYMOUS_ENTER

///output is always RGBA with alpha = 255
template<typename PIX, int maxValue, int srcNComps>
void
renderPreview(const Image & srcImg,
              int *dstWidth,
              int *dstHeight,
              bool convertToSrgb,
              unsigned int* dstPixels)
{
    ///recompute it after the rescaling
    const RectI & srcBounds = srcImg.getBounds();
    double yZoomFactor = *dstHeight / (double)srcBounds.height();
    double xZoomFactor = *dstWidth / (double)srcBounds.width();
    double zoomFactor;

    if (xZoomFactor < yZoomFactor) {
        zoomFactor = xZoomFactor;
        *dstHeight = srcBounds.height() * zoomFactor;
    } else {
        zoomFactor = yZoomFactor;
        *dstWidth = srcBounds.width() * zoomFactor;
    }

    Image::ReadAccess acc = srcImg.getReadRights();


    for (int i = 0; i < *dstHeight; ++i) {
        double y = (i - *dstHeight / 2.) / zoomFactor + (srcBounds.y1 + srcBounds.y2) / 2.;
        int yi = std::floor(y + 0.5);
        U32 *dst_pixels = dstPixels + *dstWidth * (*dstHeight - 1 - i);
        const PIX* src_pixels = (const PIX*)acc.pixelAt(srcBounds.x1, yi);
        if (!src_pixels) {
            // out of bounds
            for (int j = 0; j < *dstWidth; ++j) {
#ifndef __NATRON_WIN32__
                dst_pixels[j] = toBGRA(0, 0, 0, 0);
#else
                dst_pixels[j] = toBGRA(0, 0, 0, 255);
#endif
            }
        } else {
            for (int j = 0; j < *dstWidth; ++j) {
                // bilinear interpolation is pointless when downscaling a lot, and this is a preview anyway.
                // just use nearest neighbor
                double x = (j - *dstWidth / 2.) / zoomFactor + (srcBounds.x1 + srcBounds.x2) / 2.;
                int xi = std::floor(x + 0.5) - srcBounds.x1;     // round to nearest
                if ( (xi < 0) || ( xi >= (srcBounds.x2 - srcBounds.x1) ) ) {
#ifndef __NATRON_WIN32__
                    dst_pixels[j] = toBGRA(0, 0, 0, 0);
#else
                    dst_pixels[j] = toBGRA(0, 0, 0, 255);
#endif
                } else {
                    float rFilt = src_pixels[xi * srcNComps] / (float)maxValue;
                    float gFilt = srcNComps < 2 ? 0 : src_pixels[xi * srcNComps + 1] / (float)maxValue;
                    float bFilt = srcNComps < 3 ? 0 : src_pixels[xi * srcNComps + 2] / (float)maxValue;
                    if (srcNComps == 1) {
                        gFilt = bFilt = rFilt;
                    }
                    int r = Color::floatToInt<256>(convertToSrgb ? Color::to_func_srgb(rFilt) : rFilt);
                    int g = Color::floatToInt<256>(convertToSrgb ? Color::to_func_srgb(gFilt) : gFilt);
                    int b = Color::floatToInt<256>(convertToSrgb ? Color::to_func_srgb(bFilt) : bFilt);
                    dst_pixels[j] = toBGRA(r, g, b, 255);
                }
            }
        }
    }
}     // renderPreview

///output is always RGBA with alpha = 255
template<typename PIX, int maxValue>
void
renderPreviewForDepth(const Image & srcImg,
                      int elemCount,
                      int *dstWidth,
                      int *dstHeight,
                      bool convertToSrgb,
                      unsigned int* dstPixels)
{
    switch (elemCount) {
    case 0:

        return;
    case 1:
        renderPreview<PIX, maxValue, 1>(srcImg, dstWidth, dstHeight, convertToSrgb, dstPixels);
        break;
    case 2:
        renderPreview<PIX, maxValue, 2>(srcImg, dstWidth, dstHeight, convertToSrgb, dstPixels);
        break;
    case 3:
        renderPreview<PIX, maxValue, 3>(srcImg, dstWidth, dstHeight, convertToSrgb, dstPixels);
        break;
    case 4:
        renderPreview<PIX, maxValue, 4>(srcImg, dstWidth, dstHeight, convertToSrgb, dstPixels);
        break;
    default:
        break;
    }
}     // renderPreviewForDepth

NATRON_NAMESPACE_ANONYMOUS_EXIT


bool
Node::makePreviewImage(SequenceTime time,
                       int *width,
                       int *height,
                       unsigned int* buf)
{
    if (!isNodeCreated()) {
        return false;
    }


    {
        QMutexLocker k(&_imp->isBeingDestroyedMutex);
        if (_imp->isBeingDestroyed) {
            return false;
        }
    }

    if ( _imp->checkForExitPreview() ) {
        return false;
    }


    /// prevent 2 previews to occur at the same time since there's only 1 preview instance
    ComputingPreviewSetter_RAII computingPreviewRAII( _imp.get() );

    EffectInstancePtr effect;
    NodeGroupPtr isGroup = isEffectNodeGroup();
    if (isGroup) {
        NodePtr outputNode = isGroup->getOutputNodeInput();
        if (outputNode) {
            return outputNode->makePreviewImage(time, width, height, buf);
        }
        return false;
    } else {
        effect = _imp->effect;
    }

    if (!effect) {
        return false;
    }

    effect->clearPersistentMessage(false);


    const double par = effect->getAspectRatio(-1);


    NodePtr thisNode = shared_from_this();
    RenderingFlagSetter flagIsRendering(thisNode);


    {
        AbortableRenderInfoPtr abortInfo = AbortableRenderInfo::create(true, 0);
        const bool isRenderUserInteraction = true;
        const bool isSequentialRender = false;
        AbortableThread* isAbortable = dynamic_cast<AbortableThread*>( QThread::currentThread() );
        if (isAbortable) {
            isAbortable->setAbortInfo( isRenderUserInteraction, abortInfo, thisNode->getEffectInstance() );
        }


        ParallelRenderArgsSetter::CtorArgsPtr tlsArgs(new ParallelRenderArgsSetter::CtorArgs);
        tlsArgs->time = time;
        tlsArgs->view = ViewIdx(0);
        tlsArgs->isRenderUserInteraction = isRenderUserInteraction;
        tlsArgs->isSequential = isSequentialRender;
        tlsArgs->abortInfo = abortInfo;
        tlsArgs->treeRoot = thisNode;
        tlsArgs->textureIndex = 0;
        tlsArgs->timeline = getApp()->getTimeLine();
        tlsArgs->activeRotoPaintNode = NodePtr();
        tlsArgs->activeRotoDrawableItem = RotoDrawableItemPtr();
        tlsArgs->isDoingRotoNeatRender = false;
        tlsArgs->isAnalysis = false;
        tlsArgs->draftMode = true;
        tlsArgs->stats = RenderStatsPtr();

        boost::shared_ptr<ParallelRenderArgsSetter> frameRenderArgs;
        try {
            frameRenderArgs.reset(new ParallelRenderArgsSetter(tlsArgs));
        } catch (...) {
            return false;
        }

        U64 nodeHash;
        bool gotHash = effect->getRenderHash(time, ViewIdx(0), &nodeHash);
        assert(gotHash);
        (void)gotHash;

        RenderScale scale(1.);
        RectD rod;
        StatusEnum stat = effect->getRegionOfDefinition_public(nodeHash, time, scale, ViewIdx(0), &rod);
        if ( (stat == eStatusFailed) || rod.isNull() ) {
            return false;
        }
        assert( !rod.isNull() );

        double yZoomFactor = (double)*height / (double)rod.height();
        double xZoomFactor = (double)*width / (double)rod.width();
        double closestPowerOf2X = xZoomFactor >= 1 ? 1 : std::pow( 2, -std::ceil( std::log(xZoomFactor) / std::log(2.) ) );
        double closestPowerOf2Y = yZoomFactor >= 1 ? 1 : std::pow( 2, -std::ceil( std::log(yZoomFactor) / std::log(2.) ) );
        int closestPowerOf2 = std::max(closestPowerOf2X, closestPowerOf2Y);
        unsigned int mipMapLevel = std::min(std::log( (double)closestPowerOf2 ) / std::log(2.), 5.);
        scale.x = Image::getScaleFromMipMapLevel(mipMapLevel);
        scale.y = scale.x;


        RectI renderWindow;
        rod.toPixelEnclosing(mipMapLevel, par, &renderWindow);

        stat = frameRenderArgs->computeRequestPass(mipMapLevel, rod);
        if (stat == eStatusFailed) {
            return false;
        }

        std::list<ImageComponents> requestedComps;
        ImageBitDepthEnum depth = effect->getBitDepth(-1);
        requestedComps.push_back( effect->getComponents(-1) );


        // Exceptions are caught because the program can run without a preview,
        // but any exception in renderROI is probably fatal.
        std::map<ImageComponents, ImagePtr> planes;
        try {
            boost::scoped_ptr<EffectInstance::RenderRoIArgs> renderArgs( new EffectInstance::RenderRoIArgs(time,
                                                                                                           scale,
                                                                                                           mipMapLevel,
                                                                                                           ViewIdx(0), //< preview only renders view 0 (left)
                                                                                                           false,
                                                                                                           renderWindow,
                                                                                                           rod,
                                                                                                           requestedComps, //< preview is always rgb...
                                                                                                           depth,
                                                                                                           false,
                                                                                                           effect,
                                                                                                           eStorageModeRAM /*returnStorage*/,
                                                                                                           time /*callerRenderTime*/) );
            EffectInstance::RenderRoIRetCode retCode;
            retCode = effect->renderRoI(*renderArgs, &planes);
            if (retCode != EffectInstance::eRenderRoIRetCodeOk) {
                return false;
            }
        } catch (...) {
            return false;
        }

        if ( planes.empty() ) {
            return false;
        }

        const ImagePtr& img = planes.begin()->second;
        const ImageComponents& components = img->getComponents();
        int elemCount = components.getNumComponents();

        ///we convert only when input is Linear.
        //Rec709 and srGB is acceptable for preview
        bool convertToSrgb = getApp()->getDefaultColorSpaceForBitDepth( img->getBitDepth() ) == eViewerColorSpaceLinear;

        switch ( img->getBitDepth() ) {
        case eImageBitDepthByte: {
            renderPreviewForDepth<unsigned char, 255>(*img, elemCount, width, height, convertToSrgb, buf);
            break;
        }
        case eImageBitDepthShort: {
            renderPreviewForDepth<unsigned short, 65535>(*img, elemCount, width, height, convertToSrgb, buf);
            break;
        }
        case eImageBitDepthHalf:
            break;
        case eImageBitDepthFloat: {
            renderPreviewForDepth<float, 1>(*img, elemCount, width, height, convertToSrgb, buf);
            break;
        }
        case eImageBitDepthNone:
            break;
        }
    } // ParallelRenderArgsSetter

    ///Exit of the thread
    appPTR->getAppTLS()->cleanupTLSForThread();

    return true;
} // makePreviewImage

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

bool
Node::isOpenFXNode() const
{
    ///MT-safe, never changes
    return _imp->effect->isOpenFX();
}


/**
 * @brief Returns true if the node is a rotopaint node
 **/
bool
Node::isRotoPaintingNode() const
{
    return _imp->effect ? _imp->effect->isRotoPaintNode() : false;
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

OutputEffectInstancePtr
Node::isEffectOutput() const
{
    return toOutputEffectInstance(_imp->effect);
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

void
Node::setKnobsFrozen(bool frozen)
{
    ///MT-safe from EffectInstance::setKnobsFrozen
    _imp->effect->setKnobsFrozen(frozen);

    QMutexLocker l(&_imp->inputsMutex);
    for (std::size_t i = 0; i < _imp->inputs.size(); ++i) {
        NodePtr input = _imp->inputs[i].lock();
        if (input) {
            input->setKnobsFrozen(frozen);
        }
    }
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


int
Node::getMaxInputCount() const
{
    ///MT-safe, never changes
    assert(_imp->effect);

    return _imp->effect->getMaxInputCount();
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
Node::isEntitledForInspectorInputsStyle() const
{

    // Find the number of inputs that share the same basename

    int nInputsWithSameBasename = 0;

    std::string baseInputName;

    // We need a boolean here because the baseInputName may be empty in the case input names only contain numbers
    bool baseInputNameSet = false;

    int maxInputs = getMaxInputCount();
    for (int i = 0; i < maxInputs; ++i) {
        if (!baseInputNameSet) {
            baseInputName = removeTrailingDigits(getInputLabel(i));
            baseInputNameSet = true;
            nInputsWithSameBasename = 1;
        } else {
            std::string thisBaseName = removeTrailingDigits(getInputLabel(i));
            if (thisBaseName == baseInputName) {
                ++nInputsWithSameBasename;
            }
        }

    }
    return maxInputs > 4 && nInputsWithSameBasename >= 4;
}

bool
Node::makePreviewByDefault() const
{
    ///MT-safe, never changes
    assert(_imp->effect);

    return _imp->effect->makePreviewByDefault();
}

void
Node::togglePreview()
{
    ///MT-safe from Knob
    KnobBoolPtr b = _imp->previewEnabledKnob.lock();
    if (!b) {
        return;
    }
    b->setValue( !b->getValue() );
}

bool
Node::isPreviewEnabled() const
{
    ///MT-safe from EffectInstance
    KnobBoolPtr b = _imp->previewEnabledKnob.lock();
    if (!b) {
        return false;
    }

    return b->getValue();
}

bool
Node::aborted() const
{
    ///MT-safe from EffectInstance
    assert(_imp->effect);

    return _imp->effect->aborted();
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

void
Node::purgeAllInstancesCaches()
{
    ///Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    assert(_imp->effect);
    _imp->effect->purgeCaches();
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

void
Node::setOutputFilesForWriter(const std::string & pattern)
{
    assert(_imp->effect);
    _imp->effect->setOutputFilesForWriter(pattern);
}

void
Node::registerPluginMemory(size_t nBytes)
{
    {
        QMutexLocker l(&_imp->memoryUsedMutex);
        _imp->pluginInstanceMemoryUsed += nBytes;
    }
    Q_EMIT pluginMemoryUsageChanged(nBytes);
}

void
Node::unregisterPluginMemory(size_t nBytes)
{
    {
        QMutexLocker l(&_imp->memoryUsedMutex);
        _imp->pluginInstanceMemoryUsed -= nBytes;
    }
    Q_EMIT pluginMemoryUsageChanged(-nBytes);
}

QMutex &
Node::getRenderInstancesSharedMutex()
{
    return _imp->renderInstancesSharedMutex;
}

static void
refreshPreviewsRecursivelyUpstreamInternal(double time,
                                           const NodePtr& node,
                                           std::list<NodePtr>& marked)
{
    if ( std::find(marked.begin(), marked.end(), node) != marked.end() ) {
        return;
    }

    if ( node->isPreviewEnabled() ) {
        node->refreshPreviewImage( time );
    }

    marked.push_back(node);

    std::vector<NodeWPtr > inputs = node->getInputs_copy();

    for (std::size_t i = 0; i < inputs.size(); ++i) {
        NodePtr input = inputs[i].lock();
        if (input) {
            input->refreshPreviewsRecursivelyUpstream(time);
        }
    }
}

void
Node::refreshPreviewsRecursivelyUpstream(double time)
{
    std::list<NodePtr> marked;

    refreshPreviewsRecursivelyUpstreamInternal(time, shared_from_this(), marked);
}

static void
refreshPreviewsRecursivelyDownstreamInternal(double time,
                                             const NodePtr& node,
                                             std::list<NodePtr>& marked)
{
    if ( std::find(marked.begin(), marked.end(), node) != marked.end() ) {
        return;
    }

    if ( node->isPreviewEnabled() ) {
        node->refreshPreviewImage( time );
    }

    marked.push_back(node);

    NodesList outputs;
    node->getOutputsWithGroupRedirection(outputs);
    for (NodesList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        (*it)->refreshPreviewsRecursivelyDownstream(time);
    }
}

void
Node::refreshPreviewsRecursivelyDownstream(double time)
{
    if ( !getNodeGui() ) {
        return;
    }
    std::list<NodePtr> marked;
    refreshPreviewsRecursivelyDownstreamInternal(time, shared_from_this(), marked);
}

void
Node::onAllKnobsSlaved(bool isSlave,
                       const KnobHolderPtr& master)
{
    ///Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    if (isSlave) {
        EffectInstancePtr effect = toEffectInstance(master);
        assert(effect);
        if (effect) {
            NodePtr masterNode = effect->getNode();
            {
                QMutexLocker l(&_imp->masterNodeMutex);
                _imp->masterNode = masterNode;
            }
            QObject::connect( masterNode.get(), SIGNAL(deactivated(bool)), this, SLOT(onMasterNodeDeactivated()) );
            QObject::connect( masterNode.get(), SIGNAL(previewImageChanged(double)), this, SLOT(refreshPreviewImage(double)) );
        }
    } else {
        NodePtr master = getMasterNode();
        QObject::disconnect( master.get(), SIGNAL(deactivated(bool)), this, SLOT(onMasterNodeDeactivated()) );
        QObject::disconnect( master.get(), SIGNAL(previewImageChanged(double)), this, SLOT(refreshPreviewImage(double)) );
        {
            QMutexLocker l(&_imp->masterNodeMutex);
            _imp->masterNode.reset();
        }
    }

    Q_EMIT allKnobsSlaved(isSlave);
}

void
Node::onKnobSlaved(const KnobIPtr& slave,
                   const KnobIPtr& master,
                   DimIdx /*dimension*/,
                   ViewIdx /*view*/,
                   bool isSlave)
{
    // If this is a unslave action, remove the clone state
    if (!isSlave) {
        NodePtr masterNode = getMasterNode();
        if ( masterNode ) {
            onAllKnobsSlaved(false, KnobHolderPtr());
        }
    }

    if (slave == master) {
        return;
    }

    ///If the holder isn't an effect, ignore it too
    EffectInstancePtr isEffect = toEffectInstance( master->getHolder() );
    NodePtr parentNode;
    if (!isEffect) {
        KnobTableItemPtr isItem = toKnobTableItem(master->getHolder());
        if (isItem) {
            parentNode = isItem->getModel()->getNode();
        }
    } else {
        parentNode = isEffect->getNode();
    }

    bool changed = false;
    {
        QMutexLocker l(&_imp->masterNodeMutex);
        KnobLinkList::iterator found = _imp->nodeLinks.end();
        for (KnobLinkList::iterator it = _imp->nodeLinks.begin(); it != _imp->nodeLinks.end(); ++it) {
            if (it->slaveKnob.lock() == slave) {
                found = it;
                break;
            }
        }

        if ( found == _imp->nodeLinks.end() ) {
            if (!isSlave) {
                ///We want to unslave from the given node but the link didn't existed, just return
                return;
            } else {
                ///Add a new link
                KnobLink link;
                link.masterNode = parentNode;
                link.slaveKnob = slave;
                link.masterKnob = master;
                _imp->nodeLinks.push_back(link);
                changed = true;
            }
        } else if ( found != _imp->nodeLinks.end() ) {
            if (isSlave) {
                ///We want to slave to the given node but it already has a link on another parameter, just return
                return;
            } else {
                ///Remove the given link
                _imp->nodeLinks.erase(found);
                changed = true;
            }
        }
    }
    if (changed) {
        Q_EMIT knobsLinksChanged();
    }
} // onKnobSlaved

void
Node::getKnobsLinks(std::list<KnobLink> & links) const
{
    QMutexLocker l(&_imp->masterNodeMutex);

    links = _imp->nodeLinks;
}

void
Node::onMasterNodeDeactivated()
{
    ///Only called by the main-thread
    assert( QThread::currentThread() == qApp->thread() );
    if (!_imp->effect) {
        return;
    }
    _imp->effect->unslaveAllKnobs();
}

NodePtr
Node::getIOContainer() const
{
    return _imp->ioContainer.lock();
}


NodePtr
Node::getMasterNode() const
{
    QMutexLocker l(&_imp->masterNodeMutex);

    return _imp->masterNode.lock();
}

bool
Node::isSupportedComponent(int inputNb,
                           const ImageComponents& comp) const
{
    QMutexLocker l(&_imp->inputsMutex);

    if (inputNb >= 0) {
        assert( inputNb < (int)_imp->inputsComponents.size() );
        std::list<ImageComponents>::const_iterator found =
            std::find(_imp->inputsComponents[inputNb].begin(), _imp->inputsComponents[inputNb].end(), comp);

        return found != _imp->inputsComponents[inputNb].end();
    } else {
        assert(inputNb == -1);
        std::list<ImageComponents>::const_iterator found =
            std::find(_imp->outputComponents.begin(), _imp->outputComponents.end(), comp);

        return found != _imp->outputComponents.end();
    }
}

ImageComponents
Node::findClosestInList(const ImageComponents& comp,
                        const std::list<ImageComponents> &components,
                        bool multiPlanar)
{
    if ( components.empty() ) {
        return ImageComponents::getNoneComponents();
    }
    std::list<ImageComponents>::const_iterator closestComp = components.end();
    for (std::list<ImageComponents>::const_iterator it = components.begin(); it != components.end(); ++it) {
        if ( closestComp == components.end() ) {
            if ( multiPlanar && ( it->getNumComponents() == comp.getNumComponents() ) ) {
                return comp;
            }
            closestComp = it;
        } else {
            if ( it->getNumComponents() == comp.getNumComponents() ) {
                if (multiPlanar) {
                    return comp;
                }
                closestComp = it;
                break;
            } else {
                int diff = it->getNumComponents() - comp.getNumComponents();
                int diffSoFar = closestComp->getNumComponents() - comp.getNumComponents();
                if ( (diff > 0) && (diff < diffSoFar) ) {
                    closestComp = it;
                }
            }
        }
    }
    if ( closestComp == components.end() ) {
        return ImageComponents::getNoneComponents();
    }

    return *closestComp;
}

ImageComponents
Node::findClosestSupportedComponents(int inputNb,
                                     const ImageComponents& comp) const
{
    std::list<ImageComponents> comps;
    {
        QMutexLocker l(&_imp->inputsMutex);

        if (inputNb >= 0) {
            assert( inputNb < (int)_imp->inputsComponents.size() );
            comps = _imp->inputsComponents[inputNb];
        } else {
            assert(inputNb == -1);
            comps = _imp->outputComponents;
        }
    }

    return findClosestInList( comp, comps, _imp->effect->isMultiPlanar() );
}

int
Node::isMaskChannelKnob(const KnobIConstPtr& knob) const
{
    for (std::map<int, MaskSelector >::const_iterator it = _imp->maskSelectors.begin(); it != _imp->maskSelectors.end(); ++it) {
        if (it->second.channel.lock() == knob) {
            return it->first;
        }
    }

    return -1;
}

bool
Node::isMaskEnabled(int inputNb) const
{
    std::map<int, MaskSelector >::const_iterator it = _imp->maskSelectors.find(inputNb);

    if ( it != _imp->maskSelectors.end() ) {
        return it->second.enabled.lock()->getValue();
    } else {
        return true;
    }
}

void
Node::lock(const ImagePtr & image)
{
    QMutexLocker l(&_imp->imagesBeingRenderedMutex);
    std::list<ImagePtr >::iterator it =
        std::find(_imp->imagesBeingRendered.begin(), _imp->imagesBeingRendered.end(), image);

    while ( it != _imp->imagesBeingRendered.end() ) {
        _imp->imageBeingRenderedCond.wait(&_imp->imagesBeingRenderedMutex);
        it = std::find(_imp->imagesBeingRendered.begin(), _imp->imagesBeingRendered.end(), image);
    }
    ///Okay the image is not used by any other thread, claim that we want to use it
    assert( it == _imp->imagesBeingRendered.end() );
    _imp->imagesBeingRendered.push_back(image);
}

bool
Node::tryLock(const ImagePtr & image)
{
    QMutexLocker l(&_imp->imagesBeingRenderedMutex);
    std::list<ImagePtr >::iterator it =
        std::find(_imp->imagesBeingRendered.begin(), _imp->imagesBeingRendered.end(), image);

    if ( it != _imp->imagesBeingRendered.end() ) {
        return false;
    }
    ///Okay the image is not used by any other thread, claim that we want to use it
    assert( it == _imp->imagesBeingRendered.end() );
    _imp->imagesBeingRendered.push_back(image);

    return true;
}

void
Node::unlock(const ImagePtr & image)
{
    QMutexLocker l(&_imp->imagesBeingRenderedMutex);
    std::list<ImagePtr >::iterator it =
        std::find(_imp->imagesBeingRendered.begin(), _imp->imagesBeingRendered.end(), image);

    ///The image must exist, otherwise this is a bug
    assert( it != _imp->imagesBeingRendered.end() );
    _imp->imagesBeingRendered.erase(it);
    ///Notify all waiting threads that we're finished
    _imp->imageBeingRenderedCond.wakeAll();
}

ImagePtr
Node::getImageBeingRendered(double time,
                            unsigned int mipMapLevel,
                            ViewIdx view)
{
    QMutexLocker l(&_imp->imagesBeingRenderedMutex);

    for (std::list<ImagePtr >::iterator it = _imp->imagesBeingRendered.begin();
         it != _imp->imagesBeingRendered.end(); ++it) {
        const ImageKey &key = (*it)->getKey();
        if ( (key._view == view) && ( (*it)->getMipMapLevel() == mipMapLevel ) && (key._time == time) ) {
            return *it;
        }
    }

    return ImagePtr();
}

void
Node::beginInputEdition()
{
    assert( QThread::currentThread() == qApp->thread() );
    ++_imp->inputModifiedRecursion;
}

void
Node::endInputEdition(bool triggerRender)
{
    assert( QThread::currentThread() == qApp->thread() );
    if (_imp->inputModifiedRecursion > 0) {
        --_imp->inputModifiedRecursion;
    }

    if (!_imp->inputModifiedRecursion) {
        bool hasChanged = !_imp->inputsModified.empty();
        _imp->inputsModified.clear();

        if (hasChanged) {
            // When creating a node tree, the refresh is done on all the tree when done
            if ( !getApp()->isCreatingNodeTree() ) {
                forceRefreshAllInputRelatedData();
            }
            refreshDynamicProperties();
        }

        triggerRender = triggerRender && hasChanged;

        if (triggerRender) {
            std::list<ViewerInstancePtr> viewers;
            hasViewersConnected(&viewers);
            for (std::list<ViewerInstancePtr>::iterator it2 = viewers.begin(); it2 != viewers.end(); ++it2) {
                (*it2)->renderCurrentFrame(true);
            }
        }
    }
}

void
Node::onInputChanged(int inputNb)
{
    if ( getApp()->getProject()->isProjectClosing() ) {
        return;
    }
    assert( QThread::currentThread() == qApp->thread() );

    bool mustCallEndInputEdition = _imp->inputModifiedRecursion == 0;
    if (mustCallEndInputEdition) {
        beginInputEdition();
    }

    refreshMaskEnabledNess(inputNb);
    //refreshLayersSelectorsVisibility();

    bool shouldDoInputChanged = ( !getApp()->getProject()->isProjectClosing() /*&& !getApp()->isCreatingNodeTree() */);

    if (shouldDoInputChanged) {
        ///When loading a group (or project) just wait until everything is setup to actually compute input
        ///related data such as clip preferences
        ///Exception for the Rotopaint node which needs to setup its own graph internally


        ///Don't do clip preferences while loading a project, they will be refreshed globally once the project is loaded.
        _imp->effect->onInputChanged(inputNb);
        _imp->inputsModified.insert(inputNb);

        // If the effect has render clones, kill them as the plug-in might have changed its internal state
        _imp->effect->clearRenderInstances();

        //A knob value might have changed recursively, redraw  any overlay
        if ( !_imp->effect->isDequeueingValuesSet() &&
             ( _imp->effect->getRecursionLevel() == 0) && _imp->effect->checkIfOverlayRedrawNeeded() ) {
            _imp->effect->redrawOverlayInteract();
        }
    }

    /*
       If this is a group, also notify the output nodes of the GroupInput node inside the Group corresponding to
       the this inputNb
     */
    NodeGroupPtr isGroup = isEffectNodeGroup();
    if (isGroup) {
        std::vector<NodePtr> groupInputs;
        isGroup->getInputs(&groupInputs);
        if ( (inputNb >= 0) && ( inputNb < (int)groupInputs.size() ) && groupInputs[inputNb] ) {
            std::map<NodePtr, int> inputOutputs;
            groupInputs[inputNb]->getOutputsConnectedToThisNode(&inputOutputs);
            for (std::map<NodePtr, int> ::iterator it = inputOutputs.begin(); it != inputOutputs.end(); ++it) {
                it->first->onInputChanged(it->second);
            }
        }
    }

    /*
       If this is an output node, notify the Group output nodes that their input have changed.
     */
    GroupOutputPtr isGroupOutput = isEffectGroupOutput();
    if (isGroupOutput) {
        NodeGroupPtr containerGroup = toNodeGroup( isGroupOutput->getNode()->getGroup() );
        if (containerGroup) {
            std::map<NodePtr, int> groupOutputs;
            containerGroup->getNode()->getOutputsConnectedToThisNode(&groupOutputs);
            for (std::map<NodePtr, int> ::iterator it = groupOutputs.begin(); it != groupOutputs.end(); ++it) {
                it->first->onInputChanged(it->second);
            }
        }
    }

    /*
     * If this node is the output of a pre-comp, notify the precomp output nodes that their input have changed
     */
    PrecompNodePtr isInPrecomp = isPartOfPrecomp();
    if ( isInPrecomp && (isInPrecomp->getOutputNode().get() == this) ) {
        std::map<NodePtr, int> inputOutputs;
        isInPrecomp->getNode()->getOutputsConnectedToThisNode(&inputOutputs);
        for (std::map<NodePtr, int> ::iterator it = inputOutputs.begin(); it != inputOutputs.end(); ++it) {
            it->first->onInputChanged(it->second);
        }
    }

    if (mustCallEndInputEdition) {
        endInputEdition(true);
    }
} // Node::onInputChanged

void
Node::onParentMultiInstanceInputChanged(int input)
{
    ++_imp->inputModifiedRecursion;
    _imp->effect->onInputChanged(input);
    --_imp->inputModifiedRecursion;
}

bool
Node::duringInputChangedAction() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->inputModifiedRecursion > 0;
}

void
Node::onFileNameParameterChanged(const KnobIPtr& fileKnob)
{
    if ( _imp->effect->isReader() ) {
        computeFrameRangeForReader(fileKnob, true);

        ///Refresh the preview automatically if the filename changed

        ///union the project frame range if not locked with the reader frame range
        bool isLocked = getApp()->getProject()->isFrameRangeLocked();
        if (!isLocked) {
            double leftBound = INT_MIN, rightBound = INT_MAX;
            _imp->effect->getFrameRange_public(0, &leftBound, &rightBound);

            if ( (leftBound != INT_MIN) && (rightBound != INT_MAX) ) {
                if ( getGroup() || getIOContainer() ) {
                    getApp()->getProject()->unionFrameRangeWith(leftBound, rightBound);
                }
            }
        }
    } else if ( _imp->effect->isWriter() ) {
        KnobFilePtr isFile = toKnobFile(fileKnob);
        KnobStringPtr sublabel = _imp->ofxSubLabelKnob.lock();
        if (isFile && sublabel) {

            std::string pattern = isFile->getValue();
            if (sublabel) {
                std::size_t foundSlash = pattern.find_last_of("/");
                if (foundSlash != std::string::npos) {
                    pattern = pattern.substr(foundSlash + 1);
                }

                sublabel->setValue(pattern);
            }
        }

        /*
           Check if the filename param has a %V in it, in which case make sure to hide the Views parameter
         */
        if (isFile) {
            std::string pattern = isFile->getValue();
            std::size_t foundViewPattern = pattern.find_first_of("%v");
            if (foundViewPattern == std::string::npos) {
                foundViewPattern = pattern.find_first_of("%V");
            }
            if (foundViewPattern != std::string::npos) {
                //We found view pattern
                KnobIPtr viewsKnob = getKnobByName(kWriteOIIOParamViewsSelector);
                if (viewsKnob) {
                    KnobChoicePtr viewsSelector = toKnobChoice(viewsKnob);
                    if (viewsSelector) {
                        viewsSelector->setSecret(true);
                    }
                }
            }
        }
    }
} // Node::onFileNameParameterChanged

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
                std::string pattern = isFile->getValue();
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
Node::canHandleRenderScaleForOverlays() const
{
    return _imp->effect->canHandleRenderScaleForOverlays();
}

bool
Node::shouldDrawOverlay() const
{
    if ( !hasOverlay() ) {
        return false;
    }

    if (!_imp->isPersistent) {
        return false;
    }

    if ( !isActivated() ) {
        return false;
    }

    if ( isNodeDisabled() ) {
        return false;
    }

    if ( !isEffectViewerNode() && !isSettingsPanelVisible() ) {
        return false;
    }

    if ( isSettingsPanelMinimized() ) {
        return false;
    }


    return true;
}

void
Node::drawHostOverlay(double time,
                      const RenderScale& renderScale,
                      ViewIdx view)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        nodeGui->drawHostOverlay(time, renderScale, view);
    }
}

bool
Node::onOverlayPenDownDefault(double time,
                              const RenderScale& renderScale,
                              ViewIdx view,
                              const QPointF & viewportPos,
                              const QPointF & pos,
                              double pressure)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayPenDownDefault(time, renderScale, view, viewportPos, pos, pressure);
    }

    return false;
}

bool
Node::onOverlayPenDoubleClickedDefault(double time,
                                       const RenderScale& renderScale,
                                       ViewIdx view,
                                       const QPointF & viewportPos,
                                       const QPointF & pos)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayPenDoubleClickedDefault(time, renderScale, view, viewportPos, pos);
    }

    return false;
}

bool
Node::onOverlayPenMotionDefault(double time,
                                const RenderScale& renderScale,
                                ViewIdx view,
                                const QPointF & viewportPos,
                                const QPointF & pos,
                                double pressure)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayPenMotionDefault(time, renderScale, view, viewportPos, pos, pressure);
    }

    return false;
}

bool
Node::onOverlayPenUpDefault(double time,
                            const RenderScale& renderScale,
                            ViewIdx view,
                            const QPointF & viewportPos,
                            const QPointF & pos,
                            double pressure)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayPenUpDefault(time, renderScale, view, viewportPos, pos, pressure);
    }

    return false;
}

bool
Node::onOverlayKeyDownDefault(double time,
                              const RenderScale& renderScale,
                              ViewIdx view,
                              Key key,
                              KeyboardModifiers modifiers)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayKeyDownDefault(time, renderScale, view, key, modifiers);
    }

    return false;
}

bool
Node::onOverlayKeyUpDefault(double time,
                            const RenderScale& renderScale,
                            ViewIdx view,
                            Key key,
                            KeyboardModifiers modifiers)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayKeyUpDefault(time, renderScale, view, key, modifiers);
    }

    return false;
}

bool
Node::onOverlayKeyRepeatDefault(double time,
                                const RenderScale& renderScale,
                                ViewIdx view,
                                Key key,
                                KeyboardModifiers modifiers)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayKeyRepeatDefault(time, renderScale, view, key, modifiers);
    }

    return false;
}

bool
Node::onOverlayFocusGainedDefault(double time,
                                  const RenderScale& renderScale,
                                  ViewIdx view)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayFocusGainedDefault(time, renderScale, view);
    }

    return false;
}

bool
Node::onOverlayFocusLostDefault(double time,
                                const RenderScale& renderScale,
                                ViewIdx view)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->onOverlayFocusLostDefault(time, renderScale, view);
    }

    return false;
}

void
Node::removePositionHostOverlay(const KnobIPtr& knob)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        nodeGui->removePositionHostOverlay(knob);
    }
}

void
Node::addPositionInteract(const KnobDoublePtr& position,
                          const KnobBoolPtr& interactive)
{
    assert( QThread::currentThread() == qApp->thread() );
    if ( appPTR->isBackground() ) {
        return;
    }

    boost::shared_ptr<HostOverlayKnobsPosition> knobs( new HostOverlayKnobsPosition() );
    knobs->addKnob(position, HostOverlayKnobsPosition::eKnobsEnumerationPosition);
    if (interactive) {
        knobs->addKnob(interactive, HostOverlayKnobsPosition::eKnobsEnumerationInteractive);
    }
    NodeGuiIPtr nodeGui = getNodeGui();
    if (!nodeGui) {
        _imp->nativeOverlays.push_back(knobs);
    } else {
        nodeGui->addDefaultInteract(knobs);
    }
}

void
Node::addTransformInteract(const KnobDoublePtr& translate,
                           const KnobDoublePtr& scale,
                           const KnobBoolPtr& scaleUniform,
                           const KnobDoublePtr& rotate,
                           const KnobDoublePtr& skewX,
                           const KnobDoublePtr& skewY,
                           const KnobChoicePtr& skewOrder,
                           const KnobDoublePtr& center,
                           const KnobBoolPtr& invert,
                           const KnobBoolPtr& interactive)
{
    assert( QThread::currentThread() == qApp->thread() );
    if ( appPTR->isBackground() ) {
        return;
    }

    boost::shared_ptr<HostOverlayKnobsTransform> knobs( new HostOverlayKnobsTransform() );
    knobs->addKnob(translate, HostOverlayKnobsTransform::eKnobsEnumerationTranslate);
    knobs->addKnob(scale, HostOverlayKnobsTransform::eKnobsEnumerationScale);
    knobs->addKnob(scaleUniform, HostOverlayKnobsTransform::eKnobsEnumerationUniform);
    knobs->addKnob(rotate, HostOverlayKnobsTransform::eKnobsEnumerationRotate);
    knobs->addKnob(center, HostOverlayKnobsTransform::eKnobsEnumerationCenter);
    knobs->addKnob(skewX, HostOverlayKnobsTransform::eKnobsEnumerationSkewx);
    knobs->addKnob(skewY, HostOverlayKnobsTransform::eKnobsEnumerationSkewy);
    knobs->addKnob(skewOrder, HostOverlayKnobsTransform::eKnobsEnumerationSkewOrder);
    if (invert) {
        knobs->addKnob(invert, HostOverlayKnobsTransform::eKnobsEnumerationInvert);
    }
    if (interactive) {
        knobs->addKnob(interactive, HostOverlayKnobsTransform::eKnobsEnumerationInteractive);
    }
    NodeGuiIPtr nodeGui = getNodeGui();
    if (!nodeGui) {
        _imp->nativeOverlays.push_back(knobs);
    } else {
        nodeGui->addDefaultInteract(knobs);
    }
}

void
Node::addCornerPinInteract(const KnobDoublePtr& from1,
                           const KnobDoublePtr& from2,
                           const KnobDoublePtr& from3,
                           const KnobDoublePtr& from4,
                           const KnobDoublePtr& to1,
                           const KnobDoublePtr& to2,
                           const KnobDoublePtr& to3,
                           const KnobDoublePtr& to4,
                           const KnobBoolPtr& enable1,
                           const KnobBoolPtr& enable2,
                           const KnobBoolPtr& enable3,
                           const KnobBoolPtr& enable4,
                           const KnobChoicePtr& overlayPoints,
                           const KnobBoolPtr& invert,
                           const KnobBoolPtr& interactive)
{
    assert( QThread::currentThread() == qApp->thread() );
    if ( appPTR->isBackground() ) {
        return;
    }
    boost::shared_ptr<HostOverlayKnobsCornerPin> knobs( new HostOverlayKnobsCornerPin() );
    knobs->addKnob(from1, HostOverlayKnobsCornerPin::eKnobsEnumerationFrom1);
    knobs->addKnob(from2, HostOverlayKnobsCornerPin::eKnobsEnumerationFrom2);
    knobs->addKnob(from3, HostOverlayKnobsCornerPin::eKnobsEnumerationFrom3);
    knobs->addKnob(from4, HostOverlayKnobsCornerPin::eKnobsEnumerationFrom4);

    knobs->addKnob(to1, HostOverlayKnobsCornerPin::eKnobsEnumerationTo1);
    knobs->addKnob(to2, HostOverlayKnobsCornerPin::eKnobsEnumerationTo2);
    knobs->addKnob(to3, HostOverlayKnobsCornerPin::eKnobsEnumerationTo3);
    knobs->addKnob(to4, HostOverlayKnobsCornerPin::eKnobsEnumerationTo4);

    knobs->addKnob(enable1, HostOverlayKnobsCornerPin::eKnobsEnumerationEnable1);
    knobs->addKnob(enable2, HostOverlayKnobsCornerPin::eKnobsEnumerationEnable2);
    knobs->addKnob(enable3, HostOverlayKnobsCornerPin::eKnobsEnumerationEnable3);
    knobs->addKnob(enable4, HostOverlayKnobsCornerPin::eKnobsEnumerationEnable4);

    knobs->addKnob(overlayPoints, HostOverlayKnobsCornerPin::eKnobsEnumerationOverlayPoints);

    if (invert) {
        knobs->addKnob(invert, HostOverlayKnobsCornerPin::eKnobsEnumerationInvert);
    }
    if (interactive) {
        knobs->addKnob(interactive, HostOverlayKnobsCornerPin::eKnobsEnumerationInteractive);
    }
    NodeGuiIPtr nodeGui = getNodeGui();
    if (!nodeGui) {
        _imp->nativeOverlays.push_back(knobs);
    } else {
        nodeGui->addDefaultInteract(knobs);
    }
}

void
Node::initializeHostOverlays()
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (!nodeGui) {
        return;
    }
    for (std::list<HostOverlayKnobsPtr> ::iterator it = _imp->nativeOverlays.begin(); it != _imp->nativeOverlays.end(); ++it) {
        nodeGui->addDefaultInteract(*it);
    }
    _imp->nativeOverlays.clear();
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



bool
Node::hasHostOverlayForParam(const KnobIConstPtr& knob) const
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if ( nodeGui && nodeGui->hasHostOverlayForParam(knob) ) {
        return true;
    }

    return false;
}

bool
Node::hasHostOverlay() const
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if ( nodeGui && nodeGui->hasHostOverlay() ) {
        return true;
    }

    return false;
}

void
Node::pushUndoCommand(const UndoCommandPtr& command)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        nodeGui->pushUndoCommand(command);
    } else {
        command->redo();
    }
}

void
Node::setCurrentCursor(CursorEnum defaultCursor)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        nodeGui->setCurrentCursor(defaultCursor);
    }
}

bool
Node::setCurrentCursor(const QString& customCursorFilePath)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if (nodeGui) {
        return nodeGui->setCurrentCursor(customCursorFilePath);
    }

    return false;
}

void
Node::setCurrentViewportForHostOverlays(OverlaySupport* viewPort)
{
    NodeGuiIPtr nodeGui = getNodeGui();

    if ( nodeGui && nodeGui->hasHostOverlay() ) {
        nodeGui->setCurrentViewportForHostOverlays(viewPort);
    }
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
    double time = project->currentFrame();
    RenderScale scale(1.);
    double inputTime = 0;
    U64 hash = 0;
    bool viewAware =  _imp->effect->isViewAware();
    int nViews = !viewAware ? 1 : project->getProjectViewsCount();
    Format frmt;
    project->getProjectDefaultFormat(&frmt);

    //The one view node might report it is identity, but we do not want it to display it


    bool isIdentity = false;
    int inputNb = -1;
    OneViewNodePtr isOneView = isEffectOneViewNode();
    if (!isOneView) {
        for (int i = 0; i < nViews; ++i) {
            int identityInputNb = -1;
            ViewIdx identityView;
            bool isViewIdentity = _imp->effect->isIdentity_public(true, hash, time, scale, frmt, ViewIdx(i), &inputTime, &identityView, &identityInputNb);
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

/*
   This is called AFTER the instanceChanged action has been called on the plug-in
 */
bool
Node::onEffectKnobValueChanged(const KnobIPtr& what,
                               ValueChangedReasonEnum reason)
{
    if (!what) {
        return false;
    }
    for (std::map<int, MaskSelector >::iterator it = _imp->maskSelectors.begin(); it != _imp->maskSelectors.end(); ++it) {
        if (it->second.channel.lock() == what) {
            _imp->onMaskSelectorChanged(it->first, it->second);
            break;
        }
    }

    bool ret = true;
    if ( what == _imp->previewEnabledKnob.lock() ) {
        if ( (reason == eValueChangedReasonUserEdited) || (reason == eValueChangedReasonSlaveRefresh) ) {
            Q_EMIT previewKnobToggled();
        }
    } else if ( what == _imp->renderButton.lock() ) {
        if ( getEffectInstance()->isWriter() ) {
            /*if this is a button and it is a render button,we're safe to assume the plug-ins wants to start rendering.*/
            AppInstance::RenderWork w;
            w.writer = isEffectOutput();
            w.firstFrame = INT_MIN;
            w.lastFrame = INT_MAX;
            w.frameStep = INT_MIN;
            w.useRenderStats = getApp()->isRenderStatsActionChecked();
            std::list<AppInstance::RenderWork> works;
            works.push_back(w);
            getApp()->renderWritersNonBlocking(works);
        }
    } else if (what == _imp->disableNodeKnob.lock()) {
        Q_EMIT disabledKnobToggled( _imp->disableNodeKnob.lock()->getValue() );
    } else if ( what == _imp->nodeLabelKnob.lock() || what == _imp->ofxSubLabelKnob.lock() ) {
        Q_EMIT nodeExtraLabelChanged();
    } else if ( what == _imp->hideInputs.lock() ) {
        Q_EMIT hideInputsKnobChanged( _imp->hideInputs.lock()->getValue() );
    } else if ( _imp->effect->isReader() && (what->getName() == kReadOIIOAvailableViewsKnobName) ) {
        refreshCreatedViews(what);
    } else if ( what == _imp->refreshInfoButton.lock() ||
                (what == _imp->infoPage.lock() && reason == eValueChangedReasonUserEdited) ) {
        std::stringstream ssinfo;
        int maxinputs = getMaxInputCount();
        for (int i = 0; i < maxinputs; ++i) {
            std::string inputInfo = makeInfoForInput(i);
            if ( !inputInfo.empty() ) {
                ssinfo << inputInfo << "<br />";
            }
        }
        std::string outputInfo = makeInfoForInput(-1);
        ssinfo << outputInfo << "<br />";
        ssinfo << "<b>" << tr("Supports tiles:").toStdString() << "</b> <font color=#c8c8c8>";
        ssinfo << ( getCurrentSupportTiles() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
        if (_imp->effect) {
            ssinfo << "<b>" << tr("Supports multiresolution:").toStdString() << "</b> <font color=#c8c8c8>";
            ssinfo << ( _imp->effect->supportsMultiResolution() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
            ssinfo << "<b>" << tr("Supports renderscale:").toStdString() << "</b> <font color=#c8c8c8>";
            switch ( _imp->effect->supportsRenderScaleMaybe() ) {
                case EffectInstance::eSupportsMaybe:
                    ssinfo << tr("Maybe").toStdString();
                    break;

                case EffectInstance::eSupportsNo:
                    ssinfo << tr("No").toStdString();
                    break;

                case EffectInstance::eSupportsYes:
                    ssinfo << tr("Yes").toStdString();
                    break;
            }
            ssinfo << "</font><br />";
            ssinfo << "<b>" << tr("Supports multiple clip PARs:").toStdString() << "</b> <font color=#c8c8c8>";
            ssinfo << ( _imp->effect->supportsMultipleClipPARs() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
            ssinfo << "<b>" << tr("Supports multiple clip depths:").toStdString() << "</b> <font color=#c8c8c8>";
            ssinfo << ( _imp->effect->supportsMultipleClipDepths() ? tr("Yes") : tr("No") ).toStdString() << "</font><br />";
        }
        ssinfo << "<b>" << tr("Render thread safety:").toStdString() << "</b> <font color=#c8c8c8>";
        switch ( getCurrentRenderThreadSafety() ) {
            case eRenderSafetyUnsafe:
                ssinfo << tr("Unsafe").toStdString();
                break;

            case eRenderSafetyInstanceSafe:
                ssinfo << tr("Safe").toStdString();
                break;

            case eRenderSafetyFullySafe:
                ssinfo << tr("Fully safe").toStdString();
                break;

            case eRenderSafetyFullySafeFrame:
                ssinfo << tr("Fully safe frame").toStdString();
                break;
        }
        ssinfo << "</font><br />";
        ssinfo << "<b>" << tr("OpenGL Rendering Support:").toStdString() << "</b>: <font color=#c8c8c8>";
        PluginOpenGLRenderSupport glSupport = getCurrentOpenGLRenderSupport();
        switch (glSupport) {
            case ePluginOpenGLRenderSupportNone:
                ssinfo << tr("No").toStdString();
                break;
            case ePluginOpenGLRenderSupportNeeded:
                ssinfo << tr("Yes but CPU rendering is not supported").toStdString();
                break;
            case ePluginOpenGLRenderSupportYes:
                ssinfo << tr("Yes").toStdString();
                break;
            default:
                break;
        }
        ssinfo << "</font>";
        _imp->nodeInfos.lock()->setValue( ssinfo.str() );
    } else if ( what == _imp->openglRenderingEnabledKnob.lock() ) {
        bool enabled = true;
        int thisKnobIndex = _imp->openglRenderingEnabledKnob.lock()->getValue();
        if (thisKnobIndex == 1 || (thisKnobIndex == 2 && getApp()->isBackground())) {
            enabled = false;
        }
        if (enabled) {
            // Check value on project now
            if (!getApp()->getProject()->isOpenGLRenderActivated()) {
                enabled = false;
            }
        }
        _imp->effect->onEnableOpenGLKnobValueChanged(enabled);
    } else if (what == _imp->processAllLayersKnob.lock()) {

        std::map<int, ChannelSelector>::iterator foundOutput = _imp->channelsSelectors.find(-1);
        if (foundOutput != _imp->channelsSelectors.end()) {
            _imp->onLayerChanged(true);
        }
    } else if (what == _imp->pyPlugExportButtonKnob.lock() && reason != eValueChangedReasonRestoreDefault) {
        // Trigger a knob changed action on the group
        KnobGroupPtr k = _imp->pyPlugExportDialog.lock();
        if (k) {
            k->setValue(!k->getValue());
        }
    } else if (what == _imp->pyPlugExportDialogOkButton.lock() && reason == eValueChangedReasonUserEdited) {
        try {
            exportNodeToPyPlug(_imp->pyPlugExportDialogFile.lock()->getValue());
        } catch (const std::exception& e) {
            Dialogs::errorDialog(tr("Export").toStdString(), e.what());
        }
        _imp->pyPlugExportDialog.lock()->setValue(false);
    } else if (what == _imp->pyPlugExportDialogCancelButton.lock()) {
        _imp->pyPlugExportDialog.lock()->setValue(false);
    } else {
        ret = false;
    }

    if (!ret) {
        KnobGroupPtr isGroup = toKnobGroup(what);
        if ( isGroup && isGroup->getIsDialog() && reason == eValueChangedReasonNatronInternalEdited) {
            NodeGuiIPtr gui = getNodeGui();
            if (gui) {
                gui->showGroupKnobAsDialog(isGroup);
                ret = true;
            }
        }
    }

    if (!ret) {
        for (std::map<int, ChannelSelector>::iterator it = _imp->channelsSelectors.begin(); it != _imp->channelsSelectors.end(); ++it) {
            if (it->second.layer.lock() == what) {
                _imp->onLayerChanged(it->first == -1);
                ret = true;
                break;
            }
        }
    }

    if (!ret) {
        for (int i = 0; i < 4; ++i) {
            KnobBoolPtr enabled = _imp->enabledChan[i].lock();
            if (!enabled) {
                break;
            }
            if (enabled == what) {
                checkForPremultWarningAndCheckboxes();
                ret = true;
                Q_EMIT enabledChannelCheckboxChanged();
                break;
            }
        }
    }

    if (!ret) {
        GroupInputPtr isInput = isEffectGroupInput();
        if (isInput) {
            if ( (what->getName() == kNatronGroupInputIsOptionalParamName)
                 || ( what->getName() == kNatronGroupInputIsMaskParamName) ) {
                NodeCollectionPtr col = isInput->getNode()->getGroup();
                assert(col);
                NodeGroupPtr isGrp = toNodeGroup(col);
                assert(isGrp);
                if (isGrp) {
                    ///Refresh input arrows of the node to reflect the state
                    isGrp->getNode()->initializeInputs();
                    ret = true;
                }
            }
        }
    }

    return ret;
} // Node::onEffectKnobValueChanged

KnobChoicePtr
Node::getOrCreateOpenGLEnabledKnob() const
{
    KnobChoicePtr openglRenderingKnob = _imp->openglRenderingEnabledKnob.lock();
    if (openglRenderingKnob) {
        return openglRenderingKnob;
    }
    openglRenderingKnob = AppManager::checkIfKnobExistsWithNameOrCreate<KnobChoice>(_imp->effect, "enableGPURendering", tr("GPU Rendering"));
    openglRenderingKnob->setDeclaredByPlugin(false);
    assert(openglRenderingKnob);
    openglRenderingKnob->setAnimationEnabled(false);
    {
        std::vector<std::string> entries;
        std::vector<std::string> helps;
        entries.push_back("Enabled");
        helps.push_back( tr("If a plug-in support GPU rendering, prefer rendering using the GPU if possible.").toStdString() );
        entries.push_back("Disabled");
        helps.push_back( tr("Disable GPU rendering for all plug-ins.").toStdString() );
        entries.push_back("Disabled if background");
        helps.push_back( tr("Disable GPU rendering when rendering with NatronRenderer but not in GUI mode.").toStdString() );
        openglRenderingKnob->populateChoices(entries, helps);
    }

    openglRenderingKnob->setHintToolTip( tr("Select when to activate GPU rendering for this node. Note that if the GPU Rendering parameter in the Project settings is set to disabled then GPU rendering will not be activated regardless of that value.") );

    KnobPagePtr settingsPageKnob = AppManager::checkIfKnobExistsWithNameOrCreate<KnobPage>(_imp->effect, kNodePageParamName, tr(kNodePageParamLabel));
    settingsPageKnob->addKnob(openglRenderingKnob);
    _imp->openglRenderingEnabledKnob = openglRenderingKnob;

    return openglRenderingKnob;
}

void
Node::onOpenGLEnabledKnobChangedOnProject(bool activated)
{
    bool enabled = activated;
    KnobChoicePtr k = _imp->openglRenderingEnabledKnob.lock();
    if (enabled) {
        if (k) {
            k->setEnabled(true);
            int thisKnobIndex = k->getValue();
            if (thisKnobIndex == 1 || (thisKnobIndex == 2 && getApp()->isBackground())) {
                enabled = false;
            }
        }
    } else {
        if (k) {
            k->setEnabled(true);
        }
    }
    _imp->effect->onEnableOpenGLKnobValueChanged(enabled);
    
}

KnobChoicePtr
Node::getLayerChoiceKnob(int inputNb) const
{
    std::map<int, ChannelSelector>::iterator found = _imp->channelsSelectors.find(inputNb);

    if ( found == _imp->channelsSelectors.end() ) {
        return KnobChoicePtr();
    }
    return found->second.layer.lock();
}

void
Node::refreshLayersSelectorsVisibility()
{

    KnobBoolPtr processAllKnob = _imp->processAllLayersKnob.lock();
    if (!processAllKnob) {
        return;
    }
    bool outputIsAll = processAllKnob->getValue();

    // Disable all input selectors as it doesn't make sense to edit them whilst output is All

    ImageComponents mainInputComps, outputComps;

    int mainInputIndex = getPreferredInput();

    for (std::map<int, ChannelSelector>::iterator it = _imp->channelsSelectors.begin(); it != _imp->channelsSelectors.end(); ++it) {
        if (it->first >= 0) {
            NodePtr inp = getInput(it->first);
            bool mustBeSecret = !inp.get() || outputIsAll;
            KnobChoicePtr layerKnob = it->second.layer.lock();
            layerKnob->setSecret(mustBeSecret);

            if (mainInputIndex != -1 && mainInputIndex == it->first) {
                // This is the main-input
                mainInputComps = _imp->getSelectedLayerInternal(it->first, it->second);
            }

        } else {
            it->second.layer.lock()->setSecret(outputIsAll);
            outputComps = _imp->getSelectedLayerInternal(it->first, it->second);
        }
    }


    // Refresh RGBA checkbox visibility
    KnobBoolPtr enabledChan[4];
    bool hasRGBACheckboxes = false;
    for (int i = 0; i < 4; ++i) {
        enabledChan[i] = _imp->enabledChan[i].lock();
        hasRGBACheckboxes |= (bool)enabledChan[i];
    }
    if (hasRGBACheckboxes) {
        if (outputIsAll) {
            for (int i = 0; i < 4; ++i) {
                enabledChan[i]->setSecret(true);
            }
        } else {
            refreshEnabledKnobsLabel(mainInputComps, outputComps);
        }
    }
}


void
Node::refreshEnabledKnobsLabel(const ImageComponents& mainInputComps, const ImageComponents& outputComps)
{
    KnobBoolPtr enabledChan[4];
    bool hasRGBACheckboxes = false;
    for (int i = 0; i < 4; ++i) {
        enabledChan[i] = _imp->enabledChan[i].lock();
        hasRGBACheckboxes |= (bool)enabledChan[i];
    }
    if (!hasRGBACheckboxes) {
        return;
    }

    // We display the number of channels that the output layer can have
    int nOutputComps = outputComps.getNumComponents();

    // But we name the channels by the name of the input layer
    const std::vector<std::string>& inputChannelNames = mainInputComps.getComponentsNames();
    const std::vector<std::string>& outputChannelNames = outputComps.getComponentsNames();

    switch (nOutputComps) {
    case 1: {
        for (int i = 0; i < 3; ++i) {
            enabledChan[i]->setSecret(true);
        }
        enabledChan[3]->setSecret(false);
        std::string channelName;
        if (inputChannelNames.size() == 1) {
            channelName = inputChannelNames[0];
        } else if (inputChannelNames.size() == 4) {
            channelName = inputChannelNames[3];
        } else {
            channelName = outputChannelNames[0];
        }
        enabledChan[3]->setLabel(channelName);

        break;
    }
    case 2: {
        for (int i = 2; i < 4; ++i) {
            enabledChan[i]->setSecret(true);
        }
        for (int i = 0; i < 2; ++i) {
            enabledChan[i]->setSecret(false);
            if ((int)inputChannelNames.size() > i) {
                enabledChan[i]->setLabel(inputChannelNames[i]);
            } else {
                enabledChan[i]->setLabel(outputChannelNames[i]);
            }

        }
        break;
    }
    case 3: {
        for (int i = 3; i < 4; ++i) {
            enabledChan[i]->setSecret(true);
        }
        for (int i = 0; i < 3; ++i) {
            enabledChan[i]->setSecret(false);
            if ((int)inputChannelNames.size() > i) {
                enabledChan[i]->setLabel(inputChannelNames[i]);
            } else {
                enabledChan[i]->setLabel(outputChannelNames[i]);
            }
        }
        break;
    }
    case 4: {
        for (int i = 0; i < 4; ++i) {
            enabledChan[i]->setSecret(false);
            if ((int)inputChannelNames.size() > i) {
                enabledChan[i]->setLabel(inputChannelNames[i]);
            } else {
                enabledChan[i]->setLabel(outputChannelNames[i]);
            }
        }
        break;
    }

    case 0:
    default: {
        for (int i = 0; i < 4; ++i) {
            enabledChan[i]->setSecret(true);
        }
        break;
    }
    } // switch
} // Node::refreshEnabledKnobsLabel



bool
Node::isPluginUsingHostChannelSelectors() const
{
    return _imp->hostChannelSelectorEnabled;
}

bool
Node::getProcessChannel(int channelIndex) const
{
    if (!isPluginUsingHostChannelSelectors()) {
        return true;
    }
    assert(channelIndex >= 0 && channelIndex < 4);
    KnobBoolPtr k = _imp->enabledChan[channelIndex].lock();
    if (k) {
        return k->getValue();
    }

    return true;
}

KnobBoolPtr
Node::getProcessChannelKnob(int channelIndex) const
{
    assert(channelIndex >= 0 && channelIndex < 4);
    return _imp->enabledChan[channelIndex].lock();
}

bool
Node::getSelectedLayer(int inputNb,
                       std::bitset<4> *processChannels,
                       bool* isAll,
                       ImageComponents* layer) const
{


    std::map<int, ChannelSelector>::const_iterator foundSelector = _imp->channelsSelectors.find(inputNb);
    NodePtr maskInput;
    int chanIndex = getMaskChannel(inputNb, layer, &maskInput);
    bool hasChannelSelector = true;
    if (chanIndex != -1) {
        *isAll = false;
        Q_UNUSED(chanIndex);
        if (processChannels) {
            (*processChannels)[0] = true;
            (*processChannels)[1] = true;
            (*processChannels)[2] = true;
            (*processChannels)[3] = true;
        }

        return true;
    } else {
        if ( foundSelector == _imp->channelsSelectors.end() ) {
            //Fetch in input what the user has set for the output
            foundSelector = _imp->channelsSelectors.find(-1);
        }
        if ( foundSelector == _imp->channelsSelectors.end() ) {
            hasChannelSelector = false;
        }
    }

    // Check if it is all choice
    KnobBoolPtr processAllKnob = _imp->processAllLayersKnob.lock();
    *isAll = false;
    if (processAllKnob) {
        *isAll = processAllKnob->getValue();
    }
    if (!*isAll && foundSelector != _imp->channelsSelectors.end()) {
        *layer = _imp->getSelectedLayerInternal(inputNb, foundSelector->second);
    }

    if (processChannels) {
        if (_imp->hostChannelSelectorEnabled &&  _imp->enabledChan[0].lock() ) {
            (*processChannels)[0] = _imp->enabledChan[0].lock()->getValue();
            (*processChannels)[1] = _imp->enabledChan[1].lock()->getValue();
            (*processChannels)[2] = _imp->enabledChan[2].lock()->getValue();
            (*processChannels)[3] = _imp->enabledChan[3].lock()->getValue();
        } else {
            (*processChannels)[0] = true;
            (*processChannels)[1] = true;
            (*processChannels)[2] = true;
            (*processChannels)[3] = true;
        }
    }
    return hasChannelSelector;
}

bool
Node::hasAtLeastOneChannelToProcess() const
{
    std::map<int, ChannelSelector>::const_iterator foundSelector = _imp->channelsSelectors.find(-1);

    if ( foundSelector == _imp->channelsSelectors.end() ) {
        return true;
    }
    if ( _imp->enabledChan[0].lock() ) {
        std::bitset<4> processChannels;
        processChannels[0] = _imp->enabledChan[0].lock()->getValue();
        processChannels[1] = _imp->enabledChan[1].lock()->getValue();
        processChannels[2] = _imp->enabledChan[2].lock()->getValue();
        processChannels[3] = _imp->enabledChan[3].lock()->getValue();
        if (!processChannels[0] && !processChannels[1] && !processChannels[2] && !processChannels[3]) {
            return false;
        }
    }

    return true;
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
Node::isNodeDisabled() const
{
    KnobBoolPtr b = _imp->disableNodeKnob.lock();
    bool thisDisabled = b ? b->getValue() : false;
    NodeGroupPtr isContainerGrp = toNodeGroup( getGroup() );

    if (isContainerGrp) {
        return thisDisabled || isContainerGrp->getNode()->isNodeDisabled();
    }
    NodePtr ioContainer = getIOContainer();
    if (ioContainer) {
        return ioContainer->isNodeDisabled();
    }

    int lifeTimeFirst, lifeTimeEnd;
    bool lifeTimeEnabled = isLifetimeActivated(&lifeTimeFirst, &lifeTimeEnd);
    double curFrame = _imp->effect->getCurrentTime();
    bool enabled = ( !lifeTimeEnabled || (curFrame >= lifeTimeFirst && curFrame <= lifeTimeEnd) ) && !thisDisabled;

    return !enabled;
}

void
Node::setNodeDisabled(bool disabled)
{
    KnobBoolPtr b = _imp->disableNodeKnob.lock();

    if (b) {
        b->setValue(disabled);

        // Clear the actions cache because if this function is called from another thread, the hash will not be incremented
        _imp->effect->clearActionsCache();
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
Node::hasSequentialOnlyNodeUpstream(std::string & nodeName) const
{
    ///Just take into account sequentiallity for writers
    if ( (_imp->effect->getSequentialPreference() == eSequentialPreferenceOnlySequential) && _imp->effect->isWriter() ) {
        nodeName = getScriptName_mt_safe();

        return true;
    } else {
        QMutexLocker l(&_imp->inputsMutex);

        for (InputsV::iterator it = _imp->inputs.begin(); it != _imp->inputs.end(); ++it) {
            NodePtr input = it->lock();
            if ( input && input->hasSequentialOnlyNodeUpstream(nodeName) && input->getEffectInstance()->isWriter() ) {
                nodeName = input->getScriptName_mt_safe();

                return true;
            }
        }

        return false;
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

bool
Node::canOthersConnectToThisNode() const
{
    if ( isEffectBackdrop() ) {
        return false;
    } else if ( isEffectGroupOutput() ) {
        return false;
    } else if ( _imp->effect->isWriter() && (_imp->effect->getSequentialPreference() == eSequentialPreferenceOnlySequential) ) {
        return false;
    }
    ///In debug mode only allow connections to Writer nodes
# ifdef DEBUG
    return !isEffectViewerInstance();
# else // !DEBUG
    return !isEffectViewerInstance() /* && !_imp->effect->isWriter()*/;
# endif // !DEBUG
}

void
Node::setNodeIsRenderingInternal(NodesWList& markedNodes)
{
    ///If marked, we alredy set render args
    for (NodesWList::iterator it = markedNodes.begin(); it != markedNodes.end(); ++it) {
        if (it->lock().get() == this) {
            return;
        }
    }


    ///Increment the node is rendering counter
    {
        QMutexLocker nrLocker(&_imp->nodeIsRenderingMutex);
        ++_imp->nodeIsRendering;
    }


    ///mark this
    markedNodes.push_back( shared_from_this() );

    ///Call recursively

    int maxInpu = getMaxInputCount();
    for (int i = 0; i < maxInpu; ++i) {
        NodePtr input = getInput(i);
        if (input) {
            input->setNodeIsRenderingInternal(markedNodes);
        }
    }
}

RenderingFlagSetter::RenderingFlagSetter(const NodePtr& n)
    : node(n)
    , nodes()
{
    n->setNodeIsRendering(nodes);
}

RenderingFlagSetter::~RenderingFlagSetter()
{
    for (NodesWList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        NodePtr n = it->lock();
        if (!n) {
            continue;
        }
        n->unsetNodeIsRendering();
    }
}

void
Node::setNodeIsRendering(NodesWList& nodes)
{
    setNodeIsRenderingInternal(nodes);
}

void
Node::unsetNodeIsRendering()
{

    // Decrement the node is rendering counter
    QMutexLocker k(&_imp->nodeIsRenderingMutex);
    if (_imp->nodeIsRendering > 1) {
        --_imp->nodeIsRendering;
    } else {
        _imp->nodeIsRendering = 0;
    }

}

bool
Node::isNodeRendering_nolock() const
{
    return _imp->nodeIsRendering > 0;
}

void
Node::lockNodeRenderingMutex()
{
    _imp->nodeIsRenderingMutex.lock();
}

void
Node::unlockNodeRenderingMutex()
{
    _imp->nodeIsRenderingMutex.unlock();
}

bool
Node::isNodeRendering() const
{
    QMutexLocker k(&_imp->nodeIsRenderingMutex);

    return _imp->nodeIsRendering > 0;
}


static void
addIdentityNodesRecursively(NodeConstPtr caller,
                            NodeConstPtr node,
                            double time,
                            ViewIdx view,
                            std::list<NodeConstPtr>* outputs,
                            std::list<NodeConstPtr>* markedNodes)
{
    if ( std::find(markedNodes->begin(), markedNodes->end(), node) != markedNodes->end() ) {
        return;
    }

    markedNodes->push_back(node);


    if (caller != node) {
        ParallelRenderArgsPtr inputFrameArgs = node->getEffectInstance()->getParallelRenderArgsTLS();
        const FrameViewRequest* request = 0;
        bool isIdentity = false;
        if (inputFrameArgs && inputFrameArgs->request) {
            request = inputFrameArgs->request->getFrameViewRequest(time, view);
            if (request) {
                isIdentity = request->globalData.identityInputNb != -1;
            }
        }

        if (!request) {
            /*
               Very unlikely that there's no request pass. But we still check
             */
            RenderScale scale(1.);
            double inputTimeId;
            ViewIdx identityView;
            int inputNbId;
            U64 renderHash = 0;


            RectI format = node->getEffectInstance()->getOutputFormat();

            isIdentity = node->getEffectInstance()->isIdentity_public(true, renderHash, time, scale, format, view, &inputTimeId, &identityView, &inputNbId);
        }


        if (!isIdentity) {
            outputs->push_back(node);

            return;
        }
    }

    ///Append outputs of this node instead
    NodesWList nodeOutputs;
    node->getOutputs_mt_safe(nodeOutputs);
    NodesWList outputsToAdd;
    for (NodesWList::iterator it = nodeOutputs.begin(); it != nodeOutputs.end(); ++it) {
        NodePtr output = it->lock();
        if (!output) {
            continue;
        }
        GroupOutputPtr isGroupOutput = output->isEffectGroupOutput();
        //If the node is an output node, add all the outputs of the group node instead
        if (isGroupOutput) {
            NodeCollectionPtr collection = output->getGroup();
            assert(collection);
            NodeGroupPtr isGrp = toNodeGroup(collection);
            if (isGrp) {
                NodesWList groupOutputs;
                isGrp->getNode()->getOutputs_mt_safe(groupOutputs);
                for (NodesWList::iterator it2 = groupOutputs.begin(); it2 != groupOutputs.end(); ++it2) {
                    outputsToAdd.push_back(*it2);
                }
            }
        }

        //If the node is a group, add all its inputs
        NodeGroupPtr isGrp = output->isEffectNodeGroup();
        if (isGrp) {
            NodesList inputOutputs;
            isGrp->getInputsOutputs(&inputOutputs);
            for (NodesList::iterator it2 = inputOutputs.begin(); it2 != inputOutputs.end(); ++it2) {
                outputsToAdd.push_back(*it2);
            }

        }

    }
    nodeOutputs.insert( nodeOutputs.end(), outputsToAdd.begin(), outputsToAdd.end() );
    for (NodesWList::iterator it = nodeOutputs.begin(); it != nodeOutputs.end(); ++it) {
        NodePtr node = it->lock();
        if (node) {
            addIdentityNodesRecursively(caller, node, time, view, outputs, markedNodes);
        }
    }
} // addIdentityNodesRecursively

bool
Node::shouldCacheOutput(bool isFrameVaryingOrAnimated,
                        double time,
                        ViewIdx view,
                        int /*visitsCount*/) const
{
    /*
     * Here is a list of reasons when caching is enabled for a node:
     * - It is references multiple times below in the graph
     * - Its single output has its settings panel opened,  meaning the user is actively editing the output
     * - The force caching parameter in the "Node" tab is checked
     * - The aggressive caching preference of Natron is checked
     * - We are in a recursive action (such as an analysis)
     * - The plug-in does temporal clip access
     * - Preview image is enabled (and Natron is not running in background)
     * - The node is a direct input of a viewer, this is to overcome linear graphs where all nodes would not be cached
     * - The node is not frame varying, meaning it will always produce the same image at any time
     * - The node is a roto node and it is being edited
     * - The node does not support tiles
     */

    std::list<NodeConstPtr> outputs;
    {
        std::list<NodeConstPtr> markedNodes;
        addIdentityNodesRecursively(shared_from_this(), shared_from_this(), time, view, &outputs, &markedNodes);
    }
    std::size_t sz = outputs.size();

    if (sz > 1) {
        ///The node is referenced multiple times below, cache it
        return true;
    } else {
        if (sz == 1) {
            NodeConstPtr output = outputs.front();
            ViewerNodePtr isViewer = output->isEffectViewerNode();
            if (isViewer) {

                if (isViewer->getCurrentAInput().get() == this ||
                    isViewer->getCurrentBInput().get() == this) {
                    ///The node is a direct input of the viewer. Cache it because it is likely the user will make

                    ///changes to the viewer that will need this image.
                    return true;
                }
            }

            RotoPaintPtr isRoto = toRotoPaint( output->getEffectInstance() );
            if (isRoto) {
                // THe roto internally makes multiple references to the input so cache it
                return true;
            }

            if (!isFrameVaryingOrAnimated) {
                //This image never changes, cache it once.
                return true;
            }
            if ( output->isSettingsPanelVisible() ) {
                //Output node has panel opened, meaning the user is likely to be heavily editing

                //that output node, hence requesting this node a lot. Cache it.
                return true;
            }
            if ( _imp->effect->doesTemporalClipAccess() ) {
                //Very heavy to compute since many frames are fetched upstream. Cache it.
                return true;
            }
            if ( !_imp->effect->supportsTiles() ) {
                //No tiles, image is going to be produced fully, cache it to prevent multiple access

                //with different RoIs
                return true;
            }
            if (_imp->effect->getRecursionLevel() > 0) {
                //We are in a call from getImage() and the image needs to be computed, so likely in an

                //analysis pass. Cache it because the image is likely to get asked for severla times.
                return true;
            }
            if ( isForceCachingEnabled() ) {
                //Users wants it cached
                return true;
            }
            NodeGroupPtr parentIsGroup = toNodeGroup( getGroup() );
            if ( parentIsGroup && parentIsGroup->getNode()->isForceCachingEnabled() && (parentIsGroup->getOutputNodeInput().get() == this) ) {
                //if the parent node is a group and it has its force caching enabled, cache the output of the Group Output's node input.
                return true;
            }

            if ( appPTR->isAggressiveCachingEnabled() ) {
                ///Users wants all nodes cached
                return true;
            }

            if ( isPreviewEnabled() && !appPTR->isBackground() ) {
                ///The node has a preview, meaning the image will be computed several times between previews & actual renders. Cache it.
                return true;
            }

            if ( isRotoPaintingNode() && isSettingsPanelVisible() ) {
                ///The Roto node is being edited, cache its output (special case because Roto has an internal node tree)
                return true;
            }

            RotoDrawableItemPtr attachedStroke = _imp->paintStroke.lock();
            if ( attachedStroke && attachedStroke->getModel()->getNode()->isSettingsPanelVisible() ) {
                ///Internal RotoPaint tree and the Roto node has its settings panel opened, cache it.
                return true;
            }
        } else {
            // outputs == 0, never cache, unless explicitly set or rotopaint internal node
            RotoDrawableItemPtr attachedStroke = _imp->paintStroke.lock();

            return isForceCachingEnabled() || appPTR->isAggressiveCachingEnabled() ||
                   ( attachedStroke && attachedStroke->getModel()->getNode()->isSettingsPanelVisible() );
        }
    }

    return false;
} // Node::shouldCacheOutput


bool
Node::refreshMaskEnabledNess(int inputNb)
{
    std::map<int, MaskSelector>::iterator found = _imp->maskSelectors.find(inputNb);
    NodePtr inp = getInput(inputNb);
    bool changed = false;

    if ( found != _imp->maskSelectors.end() ) {
        KnobBoolPtr enabled = found->second.enabled.lock();
        assert(enabled);
        enabled->blockValueChanges();
        bool curValue = enabled->getValue();
        bool newValue = inp ? true : false;
        changed = curValue != newValue;
        if (changed) {
            enabled->setValue(newValue);
        }
        enabled->unblockValueChanges();
    }

    return changed;
}

bool
Node::refreshDraftFlagInternal(const std::vector<NodeWPtr >& inputs)
{
    bool hasDraftInput = false;

    for (std::size_t i = 0; i < inputs.size(); ++i) {
        NodePtr input = inputs[i].lock();
        if (input) {
            hasDraftInput |= input->isDraftModeUsed();
        }
    }
    hasDraftInput |= _imp->effect->supportsRenderQuality();
    bool changed;
    {
        QMutexLocker k(&_imp->pluginsPropMutex);
        changed = _imp->draftModeUsed != hasDraftInput;
        _imp->draftModeUsed = hasDraftInput;
    }

    return changed;
}

void
Node::refreshAllInputRelatedData(bool canChangeValues)
{
    refreshAllInputRelatedData( canChangeValues, getInputs_copy() );
}

bool
Node::refreshAllInputRelatedData(bool /*canChangeValues*/,
                                 const std::vector<NodeWPtr >& inputs)
{
    assert( QThread::currentThread() == qApp->thread() );
    RefreshingInputData_RAII _refreshingflag( _imp.get() );
    bool hasChanged = false;
    hasChanged |= refreshDraftFlagInternal(inputs);

    bool loadingProject = getApp()->getProject()->isLoadingProject();
    ///if all non optional clips are connected, call getClipPrefs
    ///The clip preferences action is never called until all non optional clips have been attached to the plugin.
    /// EDIT: we allow calling getClipPreferences even if some non optional clip is not connected so that layer menus get properly created
    const bool canCallRefreshMetaData = true; // = !hasMandatoryInputDisconnected();

    if (canCallRefreshMetaData) {
        if (loadingProject) {
            //Nb: we clear the action cache because when creating the node many calls to getRoD and stuff might have returned
            //empty rectangles, but since we force the hash to remain what was in the project file, we might then get wrong RoDs returned
            _imp->effect->clearActionsCache();
        }

        double time = (double)getApp()->getTimeLine()->currentFrame();
        RenderScale scaleOne(1.);
        ///Render scale support might not have been set already because getRegionOfDefinition could have failed until all non optional inputs were connected
        if (_imp->effect->supportsRenderScaleMaybe() == EffectInstance::eSupportsMaybe) {
            RectD rod;
            StatusEnum stat = _imp->effect->getRegionOfDefinition(time, scaleOne, ViewIdx(0), &rod);
            if (stat != eStatusFailed) {
                RenderScale scale(0.5);
                stat = _imp->effect->getRegionOfDefinition(time, scale, ViewIdx(0), &rod);
                if (stat != eStatusFailed) {
                    _imp->effect->setSupportsRenderScaleMaybe(EffectInstance::eSupportsYes);
                } else {
                    _imp->effect->setSupportsRenderScaleMaybe(EffectInstance::eSupportsNo);
                }
            }
        }
        hasChanged |= _imp->effect->refreshMetaDatas_public(false);
    }

    hasChanged |= refreshChannelSelectors();

    refreshIdentityState();

    {
        QMutexLocker k(&_imp->pluginsPropMutex);
        _imp->mustComputeInputRelatedData = false;
    }

    return hasChanged;
} // Node::refreshAllInputRelatedData

bool
Node::refreshInputRelatedDataInternal(bool domarking, std::set<NodePtr>& markedNodes)
{
    {
        QMutexLocker k(&_imp->pluginsPropMutex);
        if (!_imp->mustComputeInputRelatedData) {
            //We didn't change
            return false;
        }
    }

    if (domarking) {
        std::set<NodePtr>::iterator found = markedNodes.find(shared_from_this());

        if ( found != markedNodes.end() ) {
            return false;
        }
    }

    ///Check if inputs must be refreshed first

    int maxInputs = getMaxInputCount();
    std::vector<NodeWPtr > inputsCopy(maxInputs);
    for (int i = 0; i < maxInputs; ++i) {
        NodePtr input = getInput(i);
        inputsCopy[i] = input;
        if ( input && input->isInputRelatedDataDirty() ) {
            input->refreshInputRelatedDataInternal(true, markedNodes);
        }
    }

    if (domarking) {
        markedNodes.insert(shared_from_this());
    }

    bool hasChanged = refreshAllInputRelatedData(false, inputsCopy);

    return hasChanged;
}

bool
Node::isInputRelatedDataDirty() const
{
    QMutexLocker k(&_imp->pluginsPropMutex);

    return _imp->mustComputeInputRelatedData;
}

void
Node::forceRefreshAllInputRelatedData()
{
    markInputRelatedDataDirtyRecursive();

    NodeGroupPtr isGroup = isEffectNodeGroup();
    if (isGroup) {
        NodesList inputs;
        isGroup->getInputsOutputs(&inputs);
        for (NodesList::iterator it = inputs.begin(); it != inputs.end(); ++it) {
            if ( (*it) ) {
                (*it)->refreshInputRelatedDataRecursive();
            }
        }
    } else {
        refreshInputRelatedDataRecursive();
    }
}

void
Node::markAllInputRelatedDataDirty()
{
    {
        QMutexLocker k(&_imp->pluginsPropMutex);
        _imp->mustComputeInputRelatedData = true;
    }
}

void
Node::markInputRelatedDataDirtyRecursiveInternal(std::list<NodePtr>& markedNodes,
                                                 bool recurse)
{
    std::list<NodePtr>::iterator found = std::find(markedNodes.begin(), markedNodes.end(), shared_from_this());

    if ( found != markedNodes.end() ) {
        return;
    }
    markAllInputRelatedDataDirty();
    markedNodes.push_back(shared_from_this());
    if (recurse) {
        NodesList outputs;
        getOutputsWithGroupRedirection(outputs);
        for (NodesList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
            (*it)->markInputRelatedDataDirtyRecursiveInternal( markedNodes, true );
        }
    }
}

void
Node::markInputRelatedDataDirtyRecursive()
{
    std::list<NodePtr> marked;

    markInputRelatedDataDirtyRecursiveInternal(marked, true);
}

void
Node::refreshInputRelatedDataRecursiveInternal(std::set<NodePtr>& markedNodes)
{
    if ( getApp()->isCreatingNodeTree() ) {
        return;
    }
    std::set<NodePtr>::iterator found = markedNodes.find(shared_from_this());

    if ( found != markedNodes.end() ) {
        return;
    }

    markedNodes.insert(shared_from_this());
    refreshInputRelatedDataInternal(false, markedNodes);

    ///Now notify outputs we have changed
    NodesList outputs;
    getOutputsWithGroupRedirection(outputs);
    for (NodesList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        (*it)->refreshInputRelatedDataRecursiveInternal( markedNodes );
    }
}

void
Node::refreshInputRelatedDataRecursive()
{

    std::set<NodePtr> markedNodes;

    refreshInputRelatedDataRecursiveInternal(markedNodes);
}

bool
Node::isDraftModeUsed() const
{
    QMutexLocker k(&_imp->pluginsPropMutex);

    return _imp->draftModeUsed;
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

bool
Node::getOverlayColor(double* r,
                      double *g,
                      double* b) const
{
    NodeGuiIPtr node_ui = getNodeGui();
    if (node_ui && node_ui->isOverlayLocked()) {
        *r = 0.5;
        *g = 0.5;
        *b = 0.5;
        return true;
    }

    double tmpColor[3];
    {
        QMutexLocker k(&_imp->nodeUIDataMutex);
        tmpColor[0] = _imp->overlayColor[0];
        tmpColor[1] = _imp->overlayColor[1];
        tmpColor[2] = _imp->overlayColor[2];
    }
    if (tmpColor[0] == -1 && tmpColor[1] == -1 && tmpColor[2] == -1) {
        // No overlay color
        return false;
    }
    *r = tmpColor[0];
    *g = tmpColor[1];
    *b = tmpColor[2];

    return true;
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
Node::onNodeUIOverlayColorChanged(double r,
                           double g,
                           double b)
{
    QMutexLocker k(&_imp->nodeUIDataMutex);
    _imp->overlayColor[0] = r;
    _imp->overlayColor[1] = g;
    _imp->overlayColor[2] = b;

}

void
Node::setOverlayColor(double r, double g, double b)
{
    NodeGuiIPtr gui = _imp->guiPointer.lock();

    if (gui) {
        gui->setOverlayColor(r, g, b);
    } else {
        onNodeUIOverlayColorChanged(r,g,b);
    }
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

    if ( recursionList.find(shared_from_this()) != recursionList.end() ) {
        return false;
    }
    recursionList.insert(shared_from_this());

    {
        NodePtr master = getMasterNode();
        if (master) {
            return master->isSettingsPanelVisible();
        }
        for (KnobLinkList::iterator it = _imp->nodeLinks.begin(); it != _imp->nodeLinks.end(); ++it) {
            NodePtr masterNode = it->masterNode.lock();
            if ( masterNode && (masterNode.get() != this) && masterNode->isSettingsPanelVisibleInternal(recursionList) ) {
                return true;
            }
        }
    }

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
Node::getAttachedRotoItem() const
{
    return _imp->paintStroke.lock();
}

void
Node::declareNodeVariableToPython(const std::string& nodeName)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    if (getScriptName_mt_safe().empty()) {
        return;
    }


    PythonGILLocker pgl;
    PyObject* mainModule = appPTR->getMainModule();
    assert(mainModule);

    std::string appID = getApp()->getAppIDString();
    std::string nodeFullName = appID + "." + nodeName;
    bool alreadyDefined = false;
    PyObject* nodeObj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(nodeFullName, mainModule, &alreadyDefined);
    assert(nodeObj);
    Q_UNUSED(nodeObj);

    if (!alreadyDefined) {
        std::stringstream ss;
        ss << nodeFullName << " = " << appID << ".getNode(\"" << nodeName << "\")\n";
#ifdef DEBUG
        ss << "if not " << nodeFullName << ":\n";
        ss << "    print \"[BUG]: " << nodeFullName << " does not exist!\"";
#endif
        std::string script = ss.str();
        std::string output;
        std::string err;
        if ( !appPTR->isBackground() ) {
            getApp()->printAutoDeclaredVariable(script);
        }
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, &output) ) {
            qDebug() << err.c_str();
        }
    }
}

void
Node::setNodeVariableToPython(const std::string& oldName,
                              const std::string& newName)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    if (getScriptName_mt_safe().empty()) {
        return;
    }
    QString appID = QString::fromUtf8( getApp()->getAppIDString().c_str() );
    QString str = QString( appID + QString::fromUtf8(".%1 = ") + appID + QString::fromUtf8(".%2\ndel ") + appID + QString::fromUtf8(".%2\n") ).arg( QString::fromUtf8( newName.c_str() ) ).arg( QString::fromUtf8( oldName.c_str() ) );
    std::string script = str.toStdString();
    std::string err;
    if ( !appPTR->isBackground() ) {
        getApp()->printAutoDeclaredVariable(script);
    }
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0) ) {
        qDebug() << err.c_str();
    }
}

void
Node::deleteNodeVariableToPython(const std::string& nodeName)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
   if (getScriptName_mt_safe().empty()) {
        return;
    }

    AppInstancePtr app = getApp();
    if (!app) {
        return;
    }
    QString appID = QString::fromUtf8( getApp()->getAppIDString().c_str() );
    std::string nodeFullName = appID.toStdString() + "." + nodeName;
    bool alreadyDefined = false;
    PyObject* nodeObj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(nodeFullName, appPTR->getMainModule(), &alreadyDefined);
    assert(nodeObj);
    Q_UNUSED(nodeObj);
    if (alreadyDefined) {
        std::string script = "del " + nodeFullName;
        std::string err;
        if ( !appPTR->isBackground() ) {
            getApp()->printAutoDeclaredVariable(script);
        }
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0) ) {
            qDebug() << err.c_str();
        }
    }
}

void
Node::declarePythonKnobs()
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    if (getScriptName_mt_safe().empty()) {
        return;
    }
    PythonGILLocker pgl;

    if ( !getGroup() ) {
        return;
    }

    std::locale locale;
    std::string nodeName;
    if (getIOContainer()) {
        nodeName = getIOContainer()->getFullyQualifiedName();
    } else {
        nodeName = getFullyQualifiedName();
    }
    std::string appID = getApp()->getAppIDString();
    bool alreadyDefined = false;
    std::string nodeFullName = appID + "." + nodeName;
    PyObject* nodeObj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(nodeFullName, NATRON_PYTHON_NAMESPACE::getMainModule(), &alreadyDefined);
    assert(nodeObj);
    Q_UNUSED(nodeObj);
    if (!alreadyDefined) {
        qDebug() << QString::fromUtf8("declarePythonKnobs(): attribute ") + QString::fromUtf8( nodeFullName.c_str() ) + QString::fromUtf8(" is not defined");
        throw std::logic_error(std::string("declarePythonKnobs(): attribute ") + nodeFullName + " is not defined");
    }


    std::stringstream ss;
#ifdef DEBUG
    ss << "if not " << nodeFullName << ":\n";
    ss << "    print \"[BUG]: " << nodeFullName << " is not defined!\"\n";
#endif
    const KnobsVec& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        const std::string& knobName = knobs[i]->getName();
        if ( !knobName.empty() && (knobName.find(" ") == std::string::npos) && !std::isdigit(knobName[0], locale) ) {
            if ( PyObject_HasAttrString( nodeObj, knobName.c_str() ) ) {
                continue;
            }
            ss << nodeFullName <<  "." << knobName << " = ";
            ss << nodeFullName << ".getParam(\"" << knobName << "\")\n";
        }
    }

    std::string script = ss.str();
    if ( !script.empty() ) {
        if ( !appPTR->isBackground() ) {
            getApp()->printAutoDeclaredVariable(script);
        }
        std::string err;
        std::string output;
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, &output) ) {
            qDebug() << err.c_str();
        }
    }
} // Node::declarePythonFields

void
Node::removeParameterFromPython(const std::string& parameterName)
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    if (getScriptName_mt_safe().empty()) {
        return;
    }
    PythonGILLocker pgl;
    std::string appID = getApp()->getAppIDString();
    std::string nodeName;
    if (getIOContainer()) {
        nodeName = getIOContainer()->getFullyQualifiedName();
    } else {
        nodeName = getFullyQualifiedName();
    }
    std::string nodeFullName = appID + "." + nodeName;
    bool alreadyDefined = false;
    PyObject* nodeObj = NATRON_PYTHON_NAMESPACE::getAttrRecursive(nodeFullName, NATRON_PYTHON_NAMESPACE::getMainModule(), &alreadyDefined);
    assert(nodeObj);
    Q_UNUSED(nodeObj);
    if (!alreadyDefined) {
        qDebug() << QString::fromUtf8("removeParameterFromPython(): attribute ") + QString::fromUtf8( nodeFullName.c_str() ) + QString::fromUtf8(" is not defined");
        throw std::logic_error(std::string("removeParameterFromPython(): attribute ") + nodeFullName + " is not defined");
    }
    assert( PyObject_HasAttrString( nodeObj, parameterName.c_str() ) );
    std::string script = "del " + nodeFullName + "." + parameterName;
    if ( !appPTR->isBackground() ) {
        getApp()->printAutoDeclaredVariable(script);
    }
    std::string err;
    if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, 0) ) {
        qDebug() << err.c_str();
    }
}

void
Node::declareAllPythonAttributes()
{
#ifdef NATRON_RUN_WITHOUT_PYTHON

    return;
#endif
    try {
        declareNodeVariableToPython( getFullyQualifiedName() );
        declarePythonKnobs();
        declareTablePythonFields();
    } catch (const std::exception& e) {
        qDebug() << e.what();
    }
}

void
Node::runChangedParamCallback(const KnobIPtr& k, bool userEdited)
{
    std::string cb = getKnobChangedCallback();
    if (!cb.empty()) {
        _imp->runChangedParamCallback(cb, k, userEdited);
    }
}

std::string
Node::getKnobChangedCallback() const
{
    KnobStringPtr s = _imp->knobChangedCallback.lock();

    return s ? s->getValue() : std::string();
}

std::string
Node::getInputChangedCallback() const
{
    KnobStringPtr s = _imp->inputChangedCallback.lock();

    return s ? s->getValue() : std::string();
}


std::string
Node::getBeforeRenderCallback() const
{
    KnobStringPtr s = _imp->beforeRender.lock();

    return s ? s->getValue() : std::string();
}

std::string
Node::getBeforeFrameRenderCallback() const
{
    KnobStringPtr s = _imp->beforeFrameRender.lock();

    return s ? s->getValue() : std::string();
}

std::string
Node::getAfterRenderCallback() const
{
    KnobStringPtr s = _imp->afterRender.lock();

    return s ? s->getValue() : std::string();
}

std::string
Node::getAfterFrameRenderCallback() const
{
    KnobStringPtr s = _imp->afterFrameRender.lock();

    return s ? s->getValue() : std::string();
}

std::string
Node::getAfterNodeCreatedCallback() const
{
    KnobStringPtr s = _imp->nodeCreatedCallback.lock();

    return s ? s->getValue() : std::string();
}

std::string
Node::getBeforeNodeRemovalCallback() const
{
    KnobStringPtr s = _imp->nodeRemovalCallback.lock();

    return s ? s->getValue() : std::string();
}

void
Node::runAfterTableItemsSelectionChangedCallback(const std::list<KnobTableItemPtr>& deselected, const std::list<KnobTableItemPtr>& selected, TableChangeReasonEnum reason)
{
    KnobStringPtr s = _imp->tableSelectionChangedCallback.lock();
    if (!s) {
        return;
    }
    _imp->runAfterItemsSelectionChangedCallback(s->getValue(), deselected, selected, reason);
}

void
Node::runInputChangedCallback(int index)
{
    std::string cb = getInputChangedCallback();

    if ( !cb.empty() ) {
        _imp->runInputChangedCallback(index, cb);
    }
}

KnobChoicePtr
Node::getChannelSelectorKnob(int inputNb) const
{
    std::map<int, ChannelSelector>::const_iterator found = _imp->channelsSelectors.find(inputNb);

    if ( found == _imp->channelsSelectors.end() ) {
        if (inputNb == -1) {
            ///The effect might be multi-planar and supply its own
            KnobIPtr knob = getKnobByName(kNatronOfxParamOutputChannels);
            if (!knob) {
                return KnobChoicePtr();
            }

            return toKnobChoice(knob);
        }

        return KnobChoicePtr();
    }

    return found->second.layer.lock();
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

    //Do not display a warning for Roto paint
    if ( !prefInput || _imp->effect->isRotoPaintNode() ) {
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

static void
parseLayerString(const std::string& encoded,
                 bool* isColor)
{
    if ( (encoded == kNatronRGBAPlaneUserName) ||
         ( encoded == kNatronRGBPlaneUserName) ||
         ( encoded == kNatronAlphaPlaneUserName) ) {
        *isColor = true;
    } else {
        *isColor = false;
    }
}

static bool
parseMaskChannelString(const std::string& encodedChannel,
                       std::string* nodeName,
                       std::string* layerName,
                       std::string* channelName,
                       bool *isColor)
{
    std::size_t foundLastDot = encodedChannel.find_last_of('.');

    if (foundLastDot == std::string::npos) {
        *isColor = false;
        if (encodedChannel == "None") {
            *layerName = "None";

            return true;
        }

        return false;
    }
    *channelName = encodedChannel.substr(foundLastDot + 1);

    std::string baseName = encodedChannel.substr(0, foundLastDot);
    std::size_t foundPrevDot = baseName.find_first_of('.');
    if (foundPrevDot != std::string::npos) {
        //Remove the node name
        *layerName = baseName.substr(foundPrevDot + 1);
        *nodeName = baseName.substr(0, foundPrevDot);
    } else {
        *layerName = baseName;
        nodeName->clear();
    }
    *isColor = *layerName == kNatronRGBAComponentsName || *layerName == kNatronRGBComponentsName || *layerName == kNatronAlphaComponentsName;

    return true;
}

class MergeMaskChannelData
    : public KnobChoiceMergeEntriesData
{
public:

    std::string bNode, bLayer, bChannel;
    bool isColor;
    bool dataSet;

    MergeMaskChannelData()
        : KnobChoiceMergeEntriesData()
        , isColor(false)
        , dataSet(false)
    {
    }

    virtual void clear()
    {
        dataSet = false;
        bNode.clear();
        bLayer.clear();
        bChannel.clear();
    }

    virtual ~MergeMaskChannelData()
    {
    }
};

static bool
maskChannelEqualityFunctorInternal(const std::string& aLayer,
                                   const std::string& aChannel,
                                   const std::string& bLayer,
                                   const std::string& bChannel,
                                   bool aIsColor,
                                   bool bIsColor)
{
    if ( aChannel.empty() && bChannel.empty() ) {
        // None choice
        return aLayer == bLayer;
    } else if (aChannel != bChannel) {
        return false;
    } else {
        // Same channel, check layer
        if (aLayer == bLayer) {
            return true;
        } else if (aIsColor && bIsColor) {
            return true;
        }
    }

    return false;
}

static bool
maskChannelEqualityFunctor(const std::string& a,
                           const std::string& b,
                           KnobChoiceMergeEntriesData* data)
{
    MergeMaskChannelData* mergeData = dynamic_cast<MergeMaskChannelData*>(data);

    assert(mergeData);
    if (!mergeData) {
        return false;
    }
    std::string aNode, aLayer, aChannel;
    bool aIsColor;
    parseMaskChannelString(a, &aNode, &aLayer, &aChannel, &aIsColor);
    if (!mergeData->dataSet) {
        parseMaskChannelString(b, &mergeData->bNode, &mergeData->bLayer, &mergeData->bChannel, &mergeData->isColor);
        mergeData->dataSet = true;
    }

    return maskChannelEqualityFunctorInternal(aLayer, aChannel, mergeData->bLayer, mergeData->bChannel, aIsColor, mergeData->isColor);
}

class MergeLayerData
    : public KnobChoiceMergeEntriesData
{
public:

    bool isColor;
    bool dataSet;

    MergeLayerData()
        : KnobChoiceMergeEntriesData()
        , isColor(false)
        , dataSet(false)
    {
    }

    virtual void clear()
    {
        dataSet = false;
    }

    virtual ~MergeLayerData()
    {
    }
};

static bool
layerEqualityFunctor(const std::string& a,
                     const std::string& b,
                     KnobChoiceMergeEntriesData* data)
{
    MergeLayerData* mergeData = dynamic_cast<MergeLayerData*>(data);

    assert(mergeData);
    if (!mergeData) {
        return false;
    }
    bool aIsColor;
    parseLayerString(a, &aIsColor);
    if (!mergeData->dataSet) {
        parseLayerString(b, &mergeData->isColor);
        mergeData->dataSet = true;
    }
    if (aIsColor && mergeData->isColor) {
        return true;
    } else if (a == b) {
        return true;
    }

    return false;
}

static bool isRedComponent(const std::string& a) {
    std::string str = a;
    boost::to_lower(str);
    return str == "red" || str == "r";
}

static bool isGreenComponent(const std::string& a) {
    std::string str = a;
    boost::to_lower(str);
    return str == "green" || str == "g";
}

static bool isBlueComponent(const std::string& a) {
    std::string str = a;
    boost::to_lower(str);
    return str == "blue" || str == "b";
}

static bool isAlphaComponent(const std::string& a) {
    std::string str = a;
    boost::to_lower(str);
    return str == "alpha" || str == "a";
}

static bool channelNamesEqualFunctor(const std::string& a, const std::string& b)
{
    if (isRedComponent(a) && isRedComponent(b)) {
        return true;
    } else if (isGreenComponent(a) && isGreenComponent(b)) {
        return true;
    } else if (isBlueComponent(a) && isBlueComponent(b)) {
        return true;
    } else if (isAlphaComponent(a) && isAlphaComponent(b)) {
        return true;
    }
    return false;
}

int
Node::getMaskChannel(int inputNb,
                     ImageComponents* comps,
                     NodePtr* maskInput) const
{
    *comps = ImageComponents::getNoneComponents();
    maskInput->reset();
    std::map<int, MaskSelector >::const_iterator it = _imp->maskSelectors.find(inputNb);

    if ( it != _imp->maskSelectors.end() ) {
        std::string maskEncoded =  it->second.channel.lock()->getActiveEntryText();
        std::string nodeName, layerName, channelName;
        bool isColor;
        bool ok = parseMaskChannelString(maskEncoded, &nodeName, &layerName, &channelName, &isColor);

        if ( !ok || (layerName == "None") ) {
            return -1;
        } else {
            QMutexLocker locker(&it->second.compsMutex);
            for (std::size_t i = 0; i < it->second.compsAvailable.size(); ++i) {
                if ( (it->second.compsAvailable[i].first.getLayerName() == layerName) ||
                     ( isColor && it->second.compsAvailable[i].first.isColorPlane() ) ) {
                    const std::vector<std::string>& channels = it->second.compsAvailable[i].first.getComponentsNames();
                    for (std::size_t j = 0; j < channels.size(); ++j) {
                        if (channelNamesEqualFunctor(channels[j], channelName)) {
                            *comps = it->second.compsAvailable[i].first;
                            *maskInput = it->second.compsAvailable[i].second.lock();

                            return j;
                        }
                    }
                }
            }
        }
        //Default to alpha
        // *comps = ImageComponents::getAlphaComponents();
        //return 0;
    }

    return -1;
}

bool
Node::refreshChannelSelectors()
{
    if ( !isNodeCreated() ) {
        return false;
    }
    _imp->effect->setComponentsAvailableDirty(true);

    double time = getApp()->getTimeLine()->currentFrame();
    bool hasChanged = false;
    for (std::map<int, ChannelSelector>::iterator it = _imp->channelsSelectors.begin(); it != _imp->channelsSelectors.end(); ++it) {
        NodePtr node;
        if (it->first == -1) {
            node = shared_from_this();
        } else {
            node = getInput(it->first);
        }

        KnobChoicePtr layerKnob = it->second.layer.lock();

        // The Output Layer menu has a All choice, input layers menus have a None choice.
        std::vector<std::string> choices;
        if (it->first >= 0) {
            choices.push_back("None");
        }
        int gotColor = -1;
        ImageComponents colorComp;

        //
        // These are default layers that we always display in the layer selector.
        // If one of them is found in the clip preferences, we set the default value to it.
        //
        std::map<std::string, int > defaultLayers;
        {
            std::vector<std::string> projectLayers = getApp()->getProject()->getProjectDefaultLayerNames();
            for (std::size_t i = 0; i < projectLayers.size(); ++i) {
                defaultLayers[projectLayers[i]] = -1;
            }
        }


        // We may not have an input
        if (node) {
            // Get the components available on the node
            EffectInstance::ComponentsAvailableMap compsAvailable;
            node->getEffectInstance()->getComponentsAvailable(it->first != -1, true, time, &compsAvailable);
            {
                QMutexLocker k(&it->second.compsMutex);
                it->second.compsAvailable = compsAvailable;
            }
            for (EffectInstance::ComponentsAvailableMap::iterator it2 = compsAvailable.begin(); it2 != compsAvailable.end(); ++it2) {
                // Insert the color plane second in the menu
                if ( it2->first.isColorPlane() ) {
                    int numComp = it2->first.getNumComponents();
                    colorComp = it2->first;

                    assert(choices.size() > 0);
                    std::vector<std::string>::iterator pos = choices.begin();
                    ++pos; // bypass the "None" choice
                    gotColor = 1;


                    std::string colorCompName;
                    if (numComp == 1) {
                        colorCompName = kNatronAlphaPlaneUserName;
                    } else if (numComp == 3) {
                        colorCompName = kNatronRGBPlaneUserName;
                    } else if (numComp == 4) {
                        colorCompName = kNatronRGBAPlaneUserName;
                    } else {
                        assert(false);
                    }
                    choices.insert(pos, colorCompName);


                    // Increment all default indexes since we inserted color in the list
                    for (std::map<std::string, int >::iterator it = defaultLayers.begin(); it != defaultLayers.end(); ++it) {
                        if (it->second != -1) {
                            ++it->second;
                        }
                    }
                } else {
                    std::string choiceName = ImageComponents::mapNatronInternalPlaneNameToUserFriendlyPlaneName( it2->first.getLayerName() );
                    std::map<std::string, int>::iterator foundDefaultLayer = defaultLayers.find( it2->first.getLayerName() );
                    if ( foundDefaultLayer != defaultLayers.end() ) {
                        foundDefaultLayer->second = choices.size() - 1;
                    }


                    choices.push_back(choiceName);
                }
            }
        } // if (node) {

        // If we did not find the color plane, insert it since it is the unique mandatory plane.
        if (gotColor == -1) {
            assert(choices.size() > 0);
            std::vector<std::string>::iterator pos = choices.begin();
            ++pos;
            gotColor = 1;
            Q_UNUSED(gotColor);
            ///Increment all default indexes
            for (std::map<std::string, int>::iterator it = defaultLayers.begin(); it != defaultLayers.end(); ++it) {
                if (it->second != -1) {
                    ++it->second;
                }
            }
            colorComp = ImageComponents::getRGBAComponents();
            choices.insert(pos, kNatronRGBAPlaneUserName);
        }

        // Insert default layers
        for (std::map<std::string, int>::iterator itl = defaultLayers.begin(); itl != defaultLayers.end(); ++itl) {
            if (itl->second == -1) {
                std::string choiceName = ImageComponents::mapNatronInternalPlaneNameToUserFriendlyPlaneName(itl->first);
                choices.push_back(choiceName);
            }
        }

        const std::vector<std::string> currentLayerEntries = layerKnob->getEntries();
        const std::string curLayer_FriendlyName = layerKnob->getActiveEntryText();
        const std::string curLayer = ImageComponents::mapUserFriendlyPlaneNameToNatronInternalPlaneName(curLayer_FriendlyName);


        {
            MergeLayerData tmpData;
            bool menuChanged = layerKnob->populateChoices(choices, std::vector<std::string>(), layerEqualityFunctor, &tmpData);
            if (menuChanged) {
                hasChanged = true;
                s_outputLayerChanged();
            }
        }
    } // for (std::map<int,ChannelSelector>::iterator it = _imp->channelsSelectors.begin(); it != _imp->channelsSelectors.end(); ++it) {

    NodePtr prefInputNode;
    if ( !_imp->maskSelectors.empty() ) {
        int prefInputNb = getPreferredInput();
        if (prefInputNb != -1) {
            prefInputNode = getInput(prefInputNb);
        }
    }

    for (std::map<int, MaskSelector>::iterator it = _imp->maskSelectors.begin(); it != _imp->maskSelectors.end(); ++it) {
        NodePtr node;
        if (it->first == -1) {
            node = shared_from_this();
        } else {
            node = getInput(it->first);
        }


        std::vector<std::string> choices;
        choices.push_back("None");

        //Get the mask input components
        EffectInstance::ComponentsAvailableMap compsAvailable;
        std::list<EffectInstancePtr> markedNodes;
        if (node) {
            node->getEffectInstance()->getComponentsAvailable(true, true, time, &compsAvailable, &markedNodes);
        }

        //Also get the node's preferred input (the "main" input) components
        EffectInstance::ComponentsAvailableMap prefInputAvailComps;

        if (prefInputNode) {
            prefInputNode->getEffectInstance()->getComponentsAvailable(true, true, time, &prefInputAvailComps, &markedNodes);

            //Merge the 2 components available maps, but preferring channels coming from the Mask input
            for (EffectInstance::ComponentsAvailableMap::iterator it = prefInputAvailComps.begin(); it != prefInputAvailComps.end(); ++it) {
                //If the component is already present in the 'comps' map, only add it if we are the preferred input
                EffectInstance::ComponentsAvailableMap::iterator colorMatch = compsAvailable.end();
                bool found = false;
                for (EffectInstance::ComponentsAvailableMap::iterator it2 = compsAvailable.begin(); it2 != compsAvailable.end(); ++it2) {
                    if (it2->first == it->first) {
                        found = true;
                        break;
                    } else if ( it2->first.isColorPlane() ) {
                        colorMatch = it2;
                    }
                }
                if (!found) {
                    if ( ( colorMatch != compsAvailable.end() ) && it->first.isColorPlane() ) {
                        //we found another color components type, skip
                        continue;
                    } else {
                        compsAvailable.insert(*it);
                    }
                }
            }
        } // if (prefInputNode)


        std::vector<std::pair<ImageComponents, NodeWPtr > > compsOrdered;
        ImageComponents gotColor;
        NodePtr nodeGotColor;
        for (EffectInstance::ComponentsAvailableMap::iterator comp = compsAvailable.begin(); comp != compsAvailable.end(); ++comp) {
            if ( comp->first.isColorPlane() ) {
                //compsOrdered.insert(compsOrdered.begin(), std::make_pair(comp->first,comp->second));
                gotColor = comp->first;
                nodeGotColor = comp->second.lock();
            } else {
                compsOrdered.push_back(*comp);
            }
        }


        {
            QMutexLocker k(&it->second.compsMutex);
            it->second.compsAvailable = compsOrdered;

            // Add the components available for the color plane, but only retain RGBA in the channel selector.
            // We do this because by default the channel selector has RGBA but when input is RGB it will complain that Alpha is not available.
            if (gotColor) {
                it->second.compsAvailable.push_back( std::make_pair(gotColor, nodeGotColor) );
            }
        }

        for (std::vector<std::pair<ImageComponents, NodeWPtr > >::iterator it2 = compsOrdered.begin(); it2 != compsOrdered.end(); ++it2) {
            const std::vector<std::string>& channels = it2->first.getComponentsNames();
            bool isColor = it2->first.isColorPlane();
            const std::string& layerName = isColor ? it2->first.getComponentsGlobalName() : it2->first.getLayerName();
            NodePtr providerNode = it2->second.lock();
            std::string nodeName;
            if (providerNode) {
                nodeName = providerNode->getScriptName_mt_safe();
            }

            for (std::size_t i = 0; i < channels.size(); ++i) {
                std::string option;
                if (!isColor) {
                    option += nodeName;
                    if ( !nodeName.empty() ) {
                        option += '.';
                    }
                }
                option += layerName;
                if ( !layerName.empty() ) {
                    option += '.';
                }
                option += channels[i];
                choices.push_back(option);
            }
        }


        {
            std::vector<std::string>::iterator pos = choices.begin();
            ++pos;

            const ImageComponents& rgba = ImageComponents::getRGBAComponents();
            const std::string& rgbaCompname = rgba.getComponentsGlobalName();
            const std::vector<std::string>& rgbaChannels = rgba.getComponentsNames();
            std::vector<std::string> rgbaOptions;
            for (std::size_t i = 0; i < rgbaChannels.size(); ++i) {
                std::string option = rgbaCompname + '.' + rgbaChannels[i];
                rgbaOptions.push_back(option);
            }
            choices.insert( pos, rgbaOptions.begin(), rgbaOptions.end() );
        }

        KnobChoicePtr channelKnob = it->second.channel.lock();
        const std::vector<std::string> currentLayerEntries = channelKnob->getEntries();
        const std::string curChannelEncoded = channelKnob->getActiveEntryText();

        // This will merge previous channels with new channels available while retaining previously existing channels.
        {
            MergeMaskChannelData tmpData;
            hasChanged |= channelKnob->populateChoices(choices, std::vector<std::string>(), maskChannelEqualityFunctor, &tmpData);
        }
    }
    return hasChanged;
} // Node::refreshChannelSelectors()

bool
Node::addUserComponents(const ImageComponents& comps)
{
    ///The node has node channel selector, don't allow adding a custom plane.
    KnobIPtr outputLayerKnob = getKnobByName(kNatronOfxParamOutputChannels);

    if (_imp->channelsSelectors.empty() && !outputLayerKnob) {
        return false;
    }

    if (!outputLayerKnob) {
        //The effect does not have kNatronOfxParamOutputChannels but maybe the selector provided by Natron
        std::map<int, ChannelSelector>::iterator found = _imp->channelsSelectors.find(-1);
        if ( found == _imp->channelsSelectors.end() ) {
            return false;
        }
        outputLayerKnob = found->second.layer.lock();
    }

    {
        QMutexLocker k(&_imp->createdComponentsMutex);
        for (std::list<ImageComponents>::iterator it = _imp->createdComponents.begin(); it != _imp->createdComponents.end(); ++it) {
            if ( it->getLayerName() == comps.getLayerName() ) {
                return false;
            }
        }

        _imp->createdComponents.push_back(comps);
    }
    if (!_imp->isRefreshingInputRelatedData) {
        ///Clip preferences have changed
        RenderScale s(1.);
        getEffectInstance()->refreshMetaDatas_public(true);
    }
    {
        ///Set the selector to the new channel
        KnobChoicePtr layerChoice = toKnobChoice(outputLayerKnob);
        if (layerChoice) {
            layerChoice->setValueFromLabel(comps.getLayerName());
        }
    }

    return true;
}

void
Node::getUserCreatedComponents(std::list<ImageComponents>* comps)
{
    QMutexLocker k(&_imp->createdComponentsMutex);

    *comps = _imp->createdComponents;
}

double
Node::getHostMixingValue(double time,
                         ViewIdx view) const
{
    KnobDoublePtr mix = _imp->mixWithSource.lock();

    return mix ? mix->getValueAtTime(time, DimIdx(0), view) : 1.;
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_Node.cpp"
