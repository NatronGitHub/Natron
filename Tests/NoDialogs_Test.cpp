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

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/CLArgs.h"
#include "Engine/Project.h"

NATRON_NAMESPACE_USING

// --no-dialogs: see CLArgs::areDialogsDisabled() and AppManager::isDialogsDisabled()

static CLArgs
parse(const QStringList& args)
{
    QStringList argv;

    argv << QString::fromUtf8("Natron") << args;

    return CLArgs(argv, false);
}

TEST(NoDialogs, CLArgsParsing)
{
    EXPECT_FALSE( parse( QStringList() ).areDialogsDisabled() );
    EXPECT_TRUE( parse( QStringList() << QString::fromUtf8("--no-dialogs") ).areDialogsDisabled() );

    // with other options and a script
    CLArgs cl = parse( QStringList() << QString::fromUtf8("--no-settings") << QString::fromUtf8("--no-dialogs")
                                     << QString::fromUtf8("/tmp/script.py") );
    EXPECT_TRUE( cl.areDialogsDisabled() );
    EXPECT_FALSE( cl.getError() );
    EXPECT_TRUE( cl.isLoadedUsingDefaultSettings() );
    EXPECT_EQ( cl.getScriptFilename(), QString::fromUtf8("/tmp/script.py") );

    // after "--", it is not an option
    EXPECT_FALSE( parse( QStringList() << QString::fromUtf8("--") << QString::fromUtf8("--no-dialogs") ).areDialogsDisabled() );

    // copied with the arguments
    CLArgs copy;
    copy = cl;
    EXPECT_TRUE( copy.areDialogsDisabled() );
}

TEST(NoDialogs, DismissedAnswer)
{
    // the default button, as Enter would pick
    EXPECT_EQ( Dialogs::getDismissedDialogAnswer(StandardButtons(eStandardButtonYes | eStandardButtonNo), eStandardButtonYes), eStandardButtonYes );
    EXPECT_EQ( Dialogs::getDismissedDialogAnswer(StandardButtons(eStandardButtonYes | eStandardButtonNo), eStandardButtonNo), eStandardButtonNo );
    EXPECT_EQ( Dialogs::getDismissedDialogAnswer(StandardButtons(eStandardButtonSave | eStandardButtonDiscard | eStandardButtonCancel), eStandardButtonSave),
               eStandardButtonSave );
    // no default button: the button Escape would pick
    EXPECT_EQ( Dialogs::getDismissedDialogAnswer(StandardButtons(eStandardButtonYes | eStandardButtonNo), eStandardButtonNoButton), eStandardButtonNo );
    EXPECT_EQ( Dialogs::getDismissedDialogAnswer(StandardButtons(eStandardButtonYes | eStandardButtonNo | eStandardButtonCancel), eStandardButtonNoButton),
               eStandardButtonCancel );
    EXPECT_EQ( Dialogs::getDismissedDialogAnswer(StandardButtons(eStandardButtonOk), eStandardButtonNoButton), eStandardButtonEscape );
}

TEST(NoDialogs, DialogFunctions)
{
    ASSERT_TRUE( appPTR );
    const bool wasDisabled = appPTR->isDialogsDisabled();

    // The Tests binary runs in background mode, where a question is answered Yes
    appPTR->setDialogsDisabled(false);
    EXPECT_EQ( Dialogs::questionDialog("Test", "Question?", false, StandardButtons(eStandardButtonYes | eStandardButtonNo), eStandardButtonNo),
               eStandardButtonYes );

    // --no-dialogs: the default button
    appPTR->setDialogsDisabled(true);
    EXPECT_EQ( Dialogs::questionDialog("Test", "Question?", false, StandardButtons(eStandardButtonYes | eStandardButtonNo), eStandardButtonNo),
               eStandardButtonNo );
    EXPECT_EQ( Dialogs::questionDialog("Test", "Question?", false), eStandardButtonNo ); // Yes|No without default: Escape
    bool stopAsking = true;
    EXPECT_EQ( Dialogs::questionDialog("Test", "Question?", false, StandardButtons(eStandardButtonYes | eStandardButtonNo), eStandardButtonYes, &stopAsking),
               eStandardButtonYes );
    EXPECT_FALSE(stopAsking); // the "do not show again" answer is not set
    stopAsking = true;
    Dialogs::warningDialog("Test", "Warning", &stopAsking);
    EXPECT_FALSE(stopAsking);
    stopAsking = true;
    Dialogs::errorDialog("Test", "Error", &stopAsking);
    EXPECT_FALSE(stopAsking);
    stopAsking = true;
    Dialogs::informationDialog("Test", "Information", &stopAsking);
    EXPECT_FALSE(stopAsking);
    Dialogs::errorDialog("Test", "Error");
    Dialogs::warningDialog("Test", "Warning");
    Dialogs::informationDialog("Test", "Information");

    appPTR->setDialogsDisabled(wasDisabled);
}

static void
touch(const QString& path)
{
    QFile f(path);
    ASSERT_TRUE( f.open(QIODevice::WriteOnly) );
    f.write("x");
}

// --no-dialogs: Project::removeLastAutosave() only removes the auto-saves this session wrote,
// never one that may be the user's crash recovery (e.g. <dir>/<name>.ntp.autosave)
TEST(NoDialogs, RemoveOnlyOwnAutoSaves)
{
    ASSERT_TRUE( appPTR );
    AppInstancePtr app = appPTR->getTopLevelInstance();
    ASSERT_TRUE( app != nullptr );
    ProjectPtr project = app->getProject();
    QTemporaryDir dir;
    ASSERT_TRUE( dir.isValid() );

    // restores the shared state even if an ASSERT fails: declared after dir, so the project
    // is reset before the directory it was saved to is removed
    struct Restore
    {
        ProjectPtr project;
        bool dialogsDisabled;
        ~Restore()
        {
            project->reset(false, true);
            appPTR->setDialogsDisabled(dialogsDisabled);
        }
    } restore = { project, appPTR->isDialogsDisabled() };

    const QString path = dir.path() + QLatin1Char('/');
    const QString name = QString::fromUtf8("foo.ntp");
    const QString autoSave = path + name + QString::fromUtf8(".binautosave"); // written by 2.6 (#1056)
    const QString textAutoSave = path + name + QString::fromUtf8(".autosave"); // written by 2.5 and earlier

    // the project is saved by the user (in background mode this doesn't touch the auto-saves)
    ASSERT_TRUE( project->saveProject(path, name, 0) );
    ASSERT_TRUE( QFile::exists(path + name) );

    // a previous session left an auto-save next to the project
    appPTR->setDialogsDisabled(true);
    touch(autoSave);
    touch(textAutoSave);
    EXPECT_FALSE( project->canRemoveAutoSave(autoSave) );
    EXPECT_FALSE( project->canRemoveAutoSave(textAutoSave) );
    project->removeLastAutosave();
    EXPECT_TRUE( QFile::exists(autoSave) ) << "--no-dialogs must keep an auto-save it did not write";
    EXPECT_TRUE( QFile::exists(textAutoSave) ) << "--no-dialogs must keep an auto-save it did not write";
    QFile::remove(textAutoSave);

    // an auto-save written by this session is removed, and forgotten
    ASSERT_TRUE( project->saveProject_imp(path, name, true /*autoSave*/, true) );
    EXPECT_TRUE( project->wroteAutoSave(autoSave) );
    project->removeLastAutosave();
    EXPECT_FALSE( QFile::exists(autoSave) );
    EXPECT_FALSE( project->wroteAutoSave(autoSave) );

    // reset() forgets the auto-saves of the previous project
    ASSERT_TRUE( project->saveProject_imp(path, name, true /*autoSave*/, true) );
    EXPECT_TRUE( project->wroteAutoSave(autoSave) );
    project->reset(false, true);
    EXPECT_FALSE( project->wroteAutoSave(autoSave) );
    QFile::remove(autoSave);

    // Clear All Caches (Project::clearAutoSavesDir()) keeps the auto-saves of other sessions
    const QString foreign = Project::autoSavesDir() + QString::fromUtf8("/NoDialogsTest.ntp.autosave.%1").arg( QCoreApplication::applicationPid() );
    QDir().mkpath( Project::autoSavesDir() );
    touch(foreign);
    Project::clearAutoSavesDir();
    EXPECT_TRUE( QFile::exists(foreign) ) << "--no-dialogs must keep the auto-saves of other sessions";
    QFile::remove(foreign);

    // without --no-dialogs, removeLastAutosave() keeps removing the auto-save of the project
    ASSERT_TRUE( project->saveProject(path, name, 0) );
    appPTR->setDialogsDisabled(false);
    touch(autoSave);
    EXPECT_TRUE( project->canRemoveAutoSave(autoSave) );
    project->removeLastAutosave();
    EXPECT_FALSE( QFile::exists(autoSave) );
    EXPECT_TRUE( QFile::exists(path + name) ) << "the project file itself is never removed";
}
