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
#include <QWidget>
#include <QLineEdit>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"
#include "Engine/Singleton.h"


class TabWidget;
class QCheckBox;
class QGroupBox;
class Node;
class Variant;
class LineEdit;
class Button;
class FeedbackSpinBox;
class ComboBox;
class PluginID;
class QFrame;
class QLabel;
class QGridLayout;
class Knob;
class QVBoxLayout;
class KnobGui : public QWidget
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
    
    /*Must create the GUI and insert it in the grid layout at the index "row".*/
    virtual void createWidget(QGridLayout* layout,int row)=0;
    
    void pushUndoCommand(QUndoCommand* cmd);
    
    
    
public slots:
    /*Called when the value held by the knob is changed internally.
     This should in turn update the GUI but not emit the valueChanged()
     signal.*/
    void onInternalValueChanged(const Variant& variant){
        updateGUI(variant);
    }
    
signals:
    
    /*Must be emitted when a value is changed by the user or by
     an external source.*/
    void valueChanged(const Variant& variant);
    
protected:
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
};

//================================
class File_KnobGui:public KnobGui
{
    Q_OBJECT
    
public:
    
    
    File_KnobGui(Knob* knob):KnobGui(knob){}

    virtual ~File_KnobGui(){}
            
    virtual void createWidget(QGridLayout* layout,int row);
    
public slots:
    void open_file();
    void onReturnPressed();
    
    
protected:
    
    virtual void updateGUI(const Variant& variant);
    
private:
    
    void updateLastOpened(const QString& str);
    
    
    LineEdit* _lineEdit;
    QString _lastOpened;
};


//================================
class OutputFile_KnobGui : public KnobGui
{
    Q_OBJECT
public:
    
    OutputFile_KnobGui(Knob* knob):KnobGui(knob){}
    
    virtual ~OutputFile_KnobGui(){}
    
    virtual void createWidget(QGridLayout* layout,int row);
    
public slots:
    void open_file();
    void onReturnPressed();
    
signals:
    void filesSelected();
    
protected:
    
    virtual void updateGUI(const Variant& variant);
    
private:
    void updateLastOpened(const QString& str);

    
    LineEdit* _lineEdit;
    QString _lastOpened;
};




//================================

class Int_KnobGui : public KnobGui
{
    Q_OBJECT
public:
    
    
    
    Int_KnobGui(Knob* knob):KnobGui(knob){}
    
    virtual ~Int_KnobGui(){}
    
    virtual void createWidget(QGridLayout* layout,int row);
    
    void setMaximum(int);
    void setMinimum(int);
    
public slots:
    void onSpinBoxValueChanged();
 
protected:
    
    virtual void updateGUI(const Variant& variant);
    
private:
    std::vector<FeedbackSpinBox*> _spinBoxes;
};

//================================
class Bool_KnobGui :public KnobGui
{
	Q_OBJECT
public:
    
    Bool_KnobGui(Knob* knob):KnobGui(knob){}

    virtual ~Bool_KnobGui(){}
    
    virtual void createWidget(QGridLayout* layout,int row);
    
public slots:
    
    void onCheckBoxStateChanged(bool);
    
protected:
    
    virtual void updateGUI(const Variant& variant);
    
private:

	QCheckBox* _checkBox;
};


//================================
class Double_KnobGui : public KnobGui
{
    Q_OBJECT
public:
    
    
    
    Double_KnobGui(Knob* knob):KnobGui(knob){}
    
    virtual ~Double_KnobGui(){}
    
    virtual void createWidget(QGridLayout* layout,int row);
    
    void setMaximum(int);
    void setMinimum(int);
    
    public slots:
    void onSpinBoxValueChanged();
    
protected:
    
    virtual void updateGUI(const Variant& variant);
    
private:
    std::vector<FeedbackSpinBox*> _spinBoxes;
};

//================================

class Button_KnobGui : public KnobGui
{
    Q_OBJECT
public:
    
    Button_KnobGui(Knob* knob):KnobGui(knob){}
    
    virtual ~Button_KnobGui(){}

    virtual void createWidget(QGridLayout* layout,int row);
    
    
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
    ComboBox_KnobGui(Knob* knob);
    
    virtual ~ComboBox_KnobGui(){}
    
    virtual void createWidget(QGridLayout* layout,int row);
    
        
public slots:
    
    void onCurrentIndexChanged(int i);
    
    void populate(const QStringList& entries);


protected:
    
    virtual void updateGUI(const Variant& variant);
    
private:
    ComboBox* _comboBox;
};

//=========================
class Separator_KnobGui : public KnobGui
{
public:
    Separator_KnobGui(Knob* knob):KnobGui(knob){}
    
    virtual ~Separator_KnobGui(){}
    
    virtual void createWidget(QGridLayout* layout,int row);
    
protected:
    
    virtual void updateGUI(const Variant& variant){(void)variant;}
private:
    QFrame* _line;
};

/******************************/
class RGBA_KnobGui : public KnobGui{
    Q_OBJECT
public:
    
    RGBA_KnobGui(Knob* knob):KnobGui(knob){}
    
    virtual ~RGBA_KnobGui(){}
    
    virtual void createWidget(QGridLayout* layout,int row);
    
    void disablePermantlyAlpha();
    
public slots:
    
    void onColorChanged();
    
    void showColorDialog();
    
protected:
    
    virtual void updateGUI(const Variant& variant);
    
private:
    
    void updateLabel(const QColor& color);
    
    
    
    QLabel* _rLabel;
    QLabel* _gLabel;
    QLabel* _bLabel;
    QLabel* _aLabel;
    
    FeedbackSpinBox* _rBox;
    FeedbackSpinBox* _gBox;
    FeedbackSpinBox* _bBox;
    FeedbackSpinBox* _aBox;
    
    QLabel* _colorLabel;
    Button* _colorDialogButton;
    
    bool _alphaEnabled;
};
/***************************/
class Group_KnobGui : public KnobGui{
    Q_OBJECT
public:
    
    Group_KnobGui(Knob* knob):KnobGui(knob){}
    
    virtual ~Group_KnobGui(){}

    virtual void createWidget(QGridLayout* layout,int row);

    void addKnob(KnobGui* k);
    
protected:
    
    
    virtual void updateGUI(const Variant& variant){(void)variant;}
    
    
public slots:
    void setChecked(bool b);

    
private:
    
    QGroupBox* _box;
    QVBoxLayout* _boxLayout;
    std::vector<KnobGui*> _knobs;
    
};


/*****************************/
class Tab_KnobGui : public KnobGui{
  
public:
    typedef std::map<std::string, std::pair<QVBoxLayout*, std::vector<KnobGui*> > > KnobsTabMap;
    
    Tab_KnobGui(Knob* knob):KnobGui(knob){}
    
    ~Tab_KnobGui(){}

    virtual void createWidget(QGridLayout* layout,int row);
    
    void addTab(const std::string& name);
    
    void addKnob(const std::string& tabName,KnobGui* k);
    
protected:
    
    virtual void updateGUI(const Variant& variant);
    
private:
    TabWidget* _tabWidget;
    KnobsTabMap _knobs;
};
/*****************************/
class String_KnobGui : public KnobGui{
    Q_OBJECT
public:
    
    String_KnobGui(Knob* knob):KnobGui(knob) {}
	
    virtual ~String_KnobGui(){}
    
    virtual void createWidget(QGridLayout* layout,int row);
    
public slots:
    void onStringChanged(const QString& str);

protected:
    
    virtual void updateGUI(const Variant& variant);
    
private:
	LineEdit* _lineEdit;
};




/*****************************/


#endif // POWITER_GUI_KNOB_H_
