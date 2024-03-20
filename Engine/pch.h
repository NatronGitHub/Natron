// -*-c++-*-
#ifndef ENGINE_PCH_H
#define ENGINE_PCH_H
// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if defined(__cplusplus)

#include <vector>
#include <string>
#include <map>

#include "Global/Macros.h"

#ifdef __APPLE__
// Silence Apple's OpenGL deprecation warnings
//
//../Natron/Gui/ViewerGL.cpp:1606:5: warning: 'gluErrorString' is deprecated: first deprecated in OS X 10.9 [-Wdeprecated-declarations]
///System/Library/Frameworks/OpenGL.framework/Headers/glu.h:260:24: note: 'gluErrorString' declared here
//extern const GLubyte * gluErrorString (GLenum error) OPENGL_DEPRECATED(10_0, 10_9);
//                       ^
#include <AvailabilityMacros.h>
#ifdef MAC_OS_X_VERSION_10_9
#include <OpenGL/OpenGLAvailability.h>
#undef OPENGL_DEPRECATED
#define OPENGL_DEPRECATED(from, to)
#endif // MAC_OS_X_VERSION_10_9
#endif

#include "Global/Macros.h"

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_ONLY_DIAG_OFF(class-memaccess)
#include <QtCore/QVector>
GCC_ONLY_DIAG_ON(class-memaccess)
#include <QtCore>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "HostSupport/pch.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/Knob.h"
#include "Engine/Cache.h"

#endif // __cplusplus

#endif // ENGINE_PCH_H
