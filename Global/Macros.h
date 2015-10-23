/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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
//
//  Created by Frédéric Devernay on 03/09/13.
//
//

#ifndef NATRON_GLOBAL_MACROS_H
#define NATRON_GLOBAL_MACROS_H

#ifdef __APPLE__
#define __NATRON_OSX__
#define __NATRON_UNIX__
#elif  defined(_WIN32)
#define __NATRON_WIN32__
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__) || defined(__FreeBSD__)
#define __NATRON_UNIX__
#define __NATRON_LINUX__
#endif

#define NATRON_APPLICATION_DESCRIPTION "Open-source, cross-platform, nodal compositing software."
#define NATRON_COPYRIGHT "Copyright (C) 2015 the Natron developers."
#define NATRON_ORGANIZATION_NAME "INRIA"
#define NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "fr"
#define NATRON_ORGANIZATION_DOMAIN_SUB "inria"
#define NATRON_ORGANIZATION_DOMAIN NATRON_ORGANIZATION_DOMAIN_SUB "." NATRON_ORGANIZATION_DOMAIN_TOPLEVEL
#define NATRON_APPLICATION_NAME "Natron"
#define NATRON_WEBSITE_URL "http://www.natron.fr"
#define NATRON_ISSUE_TRACKER_URL "https://github.com/MrKepzie/Natron/issues"
// The MIME types for Natron documents are:
// *.ntp: application/vnd.natron.project
// *.nps: application/vnd.natron.nodepresets
// *.nl: application/vnd.natron.layout
// these MIME types are also used in:
// - NatronInfo.plist (for OSX)
// - tools/linux/include/qs/natron.qs
#define NATRON_PROJECT_FILE_EXT "ntp"
#define NATRON_PROJECT_FILE_MIME_TYPE "application/vnd.natron.project"
#define NATRON_PROJECT_UNTITLED "Untitled." NATRON_PROJECT_FILE_EXT
#define NATRON_CACHE_FILE_EXT "ntc"
#define NATRON_LAYOUT_FILE_EXT "nl"
#define NATRON_LAYOUT_FILE_MIME_TYPE "application/vnd.natron.layout"
#define NATRON_PRESETS_FILE_EXT "nps"
#define NATRON_PRESETS_FILE_MIME_TYPE "application/vnd.natron.nodepresets"
#define NATRON_PROJECT_ENV_VAR_NAME "Project"
#define NATRON_OCIO_ENV_VAR_NAME "OCIO"
#define NATRON_DEFAULT_OCIO_CONFIG_NAME "blender"

//Define here the name of the Engine module that was chosen in the typesystem_engine.xml
#define NATRON_ENGINE_PYTHON_MODULE_NAME "NatronEngine"
#define NATRON_GUI_PYTHON_MODULE_NAME "NatronGui"
//Uncomment to run Natron without Python functionnalities (for debug purposes)
//#define NATRON_RUN_WITHOUT_PYTHON

#define NATRON_ENV_VAR_NAME_START_TAG "<Name>"
#define NATRON_ENV_VAR_NAME_END_TAG "</Name>"
#define NATRON_ENV_VAR_VALUE_START_TAG "<Value>"
#define NATRON_ENV_VAR_VALUE_END_TAG "</Value>"

#define NATRON_PROJECT_ENV_VAR_MAX_RECURSION 100
#define NATRON_MAX_CACHE_FILES_OPENED 20000
#define NATRON_CUSTOM_HTML_TAG_START "<" NATRON_APPLICATION_NAME ">"
#define NATRON_CUSTOM_HTML_TAG_END "</" NATRON_APPLICATION_NAME ">"


#define NATRON_FILE_DIALOG_PREVIEW_READER_NAME "Natron_File_Dialog_Preview_Provider_Reader"
#define NATRON_FILE_DIALOG_PREVIEW_VIEWER_NAME "Natron_File_Dialog_Preview_Provider_Viewer"

//////////////////////////////////////////Natron version/////////////////////////////////////////////
#define NATRON_VERSION_MAJOR 2
#define NATRON_VERSION_MINOR 0
#define NATRON_VERSION_REVISION 0

<<<<<<< HEAD
///For example RC 1, RC 2 etc...
#define NATRON_BUILD_NUMBER 1

=======
>>>>>>> workshop
#define NATRON_LAST_VERSION_URL "https://raw.githubusercontent.com/MrKepzie/Natron/workshop/LATEST_VERSION.txt"
#define NATRON_LAST_VERSION_FILE_VERSION 1

// homemade builds should always show "Devel"
#define NATRON_DEVELOPMENT_DEVEL "Devel"
// the following are reserved for actual releases (binary and tarballs)
#define NATRON_DEVELOPMENT_ALPHA "Alpha"
#define NATRON_DEVELOPMENT_BETA "Beta"
#define NATRON_DEVELOPMENT_RELEASE_CANDIDATE "RC"
#define NATRON_DEVELOPMENT_RELEASE_STABLE "Release"
// The snapshot build scripts should add '-DNATRON_SNAPSHOT' to the compile
// options.
#define NATRON_DEVELOPMENT_SNAPSHOT "Snapshot"


#ifdef NATRON_CONFIG_SNAPSHOT
#define NATRON_DEVELOPMENT_STATUS NATRON_DEVELOPMENT_SNAPSHOT
#elif defined(NATRON_CONFIG_ALPHA)
#define NATRON_DEVELOPMENT_STATUS NATRON_DEVELOPMENT_ALPHA
#elif defined(NATRON_CONFIG_BETA)
#define NATRON_DEVELOPMENT_STATUS NATRON_CONFIG_BETA
#elif defined(NATRON_CONFIG_RC)
#define NATRON_DEVELOPMENT_STATUS NATRON_DEVELOPMENT_RELEASE_CANDIDATE
#elif defined(NATRON_CONFIG_STABLE)
#define NATRON_DEVELOPMENT_STATUS NATRON_DEVELOPMENT_RELEASE_STABLE
#else
//Fallback on "Devel" builds (most likely built from command line without passing to qmake the appropriate defines)
#define NATRON_DEVELOPMENT_STATUS NATRON_DEVELOPMENT_DEVEL
#endif

///For example RC 1, RC 2 etc... This is to be defined from withing the qmake call, passing BUILD_NUMBER=X to the command line
//#define NATRON_BUILD_NUMBER 0


///If set the version of Natron will no longer be displayed in the splashscreen but the name of the user
///Set this from qmake

#define STRINGIZE_CPP_NAME_(token) #token
#define STRINGIZE_CPP_NAME(token) STRINGIZE_CPP_NAME_(token)

#ifdef NATRON_CUSTOM_BUILD_USER_TOKEN
#define NATRON_CUSTOM_BUILD_USER_NAME STRINGIZE_CPP_NAME(NATRON_CUSTOM_BUILD_USER_TOKEN)
#else
#define NATRON_CUSTOM_BUILD_USER_NAME ""
#endif

#define NATRON_VERSION_ENCODE(major,minor,revision) ( \
( (major) * 10000 ) \
+ ( (minor) * 100 )  \
+ ( (revision) * 1 ) )

#define NATRON_VERSION_ENCODED NATRON_VERSION_ENCODE( \
NATRON_VERSION_MAJOR, \
NATRON_VERSION_MINOR, \
NATRON_VERSION_REVISION)

#define NATRON_VERSION_STRINGIZE_(major,minor,revision) \
# major "." # minor "." # revision

#define NATRON_VERSION_STRINGIZE(major,minor,revision) \
NATRON_VERSION_STRINGIZE_(major,minor,revision)

#define NATRON_VERSION_STRING NATRON_VERSION_STRINGIZE( \
NATRON_VERSION_MAJOR, \
NATRON_VERSION_MINOR, \
NATRON_VERSION_REVISION)
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define NATRON_PATH_ENV_VAR "NATRON_PLUGIN_PATH"
#define NATRON_IMAGES_PATH ":/Resources/Images/"
#define NATRON_APPLICATION_ICON_PATH NATRON_IMAGES_PATH "natronIcon256_linux.png"

///Natron will load all icons that are associated to a group toolbutton with the following icon set number, i.e:
///if it is 2, then it will load color_grouping_2.png , filter_grouping_2.png , etc... this way you can compile
///with different icons set easily.
#define NATRON_ICON_SET_BLACK_AND_WHITE "2"
#define NATRON_ICON_SET_FADED_COLOURS "3"
#define NATRON_ICON_SET_NUMBER NATRON_ICON_SET_FADED_COLOURS

#define PLUGIN_GROUP_IMAGE "Image"
#define PLUGIN_GROUP_IMAGE_READERS "Readers"
#define PLUGIN_GROUP_IMAGE_WRITERS "Writers"
#define PLUGIN_GROUP_COLOR "Color"
#define PLUGIN_GROUP_FILTER "Filter"
#define PLUGIN_GROUP_TRANSFORM "Transform"
#define PLUGIN_GROUP_TIME "Time"
#define PLUGIN_GROUP_PAINT "Draw"
#define PLUGIN_GROUP_KEYER "Keyer"
#define PLUGIN_GROUP_CHANNEL "Channel"
#define PLUGIN_GROUP_MERGE "Merge"
#define PLUGIN_GROUP_MULTIVIEW "Views"
#define PLUGIN_GROUP_DEEP "Deep"
#define PLUGIN_GROUP_TOOLSETS "ToolSets"
#define PLUGIN_GROUP_3D "3D"
#define PLUGIN_GROUP_OTHER "Other"
#define PLUGIN_GROUP_DEFAULT "Misc"
#define PLUGIN_GROUP_OFX "OFX"

//Use this to use trimap instead of bitmap to avoid several threads computing the same area of an image at the same time.
//When enabled the value of 2 is a code for a pixel being rendered but not yet available.
//In this context, the reader of the bitmap should then wait for the pixel to be available.
#define NATRON_ENABLE_TRIMAP 1

// compiler_warning.h
#define STRINGISE_IMPL(x) # x
#define STRINGISE(x) STRINGISE_IMPL(x)

// Use: #pragma message WARN("My message")
#if _MSC_VER
#   define FILE_LINE_LINK __FILE__ "(" STRINGISE(__LINE__) ") : "
#   define WARN(exp) (FILE_LINE_LINK "WARNING: " exp)
#else //__GNUC__ - may need other defines for different compilers
#   define WARN(exp) ("WARNING: " exp)
#endif

// The following was grabbed from WTF/wtf/Compiler.h (where WTF was replaced by NATRON)

/* COMPILER() - the compiler being used to build the project */
#define COMPILER(NATRON_FEATURE) (defined NATRON_COMPILER_ ## NATRON_FEATURE && NATRON_COMPILER_ ## NATRON_FEATURE)

/* COMPILER_SUPPORTS() - whether the compiler being used to build the project supports the given feature. */
#define COMPILER_SUPPORTS(NATRON_COMPILER_FEATURE) (defined NATRON_COMPILER_SUPPORTS_ ## NATRON_COMPILER_FEATURE && NATRON_COMPILER_SUPPORTS_ ## NATRON_COMPILER_FEATURE)

/* COMPILER_QUIRK() - whether the compiler being used to build the project requires a given quirk. */
#define COMPILER_QUIRK(NATRON_COMPILER_QUIRK) (defined NATRON_COMPILER_QUIRK_ ## NATRON_COMPILER_QUIRK && NATRON_COMPILER_QUIRK_ ## NATRON_COMPILER_QUIRK)

/* ==== COMPILER() - the compiler being used to build the project ==== */

/* COMPILER(CLANG) - Clang */
#if defined(__clang__)
#define NATRON_COMPILER_CLANG 1

#ifndef __has_extension
#define __has_extension __has_feature /* Compatibility with older versions of clang */
#endif

#define CLANG_PRAGMA(PRAGMA) _Pragma(PRAGMA)

/* Specific compiler features */
#define NATRON_COMPILER_SUPPORTS_CXX_VARIADIC_TEMPLATES __has_extension(cxx_variadic_templates)

/* There is a bug in clang that comes with Xcode 4.2 where AtomicStrings can't be implicitly converted to Strings
   in the presence of move constructors and/or move assignment operators. This bug has been fixed in Xcode 4.3 clang, so we
   check for both cxx_rvalue_references as well as the unrelated cxx_nonstatic_member_init feature which we know was added in 4.3 */
#define NATRON_COMPILER_SUPPORTS_CXX_RVALUE_REFERENCES __has_extension(cxx_rvalue_references) && __has_extension(cxx_nonstatic_member_init)

#define NATRON_COMPILER_SUPPORTS_CXX_DELETED_FUNCTIONS __has_extension(cxx_deleted_functions)
#define NATRON_SUPPORTS_CXX_NULLPTR __has_feature(cxx_nullptr)
#define NATRON_COMPILER_SUPPORTS_CXX_EXPLICIT_CONVERSIONS __has_feature(cxx_explicit_conversions)
#define NATRON_COMPILER_SUPPORTS_BLOCKS __has_feature(blocks)
#define NATRON_COMPILER_SUPPORTS_C_STATIC_ASSERT __has_extension(c_static_assert)
#define NATRON_COMPILER_SUPPORTS_CXX_OVERRIDE_CONTROL __has_extension(cxx_override_control)
#define NATRON_COMPILER_SUPPORTS_HAS_TRIVIAL_DESTRUCTOR __has_extension(has_trivial_destructor)

#endif

#ifndef CLANG_PRAGMA
#define CLANG_PRAGMA(PRAGMA)
#endif

/* COMPILER(MSVC) - Microsoft Visual C++ */
/* COMPILER(MSVC7_OR_LOWER) - Microsoft Visual C++ 2003 or lower*/
/* COMPILER(MSVC9_OR_LOWER) - Microsoft Visual C++ 2008 or lower*/
#if defined(_MSC_VER)
#define NATRON_COMPILER_MSVC 1
#if _MSC_VER < 1400
#define NATRON_COMPILER_MSVC7_OR_LOWER 1
#elif _MSC_VER < 1600
#define NATRON_COMPILER_MSVC9_OR_LOWER 1
#endif

/* Specific compiler features */
#if !COMPILER(CLANG) && _MSC_VER >= 1600
#define NATRON_SUPPORTS_CXX_NULLPTR 1
#endif

#if !COMPILER(CLANG)
#define NATRON_COMPILER_SUPPORTS_CXX_OVERRIDE_CONTROL 1
#define NATRON_COMPILER_QUIRK_FINAL_IS_CALLED_SEALED 1
#endif

#endif

/* COMPILER(RVCT) - ARM RealView Compilation Tools */
/* COMPILER(RVCT4_OR_GREATER) - ARM RealView Compilation Tools 4.0 or greater */
#if defined(__CC_ARM) || defined(__ARMCC__)
#define NATRON_COMPILER_RVCT 1
#define RVCT_VERSION_AT_LEAST(major, minor, patch, build) ( __ARMCC_VERSION >= (major * 100000 + minor * 10000 + patch * 1000 + build) )
#else
/* Define this for !RVCT compilers, just so we can write things like RVCT_VERSION_AT_LEAST(3, 0, 0, 0). */
#define RVCT_VERSION_AT_LEAST(major, minor, patch, build) 0
#endif

/* COMPILER(GCCE) - GNU Compiler Collection for Embedded */
#if defined(__GCCE__)
#define NATRON_COMPILER_GCCE 1
#define GCCE_VERSION (__GCCE__ * 10000 + __GCCE_MINOR__ * 100 + __GCCE_PATCHLEVEL__)
#define GCCE_VERSION_AT_LEAST(major, minor, patch) ( GCCE_VERSION >= (major * 10000 + minor * 100 + patch) )
#endif

/* COMPILER(GCC) - GNU Compiler Collection */
/* --gnu option of the RVCT compiler also defines __GNUC__ */
#if defined(__GNUC__) && !COMPILER(RVCT)
#define NATRON_COMPILER_GCC 1
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#define GCC_VERSION_AT_LEAST(major, minor, patch) ( GCC_VERSION >= (major * 10000 + minor * 100 + patch) )
#else
/* Define this for !GCC compilers, just so we can write things like GCC_VERSION_AT_LEAST(4, 1, 0). */
#define GCC_VERSION_AT_LEAST(major, minor, patch) 0
#endif

/* Specific compiler features */
#if COMPILER(GCC) && !COMPILER(CLANG)
#if GCC_VERSION_AT_LEAST(4, 7, 0) && defined(__cplusplus) && __cplusplus >= 201103L
#define NATRON_COMPILER_SUPPORTS_CXX_RVALUE_REFERENCES 1
#define NATRON_COMPILER_SUPPORTS_CXX_DELETED_FUNCTIONS 1
#define NATRON_SUPPORTS_CXX_NULLPTR 1
#define NATRON_COMPILER_SUPPORTS_CXX_OVERRIDE_CONTROL 1
#define NATRON_COMPILER_QUIRK_GCC11_GLOBAL_ISINF_ISNAN 1

#elif GCC_VERSION_AT_LEAST(4, 6, 0) && defined(__GXX_EXPERIMENTAL_CXX0X__)
#define NATRON_SUPPORTS_CXX_NULLPTR 1
#define NATRON_COMPILER_QUIRK_GCC11_GLOBAL_ISINF_ISNAN 1
#endif

#endif

/* COMPILER(MINGW) - MinGW GCC */
/* COMPILER(MINGW64) - mingw-w64 GCC - only used as additional check to exclude mingw.org specific functions */
#if defined(__MINGW32__)
#define NATRON_COMPILER_MINGW 1
#include <_mingw.h> /* private MinGW header */
#if defined(__MINGW64_VERSION_MAJOR) /* best way to check for mingw-w64 vs mingw.org */
#define NATRON_COMPILER_MINGW64 1
#endif /* __MINGW64_VERSION_MAJOR */
#endif /* __MINGW32__ */

/* COMPILER(INTEL) - Intel C++ Compiler */
#if defined(__INTEL_COMPILER)
#define NATRON_COMPILER_INTEL 1
#endif

/* COMPILER(SUNCC) */
#if defined(__SUNPRO_CC) || defined(__SUNPRO_C)
#define NATRON_COMPILER_SUNCC 1
#endif

/* ==== Compiler features ==== */


/* ALWAYS_INLINE */

#ifndef ALWAYS_INLINE
#if COMPILER(GCC) && defined(NDEBUG) && !COMPILER(MINGW)
#define ALWAYS_INLINE inline __attribute__( (__always_inline__) )
#elif (COMPILER(MSVC) || COMPILER(RVCT ) ) && defined(NDEBUG)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline
#endif
#endif


/* NEVER_INLINE */

#ifndef NEVER_INLINE
#if COMPILER(GCC)
#define NEVER_INLINE __attribute__( (__noinline__) )
#elif COMPILER(RVCT)
#define NEVER_INLINE __declspec(noinline)
#else
#define NEVER_INLINE
#endif
#endif


/* UNLIKELY */

#ifndef UNLIKELY
#if COMPILER(GCC) || (RVCT_VERSION_AT_LEAST(3, 0, 0, 0) && defined(__GNUC__ ) )
#define UNLIKELY(x) __builtin_expect( (x), 0 )
#else
#define UNLIKELY(x) (x)
#endif
#endif


/* LIKELY */

#ifndef LIKELY
#if COMPILER(GCC) || (RVCT_VERSION_AT_LEAST(3, 0, 0, 0) && defined(__GNUC__ ) )
#define LIKELY(x) __builtin_expect( (x), 1 )
#else
#define LIKELY(x) (x)
#endif
#endif


/* NO_RETURN */


#ifndef NO_RETURN
#if COMPILER(GCC)
#define NO_RETURN __attribute( (__noreturn__) )
#elif COMPILER(MSVC) || COMPILER(RVCT)
#define NO_RETURN __declspec(noreturn)
#else
#define NO_RETURN
#endif
#endif


/* NO_RETURN_WITH_VALUE */

#ifndef NO_RETURN_WITH_VALUE
#if !COMPILER(MSVC)
#define NO_RETURN_WITH_VALUE NO_RETURN
#else
#define NO_RETURN_WITH_VALUE
#endif
#endif


/* WARN_UNUSED_RETURN */

#if COMPILER(GCC)
#define WARN_UNUSED_RETURN __attribute__ ( (warn_unused_result) )
// a very simple template function to actually ignore the return value of functions define with WARN_UNUSED_RETURN
template<typename T>
inline T ignore_result(T x __attribute__((unused)))
{
    return x;
}
#else
#define WARN_UNUSED_RETURN
template<typename T>
inline T ignore_result(T x)
{
    return x;
}
#endif

/* OVERRIDE and FINAL */

#if COMPILER_SUPPORTS(CXX_OVERRIDE_CONTROL) &&  !COMPILER(MSVC) //< patch so msvc 2010 ignores the override and final keywords.
#define OVERRIDE override

#if COMPILER_QUIRK(FINAL_IS_CALLED_SEALED)
#define FINAL sealed
#else
#define FINAL final
#endif

#else
#define OVERRIDE
#define FINAL
#endif

/* REFERENCED_FROM_ASM */

#ifndef REFERENCED_FROM_ASM
#if COMPILER(GCC)
#define REFERENCED_FROM_ASM __attribute__( (used) )
#else
#define REFERENCED_FROM_ASM
#endif
#endif

/* OBJC_CLASS */

#ifndef OBJC_CLASS
#ifdef __OBJC__
#define OBJC_CLASS @class
#else
#define OBJC_CLASS class
#endif
#endif

/* https://code.google.com/p/address-sanitizer/wiki/AddressSanitizer#Turning_off_instrumentation */
#if defined(__clang__) || defined (__GNUC__)
# define ATTRIBUTE_NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#else
# define ATTRIBUTE_NO_SANITIZE_ADDRESS
#endif

/* ABI */
#if defined(__ARM_EABI__) || defined(__EABI__)
#define NATRON_COMPILER_SUPPORTS_EABI 1
#endif

// Warning control from https://svn.boost.org/trac/boost/wiki/Guidelines/WarningsGuidelines
#if ( ( __GNUC__ * 100) + __GNUC_MINOR__) >= 402
#define GCC_DIAG_STR(s) # s
#define GCC_DIAG_JOINSTR(x,y) GCC_DIAG_STR(x ## y)
# define GCC_DIAG_DO_PRAGMA(x) _Pragma ( # x)
# define GCC_DIAG_PRAGMA(x) GCC_DIAG_DO_PRAGMA(GCC diagnostic x)
# if ( ( __GNUC__ * 100) + __GNUC_MINOR__) >= 406
#  define GCC_DIAG_OFF(x) GCC_DIAG_PRAGMA(push) \
    GCC_DIAG_PRAGMA( ignored GCC_DIAG_JOINSTR(-W,x) )
#  define GCC_DIAG_ON(x) GCC_DIAG_PRAGMA(pop)
# else
#  define GCC_DIAG_OFF(x) GCC_DIAG_PRAGMA( ignored GCC_DIAG_JOINSTR(-W,x) )
#  define GCC_DIAG_ON(x)  GCC_DIAG_PRAGMA( warning GCC_DIAG_JOINSTR(-W,x) )
# endif
#else
# define GCC_DIAG_OFF(x)
# define GCC_DIAG_ON(x)
#endif

#ifdef __clang__
#  define CLANG_DIAG_STR(s) # s
// stringize s to "no-unused-variable"
#  define CLANG_DIAG_JOINSTR(x,y) CLANG_DIAG_STR(x ## y)
//  join -W with no-unused-variable to "-Wno-unused-variable"
#  define CLANG_DIAG_DO_PRAGMA(x) _Pragma ( # x)
// _Pragma is unary operator  #pragma ("")
#  define CLANG_DIAG_PRAGMA(x) CLANG_DIAG_DO_PRAGMA(clang diagnostic x)
#    define CLANG_DIAG_OFF(x) CLANG_DIAG_PRAGMA(push) \
    CLANG_DIAG_PRAGMA( ignored CLANG_DIAG_JOINSTR(-W,x) )
// For example: #pragma clang diagnostic ignored "-Wno-unused-variable"
#   define CLANG_DIAG_ON(x) CLANG_DIAG_PRAGMA(pop)
// For example: #pragma clang diagnostic warning "-Wno-unused-variable"
#else // Ensure these macros so nothing for other compilers.
#  define CLANG_DIAG_OFF(x)
#  define CLANG_DIAG_ON(x)
#  define CLANG_DIAG_PRAGMA(x)
#endif


/* Usage:
 CLANG_DIAG_OFF(unused-variable)
 CLANG_DIAG_OFF(unused-parameter)
 CLANG_DIAG_OFF(uninitialized)
 */


#ifndef __has_warning         // Optional of course.
#define __has_warning(x) 0  // Compatibility with non-clang compilers.
#endif

#if ( ( __GNUC__ * 100) + __GNUC_MINOR__) >= 408
//  -Wunused-local-typedefs appeared with GCC 4.8
# define GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF GCC_DIAG_OFF(unused-local-typedefs)
# define GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON GCC_DIAG_ON(unused-local-typedefs)
#else
#if __has_warning("-Wunused-local-typedef") // both unused-local-typedefs and unused-local-typedef should be available
# define GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF CLANG_DIAG_OFF(unused-local-typedef)
# define GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON CLANG_DIAG_ON(unused-local-typedef)
#elif __has_warning("-Wunused-local-typedefs")
# define GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF CLANG_DIAG_OFF(unused-local-typedefs)
# define GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON CLANG_DIAG_ON(unused-local-typedefs)
#else
# define GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
# define GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif
#endif

//#if ( ( __GNUC__ * 100) + __GNUC_MINOR__) >= 408
////  -Wunused-private-field appeared with GCC 4.8
//# define GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF GCC_DIAG_OFF(unused-private-field)
//# define GCC_DIAG_UNUSED_PRIVATE_FIELD_ON GCC_DIAG_ON(unused-private-field)
//#else
#if __has_warning("-Wunused-private-field")
# define GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF CLANG_DIAG_OFF(unused-private-field)
# define GCC_DIAG_UNUSED_PRIVATE_FIELD_ON CLANG_DIAG_ON(unused-private-field)
#else
# define GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
# define GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#endif
//#endif

#if ( ( __GNUC__ * 100) + __GNUC_MINOR__) >= 510
//  -Wsuggest-override appeared with GCC 5.1
# define GCC_DIAG_SUGGEST_OVERRIDE_OFF GCC_DIAG_OFF(suggest-override)
# define GCC_DIAG_SUGGEST_OVERRIDE_ON GCC_DIAG_ON(suggest-override)
#else
#if __has_warning("-Winconsistent-missing-override")
# define GCC_DIAG_SUGGEST_OVERRIDE_OFF CLANG_DIAG_OFF(inconsistent-missing-override)
# define GCC_DIAG_SUGGEST_OVERRIDE_ON CLANG_DIAG_ON(inconsistent-missing-override)
#else
# define GCC_DIAG_SUGGEST_OVERRIDE_OFF
# define GCC_DIAG_SUGGEST_OVERRIDE_ON
#endif
#endif

#if COMPILER_SUPPORTS(CXX_OVERRIDE_CONTROL)
// we want to use override & final, and get no warnings even if not compiling in c++11 mode
CLANG_DIAG_OFF(c++11-extensions)
GCC_DIAG_OFF(c++11-extensions)
#endif

#endif // ifndef NATRON_GLOBAL_MACROS_H
