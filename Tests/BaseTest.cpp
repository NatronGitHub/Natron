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

#include <cstdlib>
#include <fstream>
#include <string>

#include "BaseTest.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QThreadPool>

// ofxhPropertySuite.h:565:37: warning: 'this' pointer cannot be null in well-defined C++ code; comparison may be assumed to always evaluate to true [-Wtautological-undefined-compare]
// clang-format off
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare) // appeared in clang 3.5
#include <ofxhPluginCache.h>
//#include <ofxhPluginAPICache.h>
//#include <ofxhImageEffect.h>
#include <ofxhImageEffectAPI.h>
//#include <ofxOpenGLRender.h>
//#include <ofxhHost.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)
// clang-format on

#include "Engine/CreateNodeArgs.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/OutputEffectInstance.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/EffectInstance.h"
#include "Engine/Plugin.h"
#include "Engine/Curve.h"
#include "Engine/CLArgs.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_USING

BaseTest::BaseTest()
    : testing::Test()
    , _app()
{
}

BaseTest::~BaseTest()
{
}

void
BaseTest::registerTestPlugins()
{
    _allTestPluginIDs.clear();

    _generatorPluginID = QString::fromUtf8(PLUGINID_OFX_SENOISE);
    _allTestPluginIDs.push_back(_generatorPluginID);

    _readOIIOPluginID = QString::fromUtf8(PLUGINID_OFX_READOIIO);
    _allTestPluginIDs.push_back(_readOIIOPluginID);

    _writeOIIOPluginID = QString::fromUtf8(PLUGINID_OFX_WRITEOIIO);
    _allTestPluginIDs.push_back(_writeOIIOPluginID);

    for (unsigned int i = 0; i < _allTestPluginIDs.size(); ++i) {
        ///make sure the generic test plugin is present
        LibraryBinary* bin = NULL;
        try {
            Plugin* p = appPTR->getPluginBinary(_allTestPluginIDs[i], -1, -1, false);
            if (p) {
                bin = p->getLibraryBinary();
            }
        } catch (const std::exception & e) {
            std::cout << e.what() << std::endl;
        }

        ASSERT_TRUE(bin != NULL);
    }
}

void
BaseTest::SetUp()
{
    const char* path = std::getenv("OFX_PLUGIN_PATH");
    if (path) {
        std::cout << "Warning: Ignoring standard plugin path, OFX_PLUGIN_PATH=" << path << std::endl;
        OFX::Host::PluginCache::useStdOFXPluginsLocation(false);
    }

    _app = appPTR->getTopLevelInstance();
    registerTestPlugins();
}

void
BaseTest::TearDown()
{
    appPTR->setNumberOfThreads(0);
    ///Caches may have launched some threads to delete images, wait for them to be done
    QThreadPool::globalInstance()->waitForDone();
}

NodePtr
BaseTest::createNode(const QString & pluginID,
                     int majorVersion,
                     int minorVersion)
{
    CreateNodeArgs args( pluginID.toStdString(), getApp()->getProject() );
    
    args.setProperty<int>(kCreateNodeArgsPropPluginVersion, majorVersion, 0);
    args.setProperty<int>(kCreateNodeArgsPropPluginVersion, minorVersion, 1);

    NodePtr ret =  getApp()->createNode(args);


    EXPECT_NE(ret.get(), (Node*)NULL);

    return ret;
}

void
BaseTest::connectNodes(NodePtr input,
                       NodePtr output,
                       int inputNumber,
                       bool expectedReturnValue)
{
    if (expectedReturnValue) {
        ///check that the connections are internally all set as "expected"

        EXPECT_EQ( (Node*)NULL, output->getInput(inputNumber).get() );
        EXPECT_FALSE( output->isInputConnected(inputNumber) );
    } else {
        ///the call can only fail for those 2 reasons
        EXPECT_TRUE(inputNumber > output->getNInputs() || //< inputNumber is greater than the maximum input number
                    output->getInput(inputNumber).get() != (Node*)NULL); //< input slot is already filled with another node
    }


    bool ret = getApp()->getProject()->connectNodes(inputNumber, input, output);
    EXPECT_EQ(expectedReturnValue, ret);

    if (expectedReturnValue) {
        EXPECT_TRUE( input->hasOutputConnected() );
        EXPECT_EQ(output->getInput(inputNumber), input);
        EXPECT_TRUE( output->isInputConnected(inputNumber) );
    }
}

void
BaseTest::disconnectNodes(NodePtr input,
                          NodePtr output,
                          bool expectedReturnvalue)
{
    if (expectedReturnvalue) {
        ///check that the connections are internally all set as "expected"

        ///the input must have in its output the node 'output'
        EXPECT_TRUE( input->hasOutputConnected() );
        const NodesWList & outputs = input->getGuiOutputs();
        bool foundOutput = false;
        for (NodesWList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
            if (it->lock() == output) {
                foundOutput = true;
                break;
            }
        }

        ///the output must have in its inputs the node 'input'
        const std::vector<NodeWPtr> & inputs = output->getGuiInputs();
        int inputIndex = 0;
        bool foundInput = false;
        for (U32 i = 0; i < inputs.size(); ++i) {
            if (inputs[i].lock() == input) {
                foundInput = true;
                break;
            }
            ++inputIndex;
        }

        EXPECT_TRUE(foundInput);
        EXPECT_TRUE(foundOutput);
        EXPECT_EQ(output->getInput(inputIndex), input);
        EXPECT_TRUE( output->isInputConnected(inputIndex) );
    }

    ///call disconnect
    bool ret = getApp()->getProject()->disconnectNodes(input, output);
    EXPECT_EQ(expectedReturnvalue, ret);

    if (expectedReturnvalue) {
        ///check that the disconnection went OK

        const NodesWList & outputs = input->getGuiOutputs();
        bool foundOutput = false;
        for (NodesWList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
            if (it->lock() == output) {
                foundOutput = true;
                break;
            }
        }

        ///the output must have in its inputs the node 'input'
        const std::vector<NodeWPtr> & inputs = output->getGuiInputs();
        int inputIndex = 0;
        bool foundInput = false;
        for (U32 i = 0; i < inputs.size(); ++i) {
            if (inputs[i].lock() == input) {
                foundInput = true;
                break;
            }
            ++inputIndex;
        }

        EXPECT_FALSE(foundOutput);
        EXPECT_FALSE(foundInput);
        EXPECT_EQ( (Node*)NULL, output->getInput(inputIndex).get() );
        EXPECT_FALSE( output->isInputConnected(inputIndex) );
    }
} // disconnectNodes

///High level test: render 1 frame of dot generator
TEST_F(BaseTest, GenerateDot)
{
    ///create the generator
    NodePtr generator = createNode(_generatorPluginID);

    ///create the writer and set its output filename
    NodePtr writer = createNode(_writeOIIOPluginID);

    ASSERT_TRUE( bool(generator) && bool(writer) );

    KnobIPtr frameRange = generator->getApp()->getProject()->getKnobByName("frameRange");
    ASSERT_TRUE( bool(frameRange) );
    KnobInt* knob = dynamic_cast<KnobInt*>( frameRange.get() );
    ASSERT_TRUE(knob);
    knob->setValue(1, ViewSpec::all(), 0);
    knob->setValue(1, ViewSpec::all(), 1);

    Format f(0, 0, 200, 200, "toto", 1.);
    generator->getApp()->getProject()->setOrAddProjectFormat(f);

    const QString& binPath = appPTR->getApplicationBinaryPath();
    QString filePath = binPath + QString::fromUtf8("/test_dot_generator.jpg");
    writer->setOutputFilesForWriter( filePath.toStdString() );

    ///attempt to connect the 2 nodes together
    connectNodes(generator, writer, 0, true);

    ///and start rendering. This call is blocking.
    std::list<AppInstance::RenderWork> works;
    AppInstance::RenderWork w;
    w.writer = dynamic_cast<OutputEffectInstance*>( writer->getEffectInstance().get() );
    assert(w.writer);
    w.firstFrame = INT_MIN;
    w.lastFrame = INT_MAX;
    w.frameStep = INT_MIN;
    w.useRenderStats = false;
    works.push_back(w);
    getApp()->startWritersRendering(false, works);

    EXPECT_TRUE( QFile::exists(filePath) );
    QFile::remove(filePath);
}

TEST_F(BaseTest, SetValues)
{
    NodePtr generator = createNode(_generatorPluginID);

    assert(generator);
    KnobIPtr knob = generator->getKnobByName("noiseZSlope");
    KnobDouble* radius = dynamic_cast<KnobDouble*>( knob.get() );
    EXPECT_TRUE(radius != 0);
    if (!radius) {
        return;
    }
    radius->setValue(0.5);
    EXPECT_TRUE(radius->getValue() == 0.5);

    //Check that linear interpolation is working as intended
    KeyFrame kf;
    radius->setInterpolationAtTime(eCurveChangeReasonInternal, ViewSpec::all(),  0, 0, eKeyframeTypeLinear, &kf);
    radius->setValueAtTime(0, 0., ViewSpec::all(), 0);
    radius->setValueAtTime(100, 1., ViewSpec::all(), 0);
    for (int i = 0; i <= 100; ++i) {
        double v = radius->getValueAtTime(i);
        EXPECT_TRUE(std::abs(v - i / 100.) < 1e-6);
    }
}

// #1124: the node hash keys the disk and viewer caches, which persist across sessions. Reloading a project
// restores the saved knobs age: changing a parameter after that must not give the hash that a different change
// gave in an earlier session (closed without saving) from the same saved state.
TEST_F(BaseTest, HashAfterReload)
{
    NodePtr generator = createNode(_generatorPluginID);

    ASSERT_TRUE(generator.get() != NULL);
    KnobIPtr knob = generator->getKnobByName("noiseZSlope");
    KnobDouble* slope = dynamic_cast<KnobDouble*>( knob.get() );
    ASSERT_TRUE(slope != 0);

    const U64 savedAge = generator->getKnobsAge();
    const U64 savedHash = generator->getHashValue();

    // first session: change the parameter, then "close without saving"
    slope->setValue(0.25);
    const U64 firstSessionHash = generator->getHashValue();
    EXPECT_NE(firstSessionHash, savedHash);

    // reopen: the saved age (and thus the saved hash) is restored
    slope->setValue(0.);
    generator->setKnobsAge(savedAge);
    EXPECT_EQ(generator->getHashValue(), savedHash);

    // second session: a different change must give a different hash
    slope->setValue(0.75);
    EXPECT_NE(generator->getHashValue(), savedHash);
    EXPECT_NE(generator->getHashValue(), firstSessionHash);

    // later changes in the same session still change the hash
    const U64 secondSessionHash = generator->getHashValue();
    slope->setValue(0.5);
    EXPECT_NE(generator->getHashValue(), secondSessionHash);
}

// Write a binary PPM image of the given size (any image format that ReadOIIO decodes would do).
static void
writeTestPPM(const QString& filePath,
             int width,
             int height)
{
    std::ofstream ofs(filePath.toStdString().c_str(), std::ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    std::string pixels(width * height * 3, '\x80');
    ofs.write( pixels.data(), pixels.size() );
}

// same arguments as App.createReader() / App.createNode() in Python
static void
setReadNodeArgs(CreateNodeArgs& args)
{
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args.setProperty<bool>(kCreateNodeArgsPropSilent, true);
    args.setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);
}

static NodePtr
createReadNode(const AppInstancePtr& app,
               const QString& filePath)
{
    CreateNodeArgs args( PLUGINID_NATRON_READ, app->getProject() );
    setReadNodeArgs(args);

    return app->createReader(filePath.toStdString(), args);
}

// Start from an empty project with the given format, with the project format still to be auto-set.
static void
resetProjectFormat(const ProjectPtr& project,
                   const Format& f)
{
    project->setAutoSetProjectFormatEnabled(true);
    project->setOrAddProjectFormat(f); // sets the format and disables the auto-set
    project->setAutoSetProjectFormatEnabled(true);
}

static void
expectProjectFormat(const ProjectPtr& project,
                    int width,
                    int height,
                    double par)
{
    Format f;
    project->getProjectDefaultFormat(&f);
    EXPECT_EQ( width, f.width() );
    EXPECT_EQ( height, f.height() );
    EXPECT_DOUBLE_EQ( par, f.getPixelAspectRatio() );
}

// #1294: in an otherwise empty project, the project format is auto-set from the first Reader
// as soon as the Reader has a decodable file, i.e. before any render starts, and not when the Reader first renders
// (which may be in the middle of a sequence render, so that the frames of a Writer had different formats).
TEST_F(BaseTest, ProjectFormatAutoSetFromReaderBeforeRender)
{
    ProjectPtr project = getApp()->getProject();
    resetProjectFormat( project, Format(0, 0, 1920, 1080, "HD", 1.) );

    const QString& binPath = appPTR->getApplicationBinaryPath();
    QString emptyPath = binPath + QString::fromUtf8("/test_project_format_empty.ppm");
    QString filePath = binPath + QString::fromUtf8("/test_project_format_64x48.ppm");
    QString file2Path = binPath + QString::fromUtf8("/test_project_format_32x16.ppm");
    writeTestPPM(filePath, 64, 48);
    writeTestPPM(file2Path, 32, 16);
    QFile::remove(emptyPath);

    // A Reader whose file does not exist must not disable the auto-set.
    NodePtr emptyReader = createReadNode(getApp(), emptyPath);
    expectProjectFormat(project, 1920, 1080, 1.);
    EXPECT_TRUE( project->isAutoSetProjectFormatEnabled() );

    // The first Reader with a file sets the project format, without rendering.
    NodePtr reader = createReadNode(getApp(), filePath);
    ASSERT_TRUE( bool(reader) );
    EXPECT_EQ( 64, reader->getEffectInstance()->getOutputFormat().width() );
    expectProjectFormat(project, 64, 48, 1.);
    EXPECT_FALSE( project->isAutoSetProjectFormatEnabled() );

    // The next Readers don't change it.
    NodePtr reader2 = createReadNode(getApp(), file2Path);
    ASSERT_TRUE( bool(reader2) );
    expectProjectFormat(project, 64, 48, 1.);

    if (emptyReader) {
        emptyReader->destroyNode(false, false);
    }
    reader->destroyNode(false, false);
    reader2->destroyNode(false, false);
    QFile::remove(filePath);
    QFile::remove(file2Path);
}

// #1294: with a project PAR != 1, a Read node without a file reports the project rectangle with PAR 1:
// it must neither change the project format nor disable the auto-set.
TEST_F(BaseTest, ProjectFormatNotAutoSetFromReaderWithoutFile)
{
    ProjectPtr project = getApp()->getProject();
    resetProjectFormat( project, Format(0, 0, 720, 576, "PAL", 1.09) );

    const QString& binPath = appPTR->getApplicationBinaryPath();
    QString filePath = binPath + QString::fromUtf8("/test_project_format_64x48.ppm");
    writeTestPPM(filePath, 64, 48);

    CreateNodeArgs args( PLUGINID_NATRON_READ, getApp()->getProject() );
    setReadNodeArgs(args);
    NodePtr reader = getApp()->createNode(args);
    ASSERT_TRUE( bool(reader) );
    expectProjectFormat(project, 720, 576, 1.09);
    EXPECT_TRUE( project->isAutoSetProjectFormatEnabled() );

    // setting the file afterwards auto-sets the project format
    KnobIPtr fileKnob = reader->getKnobByName(kOfxImageEffectFileParamName);
    KnobFile* file = dynamic_cast<KnobFile*>( fileKnob.get() );
    ASSERT_TRUE(file);
    file->setValue( filePath.toStdString() );
    expectProjectFormat(project, 64, 48, 1.);
    EXPECT_FALSE( project->isAutoSetProjectFormatEnabled() );

    reader->destroyNode(false, false);
    QFile::remove(filePath);
}

// #1294: if the first Reader's file has the project format, the auto-set is disabled, so that the next Readers don't change it.
TEST_F(BaseTest, ProjectFormatAutoSetFromReaderWithSameFormat)
{
    ProjectPtr project = getApp()->getProject();
    resetProjectFormat( project, Format(0, 0, 200, 100, "", 1.) );

    const QString& binPath = appPTR->getApplicationBinaryPath();
    QString samePath = binPath + QString::fromUtf8("/test_project_format_200x100.ppm");
    QString filePath = binPath + QString::fromUtf8("/test_project_format_64x48.ppm");
    writeTestPPM(samePath, 200, 100);
    writeTestPPM(filePath, 64, 48);

    NodePtr reader = createReadNode(getApp(), samePath);
    ASSERT_TRUE( bool(reader) );
    expectProjectFormat(project, 200, 100, 1.);
    EXPECT_FALSE( project->isAutoSetProjectFormatEnabled() );

    NodePtr reader2 = createReadNode(getApp(), filePath);
    ASSERT_TRUE( bool(reader2) );
    expectProjectFormat(project, 200, 100, 1.);

    reader->destroyNode(false, false);
    reader2->destroyNode(false, false);
    QFile::remove(samePath);
    QFile::remove(filePath);
}

// #1294: a Reader created while a sequence render is running must not change the project format
// during that render; the format is auto-set when the render finishes.
TEST_F(BaseTest, ProjectFormatNotAutoSetDuringSequenceRender)
{
    ProjectPtr project = getApp()->getProject();
    resetProjectFormat( project, Format(0, 0, 200, 200, "", 1.) );

    NodePtr generator = createNode(_generatorPluginID);
    NodePtr writer = createNode(_writeOIIOPluginID);
    ASSERT_TRUE( bool(generator) && bool(writer) );
    const QString& binPath = appPTR->getApplicationBinaryPath();
    QString outPath = binPath + QString::fromUtf8("/test_project_format_render_####.jpg");
    writer->setOutputFilesForWriter( outPath.toStdString() );
    connectNodes(generator, writer, 0, true);

    QString filePath = binPath + QString::fromUtf8("/test_project_format_64x48.ppm");
    QString file2Path = binPath + QString::fromUtf8("/test_project_format_32x16.ppm");
    writeTestPPM(filePath, 64, 48);
    writeTestPPM(file2Path, 32, 16);

    OutputEffectInstance* output = dynamic_cast<OutputEffectInstance*>( writer->getEffectInstance().get() );
    ASSERT_TRUE(output);
    // non-blocking render (AppInstance::startWritersRendering always blocks in background mode)
    output->renderFullSequence(false, false, NULL, 1, 200, 1);
    QElapsedTimer timer;
    timer.start();
    while ( !output->isDoingSequentialRender() && timer.elapsed() < 5000 ) {
        QCoreApplication::processEvents();
    }

    NodePtr reader;
    if ( output->isDoingSequentialRender() ) {
        // The first pending Reader is deleted before the render ends: the next one must be used.
        NodePtr deletedReader = createReadNode(getApp(), file2Path);
        ASSERT_TRUE( bool(deletedReader) );
        deletedReader->destroyNode(true, false);
        deletedReader.reset();
        reader = createReadNode(getApp(), filePath);
        ASSERT_TRUE( bool(reader) );
        if ( output->isDoingSequentialRender() ) {
            expectProjectFormat(project, 200, 200, 1.);
            EXPECT_TRUE( project->isAutoSetProjectFormatEnabled() );
        }
        // renderFinished is emitted by the scheduler thread while the render is still active.
        // Block that thread until the main thread has processed the queued signals (the Project's
        // slot was connected first, so it runs first), to make sure that the Project handles
        // renderFinished while the engine is not idle yet.
        RenderEnginePtr engine = output->getRenderEngine();
        ASSERT_TRUE( bool(engine) );
        QObject::connect( engine.get(), &RenderEngine::renderFinished, project.get(), [](int) {}, Qt::BlockingQueuedConnection );
    } else {
        std::cout << "Warning: the render finished too early, not testing the auto-set during the render" << std::endl;
    }
    // Wait for the render to finish and process the queued renderFinished signal, without sleeping,
    // so that the signal is handled while the render engine may still be active.
    timer.restart();
    while ( output->isDoingSequentialRender() && timer.elapsed() < 120000 ) {
        QCoreApplication::processEvents();
    }
    EXPECT_FALSE( output->isDoingSequentialRender() );
    if (reader) {
        timer.restart();
        while ( project->isAutoSetProjectFormatEnabled() && timer.elapsed() < 2000 ) {
            QCoreApplication::processEvents();
        }
        expectProjectFormat(project, 64, 48, 1.);
        EXPECT_FALSE( project->isAutoSetProjectFormatEnabled() );
        reader->destroyNode(false, false);
    }
    writer->destroyNode(false, false);
    generator->destroyNode(false, false);
    QFile::remove(filePath);
    QFile::remove(file2Path);
    for (int i = 1; i <= 200; ++i) {
        QFile::remove( binPath + QString::fromUtf8("/test_project_format_render_%1.jpg").arg(i, 4, 10, QLatin1Char('0')) );
    }
}

///High level test: simple node connections test
TEST_F(BaseTest, SimpleNodeConnections) {
    ///create the generator
    NodePtr generator = createNode(_generatorPluginID);

    ///create the writer and set its output filename
    NodePtr writer = createNode(_writeOIIOPluginID);

    ASSERT_TRUE(writer && generator);
    connectNodes(generator, writer, 0, true);
    connectNodes(generator, writer, 0, false); //< expect it to fail
    disconnectNodes(generator, writer, true);
    disconnectNodes(generator, writer, false);
    connectNodes(generator, writer, 0, true);
}
