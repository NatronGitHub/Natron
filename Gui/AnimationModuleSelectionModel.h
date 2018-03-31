/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

NATRON_NAMESPACE_ENTER

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

    DECLARE_FLAGS(SelectionTypeFlags, SelectionType)
    
    AnimationModuleSelectionModel(const AnimationModuleSelectionProviderPtr& model);

    virtual ~AnimationModuleSelectionModel();

    AnimationModuleSelectionProviderPtr getModel() const;

    /**
     * @brief Select all items
     **/
    void selectAll();

    /**
     * @brief Select all items keyframes
     **/
    void selectAllVisibleCurvesKeyFrames();

    /**
     * @brief Return all items in the model.
     * @param withKeyFrames if true, the keyframes will have each of the item/dim/view associated to the keyframes for this particular item.
     **/
    void getAllItems(bool withKeyFrames, AnimItemDimViewKeyFramesMap* keyframes, std::vector<NodeAnimPtr>* nodes, std::vector<TableItemAnimPtr>* tableItems) const;

    /**
     * @brief Clear all items from selection
     **/
    void clearSelection();

    /**
     * @brief Clear all keyframes but not items
     **/
    void clearKeyframesFromSelection();

    /**
     * @brief Select given items in the corresponding tree
     **/
    void selectItems(const QList<QTreeWidgetItem *> &items);

    /**
     * @brief Select all the keyframes for the given item/dim/view
     **/
    void selectKeyframes(const AnimItemBasePtr& item, DimSpec dimension, ViewSetSpec view);

    /**
     * @brief Set the selection with given flags
     **/
    void makeSelection(const AnimItemDimViewKeyFramesMap &keys,
                       const std::vector<TableItemAnimPtr>& selectedTableItems,
                       const std::vector<NodeAnimPtr>& rangeNodes,
                       SelectionTypeFlags selectionFlags);

    /**
     * @brief Returns true if the selection is empty
     **/
    bool isEmpty() const;

    
    const AnimItemDimViewKeyFramesMap& getCurrentKeyFramesSelection() const;
    
    const std::list<NodeAnimPtr>& getCurrentNodesSelection() const;
    
    const std::list<TableItemAnimPtr>& getCurrentTableItemsSelection() const;

    /**
     * @brief Returns true if all selected keyframes have the given time
     **/
    bool hasSingleKeyFrameTimeSelected(double* time) const;

    /**
     * @brief Returns the count of selected keyframes
     **/
    int getSelectedKeyframesCount() const;

    /**
     * @brief Returns true if the given keyframe is selected for the item/dimension/view
     **/
    bool isKeyframeSelectedOnCurve(const AnimItemBasePtr &item, DimIdx dimension ,ViewIdx view, TimeValue time, bool alsoCheckForExistence) const;

    bool isKeyframeSelected(const AnimItemBasePtr &item, DimSpec dimension ,ViewSetSpec view, TimeValue time) const;

    /**
     * @brief Returns whether the given node is selected
     **/
    bool isNodeSelected(const NodeAnimPtr& node) const;

    void removeAnyReferenceFromSelection(const NodeAnimPtr &removed);

    void removeAnyReferenceFromSelection(const TableItemAnimPtr& item);

    static void addAnimatedItemKeyframes(const AnimItemBasePtr& item,
                                         DimSpec dim,
                                         ViewSetSpec viewSpec,
                                         AnimItemDimViewKeyFramesMap* result);

    static void addAnimatedItemsWithoutKeyframes(const AnimItemBasePtr& item,
                                         DimSpec dim,
                                         ViewSetSpec viewSpec,
                                                 AnimItemDimViewKeyFramesMap* result);

    static void addTableItemKeyframes(const TableItemAnimPtr& item,
                                      bool withKeyFrames,
                                      bool recurse,
                                      DimSpec dim,
                                      ViewSetSpec viewSpec,
                                      std::vector<TableItemAnimPtr> *selectedTableItems,
                                      AnimItemDimViewKeyFramesMap* result);
    
    
Q_SIGNALS:

    void selectionChanged(bool recurse);

private:


    
    boost::scoped_ptr<AnimationModuleSelectionModelPrivate> _imp;
};



NATRON_NAMESPACE_EXIT

DECLARE_OPERATORS_FOR_FLAGS(NATRON_NAMESPACE::AnimationModuleSelectionModel::SelectionTypeFlags)

#endif // NATRON_GUI_ANIMATIONMODULESELECTIONMODEL_H
