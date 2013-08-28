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

#ifndef POWITER_GUI_KNOB_H_
#define POWITER_GUI_KNOB_H_

#include <vector>
#include <map>
#include <cassert>
#include <QtCore/QString>
#include <QUndoCommand>
#include <QSpinBox>
#include <QWidget>
#include <QtCore/QStringList>
#include <QLineEdit>

#include "Global/GlobalDefines.h"
#include "Engine/ChannelSet.h"
#include "Engine/Model.h"
#include "Engine/Singleton.h"
#include "Gui/FeedbackSpinBox.h"
#include "Gui/ComboBox.h"
#include "Gui/LineEdit.h"

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





class TabWidget;
class QHBoxLayout;
class QLabel;
class QVBoxLayout;
class QCheckBox;
class QGroupBox;
class SettingsPanel;
class Node;
class Knob;

class KnobCallback
{
public:
    KnobCallback(SettingsPanel* panel,Node* node);
    ~KnobCallback();


    void addKnob(Knob* knob){knobs.push_back(knob);}

    void initNodeKnobsVector();

    Node* getNode(){return node;}

    void createKnobDynamically();

    void removeAndDeleteKnob(Knob* knob);

    SettingsPanel* getSettingsPanel() const {return panel;}


    void setSettingsPanel(SettingsPanel* p) {panel = p;}

private:
    Node* node;
    std::vector<Knob*> knobs;
    SettingsPanel* panel;
};


//================================
class Knob:public QWidget
{
public:
    enum Knob_Flags{NONE=0x0,INVISIBLE=0x1,READ_ONLY=0x2};
    
    
    Knob(KnobCallback* cb,const std::string& description);
    
    virtual ~Knob();

    std::vector<U64> getValues(){return values;}
    
    KnobCallback* getCallBack(){return cb;}
    
    QHBoxLayout* getHorizontalLayout() const {return layout;}
    
    /*you can use this to provide a layout coming from another Knob.
     For example you can provide the layout of the Knob added previously thus
     stacking 2 knobs on the same line.*/
    void changeLayout(QHBoxLayout* newLayout);
    
    /*Set the spacing between items in the layout*/
    void setLayoutMargin(int pix);
    
    /*Must return the name of the knob. This name will be used by the KnobFactory
     to create an instance of this knob.*/
    virtual std::string name()=0;
    
    const std::string& getDescription() const {return _description;}
    
    //to be called on events functions to tell the engine to start processing
    //made public to allow other class that knob depends on to call validateEvent
    //but shouldn't be called by any other class.
    void validateEvent(bool initViewer);
    
    /*Calling this will tell the callback to delete this
     knob and remove it properly from the GUI.
     The knob will also be removed to the node's vector
     it belongs to.*/
    void enqueueForDeletion();
    
    
    virtual std::string serialize() const =0;
    
    virtual void restoreFromString(const std::string& str) =0;
    
    void pushUndoCommand(QUndoCommand* cmd);
    
protected:
    virtual void setValues()=0; // function to add the specific values of the knob to the values vector.
    
    KnobCallback *cb;
    QHBoxLayout* layout;
    std::vector<QWidget*> elements;
    std::vector<U64> values;
    std::string _description;
};

typedef unsigned int Knob_Mask;
std::vector<Knob::Knob_Flags> Knob_Mask_to_Knobs_Flags(Knob_Mask& m);


/*Class inheriting Knob, must have a function named BuildKnob with the following signature:
 Knob* BuildKnob(Knob_Callback* cb,QString& description,Knob_Mask flags); This function
 should in turn call a specific class-based static function with the appropriate param.
 E.G : static Knob* int_Knob(int* integer,Knob_Callback* cb,QString& description,Knob_Mask flags=0);
 and return a pointer to the knob. */
typedef Knob* (*KnobBuilder)(KnobCallback* cb,const std::string& description,Knob_Mask flags);



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

    static Knob* createKnob(const std::string& name, KnobCallback* callback, const std::string& description, Knob_Mask flags);
    
};

//================================
class FileQLineEdit;


class File_Knob:public Knob
{
    Q_OBJECT
    
public:
    
    static Knob* BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags);
    
    File_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags=0);
    virtual ~File_Knob(){}
    virtual void setValues();
    virtual std::string name(){return "InputFile";}
    
    void setPointer(QStringList* list){filesList = list;}
    
    void setFileNames(const QStringList& str);
    
    void setLineEditText(const std::string& str);
    
    std::string getLineEditText() const;
    
    QStringList* getFileNames(){return filesList;}
    
    
    virtual std::string serialize() const;
    
    virtual void restoreFromString(const std::string& str);
    
    public slots:
    void open_file();
    
signals:
    void filesSelected();
    
private:
    void updateLastOpened(const QString& str);
    QStringList* filesList;
    FileQLineEdit* _name;
    QString _lastOpened;
};

class FileCommand : public QUndoCommand{
    
public:
    
    FileCommand(File_Knob* knob,const QStringList& oldList,const QStringList& newList,QUndoCommand *parent = 0):QUndoCommand(parent),
    _oldList(oldList),
    _newList(newList),
    _knob(knob)
    {}
    virtual void undo();
    virtual void redo();
    virtual bool mergeWith(const QUndoCommand *command);
    
private:
    QStringList _oldList;
    QStringList _newList;
    File_Knob* _knob;
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
    static Knob* BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags);
    
    OutputFile_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags=0);
    virtual ~OutputFile_Knob(){}
    virtual void setValues();
    virtual std::string name(){return "OutputFile";}
    
    void setPointer(std::string* filename){str = filename;}
    
    std::string * getStr(){return str;}
    
    virtual std::string serialize() const;
    
    virtual void restoreFromString(const std::string& str);
    
    void updateLastOpened(const QString& str);
    
public slots:
    void open_file();
    void setStr(const QString& str);
signals:
    void filesSelected();
private:
    std::string *str; // TODO: why keep a pointer here?
    OutputFileQLineEdit* _name;
    QString _lastOpened;
};

class OutputFileQLineEdit:public LineEdit{
public:
    explicit OutputFileQLineEdit(OutputFile_Knob* knob);
    void keyPressEvent(QKeyEvent *);
private:
    OutputFile_Knob* knob;
};

class OutputFileCommand : public QUndoCommand{
    
public:
    
    OutputFileCommand(OutputFile_Knob* knob,const std::string& oldStr,const std::string& newStr,QUndoCommand *parent = 0):QUndoCommand(parent),
    _old(oldStr),
    _new(newStr),
    _knob(knob)
    {}
    virtual void undo();
    virtual void redo();
    virtual bool mergeWith(const QUndoCommand *command);
private:
    std::string _old;
    std::string _new;
    OutputFile_Knob* _knob;
};


//================================

class Int_Knob: public Knob
{
    Q_OBJECT
public:
    static Knob* BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags);
    
    virtual void setValues();
    virtual std::string name(){return "Int";}
    void setPointer(int* value){integer = value;}
    
    Int_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags=0);
    virtual ~Int_Knob(){}
    
    void setValue(int value);
    
    void setMaximum(int);
    void setMinimum(int);

    virtual std::string serialize() const;
    
    virtual void restoreFromString(const std::string& str);
    
    int getValue() const {
        assert(integer);
        return *integer;
    }
    
    public slots:
    void onValueChanged(double);
signals:
    void valueChanged(int);
private:
    int* integer;
    FeedBackSpinBox* box;
};

class IntCommand : public QUndoCommand{
    
public:
    
    IntCommand(Int_Knob* knob,int oldStr,int newStr,QUndoCommand *parent = 0):QUndoCommand(parent),
    _old(oldStr),
    _new(newStr),
    _knob(knob)
    {}
    virtual void undo();
    virtual void redo();
    virtual bool mergeWith(const QUndoCommand *command);
    
private:
    int _old;
    int _new;
    Int_Knob* _knob;
};
/******************/

class Int2D_Knob: public Knob
{
    Q_OBJECT
public:
    static Knob* BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags);
    
    virtual void setValues();
    virtual std::string name(){return "Int2D";}
    void setPointers(int* value1,int* value2){_value1 = value1;_value2= value2;}
    
    Int2D_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags=0);
    virtual ~Int2D_Knob(){}
    
    void setValue1(int value);
    void setValue2(int value);

    void setMaximum1(int);
    void setMinimum1(int);
    void setMaximum2(int);
    void setMinimum2(int);
    
    int getValue1() const {
        assert(_value1);
        return *_value1;
    }
    int getValue2() const {
        assert(_value2);
        return *_value2;
    }
    
    virtual std::string serialize() const;
    
    virtual void restoreFromString(const std::string& str);
    
    public slots:
    void onValue1Changed(double);
    void onValue2Changed(double);
signals:
    void value1Changed(int);
    void value2Changed(int);
private:
    int* _value1;
    int* _value2;
    FeedBackSpinBox* _box1;
    FeedBackSpinBox* _box2;
};

class Int2DCommand : public QUndoCommand{
    
public:
    
    Int2DCommand(Int2D_Knob* knob,int old1,int old2,int new1,int new2,QUndoCommand *parent = 0):QUndoCommand(parent),
    _old1(old1),
    _old2(old2),
    _new1(new1),
    _new2(new2),
    _knob(knob)
    {}
    virtual void undo();
    virtual void redo();
    virtual bool mergeWith(const QUndoCommand *command);
    
private:
    int _old1,_old2;
    int _new1,_new2;
    Int2D_Knob* _knob;
};

//================================
class Double_Knob: public Knob
{
    Q_OBJECT
    
    double *_value;
    FeedBackSpinBox* box;
public:
    static Knob* BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags);
    void setPointer(double* value){_value = value;}
    virtual void setValues();
    virtual std::string name(){return "Double";}
    Double_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags=0);
    
    virtual ~Double_Knob(){}

    void setMaximum(double);
    void setMinimum(double);
    
    void setIncrement(double);
    
    void setValue(double value);
    
    
    double getValue() const {
        assert(_value);
        return *_value;
    }

    virtual std::string serialize() const;
    
    virtual void restoreFromString(const std::string& str);
    
    public slots:
    void onValueChanged(double);
    
signals:
    void valueChanged(double);
};

class DoubleCommand : public QUndoCommand{
    
public:
    
    DoubleCommand(Double_Knob* knob,double oldStr,double newStr,QUndoCommand *parent = 0):QUndoCommand(parent),
    _old(oldStr),
    _new(newStr),
    _knob(knob)
    {}
    virtual void undo();
    virtual void redo();
    virtual bool mergeWith(const QUndoCommand *command);
    
private:
    double _old;
    double _new;
    Double_Knob* _knob;
};
/*************************/
class Double2D_Knob : public Knob{
    Q_OBJECT
    
    double *_value1;
    double *_value2;
    FeedBackSpinBox* _box1;
    FeedBackSpinBox* _box2;
public:
    static Knob* BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags);
    void setPointers(double* value1,double* value2){_value1 = value1; _value2 = value2;}
    virtual void setValues();
    virtual std::string name(){return "Double2D";}
    Double2D_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags=0);
    
    virtual ~Double2D_Knob(){}
    
    void setMaximum1(double);
    void setMinimum1(double);
    void setMaximum2(double);
    void setMinimum2(double);
    
    void setIncrement1(double);
    void setIncrement2(double);

    
    void setValue1(double value);
    void setValue2(double value);
    
    double getValue1() const {
        assert(_value1);
        return *_value1;
    }
    double getValue2() const {
        assert(_value2);
        return *_value2;
    }
    
    
    virtual std::string serialize() const;
    
    virtual void restoreFromString(const std::string& str);
    
    public slots:
    void onValue1Changed(double);
    void onValue2Changed(double);
    
signals:
    void value1Changed(double);
    void value2Changed(double);
};
class Double2DCommand : public QUndoCommand{
    
public:
    
    Double2DCommand(Double2D_Knob* knob,double old1,double old2,double new1,double new2,QUndoCommand *parent = 0):QUndoCommand(parent),
    _old1(old1),
    _old2(old2),
    _new1(new1),
    _new2(new2),
    _knob(knob)
    {}
    virtual void undo();
    virtual void redo();
    virtual bool mergeWith(const QUndoCommand *command);
    
private:
    double _old1,_old2;
    double _new1,_new2;
    Double2D_Knob* _knob;
};
//================================
class Bool_Knob:public Knob
{
	Q_OBJECT
public:
    
    static Knob* BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags);
    
    void setPointer(bool* val){_boolean = val;}
    
    Bool_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags=0);
    
	virtual void setValues();
    
    virtual std::string name(){return "Bool";}
    
    virtual std::string serialize() const;
    
    virtual void restoreFromString(const std::string& str);
    
    virtual ~Bool_Knob(){}
    
    bool isChecked() const {
        assert(_boolean);
        return *_boolean;
    }
    
public slots:
	void onToggle(bool b);
    
    void setChecked(bool b);

   
signals:
    void triggered(bool);
private:
	bool *_boolean;
	QCheckBox* checkbox;
};

class BoolCommand : public QUndoCommand{
    
public:
    
    BoolCommand(Bool_Knob* knob,bool oldStr,bool newStr,QUndoCommand *parent = 0):QUndoCommand(parent),
    _old(oldStr),
    _new(newStr),
    _knob(knob)
    {}
    virtual void undo();
    virtual void redo();
    virtual bool mergeWith(const QUndoCommand *command);
    
private:
    bool _old;
    bool _new;
    Bool_Knob* _knob;
};

//================================
class Button;
class Button_Knob : public Knob
{
    Q_OBJECT
public:
    
    static Knob* BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags);
    Button_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags=0);
    void connectButtonToSlot(QObject* object,const char* slot);
    
    virtual std::string name(){return "Button";}
    virtual void setValues(){}
    virtual ~Button_Knob(){}
    
    virtual std::string serialize() const;
    
    virtual void restoreFromString(const std::string& str);
    
    public slots:
    void onButtonPressed();
private:
    Button* button;
};

//================================
class ComboBox_Knob : public Knob
{
    Q_OBJECT
public:
    static Knob* BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags);
    ComboBox_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags=0);
    
    void populate(const std::vector<std::string>& entries);
    
    void setPointer(std::string* str);
    
    virtual void setValues();
    virtual ~ComboBox_Knob(){}
    
    virtual std::string name(){return "ComboBox";}

    
    virtual std::string serialize() const;
    
    virtual void restoreFromString(const std::string& str);
    
    const std::string activeEntry() const;
    
    int currentIndex() const;
signals:
    void entryChanged(int);
    
public slots:
    void onCurrentIndexChanged(int);
    void setCurrentItem(int index);
private:
    ComboBox* _comboBox;
    std::string* _currentItem; // TODO: why a pointer?
};
class ComboBoxCommand : public QUndoCommand{
    
public:
    
    ComboBoxCommand(ComboBox_Knob* knob,int oldStr,int newStr,QUndoCommand *parent = 0):QUndoCommand(parent),
    _old(oldStr),
    _new(newStr),
    _knob(knob)
    {}
    virtual void undo();
    virtual void redo();
    virtual bool mergeWith(const QUndoCommand *command);
    
private:
    int _old;
    int _new;
    ComboBox_Knob* _knob;
};

//=========================
class Separator_Knob : public Knob
{
public:
    static Knob* BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags);
    Separator_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags=0);
    
    virtual void setValues(){}
    
    virtual std::string name(){return "Separator";}

    virtual std::string serialize() const {return "";}
    
    virtual void restoreFromString(const std::string& str){(void)str;}
    
    virtual ~Separator_Knob(){}
private:
    QFrame* line;
};
/***************************/
class Group_Knob : public Knob{
    Q_OBJECT
public:
    static Knob* BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags);
    
    Group_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags=0);
    
    virtual void setValues(){}
    
    void addKnob(Knob* k);
    
    
    virtual std::string name(){return "Group";}
    
    
    virtual std::string serialize() const;
    
    virtual void restoreFromString(const std::string& str){(void)str;}
    
    virtual ~Group_Knob(){}
    public slots:
    void setChecked(bool b);

    
private:
    QGroupBox* _box;
    QVBoxLayout* _boxLayout;
    std::vector<Knob*> _knobs;
    
};
/******************************/
class RGBA_Knob : public Knob{
  Q_OBJECT
public:
    
    RGBA_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags=0);
    
    virtual void setValues();
    
    static Knob* BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags);
    
    virtual std::string name(){return "RGBA";}
    
    void setRGBA(double r,double g,double b,double a);
    
    virtual ~RGBA_Knob(){}
    
    void setPointers(double *r,double *g,double *b,double *a);
    
    void getColor(double *r,double *g,double *b,double *a);
    
    /*Must be called before setPointers*/
    void disablePermantlyAlpha();
    
    virtual std::string serialize() const;
    
    virtual void restoreFromString(const std::string& str);

signals:
    void colorChanged(QColor);
    
    public slots:
    void onRedValueChanged(double);
    void onGreenValueChanged(double);
    void onBlueValueChanged(double);
    void onAlphaValueChanged(double);
    
    void showColorDialog();
    
    
private:
    
    void updateLabel(const QColor& color);
    
    double* _r,*_g,*_b,*_a;
    
    QLabel* _rLabel;
    QLabel* _gLabel;
    QLabel* _bLabel;
    QLabel* _aLabel;
    
    FeedBackSpinBox* _rBox;
    FeedBackSpinBox* _gBox;
    FeedBackSpinBox* _bBox;
    FeedBackSpinBox* _aBox;
    
    QLabel* _colorLabel;
    Button* _colorDialogButton;
    
    bool _alphaEnabled;
};

class RGBACommand : public QUndoCommand{
    
public:
    
    RGBACommand(RGBA_Knob* knob,
                double oldr,double oldg,double oldb,double olda,
                double newr,double newg,double newb,double newa,
                QUndoCommand *parent = 0):QUndoCommand(parent),
    _oldr(oldr),
    _oldg(oldg),
    _oldb(oldb),
    _olda(olda),
    _newr(newr),
    _newg(newg),
    _newb(newb),
    _newa(newa),
    _knob(knob)
    {}
    virtual void undo();
    virtual void redo();
    virtual bool mergeWith(const QUndoCommand *command);
    
private:
    double _oldr,_oldg,_oldb,_olda;
    double _newr,_newg,_newb,_newa;
    RGBA_Knob* _knob;
};

/*****************************/
class Tab_Knob : public Knob{
  
public:
    typedef std::map<std::string, std::pair<QVBoxLayout*, std::vector<Knob*> > > KnobsTabMap;
    
    Tab_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags=0);
        
    static Knob* BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags);
    
    
    void addTab(const std::string& name);
    
    void addKnob(const std::string& tabName,Knob* k);
    
    virtual std::string name(){return "Tab";}

    
    virtual void setValues(){}

    virtual std::string serialize() const{return "";}
    
    virtual void restoreFromString(const std::string& str){(void)str;}
    
    ~Tab_Knob(){}
    
private:
    TabWidget* _tabWidget;
    KnobsTabMap _knobs;
};
/*****************************/
class String_Knob : public Knob{
    Q_OBJECT
public:
    
    static Knob* BuildKnob(KnobCallback* cb, const std::string& description, Knob_Mask flags);
    String_Knob(KnobCallback *cb, const std::string& description, Knob_Mask flags=0);
	virtual void setValues();
    
    virtual std::string name(){return "String";}
    void setPointer(std::string* val){_string = val;}
    virtual ~String_Knob(){}
    
    virtual std::string serialize() const;
    
    virtual void restoreFromString(const std::string& str);
    
    const std::string& getString() const{
        assert(_string);
        return *_string;
    }
    
    public slots:
    void onStringChanged(const QString& str);
    void setString(QString);
signals:
    void stringChanged(QString);
private:
    std::string *_string;
	LineEdit* _lineEdit;
};

class StringCommand : public QUndoCommand{
    
public:
    
    StringCommand(String_Knob* knob,const std::string& oldStr,const std::string& newStr,QUndoCommand *parent = 0):QUndoCommand(parent),
    _old(oldStr),
    _new(newStr),
    _knob(knob)
    {}
    virtual void undo();
    virtual void redo();
    virtual bool mergeWith(const QUndoCommand *command);
    
private:
    std::string _old;
    std::string _new;
    String_Knob* _knob;
};


/*****************************/


#endif // POWITER_GUI_KNOB_H_
