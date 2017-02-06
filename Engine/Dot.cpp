/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Dot.h"

#include <cassert>
#include <stdexcept>

NATRON_NAMESPACE_ENTER;

PluginPtr
Dot::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_OTHER);
    PluginPtr ret = Plugin::create((void*)Dot::create, (void*)Dot::createRenderClone, PLUGINID_NATRON_DOT, "Dot", 1, 0, grouping);

    QString desc = tr("Doesn't do anything to the input image, this is used in the node graph to make bends in the links.");
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    ret->setProperty<int>(kNatronPluginPropRenderSafety, (int)eRenderSafetyFullySafe);
    ret->setProperty<int>(kNatronPluginPropShortcut, (int)Key_period);
    ret->setProperty<int>(kNatronPluginPropShortcut, (int)eKeyboardModifierShift, 1);
    ret->setProperty<std::string>(kNatronPluginPropIconFilePath,  "Images/dot_icon.png");
    return ret;
}


NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING
#include "moc_Dot.cpp"
