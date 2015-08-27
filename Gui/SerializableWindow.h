/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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


#ifndef SERIALIZABLEWINDOW_H
#define SERIALIZABLEWINDOW_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

class QMutex;


/**
 * @brief A serializable window is a window which have mutex-protected members that can be accessed in the serialization thread
 * because width() height() pos() etc... are not thread-safe.
 **/
class SerializableWindow
{
    mutable QMutex* _lock;
    int _w,_h;
    int _x,_y;

public:

    SerializableWindow();

    virtual ~SerializableWindow();

    /**
     * @brief Should be called in resizeEvent
     **/
    void setMtSafeWindowSize(int w,int h);

    /**
     * @brief Should be called in moveEvent
     **/
    void setMtSafePosition(int x,int y);

    void getMtSafeWindowSize(int &w,int & h);

    void getMtSafePosition(int &x, int &y);
};

#endif // SERIALIZABLEWINDOW_H
