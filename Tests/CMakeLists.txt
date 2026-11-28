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

set(Tests_HEADERS BaseTest.h NaNGuards_FastMath.h OfxTestPlugin.h TestUtils.h)
set(Tests_SOURCES
    BaseTest.cpp
    CLArgs_Test.cpp
    CacheHolderID_Test.cpp
    Curve_Test.cpp
    DeepGraphRender_Test.cpp
    DiskCache_Test.cpp
    DocumentationServer_Test.cpp
    ExpressionDepsRenderArgs_Test.cpp
    ExpressionSetError_Test.cpp
    ExprUtils_Test.cpp
    FailedRoDRender_Test.cpp
    FileSystemModel_Test.cpp
    GenericSchedulerThread_Test.cpp
    Hash64_Test.cpp
    ImageCache_Test.cpp
    Image_Test.cpp
    InOrderRender_Test.cpp
    KnobDimensionRange_Test.cpp
    KnobDuplicate_Test.cpp
    KnobExpressionCache_Test.cpp
    KnobInvalidExpression_Test.cpp
    KnobLockOrder_Test.cpp
    KnobFile_Test.cpp
    KnobHolderCreateKnob_Test.cpp
    KnobHolderPanel_Test.cpp
    KnobSlave_Test.cpp
    KnobStringAnimation_Test.cpp
    LibTess_Test.cpp
    LoadPythonScript_Test.cpp
    Lut_Test.cpp
    Markdown_Test.cpp
    MaskChannel_Test.cpp
    MultiInputLinks_Test.cpp
    NaNGuards_FastMath.cpp
    NaNGuards_Test.cpp
    NodeCreatedCallback_Test.cpp
    NodeGroup_Test.cpp
    NodePresets_Test.cpp
    NodePreview_Test.cpp
    NoDialogs_Test.cpp
    OfxBundlePyPlugs_Test.cpp
    OfxBytesParam_Test.cpp
    OfxChoiceOrder_Test.cpp
    OfxContexts_Test.cpp
    OfxCPURenderSupport_Test.cpp
    OfxClipComponentsView_Test.cpp
    OfxClipDepth_Test.cpp
    OfxClipGetImage_Test.cpp
    OfxCustomParamInterp_Test.cpp
    OfxDrawSuite_Test.cpp
    OfxHostOverlayHandle_Test.cpp
    OfxHostProperties_Test.cpp
    OfxImageMemory_Test.cpp
    OfxInstanceChanged_Test.cpp
    OfxInstanceChangedGIL_Test.cpp
    OfxInstanceEdit_Test.cpp
    OfxMultiThreadSuite_Test.cpp
    OfxMultiplePAR_Test.cpp
    OfxParamKeys_Test.cpp
    OfxParamAnimating_Test.cpp
    OfxParamRange_Test.cpp
    OfxParamCopy_Test.cpp
    OfxOpenGLSupport_Test.cpp
    OfxOverlayInput_Test.cpp
    OfxParamInteract_Test.cpp
    OfxParamLinkedProperties_Test.cpp
    OfxParamUndo_Test.cpp
    OfxPluginLabel_Test.cpp
    OfxPluginObsolete_Test.cpp
    OfxPluginScan_Test.cpp
    OfxProjectLoad_Test.cpp
    OfxQuestion_Test.cpp
    OfxRenderClone_Test.cpp
    OfxSequenceRenderArgs_Test.cpp
    OfxSingleInstance_Test.cpp
    OfxStrChoiceParam_Test.cpp
    OfxSuiteStatus_Test.cpp
    OfxSyncPrivateData_Test.cpp
    OfxTestPlugin_Test.cpp
    OpenGLRenderFailed_Test.cpp
    OSGLContext_Test.cpp
    ParamRandom_Test.cpp
    PluginGrouping_Test.cpp
    PrecompLoad_Test.cpp
    ProcessHandler_Test.cpp
    ProjectAutoSave_Test.cpp
    PyEffectEquality_Test.cpp
    PyPlugToolset_Test.cpp
    PyExprUtilsNoise_Test.cpp
    PyParamDefault_Test.cpp
    PythonGILUnlocker_Test.cpp
    PythonScript_Test.cpp
    QHttpServer_Test.cpp
    ProjectFileFormat_Test.cpp
    PythonExpressionScope_Test.cpp
    ReadNodeInputs_Test.cpp
    ReadNodeOverlay_Test.cpp
    ReadWriteDecoderChoice_Test.cpp
    ReadWriteVersion_Test.cpp
    RenderThreadSafety_Test.cpp
    RenderThreads_Test.cpp
    RotoPaint_Test.cpp
    SequentialRender_Test.cpp
    Settings_Test.cpp
    ShuffleAutoConnect_Test.cpp
    StrUtilsFormat_Test.cpp
    LibMVTrackRegion_Test.cpp
    Tracker_Test.cpp
    TrackerHeadless_Test.cpp
    ViewerCache_Test.cpp
    ViewerTexture_Test.cpp
    WriteNodeReadBack_Test.cpp
    WriteNodeViews_Test.cpp
    wmain.cpp
)
add_executable(Tests ${Tests_HEADERS} ${Tests_SOURCES})
natron_set_main_thread_stack_size(Tests)
# NaNGuards_FastMath.cpp checks that MathUtils::isFinite()/isNaN() still work in code
# compiled with -ffast-math, which assumes that NaN and infinity never happen.
if(MSVC) # MSVC and clang-cl
    set_source_files_properties(NaNGuards_FastMath.cpp PROPERTIES COMPILE_OPTIONS "/fp:fast")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(NaNGuards_FastMath.cpp PROPERTIES
        COMPILE_OPTIONS "-ffast-math"
        COMPILE_DEFINITIONS NATRON_TESTS_FAST_MATH_TU)
endif()
# openMVG and mv first, for the order of the SYSTEM include directories (see Engine/CMakeLists.txt)
target_link_libraries(Tests
    PRIVATE
        openMVG
        mv
        NatronEngine
        qhttpserver
        Qt::Core
        Qt::Network
        Python3::Python
)
target_include_directories(Tests PRIVATE ..)
target_include_directories(Tests
    SYSTEM PRIVATE
        ../libs/SequenceParsing
        ../libs/qhttpserver/src
        ../libs/qhttpserver/http-parser
)
# Google Test: an installed googletest, found with its CMake config (1.14, Ubuntu 24.04's, is the
# oldest version that CI builds with). Only GTest::gtest is linked: no test uses Google Mock, and
# main() is wmain.cpp's (not GTest::gtest_main).
find_package(GTest 1.14 CONFIG QUIET)
if(NOT GTest_FOUND)
    if(GTest_CONSIDERED_VERSIONS)
        set(_natron_gtest_found " (found ${GTest_CONSIDERED_VERSIONS}: older, or for another architecture)")
    endif()
    message(FATAL_ERROR "Tests: Google Test 1.14 or later (googletest, with its CMake config) was not found${_natron_gtest_found}. "
        "Install it (Debian/Ubuntu: libgtest-dev; Fedora, RHEL 10 (EPEL): gtest-devel; macOS: the MacPorts port gtest; "
        "MSYS2: mingw-w64-ucrt-x86_64-gtest, or mingw-w64-clang-aarch64-gtest on CLANGARM64; FreeBSD: googletest), "
        "add its prefix to CMAKE_PREFIX_PATH or set GTest_DIR, or configure with -DNATRON_BUILD_TESTS=OFF.")
endif()
message(STATUS "Tests: Google Test ${GTest_VERSION} (${GTest_DIR})")
target_link_libraries(Tests PRIVATE GTest::gtest)
add_test(NAME Tests COMMAND Tests)
# The same OFX tests with the plug-ins known from the OFX plug-in cache that Tests wrote, instead of a
# scan: the binaries are loaded at the first use of their plug-ins (OfxHost::getPluginContextAndDescribe()).
# Tests sets up the fixture (it scans with --clear-openfx-cache and writes the cache), so
# `ctest -R TestsOfxCache` runs Tests first too.
add_test(NAME TestsOfxCache COMMAND Tests "--gtest_filter=OfxHostProperties.*:OfxTestPlugin*:OfxPluginObsolete.*:OfxBundlePyPlugs.*")
# The caches of both (the same OFX plug-in cache) in the build tree, not in the user's cache directory, which
# every build of this version shares (#1569). NATRON_DISK_CACHE_PATH, if set, takes precedence (see wmain.cpp).
set(NATRON_TESTS_CACHE_DIR "${CMAKE_CURRENT_BINARY_DIR}/tests-cache")
# A hung test fails after 30 minutes (the whole binary takes 1 to 10 minutes on CI), with ctest's "Timeout"
# and the name of the running test in the log, instead of running until the CI job's time limit.
set_tests_properties(Tests PROPERTIES FIXTURES_SETUP OfxPluginCache TIMEOUT 1800
    ENVIRONMENT "NATRON_TESTS_CACHE_DIR=${NATRON_TESTS_CACHE_DIR}")
set_tests_properties(TestsOfxCache PROPERTIES FIXTURES_REQUIRED OfxPluginCache TIMEOUT 600
    ENVIRONMENT "NATRON_TESTS_KEEP_OFX_CACHE=1;NATRON_TESTS_CACHE_DIR=${NATRON_TESTS_CACHE_DIR}")
# A copy of that cache, made unreadable: Natron discards it, scans all the plug-ins again and rewrites it
# (see OfxCacheCorrupt.cmake).
add_test(NAME TestsOfxCacheCorrupt COMMAND ${CMAKE_COMMAND}
    "-DTESTS=$<TARGET_FILE:Tests>"
    "-DSRC_CACHE_DIR=${NATRON_TESTS_CACHE_DIR}"
    "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/tests-cache-corrupt"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/OfxCacheCorrupt.cmake")
set_tests_properties(TestsOfxCacheCorrupt PROPERTIES FIXTURES_REQUIRED OfxPluginCache)

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
    # native Windows on ARM64 (MSYS2 CLANGARM64, #1494): HostSupport looks in Win-arm64, then
    # Win-arm64x (ofxhArchDirs.h), never in Win64.
    # Use the compiler's target, as libs/OpenFX/cmake/OpenFX.cmake does, not CMAKE_SYSTEM_PROCESSOR:
    # that is the build machine's processor (AMD64 under an x64-emulated shell on an ARM64 host, ARM64
    # for a UCRT64 build on an ARM64 host). CMAKE_CXX_COMPILER_ARCHITECTURE_ID is only set for MinGW
    # from CMake 4.1 on ("aarch64"); before that, ask the compiler (`-dumpmachine`).
    set(_natron_test_target "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
    if(NOT _natron_test_target)
        set(_natron_test_target "${CMAKE_CXX_COMPILER_TARGET}")
    endif()
    if(NOT _natron_test_target)
        separate_arguments(_natron_test_cxx_args UNIX_COMMAND "${CMAKE_CXX_COMPILER_ARG1}")
        execute_process(COMMAND ${CMAKE_CXX_COMPILER} ${_natron_test_cxx_args} -dumpmachine
                        OUTPUT_VARIABLE _natron_test_target OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    endif()
    string(TOLOWER "${_natron_test_target}" _natron_test_target)
    if(_natron_test_target MATCHES "^(aarch64|arm64)" AND NOT _natron_test_target MATCHES "^arm64ec")
        set(_natron_test_archdir "Win-arm64")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
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
# openfx::Api: the OpenFX headers and the OFX_EXTENSIONS_*/OFX_SUPPORTS_* definitions that Natron is
# built with (e.g. the reader and writer contexts of OfxTestPlugin/NatronTestPluginVersions.cpp, only
# defined with OFX_EXTENSIONS_TUTTLE), as global.pri gives them to OfxTestPlugin/OfxTestPlugin.pro
target_link_libraries(NatronTestPlugin PRIVATE openfx::Api)
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
# A PyPlug that ships in the bundle, with its icon (OfxBundlePyPlugs_Test.cpp)
set(_natron_test_bundle_pyplugs
    "${CMAKE_CURRENT_SOURCE_DIR}/data/ofx-bundle-pyplugs/NatronTestBundlePyPlug.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/data/ofx-bundle-pyplugs/NatronTestBundlePyPlug.png")
add_custom_command(TARGET NatronTestPlugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_natron_test_bundle}/Contents/Resources/PyPlugs"
    COMMAND ${CMAKE_COMMAND} -E copy ${_natron_test_bundle_pyplugs} "${_natron_test_bundle}/Contents/Resources/PyPlugs/"
    VERBATIM)
add_dependencies(Tests NatronTestPlugin)
# The same tests with NATRON_PLUGIN_PATH set to a directory with a PyPlug of the same ID and version as the bundle's:
# that one is loaded instead (OfxBundlePyPlugs_Test.cpp)
add_test(NAME TestsOfxBundlePyPlugOverride COMMAND Tests "--gtest_filter=OfxBundlePyPlugs.*")
set_tests_properties(TestsOfxBundlePyPlugOverride PROPERTIES
    ENVIRONMENT "NATRON_PLUGIN_PATH=${CMAKE_CURRENT_SOURCE_DIR}/data/ofx-bundle-pyplugs-override;PYTHONDONTWRITEBYTECODE=1;NATRON_TESTS_CACHE_DIR=${CMAKE_CURRENT_BINARY_DIR}/tests-cache-pyplug-override")
target_compile_definitions(Tests PRIVATE
    NATRON_TESTS_OFX_PLUGIN_DIR="${NATRON_TESTS_OFX_PLUGIN_DIR}"
    NATRON_TESTS_OFX_PLUGIN_BINARY="$<TARGET_FILE:NatronTestPlugin>"
    NATRON_TESTS_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    NATRON_MAIN_THREAD_STACK_SIZE=${NATRON_MAIN_THREAD_STACK_SIZE})

# GUI smoke test (see gui/run_gui_smoke_test.py): runs the Natron GUI with --no-dialogs.
# It is skipped (exit status 77) without a display, in CI unless NATRON_GUI_TESTS=1, or
# without the Read and Transform plug-ins (set OFX_PLUGIN_PATH as for the Tests binary).
if(Python3_Interpreter_FOUND)
    add_test(NAME GuiSmokeTest
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gui/run_gui_smoke_test.py $<TARGET_FILE:Natron>)
    set_tests_properties(GuiSmokeTest PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 960 LABELS gui)
    # NatronRenderer exits with status 1 when the render of a Write node failed
    add_test(NAME RenderFailureStatusTest
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gui/run_render_failure_status_test.py $<TARGET_FILE:NatronRenderer>)
    set_tests_properties(RenderFailureStatusTest PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 660)
    # natron.quit() from a render callback stops a NatronRenderer render (headless)
    add_test(NAME RendererQuitTest
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gui/run_renderer_quit_test.py $<TARGET_FILE:NatronRenderer>)
    set_tests_properties(RendererQuitTest PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 660)
    # sys.exit() in a NatronRenderer script, or at the -t prompt, unloads the OFX plug-ins once (#1557)
    add_test(NAME RendererExitUnloadTest
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gui/run_renderer_exit_unload_test.py
                $<TARGET_FILE:NatronRenderer> "${NATRON_TESTS_OFX_PLUGIN_DIR}")
    set_tests_properties(RendererExitUnloadTest PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 660)
    # a -t script presses a Write render button, or calls app.render(), with Python render callbacks (#1568)
    add_test(NAME RendererCallbacksTest
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gui/run_renderer_callbacks_test.py $<TARGET_FILE:NatronRenderer>)
    set_tests_properties(RendererCallbacksTest PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 660)
    # a Read whose file is missing ("On missing frame: Error") or whose filename is empty fails the render (exit status 1), instead of writing black (#1602, #1131)
    add_test(NAME RendererMissingReadTest
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gui/run_renderer_missing_read_test.py $<TARGET_FILE:NatronRenderer>)
    set_tests_properties(RendererMissingReadTest PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 660)
    # NatronRenderer project.ntp -i Read <sequence> -w Write <file> <range>, on a project whose Read and Write have no file, renders the right frames (#864)
    add_test(NAME RendererCLReadWriteTest
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gui/run_renderer_cl_read_write_test.py $<TARGET_FILE:NatronRenderer>)
    set_tests_properties(RendererCLReadWriteTest PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 660)
    # short version of the render soak test (#248): four NatronRenderer runs (several Write nodes, a sequential
    # plug-in, WriteFFmpeg, renders from Python, an abort and a restart) must not stall; longer soaks:
    # .github/workflows/render_soak.yml
    add_test(NAME RenderSoakTest
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gui/run_render_soak_test.py $<TARGET_FILE:NatronRenderer>
                --frames 60 --stall-timeout 120 --configs default,1thread,parallel2,lowram
                --ofx-path "${NATRON_TESTS_OFX_PLUGIN_DIR}")
    set_tests_properties(RenderSoakTest PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 900)
    # --setting options for the caches and threads, which run before the caches exist (#1571)
    add_test(NAME RendererSettingsTest
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gui/run_renderer_settings_test.py $<TARGET_FILE:NatronRenderer>)
    set_tests_properties(RendererSettingsTest PROPERTIES TIMEOUT 360)
    # App.projectFileHasGuiData() and App.projectLoadedWithErrors() in NatronRenderer
    add_test(NAME ProjectLoadInfoTest
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gui/run_project_load_info_test.py $<TARGET_FILE:NatronRenderer>)
    set_tests_properties(ProjectLoadInfoTest PROPERTIES TIMEOUT 360)
    # NatronRenderer keeps the GUI data (node positions, sizes, colours, Viewers, window layout) of the projects it saves
    add_test(NAME ProjectGuiDataTest
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gui/run_project_gui_data_test.py $<TARGET_FILE:NatronRenderer>)
    set_tests_properties(ProjectGuiDataTest PROPERTIES TIMEOUT 360)
    # the natron_convert_project module (tools/): text -> binary -> text and text -> python -> text with NatronRenderer -c, same nodes and values
    add_test(NAME ConvertProjectTest
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gui/run_convert_project_test.py $<TARGET_FILE:NatronRenderer>)
    set_tests_properties(ConvertProjectTest PROPERTIES TIMEOUT 600)
    # App.saveProjectAsPython(), and loading its file with app.loadProject() and from the command line (#1669)
    add_test(NAME ProjectPythonTest
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gui/run_project_python_test.py $<TARGET_FILE:NatronRenderer>)
    set_tests_properties(ProjectPythonTest PROPERTIES TIMEOUT 400)
    # a project saved as Python keeps everything the .ntp keeps: .ntp -> .py -> .ntp, on the built-in projects (#2001;
    # all of Natron-Tests: .github/workflows/project_python_roundtrip.yml)
    add_test(NAME NtpPythonRoundTripTest
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gui/run_ntp_python_roundtrip_test.py $<TARGET_FILE:NatronRenderer>)
    set_tests_properties(NtpPythonRoundTripTest PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
endif()
if(Python3_Interpreter_FOUND)
    # NatronRenderer -t exits at the end of a stdin that is not a console (the null device, a pipe)
    add_test(NAME InterpreterStdinEofTest
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gui/run_interpreter_stdin_eof_test.py $<TARGET_FILE:NatronRenderer>)
    set_tests_properties(InterpreterStdinEofTest PROPERTIES TIMEOUT 300)
endif()

# Not part of a normal install. The installer scripts install it separately
# (cmake --install <build> --component Tests) and run it from the bundled tree:
# in Natron.app/Contents/MacOS on macOS PORTABLE, in bin/ elsewhere.
install(TARGETS Tests DESTINATION "${NATRON_INSTALL_TESTSDIR}"
    COMPONENT Tests EXCLUDE_FROM_ALL)

# GUI tests (Tests/gui/test_*.py, see Documentation/source/devel/guitesting.rst): each script is run by
# "Natron --test script.py", which exits with 0 (pass), 77 (skipped, e.g. without OpenGL or without a
# plug-in that the test needs) or another status (fail). They need a display (Xvfb on Linux), and
# have the "gui" label: "ctest -L gui" runs only them, "ctest -LE gui" all the other tests; the
# interactive ones also have the "gui-interactive" label.
# The Natron target is defined later (App/), which generator expressions allow.
# They are skipped (77) without a display on Linux; -DNATRON_BUILD_GUI_TESTS=OFF does not register them (not to be confused with the environment variable NATRON_GUI_TESTS, which enables them in CI).
option(NATRON_BUILD_GUI_TESTS "register the GUI tests (Tests/gui/test_*.py, Natron --test) in ctest" ON)
if(NATRON_BUILD_GUI_TESTS)
    file(GLOB NatronGuiTests_SCRIPTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/gui/test_*.py")
else()
    set(NatronGuiTests_SCRIPTS)
endif()
foreach(_natron_gui_test ${NatronGuiTests_SCRIPTS})
    get_filename_component(_natron_gui_test_name "${_natron_gui_test}" NAME_WE)
    string(REGEX REPLACE "^test_" "" _natron_gui_test_name "${_natron_gui_test_name}")
    set(_natron_gui_test_target "gui.${_natron_gui_test_name}")
    add_test(NAME ${_natron_gui_test_target} COMMAND Natron --test "${_natron_gui_test}")
    # A test that needs the real focus or mouse calls NatronGuiTest.requireInteractive() (it is skipped
    # unless NATRON_TEST_INTERACTIVE=1) and has the line "# NatronGuiTest: interactive": label gui-interactive.
    file(STRINGS "${_natron_gui_test}" _natron_gui_test_interactive REGEX "^# NatronGuiTest: interactive")
    if(_natron_gui_test_interactive)
        set(_natron_gui_test_labels "gui;gui-interactive")
    else()
        set(_natron_gui_test_labels "gui")
    endif()
    set_tests_properties(${_natron_gui_test_target} PROPERTIES
        LABELS "${_natron_gui_test_labels}"
        SKIP_RETURN_CODE 77
        TIMEOUT 180
        # one Natron GUI at a time
        RESOURCE_LOCK NatronGui
        # the caches (images, OFX plug-ins) in the build tree, not in the user's cache directory
        ENVIRONMENT "NATRON_TESTS_OFX_PLUGIN_BINARY=$<TARGET_FILE:NatronTestPlugin>;NATRON_DISK_CACHE_PATH=${CMAKE_CURRENT_BINARY_DIR}/gui-tests-cache")
    # the test plug-ins first, then the user's OFX_PLUGIN_PATH (e.g. openfx-misc for the Transform test)
    if(NOT CMAKE_VERSION VERSION_LESS 3.22)
        set_property(TEST ${_natron_gui_test_target} PROPERTY ENVIRONMENT_MODIFICATION
            "OFX_PLUGIN_PATH=path_list_prepend:${NATRON_TESTS_OFX_PLUGIN_DIR}")
    else()
        # the user's OFX_PLUGIN_PATH at configure time
        if(WIN32)
            set(_natron_gui_test_ofx_path "${NATRON_TESTS_OFX_PLUGIN_DIR}\\;$ENV{OFX_PLUGIN_PATH}")
        else()
            set(_natron_gui_test_ofx_path "${NATRON_TESTS_OFX_PLUGIN_DIR}:$ENV{OFX_PLUGIN_PATH}")
        endif()
        set_property(TEST ${_natron_gui_test_target} APPEND PROPERTY ENVIRONMENT
            "OFX_PLUGIN_PATH=${_natron_gui_test_ofx_path}")
    endif()
endforeach()
# the PyPlug that test_pyplug_export_version.py exports (#1527)
if(TEST gui.pyplug_export_version)
    set_property(TEST gui.pyplug_export_version APPEND PROPERTY ENVIRONMENT
        "NATRON_PLUGIN_PATH=${CMAKE_CURRENT_SOURCE_DIR}/data/pyplugs;PYTHONDONTWRITEBYTECODE=1")
endif()

# Headless tests (Tests/gui/headless_*.py, see gui/natronheadless.py): "NatronRenderer -t script.py" runs each
# script, which exits with 0 (pass), 77 (skipped, e.g. without a plug-in that the test needs) or 1 (fail). The
# docstring of each script says what it checks. ctest test headless.<name> (label "headless") runs
# headless_<name>.py, with the caches (images, OFX plug-ins) in the build tree, not in the user's cache
# directory (#1569).
file(GLOB NatronHeadlessTests_SCRIPTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/gui/headless_*.py")
foreach(_natron_headless_test ${NatronHeadlessTests_SCRIPTS})
    get_filename_component(_natron_headless_test_name "${_natron_headless_test}" NAME_WE)
    string(REGEX REPLACE "^headless_" "" _natron_headless_test_name "${_natron_headless_test_name}")
    set(_natron_headless_test_target "headless.${_natron_headless_test_name}")
    add_test(NAME ${_natron_headless_test_target} COMMAND NatronRenderer -t "${_natron_headless_test}")
    set_tests_properties(${_natron_headless_test_target} PROPERTIES
        LABELS headless
        SKIP_RETURN_CODE 77
        TIMEOUT 120
        ENVIRONMENT "NATRON_DISK_CACHE_PATH=${CMAKE_CURRENT_BINARY_DIR}/headless-tests-cache/${_natron_headless_test_name}")
endforeach()
# thousands of nodes; several renders
set_tests_properties(headless.node_naming headless.layer_passthrough headless.writer_pass_through PROPERTIES
    TIMEOUT 300)
# the PyPlugs of the tests
set_property(TEST headless.pyplug_rebound_app headless.unsaved_project_path APPEND PROPERTY ENVIRONMENT
    "NATRON_PLUGIN_PATH=${CMAKE_CURRENT_SOURCE_DIR}/data/pyplugs;PYTHONDONTWRITEBYTECODE=1")
