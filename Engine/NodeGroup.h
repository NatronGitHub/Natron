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

#ifndef NODEGROUP_H
#define NODEGROUP_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <set>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QCoreApplication>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/OutputEffectInstance.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"


#define kNatronGroupInputIsMaskParamName "isMask"
#define kNatronGroupInputIsOptionalParamName "optional"

NATRON_NAMESPACE_ENTER;


struct NodeCollectionPrivate;

class NodeCollection
{
    Q_DECLARE_TR_FUNCTIONS(NodeCollection)

public:
    NodeCollection(const AppInstancePtr& app);

    virtual ~NodeCollection();

    void setNodeGraphPointer(NodeGraphI* graph);

    void discardNodeGraphPointer();

    NodeGraphI* getNodeGraph() const;
    AppInstancePtr getApplication() const;

    /**
     * @brief Returns a copy of the nodes within the collection. MT-safe.
     **/
    NodesList getNodes() const;

    /**
     * @brief Same as getNodes() except that this function recurse in sub-groups.
     **/
    void getNodes_recursive(NodesList& nodes, bool onlyActive) const;

    /**
     * @brief Adds a node to the collection. MT-safe.
     **/
    void addNode(const NodePtr& node);

    /**
     * @brief Removes a node from the collection. MT-safe.
     **/
    void removeNode(const NodePtr& node);

    /**
     * @brief Get the last node added with the given id
     **/
    NodePtr getLastNode(const std::string& pluginID) const;

    /**
     * @brief Removes all nodes within the collection. MT-safe.
     **/
    void clearNodes(bool emitSignal);


    /**
     * @brief Set the name of the node to be a unique node name within the collection. MT-safe.
     **/
    void initNodeName(const std::string& pluginLabel, std::string* nodeName);

    /**
     * @brief Given the baseName, set in nodeName a possible script-name for the node.
     * @param appendDigit If a node with the same script-name exists, try to add digits at the end until no match is found
     * @param errorIfExists If a node with the same script-name exists, error
     * This function throws a runtime exception with the error message in case of error.
     **/
    void checkNodeName(const NodeConstPtr& node, const std::string& baseName, bool appendDigit, bool errorIfExists, std::string* nodeName);

    /**
     * @brief Returns true if there is one or more nodes in the collection.
     **/
    bool hasNodes() const;

    /**
     * @brief Returns true if a node is currently rendering in this collection. This is recursive and will look
     * into all sub collections too
     **/
    bool hasNodeRendering() const;

    /**
     * @brief Connects the node 'input' to the node 'output' on the input number 'inputNumber'
     * of the node 'output'. If 'force' is true, then it will disconnect any previous connection
     * existing on 'inputNumber' and connect the previous input as input of the new 'input' node.
     **/
    static bool connectNodes(int inputNumber, const NodePtr& input, const NodePtr& output, bool force = false);

    /**
     * @brief Same as above where inputName is the name of the node input.
     **/
    bool connectNodes(int inputNumber, const std::string & inputName, const NodePtr& output);

    /**
     * @brief Disconnects the node 'input' and 'output' if any connection between them is existing.
     * If autoReconnect is true, after disconnecting 'input' and 'output', if the 'input' had only
     * 1 input, and it was connected, it will connect output to the input of  'input'.
     **/
    static bool disconnectNodes(const NodePtr& input, const NodePtr& output, bool autoReconnect = false);


    /**
     * @brief Attempts to connect automatically selected and created together, depending on their role (output / filter / input).
     **/
    bool autoConnectNodes(const NodePtr& selected, const NodePtr& created);

    /**
     * @brief Returns true if a node has the give name n in the group. This is not called recursively on subgroups.
     **/
    bool checkIfNodeNameExists(const std::string & n, const NodeConstPtr& caller) const;

    /**
     * @brief Returns true if a node has the give label n in the group. This is not called recursively on subgroups.
     **/
    bool checkIfNodeLabelExists(const std::string & n, const NodeConstPtr& caller) const;

    /**
     * @brief Returns a pointer to a node whose name is the same as the name given in parameter.
     * If no such node could be found, NULL is returned.
     **/
    NodePtr getNodeByName(const std::string & name) const;

    /**
     * @brief Given a fully specified name, e.g: Group1.Group2.Blur1.1, this function extracts the first node name (starting from the left)
     * and put it in the variable 'name'. In this example, name = Group1
     * The 'remainder' contains the rest of the string, without the separating '.', i.e in this example: Group2.Blur1.1
     **/
    static void getNodeNameAndRemainder_LeftToRight(const std::string& fullySpecifiedName, std::string& name, std::string& remainder);

    /**
     * @brief Same as getNodeNameAndRemainder_LeftToRight excepts that this function scans the fullySpecifiedName from right to left,
     * hence in the example above, name = Blur1.1 and remainder = Group1.Group2
     **/
    static void getNodeNameAndRemainder_RightToLeft(const std::string& fullySpecifiedName, std::string& name, std::string& remainder);

    /**
     * @brief Same as getNodeByName() but recursive on each sub groups. As a node in a subgroup can have the same name
     * of a node in another group, the node name is "specified", e.g: <g>group1</g><g>group1</g>Blur1 to uniquely identify it.
     * @see Node::getFullySpecifiedName
     **/
    NodePtr getNodeByFullySpecifiedName(const std::string& fullySpecifiedName) const;

    /**
     * @brief Refresh recursively all previews and all viewers of each node and each sub-group
     **/
    void refreshViewersAndPreviews();
    void refreshPreviews();
    void forceRefreshPreviews();

    void quitAnyProcessingForAllNodes_non_blocking();
    void quitAnyProcessingForAllNodes_blocking();

    /**
     * @brief For all active nodes, find all file-paths that uses the given projectPathName and if the location was valid,
     * change the file-path to be relative to the newProjectPath.
     **/
    void fixRelativeFilePaths(const std::string& projectPathName, const std::string& newProjectPath, bool blockEval);


    /**
     * @brief For all active nodes, if it has a file-path parameter using the oldName of a variable, it will turn it into the
     * newName.
     **/
    void fixPathName(const std::string& oldName, const std::string& newName);

    /**
     * @brief Get a list of all active nodes
     **/
    void getActiveNodes(NodesList* nodes) const;

    void getActiveNodesExpandGroups(NodesList* nodes) const;

    /**
     * @brief Get all viewers in the group and sub groups
     **/
    void getViewers(std::list<ViewerInstancePtr>* viewers) const;

    /**
     * @brief Get all Writers in the group and sub groups
     **/
    void getWriters(std::list<OutputEffectInstancePtr>* writers) const;

    /**
     * @brief Controls whether the user can ever edit this graph from the UI. 
     **/
    void setSubGraphEditable(bool editable);
    bool isSubGraphEditable() const;

    /**
     * @brief Controls whether the sub-graph is assumed to be edited by the user
     **/
    void setSubGraphEditedByUser(bool edited);
    bool isSubGraphEditedByUser() const;

public:


    /**
     * @brief Computes the union of the frame range of all readers in the group and subgroups.
     **/
    void recomputeFrameRangeForAllReaders(int* firstFrame, int* lastFrame);


    void getParallelRenderArgs(std::map<NodePtr, ParallelRenderArgsPtr >& argsMap) const;


    void forceComputeInputDependentDataOnAllTrees();

    /**
     * @brief Callback called when a node of the collection is being deactivated
     **/
    virtual void notifyNodeDeactivated(const NodePtr& /*node*/) {}

    /**
     * @brief Callback called when a node of the collection is being activated
     **/
    virtual void notifyNodeActivated(const NodePtr& /*node*/) {}

    /**
     * @brief Callback called when an input of the group changed
     **/
    virtual void notifyInputOptionalStateChanged(const NodePtr& /*node*/) {}

    /**
     * @brief Callback called when an input of the group changed
     **/
    virtual void notifyInputMaskStateChanged(const NodePtr& /*node*/) {}


    /**
     * @brief Callback called when a node of the collection is being activated
     **/
    virtual void notifyNodeNameChanged(const NodePtr& /*node*/) {}


protected:


    virtual void onGraphEditableChanged(bool /*changed*/) {}

private:
    void quitAnyProcessingInternal(bool blocking);
    void recomputeFrameRangeForAllReadersInternal(int* firstFrame,
                                                  int* lastFrame,
                                                  bool setFrameRange);
    void exportGroupInternal(int indentLevel,
                             const NodePtr& upperLevelGroupNode,
                             const QString& upperLevelGroupName,
                             QTextStream& ts);

private:
    boost::scoped_ptr<NodeCollectionPrivate> _imp;
};


struct NodeGroupPrivate;
class NodeGroup
    : public OutputEffectInstance
    , public NodeCollection
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

protected: // derives from EffectInstance, parent of TrackerNode and WriteNode
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    NodeGroup(const NodePtr &node);

public:
    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new NodeGroup(node) );
    }

    static PluginPtr createPlugin();

    virtual ~NodeGroup();


    virtual bool isOutput() const OVERRIDE WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool supportsMultipleClipDepths() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool supportsMultipleClipFPSs() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool supportsMultipleClipPARs() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }


    virtual int getMaxInputCount() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isInputOptional(int inputNb) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isInputMask(int inputNb) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::string getInputLabel(int inputNb) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual double getCurrentTime() const OVERRIDE WARN_UNUSED_RETURN;
    virtual ViewIdx getCurrentView() const OVERRIDE WARN_UNUSED_RETURN;
    virtual void addAcceptedComponents(int inputNb, std::list<ImageComponents>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const OVERRIDE FINAL;
    virtual void notifyNodeDeactivated(const NodePtr& node) OVERRIDE FINAL;
    virtual void notifyNodeActivated(const NodePtr& node) OVERRIDE FINAL;
    virtual void notifyInputOptionalStateChanged(const NodePtr& node) OVERRIDE FINAL;
    virtual void notifyInputMaskStateChanged(const NodePtr& node) OVERRIDE FINAL;
    virtual void notifyNodeNameChanged(const NodePtr& node) OVERRIDE FINAL;
    virtual bool getCreateChannelSelectorKnob() const OVERRIDE WARN_UNUSED_RETURN { return false; }

    virtual bool isHostChannelSelectorSupported(bool* defaultR, bool* defaultG, bool* defaultB, bool* defaultA) const OVERRIDE WARN_UNUSED_RETURN;
    virtual void purgeCaches() OVERRIDE FINAL;
    virtual void clearLastRenderedImage() OVERRIDE ;

    virtual void invalidateHashCache(bool invalidateParent = true) OVERRIDE ;

    NodePtr getOutputNode(bool useGuiConnexions) const;

    NodePtr getOutputNodeInput(bool useGuiConnexions) const;

    NodePtr getRealInputForInput(bool useGuiConnexions, const NodePtr& input) const;

    void getInputs(std::vector<NodePtr>* inputs, bool useGuiConnexions) const;

    void getInputsOutputs(NodesList* nodes, bool useGuiConnexions) const;

    void dequeueConnexions();

    bool getIsDeactivatingGroup() const;
    void setIsDeactivatingGroup(bool b);

    bool getIsActivatingGroup() const;
    void setIsActivatingGroup(bool b);

    /**
     * @brief For sub-classes override to choose whether to create a node graph that will be visible by the user or not.
     * If returning false, the nodegraph will NEVER be created.
     **/
    virtual bool isSubGraphUserVisible() const
    {
        return true;
    }

    /**
     * @brief Callback called when the node gui has been created
     **/
    virtual void onEffectCreated(bool mayCreateFileDialog, const CreateNodeArgs& args) OVERRIDE;

    /**
     * @brief Callback called after the internal group has been restored completely. 
     * Base implementation doesn't do anything
     **/
    virtual void onGroupLoaded() {}

    /**
     * @brief Called when the Group is created or when the node is reset to default to re-initialize the node to its default state.
     **/
    virtual void setupInitialSubGraphState(const SERIALIZATION_NAMESPACE::NodeSerialization* serialization );

Q_SIGNALS:

    void graphEditableChanged(bool);

private:

    // A group render function should never get called
    virtual StatusEnum render(const RenderActionArgs& /*args*/) OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return eStatusOK;
    }

    virtual void onGraphEditableChanged(bool changed) OVERRIDE FINAL
    {
        Q_EMIT graphEditableChanged(changed);
    }

    boost::scoped_ptr<NodeGroupPrivate> _imp;
};

inline NodeGroupPtr
toNodeGroup(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<NodeGroup>(effect);
}

inline NodeGroupPtr
toNodeGroup(const NodeCollectionPtr& group)
{
    return boost::dynamic_pointer_cast<NodeGroup>(group);
}

NATRON_NAMESPACE_EXIT;

#endif // NODEGROUP_H
