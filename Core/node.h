//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef NODE_H
#define NODE_H

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <iostream>
#include <cstdio>
#include <map>
#include <QtCore/QString>
#include "Core/channels.h"
#include "Superviser/powiterFn.h"
#include "Core/displayFormat.h"

class Row;
class InputRow;
class Model;
class SettingsPanel;
class Hash;
class Knob;
class Knob_Callback;
class NodeGui;
class Node
{
public:
	/*Per-node infos. This class is used to pass infos along the graph
	 *and to know what we can request from a node.*/
	class Info:public Box2D{
	public:
		Info(int first_frame,int last_frame,int ydirection,Format full_size_format,ChannelMask channels):
		  _firstFrame(first_frame),
			  _lastFrame(last_frame),
			  _ydirection(ydirection),
			  _displayWindow(full_size_format),
			  _channels(channels),
            _rgbMode(true),
        _blackOutside(false)
		  {}
	    Info():_firstFrame(-1),_lastFrame(-1),_ydirection(0),_channels(),_displayWindow(),_rgbMode(true),_blackOutside(false){}
		void setYdirection(int direction){_ydirection=direction;}
		int getYdirection(){return _ydirection;}
		void setDisplayWindow(Format format){_displayWindow=format;}
		const Format& getDisplayWindow(){return _displayWindow;}
		const Box2D& getDataWindow(){return dynamic_cast<const Box2D&>(*this);}
		bool operator==( Node::Info &other);
        void operator=(const Node::Info &other);
		void firstFrame(int nb){_firstFrame=nb;}
		void lastFrame(int nb){_lastFrame=nb;}
		int firstFrame(){return _firstFrame;}
		int lastFrame(){return _lastFrame;}
		void setChannels(ChannelMask mask){_channels=mask;}	
		ChannelMask& channelsRef(){return _channels;}
		const ChannelMask& channels(){return _channels;}
		bool blackOutside(){return _blackOutside;}
		void blackOutside(bool bo){_blackOutside=bo;}
        void rgbMode(bool m){_rgbMode=m;}
        bool rgbMode(){return _rgbMode;}
        
        void reset();
        
	private:
		int _firstFrame;
		int _lastFrame;
		int _ydirection;
		bool _blackOutside;
        bool _rgbMode;
		Format _displayWindow; // display window of the data, for the data window see x,y,range,offset parameters
		ChannelMask _channels; // all channels defined by the current Node ( that are allocated)
	};
    


    /*CONSTRUCTORS AND DESTRUCTORS*/
    Node(const Node& ref);
    Node(Node* ptr);
    virtual ~Node();
    /*============================*/

    /*Hash related functions*/
    Hash* getHash() const;
    void computeTreeHash(std::vector<std::string> &alreadyComputedHash);
    bool hashChanged();
    /*============================*/

    /*Knobs related functions*/
    const std::vector<Knob*>& getKnobs() const;
    void addToKnobVector(Knob* knob);
    
    /*Do not call this function. It is used
     internally by the Knob_Callback.*/
    void removeKnob(Knob* knob);
    virtual void initKnobs(Knob_Callback *cb);
	virtual void createKnobDynamically();
	Knob_Callback* getKnobCallBack(){return _knobsCB;}
	void setNodeUi(NodeGui* ui){_nodeGUI=ui;}
	NodeGui* getNodeUi(){return _nodeGUI;}
    /*============================*/

    /*Parents & children nodes related functions*/
    const std::vector<Node*>& getParents() const;
    const std::vector<Node*>& getChildren() const;
    void addChild(Node* child);
    void addParent(Node* parent);
    void removeChild(Node* child);
    void removeParent(Node* parent);
    void removeFromParents();
    void removeFromChildren();
    /*============================*/

    /*DAG related*/
    void setMarked(bool mark){_marked = mark;}
    bool isMarked(){return _marked;}
    /*============================*/
    
	/*Node infos*/
	Info* getInfo(){return _info;}
	void merge_frameRange(int otherFirstFrame,int otherLastFrame);
	void merge_info(bool forReal);
	void copy_info(Node* parent,bool forReal);
	void clear_info();
	ChannelMask& getOutputChannels(){return _outputChannels;}
	ChannelMask& getRequestedChannels(){return _requestedChannels;}
	Box2D& getRequestedBox(){return _requestedBox;}
    int width(){return _info->getDisplayWindow().w();}
    int height(){return _info->getDisplayWindow().h();}
	/*================================*/
    

    /*OutputNB related functions*/
    int getFreeOutputCount() const;
    void releaseSocket();
    void lockSocket();
    virtual void setSocketCount();
    /*============================*/

    /*Node type related functions*/
    virtual bool isInputNode();
    virtual bool isOutputNode();
    /*============================*/

    /*Node Input related functions*/
    void initializeInputs();
    virtual int maximumInputs();
    virtual int minimumInputs();
    Node* input(int index);
    const std::map<int, std::string>& getInputLabels() const;
    virtual std::string setInputLabel(int inputNb);
    std::string getLabel(int inputNb) ;
    void applyLabelsToInputs();
    void initInputLabelsMap();
    /*============================*/

    

    /*node name related functions*/
    QString getName();
    void setName(QString name);
    /*============================*/

    /*Node mutex related functions*/
    QMutex* getMutex() const;
    void setMutex(QMutex* m){this->_mutex=m;}
    /*============================*/

    /*Node utility functions*/
    virtual std::string className();
    virtual std::string description();
    /*============================*/

    /*Calculations related functions*/
    virtual void validate(bool forReal);
    virtual void request(ChannelMask channels);
    virtual void engine(int y,int offset,int range,ChannelMask channels,Row* out){}
	
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
     otherwise engine() gets called. You should never call get() with keepCached on.
     This parameter is only used by the Interest class internally.
     */
    void get(int y,int x,int r,ChannelSet channels,InputRow& row,bool keepCached = false);
    
    /*Returns true if the node will cache rows in the node cache.
     Otherwise results will not be cached.*/
    virtual bool cacheData()=0;

	/*============================*/
    friend ostream& operator<< (ostream &out, Node &Node);
    
protected:

	void setOutputChannels(ChannelMask mask){_outputChannels=mask;} // set the output_channels, the channels in output will be the intersection of output_channels
	// and _info.channels()

	virtual void in_channels(int inputNb,ChannelMask &mask){}
	virtual void _validate(bool forReal);
    virtual void _request(ChannelMask channels);
   
	Info* _info; // contains all the info for this operator:the channels on which it is defined,the area of the image, the image format etc...this is set by validate
	ChannelMask _outputChannels; // the channels that the operator outputs from the ones among _info.channels(), by default Mask_all
	int _freeSocketCount;
	std::vector<Node*> _parents;
	std::vector<Node*> _children;
    bool _marked;
	std::map<int, std::string> _inputLabelsMap;
	QMutex* _mutex;
	QString _name;
	Hash* _hashValue; 
	std::vector<Knob*> _knobsVector;
	Knob_Callback* _knobsCB;
	Box2D _requestedBox; // composition of all the area requested by children
	ChannelMask _requestedChannels; // merge of all channels requested by children
	NodeGui* _nodeGUI;
private:
	

};
typedef Node* (*NodeBuilder)(void*);



#endif // NODE_H
