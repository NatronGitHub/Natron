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

// A kOfxMessageQuestion posted by a plug-in must return kOfxStatReplyYes only if the user answered yes
// (#1374). In background mode, as in the tests, or from a thread other than the main thread, nobody is
// asked: the plug-in must get an error ("It is an error to post a question message if the plugin is not
// in an interactive session", ofxMessage.h), never yes.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include <gtest/gtest.h>

#include "Engine/KnobTypes.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

TEST(OfxQuestion, NoReplyIsNeverYes)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginQuestion) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginQuestion);

        return;
    }
    ASSERT_TRUE( appPTR->isBackground() );
    NodePtr node = NatronTestPlugin::createNode(kNatronTestPluginQuestion);
    ASSERT_TRUE( bool(node) );

    // "ask" posts the question on the thread that calls kOfxActionInstanceChanged (here, the main
    // thread), "askFromThread" on a thread of the plug-in.
    const char* buttons[] = { "ask", "askFromThread" };
    for (int i = 0; i < 2; ++i) {
        const std::string prefix = std::string("Question.") + buttons[i];
        KnobButton* button = dynamic_cast<KnobButton*>( node->getKnobByName(buttons[i]).get() );
        ASSERT_TRUE(button != NULL) << buttons[i];
        const int yes = NatronTestPlugin::counter(prefix + ".yes");
        const int no = NatronTestPlugin::counter(prefix + ".no");
        const int error = NatronTestPlugin::counter(prefix + ".error");
        button->trigger();
        EXPECT_EQ( yes, NatronTestPlugin::counter(prefix + ".yes") ) << buttons[i] << ": nobody answered yes";
        EXPECT_EQ( no, NatronTestPlugin::counter(prefix + ".no") ) << buttons[i] << ": nobody answered no";
        EXPECT_EQ( error + 1, NatronTestPlugin::counter(prefix + ".error") ) << buttons[i];
    }

    node->destroyNode(true, false);
}
