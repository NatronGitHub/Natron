/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#ifndef Gui_GuiTest_h
#define Gui_GuiTest_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <QtCore/QString>


NATRON_NAMESPACE_ENTER

/**
 * @brief GUI test mode (Natron --test script.py).
 *
 * The test-only Python module NatronGuiTest (synthetic events, idle wait, dialog answers) is
 * only available in test mode, so that the public Python API does not grow test hooks.
 * See Documentation/source/devel/guiTests.rst.
 **/
namespace GuiTest {
/**
 * @brief Registers the NatronGuiTest built-in module, and starts watching the modal dialogs (an unexpected
 * one fails the test). Must be called before Python is initialized, once qApp exists.
 **/
void install();

/**
 * @brief Runs the test script once the event loop processes events, then quits the application.
 * The exit status of the test is stored with AppManager::setTestExitStatus().
 **/
void scheduleTestScript(const QString& path);

} // namespace GuiTest

NATRON_NAMESPACE_EXIT

#endif // Gui_GuiTest_h
