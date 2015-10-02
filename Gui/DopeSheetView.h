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

#ifndef DOPESHEETVIEW_H
#define DOPESHEETVIEW_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include "Global/GlobalDefines.h"
#include "Global/Macros.h"

#include "Engine/OverlaySupport.h"

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

class DopeSheet;
class DopeSheetViewPrivate;
class DSNode;
class Gui;
class HierarchyView;
class TimeLine;


/**
 * @brief The DopeSheetView class describes the dope sheet view view of the
 * dope sheet editor.
 *
 * It strongly depends of the hierarchy view because the keyframes, clips...
 * must be drawn in front of its items.
 *
 * The dope sheet view provides the following undoable interactions :
 * - select keyframes with a click or a rectangle selection, and move them.
 * - delete the selected keyframes.
 * - move a clip and trim it.
 * - move an entire group, including its keyframes and clips.
 * - apply a spectific interpolation to the selected keyframes.
 * - copy/paste keyframes.
 *
 * and the following features :
 * - center the timeline on the current selection.
 * - move the current frame indicator.
 * - dragging the timeline using the middle mouse button.
 *
 * /!\ This class should depends less of the hierarchy view.
 */
class DopeSheetView : public QGLWidget, public OverlaySupport
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    /**
     * @brief This enum describes the current state of the dope sheet view.
     *
     * Each state, except esNoEditingState, corresponds to  an user
     * interaction.
     */
    enum EventStateEnum
    {
        esNoEditingState,
        esPickKeyframe,
        esReaderLeftTrim,
        esReaderRightTrim,
        esReaderSlip,
        esSelectionByRect,
        esMoveKeyframeSelection,
        esMoveCurrentFrameIndicator,
        esDraggingView,
        esZoomingView,
        esTransformingKeyframesMiddleRight,
        esTransformingKeyframesMiddleLeft
    };

    explicit DopeSheetView(DopeSheet *model, HierarchyView *hierarchyView,
                           Gui *gui,
                           const boost::shared_ptr<TimeLine> &timeline,
                           QWidget *parent = 0);
    
    virtual ~DopeSheetView() OVERRIDE;

    /**
     * @brief Set the current timeline range on ['xMin', 'xMax'].
     */
    void centerOn(double xMin, double xMax);

    /**
     * @brief Returns a pair composed of the first keyframe (or the starting
     * time of the first reader) displayed in the view and the last.
     *
     * This function is used to center the displayed content on a keyframe
     * selection.
     */
    std::pair<double, double> getKeyframeRange() const;


    /**
     * @brief Returns the timeline's current frame.
     */
    SequenceTime getCurrentFrame() const;

    void swapOpenGLBuffers() OVERRIDE FINAL;
    void getViewportSize(double &width, double &height) const OVERRIDE FINAL;
    void getPixelScale(double &xScale, double &yScale) const OVERRIDE FINAL;
    void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE FINAL;
    void saveOpenGLContext() OVERRIDE FINAL;
    void restoreOpenGLContext() OVERRIDE FINAL;
    unsigned int getCurrentRenderScale() const OVERRIDE FINAL;

    void refreshSelectionBboxAndRedraw();

    
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
    void mouseDoubleClickEvent(QMouseEvent *e) OVERRIDE FINAL;

    void wheelEvent(QWheelEvent *e) OVERRIDE FINAL;

    void focusInEvent(QFocusEvent *e) OVERRIDE FINAL;

public Q_SLOTS:
    /**
     * @brief Computes the timeline positions and refresh the view.
     *
     * This slot is automatically called when the current frame has changed.
     */
    void onTimeLineFrameChanged(SequenceTime sTime, int reason);

    /**
     * @brief Computes the timeline boundaries and refresh the view.
     *
     * This slot is automatically called when the frame range of the project
     * has changed.
     */
    void onTimeLineBoundariesChanged(int, int);

    /**
     * @brief Handles 'dsNode' for the next refreshes of the view.
     *
     * If its a range-based node, a new pair describing this range
     * is stored by the view.
     *
     * This slot is automatically called after the dope sheet model
     * created 'dsNode'.
     */
    void onNodeAdded(DSNode *dsNode);

    /**
     * @brief Doesn't handle 'dsNode' for the next refreshes of the view.
     *
     * This slot is automatically called just before the dope sheet model
     * remove 'dsNode'.
     */
    void onNodeAboutToBeRemoved(DSNode *dsNode);

    /**
     * @brief If the node associated with the affected keyframe is contained
     * in a node group, update the range of this group.
     *
     * This slot is automatically called when when a keyframe in the project
     * is set, removed or moved..
     */
    void onKeyframeChanged();

    /**
     * @brief Updates the range of the node associated with the modified knob
     * that emitted the signal.
     *
     * This slot is automatically called after the changing of the value of a
     * specific knob.
     */
    void onRangeNodeChanged(int, int);

    /**
     * @brief Computes the bounding rect of the selected keyframes and the ranges
     * displayed under 'item'.
     *
     * Triggers a refresh too.
     *
     * This slot is automatically called when 'item' is expanded or collapsed.
     */
    void onHierarchyViewItemExpandedOrCollapsed(QTreeWidgetItem *item);

    /**
     * @brief Triggers a refresh.
     *
     * This slot is automatically called when the user scrolls in the hierarchy
     * view.
     */
    void onHierarchyViewScrollbarMoved(int);

    /**
     * @brief Computes the bounding rect of the selected keyframes and
     * triggers a refresh.
     *
     * This slot is automatically called when a keyframe selection is changed
     * (modified, moved...).
     */
    void onKeyframeSelectionChanged();

    // Actions

    /**
     * @brief Select all the keyframes displayed in the view.
     *
     * This slot is automatically called when the user triggers the menu action
     * or presses Ctrl + A.
     */
    void onSelectedAllTriggered();

    /**
     * @brief Delete the selected keyframes.
     *
     * This slot is automatically called when the user triggers the menu action
     * or presses Del.
     */
    void deleteSelectedKeyframes();

    /**
     * @brief Center the view content on the current selection.
     *
     * If no selection exists, center on the whole project.
     *
     * This slot is automatically called when the user presses 'F'.
     */
    void centerOnSelection();

    /**
     * The following functions apply a specific interpolation to the
     * selected keyframes.
     *
     * Each slot can be called by triggering its menu action or by pressing
     * its keyboard shortcut. See GuiApplicationManager.cpp.
     */
    void constantInterpSelectedKeyframes();
    void linearInterpSelectedKeyframes();
    void smoothInterpSelectedKeyframes();
    void catmullRomInterpSelectedKeyframes();
    void cubicInterpSelectedKeyframes();
    void horizontalInterpSelectedKeyframes();
    void breakInterpSelectedKeyframes();

    /**
     * @brief Copy the selected keyframes.
     *
     * This slot is automatically called when the user triggers the menu action
     * or presses Ctrl + C.
     */
    void copySelectedKeyframes();


    /**
     * @brief Paste the keyframes existing in the clipboard. The keyframes will
     * be placed at the time of the current frame indicator.
     *
     * This slot is automatically called when the user triggers the menu action
     * or presses Ctrl + V.
     */
    void pasteKeyframes();
    

private: /* attributes */
    boost::scoped_ptr<DopeSheetViewPrivate> _imp;
};

#endif // DOPESHEETVIEW_H
