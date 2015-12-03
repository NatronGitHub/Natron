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

#ifndef Natron_Gui_GuiFwd_h
#define Natron_Gui_GuiFwd_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/EngineFwd.h"

// Qt

class QAction;
class QCheckBox;
class QCursor;
class QDragEnterEvent;
class QDragLeaveEvent;
class QDropEvent;
class QEvent;
class QFont;
class QGraphicsLineItem;
class QGraphicsPolygonItem;
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
class QGraphicsSimpleTextItem;
class QGraphicsTextItem;
class QGridLayout;
class QHBoxLayout;
class QItemSelection;
class QKeyEvent;
class QLinearGradient;
class QMenu;
class QMenuBar;
class QModelIndex;
class QMouseEvent;
class QMutex;
class QPainterPath;
class QPixmap;
class QPointF;
class QProgressDialog;
class QRectF;
class QScrollArea;
class QSplitter;
class QStyleOptionViewItem;
class QTabWidget;
class QTextBrowser;
class QToolButton;
class QTreeWidget;
class QTreeWidgetItem;
class QUndoCommand;
class QUndoGroup;
class QUndoStack;
class QVBoxLayout;
class QWidget;


// Natron Gui

class AboutWindow;
class ActionWithShortcut;
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
class Edge;
class FloatingWidget;
class Histogram;
class Gui;
class GuiAppInstance;
class GuiLayoutSerialization;
class HierarchyView;
class Histogram;
class HostOverlay;
class InfoViewerWidget;
class KeyBoundAction;
class KnobGui;
class KnobGuiFactory;
class KnobHolder;
class LineEdit;
class LinkArrow;
class MenuWithToolTips;
class MultiInstancePanel;
class NodeBackDropSerialization;
class NodeClipBoard;
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
class PropertiesBinWrapper;
class PyModalDialog;
class PyPanel;
class PyTabWidget;
class RectI;
class RenderStatsDialog;
class RotoGui;
class RotoPanel;
class ScaleSliderQWidget;
class ScriptEditor;
class ShortCutEditor;
class SpinBox;
class SplashScreen;
class Splitter;
class TabGroup;
class TabWidget;
class TableModel;
class TableView;
class TimeLineGui;
class ToolButton;
class TrackerGui;
class VerticalColorBar;
class ViewerGL;
class ViewerTab;

namespace Natron {
class ClickableLabel;
class Label;
class Menu;
#if defined(Q_OS_MAC)
//Implementation in Gui/QtMac.mm
bool isHighDPIInternal(const QWidget* w);
#endif

}


#endif // Natron_Gui_GuiFwd_h
