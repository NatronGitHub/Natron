/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#include "ProgressPanel.h"

#include <map>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QMutex>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QItemSelectionModel>
#include <QTextEdit>
#include <QApplication>
#include <QtCore/QThread>
#include <QHeaderView>
#include <QCheckBox>
#include <QCursor>
#include <QtCore/QTimer>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif
#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/Timer.h"
#include "Engine/OutputEffectInstance.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Image.h"
#include "Engine/ProcessHandler.h"
#include "Engine/Settings.h"
#include "Engine/Utils.h" // convertFromPlainText

#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/Label.h"
#include "Gui/ProgressTaskInfo.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/Button.h"
#include "Gui/Menu.h"
#include "Gui/LogWindow.h"
#include "Gui/NodeGui.h"
#include "Gui/TableModelView.h"


#define COL_TIME_REMAINING 4
#define COL_LAST 7

#define NATRON_DISPLAY_PROGRESS_PANEL_AFTER_MS 3000

NATRON_NAMESPACE_ENTER

typedef std::map<NodeWPtr, ProgressTaskInfoPtr> TasksMap;
typedef std::vector<ProgressTaskInfoPtr> TasksOrdered;
struct ProgressPanelPrivate
{
    QVBoxLayout* mainLayout;
    QWidget* headerContainer;
    QHBoxLayout* headerLayout;
    QCheckBox* queueTasksCheckbox;
    QCheckBox* removeTasksAfterFinishCheckbox;
    TableModel* model;
    TableView* view;
    mutable QMutex tasksMutex;
    TasksMap tasks;
    TasksOrdered tasksOrdered;
    ProgressTaskInfoPtr lastTaskAdded;

    // Prevents call to processEvents() recursively on the main-thread
    int processEventsRecursionCounter;

    ProgressPanelPrivate()
        : mainLayout(0)
        , headerContainer(0)
        , headerLayout(0)
        , queueTasksCheckbox(0)
        , removeTasksAfterFinishCheckbox(0)
        , model(0)
        , view(0)
        , tasksMutex()
        , tasks()
        , tasksOrdered()
        , lastTaskAdded()
        , processEventsRecursionCounter(0)
    {
    }

    ProgressTaskInfoPtr findTask(const NodePtr& node) const
    {
        assert( !tasksMutex.tryLock() );

        for (TasksMap::const_iterator it = tasks.begin(); it != tasks.end(); ++it) {
            if (it->first.lock() == node) {
                return it->second;
            }
        }

        return ProgressTaskInfoPtr();
    }

    ProgressTaskInfoPtr findTask(const TableItem* item) const
    {
        assert( !tasksMutex.tryLock() );

        for (TasksMap::const_iterator it = tasks.begin(); it != tasks.end(); ++it) {
            std::vector<TableItem*> items;
            it->second->getTableItems(&items);
            for (std::size_t i = 0; i < items.size(); ++i) {
                if (items[i] == item) {
                    return it->second;
                }
            }
        }

        return ProgressTaskInfoPtr();
    }
};

ProgressPanel::ProgressPanel(Gui* gui)
    : QWidget(gui)
    , PanelWidget(this, gui)
    , _imp( new ProgressPanelPrivate() )
{
    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    _imp->mainLayout->setSpacing(0);
    _imp->headerContainer = new QWidget(this);
    _imp->headerLayout = new QHBoxLayout(_imp->headerContainer);
    _imp->headerLayout->setSpacing(0);
    // _imp->headerLayout->setContentsMargins(0, 0, 0, 0);
    _imp->mainLayout->addWidget(_imp->headerContainer);


    _imp->queueTasksCheckbox = new QCheckBox(tr("Queue Renders"), _imp->headerContainer);
    _imp->queueTasksCheckbox->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("When checked, renders will be queued in the Progress Panel and will start only when all other prior renders are done. This does not apply to other tasks such as Tracking or analysis."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->queueTasksCheckbox->setChecked( appPTR->getCurrentSettings()->isRenderQueuingEnabled() );
    QObject::connect( _imp->queueTasksCheckbox, SIGNAL(stateChanged(int)), this, SLOT(onQueueRendersCheckboxChecked()) );
    _imp->headerLayout->addWidget(_imp->queueTasksCheckbox);

    _imp->headerLayout->addSpacing( TO_DPIX(20) );

    _imp->removeTasksAfterFinishCheckbox = new QCheckBox(tr("Remove Finished Tasks"), _imp->headerContainer);
    _imp->removeTasksAfterFinishCheckbox->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("When checked, finished tasks that can be paused"  " will be automatically removed from the task list when they are finished. When unchecked, the tasks may be restarted."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->removeTasksAfterFinishCheckbox->setChecked(false);
    _imp->headerLayout->addWidget(_imp->removeTasksAfterFinishCheckbox);


    _imp->headerLayout->addStretch();


    _imp->view = new TableView(this);
    _imp->view->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    _imp->model = new TableModel(0, 0, _imp->view);
    _imp->view->setSortingEnabled(false);
    _imp->view->setTableModel(_imp->model);

    _imp->mainLayout->addWidget(_imp->view);

    QObject::connect( _imp->view, SIGNAL(itemRightClicked(TableItem*)), this, SLOT(onItemRightClicked(TableItem*)) );
    QStringList dimensionNames;
    dimensionNames
        << tr("Node")
        << tr("Progress")
        << tr("Status")
        << tr("Controls")
        << tr("Time remaining")
        << tr("Frame Range")
        << tr("Task");

    _imp->view->setColumnCount( dimensionNames.size() );
    _imp->view->setHorizontalHeaderLabels(dimensionNames);
    //_imp->view->header()->setResizeMode(QHeaderView::Fixed);
    _imp->view->header()->setStretchLastSection(true);
    _imp->view->header()->resizeSection( COL_TIME_REMAINING, TO_DPIX(150) );

    QItemSelectionModel* selModel = _imp->view->selectionModel();
    QObject::connect( selModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(onSelectionChanged(QItemSelection,QItemSelection)) );
    QObject::connect( this, SIGNAL(s_doProgressStartOnMainThread(NodePtr,QString,QString,bool)), this, SLOT(doProgressStartOnMainThread(NodePtr,QString,QString,bool)) );
    QObject::connect( this, SIGNAL(s_doProgressUpdateOnMainThread(ProgressTaskInfoPtr,double)), this, SLOT(doProgressOnMainThread(ProgressTaskInfoPtr,double)) );
    QObject::connect( this, SIGNAL(s_doProgressEndOnMainThread(NodePtr)), this, SLOT(doProgressEndOnMainThread(NodePtr)) );
}

ProgressPanel::~ProgressPanel()
{
}

void
ProgressPanel::onShowProgressPanelTimerTriggered()
{
    if (!_imp->lastTaskAdded) {
        return;
    }

    ProgressTaskInfo::ProgressTaskStatusEnum status = _imp->lastTaskAdded->getStatus();
    if ( (status == ProgressTaskInfo::eProgressTaskStatusCanceled) ||
         ( status == ProgressTaskInfo::eProgressTaskStatusFinished) ) {
        _imp->lastTaskAdded.reset();

        return;
    }
    for (TasksOrdered::iterator it = _imp->tasksOrdered.begin(); it != _imp->tasksOrdered.end(); ++it) {
        if ( (*it) == _imp->lastTaskAdded ) {
            _imp->lastTaskAdded->onShowProgressPanelTimerTimeout();
            _imp->lastTaskAdded.reset();

            return;
        }
    }
}

void
ProgressPanel::onSelectionChanged(const QItemSelection& /*selected*/,
                                  const QItemSelection& /*deselected*/)
{
}

void
ProgressPanel::getSelectedTaskInternal(const QItemSelection& selected,
                                       std::list<ProgressTaskInfoPtr>& selection) const
{
    std::set<int> rows;
    QModelIndexList selectedIndex = selected.indexes();

    for (int i = 0; i < selectedIndex.size(); ++i) {
        rows.insert( selectedIndex[i].row() );
    }
    for (std::set<int>::iterator it = rows.begin(); it != rows.end(); ++it) {
        if ( ( (*it) >= 0 ) && ( (*it) < (int)_imp->tasksOrdered.size() ) ) {
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

    if ( (key == Qt::Key_Delete) || (key == Qt::Key_Backspace) ) {
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
ProgressPanel::enterEvent(QEvent* e)
{
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
ProgressPanel::onCancelTasksTriggered()
{
    std::list<ProgressTaskInfoPtr> selection;

    getSelectedTask(selection);
    for (std::list<ProgressTaskInfoPtr>::iterator it = selection.begin(); it != selection.end(); ++it) {
        (*it)->cancelTask(false, 0);
    }

    removeTasksFromTable(selection);
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
        for (std::list<ProgressTaskInfoPtr>::const_iterator it = tasks.begin(); it != tasks.end(); ++it) {
            TasksMap::iterator foundInMap = _imp->tasks.find( (*it)->getNode() );
            if ( foundInMap != _imp->tasks.end() ) {
                (*it)->clearItems();
                _imp->tasks.erase(foundInMap);
            }
        }
        int rc = _imp->view->rowCount();
        int cc = _imp->view->columnCount();
        assert( (int)_imp->tasksOrdered.size() == rc );

        for (int i = 0; i < rc; ++i) {
            std::list<ProgressTaskInfoPtr>::const_iterator foundSelected = std::find(tasks.begin(), tasks.end(), _imp->tasksOrdered[i]);
            _imp->tasksOrdered[i]->removeCellWidgets(i, _imp->view);
            if ( foundSelected != tasks.end() ) {
                continue;
            }
            for (int j = 0; j < cc; ++j) {
                table.push_back( _imp->view->takeItem(i, j) );
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

static void
connectProcessSlots(ProgressTaskInfo* task,
                    ProcessHandler* process)
{
    QObject::connect( task, SIGNAL(taskCanceled()), process, SLOT(onProcessCanceled()) );
    QObject::connect( process, SIGNAL(processCanceled()), task, SLOT(onProcessCanceled()) );
    QObject::connect( process, SIGNAL(frameRendered(int,double)), task, SLOT(onRenderEngineFrameComputed(int,double)) );
    QObject::connect( process, SIGNAL(processFinished(int)), task, SLOT(onRenderEngineStopped(int)) );
}

void
ProgressPanel::onTaskRestarted(const NodePtr& node,
                               const ProcessHandlerPtr& process)
{
    QMutexLocker k(&_imp->tasksMutex);
    ProgressTaskInfoPtr task;

    task = _imp->findTask(node);
    if (!task) {
        return;
    }
    //The process may have changed
    if (process) {
        task->setProcesshandler(process);
        connectProcessSlots( task.get(), process.get() );
    }
}

void
ProgressPanel::doProgressStartOnMainThread(const NodePtr& node,
                                           const QString &message,
                                           const QString & /*messageid*/,
                                           bool canCancel)
{
    startTask(node, INT_MIN, INT_MAX, 1, false, canCancel, message);
}

void
ProgressPanel::progressStart(const NodePtr& node,
                             const std::string &message,
                             const std::string &messageid,
                             bool canCancel)
{
    if (!node) {
        return;
    }
    bool isMainThread = QThread::currentThread() == qApp->thread();
    if (isMainThread) {
        doProgressStartOnMainThread(node, QString::fromUtf8( message.c_str() ), QString::fromUtf8( messageid.c_str() ), canCancel);
    } else {
        Q_EMIT s_doProgressStartOnMainThread(node, QString::fromUtf8( message.c_str() ), QString::fromUtf8( messageid.c_str() ), canCancel);
    }
}

void
ProgressPanel::startTask(const NodePtr& node,
                         const int firstFrame,
                         const int lastFrame,
                         const int frameStep,
                         const bool canPause,
                         const bool canCancel,
                         const QString& message,
                         const ProcessHandlerPtr& process)
{
    assert( QThread::currentThread() == qApp->thread() );
    if (!node) {
        return;
    }
    assert( (canPause && firstFrame != INT_MIN && lastFrame != INT_MAX) || !canPause );

    ProgressTaskInfoPtr task;
    {
        QMutexLocker k(&_imp->tasksMutex);
        task = _imp->findTask(node);
    }
    if (task) {
        task->cancelTask(false, 1);
        removeTaskFromTable(task);
    }


    QMutexLocker k(&_imp->tasksMutex);

    task.reset( new ProgressTaskInfo(this,
                                     node,
                                     firstFrame,
                                     lastFrame,
                                     frameStep,
                                     canPause,
                                     canCancel,
                                     message, process) );


    if ( canPause || node->getEffectInstance()->isOutput() ) {
        task->createItems();
        QTimer::singleShot( NATRON_DISPLAY_PROGRESS_PANEL_AFTER_MS, this, SLOT(onShowProgressPanelTimerTriggered()) );
    }

    if (process) {
        connectProcessSlots( task.get(), process.get() );
    }
    if (!process) {
        if ( node->getEffectInstance()->isOutput() ) {
            OutputEffectInstance* isOutput = dynamic_cast<OutputEffectInstance*>( node->getEffectInstance().get() );
            if (isOutput) {

                if ( getGui() && !getGui()->isGUIFrozen() && appPTR->getCurrentSettings()->isAutoTurboEnabled() ) {
                    getGui()->onFreezeUIButtonClicked(true);
                }

                RenderEnginePtr engine = isOutput->getRenderEngine();
                assert(engine);
                QObject::connect( engine.get(), SIGNAL(frameRendered(int,double)), task.get(), SLOT(onRenderEngineFrameComputed(int,double)) );
                QObject::connect( engine.get(), SIGNAL(renderFinished(int)), task.get(), SLOT(onRenderEngineStopped(int)) );
                QObject::connect( task.get(), SIGNAL(taskCanceled()), engine.get(), SLOT(abortRendering_non_blocking()) );
            }
        }
    }
    _imp->tasks[node] = task;
} // ProgressPanel::startTask

void
ProgressPanel::onLastTaskAddedFinished(const ProgressTaskInfo* task)
{
    if ( task == _imp->lastTaskAdded.get() ) {
        _imp->lastTaskAdded.reset();
    }
}

void
ProgressPanel::addTaskToTable(const ProgressTaskInfoPtr& task)
{
    assert( QThread::currentThread() == qApp->thread() );
    int rc = _imp->view->rowCount();
    _imp->view->setRowCount(rc + 1);

    std::vector<TableItem*> items;
    task->getTableItems(&items);
    assert(items.size() == COL_LAST);

    for (std::size_t i = 0; i < items.size(); ++i) {
        _imp->view->setItem(rc, i, items[i]);
    }
    task->setCellWidgets(rc, _imp->view);

    _imp->lastTaskAdded = task;
    _imp->tasksOrdered.push_back(task);
}

void
ProgressPanel::doProgressOnMainThread(const ProgressTaskInfoPtr& task,
                                      double progress)
{
    task->updateProgressBar(progress, progress);
}

bool
ProgressPanel::updateTask(const NodePtr& node,
                          const double progress)
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

    if ( foundTask->wasCanceled() ) {
        return false;
    }

    if (isMainThread) {
        if (_imp->processEventsRecursionCounter == 0) {
            ++_imp->processEventsRecursionCounter;
            {
#ifdef DEBUG
                boost_adaptbx::floating_point::exception_trapping trap(0);
#endif
                QCoreApplication::processEvents();
            }
            --_imp->processEventsRecursionCounter;
        }
    }

    return true;
}

void
ProgressPanel::doProgressEndOnMainThread(const NodePtr& node)
{
    ProgressTaskInfoPtr task;
    {
        QMutexLocker k(&_imp->tasksMutex);
        task = _imp->findTask(node);
    }

    if (!task) {
        return;
    }
    task->cancelTask(false, 0);
    task.reset();
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
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->removeTasksAfterFinishCheckbox->isChecked();
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
    appPTR->getCurrentSettings()->setRenderQueuingEnabled( _imp->queueTasksCheckbox->isChecked() );
}

void
ProgressPanel::onItemRightClicked(TableItem* item)
{
    ProgressTaskInfoPtr task;
    {
        QMutexLocker k(&_imp->tasksMutex);
        task = _imp->findTask(item);
    }

    if (!task) {
        return;
    }
    ProcessHandlerPtr hasProcess = task->getProcess();
    Menu m(this);
    QAction* showLogAction = 0;
    if (hasProcess) {
        showLogAction = new QAction(tr("Show Process Log"), &m);
        m.addAction(showLogAction);
    }

    QAction* triggered = 0;
    if ( !m.isEmpty() ) {
        triggered = m.exec( QCursor::pos() );
    }
    if ( (triggered == showLogAction) && showLogAction ) {
        const QString& log = hasProcess->getProcessLog();
        LogWindowModal window(log,this);
        window.setWindowTitle( tr("Background Render Log") );
        ignore_result(window.exec());
    }
}

NATRON_NAMESPACE_EXIT


NATRON_NAMESPACE_USING
#include "moc_ProgressPanel.cpp"
