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


// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "KnobItemsTableGui.h"

#include <map>
#include <sstream> // stringstream

#include <QApplication>
#include <QDebug>
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
#include <QPixmapCache>
#include <QWidget>
#include <QTextLayout>

#include "Engine/Utils.h"
#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/Button.h"
#include "Gui/ComboBox.h"
#include "Gui/ClickableLabel.h"
#include "Gui/DockablePanel.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobGuiButton.h"
#include "Gui/KnobGuiChoice.h"
#include "Gui/KnobGuiValue.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiMacros.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/Menu.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/SpinBox.h"
#include "Gui/TableModelView.h"

#include "Global/StrUtils.h"


#include "Serialization/KnobTableItemSerialization.h"
#include "Serialization/SerializationIO.h"

#define kNatronKnobItemsTableGuiMimeType "Natron/NatronKnobItemsTableGuiMimeType"
NATRON_NAMESPACE_ENTER

struct ModelItem {
    // The internal item in Engine
    KnobTableItemWPtr internalItem;

    struct ColumnData
    {
        KnobIWPtr knob;
        KnobGuiPtr guiKnob;
        DimSpec knobDimension;
        
        ColumnData()
        : knob()
        , guiKnob()
        , knobDimension(DimSpec::all())
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

    KnobTableItemWPtr rightClickedItem;

    TableModelPtr tableModel;
    TableView* tableView;
    boost::scoped_ptr<TableItemEditorFactory> itemEditorFactory;

    ModelItemsVec items;

    // Prevent recursion from selectionChanged signal of QItemSelectionModel
    int selectingModelRecursion;

    // The main layout containing table + other buttons
    QLayout* containerLayout;


    KnobItemsTableGuiPrivate(KnobItemsTableGui* publicInterface, DockablePanel* panel, const KnobItemsTablePtr& table)
    : _publicInterface(publicInterface)
    , internalModel(table)
    , panel(panel)
    , tableModel()
    , tableView(0)
    , itemEditorFactory()
    , items()
    , selectingModelRecursion(0)
    , containerLayout(0)
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

    bool createItemCustomWidgetAtCol(const KnobTableItemPtr& item, int row, int col);

    void createCustomWidgetRecursively(const KnobTableItemPtr& item);
    
    void selectionFromIndexList(const QModelIndexList& indexes, std::list<KnobTableItemPtr>* items);
    
    void selectionToItems(const QItemSelection& selection, std::list<KnobTableItemPtr>* items);
    
    void itemsToSelection(const std::list<KnobTableItemPtr>& items, QItemSelection* selection);

    void createTableItems(const KnobTableItemPtr& item);

    void removeTableItem(const KnobTableItemPtr& item);

    void recreateItemsFromModel();

    void createItemsVecRecursive(const std::vector<KnobTableItemPtr>& items);

    void setItemIcon(const TableItemPtr& tableItem, int col, const KnobTableItemPtr& item);

    void refreshKnobsSelectionState(const std::list<KnobTableItemPtr>& selected, const std::list<KnobTableItemPtr>& deselected);

    void refreshKnobItemSelection(const KnobTableItemPtr& item, bool selected);

};

bool
KnobItemsTableGuiPrivate::createItemCustomWidgetAtCol(const KnobTableItemPtr& item, int row, int col)
{
    DimSpec dim;
    KnobIPtr knob = item->getColumnKnob(col, &dim);
    if (!knob) {
        return false;
    }

    ModelItemsVec::iterator foundItem = findItem(item);
    assert(foundItem != items.end());
    if (foundItem == items.end()) {
        return false;
    }
    assert(col >= 0 && col < (int)foundItem->columnItems.size());

    // destroy existing KnobGui
    foundItem->columnItems[col].guiKnob.reset();

    // Create the Knob Gui
    foundItem->columnItems[col].guiKnob = KnobGui::create(knob, KnobGui::eKnobLayoutTypeTableItemWidget, _publicInterface);
    if (!dim.isAll()) {
        foundItem->columnItems[col].guiKnob->setSingleDimensionalEnabled(true, DimIdx(dim));
    }

    QWidget* container = new QWidget(tableView);
    QHBoxLayout* containerLayout = new QHBoxLayout(container);

    {
        double bgColor[3];
        appPTR->getCurrentSettings()->getRaisedColor(&bgColor[0], &bgColor[1], &bgColor[2]);
        QColor c;
        c.setRgbF(Image::clamp(bgColor[0], 0., 1.), Image::clamp(bgColor[1], 0., 1.), Image::clamp(bgColor[2], 0., 1.));
        QString rgbaStr = QString::fromUtf8("rgb(%1,%2,%3)").arg(c.red()).arg(c.green()).arg(c.blue());
        container->setStyleSheet(QString::fromUtf8("QWidget {background-color: %1}").arg(rgbaStr));
    }
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    KnobChoicePtr isChoice = toKnobChoice(knob);
    KnobColorPtr isColor = toKnobColor(knob);

    Qt::Alignment align;
    if (isChoice || isColor) {
        // Align combobox on the left and
        align = (Qt::AlignLeft | Qt::AlignVCenter);
    } else {
        align = Qt::AlignCenter;
    }
    
    containerLayout->setAlignment(align);
    foundItem->columnItems[col].guiKnob->createGUI(container);
    containerLayout->addWidget(foundItem->columnItems[col].guiKnob->getFieldContainer());


    KnobGuiWidgetsPtr firstViewWidgets = foundItem->columnItems[col].guiKnob->getWidgetsForView(ViewIdx(0));
    assert(firstViewWidgets);

    if (isChoice) {
        // remove the shape of the combobox so it looks integrated
        KnobGuiChoice* widget = dynamic_cast<KnobGuiChoice*>(firstViewWidgets.get());
        assert(widget);
        widget->getCombobox()->setDrawShapeEnabled(false);
    }
    KnobGuiButton* isButton = dynamic_cast<KnobGuiButton*>(firstViewWidgets.get());
    KnobGuiValue* isValue = dynamic_cast<KnobGuiValue*>(firstViewWidgets.get());
    if (isButton) {
        // For buttons, just show the icon
        isButton->disableButtonBorder();
    } else if (isValue) {
        // For spinboxes, disable border
        isValue->disableSpinBoxesBorder();
    }

    QModelIndex itemIdx = tableModel->getItemIndex(foundItem->item);

    // Set the widget
    {
        ModelItemsVec::iterator foundItem = findItem(item);
        assert((int)foundItem->columnItems.size() == tableModel->columnCount());
        tableView->setCellWidget(row, col, itemIdx.parent(), container);
    }
    
    return true;
} // createItemCustomWidgetAtCol

QWidget*
KnobItemsTableGui::createKnobHorizontalFieldContainer(QWidget* parent) const
{
    return new QWidget(parent);
}

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



static QSizeF viewItemTextLayout(QTextLayout &textLayout, int lineWidth)
{
    qreal height = 0;
    qreal widthUsed = 0;
    textLayout.beginLayout();
    while (true) {
        QTextLine line = textLayout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(lineWidth);
        line.setPosition(QPointF(0, height));
        height += line.height();
        widthUsed = qMax(widthUsed, line.naturalTextWidth());
    }
    textLayout.endLayout();
    return QSizeF(widthUsed, height);
}

static void drawItemText(QPainter *p, const QStyleOptionViewItemV4 *option, const QRect &rect, QStyle* style)
{
    const QWidget *widget = option->widget;
    const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, widget) + 1;

    QRect textRect = rect.adjusted(textMargin, 0, -textMargin, 0); // remove width padding
    const bool wrapText = option->features & QStyleOptionViewItemV2::WrapText;
    QTextOption textOption;
    textOption.setWrapMode(wrapText ? QTextOption::WordWrap : QTextOption::ManualWrap);
    textOption.setTextDirection(option->direction);
    textOption.setAlignment(QStyle::visualAlignment(option->direction, option->displayAlignment));
    QTextLayout textLayout;
    textLayout.setTextOption(textOption);
    textLayout.setFont(option->font);
    textLayout.setText(option->text);

    viewItemTextLayout(textLayout, textRect.width());

    QString elidedText;
    qreal height = 0;
    qreal width = 0;
    int elidedIndex = -1;
    const int lineCount = textLayout.lineCount();
    for (int j = 0; j < lineCount; ++j) {
        const QTextLine line = textLayout.lineAt(j);
        if (j + 1 <= lineCount - 1) {
            const QTextLine nextLine = textLayout.lineAt(j + 1);
            if ((nextLine.y() + nextLine.height()) > textRect.height()) {
                int start = line.textStart();
                int length = line.textLength() + nextLine.textLength();
                //const QStackTextEngine engine(textLayout.text().mid(start, length), option->font);
                elidedText = textLayout.text().mid(start, length); //engine.elidedText(option->textElideMode, textRect.width());
                height += line.height();
                width = textRect.width();
                elidedIndex = j;
                break;
            }
        }
        if (line.naturalTextWidth() > textRect.width()) {
            int start = line.textStart();
            int length = line.textLength();
            //const QStackTextEngine engine(textLayout.text().mid(start, length), option->font);
            elidedText = textLayout.text().mid(start, length);//engine.elidedText(option->textElideMode, textRect.width());
            height += line.height();
            width = textRect.width();
            elidedIndex = j;
            break;
        }
        width = qMax<qreal>(width, line.width());
        height += line.height();
    }

    const QRect layoutRect = QStyle::alignedRect(option->direction, option->displayAlignment,
                                                 QSize(int(width), int(height)), textRect);
    const QPointF position = layoutRect.topLeft();
    for (int i = 0; i < lineCount; ++i) {
        const QTextLine line = textLayout.lineAt(i);
        if (i == elidedIndex) {
            qreal x = position.x() + line.x();
            qreal y = position.y() + line.y() + line.ascent();
            p->save();
            p->setFont(option->font);
            p->drawText(QPointF(x, y), elidedText);
            p->restore();
            break;
        }
        line.draw(p, position);
    }
} // drawItemText

void
AnimatedKnobItemDelegate::paint(QPainter * painter,
                                const QStyleOptionViewItem & option,
                                const QModelIndex & index) const
{

    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);

    // Paint the label column
    // Use the selection color for the text color

    painter->save();


    const QWidget* widget = opt.widget;
    QStyle* style = _imp->tableView->style();


    // draw the icon
    if (!opt.icon.isNull()) {
        QRect iconRect = style->subElementRect(QStyle::SE_ItemViewItemDecoration, &opt, widget);
        QIcon::Mode mode = QIcon::Normal;
        if (!(opt.state & QStyle::State_Enabled)) {
            mode = QIcon::Disabled;
        } else if (opt.state & QStyle::State_Selected) {
            mode = QIcon::Selected;
        }
        QIcon::State state = opt.state & QStyle::State_Open ? QIcon::On : QIcon::Off;
        opt.icon.paint(painter, iconRect, opt.decorationAlignment, mode, state);
    }

    // Set pen color
    {
        double textColor[3];
        if (opt.state & QStyle::State_Selected) {
            appPTR->getCurrentSettings()->getSelectionColor(&textColor[0], &textColor[1], &textColor[2]);
        } else {
            appPTR->getCurrentSettings()->getTextColor(&textColor[0], &textColor[1], &textColor[2]);
        }


        QColor c;
        c.setRgbF(textColor[0], textColor[1], textColor[2]);
        QPen pen = painter->pen();
        pen.setColor(c);
        painter->setPen(pen);
    }

    // draw the text
    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, widget);
    drawItemText(painter, &opt, textRect, style);

    painter->restore();


} // paint

class KnobItemsTableView : public TableView
{
    KnobItemsTableGuiPrivate* _imp;
public:

    KnobItemsTableView(KnobItemsTableGuiPrivate* imp, Gui* gui, QWidget* parent)
    : TableView(gui, parent)
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
                                        const std::list<TableItemPtr>& rows,
                                        Qt::DropActions supportedActions,
                                        Qt::DropAction defaultAction) OVERRIDE FINAL;

    virtual void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const OVERRIDE FINAL;
    virtual void drawRow(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const OVERRIDE FINAL;
};

class KnobItemsTableModel : public TableModel
{
private:

    KnobItemsTableModel(int cols, TableModelTypeEnum type)
    : TableModel(cols, type)
    {

    }

public:

    virtual QStringList mimeTypes() const OVERRIDE FINAL
    {
        QStringList ret;
        ret.push_back(QLatin1String(kNatronKnobItemsTableGuiMimeType));
        return ret;
    }

    static TableModelPtr create(int columns, TableModelTypeEnum type)
    {
        return TableModelPtr(new KnobItemsTableModel(columns, type));
    }

    virtual ~KnobItemsTableModel()
    {

    }

};

KnobItemsTableGui::KnobItemsTableGui(const KnobItemsTablePtr& table, DockablePanel* panel, QWidget* parent)
: _imp(new KnobItemsTableGuiPrivate(this, panel, table))
{

    setContainerWidget(panel);

    _imp->tableView = new KnobItemsTableView(_imp.get(), panel->getGui(), parent);

    connect(_imp->tableView, SIGNAL(itemRightClicked(QPoint,TableItemPtr)), this, SLOT(onViewItemRightClicked(QPoint,TableItemPtr)));

#if QT_VERSION < 0x050000
    _imp->tableView->header()->setResizeMode(QHeaderView::ResizeToContents);
#else
    _imp->tableView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
    _imp->tableView->header()->setStretchLastSection(true);

    AnimatedKnobItemDelegate* delegate = new AnimatedKnobItemDelegate(_imp.get());
    _imp->itemEditorFactory.reset(new TableItemEditorFactory);
    delegate->setItemEditorFactory( _imp->itemEditorFactory.get() );
    _imp->tableView->setItemDelegate(delegate);

    int nCols = table->getColumnsCount();

    KnobItemsTable::KnobItemsTableTypeEnum knobTableType = table->getType();
    TableModel::TableModelTypeEnum type = TableModel::eTableModelTypeTable;
    switch (knobTableType) {
        case KnobItemsTable::eKnobItemsTableTypeTable:
            type = TableModel::eTableModelTypeTable;
            break;
        case KnobItemsTable::eKnobItemsTableTypeTree:
            type = TableModel::eTableModelTypeTree;
            break;
    }
    _imp->tableModel = KnobItemsTableModel::create(nCols, type);
    QObject::connect( _imp->tableModel.get(), SIGNAL(itemDataChanged(TableItemPtr,int, int)), this, SLOT(onTableItemDataChanged(TableItemPtr,int, int)) );
    _imp->tableView->setTableModel(_imp->tableModel);

    QItemSelectionModel *selectionModel = _imp->tableView->selectionModel();
    QObject::connect( selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this,
                     SLOT(onModelSelectionChanged(QItemSelection,QItemSelection)) );



    std::vector<QString> headerLabels(nCols), headerIcons(nCols), headerTooltips(nCols);
    for (int i = 0; i < nCols; ++i) {
        headerIcons[i] = QString::fromUtf8(table->getColumnIcon(i).c_str());
        headerLabels[i] = QString::fromUtf8(table->getColumnText(i).c_str());
        headerTooltips[i] = QString::fromUtf8(table->getColumnTooltip(i).c_str());
    }

    QString iconsPath = QString::fromUtf8(table->getIconsPath().c_str());
    StrUtils::ensureLastPathSeparator(iconsPath);

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
    for (int i = 0; i < nCols; ++i) {
        _imp->tableModel->setHeaderData(i, Qt::Horizontal, QVariant(headerTooltips[i]), Qt::ToolTipRole);
    }

    _imp->tableView->setUniformRowHeights(table->getRowsHaveUniformHeight());
    _imp->tableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);


    if (knobTableType == KnobItemsTable::eKnobItemsTableTypeTree) {
        _imp->tableView->setItemsExpandable(true);
        _imp->tableView->setRootIsDecorated(true);
        _imp->tableView->setExpandsOnDoubleClick(true);
    }

    bool dndSupported = table->isDragAndDropSupported();
    _imp->tableView->setDragEnabled(dndSupported);
    _imp->tableView->setAcceptDrops(dndSupported);

    if (dndSupported) {
        // We move items only
        //_imp->tableModel->setSupportedDragActions(Qt::MoveAction);
        //_imp->tableView->setDefaultDropAction(Qt::MoveAction);
        if (table->isDropFromExternalSourceSupported()) {
            _imp->tableView->setDragDropMode(QAbstractItemView::DragDrop);
        } else {
            _imp->tableView->setDragDropMode(QAbstractItemView::InternalMove);
        }
    }

    
    _imp->recreateItemsFromModel();

    connect(table.get(), SIGNAL(selectionChanged(std::list<KnobTableItemPtr>,std::list<KnobTableItemPtr>,TableChangeReasonEnum)), this, SLOT(onModelSelectionChanged(std::list<KnobTableItemPtr>,std::list<KnobTableItemPtr>,TableChangeReasonEnum)));
    connect(table.get(), SIGNAL(itemRemoved(KnobTableItemPtr,TableChangeReasonEnum)), this, SLOT(onModelItemRemoved( KnobTableItemPtr,TableChangeReasonEnum)));
    connect(table.get(), SIGNAL(itemInserted(int,KnobTableItemPtr,TableChangeReasonEnum)), this, SLOT(onModelItemInserted(int,KnobTableItemPtr,TableChangeReasonEnum)));



} // KnobItemsTableGui::KnobItemsTableGui

KnobItemsTableGui::~KnobItemsTableGui()
{
    
}

QLayout*
KnobItemsTableGui::getContainerLayout() const
{
    return _imp->containerLayout;
}

void
KnobItemsTableGui::addWidgetsToLayout(QLayout* layout)
{

    _imp->containerLayout = layout;
    QGridLayout* isGridLayout = dynamic_cast<QGridLayout*>(layout);
    QBoxLayout* isBoxLayout = dynamic_cast<QBoxLayout*>(layout);
    if (isGridLayout) {
        int rc = isGridLayout->rowCount();
        isGridLayout->addWidget(_imp->tableView, rc + 1, 0, 1, 2);
    } else if (isBoxLayout) {
        isBoxLayout->addWidget(_imp->tableView);
    } else {
        assert(false);
    }

}


std::vector<KnobGuiPtr>
KnobItemsTableGui::getKnobsForItem(const KnobTableItemPtr& item) const
{
    std::vector<KnobGuiPtr> ret;
    ModelItemsVec::const_iterator foundItem = _imp->findItem(item);
    if (foundItem == _imp->items.end()) {
        return ret;
    }
    for (std::size_t i = 0; i < foundItem->columnItems.size(); ++i) {
        if (foundItem->columnItems[i].guiKnob) {
            ret.push_back(foundItem->columnItems[i].guiKnob);
        }
    }
    return ret;
}

void
KnobItemsTableView::drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const
{
    QStyleOptionViewItemV2 opt = viewOptions();
    QStyle::State extraFlags = QStyle::State_None;
    if (isEnabled()) {
        extraFlags |= QStyle::State_Enabled;
    }
    if (window()->isActiveWindow()) {
        extraFlags |= QStyle::State_Active;
    }
    QPoint oldBO = painter->brushOrigin();
    if (verticalScrollMode() == QAbstractItemView::ScrollPerPixel) {
        painter->setBrushOrigin(QPoint(0, verticalOffset()));
    }

    const int indent = indentation();
    QRect primitive(rect.right() + 1, rect.top(), indent, rect.height());



    TableItemPtr item = _imp->tableModel->getItem(index);
    assert(item);
    bool hasChildren = item->getChildren().size() > 0;
    bool expanded = isExpanded(index);
    int nIteration = 0;
    do {

        QStyleOptionViewItemV2 itemOpt = opt;
        itemOpt.state &= ~QStyle::State_MouseOver;
        itemOpt.state &= ~QStyle::State_Selected;
        itemOpt.state |= extraFlags;

        primitive.moveLeft(primitive.left() - indent);
        itemOpt.rect = primitive;

        TableItemPtr parentItem = item->getParentItem();
        /*bool moreSiblings = false;
        if (parentItem) {
            moreSiblings = parentItem->getChildren().size() > 1 && item->getRowInParent() < (int)parentItem->getChildren().size() - 1;
        }*/

        if (nIteration == 0) {
            itemOpt.state |= (expanded ? QStyle::State_Open : QStyle::State_None);
            itemOpt.state |= (hasChildren ? QStyle::State_Children : QStyle::State_None);
        }

        style()->drawPrimitive(QStyle::PE_IndicatorBranch, &itemOpt, painter, this);
        item = item->getParentItem();
        ++nIteration;

    } while (item);
    painter->setBrushOrigin(oldBO);


} // drawBranches

void
KnobItemsTableView::drawRow(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    // The full row rectangle
    QRect rowRect = option.rect;

    // The rectangle at which the item starts if it is a child of another item
    QRect itemRect = visualRect(index);

    QRect branchRect( 0, rowRect.y(), itemRect.x(), rowRect.height() );


    TableItemPtr item = _imp->tableModel->getItem(index);
    assert(item);
    ModelItemsVec::iterator found = _imp->findItem(item);
    assert(found != _imp->items.end());

    // Draw the label section which is the only one using a regular item and not a KnobGui
    KnobTableItemPtr internalItem = found->internalItem.lock();
    KnobItemsTablePtr model = internalItem->getModel();
    if (!model) {
        return;
    }
    bool isSelected = model->isItemSelected(internalItem);
    int labelCol = internalItem->getLabelColumnIndex();
    int xOffset = itemRect.x();
    {
        int colX = columnViewportPosition(labelCol);
        int colW = header()->sectionSize(labelCol);

        int x;
        if (labelCol == 0) {
            x = xOffset + colX;
        } else {
            x = colX;
        }
        QStyleOptionViewItemV4 opt = option;

        opt.rect.setRect(x, option.rect.y(), colW, option.rect.height());
        style()->drawPrimitive(QStyle::PE_PanelItemViewRow, &opt, painter, this);

        if (isSelected) {
            opt.state |= QStyle::State_Selected;
        }
        QModelIndex modelIndex = _imp->tableModel->index(index.row(), labelCol, index.parent());
        itemDelegate(modelIndex)->paint(painter, opt, modelIndex);
    }
    

    drawBranches(painter, branchRect, index);

}

void
KnobItemsTableView::setupAndExecDragObject(QDrag* drag,
                                           const std::list<TableItemPtr>& rows,
                                           Qt::DropActions supportedActions,
                                           Qt::DropAction defaultAction)

{

    std::list<KnobTableItemPtr> items;
    for (std::list<TableItemPtr>::const_iterator it = rows.begin(); it!=rows.end(); ++it) {

        ModelItemsVec::iterator found = _imp->findItem(*it);
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

    KnobItemsTablePtr model = _imp->internalModel.lock();
    model->setDuringDragDropOperation(true);

    NodeGuiPtr nodeUI = _imp->panel->getNodeGui();
    assert(nodeUI);
    obj.nodeScriptName = nodeUI->getNode()->getFullyQualifiedName();

    for (std::list<KnobTableItemPtr>::iterator it = items.begin(); it!= items.end(); ++it) {
        SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr s = model->createSerializationFromItem(*it);
        obj.items.push_back(s);
    }

    std::ostringstream ss;
    SERIALIZATION_NAMESPACE::write(ss, obj, std::string());

    QByteArray dataArray(ss.str().c_str());

    QMimeData *data = new QMimeData;
    data->setData(QLatin1String(kNatronKnobItemsTableGuiMimeType), dataArray);
    drag->setMimeData(data);
    
    drag->exec(supportedActions, defaultAction);
        

    model->setDuringDragDropOperation(false);
    
} // setupAndExecDragObject

void
KnobItemsTableView::dragMoveEvent(QDragMoveEvent *e)
{
    const QMimeData* mimedata = e->mimeData();
    if ( !mimedata->hasFormat( QLatin1String(kNatronKnobItemsTableGuiMimeType) ) || !(e->dropAction() & _imp->tableModel->supportedDropActions()) || !_imp->internalModel.lock()->isDragAndDropSupported() ) {
        e->ignore();
        return;
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
        setState(QAbstractItemView::DraggingState);
    }

}

void
KnobItemsTableGui::onViewItemRightClicked(const QPoint& globalPos, const TableItemPtr& item)
{
    ModelItemsVec::iterator foundItem = _imp->findItem(item);
    if (foundItem == _imp->items.end()) {
        return;
    }
    KnobTableItemPtr internalItem = foundItem->internalItem.lock();
    if (!internalItem) {
        return;
    }

    _imp->rightClickedItem = internalItem;

    std::string actionShortcutGroup;
    NodeGuiPtr node = _imp->panel->getNodeGui();
    if (node) {
        actionShortcutGroup = node->getNode()->getPlugin()->getPluginShortcutGroup();
    }

    Menu m(_imp->panel);
    // Add basic menu actions
    {
        ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupNodegraph,
                                                            kShortcutActionGraphRemoveNodes,
                                                            tr("Delete").toStdString(),
                                                            &m);
        QObject::connect( action, SIGNAL(triggered()), this, SLOT(onDeleteItemsActionTriggered()) );
        m.addAction(action);

    }
    {
        ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupNodegraph,
                                                            kShortcutActionGraphCopy,
                                                            tr("Copy").toStdString(),
                                                            &m);
        QObject::connect( action, SIGNAL(triggered()), this, SLOT(onCopyItemsActionTriggered()) );
        m.addAction(action);
    }
    {
        ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupNodegraph,
                                                            kShortcutActionGraphCut,
                                                            tr("Cut").toStdString(),
                                                            &m);
        QObject::connect( action, SIGNAL(triggered()), this, SLOT(onCutItemsActionTriggered()) );
        m.addAction(action);
    }
    {
        ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupNodegraph,
                                                            kShortcutActionGraphPaste,
                                                            tr("Paste").toStdString(),
                                                            &m);
        QObject::connect( action, SIGNAL(triggered()), this, SLOT(onPasteItemsActionTriggered()) );
        m.addAction(action);
    }
    {
        ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupNodegraph,
                                                            kShortcutActionGraphDuplicate,
                                                            tr("Duplicate").toStdString(),
                                                            &m);
        QObject::connect( action, SIGNAL(triggered()), this, SLOT(onDuplicateItemsActionTriggered()) );
        m.addAction(action);
    }
    {
        ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupNodegraph,
                                                            kShortcutActionGraphSelectAll,
                                                            tr("Select All").toStdString(),
                                                            &m);
        QObject::connect( action, SIGNAL(triggered()), this, SLOT(onSelectAllItemsActionTriggered()) );
        m.addAction(action);
    }

    m.addSeparator();

    KnobChoicePtr rightClickMenuKnob = toKnobChoice(internalItem->getKnobByName(kNatronOfxParamRightClickMenu));
    if (rightClickMenuKnob) {
        internalItem->refreshRightClickMenu();
        NodeGui::populateMenuRecursive(rightClickMenuKnob, internalItem, actionShortcutGroup, this, &m);
    }

    m.exec(globalPos);
    _imp->rightClickedItem.reset();
}

void
KnobItemsTableGui::onRightClickActionTriggered()
{
    ActionWithShortcut* action = dynamic_cast<ActionWithShortcut*>( sender() );

    if (!action) {
        return;
    }


    KnobTableItemPtr internalItem = _imp->rightClickedItem.lock();
    assert(internalItem);
    if (!internalItem) {
        return;
    }


    const std::vector<std::pair<QString, QKeySequence> >& shortcuts = action->getShortcuts();
    assert( !shortcuts.empty() );
    std::string knobName = shortcuts.front().first.toStdString();
    KnobIPtr knob = internalItem->getKnobByName(knobName);
    if (!knob) {
        // Plug-in specified invalid knob name in the menu
        return;
    }
    KnobButtonPtr button = toKnobButton(knob);
    if (!button) {
        // Plug-in must only use buttons inside menu
        return;
    }
    if ( button->getIsCheckable() ) {
        button->setValue( !button->getValue() );
    } else {
        button->trigger();
    }
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

NodeGuiPtr
KnobItemsTableGui::getNodeGui() const
{
    return _imp->panel->getNodeGui();
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
    for (ModelItemsVec::const_iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
        for (std::size_t i = 0; i < it->columnItems.size(); ++i) {
            if (it->columnItems[i].knob.lock() == knob) {
                return it->columnItems[i].guiKnob;
            }
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

    virtual ~RemoveItemsUndoCommand()
    {
        
    }

    virtual void undo() OVERRIDE FINAL
    {
        KnobItemsTablePtr table = _table->internalModel.lock();
        for (std::list<Item>::const_iterator it = _items.begin(); it!=_items.end(); ++it) {
            table->insertItem(it->indexInParent, it->item, it->parent, eTableChangeReasonInternal);
        }
    }
    
    virtual void redo() OVERRIDE FINAL
    {
        KnobItemsTablePtr table = _table->internalModel.lock();
        for (std::list<Item>::const_iterator it = _items.begin(); it!=_items.end(); ++it) {
            table->removeItem(it->item, eTableChangeReasonInternal);
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
        KnobTableItemPtr item, oldItem;
        KnobTableItemWPtr oldParent;
        KnobTableItemWPtr newParent;
        int indexInOldParent, indexInNewParent;
    };


    DragItemsUndoCommand(KnobItemsTableGuiPrivate* table,
                         KnobItemsTableGuiPrivate* originalTable,
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

    void moveItem(int indexInParent,
                  const KnobTableItemPtr& parent,
                  const KnobTableItemPtr& oldItem,
                  const KnobTableItemPtr& newItem,
                  KnobItemsTableGuiPrivate* fromTable,
                  KnobItemsTableGuiPrivate* toTable);

    KnobItemsTableGuiPrivate* _table;
    KnobItemsTableGuiPrivate* _originalTable;
    std::list<Item> _items;
};

void
DragItemsUndoCommand::moveItem(int indexInParent,
                               const KnobTableItemPtr& parent,
                               const KnobTableItemPtr& oldItem,
                               const KnobTableItemPtr& newItem,
                               KnobItemsTableGuiPrivate* fromTable,
                               KnobItemsTableGuiPrivate* toTable)
{
    // Remove the old item if it is from the same table
    if (oldItem && fromTable && fromTable == toTable) {
        fromTable->internalModel.lock()->removeItem(oldItem, eTableChangeReasonInternal);
    }
    if (toTable && newItem) {
        toTable->internalModel.lock()->insertItem(indexInParent, newItem, parent, eTableChangeReasonInternal);
    }
    if (parent && _table) {
        ModelItemsVec::iterator foundParent = _table->findItem(parent);
        if (foundParent != _table->items.end()) {
            _table->tableView->setExpanded(_table->tableModel->getItemIndex(foundParent->item), true);
        }
    }


    if (newItem && toTable) {
        toTable->createCustomWidgetRecursively(newItem);
    }


}

void
DragItemsUndoCommand::undo()
{
    for (std::list<Item>::iterator it = _items.begin(); it != _items.end(); ++it) {
        moveItem(it->indexInOldParent, it->oldParent.lock(), it->item, it->oldItem, _table, _originalTable);
    }
}

void
DragItemsUndoCommand::redo()
{
    for (std::list<Item>::iterator it = _items.begin(); it != _items.end(); ++it) {
        moveItem(it->indexInNewParent, it->newParent.lock(), it->oldItem, it->item, _originalTable, _table);
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

    SERIALIZATION_NAMESPACE::KnobItemsTableSerialization obj;
    {
        QVariant data = mimedata->data(mimeDataType);
        QString serializationStr = data.toString();
        std::stringstream ss(serializationStr.toStdString());
        try {
            SERIALIZATION_NAMESPACE::read(std::string(), ss, &obj);
        } catch (...) {
            e->ignore();
            return;
        }
    }

    if (obj.tableIdentifier != table->getTableIdentifier()) {
        Dialogs::errorDialog(KnobItemsTableGui::tr("Copy").toStdString(), KnobItemsTableGui::tr("You can only drop items from the same node type.").toStdString());
        return;
    }


    // Find the original table from which the knob was from
    // (if it is not from another process)
    KnobItemsTablePtr originalTable;
    KnobItemsTableGuiPtr originalTableUI;
    {
        NodePtr originalNode = _imp->_publicInterface->getGui()->getApp()->getProject()->getNodeByFullySpecifiedName(obj.nodeScriptName);
        if (originalNode) {

            originalTable = originalNode->getEffectInstance()->getItemsTable(obj.tableIdentifier);
            if (originalTable) {
                NodeGuiPtr originalNodeUI = boost::dynamic_pointer_cast<NodeGui>(originalNode->getNodeGui());
                assert(originalNodeUI);
                NodeSettingsPanel* originalPanel = originalNodeUI->getSettingPanel();
                assert(originalPanel);
                if (originalPanel) {
                    originalTableUI = originalPanel->getKnobItemsTable(obj.tableIdentifier);
                }
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

    //OnItem, AboveItem, BelowItem, OnViewport
    DropIndicatorPosition position = dropIndicatorPosition();
    e->accept();

    std::list<DragItemsUndoCommand::Item> dndItems;
    for (std::list<SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr>::const_iterator it = obj.items.begin(); it != obj.items.end(); ++it) {

        DragItemsUndoCommand::Item d;

        if (originalTable) {
            d.oldItem = originalTable->getItemByFullyQualifiedScriptName((*it)->scriptName);
            if (d.oldItem) {
                d.oldParent = d.oldItem->getParent();
                d.indexInOldParent = d.oldItem->getIndexInParent();
            }
        }

        d.item = table->createItemFromSerialization(*it);
        
        if (!d.item) {
            continue;
        }




        switch (position) {
            case QAbstractItemView::AboveItem: {

                assert(targetInternalItem);
                int targetItemIndex = targetInternalItem->getIndexInParent();

                assert(d.indexInOldParent != -1 && targetItemIndex != -1);
                d.newParent = targetInternalItem->getParent();

                // If the dropped item is already into the children and after the found index don't decrement
                if (d.oldParent.lock() == d.newParent.lock() && d.indexInOldParent > targetItemIndex) {
                    d.indexInNewParent = targetItemIndex;
                } else {
                    //The item "above" in the tree is index - 1 in the internal list which is ordered from bottom to top
                    d.indexInNewParent = targetItemIndex == 0 ? 0 : targetItemIndex - 1;
                }

                break;
            }
            case QAbstractItemView::BelowItem: {
                assert(targetInternalItem);
                int targetItemIndex = targetInternalItem->getIndexInParent();

                assert(d.indexInOldParent != -1 && targetItemIndex != -1);
                d.newParent = targetInternalItem->getParent();

                // If the dropped item is already into the children and before the found index don't decrement
                if (d.oldParent.lock() == d.newParent.lock() && d.indexInOldParent < targetItemIndex) {
                    d.indexInNewParent = targetItemIndex;
                } else {
                    // The item "below" in the tree is index + 1 in the internal list which is ordered from bottom to top
                    d.indexInNewParent = targetItemIndex + 1;
                }

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
    if (dndItems.empty()) {
        return;
    }

    KnobItemsTableGuiPrivate* originalTableUIPrivate = 0;
    if (originalTableUI) {
        originalTableUIPrivate = originalTableUI->_imp.get();
    }
    _imp->panel->pushUndoCommand(new DragItemsUndoCommand(_imp, originalTableUIPrivate, dndItems));

    
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
            _originalTargetItemSerialization = target->getModel()->createSerializationFromItem(target);
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

    virtual ~PasteItemUndoCommand()
    {
    }

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    KnobTableItemPtr _targetItem;
    SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr _originalTargetItemSerialization;
    
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
            _targetItem->getModel()->insertItem(-1, *it, _targetItem, eTableChangeReasonInternal);
        }
    }
}

void
PasteItemUndoCommand::undo()
{
    if (_sourceItemSerialization) {
        // We paste 1 item onto another
        assert(_targetItem);
        if (_originalTargetItemSerialization) {
            _targetItem->fromSerialization(*_originalTargetItemSerialization);
        }
    } else {
        // We paste multiple items as children of a container
        for (std::list<KnobTableItemPtr>::const_iterator it = _sourceItemsCopies.begin(); it!=_sourceItemsCopies.end(); ++it) {
            _targetItem->getModel()->removeItem(*it, eTableChangeReasonInternal);
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
    , _items(items)
    {
        for (std::list<KnobTableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
            SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr s = _table->internalModel.lock()->createSerializationFromItem(*it);
            KnobTableItemPtr dup = table->internalModel.lock()->createItemFromSerialization(s);
            assert(dup);
            _duplicates.push_back(dup);

        }
        
        
        
    }

    virtual ~DuplicateItemUndoCommand()
    {

    }

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
        _table->internalModel.lock()->insertItem(itemIndex, *it, parent, eTableChangeReasonInternal);
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
        SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr s = model->createSerializationFromItem(*it);
        obj.items.push_back(s);
    }
    
    std::ostringstream ss;
    try {
        SERIALIZATION_NAMESPACE::write(ss, obj, std::string());
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
        if (str.empty()) {
            return;
        }
    }
    
    std::istringstream ss(str);
    SERIALIZATION_NAMESPACE::KnobItemsTableSerialization obj;
    try {
        SERIALIZATION_NAMESPACE::read(std::string(), ss, &obj);
    } catch (...) {
        Dialogs::errorDialog(tr("Paste").toStdString(), tr("Failed to read content from the clipboard").toStdString());
        return;
    }
    
    // Check that table is of the same type
    if (obj.tableIdentifier != model->getTableIdentifier()) {
        Dialogs::errorDialog(tr("Paste").toStdString(), tr("Failed to read content from the clipboard").toStdString());
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
KnobItemsTableGui::onTableItemDataChanged(const TableItemPtr& item, int col, int role)
{
    if (!item) {
        return;
    }
    if (role != Qt::DisplayRole) {
        return;
    }
    ModelItemsVec::iterator found = _imp->findItem(item);
    if (found == _imp->items.end()) {
        return;
    }
    KnobTableItemPtr internalItem = found->internalItem.lock();
    if (!internalItem) {
        return;
    }

    // If the column is handled by a knob GUI, then we do not bother handling interfacing with the knob here since everything is handled in the KnobGui side
    DimSpec knobDim;
    KnobIPtr knob = internalItem->getColumnKnob(col, &knobDim);
    if (knob) {
        return;
    }

    std::string colName = internalItem->getColumnName(col);
    if (colName == kKnobTableItemColumnLabel) {
        QString label = item->getText(col);

        internalItem->setLabel(label.toStdString(), eTableChangeReasonPanel);
    }
    
}


void
KnobItemsTableGui::onSelectAllItemsActionTriggered()
{
    _imp->internalModel.lock()->selectAll(eTableChangeReasonInternal);
}

void
KnobItemsTableGuiPrivate::selectionFromIndexList(const QModelIndexList& indexes, std::list<KnobTableItemPtr>* outItems)
{
    std::set<KnobTableItemPtr> uniqueItems;
    for (QModelIndexList::const_iterator it = indexes.begin(); it != indexes.end(); ++it) {
        //Check that the item is valid
        assert(it->isValid() && it->column() >= 0 && it->column() < tableModel->columnCount());
        
        // Get the table item corresponding to the index
        TableItemPtr tableItem = tableModel->getItem(it->row(), it->parent());
        if (!tableItem) {
            continue;
        }

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
        if (found == items.end()) {
            continue;
        }
        assert(!found->columnItems.empty());
        QModelIndex leftMost = tableModel->getItemIndex(found->item);
        QModelIndex rightMost = tableModel->index(leftMost.row(), tableModel->columnCount() - 1, leftMost.parent());
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

    _imp->refreshKnobsSelectionState(selectedItems, deselectedItems);


    // Select the items in the model internally
    KnobItemsTablePtr model = _imp->internalModel.lock();
    model->beginEditSelection();
    model->removeFromSelection(deselectedItems, eTableChangeReasonPanel);
    model->addToSelection(selectedItems, eTableChangeReasonPanel);
    model->endEditSelection(eTableChangeReasonPanel);

    getNodeGui()->getDagGui()->getGui()->refreshTimelineGuiKeyframesLater();
}

void
KnobItemsTableGui::onModelSelectionChanged(const std::list<KnobTableItemPtr>& addedToSelection, const std::list<KnobTableItemPtr>& removedFromSelection, TableChangeReasonEnum reason)
{
    if (reason == eTableChangeReasonPanel) {
        // Do not recurse
        return;
    }

    // Refresh the view
    QItemSelection selectionToAdd, selectionToRemove;
    _imp->itemsToSelection(addedToSelection, &selectionToAdd);
    _imp->itemsToSelection(removedFromSelection, &selectionToRemove);
    QItemSelectionModel* selectionModel = _imp->tableView->selectionModel();

    _imp->refreshKnobsSelectionState(addedToSelection, removedFromSelection);
    
    // Ensure we don't recurse indefinitely in the selection
    ++_imp->selectingModelRecursion;
    if (!removedFromSelection.empty()) {
        selectionModel->select(selectionToRemove, QItemSelectionModel::Deselect);
    }
    if (!addedToSelection.empty()) {
        selectionModel->select(selectionToAdd, QItemSelectionModel::Select);
    }
    --_imp->selectingModelRecursion;

    getNodeGui()->getDagGui()->getGui()->refreshTimelineGuiKeyframesLater();
    
}


void
KnobItemsTableGui::onItemAnimationCurveChanged(std::list<double> /*added*/, std::list<double> /*removed*/, ViewIdx /*view*/)
{
    getNodeGui()->getDagGui()->getGui()->refreshTimelineGuiKeyframesLater();
}


void
KnobItemsTableGui::onItemLabelChanged(const QString& label, TableChangeReasonEnum reason)
{
    if (reason == eTableChangeReasonPanel) {
        return;
    }

    KnobTableItem* item = dynamic_cast<KnobTableItem*>(sender());
    if (!item) {
        return;
    }


    int labelColIndex = item->getLabelColumnIndex();
    if (labelColIndex == -1) {
        return;
    }

    TableItemPtr tableItem;
    for (ModelItemsVec::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
        if (it->internalItem.lock().get() == item) {
            tableItem = it->item;
            break;
        }
    }
    if (!tableItem) {
        return;
    }


    tableItem->setText(labelColIndex, label);
}

void
KnobItemsTableGuiPrivate::setItemIcon(const TableItemPtr& tableItem, int col, const KnobTableItemPtr& item)
{
    QString iconFilePath = QString::fromUtf8(item->getIconLabelFilePath().c_str());
    if (iconFilePath.isEmpty()) {
        return;
    }
    QPixmap pix;
    if (!QPixmapCache::find(iconFilePath, pix) ) {
        if (!pix.load(iconFilePath)) {
            return;
        }
        QPixmapCache::insert(iconFilePath, pix);
    }


    if (std::max( pix.width(), pix.height() ) != NATRON_MEDIUM_BUTTON_ICON_SIZE) {
        pix = pix.scaled(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE,
                         Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    tableItem->setIcon(col, QIcon(pix));

}

void
KnobItemsTableGui::onItemIconChanged(TableChangeReasonEnum reason)
{
    if (reason == eTableChangeReasonPanel) {
        return;
    }

    KnobTableItem* item = dynamic_cast<KnobTableItem*>(sender());
    if (!item) {
        return;
    }
    KnobTableItemPtr itemShared = toKnobTableItem(item->shared_from_this());

    int labelColIndex = item->getLabelColumnIndex();
    if (labelColIndex == -1) {
        return;
    }

    TableItemPtr tableItem;
    for (ModelItemsVec::iterator it = _imp->items.begin(); it != _imp->items.end(); ++it) {
        if (it->internalItem.lock().get() == item) {
            tableItem = it->item;
            break;
        }
    }
    if (!tableItem) {
        return;
    }

    _imp->setItemIcon(tableItem, labelColIndex, itemShared);
}

void
KnobItemsTableGui::onModelItemRemoved(const KnobTableItemPtr& item, TableChangeReasonEnum reason)
{
    if (reason == eTableChangeReasonPanel) {
        return;
    }

    _imp->removeTableItem(item);
}

void
KnobItemsTableGui::onModelItemInserted(int /*index*/, const KnobTableItemPtr& item, TableChangeReasonEnum reason)
{
    if (reason == eTableChangeReasonPanel) {
        return;
    }

    // If the item has a parent but the parent is not yet in the model, do not create the gui for the item.
    // Instead the parent will create its children gui afterwards
    KnobTableItemPtr parentItem = item->getParent();
    if (parentItem) {
        if (parentItem->getIndexInParent() == -1) {
            return;
        }
    }

    _imp->createTableItems(item);

    const std::vector<KnobTableItemPtr>& children = item->getChildren();
    if (!children.empty()) {
        _imp->createItemsVecRecursive(children);
    }


}

void
KnobItemsTableGuiPrivate::createItemsVecRecursive(const std::vector<KnobTableItemPtr>& items)
{
    for (std::size_t i = 0; i < items.size(); ++i) {
        createTableItems(items[i]);
        const std::vector<KnobTableItemPtr>& children = items[i]->getChildren();
        if (!children.empty()) {
            createItemsVecRecursive(children);
        }
    }
}

void
KnobItemsTableGuiPrivate::recreateItemsFromModel()
{
    assert(items.empty());
    const std::vector<KnobTableItemPtr>& items = internalModel.lock()->getTopLevelItems();
    createItemsVecRecursive(items);
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
KnobItemsTableGuiPrivate::removeTableItem(const KnobTableItemPtr& item)
{

    QObject::disconnect(item.get(), SIGNAL(curveAnimationChanged(std::list<double>, std::list<double>, ViewIdx)), _publicInterface, SLOT(onItemAnimationCurveChanged(std::list<double>,std::list<double>, ViewIdx)));

    ModelItemsVec::iterator foundItem = findItem(item);
    if (foundItem == items.end()) {
        return;
    }
    TableItemPtr tableItem = foundItem->item;
    if (!tableItem) {
        return;
    }
    tableModel->removeItem(tableItem);
    items.erase(foundItem);
}

void
KnobItemsTableGuiPrivate::createTableItems(const KnobTableItemPtr& item)
{
    // The item should not exist in the table GUI yet.
    assert(findItem(item) == items.end());

    QObject::connect(item.get(), SIGNAL(curveAnimationChanged(std::list<double>,std::list<double>, ViewIdx)), _publicInterface, SLOT(onItemAnimationCurveChanged(std::list<double>, std::list<double>, ViewIdx)), Qt::UniqueConnection);
    
    int itemRow = item->getIndexInParent();
    
    int nCols = tableModel->columnCount();
    

    items.push_back(ModelItem());
    ModelItem &mitem = items.back();
    mitem.columnItems.resize(nCols);
    mitem.item = TableItem::create(tableModel);
    mitem.internalItem = item;

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
            mitem.item->setFlags(i, Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        } else {
            // Ok the column must be kKnobTableItemColumnLabel
            // otherwise we don't know what the user want
            std::string columnID = item->getColumnName(i);
            if (columnID == kKnobTableItemColumnLabel) {
                mitem.labelColIndex = i;
                mitem.item->setToolTip(i, labelToolTipFromScriptName(item) );
                mitem.item->setFlags(i, Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
                mitem.item->setText(i, QString::fromUtf8( item->getLabel().c_str() ) );
                setItemIcon(mitem.item, i, item);
            } else {
                mitem.item->setFlags(i, Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
            }


        }
        
        tableView->resizeColumnToContents(i);
        mitem.columnItems[i] = d;

    }

    tableView->setExpanded(tableModel->getItemIndex(mitem.item), true);

    // Create custom widgets for knobs
    for (int i = 0; i < nCols; ++i) {
        createItemCustomWidgetAtCol(item, itemRow, i);
    }

    QObject::connect(item.get(), SIGNAL(labelChanged(QString,TableChangeReasonEnum)), _publicInterface, SLOT(onItemLabelChanged(QString,TableChangeReasonEnum)), Qt::UniqueConnection);
    QObject::connect(item.get(), SIGNAL(labelIconChanged(TableChangeReasonEnum)), _publicInterface, SLOT(onItemIconChanged(TableChangeReasonEnum)), Qt::UniqueConnection);

    
} // createTableItems

void
KnobItemsTableGuiPrivate::refreshKnobItemSelection(const KnobTableItemPtr& item, bool selected)
{
    ModelItemsVec::iterator found = findItem(item);
    if (found == items.end()) {
        return;
    }
    for (std::size_t i = 0; i < found->columnItems.size(); ++i) {
        if (found->columnItems[i].guiKnob) {
            found->columnItems[i].guiKnob->reflectKnobSelectionState(selected);
        }
    }
}

void
KnobItemsTableGuiPrivate::refreshKnobsSelectionState(const std::list<KnobTableItemPtr>& selected, const std::list<KnobTableItemPtr>& deselected)
{
    for (std::list<KnobTableItemPtr>::const_iterator it = selected.begin(); it != selected.end(); ++it) {
        refreshKnobItemSelection(*it, true);
    }
    for (std::list<KnobTableItemPtr>::const_iterator it = deselected.begin(); it != deselected.end(); ++it) {
        refreshKnobItemSelection(*it, false);
    }
}


NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING
#include "moc_KnobItemsTableGui.cpp"
