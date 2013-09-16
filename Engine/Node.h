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

#include "Global/Macros.h"

#include "Engine/ChannelSet.h"
#include "Engine/Format.h"
#include "Engine/VideoEngine.h"
#include "Engine/Hash64.h"

class Row;
class Model;
class SettingsPanel;
class Knob;
class QMutex;
class QWaitCondition;
class QUndoCommand;
class QUndoStack;
class Node : public QObject
{
    Q_OBJECT
    
public:
	/*Per-node infos. This class is used to pass infos along the graph
	 *and to know what we can request from a node.*/
	class Info{
        
	public:
		Info(int first_frame,int last_frame,int ydirection,Format displayWindow,ChannelSet channels,VideoEngine* engine)
        : _firstFrame(first_frame)
        , _lastFrame(last_frame)
        , _ydirection(ydirection)
        , _blackOutside(false)
        , _rgbMode(true)
        , _dataWindow()
        , _displayWindow(displayWindow)
        , _channels(channels),
        _executingEngine(engine)
        {}
	    Info()
        : _firstFrame(0)
        , _lastFrame(0)
        , _ydirection(0)
        , _blackOutside(false)
        , _rgbMode(true)
        , _dataWindow()
        , _displayWindow()
        , _channels()
        ,_executingEngine(NULL)
        {}

		void set_ydirection(int direction){_ydirection=direction;}
		int ydirection() const {return _ydirection;}

		void set_displayWindow(const Format& format) { _displayWindow = format; }
        void merge_displayWindow(const Format& other);
		const Format& displayWindow() const { return _displayWindow; }
        
		void set_dataWindow(const Box2D& win) { _dataWindow = win; }
        void merge_dataWindow(const Box2D& other) { _dataWindow.merge(other); }
		const Box2D& dataWindow() const { return _dataWindow; }

        void set_firstFrame(int nb) { _firstFrame = nb; }
		int firstFrame() const { return _firstFrame; }
        
		void set_lastFrame(int nb) { _lastFrame = nb; }
		int lastFrame() const { return _lastFrame; }
        
		void set_channels(const ChannelSet& mask) { _channels = mask; }
		const ChannelSet& channels() const { return _channels; }

		bool blackOutside() const { return _blackOutside; }
		void set_blackOutside(bool bo) { _blackOutside = bo; }

        void set_rgbMode(bool m) { _rgbMode = m; }
        bool rgbMode() const { return _rgbMode; }

        bool operator==( Node::Info &other);
        void operator=(const Node::Info &other);
        
        VideoEngine* executingEngine() const {return _executingEngine;}
        
        void setExecutingEngine(VideoEngine* engine){_executingEngine = engine;}

        void reset();
        
	private:
		int _firstFrame;
		int _lastFrame;
		int _ydirection;
		bool _blackOutside;
        bool _rgbMode;
        Box2D _dataWindow;
		Format _displayWindow; // display window of the data, for the data window see x,y,range,offset parameters
		ChannelSet _channels; // all channels defined by the current Node ( that are allocated)
        VideoEngine* _executingEngine;// the engine owning the node currently. This is protected by a mutex and is thread-safe.
	};
    
    typedef std::map<int,Node*> InputMap;
    typedef std::multimap<int,Node*> OutputMap;
    
#define foreachInput(CUR,NODE)\
for(Node::InputMap::const_iterator CUR = NODE->getInputs().begin(); CUR!= NODE->getInputs().end() ;++CUR) \

    /*CONSTRUCTOR AND DESTRUCTORS*/
    Node(Model* model);
    virtual ~Node();
    
    void deleteNode();
    /*============================*/
    
    /*Hash related functions*/
    const Hash64& hash() const { return _hashValue; }
    
    void computeTreeHash(std::vector<std::string> &alreadyComputedHash);
    
    bool hashChanged();
    
    /*============================*/
    
    /*overload this to init any knobs*/
    
    void initializeKnobs();
    
	virtual void createKnobDynamically();
    
    /*Called by KnobFactory::createKnob. You
     should never call this yourself. The knob belongs only to this Node.*/
    void addKnob(Knob* knob){_knobs.push_back(knob);}
    
    void removeKnob(Knob* knob);
    
    const std::vector<Knob*>& getKnobs() const {return _knobs;}
    
    /*============================*/
    /*Returns a pointer to the Viewer node ptr if this node has a viewer connected,
     otherwise returns NULL.*/
    static Node* hasViewerConnected(Node* node);
    /*============================*/
    
    /*DAG related (topological sorting)*/
    void setMarked(bool mark){_marked = mark;}
    
    bool isMarked(){return _marked;}
    
    
    /*============================*/
    
	/*Node infos*/
	const Info& info() const { return _info; }
    
    void clear_info();


	Box2D& getRequestedBox(){return _requestedBox;}

    int width() const {return info().displayWindow().width();}
    int height() const {return info().displayWindow().height();}
       
    /*============================*/
    
    /*Node type related functions*/
    virtual bool isInputNode() const {return false;}
    
    virtual bool isOutputNode() const {return false;}
    /*============================*/
    
    /*Node Input related functions*/
    void initializeInputs();
    
    virtual int maximumInputs() const {return 1;}
    
    virtual int minimumInputs() const {return 1;}
    
    Node* input(int index) const;
    
    const std::map<int, std::string>& getInputLabels() const { return _inputLabelsMap; }
    
    virtual std::string setInputLabel(int inputNb);
    
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
    
    
    
    /*node name related functions*/
    const std::string& getName() const { return _name ; }

    void setName(const std::string& name) {
        _name = name;
        emit nameChanged(name.c_str());
    }

    /*============================*/

    /*Node utility functions*/
    virtual std::string className() = 0;
    virtual std::string description() = 0;
    /*============================*/
    
    /*Calculations related functions*/
    bool validate(bool doFullWork);
    
    virtual void engine(int y,int offset,int range,ChannelSet channels,Row* out){
        Q_UNUSED(y);
        Q_UNUSED(offset);
        Q_UNUSED(range);
        Q_UNUSED(channels);
        Q_UNUSED(out);
    }
	
    /*============================*/
    
    /*overlay support:
     *Just overload this function in your operator.
     *No need to include any OpenGL related header.
     *The coordinate space is  defined by the displayWindow
     *(i.e: (0,0) = bottomLeft and  width() and height() being
     * respectivly the width and height of the frame.)
     */
    virtual void drawOverlay(){}
    
    /*cache related*/
    
    /*Returns in row, a row containing the results expected of this node
     for the line y , channels and range (r-x). Data may come from the cache,
     otherwise engine() gets called.
     */
    Row* get(int y,int x,int r);
    
    /*Returns true if the node will cache rows in the node cache.
     Otherwise results will not be cached.*/
    virtual bool cacheData() const = 0;
    

    virtual bool isOpenFXNode() const {return false;}
    
    Model* getModel() const {return _model;}
            
    VideoEngine* getExecutingEngine() const {return _info.executingEngine();}
        
    void setExecutingEngine(VideoEngine* engine){_info.setExecutingEngine(engine);}

    void set_firstFrame(int nb) { _info.set_firstFrame(nb); }

    void set_lastFrame(int nb) { _info.set_lastFrame(nb); }
    
    
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
        
    void pushUndoCommand(QUndoCommand* command);
    
    void undoCommand();
    
    void redoCommand();
    
    void doRefreshEdgesGUI(){
        emit refreshEdgesGUI();
    }
   
    void notifyGuiPreviewChanged(){
        emit previewImageChanged();
    }
    
    /*0 if not, 1 if yes and this is a Reader, 2 if yes and this is an OpenFX node*/
    int canMakePreviewImage();
    
public slots:
    
    void onGUINameChanged(const QString& str){
        _name = str.toStdString();
    }
    

signals:
    
    void inputsInitialized();
    
    void knobsInitialied();
    
    void channelsChanged();
    
    void inputChanged(int);
        
    void activated();
    
    void deactivated();
    
    void canUndoChanged(bool);
    
    void canRedoChanged(bool);
    
    void nameChanged(QString);
    
    void deleteWanted();
        
    void refreshEdgesGUI();

    void previewImageChanged();
    
protected:
    
    virtual ChannelSet supportedComponents() = 0;
        
    virtual void initKnobs(){}

	virtual bool _validate(bool /*doFullWork*/) = 0;
    
    Model* _model; // pointer to the model
	Info _info; // contains all the info for this operator:the channels on which it is defined,the area of the image, the image format etc...this is set by validate

    bool _marked; //used by the topological sort algorithm
	std::map<int, std::string> _inputLabelsMap; // inputs name
    std::string _name; //node name set by the user
	Hash64 _hashValue; // hash value
	Box2D _requestedBox; // composition of all the area requested by children
    
    std::multimap<int,Node*> _outputs; //multiple outputs per slot
    std::map<int,Node*> _inputs;//only 1 input per slot
    std::vector<Knob*> _knobs;
    
    boost::scoped_ptr<QUndoStack> _undoStack;

private:
    
    typedef std::map<Node*,std::pair<int,int> >::const_iterator OutputConnectionsIterator;
    typedef OutputConnectionsIterator InputConnectionsIterator;
    struct DeactivatedState{
        /*The output node was connected from inputNumber to the outputNumber of this...*/
        std::map<Node*,std::pair<int,int> > _outputsConnections;
        
        /*The input node was connected from outputNumber to the inputNumber of this...*/
        std::map<Node*,std::pair<int,int> > _inputConnections;
    };
        
    void merge_frameRange(int otherFirstFrame,int otherLastFrame);
    
    void merge_info(bool doFullWork);
    
    void copy_info(Node* parent);
    
    static void _hasViewerConnected(Node* node,bool* ok,Node*& out);
    
    DeactivatedState _deactivatedState;
    
};
typedef Node* (*NodeBuilder)();

class TimeLine: public QObject {
    Q_OBJECT
    
    int _firstFrame;
    int _lastFrame;
    int _currentFrame;
    

public:
    
    TimeLine():
    _firstFrame(0),
    _lastFrame(100),
    _currentFrame(0)
    {}
    
    virtual ~TimeLine(){}
    
    int firstFrame() const {return _firstFrame;}
    
    void setFirstFrame(int f) {_firstFrame = f;}
    
    int lastFrame() const {return _lastFrame;}
    
    void setLastFrame(int f){_lastFrame = f;}
    
    int currentFrame() const {return _currentFrame;}
    
    void incrementCurrentFrame(){++_currentFrame; emit frameChanged(_currentFrame);}
    
    void decrementCurrentFrame(){--_currentFrame; emit frameChanged(_currentFrame);}

public slots:
    
    void seek(int frame);
    
    void seek_noEmit(int frame);
    
    
signals:
    
    void frameChanged(int);
    
    
};

class OutputNode : public Node{
public:
    
    OutputNode(Model* model);
    
    virtual ~OutputNode();
    
    virtual bool isOutputNode() const OVERRIDE {return true;}

    
    /*Node utility functions*/
    virtual std::string className() OVERRIDE = 0; // should be const
    
    virtual std::string description() OVERRIDE = 0; // should be const
    /*Returns true if the node will cache rows in the node cache.
     Otherwise results will not be cached.*/
    virtual bool cacheData() const OVERRIDE = 0;
   
    VideoEngine* getVideoEngine() const {return _videoEngine;}
    
    const TimeLine& getTimeLine() const {return _timeline;}
    
    TimeLine& getTimeLine(){return _timeline;}
        
    void updateDAG(bool initViewer = false);

protected:
    virtual ChannelSet supportedComponents() OVERRIDE = 0; // should be const
    virtual bool _validate(bool /*doFullWork*/) OVERRIDE = 0;
    
    TimeLine _timeline;

    
private:
    VideoEngine* _videoEngine;
};


#endif // POWITER_ENGINE_NODE_H_
