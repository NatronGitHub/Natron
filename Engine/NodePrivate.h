/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_NODE_PRIVATE_H
#define NATRON_ENGINE_NODE_PRIVATE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>
#include <list>
#include <bitset>

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include <QtCore/QCoreApplication>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QDebug>

#include "Engine/EffectInstance.h"
#include "Global/FStreamsSupport.h"
#include "Engine/Node.h"
#include "Engine/NodeMetadata.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/GenericSchedulerThreadWatcher.h"
#include "Engine/AppInstance.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/GroupInput.h"
#include "Engine/NodeGuiI.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeGraphI.h"
#include "Engine/Project.h"
#include "Engine/ReadNode.h"
#include "Engine/RenderQueue.h"
#include "Engine/WriteNode.h"

#include "Serialization/KnobSerialization.h"
#include "Serialization/NodeSerialization.h"
#include "Serialization/NodeClipBoard.h"
#include "Serialization/SerializationIO.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

typedef std::map<NodeWPtr, std::list<int> > InternalOutputNodesMap;

typedef std::vector<NodeWPtr> InputsV;


struct NodePrivate
{
    Q_DECLARE_TR_FUNCTIONS(Node)

public:
    NodePrivate(Node* publicInterface,
                   const AppInstancePtr& app_,
                   const NodeCollectionPtr& collection,
                const PluginPtr& plugin_);

    void abortPreview_non_blocking();

    void abortPreview_blocking(bool allowPreviewRenders);

    bool checkForExitPreview();

    void setComputingPreview(bool v)
    {
        QMutexLocker l(&computingPreviewMutex);

        computingPreview = v;
    }

    void runOnNodeCreatedCB(bool userEdited);

    void runOnNodeDeleteCB();

    bool figureOutCallbackName(const std::string& inCallback, std::string* outCallback);

    void runChangedParamCallback(const std::string& cb, const KnobIPtr& k, bool userEdited);

    void runOnNodeCreatedCBInternal(const std::string& cb, bool userEdited);

    void runOnNodeDeleteCBInternal(const std::string& cb);

    void runInputChangedCallback(int index, const std::string& script);

    void runAfterItemsSelectionChangedCallback(const std::string& script, const KnobItemsTablePtr& table, const std::list<KnobTableItemPtr>& deselected, const std::list<KnobTableItemPtr>& selected, TableChangeReasonEnum reason);

    void refreshDefaultPagesOrder();

    void refreshDefaultViewerKnobsOrder();



    ////////////////////////////////////////////////

    // Ptr to public interface, can not be a smart ptr
    Node* _publicInterface;

    // If this node is an output node, this is a pointer to the render engine.
    // This is the object that schedules playback, renders and separate threads.
    RenderEnginePtr renderEngine;

    // The group containing this node
    mutable QMutex groupMutex;
    NodeCollectionWPtr group;

    // pointer to the app: needed to access project stuff
    AppInstanceWPtr app;

    // If true, the node is serialized
    bool isPersistent;

    // Protects outputs
    mutable QMutex outputsMutex;

    // Map of weak references to the output nodes (and the list of input numbers of the output node connected to this node)
    InternalOutputNodesMap outputs;


    // Protects inputs
    mutable QMutex inputsMutex; //< protects inputs so the serialization thread can access them

    // vector of weak references to input nodes
    InputsV inputs;

    // Data for each input, initialized from the plug-in descriptor. It may be modified afterwards hence
    // this is a copy from the plug-in
    std::vector<InputDescriptionPtr> inputDescriptions;

    // Pointer to the effect hosted by this node.
    // This is the main effect and cannot be used to render, instead
    // small lightweights render clones are created to render.
    EffectInstancePtr effect;

    // true when we're running inside an interact action
    // Only valid on the main-thread
    bool duringInteractAction;

    // Protects scriptName and label
    mutable QMutex nameMutex;

    // Node name internally and as visible to python.
    // May only be set to a Python compliant variable name (no strange characters)
    std::string scriptName;

    // Node label as visible in the GUI. Can be set to any-thing.
    std::string label;

    // The plugin which stores the function to instantiate the effect
    PluginWPtr plugin;

    // If this node was created from a PyPlug this is a pointer to the PyPlug plug-in handle.
    PluginWPtr pyPlugHandle;

    // True if this node is a PyPlug
    bool isPyPlug;

    // True while computing a preview.
    bool computingPreview;

    // True when the preview thread has quit
    bool previewThreadQuit;

    // Protects computingPreview and previewThreadQuit
    mutable QMutex computingPreviewMutex;

    // Not 0 when we should abort the preview
    int mustQuitPreview;

    // Protects mustQuitPreview
    QMutex mustQuitPreviewMutex;

    // Protected by mustQuitPreviewMutex. The thread aborting the preview rendering
    // waits in this condition until the preview is aborted
    QWaitCondition mustQuitPreviewCond;

    // When creating a Reader or Writer node, this is a pointer to the meta node that the user actually sees.
    NodeWPtr ioContainer;


    // Protects lastRenderStartedSlotCallTime & lastInputNRenderStartedSlotCallTime
    QMutex lastRenderStartedMutex;

    // This is to avoid the slots connected to the main-thread to be called too much
    timeval lastRenderStartedSlotCallTime;
    int renderStartedCounter;
    std::vector<int> inputIsRenderingCounter;
    timeval lastInputNRenderStartedSlotCallTime;

    // The last persistent message posted by the plug-in
    PersistentMessageMap persistentMessages;

    // Protects persistentMessage & persistentMessageType
    mutable QMutex persistentMessageMutex;

    // Pointer to the node gui if any
    boost::weak_ptr<NodeGuiI> guiPointer;

    // True when the node has its load() function complete
    bool nodeCreated;

    // True if the node was created with the kCreateNodeArgsPropSilent flag
    bool wasCreatedSilently;

    // Protects isBeingDestroyed
    mutable QMutex isBeingDestroyedMutex;

    // true when the node is in the destroyNode function
    bool isBeingDestroyed;

    // Used to bracket calls to onInputChanged to ensure stuff that needs to be recomputed
    // when inputs are changed is computed once.
    int inputModifiedRecursion;

    // Used to bracket inputs description modification and emit inputsDescriptionChanged only once
    int hasModifiedInputsDescription;

    // Input indices that changed whilst in the beginInput/endInputChanged bracket
    std::set<int> inputsModified;

    // For readers, this is the name of the views in the file.
    // This is read from the kReadOIIOAvailableViewsKnobName knob
    std::vector<std::string> createdViews;

    // To concatenate calls to refreshIdentityState, accessed only on main-thread
    mutable QMutex refreshIdentityStateRequestsCountMutex;
    int refreshIdentityStateRequestsCount;

    // Map of warnings that should be displayed on the NodeGui indicating issues in the stream
    std::map<Node::StreamWarningEnum, QString> streamWarnings;

    // Some plug-ins (mainly Hitfilm Ignite detected for now) use their own OpenGL context that is sharing resources with our OpenGL contexT.
    // as a result if we don't call glFinish() before calling the render action, the plug-in context might use textures that were not finished yet.
    bool requiresGLFinishBeforeRender;

    // Used in the implementation of EffectInstance::onMetadataChanged_recursive so we know if the metadata changed or not.
    U64 lastTimeInvariantMetadataHashRefreshed;

    // UI
    mutable QMutex nodeUIDataMutex;
    double nodePositionCoords[2]; // x,y  X=Y=INT_MIN if there is no position info
    double nodeSize[2]; // width, height, W=H=-1 if there is no size info
    double nodeColor[3]; // node color (RGB), between 0. and 1. If R=G=B=-1 then no color
    double overlayColor[3]; // overlay color (RGB), between 0. and 1. If R=G=B=-1 then no color
    bool overlayActionsDraftEnabled;// If true, modyfing a parameter during an overlay action will issue a draft render
    bool nodeIsSelected; // is this node selected by the user ?

    // The name of the preset with which this node was created
    mutable QMutex nodePresetMutex;
    std::string initialNodePreset;

    // This is a list of KnobPage script-names defining the ordering of the pages in the settings panel.
    // This is used to determine if the ordering has changed or not for serialization purpose
    std::list<std::string> defaultPagesOrder;

    // This is a list of Knob script-names which are by default in the viewer interface.
    // This is used to determine if the ordering has changed or not for serialization purpose
    std::list<std::string> defaultViewerKnobsOrder;

    // True when restoreNodeToDefault is called
    bool restoringDefaults;

};



class ComputingPreviewSetter_RAII
{
    NodePrivate* _imp;

public:
    ComputingPreviewSetter_RAII(NodePrivate* imp)
    : _imp(imp)
    {
        _imp->setComputingPreview(true);
    }

    ~ComputingPreviewSetter_RAII()
    {
        _imp->setComputingPreview(false);

        bool mustQuitPreview = _imp->checkForExitPreview();
        Q_UNUSED(mustQuitPreview);
    }
};




NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_NODE_PRIVATE_H
