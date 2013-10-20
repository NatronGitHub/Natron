//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef POWITER_ENGINE_NODE_H_
#define POWITER_ENGINE_NODE_H_

#include <vector>
#include <string>
#include <map>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "Global/Macros.h"

#include "Engine/ChannelSet.h"
#include "Engine/Format.h"
#include "Engine/VideoEngine.h"
#include "Engine/Hash64.h"
namespace Powiter{
    class Row;
}
class Model;
class SettingsPanel;
class Knob;
class QUndoCommand;
class ViewerNode;
class QKeyEvent;
class QUndoStack;
class Node;


class Node : public QObject
{
    Q_OBJECT
    
public:
    
    typedef std::map<int,Node*> InputMap;
    typedef std::multimap<int,Node*> OutputMap;
    

    Node(Model* model);
    
    virtual ~Node();
    
    /**
     *@brief Deletes the node properly. You should not destroy the node manually but call this
     * which will in turn notify the gui that we want to delete anything associated to this node.
     **/
    void deleteNode();
    
    /**
     * @brief Calls the virtual portion of initKnobs() and then notifies the GUI that
     * the knobs have been created.
     **/
    void initializeKnobs();
    


    const std::vector<Knob*>& getKnobs() const { return _knobs; }
    
    /*Returns a pointer to the Viewer node ptr if this node has a viewer connected,
     otherwise returns NULL.*/
    ViewerNode* hasViewerConnected();
    
public:
    
    /**
     * @brief Is this node an input node ? An input node means
     * it has no input.
     **/
    virtual bool isInputNode() const { return false; }
    
    /**
     * @brief Is this node an output node ? An output node means
     * it has no output.
     **/
    virtual bool isOutputNode() const { return false; }
    
    
    /**
     * @brief Returns true if the node is capable of generating
     * data and process data on the input as well
     **/
    virtual bool isInputAndProcessingNode() const { return false; }
    
    /**
     * @brief Is this node an OpenFX node?
     **/
    virtual bool isOpenFXNode() const { return false; }
    
    
    
    /*============================*/
    /*Initialises inputs*/
    void initializeInputs();
    
    virtual int maximumInputs() const = 0;
    
    virtual int minimumInputs() const = 0;
    
    /**
     * @brief Returns a pointer to the input Node at index 'index'
     * or NULL if it couldn't find such node.
     **/
    Node* input(int index) const;
    
    const std::map<int, std::string>& getInputLabels() const { return _inputLabelsMap; }
    
public:
    
    const std::string getInputLabel(int inputNb) const;
    
    bool isInputConnected(int inputNb) const;
    
    bool hasOutputConnected() const;
    
    const InputMap& getInputs() const {return _inputs;}
    
    const OutputMap& getOutputs() const {return _outputs;}
    
    /** @brief Adds the node parent to the input inputNumber of the
     * node. Returns true if it succeeded, false otherwise.
     * When returning false, this means an input was already
     * connected for this inputNumber. It should be removed
     * beforehand.
     */
    bool connectInput(Node* input,int inputNumber);
    
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
    const std::string& getName() const { return _name ; }

    void setName(const std::string& name) {
        _name = name;
        emit nameChanged(name.c_str());
    }


    /**
     * @brief Must be implemented to give a name to the class.
     **/
    virtual std::string className() const = 0;
    
   
    /**
     * @brief Must be implemented to give a desription of the effect that this node does. This is typically
     * what you'll see displayed when the user clicks the '?' button on the node's panel in the user interface.
     **/
    virtual std::string description() const = 0;
    
    /*============================*/
  
    
    Model* getModel() const {return _model;}
    
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
    

    const Format& getProjectDefaultFormat() const;

    /*forwards to _fakeInstance*/
    void undoCommand();
    
    /*forwards to _fakeInstance*/
    void redoCommand();
    
    
    
    /*Called by KnobFactory::createKnob. You
     should never call this yourself. The knob belongs only to this Node.*/
    void addKnob(Knob* knob){ _knobs.push_back(knob); }
    
    void removeKnob(Knob* knob);
    
    void pushUndoCommand(QUndoCommand* command);
    
    /*@brief The derived class should query this to abort any long process
     in the engine function.*/
    bool aborted() const { return _renderAborted; }
    
    /**
     * @brief Called externally when the rendering is aborted. You should never
     * call this yourself.
     **/
    void setAborted(bool b) { _renderAborted = b; }

    
    
    /*@brief Overlay support:
     *Just overload this function in your operator.
     *No need to include any OpenGL related header.
     *The coordinate space is  defined by the displayWindow
     *(i.e: (0,0) = bottomLeft and  width() and height() being
     * respectivly the width and height of the frame.)
     */
    virtual void drawOverlay(){}
    
    virtual bool onOverlayPenDown(const QPointF& viewportPos,const QPointF& pos){
        Q_UNUSED(viewportPos);
        Q_UNUSED(pos);
        return false;
    }
    
    virtual bool onOverlayPenMotion(const QPointF& viewportPos,const QPointF& pos){
        Q_UNUSED(viewportPos);
        Q_UNUSED(pos);
        return false;
    }
    
    virtual bool onOverlayPenUp(const QPointF& viewportPos,const QPointF& pos){
        Q_UNUSED(viewportPos);
        Q_UNUSED(pos);
        return false;
    }
    
    virtual void onOverlayKeyDown(QKeyEvent* e){ Q_UNUSED(e); }
    
    virtual void onOverlayKeyUp(QKeyEvent* e){ Q_UNUSED(e); }
    
    virtual void onOverlayKeyRepeat(QKeyEvent* e){ Q_UNUSED(e); }
    
    virtual void onOverlayFocusGained(){}
    
    virtual void onOverlayFocusLost(){}
    
    /**
     * @brief Overload this and return true if your operator is capable of dislaying a preview image.
     * You should also implement makePreviewImage to make a valid preview (that is not black).
     **/
    virtual bool canMakePreviewImage() const {return false;}
    
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
    
    const Hash64& hash() const { return _hashValue; }
    
    void computeTreeHash(std::vector<std::string> &alreadyComputedHash);

    void setMarkedByTopologicalSort(bool marked) {_markedByTopologicalSort = marked;}
    
    bool isMarkedByTopologicalSort() const {return _markedByTopologicalSort;}
    
    /** @brief Returns a row containing the results expected of the node
     * inputNb for the scan-line y , the range [left-right] and the channels 'channels' at time 'time'
     * Data may come from the cache, otherwise engine() gets called to compute fresh data.
     * WARNING: Once the shared_ptr is deleted, the buffer in the Row may not
     * be valid anymore.
     */
    boost::shared_ptr<const Powiter::Row> get(SequenceTime time,int y,int x,int r,const ChannelSet& channels);
    
    /**
     * @brief This is the place to initialise any per frame specific data or flag.
     * If the return value is StatOK or StatReplyDefault the render will continue, otherwise
     * the render will stop.
     * If the status is StatFailed a message should be posted by the plugin.
     **/
    virtual Powiter::Status preProcessFrame(SequenceTime /*time*/) { return Powiter::StatReplyDefault; }

    
    /**
     * @brief Can be derived to get the region that the plugin is capable of filling.
     * This is meaningful for plugins that generate images or transform images.
     * By default it returns in rod the union of all inputs RoD and StatReplyDefault is returned.
     * In case of failure the plugin should return StatFailed.
     * If displayWindow is not NULL the plug-in may change the displayWindow to inform what region
     * should really be displayed. The displayWindow may differ from the rod.
     * For instance EXR has different data window/display window and when NULL an EXR file should
     * fill in a proper display window to reflect how the image is supposed to be viewed.
     * By default the display window is the same as the rod.
     **/
    virtual Powiter::Status getRegionOfDefinition(SequenceTime time,Box2D* rod,Format* displayWindow = NULL);
    
    // virtual void getFrameRange(double* first,double* second);
    
    void ifInfiniteclipBox2DToProjectDefault(Box2D* rod) const;
    
    /**
     * @brief Can be derived to get the frame range wherein the plugin is capable of producing frames.
     * By default it merges the frame range of the inputs.
     * In case of failure the plugin should return StatFailed.
     **/
    virtual void getFrameRange(SequenceTime *first,SequenceTime *last);
    
    /**
     * @brief
     * You must call this in order to notify the GUI of any change (add/delete) for knobs
     **/
    void createKnobDynamically();
    

public slots:
    
    void onGUINameChanged(const QString& str){
        _name = str.toStdString();
    }

    void doRefreshEdgesGUI(){
        emit refreshEdgesGUI();
    }
    
    void refreshPreviewImage(int time){
        emit previewImageChanged(time);
    }
    
    void notifyGuiChannelChanged(const ChannelSet& c){
        emit channelsChanged(c);
    }

    void notifyFrameRangeChanged(int f,int l){
        emit frameRangeChanged(f,l);
    }
    
protected:
    
    /**
     * @brief Must be implemented to do the rendering. You must write to the Row 'out'
     * for the frame at time 'time'.
     * You can access the size, range and Channels defined in the Row by calling
     * Row::width()
     * Row::left()
     * Row::right()
     * Row::channels()
     *
     * To retrieve the results of this row computed from nodes upstream you can call
     * input(inputNb)->get(y,left,right)
     **/
    virtual void render(SequenceTime time,Powiter::Row* out){
        Q_UNUSED(time);
        Q_UNUSED(out);
    }
    
  
    /**
     * @Returns true if the node will cache results in the node cache.
     * Otherwise results will not be cached.
     **/
    virtual bool cacheData() const = 0;
    
    /**
     * @brief Can be derived to give a more meaningful label to the input 'inputNb'
     **/
    virtual std::string setInputLabel(int inputNb) const;
    

    /**
     * @brief Overload this to initialize any knob.
     **/
    virtual void initKnobs(){}
    
    
    
signals:
    
    void inputsInitialized();
    
    void knobsInitialized();
    
    void inputChanged(int);
        
    void activated();
    
    void deactivated();
    
    void canUndoChanged(bool);
    
    void canRedoChanged(bool);
    
    void nameChanged(QString);
    
    void deleteWanted();
        
    void refreshEdgesGUI();

    void previewImageChanged(int);
    
    void channelsChanged(ChannelSet);

    void knobUndoneChange();
    
    void knobRedoneChange();
    
    void frameRangeChanged(int,int);
protected:
    Model* _model; // pointer to the model: needed to access the application's default-project's format
    std::multimap<int,Node*> _outputs; //multiple outputs per slot
    std::map<int,Node*> _inputs;//only 1 input per slot
private:
    typedef std::map<Node*,std::pair<int,int> >::const_iterator OutputConnectionsIterator;
    typedef OutputConnectionsIterator InputConnectionsIterator;
    struct DeactivatedState{
        /*The output node was connected from inputNumber to the outputNumber of this...*/
        std::map<Node*,std::pair<int,int> > _outputsConnections;
        
        /*The input node was connected from outputNumber to the inputNumber of this...*/
        std::map<Node*,std::pair<int,int> > _inputConnections;
    };
    
    
    std::map<int, std::string> _inputLabelsMap; // inputs name
    std::string _name; //node name set by the user
    std::vector<Knob*> _knobs;
    
    boost::scoped_ptr<QUndoStack> _undoStack;
    DeactivatedState _deactivatedState;
    
    Hash64 _hashValue;
    bool _markedByTopologicalSort; //< used by the topological sort algorithm
    bool _renderAborted; //< was rendering aborted ?

    
};


/**
 *@brief A node whose role is to output an image.
 * It owns rendering engine (VideoEngine)
 * For now only 3 kind of OutputNode exist: Viewer,Writer and OfxNode.
 * The last one is particular and is not actually acting as an output node
 * every times. It depends upon the plug-in hosted by the OfxNode.
 **/
class OutputNode : public Node {
    
public:
    
    OutputNode(Model* model);
    
    virtual ~OutputNode(){}
    
    /**
     * @brief Is this node an output node ? An output node means
     * it has no output.
     **/
    virtual bool isOutputNode() const { return true; }
    
    boost::shared_ptr<VideoEngine> getVideoEngine() const {return _videoEngine;}
    
    void updateTreeAndRender(bool initViewer = false);
    
    void refreshAndContinueRender(bool initViewer = false);
    
    
private:
    boost::shared_ptr<VideoEngine> _videoEngine;
    
};


#endif // POWITER_ENGINE_NODE_H_
