/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
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

#ifndef NATRON_TESTS_OFXTESTPLUGIN_NATRONTESTPLUGINREGISTRY_H
#define NATRON_TESTS_OFXTESTPLUGIN_NATRONTESTPLUGINREGISTRY_H

// The test plug-in bundle (NatronTestPlugin.cpp) can hold plug-ins defined in other source files of
// the bundle, so that each group of tests keeps its plug-ins in its own file. Such a file defines its
// OfxPlugin structures and registers them with a static NatronTestPluginRegistrar; the bundle lists
// them after its own plug-ins. They count events with natronTestIncrementCounter(), which the tests
// read with NatronTestPlugin::counter() (Tests/OfxTestPlugin.h).

#include <string>
#include <vector>

#include <ofxCore.h>

// the plug-ins registered by the other source files of the bundle
std::vector<OfxPlugin*>& natronTestExtraPlugins();

struct NatronTestPluginRegistrar
{
    explicit NatronTestPluginRegistrar(OfxPlugin* plugin)
    {
        natronTestExtraPlugins().push_back(plugin);
    }
};

// add delta to the named counter of the bundle (thread-safe)
void natronTestIncrementCounter(const std::string& name, int delta = 1);

#endif // NATRON_TESTS_OFXTESTPLUGIN_NATRONTESTPLUGINREGISTRY_H
