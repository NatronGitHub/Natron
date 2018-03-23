/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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


#include "TrackerNodeInteract.h"

#ifndef NDEBUG
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include "Engine/Image.h"
#include "Engine/Lut.h"
#include "Engine/Node.h"
#include "Engine/OpenGLViewerI.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/TimeLine.h"
#include "Engine/TrackerNode.h"
#include "Engine/TrackerContext.h"
#include "Engine/TrackMarker.h"
#include "Engine/ViewerInstance.h"

NATRON_NAMESPACE_ENTER

TrackerNodePrivate::TrackerNodePrivate(TrackerNode* publicInterface)
    : publicInterface(publicInterface)
    , ui( new TrackerNodeInteract(this) )
{
}

TrackerNodePrivate::~TrackerNodePrivate()
{
}

TrackerNodeInteract::TrackerNodeInteract(TrackerNodePrivate* p)
    : _p(p)
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
    , selectedMarkerTextureTime(0)
    , selectedMarkerTextureRoI()
    , selectedMarker()
    , pboID(0)
    , imageGetterWatcher()
    , showMarkerTexture(false)
    , selectedMarkerScale()
    , selectedMarkerImg()
    , isTracking(false)
{
    selectedMarkerScale.x = selectedMarkerScale.y = 1.;
}

TrackerNodeInteract::~TrackerNodeInteract()
{
    if (pboID != 0) {
        glDeleteBuffers(1, &pboID);
    }
}

boost::shared_ptr<TrackerContext>
TrackerNodeInteract::getContext() const
{
    return _p->publicInterface->getNode()->getTrackerContext();
}

void
TrackerNodeInteract::onTrackRangeClicked()
{
    OverlaySupport* overlay = _p->publicInterface->getCurrentViewportForOverlays();

    assert(overlay);
    ViewerInstance* viewer = overlay->getInternalViewerNode();
    assert(viewer);
    int viewerFirst, viewerLast;
    viewer->getUiContext()->getViewerFrameRange(&viewerFirst, &viewerLast);

    int first = trackRangeDialogFirstFrame.lock()->getValue();
    int last = trackRangeDialogLastFrame.lock()->getValue();
    int step = trackRangeDialogStep.lock()->getValue();
    if (first == INT_MIN) {
        trackRangeDialogFirstFrame.lock()->setValue(viewerFirst);
    }
    if (last == INT_MIN) {
        trackRangeDialogLastFrame.lock()->setValue(viewerLast);
    }
    if (step == INT_MIN) {
        trackRangeDialogStep.lock()->setValue(1);
    }
    boost::shared_ptr<KnobGroup> k = trackRangeDialogGroup.lock();
    if ( k->getValue() ) {
        k->setValue(false);
    } else {
        k->setValue(true);
    }
}

void
TrackerNodeInteract::onTrackAllKeyframesClicked()
{
    boost::shared_ptr<TrackerContext> ctx = getContext();
    std::list<TrackMarkerPtr> selectedMarkers;

    ctx->getSelectedMarkers(&selectedMarkers);

    std::set<int> userKeys;

    for (std::list<TrackMarkerPtr>::iterator it = selectedMarkers.begin(); it != selectedMarkers.end(); ++it) {
        std::set<int> trackUserKeys;
        (*it)->getUserKeyframes(&trackUserKeys);
        userKeys.insert( trackUserKeys.begin(), trackUserKeys.end() );
    }
    if ( userKeys.empty() ) {
        return;
    }

    int first = *userKeys.begin();
    int last = *userKeys.rbegin() + 1;
    ctx->trackSelectedMarkers( first, last, 1,  _p->publicInterface->getCurrentViewportForOverlays() );
}

void
TrackerNodeInteract::onTrackCurrentKeyframeClicked()
{
    boost::shared_ptr<TrackerContext> ctx = getContext();
    SequenceTime currentFrame = _p->publicInterface->getCurrentTime();
    std::list<TrackMarkerPtr> selectedMarkers;

    ctx->getSelectedMarkers(&selectedMarkers);

    std::set<int> userKeys;

    for (std::list<TrackMarkerPtr>::iterator it = selectedMarkers.begin(); it != selectedMarkers.end(); ++it) {
        std::set<int> trackUserKeys;
        (*it)->getUserKeyframes(&trackUserKeys);
        userKeys.insert( trackUserKeys.begin(), trackUserKeys.end() );
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

    ctx->trackSelectedMarkers( first, last, 1,  _p->publicInterface->getCurrentViewportForOverlays() );
}

void
TrackerNodeInteract::onTrackBwClicked()
{
    OverlaySupport* overlay = _p->publicInterface->getCurrentViewportForOverlays();

    assert(overlay);
    ViewerInstance* viewer = overlay->getInternalViewerNode();
    assert(viewer);
    int first, last;
    viewer->getUiContext()->getViewerFrameRange(&first, &last);

    int startFrame = viewer->getTimeline()->currentFrame();
    boost::shared_ptr<TrackerContext> ctx = getContext();
    if ( ctx->isCurrentlyTracking() ) {
        ctx->abortTracking();
    } else {
        ctx->trackSelectedMarkers( startFrame, first - 1, -1,  overlay );
    }
}

void
TrackerNodeInteract::onTrackPrevClicked()
{
    OverlaySupport* overlay = _p->publicInterface->getCurrentViewportForOverlays();

    assert(overlay);
    ViewerInstance* viewer = overlay->getInternalViewerNode();
    assert(viewer);
    int startFrame = viewer->getTimeline()->currentFrame();
    boost::shared_ptr<TrackerContext> ctx = getContext();
    ctx->trackSelectedMarkers( startFrame, startFrame - 2, -1, overlay );
}

void
TrackerNodeInteract::onStopButtonClicked()
{
    getContext()->abortTracking();
}

void
TrackerNodeInteract::onTrackNextClicked()
{
    OverlaySupport* overlay = _p->publicInterface->getCurrentViewportForOverlays();

    assert(overlay);
    ViewerInstance* viewer = overlay->getInternalViewerNode();
    assert(viewer);
    int startFrame = viewer->getTimeline()->currentFrame();
    boost::shared_ptr<TrackerContext> ctx = getContext();
    ctx->trackSelectedMarkers( startFrame, startFrame + 2, true, overlay );
}

void
TrackerNodeInteract::onTrackFwClicked()
{
    OverlaySupport* overlay = _p->publicInterface->getCurrentViewportForOverlays();

    assert(overlay);
    ViewerInstance* viewer = overlay->getInternalViewerNode();
    assert(viewer);
    int first, last;
    viewer->getUiContext()->getViewerFrameRange(&first, &last);

    int startFrame = viewer->getTimeline()->currentFrame();
    boost::shared_ptr<TrackerContext> ctx = getContext();
    if ( ctx->isCurrentlyTracking() ) {
        ctx->abortTracking();
    } else {
        ctx->trackSelectedMarkers( startFrame, last + 1, true,  overlay );
    }
}

void
TrackerNodeInteract::onUpdateViewerClicked(bool clicked)
{
    getContext()->setUpdateViewer(clicked);
}

void
TrackerNodeInteract::onAddTrackClicked(bool clicked)
{
    clickToAddTrackEnabled = clicked;
}

void
TrackerNodeInteract::onClearAllAnimationClicked()
{
    std::list<TrackMarkerPtr > markers;

    getContext()->getSelectedMarkers(&markers);
    for (std::list<TrackMarkerPtr >::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->clearAnimation();
    }
}

void
TrackerNodeInteract::onClearBwAnimationClicked()
{
    int time = _p->publicInterface->getCurrentTime();
    std::list<TrackMarkerPtr > markers;

    getContext()->getSelectedMarkers(&markers);
    for (std::list<TrackMarkerPtr >::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->clearAnimationBeforeTime(time);
    }
}

void
TrackerNodeInteract::onClearFwAnimationClicked()
{
    int time = _p->publicInterface->getCurrentTime();
    std::list<TrackMarkerPtr > markers;

    getContext()->getSelectedMarkers(&markers);
    for (std::list<TrackMarkerPtr >::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->clearAnimationAfterTime(time);
    }
}

void
TrackerNodeInteract::onCenterViewerButtonClicked(bool clicked)
{
    getContext()->setCenterOnTrack(clicked);
}

void
TrackerNodeInteract::onSetKeyframeButtonClicked()
{
    int time = _p->publicInterface->getCurrentTime();
    std::list<TrackMarkerPtr > markers;

    getContext()->getSelectedMarkers(&markers);
    for (std::list<TrackMarkerPtr >::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->setUserKeyframe(time);
    }
}

void
TrackerNodeInteract::onRemoveKeyframeButtonClicked()
{
    int time = _p->publicInterface->getCurrentTime();
    std::list<TrackMarkerPtr > markers;

    getContext()->getSelectedMarkers(&markers);
    for (std::list<TrackMarkerPtr >::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->removeUserKeyframe(time);
    }
}

void
TrackerNodeInteract::onResetOffsetButtonClicked()
{
    std::list<TrackMarkerPtr > markers;

    getContext()->getSelectedMarkers(&markers);
    for (std::list<TrackMarkerPtr >::iterator it = markers.begin(); it != markers.end(); ++it) {
        boost::shared_ptr<KnobDouble> offsetKnob = (*it)->getOffsetKnob();
        assert(offsetKnob);
        for (int i = 0; i < offsetKnob->getDimension(); ++i) {
            offsetKnob->resetToDefaultValue(i);
        }
    }
}

void
TrackerNodeInteract::onResetTrackButtonClicked()
{
    boost::shared_ptr<TrackerContext> context = getContext();

    assert(context);
    std::list<TrackMarkerPtr > markers;
    context->getSelectedMarkers(&markers);
    context->clearSelection(TrackerContext::eTrackSelectionInternal);
    context->endEditSelection(TrackerContext::eTrackSelectionInternal);

    for (std::list<TrackMarkerPtr >::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->resetTrack();
    }
    context->beginEditSelection(TrackerContext::eTrackSelectionInternal);
    context->addTracksToSelection(markers, TrackerContext::eTrackSelectionInternal);
    context->endEditSelection(TrackerContext::eTrackSelectionInternal);
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
TrackerNodeInteract::isInsideKeyFrameTexture(double currentTime,
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

    OverlaySupport* overlay = _p->publicInterface->getCurrentViewportForOverlays();
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

    OverlaySupport* overlay = _p->publicInterface->getCurrentViewportForOverlays();
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

    OverlaySupport* overlay = _p->publicInterface->getCurrentViewportForOverlays();
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
    OverlaySupport* overlay = _p->publicInterface->getCurrentViewportForOverlays();

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
    assert(selectedMarkerTexture);
    if (!selectedMarkerTexture) {
        rect->clear();
        return;
    }
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
    glColor3f(r * l * a, g * l * a, b * l * a);

    glPushMatrix();
    //  center the oval at x_center, y_center
    glTranslatef( (float)x, (float)y, 0.f );
    //  draw the oval using line segments
    glBegin(GL_LINE_LOOP);
    // we don't need to be pixel-perfect here, it's just an interact!
    // 40 segments is enough.
    double m = 2 * 3.14159265358979323846264338327950288419717 / 40.;
    for (int i = 0; i < 40; ++i) {
        double theta = i * m;
        glVertex2d( radiusX * std::cos(theta), radiusY * std::sin(theta) );
    }
    glEnd();

    glPopMatrix();
}

TrackerNodeInteract::KeyFrameTexIDs
TrackerNodeInteract::getKeysToRenderForMarker(double currentTime,
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
                                                 int currentTime)
{
    TrackMarkerPtr marker = selectedMarker.lock();

    assert(marker);
    if (!marker) {
        return;
    }
    if ( !marker->isEnabled(currentTime) ) {
        return;
    }

    OverlaySupport* overlay = _p->publicInterface->getCurrentViewportForOverlays();
    assert(overlay);

    double overlayColor[3];
    if ( !_p->publicInterface->getNode()->getOverlayColor(&overlayColor[0], &overlayColor[1], &overlayColor[2]) ) {
        overlayColor[0] = overlayColor[1] = overlayColor[2] = 0.8;
    }

    boost::shared_ptr<KnobDouble> centerKnob = marker->getCenterKnob();
    boost::shared_ptr<KnobDouble> offsetKnob = marker->getOffsetKnob();
    boost::shared_ptr<KnobDouble> errorKnob = marker->getErrorKnob();
    boost::shared_ptr<KnobDouble> ptnTopLeft = marker->getPatternTopLeftKnob();
    boost::shared_ptr<KnobDouble> ptnTopRight = marker->getPatternTopRightKnob();
    boost::shared_ptr<KnobDouble> ptnBtmRight = marker->getPatternBtmRightKnob();
    boost::shared_ptr<KnobDouble> ptnBtmLeft = marker->getPatternBtmLeftKnob();
    boost::shared_ptr<KnobDouble> searchWndBtmLeft = marker->getSearchWindowBottomLeftKnob();
    boost::shared_ptr<KnobDouble> searchWndTopRight = marker->getSearchWindowTopRightKnob();
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
                double time = (double)it2->first;
                Point offset, center, topLeft, topRight, btmRight, btmLeft;

                center.x = centerKnob->getValueAtTime(time, 0);
                center.y = centerKnob->getValueAtTime(time, 1);
                offset.x = offsetKnob->getValueAtTime(time, 0);
                offset.y = offsetKnob->getValueAtTime(time, 1);

                topLeft.x = ptnTopLeft->getValueAtTime(time, 0) + offset.x + center.x;
                topLeft.y = ptnTopLeft->getValueAtTime(time, 1) + offset.y + center.y;

                topRight.x = ptnTopRight->getValueAtTime(time, 0) + offset.x + center.x;
                topRight.y = ptnTopRight->getValueAtTime(time, 1) + offset.y + center.y;

                btmRight.x = ptnBtmRight->getValueAtTime(time, 0) + offset.x + center.x;
                btmRight.y = ptnBtmRight->getValueAtTime(time, 1) + offset.y + center.y;

                btmLeft.x = ptnBtmLeft->getValueAtTime(time, 0) + offset.x + center.x;
                btmLeft.y = ptnBtmLeft->getValueAtTime(time, 1) + offset.y + center.y;

                //const double searchLeft   = searchWndBtmLeft->getValueAtTime(time, 0) + offset.x + center.x;
                //const double searchRight  = searchWndTopRight->getValueAtTime(time, 0) + offset.x + center.x;
                //const double searchBottom = searchWndBtmLeft->getValueAtTime(time, 1) + offset.y + center.y;
                //const double searchTop    = searchWndTopRight->getValueAtTime(time, 1) + offset.y + center.y;

                const TextureRect& texRect = it2->second->getTextureRect();
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
                texRect.toCanonical_noClipping(0, texRect.par, &canonicalSearchWindow);

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
                glColor4f(1., 1., 1., 1.);
                glEnable(GL_TEXTURE_2D);
                glBindTexture( GL_TEXTURE_2D, it2->second->getTexID() );
                glBegin(GL_POLYGON);
                glTexCoord2d(0, 0); glVertex2d(textureRectCanonical.x1, textureRectCanonical.y1);
                glTexCoord2d(0, 1); glVertex2d(textureRectCanonical.x1, textureRectCanonical.y2);
                glTexCoord2d(1, 1); glVertex2d(textureRectCanonical.x2, textureRectCanonical.y2);
                glTexCoord2d(1, 0); glVertex2d(textureRectCanonical.x2, textureRectCanonical.y1);
                glEnd();

                glBindTexture(GL_TEXTURE_2D, 0);

                QPointF textPos = overlay->toCanonicalCoordinates( QPointF(xOffsetPixels + 5, fontHeight + 5 ) );
                overlay->renderText(textPos.x(), textPos.y(), marker->getLabel(), overlayColor[0], overlayColor[1], overlayColor[2]);

                QPointF framePos = overlay->toCanonicalCoordinates( QPointF( xOffsetPixels + 5, overlay->toWidgetCoordinates( QPointF(textureRectCanonical.x1, textureRectCanonical.y1) ).y() ) );
                QString frameText = tr("Frame");
                frameText.append( QString::fromUtf8(" ") + QString::number(it2->first) );
                overlay->renderText(framePos.x(), framePos.y(), frameText.toStdString(), overlayColor[0], overlayColor[1], overlayColor[2]);

                //Draw contour
                glEnable(GL_LINE_SMOOTH);
                glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                if (time == currentTime) {
                    glColor4f(0.93, 0.54, 0, 1);
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
                        glColor4f(0.93, 0.54, 0, 1);
                    } else if ( ( prev == keysToRender.end() ) && (time > currentTime) ) {
                        //Before the first keyframe
                        glColor4f(0.93, 0.54, 0, 1);
                    } else {
                        if (time < currentTime) {
                            assert( next != keysToRender.end() );
                            if (next->first > currentTime) {
                                glColor4f(1, 0.75, 0.47, 1);
                            } else {
                                glColor4f(1., 1., 1., 0.5);
                            }
                        } else {
                            //time > currentTime
                            assert( prev != keysToRender.end() );
                            if (prev->first < currentTime) {
                                glColor4f(1, 0.75, 0.47, 1);
                            } else {
                                glColor4f(1., 1., 1., 0.5);
                            }
                        }
                    }
                }

                glLineWidth(1.5);
                glCheckError();
                glBegin(GL_LINE_LOOP);
                glVertex2d(textureRectCanonical.x1, textureRectCanonical.y1);
                glVertex2d(textureRectCanonical.x1, textureRectCanonical.y2);
                glVertex2d(textureRectCanonical.x2, textureRectCanonical.y2);
                glVertex2d(textureRectCanonical.x2, textureRectCanonical.y1);
                glEnd();

                glCheckError();


                //Draw internal marker
                for (int l = 0; l < 2; ++l) {
                    // shadow (uses GL_PROJECTION)
                    glMatrixMode(GL_PROJECTION);
                    int direction = (l == 0) ? 1 : -1;
                    // translate (1,-1) pixels
                    glTranslated(direction * pixelScale.first / 256, -direction * pixelScale.second / 256, 0);
                    glMatrixMode(GL_MODELVIEW);

                    glColor4f(0.8 * l, 0.8 * l, 0.8 * l, 1);

                    glBegin(GL_LINE_LOOP);
                    glVertex2d(innerTopLeft.x, innerTopLeft.y);
                    glVertex2d(innerTopRight.x, innerTopRight.y);
                    glVertex2d(innerBtmRight.x, innerBtmRight.y);
                    glVertex2d(innerBtmLeft.x, innerBtmLeft.y);
                    glEnd();

                    glBegin(GL_POINTS);
                    glVertex2d(centerPointCanonical.x, centerPointCanonical.y);
                    glEnd();
                }

                xOffsetPixels += TO_DPIX(SELECTED_MARKER_KEYFRAME_WIDTH_SCREEN_PX);
            }
            break;
        }
    }
} // TrackerNodeInteract::drawSelectedMarkerKeyframes

void
TrackerNodeInteract::drawSelectedMarkerTexture(const std::pair<double, double>& pixelScale,
                                               int currentTime,
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
    OverlaySupport* overlay = _p->publicInterface->getCurrentViewportForOverlays();

    assert(overlay);
    ViewerInstance* viewer = overlay->getInternalViewerNode();
    assert(viewer);

    if ( isTracking || !selectedMarkerTexture || !marker || !marker->isEnabled(currentTime) || viewer->getRenderEngine()->isDoingSequentialRender() ) {
        return;
    }

    RectD textureRectCanonical;
    computeSelectedMarkerCanonicalRect(&textureRectCanonical);


    double overlayColor[3];
    if ( !_p->publicInterface->getNode()->getOverlayColor(&overlayColor[0], &overlayColor[1], &overlayColor[2]) ) {
        overlayColor[0] = overlayColor[1] = overlayColor[2] = 0.8;
    }

    const TextureRect& texRect = selectedMarkerTexture->getTextureRect();
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
    texRect.toCanonical_noClipping(0, texRect.par, &canonicalSearchWindow);

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
    glColor4f(1., 1., 1., 1.);
    glEnable(GL_TEXTURE_2D);
    glBindTexture( GL_TEXTURE_2D, selectedMarkerTexture->getTexID() );
    glBegin(GL_POLYGON);
    glTexCoord2d(btmLeftTex.x, btmRightTex.y); glVertex2d(textureRectCanonical.x1, textureRectCanonical.y1);
    glTexCoord2d(topLeftTex.x, topLeftTex.y); glVertex2d(textureRectCanonical.x1, textureRectCanonical.y2);
    glTexCoord2d(topRightTex.x, topRightTex.y); glVertex2d(textureRectCanonical.x2, textureRectCanonical.y2);
    glTexCoord2d(btmRightTex.x, btmRightTex.y); glVertex2d(textureRectCanonical.x2, textureRectCanonical.y1);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);

    //Draw contour
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1., 1., 1., 0.5);
    glLineWidth(1.5);
    glCheckError();
    glBegin(GL_LINE_LOOP);
    glVertex2d(textureRectCanonical.x1, textureRectCanonical.y1);
    glVertex2d(textureRectCanonical.x1, textureRectCanonical.y2);
    glVertex2d(textureRectCanonical.x2, textureRectCanonical.y2);
    glVertex2d(textureRectCanonical.x2, textureRectCanonical.y1);
    glEnd();

    glColor4f(0.8, 0.8, 0.8, 1.);
    glPointSize(POINT_SIZE);
    glBegin(GL_POINTS);
    glVertex2d(textureRectCanonical.x2, textureRectCanonical.y1);
    glEnd();
    glCheckError();

    int fontHeight = overlay->getWidgetFontHeight();
    QPointF textPos = overlay->toCanonicalCoordinates( QPointF(5, fontHeight + 5) );
    overlay->renderText(textPos.x(), textPos.y(), marker->getLabel(), overlayColor[0], overlayColor[1], overlayColor[2]);

    //Draw internal marker

    for (int l = 0; l < 2; ++l) {
        // shadow (uses GL_PROJECTION)
        glMatrixMode(GL_PROJECTION);
        int direction = (l == 0) ? 1 : -1;
        // translate (1,-1) pixels
        glTranslated(direction * pixelScale.first / 256, -direction * pixelScale.second / 256, 0);
        glMatrixMode(GL_MODELVIEW);

        glColor4f(0.8 * l, 0.8 * l, 0.8 * l, 1);

        glBegin(GL_LINE_LOOP);
        glVertex2d(innerTopLeft.x, innerTopLeft.y);
        glVertex2d(innerTopRight.x, innerTopRight.y);
        glVertex2d(innerBtmRight.x, innerBtmRight.y);
        glVertex2d(innerBtmLeft.x, innerBtmLeft.y);
        glEnd();

        glBegin(GL_POINTS);
        glVertex2d(centerPoint.x, centerPoint.y);
        glEnd();

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
TrackerNodeInteract::isNearbyPoint(const boost::shared_ptr<KnobDouble>& knob,
                                   double xWidget,
                                   double yWidget,
                                   double toleranceWidget,
                                   double time)
{
    QPointF p;
    OverlaySupport* overlay = _p->publicInterface->getCurrentViewportForOverlays();

    assert(overlay);

    p.rx() = knob->getValueAtTime(time, 0);
    p.ry() = knob->getValueAtTime(time, 1);
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
    OverlaySupport* overlay = _p->publicInterface->getCurrentViewportForOverlays();

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
TrackerNodeInteract::refreshSelectedMarkerTexture()
{
    assert( QThread::currentThread() == qApp->thread() );
    if (isTracking) {
        return;
    }
    TrackMarkerPtr marker = selectedMarker.lock();
    if (!marker) {
        return;
    }

    int time = _p->publicInterface->getCurrentTime();
    RectI roi = marker->getMarkerImageRoI(time);
    if ( roi.isNull() ) {
        return;
    }
    ImagePtr existingMarkerImg = selectedMarkerImg.lock();
    if ( existingMarkerImg && (existingMarkerImg->getTime() == time) && (roi == selectedMarkerTextureRoI) ) {
        return;
    }

    selectedMarkerImg.reset();

    imageGetterWatcher = boost::make_shared<TrackWatcher>();
    QObject::connect( imageGetterWatcher.get(), SIGNAL(finished()), this, SLOT(onTrackImageRenderingFinished()) );
    imageGetterWatcher->setFuture( QtConcurrent::run(marker.get(), &TrackMarker::getMarkerImage, time, roi) );
}

void
TrackerNodeInteract::makeMarkerKeyTexture(int time,
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
            const TextureRect& texRect = foundKey->second->getTextureRect();
            if ( (k.roi.x1 == texRect.x1) &&
                 ( k.roi.x2 == texRect.x2) &&
                 ( k.roi.y1 == texRect.y1) &&
                 ( k.roi.y2 == texRect.y2) ) {
                return;
            }
        }
    }

    if ( !k.roi.isNull() ) {
        TrackWatcherPtr watcher( new TrackWatcher() );
        QObject::connect( watcher.get(), SIGNAL(finished()), this, SLOT(onKeyFrameImageRenderingFinished()) );
        trackRequestsMap[k] = watcher;
        watcher->setFuture( QtConcurrent::run(track.get(), &TrackMarker::getMarkerImage, time, k.roi) );
    }
}

static unsigned int toBGRA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) WARN_UNUSED_RETURN;
unsigned int
toBGRA(unsigned char r,
       unsigned char g,
       unsigned char b,
       unsigned char a)
{
    return (a << 24) | (r << 16) | (g << 8) | b;
}

void
TrackerNodeInteract::convertImageTosRGBOpenGLTexture(const boost::shared_ptr<Image>& image,
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


    std::size_t bytesCount = 4 * sizeof(unsigned char) * roi.area();
    TextureRect region;
    region.x1 = roi.x1;
    region.x2 = roi.x2;
    region.y1 = roi.y1;
    region.y2 = roi.y2;

    GLint currentBoundPBO = 0;
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING_ARB, &currentBoundPBO);

    if (pboID == 0) {
        glGenBuffers(1, &pboID);
    }

    // bind PBO to update texture source
    glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, pboID );

    // Note that glMapBufferARB() causes sync issue.
    // If GPU is working with this buffer, glMapBufferARB() will wait(stall)
    // until GPU to finish its job. To avoid waiting (idle), you can call
    // first glBufferDataARB() with NULL pointer before glMapBufferARB().
    // If you do that, the previous data in PBO will be discarded and
    // glMapBufferARB() returns a new allocated pointer immediately
    // even if GPU is still working with the previous data.
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, bytesCount, NULL, GL_DYNAMIC_DRAW_ARB);

    // map the buffer object into client's memory
    GLvoid *buf = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    glCheckError();
    assert(buf);
    if (buf) {
        // update data directly on the mapped buffer
        if (!image) {
            int pixelsCount = roi.area();
            unsigned int* dstPixels = (unsigned int*)buf;
            for (int i = 0; i < pixelsCount; ++i, ++dstPixels) {
                *dstPixels = toBGRA(0, 0, 0, 255);
            }
        } else {
            int srcNComps = (int)image->getComponentsCount();
            assert(srcNComps >= 3);
            Image::ReadAccess acc( image.get() );
            const float* srcPixels = (const float*)acc.pixelAt(roi.x1, roi.y1);
            unsigned int* dstPixels = (unsigned int*)buf;
            assert(srcPixels);

            int w = roi.width();
            int srcRowElements = bounds.width() * srcNComps;
            const Color::Lut* lut = Color::LutManager::sRGBLut();
            lut->validate();
            assert(lut);

            unsigned char alpha = 255;

            for (int y = roi.y1; y < roi.y2; ++y, dstPixels += w, srcPixels += srcRowElements) {
                // coverity[dont_call]
                int start = (int)( rand() % (roi.x2 - roi.x1) );

                for (int backward = 0; backward < 2; ++backward) {
                    int index = backward ? start - 1 : start;
                    assert( backward == 1 || ( index >= 0 && index < (roi.x2 - roi.x1) ) );
                    unsigned error_r = 0x80;
                    unsigned error_g = 0x80;
                    unsigned error_b = 0x80;

                    while (index < w && index >= 0) {
                        float r = srcPixels[index * srcNComps];
                        float g = srcPixels[index * srcNComps + 1];
                        float b = srcPixels[index * srcNComps + 2];

                        error_r = (error_r & 0xff) + lut->toColorSpaceUint8xxFromLinearFloatFast(r);
                        error_g = (error_g & 0xff) + lut->toColorSpaceUint8xxFromLinearFloatFast(g);
                        error_b = (error_b & 0xff) + lut->toColorSpaceUint8xxFromLinearFloatFast(b);
                        assert(error_r < 0x10000 && error_g < 0x10000 && error_b < 0x10000);

                        dstPixels[index] = toBGRA( (U8)(error_r >> 8),
                                                  (U8)(error_g >> 8),
                                                  (U8)(error_b >> 8),
                                                  alpha );
                        
                        
                        if (backward) {
                            --index;
                        } else {
                            ++index;
                        }
                    }
                }
            }
        }
        
        GLboolean result = glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB); // release the mapped buffer
        assert(result == GL_TRUE);
        Q_UNUSED(result);
    }
    glCheckError();

    // copy pixels from PBO to texture object
    // using glBindTexture followed by glTexSubImage2D.
    // Use offset instead of pointer (last parameter is 0).
    tex->fillOrAllocateTexture(region, RectI(), false, 0);

    // restore previously bound PBO
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, currentBoundPBO);

    glCheckError();
} // TrackerNodeInteract::convertImageTosRGBOpenGLTexture

void
TrackerNodeInteract::onTrackingStarted(int step)
{
    isTracking = true;
    if (step > 0) {
        trackFwButton.lock()->setValue(true);
    } else {
        trackBwButton.lock()->setValue(true);
    }
}

void
TrackerNodeInteract::onTrackingEnded()
{
    trackBwButton.lock()->setValue(false);
    trackFwButton.lock()->setValue(false);
    isTracking = false;
    _p->publicInterface->redrawOverlayInteract();
}

void
TrackerNodeInteract::onContextSelectionChanged(int reason)
{
    std::list<TrackMarkerPtr > selection;

    getContext()->getSelectedMarkers(&selection);
    if ( selection.empty() || (selection.size() > 1) ) {
        showMarkerTexture = false;
    } else {
        assert(selection.size() == 1);

        const TrackMarkerPtr& selectionFront = selection.front();
        TrackMarkerPtr oldMarker = selectedMarker.lock();
        if (oldMarker != selectionFront) {
            selectedMarker = selectionFront;
            refreshSelectedMarkerTexture();


            std::set<int> keys;
            selectionFront->getUserKeyframes(&keys);
            for (std::set<int>::iterator it2 = keys.begin(); it2 != keys.end(); ++it2) {
                makeMarkerKeyTexture(*it2, selectionFront);
            }
        } else {
            if (selectionFront) {
                showMarkerTexture = true;
            }
        }
    }
    if ( (TrackerContext::TrackSelectionReason)reason == TrackerContext::eTrackSelectionViewer ) {
        return;
    }

    _p->publicInterface->redrawOverlayInteract();
}

void
TrackerNodeInteract::onTrackImageRenderingFinished()
{
    assert( QThread::currentThread() == qApp->thread() );
    QFutureWatcher<std::pair<boost::shared_ptr<Image>, RectI> >* future = dynamic_cast<QFutureWatcher<std::pair<boost::shared_ptr<Image>, RectI> >*>( sender() );
    assert(future);
    std::pair<boost::shared_ptr<Image>, RectI> ret = future->result();
    OverlaySupport* overlay = _p->publicInterface->getCurrentViewportForOverlays();
    assert(overlay);
    OpenGLViewerI* isOpenGLViewer = dynamic_cast<OpenGLViewerI*>(overlay);
    assert(isOpenGLViewer);
    if (!isOpenGLViewer) {
        return;
    }
    if (!ret.first) {
        return;
    }
    isOpenGLViewer->makeOpenGLcontextCurrent();

    showMarkerTexture = true;
    if (!selectedMarkerTexture) {
        int format, internalFormat, glType;
        Texture::getRecommendedTexParametersForRGBAByteTexture(&format, &internalFormat, &glType);
        selectedMarkerTexture = boost::make_shared<Texture>(GL_TEXTURE_2D, GL_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE, Texture::eDataTypeByte,
                                                            format, internalFormat, glType);
    }
    selectedMarkerTextureTime = (int)ret.first->getTime();
    selectedMarkerTextureRoI = ret.second;

    convertImageTosRGBOpenGLTexture(ret.first, selectedMarkerTexture, ret.second);

    _p->publicInterface->redrawOverlayInteract();
}

void
TrackerNodeInteract::onKeyFrameImageRenderingFinished()
{
    assert( QThread::currentThread() == qApp->thread() );
    TrackWatcher* future = dynamic_cast<TrackWatcher*>( sender() );
    assert(future);
    std::pair<boost::shared_ptr<Image>, RectI> ret = future->result();
    if ( !ret.first || ret.second.isNull() ) {
        return;
    }

    OverlaySupport* overlay = _p->publicInterface->getCurrentViewportForOverlays();
    assert(overlay);
    OpenGLViewerI* isOpenGLViewer = dynamic_cast<OpenGLViewerI*>(overlay);
    assert(isOpenGLViewer);
    if (!isOpenGLViewer) {
        return;
    }

    isOpenGLViewer->makeOpenGLcontextCurrent();

    for (TrackKeyframeRequests::iterator it = trackRequestsMap.begin(); it != trackRequestsMap.end(); ++it) {
        if (it->second.get() == future) {
            TrackMarkerPtr track = it->first.track.lock();
            if (!track) {
                return;
            }
            TrackerNodeInteract::KeyFrameTexIDs& keyTextures = trackTextures[track];
            int format, internalFormat, glType;
            Texture::getRecommendedTexParametersForRGBAByteTexture(&format, &internalFormat, &glType);
            GLTexturePtr tex( new Texture(GL_TEXTURE_2D, GL_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE, Texture::eDataTypeByte,
                                          format, internalFormat, glType) );
            keyTextures[it->first.time] = tex;
            convertImageTosRGBOpenGLTexture(ret.first, tex, ret.second);

            trackRequestsMap.erase(it);

            _p->publicInterface->redrawOverlayInteract();

            return;
        }
    }
    assert(false);
}

void
TrackerNodeInteract::rebuildMarkerTextures()
{
    ///Refreh textures for all markers
    std::list<TrackMarkerPtr > markers;

    getContext()->getSelectedMarkers(&markers);
    for (std::list<TrackMarkerPtr >::iterator it = markers.begin(); it != markers.end(); ++it) {
        std::set<int> keys;
        (*it)->getUserKeyframes(&keys);
        for (std::set<int>::iterator it2 = keys.begin(); it2 != keys.end(); ++it2) {
            makeMarkerKeyTexture(*it2, *it);
        }
    }
    onContextSelectionChanged(TrackerContext::eTrackSelectionInternal);
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
    std::list< TrackMarkerPtr > markers;

    getContext()->getSelectedMarkers(&markers);

    if ( !markers.empty() ) {
        std::pair<double, double> pixelScale;
        _p->publicInterface->getCurrentViewportForOverlays()->getPixelScale(pixelScale.first, pixelScale.second);
        double time = _p->publicInterface->getCurrentTime();
        bool createkey = createKeyOnMoveButton.lock()->getValue();

        int hasMovedMarker = false;
        for (std::list< TrackMarkerPtr >::iterator it = markers.begin(); it != markers.end(); ++it) {

            if (!(*it)->isEnabled(time)) {
                continue;
            }
            boost::shared_ptr<KnobDouble> centerKnob = (*it)->getCenterKnob();
            boost::shared_ptr<KnobDouble> patternCorners[4];
            patternCorners[0] = (*it)->getPatternBtmLeftKnob();
            patternCorners[1] = (*it)->getPatternTopLeftKnob();
            patternCorners[2] = (*it)->getPatternTopRightKnob();
            patternCorners[3] = (*it)->getPatternBtmRightKnob();

            centerKnob->setValuesAtTime(time, centerKnob->getValueAtTime(time, 0) + x,
                                        centerKnob->getValueAtTime(time, 1) + y,
                                        ViewSpec::all(),
                                        eValueChangedReasonPluginEdited);
            for (int i = 0; i < 4; ++i) {
                for (int d = 0; d < patternCorners[i]->getDimension(); ++d) {
                    patternCorners[i]->setValueAtTime(time, patternCorners[i]->getValueAtTime(time, d), ViewSpec::all(), d);
                }
            }
            if (createkey) {
                (*it)->setUserKeyframe(time);
            }
            hasMovedMarker = true;
        }
        refreshSelectedMarkerTexture();

        return hasMovedMarker;
    }

    return false;
}

void
TrackerNodeInteract::transformPattern(double time,
                                      TrackerMouseStateEnum state,
                                      const Point& delta)
{
    boost::shared_ptr<KnobDouble> searchWndTopRight, searchWndBtmLeft;
    boost::shared_ptr<KnobDouble> patternCorners[4];
    boost::shared_ptr<TrackerContext> context = getContext();
    boost::shared_ptr<KnobDouble> centerKnob = interactMarker->getCenterKnob();
    boost::shared_ptr<KnobDouble> offsetKnob = interactMarker->getOffsetKnob();
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
    centerPoint.rx() = centerKnob->getValueAtTime(time, 0);
    centerPoint.ry() = centerKnob->getValueAtTime(time, 1);

    QPointF offset;
    offset.rx() = offsetKnob->getValueAtTime(time, 0);
    offset.ry() = offsetKnob->getValueAtTime(time, 1);

    QPointF patternPoints[4];
    QPointF searchPoints[4];
    if (transformPatternCorners) {
        for (int i = 0; i < 4; ++i) {
            patternPoints[i].rx() = patternCorners[i]->getValueAtTime(time, 0) + centerPoint.x() + offset.x();
            patternPoints[i].ry() = patternCorners[i]->getValueAtTime(time, 1) + centerPoint.y() + offset.y();
        }
    }
    searchPoints[1].rx() = searchWndBtmLeft->getValueAtTime(time, 0) + centerPoint.x() + offset.x();
    searchPoints[1].ry() = searchWndBtmLeft->getValueAtTime(time, 1) + centerPoint.y() + offset.y();

    searchPoints[3].rx() = searchWndTopRight->getValueAtTime(time, 0) + centerPoint.x() + offset.x();
    searchPoints[3].ry() = searchWndTopRight->getValueAtTime(time, 1) + centerPoint.y() + offset.y();

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

    EffectInstancePtr effect = context->getNode()->getEffectInstance();
    effect->beginChanges();

    if (transformPatternCorners) {
        for (int i = 0; i < 4; ++i) {
            patternPoints[i].rx() -= ( centerPoint.x() + offset.x() );
            patternPoints[i].ry() -= ( centerPoint.y() + offset.y() );


            if ( patternCorners[i]->hasAnimation() ) {
                patternCorners[i]->setValuesAtTime(time, patternPoints[i].x(), patternPoints[i].y(), ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
            } else {
                patternCorners[i]->setValues(patternPoints[i].x(), patternPoints[i].y(), ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
            }
        }
    }
    searchPoints[1].rx() -= ( centerPoint.x() + offset.x() );
    searchPoints[1].ry() -= ( centerPoint.y() + offset.y() );

    searchPoints[3].rx() -= ( centerPoint.x() + offset.x() );
    searchPoints[3].ry() -= ( centerPoint.y() + offset.y() );

    if ( searchWndBtmLeft->hasAnimation() ) {
        searchWndBtmLeft->setValuesAtTime(time, searchPoints[1].x(), searchPoints[1].y(), ViewSpec::current(),  eValueChangedReasonNatronInternalEdited);
    } else {
        searchWndBtmLeft->setValues(searchPoints[1].x(), searchPoints[1].y(), ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
    }

    if ( searchWndTopRight->hasAnimation() ) {
        searchWndTopRight->setValuesAtTime(time, searchPoints[3].x(), searchPoints[3].y(), ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
    } else {
        searchWndTopRight->setValues(searchPoints[3].x(), searchPoints[3].y(), ViewSpec::current(), eValueChangedReasonNatronInternalEdited);
    }
    effect->endChanges();

    refreshSelectedMarkerTexture();

    if ( createKeyOnMoveButton.lock()->getValue() ) {
        interactMarker->setUserKeyframe(time);
    }
} // TrackerNodeInteract::transformPattern

void
TrackerNodeInteract::onKeyframeSetOnTrack(const TrackMarkerPtr& marker,
                                          int key)
{
    makeMarkerKeyTexture(key, marker);
}

void
TrackerNodeInteract::onKeyframeRemovedOnTrack(const TrackMarkerPtr& marker,
                                              int key)
{
    for (TrackerNodeInteract::TrackKeysMap::iterator it = trackTextures.begin(); it != trackTextures.end(); ++it) {
        if (it->first.lock() == marker) {
            std::map<int, boost::shared_ptr<Texture> >::iterator found = it->second.find(key);
            if ( found != it->second.end() ) {
                it->second.erase(found);
            }
            break;
        }
    }
    _p->publicInterface->redrawOverlayInteract();
}

void
TrackerNodeInteract::onAllKeyframesRemovedOnTrack(const TrackMarkerPtr& marker)
{
    for (TrackerNodeInteract::TrackKeysMap::iterator it = trackTextures.begin(); it != trackTextures.end(); ++it) {
        if (it->first.lock() == marker) {
            it->second.clear();
            break;
        }
    }
    _p->publicInterface->redrawOverlayInteract();
}

NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING
#include "moc_TrackerNodeInteract.cpp"
