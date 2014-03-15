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
CLANG_DIAG_ON(deprecated)
#include <QObject>
#include <QMutex>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"
#include "Global/KeySymbols.h"

class AppInstance;
class NodeSettingsPanel;
class Knob;
class ViewerInstance;
class RenderTree;
class Format;
class NodeSerialization;
namespace Natron{

class Row;
class Image;
class EffectInstance;
class LibraryBinary;
class ChannelSet;

class Node : public QObject
{
    Q_OBJECT
    
public:

    typedef std::map<int,Node*> InputMap;
    typedef std::multimap<int,Node*> OutputMap;

    Node(AppInstance* app,Natron::LibraryBinary* plugin);
    
    virtual ~Node();
    
    void load(const std::string& pluginID,const NodeSerialization& serialization);
    
    ///called by load() and OfxEffectInstance, do not call this!
    void loadKnobs(const NodeSerialization& serialization);
    
    /*Quit all processing done by all render instances of this node */
    void quitAnyProcessing();
    
    /*Never call this yourself.*/
    void setLiveInstance(Natron::EffectInstance* liveInstance) {_liveInstance = liveInstance;}
    
    Natron::EffectInstance* getLiveInstance() const {return _liveInstance;}
    
    /**
     * @brief Find or creates a clone of the live instance (i.e all the knobs etc...) for the given tree.
     * The effect instance returns is guaranteed to reflect exactly the state of the live instance at the
     * time at which it has been called.
     **/
    Natron::EffectInstance* findOrCreateLiveInstanceClone(RenderTree* tree);
    
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
    const std::vector< boost::shared_ptr<Knob> >& getKnobs() const;

    /*Returns in viewers the list of all the viewers connected to this node*/
    void hasViewersConnected(std::list<ViewerInstance*>* viewers) const;
    
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
    
    /*Initialises inputs*/
    void initializeInputs();
    
    /**
     * @brief Forwarded to the live effect instance
     **/
    virtual int maximumInputs() const ;

    /**
     * @brief Returns a pointer to the input Node at index 'index'
     * or NULL if it couldn't find such node.
     **/
    Node* input(int index) const;
    
    /**
     * @brief Returns the input index of the node n if it exists,
     * -1 otherwise.
     **/
    int inputIndex(Node* n) const;
    
    void outputs(std::vector<Natron::Node*>* outputsV) const;
    
    const std::map<int, std::string>& getInputLabels() const;
    
    /**
     * @brief Called by RenderTree: This external locking allows the render tree not to be corrupted
     * by another user action. The architecture doesn't allow internal locking.
     **/
    void setRenderTreeIsUsingInputs(bool b);
    
    std::string getInputLabel(int inputNb) const;
    
    bool isInputConnected(int inputNb) const;
    
    bool hasOutputConnected() const;
    
    /**
     * @brief This is used by the auto-connection algorithm.
     * When connecting nodes together this function helps determine
     * on which input it should connect a new node.
     **/
    int getPreferredInputForConnection() const;
    
    /**
     * @brief Returns in 'outputs' a map of all nodes connected to this node
     * where the value of the map is the input index from which these outputs
     * are connected to this node.
     **/
    void getOutputsConnectedToThisNode(std::map<Node*,int>* outputs);
    
    const InputMap& getInputs() const {return _inputs;}
    
    const OutputMap& getOutputs() const;
    
    /** @brief Adds the node parent to the input inputNumber of the
     * node. Returns true if it succeeded, false otherwise.
     * When returning false, this means an input was already
     * connected for this inputNumber. It should be removed
     * beforehand.
     */
    virtual bool connectInput(Node* input,int inputNumber,bool autoConnection = false);
    
    /** @brief Removes the node connected to the input inputNumber of the
     * node. Returns the inputNumber if it could remove it, otherwise returns
     -1.
     */
    virtual int disconnectInput(int inputNumber);
    
    /** @brief Removes the node input of the
     * node inputs. Returns the inputNumber if it could remove it, otherwise returns
     -1.*/
    virtual int disconnectInput(Node* input);
    
    /** @brief Adds the node child to the output outputNumber of the
     * node.
     */
    void connectOutput(Node* output,int outputNumber = 0);
    
    /** @brief Removes the node output of the
     * node outputs. Returns the outputNumber if it could remove it,
     otherwise returns -1.*/
    int disconnectOutput(Node* output);
    

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
    
    void getRenderFormatForEffect(const Natron::EffectInstance* effect, Format *f) const;
    
    int getRenderViewsCountForEffect(const Natron::EffectInstance* effect) const;
    
    /**
     * @brief Forwarded to the live effect instance
     **/
    boost::shared_ptr<Knob> getKnobByDescription(const std::string& desc) const;
    
    /*@brief The derived class should query this to abort any long process
     in the engine function.*/
    bool aborted() const ;
    
    /**
     * @brief Called externally when the rendering is aborted. You should never
     * call this yourself.
     **/
    void setAborted(bool b);

    void drawOverlay();
    
    bool onOverlayPenDown(const QPointF& viewportPos,const QPointF& pos);
    
    bool onOverlayPenMotion(const QPointF& viewportPos,const QPointF& pos);
    
    bool onOverlayPenUp(const QPointF& viewportPos,const QPointF& pos);
    
    bool onOverlayKeyDown(Natron::Key key,Natron::KeyboardModifiers modifiers);
    
    bool onOverlayKeyUp(Natron::Key key,Natron::KeyboardModifiers modifiers);
    
    bool onOverlayKeyRepeat(Natron::Key key,Natron::KeyboardModifiers modifiers);
    
    bool onOverlayFocusGained();
    
    bool onOverlayFocusLost();

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
    
    void abortRenderingForEffect(Natron::EffectInstance* effect);

    Natron::EffectInstance* findExistingEffect(RenderTree* tree) const;
    
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

    /**
     * @brief Fills the serializationObject with the current state of the Node.
     **/
    void serialize(NodeSerialization* serializationObject);
    
    void purgeAllInstancesCaches();
    
    void notifyInputNIsRendering(int inputNb) ;
    
    void notifyInputNIsFinishedRendering(int inputNb);
    
    void notifyRenderingStarted();
    
    void notifyRenderingEnded();
    
    
    /**
     * @brief forwarded to the live instance
     **/
    void setInputFilesForReader(const QStringList& files);
    
    /**
     * @brief forwarded to the live instance
     **/
    void setOutputFilesForWriter(const QString& pattern);
    
    
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
    
    void setKnobsAge(U64 newAge) ;
    
    void incrementKnobsAge();
    
    U64 getKnobsAge() const;
    
public slots:
    
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
   
    
signals:
    
    void persistentMessageChanged(int,QString);
    
    void persistentMessageCleared();
    
    void inputsInitialized();
    
    void knobsInitialized();
    
    void inputChanged(int);

    void activated();
    
    void deactivated();
    
    void canUndoChanged(bool);
    
    void canRedoChanged(bool);
    
    void nameChanged(QString);
    
    void deleteWanted(Natron::Node*);

    void refreshEdgesGUI();

    void previewImageChanged(int);
    
    void previewRefreshRequested(int);
    
    void channelsChanged(const Natron::ChannelSet&);

    void inputNIsRendering(int inputNb);
    
    void inputNIsFinishedRendering(int inputNb);
    
    void renderingStarted();
    
    void renderingEnded();
    
    void pluginMemoryUsageChanged(unsigned long long);
    
protected:
    
    ///waits for any RenderTree to be done reading
    ///this function must be called explicitly while isUsingInputsMutex is locked!
    void waitForRenderTreesToBeDone();
    
    // FIXME: all data members should be private, use getter/setter instead
    mutable QMutex isUsingInputsMutex;
    std::map<int,Node*> _inputs;//only 1 input per slot
    Natron::EffectInstance*  _liveInstance; //< the instance of the effect interacting with the GUI of this node.

private:
    
    struct Implementation;
    boost::scoped_ptr<Implementation> _imp;

    /**
     * @brief Used internally by findOrCreateLiveInstanceClone
     **/
    Natron::EffectInstance*  createLiveInstanceClone();
};

} //namespace Natron

Q_DECLARE_METATYPE(Natron::Node*)
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
    
public:
    
    InspectorNode(AppInstance* app,Natron::LibraryBinary* plugin);
    
    virtual ~InspectorNode();
    
    virtual int maximumInputs() const OVERRIDE { return _inputsCount; }
    
    virtual bool connectInput(Node* input,int inputNumber,bool autoConnection = false) OVERRIDE;
    
    virtual int disconnectInput(int inputNumber) OVERRIDE;
    
    virtual int disconnectInput(Node* input) OVERRIDE;
    
    bool tryAddEmptyInput();
    
    void addEmptyInput();
    
    void removeEmptyInputs();
    
    void setActiveInputAndRefresh(int inputNb);
    
    int activeInput() const {return _activeInput;}
};

#endif // NATRON_ENGINE_NODE_H_
