//  Powiter
//
//  Created by Alexandre Gauthier-Foichat on 06/12
//  Copyright (c) 2013 Alexandre Gauthier-Foichat. All rights reserved.
//  contact: immarespond at gmail dot com
#ifndef KNOB_H
#define KNOB_H
#include <vector>
#include <QtCore/QString>
#include <QtWidgets/QWidget>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QCheckBox>
#include <QtCore/QStringList>
#include "Superviser/powiterFn.h"
#include "Core/channels.h"
#ifdef __POWITER_WIN32__
typedef unsigned _int32 uint32_t;
#elif defined(__POWITER_UNIX__)
#include "stdint.h"
#endif

/*Implementation of the usual settings knobs used by the nodes. For instance an int_knob might be useful to input a specific
 parameter for a given operation on the image, etc...This file provide utilities to build those knobs without worrying with
 the GUI stuff. If you want say an int_knob for your operator, then just call Knob::int_knob(...) in the function initKnobs(..)
 that you override. WARNING: ***YOU MUST CALL THE BASE IMPLEMENTATION OF INITKNOBS(...) AT THE END OF YOUR OVERRIDEN INITKNOBS().
 I.E: Node::initKnobs(...)***
 */

/*To add a knob dynamically ( after initKnobs call, for example a result knob), then overload the function create_Knob_after_init()
 Do the same that in initKnobs, and then call the base implementation :  Node::create_Knob_after_init(). To get the Knob_Callback
 object, call getKnobCallBack()
 */

using namespace Powiter_Enums;



typedef unsigned int Knob_Mask;
std::vector<Knob_Flags> Knob_Mask_to_Knobs_Flags(Knob_Mask& m);



class SettingsPanel;
class Node;
class Knob_Callback;


//================================
class Knob:public QWidget
{
    
public:
    Knob(Knob_Types type,Knob_Callback* cb);
    std::vector<U64> getValues(){return values;}
    virtual ~Knob();
    static Knob* file_Knob(Knob_Callback* cb,QString& description,QStringList& filePath,Knob_Mask flags=0);
    static Knob* channels_Knob(ChannelMask* channels,Knob_Callback* cb,Knob_Mask flags=0);
    static Knob* int_Knob(int* integer,Knob_Callback* cb,QString& description,Knob_Mask flags=0);
    static Knob* float_Knob(float* floating,Knob_Callback* cb,QString& description,Knob_Mask flags=0);
    static Knob* string_Knob(QString* str,Knob_Callback* cb,QString& description,Knob_Mask flags=0);
    static Knob* bool_Knob(bool& boolean,Knob_Callback* cb,QString& description,Knob_Mask flags=0);
	Knob_Callback* getCallBack(){return cb;}
    Knob_Types getType() const {return _type;}
    
    //to be called on events functions to tell the engine to start processing
    //made public to allow other class that knob depends on to call validateEvent
    //but shouldn't be called by any other class.
    void validateEvent();
protected:
    virtual void setValues()=0; // function to add the specific values of the knob to the values vector.
    
    Knob_Callback *cb;
    QHBoxLayout* layout;
    std::vector<QWidget*> elements;
    std::vector<U64> values;
    Knob_Types _type;
    
private:
    
    
};
//================================
class FileQLineEdit;


class File_Knob:public Knob
{
    Q_OBJECT
    
public:
    File_Knob(Knob_Callback *cb,QString& description,QStringList& filePath,Knob_Mask flags=0);
    virtual void setValues();
    void setStr(QStringList str){
		for(int i =0;i< str.size();i++){
            
            QString el = str[i];
			
			this->str.append(el);
		}
	}
    QStringList& getStr(){return str;}
    public slots:
    void open_file();
    
private:
    void updateLastOpened(QString str);
    QStringList& str;
    FileQLineEdit* name;
    QString _lastOpened;
};
//the following class is necessary for the File_Knob Class
class FileQLineEdit:public QLineEdit{
    
public:
    FileQLineEdit(File_Knob* knob);
    void keyPressEvent(QKeyEvent *);
private:
    File_Knob* knob;
};

//================================
class Channels_Knob:public Knob
{
public:
    virtual void setValues();
    Channels_Knob(ChannelMask *channels, Knob_Callback *cb,Knob_Mask flags=0);
private:
    ChannelMask* channels;
};
//================================
class IntQSpinBox;

class Int_Knob:public Knob
{
public:
    virtual void setValues();
    Int_Knob(int* integer,Knob_Callback *cb,QString& description,Knob_Mask flags=0);
    void setInteger(int* integer){this->integer=integer;}
private:
    int* integer;
    IntQSpinBox* box;
};

//the following class is necessary for the Int_Knob Class
class IntQSpinBox:public QSpinBox{
public:
    IntQSpinBox(Int_Knob* knob);
    void keyPressEvent(QKeyEvent *event);
private:
    Int_Knob* knob;
};

//================================
class Float_Knob: public Knob
{
public:
    virtual void setValues();
    Float_Knob(float* floating,Knob_Callback *cb,QString& description,Knob_Mask flags=0);
    
};
//================================
class String_Knob:public Knob
{
public:
    virtual void setValues();
    String_Knob(QString* str,Knob_Callback *cb,QString& description,Knob_Mask flags=0);
    
};
//================================
class Bool_Knob:public Knob
{
	Q_OBJECT
public:
    
    Bool_Knob(bool& boolean,Knob_Callback *cb,QString& description,Knob_Mask flags=0);
	virtual void setValues();
    public slots:
	void change_checkBox(int checkBoxState);
private:
	bool boolean;
	QCheckBox* checkbox;
};

#endif // KNOB_H
