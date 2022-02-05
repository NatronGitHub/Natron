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

#include "TaskBar.h"

NATRON_NAMESPACE_ENTER

#ifdef Q_OS_WIN
// MINGW workaround
const GUID nIID_ITaskbarList3 = {0xea1afb91, 0x9e28, 0x4b86, {0x90,0xe9,0x9e,0x9f,0x8a,0x5e,0xef,0xaf}};
#endif

TaskBar::TaskBar(QWidget *parent)
    : QObject(parent)
#ifdef Q_OS_WIN
    , _wtask(0)
#elif defined(Q_OS_MAC)
    , _mtask(0)
#endif
    , _min(0.0)
    , _max(100.0)
    , _val(0.0)
    , _state(NoProgress)
{
#ifdef Q_OS_WIN
    _wid = parent->window()->winId();
    CoInitialize(NULL);
    HRESULT hres = CoCreateInstance(CLSID_TaskbarList,
                                    NULL,
                                    CLSCTX_INPROC_SERVER,
                                    nIID_ITaskbarList3,
                                    (LPVOID*)&_wtask);
    if ( FAILED(hres) ) {
        _wtask = NULL;
        CoUninitialize();
    } else {
        _wtask->HrInit();
    }
#elif defined(Q_OS_MAC)
    _mtask = new TaskBarMac(this);
#endif
}

TaskBar::~TaskBar()
{
#ifdef Q_OS_WIN
    if (_wtask) {
        _wtask->Release();
        _wtask = NULL;
        CoUninitialize();
    }
#endif
}

void
TaskBar::setProgressMinimum(double value)
{
    if (_min < 0.0) {
        return;
    }
    _min = value;
}

void
TaskBar::setProgressMaximum(double value)
{
    if (_max < 0.0) {
        return;
    }
    _max = value;
}

void
TaskBar::setProgressRange(double minVal,
                          double maxVal)
{
    if (minVal >= 0.0 && minVal < maxVal) {
        setProgressMinimum(minVal);
        setProgressMaximum(maxVal);
    }
}

void
TaskBar::setProgressValue(double value)
{
    if (_val == value) {
        return;
    }

    double currentVal = value - _min;
    double totalVal = _max - _min;
    if (currentVal < 0.0 || totalVal <= 0.0) {
        return;
    }
#ifdef Q_OS_WIN
    if (!_wtask) {
        return;
    }
    if (_wtask->SetProgressValue(_wid, currentVal, totalVal) == S_OK) {
        _val = value;
    }
#elif defined(Q_OS_MAC)
    if (!_mtask) {
        return;
    }
    _mtask->setProgress(currentVal / totalVal);
    _val = value;
#endif
}

void
TaskBar::setProgressState(TaskBar::ProgressState state)
{
    if (_state == state) {
        return;
    }

#ifdef Q_OS_WIN
    if (!_wtask) {
        return;
    }
    TBPFLAG flag;
    switch(state) {
    case NoProgress:
        flag = TBPF_NOPROGRESS;
        break;
    case IndeterminateProgress:
        flag = TBPF_INDETERMINATE;
        break;
    case PausedProgress:
        flag = TBPF_PAUSED;
        break;
    case ErrorProgress:
        flag = TBPF_ERROR;
        break;
    default:
        flag = TBPF_NORMAL;
        break;
    }
    if (_wtask->SetProgressState(_wid, flag) == S_OK) {
        _state = state;
    }
#elif defined(Q_OS_MAC)
    if (!_mtask) {
        return;
    }
    if (state == NoProgress) {
        _mtask->setProgressVisible(false);
    }
    _state = state;
#endif
}

void
TaskBar::clearProgress()
{
    setProgressRange(0, 100.0);
    setProgressValue(0.0);
    setProgressState(NoProgress);
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_TaskBar.cpp"
