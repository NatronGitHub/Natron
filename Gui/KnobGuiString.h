/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QStyledItemDelegate>
#include <QTextEdit>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/Singleton.h"
#include "Engine/Knob.h"
#include "Engine/ImageComponents.h"
#include "Engine/EngineFwd.h"

#include "Gui/CurveSelection.h"
#include "Gui/KnobGui.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/Label.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;

class AnimatingTextEdit
    : public QTextEdit
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

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
class KnobGuiString
    : public KnobGui
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new KnobGuiString(knob, container);
    }

    KnobGuiString(boost::shared_ptr<KnobI> knob,
                   DockablePanel *container);

    virtual ~KnobGuiString() OVERRIDE;
    
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
    virtual void reflectAnimationLevel(int dimension,AnimationLevelEnum level) OVERRIDE FINAL;
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
    boost::weak_ptr<KnobString> _knob;
};

NATRON_NAMESPACE_EXIT;

#endif // Gui_KnobGuiString_h
