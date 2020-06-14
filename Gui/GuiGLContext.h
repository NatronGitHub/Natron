/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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

#ifndef GUIGLCONTEXT_H
#define GUIGLCONTEXT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Gui/GuiFwd.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include "Global/GLIncludes.h" //!<must be included before QGLWidget
#include <QtOpenGL/QGLWidget>
#include "Global/GLObfuscate.h" //!<must be included after QGLWidget
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#include "Engine/OSGLContext.h"



NATRON_NAMESPACE_ENTER

/**
 * @brief A wrapper around a QGLContext managed by a QGLWidget so that it can be used in all our internal background rendering functions that usually
 * take a OSGLContext in parameter.
 **/
struct GuiGLContextPrivate;
class GuiGLContext : public OSGLContext
{
public:

    GuiGLContext(QGLWidget* widget);

    virtual ~GuiGLContext();

private:

    virtual void makeGPUContextCurrent() OVERRIDE FINAL;

    virtual void unsetGPUContext() OVERRIDE FINAL;

    boost::scoped_ptr<GuiGLContextPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // GUIGLCONTEXT_H
