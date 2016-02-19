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

#include "ProgressTaskInfo.h"

#include <QTimer>
#include <QProgressBar>
#include <QApplication>
#include <QThread>
#include <QProgressBar>
#include <QHBoxLayout>

#include "Engine/AppInstance.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/OutputEffectInstance.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/ProcessHandler.h"
#include "Engine/Timer.h"

#include "Gui/NodeGui.h"
#include "Gui/ProgressPanel.h"
#include "Gui/TableModelView.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Button.h"
#include "Gui/Utils.h"

#define COL_PROGRESS 1
#define COL_CONTROLS 3

#define NATRON_SHOW_PROGRESS_TOTAL_ESTIMATED_TIME_MS 4000
#define NATRON_PROGRESS_DIALOG_ETA_REFRESH_MS 1000

NATRON_NAMESPACE_ENTER;

enum ProgressTaskStatusEnum
{
    eProgressTaskStatusPaused,
    eProgressTaskStatusRunning,
    eProgressTaskStatusQueued,
    eProgressTaskStatusFinished,
    eProgressTaskStatusCanceled
};

namespace {
    class MetaTypesRegistration
    {
    public:
        inline MetaTypesRegistration()
        {
            qRegisterMetaType<ProgressTaskInfoPtr>("ProgressTaskInfoPtr");
        }
    };
}
static MetaTypesRegistration registration;

struct ProgressTaskInfoPrivate {
    
    ProgressPanel* panel;
    
    NodeWPtr node;
    
    ProgressTaskInfo* _publicInterface;
    
    TableItem* nameItem;
    TableItem* progressItem;
    TableItem* statusItem;
    TableItem* controlsItem;
    TableItem* timeRemainingItem;
    TableItem* taskInfoItem;
    
    ProgressTaskStatusEnum status;
    
    QProgressBar* progressBar;
    double progressPercent;
    QWidget* controlsButtonsContainer;
    Button* restartTasksButton;
    Button* pauseTasksButton;
    Button* cancelTasksButton;
    
    double timeRemaining;
    
    bool canBePaused;
    
    mutable QMutex canceledMutex;
    bool canceled;
    
    bool canCancel;
    
    bool updatedProgressOnce;
    
    int firstFrame,lastFrame,frameStep, lastRenderedFrame, nFramesRendered;
    
    boost::scoped_ptr<TimeLapse> timer;
    
    boost::scoped_ptr<QTimer> refreshLabelTimer;
    
    QString message;
    
    boost::shared_ptr<ProcessHandler> process;
    
    ProgressTaskInfoPrivate(ProgressPanel* panel,
                    const NodePtr& node,
                    ProgressTaskInfo* publicInterface,
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
    , controlsItem(0)
    , timeRemainingItem(0)
    , taskInfoItem(0)
    , status(eProgressTaskStatusQueued)
    , progressBar(0)
    , progressPercent(0)
    , controlsButtonsContainer(0)
    , restartTasksButton(0)
    , pauseTasksButton(0)
    , cancelTasksButton(0)
    , timeRemaining(-1)
    , canBePaused(canPause)
    , canceledMutex()
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
    
    void refreshButtons();
    
};

ProgressTaskInfo::ProgressTaskInfo(ProgressPanel* panel,
                   const NodePtr& node,
                   const int firstFrame,
                   const int lastFrame,
                   const int frameStep,
                   const bool canPause,
                   const bool canCancel,
                   const QString& message,
                   const boost::shared_ptr<ProcessHandler>& process)
: QObject()
, _imp(new ProgressTaskInfoPrivate(panel,node, this, firstFrame, lastFrame, frameStep, canPause, canCancel, message, process))
{
    
    
    //We compute the time remaining automatically based on a timer if this is not a render but a general progress dialog
    _imp->timer.reset(new TimeLapse);
    _imp->refreshLabelTimer.reset(new QTimer);
    QObject::connect(_imp->refreshLabelTimer.get(), SIGNAL(timeout()), this, SLOT(onRefreshLabelTimeout()));
    _imp->refreshLabelTimer->start(NATRON_PROGRESS_DIALOG_ETA_REFRESH_MS);

    
}

ProgressTaskInfo::~ProgressTaskInfo()
{
    
}

void
ProgressTaskInfo::cancelTask(bool calledFromRenderEngine, int retCode)
{
    if (_imp->refreshLabelTimer) {
        _imp->refreshLabelTimer->stop();
    }
    {
        QMutexLocker k(&_imp->canceledMutex);
        if (_imp->canceled) {
            return;
        }
        _imp->canceled = true;
    }
    if (_imp->timeRemainingItem) {
        _imp->timeRemainingItem->setText(tr("N/A"));
    }
    if (_imp->statusItem) {
        if (!calledFromRenderEngine) {
            _imp->statusItem->setTextColor(QColor(243,147,0));
            if (_imp->canBePaused) {
                _imp->statusItem->setText(tr("Paused"));
                _imp->status = eProgressTaskStatusPaused;
            } else {
                _imp->statusItem->setText(tr("Canceled"));
                _imp->status = eProgressTaskStatusCanceled;
            }
        } else {
            if (retCode == 0) {
                _imp->statusItem->setTextColor(Qt::black);
                _imp->statusItem->setText(tr("Finished"));
                _imp->status = eProgressTaskStatusFinished;
            } else {
                _imp->statusItem->setTextColor(Qt::red);
                _imp->statusItem->setText(tr("Failed"));
                _imp->status = eProgressTaskStatusFinished;
            }
            
        }
        _imp->refreshButtons();
    }
    NodePtr node = getNode();
    OutputEffectInstance* effect = dynamic_cast<OutputEffectInstance*>(node->getEffectInstance().get());
    node->getApp()->removeRenderFromQueue(effect);
    if ((_imp->panel->isRemoveTasksAfterFinishChecked() && retCode == 0) || !_imp->canBePaused) {
        _imp->panel->removeTaskFromTable(shared_from_this());
    }
    if (!calledFromRenderEngine) {
        Q_EMIT taskCanceled();
    }
}

void
ProgressTaskInfo::restartTask()
{
    if (!_imp->canBePaused) {
        return;
    }
    
    {
        QMutexLocker k(&_imp->canceledMutex);
        _imp->canceled = false;
    }
    _imp->updatedProgressOnce = false;
    _imp->timer.reset(new TimeLapse);
    _imp->refreshLabelTimer->start(NATRON_PROGRESS_DIALOG_ETA_REFRESH_MS);
    
    _imp->pauseTasksButton->setEnabled(true);
    _imp->restartTasksButton->setEnabled(false);
    
    
    NodePtr node = _imp->getNode();
    if (node->getEffectInstance()->isOutput()) {
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
        _imp->status = eProgressTaskStatusQueued;
        _imp->statusItem->setText(tr("Queued"));
        _imp->refreshButtons();
        std::list<AppInstance::RenderWork> works;
        works.push_back(w);
        node->getApp()->startWritersRendering(false, works);
        
    }
}


void
ProgressTaskInfo::onRenderEngineFrameComputed(int frame,  double progress)
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
ProgressTaskInfo::onRenderEngineStopped(int retCode)
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
    
    //Hold a shared ptr because removeTasksFromTable would remove the last ref otherwise
    ProgressTaskInfoPtr thisShared = shared_from_this();

    cancelTask(true, retCode);
    
}

void
ProgressTaskInfo::onProcessCanceled()
{
    onRenderEngineStopped(1);
}

void
ProgressTaskInfo::clearItems()
{
    _imp->nameItem = 0;
    _imp->progressItem = 0;
    _imp->progressBar = 0;
    _imp->controlsItem = 0;
    _imp->timeRemainingItem = 0;
    _imp->taskInfoItem = 0;
    _imp->statusItem = 0;
}



NodePtr
ProgressTaskInfo::getNode() const
{
    return _imp->node.lock();
}

bool
ProgressTaskInfo::wasCanceled() const
{
    QMutexLocker k(&_imp->canceledMutex);
    return _imp->canceled;
}

bool
ProgressTaskInfo::canPause() const
{
    return _imp->canBePaused;
}

void
ProgressTaskInfo::onRefreshLabelTimeout()
{
    if (!_imp->timeRemainingItem) {
        if (!_imp->canBePaused && wasCanceled()) {
            return;
        }
        if ( _imp->timer->getTimeSinceCreation() * 1000 > NATRON_SHOW_PROGRESS_TOTAL_ESTIMATED_TIME_MS) {
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
ProgressTaskInfo::createItems()
{
    _imp->createItems();
}

void
ProgressTaskInfoPrivate::createItems()
{
    if (nameItem) {
        return;
    }
    NodePtr node = getNode();
    boost::shared_ptr<NodeGuiI> gui_i = node->getNodeGui();
    NodeGui* nodeUI = dynamic_cast<NodeGui*>(gui_i.get());
    
    QColor color;
    if (nodeUI) {
        double r,g,b;
        nodeUI->getColor(&r, &g, &b);
        color.setRgbF(Image::clamp(r, 0., 1.), Image::clamp(g, 0., 1.), Image::clamp(b, 0., 1.));
    }
    _publicInterface->createCellWidgets();
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
        controlsItem = item;
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
ProgressTaskInfo::updateProgressBar(double totalProgress,double subTaskProgress)
{
    assert(QThread::currentThread() == qApp->thread());
    totalProgress = std::max(std::min(totalProgress, 1.), 0.);
    subTaskProgress = std::max(std::min(subTaskProgress, 1.), 0.);
    
    _imp->progressPercent = totalProgress * 100.;
    if (_imp->progressBar) {
        _imp->progressBar->setValue(_imp->progressPercent);
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
        _imp->status = eProgressTaskStatusRunning;
        _imp->statusItem->setText(tr("Running"));
        _imp->refreshButtons();
    }
}

void
ProgressTaskInfo::updateProgress(const int frame, double progress)
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

void
ProgressTaskInfo::getTableItems(std::vector<TableItem*>* items) const
{
    items->push_back(_imp->nameItem);
    items->push_back(_imp->progressItem);
    items->push_back(_imp->statusItem);
    items->push_back(_imp->controlsItem);
    items->push_back(_imp->timeRemainingItem);
    items->push_back(_imp->taskInfoItem);
}

void
ProgressTaskInfo::setCellWidgets(int row, TableView* view)
{
    view->setCellWidget(row, COL_PROGRESS, _imp->progressBar);
    view->setCellWidget(row, COL_CONTROLS, _imp->controlsButtonsContainer);
}

void
ProgressTaskInfo::removeCellWidgets(int row, TableView* view)
{
    view->removeCellWidget(row, COL_PROGRESS);
    view->removeCellWidget(row, COL_CONTROLS);
}

void
ProgressTaskInfo::createCellWidgets()
{
    _imp->progressBar = new QProgressBar;
    _imp->progressBar->setValue(_imp->progressPercent);
    //We must call this otherwise this is never called by Qt for custom widgets (this is a Qt bug)
    {
        QSize s = _imp->progressBar->minimumSizeHint();
        Q_UNUSED(s);
    }
    _imp->controlsButtonsContainer = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout(_imp->controlsButtonsContainer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    int medSizeIcon = TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE);
    
    QPixmap restartPix,pauseOnPix, pauseOffPix, clearTasksPix;
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PLAY_DISABLED, medSizeIcon, &restartPix);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_STOP_DISABLED, medSizeIcon, &clearTasksPix);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PAUSE_ENABLED, medSizeIcon, &pauseOnPix);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PAUSE_DISABLED, medSizeIcon, &pauseOffPix);
    
    const QSize medButtonIconSize(TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE),TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE));
    const QSize medButtonSize(TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE),TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE));
    
    QIcon pauseIc;
    pauseIc.addPixmap(pauseOnPix, QIcon::Normal, QIcon::On);
    pauseIc.addPixmap(pauseOffPix, QIcon::Normal, QIcon::Off);
    _imp->pauseTasksButton = new Button(pauseIc,"",_imp->controlsButtonsContainer);
    _imp->pauseTasksButton->setFixedSize(medButtonSize);
    _imp->pauseTasksButton->setIconSize(medButtonIconSize);
    _imp->pauseTasksButton->setFocusPolicy(Qt::NoFocus);
    _imp->pauseTasksButton->setToolTip(GuiUtils::convertFromPlainText(tr("Pause the task. Tasks that can be paused "
                                                                         "may be restarted with the restart button."), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->pauseTasksButton, SIGNAL(clicked(bool)), this, SLOT(onPauseTriggered()));
    _imp->pauseTasksButton->setEnabled(_imp->canBePaused);
    layout->addWidget(_imp->pauseTasksButton);
    
    _imp->restartTasksButton = new Button(QIcon(restartPix),"",_imp->controlsButtonsContainer);
    _imp->restartTasksButton->setFixedSize(medButtonSize);
    _imp->restartTasksButton->setIconSize(medButtonIconSize);
    _imp->restartTasksButton->setFocusPolicy(Qt::NoFocus);
    _imp->restartTasksButton->setToolTip(GuiUtils::convertFromPlainText(tr("Restart task"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->restartTasksButton, SIGNAL(clicked(bool)), this, SLOT(onRestartTriggered()));
    _imp->restartTasksButton->setEnabled(false);
    layout->addWidget(_imp->restartTasksButton);
    
    
    
    _imp->cancelTasksButton = new Button(QIcon(clearTasksPix),"",_imp->controlsButtonsContainer);
    _imp->cancelTasksButton->setFixedSize(medButtonSize);
    _imp->cancelTasksButton->setIconSize(medButtonIconSize);
    _imp->cancelTasksButton->setFocusPolicy(Qt::NoFocus);
    _imp->cancelTasksButton->setToolTip(GuiUtils::convertFromPlainText(tr("Cancel and remove the task from the list"), Qt::WhiteSpaceNormal));
    QObject::connect(_imp->cancelTasksButton, SIGNAL(clicked(bool)), this, SLOT(onCancelTriggered()));
    layout->addWidget(_imp->cancelTasksButton);

    _imp->refreshButtons();
}

void
ProgressTaskInfo::setProcesshandler(const boost::shared_ptr<ProcessHandler>& process)
{
    _imp->process = process;
}

void
ProgressTaskInfo::onPauseTriggered()
{
    if (!_imp->canBePaused) {
        return;
    }
    cancelTask(false, 1);
}

void
ProgressTaskInfo::onCancelTriggered()
{
    boost::shared_ptr<ProgressTaskInfo> thisShared = shared_from_this();
    cancelTask(false, 0);
    _imp->panel->removeTaskFromTable(thisShared);
}

void
ProgressTaskInfo::onRestartTriggered()
{
    if (!_imp->canBePaused) {
        return;
    }
    if (wasCanceled()) {
        restartTask();
    }

}

void
ProgressTaskInfoPrivate::refreshButtons()
{
    switch (status) {
        case eProgressTaskStatusRunning:
            pauseTasksButton->setEnabled(canBePaused);
            restartTasksButton->setEnabled(false);
            break;
        case eProgressTaskStatusQueued:
            pauseTasksButton->setEnabled(false);
            restartTasksButton->setEnabled(false);
            break;
        case eProgressTaskStatusFinished:
            pauseTasksButton->setEnabled(false);
            restartTasksButton->setEnabled(canBePaused);
            break;
        case eProgressTaskStatusCanceled:
            pauseTasksButton->setEnabled(false);
            restartTasksButton->setEnabled(false);
            break;
        case eProgressTaskStatusPaused:
            pauseTasksButton->setEnabled(false);
            restartTasksButton->setEnabled(canBePaused);
            break;
        default:
            break;
    }
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_ProgressTaskInfo.cpp"