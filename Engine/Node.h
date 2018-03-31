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

#ifndef NATRON_ENGINE_NODE_H
#define NATRON_ENGINE_NODE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector>
#include <string>
#include <map>
#include <list>
#include <bitset>

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QMetaType>
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
#include <QtCore/QMutex>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif
#include "Engine/AppManager.h"
#include "Global/KeySymbols.h"
#include "Engine/ChoiceOption.h"
#include "Engine/InputDescription.h"
#include "Engine/DimensionIdx.h"
#include "Engine/ImagePlaneDesc.h"
#include "Serialization/SerializationBase.h"
#include "Engine/ViewIdx.h"
#include "Engine/Markdown.h"

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER


struct PersistentMessage
{
    // The message
    std::string message;

    // Warning/Error/Info...
    MessageTypeEnum type;

};


// Each message, mapped against an ID to check if a message of this type is present or not
typedef std::map<std::string, PersistentMessage> PersistentMessageMap;
typedef std::map<NodePtr, std::list<int> > OutputNodesMap;


struct NodePrivate;
class Node
    : public QObject
    , public boost::enable_shared_from_this<Node>
    , public SERIALIZATION_NAMESPACE::SerializableObjectBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON


protected: 
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    Node(const AppInstancePtr& app,
         const NodeCollectionPtr& group,
         const PluginPtr& plugin);

public:
    static NodePtr create(const AppInstancePtr& app,
                          const NodeCollectionPtr& group,
                          const PluginPtr& plugin)
    {
        return NodePtr( new Node(app, group, plugin) );
    }

    virtual ~Node();

    NodeCollectionPtr getGroup() const;

    /**
     * @brief Returns true if this node is a "user" node. For internal invisible node, this would return false.
     * If this function returns false, the node will not be serialized.
     **/
    bool isPersistent() const;

    // returns the plug-in currently instanciated in the node
    PluginPtr getPlugin() const;

    // For pyplugs, returns the handle to the pyplug
    PluginPtr getPyPlugPlugin() const;

    // For groups which may be pyplugs, return the handle of the group plugin
    PluginPtr getOriginalPlugin() const;

    /**
     * @brief Creates the EffectInstance that will be embedded into this node and set it up.
     * This function also loads all parameters. Node connections will not be setup in this method.
     **/
    void load(const CreateNodeArgsPtr& args);


    void initNodeScriptName(const SERIALIZATION_NAMESPACE::NodeSerialization* serialization, const QString& fixedName);


    void loadKnob(const KnobIPtr & knob, const std::list<SERIALIZATION_NAMESPACE::KnobSerializationPtr> & serialization);

    /**
     * @brief Links all the evaluateOnChange knobs to the other one except
     * trigger buttons. The other node must be the same plug-in
     **/
    bool linkToNode(const NodePtr& other);

    /**
     * @brief Unlink all knobs of the node.
     **/
    void unlinkAllKnobs();


    /**
     * @brief Get a list of all nodes that are linked to this one.
     * For each node in return a boolean indicates whether the link is a clone link
     * or a just a regular link.
     * A node is considered clone if all its evaluate on change knobs are linked.
     **/
    void getLinkedNodes(std::list<std::pair<NodePtr, bool> >* nodes) const;

    /**
     * @brief Wrapper around getLinkedNodes() that returns cloned nodes
     **/
    void getCloneLinkedNodes(std::list<NodePtr>* clones) const;

    /**
     * @brief Move this node to the given group
     **/
    void moveToGroup(const NodeCollectionPtr& group);

private:

    void initNodeNameFallbackOnPluginDefault();

    void createNodeGuiInternal(const CreateNodeArgsPtr& args);

  

    void restoreUserKnob(const KnobGroupPtr& group,
                         const KnobPagePtr& page,
                         const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase,
                         bool setUserKnobsAsPluginKnobs,
                         unsigned int recursionLevel);

public:


    /**
     * @brief Implement to save the content of the object to the serialization object
     **/
    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* serializationBase) OVERRIDE FINAL;

    /**
     * @brief Implement to load the content of the serialization object onto this object
     **/
    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase& serializationBase) OVERRIDE FINAL;

    void loadInternalNodeGraph(bool initialSetupAllowed,
                               const SERIALIZATION_NAMESPACE::NodeSerialization* projectSerialization,
                               const SERIALIZATION_NAMESPACE::NodeSerialization* pyPlugSerialization);

    void loadKnobsFromSerialization(const SERIALIZATION_NAMESPACE::NodeSerialization& serialization, bool setUserKnobsAsPluginKnobs);

    void getNodeSerializationFromPresetFile(const std::string& presetFile, SERIALIZATION_NAMESPACE::NodeSerialization* serialization);

    void getNodeSerializationFromPresetName(const std::string& presetName, SERIALIZATION_NAMESPACE::NodeSerialization* serialization);

private:


    void loadPresetsInternal(const SERIALIZATION_NAMESPACE::NodeSerializationPtr& serialization, bool setKnobsDefault, bool setUserKnobsAsPluginKnobs);

public:

    void refreshDefaultPagesOrder();

    /**
     * @brief Setup the node state according to the presets file.
     * This function throws exception in case of error.
     **/
    void loadPresets(const std::string& presetsLabel);

    /**
     * @brief Clear any preset flag on the node, but does'nt change the configuration
     **/
    void clearPresetFlag();

    void loadPresetsFromFile(const std::string& presetsFile);

    void exportNodeToPyPlug(const std::string& filePath);
    void exportNodeToPresets(const std::string& filePath,
                             const std::string& presetsLabel,
                             const std::string& iconFilePath,
                             int shortcutSymbol,
                             int shortcutModifiers);

public:




    std::string getCurrentNodePresets() const;

    /**
     * @brief Restores the node to its default state. If this node has a preset active or it is a PyPlug
     * it will be restored according to the preset/PyPlug.
     **/
    void restoreNodeToDefaultState(const CreateNodeArgsPtr& args);

    ///Set values for Knobs given their serialization
    void setValuesFromSerialization(const CreateNodeArgs& args);


    ///function called by EffectInstance to create a knob
    template <class K>
    boost::shared_ptr<K> createKnob(const std::string &description,
                                    int dimension = 1,
                                    bool declaredByPlugin = true)
    {
        return appPTR->getKnobFactory().createKnob<K>(getEffectInstance(), description, dimension, declaredByPlugin);
    }

    ///This cannot be done in loadKnobs as to call this all the nodes in the project must have
    ///been loaded first.
    void restoreKnobsLinks(const SERIALIZATION_NAMESPACE::NodeSerialization & serialization,
                           const std::map<SERIALIZATION_NAMESPACE::NodeSerializationPtr, NodePtr>& allCreatedNodesInGroup);

    void setPagesOrder(const std::list<std::string>& pages);

    std::list<std::string> getPagesOrder() const;

    bool hasPageOrderChangedSinceDefault() const;

    bool isNodeCreated() const;

    bool isGLFinishRequiredBeforeRender() const;


    /**
     * @brief Quits any processing on going on this node, this call is non blocking
     **/
    void quitAnyProcessing_non_blocking();
    void quitAnyProcessing_blocking(bool allowThreadsToRestart);

    /**
     * @brief Returns true if all processing threads controlled by this node have quit
     **/
    bool areAllProcessingThreadsQuit() const;

    /* @brief Similar to quitAnyProcessing except that the threads aren't destroyed
     * This is called when a node is deleted by the user
     */
    void abortAnyProcessing_non_blocking();
    void abortAnyProcessing_blocking();


    EffectInstancePtr getEffectInstance() const;
    /**
     * @brief Forwarded to the live effect instance
     **/
    void beginEditKnobs();

    /**
     * @brief Forwarded to the live effect instance
     **/
    const std::vector<KnobIPtr> & getKnobs() const;


    /**
     * @brief If this node is an output node, return the render engine
     **/
    RenderEnginePtr getRenderEngine() const;

    /**
     * @brief Is this node render engine currently doing playback ?
     **/
    bool isDoingSequentialRender() const;

  

    /**
     * @brief Forwarded to the live effect instance
     **/
    int getMajorVersion() const;

    /**
     * @brief Forwarded to the live effect instance
     **/
    int getMinorVersion() const;


    /**
     * @brief Forwarded to the live effect instance
     **/
    bool isInputNode() const;

    /**
     * @brief Forwarded to the live effect instance
     **/
    bool isOutputNode() const;


    /**
     * @brief Returns true if this node is a backdrop
     **/
    bool isBackdropNode() const;


    ViewerNodePtr isEffectViewerNode() const;

    ViewerInstancePtr isEffectViewerInstance() const;

    NodeGroupPtr isEffectNodeGroup() const;

    StubNodePtr isEffectStubNode() const;

    PrecompNodePtr isEffectPrecompNode() const;

    GroupInputPtr isEffectGroupInput() const;

    GroupOutputPtr isEffectGroupOutput() const;

    ReadNodePtr isEffectReadNode() const;

    WriteNodePtr isEffectWriteNode() const;

    BackdropPtr isEffectBackdrop() const;

    OneViewNodePtr isEffectOneViewNode() const;


    /**
     * @brief Hint indicating to the UI that this node has numerous optional inputs and should not display them all.
     * See Switch/Viewer node for examples
     **/
    bool isEntitledForInspectorInputsStyle() const;


    /**
     * @brief Returns a pointer to the input Node at index 'index'
     * or NULL if it couldn't find such node.
     * Note that this function cycles through Group nodes:
     * If the return value is a Group node, the return value will actually be the
     * input of the GroupOutput node itself.
     * If the return value is a GroupInput node, the return value will actually be the
     * corresponding input of the GroupInput node 
     *
     * MT-safe
     **/
    NodePtr getInput(int index) const;

    /**
     * @brief Same as getInput except that it doesn't do group redirections for Inputs/Outputs
     **/
    NodePtr getRealInput(int index) const;

private:

    NodePtr getInputInternal(bool useGroupRedirections, int index) const;

public:


    /**
     * @brief Returns a map of nodes connected in output of this node.
     * Each output node has a list of indices corresponding to each input index of the
     * output node connected to this node.
     **/
    void getOutputs(OutputNodesMap& outputs) const;

    /**
     * @brief Returns the list of outputs of this node but accounts for Groups:
     * If this node is a Group, this returns the list of all nodes in output of the GroupInput
     * nodes within the Group.
     * If this node is a GroupOutput, this returns the list of all nodes in output of the Group node itself.
     **/
    void getOutputsWithGroupRedirection(NodesList& outputs) const;


    /**
     * @brief Returns a list of all input indices of outputNode connected to this node
     **/
    std::list<int> getInputIndicesConnectedToThisNode(const NodeConstPtr& outputNode) const;


    /**
     * @brief Returns true if the node is currently executing the onInputChanged handler.
     **/
    bool duringInputChangedAction() const;


    /**
     * @brief Returns the number of inputs
     **/
    int getNInputs() const;

    /**
     *@brief Returns the inputs of the node as the Gui just set them.
     * The vector might be different from what getInputs_other_thread() could return.
     * This can only be called by the main thread.
     **/
    const std::vector<NodeWPtr> & getInputs() const WARN_UNUSED_RETURN;
    std::vector<NodeWPtr> getInputs_copy() const WARN_UNUSED_RETURN;

    std::string getInputLabel(int inputNb) const;

    void setInputLabel(int inputNb, const std::string& label);

    std::string getInputHint(int inputNb) const;

    void setInputHint(int inputNb, const std::string& hint);

    bool isInputVisible(int inputNb) const;

    void setInputVisible(int inputNb, bool visible);

    ImageFieldExtractionEnum getInputFieldExtraction(int inputNb) const;

    std::bitset<4> getSupportedComponents(int inputNb) const;

    bool isInputHostDescribed(int inputNb) const;

    bool isInputMask(int inputNb) const;

    bool isInputOptional(int inputNb) const;

    bool isTilesSupportedByInput(int inputNb) const;

    bool isTemporalAccessSupportedByInput(int inputNb) const;

    bool canInputReceiveDistortion(int inputNb) const;

    bool canInputReceiveTransform3x3(int inputNb) const;

    int getInputNumberFromLabel(const std::string& inputLabel) const;

    bool isInputConnected(int inputNb) const;

    bool hasOutputConnected() const;

    bool hasInputConnected() const;

    bool hasMandatoryInputDisconnected() const;

    bool hasAllInputsConnected() const;

    void addInput(const InputDescriptionPtr& description);

    void insertInput(int index, const InputDescriptionPtr& description);

    void changeInputDescription(int inputNb, const InputDescriptionPtr& description);

    bool getInputDescription(int inputNb, InputDescription* desc) const;

    void removeInput(int inputNb);

    void removeAllInputs();


    /**
     * @brief Returns true if the given input supports the given components. If inputNb equals -1
     * then this function will check whether the effect can produce the given components.
     **/
    bool isSupportedComponent(int inputNb, const ImagePlaneDesc& comp) const;

    /**
     * @brief Returns the most appropriate number of components that can be supported by the inputNb.
     * If inputNb equals -1 then this function will check the output components.
     **/
    int findClosestSupportedNumberOfComponents(int inputNb, int nComps) const;

    std::list<ImageBitDepthEnum> getSupportedBitDepths() const;

    ImageBitDepthEnum getBestSupportedBitDepth() const;
    bool isSupportedBitDepth(ImageBitDepthEnum depth) const;
    ImageBitDepthEnum getClosestSupportedBitDepth(ImageBitDepthEnum depth);


    /**
     * @brief This is used by the auto-connection algorithm.
     * When connecting nodes together this function helps determine
     * on which input it should connect a new node.
     * It returns the first non optional empty input or the first optional
     * empty input if they are all optionals, or -1 if nothing matches the 2 first conditions..
     * if all inputs are connected.
     **/
    virtual int getPreferredInputForConnection()  const;
    virtual int getPreferredInput() const;

    NodePtr getPreferredInputNode() const;

    bool isRenderScaleSupportEnabledForPlugin() const;

    bool isMultiThreadingSupportEnabledForPlugin() const;


    /**
     * @brief Used in the implementation of EffectInstance::onMetadataChanged_recursive so we know if the metadata changed or not.
     * Can only be called on the main thread.
     **/
    U64 getLastTimeInvariantMetadataHash() const;
    void setLastTimeInvariantMetadataHash(U64 lastTimeInvariantMetadataHashRefreshed);

private:

    int getPreferredInputInternal(bool connected) const;

public:


    /**
     * @brief Each input label is mapped against the script-name of the input
     * node, or an empty string if disconnected. Masks are placed in a separate map;
     **/
    void getInputNames(std::map<std::string, std::string> & inputNames,
                       std::map<std::string, std::string> & maskNames) const;


    enum CanConnectInputReturnValue
    {
        eCanConnectInput_ok = 0,
        eCanConnectInput_indexOutOfRange,
        eCanConnectInput_inputAlreadyConnected,
        eCanConnectInput_givenNodeNotConnectable,
        eCanConnectInput_groupHasNoOutput,
        eCanConnectInput_graphCycles,
        eCanConnectInput_differentPars,
        eCanConnectInput_differentFPS,
        eCanConnectInput_multiResNotSupported,
    };

    /**
     * @brief Returns true if a connection is possible for the given input number of the current node
     * to the given input.
     **/
    Node::CanConnectInputReturnValue canConnectInput(const NodePtr& input, int inputNumber) const;


    /** @brief Adds the node parent to the input inputNumber of the
     * node. Returns true if it succeeded, false otherwise.
     * When returning false, this means an input was already
     * connected for this inputNumber. It should be removed
     * beforehand.
     */
    virtual bool connectInput(const NodePtr& input, int inputNumber);

    bool connectInputBase(const NodePtr& input,
                          int inputNumber)
    {
        return Node::connectInput(input, inputNumber);
    }

    /** @brief Removes the node connected to the input inputNumber of the
     * node. Returns true upon sucess, false otherwise.
     */
    bool disconnectInput(int inputNumber);

    /** @brief Removes the node input of the
     * node inputs. Returns true upon success, false otherwise.
     */
    bool disconnectInput(const NodePtr& input);

    /**
     * @brief Atomically swaps the given inputNumber to the given input node.
     * If the input is NULL, the input is disconnected. This is the same as calling
     * atomically disconnectInput() then connectInput().
     **/
    bool swapInput(const NodePtr& input, int inputNumber);


    /**
     * @brief Tries to automatically connect the given node n according to the
     * current selection. If there are no selected nodes or multiple selected nodes
     * this method should not be called.
     * @returns True if auto-connecting worked, false otherwise
     **/
    bool autoConnect(const NodePtr& selected);

    /**
     * @brief Given the rectangle r, move the node down so it doesn't belong
     * to this rectangle and call the same function with the new bounding box of this node
     * recursively on its outputs.
     **/
    void moveBelowPositionRecursively(const RectD & r);

    /**
     * @brief Given the rectangle r, move the node up so it doesn't belong
     * to this rectangle and call the same function with the new bounding box of this node
     * recursively on its inputs.
     **/
    void moveAbovePositionRecursively(const RectD & r);
    



    void setNodeGuiPointer(const NodeGuiIPtr& gui);

    NodeGuiIPtr getNodeGui() const;

    bool isSettingsPanelVisible() const;

    bool isSettingsPanelMinimized() const;


private:

    bool replaceInputInternal(const NodePtr& input, int inputNumber, bool failIfExisting);


    bool isSettingsPanelVisibleInternal(std::set<NodeConstPtr>& recursionList) const;

public:

    bool isUserSelected() const;


    /**
     * @brief If the session is a GUI session, then this function sets the position of the node on the nodegraph.
     **/
    void setPosition(double x, double y);
    void getPosition(double *x, double *y) const;
    void movePosition(double dx, double dy);
    void onNodeUIPositionChanged(double x, double y);

    void setSize(double w, double h);
    void getSize(double* w, double* h) const;
    void onNodeUISizeChanged(double x, double y);

    RectD getNodeBoundingRect() const;

    /**
     * @brief Get the colour of the node as it appears on the nodegraph.
     **/
    bool getColor(double* r, double *g, double* b) const;
    void setColor(double r, double g, double b);
    void onNodeUIColorChanged(double r, double g, double b);
    bool hasColorChangedSinceDefault() const;
    void getDefaultColor(double* r, double *g, double* b) const;

    void setOverlayColor(double r, double g, double b);
    bool getOverlayColor(double* r, double* g, double* b) const;
    void onNodeUIOverlayColorChanged(double r, double g, double b);

    void onNodeUISelectionChanged(bool isSelected);
    bool getNodeIsSelected() const;

    void setDraftEnabledForOverlayActions(bool enabled);
    bool isDraftEnabledForOverlayActions() const;


    void runChangedParamCallback(const KnobIPtr& k, bool userEdited);



protected:

    void runInputChangedCallback(int index);

private:

    /**
     * @brief Adds an output to this node.
     **/
    void connectOutput(const NodePtr& output, int outputInputIndex);

    /** @brief Removes the node output of the
     * node outputs. Returns the outputNumber if it could remove it,
       otherwise returns -1.*/
    bool disconnectOutput(const NodePtr& output, int outputInputIndex);

public:

    /**
     * @brief Switches the 2 first inputs that are not a mask, if and only if they have compatible components/bitdepths
     **/
    void switchInput0And1();

    /**
     * @brief Redirects all outputs of this node to the main input (if connected);
     **/
    void connectOutputsToMainInput();

    /**
     * @brief Forwarded to the live effect instance
     **/
    std::string getPluginID() const;


    /**
     * @brief Forwarded to the live effect instance
     **/
    std::string getPluginLabel() const;

    /**
     * @brief Returns the path to where the resources are stored for this plug-in
     **/
    std::string getPluginResourcesPath() const;

    void getPluginGrouping(std::vector<std::string>* grouping) const;

    /**
     * @brief Forwarded to the live effect instance
     **/
    std::string getPluginDescription() const;

    /**
     * @brief Returns the absolute file-path to the plug-in icon.
     **/
    std::string getPluginIconFilePath() const;

    /**
     * @brief Returns true if this node is a PyPlug
     **/
    bool isPyPlug() const;


    AppInstancePtr getApp() const;

    /**
     * @brief Returns true if destroyNode() was called
     **/
    bool isBeingDestroyed() const;

    /**
     * @brief Removes the node from the project and destroy any data structure associated with it.
     * The object will be destroyed when the caller releases the reference to this Node
     **/
    void destroyNode();

private:
    
    
    void invalidateExpressionLinks();

public:


    /**
     * @brief Forwarded to the live effect instance
     **/
    KnobIPtr getKnobByName(const std::string & name) const;


    bool makePreviewByDefault() const;

    void togglePreview();

    bool isPreviewEnabled() const;

    /**
     * @brief Makes a small 8-bit preview image of size width x height with a ARGB32 format.
     * Pre-condition:
     *  - buf has been allocated for the correct amount of memory needed to fill the buffer.
     * Post-condition:
     *  - buf must not be freed or overflown.
     * It will serve as a preview on the node's graphical user interface.
     * This function is called directly by the PreviewThread.
     *
     * In order to notify the GUI that you want to refresh the preview, just
     * call refreshPreviewImage(time).
     **/
    bool makePreviewImage(TimeValue time, int width, int height, unsigned int* buf);

    /**
     * @brief Returns true if the node is currently rendering a preview image.
     **/
    bool isRenderingPreview() const;


    /**
     * @brief Use this function to post a transient message to the user. It will be displayed using
     * a dialog. The message can be of 4 types...
     * INFORMATION_MESSAGE : you just want to inform the user about something.
     * eMessageTypeWarning : you want to inform the user that something important happened.
     * eMessageTypeError : you want to inform the user an error occured.
     * eMessageTypeQuestion : you want to ask the user about something.
     * The function will return true always except for a message of type eMessageTypeQuestion, in which
     * case the function may return false if the user pressed the 'No' button.
     * @param content The message you want to pass.
     **/
    bool message(MessageTypeEnum type, const std::string & content) const;

    /**
     * @brief Use this function to post a persistent message to the user. It will be displayed on the
     * node's graphical interface and on any connected viewer. The message can be of 3 types...
     * INFORMATION_MESSAGE : you just want to inform the user about something.
     * eMessageTypeWarning : you want to inform the user that something important happened.
     * eMessageTypeError : you want to inform the user an error occured.
     * @param content The message you want to pass.
     **/
    void setPersistentMessage(MessageTypeEnum type, const std::string& messageID, const std::string & content);

    /**
     * @brief Clears any message posted previously by setPersistentMessage.
     * This function will also be called on all inputs
     **/
    void clearAllPersistentMessages(bool recurse);

    void clearPersistentMessage(const std::string& key);

private:

    void clearAllPersistentMessageRecursive(std::list<NodePtr>& markedNodes);

    void clearAllPersistentMessageInternal();

public:


    bool notifyInputNIsRendering(int inputNb);

    void notifyInputNIsFinishedRendering(int inputNb);

    bool notifyRenderingStarted();

    void notifyRenderingEnded();

    int getIsInputNRenderingCounter(int inputNb) const;

    int getIsNodeRenderingCounter() const;

    void refreshPreviewsRecursivelyDownstream();

    void refreshPreviewsRecursivelyUpstream();

public:

    static void choiceParamAddLayerCallback(const KnobChoicePtr& knob);

    //When creating a Reader or Writer node, this is a pointer to the "bundle" node that the user actually see.
    NodePtr getIOContainer() const;


    void beginInputEdition();

    void endInputEdition(bool triggerRender);

    void onInputChanged(int inputNb);


public:

    /**
     * @brief Returns whether this node or one of its inputs (recursively) is marked as
     * eSequentialPreferenceOnlySequential
     *
     * @param nodeName If the return value is true, this will be set to the name of the node
     * which is sequential.
     *
     **/
    bool hasSequentialOnlyNodeUpstream(std::string & nodeName) const;

    /**
     * @brief Returns true if an effect should be able to connect this node.
     **/
    bool canOthersConnectToThisNode() const;

    bool hasPersistentMessage(const std::string& key) const;

    bool hasAnyPersistentMessage() const;

    void getPersistentMessage(PersistentMessageMap* messages, bool prefixLabelAndType = true) const;


    /**
     * @brief Attempts to detect cycles considering input being an input of this node.
     * Returns true if it couldn't detect any cycle, false otherwise.
     **/
    bool checkIfConnectingInputIsOk(const NodePtr& input) const;

 

    /**
     * @brief Declares to Python all parameters as attribute of the variable representing this node.
     **/
    void declarePythonKnobs();

private:


    /**
     * @brief Declares to Python all parameters, roto, tracking attributes
     * This is called in activate() whenever the node was deleted
     **/
    void declareAllPythonAttributes();

public:
    /**
     * @brief Set the node name.
     * Throws a run-time error with the message in case of error
     **/
    void setScriptName(const std::string & name);

    void setScriptName_no_error_check(const std::string & name);


    /**
     * @brief The node unique name.
     **/
    const std::string & getScriptName() const;
    std::string getScriptName_mt_safe() const;

    /**
       @brief Returns the name of the node, prepended by the name of all the group containing it, e.g:
     * - a node in the "root" project would be: Blur1
     * - a node within the group 1 of the project would be : <g>group1</g>Blur1
     * - a node within the group 1 of the group 1 of the project would be : <g>group1</g><g>group1</g>Blur1
     **/
    std::string getFullyQualifiedName() const;

    std::string getContainerGroupFullyQualifiedName() const;

    void setLabel(const std::string& label);

    const std::string& getLabel() const;
    std::string getLabel_mt_safe() const;
   
    
    void runAfterTableItemsSelectionChangedCallback(const KnobItemsTablePtr& table, const std::list<KnobTableItemPtr>& deselected, const std::list<KnobTableItemPtr>& selected, TableChangeReasonEnum reason);


    static void getOriginalFrameRangeForReader(const std::string& pluginID, const std::string& canonicalFileName, int* firstFrame, int* lastFrame);



    bool canHandleRenderScaleForOverlays() const;


    /**
     * @brief Set the cursor to be one of the default cursor.
     * @returns True if it successfully set the cursor, false otherwise.
     * Note: this can only be called during an overlay interact action.
     **/
    void setCurrentCursor(CursorEnum defaultCursor);
    bool setCurrentCursor(const QString& customCursorFilePath);

private:

    friend class DuringInteractActionSetter_RAII;
    void setDuringInteractAction(bool b);

public:


    bool isDoingInteractAction() const;

    bool shouldDrawOverlay(TimeValue time, ViewIdx view, OverlayViewportTypeEnum type) const;

    bool isSubGraphEditedByUser() const;

    void removeParameterFromPython(const std::string& parameterName);

public:



    const std::vector<std::string>& getCreatedViews() const;

    void refreshCreatedViews(bool silent);

    void refreshIdentityState();


    QString makeDocumentation(bool genHTML) const;

    void refreshPreviewsAfterProjectLoad();

    enum StreamWarningEnum
    {
        //A bitdepth conversion occurs and converts to a lower bitdepth the stream,
        //inducing a quality loss
        eStreamWarningBitdepth,

        //The node has inputs with different aspect ratios but does not handle it
        eStreamWarningPixelAspectRatio,

        //The node has inputs with different frame rates which may produce unwanted results
        eStreamWarningFrameRate,
    };

    void setStreamWarning(StreamWarningEnum warning, const QString& message);
    void setStreamWarnings(const std::map<StreamWarningEnum, QString>& warnings);
    void getStreamWarnings(std::map<StreamWarningEnum, QString>* warnings) const;


    bool isNodeUpstream(const NodeConstPtr& input) const;


    void refreshCreatedViews(const KnobIPtr& knob, bool silent);

private:




    /**
     * @brief If the node is an input of this node, set ok to true, otherwise
     * calls this function recursively on all inputs.
     **/
    bool isNodeUpstreamInternal(const NodeConstPtr& input, std::set<NodeConstPtr>& markedNodes) const;

    bool setStreamWarningInternal(StreamWarningEnum warning, const QString& message);


    void setNameInternal(const std::string& name, bool throwErrors);

    std::string getFullyQualifiedNameInternal(const std::string& scriptName) const;


public Q_SLOTS:


    void onRefreshIdentityStateRequestReceived();


    void doRefreshEdgesGUI()
    {
        Q_EMIT refreshEdgesGUI();
    }

    /*will force a preview re-computation not matter of the project's preview mode*/
    void computePreviewImage()
    {
        Q_EMIT previewRefreshRequested();
    }

    /*will refresh the preview only if the project is in auto-preview mode*/
    void refreshPreviewImage()
    {
        Q_EMIT previewImageChanged();
    }

    void onInputLabelChanged(const QString& oldName, const QString& newName);

    void notifySettingsPanelClosed(bool closed )
    {
        Q_EMIT settingsPanelClosed(closed);
    }

    void s_nodeExtraLabelChanged()
    {
        Q_EMIT nodeExtraLabelChanged();
    }

    void s_previewKnobToggled()
    {
        Q_EMIT previewKnobToggled();
    }

    void s_disabledKnobToggled(bool disabled)
    {
        Q_EMIT disabledKnobToggled(disabled);
    }
    void s_hideInputsKnobChanged(bool hidden)
    {
        Q_EMIT hideInputsKnobChanged(hidden);
    }

    void s_keepInAnimationModuleKnobChanged()
    {
        Q_EMIT keepInAnimationModuleKnobChanged();
    }

    void s_enabledChannelCheckboxChanged()
    {
        Q_EMIT enabledChannelCheckboxChanged();
    }

    void s_outputLayerChanged() { Q_EMIT outputLayerChanged(); }

    void s_mustRefreshPluginInfo() { Q_EMIT mustRefreshPluginInfo(); }
    
Q_SIGNALS:

    void keepInAnimationModuleKnobChanged();

    void rightClickMenuKnobPopulated();

    void s_refreshPreviewsAfterProjectLoadRequested();

    void hideInputsKnobChanged(bool hidden);

    void refreshIdentityStateRequested();

    void availableViewsChanged();

    void outputLayerChanged();

    void settingsPanelClosed(bool);

    void persistentMessageChanged();

    void inputsDescriptionChanged();

    void inputLabelChanged(int, QString);

    /*
     * @brief Emitted whenever an input changed on the GUI.
     */
    void inputChanged(int);

    void outputsChanged();

    void canUndoChanged(bool);

    void canRedoChanged(bool);

    void labelChanged(QString,QString);
    void scriptNameChanged(QString);
    void inputEdgeLabelChanged(int, QString);

    void inputVisibilityChanged(int);

    void refreshEdgesGUI();

    void previewImageChanged();

    void previewRefreshRequested();

    void inputNIsRendering(int inputNb);

    void inputNIsFinishedRendering(int inputNb);

    void renderingStarted();

    void renderingEnded();

    ///how much has just changed, this not the new value but the difference between the new value
    ///and the old value
    void pluginMemoryUsageChanged(qint64 mem);

    void knobSlaved();

    void previewKnobToggled();

    void disabledKnobToggled(bool disabled);

    void streamWarningsChanged();
    void nodeExtraLabelChanged();

    void nodePresetsChanged();

    void mustRefreshPluginInfo();

    void enabledChannelCheckboxChanged();


private:


    void declareTablePythonFields();




    void declareNodeVariableToPython(const std::string& nodeName);
    void setNodeVariableToPython(const std::string& oldName, const std::string& newName);
    void deleteNodeVariableToPython(const std::string& nodeName);

    friend struct NodePrivate;
    boost::scoped_ptr<NodePrivate> _imp;
};


class DuringInteractActionSetter_RAII
{
    NodePtr _node;
public:

    DuringInteractActionSetter_RAII(const NodePtr& node)
    : _node(node)
    {
        node->setDuringInteractAction(true);
    }

    ~DuringInteractActionSetter_RAII()
    {
        _node->setDuringInteractAction(false);
    }
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_NODE_H
