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

#include <gtest/gtest.h>

#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtCore/QString>
#include <QtCore/QTemporaryDir>

#include "Engine/AppManager.h"
#include "Engine/Settings.h"

NATRON_NAMESPACE_USING

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
namespace {
// Puts the native settings back at Qt's default location when destroyed, also after a failed ASSERT
class NativeSettingsPathRestorer
{
public:
    NativeSettingsPathRestorer() = default;
    NativeSettingsPathRestorer(const NativeSettingsPathRestorer&) = delete;
    NativeSettingsPathRestorer& operator=(const NativeSettingsPathRestorer&) = delete;

    ~NativeSettingsPathRestorer()
    {
        // Qt's default on XDG platforms (~/.qttest/config in the Tests binary, which enables the
        // QStandardPaths test mode, see wmain.cpp)
        QSettings::setPath( QSettings::NativeFormat, QSettings::UserScope,
                            QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) );
    }
};
} // anonymous namespace
#endif

// #1728: with --no-settings (which the Tests binary uses, see wmain.cpp), Restore defaults resets the
// settings in memory and leaves the settings file alone.
TEST(Settings, RestoreDefaultKeepsSettingsFileWithNoSettings)
{
    ASSERT_TRUE( appPTR );
    SettingsPtr settings = appPTR->getCurrentSettings();
    ASSERT_TRUE( bool(settings) );
    ASSERT_TRUE( settings->isUsingDefaultSettings() );
    ASSERT_FALSE( settings->getSaveSettings() );

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    // On Linux and the BSDs, the native settings are a file, which the test redirects to a temporary
    // directory (on macOS and Windows they are in the system's preferences, which the test doesn't touch)
    QTemporaryDir configDir;
    ASSERT_TRUE( configDir.isValid() );
    QSettings::setPath( QSettings::NativeFormat, QSettings::UserScope, configDir.path() );
    NativeSettingsPathRestorer pathRestorer;
    QString fileName;
    {
        QSettings qsettings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );
        qsettings.setValue( QString::fromUtf8("NatronTestsMarker"), 1 );
        qsettings.sync();
        fileName = qsettings.fileName();
    }
    ASSERT_TRUE( fileName.startsWith( configDir.path() ) );
    ASSERT_TRUE( QFile::exists(fileName) );
#endif

    settings->restoreDefault();

    // saving stays off: a later change of a preference doesn't write the settings file either
    EXPECT_FALSE( settings->getSaveSettings() );
    settings->setSaveSettings(false); // if the check failed: the next tests must not write the user's settings

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    EXPECT_TRUE( QFile::exists(fileName) );
#endif
}
