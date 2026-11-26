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

// KnobHolder::create*Knob() (the Python createXParam() functions) with the name of an existing parameter: they
// return it if it has the requested type and at least the requested dimensions; otherwise they return NULL and say
// why in the error log, instead of returning NULL silently (another type) or a parameter with too few dimensions
// (#1621).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <string>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/LogEntry.h"
#include "Engine/Node.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
class KnobHolderCreateKnobTest
    : public ::testing::Test
{
protected:
    NodePtr _node;

    virtual void SetUp() OVERRIDE
    {
        if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParams) ) {
            return;
        }
        AppInstancePtr app = appPTR->getTopLevelInstance();
        CreateNodeArgs args( kNatronTestPluginParams, app->getProject() );
        args.setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
        args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        _node = app->createNode(args);
        appPTR->clearErrorLog_mt_safe();
    }

    virtual void TearDown() OVERRIDE
    {
        if (_node) {
            _node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
            _node.reset();
        }
        appPTR->clearErrorLog_mt_safe();
    }

    bool skipped()
    {
        if (!_node) {
            NatronTestPlugin::reportMissing(kNatronTestPluginParams);

            return true;
        }

        return false;
    }

    EffectInstance* holder() const
    {
        return _node->getEffectInstance().get();
    }

    // the number of messages of the error log about the parameter name
    static int errorLogMentions(const std::string& name)
    {
        std::list<LogEntry> log;
        appPTR->getErrorLog_mt_safe(&log);
        int n = 0;
        for (std::list<LogEntry>::const_iterator it = log.begin(); it != log.end(); ++it) {
            if ( it->message.contains( QString::fromUtf8( name.c_str() ) ) ) {
                ++n;
            }
        }

        return n;
    }
};
} // anonymous namespace

// the same type and dimension: the existing parameter
TEST_F(KnobHolderCreateKnobTest, SameTypeReturnsExisting)
{
    if ( skipped() ) {
        return;
    }
    KnobDoublePtr first = holder()->createDoubleKnob("userDouble", "userDouble", 2);
    ASSERT_TRUE( bool(first) );
    KnobDoublePtr second = holder()->createDoubleKnob("userDouble", "userDouble", 2);
    EXPECT_EQ(first, second);
    // more dimensions than requested: the existing parameter too (e.g. createColorParam() without alpha on an RGBA one)
    KnobColorPtr rgba = holder()->createColorKnob("userColor", "userColor", 4);
    ASSERT_TRUE( bool(rgba) );
    EXPECT_EQ( rgba, holder()->createColorKnob("userColor", "userColor", 3) );
    EXPECT_EQ( 0, errorLogMentions("userDouble") );
    EXPECT_EQ( 0, errorLogMentions("userColor") );
}

// another type: NULL, and an error
TEST_F(KnobHolderCreateKnobTest, OtherTypeIsAnError)
{
    if ( skipped() ) {
        return;
    }
    ASSERT_TRUE( bool( holder()->createDoubleKnob("userParam", "userParam", 1) ) );
    EXPECT_FALSE( bool( holder()->createIntKnob("userParam", "userParam", 1) ) );
    EXPECT_FALSE( bool( holder()->createStringKnob("userParam", "userParam") ) );
    EXPECT_EQ( 2, errorLogMentions("userParam") );
}

// fewer dimensions than requested (e.g. createInt2DParam() with the name of an IntParam): NULL, and an error
TEST_F(KnobHolderCreateKnobTest, FewerDimensionsIsAnError)
{
    if ( skipped() ) {
        return;
    }
    ASSERT_TRUE( bool( holder()->createIntKnob("userInt", "userInt", 1) ) );
    EXPECT_FALSE( bool( holder()->createIntKnob("userInt", "userInt", 2) ) );
    EXPECT_EQ( 1, errorLogMentions("userInt") );
}
