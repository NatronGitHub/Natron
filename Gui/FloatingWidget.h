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

#ifndef Gui_FloatingWidget_h
#define Gui_FloatingWidget_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QMainWindow>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/ScriptObject.h"
#include "Engine/EngineFwd.h"

#include "Gui/SerializableWindow.h"
#include "Gui/GuiFwd.h"


#define kMainSplitterObjectName "ToolbarSplitter"

NATRON_NAMESPACE_ENTER

/*This class represents a floating pane that embeds a widget*/
class FloatingWidget
    : public QWidget, public SerializableWindow
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    explicit FloatingWidget(Gui* gui,
                            QWidget* parent = 0);

    virtual ~FloatingWidget();

    /*Set the embedded widget. Only 1 widget can be embedded
       by FloatingWidget. Once set, this function does nothing
       for subsequent calls..*/
    void setWidget(QWidget* w);

    void removeEmbeddedWidget();

    QWidget* getEmbeddedWidget() const
    {
        return _embeddedWidget;
    }

public Q_SLOTS:

    void onProjectNameChanged(const QString& filePath, bool modified);

Q_SIGNALS:

    void closed();

private:

    virtual void moveEvent(QMoveEvent* e) OVERRIDE FINAL;
    virtual void resizeEvent(QResizeEvent* e) OVERRIDE FINAL;
    virtual void closeEvent(QCloseEvent* e) OVERRIDE;
    QWidget* _embeddedWidget;
    QScrollArea* _scrollArea;
    QVBoxLayout* _layout;
    Gui* _gui;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_FloatingWidget_h
