/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

#ifndef TASKBAR_H
#define TASKBAR_H

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#ifdef Q_OS_WIN
#include <shobjidl.h>
#elif defined(Q_OS_DARWIN)
#include "TaskBarMac.h"
#endif

NATRON_NAMESPACE_ENTER

#ifdef Q_OS_WIN
// MINGW workaround
extern const GUID nIID_ITaskbarList3;
#endif

class TaskBar : public QObject
{
    Q_OBJECT

public:

    enum ProgressState {
        NoProgress,
        IndeterminateProgress,
        PausedProgress,
        ErrorProgress,
        NormalProgress
    };

    explicit TaskBar(QWidget *parent = 0);
    ~TaskBar();

public Q_SLOTS:

    void setProgressMinimum(double value);
    void setProgressMaximum(double value);
    void setProgressRange(double minVal, double maxVal);
    void setProgressValue(double value);
    void setProgressState(TaskBar::ProgressState state);
    void clearProgress();

private:

#ifdef Q_OS_WIN
    ITaskbarList3 *_wtask;
    WId _wid;
#elif defined(Q_OS_DARWIN)
    TaskBarMac *_mtask;
#endif
    double _min;
    double _max;
    double _val;
    TaskBar::ProgressState _state;
};

NATRON_NAMESPACE_EXIT

#endif // TASKBAR_H
