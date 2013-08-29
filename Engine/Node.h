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

 

 



#ifndef NODE_H
#define NODE_H

#include <iostream>
#include <cstdio>
#include <map>
#include "Engine/ChannelSet.h"
#include "Global/GlobalDefines.h"
#include "Engine/Format.h"

class Row;
class InputRow;
class Model;
class SettingsPanel;
class Hash;
class Knob;
class KnobCallback;
class NodeGui;
class QUndoStack;
class Node
{
public:
	/*Per-node infos. This class is used to pass infos along the graph
	 *and to know what we can request from a node.*/
	class Info:public Box2D{
	public:
		Info(int first_frame,int last_frame,int ydirection,Format displayWindow,ChannelSet channels):Box2D(),
        _firstFrame(first_frame),
        _lastFrame(last_frame),
        _ydirection(ydirection),
        _blackOutside(false),
        _rgbMode(true),
        _displayWindow(displayWindow),
        _channels(channels)
        {}
	    Info():Box2D(),_firstFrame(-1),_lastFrame(-1),_ydirection(0),_blackOutside(false),_rgbMode(true),_displayWindow(),_channels(){}
		void setYdirection(int direction){_ydirection=direction;}
		int getYdirection() const {return _ydirection;}
		void setDisplayWindow(Format format){_displayWindow=format;}
		const Format& getDisplayWindow() const {return _displayWindow;}
		const Box2D& getDataWindow() const {return dynamic_cast<const Box2D&>(*this);}
		bool operator==( Node::Info &other);
        void operator=(const Node::Info &other);
		void firstFrame(int nb){_firstFrame=nb;}
		void lastFrame(int nb){_lastFrame=nb;}
		int firstFrame() const {return _firstFrame;}
		int lastFrame() const {return _lastFrame;}
		void setChannels(ChannelSet mask){_channels=mask;}
		const ChannelSet& channels() const {return _channels;}
		bool blackOutside() const {return _blackOutside;}
		void blackOutside(bool bo){_blackOutside=bo;}
        void rgbMode(bool m){_rgbMode=m;}
        bool rgbMode() const {return _rgbMode;}
        void mergeDisplayWindow(const Format& other);
        
        void reset();
        
	private:
		int _firstFrame;
		int _lastFrame;
		int _ydirection;
		bool _blackOutside;
        bool _rgbMode;
		Format _displayWindow; // display window of the data, for the data window see x,y,range,offset parameters
		ChannelSet _channels; // all channels defined by the current Node ( that are allocated)
	};
    
    
    
    /*CONSTRUCTOR AND DESTRUCTORS*/
    Node();
    virtual ~Node();
    /*============================*/
    
    /*Hash related functions*/
    const Hash* getHash() const{return _hashValue;}
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
	Info* getInfo(){return _info;}
    void clear_info();


	Box2D& getRequestedBox(){return _requestedBox;}
    int width(){return _info->getDisplayWindow().w();}
    int height(){return _info->getDisplayWindow().h();}
       
    /*============================*/
    
    /*Node type related functions*/
    virtual bool isInputNode()  {return false;}
    virtual bool isOutputNode()  {return false;}
    /*============================*/
    
    /*Node Input related functions*/
    void initializeInputs();
    virtual int maximumInputs() {return 1;}
    virtual int minimumInputs() {return 1;}
    int inputCount() const ;
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
    virtual const std::string className() =0;
    virtual const std::string description() =0;
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
    // FIXME: row is both input and output! The inputs should really be int y, int x, int offset (see Row constructor), and it should return a Row*
    void get(InputRow& row);
    
    /*Returns true if the node will cache rows in the node cache.
     Otherwise results will not be cached.*/
    virtual bool cacheData()=0;
    

    virtual bool isOpenFXNode() const {return false;}
        
protected:
    
    virtual ChannelSet supportedComponents() =0;
    virtual void preProcess(){}
	virtual bool _validate(bool forReal){(void)forReal; return true;}
    
	Info* _info; // contains all the info for this operator:the channels on which it is defined,the area of the image, the image format etc...this is set by validate
	std::vector<Node*> _parents;
	std::vector<Node*> _children;
    bool _marked;
	std::map<int, std::string> _inputLabelsMap;
    std::string _name;
	Hash* _hashValue;
	std::vector<Knob*> _knobsVector;
	KnobCallback* _knobsCB;
	Box2D _requestedBox; // composition of all the area requested by children
	NodeGui* _nodeGUI;
    QUndoStack* _undoStack;
private:
    void merge_frameRange(int otherFirstFrame,int otherLastFrame);
    void merge_info(bool forReal);
    void copy_info(Node* parent);
    
};
typedef Node* (*NodeBuilder)();



#endif // NODE_H
