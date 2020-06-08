/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#ifndef NATRON_ENGINE_TIMELINE_H
#define NATRON_ENGINE_TIMELINE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QMutex>
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)

#include "Global/GlobalDefines.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief A simple TimeLine representing the time for image sequences.
 * The interval [_firstFrame,_lastFrame] represents where images exist in the time space.
 * The interval [_leftBoundary,_rightBoundary] represents what the user interval of interest within the time space.
 * The _currentFrame represents the current time in the time space. It doesn't have to be within any aforementioned interval.
 **/

class TimeLine
    : public QObject
{
    Q_OBJECT

public:

    TimeLine(Project* project);

    virtual ~TimeLine()
    {
    }

    SequenceTime currentFrame() const;

    void seekFrame(SequenceTime frame,
                   bool updateLastCaller,
                   OutputEffectInstance* caller,
                   TimelineChangeReasonEnum reason);

    void incrementCurrentFrame();

    void decrementCurrentFrame();

public Q_SLOTS:


    void onFrameChanged(SequenceTime frame);


Q_SIGNALS:

    void frameAboutToChange();

    //reason being a TimelineChangeReasonEnum
    void frameChanged(SequenceTime, int reason);

private:

    mutable QMutex _lock; // protects the following SequenceTime members
    Project* _project;
    SequenceTime _currentFrame;
};

NATRON_NAMESPACE_EXIT

#endif /* defined(NATRON_ENGINE_TIMELINE_H_) */
