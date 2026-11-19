# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2023 The Natron developers
# (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
#
# Natron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Natron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****

set(Tests_HEADERS BaseTest.h NaNGuards_FastMath.h OfxTestPlugin.h)
set(Tests_SOURCES
    google-test/src/gtest-all.cc
    google-mock/src/gmock-all.cc
    BaseTest.cpp
    Curve_Test.cpp
    DiskCache_Test.cpp
    FileSystemModel_Test.cpp
    Hash64_Test.cpp
    ImageCache_Test.cpp
    Image_Test.cpp
    KnobFile_Test.cpp
    KnobSlave_Test.cpp
    KnobStringAnimation_Test.cpp
    Lut_Test.cpp
    MultiInputLinks_Test.cpp
    NaNGuards_FastMath.cpp
    NaNGuards_Test.cpp
    NodeGroup_Test.cpp
    NodePreview_Test.cpp
    OfxBytesParam_Test.cpp
    OfxCPURenderSupport_Test.cpp
    OfxClipGetImage_Test.cpp
    OfxCustomParamInterp_Test.cpp
    OfxHostProperties_Test.cpp
    OfxImageMemory_Test.cpp
    OfxInstanceChanged_Test.cpp
    OfxMultiThreadSuite_Test.cpp
    OfxMultiplePAR_Test.cpp
    OfxParamAnimating_Test.cpp
    OfxParamCopy_Test.cpp
    OfxParamInteract_Test.cpp
    OfxParamUndo_Test.cpp
    OfxPluginDeprecated_Test.cpp
    OfxPluginScan_Test.cpp
    OfxSingleInstance_Test.cpp
    OfxSyncPrivateData_Test.cpp
    OfxTestPlugin_Test.cpp
    OpenGLRenderFailed_Test.cpp
    OSGLContext_Test.cpp
    PluginGrouping_Test.cpp
    ProjectAutoSave_Test.cpp
    ReadNodeOverlay_Test.cpp
    RenderThreadSafety_Test.cpp
    RotoPaint_Test.cpp
    Tracker_Test.cpp
    ViewerCache_Test.cpp
    WriteNodeViews_Test.cpp
    wmain.cpp
)
add_executable(Tests ${Tests_HEADERS} ${Tests_SOURCES})
# NaNGuards_FastMath.cpp checks that MathUtils::isFinite()/isNaN() still work in code
# compiled with -ffast-math, which assumes that NaN and infinity never happen.
if(MSVC) # MSVC and clang-cl
    set_source_files_properties(NaNGuards_FastMath.cpp PROPERTIES COMPILE_OPTIONS "/fp:fast")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(NaNGuards_FastMath.cpp PROPERTIES
        COMPILE_OPTIONS "-ffast-math"
        COMPILE_DEFINITIONS NATRON_TESTS_FAST_MATH_TU)
endif()
# openMVG first, for the order of the SYSTEM include directories (see Engine/CMakeLists.txt)
target_link_libraries(Tests
    PRIVATE
        openMVG
        NatronEngine
        Qt::Core
        Python3::Python
)
target_include_directories(Tests PRIVATE ..)
target_include_directories(Tests
    SYSTEM PRIVATE
        ../libs/SequenceParsing
        google-test/include
        google-test
        google-mock/include
        google-mock
)
# bundled third-party code (see NATRON_THIRD_PARTY_NO_WARNINGS)
set_property(SOURCE google-test/src/gtest-all.cc google-mock/src/gmock-all.cc
    APPEND PROPERTY COMPILE_OPTIONS ${NATRON_THIRD_PARTY_NO_WARNINGS})
add_test(NAME Tests COMMAND Tests)
# The same OFX tests with the plug-ins known from the OFX plug-in cache that Tests wrote, instead of a
# scan: the binaries are loaded at the first use of their plug-ins (OfxHost::getPluginContextAndDescribe()).
# Tests sets up the fixture (it scans with --clear-openfx-cache and writes the cache), so
# `ctest -R TestsOfxCache` runs Tests first too.
add_test(NAME TestsOfxCache COMMAND Tests "--gtest_filter=OfxHostProperties.*:OfxTestPlugin*")
set_tests_properties(Tests PROPERTIES FIXTURES_SETUP OfxPluginCache)
set_tests_properties(TestsOfxCache PROPERTIES FIXTURES_REQUIRED OfxPluginCache ENVIRONMENT "NATRON_TESTS_KEEP_OFX_CACHE=1")

# A minimal OFX plug-in bundle for the tests (OfxTestPlugin/NatronTestPlugin.cpp, read through
# OfxTestPlugin.h). It is assembled in the build tree, found by the Tests binary through
# NATRON_TESTS_OFX_PLUGIN_DIR (added to OFX_PLUGIN_PATH by wmain.cpp), and never installed: the tests
# that need it are skipped when it is not there (e.g. when Tests is run from an installer's tree).
set(NATRON_TESTS_OFX_PLUGIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/OfxPlugins")
set(_natron_test_bundle "${NATRON_TESTS_OFX_PLUGIN_DIR}/NatronTestPlugin.ofx.bundle")
# the bundle architecture folder (OFX packaging spec), as HostSupport looks for it
if(APPLE)
    set(_natron_test_archdir "MacOS")
elseif(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_natron_test_archdir "Win64")
    else()
        set(_natron_test_archdir "Win32")
    endif()
else()
    if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        set(_natron_test_os "FreeBSD")
    else()
        set(_natron_test_os "Linux")
    endif()
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_natron_test_archdir "${_natron_test_os}-x86-64")
    else()
        set(_natron_test_archdir "${_natron_test_os}-${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endif()
# every source file of OfxTestPlugin/ is part of the bundle (see OfxTestPlugin/NatronTestPluginRegistry.h)
file(GLOB NatronTestPlugin_SOURCES CONFIGURE_DEPENDS OfxTestPlugin/*.cpp OfxTestPlugin/*.h)
add_library(NatronTestPlugin MODULE ${NatronTestPlugin_SOURCES})
target_include_directories(NatronTestPlugin SYSTEM PRIVATE ../libs/OpenFX/include)
set_target_properties(NatronTestPlugin PROPERTIES
    PREFIX ""
    SUFFIX ".ofx"
    # a single folder for all the configurations of multi-configuration generators
    LIBRARY_OUTPUT_DIRECTORY "${_natron_test_bundle}/Contents/${_natron_test_archdir}/$<0:>"
    AUTOMOC OFF # (CMAKE_AUTOMOC is on for the whole project; the plug-in doesn't use Qt)
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)
# Copies of the bundle that the host must skip (OfxPluginScan_Test.cpp, #1380): "Any bundle or
# sub-directory name starting with the character '@' is to be ignored" (ofxPackaging.rst), and a
# directory whose name only contains ".ofx.bundle" is not a bundle.
foreach(_natron_test_skipped
        "@NatronTestPluginDisabled.ofx.bundle/Contents/${_natron_test_archdir}/@NatronTestPluginDisabled.ofx"
        "@Disabled/NatronTestPluginInDisabledDir.ofx.bundle/Contents/${_natron_test_archdir}/NatronTestPluginInDisabledDir.ofx"
        "NatronTestPluginOld.ofx.bundle.old/Contents/${_natron_test_archdir}/NatronTestPluginOld.ofx")
    get_filename_component(_natron_test_skipped_dir "${NATRON_TESTS_OFX_PLUGIN_DIR}/${_natron_test_skipped}" DIRECTORY)
    add_custom_command(TARGET NatronTestPlugin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_natron_test_skipped_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:NatronTestPlugin>" "${NATRON_TESTS_OFX_PLUGIN_DIR}/${_natron_test_skipped}"
        VERBATIM)
endforeach()
add_dependencies(Tests NatronTestPlugin)
target_compile_definitions(Tests PRIVATE
    NATRON_TESTS_OFX_PLUGIN_DIR="${NATRON_TESTS_OFX_PLUGIN_DIR}"
    NATRON_TESTS_OFX_PLUGIN_BINARY="$<TARGET_FILE:NatronTestPlugin>")

# Not part of a normal install. The installer scripts install it separately
# (cmake --install <build> --component Tests) and run it from the bundled tree:
# in Natron.app/Contents/MacOS on macOS PORTABLE, in bin/ elsewhere.
install(TARGETS Tests DESTINATION "${NATRON_INSTALL_TESTSDIR}"
    COMPONENT Tests EXCLUDE_FROM_ALL)
