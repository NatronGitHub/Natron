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

#include "KnobAnim.h"

#include <map>

#include <QTreeWidgetItem>

#include "Engine/AppInstance.h"
#include "Engine/AnimatingObjectI.h"
#include "Engine/KnobTypes.h"
#include "Engine/Project.h"

#include "Gui/AnimationModule.h"
#include "Gui/KnobGui.h"


NATRON_NAMESPACE_ENTER;

typedef std::map<DimensionViewPair, QTreeWidgetItem* ,DimensionViewPairCompare> PerDimViewItemMap;
typedef std::map<ViewIdx, QTreeWidgetItem*> PerViewItemMap;

class KnobAnimPrivate
{
public:
    KnobAnimPrivate(KnobAnim* publicInterface, const KnobsHolderAnimBasePtr& holder)
    : publicInterface(publicInterface)
    , holder(holder)
    , rootItem(0)
    , knobGui()
    , knob()
    {

    }


    KnobAnim* publicInterface;

    KnobsHolderAnimBaseWPtr holder;

    // For each dim/view
    PerDimViewItemMap dimViewItems;

    // The views items
    PerViewItemMap viewItems;

    QTreeWidgetItem *rootItem;

    KnobGuiWPtr knobGui;
    KnobIWPtr knob;
};

NATRON_NAMESPACE_ANONYMOUS_ENTER

static QTreeWidgetItem *
createKnobNameItem(const QString &text,
                   AnimatedItemTypeEnum itemType,
                   DimSpec dimension,
                   ViewSetSpec view,
                   QTreeWidgetItem *parent)
{
    QTreeWidgetItem *ret = new QTreeWidgetItem;

    if (parent) {
        parent->addChild(ret);
    }

    ret->setData(0, QT_ROLE_CONTEXT_TYPE, itemType);

    ret->setData(0, QT_ROLE_CONTEXT_DIM, (int)dimension);

    ret->setData(0, QT_ROLE_CONTEXT_VIEW, (int)view);

    ret->setText(0, text);

    if ( (itemType == eAnimatedItemTypeKnobRoot) || (itemType == eAnimatedItemTypeKnobDim) || (itemType == eAnimatedItemTypeKnobView) ) {
        ret->setFlags(ret->flags() & ~Qt::ItemIsDragEnabled & ~Qt::ItemIsDropEnabled);
    }

    return ret;
}

NATRON_NAMESPACE_ANONYMOUS_EXIT


KnobAnim::KnobAnim(const AnimationModulePtr& model,
                   const KnobsHolderAnimBasePtr& holder,
                   QTreeWidgetItem *parentItem,
                   const KnobGuiPtr& knobGui)
: AnimItemBase(model)
, _imp(new KnobAnimPrivate(this, holder))
{
    assert(knobGui);
    assert(parentItem);

    _imp->knobGui = knobGui;
    KnobIPtr knob = knobGui->getKnob();
    _imp->knob = knob;

    const std::vector<std::string>& projectViews = knob->getHolder()->getApp()->getProject()->getProjectViewNames();

    QObject::connect( knobGui.get(), SIGNAL(mustRefreshAnimVisibility()), this, SLOT(refreshKnobVisibility()) );

    // Create the root item for the knob
    QString knobLabel = QString::fromUtf8( knob->getLabel().c_str() );
    _imp->rootItem = createKnobNameItem(knobLabel,
                                        eAnimatedItemTypeKnobRoot,
                                        DimSpec::all(),
                                        ViewSetSpec::all(),
                                        parentItem);


    std::list<ViewIdx> views = knob->getViewsList();
    assert(!views.empty());

    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {

        // Create a view item if the knob has views splits
        QTreeWidgetItem * viewItem = 0;
        if (views.size() > 1) {
            QString viewName;
            if (*it >= 0 && *it < (int)projectViews.size()) {
                viewName = QString::fromUtf8(projectViews[*it].c_str());
            }
            viewItem = createKnobNameItem(viewName,
                                          eAnimatedItemTypeKnobDim,
                                          DimSpec::all(),
                                          *it,
                                          _imp->rootItem);
            _imp->viewItems[*it] = viewItem;
        }

        // Now create an item per dimension if the knob is multi-dimensional
        int nDims = knob->getNDimensions();
        if (nDims > 1) {
            for (int i = 0; i < nDims; ++i) {
                QString dimName = QString::fromUtf8( knob->getDimensionName(DimIdx(i)).c_str() );
                QTreeWidgetItem *dimItem = createKnobNameItem(dimName,
                                                              eAnimatedItemTypeKnobDim,
                                                              DimSpec(i),
                                                              *it,
                                                              viewItem ? viewItem : _imp->rootItem);
                DimensionViewPair p;
                p.view = *it;
                p.dimension = DimIdx(i);
                _imp->dimViewItems[p] = dimItem;

            }
        } else {
            DimensionViewPair p;
            p.view = *it;
            p.dimension = DimIdx(0);
            _imp->dimViewItems[p] = viewItem ? viewItem : _imp->rootItem;
        }
    } // for all views
}

KnobAnim::~KnobAnim()
{
    delete _imp->rootItem;
    _imp->rootItem = 0;
}

KnobsHolderAnimBasePtr
KnobAnim::getHolder() const
{
    return _imp->holder.lock();
}

int
KnobAnim::getNDimensions() const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        return 0;
    }
    return knob->getNDimensions();
}

std::list<ViewIdx>
KnobAnim::getViewsList() const
{
    std::list<ViewIdx> ret;
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        return ret;
    }
    ret = knob->getViewsList();
    return ret;
}

static bool refreshDimViewVisibility(DimIdx dim, ViewIdx view, const KnobIPtr& knob, KnobAnim* self)
{
    CurvePtr curve = knob->getCurve(view, dim);
    if (!curve) {
        return false;
    }
    bool curveIsAnimated = curve->isAnimated();
    QTreeWidgetItem *dimItem = self->getTreeItem(dim, view);
    dimItem->setHidden(!curveIsAnimated);
    dimItem->setData(0, QT_ROLE_CONTEXT_IS_ANIMATED, curveIsAnimated);
    return curveIsAnimated;
}

void
KnobAnim::refreshVisibilityConditional(bool refreshHolder)
{

    if (refreshHolder) {
        _imp->holder.lock()->refreshVisibility();
    } else {
        KnobGuiPtr knobGui = _imp->knobGui.lock();
        if (!knobGui) {
            return;
        }
        KnobIPtr knob = knobGui->getKnob();
        if (!knob) {
            return;
        }
        bool showItem = false;
        std::list<ViewIdx> views = knob->getViewsList();
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            bool hasDimVisible = false;
            for (int i = 0; i < knob->getNDimensions(); ++i) {
                hasDimVisible |= refreshDimViewVisibility(DimIdx(i), *it, knob, this);
            }
            // If there's a view item, refresh its visibility
            PerViewItemMap::const_iterator foundViewItem = _imp->viewItems.find(*it);
            if (foundViewItem != _imp->viewItems.end()) {
                foundViewItem->second->setHidden(!hasDimVisible);
            }

            showItem |= hasDimVisible;
        }

        _imp->rootItem->setHidden(!showItem);
        _imp->rootItem->setData(0, QT_ROLE_CONTEXT_IS_ANIMATED, showItem);
    }
}

void
KnobAnim::refreshKnobVisibility()
{



}


QTreeWidgetItem *
KnobAnim::getRootItem() const
{
    return _imp->rootItem;
}



QTreeWidgetItem *
KnobAnim::getTreeItem(DimIdx dimension, ViewIdx view) const
{
    DimensionViewPair p;
    p.dimension = dimension;
    p.view = view;
    PerDimViewItemMap::const_iterator foundItem = _imp->dimViewItems.find(p);
    if (foundItem == _imp->dimViewItems.end()) {
        return 0;
    }
    return foundItem->second;
}

KnobGuiPtr
KnobAnim::getKnobGui() const
{
    return _imp->knobGui.lock();
}

KnobIPtr
KnobAnim::getInternalKnob() const
{
    return _imp->knob.lock();
}

CurvePtr
KnobAnim::getCurve(DimIdx dimension, ViewIdx view) const
{
    KnobIPtr knob = _imp->knob.lock();
    if (!knob) {
        return CurvePtr();
    }
    return knob->getCurve(view, dimension);
}

bool
KnobAnim::getTreeItemViewDimension(QTreeWidgetItem* item, DimSpec* dimension, ViewSetSpec* view, AnimatedItemTypeEnum* type) const
{
    if (item == _imp->rootItem) {
        *dimension = DimSpec::all();
        *view = ViewSetSpec::all();
        *type = eAnimatedItemTypeKnobRoot;
        return true;
    }

    // Now look in per-view items
    for (PerViewItemMap::const_iterator it = _imp->viewItems.begin(); it != _imp->viewItems.end(); ++it) {
        if (it->second == item) {
            *dimension = DimSpec::all();
            *view = ViewSetSpec(it->first);
            *type = eAnimatedItemTypeKnobView;
            return true;
        }
    }

    // Now look in per-view/dim items
    for (PerDimViewItemMap::const_iterator it = _imp->dimViewItems.begin(); it!=_imp->dimViewItems.end(); ++it) {
        if (it->second == item) {
            *dimension = DimSpec(it->first.dimension);
            *view = ViewSetSpec(it->first.view);
            *type = eAnimatedItemTypeKnobDim;
            return true;
        }
    }
    return false;
}

NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING;
#include "moc_KnobAnim.cpp"
