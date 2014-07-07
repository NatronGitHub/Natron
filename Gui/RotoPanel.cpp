
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
#include <QDataStream>
#include <QMimeData>
#include <QImage>
#include <QPainter>
#include <QByteArray>

#include "Gui/Button.h"
#include "Gui/SpinBox.h"
#include "Gui/ClickableLabel.h"
#include "Gui/NodeGui.h"
#include "Gui/DockablePanel.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/RotoUndoCommand.h"
#include "Gui/ComboBox.h"

#include "Engine/RotoContext.h"
#include "Engine/TimeLine.h"
#include "Engine/Node.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Image.h"
#include "Engine/RotoContextPrivate.h" // for getCompositingOperators

#define COL_NAME 0
#define COL_ACTIVATED 1
#define COL_LOCKED 2
#define COL_OVERLAY 3
#define COL_COLOR 4
#define COL_OPERATOR 5
#define COL_INVERTED 6

#define MAX_COLS 7

using namespace Natron;


static QPixmap getColorButtonDefaultPixmap()
{
    QImage img(15,15,QImage::Format_ARGB32);
    QColor gray(Qt::gray);
    img.fill(gray.rgba());
    QPainter p(&img);
    QPen pen;
    pen.setColor(Qt::black);
    pen.setWidth(2);
    p.setPen(pen);
    p.drawLine(0, 0, 14, 14);
    return QPixmap::fromImage(img);
}



class TreeWidget: public QTreeWidget
{
    
    RotoPanel* _panel;
    
public:
    
    TreeWidget(RotoPanel* panel,QWidget* parent): QTreeWidget(parent) , _panel(panel) {
    
    }
    
    virtual ~TreeWidget(){}
    
    
private:
    
    virtual void mouseReleaseEvent(QMouseEvent* event) OVERRIDE FINAL
    {
        QModelIndex index = indexAt(event->pos());
        QTreeWidgetItem* item = itemAt(event->pos());
        QList<QTreeWidgetItem*> selection = selectedItems();
        
        if (index.isValid() && index.column() != 0 && selection.contains(item)) {
            emit itemClicked(item, index.column());
        } else if (event->button() == Qt::RightButton && index.isValid()) {
            _panel->showItemMenu(item,event->globalPos());
        } else {
            QTreeWidget::mouseReleaseEvent(event);
        }
    }
    
    virtual void dragMoveEvent(QDragMoveEvent * event) OVERRIDE FINAL;
    
    virtual void dropEvent(QDropEvent* event) OVERRIDE FINAL;
    
    virtual void keyPressEvent(QKeyEvent* event) OVERRIDE FINAL;
    
    bool dragAndDropHandler(const QMimeData* mime,
                            const QPoint& pos,
                            std::list<DroppedTreeItemPtr>& dropped);
};


struct TreeItem
{
    QTreeWidgetItem* treeItem;
    boost::shared_ptr<RotoItem> rotoItem;
    
    TreeItem() : treeItem(0) , rotoItem() {}
    
    TreeItem(QTreeWidgetItem* t,const boost::shared_ptr<RotoItem>& r): treeItem(t) , rotoItem(r) {}
};

typedef std::list< TreeItem > TreeItems;


typedef std::list< boost::shared_ptr<RotoItem> > SelectedItems;

typedef std::map<RotoItem*, std::set<int> > ItemKeys;

enum ColorDialogEdition
{
    EDITING_NOTHING = 0,
    EDITING_OVERLAY_COLOR,
    EDITING_SHAPE_COLOR
};

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
    
    QWidget* buttonContainer;
    QHBoxLayout* buttonLayout;
    Button* addLayerButton;
    Button* removeItemButton;
    
    QIcon iconLayer,iconBezier,iconVisible,iconUnvisible,iconLocked,iconUnlocked,iconInverted,iconUninverted,iconWheel;
    
    TreeWidget* tree;
    QTreeWidgetItem* treeHeader;
    
    
    SelectedItems selectedItems;
    
    TreeItems items;
    
    QTreeWidgetItem* editedItem;
    std::string editedItemName;
    
    QTreeWidgetItem* lastRightClickedItem;
    QList<QTreeWidgetItem*> clipBoard;
    
    ItemKeys keyframes; //< track of all keyframes for items
    
    ColorDialogEdition dialogEdition;
    
    RotoPanelPrivate(RotoPanel* publicInter,NodeGui*  n)
    : publicInterface(publicInter)
    , node(n)
    , context(n->getNode()->getRotoContext().get())
    , editedItem(NULL)
    , lastRightClickedItem(NULL)
    , clipBoard()
    , dialogEdition(EDITING_NOTHING)
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
    
    bool itemHasKey(RotoItem* item,int time) const;
    
    void setItemKey(RotoItem* item,int time);
    
    void removeItemKey(RotoItem* item,int time);
};

RotoPanel::RotoPanel(NodeGui* n,QWidget* parent)
: QWidget(parent)
, _imp(new RotoPanelPrivate(this,n))
{
    
    QObject::connect(_imp->context, SIGNAL(selectionChanged(int)), this, SLOT(onSelectionChanged(int)));
    QObject::connect(_imp->context,SIGNAL(itemInserted(int)),this,SLOT(onItemInserted(int)));
    QObject::connect(_imp->context,SIGNAL(itemRemoved(RotoItem*,int)),this,SLOT(onItemRemoved(RotoItem*,int)));
    QObject::connect(n->getNode()->getApp()->getTimeLine().get(), SIGNAL(frameChanged(SequenceTime,int)), this,
                     SLOT(onTimeChanged(SequenceTime, int)));
    QObject::connect(n, SIGNAL(settingsPanelClosed(bool)), this, SLOT(onSettingsPanelClosed(bool)));
    
    _imp->mainLayout = new QVBoxLayout(this);
    
    _imp->splineContainer = new QWidget(this);
    _imp->mainLayout->addWidget(_imp->splineContainer);
    
    _imp->splineLayout = new QHBoxLayout(_imp->splineContainer);
    _imp->splineLayout->setSpacing(2);
    _imp->splineLabel = new ClickableLabel("Spline keyframe:",_imp->splineContainer);
    _imp->splineLabel->setSunken(false);
    _imp->splineLabel->setEnabled(false);
    _imp->splineLayout->addWidget(_imp->splineLabel);
    
    _imp->currentKeyframe = new SpinBox(_imp->splineContainer,SpinBox::DOUBLE_SPINBOX);
    _imp->currentKeyframe->setEnabled(false);
    _imp->currentKeyframe->setReadOnly(true);
    _imp->currentKeyframe->setToolTip("The current keyframe for the selected shape(s)");
    _imp->splineLayout->addWidget(_imp->currentKeyframe);
    
    _imp->ofLabel = new ClickableLabel("of",_imp->splineContainer);
    _imp->ofLabel->setEnabled(false);
    _imp->splineLayout->addWidget(_imp->ofLabel);
    
    _imp->totalKeyframes = new SpinBox(_imp->splineContainer,SpinBox::INT_SPINBOX);
    _imp->totalKeyframes->setEnabled(false);
    _imp->totalKeyframes->setReadOnly(true);
    _imp->totalKeyframes->setToolTip("The keyframe count for all the selected shapes.");
    _imp->splineLayout->addWidget(_imp->totalKeyframes);
    
    QPixmap prevPix,nextPix,addPix,removePix;
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_PREVIOUS_KEY, &prevPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_NEXT_KEY, &nextPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ADD_KEYFRAME, &addPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_REMOVE_KEYFRAME, &removePix);
    
    _imp->prevKeyframe = new Button(QIcon(prevPix),"",_imp->splineContainer);
    _imp->prevKeyframe->setToolTip("Go to the previous keyframe");
    _imp->prevKeyframe->setEnabled(false);
    _imp->prevKeyframe->setFixedSize(20,20);
    QObject::connect(_imp->prevKeyframe, SIGNAL(clicked(bool)), this, SLOT(onGoToPrevKeyframeButtonClicked()));
    _imp->splineLayout->addWidget(_imp->prevKeyframe);
    
    _imp->nextKeyframe = new Button(QIcon(nextPix),"",_imp->splineContainer);
    _imp->nextKeyframe->setToolTip("Go to the next keyframe");
    _imp->nextKeyframe->setEnabled(false);
    _imp->nextKeyframe->setFixedSize(20,20);
    QObject::connect(_imp->nextKeyframe, SIGNAL(clicked(bool)), this, SLOT(onGoToNextKeyframeButtonClicked()));
    _imp->splineLayout->addWidget(_imp->nextKeyframe);
    
    _imp->addKeyframe = new Button(QIcon(addPix),"",_imp->splineContainer);
    _imp->addKeyframe->setToolTip("Add keyframe at the current timeline's time");
    _imp->addKeyframe->setEnabled(false);
    _imp->addKeyframe->setFixedSize(20,20);
    QObject::connect(_imp->addKeyframe, SIGNAL(clicked(bool)), this, SLOT(onAddKeyframeButtonClicked()));
    _imp->splineLayout->addWidget(_imp->addKeyframe);
    
    _imp->removeKeyframe = new Button(QIcon(removePix),"",_imp->splineContainer);
    _imp->removeKeyframe->setToolTip("Remove keyframe at the current timeline's time");
    _imp->removeKeyframe->setEnabled(false);
    _imp->removeKeyframe->setFixedSize(20,20);
    QObject::connect(_imp->removeKeyframe, SIGNAL(clicked(bool)), this, SLOT(onRemoveKeyframeButtonClicked()));
    _imp->splineLayout->addWidget(_imp->removeKeyframe);
    _imp->splineLayout->addStretch();
    
    _imp->tree = new TreeWidget(this,this);
    _imp->tree->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    _imp->tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    _imp->tree->setDragDropMode(QAbstractItemView::InternalMove);
    _imp->tree->setDragEnabled(true);
    _imp->tree->setExpandsOnDoubleClick(false);
    _imp->tree->setAttribute(Qt::WA_MacShowFocusRect,0);

    _imp->mainLayout->addWidget(_imp->tree);
    
    QObject::connect(_imp->tree, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(onItemClicked(QTreeWidgetItem*, int)));
    QObject::connect(_imp->tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onItemDoubleClicked(QTreeWidgetItem*, int)));
    QObject::connect(_imp->tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(onItemChanged(QTreeWidgetItem*, int)));
    QObject::connect(_imp->tree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this,
                     SLOT(onCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
    QObject::connect(_imp->tree, SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelectionChanged()));
    

    _imp->tree->setColumnCount(MAX_COLS);
    _imp->treeHeader = new QTreeWidgetItem;
    _imp->treeHeader->setText(0, "Name");
    
    QPixmap pixLayer,pixBezier,pixVisible,pixUnvisible,pixLocked,pixUnlocked,pixInverted,pixUninverted,pixWheel,pixDefault,pixmerge;
    appPTR->getIcon(NATRON_PIXMAP_LAYER, &pixLayer);
    appPTR->getIcon(NATRON_PIXMAP_BEZIER, &pixBezier);
    appPTR->getIcon(NATRON_PIXMAP_VISIBLE, &pixVisible);
    appPTR->getIcon(NATRON_PIXMAP_UNVISIBLE, &pixUnvisible);
    appPTR->getIcon(NATRON_PIXMAP_LOCKED, &pixLocked);
    appPTR->getIcon(NATRON_PIXMAP_UNLOCKED, &pixUnlocked);
    appPTR->getIcon(NATRON_PIXMAP_INVERTED, &pixInverted);
    appPTR->getIcon(NATRON_PIXMAP_UNINVERTED, &pixUninverted);
    appPTR->getIcon(NATRON_PIXMAP_COLORWHEEL, &pixWheel);
    appPTR->getIcon(NATRON_PIXMAP_MERGE_GROUPING, &pixmerge);
    pixmerge = pixmerge.scaled(15, 15);
    pixDefault = getColorButtonDefaultPixmap();
    
    _imp->iconLayer.addPixmap(pixLayer);
    _imp->iconBezier.addPixmap(pixBezier);
    _imp->iconInverted.addPixmap(pixInverted);
    _imp->iconUninverted.addPixmap(pixUninverted);
    _imp->iconVisible.addPixmap(pixVisible);
    _imp->iconUnvisible.addPixmap(pixUnvisible);
    _imp->iconLocked.addPixmap(pixLocked);
    _imp->iconUnlocked.addPixmap(pixUnlocked);
    _imp->iconWheel.addPixmap(pixWheel);
    
    _imp->treeHeader->setIcon(COL_ACTIVATED, _imp->iconVisible);
    _imp->treeHeader->setIcon(COL_LOCKED, _imp->iconLocked);
    _imp->treeHeader->setIcon(COL_OVERLAY, QIcon(pixDefault));
    _imp->treeHeader->setIcon(COL_COLOR, _imp->iconWheel);
    _imp->treeHeader->setIcon(COL_INVERTED, _imp->iconUninverted);
    _imp->treeHeader->setIcon(COL_OPERATOR, QIcon(pixmerge));
    _imp->tree->setHeaderItem(_imp->treeHeader);
    
    for (int i = 1; i < MAX_COLS; ++i) {
        _imp->tree->setColumnWidth(i, 25);
    }


#if QT_VERSION < 0x050000
    _imp->tree->header()->setResizeMode(QHeaderView::ResizeToContents);
#else
    _imp->tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
    _imp->buttonContainer = new QWidget(this);
    _imp->buttonLayout = new QHBoxLayout(_imp->buttonContainer);
    _imp->buttonLayout->setContentsMargins(0, 0, 0, 0);
    _imp->buttonLayout->setSpacing(0);
    
    _imp->addLayerButton = new Button("+",_imp->buttonContainer);
    _imp->addLayerButton->setToolTip("Add a new layer");
    _imp->buttonLayout->addWidget(_imp->addLayerButton);
    QObject::connect(_imp->addLayerButton, SIGNAL(clicked(bool)), this, SLOT(onAddLayerButtonClicked()));
    
    _imp->removeItemButton = new Button("-",_imp->buttonContainer);
    _imp->removeItemButton->setToolTip("Remove selected items");
    _imp->buttonLayout->addWidget(_imp->removeItemButton);
    QObject::connect(_imp->removeItemButton, SIGNAL(clicked(bool)), this, SLOT(onRemoveItemButtonClicked()));
    
    _imp->buttonLayout->addStretch();
    
    _imp->mainLayout->addWidget(_imp->buttonContainer);
    
    _imp->buildTreeFromContext();
    
    ///refresh selection
    onSelectionChanged(RotoContext::OTHER);

}

RotoPanel::~RotoPanel()
{
    
}

boost::shared_ptr<RotoItem> RotoPanel::getRotoItemForTreeItem(QTreeWidgetItem* treeItem) const
{
    TreeItems::iterator it =_imp->findItem(treeItem);
    if (it != _imp->items.end()) {
        return it->rotoItem;
    }
    return boost::shared_ptr<RotoItem>();
}

QTreeWidgetItem* RotoPanel::getTreeItemForRotoItem(const boost::shared_ptr<RotoItem>& item) const
{
    TreeItems::iterator it = _imp->findItem(item.get());
    if (it != _imp->items.end()) {
        return it->treeItem;
    }
    return NULL;
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
    for (SelectedItems::const_iterator it = _imp->selectedItems.begin(); it!=_imp->selectedItems.end(); ++it) {
        Bezier* isBezier = dynamic_cast<Bezier*>(it->get());
        if (isBezier) {
            QObject::disconnect(isBezier, SIGNAL(keyframeSet(int)), this, SLOT(onSelectedBezierKeyframeSet(int)));
            QObject::disconnect(isBezier, SIGNAL(keyframeRemoved(int)), this, SLOT(onSelectedBezierKeyframeRemoved(int)));
            QObject::disconnect(isBezier, SIGNAL(aboutToClone()), this, SLOT(onSelectedBezierAboutToClone()));
            QObject::disconnect(isBezier, SIGNAL(cloned()), this, SLOT(onSelectedBezierCloned()));
        }
    }
    _imp->selectedItems.clear();
    
    ///connect new selection
    int selectedBeziersCount = 0;
    const std::list<boost::shared_ptr<RotoItem> >& items = _imp->context->getSelectedItems();
    for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = items.begin(); it!=items.end(); ++it) {
        _imp->selectedItems.push_back(*it);
        const Bezier* isBezier = dynamic_cast<const Bezier*>(it->get());
        const RotoLayer* isLayer = dynamic_cast<const RotoLayer*>(it->get());
        if (isBezier) {
            QObject::connect(isBezier, SIGNAL(keyframeSet(int)), this, SLOT(onSelectedBezierKeyframeSet(int)));
            QObject::connect(isBezier, SIGNAL(keyframeRemoved(int)), this, SLOT(onSelectedBezierKeyframeRemoved(int)));
            QObject::connect(isBezier, SIGNAL(aboutToClone()), this, SLOT(onSelectedBezierAboutToClone()));
            QObject::connect(isBezier, SIGNAL(cloned()), this, SLOT(onSelectedBezierCloned()));
            ++selectedBeziersCount;
        } else if (isLayer && !isLayer->getItems().empty()) {
            ++selectedBeziersCount;
        }
        
    }
    
    bool enabled = selectedBeziersCount > 0;
    
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
    for (SelectedItems::iterator it = _imp->selectedItems.begin(); it!=_imp->selectedItems.end(); ++it) {
        TreeItems::iterator found = _imp->findItem(it->get());
        if (found == _imp->items.end()) {
            _imp->selectedItems.erase(it);
            break;
        }
        found->treeItem->setSelected(true);
    }
    _imp->tree->blockSignals(false);

}

void RotoPanel::onSelectedBezierKeyframeSet(int time)
{
    Bezier* isBezier = qobject_cast<Bezier*>(sender());
    _imp->updateSplinesInfosGUI(time);
    if (isBezier) {
        _imp->setItemKey(isBezier, time);
    }
}

void RotoPanel::onSelectedBezierKeyframeRemoved(int time)
{
    Bezier* isBezier = qobject_cast<Bezier*>(sender());
    _imp->updateSplinesInfosGUI(time);
    if (isBezier) {
        _imp->removeItemKey(isBezier, time);
    }
}

void RotoPanel::onSelectedBezierAboutToClone()
{
    Bezier* isBezier = qobject_cast<Bezier*>(sender());
    if (isBezier) {
        ItemKeys::iterator it = _imp->keyframes.find(isBezier);
        if (it != _imp->keyframes.end()) {
            std::list<SequenceTime> markers;
            for (std::set<int>::iterator it2 = it->second.begin(); it2!=it->second.end(); ++it2) {
                markers.push_back(*it2);
            }
            _imp->node->getNode()->getApp()->getTimeLine()->removeMultipleKeyframeIndicator(markers);
        }
    }
}

void RotoPanel::onSelectedBezierCloned()
{
    Bezier* isBezier = qobject_cast<Bezier*>(sender());
    if (isBezier) {
        ItemKeys::iterator it = _imp->keyframes.find(isBezier);
        if (it != _imp->keyframes.end()) {
            std::set<int> keys;
            isBezier->getKeyframeTimes(&keys);
            std::list<SequenceTime> markers;
            for (std::set<int>::iterator it2 = keys.begin(); it2!=keys.end(); ++it2) {
                markers.push_back(*it2);
            }
            it->second = keys;
            _imp->node->getNode()->getApp()->getTimeLine()->addMultipleKeyframeIndicatorsAdded(markers);
        }

    }
}

static void makeSolidIcon(double *color,QIcon& icon)
{
    QPixmap p(15,15);
    QColor c;
    c.setRgbF(clamp<double>(color[0]), clamp<double>(color[1]), clamp<double>(color[2]));
    p.fill(c);
    icon.addPixmap(p);
}

void RotoPanel::updateItemGui(QTreeWidgetItem* item)
{
    int time = _imp->context->getTimelineCurrentTime();
    TreeItems::iterator it = _imp->findItem(item);
    assert(it!=_imp->items.end());
    it->treeItem->setIcon(COL_ACTIVATED,it->rotoItem->isGloballyActivated() ? _imp->iconVisible : _imp->iconUnvisible);
    it->treeItem->setIcon(COL_LOCKED,it->rotoItem->isLockedRecursive() ? _imp->iconLocked : _imp->iconUnlocked);
    
    RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>(it->rotoItem.get());
    if (drawable) {
        double overlayColor[4];
        drawable->getOverlayColor(overlayColor);
        QIcon overlayIcon;
        makeSolidIcon(overlayColor, overlayIcon);
        it->treeItem->setIcon(COL_OVERLAY, overlayIcon);
        
        double shapeColor[3];
        drawable->getColor(time, shapeColor);
        QIcon shapeColorIcon;
        makeSolidIcon(shapeColor, shapeColorIcon);
        it->treeItem->setIcon(COL_COLOR, shapeColorIcon);
        it->treeItem->setIcon(COL_INVERTED,drawable->getInverted(time) ? _imp->iconInverted : _imp->iconUninverted);
        QWidget* w = _imp->tree->itemWidget(it->treeItem,COL_OPERATOR);
        assert(w);
        ComboBox* cb = dynamic_cast<ComboBox*>(w);
        assert(cb);
        cb->setCurrentIndex_no_emit(drawable->getCompositingOperator(time));
    }
   

}

void RotoPanelPrivate::updateSplinesInfosGUI(int time)
{
    std::set<int> keyframes;
    for (SelectedItems::const_iterator it = selectedItems.begin(); it!=selectedItems.end(); ++it) {
        const Bezier* isBezier = dynamic_cast<const Bezier*>(it->get());
        if (isBezier) {
            isBezier->getKeyframeTimes(&keyframes);
        }
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
    
    ///Refresh the  inverted state
    for (TreeItems::iterator it = items.begin(); it != items.end(); ++it) {
        RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>(it->rotoItem.get());
        if (drawable) {
            QIcon shapeColorIC;
            double shapeColor[3];
            drawable->getColor(time, shapeColor);
            makeSolidIcon(shapeColor, shapeColorIC);
            it->treeItem->setIcon(COL_COLOR, shapeColorIC);
            it->treeItem->setIcon(COL_INVERTED,drawable->getInverted(time) ? iconInverted : iconUninverted);
            ComboBox* cb = dynamic_cast<ComboBox*>(tree->itemWidget(it->treeItem, COL_OPERATOR));
            assert(cb);
            cb->setCurrentIndex_no_emit(drawable->getCompositingOperator(time));
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

    treeItem->setText(COL_NAME, item->getName_mt_safe().c_str());
    treeItem->setIcon(COL_ACTIVATED, item->isGloballyActivated() ? iconVisible : iconUnvisible);
    treeItem->setIcon(COL_LOCKED, item->getLocked() ? iconLocked : iconUnlocked);
    
    RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>(item.get());
    RotoLayer* layer = dynamic_cast<RotoLayer*>(item.get());
    
    if (drawable) {
        double overlayColor[4];
        drawable->getOverlayColor(overlayColor);
        QIcon overlayIcon;
        makeSolidIcon(overlayColor, overlayIcon);
        treeItem->setIcon(COL_NAME, iconBezier);
        treeItem->setIcon(COL_OVERLAY,overlayIcon);
        double shapeColor[3];
        drawable->getColor(time, shapeColor);
        QIcon shapeIcon;
        makeSolidIcon(shapeColor, shapeIcon);
        treeItem->setIcon(COL_COLOR, shapeIcon);
        treeItem->setIcon(COL_INVERTED, drawable->getInverted(time)  ? iconInverted : iconUninverted);
        ComboBox* cb = new ComboBox;
        QObject::connect(cb,SIGNAL(currentIndexChanged(int)),publicInterface,SLOT(onCurrentItemCompOperatorChanged(int)));
        std::vector<std::string> compositingOperators,tooltips;
        getCompositingOperators(&compositingOperators, &tooltips);
        for (U32 i = 0; i < compositingOperators.size(); ++i) {
            cb->addItem(compositingOperators[i].c_str(),QIcon(),QKeySequence(),tooltips[i].c_str());
        }
        cb->setCurrentIndex_no_emit(drawable->getCompositingOperator(time));
        tree->setItemWidget(treeItem, COL_OPERATOR, cb);
        QObject::connect(drawable,SIGNAL(inversionChanged()), publicInterface, SLOT(onRotoItemInversionChanged()));
        QObject::connect(drawable,SIGNAL(shapeColorChanged()),publicInterface,SLOT(onRotoItemShapeColorChanged()));
        QObject::connect(drawable,SIGNAL(compositingOperatorChanged()),publicInterface,SLOT(onRotoItemCompOperatorChanged()));
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

        QObject::disconnect(drawable,SIGNAL(inversionChanged()), publicInterface, SLOT(onRotoItemInversionChanged()));
        
    }
    
    ///deleting the item will emit a selection change which would lead to a deadlock
    tree->blockSignals(true);
    delete it->treeItem;
    tree->blockSignals(false);
    items.erase(it);
}


void RotoPanel::onItemInserted(int reason)
{
    boost::shared_ptr<RotoItem> lastInsertedItem = _imp->context->getLastInsertedItem();

    Bezier* isBezier = dynamic_cast<Bezier*>(lastInsertedItem.get());
    if (isBezier) {
        ItemKeys::iterator it = _imp->keyframes.find(isBezier);
        if (it == _imp->keyframes.end()) {
            std::set<int> keys;
            isBezier->getKeyframeTimes(&keys);
            _imp->keyframes.insert(std::make_pair(isBezier, keys));
            std::list<SequenceTime> markers;
            for (std::set<int>::iterator it2 = keys.begin(); it2!=keys.end(); ++it2) {
                markers.push_back(*it2);
            }
            _imp->node->getNode()->getApp()->getTimeLine()->addMultipleKeyframeIndicatorsAdded(markers);
            
        }
    }
    if ((RotoContext::SelectionReason)reason == RotoContext::SETTINGS_PANEL) {
        return;
    }
    int time = _imp->context->getTimelineCurrentTime();
    assert(lastInsertedItem);
    _imp->insertItemRecursively(time, lastInsertedItem);
}

void RotoPanel::onItemRemoved(RotoItem* item,int reason)
{
    Bezier* isBezier = qobject_cast<Bezier*>(item);
    if (isBezier) {
        ItemKeys::iterator it = _imp->keyframes.find(isBezier);
        if (it != _imp->keyframes.end()) {
            for (std::set<int>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                _imp->node->getNode()->getApp()->getTimeLine()->removeKeyFrameIndicator(*it2);
            }
            _imp->keyframes.erase(it);
        }
    }
    if ((RotoContext::SelectionReason)reason == RotoContext::SETTINGS_PANEL) {
        return;
    }
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

void RotoPanel::onCurrentItemCompOperatorChanged(int index)
{
    
    QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
    for (int i = 0; i < selected.size(); ++i) {
        TreeItems::iterator found = _imp->findItem(selected[i]);
        assert(found != _imp->items.end());
        RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>(found->rotoItem.get());
        assert(drawable);
        boost::shared_ptr<Choice_Knob> op = drawable->getOperatorKnob();
        op->setValue(index, 0);
    }
    if (!selected.empty()) {
        _imp->context->getOperatorKnob()->setValue(index, 0);
    }
}

void RotoPanel::onRotoItemInversionChanged()
{
    RotoDrawableItem* item = qobject_cast<RotoDrawableItem*>(sender());
    if (item) {
        int time = _imp->context->getTimelineCurrentTime();
        TreeItems::iterator it = _imp->findItem(item);
        if (it != _imp->items.end()) {
            it->treeItem->setIcon(COL_INVERTED, item->getInverted(time)  ? _imp->iconInverted : _imp->iconUninverted);
        }
    }
}
void RotoPanel::onRotoItemShapeColorChanged()
{
    RotoDrawableItem* item = qobject_cast<RotoDrawableItem*>(sender());
    if (item) {
        int time = _imp->context->getTimelineCurrentTime();
        TreeItems::iterator it = _imp->findItem(item);
        if (it != _imp->items.end()) {
            QIcon icon;
            double shapeColor[3];
            item->getColor(time, shapeColor);
            makeSolidIcon(shapeColor, icon);
            it->treeItem->setIcon(COL_COLOR,icon);
        }
    }
}

void RotoPanel::onRotoItemCompOperatorChanged()
{
    RotoDrawableItem* item = qobject_cast<RotoDrawableItem*>(sender());
    if (item) {
        int time = _imp->context->getTimelineCurrentTime();
        TreeItems::iterator it = _imp->findItem(item);
        if (it != _imp->items.end()) {
            ComboBox* cb = dynamic_cast<ComboBox*>(_imp->tree->itemWidget(it->treeItem, COL_OPERATOR));
            assert(cb);
            int compIndex = item->getCompositingOperator(time);
            cb->setCurrentIndex_no_emit(compIndex);
        }
    }
}


void RotoPanel::onItemClicked(QTreeWidgetItem* item,int column)
{
    TreeItems::iterator it = _imp->findItem(item);
    if (it != _imp->items.end()) {
        int time = _imp->context->getTimelineCurrentTime();
        switch (column) {
                
            case COL_ACTIVATED:
            {
                bool activated = !it->rotoItem->isGloballyActivated();
                QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
                for (int i = 0; i < selected.size(); ++i) {
                    TreeItems::iterator found = _imp->findItem(selected[i]);
                    assert(found != _imp->items.end());
                    found->rotoItem->setGloballyActivated(activated, true);
                    _imp->setChildrenActivatedRecursively(activated, found->treeItem);
                }
                _imp->context->emitRefreshViewerOverlays();
            }   break;
               
            case COL_LOCKED:
            {
                bool locked = !it->rotoItem->getLocked();
                QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
                for (int i = 0; i < selected.size(); ++i) {
                    TreeItems::iterator found = _imp->findItem(selected[i]);
                    assert(found != _imp->items.end());
                    _imp->context->setLastItemLocked(found->rotoItem);
                    found->rotoItem->setLocked(locked,true);
                    _imp->setChildrenLockedRecursively(locked, found->treeItem);
                }
                
            }   break;
                
            case COL_OVERLAY:
            {
                RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>(it->rotoItem.get());
                if (drawable) {
                    QColorDialog dialog;
                    _imp->dialogEdition = EDITING_OVERLAY_COLOR;
                    QObject::connect(&dialog,SIGNAL(currentColorChanged(QColor)),this,SLOT(onItemColorDialogEdited(QColor)));
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
                    }
                    _imp->dialogEdition = EDITING_NOTHING;
                    QPixmap pix(15,15);
                    pix.fill(color);
                    QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
                    for (int i = 0; i < selected.size(); ++i) {
                        TreeItems::iterator found = _imp->findItem(selected[i]);
                        assert(found != _imp->items.end());
                        drawable = dynamic_cast<RotoDrawableItem*>(found->rotoItem.get());
                        if (drawable) {
                            drawable->setOverlayColor(oc);
                            found->treeItem->setIcon(COL_OVERLAY, QIcon(pix));
                        }
                    }
                }
            }   break;
                
            case COL_COLOR:
            {
                RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>(it->rotoItem.get());
                QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
                bool colorChosen = false;
                double shapeColor[3];
                if (drawable) {
                    QColorDialog dialog;
                    _imp->dialogEdition = EDITING_SHAPE_COLOR;
                    QObject::connect(&dialog,SIGNAL(currentColorChanged(QColor)),this,SLOT(onItemColorDialogEdited(QColor)));
                    drawable->getColor(time,shapeColor);
                    QColor color;
                    color.setRgbF(shapeColor[0], shapeColor[1], shapeColor[2]);
                    dialog.setCurrentColor(color);
                    if (dialog.exec()) {
                        color = dialog.selectedColor();
                        shapeColor[0] = color.redF();
                        shapeColor[1] = color.greenF();
                        shapeColor[2] = color.blueF();
                    }
                    _imp->dialogEdition = EDITING_NOTHING;
                    QIcon icon;
                    makeSolidIcon(shapeColor, icon);
                    colorChosen = true;
                    for (int i = 0; i < selected.size(); ++i) {
                        TreeItems::iterator found = _imp->findItem(selected[i]);
                        assert(found != _imp->items.end());
                        drawable = dynamic_cast<RotoDrawableItem*>(found->rotoItem.get());
                        if (drawable) {
                            boost::shared_ptr<Color_Knob> colorKnob = drawable->getColorKnob();
                            colorKnob->setValue(shapeColor[0], 0);
                            colorKnob->setValue(shapeColor[1], 1);
                            colorKnob->setValue(shapeColor[2], 2);
                            found->treeItem->setIcon(COL_COLOR, icon);
                        }
                    }
                }
                if (colorChosen && !selected.empty()) {
                    _imp->context->getColorKnob()->setValue(shapeColor[0], 0);
                    _imp->context->getColorKnob()->setValue(shapeColor[1], 1);
                    _imp->context->getColorKnob()->setValue(shapeColor[2], 2);
                }
            }   break;
               
            case COL_INVERTED:
            {
                QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
                bool inverted = false;
                bool invertedSet = false;
                for (int i = 0; i < selected.size(); ++i) {
                    TreeItems::iterator found = _imp->findItem(selected[i]);
                    assert(found != _imp->items.end());
                    RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>(found->rotoItem.get());
                    if (drawable) {
                        boost::shared_ptr<Bool_Knob> invertedKnob = drawable->getInvertedKnob();
                        bool isOnKeyframe = invertedKnob->getKeyFrameIndex(0, time) != -1;
                        inverted = !drawable->getInverted(time);
                        invertedSet = true;
                        if (_imp->context->isAutoKeyingEnabled() || isOnKeyframe) {
                            invertedKnob->setValueAtTime(time, inverted, 0);
                        } else {
                            invertedKnob->setValue(inverted, 0);
                        }
                        found->treeItem->setIcon(COL_INVERTED, inverted ? _imp->iconInverted : _imp->iconUninverted);
                    }
                }
                if (!selected.empty() && invertedSet) {
                    _imp->context->getInvertedKnob()->setValue(inverted, 0);
                
                }
                
                
            }   break;
            case 0:
            default:
                break;
        }
    }
}

void RotoPanel::onItemColorDialogEdited(const QColor& color)
{
    int time = _imp->context->getTimelineCurrentTime();
    QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
    for (int i = 0; i < selected.size(); ++i) {
        TreeItems::iterator found = _imp->findItem(selected[i]);
        assert(found != _imp->items.end());
        RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>(found->rotoItem.get());
        if (drawable) {
            
            if (_imp->dialogEdition == EDITING_SHAPE_COLOR) {
                boost::shared_ptr<Color_Knob> colorKnob = drawable->getColorKnob();
                colorKnob->setValue(color.redF(), 0);
                colorKnob->setValue(color.greenF(), 1);
                colorKnob->setValue(color.blueF(), 2);
                QIcon icon;
                double colorArray[3];
                colorArray[0] = color.redF();
                colorArray[1] = color.greenF();
                colorArray[2] = color.blueF();
                makeSolidIcon(colorArray, icon);
                found->treeItem->setIcon(COL_COLOR, icon);
                
                _imp->context->getColorKnob()->setValueAtTime(time, colorArray[0], 0);
                _imp->context->getColorKnob()->setValueAtTime(time, colorArray[1], 1);
                _imp->context->getColorKnob()->setValueAtTime(time, colorArray[2], 2);
                
            } else if (_imp->dialogEdition == EDITING_OVERLAY_COLOR) {
                double colorArray[4];
                colorArray[0] = color.redF();
                colorArray[1] = color.greenF();
                colorArray[2] = color.blueF();
                colorArray[3] = color.alphaF();
                drawable->setOverlayColor(colorArray);
                QIcon icon;
                makeSolidIcon(colorArray, icon);
                found->treeItem->setIcon(COL_OVERLAY,icon);

            }
        }
            
    }

}

void RotoPanelPrivate::setChildrenActivatedRecursively(bool activated,QTreeWidgetItem* item)
{
    item->setIcon(COL_ACTIVATED, activated ? iconVisible : iconUnvisible);
    for (int i = 0; i < item->childCount(); ++i) {
        setChildrenActivatedRecursively(activated,item->child(i));
    }
}

void RotoPanelPrivate::setChildrenLockedRecursively(bool locked,QTreeWidgetItem* item)
{
    item->setIcon(COL_LOCKED, locked ? iconLocked : iconUnlocked);
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
        std::string newName = item->text(column).toStdString();
        boost::shared_ptr<RotoItem> existingItem = _imp->context->getItemByName(newName);
        if (existingItem && existingItem != it->rotoItem) {
            Natron::warningDialog("", "An item with the name "+ newName + " already exists. Please pick something else.");
            item->setText(COL_NAME, _imp->editedItemName.c_str());
        } else {
            it->rotoItem->setName(newName);
        }
    }
}

void RotoPanel::onItemDoubleClicked(QTreeWidgetItem* item,int column)
{
    if (column != COL_NAME) {
        return;
    }
    TreeItems::iterator it = _imp->findItem(item);
    if (it != _imp->items.end()) {
        _imp->editedItem = item;
        QObject::connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)), this, SLOT(onFocusChanged(QWidget*,QWidget*)));
        _imp->editedItemName = it->rotoItem->getName_mt_safe();
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
        QWidget* w = _imp->tree->itemWidget(_imp->editedItem, COL_NAME);
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
        boost::shared_ptr<RotoLayer> l = boost::dynamic_pointer_cast<RotoLayer>(*it);
        SelectedItems::iterator found = std::find(selectedItems.begin(), selectedItems.end(), *it);
        if (found == selectedItems.end()) {
            context->select(*it, RotoContext::SETTINGS_PANEL);
            selectedItems.push_back(*it);
        }
        if (l) {
            insertSelectionRecursively(l);
        }
    }
}

void RotoPanel::onItemSelectionChanged()
{
    ///disconnect previous selection
    for (SelectedItems::const_iterator it = _imp->selectedItems.begin(); it!=_imp->selectedItems.end(); ++it) {
        const Bezier* isBezier = dynamic_cast<const Bezier*>(it->get());
        if (isBezier) {
            QObject::disconnect(isBezier, SIGNAL(keyframeSet(int)), this, SLOT(onSelectedBezierKeyframeSet(int)));
            QObject::disconnect(isBezier, SIGNAL(keyframeRemoved(int)), this, SLOT(onSelectedBezierKeyframeRemoved(int)));
        }
    }
    _imp->context->deselect(_imp->selectedItems, RotoContext::SETTINGS_PANEL);
    _imp->selectedItems.clear();
    
    QList<QTreeWidgetItem*> selectedItems = _imp->tree->selectedItems();
    
    int selectedBeziersCount = 0;
    for (int i = 0; i < selectedItems.size(); ++i) {
        TreeItems::iterator it = _imp->findItem(selectedItems[i]);
        assert(it != _imp->items.end());
        boost::shared_ptr<Bezier> bezier = boost::dynamic_pointer_cast<Bezier>(it->rotoItem);
        boost::shared_ptr<RotoLayer> layer = boost::dynamic_pointer_cast<RotoLayer>(it->rotoItem);
        if (bezier) {
            SelectedItems::iterator found = std::find(_imp->selectedItems.begin(), _imp->selectedItems.end(), bezier);
            if (found == _imp->selectedItems.end()) {
                _imp->selectedItems.push_back(bezier);
                ++selectedBeziersCount;
            }
        } else if (layer) {
            if (!layer->getItems().empty()) {
                ++selectedBeziersCount;
            }
            SelectedItems::iterator found = std::find(_imp->selectedItems.begin(), _imp->selectedItems.end(), it->rotoItem);
            if (found == _imp->selectedItems.end()) {
                _imp->selectedItems.push_back(it->rotoItem);
            }
            _imp->insertSelectionRecursively(layer);
        }
    }
    _imp->context->select(_imp->selectedItems, RotoContext::SETTINGS_PANEL);
    
    bool enabled = selectedBeziersCount > 0;
    
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

void RotoPanel::onAddLayerButtonClicked()
{
    pushUndoCommand(new AddLayerUndoCommand(this));
}

void RotoPanel::onRemoveItemButtonClicked()
{
    QList<QTreeWidgetItem*> selectedItems = _imp->tree->selectedItems();
    pushUndoCommand(new RemoveItemsUndoCommand(this,selectedItems));
  
}

static void isLayerAParent_recursive(RotoLayer* layer,RotoItem* item,bool* ret)
{
    RotoLayer* parent = item->getParentLayer();
    if (!parent) {
        return;
    } else {
        if (layer == parent) {
            *ret = true;
            return;
        } else {
            isLayerAParent_recursive(layer, parent, ret);
        }
    }
}

void TreeWidget::dragMoveEvent(QDragMoveEvent * event)
{
    const QMimeData* mime = event->mimeData();
    std::list<DroppedTreeItemPtr> droppedItems;
    bool ret = dragAndDropHandler(mime, event->pos(), droppedItems);
    
    QTreeWidget::dragMoveEvent(event);
    
    if (!ret) {
        event->setAccepted(ret);
    }
}

static void checkIfTreatedRecursive(QTreeWidgetItem* matcher, QTreeWidgetItem* item,bool *ret)
{
    if (item == matcher) {
        *ret = true;
    } else {
        if (item->parent()) {
            checkIfTreatedRecursive(matcher,item->parent(),ret);
        }
    }
}


bool TreeWidget::dragAndDropHandler(const QMimeData* mime,
                                    const QPoint& pos,
                                    std::list<DroppedTreeItemPtr>& dropped)
{
   
    
    if (mime->hasFormat("application/x-qabstractitemmodeldatalist")) {
        QByteArray encoded = mime->data("application/x-qabstractitemmodeldatalist");
        QDataStream stream(&encoded,QIODevice::ReadOnly);
        
        DropIndicatorPosition position = dropIndicatorPosition();
        
        ///list of items we already handled d&d for. If we find an item whose parent
        ///is already in this list we don't handle it
        std::list<QTreeWidgetItem*> treatedItems;
        while (!stream.atEnd())
        {
            
            int row, col;
            QMap<int,QVariant> roleDataMap;
            
            stream >> row >> col >> roleDataMap;
            
            QMap<int, QVariant>::Iterator it = roleDataMap.find(0);
            
            if (it != roleDataMap.end()) {
                DroppedTreeItemPtr ret(new DroppedTreeItem);

                ///The target item
                QTreeWidgetItem* into = itemAt(pos);
                
                if (!into) {
                    return false;
                }
                
                boost::shared_ptr<RotoItem> intoRotoItem = _panel->getRotoItemForTreeItem(into);
                QList<QTreeWidgetItem*> foundDropped = findItems(it.value().toString(),Qt::MatchExactly | Qt::MatchRecursive,0);
                assert(!foundDropped.empty());
                
                ///the dropped item
                ret->dropped = foundDropped[0];
                
                bool treated = false;
                for (std::list<QTreeWidgetItem*>::iterator treatedIt = treatedItems.begin(); treatedIt != treatedItems.end(); ++treatedIt) {
                    checkIfTreatedRecursive(*treatedIt,ret->dropped,&treated);
                    if (treated) {
                        break;
                    }
                }
                if (treated) {
                    continue;
                }
                
                
                ret->droppedRotoItem = _panel->getRotoItemForTreeItem(ret->dropped);
                assert(into && ret->dropped && intoRotoItem && ret->droppedRotoItem);
                
                ///Is the target item a layer ?
                RotoLayer* isIntoALayer = dynamic_cast<RotoLayer*>(intoRotoItem.get());
     
                ///Determine into which layer the item should be inserted.
                switch (position) {
                    case QAbstractItemView::AboveItem:
                    {
                        ret->newParentLayer = intoRotoItem->getParentLayer();
                        if (ret->newParentLayer) {
                            ret->newParentItem = into->parent();
                            ///find the target item index into its parent layer and insert the item above it
                            const std::list<boost::shared_ptr<RotoItem> >& children = intoRotoItem->getParentLayer()->getItems();
                            std::list<boost::shared_ptr<RotoItem> >::const_iterator found =
                            std::find(children.begin(),children.end(),intoRotoItem);
                            assert(found != children.end());
                            int index = std::distance(children.begin(), found);
                            
                            ///if the dropped item is already into the children and after the found index don't decrement
                            found = std::find(children.begin(),children.end(),ret->droppedRotoItem);
                            if (found != children.end() && std::distance(children.begin(), found) > index) {
                                ret->insertIndex = index;
                            } else {
                                ret->insertIndex = index == 0 ? 0 : index - 1;
                            }
                        } else {
                            return false;
                        }
                    }   break;
                    case QAbstractItemView::BelowItem:
                    {
                        RotoLayer* intoParentLayer = intoRotoItem->getParentLayer();
                        bool isTargetLayerAParent = false;
                        if (isIntoALayer) {
                            isLayerAParent_recursive(isIntoALayer, ret->droppedRotoItem.get(), &isTargetLayerAParent);
                        }
                        if (!intoParentLayer || isTargetLayerAParent) {
                            ///insert at the begining of the layer
                            ret->insertIndex = 0;
                            ret->newParentLayer = isIntoALayer;
                            ret->newParentItem = into;
                        } else {
                            ///find the target item index into its parent layer and insert the item after it
                            const std::list<boost::shared_ptr<RotoItem> >& children = intoParentLayer->getItems();
                            std::list<boost::shared_ptr<RotoItem> >::const_iterator found =
                            std::find(children.begin(),children.end(),intoRotoItem);
                            assert(found != children.end());
                            int index = std::distance(children.begin(), found);
                            
                            ///if the dropped item is already into the children and before the found index don't decrement
                            found = std::find(children.begin(),children.end(),ret->droppedRotoItem);
                            if (found != children.end() && std::distance(children.begin(), found) < index) {
                                ret->insertIndex = index;
                            } else {
                                ret->insertIndex = index + 1;
                            }
                            
                            ret->newParentLayer = intoParentLayer;
                            ret->newParentItem = into->parent();
                        }
                    }   break;
                    case QAbstractItemView::OnItem:
                    {
                        if (isIntoALayer) {
                            ///insert at the end of the layer
                            const std::list<boost::shared_ptr<RotoItem> >& children = isIntoALayer->getItems();
                            ///check if the item is already in that layer
                            std::list<boost::shared_ptr<RotoItem> >::const_iterator found =
                            std::find(children.begin(), children.end(), ret->droppedRotoItem);
                            if (found != children.end()) {
                                ret->insertIndex =  children.empty() ? 0 : (int)children.size() -1; // minus one because we're going to remove the item from it
                            } else {
                                ret->insertIndex = (int)children.size();
                            }
                            
                            ret->newParentLayer = isIntoALayer;
                            ret->newParentItem = into;
                        } else {
                            ///dropping an item on another item which is not a layer is not accepted
                            return false;
                        }
                    }   break;
                    case QAbstractItemView::OnViewport:
                    default:
                        //do nothing we don't accept it
                        return false;
                } // switch
                dropped.push_back(ret);
                treatedItems.push_back(ret->dropped);
            } //  if (it != roleDataMap.end())

        } // while (!stream.atEnd())
    } //if (mime->hasFormat("application/x-qabstractitemmodeldatalist"))
    return true;
}

void TreeWidget::dropEvent(QDropEvent* event)
{
   
    std::list<DroppedTreeItemPtr> droppedItems;
    const QMimeData* mime = event->mimeData();
    bool accepted = dragAndDropHandler(mime,event->pos(), droppedItems);
    event->setAccepted(accepted);
    
    if (accepted) {
        _panel->pushUndoCommand(new DragItemsUndoCommand(_panel,droppedItems));
    }
    
}

void TreeWidget::keyPressEvent(QKeyEvent* event)
{
    
    QList<QTreeWidgetItem*> selected = selectedItems();
    QTreeWidgetItem* item = selected.empty() ? 0 :  selected.front();
    
    if (item) {
        _panel->setLastRightClickedItem(item);
        if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
            _panel->onRemoveItemButtonClicked();
        } else if (event->key() == Qt::Key_C && event->modifiers().testFlag(Qt::ControlModifier)) {
            _panel->onCopyItemActionTriggered();
        } else if (event->key() == Qt::Key_V && event->modifiers().testFlag(Qt::ControlModifier)) {
            _panel->onPasteItemActionTriggered();
        } else if (event->key() == Qt::Key_X && event->modifiers().testFlag(Qt::ControlModifier)) {
            _panel->onCutItemActionTriggered();
        } else if (event->key() == Qt::Key_C && event->modifiers().testFlag(Qt::AltModifier)) {
            _panel->onDuplicateItemActionTriggered();
        } else if (event->key() == Qt::Key_A && event->modifiers().testFlag(Qt::ControlModifier)) {
            _panel->selectAll();
        } else {
            QTreeWidget::keyPressEvent(event);
        }
    } else {
        QTreeWidget::keyPressEvent(event);
    }
}

void RotoPanel::pushUndoCommand(QUndoCommand* cmd)
{
    NodeSettingsPanel* panel = _imp->node->getSettingPanel();
    assert(panel);
    panel->pushUndoCommand(cmd);
}

std::string RotoPanel::getNodeName() const
{
    return _imp->node->getNode()->getName();
}

RotoContext* RotoPanel::getContext() const
{
    return _imp->context;
}

void RotoPanel::clearSelection()
{
    _imp->selectedItems.clear();
    _imp->context->clearSelection(RotoContext::SETTINGS_PANEL);
}

void RotoPanel::showItemMenu(QTreeWidgetItem* item,const QPoint& globalPos)
{
    TreeItems::iterator it = _imp->findItem(item);
    if (it == _imp->items.end()) {
        return;
    }
    
    
    _imp->lastRightClickedItem = item;
    
    QMenu menu(this);
    menu.setShortcutEnabled(false);
    QAction* addLayerAct = menu.addAction(tr("Add layer"));
    QObject::connect(addLayerAct, SIGNAL(triggered()), this, SLOT(onAddLayerActionTriggered()));
    QAction* deleteAct = menu.addAction(tr("Delete"));
    deleteAct->setShortcut(QKeySequence(Qt::Key_Backspace));
    QObject::connect(deleteAct, SIGNAL(triggered()), this, SLOT(onDeleteItemActionTriggered()));
    QAction* cutAct = menu.addAction(tr("Cut"));
    cutAct->setShortcut(QKeySequence(Qt::Key_X + Qt::CTRL));
    QObject::connect(cutAct, SIGNAL(triggered()), this, SLOT(onCutItemActionTriggered()));
    QAction* copyAct = menu.addAction(tr("Copy"));
    copyAct->setShortcut(QKeySequence(Qt::Key_C + Qt::CTRL));
    QObject::connect(copyAct, SIGNAL(triggered()), this, SLOT(onCopyItemActionTriggered()));
    QAction* pasteAct = menu.addAction(tr("Paste"));
    pasteAct->setShortcut(QKeySequence(Qt::Key_V + Qt::CTRL));
    QObject::connect(pasteAct, SIGNAL(triggered()), this, SLOT(onPasteItemActionTriggered()));
    pasteAct->setEnabled(!_imp->clipBoard.empty());
    QAction* duplicateAct = menu.addAction(tr("Duplicate"));
    duplicateAct->setShortcut(QKeySequence(Qt::Key_C + Qt::ALT));
    QObject::connect(duplicateAct, SIGNAL(triggered()), this, SLOT(onDuplicateItemActionTriggered()));
    
    ///The base layer cannot be duplicated
    duplicateAct->setEnabled(it->rotoItem->getParentLayer() != NULL);
    
    menu.exec(globalPos);
}

void RotoPanel::onAddLayerActionTriggered()
{
    assert(_imp->lastRightClickedItem);
    pushUndoCommand(new AddLayerUndoCommand(this));
}

void RotoPanel::onDeleteItemActionTriggered()
{
    assert(_imp->lastRightClickedItem);
    QList<QTreeWidgetItem*> selectedItems = _imp->tree->selectedItems();
    pushUndoCommand(new RemoveItemsUndoCommand(this,selectedItems));

}

void RotoPanel::onCutItemActionTriggered()
{
    assert(_imp->lastRightClickedItem);
    QList<QTreeWidgetItem*> selectedItems = _imp->tree->selectedItems();
    _imp->clipBoard = selectedItems;
    pushUndoCommand(new RemoveItemsUndoCommand(this,selectedItems));

}

void RotoPanel::onCopyItemActionTriggered()
{
    assert(_imp->lastRightClickedItem);
    _imp->clipBoard = _imp->tree->selectedItems();
}

void RotoPanel::onPasteItemActionTriggered()
{
    assert(!_imp->clipBoard.empty());
    
    TreeItems::iterator it = _imp->findItem(_imp->lastRightClickedItem);
    if (it == _imp->items.end()) {
        return;
    }
    
    
    RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>(_imp->lastRightClickedItem);
    if (drawable) {
        
        ///cannot paste multiple items on a drawable item
        if (_imp->clipBoard.size() > 1) {
            return;
        }
        
        ///cannot paste a non-drawable to a drawable
        TreeItems::iterator clip = _imp->findItem(_imp->clipBoard.front());
        RotoDrawableItem* isClipBoardDrawable = dynamic_cast<RotoDrawableItem*>(clip->rotoItem.get());
        if (!isClipBoardDrawable) {
            return;
        }
    }
    
    pushUndoCommand(new PasteItemUndoCommand(this,_imp->lastRightClickedItem,_imp->clipBoard));
}

void RotoPanel::onDuplicateItemActionTriggered()
{
    pushUndoCommand(new DuplicateItemUndoCommand(this,_imp->lastRightClickedItem));
}

void RotoPanel::setLastRightClickedItem(QTreeWidgetItem* item)
{
    assert(item);
    _imp->lastRightClickedItem = item;
}

void RotoPanel::selectAll()
{
    _imp->tree->blockSignals(true);
    for (TreeItems::iterator it = _imp->items.begin(); it!=_imp->items.end(); ++it) {
        it->treeItem->setSelected(true);
    }
    _imp->tree->blockSignals(false);
    onItemSelectionChanged();
}

bool RotoPanelPrivate::itemHasKey(RotoItem* item,int time) const
{
    ItemKeys::const_iterator it = keyframes.find(item);
    if (it != keyframes.end()) {
        std::set<int>::const_iterator it2 = it->second.find(time);
        if (it2 != it->second.end()) {
            return true;
        }
    }
    return false;
}

void RotoPanelPrivate::setItemKey(RotoItem* item,int time)
{
    ItemKeys::iterator it = keyframes.find(item);
    if (it != keyframes.end()) {
        std::pair<std::set<int>::iterator,bool> ret = it->second.insert(time);
        if (ret.second) {
            node->getNode()->getApp()->getTimeLine()->addKeyframeIndicator(time);
        }
    } else {
        std::set<int> keys;
        keys.insert(time);
        keyframes.insert(std::make_pair(item, keys));
    }
}

void RotoPanelPrivate::removeItemKey(RotoItem* item,int time)
{
    ItemKeys::iterator it = keyframes.find(item);
    if (it != keyframes.end()) {
        std::set<int>::iterator it2 = it->second.find(time);
        if (it2 != it->second.end()) {
            it->second.erase(it2);
            node->getNode()->getApp()->getTimeLine()->removeKeyFrameIndicator(time);
        }
    }
}

void RotoPanel::onSettingsPanelClosed(bool closed) {
    if (closed) {
        ///remove all keyframes from the structure kept
        for (TreeItems::iterator it = _imp->items.begin(); it!=_imp->items.end(); ++it) {
            Bezier* isBezier = dynamic_cast<Bezier*>(it->rotoItem.get());
            if (isBezier) {
                ItemKeys::iterator it2 = _imp->keyframes.find(isBezier);
                if (it2 != _imp->keyframes.end()) {
                    std::list<SequenceTime> markers;
                    for (std::set<int>::iterator it3 = it2->second.begin(); it3 != it2->second.end(); ++it3) {
                        markers.push_back(*it3);
                    }
                    _imp->node->getNode()->getApp()->getTimeLine()->removeMultipleKeyframeIndicator(markers);
                    _imp->keyframes.erase(it2);
                }
            }
        }
    } else {
        ///rebuild all the keyframe structure
        for (TreeItems::iterator it = _imp->items.begin(); it!=_imp->items.end(); ++it) {
            Bezier* isBezier = dynamic_cast<Bezier*>(it->rotoItem.get());
            if (isBezier) {
                ItemKeys::iterator it2 = _imp->keyframes.find(isBezier);
                assert (it2 == _imp->keyframes.end());
                std::set<int> keys;
                isBezier->getKeyframeTimes(&keys);
                std::list<SequenceTime> markers;
                for (std::set<int>::iterator it3 = keys.begin(); it3 != keys.end(); ++it3) {
                    markers.push_back(*it3);
                }
                _imp->keyframes.insert(std::make_pair(isBezier, keys));
                _imp->node->getNode()->getApp()->getTimeLine()->addMultipleKeyframeIndicatorsAdded(markers);
                
            }
        }
    }
}
