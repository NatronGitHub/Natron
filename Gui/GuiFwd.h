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

#ifndef Natron_Gui_GuiFwd_h
#define Natron_Gui_GuiFwd_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/EngineFwd.h"

// Qt

class QAbstractButton;
class QAction;
class QCheckBox;
class QColor;
class QCursor;
class QDialogButtonBox;
class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;
class QEvent;
class QFileSystemModel;
class QFileSystemWatcher;
class QFont;
class QFontComboBox;
class QFrame;
class QGLShaderProgram;
class QGraphicsLineItem;
class QGraphicsPolygonItem;
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
class QGraphicsSimpleTextItem;
class QGraphicsTextItem;
class QGridLayout;
class QHBoxLayout;
class QIcon;
class QInputEvent;
class QItemSelection;
class QKeyEvent;
class QLinearGradient;
class QListView;
class QMenu;
class QMenuBar;
class QModelIndex;
class QMouseEvent;
class QMutex;
class QPaintEvent;
class QPainterPath;
class QPixmap;
class QPoint;
class QPointF;
class QProgressDialog;
class QRectF;
class QScrollArea;
class QSplitter;
class QStyleOptionViewItem;
class QTabWidget;
class QTextBrowser;
class QTextCharFormat;
class QToolBar;
class QToolButton;
class QTreeWidget;
class QTreeWidgetItem;
class QUndoCommand;
class QUndoGroup;
class QUndoStack;
class QVBoxLayout;
class QWheelEvent;
class QWidget;


// Natron Gui
NATRON_NAMESPACE_ENTER;
class AboutWindow;
class ActionWithShortcut;
class AnimatedCheckBox;
class AnimationButton;
class BoundAction;
class Button;
class ChannelsComboBox;
class ComboBox;
class CurveEditor;
class CurveGui;
class CurveSelection;
class CurveWidget;
class DSKnob;
class DSNode;
class DockablePanel;
class DopeSheet;
class DopeSheetEditor;
class DopeSheetKey;
class DopeSheetView;
class DroppedTreeItem;
class Edge;
class FileDialogPreviewProvider;
class FloatingWidget;
class Histogram;
class GeneralProgressDialog;
class Gui;
class GuiApp;
class GuiAppInstance;
class GuiLayoutSerialization;
class HierarchyView;
class Histogram;
class HostOverlay;
class InfoViewerWidget;
class KeyBoundAction;
class KnobClickableLabel;
class KnobCurveGui;
class KnobGui;
class KnobGuiFactory;
class KnobHolder;
class LineEdit;
class LinkArrow;
class MenuWithToolTips;
class MultiInstancePanel;
class NodeBackdropSerialization;
class NodeClipBoard;
class NodeCollection;
class NodeCurveEditorElement;
class NodeGraph;
class NodeGraphPixmapItem;
class NodeGraphTextItem;
class NodeGui;
class NodeGuiSerialization;
class NodeSerialization;
class NodeSettingsPanel;
class PanelWidget;
class PreferencesPanel;
class ProjectGui;
class ProjectGuiSerialization;
class ProgressPanel;
class ProgressTaskInfo;
class PropertiesBinWrapper;
class PyModalDialog;
class PyPanel;
class PyTabWidget;
class RectI;
class RenderStatsDialog;
class RotoGui;
class RotoGuiSharedData;
class RotoPanel;
class ScaleSliderQWidget;
class ScriptEditor;
class ScriptObject;
class SequenceFileDialog;
class SequenceItemDelegate;
class ShortCutEditor;
class SpinBox;
class SpinBoxValidator;
class SplashScreen;
class Splitter;
class TabGroup;
class TabWidget;
class TableItem;
class TableModel;
class TableView;
class Texture;
class TimeLineGui;
class ToolButton;
class TrackerGui;
class TrackerPanel;
class VerticalColorBar;
class ViewerGL;
class ViewerTab;
class ClickableLabel;
class GroupBoxLabel;
class Label;
class Menu;
//Implementation in Gui/QtMac.mm
namespace QtMac {
bool isHighDPIInternal(const QWidget* w);
}

typedef boost::shared_ptr<NodeGui> NodeGuiPtr;
typedef std::list<NodeGuiPtr> NodesGuiList;
typedef boost::shared_ptr<ProgressTaskInfo> ProgressTaskInfoPtr;

NATRON_NAMESPACE_EXIT;


#endif // Natron_Gui_GuiFwd_h
