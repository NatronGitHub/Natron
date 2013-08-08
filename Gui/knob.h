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

 

 



#ifndef KNOB_H
#define KNOB_H
#include <vector>
#include <map>
#include <QtCore/QString>
#include <QSpinBox>
#include <QWidget>
#include <QtCore/QStringList>
#include <QLineEdit>
#include "Superviser/powiterFn.h"
#include "Core/channels.h"
#include "Core/model.h"
#include "Core/singleton.h"
#include "Gui/FeedbackSpinBox.h"
#include "Gui/comboBox.h"
#include "Gui/lineEdit.h"
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






class QHBoxLayout;
class QCheckBox;
class SettingsPanel;
class Node;
class Knob_Callback;


//================================
class Knob:public QWidget
{
public:
    enum Knob_Flags{NONE=0x0,INVISIBLE=0x1,READ_ONLY=0x2};
    
    
    Knob(Knob_Callback* cb);
    virtual ~Knob();

    std::vector<U64> getValues(){return values;}
	Knob_Callback* getCallBack(){return cb;}
    
    /*Must return the name of the knob. This name will be used by the KnobFactory
     to create an instance of this knob.*/
    virtual std::string name()=0;
    
    //to be called on events functions to tell the engine to start processing
    //made public to allow other class that knob depends on to call validateEvent
    //but shouldn't be called by any other class.
    void validateEvent(bool initViewer);
    
    /*Calling this will tell the callback to delete this
     knob and remove it properly from the GUI.
     The knob will also be removed to the node's vector
     it belongs to.*/
    void enqueueForDeletion();
    
protected:
    virtual void setValues()=0; // function to add the specific values of the knob to the values vector.
    
    Knob_Callback *cb;
    QHBoxLayout* layout;
    std::vector<QWidget*> elements;
    std::vector<U64> values;
};

typedef unsigned int Knob_Mask;
std::vector<Knob::Knob_Flags> Knob_Mask_to_Knobs_Flags(Knob_Mask& m);


/*Class inheriting Knob, must have a function named BuildKnob with the following signature:
 Knob* BuildKnob(Knob_Callback* cb,QString& description,Knob_Mask flags); This function
 should in turn call a specific class-based static function with the appropriate param.
 E.G : static Knob* int_Knob(int* integer,Knob_Callback* cb,QString& description,Knob_Mask flags=0);
 and return a pointer to the knob. */
typedef Knob* (*KnobBuilder)(Knob_Callback* cb,const std::string& description,Knob_Mask flags);



class KnobFactory : public Singleton<KnobFactory>{
    
    std::map<std::string,PluginID*> _loadedKnobs;
    
    void loadKnobPlugins();
    
    void loadBultinKnobs();
    
public:
    
    
    KnobFactory();
    
    ~KnobFactory();
    
    const std::map<std::string,PluginID*>& getLoadedKnobs(){return _loadedKnobs;}
    
    /*Calls the unique instance of the KnobFactory and
     calls the appropriate pointer to function to create a knob.*/

    static Knob* createKnob(const std::string& name, Knob_Callback* callback, const std::string& description, Knob_Mask flags);
    
};

//================================
class FileQLineEdit;


class File_Knob:public Knob
{
    Q_OBJECT
    
public:
    
    static Knob* BuildKnob(Knob_Callback* cb, const std::string& description, Knob_Mask flags);
    
    File_Knob(Knob_Callback *cb, const std::string& description, Knob_Mask flags=0);
    virtual ~File_Knob(){}
    virtual void setValues();
    virtual std::string name(){return "InputFile";}
    
    void setPointer(QStringList* list){str = list;}
    void setStr(QStringList& str){
		for(int i =0;i< str.size();i++){
            
            QString el = str[i];
			
			this->str->append(el);
		}
	}
    QStringList* getStr(){return str;}
    public slots:
    void open_file();
    
private:
    void updateLastOpened(QString str);
    QStringList* str;
    FileQLineEdit* _name;
    QString _lastOpened;
};
//the following class is necessary for the File_Knob Class
class FileQLineEdit:public LineEdit{
    
public:
    FileQLineEdit(File_Knob* knob);
    void keyPressEvent(QKeyEvent *);
private:
    File_Knob* knob;
};
//================================
class OutputFileQLineEdit;
class OutputFile_Knob:public Knob
{
    Q_OBJECT
public:
    static Knob* BuildKnob(Knob_Callback* cb, const std::string& description, Knob_Mask flags);
    
    OutputFile_Knob(Knob_Callback *cb, const std::string& description, Knob_Mask flags=0);
    virtual ~OutputFile_Knob(){}
    virtual void setValues();
    virtual std::string name(){return "OutputFile";}
    
    void setPointer(std::string* filename){str = filename;}
    
    std::string * getStr(){return str;}
public slots:
    void open_file();
    void setStr(const QString& str){
		*(this->str) = str.toStdString();
	}
private:
    std::string *str; // TODO: why keep a pointer here?
    OutputFileQLineEdit* _name;
};

class OutputFileQLineEdit:public LineEdit{
public:
    OutputFileQLineEdit(OutputFile_Knob* knob);
    void keyPressEvent(QKeyEvent *);
private:
    OutputFile_Knob* knob;
};
//================================
class IntQSpinBox;

class Int_Knob:public Knob
{
public:
    static Knob* BuildKnob(Knob_Callback* cb, const std::string& description, Knob_Mask flags);
    
    virtual void setValues();
    virtual std::string name(){return "Int";}
    void setPointer(int* value){integer = value;}
    
    Int_Knob(Knob_Callback *cb, const std::string& description, Knob_Mask flags=0);
    virtual ~Int_Knob(){}
    void setInteger(int& value){*integer = value;}
private:
    int* integer;
    IntQSpinBox* box;
};

//the following class is necessary for the Int_Knob Class
class IntQSpinBox:public FeedBackSpinBox{
public:
    IntQSpinBox(Int_Knob* knob,QWidget* parent = 0);
    virtual void keyPressEvent(QKeyEvent *event);
private:
    Int_Knob* knob;
};

//================================
class DoubleQSpinBox;
class Double_Knob: public Knob
{
    double *_value;
    DoubleQSpinBox* box;
public:
    static Knob* BuildKnob(Knob_Callback* cb, const std::string& description, Knob_Mask flags);
    void setPointer(double* value){_value = value;}
    virtual void setValues();
    virtual std::string name(){return "Double";}
    Double_Knob(Knob_Callback *cb, const std::string& description, Knob_Mask flags=0);
    
    virtual ~Double_Knob(){}
    void setDouble(double& value){*_value = value;}
    
};

//the following class is necessary for the Int_Knob Class
class DoubleQSpinBox:public FeedBackSpinBox{
public:
    DoubleQSpinBox(Double_Knob* knob,QWidget* parent = 0);
    virtual void keyPressEvent(QKeyEvent *event);
private:
    Double_Knob* knob;
};
//================================
class String_Knob:public Knob
{
    std::string* _string;
    
public:
    static Knob* BuildKnob(Knob_Callback* cb, const std::string& description, Knob_Mask flags);
    void setPointer(std::string* str){_string = str;}
    virtual void setValues();
    virtual std::string name(){return "String";}
    String_Knob(Knob_Callback *cb, const std::string& description, Knob_Mask flags=0);
    virtual ~String_Knob(){}
    
};
//================================
class Bool_Knob:public Knob
{
	Q_OBJECT
public:
    
    static Knob* BuildKnob(Knob_Callback* cb, const std::string& description, Knob_Mask flags);
    void setPointer(bool* val){_boolean = val;}
    Bool_Knob(Knob_Callback *cb, const std::string& description, Knob_Mask flags=0);
	virtual void setValues();
    virtual std::string name(){return "Bool";}
    virtual ~Bool_Knob(){}
    public slots:
	void change_checkBox(int checkBoxState);
private:
	bool *_boolean;
	QCheckBox* checkbox;
};
//================================
class Button;
class Button_Knob : public Knob
{
    Q_OBJECT
public:
    
    static Knob* BuildKnob(Knob_Callback* cb, const std::string& description, Knob_Mask flags);
    Button_Knob(Knob_Callback *cb, const std::string& description, Knob_Mask flags=0);
    void connectButtonToSlot(QObject* object,const char* slot);
    
    virtual std::string name(){return "Button";}
    virtual void setValues(){}
    virtual ~Button_Knob(){}
    
    
private:
    Button* button;
};

//================================
class ComboBox_Knob : public Knob
{
    Q_OBJECT
public:
    static Knob* BuildKnob(Knob_Callback* cb, const std::string& description, Knob_Mask flags);
    ComboBox_Knob(Knob_Callback *cb, const std::string& description, Knob_Mask flags=0);
    
    void populate(const std::vector<std::string>& entries);
    
    void setPointer(std::string* str);
    
    virtual void setValues();
    virtual ~ComboBox_Knob(){}
    
    virtual std::string name(){return "ComboBox";}

signals:
    void entryChanged(std::string&);
    
public slots:
    void setCurrentItem(const QString&);
private:
    ComboBox* _comboBox;
    std::string* _currentItem; // TODO: why a pointer?
};

//=========================
class Separator_Knob : public Knob
{
public:
    static Knob* BuildKnob(Knob_Callback* cb, const std::string& description, Knob_Mask flags);
    Separator_Knob(Knob_Callback *cb, const std::string& description, Knob_Mask flags=0);
    
    virtual void setValues(){}
    
    virtual std::string name(){return "Separator";}

    
    virtual ~Separator_Knob(){}
private:
    QFrame* line;
};

#endif // KNOB_H
