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

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <vector> // Int_KnobGui
#include <list>
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QStyledItemDelegate>
#include <QTextEdit>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"

#include "Engine/Singleton.h"
#include "Engine/Knob.h"
#include "Engine/ImageComponents.h"

#include "Gui/CurveSelection.h"
#include "Gui/KnobGui.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/Label.h"

//Define this if you want the spinbox to clamp to the plugin defined range
//#define SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT

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
class CurveWidget;
class KnobCurveGui;
class TabGroup;

// private classes, defined in KnobGuiTypes.cpp
namespace Natron {
class GroupBoxLabel;
class ClickableLabel;
}
class AnimatedCheckBox;

namespace Natron {
class Node;
}

//================================

class Int_KnobGui
    : public KnobGui
{
CLANG_DIAG_OFF_36(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON_36(inconsistent-missing-override)

public:

    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new Int_KnobGui(knob, container);
    }

    Int_KnobGui(boost::shared_ptr<KnobI> knob,
                DockablePanel *container);

    virtual ~Int_KnobGui() OVERRIDE;
    
    virtual void removeSpecificGui() OVERRIDE FINAL;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public Q_SLOTS:

    void onSpinBoxValueChanged();

    void onSliderValueChanged(double);
    void onSliderEditingFinished(bool hasMovedOnce);

#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
    void onMinMaxChanged(double mini, double maxi, int index = 0);
#endif
    
    void onDisplayMinMaxChanged(double mini,double maxi,int index = 0);

    void onIncrementChanged(int incr, int index = 0);
    
    void onDimensionSwitchClicked();
    
private:
    void expandAllDimensions();
    void foldAllDimensions();
    
    void sliderEditingEnd(double d);

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;

    void setMaximum(int);

    void setMinimum(int);

    virtual bool shouldAddStretch() const OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevelEnum level) OVERRIDE FINAL;
    virtual void reflectExpressionState(int dimension,bool hasExpr) OVERRIDE FINAL;
    virtual void updateToolTip() OVERRIDE FINAL;
    virtual void reflectModificationsState() OVERRIDE FINAL;

private:
    std::vector<std::pair<SpinBox *, Natron::Label *> > _spinBoxes;
    QWidget *_container;
    ScaleSliderQWidget *_slider;
    Button *_dimensionSwitchButton;
    boost::weak_ptr<Int_Knob> _knob;
};


//================================

class Bool_CheckBox: public AnimatedCheckBox
{
    bool useCustomColor;
    QColor customColor;
    
public:
    
    Bool_CheckBox(QWidget* parent = 0) : AnimatedCheckBox(parent), useCustomColor(false), customColor() {}
    
    virtual ~Bool_CheckBox() {}
    
    void setCustomColor(const QColor& color, bool useCustom)
    {
        useCustomColor = useCustom;
        customColor = color;
    }
    
    virtual void getBackgroundColor(double *r,double *g,double *b) const OVERRIDE FINAL;
    
    
};

class Bool_KnobGui
    : public KnobGui
{
CLANG_DIAG_OFF_36(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON_36(inconsistent-missing-override)

public:

    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new Bool_KnobGui(knob, container);
    }

    Bool_KnobGui(boost::shared_ptr<KnobI> knob,
                 DockablePanel *container);

    virtual ~Bool_KnobGui() OVERRIDE;
    
    virtual void removeSpecificGui() OVERRIDE FINAL;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public Q_SLOTS:

    void onCheckBoxStateChanged(bool);
    void onLabelClicked(bool);
private:

    
    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevelEnum level) OVERRIDE FINAL;
    virtual void reflectExpressionState(int dimension,bool hasExpr) OVERRIDE FINAL;
    virtual void updateToolTip() OVERRIDE FINAL;
    virtual void onLabelChanged() OVERRIDE FINAL;
private:

    Bool_CheckBox *_checkBox;
    boost::weak_ptr<Bool_Knob> _knob;
};


//================================
class Double_KnobGui
    : public KnobGui
{
CLANG_DIAG_OFF_36(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON_36(inconsistent-missing-override)

public:

    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new Double_KnobGui(knob, container);
    }

    Double_KnobGui(boost::shared_ptr<KnobI> knob,
                   DockablePanel *container);

    virtual ~Double_KnobGui() OVERRIDE;
    
    virtual void removeSpecificGui() OVERRIDE FINAL;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public Q_SLOTS:
    void onSpinBoxValueChanged();
    void onSliderValueChanged(double);
    void onSliderEditingFinished(bool hasMovedOnce);
#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
    void onMinMaxChanged(double mini, double maxi, int index = 0);
#endif
    void onDisplayMinMaxChanged(double mini,double maxi,int index = 0);
    void onIncrementChanged(double incr, int index = 0);
    void onDecimalsChanged(int deci, int index = 0);

    void onDimensionSwitchClicked();

private:
    void expandAllDimensions();
    void foldAllDimensions();

    void sliderEditingEnd(double d);
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

    virtual bool shouldAddStretch() const OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevelEnum level) OVERRIDE FINAL;
    virtual void reflectExpressionState(int dimension,bool hasExpr) OVERRIDE FINAL;
    virtual void updateToolTip() OVERRIDE FINAL;
    virtual void reflectModificationsState() OVERRIDE FINAL;
private:
    std::vector<std::pair<SpinBox *, Natron::Label *> > _spinBoxes;
    QWidget *_container;
    ScaleSliderQWidget *_slider;
    Button *_dimensionSwitchButton;
    boost::weak_ptr<Double_Knob> _knob;
};

//================================

class Button_KnobGui
    : public KnobGui
{
CLANG_DIAG_OFF_36(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON_36(inconsistent-missing-override)

public:
    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new Button_KnobGui(knob, container);
    }

    Button_KnobGui(boost::shared_ptr<KnobI> knob,
                   DockablePanel *container);

    virtual ~Button_KnobGui() OVERRIDE;

    virtual void removeSpecificGui() OVERRIDE FINAL;
    
    virtual bool showDescriptionLabel() const OVERRIDE
    {
        return false;
    }

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public Q_SLOTS:

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
    boost::weak_ptr<Button_Knob> _knob;
};

//================================
struct NewLayerDialogPrivate;
class NewLayerDialog : public QDialog
{
    Q_OBJECT
    
public:
    
    
    NewLayerDialog(QWidget* parent);
    
    virtual ~NewLayerDialog();
    
    Natron::ImageComponents getComponents() const;
    
public Q_SLOTS:
    
    void onNumCompsChanged(double value);
    
    void onRGBAButtonClicked();
    
private:
    
    boost::scoped_ptr<NewLayerDialogPrivate> _imp;
    
};

class Choice_KnobGui
    : public KnobGui
{
CLANG_DIAG_OFF_36(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON_36(inconsistent-missing-override)

public:
    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new Choice_KnobGui(knob, container);
    }

    Choice_KnobGui(boost::shared_ptr<KnobI> knob,
                   DockablePanel *container);

    virtual ~Choice_KnobGui() OVERRIDE;
    
    virtual void removeSpecificGui() OVERRIDE FINAL;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public Q_SLOTS:

    void onCurrentIndexChanged(int i);

    void onEntriesPopulated();
    
    void onItemNewSelected();

private:

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevelEnum level) OVERRIDE FINAL;
    virtual void reflectExpressionState(int dimension,bool hasExpr) OVERRIDE FINAL;
    virtual void updateToolTip() OVERRIDE FINAL;
    virtual void reflectModificationsState() OVERRIDE FINAL;
    
    std::vector<std::string> _entries;
    ComboBox *_comboBox;
    boost::weak_ptr<Choice_Knob> _knob;
};

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
    
    virtual void removeSpecificGui() OVERRIDE FINAL;

    virtual bool showDescriptionLabel() const OVERRIDE
    {
        return false;
    }
    
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

private:
    virtual bool shouldAddStretch() const OVERRIDE FINAL { return false; }
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
    boost::weak_ptr<Separator_Knob> _knob;
};

/******************************/

class Color_KnobGui;
class ColorPickerLabel
    : public Natron::Label
{
CLANG_DIAG_OFF_36(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON_36(inconsistent-missing-override)

public:

    ColorPickerLabel(Color_KnobGui* knob,QWidget* parent = NULL);

    virtual ~ColorPickerLabel() OVERRIDE
    {
    }
    
    bool isPickingEnabled() const
    {
        return _pickingEnabled;
    }

    void setColor(const QColor & color);

    const QColor& getCurrentColor() const
    {
        return _currentColor;
    }

    void setPickingEnabled(bool enabled);
    
    void setEnabledMode(bool enabled);

Q_SIGNALS:

    void pickingEnabled(bool);

private:

    virtual void enterEvent(QEvent*) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent*) OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent*) OVERRIDE FINAL;

private:

    bool _pickingEnabled;
    QColor _currentColor;
    Color_KnobGui* _knob;
};


class Color_KnobGui
    : public KnobGui
{
CLANG_DIAG_OFF_36(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON_36(inconsistent-missing-override)

public:
    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new Color_KnobGui(knob, container);
    }

    Color_KnobGui(boost::shared_ptr<KnobI> knob,
                  DockablePanel *container);

    virtual ~Color_KnobGui() OVERRIDE;
    
    virtual void removeSpecificGui() OVERRIDE FINAL;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public Q_SLOTS:

    void onColorChanged();
    void onMinMaxChanged(double mini, double maxi, int index);
    void onDisplayMinMaxChanged(double mini, double maxi, int index);

    void showColorDialog();

    void setPickingEnabled(bool enabled);


    void onPickingEnabled(bool enabled);

    void onDimensionSwitchClicked();

    void onSliderValueChanged(double v);
    
    void onSliderEditingFinished(bool hasMovedOnce);

    void onMustShowAllDimension();

    void onDialogCurrentColorChanged(const QColor & color);

Q_SIGNALS:

    void dimensionSwitchToggled(bool b);

private:

    void expandAllDimensions();
    void foldAllDimensions();

    virtual bool shouldAddStretch() const OVERRIDE { return false; }
    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevelEnum level) OVERRIDE FINAL;
    virtual void reflectExpressionState(int dimension,bool hasExpr) OVERRIDE FINAL;
    virtual void updateToolTip() OVERRIDE FINAL;
    virtual void reflectModificationsState() OVERRIDE FINAL;
    
    void updateLabel(double r, double g, double b, double a);

private:
    QWidget *mainContainer;
    QHBoxLayout *mainLayout;
    QWidget *boxContainers;
    QHBoxLayout *boxLayout;
    QWidget *colorContainer;
    QHBoxLayout *colorLayout;
    Natron::Label *_rLabel;
    Natron::Label *_gLabel;
    Natron::Label *_bLabel;
    Natron::Label *_aLabel;
    SpinBox *_rBox;
    SpinBox *_gBox;
    SpinBox *_bBox;
    SpinBox *_aBox;
    ColorPickerLabel *_colorLabel;
    Button *_colorDialogButton;
    Button *_dimensionSwitchButton;
    ScaleSliderQWidget* _slider;
    int _dimension;
    boost::weak_ptr<Color_Knob> _knob;
    std::vector<double> _lastColor;
};

class AnimatingTextEdit
    : public QTextEdit
{
CLANG_DIAG_OFF_36(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON_36(inconsistent-missing-override)

    Q_PROPERTY( int animation READ getAnimation WRITE setAnimation)
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

Q_SIGNALS:

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
CLANG_DIAG_OFF_36(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON_36(inconsistent-missing-override)

public:
    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new String_KnobGui(knob, container);
    }

    String_KnobGui(boost::shared_ptr<KnobI> knob,
                   DockablePanel *container);

    virtual ~String_KnobGui() OVERRIDE;
    
    virtual void removeSpecificGui() OVERRIDE FINAL;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public Q_SLOTS:

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
    static void parseFont(const QString & s, QFont* f, QColor* color);
    static void findReplaceColorName(QString& text,const QColor& color);
    static QString makeFontTag(const QString& family,int fontSize,const QColor& color);
    static QString decorateTextWithFontTag(const QString& family,int fontSize,const QColor& color,const QString& text);
    static QString removeNatronHtmlTag(QString text);

    static QString getNatronHtmlTagContent(QString text);
    
    /**
     * @brief The goal here is to remove all the tags added automatically by Natron (like font color,size family etc...)
     * so the user does not see them in the user interface. Those tags are  present in the internal value held by the knob.
     **/
    static QString removeAutoAddedHtmlTags(QString text,bool removeNatronTag = true) ;
    
private:

    virtual bool shouldAddStretch() const OVERRIDE { return false; }
    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevelEnum level) OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void reflectExpressionState(int dimension,bool hasExpr) OVERRIDE FINAL;
    virtual void updateToolTip() OVERRIDE FINAL;
    virtual void reflectModificationsState() OVERRIDE FINAL;
    
    void mergeFormat(const QTextCharFormat & fmt);

    void restoreTextInfoFromString();



    QString addHtmlTags(QString text) const;

    /**
     * @brief Removes the prepending and appending '\n' and ' ' from str except for the last character.
     **/
    static QString stripWhitespaces(const QString & str);

private:
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
    Natron::Label *_label; //< if label
    boost::weak_ptr<String_Knob> _knob;
};

/*****************************/


class Group_KnobGui
    : public KnobGui
{
CLANG_DIAG_OFF_36(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON_36(inconsistent-missing-override)

public:
    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new Group_KnobGui(knob, container);
    }

    Group_KnobGui(boost::shared_ptr<KnobI> knob,
                  DockablePanel *container);

    virtual ~Group_KnobGui() OVERRIDE;

    virtual void removeSpecificGui() OVERRIDE FINAL;
    
    void addKnob(KnobGui *child);
    
    const std::list<KnobGui*>& getChildren() const { return _children; }

    bool isChecked() const;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;
    
    TabGroup* getOrCreateTabWidget();
    
    void removeTabWidget();

public Q_SLOTS:
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
    Natron::GroupBoxLabel *_button;
    std::list<KnobGui*> _children;
    std::vector< std::pair<KnobGui*,std::vector<int> > > _childrenToEnable; //< when re-enabling a group, what are the children that we should set
    TabGroup* _tabGroup;
    //enabled too
    boost::weak_ptr<Group_Knob> _knob;
};

/*****************************/
class Parametric_KnobGui
    : public KnobGui
    , public CurveSelection
{
CLANG_DIAG_OFF_36(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON_36(inconsistent-missing-override)

public:
    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new Parametric_KnobGui(knob, container);
    }

    Parametric_KnobGui(boost::shared_ptr<KnobI> knob,
                       DockablePanel *container);
    
    virtual void removeSpecificGui() OVERRIDE FINAL;
    
    virtual bool showDescriptionLabel() const OVERRIDE
    {
        return false;
    }

    virtual ~Parametric_KnobGui() OVERRIDE;
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

    virtual void getSelectedCurves(std::vector<boost::shared_ptr<CurveGui> >* selection) OVERRIDE FINAL;

    
public Q_SLOTS:


    void onCurveChanged(int dimension);

    void onItemsSelectionChanged();

    void resetSelectedCurves();

    void onColorChanged(int dimension);

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
    QWidget* treeColumn;
    CurveWidget* _curveWidget;
    QTreeWidget* _tree;
    Button* _resetButton;
    struct CurveDescriptor
    {
        boost::shared_ptr<KnobCurveGui> curve;
        QTreeWidgetItem* treeItem;
    };

    typedef std::map<int,CurveDescriptor> CurveGuis;
    CurveGuis _curves;
    boost::weak_ptr<Parametric_Knob> _knob;
};


#endif // NATRON_GUI_KNOBGUITYPES_H_
