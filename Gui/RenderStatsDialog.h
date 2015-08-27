/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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

#ifndef RENDERSTATSDIALOG_H
#define RENDERSTATSDIALOG_H

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include <QWidget>

#include "Engine/RenderStats.h"

namespace Natron {
    class Node;
}

class TableItem;
class Gui;
class QItemSelection;

struct RenderStatsDialogPrivate;
class RenderStatsDialog : public QWidget
{
    
    Q_OBJECT
    
public:
    
    RenderStatsDialog(Gui* gui);
    
    virtual ~RenderStatsDialog();
    
    void addStats(int time, int view, double wallTime, const std::map<boost::shared_ptr<Natron::Node>,NodeRenderStats >& stats);
    
public Q_SLOTS:
    
    void resetStats();
    void refreshAdvancedColsVisibility();
    void onItemClicked(TableItem* item);
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    
private:
    
    virtual void closeEvent(QCloseEvent * event) OVERRIDE FINAL;
    
    boost::scoped_ptr<RenderStatsDialogPrivate> _imp;
};

#endif // RENDERSTATSDIALOG_H
