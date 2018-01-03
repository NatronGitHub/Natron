/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#ifndef NATRON_GUI_ANIMATEDCHECKBOX_H
#define NATRON_GUI_ANIMATEDCHECKBOX_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QFrame>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Macros.h"

#include "Gui/StyledKnobWidgetBase.h"

NATRON_NAMESPACE_ENTER

class AnimatedCheckBox
    : public QFrame
    , public StyledKnobWidgetBase
{

    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON
    DEFINE_KNOB_GUI_STYLE_PROPERTIES
    
    bool readOnly;
    bool checked;
    bool _linked;

public:

    AnimatedCheckBox(QWidget *parent = NULL);

    virtual ~AnimatedCheckBox() OVERRIDE
    {
    }

    bool isChecked() const
    {
        return checked;
    }

    void setChecked(bool c);

    void setReadOnly(bool readOnly);

    bool getReadOnly() const
    {
        return readOnly;
    }

    void setLinked(bool linked);

    virtual QSize minimumSizeHint() const OVERRIDE FINAL;
    virtual QSize sizeHint() const OVERRIDE FINAL;
    virtual void getBackgroundColor(double *r, double *g, double *b) const;

Q_SIGNALS:

    void toggled(bool);

    void clicked(bool);

protected:

    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE;

private:

    virtual void refreshStylesheet() OVERRIDE FINAL;

    virtual void paintEvent(QPaintEvent* e) OVERRIDE FINAL;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_ANIMATEDCHECKBOX_H
