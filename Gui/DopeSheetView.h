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

#ifndef DOPESHEETVIEW_H
#define DOPESHEETVIEW_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Gui/AnimationModuleViewBase.h"

NATRON_NAMESPACE_ENTER;

class DopeSheetViewPrivate;
class DopeSheetView : public AnimationViewBase
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
        eEventStateNoEditingState,
        eEventStatePickKeyframe,
        eEventStateReaderLeftTrim,
        eEventStateReaderRightTrim,
        eEventStateReaderSlip,
        eEventStateSelectionByRect,
        eEventStateMoveKeyframeSelection,
        eEventStateMoveCurrentFrameIndicator,
        eEventStateDraggingView,
        eEventStateZoomingView,
        eEventStateDraggingMidLeftBbox,
        eEventStateDraggingMidRightBbox
    };

    explicit DopeSheetView(QWidget *parent);

    virtual ~DopeSheetView() OVERRIDE;

    virtual QSize sizeHint() const OVERRIDE FINAL;


    /**
     * @brief Returns a pair composed of the first keyframe (or the starting
     * time of the first reader) displayed in the view and the last.
     *
     * This function is used to center the displayed content on a keyframe
     * selection.
     */
    std::pair<double, double> getKeyframeRange() const;

    // Overriden from AnimationViewBase
    virtual void centerOnAllItems() OVERRIDE FINAL;
    virtual void centerOnSelection() OVERRIDE FINAL;
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE FINAL;
    virtual void refreshSelectionBoundingBox() OVERRIDE FINAL;
private:
    virtual void initializeImplementation(Gui* gui, const AnimationModuleBasePtr& model, AnimationViewBase* publicInterface) OVERRIDE FINAL;
    virtual void drawView() OVERRIDE FINAL;



public Q_SLOTS:

    /**
     * @brief Handles 'NodeAnim' for the next refreshes of the view.
     *
     * If its a range-based node, a new pair describing this range
     * is stored by the view.
     *
     * This slot is automatically called after the dope sheet model
     * created 'NodeAnim'.
     */
    void onNodeAdded(const NodeAnimPtr& NodeAnim);

    /**
     * @brief Doesn't handle 'NodeAnim' for the next refreshes of the view.
     *
     * This slot is automatically called just before the dope sheet model
     * remove 'NodeAnim'.
     */
    void onNodeAboutToBeRemoved(const NodeAnimPtr& NodeAnim);

    /**
     * @brief If the node associated with the affected keyframe is contained
     * in a node group, update the range of this group.
     *
     * This slot is automatically called when when a keyframe in the project
     * is set, removed or moved..
     */
    void onKnobAnimationChanged();

    /**
     * @brief Updates the range of the node associated with the modified knob
     * that emitted the signal.
     *
     * This slot is automatically called after the changing of the value of a
     * specific knob.
     */
    void onRangeNodeKnobChanged();

    void onAnimationTreeViewItemExpandedOrCollapsed(QTreeWidgetItem *item);

    void onAnimationTreeViewScrollbarMoved(int);


private:

    void centerOnRangeInternal(const std::pair<double, double>& range);

    void mousePressEvent(QMouseEvent *e) OVERRIDE FINAL;
    void mouseMoveEvent(QMouseEvent *e) OVERRIDE FINAL;
    void mouseReleaseEvent(QMouseEvent *e) OVERRIDE FINAL;
    void mouseDoubleClickEvent(QMouseEvent *e) OVERRIDE FINAL;

    void wheelEvent(QWheelEvent *e) OVERRIDE FINAL;

private: /* attributes */
    boost::shared_ptr<DopeSheetViewPrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // DOPESHEETVIEW_H
