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

// "All change actions are bracketed by a pair of kOfxActionBeginInstanceChanged and kOfxActionEndInstanceChanged
// actions" (OFX 1.5.1 ofxCore.h, kOfxActionInstanceChanged), with the reason of the change, and "For
// BeginInstanceChanged, InstanceChanged will be called at least once". The test plug-in net.natron.test.Params
// (Tests/OfxTestPlugin) counts these actions, and the errors: an instance change outside a bracket or with another
// reason than its bracket, an end without a begin or with another reason.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>
#include <string>

#include <gtest/gtest.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
const char* const kCounters[] = {
    "Params.begin", "Params.end", "Params.instanceChanged", "Params.openBrackets",
    "Params.begin." kOfxChangeUserEdited, "Params.begin." kOfxChangePluginEdited,
    "Params.instanceChanged.chainSource", "Params.instanceChanged.chainTarget",
    "Params.error.instanceChangedOutsideBracket", "Params.error.instanceChangedReasonMismatch",
    "Params.error.unmatchedEnd", "Params.error.endReasonMismatch", "Params.error.noReason",
};

class OfxInstanceChangedTest
    : public ::testing::Test
{
protected:
    NodePtr _node;
    EffectInstancePtr _effect;
    std::map<std::string, int> _start;

    virtual void SetUp() OVERRIDE
    {
        if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParams) ) {
            return;
        }
        AppInstancePtr app = appPTR->getTopLevelInstance();
        CreateNodeArgs args( kNatronTestPluginParams, app->getProject() );
        args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        _node = app->createNode(args);
        if (_node) {
            _effect = _node->getEffectInstance();
        }
        resetCounters();
    }

    virtual void TearDown() OVERRIDE
    {
        _effect.reset();
        if (_node) {
            _node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
            _node.reset();
        }
    }

    bool skipped()
    {
        if (!_node) {
            NatronTestPlugin::reportSkipped(kNatronTestPluginParams);

            return true;
        }

        return false;
    }

    void resetCounters()
    {
        for (std::size_t i = 0; i < sizeof(kCounters) / sizeof(kCounters[0]); ++i) {
            _start[kCounters[i]] = NatronTestPlugin::counter(kCounters[i]);
        }
    }

    // the change of a counter since the last resetCounters()
    int delta(const std::string& name)
    {
        return NatronTestPlugin::counter(name) - _start[name];
    }

    // no bracketing error, and no bracket left open
    void expectNoError()
    {
        EXPECT_EQ( 0, delta("Params.error.instanceChangedOutsideBracket") );
        EXPECT_EQ( 0, delta("Params.error.instanceChangedReasonMismatch") );
        EXPECT_EQ( 0, delta("Params.error.unmatchedEnd") );
        EXPECT_EQ( 0, delta("Params.error.endReasonMismatch") );
        EXPECT_EQ( 0, delta("Params.error.noReason") );
        EXPECT_EQ( 0, delta("Params.openBrackets") );
        EXPECT_EQ( delta("Params.begin"), delta("Params.end") );
    }

    KnobIntPtr intKnob(const std::string& name) const
    {
        return std::dynamic_pointer_cast<KnobInt>( _node->getKnobByName(name) );
    }
};
} // anonymous namespace

// A change by the user: one kOfxActionInstanceChanged in one bracket, with the reason kOfxChangeUserEdited.
TEST_F(OfxInstanceChangedTest, UserEdit)
{
    if ( skipped() ) {
        return;
    }
    KnobIntPtr k = intKnob("intFrom");
    ASSERT_TRUE( bool(k) );

    k->setValue(3, ViewSpec::all(), 0, eValueChangedReasonUserEdited, NULL);
    EXPECT_EQ( 1, delta("Params.instanceChanged") );
    EXPECT_EQ( 1, delta("Params.begin") );
    EXPECT_EQ( 1, delta("Params.begin." kOfxChangeUserEdited) );
    expectNoError();
}

// The plug-in sets a parameter in kOfxActionInstanceChanged: the instance change of that parameter, with the reason
// kOfxChangePluginEdited, has its own bracket.
TEST_F(OfxInstanceChangedTest, PluginEditInInstanceChanged)
{
    if ( skipped() ) {
        return;
    }
    KnobIntPtr source = intKnob("chainSource");
    KnobIntPtr target = intKnob("chainTarget");
    ASSERT_TRUE(source && target);

    source->setValue(4, ViewSpec::all(), 0, eValueChangedReasonUserEdited, NULL);
    EXPECT_EQ( 5, target->getValue() );
    EXPECT_EQ( 1, delta("Params.instanceChanged.chainSource") );
    EXPECT_EQ( 1, delta("Params.instanceChanged.chainTarget") );
    EXPECT_EQ( 2, delta("Params.instanceChanged") );
    EXPECT_EQ( 1, delta("Params.begin." kOfxChangeUserEdited) );
    EXPECT_EQ( 1, delta("Params.begin." kOfxChangePluginEdited) );
    expectNoError();
}

// A group of value changes (e.g. restoring the default values of a node): a single bracket for all the instance
// changes with the same reason. The instance changes they trigger still get their own bracket.
TEST_F(OfxInstanceChangedTest, Group)
{
    if ( skipped() ) {
        return;
    }
    KnobIPtr a = _node->getKnobByName("intFrom");
    KnobIPtr b = _node->getKnobByName("doubleFrom");
    KnobIPtr source = _node->getKnobByName("chainSource");
    ASSERT_TRUE(a && b && source);

    const double time = _effect->getCurrentTime();
    _effect->beginKnobsValuesChanged_public(eValueChangedReasonRestoreDefault);
    _effect->onKnobValueChanged_public(a.get(), eValueChangedReasonRestoreDefault, time, ViewIdx(0), true);
    _effect->onKnobValueChanged_public(b.get(), eValueChangedReasonRestoreDefault, time, ViewIdx(0), true);
    _effect->onKnobValueChanged_public(source.get(), eValueChangedReasonRestoreDefault, time, ViewIdx(0), true);
    _effect->endKnobsValuesChanged_public(eValueChangedReasonRestoreDefault);
    EXPECT_EQ( 4, delta("Params.instanceChanged") );
    EXPECT_EQ( 1, delta("Params.begin." kOfxChangeUserEdited) );
    EXPECT_EQ( 1, delta("Params.begin." kOfxChangePluginEdited) );
    EXPECT_EQ( 2, delta("Params.begin") );
    expectNoError();

    // no change: no bracket ("For BeginInstanceChanged, InstanceChanged will be called at least once")
    resetCounters();
    _effect->beginKnobsValuesChanged_public(eValueChangedReasonUserEdited);
    _effect->endKnobsValuesChanged_public(eValueChangedReasonUserEdited);
    EXPECT_EQ( 0, delta("Params.begin") );
    expectNoError();
}

// Several changes queued while the evaluation of the node is blocked: one bracket per sequence of changes with the
// same reason.
TEST_F(OfxInstanceChangedTest, QueuedChanges)
{
    if ( skipped() ) {
        return;
    }
    KnobIPtr a = _node->getKnobByName("intFrom");
    KnobIPtr b = _node->getKnobByName("doubleFrom");
    KnobIPtr c = _node->getKnobByName("boolFrom");
    ASSERT_TRUE(a && b && c);

    const double time = _effect->getCurrentTime();
    _effect->beginChanges();
    _effect->appendValueChange(a, -1, false, time, ViewIdx(0), eValueChangedReasonUserEdited, eValueChangedReasonUserEdited);
    _effect->appendValueChange(b, -1, false, time, ViewIdx(0), eValueChangedReasonUserEdited, eValueChangedReasonUserEdited);
    _effect->appendValueChange(c, -1, false, time, ViewIdx(0), eValueChangedReasonPluginEdited, eValueChangedReasonPluginEdited);
    _effect->endChanges();
    EXPECT_EQ( 3, delta("Params.instanceChanged") );
    EXPECT_EQ( 1, delta("Params.begin." kOfxChangeUserEdited) );
    EXPECT_EQ( 1, delta("Params.begin." kOfxChangePluginEdited) );
    EXPECT_EQ( 2, delta("Params.begin") );
    expectNoError();
}

// The preset of the test plug-in sets other parameters in a paramEditBegin/paramEditEnd block, from
// kOfxActionInstanceChanged: every instance change is bracketed.
TEST_F(OfxInstanceChangedTest, EditBlockInInstanceChanged)
{
    if ( skipped() ) {
        return;
    }
    KnobChoicePtr preset = std::dynamic_pointer_cast<KnobChoice>( _node->getKnobByName("preset") );
    ASSERT_TRUE( bool(preset) );
    const int applied = NatronTestPlugin::counter("Params.presetApplied");

    preset->setValue(2, ViewSpec::all(), 0, eValueChangedReasonUserEdited, NULL);
    EXPECT_EQ( applied + 1, NatronTestPlugin::counter("Params.presetApplied") );
    EXPECT_EQ( 0, preset->getValue() );
    EXPECT_LE( 4, delta("Params.instanceChanged") );
    expectNoError();
}
