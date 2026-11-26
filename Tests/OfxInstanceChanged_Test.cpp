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

#include <list>
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
    : public NatronTestPlugin::ParamsNodeTest
{
protected:
    EffectInstancePtr _effect;
    std::map<std::string, int> _start;

    virtual void SetUp() OVERRIDE
    {
        ParamsNodeTest::SetUp();
        if (_node) {
            _effect = _node->getEffectInstance();
        }
        resetCounters();
    }

    virtual void TearDown() OVERRIDE
    {
        _effect.reset();
        ParamsNodeTest::TearDown();
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

// Restore Defaults (RestoreDefaultsCommand, KnobHolder::resetKnobsToDefaultValues()): one bracket, and one instance
// change per parameter, whether its value changed or not.
TEST_F(OfxInstanceChangedTest, RestoreDefaults)
{
    if ( skipped() ) {
        return;
    }
    KnobIntPtr a = intKnob("intFrom");
    KnobDoublePtr b = std::dynamic_pointer_cast<KnobDouble>( _node->getKnobByName("doubleFrom") );
    KnobIPtr c = _node->getKnobByName("boolFrom");
    ASSERT_TRUE(a && b && c);
    a->setValue(3, ViewSpec::all(), 0, eValueChangedReasonUserEdited, NULL);
    b->setValueAtTime(1., 2., ViewSpec::all(), 0);
    b->setValueAtTime(5., 3., ViewSpec::all(), 0);
    resetCounters();
    const int icA = NatronTestPlugin::counter("Params.instanceChanged.intFrom");
    const int icB = NatronTestPlugin::counter("Params.instanceChanged.doubleFrom");
    const int icC = NatronTestPlugin::counter("Params.instanceChanged.boolFrom");

    std::list<KnobIPtr> knobs;
    knobs.push_back(a);
    knobs.push_back(b);
    knobs.push_back(c);
    _effect->resetKnobsToDefaultValues( knobs, -1, _effect->getCurrentTime() );
    EXPECT_EQ( 0, a->getValue() );
    EXPECT_EQ( 0, b->getKeyFramesCount(ViewIdx(0), 0) );
    EXPECT_EQ( 3, delta("Params.instanceChanged") );
    EXPECT_EQ( icA + 1, NatronTestPlugin::counter("Params.instanceChanged.intFrom") );
    EXPECT_EQ( icB + 1, NatronTestPlugin::counter("Params.instanceChanged.doubleFrom") );
    EXPECT_EQ( icC + 1, NatronTestPlugin::counter("Params.instanceChanged.boolFrom") );
    EXPECT_EQ( 1, delta("Params.begin") );
    EXPECT_EQ( 1, delta("Params.begin." kOfxChangeUserEdited) );
    expectNoError();
}

// Setting or removing an expression from Python (Param.setExpression()) changes the value of the parameter: the
// plug-in gets one kOfxActionInstanceChanged in one bracket, with the reason of a user edit, as when the expression
// is set in the GUI, for an int, double, bool and choice parameter (#1336). As with setValue(), nothing is sent if
// nothing changed.
TEST_F(OfxInstanceChangedTest, PythonSetExpression)
{
    if ( skipped() ) {
        return;
    }
    // each step: the expression to set, whether setExpression() succeeds, the expression after it, and the number of
    // kOfxActionInstanceChanged
    struct Step
    {
        const char* expr;
        bool ok;
        const char* exprAfter;
        int instanceChanged;
    };
    const Step steps[] = {
        { "1", true, "1", 1 },           // set
        { "1", true, "1", 0 },           // the same again: no change
        { "1 +", false, "", 1 },         // invalid: removes the previous expression
        { "1 +", false, "", 0 },         // invalid, with no previous expression: no change
        { "1", true, "1", 1 },
        { "", true, "", 1 },             // remove
        { "", true, "", 0 },             // remove again: no change
    };
    const char* const names[] = { "intTo", "doubleTo", "boolTo", "choiceTo" };
    const std::string node = _node->getApp()->getAppIDString() + '.' + _node->getFullyQualifiedName();
    for (std::size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        const std::string name = names[i];
        KnobIPtr knob = _node->getKnobByName(name);
        ASSERT_TRUE( bool(knob) ) << name;
        for (std::size_t s = 0; s < sizeof(steps) / sizeof(steps[0]); ++s) {
            const Step& step = steps[s];
            resetCounters();
            const int ic = NatronTestPlugin::counter("Params.instanceChanged." + name);
            std::string error;
            testing::internal::CaptureStdout(); // the error message of an invalid expression
            const bool ran = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript(
                std::string("assert ") + (step.ok ? "" : "not ") + node + '.' + name + ".setExpression('" + step.expr + "', False)\n",
                &error, 0);
            testing::internal::GetCapturedStdout();
            EXPECT_TRUE(ran) << name << " step " << s << ": " << error;
            EXPECT_EQ( step.exprAfter, knob->getExpression(0) ) << name << " step " << s;
            EXPECT_EQ( ic + step.instanceChanged, NatronTestPlugin::counter("Params.instanceChanged." + name) ) << name << " step " << s;
            EXPECT_EQ( step.instanceChanged, delta("Params.instanceChanged") ) << name << " step " << s;
            EXPECT_EQ( step.instanceChanged, delta("Params.begin") ) << name << " step " << s;
            EXPECT_EQ( step.instanceChanged, delta("Params.begin." kOfxChangeUserEdited) ) << name << " step " << s;
            expectNoError();
        }
    }
}
