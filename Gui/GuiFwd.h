/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#ifndef Natron_Gui_GuiFwd_h
#define Natron_Gui_GuiFwd_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/EngineFwd.h"

#ifdef Q_OS_DARWIN
// for dockClickHandler
#include <objc/objc.h> // defines OBJC_OLD_DISPATCH_PROTOTYPES
#endif

// Qt
#include <QtCore/QtGlobal>

class QAbstractButton;
class QAction;
class QCheckBox;
class QColor;
class QCursor;
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
typedef std::shared_ptr<QUndoStack> QUndoStackPtr;

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
class QOpenGLShaderProgram;
#endif

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
class BezierCPCurveGui;
class BoundAction;
class Button;
class ChannelsComboBox;
class ClickableLabel;
class ColoredFrame;
class ComboBox;
class ComboBoxMenuNode;
class CurveEditor;
class CurveGui;
class CurveSelection;
class CurveWidget;
class DSKnob;
class DSNode;
class DefaultInteractI;
class DialogButtonBox;
class DockablePanel;
class DocumentationManager;
class DopeSheet;
class DopeSheetEditor;
class DopeSheetKey;
class DopeSheetView;
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
class GuiLayoutSerialization;
class HierarchyView;
class Histogram;
class HostOverlay;
class InfoViewerWidget;
class InspectorNode;
class KeyBoundAction;
class KeybindRecorder;
class KnobAnim;
class KnobClickableLabel;
class KnobCurveGui;
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
class KnobPageGui;
class KnobWidgetDnD;
class KnobsHolderAnimBase;
class Label;
class LineEdit;
class LinkArrow;
class Menu;
class MenuWithToolTips;
class MultiInstancePanel;
class NodeAnim;
class NodeBackdropSerialization;
class NodeClipBoard;
class NodeCollection;
class NodeCurveEditorElement;
class NodeGraph;
class NodeGraphPixmapItem;
class NodeGraphRectItem;
class NodeGraphTextItem;
class NodeGui;
class NodeGuiI;
class NodeGuiIndicator;
class NodeGuiSerialization;
class NodeSerialization;
class NodeSettingsPanel;
class NodeViewerContext;
class PanelWidget;
class PreferencesPanel;
class ProgressPanel;
class ProgressTaskInfo;
class ProjectGui;
class ProjectGuiSerialization;
class PropertiesBinWrapper;
class PythonPanelSerialization;
class RectI;
class RenderStatsDialog;
class RotoGui;
class RotoGuiSharedData;
class RotoPanel;
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
class TrackerGui;
class TrackerPanel;
class TrackerPanelV1;
class VerticalColorBar;
class ViewerGL;
class ViewerTab;
class ViewerToolButton;
class KnobPageGui;

#ifdef Q_OS_DARWIN
//Implementation in Gui/QtMac.mm
namespace QtMac {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
qreal devicePixelRatioInternal(const QWidget* w);
#endif
#if OBJC_OLD_DISPATCH_PROTOTYPES != 1
void setupDockClickHandler(void (*)(void));
#endif
}
#endif

NATRON_PYTHON_NAMESPACE_ENTER
class GuiApp;
class PyModalDialog;
class PyPanel;
class PyTabWidget;
class IntParam;
class StringParam;
typedef std::shared_ptr<StringParam> StringParamPtr;
typedef std::shared_ptr<IntParam> IntParamPtr;
typedef std::shared_ptr<PyModalDialog> PyModalDialogPtr;
NATRON_PYTHON_NAMESPACE_EXIT

typedef std::shared_ptr<AnimItemBase> AnimItemBasePtr;
typedef std::shared_ptr<AnimationModule> AnimationModulePtr;
typedef std::shared_ptr<AnimationModuleBase> AnimationModuleBasePtr;
typedef std::shared_ptr<AnimationModuleSelectionModel> AnimationModuleSelectionModelPtr;
typedef std::shared_ptr<AnimationModuleSelectionProvider> AnimationModuleSelectionProviderPtr;
typedef std::shared_ptr<BackdropGui> BackdropGuiPtr;
typedef std::shared_ptr<BezierCPCurveGui> BezierCPCurveGuiPtr;
typedef std::shared_ptr<ComboBoxMenuNode> ComboBoxMenuNodePtr;
typedef std::shared_ptr<CurveGui> CurveGuiPtr;
typedef std::shared_ptr<DSKnob> DSKnobPtr;
typedef std::shared_ptr<DSNode> DSNodePtr;
typedef std::shared_ptr<DefaultInteractI> DefaultInteractIPtr;
typedef std::shared_ptr<DopeSheetKey> DopeSheetKeyPtr;
typedef std::shared_ptr<DroppedTreeItem> DroppedTreeItemPtr;
typedef std::shared_ptr<FileDialogPreviewProvider> FileDialogPreviewProviderPtr;
typedef std::shared_ptr<GuiAppInstance> GuiAppInstancePtr;
typedef std::shared_ptr<HostOverlay> HostOverlayPtr;
typedef std::shared_ptr<InspectorNode> InspectorNodePtr;
typedef std::shared_ptr<KnobAnim> KnobAnimPtr;
typedef std::shared_ptr<KnobCurveGui> KnobCurveGuiPtr;
typedef std::shared_ptr<KnobGui const> KnobGuiConstPtr;
typedef std::shared_ptr<KnobGui> KnobGuiPtr;
typedef std::shared_ptr<KnobGuiColor> KnobGuiColorPtr;
typedef std::shared_ptr<KnobGuiDouble> KnobGuiDoublePtr;
typedef std::shared_ptr<KnobGuiInt> KnobGuiIntPtr;
typedef std::shared_ptr<KnobGuiWidgets> KnobGuiWidgetsPtr;
typedef std::shared_ptr<KnobItemsTableGui> KnobItemsTableGuiPtr;
typedef std::shared_ptr<KnobPageGui> KnobPageGuiPtr;
typedef std::shared_ptr<KnobWidgetDnD> KnobWidgetDnDPtr;
typedef std::shared_ptr<KnobsHolderAnimBase> KnobsHolderAnimBasePtr;
typedef std::shared_ptr<MultiInstancePanel> MultiInstancePanelPtr;
typedef std::shared_ptr<NodeAnim> NodeAnimPtr;
typedef std::shared_ptr<NodeGui> NodeGuiPtr;
typedef std::shared_ptr<NodeGuiI> NodeGuiIPtr;
typedef std::shared_ptr<NodeGuiIndicator> NodeGuiIndicatorPtr;
typedef std::shared_ptr<NodeGuiSerialization> NodeGuiSerializationPtr;
typedef std::shared_ptr<NodeViewerContext> NodeViewerContextPtr;
typedef std::shared_ptr<ProgressTaskInfo> ProgressTaskInfoPtr;
typedef std::shared_ptr<PythonPanelSerialization> PythonPanelSerializationPtr;
typedef std::shared_ptr<SelectedKey> SelectedKeyPtr;
typedef std::shared_ptr<TableItem> TableItemPtr;
typedef std::shared_ptr<TableItemAnim> TableItemAnimPtr;
typedef std::shared_ptr<TableModel> TableModelPtr;
typedef std::shared_ptr<const TableItem> TableItemConstPtr;
typedef std::weak_ptr<AnimItemBase> AnimItemBaseWPtr;
typedef std::weak_ptr<AnimationModule> AnimationModuleWPtr;
typedef std::weak_ptr<AnimationModuleBase> AnimationModuleBaseWPtr;
typedef std::weak_ptr<AnimationModuleSelectionModel> AnimationModuleSelectionModelWPtr;
typedef std::weak_ptr<AnimationModuleSelectionProvider> AnimationModuleSelectionProviderWPtr;
typedef std::weak_ptr<DSKnob> DSKnobWPtr;
typedef std::weak_ptr<DSNode> DSNodeWPtr;
typedef std::weak_ptr<GuiAppInstance> GuiAppInstanceWPtr;
typedef std::weak_ptr<KnobAnim> KnobAnimWPtr;
typedef std::weak_ptr<KnobCurveGui> KnobCurveGuiWPtr;
typedef std::weak_ptr<KnobGui> KnobGuiWPtr;
typedef std::weak_ptr<KnobGuiWidgets> KnobGuiWidgetsWPtr;
typedef std::weak_ptr<KnobItemsTableGui> KnobItemsTableGuiWPtr;
typedef std::weak_ptr<KnobPageGui> KnobPageGuiWPtr;
typedef std::weak_ptr<KnobsHolderAnimBase> KnobsHolderAnimBaseWPtr;
typedef std::weak_ptr<NodeAnim> NodeAnimWPtr;
typedef std::weak_ptr<NodeGui> NodeGuiWPtr;
typedef std::weak_ptr<NodeViewerContext> NodeViewerContextWPtr;
typedef std::weak_ptr<TableItem> TableItemWPtr;
typedef std::weak_ptr<TableItemAnim> TableItemAnimWPtr;
typedef std::weak_ptr<TableModel> TableModelWPtr;
typedef std::list<NodeGuiPtr> NodesGuiList;
typedef std::list<NodeGuiWPtr> NodesGuiWList;

NATRON_NAMESPACE_EXIT


#endif // Natron_Gui_GuiFwd_h
