#include "KnobGuiTable.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QItemSelectionModel>
#include <QHeaderView>
#include <QPainter>
#include <QStyle>
#include <QApplication>
#include <QStyledItemDelegate>


#include "Engine/KnobTypes.h"
#include "Engine/Project.h"
#include "Engine/Utils.h" // convertFromPlainText

#include "Gui/Button.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/TableModelView.h"

NATRON_NAMESPACE_ENTER;


NATRON_NAMESPACE_ANONYMOUS_ENTER


NATRON_NAMESPACE_ANONYMOUS_EXIT


struct KnobGuiTablePrivate
{
    std::string encodeTable(const boost::shared_ptr<KnobTable>& knob) const;

    void createItem(const boost::shared_ptr<KnobTable>& knob, int row, const QStringList& values);

    QWidget* mainContainer;
    TableView *table;
    TableModelPtr model;
    Button *addPathButton;
    Button* removePathButton;
    Button* editPathButton;
    bool isInsertingItem;
    bool dragAndDropping;

    KnobGuiTablePrivate()
        : mainContainer(0)
        , table(0)
        , model()
        , addPathButton(0)
        , removePathButton(0)
        , editPathButton(0)
        , isInsertingItem(false)
        , dragAndDropping(false)
    {
    }
};

KnobGuiTable::KnobGuiTable(KnobIPtr knob,
                           KnobGuiContainerI *container)
    : KnobGui(knob, container)
    , _imp( new KnobGuiTablePrivate() )
{
}

KnobGuiTable::~KnobGuiTable()
{
}

void
KnobGuiTable::removeSpecificGui()
{
    if (_imp->mainContainer) {
        _imp->mainContainer->deleteLater();
    }
}

int
KnobGuiTable::rowCount() const
{
    return _imp->model->rowCount();
}

////////////// TableView delegate

class KnobTableItemDelegate
    : public QStyledItemDelegate
{
    TableView* _view;
    KnobTableWPtr _knob;

public:

    explicit KnobTableItemDelegate(const boost::shared_ptr<KnobTable>& knob,
                                   TableView* view);

private:

    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const OVERRIDE FINAL;
};

KnobTableItemDelegate::KnobTableItemDelegate(const boost::shared_ptr<KnobTable>& knob,
                                             TableView* view)
    : QStyledItemDelegate(view)
    , _view(view)
    , _knob(knob)
{
}

void
KnobTableItemDelegate::paint(QPainter * painter,
                             const QStyleOptionViewItem & option,
                             const QModelIndex & index) const
{
    if (!index.isValid() || option.state & QStyle::State_Selected) {
        QStyledItemDelegate::paint(painter, option, index);

        return;
    }

    boost::shared_ptr<KnobTable> knob = _knob.lock();
    if (!knob) {
        return;
    }


    TableModel* model = dynamic_cast<TableModel*>( _view->model() );
    assert(model);
    if (!model) {
        // coverity[dead_error_begin]
        QStyledItemDelegate::paint(painter, option, index);

        return;
    }
    TableItemPtr item = model->getItem(index);
    if (!item) {
        QStyledItemDelegate::paint(painter, option, index);

        return;
    }
    QPen pen;

    if ( !item->getFlags(index.column()).testFlag(Qt::ItemIsEnabled) ) {
        pen.setColor(Qt::black);
    } else {
        pen.setColor( QColor(200, 200, 200) );
    }
    painter->setPen(pen);

    // get the proper subrect from the style
    QStyle *style = QApplication::style();
    QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &option);

    ///Draw the item name column
    if (option.state & QStyle::State_Selected) {
        painter->fillRect( geom, option.palette.highlight() );
    }
    QRect r;
    QString str = item->getData(index.column(), Qt::DisplayRole).toString();
    bool decorateWithBrackets =  knob->isCellBracketDecorated( index.row(), index.column() );

    if (decorateWithBrackets) {
        ///Env vars are used between brackets
        str.prepend( QLatin1Char('[') );
        str.append( QLatin1Char(']') );
    }
    painter->drawText(geom, Qt::TextSingleLine, str, &r);
} // KnobTableItemDelegate::paint

void
KnobGuiTable::createWidget(QHBoxLayout* layout)
{
    boost::shared_ptr<KnobTable> knob = boost::dynamic_pointer_cast<KnobTable>( getKnob() );

    assert(knob);

    _imp->mainContainer = new QWidget( layout->parentWidget() );
    QVBoxLayout* mainLayout = new QVBoxLayout(_imp->mainContainer);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    _imp->table = new TableView(_imp->mainContainer);
    QObject::connect( _imp->table, SIGNAL(aboutToDrop()), this, SLOT(onItemAboutToDrop()) );
    QObject::connect( _imp->table, SIGNAL(itemDropped()), this, SLOT(onItemDropped()) );
    QObject::connect( _imp->table, SIGNAL(itemDoubleClicked(TableItemPtr)), this, SLOT(onItemDoubleClicked(TableItemPtr)) );

    layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    _imp->table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    _imp->table->setAttribute(Qt::WA_MacShowFocusRect, 0);

#if QT_VERSION < 0x050000
    _imp->table->header()->setResizeMode(QHeaderView::ResizeToContents);
#else
    _imp->table->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
    _imp->table->setDragDropMode(QAbstractItemView::InternalMove);
    _imp->table->header()->setStretchLastSection(true);
    _imp->table->setUniformRowHeights(true);
    _imp->table->setItemDelegate( new KnobTableItemDelegate(knob, _imp->table) );

    const int numCols = knob->getColumnsCount();
    _imp->model = TableModel::create(numCols, TableModel::eTableModelTypeTable);
    QObject::connect( _imp->model.get(), SIGNAL(itemDataChanged(TableItemPtr,int)), this, SLOT(onItemDataChanged(TableItemPtr,int)) );


    _imp->table->setTableModel(_imp->model);
    if (numCols == 1) {
        _imp->table->header()->hide();
    }
    QStringList headers;
    for (int i = 0; i < numCols; ++i) {
        headers.push_back( QString::fromUtf8( knob->getColumnLabel(i).c_str() ) );
    }
    _imp->model->setHorizontalHeaderData(headers);

    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_imp->table, 0);

    QWidget* buttonsContainer = new QWidget(_imp->mainContainer);
    QHBoxLayout* buttonsLayout = new QHBoxLayout(buttonsContainer);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);

    _imp->addPathButton = new Button( tr("Add..."), buttonsContainer );
    _imp->addPathButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Add a new value"), NATRON_NAMESPACE::WhiteSpaceNormal) );

    QObject::connect( _imp->addPathButton, SIGNAL(clicked()), this, SLOT(onAddButtonClicked()) );

    _imp->removePathButton = new Button( tr("Remove"), buttonsContainer);
    QObject::connect( _imp->removePathButton, SIGNAL(clicked()), this, SLOT(onRemoveButtonClicked()) );
    _imp->removePathButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Remove selected values"), NATRON_NAMESPACE::WhiteSpaceNormal) );

    _imp->editPathButton = new Button( tr("Edit..."), buttonsContainer);
    QObject::connect( _imp->editPathButton, SIGNAL(clicked()), this, SLOT(onEditButtonClicked()) );
    _imp->editPathButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Click to edit seleted value."), NATRON_NAMESPACE::WhiteSpaceNormal) );


    buttonsLayout->addWidget(_imp->addPathButton);
    buttonsLayout->addWidget(_imp->removePathButton);
    buttonsLayout->addWidget(_imp->editPathButton);
    buttonsLayout->addStretch();

    mainLayout->addWidget(_imp->table);
    mainLayout->addWidget(buttonsContainer);

    if ( !knob->useEditButton() ) {
        _imp->editPathButton->hide();
    }

    layout->addWidget(_imp->mainContainer);
} // KnobGuiTable::createWidget

std::string
KnobGuiTablePrivate::encodeTable(const boost::shared_ptr<KnobTable>& knob) const
{
    std::stringstream ss;

    int nCols = model->columnCount();
    int nRows = model->rowCount();
    for (int i = 0; i < nRows; ++i) {
        TableItemPtr item = model->getItem(i);
        // In order to use XML tags, the text inside the tags has to be escaped.
        for (int c = 0; c < nCols; ++c) {
            std::string label = knob->getColumnLabel(c);
            ss << "<" << label << ">";
            ss << Project::escapeXML( item->getText(c).toStdString() );
            ss << "</" << label << ">";
        }


    }

    return ss.str();
}

void
KnobGuiTablePrivate::createItem(const boost::shared_ptr<KnobTable>& knob,
                                int row,
                                const QStringList& values)
{
    assert( values.size() == knob->getColumnsCount() );
    int col = 0;
    TableItemPtr tableItem = TableItem::create();

    for (QStringList::const_iterator it = values.begin(); it != values.end(); ++it, ++col) {
        Qt::ItemFlags flags;
        if ( knob->isCellEnabled(row, col, values) ) {
            flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
            if ( knob->isColumnEditable(col) ) {
                flags |= Qt::ItemIsEditable;
            }

            if (values.size() == 1) {
                flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsUserCheckable;
            }
        }

        tableItem->setText(col, *it);
        tableItem->setFlags(col, flags);
    }

    int modelRowCount = model->rowCount();
    if (row >= modelRowCount) {
        model->insertRow(row);
    }
    isInsertingItem = true;
    model->setRow(row, tableItem);
    for (int c= 0; c < values.size(); ++c) {
        table->resizeColumnToContents(c);
    }

    isInsertingItem = false;
}

void
KnobGuiTable::onAddButtonClicked()
{
    boost::shared_ptr<KnobTable> knob = boost::dynamic_pointer_cast<KnobTable>( getKnob() );

    assert(knob);

    const int numCols = knob->getColumnsCount();
    std::string oldTable = knob->getValue();
    QStringList row;

    int rowCount = _imp->model->rowCount();
    if (numCols == 1) {
        QStringList existingEntries;
        for (int i = 0; i < rowCount; ++i) {
            TableItemPtr item = _imp->model->getItem(i);
            if (item) {
                existingEntries.push_back( item->getText(0) );
            }
        }

        QString newItemName = QString::fromUtf8("Placeholder");
        int i = 1;
        while ( existingEntries.contains(newItemName) ) {
            newItemName = QString::fromUtf8("Placeholder") + QString::number(i);
            ++i;
        }
        row.push_back(newItemName);
    } else {
        if ( !addNewUserEntry(row) ) {
            return;
        }
    }

    _imp->createItem(knob, rowCount, row);
    std::string newTable = _imp->encodeTable(knob);
    pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), oldTable, newTable) );
}

void
KnobGuiTable::onEditButtonClicked()
{
    boost::shared_ptr<KnobTable> knob = boost::dynamic_pointer_cast<KnobTable>( getKnob() );

    assert(knob);
    std::string oldTable = knob->getValue();
    QModelIndexList selection = _imp->table->selectionModel()->selectedRows();

    if (selection.size() != 1) {
        return;
    }

    int rowCount = _imp->model->rowCount();
    if (selection[0].row() < 0 || selection[0].row() >= rowCount) {
        return;
    }

    int nCol = _imp->model->columnCount();
    TableItemPtr item = _imp->model->getItem(selection[0].row());
    QStringList row;
    for (int c = 0; c < nCol; ++c) {
        row.push_back( item->getText(c) );
    }
    if ( !editUserEntry(row) ) {
        return;
    }

    for (int c = 0; c < nCol; ++c) {
        item->setText(c, row[c]);
    }

    std::string newTable = _imp->encodeTable(knob);
    pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), oldTable, newTable) );

}

void
KnobGuiTable::onRemoveButtonClicked()
{
    boost::shared_ptr<KnobTable> knob = boost::dynamic_pointer_cast<KnobTable>( getKnob() );

    assert(knob);
    QModelIndexList selection = _imp->table->selectionModel()->selectedRows();

    if (selection.size() < 1) {
        return;
    }

    int nCols = _imp->model->columnCount();
    int nRows = _imp->model->rowCount();
    for (int i = 0; i < selection.size(); ++i) {
        QStringList removedRow;
        if ( (selection[0].row() >= 0) && ( selection[0].row() < nRows) ) {
            TableItemPtr r = _imp->model->getItem(selection[0].row());
            for (int c = 0; c < nCols; ++c) {
                removedRow.push_back( r->getText(c) );
            }
        }

        entryRemoved(removedRow);
    }

    _imp->model->removeRows( selection.front().row(), selection.size() );


    std::string oldTable = knob->getValue();
    std::string newTable = _imp->encodeTable(knob);
    pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), oldTable, newTable) );
}

void
KnobGuiTable::updateGUI(DimSpec /*dimension*/, ViewSetSpec view)
{
    boost::shared_ptr<KnobTable> knob = boost::dynamic_pointer_cast<KnobTable>( getKnob() );

    assert(knob);

    std::list<std::vector<std::string> > table;
    knob->getTable(&table);

    _imp->model->clear();
    int i = 0;
    for (std::list<std::vector<std::string> >::const_iterator it = table.begin(); it != table.end(); ++it, ++i) {
        QStringList row;
        for (std::size_t j = 0; j < it->size(); ++j) {
            row.push_back( QString::fromUtf8( (*it)[j].c_str() ) );
        }
        _imp->createItem(knob, i, row);
    }
}

void
KnobGuiTable::_hide()
{
    if (_imp->mainContainer) {
        _imp->mainContainer->hide();
    }
}

void
KnobGuiTable::_show()
{
    if (_imp->mainContainer) {
        _imp->mainContainer->show();
    }
}

void
KnobGuiTable::setEnabled()
{
    bool enabled = getKnob()->isEnabled(0);

    if (_imp->table) {
        _imp->table->setEnabled(enabled);
        _imp->addPathButton->setEnabled(enabled);
        _imp->removePathButton->setEnabled(enabled);
    }
}

void
KnobGuiTable::setReadOnly(bool readOnly,
                          int /*dimension*/)
{
    if (_imp->table) {
        _imp->table->setEnabled(!readOnly);
        _imp->addPathButton->setEnabled(!readOnly);
        _imp->removePathButton->setEnabled(!readOnly);
    }
}

void
KnobGuiTable::onItemAboutToDrop()
{
    _imp->dragAndDropping = true;
}

void
KnobGuiTable::onItemDropped()
{

    _imp->dragAndDropping = false;

    //Now refresh the knob balue
    onItemDataChanged(TableItemPtr(), 0);
}

void
KnobGuiTable::onItemDoubleClicked(const TableItemPtr& item)
{

    QModelIndex idx = _imp->model->getItemIndex(item);
    if (!idx.isValid()) {
        return;
    }
    boost::shared_ptr<KnobTable> knob = boost::dynamic_pointer_cast<KnobTable>( getKnob() );
    assert(knob);
    if (!knob) {
        return;
    }

    int nCols = _imp->model->columnCount();
    QStringList values;
    for (int c = 0; c < nCols; ++c) {
        values.push_back(item->getText(c));
    }

    // Not enabled
    if ( !knob->isCellEnabled(idx.row(), idx.column(), values) ) {
        return;
    }

    // Let the standard editor
    if ( knob->isColumnEditable(idx.column()) ) {
        return;
    }
    if ( !editUserEntry(values) ) {
        return;
    }

    for (int c = 0; c < nCols; ++c) {
        item->setText(c, values[c]);
    }

    std::string oldTable = knob->getValue();
    std::string newTable = _imp->encodeTable(knob);
    pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), oldTable, newTable) );
} // KnobGuiTable::onItemDoubleClicked

void
KnobGuiTable::onItemDataChanged(const TableItemPtr& item, int /*col*/)
{
    if (_imp->isInsertingItem || _imp->dragAndDropping) {
        return;
    }

    QModelIndex idx = _imp->model->getItemIndex(item);

    if (idx.isValid()) {
        boost::shared_ptr<KnobTable> knob = boost::dynamic_pointer_cast<KnobTable>( getKnob() );
        assert(knob);

        std::string oldValue = knob->getValue();
        std::string newValue = _imp->encodeTable(knob);
        if (oldValue != newValue) {
            tableChanged(idx.row(), idx.column(), &newValue);
            pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), oldValue, newValue) );
        }
    }
}

void
KnobGuiTable::updateToolTip()
{
    if ( hasToolTip() ) {
        toolTip(_imp->table);
    }
}

KnobGuiLayers::KnobGuiLayers(KnobIPtr knob,
                             KnobGuiContainerI *container)
    : KnobGuiTable(knob, container)
{
    _knob = boost::dynamic_pointer_cast<KnobLayers>(knob);
}

KnobGuiLayers::~KnobGuiLayers()
{
}

bool
KnobGuiLayers::addNewUserEntry(QStringList& row)
{
    NewLayerDialog dialog( ImageComponents::getNoneComponents(), getGui() );

    if ( dialog.exec() ) {
        ImageComponents comps = dialog.getComponents();
        if ( comps == ImageComponents::getNoneComponents() ) {
            Dialogs::errorDialog( tr("Layer").toStdString(), tr("A layer must contain at least 1 channel and channel names must be "
                                                                "Python compliant.").toStdString() );

            return false;
        }
        row.push_back( QString::fromUtf8( comps.getLayerName().c_str() ) );

        std::list<std::vector<std::string> > table;
        _knob.lock()->getTable(&table);
        for (std::list<std::vector<std::string> >::iterator it = table.begin(); it != table.end(); ++it) {
            if ( (*it)[0] == comps.getLayerName() ) {
                Dialogs::errorDialog( tr("Layer").toStdString(), tr("A Layer with the same name already exists").toStdString() );

                return false;
            }
        }

        std::string channelsStr;
        const std::vector<std::string>& channels = comps.getComponentsNames();
        for (std::size_t i = 0; i < channels.size(); ++i) {
            channelsStr += channels[i];
            if ( i < (channels.size() - 1) ) {
                channelsStr += ' ';
            }
        }
        row.push_back( QString::fromUtf8( channelsStr.c_str() ) );

        return true;
    }

    return false;
}

bool
KnobGuiLayers::editUserEntry(QStringList& row)
{
    std::vector<std::string> channels;
    QStringList splits = row[1].split( QLatin1Char(' ') );

    for (int i = 0; i < splits.size(); ++i) {
        channels.push_back( splits[i].toStdString() );
    }
    ImageComponents original(row[0].toStdString(), std::string(), channels);;
    NewLayerDialog dialog( original, getGui() );
    if ( dialog.exec() ) {
        ImageComponents comps = dialog.getComponents();
        if ( comps == ImageComponents::getNoneComponents() ) {
            Dialogs::errorDialog( tr("Layer").toStdString(), tr("A layer must contain at least 1 channel and channel names must be "
                                                                "Python compliant.").toStdString() );

            return false;
        }

        std::string oldLayerName = row[0].toStdString();
        row[0] = ( QString::fromUtf8( comps.getLayerName().c_str() ) );

        std::list<std::vector<std::string> > table;
        _knob.lock()->getTable(&table);
        for (std::list<std::vector<std::string> >::iterator it = table.begin(); it != table.end(); ++it) {
            if ( ( (*it)[0] == comps.getLayerName() ) && ( (*it)[0] != oldLayerName ) ) {
                Dialogs::errorDialog( tr("Layer").toStdString(), tr("A Layer with the same name already exists").toStdString() );

                return false;
            }
        }

        std::string channelsStr;
        const std::vector<std::string>& channels = comps.getComponentsNames();
        for (std::size_t i = 0; i < channels.size(); ++i) {
            channelsStr += channels[i];
            if ( i < (channels.size() - 1) ) {
                channelsStr += ' ';
            }
        }
        row[1] = ( QString::fromUtf8( channelsStr.c_str() ) );

        return true;
    }

    return false;
}

void
KnobGuiLayers::tableChanged(int row,
                            int col,
                            std::string* newEncodedValue)
{
    if (col != 0) {
        return;
    }
    boost::shared_ptr<KnobLayers> knob = boost::dynamic_pointer_cast<KnobLayers>( getKnob() );
    assert(knob);
    std::list<std::vector<std::string> > table;
    knob->decodeFromKnobTableFormat(*newEncodedValue, &table);

    if ( row < (int)table.size() ) {
        std::list<std::vector<std::string> >::iterator it = table.begin();
        std::advance(it, row);
        std::string copy = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendlyWithDots( (*it)[0] );
        if (copy != (*it)[0]) {
            (*it)[0] = copy;
            *newEncodedValue = knob->encodeToKnobTableFormat(table);
        }
    }
}

KnobIPtr
KnobGuiLayers::getKnob() const
{
    return _knob.lock();
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_KnobGuiTable.cpp"
