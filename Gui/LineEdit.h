/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_GUI_LINEEDIT_H
#define NATRON_GUI_LINEEDIT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QLineEdit>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/StyledKnobWidgetBase.h"


NATRON_NAMESPACE_ENTER

class LineEdit
    : public QLineEdit
    , public StyledKnobWidgetBase
{
    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON
    DEFINE_KNOB_GUI_STYLE_PROPERTIES
    Q_PROPERTY(bool borderDisabled READ getBorderDisabled WRITE setBorderDisabled)
    Q_PROPERTY(bool isBold READ getIsBold WRITE setIsBold)

public:
    explicit LineEdit(QWidget* parent = 0);

    virtual ~LineEdit() OVERRIDE;

    void setReadOnly_NoFocusRect(bool readOnly);
    void setBorderDisabled(bool disabled);

    void setReadOnly(bool ro);

    bool getBorderDisabled() const;

    bool getIsBold() const;

    void setIsBold(bool b);

    void setCustomTextColor(const QColor& color);



    enum AdditionalDecorationType
    {
        eAdditionalDecorationColoredFrame = 0x1,
        eAdditionalDecorationColoredUnderlinedText = 0x2
    };

    void setAdditionalDecorationTypeEnabled(AdditionalDecorationType type, bool enabled, const QColor& color = Qt::black);

    void disableAllDecorations();
    
Q_SIGNALS:

    void textDropped();

    void textPasted();

protected:

    virtual void refreshStylesheet() OVERRIDE;
    virtual void paintEvent(QPaintEvent* e) OVERRIDE;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE;
    virtual void dropEvent(QDropEvent* e) OVERRIDE;
    virtual void dragEnterEvent(QDragEnterEvent* e) OVERRIDE;
    virtual void dragMoveEvent(QDragMoveEvent* e) OVERRIDE;
    virtual void dragLeaveEvent(QDragLeaveEvent* e) OVERRIDE;

private:
    
    QColor _customColor;
    bool _customColorSet;
    bool isBold;
    bool borderDisabled;


    struct AdditionalDecoration
    {
        bool enabled;
        QColor color;
    };
    typedef std::map<AdditionalDecorationType, AdditionalDecoration> AdditionalDecorationsMap;


    AdditionalDecorationsMap decorationType;
};

NATRON_NAMESPACE_EXIT

#endif // ifndef NATRON_GUI_LINEEDIT_H
