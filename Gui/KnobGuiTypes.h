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

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QObject>
#include <QLabel>
#include <QStyledItemDelegate>
#include <QTextEdit>
CLANG_DIAG_ON(deprecated)

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"

#include "Engine/Singleton.h"
#include "Engine/Knob.h"

#include "Gui/KnobGui.h"

// Qt
class QString;
class QFrame;
class QHBoxLayout;
class QTreeWidget;
class QTreeWidgetItem;
class QScrollArea;

// Engine
class Knob;

// Gui
class DockablePanel;
class LineEdit;
class Button;
class SpinBox;
class ComboBox;
class ScaleSliderQWidget;
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

    virtual ~Int_KnobGui() OVERRIDE;

public slots:

    void onSpinBoxValueChanged();

    void onSliderValueChanged(double);

    void onMinMaxChanged(int mini, int maxi, int index = 0);
    
    void onDisplayMinMaxChanged(int mini,int maxi,int index = 0);

    void onIncrementChanged(int incr, int index = 0);

private:

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;

    void setMaximum(int);

    void setMinimum(int);

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;

    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;
    
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevel level) OVERRIDE FINAL;

private:
    std::vector<std::pair<SpinBox *, QLabel *> > _spinBoxes;
    ScaleSliderQWidget *_slider;

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

    virtual ~Bool_KnobGui() OVERRIDE;

public slots:

    void onCheckBoxStateChanged(bool);

private:

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL;

    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    
    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;

    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevel level) OVERRIDE FINAL;
private:

    AnimatedCheckBox *_checkBox;
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

    virtual ~Double_KnobGui() OVERRIDE;

public slots:
    void onSpinBoxValueChanged();
    void onSliderValueChanged(double);

    void onMinMaxChanged(double mini, double maxi, int index = 0);
    void onDisplayMinMaxChanged(double mini,double maxi,int index = 0);
    void onIncrementChanged(double incr, int index = 0);
    void onDecimalsChanged(int deci, int index = 0);

private:

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;

    void setMaximum(int);
    void setMinimum(int);

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL;

    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    
    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;

    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevel level) OVERRIDE FINAL;

private:
    std::vector<std::pair<SpinBox *, QLabel *> > _spinBoxes;
    ScaleSliderQWidget *_slider;
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

    virtual ~Button_KnobGui() OVERRIDE;
    
    virtual bool showDescriptionLabel() const { return false; }

public slots:

    void emitValueChanged();

private:

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;

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

    virtual ~Choice_KnobGui() OVERRIDE;

public slots:

    void onCurrentIndexChanged(int i);

    void onEntriesPopulated();

private:

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;

    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;

    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevel level) OVERRIDE FINAL;

    std::vector<std::string> _entries;
    ComboBox *_comboBox;
};

//================================
//class Table_KnobGui;
//class ComboBoxDelegate : public QStyledItemDelegate{
//    
//    Q_OBJECT
//    
//    Table_KnobGui* _tableKnob;
//    
//public:
//    
//    ComboBoxDelegate(Table_KnobGui* tableKnob,QObject *parent = 0);
//    
//    virtual ~ComboBoxDelegate(){}
//    
//    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
//                          const QModelIndex &index) const OVERRIDE;
//    
//    virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
//    virtual void setModelData(QWidget *editor, QAbstractItemModel *model,
//                      const QModelIndex &index) const OVERRIDE;
//    
//    virtual void updateEditorGeometry(QWidget *editor,
//                              const QStyleOptionViewItem &option, const QModelIndex &index) const OVERRIDE;
//    
//    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const OVERRIDE;
//    
//    virtual QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const OVERRIDE;
//
//};
//
//
//class Table_KnobGui : public KnobGui
//{
//    Q_OBJECT
//public:
//    static KnobGui *BuildKnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container) {
//        return new Table_KnobGui(knob, container);
//    }
//    
//    
//    Table_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container);
//    
//    virtual ~Table_KnobGui() OVERRIDE;
//    
//    public slots:
//    
//    void onCurrentIndexChanged(int);
//    
//    void onPopulated();
//    
//private:
//    
//    virtual void createWidget(QGridLayout *layout, int row) OVERRIDE FINAL;
//    
//    virtual void _hide() OVERRIDE FINAL;
//    
//    virtual void _show() OVERRIDE FINAL;
//    
//    virtual void setEnabled() OVERRIDE FINAL;
//    
//    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;
//    
//    // ComboBoxDelegate* _tableComboBoxDelegate;
//    QLabel *_descriptionLabel;
//    QScrollArea* _sa;
//    QWidget* _container;
//    QVBoxLayout* _layout;
//    std::vector<ComboBox*> _choices;
//};

//=========================
class Separator_KnobGui : public KnobGui
{
public:
    static KnobGui *BuildKnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container) {
        return new Separator_KnobGui(knob, container);
    }

    Separator_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container): KnobGui(knob, container) {}

    virtual ~Separator_KnobGui() OVERRIDE;

private:

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL {}
    
    virtual void setReadOnly(bool /*readOnly*/,int /*dimension*/) OVERRIDE FINAL {}

    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL {(void)dimension; (void)variant;}

private:
    QFrame *_line;
};
/******************************/

class ColorPickerLabel : public QLabel {
    
    Q_OBJECT
    
public:
    
    ColorPickerLabel(QWidget* parent = NULL);
    
    virtual ~ColorPickerLabel() OVERRIDE {}
    
    bool isPickingEnabled() const { return _pickingEnabled; }

    void setColor(const QColor& color);
    
signals:
    
    void pickingEnabled(bool);
    
private:

    virtual void enterEvent(QEvent*) OVERRIDE FINAL;

    virtual void leaveEvent(QEvent*) OVERRIDE FINAL;

    virtual void mousePressEvent(QMouseEvent*) OVERRIDE FINAL;

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

    virtual ~Color_KnobGui() OVERRIDE;

public slots:

    void onColorChanged();

    void showColorDialog();
    
    void onPickingEnabled(bool enabled);

private:

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;

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

class AnimatingTextEdit : public QTextEdit {
    Q_OBJECT
    Q_PROPERTY( int animation READ getAnimation WRITE setAnimation)
    Q_PROPERTY( bool readOnlyNatron READ isReadOnlyNatron WRITE setReadOnlyNatron)

public:
    
    AnimatingTextEdit(QWidget* parent = 0) : QTextEdit(parent) , animation(0) , readOnlyNatron(false) , _hasChanged(false) {}
    
    virtual ~AnimatingTextEdit(){}
    
    int getAnimation() const { return animation; }
    
    void setAnimation(int v) ;
    
    bool isReadOnlyNatron() const { return readOnlyNatron; }
    
    void setReadOnlyNatron(bool ro);
signals:
    
    void editingFinished();
private:
    
    virtual void focusOutEvent(QFocusEvent* e) OVERRIDE;
    
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE;

    virtual void paintEvent(QPaintEvent* e) OVERRIDE;

    int animation;
    bool readOnlyNatron; //< to bypass the readonly property of Qt that is bugged
    bool _hasChanged;

};

/*****************************/
class String_KnobGui : public KnobGui
{
    Q_OBJECT
public:
    static KnobGui *BuildKnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container) {
        return new String_KnobGui(knob, container);
    }


    String_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container);

    virtual ~String_KnobGui() OVERRIDE;

    virtual bool showDescriptionLabel() const OVERRIDE FINAL;
    
public slots:
    
    ///if the knob is not multiline
    void onLineChanged();

    ///if the knob is multiline
    void onTextChanged();
private:

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL;

    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;
    
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevel level) OVERRIDE FINAL;
    
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;


private:
    LineEdit *_lineEdit; //< if single line
    AnimatingTextEdit *_textEdit; //< if multiline
    QLabel *_label; //< if label
};

/*****************************/




class Group_KnobGui : public KnobGui
{
    Q_OBJECT
public:
    static KnobGui *BuildKnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container) {
        return new Group_KnobGui(knob, container);
    }


    Group_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container): KnobGui(knob, container), _checked(false) {}

    virtual ~Group_KnobGui() OVERRIDE;

    void addKnob(KnobGui *child, int row, int column);

    bool isChecked() const;

public slots:
    void setChecked(bool b);

private:
    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled()  OVERRIDE FINAL;

    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;

    virtual bool eventFilter(QObject *target, QEvent *event) OVERRIDE FINAL;
    
    virtual void setReadOnly(bool /*readOnly*/,int /*dimension*/) OVERRIDE FINAL {}

private:
    bool _checked;
    GroupBoxLabel *_button;
    std::vector< std::pair< KnobGui *, std::pair<int, int> > > _children;
    std::vector< std::pair<KnobGui*,std::vector<int> > > _childrenToEnable; //< when re-enabling a group, what are the children that we should set
                                               //enabled too
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
    
    virtual bool showDescriptionLabel() const { return false; }
    
    virtual ~Parametric_KnobGui() OVERRIDE;
    
public slots:
    
    void onCurveChanged(int dimension);
    
    void onItemsSelectionChanged();
    
    void resetSelectedCurves();
    
private:
    
    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    
    virtual void _hide() OVERRIDE FINAL;
    
    virtual void _show() OVERRIDE FINAL;
    
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;
    
    virtual void setReadOnly(bool /*readOnly*/,int /*dimension*/) OVERRIDE FINAL {}
    
private:
    // TODO: PIMPL
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
