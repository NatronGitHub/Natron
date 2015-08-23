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

#ifndef _Gui_FloatingWidget_h_
#define _Gui_FloatingWidget_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

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
class Color_Knob;
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
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

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
