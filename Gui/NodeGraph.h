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

#ifndef Gui_NodeGraph_h
#define Gui_NodeGraph_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QGraphicsView>
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/NodeGraphI.h"
#include "Engine/EngineFwd.h"

#include "Gui/PanelWidget.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;

class NodeGraphPrivate;

class NodeGraph : public QGraphicsView, public NodeGraphI, public PanelWidget, public boost::noncopyable
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    explicit NodeGraph(Gui* gui,
                       const boost::shared_ptr<NodeCollection>& group,
                       QGraphicsScene* scene = 0,
                       QWidget *parent = 0);

    virtual ~NodeGraph();
    
    static void makeFullyQualifiedLabel(Node* node,std::string* ret);
    
    boost::shared_ptr<NodeCollection> getGroup() const;

    const std::list< NodeGuiPtr > & getSelectedNodes() const;
    NodeGuiPtr createNodeGUI(const NodePtr & node,
                                             const CreateNodeArgs& args);

    void selectNode(const NodeGuiPtr & n,bool addToSelection);
    
    void deselectNode(const NodeGuiPtr& n);
    
    void setSelection(const NodesGuiList& nodes);
    
    void clearSelection();

    ///The visible portion of the graph, in scene coordinates.
    QRectF visibleSceneRect() const;
    QRect visibleWidgetRect() const;

    void deselect();

    QImage getFullSceneScreenShot();

    bool areAllNodesVisible();

    /**
     * @brief Repaint the navigator
     **/
    void updateNavigator();

    const NodesGuiList & getAllActiveNodes() const;
    NodesGuiList getAllActiveNodes_mt_safe() const;

    void moveToTrash(NodeGui* node);

    void restoreFromTrash(NodeGui* node);

    QGraphicsItem* getRootItem() const;

    virtual void notifyGuiClosing() OVERRIDE FINAL;
    void discardScenePointer();


    /**
     * @brief Removes the given node from the nodegraph, using the undo/redo stack.
     **/
    void removeNode(const NodeGuiPtr & node);

    void centerOnItem(QGraphicsItem* item);

    void setUndoRedoStackLimit(int limit);

    void deleteNodePermanantly(const NodeGuiPtr& n);

    NodesGuiList getNodesWithinBackdrop(const NodeGuiPtr& node) const;

    void selectAllNodes(bool onlyInVisiblePortion);

    /**
     * @brief Calls setParentItem(NULL) on all items of the scene to avoid Qt to double delete the nodes.
     **/
    void invalidateAllNodesParenting();

    bool areKnobLinksVisible() const;
    
    void refreshNodesKnobsAtTime(SequenceTime time);
    
    void pushUndoCommand(QUndoCommand* command);
    
    bool areOptionalInputsAutoHidden() const;
    
    void copyNodesAndCreateInGroup(const NodesGuiList& nodes,
                                   const boost::shared_ptr<NodeCollection>& group,
                                   std::list<std::pair<std::string,NodeGuiPtr > >& createdNodes);

    virtual void onNodesCleared() OVERRIDE FINAL;
    
    void setLastSelectedViewer(ViewerTab* tab);
    
    ViewerTab* getLastSelectedViewer() const;
    
    /**
     * @brief Given the node, it tries to move it to the ideal position
     * according to the position of the selected node and its inputs/outputs.
     * This is used when creating a node to position it correctly.
     * It will move the inputs / outputs slightly to fit this node into the nodegraph
     * so they do not overlap.
     **/
    void moveNodesForIdealPosition(const NodeGuiPtr &n,
                                   const NodeGuiPtr& selected,
                                   bool autoConnect);
    
    void copyNodes(const NodesGuiList& nodes,NodeClipBoard& clipboard);
    
    void pasteCliboard(const NodeClipBoard& clipboard,std::list<std::pair<std::string,NodeGuiPtr > >* newNodes);
    
    void duplicateSelectedNodes(const QPointF& pos);
    void pasteNodeClipBoards(const QPointF& pos);
    void cloneSelectedNodes(const QPointF& pos);
    
    QPointF getRootPos() const;
    
    bool isDoingNavigatorRender() const;
    
public Q_SLOTS:

    void deleteSelection();

    void connectCurrentViewerToSelection(int inputNB);

    void updateCacheSizeText();

    void showMenu(const QPoint & pos);

    void toggleCacheInfo();

    void togglePreviewsForSelectedNodes();

    void toggleAutoPreview();
    
    void toggleSelectedNodesEnabled();

    void forceRefreshAllPreviews();

    void toggleKnobLinksVisible();

    void switchInputs1and2ForSelectedNodes();
    
    void extractSelectedNode();
    
    void createGroupFromSelection();
    
    void expandSelectedGroups();

    ///All these actions also work for backdrops
    /////////////////////////////////////////////
    ///Copy selected nodes to the clipboard, wiping previous clipboard
    void copySelectedNodes();

    void cutSelectedNodes();
    void pasteNodeClipBoards();
    void duplicateSelectedNodes();
    void cloneSelectedNodes();
    void decloneSelectedNodes();
    /////////////////////////////////////////////

    void centerOnAllNodes();

    void toggleConnectionHints();
    
    void toggleAutoHideInputs(bool setSettings = true);
    
    void toggleHideInputs();
        
    void onNodeCreationDialogFinished();

    void popFindDialog(const QPoint& pos = QPoint(0,0));
    
    void popRenameDialog(const QPoint& pos = QPoint(0,0));
    
    void onFindNodeDialogFinished();
    
    void refreshAllKnobsGui();
        
    void onNodeNameEditDialogFinished();
    
    void toggleAutoTurbo();
    
    void onGroupNameChanged(const QString& name);
    void onGroupScriptNameChanged(const QString& name);
    
    
    void onAutoScrollTimerTriggered();
    
private:
    
    void checkForHints(bool shiftdown, bool controlDown, const NodeGuiPtr& selectedNode,const QRectF& visibleSceneR);
    
    void moveSelectedNodesBy(bool shiftdown, bool controlDown, const QPointF& lastMousePosScene, const QPointF& newPos, const QRectF& visibleSceneR, bool userEdit);
    
    void scrollViewIfNeeded(const QPointF& scenePos);
    
    void checkAndStartAutoScrollTimer(const QPointF& scenePos);
    
    bool isNearbyNavigator(const QPoint& widgetPos,QPointF& scenePos) const;
    
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void keyReleaseEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual bool event(QEvent* e) OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseDoubleClickEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void resizeEvent(QResizeEvent* e) OVERRIDE FINAL;
    virtual void paintEvent(QPaintEvent* e) OVERRIDE FINAL;
    virtual void wheelEvent(QWheelEvent* e) OVERRIDE FINAL;
    virtual void dropEvent(QDropEvent* e) OVERRIDE FINAL;
    virtual void dragEnterEvent(QDragEnterEvent* e) OVERRIDE FINAL;
    virtual void dragMoveEvent(QDragMoveEvent* e) OVERRIDE FINAL;
    virtual void dragLeaveEvent(QDragLeaveEvent* e) OVERRIDE FINAL;
    virtual void focusInEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual void focusOutEvent(QFocusEvent* e) OVERRIDE FINAL;

private:
    
    void moveRootInternal(double dx, double dy);
    
    void wheelEventInternal(bool ctrlDown,double delta);

    boost::scoped_ptr<NodeGraphPrivate> _imp;
};


struct FindNodeDialogPrivate;
class FindNodeDialog : public QDialog
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    
    FindNodeDialog(NodeGraph* graph,QWidget* parent);
    
    virtual ~FindNodeDialog();
    
public Q_SLOTS:
    
    void onOkClicked();
    void onCancelClicked();
    
    void updateFindResults(const QString& filter);
    
    void updateFindResultsWithCurrentFilter();
    void forceUpdateFindResults();
private:
    
    
    void selectNextResult();
    
    virtual void changeEvent(QEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    
    boost::scoped_ptr<FindNodeDialogPrivate> _imp;
};

struct EditNodeNameDialogPrivate;
class EditNodeNameDialog: public QDialog
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON
    
public:
    
    EditNodeNameDialog(const NodeGuiPtr& node,QWidget* parent);
    
    virtual ~EditNodeNameDialog();
    
    QString getTypedName() const;
    
    NodeGuiPtr getNode() const;
    
private:
    
    virtual void changeEvent(QEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    
    boost::scoped_ptr<EditNodeNameDialogPrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // Gui_NodeGraph_h
