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

#include "Global/Macros.h"
#include "Engine/ChannelSet.h"
#include "Engine/Format.h"
#include "Engine/VideoEngine.h"
#include "Engine/Hash64.h"

class Row;
class Model;
class SettingsPanel;
class Knob; // from Gui/Knob.h (shouldn't be used here?)
class KnobCallback; // from Gui/Knob.h (shouldn't be used here?)
class NodeGui;
class QMutex;
class QWaitCondition;
class QUndoStack;
class Node
{
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
        : _firstFrame(-1)
        , _lastFrame(-1)
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
    
    
    
    /*CONSTRUCTOR AND DESTRUCTORS*/
    Node(Model* model);
    virtual ~Node();
    /*============================*/
    
    /*Hash related functions*/
    const Hash64& hash() const { return _hashValue; }
    void computeTreeHash(std::vector<std::string> &alreadyComputedHash);
    bool hashChanged();
    /*============================*/
    
    /*Knobs related functions*/
    const std::vector<Knob*>& getKnobs() const { return _knobsVector; }
    void addToKnobVector(Knob* knob){ _knobsVector.push_back(knob); }
    
    /*Do not call this function. It is used
     internally by the Knob_Callback.*/
    void removeKnob(Knob* knob);
    virtual void initKnobs(KnobCallback *cb);
	virtual void createKnobDynamically();
	KnobCallback* getKnobCallBack(){return _knobsCB;}
	void setNodeUi(NodeGui* ui){_nodeGUI=ui;}
	NodeGui* getNodeUi(){return _nodeGUI;}
    /*============================*/
    
    /*Parents & children nodes related functions*/
   const std::vector<Node*>& getParents() const {return _parents;}
    const std::vector<Node*>& getChildren() const {return _children;}
    void addChild(Node* child){ _children.push_back(child); }
    void addParent(Node* parent){ _parents.push_back(parent); }
    void removeChild(Node* child);
    void removeParent(Node* parent);
    void removeThisFromParents();
    void removeThisFromChildren();
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
    int inputCount() const;
    Node* input(int index);
    const std::map<int, std::string>& getInputLabels() const { return _inputLabelsMap; }
    virtual std::string setInputLabel(int inputNb);
    std::string getInputLabel(int inputNb) {  return _inputLabelsMap[inputNb]; }
    void applyLabelsToInputs();
    void initInputLabelsMap();
    /*============================*/
    
    
    
    /*node name related functions*/
    const std::string getName() const { return _name ; }

    void setName(const std::string& name) { _name = name; }

    /*============================*/

    /*Node utility functions*/
    virtual std::string className() = 0;
    virtual std::string description() = 0;
    /*============================*/
    
    /*Calculations related functions*/
    bool validate(bool forReal);
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
    
    
    VideoEngine* getExecutingEngine() const {return _info.executingEngine();}
    
    Model* getModel() const {return _model;}
    
    void setExecutingEngine(VideoEngine* engine){_info.setExecutingEngine(engine);}

    void set_firstFrame(int nb) { _info.set_firstFrame(nb); }

    void set_lastFrame(int nb) { _info.set_lastFrame(nb); }

protected:
    
    virtual ChannelSet supportedComponents() = 0; 
    virtual void preProcess(){}

	virtual bool _validate(bool /*forReal*/) = 0;
    
    Model* _model;
	Info _info; // contains all the info for this operator:the channels on which it is defined,the area of the image, the image format etc...this is set by validate
	std::vector<Node*> _parents;
	std::vector<Node*> _children;
    bool _marked;
	std::map<int, std::string> _inputLabelsMap;
    std::string _name;
	Hash64 _hashValue;
	std::vector<Knob*> _knobsVector;
	KnobCallback* _knobsCB;
	Box2D _requestedBox; // composition of all the area requested by children
	NodeGui* _nodeGUI;

private:
    void merge_frameRange(int otherFirstFrame,int otherLastFrame);
    void merge_info(bool forReal);
    void copy_info(Node* parent);
    
};
typedef Node* (*NodeBuilder)();

class TimeLine: public QObject{
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
    
    QMutex* getEngineMutex() const {return _mutex;}
    
    QWaitCondition* getOpenGLCondition() const {return _openGLCondition;}
    
    const TimeLine& getTimeLine() const {return _timeline;}
    
    TimeLine& getTimeLine(){return _timeline;}
    
protected:
    virtual ChannelSet supportedComponents() OVERRIDE = 0; // should be const
    virtual bool _validate(bool /*forReal*/) OVERRIDE = 0;
    
    TimeLine _timeline;

    
private:
    QMutex* _mutex;
    QWaitCondition* _openGLCondition;
    VideoEngine* _videoEngine;
};


#endif // POWITER_ENGINE_NODE_H_
