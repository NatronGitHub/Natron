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

#ifndef Natron_Engine_ThreadPool_h
#define Natron_Engine_ThreadPool_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <QThreadPool>

#include "Global/Macros.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

#ifdef QT_USE_NATRON_CUSTOM_THREADPOOL_EXT


/**
 * @brief This class provides a fast way to determine whether a render thread
 * is aborted or not.
 **/
struct AbortableThreadPrivate;
class AbortableThread
{
public:
    
    AbortableThread();
    
    virtual ~AbortableThread();
    
    void setAbortInfo(bool isRenderResponseToUserInteraction,
                      const AbortableRenderInfoPtr& abortInfo,
                      const EffectInstPtr& treeRoot);
    
    void clearAbortInfo();
    
    bool getAbortInfo(bool* isRenderResponseToUserInteraction,
                      AbortableRenderInfoPtr* abortInfo,
                      EffectInstPtr* treeRoot) const;
    
private:
    
    boost::scoped_ptr<AbortableThreadPrivate> _imp;
    
};

class ThreadPool : public QThreadPool
{
public:
    
    ThreadPool();
    
    virtual ~ThreadPool();
    
private:
    
    virtual QThreadPoolThread* createThreadPoolThread() const OVERRIDE FINAL;
};

#endif // QT_USE_NATRON_CUSTOM_THREADPOOL_EXT

NATRON_NAMESPACE_EXIT;

#endif // Natron_Engine_ThreadPool_h
