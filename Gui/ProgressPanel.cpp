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

#include "ProgressPanel.h"

#include <map>

#include <QMutex>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QItemSelectionModel>
#include <QTextEdit>
#include <QApplication>
#include <QThread>
#include <QHeaderView>
#include <QCheckBox>
#include <QTimer>

#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/Timer.h"
#include "Engine/OutputEffectInstance.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Image.h"
#include "Engine/ProcessHandler.h"
#include "Engine/Settings.h"

#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/Label.h"
#include "Gui/ProgressTaskInfo.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/Button.h"
#include "Gui/Utils.h"
#include "Gui/NodeGui.h"
#include "Gui/TableModelView.h"



#define COL_TIME_REMAINING 4
#define COL_LAST 6

NATRON_NAMESPACE_ENTER;

typedef std::map<NodeWPtr, ProgressTaskInfoPtr> TasksMap;
typedef std::vector<ProgressTaskInfoPtr> TasksOrdered;
struct ProgressPanelPrivate
{
    
    QVBoxLayout* mainLayout;
    
    QWidget* headerContainer;
    QHBoxLayout* headerLayout;
    Button* restartTasksButton;
    Button* pauseTasksButton;
    Button* cancelTasksButton;
    
    QCheckBox* queueTasksCheckbox;
    QCheckBox* removeTasksAfterFinishCheckbox;
    
    TableModel* model;
    TableView* view;
    
    mutable QMutex tasksMutex;
    TasksMap tasks;
    TasksOrdered tasksOrdered;
    
    ProgressPanelPrivate()
    : mainLayout(0)
    , headerContainer(0)
    , headerLayout(0)
    , restartTasksButton(0)
    , pauseTasksButton(0)
    , cancelTasksButton(0)
    , queueTasksCheckbox(0)
    , removeTasksAfterFinishCheckbox(0)
    , model(0)
    , view(0)
    , tasksMutex()
    , tasks()
    , tasksOrdered()
    {
        
    }
    
    ProgressTaskInfoPtr findTask(const NodePtr& node) const
    {
        assert(!tasksMutex.tryLock());
        
        for (TasksMap::const_iterator it = tasks.begin(); it!=tasks.end(); ++it) {
            if (it->first.lock() == node) {
                return it->second;
            }
        }
        return ProgressTaskInfoPtr();
    }
    
    
    void refreshButtonsEnableness(const std::list<ProgressTaskInfoPtr>& selection);
};



ProgressPanel::ProgressPanel(Gui* gui)
: QWidget(gui)
, PanelWidget(this, gui)
, _imp(new ProgressPanelPrivate())
{
    
    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    _imp->mainLayout->setSpacing(0);
    _imp->headerContainer = new QWidget(this);
    _imp->headerLayout = new QHBoxLayout(_imp->headerContainer);
    _imp->headerLayout->setSpacing(0);
   // _imp->headerLayout->setContentsMargins(0, 0, 0, 0);
    _imp->mainLayout->addWidget(_imp->headerContainer);
    
    int medSizeIcon = TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE);
    
    QPixmap restartPix,pausePix, clearTasksPix;
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PLAY_DISABLED, medSizeIcon, &restartPix);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_STOP, medSizeIcon, &clearTasksPix);
    
    const QSize medButtonIconSize(TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE),TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE));
    const QSize medButtonSize(TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE),TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE));
    
    _imp->pauseTasksButton = new Button(QIcon(pausePix),"Pause",_imp->headerContainer);
    _imp->pauseTasksButton->setFixedSize(medButtonSize);
    _imp->pauseTasksButton->setIconSize(medButtonIconSize);
    _imp->pauseTasksButton->setFocusPolicy(Qt::NoFocus);
    _imp->pauseTasksButton->setToolTip(GuiUtils::convertFromPlainText(tr("Pause selected tasks. Tasks that can be paused "
                                                                        "may be restarted with the restart button."), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->pauseTasksButton, SIGNAL(clicked(bool)), this, SLOT(onPauseTasksTriggered()));
    _imp->pauseTasksButton->setEnabled(false);
    _imp->headerLayout->addWidget(_imp->pauseTasksButton);
    
    _imp->restartTasksButton = new Button(QIcon(restartPix),"",_imp->headerContainer);
    _imp->restartTasksButton->setFixedSize(medButtonSize);
    _imp->restartTasksButton->setIconSize(medButtonIconSize);
    _imp->restartTasksButton->setFocusPolicy(Qt::NoFocus);
    _imp->restartTasksButton->setToolTip(GuiUtils::convertFromPlainText(tr("Restart tasks in order"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->restartTasksButton, SIGNAL(clicked(bool)), this, SLOT(onRestartAllTasksTriggered()));
    _imp->restartTasksButton->setEnabled(false);
    _imp->headerLayout->addWidget(_imp->restartTasksButton);
    
    
    _imp->headerLayout->addSpacing(TO_DPIX(20));
    
    _imp->cancelTasksButton = new Button(QIcon(clearTasksPix),"",_imp->headerContainer);
    _imp->cancelTasksButton->setFixedSize(medButtonSize);
    _imp->cancelTasksButton->setIconSize(medButtonIconSize);
    _imp->cancelTasksButton->setFocusPolicy(Qt::NoFocus);
    _imp->cancelTasksButton->setEnabled(false);
    _imp->cancelTasksButton->setToolTip(GuiUtils::convertFromPlainText(tr("Remove from the list the selected tasks"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->cancelTasksButton, SIGNAL(clicked(bool)), this, SLOT(onCancelTasksTriggered()));
    _imp->headerLayout->addWidget(_imp->cancelTasksButton);
    
    _imp->headerLayout->addSpacing(TO_DPIX(20));
    
    _imp->queueTasksCheckbox = new QCheckBox(tr("Queue Renders"),_imp->headerContainer);
    _imp->queueTasksCheckbox->setToolTip(GuiUtils::convertFromPlainText(tr("When checked, renders will be queued in the Progress Panel and will start only when all other prior renders are done. This does not apply to other tasks such as Tracking or analysis."), Qt::WhiteSpaceNormal));
    _imp->queueTasksCheckbox->setChecked(appPTR->getCurrentSettings()->isRenderQueuingEnabled());
    QObject::connect(_imp->queueTasksCheckbox, SIGNAL(stateChanged(int)), this, SLOT(onQueueRendersCheckboxChecked()));
    _imp->headerLayout->addWidget(_imp->queueTasksCheckbox);
    
    _imp->headerLayout->addSpacing(TO_DPIX(20));
    
    _imp->removeTasksAfterFinishCheckbox = new QCheckBox(tr("Remove Finished Tasks"),_imp->headerContainer);
    _imp->removeTasksAfterFinishCheckbox->setToolTip(GuiUtils::convertFromPlainText(tr("When checked, finished tasks that can be paused"  " will be automatically removed from the task list when they are finished. When unchecked, the tasks may be restarted."), Qt::WhiteSpaceNormal));
    _imp->removeTasksAfterFinishCheckbox->setChecked(true);
    _imp->headerLayout->addWidget(_imp->removeTasksAfterFinishCheckbox);

    
    
    _imp->headerLayout->addStretch();
    
    
    _imp->view = new TableView(this);
    _imp->view->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    _imp->model = new TableModel(0,0, _imp->view);
    _imp->view->setSortingEnabled(false);
    _imp->view->setTableModel(_imp->model);
    
    _imp->mainLayout->addWidget(_imp->view);

    
    QStringList dimensionNames;
    dimensionNames
    << tr("Node")
    << tr("Progress")
    << tr("Status")
    << tr("Can Pause")
    << tr("Time remaining")
    << tr("Task");
    
    _imp->view->setColumnCount(dimensionNames.size());
    _imp->view->setHorizontalHeaderLabels(dimensionNames);
    _imp->view->header()->setResizeMode(QHeaderView::Fixed);
    _imp->view->header()->setStretchLastSection(true);
    _imp->view->header()->resizeSection(COL_TIME_REMAINING, TO_DPIX(150));

    QItemSelectionModel* selModel = _imp->view->selectionModel();
    QObject::connect(selModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(onSelectionChanged(QItemSelection,QItemSelection)));
    
}

ProgressPanel::~ProgressPanel()
{
    
}

void
ProgressPanel::onSelectionChanged(const QItemSelection& selected, const QItemSelection& /*deselected*/)
{
    std::list<ProgressTaskInfoPtr> selection;
    getSelectedTaskInternal(selected, selection);
    _imp->refreshButtonsEnableness(selection);
}

void
ProgressPanel::getSelectedTaskInternal(const QItemSelection& selected, std::list<ProgressTaskInfoPtr>& selection) const
{
    std::set<int> rows;
    QModelIndexList selectedIndex = selected.indexes();
    for (int i = 0; i < selectedIndex.size(); ++i) {
        rows.insert(selectedIndex[i].row());
    }
    for (std::set<int>::iterator it = rows.begin(); it!=rows.end(); ++it) {
        if ((*it) >= 0 && (*it) < (int)_imp->tasksOrdered.size()) {
            selection.push_back(_imp->tasksOrdered[*it]);
        }
    }
}

void
ProgressPanel::getSelectedTask(std::list<ProgressTaskInfoPtr>& selection) const
{
    const QItemSelection selected = _imp->view->selectionModel()->selection();
    getSelectedTaskInternal(selected, selection);
}

void
ProgressPanel::keyPressEvent(QKeyEvent* e)
{
    
    //Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();
    
    bool accept = true;
    if (key == Qt::Key_Delete || key == Qt::Key_Backspace) {
        onCancelTasksTriggered();
    } else {
        accept = false;
    }
    if (accept) {
        takeClickFocus();
        e->accept();
    } else {
        handleUnCaughtKeyPressEvent(e);
        QWidget::keyPressEvent(e);
    }
}

void
ProgressPanel::keyReleaseEvent(QKeyEvent* e)
{
    handleUnCaughtKeyUpEvent(e);
    QWidget::keyReleaseEvent(e);
}

void
ProgressPanel::enterEvent(QEvent* e) {
    enterEventBase();
    QWidget::enterEvent(e);
}

void
ProgressPanel::leaveEvent(QEvent* e)
{
    leaveEventBase();
    QWidget::leaveEvent(e);
}

void
ProgressPanel::onPauseTasksTriggered()
{
    std::list<ProgressTaskInfoPtr> selection;
    getSelectedTask(selection);
    for (std::list<ProgressTaskInfoPtr>::iterator it = selection.begin(); it!=selection.end(); ++it) {
        if ((*it)->canPause()) {
            (*it)->cancelTask(false, 0);
        }
    }
    
    _imp->refreshButtonsEnableness(selection);
}

void
ProgressPanel::onCancelTasksTriggered()
{
    std::list<ProgressTaskInfoPtr> selection;
    getSelectedTask(selection);
    for (std::list<ProgressTaskInfoPtr>::iterator it = selection.begin(); it!=selection.end(); ++it) {
        (*it)->cancelTask(false, 0);
    }
    
    removeTasksFromTable(selection);
    
    getSelectedTask(selection);
    _imp->refreshButtonsEnableness(selection);
    

}

void
ProgressPanel::removeTaskFromTable(const ProgressTaskInfoPtr& task)
{
    std::list<ProgressTaskInfoPtr> list;
    list.push_back(task);
    removeTasksFromTable(list);
}

void
ProgressPanel::removeTasksFromTable(const std::list<ProgressTaskInfoPtr>& tasks)
{
    std::vector<TableItem*> table;
    std::vector<ProgressTaskInfoPtr> newOrder;
    
    {
        QMutexLocker k(&_imp->tasksMutex);
        
        int rc = _imp->view->rowCount();
        int cc = _imp->view->columnCount();
        assert((int)_imp->tasksOrdered.size() == rc);
        
        for (int i = 0; i < rc; ++i) {
            std::list<ProgressTaskInfoPtr>::const_iterator foundSelected = std::find(tasks.begin(), tasks.end(), _imp->tasksOrdered[i]);
            _imp->tasksOrdered[i]->removeCellWidgets(i, _imp->view);
            if (foundSelected != tasks.end()) {
                (*foundSelected)->clearItems();
                TasksMap::iterator foundInMap = _imp->tasks.find((*foundSelected)->getNode());
                if (foundInMap != _imp->tasks.end()) {
                    _imp->tasks.erase(foundInMap);
                }
                continue;
            }
            for (int j = 0; j < cc; ++j) {
                table.push_back(_imp->view->takeItem(i, j));
            }
            
            
            newOrder.push_back(_imp->tasksOrdered[i]);
        }
        _imp->tasksOrdered = newOrder;
    }
    _imp->model->setTable(table);
    
    ///Refresh custom widgets
    for (std::size_t i = 0; i < newOrder.size(); ++i) {
        _imp->tasksOrdered[i]->createCellWidgets();
        _imp->tasksOrdered[i]->setCellWidgets(i, _imp->view);
    }
}

void
ProgressPanel::onRestartAllTasksTriggered()
{
    std::list<ProgressTaskInfoPtr> selection;
    getSelectedTask(selection);
    for (std::list<ProgressTaskInfoPtr>::iterator it = selection.begin(); it!=selection.end(); ++it) {
        if ((*it)->wasCanceled() && (*it)->canPause())
            (*it)->restartTask();
    }
    _imp->refreshButtonsEnableness(selection);
}

void
ProgressPanel::refreshButtonsEnabledNess()
{
    std::list<ProgressTaskInfoPtr> selection;
    getSelectedTask(selection);
    _imp->refreshButtonsEnableness(selection);
}

static void connectProcessSlots(ProgressTaskInfo* task, ProcessHandler* process)
{
    QObject::connect(task,SIGNAL(taskCanceled()),process,SLOT(onProcessCanceled()));
    QObject::connect(process,SIGNAL(processCanceled()),task,SLOT(onProcessCanceled()));
    QObject::connect(process,SIGNAL(frameRendered(int,double)),task,SLOT(onRenderEngineFrameComputed(int,double)));
    QObject::connect(process,SIGNAL(processFinished(int)),task,SLOT(onRenderEngineStopped(int)));

}

void
ProgressPanel::onTaskRestarted(const NodePtr& node,
                     const boost::shared_ptr<ProcessHandler>& process)
{
    QMutexLocker k(&_imp->tasksMutex);
    ProgressTaskInfoPtr task;
    task = _imp->findTask(node);
    if (!task) {
        return;
    }
    //The process may have changed
    task->setProcesshandler(process);
    connectProcessSlots(task.get(), process.get());
}

void
ProgressPanel::startTask(const NodePtr& node,
                         const int firstFrame,
                         const int lastFrame,
                         const int frameStep,
                         const bool canPause,
                         const bool canCancel,
                         const QString& message,
                         const boost::shared_ptr<ProcessHandler>& process)
{
    if (!node) {
        return;
    }
    assert((canPause && firstFrame != INT_MIN && lastFrame != INT_MAX) || !canPause);
    
    ProgressTaskInfoPtr task;
    {
        
        QMutexLocker k(&_imp->tasksMutex);
        task = _imp->findTask(node);
        if (task) {
            task->cancelTask(false, 1);
            k.unlock();
            removeTaskFromTable(task);
        }
    }

    
    QMutexLocker k(&_imp->tasksMutex);
    
    task.reset(new ProgressTaskInfo(this,
                            node,
                            firstFrame,
                            lastFrame,
                            frameStep,
                            canPause,
                            canCancel,
                            message, process));
    
    
    
    
    if (process) {
        connectProcessSlots(task.get(), process.get());
    }
    if (!process) {
        if (node->getEffectInstance()->isWriter()) {
            OutputEffectInstance* isOutput = dynamic_cast<OutputEffectInstance*>(node->getEffectInstance().get());
            if (isOutput) {
                RenderEngine* engine = isOutput->getRenderEngine();
                assert(engine);
                QObject::connect(engine,SIGNAL(frameRendered(int,double)), task.get(), SLOT(onRenderEngineFrameComputed(int,double)));
                QObject::connect(engine, SIGNAL(renderFinished(int)), task.get(), SLOT(onRenderEngineStopped(int)));
                QObject::connect(task.get(),SIGNAL(taskCanceled()),engine,SLOT(abortRendering_Blocking()));
            }
        }
    }
    _imp->tasks[node] = task;

}

void
ProgressPanel::addTaskToTable(const ProgressTaskInfoPtr& task)
{
    assert(QThread::currentThread() == qApp->thread());
    int rc = _imp->view->rowCount();
    _imp->view->setRowCount(rc + 1);
    
    std::vector<TableItem*> items;
    task->getTableItems(&items);
    assert(items.size() == COL_LAST);
    
    for (std::size_t i = 0; i < items.size(); ++i) {
        _imp->view->setItem(rc, i, items[i]);
    }
    task->setCellWidgets(rc, _imp->view);

    _imp->tasksOrdered.push_back(task);
    
    getGui()->ensureProgressPanelVisible();
    
    refreshButtonsEnabledNess();
}

void
ProgressPanel::doProgressOnMainThread(const ProgressTaskInfoPtr& task, double progress)
{
    task->updateProgressBar(progress, progress);
}

bool
ProgressPanel::updateTask(const NodePtr& node, const double progress)
{
    
    bool isMainThread = QThread::currentThread() == qApp->thread();
    ProgressTaskInfoPtr foundTask;
    {
        QMutexLocker k(&_imp->tasksMutex);
        foundTask = _imp->findTask(node);
    }
    if (!foundTask) {
        return false;
    }
    if (!isMainThread) {
        Q_EMIT s_doProgressUpdateOnMainThread(foundTask, progress);
    } else {
        doProgressOnMainThread(foundTask, progress);
    }

    if (foundTask->wasCanceled()) {
        return false;
    }
    
    if (isMainThread) {
        QCoreApplication::processEvents();
    }
    
    return true;
}


void
ProgressPanel::doProgressEndOnMainThread(const NodePtr& node)
{
    ProgressTaskInfoPtr task;
    {
        QMutexLocker k(&_imp->tasksMutex);
        task= _imp->findTask(node);
    }
    
    if (!task) {
        return;
    }
    
    {
        std::list<ProgressTaskInfoPtr> toRemove;
        toRemove.push_back(task);
        removeTasksFromTable(toRemove);
    }
    
    task.reset();
    
    refreshButtonsEnabledNess();
    
}

void
ProgressPanel::endTask(const NodePtr& node)
{
    if (!node) {
        return;
    }
    bool isMainThread = QThread::currentThread() == qApp->thread();
    if (isMainThread) {
        doProgressEndOnMainThread(node);
    } else {
        Q_EMIT s_doProgressEndOnMainThread(node);
    }
}

bool
ProgressPanel::isRemoveTasksAfterFinishChecked() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->removeTasksAfterFinishCheckbox->isChecked();
}


void
ProgressPanelPrivate::refreshButtonsEnableness(const std::list<ProgressTaskInfoPtr>& selection)
{
    int nActiveTasksCanPause = 0;
    int nCanceledTasksCanPause = 0;
    int nActiveTasks = 0;
    for (std::list<ProgressTaskInfoPtr>::const_iterator it = selection.begin(); it!=selection.end(); ++it) {
        bool canceled = (*it)->wasCanceled();
        if ((*it)->canPause()) {
            if (canceled) {
                ++nCanceledTasksCanPause;
            } else {
                ++nActiveTasksCanPause;
            }
        }
        if (!canceled) {
            ++nActiveTasks;
        }
    }

    cancelTasksButton->setEnabled(selection.size() > 0);
    pauseTasksButton->setEnabled(nActiveTasksCanPause > 0);
    restartTasksButton->setEnabled(nCanceledTasksCanPause > 0);
}

void
ProgressPanel::onRenderQueuingSettingChanged(bool queueingEnabled)
{
    _imp->queueTasksCheckbox->blockSignals(true);
    _imp->queueTasksCheckbox->setChecked(queueingEnabled);
    _imp->queueTasksCheckbox->blockSignals(false);
}

void
ProgressPanel::onQueueRendersCheckboxChecked()
{
    appPTR->getCurrentSettings()->setRenderQueuingEnabled(_imp->queueTasksCheckbox->isChecked());
}

NATRON_NAMESPACE_EXIT;


NATRON_NAMESPACE_USING;
#include "moc_ProgressPanel.cpp"