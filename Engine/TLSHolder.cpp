/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include "TLSHolder.h"
#include "TLSHolderImpl.h"

#include "Engine/OfxClipInstance.h"
#include "Engine/OfxHost.h"
#include "Engine/OfxParamInstance.h"
#include "Engine/Project.h"

#include <QWaitCondition>
#include <QThreadPool>
#include <QThread>
#include <QDebug>

NATRON_NAMESPACE_ENTER;


AppTLS::AppTLS()
: _objectMutex()
, _object(new GLobalTLSObject())
, _spawns()
{
}

AppTLS::~AppTLS()
{
    
}

void
AppTLS::registerTLSHolder(const boost::shared_ptr<const TLSHolderBase>& holder)
{
    //This must be the first time for this thread that we reach here since cleanupTLSForThread() was made, otherwise
    //the TLS data should always be available on the TLSHolder
    {
        QWriteLocker k(&_objectMutex);
        //The insert call might not succeed because another thread inserted this holder in the set already, that's fine
        _object->objects.insert(holder);
    }
    
}

void
AppTLS::copyTLS(const QThread* fromThread,const QThread* toThread)
{
    if (fromThread == toThread || !fromThread || !toThread) {
        return;
    }
    QReadLocker k(&_objectMutex);
    for (TLSObjects::iterator it = _object->objects.begin();
         it!=_object->objects.end(); ++it) {
        boost::shared_ptr<const TLSHolderBase> p = (*it).lock();
        if (p) {
            p->copyTLS(fromThread, toThread);
        }
    
    }
}

void
AppTLS::softCopy(const QThread* fromThread,const QThread* toThread)
{
    if (fromThread == toThread || !fromThread || !toThread) {
        return;
    }
    QWriteLocker k(&_objectMutex);
    _spawns[toThread] = fromThread;
}

void
AppTLS::cleanupTLSForThread()
{
    
    QThread* curThread = QThread::currentThread();

    //Cleanup any cached data on the TLSHolder
    {
        QWriteLocker k(&_objectMutex);
        
        //This thread was spawned, but TLS not used, do not bother to clean-up
        ThreadSpawnMap::iterator foundSpawned = _spawns.find(curThread);
        if (foundSpawned != _spawns.end()) {
            _spawns.erase(foundSpawned);
            return;
        }
        
        TLSObjects newObjects;
        for (TLSObjects::iterator it = _object->objects.begin();
             it!=_object->objects.end(); ++it) {
            boost::shared_ptr<const TLSHolderBase> p = (*it).lock();
            if (p) {
                if (!p->cleanupPerThreadData(curThread)) {
                    //The TLSHolder still has TLS on it for another thread and is still alive,
                    //then leave it in the set
                    newObjects.insert(p);
                }
            }
        }
        _object->objects = newObjects;
    }
}

template class TLSHolder<EffectInstance::EffectTLSData>;
template class TLSHolder<NATRON_NAMESPACE::OfxHost::OfxHostTLSData>;
template class TLSHolder<KnobHelper::KnobTLSData>;
template class TLSHolder<Project::ProjectTLSData>;
template class TLSHolder<OfxClipInstance::ClipTLSData>;
template class TLSHolder<OfxParamToKnob::OfxParamTLSData>;

NATRON_NAMESPACE_EXIT;

