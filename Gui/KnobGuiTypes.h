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

#ifndef NATRON_GUI_KNOBGUITYPES_H_
#define NATRON_GUI_KNOBGUITYPES_H_

#include <vector> // Int_KnobGui

#include <QLabel>
#include <QObject>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"

#include "Engine/Singleton.h"
#include "Engine/Knob.h"

#include "Gui/KnobGui.h"

// Qt
class QString;
class QFrame;
class QGridLayout;
class QTextEdit;
class QTreeWidget;
class QTreeWidgetItem;

// Engine
class Knob;

// Gui
class DockablePanel;
class LineEdit;
class Button;
class SpinBox;
class ComboBox;
class ScaleSlider;
class GroupBoxLabel;
class CurveWidget;
class CurveGui;

// private classes, defined in KnobGuiTypes.cpp
class ClickableLabel;
class AnimatedCheckBox;

namespace Natron
{
class Node;
}

//================================

class Int_KnobGui : public KnobGui
{
    Q_OBJECT
public:

    static KnobGui *BuildKnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container) {
        return new Int_KnobGui(knob, container);
    }


    Int_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container);

    virtual ~Int_KnobGui();

public slots:

    void onSpinBoxValueChanged();

    void onSliderValueChanged(double);

    void onMinMaxChanged(int mini, int maxi, int index = 0);

    void onIncrementChanged(int incr, int index = 0);

private:

    virtual void createWidget(QGridLayout *layout, int row) OVERRIDE FINAL;

    void setMaximum(int);

    void setMinimum(int);

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL;

    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;
    
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevel level) OVERRIDE FINAL;

private:
    std::vector<std::pair<SpinBox *, QLabel *> > _spinBoxes;
    QLabel *_descriptionLabel;
    ScaleSlider *_slider;

};



//================================
class Bool_KnobGui : public KnobGui
{
    Q_OBJECT
public:

    static KnobGui *BuildKnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container) {
        return new Bool_KnobGui(knob, container);
    }


    Bool_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container): KnobGui(knob, container) {}

    virtual ~Bool_KnobGui();

public slots:

    void onCheckBoxStateChanged(bool);

private:

    virtual void createWidget(QGridLayout *layout, int row) OVERRIDE FINAL;

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL;

    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;

    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevel level) OVERRIDE FINAL;
private:

    AnimatedCheckBox *_checkBox;
    ClickableLabel *_descriptionLabel;
};


//================================
class Double_KnobGui : public KnobGui
{
    Q_OBJECT
public:

    static KnobGui *BuildKnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container) {
        return new Double_KnobGui(knob, container);
    }


    Double_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container);

    virtual ~Double_KnobGui();

public slots:
    void onSpinBoxValueChanged();
    void onSliderValueChanged(double);

    void onMinMaxChanged(double mini, double maxi, int index = 0);
    void onIncrementChanged(double incr, int index = 0);
    void onDecimalsChanged(int deci, int index = 0);

private:

    virtual void createWidget(QGridLayout *layout, int row) OVERRIDE FINAL;

    void setMaximum(int);
    void setMinimum(int);

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL;

    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;

    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevel level) OVERRIDE FINAL;

private:
    std::vector<std::pair<SpinBox *, QLabel *> > _spinBoxes;
    QLabel *_descriptionLabel;
    ScaleSlider *_slider;
};

//================================

class Button_KnobGui : public KnobGui
{
    Q_OBJECT
public:
    static KnobGui *BuildKnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container) {
        return new Button_KnobGui(knob, container);
    }


    Button_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container): KnobGui(knob, container) {}

    virtual ~Button_KnobGui();

public slots:

    void emitValueChanged();

private:

    virtual void createWidget(QGridLayout *layout, int row) OVERRIDE FINAL;

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL;

    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL {(void)dimension; Q_UNUSED(variant);}


private:
    Button *_button;
};

//================================
class Choice_KnobGui : public KnobGui
{
    Q_OBJECT
public:
    static KnobGui *BuildKnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container) {
        return new Choice_KnobGui(knob, container);
    }


    Choice_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container);

    virtual ~Choice_KnobGui();

public slots:

    void onCurrentIndexChanged(int i);

    void onEntriesPopulated();

private:

    virtual void createWidget(QGridLayout *layout, int row) OVERRIDE FINAL;

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL;

    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;

    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevel level) OVERRIDE FINAL;

private:
    std::vector<std::string> _entries;
    ComboBox *_comboBox;
    QLabel *_descriptionLabel;
};

//=========================
class Separator_KnobGui : public KnobGui
{
public:
    static KnobGui *BuildKnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container) {
        return new Separator_KnobGui(knob, container);
    }

    Separator_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container): KnobGui(knob, container) {}

    virtual ~Separator_KnobGui();

private:

    virtual void createWidget(QGridLayout *layout, int row) OVERRIDE FINAL;

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL {}

    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL {(void)dimension; (void)variant;}

private:
    QFrame *_line;
    QLabel *_descriptionLabel;
};
/******************************/

class ColorPickerLabel : public QLabel {
    
    Q_OBJECT
    
public:
    
    ColorPickerLabel(QWidget* parent = NULL);
    
    virtual ~ColorPickerLabel(){}
    
    bool isPickingEnabled() const { return _pickingEnabled; }

    void setColor(const QColor& color);
    
protected:
    
    virtual void enterEvent(QEvent*);
    
    virtual void leaveEvent(QEvent*);
        
    virtual void mousePressEvent(QMouseEvent*) ;    

signals:
    
    void pickingEnabled(bool);
    
private:

    bool _pickingEnabled;
    QColor _currentColor;
};


class Color_KnobGui : public KnobGui
{
    Q_OBJECT
public:
    static KnobGui *BuildKnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container) {
        return new Color_KnobGui(knob, container);
    }


    Color_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container);

    virtual ~Color_KnobGui();

public slots:

    void onColorChanged();

    void showColorDialog();
    
    void onPickingEnabled(bool enabled);

private:

    virtual void createWidget(QGridLayout *layout, int row) OVERRIDE FINAL;

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL;

    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;

    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevel level) OVERRIDE FINAL;
    
    void updateLabel(const QColor &color);

private:
    QWidget *mainContainer;
    QHBoxLayout *mainLayout;

    QWidget *boxContainers;
    QHBoxLayout *boxLayout;

    QWidget *colorContainer;
    QHBoxLayout *colorLayout;

    QLabel *_descriptionLabel;

    QLabel *_rLabel;
    QLabel *_gLabel;
    QLabel *_bLabel;
    QLabel *_aLabel;

    SpinBox *_rBox;
    SpinBox *_gBox;
    SpinBox *_bBox;
    SpinBox *_aBox;

    ColorPickerLabel *_colorLabel;
    Button *_colorDialogButton;

    int _dimension;
};

/*****************************/
class String_KnobGui : public KnobGui
{
    Q_OBJECT
public:
    static KnobGui *BuildKnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container) {
        return new String_KnobGui(knob, container);
    }


    String_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container): KnobGui(knob, container) {}

    virtual ~String_KnobGui();

public slots:
    void onStringChanged(const QString &str);

private:

    virtual void createWidget(QGridLayout *layout, int row) OVERRIDE FINAL;

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL;

    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;

private:
    LineEdit *_lineEdit;
    QLabel *_descriptionLabel;

};

/*****************************/
class Custom_KnobGui : public KnobGui
{
public:
    static KnobGui *BuildKnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container) {
        return new Custom_KnobGui(knob, container);
    }


    Custom_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container): KnobGui(knob, container) {}

    virtual ~Custom_KnobGui() ;

private:

    virtual void createWidget(QGridLayout *layout, int row) OVERRIDE FINAL;

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL;

    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;

private:
    LineEdit *_lineEdit;
    QLabel *_descriptionLabel;
};




class Group_KnobGui : public KnobGui
{
    Q_OBJECT
public:
    static KnobGui *BuildKnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container) {
        return new Group_KnobGui(knob, container);
    }


    Group_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container): KnobGui(knob, container), _checked(false) {}

    virtual ~Group_KnobGui();

    void addKnob(KnobGui *child, int row, int column);

    bool isChecked() const;

public slots:
    void setChecked(bool b);

private:
    virtual void createWidget(QGridLayout *layout, int row) OVERRIDE FINAL;

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled()  OVERRIDE FINAL {}

    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;


private:
    bool _checked;
    QGridLayout *_layout;
    GroupBoxLabel *_button;
    QLabel *_descriptionLabel;
    std::vector< std::pair< KnobGui *, std::pair<int, int> > > _children;
};




/*****************************/

/*****************************/
class RichText_KnobGui : public KnobGui
{
    Q_OBJECT
public:
    static KnobGui *BuildKnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container) {
        return new RichText_KnobGui(knob, container);
    }


    RichText_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container): KnobGui(knob, container) {}

    virtual ~RichText_KnobGui();

public slots:
    void onTextChanged();

private:

    virtual void createWidget(QGridLayout *layout, int row) OVERRIDE FINAL;

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL;

    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;

private:
    QTextEdit *_textEdit;
    QLabel *_descriptionLabel;

};


/*****************************/
class Parametric_KnobGui : public KnobGui
{
    Q_OBJECT
    
public:
    static KnobGui *BuildKnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container) {
        return new Parametric_KnobGui(knob, container);
    }
    
    
    Parametric_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container);
    
    virtual ~Parametric_KnobGui();
    
public slots:
    
    void onCurveChanged(int dimension);
    
    void onItemsSelectionChanged();
    
    void resetSelectedCurves();
    
private:
    
    virtual void createWidget(QGridLayout *layout, int row) OVERRIDE FINAL;
    
    virtual void _hide() OVERRIDE FINAL;
    
    virtual void _show() OVERRIDE FINAL;
    
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;
    
    
    CurveWidget* _curveWidget;
    QTreeWidget* _tree;
    Button* _resetButton;
    
    struct CurveDescriptor{
        CurveGui* curve;
        QTreeWidgetItem* treeItem;
    };
    
    typedef std::map<int,CurveDescriptor> CurveGuis;
    CurveGuis _curves;
};


#endif // NATRON_GUI_KNOBGUITYPES_H_
