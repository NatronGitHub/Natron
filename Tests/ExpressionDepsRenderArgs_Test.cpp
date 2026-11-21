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

// At render setup (ParallelRenderArgsSetter), the nodes that the expressions of the rendered nodes read get
// render args (ParallelRenderArgs) too, so that during the render their parameters are read at the time of the
// render, not at the time of the timeline. The loop that added them inserted the node itself instead of each
// dependency, so they never got render args (#1564).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Engine/Curve.h"
#include "Engine/KnobTypes.h"
#include "Engine/NodeGroup.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
const double kTimelineTime = 1.;
const double kRenderTime = 5.;

class ExpressionDepsRenderArgsTest
    : public ::testing::Test
{
protected:
    std::vector<NodePtr> _nodes;
    SequenceTime _savedTimelineTime;

    ExpressionDepsRenderArgsTest()
        : _savedTimelineTime(0)
    {
    }

    static void seek(SequenceTime time)
    {
        appPTR->getTopLevelInstance()->getTimeLine()->seekFrame(time, false, NULL, eTimelineChangeReasonOtherSeek);
    }

    virtual void SetUp() OVERRIDE
    {
        _savedTimelineTime = appPTR->getTopLevelInstance()->getTimeLine()->currentFrame();
        seek(kTimelineTime);
    }

    virtual void TearDown() OVERRIDE
    {
        // clear the expressions first, so that no expression reads a destroyed node
        for (std::vector<NodePtr>::iterator it = _nodes.begin(); it != _nodes.end(); ++it) {
            const KnobsVec& knobs = (*it)->getKnobs();
            for (KnobsVec::const_iterator k = knobs.begin(); k != knobs.end(); ++k) {
                for (int i = 0; i < (*k)->getDimension(); ++i) {
                    if ( !(*k)->getExpression(i).empty() ) {
                        (*k)->clearExpression(i, true);
                    }
                }
            }
        }
        for (std::vector<NodePtr>::reverse_iterator it = _nodes.rbegin(); it != _nodes.rend(); ++it) {
            (*it)->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
        }
        _nodes.clear();
        seek(_savedTimelineTime);
    }

    bool skipped(const char* pluginID = kNatronTestPluginParams)
    {
        if ( !NatronTestPlugin::isAvailable(pluginID) ) {
            NatronTestPlugin::reportSkipped(pluginID);

            return true;
        }

        return false;
    }

    NodePtr createNode(const char* pluginID = kNatronTestPluginParams,
                       const NodeCollectionPtr& group = NodeCollectionPtr())
    {
        NodePtr node;

        if (group) {
            CreateNodeArgs args(pluginID, group);
            args.setProperty<bool>(kCreateNodeArgsPropNodeGroupDisableCreateInitialNodes, true);
            args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
            args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
            args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
            node = appPTR->getTopLevelInstance()->createNode(args);
        } else {
            node = NatronTestPlugin::createNode(pluginID);
        }

        if (node) {
            _nodes.push_back(node);
        }

        return node;
    }

    static KnobDoublePtr knob(const NodePtr& node,
                              const std::string& name)
    {
        return std::dynamic_pointer_cast<KnobDouble>( node->getKnobByName(name) );
    }

    // param of node = "<other>.otherParam.get()", where <other> is the script name of other, or the given name
    static bool setExpressionReading(const NodePtr& node,
                                     const std::string& param,
                                     const NodePtr& other,
                                     const std::string& otherParam,
                                     const std::string& otherName = std::string())
    {
        KnobDoublePtr k = knob(node, param);

        if (!k) {
            return false;
        }
        k->setExpression(0, ( otherName.empty() ? other->getScriptName_mt_safe() : otherName ) + "." + otherParam + ".get()",
                         false /*hasRetVariable*/, false /*failIfInvalid*/);

        return !k->getExpression(0).empty();
    }

    // 10 at the timeline's frame, 50 at the frame of the render
    static void setKeys(const KnobDoublePtr& k)
    {
        k->setValueAtTime(kTimelineTime, 10., ViewSpec::all(), 0);
        k->setValueAtTime(kRenderTime, 50., ViewSpec::all(), 0);
    }

    static bool dependsOn(const NodePtr& node,
                          const NodePtr& dep)
    {
        std::set<NodePtr> deps;

        node->getEffectInstance()->getAllExpressionDependenciesRecursive(deps);

        return deps.find(dep) != deps.end();
    }

    // The render args of the render that ParallelRenderArgsSetter sets up for the tree of root, as renderFrame() does
    static AbortableRenderInfoPtr makeAbortInfo()
    {
        return AbortableRenderInfo::create(false, 0);
    }

    static ParallelRenderArgsSetter* makeRenderArgs(const NodePtr& root,
                                                    const AbortableRenderInfoPtr& abortInfo)
    {
        return new ParallelRenderArgsSetter( kRenderTime, ViewIdx(0),
                                             true, // isRenderUserInteraction
                                             false, // isSequential
                                             abortInfo,
                                             root, // tree root
                                             0, // texture index
                                             root->getApp()->getTimeLine().get(),
                                             NodePtr(), // rotoPaint node
                                             false, // isAnalysis
                                             false, // isDraft
                                             RenderStatsPtr() );
    }

    static ParallelRenderArgsPtr renderArgs(const NodePtr& node)
    {
        return node->getEffectInstance()->getParallelRenderArgsTLS();
    }
};
} // anonymous namespace

// A's parameter reads B, which is not connected to A: during the render of A, B has the render args of that
// render, and A's expression reads B at the time of the render.
TEST_F(ExpressionDepsRenderArgsTest, DependencyGetsRenderArgs)
{
    if ( skipped() ) {
        return;
    }
    NodePtr a = createNode();
    NodePtr b = createNode();
    ASSERT_TRUE(a && b);
    KnobDoublePtr bFrom = knob(b, "doubleFrom");
    ASSERT_TRUE( bool(bFrom) );
    setKeys(bFrom);
    ASSERT_TRUE( setExpressionReading(a, "doubleTo", b, "doubleFrom") );
    EXPECT_FALSE( bool( renderArgs(b) ) );

    {
        AbortableRenderInfoPtr abortInfo = makeAbortInfo();
        std::unique_ptr<ParallelRenderArgsSetter> setter( makeRenderArgs(a, abortInfo) );
        ParallelRenderArgsPtr aArgs = renderArgs(a);
        ParallelRenderArgsPtr bArgs = renderArgs(b);
        ASSERT_TRUE( bool(aArgs) );
        ASSERT_TRUE( bool(bArgs) ) << "the node read by an expression of the rendered node has no render args";
        EXPECT_EQ(kRenderTime, bArgs->time);
        EXPECT_EQ(a, bArgs->treeRoot);
        EXPECT_EQ( abortInfo, bArgs->abortInfo.lock() );
        EXPECT_EQ(1, aArgs->visitsCount);
        // B is not an input of the tree
        EXPECT_EQ(0, bArgs->visitsCount);
        // B.doubleFrom.get() is read at the time of the render, not at the time of the timeline
        EXPECT_EQ( kRenderTime, b->getEffectInstance()->getCurrentTime() );
        EXPECT_EQ( 50., knob(a, "doubleTo")->getValueAtTime(kRenderTime) );
    }
    // The render args are removed at the end of the render
    EXPECT_FALSE( bool( renderArgs(a) ) );
    EXPECT_FALSE( bool( renderArgs(b) ) );
}

// The inputs of a node that only an expression reads are not part of the render; if that node is also upstream
// of the tree root, it is recursed through its inputs and its visits are counted as for any other input.
TEST_F(ExpressionDepsRenderArgsTest, DependencyInputs)
{
    if ( skipped() ) {
        return;
    }
    NodePtr a = createNode();
    NodePtr b = createNode();
    NodePtr c = createNode();
    ASSERT_TRUE(a && b && c);
    ProjectPtr project = a->getApp()->getProject();
    ASSERT_TRUE( project->connectNodes(0, c, b) ); // c -> b
    ASSERT_TRUE( setExpressionReading(a, "doubleTo", b, "doubleFrom") );

    {
        AbortableRenderInfoPtr abortInfo = makeAbortInfo();
        std::unique_ptr<ParallelRenderArgsSetter> setter( makeRenderArgs(a, abortInfo) );
        EXPECT_TRUE( bool( renderArgs(b) ) );
        EXPECT_FALSE( bool( renderArgs(c) ) ) << "the input of a node read by an expression is not rendered";
    }
    EXPECT_FALSE( bool( renderArgs(b) ) );

    // c -> b -> a: b is both read by the expression of a (added first) and an input of a
    ASSERT_TRUE( project->connectNodes(0, b, a) );
    {
        AbortableRenderInfoPtr abortInfo = makeAbortInfo();
        std::unique_ptr<ParallelRenderArgsSetter> setter( makeRenderArgs(a, abortInfo) );
        ParallelRenderArgsPtr bArgs = renderArgs(b);
        ParallelRenderArgsPtr cArgs = renderArgs(c);
        ASSERT_TRUE( bool(bArgs) );
        ASSERT_TRUE( bool(cArgs) ) << "the inputs of an input that an expression also reads were not visited";
        EXPECT_EQ(1, bArgs->visitsCount);
        EXPECT_EQ(1, cArgs->visitsCount);
    }
    EXPECT_FALSE( bool( renderArgs(b) ) );
    EXPECT_FALSE( bool( renderArgs(c) ) );
}

// The expressions of a node that is first reached as an expression dependency, then as an input, are followed:
// a -> root, root reads a.doubleFrom, a.doubleTo reads d.
TEST_F(ExpressionDepsRenderArgsTest, ExpressionsOfDependencyThatIsAlsoAnInput)
{
    if ( skipped() ) {
        return;
    }
    NodePtr root = createNode();
    NodePtr a = createNode();
    NodePtr d = createNode();
    ASSERT_TRUE(root && a && d);
    ASSERT_TRUE( root->getApp()->getProject()->connectNodes(0, a, root) );
    ASSERT_TRUE( setExpressionReading(root, "doubleTo", a, "doubleFrom") );
    ASSERT_TRUE( setExpressionReading(a, "doubleTo", d, "doubleFrom") );
    {
        AbortableRenderInfoPtr abortInfo = makeAbortInfo();
        std::unique_ptr<ParallelRenderArgsSetter> setter( makeRenderArgs(root, abortInfo) );
        ASSERT_TRUE( bool( renderArgs(a) ) );
        EXPECT_EQ(1, renderArgs(a)->visitsCount);
        EXPECT_TRUE( bool( renderArgs(d) ) ) << "the node read by an expression of an input has no render args";
    }
    EXPECT_FALSE( bool( renderArgs(a) ) );
    EXPECT_FALSE( bool( renderArgs(d) ) );
}

// Cycles through expressions: a node reading itself, a node reading a node downstream, and two nodes reading
// each other. The render setup must not hang, and the render args are all removed at the end.
TEST_F(ExpressionDepsRenderArgsTest, Cycles)
{
    if ( skipped() ) {
        return;
    }
    NodePtr a = createNode();
    NodePtr b = createNode();
    NodePtr down = createNode();
    ASSERT_TRUE(a && b && down);
    ASSERT_TRUE( a->getApp()->getProject()->connectNodes(0, a, down) ); // a -> down
    // a reads itself
    ASSERT_TRUE( setExpressionReading(a, "doubleTo", a, "doubleFrom") );
    // a reads a node downstream of it
    ASSERT_TRUE( setExpressionReading(a, "double2DFrom", down, "doubleFrom") );
    // a and b read each other (different parameters)
    ASSERT_TRUE( setExpressionReading(a, "double3DFrom", b, "doubleTo") );
    ASSERT_TRUE( setExpressionReading(b, "doubleTo", a, "doubleFrom") );

    const NodePtr roots[] = { a, down, b };
    for (int i = 0; i < 3; ++i) {
        {
            AbortableRenderInfoPtr abortInfo = makeAbortInfo();
            std::unique_ptr<ParallelRenderArgsSetter> setter( makeRenderArgs(roots[i], abortInfo) );
            ParallelRenderArgsPtr aArgs = renderArgs(a);
            ParallelRenderArgsPtr bArgs = renderArgs(b);
            ParallelRenderArgsPtr downArgs = renderArgs(down);
            ASSERT_TRUE( aArgs && bArgs );
            EXPECT_EQ(roots[i], aArgs->treeRoot);
            EXPECT_EQ(roots[i], bArgs->treeRoot);
            if (roots[i] == b) {
                // b reads a.doubleFrom, which has no expression: down is not part of that render
                EXPECT_FALSE( bool(downArgs) );
            } else {
                ASSERT_TRUE( bool(downArgs) );
                EXPECT_EQ(roots[i], downArgs->treeRoot);
            }
            // a is visited once when it is upstream of the root (it reads itself and down, which is downstream)
            EXPECT_EQ(roots[i] == b ? 0 : 1, aArgs->visitsCount);
        }
        EXPECT_FALSE( bool( renderArgs(a) ) );
        EXPECT_FALSE( bool( renderArgs(b) ) );
        EXPECT_FALSE( bool( renderArgs(down) ) );
    }
}

// Two parameters that read each other through their expressions (a.doubleTo = b.doubleTo, b.doubleTo =
// a.doubleTo). The render setup must not hang or crash.
TEST_F(ExpressionDepsRenderArgsTest, ParameterCycle)
{
    if ( skipped() ) {
        return;
    }
    NodePtr a = createNode();
    NodePtr b = createNode();
    ASSERT_TRUE(a && b);
    ASSERT_TRUE( setExpressionReading(a, "doubleTo", b, "doubleTo") );
    ASSERT_TRUE( setExpressionReading(b, "doubleTo", a, "doubleTo") );
    ASSERT_TRUE( dependsOn(a, b) && dependsOn(b, a) );
    {
        AbortableRenderInfoPtr abortInfo = makeAbortInfo();
        std::unique_ptr<ParallelRenderArgsSetter> setter( makeRenderArgs(a, abortInfo) );
        EXPECT_TRUE( bool( renderArgs(a) ) );
        EXPECT_TRUE( bool( renderArgs(b) ) );
    }
    EXPECT_FALSE( bool( renderArgs(a) ) );
    EXPECT_FALSE( bool( renderArgs(b) ) );
}

// A node inside a Group reads a parameter of the Group (thisGroup.shift.get(), as in PyPlugs): the Group gets
// render args, and the parameter is read at the frame of the render.
TEST_F(ExpressionDepsRenderArgsTest, GroupParameter)
{
    if ( skipped() ) {
        return;
    }
    NodePtr groupNode = createNode(PLUGINID_NATRON_GROUP);
    ASSERT_TRUE( bool(groupNode) );
    NodeGroupPtr group = std::dynamic_pointer_cast<NodeGroup>( groupNode->getEffectInstance() );
    ASSERT_TRUE( bool(group) );
    KnobDoublePtr shift = AppManager::createKnob<KnobDouble>(group.get(), std::string("shift"), 1);
    shift->setName("shift");
    groupNode->declarePythonFields();
    setKeys(shift);
    NodePtr inside = createNode(kNatronTestPluginParams, group);
    ASSERT_TRUE( bool(inside) );
    ASSERT_TRUE( setExpressionReading(inside, "doubleTo", groupNode, "shift", "thisGroup") );
    ASSERT_TRUE( dependsOn(inside, groupNode) );
    {
        AbortableRenderInfoPtr abortInfo = makeAbortInfo();
        std::unique_ptr<ParallelRenderArgsSetter> setter( makeRenderArgs(inside, abortInfo) );
        ParallelRenderArgsPtr groupArgs = renderArgs(groupNode);
        ASSERT_TRUE( bool(groupArgs) ) << "the Group read by an expression of a node inside it has no render args";
        EXPECT_EQ(kRenderTime, groupArgs->time);
        EXPECT_EQ( 50., knob(inside, "doubleTo")->getValueAtTime(kRenderTime) );
    }
    EXPECT_FALSE( bool( renderArgs(groupNode) ) );
}

// A deactivated node (deleted, kept for undo) that an expression still reads gets no render args.
TEST_F(ExpressionDepsRenderArgsTest, DeactivatedDependency)
{
    if ( skipped() ) {
        return;
    }
    NodePtr a = createNode();
    NodePtr b = createNode();
    ASSERT_TRUE(a && b);
    ASSERT_TRUE( setExpressionReading(a, "doubleTo", b, "doubleFrom") );
    b->deactivate(std::list<NodePtr>(), true /*disconnectAll*/, false /*reconnect*/, true /*hideGui*/, false /*triggerRender*/, false /*unslaveKnobs*/);
    ASSERT_FALSE( b->isActivated() );
    ASSERT_TRUE( dependsOn(a, b) );
    {
        AbortableRenderInfoPtr abortInfo = makeAbortInfo();
        std::unique_ptr<ParallelRenderArgsSetter> setter( makeRenderArgs(a, abortInfo) );
        EXPECT_TRUE( bool( renderArgs(a) ) );
        EXPECT_FALSE( bool( renderArgs(b) ) ) << "a deactivated node got render args";
    }
    b->activate(std::list<NodePtr>(), true /*restoreAll*/, false /*triggerRender*/);
}

// A render on disk (through a DiskCache, which renders like a Write node) of a generator whose region of definition
// depends on a parameter that reads another node, not connected to it, which reads the generator back. The RoD and
// the render read that node at the frame of the render, not at the timeline's frame: the RoD used to be computed
// before the render args were set, and its result was then used by the render.
TEST_F(ExpressionDepsRenderArgsTest, RenderOnDisk)
{
    if ( skipped() || skipped(kNatronTestPluginParamRoD) ) {
        return;
    }
    NodePtr gen = createNode(kNatronTestPluginParamRoD);
    NodePtr b = createNode();
    NodePtr output = createNode(PLUGINID_NATRON_DISKCACHE);
    ASSERT_TRUE(gen && b && output);
    ASSERT_TRUE( gen->getApp()->getProject()->connectNodes(0, gen, output) );
    KnobDoublePtr bFrom = knob(b, "doubleFrom");
    ASSERT_TRUE( bool(bFrom) );
    setKeys(bFrom);
    ASSERT_TRUE( setExpressionReading(gen, "size", b, "doubleFrom") );
    ASSERT_TRUE( setExpressionReading(b, "doubleTo", gen, "size") );
    appPTR->clearAllCaches();
    const int rod50 = NatronTestPlugin::counter("ParamRoD.rod.50");
    const int rod10 = NatronTestPlugin::counter("ParamRoD.rod.10");
    const int render50 = NatronTestPlugin::counter("ParamRoD.render.50");
    const int width50 = NatronTestPlugin::counter("ParamRoD.renderWidth.50");
    NatronTestPlugin::renderSequence( output, (int)kRenderTime, (int)kRenderTime );
    EXPECT_LE( 1, NatronTestPlugin::counter("ParamRoD.rod.50") - rod50 );
    EXPECT_EQ( 0, NatronTestPlugin::counter("ParamRoD.rod.10") - rod10 ) << "the RoD read the other node at the timeline's frame";
    EXPECT_EQ( 1, NatronTestPlugin::counter("ParamRoD.render.50") - render50 );
    EXPECT_EQ( 1, NatronTestPlugin::counter("ParamRoD.renderWidth.50") - width50 );
    EXPECT_FALSE( bool( renderArgs(gen) ) );
    EXPECT_FALSE( bool( renderArgs(b) ) );
}
