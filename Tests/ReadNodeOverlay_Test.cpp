/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

// The Read node forwards the overlay actions to its embedded reader plug-in, so that the overlay
// interact of a reader (e.g. the TransformInteract of openfx-arena's ReadSVG) is shown and usable.
//
// The overlay is driven headless, with a mock viewport, through the same EffectInstance::*_public()
// functions the viewer calls on the Read node (see ViewerTab::notifyOverlaysPenDown_internal()).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <fstream>
#include <iostream>
#include <string>

#include <gtest/gtest.h>

#include <QtCore/QTemporaryDir>
#include <QtCore/QPointF>
#include <QtCore/QString>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/OverlaySupport.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/ReadNode.h"
#include "Engine/RenderScale.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_USING

namespace {
// A viewport that does not draw anything: only what an OFX overlay interact queries during pen events.
class MockViewport
    : public OverlaySupport
{
public:
    virtual void swapOpenGLBuffers() OVERRIDE FINAL {}

    virtual void redraw() OVERRIDE FINAL {}

    virtual void getViewportSize(double &width,
                                 double &height) const OVERRIDE FINAL
    {
        width = 1000.;
        height = 1000.;
    }

    virtual void getPixelScale(double & xScale,
                               double & yScale) const OVERRIDE FINAL
    {
        xScale = 1.;
        yScale = 1.;
    }

#ifdef OFX_EXTENSIONS_NATRON
    virtual double getScreenPixelRatio() const OVERRIDE FINAL
    {
        return 1.;
    }

#endif

    virtual void getBackgroundColour(double &r,
                                     double &g,
                                     double &b) const OVERRIDE FINAL
    {
        r = g = b = 0.;
    }

    virtual void saveOpenGLContext() OVERRIDE FINAL {}

    virtual void restoreOpenGLContext() OVERRIDE FINAL {}

    virtual RectD getViewportRect() const OVERRIDE FINAL
    {
        return RectD(-500., -500., 500., 500.);
    }

    virtual void getCursorPosition(double& x,
                                   double& y) const OVERRIDE FINAL
    {
        x = y = 0.;
    }

    virtual void toWidgetCoordinates(double * /*x*/,
                                     double * /*y*/) const OVERRIDE FINAL {}

    virtual void toCanonicalCoordinates(double * /*x*/,
                                        double * /*y*/) const OVERRIDE FINAL {}

    virtual int getWidgetFontHeight() const OVERRIDE FINAL
    {
        return 10;
    }

    virtual int getStringWidthForCurrentFont(const std::string& /*string*/) const OVERRIDE FINAL
    {
        return 10;
    }
};

bool
isPluginAvailable(const char* pluginID)
{
    try {
        Plugin* p = appPTR->getPluginBinary(QString::fromUtf8(pluginID), -1, -1, false);

        return p && p->getLibraryBinary();
    } catch (...) {
        return false;
    }
}

// The bundled google-test predates GTEST_SKIP(): the tests return early and are reported as passed,
// so say clearly that they did not run, as RotoPaint_Test does.
// TODO: Convert to GTEST_SKIP() when gtest updated.
void
reportSkipped(const std::string& why)
{
    const testing::TestInfo* info = testing::UnitTest::GetInstance()->current_test_info();

    std::cerr << "[  SKIPPED ] " << info->test_case_name() << '.' << info->name() << ": " << why << std::endl;
}

const double kTime = 1.;
} // anonymous namespace

class ReadNodeOverlayTest
    : public testing::Test
{
protected:

    virtual void SetUp() OVERRIDE FINAL
    {
        _app = appPTR->getTopLevelInstance();
        ASSERT_TRUE( _tmpDir.isValid() );
        _dir = _tmpDir.path();
        _svgFile = _dir.toStdString() + "/square.svg";
        std::ofstream svg( _svgFile.c_str() );
        svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" height=\"100\">"
            "<rect x=\"10\" y=\"10\" width=\"80\" height=\"80\" fill=\"red\"/></svg>\n";
        // Only the file extension matters to pick the reader: ReadOIIO reads .exr files.
        _exrFile = _dir.toStdString() + "/missing.exr";
    }

    virtual void TearDown() OVERRIDE FINAL
    {
        if (_effect) {
            _effect->setCurrentViewportForOverlays_public(NULL);
        }
        if (_node) {
            _node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
        }
        _effect.reset();
        _node.reset();
        if (_projectLoaded) {
            // Leave the shared app with an untitled empty project, as the other tests find it,
            // before the temporary directory of the saved project is removed.
            _app->getProject()->resetProject();
            EXPECT_TRUE( _app->getProject()->getProjectPath().isEmpty() );
            _projectLoaded = false;
        }
    }

    void createReadNode(const std::string& filename)
    {
        CreateNodeArgs args( PLUGINID_NATRON_READ, _app->getProject() );

        args.addParamDefaultValue<std::string>(kOfxImageEffectFileParamName, filename);
        _node = _app->createNode(args);
        ASSERT_TRUE( bool(_node) );
        _effect = _node->getEffectInstance();
        ASSERT_TRUE( bool(_effect) );
        ASSERT_TRUE( dynamic_cast<ReadNode*>( _effect.get() ) != NULL );
        _effect->setCurrentViewportForOverlays_public(&_viewport);
    }

    NodePtr getEmbeddedReader() const
    {
        ReadNode* read = dynamic_cast<ReadNode*>( _effect.get() );

        return read ? read->getEmbeddedReader() : NodePtr();
    }

    void setFile(const std::string& filename)
    {
        KnobFilePtr file = std::dynamic_pointer_cast<KnobFile>( _node->getKnobByName(kOfxImageEffectFileParamName) );

        ASSERT_TRUE( bool(file) );
        file->setValue(filename);
    }

    bool penDown(double x,
                 double y)
    {
        return _effect->onOverlayPenDown_public(kTime, RenderScale(), ViewIdx(0), QPointF(x, y), QPointF(x, y), 1., 0., ePenTypeLMB, KeyboardModifiers());
    }

    bool penMotion(double x,
                   double y)
    {
        return _effect->onOverlayPenMotion_public(kTime, RenderScale(), ViewIdx(0), QPointF(x, y), QPointF(x, y), 1., 0., KeyboardModifiers());
    }

    bool penUp(double x,
               double y)
    {
        return _effect->onOverlayPenUp_public(kTime, RenderScale(), ViewIdx(0), QPointF(x, y), QPointF(x, y), 1., 0., KeyboardModifiers());
    }

    AppInstancePtr _app;
    QTemporaryDir _tmpDir; // removed with the test fixture
    bool _projectLoaded = false;
    QString _dir;
    std::string _svgFile, _exrFile;
    MockViewport _viewport;
    NodePtr _node;
    EffectInstancePtr _effect;
};

// A reader without overlay (ReadOIIO): the Read node has no overlay, and ignores overlay events.
TEST_F(ReadNodeOverlayTest, ReaderWithoutOverlay)
{
    if ( !isPluginAvailable(PLUGINID_OFX_READOIIO) ) {
        reportSkipped("plug-in not found (needs " PLUGINID_OFX_READOIIO " from openfx-io in OFX_PLUGIN_PATH)");

        return;
    }
    createReadNode(_exrFile);
    NodePtr reader = getEmbeddedReader();
    ASSERT_TRUE( bool(reader) );
    EXPECT_EQ(reader->getPluginID(), std::string(PLUGINID_OFX_READOIIO));
    EXPECT_FALSE( reader->getEffectInstance()->hasOverlay() );
    EXPECT_FALSE( _effect->hasOverlay() );
    EXPECT_FALSE( penDown(0., 0.) );
    EXPECT_FALSE( penUp(0., 0.) );
    EXPECT_FALSE( _effect->onOverlayKeyDown_public(kTime, RenderScale(), ViewIdx(0), Key_A, KeyboardModifiers(), QString()) );
    _effect->onInteractViewportSelectionCleared( KeyboardModifiers() );
}

// A reader with an overlay (ReadSVG with the TransformInteract of openfx-arena#84): the Read node has
// its overlay, which edits the reader's parameters, and follows the reader when the file type changes.
TEST_F(ReadNodeOverlayTest, ReaderWithOverlay)
{
    if ( !isPluginAvailable(PLUGINID_OFX_READSVG) || !isPluginAvailable(PLUGINID_OFX_READOIIO) ) {
        reportSkipped("plug-ins not found (needs " PLUGINID_OFX_READSVG " from openfx-arena and " PLUGINID_OFX_READOIIO " from openfx-io in OFX_PLUGIN_PATH)");

        return;
    }
    createReadNode(_svgFile);
    NodePtr reader = getEmbeddedReader();
    ASSERT_TRUE( bool(reader) );
    ASSERT_EQ(reader->getPluginID(), std::string(PLUGINID_OFX_READSVG));
    if ( !reader->getEffectInstance()->hasOverlay() ) {
        reportSkipped(PLUGINID_OFX_READSVG " has no overlay interact (needs openfx-arena#84)");

        return;
    }
    EXPECT_TRUE( _effect->hasOverlay() );
    // The embedded reader got the viewport of the Read node.
    EXPECT_EQ( reader->getEffectInstance()->getCurrentViewportForOverlays(), &_viewport );

    KnobDoublePtr translate = std::dynamic_pointer_cast<KnobDouble>( _node->getKnobByName("transformTranslate") );
    KnobDoublePtr center = std::dynamic_pointer_cast<KnobDouble>( _node->getKnobByName("transformCenter") );
    ASSERT_TRUE( bool(translate) );
    ASSERT_TRUE( bool(center) );
    // ReadSVG centers its transform on the 100x100 canvas when it gets its file.
    EXPECT_DOUBLE_EQ( center->getValueAtTime(kTime, 0), 50. );
    EXPECT_DOUBLE_EQ( center->getValueAtTime(kTime, 1), 50. );
    const double x0 = center->getValueAtTime(kTime, 0) + translate->getValueAtTime(kTime, 0);
    const double y0 = center->getValueAtTime(kTime, 1) + translate->getValueAtTime(kTime, 1);
    const double tx0 = translate->getValueAtTime(kTime, 0);
    const double ty0 = translate->getValueAtTime(kTime, 1);

    // Drag the center of the TransformInteract: this translates the image.
    EXPECT_TRUE( penDown(x0, y0) );
    penMotion(x0 + 5., y0 + 2.);
    penMotion(x0 + 10., y0 + 4.);
    penUp(x0 + 10., y0 + 4.);
    EXPECT_DOUBLE_EQ( translate->getValueAtTime(kTime, 0), tx0 + 10. );
    EXPECT_DOUBLE_EQ( translate->getValueAtTime(kTime, 1), ty0 + 4. );

    // The selection rectangle and focus callbacks reach the reader without crashing.
    _effect->onInteractViewportSelectionCleared( KeyboardModifiers() );
    _effect->onInteractViewportSelectionUpdated(RectD(0., 0., 10., 10.), true, KeyboardModifiers() );
    bool focus = _effect->onOverlayFocusGained_public( kTime, RenderScale(), ViewIdx(0) );
    focus = _effect->onOverlayFocusLost_public( kTime, RenderScale(), ViewIdx(0) );
    Q_UNUSED(focus);

    // Another file type picks another reader, without overlay: so has the Read node.
    setFile(_exrFile);
    reader = getEmbeddedReader();
    ASSERT_TRUE( bool(reader) );
    EXPECT_EQ(reader->getPluginID(), std::string(PLUGINID_OFX_READOIIO));
    EXPECT_FALSE( _effect->hasOverlay() );
    _effect->setCurrentViewportForOverlays_public(&_viewport);
    EXPECT_FALSE( penDown(x0, y0) );
    EXPECT_FALSE( penUp(x0, y0) );

    // Back to an SVG file: a new ReadSVG, with a new overlay interact.
    setFile(_svgFile);
    reader = getEmbeddedReader();
    ASSERT_TRUE( bool(reader) );
    EXPECT_EQ(reader->getPluginID(), std::string(PLUGINID_OFX_READSVG));
    EXPECT_TRUE( _effect->hasOverlay() );
    _effect->setCurrentViewportForOverlays_public(&_viewport);
    EXPECT_EQ( reader->getEffectInstance()->getCurrentViewportForOverlays(), &_viewport );
    // The parameters of the previous ReadSVG were deleted with it.
    translate = std::dynamic_pointer_cast<KnobDouble>( _node->getKnobByName("transformTranslate") );
    center = std::dynamic_pointer_cast<KnobDouble>( _node->getKnobByName("transformCenter") );
    ASSERT_TRUE( bool(translate) );
    ASSERT_TRUE( bool(center) );
    // The new ReadSVG got the instance changed action for its file (it shares the filename knob of
    // the Read node, which already had that value), so it centered its transform on the canvas,
    // not on the project (the default of its normalised center parameter).
    EXPECT_DOUBLE_EQ( center->getValueAtTime(kTime, 0), 50. );
    EXPECT_DOUBLE_EQ( center->getValueAtTime(kTime, 1), 50. );
    const double x1 = center->getValueAtTime(kTime, 0) + translate->getValueAtTime(kTime, 0);
    const double y1 = center->getValueAtTime(kTime, 1) + translate->getValueAtTime(kTime, 1);
    EXPECT_TRUE( penDown(x1, y1) );
    penMotion(x1 + 3., y1);
    penUp(x1 + 3., y1);
    EXPECT_DOUBLE_EQ( translate->getValueAtTime(kTime, 0), x1 - center->getValueAtTime(kTime, 0) + 3. );
}

// A project with a Read node is saved and loaded: the reloaded Read node has its decoder, file and
// parameters, and still switches decoders after the load.
// Note: this does NOT test that the decoder misses the instance changed action of its filename at
// load (see ReadNodePrivate::createReadNode()): the parameters are loaded again after the decoder is
// created, so the values checked here are the same either way.
TEST_F(ReadNodeOverlayTest, ProjectSaveAndLoad)
{
    if ( !isPluginAvailable(PLUGINID_OFX_READOIIO) ) {
        reportSkipped("plug-in not found (needs " PLUGINID_OFX_READOIIO " from openfx-io in OFX_PLUGIN_PATH)");

        return;
    }
    // A sequence of three 2x2 PPM images
    for (int f = 1; f <= 3; ++f) {
        std::ofstream ppm( ( _dir.toStdString() + "/img" + std::to_string(f) + ".ppm" ).c_str(), std::ios::binary );
        ppm << "P6\n2 2\n255\n" << std::string(12, '\x80');
    }
    const std::string pattern = _dir.toStdString() + "/img#.ppm";
    createReadNode(pattern);
    ASSERT_EQ( getEmbeddedReader()->getPluginID(), std::string(PLUGINID_OFX_READOIIO) );
    KnobIntPtr firstFrame = std::dynamic_pointer_cast<KnobInt>( _node->getKnobByName("firstFrame") );
    KnobIntPtr lastFrame = std::dynamic_pointer_cast<KnobInt>( _node->getKnobByName("lastFrame") );
    ASSERT_TRUE( bool(firstFrame) );
    ASSERT_TRUE( bool(lastFrame) );
    EXPECT_EQ( firstFrame->getValue(), 1 );
    EXPECT_EQ( lastFrame->getValue(), 3 );
    // the user reads only frame 2
    firstFrame->setValue(2);
    lastFrame->setValue(2);
    const std::string name = _node->getScriptName_mt_safe();
    _effect->setCurrentViewportForOverlays_public(NULL);
    _effect.reset();
    _node.reset();
    firstFrame.reset();
    lastFrame.reset();

    const QString dir = _dir + QLatin1Char('/');
    QString savedPath;
    ASSERT_TRUE( _app->getProject()->saveProject( dir, QString::fromUtf8("ReadNodeOverlay.ntp"), &savedPath ) );
    // The project of the shared app now is in the temporary directory: reset it in TearDown().
    _projectLoaded = true;
    ASSERT_TRUE( _app->getProject()->loadProject( dir, QString::fromUtf8("ReadNodeOverlay.ntp") ) );

    _node = _app->getProject()->getNodeByName(name);
    ASSERT_TRUE( bool(_node) );
    _effect = _node->getEffectInstance();
    NodePtr reader = getEmbeddedReader();
    ASSERT_TRUE( bool(reader) );
    EXPECT_EQ( reader->getPluginID(), std::string(PLUGINID_OFX_READOIIO) );
    KnobFilePtr file = std::dynamic_pointer_cast<KnobFile>( _node->getKnobByName(kOfxImageEffectFileParamName) );
    ASSERT_TRUE( bool(file) );
    EXPECT_EQ( file->getValue(), pattern );
    firstFrame = std::dynamic_pointer_cast<KnobInt>( _node->getKnobByName("firstFrame") );
    lastFrame = std::dynamic_pointer_cast<KnobInt>( _node->getKnobByName("lastFrame") );
    ASSERT_TRUE( bool(firstFrame) );
    ASSERT_TRUE( bool(lastFrame) );
    EXPECT_EQ( firstFrame->getValue(), 2 );
    EXPECT_EQ( lastFrame->getValue(), 2 );
    EXPECT_FALSE( _effect->hasOverlay() );

    if ( isPluginAvailable(PLUGINID_OFX_READSVG) ) {
        // After the load, a new decoder still gets its file.
        setFile(_svgFile);
        reader = getEmbeddedReader();
        ASSERT_TRUE( bool(reader) );
        EXPECT_EQ( reader->getPluginID(), std::string(PLUGINID_OFX_READSVG) );
        if ( reader->getEffectInstance()->hasOverlay() ) {
            KnobDoublePtr center = std::dynamic_pointer_cast<KnobDouble>( _node->getKnobByName("transformCenter") );
            ASSERT_TRUE( bool(center) );
            EXPECT_DOUBLE_EQ( center->getValueAtTime(kTime, 0), 50. );
            EXPECT_DOUBLE_EQ( center->getValueAtTime(kTime, 1), 50. );
        }
    }
}
