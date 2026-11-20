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

// Regression test for https://github.com/NatronGitHub/Natron/issues/1276: AppInstance::loadPythonScript()
// returned true when a script (without createInstance()) raised an exception, so NatronRenderer
// rendered and exited with status 0 after a Python error.

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
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QtCore/QTemporaryDir>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"

NATRON_NAMESPACE_USING

namespace {
// Writes a script into dir and returns its file info.
QFileInfo
writeScript(const QTemporaryDir& dir,
            const char* name,
            const char* content)
{
    QString path = QDir( dir.path() ).filePath( QString::fromUtf8(name) );
    QFile f(path);

    EXPECT_TRUE( f.open(QIODevice::WriteOnly | QIODevice::Truncate) );
    f.write( QByteArray(content) );
    f.close();

    return QFileInfo(path);
}
} // anonymous namespace

TEST(LoadPythonScript, ReturnValue)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ASSERT_TRUE( bool(app) );
    QTemporaryDir dir;
    ASSERT_TRUE( dir.isValid() );

    // A plain script, run in __main__
    EXPECT_TRUE( app->loadPythonScript( writeScript(dir, "natronTest1276Ok.py", "natronTest1276Value = 1\n") ) );
    EXPECT_FALSE( app->loadPythonScript( writeScript(dir, "natronTest1276Raise.py",
                                                     "print('natronTest1276Raise.py: the RuntimeError below is expected')\n"
                                                     "raise RuntimeError('expected error, see issue 1276')\n") ) )
        << "a script that raises an exception must fail";

    // A script with createInstance(), imported as a module
    EXPECT_TRUE( app->loadPythonScript( writeScript(dir, "natronTest1276CreateOk.py",
                                                    "def createInstance(app, group):\n"
                                                    "    pass\n") ) );
    EXPECT_FALSE( app->loadPythonScript( writeScript(dir, "natronTest1276CreateRaise.py",
                                                     "def createInstance(app, group):\n"
                                                     "    print('natronTest1276CreateRaise.py: the RuntimeError below is expected')\n"
                                                     "    raise RuntimeError('expected error, see issue 1276')\n") ) );

    // Clean up what the scripts left in Python: the directory added to sys.path, the imported modules, the variable
    std::string err;
    const std::string dirPath = dir.path().toStdString();
    const std::string cleanup =
        "import sys, __main__\n"
        "while '" + dirPath + "' in sys.path:\n"
        "    sys.path.remove('" + dirPath + "')\n"
        "for m in [m for m in sys.modules if m.startswith('natronTest1276')]:\n"
        "    del sys.modules[m]\n"
        "__main__.__dict__.pop('natronTest1276Value', None)\n"
        "assert not [m for m in sys.modules if m.startswith('natronTest1276')]\n";
    EXPECT_TRUE( NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript(cleanup, &err, 0) ) << err;
}
