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

// Pasted nodes and the nodes of a new PyPlug instance are created and connected as one node tree
// (CreatingNodeTreeFlag_RAII), during which the hashes are not computed after each connection. They were not computed
// when the tree was complete either (#1847): a node kept the hash it had before its inputs were connected, so that
// after a disconnection its hash didn't change, and the images cached while it was connected were used.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include <gtest/gtest.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeSerialization.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"

#include "OfxTestPlugin.h"
#include "TestUtils.h"

NATRON_NAMESPACE_USING

namespace {
bool
canRunTest()
{
    for (const char* id : { kNatronTestPluginGenerator, kNatronTestPluginMultiPAR }) {
        if ( !NatronTestPlugin::isAvailable(id) ) {
            NatronTestPlugin::reportMissing(id);

            return false;
        }
    }

    return true;
}

// A small project format, so that the renders are quick; the project is reset at the end
struct SmallProject
{
    ProjectPtr project;

    SmallProject()
        : project( appPTR->getTopLevelInstance()->getProject() )
    {
        project->reset(false, true);
        // setOrAddProjectFormat() only changes the format if it may be auto-set, a user setting
        project->setAutoSetProjectFormatEnabled(true);
        project->setOrAddProjectFormat( Format(0, 0, 16, 16, "NodeTreeHash", 1.) );
    }

    ~SmallProject()
    {
        project->reset(false, true);
    }
};

// A copy of node made from its serialization, as NodeGraphPrivate::pasteNode() makes it
NodePtr
pasteNode(const NodePtr& node)
{
    const NodeSerializationPtr serialization = std::make_shared<NodeSerialization>(node);
    AppInstancePtr app = node->getApp();
    CreateNodeArgs args( serialization->getPluginID(), app->getProject() );

    args.setProperty<NodeSerializationPtr>(kCreateNodeArgsPropNodeSerialization, serialization);
    args.setProperty<int>(kCreateNodeArgsPropPluginVersion, serialization->getPluginMajorVersion(), 0);
    args.setProperty<int>(kCreateNodeArgsPropPluginVersion, serialization->getPluginMinorVersion(), 1);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);

    return app->createNode(args);
}

// Renders filter, connected to generator, then disconnects it: its hash must change, and its render must not be the
// image cached while it was connected.
void
checkDisconnectionChangesHash(const NodePtr& generator,
                              const NodePtr& filter)
{
    ASSERT_EQ( generator, filter->getInput(0) );
    EXPECT_TRUE( bool( NatronTestPlugin::renderFrame(filter, 1.) ) );
    const U64 connectedHash = filter->getHashValue();

    ASSERT_TRUE( Project::disconnectNodes(generator, filter) );
    EXPECT_NE( connectedHash, filter->getHashValue() ) << "the hash is the one the node had before its input was connected";
    // Without its Source, the filter renders nothing (an empty region of definition) or a new image: not the image
    // cached while it was connected
    const int rendersBefore = NatronTestPlugin::counter("MultiPAR.render");
    const ImagePtr image = NatronTestPlugin::renderFrame(filter, 1.);
    EXPECT_TRUE( !image || (NatronTestPlugin::counter("MultiPAR.render") > rendersBefore) )
        << "the image cached while the node was connected was used";
}
} // anon namespace

// Paste a Generator and a filter connected to it, as NodeGraphPrivate::pasteNodesInternal() does: the nodes are
// created from their serializations, then connected, as one node tree.
TEST(NodeTreeHash, PastedNodes)
{
    if ( !canRunTest() ) {
        return;
    }
    SmallProject smallProject;
    NatronTest::NodeScope scope;
    AppInstancePtr app = appPTR->getTopLevelInstance();
    const NodePtr generator = scope.create(kNatronTestPluginGenerator);
    const NodePtr filter = scope.create(kNatronTestPluginMultiPAR);
    ASSERT_TRUE(generator && filter);
    ASSERT_TRUE( smallProject.project->connectNodes(0, generator, filter) );

    NodePtr pastedGenerator, pastedFilter;
    {
        CreatingNodeTreeFlag_RAII createNodeTree(app);
        pastedGenerator = scope.add( pasteNode(generator) );
        pastedFilter = scope.add( pasteNode(filter) );
        ASSERT_TRUE(pastedGenerator && pastedFilter);
        ASSERT_TRUE( NodeCollection::connectNodes(0, pastedGenerator, pastedFilter) );
    }
    smallProject.project->forceComputeInputDependentDataOnAllTrees();

    checkDisconnectionChangesHash(pastedGenerator, pastedFilter);
}

// A PyPlug whose instance has a Generator and a filter connected to it: they are created and connected by
// createInstance(), as one node tree.
TEST(NodeTreeHash, PyPlugInstance)
{
    if ( !canRunTest() ) {
        return;
    }
    SmallProject smallProject;
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    const char* const pyPlugID = "net.natron.test.NodeTreeHashPyPlug";
    const QString moduleName = QString::fromUtf8("NatronTestNodeTreeHashPyPlug");
    const std::string script = std::string("from NatronEngine import *\n"
                                           "def getPluginID():\n"
                                           "    return '") + pyPlugID + "'\n"
                               "def getLabel():\n"
                               "    return 'NodeTreeHashPyPlug'\n"
                               "def getVersion():\n"
                               "    return 1\n"
                               "def createInstance(app, group):\n"
                               "    gen = app.createNode('" kNatronTestPluginGenerator "', -1, group)\n"
                               "    gen.setScriptName('gen')\n"
                               "    f = app.createNode('" kNatronTestPluginMultiPAR "', -1, group)\n"
                               "    f.setScriptName('filter')\n"
                               "    f.connectInput(0, gen)\n"
                               // cached under the hash of the filter before its input was connected
                               "    f.getRegionOfDefinition(1, 0)\n"
                               "    out = app.createNode('" + PLUGINID_NATRON_OUTPUT + "', -1, group)\n"
                               "    out.connectInput(0, f)\n";
    {
        QFile file( dir + moduleName + QString::fromUtf8(".py") );
        ASSERT_TRUE( file.open(QIODevice::WriteOnly | QIODevice::Truncate) );
        ASSERT_EQ( (qint64)script.size(), file.write( script.c_str(), (qint64)script.size() ) );
    }
    const std::string pathDir = tmpDir.path().toStdString();
    std::string err, output;
    ASSERT_TRUE( NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("import sys\nsys.path.insert(0, r'" + pathDir + "')\n", &err, &output) ) << err;
    Plugin* plugin = appPTR->registerPlugin( dir, QStringList() << QString::fromUtf8("Other") << QString::fromUtf8("Test"),
                                             QString::fromUtf8(pyPlugID), QString::fromUtf8("NodeTreeHashPyPlug"), QString(), QStringList(),
                                             false, false, 0, false, 1, 0, false );
    ASSERT_TRUE(plugin);
    plugin->setPythonModule(dir + moduleName);

    {
        NatronTest::NodeScope scope;
        const NodePtr pyPlug = scope.add( NatronTest::createNode(pyPlugID) );
        ASSERT_TRUE( bool(pyPlug) );
        NodeGroup* group = pyPlug->isEffectGroup();
        ASSERT_TRUE(group);
        const NodePtr generator = group->getNodeByName("gen");
        const NodePtr filter = group->getNodeByName("filter");
        ASSERT_TRUE(generator && filter);

        RectD connectedRoD;
        ASSERT_EQ( eStatusOK, filter->getEffectInstance()->getRegionOfDefinition_public(filter->getHashValue(), 1., RenderScale::identity, ViewIdx(0), &connectedRoD, nullptr) );

        checkDisconnectionChangesHash(generator, filter);

        // The RoD that createInstance() asked for, connected, is not the RoD of the disconnected filter: that of a new
        // filter without inputs
        const NodePtr unconnected = scope.create(kNatronTestPluginMultiPAR);
        ASSERT_TRUE( bool(unconnected) );
        RectD unconnectedRoD, rod;
        const StatusEnum unconnectedStat = unconnected->getEffectInstance()->getRegionOfDefinition_public(unconnected->getHashValue(), 1., RenderScale::identity, ViewIdx(0), &unconnectedRoD, nullptr);
        const StatusEnum stat = filter->getEffectInstance()->getRegionOfDefinition_public(filter->getHashValue(), 1., RenderScale::identity, ViewIdx(0), &rod, nullptr);
        if ( (unconnectedStat == eStatusOK) && (unconnectedRoD == connectedRoD) ) {
            NatronTest::skipPart("the RoD of the filter doesn't depend on its input");
        } else {
            EXPECT_EQ(unconnectedStat, stat);
            EXPECT_EQ(unconnectedRoD, rod) << "the RoD cached while the filter was connected was used";
        }
    }

    EXPECT_TRUE( NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::interpretPythonScript("import sys\n"
                                                                "sys.path.remove(r'" + pathDir + "')\n"
                                                                "sys.modules.pop('" + moduleName.toStdString() + "', None)\n", &err, &output) ) << err;
}
