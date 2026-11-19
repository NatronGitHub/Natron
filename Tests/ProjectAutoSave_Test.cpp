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
// Regression test for #825: a user save during a background auto-save must not be dropped.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <iostream>
#include <thread>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <QThread>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/KnobTypes.h"
#include "Engine/LogEntry.h"
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

namespace {
// Runs a background auto-save of the top-level project in another thread, as
// Project::onAutoSaveTimerTriggered() does, and makes it last ~1.5 s with a
// beforeProjectSave Python callback that sleeps when called for an auto-save.
// autoSaveCode and saveCode are Python lines (indented by 8 spaces) run by the callback
// for the auto-save (before the sleep) and for the other saves.
class SlowAutoSave
{
    ProjectPtr _project;
    KnobStringPtr _callbackKnob;
    std::string _oldCallback;
    QString _marker;
    bool _ok;
    std::thread _thread;

public:
    SlowAutoSave(const QString& dir,
                 const QString& name,
                 const std::string& autoSaveCode = std::string(),
                 const std::string& saveCode = std::string())
        : _project( appPTR->getTopLevelInstance()->getProject() )
        , _marker( dir + QString::fromUtf8("autosave-started") )
        , _ok(false)
    {
        std::string script =
            "import os, time\n"
            "def natronTest825BeforeSave(filename, app, autoSave):\n"
            "    if autoSave:\n"
            "        open(\"" + _marker.toStdString() + "\", \"w\").close()\n"
            + autoSaveCode +
            "        time.sleep(1.5)\n"
            "    else:\n"
            "        pass\n"
            + saveCode +
            "    return filename\n";
        std::string error, output;
        EXPECT_TRUE( ::Natron::Python::interpretPythonScript(script, &error, &output) ) << error << output;
        _callbackKnob = std::dynamic_pointer_cast<KnobString>( _project->getKnobByName("beforeProjectSave") );
        EXPECT_TRUE( bool(_callbackKnob) );
        if (_callbackKnob) {
            _oldCallback = _callbackKnob->getValue();
            _callbackKnob->setValue("natronTest825BeforeSave");
        }
        _thread = std::thread([this, dir, name]() {
            _ok = _project->saveProject_imp(dir, name, true /*autoSave*/, true /*updateProjectProperties*/);
        });
        // wait until the auto-save is running
        QElapsedTimer timer;
        timer.start();
        while ( !QFile::exists(_marker) && timer.elapsed() < 10000 ) {
            QThread::msleep(10);
        }
        EXPECT_TRUE( QFile::exists(_marker) ) << "the auto-save did not start";
    }

    ~SlowAutoSave()
    {
        join();
        if (_callbackKnob) {
            _callbackKnob->setValue(_oldCallback);
        }
        _project->removeLastAutosave();
    }

    bool join()
    {
        if ( _thread.joinable() ) {
            _thread.join();
        }

        return _ok;
    }
};

// Runs f in another thread and waits at most timeoutS seconds for it. A hang is a test failure,
// but the test binary can't go on with a thread stuck in a save: exit.
template<typename F>
bool
runWithTimeout(F f,
               int timeoutS)
{
    std::packaged_task<bool()> task(f);
    std::future<bool> result = task.get_future();
    std::thread thread( std::move(task) );
    if (result.wait_for( std::chrono::seconds(timeoutS) ) != std::future_status::ready) {
        ADD_FAILURE() << "the save did not finish after " << timeoutS << " s: the project stayed marked as being saved";
        std::cerr << std::flush;
        std::fflush(stdout);
        std::_Exit(1);
    }
    thread.join();

    return result.get();
}

bool
errorLogContains(const QString& text)
{
    std::list<LogEntry> entries;
    appPTR->getErrorLog_mt_safe(&entries);
    for (std::list<LogEntry>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        if ( it->message.contains(text) ) {
            return true;
        }
    }

    return false;
}
} // anonymous namespace

// Ctrl+S (Gui::saveProject() -> Project::saveProject()) while the auto-save is running
// returned at once without saving and without any error (#825). It must wait for the auto-save and save.
TEST(ProjectAutoSave, SaveDuringAutoSave)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    const QString name = QString::fromUtf8("SaveDuringAutoSave.ntp");
    ProjectPtr project = appPTR->getTopLevelInstance()->getProject();
    {
        SlowAutoSave autoSave(dir, name);
        QString savedPath;
        EXPECT_TRUE( project->saveProject(dir, name, &savedPath) );
        EXPECT_EQ(dir + name, savedPath);
        EXPECT_TRUE( QFile::exists(dir + name) ) << "the save was dropped";
        EXPECT_TRUE( autoSave.join() );
    }
    // the save after the auto-save made it obsolete: it was removed
    EXPECT_FALSE( QFile::exists(dir + name + QString::fromUtf8(".autosave")) );
    project->resetProject();
}

// Same from Python (app.saveProjectAs()), where the thread that waits holds the Python GIL, which
// the auto-save needs to run the beforeProjectSave callback: the wait must not deadlock.
TEST(ProjectAutoSave, PythonSaveDuringAutoSave)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    const QString name = QString::fromUtf8("PythonSaveDuringAutoSave.ntp");
    AppInstancePtr app = appPTR->getTopLevelInstance();
    {
        SlowAutoSave autoSave(dir, name);
        std::string script = "assert " + app->getAppIDString() + ".saveProjectAs(\"" + (dir + name).toStdString() + "\")\n";
        std::string error, output;
        EXPECT_TRUE( ::Natron::Python::interpretPythonScript(script, &error, &output) ) << error << output;
        EXPECT_TRUE( QFile::exists(dir + name) ) << "the save was dropped";
        EXPECT_TRUE( autoSave.join() );
    }
    app->getProject()->resetProject();
}

// The beforeProjectSave callback of the auto-save calls app.saveProjectAs() in the auto-save
// thread: that nested save fails (the project is being saved), without showing a dialog from that
// thread, which would wait for the main thread while it waits for this auto-save (deadlock).
// A save during that auto-save still waits and saves.
TEST(ProjectAutoSave, NestedSaveInAutoSaveCallback)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    const QString name = QString::fromUtf8("NestedSave.ntp");
    const QString nested = dir + QString::fromUtf8("Nested.ntp");
    const QString nestedResult = dir + QString::fromUtf8("nested-result");
    ProjectPtr project = appPTR->getTopLevelInstance()->getProject();
    appPTR->clearErrorLog_mt_safe();
    {
        SlowAutoSave autoSave(dir, name,
                              "        with open(\"" + nestedResult.toStdString() + "\", \"w\") as f:\n"
                              "            f.write(str(app.saveProjectAs(\"" + nested.toStdString() + "\")))\n");
        EXPECT_TRUE( runWithTimeout([&]() { return project->saveProject(dir, name, 0); }, 30) );
        EXPECT_TRUE( QFile::exists(dir + name) ) << "the save was dropped";
        EXPECT_TRUE( autoSave.join() );
    }
    QFile f(nestedResult);
    ASSERT_TRUE( f.open(QIODevice::ReadOnly) );
    EXPECT_EQ( QByteArray("False"), f.readAll() );
    EXPECT_FALSE( QFile::exists(nested) );
    EXPECT_TRUE( errorLogContains( QString::fromUtf8("being saved") ) );
    project->resetProject();
}

// An auto-save that fails with an exception must not leave the project marked as being saved:
// the save waiting for it, and the later ones, must run (they would hang forever).
// The auto-save fails because the callback points TMPDIR to a missing directory (the save first
// writes to a temporary file there); the callback of the next save restores it.
TEST(ProjectAutoSave, SaveAfterFailedAutoSave)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    const QString name = QString::fromUtf8("SaveAfterFailedAutoSave.ntp");
    ProjectPtr project = appPTR->getTopLevelInstance()->getProject();
    const QByteArray oldTmpDir = qgetenv("TMPDIR");
    {
        SlowAutoSave autoSave(dir, name,
                              "        globals()['natronTest825OldTmp'] = os.environ.get('TMPDIR')\n"
                              "        os.environ['TMPDIR'] = \"" + (dir + QString::fromUtf8("missing/dir")).toStdString() + "\"\n",
                              "        if globals().get('natronTest825OldTmp') is not None:\n"
                              "            os.environ['TMPDIR'] = globals()['natronTest825OldTmp']\n"
                              "        elif 'TMPDIR' in os.environ and 'missing' in os.environ['TMPDIR']:\n"
                              "            del os.environ['TMPDIR']\n");
        EXPECT_TRUE( runWithTimeout([&]() { return project->saveProject(dir, name, 0); }, 30) );
        EXPECT_FALSE( autoSave.join() ) << "the auto-save should have failed";
        EXPECT_TRUE( QFile::exists(dir + name) ) << "the save was dropped";
    }
    // and a later save
    EXPECT_TRUE( runWithTimeout([&]() { return project->saveProject(dir, name, 0); }, 30) );
    if ( oldTmpDir.isEmpty() ) {
        qunsetenv("TMPDIR");
    } else {
        qputenv("TMPDIR", oldTmpDir);
    }
    project->resetProject();
}
