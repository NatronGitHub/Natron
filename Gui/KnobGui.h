//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef NATRON_GUI_KNOB_H_
#define NATRON_GUI_KNOB_H_

#include <vector>
#include <map>
#include <cassert>
#include <QtCore/QString>
#include <QtCore/QMetaType>
#include <QtCore/QStringList>
#include <QUndoCommand>
#include <QLineEdit>
#include <QLabel>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"

#include "Engine/Singleton.h"
#include "Engine/Knob.h"

class QCheckBox;
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
class DockablePanel;
class QTextEdit;

namespace Natron{
    class Node;
}

class KnobGui : public QObject
{
    Q_OBJECT
    
    
public:
    
    friend class KnobUndoCommand;
    
    KnobGui(Knob* knob,DockablePanel* container);
    
    virtual ~KnobGui();
        
    bool triggerNewLine() const { return _triggerNewLine; }
    
    void turnOffNewLine() { _triggerNewLine = false; }
    
    Knob* getKnob() const { return _knob; }
    
    /*Set the spacing between items in the layout*/
    void setSpacingBetweenItems(int spacing){ _spacingBetweenItems = spacing; }
    
    int getSpacingBetweenItems() const { return _spacingBetweenItems; }
    
    void createGUI(QGridLayout* layout,int row);
    
    void moveToLayout(QVBoxLayout* layout);
    
    /*Used by Tab_KnobGui to insert already existing widgets
     in a tab.*/
    virtual void addToLayout(QHBoxLayout* layout)=0;
    
    
    void pushUndoCommand(QUndoCommand* cmd);
    
    bool hasWidgetBeenCreated() const {return _widgetCreated;}
    


public slots:
    /*Called when the value held by the knob is changed internally.
     This should in turn update the GUI but not emit the valueChanged()
     signal.*/
    void onInternalValueChanged(int dimension);
    
    void deleteKnob(){
        delete this;
    }

    void setSecret();

    void showAnimationMenu();
    
    virtual void setEnabled() = 0;
    
    void onSetKeyActionTriggered();
    
    void onShowInCurveEditorActionTriggered();

    void hide();

    void show();

    
signals:
    void deleted(KnobGui*);
    
    /*Must be emitted when a value is changed by the user or by
     an external source.*/
    void valueChanged(int dimension,const Variant& variant);
    
    void knobUndoneChange();
    
    void knobRedoneChange();
    
private:

    virtual void _hide() = 0;

    virtual void _show() = 0;


    /*Must create the GUI and insert it in the grid layout at the index "row".*/
    virtual void createWidget(QGridLayout* layout,int row) = 0;
    
   
    /*Called by the onInternalValueChanged slot. This should update
     the widget to reflect the new internal value held by variant.*/
    virtual void updateGUI(int dimension,const Variant& variant) = 0;

    void createAnimationMenu(QWidget* parent);
    
    void createAnimationButton(QGridLayout* layout,int row);
    
    /*This function is used by KnobUndoCommand. Calling this in a onInternalValueChanged/valueChanged
     signal/slot sequence can cause an infinite loop.*/
    void setValue(int dimension,const Variant& variant){
        updateGUI(dimension,variant);
        emit valueChanged(dimension,variant);
    }
    
private:
    Knob* const _knob;
    bool _triggerNewLine;
    int _spacingBetweenItems;
    bool _widgetCreated;
    DockablePanel* const _container;
    QMenu* _animationMenu;
    Button* _animationButton;
};
Q_DECLARE_METATYPE(KnobGui*)

//================================


class KnobUndoCommand : public QObject, public QUndoCommand{
    
    Q_OBJECT
    
public:
    
    KnobUndoCommand(KnobGui* knob,const std::map<int,Variant>& oldValue,const std::map<int,Variant>& newValue,QUndoCommand *parent = 0):QUndoCommand(parent),
    _oldValue(oldValue),
    _newValue(newValue),
    _knob(knob)
    {
        QObject::connect(this, SIGNAL(knobUndoneChange()), knob,SIGNAL(knobUndoneChange()));
        QObject::connect(this, SIGNAL(knobRedoneChange()), knob,SIGNAL(knobRedoneChange()));
    }
    virtual void undo();
    virtual void redo();
    virtual bool mergeWith(const QUndoCommand *command);
    
signals:
    void knobUndoneChange();
    
    void knobRedoneChange();
    
private:
    std::map<int,Variant> _oldValue;
    std::map<int,Variant> _newValue;
    KnobGui* _knob;
};


//================================
class File_KnobGui:public KnobGui
{
    Q_OBJECT
    
public:
    
    static KnobGui* BuildKnobGui(Knob* knob,DockablePanel* container){ return new File_KnobGui(knob,container); }
    
    File_KnobGui(Knob* knob,DockablePanel* container);

    virtual ~File_KnobGui() OVERRIDE FINAL;

public slots:
    void open_file();

    void onReturnPressed();

private:
    virtual void createWidget(QGridLayout* layout,int row) OVERRIDE FINAL;
    
    virtual void _hide() OVERRIDE FINAL;
    
    virtual void _show() OVERRIDE FINAL;
    
    virtual void setEnabled() OVERRIDE FINAL {}
    
    virtual void addToLayout(QHBoxLayout* layout) OVERRIDE FINAL;

    virtual void updateGUI(int dimension,const Variant& variant) OVERRIDE FINAL;

    void updateLastOpened(const QString& str);

private:
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
    
    static KnobGui* BuildKnobGui(Knob* knob,DockablePanel* container){ return new OutputFile_KnobGui(knob,container); }
    
    OutputFile_KnobGui(Knob* knob,DockablePanel* container);
    
    virtual ~OutputFile_KnobGui() OVERRIDE FINAL;
    
public slots:

    void open_file();
    void onReturnPressed();

private:
    virtual void createWidget(QGridLayout* layout,int row) OVERRIDE FINAL;
    
    virtual void _hide() OVERRIDE FINAL;
    
    virtual void _show() OVERRIDE FINAL;
    
    virtual void setEnabled() OVERRIDE FINAL {}
    
    virtual void addToLayout(QHBoxLayout* layout) OVERRIDE FINAL;

    virtual void updateGUI(int dimension,const Variant& variant) OVERRIDE FINAL;

    void updateLastOpened(const QString& str);

private:
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
    
    static KnobGui* BuildKnobGui(Knob* knob,DockablePanel* container){ return new Int_KnobGui(knob,container); }

    
    Int_KnobGui(Knob* knob,DockablePanel* container);
    
    virtual ~Int_KnobGui() OVERRIDE FINAL;
    
public slots:
    
    void onSpinBoxValueChanged();
    
    void onSliderValueChanged(double);
    
    void onMinMaxChanged(int mini,int maxi,int index = 0);
    
    void onIncrementChanged(int incr,int index = 0);
 
private:
    
    virtual void createWidget(QGridLayout* layout,int row) OVERRIDE FINAL;
    
    void setMaximum(int);
    
    void setMinimum(int);
    
    virtual void _hide() OVERRIDE FINAL;
    
    virtual void _show() OVERRIDE FINAL;
    
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void addToLayout(QHBoxLayout* layout) OVERRIDE FINAL;
    
    virtual void updateGUI(int dimension,const Variant& variant) OVERRIDE FINAL;

private:
    std::vector<std::pair<SpinBox*,QLabel*> > _spinBoxes;
    QLabel* _descriptionLabel;
    ScaleSlider* _slider;

};

class ClickableLabel : public QLabel {
    Q_OBJECT
public:
    
    ClickableLabel(const QString& text, QWidget * parent):QLabel(text,parent),_toggled(false){}
    
    virtual ~ClickableLabel() OVERRIDE FINAL {}
    
    void setClicked(bool b){_toggled = b;}

signals:
    void clicked(bool);

private:
    void mousePressEvent ( QMouseEvent *  ) {
        _toggled = !_toggled;
        emit clicked(_toggled);
    }

private:
    bool _toggled;
};

class AnimatedCheckBox : public QCheckBox {


    Q_PROPERTY( int animation READ getAnimation WRITE setAnimation)

    int animation;
public:
    AnimatedCheckBox(QWidget* parent = NULL): QCheckBox(parent),animation(0){}

    virtual ~AnimatedCheckBox(){}

    void setAnimation(int i ) ;

    int getAnimation() const { return animation; }
};

//================================
class Bool_KnobGui :public KnobGui {
	Q_OBJECT
public:
    
    static KnobGui* BuildKnobGui(Knob* knob,DockablePanel* container){ return new Bool_KnobGui(knob,container); }

    
    Bool_KnobGui(Knob* knob,DockablePanel* container):KnobGui(knob,container){}

    virtual ~Bool_KnobGui() OVERRIDE FINAL;
    
public slots:
    
    void onCheckBoxStateChanged(bool);
    
private:
    
    virtual void createWidget(QGridLayout* layout,int row) OVERRIDE FINAL;
    
    virtual void _hide() OVERRIDE FINAL;
    
    virtual void _show() OVERRIDE FINAL;
    
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void addToLayout(QHBoxLayout* layout) OVERRIDE FINAL;
    
    virtual void updateGUI(int dimension,const Variant& variant) OVERRIDE FINAL;
    
private:

    AnimatedCheckBox* _checkBox;
    ClickableLabel* _descriptionLabel;
};


//================================
class Double_KnobGui : public KnobGui {
    Q_OBJECT
public:
    
    static KnobGui* BuildKnobGui(Knob* knob,DockablePanel* container){ return new Double_KnobGui(knob,container); }

    
    Double_KnobGui(Knob* knob,DockablePanel* container);
    
    virtual ~Double_KnobGui() OVERRIDE FINAL;
    
public slots:
    void onSpinBoxValueChanged();
    void onSliderValueChanged(double);
    
    void onMinMaxChanged(double mini,double maxi,int index = 0);
    void onIncrementChanged(double incr,int index = 0);
    void onDecimalsChanged(int deci,int index = 0);
    
private:
    
    virtual void createWidget(QGridLayout* layout,int row) OVERRIDE FINAL;
    
    void setMaximum(int);
    void setMinimum(int);
    
    virtual void _hide() OVERRIDE FINAL;
    
    virtual void _show() OVERRIDE FINAL;
    
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void addToLayout(QHBoxLayout* layout);
    
    virtual void updateGUI(int dimension,const Variant& variant) OVERRIDE FINAL;
    
private:
    std::vector<std::pair<SpinBox*,QLabel*> > _spinBoxes;
    QLabel* _descriptionLabel;
    ScaleSlider* _slider;
};

//================================

class Button_KnobGui : public KnobGui {
    Q_OBJECT
public:
    static KnobGui* BuildKnobGui(Knob* knob,DockablePanel* container){ return new Button_KnobGui(knob,container); }

    
    Button_KnobGui(Knob* knob,DockablePanel* container):KnobGui(knob,container){}
    
    virtual ~Button_KnobGui() OVERRIDE FINAL;

public slots:
    
    void emitValueChanged();

private:
    
    virtual void createWidget(QGridLayout* layout,int row) OVERRIDE FINAL;
    
    virtual void _hide() OVERRIDE FINAL;
    
    virtual void _show() OVERRIDE FINAL;
    
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void addToLayout(QHBoxLayout* layout) OVERRIDE FINAL;
    
    virtual void updateGUI(int dimension,const Variant& variant) OVERRIDE FINAL {(void)dimension;Q_UNUSED(variant);}

    
private:
    Button* _button;
};

//================================
class ComboBox_KnobGui : public KnobGui {
    Q_OBJECT
public:
    static KnobGui* BuildKnobGui(Knob* knob,DockablePanel* container){ return new ComboBox_KnobGui(knob,container); }

    
    ComboBox_KnobGui(Knob* knob,DockablePanel* container);
    
    virtual ~ComboBox_KnobGui() OVERRIDE FINAL;
    
public slots:
    
    void onCurrentIndexChanged(int i);

    void onEntriesPopulated();
    
private:
    
    virtual void createWidget(QGridLayout* layout,int row) OVERRIDE FINAL;
    
    virtual void _hide() OVERRIDE FINAL;
    
    virtual void _show() OVERRIDE FINAL;
    
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void addToLayout(QHBoxLayout* layout) OVERRIDE FINAL;
    
    virtual void updateGUI(int dimension,const Variant& variant) OVERRIDE FINAL;
    
private:
    std::vector<std::string> _entries;
    ComboBox* _comboBox;
    QLabel* _descriptionLabel;
};

//=========================
class Separator_KnobGui : public KnobGui {
public:
    static KnobGui* BuildKnobGui(Knob* knob,DockablePanel* container){ return new Separator_KnobGui(knob,container); }

    Separator_KnobGui(Knob* knob,DockablePanel* container):KnobGui(knob,container){}
    
    virtual ~Separator_KnobGui() OVERRIDE FINAL;
    
private:

    virtual void createWidget(QGridLayout* layout,int row) OVERRIDE FINAL;
    
    virtual void _hide() OVERRIDE FINAL;
    
    virtual void _show() OVERRIDE FINAL;
    
    virtual void setEnabled() OVERRIDE FINAL {}
    
    virtual void addToLayout(QHBoxLayout* layout) OVERRIDE FINAL;
    
    virtual void updateGUI(int dimension,const Variant& variant) OVERRIDE FINAL {(void)dimension;(void)variant;}

private:
    QFrame* _line;
    QLabel* _descriptionLabel;
};

/******************************/
class Color_KnobGui : public KnobGui {
    Q_OBJECT
public:
    static KnobGui* BuildKnobGui(Knob* knob,DockablePanel* container){ return new Color_KnobGui(knob,container); }

    
    Color_KnobGui(Knob* knob,DockablePanel* container);
    
    virtual ~Color_KnobGui() OVERRIDE FINAL;
    
public slots:

    void onColorChanged();

    void showColorDialog();

private:

    virtual void createWidget(QGridLayout* layout,int row) OVERRIDE FINAL;
        
    virtual void _hide() OVERRIDE FINAL;
    
    virtual void _show() OVERRIDE FINAL;
    
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void addToLayout(QHBoxLayout* layout) OVERRIDE FINAL;
    
    virtual void updateGUI(int dimension,const Variant& variant) OVERRIDE FINAL;

    void updateLabel(const QColor& color);
    
private:
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
    
    int _dimension;
};

/*****************************/
class String_KnobGui : public KnobGui{
    Q_OBJECT
public:
    static KnobGui* BuildKnobGui(Knob* knob,DockablePanel* container){ return new String_KnobGui(knob,container); }

    
    String_KnobGui(Knob* knob,DockablePanel* container):KnobGui(knob,container) {}
	
    virtual ~String_KnobGui() OVERRIDE FINAL;
    
public slots:
    void onStringChanged(const QString& str);

private:

    virtual void createWidget(QGridLayout* layout,int row) OVERRIDE FINAL;
    
    virtual void _hide() OVERRIDE FINAL;
    
    virtual void _show() OVERRIDE FINAL;
    
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void addToLayout(QHBoxLayout* layout) OVERRIDE FINAL;
    
    virtual void updateGUI(int dimension,const Variant& variant) OVERRIDE FINAL;
    
private:
	LineEdit* _lineEdit;
    QLabel* _descriptionLabel;

};


/***************************/

class GroupBoxLabel : public QLabel{
    Q_OBJECT
public:
    
    GroupBoxLabel(QWidget* parent = 0);
    
    virtual ~GroupBoxLabel() OVERRIDE FINAL {}
    
    bool isChecked() const {return _checked;}

public slots:
    
    void setChecked(bool);
    
private:
    virtual void mousePressEvent(QMouseEvent*) OVERRIDE FINAL {
        emit checked(!_checked);
    }

signals:
    void checked(bool);
    
private:
    bool _checked;
};



class Group_KnobGui : public KnobGui{
    Q_OBJECT
public:
    static KnobGui* BuildKnobGui(Knob* knob,DockablePanel* container){ return new Group_KnobGui(knob,container); }

    
    Group_KnobGui(Knob* knob,DockablePanel* container):KnobGui(knob,container),_checked(false){}
    
    virtual ~Group_KnobGui() OVERRIDE FINAL;

    void addKnob(KnobGui* child,int row,int column){
        _children.push_back(std::make_pair(child,std::make_pair(row,column)));
    }

    bool isChecked() const {return _button->isChecked();}

public slots:
    void setChecked(bool b);
    
private:
    virtual void createWidget(QGridLayout* layout,int row) OVERRIDE FINAL;

    virtual void _hide() OVERRIDE FINAL;
    
    virtual void _show() OVERRIDE FINAL;
    
    virtual void setEnabled()  OVERRIDE FINAL {}
    
    virtual void addToLayout(QHBoxLayout* layout);
    

    virtual void updateGUI(int dimension,const Variant& variant) OVERRIDE FINAL;


private:
    bool _checked;
    QGridLayout* _layout;
    GroupBoxLabel* _button;
    QLabel* _descriptionLabel;
    std::vector< std::pair< KnobGui*,std::pair<int,int> > > _children;
};




/*****************************/

/*****************************/
class RichText_KnobGui : public KnobGui{
    Q_OBJECT
public:
    static KnobGui* BuildKnobGui(Knob* knob,DockablePanel* container){ return new RichText_KnobGui(knob,container); }
    
    
    RichText_KnobGui(Knob* knob,DockablePanel* container):KnobGui(knob,container) {}
	
    virtual ~RichText_KnobGui() OVERRIDE FINAL;
    
public slots:
    void onTextChanged();

private:

    virtual void createWidget(QGridLayout* layout,int row) OVERRIDE FINAL;
    
    virtual void _hide() OVERRIDE FINAL;
    
    virtual void _show() OVERRIDE FINAL;
    
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void addToLayout(QHBoxLayout* layout) OVERRIDE FINAL;
    
    virtual void updateGUI(int dimension,const Variant& variant) OVERRIDE FINAL;
    
private:
	QTextEdit* _textEdit;
    QLabel* _descriptionLabel;
    
};



#endif // NATRON_GUI_KNOB_H_
