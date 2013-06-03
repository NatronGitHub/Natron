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
#include "Core/row.h"
#include "Core/viewercache.h"
#include "Core/displayFormat.h"
#include "Core/Box.h"
#include "Gui/GLViewer.h"

using namespace Powiter_Enums;



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
			  _channels(channels)
		  {}
	    Info():_firstFrame(-1),_lastFrame(-1),_ydirection(0),_channels(),_displayWindow(){}
		void setYdirection(int direction){_ydirection=direction;}
		int getYdirection(){return _ydirection;}
		void setDisplayWindow(Format format){
            _displayWindow=format;
                    }
		Format& getDisplayWindow(){return _displayWindow;}
		Box2D& getDataWindow(){return dynamic_cast<Box2D&>(*this);}
		bool operator==( Node::Info &other);
		void firstFrame(int nb){_firstFrame=nb;}
		void lastFrame(int nb){_lastFrame=nb;}
		int firstFrame(){return _firstFrame;}
		int lastFrame(){return _lastFrame;}
		void add_to_channels(Channel z);
		void add_to_channels(ChannelMask &m);
		void turnOffChannel(Channel z);
		void turnOffChannels(ChannelMask &m);
		void set_channels(ChannelMask mask){_channels=mask;}	
		ChannelMask& channels(){return _channels;}
		bool blackOutside(){return _blackOutside;}
		void blackOutside(bool bo){_blackOutside=bo;}
        void rgbMode(bool m){_rgbMode=m;}
        bool rgbMode(){return _rgbMode;}
        
		
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
    void computeTreeHash(std::vector<char*> &alreadyComputedHash);
    bool hashChanged();
    /*============================*/

    /*Knobs related functions*/
    std::vector<Knob*> getKnobs() const;
    void add_knob_to_vector(Knob* knob);
    virtual void initKnobs(Knob_Callback *cb);
	virtual void createKnobDynamically();
	Knob_Callback* getKnobCallBack(){return knob_cb;}
	void setNodeUi(NodeGui* ui){node_gui=ui;}
	NodeGui* getNodeUi(){return node_gui;}
    /*============================*/

    /*Parents & children nodes related functions*/
    std::vector<Node*> getParents() const;
    std::vector<Node*> getChildren() const;
    void addChild(Node* child);
    void addParent(Node* parent);
    void removeChild(Node* child);
    void removeParent(Node* parent);
    /*============================*/

    /*DAG related*/
    void setMarked(bool mark){_marked = mark;}
    bool isMarked(){return _marked;}
    /*============================*/
    
	/*Node infos*/
	Info* getInfo(){return _info;}
	void merge_frameRange(int otherFirstFrame,int otherLastFrame);
	void merge_info();
	void copy_info();
	void clear_info();
	ChannelMask& getOutputChannels(){return output_channels;}
	ChannelMask& getRequestedChannels(){return requested_channels;}
	Box2D& get_requested_box(){return requested_box;}
    int width(){return _info->getDisplayWindow().w();}
    int height(){return _info->getDisplayWindow().h();}
	/*================================*/
    

    /*OutputNB related functions*/
    virtual bool hasOutput();
    int getFreeOutputNb() const;
    void decrementFreeOutputNb();
    void incrementFreeOutputNb();
    virtual void setOutputNb();
    /*============================*/

    /*Node type related functions*/
    virtual bool isInputNode();
    virtual bool isOutputNode();
    /*============================*/

    /*Node Input related functions*/
    void _inputs();
    virtual int inputs();
    int getInputsNb() const;
    const std::map<int, std::string>& getInputLabels() const;
    virtual std::string setInputLabel(int inputNb);
    std::string getLabel(int inputNb) ;
    void _setLabels();
    void initInputsLabels();
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
    virtual void validate(bool first_time);
    virtual void request(int y,int top,int offset, int range,ChannelMask channels);
    virtual void engine(int y,int offset,int range,ChannelMask channels,Row* out);
	
    /*============================*/
    
    /*overlay support:
     *Just overload this function in your operator.
     *No need to include any openGL related header.
     *The coordinate space is the defined by the displayWindow
     *(i.e: (0,0) = bottomLeft and  width() and height() being
     * respectivly the width and height of the frame.)
     */
    virtual void drawOverlay(){}

	
	/*============================*/
    friend ostream& operator<< (ostream &out, Node &Node);
    
protected:

	void set_output_channels(ChannelMask mask){output_channels=mask;} // set the output_channels, the channels in output will be the intersection of output_channels
	// and _info.channels()

	virtual void in_channels(int inputNb,ChannelMask &mask){};
	virtual void _validate(bool first_time);
    virtual void _request(int y,int top,int offset,int range,ChannelMask channels);
   
	Info* _info; // contains all the info for this operator:the channels on which it is defined,the area of the image, the image format etc...this is set by validate
	ChannelMask output_channels; // the channels that the operator chooses to output from the ones among _info.channels(), by default Mask_all
	int inputNb;
	int freeOutputNb;
	std::vector<Node*> parents;
	std::vector<Node*> children;
    bool _marked;
	std::map<int, std::string> inputLabels;
	QMutex* _mutex;
	QString name;
	Hash* hashValue; 
	std::vector<Knob*> knobs;
	Knob_Callback* knob_cb;
	Box2D requested_box; // composition of all the area requested by children
	ChannelMask requested_channels; // merge of all channels requested by children
	NodeGui* node_gui;
private:
	

};
typedef Node* (*NodeBuilder)(void*);



#endif // NODE_H
