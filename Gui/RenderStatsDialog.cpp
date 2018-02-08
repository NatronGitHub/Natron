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

#include "RenderStatsDialog.h"

#include <bitset>
#include <stdexcept>

#include <QtCore/QCoreApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QCheckBox>
#include <QItemSelectionModel>
#include <QtCore/QRegExp>

#include "Engine/Node.h"
#include "Engine/Timer.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewIdx.h"

#include "Gui/Button.h"
#include "Gui/Gui.h"
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/NodeGui.h"
#include "Gui/TableModelView.h"


#define COL_NAME 0
#define COL_PLUGIN_ID 1
#define COL_TIME 2

#define NUM_COLS 3

NATRON_NAMESPACE_ENTER

enum ItemsRoleEnum
{
    eItemsRoleTime = 100,
    eItemsRoleIdentityTilesNb = 101,
    eItemsRoleIdentityTilesInfo = 102,
    eItemsRoleRenderedTilesNb = 103,
    eItemsRoleRenderedTilesInfo = 104,
};

struct RowInfo
{
    NodeWPtr node;
    int rowIndex;
    TableItemPtr item;

    RowInfo()
        : node(), rowIndex(-1), item() {}
};

struct StatRowsCompare
{
    int _col;

    StatRowsCompare(int col)
        : _col(col)
    {
    }

    bool operator() (const RowInfo& lhs,
                     const RowInfo& rhs) const
    {
        switch (_col) {
            case COL_TIME:
                return lhs.item->getData(_col, (int)eItemsRoleTime ).toDouble() < rhs.item->getData(_col, (int)eItemsRoleTime ).toDouble();
            default:
                return lhs.item->getText(_col) < rhs.item->getText(_col);
        }
    }
};

class StatsTableModel;
typedef boost::shared_ptr<StatsTableModel> StatsTableModelPtr;

class StatsTableModel
    : public TableModel
{
    Q_DECLARE_TR_FUNCTIONS(StatsTableModel)

    std::vector<NodeWPtr > rows;

    struct MakeSharedEnabler;

    StatsTableModel(int cols)
    : TableModel(cols, eTableModelTypeTable)
    , rows()
    {
    }

public:
    static StatsTableModelPtr create(int cols);

    virtual ~StatsTableModel() {}

    void clearRows()
    {
        clear();
        rows.clear();
    }

    const std::vector<NodeWPtr >& getRows() const
    {
        return rows;
    }

    void editNodeRow(const NodePtr& node,
                     const NodeRenderStats& stats)
    {
        int row = -1;
        bool exists = false;

        for (std::size_t i = 0; i < rows.size(); ++i) {
            NodePtr n = rows[i].lock();
            if (n == node) {
                row = i;
                exists = true;
                break;
            }
        }


        if (row == -1) {
            row = rows.size();
        }

        if ( row >= rowCount() ) {
            insertRow(row);
        }

        QColor c;
        NodeGuiPtr nodeUi = boost::dynamic_pointer_cast<NodeGui>( node->getNodeGui() );
        if (nodeUi) {
            double r, g, b;
            nodeUi->getColor(&r, &g, &b);
            c.setRgbF(r, g, b);
        }
        TableModelPtr thisShared = shared_from_this();
        TableItemPtr item = getItem(row);
        if (!item) {
            item = TableItem::create(thisShared);
            setRow(row, item);
        }
        {
            QString tt = NATRON_NAMESPACE::convertFromPlainText(tr("The label of the node as it appears on the nodegraph."), NATRON_NAMESPACE::WhiteSpaceNormal);
            item->setToolTip(COL_NAME, tt);
            item->setFlags(COL_NAME, Qt::ItemIsSelectable | Qt::ItemIsEnabled);

            NodeGuiPtr nodeUi = boost::dynamic_pointer_cast<NodeGui>( node->getNodeGui() );
            if (nodeUi) {
                item->setTextColor(COL_NAME, Qt::black);
                item->setBackgroundColor(COL_NAME, c);
            }
            item->setText(COL_NAME,  QString::fromUtf8( node->getLabel().c_str() ) );
        }


        {

            QString tt = NATRON_NAMESPACE::convertFromPlainText(tr("The ID of the plug-in embedded in the node."), NATRON_NAMESPACE::WhiteSpaceNormal);
            item->setToolTip(COL_PLUGIN_ID, tt);
            item->setFlags(COL_PLUGIN_ID, Qt::ItemIsSelectable | Qt::ItemIsEnabled);

            if (nodeUi) {
                item->setTextColor(COL_PLUGIN_ID, Qt::black);
                item->setBackgroundColor(COL_PLUGIN_ID, c);
            }
            item->setText(COL_PLUGIN_ID, QString::fromUtf8( node->getPluginID().c_str() ) );

        }

        {
            double timeSoFar;
            if (exists) {
                timeSoFar = item->getData(COL_TIME, (int)eItemsRoleTime).toDouble();
                timeSoFar += stats.getTotalTimeSpentRendering();
            } else {
                QString tt = NATRON_NAMESPACE::convertFromPlainText(tr("The time spent rendering by this node across all threads."), NATRON_NAMESPACE::WhiteSpaceNormal);
                item->setToolTip(COL_TIME, tt);
                timeSoFar = stats.getTotalTimeSpentRendering();
                item->setFlags(COL_TIME, Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            if (nodeUi) {
                item->setTextColor(COL_TIME, Qt::black);
                item->setBackgroundColor(COL_TIME, c);
            }
            item->setData(COL_TIME, (int)eItemsRoleTime, timeSoFar );
            item->setText(COL_TIME, Timer::printAsTime(timeSoFar, false) );
        }

        if (!exists) {
            rows.push_back(node);
        }
    } // editNodeRow

    virtual void sort(int column,
                      Qt::SortOrder order = Qt::AscendingOrder) OVERRIDE FINAL
    {
        if ( (column < 0) || (column >= NUM_COLS) ) {
            return;
        }
        std::vector<RowInfo> vect( rows.size() );
        for (std::size_t i = 0; i < rows.size(); ++i) {
            vect[i].node = rows[i];
            vect[i].rowIndex = i;
            vect[i].item = getItem(vect[i].rowIndex);
            assert(vect[i].item);
        }
        Q_EMIT layoutAboutToBeChanged();
        {
            StatRowsCompare o(column);
            std::sort(vect.begin(), vect.end(), o);
        }

        if (order == Qt::DescendingOrder) {
            std::vector<RowInfo> copy = vect;
            for (std::size_t i = 0; i < copy.size(); ++i) {
                vect[vect.size() - i - 1] = copy[i];
            }
        }
        std::vector<TableItemPtr> newTable(vect.size());
        for (std::size_t i = 0; i < vect.size(); ++i) {
            rows[i] = vect[i].node;
            newTable[i] = getItem(vect[i].rowIndex);
        }
        setTable(newTable);
        Q_EMIT layoutChanged();
    }
};


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct StatsTableModel::MakeSharedEnabler: public StatsTableModel
{
    MakeSharedEnabler(int cols) : StatsTableModel(cols) {
    }
};


StatsTableModelPtr
StatsTableModel::create(int cols)
{
    return boost::make_shared<StatsTableModel::MakeSharedEnabler>(cols);
}

struct RenderStatsDialogPrivate
{
    Gui* gui;
    QVBoxLayout* mainLayout;
    Label* descriptionLabel;
    QWidget* globalInfosContainer;
    QHBoxLayout* globalInfosLayout;
    Label* accumulateLabel;
    QCheckBox* accumulateCheckbox;
    Label* totalTimeSpentDescLabel;
    Label* totalTimeSpentValueLabel;
    double totalSpentTime;
    Button* resetButton;
    QWidget* filterContainer;
    QHBoxLayout* filterLayout;
    Label* filtersLabel;
    Label* nameFilterLabel;
    LineEdit* nameFilterEdit;
    Label* idFilterLabel;
    LineEdit* idFilterEdit;
    Label* useUnixWildcardsLabel;
    QCheckBox* useUnixWildcardsCheckbox;
    TableView* view;
    StatsTableModelPtr model;

    RenderStatsDialogPrivate(Gui* gui)
        : gui(gui)
        , mainLayout(0)
        , descriptionLabel(0)
        , globalInfosContainer(0)
        , globalInfosLayout(0)
        , accumulateLabel(0)
        , accumulateCheckbox(0)
        , totalTimeSpentDescLabel(0)
        , totalTimeSpentValueLabel(0)
        , totalSpentTime(0)
        , resetButton(0)
        , filterContainer(0)
        , filterLayout(0)
        , filtersLabel(0)
        , nameFilterLabel(0)
        , nameFilterEdit(0)
        , idFilterLabel(0)
        , idFilterEdit(0)
        , useUnixWildcardsLabel(0)
        , useUnixWildcardsCheckbox(0)
        , view(0)
        , model()
    {
    }

    void editNodeRow(const NodePtr& node, const NodeRenderStats& stats);

    void updateVisibleRowsInternal(const QString& nameFilter, const QString& pluginIDFilter);
};

RenderStatsDialog::RenderStatsDialog(Gui* gui)
    : QWidget(gui)
    , _imp( new RenderStatsDialogPrivate(gui) )
{
    setWindowFlags(Qt::Tool);
    setWindowTitle( tr("Render statistics") );

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    _imp->mainLayout = new QVBoxLayout(this);

    QString statsDesc = NATRON_NAMESPACE::convertFromPlainText(tr(
                                                           "Statistics are accumulated over each frame rendered by default.\nIf you want to display statistics of the last "
                                                           "frame rendered, uncheck the \"Accumulate\" checkbox.\nIf you want to have more detailed informations besides the time spent "
                                                           "rendering for each node,\ncheck the \"Advanced\" checkbox.\n The \"Time spent to render\" is the time spent "
                                                           "to render a frame (or more if \"Accumulate\" is checked).\nIf there are multiple parallel renders (see preferences) "
                                                           "the time will be accumulated across each threads.\nHover the mouse over each column header to have detailed informations "
                                                           "for each statistic.\nBy default, nodes are sorted by decreasing time spent to render.\nClicking on a node will center "
                                                           "the node-graph on it.\nWhen in \"Advanced\" mode, double-clicking on the \"Rendered Tiles\" or "
                                                           "the \"Identity Tiles\" cell\nwill open-up a window containing detailed informations about the tiles rendered.\n"), NATRON_NAMESPACE::WhiteSpaceNormal);
    _imp->descriptionLabel = new Label(statsDesc, this);
    _imp->mainLayout->addWidget(_imp->descriptionLabel);

    _imp->globalInfosContainer = new QWidget(this);
    _imp->globalInfosLayout = new QHBoxLayout(_imp->globalInfosContainer);

    QString accTt = NATRON_NAMESPACE::convertFromPlainText(tr("When checked, stats are not cleared between the computation of frames."), NATRON_NAMESPACE::WhiteSpaceNormal);
    _imp->accumulateLabel = new Label(tr("Accumulate:"), _imp->globalInfosContainer);
    _imp->accumulateLabel->setToolTip(accTt);
    _imp->accumulateCheckbox = new QCheckBox(_imp->globalInfosContainer);
    _imp->accumulateCheckbox->setChecked(true);
    _imp->accumulateCheckbox->setToolTip(accTt);

    _imp->globalInfosLayout->addWidget(_imp->accumulateLabel);
    _imp->globalInfosLayout->addWidget(_imp->accumulateCheckbox);

    _imp->globalInfosLayout->addSpacing(10);


    QString wallTimett = NATRON_NAMESPACE::convertFromPlainText(tr("This is the time spent to compute the frame for the whole tree.\n "
                                                           ), NATRON_NAMESPACE::WhiteSpaceNormal);
    _imp->totalTimeSpentDescLabel = new Label(tr("Time spent to render:"), _imp->globalInfosContainer);
    _imp->totalTimeSpentDescLabel->setToolTip(wallTimett);
    _imp->totalTimeSpentValueLabel = new Label(QString::fromUtf8("0.0 sec"), _imp->globalInfosContainer);
    _imp->totalTimeSpentValueLabel->setToolTip(wallTimett);

    _imp->globalInfosLayout->addWidget(_imp->totalTimeSpentDescLabel);
    _imp->globalInfosLayout->addWidget(_imp->totalTimeSpentValueLabel);

    _imp->resetButton = new Button(tr("Reset"), _imp->globalInfosContainer);
    _imp->resetButton->setToolTip( tr("Clears the statistics.") );
    QObject::connect( _imp->resetButton, SIGNAL(clicked(bool)), this, SLOT(resetStats()) );
    _imp->globalInfosLayout->addWidget(_imp->resetButton);

    _imp->globalInfosLayout->addStretch();

    _imp->mainLayout->addWidget(_imp->globalInfosContainer);

    _imp->filterContainer = new QWidget(this);
    _imp->filterLayout = new QHBoxLayout(_imp->filterContainer);

    _imp->filtersLabel = new Label(tr("Filters:"), _imp->filterContainer);
    _imp->filterLayout->addWidget(_imp->filtersLabel);

    _imp->filterLayout->addSpacing(10);

    QString nameFilterTt = NATRON_NAMESPACE::convertFromPlainText(tr("If unix wildcards are enabled, show only nodes "
                                                             "with a label matching the filter.\nOtherwise if unix wildcards are disabled, "
                                                             "show only nodes with a label containing the text in the filter."), NATRON_NAMESPACE::WhiteSpaceNormal);
    _imp->nameFilterLabel = new Label(tr("Name:"), _imp->filterContainer);
    _imp->nameFilterLabel->setToolTip(nameFilterTt);
    _imp->nameFilterEdit = new LineEdit(_imp->filterContainer);
    _imp->nameFilterEdit->setToolTip(nameFilterTt);
    QObject::connect( _imp->nameFilterEdit, SIGNAL(editingFinished()), this, SLOT(updateVisibleRows()) );
    QObject::connect( _imp->nameFilterEdit, SIGNAL(textEdited(QString)), this, SLOT(onNameLineEditChanged(QString)) );

    _imp->filterLayout->addWidget(_imp->nameFilterLabel);
    _imp->filterLayout->addWidget(_imp->nameFilterEdit);

    _imp->filterLayout->addSpacing(20);

    QString idFilterTt = NATRON_NAMESPACE::convertFromPlainText(tr("If unix wildcards are enabled, show only nodes "
                                                           "with a plugin ID matching the filter.\nOtherwise if unix wildcards are disabled, "
                                                           "show only nodes with a plugin ID containing the text in the filter."), NATRON_NAMESPACE::WhiteSpaceNormal);
    _imp->idFilterLabel = new Label(tr("Plugin ID:"), _imp->idFilterLabel);
    _imp->idFilterLabel->setToolTip(idFilterTt);
    _imp->idFilterEdit = new LineEdit(_imp->idFilterEdit);
    _imp->idFilterEdit->setToolTip(idFilterTt);
    QObject::connect( _imp->idFilterEdit, SIGNAL(editingFinished()), this, SLOT(updateVisibleRows()) );
    QObject::connect( _imp->idFilterEdit, SIGNAL(textEdited(QString)), this, SLOT(onIDLineEditChanged(QString)) );

    _imp->filterLayout->addWidget(_imp->idFilterLabel);
    _imp->filterLayout->addWidget(_imp->idFilterEdit);

    _imp->useUnixWildcardsLabel = new Label(tr("Use Unix wildcards (*, ?, etc..)"), _imp->filterContainer);
    _imp->useUnixWildcardsCheckbox = new QCheckBox(_imp->filterContainer);
    _imp->useUnixWildcardsCheckbox->setChecked(false);
    QObject::connect( _imp->useUnixWildcardsCheckbox, SIGNAL(toggled(bool)), this, SLOT(updateVisibleRows()) );

    _imp->filterLayout->addWidget(_imp->useUnixWildcardsLabel);
    _imp->filterLayout->addWidget(_imp->useUnixWildcardsCheckbox);

    _imp->filterLayout->addStretch();

    _imp->mainLayout->addWidget(_imp->filterContainer);

    _imp->view = new TableView(_imp->gui, this);


    QStringList dimensionNames;

    dimensionNames
    << tr("Node")
    << tr("Plugin ID")
    << tr("Time Spent");
    _imp->model = StatsTableModel::create(dimensionNames.size());
    _imp->view->setTableModel(_imp->model);

    _imp->model->setHorizontalHeaderData(dimensionNames);

    _imp->view->setAttribute(Qt::WA_MacShowFocusRect, 0);
    _imp->view->setSelectionMode(QAbstractItemView::SingleSelection);
    _imp->view->setSelectionBehavior(QAbstractItemView::SelectRows);


#if QT_VERSION < 0x050000
    _imp->view->header()->setResizeMode(QHeaderView::ResizeToContents);
#else
    _imp->view->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
    _imp->view->header()->setStretchLastSection(true);
    _imp->view->setUniformRowHeights(true);
    _imp->view->setSortingEnabled(true);
    _imp->view->header()->setSortIndicator(COL_TIME, Qt::DescendingOrder);
    _imp->model->sort(COL_TIME, Qt::DescendingOrder);

    QItemSelectionModel* selModel = _imp->view->selectionModel();
    QObject::connect( selModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(onKnobsTreeSelectionChanged(QItemSelection,QItemSelection)) );
    _imp->mainLayout->addWidget(_imp->view);
}

RenderStatsDialog::~RenderStatsDialog()
{
}

void
RenderStatsDialog::onKnobsTreeSelectionChanged(const QItemSelection &selected,
                                      const QItemSelection & /*deselected*/)
{
    QModelIndexList indexes = selected.indexes();

    if ( indexes.isEmpty() ) {
        return;
    }
    int idx = indexes[0].row();
    const std::vector<NodeWPtr >& rows = _imp->model->getRows();
    if ( (idx < 0) || ( idx >= (int)rows.size() ) ) {
        return;
    }
    NodePtr node = rows[idx].lock();
    if (!node) {
        return;
    }
    NodeGuiPtr nodeUi = boost::dynamic_pointer_cast<NodeGui>( node->getNodeGui() );
    if (!nodeUi) {
        return;
    }
    nodeUi->centerGraphOnIt();
}

void
RenderStatsDialog::resetStats()
{
    _imp->model->clearRows();
    _imp->totalTimeSpentValueLabel->setText( QString::fromUtf8("0.0 sec") );
    _imp->totalSpentTime = 0;
}

void
RenderStatsDialog::addStats(int /*time*/,
                            double wallTime,
                            const std::map<NodePtr, NodeRenderStats >& stats)
{
    if ( !_imp->accumulateCheckbox->isChecked() ) {
        _imp->model->clearRows();
        _imp->totalSpentTime = 0;
    }

    _imp->totalSpentTime += wallTime;
    _imp->totalTimeSpentValueLabel->setText( Timer::printAsTime(_imp->totalSpentTime, false) );

    for (std::map<NodePtr, NodeRenderStats >::const_iterator it = stats.begin(); it != stats.end(); ++it) {
        _imp->model->editNodeRow(it->first, it->second);
    }

    updateVisibleRows();
    if ( !stats.empty() ) {
        _imp->view->header()->setSortIndicator(COL_TIME, Qt::DescendingOrder);
        _imp->model->sort(COL_TIME, Qt::DescendingOrder);
    }
}

void
RenderStatsDialog::closeEvent(QCloseEvent * /*event*/)
{
    _imp->gui->setRenderStatsEnabled(false);
}

void
RenderStatsDialogPrivate::updateVisibleRowsInternal(const QString& nameFilter,
                                                    const QString& pluginIDFilter)
{
    QModelIndex rootIdx = view->rootIndex();
    const std::vector<NodeWPtr >& rows = model->getRows();


    if ( useUnixWildcardsCheckbox->isChecked() ) {
        QRegExp nameExpr(nameFilter, Qt::CaseInsensitive, QRegExp::Wildcard);
        if ( !nameExpr.isValid() ) {
            return;
        }

        QRegExp idExpr(pluginIDFilter, Qt::CaseInsensitive, QRegExp::Wildcard);
        if ( !idExpr.isValid() ) {
            return;
        }


        int i = 0;
        for (std::vector<NodeWPtr >::const_iterator it = rows.begin(); it != rows.end(); ++it, ++i) {
            NodePtr node = it->lock();
            if (!node) {
                continue;
            }

            if ( ( nameFilter.isEmpty() || nameExpr.exactMatch( QString::fromUtf8( node->getLabel().c_str() ) ) ) &&
                 ( pluginIDFilter.isEmpty() || idExpr.exactMatch( QString::fromUtf8( node->getPluginID().c_str() ) ) ) ) {
                if ( view->isRowHidden(i, rootIdx) ) {
                    view->setRowHidden(i, rootIdx, false);
                }
            } else {
                if ( !view->isRowHidden(i, rootIdx) ) {
                    view->setRowHidden(i, rootIdx, true);
                }
            }
        }
    } else {
        int i = 0;

        for (std::vector<NodeWPtr >::const_iterator it = rows.begin(); it != rows.end(); ++it, ++i) {
            NodePtr node = it->lock();
            if (!node) {
                continue;
            }

            if ( ( nameFilter.isEmpty() || QString::fromUtf8( node->getLabel().c_str() ).contains(nameFilter) ) &&
                 ( pluginIDFilter.isEmpty() || QString::fromUtf8( node->getPluginID().c_str() ).contains(pluginIDFilter) ) ) {
                if ( view->isRowHidden(i, rootIdx) ) {
                    view->setRowHidden(i, rootIdx, false);
                }
            } else {
                if ( !view->isRowHidden(i, rootIdx) ) {
                    view->setRowHidden(i, rootIdx, true);
                }
            }
        }
    }
} // RenderStatsDialogPrivate::updateVisibleRowsInternal

void
RenderStatsDialog::updateVisibleRows()
{
    _imp->updateVisibleRowsInternal( _imp->nameFilterEdit->text(), _imp->idFilterEdit->text() );
}

void
RenderStatsDialog::onNameLineEditChanged(const QString& filter)
{
    _imp->updateVisibleRowsInternal( filter, _imp->idFilterEdit->text() );
}

void
RenderStatsDialog::onIDLineEditChanged(const QString& filter)
{
    _imp->updateVisibleRowsInternal(_imp->nameFilterEdit->text(), filter);
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_RenderStatsDialog.cpp"
