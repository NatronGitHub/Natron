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

#ifndef NATRON_GUI_CLICKABLELABEL_H
#define NATRON_GUI_CLICKABLELABEL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Macros.h"
#include "Gui/GuiFwd.h"
#include "Gui/KnobWidgetDnD.h"
#include "Gui/Label.h"

NATRON_NAMESPACE_ENTER;
 
class ClickableLabel
    : public Label
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    Q_PROPERTY(bool dirty READ getDirty WRITE setDirty)
    Q_PROPERTY( int animation READ getAnimation WRITE setAnimation)
    Q_PROPERTY( bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY( bool sunkenStyle READ isSunken WRITE setSunken)

public:

    ClickableLabel(const QString &text,
                   QWidget *parent);

    virtual ~ClickableLabel() OVERRIDE
    {
    }

    void setClicked(bool b)
    {
        _toggled = b;
    }

    void setDirty(bool b);

    bool getDirty() const
    {
        return dirty;
    }

    ///Updates the text as setText does but also keeps the current color info
    void setText_overload(const QString & str);

    int getAnimation() const
    {
        return animation;
    }

    void setAnimation(int i);

    bool isReadOnly() const
    {
        return readOnly;
    }

    void setReadOnly(bool readOnly);

    bool isSunken() const
    {
        return sunkenStyle;
    }

    void setSunken(bool s);

Q_SIGNALS:
    void clicked(bool);

protected:
    
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE;
    
private:
    
    virtual void changeEvent(QEvent* e) OVERRIDE FINAL;

private:
    bool _toggled;
    bool dirty;
    bool readOnly;
    int animation;
    bool sunkenStyle;
};


class KnobClickableLabel : public ClickableLabel, public KnobWidgetDnD
{
  
public:
    
    KnobClickableLabel(const QString& text, KnobGui* knob, QWidget* parent = 0)
    : ClickableLabel(text, parent)
    , KnobWidgetDnD(knob,-1, this)
    {
        
    }
    
    virtual ~KnobClickableLabel() {
        
    }
    
private:
    
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
};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_GUI_CLICKABLELABEL_H
