/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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


#include "TrackerNodePrivate.h"

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/algorithm/clamp.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Engine/AppInstance.h"
#include "Engine/Image.h"
#include "Engine/Lut.h"
#include "Engine/Node.h"
#include "Engine/OpenGLViewerI.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/OSGLContext.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Project.h"
#include "Engine/RenderEngine.h"
#include "Engine/TimeLine.h"
#include "Engine/TrackerNode.h"
#include "Engine/TrackerHelper.h"
#include "Engine/TrackMarker.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerInstance.h"

NATRON_NAMESPACE_ENTER



TrackerNodePrivate::TrackerNodePrivate(TrackerNode* publicInterface)
    : TrackerParamsProvider()
    , publicInterface(publicInterface)
    , ui()
{
}

TrackerNodePrivate::~TrackerNodePrivate()
{

}

TrackerNodeInteract::TrackerNodeInteract(TrackerNodePrivate* p)
    : OverlayInteractBase()
    , _imp(p)
    , addTrackButton()
    , trackRangeButton()
    , trackBwButton()
    , trackPrevButton()
    , trackNextButton()
    , trackFwButton()
    , trackAllKeyframesButton()
    , trackCurrentKeyframeButton()
    , clearAllAnimationButton()
    , clearBwAnimationButton()
    , clearFwAnimationButton()
    , updateViewerButton()
    , centerViewerButton()
    , createKeyOnMoveButton()
    , setKeyFrameButton()
    , removeKeyFrameButton()
    , resetOffsetButton()
    , resetTrackButton()
    , showCorrelationButton()
    , clickToAddTrackEnabled(false)
    , lastMousePos()
    , selectionRectangle()
    , controlDown(0)
    , shiftDown(0)
    , altDown(0)
    , eventState(eMouseStateIdle)
    , hoverState(eDrawStateInactive)
    , interactMarker()
    , trackTextures()
    , trackRequestsMap()
    , selectedMarkerTexture()
    , selectedMarker()
    , pboID(0)
    , imageGetterWatcher()
    , showMarkerTexture(false)
    , selectedMarkerScale()
    , selectedMarkerImg()
    , isTracking(false)
    , nSelectedMarkerTextureRefreshRequests(0)
{
    selectedMarkerScale.x = selectedMarkerScale.y = 1.;

    QObject::connect(this, SIGNAL(requestRefreshSelectedMarkerTexture()), this, SLOT(onRequestRefreshSelectedMarkerReceived()), Qt::QueuedConnection);
}

TrackerNodeInteract::~TrackerNodeInteract()
{
    if (pboID != 0) {
        GL_GPU::DeleteBuffers(1, &pboID);
    }
}

TrackerKnobItemsTable::TrackerKnobItemsTable(TrackerNodePrivate* imp, KnobItemsTableTypeEnum type)
: KnobItemsTable(imp->publicInterface->shared_from_this(), type)
, _imp(imp)
{

}

void
TrackerNodePrivate::getMotionModelsAndHelps(bool addPerspective,
                                        std::vector<ChoiceOption>* models,
                                        std::map<int, std::string> *icons)
{
    models->push_back(ChoiceOption("Trans.", "", tr(kTrackerParamMotionModelTranslation).toStdString()));
    (*icons)[0] = NATRON_IMAGES_PATH "motionTypeT.png";
    models->push_back(ChoiceOption("Trans.+Rot.", "", tr(kTrackerParamMotionModelTransRot).toStdString()));
    (*icons)[1] = NATRON_IMAGES_PATH "motionTypeRT.png";
    models->push_back(ChoiceOption("Trans.+Scale", "", tr(kTrackerParamMotionModelTransScale).toStdString()));
    (*icons)[2] = NATRON_IMAGES_PATH "motionTypeTS.png";
    models->push_back(ChoiceOption("Trans.+Rot.+Scale", "", tr(kTrackerParamMotionModelTransRotScale).toStdString()));
    (*icons)[3] = NATRON_IMAGES_PATH "motionTypeRTS.png";
    models->push_back(ChoiceOption("Affine", "", tr(kTrackerParamMotionModelAffine).toStdString()));
    (*icons)[4] = NATRON_IMAGES_PATH "motionTypeAffine.png";
    if (addPerspective) {
        models->push_back(ChoiceOption("Perspective", "", tr(kTrackerParamMotionModelPerspective).toStdString()));
        (*icons)[5] = NATRON_IMAGES_PATH "motionTypePerspective.png";
    }
}


void
TrackerNodeInteract::onTrackRangeClicked()
{

    TimeValue projFirst,projLast;
    _imp->publicInterface->getApp()->getProject()->getFrameRange(&projFirst, &projLast);

    int first = trackRangeDialogFirstFrame.lock()->getValue();
    int last = trackRangeDialogLastFrame.lock()->getValue();
    int step = trackRangeDialogStep.lock()->getValue();
    if (first == INT_MIN) {
        trackRangeDialogFirstFrame.lock()->setValue(projFirst);
    }
    if (last == INT_MIN) {
        trackRangeDialogLastFrame.lock()->setValue(projLast);
    }
    if (step == INT_MIN) {
        trackRangeDialogStep.lock()->setValue(1);
    }
    KnobGroupPtr k = trackRangeDialogGroup.lock();
    if ( k->getValue() ) {
        k->setValue(false);
    } else {
        k->setValue(true);
    }
}

void
TrackerNodeInteract::onTrackAllKeyframesClicked()
{
    std::list<TrackMarkerPtr> selectedMarkers;
    _imp->knobsTable->getSelectedMarkers(&selectedMarkers);

    std::set<int> userKeys;

    for (std::list<TrackMarkerPtr>::iterator it = selectedMarkers.begin(); it != selectedMarkers.end(); ++it) {
        KeyFrameSet trackUserKeys = (*it)->getKeyFrames();
        for (KeyFrameSet::const_iterator it2 = trackUserKeys.begin(); it2 != trackUserKeys.end(); ++it2) {
            userKeys.insert(it2->getTime());
        }
    }
    if ( userKeys.empty() ) {
        return;
    }

    OverlaySupport* overlay = getLastCallingViewport();
    assert(overlay);
    int first = *userKeys.begin();
    int last = *userKeys.rbegin() + 1;
    _imp->trackSelectedMarkers( TimeValue(first), TimeValue(last), TimeValue(1),  overlay );
}

void
TrackerNodeInteract::onTrackCurrentKeyframeClicked()
{
    TimeValue currentFrame(_imp->publicInterface->getTimelineCurrentTime());
    std::list<TrackMarkerPtr> selectedMarkers;

    _imp->knobsTable->getSelectedMarkers(&selectedMarkers);

    std::set<int> userKeys;

    for (std::list<TrackMarkerPtr>::iterator it = selectedMarkers.begin(); it != selectedMarkers.end(); ++it) {
        KeyFrameSet trackUserKeys = (*it)->getKeyFrames();
        for (KeyFrameSet::const_iterator it2 = trackUserKeys.begin(); it2 != trackUserKeys.end(); ++it2) {
            userKeys.insert(it2->getTime());
        }
    }
    if ( userKeys.empty() ) {
        return;
    }

    std::set<int>::iterator it = userKeys.lower_bound(currentFrame);
    if ( it == userKeys.end() ) {
        return;
    }

    int last = *it + 1;
    int first;
    if ( it == userKeys.begin() ) {
        first = *it;
    } else {
        --it;
        first = *it;
    }
    OverlaySupport* overlay = getLastCallingViewport();
    assert(overlay);

    _imp->trackSelectedMarkers( TimeValue(first), TimeValue(last), TimeValue(1.), overlay );
}

void
TrackerNodeInteract::onTrackBwClicked()
{
    OverlaySupport* overlay = getLastCallingViewport();
    assert(overlay);

    RangeD range = overlay->getFrameRange();

    TimeValue startFrame = _imp->publicInterface->getTimelineCurrentTime();

    if ( _imp->tracker->isCurrentlyTracking() ) {
        _imp->tracker->abortTracking();
    } else {
        _imp->trackSelectedMarkers( TimeValue(startFrame), TimeValue(range.min - 1), TimeValue(-1),  overlay );
    }
}

void
TrackerNodeInteract::onTrackPrevClicked()
{
    OverlaySupport* overlay = getLastCallingViewport();
    assert(overlay);
    TimeValue startFrame = _imp->publicInterface->getTimelineCurrentTime();
    _imp->trackSelectedMarkers( TimeValue(startFrame), TimeValue(startFrame - 2), TimeValue(-1), overlay );
}

void
TrackerNodeInteract::onStopButtonClicked()
{
    _imp->tracker->abortTracking();
}

void
TrackerNodeInteract::onTrackNextClicked()
{
    OverlaySupport* overlay = getLastCallingViewport();
    assert(overlay);
    TimeValue startFrame = _imp->publicInterface->getTimelineCurrentTime();
    _imp->trackSelectedMarkers( TimeValue(startFrame), TimeValue(startFrame + 2), TimeValue(1.), overlay );
}

void
TrackerNodeInteract::onTrackFwClicked()
{
    OverlaySupport* overlay = getLastCallingViewport();
    assert(overlay);
    RangeD range = overlay->getFrameRange();

    TimeValue startFrame = _imp->publicInterface->getTimelineCurrentTime();
    if ( _imp->tracker->isCurrentlyTracking() ) {
        _imp->tracker->abortTracking();
    } else {
        _imp->trackSelectedMarkers( TimeValue(startFrame), TimeValue(range.max + 1), TimeValue(1.),  overlay );
    }
}

void
TrackerNodeInteract::onAddTrackClicked(bool clicked)
{
    clickToAddTrackEnabled = clicked;
}

void
TrackerNodeInteract::onClearAllAnimationClicked()
{
    std::list<TrackMarkerPtr> markers;
    _imp->knobsTable->getSelectedMarkers(&markers);
    for (std::list<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->clearAnimation();
    }
}

void
TrackerNodeInteract::onClearBwAnimationClicked()
{
    TimeValue time = _imp->publicInterface->getTimelineCurrentTime();
    std::list<TrackMarkerPtr> markers;

    _imp->knobsTable->getSelectedMarkers(&markers);
    for (std::list<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->clearAnimationBeforeTime(time);
    }
}

void
TrackerNodeInteract::onClearFwAnimationClicked()
{
    TimeValue time = _imp->publicInterface->getTimelineCurrentTime();
    std::list<TrackMarkerPtr> markers;

    _imp->knobsTable->getSelectedMarkers(&markers);
    for (std::list<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->clearAnimationAfterTime(time);
    }
}

void
TrackerNodeInteract::onSetKeyframeButtonClicked()
{
    TimeValue time = _imp->publicInterface->getTimelineCurrentTime();
    std::list<TrackMarkerPtr> markers;

    _imp->knobsTable->getSelectedMarkers(&markers);
    for (std::list<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->setKeyFrame(time);
    }
}

void
TrackerNodeInteract::onRemoveKeyframeButtonClicked()
{
    TimeValue time = _imp->publicInterface->getTimelineCurrentTime();
    std::list<TrackMarkerPtr> markers;

    _imp->knobsTable->getSelectedMarkers(&markers);
    for (std::list<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->removeKeyFrame(time);
    }
}

void
TrackerNodeInteract::onResetOffsetButtonClicked()
{
    std::list<TrackMarkerPtr> markers;

    _imp->knobsTable->getSelectedMarkers(&markers);
    for (std::list<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
        KnobDoublePtr offsetKnob = (*it)->getOffsetKnob();
        assert(offsetKnob);
        offsetKnob->resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());
    }
}

void
TrackerNodeInteract::onResetTrackButtonClicked()
{

    std::list<TrackMarkerPtr> markers;
    _imp->knobsTable->getSelectedMarkers(&markers);
    _imp->knobsTable->clearSelection(eTableChangeReasonInternal);

    std::list<KnobTableItemPtr> itemsList;
    for (std::list<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->resetTrack();
        itemsList.push_back(*it);
    }
    _imp->knobsTable->beginEditSelection();
    _imp->knobsTable->addToSelection(itemsList, eTableChangeReasonInternal);
    _imp->knobsTable->endEditSelection(eTableChangeReasonInternal);
}

QPointF
TrackerNodeInteract::computeMidPointExtent(const QPointF& prev,
                                           const QPointF& next,
                                           const QPointF& point,
                                           const QPointF& handleSize)
{
    Point leftDeriv, rightDeriv;

    leftDeriv.x = prev.x() - point.x();
    leftDeriv.y = prev.y() - point.y();

    rightDeriv.x = next.x() - point.x();
    rightDeriv.y = next.y() - point.y();
    double derivNorm = std::sqrt( (rightDeriv.x - leftDeriv.x) * (rightDeriv.x - leftDeriv.x) + (rightDeriv.y - leftDeriv.y) * (rightDeriv.y - leftDeriv.y) );
    QPointF ret;
    if (derivNorm == 0) {
        double norm = std::sqrt( ( leftDeriv.x - point.x() ) * ( leftDeriv.x - point.x() ) + ( leftDeriv.y - point.y() ) * ( leftDeriv.y  - point.y() ) );
        if (norm != 0) {
            ret.rx() = point.x() + ( ( leftDeriv.y - point.y() ) / norm ) * handleSize.x();
            ret.ry() = point.y() - ( ( leftDeriv.x - point.x() ) / norm ) * handleSize.y();

            return ret;
        } else {
            return QPointF(0, 0);
        }
    } else {
        ret.rx() = point.x() + ( (rightDeriv.y - leftDeriv.y) / derivNorm ) * handleSize.x();
        ret.ry() = point.y() - ( (rightDeriv.x - leftDeriv.x) / derivNorm ) * handleSize.y();
    }

    return ret;
}

int
TrackerNodeInteract::isInsideKeyFrameTexture(TimeValue currentTime,
                                             const QPointF& pos,
                                             const QPointF& viewportPos) const
{
    if (!showMarkerTexture) {
        return INT_MAX;
    }


    RectD textureRectCanonical;
    if (selectedMarkerTexture) {
        computeSelectedMarkerCanonicalRect(&textureRectCanonical);
    }

    if ( (pos.y() < textureRectCanonical.y1) || (pos.y() > textureRectCanonical.y2) ) {
        return INT_MAX;
    }
    if (pos.x() < textureRectCanonical.x2) {
        return INT_MAX;
    }

    TrackMarkerPtr marker = selectedMarker.lock();
    if (!marker) {
        return INT_MAX;
    }

    OverlaySupport* overlay = getLastCallingViewport();
    assert(overlay);

    //Find out which keyframe it is by counting keyframe portions
    int xRightMainTexture = overlay->toWidgetCoordinates( QPointF(textureRectCanonical.x2, 0) ).x();
    const double keyWidthpx = TO_DPIX(SELECTED_MARKER_KEYFRAME_WIDTH_SCREEN_PX);
    double indexF = (viewportPos.x() - xRightMainTexture) / keyWidthpx;
    int texIndex = (int)std::floor(indexF);

    for (TrackKeysMap::const_iterator it = trackTextures.begin(); it != trackTextures.end(); ++it) {
        if (it->first.lock() == marker) {
            if ( it->second.empty() ) {
                break;
            }
            ///Render at most MAX_TRACK_KEYS_TO_DISPLAY keyframes
            KeyFrameTexIDs keysToRender = getKeysToRenderForMarker(currentTime, it->second);
            if ( (texIndex < 0) || ( texIndex >= (int)keysToRender.size() ) ) {
                return INT_MAX;
            }
            KeyFrameTexIDs::iterator found = keysToRender.begin();
            std::advance(found, texIndex);
            RectD texCanonicalRect;
            computeTextureCanonicalRect(*found->second, indexF * keyWidthpx + xRightMainTexture,
                                        keyWidthpx, &texCanonicalRect);

            if ( (pos.y() >= texCanonicalRect.y1) && (pos.y() < texCanonicalRect.y2) ) {
                return found->first;
            }
            break;
        }
    }

    return INT_MAX;
} // isInsideKeyFrameTexture

bool
TrackerNodeInteract::isNearbySelectedMarkerTextureResizeAnchor(const QPointF& pos) const
{
    RectD textureRectCanonical;

    computeSelectedMarkerCanonicalRect(&textureRectCanonical);

    OverlaySupport* overlay = getLastCallingViewport();
    assert(overlay);
    QPointF clickWidget = overlay->toWidgetCoordinates(pos);
    QPointF btmRightWidget = overlay->toWidgetCoordinates( QPointF(textureRectCanonical.x2, textureRectCanonical.y1) );
    double tolerance = TO_DPIX(POINT_TOLERANCE);
    if ( ( clickWidget.x() >= (btmRightWidget.x() - tolerance) ) && ( clickWidget.x() <= (btmRightWidget.x() + tolerance) ) &&
         ( clickWidget.y() >= (btmRightWidget.y() - tolerance) ) && ( clickWidget.y() <= (btmRightWidget.y() + tolerance) ) ) {
        return true;
    }

    return false;
}

bool
TrackerNodeInteract::isInsideSelectedMarkerTexture(const QPointF& pos) const
{
    RectD textureRectCanonical;

    computeSelectedMarkerCanonicalRect(&textureRectCanonical);

    OverlaySupport* overlay = getLastCallingViewport();
    assert(overlay);
    QPointF clickWidget = overlay->toWidgetCoordinates(pos);
    QPointF btmRightWidget = overlay->toWidgetCoordinates( QPointF(textureRectCanonical.x2, textureRectCanonical.y1) );
    QPointF topLeftWidget = overlay->toWidgetCoordinates( QPointF(textureRectCanonical.x1, textureRectCanonical.y2) );
    RectD rect;
    rect.x1 = topLeftWidget.x();
    rect.y1 = topLeftWidget.y();
    rect.x2 = btmRightWidget.x();
    rect.y2 = btmRightWidget.y();

    return rect.contains( clickWidget.x(), clickWidget.y() );
}

void
TrackerNodeInteract::computeTextureCanonicalRect(const Texture& tex,
                                                 int xOffset,
                                                 int texWidthPx,
                                                 RectD* rect) const
{
    ///Preserve width
    if (tex.h() == 0 || tex.w() == 0) {
        return;
    }
    double par = tex.w() / (double)tex.h();
    OverlaySupport* overlay = getLastCallingViewport();

    assert(overlay);
    rect->x2 = overlay->toCanonicalCoordinates( QPointF(xOffset + texWidthPx, 0.) ).x();
    QPointF topLeft = overlay->toCanonicalCoordinates( QPointF(xOffset, 0.) );
    rect->x1 = topLeft.x();
    rect->y2 = topLeft.y();
    double height = rect->width() / par;
    rect->y1 = rect->y2 - height;
}

void
TrackerNodeInteract::computeSelectedMarkerCanonicalRect(RectD* rect) const
{
    if (!selectedMarkerTexture) {
        rect->clear();
        return;
    }
    assert(selectedMarkerTexture);
    int selectedMarkerWidth = magWindowPxSizeKnob.lock()->getValue();
    computeTextureCanonicalRect(*selectedMarkerTexture, 0, selectedMarkerWidth, rect);
}

Point
TrackerNodeInteract::toMagWindowPoint(const Point& ptnPoint,
                                      const RectD& canonicalSearchWindow,
                                      const RectD& textureRectCanonical)
{
    Point ret;
    double xCenterPercent = (ptnPoint.x - canonicalSearchWindow.x1) / (canonicalSearchWindow.x2 - canonicalSearchWindow.x1);
    double yCenterPercent = (ptnPoint.y - canonicalSearchWindow.y1) / (canonicalSearchWindow.y2 - canonicalSearchWindow.y1);

    ret.y = textureRectCanonical.y1 + yCenterPercent * (textureRectCanonical.y2 - textureRectCanonical.y1);
    ret.x = textureRectCanonical.x1 + xCenterPercent * (textureRectCanonical.x2 - textureRectCanonical.x1);

    return ret;
}

void
TrackerNodeInteract::drawEllipse(double x,
                                 double y,
                                 double radiusX,
                                 double radiusY,
                                 int l,
                                 double r,
                                 double g,
                                 double b,
                                 double a)
{
    GL_GPU::Color3f(r * l * a, g * l * a, b * l * a);

    GL_GPU::PushMatrix();
    //  center the oval at x_center, y_center
    GL_GPU::Translatef( (float)x, (float)y, 0.f );
    //  draw the oval using line segments
    GL_GPU::Begin(GL_LINE_LOOP);
    // we don't need to be pixel-perfect here, it's just an interact!
    // 40 segments is enough.
    double m = 2 * 3.14159265358979323846264338327950288419717 / 40.;
    for (int i = 0; i < 40; ++i) {
        double theta = i * m;
        GL_GPU::Vertex2d( radiusX * std::cos(theta), radiusY * std::sin(theta) );
    }
    GL_GPU::End();

    GL_GPU::PopMatrix();
}

TrackerNodeInteract::KeyFrameTexIDs
TrackerNodeInteract::getKeysToRenderForMarker(TimeValue currentTime,
                                              const KeyFrameTexIDs& allKeys)
{
    KeyFrameTexIDs keysToRender;
    ///Find the first key equivalent to currentTime or after
    KeyFrameTexIDs::const_iterator lower = allKeys.lower_bound(currentTime);
    KeyFrameTexIDs::const_iterator prev = lower;

    if ( lower != allKeys.begin() ) {
        --prev;
    } else {
        prev = allKeys.end();
    }

    for (int i = 0; i < MAX_TRACK_KEYS_TO_DISPLAY; ++i) {
        if ( lower != allKeys.end() ) {
            keysToRender.insert(*lower);
            ++lower;
        }
        if ( prev != allKeys.end() ) {
            keysToRender.insert(*prev);
            if ( prev != allKeys.begin() ) {
                --prev;
            } else {
                prev = allKeys.end();
            }
        } else {
            if ( lower == allKeys.end() ) {
                ///No more keyframes
                break;
            }
        }
    }

    return keysToRender;
}

void
TrackerNodeInteract::drawSelectedMarkerKeyframes(const std::pair<double, double>& pixelScale,
                                                 TimeValue currentTime)
{
    TrackMarkerPtr marker = selectedMarker.lock();

    assert(marker);
    if (!marker) {
        return;
    }
    if ( !marker->isEnabled(currentTime) ) {
        return;
    }

    OverlaySupport* overlay = getLastCallingViewport();
    assert(overlay);

    double overlayColor[4];
    if ( !_imp->publicInterface->getNode()->getOverlayColor(&overlayColor[0], &overlayColor[1], &overlayColor[2]) ) {
        overlayColor[0] = overlayColor[1] = overlayColor[2] = 0.8;
    }
    overlayColor[3] = 1.;

    KnobDoublePtr centerKnob = marker->getCenterKnob();
    KnobDoublePtr offsetKnob = marker->getOffsetKnob();
    KnobDoublePtr errorKnob = marker->getErrorKnob();
    KnobDoublePtr ptnTopLeft = marker->getPatternTopLeftKnob();
    KnobDoublePtr ptnTopRight = marker->getPatternTopRightKnob();
    KnobDoublePtr ptnBtmRight = marker->getPatternBtmRightKnob();
    KnobDoublePtr ptnBtmLeft = marker->getPatternBtmLeftKnob();
    KnobDoublePtr searchWndBtmLeft = marker->getSearchWindowBottomLeftKnob();
    KnobDoublePtr searchWndTopRight = marker->getSearchWindowTopRightKnob();
    int fontHeight = overlay->getWidgetFontHeight();

    int selectedMarkerWidth = magWindowPxSizeKnob.lock()->getValue();
    double xOffsetPixels = selectedMarkerWidth;
    QPointF viewerTopLeftCanonical = overlay->toCanonicalCoordinates( QPointF(0, 0.) );


    for (TrackKeysMap::iterator it = trackTextures.begin(); it != trackTextures.end(); ++it) {
        if (it->first.lock() == marker) {
            if ( it->second.empty() ) {
                break;
            }
            ///Render at most MAX_TRACK_KEYS_TO_DISPLAY keyframes
            KeyFrameTexIDs keysToRender = getKeysToRenderForMarker(currentTime, it->second);

            for (KeyFrameTexIDs::const_iterator it2 = keysToRender.begin(); it2 != keysToRender.end(); ++it2) {
                TimeValue time = it2->first;
                Point offset, center, topLeft, topRight, btmRight, btmLeft;

                center.x = centerKnob->getValueAtTime(time, DimIdx(0));
                center.y = centerKnob->getValueAtTime(time, DimIdx(1));
                offset.x = offsetKnob->getValueAtTime(time, DimIdx(0));
                offset.y = offsetKnob->getValueAtTime(time, DimIdx(1));

                topLeft.x = ptnTopLeft->getValueAtTime(time, DimIdx(0)) + offset.x + center.x;
                topLeft.y = ptnTopLeft->getValueAtTime(time, DimIdx(1)) + offset.y + center.y;

                topRight.x = ptnTopRight->getValueAtTime(time, DimIdx(0)) + offset.x + center.x;
                topRight.y = ptnTopRight->getValueAtTime(time, DimIdx(1)) + offset.y + center.y;

                btmRight.x = ptnBtmRight->getValueAtTime(time, DimIdx(0)) + offset.x + center.x;
                btmRight.y = ptnBtmRight->getValueAtTime(time, DimIdx(1)) + offset.y + center.y;

                btmLeft.x = ptnBtmLeft->getValueAtTime(time, DimIdx(0)) + offset.x + center.x;
                btmLeft.y = ptnBtmLeft->getValueAtTime(time, DimIdx(1)) + offset.y + center.y;

                //const double searchLeft   = searchWndBtmLeft->getValueAtTime(time, 0) + offset.x + center.x;
                //const double searchRight  = searchWndTopRight->getValueAtTime(time, 0) + offset.x + center.x;
                //const double searchBottom = searchWndBtmLeft->getValueAtTime(time, 1) + offset.y + center.y;
                //const double searchTop    = searchWndTopRight->getValueAtTime(time, 1) + offset.y + center.y;

                const RectI& texRect = it2->second->getBounds();
                if (texRect.height() <= 0) {
                    continue;
                }
                double par = texRect.width() / (double)texRect.height();
                RectD textureRectCanonical;

                textureRectCanonical.x2 = overlay->toCanonicalCoordinates( QPointF(TO_DPIX(SELECTED_MARKER_KEYFRAME_WIDTH_SCREEN_PX) + xOffsetPixels, 0.) ).x();
                textureRectCanonical.x1 = overlay->toCanonicalCoordinates( QPointF(xOffsetPixels, 0.) ).x();
                textureRectCanonical.y2 = viewerTopLeftCanonical.y();
                double height = textureRectCanonical.width() / par;
                textureRectCanonical.y1 = textureRectCanonical.y2 - height;


                RectD canonicalSearchWindow;
                texRect.toCanonical_noClipping(0, 1., &canonicalSearchWindow);

                //Remove any offset to the center to see the marker in the magnification window
                double xCenterPercent = (center.x - canonicalSearchWindow.x1 + offset.x) / (canonicalSearchWindow.x2 - canonicalSearchWindow.x1);
                double yCenterPercent = (center.y - canonicalSearchWindow.y1 + offset.y) / (canonicalSearchWindow.y2 - canonicalSearchWindow.y1);
                Point centerPointCanonical;
                centerPointCanonical.y = textureRectCanonical.y1 + yCenterPercent * (textureRectCanonical.y2 - textureRectCanonical.y1);
                centerPointCanonical.x = textureRectCanonical.x1 + xCenterPercent * (textureRectCanonical.x2 - textureRectCanonical.x1);


                Point innerTopLeft = toMagWindowPoint(topLeft, canonicalSearchWindow, textureRectCanonical);
                Point innerTopRight = toMagWindowPoint(topRight, canonicalSearchWindow, textureRectCanonical);
                Point innerBtmLeft = toMagWindowPoint(btmLeft, canonicalSearchWindow, textureRectCanonical);
                Point innerBtmRight = toMagWindowPoint(btmRight, canonicalSearchWindow, textureRectCanonical);

                //Map texture
                GL_GPU::Color4f(1., 1., 1., 1.);
                GL_GPU::Enable(GL_TEXTURE_2D);
                GL_GPU::BindTexture( GL_TEXTURE_2D, it2->second->getTexID() );
                GL_GPU::Begin(GL_POLYGON);
                GL_GPU::TexCoord2d(0, 0); GL_GPU::Vertex2d(textureRectCanonical.x1, textureRectCanonical.y1);
                GL_GPU::TexCoord2d(0, 1); GL_GPU::Vertex2d(textureRectCanonical.x1, textureRectCanonical.y2);
                GL_GPU::TexCoord2d(1, 1); GL_GPU::Vertex2d(textureRectCanonical.x2, textureRectCanonical.y2);
                GL_GPU::TexCoord2d(1, 0); GL_GPU::Vertex2d(textureRectCanonical.x2, textureRectCanonical.y1);
                GL_GPU::End();

                GL_GPU::BindTexture(GL_TEXTURE_2D, 0);

                QPointF textPos = overlay->toCanonicalCoordinates( QPointF(xOffsetPixels + 5, fontHeight + 5 ) );
                overlay->renderText(textPos.x(), textPos.y(), marker->getLabel(), overlayColor[0], overlayColor[1], overlayColor[2], overlayColor[3]);

                QPointF framePos = overlay->toCanonicalCoordinates( QPointF( xOffsetPixels + 5, overlay->toWidgetCoordinates( QPointF(textureRectCanonical.x1, textureRectCanonical.y1) ).y() ) );
                QString frameText = tr("Frame");
                frameText.append( QString::fromUtf8(" ") + QString::number(it2->first) );
                overlay->renderText(framePos.x(), framePos.y(), frameText.toStdString(), overlayColor[0], overlayColor[1], overlayColor[2], overlayColor[3]);

                //Draw contour
                GL_GPU::Enable(GL_LINE_SMOOTH);
                GL_GPU::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
                GL_GPU::Enable(GL_BLEND);
                GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                if (time == currentTime) {
                    GL_GPU::Color4f(0.93, 0.54, 0, 1);
                } else {
                    KeyFrameTexIDs::const_iterator next = it2;
                    ++next;
                    KeyFrameTexIDs::const_iterator prev = it2;
                    if ( prev != keysToRender.begin() ) {
                        --prev;
                    } else {
                        prev = keysToRender.end();
                    }
                    if ( ( next == keysToRender.end() ) && (time < currentTime) ) {
                        //Beyond the last keyframe
                        GL_GPU::Color4f(0.93, 0.54, 0, 1);
                    } else if ( ( prev == keysToRender.end() ) && (time > currentTime) ) {
                        //Before the first keyframe
                        GL_GPU::Color4f(0.93, 0.54, 0, 1);
                    } else {
                        if (time < currentTime) {
                            assert( next != keysToRender.end() );
                            if (next->first > currentTime) {
                                GL_GPU::Color4f(1, 0.75, 0.47, 1);
                            } else {
                                GL_GPU::Color4f(1., 1., 1., 0.5);
                            }
                        } else {
                            //time > currentTime
                            assert( prev != keysToRender.end() );
                            if (prev->first < currentTime) {
                                GL_GPU::Color4f(1, 0.75, 0.47, 1);
                            } else {
                                GL_GPU::Color4f(1., 1., 1., 0.5);
                            }
                        }
                    }
                }

                GL_GPU::LineWidth(1.5);
                glCheckError(GL_GPU);
                GL_GPU::Begin(GL_LINE_LOOP);
                GL_GPU::Vertex2d(textureRectCanonical.x1, textureRectCanonical.y1);
                GL_GPU::Vertex2d(textureRectCanonical.x1, textureRectCanonical.y2);
                GL_GPU::Vertex2d(textureRectCanonical.x2, textureRectCanonical.y2);
                GL_GPU::Vertex2d(textureRectCanonical.x2, textureRectCanonical.y1);
                GL_GPU::End();

                glCheckError(GL_GPU);


                //Draw internal marker
                for (int l = 0; l < 2; ++l) {
                    // shadow (uses GL_PROJECTION)
                    GL_GPU::MatrixMode(GL_PROJECTION);
                    int direction = (l == 0) ? 1 : -1;
                    // translate (1,-1) pixels
                    GL_GPU::Translated(direction * pixelScale.first / 256, -direction * pixelScale.second / 256, 0);
                    GL_GPU::MatrixMode(GL_MODELVIEW);

                    GL_GPU::Color4f(0.8 * l, 0.8 * l, 0.8 * l, 1);

                    GL_GPU::Begin(GL_LINE_LOOP);
                    GL_GPU::Vertex2d(innerTopLeft.x, innerTopLeft.y);
                    GL_GPU::Vertex2d(innerTopRight.x, innerTopRight.y);
                    GL_GPU::Vertex2d(innerBtmRight.x, innerBtmRight.y);
                    GL_GPU::Vertex2d(innerBtmLeft.x, innerBtmLeft.y);
                    GL_GPU::End();

                    GL_GPU::Begin(GL_POINTS);
                    GL_GPU::Vertex2d(centerPointCanonical.x, centerPointCanonical.y);
                    GL_GPU::End();
                }

                xOffsetPixels += TO_DPIX(SELECTED_MARKER_KEYFRAME_WIDTH_SCREEN_PX);
            }
            break;
        }
    }
} // TrackerNodeInteract::drawSelectedMarkerKeyframes

void
TrackerNodeInteract::drawSelectedMarkerTexture(const std::pair<double, double>& pixelScale,
                                               TimeValue currentTime,
                                               const Point& ptnCenter,
                                               const Point& offset,
                                               const Point& ptnTopLeft,
                                               const Point& ptnTopRight,
                                               const Point& ptnBtmRight,
                                               const Point& ptnBtmLeft,
                                               const Point& /*selectedSearchWndBtmLeft*/,
                                               const Point& /*selectedSearchWndTopRight*/)
{
    TrackMarkerPtr marker = selectedMarker.lock();
    OverlaySupport* overlay = getLastCallingViewport();

    assert(overlay);


    if ( isTracking || !selectedMarkerTexture || !marker || !marker->isEnabled(currentTime)) {
        return;
    }

    // If a viewer is doing playback, do not refresh the marker texture
    {
        std::list<ViewerNodePtr> viewers;
        _imp->publicInterface->getApp()->getAllViewers(&viewers);
        for (std::list<ViewerNodePtr>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
            if ((*it)->getNode()->getRenderEngine()->isDoingSequentialRender()) {
                return;
            }
        }
    }

    RectD textureRectCanonical;
    computeSelectedMarkerCanonicalRect(&textureRectCanonical);


    double overlayColor[4];
    if ( !_imp->publicInterface->getNode()->getOverlayColor(&overlayColor[0], &overlayColor[1], &overlayColor[2]) ) {
        overlayColor[0] = overlayColor[1] = overlayColor[2] = 0.8;
    }
    overlayColor[3] = 1.;

    const RectI& texRect = selectedMarkerTexture->getBounds();
    RectD texCoords;
    /*texCoords.x1 = (texRect.x1 - selectedMarkerTextureRoI.x1) / (double)selectedMarkerTextureRoI.width();
       texCoords.y1 = (texRect.y1 - selectedMarkerTextureRoI.y1) / (double)selectedMarkerTextureRoI.height();
       if (texRect.x2 <=  selectedMarkerTextureRoI.x2) {
       texCoords.x2 = (texRect.x2 - selectedMarkerTextureRoI.x1) / (double)selectedMarkerTextureRoI.width();
       } else {
       texCoords.x2 = 1.;
       }
       if (texRect.y2 <=  selectedMarkerTextureRoI.y2) {
       texCoords.y2 = (texRect.y2 - selectedMarkerTextureRoI.y1) / (double)selectedMarkerTextureRoI.height();
       } else {
       texCoords.y2 = 1.;
       }*/
    texCoords.x1 = texCoords.y1 = 0.;
    texCoords.x2 = texCoords.y2 = 1.;

    RectD canonicalSearchWindow;
    texRect.toCanonical_noClipping(0, 1., &canonicalSearchWindow);

    Point centerPoint, innerTopLeft, innerTopRight, innerBtmLeft, innerBtmRight;

    //Remove any offset to the center to see the marker in the magnification window
    double xCenterPercent = (ptnCenter.x - canonicalSearchWindow.x1 + offset.x) / (canonicalSearchWindow.x2 - canonicalSearchWindow.x1);
    double yCenterPercent = (ptnCenter.y - canonicalSearchWindow.y1 + offset.y) / (canonicalSearchWindow.y2 - canonicalSearchWindow.y1);
    centerPoint.y = textureRectCanonical.y1 + yCenterPercent * (textureRectCanonical.y2 - textureRectCanonical.y1);
    centerPoint.x = textureRectCanonical.x1 + xCenterPercent * (textureRectCanonical.x2 - textureRectCanonical.x1);


    innerTopLeft = toMagWindowPoint(ptnTopLeft, canonicalSearchWindow, textureRectCanonical);
    innerTopRight = toMagWindowPoint(ptnTopRight, canonicalSearchWindow, textureRectCanonical);
    innerBtmLeft = toMagWindowPoint(ptnBtmLeft, canonicalSearchWindow, textureRectCanonical);
    innerBtmRight = toMagWindowPoint(ptnBtmRight, canonicalSearchWindow, textureRectCanonical);

    Transform::Point3D btmLeftTex, topLeftTex, topRightTex, btmRightTex;
    btmLeftTex.z = topLeftTex.z = topRightTex.z = btmRightTex.z = 1.;
    btmLeftTex.x = texCoords.x1; btmLeftTex.y = texCoords.y1;
    topLeftTex.x = texCoords.x1; topLeftTex.y = texCoords.y2;
    topRightTex.x = texCoords.x2; topRightTex.y = texCoords.y2;
    btmRightTex.x = texCoords.x2; btmRightTex.y = texCoords.y1;
    Transform::Matrix3x3 m = Transform::matTransformCanonical(0, 0, selectedMarkerScale.x, selectedMarkerScale.y, 0, 0, false, 0, xCenterPercent, yCenterPercent);
    btmLeftTex = Transform::matApply(m, btmLeftTex);
    topLeftTex = Transform::matApply(m, topLeftTex);
    btmRightTex = Transform::matApply(m, btmRightTex);
    topRightTex = Transform::matApply(m, topRightTex);

    //Map texture
    GL_GPU::Color4f(1., 1., 1., 1.);
    GL_GPU::Enable(GL_TEXTURE_2D);
    GL_GPU::BindTexture( GL_TEXTURE_2D, selectedMarkerTexture->getTexID() );
    GL_GPU::Begin(GL_POLYGON);
    GL_GPU::TexCoord2d(btmLeftTex.x, btmRightTex.y); GL_GPU::Vertex2d(textureRectCanonical.x1, textureRectCanonical.y1);
    GL_GPU::TexCoord2d(topLeftTex.x, topLeftTex.y); GL_GPU::Vertex2d(textureRectCanonical.x1, textureRectCanonical.y2);
    GL_GPU::TexCoord2d(topRightTex.x, topRightTex.y); GL_GPU::Vertex2d(textureRectCanonical.x2, textureRectCanonical.y2);
    GL_GPU::TexCoord2d(btmRightTex.x, btmRightTex.y); GL_GPU::Vertex2d(textureRectCanonical.x2, textureRectCanonical.y1);
    GL_GPU::End();

    GL_GPU::BindTexture(GL_TEXTURE_2D, 0);

    //Draw contour
    GL_GPU::Enable(GL_LINE_SMOOTH);
    GL_GPU::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    GL_GPU::Enable(GL_BLEND);
    GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GL_GPU::Color4f(1., 1., 1., 0.5);
    GL_GPU::LineWidth(1.5);
    glCheckError(GL_GPU);
    GL_GPU::Begin(GL_LINE_LOOP);
    GL_GPU::Vertex2d(textureRectCanonical.x1, textureRectCanonical.y1);
    GL_GPU::Vertex2d(textureRectCanonical.x1, textureRectCanonical.y2);
    GL_GPU::Vertex2d(textureRectCanonical.x2, textureRectCanonical.y2);
    GL_GPU::Vertex2d(textureRectCanonical.x2, textureRectCanonical.y1);
    GL_GPU::End();

    GL_GPU::Color4f(0.8, 0.8, 0.8, 1.);
    GL_GPU::PointSize(POINT_SIZE);
    GL_GPU::Begin(GL_POINTS);
    GL_GPU::Vertex2d(textureRectCanonical.x2, textureRectCanonical.y1);
    GL_GPU::End();
    glCheckError(GL_GPU);

    int fontHeight = overlay->getWidgetFontHeight();
    QPointF textPos = overlay->toCanonicalCoordinates( QPointF(5, fontHeight + 5) );
    overlay->renderText(textPos.x(), textPos.y(), marker->getLabel(), overlayColor[0], overlayColor[1], overlayColor[2], overlayColor[3]);

    //Draw internal marker

    for (int l = 0; l < 2; ++l) {
        // shadow (uses GL_PROJECTION)
        GL_GPU::MatrixMode(GL_PROJECTION);
        int direction = (l == 0) ? 1 : -1;
        // translate (1,-1) pixels
        GL_GPU::Translated(direction * pixelScale.first / 256, -direction * pixelScale.second / 256, 0);
        GL_GPU::MatrixMode(GL_MODELVIEW);

        GL_GPU::Color4f(0.8 * l, 0.8 * l, 0.8 * l, 1);

        GL_GPU::Begin(GL_LINE_LOOP);
        GL_GPU::Vertex2d(innerTopLeft.x, innerTopLeft.y);
        GL_GPU::Vertex2d(innerTopRight.x, innerTopRight.y);
        GL_GPU::Vertex2d(innerBtmRight.x, innerBtmRight.y);
        GL_GPU::Vertex2d(innerBtmLeft.x, innerBtmLeft.y);
        GL_GPU::End();

        GL_GPU::Begin(GL_POINTS);
        GL_GPU::Vertex2d(centerPoint.x, centerPoint.y);
        GL_GPU::End();

        ///Draw ellipse if scaling
        if ( (eventState == eMouseStateScalingSelectedMarker) || (hoverState == eDrawStateShowScalingHint) ) {
            double ellipseColor[3];
            if (eventState == eMouseStateScalingSelectedMarker) {
                ellipseColor[0] = 0.8;
                ellipseColor[1] = 0.8;
                ellipseColor[2] = 0.;
            } else {
                ellipseColor[0] = 0.8;
                ellipseColor[1] = 0.8;
                ellipseColor[2] = 0.8;
            }
            double rx = std::sqrt( (lastMousePos.x() - centerPoint.x) * (lastMousePos.x() - centerPoint.x) + (lastMousePos.y() - centerPoint.y) * (lastMousePos.y() - centerPoint.y) );
            double ry = rx;
            drawEllipse(centerPoint.x, centerPoint.y, rx, ry, l, ellipseColor[0], ellipseColor[1], ellipseColor[2], 1.);
        }
    }

    ///Now draw keyframes
    drawSelectedMarkerKeyframes(pixelScale, currentTime);
} // TrackerNodeInteract::drawSelectedMarkerTexture

bool
TrackerNodeInteract::isNearbyPoint(const KnobDoublePtr& knob,
                                   double xWidget,
                                   double yWidget,
                                   double toleranceWidget,
                                   TimeValue time)
{
    QPointF p;
    OverlaySupport* overlay = getLastCallingViewport();

    assert(overlay);

    p.rx() = knob->getValueAtTime(time, DimIdx(0));
    p.ry() = knob->getValueAtTime(time, DimIdx(1));
    p = overlay->toWidgetCoordinates(p);
    if ( ( p.x() <= (xWidget + toleranceWidget) ) && ( p.x() >= (xWidget - toleranceWidget) ) &&
         ( p.y() <= (yWidget + toleranceWidget) ) && ( p.y() >= (yWidget - toleranceWidget) ) ) {
        return true;
    }

    return false;
}

bool
TrackerNodeInteract::isNearbyPoint(const QPointF& p,
                                   double xWidget,
                                   double yWidget,
                                   double toleranceWidget)
{
    OverlaySupport* overlay = getLastCallingViewport();

    assert(overlay);


    QPointF pw = overlay->toWidgetCoordinates(p);

    if ( ( pw.x() <= (xWidget + toleranceWidget) ) && ( pw.x() >= (xWidget - toleranceWidget) ) &&
         ( pw.y() <= (yWidget + toleranceWidget) ) && ( pw.y() >= (yWidget - toleranceWidget) ) ) {
        return true;
    }

    return false;
}

void
TrackerNodeInteract::findLineIntersection(const Point& p,
                                          const Point& l1,
                                          const Point& l2,
                                          Point* inter)
{
    Point h, u;
    double a;

    h.x = p.x - l1.x;
    h.y = p.y - l1.y;

    u.x = l2.x - l1.x;
    u.y = l2.y - l1.y;

    a = (u.x * h.x + u.y * h.y) / (u.x * u.x + u.y * u.y);
    inter->x = l1.x + u.x * a;
    inter->y = l1.y + u.y * a;
}

void
TrackerNodeInteract::onRequestRefreshSelectedMarkerReceived()
{
    if (!nSelectedMarkerTextureRefreshRequests) {
        return;
    }
    nSelectedMarkerTextureRefreshRequests = 0;

    refreshSelectedMarkerTextureNow();
}

void
TrackerNodeInteract::refreshSelectedMarkerTextureLater()
{
    ++nSelectedMarkerTextureRefreshRequests;
    Q_EMIT requestRefreshSelectedMarkerTexture();
}

void
TrackerNodeInteract::refreshSelectedMarkerTextureNow()
{


    assert( QThread::currentThread() == qApp->thread() );
    if (isTracking) {
        return;
    }
    TrackMarkerPtr marker = selectedMarker.lock();
    if (!marker) {
        return;
    }

    TimeValue time = _imp->publicInterface->getTimelineCurrentTime();
    RectD roi = marker->getMarkerImageRoI(time);
    if ( roi.isNull() ) {
        return;
    }

    selectedMarkerImg.reset();

    imageGetterWatcher = boost::make_shared<TrackWatcher>();
    QObject::connect( imageGetterWatcher.get(), SIGNAL(finished()), this, SLOT(onTrackImageRenderingFinished()) );
    imageGetterWatcher->setFuture( QtConcurrent::run(marker.get(), &TrackMarker::getMarkerImage, time, roi) );
}

void
TrackerNodeInteract::makeMarkerKeyTexture(TimeValue time,
                                          const TrackMarkerPtr& track)
{
    assert( QThread::currentThread() == qApp->thread() );
    TrackRequestKey k;
    k.time = time;
    k.track = track;
    k.roi = track->getMarkerImageRoI(time);

    TrackKeysMap::iterator foundTrack = trackTextures.find(track);
    if ( foundTrack != trackTextures.end() ) {
        KeyFrameTexIDs::iterator foundKey = foundTrack->second.find(k.time);
        if ( foundKey != foundTrack->second.end() ) {
            const RectI& texRect = foundKey->second->getBounds();
            if ( (k.roi.x1 == texRect.x1) &&
                 ( k.roi.x2 == texRect.x2) &&
                 ( k.roi.y1 == texRect.y1) &&
                 ( k.roi.y2 == texRect.y2) ) {
                return;
            }
        }
    }

    if ( !k.roi.isNull() ) {
        TrackWatcherPtr watcher = boost::make_shared<TrackWatcher>();
        QObject::connect( watcher.get(), SIGNAL(finished()), this, SLOT(onKeyFrameImageRenderingFinished()) );
        trackRequestsMap[k] = watcher;
        watcher->setFuture( QtConcurrent::run(track.get(), &TrackMarker::getMarkerImage, time, k.roi) );
    }
}

inline
unsigned int
toBGRA(unsigned char r,
       unsigned char g,
       unsigned char b,
       unsigned char a)
{
    return (a << 24) | (r << 16) | (g << 8) | b;
}


template <int srcNComps>
void
fullRectFloatCPUImageToPBOForNComps(const Image::CPUData& srcImage,
                                    const RectI& roi,
                                    unsigned int* buf)
{


    // Always display the selected marker texture in sRGB
    const Color::Lut* lut = Color::LutManager::sRGBLut();
    lut->validate();
    assert(lut);

    const unsigned char alpha = 255;


    for (int y = roi.y1; y < roi.y2; ++y) {

        const int start_x = (int)( rand() % (roi.width()) ) + roi.x1;

        for (int backward = 0; backward < 2; ++backward) {


            int x = backward ? std::max(roi.x1 - 1, start_x - 1) : start_x;
            const int end_x = backward ? roi.x1 - 1 : roi.x2;
            assert( backward == 1 || ( x >= 0 && x < roi.x2 ) );

            const float* srcPixels[4] = {NULL, NULL, NULL, NULL};
            int srcPixelStride;
            Image::getChannelPointers<float, srcNComps>((const float**)srcImage.ptrs, x, y, srcImage.bounds, (float**)srcPixels, &srcPixelStride);

            unsigned int* dstPixels = buf + (y - roi.y1) * roi.width() + (x - roi.x1);

            unsigned error[3] = {0x80, 0x80, 0x80};

            while (x != end_x) {

                float tmpPix[3] = {0.f, 0.f, 0.f};
                for (int c = 0; c < srcNComps; ++c) {
                    if (srcPixels[c]) {
                        tmpPix[c] = *srcPixels[c];
                        error[c] = (error[c] & 0xff) + lut->toColorSpaceUint8xxFromLinearFloatFast(tmpPix[c]);
                        assert(error[c] < 0x10000);
                    }
                }

                *dstPixels = toBGRA( (U8)(error[0] >> 8), (U8)(error[1] >> 8), (U8)(error[2] >> 8), alpha );
                if (backward) {
                    --x;
                    --dstPixels;
                    for (int c = 0; c < srcNComps; ++c) {
                        srcPixels[c] -= srcPixelStride;
                    }
                } else {
                    ++x;
                    ++dstPixels;
                    for (int c = 0; c < srcNComps; ++c) {
                        srcPixels[c] += srcPixelStride;
                    }
                }
            } // for each pixels along the line
        } // backward ?
    } // for each scan-line

} // fullRectFloatCPUImageToPBO


static void
fullRectFloatCPUImageToPBO(const Image::CPUData& srcImage,
                           const RectI& roi,
                           unsigned int* dstBuf)
{
    switch (srcImage.nComps) {
        case 1:
            fullRectFloatCPUImageToPBOForNComps<1>(srcImage, roi, dstBuf);
            break;
        case 2:
            fullRectFloatCPUImageToPBOForNComps<2>(srcImage, roi, dstBuf);
            break;
        case 3:
            fullRectFloatCPUImageToPBOForNComps<3>(srcImage, roi, dstBuf);
            break;
        case 4:
            fullRectFloatCPUImageToPBOForNComps<4>(srcImage, roi, dstBuf);
            break;
        default:
            break;
    }
}

void
TrackerNodeInteract::convertImageTosRGBOpenGLTexture(const ImagePtr& image,
                                                     const boost::shared_ptr<Texture>& tex,
                                                     const RectI& renderWindow)
{
    RectI bounds;
    RectI roi;

    if (image) {
        bounds = image->getBounds();
        renderWindow.intersect(bounds, &roi);
    } else {
        bounds = renderWindow;
        roi = bounds;
    }
    if ( roi.isNull() ) {
        return;
    }

    // The image must have the appropriate format: it has been converted in TrackMarker::getMarkerImage
    assert(image->getBitDepth() == eImageBitDepthFloat && image->getBufferFormat() == eImageBufferLayoutRGBAPackedFullRect && image->getStorageMode() != eStorageModeGLTex);
    Image::CPUData imageData;
    image->getCPUData(&imageData);


    std::size_t bytesCount = 4 * sizeof(unsigned char) * roi.area();

    GLint currentBoundPBO = 0;
    GL_GPU::GetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING_ARB, &currentBoundPBO);

    if (pboID == 0) {
        GL_GPU::GenBuffers(1, &pboID);
    }

    // bind PBO to update texture source
    GL_GPU::BindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, pboID );

    // Note that glMapBufferARB() causes sync issue.
    // If GPU is working with this buffer, glMapBufferARB() will wait(stall)
    // until GPU to finish its job. To avoid waiting (idle), you can call
    // first glBufferDataARB() with NULL pointer before glMapBufferARB().
    // If you do that, the previous data in PBO will be discarded and
    // glMapBufferARB() returns a new allocated pointer immediately
    // even if GPU is still working with the previous data.
    GL_GPU::BufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, bytesCount, NULL, GL_DYNAMIC_DRAW_ARB);

    // map the buffer object into client's memory
    GLvoid *buf = GL_GPU::MapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    glCheckError(GL_GPU);
    assert(buf);
    if (buf) {
        fullRectFloatCPUImageToPBO(imageData, roi, (unsigned int*)buf);
        GLboolean result = GL_GPU::UnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release the mapped buffer
        assert(result == GL_TRUE);
        Q_UNUSED(result);
    }
    glCheckError(GL_GPU);

    // copy pixels from PBO to texture object
    // using glBindTexture followed by glTexSubImage2D.
    // Use offset instead of pointer (last parameter is 0).
    tex->fillOrAllocateTexture(roi, 0, 0);

    // restore previously bound PBO
    GL_GPU::BindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, currentBoundPBO);

    glCheckError(GL_GPU);
} // TrackerNodeInteract::convertImageTosRGBOpenGLTexture

void
TrackerNodeInteract::onTrackingStarted(int step)
{
    isTracking = true;
    if (step > 0) {
        trackFwButton.lock()->setValue(true, ViewIdx(0), DimIdx(0), eValueChangedReasonPluginEdited);
    } else {
        trackBwButton.lock()->setValue(true, ViewIdx(0), DimIdx(0), eValueChangedReasonPluginEdited);
    }
}

void
TrackerNodeInteract::onTrackingEnded()
{
    _imp->solveTransformParams();
    trackBwButton.lock()->setValue(false, ViewIdx(0), DimIdx(0), eValueChangedReasonPluginEdited);
    trackFwButton.lock()->setValue(false, ViewIdx(0), DimIdx(0), eValueChangedReasonPluginEdited);
    isTracking = false;
    redraw();
}

void
TrackerNodeInteract::onModelSelectionChanged(const std::list<KnobTableItemPtr>& /*addedToSelection*/, const std::list<KnobTableItemPtr>& /*removedFromSelection*/, TableChangeReasonEnum reason)
{
    std::list<TrackMarkerPtr> selection;

    _imp->knobsTable->getSelectedMarkers(&selection);
    if ( selection.empty() || (selection.size() > 1) ) {
        showMarkerTexture = false;
    } else {
        assert(selection.size() == 1);

        const TrackMarkerPtr& selectionFront = selection.front();
        TrackMarkerPtr oldMarker = selectedMarker.lock();
        if (oldMarker != selectionFront) {
            selectedMarker = selectionFront;
            refreshSelectedMarkerTextureLater();


            KeyFrameSet keys = selectionFront->getKeyFrames();

            for (KeyFrameSet::iterator it2 = keys.begin(); it2 != keys.end(); ++it2) {
                makeMarkerKeyTexture(it2->getTime(), selectionFront);
            }
        } else {
            if (selectionFront) {
                showMarkerTexture = true;
            }
        }
    }
    if ( reason == eTableChangeReasonViewer ) {
        return;
    }

    redraw();
}

void
TrackerNodeInteract::onTrackImageRenderingFinished()
{
    assert( QThread::currentThread() == qApp->thread() );
    TrackWatcher* future = dynamic_cast<TrackWatcher*>( sender() );
    assert(future);
    std::pair<ImagePtr, RectD> ret = future->result();
    OverlaySupport* overlay = getLastCallingViewport();
    assert(overlay);
    OpenGLViewerI* isOpenGLViewer = dynamic_cast<OpenGLViewerI*>(overlay);
    assert(isOpenGLViewer);
    if (!isOpenGLViewer) {
        return;
    }
    if (!ret.first) {
        return;
    }
    OSGLContextPtr viewerGLContext = isOpenGLViewer->getOpenGLViewerContext();
    OSGLContextAttacherPtr locker = OSGLContextAttacher::create(viewerGLContext);
    locker->attach();

    showMarkerTexture = true;
    if (!selectedMarkerTexture) {
        int format, internalFormat, glType;
        Texture::getRecommendedTexParametersForRGBAByteTexture(&format, &internalFormat, &glType);
        selectedMarkerTexture = boost::make_shared<Texture>(GL_TEXTURE_2D, GL_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE, eImageBitDepthByte,
                                                 format, internalFormat, glType, true /*useGL*/);
    }

    RectI roi;
    ret.second.toPixelEnclosing(0, 1., &roi);
    convertImageTosRGBOpenGLTexture(ret.first, selectedMarkerTexture, roi);

    redraw();
}

void
TrackerNodeInteract::onKeyFrameImageRenderingFinished()
{
    assert( QThread::currentThread() == qApp->thread() );
    TrackWatcher* future = dynamic_cast<TrackWatcher*>( sender() );
    assert(future);
    std::pair<ImagePtr, RectD> ret = future->result();
    if ( !ret.first || ret.second.isNull() ) {
        return;
    }

    OverlaySupport* overlay = getLastCallingViewport();
    assert(overlay);
    OpenGLViewerI* isOpenGLViewer = dynamic_cast<OpenGLViewerI*>(overlay);
    assert(isOpenGLViewer);
    if (!isOpenGLViewer) {
        return;
    }

    OSGLContextPtr viewerGLContext = isOpenGLViewer->getOpenGLViewerContext();
    OSGLContextAttacherPtr locker = OSGLContextAttacher::create(viewerGLContext);
    locker->attach();

    for (TrackKeyframeRequests::iterator it = trackRequestsMap.begin(); it != trackRequestsMap.end(); ++it) {
        if (it->second.get() == future) {
            TrackMarkerPtr track = it->first.track.lock();
            if (!track) {
                return;
            }
            TrackerNodeInteract::KeyFrameTexIDs& keyTextures = trackTextures[track];
            int format, internalFormat, glType;
            Texture::getRecommendedTexParametersForRGBAByteTexture(&format, &internalFormat, &glType);
            GLTexturePtr tex = boost::make_shared<Texture>(GL_TEXTURE_2D, GL_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE, eImageBitDepthByte,
                                          format, internalFormat, glType, true);
            keyTextures[it->first.time] = tex;

            RectI roi;
            ret.second.toPixelEnclosing(0, 1., &roi);
            convertImageTosRGBOpenGLTexture(ret.first, tex, roi);

            trackRequestsMap.erase(it);

            redraw();

            return;
        }
    }
    assert(false);
}

void
TrackerNodeInteract::rebuildMarkerTextures()
{
    ///Refreh textures for all markers
    std::list<TrackMarkerPtr> markers;

    _imp->knobsTable->getSelectedMarkers(&markers);
    for (std::list<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
        KeyFrameSet keys = (*it)->getKeyFrames();
        for (KeyFrameSet::iterator it2 = keys.begin(); it2 != keys.end(); ++it2) {
            makeMarkerKeyTexture(it2->getTime(), *it);
        }
    }
    onModelSelectionChanged(std::list<KnobTableItemPtr>(),std::list<KnobTableItemPtr>(), eTableChangeReasonInternal);
}

/**
 *@brief Moves of the given pixel the selected tracks.
 * This takes into account the zoom factor.
 **/
bool
TrackerNodeInteract::nudgeSelectedTracks(int x,
                                         int y)
{
    if (!isInsideSelectedMarkerTexture(lastMousePos)) {
        return false;
    }
    std::list<TrackMarkerPtr> markers;

    _imp->knobsTable->getSelectedMarkers(&markers);

    if ( !markers.empty() ) {
        std::pair<double, double> pixelScale;

        getPixelScale(pixelScale.first, pixelScale.second);
        TimeValue time = _imp->publicInterface->getTimelineCurrentTime();
        bool createkey = createKeyOnMoveButton.lock()->getValue();

        int hasMovedMarker = false;
        for (std::list<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {

            if (!(*it)->isEnabled(time)) {
                continue;
            }
            KnobDoublePtr centerKnob = (*it)->getCenterKnob();
            KnobDoublePtr patternCorners[4];
            patternCorners[0] = (*it)->getPatternBtmLeftKnob();
            patternCorners[1] = (*it)->getPatternTopLeftKnob();
            patternCorners[2] = (*it)->getPatternTopRightKnob();
            patternCorners[3] = (*it)->getPatternBtmRightKnob();

            {
                std::vector<double> values(2);
                values[0] = centerKnob->getValueAtTime(time) + x;
                values[1] = centerKnob->getValueAtTime(time, DimIdx(1)) + y;
                centerKnob->setValueAtTimeAcrossDimensions(time, values);
            }
            for (int i = 0; i < 4; ++i) {
                std::vector<double> values(2);
                values[0] = patternCorners[i]->getValueAtTime(time, DimIdx(0));
                values[1] = patternCorners[i]->getValueAtTime(time, DimIdx(1));
                patternCorners[i]->setValueAcrossDimensions(values);
            }
            if (createkey) {
                (*it)->setKeyFrame(time);
            }
            hasMovedMarker = true;
        }
        refreshSelectedMarkerTextureLater();

        return hasMovedMarker;
    }

    return false;
}

void
TrackerNodeInteract::transformPattern(TimeValue time,
                                      TrackerMouseStateEnum state,
                                      const Point& delta)
{
    KnobDoublePtr searchWndTopRight, searchWndBtmLeft;
    KnobDoublePtr patternCorners[4];
    KnobDoublePtr centerKnob = interactMarker->getCenterKnob();
    KnobDoublePtr offsetKnob = interactMarker->getOffsetKnob();
    bool transformPatternCorners = state != eMouseStateDraggingOuterBtmLeft &&
                                   state != eMouseStateDraggingOuterBtmRight &&
                                   state != eMouseStateDraggingOuterTopLeft &&
                                   state != eMouseStateDraggingOuterTopRight &&
                                   state != eMouseStateDraggingOuterMidLeft &&
                                   state != eMouseStateDraggingOuterMidRight &&
                                   state != eMouseStateDraggingOuterTopMid &&
                                   state != eMouseStateDraggingOuterBtmMid;

    if (transformPatternCorners) {
        patternCorners[0] = interactMarker->getPatternTopLeftKnob();
        patternCorners[1] = interactMarker->getPatternBtmLeftKnob();
        patternCorners[2] = interactMarker->getPatternBtmRightKnob();
        patternCorners[3] = interactMarker->getPatternTopRightKnob();
    }
    searchWndTopRight = interactMarker->getSearchWindowTopRightKnob();
    searchWndBtmLeft = interactMarker->getSearchWindowBottomLeftKnob();

    QPointF centerPoint;
    centerPoint.rx() = centerKnob->getValueAtTime(time, DimIdx(0));
    centerPoint.ry() = centerKnob->getValueAtTime(time, DimIdx(1));

    QPointF offset;
    offset.rx() = offsetKnob->getValueAtTime(time, DimIdx(0));
    offset.ry() = offsetKnob->getValueAtTime(time, DimIdx(1));

    QPointF patternPoints[4];
    QPointF searchPoints[4];
    if (transformPatternCorners) {
        for (int i = 0; i < 4; ++i) {
            patternPoints[i].rx() = patternCorners[i]->getValueAtTime(time, DimIdx(0)) + centerPoint.x() + offset.x();
            patternPoints[i].ry() = patternCorners[i]->getValueAtTime(time, DimIdx(1)) + centerPoint.y() + offset.y();
        }
    }
    searchPoints[1].rx() = searchWndBtmLeft->getValueAtTime(time, DimIdx(0)) + centerPoint.x() + offset.x();
    searchPoints[1].ry() = searchWndBtmLeft->getValueAtTime(time, DimIdx(1)) + centerPoint.y() + offset.y();

    searchPoints[3].rx() = searchWndTopRight->getValueAtTime(time, DimIdx(0)) + centerPoint.x() + offset.x();
    searchPoints[3].ry() = searchWndTopRight->getValueAtTime(time, DimIdx(1)) + centerPoint.y() + offset.y();

    searchPoints[0].rx() = searchPoints[1].x();
    searchPoints[0].ry() = searchPoints[3].y();

    searchPoints[2].rx() = searchPoints[3].x();
    searchPoints[2].ry() = searchPoints[1].y();

    if ( (state == eMouseStateDraggingInnerBtmLeft) ||
         ( state == eMouseStateDraggingOuterBtmLeft) ) {
        if (transformPatternCorners) {
            patternPoints[1].rx() += delta.x;
            patternPoints[1].ry() += delta.y;

            if (controlDown == 0) {
                patternPoints[0].rx() += delta.x;
                patternPoints[0].ry() -= delta.y;

                patternPoints[2].rx() -= delta.x;
                patternPoints[2].ry() += delta.y;

                patternPoints[3].rx() -= delta.x;
                patternPoints[3].ry() -= delta.y;
            }
        }

        searchPoints[1].rx() += delta.x;
        searchPoints[1].ry() += delta.y;

        if (controlDown == 0) {
            searchPoints[0].rx() += delta.x;
            searchPoints[0].ry() -= delta.y;

            searchPoints[2].rx() -= delta.x;
            searchPoints[2].ry() += delta.y;

            searchPoints[3].rx() -= delta.x;
            searchPoints[3].ry() -= delta.y;
        }
    } else if ( (state == eMouseStateDraggingInnerBtmRight) ||
               ( state == eMouseStateDraggingOuterBtmRight) ) {
        if (transformPatternCorners) {


            patternPoints[2].rx() += delta.x;
            patternPoints[2].ry() += delta.y;

            if (controlDown == 0) {
                patternPoints[1].rx() -= delta.x;
                patternPoints[1].ry() += delta.y;

                patternPoints[0].rx() -= delta.x;
                patternPoints[0].ry() -= delta.y;

                patternPoints[3].rx() += delta.x;
                patternPoints[3].ry() -= delta.y;
            }
        }


        searchPoints[2].rx() += delta.x;
        searchPoints[2].ry() += delta.y;

        if (controlDown == 0) {
            searchPoints[1].rx() -= delta.x;
            searchPoints[1].ry() += delta.y;

            searchPoints[0].rx() -= delta.x;
            searchPoints[0].ry() -= delta.y;

            searchPoints[3].rx() += delta.x;
            searchPoints[3].ry() -= delta.y;
        }
    } else if ( (state == eMouseStateDraggingInnerTopRight) ||
               ( state == eMouseStateDraggingOuterTopRight) ) {
        if (transformPatternCorners) {

            if (controlDown == 0) {
                patternPoints[1].rx() -= delta.x;
                patternPoints[1].ry() -= delta.y;

                patternPoints[0].rx() -= delta.x;
                patternPoints[0].ry() += delta.y;

                patternPoints[2].rx() += delta.x;
                patternPoints[2].ry() -= delta.y;
            }

            patternPoints[3].rx() += delta.x;
            patternPoints[3].ry() += delta.y;
        }

        if (controlDown == 0) {
            searchPoints[1].rx() -= delta.x;
            searchPoints[1].ry() -= delta.y;

            searchPoints[0].rx() -= delta.x;
            searchPoints[0].ry() += delta.y;

            searchPoints[2].rx() += delta.x;
            searchPoints[2].ry() -= delta.y;
        }
        
        searchPoints[3].rx() += delta.x;
        searchPoints[3].ry() += delta.y;
    } else if ( (state == eMouseStateDraggingInnerTopLeft) ||
               ( state == eMouseStateDraggingOuterTopLeft) ) {
        if (transformPatternCorners) {
            patternPoints[0].rx() += delta.x;
            patternPoints[0].ry() += delta.y;

            if (controlDown == 0) {
                patternPoints[1].rx() += delta.x;
                patternPoints[1].ry() -= delta.y;

                patternPoints[2].rx() -= delta.x;
                patternPoints[2].ry() -= delta.y;

                patternPoints[3].rx() -= delta.x;
                patternPoints[3].ry() += delta.y;
            }
        }

        searchPoints[0].rx() += delta.x;
        searchPoints[0].ry() += delta.y;


        if (controlDown == 0) {
            searchPoints[1].rx() += delta.x;
            searchPoints[1].ry() -= delta.y;

            searchPoints[2].rx() -= delta.x;
            searchPoints[2].ry() -= delta.y;

            searchPoints[3].rx() -= delta.x;
            searchPoints[3].ry() += delta.y;
        }
    } else if ( (state == eMouseStateDraggingInnerBtmMid) ||
               ( state == eMouseStateDraggingOuterBtmMid) ) {
        if (transformPatternCorners) {
            patternPoints[1].ry() += delta.y;
            patternPoints[2].ry() += delta.y;
            if (controlDown == 0) {
                patternPoints[0].ry() -= delta.y;
                patternPoints[3].ry() -= delta.y;
            }
        }
        searchPoints[1].ry() += delta.y;
        searchPoints[2].ry() += delta.y;
        if (controlDown == 0) {
            searchPoints[0].ry() -= delta.y;
            searchPoints[3].ry() -= delta.y;
        }
    } else if ( (state == eMouseStateDraggingInnerTopMid) ||
                ( state == eMouseStateDraggingOuterTopMid) ) {
        if (transformPatternCorners) {
            if (controlDown == 0) {
                patternPoints[1].ry() -= delta.y;
                patternPoints[2].ry() -= delta.y;
            }
            patternPoints[0].ry() += delta.y;
            patternPoints[3].ry() += delta.y;
        }
        if (controlDown == 0) {
            searchPoints[1].ry() -= delta.y;
            searchPoints[2].ry() -= delta.y;
        }
        searchPoints[0].ry() += delta.y;
        searchPoints[3].ry() += delta.y;
    } else if ( (state == eMouseStateDraggingInnerMidLeft) ||
                ( state == eMouseStateDraggingOuterMidLeft) ) {
        if (transformPatternCorners) {
            patternPoints[0].rx() += delta.x;
            patternPoints[1].rx() += delta.x;
            if (controlDown == 0) {
                patternPoints[2].rx() -= delta.x;
                patternPoints[3].rx() -= delta.x;
            }
        }
        searchPoints[0].rx() += delta.x;
        searchPoints[1].rx() += delta.x;
        if (controlDown == 0) {
            searchPoints[2].rx() -= delta.x;
            searchPoints[3].rx() -= delta.x;
        }
    } else if ( (state == eMouseStateDraggingInnerMidRight) ||
                ( state == eMouseStateDraggingOuterMidRight) ) {
        if (transformPatternCorners) {
            if (controlDown == 0) {
                patternPoints[0].rx() -= delta.x;
                patternPoints[1].rx() -= delta.x;
            }
            patternPoints[2].rx() += delta.x;
            patternPoints[3].rx() += delta.x;
        }
        if (controlDown == 0) {
            searchPoints[0].rx() -= delta.x;
            searchPoints[1].rx() -= delta.x;
        }
        searchPoints[2].rx() += delta.x;
        searchPoints[3].rx() += delta.x;
    }
    {
        ScopedChanges_RAII changes(_imp->publicInterface);

        if (transformPatternCorners) {
            for (int i = 0; i < 4; ++i) {
                patternPoints[i].rx() -= ( centerPoint.x() + offset.x() );
                patternPoints[i].ry() -= ( centerPoint.y() + offset.y() );

                std::vector<double> values(2);
                values[0] = patternPoints[i].x();
                values[1] = patternPoints[i].y();

                if ( patternCorners[i]->hasAnimation() ) {
                    patternCorners[i]->setValueAcrossDimensions(values);
                } else {
                    patternCorners[i]->setValueAcrossDimensions(values);
                }
            }
        }
        searchPoints[1].rx() -= ( centerPoint.x() + offset.x() );
        searchPoints[1].ry() -= ( centerPoint.y() + offset.y() );

        searchPoints[3].rx() -= ( centerPoint.x() + offset.x() );
        searchPoints[3].ry() -= ( centerPoint.y() + offset.y() );

        {
            std::vector<double> values(2);
            values[0] = searchPoints[1].x();
            values[1] = searchPoints[1].y();
            if ( searchWndBtmLeft->hasAnimation() ) {
                searchWndBtmLeft->setValueAcrossDimensions(values);
            } else {
                searchWndBtmLeft->setValueAcrossDimensions(values);
            }
        }

        {
            std::vector<double> values(2);
            values[0] = searchPoints[3].x();
            values[1] = searchPoints[3].y();
            if ( searchWndTopRight->hasAnimation() ) {
                searchWndTopRight->setValueAcrossDimensions(values);
            } else {
                searchWndTopRight->setValueAcrossDimensions(values);
            }
        }
    }   // changes
    refreshSelectedMarkerTextureLater();
    
    if ( createKeyOnMoveButton.lock()->getValue() ) {
        interactMarker->setKeyFrame(time);
    }
} // TrackerNodeInteract::transformPattern

void
TrackerNodeInteract::onKeyframeSetOnTrack(TimeValue time)
{
    TrackMarker* marker = (TrackMarker*)sender();
    if (!marker) {
        return;
    }
    makeMarkerKeyTexture(time, marker->shared_from_this());
}

void
TrackerNodeInteract::onKeyframeRemovedOnTrack(TimeValue time)
{
    TrackMarker* marker = (TrackMarker*)sender();
    if (!marker) {
        return;
    }
    for (TrackerNodeInteract::TrackKeysMap::iterator it = trackTextures.begin(); it != trackTextures.end(); ++it) {
        if (it->first.lock().get() == marker) {
            std::map<TimeValue, boost::shared_ptr<Texture> >::iterator found = it->second.find(time);
            if ( found != it->second.end() ) {
                it->second.erase(found);
            }
            break;
        }
    }
    redraw();
}

void
TrackerNodeInteract::onKeyframeMovedOnTrack(TimeValue from, TimeValue to)
{
    TrackMarker* marker = (TrackMarker*)sender();
    if (!marker) {
        return;
    }
    for (TrackerNodeInteract::TrackKeysMap::iterator it = trackTextures.begin(); it != trackTextures.end(); ++it) {
        if (it->first.lock().get() == marker) {
            std::map<TimeValue, boost::shared_ptr<Texture> >::iterator found = it->second.find(from);
            if (found != it->second.end()) {
                it->second[to] = found->second;
                it->second.erase(found);
            }
            break;
        }
    }
    redraw();
}


struct CenterPointDisplayInfo
{
    double x;
    double y;
    double err;
    bool isValid;

    CenterPointDisplayInfo()
    : x(0)
    , y(0)
    , err(0)
    , isValid(false)
    {

    }
};

typedef std::map<double, CenterPointDisplayInfo> CenterPointsMap;

void
TrackerNodeInteract::drawOverlay(TimeValue time,
                         const RenderScale & /*renderScale*/,
                         ViewIdx /*view*/)
{
    double pixelScaleX, pixelScaleY;
    OverlaySupport* overlay = getLastCallingViewport();

    assert(overlay);
    overlay->getPixelScale(pixelScaleX, pixelScaleY);
    double viewportSize[2];
    overlay->getViewportSize(viewportSize[0], viewportSize[1]);

    {
        GLProtectAttrib<GL_GPU> a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_POINT_BIT | GL_ENABLE_BIT | GL_HINT_BIT | GL_TRANSFORM_BIT);
        double markerColor[4];
        if ( !getOverlayColor(markerColor[0], markerColor[1], markerColor[2]) ) {
            markerColor[0] = markerColor[1] = markerColor[2] = 0.8;
        }
        markerColor[3] = 1.;

        std::vector<TrackMarkerPtr> allMarkers;
        std::list<TrackMarkerPtr> selectedMarkers;
        _imp->knobsTable->getSelectedMarkers(&selectedMarkers);
        _imp->knobsTable->getAllMarkers(&allMarkers);

        bool trackingPageSecret = _imp->trackingPageKnob.lock()->getIsSecret();
        bool showErrorColor = showCorrelationButton.lock()->getValue();
        TrackMarkerPtr marker = selectedMarker.lock();
        bool selectedFound = false;
        Point selectedCenter;
        Point selectedPtnTopLeft;
        Point selectedPtnTopRight;
        Point selectedPtnBtmRight;
        Point selectedPtnBtmLeft;
        Point selectedOffset;
        Point selectedSearchBtmLeft;
        Point selectedSearchTopRight;

        for (std::vector<TrackMarkerPtr>::iterator it = allMarkers.begin(); it != allMarkers.end(); ++it) {
            bool isEnabled = (*it)->isEnabled(time);

            double thisMarkerColor[3];
            if (!isEnabled) {
                for (int i = 0; i < 3; ++i) {
                    thisMarkerColor[i] = markerColor[i] / 2.;
                }
            } else {
                for (int i = 0; i < 3; ++i) {
                    thisMarkerColor[i] = markerColor[i];
                }
            }

            bool isHoverMarker = *it == hoverMarker;
            bool isDraggedMarker = *it == interactMarker;
            bool isHoverOrDraggedMarker = isHoverMarker || isDraggedMarker;
            std::list<TrackMarkerPtr>::iterator foundSelected = std::find(selectedMarkers.begin(), selectedMarkers.end(), *it);
            bool isSelected = foundSelected != selectedMarkers.end();
            KnobDoublePtr centerKnob = (*it)->getCenterKnob();
            KnobDoublePtr offsetKnob = (*it)->getOffsetKnob();
            KnobDoublePtr errorKnob = (*it)->getErrorKnob();
            KnobDoublePtr ptnTopLeft = (*it)->getPatternTopLeftKnob();
            KnobDoublePtr ptnTopRight = (*it)->getPatternTopRightKnob();
            KnobDoublePtr ptnBtmRight = (*it)->getPatternBtmRightKnob();
            KnobDoublePtr ptnBtmLeft = (*it)->getPatternBtmLeftKnob();
            KnobDoublePtr searchWndBtmLeft = (*it)->getSearchWindowBottomLeftKnob();
            KnobDoublePtr searchWndTopRight = (*it)->getSearchWindowTopRightKnob();

            // When the tracking page is secret, still show markers, but as if deselected
            if (!isSelected || trackingPageSecret) {
                ///Draw a custom interact, indicating the track isn't selected
                GL_GPU::Enable(GL_LINE_SMOOTH);
                GL_GPU::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
                GL_GPU::LineWidth(1.5f);

                for (int l = 0; l < 2; ++l) {
                    // shadow (uses GL_PROJECTION)
                    GL_GPU::MatrixMode(GL_PROJECTION);
                    int direction = (l == 0) ? 1 : -1;
                    // translate (1,-1) pixels
                    GL_GPU::Translated(direction * pixelScaleX / 256, -direction * pixelScaleY / 256, 0);
                    GL_GPU::MatrixMode(GL_MODELVIEW);

                    if (l == 0) {
                        GL_GPU::Color4d(0., 0., 0., 1.);
                    } else {
                        GL_GPU::Color4f(thisMarkerColor[0], thisMarkerColor[1], thisMarkerColor[2], 1.);
                    }


                    double x = centerKnob->getValueAtTime(time, DimIdx(0));
                    double y = centerKnob->getValueAtTime(time, DimIdx(1));

                    GL_GPU::PointSize(POINT_SIZE);
                    GL_GPU::Begin(GL_POINTS);
                    GL_GPU::Vertex2d(x, y);
                    GL_GPU::End();

                    GL_GPU::Begin(GL_LINES);
                    GL_GPU::Vertex2d(x - CROSS_SIZE * pixelScaleX, y);
                    GL_GPU::Vertex2d(x + CROSS_SIZE * pixelScaleX, y);


                    GL_GPU::Vertex2d(x, y - CROSS_SIZE * pixelScaleY);
                    GL_GPU::Vertex2d(x, y + CROSS_SIZE * pixelScaleY);
                    GL_GPU::End();
                }
                GL_GPU::PointSize(1.);
            } else { // if (isSelected) {
                GL_GPU::Enable(GL_LINE_SMOOTH);
                GL_GPU::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
                GLdouble projection[16];
                GL_GPU::GetDoublev( GL_PROJECTION_MATRIX, projection);
                OfxPointD shadow; // how much to translate GL_PROJECTION to get exactly one pixel on screen
                shadow.x = 2. / (projection[0] * viewportSize[0]);
                shadow.y = 2. / (projection[5] * viewportSize[1]);

                const QPointF center( centerKnob->getValueAtTime(time, DimIdx(0)),
                                     centerKnob->getValueAtTime(time, DimIdx(1)) );
                const QPointF offset( offsetKnob->getValueAtTime(time, DimIdx(0)),
                                     offsetKnob->getValueAtTime(time, DimIdx(1)) );
                const QPointF topLeft( ptnTopLeft->getValueAtTime(time, DimIdx(0)) + offset.x() + center.x(),
                                      ptnTopLeft->getValueAtTime(time, DimIdx(1)) + offset.y() + center.y() );
                const QPointF topRight( ptnTopRight->getValueAtTime(time, DimIdx(0)) + offset.x() + center.x(),
                                       ptnTopRight->getValueAtTime(time, DimIdx(1)) + offset.y() + center.y() );
                const QPointF btmRight( ptnBtmRight->getValueAtTime(time, DimIdx(0)) + offset.x() + center.x(),
                                       ptnBtmRight->getValueAtTime(time, DimIdx(1)) + offset.y() + center.y() );
                const QPointF btmLeft( ptnBtmLeft->getValueAtTime(time, DimIdx(0)) + offset.x() + center.x(),
                                      ptnBtmLeft->getValueAtTime(time, DimIdx(1)) + offset.y() + center.y() );
                const double searchLeft   = searchWndBtmLeft->getValueAtTime(time, DimIdx(0))  + offset.x() + center.x();
                const double searchBottom = searchWndBtmLeft->getValueAtTime(time, DimIdx(1))  + offset.y() + center.y();
                const double searchRight  = searchWndTopRight->getValueAtTime(time, DimIdx(0)) + offset.x() + center.x();
                const double searchTop    = searchWndTopRight->getValueAtTime(time, DimIdx(1)) + offset.y() + center.y();
                const double searchMidX   = (searchLeft + searchRight) / 2;
                const double searchMidY   = (searchTop + searchBottom) / 2;

                if (marker == *it) {
                    selectedCenter.x = center.x();
                    selectedCenter.y = center.y();
                    selectedOffset.x = offset.x();
                    selectedOffset.y = offset.y();
                    selectedPtnBtmLeft.x = btmLeft.x();
                    selectedPtnBtmLeft.y = btmLeft.y();
                    selectedPtnBtmRight.x = btmRight.x();
                    selectedPtnBtmRight.y = btmRight.y();
                    selectedPtnTopRight.x = topRight.x();
                    selectedPtnTopRight.y = topRight.y();
                    selectedPtnTopLeft.x = topLeft.x();
                    selectedPtnTopLeft.y = topLeft.y();
                    selectedSearchBtmLeft.x = searchLeft;
                    selectedSearchBtmLeft.y = searchBottom;

                    selectedSearchTopRight.x = searchRight;
                    selectedSearchTopRight.y = searchTop;
                    selectedFound = true;
                }

                const QPointF innerMidLeft( (btmLeft + topLeft) / 2 );
                const QPointF innerMidTop( (topLeft + topRight) / 2 );
                const QPointF innerMidRight( (btmRight + topRight) / 2 );
                const QPointF innerMidBtm( (btmLeft + btmRight) / 2 );
                const QPointF outterMidLeft(searchLeft, searchMidY);
                const QPointF outterMidTop(searchMidX, searchTop);
                const QPointF outterMidRight(searchRight, searchMidY);
                const QPointF outterMidBtm(searchMidX, searchBottom);
                const QPointF handleSize( HANDLE_SIZE * pixelScaleX, HANDLE_SIZE * pixelScaleY );
                const QPointF innerMidLeftExt = TrackerNodeInteract::computeMidPointExtent(topLeft, btmLeft, innerMidLeft, handleSize);
                const QPointF innerMidRightExt = TrackerNodeInteract::computeMidPointExtent(btmRight, topRight, innerMidRight, handleSize);
                const QPointF innerMidTopExt = TrackerNodeInteract::computeMidPointExtent(topRight, topLeft, innerMidTop, handleSize);
                const QPointF innerMidBtmExt = TrackerNodeInteract::computeMidPointExtent(btmLeft, btmRight, innerMidBtm, handleSize);
                const QPointF searchTopLeft(searchLeft, searchTop);
                const QPointF searchTopRight(searchRight, searchTop);
                const QPointF searchBtmRight(searchRight, searchBottom);
                const QPointF searchBtmLeft(searchLeft, searchBottom);
                const QPointF searchTopMid(searchMidX, searchTop);
                const QPointF searchRightMid(searchRight, searchMidY);
                const QPointF searchLeftMid(searchLeft, searchMidY);
                const QPointF searchBtmMid(searchMidX, searchBottom);
                const QPointF outterMidLeftExt  = TrackerNodeInteract::computeMidPointExtent(searchTopLeft,  searchBtmLeft,  outterMidLeft,  handleSize);
                const QPointF outterMidRightExt = TrackerNodeInteract::computeMidPointExtent(searchBtmRight, searchTopRight, outterMidRight, handleSize);
                const QPointF outterMidTopExt   = TrackerNodeInteract::computeMidPointExtent(searchTopRight, searchTopLeft,  outterMidTop,   handleSize);
                const QPointF outterMidBtmExt   = TrackerNodeInteract::computeMidPointExtent(searchBtmLeft,  searchBtmRight, outterMidBtm,   handleSize);
                std::string name = (*it)->getLabel();
                if (!isEnabled) {
                    name += ' ';
                    name += tr("(disabled)").toStdString();
                }
                CenterPointsMap centerPoints;
                CurvePtr xCurve = centerKnob->getAnimationCurve(ViewIdx(0), DimIdx(0));
                CurvePtr yCurve = centerKnob->getAnimationCurve(ViewIdx(0), DimIdx(1));
                CurvePtr errorCurve = errorKnob->getAnimationCurve(ViewIdx(0), DimIdx(0));

                {
                    KeyFrameSet xKeyframes = xCurve->getKeyFrames_mt_safe();
                    KeyFrameSet yKeyframes = yCurve->getKeyFrames_mt_safe();
                    KeyFrameSet errKeyframes;
                    if (showErrorColor) {
                        errKeyframes = errorCurve->getKeyFrames_mt_safe();
                    }

                    // Try first to do an optimized case in O(N) where we assume that all 3 curves have the same keyframes
                    // at the same time
                    KeyFrameSet remainingXKeys,remainingYKeys, remainingErrKeys;
                    if (xKeyframes.size() == yKeyframes.size() && (!showErrorColor || xKeyframes.size() == errKeyframes.size())) {
                        KeyFrameSet::iterator errIt = errKeyframes.begin();
                        KeyFrameSet::iterator xIt = xKeyframes.begin();
                        KeyFrameSet::iterator yIt = yKeyframes.begin();

                        bool setsHaveDifferentKeyTimes = false;
                        while (xIt!=xKeyframes.end()) {
                            if (xIt->getTime() != yIt->getTime() || (showErrorColor && xIt->getTime() != errIt->getTime())) {
                                setsHaveDifferentKeyTimes = true;
                                break;
                            }
                            CenterPointDisplayInfo& p = centerPoints[xIt->getTime()];
                            p.x = xIt->getValue();
                            p.y = yIt->getValue();
                            if ( showErrorColor ) {
                                p.err = errIt->getValue();
                            }
                            p.isValid = true;

                            ++xIt;
                            ++yIt;
                            if (showErrorColor) {
                                ++errIt;
                            }

                        }
                        if (setsHaveDifferentKeyTimes) {
                            remainingXKeys.insert(xIt, xKeyframes.end());
                            remainingYKeys.insert(yIt, yKeyframes.end());
                            if (showErrorColor) {
                                remainingErrKeys.insert(errIt, errKeyframes.end());
                            }
                        }
                    } else {
                        remainingXKeys = xKeyframes;
                        remainingYKeys = yKeyframes;
                        if (showErrorColor) {
                            remainingErrKeys = errKeyframes;
                        }
                    }
                    for (KeyFrameSet::iterator xIt = remainingXKeys.begin(); xIt != remainingXKeys.end(); ++xIt) {
                        CenterPointDisplayInfo& p = centerPoints[xIt->getTime()];
                        p.x = xIt->getValue();
                        p.isValid = false;
                    }
                    for (KeyFrameSet::iterator yIt = remainingYKeys.begin(); yIt != remainingYKeys.end(); ++yIt) {
                        CenterPointsMap::iterator foundPoint = centerPoints.find(yIt->getTime());
                        if (foundPoint == centerPoints.end()) {
                            continue;
                        }
                        foundPoint->second.y = yIt->getValue();
                        if (!showErrorColor) {
                            foundPoint->second.isValid = true;
                        }
                    }
                    for (KeyFrameSet::iterator errIt = remainingErrKeys.begin(); errIt != remainingErrKeys.end(); ++errIt) {
                        CenterPointsMap::iterator foundPoint = centerPoints.find(errIt->getTime());
                        if (foundPoint == centerPoints.end()) {
                            continue;
                        }
                        foundPoint->second.err = errIt->getValue();
                        foundPoint->second.isValid = true;
                    }

                }



                for (int l = 0; l < 2; ++l) {
                    // shadow (uses GL_PROJECTION)
                    GL_GPU::MatrixMode(GL_PROJECTION);
                    int direction = (l == 0) ? 1 : -1;
                    // translate (1,-1) pixels
                    GL_GPU::Translated(direction * shadow.x, -direction * shadow.y, 0);
                    GL_GPU::MatrixMode(GL_MODELVIEW);

                    ///Draw center position

                    GL_GPU::Enable(GL_LINE_SMOOTH);
                    GL_GPU::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
                    GL_GPU::Begin(GL_LINE_STRIP);
                    GL_GPU::Color3f(0.5 * l, 0.5 * l, 0.5 * l);
                    for (CenterPointsMap::iterator it = centerPoints.begin(); it != centerPoints.end(); ++it) {
                        if (it->second.isValid) {
                            GL_GPU::Vertex2d(it->second.x, it->second.y);
                        }
                    }
                    GL_GPU::End();
                    GL_GPU::Disable(GL_LINE_SMOOTH);

                    GL_GPU::Enable(GL_POINT_SMOOTH);
                    GL_GPU::Begin(GL_POINTS);
                    if (!showErrorColor) {
                        GL_GPU::Color3f(0.5 * l, 0.5 * l, 0.5 * l);
                    }

                    for (CenterPointsMap::iterator it2 = centerPoints.begin(); it2 != centerPoints.end(); ++it2) {
                        if (!it2->second.isValid) {
                            continue;
                        }
                        if (showErrorColor) {
                            if (l != 0) {
                                /*
                                 Clamp the correlation to [CORRELATION_ERROR_MIN, 1] then
                                 Map the correlation to [0, 0.33] since 0 is Red for HSV and 0.33 is Green.
                                 Also clamp to the interval if the correlation is higher, and reverse.
                                 */

                                double error = boost::algorithm::clamp(it2->second.err, 0., CORRELATION_ERROR_MAX_DISPLAY);
                                double mappedError = 0.33 - 0.33 * error / CORRELATION_ERROR_MAX_DISPLAY;
                                float r, g, b;
                                Color::hsv_to_rgb(mappedError, 1, 1, &r, &g, &b);
                                GL_GPU::Color3f(r, g, b);
                            } else {
                                GL_GPU::Color3f(0., 0., 0.);
                            }
                        }
                        GL_GPU::Vertex2d(it2->second.x, it2->second.y);
                    }
                    GL_GPU::End();
                    GL_GPU::Disable(GL_POINT_SMOOTH);


                    GL_GPU::Color3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    GL_GPU::Begin(GL_LINE_LOOP);
                    GL_GPU::Vertex2d( topLeft.x(), topLeft.y() );
                    GL_GPU::Vertex2d( topRight.x(), topRight.y() );
                    GL_GPU::Vertex2d( btmRight.x(), btmRight.y() );
                    GL_GPU::Vertex2d( btmLeft.x(), btmLeft.y() );
                    GL_GPU::End();

                    GL_GPU::Begin(GL_LINE_LOOP);
                    GL_GPU::Vertex2d( searchTopLeft.x(), searchTopLeft.y() );
                    GL_GPU::Vertex2d( searchTopRight.x(), searchTopRight.y() );
                    GL_GPU::Vertex2d( searchBtmRight.x(), searchBtmRight.y() );
                    GL_GPU::Vertex2d( searchBtmLeft.x(), searchBtmRight.y() );
                    GL_GPU::End();

                    GL_GPU::PointSize(POINT_SIZE);
                    GL_GPU::Begin(GL_POINTS);

                    ///draw center
                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringCenter) || (eventState == eMouseStateDraggingCenter) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                    } else {
                        GL_GPU::Color3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    }
                    GL_GPU::Vertex2d( center.x(), center.y() );

                    if ( (offset.x() != 0) || (offset.y() != 0) ) {
                        GL_GPU::Vertex2d( center.x() + offset.x(), center.y() + offset.y() );
                    }

                    //////DRAWING INNER POINTS
                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringInnerBtmLeft) || (eventState == eMouseStateDraggingInnerBtmLeft) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                        GL_GPU::Vertex2d( btmLeft.x(), btmLeft.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringInnerBtmMid) || (eventState == eMouseStateDraggingInnerBtmMid) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                        GL_GPU::Vertex2d( innerMidBtm.x(), innerMidBtm.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringInnerBtmRight) || (eventState == eMouseStateDraggingInnerBtmRight) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                        GL_GPU::Vertex2d( btmRight.x(), btmRight.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringInnerMidLeft) || (eventState == eMouseStateDraggingInnerMidLeft) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                        GL_GPU::Vertex2d( innerMidLeft.x(), innerMidLeft.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringInnerMidRight) || (eventState == eMouseStateDraggingInnerMidRight) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                        GL_GPU::Vertex2d( innerMidRight.x(), innerMidRight.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringInnerTopLeft) || (eventState == eMouseStateDraggingInnerTopLeft) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                        GL_GPU::Vertex2d( topLeft.x(), topLeft.y() );
                    }

                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringInnerTopMid) || (eventState == eMouseStateDraggingInnerTopMid) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                        GL_GPU::Vertex2d( innerMidTop.x(), innerMidTop.y() );
                    }

                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringInnerTopRight) || (eventState == eMouseStateDraggingInnerTopRight) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                        GL_GPU::Vertex2d( topRight.x(), topRight.y() );
                    }


                    //////DRAWING OUTTER POINTS

                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringOuterBtmLeft) || (eventState == eMouseStateDraggingOuterBtmLeft) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                        GL_GPU::Vertex2d( searchBtmLeft.x(), searchBtmLeft.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringOuterBtmMid) || (eventState == eMouseStateDraggingOuterBtmMid) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                        GL_GPU::Vertex2d( outterMidBtm.x(), outterMidBtm.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringOuterBtmRight) || (eventState == eMouseStateDraggingOuterBtmRight) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                        GL_GPU::Vertex2d( searchBtmRight.x(), searchBtmRight.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringOuterMidLeft) || (eventState == eMouseStateDraggingOuterMidLeft) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                        GL_GPU::Vertex2d( outterMidLeft.x(), outterMidLeft.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringOuterMidRight) || (eventState == eMouseStateDraggingOuterMidRight) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                        GL_GPU::Vertex2d( outterMidRight.x(), outterMidRight.y() );
                    }

                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringOuterTopLeft) || (eventState == eMouseStateDraggingOuterTopLeft) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                        GL_GPU::Vertex2d( searchTopLeft.x(), searchTopLeft.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringOuterTopMid) || (eventState == eMouseStateDraggingOuterTopMid) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                        GL_GPU::Vertex2d( outterMidTop.x(), outterMidTop.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringOuterTopRight) || (eventState == eMouseStateDraggingOuterTopRight) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                        GL_GPU::Vertex2d( searchTopRight.x(), searchTopRight.y() );
                    }

                    GL_GPU::End();

                    if ( (offset.x() != 0) || (offset.y() != 0) ) {
                        GL_GPU::Begin(GL_LINES);
                        GL_GPU::Color3f( (float)thisMarkerColor[0] * l * 0.5, (float)thisMarkerColor[1] * l * 0.5, (float)thisMarkerColor[2] * l * 0.5 );
                        GL_GPU::Vertex2d( center.x(), center.y() );
                        GL_GPU::Vertex2d( center.x() + offset.x(), center.y() + offset.y() );
                        GL_GPU::End();
                    }

                    ///now show small lines at handle positions
                    GL_GPU::Begin(GL_LINES);

                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringInnerMidLeft) || (eventState == eMouseStateDraggingInnerMidLeft) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                    } else {
                        GL_GPU::Color3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    }
                    GL_GPU::Vertex2d( innerMidLeft.x(), innerMidLeft.y() );
                    GL_GPU::Vertex2d( innerMidLeftExt.x(), innerMidLeftExt.y() );

                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringInnerTopMid) || (eventState == eMouseStateDraggingInnerTopMid) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                    } else {
                        GL_GPU::Color3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    }
                    GL_GPU::Vertex2d( innerMidTop.x(), innerMidTop.y() );
                    GL_GPU::Vertex2d( innerMidTopExt.x(), innerMidTopExt.y() );

                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringInnerMidRight) || (eventState == eMouseStateDraggingInnerMidRight) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                    } else {
                        GL_GPU::Color3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    }
                    GL_GPU::Vertex2d( innerMidRight.x(), innerMidRight.y() );
                    GL_GPU::Vertex2d( innerMidRightExt.x(), innerMidRightExt.y() );

                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringInnerBtmMid) || (eventState == eMouseStateDraggingInnerBtmMid) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                    } else {
                        GL_GPU::Color3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    }
                    GL_GPU::Vertex2d( innerMidBtm.x(), innerMidBtm.y() );
                    GL_GPU::Vertex2d( innerMidBtmExt.x(), innerMidBtmExt.y() );

                    //////DRAWING OUTTER HANDLES

                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringOuterMidLeft) || (eventState == eMouseStateDraggingOuterMidLeft) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                    } else {
                        GL_GPU::Color3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    }
                    GL_GPU::Vertex2d( outterMidLeft.x(), outterMidLeft.y() );
                    GL_GPU::Vertex2d( outterMidLeftExt.x(), outterMidLeftExt.y() );

                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringOuterTopMid) || (eventState == eMouseStateDraggingOuterTopMid) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                    } else {
                        GL_GPU::Color3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    }
                    GL_GPU::Vertex2d( outterMidTop.x(), outterMidTop.y() );
                    GL_GPU::Vertex2d( outterMidTopExt.x(), outterMidTopExt.y() );

                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringOuterMidRight) || (eventState == eMouseStateDraggingOuterMidRight) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                    } else {
                        GL_GPU::Color3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    }
                    GL_GPU::Vertex2d( outterMidRight.x(), outterMidRight.y() );
                    GL_GPU::Vertex2d( outterMidRightExt.x(), outterMidRightExt.y() );

                    if ( isHoverOrDraggedMarker && ( (hoverState == eDrawStateHoveringOuterBtmMid) || (eventState == eMouseStateDraggingOuterBtmMid) ) ) {
                        GL_GPU::Color3f(0.f * l, 1.f * l, 0.f * l);
                    } else {
                        GL_GPU::Color3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    }
                    GL_GPU::Vertex2d( outterMidBtm.x(), outterMidBtm.y() );
                    GL_GPU::Vertex2d( outterMidBtmExt.x(), outterMidBtmExt.y() );
                    GL_GPU::End();

                    GL_GPU::Color3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );

                    overlay->renderText( center.x(), center.y(), name, markerColor[0], markerColor[1], markerColor[2], markerColor[3]);
                } // for (int l = 0; l < 2; ++l) {
            } // if (!isSelected) {
        } // for (std::vector<TrackMarkerPtr>::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {

        if (showMarkerTexture && selectedFound && !_imp->publicInterface->getApp()->isDraftRenderEnabled()) {
            drawSelectedMarkerTexture(std::make_pair(pixelScaleX, pixelScaleY), time, selectedCenter, selectedOffset, selectedPtnTopLeft, selectedPtnTopRight, selectedPtnBtmRight, selectedPtnBtmLeft, selectedSearchBtmLeft, selectedSearchTopRight);
        }
        // context->drawInternalNodesOverlay( time, renderScale, view, overlay);


        if (clickToAddTrackEnabled) {
            ///draw a square of 20px around the mouse cursor
            GL_GPU::Enable(GL_BLEND);
            GL_GPU::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            GL_GPU::Enable(GL_LINE_SMOOTH);
            GL_GPU::Hint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
            GL_GPU::LineWidth(1.5);
            //GLProtectMatrix p(GL_PROJECTION); // useless (we do two glTranslate in opposite directions)

            const double addTrackSize = TO_DPIX(ADDTRACK_SIZE);

            for (int l = 0; l < 2; ++l) {
                // shadow (uses GL_PROJECTION)
                GL_GPU::MatrixMode(GL_PROJECTION);
                int direction = (l == 0) ? 1 : -1;
                // translate (1,-1) pixels
                GL_GPU::Translated(direction * pixelScaleX / 256, -direction * pixelScaleY / 256, 0);
                GL_GPU::MatrixMode(GL_MODELVIEW);

                if (l == 0) {
                    GL_GPU::Color4d(0., 0., 0., 0.8);
                } else {
                    GL_GPU::Color4d(0., 1., 0., 0.8);
                }

                GL_GPU::Begin(GL_LINE_LOOP);
                GL_GPU::Vertex2d(lastMousePos.x() - addTrackSize * 2 * pixelScaleX, lastMousePos.y() - addTrackSize * 2 * pixelScaleY);
                GL_GPU::Vertex2d(lastMousePos.x() - addTrackSize * 2 * pixelScaleX, lastMousePos.y() + addTrackSize * 2 * pixelScaleY);
                GL_GPU::Vertex2d(lastMousePos.x() + addTrackSize * 2 * pixelScaleX, lastMousePos.y() + addTrackSize * 2 * pixelScaleY);
                GL_GPU::Vertex2d(lastMousePos.x() + addTrackSize * 2 * pixelScaleX, lastMousePos.y() - addTrackSize * 2 * pixelScaleY);
                GL_GPU::End();

                ///draw a cross at the cursor position
                GL_GPU::Begin(GL_LINES);
                GL_GPU::Vertex2d( lastMousePos.x() - addTrackSize * pixelScaleX, lastMousePos.y() );
                GL_GPU::Vertex2d( lastMousePos.x() + addTrackSize * pixelScaleX, lastMousePos.y() );
                GL_GPU::Vertex2d(lastMousePos.x(), lastMousePos.y() - addTrackSize * pixelScaleY);
                GL_GPU::Vertex2d(lastMousePos.x(), lastMousePos.y() + addTrackSize * pixelScaleY);
                GL_GPU::End();
            }
        }
    } // GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_ENABLE_BIT | GL_HINT_BIT);
} // drawOverlay

bool
TrackerNodeInteract::onOverlayPenDown(TimeValue time,
                              const RenderScale & /*renderScale*/,
                              ViewIdx /*view*/,
                              const QPointF & viewportPos,
                              const QPointF & pos,
                              double /*pressure*/,
                              TimeValue /*timestamp*/,
                              PenType /*pen*/)
{
    std::pair<double, double> pixelScale;
    OverlaySupport* overlay = getLastCallingViewport();
    assert(overlay);
    overlay->getPixelScale(pixelScale.first, pixelScale.second);
    bool didSomething = false;
    /*if ( context->onOverlayPenDownInternalNodes( time, renderScale, view, viewportPos, pos, pressure, timestamp, pen, overlay ) ) {
     return true;
     }*/
    std::vector<TrackMarkerPtr> allMarkers;
    _imp->knobsTable->getAllMarkers(&allMarkers);

    bool trackingPageSecret = _imp->trackingPageKnob.lock()->getIsSecret();
    for (std::vector<TrackMarkerPtr>::iterator it = allMarkers.begin(); it != allMarkers.end(); ++it) {
        if (!(*it)->isEnabled(time) || trackingPageSecret) {
            continue;
        }

        bool isSelected = _imp->knobsTable->isMarkerSelected( (*it) );
        KnobDoublePtr centerKnob = (*it)->getCenterKnob();
        KnobDoublePtr offsetKnob = (*it)->getOffsetKnob();
        KnobDoublePtr ptnTopLeft = (*it)->getPatternTopLeftKnob();
        KnobDoublePtr ptnTopRight = (*it)->getPatternTopRightKnob();
        KnobDoublePtr ptnBtmRight = (*it)->getPatternBtmRightKnob();
        KnobDoublePtr ptnBtmLeft = (*it)->getPatternBtmLeftKnob();
        KnobDoublePtr searchWndTopRight = (*it)->getSearchWindowTopRightKnob();
        KnobDoublePtr searchWndBtmLeft = (*it)->getSearchWindowBottomLeftKnob();
        QPointF centerPoint;
        centerPoint.rx() = centerKnob->getValueAtTime(time, DimIdx(0));
        centerPoint.ry() = centerKnob->getValueAtTime(time, DimIdx(1));

        QPointF offset;
        offset.rx() = offsetKnob->getValueAtTime(time, DimIdx(0));
        offset.ry() = offsetKnob->getValueAtTime(time, DimIdx(1));

        if ( isNearbyPoint(centerKnob, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE, time) ) {
            if (controlDown > 0) {
                eventState = eMouseStateDraggingOffset;
            } else {
                eventState = eMouseStateDraggingCenter;
            }
            interactMarker = *it;
            didSomething = true;
        } else if ( ( (offset.x() != 0) || (offset.y() != 0) ) && isNearbyPoint(QPointF( centerPoint.x() + offset.x(), centerPoint.y() + offset.y() ), viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
            eventState = eMouseStateDraggingOffset;
            interactMarker = *it;
            didSomething = true;
        }

        if (!didSomething && isSelected) {
            QPointF topLeft, topRight, btmRight, btmLeft;
            topLeft.rx() = ptnTopLeft->getValueAtTime(time, DimIdx(0)) + offset.x() + centerPoint.x();
            topLeft.ry() = ptnTopLeft->getValueAtTime(time, DimIdx(1)) + offset.y() + centerPoint.y();

            topRight.rx() = ptnTopRight->getValueAtTime(time, DimIdx(0)) + offset.x() + centerPoint.x();
            topRight.ry() = ptnTopRight->getValueAtTime(time, DimIdx(1)) + offset.y() + centerPoint.y();

            btmRight.rx() = ptnBtmRight->getValueAtTime(time, DimIdx(0)) + offset.x() + centerPoint.x();
            btmRight.ry() = ptnBtmRight->getValueAtTime(time, DimIdx(1)) + offset.y() + centerPoint.y();

            btmLeft.rx() = ptnBtmLeft->getValueAtTime(time, DimIdx(0)) + offset.x() + centerPoint.x();
            btmLeft.ry() = ptnBtmLeft->getValueAtTime(time, DimIdx(1)) + offset.y() + centerPoint.y();

            QPointF midTop, midRight, midBtm, midLeft;
            midTop.rx() = ( topLeft.x() + topRight.x() ) / 2.;
            midTop.ry() = ( topLeft.y() + topRight.y() ) / 2.;

            midRight.rx() = ( btmRight.x() + topRight.x() ) / 2.;
            midRight.ry() = ( btmRight.y() + topRight.y() ) / 2.;

            midBtm.rx() = ( btmRight.x() + btmLeft.x() ) / 2.;
            midBtm.ry() = ( btmRight.y() + btmLeft.y() ) / 2.;

            midLeft.rx() = ( topLeft.x() + btmLeft.x() ) / 2.;
            midLeft.ry() = ( topLeft.y() + btmLeft.y() ) / 2.;

            if ( isSelected && isNearbyPoint(topLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                eventState = eMouseStateDraggingInnerTopLeft;
                interactMarker = *it;
                didSomething = true;
            } else if ( isSelected && isNearbyPoint(topRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                eventState = eMouseStateDraggingInnerTopRight;
                interactMarker = *it;
                didSomething = true;
            } else if ( isSelected && isNearbyPoint(btmRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                eventState = eMouseStateDraggingInnerBtmRight;
                interactMarker = *it;
                didSomething = true;
            } else if ( isSelected && isNearbyPoint(btmLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                eventState = eMouseStateDraggingInnerBtmLeft;
                interactMarker = *it;
                didSomething = true;
            } else if ( isSelected && isNearbyPoint(midTop, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                eventState = eMouseStateDraggingInnerTopMid;
                interactMarker = *it;
                didSomething = true;
            } else if ( isSelected && isNearbyPoint(midRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                eventState = eMouseStateDraggingInnerMidRight;
                interactMarker = *it;
                didSomething = true;
            } else if ( isSelected && isNearbyPoint(midLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                eventState = eMouseStateDraggingInnerMidLeft;
                interactMarker = *it;
                didSomething = true;
            } else if ( isSelected && isNearbyPoint(midBtm, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                eventState = eMouseStateDraggingInnerBtmMid;
                interactMarker = *it;
                didSomething = true;
            }
        }

        if (!didSomething && isSelected) {
            ///Test search window
            const double searchLeft = searchWndBtmLeft->getValueAtTime(time, DimIdx(0)) + centerPoint.x() + offset.x();
            const double searchRight = searchWndTopRight->getValueAtTime(time, DimIdx(0)) + centerPoint.x() + offset.x();
            const double searchTop = searchWndTopRight->getValueAtTime(time, DimIdx(1)) + centerPoint.y() + offset.y();
            const double searchBottom = searchWndBtmLeft->getValueAtTime(time, DimIdx(1)) + +centerPoint.y() + offset.y();
            const double searchMidX = (searchLeft + searchRight) / 2.;
            const double searchMidY = (searchTop + searchBottom) / 2.;
            const QPointF searchTopLeft(searchLeft, searchTop);
            const QPointF searchTopRight(searchRight, searchTop);
            const QPointF searchBtmRight(searchRight, searchBottom);
            const QPointF searchBtmLeft(searchLeft, searchBottom);
            const QPointF searchTopMid(searchMidX, searchTop);
            const QPointF searchRightMid(searchRight, searchMidY);
            const QPointF searchLeftMid(searchLeft, searchMidY);
            const QPointF searchBtmMid(searchMidX, searchBottom);

            if ( isNearbyPoint(searchTopLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                eventState = eMouseStateDraggingOuterTopLeft;
                interactMarker = *it;
                didSomething = true;
            } else if ( isNearbyPoint(searchTopRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                eventState = eMouseStateDraggingOuterTopRight;
                interactMarker = *it;
                didSomething = true;
            } else if ( isNearbyPoint(searchBtmRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                eventState = eMouseStateDraggingOuterBtmRight;
                interactMarker = *it;
                didSomething = true;
            } else if ( isNearbyPoint(searchBtmLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                eventState = eMouseStateDraggingOuterBtmLeft;
                interactMarker = *it;
                didSomething = true;
            } else if ( isNearbyPoint(searchTopMid, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                eventState = eMouseStateDraggingOuterTopMid;
                interactMarker = *it;
                didSomething = true;
            } else if ( isNearbyPoint(searchBtmMid, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                eventState = eMouseStateDraggingOuterBtmMid;
                interactMarker = *it;
                didSomething = true;
            } else if ( isNearbyPoint(searchLeftMid, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                eventState = eMouseStateDraggingOuterMidLeft;
                interactMarker = *it;
                didSomething = true;
            } else if ( isNearbyPoint(searchRightMid, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                eventState = eMouseStateDraggingOuterMidRight;
                interactMarker = *it;
                didSomething = true;
            }
        }

        //If we hit the interact, make sure it is selected
        if (interactMarker) {
            if (!isSelected) {
                _imp->knobsTable->beginEditSelection();
                if (!shiftDown) {
                    _imp->knobsTable->clearSelection(eTableChangeReasonViewer);
                }
                _imp->knobsTable->addToSelection(interactMarker, eTableChangeReasonViewer);
                _imp->knobsTable->endEditSelection(eTableChangeReasonViewer);
            }
        }

        if (didSomething) {
            break;
        }
    } // for (std::vector<TrackMarkerPtr>::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {

    if (clickToAddTrackEnabled && !didSomething && !trackingPageSecret) {
        TrackMarkerPtr marker = _imp->createMarker();
        KnobDoublePtr centerKnob = marker->getCenterKnob();
        {
            std::vector<double> values(2);
            values[0] = pos.x();
            values[1] = pos.y();
            centerKnob->setValueAtTimeAcrossDimensions(time, values);
        }

        if ( createKeyOnMoveButton.lock()->getValue() ) {
            marker->setKeyFrame(time);
        }
        _imp->publicInterface->pushUndoCommand( new AddItemsCommand(marker) );
        refreshSelectedMarkerTextureLater();
        didSomething = true;
    }

    if ( !didSomething && showMarkerTexture && selectedMarkerTexture && isNearbySelectedMarkerTextureResizeAnchor(pos) ) {
        eventState = eMouseStateDraggingSelectedMarkerResizeAnchor;
        didSomething = true;
    }

    if ( !didSomething && showMarkerTexture && selectedMarkerTexture  && isInsideSelectedMarkerTexture(pos) ) {
        if (shiftDown) {
            eventState = eMouseStateScalingSelectedMarker;
        } else {
            eventState = eMouseStateDraggingSelectedMarker;
        }
        interactMarker = selectedMarker.lock();
        didSomething = true;
    }

    if (!didSomething) {
        int keyTime = isInsideKeyFrameTexture(time, pos, viewportPos);
        if (keyTime != INT_MAX) {

            _imp->publicInterface->getApp()->getTimeLine()->seekFrame(keyTime, true, EffectInstancePtr(), eTimelineChangeReasonOtherSeek);

            didSomething = true;
        }
    }
    if (!didSomething && !trackingPageSecret) {
        std::list<TrackMarkerPtr> selectedMarkers;
        _imp->knobsTable->getSelectedMarkers(&selectedMarkers);
        if ( !selectedMarkers.empty() ) {
            _imp->knobsTable->clearSelection(eTableChangeReasonViewer);

            didSomething = true;
        }
    }

    lastMousePos = pos;

    return didSomething;
} // penDown

bool
TrackerNodeInteract::onOverlayPenMotion(TimeValue time,
                                const RenderScale & /*renderScale*/,
                                ViewIdx view,
                                const QPointF & viewportPos,
                                const QPointF & pos,
                                double /*pressure*/,
                                TimeValue /*timestamp*/)
{
    std::pair<double, double> pixelScale;
    OverlaySupport* overlay = getLastCallingViewport();

    assert(overlay);
    overlay->getPixelScale(pixelScale.first, pixelScale.second);
    bool didSomething = false;
    Point delta;
    delta.x = pos.x() - lastMousePos.x();
    delta.y = pos.y() - lastMousePos.y();


    if (hoverState != eDrawStateInactive) {
        hoverState = eDrawStateInactive;
        hoverMarker.reset();
        didSomething = true;
    }

    std::vector<TrackMarkerPtr> allMarkers;
    _imp->knobsTable->getAllMarkers(&allMarkers);
    bool trackingPageSecret = _imp->trackingPageKnob.lock()->getIsSecret();
    bool hoverProcess = false;
    for (std::vector<TrackMarkerPtr>::iterator it = allMarkers.begin(); it != allMarkers.end(); ++it) {
        if (!(*it)->isEnabled(time) || trackingPageSecret) {
            continue;
        }

        bool isSelected = _imp->knobsTable->isMarkerSelected( (*it) );
        KnobDoublePtr centerKnob = (*it)->getCenterKnob();
        KnobDoublePtr offsetKnob = (*it)->getOffsetKnob();
        KnobDoublePtr ptnTopLeft = (*it)->getPatternTopLeftKnob();
        KnobDoublePtr ptnTopRight = (*it)->getPatternTopRightKnob();
        KnobDoublePtr ptnBtmRight = (*it)->getPatternBtmRightKnob();
        KnobDoublePtr ptnBtmLeft = (*it)->getPatternBtmLeftKnob();
        KnobDoublePtr searchWndTopRight = (*it)->getSearchWindowTopRightKnob();
        KnobDoublePtr searchWndBtmLeft = (*it)->getSearchWindowBottomLeftKnob();
        QPointF center;
        center.rx() = centerKnob->getValueAtTime(time, DimIdx(0));
        center.ry() = centerKnob->getValueAtTime(time, DimIdx(1));

        QPointF offset;
        offset.rx() = offsetKnob->getValueAtTime(time, DimIdx(0));
        offset.ry() = offsetKnob->getValueAtTime(time, DimIdx(1));
        if ( isNearbyPoint(centerKnob, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE, time) ) {
            hoverState = eDrawStateHoveringCenter;
            hoverMarker = *it;
            hoverProcess = true;
        } else if ( ( (offset.x() != 0) || (offset.y() != 0) ) && isNearbyPoint(QPointF( center.x() + offset.x(), center.y() + offset.y() ), viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
            hoverState = eDrawStateHoveringCenter;
            hoverMarker = *it;
            didSomething = true;
        }


        if (!hoverProcess) {
            QPointF topLeft, topRight, btmRight, btmLeft;
            topLeft.rx() = ptnTopLeft->getValueAtTime(time, DimIdx(0)) + offset.x() + center.x();
            topLeft.ry() = ptnTopLeft->getValueAtTime(time, DimIdx(1)) + offset.y() + center.y();

            topRight.rx() = ptnTopRight->getValueAtTime(time, DimIdx(0)) + offset.x() + center.x();
            topRight.ry() = ptnTopRight->getValueAtTime(time, DimIdx(1)) + offset.y() + center.y();

            btmRight.rx() = ptnBtmRight->getValueAtTime(time, DimIdx(0)) + offset.x() + center.x();
            btmRight.ry() = ptnBtmRight->getValueAtTime(time, DimIdx(1)) + offset.y() + center.y();

            btmLeft.rx() = ptnBtmLeft->getValueAtTime(time, DimIdx(0)) + offset.x() + center.x();
            btmLeft.ry() = ptnBtmLeft->getValueAtTime(time, DimIdx(1)) + offset.y() + center.y();

            QPointF midTop, midRight, midBtm, midLeft;
            midTop.rx() = ( topLeft.x() + topRight.x() ) / 2.;
            midTop.ry() = ( topLeft.y() + topRight.y() ) / 2.;

            midRight.rx() = ( btmRight.x() + topRight.x() ) / 2.;
            midRight.ry() = ( btmRight.y() + topRight.y() ) / 2.;

            midBtm.rx() = ( btmRight.x() + btmLeft.x() ) / 2.;
            midBtm.ry() = ( btmRight.y() + btmLeft.y() ) / 2.;

            midLeft.rx() = ( topLeft.x() + btmLeft.x() ) / 2.;
            midLeft.ry() = ( topLeft.y() + btmLeft.y() ) / 2.;


            if ( isSelected && isNearbyPoint(topLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                hoverState = eDrawStateHoveringInnerTopLeft;
                hoverMarker = *it;
                hoverProcess = true;
            } else if ( isSelected && isNearbyPoint(topRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                hoverState = eDrawStateHoveringInnerTopRight;
                hoverMarker = *it;
                hoverProcess = true;
            } else if ( isSelected && isNearbyPoint(btmRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                hoverState = eDrawStateHoveringInnerBtmRight;
                hoverMarker = *it;
                hoverProcess = true;
            } else if ( isSelected && isNearbyPoint(btmLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                hoverState = eDrawStateHoveringInnerBtmLeft;
                hoverMarker = *it;
                hoverProcess = true;
            } else if ( isSelected && isNearbyPoint(midTop, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                hoverState = eDrawStateHoveringInnerTopMid;
                hoverMarker = *it;
                hoverProcess = true;
            } else if ( isSelected && isNearbyPoint(midRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                hoverState = eDrawStateHoveringInnerMidRight;
                hoverMarker = *it;
                hoverProcess = true;
            } else if ( isSelected && isNearbyPoint(midLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                hoverState = eDrawStateHoveringInnerMidLeft;
                hoverMarker = *it;
                hoverProcess = true;
            } else if ( isSelected && isNearbyPoint(midBtm, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                hoverState = eDrawStateHoveringInnerBtmMid;
                hoverMarker = *it;
                hoverProcess = true;
            }
        }

        if (!hoverProcess && isSelected) {
            ///Test search window
            const double searchLeft   = searchWndBtmLeft->getValueAtTime(time, DimIdx(0))  + offset.x() + center.x();
            const double searchBottom = searchWndBtmLeft->getValueAtTime(time, DimIdx(1))  + offset.y() + center.y();
            const double searchRight  = searchWndTopRight->getValueAtTime(time, DimIdx(0)) + offset.x() + center.x();
            const double searchTop    = searchWndTopRight->getValueAtTime(time, DimIdx(1)) + offset.y() + center.y();
            const double searchMidX   = (searchLeft + searchRight) / 2;
            const double searchMidY   = (searchTop + searchBottom) / 2;
            const QPointF searchTopLeft(searchLeft, searchTop);
            const QPointF searchTopRight(searchRight, searchTop);
            const QPointF searchBtmRight(searchRight, searchBottom);
            const QPointF searchBtmLeft(searchLeft, searchBottom);
            const QPointF searchTopMid(searchMidX, searchTop);
            const QPointF searchRightMid(searchRight, searchMidY);
            const QPointF searchLeftMid(searchLeft, searchMidY);
            const QPointF searchBtmMid(searchMidX, searchBottom);

            if ( isNearbyPoint(searchTopLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                hoverState = eDrawStateHoveringOuterTopLeft;
                hoverMarker = *it;
                hoverProcess = true;
            } else if ( isNearbyPoint(searchTopRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                hoverState = eDrawStateHoveringOuterTopRight;
                hoverMarker = *it;
                hoverProcess = true;
            } else if ( isNearbyPoint(searchBtmRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                hoverState = eDrawStateHoveringOuterBtmRight;
                hoverMarker = *it;
                hoverProcess = true;
            } else if ( isNearbyPoint(searchBtmLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                hoverState = eDrawStateHoveringOuterBtmLeft;
                hoverMarker = *it;
                hoverProcess = true;
            } else if ( isNearbyPoint(searchTopMid, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                hoverState = eDrawStateHoveringOuterTopMid;
                hoverMarker = *it;
                hoverProcess = true;
            } else if ( isNearbyPoint(searchBtmMid, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                hoverState = eDrawStateHoveringOuterBtmMid;
                hoverMarker = *it;
                hoverProcess = true;
            } else if ( isNearbyPoint(searchLeftMid, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                hoverState = eDrawStateHoveringOuterMidLeft;
                hoverMarker = *it;
                hoverProcess = true;
            } else if ( isNearbyPoint(searchRightMid, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                hoverState = eDrawStateHoveringOuterMidRight;
                hoverMarker = *it;
                hoverProcess = true;
            }
        }

        if (hoverProcess) {
            break;
        }
    } // for (std::vector<TrackMarkerPtr>::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {

    if ( showMarkerTexture && selectedMarkerTexture && isNearbySelectedMarkerTextureResizeAnchor(pos) ) {
        _imp->publicInterface->setCurrentCursor(eCursorFDiag);
        hoverProcess = true;
    } else if ( showMarkerTexture && selectedMarkerTexture && isInsideSelectedMarkerTexture(pos) ) {
        _imp->publicInterface->setCurrentCursor(eCursorSizeAll);
        hoverProcess = true;
    } else if ( showMarkerTexture && (isInsideKeyFrameTexture(time, pos, viewportPos) != INT_MAX) ) {
        _imp->publicInterface->setCurrentCursor(eCursorPointingHand);
        hoverProcess = true;
    } else {
        _imp->publicInterface->setCurrentCursor(eCursorDefault);
    }

    if ( showMarkerTexture && selectedMarkerTexture && shiftDown && isInsideSelectedMarkerTexture(pos) ) {
        hoverState = eDrawStateShowScalingHint;
        hoverProcess = true;
    }

    if (hoverProcess) {
        didSomething = true;
    }

    KnobDoublePtr centerKnob, offsetKnob, searchWndTopRight, searchWndBtmLeft;
    KnobDoublePtr patternCorners[4];
    if (interactMarker) {
        centerKnob = interactMarker->getCenterKnob();
        offsetKnob = interactMarker->getOffsetKnob();

        /*

         TopLeft(0) ------------- Top right(3)
         |                        |
         |                        |
         |                        |
         Btm left (1) ------------ Btm right (2)

         */
        patternCorners[0] = interactMarker->getPatternTopLeftKnob();
        patternCorners[1] = interactMarker->getPatternBtmLeftKnob();
        patternCorners[2] = interactMarker->getPatternBtmRightKnob();
        patternCorners[3] = interactMarker->getPatternTopRightKnob();
        searchWndTopRight = interactMarker->getSearchWindowTopRightKnob();
        searchWndBtmLeft = interactMarker->getSearchWindowBottomLeftKnob();
    }
    if (!trackingPageSecret) {
        switch (eventState) {
            case eMouseStateDraggingCenter:
            case eMouseStateDraggingOffset: {
                assert(interactMarker);
                if (!centerKnob || !offsetKnob || !patternCorners[0] || !patternCorners[1] || !patternCorners[2] | !patternCorners[3]) {
                    didSomething = false;
                    break;
                }


                if (eventState == eMouseStateDraggingOffset) {
                    std::vector<double> values(2);
                    values[0] = offsetKnob->getValueAtTime(time, DimIdx(0)) + delta.x;
                    values[1] = offsetKnob->getValueAtTime(time, DimIdx(1)) + delta.y;
                    offsetKnob->setValueAcrossDimensions(values);
                } else {
                    {
                        std::vector<double> values(2);
                        values[0] = centerKnob->getValueAtTime(time, DimIdx(0)) + delta.x;
                        values[1] = centerKnob->getValueAtTime(time, DimIdx(1)) + delta.y;
                        centerKnob->setValueAtTimeAcrossDimensions(time, values);
                    }
                    for (int i = 0; i < 4; ++i) {
                        std::vector<double> values(2);
                        values[0] = patternCorners[i]->getValueAtTime(time, DimIdx(0));
                        values[1] = patternCorners[i]->getValueAtTime(time, DimIdx(1));
                        patternCorners[i]->setValueAcrossDimensions(values);
                    }
                }
                refreshSelectedMarkerTextureLater();
                if ( createKeyOnMoveButton.lock()->getValue() ) {
                    interactMarker->setKeyFrame(time);
                }
                didSomething = true;
                break;
            }
            case eMouseStateDraggingInnerBtmLeft:
            case eMouseStateDraggingInnerTopRight:
            case eMouseStateDraggingInnerTopLeft:
            case eMouseStateDraggingInnerBtmRight: {
                if (controlDown == 0) {
                    transformPattern(time, eventState, delta);
                    didSomething = true;
                    break;
                }
                if (!centerKnob || !offsetKnob || !searchWndBtmLeft || !searchWndTopRight || !patternCorners[0] || !patternCorners[1] || !patternCorners[2] | !patternCorners[3]) {
                    didSomething = false;
                    break;
                }

                int index = 0;
                if (eventState == eMouseStateDraggingInnerBtmLeft) {
                    index = 1;
                } else if (eventState == eMouseStateDraggingInnerBtmRight) {
                    index = 2;
                } else if (eventState == eMouseStateDraggingInnerTopRight) {
                    index = 3;
                } else if (eventState == eMouseStateDraggingInnerTopLeft) {
                    index = 0;
                }

                int nextIndex = (index + 1) % 4;
                int prevIndex = (index + 3) % 4;
                int diagIndex = (index + 2) % 4;
                Point center;
                center.x = centerKnob->getValueAtTime(time, DimIdx(0));
                center.y = centerKnob->getValueAtTime(time, DimIdx(1));
                Point offset;
                offset.x = offsetKnob->getValueAtTime(time, DimIdx(0));
                offset.y = offsetKnob->getValueAtTime(time, DimIdx(1));

                Point cur, prev, next, diag;
                cur.x = patternCorners[index]->getValueAtTime(time, DimIdx(0)) + delta.x  + center.x + offset.x;;
                cur.y = patternCorners[index]->getValueAtTime(time, DimIdx(1)) + delta.y  + center.y + offset.y;

                prev.x = patternCorners[prevIndex]->getValueAtTime(time, DimIdx(0))  + center.x + offset.x;;
                prev.y = patternCorners[prevIndex]->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;

                next.x = patternCorners[nextIndex]->getValueAtTime(time, DimIdx(0))  + center.x + offset.x;;
                next.y = patternCorners[nextIndex]->getValueAtTime(time, DimIdx(1))  + center.y + offset.y;

                diag.x = patternCorners[diagIndex]->getValueAtTime(time, DimIdx(0))  + center.x + offset.x;;
                diag.y = patternCorners[diagIndex]->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;

                Point nextVec;
                nextVec.x = next.x - cur.x;
                nextVec.y = next.y - cur.y;

                Point prevVec;
                prevVec.x = cur.x - prev.x;
                prevVec.y = cur.y - prev.y;

                Point nextDiagVec, prevDiagVec;
                prevDiagVec.x = diag.x - next.x;
                prevDiagVec.y = diag.y - next.y;

                nextDiagVec.x = prev.x - diag.x;
                nextDiagVec.y = prev.y - diag.y;

                //Clamp so the 4 points remaing the same in the homography
                if (prevVec.x * nextVec.y - prevVec.y * nextVec.x < 0.) {         // cross-product
                    TrackerNodeInteract::findLineIntersection(cur, prev, next, &cur);
                }
                if (nextDiagVec.x * prevVec.y - nextDiagVec.y * prevVec.x < 0.) {         // cross-product
                    TrackerNodeInteract::findLineIntersection(cur, prev, diag, &cur);
                }
                if (nextVec.x * prevDiagVec.y - nextVec.y * prevDiagVec.x < 0.) {         // cross-product
                    TrackerNodeInteract::findLineIntersection(cur, next, diag, &cur);
                }


                Point searchWindowCorners[2];
                searchWindowCorners[0].x = searchWndBtmLeft->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
                searchWindowCorners[0].y = searchWndBtmLeft->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;

                searchWindowCorners[1].x = searchWndTopRight->getValueAtTime(time, DimIdx(0))  + center.x + offset.x;
                searchWindowCorners[1].y = searchWndTopRight->getValueAtTime(time, DimIdx(1))  + center.y + offset.y;

                cur.x = boost::algorithm::clamp(cur.x, searchWindowCorners[0].x, searchWindowCorners[1].x);
                cur.y = boost::algorithm::clamp(cur.y, searchWindowCorners[0].y, searchWindowCorners[1].y);

                cur.x -= (center.x + offset.x);
                cur.y -= (center.y + offset.y);

                {
                    std::vector<double> values(2);
                    values[0] = cur.x;
                    values[1] = cur.y;
                    patternCorners[index]->setValueAcrossDimensions(values);
                }

                if ( createKeyOnMoveButton.lock()->getValue() ) {
                    interactMarker->setKeyFrame(time);
                }
                didSomething = true;
                break;
            }
            case eMouseStateDraggingOuterBtmLeft: {
                if (controlDown == 0) {
                    transformPattern(time, eventState, delta);
                    didSomething = true;
                    break;
                }
                if (!centerKnob || !offsetKnob || !searchWndBtmLeft || !searchWndTopRight || !patternCorners[0] || !patternCorners[1] || !patternCorners[2] | !patternCorners[3]) {
                    didSomething = false;
                    break;
                }

                Point center;
                center.x = centerKnob->getValueAtTime(time, DimIdx(0));
                center.y = centerKnob->getValueAtTime(time, DimIdx(1));
                Point offset;
                offset.x = offsetKnob->getValueAtTime(time, DimIdx(0));
                offset.y = offsetKnob->getValueAtTime(time, DimIdx(1));

                Point p = {0, 0.};
                p.x = searchWndBtmLeft->getValueAtTime(time, DimIdx(0)) + center.x + offset.x + delta.x;
                p.y = searchWndBtmLeft->getValueAtTime(time, DimIdx(1)) + center.y + offset.y + delta.y;
                Point topLeft;
                topLeft.x = patternCorners[0]->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
                topLeft.y = patternCorners[0]->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;
                Point btmLeft;
                btmLeft.x = patternCorners[1]->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
                btmLeft.y = patternCorners[1]->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;
                Point btmRight;
                btmRight.x = patternCorners[2]->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
                btmRight.y = patternCorners[2]->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;
                Point topRight;
                topRight.x = patternCorners[3]->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
                topRight.y = patternCorners[3]->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;

                // test every point: even topRight pattern corner may be on the left of topLeft
                p.x = std::min(p.x, topLeft.x);
                p.x = std::min(p.x, btmLeft.x);
                p.x = std::min(p.x, btmRight.x);
                p.x = std::min(p.x, topRight.x);

                p.y = std::min(p.y, topLeft.y);
                p.y = std::min(p.y, btmLeft.y);
                p.y = std::min(p.y, btmRight.y);
                p.y = std::min(p.y, topRight.y);

                p.x -= (center.x + offset.x);
                p.y -= (center.y + offset.y);

                std::vector<double> values(2);
                values[0] = p.x;
                values[1] = p.y;
                if ( searchWndBtmLeft->hasAnimation() ) {
                    searchWndBtmLeft->setValueAtTimeAcrossDimensions(time, values);
                } else {
                    searchWndBtmLeft->setValueAcrossDimensions(values);
                }

                refreshSelectedMarkerTextureLater();
                didSomething = true;
                break;
            }
            case eMouseStateDraggingOuterBtmRight: {
                if (controlDown == 0) {
                    transformPattern(time, eventState, delta);
                    didSomething = true;
                    break;
                }
                if (!centerKnob || !offsetKnob || !searchWndBtmLeft || !searchWndTopRight || !patternCorners[0] || !patternCorners[1] || !patternCorners[2] | !patternCorners[3]) {
                    didSomething = false;
                    break;
                }

                Point center;
                center.x = centerKnob->getValueAtTime(time, DimIdx(0));
                center.y = centerKnob->getValueAtTime(time, DimIdx(1));
                Point offset;
                offset.x = offsetKnob->getValueAtTime(time, DimIdx(0));
                offset.y = offsetKnob->getValueAtTime(time, DimIdx(1));

                Point p;
                p.x = searchWndTopRight->getValueAtTime(time, DimIdx(0)) + center.x + offset.x + delta.x;
                p.y = searchWndBtmLeft->getValueAtTime(time, DimIdx(1)) + center.y + offset.y + delta.y;

                Point topLeft;
                topLeft.x = patternCorners[0]->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
                topLeft.y = patternCorners[0]->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;
                Point btmLeft;
                btmLeft.x = patternCorners[1]->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
                btmLeft.y = patternCorners[1]->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;
                Point btmRight;
                btmRight.x = patternCorners[2]->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
                btmRight.y = patternCorners[2]->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;
                Point topRight;
                topRight.x = patternCorners[3]->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
                topRight.y = patternCorners[3]->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;

                // test every point: even topRight pattern corner may be on the left of topLeft
                p.x = std::max(p.x, topLeft.x);
                p.x = std::max(p.x, btmLeft.x);
                p.x = std::max(p.x, btmRight.x);
                p.x = std::max(p.x, topRight.x);

                p.y = std::min(p.y, topLeft.y);
                p.y = std::min(p.y, btmLeft.y);
                p.y = std::min(p.y, btmRight.y);
                p.y = std::min(p.y, topRight.y);

                p.x -= (center.x + offset.x);
                p.y -= (center.y + offset.y);

                if ( searchWndBtmLeft->hasAnimation() ) {
                    searchWndBtmLeft->setValueAtTime(time, p.y, ViewSetSpec::all(), DimIdx(1));
                } else {
                    searchWndBtmLeft->setValue(p.y, view, DimIdx(1));
                }
                if ( searchWndTopRight->hasAnimation() ) {
                    searchWndTopRight->setValueAtTime(time, p.x, ViewSetSpec::all(), DimIdx(0));
                } else {
                    searchWndTopRight->setValue(p.x, ViewSetSpec::all(), DimIdx(0));
                }

                refreshSelectedMarkerTextureLater();
                didSomething = true;
                break;
            }
            case eMouseStateDraggingOuterTopRight: {
                if (controlDown == 0) {
                    transformPattern(time, eventState, delta);
                    didSomething = true;
                    break;
                }
                if (!centerKnob || !offsetKnob || !searchWndBtmLeft || !searchWndTopRight ||  !patternCorners[0] || !patternCorners[1] || !patternCorners[2] | !patternCorners[3]) {
                    didSomething = false;
                    break;
                }

                Point center;
                center.x = centerKnob->getValueAtTime(time, DimIdx(0));
                center.y = centerKnob->getValueAtTime(time, DimIdx(1));
                Point offset;
                offset.x = offsetKnob->getValueAtTime(time, DimIdx(0));
                offset.y = offsetKnob->getValueAtTime(time, DimIdx(1));

                Point p = {0, 0};
                if (searchWndTopRight) {
                    p.x = searchWndTopRight->getValueAtTime(time, DimIdx(0)) + center.x + offset.x + delta.x;
                    p.y = searchWndTopRight->getValueAtTime(time, DimIdx(1)) + center.y + offset.y + delta.y;
                }

                Point topLeft;
                topLeft.x = patternCorners[0]->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
                topLeft.y = patternCorners[0]->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;
                Point btmLeft;
                btmLeft.x = patternCorners[1]->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
                btmLeft.y = patternCorners[1]->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;
                Point btmRight;
                btmRight.x = patternCorners[2]->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
                btmRight.y = patternCorners[2]->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;
                Point topRight;
                topRight.x = patternCorners[3]->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
                topRight.y = patternCorners[3]->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;

                // test every point: even topRight pattern corner may be on the left of topLeft
                p.x = std::max(p.x, topLeft.x);
                p.x = std::max(p.x, btmLeft.x);
                p.x = std::max(p.x, btmRight.x);
                p.x = std::max(p.x, topRight.x);

                p.y = std::max(p.y, topLeft.y);
                p.y = std::max(p.y, btmLeft.y);
                p.y = std::max(p.y, btmRight.y);
                p.y = std::max(p.y, topRight.y);

                p.x -= (center.x + offset.x);
                p.y -= (center.y + offset.y);

                std::vector<double> values(2);
                values[0] = p.x;
                values[1] = p.y;
                if ( searchWndTopRight->hasAnimation() ) {
                    searchWndTopRight->setValueAtTimeAcrossDimensions(time, values);
                } else {
                    searchWndTopRight->setValueAcrossDimensions(values);
                }

                refreshSelectedMarkerTextureLater();
                didSomething = true;
                break;
            }
            case eMouseStateDraggingOuterTopLeft: {
                if (controlDown == 0) {
                    transformPattern(time, eventState, delta);
                    didSomething = true;
                    break;
                }
                if (!centerKnob || !offsetKnob || !searchWndBtmLeft || !searchWndTopRight || !patternCorners[0] || !patternCorners[1] || !patternCorners[2] | !patternCorners[3]) {
                    didSomething = false;
                    break;
                }

                Point center;
                center.x = centerKnob->getValueAtTime(time, DimIdx(0));
                center.y = centerKnob->getValueAtTime(time, DimIdx(1));
                Point offset;
                offset.x = offsetKnob->getValueAtTime(time, DimIdx(0));
                offset.y = offsetKnob->getValueAtTime(time, DimIdx(1));

                Point p;
                p.x = searchWndBtmLeft->getValueAtTime(time, DimIdx(0)) + center.x + offset.x + delta.x;
                p.y = searchWndTopRight->getValueAtTime(time, DimIdx(1)) + center.y + offset.y + delta.y;

                Point topLeft;
                topLeft.x = patternCorners[0]->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
                topLeft.y = patternCorners[0]->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;
                Point btmLeft;
                btmLeft.x = patternCorners[1]->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
                btmLeft.y = patternCorners[1]->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;
                Point btmRight;
                btmRight.x = patternCorners[2]->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
                btmRight.y = patternCorners[2]->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;
                Point topRight;
                topRight.x = patternCorners[3]->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
                topRight.y = patternCorners[3]->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;

                // test every point: even topRight pattern corner may be on the left of topLeft
                p.x = std::min(p.x, topLeft.x);
                p.x = std::min(p.x, btmLeft.x);
                p.x = std::min(p.x, btmRight.x);
                p.x = std::min(p.x, topRight.x);

                p.y = std::max(p.y, topLeft.y);
                p.y = std::max(p.y, btmLeft.y);
                p.y = std::max(p.y, btmRight.y);
                p.y = std::max(p.y, topRight.y);

                p.x -= (center.x + offset.x);
                p.y -= (center.y + offset.y);
                if ( searchWndBtmLeft->hasAnimation() ) {
                    searchWndBtmLeft->setValueAtTime(time, p.x, view, DimIdx(0));
                } else {
                    searchWndBtmLeft->setValue(p.x, view, DimIdx(0));
                }
                if ( searchWndTopRight->hasAnimation() ) {
                    searchWndTopRight->setValueAtTime(time, p.y, view, DimIdx(1));
                } else {
                    searchWndTopRight->setValue(p.y, view, DimIdx(1));
                }

                refreshSelectedMarkerTextureLater();
                didSomething = true;
                break;
            }
            case eMouseStateDraggingInnerBtmMid:
            case eMouseStateDraggingInnerTopMid:
            case eMouseStateDraggingInnerMidLeft:
            case eMouseStateDraggingInnerMidRight:
            case eMouseStateDraggingOuterBtmMid:
            case eMouseStateDraggingOuterTopMid:
            case eMouseStateDraggingOuterMidLeft:
            case eMouseStateDraggingOuterMidRight: {
                transformPattern(time, eventState, delta);
                didSomething = true;
                break;
            }
            case eMouseStateDraggingSelectedMarkerResizeAnchor: {
                QPointF lastPosWidget = overlay->toWidgetCoordinates(lastMousePos);
                double dx = viewportPos.x() - lastPosWidget.x();
                KnobIntPtr knob = magWindowPxSizeKnob.lock();
                int value = knob->getValue();
                value += dx;
                value = std::max(value, 10);
                knob->setValue(value);
                didSomething = true;
                break;
            }
            case eMouseStateScalingSelectedMarker: {
                TrackMarkerPtr marker = selectedMarker.lock();
                assert(marker);
                RectD markerMagRect;
                computeSelectedMarkerCanonicalRect(&markerMagRect);
                KnobDoublePtr centerKnob = marker->getCenterKnob();
                KnobDoublePtr offsetKnob = marker->getOffsetKnob();
                KnobDoublePtr searchBtmLeft = marker->getSearchWindowBottomLeftKnob();
                KnobDoublePtr searchTopRight = marker->getSearchWindowTopRightKnob();
                if (!centerKnob || !offsetKnob || !searchBtmLeft || !searchTopRight) {
                    didSomething = false;
                    break;
                }

                Point center, offset, btmLeft, topRight;
                center.x = centerKnob->getValueAtTime(time, DimIdx(0));
                center.y = centerKnob->getValueAtTime(time, DimIdx(1));
                offset.x = offsetKnob->getValueAtTime(time, DimIdx(0));
                offset.y = offsetKnob->getValueAtTime(time, DimIdx(1));
                btmLeft.x = searchBtmLeft->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
                btmLeft.y = searchBtmLeft->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;
                topRight.x = searchTopRight->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
                topRight.y = searchTopRight->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;

                //Remove any offset to the center to see the marker in the magnification window
                double xCenterPercent = (center.x - btmLeft.x + offset.x) / (topRight.x - btmLeft.x);
                double yCenterPercent = (center.y - btmLeft.y + offset.y) / (topRight.y - btmLeft.y);
                Point centerPoint;
                centerPoint.x = markerMagRect.x1 + xCenterPercent * (markerMagRect.x2 - markerMagRect.x1);
                centerPoint.y = markerMagRect.y1 + yCenterPercent * (markerMagRect.y2 - markerMagRect.y1);

                double prevDist = std::sqrt( (lastMousePos.x() - centerPoint.x ) * ( lastMousePos.x() - centerPoint.x) + ( lastMousePos.y() - centerPoint.y) * ( lastMousePos.y() - centerPoint.y) );
                if (prevDist != 0) {
                    double dist = std::sqrt( ( pos.x() - centerPoint.x) * ( pos.x() - centerPoint.x) + ( pos.y() - centerPoint.y) * ( pos.y() - centerPoint.y) );
                    double ratio = dist / prevDist;
                    selectedMarkerScale.x *= ratio;
                    selectedMarkerScale.x = boost::algorithm::clamp(selectedMarkerScale.x, 0.05, 1.);
                    selectedMarkerScale.y = selectedMarkerScale.x;
                    didSomething = true;
                }
                break;
            }
            case eMouseStateDraggingSelectedMarker: {
                if (!centerKnob || !offsetKnob || !searchWndBtmLeft || !searchWndTopRight || !patternCorners[0] || !patternCorners[1] || !patternCorners[2] | !patternCorners[3]) {
                    didSomething = false;
                    break;
                }

                double x = centerKnob->getValueAtTime(time, DimIdx(0));
                double y = centerKnob->getValueAtTime(time, DimIdx(1));
                double dx = delta.x *  selectedMarkerScale.x;
                double dy = delta.y *  selectedMarkerScale.y;
                x += dx;
                y += dy;

                std::vector<double> values(2);
                values[0] = x;
                values[1] = y;
                centerKnob->setValueAtTimeAcrossDimensions(time, values);
                for (int i = 0; i < 4; ++i) {
                    values[0] = patternCorners[i]->getValueAtTime(time, DimIdx(0));
                    values[1] = patternCorners[i]->getValueAtTime(time, DimIdx(1));
                    patternCorners[i]->setValueAcrossDimensions(values);
                }
                if ( createKeyOnMoveButton.lock()->getValue() ) {
                    interactMarker->setKeyFrame(time);
                }
                refreshSelectedMarkerTextureLater();
                didSomething = true;
                break;
            }
            default:
                break;
        } // switch
    } // !trackingPageSecret
    if (clickToAddTrackEnabled) {
        ///Refresh the overlay
        didSomething = true;
    }
    lastMousePos = pos;

    return didSomething;
} //penMotion

bool
TrackerNodeInteract::onOverlayPenDoubleClicked(TimeValue /*time*/,
                                       const RenderScale & /*renderScale*/,
                                       ViewIdx /*view*/,
                                       const QPointF & /*viewportPos*/,
                                       const QPointF & /*pos*/)
{
    return false;
}

bool
TrackerNodeInteract::onOverlayPenUp(TimeValue /*time*/,
                            const RenderScale & /*renderScale*/,
                            ViewIdx /*view*/,
                            const QPointF & /*viewportPos*/,
                            const QPointF & /*pos*/,
                            double /*pressure*/,
                            TimeValue /*timestamp*/)
{
    bool didSomething = false;
    TrackerMouseStateEnum state = eventState;

    if (state != eMouseStateIdle) {
        eventState = eMouseStateIdle;
        didSomething = true;
    }
    if (interactMarker) {
        interactMarker.reset();
        didSomething = true;
    }

    return didSomething;
} // penUp

bool
TrackerNodeInteract::onOverlayKeyDown(TimeValue /*time*/,
                              const RenderScale & /*renderScale*/,
                              ViewIdx /*view*/,
                              Key key,
                              KeyboardModifiers /*modifiers*/)
{
    bool didSomething = false;
    bool isCtrl = false;
    bool isAlt = false;

    if ( (key == Key_Shift_L) || (key == Key_Shift_R) ) {
        ++shiftDown;
    } else if ( (key == Key_Control_L) || (key == Key_Control_R) ) {
        ++controlDown;
        isCtrl = true;
    } else if ( (key == Key_Alt_L) || (key == Key_Alt_R) ) {
        ++altDown;
        isAlt = true;
    }
    
    bool trackingPageSecret = _imp->trackingPageKnob.lock()->getIsSecret();
    
    if ( !trackingPageSecret && controlDown && altDown && !shiftDown && (isCtrl || isAlt) ) {
        clickToAddTrackEnabled = true;
        addTrackButton.lock()->setValue(true);
        didSomething = true;
    }
    
    
    return didSomething;
} // keydown

bool
TrackerNodeInteract::onOverlayKeyUp(TimeValue /*time*/,
                            const RenderScale & /*renderScale*/,
                            ViewIdx /*view*/,
                            Key key,
                            KeyboardModifiers /*modifiers*/)
{
    bool didSomething = false;
    bool isAlt = false;
    bool isControl = false;
    
    if ( (key == Key_Shift_L) || (key == Key_Shift_R) ) {
        if (shiftDown) {
            --shiftDown;
        }
        if (eventState == eMouseStateScalingSelectedMarker) {
            eventState = eMouseStateIdle;
            didSomething = true;
        }
    } else if ( (key == Key_Control_L) || (key == Key_Control_R) ) {
        if (controlDown) {
            --controlDown;
        }
        isControl = true;
    } else if ( (key == Key_Alt_L) || (key == Key_Alt_R) ) {
        if (altDown) {
            --altDown;
        }
        isAlt = true;
    }
    
    
    if ( clickToAddTrackEnabled && (isControl || isAlt) ) {
        clickToAddTrackEnabled = false;
        addTrackButton.lock()->setValue(false);
        didSomething = true;
    }
    
    
    return didSomething;
} // KeyUp

bool
TrackerNodeInteract::onOverlayKeyRepeat(TimeValue /*time*/,
                                const RenderScale & /*renderScale*/,
                                ViewIdx /*view*/,
                                Key /*key*/,
                                KeyboardModifiers /* modifiers*/)
{
    return false;
} // keyrepeat

bool
TrackerNodeInteract::onOverlayFocusGained(TimeValue /*time*/,
                                  const RenderScale & /*renderScale*/,
                                  ViewIdx /* view*/)
{
    return false;
} // gainFocus

bool
TrackerNodeInteract::onOverlayFocusLost(TimeValue /*time*/,
                                const RenderScale & /*renderScale*/,
                                ViewIdx /*view*/)
{
    altDown = 0;
    controlDown = 0;
    shiftDown = 0;
    
    
    return true;
} // loseFocus

void
TrackerNodeInteract::onViewportSelectionCleared()
{
    bool trackingPageSecret = _imp->trackingPageKnob.lock()->getIsSecret();
    
    if (trackingPageSecret) {
        return;
    }
    
    _imp->knobsTable->clearSelection(eTableChangeReasonViewer);
}

void
TrackerNodeInteract::onViewportSelectionUpdated(const RectD& rectangle,
                                                bool onRelease)
{
    if (!onRelease) {
        return;
    }
    
    
    bool trackingPageSecret = _imp->trackingPageKnob.lock()->getIsSecret();
    if (trackingPageSecret) {
        return;
    }
    
    std::vector<TrackMarkerPtr> allMarkers;
    _imp->knobsTable->getAllMarkers(&allMarkers);

    std::list<KnobTableItemPtr> items;
    for (std::size_t i = 0; i < allMarkers.size(); ++i) {
        if ( !allMarkers[i]->isEnabled( allMarkers[i]->getTimelineCurrentTime() ) ) {
            continue;
        }
        KnobDoublePtr center = allMarkers[i]->getCenterKnob();
        double x, y;
        x = center->getValue();
        y = center->getValue(DimIdx(1));
        if ( (x >= rectangle.x1) && (x <= rectangle.x2) && (y >= rectangle.y1) && (y <= rectangle.y2) ) {
            items.push_back(allMarkers[i]);
        }
    }
    
    _imp->knobsTable->beginEditSelection();
    _imp->knobsTable->clearSelection(eTableChangeReasonInternal);
    _imp->knobsTable->addToSelection(items, eTableChangeReasonInternal);
    _imp->knobsTable->endEditSelection(eTableChangeReasonInternal);
}


NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING
#include "moc_TrackerNodePrivate.cpp"
