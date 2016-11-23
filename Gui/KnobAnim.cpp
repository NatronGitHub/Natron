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
#include "Engine/KnobItemsTable.h"
#include "Engine/Project.h"

#include "Gui/AnimationModule.h"
#include "Gui/AnimationModuleTreeView.h"
#include "Gui/AnimationModuleView.h"
#include "Gui/CurveGui.h"
#include "Gui/KnobGui.h"


NATRON_NAMESPACE_ENTER;

struct DimViewItem
{
    CurveGuiPtr curve;
    QTreeWidgetItem* treeItem;
};
typedef std::map<DimensionViewPair, DimViewItem ,DimensionViewPairCompare> PerDimViewItemMap;
typedef std::map<ViewIdx, QTreeWidgetItem*> PerViewItemMap;

class KnobAnimPrivate
{
public:
    KnobAnimPrivate(KnobAnim* publicInterface, const KnobsHolderAnimBasePtr& holder)
    : publicInterface(publicInterface)
    , holder(holder)
    , rootItem(0)
    , parentItem(0)
    , knob()
    , nRefreshRequestsPending(0)
    , nDimVisibilityRequestsPending(0)
    {

    }

    void createItemForView(const std::vector<std::string>& projectViewNames, const std::list<ViewIdx>& itemViews, ViewIdx view);

    KnobAnim* publicInterface;

    KnobsHolderAnimBaseWPtr holder;

    // For each dim/view
    PerDimViewItemMap dimViewItems;

    // The views items
    PerViewItemMap viewItems;

    QTreeWidgetItem *rootItem;
    QTreeWidgetItem *parentItem;

    KnobIWPtr knob;

    // To avoid refreshing knob visibility too much from knob signals
    int nRefreshRequestsPending;
    int nDimVisibilityRequestsPending;
};

NATRON_NAMESPACE_ANONYMOUS_ENTER

static QTreeWidgetItem *
createKnobNameItem(const AnimItemBasePtr& anim,
                   const QString &text,
                   AnimatedItemTypeEnum itemType,
                   DimSpec dimension,
                   ViewSetSpec view,
                   QTreeWidgetItem *parent)
{
    QTreeWidgetItem *ret = new QTreeWidgetItem;

    if (parent) {
        parent->addChild(ret);
    }


    ret->setData(0, QT_ROLE_CONTEXT_ITEM_POINTER, qVariantFromValue((void*)anim.get()));

    ret->setData(0, QT_ROLE_CONTEXT_TYPE, itemType);

    ret->setData(0, QT_ROLE_CONTEXT_DIM, (int)dimension);

    ret->setData(0, QT_ROLE_CONTEXT_VIEW, (int)view);

    ret->setText(0, text);

    if (anim->getModel()->getTreeColumnsCount() > ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE) {
        ret->setData(ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE, QT_ROLE_CONTEXT_ITEM_VISIBLE, QVariant(true));

        ret->setIcon(ANIMATION_MODULE_TREE_VIEW_COL_VISIBLE, anim->getModel()->getItemVisibilityIcon(true));
    }
    
    ret->setExpanded(true);

    if ( (itemType == eAnimatedItemTypeKnobRoot) || (itemType == eAnimatedItemTypeKnobDim) || (itemType == eAnimatedItemTypeKnobView) ) {
        ret->setFlags(ret->flags() & ~Qt::ItemIsDragEnabled & ~Qt::ItemIsDropEnabled);
    }

    return ret;
}

NATRON_NAMESPACE_ANONYMOUS_EXIT


KnobAnim::KnobAnim(const AnimationModuleBasePtr& model,
                   const KnobsHolderAnimBasePtr& holder,
                   QTreeWidgetItem *parentItem,
                   const KnobIPtr& knob)
: AnimItemBase(model)
, _imp(new KnobAnimPrivate(this, holder))
{
    assert(knob);
    _imp->knob = knob;
    _imp->parentItem = parentItem;
    
    QObject::connect( knob->getSignalSlotHandler().get(), SIGNAL(secretChanged()), this, SLOT(emit_s_refreshKnobVisibilityLater()) );
    QObject::connect( knob->getSignalSlotHandler().get(), SIGNAL(enabledChanged()), this, SLOT(emit_s_refreshKnobVisibilityLater()) );
    QObject::connect( this, SIGNAL(s_refreshKnobVisibilityLater()), this, SLOT(onDoRefreshKnobVisibilityLaterTriggered()), Qt::QueuedConnection);
    QObject::connect( knob->getSignalSlotHandler().get(), SIGNAL(availableViewsChanged()), this, SLOT(onKnobAvailableViewsChanged()));
    QObject::connect( knob->getSignalSlotHandler().get(), SIGNAL(dimensionsVisibilityChanged(ViewSetSpec)), this, SLOT(onInternalKnobDimensionsVisibilityChanged(ViewSetSpec)));
    QObject::connect( this, SIGNAL(s_refreshDimensionsVisibilityLater()), this, SLOT(onDoRefreshDimensionsVisibilitylaterTriggered()), Qt::QueuedConnection);
}

KnobAnim::~KnobAnim()
{

    /*AnimationModuleBasePtr model = getModel();
    bool isTearingDown;
    if (model) {
        isTearingDown = model->isAboutToBeDestroyed();
    } else {
        isTearingDown = true;
    }

    if (!isTearingDown) {
        delete _imp->rootItem;
    }
    _imp->rootItem = 0;*/
}

void
KnobAnim::initialize()
{
    KnobIPtr knob = getInternalKnob();
    const std::vector<std::string>& projectViews = knob->getHolder()->getApp()->getProject()->getProjectViewNames();

    int nDims = knob->getNDimensions();
    std::list<ViewIdx> views = knob->getViewsList();
    assert(nDims >= 1 && !views.empty());



    // Create the root item for the knob
    QString knobLabel = QString::fromUtf8( knob->getLabel().c_str() );
    _imp->rootItem = createKnobNameItem(shared_from_this(),
                                        knobLabel,
                                        eAnimatedItemTypeKnobRoot,
                                        nDims > 0 ? DimSpec::all() : DimSpec(0),
                                        views.size() > 1 ? ViewSetSpec::all() : ViewIdx(0),
                                        _imp->parentItem);



    for (std::list<ViewIdx>::const_iterator it = views.begin(); it!=views.end(); ++it) {
        _imp->createItemForView(projectViews, views, *it);
    }

} // initialize

void
KnobAnimPrivate::createItemForView(const std::vector<std::string>& projectViewNames, const std::list<ViewIdx>& itemViews, ViewIdx view)
{
    AnimItemBasePtr thisShared = publicInterface->shared_from_this();

    KnobIPtr internalKnob = knob.lock();
    int nDims = internalKnob->getNDimensions();

    AnimationModuleBasePtr model = publicInterface->getModel();
    AnimationModuleView* curveWidget = model->getView();

    // Create a view item if the knob has views splits
    QTreeWidgetItem * viewItem = 0;
    if (itemViews.size() > 1) {
        QString viewName;
        if (view >= 0 && view < (int)projectViewNames.size()) {
            viewName = QString::fromUtf8(projectViewNames[view].c_str());
        }
        viewItem = createKnobNameItem(thisShared,
                                      viewName,
                                      eAnimatedItemTypeKnobView,
                                      nDims > 1 ? DimSpec::all() : DimSpec(0),
                                      view,
                                      rootItem);
        viewItems[view] = viewItem;
    }

    bool dimensionsExpanded = internalKnob->getAllDimensionsVisible(view);
    // Now create an item per dimension if the knob is multi-dimensional
    if (nDims > 1 && dimensionsExpanded) {
        for (int i = 0; i < nDims; ++i) {
            QString dimName = QString::fromUtf8( internalKnob->getDimensionName(DimIdx(i)).c_str() );
            QTreeWidgetItem *dimItem = createKnobNameItem(thisShared,
                                                          dimName,
                                                          eAnimatedItemTypeKnobDim,
                                                          DimSpec(i),
                                                          view,
                                                          viewItem ? viewItem : rootItem);
            DimensionViewPair p;
            p.view = view;
            p.dimension = DimIdx(i);
            DimViewItem& item = dimViewItems[p];
            item.treeItem = dimItem;
            if (curveWidget) {
                item.curve.reset(new CurveGui(curveWidget, thisShared, p.dimension, p.view));
            }

        }
    } else {
        // If single-dimensional, use the view item for the dimension item if multi-view
        // otherwise use the root item
        DimensionViewPair p;
        p.view = view;
        p.dimension = DimIdx(0);
        DimViewItem& item = dimViewItems[p];
        item.treeItem = viewItem ? viewItem : rootItem;
        if (curveWidget) {
            item.curve.reset(new CurveGui(curveWidget, thisShared, p.dimension, p.view));
        }
    }
} // createItemForView

void
KnobAnim::destroyAndRecreate()
{
    delete _imp->rootItem;
    _imp->dimViewItems.clear();
    _imp->viewItems.clear();

    initialize();

    refreshKnobVisibilityNow();

    getModel()->refreshSelectionBboxAndUpdateView();
}

void
KnobAnim::onKnobAvailableViewsChanged()
{
    destroyAndRecreate();
}

void
KnobAnim::onInternalKnobDimensionsVisibilityChanged(ViewSetSpec /*view*/)
{
    ++_imp->nDimVisibilityRequestsPending;
    Q_EMIT s_refreshDimensionsVisibilityLater();
}


void
KnobAnim::onDoRefreshDimensionsVisibilitylaterTriggered()
{
    if (_imp->nDimVisibilityRequestsPending == 0) {
        return;
    }
    _imp->nDimVisibilityRequestsPending = 0;
    destroyAndRecreate();
}

void
KnobAnim::refreshViewLabels(const std::vector<std::string>& projectViewNames)
{
    for (PerViewItemMap::const_iterator it = _imp->viewItems.begin(); it != _imp->viewItems.end(); ++it) {
        QString viewName;
        if (it->first >= 0 && it->first < (int)projectViewNames.size()) {
            viewName = QString::fromUtf8(projectViewNames[it->first].c_str());
        } else {
            viewName = it->second->text(0);
            QString notFoundSuffix = QString::fromUtf8("(*)");
            if (!viewName.endsWith(notFoundSuffix)) {
                viewName += QLatin1Char(' ');
                viewName += notFoundSuffix;
            }
        }
        it->second->setText(0, viewName);
    }
}

KnobsHolderAnimBasePtr
KnobAnim::getHolder() const
{
    return _imp->holder.lock();
}

AnimatingObjectIPtr
KnobAnim::getInternalAnimItem() const
{
    return _imp->knob.lock();
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

    CurvePtr curve = knob->getAnimationCurve(view, dim);
    bool curveIsAnimated = (curve && curve->isAnimated()) || !knob->getExpression(dim, view).empty();
    QTreeWidgetItem *dimItem = self->getTreeItem(dim, view);
    if (dimItem) {
        bool enabled = knob->isEnabled(dim, view);
        dimItem->setHidden(!curveIsAnimated || !enabled);
        dimItem->setData(0, QT_ROLE_CONTEXT_IS_ANIMATED, curveIsAnimated);
    }
    return curveIsAnimated;
}

void
KnobAnim::refreshVisibilityConditional(bool refreshHolder)
{
    KnobsHolderAnimBasePtr holder = getHolder();
    if (holder && refreshHolder) {
        holder->refreshVisibility();
        return;
    }



    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        return;
    }

    bool isTableItemMasterKnob = false;
    assert(knob->getHolder());
    EffectInstancePtr holderIsEffect = toEffectInstance(knob->getHolder());
    if (holderIsEffect) {
        KnobItemsTablePtr table = holderIsEffect->getItemsTable();
        if (table) {
            // Do not show anim for a master knob because all items in the table already have one.
            if (table->isPerItemKnobMaster(knob)) {
                isTableItemMasterKnob = true;
            }
        }
    }


    bool showItem = false;
    if (!isTableItemMasterKnob && !knob->getIsSecret()) {
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
    }
    _imp->rootItem->setData(0, QT_ROLE_CONTEXT_IS_ANIMATED, showItem);


    _imp->rootItem->setHidden(!showItem);

    AnimationModuleBasePtr model = getModel();
    AnimationModulePtr isAnimModule = toAnimationModule(model);

    // Post a redraw, this will concatenate
    model->getView()->redraw();


}

void
KnobAnim::refreshKnobVisibilityNow()
{
    refreshVisibilityConditional(true);
}


void
KnobAnim::emit_s_refreshKnobVisibilityLater()
{
    ++_imp->nRefreshRequestsPending;
    Q_EMIT s_refreshKnobVisibilityLater();

}

void
KnobAnim::onDoRefreshKnobVisibilityLaterTriggered()
{
    if (_imp->nRefreshRequestsPending == 0) {
        return;
    }
    _imp->nRefreshRequestsPending = 0;
    refreshKnobVisibilityNow();
}


QTreeWidgetItem *
KnobAnim::getRootItem() const
{
    return _imp->rootItem;
}



QTreeWidgetItem *
KnobAnim::getTreeItem(DimSpec dimension, ViewSetSpec view) const
{
    // The only item controlling all views is the root item
    if (view.isAll()) {
        return _imp->rootItem;
    }
    assert(view.isViewIdx());

    // Look in
    if (dimension.isAll()) {
        PerViewItemMap::const_iterator foundView = _imp->viewItems.find(ViewIdx(view));
        if (foundView == _imp->viewItems.end()) {
            return 0;
        }
        return foundView->second;
    }
    assert(!dimension.isAll());

    DimensionViewPair p;
    p.dimension = DimIdx(dimension);
    p.view = ViewIdx(view);
    PerDimViewItemMap::const_iterator foundItem = _imp->dimViewItems.find(p);
    if (foundItem == _imp->dimViewItems.end()) {
        return 0;
    }
    return foundItem->second.treeItem;
}

static void prependItemNameRecursive(QTreeWidgetItem* item, QString* str)
{
    if (!str->isEmpty()) {
        str->prepend(QLatin1Char('.'));
    }
    str->prepend(item->text(0));
    QTreeWidgetItem* parent = item->parent();
    if (!parent) {
        return;
    }
    AnimatedItemTypeEnum type = (AnimatedItemTypeEnum)parent->data(0, QT_ROLE_CONTEXT_TYPE).toInt();
    if (type == eAnimatedItemTypeKnobView ||
        type == eAnimatedItemTypeKnobRoot) {
        prependItemNameRecursive(parent, str);
    }
}

QString
KnobAnim::getViewDimensionLabel(DimIdx dimension, ViewIdx view) const
{
    QString ret;
    QTreeWidgetItem* item = getTreeItem(dimension, view);
    if (item) {
        prependItemNameRecursive(item, &ret);
    }
    return ret;
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

    // Using getAnimationCurve instead of getCurve will make this also work for parametric curves transparantly
    // without making any special case.
    return knob->getAnimationCurve(view, dimension);
}

CurveGuiPtr
KnobAnim::getCurveGui(DimIdx dimension, ViewIdx view) const
{
    DimensionViewPair p = {dimension, view};
    PerDimViewItemMap::const_iterator found = _imp->dimViewItems.find(p);
    if (found == _imp->dimViewItems.end()) {
        return CurveGuiPtr();
    }
    return found->second.curve;
}

bool
KnobAnim::hasExpression(DimIdx dimension, ViewIdx view) const
{
    KnobIPtr knob = getInternalKnob();
    if (!knob) {
        return false;
    }
    std::string expr = knob->getExpression(dimension, view);
    return !expr.empty();
}

double
KnobAnim::evaluateCurve(bool useExpressionIfAny, double x, DimIdx dimension, ViewIdx view)
{
    KnobIPtr knob = getInternalKnob();
    if (useExpressionIfAny && knob) {
        std::string expr = knob->getExpression(dimension, view);
        if (!expr.empty()) {
            return knob->getValueAtWithExpression(x, view, dimension);
        }
    }
    return AnimItemBase::evaluateCurve(false, x, dimension, view);
    
}

bool
KnobAnim::getAllDimensionsVisible(ViewIdx view) const
{
    return _imp->knob.lock()->getAllDimensionsVisible(view);
}

NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING;
#include "moc_KnobAnim.cpp"
