/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#include "Engine/EffectInstance.h"
#include "Engine/ViewIdx.h"

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER


#define kNatronGroupInputIsMaskParamName "isMask"
#define kNatronGroupInputIsOptionalParamName "optional"


struct NodeCollectionPrivate;

class NodeCollection
{
    Q_DECLARE_TR_FUNCTIONS(NodeCollection)

protected:

    // You must inherit this class
    NodeCollection(const AppInstancePtr& app);

public:

    virtual ~NodeCollection();

    virtual NodeCollectionPtr getThisShared() = 0;

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
    void getNodes_recursive(NodesList& nodes) const;

    /**
     * @brief Adds a node to the collection. MT-safe.
     **/
    void addNode(const NodePtr& node);

    /**
     * @brief Removes a node from the collection. MT-safe.
     **/
    void removeNode(const NodePtr& node);
    void removeNode(const Node* node);

    /**
     * @brief Get the last node added with the given id
     **/
    NodePtr getLastNode(const std::string& pluginID) const;

    /**
     * @brief Removes all nodes within the collection. MT-safe.
     **/
    void clearNodesBlocking();
    void clearNodesNonBlocking();

private:

    void clearNodesInternal();

public:


    /**
     * @brief Set the name of the node to be a unique node name within the collection. MT-safe.
     **/
    void initNodeName(const std::string& pluginID,
                      const std::string& pluginLabel,
                      std::string* nodeName);

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
    void getWriters(std::list<EffectInstancePtr>* writers) const;

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

    /**
     * @brief Refresh the meta-datas on all nodes in the group
     **/
    void refreshTimeInvariantMetadataOnAllNodes_recursive();

    enum CreateNodesFromSerializationFlagsEnum
    {
        eCreateNodesFromSerializationFlagsNone = 0x0,

        // When creating the node, its serialization contains a list of input nodes to connect to.
        // This flag indicates whether we should attempt to connect to these nodes, even if they are
        // not part of the serializedNodes list.
        // For example, we might have a list of nodes that the user copied from the NodeGraph.
        // We want to restore connections between nodes of that list so that the tree is preserved,
        // however we do not want the input nodes of that tree to connect to their original input which may
        // not be in this list. If this flag is passed, this will also attempt to restore inputs which are
        // not in this list.
        eCreateNodesFromSerializationFlagsConnectToExternalNodes = 0x1
    };

    /**
     * @brief Create nodes within this NodeCollection from their serialization.
     * This function is recursive because a serialized node may be a Group and contain serialization of nodes within itself.
     * @param serializedNodes List of nodes serialization to be created
     * @param createdNodes[out] For each serialization passed in input, a pointer to the created node, or NULL if the node could not be created.
     * This parameter is optional and may be set to NULL.
     * Note that this will return node pointers for the passed serializedNodes in the same order as they were
     * given. This will not include children nodes in case of groups.
     * @returns True if everything went fine, false otherwise
     **/
    bool createNodesFromSerialization(const SERIALIZATION_NAMESPACE::NodeSerializationList & serializedNodes,
                                      CreateNodesFromSerializationFlagsEnum flags,
                                      NodesList* createdNodes);

    /**
     * @brief Find a node by its script-name within the list of createdNodes. 
     * If allowSearchInAllNodes, this will also look for it in allNodesInGroup if it could not be found in createdNodes.
     * This function is useful when restoring serialized nodes, because when instanciated, their corresponding node may be attributed
     * a different script-name from what's within the serialization. 
     **/
    static NodePtr findSerializedNodeWithScriptName(const std::string& nodeScriptName,
                                                    const std::map<SERIALIZATION_NAMESPACE::NodeSerializationPtr, NodePtr>& createdNodes,
                                                    const NodesList& allNodesInGroup,
                                                    bool allowSearchInAllNodes);


    struct TopologicalSortNode;
    typedef boost::shared_ptr<TopologicalSortNode> TopologicalSortNodePtr;
    typedef boost::weak_ptr<TopologicalSortNode> TopologicalSortNodeWPtr;
    struct TopologicalSortNode
    {
        // A strong ref to the node
        NodePtr node;

        // True if this node is part of the nodes list given to extractTopologicallySortedTreesFromNodes
        // otherwise false.
        // We still add this node to the topological sort, even if not part of the nodes list so that
        // we can remember the outside context of the tree:
        // This is needed for example for the Group/UnGroup undo/redo command when we want to re-insert the nodes in
        // an existing tree.
        // If False, the outputs and inputs list are not filled so no further recursion is required.
        bool isPartOfGivenNodes;

        // List of output nodes within the same tree
        typedef std::map<TopologicalSortNodeWPtr, std::list<int> > InternalOutputsMap;

        typedef std::map<TopologicalSortNodePtr, std::list<int> > ExternalOutputsMap;
        InternalOutputsMap outputs;
        ExternalOutputsMap externalOutputs;

        // This list as the size of the number of inputs of the node: if an input is disconnected,
        // the input node is NULL
        typedef std::vector<TopologicalSortNodePtr> InputsVec;
        InputsVec inputs;

        TopologicalSortNode()
        : node()
        , isPartOfGivenNodes(false)
        , outputs()
        , externalOutputs()
        , inputs()
        {

        }

        /**
         * @brief Returns true if this node is a tree input, i.e:
         * if the node does not have any input, or all inputs are outside of the nodes list that
         * was given to extractTopologicallySortedTreesFromNodes
         **/
        bool isTreeInput() const;

        /**
         * @brief Returns true if this node is a tree output, i.e:
         * if the node does not have any output, or all outputs are outside of the nodes list that
         * was given to extractTopologicallySortedTreesFromNodes
         **/
        bool isTreeOutput() const;

    };

    typedef std::list<TopologicalSortNodePtr> TopologicallySortedNodesList;

    /**
     * @brief Extracts from the given output nodes list, a list of topologically sorted nodes tree.
     * For each tree stemming from an output node, the tree's input nodes and outputs also keep their link to nodes outside of the tree:
     * Imagine that the user copy a sub portion of at tree in the NodeGraph, we can still build a tree from this selection,
     * but we also remember the connections of the sub-tree to the actual tree it lives in.
     *
     * @param enterGroups If true, the topological sort will also cycle through recursively on any sub-node-graph
     * @param outputNodesList List of nodes which are considered output (root) of a tree.
     * @param allNodesList If non-null, any input or output of a node that is in not in this list is considered "out of the tree" and
     * will not be added to the topological sort
     * @param sortedNodes A list of nodes containing the topological ordering of the given nodes (from inputs to outputs)
     * This parameter is optional and may be set to NULL
     * @param outputNodes A list of the different outputs (tree roots) that were part of the nodes list. This parameter is optional
     * and may be set to NULL
     **/
    static void extractTopologicallySortedTreesForOutputs(bool enterGroups,
                                                          const NodesList& outputNodesList,
                                                          const NodesList* allNodesList,
                                                          NodeCollection::TopologicallySortedNodesList* sortedNodes,
                                                          std::list<NodeCollection::TopologicalSortNodePtr>* outputNodes);


    /**
     * @brief Same as extractTopologicallySortedTreesForOutputs but the outputNodesList is extracted from the nodes list directly:
     * any node within the list that doesn't have any output in the nodes list is marked as an output node.
     **/
    static void extractTopologicallySortedTreesFromNodes(bool enterGroups,
                                                         const NodesList& nodes,
                                                         TopologicallySortedNodesList* sortedNodes,
                                                         std::list<TopologicalSortNodePtr>* outputNodes);

    /**
     * @brief Same as extractTopologicallySortedTreesForOutputs, but the output nodes list is built from the output
     * nodes of this node graph.
     **/
    void extractTopologicallySortedTrees(bool enterGroups,
                                        TopologicallySortedNodesList* sortedNodes,
                                        std::list<TopologicalSortNodePtr>* outputNodes) const;



public:


    /**
     * @brief Computes the union of the frame range of all readers in the group and subgroups.
     **/
    void recomputeFrameRangeForAllReaders(int* firstFrame, int* lastFrame);

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
    virtual void notifyNodeLabelChanged(const NodePtr& /*node*/) {}


protected:

    virtual void onNodeRemoved(const Node* /*node*/) {}

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
    : public EffectInstance
    , public NodeCollection
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

protected: // derives from EffectInstance, parent of TrackerNode and WriteNode
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    NodeGroup(const NodePtr &node);

    NodeGroup(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key);
public:
    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new NodeGroup(node) );
    }

    static EffectInstancePtr createRenderClone(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new NodeGroup(mainInstance, key) );
    }

    virtual NodeCollectionPtr getThisShared() OVERRIDE FINAL
    {
        NodeGroupPtr isGroup = boost::dynamic_pointer_cast<NodeGroup>(shared_from_this());
        return boost::dynamic_pointer_cast<NodeCollection>(isGroup);
    }

    static PluginPtr createPlugin();

    virtual ~NodeGroup();


    virtual bool isOutput() const OVERRIDE WARN_UNUSED_RETURN
    {
        return false;
    }

    
    virtual TimeValue getCurrentRenderTime() const OVERRIDE WARN_UNUSED_RETURN;
    virtual ViewIdx getCurrentRenderView() const OVERRIDE WARN_UNUSED_RETURN;

    virtual void notifyNodeDeactivated(const NodePtr& node) OVERRIDE FINAL;
    virtual void notifyNodeActivated(const NodePtr& node) OVERRIDE FINAL;
    virtual void notifyInputOptionalStateChanged(const NodePtr& node) OVERRIDE FINAL;
    virtual void notifyInputMaskStateChanged(const NodePtr& node) OVERRIDE FINAL;
    virtual void notifyNodeLabelChanged(const NodePtr& node) OVERRIDE FINAL;
    virtual void purgeCaches() OVERRIDE FINAL;
    virtual void clearLastRenderedImage() OVERRIDE ;

    void refreshInputs();

    NodePtr getOutputNode() const;

    NodePtr getOutputNodeInput() const;

    NodePtr getRealInputForInput(const NodePtr& input) const;

    void getInputs(std::vector<NodePtr>* inputs) const;

    void getInputsOutputs(NodesList* nodes) const;


    bool getIsDeactivatingGroup() const;
    void setIsDeactivatingGroup(bool b);

    bool getIsActivatingGroup() const;
    void setIsActivatingGroup(bool b);

    /**
     * @brief For sub-classes override to choose whether to create a node graph that will ever be visible by the user or not.
     **/
    virtual bool isSubGraphUserVisible() const
    {
        return true;
    }

    /*
     * If returning false, all nodes in the node-graph are expected to be non-persistent, i.e:
     * when reloading the node, it should create the internal nodes again in setupInitialSubGraphState()
     */
    virtual bool isSubGraphPersistent() const
    {
        return true;
    }


    /**
     * @brief Called when the Group is created or when the node is reset to default to re-initialize the node to its default state.
     **/
    virtual void setupInitialSubGraphState();

    virtual void loadSubGraph(const SERIALIZATION_NAMESPACE::NodeSerialization* projectSerialization,
                              const SERIALIZATION_NAMESPACE::NodeSerialization* pyPlugSerialization);


Q_SIGNALS:

    void graphEditableChanged(bool);

private:

    virtual bool isRenderCloneNeeded() const OVERRIDE FINAL;

    virtual void onNodeRemoved(const Node* node) OVERRIDE FINAL;

    // A group render function should never get called
    virtual ActionRetCodeEnum render(const RenderActionArgs& /*args*/) OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        assert(false);
        return eActionStatusFailed;
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


NATRON_NAMESPACE_EXIT

#endif // NODEGROUP_H
