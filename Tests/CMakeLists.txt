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

set(Tests_HEADERS BaseTest.h)
set(Tests_SOURCES
    google-test/src/gtest-all.cc
    google-mock/src/gmock-all.cc
    BaseTest.cpp
    Curve_Test.cpp
    FileSystemModel_Test.cpp
    Hash64_Test.cpp
    Image_Test.cpp
    KnobFile_Test.cpp
    KnobSlave_Test.cpp
    KnobStringAnimation_Test.cpp
    Lut_Test.cpp
    NodeGroup_Test.cpp
    NodePreview_Test.cpp
    NoDialogs_Test.cpp
    OSGLContext_Test.cpp
    PluginGrouping_Test.cpp
    ReadNodeOverlay_Test.cpp
    RotoPaint_Test.cpp
    Tracker_Test.cpp
    wmain.cpp
)
add_executable(Tests ${Tests_HEADERS} ${Tests_SOURCES})
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

# GUI smoke test (see gui/run_gui_smoke_test.py): runs the Natron GUI with --no-dialogs.
# It is skipped (exit status 77) without a display, in CI unless NATRON_GUI_TESTS=1, or
# without the Read and Transform plug-ins (set OFX_PLUGIN_PATH as for the Tests binary).
if(Python3_Interpreter_FOUND)
    add_test(NAME GuiSmokeTest
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gui/run_gui_smoke_test.py $<TARGET_FILE:Natron>)
    set_tests_properties(GuiSmokeTest PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 960)
    # natron.quit() from a render callback stops a NatronRenderer render (headless)
    add_test(NAME RendererQuitTest
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gui/run_renderer_quit_test.py $<TARGET_FILE:NatronRenderer>)
    set_tests_properties(RendererQuitTest PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 660)
endif()

# Not part of a normal install. The installer scripts install it separately
# (cmake --install <build> --component Tests) and run it from the bundled tree:
# in Natron.app/Contents/MacOS on macOS PORTABLE, in bin/ elsewhere.
install(TARGETS Tests DESTINATION "${NATRON_INSTALL_TESTSDIR}"
    COMPONENT Tests EXCLUDE_FROM_ALL)
