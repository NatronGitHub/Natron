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

#ifndef Gui_ProgressTaskInfo_h
#define Gui_ProgressTaskInfo_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <QObject>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include "Gui/GuiFwd.h"


NATRON_NAMESPACE_ENTER;

struct ProgressTaskInfoPrivate;
class ProgressTaskInfo : public QObject, public boost::enable_shared_from_this<ProgressTaskInfo>
{
    
    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON
    
    friend class ProgressPanel;
    
public:
    
    ProgressTaskInfo(ProgressPanel* panel,
             const NodePtr& node,
             const int firstFrame,
             const int lastFrame,
             const int frameStep,
             const bool canPause,
             const bool canCancel,
             const QString& message,
             const boost::shared_ptr<ProcessHandler>& process);
    
    virtual ~ProgressTaskInfo();
    
    bool wasCanceled() const;
    
    bool canPause() const;
    
    /**
     * @brief If the task has been restarted, totalProgress is the progress over the whole task,
     * and subTaskProgress is the progress over the smaller range from which we stopped.
     **/
    void updateProgressBar(double totalProgress,double subTaskProgress);
    
    void updateProgress(const int frame, double progress);
    
    void cancelTask(bool calledFromRenderEngine, int retCode);
    
    void restartTask();
    
    NodePtr getNode() const;
    
    public Q_SLOTS:
    
    void onRefreshLabelTimeout();
    
    /**
     * @brief Slot executed when a render engine reports progress
     **/
    void onRenderEngineFrameComputed(int frame, double progress);
    
    /**
     * @brief Executed when a render engine stops, retCode can be 1 in which case that means the render
     * was aborted, or 0 in which case the render was successful or a failure.
     **/
    void onRenderEngineStopped(int retCode);
    
    void onProcessCanceled();
    
    void getTableItems(std::vector<TableItem*>* items) const;
    
Q_SIGNALS:
    
    void taskCanceled();
    
private:
    
    void setCellWidgets(int row, TableView* view);
    void removeCellWidgets(int row, TableView* view);
    void createCellWidgets();

    
    void setProcesshandler(const boost::shared_ptr<ProcessHandler>& process);

    
    void clearItems();
    
    boost::scoped_ptr<ProgressTaskInfoPrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // Gui_ProgressTaskInfo_h
