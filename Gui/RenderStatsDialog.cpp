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
#define COL_SUPPORT_TILES 3
#define COL_SUPPORT_RS 4
#define COL_MIPMAP_LEVEL 5
#define COL_CHANNELS 6
#define COL_PREMULT 7
#define COL_ROD 8
#define COL_IDENTITY 9
#define COL_IDENTITY_TILES 10
#define COL_RENDERED_TILES 11
#define COL_RENDERED_PLANES 12
#define COL_NB_CACHE_HIT 13
#define COL_NB_CACHE_HIT_DOWNSCALED 14
#define COL_NB_CACHE_MISS 15

#define NUM_COLS 16

NATRON_NAMESPACE_ENTER;

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
            case COL_IDENTITY_TILES:

                return lhs.item->getData(_col, (int)eItemsRoleIdentityTilesNb ).toInt() < rhs.item->getData(_col, (int)eItemsRoleIdentityTilesNb ).toInt();
            case COL_RENDERED_TILES:

                return lhs.item->getData(_col, (int)eItemsRoleRenderedTilesNb ).toInt() < rhs.item->getData(_col, (int)eItemsRoleRenderedTilesNb ).toInt();
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

private:

    std::vector<NodeWPtr > rows;

    StatsTableModel(int cols)
    : TableModel(cols, eTableModelTypeTable)
    , rows()
    {
    }


public:

    static StatsTableModelPtr create(int cols)
    {
        return StatsTableModelPtr(new StatsTableModel(cols));
    }


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
        {
            QString tt = NATRON_NAMESPACE::convertFromPlainText(tr("Whether this node has tiles (portions of the final image) support or not."), NATRON_NAMESPACE::WhiteSpaceNormal);
            item->setToolTip(COL_SUPPORT_TILES, tt);
            item->setFlags(COL_SUPPORT_TILES, Qt::ItemIsSelectable | Qt::ItemIsEnabled);

            QString str;
            if ( stats.isTilesSupportEnabled() ) {
                str = QString::fromUtf8("Yes");
            } else {
                str = QString::fromUtf8("No");
            }
            item->setText(COL_SUPPORT_TILES, str);
            if (nodeUi) {
                item->setTextColor(COL_SUPPORT_TILES, Qt::black);
                item->setBackgroundColor(COL_SUPPORT_TILES, c);
            }

        }
        {

            QString tt = NATRON_NAMESPACE::convertFromPlainText(tr("Whether this node has render scale support or not.\n"
                                                                   "When activated, that means the node can render an image at a "
                                                                   "lower scale."), NATRON_NAMESPACE::WhiteSpaceNormal);
            item->setToolTip(COL_SUPPORT_RS, tt);
            item->setFlags(COL_SUPPORT_RS, Qt::ItemIsSelectable | Qt::ItemIsEnabled);

            QString str;
            if ( stats.isRenderScaleSupportEnabled() ) {
                str = QString::fromUtf8("Yes");
            } else {
                str = QString::fromUtf8("No");
            }
            item->setText(COL_SUPPORT_RS, str);
            if (nodeUi) {
                item->setTextColor(COL_SUPPORT_RS, Qt::black);
                item->setBackgroundColor(COL_SUPPORT_RS, c);
            }
        }
        {
            QString str;

            QString tt = NATRON_NAMESPACE::convertFromPlainText(tr("The mipmaplevel rendered (See render-scale). 0 means scale = 100%, "
                                                                   "1 means scale = 50%, 2 means scale = 25%, etc."), NATRON_NAMESPACE::WhiteSpaceNormal);
            item->setToolTip(COL_MIPMAP_LEVEL, tt);
            item->setFlags(COL_MIPMAP_LEVEL, Qt::ItemIsSelectable | Qt::ItemIsEnabled);

            const std::set<unsigned int>& mm = stats.getMipMapLevelsRendered();
            for (std::set<unsigned int>::const_iterator it = mm.begin(); it != mm.end(); ++it) {
                str.append( QString::number(*it) );
                str.append( QLatin1Char(' ') );
            }
            item->setText(COL_MIPMAP_LEVEL, str);
            if (nodeUi) {
                item->setTextColor(COL_MIPMAP_LEVEL, Qt::black);
                item->setBackgroundColor(COL_MIPMAP_LEVEL, c);
            }

        }
        {

            QString tt = NATRON_NAMESPACE::convertFromPlainText(tr("The channels processed by this node (corresponding to the RGBA checkboxes)."), NATRON_NAMESPACE::WhiteSpaceNormal);
            item->setToolTip(COL_CHANNELS, tt);
            item->setFlags(COL_CHANNELS, Qt::ItemIsSelectable | Qt::ItemIsEnabled);

            QString str;
            std::bitset<4> processChannels = stats.getChannelsRendered();
            if (processChannels[0]) {
                str.append( QString::fromUtf8("R ") );
            }
            if (processChannels[1]) {
                str.append( QString::fromUtf8("G ") );
            }
            if (processChannels[2]) {
                str.append( QString::fromUtf8("B ") );
            }
            if (processChannels[3]) {
                str.append( QString::fromUtf8("A") );
            }
            item->setText(COL_CHANNELS, str);
            if (nodeUi) {
                item->setTextColor(COL_CHANNELS, Qt::black);
                item->setBackgroundColor(COL_CHANNELS, c);
            }

        }
        {

            QString tt = NATRON_NAMESPACE::convertFromPlainText(tr("The alpha premultiplication of the image produced by this node."), NATRON_NAMESPACE::WhiteSpaceNormal);
            item->setToolTip(COL_PREMULT, tt);
            item->setFlags(COL_PREMULT, Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            QString str;
            ImagePremultiplicationEnum premult = stats.getOutputPremult();
            switch (premult) {
                case eImagePremultiplicationOpaque:
                    str = QString::fromUtf8("Opaque");
                    break;
                case eImagePremultiplicationPremultiplied:
                    str = QString::fromUtf8("Premultiplied");
                    break;
                case eImagePremultiplicationUnPremultiplied:
                    str = QString::fromUtf8("Unpremultiplied");
                    break;
            }
            item->setText(COL_PREMULT, str);
            if (nodeUi) {
                item->setTextColor(COL_PREMULT, Qt::black);
                item->setBackgroundColor(COL_PREMULT, c);
            }
        }
        {
            QString str;
            QString tt = NATRON_NAMESPACE::convertFromPlainText(tr("The region of definition of the image produced."), NATRON_NAMESPACE::WhiteSpaceNormal);
            item->setToolTip(COL_ROD, tt);
            item->setFlags(COL_ROD, Qt::ItemIsSelectable | Qt::ItemIsEnabled);

            const RectD& rod = stats.getRoD();
            str = QString::fromUtf8("(%1, %2, %3, %4)").arg(rod.x1).arg(rod.y1).arg(rod.x2).arg(rod.y2);
            item->setText(COL_ROD, str);
            if (nodeUi) {
                item->setTextColor(COL_ROD, Qt::black);
                item->setBackgroundColor(COL_ROD, c);
            }

        }
        {

            QString tt = NATRON_NAMESPACE::convertFromPlainText(tr("When different of \"-\", this node does not render but rather "
                                                                   "directly returns the image produced by the node indicated by its label."), NATRON_NAMESPACE::WhiteSpaceNormal);
            item->setToolTip(COL_IDENTITY, tt);
            item->setFlags(COL_IDENTITY, Qt::ItemIsSelectable | Qt::ItemIsEnabled);

            QString str;
            NodePtr identity = stats.getInputImageIdentity();
            if (identity) {
                str = QString::fromUtf8( identity->getLabel().c_str() );
            } else {
                str = QLatin1Char('-');
            }
            item->setText(COL_IDENTITY, str);
            if (nodeUi) {
                item->setTextColor(COL_IDENTITY, Qt::black);
                item->setBackgroundColor(COL_IDENTITY, c);
            }
            
        }
        {
            int nbIdentityTiles = 0;
            QString tilesInfo;
            if (exists) {
                nbIdentityTiles = item->getData(COL_IDENTITY_TILES, eItemsRoleIdentityTilesNb ).toInt();
                tilesInfo = item->getData(COL_IDENTITY_TILES, eItemsRoleIdentityTilesInfo ).toString();
            } else {
                QString tt = NATRON_NAMESPACE::convertFromPlainText(tr("The list of the tiles that were identity in the image.\n"
                                                               "Double-click for more info."), NATRON_NAMESPACE::WhiteSpaceNormal);
                item->setToolTip(COL_IDENTITY_TILES, tt);
                item->setFlags(COL_IDENTITY_TILES, Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            std::list<std::pair<RectI, NodePtr > > tiles = stats.getIdentityRectangles();
            for (std::list<std::pair<RectI, NodePtr > >::iterator it = tiles.begin(); it != tiles.end(); ++it) {
                const RectI& tile = it->first;
                QString tileEnc = QString::fromUtf8("(%1, %2, %3, %4)").arg(tile.x1).arg(tile.y1).arg(tile.x2).arg(tile.y2);
                tilesInfo.append(tileEnc);
            }
            nbIdentityTiles += (int)tiles.size();

            item->setData( COL_IDENTITY_TILES, (int)eItemsRoleIdentityTilesNb, nbIdentityTiles );
            item->setData( COL_IDENTITY_TILES, (int)eItemsRoleIdentityTilesInfo, tilesInfo );

            QString str = QString::number(nbIdentityTiles) + QString::fromUtf8(" tiles...");
            if (nodeUi) {
                item->setTextColor(COL_IDENTITY_TILES, Qt::black);
                item->setBackgroundColor(COL_IDENTITY_TILES, c);
            }
            item->setText(COL_IDENTITY_TILES, str);
        }
        {
            int nbTiles = 0;
            QString tilesInfo;
            if (exists) {
                nbTiles = item->getData(COL_RENDERED_TILES,  (int)eItemsRoleRenderedTilesNb ).toInt();
                tilesInfo = item->getData(COL_RENDERED_TILES, (int)eItemsRoleRenderedTilesInfo ).toString();
            } else {
                QString tt = NATRON_NAMESPACE::convertFromPlainText(tr("The list of the tiles effectivly rendered.\n"
                                                               "Double-click for more infos"), NATRON_NAMESPACE::WhiteSpaceNormal);
                item->setToolTip(COL_RENDERED_TILES, tt);
                item->setFlags(COL_RENDERED_TILES, Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            const std::list<RectI>& tiles = stats.getRenderedRectangles();
            for (std::list<RectI>::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
                const RectI& tile = *it;
                QString tileEnc = QString::fromUtf8("(%1, %2, %3, %4)").arg(tile.x1).arg(tile.y1).arg(tile.x2).arg(tile.y2);
                tilesInfo.append(tileEnc);
            }
            nbTiles += (int)tiles.size();

            item->setData(COL_RENDERED_TILES, (int)eItemsRoleRenderedTilesNb, nbTiles );
            item->setData(COL_RENDERED_TILES, (int)eItemsRoleRenderedTilesInfo, tilesInfo );

            QString str = QString::number(nbTiles) + QString::fromUtf8(" tiles...");
            if (nodeUi) {
                item->setTextColor(COL_RENDERED_TILES,Qt::black);
                item->setBackgroundColor(COL_RENDERED_TILES,c);
            }
            item->setText(COL_RENDERED_TILES, str);

        }
        {
            QString planesInfo;

            QString tt = NATRON_NAMESPACE::convertFromPlainText(tr("The list of the planes rendered by this node."), NATRON_NAMESPACE::WhiteSpaceNormal);
            item->setToolTip(COL_RENDERED_PLANES, tt);
            item->setFlags(COL_RENDERED_PLANES, Qt::ItemIsSelectable | Qt::ItemIsEnabled);

            const std::set<std::string>& planes = stats.getPlanesRendered();
            for (std::set<std::string>::const_iterator it = planes.begin(); it != planes.end(); ++it) {
                if ( !planesInfo.isEmpty() ) {
                    planesInfo.append( QLatin1Char(' ') );
                }
                planesInfo.append( QString::fromUtf8( it->c_str() ) );
            }

            if (nodeUi) {
                item->setTextColor(COL_RENDERED_PLANES, Qt::black);
                item->setBackgroundColor(COL_RENDERED_PLANES, c);
            }
            item->setText(COL_RENDERED_PLANES, planesInfo);

        }
        {
            int nb = 0;
            if (exists) {
                nb = item->getText(COL_NB_CACHE_HIT).toInt();
            } else {
                QString tt = NATRON_NAMESPACE::convertFromPlainText(tr("The number of cache hits."), NATRON_NAMESPACE::WhiteSpaceNormal);
                item->setToolTip(COL_NB_CACHE_HIT, tt);
                item->setFlags(COL_NB_CACHE_HIT, Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            int nbCacheMiss, nbCacheHits, nbCacheHitButDown;
            stats.getCacheAccessInfos(&nbCacheMiss, &nbCacheHits, &nbCacheHitButDown);
            nb += nbCacheHits;

            QString str = QString::number(nb);
            if (nodeUi) {
                item->setTextColor(COL_NB_CACHE_HIT, Qt::black);
                item->setBackgroundColor(COL_NB_CACHE_HIT, c);
            }
            item->setText(COL_NB_CACHE_HIT, str);
        }
        {
            int nb = 0;
            if (exists) {
                nb = item->getText(COL_NB_CACHE_HIT_DOWNSCALED).toInt();
            } else {
                QString tt = NATRON_NAMESPACE::convertFromPlainText(tr("The number of cache access hits but at higher scale "
                                                                       "hence requiring downscaling."), NATRON_NAMESPACE::WhiteSpaceNormal);
                item->setToolTip(COL_NB_CACHE_HIT_DOWNSCALED,tt);
                item->setFlags(COL_NB_CACHE_HIT_DOWNSCALED,Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            int nbCacheMiss, nbCacheHits, nbCacheHitButDown;
            stats.getCacheAccessInfos(&nbCacheMiss, &nbCacheHits, &nbCacheHitButDown);
            nb += nbCacheHitButDown;

            QString str = QString::number(nb);
            if (nodeUi) {
                item->setTextColor(COL_NB_CACHE_HIT_DOWNSCALED, Qt::black);
                item->setBackgroundColor(COL_NB_CACHE_HIT_DOWNSCALED, c);
            }
            item->setText(COL_NB_CACHE_HIT_DOWNSCALED, str);
        }
        {
            int nb = 0;
            if (exists) {
                nb = item->getText(COL_NB_CACHE_MISS).toInt();
            } else {
                QString tt = NATRON_NAMESPACE::convertFromPlainText(tr("The number of cache misses."), NATRON_NAMESPACE::WhiteSpaceNormal);
                item->setToolTip(COL_NB_CACHE_MISS,tt);
                item->setFlags(COL_NB_CACHE_MISS,Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            int nbCacheMiss, nbCacheHits, nbCacheHitButDown;
            stats.getCacheAccessInfos(&nbCacheMiss, &nbCacheHits, &nbCacheHitButDown);
            nb += nbCacheMiss;

            QString str = QString::number(nb);
            if (nodeUi) {
                item->setTextColor(COL_NB_CACHE_MISS, Qt::black);
                item->setBackgroundColor(COL_NB_CACHE_MISS, c);
            }
            item->setText(COL_NB_CACHE_MISS, str);


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

struct RenderStatsDialogPrivate
{
    Gui* gui;
    QVBoxLayout* mainLayout;
    Label* descriptionLabel;
    QWidget* globalInfosContainer;
    QHBoxLayout* globalInfosLayout;
    Label* accumulateLabel;
    QCheckBox* accumulateCheckbox;
    Label* advancedLabel;
    QCheckBox* advancedCheckbox;
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
        , advancedLabel(0)
        , advancedCheckbox(0)
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

    QString adTt = NATRON_NAMESPACE::convertFromPlainText(tr("When checked, more statistics are displayed. Useful mainly for debuging purposes."), NATRON_NAMESPACE::WhiteSpaceNormal);
    _imp->advancedLabel = new Label(tr("Advanced:"), _imp->globalInfosContainer);
    _imp->advancedLabel->setToolTip(adTt);
    _imp->advancedCheckbox = new QCheckBox(_imp->globalInfosContainer);
    _imp->advancedCheckbox->setChecked(false);
    _imp->advancedCheckbox->setToolTip(adTt);
    QObject::connect( _imp->advancedCheckbox, SIGNAL(clicked(bool)), this, SLOT(refreshAdvancedColsVisibility()) );

    _imp->globalInfosLayout->addWidget(_imp->advancedLabel);
    _imp->globalInfosLayout->addWidget(_imp->advancedCheckbox);

    _imp->globalInfosLayout->addSpacing(20);

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

    _imp->view = new TableView(this);


    QStringList dimensionNames;

    dimensionNames
    << tr("Node")
    << tr("Plugin ID")
    << tr("Time Spent")
    << tr("Tiles Support")
    << tr("Render-scale Support")
    << tr("Mipmap Level(s)")
    << tr("Channels")
    << tr("Output Premult")
    << tr("RoD")
    << tr("Identity")
    << tr("Identity Tiles")
    << tr("Rendered Tiles")
    << tr("Rendered Planes")
    << tr("Cache Hits")
    << tr("Cache Hits Higher Scale")
    << tr("Cache Misses");
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

    refreshAdvancedColsVisibility();
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
RenderStatsDialog::refreshAdvancedColsVisibility()
{
    bool checked = _imp->advancedCheckbox->isChecked();

    _imp->view->setColumnHidden(COL_SUPPORT_TILES, !checked);
    _imp->view->setColumnHidden(COL_SUPPORT_RS, !checked);
    _imp->view->setColumnHidden(COL_MIPMAP_LEVEL, !checked);
    _imp->view->setColumnHidden(COL_CHANNELS, !checked);
    _imp->view->setColumnHidden(COL_PREMULT, !checked);
    _imp->view->setColumnHidden(COL_ROD, !checked);
    _imp->view->setColumnHidden(COL_IDENTITY, !checked);
    _imp->view->setColumnHidden(COL_IDENTITY_TILES, !checked);
    _imp->view->setColumnHidden(COL_RENDERED_TILES, !checked);
    _imp->view->setColumnHidden(COL_RENDERED_PLANES, !checked);
    _imp->view->setColumnHidden(COL_NB_CACHE_HIT, !checked);
    _imp->view->setColumnHidden(COL_NB_CACHE_HIT_DOWNSCALED, !checked);
    _imp->view->setColumnHidden(COL_NB_CACHE_MISS, !checked);
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
                            ViewIdx /*view*/,
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

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_RenderStatsDialog.cpp"
