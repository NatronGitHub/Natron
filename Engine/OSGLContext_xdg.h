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

#ifndef OSGLCONTEXT_XDG_H
#define OSGLCONTEXT_XDG_H

#include "Global/Macros.h"

#ifdef __NATRON_LINUX__

NATRON_NAMESPACE_ENTER

class OSGLContext_xdg
{
public:

    virtual void swapBuffers() = 0;

    virtual void swapInterval(int interval) = 0;

    virtual ~OSGLContext_xdg() = 0;
};

inline OSGLContext_xdg::~OSGLContext_xdg() { }

NATRON_NAMESPACE_EXIT

#endif // __NATRON_LINUX__

#endif // OSGLCONTEXT_XDG_H
