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
#include "Gui/Button.h"
#include "Gui/Utils.h"
#include "Gui/NodeGui.h"
#include "Gui/TableModelView.h"


#define NATRON_SHOW_PROGRESS_TOTAL_ESTIMATED_TIME_MS 4000
#define NATRON_PROGRESS_DIALOG_ETA_REFRESH_MS 1000

#define COL_NAME 0
#define COL_PROGRESS 1
#define COL_STATUS 2
#define COL_CAN_PAUSE 3
#define COL_TIME_REMAINING 4
#define COL_TASK 5

NATRON_NAMESPACE_ENTER;

struct TaskInfoPrivate;
class TaskInfo : public QObject, public boost::enable_shared_from_this<TaskInfo>
{

    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON

    friend class ProgressPanel;

public:

    TaskInfo(ProgressPanel* panel,
             const NodePtr& node,
             const int firstFrame,
             const int lastFrame,
             const int frameStep,
             const bool canPause,
             const bool canCancel,
             const QString& message,
             const boost::shared_ptr<ProcessHandler>& process);

    virtual ~TaskInfo();

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

Q_SIGNALS:

    void taskCanceled();

private:

    void clearItems();

    boost::scoped_ptr<TaskInfoPrivate> _imp;
};

typedef std::map<NodeWPtr, TaskInfoPtr> TasksMap;
typedef std::vector<TaskInfoPtr> TasksOrdered;


namespace {
    class MetaTypesRegistration
    {
    public:
        inline MetaTypesRegistration()
        {
            qRegisterMetaType<TaskInfoPtr>("TaskInfoPtr");
        }
    };
}
static MetaTypesRegistration registration;

struct TaskInfoPrivate {
    
    ProgressPanel* panel;
    
    NodeWPtr node;
    
    TaskInfo* _publicInterface;
    
    TableItem* nameItem;
    TableItem* progressItem;
    TableItem* statusItem;
    TableItem* canPauseItem;
    TableItem* timeRemainingItem;
    TableItem* taskInfoItem;
    
    QProgressBar* progressBar;
    
    double timeRemaining;
    
    bool canBePaused;
    
    bool canceled;
    
    bool canCancel;
    
    bool updatedProgressOnce;
    
    int firstFrame,lastFrame,frameStep, lastRenderedFrame, nFramesRendered;
    
    boost::scoped_ptr<TimeLapse> timer;
    
    boost::scoped_ptr<QTimer> refreshLabelTimer;
    
    QString message;
    
    boost::shared_ptr<ProcessHandler> process;
    
    TaskInfoPrivate(ProgressPanel* panel,
                    const NodePtr& node,
                    TaskInfo* publicInterface,
                    const int firstFrame,
                    const int lastFrame,
                    const int frameStep,
                    const bool canPause,
                    const bool canCancel,
                    const QString& message,
                    const boost::shared_ptr<ProcessHandler>& process)
    : panel(panel)
    , node(node)
    , _publicInterface(publicInterface)
    , nameItem(0)
    , progressItem(0)
    , statusItem(0)
    , canPauseItem(0)
    , timeRemainingItem(0)
    , taskInfoItem(0)
    , progressBar(0)
    , timeRemaining(-1)
    , canBePaused(canPause)
    , canceled(false)
    , canCancel(canCancel)
    , updatedProgressOnce(false)
    , firstFrame(firstFrame)
    , lastFrame(lastFrame)
    , frameStep(frameStep)
    , lastRenderedFrame(-1)
    , nFramesRendered(0)
    , timer()
    , refreshLabelTimer()
    , message(message)
    , process(process)
    {
        
    }
    
    NodePtr getNode() const
    {
        return node.lock();
    }
    
    void createItems();

};


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
    
    TaskInfoPtr findTask(const NodePtr& node) const
    {
        assert(!tasksMutex.tryLock());
        
        for (TasksMap::const_iterator it = tasks.begin(); it!=tasks.end(); ++it) {
            if (it->first.lock() == node) {
                return it->second;
            }
        }
        return TaskInfoPtr();
    }
    
    
    void refreshButtonsEnableness(const std::list<TaskInfoPtr>& selection);
};


TaskInfo::TaskInfo(ProgressPanel* panel,
                   const NodePtr& node,
                   const int firstFrame,
                   const int lastFrame,
                   const int frameStep,
                   const bool canPause,
                   const bool canCancel,
                   const QString& message,
                   const boost::shared_ptr<ProcessHandler>& process)
: QObject()
, _imp(new TaskInfoPrivate(panel,node, this, firstFrame, lastFrame, frameStep, canPause, canCancel, message, process))
{
    //We compute the time remaining automatically based on a timer if this is not a render but a general progress dialog
    _imp->timer.reset(new TimeLapse);
    _imp->refreshLabelTimer.reset(new QTimer);
    QObject::connect(_imp->refreshLabelTimer.get(), SIGNAL(timeout()), this, SLOT(onRefreshLabelTimeout()));
    _imp->refreshLabelTimer->start(NATRON_PROGRESS_DIALOG_ETA_REFRESH_MS);
    
}

TaskInfo::~TaskInfo()
{
    
}

void
TaskInfo::cancelTask(bool calledFromRenderEngine, int retCode)
{
    if (_imp->refreshLabelTimer) {
        _imp->refreshLabelTimer->stop();
    }
    if (_imp->canceled) {
        return;
    }
    _imp->canceled = true;
    if (_imp->timeRemainingItem) {
        _imp->timeRemainingItem->setText(tr("N/A"));
    }
    if (_imp->statusItem) {
        if (!calledFromRenderEngine) {
            _imp->statusItem->setTextColor(QColor(243,147,0));
            _imp->statusItem->setText(_imp->canBePaused ? tr("Paused") : tr("Canceled"));
        } else {
            if (retCode == 0) {
                _imp->statusItem->setTextColor(Qt::black);
                _imp->statusItem->setText(tr("Finished"));
            } else {
                _imp->statusItem->setTextColor(Qt::red);
                _imp->statusItem->setText(tr("Failed"));
            }
            
        }
    }
    NodePtr node = getNode();
    OutputEffectInstance* effect = dynamic_cast<OutputEffectInstance*>(node->getEffectInstance().get());
    node->getApp()->removeRenderFromQueue(effect);
    if (!_imp->canBePaused) {
        _imp->panel->removeTaskFromTable(shared_from_this());
    }
    if (!calledFromRenderEngine) {
        Q_EMIT taskCanceled();
    }
}

void
TaskInfo::restartTask()
{
    if (!_imp->canBePaused) {
        return;
    }
    _imp->canceled = false;
    _imp->updatedProgressOnce = false;
    _imp->timer.reset(new TimeLapse);
    _imp->refreshLabelTimer->start(NATRON_PROGRESS_DIALOG_ETA_REFRESH_MS);
    
    NodePtr node = _imp->getNode();
    if (node->getEffectInstance()->isWriter()) {
        int firstFrame;
        if (_imp->lastRenderedFrame == _imp->lastFrame || _imp->lastRenderedFrame == -1) {
            firstFrame = _imp->firstFrame;
            _imp->nFramesRendered = 0;
        } else {
            firstFrame =  _imp->lastRenderedFrame;
        }
        AppInstance::RenderWork w(dynamic_cast<OutputEffectInstance*>(node->getEffectInstance().get()),
                                  firstFrame,
                                  _imp->lastFrame,
                                  _imp->frameStep,
                                  node->getApp()->isRenderStatsActionChecked());
        w.isRestart = true;
        _imp->statusItem->setTextColor(Qt::yellow);
        _imp->statusItem->setText(tr("Queued"));
        std::list<AppInstance::RenderWork> works;
        works.push_back(w);
        node->getApp()->startWritersRendering(false, works);

    }
}


void
TaskInfo::onRenderEngineFrameComputed(int frame,  double progress)
{
    RenderEngine* r = qobject_cast<RenderEngine*>(sender());
    ProcessHandler* process = qobject_cast<ProcessHandler*>(sender());
    if (!r && !process) {
        return;
    }
    OutputEffectInstance* output = r ? r->getOutput().get() : process->getWriter();
    if (!output) {
        return;
    }
    updateProgress(frame, progress);

}

void
TaskInfo::onRenderEngineStopped(int retCode)
{
    RenderEngine* r = qobject_cast<RenderEngine*>(sender());
    ProcessHandler* process = qobject_cast<ProcessHandler*>(sender());
    if (!r && !process) {
        return;
    }
    OutputEffectInstance* output = r ? r->getOutput().get() : process->getWriter();
    if (!output) {
        return;
    }
    
    if ((_imp->panel->isRemoveTasksAfterFinishChecked() && retCode == 0) || !canPause()) {
        std::list<TaskInfoPtr> toRemove;
        toRemove.push_back(shared_from_this());
        _imp->panel->removeTasksFromTable(toRemove);
    } else {
        cancelTask(true, retCode);
    }
  
    _imp->panel->refreshButtonsEnabledNess();
}

void
TaskInfo::onProcessCanceled()
{
    onRenderEngineStopped(1);
}

void
TaskInfo::clearItems()
{
    _imp->nameItem = 0;
    _imp->progressItem = 0;
    _imp->progressBar = 0;
    _imp->canPauseItem = 0;
    _imp->timeRemainingItem = 0;
    _imp->taskInfoItem = 0;
    _imp->statusItem = 0;
}



NodePtr
TaskInfo::getNode() const
{
    return _imp->node.lock();
}

bool
TaskInfo::wasCanceled() const
{
    return _imp->canceled;
}

bool
TaskInfo::canPause() const
{
    return _imp->canBePaused;
}

void
TaskInfo::onRefreshLabelTimeout()
{
    if (!_imp->timeRemainingItem) {
        if (_imp->timer->getTimeSinceCreation() * 1000 > NATRON_SHOW_PROGRESS_TOTAL_ESTIMATED_TIME_MS) {
            _imp->createItems();
        }
        return;
    }
    QString timeStr;
    if (_imp->timeRemaining == -1) {
        timeStr += tr("N/A");
    } else {
        timeStr += Timer::printAsTime(_imp->timeRemaining, true);
    }
    _imp->timeRemainingItem->setText(timeStr);
}

void
TaskInfoPrivate::createItems()
{
    NodePtr node = getNode();
    boost::shared_ptr<NodeGuiI> gui_i = node->getNodeGui();
    NodeGui* nodeUI = dynamic_cast<NodeGui*>(gui_i.get());
    
    QColor color;
    if (nodeUI) {
        double r,g,b;
        nodeUI->getColor(&r, &g, &b);
        color.setRgbF(Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.));
    }
    {
        TableItem* item = new TableItem;
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        assert(item);
        if (nodeUI) {
            item->setTextColor(Qt::black);
            item->setBackgroundColor(color);
        }
        item->setText(node->getLabel().c_str());
        nameItem = item;
    }
    {
        TableItem* item = new TableItem;
        progressBar = new QProgressBar;
        //We must call this otherwise this is never called by Qt for custom widgets (this is a Qt bug)
        {
            QSize s = progressBar->minimumSizeHint();
            Q_UNUSED(s);
        }
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        assert(item);
        progressItem = item;
    }
    {
        TableItem* item = new TableItem;
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        assert(item);
        item->setTextColor(Qt::yellow);
        item->setText(QObject::tr("Queued"));
        statusItem = item;
    }
    {
        TableItem* item = new TableItem;
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        assert(item);
        if (nodeUI) {
            item->setIcon(QIcon());
        }
        item->setText(canBePaused ? "Yes":"No");
        canPauseItem = item;
    }
    {
        TableItem* item = new TableItem;
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        assert(item);
        item->setText(QObject::tr("N/A"));
        timeRemainingItem = item;
    }
    
    {
        TableItem* item = new TableItem;
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        assert(item);
        item->setText(message);
        taskInfoItem = item;
    }
    panel->addTaskToTable(_publicInterface->shared_from_this());
}

void
TaskInfo::updateProgressBar(double totalProgress,double subTaskProgress)
{
    assert(QThread::currentThread() == qApp->thread());
    totalProgress = std::max(std::min(totalProgress, 1.), 0.);
    subTaskProgress = std::max(std::min(subTaskProgress, 1.), 0.);
    if (_imp->progressBar) {
        _imp->progressBar->setValue(totalProgress * 100.);
    }
    
    double timeElapsedSecs = _imp->timer->getTimeSinceCreation();
    _imp->timeRemaining = subTaskProgress == 0 ? 0 : timeElapsedSecs * (1. - subTaskProgress) / subTaskProgress;
    
    if (!_imp->nameItem && !wasCanceled()) {
        
        ///Show the item if the total estimated time is gt NATRON_SHOW_PROGRESS_TOTAL_ESTIMATED_TIME_MS
        double totalTime = subTaskProgress == 0 ? 0 : timeElapsedSecs * 1. / subTaskProgress;
        //also,  don't show if it was not shown yet but there are less than NATRON_SHOW_PROGRESS_TOTAL_ESTIMATED_TIME_MS remaining
        if (std::min(_imp->timeRemaining, totalTime) * 1000 > NATRON_SHOW_PROGRESS_TOTAL_ESTIMATED_TIME_MS) {
            _imp->createItems();
        }
    }
    
    if (!_imp->updatedProgressOnce && _imp->statusItem) {
        _imp->updatedProgressOnce = true;
        _imp->statusItem->setTextColor(Qt::green);
        _imp->statusItem->setText(tr("Running"));
    }
}

void
TaskInfo::updateProgress(const int frame, double progress)
{

    ++_imp->nFramesRendered;
    U64 totalFrames = (double)(_imp->lastFrame - _imp->firstFrame + 1) / _imp->frameStep;
    double percent = 0;
    if (totalFrames > 0) {
        percent = _imp->nFramesRendered / (double)totalFrames;
    }
    _imp->lastRenderedFrame = frame;
    updateProgressBar(percent, progress);
    
}

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
    std::list<TaskInfoPtr> selection;
    getSelectedTaskInternal(selected, selection);
    _imp->refreshButtonsEnableness(selection);
}

void
ProgressPanel::getSelectedTaskInternal(const QItemSelection& selected, std::list<TaskInfoPtr>& selection) const
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
ProgressPanel::getSelectedTask(std::list<TaskInfoPtr>& selection) const
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
    std::list<TaskInfoPtr> selection;
    getSelectedTask(selection);
    for (std::list<TaskInfoPtr>::iterator it = selection.begin(); it!=selection.end(); ++it) {
        if ((*it)->canPause()) {
            (*it)->cancelTask(false, 0);
        }
    }
    
    _imp->refreshButtonsEnableness(selection);
}

void
ProgressPanel::onCancelTasksTriggered()
{
    std::list<TaskInfoPtr> selection;
    getSelectedTask(selection);
    for (std::list<TaskInfoPtr>::iterator it = selection.begin(); it!=selection.end(); ++it) {
        (*it)->cancelTask(false, 0);
    }
    
    removeTasksFromTable(selection);
    
    getSelectedTask(selection);
    _imp->refreshButtonsEnableness(selection);
    

}

void
ProgressPanel::removeTaskFromTable(const TaskInfoPtr& task)
{
    std::list<TaskInfoPtr> list;
    list.push_back(task);
    removeTasksFromTable(list);
}

void
ProgressPanel::removeTasksFromTable(const std::list<TaskInfoPtr>& tasks)
{
    std::vector<TableItem*> table;
    std::vector<TaskInfoPtr> newOrder;
    
    {
        QMutexLocker k(&_imp->tasksMutex);
        
        int rc = _imp->view->rowCount();
        int cc = _imp->view->columnCount();
        assert((int)_imp->tasksOrdered.size() == rc);
        
        for (int i = 0; i < rc; ++i) {
            std::list<TaskInfoPtr>::const_iterator foundSelected = std::find(tasks.begin(), tasks.end(), _imp->tasksOrdered[i]);
            _imp->view->removeCellWidget(i, COL_PROGRESS);
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
        _imp->tasksOrdered[i]->_imp->progressBar = new QProgressBar;
        //We must call this otherwise this is never called by Qt for custom widgets (this is a Qt bug)
        {
            QSize s = _imp->tasksOrdered[i]->_imp->progressBar->minimumSizeHint();
            Q_UNUSED(s);
        }
        _imp->view->setCellWidget(i, COL_PROGRESS, _imp->tasksOrdered[i]->_imp->progressBar);
    }
}

void
ProgressPanel::onRestartAllTasksTriggered()
{
    std::list<TaskInfoPtr> selection;
    getSelectedTask(selection);
    for (std::list<TaskInfoPtr>::iterator it = selection.begin(); it!=selection.end(); ++it) {
        if ((*it)->wasCanceled() && (*it)->canPause())
            (*it)->restartTask();
    }
    _imp->refreshButtonsEnableness(selection);
}

void
ProgressPanel::refreshButtonsEnabledNess()
{
    std::list<TaskInfoPtr> selection;
    getSelectedTask(selection);
    _imp->refreshButtonsEnableness(selection);
}

static void connectProcessSlots(TaskInfo* task, ProcessHandler* process)
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
    TaskInfoPtr task;
    task = _imp->findTask(node);
    if (!task) {
        return;
    }
    //The process may have changed
    task->_imp->process = process;
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
    
    TaskInfoPtr task;
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
    
    task.reset(new TaskInfo(this,
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
ProgressPanel::addTaskToTable(const TaskInfoPtr& task)
{
    assert(QThread::currentThread() == qApp->thread());
    int rc = _imp->view->rowCount();
    _imp->view->setRowCount(rc + 1);
    
    _imp->view->setItem(rc, COL_NAME, task->_imp->nameItem);
    _imp->view->setItem(rc, COL_PROGRESS, task->_imp->progressItem);
    _imp->view->setCellWidget(rc, COL_PROGRESS, task->_imp->progressBar);
    _imp->view->setItem(rc, COL_STATUS, task->_imp->statusItem);
    _imp->view->setItem(rc, COL_CAN_PAUSE, task->_imp->canPauseItem);
    _imp->view->setItem(rc, COL_TIME_REMAINING, task->_imp->timeRemainingItem);
    _imp->view->setItem(rc, COL_TASK, task->_imp->taskInfoItem);
    
    _imp->tasksOrdered.push_back(task);
    
    getGui()->ensureProgressPanelVisible();
    
    refreshButtonsEnabledNess();
}

void
ProgressPanel::doProgressOnMainThread(const TaskInfoPtr& task, double progress)
{
    task->updateProgressBar(progress, progress);
}

bool
ProgressPanel::updateTask(const NodePtr& node, const double progress)
{
    
    bool isMainThread = QThread::currentThread() == qApp->thread();
    TaskInfoPtr foundTask;
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
    TaskInfoPtr task;
    {
        QMutexLocker k(&_imp->tasksMutex);
        task= _imp->findTask(node);
    }
    
    if (!task) {
        return;
    }
    
    {
        std::list<TaskInfoPtr> toRemove;
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
ProgressPanelPrivate::refreshButtonsEnableness(const std::list<TaskInfoPtr>& selection)
{
    int nActiveTasksCanPause = 0;
    int nCanceledTasksCanPause = 0;
    int nActiveTasks = 0;
    for (std::list<TaskInfoPtr>::const_iterator it = selection.begin(); it!=selection.end(); ++it) {
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