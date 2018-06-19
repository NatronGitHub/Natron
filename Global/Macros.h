/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#ifdef __MINGW32__
#define __NATRON_MINGW__
#endif
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__) || defined(__FreeBSD__)
#define __NATRON_UNIX__
#define __NATRON_LINUX__
#endif

#ifdef SBK_RUN

// run shiboken without the Natron namespace, and add NATRON_NAMESPACE_USING to each cpp afterwards
#define NATRON_NAMESPACE
#define NATRON_NAMESPACE_ENTER
#define NATRON_NAMESPACE_EXIT
#define NATRON_PYTHON_NAMESPACE
#define NATRON_PYTHON_NAMESPACE_ENTER
#define NATRON_PYTHON_NAMESPACE_EXIT

#else // !SBK_RUN

#define NATRON_NAMESPACE Natron
// Macros to use in each file to enter and exit the right name spaces.
#define NATRON_NAMESPACE_ENTER namespace NATRON_NAMESPACE {
#define NATRON_NAMESPACE_EXIT }
#define NATRON_NAMESPACE_USING using namespace NATRON_NAMESPACE;

#define NATRON_PYTHON_NAMESPACE Python
#define NATRON_PYTHON_NAMESPACE_ENTER namespace NATRON_PYTHON_NAMESPACE {
#define NATRON_PYTHON_NAMESPACE_EXIT }
#define NATRON_PYTHON_NAMESPACE_USING using namespace NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE;

#ifdef __cplusplus
// Establish the name space.
namespace NATRON_NAMESPACE { }
namespace NATRON_PYTHON_NAMESPACE { }
#endif

#endif

#define NATRON_NAMESPACE_ANONYMOUS_ENTER namespace {
#define NATRON_NAMESPACE_ANONYMOUS_EXIT }

#define NATRON_APPLICATION_DESCRIPTION "Open-source, cross-platform, nodal video compositing software."
#define NATRON_COPYRIGHT "Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat."
#define NATRON_ORGANIZATION_NAME "INRIA"
#define NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "fr"
#define NATRON_ORGANIZATION_DOMAIN_SUB "inria"
#define NATRON_ORGANIZATION_DOMAIN NATRON_ORGANIZATION_DOMAIN_SUB "." NATRON_ORGANIZATION_DOMAIN_TOPLEVEL
#define NATRON_APPLICATION_NAME "Natron"
#define NATRON_WEBSITE_URL "http://www.natron.fr"
#define NATRON_FORUM_URL "https://discuss.pixls.us/c/software/natron"
#define NATRON_ISSUE_TRACKER_URL "https://github.com/NatronGitHub/Natron/issues"

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

//Define here the name of the Engine module that was chosen in the typesystem_engine.xml
#define NATRON_ENGINE_PYTHON_MODULE_NAME "NatronEngine"
#define NATRON_GUI_PYTHON_MODULE_NAME "NatronGui"

#define NATRON_PROJECT_ENV_VAR_MAX_RECURSION 100
#define NATRON_MAX_CACHE_FILES_OPENED 20000
#define NATRON_CUSTOM_HTML_TAG_START "<" NATRON_APPLICATION_NAME ">"
#define NATRON_CUSTOM_HTML_TAG_END "</" NATRON_APPLICATION_NAME ">"


#define NATRON_FILE_DIALOG_PREVIEW_READER_NAME "Natron_File_Dialog_Preview_Provider_Reader"
#define NATRON_FILE_DIALOG_PREVIEW_VIEWER_NAME "Natron_File_Dialog_Preview_Provider_Viewer"

//////////////////////////////////////////Natron version/////////////////////////////////////////////

// The currently maintained Natron versions
// RB-3: 3.0.0
#define NATRON_VERSION_MAJOR_30 3
#define NATRON_VERSION_MINOR_30 0
#define NATRON_VERSION_REVISION_30 0

// RB-2: 2.3.13
#define NATRON_VERSION_MAJOR_23 2
#define NATRON_VERSION_MINOR_23 3
#define NATRON_VERSION_REVISION_23 13

// RB-2.2: 2.2.10
#define NATRON_VERSION_MAJOR_22 2
#define NATRON_VERSION_MINOR_22 2
#define NATRON_VERSION_REVISION_22 10

// RB-2.1: 2.1.10
#define NATRON_VERSION_MAJOR_21 2
#define NATRON_VERSION_MINOR_21 1
#define NATRON_VERSION_REVISION_21 10

// The Natron version for this branch
#define NATRON_VERSION_MAJOR NATRON_VERSION_MAJOR_23
#define NATRON_VERSION_MINOR NATRON_VERSION_MINOR_23
#define NATRON_VERSION_REVISION NATRON_VERSION_REVISION_23


#define NATRON_LAST_VERSION_URL "http://downloads.natron.fr/LATEST_VERSION.txt"
#define NATRON_LAST_VERSION_FILE_VERSION 1

// homemade builds should always show "Devel"
#define NATRON_DEVELOPMENT_DEVEL "Devel"
// the following are reserved for actual releases (binary and tarballs)
#define NATRON_DEVELOPMENT_ALPHA "Alpha"
#define NATRON_DEVELOPMENT_BETA "Beta"
#define NATRON_DEVELOPMENT_RELEASE_CANDIDATE "RC"
#define NATRON_DEVELOPMENT_RELEASE_STABLE "Release"
// The snapshot build scripts should add '-DNATRON_CONFIG_SNAPSHOT' to the compile
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


// Documentation
#if (NATRON_VERSION_MAJOR == NATRON_VERSION_MAJOR_22) && (NATRON_VERSION_MINOR == NATRON_VERSION_MINOR_22)
#define NATRON_DOCUMENTATION_ONLINE "http://natron.readthedocs.io/en/rb-2.2"
//#elif (NATRON_VERSION_MAJOR == NATRON_VERSION_MAJOR_30) && (NATRON_VERSION_MINOR == NATRON_VERSION_MINOR_30)
//#define NATRON_DOCUMENTATION_ONLINE "http://natron.readthedocs.io/en/rb-3.0"
#else
#define NATRON_DOCUMENTATION_ONLINE "http://natron.readthedocs.io"
#endif

#if defined(__NATRON_LINUX__) || defined(__NATRON_OSX__)
/*
   On Linux crash reporter MUST use fork() to spawn the Natron process because it needs to duplicate file descriptors for the pipe.
   On Windows, fork() doesn't exist so we use QProcess.
   OS X can use both because it doesn't require a file descriptor to be passed to Natron for the breakpad pipe.
 */
#define NATRON_CRASH_REPORTER_USE_FORK 1
#endif


#define NATRON_BREAKPAD_PROCESS_EXEC "breakpad_process_exec"
#define NATRON_BREAKPAD_PROCESS_PID "breakpad_process_pid"
#define NATRON_BREAKPAD_CLIENT_FD_ARG "breakpad_client_fd"
#define NATRON_BREAKPAD_PIPE_ARG "breakpad_pipe_path"
#define NATRON_BREAKPAD_COM_PIPE_ARG "breakpad_com_pipe_path"

#define NATRON_NATRON_TO_BREAKPAD_EXISTENCE_CHECK "-e"
#define NATRON_NATRON_TO_BREAKPAD_EXISTENCE_CHECK_ACK "-eack"

///If set the version of Natron will no longer be displayed in the splashscreen but the name of the user
///Set this from qmake

#define STRINGIZE_CPP_NAME_(token) # token
#define STRINGIZE_CPP_NAME(token) STRINGIZE_CPP_NAME_(token)

#ifdef NATRON_CUSTOM_BUILD_USER_TOKEN
#define NATRON_CUSTOM_BUILD_USER_NAME STRINGIZE_CPP_NAME(NATRON_CUSTOM_BUILD_USER_TOKEN)
#else
#define NATRON_CUSTOM_BUILD_USER_NAME ""
#endif

#define NATRON_VERSION_ENCODE(major, minor, revision) ( \
        ( (major) * 10000 ) \
        + ( (minor) * 100 )  \
        + ( (revision) * 1 ) )

#define NATRON_VERSION_ENCODED NATRON_VERSION_ENCODE( \
        NATRON_VERSION_MAJOR, \
        NATRON_VERSION_MINOR, \
        NATRON_VERSION_REVISION)

// Natron version string: if revision is 0, use only major.minor, else major.minor.revision
#if NATRON_VERSION_REVISION > 0
#define NATRON_VERSION_STRINGIZE__(major, minor, revision) \
    # major "." # minor "." # revision

#define NATRON_VERSION_STRINGIZE_(major, minor, revision) \
    NATRON_VERSION_STRINGIZE__(major, minor, revision)

#define NATRON_VERSION_STRING NATRON_VERSION_STRINGIZE_( \
        NATRON_VERSION_MAJOR, \
        NATRON_VERSION_MINOR, \
        NATRON_VERSION_REVISION)
#else
#define NATRON_VERSION_STRINGIZE__(major, minor) \
# major "." # minor

#define NATRON_VERSION_STRINGIZE_(major, minor) \
NATRON_VERSION_STRINGIZE__(major, minor)

#define NATRON_VERSION_STRING NATRON_VERSION_STRINGIZE_( \
        NATRON_VERSION_MAJOR, \
        NATRON_VERSION_MINOR)
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define NATRON_PATH_ENV_VAR "NATRON_PLUGIN_PATH"
#define NATRON_DISK_CACHE_PATH_ENV_VAR "NATRON_DISK_CACHE_PATH"
#define NATRON_IMAGES_PATH ":/Resources/Images/"
#define NATRON_APPLICATION_ICON_PATH NATRON_IMAGES_PATH "natronIcon256_linux.png"

///Natron will load all icons that are associated to a group toolbutton with the following icon set number, i.e:
///if it is 2, then it will load color_grouping_2.png , filter_grouping_2.png , etc... this way you can compile
///with different icons set easily.
#define NATRON_ICON_SET_BLACK_AND_WHITE "2"
#define NATRON_ICON_SET_FADED_COLOURS "3"
#define NATRON_ICON_SET_NUMBER NATRON_ICON_SET_FADED_COLOURS

// Group ordering is set at every place in the code where GROUP_ORDER appears in the comments
#define PLUGIN_GROUP_IMAGE "Image"
#define PLUGIN_GROUP_IMAGE_READERS "Readers"
#define PLUGIN_GROUP_IMAGE_WRITERS "Writers"
#define PLUGIN_GROUP_PAINT "Draw"
#define PLUGIN_GROUP_TIME "Time"
#define PLUGIN_GROUP_CHANNEL "Channel"
#define PLUGIN_GROUP_COLOR "Color"
#define PLUGIN_GROUP_FILTER "Filter"
#define PLUGIN_GROUP_KEYER "Keyer"
#define PLUGIN_GROUP_MERGE "Merge"
#define PLUGIN_GROUP_TRANSFORM "Transform"
#define PLUGIN_GROUP_3D "3D"
#define PLUGIN_GROUP_DEEP "Deep"
#define PLUGIN_GROUP_MULTIVIEW "Views"
#define PLUGIN_GROUP_TOOLSETS "ToolSets"
#define PLUGIN_GROUP_OTHER "Other"
#define PLUGIN_GROUP_DEFAULT "Misc"
#define PLUGIN_GROUP_OFX "OFX"

//Use this to use trimap instead of bitmap to avoid several threads computing the same area of an image at the same time.
//When enabled the value of 2 is a code for a pixel being rendered but not yet available.
//In this context, the reader of the bitmap should then wait for the pixel to be available.
#define NATRON_ENABLE_TRIMAP 1

//Use this to have all readers inside the same Read meta-node and all the writers
//into the same Write meta-node
#define NATRON_ENABLE_IO_META_NODES 1

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

#if defined(__clang__)
#define CLANG_PRAGMA(PRAGMA) _Pragma(PRAGMA)
#else
#define CLANG_PRAGMA(PRAGMA)
#endif

// Warning control from https://svn.boost.org/trac/boost/wiki/Guidelines/WarningsGuidelines
#if ( ( __GNUC__ * 100) + __GNUC_MINOR__) >= 402
#define GCC_DIAG_STR(s) # s
#define GCC_DIAG_JOINSTR(x, y) GCC_DIAG_STR(x ## y)
# define GCC_DIAG_DO_PRAGMA(x) _Pragma ( # x)
# define GCC_DIAG_PRAGMA(x) GCC_DIAG_DO_PRAGMA(GCC diagnostic x)
# if defined(__clang__) || ( ( __GNUC__ * 100) + __GNUC_MINOR__) >= 406
#  define GCC_DIAG_OFF(x) GCC_DIAG_PRAGMA(push) \
GCC_DIAG_PRAGMA( ignored GCC_DIAG_JOINSTR(-W, x) )
#  define GCC_DIAG_ON(x) GCC_DIAG_PRAGMA(pop)
#  define GCC_DIAG_PEDANTIC_OFF GCC_DIAG_PRAGMA(push) \
GCC_DIAG_PRAGMA( ignored GCC_DIAG_PEDANTIC )
#  define GCC_DIAG_PEDANTIC_ON GCC_DIAG_PRAGMA(pop)
# else
#  define GCC_DIAG_OFF(x) GCC_DIAG_PRAGMA( ignored GCC_DIAG_JOINSTR(-W, x) )
#  define GCC_DIAG_ON(x)  GCC_DIAG_PRAGMA( warning GCC_DIAG_JOINSTR(-W, x) )
#  define GCC_DIAG_PEDANTIC_OFF GCC_DIAG_PRAGMA( ignored GCC_DIAG_PEDANTIC )
#  define GCC_DIAG_PEDANTIC_ON  GCC_DIAG_PRAGMA( warning GCC_DIAG_PEDANTIC )
# endif
#else
# define GCC_DIAG_OFF(x)
# define GCC_DIAG_ON(x)
# define GCC_DIAG_PEDANTIC_OFF
# define GCC_DIAG_PEDANTIC_ON
#endif

#ifdef __clang__
#  define CLANG_DIAG_STR(s) # s
// stringize s to "no-unused-variable"
#  define CLANG_DIAG_JOINSTR(x, y) CLANG_DIAG_STR(x ## y)
//  join -W with no-unused-variable to "-Wno-unused-variable"
#  define CLANG_DIAG_DO_PRAGMA(x) _Pragma ( # x)
// _Pragma is unary operator  #pragma ("")
#  define CLANG_DIAG_PRAGMA(x) CLANG_DIAG_DO_PRAGMA(clang diagnostic x)
#    define CLANG_DIAG_OFF(x) CLANG_DIAG_PRAGMA(push) \
CLANG_DIAG_PRAGMA( ignored CLANG_DIAG_JOINSTR(-W, x) )
// For example: #pragma clang diagnostic ignored "-Wno-unused-variable"
#   define CLANG_DIAG_ON(x) CLANG_DIAG_PRAGMA(pop)
// For example: #pragma clang diagnostic warning "-Wno-unused-variable"
#  define GCC_DIAG_PEDANTIC "-Wpedantic"
#else // Ensure these macros so nothing for other compilers.
#  define CLANG_DIAG_OFF(x)
#  define CLANG_DIAG_ON(x)
#  define CLANG_DIAG_PRAGMA(x)
#  if (__GNUC__ >= 7)
#    define GCC_DIAG_PEDANTIC "-Wpedantic"
#  else
// GCC before ~4.8 does not accept "-Wpedantic" quietly.
#    define GCC_DIAG_PEDANTIC "-pedantic"
#  endif
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

// silence warnings from the COMPILER() COMPILER_SUPPORTS() and COMPILER_QUIRK() macros below
#if __has_warning("-Wexpansion-to-defined")
CLANG_DIAG_OFF(expansion-to-defined)
#endif


/////////////////////////////////////////////////////////////////////////////////////////////
// The following was grabbed from WTF/wtf/Compiler.h (where WTF was replaced by NATRON)
// see https://trac.webkit.org/browser/webkit/trunk/Source/WTF/wtf/Compiler.h?format=txt
/////////////////////////////////////////////////////////////////////////////////////////////

/* COMPILER() - the compiler being used to build the project */
#define COMPILER(NATRON_FEATURE) (defined NATRON_COMPILER_ ## NATRON_FEATURE && NATRON_COMPILER_ ## NATRON_FEATURE)

/* COMPILER_SUPPORTS() - whether the compiler being used to build the project supports the given feature. */
#define COMPILER_SUPPORTS(NATRON_COMPILER_FEATURE) (defined NATRON_COMPILER_SUPPORTS_ ## NATRON_COMPILER_FEATURE && NATRON_COMPILER_SUPPORTS_ ## NATRON_COMPILER_FEATURE)

/* COMPILER_QUIRK() - whether the compiler being used to build the project requires a given quirk. */
#define COMPILER_QUIRK(NATRON_COMPILER_QUIRK) (defined NATRON_COMPILER_QUIRK_ ## NATRON_COMPILER_QUIRK && NATRON_COMPILER_QUIRK_ ## NATRON_COMPILER_QUIRK)

/* COMPILER_HAS_CLANG_HEATURE() - whether the compiler supports a particular language or library feature. */
/* http://clang.llvm.org/docs/LanguageExtensions.html#has-feature-and-has-extension */
#ifdef __has_feature
#define COMPILER_HAS_CLANG_FEATURE(x) __has_feature(x)
#else
#define COMPILER_HAS_CLANG_FEATURE(x) 0
#endif

/* COMPILER_HAS_CLANG_DECLSPEC() - whether the compiler supports a Microsoft style __declspec attribute. */
/* https://clang.llvm.org/docs/LanguageExtensions.html#has-declspec-attribute */
#ifdef __has_declspec_attribute
#define COMPILER_HAS_CLANG_DECLSPEC(x) __has_declspec_attribute(x)
#else
#define COMPILER_HAS_CLANG_DECLSPEC(x) 0
#endif

/* ==== COMPILER() - primary detection of the compiler being used to build the project, in alphabetical order ==== */

/* COMPILER(CLANG) - Clang */

#if defined(__clang__)
#define NATRON_COMPILER_CLANG 1
#define NATRON_COMPILER_SUPPORTS_BLOCKS COMPILER_HAS_CLANG_FEATURE(blocks)
#define NATRON_COMPILER_SUPPORTS_C_STATIC_ASSERT COMPILER_HAS_CLANG_FEATURE(c_static_assert)
#define NATRON_COMPILER_SUPPORTS_CXX_REFERENCE_QUALIFIED_FUNCTIONS COMPILER_HAS_CLANG_FEATURE(cxx_reference_qualified_functions)
#define NATRON_COMPILER_SUPPORTS_CXX_EXCEPTIONS COMPILER_HAS_CLANG_FEATURE(cxx_exceptions)
#define NATRON_COMPILER_SUPPORTS_BUILTIN_IS_TRIVIALLY_COPYABLE COMPILER_HAS_CLANG_FEATURE(is_trivially_copyable)

#ifdef __cplusplus
#if __cplusplus <= 201103L
#define NATRON_CPP_STD_VER 11
#elif __cplusplus <= 201402L
#define NATRON_CPP_STD_VER 14
#endif
#endif

#endif // defined(__clang__)

/* COMPILER(GCC_OR_CLANG) - GNU Compiler Collection or Clang */
#if defined(__GNUC__)
#define NATRON_COMPILER_GCC_OR_CLANG 1
#endif

/* COMPILER(GCC) - GNU Compiler Collection */
/* Note: This section must come after the Clang section since we check !COMPILER(CLANG) here. */
#if COMPILER(GCC_OR_CLANG) && !COMPILER(CLANG)
#define NATRON_COMPILER_GCC 1
#define NATRON_COMPILER_SUPPORTS_CXX_REFERENCE_QUALIFIED_FUNCTIONS 1

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#define GCC_VERSION_AT_LEAST(major, minor, patch) (GCC_VERSION >= (major * 10000 + minor * 100 + patch))

#if !GCC_VERSION_AT_LEAST(4, 9, 0)
//#error "Please use a newer version of GCC. WebKit requires GCC 4.9.0 or newer to compile."
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define NATRON_COMPILER_SUPPORTS_C_STATIC_ASSERT 1
#endif

#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

#endif /* COMPILER(GCC) */

/* COMPILER(MINGW) - MinGW GCC */

#if defined(__MINGW32__)
#define NATRON_COMPILER_MINGW 1
#include <_mingw.h>
#endif

/* COMPILER(MINGW64) - mingw-w64 GCC - used as additional check to exclude mingw.org specific functions */

/* Note: This section must come after the MinGW section since we check COMPILER(MINGW) here. */

#if COMPILER(MINGW) && defined(__MINGW64_VERSION_MAJOR) /* best way to check for mingw-w64 vs mingw.org */
#define NATRON_COMPILER_MINGW64 1
#endif

/* COMPILER(MSVC) - Microsoft Visual C++ */

#if defined(_MSC_VER)

#define NATRON_COMPILER_MSVC 1
#define NATRON_COMPILER_SUPPORTS_CXX_REFERENCE_QUALIFIED_FUNCTIONS 1

#if _MSC_VER < 1900
//#error "Please use a newer version of Visual Studio. WebKit requires VS2015 or newer to compile."
#endif

#endif

/* COMPILER(SUNCC) */

#if defined(__SUNPRO_CC) || defined(__SUNPRO_C)
#define NATRON_COMPILER_SUNCC 1
#endif

#if !COMPILER(CLANG) && !COMPILER(MSVC)
#define NATRON_COMPILER_QUIRK_CONSIDERS_UNREACHABLE_CODE 1
#endif

/* ==== COMPILER_SUPPORTS - additional compiler feature detection, in alphabetical order ==== */

/* COMPILER_SUPPORTS(EABI) */

#if defined(__ARM_EABI__) || defined(__EABI__)
#define NATRON_COMPILER_SUPPORTS_EABI 1
#endif

/* RELAXED_CONSTEXPR */

#if defined(__cpp_constexpr) && __cpp_constexpr >= 201304
#define NATRON_COMPILER_SUPPORTS_RELAXED_CONSTEXPR 1
#endif

#if !defined(RELAXED_CONSTEXPR)
#if COMPILER_SUPPORTS(RELAXED_CONSTEXPR)
#define RELAXED_CONSTEXPR constexpr
#else
#define RELAXED_CONSTEXPR
#endif
#endif

#define ASAN_ENABLED COMPILER_HAS_CLANG_FEATURE(address_sanitizer)

#if ASAN_ENABLED
#define SUPPRESS_ASAN __attribute__((no_sanitize_address))
#else
#define SUPPRESS_ASAN
#endif

/* ==== Compiler-independent macros for various compiler features, in alphabetical order ==== */

/* ALWAYS_INLINE */

#if !defined(ALWAYS_INLINE) && COMPILER(GCC_OR_CLANG) && defined(NDEBUG) && !COMPILER(MINGW)
#define ALWAYS_INLINE inline __attribute__((__always_inline__))
#endif

#if !defined(ALWAYS_INLINE) && COMPILER(MSVC) && defined(NDEBUG)
#define ALWAYS_INLINE __forceinline
#endif

#if !defined(ALWAYS_INLINE)
#define ALWAYS_INLINE inline
#endif

/* NATRON_EXTERN_C_{BEGIN, END} */

#ifdef __cplusplus
#define NATRON_EXTERN_C_BEGIN extern "C" {
#define NATRON_EXTERN_C_END }
#else
#define NATRON_EXTERN_C_BEGIN
#define NATRON_EXTERN_C_END
#endif

/* FALLTHROUGH */

#if !defined(FALLTHROUGH) && defined(__cplusplus) && defined(__has_cpp_attribute)

#if __has_cpp_attribute(fallthrough)
#define FALLTHROUGH [[fallthrough]]
#elif __has_cpp_attribute(clang::fallthrough)
#define FALLTHROUGH [[clang::fallthrough]]
#elif __has_cpp_attribute(gnu::fallthrough)
#define FALLTHROUGH [[gnu::fallthrough]]
#endif

#endif // !defined(FALLTHROUGH) && defined(__cplusplus) && defined(__has_cpp_attribute)

#if !defined(FALLTHROUGH)
#define FALLTHROUGH
#endif

/* LIKELY */

#if !defined(LIKELY) && COMPILER(GCC_OR_CLANG)
#define LIKELY(x) __builtin_expect(!!(x), 1)
#endif

#if !defined(LIKELY)
#define LIKELY(x) (x)
#endif

/* NEVER_INLINE */

#if !defined(NEVER_INLINE) && COMPILER(GCC_OR_CLANG)
#define NEVER_INLINE __attribute__((__noinline__))
#endif

#if !defined(NEVER_INLINE) && COMPILER(MSVC)
#define NEVER_INLINE __declspec(noinline)
#endif

#if !defined(NEVER_INLINE)
#define NEVER_INLINE
#endif

/* NO_RETURN */

#if !defined(NO_RETURN) && COMPILER(GCC_OR_CLANG)
#define NO_RETURN __attribute((__noreturn__))
#endif

#if !defined(NO_RETURN) && COMPILER(MSVC)
#define NO_RETURN __declspec(noreturn)
#endif

#if !defined(NO_RETURN)
#define NO_RETURN
#endif

/* RETURNS_NONNULL */
#if !defined(RETURNS_NONNULL) && COMPILER(GCC_OR_CLANG)
#define RETURNS_NONNULL __attribute__((returns_nonnull))
#endif

#if !defined(RETURNS_NONNULL)
#define RETURNS_NONNULL
#endif

/* NO_RETURN_WITH_VALUE */

#if !defined(NO_RETURN_WITH_VALUE) && !COMPILER(MSVC)
#define NO_RETURN_WITH_VALUE NO_RETURN
#endif

#if !defined(NO_RETURN_WITH_VALUE)
#define NO_RETURN_WITH_VALUE
#endif

/* OBJC_CLASS */

#if !defined(OBJC_CLASS) && defined(__OBJC__)
#define OBJC_CLASS @class
#endif

#if !defined(OBJC_CLASS)
#define OBJC_CLASS class
#endif

/* PURE_FUNCTION */

#if !defined(PURE_FUNCTION) && COMPILER(GCC_OR_CLANG)
#define PURE_FUNCTION __attribute__((__pure__))
#endif

#if !defined(PURE_FUNCTION)
#define PURE_FUNCTION
#endif

/* UNUSED_FUNCTION */

#if !defined(UNUSED_FUNCTION) && COMPILER(GCC_OR_CLANG)
#define UNUSED_FUNCTION __attribute__((unused))
#endif

#if !defined(UNUSED_FUNCTION)
#define UNUSED_FUNCTION
#endif

/* REFERENCED_FROM_ASM */

#if !defined(REFERENCED_FROM_ASM) && COMPILER(GCC_OR_CLANG)
#define REFERENCED_FROM_ASM __attribute__((__used__))
#endif

#if !defined(REFERENCED_FROM_ASM)
#define REFERENCED_FROM_ASM
#endif

/* UNLIKELY */

#if !defined(UNLIKELY) && COMPILER(GCC_OR_CLANG)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

#if !defined(UNLIKELY)
#define UNLIKELY(x) (x)
#endif

/* UNUSED_LABEL */

/* Keep the compiler from complaining for a local label that is defined but not referenced. */
/* Helpful when mixing hand-written and autogenerated code. */

#if !defined(UNUSED_LABEL) && COMPILER(MSVC)
#define UNUSED_LABEL(label) if (false) goto label
#endif

#if !defined(UNUSED_LABEL)
#define UNUSED_LABEL(label) UNUSED_PARAM(&& label)
#endif

/* UNUSED_PARAM */

#if !defined(UNUSED_PARAM) && COMPILER(MSVC)
#define UNUSED_PARAM(variable) (void)&variable
#endif

#if !defined(UNUSED_PARAM)
#define UNUSED_PARAM(variable) (void)variable
#endif

/* WARN_UNUSED_RETURN */

#if !defined(WARN_UNUSED_RETURN) && COMPILER(GCC_OR_CLANG)
#define WARN_UNUSED_RETURN __attribute__((__warn_unused_result__))
#endif

#if !defined(WARN_UNUSED_RETURN)
#define WARN_UNUSED_RETURN
#endif

#if !defined(__has_include) && COMPILER(MSVC)
#define __has_include(path) 0
#endif

// End of https://trac.webkit.org/browser/webkit/trunk/Source/WTF/wtf/Compiler.h?format=txt
/////////////////////////////////////////////////////////////////////////////////////////////

// Bonus definitions and stuff from older WTF versions

#if defined(__clang__)

#ifdef __has_extension
#define COMPILER_HAS_CLANG_EXTENSION(x) __has_extension(x)
#else
#define COMPILER_HAS_CLANG_FEATURE(x) COMPILER_HAS_CLANG_FEATURE(x) /* Compatibility with older versions of clang */
#endif

/* Specific compiler features */
#define NATRON_COMPILER_SUPPORTS_CXX_VARIADIC_TEMPLATES __has_extension(cxx_variadic_templates)

/* There is a bug in clang that comes with Xcode 4.2 where AtomicStrings can't be implicitly converted to Strings
 in the presence of move constructors and/or move assignment operators. This bug has been fixed in Xcode 4.3 clang, so we
 check for both cxx_rvalue_references as well as the unrelated cxx_nonstatic_member_init feature which we know was added in 4.3 */
#define NATRON_COMPILER_SUPPORTS_CXX_RVALUE_REFERENCES COMPILER_HAS_CLANG_EXTENSION(cxx_rvalue_references) && COMPILER_HAS_CLANG_EXTENSION(cxx_nonstatic_member_init)

#define NATRON_COMPILER_SUPPORTS_CXX_DELETED_FUNCTIONS COMPILER_HAS_CLANG_EXTENSION(cxx_deleted_functions)
#define NATRON_SUPPORTS_CXX_NULLPTR COMPILER_HAS_CLANG_FEATURE(cxx_nullptr)
#define NATRON_COMPILER_SUPPORTS_CXX_EXPLICIT_CONVERSIONS COMPILER_HAS_CLANG_FEATURE(cxx_explicit_conversions)
#define NATRON_COMPILER_SUPPORTS_CXX_OVERRIDE_CONTROL COMPILER_HAS_CLANG_EXTENSION(cxx_override_control)
#define NATRON_COMPILER_SUPPORTS_HAS_TRIVIAL_DESTRUCTOR COMPILER_HAS_CLANG_EXTENSION(has_trivial_destructor)
#endif

/* COMPILER(MSVC7_OR_LOWER) - Microsoft Visual C++ 2003 or lower*/
/* COMPILER(MSVC9_OR_LOWER) - Microsoft Visual C++ 2008 or lower*/
#if defined(_MSC_VER)
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

#endif // defined(_MSC_VER)


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

#endif // COMPILER(GCC) && !COMPILER(CLANG)

/* ignore_result */
// a very simple template function to actually ignore the return value of functions define with WARN_UNUSED_RETURN
#if COMPILER(GCC)
#ifdef __cplusplus
template<typename T>
inline T
ignore_result( T x __attribute__( (unused) ) )
{
    return x;
}
#endif
#else // !GCC
#ifdef __cplusplus
template<typename T>
inline T
ignore_result(T x)
{
    return x;
}
#endif
#endif // !GCC

/* OVERRIDE and FINAL */

#if COMPILER_SUPPORTS(CXX_OVERRIDE_CONTROL) && !COMPILER(MSVC) //< patch so msvc 2010 ignores the override and final keywords.
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

#if COMPILER_SUPPORTS(CXX_OVERRIDE_CONTROL)
// we want to use override & final, and get no warnings even if not compiling in c++11 mode
CLANG_DIAG_OFF(c++11-extensions)
//GCC_DIAG_OFF(c++11-extensions) // no corresponding GCC warning option?
#endif


#endif // ifndef NATRON_GLOBAL_MACROS_H
