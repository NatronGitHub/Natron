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


// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****
#include "AnimationModuleSelectionModel.h"

#include "Engine/KnobItemsTable.h"

#include "Gui/AnimationModule.h"
#include "Gui/KnobAnim.h"
#include "Gui/NodeAnim.h"
#include "Gui/TableItemAnim.h"

NATRON_NAMESPACE_ENTER;

class AnimationModuleSelectionModelPrivate
{
public:
    AnimationModuleSelectionModelPrivate(const AnimationModulePtr& model)
    : model(model)
    , selectedKeyframes()
    , selectedTableItems()
    , selectedNodes()
    {
    }


    /* attributes */
    AnimationModuleWPtr model;
    AnimItemDimViewKeyFramesMap selectedKeyframes;
    std::list<TableItemAnimWPtr> selectedTableItems;
    std::list<NodeAnimWPtr> selectedNodes;
};

AnimationModuleSelectionModel::AnimationModuleSelectionModel(const AnimationModulePtr& model)
: _imp(new AnimationModuleSelectionModelPrivate(model))
{
}

AnimationModuleSelectionModel::~AnimationModuleSelectionModel()
{
}

AnimationModulePtr
AnimationModuleSelectionModel::getModel() const
{
    return _imp->model.lock();
}

static void addTableItemKeyframesRecursive(const TableItemAnimPtr& item, std::vector<TableItemAnimPtr> *selectedTableItems, KeyFrameSet* result)
{
    if (item->isRangeDrawingEnabled()) {
        selectedTableItems->push_back(item);
    }
    item->getKeyframes(DimSpec::all(), ViewSetSpec::all(), result);
    std::vector<TableItemAnimPtr> children = item->getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        addTableItemKeyframesRecursive(children[i], selectedTableItems, result);
    }
}

void
AnimationModuleSelectionModel::selectAll()
{
    const std::list<NodeAnimPtr>& nodeRows = getModel()->getNodes();

    AnimItemDimViewKeyFramesMap selectedKeyframes;
    std::vector<NodeAnimPtr > selectedNodes;
    std::vector<TableItemAnimPtr> selectedTableItems;

    for (std::list<NodeAnimPtr>::const_iterator it = nodeRows.begin(); it != nodeRows.end(); ++it) {

        const std::vector<KnobAnimPtr>& knobs = (*it)->getKnobs();

        for (std::vector<KnobAnimPtr>::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {

            // We select keyframes for the root item, this will also select children
            AnimItemDimViewID key(*it2, ViewSetSpec::all(), DimSpec::all());
            KeyFrameSet &keysForItem = selectedKeyframes[key];
            (*it2)->getKeyframes(DimSpec::all(), ViewSetSpec::all(), &keysForItem);
        }

        const std::vector<TableItemAnimPtr>& topLevelTableItems = (*it)->getTopLevelItems();
        for (std::vector<TableItemAnimPtr>::const_iterator it2 = topLevelTableItems.begin(); it2 != topLevelTableItems.end(); ++it2) {
            // We select keyframes for the root item, this will also select children
            AnimItemDimViewID key(*it2, ViewSetSpec::all(), DimSpec::all());
            KeyFrameSet &keysForItem = selectedKeyframes[key];

            addTableItemKeyframesRecursive(*it2, &selectedTableItems, &keysForItem);
        }
        if ((*it)->isRangeDrawingEnabled()) {
            selectedNodes.push_back(*it);
        }


    }

    makeSelection( selectedKeyframes, selectedTableItems, selectedNodes, (AnimationModuleSelectionModel::SelectionTypeAdd |
                                                               AnimationModuleSelectionModel::SelectionTypeClear |
                                                               AnimationModuleSelectionModel::SelectionTypeRecurse) );
}



void
AnimationModuleSelectionModel::clearSelection()
{
    if ( _imp->selectedKeyframes.empty() && _imp->selectedNodes.empty() ) {
        return;
    }

    makeSelection(AnimItemDimViewKeyFramesMap(), std::vector<TableItemAnimPtr>(), std::vector<NodeAnimPtr >(), AnimationModuleSelectionModel::SelectionTypeClear);
}

void
AnimationModuleSelectionModel::makeSelection(const AnimItemDimViewKeyFramesMap &keys,
                                             const std::vector<TableItemAnimPtr>& selectedTableItems,
                                             const std::vector<NodeAnimPtr >& nodes,
                                             SelectionTypeFlags selectionFlags)
{
    // Don't allow unsupported combinations
    assert( !(selectionFlags & AnimationModuleSelectionModel::SelectionTypeAdd
              &&
              selectionFlags & AnimationModuleSelectionModel::SelectionTypeToggle) );

    bool hasChanged = false;
    if (selectionFlags & AnimationModuleSelectionModel::SelectionTypeClear) {
        hasChanged = true;
        _imp->selectedKeyframes.clear();
        _imp->selectedNodes.clear();
        _imp->selectedTableItems.clear();
    }

    for (AnimItemDimViewKeyFramesMap::const_iterator it = keys.begin(); it != keys.end(); ++it) {

        AnimItemDimViewKeyFramesMap::iterator exists = _imp->selectedKeyframes.find(it->first);
        if (exists == _imp->selectedKeyframes.end()) {
            _imp->selectedKeyframes[it->first] = it->second;
        } else {
            for (KeyFrameSet::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                KeyFrameSet::iterator timeExist = exists->second.find(*it2);
                if (timeExist == exists->second.end()) {
                    exists->second.erase(timeExist);
                    exists->second.insert(*it2);
                    hasChanged = true;
                } else if (selectionFlags & AnimationModuleSelectionModel::SelectionTypeToggle) {
                    exists->second.erase(timeExist);
                    hasChanged = true;
                }
            }

        }
    }

    for (std::vector<NodeAnimPtr >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        std::list<NodeAnimWPtr>::iterator found = isNodeSelected(*it);
        if ( found == _imp->selectedNodes.end() ) {
            _imp->selectedNodes.push_back(*it);
            hasChanged = true;
        } else if (selectionFlags & AnimationModuleSelectionModel::SelectionTypeToggle) {
            _imp->selectedNodes.erase(found);
            hasChanged = true;
        }
    }

    for (std::vector<TableItemAnimPtr >::const_iterator it = selectedTableItems.begin(); it != selectedTableItems.end(); ++it) {
        std::list<TableItemAnimWPtr>::iterator found = isTableItemSelected(*it);
        if ( found == _imp->selectedTableItems.end() ) {
            _imp->selectedTableItems.push_back(*it);
            hasChanged = true;
        } else if (selectionFlags & AnimationModuleSelectionModel::SelectionTypeToggle) {
            _imp->selectedTableItems.erase(found);
            hasChanged = true;
        }
    }

    if (hasChanged) {
        Q_EMIT keyframeSelectionChangedFromModel(selectionFlags & AnimationModuleSelectionModel::SelectionTypeRecurse);
    }
} // makeSelection

bool
AnimationModuleSelectionModel::isEmpty() const
{
    return _imp->selectedKeyframes.empty() && _imp->selectedNodes.empty();
}

void
AnimationModuleSelectionModel::getCurrentSelection(AnimItemDimViewKeyFramesMap* keys, std::vector<NodeAnimPtr >* nodes, std::vector<TableItemAnimPtr>* tableItems) const
{
    *keys =  _imp->selectedKeyframes;
    for (std::list<NodeAnimWPtr >::const_iterator it = _imp->selectedNodes.begin(); it != _imp->selectedNodes.end(); ++it) {
        NodeAnimPtr n = it->lock();
        if (n) {
            nodes->push_back(n);
        }
    }
    for (std::list<TableItemAnimWPtr >::const_iterator it = _imp->selectedTableItems.begin(); it != _imp->selectedTableItems.end(); ++it) {
        TableItemAnimPtr n = it->lock();
        if (n) {
            tableItems->push_back(n);
        }
    }
}


bool
AnimationModuleSelectionModel::hasSingleKeyFrameTimeSelected(double* time) const
{
    bool timeSet = false;
    AnimItemBasePtr itemSet;


    if ( _imp->selectedKeyframes.empty() ) {
        return false;
    }

    for (AnimItemDimViewKeyFramesMap::iterator it = _imp->selectedKeyframes.begin(); it != _imp->selectedKeyframes.end(); ++it) {
        if (itemSet) {
            itemSet = it->first.item;
        } else {
            if (it->first.item != itemSet) {
                return false;
            }
        }
        for (KeyFrameSet::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            if (!timeSet) {
                *time = it2->getTime();
                timeSet = true;
            } else {
                if (it2->getTime() != *time) {
                    return false;
                }
            }
        }

    }

    return true;
}

int
AnimationModuleSelectionModel::getSelectedKeyframesCount() const
{
    return (int)_imp->selectedKeyframes.size();
}

bool
AnimationModuleSelectionModel::isKeyframeSelected(const AnimItemBasePtr &anim, DimSpec dimension ,ViewSetSpec view, const KeyFrame &keyframe) const
{
    AnimItemDimViewID key(anim, view, dimension);
    AnimItemDimViewKeyFramesMap::const_iterator found = _imp->selectedKeyframes.find(key);
    if (found == _imp->selectedKeyframes.end()) {
        return false;
    }
    KeyFrameSet::const_iterator foundKey = found->second.find(keyframe);
    if (foundKey == found->second.end()) {
        return false;
    }
    return true;
}

std::list<TableItemAnimWPtr>::iterator
AnimationModuleSelectionModel::isTableItemSelected(const TableItemAnimPtr& node)
{
    for (std::list<TableItemAnimWPtr>::iterator it = _imp->selectedTableItems.begin(); it != _imp->selectedTableItems.end(); ++it) {
        TableItemAnimPtr n = it->lock();
        if ( n && (n == node) ) {
            return it;
        }
    }

    return _imp->selectedTableItems.end();
}

std::list<NodeAnimWPtr>::iterator
AnimationModuleSelectionModel::isNodeSelected(const NodeAnimPtr& node)
{
    for (std::list<NodeAnimWPtr>::iterator it = _imp->selectedNodes.begin(); it != _imp->selectedNodes.end(); ++it) {
        NodeAnimPtr n = it->lock();
        if ( n && (n == node) ) {
            return it;
        }
    }

    return _imp->selectedNodes.end();
}

bool
AnimationModuleSelectionModel::isNodeSelected(const NodeAnimPtr& node) const
{
    for (std::list<NodeAnimWPtr>::const_iterator it = _imp->selectedNodes.begin(); it != _imp->selectedNodes.end(); ++it) {
        NodeAnimPtr n = it->lock();
        if ( n && (n == node) ) {
            return true;
        }
    }

    return false;
}


void
AnimationModuleSelectionModel::removeAnyReferenceFromSelection(const NodeAnimPtr &removed)
{
    // Remove selected item for given node
    for (std::list<NodeAnimWPtr>::iterator it = _imp->selectedNodes.begin(); it != _imp->selectedNodes.end(); ++it) {
        if (it->lock() == removed) {
            _imp->selectedNodes.erase(it);
            break;
        }
    }
    for (std::list<TableItemAnimWPtr>::const_iterator it = _imp->selectedTableItems.begin(); it != _imp->selectedTableItems.end(); ++it) {
        TableItemAnimPtr item = it->lock();
        if (!item) {
            continue;
        }
        if (item->getNode() == removed) {
            it = _imp->selectedTableItems.erase(it);
        }
    }
    for (AnimItemDimViewKeyFramesMap::iterator it = _imp->selectedKeyframes.begin(); it != _imp->selectedKeyframes.end(); ++it) {

        AnimItemBasePtr keyHolder = it->first.item;
        assert(keyHolder);


        bool found = false;
        // If it is a keyframe of a table item animation
        TableItemAnimPtr isTableItemAnim = toTableItemAnim(keyHolder);
        if (isTableItemAnim && isTableItemAnim->getNode() == removed) {
            found = true;
            it = _imp->selectedKeyframes.erase(it);
        }

        if (!found) {
            // If it is a keyframe of a knob
            KnobAnimPtr isKnob = toKnobAnim(keyHolder);
            if (isKnob) {
                KnobsHolderAnimBasePtr holder = isKnob->getHolder();
                TableItemAnimPtr holderIsTableItem = toTableItemAnim(holder);
                if (holderIsTableItem && holderIsTableItem->getNode() == removed) {
                    it = _imp->selectedKeyframes.erase(it);
                    found = true;
                }
                if (!found) {
                    NodeAnimPtr holderIsNode = toNodeAnim(holder);
                    if (holderIsNode && holderIsNode == removed) {
                        it = _imp->selectedKeyframes.erase(it);
                        found = true;
                    }
                }
            }
        }

    }
}

void
AnimationModuleSelectionModel::removeAnyReferenceFromSelection(const TableItemAnimPtr& removed)
{
    for (std::list<TableItemAnimWPtr>::const_iterator it = _imp->selectedTableItems.begin(); it != _imp->selectedTableItems.end(); ++it) {
        TableItemAnimPtr item = it->lock();
        if (!item) {
            continue;
        }
        if (item == removed) {
            it = _imp->selectedTableItems.erase(it);
        }
    }
    for (AnimItemDimViewKeyFramesMap::iterator it = _imp->selectedKeyframes.begin(); it != _imp->selectedKeyframes.end(); ++it) {

        AnimItemBasePtr keyHolder = it->first.item;
        assert(keyHolder);


        bool found = false;
        // If it is a keyframe of a table item animation
        TableItemAnimPtr isTableItemAnim = toTableItemAnim(keyHolder);
        if (isTableItemAnim && isTableItemAnim == removed) {
            found = true;
            it = _imp->selectedKeyframes.erase(it);
        }

        if (!found) {
            // If it is a keyframe of a knob
            KnobAnimPtr isKnob = toKnobAnim(keyHolder);
            if (isKnob) {
                KnobsHolderAnimBasePtr holder = isKnob->getHolder();
                TableItemAnimPtr holderIsTableItem = toTableItemAnim(holder);
                if (holderIsTableItem && holderIsTableItem == removed) {
                    it = _imp->selectedKeyframes.erase(it);
                    found = true;
                }
            }
        }
        
    }
}


NATRON_NAMESPACE_EXIT;
