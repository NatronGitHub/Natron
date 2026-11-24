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

// A PyPlug is a toolset when its getIsToolset() returns a true value, not only the bool True (#1528): the
// result was left uninitialised for any other value (1, None...), so the PyPlug was a toolset or not at random.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include <gtest/gtest.h>

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtCore/QTemporaryDir>

#include "Engine/AppManager.h"

NATRON_NAMESPACE_USING

namespace {
// Writes the PyPlug <moduleName>.py into dir, with getIsToolset() returning isToolsetExpr.
void
writePyPlug(const QTemporaryDir& dir,
            const std::string& moduleName,
            const char* isToolsetExpr)
{
    QFile f( QDir( dir.path() ).filePath( QString::fromUtf8( (moduleName + ".py").c_str() ) ) );

    ASSERT_TRUE( f.open(QIODevice::WriteOnly | QIODevice::Truncate) );
    std::string content = "def getPluginID():\n"
                          "    return 'net.natron.test." + moduleName + "'\n"
                          "def getLabel():\n"
                          "    return '" + moduleName + "'\n"
                          "def getIsToolset():\n"
                          "    return " + isToolsetExpr + "\n"
                          "def createInstance(app, group):\n"
                          "    pass\n";
    f.write( QByteArray( content.c_str() ) );
}

// What getGroupInfos() says about getIsToolset(); the output starts with the other value, as an
// uninitialised variable could
bool
isToolset(const QTemporaryDir& dir,
          const std::string& moduleName,
          bool initialValue)
{
    std::string pluginID, pluginLabel, iconFilePath, grouping, description;
    unsigned int version = 0;
    bool ret = initialValue;
    const std::string modulePath = QDir( dir.path() ).absolutePath().toStdString() + '/';

    EXPECT_TRUE( NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::getGroupInfos(modulePath, moduleName, &pluginID, &pluginLabel, &iconFilePath,
                                                        &grouping, &description, &ret, &version) ) << moduleName;
    EXPECT_EQ("net.natron.test." + moduleName, pluginID);

    return ret;
}
} // anonymous namespace

TEST(PyPlugToolset, GetIsToolsetTruthValue)
{
    QTemporaryDir dir;
    ASSERT_TRUE( dir.isValid() );
    const std::string dirPath = QDir( dir.path() ).absolutePath().toStdString();
    std::string err;
    ASSERT_TRUE( NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("import sys\nsys.path.append('" + dirPath + "')\n", &err, 0) ) << err;

    const struct
    {
        const char* name;
        const char* expr;
        bool expected;
    } cases[] = {
        { "natronTest1528True", "True", true },
        { "natronTest1528False", "False", false },
        { "natronTest1528One", "1", true },
        { "natronTest1528Zero", "0", false },
        { "natronTest1528None", "None", false },
        { "natronTest1528String", "'yes'", true },
    };
    // all the files first: the import system caches the directory listing, which a file written in the same
    // clock tick as the previous import may not be in
    for (const auto& c : cases) {
        writePyPlug(dir, c.name, c.expr);
    }
    ASSERT_TRUE( NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("import importlib\nimportlib.invalidate_caches()\n", &err, 0) ) << err;
    for (const auto& c : cases) {
        EXPECT_EQ( c.expected, isToolset(dir, c.name, !c.expected) ) << c.expr;
    }

    // Clean up what the PyPlugs left in Python: the directory added to sys.path, the imported modules
    const std::string cleanup =
        "import sys\n"
        "while '" + dirPath + "' in sys.path:\n"
        "    sys.path.remove('" + dirPath + "')\n"
        "for m in [m for m in sys.modules if m.startswith('natronTest1528')]:\n"
        "    del sys.modules[m]\n"
        "import __main__\n"
        "assert not hasattr(__main__, 'isToolset')\n";
    EXPECT_TRUE( NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript(cleanup, &err, 0) ) << err;
}
