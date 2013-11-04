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

#include <QMutex>
#include <QWaitCondition>
#include <QThreadStorage>
#include <boost/shared_ptr.hpp>

#include "Global/Macros.h"

#include "Engine/ChannelSet.h"
#include "Engine/Format.h"
#include "Engine/VideoEngine.h"
#include "Engine/Hash64.h"
#include "Engine/Variant.h"

namespace Powiter{
class Row;
class Image;
class EffectInstance;
class LibraryBinary;
}
class AppInstance;
class NodeSettingsPanel;
class Knob;
class ViewerInstance;
class QKeyEvent;
class RenderTree;

class Node : public QObject
{
    Q_OBJECT
    
public:

    typedef std::map<int,Node*> InputMap;
    typedef std::multimap<int,Node*> OutputMap;

    Node(AppInstance* app,Powiter::LibraryBinary* plugin,const std::string& name);
    
    virtual ~Node();
    
    /*Never call this yourself.*/
    void setLiveInstance(Powiter::EffectInstance* liveInstance) {_liveInstance = liveInstance;}
    
    Powiter::EffectInstance* getLiveInstance() const {return _liveInstance;}
    
    /**
     * @brief Find or creates a clone of the live instance (i.e all the knobs etc...) for the given tree.
     * The effect instance returns is guaranteed to reflect exactly the state of the live instance at the
     * time at which it has been called.
     **/
    Powiter::EffectInstance* findOrCreateLiveInstanceClone(RenderTree* tree);
    
    /**
     * @brief Forwarded to the live effect instance
     **/
    void initializeKnobs();
    
    /**
     * @brief Forwarded to the live effect instance
     **/
    const std::vector<Knob*>& getKnobs() const;

    /*Returns a pointer to the Viewer node ptr if this node has a viewer connected,
     otherwise returns NULL.*/
    ViewerInstance* hasViewerConnected();
    
    
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
     * @brief Forwarded to the live effect instance
     **/
    void openFilesForAllFileKnobs();
    
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
    
    const std::map<int, std::string>& getInputLabels() const { return _inputLabelsMap; }
    
    std::string getInputLabel(int inputNb) const;
    
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
     * @brief Forwarded to the live effect instance
     **/
    std::string className() const;
    

    /**
     * @brief Forwarded to the live effect instance
     **/
    std::string description() const;
    
    /*============================*/

    
    AppInstance* getApp() const {return _app;}
    
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
    

    const Format& getRenderFormatForEffect(const Powiter::EffectInstance* effect) const;
    
    int getRenderViewsCountForEffect(const Powiter::EffectInstance* effect) const;
    
    /**
     * @brief Forwarded to the live effect instance
     **/
    Knob* getKnobByDescription(const std::string& desc) const;
    
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
    
    void onOverlayKeyDown(QKeyEvent* e);
    
    void onOverlayKeyUp(QKeyEvent* e);
    
    void onOverlayKeyRepeat(QKeyEvent* e);
    
    void onOverlayFocusGained();
    
    void onOverlayFocusLost();

    bool makePreviewByDefault() const;
    
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
    
    void setMarkedByTopologicalSort(bool marked) const {_markedByTopologicalSort = marked;}
    
    bool isMarkedByTopologicalSort() const {return _markedByTopologicalSort;}
    
    
    /**
     * @brief
     * You must call this in order to notify the GUI of any change (add/delete) for knobs
     **/
    void createKnobDynamically();
    
    /**
     * @brief Returns true if the node is active for use in the graph editor.
     **/
    bool isActivated() const {return _activated;}

    
    boost::shared_ptr<Powiter::Image> getImageBeingRendered(SequenceTime time,int view) const;
    
    void addImageBeingRendered(boost::shared_ptr<Powiter::Image>,SequenceTime time,int view );
    
    void removeImageBeingRendered(SequenceTime time,int view );
    
    void abortRenderingForEffect(Powiter::EffectInstance* effect);

    Powiter::EffectInstance* findExistingEffect(RenderTree* tree) const;

    void invalidateDownStreamHash();
    
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
    
    void notifyGuiChannelChanged(const Powiter::ChannelSet& c){
        emit channelsChanged(c);
    }

    void notifyFrameRangeChanged(int f,int l){
        emit frameRangeChanged(f,l);
    }
    
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
    
    void channelsChanged(Powiter::ChannelSet);

    void knobUndoneChange();
    
    void knobRedoneChange();
    
    void frameRangeChanged(int,int);
protected:
    AppInstance* _app; // pointer to the model: needed to access the application's default-project's format
    std::multimap<int,Node*> _outputs; //multiple outputs per slot
    std::map<int,Node*> _inputs;//only 1 input per slot
    Powiter::EffectInstance*  _liveInstance; //< the instance of the effect interacting with the GUI of this node.
    
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
    
    DeactivatedState _deactivatedState;
    
    mutable bool _markedByTopologicalSort; //< used by the topological sort algorithm
    bool _activated;
    QMutex _nodeInstanceLock;
    QWaitCondition _imagesBeingRenderedNotEmpty; //to avoid computing preview in parallel of the real rendering
    
    /*A key to identify an image rendered for this node.*/
    struct ImageBeingRenderedKey{
        
        ImageBeingRenderedKey():_time(0),_view(0){}
        
        ImageBeingRenderedKey(int time,int view):_time(time),_view(view){}
        
        SequenceTime _time;
        int _view;
        
        bool operator==(const ImageBeingRenderedKey& other) const {
            return _time == other._time &&
                    _view == other._view;
        }
        
        bool operator<(const ImageBeingRenderedKey& other) const {
            return _time < other._time ||
                    _view < other._view;
        }
    };
    
    typedef std::multimap<ImageBeingRenderedKey,boost::shared_ptr<Powiter::Image> > ImagesMap;
    ImagesMap _imagesBeingRendered; //< a map storing the ongoing render for this node
    Powiter::LibraryBinary* _plugin;
    std::map<RenderTree*,Powiter::EffectInstance*> _renderInstances;
};


/**
 * @brief An InspectorNode is a type of node that is able to have a dynamic number of inputs.
 * Only 1 input is considered to be the "active" input of the InspectorNode, but several inputs
 * can be connected. This Node is suitable for effects that take only 1 input in parameter.
 * This is used for example by the Viewer, to be able to switch quickly from several inputs
 * while still having 1 input active.
 **/
class InspectorNode: public Node
{
    int _inputsCount;
    int _activeInput;
    
public:
    
    InspectorNode(AppInstance* app,Powiter::LibraryBinary* plugin,const std::string& name);
    
    virtual ~InspectorNode();
    
    virtual int maximumInputs() const OVERRIDE { return _inputsCount; }
    
    bool connectInput(Node* input,int inputNumber,bool autoConnection = false);
    
    virtual int disconnectInput(int inputNumber) OVERRIDE;
    
    virtual int disconnectInput(Node* input);
    
    bool tryAddEmptyInput();
    
    void addEmptyInput();
    
    void removeEmptyInputs();
    
    void setActiveInputAndRefresh(int inputNb);
    
    int activeInput() const {return _activeInput;}
};

#endif // POWITER_ENGINE_NODE_H_
