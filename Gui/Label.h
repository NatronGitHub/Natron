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

#ifndef NATRON_GUI_LABEL_H
#define NATRON_GUI_LABEL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QLabel> // in QtGui on Qt4, in QtWidgets on Qt5
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/StyledKnobWidgetBase.h"

NATRON_NAMESPACE_ENTER

class Label
    : public QLabel
    , public StyledKnobWidgetBase
{
    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON
    DEFINE_KNOB_GUI_STYLE_PROPERTIES

    Q_PROPERTY(bool isBold READ getIsBold WRITE setIsBold)

public:

    Label(const QString &text,
          QWidget *parent = 0,
          Qt::WindowFlags f = 0);

    Label(QWidget *parent = 0,
          Qt::WindowFlags f = 0);

    void setCustomTextColor(const QColor& color);

    virtual bool canAlter() const
    {
        return true;
    }

    bool getIsBold() const;

    void setIsBold(bool b);

    virtual void setIsModified(bool b) OVERRIDE;

protected:

    virtual void changeEvent(QEvent* e) OVERRIDE FINAL;

    virtual void refreshStylesheet() OVERRIDE FINAL;

private:

    void init();

    QColor _customColor;
    bool _customColorSet;
    bool isBold;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_-LABEL_H
