/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef _Gui_FloatingWidget_h_
#define _Gui_FloatingWidget_h_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QMainWindow>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"
#include "Gui/SerializableWindow.h"
#include "Engine/ScriptObject.h"


#define kMainSplitterObjectName "ToolbarSplitter"

//boost
namespace boost {
namespace archive {
class xml_iarchive;
class xml_oarchive;
}
}

//QtGui
class Splitter;
class QUndoStack;
class QScrollArea;
class QToolButton;
class QVBoxLayout;
class QMutex;

//Natron gui
class GuiLayoutSerialization;
class GuiAppInstance;
class NodeGui;
class TabWidget;
class ToolButton;
class ViewerTab;
class DockablePanel;
class NodeGraph;
class CurveEditor;
class Histogram;
class RotoGui;
class FloatingWidget;
class BoundAction;
class ScriptEditor;
class PyPanel;
class RectI;
class DopeSheetEditor;

//Natron engine
class ViewerInstance;
class PluginGroupNode;
class KnobColor;
class ProcessHandler;
class NodeCollection;
class KnobHolder;
namespace Natron {
class Node;
class Image;
class EffectInstance;
class OutputEffectInstance;
}

class Gui;

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

    void onProjectNameChanged(const QString& name);
    
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

#endif // _Gui_FloatingWidget_h_
