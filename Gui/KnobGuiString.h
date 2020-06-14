/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef Gui_KnobGuiString_h
#define Gui_KnobGuiString_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector> // KnobGuiInt
#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QStyledItemDelegate>
#include <QTextEdit>
#include <QColor>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/Knob.h"
#include "Engine/ImagePlaneDesc.h"
#include "Engine/EngineFwd.h"

#include "Gui/KnobGuiWidgets.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/StyledKnobWidgetBase.h"


NATRON_NAMESPACE_ENTER

class AnimatingTextEdit
    : public QTextEdit
    , public StyledKnobWidgetBase
{
    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON 
    DEFINE_KNOB_GUI_STYLE_PROPERTIES
public:

    AnimatingTextEdit(const KnobGuiPtr& knob,
                      DimSpec dimension,
                      ViewIdx view,
                      QWidget* parent = 0);

    virtual ~AnimatingTextEdit();

    void setLinkedAppearanceEnabled(bool linked);

Q_SIGNALS:

    void editingFinished();

private:

    virtual void refreshStylesheet() OVERRIDE FINAL;

    virtual void focusOutEvent(QFocusEvent* e) OVERRIDE;
    virtual void focusInEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE;
    virtual void keyReleaseEvent(QKeyEvent* e) OVERRIDE;
    virtual void paintEvent(QPaintEvent* e) OVERRIDE;
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void dragEnterEvent(QDragEnterEvent* e) OVERRIDE FINAL;
    virtual void dragMoveEvent(QDragMoveEvent* e) OVERRIDE FINAL;
    virtual void dropEvent(QDropEvent* e) OVERRIDE FINAL;

private:
    bool _hasChanged;
    bool _appearLinked;
    QColor _lastBgColor, _lastFgColor;
    boost::shared_ptr<KnobWidgetDnD> _dnd;
};

class KnobLineEdit
    : public LineEdit
{
public:
    KnobLineEdit(const KnobGuiPtr& knob,
                 DimSpec dimension,
                 ViewIdx view,
                 QWidget* parent = 0);

    virtual ~KnobLineEdit();

private:

    virtual void focusInEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual void focusOutEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void keyReleaseEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void dragEnterEvent(QDragEnterEvent* e) OVERRIDE FINAL;
    virtual void dragMoveEvent(QDragMoveEvent* e) OVERRIDE FINAL;
    virtual void dropEvent(QDropEvent* e) OVERRIDE FINAL;

private:
    KnobWidgetDnDPtr _dnd;
};

/*****************************/
class KnobGuiString
    : public QObject, public KnobGuiWidgets
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    static KnobGuiWidgets * BuildKnobGui(const KnobGuiPtr& knob, ViewIdx view)
    {
        return new KnobGuiString(knob, view);
    }

    KnobGuiString(const KnobGuiPtr& knob, ViewIdx view);

    virtual ~KnobGuiString() OVERRIDE;

    bool isLabelOnSameColumn() const OVERRIDE FINAL;

    virtual void reflectLinkedState(DimIdx dimension, bool linked) OVERRIDE;

    virtual std::string getDescriptionLabel() const OVERRIDE FINAL WARN_UNUSED_RETURN;

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

    static bool parseFont(const QString & s, QFont* f, QColor* color);

    void onFontPropertyChanged();

Q_SIGNALS:

    void fontPropertyChanged();

private:


    QFont makeFontFromState() const;

    virtual bool shouldAddStretch() const OVERRIDE;

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void setWidgetsVisible(bool visible) OVERRIDE FINAL;
    virtual void setEnabled(const std::vector<bool>& perDimEnabled) OVERRIDE FINAL;
    virtual void updateGUI() OVERRIDE FINAL;
    virtual void reflectMultipleSelection(bool dirty) OVERRIDE FINAL;
    virtual void reflectSelectionState(bool selected) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(DimIdx dimension, AnimationLevelEnum level) OVERRIDE FINAL;
    virtual void updateToolTip() OVERRIDE FINAL;
    virtual void reflectModificationsState() OVERRIDE FINAL;

    void mergeFormat(const QTextCharFormat & fmt);

private:
    KnobLineEdit *_lineEdit; //< if single line
    Label* _label; // if label and the actual label is an icon
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
    KnobStringWPtr _knob;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_KnobGuiString_h
