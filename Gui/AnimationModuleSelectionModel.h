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

#ifndef NATRON_GUI_ANIMATIONMODULESELECTIONMODEL_H
#define NATRON_GUI_ANIMATIONMODULESELECTIONMODEL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <QObject>

#include "Gui/AnimItemBase.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;

class AnimationModuleSelectionModelPrivate;
class AnimationModuleSelectionModel
: public QObject
{
    Q_OBJECT

public:
    enum SelectionType
    {
        SelectionTypeNoSelection = 0x0,
        SelectionTypeClear = 0x1,
        SelectionTypeAdd = 0x2,
        SelectionTypeToggle = 0x4,
        SelectionTypeRecurse = 0x8
    };

    Q_DECLARE_FLAGS(SelectionTypeFlags, SelectionType)

    AnimationModuleSelectionModel(const AnimationModulePtr& model);
    virtual ~AnimationModuleSelectionModel();

    AnimationModulePtr getModel() const;

    void selectAll();

    void clearSelection();

    void selectItems(const QList<QTreeWidgetItem *> &items);

    void makeSelection(const AnimItemDimViewKeyFramesMap &keys,
                       const std::vector<TableItemAnimPtr>& selectedTableItems,
                       const std::vector<NodeAnimPtr >& rangeNodes,
                       SelectionTypeFlags selectionFlags);

    bool isEmpty() const;

    void getCurrentSelection(AnimItemDimViewKeyFramesMap* keys, std::vector<NodeAnimPtr >* nodes, std::vector<TableItemAnimPtr>* tableItems) const;


    bool hasSingleKeyFrameTimeSelected(double* time) const;

    int getSelectedKeyframesCount() const;

    bool isKeyframeSelected(const AnimItemBasePtr &KnobAnim, DimIdx dimension ,ViewIdx view, double time) const;

    /**
     * @brief If this node has a range, such as a Reader or Group, returns whether it is selected or not.
     **/
    bool isNodeSelected(const NodeAnimPtr& node) const;

    void removeAnyReferenceFromSelection(const NodeAnimPtr &removed);

    void removeAnyReferenceFromSelection(const TableItemAnimPtr& item);

    static void addAnimatedItemKeyframes(const AnimItemBasePtr& item,
                                         DimSpec dim,
                                         ViewSetSpec viewSpec,
                                         AnimItemDimViewKeyFramesMap* result);

    static void addTableItemKeyframes(const TableItemAnimPtr& item,
                                      bool recurse,
                                      DimSpec dim,
                                      ViewSetSpec viewSpec,
                                      std::vector<TableItemAnimPtr> *selectedTableItems,
                                      AnimItemDimViewKeyFramesMap* result);


Q_SIGNALS:

    void keyframeSelectionChangedFromModel(bool recurse);

private:

    std::list<NodeAnimWPtr>::iterator isNodeSelected(const NodeAnimPtr& node);

    std::list<TableItemAnimWPtr>::iterator isTableItemSelected(const TableItemAnimPtr& node);
    
    boost::scoped_ptr<AnimationModuleSelectionModelPrivate> _imp;
};


NATRON_NAMESPACE_EXIT;

Q_DECLARE_OPERATORS_FOR_FLAGS(NATRON_NAMESPACE::AnimationModuleSelectionModel::SelectionTypeFlags)

#endif // NATRON_GUI_ANIMATIONMODULESELECTIONMODEL_H
