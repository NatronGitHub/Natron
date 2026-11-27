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

// The decoder of a Read node and the encoder of a Write node, chosen in their Decoder/Encoder menu, with
// the openfx-io plug-ins and PNG files (ReadPNG and WritePNG, or ReadOIIO and WriteOIIO): a choice that
// isn't the default plug-in of the file extension is kept when the project is saved and loaded (#1783). The
// parameters of a Read node set before its first file name are kept when that file creates its decoder (#1784).

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
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/ReadNode.h"
#include "Engine/WriteNode.h"

#include "TestUtils.h"

NATRON_NAMESPACE_USING

namespace {
// a 1x1 RGBA PNG image
const char kPNG[] =
    "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a\x00\x00\x00\x0d\x49\x48\x44\x52\x00\x00\x00\x01\x00\x00\x00\x01\x08\x06\x00\x00"
    "\x00\x1f\x15\xc4\x89\x00\x00\x00\x0d\x49\x44\x41\x54\x78\x9c\x63\x68\x70\x50\xf8\x0f\x00\x04\x04\x01\xe0\xf2\xeb"
    "\x80\xf9\x00\x00\x00\x00\x49\x45\x4e\x44\xae\x42\x60\x82";

bool
writePNG(const QString& path)
{
    QFile file(path);

    return file.open(QIODevice::WriteOnly) && ( file.write(kPNG, sizeof(kPNG) - 1) == qint64(sizeof(kPNG) - 1) );
}

// A Read (Write) node of the project, with a file name or none
NodePtr
createIONode(const char* pluginID,
             const std::string& filename)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( pluginID, app->getProject() );

    if ( !filename.empty() ) {
        args.addParamDefaultValue<std::string>(kOfxImageEffectFileParamName, filename);
    } else {
        // without a file name, a node that isn't created silently asks for a file (and fails without a GUI)
        args.setProperty<bool>(kCreateNodeArgsPropSilent, true);
    }
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);

    return app->createNode(args);
}

// The ID of the decoder (encoder) of a Read (Write) node, or "<none>"
std::string
embeddedPluginID(const NodePtr& node)
{
    NodePtr embedded;

    if (node) {
        if ( ReadNode* read = dynamic_cast<ReadNode*>( node->getEffectInstance().get() ) ) {
            embedded = read->getEmbeddedReader();
        } else if ( WriteNode* write = dynamic_cast<WriteNode*>( node->getEffectInstance().get() ) ) {
            embedded = write->getEmbeddedWriter();
        }
    }

    return embedded ? embedded->getPluginID() : std::string("<none>");
}

// The ID of the active entry of the Decoder (Encoder) menu, or "<none>"
std::string
menuEntryID(const NodePtr& node,
            const char* knobName)
{
    KnobChoicePtr knob = node ? std::dynamic_pointer_cast<KnobChoice>( node->getKnobByName(knobName) ) : KnobChoicePtr();

    return knob ? knob->getActiveEntry().id : std::string("<none>");
}

// The value of the plug-in ID parameter of a Read (Write) node, or "<none>"
std::string
pluginIDKnobValue(const NodePtr& node,
                  const char* knobName)
{
    KnobStringPtr knob = node ? std::dynamic_pointer_cast<KnobString>( node->getKnobByName(knobName) ) : KnobStringPtr();

    return knob ? knob->getValue() : std::string("<none>");
}

// A bundled decoder (encoder) of the "png" extension that isn't its default one, or an empty string
std::string
nonDefaultPluginForPNG(bool reader)
{
    IOPluginSetForFormat plugins;
    std::string defaultID;

    if (reader) {
        appPTR->getReadersForFormat("png", &plugins);
        defaultID = appPTR->getReaderPluginIDForFileType("png");
    } else {
        appPTR->getWritersForFormat("png", &plugins);
        defaultID = appPTR->getWriterPluginIDForFileType("png");
    }
    // by decreasing score, as in the menu
    for (IOPluginSetForFormat::reverse_iterator it = plugins.rbegin(); it != plugins.rend(); ++it) {
        const bool bundled = reader ? ReadNode::isBundledReader(it->pluginID, false) : WriteNode::isBundledWriter(it->pluginID, false);
        if ( bundled && (it->pluginID != defaultID) ) {
            return it->pluginID;
        }
    }

    return std::string();
}
} // anonymous namespace

// A decoder or encoder chosen in the menu that isn't the default one of the file extension is the one
// that the loaded project creates, and the menu still shows it, in the three project file formats (#1783): a
// Python project gave the Read node the default decoder of the extension once the script had run.
TEST(ReadWriteDecoderChoice, NonDefaultChoiceKeptOnReload)
{
    if ( !NatronTest::requirePlugins({ PLUGINID_OFX_READPNG, PLUGINID_OFX_READOIIO, PLUGINID_OFX_WRITEPNG, PLUGINID_OFX_WRITEOIIO }, "openfx-io") ) {
        return;
    }
    const std::string decoderID = nonDefaultPluginForPNG(true);
    const std::string encoderID = nonDefaultPluginForPNG(false);
    ASSERT_FALSE( decoderID.empty() );
    ASSERT_FALSE( encoderID.empty() );
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const NatronTest::ProjectResetScope resetProject; // destroyed before tmpDir
    ProjectPtr project = appPTR->getTopLevelInstance()->getProject();
    const QString dir = tmpDir.path() + QLatin1Char('/');
    ASSERT_TRUE( writePNG( dir + QString::fromUtf8("input.png") ) );

    const struct
    {
        ProjectFileFormatEnum format;
        const char* name;
    } formats[] = {
        { eProjectFileFormatText, "DecoderChoice.ntp" },
        { eProjectFileFormatBinary, "DecoderChoiceBinary.ntp" },
        { eProjectFileFormatPython, "DecoderChoice.py" },
    };
    for (const auto& f : formats) {
        SCOPED_TRACE(f.name);
        project->resetProject();
        NodePtr read = createIONode( PLUGINID_NATRON_READ, dir.toStdString() + "input.png" );
        NodePtr write = createIONode( PLUGINID_NATRON_WRITE, dir.toStdString() + "output.png" );
        ASSERT_TRUE(read && write);
        KnobChoicePtr decoderMenu = std::dynamic_pointer_cast<KnobChoice>( read->getKnobByName(kNatronReadNodeParamDecodingPluginChoice) );
        KnobChoicePtr encoderMenu = std::dynamic_pointer_cast<KnobChoice>( write->getKnobByName(kNatronWriteNodeParamEncodingPluginChoice) );
        ASSERT_TRUE(decoderMenu && encoderMenu);
        // as the user does in the menu
        decoderMenu->setValueFromID(decoderID, 0);
        encoderMenu->setValueFromID(encoderID, 0);
        decoderMenu.reset();
        encoderMenu.reset();
        ASSERT_EQ( decoderID, embeddedPluginID(read) );
        ASSERT_EQ( encoderID, embeddedPluginID(write) );
        // a parameter of the decoder, which a replaced decoder would reset
        KnobChoicePtr before = std::dynamic_pointer_cast<KnobChoice>( read->getKnobByName("before") );
        ASSERT_TRUE(before);
        ASSERT_GE(before->getNumEntries(), 2);
        const int beforeValue = (before->getDefaultValue(0) + 1) % before->getNumEntries();
        before->setValue(beforeValue);
        before.reset();
        const std::string readName = read->getScriptName();
        const std::string writeName = write->getScriptName();
        read.reset();
        write.reset();

        QString savedPath;
        ASSERT_TRUE( project->saveProject( dir, QString::fromUtf8(f.name), &savedPath, f.format ) );
        ASSERT_TRUE( project->loadProject( dir, QString::fromUtf8(f.name) ) );
        read = project->getNodeByName(readName);
        write = project->getNodeByName(writeName);
        EXPECT_EQ( decoderID, embeddedPluginID(read) );
        EXPECT_EQ( decoderID, pluginIDKnobValue(read, kNatronReadNodeParamDecodingPluginID) );
        EXPECT_EQ( decoderID, menuEntryID(read, kNatronReadNodeParamDecodingPluginChoice) );
        before = read ? std::dynamic_pointer_cast<KnobChoice>( read->getKnobByName("before") ) : KnobChoicePtr();
        ASSERT_TRUE(before);
        EXPECT_EQ( beforeValue, before->getValue() );
        before.reset();
        EXPECT_EQ( encoderID, embeddedPluginID(write) );
        EXPECT_EQ( encoderID, pluginIDKnobValue(write, kNatronWriteNodeParamEncodingPluginID) );
        EXPECT_EQ( encoderID, menuEntryID(write, kNatronWriteNodeParamEncodingPluginChoice) );
    }
}

// The parameters of a Read node that are kept when its file changes (onMissingFrame, before, after,
// customFps, frameRate: see genericReaderKnobNames in Engine/ReadNode.cpp), set before its first file name,
// are kept when that file name creates its decoder, as for the Write node (#1574): they were reset to the
// defaults of the decoder (#1784).
TEST(ReadWriteDecoderChoice, ReadKnobsSetBeforeTheFirstFileAreKept)
{
    // ReadOIIO is the placeholder decoder of a Read node without a file (READ_NODE_DEFAULT_READER)
    if ( !NatronTest::requirePlugins({ PLUGINID_OFX_READPNG, PLUGINID_OFX_READOIIO }, "openfx-io") ) {
        return;
    }
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    ASSERT_TRUE( writePNG( dir + QString::fromUtf8("input.png") ) );
    NatronTest::NodeScope nodes;

    NodePtr read = nodes.add( createIONode( PLUGINID_NATRON_READ, std::string() ) );
    ASSERT_TRUE(read);
    // another value than the default one
    int choiceValues[3];
    const char* const choiceNames[3] = { "onMissingFrame", "before", "after" };
    for (int i = 0; i < 3; ++i) {
        SCOPED_TRACE(choiceNames[i]);
        KnobChoicePtr knob = std::dynamic_pointer_cast<KnobChoice>( read->getKnobByName(choiceNames[i]) );
        ASSERT_TRUE(knob);
        ASSERT_GE(knob->getNumEntries(), 2);
        choiceValues[i] = (knob->getDefaultValue(0) + 1) % knob->getNumEntries();
        knob->setValue(choiceValues[i]);
    }
    KnobBoolPtr customFps = std::dynamic_pointer_cast<KnobBool>( read->getKnobByName("customFps") );
    KnobDoublePtr frameRate = std::dynamic_pointer_cast<KnobDouble>( read->getKnobByName("frameRate") );
    ASSERT_TRUE(customFps && frameRate);
    const bool customFpsValue = !customFps->getDefaultValue(0);
    const double frameRateValue = frameRate->getDefaultValue(0) + 1.5;
    customFps->setValue(customFpsValue);
    frameRate->setValue(frameRateValue);
    customFps.reset();
    frameRate.reset();

    KnobFilePtr file = std::dynamic_pointer_cast<KnobFile>( read->getKnobByName(kOfxImageEffectFileParamName) );
    ASSERT_TRUE(file);
    file->setValue( dir.toStdString() + "input.png" );
    file.reset();
    ASSERT_EQ( appPTR->getReaderPluginIDForFileType("png"), embeddedPluginID(read) );

    for (int i = 0; i < 3; ++i) {
        KnobChoicePtr knob = std::dynamic_pointer_cast<KnobChoice>( read->getKnobByName(choiceNames[i]) );
        ASSERT_TRUE(knob) << choiceNames[i];
        EXPECT_EQ( choiceValues[i], knob->getValue() ) << choiceNames[i];
    }
    customFps = std::dynamic_pointer_cast<KnobBool>( read->getKnobByName("customFps") );
    frameRate = std::dynamic_pointer_cast<KnobDouble>( read->getKnobByName("frameRate") );
    ASSERT_TRUE(customFps && frameRate);
    EXPECT_EQ( customFpsValue, customFps->getValue() );
    EXPECT_EQ( frameRateValue, frameRate->getValue() );
    // the decoder got the change of customFps: it enables Frame Rate
    EXPECT_TRUE( frameRate->isEnabled(0) );
}

// The parameters that the decoder guesses from its file are still guessed, also for a first file that doesn't
// exist yet (e.g. a sequence not rendered yet): ParamExistingInstance, which the placeholder decoder of a Read
// node without a file has, is not kept for the first decoder (#1784).
TEST(ReadWriteDecoderChoice, ReadGuessesFromItsFirstFile)
{
    // ReadOIIO: the placeholder decoder, see ReadKnobsSetBeforeTheFirstFileAreKept
    if ( !NatronTest::requirePlugins({ PLUGINID_OFX_READPNG, PLUGINID_OFX_READOIIO }, "openfx-io") ) {
        return;
    }
    QTemporaryDir tmpDir;
    ASSERT_TRUE( tmpDir.isValid() );
    const QString dir = tmpDir.path() + QLatin1Char('/');
    ASSERT_TRUE( writePNG( dir + QString::fromUtf8("input.png") ) );
    NatronTest::NodeScope nodes;

    // the parameters guessed from the file by a Read node created with it
    NodePtr reference = nodes.add( createIONode( PLUGINID_NATRON_READ, dir.toStdString() + "input.png" ) );
    ASSERT_TRUE(reference);
    const char* const guessedNames[2] = { "outputComponents", "filePremult" };

    // the first file name of a Read node created without a file exists...
    NodePtr read = nodes.add( createIONode( PLUGINID_NATRON_READ, std::string() ) );
    // ...or not yet
    NodePtr laterRead = nodes.add( createIONode( PLUGINID_NATRON_READ, std::string() ) );
    ASSERT_TRUE(read && laterRead);
    KnobFilePtr file = std::dynamic_pointer_cast<KnobFile>( read->getKnobByName(kOfxImageEffectFileParamName) );
    KnobFilePtr laterFile = std::dynamic_pointer_cast<KnobFile>( laterRead->getKnobByName(kOfxImageEffectFileParamName) );
    ASSERT_TRUE(file && laterFile);
    file->setValue( dir.toStdString() + "input.png" );
    laterFile->setValue( dir.toStdString() + "notyet.png" );
    file.reset();
    laterFile.reset();

    for (const char* name : guessedNames) {
        KnobChoicePtr expected = std::dynamic_pointer_cast<KnobChoice>( reference->getKnobByName(name) );
        KnobChoicePtr knob = std::dynamic_pointer_cast<KnobChoice>( read->getKnobByName(name) );
        ASSERT_TRUE(expected && knob) << name;
        EXPECT_EQ( expected->getActiveEntry().id, knob->getActiveEntry().id ) << name;
    }
    KnobBoolPtr existingInstance = std::dynamic_pointer_cast<KnobBool>( laterRead->getKnobByName("ParamExistingInstance") );
    ASSERT_TRUE(existingInstance);
    EXPECT_FALSE( existingInstance->getValue() );
}
