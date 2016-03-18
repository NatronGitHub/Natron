/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "TrackerGui.h"

#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QApplication>
#include <QThread>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QKeyEvent>
#include <QPixmap>
#include <QIcon>
#include <QtConcurrentRun>
#include <QFutureWatcher>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/AppInstance.h"
#include "Engine/Curve.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Image.h"
#include "Engine/Lut.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerContext.h"

#include "Engine/ViewIdx.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/Button.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/Gui.h"
#include "Gui/TrackerUndoCommand.h"
#include "Gui/Texture.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/NodeGui.h"
#include "Gui/TrackerPanel.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/QtEnumConvert.h"
#include "Gui/Utils.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

#define POINT_SIZE 5
#define CROSS_SIZE 6
#define POINT_TOLERANCE 6
#define ADDTRACK_SIZE 5
#define HANDLE_SIZE 6

//Controls how many center keyframes should be displayed before and after the time displayed
#define MAX_CENTER_POINTS_DISPLAYED 20

#define SELECTED_MARKER_WINDOW_BASE_WIDTH_SCREEN_PX 200

#define SELECTED_MARKER_KEYFRAME_WIDTH_SCREEN_PX 75
#define MAX_TRACK_KEYS_TO_DISPLAY 10

NATRON_NAMESPACE_ENTER;


enum TrackerMouseStateEnum
{
    eMouseStateIdle = 0,
    eMouseStateDraggingCenter,
    eMouseStateDraggingOffset,
    
    eMouseStateDraggingInnerTopLeft,
    eMouseStateDraggingInnerTopRight,
    eMouseStateDraggingInnerBtmLeft,
    eMouseStateDraggingInnerBtmRight,
    eMouseStateDraggingInnerTopMid,
    eMouseStateDraggingInnerMidRight,
    eMouseStateDraggingInnerBtmMid,
    eMouseStateDraggingInnerMidLeft,
    
    eMouseStateDraggingOuterTopLeft,
    eMouseStateDraggingOuterTopRight,
    eMouseStateDraggingOuterBtmLeft,
    eMouseStateDraggingOuterBtmRight,
    eMouseStateDraggingOuterTopMid,
    eMouseStateDraggingOuterMidRight,
    eMouseStateDraggingOuterBtmMid,
    eMouseStateDraggingOuterMidLeft,
    
    eMouseStateDraggingSelectedMarkerResizeAnchor,
    eMouseStateScalingSelectedMarker,
    eMouseStateDraggingSelectedMarker,
};

enum TrackerDrawStateEnum
{
    eDrawStateInactive = 0,
    eDrawStateHoveringCenter,
    
    eDrawStateHoveringInnerTopLeft,
    eDrawStateHoveringInnerTopRight,
    eDrawStateHoveringInnerBtmLeft,
    eDrawStateHoveringInnerBtmRight,
    eDrawStateHoveringInnerTopMid,
    eDrawStateHoveringInnerMidRight,
    eDrawStateHoveringInnerBtmMid,
    eDrawStateHoveringInnerMidLeft,
    
    eDrawStateHoveringOuterTopLeft,
    eDrawStateHoveringOuterTopRight,
    eDrawStateHoveringOuterBtmLeft,
    eDrawStateHoveringOuterBtmRight,
    eDrawStateHoveringOuterTopMid,
    eDrawStateHoveringOuterMidRight,
    eDrawStateHoveringOuterBtmMid,
    eDrawStateHoveringOuterMidLeft,
    
    eDrawStateShowScalingHint,
};

typedef QFutureWatcher<std::pair<boost::shared_ptr<Natron::Image>,RectI> > TrackWatcher;
typedef boost::shared_ptr<TrackWatcher> TrackWatcherPtr;

struct TrackRequestKey {
    int time;
    boost::weak_ptr<TrackMarker> track;
    RectI roi;
};

struct TrackRequestKey_compareLess
{
    bool operator()(const TrackRequestKey& lhs, const TrackRequestKey& rhs) const
    {
        if (lhs.time < rhs.time) {
            return true;
        } else if (lhs.time > rhs.time) {
            return false;
        } else {
            boost::shared_ptr<TrackMarker> lptr = lhs.track.lock();
            boost::shared_ptr<TrackMarker> rptr = rhs.track.lock();
            if (lptr.get() < rptr.get()) {
                return true;
            } else if (lptr.get() > rptr.get()) {
                return false;
            } else {
                double la = lhs.roi.area();
                double ra = rhs.roi.area();
                if (la < ra) {
                    return true;
                } else if (la > ra) {
                    return false;
                } else {
                    return false;
                }
            }
        }
    }
};

typedef std::map<TrackRequestKey, TrackWatcherPtr, TrackRequestKey_compareLess> TrackKeyframeRequests;


struct TrackerGuiPrivate
{
    TrackerGui* _publicInterface;
    boost::shared_ptr<TrackerPanelV1> panelv1;
    TrackerPanel* panel;
    ViewerTab* viewer;
    QWidget* buttonsBar;
    QHBoxLayout* buttonsLayout;
    Button* addTrackButton;
    Button* trackBwButton;
    Button* trackPrevButton;
    Button* stopTrackingButton;
    Button* trackNextButton;
    Button* trackFwButton;
    Button* clearAllAnimationButton;
    Button* clearBwAnimationButton;
    Button* clearFwAnimationButton;
    Button* updateViewerButton;
    Button* centerViewerButton;
    
    Button* createKeyOnMoveButton;
    Button* setKeyFrameButton;
    Button* removeKeyFrameButton;
    Button* resetOffsetButton;
    Button* resetTrackButton;
    Button* showCorrelationButton;

    
    bool clickToAddTrackEnabled;
    QPointF lastMousePos;
    QRectF selectionRectangle;
    int controlDown;
    int shiftDown;
    
    TrackerMouseStateEnum eventState;
    TrackerDrawStateEnum hoverState;
    
    boost::shared_ptr<TrackMarker> interactMarker,hoverMarker;
    
    typedef std::map<int,boost::shared_ptr<Texture> > KeyFrameTexIDs;
    typedef std::map<boost::weak_ptr<TrackMarker>, KeyFrameTexIDs> TrackKeysMap;
    TrackKeysMap trackTextures;
    TrackKeyframeRequests trackRequestsMap;
    
    boost::shared_ptr<Texture> selectedMarkerTexture;
    RectI selectedMarkerTextureRoI;
    //If theres a single selection, this points to it
    boost::weak_ptr<TrackMarker> selectedMarker;
    GLuint pboID;
    int selectedMarkerWidth;
    TrackWatcherPtr imageGetterWatcher;
    bool showMarkerTexture;
    RenderScale selectedMarkerScale;
    boost::weak_ptr<Image> selectedMarkerImg;
    
    bool isTracking;
    
    TrackerGuiPrivate(TrackerGui* publicInterface,
                      const boost::shared_ptr<TrackerPanelV1> & panelv1,
                      TrackerPanel* panel,
                      ViewerTab* parent)
    : _publicInterface(publicInterface)
    , panelv1(panelv1)
    , panel(panel)
    , viewer(parent)
    , buttonsBar(NULL)
    , buttonsLayout(NULL)
    , addTrackButton(NULL)
    , trackBwButton(NULL)
    , trackPrevButton(NULL)
    , stopTrackingButton(NULL)
    , trackNextButton(NULL)
    , trackFwButton(NULL)
    , clearAllAnimationButton(NULL)
    , clearBwAnimationButton(NULL)
    , clearFwAnimationButton(NULL)
    , updateViewerButton(NULL)
    , centerViewerButton(NULL)
    , createKeyOnMoveButton(0)
    , setKeyFrameButton(0)
    , removeKeyFrameButton(0)
    , resetOffsetButton(0)
    , resetTrackButton(0)
    , showCorrelationButton(0)
    , clickToAddTrackEnabled(false)
    , lastMousePos()
    , selectionRectangle()
    , controlDown(0)
    , shiftDown(0)
    , eventState(eMouseStateIdle)
    , hoverState(eDrawStateInactive)
    , interactMarker()
    , trackTextures()
    , trackRequestsMap()
    , selectedMarkerTexture()
    , selectedMarkerTextureRoI()
    , selectedMarker()
    , pboID(0)
    , selectedMarkerWidth(SELECTED_MARKER_WINDOW_BASE_WIDTH_SCREEN_PX)
    , imageGetterWatcher()
    , showMarkerTexture(false)
    , selectedMarkerScale()
    , selectedMarkerImg()
    , isTracking(false)
    {
        glGenBuffers(1, &pboID);
        selectedMarkerScale.x = selectedMarkerScale.y = 1.;
    }
    
    ~TrackerGuiPrivate()
    {
        glDeleteBuffers(1, &pboID);
    }
    
    void transformPattern(double time, TrackerMouseStateEnum state, const Natron::Point& delta);
    
    void refreshSelectedMarkerTexture();
    
    void convertImageTosRGBOpenGLTexture(const boost::shared_ptr<Image>& image,const boost::shared_ptr<Texture>& tex, const RectI& renderWindow);
    
    void makeMarkerKeyTexture(int time, const boost::shared_ptr<TrackMarker>& track);
    
    void drawSelectedMarkerTexture(const std::pair<double,double>& pixelScale,
                                   int currentTime,
                                   const Natron::Point& selectedCenter,
                                   const Natron::Point& offset,
                                   const Natron::Point& selectedPtnTopLeft,
                                   const Natron::Point& selectedPtnTopRight,
                                   const Natron::Point& selectedPtnBtmRight,
                                   const Natron::Point& selectedPtnBtmLeft,
                                   const Natron::Point& selectedSearchWndBtmLeft,
                                   const Natron::Point& selectedSearchWndTopRight);
    
    void drawSelectedMarkerKeyframes(const std::pair<double,double>& pixelScale, int currentTime);
    
    ///Filter allkeys so only that only the MAX_TRACK_KEYS_TO_DISPLAY surrounding are displayed
    static KeyFrameTexIDs getKeysToRenderForMarker(double currentTime, const KeyFrameTexIDs& allKeys);
    
    void computeTextureCanonicalRect(const Texture& tex, int xOffset, int texWidthPx, RectD* rect) const;
    void computeSelectedMarkerCanonicalRect(RectD* rect) const;
    bool isNearbySelectedMarkerTextureResizeAnchor(const QPointF& pos) const;
    bool isInsideSelectedMarkerTextureResizeAnchor(const QPointF& pos) const;
    
    ///Returns the keyframe time if the mouse is inside a keyframe texture
    int isInsideKeyFrameTexture(double currentTime, const QPointF& pos, const QPointF& viewportPos) const;
    
};

class DuringOverlayFlag_RAII
{
    EffectInstPtr e;
public:
    
    DuringOverlayFlag_RAII(TrackerGuiPrivate* p)
    : e()
    {
        
        if (p->panel) {
            e = p->panel->getNode()->getNode()->getEffectInstance();
        }
        if (e) {
            e->setDoingInteractAction(true);
        }
    }
    ~DuringOverlayFlag_RAII()
    {
        if (e) {
            e->setDoingInteractAction(false);
        }
    }
};

#define FLAG_DURING_INTERACT DuringOverlayFlag_RAII __flag_setter__(_imp.get());

TrackerGui::TrackerGui(TrackerPanel* panel,
           ViewerTab* parent)
: QObject()
, _imp(new TrackerGuiPrivate(this, boost::shared_ptr<TrackerPanelV1>(), panel, parent))

{
    createGui();
}

TrackerGui::TrackerGui(const boost::shared_ptr<TrackerPanelV1> & panel,
                       ViewerTab* parent)
: QObject()
, _imp(new TrackerGuiPrivate(this, panel, 0, parent))
{
    createGui();
}


void
TrackerGui::createGui()
{
    assert(_imp->viewer);
    
    QObject::connect( _imp->viewer->getViewer(),SIGNAL(selectionRectangleChanged(bool)),this,SLOT(updateSelectionFromSelectionRectangle(bool)));
    QObject::connect( _imp->viewer->getViewer(), SIGNAL(selectionCleared()), this, SLOT(onSelectionCleared()));
    
    if (_imp->panelv1) {
        QObject::connect(_imp->panelv1.get(), SIGNAL(trackingEnded()), this, SLOT(onTrackingEnded()));
    }
    
    _imp->buttonsBar = new QWidget(_imp->viewer);
    _imp->buttonsLayout = new QHBoxLayout(_imp->buttonsBar);
    _imp->buttonsLayout->setContentsMargins(3, 2, 0, 0);

    
    const double iconSizeX = TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE);
    
    QPixmap pixAdd;

    appPTR->getIcon(NATRON_PIXMAP_ADD_TRACK,iconSizeX, &pixAdd);

    const QSize medButtonSize(TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE),TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE));
    const QSize medButtonIconSize(TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE),TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE));
    
    _imp->addTrackButton = new Button(QIcon(pixAdd),QString(),_imp->buttonsBar);
    _imp->addTrackButton->setCheckable(true);
    _imp->addTrackButton->setChecked(false);
    _imp->addTrackButton->setFixedSize(medButtonSize);
    _imp->addTrackButton->setIconSize(medButtonIconSize);
    _imp->addTrackButton->setToolTip(GuiUtils::convertFromPlainText(tr("When enabled you can add new tracks "
                                                                 "by clicking on the Viewer. "
                                                                 "Holding the Control + Alt keys is the "
                                                                 "same as pressing this button."),
                                                              Qt::WhiteSpaceNormal) );

    _imp->buttonsLayout->addWidget(_imp->addTrackButton);
    QObject::connect( _imp->addTrackButton, SIGNAL(clicked(bool)), this, SLOT(onAddTrackClicked(bool)) );
    QPixmap pixPrev,pixNext,pixClearAll,pixClearBw,pixClearFw,pixUpdateViewerEnabled,pixUpdateViewerDisabled,pixStop;
    QPixmap bwEnabled,bwDisabled,fwEnabled,fwDisabled;

    appPTR->getIcon(NATRON_PIXMAP_PLAYER_REWIND_DISABLED,iconSizeX, &bwDisabled);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_REWIND_ENABLED, iconSizeX,&bwEnabled);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PREVIOUS, iconSizeX,&pixPrev);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_NEXT, iconSizeX,&pixNext);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PLAY_DISABLED, iconSizeX,&fwDisabled);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PLAY_ENABLED, iconSizeX,&fwEnabled);
    appPTR->getIcon(NATRON_PIXMAP_CLEAR_ALL_ANIMATION, iconSizeX,&pixClearAll);
    appPTR->getIcon(NATRON_PIXMAP_CLEAR_BACKWARD_ANIMATION, iconSizeX,&pixClearBw);
    appPTR->getIcon(NATRON_PIXMAP_CLEAR_FORWARD_ANIMATION, iconSizeX,&pixClearFw);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_REFRESH_ACTIVE, iconSizeX,&pixUpdateViewerEnabled);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_REFRESH, iconSizeX,&pixUpdateViewerDisabled);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_STOP_ENABLED, iconSizeX,&pixStop);



    
    QIcon bwIcon;
    bwIcon.addPixmap(pixStop,QIcon::Normal,QIcon::On);
    bwIcon.addPixmap(bwDisabled,QIcon::Normal,QIcon::Off);
    
    QWidget* trackPlayer = new QWidget(_imp->buttonsBar);
    QHBoxLayout* trackPlayerLayout = new QHBoxLayout(trackPlayer);
    trackPlayerLayout->setContentsMargins(0, 0, 0, 0);
    trackPlayerLayout->setSpacing(0);
    
    _imp->trackBwButton = new Button(bwIcon,QString(),_imp->buttonsBar);
    _imp->trackBwButton->setFixedSize(medButtonSize);
    _imp->trackBwButton->setIconSize(medButtonIconSize);
    _imp->trackBwButton->setToolTip(QString::fromUtf8("<p>") + tr("Track selected tracks backward until left bound of the timeline.") +
                                    QString::fromUtf8("</p><p><b>") + tr("Keyboard shortcut:") + QString::fromUtf8(" Z</b></p>"));
    _imp->trackBwButton->setCheckable(true);
    _imp->trackBwButton->setChecked(false);
    QObject::connect( _imp->trackBwButton,SIGNAL( clicked(bool) ),this,SLOT( onTrackBwClicked() ) );
    trackPlayerLayout->addWidget(_imp->trackBwButton);
    
    _imp->trackPrevButton = new Button(QIcon(pixPrev),QString(),_imp->buttonsBar);
    _imp->trackPrevButton->setFixedSize(medButtonSize);
    _imp->trackPrevButton->setIconSize(medButtonIconSize);
    _imp->trackPrevButton->setToolTip(QString::fromUtf8("<p>") + tr("Track selected tracks on the previous frame.") +
                                      QString::fromUtf8("</p><p><b>") + tr("Keyboard shortcut:") + QString::fromUtf8(" X</b></p>"));
    QObject::connect( _imp->trackPrevButton,SIGNAL( clicked(bool) ),this,SLOT( onTrackPrevClicked() ) );
    trackPlayerLayout->addWidget(_imp->trackPrevButton);
    
    _imp->stopTrackingButton = new Button(QIcon(pixStop),QString(),_imp->buttonsBar);
    _imp->stopTrackingButton->setFixedSize(medButtonSize);
    _imp->stopTrackingButton->setIconSize(medButtonIconSize);
    _imp->stopTrackingButton->setToolTip(QString::fromUtf8("<p>") + tr("Stop the ongoing tracking if any")  +
                                         QString::fromUtf8("</p><p><b>") + tr("Keyboard shortcut:") + QString::fromUtf8(" Escape</b></p>"));
    QObject::connect( _imp->stopTrackingButton,SIGNAL( clicked(bool) ),this,SLOT( onStopButtonClicked() ) );
    trackPlayerLayout->addWidget(_imp->stopTrackingButton);
    
    _imp->trackNextButton = new Button(QIcon(pixNext),QString(),_imp->buttonsBar);
    _imp->trackNextButton->setFixedSize(medButtonSize);
    _imp->trackNextButton->setIconSize(medButtonIconSize);
    _imp->trackNextButton->setToolTip(QString::fromUtf8("<p>") + tr("Track selected tracks on the next frame.") +
                                      QString::fromUtf8("</p><p><b>") + tr("Keyboard shortcut:") + QString::fromUtf8(" C</b></p>"));
    QObject::connect( _imp->trackNextButton,SIGNAL( clicked(bool) ),this,SLOT( onTrackNextClicked() ) );
    trackPlayerLayout->addWidget(_imp->trackNextButton);
    
    
    QIcon fwIcon;
    fwIcon.addPixmap(pixStop,QIcon::Normal,QIcon::On);
    fwIcon.addPixmap(fwDisabled,QIcon::Normal,QIcon::Off);
    _imp->trackFwButton = new Button(fwIcon,QString(),_imp->buttonsBar);
    _imp->trackFwButton->setFixedSize(medButtonSize);
    _imp->trackFwButton->setIconSize(medButtonIconSize);
    _imp->trackFwButton->setToolTip(QString::fromUtf8("<p>") + tr("Track selected tracks forward until right bound of the timeline.") +
                                    QString::fromUtf8("</p><p><b>") + tr("Keyboard shortcut:") + QString::fromUtf8(" V</b></p>"));
    _imp->trackFwButton->setCheckable(true);
    _imp->trackFwButton->setChecked(false);
    QObject::connect( _imp->trackFwButton,SIGNAL(clicked(bool)),this,SLOT(onTrackFwClicked()) );
    trackPlayerLayout->addWidget(_imp->trackFwButton);
    
    _imp->buttonsLayout->addWidget(trackPlayer);


#pragma message WARN("Add a button to track between keyframes surrounding the current frame")

    QWidget* clearAnimationContainer = new QWidget(_imp->buttonsBar);
    QHBoxLayout* clearAnimationLayout = new QHBoxLayout(clearAnimationContainer);
    clearAnimationLayout->setContentsMargins(0, 0, 0, 0);
    clearAnimationLayout->setSpacing(0);
    
    _imp->clearAllAnimationButton = new Button(QIcon(pixClearAll),QString(),_imp->buttonsBar);
    _imp->clearAllAnimationButton->setFixedSize(medButtonSize);
    _imp->clearAllAnimationButton->setIconSize(medButtonIconSize);
    _imp->clearAllAnimationButton->setToolTip(GuiUtils::convertFromPlainText(tr("Clear all animation for selected tracks."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->clearAllAnimationButton,SIGNAL(clicked(bool)),this,SLOT(onClearAllAnimationClicked()) );
    clearAnimationLayout->addWidget(_imp->clearAllAnimationButton);
    
    _imp->clearBwAnimationButton = new Button(QIcon(pixClearBw),QString(),_imp->buttonsBar);
    _imp->clearBwAnimationButton->setFixedSize(medButtonSize);
    _imp->clearBwAnimationButton->setIconSize(medButtonIconSize);
    _imp->clearBwAnimationButton->setToolTip(GuiUtils::convertFromPlainText(tr("Clear animation backward from the current frame."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->clearBwAnimationButton,SIGNAL(clicked(bool)),this,SLOT(onClearBwAnimationClicked()) );
    clearAnimationLayout->addWidget(_imp->clearBwAnimationButton);
    
    _imp->clearFwAnimationButton = new Button(QIcon(pixClearFw),QString(),_imp->buttonsBar);
    _imp->clearFwAnimationButton->setFixedSize(medButtonSize);
    _imp->clearFwAnimationButton->setIconSize(medButtonIconSize);
    _imp->clearFwAnimationButton->setToolTip(GuiUtils::convertFromPlainText(tr("Clear animation forward from the current frame."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->clearFwAnimationButton,SIGNAL(clicked(bool)),this,SLOT(onClearFwAnimationClicked()) );
    clearAnimationLayout->addWidget(_imp->clearFwAnimationButton);
    
    _imp->buttonsLayout->addWidget(clearAnimationContainer);
    
    QIcon updateViewerIC;
    updateViewerIC.addPixmap(pixUpdateViewerEnabled,QIcon::Normal,QIcon::On);
    updateViewerIC.addPixmap(pixUpdateViewerDisabled,QIcon::Normal,QIcon::Off);
    _imp->updateViewerButton = new Button(updateViewerIC,QString(),_imp->buttonsBar);
    _imp->updateViewerButton->setFixedSize(medButtonSize);
    _imp->updateViewerButton->setIconSize(medButtonIconSize);

    _imp->updateViewerButton->setCheckable(true);
    _imp->updateViewerButton->setChecked(true);
    _imp->updateViewerButton->setDown(true);
    _imp->updateViewerButton->setToolTip(GuiUtils::convertFromPlainText(tr("Update viewer during tracking for each frame instead of just the tracks."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->updateViewerButton,SIGNAL(clicked(bool)),this,SLOT(onUpdateViewerClicked(bool)) );
    _imp->buttonsLayout->addWidget(_imp->updateViewerButton);

    QPixmap centerViewerPix;
    appPTR->getIcon(Natron::NATRON_PIXMAP_CENTER_VIEWER_ON_TRACK, &centerViewerPix);
    
    _imp->centerViewerButton = new Button(QIcon(centerViewerPix),QString(),_imp->buttonsBar);
    _imp->centerViewerButton->setFixedSize(medButtonSize);
    _imp->centerViewerButton->setIconSize(medButtonIconSize);
    _imp->centerViewerButton->setCheckable(true);
    _imp->centerViewerButton->setChecked(false);
    _imp->centerViewerButton->setDown(false);
    _imp->centerViewerButton->setToolTip(GuiUtils::convertFromPlainText(tr("Center the viewer on selected tracks during tracking."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->centerViewerButton,SIGNAL( clicked(bool) ),this,SLOT( onCenterViewerButtonClicked(bool) ) );
    _imp->buttonsLayout->addWidget(_imp->centerViewerButton);

    
    if (_imp->panel) {
        /// This is for v2 only
        
        boost::shared_ptr<TrackerContext> context = _imp->panel->getContext();
        boost::shared_ptr<TimeLine> timeline = _imp->panel->getNode()->getNode()->getApp()->getTimeLine();
        QObject::connect(timeline.get(), SIGNAL(frameChanged(SequenceTime,int)), this, SLOT(onTimelineTimeChanged(SequenceTime,int)));
    
        assert(context);
        QObject::connect(context.get(), SIGNAL(selectionChanged(int)), this , SLOT(onContextSelectionChanged(int)));
        QObject::connect(context.get(), SIGNAL(keyframeSetOnTrack(boost::shared_ptr<TrackMarker>,int)), this , SLOT(onKeyframeSetOnTrack(boost::shared_ptr<TrackMarker>,int)));
        QObject::connect(context.get(), SIGNAL(keyframeRemovedOnTrack(boost::shared_ptr<TrackMarker>,int)), this , SLOT(onKeyframeRemovedOnTrack(boost::shared_ptr<TrackMarker>,int)));
        QObject::connect(context.get(), SIGNAL(allKeyframesRemovedOnTrack(boost::shared_ptr<TrackMarker>)), this , SLOT(onAllKeyframesRemovedOnTrack(boost::shared_ptr<TrackMarker>)));
        QObject::connect(context.get(), SIGNAL(onNodeInputChanged(int)), this , SLOT(onTrackerInputChanged(int)));
        QObject::connect(context.get(), SIGNAL(trackingFinished()), this , SLOT(onTrackingEnded()));
        QObject::connect(context.get(), SIGNAL(trackingStarted()), this, SLOT(onTrackingStarted()));
        
        QPixmap addKeyOnPix,addKeyOffPix;
        QIcon addKeyIc;
        appPTR->getIcon(Natron::NATRON_PIXMAP_CREATE_USER_KEY_ON_MOVE_ON, &addKeyOnPix);
        appPTR->getIcon(Natron::NATRON_PIXMAP_CREATE_USER_KEY_ON_MOVE_OFF, &addKeyOffPix);
        addKeyIc.addPixmap(addKeyOnPix, QIcon::Normal, QIcon::On);
        addKeyIc.addPixmap(addKeyOffPix, QIcon::Normal, QIcon::Off);
        
        _imp->createKeyOnMoveButton = new Button(addKeyIc, QString(), _imp->buttonsBar);
        _imp->createKeyOnMoveButton->setFixedSize(medButtonSize);
        _imp->createKeyOnMoveButton->setIconSize(medButtonIconSize);
        _imp->createKeyOnMoveButton->setToolTip(GuiUtils::convertFromPlainText(tr("When enabled, adjusting a track on the viewer will create a new keyframe"), Qt::WhiteSpaceNormal));
        _imp->createKeyOnMoveButton->setCheckable(true);
        _imp->createKeyOnMoveButton->setChecked(true);
        _imp->createKeyOnMoveButton->setDown(true);
        QObject::connect( _imp->createKeyOnMoveButton,SIGNAL( clicked(bool) ),this,SLOT( onCreateKeyOnMoveButtonClicked(bool) ) );
        _imp->buttonsLayout->addWidget(_imp->createKeyOnMoveButton);
        
        QPixmap showCorrPix,hideCorrPix;
        appPTR->getIcon(Natron::NATRON_PIXMAP_SHOW_TRACK_ERROR, &showCorrPix);
        appPTR->getIcon(Natron::NATRON_PIXMAP_HIDE_TRACK_ERROR, &hideCorrPix);
        QIcon corrIc;
        corrIc.addPixmap(showCorrPix, QIcon::Normal, QIcon::On);
        corrIc.addPixmap(hideCorrPix, QIcon::Normal, QIcon::Off);
        _imp->showCorrelationButton = new Button(corrIc, QString(), _imp->buttonsBar);
        _imp->showCorrelationButton->setFixedSize(medButtonSize);
        _imp->showCorrelationButton->setIconSize(medButtonIconSize);
        _imp->showCorrelationButton->setToolTip(GuiUtils::convertFromPlainText(tr("When enabled, the correlation score of each tracked frame will be displayed on "
                                                                                "the viewer, with lower correlations close to green and greater correlations "
                                                                                "close to red."), Qt::WhiteSpaceNormal));
        _imp->showCorrelationButton->setCheckable(true);
        _imp->showCorrelationButton->setChecked(false);
        _imp->showCorrelationButton->setDown(false);
        QObject::connect( _imp->showCorrelationButton,SIGNAL( clicked(bool) ),this,SLOT( onShowCorrelationButtonClicked(bool) ) );
        _imp->buttonsLayout->addWidget(_imp->showCorrelationButton);
        
        QWidget* keyframeContainer = new QWidget(_imp->buttonsBar);
        QHBoxLayout* keyframeLayout = new QHBoxLayout(keyframeContainer);
        keyframeLayout->setContentsMargins(0, 0, 0, 0);
        keyframeLayout->setSpacing(0);
        
        QPixmap addKeyPix,removeKeyPix,resetOffsetPix,removeAllUserKeysPix;
        appPTR->getIcon(Natron::NATRON_PIXMAP_ADD_USER_KEY, &addKeyPix);
        appPTR->getIcon(Natron::NATRON_PIXMAP_REMOVE_USER_KEY, &removeKeyPix);
        appPTR->getIcon(Natron::NATRON_PIXMAP_RESET_TRACK_OFFSET, &resetOffsetPix);
        appPTR->getIcon(Natron::NATRON_PIXMAP_RESET_USER_KEYS, &removeAllUserKeysPix);
        
        
        _imp->setKeyFrameButton = new Button(QIcon(addKeyPix), QString(), keyframeContainer);
        _imp->setKeyFrameButton->setFixedSize(medButtonSize);
        _imp->setKeyFrameButton->setIconSize(medButtonIconSize);
        _imp->setKeyFrameButton->setToolTip(GuiUtils::convertFromPlainText(tr("Set a keyframe for the pattern for the selected tracks"), Qt::WhiteSpaceNormal));
        QObject::connect( _imp->setKeyFrameButton,SIGNAL( clicked(bool) ),this,SLOT( onSetKeyframeButtonClicked() ) );
        keyframeLayout->addWidget(_imp->setKeyFrameButton);
        
        _imp->removeKeyFrameButton = new Button(QIcon(removeKeyPix), QString(), keyframeContainer);
        _imp->removeKeyFrameButton->setFixedSize(medButtonSize);
        _imp->removeKeyFrameButton->setIconSize(medButtonIconSize);
        _imp->removeKeyFrameButton->setToolTip(GuiUtils::convertFromPlainText(tr("Remove a keyframe for the pattern for the selected tracks"), Qt::WhiteSpaceNormal));
        QObject::connect( _imp->removeKeyFrameButton,SIGNAL( clicked(bool) ),this,SLOT( onRemoveKeyframeButtonClicked() ) );
        keyframeLayout->addWidget(_imp->removeKeyFrameButton);
        

        
        _imp->buttonsLayout->addWidget(keyframeContainer);
        
        QPixmap resetPix;
        appPTR->getIcon(Natron::NATRON_PIXMAP_RESTORE_DEFAULTS_ENABLED, &resetPix);
        _imp->resetOffsetButton = new Button(QIcon(resetOffsetPix), QString(), _imp->buttonsBar);
        _imp->resetOffsetButton->setFixedSize(medButtonSize);
        _imp->resetOffsetButton->setIconSize(medButtonIconSize);
        _imp->resetOffsetButton->setToolTip(GuiUtils::convertFromPlainText(tr("Resets the offset for the selected tracks"), Qt::WhiteSpaceNormal));
        QObject::connect( _imp->resetOffsetButton,SIGNAL( clicked(bool) ),this,SLOT( onResetOffsetButtonClicked() ) );
        _imp->buttonsLayout->addWidget(_imp->resetOffsetButton);
        
        _imp->resetTrackButton = new Button(QIcon(resetPix), QString(), _imp->buttonsBar);
        _imp->resetTrackButton->setFixedSize(medButtonSize);
        _imp->resetTrackButton->setIconSize(medButtonIconSize);
        _imp->resetTrackButton->setToolTip(GuiUtils::convertFromPlainText(tr("Resets animation for the selected tracks"), Qt::WhiteSpaceNormal));
        QObject::connect( _imp->resetTrackButton,SIGNAL( clicked(bool) ),this,SLOT( onResetTrackButtonClicked() ) );
        _imp->buttonsLayout->addWidget(_imp->resetTrackButton);
        
    }
    
    _imp->buttonsLayout->addStretch();
    
    if (_imp->panel) {
        QObject::connect(_imp->panel->getNode()->getNode().get(), SIGNAL(s_refreshPreviewsAfterProjectLoadRequested()), this, SLOT(rebuildMarkerTextures()));
    }
    
    
    ///Refresh track parameters according to buttons state
    if (_imp->panelv1) {
        _imp->panelv1->setUpdateViewer(_imp->updateViewerButton->isDown());
        _imp->panelv1->setCenterOnTrack(_imp->centerViewerButton->isDown());
    } else {
        _imp->panel->getContext()->setUpdateViewer(_imp->updateViewerButton->isDown());
        _imp->panel->getContext()->setCenterOnTrack(_imp->centerViewerButton->isDown());
    }
}

TrackerGui::~TrackerGui()
{
}

void
TrackerGui::rebuildMarkerTextures()
{
    ///Refreh textures for all markers
    std::list<boost::shared_ptr<TrackMarker> > markers;
    _imp->panel->getContext()->getSelectedMarkers(&markers);
    for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it!=markers.end(); ++it) {
        std::set<int> keys;
        (*it)->getUserKeyframes(&keys);
        for (std::set<int>::iterator it2 = keys.begin(); it2 != keys.end(); ++it2) {
            _imp->makeMarkerKeyTexture(*it2, *it);
        }
    }
    onContextSelectionChanged(TrackerContext::eTrackSelectionInternal);

}

QWidget*
TrackerGui::getButtonsBar() const
{
    return _imp->buttonsBar;
}

void
TrackerGui::onAddTrackClicked(bool clicked)
{
    _imp->clickToAddTrackEnabled = !_imp->clickToAddTrackEnabled;
    _imp->addTrackButton->setDown(clicked);
    _imp->addTrackButton->setChecked(clicked);
    _imp->viewer->getViewer()->redraw();
}

static QPointF computeMidPointExtent(const QPointF& prev, const QPointF& next, const QPointF& point,
                                     const QPointF& handleSize)
{
    Natron::Point leftDeriv,rightDeriv;
    leftDeriv.x = prev.x() - point.x();
    leftDeriv.y = prev.y() - point.y();
    
    rightDeriv.x = next.x() - point.x();
    rightDeriv.y = next.y() - point.y();
    double derivNorm = std::sqrt((rightDeriv.x - leftDeriv.x) * (rightDeriv.x - leftDeriv.x) + (rightDeriv.y - leftDeriv.y) * (rightDeriv.y - leftDeriv.y));
    QPointF ret;
    if (derivNorm == 0) {
        double norm = std::sqrt((leftDeriv.x - point.x()) * (leftDeriv.x - point.x()) + (leftDeriv.y - point.y()) * (leftDeriv.y  - point.y()));
        if (norm != 0) {
            ret.rx() = point.x() + ((leftDeriv.y - point.y()) / norm) * handleSize.x();
            ret.ry() = point.y() - ((leftDeriv.x - point.x()) / norm) * handleSize.y();
            return ret;
        } else {
            return QPointF(0,0);
        }

    } else {
        ret.rx() = point.x() + ((rightDeriv.y - leftDeriv.y) / derivNorm) * handleSize.x();
        ret.ry() = point.y() - ((rightDeriv.x - leftDeriv.x) / derivNorm) * handleSize.y();
    }
    return ret;
}

void
TrackerGui::drawOverlays(double time,
                         const RenderScale & renderScale,
                         ViewIdx view) const
{
    FLAG_DURING_INTERACT
    
    double pixelScaleX, pixelScaleY;
    ViewerGL* viewer = _imp->viewer->getViewer();
    viewer->getPixelScale(pixelScaleX, pixelScaleY);
    double viewportSize[2];
    viewer->getViewportSize(viewportSize[0], viewportSize[1]);
    
    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_POINT_BIT | GL_ENABLE_BIT | GL_HINT_BIT | GL_TRANSFORM_BIT);
        
        if (_imp->panelv1) {
            ///For each instance: <pointer,selected ? >
            const std::list<std::pair<NodeWPtr,bool> > & instances = _imp->panelv1->getInstances();
            for (std::list<std::pair<NodeWPtr,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
                
                boost::shared_ptr<Natron::Node> instance = it->first.lock();
                
                if (instance->isNodeDisabled()) {
                    continue;
                }
                if (it->second) {
                    ///The track is selected, use the plug-ins interact
                    EffectInstPtr effect = instance->getEffectInstance();
                    assert(effect);
                    effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                    effect->drawOverlay_public(time, renderScale, view);
                } else {
                    ///Draw a custom interact, indicating the track isn't selected
                    boost::shared_ptr<KnobI> newInstanceKnob = instance->getKnobByName("center");
                    assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
                    KnobDouble* dblKnob = dynamic_cast<KnobDouble*>( newInstanceKnob.get() );
                    assert(dblKnob);
                    
                    //GLProtectMatrix p(GL_PROJECTION); // useless (we do two glTranslate in opposite directions)
                    for (int l = 0; l < 2; ++l) {
                        // shadow (uses GL_PROJECTION)
                        glMatrixMode(GL_PROJECTION);
                        int direction = (l == 0) ? 1 : -1;
                        // translate (1,-1) pixels
                        glTranslated(direction * pixelScaleX / 256, -direction * pixelScaleY / 256, 0);
                        glMatrixMode(GL_MODELVIEW);
                        
                        if (l == 0) {
                            glColor4d(0., 0., 0., 1.);
                        } else {
                            glColor4f(1., 1., 1., 1.);
                        }
                        
                        double x = dblKnob->getValue(0);
                        double y = dblKnob->getValue(1);
                        glPointSize(POINT_SIZE);
                        glBegin(GL_POINTS);
                        glVertex2d(x,y);
                        glEnd();
                        
                        glBegin(GL_LINES);
                        glVertex2d(x - CROSS_SIZE * pixelScaleX, y);
                        glVertex2d(x + CROSS_SIZE * pixelScaleX, y);
                        
                        glVertex2d(x, y - CROSS_SIZE * pixelScaleY);
                        glVertex2d(x, y + CROSS_SIZE * pixelScaleY);
                        glEnd();
                    }
                    glPointSize(1.);
                }
            }
            
        } // if (_imp->panelv1) {
        else {
            assert(_imp->panel);
            double markerColor[3];
            if (!_imp->panel->getNode()->getOverlayColor(&markerColor[0], &markerColor[1], &markerColor[2])) {
                markerColor[0] = markerColor[1] = markerColor[2] = 0.8;
            }
            
            std::vector<boost::shared_ptr<TrackMarker> > allMarkers;
            std::list<boost::shared_ptr<TrackMarker> > selectedMarkers;
            
            boost::shared_ptr<TrackerContext> context = _imp->panel->getContext();
            context->getSelectedMarkers(&selectedMarkers);
            context->getAllMarkers(&allMarkers);
            
            bool showErrorColor = _imp->showCorrelationButton->isChecked();
            
            boost::shared_ptr<TrackMarker> selectedMarker = _imp->selectedMarker.lock();
            Natron::Point selectedCenter,selectedPtnTopLeft,selectedPtnTopRight,selectedPtnBtmRight,selectedPtnBtmLeft, selectedOffset, selectedSearchBtmLeft, selectedSearchTopRight;
            
            for (std::vector<boost::shared_ptr<TrackMarker> >::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {
                if (!(*it)->isEnabled()) {
                    continue;
                }
                std::list<boost::shared_ptr<TrackMarker> >::iterator foundSelected = std::find(selectedMarkers.begin(),selectedMarkers.end(),*it);
                bool isSelected = foundSelected != selectedMarkers.end();
                
                boost::shared_ptr<KnobDouble> centerKnob = (*it)->getCenterKnob();
                boost::shared_ptr<KnobDouble> offsetKnob = (*it)->getOffsetKnob();
                boost::shared_ptr<KnobDouble> correlationKnob = (*it)->getCorrelationKnob();
                boost::shared_ptr<KnobDouble> ptnTopLeft = (*it)->getPatternTopLeftKnob();
                boost::shared_ptr<KnobDouble> ptnTopRight = (*it)->getPatternTopRightKnob();
                boost::shared_ptr<KnobDouble> ptnBtmRight = (*it)->getPatternBtmRightKnob();
                boost::shared_ptr<KnobDouble> ptnBtmLeft = (*it)->getPatternBtmLeftKnob();
                boost::shared_ptr<KnobDouble> searchWndBtmLeft = (*it)->getSearchWindowBottomLeftKnob();
                boost::shared_ptr<KnobDouble> searchWndTopRight = (*it)->getSearchWindowTopRightKnob();
                
                if (!isSelected) {
                    ///Draw a custom interact, indicating the track isn't selected
                    glEnable(GL_LINE_SMOOTH);
                    glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
                    glLineWidth(1.5f);
                    for (int l = 0; l < 2; ++l) {
                        // shadow (uses GL_PROJECTION)
                        glMatrixMode(GL_PROJECTION);
                        int direction = (l == 0) ? 1 : -1;
                        // translate (1,-1) pixels
                        glTranslated(direction * pixelScaleX / 256, -direction * pixelScaleY / 256, 0);
                        glMatrixMode(GL_MODELVIEW);
                        
                        if (l == 0) {
                            glColor4d(0., 0., 0., 1.);
                        } else {
                            glColor4f(markerColor[0], markerColor[1], markerColor[2], 1.);
                        }
                        
                        
                        double x = centerKnob->getValueAtTime(time,0);
                        double y = centerKnob->getValueAtTime(time,1);
                        glPointSize(POINT_SIZE);
                        glBegin(GL_POINTS);
                        glVertex2d(x,y);
                        glEnd();
                        
                        glBegin(GL_LINES);
                        glVertex2d(x - CROSS_SIZE * pixelScaleX, y);
                        glVertex2d(x + CROSS_SIZE * pixelScaleX, y);
                        
                        glVertex2d(x, y - CROSS_SIZE * pixelScaleY);
                        glVertex2d(x, y + CROSS_SIZE * pixelScaleY);
                        glEnd();
                    }
                    glPointSize(1.);
                } else { // if (isSelected) {
                    glEnable(GL_LINE_SMOOTH);
                    glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
                    GLdouble projection[16];
                    glGetDoublev( GL_PROJECTION_MATRIX, projection);
                    OfxPointD shadow; // how much to translate GL_PROJECTION to get exactly one pixel on screen
                    shadow.x = 2./(projection[0] * viewportSize[0]);
                    shadow.y = 2./(projection[5] * viewportSize[1]);
                    
                    QPointF offset,center,topLeft,topRight,btmRight,btmLeft;
                    QPointF searchTopLeft,searchTopRight,searchBtmLeft,searchBtmRight;
                    center.rx() = centerKnob->getValueAtTime(time, 0);
                    center.ry() = centerKnob->getValueAtTime(time, 1);
                    offset.rx() = offsetKnob->getValueAtTime(time, 0);
                    offset.ry() = offsetKnob->getValueAtTime(time, 1);
                    
                    topLeft.rx() = ptnTopLeft->getValueAtTime(time, 0) + offset.x() + center.x();
                    topLeft.ry() = ptnTopLeft->getValueAtTime(time, 1) + offset.y() + center.y();
                    
                    topRight.rx() = ptnTopRight->getValueAtTime(time, 0) + offset.x() + center.x();
                    topRight.ry() = ptnTopRight->getValueAtTime(time, 1) + offset.y() + center.y();
                    
                    btmRight.rx() = ptnBtmRight->getValueAtTime(time, 0) + offset.x() + center.x();
                    btmRight.ry() = ptnBtmRight->getValueAtTime(time, 1) + offset.y() + center.y();
                    
                    btmLeft.rx() = ptnBtmLeft->getValueAtTime(time, 0) + offset.x() + center.x();
                    btmLeft.ry() = ptnBtmLeft->getValueAtTime(time, 1) + offset.y() + center.y();
                    
                    searchBtmLeft.rx() = searchWndBtmLeft->getValueAtTime(time, 0) + offset.x() + center.x();
                    searchBtmLeft.ry() = searchWndBtmLeft->getValueAtTime(time, 1) + offset.y() + center.y();
                    
                    searchTopRight.rx() = searchWndTopRight->getValueAtTime(time, 0) + offset.x() + center.x();
                    searchTopRight.ry() = searchWndTopRight->getValueAtTime(time, 1) + offset.y() + center.y();
                    
                    searchTopLeft.rx() = searchBtmLeft.x();
                    searchTopLeft.ry() = searchTopRight.y();
                    
                    searchBtmRight.rx() = searchTopRight.x();
                    searchBtmRight.ry() = searchBtmLeft.y();
                    
                    
                    if (selectedMarker == *it) {
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
                        selectedSearchBtmLeft.x = searchBtmLeft.x();
                        selectedSearchBtmLeft.y = searchBtmLeft.y();
                        
                        selectedSearchTopRight.x = searchTopRight.x();
                        selectedSearchTopRight.y = searchTopRight.y();
                    }
                    
                    QPointF innerMidLeft((btmLeft.x() + topLeft.x()) / 2., (btmLeft.y() + topLeft.y()) / 2.),
                    innerMidTop((topLeft.x() + topRight.x()) / 2., (topLeft.y() + topRight.y()) / 2.),
                    innerMidRight((btmRight.x() + topRight.x()) / 2., (btmRight.y() + topRight.y()) / 2.),
                    innerMidBtm((btmLeft.x() + btmRight.x()) / 2., (btmLeft.y() + btmRight.y()) / 2.),
                    outterMidLeft((searchBtmLeft.x() + searchTopLeft.x()) / 2., (searchBtmLeft.y() + searchTopLeft.y()) / 2.),
                    outterMidTop((searchTopLeft.x() + searchTopRight.x()) / 2., (searchTopLeft.y() + searchTopRight.y()) / 2.),
                    outterMidRight((searchBtmRight.x() + searchTopRight.x()) / 2., (searchBtmRight.y() + searchTopRight.y()) / 2.),
                    outterMidBtm((searchBtmLeft.x() + searchBtmRight.x()) / 2., (searchBtmLeft.y() + searchBtmRight.y()) / 2.);
                    
                    QPointF handleSize;
                    handleSize.rx() = HANDLE_SIZE * pixelScaleX;
                    handleSize.ry() = handleSize.x();
                    
                    QPointF innerMidLeftExt = computeMidPointExtent(topLeft, btmLeft, innerMidLeft, handleSize);
                    QPointF innerMidRightExt = computeMidPointExtent(btmRight, topRight, innerMidRight, handleSize);
                    QPointF innerMidTopExt = computeMidPointExtent(topRight, topLeft, innerMidTop, handleSize);
                    QPointF innerMidBtmExt = computeMidPointExtent(btmLeft, btmRight, innerMidBtm, handleSize);
                    
                    QPointF outterMidLeftExt = computeMidPointExtent(searchTopLeft, searchBtmLeft, outterMidLeft, handleSize);
                    QPointF outterMidRightExt = computeMidPointExtent(searchBtmRight, searchTopRight, outterMidRight, handleSize);
                    QPointF outterMidTopExt = computeMidPointExtent(searchTopRight, searchTopLeft, outterMidTop, handleSize);
                    QPointF outterMidBtmExt = computeMidPointExtent(searchBtmLeft,searchBtmRight, outterMidBtm, handleSize);
                    
                    std::string name = (*it)->getLabel();

                    std::map<int,std::pair<Natron::Point,double> > centerPoints;
                    int floorTime = std::floor(time + 0.5);
                    
                    boost::shared_ptr<Curve> xCurve = centerKnob->getCurve(ViewSpec::current(), 0);
                    boost::shared_ptr<Curve> yCurve = centerKnob->getCurve(ViewSpec::current(), 1);
                    boost::shared_ptr<Curve> errorCurve = correlationKnob->getCurve(ViewSpec::current(), 0);
                    
                    for (int i = 0; i < MAX_CENTER_POINTS_DISPLAYED / 2; ++i) {
                        KeyFrame k;
                        int keyTimes[2] = {floorTime + i, floorTime - i};
                        
                        for (int j = 0; j < 2; ++j) {
                            if (xCurve->getKeyFrameWithTime(keyTimes[j], &k)) {
                                std::pair<Natron::Point,double>& p = centerPoints[k.getTime()];
                                p.first.x = k.getValue();
                                p.first.y = INT_MIN;
                                
                                if (yCurve->getKeyFrameWithTime(keyTimes[j], &k)) {
                                    p.first.y = k.getValue();
                                }
                                if (showErrorColor && errorCurve->getKeyFrameWithTime(keyTimes[j], &k)) {
                                    p.second = k.getValue();
                                }
                            }
                        }
                    }
           
          
                    for (int l = 0; l < 2; ++l) {
                        // shadow (uses GL_PROJECTION)
                        glMatrixMode(GL_PROJECTION);
                        int direction = (l == 0) ? 1 : -1;
                        // translate (1,-1) pixels
                        glTranslated(direction * shadow.x, -direction * shadow.y, 0);
                        glMatrixMode(GL_MODELVIEW);
                        
                        ///Draw center position
                        
                        glBegin(GL_POINTS);
                        if (!showErrorColor) {
                            glColor3f(0.5 * l,0.5 * l,0.5 * l);
                        }
                        for (std::map<int,std::pair<Natron::Point,double> >::iterator it = centerPoints.begin() ;it!=centerPoints.end(); ++it) {
                            if (showErrorColor) {
                                if (l != 0) {
                                    /*
                                     Map the correlation to [0, 0.33] since 0 is Red for HSV and 0.33 is Green. 
                                     Also clamp to the interval if the correlation is higher, and reverse.
                                     */
                                    double mappedError = 0.33 - std::max(std::min(0.33,it->second.second / 3.),0.);
                                    QColor c;
                                    c.setHsvF(mappedError, 1., 1.);
                                    glColor3f(c.redF(), c.greenF(), c.blueF());
                                } else {
                                    glColor3f(0., 0., 0.);
                                }
                            }
                            glVertex2d(it->second.first.x, it->second.first.y);
                        }
                        glEnd();
                        
                        glBegin(GL_LINE_STRIP);
                        glColor3f(0.5 * l,0.5 * l,0.5 * l);
                        for (std::map<int,std::pair<Natron::Point,double> >::iterator it = centerPoints.begin() ;it!=centerPoints.end(); ++it) {
                            glVertex2d(it->second.first.x, it->second.first.y);
                        }
                        glEnd();
                        
                        glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        glBegin(GL_LINE_LOOP);
                        glVertex2d(topLeft.x(), topLeft.y());
                        glVertex2d(topRight.x(), topRight.y());
                        glVertex2d(btmRight.x(), btmRight.y());
                        glVertex2d(btmLeft.x(), btmLeft.y());
                        glEnd();
                        
                        glBegin(GL_LINE_LOOP);
                        glVertex2d(searchTopLeft.x(),searchTopLeft.y());
                        glVertex2d(searchTopRight.x(), searchTopRight.y());
                        glVertex2d(searchBtmRight.x(), searchBtmRight.y());
                        glVertex2d(searchBtmLeft.x(), searchBtmRight.y());
                        glEnd();
                        
                        glPointSize(POINT_SIZE);
                        glBegin(GL_POINTS);
                        
                        ///draw center
                        if ( (_imp->hoverState == eDrawStateHoveringCenter) || (_imp->eventState == eMouseStateDraggingCenter)) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        }
                        glVertex2d(center.x(), center.y());
                        
                        if (offset.x() != 0 || offset.y() != 0) {
                            glVertex2d(center.x() + offset.x(), center.y() + offset.y());
                        }
                        
                        //////DRAWING INNER POINTS
                        if ( (_imp->hoverState == eDrawStateHoveringInnerBtmLeft) || (_imp->eventState == eMouseStateDraggingInnerBtmLeft) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(btmLeft.x(), btmLeft.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringInnerBtmMid) || (_imp->eventState == eMouseStateDraggingInnerBtmMid) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(innerMidBtm.x(), innerMidBtm.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringInnerBtmRight) || (_imp->eventState == eMouseStateDraggingInnerBtmRight) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(btmRight.x(), btmRight.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringInnerMidLeft) || (_imp->eventState == eMouseStateDraggingInnerMidLeft) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(innerMidLeft.x(), innerMidLeft.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringInnerMidRight) || (_imp->eventState == eMouseStateDraggingInnerMidRight) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(innerMidRight.x(), innerMidRight.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringInnerTopLeft) || (_imp->eventState == eMouseStateDraggingInnerTopLeft) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(topLeft.x(), topLeft.y());
                        }
                        
                        if ( (_imp->hoverState == eDrawStateHoveringInnerTopMid) || (_imp->eventState == eMouseStateDraggingInnerTopMid) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(innerMidTop.x(), innerMidTop.y());
                        }
                        
                        if ( (_imp->hoverState == eDrawStateHoveringInnerTopRight) || (_imp->eventState == eMouseStateDraggingInnerTopRight) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(topRight.x(), topRight.y());
                        }
                        
                        
                        //////DRAWING OUTTER POINTS
                        
                        if ( (_imp->hoverState == eDrawStateHoveringOuterBtmLeft) || (_imp->eventState == eMouseStateDraggingOuterBtmLeft) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(searchBtmLeft.x(), searchBtmLeft.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringOuterBtmMid) || (_imp->eventState == eMouseStateDraggingOuterBtmMid) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(outterMidBtm.x(), outterMidBtm.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringOuterBtmRight) || (_imp->eventState == eMouseStateDraggingOuterBtmRight) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(searchBtmRight.x() , searchBtmRight.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringOuterMidLeft) || (_imp->eventState == eMouseStateDraggingOuterMidLeft) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(outterMidLeft.x(), outterMidLeft.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringOuterMidRight) || (_imp->eventState == eMouseStateDraggingOuterMidRight) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(outterMidRight.x(), outterMidRight.y());
                        }
                        
                        if ( (_imp->hoverState == eDrawStateHoveringOuterTopLeft) || (_imp->eventState == eMouseStateDraggingOuterTopLeft) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(searchTopLeft.x(), searchTopLeft.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringOuterTopMid) || (_imp->eventState == eMouseStateDraggingOuterTopMid) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(outterMidTop.x(), outterMidTop.y());
                        }
                        if ( (_imp->hoverState == eDrawStateHoveringOuterTopRight) || (_imp->eventState == eMouseStateDraggingOuterTopRight) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d(searchTopRight.x(), searchTopRight.y());
                        }
                        
                        glEnd();
                        
                        if (offset.x() != 0 || offset.y() != 0) {
                            glBegin(GL_LINES);
                            glColor3f((float)markerColor[0] * l * 0.5, (float)markerColor[1] * l * 0.5, (float)markerColor[2] * l * 0.5);
                            glVertex2d(center.x(), center.y());
                            glVertex2d(center.x() + offset.x(), center.y() + offset.y());
                            glEnd();
                        }
                        
                        ///now show small lines at handle positions
                        glBegin(GL_LINES);
                        
                        if ( (_imp->hoverState == eDrawStateHoveringInnerMidLeft) || (_imp->eventState == eMouseStateDraggingInnerMidLeft) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        }
                        glVertex2d(innerMidLeft.x(), innerMidLeft.y());
                        glVertex2d(innerMidLeftExt.x(), innerMidLeftExt.y());
                        
                        if ( (_imp->hoverState == eDrawStateHoveringInnerTopMid) || (_imp->eventState == eMouseStateDraggingInnerTopMid) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        }
                        glVertex2d(innerMidTop.x(), innerMidTop.y());
                        glVertex2d(innerMidTopExt.x(), innerMidTopExt.y());
                        
                        if ( (_imp->hoverState == eDrawStateHoveringInnerMidRight) || (_imp->eventState == eMouseStateDraggingInnerMidRight) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        }
                        glVertex2d(innerMidRight.x(), innerMidRight.y());
                        glVertex2d(innerMidRightExt.x(), innerMidRightExt.y());
                        
                        if ( (_imp->hoverState == eDrawStateHoveringInnerBtmMid) || (_imp->eventState == eMouseStateDraggingInnerBtmMid) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        }
                        glVertex2d(innerMidBtm.x(), innerMidBtm.y());
                        glVertex2d(innerMidBtmExt.x(), innerMidBtmExt.y());
                        
                        //////DRAWING OUTTER HANDLES
                        
                        if ( (_imp->hoverState == eDrawStateHoveringOuterMidLeft) || (_imp->eventState == eMouseStateDraggingOuterMidLeft) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        }
                        glVertex2d(outterMidLeft.x(), outterMidLeft.y());
                        glVertex2d(outterMidLeftExt.x(), outterMidLeftExt.y());
                        
                        if ( (_imp->hoverState == eDrawStateHoveringOuterTopMid) || (_imp->eventState == eMouseStateDraggingOuterTopMid) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        }
                        glVertex2d(outterMidTop.x(), outterMidTop.y());
                        glVertex2d(outterMidTopExt.x(), outterMidTopExt.y());
                        
                        if ( (_imp->hoverState == eDrawStateHoveringOuterMidRight) || (_imp->eventState == eMouseStateDraggingOuterMidRight) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        }
                        glVertex2d(outterMidRight.x(), outterMidRight.y());
                        glVertex2d(outterMidRightExt.x(), outterMidRightExt.y());
                        
                        if ( (_imp->hoverState == eDrawStateHoveringOuterBtmMid) || (_imp->eventState == eMouseStateDraggingOuterBtmMid) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                        }
                        glVertex2d(outterMidBtm.x(), outterMidBtm.y());
                        glVertex2d(outterMidBtmExt.x(), outterMidBtmExt.y());
                        glEnd();
                        
                        glColor3f((float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l);
                       
                        QColor c;
                        c.setRgbF(markerColor[0], markerColor[1], markerColor[2]);
                        viewer->renderText(center.x(), center.y(), QString::fromUtf8(name.c_str()), c, viewer->font());
                    } // for (int l = 0; l < 2; ++l) {

                } // if (!isSelected) {
            } // for (std::vector<boost::shared_ptr<TrackMarker> >::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {
            
            if (_imp->showMarkerTexture) {
                _imp->drawSelectedMarkerTexture(std::make_pair(pixelScaleX, pixelScaleY), time, selectedCenter, selectedOffset,  selectedPtnTopLeft, selectedPtnTopRight,selectedPtnBtmRight, selectedPtnBtmLeft, selectedSearchBtmLeft, selectedSearchTopRight);

            }
        } // // if (_imp->panelv1) {
        
        
        if (_imp->clickToAddTrackEnabled) {
            ///draw a square of 20px around the mouse cursor
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_LINE_SMOOTH);
            glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
            glLineWidth(1.5);
            //GLProtectMatrix p(GL_PROJECTION); // useless (we do two glTranslate in opposite directions)
            
            const double addTrackSize = TO_DPIX(ADDTRACK_SIZE);
            
            for (int l = 0; l < 2; ++l) {
                // shadow (uses GL_PROJECTION)
                glMatrixMode(GL_PROJECTION);
                int direction = (l == 0) ? 1 : -1;
                // translate (1,-1) pixels
                glTranslated(direction * pixelScaleX / 256, -direction * pixelScaleY / 256, 0);
                glMatrixMode(GL_MODELVIEW);
                
                if (l == 0) {
                    glColor4d(0., 0., 0., 0.8);
                } else {
                    glColor4d(0., 1., 0.,0.8);
                }
                
                glBegin(GL_LINE_LOOP);
                glVertex2d(_imp->lastMousePos.x() - addTrackSize * 2 * pixelScaleX, _imp->lastMousePos.y() - addTrackSize * 2 * pixelScaleY);
                glVertex2d(_imp->lastMousePos.x() - addTrackSize * 2 * pixelScaleX, _imp->lastMousePos.y() + addTrackSize * 2 * pixelScaleY);
                glVertex2d(_imp->lastMousePos.x() + addTrackSize * 2 * pixelScaleX, _imp->lastMousePos.y() + addTrackSize * 2 * pixelScaleY);
                glVertex2d(_imp->lastMousePos.x() + addTrackSize * 2 * pixelScaleX, _imp->lastMousePos.y() - addTrackSize * 2 * pixelScaleY);
                glEnd();
                
                ///draw a cross at the cursor position
                glBegin(GL_LINES);
                glVertex2d( _imp->lastMousePos.x() - addTrackSize * pixelScaleX, _imp->lastMousePos.y() );
                glVertex2d( _imp->lastMousePos.x() + addTrackSize * pixelScaleX, _imp->lastMousePos.y() );
                glVertex2d(_imp->lastMousePos.x(), _imp->lastMousePos.y() - addTrackSize * pixelScaleY);
                glVertex2d(_imp->lastMousePos.x(), _imp->lastMousePos.y() + addTrackSize * pixelScaleY);
                glEnd();
            }
        }
    } // GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_ENABLE_BIT | GL_HINT_BIT);
} // drawOverlays

int
TrackerGuiPrivate::isInsideKeyFrameTexture(double currentTime, const QPointF& pos, const QPointF& viewportPos) const
{
    
    if (!showMarkerTexture) {
        return INT_MAX;
    }
    
    
    RectD textureRectCanonical;
    if (selectedMarkerTexture) {
        computeSelectedMarkerCanonicalRect(&textureRectCanonical);
    }
    
    if (pos.y() < textureRectCanonical.y1 || pos.y() > textureRectCanonical.y2) {
        return INT_MAX;
    }
    if (pos.x() < textureRectCanonical.x2) {
        return INT_MAX;
    }
    
    boost::shared_ptr<TrackMarker> marker = selectedMarker.lock();
    if (!marker) {
        return INT_MAX;
    }
    
    //Find out which keyframe it is by counting keyframe portions
    int xRightMainTexture = viewer->getViewer()->toWidgetCoordinates(QPointF(textureRectCanonical.x2,0)).x();
    
    const double keyWidthpx = TO_DPIX(SELECTED_MARKER_KEYFRAME_WIDTH_SCREEN_PX);
    
    double indexF = (viewportPos.x() - xRightMainTexture) / keyWidthpx;
    int texIndex = (int)std::floor(indexF);
    
    for (TrackKeysMap::const_iterator it = trackTextures.begin(); it!=trackTextures.end(); ++it) {
        if (it->first.lock() == marker) {
            
            if (it->second.empty()) {
                break;
            }
            ///Render at most MAX_TRACK_KEYS_TO_DISPLAY keyframes
            KeyFrameTexIDs keysToRender = getKeysToRenderForMarker(currentTime, it->second);
            if (texIndex < 0 || texIndex >= (int)keysToRender.size()) {
                return INT_MAX;
            }
            KeyFrameTexIDs::iterator found = keysToRender.begin();
            std::advance(found, texIndex);
            
            RectD texCanonicalRect;
            computeTextureCanonicalRect(*found->second, indexF * keyWidthpx + xRightMainTexture,
                                        keyWidthpx, &texCanonicalRect);
            
            if (pos.y() >= texCanonicalRect.y1 && pos.y() < texCanonicalRect.y2) {
                return found->first;
            }
            break;

        }
    }
    return INT_MAX;
}

bool
TrackerGuiPrivate::isNearbySelectedMarkerTextureResizeAnchor(const QPointF& pos) const
{
    RectD textureRectCanonical;
    computeSelectedMarkerCanonicalRect(&textureRectCanonical);
    
    QPointF clickWidget = viewer->getViewer()->toWidgetCoordinates(pos);
    QPointF btmRightWidget = viewer->getViewer()->toWidgetCoordinates(QPointF(textureRectCanonical.x2, textureRectCanonical.y1));
    
    double tolerance = TO_DPIX(POINT_TOLERANCE);
    if (clickWidget.x() >= (btmRightWidget.x() - tolerance) && clickWidget.x() <= (btmRightWidget.x() + tolerance) &&
        clickWidget.y() >= (btmRightWidget.y() - tolerance) && clickWidget.y() <= (btmRightWidget.y() + tolerance)) {
        return true;
    }
    return false;
}

bool
TrackerGuiPrivate::isInsideSelectedMarkerTextureResizeAnchor(const QPointF& pos) const
{
    RectD textureRectCanonical;
    computeSelectedMarkerCanonicalRect(&textureRectCanonical);
    
    QPointF clickWidget = viewer->getViewer()->toWidgetCoordinates(pos);
    QPointF btmRightWidget = viewer->getViewer()->toWidgetCoordinates(QPointF(textureRectCanonical.x2, textureRectCanonical.y1));
    QPointF topLeftWidget = viewer->getViewer()->toWidgetCoordinates(QPointF(textureRectCanonical.x1, textureRectCanonical.y2));
    
    RectD rect;
    rect.x1 = topLeftWidget.x();
    rect.y1 = topLeftWidget.y();
    rect.x2 = btmRightWidget.x();
    rect.y2 = btmRightWidget.y();
    return rect.contains(clickWidget.x(), clickWidget.y());
}

void
TrackerGuiPrivate::computeTextureCanonicalRect(const Texture& tex,int xOffset, int texWidthPx, RectD* rect) const
{
    
    ///Preserve width
    double par = tex.w() / (double)tex.h();
    
    rect->x2 = viewer->getViewer()->toZoomCoordinates(QPointF(xOffset + texWidthPx, 0.)).x();
    QPointF topLeft = viewer->getViewer()->toZoomCoordinates(QPointF(xOffset, 0.));
    rect->x1 = topLeft.x();
    rect->y2 = topLeft.y();
    double height = rect->width() / par;
    rect->y1 = rect->y2 - height;
}

void
TrackerGuiPrivate::computeSelectedMarkerCanonicalRect(RectD* rect) const
{
    assert(selectedMarkerTexture);
    computeTextureCanonicalRect(*selectedMarkerTexture, 0, selectedMarkerWidth, rect);
}

static Natron::Point toMagWindowPoint(const Natron::Point& ptnPoint,
                                      const Natron::Point& selectedSearchWndBtmLeft,
                                      const Natron::Point& selectedSearchWndTopRight,
                                      const RectD& textureRectCanonical) {
    Natron::Point ret;
    double xCenterPercent = (ptnPoint.x - selectedSearchWndBtmLeft.x) / (selectedSearchWndTopRight.x - selectedSearchWndBtmLeft.x);
    double yCenterPercent = (ptnPoint.y - selectedSearchWndBtmLeft.y) / (selectedSearchWndTopRight.y - selectedSearchWndBtmLeft.y);
    ret.y = textureRectCanonical.y1 + yCenterPercent * (textureRectCanonical.y2 - textureRectCanonical.y1);
    ret.x = textureRectCanonical.x1 + xCenterPercent * (textureRectCanonical.x2 - textureRectCanonical.x1);
    return ret;
}

static void
drawEllipse(double x, double y, double radiusX, double radiusY, int l, double r, double g, double b, double a)
{
    glColor3f(r*l*a, g*l*a, b*l*a);
    
    glPushMatrix();
    //  center the oval at x_center, y_center
    glTranslatef((float)x, (float)y, 0.f);
    //  draw the oval using line segments
    glBegin(GL_LINE_LOOP);
    // we don't need to be pixel-perfect here, it's just an interact!
    // 40 segments is enough.
    double m = 2 * 3.14159265358979323846264338327950288419717 / 40.;
    for (int i = 0; i < 40; ++i) {
        double theta = i * m;
        glVertex2d(radiusX * std::cos(theta), radiusY * std::sin(theta));
    }
    glEnd();
    
    glPopMatrix();
}

TrackerGuiPrivate::KeyFrameTexIDs
TrackerGuiPrivate::getKeysToRenderForMarker(double currentTime, const KeyFrameTexIDs& allKeys)
{
    KeyFrameTexIDs keysToRender;
    ///Find the first key equivalent to currentTime or after
    KeyFrameTexIDs::const_iterator lower = allKeys.lower_bound(currentTime);
    KeyFrameTexIDs::const_iterator prev = lower;
    if (lower != allKeys.begin()) {
        --prev;
    } else {
        prev = allKeys.end();
    }
    
    for (int i = 0; i < MAX_TRACK_KEYS_TO_DISPLAY; ++i) {
        if (lower != allKeys.end()) {
            keysToRender.insert(*lower);
            ++lower;
        }
        if (i == MAX_TRACK_KEYS_TO_DISPLAY) {
            break;
        }
        if (prev != allKeys.end()) {
            keysToRender.insert(*prev);
            if (prev != allKeys.begin()) {
                --prev;
            } else {
                prev = allKeys.end();
            }
        } else {
            if (lower == allKeys.end()) {
                ///No more keyframes
                break;
            }
        }
    }
    return keysToRender;
}

void
TrackerGuiPrivate::drawSelectedMarkerKeyframes(const std::pair<double,double>& pixelScale, int currentTime)
{
    boost::shared_ptr<TrackMarker> marker = selectedMarker.lock();
    assert(marker);
    if (!marker) {
        return;
    }
    if (!marker->isEnabled()) {
        return;
    }
    boost::shared_ptr<KnobDouble> centerKnob = marker->getCenterKnob();
    boost::shared_ptr<KnobDouble> offsetKnob = marker->getOffsetKnob();
    boost::shared_ptr<KnobDouble> correlationKnob = marker->getCorrelationKnob();
    boost::shared_ptr<KnobDouble> ptnTopLeft = marker->getPatternTopLeftKnob();
    boost::shared_ptr<KnobDouble> ptnTopRight = marker->getPatternTopRightKnob();
    boost::shared_ptr<KnobDouble> ptnBtmRight = marker->getPatternBtmRightKnob();
    boost::shared_ptr<KnobDouble> ptnBtmLeft = marker->getPatternBtmLeftKnob();
    boost::shared_ptr<KnobDouble> searchWndBtmLeft = marker->getSearchWindowBottomLeftKnob();
    boost::shared_ptr<KnobDouble> searchWndTopRight = marker->getSearchWindowTopRightKnob();
    
    const QFont& font = viewer->font();
    QFontMetrics fm(font);
    int fontHeight = fm.height();
    
    double xOffsetPixels = selectedMarkerWidth;
    QPointF viewerTopLeftCanonical = viewer->getViewer()->toZoomCoordinates(QPointF(0, 0.));

    
    for (TrackKeysMap::iterator it = trackTextures.begin(); it!=trackTextures.end(); ++it) {
        if (it->first.lock() == marker) {
            
            if (it->second.empty()) {
                break;
            }
            ///Render at most MAX_TRACK_KEYS_TO_DISPLAY keyframes
            KeyFrameTexIDs keysToRender = getKeysToRenderForMarker(currentTime, it->second);
            
            for (KeyFrameTexIDs::const_iterator it2 = keysToRender.begin(); it2 != keysToRender.end(); ++it2) {
                
                double time = (double)it2->first;
                
                Natron::Point offset,center,topLeft,topRight,btmRight,btmLeft;
                Natron::Point searchTopLeft,searchTopRight,searchBtmLeft,searchBtmRight;
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
                
                searchBtmLeft.x = searchWndBtmLeft->getValueAtTime(time, 0) + offset.x + center.x;
                searchBtmLeft.y = searchWndBtmLeft->getValueAtTime(time, 1) + offset.y + center.y;
                
                searchTopRight.x = searchWndTopRight->getValueAtTime(time, 0) + offset.x + center.x;
                searchTopRight.y = searchWndTopRight->getValueAtTime(time, 1) + offset.y + center.y;
                
                searchTopLeft.x = searchBtmLeft.x;
                searchTopLeft.y = searchTopRight.y;
                
                searchBtmRight.x = searchTopRight.x;
                searchBtmRight.y = searchBtmLeft.y;
                
                const TextureRect& texRect = it2->second->getTextureRect();
                assert(texRect.h > 0);
                double par = texRect.w / (double)texRect.h;
                RectD textureRectCanonical;
                
                textureRectCanonical.x2 = viewer->getViewer()->toZoomCoordinates(QPointF(TO_DPIX(SELECTED_MARKER_KEYFRAME_WIDTH_SCREEN_PX) + xOffsetPixels, 0.)).x();
                textureRectCanonical.x1 = viewer->getViewer()->toZoomCoordinates(QPointF(xOffsetPixels, 0.)).x();
                textureRectCanonical.y2 = viewerTopLeftCanonical.y();
                double height = textureRectCanonical.width() / par;
                textureRectCanonical.y1 = textureRectCanonical.y2 - height;
                
                
                //Remove any offset to the center to see the marker in the magnification window
                double xCenterPercent = (center.x - searchBtmLeft.x + offset.x) / (searchTopRight.x - searchBtmLeft.x);
                double yCenterPercent = (center.y - searchBtmLeft.y + offset.y) / (searchTopRight.y - searchBtmLeft.y);
                Natron::Point centerPointCanonical;
                centerPointCanonical.y = textureRectCanonical.y1 + yCenterPercent * (textureRectCanonical.y2 - textureRectCanonical.y1);
                centerPointCanonical.x = textureRectCanonical.x1 + xCenterPercent * (textureRectCanonical.x2 - textureRectCanonical.x1);
                
                Natron::Point innerTopLeft = toMagWindowPoint(topLeft, searchBtmLeft, searchTopRight, textureRectCanonical);
                Natron::Point innerTopRight = toMagWindowPoint(topRight, searchBtmLeft, searchTopRight, textureRectCanonical);
                Natron::Point innerBtmLeft = toMagWindowPoint(btmLeft, searchBtmLeft, searchTopRight, textureRectCanonical);
                Natron::Point innerBtmRight = toMagWindowPoint(btmRight, searchBtmLeft, searchTopRight, textureRectCanonical);
                
                //Map texture
                glColor4f(1., 1., 1., 1.);
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, it2->second->getTexID());
                glBegin(GL_POLYGON);
                glTexCoord2d(0, 0); glVertex2d(textureRectCanonical.x1, textureRectCanonical.y1);
                glTexCoord2d(0, 1); glVertex2d(textureRectCanonical.x1, textureRectCanonical.y2);
                glTexCoord2d(1, 1); glVertex2d(textureRectCanonical.x2, textureRectCanonical.y2);
                glTexCoord2d(1, 0); glVertex2d(textureRectCanonical.x2, textureRectCanonical.y1);
                glEnd();
                
                glBindTexture(GL_TEXTURE_2D, 0);
                
                QPointF textPos = viewer->getViewer()->toZoomCoordinates(QPointF(xOffsetPixels + 5, fontHeight + 5 ));
                viewer->getViewer()->renderText(textPos.x(), textPos.y(), QString::fromUtf8(marker->getLabel().c_str()), QColor(200,200,200), font);
                
                QPointF framePos = viewer->getViewer()->toZoomCoordinates(QPointF(xOffsetPixels + 5, viewer->getViewer()->toWidgetCoordinates(QPointF(textureRectCanonical.x1, textureRectCanonical.y1)).y()));
                QString frameText = _publicInterface->tr("Frame");
                frameText.append(QString::fromUtf8(" ") + QString::number(it2->first));
                viewer->getViewer()->renderText(framePos.x(), framePos.y(), frameText, QColor(200,200,200), font);
                
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
                    if (prev != keysToRender.begin()) {
                        --prev;
                    } else {
                        prev = keysToRender.end();
                    }
                    if (next == keysToRender.end() && time < currentTime) {
                        //Beyond the last keyframe
                        glColor4f(0.93, 0.54, 0, 1);
                    } else if (prev == keysToRender.end() && time > currentTime) {
                        //Before the first keyframe
                        glColor4f(0.93, 0.54, 0, 1);
                    } else {
                        if (time < currentTime) {
                            assert(next != keysToRender.end());
                            if (next->first > currentTime) {
                                glColor4f(1, 0.75, 0.47, 1);
                            } else {
                                glColor4f(1., 1., 1., 0.5);
                            }
                                
                        } else {
                            //time > currentTime
                            assert(prev != keysToRender.end());
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
                    
                    glColor4f(0.8 *l, 0.8 *l, 0.8 *l, 1);
                    
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
    
}

void
TrackerGuiPrivate::drawSelectedMarkerTexture(const std::pair<double,double>& pixelScale,
                                             int currentTime,
                                             const Natron::Point& ptnCenter,
                                             const Natron::Point& offset,
                                             const Natron::Point& ptnTopLeft,
                                             const Natron::Point& ptnTopRight,
                                             const Natron::Point& ptnBtmRight,
                                             const Natron::Point& ptnBtmLeft,
                                             const Natron::Point& selectedSearchWndBtmLeft,
                                             const Natron::Point& selectedSearchWndTopRight)
{
    boost::shared_ptr<TrackMarker> marker = selectedMarker.lock();
    if (!selectedMarkerTexture || !marker) {
        return;
    }
    
    RectD textureRectCanonical;
    computeSelectedMarkerCanonicalRect(&textureRectCanonical);


    
    const TextureRect& texRect = selectedMarkerTexture->getTextureRect();
    RectI texCoords;
    texCoords.x1 = (texRect.x1 - selectedMarkerTextureRoI.x1) / (double)selectedMarkerTextureRoI.width();
    texCoords.y1 = (texRect.y1 - selectedMarkerTextureRoI.y1) / (double)selectedMarkerTextureRoI.height();
    assert(texRect.x2 <=  selectedMarkerTextureRoI.x2);
    texCoords.x2 = (texRect.x2 - selectedMarkerTextureRoI.x1) / (double)selectedMarkerTextureRoI.width();
    assert(texRect.y2 <=  selectedMarkerTextureRoI.y2);
    texCoords.y2 = (texRect.y2 - selectedMarkerTextureRoI.y1) / (double)selectedMarkerTextureRoI.height();
    
    Natron::Point centerPoint, innerTopLeft,innerTopRight,innerBtmLeft,innerBtmRight;
    
    //Remove any offset to the center to see the marker in the magnification window
    double xCenterPercent = (ptnCenter.x - selectedSearchWndBtmLeft.x + offset.x) / (selectedSearchWndTopRight.x - selectedSearchWndBtmLeft.x);
    double yCenterPercent = (ptnCenter.y - selectedSearchWndBtmLeft.y + offset.y) / (selectedSearchWndTopRight.y - selectedSearchWndBtmLeft.y);
    centerPoint.y = textureRectCanonical.y1 + yCenterPercent * (textureRectCanonical.y2 - textureRectCanonical.y1);
    centerPoint.x = textureRectCanonical.x1 + xCenterPercent * (textureRectCanonical.x2 - textureRectCanonical.x1);
    
    
    innerTopLeft = toMagWindowPoint(ptnTopLeft, selectedSearchWndBtmLeft, selectedSearchWndTopRight, textureRectCanonical);
    innerTopRight = toMagWindowPoint(ptnTopRight, selectedSearchWndBtmLeft, selectedSearchWndTopRight, textureRectCanonical);
    innerBtmLeft = toMagWindowPoint(ptnBtmLeft, selectedSearchWndBtmLeft, selectedSearchWndTopRight, textureRectCanonical);
    innerBtmRight = toMagWindowPoint(ptnBtmRight, selectedSearchWndBtmLeft, selectedSearchWndTopRight, textureRectCanonical);
    
    Transform::Point3D btmLeftTex,topLeftTex,topRightTex,btmRightTex;
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
    glBindTexture(GL_TEXTURE_2D, selectedMarkerTexture->getTexID());
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
    
    const QFont& font = viewer->font();
    QFontMetrics fm(font);
    
    QPointF textPos = viewer->getViewer()->toZoomCoordinates(QPointF(5, fm.height() + 5));
    viewer->getViewer()->renderText(textPos.x(), textPos.y(), QString::fromUtf8(marker->getLabel().c_str()), QColor(200,200,200), font);
    
    //Draw internal marker

    for (int l = 0; l < 2; ++l) {
        // shadow (uses GL_PROJECTION)
        glMatrixMode(GL_PROJECTION);
        int direction = (l == 0) ? 1 : -1;
        // translate (1,-1) pixels
        glTranslated(direction * pixelScale.first / 256, -direction * pixelScale.second / 256, 0);
        glMatrixMode(GL_MODELVIEW);
        
        glColor4f(0.8 *l, 0.8 *l, 0.8 *l, 1);
        
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
        if (eventState == eMouseStateScalingSelectedMarker || hoverState == eDrawStateShowScalingHint) {
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
            double rx = std::sqrt((lastMousePos.x() - centerPoint.x) * (lastMousePos.x() - centerPoint.x) + (lastMousePos.y() - centerPoint.y) * (lastMousePos.y() - centerPoint.y));
            double ry = rx;
            drawEllipse(centerPoint.x, centerPoint.y, rx, ry, l, ellipseColor[0], ellipseColor[1], ellipseColor[2], 1.);
        }
        
    }
    
    ///Now draw keyframes
    drawSelectedMarkerKeyframes(pixelScale,currentTime);
    
    
}

static bool isNearbyPoint(const boost::shared_ptr<KnobDouble>& knob,
                          ViewerGL* viewer,
                          double xWidget, double yWidget,
                          double toleranceWidget,
                          double time)
{
    QPointF p;
    p.rx() = knob->getValueAtTime(time, 0);
    p.ry() = knob->getValueAtTime(time, 1);
    p = viewer->toWidgetCoordinates(p);
    if (p.x() <= (xWidget + toleranceWidget) && p.x() >= (xWidget - toleranceWidget) &&
        p.y() <= (yWidget + toleranceWidget) && p.y() >= (yWidget - toleranceWidget)) {
        return true;
    }
    return false;
}

static bool isNearbyPoint(const QPointF& p,
                          ViewerGL* viewer,
                          double xWidget, double yWidget,
                          double toleranceWidget)
{
    QPointF pw = viewer->toWidgetCoordinates(p);
    if (pw.x() <= (xWidget + toleranceWidget) && pw.x() >= (xWidget - toleranceWidget) &&
        pw.y() <= (yWidget + toleranceWidget) && pw.y() >= (yWidget - toleranceWidget)) {
        return true;
    }
    return false;
}

static void findLineIntersection(const Natron::Point& p, const Natron::Point& l1, const Natron::Point& l2, Natron::Point* inter)
{
    Natron::Point h,u;
    double a;
    h.x = p.x - l1.x;
    h.y = p.y - l1.y;
    
    u.x = l2.x - l1.x;
    u.y = l2.y - l1.y;
    
    a = (u.x * h.x + u.y * h.y) / (u.x * u.x + u.y * u.y);
    inter->x = l1.x + u.x * a;
    inter->y = l1.y + u.y * a;
}

bool
TrackerGui::penDown(double time,
                    const RenderScale& renderScale,
                    ViewIdx view,
                    const QPointF & viewportPos,
                    const QPointF & pos,
                    double pressure,
                    QMouseEvent* e)
{
    FLAG_DURING_INTERACT
    
    std::pair<double,double> pixelScale;
    ViewerGL* viewer = _imp->viewer->getViewer();
    viewer->getPixelScale(pixelScale.first, pixelScale.second);
    bool didSomething = false;
 
    
    if (_imp->panelv1) {
        
        const std::list<std::pair<NodeWPtr,bool> > & instances = _imp->panelv1->getInstances();
        for (std::list<std::pair<NodeWPtr,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            
            NodePtr instance = it->first.lock();
            if ( it->second && !instance->isNodeDisabled() ) {
                EffectInstPtr effect = instance->getEffectInstance();
                assert(effect);
                effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                didSomething = effect->onOverlayPenDown_public(time, renderScale, view, viewportPos, pos, pressure);
            }
        }
        
        double selectionTol = pixelScale.first * 10.;
        for (std::list<std::pair<NodeWPtr,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            
            NodePtr instance = it->first.lock();
            boost::shared_ptr<KnobI> newInstanceKnob = instance->getKnobByName("center");
            assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
            KnobDouble* dblKnob = dynamic_cast<KnobDouble*>( newInstanceKnob.get() );
            assert(dblKnob);
            double x,y;
            x = dblKnob->getValueAtTime(time, 0);
            y = dblKnob->getValueAtTime(time, 1);
            
            if ( ( pos.x() >= (x - selectionTol) ) && ( pos.x() <= (x + selectionTol) ) &&
                ( pos.y() >= (y - selectionTol) ) && ( pos.y() <= (y + selectionTol) ) ) {
                if (!it->second) {
                    _imp->panelv1->selectNode( instance, modCASIsShift(e) );
                    
                }
                didSomething = true;
            }
        }
        
        if (_imp->clickToAddTrackEnabled && !didSomething) {
            NodePtr newInstance = _imp->panelv1->createNewInstance(true);
            boost::shared_ptr<KnobI> newInstanceKnob = newInstance->getKnobByName("center");
            assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
            KnobDouble* dblKnob = dynamic_cast<KnobDouble*>( newInstanceKnob.get() );
            assert(dblKnob);
            dblKnob->beginChanges();
            dblKnob->blockValueChanges();
            dblKnob->setValueAtTime(time, pos.x(), view, 0);
            dblKnob->setValueAtTime(time, pos.y(), view, 1);
            dblKnob->unblockValueChanges();
            dblKnob->endChanges();
            didSomething = true;
        }
        
        if ( !didSomething && !modCASIsShift(e) ) {
            _imp->panelv1->clearSelection();
        }
    } else { // if (_imp->panelv1) {
        
        boost::shared_ptr<TrackerContext> context = _imp->panel->getContext();
        std::vector<boost::shared_ptr<TrackMarker> > allMarkers;
        context->getAllMarkers(&allMarkers);
        for (std::vector<boost::shared_ptr<TrackMarker> >::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {
            if (!(*it)->isEnabled()) {
                continue;
            }
            
            bool isSelected = context->isMarkerSelected((*it));
            
            boost::shared_ptr<KnobDouble> centerKnob = (*it)->getCenterKnob();
            boost::shared_ptr<KnobDouble> offsetKnob = (*it)->getOffsetKnob();
            boost::shared_ptr<KnobDouble> ptnTopLeft = (*it)->getPatternTopLeftKnob();
            boost::shared_ptr<KnobDouble> ptnTopRight = (*it)->getPatternTopRightKnob();
            boost::shared_ptr<KnobDouble> ptnBtmRight = (*it)->getPatternBtmRightKnob();
            boost::shared_ptr<KnobDouble> ptnBtmLeft = (*it)->getPatternBtmLeftKnob();
            
            boost::shared_ptr<KnobDouble> searchWndTopRight = (*it)->getSearchWindowTopRightKnob();
            boost::shared_ptr<KnobDouble> searchWndBtmLeft = (*it)->getSearchWindowBottomLeftKnob();
            
            
            QPointF centerPoint;
            centerPoint.rx() = centerKnob->getValueAtTime(time, 0);
            centerPoint.ry() = centerKnob->getValueAtTime(time, 1);
            
            QPointF offset;
            offset.rx() = offsetKnob->getValueAtTime(time, 0);
            offset.ry() = offsetKnob->getValueAtTime(time, 1);
            
            if (isNearbyPoint(centerKnob, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE, time)) {
                if (_imp->controlDown > 0) {
                    _imp->eventState = eMouseStateDraggingOffset;
                } else {
                    _imp->eventState = eMouseStateDraggingCenter;
                }
                _imp->interactMarker = *it;
                didSomething = true;
            } else if ((offset.x() != 0 || offset.y() != 0) && isNearbyPoint(QPointF(centerPoint.x() + offset.x(), centerPoint.y() + offset.y()), viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                _imp->eventState = eMouseStateDraggingOffset;
                _imp->interactMarker = *it;
                didSomething = true;
            }
            
            if (!didSomething && isSelected) {
                
                QPointF topLeft,topRight,btmRight,btmLeft;
                topLeft.rx() = ptnTopLeft->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
                topLeft.ry() = ptnTopLeft->getValueAtTime(time, 1) + offset.y() + centerPoint.y();
                
                topRight.rx() = ptnTopRight->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
                topRight.ry() = ptnTopRight->getValueAtTime(time, 1) + offset.y() + centerPoint.y();
                
                btmRight.rx() = ptnBtmRight->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
                btmRight.ry() = ptnBtmRight->getValueAtTime(time, 1) + offset.y() + centerPoint.y();
                
                btmLeft.rx() = ptnBtmLeft->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
                btmLeft.ry() = ptnBtmLeft->getValueAtTime(time, 1) + offset.y() + centerPoint.y();
                
                QPointF midTop,midRight,midBtm,midLeft;
                midTop.rx() = (topLeft.x() + topRight.x()) / 2.;
                midTop.ry() = (topLeft.y() + topRight.y()) / 2.;
                
                midRight.rx() = (btmRight.x() + topRight.x()) / 2.;
                midRight.ry() = (btmRight.y() + topRight.y()) / 2.;
                
                midBtm.rx() = (btmRight.x() + btmLeft.x()) / 2.;
                midBtm.ry() = (btmRight.y() + btmLeft.y()) / 2.;
                
                midLeft.rx() = (topLeft.x() + btmLeft.x()) / 2.;
                midLeft.ry() = (topLeft.y() + btmLeft.y()) / 2.;
                
                if (isSelected && isNearbyPoint(topLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingInnerTopLeft;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isSelected && isNearbyPoint(topRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingInnerTopRight;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isSelected && isNearbyPoint(btmRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingInnerBtmRight;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isSelected && isNearbyPoint(btmLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingInnerBtmLeft;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isSelected && isNearbyPoint(midTop, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingInnerTopMid;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isSelected && isNearbyPoint(midRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingInnerMidRight;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isSelected && isNearbyPoint(midLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingInnerMidLeft;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isSelected && isNearbyPoint(midBtm, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingInnerBtmMid;
                    _imp->interactMarker = *it;
                    didSomething = true;
                }
            }
            if (!didSomething && isSelected) {
              
                
                ///Test search window
                QPointF searchTopLeft,searchTopRight,searchBtmRight,searchBtmLeft;
                searchTopRight.rx() = searchWndTopRight->getValueAtTime(time, 0) + centerPoint.x() + offset.x();
                searchTopRight.ry() = searchWndTopRight->getValueAtTime(time, 1) + centerPoint.y() + offset.y();
                
                searchBtmLeft.rx() = searchWndBtmLeft->getValueAtTime(time, 0) + centerPoint.x() + offset.x();
                searchBtmLeft.ry() = searchWndBtmLeft->getValueAtTime(time, 1) + + centerPoint.y() + offset.y();
                
                searchTopLeft.rx() = searchBtmLeft.x();
                searchTopLeft.ry() = searchTopRight.y();
                
                searchBtmRight.rx() = searchTopRight.x();
                searchBtmRight.ry() = searchBtmLeft.y();
                
                QPointF searchTopMid,searchRightMid,searchLeftMid,searchBtmMid;
                searchTopMid.rx() = (searchTopLeft.x() + searchTopRight.x()) / 2.;
                searchTopMid.ry() = (searchTopLeft.y() + searchTopRight.y()) / 2.;
                
                searchRightMid.rx() = (searchBtmRight.x() + searchTopRight.x()) / 2.;
                searchRightMid.ry() = (searchBtmRight.y() + searchTopRight.y()) / 2.;
                
                searchBtmMid.rx() = (searchBtmRight.x() + searchBtmLeft.x()) / 2.;
                searchBtmMid.ry() = (searchBtmRight.y() + searchBtmLeft.y()) / 2.;
                
                searchLeftMid.rx() = (searchTopLeft.x() + searchBtmLeft.x()) / 2.;
                searchLeftMid.ry() = (searchTopLeft.y() + searchBtmLeft.y()) / 2.;
                
                if (isNearbyPoint(searchTopLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingOuterTopLeft;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isNearbyPoint(searchTopRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingOuterTopRight;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isNearbyPoint(searchBtmRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingOuterBtmRight;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isNearbyPoint(searchBtmLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingOuterBtmLeft;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isNearbyPoint(searchTopMid, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingOuterTopMid;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isNearbyPoint(searchBtmMid, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingOuterBtmMid;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isNearbyPoint(searchLeftMid, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingOuterMidLeft;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if (isNearbyPoint(searchRightMid, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->eventState = eMouseStateDraggingOuterMidRight;
                    _imp->interactMarker = *it;
                    didSomething = true;
                }
            }
            
            //If we hit the interact, make sure it is selected
            if (_imp->interactMarker) {
                if (!isSelected) {
                    context->addTrackToSelection(_imp->interactMarker, TrackerContext::eTrackSelectionViewer);
                }
            }
            
            if (didSomething) {
                break;
            }
        } // for (std::vector<boost::shared_ptr<TrackMarker> >::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {
        
        if (_imp->clickToAddTrackEnabled && !didSomething) {
            boost::shared_ptr<TrackMarker> marker = context->createMarker();
            boost::shared_ptr<KnobDouble> centerKnob = marker->getCenterKnob();
            centerKnob->setValuesAtTime(time, pos.x(), pos.y(), view, Natron::eValueChangedReasonNatronInternalEdited);
            if (_imp->createKeyOnMoveButton->isChecked()) {
                marker->setUserKeyframe(time);
            }
            _imp->panel->pushUndoCommand(new AddTrackCommand(marker,context));
            updateSelectedMarkerTexture();
            didSomething = true;
        }
        
        if (!didSomething && _imp->showMarkerTexture && _imp->selectedMarkerTexture && _imp->isNearbySelectedMarkerTextureResizeAnchor(pos)) {
            _imp->eventState = eMouseStateDraggingSelectedMarkerResizeAnchor;
            didSomething = true;
        }
        
        if (!didSomething && _imp->showMarkerTexture && _imp->selectedMarkerTexture  && _imp->isInsideSelectedMarkerTextureResizeAnchor(pos)) {
            if (_imp->shiftDown) {
                _imp->eventState = eMouseStateScalingSelectedMarker;
            } else {
                _imp->eventState = eMouseStateDraggingSelectedMarker;
            }
            _imp->interactMarker = _imp->selectedMarker.lock();
            didSomething = true;
        }
        
        if (!didSomething) {
            int keyTime = _imp->isInsideKeyFrameTexture(time, pos, viewportPos);
            if (keyTime != INT_MAX) {
                _imp->viewer->seek(keyTime);
                didSomething = true;
            }
        }
        if (!didSomething) {
            std::list<boost::shared_ptr<TrackMarker> > selectedMarkers;
            context->getSelectedMarkers(&selectedMarkers);
            if (!selectedMarkers.empty()) {
                context->clearSelection(TrackerContext::eTrackSelectionViewer);

                didSomething = true;
            }
        }
    }
    _imp->lastMousePos = pos;
    
    return didSomething;
} // penDown

bool
TrackerGui::penDoubleClicked(double /*time*/,
                             const RenderScale& /*renderScale*/,
                             ViewIdx /*view*/,
                             const QPointF & /*viewportPos*/,
                             const QPointF & /*pos*/,
                             QMouseEvent* /*e*/)
{
    bool didSomething = false;

    return didSomething;
}

void
TrackerGuiPrivate::transformPattern(double time, TrackerMouseStateEnum state, const Natron::Point& delta)
{
    boost::shared_ptr<KnobDouble> searchWndTopRight,searchWndBtmLeft;
    boost::shared_ptr<KnobDouble> patternCorners[4];
    boost::shared_ptr<TrackerContext> context = panel->getContext();
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
    searchPoints[1].rx()= searchWndBtmLeft->getValueAtTime(time , 0) + centerPoint.x() + offset.x();
    searchPoints[1].ry()= searchWndBtmLeft->getValueAtTime(time , 1) + centerPoint.y() + offset.y();
    
    searchPoints[3].rx()= searchWndTopRight->getValueAtTime(time , 0) + centerPoint.x() + offset.x();
    searchPoints[3].ry()= searchWndTopRight->getValueAtTime(time , 1) + centerPoint.y() + offset.y();
    
    searchPoints[0].rx() = searchPoints[1].x();
    searchPoints[0].ry() = searchPoints[3].y();
    
    searchPoints[2].rx() = searchPoints[3].x();
    searchPoints[2].ry() = searchPoints[1].y();
    
    if (state == eMouseStateDraggingInnerBtmLeft ||
        state == eMouseStateDraggingOuterBtmLeft) {
        if (transformPatternCorners) {
            patternPoints[1].rx() += delta.x;
            patternPoints[1].ry() += delta.y;
            
            patternPoints[0].rx() += delta.x;
            patternPoints[0].ry() -= delta.y;
            
            patternPoints[2].rx() -= delta.x;
            patternPoints[2].ry() += delta.y;
            
            patternPoints[3].rx() -= delta.x;
            patternPoints[3].ry() -= delta.y;
        }
        
        searchPoints[1].rx() += delta.x;
        searchPoints[1].ry() += delta.y;
        
        searchPoints[0].rx() += delta.x;
        searchPoints[0].ry() -= delta.y;
        
        searchPoints[2].rx() -= delta.x;
        searchPoints[2].ry() += delta.y;
        
        searchPoints[3].rx() -= delta.x;
        searchPoints[3].ry() -= delta.y;
    } else if (state == eMouseStateDraggingInnerBtmRight ||
               state == eMouseStateDraggingOuterBtmRight) {
        
        if (transformPatternCorners) {
            patternPoints[1].rx() -= delta.x;
            patternPoints[1].ry() += delta.y;
            
            patternPoints[0].rx() -= delta.x;
            patternPoints[0].ry() -= delta.y;
            
            patternPoints[2].rx() += delta.x;
            patternPoints[2].ry() += delta.y;
            
            patternPoints[3].rx() += delta.x;
            patternPoints[3].ry() -= delta.y;
        }
        
        searchPoints[1].rx() -= delta.x;
        searchPoints[1].ry() += delta.y;
        
        searchPoints[0].rx() -= delta.x;
        searchPoints[0].ry() -= delta.y;
        
        searchPoints[2].rx() += delta.x;
        searchPoints[2].ry() += delta.y;
        
        searchPoints[3].rx() += delta.x;
        searchPoints[3].ry() -= delta.y;
        
    } else if (state == eMouseStateDraggingInnerTopRight ||
               state == eMouseStateDraggingOuterTopRight) {
        
        if (transformPatternCorners) {
            patternPoints[1].rx() -= delta.x;
            patternPoints[1].ry() -= delta.y;
            
            patternPoints[0].rx() -= delta.x;
            patternPoints[0].ry() += delta.y;
            
            patternPoints[2].rx() += delta.x;
            patternPoints[2].ry() -= delta.y;
            
            patternPoints[3].rx() += delta.x;
            patternPoints[3].ry() += delta.y;
        }
        
        searchPoints[1].rx() -= delta.x;
        searchPoints[1].ry() -= delta.y;
        
        searchPoints[0].rx() -= delta.x;
        searchPoints[0].ry() += delta.y;
        
        searchPoints[2].rx() += delta.x;
        searchPoints[2].ry() -= delta.y;
        
        searchPoints[3].rx() += delta.x;
        searchPoints[3].ry() += delta.y;
    } else if (state == eMouseStateDraggingInnerTopLeft ||
               state == eMouseStateDraggingOuterTopLeft) {
        
        if (transformPatternCorners) {
            patternPoints[1].rx() += delta.x;
            patternPoints[1].ry() -= delta.y;
            
            patternPoints[0].rx() += delta.x;
            patternPoints[0].ry() += delta.y;
            
            patternPoints[2].rx() -= delta.x;
            patternPoints[2].ry() -= delta.y;
            
            patternPoints[3].rx() -= delta.x;
            patternPoints[3].ry() += delta.y;
        }
        
        searchPoints[1].rx() += delta.x;
        searchPoints[1].ry() -= delta.y;
        
        searchPoints[0].rx() += delta.x;
        searchPoints[0].ry() += delta.y;
        
        searchPoints[2].rx() -= delta.x;
        searchPoints[2].ry() -= delta.y;
        
        searchPoints[3].rx() -= delta.x;
        searchPoints[3].ry() += delta.y;
    } else if (state == eMouseStateDraggingInnerBtmMid ||
               state == eMouseStateDraggingOuterBtmMid) {
        if (transformPatternCorners) {
            patternPoints[1].ry() += delta.y;
            patternPoints[2].ry() += delta.y;
            patternPoints[0].ry() -= delta.y;
            patternPoints[3].ry() -= delta.y;
        }
        searchPoints[1].ry() += delta.y;
        searchPoints[2].ry() += delta.y;
        searchPoints[0].ry() -= delta.y;
        searchPoints[3].ry() -= delta.y;
    } else if (state == eMouseStateDraggingInnerTopMid ||
               state == eMouseStateDraggingOuterTopMid) {
        if (transformPatternCorners) {
            patternPoints[1].ry() -= delta.y;
            patternPoints[2].ry() -= delta.y;
            patternPoints[0].ry() += delta.y;
            patternPoints[3].ry() += delta.y;
        }
        searchPoints[1].ry() -= delta.y;
        searchPoints[2].ry() -= delta.y;
        searchPoints[0].ry() += delta.y;
        searchPoints[3].ry() += delta.y;
    } else if (state == eMouseStateDraggingInnerMidLeft ||
               state == eMouseStateDraggingOuterMidLeft) {
        if (transformPatternCorners) {
            patternPoints[1].rx() += delta.x;
            patternPoints[2].rx() -= delta.x;
            patternPoints[0].rx() += delta.x;
            patternPoints[3].rx() -= delta.x;
        }
        searchPoints[1].rx() += delta.x;
        searchPoints[2].rx() -= delta.x;
        searchPoints[0].rx() += delta.x;
        searchPoints[3].rx() -= delta.x;
    } else if (state == eMouseStateDraggingInnerMidRight ||
               state == eMouseStateDraggingOuterMidRight) {
        if (transformPatternCorners) {
            patternPoints[1].rx() -= delta.x;
            patternPoints[2].rx() += delta.x;
            patternPoints[0].rx() -= delta.x;
            patternPoints[3].rx() += delta.x;
        }
        searchPoints[1].rx() -= delta.x;
        searchPoints[2].rx() += delta.x;
        searchPoints[0].rx() -= delta.x;
        searchPoints[3].rx() += delta.x;
    }

    EffectInstPtr effect = context->getNode()->getEffectInstance();
    effect->beginChanges();
    
    if (transformPatternCorners) {
        for (int i = 0; i < 4; ++i) {
            
            patternPoints[i].rx() -= (centerPoint.x() + offset.x());
            patternPoints[i].ry() -= (centerPoint.y() + offset.y());
            
            
            if (patternCorners[i]->hasAnimation()) {
                patternCorners[i]->setValuesAtTime(time, patternPoints[i].x(), patternPoints[i].y(), ViewSpec::current(), Natron::eValueChangedReasonNatronInternalEdited);
            } else {
                patternCorners[i]->setValues(patternPoints[i].x(), patternPoints[i].y(), ViewSpec::current(), Natron::eValueChangedReasonNatronInternalEdited);
            }
        }
    }
    searchPoints[1].rx() -= (centerPoint.x() + offset.x());
    searchPoints[1].ry() -= (centerPoint.y() + offset.y());
    
    searchPoints[3].rx() -= (centerPoint.x() + offset.x());
    searchPoints[3].ry() -= (centerPoint.y() + offset.y());
    
    if (searchWndBtmLeft->hasAnimation()) {
        searchWndBtmLeft->setValuesAtTime(time, searchPoints[1].x(), searchPoints[1].y(),ViewSpec::current(),  Natron::eValueChangedReasonNatronInternalEdited);
    } else {
        searchWndBtmLeft->setValues(searchPoints[1].x(), searchPoints[1].y(), ViewSpec::current(), Natron::eValueChangedReasonNatronInternalEdited);
    }
    
    if (searchWndTopRight->hasAnimation()) {
        searchWndTopRight->setValuesAtTime(time, searchPoints[3].x(), searchPoints[3].y(), ViewSpec::current(), Natron::eValueChangedReasonNatronInternalEdited);
    } else {
        searchWndTopRight->setValues(searchPoints[3].x(), searchPoints[3].y(), ViewSpec::current(), Natron::eValueChangedReasonNatronInternalEdited);
    }
    effect->endChanges();
    
    refreshSelectedMarkerTexture();
    
    if (createKeyOnMoveButton->isChecked()) {
        interactMarker->setUserKeyframe(time);
    }
}

bool
TrackerGui::penMotion(double time,
                      const RenderScale & renderScale,
                      ViewIdx view,
                      const QPointF & viewportPos,
                      const QPointF & pos,
                      double pressure,
                      QInputEvent* /*e*/)
{
    FLAG_DURING_INTERACT
    
    std::pair<double,double> pixelScale;
    ViewerGL* viewer = _imp->viewer->getViewer();
    viewer->getPixelScale(pixelScale.first, pixelScale.second);
    bool didSomething = false;
    
    
    Natron::Point delta;
    delta.x = pos.x() - _imp->lastMousePos.x();
    delta.y = pos.y() - _imp->lastMousePos.y();
    
    if (_imp->panelv1) {
        const std::list<std::pair<NodeWPtr,bool> > & instances = _imp->panelv1->getInstances();
        
        for (std::list<std::pair<NodeWPtr,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            
            NodePtr instance = it->first.lock();
            if ( it->second && !instance->isNodeDisabled() ) {
                EffectInstPtr effect = instance->getEffectInstance();
                assert(effect);
                effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                if ( effect->onOverlayPenMotion_public(time, renderScale, view, viewportPos, pos, pressure) ) {
                    didSomething = true;
                }
            }
        }
    } else {
        
        if (_imp->hoverState != eDrawStateInactive) {
            _imp->hoverState = eDrawStateInactive;
            _imp->hoverMarker.reset();
            didSomething = true;
        }
        
        boost::shared_ptr<TrackerContext> context = _imp->panel->getContext();
        std::vector<boost::shared_ptr<TrackMarker> > allMarkers;
        context->getAllMarkers(&allMarkers);
        
        bool hoverProcess = false;
        for (std::vector<boost::shared_ptr<TrackMarker> >::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {
            if (!(*it)->isEnabled()) {
                continue;
            }
            
            bool isSelected = context->isMarkerSelected((*it));
            
            boost::shared_ptr<KnobDouble> centerKnob = (*it)->getCenterKnob();
            boost::shared_ptr<KnobDouble> offsetKnob = (*it)->getOffsetKnob();
            
            boost::shared_ptr<KnobDouble> ptnTopLeft = (*it)->getPatternTopLeftKnob();
            boost::shared_ptr<KnobDouble> ptnTopRight = (*it)->getPatternTopRightKnob();
            boost::shared_ptr<KnobDouble> ptnBtmRight = (*it)->getPatternBtmRightKnob();
            boost::shared_ptr<KnobDouble> ptnBtmLeft = (*it)->getPatternBtmLeftKnob();
            
            boost::shared_ptr<KnobDouble> searchWndTopRight = (*it)->getSearchWindowTopRightKnob();
            boost::shared_ptr<KnobDouble> searchWndBtmLeft = (*it)->getSearchWindowBottomLeftKnob();
            
            
            QPointF centerPoint;
            centerPoint.rx() = centerKnob->getValueAtTime(time, 0);
            centerPoint.ry() = centerKnob->getValueAtTime(time, 1);
            
            QPointF offset;
            offset.rx() = offsetKnob->getValueAtTime(time, 0);
            offset.ry() = offsetKnob->getValueAtTime(time, 1);
            if (isNearbyPoint(centerKnob, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE, time)) {
                _imp->hoverState = eDrawStateHoveringCenter;
                _imp->hoverMarker = *it;
                hoverProcess = true;
            } else if ((offset.x() != 0 || offset.y() != 0) && isNearbyPoint(QPointF(centerPoint.x() + offset.x(), centerPoint.y() + offset.y()), viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                _imp->hoverState = eDrawStateHoveringCenter;
                _imp->hoverMarker = *it;
                didSomething = true;
            }
            
            
            
            if (!hoverProcess) {
                QPointF topLeft,topRight,btmRight,btmLeft;
                topLeft.rx() = ptnTopLeft->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
                topLeft.ry() = ptnTopLeft->getValueAtTime(time, 1) + offset.y() + centerPoint.y();
                
                topRight.rx() = ptnTopRight->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
                topRight.ry() = ptnTopRight->getValueAtTime(time, 1) + offset.y() + centerPoint.y();
                
                btmRight.rx() = ptnBtmRight->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
                btmRight.ry() = ptnBtmRight->getValueAtTime(time, 1) + offset.y() + centerPoint.y();
                
                btmLeft.rx() = ptnBtmLeft->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
                btmLeft.ry() = ptnBtmLeft->getValueAtTime(time, 1) + offset.y() + centerPoint.y();
                
                QPointF midTop,midRight,midBtm,midLeft;
                midTop.rx() = (topLeft.x() + topRight.x()) / 2.;
                midTop.ry() = (topLeft.y() + topRight.y()) / 2.;
                
                midRight.rx() = (btmRight.x() + topRight.x()) / 2.;
                midRight.ry() = (btmRight.y() + topRight.y()) / 2.;
                
                midBtm.rx() = (btmRight.x() + btmLeft.x()) / 2.;
                midBtm.ry() = (btmRight.y() + btmLeft.y()) / 2.;
                
                midLeft.rx() = (topLeft.x() + btmLeft.x()) / 2.;
                midLeft.ry() = (topLeft.y() + btmLeft.y()) / 2.;

                
                if (isSelected && isNearbyPoint(topLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringInnerTopLeft;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isSelected && isNearbyPoint(topRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringInnerTopRight;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isSelected && isNearbyPoint(btmRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringInnerBtmRight;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isSelected && isNearbyPoint(btmLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringInnerBtmLeft;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isSelected && isNearbyPoint(midTop, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringInnerTopMid;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isSelected && isNearbyPoint(midRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringInnerMidRight;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isSelected && isNearbyPoint(midLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringInnerMidLeft;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isSelected && isNearbyPoint(midBtm, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringInnerBtmMid;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                }
            }
            if (!hoverProcess && isSelected) {
               
                
                ///Test search window
                QPointF searchTopLeft,searchTopRight,searchBtmRight,searchBtmLeft;
                searchTopRight.rx() = searchWndTopRight->getValueAtTime(time, 0) + centerPoint.x() + offset.x();
                searchTopRight.ry() = searchWndTopRight->getValueAtTime(time, 1) + centerPoint.y() + offset.y();
                
                searchBtmLeft.rx() = searchWndBtmLeft->getValueAtTime(time, 0) + centerPoint.x() + offset.x();
                searchBtmLeft.ry() = searchWndBtmLeft->getValueAtTime(time, 1) + + centerPoint.y() + offset.y();
                
                searchTopLeft.rx() = searchBtmLeft.x();
                searchTopLeft.ry() = searchTopRight.y();
                
                searchBtmRight.rx() = searchTopRight.x();
                searchBtmRight.ry() = searchBtmLeft.y();
                
                QPointF searchTopMid,searchRightMid,searchLeftMid,searchBtmMid;
                searchTopMid.rx() = (searchTopLeft.x() + searchTopRight.x()) / 2.;
                searchTopMid.ry() = (searchTopLeft.y() + searchTopRight.y()) / 2.;
                
                searchRightMid.rx() = (searchBtmRight.x() + searchTopRight.x()) / 2.;
                searchRightMid.ry() = (searchBtmRight.y() + searchTopRight.y()) / 2.;
                
                searchBtmMid.rx() = (searchBtmRight.x() + searchBtmLeft.x()) / 2.;
                searchBtmMid.ry() = (searchBtmRight.y() + searchBtmLeft.y()) / 2.;
                
                searchLeftMid.rx() = (searchTopLeft.x() + searchBtmLeft.x()) / 2.;
                searchLeftMid.ry() = (searchTopLeft.y() + searchBtmLeft.y()) / 2.;
                
                if (isNearbyPoint(searchTopLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringOuterTopLeft;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isNearbyPoint(searchTopRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringOuterTopRight;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isNearbyPoint(searchBtmRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringOuterBtmRight;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isNearbyPoint(searchBtmLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringOuterBtmLeft;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isNearbyPoint(searchTopMid, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringOuterTopMid;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isNearbyPoint(searchBtmMid, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringOuterBtmMid;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isNearbyPoint(searchLeftMid, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringOuterMidLeft;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if (isNearbyPoint(searchRightMid, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE)) {
                    _imp->hoverState = eDrawStateHoveringOuterMidRight;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                }


            }
            
            if (hoverProcess) {
                break;
            }
        } // for (std::vector<boost::shared_ptr<TrackMarker> >::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {
        
        if (_imp->showMarkerTexture && _imp->selectedMarkerTexture && _imp->isNearbySelectedMarkerTextureResizeAnchor(pos)) {
            _imp->viewer->getViewer()->setCursor(Qt::SizeFDiagCursor);
            hoverProcess = true;
        } else if (_imp->showMarkerTexture && _imp->selectedMarkerTexture && _imp->isInsideSelectedMarkerTextureResizeAnchor(pos)) {
            _imp->viewer->getViewer()->setCursor(Qt::SizeAllCursor);
            hoverProcess = true;
        } else if (_imp->showMarkerTexture && _imp->isInsideKeyFrameTexture(time, pos, viewportPos) != INT_MAX) {
            _imp->viewer->getViewer()->setCursor(Qt::PointingHandCursor);
            hoverProcess = true;
        } else {
            _imp->viewer->getViewer()->unsetCursor();
        }
        
        if (_imp->showMarkerTexture && _imp->selectedMarkerTexture && _imp->shiftDown && _imp->isInsideSelectedMarkerTextureResizeAnchor(pos)) {
            _imp->hoverState = eDrawStateShowScalingHint;
            hoverProcess = true;
        }
        
        if (hoverProcess) {
            didSomething = true;
        }
        
        boost::shared_ptr<KnobDouble> centerKnob,offsetKnob,searchWndTopRight,searchWndBtmLeft;
        boost::shared_ptr<KnobDouble> patternCorners[4];
        if (_imp->interactMarker) {
            centerKnob = _imp->interactMarker->getCenterKnob();
            offsetKnob = _imp->interactMarker->getOffsetKnob();
            
            /*
             
             TopLeft(0) ------------- Top right(3)
             |                        |
             |                        |
             |                        |
             Btm left (1) ------------ Btm right (2)
             
             */
            patternCorners[0] = _imp->interactMarker->getPatternTopLeftKnob();
            patternCorners[1] = _imp->interactMarker->getPatternBtmLeftKnob();
            patternCorners[2] = _imp->interactMarker->getPatternBtmRightKnob();
            patternCorners[3] = _imp->interactMarker->getPatternTopRightKnob();
            searchWndTopRight = _imp->interactMarker->getSearchWindowTopRightKnob();
            searchWndBtmLeft = _imp->interactMarker->getSearchWindowBottomLeftKnob();
        }
        
        
        switch (_imp->eventState) {
            case eMouseStateDraggingCenter:
            case eMouseStateDraggingOffset:
            {
                assert(_imp->interactMarker);
                if (_imp->eventState == eMouseStateDraggingOffset) {
                    offsetKnob->setValues(offsetKnob->getValueAtTime(time,0) + delta.x,
                                          offsetKnob->getValueAtTime(time,1) + delta.y,
                                          view,
                                          Natron::eValueChangedReasonPluginEdited);
                } else {
                    centerKnob->setValuesAtTime(time, centerKnob->getValueAtTime(time,0) + delta.x,
                                                centerKnob->getValueAtTime(time,1) + delta.y,
                                                view,
                                                Natron::eValueChangedReasonPluginEdited);
                    for (int i = 0; i < 4; ++i) {
                        for (int d = 0; d < patternCorners[i]->getDimension(); ++d) {
                            patternCorners[i]->setValueAtTime(time, patternCorners[i]->getValueAtTime(i, d), view,d);
                        }
                    }
                }
                updateSelectedMarkerTexture();
                if (_imp->createKeyOnMoveButton->isChecked()) {
                    _imp->interactMarker->setUserKeyframe(time);
                }
                didSomething = true;
            }   break;
            case eMouseStateDraggingInnerBtmLeft:
            case eMouseStateDraggingInnerTopRight:
            case eMouseStateDraggingInnerTopLeft:
            case eMouseStateDraggingInnerBtmRight:
            {
                
                if (_imp->controlDown == 0) {
                    _imp->transformPattern(time, _imp->eventState, delta);
                    didSomething = true;
                    break;
                }
                
                int index;
                if (_imp->eventState == eMouseStateDraggingInnerBtmLeft) {
                    index = 1;
                } else if (_imp->eventState == eMouseStateDraggingInnerBtmRight) {
                    index = 2;
                } else if (_imp->eventState == eMouseStateDraggingInnerTopRight) {
                    index = 3;
                } else if (_imp->eventState == eMouseStateDraggingInnerTopLeft) {
                    index = 0;
                }
                
                int nextIndex = (index + 1) % 4;
                int prevIndex = (index + 3) % 4;
                int diagIndex = (index + 2) % 4;
                
                
                Natron::Point center,offset;
                center.x = centerKnob->getValueAtTime(time,0);
                center.y = centerKnob->getValueAtTime(time,1);
                
                offset.x = offsetKnob->getValueAtTime(time, 0);
                offset.y = offsetKnob->getValueAtTime(time, 1);
                
                Natron::Point cur,prev,next,diag;
                cur.x = patternCorners[index]->getValueAtTime(time,0) + delta.x  + center.x + offset.x;;
                cur.y = patternCorners[index]->getValueAtTime(time,1) + delta.y  + center.y + offset.y;
                
                prev.x = patternCorners[prevIndex]->getValueAtTime(time,0)  + center.x + offset.x;;
                prev.y = patternCorners[prevIndex]->getValueAtTime(time,1) + center.y + offset.y;
                
                next.x = patternCorners[nextIndex]->getValueAtTime(time,0)  + center.x + offset.x;;
                next.y = patternCorners[nextIndex]->getValueAtTime(time,1)  + center.y + offset.y;
                
                diag.x = patternCorners[diagIndex]->getValueAtTime(time,0)  + center.x + offset.x;;
                diag.y = patternCorners[diagIndex]->getValueAtTime(time,1) + center.y + offset.y;
                
                Natron::Point nextVec;
                nextVec.x = next.x - cur.x;
                nextVec.y = next.y - cur.y;
                
                Natron::Point prevVec;
                prevVec.x = cur.x - prev.x;
                prevVec.y = cur.y - prev.y;
                
                Natron::Point nextDiagVec,prevDiagVec;
                prevDiagVec.x = diag.x - next.x;
                prevDiagVec.y = diag.y - next.y;
                
                nextDiagVec.x = prev.x - diag.x;
                nextDiagVec.y = prev.y - diag.y;
                
                //Clamp so the 4 points remaing the same in the homography
                if (prevVec.x * nextVec.y - prevVec.y * nextVec.x < 0.) { // cross-product
                    findLineIntersection(cur, prev, next, &cur);
                }
                if (nextDiagVec.x * prevVec.y - nextDiagVec.y * prevVec.x < 0.) { // cross-product
                    findLineIntersection(cur, prev, diag, &cur);
                }
                if (nextVec.x * prevDiagVec.y - nextVec.y * prevDiagVec.x < 0.) { // cross-product
                    findLineIntersection(cur, next, diag, &cur);
                }
                
                
                Natron::Point searchWindowCorners[2];
                searchWindowCorners[0].x = searchWndBtmLeft->getValueAtTime(time, 0) + center.x + offset.x;
                searchWindowCorners[0].y = searchWndBtmLeft->getValueAtTime(time, 1) + center.y + offset.y;
                
                searchWindowCorners[1].x = searchWndTopRight->getValueAtTime(time, 0)  + center.x + offset.x;
                searchWindowCorners[1].y = searchWndTopRight->getValueAtTime(time, 1)  + center.y + offset.y;
                
                cur.x = std::max(std::min(cur.x, searchWindowCorners[1].x), searchWindowCorners[0].x);
                cur.y = std::max(std::min(cur.y, searchWindowCorners[1].y), searchWindowCorners[0].y);
                
                cur.x -= (center.x + offset.x);
                cur.y -= (center.y + offset.y);
                
                if (patternCorners[index]->hasAnimation()) {
                    patternCorners[index]->setValuesAtTime(time, cur.x, cur.y,view, Natron::eValueChangedReasonNatronInternalEdited);
                } else {
                    patternCorners[index]->setValues(cur.x, cur.y, view,Natron::eValueChangedReasonNatronInternalEdited);
                }
                if (_imp->createKeyOnMoveButton->isChecked()) {
                    _imp->interactMarker->setUserKeyframe(time);
                }
                didSomething = true;
            }   break;
            case eMouseStateDraggingOuterBtmLeft:
            {
                if (_imp->controlDown == 0) {
                    _imp->transformPattern(time, _imp->eventState, delta);
                    didSomething = true;
                    break;
                }
                Natron::Point center,offset;
                center.x = centerKnob->getValueAtTime(time,0);
                center.y = centerKnob->getValueAtTime(time,1);
                
                offset.x = offsetKnob->getValueAtTime(time, 0);
                offset.y = offsetKnob->getValueAtTime(time, 1);
                
                Natron::Point p;
                p.x = searchWndBtmLeft->getValueAtTime(time, 0) + center.x + offset.x + delta.x;
                p.y = searchWndBtmLeft->getValueAtTime(time, 1) + center.y + offset.y + delta.y;
                
                Natron::Point btmLeft,btmRight,topLeft;
                btmLeft.x = patternCorners[1]->getValueAtTime(time,0) + center.x + offset.x;
                btmLeft.y = patternCorners[1]->getValueAtTime(time,1) + center.y + offset.y;
                btmRight.y = patternCorners[2]->getValueAtTime(time,1) + center.y + offset.y;
                topLeft.x = patternCorners[0]->getValueAtTime(time,0) + center.x + offset.x;

                p.x = std::min(p.x, topLeft.x);
                p.x = std::min(p.x, btmLeft.x);
                p.y = std::min(p.y, btmLeft.y);
                p.y = std::min(p.y, btmRight.y);
                
                p.x -= (center.x + offset.x);
                p.y -= (center.y + offset.y);
                if (searchWndBtmLeft->hasAnimation()) {
                    searchWndBtmLeft->setValuesAtTime(time, p.x, p.y, view,Natron::eValueChangedReasonNatronInternalEdited);
                } else {
                    searchWndBtmLeft->setValues(p.x, p.y,view, Natron::eValueChangedReasonNatronInternalEdited);
                }

                updateSelectedMarkerTexture();
                didSomething = true;
            }   break;
            case eMouseStateDraggingOuterBtmRight:
            {
               
                if (_imp->controlDown == 0) {
                    _imp->transformPattern(time, _imp->eventState, delta);
                    didSomething = true;
                    break;
                }
                Natron::Point center,offset;
                center.x = centerKnob->getValueAtTime(time,0);
                center.y = centerKnob->getValueAtTime(time,1);
                
                offset.x = offsetKnob->getValueAtTime(time, 0);
                offset.y = offsetKnob->getValueAtTime(time, 1);
                
                Natron::Point p;
                p.x = searchWndTopRight->getValueAtTime(time, 0) + center.x + offset.x + delta.x;
                p.y = searchWndBtmLeft->getValueAtTime(time, 1) + center.y + offset.y + delta.y;
                
                Natron::Point btmLeft,btmRight,topRight;
                btmLeft.y = patternCorners[1]->getValueAtTime(time,1) + center.y + offset.y;
                btmRight.x = patternCorners[2]->getValueAtTime(time,0) + center.x + offset.x;
                btmRight.y = patternCorners[2]->getValueAtTime(time,1) + center.y + offset.y;
                topRight.x = patternCorners[0]->getValueAtTime(time,0) + center.x + offset.x;
                p.x = std::max(p.x, topRight.x);
                p.x = std::max(p.x, btmRight.x);
                p.y = std::min(p.y, btmRight.y);
                p.y = std::min(p.y, btmLeft.y);
                
                p.x -= (center.x + offset.x);
                p.y -= (center.y + offset.y);
                if (searchWndBtmLeft->hasAnimation()) {
                    searchWndBtmLeft->setValueAtTime(time, p.y ,view, 1);
                } else {
                    searchWndBtmLeft->setValue(p.y,view,1);
                }
                if (searchWndTopRight->hasAnimation()) {
                    searchWndTopRight->setValueAtTime(time, p.x ,view, 0);
                } else {
                    searchWndTopRight->setValue(p.x,view,0);
                }
                
                updateSelectedMarkerTexture();
                didSomething = true;
            }   break;
            case eMouseStateDraggingOuterTopRight:
            {
                if (_imp->controlDown == 0) {
                    _imp->transformPattern(time, _imp->eventState, delta);
                    didSomething = true;
                    break;
                }
                Natron::Point center,offset;
                center.x = centerKnob->getValueAtTime(time,0);
                center.y = centerKnob->getValueAtTime(time,1);
                
                offset.x = offsetKnob->getValueAtTime(time, 0);
                offset.y = offsetKnob->getValueAtTime(time, 1);
                
                Natron::Point p;
                p.x = searchWndTopRight->getValueAtTime(time, 0) + center.x + offset.x + delta.x;
                p.y = searchWndTopRight->getValueAtTime(time, 1) + center.y + offset.y + delta.y;
                
                Natron::Point topLeft,btmRight,topRight;
                topRight.x = patternCorners[3]->getValueAtTime(time,0) + center.x + offset.x;
                topRight.y = patternCorners[3]->getValueAtTime(time,1) + center.y + offset.y;
                btmRight.x = patternCorners[2]->getValueAtTime(time,0) + center.x + offset.x;
                topLeft.y = patternCorners[0]->getValueAtTime(time,1) + center.y + offset.y;
                
                p.x = std::max(p.x, topRight.x);
                p.y = std::max(p.y, topRight.y);
                p.x = std::max(p.x, btmRight.x);
                p.y = std::max(p.y, topLeft.y);
                
                p.x -= (center.x + offset.x);
                p.y -= (center.y + offset.y);
                if (searchWndTopRight->hasAnimation()) {
                    searchWndTopRight->setValuesAtTime(time, p.x , p.y, view,Natron::eValueChangedReasonNatronInternalEdited);
                } else {
                    searchWndTopRight->setValues(p.x, p.y,view,Natron::eValueChangedReasonNatronInternalEdited);
                }
                
                updateSelectedMarkerTexture();
                didSomething = true;
            }   break;
            case eMouseStateDraggingOuterTopLeft:
            {
                
                if (_imp->controlDown == 0) {
                    _imp->transformPattern(time, _imp->eventState, delta);
                    didSomething = true;
                    break;
                }
                Natron::Point center,offset;
                center.x = centerKnob->getValueAtTime(time,0);
                center.y = centerKnob->getValueAtTime(time,1);
                
                offset.x = offsetKnob->getValueAtTime(time, 0);
                offset.y = offsetKnob->getValueAtTime(time, 1);
                
                Natron::Point p;
                p.x = searchWndBtmLeft->getValueAtTime(time, 0) + center.x + offset.x + delta.x;
                p.y = searchWndTopRight->getValueAtTime(time, 1) + center.y + offset.y + delta.y;
                
                Natron::Point btmLeft,topLeft,topRight;
                topLeft.x = patternCorners[0]->getValueAtTime(time,0) + center.x + offset.x;
                topLeft.y = patternCorners[0]->getValueAtTime(time,1) + center.y + offset.y;
                btmLeft.x = patternCorners[1]->getValueAtTime(time,0) + center.x + offset.x;
                topRight.y = patternCorners[3]->getValueAtTime(time,1) + center.y + offset.y;
                p.x = std::min(p.x, topLeft.x);
                p.y = std::max(p.y, topLeft.y);
                p.x = std::min(p.x, btmLeft.x);
                p.y = std::max(p.y, topRight.y);
                
                p.x -= (center.x + offset.x);
                p.y -= (center.y + offset.y);
                if (searchWndBtmLeft->hasAnimation()) {
                    searchWndBtmLeft->setValueAtTime(time, p.x ,view, 0);
                } else {
                    searchWndBtmLeft->setValue(p.x,view,0);
                }
                if (searchWndTopRight->hasAnimation()) {
                    searchWndTopRight->setValueAtTime(time, p.y ,view, 1);
                } else {
                    searchWndTopRight->setValue(p.y,view,1);
                }
                
                updateSelectedMarkerTexture();
                didSomething = true;
            }   break;
            case eMouseStateDraggingInnerBtmMid:
            case eMouseStateDraggingInnerTopMid:
            case eMouseStateDraggingInnerMidLeft:
            case eMouseStateDraggingInnerMidRight:
            case eMouseStateDraggingOuterBtmMid:
            case eMouseStateDraggingOuterTopMid:
            case eMouseStateDraggingOuterMidLeft:
            case eMouseStateDraggingOuterMidRight: {
                _imp->transformPattern(time, _imp->eventState, delta);
                didSomething = true;
            }   break;
            case eMouseStateDraggingSelectedMarkerResizeAnchor:
            {
                QPointF lastPosWidget = viewer->toWidgetCoordinates(_imp->lastMousePos);
                double dx = viewportPos.x() - lastPosWidget.x();
                _imp->selectedMarkerWidth += dx;
                _imp->selectedMarkerWidth = std::max(_imp->selectedMarkerWidth, 10);
                didSomething = true;
            }   break;
            case eMouseStateScalingSelectedMarker:
            {
                boost::shared_ptr<TrackMarker> marker = _imp->selectedMarker.lock();
                assert(marker);
                RectD markerMagRect;
                _imp->computeSelectedMarkerCanonicalRect(&markerMagRect);
                boost::shared_ptr<KnobDouble> centerKnob = marker->getCenterKnob();
                boost::shared_ptr<KnobDouble> offsetKnob = marker->getOffsetKnob();
                boost::shared_ptr<KnobDouble> searchBtmLeft = marker->getSearchWindowBottomLeftKnob();
                boost::shared_ptr<KnobDouble> searchTopRight = marker->getSearchWindowTopRightKnob();
                
                Natron::Point center,offset,btmLeft,topRight;
                center.x = centerKnob->getValueAtTime(time, 0);
                center.y = centerKnob->getValueAtTime(time, 1);
                offset.x = offsetKnob->getValueAtTime(time, 0);
                offset.y = offsetKnob->getValueAtTime(time, 1);
                btmLeft.x = searchBtmLeft->getValueAtTime(time , 0) + center.x + offset.x;
                btmLeft.y = searchBtmLeft->getValueAtTime(time , 1) + center.y + offset.y;
                topRight.x = searchTopRight->getValueAtTime(time , 0) + center.x + offset.x;
                topRight.y = searchTopRight->getValueAtTime(time , 1) + center.y + offset.y;
                Natron::Point centerPoint;
                
                //Remove any offset to the center to see the marker in the magnification window
                double xCenterPercent = (center.x - btmLeft.x + offset.x) / (topRight.x - btmLeft.x);
                double yCenterPercent = (center.y - btmLeft.y + offset.y) / (topRight.y - btmLeft.y);
                centerPoint.y = markerMagRect.y1 + yCenterPercent * (markerMagRect.y2 - markerMagRect.y1);
                centerPoint.x = markerMagRect.x1 + xCenterPercent * (markerMagRect.x2 - markerMagRect.x1);
                
                double prevDist = std::sqrt((_imp->lastMousePos.x() - centerPoint.x ) * ( _imp->lastMousePos.x() - centerPoint.x) + ( _imp->lastMousePos.y() - centerPoint.y) * ( _imp->lastMousePos.y() - centerPoint.y));
                if (prevDist != 0) {
                    double dist = std::sqrt(( pos.x() - centerPoint.x) * ( pos.x() - centerPoint.x) + ( pos.y() - centerPoint.y) * ( pos.y() - centerPoint.y));
                    double ratio = dist / prevDist;
                    _imp->selectedMarkerScale.x *= ratio;
                    _imp->selectedMarkerScale.x = std::max(0.05,std::min(1., _imp->selectedMarkerScale.x));
                    _imp->selectedMarkerScale.y = _imp->selectedMarkerScale.x;
                    didSomething = true;
                }
            }   break;
            case eMouseStateDraggingSelectedMarker:
            {
                double x = centerKnob->getValueAtTime(time,0);
                double y = centerKnob->getValueAtTime(time,1);
                x -= delta.x *  _imp->selectedMarkerScale.x;
                y -= delta.y *  _imp->selectedMarkerScale.y;
                centerKnob->setValuesAtTime(time, x,y,view,Natron::eValueChangedReasonPluginEdited);
            
                if (_imp->createKeyOnMoveButton->isChecked()) {
                    _imp->interactMarker->setUserKeyframe(time);
                }
                updateSelectedMarkerTexture();
                didSomething = true;

            }   break;
            default:
                break;
        }

    }
    if (_imp->clickToAddTrackEnabled) {
        ///Refresh the overlay
        didSomething = true;
    }
    _imp->lastMousePos = pos;

    return didSomething;
} //penMotion

bool
TrackerGui::penUp(double time,
                  const RenderScale & renderScale,
                  ViewIdx view,
                  const QPointF & viewportPos,
                  const QPointF & pos,
                  double pressure,
                  QMouseEvent* /*e*/)
{
    FLAG_DURING_INTERACT
    
    bool didSomething = false;
    
    TrackerMouseStateEnum state = _imp->eventState;
    _imp->eventState = eMouseStateIdle;
    if (_imp->panelv1) {
        const std::list<std::pair<NodeWPtr,bool> > & instances = _imp->panelv1->getInstances();
        
        for (std::list<std::pair<NodeWPtr,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            
            NodePtr instance = it->first.lock();
            if ( it->second && !instance->isNodeDisabled() ) {
                EffectInstPtr effect = instance->getEffectInstance();
                assert(effect);
                effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                didSomething = effect->onOverlayPenUp_public(time, renderScale, view, viewportPos, pos, pressure);
                if (didSomething) {
                    return true;
                }

            }
        }
    } else { // if (_imp->panelv1) {
        _imp->interactMarker.reset();
        (void)state;
    } // if (_imp->panelv1) {

    return didSomething;
}

bool
TrackerGui::keyDown(double time,
                    const RenderScale & renderScale,
                    ViewIdx view,
                    QKeyEvent* e)
{
    
    FLAG_DURING_INTERACT
    
    bool didSomething = false;
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();


    if (e->key() == Qt::Key_Control) {
        ++_imp->controlDown;
    } else if (e->key() == Qt::Key_Shift) {
        ++_imp->shiftDown;
    }

    Key natronKey = QtEnumConvert::fromQtKey( (Qt::Key)e->key() );
    KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers( e->modifiers() );
    
    if (_imp->panelv1) {
        const std::list<std::pair<NodeWPtr,bool> > & instances = _imp->panelv1->getInstances();
        for (std::list<std::pair<NodeWPtr,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            
            NodePtr instance = it->first.lock();
            if ( it->second && !instance->isNodeDisabled() ) {
                EffectInstPtr effect = instance->getEffectInstance();
                assert(effect);
                effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                didSomething = effect->onOverlayKeyDown_public(time, renderScale, view, natronKey, natronMod);
                if (didSomething) {
                    return true;
                }


            }
        }
    }
    
    if ( modCASIsControlAlt(e) && ( (e->key() == Qt::Key_Control) || (e->key() == Qt::Key_Alt) ) ) {
        _imp->clickToAddTrackEnabled = true;
        _imp->addTrackButton->setDown(true);
        _imp->addTrackButton->setChecked(true);
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingSelectAll, modifiers, key) ) {
        if (_imp->panelv1) {
            _imp->panelv1->onSelectAllButtonClicked();
            std::list<Natron::Node*> selectedInstances;
            _imp->panelv1->getSelectedInstances(&selectedInstances);
            didSomething = !selectedInstances.empty();
        } else {
            _imp->panel->getContext()->selectAll(TrackerContext::eTrackSelectionInternal);
            didSomething = false; //viewer is refreshed already
        }
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingDelete, modifiers, key) ) {
        if (_imp->panelv1) {
            _imp->panelv1->onDeleteKeyPressed();
            std::list<Natron::Node*> selectedInstances;
            _imp->panelv1->getSelectedInstances(&selectedInstances);
            didSomething = !selectedInstances.empty();
        } else {
            _imp->panel->onRemoveButtonClicked();
            didSomething = true;

        }
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingBackward, modifiers, key) ) {
        onTrackBwClicked();
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingPrevious, modifiers, key) ) {
        onTrackPrevClicked();
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingNext, modifiers, key) ) {
        onTrackNextClicked();
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingForward, modifiers, key) ) {
        onTrackFwClicked();
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingStop, modifiers, key) ) {
        onStopButtonClicked();
        didSomething = true;
    }

    return didSomething;
} // keyDown

bool
TrackerGui::keyUp(double time,
                  const RenderScale & renderScale,
                  ViewIdx view,
                  QKeyEvent* e)
{
    
    FLAG_DURING_INTERACT
    
    bool didSomething = false;

    if (e->key() == Qt::Key_Control) {
        if (_imp->controlDown > 0) {
            --_imp->controlDown;
        }
    } else if (e->key() == Qt::Key_Shift) {
        if (_imp->shiftDown > 0) {
            --_imp->shiftDown;
        }
        if (_imp->eventState == eMouseStateScalingSelectedMarker) {
            _imp->eventState = eMouseStateIdle;
            didSomething = true;
        }
    }
    
    Key natronKey = QtEnumConvert::fromQtKey( (Qt::Key)e->key() );
    KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers( e->modifiers() );
    
    if (_imp->panelv1) {
        const std::list<std::pair<NodeWPtr,bool> > & instances = _imp->panelv1->getInstances();
        for (std::list<std::pair<NodeWPtr,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            
            NodePtr instance = it->first.lock();
            if ( it->second && !instance->isNodeDisabled() ) {
                EffectInstPtr effect = instance->getEffectInstance();
                assert(effect);
                effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                didSomething = effect->onOverlayKeyUp_public(time, renderScale, view, natronKey, natronMod);
                if (didSomething) {
                    return true;
                }

            }
        }
    }
    if ( _imp->clickToAddTrackEnabled && ( (e->key() == Qt::Key_Control) || (e->key() == Qt::Key_Alt) ) ) {
        _imp->clickToAddTrackEnabled = false;
        _imp->addTrackButton->setDown(false);
        _imp->addTrackButton->setChecked(false);
        didSomething = true;
    }

    return didSomething;
}

bool
TrackerGui::loseFocus(double time,
                      const RenderScale & renderScale,
                      ViewIdx view)
{
    bool didSomething = false;
    
    _imp->controlDown = 0;
    _imp->shiftDown = 0;
    
    if (_imp->panelv1) {
        const std::list<std::pair<NodeWPtr,bool> > & instances = _imp->panelv1->getInstances();
        for (std::list<std::pair<NodeWPtr,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            
            NodePtr instance = it->first.lock();
            if ( it->second && !instance->isNodeDisabled() ) {
                EffectInstPtr effect = instance->getEffectInstance();
                assert(effect);
                effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                didSomething |= effect->onOverlayFocusLost_public(time, renderScale, view);
            }

        }
    }

    return didSomething;
}

void
TrackerGui::updateSelectionFromSelectionRectangle(bool onRelease)
{
    if (!onRelease) {
        return;
    }
    double l,r,b,t;
    _imp->viewer->getViewer()->getSelectionRectangle(l, r, b, t);
    
    if (_imp->panelv1) {
        std::list<Natron::Node*> currentSelection;
        const std::list<std::pair<NodeWPtr,bool> > & instances = _imp->panelv1->getInstances();
        for (std::list<std::pair<NodeWPtr,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            
            NodePtr instance = it->first.lock();
            boost::shared_ptr<KnobI> newInstanceKnob = instance->getKnobByName("center");
            assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
            KnobDouble* dblKnob = dynamic_cast<KnobDouble*>( newInstanceKnob.get() );
            assert(dblKnob);
            double x,y;
            x = dblKnob->getValue(0);
            y = dblKnob->getValue(1);
            if ( (x >= l) && (x <= r) && (y >= b) && (y <= t) ) {
                ///assert that the node is really not part of the selection
                assert( std::find( currentSelection.begin(),currentSelection.end(),instance.get() ) == currentSelection.end() );
                currentSelection.push_back( instance.get() );
            }

        }
        _imp->panelv1->selectNodes( currentSelection, (_imp->controlDown > 0) );
    } else {
        std::vector<boost::shared_ptr<TrackMarker> > allMarkers;
        std::list<boost::shared_ptr<TrackMarker> > selectedMarkers;
        boost::shared_ptr<TrackerContext> context = _imp->panel->getContext();
        context->getAllMarkers(&allMarkers);
        for (std::size_t i = 0; i < allMarkers.size(); ++i) {
            if (!allMarkers[i]->isEnabled()) {
                continue;
            }
            boost::shared_ptr<KnobDouble> center = allMarkers[i]->getCenterKnob();
            double x,y;
            x = center->getValue(0);
            y = center->getValue(1);
            if ( (x >= l) && (x <= r) && (y >= b) && (y <= t) ) {
                selectedMarkers.push_back(allMarkers[i]);
            }
        }
        
        context->beginEditSelection();
        context->clearSelection(TrackerContext::eTrackSelectionInternal);
        context->addTracksToSelection(selectedMarkers,TrackerContext::eTrackSelectionInternal);
        context->endEditSelection(TrackerContext::eTrackSelectionInternal);
    }
}

void
TrackerGui::onSelectionCleared()
{
    if (_imp->panelv1) {
        _imp->panelv1->clearSelection();
    } else {
        _imp->panel->getContext()->clearSelection(TrackerContext::eTrackSelectionViewer);
    }
}

void
TrackerGui::onTrackBwClicked()
{
  
    _imp->trackBwButton->setDown(true);
    _imp->trackBwButton->setChecked(true);
    if (_imp->panelv1) {
        if (_imp->panelv1->isTracking()) {
            _imp->panelv1->stopTracking();
            return;
        }
        if (!_imp->panelv1->trackBackward(_imp->viewer->getInternalNode())) {
            _imp->panelv1->stopTracking();
            _imp->trackBwButton->setDown(false);
            _imp->trackBwButton->setChecked(false);
        }
    } else {
        boost::shared_ptr<TimeLine> timeline = _imp->viewer->getGui()->getApp()->getTimeLine();
        int startFrame = timeline->currentFrame() - 1;
        double first,last;
        _imp->viewer->getGui()->getApp()->getProject()->getFrameRange(&first, &last);
        boost::shared_ptr<TrackerContext> ctx = _imp->panel->getContext();
        if (ctx->isCurrentlyTracking()) {
            ctx->abortTracking();
        } else {
            ctx->trackSelectedMarkers(startFrame, first -1, false,  _imp->viewer->getInternalNode());
        }
    }
}

void
TrackerGui::onTrackPrevClicked()
{
    if (_imp->panelv1) {
        _imp->panelv1->trackPrevious(_imp->viewer->getInternalNode());
    } else {
        boost::shared_ptr<TimeLine> timeline = _imp->viewer->getGui()->getApp()->getTimeLine();
        int startFrame = timeline->currentFrame() - 1;
        boost::shared_ptr<TrackerContext> ctx = _imp->panel->getContext();
        ctx->trackSelectedMarkers(startFrame, startFrame - 1, false,  _imp->viewer->getInternalNode());
    }
}

void
TrackerGui::onStopButtonClicked()
{
    _imp->trackBwButton->setDown(false);
    _imp->trackFwButton->setDown(false);
    if (_imp->panelv1) {
        _imp->panelv1->stopTracking();
    } else {
        _imp->panel->getContext()->abortTracking();
    }
}

void
TrackerGui::onTrackNextClicked()
{
    if (_imp->panelv1) {
        _imp->panelv1->trackNext(_imp->viewer->getInternalNode());
    } else {
        int startFrame = _imp->viewer->getGui()->getApp()->getTimeLine()->currentFrame() + 1;
        boost::shared_ptr<TrackerContext> ctx = _imp->panel->getContext();
        ctx->trackSelectedMarkers(startFrame, startFrame + 1, true,  _imp->viewer->getInternalNode());
    }
}

void
TrackerGui::onTrackFwClicked()
{
    _imp->trackFwButton->setDown(true);
    _imp->trackFwButton->setChecked(true);
    if (_imp->panelv1) {
        if (_imp->panelv1->isTracking()) {
            _imp->panelv1->stopTracking();
            return;
        }

        if (!_imp->panelv1->trackForward(_imp->viewer->getInternalNode())) {
            _imp->panelv1->stopTracking();
            _imp->trackFwButton->setDown(false);
            _imp->trackFwButton->setChecked(false);
        }
    } else {
        boost::shared_ptr<TimeLine> timeline = _imp->viewer->getGui()->getApp()->getTimeLine();
        int startFrame = timeline->currentFrame() + 1;
        double first,last;
        _imp->viewer->getGui()->getApp()->getProject()->getFrameRange(&first, &last);
        boost::shared_ptr<TrackerContext> ctx = _imp->panel->getContext();
        if (ctx->isCurrentlyTracking()) {
            ctx->abortTracking();
        } else {
            ctx->trackSelectedMarkers(startFrame, last + 1, true, _imp->viewer->getInternalNode());
        }
    }
}

void
TrackerGui::onUpdateViewerClicked(bool clicked)
{
    _imp->updateViewerButton->setDown(clicked);
    _imp->updateViewerButton->setChecked(clicked);
    if (_imp->panelv1) {
        _imp->panelv1->setUpdateViewer(clicked);
    } else {
        _imp->panel->getContext()->setUpdateViewer(clicked);
    }
}

void
TrackerGui::onTrackingStarted()
{
    _imp->isTracking = true;
}

void
TrackerGui::onTrackingEnded()
{
    _imp->trackBwButton->setChecked(false);
    _imp->trackFwButton->setChecked(false);
    _imp->trackBwButton->setDown(false);
    _imp->trackFwButton->setDown(false);
    _imp->isTracking = false;
}

void
TrackerGui::onClearAllAnimationClicked()
{
    if (_imp->panelv1) {
        _imp->panelv1->clearAllAnimationForSelection();
    } else {
        #pragma message WARN("Todo")
        /*std::list<boost::shared_ptr<TrackMarker> > markers;
        _imp->panel->getContext()->getSelectedMarkers(&markers);
        for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it != markers.end(); ++it) {
            (*it)->removeAllKeyframes();
        }*/
    }
}

void
TrackerGui::onClearBwAnimationClicked()
{
    if (_imp->panelv1) {
        _imp->panelv1->clearBackwardAnimationForSelection();
    }
#pragma message WARN("Todo")
}

void
TrackerGui::onClearFwAnimationClicked()
{
    if (_imp->panelv1) {
        _imp->panelv1->clearForwardAnimationForSelection();
    }
    #pragma message WARN("Todo")
}

void
TrackerGui::onCreateKeyOnMoveButtonClicked(bool clicked)
{
    _imp->createKeyOnMoveButton->setDown(clicked);
    _imp->createKeyOnMoveButton->setChecked(clicked);
}

void
TrackerGui::onShowCorrelationButtonClicked(bool clicked)
{
    _imp->showCorrelationButton->setDown(clicked);
    _imp->viewer->getViewer()->redraw();
}

void
TrackerGui::onCenterViewerButtonClicked(bool clicked)
{
    _imp->centerViewerButton->setDown(clicked);
    if (_imp->panelv1) {
        _imp->panelv1->setCenterOnTrack(clicked);
    } else {
        _imp->panel->getContext()->setCenterOnTrack(clicked);
    }
}

void
TrackerGui::onSetKeyframeButtonClicked()
{
    int time = _imp->panel->getNode()->getNode()->getApp()->getTimeLine()->currentFrame();
    std::list<boost::shared_ptr<TrackMarker> > markers;
    _imp->panel->getContext()->getSelectedMarkers(&markers);
    for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->setUserKeyframe(time);
    }
}

void
TrackerGui::onRemoveKeyframeButtonClicked()
{
    int time = _imp->panel->getNode()->getNode()->getApp()->getTimeLine()->currentFrame();
    std::list<boost::shared_ptr<TrackMarker> > markers;
    _imp->panel->getContext()->getSelectedMarkers(&markers);
    for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it != markers.end(); ++it) {
        (*it)->removeUserKeyframe(time);
    }
}


void
TrackerGui::onResetOffsetButtonClicked()
{
    std::list<boost::shared_ptr<TrackMarker> > markers;
    _imp->panel->getContext()->getSelectedMarkers(&markers);
    for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it!=markers.end(); ++it) {
        boost::shared_ptr<KnobDouble> offsetKnob = (*it)->getOffsetKnob();
        assert(offsetKnob);
        for (int i = 0; i < offsetKnob->getDimension(); ++i) {
            offsetKnob->resetToDefaultValue(i);
        }
    }
    

}

void
TrackerGui::onResetTrackButtonClicked()
{
    _imp->panel->onResetButtonClicked();
}



void
TrackerGui::onContextSelectionChanged(int reason)
{
    std::list<boost::shared_ptr<TrackMarker> > selection;
    _imp->panel->getContext()->getSelectedMarkers(&selection);
    if (selection.empty() || selection.size() > 1) {
        _imp->showMarkerTexture = false;
    } else {
        
        assert(selection.size() == 1);
        
        const boost::shared_ptr<TrackMarker>& selectionFront = selection.front();
        boost::shared_ptr<TrackMarker> oldMarker = _imp->selectedMarker.lock();
        if (oldMarker != selectionFront) {
   
            _imp->selectedMarker = selectionFront;
            _imp->refreshSelectedMarkerTexture();
            
            //Don't update in this case, the refresh of the texture will do it for us
            return;
        } else {
            if (selectionFront) {
                _imp->showMarkerTexture = true;
            }
        }
    }
    if ((TrackerContext::TrackSelectionReason)reason == TrackerContext::eTrackSelectionViewer) {
        return;
    }

    _imp->viewer->getViewer()->redraw();
    
}

void
TrackerGui::onTimelineTimeChanged(SequenceTime /*time*/, int /*reason*/)
{
    if (_imp->showMarkerTexture) {
        _imp->refreshSelectedMarkerTexture();
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
TrackerGui::onTrackImageRenderingFinished()
{
    
    assert(QThread::currentThread() == qApp->thread());
    QFutureWatcher<std::pair<boost::shared_ptr<Natron::Image>,RectI> >* future = dynamic_cast<QFutureWatcher<std::pair<boost::shared_ptr<Natron::Image>,RectI> >*>(sender());
    assert(future);
    std::pair<boost::shared_ptr<Natron::Image>,RectI> ret = future->result();
    
    
    _imp->viewer->getViewer()->makeOpenGLcontextCurrent();
    _imp->showMarkerTexture = true;
    if (!_imp->selectedMarkerTexture) {
        _imp->selectedMarkerTexture.reset(new Texture(GL_TEXTURE_2D, GL_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE));
    }
    
    _imp->selectedMarkerTextureRoI = ret.second;
    
    _imp->convertImageTosRGBOpenGLTexture(ret.first, _imp->selectedMarkerTexture, ret.second);
   
    
    _imp->viewer->getViewer()->redraw();

}

void
TrackerGui::onKeyFrameImageRenderingFinished()
{
    assert(QThread::currentThread() == qApp->thread());
    TrackWatcher* future = dynamic_cast<TrackWatcher*>(sender());
    assert(future);
    std::pair<boost::shared_ptr<Natron::Image>,RectI> ret = future->result();
    if (!ret.first || ret.second.isNull()) {
        return;
    }
    
    _imp->viewer->getViewer()->makeOpenGLcontextCurrent();
    
    for (TrackKeyframeRequests::iterator it = _imp->trackRequestsMap.begin(); it!=_imp->trackRequestsMap.end(); ++it) {
        if (it->second.get() == future) {
            
            boost::shared_ptr<TrackMarker> track = it->first.track.lock();
            if (!track) {
                return;
            }
            TrackerGuiPrivate::KeyFrameTexIDs& keyTextures = _imp->trackTextures[track];
            boost::shared_ptr<Texture> tex(new Texture(GL_TEXTURE_2D, GL_LINEAR, GL_NEAREST, GL_CLAMP_TO_EDGE));
            keyTextures[it->first.time] = tex;
            _imp->convertImageTosRGBOpenGLTexture(ret.first, tex, ret.second);
            
            _imp->trackRequestsMap.erase(it);
            
            _imp->viewer->getViewer()->redraw();
            return;
        }
    }
    assert(false);
}

void
TrackerGuiPrivate::convertImageTosRGBOpenGLTexture(const boost::shared_ptr<Image>& image,const boost::shared_ptr<Texture>& tex, const RectI& renderWindow)
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
    if (roi.isNull()) {
        return;
    }

    
    std::size_t bytesCount = 4 * sizeof(unsigned char) * roi.area();
    
    TextureRect region;
    region.x1 = roi.x1;
    region.x2 = roi.x2;
    region.y1 = roi.y1;
    region.y2 = roi.y2;
    region.w = roi.width();
    region.h = roi.height();
    
    GLint currentBoundPBO = 0;
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &currentBoundPBO);
    
    glBindBufferARB( GL_PIXEL_UNPACK_BUFFER_ARB, pboID );
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, bytesCount, NULL, GL_DYNAMIC_DRAW_ARB);
    GLvoid *buf = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY_ARB);
    glCheckError();
    assert(buf);
    
    if (!image) {
        int pixelsCount = roi.area();
        unsigned int* dstPixels = (unsigned int*)buf;
        for (int i = 0; i < pixelsCount; ++i, ++dstPixels) {
            *dstPixels = toBGRA(0, 0, 0, 255);
        }
    } else {
        assert(image->getComponents() == Natron::ImageComponents::getRGBComponents());
        Image::ReadAccess acc(image.get());
        
        
        
        const float* srcPixels = (const float*)acc.pixelAt(roi.x1, roi.y1);
        unsigned int* dstPixels = (unsigned int*)buf;
        assert(srcPixels);
        
        int w = roi.width();
        int srcRowElements = bounds.width() * 3;
        
        const Natron::Color::Lut* lut = Natron::Color::LutManager::sRGBLut();
        lut->validate();
        assert(lut);
        
        unsigned char alpha = 255;
        
        for (int y = roi.y1 ; y < roi.y2; ++y, dstPixels += w, srcPixels += srcRowElements) {
            
            int start = (int)(rand() % (roi.x2 - roi.x1));
            
            for (int backward = 0; backward < 2; ++backward) {
                int index = backward ? start - 1 : start;
                assert( backward == 1 || ( index >= 0 && index < (roi.x2 - roi.x1) ) );
                unsigned error_r = 0x80;
                unsigned error_g = 0x80;
                unsigned error_b = 0x80;
                
                while (index < w && index >= 0) {
                    
                    float r = srcPixels[index * 3];
                    float g = srcPixels[index * 3 + 1];
                    float b = srcPixels[index * 3 + 2];
                    
                    error_r = (error_r & 0xff) + lut->toColorSpaceUint8xxFromLinearFloatFast(r);
                    error_g = (error_g & 0xff) + lut->toColorSpaceUint8xxFromLinearFloatFast(g);
                    error_b = (error_b & 0xff) + lut->toColorSpaceUint8xxFromLinearFloatFast(b);
                    assert(error_r < 0x10000 && error_g < 0x10000 && error_b < 0x10000);
                    
                    dstPixels[index] = toBGRA((U8)(error_r >> 8),
                                              (U8)(error_g >> 8),
                                              (U8)(error_b >> 8),
                                              alpha);
                    
                    
                    if (backward) {
                        --index;
                    } else {
                        ++index;
                    }
                    
                }
            }
        }
        
    }
    
    glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB);
    glCheckError();
    tex->fillOrAllocateTexture(region, Texture::eDataTypeByte, RectI(), false);
    
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, currentBoundPBO);
    
    glCheckError();
}

void
TrackerGui::updateSelectedMarkerTexture()
{
    _imp->refreshSelectedMarkerTexture();
}

void
TrackerGui::onTrackerInputChanged(int /*inputNb*/)
{
    _imp->refreshSelectedMarkerTexture();
}

void
TrackerGuiPrivate::refreshSelectedMarkerTexture()
{
    
    assert(QThread::currentThread() == qApp->thread());
    if (isTracking) {
        return;
    }
    boost::shared_ptr<TrackMarker> marker = selectedMarker.lock();
    if (!marker) {
        return;
    }
    
    int time = panel->getNode()->getNode()->getApp()->getTimeLine()->currentFrame();
    
    RectI roi = marker->getMarkerImageRoI(time);
    if (roi.isNull()) {
        return;
    }
    ImagePtr existingMarkerImg = selectedMarkerImg.lock();
    if (existingMarkerImg && existingMarkerImg->getTime() == time && roi == selectedMarkerTextureRoI) {
        return;
    }
    
    selectedMarkerImg.reset();
    
    imageGetterWatcher.reset(new TrackWatcher());
    QObject::connect(imageGetterWatcher.get(), SIGNAL(finished()), _publicInterface, SLOT(onTrackImageRenderingFinished()));
    imageGetterWatcher->setFuture(QtConcurrent::run(marker.get(),&TrackMarker::getMarkerImage, time, roi));
    
}

void
TrackerGuiPrivate::makeMarkerKeyTexture(int time, const boost::shared_ptr<TrackMarker>& track)
{
    assert(QThread::currentThread() == qApp->thread());
    TrackRequestKey k;
    k.time = time;
    k.track = track;
    k.roi = track->getMarkerImageRoI(time);
    
    TrackKeysMap::iterator foundTrack = trackTextures.find(track);
    if (foundTrack != trackTextures.end()) {
        KeyFrameTexIDs::iterator foundKey = foundTrack->second.find(k.time);
        if (foundKey != foundTrack->second.end()) {
            const TextureRect& texRect = foundKey->second->getTextureRect();
            if (k.roi.x1 == texRect.x1 &&
                k.roi.x2 == texRect.x2 &&
                k.roi.y1 == texRect.y1 &&
                k.roi.y2 == texRect.y2) {
                return;
            }
        }
    }
    
    if (!k.roi.isNull()) {
        TrackWatcherPtr watcher(new TrackWatcher());
        QObject::connect(watcher.get(), SIGNAL(finished()), _publicInterface, SLOT(onKeyFrameImageRenderingFinished()));
        trackRequestsMap[k] = watcher;
        watcher->setFuture(QtConcurrent::run(track.get(),&TrackMarker::getMarkerImage, time, k.roi));
    }
}


void
TrackerGui::onKeyframeSetOnTrack(const boost::shared_ptr<TrackMarker>& marker, int key)
{
    _imp->makeMarkerKeyTexture(key,marker);
}

void
TrackerGui::onKeyframeRemovedOnTrack(const boost::shared_ptr<TrackMarker>& marker, int key)
{
    for (TrackerGuiPrivate::TrackKeysMap::iterator it = _imp->trackTextures.begin(); it!=_imp->trackTextures.end(); ++it) {
        if (it->first.lock() == marker) {
            std::map<int,boost::shared_ptr<Texture> >::iterator found = it->second.find(key);
            if (found != it->second.end()) {
                it->second.erase(found);
            }
            break;
        }
    }
    _imp->viewer->getViewer()->redraw();
}

void
TrackerGui::onAllKeyframesRemovedOnTrack(const boost::shared_ptr<TrackMarker>& marker)
{
    for (TrackerGuiPrivate::TrackKeysMap::iterator it = _imp->trackTextures.begin(); it!=_imp->trackTextures.end(); ++it) {
        if (it->first.lock() == marker) {
            it->second.clear();
            break;
        }
    }
    _imp->viewer->getViewer()->redraw();
}
NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_TrackerGui.cpp"
