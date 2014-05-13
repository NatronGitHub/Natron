//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "RotoPanel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QPixmap>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QColor>
#include <QColorDialog>
#include <QMenu>
#include <QCursor>
#include <QMouseEvent>
#include <QApplication>

#include "Gui/Button.h"
#include "Gui/SpinBox.h"
#include "Gui/ClickableLabel.h"
#include "Gui/NodeGui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"

#include "Engine/RotoContext.h"
#include "Engine/TimeLine.h"
#include "Engine/Node.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"

using namespace Natron;


class TreeWidget: public QTreeWidget
{
    
public:
    
    TreeWidget(QWidget* parent): QTreeWidget(parent) {
    
    }
    
    virtual ~TreeWidget(){}
    
private:
    
    virtual void mousePressEvent(QMouseEvent* event) OVERRIDE FINAL
    {
        QModelIndex index = indexAt(event->pos());
        if (index.isValid() && index.column() != 0 && selectedItems().size() > 1) {
            QTreeWidgetItem* item = itemAt(event->pos());
            if (item) {
                emit itemClicked(item, index.column());
            }
            
        } else {
            QTreeWidget::mousePressEvent(event);
        }
    }
};


struct TreeItem
{
    QTreeWidgetItem* treeItem;
    boost::shared_ptr<RotoItem> rotoItem;
    
    TreeItem() : treeItem(0) , rotoItem() {}
    
    TreeItem(QTreeWidgetItem* t,const boost::shared_ptr<RotoItem>& r): treeItem(t) , rotoItem(r) {}
};

typedef std::list< TreeItem > TreeItems;


static QString interpName(int interp)
{
    switch (interp) {
        case 0:
            return "Smooth";
            break;
        case 1:
            return "Horizontal";
            break;
        case 2:
            return "Linear";
            break;
        case 3:
            return "Constant";
            break;
        case 4:
            return "Catmull-Rom";
            break;
        case 5:
            return "Cubic";
            break;
        default:
            return "";
            break;
    }
}

typedef std::list< boost::shared_ptr<Bezier> > SelectedCurves;

struct RotoPanelPrivate
{
    
    RotoPanel* publicInterface;
    
    NodeGui* node;
    RotoContext* context;
    
    QVBoxLayout* mainLayout;
    
    QWidget* splineContainer;
    QHBoxLayout* splineLayout;
    ClickableLabel* splineLabel;
    SpinBox* currentKeyframe;
    ClickableLabel* ofLabel;
    SpinBox* totalKeyframes;
    
    Button* prevKeyframe;
    Button* nextKeyframe;
    Button* addKeyframe;
    Button* removeKeyframe;
    
    QIcon iconLayer,iconBezier,iconVisible,iconUnvisible,iconLocked,iconUnlocked,iconInverted,iconUninverted,iconCurve,iconWheel;
    
    TreeWidget* tree;
    QTreeWidgetItem* treeHeader;
    
    
    SelectedCurves selectedCurves;
    
    TreeItems items;
    
    QTreeWidgetItem* editedItem;
    
    RotoPanelPrivate(RotoPanel* publicInter,NodeGui*  n)
    : publicInterface(publicInter)
    , node(n)
    , context(n->getNode()->getRotoContext().get())
    , editedItem(NULL)
    {
        assert(n && context);
        
    }
    
    void updateSplinesInfosGUI(int time);
    
    void buildTreeFromContext();
    
    TreeItems::iterator findItem(RotoItem* item)
    {
        for (TreeItems::iterator it = items.begin() ; it != items.end() ;++it) {
            if (it->rotoItem.get() == item) {
                return it;
            }
        }
        return items.end();
    }
    
    TreeItems::iterator findItem(QTreeWidgetItem* item)
    {
        for (TreeItems::iterator it = items.begin() ; it != items.end() ;++it) {
            if (it->treeItem == item) {
                return it;
            }
        }
        return items.end();
    }
    
    void insertItemRecursively(int time,const boost::shared_ptr<RotoItem>& item);
    
    void removeItemRecursively(RotoItem* item);
    
    void insertSelectionRecursively(const boost::shared_ptr<RotoLayer>& layer);
    
    void setChildrenActivatedRecursively(bool activated,QTreeWidgetItem* item);
    
    void setChildrenLockedRecursively(bool locked,QTreeWidgetItem* item);
};

RotoPanel::RotoPanel(NodeGui* n,QWidget* parent)
: QWidget(parent)
, _imp(new RotoPanelPrivate(this,n))
{
    
    QObject::connect(_imp->context, SIGNAL(selectionChanged(int)), this, SLOT(onSelectionChanged(int)));
    QObject::connect(_imp->context,SIGNAL(itemInserted()),this,SLOT(onItemInserted()));
    QObject::connect(_imp->context,SIGNAL(itemRemoved(RotoItem*)),this,SLOT(onItemRemoved(RotoItem*)));
    QObject::connect(n->getNode()->getApp()->getTimeLine().get(), SIGNAL(frameChanged(SequenceTime,int)), this,
                     SLOT(onTimeChanged(SequenceTime, int)));
    
    _imp->mainLayout = new QVBoxLayout(this);
    
    _imp->splineContainer = new QWidget(this);
    _imp->mainLayout->addWidget(_imp->splineContainer);
    
    _imp->splineLayout = new QHBoxLayout(_imp->splineContainer);
    _imp->splineLayout->setContentsMargins(0, 0, 0, 0);
    _imp->splineLayout->setSpacing(0);
    
    _imp->splineLabel = new ClickableLabel("Spline keyframe:",_imp->splineContainer);
    _imp->splineLabel->setSunken(false);
    _imp->splineLabel->setEnabled(false);
    _imp->splineLayout->addWidget(_imp->splineLabel);
    
    _imp->currentKeyframe = new SpinBox(_imp->splineContainer,SpinBox::DOUBLE_SPINBOX);
    _imp->currentKeyframe->setEnabled(false);
    _imp->currentKeyframe->setToolTip("The current keyframe for the selected shape(s)");
    _imp->splineLayout->addWidget(_imp->currentKeyframe);
    
    _imp->ofLabel = new ClickableLabel("of",_imp->splineContainer);
    _imp->ofLabel->setEnabled(false);
    _imp->splineLayout->addWidget(_imp->ofLabel);
    
    _imp->totalKeyframes = new SpinBox(_imp->splineContainer,SpinBox::INT_SPINBOX);
    _imp->totalKeyframes->setEnabled(false);
    _imp->totalKeyframes->setToolTip("The keyframe count for all the shapes.");
    _imp->splineLayout->addWidget(_imp->totalKeyframes);
    
    QPixmap prevPix,nextPix,addPix,removePix;
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_PREVIOUS_KEY, &prevPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_NEXT_KEY, &nextPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ADD_KEYFRAME, &addPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_REMOVE_KEYFRAME, &removePix);
    
    _imp->prevKeyframe = new Button(QIcon(prevPix),"",_imp->splineContainer);
    _imp->prevKeyframe->setToolTip("Go to the previous keyframe");
    _imp->prevKeyframe->setEnabled(false);
    QObject::connect(_imp->prevKeyframe, SIGNAL(clicked(bool)), this, SLOT(onGoToPrevKeyframeButtonClicked()));
    _imp->splineLayout->addWidget(_imp->prevKeyframe);
    
    _imp->nextKeyframe = new Button(QIcon(nextPix),"",_imp->splineContainer);
    _imp->nextKeyframe->setToolTip("Go to the next keyframe");
    _imp->nextKeyframe->setEnabled(false);
    QObject::connect(_imp->nextKeyframe, SIGNAL(clicked(bool)), this, SLOT(onGoToNextKeyframeButtonClicked()));
    _imp->splineLayout->addWidget(_imp->nextKeyframe);
    
    _imp->addKeyframe = new Button(QIcon(addPix),"",_imp->splineContainer);
    _imp->addKeyframe->setToolTip("Add keyframe at the current timeline's time");
    _imp->addKeyframe->setEnabled(false);
    QObject::connect(_imp->addKeyframe, SIGNAL(clicked(bool)), this, SLOT(onAddKeyframeButtonClicked()));
    _imp->splineLayout->addWidget(_imp->addKeyframe);
    
    _imp->removeKeyframe = new Button(QIcon(removePix),"",_imp->splineContainer);
    _imp->removeKeyframe->setToolTip("Remove keyframe at the current timeline's time");
    _imp->removeKeyframe->setEnabled(false);
    QObject::connect(_imp->removeKeyframe, SIGNAL(clicked(bool)), this, SLOT(onRemoveKeyframeButtonClicked()));
    _imp->splineLayout->addWidget(_imp->removeKeyframe);
    
    _imp->tree = new TreeWidget(this);
    _imp->tree->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    _imp->tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    _imp->tree->setExpandsOnDoubleClick(false);
    _imp->tree->setAttribute(Qt::WA_MacShowFocusRect,0);

    _imp->mainLayout->addWidget(_imp->tree);
    
    QObject::connect(_imp->tree, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(onItemClicked(QTreeWidgetItem*, int)));
    QObject::connect(_imp->tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onItemDoubleClicked(QTreeWidgetItem*, int)));
    QObject::connect(_imp->tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(onItemChanged(QTreeWidgetItem*, int)));
    QObject::connect(_imp->tree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this,
                     SLOT(onCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
    QObject::connect(_imp->tree, SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelectionChanged()));
    ///4 columns: Name - Visible - Locked - Overlay color - Inverted - Interpolation
    _imp->tree->setColumnCount(6);
    _imp->treeHeader = new QTreeWidgetItem;
    _imp->treeHeader->setText(0, "Name");
    
    QPixmap pixLayer,pixBezier,pixVisible,pixUnvisible,pixLocked,pixUnlocked,pixInverted,pixUninverted,pixCurve,pixWheel;
    appPTR->getIcon(NATRON_PIXMAP_LAYER, &pixLayer);
    appPTR->getIcon(NATRON_PIXMAP_BEZIER, &pixBezier);
    appPTR->getIcon(NATRON_PIXMAP_VISIBLE, &pixVisible);
    appPTR->getIcon(NATRON_PIXMAP_UNVISIBLE, &pixUnvisible);
    appPTR->getIcon(NATRON_PIXMAP_LOCKED, &pixLocked);
    appPTR->getIcon(NATRON_PIXMAP_UNLOCKED, &pixUnlocked);
    appPTR->getIcon(NATRON_PIXMAP_INVERTED, &pixInverted);
    appPTR->getIcon(NATRON_PIXMAP_UNINVERTED, &pixUninverted);
    appPTR->getIcon(NATRON_PIXMAP_CURVE, &pixCurve);
    appPTR->getIcon(NATRON_PIXMAP_COLORWHEEL, &pixWheel);
    
    _imp->iconLayer.addPixmap(pixLayer);
    _imp->iconBezier.addPixmap(pixBezier);
    _imp->iconCurve.addPixmap(pixCurve);
    _imp->iconInverted.addPixmap(pixInverted);
    _imp->iconUninverted.addPixmap(pixUninverted);
    _imp->iconVisible.addPixmap(pixVisible);
    _imp->iconUnvisible.addPixmap(pixUnvisible);
    _imp->iconLocked.addPixmap(pixLocked);
    _imp->iconUnlocked.addPixmap(pixUnlocked);
    _imp->iconWheel.addPixmap(pixWheel);
    
    _imp->treeHeader->setIcon(1, _imp->iconVisible);
    _imp->treeHeader->setIcon(2, _imp->iconLocked);
    _imp->treeHeader->setIcon(3, _imp->iconWheel);
    _imp->treeHeader->setIcon(4, _imp->iconUninverted);
    _imp->treeHeader->setIcon(5, _imp->iconCurve);
    _imp->tree->setHeaderItem(_imp->treeHeader);
    
    _imp->tree->setColumnWidth(1, 25);
    _imp->tree->setColumnWidth(2, 25);
    _imp->tree->setColumnWidth(3, 25);
    _imp->tree->setColumnWidth(4, 25);

    _imp->tree->header()->setResizeMode(QHeaderView::ResizeToContents);
    
    _imp->buildTreeFromContext();
    
    ///refresh selection
    onSelectionChanged(RotoContext::OTHER);

}

RotoPanel::~RotoPanel()
{
    
}

void RotoPanel::onGoToPrevKeyframeButtonClicked()
{
    _imp->context->goToPreviousKeyframe();
}

void RotoPanel::onGoToNextKeyframeButtonClicked()
{
    _imp->context->goToNextKeyframe();
}

void RotoPanel::onAddKeyframeButtonClicked()
{
    _imp->context->setKeyframeOnSelectedCurves();
}

void RotoPanel::onRemoveKeyframeButtonClicked()
{
    _imp->context->removeKeyframeOnSelectedCurves();
}

void RotoPanel::onSelectionChangedInternal()
{
    ///disconnect previous selection
    for (SelectedCurves::const_iterator it = _imp->selectedCurves.begin(); it!=_imp->selectedCurves.end(); ++it) {
        QObject::disconnect(it->get(), SIGNAL(keyframeSet(int)), this, SLOT(onSelectedBezierKeyframeSet(int)));
        QObject::disconnect(it->get(), SIGNAL(keyframeRemoved(int)), this, SLOT(onSelectedBezierKeyframeRemoved(int)));
    }
    _imp->selectedCurves.clear();
    
    ///connect new selection
    const std::list<boost::shared_ptr<Bezier> >& curves = _imp->context->getSelectedCurves();
    for (std::list<boost::shared_ptr<Bezier> >::const_iterator it = curves.begin(); it!=curves.end(); ++it) {
        _imp->selectedCurves.push_back(*it);
        QObject::connect((*it).get(), SIGNAL(keyframeSet(int)), this, SLOT(onSelectedBezierKeyframeSet(int)));
        QObject::connect((*it).get(), SIGNAL(keyframeRemoved(int)), this, SLOT(onSelectedBezierKeyframeRemoved(int)));
    }
    
    bool enabled = !_imp->selectedCurves.empty();
    
    _imp->splineLabel->setEnabled(enabled);
    _imp->currentKeyframe->setEnabled(enabled);
    _imp->ofLabel->setEnabled(enabled);
    _imp->totalKeyframes->setEnabled(enabled);
    _imp->prevKeyframe->setEnabled(enabled);
    _imp->nextKeyframe->setEnabled(enabled);
    _imp->addKeyframe->setEnabled(enabled);
    _imp->removeKeyframe->setEnabled(enabled);
    
    int time = _imp->context->getTimelineCurrentTime();
    
    ///update the splines info GUI
    _imp->updateSplinesInfosGUI(time);
}

void RotoPanel::onSelectionChanged(int reason)
{
    if ((RotoContext::SelectionReason)reason == RotoContext::SETTINGS_PANEL) {
        return;
    }
    
    onSelectionChangedInternal();
    _imp->tree->blockSignals(true);
    _imp->tree->clearSelection();
    ///now update the selection according to the selected curves
    ///Note that layers will not be selected because this is called when the user clicks on a overlay on the viewer
    for (SelectedCurves::const_iterator it = _imp->selectedCurves.begin(); it!=_imp->selectedCurves.end(); ++it) {
        TreeItems::iterator found = _imp->findItem(it->get());
        assert(found != _imp->items.end());
        found->treeItem->setSelected(true);
    }
    _imp->tree->blockSignals(false);

}

void RotoPanel::onSelectedBezierKeyframeSet(int time)
{
    _imp->updateSplinesInfosGUI(time);
}

void RotoPanel::onSelectedBezierKeyframeRemoved(int time)
{
    _imp->updateSplinesInfosGUI(time);
}



void RotoPanelPrivate::updateSplinesInfosGUI(int time)
{
    std::set<int> keyframes;
    for (SelectedCurves::const_iterator it = selectedCurves.begin(); it!=selectedCurves.end(); ++it) {
        (*it)->getKeyframeTimes(&keyframes);
    }
    totalKeyframes->setValue((double)keyframes.size());
    
    if (keyframes.empty()) {
        currentKeyframe->setValue(1.);
        currentKeyframe->setAnimation(0);
    } else {
        ///get the first time that is equal or greater to the current time
        std::set<int>::iterator lowerBound = keyframes.lower_bound(time);
        int dist = 0;
        if (lowerBound != keyframes.end()) {
            dist = std::distance(keyframes.begin(), lowerBound);
        }
        
        if (lowerBound == keyframes.end()) {
            ///we're after the last keyframe
            currentKeyframe->setValue((double)keyframes.size());
            currentKeyframe->setAnimation(1);
        } else if (*lowerBound == time) {
            currentKeyframe->setValue(dist + 1);
            currentKeyframe->setAnimation(2);
        } else {
            ///we're in-between 2 keyframes, interpolate
            if (lowerBound == keyframes.begin()) {
                currentKeyframe->setValue(1.);
            } else {
                std::set<int>::iterator prev = lowerBound;
                --prev;
                currentKeyframe->setValue((double)(time - *prev) / (double)(*lowerBound - *prev) + dist);
            }
            
            currentKeyframe->setAnimation(1);
        }
    }
    
    ///Refresh the interpolation + inverted state
    for (TreeItems::iterator it = items.begin(); it != items.end(); ++it) {
        RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>(it->rotoItem.get());
        if (drawable) {
            it->treeItem->setIcon(4,drawable->getInverted(time) ? iconInverted : iconUninverted);
            it->treeItem->setText(5,interpName(drawable->getInterpolation(time)));
        }
    }
}

void RotoPanel::onTimeChanged(SequenceTime time,int /*reason*/)
{
    _imp->updateSplinesInfosGUI(time);
}

static void expandRecursively(QTreeWidgetItem* item)
{
    item->setExpanded(true);
    if (item->parent()) {
        expandRecursively(item->parent());
    }
}

void RotoPanelPrivate::insertItemRecursively(int time,const boost::shared_ptr<RotoItem>& item)
{
    QTreeWidgetItem* treeItem = new QTreeWidgetItem;
    RotoItem* parent = item->getParentLayer();
    if (parent) {
        TreeItems::iterator parentIT = findItem(parent);
        
        ///the parent must have already been inserted!
        assert(parentIT != items.end());
        parentIT->treeItem->addChild(treeItem);
    } else {
        tree->addTopLevelItem(treeItem);
    }
    items.push_back(TreeItem(treeItem,item));
    //treeItem->setFlags(treeItem->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
    treeItem->setText(0, item->getName_mt_safe().c_str());
    treeItem->setIcon(1, item->isGloballyActivated() ? iconVisible : iconUnvisible);
    treeItem->setIcon(2, item->getLocked() ? iconLocked : iconUnlocked);
    
    RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>(item.get());
    RotoLayer* layer = dynamic_cast<RotoLayer*>(item.get());
    
    if (drawable) {
        double overlayColor[4];
        drawable->getOverlayColor(overlayColor);
        QPixmap p(15,15);
        QColor c;
        c.setRgbF(overlayColor[0], overlayColor[1], overlayColor[2]);
        c.setAlphaF(overlayColor[3]);
        p.fill(c);
        treeItem->setIcon(0, iconBezier);
        treeItem->setIcon(3,QIcon(p));
        treeItem->setIcon(4, drawable->getInverted(time)  ? iconInverted : iconUninverted);
        treeItem->setText(5, interpName(drawable->getInterpolation(time)));
        
        QObject::connect(drawable,SIGNAL(interpolationChanged()), publicInterface, SLOT(onRotoItemInterpolationChanged()));
        QObject::connect(drawable,SIGNAL(inversionChanged()), publicInterface, SLOT(onRotoItemInversionChanged()));
        
    } else {
        treeItem->setIcon(0, iconLayer);
        ///insert children
        const std::list<boost::shared_ptr<RotoItem> >& children = layer->getItems();
        for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = children.begin(); it!= children.end(); ++it) {
            insertItemRecursively(time,*it);
        }
    }
    expandRecursively(treeItem);
}

void RotoPanelPrivate::removeItemRecursively(RotoItem* item)
{
    TreeItems::iterator it = findItem(item);
    if (it == items.end()) {
        return;
    }
    RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>(item);
    if (drawable) {

        QObject::disconnect(drawable,SIGNAL(interpolationChanged()), publicInterface, SLOT(onRotoItemInterpolationChanged()));
        QObject::disconnect(drawable,SIGNAL(inversionChanged()), publicInterface, SLOT(onRotoItemInversionChanged()));
        
    }
    
    delete it->treeItem;
    items.erase(it);
}


void RotoPanel::onItemInserted()
{
    int time = _imp->context->getTimelineCurrentTime();
    boost::shared_ptr<RotoItem> lastInsertedItem = _imp->context->getLastInsertedItem();
    assert(lastInsertedItem);
    _imp->insertItemRecursively(time, lastInsertedItem);
}

void RotoPanel::onItemRemoved(RotoItem* item)
{
    _imp->removeItemRecursively(item);
}

void RotoPanelPrivate::buildTreeFromContext()
{
    
    int time = context->getTimelineCurrentTime();
    const std::list< boost::shared_ptr<RotoLayer> >& layers = context->getLayers();
    if (!layers.empty()) {
        const boost::shared_ptr<RotoLayer>& base = layers.front();
        insertItemRecursively(time, base);
    }
    
}

void RotoPanel::onRotoItemInterpolationChanged()
{
    RotoDrawableItem* item = qobject_cast<RotoDrawableItem*>(sender());
    if (item) {
        int time = _imp->context->getTimelineCurrentTime();
        TreeItems::iterator it = _imp->findItem(item);
        if (it != _imp->items.end()) {
            it->treeItem->setText(5, interpName(item->getInterpolation(time)));
        }
    }
}

void RotoPanel::onRotoItemInversionChanged()
{
    RotoDrawableItem* item = qobject_cast<RotoDrawableItem*>(sender());
    if (item) {
        int time = _imp->context->getTimelineCurrentTime();
        TreeItems::iterator it = _imp->findItem(item);
        if (it != _imp->items.end()) {
            it->treeItem->setIcon(4, item->getInverted(time)  ? _imp->iconInverted : _imp->iconUninverted);
        }
    }
}

static void createMenuAction(QMenu* menu,int index,int currentIndex)
{
    QAction* act = new QAction(menu);
    act->setCheckable(true);
    act->setChecked(currentIndex == index);
    act->setData(QVariant(index));
    act->setText(interpName(index));
    menu->addAction(act);

}

void RotoPanel::onItemClicked(QTreeWidgetItem* item,int column)
{

    TreeItems::iterator it = _imp->findItem(item);
    if (it != _imp->items.end()) {
        int time = _imp->context->getTimelineCurrentTime();
        switch (column) {
                ///visible
            case 1:
            {
                bool activated = !it->rotoItem->isGloballyActivated();
                QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
                for (int i = 0; i < selected.size(); ++i) {
                    TreeItems::iterator found = _imp->findItem(selected[i]);
                    assert(found != _imp->items.end());
                    found->rotoItem->setGloballyActivated(activated, true);
                    _imp->setChildrenActivatedRecursively(activated, found->treeItem);
                }
                
            }   break;
                ///locked
            case 2:
            {
                bool locked = !it->rotoItem->getLocked();
                QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
                for (int i = 0; i < selected.size(); ++i) {
                    TreeItems::iterator found = _imp->findItem(selected[i]);
                    assert(found != _imp->items.end());
                    found->rotoItem->setLocked(locked,true);
                    _imp->context->setLastItemLocked(found->rotoItem);
                    _imp->setChildrenLockedRecursively(locked, found->treeItem);
                }
                
            }   break;
                ///overlay color
            case 3:
            {
                RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>(it->rotoItem.get());
                if (drawable) {
                    QColorDialog dialog;
                    double oc[4];
                    drawable->getOverlayColor(oc);
                    QColor color;
                    color.setRgbF(oc[0], oc[1], oc[2]);
                    color.setAlphaF(oc[3]);
                    dialog.setCurrentColor(color);
                    if (dialog.exec()) {
                        color = dialog.selectedColor();
                        oc[0] = color.redF();
                        oc[1] = color.greenF();
                        oc[2] = color.blueF();
                        oc[3] = color.alphaF();
                        QPixmap pix(15,15);
                        pix.fill(color);

                        QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
                        for (int i = 0; i < selected.size(); ++i) {
                            TreeItems::iterator found = _imp->findItem(selected[i]);
                            assert(found != _imp->items.end());
                            drawable = dynamic_cast<RotoDrawableItem*>(found->rotoItem.get());
                            drawable->setOverlayColor(oc);
                            found->treeItem->setIcon(3, QIcon(pix));
                        }
                    }
                }
            }   break;
                ///inverted
            case 4:
            {
                QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
                bool inverted = false;
                for (int i = 0; i < selected.size(); ++i) {
                    TreeItems::iterator found = _imp->findItem(selected[i]);
                    assert(found != _imp->items.end());
                    RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>(found->rotoItem.get());
                    if (drawable) {
                        boost::shared_ptr<Bool_Knob> invertedKnob = drawable->getInvertedKnob();
                        inverted = !invertedKnob->getValueAtTime(time);
                        bool isOnKeyframe = invertedKnob->getKeyFrameIndex(0, time) != -1;
                        inverted = !drawable->getInverted(time);
                        
                        if (_imp->context->isAutoKeyingEnabled() || isOnKeyframe) {
                            invertedKnob->setValueAtTime(time, inverted, 0);
                        } else {
                            invertedKnob->setValue(inverted, 0);
                        }
                        found->treeItem->setIcon(4, inverted ? _imp->iconInverted : _imp->iconUninverted);
                    }
                }
                if (!selected.empty()) {
                    _imp->context->getInvertedKnob()->setValueAtTime(time, inverted, 0);
                
                }
                
                
            }
                break;
                ///show menu for interpolation
            case 5:
            {
                RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>(it->rotoItem.get());
                if (drawable) {
                    int interp = drawable->getInterpolation(time);
                    QMenu menu(this);
                    for (int j = 0; j < 5; ++j ) {
                        createMenuAction(&menu,j,interp);
                    }
                    QAction* ret = menu.exec(QCursor::pos());
                    if (ret) {
                        int index = ret->data().toInt();
                        
                        QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
                        for (int i = 0; i < selected.size(); ++i) {
                            TreeItems::iterator found = _imp->findItem(selected[i]);
                            assert(found != _imp->items.end());
                            drawable = dynamic_cast<RotoDrawableItem*>(found->rotoItem.get());
                            boost::shared_ptr<Choice_Knob> interpolationKnob = drawable->getInterpolationKnob();
                            bool isOnKeyframe = interpolationKnob->getKeyFrameIndex(0, time) != -1;
                            if (_imp->context->isAutoKeyingEnabled() || isOnKeyframe) {
                                interpolationKnob->setValueAtTime(time, index, 0);
                            } else {
                                interpolationKnob->setValue(index, 0);
                            }
                            found->treeItem->setText(5, interpName(index));
                        }
                        if (!selected.empty()) {
                            _imp->context->getInterpolationKnob()->setValueAtTime(time, index, 0);
                        }
                        
                    }
                }
                
                
                
            }   break;
            case 0:
            default:
                break;
        }
    }
}

void RotoPanelPrivate::setChildrenActivatedRecursively(bool activated,QTreeWidgetItem* item)
{
    item->setIcon(1, activated ? iconVisible : iconUnvisible);
    for (int i = 0; i < item->childCount(); ++i) {
        setChildrenActivatedRecursively(activated,item->child(i));
    }
}

void RotoPanelPrivate::setChildrenLockedRecursively(bool locked,QTreeWidgetItem* item)
{
    item->setIcon(2, locked ? iconLocked : iconUnlocked);
    for (int i = 0; i < item->childCount(); ++i) {
        setChildrenLockedRecursively(locked,item->child(i));
    }

}

void RotoPanel::onItemChanged(QTreeWidgetItem* item,int column)
{
    if (column != 0) {
        return;
    }
    TreeItems::iterator it = _imp->findItem(item);
    if (it != _imp->items.end()) {
        it->rotoItem->setName(item->text(column).toStdString());
    }
}

void RotoPanel::onItemDoubleClicked(QTreeWidgetItem* item,int column)
{
    if (column != 0) {
        return;
    }
    TreeItems::iterator it = _imp->findItem(item);
    if (it != _imp->items.end()) {
        _imp->editedItem = item;
        QObject::connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)), this, SLOT(onFocusChanged(QWidget*,QWidget*)));
        _imp->tree->openPersistentEditor(item);
    }
}

void RotoPanel::onCurrentItemChanged(QTreeWidgetItem* /*current*/,QTreeWidgetItem* /*previous*/)
{
    onTreeOutOfFocusEvent();
}

void RotoPanel::onTreeOutOfFocusEvent()
{
    if (_imp->editedItem) {
        _imp->tree->closePersistentEditor(_imp->editedItem);
        _imp->editedItem = NULL;
    }
    
}

void RotoPanel::onFocusChanged(QWidget* old,QWidget*)
{
    if (_imp->editedItem) {
        QWidget* w = _imp->tree->itemWidget(_imp->editedItem, 0);
        if (w == old) {
            _imp->tree->closePersistentEditor(_imp->editedItem);
            _imp->editedItem = NULL;
        }
    }
}

void RotoPanelPrivate::insertSelectionRecursively(const boost::shared_ptr<RotoLayer>& layer)
{
    const std::list<boost::shared_ptr<RotoItem> >& children = layer->getItems();
    for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = children.begin(); it!=children.end(); ++it) {
        boost::shared_ptr<Bezier> bezier = boost::dynamic_pointer_cast<Bezier>(*it);
        boost::shared_ptr<RotoLayer> l = boost::dynamic_pointer_cast<RotoLayer>(*it);
        if (bezier) {
            SelectedCurves::iterator found = std::find(selectedCurves.begin(), selectedCurves.end(), bezier);
            if (found == selectedCurves.end()) {
                context->select(bezier, RotoContext::SETTINGS_PANEL);
                selectedCurves.push_back(bezier);
            }
            
        } else if (l) {
            insertSelectionRecursively(l);
        }
    }
}

void RotoPanel::onItemSelectionChanged()
{
    ///disconnect previous selection
    for (SelectedCurves::const_iterator it = _imp->selectedCurves.begin(); it!=_imp->selectedCurves.end(); ++it) {
        QObject::disconnect(it->get(), SIGNAL(keyframeSet(int)), this, SLOT(onSelectedBezierKeyframeSet(int)));
        QObject::disconnect(it->get(), SIGNAL(keyframeRemoved(int)), this, SLOT(onSelectedBezierKeyframeRemoved(int)));
    }
    _imp->context->deselect(_imp->selectedCurves, RotoContext::SETTINGS_PANEL);
    _imp->selectedCurves.clear();
    
    QList<QTreeWidgetItem*> selectedItems = _imp->tree->selectedItems();
    
    for (int i = 0; i < selectedItems.size(); ++i) {
        TreeItems::iterator it = _imp->findItem(selectedItems[i]);
        assert(it != _imp->items.end());
        boost::shared_ptr<Bezier> bezier = boost::dynamic_pointer_cast<Bezier>(it->rotoItem);
        boost::shared_ptr<RotoLayer> layer = boost::dynamic_pointer_cast<RotoLayer>(it->rotoItem);
        if (bezier) {
            SelectedCurves::iterator found = std::find(_imp->selectedCurves.begin(), _imp->selectedCurves.end(), bezier);
            if (found == _imp->selectedCurves.end()) {
                _imp->selectedCurves.push_back(bezier);
            }
        } else if (layer) {
            _imp->insertSelectionRecursively(layer);
        }
    }
    _imp->context->select(_imp->selectedCurves, RotoContext::SETTINGS_PANEL);
    
    bool enabled = !_imp->selectedCurves.empty();
    
    _imp->splineLabel->setEnabled(enabled);
    _imp->currentKeyframe->setEnabled(enabled);
    _imp->ofLabel->setEnabled(enabled);
    _imp->totalKeyframes->setEnabled(enabled);
    _imp->prevKeyframe->setEnabled(enabled);
    _imp->nextKeyframe->setEnabled(enabled);
    _imp->addKeyframe->setEnabled(enabled);
    _imp->removeKeyframe->setEnabled(enabled);
    
    int time = _imp->context->getTimelineCurrentTime();
    
    ///update the splines info GUI
    _imp->updateSplinesInfosGUI(time);

}