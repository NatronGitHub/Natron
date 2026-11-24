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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"

NATRON_NAMESPACE_USING

// IntParam, DoubleParam and ColorParam getDefaultValue(dimension) read past the end of the default
// values for a dimension out of range (#1743). Like getValue(dimension), they now return 0.
TEST(PyParamDefault, DimensionOutOfRange)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ASSERT_TRUE( bool(app) );
    const std::string a = app->getAppIDString();
    const std::string script =
        "g = " + a + ".createNode('fr.inria.built-in.Group')\n"
        "i = g.createIntParam('natronTestDefInt', 'Int')\n"
        "i.setDefaultValue(3, 0)\n"
        "d = g.createDoubleParam('natronTestDefDouble', 'Double')\n"
        "d.setDefaultValue(0.5, 0)\n"
        "c = g.createColorParam('natronTestDefColor', 'Color', True)\n"
        "c.setDefaultValue(0.25, 3)\n"
        "values = (i.getDefaultValue(0), i.getDefaultValue(1), i.getDefaultValue(-1),\n"
        "          d.getDefaultValue(0), d.getDefaultValue(1), d.getDefaultValue(-1),\n"
        "          c.getDefaultValue(3), c.getDefaultValue(4), c.getDefaultValue(-1))\n"
        "g.destroy()\n"
        "del g, i, d, c\n"
        "assert values == (3, 0, 0, 0.5, 0., 0., 0.25, 0., 0.), values\n"
        "del values\n";
    std::string error, output;
    EXPECT_TRUE( NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &error, &output) ) << error << output;
}
