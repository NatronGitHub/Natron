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

#ifndef NATRON_GUI_NODEGRAPH_H_
#define NATRON_GUI_NODEGRAPH_H_

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QGraphicsView>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#ifndef Q_MOC_RUN
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"

class QVBoxLayout;
class QScrollArea;
class QEvent;
class QKeyEvent;
class Gui;
class NodeGui;
class QDropEvent;
class QDragEnterEvent;
class NodeSerialization;
class NodeGuiSerialization;
class NodeBackDropSerialization;
class NodeBackDrop;
struct NodeGraphPrivate;
namespace Natron {
class Node;
}

class NodeGraph
    : public QGraphicsView, public boost::noncopyable
{
    Q_OBJECT

public:

    explicit NodeGraph(Gui* gui,
                       QGraphicsScene* scene = 0,
                       QWidget *parent = 0);

    virtual ~NodeGraph() OVERRIDE;

    void setPropertyBinPtr(QScrollArea* propertyBin);

    const std::list< boost::shared_ptr<NodeGui> > & getSelectedNodes() const;
    boost::shared_ptr<NodeGui> createNodeGUI(QVBoxLayout *dockContainer,const boost::shared_ptr<Natron::Node> & node,bool requestedByLoad,
                                             double xPosHint,double yPosHint,bool pushUndoRedoCommand);

    void selectNode(const boost::shared_ptr<NodeGui> & n,bool addToSelection);

    void selectBackDrop(NodeBackDrop* bd,bool addToSelection);

    ///The visible portion of the graph, in scene coordinates.
    QRectF visibleSceneRect();
    QRect visibleWidgetRect();

    void deselect();

    QImage getFullSceneScreenShot();

    bool areAllNodesVisible();

    /**
     * @brief Repaint the navigator
     **/
    void updateNavigator();

    const std::list<boost::shared_ptr<NodeGui> > & getAllActiveNodes() const;
    std::list<boost::shared_ptr<NodeGui> > getAllActiveNodes_mt_safe() const;

    void moveToTrash(NodeGui* node);

    void restoreFromTrash(NodeGui* node);

    QGraphicsItem* getRootItem() const;
    Gui* getGui() const;

    void discardGuiPointer();
    void discardScenePointer();

    void refreshAllEdges();

    /**
     * @brief Removes the given node from the nodegraph, using the undo/redo stack.
     **/
    void removeNode(const boost::shared_ptr<NodeGui> & node);

    void centerOnNode(const boost::shared_ptr<NodeGui> & n);

    boost::shared_ptr<NodeGui> getNodeGuiSharedPtr(const NodeGui* n) const;

    void setUndoRedoStackLimit(int limit);

    void deleteNodePermanantly(boost::shared_ptr<NodeGui> n);

    NodeBackDrop* createBackDrop(QVBoxLayout *dockContainer,bool requestedByLoad,const NodeBackDropSerialization & serialization);

    ///Returns true if it already exists
    bool checkIfBackDropNameExists(const QString & n,const NodeBackDrop* bd) const;

    bool checkIfNodeNameExists(const std::string & n,const NodeGui* node) const;

    std::list<NodeBackDrop*> getBackDrops() const;
    std::list<NodeBackDrop*> getActiveBackDrops() const;

    /**
     * @brief This function just inserts the given backdrop in the list
     **/
    void insertNewBackDrop(NodeBackDrop* bd);

    /**
     * @brief This function just removes the given backdrop from the list, it does not delete it or anything.
     **/
    void removeBackDrop(NodeBackDrop* bd);

    std::list<boost::shared_ptr<NodeGui> > getNodesWithinBackDrop(const NodeBackDrop* bd) const;

    void selectAllNodes(bool onlyInVisiblePortion);

    /**
     * @brief Calls setParentItem(NULL) on all items of the scene to avoid Qt to double delete the nodes.
     **/
    void invalidateAllNodesParenting();

    bool areKnobLinksVisible() const;

public slots:

    void deleteSelection();

    void connectCurrentViewerToSelection(int inputNB);

    void updateCacheSizeText();

    void showMenu(const QPoint & pos);

    void populateMenu();

    void toggleCacheInfos();

    void togglePreviewsForSelectedNodes();

    void toggleAutoPreview();

    void forceRefreshAllPreviews();

    void toggleKnobLinksVisible();

    void onProjectNodesCleared();

    void switchInputs1and2ForSelectedNodes();

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

    ///Called whenever the time changes on the timeline
    void onTimeChanged(SequenceTime time,int reason);

    void onNodeCreationDialogFinished();

private:


    /**
     * @brief Given the node, it tries to move it to the ideal position
     * according to the position of the selected node and its inputs/outputs.
     * This is used when creating a node to position it correctly.
     * It will move the inputs / outputs slightly to fit this node into the nodegraph
     * so they do not overlap.
     **/
    void moveNodesForIdealPosition(boost::shared_ptr<NodeGui> n);

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

    boost::scoped_ptr<NodeGraphPrivate> _imp;
};


#endif // NATRON_GUI_NODEGRAPH_H_
