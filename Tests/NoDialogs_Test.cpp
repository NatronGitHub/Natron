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

#include <QtCore/QStringList>

#include "Engine/AppManager.h"
#include "Engine/CLArgs.h"

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
