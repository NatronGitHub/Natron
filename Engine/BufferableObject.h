/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Engine_BufferedFrame_h
#define Engine_BufferedFrame_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cstddef>
#include <list>

#include "Engine/EngineFwd.h"
#include "Engine/TimeValue.h"
#include "Engine/ViewIdx.h"
NATRON_NAMESPACE_ENTER;

/**
 * @brief An object that get passed between a render thread and the scheduler thread
 **/
class BufferedFrame
{

public:

    BufferedFrame()
    : view(0)
    {}

    virtual ~BufferedFrame() {}

    ViewIdx view;
    RenderStatsPtr stats;
};

class BufferedFrameContainer
{
public:

    BufferedFrameContainer()
    {

    }

    virtual ~BufferedFrameContainer() {}

    TimeValue time;

    // The list of frames that should be processed together by the scheduler
    std::list<BufferedFramePtr> frames;


};


NATRON_NAMESPACE_EXIT;

#endif // OUTPUTSCHEDULERTHREAD_H
