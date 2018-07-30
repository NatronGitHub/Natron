/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_LOADKNOBSCOMPAT_H
#define NATRON_LOADKNOBSCOMPAT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <string>
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/*
 * @brief Maps a knob name that existed in a specific version of Natron to the actual knob name in the current version.
 * @param pluginID The plugin ID of the plug-in being loaded.
 * @param natronVersionMajor/Minor/Revision The version of Natron that saved the project being loaded. This is -1 if the version of Natron could not be determined.
 * @param pluginVersionMajor/Minor The version of the plug-in that saved the project being loaded. This is -1 if the version of the plug-in could not be determined.
 * @param name[in][out] In input and output the name of the knob which may be modified by this function
 * @returns true if a filter was applied, false otherwise
 */
bool filterKnobNameCompat(const std::string& pluginID,
                          int pluginVersionMajor, int pluginVersionMinor,
                          int natronVersionMajor, int natronVersionMinor, int natronVersionRevision,
                          std::string* name);

/*
 * @brief Maps a KnobChoice option ID that existed in a specific version of Natron to the actual option in the current version.
 * @param pluginID The plugin ID of the plug-in being loaded
 * @param natronVersionMajor/Minor/Revision The version of Natron that saved the project being loaded
 * @param pluginVersionMajor/Minor The version of the plug-in that saved the project being loaded
 * @param paramName The name of the parameter being loaded. Note that this is the name of the parameter in the saved project, prior to being passed to filterKnobNameCompat
 * dynamic choice parameters: we used to store the value in a string parameter with the suffix "Choice".
 * @param optionID[in][out] In input and output the optionID which may be modified by this function
 * @returns true if a filter was applied, false otherwise
 */
bool filterKnobChoiceOptionCompat(const std::string& pluginID,
                                  int pluginVersionMajor, int pluginVersionMinor,
                                  int natronVersionMajor, int natronVersionMinor, int natronVersionRevision,
                                  const std::string& paramName,
                                  std::string* optionID);

NATRON_NAMESPACE_EXIT

#endif // NATRON_LOADKNOBSCOMPAT_H
