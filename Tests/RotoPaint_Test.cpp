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

// Regression tests for https://github.com/NatronGitHub/Natron/issues/510
// ("Rotopaint doesn't apply stroke to pencil at the first frame it is created"), and for the other RotoPaint
// overlay issues cited by each test.
//
// The RotoPaint overlay interact is driven headless, with a mock viewport. Every user edit that changes what the
// node renders must evaluate the node (RotoContext::evaluateChange()), which increments the node hash: that is
// what invalidates the image cached for the current frame and makes the viewer render it again.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <algorithm>
#include <iostream>
#include <list>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include <QtCore/QPointF>
#include <QtCore/QString>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Bezier.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/OverlaySupport.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/PyRoto.h"
#include "Engine/RenderScale.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoLayer.h"
#include "Engine/RotoPaintInteract.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_USING

namespace {
// A viewport that does not draw anything: only what the RotoPaint overlay interact queries during pen events.
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
        return RectD(0., 0., 1000., 1000.);
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

// The overlay actions are protected in EffectInstance (and private in RotoPaint), and the public
// EffectInstance::onOverlayPen*_public() wrappers only forward them when the node has a GUI (hasHostOverlay()).
// A pointer to a protected member can be formed from a derived class: calling through it still dispatches
// to RotoPaint's override, without needing a GUI.
class OverlayActions
    : public EffectInstance
{
public:
    typedef bool (EffectInstance::*PenDownFn)(double, const RenderScale &, ViewIdx, const QPointF &, const QPointF &, double, double, PenType, KeyboardModifiers);
    typedef bool (EffectInstance::*PenMotionFn)(double, const RenderScale &, ViewIdx, const QPointF &, const QPointF &, double, double, KeyboardModifiers);
    typedef bool (EffectInstance::*PenUpFn)(double, const RenderScale &, ViewIdx, const QPointF &, const QPointF &, double, double, KeyboardModifiers);

    static PenDownFn penDown() { return &OverlayActions::onOverlayPenDown; }

    static PenMotionFn penMotion() { return &OverlayActions::onOverlayPenMotion; }

    static PenUpFn penUp() { return &OverlayActions::onOverlayPenUp; }

    typedef bool (EffectInstance::*KeyFn)(double, const RenderScale &, ViewIdx, Key, KeyboardModifiers, const QString &);

    static KeyFn keyDown() { return &OverlayActions::onOverlayKeyDown; }

    static KeyFn keyUp() { return &OverlayActions::onOverlayKeyUp; }
};

const double kTime = 1.;
} // anonymous namespace

class RotoPaintOverlayTest
    : public testing::Test
{
protected:

    virtual void SetUp() OVERRIDE FINAL
    {
        _app = appPTR->getTopLevelInstance();

        // Every shape is backed by OpenFX nodes (see RotoDrawableItem::createNodes()).
        _havePlugins = isPluginAvailable(PLUGINID_OFX_MERGE) && isPluginAvailable(PLUGINID_OFX_ROTO);
        if (!_havePlugins) {
            // The bundled google-test predates GTEST_SKIP(): the tests return early and are reported as passed,
            // so say clearly that they did not run, as OSGLContext_Test does.
            // TODO: Convert to GTEST_SKIP() when gtest updated.
            const testing::TestInfo* info = testing::UnitTest::GetInstance()->current_test_info();
            std::cerr << "[  SKIPPED ] " << info->test_case_name() << '.' << info->name()
                      << ": plug-ins not found (needs " PLUGINID_OFX_MERGE " and " PLUGINID_OFX_ROTO
                " from openfx-misc in OFX_PLUGIN_PATH)" << std::endl;

            return;
        }

        CreateNodeArgs args( PLUGINID_NATRON_ROTOPAINT, _app->getProject() );
        _node = _app->createNode(args);
        ASSERT_TRUE( bool(_node) );
        _effect = _node->getEffectInstance();
        ASSERT_TRUE( bool(_effect) );
        _context = _node->getRotoContext();
        ASSERT_TRUE( bool(_context) );
        _effect->setCurrentViewportForOverlays_public(&_viewport);
    }

    virtual void TearDown() OVERRIDE FINAL
    {
        if (_effect) {
            _effect->setCurrentViewportForOverlays_public(NULL);
        }
        if (_node) {
            _node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
        }
        _context.reset();
        _effect.reset();
        _node.reset();
    }

    // Activates a toolbar knob like NodeViewerContext does: when it is already on (e.g. the default tool)
    // setting its value is not a change, so at least a knobChanged() call must be issued.
    template <typename KNOB>
    void activate(const std::shared_ptr<KNOB>& knob)
    {
        if (knob->getValue() != true) {
            knob->setValue(true);
        } else {
            _effect->onKnobValueChanged_public(knob.get(), eValueChangedReasonUserEdited, _effect->getCurrentTime(), ViewSpec(0), true);
        }
    }

    // Selects a tool like the toolbar does: its group (role) first, then the tool itself.
    void selectTool(const char* groupName,
                    const char* toolName)
    {
        KnobGroupPtr group = std::dynamic_pointer_cast<KnobGroup>( _node->getKnobByName(groupName) );
        KnobButtonPtr tool = std::dynamic_pointer_cast<KnobButton>( _node->getKnobByName(toolName) );

        ASSERT_TRUE( bool(group) );
        ASSERT_TRUE( bool(tool) );
        activate(group);
        activate(tool);
    }

    void penDown(double x,
                 double y)
    {
        EXPECT_TRUE( ( (*_effect).*OverlayActions::penDown() )(kTime, RenderScale(), ViewIdx(0), QPointF(x, y), QPointF(x, y), 1., 0., ePenTypeLMB, KeyboardModifiers()) );
    }

    void penMotion(double x,
                   double y)
    {
        EXPECT_TRUE( ( (*_effect).*OverlayActions::penMotion() )(kTime, RenderScale(), ViewIdx(0), QPointF(x, y), QPointF(x, y), 1., 0., KeyboardModifiers()) );
    }

    void penUp(double x,
               double y)
    {
        // The return value only tells whether the interact needs a redraw
        const bool redraw = ( (*_effect).*OverlayActions::penUp() )(kTime, RenderScale(), ViewIdx(0), QPointF(x, y), QPointF(x, y), 1., 0., KeyboardModifiers());

        Q_UNUSED(redraw);
    }

    void keyDown(Key key)
    {
        // The return value only tells whether the interact used the key
        const bool used = ( (*_effect).*OverlayActions::keyDown() )(kTime, RenderScale(), ViewIdx(0), key, KeyboardModifiers(), QString());

        Q_UNUSED(used);
    }

    void keyUp(Key key)
    {
        const bool used = ( (*_effect).*OverlayActions::keyUp() )(kTime, RenderScale(), ViewIdx(0), key, KeyboardModifiers(), QString());

        Q_UNUSED(used);
    }

    // Paints a stroke from (x1, y1) to (x2, y2) with the selected paint tool.
    void paintStroke(double x1,
                     double y1,
                     double x2,
                     double y2)
    {
        penDown(x1, y1);
        penMotion( (x1 + x2) / 2., (y1 + y2) / 2. );
        penMotion(x2, y2);
        penUp(x2, y2);
    }

    // The strokes in render order: the first one painted first.
    std::list<RotoStrokeItemPtr> getStrokes() const
    {
        std::list<RotoStrokeItemPtr> strokes;
        std::list<RotoDrawableItemPtr> items = _context->getCurvesByRenderOrder(false);

        for (std::list<RotoDrawableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
            RotoStrokeItemPtr stroke = std::dynamic_pointer_cast<RotoStrokeItem>(*it);
            if (stroke) {
                strokes.push_back(stroke);
            }
        }

        return strokes;
    }

    // Two strokes with the Clone or Reveal tool from the foreground must be two items, the second one
    // cloning the first one.
    void checkForegroundStartsNewStroke(const char* toolName);

    BezierPtr getDrawnBezier() const
    {
        // onlyActivated = false: a closed bezier that is not finished yet is not listed otherwise
        std::list<RotoDrawableItemPtr> items = _context->getCurvesByRenderOrder(false);

        return items.empty() ? BezierPtr() : std::dynamic_pointer_cast<Bezier>( items.front() );
    }

    AppInstancePtr _app;
    bool _havePlugins;
    NodePtr _node;
    EffectInstancePtr _effect;
    RotoContextPtr _context;
    MockViewport _viewport;
};

// Drawing a Pencil stroke (an open bezier) must re-render the node when the pen is released. Otherwise the viewer
// keeps showing the image of the current frame cached before the stroke existed (the other frames are not cached
// yet, so they show it): issue 510.
TEST_F(RotoPaintOverlayTest, PencilStrokeEvaluatesNodeOnPenUp)
{
    if (!_havePlugins) {
        return;
    }
    selectTool(kRotoUIParamBezierEditionToolButton, kRotoUIParamPencilToolButtonAction);

    penDown(100., 100.);
    penMotion(140., 120.);
    const U64 ageBeforePenUp = _context->getAge();
    const U64 hashBeforePenUp = _node->getHashValue();
    penUp(140., 120.);

    BezierPtr bezier = getDrawnBezier();
    ASSERT_TRUE( bool(bezier) );
    EXPECT_TRUE( bezier->isOpenBezier() );
    EXPECT_GT(_context->getAge(), ageBeforePenUp);
    EXPECT_NE(_node->getHashValue(), hashBeforePenUp);

    // Extending the stroke with a second point must re-render it too.
    penDown(200., 150.);
    const U64 ageBeforeSecondPenUp = _context->getAge();
    const U64 hashBeforeSecondPenUp = _node->getHashValue();
    penUp(200., 150.);
    EXPECT_EQ(2, bezier->getControlPointsCount());
    EXPECT_GT(_context->getAge(), ageBeforeSecondPenUp);
    EXPECT_NE(_node->getHashValue(), hashBeforeSecondPenUp);
}

// With Multi-Stroke on (the default), a clone stroke from the foreground must start a new stroke, which clones
// what the strokes below it painted. It used to be appended to the previous clone stroke, so it could not clone
// what that one painted, and "foreground" did the same as "background": issue 629.
TEST_F(RotoPaintOverlayTest, CloneFromForegroundStartsNewStroke)
{
    if (!_havePlugins) {
        return;
    }
    checkForegroundStartsNewStroke(kRotoUIParamCloneToolButtonAction);
}

// The Reveal tool too: issue 629.
TEST_F(RotoPaintOverlayTest, RevealFromForegroundStartsNewStroke)
{
    if (!_havePlugins) {
        return;
    }
    checkForegroundStartsNewStroke(kRotoUIParamRevealToolButtonAction);
}

void
RotoPaintOverlayTest::checkForegroundStartsNewStroke(const char* toolName)
{
    selectTool(kRotoUIParamCloneBrushToolButton, toolName);
    KnobBoolPtr multiStroke = std::dynamic_pointer_cast<KnobBool>( _node->getKnobByName(kRotoUIParamMultiStrokeEnabled) );
    KnobChoicePtr sourceType = std::dynamic_pointer_cast<KnobChoice>( _node->getKnobByName(kRotoUIParamSourceType) );
    ASSERT_TRUE(multiStroke && sourceType);
    ASSERT_TRUE( multiStroke->getValue() );
    sourceType->setValue(0); // foreground

    paintStroke(100., 100., 150., 100.);
    paintStroke(300., 100., 350., 100.);

    std::list<RotoStrokeItemPtr> strokes = getStrokes();
    ASSERT_EQ( 2u, strokes.size() ) << "each foreground clone stroke must be a new stroke";
    RotoStrokeItemPtr first = strokes.front();
    RotoStrokeItemPtr second = strokes.back();
    EXPECT_EQ( 0, second->getBrushSourceTypeKnob()->getValue() );
    // the clone source (through the time offset node) of the second stroke is the output of the first one
    NodePtr source = second->getTimeOffsetNode();
    ASSERT_TRUE( bool(source) );
    EXPECT_EQ( first->getMergeNode(), source->getInput(0) );
}

// A clone stroke from a background input still goes to the same multi-stroke item: issue 629 must not change that.
TEST_F(RotoPaintOverlayTest, CloneFromBackgroundIsAppendedToMultiStroke)
{
    if (!_havePlugins) {
        return;
    }
    selectTool(kRotoUIParamCloneBrushToolButton, kRotoUIParamCloneToolButtonAction);
    KnobChoicePtr sourceType = std::dynamic_pointer_cast<KnobChoice>( _node->getKnobByName(kRotoUIParamSourceType) );
    ASSERT_TRUE( bool(sourceType) );
    ASSERT_EQ( 1, sourceType->getValue() ); // background, the default of the Clone tool

    paintStroke(100., 100., 150., 100.);
    paintStroke(300., 100., 350., 100.);

    EXPECT_EQ( 1u, getStrokes().size() );

    // changing the source starts a new stroke, with the new source
    sourceType->setValue(0);
    paintStroke(500., 100., 550., 100.);
    std::list<RotoStrokeItemPtr> strokes = getStrokes();
    ASSERT_EQ( 2u, strokes.size() );
    EXPECT_EQ( 0, strokes.back()->getBrushSourceTypeKnob()->getValue() );
}

// Unchecking Multi-Stroke after a multi-stroke must start a new item: the next stroke used to be painted into the
// item already in the hierarchy (added to it again), and undoing it removed the whole multi-stroke.
TEST_F(RotoPaintOverlayTest, UncheckingMultiStrokeStartsNewStroke)
{
    if (!_havePlugins) {
        return;
    }
    selectTool(kRotoUIParamPaintBrushToolButton, kRotoUIParamDrawBrushToolButtonAction);
    KnobBoolPtr multiStroke = std::dynamic_pointer_cast<KnobBool>( _node->getKnobByName(kRotoUIParamMultiStrokeEnabled) );
    ASSERT_TRUE( multiStroke && multiStroke->getValue() );

    paintStroke(100., 100., 150., 100.);
    paintStroke(300., 100., 350., 100.);
    ASSERT_EQ( 1u, getStrokes().size() );

    multiStroke->setValue(false);
    paintStroke(500., 100., 550., 100.);
    EXPECT_EQ( 2u, getStrokes().size() );
}

// A closed bezier is not rendered before it is finished (see RotoContextPrivate::renderBezier()): adding a point
// to it must not re-render the node. FinishingBezierByChangingToolEvaluatesNode checks that finishing it does.
TEST_F(RotoPaintOverlayTest, UnfinishedBezierPointDoesNotEvaluateNodeOnPenUp)
{
    if (!_havePlugins) {
        return;
    }
    selectTool(kRotoUIParamBezierEditionToolButton, kRotoUIParamDrawBezierToolButtonAction);

    penDown(100., 100.);
    penUp(100., 100.);
    penDown(300., 100.);
    const U64 ageBeforePenUp = _context->getAge();
    const U64 hashBeforePenUp = _node->getHashValue();
    penUp(300., 100.);

    BezierPtr bezier = getDrawnBezier();
    ASSERT_TRUE( bool(bezier) );
    EXPECT_FALSE( bezier->isOpenBezier() );
    EXPECT_FALSE( bezier->isCurveFinished() );
    EXPECT_EQ(2, bezier->getControlPointsCount());
    EXPECT_EQ(_context->getAge(), ageBeforePenUp);
    EXPECT_EQ(_node->getHashValue(), hashBeforePenUp);
}

// Choosing another tool of the same group finishes the closed bezier being drawn, which makes it renderable
// (closed beziers are not rendered before they are finished): that must evaluate the node as well.
TEST_F(RotoPaintOverlayTest, FinishingBezierByChangingToolEvaluatesNode)
{
    if (!_havePlugins) {
        return;
    }
    selectTool(kRotoUIParamBezierEditionToolButton, kRotoUIParamDrawBezierToolButtonAction);

    penDown(100., 100.);
    penUp(100., 100.);
    penDown(300., 100.);
    penUp(300., 100.);
    penDown(200., 250.);
    penUp(200., 250.);

    BezierPtr bezier = getDrawnBezier();
    ASSERT_TRUE( bool(bezier) );
    EXPECT_EQ(3, bezier->getControlPointsCount());
    EXPECT_FALSE( bezier->isCurveFinished() );

    const U64 ageBeforeToolChange = _context->getAge();
    const U64 hashBeforeToolChange = _node->getHashValue();
    selectTool(kRotoUIParamBezierEditionToolButton, kRotoUIParamPencilToolButtonAction);

    EXPECT_TRUE( bezier->isCurveFinished() );
    EXPECT_GT(_context->getAge(), ageBeforeToolChange);
    EXPECT_NE(_node->getHashValue(), hashBeforeToolChange);
}

// Python Layer.removeItem() must remove the item as the Roto panel does: deselect it and emit itemRemoved,
// which the Roto panel uses to remove the item from its tree. It used to call RotoLayer::removeItem() only,
// which left an empty item in the panel: issue 405.
TEST_F(RotoPaintOverlayTest, PythonLayerRemoveItemDeselectsAndNotifies)
{
    if (!_havePlugins) {
        return;
    }
    typedef NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Roto PyRotoContext;
    typedef NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Layer PyLayer;
    typedef NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::BezierCurve PyBezierCurve;
    PyRotoContext roto(_context);
    std::unique_ptr<PyBezierCurve> ellipse( roto.createEllipse(500., 500., 100., true, kTime) );
    ASSERT_TRUE( bool(ellipse) );
    BezierPtr bezier = std::dynamic_pointer_cast<Bezier>( ellipse->getInternalItem() );
    ASSERT_TRUE( bool(bezier) );
    RotoLayerPtr layer = bezier->getParentLayer();
    ASSERT_TRUE( bool(layer) );

    // Created before the selection is set: creating a layer selects it.
    std::unique_ptr<PyLayer> otherLayer( roto.createLayer() );
    ASSERT_TRUE( bool(otherLayer) );

    _context->clearSelection(RotoItem::eSelectionReasonOther);
    _context->select(bezier, RotoItem::eSelectionReasonOther);
    ASSERT_EQ( 1u, _context->getSelectedItems().size() );

    int removedCount = 0;
    QMetaObject::Connection connection =
        QObject::connect( _context.get(), &RotoContext::itemRemoved, [&removedCount, &bezier](const RotoItemPtr& item, int) {
        if (item == bezier) {
            ++removedCount;
        }
    } );

    // Removing an item from a layer it doesn't belong to does nothing.
    otherLayer->removeItem( ellipse.get() );
    EXPECT_EQ( layer, bezier->getParentLayer() );
    EXPECT_EQ(0, removedCount);
    EXPECT_EQ( 1u, _context->getSelectedItems().size() );

    PyLayer pyLayer(layer);
    pyLayer.removeItem( ellipse.get() );
    QObject::disconnect(connection);

    EXPECT_FALSE( bool( bezier->getParentLayer() ) );
    EXPECT_TRUE( _context->getSelectedItems().empty() ) << "the removed item must be deselected";
    EXPECT_EQ(1, removedCount) << "itemRemoved must be emitted, for the Roto panel";
    const std::list<RotoItemPtr>& items = layer->getItems();
    EXPECT_TRUE( std::find(items.begin(), items.end(), RotoItemPtr(bezier)) == items.end() );
}

// Python Roto.getSelectedItems() returns the selected items, with the wrapper of their type: issue 406.
TEST_F(RotoPaintOverlayTest, PythonRotoGetSelectedItems)
{
    if (!_havePlugins) {
        return;
    }
    typedef NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Roto PyRotoContext;
    typedef NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::ItemBase PyItemBase;
    typedef NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::BezierCurve PyBezierCurve;
    PyRotoContext roto(_context);
    std::unique_ptr<PyBezierCurve> ellipse( roto.createEllipse(500., 500., 100., true, kTime) );
    std::unique_ptr<PyBezierCurve> rectangle( roto.createRectangle(100., 100., 50., kTime) );
    ASSERT_TRUE( ellipse && rectangle );

    _context->clearSelection(RotoItem::eSelectionReasonOther);
    EXPECT_TRUE( roto.getSelectedItems().empty() );

    _context->select(rectangle->getInternalItem(), RotoItem::eSelectionReasonOther);
    std::list<PyItemBase*> selected = roto.getSelectedItems();
    ASSERT_EQ( 1u, selected.size() );
    EXPECT_EQ( rectangle->getInternalItem(), selected.front()->getInternalItem() );
    EXPECT_TRUE( dynamic_cast<PyBezierCurve*>( selected.front() ) != NULL ) << "a Bezier is returned as a BezierCurve";
    for (std::list<PyItemBase*>::iterator it = selected.begin(); it != selected.end(); ++it) {
        delete *it;
    }

    _context->clearSelection(RotoItem::eSelectionReasonOther);
    EXPECT_TRUE( roto.getSelectedItems().empty() );
}

// A layer removed with Layer.removeItem() and added back with Layer.addItem() must be registered in the context
// again and shown in the Roto panel (itemInserted), with its children: removeItem() takes it out of the context.
TEST_F(RotoPaintOverlayTest, PythonLayerAddItemAfterRemoveItem)
{
    if (!_havePlugins) {
        return;
    }
    typedef NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Roto PyRotoContext;
    typedef NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::Layer PyLayer;
    typedef NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::BezierCurve PyBezierCurve;
    PyRotoContext roto(_context);
    std::unique_ptr<PyLayer> baseLayer( roto.getBaseLayer() );
    std::unique_ptr<PyLayer> layer( roto.createLayer() );
    std::unique_ptr<PyBezierCurve> ellipse( roto.createEllipse(500., 500., 100., true, kTime) );
    ASSERT_TRUE( baseLayer && layer && ellipse );
    RotoLayerPtr internalLayer = std::dynamic_pointer_cast<RotoLayer>( layer->getInternalItem() );
    ASSERT_TRUE( bool(internalLayer) );
    layer->addItem( ellipse.get() ); // a move: the ellipse is in a layer
    ASSERT_EQ( internalLayer, ellipse->getInternalItem()->getParentLayer() );

    baseLayer->removeItem( layer.get() );
    const std::list<RotoLayerPtr>& layersAfterRemove = _context->getLayers();
    EXPECT_TRUE( std::find(layersAfterRemove.begin(), layersAfterRemove.end(), internalLayer) == layersAfterRemove.end() );

    int insertedCount = 0;
    QMetaObject::Connection connection =
        QObject::connect( _context.get(), &RotoContext::itemInserted, [&insertedCount](int, int) {
        ++insertedCount;
    } );
    baseLayer->addItem( layer.get() );
    QObject::disconnect(connection);

    EXPECT_EQ(1, insertedCount) << "itemInserted must be emitted, for the Roto panel";
    EXPECT_EQ( baseLayer->getInternalItem(), internalLayer->getParentLayer() );
    EXPECT_EQ( layer->getInternalItem(), _context->getLastInsertedItem() );
    const std::list<RotoLayerPtr>& layers = _context->getLayers();
    EXPECT_TRUE( std::find(layers.begin(), layers.end(), internalLayer) != layers.end() ) << "the layer must be in the context again";
    EXPECT_EQ( internalLayer, ellipse->getInternalItem()->getParentLayer() ) << "the children stay in the layer";

    // Python None is ignored
    baseLayer->addItem(NULL);
    baseLayer->insertItem(0, NULL);
}

// With a paint tool, Ctrl+Shift+drag changes the brush opacity, and Shift+drag the brush size. Ctrl+Shift+drag
// used to change the size, and a small move to the left made the brush more opaque: issue 1813.
TEST_F(RotoPaintOverlayTest, CtrlShiftDragChangesBrushOpacity)
{
    if (!_havePlugins) {
        return;
    }
    selectTool(kRotoUIParamPaintBrushToolButton, kRotoUIParamDrawBrushToolButtonAction);
    KnobDoublePtr opacity = std::dynamic_pointer_cast<KnobDouble>( _node->getKnobByName(kRotoUIParamOpacity) );
    KnobDoublePtr size = std::dynamic_pointer_cast<KnobDouble>( _node->getKnobByName(kRotoUIParamSize) );
    ASSERT_TRUE(opacity && size);
    opacity->setValue(0.5);
    const double sizeBefore = size->getValue();

    keyDown(Key_Control_L);
    keyDown(Key_Shift_L);
    penDown(100., 100.);
    penMotion(110., 110.); // to the right: more opaque
    EXPECT_NEAR(0.55, opacity->getValue(), 1e-9);
    penMotion(109.5, 109.5); // a little to the left: less opaque
    penUp(109.5, 109.5);
    keyUp(Key_Shift_L);
    keyUp(Key_Control_L);

    EXPECT_DOUBLE_EQ( sizeBefore, size->getValue() );
    EXPECT_NEAR(0.5, opacity->getValue(), 1e-9);
    EXPECT_TRUE( _context->getCurvesByRenderOrder(false).empty() ) << "the drag must not paint";
}

// Selecting the Clone or Reveal tool again keeps the source the user chose for it: it used to be reset to
// the tool's default (background for Clone), also when a tablet eraser switched back to the tool: issue 1814.
TEST_F(RotoPaintOverlayTest, CloneToolKeepsChosenSource)
{
    if (!_havePlugins) {
        return;
    }
    KnobChoicePtr sourceType = std::dynamic_pointer_cast<KnobChoice>( _node->getKnobByName(kRotoUIParamSourceType) );
    ASSERT_TRUE( bool(sourceType) );

    selectTool(kRotoUIParamCloneBrushToolButton, kRotoUIParamCloneToolButtonAction);
    EXPECT_EQ( 1, sourceType->getValue() ); // background, the default of the Clone tool
    sourceType->setValue(0); // foreground

    selectTool(kRotoUIParamCloneBrushToolButton, kRotoUIParamRevealToolButtonAction);
    EXPECT_EQ( 2, sourceType->getValue() ); // background 2, the default of the Reveal tool
    sourceType->setValue(3); // background 3

    selectTool(kRotoUIParamPaintBrushToolButton, kRotoUIParamDrawBrushToolButtonAction);
    selectTool(kRotoUIParamCloneBrushToolButton, kRotoUIParamCloneToolButtonAction);
    EXPECT_EQ( 0, sourceType->getValue() ) << "the Clone tool lost its source";

    // as a tablet eraser does: the eraser, then back to the last paint tool
    selectTool(kRotoUIParamPaintBrushToolButton, kRotoUIParamEraserToolButtonAction);
    selectTool(kRotoUIParamCloneBrushToolButton, kRotoUIParamCloneToolButtonAction);
    EXPECT_EQ( 0, sourceType->getValue() ) << "the Clone tool lost its source after the eraser";

    selectTool(kRotoUIParamCloneBrushToolButton, kRotoUIParamRevealToolButtonAction);
    EXPECT_EQ( 3, sourceType->getValue() ) << "the Reveal tool lost its source";
}
