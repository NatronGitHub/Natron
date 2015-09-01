/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QCheckBox>
#include <QItemSelectionModel>
#include <QRegExp>

#include "Engine/Node.h"
#include "Engine/Timer.h"

#include "Gui/Button.h"
#include "Gui/Gui.h"
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/NodeGui.h"
#include "Gui/TableModelView.h"
#include "Gui/Utils.h"

using namespace Natron;

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


enum ItemsRoleEnum
{
    eItemsRoleTime = 100,
    eItemsRoleIdentityTilesNb = 101,
    eItemsRoleIdentityTilesInfo = 102,
    eItemsRoleRenderedTilesNb = 103,
    eItemsRoleRenderedTilesInfo = 104,
};


template <int COL>
struct StatRowsCompare
{
    TableView* _view;
    
    StatRowsCompare(TableView* view)
    : _view(view)
    {
        
    }
    
    bool operator() (const std::pair<boost::weak_ptr<Natron::Node>,int>& lhs,
                     const std::pair<boost::weak_ptr<Natron::Node>,int>& rhs) const
    {
        switch (COL) {
            case COL_NAME:
            {
                TableItem* leftItem = _view->item(lhs.second, COL_NAME);
                assert(leftItem);
                TableItem* rightItem = _view->item(rhs.second, COL_NAME);
                assert(rightItem);
                return leftItem->text() < rightItem->text();
            } break;
            case COL_PLUGIN_ID:
            {
                TableItem* leftItem = _view->item(lhs.second, COL_PLUGIN_ID);
                assert(leftItem);
                TableItem* rightItem = _view->item(rhs.second, COL_PLUGIN_ID);
                assert(rightItem);
                return leftItem->text() < rightItem->text();
            } break;
            case COL_TIME:
            {
                TableItem* leftItem = _view->item(lhs.second, COL_TIME);
                assert(leftItem);
                TableItem* rightItem = _view->item(rhs.second, COL_TIME);
                assert(rightItem);
                return leftItem->data((int)eItemsRoleTime).toDouble() < rightItem->data((int)eItemsRoleTime).toDouble();
            } break;
            case COL_SUPPORT_TILES:
            {
                TableItem* leftItem = _view->item(lhs.second, COL_SUPPORT_TILES);
                assert(leftItem);
                TableItem* rightItem = _view->item(rhs.second, COL_SUPPORT_TILES);
                assert(rightItem);
                return leftItem->text() < rightItem->text();
            } break;
            case COL_SUPPORT_RS:
            {
                TableItem* leftItem = _view->item(lhs.second, COL_SUPPORT_RS);
                assert(leftItem);
                TableItem* rightItem = _view->item(rhs.second, COL_SUPPORT_RS);
                assert(rightItem);
                return leftItem->text() < rightItem->text();
            } break;
            case COL_MIPMAP_LEVEL:
            {
                TableItem* leftItem = _view->item(lhs.second, COL_MIPMAP_LEVEL);
                assert(leftItem);
                TableItem* rightItem = _view->item(rhs.second, COL_MIPMAP_LEVEL);
                assert(rightItem);
                return leftItem->text() < rightItem->text();
            } break;
            case COL_CHANNELS:
            {
                TableItem* leftItem = _view->item(lhs.second, COL_CHANNELS);
                assert(leftItem);
                TableItem* rightItem = _view->item(rhs.second, COL_CHANNELS);
                assert(rightItem);
                return leftItem->text() < rightItem->text();
            } break;
            case COL_PREMULT:
            {
                TableItem* leftItem = _view->item(lhs.second, COL_PREMULT);
                assert(leftItem);
                TableItem* rightItem = _view->item(rhs.second, COL_PREMULT);
                assert(rightItem);
                return leftItem->text() < rightItem->text();
            } break;
            case COL_ROD:
            {
                TableItem* leftItem = _view->item(lhs.second, COL_ROD);
                assert(leftItem);
                TableItem* rightItem = _view->item(rhs.second, COL_ROD);
                assert(rightItem);
                return leftItem->text() < rightItem->text();
            } break;
            case COL_IDENTITY:
            {
                TableItem* leftItem = _view->item(lhs.second, COL_IDENTITY);
                assert(leftItem);
                TableItem* rightItem = _view->item(rhs.second, COL_IDENTITY);
                assert(rightItem);
                return leftItem->text() < rightItem->text();
            } break;
            case COL_IDENTITY_TILES:
            {
                TableItem* leftItem = _view->item(lhs.second, COL_IDENTITY_TILES);
                assert(leftItem);
                TableItem* rightItem = _view->item(rhs.second, COL_IDENTITY_TILES);
                assert(rightItem);
                return leftItem->data((int)eItemsRoleIdentityTilesNb).toInt() < rightItem->data((int)eItemsRoleIdentityTilesNb).toInt();
            } break;
            case COL_RENDERED_TILES:
            {
                TableItem* leftItem = _view->item(lhs.second, COL_RENDERED_TILES);
                assert(leftItem);
                TableItem* rightItem = _view->item(rhs.second, COL_RENDERED_TILES);
                assert(rightItem);
                return leftItem->data((int)eItemsRoleIdentityTilesNb).toInt() < rightItem->data((int)eItemsRoleIdentityTilesNb).toInt();
            } break;
            case COL_RENDERED_PLANES:
            {
                TableItem* leftItem = _view->item(lhs.second, COL_RENDERED_PLANES);
                assert(leftItem);
                TableItem* rightItem = _view->item(rhs.second, COL_RENDERED_PLANES);
                assert(rightItem);
                return leftItem->text() < rightItem->text();
            } break;
            case COL_NB_CACHE_HIT:
            {
                TableItem* leftItem = _view->item(lhs.second, COL_NB_CACHE_HIT);
                assert(leftItem);
                TableItem* rightItem = _view->item(rhs.second, COL_NB_CACHE_HIT);
                assert(rightItem);
                return leftItem->text() < rightItem->text();
            } break;
            case COL_NB_CACHE_HIT_DOWNSCALED:
            {
                TableItem* leftItem = _view->item(lhs.second, COL_NB_CACHE_HIT_DOWNSCALED);
                assert(leftItem);
                TableItem* rightItem = _view->item(rhs.second, COL_NB_CACHE_HIT_DOWNSCALED);
                assert(rightItem);
                return leftItem->text() < rightItem->text();
            } break;
            case COL_NB_CACHE_MISS:
            {
                TableItem* leftItem = _view->item(lhs.second, COL_NB_CACHE_MISS);
                assert(leftItem);
                TableItem* rightItem = _view->item(rhs.second, COL_NB_CACHE_MISS);
                assert(rightItem);
                return leftItem->text() < rightItem->text();
            } break;
        }
    }
};

class StatsTableModel : public TableModel
{
    TableView* view;
    std::vector<boost::weak_ptr<Natron::Node> > rows;
    
public:
    
    StatsTableModel(int row, int cols, TableView* view)
    : TableModel(row,cols,view)
    , view(view)
    , rows()
    {
        
    }
    
    virtual ~StatsTableModel() {}
    
    void clearRows()
    {
        clear();
        rows.clear();
    }
    
    const std::vector<boost::weak_ptr<Natron::Node> >& getRows() const
    {
        return rows;
    }
    
    void editNodeRow(const boost::shared_ptr<Natron::Node>& node, const NodeRenderStats& stats)
    {
        int row = -1;
        bool exists = false;
        for (std::size_t i = 0; i < rows.size(); ++i) {
            boost::shared_ptr<Natron::Node> n = rows[i].lock();
            if (n == node) {
                row = i;
                exists = true;
                break;
            }
        }
        
        
        if (row == -1) {
            row = rows.size();
        }
        
        if (row >= rowCount()) {
            insertRow(row);
        }
        
        QColor c;
        boost::shared_ptr<NodeGui> nodeUi = boost::dynamic_pointer_cast<NodeGui>(node->getNodeGui());
        if (nodeUi) {
            double r,g,b;
            nodeUi->getColor(&r, &g, &b);
            c.setRgbF(r, g, b);
        }
        
        {
            TableItem* item;
            if (exists) {
                item = view->item(row, COL_NAME);
            } else {
                item = new TableItem;
                
                QString tt = Natron::convertFromPlainText(QObject::tr("The label of the node as it appears on the nodegraph"), Qt::WhiteSpaceNormal);
                item->setToolTip(tt);
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            assert(item);
            boost::shared_ptr<NodeGui> nodeUi = boost::dynamic_pointer_cast<NodeGui>(node->getNodeGui());
            if (nodeUi) {
                item->setTextColor(Qt::black);
                item->setBackgroundColor(c);
            }
            item->setText(node->getLabel().c_str());
            if (!exists) {
                view->setItem(row, COL_NAME, item);
            }
        }
        
        
        {
            TableItem* item;
            if (exists) {
                item = view->item(row, COL_PLUGIN_ID);
            } else {
                item = new TableItem;
                QString tt = Natron::convertFromPlainText(QObject::tr("The ID of the plug-in embedded in the node"), Qt::WhiteSpaceNormal);
                item->setToolTip(tt);
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            assert(item);
            if (nodeUi) {
                item->setTextColor(Qt::black);
                item->setBackgroundColor(c);
            }
            item->setText(node->getPluginID().c_str());
            if (!exists) {
                view->setItem(row, COL_PLUGIN_ID, item);
            }
        }
        
        {
            TableItem* item;
            double timeSoFar;
            if (exists) {
                item = view->item(row, COL_TIME);
                timeSoFar = item->data((int)eItemsRoleTime).toDouble();
                timeSoFar += stats.getTotalTimeSpentRendering();
            } else {
                item = new TableItem;
                QString tt = Natron::convertFromPlainText(QObject::tr("The time spent rendering by this node across all threads"), Qt::WhiteSpaceNormal);
                item->setToolTip(tt);
                timeSoFar = stats.getTotalTimeSpentRendering();
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            assert(item);
            if (nodeUi) {
                item->setTextColor(Qt::black);
                item->setBackgroundColor(c);
            }
            item->setData((int)eItemsRoleTime, timeSoFar);
            item->setText(Timer::printAsTime(timeSoFar,false));
            
            if (!exists) {
                view->setItem(row, COL_TIME, item);
            }
        }
        {
            TableItem* item;
            if (exists) {
                item = view->item(row, COL_SUPPORT_TILES);
            } else {
                item = new TableItem;
                QString tt = Natron::convertFromPlainText(QObject::tr("Whether this node has tiles (portions of the final image) support or not"), Qt::WhiteSpaceNormal);
                item->setToolTip(tt);
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            assert(item);
            QString str;
            if (stats.isTilesSupportEnabled()) {
                str = "Yes";
            } else {
                str = "No";
            }
            item->setText(str);
            if (nodeUi) {
                item->setTextColor(Qt::black);
                item->setBackgroundColor(c);
            }
            if (!exists) {
                view->setItem(row, COL_SUPPORT_TILES, item);
            }
        }
        {
            TableItem* item;
            if (exists) {
                item = view->item(row, COL_SUPPORT_RS);
            } else {
                item = new TableItem;
                QString tt = Natron::convertFromPlainText(QObject::tr("Whether this node has render scale support or not.\n"
                                                                      "When activated, that means the node can render an image at a "
                                                                      "lower scale."), Qt::WhiteSpaceNormal);
                item->setToolTip(tt);
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            assert(item);
            QString str;
            if (stats.isRenderScaleSupportEnabled()) {
                str = "Yes";
            } else {
                str = "No";
            }
            item->setText(str);
            if (nodeUi) {
                item->setTextColor(Qt::black);
                item->setBackgroundColor(c);
            }
            if (!exists) {
                view->setItem(row, COL_SUPPORT_RS, item);
            }
        }
        {
            TableItem* item;
            QString str;
            if (exists) {
                item = view->item(row, COL_MIPMAP_LEVEL);
            } else {
                item = new TableItem;
                QString tt = Natron::convertFromPlainText(QObject::tr("The mipmaplevel rendered (See render-scale). 0 means scale = 100%, "
                                                                      "1 means scale = 50%, 2 means scale = 25%, etc..."), Qt::WhiteSpaceNormal);
                item->setToolTip(tt);
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            assert(item);
            const std::set<unsigned int>& mm = stats.getMipMapLevelsRendered();
            for (std::set<unsigned int>::const_iterator it = mm.begin(); it!=mm.end(); ++it) {
                str.append(QString::number(*it));
                str.append(' ');
            }
            item->setText(str);
            if (nodeUi) {
                item->setTextColor(Qt::black);
                item->setBackgroundColor(c);
            }
            if (!exists) {
                view->setItem(row, COL_MIPMAP_LEVEL, item);
            }
        }
        {
            TableItem* item;
            if (exists) {
                item = view->item(row, COL_CHANNELS);
            } else {
                item = new TableItem;
                QString tt = Natron::convertFromPlainText(QObject::tr("The channels processed by this node (corresponding to the RGBA checkboxes)"), Qt::WhiteSpaceNormal);
                item->setToolTip(tt);
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            assert(item);
            QString str;
            
            bool r,g,b,a;
            stats.getChannelsRendered(&r, &g, &b, &a);
            if (r) {
                str.append("R ");
            }
            if (g) {
                str.append("G ");
            }
            if (b) {
                str.append("B ");
            }
            if (a) {
                str.append("A");
            }
            item->setText(str);
            if (nodeUi) {
                item->setTextColor(Qt::black);
                item->setBackgroundColor(c);
            }
            if (!exists) {
                view->setItem(row, COL_CHANNELS, item);
            }
        }
        {
            TableItem* item;
            QString str;
            if (exists) {
                item = view->item(row, COL_PREMULT);
            } else {
                item = new TableItem;
                QString tt = Natron::convertFromPlainText(QObject::tr("The alpha premultiplication of the image produced by this node"), Qt::WhiteSpaceNormal);
                item->setToolTip(tt);
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            assert(item);
            Natron::ImagePremultiplicationEnum premult = stats.getOutputPremult();
            switch (premult) {
                case Natron::eImagePremultiplicationOpaque:
                    str = "Opaque";
                    break;
                case Natron::eImagePremultiplicationPremultiplied:
                    str = "Premultiplied";
                    break;
                case Natron::eImagePremultiplicationUnPremultiplied:
                    str = "Unpremultiplied";
                    break;
            }
            item->setText(str);
            if (nodeUi) {
                item->setTextColor(Qt::black);
                item->setBackgroundColor(c);
            }
            if (!exists) {
                view->setItem(row, COL_PREMULT, item);
            }
        }
        {
            TableItem* item;
            QString str;
            if (exists) {
                item = view->item(row, COL_ROD);
            } else {
                item = new TableItem;
                QString tt = Natron::convertFromPlainText(QObject::tr("The region of definition of the image produced"), Qt::WhiteSpaceNormal);
                item->setToolTip(tt);
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            assert(item);
            const RectD& rod = stats.getRoD();
            str = QString("(%1, %2, %3, %4)").arg(rod.x1).arg(rod.y1).arg(rod.x2).arg(rod.y2);
            item->setText(str);
            if (nodeUi) {
                item->setTextColor(Qt::black);
                item->setBackgroundColor(c);
            }
            if (!exists) {
                view->setItem(row, COL_ROD, item);
            }
        }
        {
            TableItem* item;
            if (exists) {
                item = view->item(row, COL_IDENTITY);
            } else {
                item = new TableItem;
                QString tt = Natron::convertFromPlainText(QObject::tr("When different of \"-\", this node does not render but rather "
                                                                      "directly returns the image produced by the node indicated by its label."), Qt::WhiteSpaceNormal);
                item->setToolTip(tt);
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            assert(item);
            QString str;
            boost::shared_ptr<Node> identity = stats.getInputImageIdentity();
            if (identity) {
                str = identity->getLabel().c_str();
            } else {
                str = "-";
            }
            item->setText(str);
            if (nodeUi) {
                item->setTextColor(Qt::black);
                item->setBackgroundColor(c);
            }
            if (!exists) {
                view->setItem(row, COL_IDENTITY, item);
            }
        }
        {
            TableItem* item;
            
            int nbIdentityTiles = 0;
            QString tilesInfo;
            if (exists) {
                item = view->item(row, COL_IDENTITY_TILES);
                nbIdentityTiles = item->data((int)eItemsRoleIdentityTilesNb).toInt();
                tilesInfo = item->data((int)eItemsRoleIdentityTilesInfo).toString();
            } else {
                item = new TableItem;
                QString tt = Natron::convertFromPlainText(QObject::tr("The list of the tiles that were identity in the image.\n"
                                                                      "Double-click for more info"), Qt::WhiteSpaceNormal);
                item->setToolTip(tt);
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            assert(item);
            std::list<std::pair<RectI,boost::shared_ptr<Natron::Node> > > tiles = stats.getIdentityRectangles();
            for (std::list<std::pair<RectI,boost::shared_ptr<Natron::Node> > >::iterator it = tiles.begin(); it!=tiles.end(); ++it) {
                
                const RectI& tile = it->first;
                QString tileEnc = QString("(%1, %2, %3, %4)").arg(tile.x1).arg(tile.y1).arg(tile.x2).arg(tile.y2);
                tilesInfo.append(tileEnc);
            }
            nbIdentityTiles += (int)tiles.size();
            
            item->setData((int)eItemsRoleIdentityTilesNb, nbIdentityTiles);
            item->setData((int)eItemsRoleIdentityTilesInfo, tilesInfo);
            
            QString str = QString::number(nbIdentityTiles) + QString(" tiles...");
            if (nodeUi) {
                item->setTextColor(Qt::black);
                item->setBackgroundColor(c);
            }
            item->setText(str);
            if (!exists) {
                view->setItem(row, COL_IDENTITY_TILES, item);
            }
        }
        {
            TableItem* item;
            
            int nbTiles = 0;
            QString tilesInfo;
            if (exists) {
                item = view->item(row, COL_RENDERED_TILES);
                nbTiles = item->data((int)eItemsRoleRenderedTilesNb).toInt();
                tilesInfo = item->data((int)eItemsRoleRenderedTilesInfo).toString();
            } else {
                item = new TableItem;
                QString tt = Natron::convertFromPlainText(QObject::tr("The list of the tiles effectivly rendered.\n"
                                                                      "Double-click for more infos."), Qt::WhiteSpaceNormal);
                item->setToolTip(tt);
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            assert(item);
            const std::list<RectI>& tiles = stats.getRenderedRectangles();
            for (std::list<RectI>::const_iterator it = tiles.begin(); it!=tiles.end(); ++it) {
                
                const RectI& tile = *it;
                QString tileEnc = QString("(%1, %2, %3, %4)").arg(tile.x1).arg(tile.y1).arg(tile.x2).arg(tile.y2);
                tilesInfo.append(tileEnc);
            }
            nbTiles += (int)tiles.size();
            
            item->setData((int)eItemsRoleRenderedTilesNb, nbTiles);
            item->setData((int)eItemsRoleRenderedTilesInfo, tilesInfo);
            
            QString str = QString::number(nbTiles) + QString(" tiles...");
            if (nodeUi) {
                item->setTextColor(Qt::black);
                item->setBackgroundColor(c);
            }
            item->setText(str);
            if (!exists) {
                view->setItem(row, COL_RENDERED_TILES, item);
            }
        }
        {
            TableItem* item;
            
            QString planesInfo;
            if (exists) {
                item = view->item(row, COL_RENDERED_PLANES);
            } else {
                item = new TableItem;
                QString tt = Natron::convertFromPlainText(QObject::tr("The list of the planes rendered by this node"), Qt::WhiteSpaceNormal);
                item->setToolTip(tt);
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            assert(item);
            const std::set<std::string>& planes = stats.getPlanesRendered();
            for (std::set<std::string>::const_iterator it = planes.begin(); it!=planes.end(); ++it) {
                if (!planesInfo.isEmpty()) {
                    planesInfo.append(' ');
                }
                planesInfo.append(it->c_str());
            }
            
            if (nodeUi) {
                item->setTextColor(Qt::black);
                item->setBackgroundColor(c);
            }
            item->setText(planesInfo);
            if (!exists) {
                view->setItem(row, COL_RENDERED_PLANES, item);
            }
        }
        {
            TableItem* item;
            
            int nb = 0;
            if (exists) {
                item = view->item(row, COL_NB_CACHE_HIT);
                nb = item->text().toInt();
            } else {
                item = new TableItem;
                QString tt = Natron::convertFromPlainText(QObject::tr("The number of cache hit (success)"), Qt::WhiteSpaceNormal);
                item->setToolTip(tt);
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            assert(item);
            int nbCacheMiss,nbCacheHits,nbCacheHitButDown;
            stats.getCacheAccessInfos(&nbCacheMiss, &nbCacheHits, &nbCacheHitButDown);
            nb += nbCacheHits;
            
            QString str = QString::number(nb);
            if (nodeUi) {
                item->setTextColor(Qt::black);
                item->setBackgroundColor(c);
            }
            item->setText(str);
            if (!exists) {
                view->setItem(row, COL_NB_CACHE_HIT, item);
            }
        }
        {
            TableItem* item;
            
            int nb = 0;
            if (exists) {
                item = view->item(row, COL_NB_CACHE_HIT_DOWNSCALED);
                nb = item->text().toInt();
            } else {
                item = new TableItem;
                QString tt = Natron::convertFromPlainText(QObject::tr("The number of cache access hit (success) but at higher scale "
                                                                      "hence requiring downscaling."), Qt::WhiteSpaceNormal);
                item->setToolTip(tt);
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            assert(item);
            int nbCacheMiss,nbCacheHits,nbCacheHitButDown;
            stats.getCacheAccessInfos(&nbCacheMiss, &nbCacheHits, &nbCacheHitButDown);
            nb += nbCacheHitButDown;
            
            QString str = QString::number(nb);
            if (nodeUi) {
                item->setTextColor(Qt::black);
                item->setBackgroundColor(c);
            }
            item->setText(str);
            if (!exists) {
                view->setItem(row, COL_NB_CACHE_HIT_DOWNSCALED, item);
            }
        }
        {
            TableItem* item;
            
            int nb = 0;
            if (exists) {
                item = view->item(row, COL_NB_CACHE_MISS);
                if (item) {
                    nb = item->text().toInt();
                }
            } else {
                item = new TableItem;
                QString tt = Natron::convertFromPlainText(QObject::tr("The number of cache access miss (image missing)"), Qt::WhiteSpaceNormal);
                item->setToolTip(tt);
                item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
            assert(item);
            int nbCacheMiss,nbCacheHits,nbCacheHitButDown;
            stats.getCacheAccessInfos(&nbCacheMiss, &nbCacheHits, &nbCacheHitButDown);
            nb += nbCacheMiss;
            
            QString str = QString::number(nb);
            if (nodeUi) {
                item->setTextColor(Qt::black);
                item->setBackgroundColor(c);
            }
            item->setText(str);
            if (!exists) {
                view->setItem(row, COL_NB_CACHE_MISS, item);
            }
        }
        if (!exists) {
            rows.push_back(node);
        }
    }

    
    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) OVERRIDE FINAL
    {
        if (column < 0 || column >= NUM_COLS) {
            return;
        }
        std::vector<std::pair<boost::weak_ptr<Natron::Node>,int> > vect;
        for (std::size_t i = 0; i < rows.size(); ++i) {
            vect.push_back(std::make_pair(rows[i], i));
        }
        Q_EMIT layoutAboutToBeChanged();
        switch (column) {
            case COL_NAME: {
                StatRowsCompare<COL_NAME> o(view);
                std::sort(vect.begin(), vect.end(), o);
            }   break;
            case COL_PLUGIN_ID: {
                StatRowsCompare<COL_PLUGIN_ID> o(view);
                std::sort(vect.begin(), vect.end(), o);
            }   break;
            case COL_TIME: {
                StatRowsCompare<COL_TIME> o(view);
                std::sort(vect.begin(), vect.end(), o);
            }   break;
            case COL_SUPPORT_TILES: {
                StatRowsCompare<COL_SUPPORT_TILES> o(view);
                std::sort(vect.begin(), vect.end(), o);
            }   break;
            case COL_SUPPORT_RS: {
                StatRowsCompare<COL_SUPPORT_RS> o(view);
                std::sort(vect.begin(), vect.end(), o);
            }   break;
            case COL_MIPMAP_LEVEL: {
                StatRowsCompare<COL_MIPMAP_LEVEL> o(view);
                std::sort(vect.begin(), vect.end(), o);
            }   break;
            case COL_CHANNELS: {
                StatRowsCompare<COL_CHANNELS> o(view);
                std::sort(vect.begin(), vect.end(), o);
            }   break;
            case COL_PREMULT: {
                StatRowsCompare<COL_PREMULT> o(view);
                std::sort(vect.begin(), vect.end(), o);
            }   break;
            case COL_ROD: {
                StatRowsCompare<COL_ROD> o(view);
                std::sort(vect.begin(), vect.end(), o);
            }   break;
            case COL_IDENTITY: {
                StatRowsCompare<COL_IDENTITY> o(view);
                std::sort(vect.begin(), vect.end(), o);
            }   break;
            case COL_IDENTITY_TILES: {
                StatRowsCompare<COL_IDENTITY_TILES> o(view);
                std::sort(vect.begin(), vect.end(), o);
            }   break;
            case COL_RENDERED_TILES: {
                StatRowsCompare<COL_RENDERED_TILES> o(view);
                std::sort(vect.begin(), vect.end(), o);
            }   break;
            case COL_RENDERED_PLANES: {
                StatRowsCompare<COL_RENDERED_PLANES> o(view);
                std::sort(vect.begin(), vect.end(), o);
            }   break;
            case COL_NB_CACHE_HIT: {
                StatRowsCompare<COL_NB_CACHE_HIT> o(view);
                std::sort(vect.begin(), vect.end(), o);
            }   break;
            case COL_NB_CACHE_HIT_DOWNSCALED: {
                StatRowsCompare<COL_NB_CACHE_HIT_DOWNSCALED> o(view);
                std::sort(vect.begin(), vect.end(), o);
            }   break;
            case COL_NB_CACHE_MISS: {
                StatRowsCompare<COL_NB_CACHE_MISS> o(view);
                std::sort(vect.begin(), vect.end(), o);
            }   break;
            default:
                break;
        }
        if (order == Qt::DescendingOrder) {
            std::vector<std::pair<boost::weak_ptr<Natron::Node>,int> > copy = vect;
            for (std::size_t i = 0; i < copy.size(); ++i) {
                vect[vect.size() - i - 1] = copy[i];
            }
        }
        QVector<TableItem*> newTable(vect.size() * NUM_COLS);
        for (std::size_t i = 0; i < vect.size(); ++i) {
            rows[i] = vect[i].first;
            for (int j = 0; j < NUM_COLS; ++j) {
                TableItem* item = takeItem(vect[i].second, j);
                assert(item);
                newTable[(i * NUM_COLS) + j] = item;
            }
        }
        setTable(newTable);
        Q_EMIT layoutChanged();
    }
};

struct RenderStatsDialogPrivate
{
    Gui* gui;
    
    QVBoxLayout* mainLayout;
    
    Natron::Label* descriptionLabel;
    QWidget* globalInfosContainer;
    QHBoxLayout* globalInfosLayout;
    
    Natron::Label* accumulateLabel;
    QCheckBox* accumulateCheckbox;
    
    Natron::Label* advancedLabel;
    QCheckBox* advancedCheckbox;
    
    Natron::Label* totalTimeSpentDescLabel;
    Natron::Label* totalTimeSpentValueLabel;
    double totalSpentTime;
    
    Button* resetButton;
    
    QWidget* filterContainer;
    QHBoxLayout* filterLayout;
    
    Natron::Label* filtersLabel;
    
    Natron::Label* nameFilterLabel;
    LineEdit* nameFilterEdit;
    
    Natron::Label* idFilterLabel;
    LineEdit* idFilterEdit;
    
    Natron::Label* useUnixWildcardsLabel;
    QCheckBox* useUnixWildcardsCheckbox;
    
  
    TableView* view;
    StatsTableModel* model;
    
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
    , model(0)
    {
        
    }
    
    void editNodeRow(const boost::shared_ptr<Natron::Node>& node, const NodeRenderStats& stats);
    
    void updateVisibleRowsInternal(const QString& nameFilter, const QString& pluginIDFilter);
        

};

RenderStatsDialog::RenderStatsDialog(Gui* gui)
: QWidget(gui)
, _imp(new RenderStatsDialogPrivate(gui))
{
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    setWindowTitle(tr("Render statistics"));
    
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    
    _imp->mainLayout = new QVBoxLayout(this);
    
    QString statsDesc = Natron::convertFromPlainText(tr(
    "Statistics are accumulated over each frame rendered by default.\nIf you want to display statistics of the last "
    "frame rendered, uncheck the \"Accumulate\" checkbox.\nIf you want to have more detailed informations besides the time spent "
    "rendering for each node,\ncheck the \"Advanced\" checkbox.\n The \"Time spent to render\" is the time spent "
    "to render a frame (or more if \"Accumulate\" is checked).\nIf there are multiple parallel renders (see preferences) "
    "the time will be accumulated across each threads.\nHover the mouse over each column header to have detailed informations "
    "for each statistic.\nBy default, nodes are sorted by decreasing time spent to render.\nClicking on a node will center "
    "the node-graph on it.\nWhen in \"Advanced\" mode, double-clicking on the \"Rendered Tiles\" or "
    "the \"Identity Tiles\" cell\nwill open-up a window containing detailed informations about the tiles rendered.\n"),Qt::WhiteSpaceNormal);
    _imp->descriptionLabel = new Natron::Label(statsDesc, this);
    _imp->mainLayout->addWidget(_imp->descriptionLabel);
    
    _imp->globalInfosContainer = new QWidget(this);
    _imp->globalInfosLayout = new QHBoxLayout(_imp->globalInfosContainer);
    
    QString accTt = Natron::convertFromPlainText(tr("When checked, stats are not cleared between the computation of frames."),Qt::WhiteSpaceNormal);
    _imp->accumulateLabel = new Natron::Label(tr("Accumulate:"), _imp->globalInfosContainer);
    _imp->accumulateLabel->setToolTip(accTt);
    _imp->accumulateCheckbox = new QCheckBox(_imp->globalInfosContainer);
    _imp->accumulateCheckbox->setChecked(true);
    _imp->accumulateCheckbox->setToolTip(accTt);
    
    _imp->globalInfosLayout->addWidget(_imp->accumulateLabel);
    _imp->globalInfosLayout->addWidget(_imp->accumulateCheckbox);
    
    _imp->globalInfosLayout->addSpacing(10);
    
    QString adTt = Natron::convertFromPlainText(tr("When checked, more statistics are displayed. Useful mainly for debuging purposes."),Qt::WhiteSpaceNormal);
    _imp->advancedLabel = new Natron::Label(tr("Advanced:"),_imp->globalInfosContainer);
    _imp->advancedLabel->setToolTip(adTt);
    _imp->advancedCheckbox = new QCheckBox(_imp->globalInfosContainer);
    _imp->advancedCheckbox->setChecked(false);
    _imp->advancedCheckbox->setToolTip(adTt);
    QObject::connect(_imp->advancedCheckbox, SIGNAL(clicked(bool)), this, SLOT(refreshAdvancedColsVisibility()));
    
    _imp->globalInfosLayout->addWidget(_imp->advancedLabel);
    _imp->globalInfosLayout->addWidget(_imp->advancedCheckbox);
    
    _imp->globalInfosLayout->addSpacing(20);
    
    QString wallTimett = Natron::convertFromPlainText(tr("This is the time spent to compute the frame for the whole tree.\n "
                                                         ),Qt::WhiteSpaceNormal);
    _imp->totalTimeSpentDescLabel = new Natron::Label(tr("Time spent to render:"),_imp->globalInfosContainer);
    _imp->totalTimeSpentDescLabel->setToolTip(wallTimett);
    _imp->totalTimeSpentValueLabel = new Natron::Label("0.0 sec", _imp->globalInfosContainer);
    _imp->totalTimeSpentValueLabel->setToolTip(wallTimett);
    
    _imp->globalInfosLayout->addWidget(_imp->totalTimeSpentDescLabel);
    _imp->globalInfosLayout->addWidget(_imp->totalTimeSpentValueLabel);
    
    _imp->resetButton = new Button(tr("Reset"), _imp->globalInfosContainer);
    _imp->resetButton->setToolTip(tr("Clears the statistics."));
    QObject::connect(_imp->resetButton, SIGNAL(clicked(bool)), this, SLOT(resetStats()));
    _imp->globalInfosLayout->addWidget(_imp->resetButton);
    
    _imp->globalInfosLayout->addStretch();
    
    _imp->mainLayout->addWidget(_imp->globalInfosContainer);
    
    _imp->filterContainer = new QWidget(this);
    _imp->filterLayout = new QHBoxLayout(_imp->filterContainer);
    
    _imp->filtersLabel = new Natron::Label(tr("Filters:"),_imp->filterContainer);
    _imp->filterLayout->addWidget(_imp->filtersLabel);
    
    _imp->filterLayout->addSpacing(10);
    
    QString nameFilterTt = Natron::convertFromPlainText(tr("If unix wildcards are enabled, show only nodes "
                                                           "with a label matching the filter.\nOtherwise if unix wildcards are disabled, "
                                                           "show only nodes with a label containing the text in the filter."), Qt::WhiteSpaceNormal);
    _imp->nameFilterLabel = new Natron::Label(tr("Name:"),_imp->filterContainer);
    _imp->nameFilterLabel->setToolTip(nameFilterTt);
    _imp->nameFilterEdit = new LineEdit(_imp->filterContainer);
    _imp->nameFilterEdit->setToolTip(nameFilterTt);
    QObject::connect(_imp->nameFilterEdit, SIGNAL(editingFinished()), this, SLOT(updateVisibleRows()));
    QObject::connect(_imp->nameFilterEdit, SIGNAL(textEdited(QString)), this, SLOT(onNameLineEditChanged(QString)));
    
    _imp->filterLayout->addWidget(_imp->nameFilterLabel);
    _imp->filterLayout->addWidget(_imp->nameFilterEdit);
    
    _imp->filterLayout->addSpacing(20);
    
    QString idFilterTt = Natron::convertFromPlainText(tr("If unix wildcards are enabled, show only nodes "
                                                           "with a plugin ID matching the filter.\nOtherwise if unix wildcards are disabled, "
                                                           "show only nodes with a plugin ID containing the text in the filter."), Qt::WhiteSpaceNormal);
    _imp->idFilterLabel = new Natron::Label(tr("Plugin ID:"),_imp->idFilterLabel);
    _imp->idFilterLabel->setToolTip(idFilterTt);
    _imp->idFilterEdit = new LineEdit(_imp->idFilterEdit);
    _imp->idFilterEdit->setToolTip(idFilterTt);
    QObject::connect(_imp->idFilterEdit, SIGNAL(editingFinished()), this, SLOT(updateVisibleRows()));
    QObject::connect(_imp->idFilterEdit, SIGNAL(textEdited(QString)), this, SLOT(onIDLineEditChanged(QString)));
    
    _imp->filterLayout->addWidget(_imp->idFilterLabel);
    _imp->filterLayout->addWidget(_imp->idFilterEdit);
    
    _imp->useUnixWildcardsLabel = new Natron::Label(tr("Use Unix wildcards (*, ?, etc..)"),_imp->filterContainer);
    _imp->useUnixWildcardsCheckbox = new QCheckBox(_imp->filterContainer);
    _imp->useUnixWildcardsCheckbox->setChecked(false);
    QObject::connect(_imp->useUnixWildcardsCheckbox, SIGNAL(toggled(bool)), this, SLOT(updateVisibleRows()));
    
    _imp->filterLayout->addWidget(_imp->useUnixWildcardsLabel);
    _imp->filterLayout->addWidget(_imp->useUnixWildcardsCheckbox);
    
    _imp->filterLayout->addStretch();
    
    _imp->mainLayout->addWidget(_imp->filterContainer);
    
    _imp->view = new TableView(this);
    
    _imp->model = new StatsTableModel(0,0,_imp->view);
    _imp->view->setTableModel(_imp->model);
    
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
    
    _imp->view->setColumnCount( dimensionNames.size() );
    _imp->view->setHorizontalHeaderLabels(dimensionNames);
    
    _imp->view->setAttribute(Qt::WA_MacShowFocusRect,0);
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
    QObject::connect(selModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(onSelectionChanged(QItemSelection,QItemSelection)));
    _imp->mainLayout->addWidget(_imp->view);
    
}

RenderStatsDialog::~RenderStatsDialog()
{
    
}

void
RenderStatsDialog::onSelectionChanged(const QItemSelection &selected, const QItemSelection &/*deselected*/)
{
  
    QModelIndexList indexes = selected.indexes();
    if (indexes.isEmpty()) {
        return;
    }
    int idx = indexes[0].row();
    const std::vector<boost::weak_ptr<Natron::Node> >& rows = _imp->model->getRows();
    if (idx < 0 || idx >= (int)rows.size()) {
        return;
    }
    boost::shared_ptr<Node> node = rows[idx].lock();
    if (!node) {
        return;
    }
    boost::shared_ptr<NodeGui> nodeUi = boost::dynamic_pointer_cast<NodeGui>(node->getNodeGui());
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
    _imp->totalTimeSpentValueLabel->setText("0.0 sec");
    _imp->totalSpentTime = 0;
}



void
RenderStatsDialog::addStats(int /*time*/, int /*view*/, double wallTime, const std::map<boost::shared_ptr<Natron::Node>,NodeRenderStats >& stats)
{
    
    if (!_imp->accumulateCheckbox->isChecked()) {
        _imp->model->clearRows();
        _imp->totalSpentTime = 0;
    }
    
    _imp->totalSpentTime += wallTime;
    _imp->totalTimeSpentValueLabel->setText(Timer::printAsTime(_imp->totalSpentTime, false));
    
    for (std::map<boost::shared_ptr<Natron::Node>,NodeRenderStats >::const_iterator it = stats.begin(); it!=stats.end(); ++it) {
        _imp->model->editNodeRow(it->first, it->second);
    }
    
    updateVisibleRows();
    if (!stats.empty()) {
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
RenderStatsDialogPrivate::updateVisibleRowsInternal(const QString& nameFilter, const QString& pluginIDFilter)
{
    QModelIndex rootIdx = view->rootIndex();
    const std::vector<boost::weak_ptr<Natron::Node> >& rows = model->getRows();

    
    if (useUnixWildcardsCheckbox->isChecked()) {
        QRegExp nameExpr(nameFilter,Qt::CaseInsensitive,QRegExp::Wildcard);
        if (!nameExpr.isValid()) {
            return;
        }
        
        QRegExp idExpr(pluginIDFilter,Qt::CaseInsensitive,QRegExp::Wildcard);
        if (!idExpr.isValid()) {
            return;
        }
   
        

        int i = 0;
        for (std::vector<boost::weak_ptr<Natron::Node> >::const_iterator it = rows.begin(); it != rows.end(); ++it,++i) {
            boost::shared_ptr<Node> node = it->lock();
            if (!node) {
                continue;
            }
            
            if ((nameFilter.isEmpty() || nameExpr.exactMatch(node->getLabel().c_str())) &&
                (pluginIDFilter.isEmpty() || idExpr.exactMatch(node->getPluginID().c_str()))) {
                
                if (view->isRowHidden(i, rootIdx)) {
                    view->setRowHidden(i, rootIdx, false);
                }
            } else {
                if (!view->isRowHidden(i, rootIdx)) {
                    view->setRowHidden(i, rootIdx, true);
                }
            }
        }
    } else {
        
        int i = 0;

        for (std::vector<boost::weak_ptr<Natron::Node> >::const_iterator it = rows.begin(); it != rows.end(); ++it,++i) {
            boost::shared_ptr<Node> node = it->lock();
            if (!node) {
                continue;
            }
            
            if ((nameFilter.isEmpty() || QString(node->getLabel().c_str()).contains(nameFilter)) &&
                (pluginIDFilter.isEmpty() || QString(node->getPluginID().c_str()).contains(pluginIDFilter))) {
                
                if (view->isRowHidden(i, rootIdx)) {
                    view->setRowHidden(i, rootIdx, false);
                }
            } else {
                if (!view->isRowHidden(i, rootIdx)) {
                    view->setRowHidden(i, rootIdx, true);
                }
            }
        }
  
    }

}

void
RenderStatsDialog::updateVisibleRows()
{
    _imp->updateVisibleRowsInternal(_imp->nameFilterEdit->text(), _imp->idFilterEdit->text());
}

void
RenderStatsDialog::onNameLineEditChanged(const QString& filter)
{
    _imp->updateVisibleRowsInternal(filter, _imp->idFilterEdit->text());
}

void
RenderStatsDialog::onIDLineEditChanged(const QString& filter)
{
    _imp->updateVisibleRowsInternal(_imp->nameFilterEdit->text(), filter);
}
