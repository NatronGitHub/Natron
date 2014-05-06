//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef NATRON_ENGINE_NODE_H_
#define NATRON_ENGINE_NODE_H_

#include <vector>
#include <string>
#include <map>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QMetaType>
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
#include <QMutex>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include "Engine/AppManager.h"
#include "Global/KeySymbols.h"

class AppInstance;
class NodeSettingsPanel;
class KnobI;
class ViewerInstance;
class RenderTree;
class Format;
class NodeSerialization;
class KnobHolder;
class RotoContext;
namespace Natron{

class Image;
class EffectInstance;
class LibraryBinary;
class ChannelSet;

class Node : public QObject
{
    Q_OBJECT
    
public:

    Node(AppInstance* app,Natron::LibraryBinary* plugin);
    
    virtual ~Node();
    
    void load(const std::string& pluginID,const boost::shared_ptr<Natron::Node>& thisShared,
              const NodeSerialization& serialization,bool dontLoadName);
    
    ///called by load() and OfxEffectInstance, do not call this!
    void loadKnobs(const NodeSerialization& serialization);
    
    ///called by Project::removeNode, never call this
    void removeReferences();
    
    ///function called by EffectInstance to create a knob
    template <class K>
    boost::shared_ptr<K> createKnob(const std::string &description, int dimension = 1){
        return appPTR->getKnobFactory().createKnob<K>(getLiveInstance(),description,dimension);
    }
    
    ///This cannot be done in loadKnobs as to call this all the nodes in the project must have
    ///been loaded first.
    void restoreKnobsLinks(const NodeSerialization& serialization,const std::vector<boost::shared_ptr<Natron::Node> >& allNodes);
    
    /*Quit all processing done by all render instances of this node */
    void quitAnyProcessing();
    
    /*Never call this yourself. This is needed by OfxEffectInstance so the pointer to the live instance
     *is set earlier.
     */
    void setLiveInstance(Natron::EffectInstance* liveInstance);
    
    Natron::EffectInstance* getLiveInstance() const;
    
    /**
     * @brief Returns the hash value of the node, or 0 if it has never been computed.
     **/
    U64 getHashValue() const;
    
    /**
     * @brief Forwarded to the live effect instance
     **/
    void initializeKnobs();
    
    /**
     * @brief Forwarded to the live effect instance
     **/
    void beginEditKnobs();
    
    /**
     * @brief Forwarded to the live effect instance
     **/
    const std::vector< boost::shared_ptr<KnobI> >& getKnobs() const;

    /*Returns in viewers the list of all the viewers connected to this node*/
    void hasViewersConnected(std::list<ViewerInstance* >* viewers) const;
    
    /**
     * @brief Forwarded to the live effect instance
     **/
    int majorVersion() const;
    
    /**
     * @brief Forwarded to the live effect instance
     **/
    int minorVersion() const;

    
    /**
     * @brief Forwarded to the live effect instance
     **/
    bool isInputNode() const;
    
    /**
     * @brief Forwarded to the live effect instance
     **/
    bool isOutputNode() const;
    
    
    /**
     * @brief Forwarded to the live effect instance
     **/
    bool isInputAndProcessingNode() const ;
    
    /**
     * @brief Forwarded to the live effect instance
     **/
    bool isOpenFXNode() const;
    
    /**
     * @brief Returns true if the node is either a roto or rotopaint node
     **/
    bool isRotoNode() const;
    
    /**
     * @brief Returns true if the node is a rotopaint node
     **/
    bool isRotoPaintingNode() const;
    
    /**
     * @brief Returns a pointer to the rotoscoping context if the node is in the paint context, otherwise NULL.
     **/
    boost::shared_ptr<RotoContext> getRotoContext() const;
    
    /**
     * @brief Forwarded to the live effect instance
     **/
    virtual int maximumInputs() const ;

    /**
     * @brief Returns true if the given input supports the given components. If inputNb equals -1
     * then this function will check whether the effect can produce the given components.
     **/
    bool isSupportedComponent(int inputNb,Natron::ImageComponents comp) const;
    
    /**
     * @brief Returns the most appropriate components that can be supported by the inputNb.
     * If inputNb equals -1 then this function will check the output components.
     **/
    Natron::ImageComponents findClosestSupportedComponents(int inputNb,Natron::ImageComponents comp) const;
    
    /**
     * @brief Returns the index of the channel to use to produce the mask.
     * None = -1
     * R = 0
     * G = 1
     * B = 2
     * A = 3
     **/
    int getMaskChannel() const;
    
    /**
     * @brief Returns whether masking is enabled or not
     **/
    bool isMaskEnabled() const;
    
    /**
     * @brief Returns true if the mask should be used inverted
     **/
    bool isMaskInverted() const;
    
    /**
     * @brief Returns a pointer to the input Node at index 'index'
     * or NULL if it couldn't find such node.
     **/
    boost::shared_ptr<Node> input(int index) const;
   
    
    /**
     * @brief Same as Node::input(int) but can be called by another thread than the main thread.
     * Basically it returns a consistent value throughout the rendering of a frame.
     * The inputs are then updated before the rendering of any frame.
     **/
    boost::shared_ptr<Node> input_other_thread(int index) const;
    
    /**
     * @brief Look-ups the changes made to the inputs that are queued, and if any it updates
     * the inputs of the node in a thread-safe manner.
     **/
    void updateRenderInputs();
    
    /**
     *@brief Returns the inputs of the node as the Gui just set them.
     *The vector might be different from what getInputs_other_thread() could return.
     *This can only be called by the main thread.
     **/
    const std::vector<boost::shared_ptr<Natron::Node> >& getInputs_mt_safe() const WARN_UNUSED_RETURN;
    
    /**
     *@brief Returns the inputs of the node as the render thread see them.
     * They are modified only by the render thread when updateRenderInputs() is called.
     **/
    const std::vector<boost::shared_ptr<Natron::Node> >& getInputs_other_thread() const WARN_UNUSED_RETURN;
    
    
    
    /**
     * @brief Returns the input index of the node n if it exists,
     * -1 otherwise.
     **/
    int inputIndex(Node* n) const;
        
    const std::vector<std::string>& getInputLabels() const;
 
    
    std::string getInputLabel(int inputNb) const;
    
    bool isInputConnected(int inputNb) const;
    
    bool hasOutputConnected() const;
    
    /**
     * @brief This is used by the auto-connection algorithm.
     * When connecting nodes together this function helps determine
     * on which input it should connect a new node.
     * It returns the first non optional empty input or the first optional
     * empty input if they are all optionals, or -1 if nothing matches the 2 first conditions..
     * if all inputs are connected.
     **/
    int getPreferredInputForConnection() const;
    
    /**
     * @brief Returns in 'outputs' a map of all nodes connected to this node
     * where the value of the map is the input index from which these outputs
     * are connected to this node.
     **/
    void getOutputsConnectedToThisNode(std::map< boost::shared_ptr<Node>,int>* outputs);
        
    const std::list<boost::shared_ptr<Natron::Node> >& getOutputs() const;
    
    /**
     * @brief Each input name is appended to the vector, in the same order
     * as they are in the internal inputs vector. Disconnected inputs are
     * represented as empty strings.
     **/
    std::vector<std::string> getInputNames() const;
    
    /** @brief Adds the node parent to the input inputNumber of the
     * node. Returns true if it succeeded, false otherwise.
     * When returning false, this means an input was already
     * connected for this inputNumber. It should be removed
     * beforehand.
     */
    virtual bool connectInput(boost::shared_ptr<Node> input,int inputNumber);
    
    /** @brief Removes the node connected to the input inputNumber of the
     * node. Returns the inputNumber if it could remove it, otherwise returns
     -1.
     */
    virtual int disconnectInput(int inputNumber);
    
    /** @brief Removes the node input of the
     * node inputs. Returns the inputNumber if it could remove it, otherwise returns
     -1.*/
    virtual int disconnectInput(boost::shared_ptr<Node> input);
    
    /**
     * @brief Adds an output to this node.
     **/
    void connectOutput(boost::shared_ptr<Node> output);
    
    /** @brief Removes the node output of the
     * node outputs. Returns the outputNumber if it could remove it,
     otherwise returns -1.*/
    int disconnectOutput(boost::shared_ptr<Node> output);
    

    /*============================*/
    
    /**
     * @brief The node unique name.
     **/
    const std::string& getName() const;
    
    std::string getName_mt_safe() const;

    void setName(const std::string& name);

    /**
     * @brief Forwarded to the live effect instance
     **/
    std::string pluginID() const;
    
    /**
     * @brief Forwarded to the live effect instance
     **/
    std::string pluginLabel() const;

    /**
     * @brief Forwarded to the live effect instance
     **/
    std::string description() const;
    
    /*============================*/

    
    AppInstance* getApp() const;
    
    /*Make this node inactive. It will appear
     as if it was removed from the graph editor
     but the object still lives to allow
     undo/redo operations.*/
    void deactivate();
    
    /*Make this node active. It will appear
     again on the node graph.
     WARNING: this function can only be called
     after a call to deactivate() has been made.
     Calling activate() on a node whose already
     been activated will not do anything.
     */
    void activate();
    
    /**
     * @brief Forwarded to the live effect instance
     **/
    boost::shared_ptr<KnobI> getKnobByName(const std::string& name) const;
    
    /*@brief The derived class should query this to abort any long process
     in the engine function.*/
    bool aborted() const ;
    
    /**
     * @brief Called externally when the rendering is aborted. You should never
     * call this yourself.
     **/
    void setAborted(bool b);

    bool makePreviewByDefault() const;
    
    void togglePreview();
    
    bool isPreviewEnabled() const;
    
    /**
     * @brief Makes a small 8bits preview image of size width x height of format ARGB32.
     * Pre-condition:
     *  - buf has been allocated for the correct amount of memory needed to fill the buffer.
     * Post-condition:
     *  - buf must not be freed or overflown.
     * It will serve as a preview on the node's graphical user interface.
     * This function is called directly by the GUI to display the preview.
     * In order to notify the GUI that you want to refresh the preview, just
     * call refreshPreviewImage(time).
     **/
    void makePreviewImage(SequenceTime time,int width,int height,unsigned int* buf);
    
    /**
     * @brief Returns true if the node is currently rendering a preview image.
     **/
    bool isRenderingPreview() const;
    
    /**
     * @brief
     * You must call this in order to notify the GUI of any change (add/delete) for knobs
     **/
    void createKnobDynamically();
    
    /**
     * @brief Returns true if the node is active for use in the graph editor. MT-safe
     **/
    bool isActivated() const;

    
    boost::shared_ptr<Natron::Image> getImageBeingRendered(SequenceTime time,int view) const;
    
    void addImageBeingRendered(boost::shared_ptr<Natron::Image>,SequenceTime time,int view );
    
    void removeImageBeingRendered(SequenceTime time,int view );
        
    /**
     * @brief Use this function to post a transient message to the user. It will be displayed using
     * a dialog. The message can be of 4 types...
     * INFORMATION_MESSAGE : you just want to inform the user about something.
     * WARNING_MESSAGE : you want to inform the user that something important happened.
     * ERROR_MESSAGE : you want to inform the user an error occured.
     * QUESTION_MESSAGE : you want to ask the user about something.
     * The function will return true always except for a message of type QUESTION_MESSAGE, in which
     * case the function may return false if the user pressed the 'No' button.
     * @param content The message you want to pass.
     **/
    bool message(Natron::MessageType type,const std::string& content) const;

    /**
     * @brief Use this function to post a persistent message to the user. It will be displayed on the
     * node's graphical interface and on any connected viewer. The message can be of 3 types...
     * INFORMATION_MESSAGE : you just want to inform the user about something.
     * WARNING_MESSAGE : you want to inform the user that something important happened.
     * ERROR_MESSAGE : you want to inform the user an error occured.
     * @param content The message you want to pass.
     **/
    void setPersistentMessage(Natron::MessageType type,const std::string& content);
    
    /**
     * @brief Clears any message posted previously by setPersistentMessage.
     **/
    void clearPersistentMessage();
    
    void purgeAllInstancesCaches();
    
    void notifyInputNIsRendering(int inputNb) ;
    
    void notifyInputNIsFinishedRendering(int inputNb);
    
    void notifyRenderingStarted();
    
    void notifyRenderingEnded();
    
    
    /**
     * @brief forwarded to the live instance
     **/
    void setInputFilesForReader(const std::vector<std::string>& files);
    
    /**
     * @brief forwarded to the live instance
     **/
    void setOutputFilesForWriter(const std::string& pattern);
    
    
    ///called by EffectInstance
    void registerPluginMemory(size_t nBytes);
    
    ///called by EffectInstance
    void unregisterPluginMemory(size_t nBytes);

    //see INSTANCE_SAFE in EffectInstance::renderRoI
    //only 1 clone can render at any time
    QMutex& getRenderInstancesSharedMutex();
    
    ///see FULLY_SAFE in EffectInstance::renderRoI
    QMutex& getFrameMutex(int time);
    
    void refreshPreviewsRecursively();
    
    
    void incrementKnobsAge();
    
    U64 getKnobsAge() const;
    
    void onSlaveStateChanged(bool isSlave,KnobHolder* master);
  
    boost::shared_ptr<Natron::Node> getMasterNode() const;
    
public slots:
    
    void setKnobsAge(U64 newAge) ;
    
    void onGUINameChanged(const QString& str);

    void doRefreshEdgesGUI(){
        emit refreshEdgesGUI();
    }
    
    /*will force a preview re-computation not matter of the project's preview mode*/
    void computePreviewImage(int time) {
        emit previewRefreshRequested(time);
    }
    
    /*will refresh the preview only if the project is in auto-preview mode*/
    void refreshPreviewImage(int time) {
        emit previewImageChanged(time);
    }
    
    void notifyGuiChannelChanged(const Natron::ChannelSet& c) {
        emit channelsChanged(c);
    }
    
    void onMasterNodeDeactivated();

    
signals:
    
    void knobsAgeChanged(U64 age);
    
    void persistentMessageChanged(int,QString);
    
    void persistentMessageCleared();
    
    void inputsInitialized();
    
    void knobsInitialized();
    
    void inputChanged(int);
    
    void outputsChanged();

    void activated();
    
    void deactivated();
    
    void canUndoChanged(bool);
    
    void canRedoChanged(bool);
    
    void nameChanged(QString);
    
    void refreshEdgesGUI();

    void previewImageChanged(int);
    
    void previewRefreshRequested(int);
    
    void channelsChanged(const Natron::ChannelSet&);

    void inputNIsRendering(int inputNb);
    
    void inputNIsFinishedRendering(int inputNb);
    
    void renderingStarted();
    
    void renderingEnded();
    
    ///how much has just changed, this not the new value but the difference between the new value
    ///and the old value
    void pluginMemoryUsageChanged(qint64 mem);
    
    void slavedStateChanged(bool b);
    
    
protected:

    /**
     * @brief Attempts to detect cycles considering input being an input of this node.
     * Returns true if it couldn't detect any cycle, false otherwise.
     **/
    bool checkIfConnectingInputIsOk(Natron::Node* input) const;
    
    /**
     * @brief Recompute the hash value of this node and notify all the clone effects that the values they store in their
     * knobs is dirty and that they should refresh it by cloning the live instance.
     **/
    void computeHash();
    
    /*Initialises inputs*/
    void initializeInputs();

private:
    /**
     * @brief If the node is an input of this node, set ok to true, otherwise
     * calls this function recursively on all inputs.
     **/
    void isNodeUpstream(const Natron::Node* input,bool* ok) const;
    
    struct Implementation;
    boost::scoped_ptr<Implementation> _imp;
};

} //namespace Natron

/**
 * @brief An InspectorNode is a type of node that is able to have a dynamic number of inputs.
 * Only 1 input is considered to be the "active" input of the InspectorNode, but several inputs
 * can be connected. This Node is suitable for effects that take only 1 input in parameter.
 * This is used for example by the Viewer, to be able to switch quickly from several inputs
 * while still having 1 input active.
 **/
class InspectorNode: public Natron::Node
{
    int _inputsCount;
    int _activeInput;
    mutable QMutex _activeInputMutex;
    
public:
    
    InspectorNode(AppInstance* app,Natron::LibraryBinary* plugin);
    
    virtual ~InspectorNode();
    
    virtual int maximumInputs() const OVERRIDE { return _inputsCount; }
    
    virtual bool connectInput(boost::shared_ptr<Node> input,int inputNumber) OVERRIDE;
    
    virtual int disconnectInput(int inputNumber) OVERRIDE;
    
    virtual int disconnectInput(boost::shared_ptr<Node> input) OVERRIDE;
    
    bool tryAddEmptyInput();
    
    void addEmptyInput();
    
    void removeEmptyInputs();
    
    void setActiveInputAndRefresh(int inputNb);
    
    int activeInput() const { QMutexLocker l(&_activeInputMutex); return _activeInput; }
};

#endif // NATRON_ENGINE_NODE_H_
