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

#include "Gui/AnimationModuleBase.h"
#include "Gui/KnobAnim.h"
#include "Gui/NodeAnim.h"
#include "Gui/TableItemAnim.h"

NATRON_NAMESPACE_ENTER;

class AnimationModuleSelectionModelPrivate
{
public:
    AnimationModuleSelectionModelPrivate(const AnimationModuleSelectionProviderPtr& model)
    : model(model)
    , selectedKeyframes()
    , selectedTableItems()
    , selectedNodes()
    {
    }

    std::list<NodeAnimPtr>::iterator isNodeSelected(const NodeAnimPtr& node);

    std::list<TableItemAnimPtr>::iterator isTableItemSelected(const TableItemAnimPtr& node);


    /* attributes */
    AnimationModuleSelectionProviderWPtr model;
    AnimItemDimViewKeyFramesMap selectedKeyframes;
    std::list<TableItemAnimPtr> selectedTableItems;
    std::list<NodeAnimPtr> selectedNodes;
};

AnimationModuleSelectionModel::AnimationModuleSelectionModel(const AnimationModuleSelectionProviderPtr& model)
: _imp(new AnimationModuleSelectionModelPrivate(model))
{
}

AnimationModuleSelectionModel::~AnimationModuleSelectionModel()
{
}

AnimationModuleSelectionProviderPtr
AnimationModuleSelectionModel::getModel() const
{
    return _imp->model.lock();
}

void
AnimationModuleSelectionModel::addAnimatedItemKeyframes(const AnimItemBasePtr& item,
                                                        DimSpec dim,
                                                        ViewSetSpec viewSpec,
                                                        AnimItemDimViewKeyFramesMap* result)
{
    std::list<ViewIdx> views = item->getViewsList();
    int nDims = item->getNDimensions();
    if (viewSpec.isAll()) {
        for (std::list<ViewIdx>::const_iterator it3 = views.begin(); it3 != views.end(); ++it3) {

            if (dim.isAll()) {
                for (int i = 0; i < nDims; ++i) {
                    AnimItemDimViewIndexID key(item, *it3, DimIdx(i));
                    KeyFrameWithStringSet &keysForItem = (*result)[key];
                    item->getKeyframes(DimIdx(i), *it3, AnimItemBase::eGetKeyframesTypeMerged, &keysForItem);
                }
            } else {
                AnimItemDimViewIndexID key(item, *it3, DimIdx(dim));
                KeyFrameWithStringSet &keysForItem = (*result)[key];
                item->getKeyframes(DimIdx(dim), *it3, AnimItemBase::eGetKeyframesTypeMerged, &keysForItem);
            }
        }

    } else {
        if (dim.isAll()) {
            for (int i = 0; i < nDims; ++i) {
                AnimItemDimViewIndexID key(item, ViewIdx(viewSpec), DimIdx(i));
                KeyFrameWithStringSet &keysForItem = (*result)[key];
                item->getKeyframes(DimIdx(i), ViewIdx(viewSpec), AnimItemBase::eGetKeyframesTypeMerged, &keysForItem);
            }
        } else {
            AnimItemDimViewIndexID key(item, ViewIdx(viewSpec), DimIdx(dim));
            KeyFrameWithStringSet &keysForItem = (*result)[key];
            item->getKeyframes(DimIdx(dim), ViewIdx(viewSpec), AnimItemBase::eGetKeyframesTypeMerged,&keysForItem);
        }
    }

}

void
AnimationModuleSelectionModel::addAnimatedItemsWithoutKeyframes(const AnimItemBasePtr& item,
                                                        DimSpec dim,
                                                        ViewSetSpec viewSpec,
                                                        AnimItemDimViewKeyFramesMap* result)
{
    std::list<ViewIdx> views = item->getViewsList();
    int nDims = item->getNDimensions();
    if (viewSpec.isAll()) {
        for (std::list<ViewIdx>::const_iterator it3 = views.begin(); it3 != views.end(); ++it3) {

            if (dim.isAll()) {
                for (int i = 0; i < nDims; ++i) {
                    AnimItemDimViewIndexID key(item, *it3, DimIdx(i));
                    KeyFrameWithStringSet &keysForItem = (*result)[key];
                    (void)keysForItem;
                }
            } else {
                AnimItemDimViewIndexID key(item, *it3, DimIdx(dim));
                KeyFrameWithStringSet &keysForItem = (*result)[key];
                (void)keysForItem;
            }
        }

    } else {
        if (dim.isAll()) {
            for (int i = 0; i < nDims; ++i) {
                AnimItemDimViewIndexID key(item, ViewIdx(viewSpec), DimIdx(i));
                KeyFrameWithStringSet &keysForItem = (*result)[key];
                (void)keysForItem;
            }
        } else {
            AnimItemDimViewIndexID key(item, ViewIdx(viewSpec), DimIdx(dim));
            KeyFrameWithStringSet &keysForItem = (*result)[key];
            (void)keysForItem;
        }
    }
    
}


void
AnimationModuleSelectionModel::addTableItemKeyframes(const TableItemAnimPtr& item,
                                                     bool recurse,
                                                     DimSpec dim,
                                                     ViewSetSpec viewSpec,
                                                     std::vector<TableItemAnimPtr> *selectedTableItems,
                                                     AnimItemDimViewKeyFramesMap* result)
{
    if (item->isRangeDrawingEnabled()) {
        selectedTableItems->push_back(item);
    }

    addAnimatedItemKeyframes(item, dim, viewSpec, result);

    const std::vector<KnobAnimPtr>& knobs = item->getKnobs();
    for (std::vector<KnobAnimPtr>::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
        addAnimatedItemKeyframes(*it2, DimSpec::all(), ViewSetSpec::all(), result);
    }

    if (recurse) {
        std::vector<TableItemAnimPtr> children = item->getChildren();
        for (std::size_t i = 0; i < children.size(); ++i) {
            addTableItemKeyframes(children[i], true, dim, viewSpec, selectedTableItems, result);
        }
    }
}

void
AnimationModuleSelectionModel::getAllKeyFrames(AnimItemDimViewKeyFramesMap* selectedKeyframes, std::vector<NodeAnimPtr >* selectedNodes, std::vector<TableItemAnimPtr>* selectedTableItems) const
{

    AnimationModuleSelectionProviderPtr model = getModel();

    std::vector<KnobAnimPtr> topLevelKnobs;
    model->getTopLevelKnobs(&topLevelKnobs);

    std::vector<NodeAnimPtr > topLevelNodes;
    model->getTopLevelNodes(&topLevelNodes);

    std::vector<TableItemAnimPtr> topLevelTableItems;
    model->getTopLevelTableItems(&topLevelTableItems);



    for (std::vector<NodeAnimPtr>::const_iterator it = topLevelNodes.begin(); it != topLevelNodes.end(); ++it) {

        const std::vector<KnobAnimPtr>& knobs = (*it)->getKnobs();

        for (std::vector<KnobAnimPtr>::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
            addAnimatedItemKeyframes(*it2, DimSpec::all(), ViewSetSpec::all(), selectedKeyframes);
        }

        const std::vector<TableItemAnimPtr>& topLevelTableItems = (*it)->getTopLevelItems();
        for (std::vector<TableItemAnimPtr>::const_iterator it2 = topLevelTableItems.begin(); it2 != topLevelTableItems.end(); ++it2) {
            // We select keyframes for the root item, this will also select children
            addTableItemKeyframes(*it2, true, DimSpec::all(), ViewSetSpec::all(), selectedTableItems, selectedKeyframes);
        }
        if ((*it)->isRangeDrawingEnabled()) {
            selectedNodes->push_back(*it);
        }


    }

    for (std::vector<KnobAnimPtr>::const_iterator it2 = topLevelKnobs.begin(); it2 != topLevelKnobs.end(); ++it2) {
        addAnimatedItemKeyframes(*it2, DimSpec::all(), ViewSetSpec::all(), selectedKeyframes);
    }

    for (std::vector<TableItemAnimPtr>::const_iterator it2 = topLevelTableItems.begin(); it2 != topLevelTableItems.end(); ++it2) {
        addTableItemKeyframes(*it2, true, DimSpec::all(), ViewSetSpec::all(), selectedTableItems, selectedKeyframes);
    }

}

void
AnimationModuleSelectionModel::selectAll()
{



    AnimItemDimViewKeyFramesMap selectedKeyframes;
    std::vector<NodeAnimPtr > selectedNodes;
    std::vector<TableItemAnimPtr> selectedTableItems;
    getAllKeyFrames(&selectedKeyframes, &selectedNodes, &selectedTableItems);

    makeSelection( selectedKeyframes, selectedTableItems, selectedNodes, (AnimationModuleSelectionModel::SelectionTypeAdd |
                                                               AnimationModuleSelectionModel::SelectionTypeClear |
                                                               AnimationModuleSelectionModel::SelectionTypeRecurse) );
}


void
AnimationModuleSelectionModel::selectItems(const QList<QTreeWidgetItem *> &items)
{
    AnimItemDimViewKeyFramesMap keys;
    std::vector<NodeAnimPtr > nodes;
    std::vector<TableItemAnimPtr> tableItems;

    Q_FOREACH (QTreeWidgetItem * item, items) {
        AnimatedItemTypeEnum foundType;
        KnobAnimPtr isKnob;
        TableItemAnimPtr isTableItem;
        NodeAnimPtr isNodeItem;
        ViewSetSpec view;
        DimSpec dim;
        bool found = getModel()->findItem(item, &foundType, &isKnob, &isTableItem, &isNodeItem, &view, &dim);
        if (!found) {
            continue;
        }

        if (isKnob) {

            AnimationModuleSelectionModel::addAnimatedItemsWithoutKeyframes(isKnob, dim, view, &keys);
            // Commented-out: this would select all keyframes for the curve
            // AnimationModuleSelectionModel::addAnimatedItemKeyframes(isKnob, dim, view, &keys);
        } else if (isNodeItem) {
            nodes.push_back(isNodeItem);
        } else if (isTableItem) {
            tableItems.push_back(isTableItem);
            AnimationModuleSelectionModel::addAnimatedItemsWithoutKeyframes(isTableItem, dim, view, &keys);
            //AnimationModuleSelectionModel::addTableItemKeyframes(isTableItem, false, dim, view, &tableItems, &keys);
        }
    }

    AnimationModuleSelectionModel::SelectionTypeFlags sFlags = AnimationModuleSelectionModel::SelectionTypeAdd
    | AnimationModuleSelectionModel::SelectionTypeClear | AnimationModuleSelectionModel::SelectionTypeRecurse;

    makeSelection(keys, tableItems, nodes, sFlags);
}

void
AnimationModuleSelectionModel::selectKeyframes(const AnimItemBasePtr& item, DimSpec dimension, ViewSetSpec view)
{
    AnimItemDimViewKeyFramesMap keys;
    item->getKeyframes(dimension, view, &keys);
    makeSelection(keys, std::vector<TableItemAnimPtr>(), std::vector<NodeAnimPtr >(), AnimationModuleSelectionModel::SelectionTypeClear);
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
AnimationModuleSelectionModel::clearKeyframesFromSelection()
{
    const std::list<TableItemAnimPtr>& tableItems = getCurrentTableItemsSelection();
    const std::list<NodeAnimPtr>& nodes = getCurrentNodesSelection();
    const AnimItemDimViewKeyFramesMap& selectedKeys = getCurrentKeyFramesSelection();

    AnimItemDimViewKeyFramesMap newKeys;
    std::vector<TableItemAnimPtr> newTableItems;
    std::vector<NodeAnimPtr> newNodes;
    newNodes.insert(newNodes.end(), nodes.begin(), nodes.end());
    newTableItems.insert(newTableItems.end(), tableItems.begin(), tableItems.end());
    for (AnimItemDimViewKeyFramesMap::const_iterator it = selectedKeys.begin(); it!=selectedKeys.end(); ++it) {
        newKeys.insert(std::make_pair(it->first, KeyFrameWithStringSet()));
    }
    makeSelection(newKeys, newTableItems, newNodes, AnimationModuleSelectionModel::SelectionTypeAdd | AnimationModuleSelectionModel::SelectionTypeClear);
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
            for (KeyFrameWithStringSet::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                KeyFrameWithStringSet::iterator timeExist = exists->second.find(*it2);
                if (timeExist != exists->second.end()) {
                    exists->second.erase(timeExist);
                    if (!selectionFlags.testFlag(AnimationModuleSelectionModel::SelectionTypeToggle)) {
                        exists->second.insert(*it2);
                    }
                    hasChanged = true;
                } else {
                    exists->second.insert(*it2);
                    hasChanged = true;
                }
            }

        }
    }

    for (std::vector<NodeAnimPtr >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        std::list<NodeAnimPtr>::iterator found = _imp->isNodeSelected(*it);
        if ( found == _imp->selectedNodes.end() ) {
            _imp->selectedNodes.push_back(*it);
            hasChanged = true;
        } else if (selectionFlags & AnimationModuleSelectionModel::SelectionTypeToggle) {
            _imp->selectedNodes.erase(found);
            hasChanged = true;
        }
    }

    for (std::vector<TableItemAnimPtr >::const_iterator it = selectedTableItems.begin(); it != selectedTableItems.end(); ++it) {
        std::list<TableItemAnimPtr>::iterator found = _imp->isTableItemSelected(*it);
        if ( found == _imp->selectedTableItems.end() ) {
            _imp->selectedTableItems.push_back(*it);
            hasChanged = true;
        } else if (selectionFlags & AnimationModuleSelectionModel::SelectionTypeToggle) {
            _imp->selectedTableItems.erase(found);
            hasChanged = true;
        }
    }

    if (hasChanged) {
        Q_EMIT selectionChanged(selectionFlags & AnimationModuleSelectionModel::SelectionTypeRecurse);
    }
} // makeSelection

bool
AnimationModuleSelectionModel::isEmpty() const
{
    return _imp->selectedKeyframes.empty() && _imp->selectedNodes.empty();
}

const std::list<NodeAnimPtr >&
AnimationModuleSelectionModel::getCurrentNodesSelection() const
{
    return _imp->selectedNodes;
}

const std::list<TableItemAnimPtr>&
AnimationModuleSelectionModel::getCurrentTableItemsSelection() const
{
    return _imp->selectedTableItems;
}


const AnimItemDimViewKeyFramesMap&
AnimationModuleSelectionModel::getCurrentKeyFramesSelection() const
{
    return _imp->selectedKeyframes;
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
        for (KeyFrameWithStringSet::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            if (!timeSet) {
                *time = it2->key.getTime();
                timeSet = true;
            } else {
                if (it2->key.getTime() != *time) {
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
    int nKeys = 0;
    for (AnimItemDimViewKeyFramesMap::const_iterator it = _imp->selectedKeyframes.begin(); it != _imp->selectedKeyframes.end(); ++it) {
        nKeys += it->second.size();
    }
    return nKeys;
}

bool
AnimationModuleSelectionModel::isKeyframeSelected(const AnimItemBasePtr &item, DimSpec dimension ,ViewSetSpec view, double time) const
{
    std::list<DimensionViewPair> curvesToProcess;
    if (dimension.isAll()) {
        int nDims = item->getNDimensions();
        for (int i = 0; i < nDims; ++i) {
            if (view.isAll()) {
                std::list<ViewIdx> views = item->getViewsList();
                for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
                    DimensionViewPair p = {DimIdx(i), *it};
                    curvesToProcess.push_back(p);
                }
            } else {
                assert(view.isViewIdx());
                DimensionViewPair p = {DimIdx(i), ViewIdx(view)};
                curvesToProcess.push_back(p);
            }
        }
    } else {
        if (view.isAll()) {
            std::list<ViewIdx> views = item->getViewsList();
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
                DimensionViewPair p = {DimIdx(dimension), *it};
                curvesToProcess.push_back(p);
            }

        } else {
            assert(view.isViewIdx());
            DimensionViewPair p = {DimIdx(dimension), ViewIdx(view.value())};
            curvesToProcess.push_back(p);
        }
    }
    assert(!curvesToProcess.empty());
    if (curvesToProcess.empty()) {
        return false;
    }
    for (std::list<DimensionViewPair>::iterator it = curvesToProcess.begin(); it != curvesToProcess.end(); ++it) {
        if (!isKeyframeSelectedOnCurve(item, it->dimension, it->view, time)) {
            return false;
        }
    }
    return true;
} // isKeyframeSelected

bool
AnimationModuleSelectionModel::isKeyframeSelectedOnCurve(const AnimItemBasePtr &anim, DimIdx dimension ,ViewIdx view, double time) const
{
    AnimItemDimViewIndexID key(anim, view, dimension);
    AnimItemDimViewKeyFramesMap::const_iterator found = _imp->selectedKeyframes.find(key);
    if (found == _imp->selectedKeyframes.end()) {
        return false;
    }
    KeyFrameWithString k;
    k.key.setTime(time);
    KeyFrameWithStringSet::const_iterator foundKey = found->second.find(k);
    if (foundKey == found->second.end()) {
        return false;
    }
    return true;
}

std::list<TableItemAnimPtr>::iterator
AnimationModuleSelectionModelPrivate::isTableItemSelected(const TableItemAnimPtr& node)
{
    for (std::list<TableItemAnimPtr>::iterator it = selectedTableItems.begin(); it != selectedTableItems.end(); ++it) {
        if ( *it == node ) {
            return it;
        }
    }

    return selectedTableItems.end();
}

std::list<NodeAnimPtr>::iterator
AnimationModuleSelectionModelPrivate::isNodeSelected(const NodeAnimPtr& node)
{
    for (std::list<NodeAnimPtr>::iterator it = selectedNodes.begin(); it != selectedNodes.end(); ++it) {
        if ( *it == node ) {
            return it;
        }
    }

    return selectedNodes.end();
}

bool
AnimationModuleSelectionModel::isNodeSelected(const NodeAnimPtr& node) const
{
    for (std::list<NodeAnimPtr>::const_iterator it = _imp->selectedNodes.begin(); it != _imp->selectedNodes.end(); ++it) {
        if ( *it == node) {
            return true;
        }
    }

    return false;
}


void
AnimationModuleSelectionModel::removeAnyReferenceFromSelection(const NodeAnimPtr &removed)
{
    // Remove selected item for given node
    for (std::list<NodeAnimPtr>::iterator it = _imp->selectedNodes.begin(); it != _imp->selectedNodes.end(); ++it) {
        if (*it == removed) {
            _imp->selectedNodes.erase(it);
            break;
        }
    }

    {
        std::list<TableItemAnimPtr> newTableItems;
        for (std::list<TableItemAnimPtr>::const_iterator it = _imp->selectedTableItems.begin(); it != _imp->selectedTableItems.end(); ++it) {
            if ((*it)->getNode() != removed) {
                newTableItems.push_back(*it);
            }
        }
        _imp->selectedTableItems = newTableItems;
    }

    {
        AnimItemDimViewKeyFramesMap newKeys;
        for (AnimItemDimViewKeyFramesMap::iterator it = _imp->selectedKeyframes.begin(); it != _imp->selectedKeyframes.end(); ++it) {

            AnimItemBasePtr keyHolder = it->first.item;
            assert(keyHolder);


            bool found = false;
            // If it is a keyframe of a table item animation
            TableItemAnimPtr isTableItemAnim = toTableItemAnim(keyHolder);
            if (isTableItemAnim && isTableItemAnim->getNode() == removed) {
                found = true;
            }

            if (!found) {
                // If it is a keyframe of a knob
                KnobAnimPtr isKnob = toKnobAnim(keyHolder);
                if (isKnob) {
                    KnobsHolderAnimBasePtr holder = isKnob->getHolder();
                    TableItemAnimPtr holderIsTableItem = toTableItemAnim(holder);
                    if (holderIsTableItem && holderIsTableItem->getNode() == removed) {
                        found = true;
                    }
                    if (!found) {
                        NodeAnimPtr holderIsNode = toNodeAnim(holder);
                        if (holderIsNode && holderIsNode == removed) {
                            found = true;
                        }
                    }
                }
            }
            if (!found) {
                newKeys.insert(*it);
            }
        }
        _imp->selectedKeyframes = newKeys;
    }
}

void
AnimationModuleSelectionModel::removeAnyReferenceFromSelection(const TableItemAnimPtr& removed)
{
    for (std::list<TableItemAnimPtr>::const_iterator it = _imp->selectedTableItems.begin(); it != _imp->selectedTableItems.end(); ++it) {
 
        if (*it == removed) {
            _imp->selectedTableItems.erase(it);
            break;
        }
    }

    {
        AnimItemDimViewKeyFramesMap newKeys;
        for (AnimItemDimViewKeyFramesMap::iterator it = _imp->selectedKeyframes.begin(); it != _imp->selectedKeyframes.end(); ++it) {

            AnimItemBasePtr keyHolder = it->first.item;
            assert(keyHolder);


            bool found = false;
            // If it is a keyframe of a table item animation
            TableItemAnimPtr isTableItemAnim = toTableItemAnim(keyHolder);
            if (isTableItemAnim && isTableItemAnim == removed) {
                found = true;
            }

            if (!found) {
                // If it is a keyframe of a knob
                KnobAnimPtr isKnob = toKnobAnim(keyHolder);
                if (isKnob) {
                    KnobsHolderAnimBasePtr holder = isKnob->getHolder();
                    TableItemAnimPtr holderIsTableItem = toTableItemAnim(holder);
                    if (holderIsTableItem && holderIsTableItem == removed) {
                        found = true;
                    }
                }
            }
            if (!found) {
                newKeys.insert(*it);
            }
        }
        _imp->selectedKeyframes = newKeys;
    }
}


NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING;
#include "moc_AnimationModuleSelectionModel.cpp"
