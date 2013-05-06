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
#include "Core/diskcache.h"
#include "Core/displayFormat.h"
#include "Core/Box.h"
#include "Gui/GLViewer.h"

using namespace Powiter_Enums;



class Model;
class SettingsPanel;
class Hash;
class Knob;
class Knob_Callback;
class Node_ui;
class Node
{
public:
	class Info:public IntegerBox{
	
	public:
		Info(int first_frame,int last_frame,int ydirection,DisplayFormat full_size_format,ChannelMask _channels):
		  first_frame(first_frame),
			  last_frame(last_frame),
			  ydirection(ydirection),
			  full_size_format(full_size_format),
			  _channels(_channels)
		  {}
	    Info():first_frame(-1),last_frame(-1),ydirection(0),_channels(),full_size_format(){}
		void setYdirection(int direction){ydirection=direction;}
		int getYdirection(){return ydirection;}
		void set_full_size_format(DisplayFormat format){
            full_size_format=format;
                    }
		DisplayFormat& getFull_size_format(){return full_size_format;}
		bool is_similar(Info other);
		void firstFrame(int nb){first_frame=nb;}
		void lastFrame(int nb){last_frame=nb;}
		int firstFrame(){return first_frame;}
		int lastFrame(){return last_frame;}
		void add_to_channels(Channel z);
		void add_to_channels(ChannelMask &m);
		void remove_from_channels(Channel z);
		void remove_from_channels(ChannelMask &m);
		void set_channels(ChannelMask mask){_channels=mask;}	
		ChannelMask& channels(){return _channels;}
		bool black_outside(){return _black_outside;}
		void black_outside(bool bo){_black_outside=bo;}
        void rgbMode(bool m){_rgbMode=m;}
        bool rgbMode(){return _rgbMode;}
        
		
	private:
		int first_frame;
		int last_frame;
		int ydirection;
		bool _black_outside;
        bool _rgbMode;
		DisplayFormat full_size_format; // display window of the data, for the data window see x,y,range,offset parameters
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
	virtual void create_Knob_after_init();
	Knob_Callback* getKnobCallBack(){return knob_cb;}
	void setNodeUi(Node_ui* ui){node_gui=ui;}
	Node_ui* getNodeUi(){return node_gui;}
    /*============================*/

    /*Parents & children nodes related functions*/
    std::vector<Node*> getParents() const;
    std::vector<Node*> getChildren() const;
    void addChild(Node* child);
    void addParent(Node* parent);
    void removeChild(Node* child);
    void removeParent(Node* parent);
    /*============================*/

	/*Node infos*/
	Info* getInfo(){return _info;}
	void merge_frameRange(int otherFirstFrame,int otherLastFrame);
	void merge_info();
	void copy_info();
	void clear_info();
	ChannelMask& get_output_channels(){return output_channels;}
	ChannelMask& get_requested_channels(){return requested_channels;}
	IntegerBox& get_requested_box(){return requested_box;}
    int width(){return _info->getFull_size_format().w();}
    int height(){return _info->getFull_size_format().h();}
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
    std::map<int, char*> getInputLabels() const;
    virtual std::string setInputLabel(int inputNb);
    const char* getLabel(int inputNb) ;
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
    virtual const char* class_name();
    virtual const char* description();
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

	/*Readers only function*/
	virtual void setFileExtension(){}
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
	std::map<int, char*> inputLabels;
	QMutex* _mutex;
	QString name;
	Hash* hashValue; 
	std::vector<Knob*> knobs;
	Knob_Callback* knob_cb;
	IntegerBox requested_box; // composition of all the area requested by children
	ChannelMask requested_channels; // merge of all channels requested by children
	Node_ui* node_gui;
private:
	

};
typedef Node* (*NodeBuilder)(void*);



#endif // NODE_H
