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

#ifndef NATRON_GUI_TEXTRENDERER_H
#define NATRON_GUI_TEXTRENDERER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Gui/GuiFwd.h"


NATRON_NAMESPACE_ENTER

class TextRenderer
{
public:

    TextRenderer();

    ~TextRenderer();

    void renderText(float x,
                    float y,
                    float scalex,
                    float scaley,
                    const QString &text,
                    const QColor &color,
                    const QFont &font,
                    int flags = 0) const; //!< see http://doc.qt.io/qt-4.8/qpainter.html#drawText-10

private:
    struct Implementation;
    boost::scoped_ptr<Implementation> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_TEXTRENDERER_H
