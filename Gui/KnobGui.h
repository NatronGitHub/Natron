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
#include <QtCore/QStringList>
#include <QUndoCommand>
#include <QLineEdit>
#include <QLabel>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"

#include "Engine/Singleton.h"
#include "Engine/Knob.h"

class TabWidget;
class QCheckBox;
class Node;
class LineEdit;
class Button;
class SpinBox;
class ComboBox;
class QFrame;
class QLabel;
class QGridLayout;
class Knob;
class QVBoxLayout;
class QHBoxLayout;
class ScaleSlider;
class QGridLayout;
class KnobGui : public QObject
{
    Q_OBJECT
    
    
public:
    
    friend class KnobUndoCommand;
    
    KnobGui(Knob* knob);
    
    virtual ~KnobGui();
        
    bool triggerNewLine() const { return _triggerNewLine; }
    
    void turnOffNewLine() { _triggerNewLine = false; }
    
    const Knob* getKnob() const { return _knob; }
    
    /*Set the spacing between items in the layout*/
    void setSpacingBetweenItems(int spacing){ _spacingBetweenItems = spacing; }
    
    int getSpacingBetweenItems() const { return _spacingBetweenItems; }
    
    void createGUI(QGridLayout* layout,int row){
        createWidget(layout, row);
        _widgetCreated = true;
        updateGUI(_knob->getValueAsVariant());
        setEnabled(_knob->isEnabled());
        setVisible(_knob->isVisible());
    }
    
    void moveToLayout(QVBoxLayout* layout);
    
    /*Used by Tab_KnobGui to insert already existing widgets
     in a tab.*/
    virtual void addToLayout(QHBoxLayout* layout)=0;
    
    
    void pushUndoCommand(QUndoCommand* cmd);
    
    bool hasWidgetBeenCreated() const {return _widgetCreated;}
    
    virtual void hide() =0;
    
    virtual void show() =0;
    
public slots:
    /*Called when the value held by the knob is changed internally.
     This should in turn update the GUI but not emit the valueChanged()
     signal.*/
    void onInternalValueChanged(const Variant& variant){
        if(_widgetCreated)
            updateGUI(variant);
    }
    
    virtual void setVisible(bool b){
        if(b){
            show();
        }else{
            hide();
        }
    }
    
    virtual void setEnabled(bool b) =0;
    
signals:
    
    /*Must be emitted when a value is changed by the user or by
     an external source.*/
    void valueChanged(const Variant& variant);
    
protected:
    /*Must create the GUI and insert it in the grid layout at the index "row".*/
    virtual void createWidget(QGridLayout* layout,int row)=0;
    
   
    /*Called by the onInternalValueChanged slot. This should update
     the widget to reflect the new internal value held by variant.*/
    virtual void updateGUI(const Variant& variant)=0;
    
    Knob* _knob;
    
    

    
private:
    
    /*This function is used by KnobUndoCommand. Calling this in a onInternalValueChanged/valueChanged
     signal/slot sequence can cause an infinite loop.*/
    void setValue(const Variant& variant){
        updateGUI(variant);
        emit valueChanged(variant);
    }
    
    bool _triggerNewLine;
    int _spacingBetweenItems;
    bool _widgetCreated;
};

//================================
class File_KnobGui:public KnobGui
{
    Q_OBJECT
    
public:
    
    static KnobGui* BuildKnobGui(Knob* knob){ return new File_KnobGui(knob); }
    
    File_KnobGui(Knob* knob);

    virtual ~File_KnobGui(){}
            
    virtual void createWidget(QGridLayout* layout,int row);
    
    virtual void hide();
    
    virtual void show();
    
    virtual void setEnabled(bool b){(void)b;}
    
    virtual void addToLayout(QHBoxLayout* layout);
    
public slots:
    void open_file();
    void onReturnPressed();
    

    
protected:
    
    virtual void updateGUI(const Variant& variant);
    
private:
    
    void updateLastOpened(const QString& str);
    
    
    LineEdit* _lineEdit;
    QLabel* _descriptionLabel;
    Button* _openFileButton;
    
    QString _lastOpened;
};


//================================
class OutputFile_KnobGui : public KnobGui
{
    Q_OBJECT
public:
    
    static KnobGui* BuildKnobGui(Knob* knob){ return new OutputFile_KnobGui(knob); }
    
    OutputFile_KnobGui(Knob* knob);
    
    virtual ~OutputFile_KnobGui(){}
    
    virtual void createWidget(QGridLayout* layout,int row);
    
    virtual void hide();
    
    virtual void show();
    
    virtual void setEnabled(bool b){(void)b;}
    
    virtual void addToLayout(QHBoxLayout* layout);
    
public slots:
    void open_file();
    void onReturnPressed();

    
protected:
    
    virtual void updateGUI(const Variant& variant);
    
private:
    void updateLastOpened(const QString& str);

    
    LineEdit* _lineEdit;
    QLabel* _descriptionLabel;
    Button* _openFileButton;
    QString _lastOpened;
};




//================================

class Int_KnobGui : public KnobGui
{
    Q_OBJECT
public:
    
    static KnobGui* BuildKnobGui(Knob* knob){ return new Int_KnobGui(knob); }

    
    Int_KnobGui(Knob* knob):KnobGui(knob),_slider(0){}
    
    virtual ~Int_KnobGui(){}
    
    virtual void createWidget(QGridLayout* layout,int row);
    
    void setMaximum(int);
    void setMinimum(int);
    
    virtual void hide();
    
    virtual void show();
    
    virtual void setEnabled(bool b);
    
    virtual void addToLayout(QHBoxLayout* layout);
    
public slots:
    void onSpinBoxValueChanged();
    void onSliderValueChanged(double);
 
protected:
    
    virtual void updateGUI(const Variant& variant);
    
private:
    std::vector<std::pair<SpinBox*,QLabel*> > _spinBoxes;
    QLabel* _descriptionLabel;
    ScaleSlider* _slider;

};

//================================
class Bool_KnobGui :public KnobGui
{
	Q_OBJECT
public:
    
    static KnobGui* BuildKnobGui(Knob* knob){ return new Bool_KnobGui(knob); }

    
    Bool_KnobGui(Knob* knob):KnobGui(knob){}

    virtual ~Bool_KnobGui(){}
    
    virtual void createWidget(QGridLayout* layout,int row);
    
    virtual void hide();
    
    virtual void show();
    
    virtual void setEnabled(bool b);
    
    virtual void addToLayout(QHBoxLayout* layout);
    
public slots:
    
    void onCheckBoxStateChanged(bool);
    
protected:
    
    virtual void updateGUI(const Variant& variant);
    
private:

	QCheckBox* _checkBox;
    QLabel* _descriptionLabel;
};


//================================
class Double_KnobGui : public KnobGui
{
    Q_OBJECT
public:
    
    static KnobGui* BuildKnobGui(Knob* knob){ return new Double_KnobGui(knob); }

    
    Double_KnobGui(Knob* knob):KnobGui(knob),_slider(0){}
    
    virtual ~Double_KnobGui(){}
    
    virtual void createWidget(QGridLayout* layout,int row);
    
    void setMaximum(int);
    void setMinimum(int);
    
    virtual void hide();
    
    virtual void show();
    
    virtual void setEnabled(bool b);
    
    virtual void addToLayout(QHBoxLayout* layout);
    
public slots:
    void onSpinBoxValueChanged();
    void onSliderValueChanged(double);
    
protected:
    
    virtual void updateGUI(const Variant& variant);
    
private:
    std::vector<std::pair<SpinBox*,QLabel*> > _spinBoxes;
    QLabel* _descriptionLabel;
    ScaleSlider* _slider;
};

//================================

class Button_KnobGui : public KnobGui
{
    Q_OBJECT
public:
    static KnobGui* BuildKnobGui(Knob* knob){ return new Button_KnobGui(knob); }

    
    Button_KnobGui(Knob* knob):KnobGui(knob){}
    
    virtual ~Button_KnobGui(){}

    virtual void createWidget(QGridLayout* layout,int row);
    
    virtual void hide();
    
    virtual void show();
    
    virtual void setEnabled(bool b);
    
    virtual void addToLayout(QHBoxLayout* layout);
    
public slots:
    
    void emitValueChanged();
protected:
    
    virtual void updateGUI(const Variant& variant){Q_UNUSED(variant);}

    
private:
    Button* _button;
};

//================================
class ComboBox_KnobGui : public KnobGui
{
    Q_OBJECT
public:
    static KnobGui* BuildKnobGui(Knob* knob){ return new ComboBox_KnobGui(knob); }

    
    ComboBox_KnobGui(Knob* knob);
    
    virtual ~ComboBox_KnobGui(){}
    
    virtual void createWidget(QGridLayout* layout,int row);
    
    virtual void hide();
    
    virtual void show();
    
    virtual void setEnabled(bool b);
    
    virtual void addToLayout(QHBoxLayout* layout);
    
public slots:
    
    void onCurrentIndexChanged(int i);

protected:
    
    virtual void updateGUI(const Variant& variant);
    
private:
    std::vector<std::string> _entries;
    ComboBox* _comboBox;
    QLabel* _descriptionLabel;
};

//=========================
class Separator_KnobGui : public KnobGui
{
public:
    static KnobGui* BuildKnobGui(Knob* knob){ return new Separator_KnobGui(knob); }

    Separator_KnobGui(Knob* knob):KnobGui(knob){}
    
    virtual ~Separator_KnobGui(){}
    
    virtual void createWidget(QGridLayout* layout,int row);
    
    virtual void hide();
    
    virtual void show();
    
    virtual void setEnabled(bool b){(void)b;}
    
    virtual void addToLayout(QHBoxLayout* layout);
    
protected:
    
    virtual void updateGUI(const Variant& variant){(void)variant;}
private:
    QFrame* _line;
    QLabel* _descriptionLabel;
};

/******************************/
class RGBA_KnobGui : public KnobGui{
    Q_OBJECT
public:
    static KnobGui* BuildKnobGui(Knob* knob){ return new RGBA_KnobGui(knob); }

    
    RGBA_KnobGui(Knob* knob):KnobGui(knob),_alphaEnabled(true){}
    
    virtual ~RGBA_KnobGui(){}
    
    virtual void createWidget(QGridLayout* layout,int row);
    
    void disablePermantlyAlpha();
    
    virtual void hide();
    
    virtual void show();
    
    virtual void setEnabled(bool b);
    
    virtual void addToLayout(QHBoxLayout* layout);
    
public slots:
    
    void onColorChanged();
    
    void showColorDialog();
    
protected:
    
    virtual void updateGUI(const Variant& variant);
    
private:
    
    void updateLabel(const QColor& color);
    
    QWidget* mainContainer;
    QHBoxLayout* mainLayout;
    
    QWidget* boxContainers;
    QHBoxLayout* boxLayout;
    
    QWidget* colorContainer;
    QHBoxLayout* colorLayout;
    
    QLabel* _descriptionLabel;
    
    QLabel* _rLabel;
    QLabel* _gLabel;
    QLabel* _bLabel;
    QLabel* _aLabel;
    
    SpinBox* _rBox;
    SpinBox* _gBox;
    SpinBox* _bBox;
    SpinBox* _aBox;
    
    QLabel* _colorLabel;
    Button* _colorDialogButton;
    
    bool _alphaEnabled;
};

/*****************************/
class String_KnobGui : public KnobGui{
    Q_OBJECT
public:
    static KnobGui* BuildKnobGui(Knob* knob){ return new String_KnobGui(knob); }

    
    String_KnobGui(Knob* knob):KnobGui(knob) {}
	
    virtual ~String_KnobGui(){}
    
    virtual void createWidget(QGridLayout* layout,int row);
    
    virtual void hide();
    
    virtual void show();
    
    virtual void setEnabled(bool b);
    
    virtual void addToLayout(QHBoxLayout* layout);
    
    public slots:
    void onStringChanged(const QString& str);
    
protected:
    
    virtual void updateGUI(const Variant& variant);
    
private:
	LineEdit* _lineEdit;
    QLabel* _descriptionLabel;

};


/***************************/

class GroupBoxLabel : public QLabel{
    Q_OBJECT
    
    bool _checked;
public:
    
    GroupBoxLabel(QWidget* parent = 0);
    
    virtual ~GroupBoxLabel(){}
    
    virtual void mousePressEvent(QMouseEvent*){
        emit checked(!_checked);
    }
    
    
    bool isChecked() const {return _checked;}
    
public slots:
    
    void setChecked(bool);
    
signals:
    void checked(bool);
    
    
    
};



class Group_KnobGui : public KnobGui{
    Q_OBJECT
public:
    static KnobGui* BuildKnobGui(Knob* knob){ return new Group_KnobGui(knob); }

    
    Group_KnobGui(Knob* knob):KnobGui(knob),_checked(false){}
    
    virtual ~Group_KnobGui(){}

    virtual void createWidget(QGridLayout* layout,int row);
    
    void addKnob(KnobGui* child,int row,int column){
        _children.push_back(std::make_pair(child,std::make_pair(row,column)));
    }
    
    bool isChecked() const {return _button->isChecked();}
    
    virtual void hide();
    
    virtual void show();
    
    virtual void setEnabled(bool b){(void)b;}
    
    virtual void addToLayout(QHBoxLayout* layout);
    
protected:
    
    
    virtual void updateGUI(const Variant& variant);
    
    
public slots:
    void setChecked(bool b);

    
private:
    bool _checked;
    QGridLayout* _layout;
    GroupBoxLabel* _button;
    QLabel* _descriptionLabel;
    std::vector< std::pair< KnobGui*,std::pair<int,int> > > _children;
};


/*****************************/
class Tab_KnobGui : public KnobGui{
  
public:
    typedef std::map<std::string, std::pair<QVBoxLayout*, std::vector<KnobGui*> > > KnobsTabMap;
    
    static KnobGui* BuildKnobGui(Knob* knob){ return new Tab_KnobGui(knob); }

    
    Tab_KnobGui(Knob* knob):KnobGui(knob){}
    
    ~Tab_KnobGui(){}

    virtual void createWidget(QGridLayout* layout,int row);
    
    void addKnobs(const std::map<std::string,std::vector<KnobGui*> >& knobs);
    
    virtual void hide();
    
    virtual void show();
    
    virtual void setEnabled(bool b);
    
    virtual void addToLayout(QHBoxLayout* layout);
    
protected:
    
    virtual void updateGUI(const Variant& variant){(void)variant;}
    
private:
  
    TabWidget* _tabWidget;
};



/*****************************/


#endif // POWITER_GUI_KNOB_H_
