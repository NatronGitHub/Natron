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

// Regression test for #1258: clearing the caches must not delete the auto-saves (the crash-recovery
// files of the unsaved projects).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>

#include "Engine/AppManager.h"
#include "Engine/Project.h"

NATRON_NAMESPACE_USING

namespace {
// Creates an auto-save file of an untitled project in the auto-saves directory, and removes it when destroyed.
class FakeAutoSave
{
    QString _path;

public:
    FakeAutoSave()
    {
        QString dirPath = Project::autoSavesDir();
        QDir().mkpath(dirPath);
        // Same name pattern as Project::saveProjectInternal(): <name>.ntp.autosave.<time hash>
        _path = dirPath + QLatin1Char('/') +
                QString::fromUtf8("NatronTest1258-%1." NATRON_PROJECT_FILE_EXT ".autosave.1").arg( QCoreApplication::applicationPid() );
        QFile f(_path);
        if ( f.open(QIODevice::WriteOnly) ) {
            f.write("<Natron project>");
        }
    }

    ~FakeAutoSave()
    {
        QFile::remove(_path);
    }

    const QString& path() const
    {
        return _path;
    }
};
} // anonymous namespace

TEST(ProjectAutoSave, ClearAllCachesKeepsAutoSaves)
{
    FakeAutoSave autoSave;
    ASSERT_TRUE( QFile::exists( autoSave.path() ) );

    appPTR->clearAllCaches();

    EXPECT_TRUE( QFile::exists( autoSave.path() ) );
}

TEST(ProjectAutoSave, WipeDiskCacheKeepsAutoSaves)
{
    // Called by the "Wipe Disk Cache" preference, and at startup with --clear-cache or after a cache version change
    FakeAutoSave autoSave;
    ASSERT_TRUE( QFile::exists( autoSave.path() ) );

    appPTR->wipeAndCreateDiskCacheStructure();

    EXPECT_TRUE( QFile::exists( autoSave.path() ) );
}
