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
class QFontMetrics;
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
class QMessageBox;
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
typedef boost::shared_ptr<QUndoStack> QUndoStackPtr;

// Natron Gui
NATRON_NAMESPACE_ENTER
class AboutWindow;
class ActionWithShortcut;
class AnimItemBase;
class AnimatedCheckBox;
class AnimationModule;
class AnimationModuleBase;
class AnimationModuleEditor;
class AnimationModuleSelectionModel;
class AnimationModuleSelectionProvider;
class AnimationModuleTreeView;
class AnimationModuleView;
class BackdropGui;
class BoundAction;
class Button;
class ClickableLabel;
class ColoredFrame;
class ComboBox;
class CurveGui;
class DialogButtonBox;
class DockablePanel;
class DocumentationManager;
class DroppedTreeItem;
class DroppedTreeItem;
class Edge;
class FileDialogPreviewProvider;
class FloatingWidget;
class GeneralProgressDialog;
class GroupBoxLabel;
class GroupKnobDialog;
class Gui;
class GuiAppInstance;
class GuiGLContext;
class Histogram;
class Histogram;
class HostOverlay;
class InfoViewerWidget;
class KeybindRecorder;
class KnobAnim;
class KnobClickableLabel;
class KnobGui;
class KnobGuiColor;
class KnobGuiContainerHelper;
class KnobGuiContainerI;
class KnobGuiDouble;
class KnobGuiFactory;
class KnobGuiInt;
class KnobGuiLayers;
class KnobGuiTable;
class KnobGuiWidgets;
class KnobHolder;
class KnobItemsTableGui;
class KnobWidgetDnD;
class KnobsHolderAnimBase;
class Label;
class LineEdit;
class LinkArrow;
class Menu;
class MenuWithToolTips;
class NodeAnim;
class NodeCollection;
class NodeGraph;
class NodeGraphPixmapItem;
class NodeGraphRectItem;
class NodeGraphTextItem;
class NodeGui;
class NodeGuiI;
class NodeGuiIndicator;
class NodeSettingsPanel;
class NodeViewerContext;
class PanelWidget;
class PreferencesPanel;
class ProgressPanel;
class ProgressTaskInfo;
class ProjectGui;
class PropertiesBinWrapper;
class PythonPanelSerialization;
class RectI;
class RenderStatsDialog;
class ScaleSliderQWidget;
class ScriptEditor;
class ScriptObject;
class SelectedKey;
class SequenceFileDialog;
class SequenceItemDelegate;
class SpinBox;
class SpinBoxValidator;
class SplashScreen;
class Splitter;
class TabGroup;
class TabWidget;
class TableItem;
class TableItemAnim;
class TableModel;
class TableView;
class TimeLineGui;
class ToolButton;
class VerticalColorBar;
class ViewerGL;
class ViewerTab;
class ViewerToolButton;
class KnobPageGui;
//Implementation in Gui/QtMac.mm
namespace QtMac {
double getHighDPIScaleFactorInternal(const QWidget* w);
}


typedef boost::shared_ptr<AnimItemBase> AnimItemBasePtr;
typedef boost::shared_ptr<AnimationModule> AnimationModulePtr;
typedef boost::shared_ptr<AnimationModuleBase> AnimationModuleBasePtr;
typedef boost::shared_ptr<AnimationModuleSelectionModel> AnimationModuleSelectionModelPtr;
typedef boost::shared_ptr<AnimationModuleSelectionProvider> AnimationModuleSelectionProviderPtr;
typedef boost::shared_ptr<BackdropGui> BackdropGuiPtr;
typedef boost::shared_ptr<CurveGui> CurveGuiPtr;
typedef boost::shared_ptr<FileDialogPreviewProvider> FileDialogPreviewProviderPtr;
typedef boost::shared_ptr<GuiAppInstance> GuiAppInstancePtr;
typedef boost::shared_ptr<KnobAnim> KnobAnimPtr;
typedef boost::shared_ptr<KnobGui const> KnobGuiConstPtr;
typedef boost::shared_ptr<KnobGui> KnobGuiPtr;
typedef boost::shared_ptr<KnobGuiColor> KnobGuiColorPtr;
typedef boost::shared_ptr<KnobGuiDouble> KnobGuiDoublePtr;
typedef boost::shared_ptr<KnobGuiInt> KnobGuiIntPtr;
typedef boost::shared_ptr<KnobGuiWidgets> KnobGuiWidgetsPtr;
typedef boost::shared_ptr<KnobItemsTableGui> KnobItemsTableGuiPtr;
typedef boost::shared_ptr<KnobWidgetDnD> KnobWidgetDnDPtr;
typedef boost::shared_ptr<KnobsHolderAnimBase> KnobsHolderAnimBasePtr;
typedef boost::shared_ptr<NodeAnim> NodeAnimPtr;
typedef boost::shared_ptr<NodeGui> NodeGuiPtr;
typedef boost::shared_ptr<NodeGuiI> NodeGuiIPtr;
typedef boost::shared_ptr<NodeGuiIndicator> NodeGuiIndicatorPtr;
typedef boost::shared_ptr<NodeViewerContext> NodeViewerContextPtr;
typedef boost::shared_ptr<ProgressTaskInfo> ProgressTaskInfoPtr;
typedef boost::shared_ptr<TableItem> TableItemPtr;
typedef boost::shared_ptr<TableItemAnim> TableItemAnimPtr;
typedef boost::shared_ptr<TableModel> TableModelPtr;
typedef boost::shared_ptr<const TableItem> TableItemConstPtr;
typedef boost::weak_ptr<AnimItemBase> AnimItemBaseWPtr;
typedef boost::weak_ptr<AnimationModule> AnimationModuleWPtr;
typedef boost::weak_ptr<AnimationModuleBase> AnimationModuleBaseWPtr;
typedef boost::weak_ptr<AnimationModuleSelectionModel> AnimationModuleSelectionModelWPtr;
typedef boost::weak_ptr<AnimationModuleSelectionProvider> AnimationModuleSelectionProviderWPtr;
typedef boost::weak_ptr<GuiAppInstance> GuiAppInstanceWPtr;
typedef boost::weak_ptr<KnobAnim> KnobAnimWPtr;
typedef boost::weak_ptr<KnobGui> KnobGuiWPtr;
typedef boost::weak_ptr<KnobGuiWidgets> KnobGuiWidgetsWPtr;
typedef boost::weak_ptr<KnobItemsTableGui> KnobItemsTableGuiWPtr;
typedef boost::weak_ptr<KnobPageGui> KnobPageGuiWPtr;
typedef boost::weak_ptr<KnobsHolderAnimBase> KnobsHolderAnimBaseWPtr;
typedef boost::weak_ptr<NodeAnim> NodeAnimWPtr;
typedef boost::weak_ptr<NodeGui> NodeGuiWPtr;
typedef boost::weak_ptr<NodeViewerContext> NodeViewerContextWPtr;
typedef boost::weak_ptr<TableItem> TableItemWPtr;
typedef boost::weak_ptr<TableItemAnim> TableItemAnimWPtr;
typedef boost::weak_ptr<TableModel> TableModelWPtr;
typedef std::list<NodeGuiPtr> NodesGuiList;
typedef std::list<NodeGuiWPtr> NodesGuiWList;


NATRON_PYTHON_NAMESPACE_ENTER

class DialogParamHolder;
class GuiApp;
class PyModalDialog;
class PyPanel;
class PyTabWidget;

typedef boost::shared_ptr<DialogParamHolder> DialogParamHolderPtr;

NATRON_PYTHON_NAMESPACE_EXIT


NATRON_NAMESPACE_EXIT


#endif // Natron_Gui_GuiFwd_h
