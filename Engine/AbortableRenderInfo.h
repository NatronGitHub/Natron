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

#ifndef ABORTABLERENDERINFO_H
#define ABORTABLERENDERINFO_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"

#include <QtCore/QObject>
#include <QtCore/QThreadPool> // defines QT_CUSTOM_THREADPOOL (or not)

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER;

/**
 * @brief Holds infos necessary to identify one render request and whether it was aborted or not.
 **/
struct AbortableRenderInfoPrivate;
class AbortableRenderInfo
    : public QObject
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    AbortableRenderInfo();

    AbortableRenderInfo(bool canAbort,
                        U64 age);
    ~AbortableRenderInfo();

    // Is this render abortable ?
    bool canAbort() const;

    // Is this render aborted ?
    bool isAborted() const;

    // Set this render as aborted, cannot be reversed
    void setAborted();

    // Get the render age
    U64 getRenderAge() const;

#ifdef QT_CUSTOM_THREADPOOL
    // Register the thread as part of this render request
    void registerThreadForRender(AbortableThread* thread);

    // Unregister the thread as part of this render, returns true
    // if it was registered and found
    // This is called whenever a thread calls cleanupTLSForThread(), i.e:
    // when its processing is finished
    bool unregisterThreadForRender(AbortableThread* thread);
#endif

public Q_SLOTS:

    void onAbortTimerTimeout();

private:

    boost::scoped_ptr<AbortableRenderInfoPrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // ABORTABLERENDERINFO_H
