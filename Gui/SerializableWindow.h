//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef SERIALIZABLEWINDOW_H
#define SERIALIZABLEWINDOW_H

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
