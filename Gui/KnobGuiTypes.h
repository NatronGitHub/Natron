//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_GUI_KNOBGUITYPES_H_
#define NATRON_GUI_KNOBGUITYPES_H_

#include <vector> // Int_KnobGui

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QLabel>
#include <QStyledItemDelegate>
#include <QTextEdit>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

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
class QFontComboBox;

// Engine
class KnobI;
class Int_Knob;
class Bool_Knob;
class Double_Knob;
class Button_Knob;
class Separator_Knob;
class Group_Knob;
class Tab_Knob;
class Parametric_Knob;
class Color_Knob;
class Choice_Knob;
class String_Knob;

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

namespace Natron {
class Node;
}

//================================

class Int_KnobGui
    : public KnobGui
{
    Q_OBJECT

public:

    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new Int_KnobGui(knob, container);
    }

    Int_KnobGui(boost::shared_ptr<KnobI> knob,
                DockablePanel *container);

    virtual ~Int_KnobGui() OVERRIDE;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public slots:

    void onSpinBoxValueChanged();

    void onSliderValueChanged(double);
    void onSliderEditingFinished();

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
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevel level) OVERRIDE FINAL;

private:
    std::vector<std::pair<SpinBox *, QLabel *> > _spinBoxes;
    ScaleSliderQWidget *_slider;
    boost::shared_ptr<Int_Knob> _knob;
};


//================================
class Bool_KnobGui
    : public KnobGui
{
    Q_OBJECT

public:

    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new Bool_KnobGui(knob, container);
    }

    Bool_KnobGui(boost::shared_ptr<KnobI> knob,
                 DockablePanel *container);

    virtual ~Bool_KnobGui() OVERRIDE;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public slots:

    void onCheckBoxStateChanged(bool);

private:

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevel level) OVERRIDE FINAL;

private:

    AnimatedCheckBox *_checkBox;
    boost::shared_ptr<Bool_Knob> _knob;
};


//================================
class Double_KnobGui
    : public KnobGui
{
    Q_OBJECT

public:

    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new Double_KnobGui(knob, container);
    }

    Double_KnobGui(boost::shared_ptr<KnobI> knob,
                   DockablePanel *container);

    virtual ~Double_KnobGui() OVERRIDE;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public slots:
    void onSpinBoxValueChanged();
    void onSliderValueChanged(double);
    void onSliderEditingFinished();

    void onMinMaxChanged(double mini, double maxi, int index = 0);
    void onDisplayMinMaxChanged(double mini,double maxi,int index = 0);
    void onIncrementChanged(double incr, int index = 0);
    void onDecimalsChanged(int deci, int index = 0);

private:

    /**
     * @brief Normalized parameters handling. It converts from project format
     * to normailzed coords or from project format to normalized coords.
     * @param normalize True if we want to normalize, false otherwise
     * @param dimension Must be either 0 and 1
     * @note If the dimension of the knob is not 1 or 2 this function does nothing.
     **/
    void valueAccordingToType(bool normalize,int dimension,double* value);

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;

    void setMaximum(int);
    void setMinimum(int);

    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevel level) OVERRIDE FINAL;

private:
    std::vector<std::pair<SpinBox *, QLabel *> > _spinBoxes;
    ScaleSliderQWidget *_slider;
    boost::shared_ptr<Double_Knob> _knob;
};

//================================

class Button_KnobGui
    : public KnobGui
{
    Q_OBJECT

public:
    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new Button_KnobGui(knob, container);
    }

    Button_KnobGui(boost::shared_ptr<KnobI> knob,
                   DockablePanel *container);

    virtual ~Button_KnobGui() OVERRIDE;

    virtual bool showDescriptionLabel() const
    {
        return false;
    }

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public slots:

    void emitValueChanged();

private:

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool /*dirty*/) OVERRIDE FINAL
    {
    }

    virtual void updateGUI(int /*dimension*/) OVERRIDE FINAL
    {
    }

private:
    Button *_button;
    boost::shared_ptr<Button_Knob> _knob;
};

//================================
class Choice_KnobGui
    : public KnobGui
{
    Q_OBJECT

public:
    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new Choice_KnobGui(knob, container);
    }

    Choice_KnobGui(boost::shared_ptr<KnobI> knob,
                   DockablePanel *container);

    virtual ~Choice_KnobGui() OVERRIDE;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public slots:

    void onCurrentIndexChanged(int i);

    void onEntriesPopulated();

private:

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevel level) OVERRIDE FINAL;
    std::vector<std::string> _entries;
    ComboBox *_comboBox;
    boost::shared_ptr<Choice_Knob> _knob;
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
//    static KnobGui *BuildKnobGui(boost::shared_ptr<KnobI> knob, DockablePanel *container) {
//        return new Table_KnobGui(knob, container);
//    }
//
//
//    Table_KnobGui(boost::shared_ptr<KnobI> knob, DockablePanel *container);
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
//    virtual void updateGUI(int dimension) OVERRIDE FINAL;
//
//    // ComboBoxDelegate* _tableComboBoxDelegate;
//    QLabel *_descriptionLabel;
//    QScrollArea* _sa;
//    QWidget* _container;
//    QVBoxLayout* _layout;
//    std::vector<ComboBox*> _choices;
//};

//=========================
class Separator_KnobGui
    : public KnobGui
{
public:
    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new Separator_KnobGui(knob, container);
    }

    Separator_KnobGui(boost::shared_ptr<KnobI> knob,
                      DockablePanel *container);

    virtual ~Separator_KnobGui() OVERRIDE;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

private:

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL
    {
    }

    virtual void setReadOnly(bool /*readOnly*/,
                             int /*dimension*/) OVERRIDE FINAL
    {
    }

    virtual void setDirty(bool /*dirty*/) OVERRIDE FINAL
    {
    }

    virtual void updateGUI(int /*dimension*/) OVERRIDE FINAL
    {
    }

private:
    QFrame *_line;
    boost::shared_ptr<Separator_Knob> _knob;
};

/******************************/

class ColorPickerLabel
    : public QLabel
{
    Q_OBJECT

public:

    ColorPickerLabel(QWidget* parent = NULL);

    virtual ~ColorPickerLabel() OVERRIDE
    {
    }

    bool isPickingEnabled() const
    {
        return _pickingEnabled;
    }

    void setColor(const QColor & color);

    void setPickingEnabled(bool enabled);

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


class Color_KnobGui
    : public KnobGui
{
    Q_OBJECT

public:
    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new Color_KnobGui(knob, container);
    }

    Color_KnobGui(boost::shared_ptr<KnobI> knob,
                  DockablePanel *container);

    virtual ~Color_KnobGui() OVERRIDE;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public slots:

    void onColorChanged();
    void onMinMaxChanged(double mini, double maxi, int index);
    void onDisplayMinMaxChanged(double mini, double maxi, int index);

    void showColorDialog();

    void setPickingEnabled(bool enabled);


    void onPickingEnabled(bool enabled);

    void onDimensionSwitchClicked();

    void onSliderValueChanged(double v);

    void onMustShowAllDimension();

    void onDialogCurrentColorChanged(const QColor & color);

signals:

    void dimensionSwitchToggled(bool b);

private:

    void expandAllDimensions();
    void foldAllDimensions();

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevel level) OVERRIDE FINAL;

    void updateLabel(double r, double g, double b, double a);

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
    Button *_dimensionSwitchButton;
    ScaleSliderQWidget* _slider;
    int _dimension;
    boost::shared_ptr<Color_Knob> _knob;
    std::vector<double> _lastColor;
};

class AnimatingTextEdit
    : public QTextEdit
{
    Q_OBJECT Q_PROPERTY( int animation READ getAnimation WRITE setAnimation)
    Q_PROPERTY( bool readOnlyNatron READ isReadOnlyNatron WRITE setReadOnlyNatron)
    Q_PROPERTY(bool dirty READ getDirty WRITE setDirty)

public:

    AnimatingTextEdit(QWidget* parent = 0)
        : QTextEdit(parent), animation(0), readOnlyNatron(false), _hasChanged(false), dirty(false)
    {
    }

    virtual ~AnimatingTextEdit()
    {
    }

    int getAnimation() const
    {
        return animation;
    }

    void setAnimation(int v);

    bool isReadOnlyNatron() const
    {
        return readOnlyNatron;
    }

    void setReadOnlyNatron(bool ro);

    bool getDirty() const
    {
        return dirty;
    }

    void setDirty(bool b);

signals:

    void editingFinished();

private:

    virtual void focusOutEvent(QFocusEvent* e) OVERRIDE;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE;
    virtual void paintEvent(QPaintEvent* e) OVERRIDE;
    int animation;
    bool readOnlyNatron; //< to bypass the readonly property of Qt that is bugged
    bool _hasChanged;
    bool dirty;
};

/*****************************/
class QTextCharFormat;
class String_KnobGui
    : public KnobGui
{
    Q_OBJECT

public:
    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new String_KnobGui(knob, container);
    }

    String_KnobGui(boost::shared_ptr<KnobI> knob,
                   DockablePanel *container);

    virtual ~String_KnobGui() OVERRIDE;

    virtual bool showDescriptionLabel() const OVERRIDE FINAL;
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public slots:

    ///if the knob is not multiline
    void onLineChanged();

    ///if the knob is multiline
    void onTextChanged();

    ///if the knob is multiline
    void onCurrentFontChanged(const QFont & font);

    ///if the knob is multiline
    void onFontSizeChanged(double size);

    ///is bold activated
    void boldChanged(bool toggled);

    ///is italic activated
    void italicChanged(bool toggled);

    void colorFontButtonClicked();

    void updateFontColorIcon(const QColor & color);

    ///this is a big hack: the html parser builtin QGraphicsTextItem should do this for us...but it doesn't seem to take care
    ///of the font size.
    static void parseFont(const QString & s,QFont & f);
    static QString removeNatronHtmlTag(QString text);

private:

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevel level) OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;

    void mergeFormat(const QTextCharFormat & fmt);

    void restoreTextInfosFromString();


    /**
     * @brief The goal here is to remove all the tags added automatically by Natron (like font color,size family etc...)
     * so the user does not see them in the user interface. Those tags are  present in the internal value held by the knob.
     **/
    QString removeAutoAddedHtmlTags(QString text) const;

    QString addHtmlTags(QString text) const;

    /**
     * @brief Removes the prepending and appending '\n' and ' ' from str except for the last character.
     **/
    static QString stripWhitespaces(const QString & str);
    LineEdit *_lineEdit; //< if single line
    QWidget* _container; //< only used when multiline is on
    QVBoxLayout* _mainLayout; //< only used when multiline is on
    AnimatingTextEdit *_textEdit; //< if multiline
    QWidget* _richTextOptions;
    QHBoxLayout* _richTextOptionsLayout;
    QFontComboBox* _fontCombo;
    Button* _setBoldButton;
    Button* _setItalicButton;
    SpinBox* _fontSizeSpinBox;
    Button* _fontColorButton;
    int _fontSize;
    bool _boldActivated;
    bool _italicActivated;
    QString _fontFamily;
    QColor _fontColor;
    QLabel *_label; //< if label
    boost::shared_ptr<String_Knob> _knob;
};

/*****************************/


class Group_KnobGui
    : public KnobGui
{
    Q_OBJECT

public:
    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new Group_KnobGui(knob, container);
    }

    Group_KnobGui(boost::shared_ptr<KnobI> knob,
                  DockablePanel *container);

    virtual ~Group_KnobGui() OVERRIDE;

    void addKnob(KnobGui *child, int row, int column);

    bool isChecked() const;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public slots:
    void setChecked(bool b);

private:
    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled()  OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool /*dirty*/) OVERRIDE FINAL
    {
    }

    virtual bool eventFilter(QObject *target, QEvent* e) OVERRIDE FINAL;
    virtual void setReadOnly(bool /*readOnly*/,
                             int /*dimension*/) OVERRIDE FINAL
    {
    }

private:
    bool _checked;
    GroupBoxLabel *_button;
    std::vector< std::pair< KnobGui *, std::pair<int, int> > > _children;
    std::vector< std::pair<KnobGui*,std::vector<int> > > _childrenToEnable; //< when re-enabling a group, what are the children that we should set
    //enabled too
    boost::shared_ptr<Group_Knob> _knob;
};

/*****************************/
class Parametric_KnobGui
    : public KnobGui
{
    Q_OBJECT

public:
    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new Parametric_KnobGui(knob, container);
    }

    Parametric_KnobGui(boost::shared_ptr<KnobI> knob,
                       DockablePanel *container);
    virtual bool showDescriptionLabel() const
    {
        return false;
    }

    virtual ~Parametric_KnobGui() OVERRIDE;
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public slots:

    void onCurveChanged(int dimension);

    void onItemsSelectionChanged();

    void resetSelectedCurves();

private:

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool /*dirty*/) OVERRIDE FINAL
    {
    }

    virtual void setReadOnly(bool /*readOnly*/,
                             int /*dimension*/) OVERRIDE FINAL
    {
    }

private:
    // TODO: PIMPL
    CurveWidget* _curveWidget;
    QTreeWidget* _tree;
    Button* _resetButton;
    struct CurveDescriptor
    {
        CurveGui* curve;
        QTreeWidgetItem* treeItem;
    };

    typedef std::map<int,CurveDescriptor> CurveGuis;
    CurveGuis _curves;
    boost::shared_ptr<Parametric_Knob> _knob;
};


#endif // NATRON_GUI_KNOBGUITYPES_H_
