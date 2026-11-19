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

// Which knob changes are undoable (KnobI::getCanUndo(), used by KnobGui::pushUndoCommand() and by Knob<T>::setValue()
// for the changes of a plug-in in a paramEditBegin()/paramEditEnd() block): for an OpenFX parameter,
// kOfxParamPropCanUndo ("Flags whether changes to a parameter should be put on the undo/redo stack", OFX 1.5.1
// ofxParam.h), whatever kOfxParamPropEvaluateOnChange, which "is used to indicate if the value of a parameter has any
// affect on an effect's output". The undo stacks themselves are in the GUI, which the tests don't have.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

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

// the parameters of the test plug-in net.natron.test.Params
TEST(OfxParamUndo, CanUndoIsTheOfxProperty)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParams) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginParams);

        return;
    }
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( kNatronTestPluginParams, app->getProject() );
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    NodePtr node = app->createNode(args);
    ASSERT_TRUE( bool(node) );

    // kOfxParamPropEvaluateOnChange = 0, kOfxParamPropCanUndo = 1 (the default)
    KnobIPtr uiOnly = node->getKnobByName("uiOnly");
    ASSERT_TRUE( bool(uiOnly) );
    EXPECT_FALSE( uiOnly->getEvaluateOnChange() );
    EXPECT_TRUE( uiOnly->getCanUndo() );
    // kOfxParamPropCanUndo = 0
    EXPECT_FALSE( node->getKnobByName("noUndo")->getCanUndo() );
    EXPECT_FALSE( node->getKnobByName("preset")->getCanUndo() );
    // the defaults
    EXPECT_TRUE( node->getKnobByName("presetA")->getCanUndo() );

    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
}

// The Natron knobs keep their rule: unless setCanUndo() says otherwise, only the knobs that evaluate on change are
// undoable.
TEST(OfxParamUndo, NatronKnobs)
{
    KnobDoublePtr knob = std::make_shared<KnobDouble>( (KnobHolder*)NULL, std::string("knob"), 1, false );

    EXPECT_TRUE( knob->getCanUndo() );
    knob->setEvaluateOnChange(false);
    EXPECT_FALSE( knob->getCanUndo() );
    knob->setCanUndo(true);
    EXPECT_TRUE( knob->getCanUndo() );
    knob->setEvaluateOnChange(true);
    knob->setCanUndo(false);
    EXPECT_FALSE( knob->getCanUndo() );
}

// An unmatched paramEditEnd() must not end a beginChanges() bracket of Natron early, and a matched
// paramEditBegin()/paramEditEnd() pair, with an empty name, leaves it as it was.
// Note: Tests runs as a background app, where KnobHolder::setMultipleParamsEditLevel() ignores the edit blocks, so
// this only fails when run with a GUI app; it documents the contract.
TEST(OfxParamUndo, UnmatchedParamEditEnd)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginParams) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginParams);

        return;
    }
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( kNatronTestPluginParams, app->getProject() );
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    NodePtr node = app->createNode(args);
    ASSERT_TRUE( bool(node) );
    EffectInstancePtr effect = node->getEffectInstance();
    OfxEffectInstance* ofxEffect = dynamic_cast<OfxEffectInstance*>( effect.get() );
    ASSERT_TRUE(ofxEffect && ofxEffect->effectInstance());
    OfxParamSetHandle paramSet = ofxEffect->effectInstance()->getParamSetHandle();
    const OfxParameterSuiteV1* suite = NatronTestPlugin::parameterSuite();

    ASSERT_FALSE( effect->isEvaluationBlocked() );
    effect->beginChanges();
    EXPECT_TRUE( effect->isEvaluationBlocked() );
    EXPECT_EQ( kOfxStatOK, suite->paramEditEnd(paramSet) ); // unmatched
    EXPECT_TRUE( effect->isEvaluationBlocked() );
    EXPECT_EQ( kOfxStatOK, suite->paramEditBegin(paramSet, "") );
    EXPECT_EQ( kOfxStatOK, suite->paramEditEnd(paramSet) );
    EXPECT_TRUE( effect->isEvaluationBlocked() );
    EXPECT_EQ( KnobHolder::eMultipleParamsEditOff, effect->getMultipleParamsEditLevel() );
    EXPECT_EQ( std::string(), effect->getMultipleParamsEditLabel() );
    effect->endChanges();
    EXPECT_FALSE( effect->isEvaluationBlocked() );

    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
}
