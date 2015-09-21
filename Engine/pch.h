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

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#ifdef __clang__
// fix clang warnings

// /usr/include/qt5/QtCore/qgenericatomic.h:177:13: warning: 'register' storage class specifier is deprecated [-Wdeprecated]
//             register T tmp = load(_q_value);
//             ^~~~~~~~~
// /usr/include/qt5/QtCore/qmetatype.h:688:5: warning: 'register' storage class specifier is deprecated [-Wdeprecated]
//     register int id = qMetaTypeId<T>();
//     ^~~~~~~~~
// /usr/include/qt5/QtCore/qsharedpointer_impl.h:481:13: warning: 'register' storage class specifier is deprecated [-Wdeprecated]
//            register int tmp = o->strongref.load();
//            ^~~~~~~~~
// #if QT_VERSION >= 0x050000
// CLANG_DIAG_OFF(deprecated);
// #include <QtCore/qgenericatomic.h>
// #include <QtCore/qmetatype.h>
// #include <QtCore/qsharedpointer.h>
// CLANG_DIAG_ON(deprecated);
// #else
// CLANG_DIAG_OFF(deprecated);
// #include <QtCore/qmetatype.h>
// // In file included from /opt/local/include/QtCore/qsharedpointer.h:50:
// // /opt/local/include/QtCore/qsharedpointer_impl.h:435:17: warning: 'register' storage class specifier is deprecated [-Wdeprecated-register]
// //                register int tmp = o->strongref;
// #include <QtCore/qsharedpointer.h>
// CLANG_DIAG_ON(deprecated);
// #endif

// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
//        static T cast(U u){
//CLANG_DIAG_OFF(unused-parameter);
//#include <boost/serialization/smart_cast.hpp>
//CLANG_DIAG_ON(unused-parameter);

//In file included from /opt/local/include/boost/bimap/list_of.hpp:37:
//In file included from /opt/local/include/boost/bimap/views/list_map_view.hpp:22:
//In file included from /opt/local/include/boost/bimap/relation/support/pair_by.hpp:21:
//In file included from /opt/local/include/boost/bimap/relation/support/pair_type_by.hpp:21:
//In file included from /opt/local/include/boost/bimap/relation/detail/metadata_access_builder.hpp:21:
//In file included from /opt/local/include/boost/bimap/relation/support/is_tag_of_member_at.hpp:26:
///opt/local/include/boost/bimap/relation/support/member_with_tag.hpp:73:5: warning: class member cannot be redeclared [-Wredeclared-class-member]
//    BOOST_BIMAP_STATIC_ERROR( MEMBER_WITH_TAG_FAILURE, (Relation,Tag) );
//CLANG_DIAG_OFF(redeclared-class-member)
//#include <boost/bimap.hpp>
//CLANG_DIAG_ON(redeclared-class-member)

//#if QT_VERSION < 0x050000
//CLANG_DIAG_OFF(unused-private-field);
//#include <QtGui/qmime.h>
//CLANG_DIAG_ON(unused-private-field);
//#endif

//In file included from ../Natron/Writers/ExrEncoder.cpp:20:
///opt/local/include/OpenEXR/half.h:471:2: warning: 'register' storage class specifier is deprecated [-Wdeprecated-register]
//        register int e = (x.i >> 23) & 0x000001ff;
//        ^~~~~~~~~
//CLANG_DIAG_OFF(deprecated-register);
//#include <OpenEXR/half.h>
//CLANG_DIAG_ON(deprecated-register);

#endif // __clang__

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

#include <QtCore>

#include "HostSupport/pch.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/Knob.h"
#include "Engine/Cache.h"

#endif // __cplusplus

#endif // ENGINE_PCH_H
