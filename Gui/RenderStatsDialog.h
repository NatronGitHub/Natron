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

#ifndef RENDERSTATSDIALOG_H
#define RENDERSTATSDIALOG_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/RenderStats.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

struct RenderStatsDialogPrivate;

class RenderStatsDialog
    : public QWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    RenderStatsDialog(Gui* gui);

    virtual ~RenderStatsDialog();

    void addStats(int time, ViewIdx view, double wallTime, const std::map<NodePtr, NodeRenderStats >& stats);

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

NATRON_NAMESPACE_EXIT

#endif // RENDERSTATSDIALOG_H
