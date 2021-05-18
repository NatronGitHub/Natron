/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#ifndef NATRON_GUI_CLICKABLELABEL_H
#define NATRON_GUI_CLICKABLELABEL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Macros.h"
#include "Gui/Label.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

class ClickableLabel
    : public Label
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON
    // properties
    Q_PROPERTY(bool dirty READ getDirty WRITE setDirty)
    Q_PROPERTY(int animation READ getAnimation WRITE setAnimation)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY(bool sunkenStyle READ isSunken WRITE setSunken)

public:

    ClickableLabel(const QPixmap &icon,
                   QWidget *parent);


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
        return _dirty;
    }

    ///Updates the text as setText does but also keeps the current color info
    void setText_overload(const QString & str);

    int getAnimation() const
    {
        return _animation;
    }

    void setAnimation(int i);

    bool isReadOnly() const
    {
        return _readOnly;
    }

    void setReadOnly(bool readOnly);

    bool isSunken() const
    {
        return _sunkenStyle;
    }

    void setSunken(bool s);

    void setBold(bool b);

    virtual bool canAlter() const OVERRIDE FINAL;

Q_SIGNALS:
    void clicked(bool);

protected:

    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE;

private:

    virtual void changeEvent(QEvent* e) OVERRIDE FINAL;

private:
    bool _toggled;
    bool _bold;
    bool _dirty;
    bool _readOnly;
    int _animation;
    bool _sunkenStyle;
};


class KnobClickableLabel
    : public ClickableLabel
{
public:

    KnobClickableLabel(const QPixmap& icon,
                       const KnobGuiPtr& knob,
                       QWidget* parent = 0);

    KnobClickableLabel(const QString& text,
                       const KnobGuiPtr& knob,
                       QWidget* parent = 0);

    virtual ~KnobClickableLabel();

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
    virtual void focusInEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual void focusOutEvent(QFocusEvent* e) OVERRIDE FINAL;
    KnobWidgetDnDPtr _dnd;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_CLICKABLELABEL_H
