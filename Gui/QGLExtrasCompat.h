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

// This is a header that provides type declarations that map a subset of the newer QOpenGL
// classes used by Natron with their older QGL counterparts

#ifndef NATRON_GUI_QGLEXTRASCOMPAT_H
#define NATRON_GUI_QGLEXTRASCOMPAT_H

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
#error "This file must not be included while using Qt >= 5.4.0"
#endif

#include "Global/Macros.h"

// QGL was deprecated in macOS
CLANG_DIAG_OFF(deprecated)
#include <QtOpenGL/QGLShaderProgram>
#include <QtOpenGL/QGLShader>
CLANG_DIAG_ON(deprecated)

typedef QGLShaderProgram QOpenGLShaderProgram;
typedef QSharedPointer<QGLShaderProgram> QOpenGLShaderProgramPtr;
typedef QGLShader QOpenGLShader;

#endif // NATRON_GUI_QGLEXTRASCOMPAT_H
