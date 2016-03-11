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

#ifndef Engine_BufferableObject_h
#define Engine_BufferableObject_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <cstddef>

NATRON_NAMESPACE_ENTER;

/**
 * @brief Stub class used by internal implementation of OutputSchedulerThread to pass objects through signal/slots
 **/
class BufferableObject
{
    
    int uniqueID; //< used to differentiate frames which may belong to the same time/view (e.g when wipe is enabled)
public:
    
    int getUniqueID() const
    {
        return uniqueID;
    }
    
    BufferableObject() : uniqueID(0) {}
    
    virtual ~BufferableObject() {}
    
    void setUniqueID(int aid)
    {
        uniqueID = aid;
    }
    
    virtual std::size_t sizeInRAM() const = 0;
};

typedef std::list<boost::shared_ptr<BufferableObject> > BufferableObjectList;

NATRON_NAMESPACE_EXIT;

#endif // OUTPUTSCHEDULERTHREAD_H
