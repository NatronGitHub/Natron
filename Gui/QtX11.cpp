/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#include <QtCore/QtGlobal>

#ifdef Q_WS_X11

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>

namespace QtX11 {

qreal
devicePixelRatioInternal(Display* display)
{
    // see https://github.com/glfw/glfw/blob/84f95a7d7fa454ca99efcdd49da89472294b16bf/src/x11_init.c#L971

    qreal dpi = 96.;

    char *rms = XResourceManagerString(display);
    if (rms) {
        XrmDatabase db = XrmGetStringDatabase(rms);
        if (db) {
            XrmValue value;
            char *type = NULL;

            XrmInitialize(); /* Need to initialize the DB before calling Xrm* functions */

            if (XrmGetResource(db, "Xft.dpi", "Xft.Dpi", &type, &value)) {
                if (type && strcmp(type, "String") == 0) {
                    dpi = atof(value.addr);
                }
            }
        }
    }

    return dpi / 96.;
}

}

#endif
