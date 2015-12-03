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

#ifndef RENDERSTATSDIALOG_H
#define RENDERSTATSDIALOG_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include <QWidget>

#include "Engine/RenderStats.h"
#include "Engine/EngineFwd.h"


class QItemSelection;


class TableItem;
class Gui;


struct RenderStatsDialogPrivate;
class RenderStatsDialog : public QWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    
    RenderStatsDialog(Gui* gui);
    
    virtual ~RenderStatsDialog();
    
    void addStats(int time, int view, double wallTime, const std::map<boost::shared_ptr<Natron::Node>,NodeRenderStats >& stats);
    
public Q_SLOTS:
    
    void resetStats();
    void refreshAdvancedColsVisibility();
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    
    void updateVisibleRows();
    void onNameLineEditChanged(const QString& filter);
    void onIDLineEditChanged(const QString& filter);
    
private:
    
    virtual void closeEvent(QCloseEvent * event) OVERRIDE FINAL;
    
    boost::scoped_ptr<RenderStatsDialogPrivate> _imp;
};

#endif // RENDERSTATSDIALOG_H
