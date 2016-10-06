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

#include "KnobItemsTableGui.h"

#include <map>

#include <QApplication>
#include <QHBoxLayout>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QHeaderView>
#include <QCheckBox>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QMouseEvent>
#include <QMimeData>
#include <QUndoCommand>
#include <QClipboard>

#include "Engine/Utils.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"

#include "Gui/AnimatedCheckBox.h"
#include "Gui/Button.h"
#include "Gui/ComboBox.h"
#include "Gui/DockablePanel.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobGuiChoice.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiMacros.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/TableModelView.h"

#include "Engine/KnobItemsTable.h"

#include "Serialization/KnobTableItemSerialization.h"
#include "Serialization/SerializationIO.h"

#define kNatronKnobItemsTableGuiMimeType "Natron/NatronKnobItemsTableGuiMimeType"
NATRON_NAMESPACE_ENTER;

struct ModelItem {
    // The internal item in Engine
    KnobTableItemWPtr internalItem;

    struct ColumnData
    {
        KnobIWPtr knob;
        int knobDimension;
        
        ColumnData()
        : knob()
        , knobDimension(-1)
        {
            
        }
    };
    
    TableItemPtr item;
    
    std::vector<ColumnData> columnItems;
    
    // If the item wants to display its label in a column, here it is
    int labelColIndex;
    
    // If the item wants to display the scriptname in a column here it is
    int scriptNameColIndex;
    
    ModelItem()
    : internalItem()
    , item()
    , columnItems()
    , labelColIndex(-1)
    , scriptNameColIndex(-1)
    {

    }
};
typedef std::vector<ModelItem> ModelItemsVec;


struct KnobItemsTableGuiPrivate
{
    KnobItemsTableGui* _publicInterface;
    KnobItemsTableWPtr internalModel;
    DockablePanel* panel;

    TableModelPtr tableModel;
    TableView* tableView;
    boost::scoped_ptr<TableItemEditorFactory> itemEditorFactory;

    ModelItemsVec items;

    std::map<KnobIWPtr, KnobGuiPtr> knobsMapping;
    
    // Prevent recursion from selectionChanged signal of QItemSelectionModel
    int selectingModelRecursion;

    KnobItemsTableGuiPrivate(KnobItemsTableGui* publicInterface, DockablePanel* panel, const KnobItemsTablePtr& table)
    : _publicInterface(publicInterface)
    , internalModel(table)
    , panel(panel)
    , tableModel()
    , tableView(0)
    , itemEditorFactory()
    , items()
    , knobsMapping()
    , selectingModelRecursion(0)
    {
    }

    ModelItemsVec::iterator findItem(const KnobTableItemPtr& internalItem) {
        for (ModelItemsVec::iterator it = items.begin(); it!=items.end(); ++it) {
            if (it->internalItem.lock() == internalItem) {
                return it;
            }
        }
        return items.end();
    }

    ModelItemsVec::iterator findItem(const TableItemConstPtr& item) {
        for (ModelItemsVec::iterator it = items.begin(); it!=items.end(); ++it) {
            if (it->item == item) {
                return it;
            }
        }
        return items.end();
    }
    
    void showItemMenu(const TableItemPtr& item, const QPoint & globalPos);
    
    bool createItemCustomWidgetAtCol(const KnobTableItemPtr& item, int row, int col);

    void createCustomWidgetRecursively(const KnobTableItemPtr& item);
    
    void selectionFromIndexList(const QModelIndexList& indexes, std::list<KnobTableItemPtr>* items);
    
    void selectionToItems(const QItemSelection& selection, std::list<KnobTableItemPtr>* items);
    
    void itemsToSelection(const std::list<KnobTableItemPtr>& items, QItemSelection* selection);

    void createTableItems(const KnobTableItemPtr& item);
};

bool
KnobItemsTableGuiPrivate::createItemCustomWidgetAtCol(const KnobTableItemPtr& item, int row, int col)
{
    int dim;
    KnobIPtr knob = item->getColumnKnob(col, &dim);
    if (!knob) {
        return false;
    }
    
    // For these kind of knobs, create a knob GUI. For all others, the default line-edit provided by the view is enough to display numbers/strings
    KnobChoicePtr isChoice = toKnobChoice(knob);
    KnobColorPtr isColor = toKnobColor(knob);
    KnobButtonPtr isButton = toKnobButton(knob);
    KnobBoolPtr isBoolean = toKnobBool(knob);
    
    // Create a KnobGui just for these kinds of parameters, for all others, use default interface
    if (!isChoice && (!isColor || dim != -1) && !isButton && !isBoolean) {
        return false;
    }
    
    // Look for any existing KnobGui and destroy it
    {
        std::map<KnobIWPtr, KnobGuiPtr>::iterator found = knobsMapping.find(knob);
        if (found != knobsMapping.end()) {
            found->second->removeGui();
            knobsMapping.erase(found);
        }
    }
    
    
    // Create the Knob Gui
    KnobGuiPtr ret( appPTR->createGuiForKnob(knob, _publicInterface) );
    if (!ret) {
        assert(false);
        return false;
    }
    ret->initialize();
    knobsMapping.insert(std::make_pair(knob, ret));
    
    QWidget* rowContainer = new QWidget;
    QHBoxLayout* rowLayout = new QHBoxLayout(rowContainer);
    std::vector<KnobIPtr> knobsOnSameLine;
    ret->createGUI(rowContainer, 0 /*labelContainer*/, 0 /*label*/, 0 /*warningIndicator*/, rowLayout, true /*isOnnewLine*/, 0 /*lastKnobSpacing*/, knobsOnSameLine);
    
    
    // Set the widget
    {
        ModelItemsVec::iterator foundItem = findItem(item);
        assert((int)foundItem->columnItems.size() == tableModel->columnCount());
        tableView->setCellWidget(row, col, rowContainer);
    }
    
    // We must call this otherwise this is never called by Qt for custom widgets in an item view (this is a Qt bug)
    KnobGuiChoice* isChoiceGui = dynamic_cast<KnobGuiChoice*>(ret.get());
    if (isChoiceGui) {
#pragma message WARN("Check if in Qt5 this hack is still needed")
        ComboBox* combobox = isChoiceGui->getCombobox();
        combobox->setProperty("ComboboxColumn", col);
        QObject::connect(combobox, SIGNAL(minimumSizeChanged(QSize)), _publicInterface, SLOT(onComboBoxMinimumSizeChanged(QSize)));
        QSize s = combobox->minimumSizeHint();
        Q_UNUSED(s);
    }
    return true;
} // createItemCustomWidgetAtCol

void
KnobItemsTableGuiPrivate::createCustomWidgetRecursively(const KnobTableItemPtr& item)
{
    int row = item->getIndexInParent();
    int nCols = tableModel->columnCount();
    for (int i = 0; i < nCols; ++i) {
        createItemCustomWidgetAtCol(item, row, i);
    }

    const std::vector<KnobTableItemPtr>& children = item->getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        createCustomWidgetRecursively(children[i]);
    }
}

/**
 * @brief Helper class to properly relfect animation level on a knob in the background of the cell
 **/
class AnimatedKnobItemDelegate
: public QStyledItemDelegate
{
    KnobItemsTableGuiPrivate* _imp;
public:

    explicit AnimatedKnobItemDelegate(KnobItemsTableGuiPrivate* imp)
    : QStyledItemDelegate()
    , _imp(imp)
    {

    }

private:

    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const OVERRIDE FINAL;
    
};

void
AnimatedKnobItemDelegate::paint(QPainter * painter,
                                const QStyleOptionViewItem & option,
                                const QModelIndex & index) const
{
    assert( index.isValid() );
    if ( !index.isValid() ) {
        QStyledItemDelegate::paint(painter, option, index);

        return;
    }
    int col = index.column();


    TableItemPtr item = _imp->tableModel->getItem(index);
    assert(item);
    if (!item) {
        // coverity[dead_error_line]
        QStyledItemDelegate::paint(painter, option, index);

        return;
    }
    // If the item is being edited, do not draw it
    if (_imp->tableView->editedItem() == item) {
        QStyledItemDelegate::paint(painter, option, index);

        return;
    }

    // If there's a widget, don't draw on top of it
    if (_imp->tableView->cellWidget(index.row(), index.column())) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    // Find from the internal Engine item the associated knob
    ModelItemsVec::iterator foundItem = _imp->findItem(item);;
    assert(foundItem != _imp->items.end());
    if (foundItem == _imp->items.end()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    KnobTableItemPtr internalItem = foundItem->internalItem.lock();
    if (!internalItem) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    int knobDimensionIndex;
    KnobIPtr knob = internalItem->getColumnKnob(col, &knobDimensionIndex);
    if (!knob) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    // get the proper subrect from the style
    QStyle *style = QApplication::style();
    QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &option);

    // Get the animation level
    AnimationLevelEnum level = knob->getAnimationLevel(knobDimensionIndex);


    // Figure out the background color depending on the animation level
    bool fillRect = true;
    QBrush brush;
    if (option.state & QStyle::State_Selected) {
        brush = option.palette.highlight();
    } else if (level == eAnimationLevelInterpolatedValue) {
        double r, g, b;
        appPTR->getCurrentSettings()->getInterpolatedColor(&r, &g, &b);
        QColor col;
        col.setRgbF(r, g, b);
        brush = col;
    } else if (level == eAnimationLevelOnKeyframe) {
        double r, g, b;
        appPTR->getCurrentSettings()->getKeyframeColor(&r, &g, &b);
        QColor col;
        col.setRgbF(r, g, b);
        brush = col;
    } else {
        fillRect = false;
    }

    // No special animation, do not fill the rect, default will do it
    if (fillRect) {
        painter->fillRect( geom, brush);
    }

    // Figure out text color
    QPen pen = painter->pen();
    if ( !item->getFlags(col).testFlag(Qt::ItemIsEditable) ) {
        // Paint non editable items text in black
        pen.setColor(Qt::black);
    } else {
        double r, g, b;
        appPTR->getCurrentSettings()->getTextColor(&r, &g, &b);
        QColor col;
        col.setRgbF(r, g, b);
        pen.setColor(col);
    }
    painter->setPen(pen);

    QRect textRect( geom.x() + 5, geom.y(), geom.width() - 5, geom.height() );
    QRect r;
    QVariant var = item->getData(col, Qt::DisplayRole);
    double d = var.toDouble();
    QString dataStr = QString::number(d, 'f', 6);
    painter->drawText(textRect, Qt::TextSingleLine | Qt::AlignVCenter | Qt::AlignLeft, dataStr, &r);
} // paint

class KnobItemsTableView : public TableView
{
    KnobItemsTableGuiPrivate* _imp;
public:

    KnobItemsTableView(KnobItemsTableGuiPrivate* imp, QWidget* parent)
    : TableView(parent)
    , _imp(imp)
    {
    }

    virtual ~KnobItemsTableView()
    {

    }

private:

    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void dragMoveEvent(QDragMoveEvent *e) OVERRIDE FINAL;
    virtual void dragEnterEvent(QDragEnterEvent *e) OVERRIDE FINAL;
    virtual void dropEvent(QDropEvent* e) OVERRIDE FINAL;
    virtual void setupAndExecDragObject(QDrag* drag,
                                        const QModelIndexList& rows,
                                        Qt::DropActions supportedActions,
                                        Qt::DropAction defaultAction) OVERRIDE FINAL;
};

KnobItemsTableGui::KnobItemsTableGui(const KnobItemsTablePtr& table, DockablePanel* panel, QWidget* parent)
: _imp(new KnobItemsTableGuiPrivate(this, panel, table))
{

    setContainerWidget(panel);

    _imp->tableView = new TableView(parent);

    // Very important or else a bug in Qt selection frame will ask to redraw the whole interface, making everything laggy
    _imp->tableView->setAttribute(Qt::WA_MacShowFocusRect, 0);

#if QT_VERSION < 0x050000
    _imp->tableView->header()->setResizeMode(QHeaderView::ResizeToContents);
#else
    _imp->tableView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
    _imp->tableView->header()->setStretchLastSection(true);

    QObject::connect( _imp->tableView, SIGNAL(itemRightClicked(QPoint,TableItemPtr)), this, SLOT(onTableItemRightClicked(QPoint,TableItemPtr)) );

    AnimatedKnobItemDelegate* delegate = new AnimatedKnobItemDelegate(_imp.get());
    _imp->itemEditorFactory.reset(new TableItemEditorFactory);
    delegate->setItemEditorFactory( _imp->itemEditorFactory.get() );
    _imp->tableView->setItemDelegate(delegate);

    int nCols = table->getColumnsCount();

    KnobItemsTable::KnobItemsTableTypeEnum knobTableType = table->getType();
    TableModel::TableModelTypeEnum type;
    switch (knobTableType) {
        case KnobItemsTable::eKnobItemsTableTypeTable:
            type = TableModel::eTableModelTypeTable;
            break;
        case KnobItemsTable::eKnobItemsTableTypeTree:
            type = TableModel::eTableModelTypeTree;
            break;
    }
    _imp->tableModel = TableModel::create(nCols, type);
    QObject::connect( _imp->tableModel.get(), SIGNAL(s_itemChanged(TableItemPtr)), this, SLOT(onTableItemDataChanged(TableItemPtr)) );
    _imp->tableView->setTableModel(_imp->tableModel);

    QItemSelectionModel *selectionModel = _imp->tableView->selectionModel();
    QObject::connect( selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this,
                     SLOT(onModelSelectionChanged(QItemSelection,QItemSelection)) );



    std::vector<QString> headerLabels(nCols), headerIcons(nCols);
    for (int i = 0; i < nCols; ++i) {
        headerIcons[i] = QString::fromUtf8(table->getColumnIcon(i).c_str());
        headerLabels[i] = QString::fromUtf8(table->getColumnText(i).c_str());
    }

    QString iconsPath = QString::fromUtf8(table->getIconsPath().c_str());
    Global::ensureLastPathSeparator(iconsPath);

    std::vector<std::pair<QString, QIcon> > headerDatas(nCols);
    for (int i = 0; i < nCols; ++i) {
        if (!headerLabels[i].isEmpty()) {
            headerDatas[i].first = headerLabels[i];
        }
        if (!headerIcons[i].isEmpty()) {
            QString filePath = iconsPath + headerIcons[i];
            QPixmap p;
            if (p.load(filePath) && !p.isNull()) {
                QIcon ic(p);
                headerDatas[i].second = ic;
            }
        }
    }
    _imp->tableModel->setHorizontalHeaderData(headerDatas);


    _imp->tableView->setUniformRowHeights(table->getRowsHaveUniformHeight());


    if (knobTableType == KnobItemsTable::eKnobItemsTableTypeTree) {
        _imp->tableView->setItemsExpandable(false);
        _imp->tableView->setRootIsDecorated(true);
        _imp->tableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
        _imp->tableView->setExpandsOnDoubleClick(true);
    }

    bool dndSupported = table->isDragAndDropSupported();
    _imp->tableView->setDragEnabled(dndSupported);
    _imp->tableView->setAcceptDrops(dndSupported);

    if (dndSupported) {
        if (table->isDropFromExternalSourceSupported()) {
            _imp->tableView->setDragDropMode(QAbstractItemView::DragDrop);
        } else {
            _imp->tableView->setDragDropMode(QAbstractItemView::InternalMove);
        }
    }

    
    
    connect(table.get(), SIGNAL(selectionChanged(std::list<KnobTableItemPtr>,std::list<KnobTableItemPtr>,TableChangeReasonEnum)), this, SLOT(onModelSelectionChanged(std::list<KnobTableItemPtr>,std::list<KnobTableItemPtr>,TableChangeReasonEnum)));
    connect(table.get(), SIGNAL(topLevelItemRemoved(KnobTableItemPtr,TableChangeReasonEnum)), this, SLOT(onModelTopLevelItemRemoved( KnobTableItemPtr,TableChangeReasonEnum)));
    connect(table.get(), SIGNAL(topLevelItemInserted(int,KnobTableItemPtr,TableChangeReasonEnum)), this, SLOT(onModelTopLevelItemInserted(int,KnobTableItemPtr,TableChangeReasonEnum)));
    
    /*connect(table.get(), SIGNAL(labelChanged(QString,TableChangeReasonEnum)), this, SLOT(onItemLabelChanged(QString,TableChangeReasonEnum)));
    connect(table.get(), SIGNAL(childRemoved(KnobTableItemPtr,TableChangeReasonEnum)), this, SLOT(onItemChildRemoved(KnobTableItemPtr,TableChangeReasonEnum)));
    connect(table.get(), SIGNAL(childInserted(int,KnobTableItemPtr,TableChangeReasonEnum)), this, SLOT(childInserted(int, KnobTableItemPtr,TableChangeReasonEnum)));*/
} // KnobItemsTableGui::KnobItemsTableGui

KnobItemsTableGui::~KnobItemsTableGui()
{
    
}

TableView*
KnobItemsTableGui::getTableView() const
{
    return _imp->tableView;
}

void
KnobItemsTableView::setupAndExecDragObject(QDrag* drag,
                                           const QModelIndexList& rows,
                                           Qt::DropActions supportedActions,
                                           Qt::DropAction defaultAction)

{

    std::list<KnobTableItemPtr> items;
    for (QModelIndexList::const_iterator it = rows.begin(); it!=rows.end(); ++it) {
        // Get the first col item
        TableItemPtr item = _imp->tableModel->getItem(*it);
        assert(item);
        if (!item) {
            continue;
        }
        ModelItemsVec::iterator found = _imp->findItem(item);
        if (found == _imp->items.end()) {
            continue;
        }
        KnobTableItemPtr internalItem = found->internalItem.lock();
        if (!internalItem) {
            continue;
        }
        items.push_back(internalItem);

    }

    if (items.empty()) {
        return;
    }

    // Make up drag data
    SERIALIZATION_NAMESPACE::KnobItemsTableSerialization obj;

    NodeSettingsPanel* isNodePanel = dynamic_cast<NodeSettingsPanel*>(_imp->panel);
    if (isNodePanel) {
        NodeGuiPtr nodeUI = isNodePanel->getNode();
        assert(nodeUI);
        obj.nodeScriptName = nodeUI->getNode()->getFullyQualifiedName();
    }
    for (std::list<KnobTableItemPtr>::iterator it = items.begin(); it!= items.end(); ++it) {
        SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr s(new SERIALIZATION_NAMESPACE::KnobTableItemSerialization);
        (*it)->toSerialization(s.get());
        obj.items.push_back(s);
    }

    std::ostringstream ss;
    SERIALIZATION_NAMESPACE::write(ss, obj);

    QByteArray dataArray(ss.str().c_str());

    QMimeData *data = new QMimeData;
    data->setData(QLatin1String(kNatronKnobItemsTableGuiMimeType), dataArray);
    drag->setMimeData(data);
    
    if (drag->exec(supportedActions, defaultAction) == Qt::MoveAction) {
        
        // If the target is NULL, we have no choice but to remove data from the original table.
        // This means the drop finished on another instance of Natron
        if (!drag->target()) {
            KnobItemsTablePtr model = _imp->internalModel.lock();
            // If the target table is not this one, we have no choice but to remove from this table the items out of undo/redo operation
            for (std::list<KnobTableItemPtr>::iterator it = items.begin(); it!= items.end(); ++it) {
                model->removeItem(*it, eTableChangeReasonInternal);
            }
        }
    }
    
} // setupAndExecDragObject

void
KnobItemsTableView::dragMoveEvent(QDragMoveEvent *e)
{
    const QMimeData* mimedata = e->mimeData();
    if ( !mimedata->hasFormat( QLatin1String(kNatronKnobItemsTableGuiMimeType) ) || !_imp->internalModel.lock()->isDragAndDropSupported() ) {
        e->ignore();
    } else {
        e->accept();
    }
    TableView::dragMoveEvent(e);
}

void
KnobItemsTableView::dragEnterEvent(QDragEnterEvent *e)
{
    const QMimeData* mimedata = e->mimeData();
    if ( !mimedata->hasFormat( QLatin1String(kNatronKnobItemsTableGuiMimeType) ) || !_imp->internalModel.lock()->isDragAndDropSupported() ) {
        e->ignore();
    } else {
        e->accept();
    }
    TableView::dragEnterEvent(e);

}


void
KnobItemsTableView::keyPressEvent(QKeyEvent* e)
{
    if ( (e->key() == Qt::Key_Delete) || (e->key() == Qt::Key_Backspace) ) {
        _imp->_publicInterface->onDeleteItemsActionTriggered();
    } else if ( (e->key() == Qt::Key_C) && modCASIsControl(e) ) {
        _imp->_publicInterface->onCopyItemsActionTriggered();
    } else if ( (e->key() == Qt::Key_V) && modCASIsControl(e) ) {
        _imp->_publicInterface->onPasteItemsActionTriggered();
    } else if ( (e->key() == Qt::Key_X) && modCASIsControl(e) ) {
        _imp->_publicInterface->onCutItemsActionTriggered();
    } else if ( (e->key() == Qt::Key_C) && modCASIsAlt(e) ) {
        _imp->_publicInterface->onDuplicateItemsActionTriggered();
    } else if ( (e->key() == Qt::Key_A) && modCASIsControl(e) ) {
        _imp->_publicInterface->onSelectAllItemsActionTriggered();
    } else {
        TableView::keyPressEvent(e);
    }
}

Gui*
KnobItemsTableGui::getGui() const
{
    return _imp->panel->getGui();
}

const QUndoCommand*
KnobItemsTableGui::getLastUndoCommand() const
{
    return _imp->panel->getLastUndoCommand();
}

void
KnobItemsTableGui::pushUndoCommand(QUndoCommand* cmd)
{
    _imp->panel->pushUndoCommand(cmd);
}

KnobGuiPtr
KnobItemsTableGui::getKnobGui(const KnobIPtr& knob) const {
    for (std::map<KnobIWPtr, KnobGuiPtr>::const_iterator it = _imp->knobsMapping.begin(); it != _imp->knobsMapping.end(); ++it) {
        if (it->first.lock() == knob) {
            return it->second;
        }
    }
    return KnobGuiPtr();
}

int
KnobItemsTableGui::getItemsSpacingOnSameLine() const
{
    return 0;
}

KnobItemsTablePtr
KnobItemsTableGui::getInternalTable() const
{
    return _imp->internalModel.lock();
}

void
KnobItemsTableGuiPrivate::showItemMenu(const TableItemPtr& item, const QPoint & globalPos)
{
    
}

void
KnobItemsTableGui::onComboBoxMinimumSizeChanged(const QSize& size)
{
    ComboBox* combobox = dynamic_cast<ComboBox*>(sender());
    if (!combobox) {
        return;
    }
#if QT_VERSION < 0x050000
    _imp->tableView->header()->setResizeMode(QHeaderView::Fixed);
#else
    _imp->tableView->header()->setSectionResizeMode(QHeaderView::Fixed);
#endif
    int col = combobox->property("ComboboxColumn").toInt();
    _imp->tableView->setColumnWidth( col, size.width() );
#if QT_VERSION < 0x050000
    _imp->tableView->header()->setResizeMode(QHeaderView::ResizeToContents);
#else
    _imp->tableView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
}


class RemoveItemsUndoCommand
: public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(RemoveItemsUndoCommand)

public:

    struct Item
    {
        KnobTableItemPtr item;
        KnobTableItemPtr parent;
        int indexInParent;
    };
    
    RemoveItemsUndoCommand(KnobItemsTableGuiPrivate* table,
                           const std::list<KnobTableItemPtr>& items)
    : QUndoCommand()
    , _table(table)
    , _items()
    {
        for (std::list<KnobTableItemPtr>::const_iterator it = items.begin(); it!=items.end(); ++it) {
            Item i;
            i.item = *it;
            i.indexInParent = (*it)->getIndexInParent();
            i.parent = (*it)->getParent();
            _items.push_back(i);
        }
    }

    virtual ~RemoveItemsUndoCommand();

    virtual void undo() OVERRIDE FINAL
    {
        KnobItemsTablePtr table = _table->internalModel.lock();
        for (std::list<Item>::const_iterator it = _items.begin(); it!=_items.end(); ++it) {
            table->removeItem(it->item, eTableChangeReasonInternal);
        }
    }
    
    virtual void redo() OVERRIDE FINAL
    {
        KnobItemsTablePtr table = _table->internalModel.lock();
        for (std::list<Item>::const_iterator it = _items.begin(); it!=_items.end(); ++it) {
            if (it->parent) {
                it->parent->insertChild(it->indexInParent, it->item, eTableChangeReasonInternal);
            } else {
                table->insertTopLevelItem(it->indexInParent, it->item, eTableChangeReasonInternal);
            }
        }
    }

private:

    KnobItemsTableGuiPrivate* _table;
    std::list<Item> _items;
};



class DragItemsUndoCommand
: public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(DragItemsUndoCommand)

public:

    struct Item
    {
        KnobTableItemPtr item;
        KnobTableItemWPtr oldParent;
        KnobTableItemWPtr newParent;
        int indexInOldParent, indexInNewParent;
    };


    DragItemsUndoCommand(KnobItemsTableGuiPrivate* table,
                         const KnobItemsTablePtr& originalTable,
                         const std::list<DragItemsUndoCommand::Item>& items)
    : QUndoCommand()
    , _table(table)
    , _originalTable(originalTable)
    , _items(items)
    {
        setText(tr("Re-organize items"));
    }

    virtual ~DragItemsUndoCommand()
    {

    }

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    void moveItem(int indexInparent, const KnobTableItemPtr& parent, const KnobTableItemPtr& item, const KnobItemsTablePtr& fromTable, const KnobItemsTablePtr& toTable);

    KnobItemsTableGuiPrivate* _table;
    KnobItemsTablePtr _originalTable;
    std::list<Item> _items;
};

void
DragItemsUndoCommand::moveItem(int indexInParent, const KnobTableItemPtr& parent, const KnobTableItemPtr& item, const KnobItemsTablePtr& fromTable, const KnobItemsTablePtr& toTable)
{
    fromTable->removeItem(item, eTableChangeReasonInternal);
    if (parent) {
        parent->insertChild(indexInParent, item, eTableChangeReasonInternal);
        ModelItemsVec::iterator foundParent = _table->findItem(parent);
        if (foundParent != _table->items.end()) {
            _table->tableView->setExpanded(_table->tableModel->getItemIndex(foundParent->item), true);
        }
    } else {
        toTable->insertTopLevelItem(indexInParent, item, eTableChangeReasonInternal);
    }


    _table->createCustomWidgetRecursively(item);


}

void
DragItemsUndoCommand::undo()
{
    for (std::list<Item>::iterator it = _items.begin(); it != _items.end(); ++it) {
        moveItem(it->indexInOldParent, it->oldParent.lock(), it->item, _table->internalModel.lock(), _originalTable);
    }
}

void
DragItemsUndoCommand::redo()
{
    for (std::list<Item>::iterator it = _items.begin(); it != _items.end(); ++it) {
        moveItem(it->indexInNewParent, it->newParent.lock(), it->item, _originalTable, _table->internalModel.lock());
    }

}


void
KnobItemsTableView::dropEvent(QDropEvent* e)
{

    KnobItemsTablePtr table = _imp->internalModel.lock();
    if (!table->isDragAndDropSupported()) {
        return;
    }
    const QMimeData* mimedata = e->mimeData();
    static const QString mimeDataType(QLatin1String(kNatronKnobItemsTableGuiMimeType));
    if ( !mimedata->hasFormat(mimeDataType) ) {
        e->ignore();
        TableView::dropEvent(e);
        return;
    } else {
        e->accept();
    }

    QVariant data = mimedata->data(mimeDataType);
    QString serializationStr = data.toString();
    std::stringstream ss(serializationStr.toStdString());
    SERIALIZATION_NAMESPACE::KnobItemsTableSerialization obj;
    try {
        SERIALIZATION_NAMESPACE::read(ss, &obj);
    } catch (...) {
        e->ignore();
        return;
    }


    // Find the original table from which the knob was from
    // Operation was move, hence remove items from this view
    KnobItemsTablePtr originalTable;
    {
        NodePtr originalNode = _imp->_publicInterface->getGui()->getApp()->getProject()->getNodeByFullySpecifiedName(obj.nodeScriptName);
        if (!originalNode) {
            e->ignore();
            return;
        }

        NodeGuiPtr originalNodeUI = boost::dynamic_pointer_cast<NodeGui>(originalNode->getNodeGui());
        assert(originalNodeUI);
        NodeSettingsPanel* originalPanel = originalNodeUI->getSettingPanel();
        assert(originalPanel);
        if (originalPanel) {
            KnobItemsTableGuiPtr originalTableUI = originalPanel->getKnobItemsTable();
            if (originalTableUI) {
                originalTable = originalTableUI->getInternalTable();
            }
        }
    }

    KnobTableItemPtr targetInternalItem;
    {
        TableItemPtr targetItem = itemAt(e->pos());
        if (targetItem) {
            ModelItemsVec::iterator found = _imp->findItem(targetItem);
            if (found != _imp->items.end()) {
                targetInternalItem = found->internalItem.lock();
            }

        }
    }

    std::list<KnobTableItemPtr> droppedItems;
    for (std::list<SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr>::const_iterator it = obj.items.begin(); it != obj.items.end(); ++it) {
        KnobTableItemPtr newItem = table->createItemFromSerialization(*it);
        if (newItem) {
            droppedItems.push_back(newItem);
        }
    }
    if (droppedItems.empty()) {
        return;
    }


    //OnItem, AboveItem, BelowItem, OnViewport
    DropIndicatorPosition position = dropIndicatorPosition();

    e->accept();

    std::list<DragItemsUndoCommand::Item> dndItems;
    for (std::list<KnobTableItemPtr>::const_iterator it = droppedItems.begin(); it!=droppedItems.end(); ++it) {

        DragItemsUndoCommand::Item d;
        d.oldParent = (*it)->getParent();
        d.indexInOldParent = (*it)->getIndexInParent();
        d.item = (*it);
        switch (position) {
            case QAbstractItemView::AboveItem: {

                assert(targetInternalItem);
                int targetItemIndex = targetInternalItem->getIndexInParent();

                assert(d.indexInOldParent != -1 && targetItemIndex != -1);

                // If the dropped item is already into the children and after the found index don't decrement
                if (d.indexInOldParent > targetItemIndex) {
                    d.indexInNewParent = targetItemIndex;
                } else {
                    //The item "above" in the tree is index - 1 in the internal list which is ordered from bottom to top
                    d.indexInNewParent = targetItemIndex == 0 ? 0 : targetItemIndex - 1;
                }
                d.newParent = targetInternalItem->getParent();

                break;
            }
            case QAbstractItemView::BelowItem: {
                assert(targetInternalItem);
                int targetItemIndex = targetInternalItem->getIndexInParent();

                assert(d.indexInOldParent != -1 && targetItemIndex != -1);

                // If the dropped item is already into the children and before the found index don't decrement
                if (d.indexInOldParent < targetItemIndex) {
                    d.indexInNewParent = targetItemIndex;
                } else {
                    // The item "below" in the tree is index + 1 in the internal list which is ordered from bottom to top
                    d.indexInNewParent = targetItemIndex + 1;
                }
                d.newParent = targetInternalItem->getParent();

                dndItems.push_back(d);
                break;
            }
            case QAbstractItemView::OnItem: {
                // Only allow dropping when the layout is a tree
                if (_imp->internalModel.lock()->getType() == KnobItemsTable::eKnobItemsTableTypeTree) {
                    // always insert on-top of others
                    d.indexInNewParent = 0;
                    d.newParent = targetInternalItem;
                } else {
                    continue;
                }
                break;
            }
            case QAbstractItemView::OnViewport: {
                // Only allow dragging on viewport for tables
                if (_imp->internalModel.lock()->getType() == KnobItemsTable::eKnobItemsTableTypeTable) {
                    // Append the top-level item
                    d.indexInNewParent = -1;
                } else {
                    continue;
                }
                break;
            }
        }
        dndItems.push_back(d);

    }
    if (!dndItems.empty()) {
        _imp->panel->pushUndoCommand(new DragItemsUndoCommand(_imp, originalTable, dndItems));
    }

    
} // dropEvent



class PasteItemUndoCommand
: public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(PasteItemUndoCommand)

public:


    struct PastedItem
    {
        QTreeWidgetItem* treeItem;
        RotoItemPtr rotoItem;
        RotoItemPtr itemCopy;
    };


    PasteItemUndoCommand(KnobItemsTableGuiPrivate* table,
                         const KnobTableItemPtr& target,
                         const SERIALIZATION_NAMESPACE::KnobItemsTableSerialization& source)
    : QUndoCommand()
    , _table(table)
    , _targetItem(target)
    , _originalTargetItemSerialization()
    , _sourceItemsCopies()
    , _sourceItemSerialization()
    {
        // Make sure tables match content type
        KnobItemsTablePtr model = table->internalModel.lock();
        assert(source.tableIdentifier == model->getTableIdentifier());
        
        // Remember the state of the target item
        if (target) {
            target->toSerialization(&_originalTargetItemSerialization);
        }
        
        // If this is a tree and the item can receive children, add as sub children
        if ((!target || target->isItemContainer()) && model->getType() == KnobItemsTable::eKnobItemsTableTypeTree) {
            for (std::list<SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr>::const_iterator it = source.items.begin(); it!=source.items.end(); ++it) {
                KnobTableItemPtr copy = model->createItemFromSerialization(*it);
                if (copy) {
                    _sourceItemsCopies.push_back(copy);
                }
            }
        } else {
            // Should have been caught earlier with a nice error dialog for the user:
            // You cannot paste multiple items onto one
            assert(source.items.size() == 1);
            _sourceItemSerialization = source.items.front();
        }
        setText(tr("Paste Item(s)"));
    }

    virtual ~PasteItemUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    KnobItemsTableGuiPrivate* _table;
    KnobTableItemPtr _targetItem;
    SERIALIZATION_NAMESPACE::KnobTableItemSerialization _originalTargetItemSerialization;
    
    // Only used when pasting multiple items as children of a container
    std::list<KnobTableItemPtr> _sourceItemsCopies;
    
    // Only used when pasting an item onto another one
    SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr _sourceItemSerialization;
};

void
PasteItemUndoCommand::redo()
{
    if (_sourceItemSerialization) {
        // We paste 1 item onto another
        assert(_targetItem);
        _targetItem->fromSerialization(*_sourceItemSerialization);
    } else {
        // We paste multiple items as children of a container
        for (std::list<KnobTableItemPtr>::const_iterator it = _sourceItemsCopies.begin(); it!=_sourceItemsCopies.end(); ++it) {
            _targetItem->addChild(*it, eTableChangeReasonInternal);
        }
    }
}

void
PasteItemUndoCommand::undo()
{
    if (_sourceItemSerialization) {
        // We paste 1 item onto another
        assert(_targetItem);
        _targetItem->fromSerialization(_originalTargetItemSerialization);
    } else {
        // We paste multiple items as children of a container
        for (std::list<KnobTableItemPtr>::const_iterator it = _sourceItemsCopies.begin(); it!=_sourceItemsCopies.end(); ++it) {
            _targetItem->removeChild(*it, eTableChangeReasonInternal);
        }
    }
}

class DuplicateItemUndoCommand
: public QUndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(DuplicateItemUndoCommand)

public:


    DuplicateItemUndoCommand(KnobItemsTableGuiPrivate* table,
                             const std::list<KnobTableItemPtr>& items)
    : QUndoCommand()
    , _table(table)
    , _items()
    {
        for (std::list<KnobTableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
            SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr s(new SERIALIZATION_NAMESPACE::KnobTableItemSerialization);
            (*it)->toSerialization(s.get());
            KnobTableItemPtr dup = table->internalModel.lock()->createItemFromSerialization(s);
            assert(dup);
            _duplicates.push_back(dup);
        }
        
        
        
    }

    virtual ~DuplicateItemUndoCommand();

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    
private:
    

    KnobItemsTableGuiPrivate* _table;
    std::list<KnobTableItemPtr> _items, _duplicates;
};

void
DuplicateItemUndoCommand::redo()
{
    assert(_duplicates.size() == _items.size());
    std::list<KnobTableItemPtr>::const_iterator ito = _items.begin();
    for (std::list<KnobTableItemPtr>::const_iterator it = _duplicates.begin(); it != _duplicates.end(); ++it, ++ito) {
        int itemIndex = (*ito)->getIndexInParent();
        itemIndex += 1;
        KnobTableItemPtr parent = (*ito)->getParent();
        if (parent) {
            parent->insertChild(itemIndex, *it, eTableChangeReasonInternal);
        } else {
            _table->internalModel.lock()->insertTopLevelItem(itemIndex, *it, eTableChangeReasonInternal);
        }
    }
    
}

void
DuplicateItemUndoCommand::undo()
{
    assert(_duplicates.size() == _items.size());
    for (std::list<KnobTableItemPtr>::const_iterator it = _duplicates.begin(); it != _duplicates.end(); ++it) {
        _table->internalModel.lock()->removeItem(*it, eTableChangeReasonInternal);
    }
}

void
KnobItemsTableGui::onDeleteItemsActionTriggered()
{
    std::list<KnobTableItemPtr> selection = _imp->internalModel.lock()->getSelectedItems();
    if (selection.empty()) {
        return;
    }
    pushUndoCommand(new RemoveItemsUndoCommand(_imp.get(), selection));
}

void
KnobItemsTableGui::onCopyItemsActionTriggered()
{
    KnobItemsTablePtr model = _imp->internalModel.lock();
    std::list<KnobTableItemPtr> selection = model->getSelectedItems();
    if (selection.empty()) {
        return;
    }
    SERIALIZATION_NAMESPACE::KnobItemsTableSerialization obj;
    obj.tableIdentifier = model->getTableIdentifier();
    obj.nodeScriptName = model->getNode()->getFullyQualifiedName();
    for (std::list<KnobTableItemPtr>::const_iterator it = selection.begin(); it!=selection.end(); ++it) {
        SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr s(new SERIALIZATION_NAMESPACE::KnobTableItemSerialization);
        (*it)->toSerialization(s.get());
        obj.items.push_back(s);
    }
    
    std::ostringstream ss;
    try {
        SERIALIZATION_NAMESPACE::write(ss, obj);
    } catch (...) {
        return;
    }
    
    QMimeData* mimedata = new QMimeData;
    QByteArray data( ss.str().c_str() );
    mimedata->setData(QLatin1String("text/plain"), data);
    QClipboard* clipboard = QApplication::clipboard();
    
    // Ownership is transferred to the clipboard
    clipboard->setMimeData(mimedata);
}

void
KnobItemsTableGui::onPasteItemsActionTriggered()
{
    KnobItemsTablePtr model = _imp->internalModel.lock();
    std::list<KnobTableItemPtr> selection = model->getSelectedItems();

    if (selection.size() > 1) {
        Dialogs::errorDialog(tr("Paste").toStdString(), tr("You can only copy an item onto another one or on the view itself").toStdString());
        return;
    }
    
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* data = clipboard->mimeData();
    if (!data) {
        return;
    }
    std::string str;
    {
        QByteArray array = data->data(QLatin1String("text/plain"));
        if (array.data()) {
            str = std::string(array.data());
        }
    }
    
    std::istringstream ss(str);
    SERIALIZATION_NAMESPACE::KnobItemsTableSerialization obj;
    try {
        SERIALIZATION_NAMESPACE::read(ss, &obj);
    } catch (...) {
        Dialogs::errorDialog(tr("Paste").toStdString(), tr("You cannot copy this kind of data here").toStdString());
        return;
    }
    
    // Check that table is of the same type
    if (obj.tableIdentifier != model->getTableIdentifier()) {
        Dialogs::errorDialog(tr("Paste").toStdString(), tr("You cannot copy this kind of data here").toStdString());
        return;
    }
    
    KnobTableItemPtr target;
    if (!selection.empty()) {
        target = selection.front();
    }
    
    if (obj.items.size() > 1 && target && !target->isItemContainer()) {
        Dialogs::errorDialog(tr("Paste").toStdString(), tr("%1 is not a container, you can only copy a single item onto it").arg(QString::fromUtf8(target->getScriptName_mt_safe().c_str())).toStdString());
        return;
    }
    
    pushUndoCommand(new PasteItemUndoCommand(_imp.get(), target, obj));
    
    
}

void
KnobItemsTableGui::onCutItemsActionTriggered()
{
    onCopyItemsActionTriggered();
    onDeleteItemsActionTriggered();
    
}

void
KnobItemsTableGui::onDuplicateItemsActionTriggered()
{
    std::list<KnobTableItemPtr> selection = _imp->internalModel.lock()->getSelectedItems();
    if (selection.empty()) {
        return;
    }
    pushUndoCommand(new DuplicateItemUndoCommand(_imp.get(), selection));
}


void
KnobItemsTableGui::onTableItemRightClicked(const QPoint& globalPos, const TableItemPtr& item)
{
    _imp->showItemMenu(item, globalPos);
}

void
KnobItemsTableGui::onTableItemDataChanged(const TableItemPtr& item)
{

}


void
KnobItemsTableGui::onSelectAllItemsActionTriggered()
{
    
}

void
KnobItemsTableGuiPrivate::selectionFromIndexList(const QModelIndexList& indexes, std::list<KnobTableItemPtr>* outItems)
{
    std::set<KnobTableItemPtr> uniqueItems;
    for (QModelIndexList::const_iterator it = indexes.begin(); it != indexes.end(); ++it) {
        //Check that the item is valid
        assert(it->isValid() && it->row() >= 0 && it->row() < (int)items.size() && it->column() >= 0 && it->column() < tableModel->columnCount());
        
        // Get the table item corresponding to the index
        TableItemPtr tableItem = tableModel->getItem(it->row());
        assert(tableItem);
        
        // Get the internal knobtableitem corresponding to the table item
        ModelItemsVec::iterator found = findItem(tableItem);
        assert(found != items.end());
        if (found == items.end()) {
            continue;
        }
        
        KnobTableItemPtr internalItem = found->internalItem.lock();
        assert(internalItem);
        if (!internalItem) {
            continue;
        }
        uniqueItems.insert(internalItem);
    }
    
    for (std::set<KnobTableItemPtr>::iterator it = uniqueItems.begin(); it != uniqueItems.end(); ++it) {
        outItems->push_back(*it);
    }
}

void
KnobItemsTableGuiPrivate::itemsToSelection(const std::list<KnobTableItemPtr>& inItems, QItemSelection* selection)
{
    
    for (std::list<KnobTableItemPtr>::const_iterator it = inItems.begin(); it!=inItems.end(); ++it) {
        ModelItemsVec::iterator found = findItem(*it);
        assert(found != items.end());
        if (found == items.end()) {
            continue;
        }
        assert(!found->columnItems.empty());
        int row_i = found->item->getRowInParent();
        QModelIndex leftMost = tableModel->index(row_i, 0);
        QModelIndex rightMost = tableModel->index(row_i, tableModel->columnCount() - 1);
        QItemSelectionRange t(leftMost, rightMost);
        for (std::size_t i = 0; i < found->columnItems.size(); ++i) {
            selection->append(t);
        }
    }
}

void
KnobItemsTableGuiPrivate::selectionToItems(const QItemSelection& selection, std::list<KnobTableItemPtr>* items)
{
    QModelIndexList indexes = selection.indexes();
    selectionFromIndexList(indexes, items);
}

void
KnobItemsTableGui::onModelSelectionChanged(const QItemSelection& selected,const QItemSelection& deselected)
{
    // Watch for recursion
    if (_imp->selectingModelRecursion) {
        return;
    }
    
    // Convert indexes to items
    std::list<KnobTableItemPtr> deselectedItems, selectedItems;
    _imp->selectionToItems(selected, &selectedItems);
    _imp->selectionToItems(deselected, &deselectedItems);

#pragma message WARN("refresh keyframes here")

    // Select the items in the model internally
    KnobItemsTablePtr model = _imp->internalModel.lock();
    model->beginEditSelection();
    model->removeFromSelection(selectedItems, eTableChangeReasonPanel);
    model->addToSelection(selectedItems, eTableChangeReasonPanel);
    model->endEditSelection(eTableChangeReasonPanel);
}

void
KnobItemsTableGui::onModelSelectionChanged(const std::list<KnobTableItemPtr>& addedToSelection, const std::list<KnobTableItemPtr>& removedFromSelection, TableChangeReasonEnum reason)
{
    if (reason == eTableChangeReasonPanel) {
        // Do not recurse
        return;
    }
    
#pragma message WARN("refresh keyframes here")

    // Refresh the view
    QItemSelection selectionToAdd, selectionToRemove;
    _imp->itemsToSelection(addedToSelection, &selectionToAdd);
    _imp->itemsToSelection(removedFromSelection, &selectionToRemove);
    QItemSelectionModel* selectionModel = _imp->tableView->selectionModel();
    
    // Ensure we don't recurse indefinitely in the selection
    ++_imp->selectingModelRecursion;
    selectionModel->select(selectionToRemove, QItemSelectionModel::Deselect);
    selectionModel->select(selectionToAdd, QItemSelectionModel::Select);
    --_imp->selectingModelRecursion;
    
}

void
KnobItemsTableGui::onModelTopLevelItemRemoved(const KnobTableItemPtr& item, TableChangeReasonEnum reason)
{
    
}

void
KnobItemsTableGui::onModelTopLevelItemInserted(int index, const KnobTableItemPtr& item, TableChangeReasonEnum reason)
{
    
}

void
KnobItemsTableGui::onItemLabelChanged(const QString& label, TableChangeReasonEnum reason)
{
    
}
void
KnobItemsTableGui::onItemChildRemoved(const KnobTableItemPtr& item, TableChangeReasonEnum reason)
{
    
}

void
KnobItemsTableGui::onItemChildInserted(int index, const KnobTableItemPtr& item, TableChangeReasonEnum reason)
{
    
}

static QString
labelToolTipFromScriptName(const KnobTableItemPtr& item)
{
    return ( QString::fromUtf8("<p><b>")
            + QString::fromUtf8( item->getScriptName_mt_safe().c_str() )
            + QString::fromUtf8("</b></p>")
            +  NATRON_NAMESPACE::convertFromPlainText(QCoreApplication::translate("KnobItemsTableGui", "The label of the item"), NATRON_NAMESPACE::WhiteSpaceNormal) );
}

void
KnobItemsTableGuiPrivate::createTableItems(const KnobTableItemPtr& item)
{
    // The item should not exist in the table GUI yet.
    assert(findItem(item) == items.end());
    
    int itemRow = item->getItemRow();
    
    int nCols = tableModel->columnCount();
    
    ModelItem mitem;
    mitem.columnItems.resize(nCols);
    mitem.item = TableItem::create();
    
    TableItemPtr parentItem;
    KnobTableItemPtr knobParentItem = item->getParent();
    if (knobParentItem) {
        ModelItemsVec::iterator foundParent = findItem(knobParentItem);
        assert(foundParent != items.end());
        if (foundParent != items.end()) {
            parentItem = foundParent->item;
        }
    }
    if (parentItem) {
        parentItem->insertChild(itemRow, mitem.item);
    } else {
        tableModel->insertTopLevelItem(itemRow, mitem.item);
    }

    for (int i = 0; i < nCols; ++i) {
        
        ModelItem::ColumnData d;
        // If this column represents a knob, this is the knob
        d.knob = item->getColumnKnob(i, &d.knobDimension);
        

        
        if (d.knob.lock()) {
            // If we have a knob, create the custom widget
            createItemCustomWidgetAtCol(item, itemRow, i);
        } else {
            // Ok the column must be kKnobTableItemColumnLabel
            // otherwise we don't know what the user want
            std::string columnID = item->getColumnName(i);
            assert(columnID == kKnobTableItemColumnLabel);
            mitem.labelColIndex = i;
            mitem.item->setToolTip(i, labelToolTipFromScriptName(item) );
            mitem.item->setFlags(i, Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
            mitem.item->setText(i, QString::fromUtf8( item->getLabel().c_str() ) );
            
        }
        
        tableView->resizeColumnToContents(i);
        mitem.columnItems[i] = d;

    }

    // Create custom widgets for knobs
    createCustomWidgetRecursively(item);

}

NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING
#include "moc_KnobItemsTableGui.cpp"
