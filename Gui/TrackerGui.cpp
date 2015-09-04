/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QHBoxLayout>
#include <QTextEdit>
#include <QKeyEvent>
#include <QPixmap>
#include <QIcon>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"

#include "Gui/GuiApplicationManager.h"
#include "Gui/Button.h"
#include "Gui/QtEnumConvert.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"
#include "Gui/GuiMacros.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/Utils.h"

#define POINT_SIZE 5
#define CROSS_SIZE 6
#define ADDTRACK_SIZE 5

using namespace Natron;

struct TrackerGuiPrivate
{
    boost::shared_ptr<TrackerPanel> panel;
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
    bool clickToAddTrackEnabled;
    QPointF lastMousePos;
    QRectF selectionRectangle;
    int controlDown;

    TrackerGuiPrivate(const boost::shared_ptr<TrackerPanel> & panel,
                      ViewerTab* parent)
        : panel(panel)
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
          , clickToAddTrackEnabled(false)
          , lastMousePos()
          , selectionRectangle()
          , controlDown(0)
    {
    }
};

TrackerGui::TrackerGui(const boost::shared_ptr<TrackerPanel> & panel,
                       ViewerTab* parent)
    : QObject()
      , _imp( new TrackerGuiPrivate(panel,parent) )
{
    assert(parent);

    QObject::connect( parent->getViewer(),SIGNAL( selectionRectangleChanged(bool) ),this,SLOT( updateSelectionFromSelectionRectangle(bool) ) );
    QObject::connect( parent->getViewer(), SIGNAL( selectionCleared() ), this, SLOT( onSelectionCleared() ) );

    QObject::connect( panel.get(), SIGNAL(trackingEnded()), this, SLOT(onTrackingEnded()));
    _imp->buttonsBar = new QWidget(parent);
    _imp->buttonsLayout = new QHBoxLayout(_imp->buttonsBar);
    _imp->buttonsLayout->setContentsMargins(3, 2, 0, 0);

    QPixmap pixAdd;
    appPTR->getIcon(Natron::NATRON_PIXMAP_ADD_TRACK,&pixAdd);
    
    _imp->addTrackButton = new Button(QIcon(pixAdd),"",_imp->buttonsBar);
    _imp->addTrackButton->setCheckable(true);
    _imp->addTrackButton->setChecked(false);
    _imp->addTrackButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->addTrackButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->addTrackButton->setToolTip(Natron::convertFromPlainText(tr("When enabled you can add new tracks "
                                                                 "by clicking on the Viewer. "
                                                                 "Holding the Control + Alt keys is the "
                                                                 "same as pressing this button."),
                                                              Qt::WhiteSpaceNormal) );
    _imp->buttonsLayout->addWidget(_imp->addTrackButton);
    QObject::connect( _imp->addTrackButton, SIGNAL( clicked(bool) ), this, SLOT( onAddTrackClicked(bool) ) );
    QPixmap pixPrev,pixNext,pixClearAll,pixClearBw,pixClearFw,pixUpdateViewerEnabled,pixUpdateViewerDisabled,pixStop;
    QPixmap bwEnabled,bwDisabled,fwEnabled,fwDisabled;
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_REWIND_DISABLED, &bwDisabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_REWIND_ENABLED, &bwEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_PREVIOUS, &pixPrev);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_NEXT, &pixNext);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_PLAY_DISABLED, &fwDisabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_PLAY_ENABLED, &fwEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_CLEAR_ALL_ANIMATION, &pixClearAll);
    appPTR->getIcon(Natron::NATRON_PIXMAP_CLEAR_BACKWARD_ANIMATION, &pixClearBw);
    appPTR->getIcon(Natron::NATRON_PIXMAP_CLEAR_FORWARD_ANIMATION, &pixClearFw);
    appPTR->getIcon(Natron::NATRON_PIXMAP_VIEWER_REFRESH_ACTIVE, &pixUpdateViewerEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_VIEWER_REFRESH, &pixUpdateViewerDisabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_STOP, &pixStop);

    QIcon bwIcon;
    bwIcon.addPixmap(bwEnabled,QIcon::Normal,QIcon::On);
    bwIcon.addPixmap(bwDisabled,QIcon::Normal,QIcon::Off);
    _imp->trackBwButton = new Button(bwIcon,"",_imp->buttonsBar);
    _imp->trackBwButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->trackBwButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->trackBwButton->setToolTip("<p>" + tr("Track selected tracks backward until left bound of the timeline.") +
                                    "</p><p><b>" + tr("Keyboard shortcut:") + " Z</b></p>");
    _imp->trackBwButton->setCheckable(true);
    _imp->trackBwButton->setChecked(false);
    QObject::connect( _imp->trackBwButton,SIGNAL( clicked(bool) ),this,SLOT( onTrackBwClicked() ) );
    _imp->buttonsLayout->addWidget(_imp->trackBwButton);

    _imp->trackPrevButton = new Button(QIcon(pixPrev),"",_imp->buttonsBar);
    _imp->trackPrevButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->trackPrevButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->trackPrevButton->setToolTip("<p>" + tr("Track selected tracks on the previous frame.") +
                                      "</p><p><b>" + tr("Keyboard shortcut:") + " X</b></p>");
    QObject::connect( _imp->trackPrevButton,SIGNAL( clicked(bool) ),this,SLOT( onTrackPrevClicked() ) );
    _imp->buttonsLayout->addWidget(_imp->trackPrevButton);

    _imp->stopTrackingButton = new Button(QIcon(pixStop),"",_imp->buttonsBar);
    _imp->stopTrackingButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->stopTrackingButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->stopTrackingButton->setToolTip("<p>" + tr("Stop the ongoing tracking if any")  +
                                         "</p><p><b>" + tr("Keyboard shortcut:") + " Escape</b></p>");
    QObject::connect( _imp->stopTrackingButton,SIGNAL( clicked(bool) ),this,SLOT( onStopButtonClicked() ) );
    _imp->buttonsLayout->addWidget(_imp->stopTrackingButton);

    _imp->trackNextButton = new Button(QIcon(pixNext),"",_imp->buttonsBar);
    _imp->trackNextButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->trackNextButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->trackNextButton->setToolTip("<p>" + tr("Track selected tracks on the next frame.") +
                                      "</p><p><b>" + tr("Keyboard shortcut:") + " C</b></p>");
    QObject::connect( _imp->trackNextButton,SIGNAL( clicked(bool) ),this,SLOT( onTrackNextClicked() ) );
    _imp->buttonsLayout->addWidget(_imp->trackNextButton);

    QIcon fwIcon;
    fwIcon.addPixmap(fwEnabled,QIcon::Normal,QIcon::On);
    fwIcon.addPixmap(fwDisabled,QIcon::Normal,QIcon::Off);
    _imp->trackFwButton = new Button(fwIcon,"",_imp->buttonsBar);
    _imp->trackFwButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->trackFwButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->trackFwButton->setToolTip("<p>" + tr("Track selected tracks forward until right bound of the timeline.") +
                                    "</p><p><b>" + tr("Keyboard shortcut:") + " V</b></p>");
    _imp->trackFwButton->setCheckable(true);
    _imp->trackFwButton->setChecked(false);
    QObject::connect( _imp->trackFwButton,SIGNAL( clicked(bool) ),this,SLOT( onTrackFwClicked() ) );
    _imp->buttonsLayout->addWidget(_imp->trackFwButton);


    _imp->clearAllAnimationButton = new Button(QIcon(pixClearAll),"",_imp->buttonsBar);
    _imp->clearAllAnimationButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->clearAllAnimationButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->clearAllAnimationButton->setToolTip(Natron::convertFromPlainText(tr("Clear all animation for selected tracks."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->clearAllAnimationButton,SIGNAL( clicked(bool) ),this,SLOT( onClearAllAnimationClicked() ) );
    _imp->buttonsLayout->addWidget(_imp->clearAllAnimationButton);

    _imp->clearBwAnimationButton = new Button(QIcon(pixClearBw),"",_imp->buttonsBar);
    _imp->clearBwAnimationButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->clearBwAnimationButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->clearBwAnimationButton->setToolTip(Natron::convertFromPlainText(tr("Clear animation backward from the current frame."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->clearBwAnimationButton,SIGNAL( clicked(bool) ),this,SLOT( onClearBwAnimationClicked() ) );
    _imp->buttonsLayout->addWidget(_imp->clearBwAnimationButton);

    _imp->clearFwAnimationButton = new Button(QIcon(pixClearFw),"",_imp->buttonsBar);
    _imp->clearFwAnimationButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->clearFwAnimationButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->clearFwAnimationButton->setToolTip(Natron::convertFromPlainText(tr("Clear animation forward from the current frame."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->clearFwAnimationButton,SIGNAL( clicked(bool) ),this,SLOT( onClearFwAnimationClicked() ) );
    _imp->buttonsLayout->addWidget(_imp->clearFwAnimationButton);

    QIcon updateViewerIC;
    updateViewerIC.addPixmap(pixUpdateViewerEnabled,QIcon::Normal,QIcon::On);
    updateViewerIC.addPixmap(pixUpdateViewerDisabled,QIcon::Normal,QIcon::Off);
    _imp->updateViewerButton = new Button(updateViewerIC,"",_imp->buttonsBar);
    _imp->updateViewerButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->updateViewerButton->setIconSize(QSize(NATRON_MEDIUM_BUTTON_ICON_SIZE, NATRON_MEDIUM_BUTTON_ICON_SIZE));
    _imp->updateViewerButton->setCheckable(true);
    _imp->updateViewerButton->setChecked(true);
    _imp->updateViewerButton->setDown(true);
    _imp->updateViewerButton->setToolTip(Natron::convertFromPlainText(tr("Update viewer during tracking for each frame instead of just the tracks."), Qt::WhiteSpaceNormal));
    QObject::connect( _imp->updateViewerButton,SIGNAL( clicked(bool) ),this,SLOT( onUpdateViewerClicked(bool) ) );
    _imp->buttonsLayout->addWidget(_imp->updateViewerButton);


    _imp->buttonsLayout->addStretch();
}

TrackerGui::~TrackerGui()
{
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

void
TrackerGui::drawOverlays(double time,
                         double scaleX,
                         double scaleY) const
{
    double pixelScaleX, pixelScaleY;

    _imp->viewer->getViewer()->getPixelScale(pixelScaleX, pixelScaleY);

    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_POINT_BIT | GL_ENABLE_BIT | GL_HINT_BIT | GL_TRANSFORM_BIT);

        ///For each instance: <pointer,selected ? >
        const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & instances = _imp->panel->getInstances();
        for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            
            boost::shared_ptr<Natron::Node> instance = it->first.lock();
            
            if (instance->isNodeDisabled()) {
                continue;
            }
            if (it->second) {
                ///The track is selected, use the plug-ins interact
                Natron::EffectInstance* effect = instance->getLiveInstance();
                assert(effect);
                effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                effect->drawOverlay_public(time, scaleX,scaleY);
            } else {
                ///Draw a custom interact, indicating the track isn't selected
                boost::shared_ptr<KnobI> newInstanceKnob = instance->getKnobByName("center");
                assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
                Double_Knob* dblKnob = dynamic_cast<Double_Knob*>( newInstanceKnob.get() );
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

        if (_imp->clickToAddTrackEnabled) {
            ///draw a square of 20px around the mouse cursor
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_LINE_SMOOTH);
            glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
            glLineWidth(1.5);
            //GLProtectMatrix p(GL_PROJECTION); // useless (we do two glTranslate in opposite directions)
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
                glVertex2d(_imp->lastMousePos.x() - ADDTRACK_SIZE * 2 * pixelScaleX, _imp->lastMousePos.y() - ADDTRACK_SIZE * 2 * pixelScaleY);
                glVertex2d(_imp->lastMousePos.x() - ADDTRACK_SIZE * 2 * pixelScaleX, _imp->lastMousePos.y() + ADDTRACK_SIZE * 2 * pixelScaleY);
                glVertex2d(_imp->lastMousePos.x() + ADDTRACK_SIZE * 2 * pixelScaleX, _imp->lastMousePos.y() + ADDTRACK_SIZE * 2 * pixelScaleY);
                glVertex2d(_imp->lastMousePos.x() + ADDTRACK_SIZE * 2 * pixelScaleX, _imp->lastMousePos.y() - ADDTRACK_SIZE * 2 * pixelScaleY);
                glEnd();

                ///draw a cross at the cursor position
                glBegin(GL_LINES);
                glVertex2d( _imp->lastMousePos.x() - ADDTRACK_SIZE * pixelScaleX, _imp->lastMousePos.y() );
                glVertex2d( _imp->lastMousePos.x() + ADDTRACK_SIZE * pixelScaleX, _imp->lastMousePos.y() );
                glVertex2d(_imp->lastMousePos.x(), _imp->lastMousePos.y() - ADDTRACK_SIZE * pixelScaleY);
                glVertex2d(_imp->lastMousePos.x(), _imp->lastMousePos.y() + ADDTRACK_SIZE * pixelScaleY);
                glEnd();
            }
        }
    } // GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_ENABLE_BIT | GL_HINT_BIT);
} // drawOverlays

bool
TrackerGui::penDown(double time,
                    double scaleX,
                    double scaleY,
                    const QPointF & viewportPos,
                    const QPointF & pos,
                    double pressure,
                    QMouseEvent* e)
{
    std::pair<double,double> pixelScale;

    _imp->viewer->getViewer()->getPixelScale(pixelScale.first, pixelScale.second);
    bool didSomething = false;
    const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & instances = _imp->panel->getInstances();
    for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        
        boost::shared_ptr<Node> instance = it->first.lock();
        if ( it->second && !instance->isNodeDisabled() ) {
            Natron::EffectInstance* effect = instance->getLiveInstance();
            assert(effect);
            effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
            didSomething = effect->onOverlayPenDown_public(time, scaleX, scaleY, viewportPos, pos, pressure);
        }
    }

    double selectionTol = pixelScale.first * 10.;
    for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        
        boost::shared_ptr<Node> instance = it->first.lock();
        boost::shared_ptr<KnobI> newInstanceKnob = instance->getKnobByName("center");
        assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
        Double_Knob* dblKnob = dynamic_cast<Double_Knob*>( newInstanceKnob.get() );
        assert(dblKnob);
        double x,y;
        x = dblKnob->getValueAtTime(time, 0);
        y = dblKnob->getValueAtTime(time, 1);

        if ( ( pos.x() >= (x - selectionTol) ) && ( pos.x() <= (x + selectionTol) ) &&
             ( pos.y() >= (y - selectionTol) ) && ( pos.y() <= (y + selectionTol) ) ) {
            if (!it->second) {
                _imp->panel->selectNode( instance, modCASIsShift(e) );

            }
            didSomething = true;
        }
    }

    if (_imp->clickToAddTrackEnabled && !didSomething) {
        boost::shared_ptr<Node> newInstance = _imp->panel->createNewInstance(true);
        boost::shared_ptr<KnobI> newInstanceKnob = newInstance->getKnobByName("center");
        assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
        Double_Knob* dblKnob = dynamic_cast<Double_Knob*>( newInstanceKnob.get() );
        assert(dblKnob);
        dblKnob->beginChanges();
        dblKnob->blockValueChanges();
        dblKnob->setValueAtTime(time, pos.x(), 0);
        dblKnob->setValueAtTime(time, pos.y(), 1);
        dblKnob->unblockValueChanges();
        dblKnob->endChanges();
        didSomething = true;
    }

    if ( !didSomething && !modCASIsShift(e) ) {
        _imp->panel->clearSelection();
    }

    _imp->lastMousePos = pos;

    return didSomething;
} // penDown

bool
TrackerGui::penDoubleClicked(double /*time*/,
                             double /*scaleX*/,
                             double /*scaleY*/,
                             const QPointF & /*viewportPos*/,
                             const QPointF & /*pos*/,
                             QMouseEvent* /*e*/)
{
    bool didSomething = false;

    return didSomething;
}

bool
TrackerGui::penMotion(double time,
                      double scaleX,
                      double scaleY,
                      const QPointF & viewportPos,
                      const QPointF & pos,
                      double pressure,
                      QInputEvent* /*e*/)
{
    bool didSomething = false;
    const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & instances = _imp->panel->getInstances();

    for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        
        boost::shared_ptr<Node> instance = it->first.lock();
        if ( it->second && !instance->isNodeDisabled() ) {
            Natron::EffectInstance* effect = instance->getLiveInstance();
            assert(effect);
            effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
            if ( effect->onOverlayPenMotion_public(time, scaleX, scaleY, viewportPos, pos, pressure) ) {
                didSomething = true;
            }
        }
    }

    if (_imp->clickToAddTrackEnabled) {
        ///Refresh the overlay
        didSomething = true;
    }
    _imp->lastMousePos = pos;

    return didSomething;
}

bool
TrackerGui::penUp(double time,
                  double scaleX,
                  double scaleY,
                  const QPointF & viewportPos,
                  const QPointF & pos,
                  double pressure,
                  QMouseEvent* /*e*/)
{
    bool didSomething = false;
    const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & instances = _imp->panel->getInstances();

    for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        
        boost::shared_ptr<Node> instance = it->first.lock();
        if ( it->second && !instance->isNodeDisabled() ) {
            Natron::EffectInstance* effect = instance->getLiveInstance();
            assert(effect);
            effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
            didSomething = effect->onOverlayPenUp_public(time, scaleX, scaleY, viewportPos, pos, pressure);
            if (didSomething) {
                return true;
            }
        }
    }

    return didSomething;
}

bool
TrackerGui::keyDown(double time,
                    double scaleX,
                    double scaleY,
                    QKeyEvent* e)
{
    bool didSomething = false;
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();


    if (e->key() == Qt::Key_Control) {
        ++_imp->controlDown;
    }

    Natron::Key natronKey = QtEnumConvert::fromQtKey( (Qt::Key)e->key() );
    Natron::KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers( e->modifiers() );
    const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & instances = _imp->panel->getInstances();
    for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        
        boost::shared_ptr<Node> instance = it->first.lock();
        if ( it->second && !instance->isNodeDisabled() ) {
            Natron::EffectInstance* effect = instance->getLiveInstance();
            assert(effect);
            effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
            didSomething = effect->onOverlayKeyDown_public(time, scaleX, scaleY, natronKey, natronMod);
            if (didSomething) {
                return true;
            }
        }
    }

    if ( modCASIsControlAlt(e) && ( (e->key() == Qt::Key_Control) || (e->key() == Qt::Key_Alt) ) ) {
        _imp->clickToAddTrackEnabled = true;
        _imp->addTrackButton->setDown(true);
        _imp->addTrackButton->setChecked(true);
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingSelectAll, modifiers, key) ) {
        _imp->panel->onSelectAllButtonClicked();
        std::list<Natron::Node*> selectedInstances;
        _imp->panel->getSelectedInstances(&selectedInstances);
        didSomething = !selectedInstances.empty();
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingDelete, modifiers, key) ) {
        _imp->panel->onDeleteKeyPressed();
        std::list<Natron::Node*> selectedInstances;
        _imp->panel->getSelectedInstances(&selectedInstances);
        didSomething = !selectedInstances.empty();
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingBackward, modifiers, key) ) {
        _imp->trackBwButton->setDown(true);
        _imp->trackBwButton->setChecked(true);
        didSomething = _imp->panel->trackBackward();
        if (!didSomething) {
            _imp->panel->stopTracking();
            _imp->trackBwButton->setDown(false);
            _imp->trackBwButton->setChecked(false);
        }
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingPrevious, modifiers, key) ) {
        didSomething = _imp->panel->trackPrevious();
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingNext, modifiers, key) ) {
        didSomething = _imp->panel->trackNext();
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingForward, modifiers, key) ) {
        _imp->trackFwButton->setDown(true);
        _imp->trackFwButton->setChecked(true);
        didSomething = _imp->panel->trackForward();
        if (!didSomething) {
            _imp->panel->stopTracking();
            _imp->trackFwButton->setDown(false);
            _imp->trackFwButton->setChecked(false);
        }
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingStop, modifiers, key) ) {
        _imp->panel->stopTracking();
    }

    return didSomething;
} // keyDown

bool
TrackerGui::keyUp(double time,
                  double scaleX,
                  double scaleY,
                  QKeyEvent* e)
{
    bool didSomething = false;

    if (e->key() == Qt::Key_Control) {
        if (_imp->controlDown > 0) {
            --_imp->controlDown;
        }
    }

    Natron::Key natronKey = QtEnumConvert::fromQtKey( (Qt::Key)e->key() );
    Natron::KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers( e->modifiers() );
    const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & instances = _imp->panel->getInstances();
    for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        
        boost::shared_ptr<Node> instance = it->first.lock();
        if ( it->second && !instance->isNodeDisabled() ) {
            Natron::EffectInstance* effect = instance->getLiveInstance();
            assert(effect);
            effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
            didSomething = effect->onOverlayKeyUp_public(time, scaleX, scaleY, natronKey, natronMod);
            if (didSomething) {
                return true;
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
                      double scaleX,
                      double scaleY)
{
    bool didSomething = false;

    _imp->controlDown = 0;

    const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & instances = _imp->panel->getInstances();
    for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        
        boost::shared_ptr<Node> instance = it->first.lock();
        if ( it->second && !instance->isNodeDisabled() ) {
            Natron::EffectInstance* effect = instance->getLiveInstance();
            assert(effect);
            effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
            didSomething |= effect->onOverlayFocusLost_public(time, scaleX, scaleY);
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

    std::list<Natron::Node*> currentSelection;
    const std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> > & instances = _imp->panel->getInstances();
    for (std::list<std::pair<boost::weak_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        
        boost::shared_ptr<Node> instance = it->first.lock();
        boost::shared_ptr<KnobI> newInstanceKnob = instance->getKnobByName("center");
        assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
        Double_Knob* dblKnob = dynamic_cast<Double_Knob*>( newInstanceKnob.get() );
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
    _imp->panel->selectNodes( currentSelection, (_imp->controlDown > 0) );
}

void
TrackerGui::onSelectionCleared()
{
    _imp->panel->clearSelection();
}

void
TrackerGui::onTrackBwClicked()
{
    _imp->trackBwButton->setDown(true);
    if (!_imp->panel->trackBackward()) {
        _imp->panel->stopTracking();
        _imp->trackBwButton->setDown(false);
        _imp->trackBwButton->setChecked(false);
    }
}

void
TrackerGui::onTrackPrevClicked()
{
    _imp->panel->trackPrevious();
}

void
TrackerGui::onStopButtonClicked()
{
    _imp->trackBwButton->setDown(false);
    _imp->trackFwButton->setDown(false);
    _imp->panel->stopTracking();
}

void
TrackerGui::onTrackNextClicked()
{
    _imp->panel->trackNext();
}

void
TrackerGui::onTrackFwClicked()
{
    _imp->trackFwButton->setDown(true);
    if (!_imp->panel->trackForward()) {
        _imp->panel->stopTracking();
        _imp->trackFwButton->setDown(false);
        _imp->trackFwButton->setChecked(false);
    }
}

void
TrackerGui::onUpdateViewerClicked(bool clicked)
{
    _imp->panel->setUpdateViewerOnTracking(clicked);
    _imp->updateViewerButton->setDown(clicked);
    _imp->updateViewerButton->setChecked(clicked);
}

void
TrackerGui::onTrackingEnded()
{
    _imp->trackBwButton->setChecked(false);
    _imp->trackFwButton->setChecked(false);
    _imp->trackBwButton->setDown(false);
    _imp->trackFwButton->setDown(false);
}

void
TrackerGui::onClearAllAnimationClicked()
{
    _imp->panel->clearAllAnimationForSelection();
}

void
TrackerGui::onClearBwAnimationClicked()
{
    _imp->panel->clearBackwardAnimationForSelection();
}

void
TrackerGui::onClearFwAnimationClicked()
{
    _imp->panel->clearForwardAnimationForSelection();
}

