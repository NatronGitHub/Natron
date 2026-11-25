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

// A project loads the major version of the decoder of a Read node, and of the encoder of a Write node,
// that it was saved with, not the newest one (#1681). The test plug-ins of
// Tests/OfxTestPlugin/NatronTestPluginVersions.cpp are a reader and a writer in versions 90.0 and 91.0.
// A project saved without the versions (by a Natron older than 2.6) loads the lowest major.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>

#include <gtest/gtest.h>

#include <ofxImageEffect.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/ReadNode.h"
#include "Engine/WriteNode.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace {
const int kOldMajor = 90;
const int kNewestMajor = 91;

bool
versionsAvailable()
{
    const char* const ids[] = { PLUGINID_OFX_READCDR, PLUGINID_OFX_WRITEPFM };

    for (int i = 0; i < 2; ++i) {
        try {
            Plugin* oldPlugin = appPTR->getPluginBinary(QString::fromUtf8(ids[i]), kOldMajor, 0, false);
            Plugin* newestPlugin = appPTR->getPluginBinary(QString::fromUtf8(ids[i]), -1, -1, false);
            if ( !oldPlugin || !newestPlugin ) {
                NatronTestPlugin::reportSkipped(ids[i]);

                return false;
            }
            if ( (oldPlugin->getMajorVersion() != kOldMajor) || (newestPlugin->getMajorVersion() != kNewestMajor) ) {
                // e.g. a real plug-in with that ID and a major of 91 or more is installed
                NatronTest::skip(std::string("the versions of ") + ids[i] + " are not those of the test plug-ins");

                return false;
            }
        } catch (...) {
            NatronTestPlugin::reportSkipped(ids[i]);

            return false;
        }
    }

    return true;
}

// a Read or Write node of the project, as app.createNode(pluginID, major) creates it (major -1: the newest)
NodePtr
createIONode(const char* pluginID,
             int major,
             const std::string& filename)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( pluginID, app->getProject() );

    args.setProperty<int>(kCreateNodeArgsPropPluginVersion, major, 0);
    args.setProperty<int>(kCreateNodeArgsPropPluginVersion, major == -1 ? -1 : 0, 1);
    args.addParamDefaultValue<std::string>(kOfxImageEffectFileParamName, filename);
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);

    return app->createNode(args);
}

// the lowest registered major of a plug-in, or -1: e.g. 1 for WritePFM when openfx-io is loaded, as on CI
int
lowestMajor(const char* pluginID)
{
    const PluginsMap& plugins = appPTR->getPluginsList();
    PluginsMap::const_iterator found = plugins.find(pluginID);

    if ( ( found == plugins.end() ) || found->second.empty() ) {
        return -1;
    }

    return ( *found->second.begin() )->getMajorVersion(); // ordered by increasing version
}

// Replaces every occurrence of a text in a file
bool
replaceInFile(const QString& filePath,
              const QByteArray& before,
              const QByteArray& after)
{
    QFile file(filePath);

    if ( !file.open(QIODevice::ReadOnly) ) {
        return false;
    }
    QByteArray contents = file.readAll();
    file.close();
    if ( !contents.contains(before) ) {
        return false;
    }
    contents.replace(before, after);
    if ( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) ) {
        return false;
    }

    return file.write(contents) == contents.size();
}

// the major version of the decoder or encoder of a Read or Write node, or -1
int
embeddedMajor(const NodePtr& node)
{
    if (!node) {
        return -1;
    }
    NodePtr embedded;
    if ( ReadNode* read = dynamic_cast<ReadNode*>( node->getEffectInstance().get() ) ) {
        embedded = read->getEmbeddedReader();
    } else if ( WriteNode* write = dynamic_cast<WriteNode*>( node->getEffectInstance().get() ) ) {
        embedded = write->getEmbeddedWriter();
    }
    const Plugin* plugin = embedded ? embedded->getPlugin() : 0;

    return plugin ? plugin->getMajorVersion() : -1;
}

// the value of the plug-in ID parameter of a Read or Write node, or "<no node>"
std::string
pluginIDKnobValue(const NodePtr& node,
                  const char* knobName)
{
    if (!node) {
        return "<no node>";
    }
    KnobStringPtr knob = std::dynamic_pointer_cast<KnobString>( node->getKnobByName(knobName) );

    return knob ? knob->getValue() : "<no knob>";
}

// Resets the shared app's project when the test ends, also when an ASSERT fails, so that the next
// tests get an untitled empty project, not one in a removed directory.
struct ResetProjectOnExit
{
    ~ResetProjectOnExit()
    {
        appPTR->getTopLevelInstance()->getProject()->resetProject();
    }
};
} // anonymous namespace

TEST(ReadWriteVersion, ProjectLoadsSavedVersion)
{
    if ( !versionsAvailable() ) {
        return;
    }
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const ResetProjectOnExit resetProject; // destroyed before tmpDir
    ProjectPtr project = appPTR->getTopLevelInstance()->getProject();
    const QString dir = tmpDir.path() + QLatin1Char('/');
    const std::string filename = dir.toStdString() + "image.natrontestversion";

    NodePtr oldRead = createIONode(PLUGINID_OFX_READCDR, kOldMajor, filename);
    NodePtr newestRead = createIONode(PLUGINID_OFX_READCDR, -1, filename);
    NodePtr oldWrite = createIONode(PLUGINID_OFX_WRITEPFM, kOldMajor, filename);
    NodePtr newestWrite = createIONode(PLUGINID_OFX_WRITEPFM, -1, filename);
    // a major that is not installed: the closest one above
    NodePtr missingRead = createIONode(PLUGINID_OFX_READCDR, kOldMajor - 1, filename);
    ASSERT_TRUE(oldRead && newestRead && oldWrite && newestWrite && missingRead);
    EXPECT_EQ( kOldMajor, embeddedMajor(missingRead) );
    EXPECT_EQ( kOldMajor, embeddedMajor(oldRead) );
    EXPECT_EQ( kNewestMajor, embeddedMajor(newestRead) );
    EXPECT_EQ( kOldMajor, embeddedMajor(oldWrite) );
    EXPECT_EQ( kNewestMajor, embeddedMajor(newestWrite) );

    const std::string oldReadName = oldRead->getScriptName();
    const std::string newestReadName = newestRead->getScriptName();
    const std::string oldWriteName = oldWrite->getScriptName();
    const std::string newestWriteName = newestWrite->getScriptName();

    // setting the version parameter (e.g. an exported PyPlug does) does not change the decoder or
    // encoder, so the parameter keeps its version: the project must not load another one
    KnobIntPtr readVersion = std::dynamic_pointer_cast<KnobInt>( newestRead->getKnobByName(kNatronReadNodeParamDecodingPluginVersion) );
    KnobIntPtr writeVersion = std::dynamic_pointer_cast<KnobInt>( newestWrite->getKnobByName(kNatronWriteNodeParamEncodingPluginVersion) );
    ASSERT_TRUE(readVersion && writeVersion);
    readVersion->setValue(kOldMajor);
    writeVersion->setValue(kOldMajor);
    EXPECT_EQ( kNewestMajor, readVersion->getValue() );
    EXPECT_EQ( kNewestMajor, writeVersion->getValue() );
    readVersion.reset();
    writeVersion.reset();
    QString savedPath;
    ASSERT_TRUE( project->saveProject( dir, QString::fromUtf8("ReadWriteVersion.ntp"), &savedPath ) );
    oldRead.reset();
    newestRead.reset();
    oldWrite.reset();
    newestWrite.reset();
    missingRead.reset();

    ASSERT_TRUE( project->loadProject( dir, QString::fromUtf8("ReadWriteVersion.ntp") ) );
    EXPECT_EQ( kOldMajor, embeddedMajor( project->getNodeByName(oldReadName) ) );
    EXPECT_EQ( kNewestMajor, embeddedMajor( project->getNodeByName(newestReadName) ) );
    EXPECT_EQ( kOldMajor, embeddedMajor( project->getNodeByName(oldWriteName) ) );
    EXPECT_EQ( kNewestMajor, embeddedMajor( project->getNodeByName(newestWriteName) ) );
}

// A project saved by a Natron older than 2.6 has no version parameter: it loads the lowest major of
// the decoder and encoder, which is the one those versions created (e.g. ReadSVG 3, openfx-arena#224),
// while a new node still gets the newest.
TEST(ReadWriteVersion, ProjectWithoutVersionLoadsLowestMajor)
{
    if ( !versionsAvailable() ) {
        return;
    }
    // 90, or the major of the real plug-in with that ID if it is loaded (e.g. openfx-io's WritePFM 1)
    const int readLowest = lowestMajor(PLUGINID_OFX_READCDR);
    const int writeLowest = lowestMajor(PLUGINID_OFX_WRITEPFM);
    ASSERT_TRUE( (0 <= readLowest) && (readLowest <= kOldMajor) );
    ASSERT_TRUE( (0 <= writeLowest) && (writeLowest <= kOldMajor) );
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const ResetProjectOnExit resetProject; // destroyed before tmpDir
    ProjectPtr project = appPTR->getTopLevelInstance()->getProject();
    const QString dir = tmpDir.path() + QLatin1Char('/');
    const std::string filename = dir.toStdString() + "image.natrontestversion";

    NodePtr read = createIONode(PLUGINID_OFX_READCDR, -1, filename);
    NodePtr write = createIONode(PLUGINID_OFX_WRITEPFM, -1, filename);
    ASSERT_TRUE(read && write);
    EXPECT_EQ( kNewestMajor, embeddedMajor(read) );
    EXPECT_EQ( kNewestMajor, embeddedMajor(write) );
    const std::string readName = read->getScriptName();
    const std::string writeName = write->getScriptName();

    // save the project as an older Natron did: without the version parameters
    KnobIPtr readVersion = read->getKnobByName(kNatronReadNodeParamDecodingPluginVersion);
    KnobIPtr writeVersion = write->getKnobByName(kNatronWriteNodeParamEncodingPluginVersion);
    ASSERT_TRUE(readVersion && writeVersion);
    readVersion->setIsPersistent(false);
    writeVersion->setIsPersistent(false);
    readVersion.reset();
    writeVersion.reset();
    QString savedPath;
    ASSERT_TRUE( project->saveProject( dir, QString::fromUtf8("ReadWriteNoVersion.ntp"), &savedPath ) );
    read.reset();
    write.reset();

    ASSERT_TRUE( project->loadProject( dir, QString::fromUtf8("ReadWriteNoVersion.ntp") ) );
    EXPECT_EQ( readLowest, embeddedMajor( project->getNodeByName(readName) ) );
    EXPECT_EQ( writeLowest, embeddedMajor( project->getNodeByName(writeName) ) );

    // a new node in the loaded project gets the newest major
    EXPECT_EQ( kNewestMajor, embeddedMajor( createIONode(PLUGINID_OFX_READCDR, -1, filename) ) );
    EXPECT_EQ( kNewestMajor, embeddedMajor( createIONode(PLUGINID_OFX_WRITEPFM, -1, filename) ) );
}

// A project loaded where its decoder and encoder are not installed keeps their IDs and versions: saved there
// and loaded again where they are installed, it gets the plug-ins and versions it had, not the lowest ones.
TEST(ReadWriteVersion, MissingPluginKeepsVersion)
{
    if ( !versionsAvailable() ) {
        return;
    }
    // Without the decoder or encoder, a Read or Write node needs the default reader and writer of
    // IO.ofx.bundle (openfx-io), else the node is not loaded (e.g. macOS CI has no openfx-io)
    const char* const defaultIDs[] = { PLUGINID_OFX_READOIIO, PLUGINID_OFX_WRITEOIIO };
    for (const char* id : defaultIDs) {
        if (lowestMajor(id) < 0) {
            NatronTest::skip(std::string("plug-in ") + id + " not found (needs openfx-io in OFX_PLUGIN_PATH)");

            return;
        }
    }
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const ResetProjectOnExit resetProject; // destroyed before tmpDir
    ProjectPtr project = appPTR->getTopLevelInstance()->getProject();
    const QString dir = tmpDir.path() + QLatin1Char('/');
    const QString projectName = QString::fromUtf8("ReadWriteMissing.ntp");
    const QString projectPath = dir + projectName;
    const std::string filename = dir.toStdString() + "image.natrontestversion";
    // the IDs as they appear in the XML text of the project, and IDs of plug-ins that are not installed
    const QByteArray readID(PLUGINID_OFX_READCDR), writeID(PLUGINID_OFX_WRITEPFM);
    const QByteArray missingReadID = readID + "Missing", missingWriteID = writeID + "Missing";
    // the file name too: the test decoder and encoder read and write "natrontestversion" files, so with
    // only the IDs renamed the Read and Write nodes would create them again, from the file extension
    const QByteArray extension(".natrontestversion"), missingExtension(".natrontestmissing");

    NodePtr read = createIONode(PLUGINID_OFX_READCDR, -1, filename);
    NodePtr write = createIONode(PLUGINID_OFX_WRITEPFM, -1, filename);
    ASSERT_TRUE(read && write);
    EXPECT_EQ( kNewestMajor, embeddedMajor(read) );
    EXPECT_EQ( kNewestMajor, embeddedMajor(write) );
    const std::string readName = read->getScriptName();
    const std::string writeName = write->getScriptName();
    read.reset();
    write.reset();
    QString savedPath;
    ASSERT_TRUE( project->saveProject( dir, projectName, &savedPath, eProjectFileFormatText ) );

    // where the decoder and encoder are not installed: load and save
    ASSERT_TRUE( replaceInFile(projectPath, readID, missingReadID) );
    ASSERT_TRUE( replaceInFile(projectPath, writeID, missingWriteID) );
    ASSERT_TRUE( replaceInFile(projectPath, extension, missingExtension) );
    ASSERT_TRUE( project->loadProject(dir, projectName) );
    // the Read and Write nodes keep the IDs of the missing decoder and encoder, with their version (#1825):
    // before, the ID became that of the plug-in for the file extension (none here), while the version
    // stayed, so the next load gave that version to another plug-in
    EXPECT_EQ( std::string( missingReadID.constData() ), pluginIDKnobValue( project->getNodeByName(readName), kNatronReadNodeParamDecodingPluginID ) );
    EXPECT_EQ( std::string( missingWriteID.constData() ), pluginIDKnobValue( project->getNodeByName(writeName), kNatronWriteNodeParamEncodingPluginID ) );
    ASSERT_TRUE( project->saveProject( dir, projectName, &savedPath, eProjectFileFormatText ) );

    // where they are installed again: the IDs and the versions were saved, so the Read and Write nodes
    // create the same decoder and encoder, in the saved version: the newest, not the lowest
    ASSERT_TRUE( replaceInFile(projectPath, missingReadID, readID) );
    ASSERT_TRUE( replaceInFile(projectPath, missingWriteID, writeID) );
    ASSERT_TRUE( replaceInFile(projectPath, missingExtension, extension) );
    ASSERT_TRUE( project->loadProject(dir, projectName) );
    EXPECT_EQ( kNewestMajor, embeddedMajor( project->getNodeByName(readName) ) );
    EXPECT_EQ( kNewestMajor, embeddedMajor( project->getNodeByName(writeName) ) );
}
