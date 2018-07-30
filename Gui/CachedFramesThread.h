/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#ifndef NATRON_ENGINE_CACHEDFRAMESTHREADTHREAD_H
#define NATRON_ENGINE_CACHEDFRAMESTHREADTHREAD_H

#include <list>

#include "Gui/GuiFwd.h"
#include "Global/Macros.h"

#include <QThread>


#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/TimeValue.h"

NATRON_NAMESPACE_ENTER

class CachedFramesThread : public QThread
{
    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    CachedFramesThread(ViewerTab* viewer);

    virtual ~CachedFramesThread();

    void getCachedFrames(std::list<TimeValue>* cachedFrames) const;

    void quitThread();

Q_SIGNALS:

    void cachedFramesRefreshed();
    
private:

    virtual void run() OVERRIDE FINAL;

    struct Implementation;
    boost::scoped_ptr<Implementation> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_CACHEDFRAMESTHREADTHREAD_H
