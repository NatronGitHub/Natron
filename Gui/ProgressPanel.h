/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#ifndef PROGRESSPANEL_H
#define PROGRESSPANEL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/PanelWidget.h"
#include "Gui/GuiFwd.h"

#include "Gui/TaskBar.h"

NATRON_NAMESPACE_ENTER

struct ProgressPanelPrivate;
struct ProgressPanelPrivate;
class ProgressPanel
    : public QWidget, public PanelWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    friend struct ProgressTaskInfoPrivate;

public:

    ProgressPanel(Gui* gui);

    virtual ~ProgressPanel();

    /**
     * @brief Add a new task to the tasks list.
     * The frame range/step passed are used to compute the progress
     * value if the task uses the progressUpdate(const int frame) function.
     **/
    void startTask( const NodePtr& node,
                    const int firstFrame,
                    const int lastFrame,
                    const int frameStep,
                    const bool canPause,
                    const bool canCancel,
                    const QString& message,
                    const ProcessHandlerPtr& process = ProcessHandlerPtr() );

    void onTaskRestarted( const NodePtr& node,
                          const ProcessHandlerPtr& process = ProcessHandlerPtr() );

    /**
     * @brief Start progress report for the given node. The progress bar will be displayed only if the estimated remaining
     * time is greater than NATRON_DISPLAY_PROGRESS_PANEL_AFTER_MS
     **/
    void progressStart(const NodePtr& node, const std::string &message, const std::string &messageid, bool canCancel = true);

    /**
     * @brief Increase progress to the specified value in the range [0., 1.], the time remaining
     * will be computed automatically.
     * @returns false if the task should be aborted, true otherwise.
     **/
    bool updateTask(const NodePtr& node, const double progress);

    /**
     * @brief Called to remove the task from the list
     **/
    void endTask(const NodePtr& node);

    void onRenderQueuingSettingChanged(bool queueingEnabled);

    void removeTaskFromTable(const ProgressTaskInfoPtr& task);
    void removeTasksFromTable(const std::list<ProgressTaskInfoPtr>& task);

    bool isRemoveTasksAfterFinishChecked() const;

    void getSelectedTask(std::list<ProgressTaskInfoPtr>& selection) const;

    void onLastTaskAddedFinished(const ProgressTaskInfo* task);

public Q_SLOTS:

    void onCancelTasksTriggered();

    void doProgressStartOnMainThread(const NodePtr& node, const QString &message, const QString &messageid, bool canCancel);

    void doProgressOnMainThread(const ProgressTaskInfoPtr& task, double progress);

    void doProgressEndOnMainThread(const NodePtr& node);

    void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    void onQueueRendersCheckboxChecked();

    void onShowProgressPanelTimerTriggered();

    void onItemRightClicked(TableItem* item);

Q_SIGNALS:

    void s_doProgressStartOnMainThread(const NodePtr& node, const QString &message, const QString &messageid, bool canCancel);

    void s_doProgressUpdateOnMainThread(const ProgressTaskInfoPtr& task, double progress);

    void s_doProgressEndOnMainThread(const NodePtr& node);

private:
    void getSelectedTaskInternal(const QItemSelection& selected, std::list<ProgressTaskInfoPtr>& selection) const;

    void addTaskToTable(const ProgressTaskInfoPtr& task);

private:
    // overridden from QWidget
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void keyReleaseEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
    std::unique_ptr<ProgressPanelPrivate> _imp;
    TaskBar *_taskbar;
};

NATRON_NAMESPACE_EXIT

#endif // PROGRESSPANEL_H
