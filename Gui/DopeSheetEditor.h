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

#ifndef DOPESHEETEDITOR_H
#define DOPESHEETEDITOR_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtGui/QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/PanelWidget.h"
#include "Gui/GuiFwd.h"

class DopeSheetEditorPrivate;

/**
 * @class DopeSheetEditor
 *
 * The DopeSheetEditor class provides several widgets to edit keyframe animations
 * and video clips.
 *
 * It contains two main widgets : at left, the hierarchy view provides a tree
 * representation of the animated parameters (knobs) of each opened node.
 * At right, the dope sheet view is an OpenGL widget displaying horizontally the
 * keyframes of these knobs.
 *
 * Visually, the editor looks like a single tool with horizontally layered
 * datas.
 *
 * The user can select, move, delete the keyframes and set their interpolation.
 * He can move and trim clips too.
 *
 * The DopeSheetEditor class acts like a facade. It's the only class visible
 * from the outside. It means that if you want to add a feature which require
 * some interaction with the other tools of Natron, you must add its "entry
 * point" here.
 *
 * For example the 'centerOn' function is called from the CurveWidget and
 * the TimeLineGui classes, and calls the 'centerOn' method of the
 * DopeSheetView class.
 *
 * Internally, this class is a composition of the following classes :
 * - DopeSheet -> it's the main model of the dope sheet editor
 * - HierarchyView -> describes the hierarchy view
 * - DopeSheetView -> describes the dope sheet view
 *
 * These two views query the model to display and modify data.
 */
class DopeSheetEditor : public QWidget, public PanelWidget
{

public:
    DopeSheetEditor(Gui *gui, boost::shared_ptr<TimeLine> timeline, QWidget *parent = 0);
    ~DopeSheetEditor();

    /**
     * @brief Tells to the dope sheet model to add 'nodeGui' to the dope sheet
     * editor.
     *
     * The model will decide if the node should be accepted or discarded.
     */
    void addNode(boost::shared_ptr<NodeGui> nodeGui);

    /**
     * @brief Tells to the dope sheet model to remove 'node' from the dope
     * sheet editor.
     */
    void removeNode(NodeGui *node);

    /**
     * @brief Center the content of the dope sheet view on the timeline range
     * ['xMin', 'xMax'].
     */
    void centerOn(double xMin, double xMax);
    
    void refreshSelectionBboxAndRedrawView();
    
    int getTimelineCurrentTime() const;
    
    DopeSheetView* getDopesheetView() const;
    
    void setTreeWidgetWidth(int width);
    
    int getTreeWidgetWidth() const;
    
    void onInputEventCalled();

private:
    
    virtual void enterEvent(QEvent *e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent *e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    
    boost::scoped_ptr<DopeSheetEditorPrivate> _imp;
};

#endif // DOPESHEETEDITOR_H
