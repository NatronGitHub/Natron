/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include <stdexcept>
#include <sstream> // stringstream

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
#include <QtCore/QDataStream>
#include <QtCore/QMimeData>
#include <QImage>
#include <QPainter>
#include <QUndoCommand>
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
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewIdx.h"

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
#include "Gui/SpinBox.h"

#define COL_LABEL 0
#define COL_ACTIVATED 1
#define COL_LOCKED 2
#define COL_OPERATOR 3
#define COL_OVERLAY 4
#define COL_COLOR 5


#ifdef NATRON_ROTO_INVERTIBLE
#define COL_INVERTED 6
#define MAX_COLS 7
#else
#define MAX_COLS 6
#endif

NATRON_NAMESPACE_ENTER


class RemoveItemsUndoCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(RemoveItemsUndoCommand)

    struct RemovedItem
    {
        QTreeWidgetItem* treeItem;
        QTreeWidgetItem* parentTreeItem;
        RotoLayerPtr parentLayer;
        int indexInLayer;
        RotoItemPtr item;

        RemovedItem()
            : treeItem(0)
            , parentTreeItem(0)
            , parentLayer()
            , indexInLayer(-1)
            , item()
        {
        }
    };

public:


    RemoveItemsUndoCommand(RotoPanel* roto,
                           const QList<QTreeWidgetItem*> & items);

    virtual ~RemoveItemsUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    RotoPanel* _roto;
    std::list<RemovedItem> _items;
};

class AddLayerUndoCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(AddLayerUndoCommand)

public:


    AddLayerUndoCommand(RotoPanel* roto);

    virtual ~AddLayerUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    RotoPanel* _roto;
    bool _firstRedoCalled;
    RotoLayerPtr _parentLayer;
    QTreeWidgetItem* _parentTreeItem;
    QTreeWidgetItem* _treeItem;
    RotoLayerPtr _layer;
    int _indexInParentLayer;
};


class DragItemsUndoCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(DragItemsUndoCommand)

public:

    struct Item
    {
        DroppedTreeItemPtr dropped;
        RotoLayerPtr oldParentLayer;
        int indexInOldLayer;
        QTreeWidgetItem* oldParentItem;
    };


    DragItemsUndoCommand(RotoPanel* roto,
                         const std::list<DroppedTreeItemPtr> & items);

    virtual ~DragItemsUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    RotoPanel* _roto;
    std::list<Item > _items;
};


/**
 * @class This class supports 2 behaviours:
 * 1) The user pastes one item upon another. The target's shape and attributes are copied and the
 * name is the source's name plus "- copy" at the end.
 * 2) The user pastes several items upon a layer in which case the items are copied into that layer and
 * the new items name is the same than the original appended with "- copy".
 *
 * Anything else will not do anything and you should not issue a command which will yield an unsupported behaviour
 * otherwise you'll create an empty action in the undo/redo stack.
 **/
class PasteItemUndoCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(PasteItemUndoCommand)

public:

    enum PasteModeEnum
    {
        ePasteModeCopyToLayer = 0,
        ePasteModeCopyToItem
    };

    struct PastedItem
    {
        QTreeWidgetItem* treeItem;
        RotoItemPtr rotoItem;
        RotoItemPtr itemCopy;
    };


    PasteItemUndoCommand(RotoPanel* roto,
                         QTreeWidgetItem* target,
                         QList<QTreeWidgetItem*> source);

    virtual ~PasteItemUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    RotoPanel* _roto;
    PasteModeEnum _mode;
    QTreeWidgetItem* _targetTreeItem;
    RotoItemPtr _targetItem;
    RotoItemPtr _oldTargetItem;
    std::list<PastedItem > _pastedItems;
};


class DuplicateItemUndoCommand
    : public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(DuplicateItemUndoCommand)

public:

    struct DuplicatedItem
    {
        QTreeWidgetItem* treeItem;
        RotoItemPtr item;
        RotoItemPtr duplicatedItem;
    };

    DuplicateItemUndoCommand(RotoPanel* roto,
                             QTreeWidgetItem* items);

    virtual ~DuplicateItemUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    RotoPanel* _roto;
    DuplicatedItem _item;
};


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
            _panel->showItemMenu( item, e->globalPos() );
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
    RotoItemPtr rotoItem;

    TreeItem()
        : treeItem(0), rotoItem()
    {
    }

    TreeItem(QTreeWidgetItem* t,
             const RotoItemPtr & r)
        : treeItem(t), rotoItem(r)
    {
    }
};

typedef std::list<TreeItem > TreeItems;
typedef std::list<RotoItemPtr> SelectedItems;

struct TimeLineKeys
{
    std::set<double> keys;
    bool visible;
};

typedef std::map<RotoItemPtr, TimeLineKeys > ItemKeys;

enum ColorDialogEditingEnum
{
    eColorDialogEditingNothing = 0,
    eColorDialogEditingOverlayColor,
    eColorDialogEditingShapeColor
};

struct RotoPanelPrivate
{
    Q_DECLARE_TR_FUNCTIONS(RotoPanel)

public:
    RotoPanel* publicInterface;
    NodeGuiWPtr node;
    RotoContextPtr context;
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
    QIcon iconLayer, iconBezier, iconVisible, iconUnvisible, iconLocked, iconUnlocked, iconInverted, iconUninverted, iconWheel;
    QIcon iconStroke, iconEraser, iconSmear, iconSharpen, iconBlur, iconClone, iconReveal, iconDodge, iconBurn;
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
                     const NodeGuiPtr&   n)
        : publicInterface(publicInter)
        , node(n)
        , context( n->getNode()->getRotoContext() )
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

    TreeItems::iterator findItem(const RotoItemPtr& item)
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

    void insertItemRecursively(double time, const RotoItemPtr& item, int indexInParentLayer);

    void removeItemRecursively(const RotoItemPtr& item);

    void insertSelectionRecursively(const RotoLayerPtr & layer);

    void setChildrenLockedRecursively(bool locked, QTreeWidgetItem* item);

    bool itemHasKey(const RotoItemPtr& item, double time) const;

    void setItemKey(const RotoItemPtr& item, double time);

    void removeItemKey(const RotoItemPtr& item, double time);

    void removeItemAnimation(const RotoItemPtr& item);

    void insertItemInternal(int reason, double time, const RotoItemPtr& item, int indexInParentLayer);

    void setVisibleItemKeyframes(const std::set<double>& keys, bool visible, bool emitSignal);
};

RotoPanel::RotoPanel(const NodeGuiPtr&  n,
                     QWidget* parent)
    : QWidget(parent)
    , _imp( new RotoPanelPrivate(this, n) )
{
    QObject::connect( _imp->context.get(), SIGNAL(selectionChanged(int)), this, SLOT(onSelectionChanged(int)) );
    QObject::connect( _imp->context.get(), SIGNAL(itemInserted(int,int)), this, SLOT(onItemInserted(int,int)) );
    QObject::connect( _imp->context.get(), SIGNAL(itemRemoved(RotoItemPtr,int)), this, SLOT(onItemRemoved(RotoItemPtr,int)) );
    QObject::connect( n->getNode()->getApp()->getTimeLine().get(), SIGNAL(frameChanged(SequenceTime,int)), this,
                      SLOT(onTimeChanged(SequenceTime,int)) );
    QObject::connect( n.get(), SIGNAL(settingsPanelClosed(bool)), this, SLOT(onSettingsPanelClosed(bool)) );
    QObject::connect( _imp->context.get(), SIGNAL(itemScriptNameChanged(RotoItemPtr)), this, SLOT(onItemScriptNameChanged(RotoItemPtr)) );
    QObject::connect( _imp->context.get(), SIGNAL(itemLabelChanged(RotoItemPtr)), this, SLOT(onItemLabelChanged(RotoItemPtr)) );
    QObject::connect( _imp->context.get(), SIGNAL(itemLockedChanged(int)), this, SLOT(onItemLockChanged(int)) );
    QObject::connect( _imp->context.get(), SIGNAL(itemGloballyActivatedChanged(RotoItemPtr)), this, SLOT(onItemGloballyActivatedChanged(RotoItemPtr)) );
    const QSize medButtonSize( TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE) );
    const QSize medButtonIconSize( TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE) );


    _imp->mainLayout = new QVBoxLayout(this);

    _imp->splineContainer = new QWidget(this);
    _imp->mainLayout->addWidget(_imp->splineContainer);

    _imp->splineLayout = new QHBoxLayout(_imp->splineContainer);
    _imp->splineLayout->setSpacing(2);
    _imp->splineLabel = new ClickableLabel(tr("Spline keyframe:"), _imp->splineContainer);
    _imp->splineLabel->setSunken(false);
    _imp->splineLabel->setEnabled(false);
    _imp->splineLayout->addWidget(_imp->splineLabel);

    _imp->currentKeyframe = new SpinBox(_imp->splineContainer, SpinBox::eSpinBoxTypeDouble);
    _imp->currentKeyframe->setEnabled(false);
    _imp->currentKeyframe->setReadOnly_NoFocusRect(true);
    _imp->currentKeyframe->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("The current keyframe for the selected shape(s)."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->splineLayout->addWidget(_imp->currentKeyframe);

    _imp->ofLabel = new ClickableLabel(QString::fromUtf8("of"), _imp->splineContainer);
    _imp->ofLabel->setEnabled(false);
    _imp->splineLayout->addWidget(_imp->ofLabel);

    _imp->totalKeyframes = new SpinBox(_imp->splineContainer, SpinBox::eSpinBoxTypeInt);
    _imp->totalKeyframes->setEnabled(false);
    _imp->totalKeyframes->setReadOnly_NoFocusRect(true);
    _imp->totalKeyframes->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("The keyframe count for all the selected shapes."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->splineLayout->addWidget(_imp->totalKeyframes);

    int medIconSize = TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE);
    QPixmap prevPix, nextPix, addPix, removePix, clearAnimPix;
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PREVIOUS_KEY, medIconSize, &prevPix);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_NEXT_KEY, medIconSize, &nextPix);
    appPTR->getIcon(NATRON_PIXMAP_ADD_KEYFRAME, medIconSize, &addPix);
    appPTR->getIcon(NATRON_PIXMAP_REMOVE_KEYFRAME, medIconSize, &removePix);
    appPTR->getIcon(NATRON_PIXMAP_CLEAR_ALL_ANIMATION, medIconSize, &clearAnimPix);

    _imp->prevKeyframe = new Button(QIcon(prevPix), QString(), _imp->splineContainer);
    _imp->prevKeyframe->setFixedSize(medButtonSize);
    _imp->prevKeyframe->setIconSize(medButtonIconSize);
    _imp->prevKeyframe->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Go to the previous keyframe."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->prevKeyframe->setEnabled(false);
    QObject::connect( _imp->prevKeyframe, SIGNAL(clicked(bool)), this, SLOT(onGoToPrevKeyframeButtonClicked()) );
    _imp->splineLayout->addWidget(_imp->prevKeyframe);

    _imp->nextKeyframe = new Button(QIcon(nextPix), QString(), _imp->splineContainer);
    _imp->nextKeyframe->setFixedSize(medButtonSize);
    _imp->nextKeyframe->setIconSize(medButtonIconSize);
    _imp->nextKeyframe->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Go to the next keyframe."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->nextKeyframe->setEnabled(false);
    QObject::connect( _imp->nextKeyframe, SIGNAL(clicked(bool)), this, SLOT(onGoToNextKeyframeButtonClicked()) );
    _imp->splineLayout->addWidget(_imp->nextKeyframe);

    _imp->addKeyframe = new Button(QIcon(addPix), QString(), _imp->splineContainer);
    _imp->addKeyframe->setFixedSize(medButtonSize);
    _imp->addKeyframe->setIconSize(medButtonIconSize);
    _imp->addKeyframe->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Add keyframe at the current timeline's time."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->addKeyframe->setEnabled(false);
    QObject::connect( _imp->addKeyframe, SIGNAL(clicked(bool)), this, SLOT(onAddKeyframeButtonClicked()) );
    _imp->splineLayout->addWidget(_imp->addKeyframe);

    _imp->removeKeyframe = new Button(QIcon(removePix), QString(), _imp->splineContainer);
    _imp->removeKeyframe->setFixedSize(medButtonSize);
    _imp->removeKeyframe->setIconSize(medButtonIconSize);
    _imp->removeKeyframe->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Remove keyframe at the current timeline's time."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->removeKeyframe->setEnabled(false);
    QObject::connect( _imp->removeKeyframe, SIGNAL(clicked(bool)), this, SLOT(onRemoveKeyframeButtonClicked()) );
    _imp->splineLayout->addWidget(_imp->removeKeyframe);

    _imp->clearAnimation = new Button(QIcon(clearAnimPix), QString(), _imp->splineContainer);
    _imp->clearAnimation->setFixedSize(medButtonSize);
    _imp->clearAnimation->setIconSize(medButtonIconSize);
    _imp->clearAnimation->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Remove all animation for the selected shape(s)."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->clearAnimation->setEnabled(false);
    QObject::connect( _imp->clearAnimation, SIGNAL(clicked(bool)), this, SLOT(onRemoveAnimationButtonClicked()) );
    _imp->splineLayout->addWidget(_imp->clearAnimation);


    _imp->splineLayout->addStretch();


    _imp->tree = new TreeWidget(this, this);
    _imp->tree->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    _imp->tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    _imp->tree->setDragDropMode(QAbstractItemView::InternalMove);
    _imp->tree->setDragEnabled(true);
    _imp->tree->setExpandsOnDoubleClick(false);
    _imp->tree->setAttribute(Qt::WA_MacShowFocusRect, 0);
    QString treeToolTip = NATRON_NAMESPACE::convertFromPlainText(tr("This tree contains the hierarchy of shapes, strokes and layers along with some "
                                                            "most commonly used attributes for each of them. "
                                                            "Each attribute can be found in the parameters above in the panel.\n"
                                                            "You can reorder items by drag and dropping them and can also right click "
                                                            "each item for more options.\n"
                                                            "The items are rendered from bottom to top always, so that the first shape in "
                                                            "this list will always be the last one rendered "
                                                            "(generally on top of everything else)."), NATRON_NAMESPACE::WhiteSpaceNormal);
    _imp->tree->setToolTip(treeToolTip);

    _imp->mainLayout->addWidget(_imp->tree);

    QObject::connect( _imp->tree, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(onItemClicked(QTreeWidgetItem*,int)) );
    QObject::connect( _imp->tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(onItemDoubleClicked(QTreeWidgetItem*,int)) );
    QObject::connect( _imp->tree, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(onItemChanged(QTreeWidgetItem*,int)) );
    QObject::connect( _imp->tree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this,
                      SLOT(onCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)) );
    QObject::connect( _imp->tree, SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelectionChanged()) );


    _imp->tree->setColumnCount(MAX_COLS);
    _imp->treeHeader = new QTreeWidgetItem;
    _imp->treeHeader->setText( COL_LABEL, tr("Label") );

    QPixmap pixLayer, pixBezier, pixVisible, pixUnvisible, pixLocked, pixUnlocked, pixInverted, pixUninverted, pixWheel, pixDefault, pixmerge;
    QPixmap pixPaintBrush, pixEraser, pixBlur, pixSmear, pixSharpen, pixDodge, pixBurn, pixClone, pixReveal;
    appPTR->getIcon(NATRON_PIXMAP_LAYER, medIconSize, &pixLayer);
    appPTR->getIcon(NATRON_PIXMAP_BEZIER, medIconSize, &pixBezier);
    appPTR->getIcon(NATRON_PIXMAP_VISIBLE, medIconSize, &pixVisible);
    appPTR->getIcon(NATRON_PIXMAP_UNVISIBLE, medIconSize, &pixUnvisible);
    appPTR->getIcon(NATRON_PIXMAP_LOCKED, medIconSize, &pixLocked);
    appPTR->getIcon(NATRON_PIXMAP_UNLOCKED, medIconSize, &pixUnlocked);
    appPTR->getIcon(NATRON_PIXMAP_INVERTED, medIconSize, &pixInverted);
    appPTR->getIcon(NATRON_PIXMAP_UNINVERTED, medIconSize, &pixUninverted);
    appPTR->getIcon(NATRON_PIXMAP_COLORWHEEL, medIconSize, &pixWheel);
    appPTR->getIcon(NATRON_PIXMAP_ROTO_MERGE, medIconSize, &pixmerge);
    appPTR->getIcon(NATRON_PIXMAP_OVERLAY, medIconSize, &pixDefault);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_SOLID, medIconSize, &pixPaintBrush);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_ERASER, medIconSize, &pixEraser);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_BLUR, medIconSize, &pixBlur);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_SMEAR, medIconSize, &pixSmear);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_SHARPEN, medIconSize, &pixSharpen);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_DODGE, medIconSize, &pixDodge);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_BURN, medIconSize, &pixBurn);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_CLONE, medIconSize, &pixClone);
    appPTR->getIcon(NATRON_PIXMAP_ROTOPAINT_REVEAL, medIconSize, &pixReveal);

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
    _imp->treeHeader->setIcon( COL_OVERLAY, QIcon(pixDefault) );
    _imp->treeHeader->setIcon(COL_COLOR, _imp->iconWheel);
#ifdef NATRON_ROTO_INVERTIBLE
    _imp->treeHeader->setIcon(COL_INVERTED, _imp->iconUninverted);
#endif
    _imp->treeHeader->setIcon( COL_OPERATOR, QIcon(pixmerge) );
    _imp->tree->setHeaderItem(_imp->treeHeader);
    _imp->tree->setMinimumHeight( TO_DPIY(350) );
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    _imp->tree->header()->setResizeMode(QHeaderView::ResizeToContents);
#else
    _imp->tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
    _imp->buttonContainer = new QWidget(this);
    _imp->buttonLayout = new QHBoxLayout(_imp->buttonContainer);
    _imp->buttonLayout->setContentsMargins(0, 0, 0, 0);
    _imp->buttonLayout->setSpacing(0);

    _imp->addLayerButton = new Button(QString::fromUtf8("+"), _imp->buttonContainer);
    _imp->addLayerButton->setFixedSize(medButtonSize);
    _imp->addLayerButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Add a new layer."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->buttonLayout->addWidget(_imp->addLayerButton);
    QObject::connect( _imp->addLayerButton, SIGNAL(clicked(bool)), this, SLOT(onAddLayerButtonClicked()) );

    _imp->removeItemButton = new Button(QString::fromUtf8("-"), _imp->buttonContainer);
    _imp->removeItemButton->setFixedSize(medButtonSize);
    _imp->removeItemButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Remove selected items."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->buttonLayout->addWidget(_imp->removeItemButton);
    QObject::connect( _imp->removeItemButton, SIGNAL(clicked(bool)), this, SLOT(onRemoveItemButtonClicked()) );

    _imp->buttonLayout->addStretch();

    _imp->mainLayout->addWidget(_imp->buttonContainer);

    _imp->buildTreeFromContext();

    ///refresh selection
    onSelectionChanged(RotoItem::eSelectionReasonOther);
}

RotoPanel::~RotoPanel()
{
}

RotoItemPtr RotoPanel::getRotoItemForTreeItem(QTreeWidgetItem* treeItem) const
{
    TreeItems::iterator it = _imp->findItem(treeItem);

    if ( it != _imp->items.end() ) {
        return it->rotoItem;
    }

    return RotoItemPtr();
}

QTreeWidgetItem*
RotoPanel::getTreeItemForRotoItem(const RotoItemPtr & item) const
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
        BezierPtr isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        if (isBezier) {
            QObject::disconnect( isBezier.get(), SIGNAL(keyframeSet(double)), this, SLOT(onSelectedBezierKeyframeSet(double)) );
            QObject::disconnect( isBezier.get(), SIGNAL(keyframeRemoved(double)), this, SLOT(onSelectedBezierKeyframeRemoved(double)) );
            QObject::disconnect( isBezier.get(), SIGNAL(animationRemoved()), this, SLOT(onSelectedBezierAnimationRemoved()) );
            QObject::disconnect( isBezier.get(), SIGNAL(aboutToClone()), this, SLOT(onSelectedBezierAboutToClone()) );
            QObject::disconnect( isBezier.get(), SIGNAL(cloned()), this, SLOT(onSelectedBezierCloned()) );
        }
        ItemKeys::iterator found = _imp->keyframes.find(*it);
        if ( found != _imp->keyframes.end() ) {
            toRemove.push_back(found->second.keys);
            found->second.visible = false;
        }
    }
    _imp->selectedItems.clear();

    ///Remove previous selection's keyframes
    {
        std::list<std::set<double> >::iterator next = toRemove.begin();
        if ( next != toRemove.end() ) {
            ++next;
        }
        for (std::list<std::set<double> >::iterator it = toRemove.begin(); it != toRemove.end(); ++it) {
            _imp->setVisibleItemKeyframes( *it, false, next == toRemove.end() );
            if ( next != toRemove.end() ) {
                ++next;
            }
        }
    }

    ///connect new selection

    std::list<std::set<double> > toAdd;
    int selectedBeziersCount = 0;
    const std::list<RotoItemPtr> & items = _imp->context->getSelectedItems();
    for (std::list<RotoItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        _imp->selectedItems.push_back(*it);
        BezierPtr isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        const RotoLayer* isLayer = dynamic_cast<const RotoLayer*>( it->get() );
        if (isBezier) {
            QObject::connect( isBezier.get(), SIGNAL(keyframeSet(double)), this, SLOT(onSelectedBezierKeyframeSet(double)) );
            QObject::connect( isBezier.get(), SIGNAL(keyframeRemoved(double)), this, SLOT(onSelectedBezierKeyframeRemoved(double)) );
            QObject::connect( isBezier.get(), SIGNAL(animationRemoved()), this, SLOT(onSelectedBezierAnimationRemoved()) );
            QObject::connect( isBezier.get(), SIGNAL(aboutToClone()), this, SLOT(onSelectedBezierAboutToClone()) );
            QObject::connect( isBezier.get(), SIGNAL(cloned()), this, SLOT(onSelectedBezierCloned()) );
            ++selectedBeziersCount;
        } else if ( isLayer && !isLayer->getItems().empty() ) {
            ++selectedBeziersCount;
        }
        ItemKeys::iterator found = _imp->keyframes.find(*it);
        if ( found != _imp->keyframes.end() ) {
            toAdd.push_back(found->second.keys);
            found->second.visible = true;
        }
    }

    ///Add new selection keyframes
    {
        std::list<std::set<double> >::iterator next = toAdd.begin();
        if ( next != toAdd.end() ) {
            ++next;
        }
        for (std::list<std::set<double> >::iterator it = toAdd.begin(); it != toAdd.end(); ++it) {
            _imp->setVisibleItemKeyframes( *it, true, next == toAdd.end() );
            if ( next != toAdd.end() ) {
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
} // RotoPanel::onSelectionChangedInternal

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
    BezierPtr isBezier;

    if (b) {
        isBezier = boost::dynamic_pointer_cast<Bezier>( b->shared_from_this() );
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
    BezierPtr isBezier;

    if (b) {
        isBezier = boost::dynamic_pointer_cast<Bezier>( b->shared_from_this() );
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
    BezierPtr isBezier;

    if (b) {
        isBezier = boost::dynamic_pointer_cast<Bezier>( b->shared_from_this() );
    }

    _imp->updateSplinesInfoGUI( _imp->context->getTimelineCurrentTime() );
    if (isBezier) {
        _imp->removeItemAnimation(isBezier);
    }
}

void
RotoPanel::onSelectedBezierAboutToClone()
{
    Bezier* b = qobject_cast<Bezier*>( sender() );
    BezierPtr isBezier;

    if (b) {
        isBezier = boost::dynamic_pointer_cast<Bezier>( b->shared_from_this() );
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
    BezierPtr isBezier;

    if (b) {
        isBezier = boost::dynamic_pointer_cast<Bezier>( b->shared_from_this() );
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
    QPixmap p(15, 15);
    QColor c;

    c.setRgbF( Image::clamp<qreal>(color[0], 0., 1.),
               Image::clamp<qreal>(color[1], 0., 1.),
               Image::clamp<qreal>(color[2], 0., 1.) );
    p.fill(c);
    icon.addPixmap(p);
}

void
RotoPanel::updateItemGui(QTreeWidgetItem* item)
{
    double time = _imp->context->getTimelineCurrentTime();
    TreeItems::iterator it = _imp->findItem(item);

    assert( it != _imp->items.end() );
    it->treeItem->setIcon(COL_ACTIVATED, it->rotoItem->isGloballyActivated() ? _imp->iconVisible : _imp->iconUnvisible);
    it->treeItem->setIcon(COL_LOCKED, it->rotoItem->isLockedRecursive() ? _imp->iconLocked : _imp->iconUnlocked);

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
        it->treeItem->setIcon(COL_INVERTED, drawable->getInverted(time) ? _imp->iconInverted : _imp->iconUninverted);
#endif
        QWidget* w = _imp->tree->itemWidget(it->treeItem, COL_OPERATOR);
        assert(w);
        ComboBox* cb = dynamic_cast<ComboBox*>(w);
        assert(cb);
        if (cb) {
            cb->setCurrentIndex_no_emit( drawable->getCompositingOperator() );
        }
    }
}

void
RotoPanelPrivate::updateSplinesInfoGUI(double time)
{
    std::set<double> keyframes;

    for (SelectedItems::const_iterator it = selectedItems.begin(); it != selectedItems.end(); ++it) {
        BezierPtr isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
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
                if ( prev != keyframes.begin() ) {
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

static QString
scriptNameToolTipFromItem(const RotoItemPtr& item)
{
    return ( QString::fromUtf8("<p><b>")
             + QString::fromUtf8( item->getScriptName().c_str() )
             + QString::fromUtf8("</b></p>")
             +  NATRON_NAMESPACE::convertFromPlainText(QCoreApplication::translate("RotoPanel", "The label of the item."), NATRON_NAMESPACE::WhiteSpaceNormal) );
}

void
RotoPanelPrivate::insertItemRecursively(double time,
                                        const RotoItemPtr & item,
                                        int indexInParentLayer)
{
    QTreeWidgetItem* treeItem = new QTreeWidgetItem;
    RotoLayerPtr parent = item->getParentLayer();

    if (parent) {
        TreeItems::iterator parentIT = findItem(parent);

        ///the parent must have already been inserted!
        assert( parentIT != items.end() );
        parentIT->treeItem->insertChild(indexInParentLayer, treeItem);
    } else {
        tree->insertTopLevelItem(0, treeItem);
    }
    items.push_back( TreeItem(treeItem, item) );

    treeItem->setText( COL_LABEL, QString::fromUtf8( item->getLabel().c_str() ) );
    treeItem->setToolTip( COL_LABEL, scriptNameToolTipFromItem(item) );


    treeItem->setIcon(COL_ACTIVATED, item->isGloballyActivated() ? iconVisible : iconUnvisible);
    treeItem->setToolTip( COL_ACTIVATED, NATRON_NAMESPACE::convertFromPlainText(tr("Controls whether the overlay should be visible on the viewer for "
                                                                           "the shape."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    treeItem->setIcon(COL_LOCKED, item->getLocked() ? iconLocked : iconUnlocked);
    treeItem->setToolTip( COL_LOCKED, NATRON_NAMESPACE::convertFromPlainText(tr(kRotoLockedHint), NATRON_NAMESPACE::WhiteSpaceNormal) );

    RotoDrawableItemPtr drawable = boost::dynamic_pointer_cast<RotoDrawableItem>(item);
    RotoLayerPtr layer = boost::dynamic_pointer_cast<RotoLayer>(item);

    if (drawable) {
        RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>( drawable.get() );
        double overlayColor[4];
        drawable->getOverlayColor(overlayColor);
        QIcon overlayIcon;
        makeSolidIcon(overlayColor, overlayIcon);
        if (!isStroke) {
            treeItem->setIcon(COL_LABEL, iconBezier);
        } else {
            switch ( isStroke->getBrushType() ) {
            case eRotoStrokeTypeBlur:
                treeItem->setIcon(COL_LABEL, iconBlur);
                break;
            case eRotoStrokeTypeBurn:
                treeItem->setIcon(COL_LABEL, iconBurn);
                break;
            case eRotoStrokeTypeClone:
                treeItem->setIcon(COL_LABEL, iconClone);
                break;
            case eRotoStrokeTypeDodge:
                treeItem->setIcon(COL_LABEL, iconDodge);
                break;
            case eRotoStrokeTypeEraser:
                treeItem->setIcon(COL_LABEL, iconEraser);
                break;
            case eRotoStrokeTypeReveal:
                treeItem->setIcon(COL_LABEL, iconReveal);
                break;
            case eRotoStrokeTypeSharpen:
                treeItem->setIcon(COL_LABEL, iconSharpen);
                break;
            case eRotoStrokeTypeSmear:
                treeItem->setIcon(COL_LABEL, iconSmear);
                break;
            case eRotoStrokeTypeSolid:
                treeItem->setIcon(COL_LABEL, iconStroke);
                break;
            }
        }
        treeItem->setIcon(COL_OVERLAY, overlayIcon);
        treeItem->setToolTip( COL_OVERLAY, NATRON_NAMESPACE::convertFromPlainText(tr(kRotoOverlayHint), NATRON_NAMESPACE::WhiteSpaceNormal) );
        double shapeColor[3];
        drawable->getColor(time, shapeColor);
        QIcon shapeIcon;
        makeSolidIcon(shapeColor, shapeIcon);
        treeItem->setIcon(COL_COLOR, shapeIcon);
        treeItem->setToolTip( COL_COLOR, NATRON_NAMESPACE::convertFromPlainText(tr(kRotoColorHint), NATRON_NAMESPACE::WhiteSpaceNormal) );
#ifdef NATRON_ROTO_INVERTIBLE
        treeItem->setIcon(COL_INVERTED, drawable->getInverted(time)  ? iconInverted : iconUninverted);
        treeItem->setToolTip( COL_INVERTED, NATRON_NAMESPACE::convertFromPlainText(tr(kRotoInvertedHint), NATRON_NAMESPACE::WhiteSpaceNormal) );
#endif

        publicInterface->makeCustomWidgetsForItem(drawable, treeItem);
        QObject::connect( drawable.get(), SIGNAL(invertedStateChanged()), publicInterface, SLOT(onRotoItemInvertedStateChanged()) );
        QObject::connect( drawable.get(),
                          SIGNAL(shapeColorChanged()),
                          publicInterface,
                          SLOT(onRotoItemShapeColorChanged()) );
        QObject::connect( drawable.get(),
                          SIGNAL(compositingOperatorChanged(ViewSpec,int,int)),
                          publicInterface,
                          SLOT(onRotoItemCompOperatorChanged(ViewSpec,int,int)) );
        BezierPtr isBezier = boost::dynamic_pointer_cast<Bezier>(item);
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
        const std::list<RotoItemPtr> & children = layer->getItems();
        int i = 0;
        for (std::list<RotoItemPtr>::const_iterator it = children.begin(); it != children.end(); ++it, ++i) {
            insertItemRecursively(time, *it, i);
        }
    }
    expandRecursively(treeItem);
} // insertItemRecursively

NodeGuiPtr
RotoPanel::getNode() const
{
    return _imp->node.lock();
}

void
RotoPanel::makeCustomWidgetsForItem(const RotoDrawableItemPtr& item,
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
    cb->setFocusPolicy(Qt::NoFocus);
    QObject::connect( cb, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentItemCompOperatorChanged(int)) );
    std::vector<ChoiceOption> compositingOperators;
    Merge::getOperatorStrings(&compositingOperators);
    for (U32 i = 0; i < compositingOperators.size(); ++i) {
        cb->addItem( QString::fromUtf8( compositingOperators[i].id.c_str() ), QIcon(), QKeySequence(), QString::fromUtf8( compositingOperators[i].tooltip.c_str() ) );
    }
    // set the tooltip
    const std::string & tt = item->getCompositingOperatorToolTip();

    cb->setToolTip( NATRON_NAMESPACE::convertFromPlainText(QString::fromUtf8( tt.c_str() ).trimmed(), NATRON_NAMESPACE::WhiteSpaceNormal) );
    cb->setCurrentIndex_no_emit( item->getCompositingOperator() );
    QObject::connect( cb, SIGNAL(minimumSizeChanged(QSize)), this, SLOT(onOperatorColMinimumSizeChanged(QSize)) );
    _imp->tree->setItemWidget(treeItem, COL_OPERATOR, cb);

    //We must call this otherwise this is never called by Qt for custom widgets (this is a Qt bug)
    QSize s = cb->minimumSizeHint();
    Q_UNUSED(s);
}

void
RotoPanelPrivate::removeItemRecursively(const RotoItemPtr& item)
{
    TreeItems::iterator it = findItem(item);

    if ( it == items.end() ) {
        return;
    }
    RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>( item.get() );
    if (drawable) {
        QObject::disconnect( drawable, SIGNAL(invertedStateChanged()), publicInterface, SLOT(onRotoItemInvertedStateChanged()) );
    }

    ///deleting the item will Q_EMIT a selection change which would lead to a deadlock
    tree->blockSignals(true);
    delete it->treeItem;
    tree->blockSignals(false);
    items.erase(it);
}

void
RotoPanel::onItemInserted(int index,
                          int reason)
{
    RotoItemPtr lastInsertedItem = _imp->context->getLastInsertedItem();
    double time = _imp->context->getTimelineCurrentTime();

    _imp->insertItemInternal(reason, time, lastInsertedItem, index);
}

void
RotoPanelPrivate::insertItemInternal(int reason,
                                     double time,
                                     const RotoItemPtr & item,
                                     int indexInParentLayer)
{
    BezierPtr isBezier = boost::dynamic_pointer_cast<Bezier>(item);

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
        RotoDrawableItemPtr drawable = boost::dynamic_pointer_cast<RotoDrawableItem>(item);
        if (drawable) {
            publicInterface->makeCustomWidgetsForItem(drawable);
        }

        return;
    }
    assert(item);
    insertItemRecursively(time, item, indexInParentLayer);
}

void
RotoPanel::onItemRemoved(const RotoItemPtr& item,
                         int reason)
{
    Bezier* b = qobject_cast<Bezier*>( sender() );
    BezierPtr isBezier;

    if (b) {
        isBezier = boost::dynamic_pointer_cast<Bezier>( b->shared_from_this() );
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
    const std::list<RotoLayerPtr> & layers = context->getLayers();

    tree->blockSignals(true);
    if ( !layers.empty() ) {
        const RotoLayerPtr & base = layers.front();
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
        if (_imp->tree->itemWidget(it->treeItem, COL_OPERATOR) == comboboxSender) {
            RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>( it->rotoItem.get() );
            assert(drawable);
            if (drawable) {
                KnobChoicePtr op = drawable->getOperatorKnob();
                KeyFrame k;
                op->setValue(index, ViewIdx(0), 0, eValueChangedReasonUserEdited, &k);
            }
            _imp->context->clearSelection(RotoItem::eSelectionReasonOther);
            _imp->context->select(it->rotoItem, RotoItem::eSelectionReasonOther);
            _imp->context->evaluateChange();
            break;
        }
    }
}

void
RotoPanel::onRotoItemInvertedStateChanged()
{
#ifdef NATRON_ROTO_INVERTIBLE
    RotoDrawableItem* i = qobject_cast<RotoDrawableItem*>( sender() );
    RotoDrawableItemPtr item;
    if (i) {
        item = boost::dynamic_pointer_cast<RotoDrawableItem>( i->shared_from_this() );
    }
    if (item) {
        double time = _imp->context->getTimelineCurrentTime();
        TreeItems::iterator it = _imp->findItem(item);
        if ( it != _imp->items.end() ) {
            it->treeItem->setIcon(COL_INVERTED, item->getInverted(time)  ? _imp->iconInverted : _imp->iconUninverted);
        }
    }
#endif
}

void
RotoPanel::onRotoItemShapeColorChanged()
{
    RotoDrawableItem* i = qobject_cast<RotoDrawableItem*>( sender() );
    RotoDrawableItemPtr item;

    if (i) {
        item = boost::dynamic_pointer_cast<RotoDrawableItem>( i->shared_from_this() );
    }

    if (item) {
        assert(_imp->context);
        double time = _imp->context ? _imp->context->getTimelineCurrentTime() : 0.;
        TreeItems::iterator it = _imp->findItem(item);
        if ( it != _imp->items.end() ) {
            QIcon icon;
            double shapeColor[3];
            item->getColor(time, shapeColor);
            makeSolidIcon(shapeColor, icon);
            assert(it->treeItem);
            if (it->treeItem) {
                it->treeItem->setIcon(COL_COLOR, icon);
            }
        }
    }
}

void
RotoPanel::onRotoItemCompOperatorChanged(ViewSpec /*view*/,
                                         int /*dim*/,
                                         int reason)
{
    if ( (ValueChangedReasonEnum)reason == eValueChangedReasonSlaveRefresh ) {
        return;
    }
    RotoDrawableItem* i = qobject_cast<RotoDrawableItem*>( sender() );
    RotoDrawableItemPtr item;
    if (i) {
        item = boost::dynamic_pointer_cast<RotoDrawableItem>( i->shared_from_this() );
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
            }
            break;
        }

        case COL_LOCKED: {
            bool locked = !it->rotoItem->getLocked();
            QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
            for (int i = 0; i < selected.size(); ++i) {
                TreeItems::iterator found = _imp->findItem(selected[i]);
                assert( found != _imp->items.end() );
                found->rotoItem->setLocked(locked, true, RotoItem::eSelectionReasonSettingsPanel);
                _imp->setChildrenLockedRecursively(locked, found->treeItem);
            }
            break;
        }
#ifdef NATRON_ROTO_INVERTIBLE
        case COL_INVERTED: {
            QList<QTreeWidgetItem*> selected = _imp->tree->selectedItems();
            bool inverted = false;
            bool invertedSet = false;
            double time = _imp->context->getTimelineCurrentTime();
            for (int i = 0; i < selected.size(); ++i) {
                TreeItems::iterator found = _imp->findItem(selected[i]);
                assert( found != _imp->items.end() );
                RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>( found->rotoItem.get() );
                if (drawable) {
                    KnobBoolPtr invertedKnob = drawable->getInvertedKnob();
                    inverted = !invertedKnob->getValueAtTime(time);
                    invertedSet = true;
                    if (invertedKnob->getAnimationLevel(0) != eAnimationLevelNone) {
                        invertedKnob->setValueAtTime(time, inverted, ViewSpec::all(), 0);
                    } else {
                        invertedKnob->setValue(inverted, ViewSpec::all(),  0);
                    }
                    found->treeItem->setIcon(COL_INVERTED, inverted ? _imp->iconInverted : _imp->iconUninverted);
                }
            }
            if (!selected.empty() && invertedSet) {
                _imp->context->getInvertedKnob()->setValue(inverted, ViewSpec::all(), 0);
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
                KnobColorPtr colorKnob = drawable->getColorKnob();
                colorKnob->setValues(color.redF(), color.greenF(), color.blueF(), ViewSpec::all(), eValueChangedReasonNatronInternalEdited);
                QIcon icon;
                double colorArray[3];
                colorArray[0] = color.redF();
                colorArray[1] = color.greenF();
                colorArray[2] = color.blueF();
                makeSolidIcon(colorArray, icon);
                found->treeItem->setIcon(COL_COLOR, icon);
                KnobColorPtr contextKnob = _imp->context->getColorKnob();
                contextKnob->setValues(colorArray[0], colorArray[1], colorArray[2], ViewSpec::all(), eValueChangedReasonNatronInternalEdited);
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
                found->treeItem->setIcon(COL_OVERLAY, icon);
            }
        }
    }
    if (mustEvaluate) {
        _imp->context->evaluateChange();
    }
}

void
RotoPanel::onItemLockChanged(int reason)
{
    RotoItemPtr item = getContext()->getLastItemLocked();

    if ( !item || ( (RotoItem::SelectionReasonEnum)reason == RotoItem::eSelectionReasonSettingsPanel ) ) {
        return;
    }
    TreeItems::iterator found = _imp->findItem(item);
    if ( found != _imp->items.end() ) {
        found->treeItem->setIcon(COL_LOCKED, item->isLockedRecursive() ? _imp->iconLocked : _imp->iconUnlocked);
    }
}

void
RotoPanel::onItemGloballyActivatedChanged(const RotoItemPtr& item)
{
    TreeItems::iterator found = _imp->findItem(item);
    if ( found != _imp->items.end() ) {
        found->treeItem->setIcon(COL_ACTIVATED, item->isGloballyActivated() ? _imp->iconVisible : _imp->iconUnvisible);
    }
    _imp->context->emitRefreshViewerOverlays();
}

void
RotoPanelPrivate::setChildrenLockedRecursively(bool locked,
                                               QTreeWidgetItem* item)
{
    item->setIcon(COL_LOCKED, locked ? iconLocked : iconUnlocked);
    for (int i = 0; i < item->childCount(); ++i) {
        setChildrenLockedRecursively( locked, item->child(i) );
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
        it->rotoItem->setScriptName(newName);
        item->setToolTip( COL_LABEL, scriptNameToolTipFromItem(it->rotoItem) );

        _imp->settingNameFromGui = false;
    }
}

void
RotoPanel::onItemLabelChanged(const RotoItemPtr& item)
{
    if (_imp->settingNameFromGui) {
        return;
    }
    TreeItems::iterator it = _imp->findItem(item);
    if ( it != _imp->items.end() ) {
        it->treeItem->setText( COL_LABEL, QString::fromUtf8( item->getLabel().c_str() ) );
    }
}

void
RotoPanel::onItemScriptNameChanged(const RotoItemPtr& item)
{
    if (_imp->settingNameFromGui) {
        return;
    }
    TreeItems::iterator it = _imp->findItem(item);
    if ( it != _imp->items.end() ) {
        it->treeItem->setToolTip( COL_LABEL, scriptNameToolTipFromItem(it->rotoItem) );
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
            QObject::connect( qApp, SIGNAL(focusChanged(QWidget*,QWidget*)), this, SLOT(onFocusChanged(QWidget*,QWidget*)) );
            _imp->editedItemName = it->rotoItem->getLabel();
            _imp->tree->openPersistentEditor(item);
            break;
        }
        case COL_OVERLAY: {
            RotoDrawableItem* drawable = dynamic_cast<RotoDrawableItem*>( it->rotoItem.get() );
            if (drawable) {
                QColorDialog dialog;
                dialog.setOption(QColorDialog::DontUseNativeDialog);
                dialog.setOption(QColorDialog::ShowAlphaChannel);
                _imp->dialogEdition = eColorDialogEditingOverlayColor;
                double oc[4];
                drawable->getOverlayColor(oc);
                QColor color;
                color.setRgbF(oc[0], oc[1], oc[2]);
                color.setAlphaF(oc[3]);
                dialog.setCurrentColor(color);
                QObject::connect( &dialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(onItemColorDialogEdited(QColor)) );
                if ( dialog.exec() ) {
                    color = dialog.selectedColor();
                    oc[0] = color.redF();
                    oc[1] = color.greenF();
                    oc[2] = color.blueF();
                    oc[3] = color.alphaF();
                }
                _imp->dialogEdition = eColorDialogEditingNothing;
                QPixmap pix(15, 15);
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
                dialog.setOption(QColorDialog::DontUseNativeDialog);
                _imp->dialogEdition = eColorDialogEditingShapeColor;
                drawable->getColor(time, shapeColor);
                QColor color;
                color.setRgbF(shapeColor[0], shapeColor[1], shapeColor[2]);
                dialog.setCurrentColor(color);
                QObject::connect( &dialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(onItemColorDialogEdited(QColor)) );
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
                        KnobColorPtr colorKnob = drawable->getColorKnob();
                        colorKnob->setValues(shapeColor[0], shapeColor[1], shapeColor[2], ViewSpec::all(), eValueChangedReasonNatronInternalEdited);
                        mustEvaluate = true;
                        found->treeItem->setIcon(COL_COLOR, icon);
                    }
                }
            }

            if ( colorChosen && !selected.empty() ) {
                KnobColorPtr colorKnob = _imp->context->getColorKnob();
                colorKnob->setValues(shapeColor[0], shapeColor[1], shapeColor[2], ViewSpec::all(), eValueChangedReasonNatronInternalEdited);
                mustEvaluate = true;
            }
            if (mustEvaluate) {
                _imp->context->evaluateChange();
            }
            break;
        }
        default:
            break;
        } // switch
    }
} // RotoPanel::onItemDoubleClicked

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
RotoPanelPrivate::insertSelectionRecursively(const RotoLayerPtr & layer)
{
    const std::list<RotoItemPtr> & children = layer->getItems();

    for (std::list<RotoItemPtr>::const_iterator it = children.begin(); it != children.end(); ++it) {
        RotoLayerPtr l = boost::dynamic_pointer_cast<RotoLayer>(*it);
        SelectedItems::iterator found = std::find(selectedItems.begin(), selectedItems.end(), *it);
        if ( found == selectedItems.end() ) {
            context->selectFromLayer(*it, RotoItem::eSelectionReasonSettingsPanel);
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
        BezierPtr isBezier = boost::dynamic_pointer_cast<Bezier>(*it);
        if (isBezier) {
            QObject::disconnect( isBezier.get(),
                                 SIGNAL(keyframeSet(double)),
                                 this,
                                 SLOT(onSelectedBezierKeyframeSet(double)) );
            QObject::disconnect( isBezier.get(),
                                 SIGNAL(keyframeRemoved(double)),
                                 this,
                                 SLOT(onSelectedBezierKeyframeRemoved(double)) );
            QObject::disconnect( isBezier.get(), SIGNAL(animationRemoved()), this, SLOT(onSelectedBezierAnimationRemoved()) );
        }
        ItemKeys::iterator found = _imp->keyframes.find(*it);
        if ( found != _imp->keyframes.end() ) {
            toRemove.push_back(found->second.keys);
            found->second.visible = false;
        }
    }

    ///Remove previous selection's keyframes
    {
        std::list<std::set<double> >::iterator next = toRemove.begin();
        if ( next != toRemove.end() ) {
            ++next;
        }
        for (std::list<std::set<double> >::iterator it = toRemove.begin(); it != toRemove.end(); ++it) {
            _imp->setVisibleItemKeyframes( *it, false, next == toRemove.end() );
            if ( next != toRemove.end() ) {
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
        BezierPtr bezier = boost::dynamic_pointer_cast<Bezier>(it->rotoItem);
        RotoStrokeItemPtr stroke = boost::dynamic_pointer_cast<RotoStrokeItem>(it->rotoItem);
        RotoLayerPtr layer = boost::dynamic_pointer_cast<RotoLayer>(it->rotoItem);
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

        ItemKeys::iterator found = _imp->keyframes.find(it->rotoItem);
        if ( found != _imp->keyframes.end() ) {
            toAdd.push_back(found->second.keys);
            found->second.visible = true;
        }
    }

    ///Remove previous selection's keyframes
    {
        std::list<std::set<double> >::iterator next = toAdd.begin();
        if ( next != toAdd.end() ) {
            ++next;
        }
        for (std::list<std::set<double> >::iterator it = toAdd.begin(); it != toAdd.end(); ++it) {
            _imp->setVisibleItemKeyframes( *it, true, next == toAdd.end() );
            if ( next != toAdd.end() ) {
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
    pushUndoCommand( new RemoveItemsUndoCommand(this, selectedItems) );
}

static bool
isLayerAParent_recursive(const RotoLayerPtr& layer,
                         const RotoItemPtr& item)
{
    RotoLayerPtr parent = item->getParentLayer();

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
            checkIfProcessedRecursive(matcher, item->parent(), ret);
        }
    }
}

bool
TreeWidget::dragAndDropHandler(const QMimeData* mime,
                               const QPoint & pos,
                               std::list<DroppedTreeItemPtr> & dropped)
{
    if ( mime->hasFormat( QString::fromUtf8("application/x-qabstractitemmodeldatalist") ) ) {
        QByteArray encoded = mime->data( QString::fromUtf8("application/x-qabstractitemmodeldatalist") );
        QDataStream stream(&encoded, QIODevice::ReadOnly);
        DropIndicatorPosition position = dropIndicatorPosition();

        ///list of items we already handled d&d for. If we find an item whose parent
        ///is already in this list we don't handle it
        std::list<QTreeWidgetItem*> processedItems;
        while ( !stream.atEnd() ) {
            int row, col;
            QMap<int, QVariant> roleDataMap;

            stream >> row >> col >> roleDataMap;

            QMap<int, QVariant>::Iterator it = roleDataMap.find(0);

            if ( it != roleDataMap.end() ) {
                DroppedTreeItemPtr ret = boost::make_shared<DroppedTreeItem>();

                ///The target item
                QTreeWidgetItem* into = itemAt(pos);

                if (!into) {
                    return false;
                }

                RotoItemPtr intoRotoItem = _panel->getRotoItemForTreeItem(into);
                QList<QTreeWidgetItem*> foundDropped = findItems(it.value().toString(), Qt::MatchExactly | Qt::MatchRecursive, 0);
                assert( !foundDropped.empty() );
                if ( foundDropped.empty() ) {
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
                RotoLayerPtr isIntoALayer = boost::dynamic_pointer_cast<RotoLayer>(intoRotoItem);

                ///Determine into which layer the item should be inserted.
                switch (position) {
                case QAbstractItemView::AboveItem: {
                    ret->newParentLayer = intoRotoItem->getParentLayer();
                    if (ret->newParentLayer) {
                        ret->newParentItem = into->parent();
                        ///find the target item index into its parent layer and insert the item above it
                        const std::list<RotoItemPtr> & children = ret->newParentLayer->getItems();
                        std::list<RotoItemPtr>::const_iterator found =
                            std::find(children.begin(), children.end(), intoRotoItem);
                        assert( found != children.end() );
                        int intoIndex = std::distance(children.begin(), found);

                        found = std::find(children.begin(), children.end(), ret->droppedRotoItem);
                        int droppedIndex = -1;
                        if ( found != children.end() ) {
                            droppedIndex = std::distance(children.begin(), found);
                        }

                        ///if the dropped item is already into the children and after the found index don't decrement
                        if ( (droppedIndex != -1) && (droppedIndex > intoIndex) ) {
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
                    RotoLayerPtr intoParentLayer = intoRotoItem->getParentLayer();
                    bool isTargetLayerAParent = false;
                    if (isIntoALayer) {
                        isTargetLayerAParent = isLayerAParent_recursive(isIntoALayer, ret->droppedRotoItem);
                    }
                    if (!intoParentLayer || isTargetLayerAParent) {
                        ///insert at the beginning of the layer
                        ret->insertIndex = isIntoALayer ? isIntoALayer->getItems().size() : 0;
                        ret->newParentLayer = isIntoALayer;
                        ret->newParentItem = into;
                    } else {
                        assert(intoParentLayer);
                        ///find the target item index into its parent layer and insert the item after it
                        const std::list<RotoItemPtr> & children = intoParentLayer->getItems();
                        std::list<RotoItemPtr>::const_iterator found =
                            std::find(children.begin(), children.end(), intoRotoItem);
                        assert( found != children.end() );
                        int index = std::distance(children.begin(), found);

                        ///if the dropped item is already into the children and before the found index don't decrement
                        found = std::find(children.begin(), children.end(), ret->droppedRotoItem);
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
    bool accepted = dragAndDropHandler(mime, e->pos(), droppedItems);

    e->setAccepted(accepted);

    if (accepted) {
        _panel->pushUndoCommand( new DragItemsUndoCommand(_panel, droppedItems) );
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

RotoContextPtr
RotoPanel::getContext() const
{
    return _imp->context;
}

void
RotoPanel::clearAndSelectPreviousItem(const RotoItemPtr & item)
{
    _imp->context->clearAndSelectPreviousItem(item, RotoItem::eSelectionReasonOther);
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

    Menu menu(this);
    //menu.setFont( QFont(appFont,appFontSize) );
    menu.setShortcutEnabled(false);
    QAction* addLayerAct = menu.addAction( tr("Add layer") );
    QObject::connect( addLayerAct, SIGNAL(triggered()), this, SLOT(onAddLayerActionTriggered()) );
    QAction* deleteAct = menu.addAction( tr("Delete") );
    deleteAct->setShortcut( QKeySequence(Qt::Key_Backspace) );
    QObject::connect( deleteAct, SIGNAL(triggered()), this, SLOT(onDeleteItemActionTriggered()) );
    QAction* cutAct = menu.addAction( tr("Cut") );
    cutAct->setShortcut( QKeySequence(Qt::Key_X + Qt::CTRL) );
    QObject::connect( cutAct, SIGNAL(triggered()), this, SLOT(onCutItemActionTriggered()) );
    QAction* copyAct = menu.addAction( tr("Copy") );
    copyAct->setShortcut( QKeySequence(Qt::Key_C + Qt::CTRL) );
    QObject::connect( copyAct, SIGNAL(triggered()), this, SLOT(onCopyItemActionTriggered()) );
    QAction* pasteAct = menu.addAction( tr("Paste") );
    pasteAct->setShortcut( QKeySequence(Qt::Key_V + Qt::CTRL) );
    QObject::connect( pasteAct, SIGNAL(triggered()), this, SLOT(onPasteItemActionTriggered()) );
    pasteAct->setEnabled( !_imp->clipBoard.empty() );
    QAction* duplicateAct = menu.addAction( tr("Duplicate") );
    duplicateAct->setShortcut( QKeySequence(Qt::Key_C + Qt::ALT) );
    QObject::connect( duplicateAct, SIGNAL(triggered()), this, SLOT(onDuplicateItemActionTriggered()) );

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
    pushUndoCommand( new RemoveItemsUndoCommand(this, selectedItems) );
}

void
RotoPanel::onCutItemActionTriggered()
{
    assert(_imp->lastRightClickedItem);
    QList<QTreeWidgetItem*> selectedItems = _imp->tree->selectedItems();
    _imp->clipBoard = selectedItems;
    pushUndoCommand( new RemoveItemsUndoCommand(this, selectedItems) );
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
    RotoDrawableItemPtr drawable;
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
        RotoDrawableItemPtr isClipBoardDrawable = boost::dynamic_pointer_cast<RotoDrawableItem>(clip->rotoItem);
        if (!isClipBoardDrawable) {
            return;
        }
    }

    pushUndoCommand( new PasteItemUndoCommand(this, _imp->lastRightClickedItem, _imp->clipBoard) );
}

void
RotoPanel::onDuplicateItemActionTriggered()
{
    pushUndoCommand( new DuplicateItemUndoCommand(this, _imp->lastRightClickedItem) );
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
RotoPanelPrivate::itemHasKey(const RotoItemPtr& item,
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
RotoPanelPrivate::setItemKey(const RotoItemPtr& item,
                             double time)
{
    ItemKeys::iterator it = keyframes.find(item);

    if ( it != keyframes.end() ) {
        std::pair<std::set<double>::iterator, bool> ret = it->second.keys.insert(time);
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
RotoPanelPrivate::removeItemKey(const RotoItemPtr& item,
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
RotoPanelPrivate::removeItemAnimation(const RotoItemPtr& item)
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
RotoPanelPrivate::setVisibleItemKeyframes(const std::set<double>& keyframes,
                                          bool visible,
                                          bool emitSignal)
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
    TimeLinePtr timeline = getNode()->getNode()->getApp()->getTimeLine();

    if (closed) {
        ///remove all keyframes from the structure kept
        std::set< std::set<double> > toRemove;

        for (TreeItems::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
            BezierPtr isBezier = boost::dynamic_pointer_cast<Bezier>(it->rotoItem);
            if (isBezier) {
                ItemKeys::iterator it2 = _imp->keyframes.find(isBezier);
                if ( ( it2 != _imp->keyframes.end() ) && it2->second.visible ) {
                    it2->second.visible = false;
                    toRemove.insert(it2->second.keys);
                }
            }
        }

        std::set<std::set<double> >::iterator next = toRemove.begin();
        if ( next != toRemove.end() ) {
            ++next;
        }
        for (std::set<std::set<double> >::iterator it = toRemove.begin(); it != toRemove.end(); ++it) {
            _imp->setVisibleItemKeyframes( *it, false, next == toRemove.end() );
            if ( next != toRemove.end() ) {
                ++next;
            }
        }
        _imp->keyframes.clear();
    } else {
        ///rebuild all the keyframe structure
        std::set< std::set<double> > toAdd;
        for (TreeItems::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
            BezierPtr isBezier = boost::dynamic_pointer_cast<Bezier>(it->rotoItem);
            if (isBezier) {
                assert ( _imp->keyframes.find(isBezier) == _imp->keyframes.end() );
                TimeLineKeys keys;
                isBezier->getKeyframeTimes(&keys.keys);
                keys.visible = false;
                std::list<SequenceTime> markers;
                for (std::set<double>::iterator it3 = keys.keys.begin(); it3 != keys.keys.end(); ++it3) {
                    markers.push_back(*it3);
                }

                std::pair<ItemKeys::iterator, bool> ret = _imp->keyframes.insert( std::make_pair(isBezier, keys) );
                assert(ret.second);

                ///If the item is selected, make its keyframes visible
                for (SelectedItems::iterator it2 = _imp->selectedItems.begin(); it2 != _imp->selectedItems.end(); ++it2) {
                    if ( it2->get() == isBezier.get() ) {
                        toAdd.insert(keys.keys);
                        ret.first->second.visible = true;
                        break;
                    }
                }
            }
        }
        std::set<std::set<double> >::iterator next = toAdd.begin();
        if ( next != toAdd.end() ) {
            ++next;
        }
        for (std::set<std::set<double> >::iterator it = toAdd.begin(); it != toAdd.end(); ++it) {
            _imp->setVisibleItemKeyframes( *it, true, next == toAdd.end() );
            if ( next != toAdd.end() ) {
                ++next;
            }
        }
    }
} // RotoPanel::onSettingsPanelClosed

void
RotoPanel::onOperatorColMinimumSizeChanged(const QSize& size)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    _imp->tree->header()->setResizeMode(QHeaderView::Fixed);
#else
    _imp->tree->header()->setSectionResizeMode(QHeaderView::Fixed);
#endif
    _imp->tree->setColumnWidth( COL_OPERATOR, size.width() );
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    _imp->tree->header()->setResizeMode(QHeaderView::ResizeToContents);
#else
    _imp->tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
}

//////////////////////////////


RemoveItemsUndoCommand::RemoveItemsUndoCommand(RotoPanel* roto,
                                               const QList<QTreeWidgetItem*> & items)
    : QUndoCommand()
    , _roto(roto)
    , _items()
{
    for (QList<QTreeWidgetItem*>::const_iterator it = items.begin(); it != items.end(); ++it) {
        QTreeWidgetItem* parentItem = (*it)->parent();
        bool foundParent = false;
        for (QList<QTreeWidgetItem*>::const_iterator it2 = items.begin(); it2 != items.end(); ++it2) {
            if ( (*it2) == parentItem ) {
                foundParent = true;
                break;
            }
        }
        if (foundParent) {
            //Not necessary to add this item to the list since the parent is going to remove it anyway
            continue;
        }
        RemovedItem r;
        r.treeItem = *it;
        r.parentTreeItem = parentItem;
        r.item = _roto->getRotoItemForTreeItem(r.treeItem);
        assert(r.item);
        if (r.parentTreeItem) {
            r.parentLayer = boost::dynamic_pointer_cast<RotoLayer>( _roto->getRotoItemForTreeItem(r.parentTreeItem) );
            assert(r.parentLayer);
            r.indexInLayer = r.parentLayer->getChildIndex(r.item);
        }
        _items.push_back(r);
    }
}

RemoveItemsUndoCommand::~RemoveItemsUndoCommand()
{
}

void
RemoveItemsUndoCommand::undo()
{
    for (std::list<RemovedItem>::iterator it = _items.begin(); it != _items.end(); ++it) {
        if (it->parentTreeItem) {
            it->parentTreeItem->addChild(it->treeItem);
        }
        _roto->getContext()->addItem(it->parentLayer, it->indexInLayer, it->item, RotoItem::eSelectionReasonSettingsPanel);

        it->treeItem->setHidden(false);
    }
    _roto->getContext()->evaluateChange();
    setText( tr("Remove items of %2").arg( QString::fromUtf8( _roto->getNodeName().c_str() ) ) );
}

void
RemoveItemsUndoCommand::redo()
{
    if ( _items.empty() ) {
        return;
    }
    _roto->clearAndSelectPreviousItem(_items.back().item);
    for (std::list<RemovedItem>::iterator it = _items.begin(); it != _items.end(); ++it) {
        _roto->getContext()->removeItem(it->item, RotoItem::eSelectionReasonSettingsPanel);
        it->treeItem->setHidden(true);
        if ( it->treeItem->isSelected() ) {
            it->treeItem->setSelected(false);
        }
        if (it->parentTreeItem) {
            it->parentTreeItem->removeChild(it->treeItem);
        }
    }
    _roto->getContext()->evaluateChange();
    setText( tr("Remove items of %2").arg( QString::fromUtf8( _roto->getNodeName().c_str() ) ) );
}

/////////////////////////////


AddLayerUndoCommand::AddLayerUndoCommand(RotoPanel* roto)
    : QUndoCommand()
    , _roto(roto)
    , _firstRedoCalled(false)
    , _parentLayer()
    , _parentTreeItem(0)
    , _treeItem(0)
    , _layer()
    , _indexInParentLayer(-1)
{
}

AddLayerUndoCommand::~AddLayerUndoCommand()
{
}

void
AddLayerUndoCommand::undo()
{
    _treeItem->setHidden(true);
    if (_parentTreeItem) {
        _parentTreeItem->removeChild(_treeItem);
    }
    _roto->getContext()->removeItem(_layer, RotoItem::eSelectionReasonSettingsPanel);
    _roto->clearSelection();
    _roto->getContext()->evaluateChange();
    setText( tr("Add layer to %2").arg( QString::fromUtf8( _roto->getNodeName().c_str() ) ) );
}

void
AddLayerUndoCommand::redo()
{
    if (!_firstRedoCalled) {
        _layer = _roto->getContext()->addLayer();
        _parentLayer = _layer->getParentLayer();
        _treeItem = _roto->getTreeItemForRotoItem(_layer);
        _parentTreeItem = _treeItem->parent();
        if (_parentLayer) {
            _indexInParentLayer = _parentLayer->getChildIndex(_layer);
        }
    } else {
        _roto->getContext()->addLayer(_layer);
        _treeItem->setHidden(false);
        if (_parentLayer) {
            _roto->getContext()->addItem(_parentLayer, _indexInParentLayer, _layer, RotoItem::eSelectionReasonSettingsPanel);
            _parentTreeItem->addChild(_treeItem);
        }
    }
    _roto->clearSelection();
    _roto->getContext()->select(_layer, RotoItem::eSelectionReasonOther);
    _roto->getContext()->evaluateChange();
    setText( tr("Add layer to %2").arg( QString::fromUtf8( _roto->getNodeName().c_str() ) ) );
    _firstRedoCalled = true;
}

/////////////////////////////////

DragItemsUndoCommand::DragItemsUndoCommand(RotoPanel* roto,
                                           const std::list<DroppedTreeItemPtr> & items)
    : QUndoCommand()
    , _roto(roto)
    , _items()
{
    for (std::list<DroppedTreeItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        assert( (*it)->newParentLayer && (*it)->newParentItem && (*it)->insertIndex != -1 );
        Item i;
        i.dropped = *it;
        i.oldParentItem = (*it)->dropped->parent();
        if (!i.oldParentItem) {
            continue;
        }
        i.oldParentLayer = (*it)->droppedRotoItem->getParentLayer();
        if (i.oldParentLayer) {
            i.indexInOldLayer = i.oldParentLayer->getChildIndex( (*it)->droppedRotoItem );
        } else {
            i.indexInOldLayer = -1;
        }
        _items.push_back(i);
    }
}

DragItemsUndoCommand::~DragItemsUndoCommand()
{
}

static void
createCustomWidgetRecursively(RotoPanel* panel,
                              const RotoItemPtr& item)
{
    const RotoDrawableItemPtr isDrawable = boost::dynamic_pointer_cast<RotoDrawableItem>(item);

    if (isDrawable) {
        panel->makeCustomWidgetsForItem(isDrawable);
    }
    const RotoLayerPtr isLayer = boost::dynamic_pointer_cast<RotoLayer>(item);
    if (isLayer) {
        const std::list<RotoItemPtr> & children = isLayer->getItems();
        for (std::list<RotoItemPtr>::const_iterator it = children.begin(); it != children.end(); ++it) {
            createCustomWidgetRecursively( panel, *it );
        }
    }
}

void
DragItemsUndoCommand::undo()
{
    for (std::list<Item>::iterator it = _items.begin(); it != _items.end(); ++it) {
        assert(it->dropped->newParentItem);
        it->dropped->newParentItem->removeChild(it->dropped->dropped);
        it->dropped->newParentLayer->removeItem(it->dropped->droppedRotoItem);
        if (it->oldParentItem) {
            it->oldParentItem->insertChild(it->indexInOldLayer, it->dropped->dropped);

            createCustomWidgetRecursively( _roto, it->dropped->droppedRotoItem );

            assert(it->oldParentLayer);
            it->dropped->droppedRotoItem->setParentLayer(it->oldParentLayer);
            _roto->getContext()->addItem(it->oldParentLayer, it->indexInOldLayer, it->dropped->droppedRotoItem, RotoItem::eSelectionReasonSettingsPanel);
        } else {
            it->dropped->droppedRotoItem->setParentLayer( RotoLayerPtr() );
        }
    }
    _roto->getContext()->refreshRotoPaintTree();
    _roto->getContext()->evaluateChange();

    setText( tr("Re-organize items of %2").arg( QString::fromUtf8( _roto->getNodeName().c_str() ) ) );
}

void
DragItemsUndoCommand::redo()
{
    for (std::list<Item>::iterator it = _items.begin(); it != _items.end(); ++it) {
        it->oldParentItem->removeChild(it->dropped->dropped);
        if (it->oldParentLayer) {
            it->oldParentLayer->removeItem(it->dropped->droppedRotoItem);
        }
        assert(it->dropped->newParentItem);
        it->dropped->newParentItem->insertChild(it->dropped->insertIndex, it->dropped->dropped);

        createCustomWidgetRecursively( _roto, it->dropped->droppedRotoItem );

        it->dropped->newParentItem->setExpanded(true);
        it->dropped->newParentLayer->insertItem(it->dropped->droppedRotoItem, it->dropped->insertIndex);
    }
    _roto->getContext()->refreshRotoPaintTree();
    _roto->getContext()->evaluateChange();
    setText( tr("Re-organize items of %2").arg( QString::fromUtf8( _roto->getNodeName().c_str() ) ) );
}

//////////////////////

static std::string
getItemCopyName(RotoPanel* roto,
                const RotoItemPtr& originalItem)
{
    int i = 1;
    std::string name = originalItem->getScriptName() + "_copy";
    RotoItemPtr foundItemWithName = roto->getContext()->getItemByName(name);

    while (foundItemWithName && foundItemWithName != originalItem) {
        std::stringstream ss;
        ss << originalItem->getScriptName()  << "_copy " << i;
        name = ss.str();
        foundItemWithName = roto->getContext()->getItemByName(name);
        ++i;
    }

    return name;
}

static
void
setItemCopyNameRecursive(RotoPanel* panel,
                         const RotoItemPtr& item)
{
    item->setScriptName( getItemCopyName(panel, item) );
    RotoLayerPtr isLayer = boost::dynamic_pointer_cast<RotoLayer>(item);

    if (isLayer) {
        for (std::list<RotoItemPtr>::const_iterator it = isLayer->getItems().begin(); it != isLayer->getItems().end(); ++it) {
            setItemCopyNameRecursive( panel, *it );
        }
    }
}

PasteItemUndoCommand::PasteItemUndoCommand(RotoPanel* roto,
                                           QTreeWidgetItem* target,
                                           QList<QTreeWidgetItem*> source)
    : QUndoCommand()
    , _roto(roto)
    , _mode()
    , _targetTreeItem(target)
    , _targetItem()
    , _pastedItems()
{
    _targetItem = roto->getRotoItemForTreeItem(target);
    assert(_targetItem);

    for (int i  = 0; i < source.size(); ++i) {
        PastedItem item;
        item.treeItem = source[i];
        item.rotoItem = roto->getRotoItemForTreeItem(item.treeItem);
        assert(item.rotoItem);
        _pastedItems.push_back(item);
    }

    RotoDrawableItemPtr isDrawable = boost::dynamic_pointer_cast<RotoDrawableItem>(_targetItem);

    if (isDrawable) {
        _mode = ePasteModeCopyToItem;
        assert(source.size() == 1 && _pastedItems.size() == 1);
        assert( dynamic_cast<RotoDrawableItem*>( _pastedItems.front().rotoItem.get() ) );
    } else {
        _mode = ePasteModeCopyToLayer;
        for (std::list<PastedItem>::iterator it = _pastedItems.begin(); it != _pastedItems.end(); ++it) {
            BezierPtr srcBezier = boost::dynamic_pointer_cast<Bezier>(it->rotoItem);
            RotoLayerPtr srcLayer = boost::dynamic_pointer_cast<RotoLayer>(it->rotoItem);
            RotoStrokeItemPtr srcStroke = boost::dynamic_pointer_cast<RotoStrokeItem>(it->rotoItem);
            if (srcBezier) {
                std::string name = getItemCopyName(roto, it->rotoItem);
                BezierPtr copy( new Bezier(srcBezier->getContext(), name,
                                                           srcBezier->getParentLayer(), false) );
                copy->clone( srcBezier.get() );
                copy->createNodes();
                //clone overwrites the script name, don't forget to set it back
                copy->setScriptName(name);
                copy->setLabel(name);
                it->itemCopy = copy;
            } else if (srcStroke) {
                std::string name = getItemCopyName(roto, it->rotoItem);
                RotoStrokeItemPtr copy( new RotoStrokeItem( srcStroke->getBrushType(),
                                                                            srcStroke->getContext(),
                                                                            name,
                                                                            RotoLayerPtr() ) );
                copy->createNodes();
                if ( srcStroke->getParentLayer() ) {
                    srcStroke->getParentLayer()->insertItem(copy, 0);
                }
                copy->clone( srcStroke.get() );
                copy->createNodes();
                //clone overwrites the script name, don't forget to set it back
                copy->setScriptName(name);
                copy->setLabel(name);
                it->itemCopy = copy;
            } else {
                assert(srcLayer);
                RotoLayerPtr copy( new RotoLayer( srcLayer->getContext(),
                                                                  "",
                                                                  RotoLayerPtr() ) );
                copy->clone( srcLayer.get() );
                setItemCopyNameRecursive( roto, copy );
                it->itemCopy = copy;
            }
        }
    }
}

PasteItemUndoCommand::~PasteItemUndoCommand()
{
}

void
PasteItemUndoCommand::undo()
{
    if (_mode == ePasteModeCopyToItem) {
        assert(_oldTargetItem);
        _roto->getContext()->deselect(_targetItem, RotoItem::eSelectionReasonOther);
        _targetItem->clone( _oldTargetItem.get() );
        _roto->updateItemGui(_targetTreeItem);
        _roto->getContext()->select(_targetItem, RotoItem::eSelectionReasonOther);
    } else {
        // check that it is a RotoLayer
        assert( dynamic_cast<RotoLayer*>( _targetItem.get() ) );
        for (std::list<PastedItem>::iterator it = _pastedItems.begin(); it != _pastedItems.end(); ++it) {
            _roto->getContext()->removeItem(it->itemCopy, RotoItem::eSelectionReasonOther);
        }
    }
    _roto->getContext()->evaluateChange();
    setText( tr("Paste item(s) of %2").arg( QString::fromUtf8( _roto->getNodeName().c_str() ) ) );
}

void
PasteItemUndoCommand::redo()
{
    if (_mode == ePasteModeCopyToItem) {
        Bezier* isBezier = dynamic_cast<Bezier*>( _targetItem.get() );
        RotoStrokeItem* isStroke = dynamic_cast<RotoStrokeItem*>( _targetItem.get() );
        if (isBezier) {
            BezierPtr oldBezier( new Bezier(isBezier->getContext(), isBezier->getScriptName(), isBezier->getParentLayer(), false) );
            oldBezier->createNodes();
            _oldTargetItem = oldBezier;
        } else if (isStroke) {
            RotoStrokeItemPtr oldStroke( new RotoStrokeItem( isStroke->getBrushType(), isStroke->getContext(), isStroke->getScriptName(), RotoLayerPtr() ) );
            oldStroke->createNodes();
            _oldTargetItem = oldStroke;
            if ( isStroke->getParentLayer() ) {
                //isStroke->getParentLayer()->insertItem(_oldTargetItem, 0);
            }
        }
        _oldTargetItem->clone( _targetItem.get() );
        assert(_pastedItems.size() == 1);
        PastedItem & front = _pastedItems.front();
        ///If we don't deselct the updateItemGUI call will not function correctly because the knobs GUI
        ///have not been refreshed and the selected item is linked to those dirty knobs
        _roto->getContext()->deselect(_targetItem, RotoItem::eSelectionReasonOther);
        _targetItem->clone( front.rotoItem.get() );
        _targetItem->setScriptName( _oldTargetItem->getScriptName() );
        _roto->updateItemGui(_targetTreeItem);
        _roto->getContext()->select(_targetItem, RotoItem::eSelectionReasonOther);
    } else {
        RotoLayerPtr isLayer = boost::dynamic_pointer_cast<RotoLayer>(_targetItem);
        assert(isLayer);
        for (std::list<PastedItem>::iterator it = _pastedItems.begin(); it != _pastedItems.end(); ++it) {
            assert(it->itemCopy);
            //it->itemCopy->setParentLayer(isLayer);
            _roto->getContext()->addItem(isLayer, 0, it->itemCopy, RotoItem::eSelectionReasonOther);
        }
    }

    _roto->getContext()->evaluateChange();
    setText( tr("Paste item(s) of %2").arg( QString::fromUtf8( _roto->getNodeName().c_str() ) ) );
}

//////////////////


DuplicateItemUndoCommand::DuplicateItemUndoCommand(RotoPanel* roto,
                                                   QTreeWidgetItem* items)
    : QUndoCommand()
    , _roto(roto)
    , _item()
{
    _item.treeItem = items;
    _item.item = _roto->getRotoItemForTreeItem(_item.treeItem);
    assert( _item.item->getParentLayer() );
    BezierPtr isBezier = boost::dynamic_pointer_cast<Bezier>( _item.item );
    RotoStrokeItemPtr isStroke = boost::dynamic_pointer_cast<RotoStrokeItem>( _item.item);
    RotoLayerPtr isLayer = boost::dynamic_pointer_cast<RotoLayer>( _item.item);
    if (isBezier) {
        std::string name = getItemCopyName(roto, isBezier);
        BezierPtr bezierCopy( new Bezier(isBezier->getContext(), name, isBezier->getParentLayer(), false) );
        bezierCopy->createNodes();
        _item.duplicatedItem = bezierCopy;
        _item.duplicatedItem->clone( isBezier.get() );
        //clone has overwritten the name
        _item.duplicatedItem->setScriptName(name);
        _item.duplicatedItem->setLabel(name);
    } else if (isStroke) {
        std::string name = getItemCopyName(roto, isStroke);
        RotoStrokeItemPtr strokeCopy( new RotoStrokeItem( isStroke->getBrushType(), isStroke->getContext(), name, RotoLayerPtr() ) );
        strokeCopy->createNodes();
        _item.duplicatedItem = strokeCopy;
        if ( isStroke->getParentLayer() ) {
            isStroke->getParentLayer()->insertItem(_item.duplicatedItem, 0);
        }
        _item.duplicatedItem->clone( isStroke.get() );
        //clone has overwritten the name
        _item.duplicatedItem->setScriptName(name);
        _item.duplicatedItem->setLabel(name);
    } else {
        assert(isLayer);
        _item.duplicatedItem.reset( new RotoLayer(*isLayer) );
        setItemCopyNameRecursive( roto, _item.duplicatedItem );
    }
}

DuplicateItemUndoCommand::~DuplicateItemUndoCommand()
{
}

void
DuplicateItemUndoCommand::undo()
{
    _roto->getContext()->removeItem(_item.duplicatedItem, RotoItem::eSelectionReasonOther);
    _roto->getContext()->evaluateChange();
    setText( tr("Duplicate item(s) of %2").arg( QString::fromUtf8( _roto->getNodeName().c_str() ) ) );
}

void
DuplicateItemUndoCommand::redo()
{
    _roto->getContext()->addItem(_item.item->getParentLayer(),
                                 0, _item.duplicatedItem, RotoItem::eSelectionReasonOther);

    _roto->getContext()->evaluateChange();
    setText( tr("Duplicate item(s) of %2").arg( QString::fromUtf8( _roto->getNodeName().c_str() ) ) );
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_RotoPanel.cpp"
