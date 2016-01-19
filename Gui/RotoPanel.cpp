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

#include "RotoPanel.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QPixmap>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QColor>
#include <QColorDialog>
#include <QCursor>
#include <QMouseEvent>
#include <QApplication>
#include <QDataStream>
#include <QMimeData>
#include <QImage>
#include <QPainter>
#include <QByteArray>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Bezier.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/RotoContextPrivate.h" // for getCompositingOperators
#include "Engine/RotoLayer.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/TimeLine.h"

#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/DockablePanel.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/Menu.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/RotoUndoCommand.h"
#include "Gui/SpinBox.h"
#include "Gui/Utils.h"

#define COL_LABEL 0
#define COL_SCRIPT_NAME 1
#define COL_ACTIVATED 2
#define COL_LOCKED 3
#define COL_OVERLAY 5
#define COL_COLOR 6
#define COL_OPERATOR 4

#ifdef NATRON_ROTO_INVERTIBLE
#define COL_INVERTED 7
#define MAX_COLS 8
#else
#define MAX_COLS 7
#endif

NATRON_NAMESPACE_USING


class TreeWidget
    : public QTreeWidget
{
    RotoPanel* _panel;

public:

    TreeWidget(RotoPanel* panel,
               QWidget* parent)
        : QTreeWidget(parent), _panel(panel)
    {
    }

    virtual ~TreeWidget()
    {
    }

private:

    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL
    {
        QModelIndex index = indexAt( e->pos() );
        QTreeWidgetItem* item = itemAt( e->pos() );

        QList<QTreeWidgetItem*> selection = selectedItems();

        if ( index.isValid() && (index.column() != 0) && selection.contains(item) ) {
            Q_EMIT itemClicked( item, index.column() );
        } else if ( triggerButtonIsRight(e) && index.isValid() ) {
            _panel->showItemMenu( item,e->globalPos() );
        } else {
            QTreeWidget::mouseReleaseEvent(e);
        }
    }

    virtual void dragMoveEvent(QDragMoveEvent* e) OVERRIDE FINAL;
    virtual void dropEvent(QDropEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;

    bool dragAndDropHandler(const QMimeData* mime,
                            const QPoint & pos,
                            std::list<DroppedTreeItemPtr> & dropped);
};

struct TreeItem
{
    QTreeWidgetItem* treeItem;
    boost::shared_ptr<RotoItem> rotoItem;

    TreeItem()
        : treeItem(0), rotoItem()
    {
    }

    TreeItem(QTreeWidgetItem* t,
             const boost::shared_ptr<RotoItem> & r)
        : treeItem(t), rotoItem(r)
    {
    }
};

typedef std::list< TreeItem > TreeItems;
typedef std::list< boost::shared_ptr<RotoItem> > SelectedItems;

struct TimeLineKeys
{
    std::set<double> keys;
    bool visible;
};

typedef std::map<boost::shared_ptr<RotoItem>, TimeLineKeys > ItemKeys;

enum ColorDialogEditingEnum
{
    eColorDialogEditingNothing = 0,
    eColorDialogEditingOverlayColor,
    eColorDialogEditingShapeColor
};

struct NATRON_NAMESPACE::RotoPanelPrivate
{
    RotoPanel* publicInterface;
    boost::weak_ptr<NodeGui> node;
    boost::shared_ptr<RotoContext> context;
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
    Button* clearAnimation;
    QWidget* buttonContainer;
    QHBoxLayout* buttonLayout;
    Button* addLayerButton;
    Button* removeItemButton;
    QIcon iconLayer,iconBezier,iconVisible,iconUnvisible,iconLocked,iconUnlocked,iconInverted,iconUninverted,iconWheel;
    QIcon iconStroke,iconEraser,iconSmear,iconSharpen,iconBlur,iconClone,iconReveal,iconDodge,iconBurn;
    TreeWidget* tree;
    QTreeWidgetItem* treeHeader;
    SelectedItems selectedItems;
    TreeItems items;
    QTreeWidgetItem* editedItem;
    std::string editedItemName;
    QTreeWidgetItem* lastRightClickedItem;
    QList<QTreeWidgetItem*> clipBoard;

    ItemKeys keyframes; //< track of all keyframes for items
    ColorDialogEditingEnum dialogEdition;
    
    bool settingNameFromGui;

    RotoPanelPrivate(RotoPanel* publicInter,
                     const boost::shared_ptr<NodeGui>&   n)
    : publicInterface(publicInter)
    , node(n)
    , context( n->getNode()->getRotoContext())
    , mainLayout(0)
    , splineContainer(0)
    , splineLayout(0)
    , splineLabel(0)
    , currentKeyframe(0)
    , ofLabel(0)
    , totalKeyframes(0)
    , prevKeyframe(0)
    , nextKeyframe(0)
    , addKeyframe(0)
    , removeKeyframe(0)
    , clearAnimation(0)
    , buttonContainer(0)
    , buttonLayout(0)
    , addLayerButton(0)
    , removeItemButton(0)
    , iconLayer()
    , iconBezier()
    , iconVisible()
    , iconUnvisible()
    , iconLocked()
    , iconUnlocked()
    , iconInverted()
    , iconUninverted()
    , iconWheel()
    , iconStroke()
    , iconEraser()
    , iconSmear()
    , iconSharpen()
    , iconBlur()
    , iconClone()
    , iconReveal()
    , iconDodge()
    , iconBurn()
    , tree(0)
    , treeHeader(0)
    , selectedItems()
    , items()
    , editedItem(NULL)
    , editedItemName()
    , lastRightClickedItem(NULL)
    , clipBoard()
    , keyframes()
    , dialogEdition(eColorDialogEditingNothing)
    , settingNameFromGui(false)
    {
        assert(n && context);
    }
    
    void updateSplinesInfoGUI(double time);
    
    void buildTreeFromContext();
    
    TreeItems::iterator findItem(const boost::shared_ptr<RotoItem>& item)
    {
        for (TreeItems::iterator it = items.begin(); it != items.end(); ++it) {
            if (it->rotoItem == item) {
                return it;
            }
        }

        return items.end();
    }

    TreeItems::iterator findItem(QTreeWidgetItem* item)
    {
        for (TreeItems::iterator it = items.begin(); it != items.end(); ++it) {
            if (it->treeItem == item) {
                return it;
            }
        }

        return items.end();
    }

    void insertItemRecursively(double time, const boost::shared_ptr<RotoItem>& item,int indexInParentLayer);

    void removeItemRecursively(const boost::shared_ptr<RotoItem>& item);

    void insertSelectionRecursively(const boost::shared_ptr<RotoLayer> & layer);

    void setChildrenActivatedRecursively(bool activated, QTreeWidgetItem* item);

    void setChildrenLockedRecursively(bool locked, QTreeWidgetItem* item);

    bool itemHasKey(const boost::shared_ptr<RotoItem>& item, double time) const;

    void setItemKey(const boost::shared_ptr<RotoItem>& item, double time);

    void removeItemKey(const boost::shared_ptr<RotoItem>& item, double time);
    
    void removeItemAnimation(const boost::shared_ptr<RotoItem>& item);

    void insertItemInternal(int reason, double time, const boost::shared_ptr<RotoItem>& item, int indexInParentLayer);
    
    void setVisibleItemKeyframes(const std::set<double>& keys,bool visible, bool emitSignal);
};

RotoPanel::RotoPanel(const boost::shared_ptr<NodeGui>&  n,
                     QWidget* parent)
    : QWidget(parent)
      , _imp( new RotoPanelPrivate(this,n) )
{
    QObject::connect( _imp->context.get(), SIGNAL( selectionChanged(int) ), this, SLOT( onSelectionChanged(int) ) );
    QObject::connect( _imp->context.get(),SIGNAL( itemInserted(int,int) ),this,SLOT( onItemInserted(int,int) ) );
    QObject::connect( _imp->context.get(),SIGNAL( itemRemoved(const boost::shared_ptr<RotoItem>&,int) ),this,SLOT( onItemRemoved(const boost::shared_ptr<RotoItem>&,int) ) );
    QObject::connect( n->getNode()->getApp()->getTimeLine().get(), SIGNAL( frameChanged(SequenceTime,int) ), this,
                      SLOT( onTimeChanged(SequenceTime, int) ) );
    QObject::connect( n.get(), SIGNAL( settingsPanelClosed(bool) ), this, SLOT( onSettingsPanelClosed(bool) ) );

    QObject::connect( _imp->context.get(),SIGNAL( itemScriptNameChanged(boost::shared_ptr<RotoItem>)),this,SLOT( onItemScriptNameChanged(boost::shared_ptr<RotoItem>) ) );
    QObject::connect( _imp->context.get(),SIGNAL( itemLabelChanged(boost::shared_ptr<RotoItem>)),this,SLOT( onItemLabelChanged(boost::shared_ptr<RotoItem>) ) );
    QObject::connect( _imp->context.get(), SIGNAL(itemLockedChanged(int)), this, SLOT(onItemLockChanged(int)));
    
    _imp->mainLayout = new QVBoxLayout(this);

    _imp->splineContainer = new QWidget(this);
    _imp->mainLayout->addWidget(_imp->splineContainer);

    _imp->splineLayout = new QHBoxLayout(_imp->splineContainer);
    _imp->splineLayout->setSpacing(2);
    _imp->splineLabel = new ClickableLabel(tr("Spline keyframe:"),_imp->splineContainer);
    _imp->splineLabel->setSunken(false);
    _imp->splineLabel->setEnabled(false);
    _imp->splineLayout->addWidget(_imp->splineLabel);

    _imp->currentKeyframe = new SpinBox(_imp->splineContainer,SpinBox::eSpinBoxTypeDouble);
    _imp->currentKeyframe->setEnabled(false);
    _imp->currentKeyframe->setReadOnly(true);
    _imp->currentKeyframe->setToolTip(Natron::convertFromPlainText(tr("The current keyframe for the selected shape(s)."), Qt::WhiteSpaceNormal));
    _imp->splineLayout->addWidget(_imp->currentKeyframe);

    _imp->ofLabel = new ClickableLabel("of",_imp->splineContainer);
    _imp->ofLabel->setEnabled(false);
    _imp->splineLayout->addWidget(_imp->ofLabel);

    _imp->totalKeyframes = new SpinBox(_imp->splineContainer,SpinBox::eSpinBoxTypeInt);
    _imp->totalKeyframes->setEnabled(false);
    _imp->totalKeyframes->setReadOnly(true);
    _imp->totalKeyframes->setToolTip(Natron::convertFromPlainText(tr("The keyframe count for all the selected shapes."), Qt::WhiteSpaceNormal));
    _imp->splineLayout->addWidget(_imp->totalKeyframes);

    QPixmap prevPix,nextPix,addPix,removePix,clearAnimPix;
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_PREVIOUS_KEY, NATRON_MEDIUM_BUTTON_ICON_SIZE, &prevPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_NEXT_KEY, NATRON_MEDIUM_BUTTON_ICON_SIZE, &nextPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ADD_KEYFRAME, NATRON_MEDIUM_BUTTON_ICON_SIZE, &addPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_REMOVE_KEYFRAME, NATRON_MEDIUM_BUTTON_ICON_SIZE, &removePix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_CLEAR_ALL_ANIMATION, NATRON_MEDIUM_BUTTON_ICON_SIZE, &clearAnimPix);

    _imp->prevKeyframe = new Button(QIcon(prevPix),"",_imp->splineContainer);
    _imp->prevKeyframe->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->prevKeyframe->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->prevKeyframe->setToolTip(Natron::convertFromPlainText(tr("Go to the previous keyframe."), Qt::WhiteSpaceNormal));
    _imp->prevKeyframe->setEnabled(false);
    QObject::connect( _imp->prevKeyframe, SIGNAL( clicked(bool) ), this, SLOT( onGoToPrevKeyframeButtonClicked() ) );
    _imp->splineLayout->addWidget(_imp->prevKeyframe);

    _imp->nextKeyframe = new Button(QIcon(nextPix),"",_imp->splineContainer);
    _imp->nextKeyframe->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->nextKeyframe->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->nextKeyframe->setToolTip(Natron::convertFromPlainText(tr("Go to the next keyframe."), Qt::WhiteSpaceNormal));
    _imp->nextKeyframe->setEnabled(false);
    QObject::connect( _imp->nextKeyframe, SIGNAL( clicked(bool) ), this, SLOT( onGoToNextKeyframeButtonClicked() ) );
    _imp->splineLayout->addWidget(_imp->nextKeyframe);

    _imp->addKeyframe = new Button(QIcon(addPix),"",_imp->splineContainer);
    _imp->addKeyframe->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE);
    _imp->addKeyframe->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->addKeyframe->setToolTip(Natron::convertFromPlainText(tr("Add keyframe at the current timeline's time."), Qt::WhiteSpaceNormal));
    _imp->addKeyframe->setEnabled(false);
    QObject::connect( _imp->addKeyframe, SIGNAL( clicked(bool) ), this, SLOT( onAddKeyframeButtonClicked() ) );
    _imp->splineLayout->addWidget(_imp->addKeyframe);

    _imp->removeKeyframe = new Button(QIcon(removePix),"",_imp->splineContainer);
    _imp->removeKeyframe->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE);
    _imp->removeKeyframe->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->removeKeyframe->setToolTip(Natron::convertFromPlainText(tr("Remove keyframe at the current timeline's time."), Qt::WhiteSpaceNormal));
    _imp->removeKeyframe->setEnabled(false);
    QObject::connect( _imp->removeKeyframe, SIGNAL( clicked(bool) ), this, SLOT( onRemoveKeyframeButtonClicked() ) );
    _imp->splineLayout->addWidget(_imp->removeKeyframe);
    
    _imp->clearAnimation = new Button(QIcon(clearAnimPix),"",_imp->splineContainer);
    _imp->clearAnimation->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE);
    _imp->clearAnimation->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->clearAnimation->setToolTip(Natron::convertFromPlainText(tr("Remove all animation for the selected shape(s)."), Qt::WhiteSpaceNormal));
    _imp->clearAnimation->setEnabled(false);
    QObject::connect( _imp->clearAnimation, SIGNAL( clicked(bool) ), this, SLOT( onRemoveAnimationButtonClicked() ) );
    _imp->splineLayout->addWidget(_imp->clearAnimation);

    
    _imp->splineLayout->addStretch();


    _imp->tree = new TreeWidget(this,this);
    _imp->tree->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    _imp->tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    _imp->tree->setDragDropMode(QAbstractItemView::InternalMove);
    _imp->tree->setDragEnabled(true);
    _imp->tree->setExpandsOnDoubleClick(false);
    _imp->tree->setAttribute(Qt::WA_MacShowFocusRect,1);
    QString treeToolTip = Natron::convertFromPlainText(tr("This tree contains the hierarchy of shapes, strokes and layers along with some "
                                                      "most commonly used attributes for each of them. "
                                                      "Each attribute can be found in the parameters above in the panel.\n"
                                                      "You can reorder items by drag and dropping them and can also right click "
                                                      "each item for more options.\n"
                                                      "The items are rendered from bottom to top always, so that the first shape in "
                                                      "this list will always be the last one rendered "
                                                      "(generally on top of everything else)."), Qt::WhiteSpaceNormal);
    _imp->tree->setToolTip(treeToolTip);

    _imp->mainLayout->addWidget(_imp->tree);

    QObject::connect( _imp->tree, SIGNAL( itemClicked(QTreeWidgetItem*,int) ), this, SLOT( onItemClicked(QTreeWidgetItem*, int) ) );
    QObject::connect( _imp->tree, SIGNAL( itemDoubleClicked(QTreeWidgetItem*,int) ), this, SLOT( onItemDoubleClicked(QTreeWidgetItem*, int) ) );
    QObject::connect( _imp->tree, SIGNAL( itemChanged(QTreeWidgetItem*,int) ), this, SLOT( onItemChanged(QTreeWidgetItem*, int) ) );
    QObject::connect( _imp->tree, SIGNAL( currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*) ), this,
                      SLOT( onCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*) ) );
    QObject::connect( _imp->tree, SIGNAL( itemSelectionChanged() ), this, SLOT( onItemSelectionChanged() ) );


    _imp->tree->setColumnCount(MAX_COLS);
    _imp->treeHeader = new QTreeWidgetItem;
    _imp->treeHeader->setText( COL_LABEL, tr("Label") );
    _imp->treeHeader->setText(COL_SCRIPT_NAME, tr("Script"));

    QPixmap pixLayer,pixBezier,pixVisible,pixUnvisible,pixLocked,pixUnlocked,pixInverted,pixUninverted,pixWheel,pixDefault,pixmerge;
    QPixmap pixPaintBrush,pixEraser,pixBlur,pixSmear,pixSharpen,pixDodge,pixBurn,pixClone,pixReveal;
    appPTR->getIcon(NATRON_PIXMAP_LAYER, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixLayer);
    appPTR->getIcon(NATRON_PIXMAP_BEZIER, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixBezier);
    appPTR->getIcon(NATRON_PIXMAP_VISIBLE, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixVisible);
    appPTR->getIcon(NATRON_PIXMAP_UNVISIBLE, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixUnvisible);
    appPTR->getIcon(NATRON_PIXMAP_LOCKED, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixLocked);
    appPTR->getIcon(NATRON_PIXMAP_UNLOCKED, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixUnlocked);
    appPTR->getIcon(NATRON_PIXMAP_INVERTED, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixInverted);
    appPTR->getIcon(NATRON_PIXMAP_UNINVERTED, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixUninverted);
    appPTR->getIcon(NATRON_PIXMAP_COLORWHEEL, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixWheel);
    appPTR->getIcon(NATRON_PIXMAP_ROTO_MERGE, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixmerge);
    appPTR->getIcon(NATRON_PIXMAP_OVERLAY, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixDefault);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_SOLID, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixPaintBrush);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_ERASER, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixEraser);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_BLUR, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixBlur);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_SMEAR, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixSmear);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_SHARPEN, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixSharpen);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_DODGE, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixDodge);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_BURN, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixBurn);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_CLONE, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixClone);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_REVEAL, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixReveal);
    
    _imp->iconLayer.addPixmap(pixLayer);
    _imp->iconBezier.addPixmap(pixBezier);
    _imp->iconInverted.addPixmap(pixInverted);
    _imp->iconUninverted.addPixmap(pixUninverted);
    _imp->iconVisible.addPixmap(pixVisible);
    _imp->iconUnvisible.addPixmap(pixUnvisible);
    _imp->iconLocked.addPixmap(pixLocked);
    _imp->iconUnlocked.addPixmap(pixUnlocked);
    _imp->iconWheel.addPixmap(pixWheel);
    
    _imp->iconStroke.addPixmap(pixPaintBrush);
    _imp->iconEraser.addPixmap(pixEraser);
    _imp->iconBlur.addPixmap(pixBlur);
    _imp->iconSmear.addPixmap(pixSmear);
    _imp->iconSharpen.addPixmap(pixSharpen);
    _imp->iconDodge.addPixmap(pixDodge);
    _imp->iconBurn.addPixmap(pixBurn);
    _imp->iconClone.addPixmap(pixClone);
    _imp->iconReveal.addPixmap(pixReveal);

    _imp->treeHeader->setIcon(COL_ACTIVATED, _imp->iconVisible);
    _imp->treeHeader->setIcon(COL_LOCKED, _imp->iconLocked);
    _imp->treeHeader->setIcon(COL_OVERLAY, QIcon(pixDefault) );
    _imp->treeHeader->setIcon(COL_COLOR, _imp->iconWheel);
#ifdef NATRON_ROTO_INVERTIBLE
    _imp->treeHeader->setIcon(COL_INVERTED, _imp->iconUninverted);
#endif
    _imp->treeHeader->setIcon( COL_OPERATOR, QIcon(pixmerge) );
    _imp->tree->setHeaderItem(_imp->treeHeader);

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
    _imp->addLayerButton->setToolTip(Natron::convertFromPlainText(tr("Add a new layer."), Qt::WhiteSpaceNormal));
    _imp->buttonLayout->addWidget(_imp->addLayerButton);
    QObject::connect( _imp->addLayerButton, SIGNAL( clicked(bool) ), this, SLOT( onAddLayerButtonClicked() ) );

    _imp->removeItemButton = new Button("-",_imp->buttonContainer);
    _imp->removeItemButton->setToolTip(Natron::convertFromPlainText(tr("Remove selected items."), Qt::WhiteSpaceNormal));
    _imp->buttonLayout->addWidget(_imp->removeItemButton);
    QObject::connect( _imp->removeItemButton, SIGNAL( clicked(bool) ), this, SLOT( onRemoveItemButtonClicked() ) );

    _imp->buttonLayout->addStretch();

    _imp->mainLayout->addWidget(_imp->buttonContainer);

    _imp->buildTreeFromContext();

    ///refresh selection
    onSelectionChanged(RotoItem::eSelectionReasonOther);
}

RotoPanel::~RotoPanel()
{
}

boost::shared_ptr<RotoItem> RotoPanel::getRotoItemForTreeItem(QTreeWidgetItem* treeItem) const
{
    TreeItems::iterator it = _imp->findItem(treeItem);

    if ( it != _imp->items.end() ) {
        return it->rotoItem;
    }

    return boost::shared_ptr<RotoItem>();
}

QTreeWidgetItem*
RotoPanel::getTreeItemForRotoItem(const boost::shared_ptr<RotoItem> & item) const
{
    TreeItems::iterator it = _imp->findItem(item);

    if ( it != _imp->items.end() ) {
        return it->treeItem;
    }

    return NULL;
}

void
RotoPanel::onGoToPrevKeyframeButtonClicked()
{
    _imp->context->goToPreviousKeyframe();
}

void
RotoPanel::onGoToNextKeyframeButtonClicked()
{
    _imp->context->goToNextKeyframe();
}

void
RotoPanel::onAddKeyframeButtonClicked()
{
    _imp->context->setKeyframeOnSelectedCurves();
}

void
RotoPanel::onRemoveAnimationButtonClicked()
{
    _imp->context->removeAnimationOnSelectedCurves();
}

void
RotoPanel::onRemoveKeyframeButtonClicked()
{
    _imp->context->removeKeyframeOnSelectedCurves();
}

void
RotoPanel::onSelectionChangedInternal()
{
    ///disconnect previous selection
    std::list<std::set<double> > toRemove;
    
    for (SelectedItems::const_iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        if (isBezier) {
            QObject::disconnect( isBezier.get(), SIGNAL( keyframeSet(double) ), this, SLOT( onSelectedBezierKeyframeSet(double) ) );
            QObject::disconnect( isBezier.get(), SIGNAL( keyframeRemoved(double) ), this, SLOT( onSelectedBezierKeyframeRemoved(double) ) );
            QObject::disconnect( isBezier.get(), SIGNAL( animationRemoved() ), this, SLOT( onSelectedBezierAnimationRemoved() ) );
            QObject::disconnect( isBezier.get(), SIGNAL( aboutToClone() ), this, SLOT( onSelectedBezierAboutToClone() ) );
            QObject::disconnect( isBezier.get(), SIGNAL( cloned() ), this, SLOT( onSelectedBezierCloned() ) );
        }
        ItemKeys::iterator found = _imp->keyframes.find(*it);
        if (found != _imp->keyframes.end()) {
            toRemove.push_back(found->second.keys);
            found->second.visible = false;
        }
    }
    _imp->selectedItems.clear();
    
    ///Remove previous selection's keyframes
    {
        std::list<std::set<double> >::iterator next = toRemove.begin();
        if (next != toRemove.end()) {
            ++next;
        }
        for (std::list<std::set<double> >::iterator it = toRemove.begin() ; it != toRemove.end(); ++it) {
            _imp->setVisibleItemKeyframes(*it, false, next == toRemove.end());
            if (next != toRemove.end()) {
                ++next;
            }
        }
    }
    
    ///connect new selection
    
    std::list<std::set<double> > toAdd;
    int selectedBeziersCount = 0;
    const std::list<boost::shared_ptr<RotoItem> > & items = _imp->context->getSelectedItems();
    for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = items.begin(); it != items.end(); ++it) {
        _imp->selectedItems.push_back(*it);
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        const RotoLayer* isLayer = dynamic_cast<const RotoLayer*>( it->get() );
        if (isBezier) {
            QObject::connect( isBezier.get(), SIGNAL( keyframeSet(double) ), this, SLOT( onSelectedBezierKeyframeSet(double) ) );
            QObject::connect( isBezier.get(), SIGNAL( keyframeRemoved(double) ), this, SLOT( onSelectedBezierKeyframeRemoved(double) ) );
            QObject::connect( isBezier.get(), SIGNAL( animationRemoved() ), this, SLOT( onSelectedBezierAnimationRemoved() ) );
            QObject::connect( isBezier.get(), SIGNAL( aboutToClone() ), this, SLOT( onSelectedBezierAboutToClone() ) );
            QObject::connect( isBezier.get(), SIGNAL( cloned() ), this, SLOT( onSelectedBezierCloned() ) );
            ++selectedBeziersCount;
        } else if ( isLayer && !isLayer->getItems().empty() ) {
            ++selectedBeziersCount;
        }
        ItemKeys::iterator found = _imp->keyframes.find(*it);
        if (found != _imp->keyframes.end()) {
            toAdd.push_back(found->second.keys);
            found->second.visible = true;
        }
    }
    
    ///Add new selection keyframes
    {
        std::list<std::set<double> >::iterator next = toAdd.begin();
        if (next != toAdd.end()) {
            ++next;
        }
        for (std::list<std::set<double> >::iterator it = toAdd.begin() ; it != toAdd.end(); ++it) {
            _imp->setVisibleItemKeyframes(*it, true, next == toAdd.end());
            if (next != toAdd.end()) {
                ++next;
            }
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
    _imp->clearAnimation->setEnabled(enabled);
    

    double time = _imp->context->getTimelineCurrentTime();

    ///update the splines info GUI
    _imp->updateSplinesInfoGUI(time);
}

void
RotoPanel::onSelectionChanged(int reason)
{
    if ( (RotoItem::SelectionReasonEnum)reason == RotoItem::eSelectionReasonSettingsPanel ) {
        return;
    }

    onSelectionChangedInternal();
    _imp->tree->blockSignals(true);
    _imp->tree->clearSelection();
    ///now update the selection according to the selected curves
    ///Note that layers will not be selected because this is called when the user clicks on a overlay on the viewer
    for (SelectedItems::iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        TreeItems::iterator found = _imp->findItem(*it);
        if ( found == _imp->items.end() ) {
            _imp->selectedItems.erase(it);
            break;
        }
        found->treeItem->setSelected(true);
    }
    _imp->tree->blockSignals(false);
}

void
RotoPanel::onSelectedBezierKeyframeSet(double time)
{
    Bezier* b = qobject_cast<Bezier*>( sender() );
    boost::shared_ptr<Bezier> isBezier;
    if (b) {
        isBezier = boost::dynamic_pointer_cast<Bezier>(b->shared_from_this());
    }
    _imp->updateSplinesInfoGUI(time);
    if (isBezier) {
        _imp->setItemKey(isBezier, time);
    }
}

void
RotoPanel::onSelectedBezierKeyframeRemoved(double time)
{
    Bezier* b = qobject_cast<Bezier*>( sender() );
    boost::shared_ptr<Bezier> isBezier ;
    if (b) {
        isBezier = boost::dynamic_pointer_cast<Bezier>(b->shared_from_this());
    }
    _imp->updateSplinesInfoGUI(time);
    if (isBezier) {
        _imp->removeItemKey(isBezier, time);
    }
}

void
RotoPanel::onSelectedBezierAnimationRemoved()
{
    Bezier* b = qobject_cast<Bezier*>( sender() );
    boost::shared_ptr<Bezier> isBezier ;
    if (b) {
        isBezier = boost::dynamic_pointer_cast<Bezier>(b->shared_from_this());
    }

    _imp->updateSplinesInfoGUI(_imp->context->getTimelineCurrentTime());
    if (isBezier) {
       _imp->removeItemAnimation(isBezier);
    }
}

void
RotoPanel::onSelectedBezierAboutToClone()
{
    Bezier* b = qobject_cast<Bezier*>( sender() );
    boost::shared_ptr<Bezier> isBezier ;
    if (b) {
        isBezier = boost::dynamic_pointer_cast<Bezier>(b->shared_from_this());
    }

    if (isBezier) {
        ItemKeys::iterator it = _imp->keyframes.find(isBezier);
        if ( it != _imp->keyframes.end() ) {
            if (it->second.visible) {
                _imp->setVisibleItemKeyframes(it->second.keys, false, true);
                ///Hack, making the visible flag to true, for the onSelectedBezierCloned() function
                it->second.visible = true;
            }
        }
    }
}

void
RotoPanel::onSelectedBezierCloned()
{
    Bezier* b = qobject_cast<Bezier*>( sender() );
    boost::shared_ptr<Bezier> isBezier;
    if (b) {
        isBezier = boost::dynamic_pointer_cast<Bezier>(b->shared_from_this());
    }

    if (isBezier) {
        ItemKeys::iterator it = _imp->keyframes.find(isBezier);
        if ( it != _imp->keyframes.end() ) {
            std::set<double> keys;
            isBezier->getKeyframeTimes(&keys);
            it->second.keys = keys;
            if (it->second.visible) {
                _imp->setVisibleItemKeyframes(it->second.keys, true, true);
                it->second.visible = true;
            }
        }
    }
}

static void
makeSolidIcon(double *color,
              QIcon & icon)
{
    QPixmap p(15,15);
    QColor c;

    c.setRgbF(clamp<qreal>(color[0], 0., 1.),
              clamp<qreal>(color[1], 0., 1.),
              clamp<qreal>(color[2], 0., 1.));
    p.fill(c);
    icon.addPixmap(p);
}

void
RotoPanel::updateItemGui(QTreeWidgetItem* item)
{
    double time = _imp->context->getTimelineCurrentTime();
    TreeItems::iterator it = _imp->findItem(item);

    assert( it != _imp->items.end() );
    it->treeItem->setIcon(COL_ACTIVATED,it->rotoItem->isGloballyActivated() ? _imp->iconVisible : _imp->iconUnvisible);
    it->treeItem->setIcon(COL_LOCKED,it->rotoItem->isLockedRecursive() ? _imp->iconLocked : _imp->iconUnlocked);

    RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>( it->rotoItem.get() );
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
#ifdef NATRON_ROTO_INVERTIBLE
        it->treeItem->setIcon(COL_INVERTED,drawable->getInverted(time) ? _imp->iconInverted : _imp->iconUninverted);
#endif
        QWidget* w = _imp->tree->itemWidget(it->treeItem,COL_OPERATOR);
        assert(w);
        ComboBox* cb = dynamic_cast<ComboBox*>(w);
        assert(cb);
        cb->setCurrentIndex_no_emit( drawable->getCompositingOperator() );
    }
}

void
RotoPanelPrivate::updateSplinesInfoGUI(double time)
{
  
    std::set<double> keyframes;

    for (SelectedItems::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        if (isBezier) {
            isBezier->getKeyframeTimes(&keyframes);
        }
    }
    totalKeyframes->setValue( (double)keyframes.size() );

    if ( keyframes.empty() ) {
        currentKeyframe->setValue(1.);
        currentKeyframe->setAnimation(0);
    } else {
        ///get the first time that is equal or greater to the current time
        std::set<double>::iterator lowerBound = keyframes.lower_bound(time);
        int dist = 0;
        if ( lowerBound != keyframes.end() ) {
            dist = std::distance(keyframes.begin(), lowerBound);
        }

        if ( lowerBound == keyframes.end() ) {
            ///we're after the last keyframe
            currentKeyframe->setValue( (double)keyframes.size() );
            currentKeyframe->setAnimation(1);
        } else if (*lowerBound == time) {
            currentKeyframe->setValue(dist + 1);
            currentKeyframe->setAnimation(2);
        } else {
            ///we're in-between 2 keyframes, interpolate
            if ( lowerBound == keyframes.begin() ) {
                currentKeyframe->setValue(1.);
            } else {
                std::set<double>::iterator prev = lowerBound;
                if (prev != keyframes.begin()) {
                    --prev;
                }
                currentKeyframe->setValue( (double)(time - *prev) / (double)(*lowerBound - *prev) + dist );
            }

            currentKeyframe->setAnimation(1);
        }
    }

    ///Refresh the  inverted state
/*    for (TreeItems::iterator it = items.begin(); it != items.end(); ++it) {
        RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>( it->rotoItem.get() );
        if (drawable) {
            QIcon shapeColorIC;
            double shapeColor[3];
            drawable->getColor(time, shapeColor);
            makeSolidIcon(shapeColor, shapeColorIC);
            it->treeItem->setIcon(COL_COLOR, shapeColorIC);
#ifdef NATRON_ROTO_INVERTIBLE
            it->treeItem->setIcon(COL_INVERTED,drawable->getInverted(time) ? iconInverted : iconUninverted);
#endif
//            ComboBox* cb = dynamic_cast<ComboBox*>(tree->itemWidget(it->treeItem, COL_OPERATOR));
//            if (cb) {
//                cb->setCurrentIndex_no_emit(drawable->getCompositingOperator(time));
//            }
        }
    }*/
} // updateSplinesInfoGUI

void
RotoPanel::onTimeChanged(SequenceTime time,
                         int /*reason*/)
{
    _imp->updateSplinesInfoGUI(time);
}

static void
expandRecursively(QTreeWidgetItem* item)
{
    item->setExpanded(true);
    if ( item->parent() ) {
        expandRecursively( item->parent() );
    }
}

void
RotoPanelPrivate::insertItemRecursively(double time,
                                        const boost::shared_ptr<RotoItem> & item,int indexInParentLayer)
{
    QTreeWidgetItem* treeItem = new QTreeWidgetItem;
    boost::shared_ptr<RotoLayer> parent = item->getParentLayer();

    if (parent) {
        TreeItems::iterator parentIT = findItem(parent);

        ///the parent must have already been inserted!
        assert( parentIT != items.end() );
        parentIT->treeItem->insertChild(indexInParentLayer,treeItem);
    } else {
        tree->insertTopLevelItem(0,treeItem);
    }
    items.push_back( TreeItem(treeItem,item) );

    treeItem->setText( COL_LABEL, item->getLabel().c_str() );
    treeItem->setToolTip( COL_LABEL, Natron::convertFromPlainText(kRotoLabelHint, Qt::WhiteSpaceNormal) );

    treeItem->setText(COL_SCRIPT_NAME, item->getScriptName().c_str());
    treeItem->setToolTip( COL_SCRIPT_NAME, Natron::convertFromPlainText(kRotoScriptNameHint, Qt::WhiteSpaceNormal) );
    
    treeItem->setIcon(COL_ACTIVATED, item->isGloballyActivated() ? iconVisible : iconUnvisible);
    treeItem->setToolTip( COL_ACTIVATED, Natron::convertFromPlainText(publicInterface->tr("Controls whether the overlay should be visible on the viewer for "
                                                                  "the shape."), Qt::WhiteSpaceNormal) );
    treeItem->setIcon(COL_LOCKED, item->getLocked() ? iconLocked : iconUnlocked);
    treeItem->setToolTip( COL_LOCKED, Natron::convertFromPlainText(publicInterface->tr(kRotoLockedHint), Qt::WhiteSpaceNormal) );

    boost::shared_ptr<RotoDrawableItem> drawable = boost::dynamic_pointer_cast<RotoDrawableItem>(item);
    boost::shared_ptr<RotoLayer> layer = boost::dynamic_pointer_cast<RotoLayer>(item);

    if (drawable) {
        
        RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>(drawable.get());
        double overlayColor[4];
        drawable->getOverlayColor(overlayColor);
        QIcon overlayIcon;
        makeSolidIcon(overlayColor, overlayIcon);
        if (!isStroke) {
            treeItem->setIcon(COL_LABEL, iconBezier);
        } else {
            switch (isStroke->getBrushType()) {
                case Natron::eRotoStrokeTypeBlur:
                    treeItem->setIcon(COL_LABEL, iconBlur);
                    break;
                case Natron::eRotoStrokeTypeBurn:
                    treeItem->setIcon(COL_LABEL, iconBurn);
                    break;
                case Natron::eRotoStrokeTypeClone:
                    treeItem->setIcon(COL_LABEL, iconClone);
                    break;
                case Natron::eRotoStrokeTypeDodge:
                    treeItem->setIcon(COL_LABEL, iconDodge);
                    break;
                case Natron::eRotoStrokeTypeEraser:
                    treeItem->setIcon(COL_LABEL, iconEraser);
                    break;
                case Natron::eRotoStrokeTypeReveal:
                    treeItem->setIcon(COL_LABEL, iconReveal);
                    break;
                case Natron::eRotoStrokeTypeSharpen:
                    treeItem->setIcon(COL_LABEL, iconSharpen);
                    break;
                case Natron::eRotoStrokeTypeSmear:
                    treeItem->setIcon(COL_LABEL, iconSmear);
                    break;
                case Natron::eRotoStrokeTypeSolid:
                    treeItem->setIcon(COL_LABEL, iconStroke);
                    break;
            }
        }
        treeItem->setIcon(COL_OVERLAY,overlayIcon);
        treeItem->setToolTip( COL_OVERLAY, Natron::convertFromPlainText(publicInterface->tr(kRotoOverlayHint), Qt::WhiteSpaceNormal) );
        double shapeColor[3];
        drawable->getColor(time, shapeColor);
        QIcon shapeIcon;
        makeSolidIcon(shapeColor, shapeIcon);
        treeItem->setIcon(COL_COLOR, shapeIcon);
        treeItem->setToolTip( COL_COLOR, Natron::convertFromPlainText(publicInterface->tr(kRotoColorHint), Qt::WhiteSpaceNormal) );
#ifdef NATRON_ROTO_INVERTIBLE
        treeItem->setIcon(COL_INVERTED, drawable->getInverted(time)  ? iconInverted : iconUninverted);
        treeItem->setTooltip( COL_INVERTED, Natron::convertFromPlainText(tr(kRotoInvertedHint), Qt::WhiteSpaceNormal) );
#endif

        publicInterface->makeCustomWidgetsForItem(drawable,treeItem);
#ifdef NATRON_ROTO_INVERTIBLE
        QObject::connect( drawable,SIGNAL( invertedStateChanged() ), publicInterface, SLOT( onRotoItemInvertedStateChanged() ) );
#endif
        QObject::connect( drawable.get(),
                         SIGNAL( shapeColorChanged() ),
                         publicInterface,
                         SLOT( onRotoItemShapeColorChanged() ) );
        QObject::connect( drawable.get(),
                         SIGNAL( compositingOperatorChanged(int,int) ),
                         publicInterface,
                         SLOT( onRotoItemCompOperatorChanged(int,int) ) );
        
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(item);
        if (isBezier) {
            ItemKeys::iterator it = keyframes.find(isBezier);
            if ( it == keyframes.end() ) {
                TimeLineKeys keys;
                isBezier->getKeyframeTimes(&keys.keys);
                keys.visible = false;
                keyframes.insert( std::make_pair(isBezier, keys) );
            }
        }
        
    } else if (layer) {
        treeItem->setIcon(0, iconLayer);
        ///insert children
        const std::list<boost::shared_ptr<RotoItem> > & children = layer->getItems();
        int i = 0;
        for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = children.begin(); it != children.end(); ++it,++i) {
            insertItemRecursively(time,*it,i);
        }
    }
    expandRecursively(treeItem);
} // insertItemRecursively

boost::shared_ptr<NodeGui>
RotoPanel::getNode() const
{
    return _imp->node.lock();
}

void
RotoPanel::makeCustomWidgetsForItem(const boost::shared_ptr<RotoDrawableItem>& item,
                                    QTreeWidgetItem* treeItem)
{
    //If NULL was passed to treeItem,find it ourselves
    if (!treeItem) {
        TreeItems::iterator found = _imp->findItem(item);
        if ( found == _imp->items.end() ) {
            return;
        }
        treeItem = found->treeItem;
    }
    
    ComboBox* cb = new ComboBox;
    QObject::connect( cb,SIGNAL( currentIndexChanged(int) ),this,SLOT( onCurrentItemCompOperatorChanged(int) ) );
    std::vector<std::string> compositingOperators,tooltips;
    getNatronCompositingOperators(&compositingOperators, &tooltips);
    for (U32 i = 0; i < compositingOperators.size(); ++i) {
        cb->addItem( compositingOperators[i].c_str(),QIcon(),QKeySequence(),tooltips[i].c_str() );
    }
    // set the tooltip
    const std::string & tt = item->getCompositingOperatorToolTip();

    cb->setToolTip( Natron::convertFromPlainText(QString(tt.c_str()).trimmed(), Qt::WhiteSpaceNormal) );
    cb->setCurrentIndex_no_emit( item->getCompositingOperator() );
    QObject::connect(cb, SIGNAL(minimumSizeChanged(QSize)), this, SLOT(onOperatorColMinimumSizeChanged(QSize)));
    _imp->tree->setItemWidget(treeItem, COL_OPERATOR, cb);
    
    //We must call this otherwise this is never called by Qt for custom widgets (this is a Qt bug)
    QSize s = cb->minimumSizeHint();
    Q_UNUSED(s);
}

void
RotoPanelPrivate::removeItemRecursively(const boost::shared_ptr<RotoItem>& item)
{
    TreeItems::iterator it = findItem(item);

    if ( it == items.end() ) {
        return;
    }
#ifdef NATRON_ROTO_INVERTIBLE
    RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>(item);
    if (drawable) {
        QObject::disconnect( drawable,SIGNAL( invertedStateChanged() ), publicInterface, SLOT( onRotoItemInvertedStateChanged() ) );
    }
#endif

    ///deleting the item will Q_EMIT a selection change which would lead to a deadlock
    tree->blockSignals(true);
    delete it->treeItem;
    tree->blockSignals(false);
    items.erase(it);
}

void
RotoPanel::onItemInserted(int index,int reason)
{
    boost::shared_ptr<RotoItem> lastInsertedItem = _imp->context->getLastInsertedItem();
    double time = _imp->context->getTimelineCurrentTime();

    _imp->insertItemInternal(reason,time, lastInsertedItem,index);
}

void
RotoPanelPrivate::insertItemInternal(int reason,
                                     double time,
                                     const boost::shared_ptr<RotoItem> & item,
                                     int indexInParentLayer)
{
    boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(item);

    if (isBezier) {
        ItemKeys::iterator it = keyframes.find(isBezier);
        if ( it == keyframes.end() ) {
            TimeLineKeys keys;
            isBezier->getKeyframeTimes(&keys.keys);
            keys.visible = false;
            keyframes.insert( std::make_pair(isBezier, keys) );
        }
    }
    if ( (RotoItem::SelectionReasonEnum)reason == RotoItem::eSelectionReasonSettingsPanel ) {
        boost::shared_ptr<RotoDrawableItem> drawable = boost::dynamic_pointer_cast<RotoDrawableItem>(item);
        if (drawable) {
            publicInterface->makeCustomWidgetsForItem(drawable);
        }

        return;
    }
    assert(item);
    insertItemRecursively(time, item, indexInParentLayer);
}

void
RotoPanel::onItemRemoved(const boost::shared_ptr<RotoItem>& item,
                         int reason)
{
    Bezier* b = qobject_cast<Bezier*>( sender() );
    boost::shared_ptr<Bezier> isBezier;
    if (b) {
        isBezier = boost::dynamic_pointer_cast<Bezier>(b->shared_from_this());
    }

    if (isBezier) {
        ItemKeys::iterator it = _imp->keyframes.find(isBezier);
        if ( it != _imp->keyframes.end() ) {

            for (std::set<double>::iterator it2 = it->second.keys.begin(); it2 != it->second.keys.end(); ++it2) {
                getNode()->getNode()->getApp()->removeKeyFrameIndicator(*it2);
            }
            _imp->keyframes.erase(it);
        }
    }
    if ( (RotoItem::SelectionReasonEnum)reason == RotoItem::eSelectionReasonSettingsPanel ) {
        return;
    }
    _imp->removeItemRecursively(item);
}

void
RotoPanelPrivate::buildTreeFromContext()
{
    double time = context->getTimelineCurrentTime();
    const std::list< boost::shared_ptr<RotoLayer> > & layers = context->getLayers();

    tree->blockSignals(true);
    if ( !layers.empty() ) {
        const boost::shared_ptr<RotoLayer> & base = layers.front();
        insertItemRecursively(time, base, 0);
    }
    tree->blockSignals(false);
}

void
RotoPanel::onCurrentItemCompOperatorChanged(int index)
{
    QWidget* comboboxSender = qobject_cast<QWidget*>( sender() );

    assert(comboboxSender);
    for (TreeItems::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
        if (_imp->tree->itemWidget(it->treeItem,COL_OPERATOR) == comboboxSender) {
            RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>( it->rotoItem.get() );
            assert(drawable);
            boost::shared_ptr<KnobChoice> op = drawable->getOperatorKnob();
            KeyFrame k;
            op->setValue(index, 0, Natron::eValueChangedReasonUserEdited,&k);
            _imp->context->clearSelection(RotoItem::eSelectionReasonOther);
            _imp->context->select(it->rotoItem, RotoItem::eSelectionReasonOther);
            _imp->context->evaluateChange();
            break;
        }
    }
}

#ifdef NATRON_ROTO_INVERTIBLE
void
RotoPanel::onRotoItemInvertedStateChanged()
{
    RotoDrawableItem* item = qobject_cast<RotoDrawableItem*>( sender() );

    if (item) {
        double time = _imp->context->getTimelineCurrentTime();
        TreeItems::iterator it = _imp->findItem(item);
        if ( it != _imp->items.end() ) {
            it->treeItem->setIcon(COL_INVERTED, item->getInverted(time)  ? _imp->iconInverted : _imp->iconUninverted);
        }
    }
}

#endif

void
RotoPanel::onRotoItemShapeColorChanged()
{
    RotoDrawableItem* i = qobject_cast<RotoDrawableItem*>( sender() );
    boost::shared_ptr<RotoDrawableItem> item;
    if (i) {
        item = boost::dynamic_pointer_cast<RotoDrawableItem>(i->shared_from_this());
    }

    if (item) {
        double time = _imp->context->getTimelineCurrentTime();
        TreeItems::iterator it = _imp->findItem(item);
        if ( it != _imp->items.end() ) {
            QIcon icon;
            double shapeColor[3];
            item->getColor(time, shapeColor);
            makeSolidIcon(shapeColor, icon);
            it->treeItem->setIcon(COL_COLOR,icon);
        }
    }
}

void
RotoPanel::onRotoItemCompOperatorChanged(int /*dim*/,
                                         int reason)
{
    if ( (ValueChangedReasonEnum)reason == Natron::eValueChangedReasonSlaveRefresh ) {
        return;
    }
    RotoDrawableItem* i = qobject_cast<RotoDrawableItem*>( sender() );
    boost::shared_ptr<RotoDrawableItem> item;
    if (i) {
        item = boost::dynamic_pointer_cast<RotoDrawableItem>(i->shared_from_this());
    }

    if (item) {
        TreeItems::iterator it = _imp->findItem(item);
        if ( it != _imp->items.end() ) {
            ComboBox* cb = dynamic_cast<ComboBox*>( _imp->tree->itemWidget(it->treeItem, COL_OPERATOR) );
            if (cb) {
                int compIndex = item->getCompositingOperator();
                cb->setCurrentIndex_no_emit(compIndex);
            }
        }
    }
}

void
RotoPanel::onItemClicked(QTreeWidgetItem* item,
                         int column)
{
    TreeItems::iterator it = _imp->findItem(item);

    if ( it != _imp->items.end() ) {
        switch (column) {
        case COL_ACTIVATED: {
            bool activated = !it->rotoItem->isGloballyActivated();
            QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
            for (int i = 0; i < selected.size(); ++i) {
                TreeItems::iterator found = _imp->findItem(selected[i]);
                assert( found != _imp->items.end() );
                found->rotoItem->setGloballyActivated(activated, true);
                _imp->setChildrenActivatedRecursively(activated, found->treeItem);
            }
            _imp->context->emitRefreshViewerOverlays();
            break;
        }

        case COL_LOCKED: {
            bool locked = !it->rotoItem->getLocked();
            QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
            for (int i = 0; i < selected.size(); ++i) {
                TreeItems::iterator found = _imp->findItem(selected[i]);
                assert( found != _imp->items.end() );
                found->rotoItem->setLocked(locked,true,RotoItem::eSelectionReasonSettingsPanel);
                _imp->setChildrenLockedRecursively(locked, found->treeItem);
            }
            break;
        }

#ifdef NATRON_ROTO_INVERTIBLE
        case COL_INVERTED: {
            QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
            bool inverted = false;
            bool invertedSet = false;
            for (int i = 0; i < selected.size(); ++i) {
                TreeItems::iterator found = _imp->findItem(selected[i]);
                assert( found != _imp->items.end() );
                RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>( found->rotoItem.get() );
                if (drawable) {
                    boost::shared_ptr<KnobBool> invertedKnob = drawable->getInvertedKnob();
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
            break;
        }
#endif
        case 0:
        default:
            break;
        } // switch
    }
} // onItemClicked

void
RotoPanel::onItemColorDialogEdited(const QColor & color)
{
    QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
    bool mustEvaluate = false;
    for (int i = 0; i < selected.size(); ++i) {
        TreeItems::iterator found = _imp->findItem(selected[i]);
        assert( found != _imp->items.end() );
        RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>( found->rotoItem.get() );
        if (drawable) {
            if (_imp->dialogEdition == eColorDialogEditingShapeColor) {
                boost::shared_ptr<KnobColor> colorKnob = drawable->getColorKnob();
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

                _imp->context->getColorKnob()->setValue(colorArray[0], 0);
                _imp->context->getColorKnob()->setValue(colorArray[1], 1);
                _imp->context->getColorKnob()->setValue(colorArray[2], 2);
                mustEvaluate = true;
            } else if (_imp->dialogEdition == eColorDialogEditingOverlayColor) {
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
    if (mustEvaluate) {
        _imp->context->evaluateChange();
    }
}

void
RotoPanelPrivate::setChildrenActivatedRecursively(bool activated,
                                                  QTreeWidgetItem* item)
{
    item->setIcon(COL_ACTIVATED, activated ? iconVisible : iconUnvisible);
    for (int i = 0; i < item->childCount(); ++i) {
        setChildrenActivatedRecursively( activated,item->child(i) );
    }
}

void
RotoPanel::onItemLockChanged(int reason)
{
    
    boost::shared_ptr<RotoItem> item = getContext()->getLastItemLocked();
    if (!item || (RotoItem::SelectionReasonEnum)reason == RotoItem::eSelectionReasonSettingsPanel) {
        return;
    }
    TreeItems::iterator found = _imp->findItem(item);
    if (found != _imp->items.end()) {
        found->treeItem->setIcon(COL_LOCKED, item->isLockedRecursive() ? _imp->iconLocked : _imp->iconUnlocked);
    }
}

void
RotoPanelPrivate::setChildrenLockedRecursively(bool locked,
                                               QTreeWidgetItem* item)
{
    item->setIcon(COL_LOCKED, locked ? iconLocked : iconUnlocked);
    for (int i = 0; i < item->childCount(); ++i) {
        setChildrenLockedRecursively( locked,item->child(i) );
    }
}

void
RotoPanel::onItemChanged(QTreeWidgetItem* item,
                         int column)
{
    if (column != COL_LABEL) {
        return;
    }
    TreeItems::iterator it = _imp->findItem(item);
    if ( it != _imp->items.end() ) {
        std::string newName = item->text(column).toStdString();
        _imp->settingNameFromGui = true;
        it->rotoItem->setLabel(newName);
        _imp->settingNameFromGui = false;
    }
}

void
RotoPanel::onItemLabelChanged(const boost::shared_ptr<RotoItem>& item)
{
    if (_imp->settingNameFromGui) {
        return;
    }
    TreeItems::iterator it = _imp->findItem(item);
    if (it != _imp->items.end()) {
        it->treeItem->setText(COL_LABEL, item->getLabel().c_str());
    }
}

void
RotoPanel::onItemScriptNameChanged(const boost::shared_ptr<RotoItem>& item)
{
    if (_imp->settingNameFromGui) {
        return;
    }
    TreeItems::iterator it = _imp->findItem(item);
    if (it != _imp->items.end()) {
        it->treeItem->setText(COL_SCRIPT_NAME, item->getScriptName().c_str());
    }
}

void
RotoPanel::onItemDoubleClicked(QTreeWidgetItem* item,
                               int column)
{
   
    TreeItems::iterator it = _imp->findItem(item);
    if ( it != _imp->items.end() ) {
        
        switch (column) {
            case COL_LABEL: {
                _imp->editedItem = item;
                QObject::connect( qApp, SIGNAL( focusChanged(QWidget*,QWidget*) ), this, SLOT( onFocusChanged(QWidget*,QWidget*) ) );
                _imp->editedItemName = it->rotoItem->getLabel();
                _imp->tree->openPersistentEditor(item);
            }   break;
            case COL_OVERLAY: {
                RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>( it->rotoItem.get() );
                if (drawable) {
                    QColorDialog dialog;
                    _imp->dialogEdition = eColorDialogEditingOverlayColor;
                    double oc[4];
                    drawable->getOverlayColor(oc);
                    QColor color;
                    color.setRgbF(oc[0], oc[1], oc[2]);
                    color.setAlphaF(oc[3]);
                    dialog.setCurrentColor(color);
                    QObject::connect( &dialog,SIGNAL( currentColorChanged(QColor) ),this,SLOT( onItemColorDialogEdited(QColor) ) );
                    if ( dialog.exec() ) {
                        color = dialog.selectedColor();
                        oc[0] = color.redF();
                        oc[1] = color.greenF();
                        oc[2] = color.blueF();
                        oc[3] = color.alphaF();
                    }
                    _imp->dialogEdition = eColorDialogEditingNothing;
                    QPixmap pix(15,15);
                    pix.fill(color);
                    QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
                    for (int i = 0; i < selected.size(); ++i) {
                        TreeItems::iterator found = _imp->findItem(selected[i]);
                        assert( found != _imp->items.end() );
                        drawable = dynamic_cast<RotoDrawableItem*>( found->rotoItem.get() );
                        if (drawable) {
                            drawable->setOverlayColor(oc);
                            found->treeItem->setIcon( COL_OVERLAY, QIcon(pix) );
                        }
                    }
                }
                break;
            }
                
            case COL_COLOR: {
                double time = _imp->context->getTimelineCurrentTime();
                RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>( it->rotoItem.get() );
                QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
                bool colorChosen = false;
                double shapeColor[3];
                bool mustEvaluate = false;
                if (drawable) {
                    QColorDialog dialog;
                    _imp->dialogEdition = eColorDialogEditingShapeColor;
                    drawable->getColor(time,shapeColor);
                    QColor color;
                    color.setRgbF(shapeColor[0], shapeColor[1], shapeColor[2]);
                    dialog.setCurrentColor(color);
                    QObject::connect( &dialog,SIGNAL( currentColorChanged(QColor) ),this,SLOT( onItemColorDialogEdited(QColor) ) );
                    if ( dialog.exec() ) {
                        color = dialog.selectedColor();
                        shapeColor[0] = color.redF();
                        shapeColor[1] = color.greenF();
                        shapeColor[2] = color.blueF();
                    }
                    _imp->dialogEdition = eColorDialogEditingNothing;
                    QIcon icon;
                    makeSolidIcon(shapeColor, icon);
                    colorChosen = true;
                    for (int i = 0; i < selected.size(); ++i) {
                        TreeItems::iterator found = _imp->findItem(selected[i]);
                        assert( found != _imp->items.end() );
                        drawable = dynamic_cast<RotoDrawableItem*>( found->rotoItem.get() );
                        if (drawable) {
                            boost::shared_ptr<KnobColor> colorKnob = drawable->getColorKnob();
                            colorKnob->setValue(shapeColor[0], 0);
                            colorKnob->setValue(shapeColor[1], 1);
                            colorKnob->setValue(shapeColor[2], 2);
                            mustEvaluate = true;
                            found->treeItem->setIcon(COL_COLOR, icon);
                        }
                    }
                }
                
                if ( colorChosen && !selected.empty() ) {
                    _imp->context->getColorKnob()->setValue(shapeColor[0], 0);
                    _imp->context->getColorKnob()->setValue(shapeColor[1], 1);
                    _imp->context->getColorKnob()->setValue(shapeColor[2], 2);
                    mustEvaluate = true;
                }
                if (mustEvaluate) {
                    _imp->context->evaluateChange();
                }
                break;
            }
            default:
                break;
        }
        
        
    }
}

void
RotoPanel::onCurrentItemChanged(QTreeWidgetItem* /*current*/,
                                QTreeWidgetItem* /*previous*/)
{
    onTreeOutOfFocusEvent();
}

void
RotoPanel::onTreeOutOfFocusEvent()
{
    if (_imp->editedItem) {
        _imp->tree->closePersistentEditor(_imp->editedItem);
        _imp->editedItem = NULL;
    }
}

void
RotoPanel::onFocusChanged(QWidget* old,
                          QWidget*)
{
    if (_imp->editedItem) {
        QWidget* w = _imp->tree->itemWidget(_imp->editedItem, COL_LABEL);
        if (w == old) {
            _imp->tree->closePersistentEditor(_imp->editedItem);
            _imp->editedItem = NULL;
        }
    }
}

void
RotoPanelPrivate::insertSelectionRecursively(const boost::shared_ptr<RotoLayer> & layer)
{
    const std::list<boost::shared_ptr<RotoItem> > & children = layer->getItems();

    for (std::list<boost::shared_ptr<RotoItem> >::const_iterator it = children.begin(); it != children.end(); ++it) {
        boost::shared_ptr<RotoLayer> l = boost::dynamic_pointer_cast<RotoLayer>(*it);
        SelectedItems::iterator found = std::find(selectedItems.begin(), selectedItems.end(), *it);
        if ( found == selectedItems.end() ) {
            context->select(*it, RotoItem::eSelectionReasonSettingsPanel);
            selectedItems.push_back(*it);
        }
        if (l) {
            insertSelectionRecursively(l);
        }
    }
}

void
RotoPanel::onItemSelectionChanged()
{
    ///disconnect previous selection
    std::list<std::set<double> > toRemove;
    for (SelectedItems::const_iterator it = _imp->selectedItems.begin(); it != _imp->selectedItems.end(); ++it) {
        boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        if (isBezier) {
            QObject::disconnect( isBezier.get(),
                                SIGNAL( keyframeSet(double) ),
                                this,
                                SLOT( onSelectedBezierKeyframeSet(double) ) );
            QObject::disconnect( isBezier.get(),
                                SIGNAL( keyframeRemoved(double) ),
                                this,
                                SLOT( onSelectedBezierKeyframeRemoved(double) ) );
            QObject::disconnect( isBezier.get(), SIGNAL( animationRemoved() ), this, SLOT( onSelectedBezierAnimationRemoved() ) );
        }
        ItemKeys::iterator found = _imp->keyframes.find(*it);
        if (found != _imp->keyframes.end()) {
            toRemove.push_back(found->second.keys);
            found->second.visible = false;
        }
    }
    
    ///Remove previous selection's keyframes
    {
        std::list<std::set<double> >::iterator next = toRemove.begin();
        if (next != toRemove.end()) {
            ++next;
        }
        for (std::list<std::set<double> >::iterator it = toRemove.begin() ; it != toRemove.end(); ++it) {
            _imp->setVisibleItemKeyframes(*it, false, next == toRemove.end());
            if (next != toRemove.end()) {
                ++next;
            }
        }
    }

    
    _imp->context->deselect(_imp->selectedItems, RotoItem::eSelectionReasonSettingsPanel);
    _imp->selectedItems.clear();

    ///Don't allow any selection to be made if the roto is a clone of another roto  node.
    if ( getNode()->getNode()->getMasterNode() ) {
        _imp->tree->selectionModel()->clear();

        return;
    }

    QList<QTreeWidgetItem*> selectedItems = _imp->tree->selectedItems();
    std::list<std::set<double> > toAdd;
    int selectedBeziersCount = 0;
    for (int i = 0; i < selectedItems.size(); ++i) {
        TreeItems::iterator it = _imp->findItem(selectedItems[i]);
        assert( it != _imp->items.end() );
        boost::shared_ptr<Bezier> bezier = boost::dynamic_pointer_cast<Bezier>(it->rotoItem);
        boost::shared_ptr<RotoStrokeItem> stroke = boost::dynamic_pointer_cast<RotoStrokeItem>(it->rotoItem);
        boost::shared_ptr<RotoLayer> layer = boost::dynamic_pointer_cast<RotoLayer>(it->rotoItem);
        if (bezier) {
            SelectedItems::iterator found = std::find(_imp->selectedItems.begin(), _imp->selectedItems.end(), bezier);
            if ( found == _imp->selectedItems.end() ) {
                _imp->selectedItems.push_back(bezier);
                ++selectedBeziersCount;
            }
        } else if (stroke) {
            SelectedItems::iterator found = std::find(_imp->selectedItems.begin(), _imp->selectedItems.end(), stroke);
            if ( found == _imp->selectedItems.end() ) {
                _imp->selectedItems.push_back(stroke);
            }
        } else if (layer) {
            if ( !layer->getItems().empty() ) {
                ++selectedBeziersCount;
            }
            SelectedItems::iterator found = std::find(_imp->selectedItems.begin(), _imp->selectedItems.end(), it->rotoItem);
            if ( found == _imp->selectedItems.end() ) {
                _imp->selectedItems.push_back(it->rotoItem);
            }
            _imp->insertSelectionRecursively(layer);
        }
        
        ItemKeys::iterator  found = _imp->keyframes.find(it->rotoItem);
        if (found != _imp->keyframes.end()) {
            toAdd.push_back(found->second.keys);
            found->second.visible = true;
        }
    }
    
    ///Remove previous selection's keyframes
    {
        std::list<std::set<double> >::iterator next = toAdd.begin();
        if (next != toAdd.end()) {
            ++next;
        }
        for (std::list<std::set<double> >::iterator it = toAdd.begin() ; it != toAdd.end(); ++it) {
            _imp->setVisibleItemKeyframes(*it, true, next == toAdd.end());
            if (next != toAdd.end()) {
                ++next;
            }
        }
    }

    
    _imp->context->select(_imp->selectedItems, RotoItem::eSelectionReasonSettingsPanel);

    bool enabled = selectedBeziersCount > 0;

    _imp->splineLabel->setEnabled(enabled);
    _imp->currentKeyframe->setEnabled(enabled);
    _imp->ofLabel->setEnabled(enabled);
    _imp->totalKeyframes->setEnabled(enabled);
    _imp->prevKeyframe->setEnabled(enabled);
    _imp->nextKeyframe->setEnabled(enabled);
    _imp->addKeyframe->setEnabled(enabled);
    _imp->removeKeyframe->setEnabled(enabled);
    _imp->clearAnimation->setEnabled(enabled);
    
    double time = _imp->context->getTimelineCurrentTime();

    ///update the splines info GUI
    _imp->updateSplinesInfoGUI(time);
} // onItemSelectionChanged

void
RotoPanel::onAddLayerButtonClicked()
{
    pushUndoCommand( new AddLayerUndoCommand(this) );
}

void
RotoPanel::onRemoveItemButtonClicked()
{
    QList<QTreeWidgetItem*> selectedItems = _imp->tree->selectedItems();
    pushUndoCommand( new RemoveItemsUndoCommand(this,selectedItems) );
}

static bool
isLayerAParent_recursive(const boost::shared_ptr<RotoLayer>& layer,
                         const boost::shared_ptr<RotoItem>& item)
{
    boost::shared_ptr<RotoLayer> parent = item->getParentLayer();

    if (parent) {
        if (layer == parent) {
            return true;
        } else {
            return isLayerAParent_recursive(layer, parent);
        }
    }
    return false;
}

void
TreeWidget::dragMoveEvent(QDragMoveEvent* e)
{
    const QMimeData* mime = e->mimeData();
    std::list<DroppedTreeItemPtr> droppedItems;
    bool ret = dragAndDropHandler(mime, e->pos(), droppedItems);
    QTreeWidget::dragMoveEvent(e);

    if (!ret) {
        e->setAccepted(ret);
    }
}

static void
checkIfProcessedRecursive(QTreeWidgetItem* matcher,
                        QTreeWidgetItem* item,
                        bool *ret)
{
    if (!item) {
        *ret = false;
        return;
    }
    if (item == matcher) {
        *ret = true;
    } else {
        if ( item && item->parent() ) {
            checkIfProcessedRecursive(matcher,item->parent(),ret);
        }
    }
}

bool
TreeWidget::dragAndDropHandler(const QMimeData* mime,
                               const QPoint & pos,
                               std::list<DroppedTreeItemPtr> & dropped)
{
    if ( mime->hasFormat("application/x-qabstractitemmodeldatalist") ) {
        QByteArray encoded = mime->data("application/x-qabstractitemmodeldatalist");
        QDataStream stream(&encoded,QIODevice::ReadOnly);
        DropIndicatorPosition position = dropIndicatorPosition();

        ///list of items we already handled d&d for. If we find an item whose parent
        ///is already in this list we don't handle it
        std::list<QTreeWidgetItem*> processedItems;
        while ( !stream.atEnd() ) {
            int row, col;
            QMap<int,QVariant> roleDataMap;

            stream >> row >> col >> roleDataMap;

            QMap<int, QVariant>::Iterator it = roleDataMap.find(0);

            if ( it != roleDataMap.end() ) {
                DroppedTreeItemPtr ret(new DroppedTreeItem);

                ///The target item
                QTreeWidgetItem* into = itemAt(pos);

                if (!into) {
                    return false;
                }

                boost::shared_ptr<RotoItem> intoRotoItem = _panel->getRotoItemForTreeItem(into);
                QList<QTreeWidgetItem*> foundDropped = findItems(it.value().toString(),Qt::MatchExactly | Qt::MatchRecursive,0);
                assert( !foundDropped.empty() );
                if (foundDropped.empty()) {
                    return false;
                }

                ///the dropped item
                ret->dropped = foundDropped[0];

                bool processed = false;
                for (std::list<QTreeWidgetItem*>::iterator processedIt = processedItems.begin(); processedIt != processedItems.end(); ++processedIt) {
                    checkIfProcessedRecursive(*processedIt, ret->dropped, &processed);
                    if (processed) {
                        break;
                    }
                }
                if (processed) {
                    continue;
                }


                ret->droppedRotoItem = _panel->getRotoItemForTreeItem(ret->dropped);
                assert(into && ret->dropped && intoRotoItem && ret->droppedRotoItem);

                ///Is the target item a layer ?
                boost::shared_ptr<RotoLayer> isIntoALayer = boost::dynamic_pointer_cast<RotoLayer>(intoRotoItem);

                ///Determine into which layer the item should be inserted.
                switch (position) {
                case QAbstractItemView::AboveItem: {
                    ret->newParentLayer = intoRotoItem->getParentLayer();
                    if (ret->newParentLayer) {
                        ret->newParentItem = into->parent();
                        ///find the target item index into its parent layer and insert the item above it
                        const std::list<boost::shared_ptr<RotoItem> > & children = ret->newParentLayer->getItems();
                        std::list<boost::shared_ptr<RotoItem> >::const_iterator found =
                            std::find(children.begin(),children.end(),intoRotoItem);
                        assert( found != children.end() );
                        int intoIndex = std::distance(children.begin(), found);

                        found = std::find(children.begin(),children.end(),ret->droppedRotoItem);
                        int droppedIndex = -1;
                        if (found != children.end()) {
                            droppedIndex = std::distance(children.begin(), found) ;
                        }
                        
                        ///if the dropped item is already into the children and after the found index don't decrement
                        if (droppedIndex != -1 && droppedIndex > intoIndex) {
                            ret->insertIndex = intoIndex;
                        } else {
                            //The item "above" in the tree is index - 1 in the internal list which is ordered from bottom to top
                            ret->insertIndex = intoIndex == 0 ? 0 : intoIndex - 1;

                        }
                    } else {
                        return false;
                    }
                    break;
                }
                case QAbstractItemView::BelowItem: {
                    boost::shared_ptr<RotoLayer> intoParentLayer = intoRotoItem->getParentLayer();
                    bool isTargetLayerAParent = false;
                    if (isIntoALayer) {
                        isTargetLayerAParent = isLayerAParent_recursive(isIntoALayer, ret->droppedRotoItem);
                    }
                    if (!intoParentLayer || isTargetLayerAParent) {
                        ///insert at the begining of the layer
                        ret->insertIndex = isIntoALayer ? isIntoALayer->getItems().size() : 0;
                        ret->newParentLayer = isIntoALayer;
                        ret->newParentItem = into;
                    } else {
                        assert(intoParentLayer);
                        ///find the target item index into its parent layer and insert the item after it
                        const std::list<boost::shared_ptr<RotoItem> > & children = intoParentLayer->getItems();
                        std::list<boost::shared_ptr<RotoItem> >::const_iterator found =
                            std::find(children.begin(),children.end(),intoRotoItem);
                        assert( found != children.end() );
                        int index = std::distance(children.begin(), found);

                        ///if the dropped item is already into the children and before the found index don't decrement
                        found = std::find(children.begin(),children.end(),ret->droppedRotoItem);
                        if ( ( found != children.end() ) && ( std::distance(children.begin(), found) < index) ) {
                            ret->insertIndex = index;
                        } else {
                            //The item "below" in the tree is index + 1 in the internal list which is ordered from bottom to top
                            ret->insertIndex = index + 1;
                        }

                        ret->newParentLayer = intoParentLayer;
                        ret->newParentItem = into->parent();
                    }
                    break;
                }
                case QAbstractItemView::OnItem: {
                    if (isIntoALayer) {
                        ret->insertIndex =  0; // always insert on-top of others
                        ret->newParentLayer = isIntoALayer;
                        ret->newParentItem = into;
                    } else {
                        ///dropping an item on another item which is not a layer is not accepted
                        return false;
                    }
                    break;
                }
                case QAbstractItemView::OnViewport:
                default:

                    //do nothing we don't accept it
                    return false;
                } // switch
                dropped.push_back(ret);
                if (ret->dropped) {
                    processedItems.push_back(ret->dropped);
                }
            } //  if (it != roleDataMap.end())
        } // while (!stream.atEnd())
    } //if (mime->hasFormat("application/x-qabstractitemmodeldatalist"))
    return true;
} // dragAndDropHandler

void
TreeWidget::dropEvent(QDropEvent* e)
{
    std::list<DroppedTreeItemPtr> droppedItems;
    const QMimeData* mime = e->mimeData();
    bool accepted = dragAndDropHandler(mime,e->pos(), droppedItems);

    e->setAccepted(accepted);

    if (accepted) {
        _panel->pushUndoCommand( new DragItemsUndoCommand(_panel,droppedItems) );
    }
}

void
TreeWidget::keyPressEvent(QKeyEvent* e)
{
    QList<QTreeWidgetItem*> selected = selectedItems();
    QTreeWidgetItem* item = selected.empty() ? 0 :  selected.front();

    if (item) {
        _panel->setLastRightClickedItem(item);
        if ( (e->key() == Qt::Key_Delete) || (e->key() == Qt::Key_Backspace) ) {
            _panel->onRemoveItemButtonClicked();
        } else if ( (e->key() == Qt::Key_C) && modCASIsControl(e) ) {
            _panel->onCopyItemActionTriggered();
        } else if ( (e->key() == Qt::Key_V) && modCASIsControl(e) ) {
            _panel->onPasteItemActionTriggered();
        } else if ( (e->key() == Qt::Key_X) && modCASIsControl(e) ) {
            _panel->onCutItemActionTriggered();
        } else if ( (e->key() == Qt::Key_C) && modCASIsAlt(e) ) {
            _panel->onDuplicateItemActionTriggered();
        } else if ( (e->key() == Qt::Key_A) && modCASIsControl(e) ) {
            _panel->selectAll();
        } else {
            QTreeWidget::keyPressEvent(e);
        }
    } else {
        QTreeWidget::keyPressEvent(e);
    }
}

void
RotoPanel::pushUndoCommand(QUndoCommand* cmd)
{
    NodeSettingsPanel* panel = getNode()->getSettingPanel();

    assert(panel);
    panel->pushUndoCommand(cmd);
}

std::string
RotoPanel::getNodeName() const
{
    return getNode()->getNode()->getScriptName();
}

boost::shared_ptr<RotoContext>
RotoPanel::getContext() const
{
    return _imp->context;
}

void
RotoPanel::clearAndSelectPreviousItem(const boost::shared_ptr<RotoItem> & item)
{
    
    _imp->context->clearAndSelectPreviousItem(item,RotoItem::eSelectionReasonOther);
}

void
RotoPanel::clearSelection()
{
    _imp->selectedItems.clear();
    _imp->context->clearSelection(RotoItem::eSelectionReasonSettingsPanel);
}

void
RotoPanel::showItemMenu(QTreeWidgetItem* item,
                        const QPoint & globalPos)
{
    TreeItems::iterator it = _imp->findItem(item);

    if ( it == _imp->items.end() ) {
        return;
    }


    _imp->lastRightClickedItem = item;

    Natron::Menu menu(this);
    //menu.setFont( QFont(appFont,appFontSize) );
    menu.setShortcutEnabled(false);
    QAction* addLayerAct = menu.addAction( tr("Add layer") );
    QObject::connect( addLayerAct, SIGNAL( triggered() ), this, SLOT( onAddLayerActionTriggered() ) );
    QAction* deleteAct = menu.addAction( tr("Delete") );
    deleteAct->setShortcut( QKeySequence(Qt::Key_Backspace) );
    QObject::connect( deleteAct, SIGNAL( triggered() ), this, SLOT( onDeleteItemActionTriggered() ) );
    QAction* cutAct = menu.addAction( tr("Cut") );
    cutAct->setShortcut( QKeySequence(Qt::Key_X + Qt::CTRL) );
    QObject::connect( cutAct, SIGNAL( triggered() ), this, SLOT( onCutItemActionTriggered() ) );
    QAction* copyAct = menu.addAction( tr("Copy") );
    copyAct->setShortcut( QKeySequence(Qt::Key_C + Qt::CTRL) );
    QObject::connect( copyAct, SIGNAL( triggered() ), this, SLOT( onCopyItemActionTriggered() ) );
    QAction* pasteAct = menu.addAction( tr("Paste") );
    pasteAct->setShortcut( QKeySequence(Qt::Key_V + Qt::CTRL) );
    QObject::connect( pasteAct, SIGNAL( triggered() ), this, SLOT( onPasteItemActionTriggered() ) );
    pasteAct->setEnabled( !_imp->clipBoard.empty() );
    QAction* duplicateAct = menu.addAction( tr("Duplicate") );
    duplicateAct->setShortcut( QKeySequence(Qt::Key_C + Qt::ALT) );
    QObject::connect( duplicateAct, SIGNAL( triggered() ), this, SLOT( onDuplicateItemActionTriggered() ) );

    ///The base layer cannot be duplicated
    duplicateAct->setEnabled(it->rotoItem->getParentLayer() != NULL);

    menu.exec(globalPos);
}

void
RotoPanel::onAddLayerActionTriggered()
{
    assert(_imp->lastRightClickedItem);
    pushUndoCommand( new AddLayerUndoCommand(this) );
}

void
RotoPanel::onDeleteItemActionTriggered()
{
    assert(_imp->lastRightClickedItem);
    QList<QTreeWidgetItem*> selectedItems = _imp->tree->selectedItems();
    pushUndoCommand( new RemoveItemsUndoCommand(this,selectedItems) );
}

void
RotoPanel::onCutItemActionTriggered()
{
    assert(_imp->lastRightClickedItem);
    QList<QTreeWidgetItem*> selectedItems = _imp->tree->selectedItems();
    _imp->clipBoard = selectedItems;
    pushUndoCommand( new RemoveItemsUndoCommand(this,selectedItems) );
}

void
RotoPanel::onCopyItemActionTriggered()
{
    assert(_imp->lastRightClickedItem);
    _imp->clipBoard = _imp->tree->selectedItems();
}

void
RotoPanel::onPasteItemActionTriggered()
{
    assert( !_imp->clipBoard.empty() );
    boost::shared_ptr<RotoDrawableItem> drawable;
    {
        TreeItems::iterator it = _imp->findItem(_imp->lastRightClickedItem);
        if ( it == _imp->items.end() ) {
            return;
        }
        drawable = boost::dynamic_pointer_cast<RotoDrawableItem>(it->rotoItem);
    }


    ///make sure that if the item copied is only a bezier that we do not paste it on the same bezier.
    if (_imp->clipBoard.size() == 1) {
        TreeItems::iterator it = _imp->findItem( _imp->clipBoard.front() );
        if ( it == _imp->items.end() ) {
            return;
        }
        if (it->rotoItem == drawable) {
            return;
        }
    }

    if (drawable) {
        ///cannot paste multiple items on a drawable item
        if (_imp->clipBoard.size() > 1) {
            return;
        }

        ///cannot paste a non-drawable to a drawable
        TreeItems::iterator clip = _imp->findItem( _imp->clipBoard.front() );
        boost::shared_ptr<RotoDrawableItem> isClipBoardDrawable = boost::dynamic_pointer_cast<RotoDrawableItem>(clip->rotoItem);
        if (!isClipBoardDrawable) {
            return;
        }
    }

    pushUndoCommand( new PasteItemUndoCommand(this,_imp->lastRightClickedItem,_imp->clipBoard) );
}

void
RotoPanel::onDuplicateItemActionTriggered()
{
    pushUndoCommand( new DuplicateItemUndoCommand(this,_imp->lastRightClickedItem) );
}

void
RotoPanel::setLastRightClickedItem(QTreeWidgetItem* item)
{
    assert(item);
    _imp->lastRightClickedItem = item;
}

void
RotoPanel::selectAll()
{
    _imp->tree->blockSignals(true);
    for (TreeItems::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
        it->treeItem->setSelected(true);
    }
    _imp->tree->blockSignals(false);
    onItemSelectionChanged();
}

bool
RotoPanelPrivate::itemHasKey(const boost::shared_ptr<RotoItem>& item,
                             double time) const
{
    ItemKeys::const_iterator it = keyframes.find(item);

    if ( it != keyframes.end() ) {
        std::set<double>::const_iterator it2 = it->second.keys.find(time);
        if ( it2 != it->second.keys.end() ) {
            return true;
        }
    }

    return false;
}

void
RotoPanelPrivate::setItemKey(const boost::shared_ptr<RotoItem>& item,
                             double time)
{
    ItemKeys::iterator it = keyframes.find(item);

    if ( it != keyframes.end() ) {

        std::pair<std::set<double>::iterator,bool> ret = it->second.keys.insert(time);
        if (ret.second && it->second.visible) {
            node.lock()->getNode()->getApp()->addKeyframeIndicator(time);
        }
    } else {
        TimeLineKeys keys;
        keys.keys.insert(time);
        keys.visible = false;
        keyframes.insert( std::make_pair(item, keys) );
    }
}

void
RotoPanelPrivate::removeItemKey(const boost::shared_ptr<RotoItem>& item,
                                double time)
{
    ItemKeys::iterator it = keyframes.find(item);

    if ( it != keyframes.end() ) {

        std::set<double>::iterator it2 = it->second.keys.find(time);
        if ( it2 != it->second.keys.end() ) {
            it->second.keys.erase(it2);
            if (it->second.visible) {
                node.lock()->getNode()->getApp()->removeKeyFrameIndicator(time);
            }
        }
    }
}

void
RotoPanelPrivate::removeItemAnimation(const boost::shared_ptr<RotoItem>& item)
{
    ItemKeys::iterator it = keyframes.find(item);
    
    if ( it != keyframes.end() ) {
        std::list<SequenceTime> toRemove;

        for (std::set<double>::iterator it2 = it->second.keys.begin(); it2 != it->second.keys.end(); ++it2) {
            toRemove.push_back(*it2);
        }
        it->second.keys.clear();
        if (it->second.visible) {
            node.lock()->getNode()->getApp()->removeMultipleKeyframeIndicator(toRemove, true);
        }
    }
}


void
RotoPanelPrivate::setVisibleItemKeyframes(const std::set<double>& keyframes,bool visible, bool emitSignal)
{
    std::list<SequenceTime> keys;
    for (std::set<double>::iterator it2 = keyframes.begin(); it2 != keyframes.end(); ++it2) {
        keys.push_back(*it2);
    }
    if (!visible) {
        node.lock()->getNode()->getApp()->removeMultipleKeyframeIndicator(keys, emitSignal);
    } else {
        node.lock()->getNode()->getApp()->addMultipleKeyframeIndicatorsAdded(keys, emitSignal);
    }
}

void
RotoPanel::onSettingsPanelClosed(bool closed)
{
    boost::shared_ptr<TimeLine> timeline = getNode()->getNode()->getApp()->getTimeLine();
    if (closed) {
        ///remove all keyframes from the structure kept
        std::set< std::set<double> > toRemove;
        
        for (TreeItems::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
            boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(it->rotoItem);
            if (isBezier) {
                ItemKeys::iterator it2 = _imp->keyframes.find(isBezier);
                if ( it2 != _imp->keyframes.end() && it2->second.visible) {
                    it2->second.visible = false;
                    toRemove.insert(it2->second.keys);
                }
            }
        }
        
        std::set<std::set<double> >::iterator next = toRemove.begin();
        if (next != toRemove.end()) {
            ++next;
        }
        for (std::set<std::set<double> >::iterator it = toRemove.begin(); it != toRemove.end(); ++it) {
            _imp->setVisibleItemKeyframes(*it, false, next == toRemove.end());
            if (next != toRemove.end()) {
                ++next;
            }
        }
        _imp->keyframes.clear();
    } else {
        ///rebuild all the keyframe structure
        std::set< std::set<double> > toAdd;
        for (TreeItems::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
            boost::shared_ptr<Bezier> isBezier = boost::dynamic_pointer_cast<Bezier>(it->rotoItem);
            if (isBezier) {
                assert ( _imp->keyframes.find(isBezier) == _imp->keyframes.end() );
                TimeLineKeys keys;
                isBezier->getKeyframeTimes(&keys.keys);
                keys.visible = false;
                std::list<SequenceTime> markers;
                for (std::set<double>::iterator it3 = keys.keys.begin(); it3 != keys.keys.end(); ++it3) {
                    markers.push_back(*it3);
                }

                std::pair<ItemKeys::iterator,bool> ret = _imp->keyframes.insert( std::make_pair(isBezier, keys) );
                assert(ret.second);
                
                ///If the item is selected, make its keyframes visible
                for (SelectedItems::iterator it2 = _imp->selectedItems.begin() ; it2 != _imp->selectedItems.end(); ++it2) {
                    if (it2->get() == isBezier.get()) {
                        toAdd.insert(keys.keys);
                        ret.first->second.visible = true;
                        break;
                    }
                }
            }
        }
        std::set<std::set<double> >::iterator next = toAdd.begin();
        if (next != toAdd.end()) {
            ++next;
        }
        for (std::set<std::set<double> >::iterator it = toAdd.begin(); it != toAdd.end(); ++it) {
            _imp->setVisibleItemKeyframes(*it, true, next == toAdd.end());
            if (next != toAdd.end()) {
                ++next;
            }
        }
    }
}

void
RotoPanel::onOperatorColMinimumSizeChanged(const QSize& size)
{
    
#if QT_VERSION < 0x050000
    _imp->tree->header()->setResizeMode(QHeaderView::Fixed);
#else
    _imp->tree->header()->setSectionResizeMode(QHeaderView::Fixed);
#endif
    _imp->tree->setColumnWidth(COL_OPERATOR, size.width());
#if QT_VERSION < 0x050000
    _imp->tree->header()->setResizeMode(QHeaderView::ResizeToContents);
#else
    _imp->tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
}

#include "moc_RotoPanel.cpp"
