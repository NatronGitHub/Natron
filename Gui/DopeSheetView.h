#ifndef DOPESHEETVIEW_H
#define DOPESHEETVIEW_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtGui/QTreeWidget>
#include <QtOpenGL/QGLWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"

#include "Engine/OverlaySupport.h"

class DopeSheet;
class DopeSheetViewPrivate;
class DSNode;
class DSKnob;
class Gui;
class HierarchyViewPrivate;
class QModelIndex;
class QStyleOptionViewItem;
class TimeLine;


/**
 * @brief The HierarchyView class
 *
 *
 */
class HierarchyView : public QTreeWidget
{
    Q_OBJECT

public:
    friend class HierarchyViewItemDelegate;

    explicit HierarchyView(DopeSheet *model, Gui *gui, QWidget *parent = 0);
    ~HierarchyView();

    template <typename T>
    QRect getItemRect(const T *dopeSheetContext) const
    {
        return visualItemRect(dopeSheetContext->getTreeItem());
    }

    DSKnob *getDSKnobAt(int y) const;

private Q_SLOTS:
    void onNodeAdded(DSNode *dsNode);
    void onNodeAboutToBeRemoved(DSNode *dsNode);
    void onKeyframeSetOrRemoved(DSKnob *dsKnob);
    void onNodeSettingsPanelOpened(DSNode *dsNode);
    void onGroupNodeSettingsPanelCloseChanged(DSNode *dsNode);

    void onItemSelectionChanged();
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);

protected:
    virtual void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const OVERRIDE FINAL;
    virtual void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const OVERRIDE FINAL;

private:
    boost::scoped_ptr<HierarchyViewPrivate> _imp;
};


/**
 * @brief The DopeSheetView class
 *
 *
 */
class DopeSheetView : public QGLWidget, public OverlaySupport
{
    Q_OBJECT

public:
    enum EventStateEnum
    {
        esNoEditingState,
        esPickKeyframe,
        esReaderRepos,
        esReaderLeftTrim,
        esReaderRightTrim,
        esReaderSlip,
        esGroupRepos,
        esSelectionByRect,
        esMoveKeyframeSelection,
        esMoveCurrentFrameIndicator,
        esDraggingView
    };

    explicit DopeSheetView(DopeSheet *model, HierarchyView *hierarchyView,
                           Gui *gui,
                           const boost::shared_ptr<TimeLine> &timeline,
                           QWidget *parent = 0);
    ~DopeSheetView();

    void frame(double xMin, double xMax);

    SequenceTime getCurrentFrame() const;

    void swapOpenGLBuffers() OVERRIDE FINAL;
    void getViewportSize(double &width, double &height) const OVERRIDE FINAL;
    void getPixelScale(double &xScale, double &yScale) const OVERRIDE FINAL;
    void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE FINAL;
    void saveOpenGLContext() OVERRIDE FINAL;
    void restoreOpenGLContext() OVERRIDE FINAL;
    unsigned int getCurrentRenderScale() const OVERRIDE FINAL;

public Q_SLOTS:
    void redraw() OVERRIDE FINAL;

protected:
    void initializeGL() OVERRIDE FINAL;
    void resizeGL(int w, int h) OVERRIDE FINAL;
    void paintGL() OVERRIDE FINAL;

    /* events */
    void mousePressEvent(QMouseEvent *e) OVERRIDE FINAL;
    void mouseMoveEvent(QMouseEvent *e) OVERRIDE FINAL;
    void mouseReleaseEvent(QMouseEvent *e) OVERRIDE FINAL;

    void wheelEvent(QWheelEvent *e) OVERRIDE FINAL;

    void enterEvent(QEvent *e) OVERRIDE FINAL;
    void focusInEvent(QFocusEvent *e) OVERRIDE FINAL;

    void keyPressEvent(QKeyEvent *e) OVERRIDE FINAL;

private Q_SLOTS:
    void onTimeLineFrameChanged(SequenceTime sTime, int reason);
    void onTimeLineBoundariesChanged(int, int);

    void onNodeAdded(DSNode *dsNode);
    void onNodeAboutToBeRemoved(DSNode *dsNode);

    void onKeyframeChanged();
    void onRangeNodeChanged(int, int);

    void onHierarchyViewItemExpandedOrCollapsed(QTreeWidgetItem *item);
    void onHierarchyViewScrollbarMoved(int);

    void onGroupNodeSettingsPanelCloseChanged(DSNode *dsNode);
    void onKeyframeSelectionChanged();

    // Actions
    void selectAllKeyframes();
    void deleteSelectedKeyframes();
    void frame();

    void constantInterpSelectedKeyframes();
    void linearInterpSelectedKeyframes();
    void smoothInterpSelectedKeyframes();
    void catmullRomInterpSelectedKeyframes();
    void cubicInterpSelectedKeyframes();
    void horizontalInterpSelectedKeyframes();
    void breakInterpSelectedKeyframes();

    void copySelectedKeyframes();
    void pasteKeyframes();

private: /* attributes */
    boost::scoped_ptr<DopeSheetViewPrivate> _imp;
};

#endif // DOPESHEETVIEW_H
