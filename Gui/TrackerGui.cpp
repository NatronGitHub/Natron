//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */



#include "TrackerGui.h"

#include <QHBoxLayout>
#include <QTextEdit>
#include <QKeyEvent>
#include <QPixmap>
#include <QIcon>

#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"

#include "Gui/GuiApplicationManager.h"
#include "Gui/Button.h"
#include "Gui/FromQtEnums.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"


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
    
    TrackerGuiPrivate(const boost::shared_ptr<TrackerPanel>& panel,ViewerTab* parent)
    : panel(panel)
    , viewer(parent)
    , buttonsBar(NULL)
    , buttonsLayout(NULL)
    , addTrackButton(NULL)
    , trackBwButton(NULL)
    , trackPrevButton(NULL)
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

TrackerGui::TrackerGui(const boost::shared_ptr<TrackerPanel>& panel,ViewerTab* parent)
: QObject()
, _imp(new TrackerGuiPrivate(panel,parent))
{
    assert(parent);
    
    QObject::connect(parent->getViewer(),SIGNAL(selectionRectangleChanged(bool)),this,SLOT(updateSelectionFromSelectionRectangle(bool)));
    QObject::connect(parent->getViewer(), SIGNAL(selectionCleared()), this, SLOT(onSelectionCleared()));
    
    _imp->buttonsBar = new QWidget(parent);
    _imp->buttonsLayout = new QHBoxLayout(_imp->buttonsBar);
    _imp->buttonsLayout->setContentsMargins(3, 2, 0, 0);
    
    _imp->addTrackButton = new Button("+",_imp->buttonsBar);
    _imp->addTrackButton->setFixedSize(25, 25);
    _imp->addTrackButton->setCheckable(true);
    _imp->addTrackButton->setChecked(false);
    _imp->addTrackButton->setToolTip(tr(Qt::convertFromPlainText("When enabled you can add new tracks by clicking on the Viewer. "
                                                                 "Holding the Control + Alt keys is the same as pressing this button."
                                                                 ,Qt::WhiteSpaceNormal).toStdString().c_str()));
    _imp->buttonsLayout->addWidget(_imp->addTrackButton);
    QObject::connect(_imp->addTrackButton, SIGNAL(clicked(bool)), this, SLOT(onAddTrackClicked(bool)));
    
    QPixmap pixBw,pixPrev,pixNext,pixFw,pixClearAll,pixClearBw,pixClearFw,pixUpdateViewerEnabled,pixUpdateViewerDisabled;
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_REWIND, &pixBw);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_PREVIOUS, &pixPrev);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_NEXT, &pixNext);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_PLAY, &pixFw);
    appPTR->getIcon(Natron::NATRON_PIXMAP_CLEAR_ALL_ANIMATION, &pixClearAll);
    appPTR->getIcon(Natron::NATRON_PIXMAP_CLEAR_BACKWARD_ANIMATION, &pixClearBw);
    appPTR->getIcon(Natron::NATRON_PIXMAP_CLEAR_FORWARD_ANIMATION, &pixClearFw);
    appPTR->getIcon(Natron::NATRON_PIXMAP_UPDATE_VIEWER_ENABLED, &pixUpdateViewerEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_UPDATE_VIEWER_DISABLED, &pixUpdateViewerDisabled);
    
    _imp->trackBwButton = new Button(QIcon(pixBw),"",_imp->buttonsBar);
    _imp->trackBwButton->setToolTip(tr("Track selected tracks backward until left bound of the timeline.") +
                                    "<p><b>" + tr("Keyboard shortcut") + ": Z</b></p>");
    QObject::connect(_imp->trackBwButton,SIGNAL(clicked(bool)),this,SLOT(onTrackBwClicked()));
    _imp->buttonsLayout->addWidget(_imp->trackBwButton);
    
    _imp->trackPrevButton = new Button(QIcon(pixPrev),"",_imp->buttonsBar);
    _imp->trackPrevButton->setToolTip(tr("Track selected tracks on the previous frame.") +
                                      "<p><b>" + tr("Keyboard shortcut") + ": X</b></p>");
    QObject::connect(_imp->trackPrevButton,SIGNAL(clicked(bool)),this,SLOT(onTrackPrevClicked()));
    _imp->buttonsLayout->addWidget(_imp->trackPrevButton);
    
    _imp->trackNextButton = new Button(QIcon(pixNext),"",_imp->buttonsBar);
    _imp->trackNextButton->setToolTip(tr("Track selected tracks on the next frame.") +
                                         "<p><b>" + tr("Keyboard shortcut") + ": C</b></p>");
    QObject::connect(_imp->trackNextButton,SIGNAL(clicked(bool)),this,SLOT(onTrackNextClicked()));
    _imp->buttonsLayout->addWidget(_imp->trackNextButton);

    
    _imp->trackFwButton = new Button(QIcon(pixFw),"",_imp->buttonsBar);
    _imp->trackFwButton->setToolTip(tr("Track selected tracks forward until right bound of the timeline.") +
                                    "<p><b>" + tr("Keyboard shortcut") + ": V</b></p>");
    QObject::connect(_imp->trackFwButton,SIGNAL(clicked(bool)),this,SLOT(onTrackFwClicked()));
    _imp->buttonsLayout->addWidget(_imp->trackFwButton);

    
    _imp->clearAllAnimationButton = new Button(QIcon(pixClearAll),"",_imp->buttonsBar);
    _imp->clearAllAnimationButton->setToolTip(tr(Qt::convertFromPlainText("Clear all animation for selected tracks.",
                                                                Qt::WhiteSpaceNormal).toStdString().c_str()));
    QObject::connect(_imp->clearAllAnimationButton,SIGNAL(clicked(bool)),this,SLOT(onClearAllAnimationClicked()));
    _imp->buttonsLayout->addWidget(_imp->clearAllAnimationButton);
    
    _imp->clearBwAnimationButton = new Button(QIcon(pixClearBw),"",_imp->buttonsBar);
    _imp->clearBwAnimationButton->setToolTip(tr(Qt::convertFromPlainText("Clear animation backward from the current frame.",
                                                                Qt::WhiteSpaceNormal).toStdString().c_str()));
    QObject::connect(_imp->clearBwAnimationButton,SIGNAL(clicked(bool)),this,SLOT(onClearBwAnimationClicked()));
    _imp->buttonsLayout->addWidget(_imp->clearBwAnimationButton);
    
    _imp->clearFwAnimationButton = new Button(QIcon(pixClearFw),"",_imp->buttonsBar);
    _imp->clearFwAnimationButton->setToolTip(tr(Qt::convertFromPlainText("Clear animation forward from the current frame.",
                                                                Qt::WhiteSpaceNormal).toStdString().c_str()));
    QObject::connect(_imp->clearFwAnimationButton,SIGNAL(clicked(bool)),this,SLOT(onClearFwAnimationClicked()));
    _imp->buttonsLayout->addWidget(_imp->clearFwAnimationButton);
    
    QIcon updateViewerIC;
    updateViewerIC.addPixmap(pixUpdateViewerEnabled,QIcon::Normal,QIcon::On);
    updateViewerIC.addPixmap(pixUpdateViewerDisabled,QIcon::Normal,QIcon::Off);
    _imp->updateViewerButton = new Button(updateViewerIC,"",_imp->buttonsBar);
    _imp->updateViewerButton->setCheckable(true);
    _imp->updateViewerButton->setChecked(true);
    _imp->updateViewerButton->setDown(true);
    _imp->updateViewerButton->setToolTip(tr(Qt::convertFromPlainText("Update viewer during tracking for each frame instead of just the tracks."
                                                                     , Qt::WhiteSpaceNormal).toStdString().c_str()));
    QObject::connect(_imp->updateViewerButton,SIGNAL(clicked(bool)),this,SLOT(onUpdateViewerClicked(bool)));
    _imp->buttonsLayout->addWidget(_imp->updateViewerButton);
    
    
    _imp->buttonsLayout->addStretch();
    
}

TrackerGui::~TrackerGui()
{
    
}

QWidget* TrackerGui::getButtonsBar() const
{
    return _imp->buttonsBar;
}
 
void TrackerGui::onAddTrackClicked(bool clicked)
{
    _imp->clickToAddTrackEnabled = !_imp->clickToAddTrackEnabled;
    _imp->addTrackButton->setDown(clicked);
    _imp->addTrackButton->setChecked(clicked);
    _imp->viewer->getViewer()->redraw();
}


void TrackerGui::drawOverlays(double scaleX,double scaleY) const
{
    double pixelScaleX, pixelScaleY;
    _imp->viewer->getViewer()->getPixelScale(pixelScaleX, pixelScaleY);
    
    ///For each instance: <pointer,selected ? >
    const std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >& instances = _imp->panel->getInstances();
    for (std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it!=instances.end(); ++it) {
        if (it->first->isNodeDisabled()) {
            continue;
        }
        if (it->second) {
            ///The track is selected, use the plug-ins interact
            Natron::EffectInstance* effect = it->first->getLiveInstance();
            assert(effect);
            effect->setCurrentViewportForOverlays(_imp->viewer->getViewer());
            effect->drawOverlay_public(scaleX,scaleY);
        } else {
            ///Draw a custom interact, indicating the track isn't selected
            boost::shared_ptr<KnobI> newInstanceKnob = it->first->getKnobByName("center");
            assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
            Double_Knob* dblKnob = dynamic_cast<Double_Knob*>(newInstanceKnob.get());
            assert(dblKnob);
            
            for (int i = 0; i < 2; ++i) {
                if (i == 0) {
                    // Draw a shadow for the cross hair
                    // shift by (1,1) pixel
                    glPushMatrix();
                    glTranslated(pixelScaleX, -pixelScaleY, 0);
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
                glVertex2d(x - CROSS_SIZE*pixelScaleX, y);
                glVertex2d(x + CROSS_SIZE*pixelScaleX, y);
                
                glVertex2d(x, y - CROSS_SIZE*pixelScaleY);
                glVertex2d(x, y + CROSS_SIZE*pixelScaleY);
                glEnd();

                if (i == 0) {
                    glPopMatrix();
                }
                

            }
            glPointSize(1.);
        }
    }
    
    if (_imp->clickToAddTrackEnabled) {
        ///draw a square of 20px around the mouse cursor
        glLineWidth(1.5);
        glEnable(GL_LINE_SMOOTH);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
        for (int i = 0; i < 2; ++i) {
            if (i == 0) {
                // Draw a shadow for the cross hair
                // shift by (1,1) pixel
                glPushMatrix();
                glTranslated(pixelScaleX, -pixelScaleY, 0);
                glColor4d(0., 0., 0., 0.8);
            } else {
                glColor4d(0., 1., 0.,0.8);
            }
            
            glBegin(GL_LINE_LOOP);
            glVertex2d(_imp->lastMousePos.x() - ADDTRACK_SIZE*2*pixelScaleX, _imp->lastMousePos.y() - ADDTRACK_SIZE*2*pixelScaleY);
            glVertex2d(_imp->lastMousePos.x() - ADDTRACK_SIZE*2*pixelScaleX, _imp->lastMousePos.y() + ADDTRACK_SIZE*2*pixelScaleY);
            glVertex2d(_imp->lastMousePos.x() + ADDTRACK_SIZE*2*pixelScaleX, _imp->lastMousePos.y() + ADDTRACK_SIZE*2*pixelScaleY);
            glVertex2d(_imp->lastMousePos.x() + ADDTRACK_SIZE*2*pixelScaleX, _imp->lastMousePos.y() - ADDTRACK_SIZE*2*pixelScaleY);
            glEnd();
            
            ///draw a cross at the cursor position
            glBegin(GL_LINES);
            glVertex2d(_imp->lastMousePos.x() - ADDTRACK_SIZE*pixelScaleX, _imp->lastMousePos.y());
            glVertex2d(_imp->lastMousePos.x() + ADDTRACK_SIZE*pixelScaleX, _imp->lastMousePos.y());
            glVertex2d(_imp->lastMousePos.x(), _imp->lastMousePos.y() - ADDTRACK_SIZE*pixelScaleY);
            glVertex2d(_imp->lastMousePos.x(), _imp->lastMousePos.y() + ADDTRACK_SIZE*pixelScaleY);
            glEnd();

            if (i == 0) {
                glPopMatrix();
            }

        }
        glDisable(GL_LINE_SMOOTH);
        glDisable(GL_BLEND);
        glLineWidth(1.);
    }

}

bool TrackerGui::penDown(double scaleX,double scaleY,const QPointF& viewportPos,const QPointF& pos,QMouseEvent* e)
{
    std::pair<double,double> pixelScale;
    _imp->viewer->getViewer()->getPixelScale(pixelScale.first, pixelScale.second);
    bool didSomething = false;
    
    const std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >& instances = _imp->panel->getInstances();
    for (std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it!=instances.end(); ++it) {
        if (it->second && !it->first->isNodeDisabled()) {
            Natron::EffectInstance* effect = it->first->getLiveInstance();
            assert(effect);
            effect->setCurrentViewportForOverlays(_imp->viewer->getViewer());
            didSomething = effect->onOverlayPenDown_public(scaleX,scaleY,viewportPos, pos);
        }
    }
    
    double selectionTol = pixelScale.first * 10.;
    for (std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it!=instances.end(); ++it) {
        boost::shared_ptr<KnobI> newInstanceKnob = it->first->getKnobByName("center");
        assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
        Double_Knob* dblKnob = dynamic_cast<Double_Knob*>(newInstanceKnob.get());
        assert(dblKnob);
        double x,y;
        x = dblKnob->getValue(0);
        y = dblKnob->getValue(1);
        
        if (pos.x() >= (x - selectionTol) && pos.x() <= (x + selectionTol) &&
            pos.y() >= (y - selectionTol) && pos.y() <= (y + selectionTol)) {
            bool controlHeld = e->modifiers().testFlag(Qt::ControlModifier);
            if (!it->second) {
                _imp->panel->selectNode(it->first, controlHeld);
            }
            didSomething = true;
        }
    }
    
    if (_imp->clickToAddTrackEnabled && !didSomething) {
        boost::shared_ptr<Node> newInstance = _imp->panel->createNewInstance();
        boost::shared_ptr<KnobI> newInstanceKnob = newInstance->getKnobByName("center");
        assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
        Double_Knob* dblKnob = dynamic_cast<Double_Knob*>(newInstanceKnob.get());
        assert(dblKnob);
        dblKnob->blockEvaluation();
        dblKnob->setValue(pos.x(), 0);
        dblKnob->unblockEvaluation();
        dblKnob->setValue(pos.y(), 1);
        didSomething = true;
    }
    
    if (!didSomething && !e->modifiers().testFlag(Qt::ControlModifier)) {
        _imp->panel->clearSelection();
    }
    
    _imp->lastMousePos = pos;

    return didSomething;
}

bool TrackerGui::penDoubleClicked(double /*scaleX*/,double /*scaleY*/,const QPointF& /*viewportPos*/,const QPointF& /*pos*/)
{
    bool didSomething = false;

    return didSomething;
}

bool TrackerGui::penMotion(double scaleX,double scaleY,const QPointF& viewportPos,const QPointF& pos)
{
    bool didSomething = false;
    
    const std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >& instances = _imp->panel->getInstances();
    for (std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it!=instances.end(); ++it) {
        if (it->second && !it->first->isNodeDisabled()) {
            Natron::EffectInstance* effect = it->first->getLiveInstance();
            assert(effect);
            effect->setCurrentViewportForOverlays(_imp->viewer->getViewer());
            if (effect->onOverlayPenMotion_public(scaleX,scaleY,viewportPos, pos)) {
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

bool TrackerGui::penUp(double scaleX,double scaleY,const QPointF& viewportPos,const QPointF& pos)
{
    bool didSomething = false;
    const std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >& instances = _imp->panel->getInstances();
    for (std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it!=instances.end(); ++it) {
        if (it->second && !it->first->isNodeDisabled()) {
            Natron::EffectInstance* effect = it->first->getLiveInstance();
            assert(effect);
            effect->setCurrentViewportForOverlays(_imp->viewer->getViewer());
            didSomething = effect->onOverlayPenUp_public(scaleX,scaleY,viewportPos, pos);
            if (didSomething) {
                return true;
            }
        }
    }
    return didSomething;
}

bool TrackerGui::keyDown(double scaleX,double scaleY,QKeyEvent* e)
{
    bool didSomething = false;
    
    if (e->key() == Qt::Key_Control) {
        ++_imp->controlDown;
    }
    bool controlHeld = e->modifiers().testFlag(Qt::ControlModifier);
    bool shiftHeld = e->modifiers().testFlag(Qt::ShiftModifier);
    bool altHeld = e->modifiers().testFlag(Qt::AltModifier);
    
    Natron::Key natronKey = QtEnumConvert::fromQtKey((Qt::Key)e->key());
    Natron::KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers(e->modifiers());
    const std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >& instances = _imp->panel->getInstances();
    for (std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it!=instances.end(); ++it) {
        if (it->second && !it->first->isNodeDisabled()) {
            Natron::EffectInstance* effect = it->first->getLiveInstance();
            assert(effect);
            effect->setCurrentViewportForOverlays(_imp->viewer->getViewer());
            didSomething = effect->onOverlayKeyDown_public(scaleX, scaleY, natronKey, natronMod);
            if (didSomething) {
                return true;
            }
        }
    }
    
    if (controlHeld && altHeld && (e->key() == Qt::Key_Control || e->key() == Qt::Key_Alt)) {
        _imp->clickToAddTrackEnabled = true;
        _imp->addTrackButton->setDown(true);
        _imp->addTrackButton->setChecked(true);
        didSomething = true;
    } else if (controlHeld && !shiftHeld && !altHeld && e->key() == Qt::Key_A) {
        _imp->panel->onSelectAllButtonClicked();
        std::list<Natron::Node*> selectedInstances;
        _imp->panel->getSelectedInstances(&selectedInstances);
        didSomething = !selectedInstances.empty();
    } else if (!controlHeld && !shiftHeld && !altHeld && (e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete)) {
        _imp->panel->onDeleteKeyPressed();
        std::list<Natron::Node*> selectedInstances;
        _imp->panel->getSelectedInstances(&selectedInstances);
        didSomething = !selectedInstances.empty();
    } else if (!controlHeld && !shiftHeld && !altHeld && (e->key() == Qt::Key_Z)) {
        didSomething = _imp->panel->trackBackward();
    } else if (!controlHeld && !shiftHeld && !altHeld && (e->key() == Qt::Key_X)) {
        didSomething = _imp->panel->trackPrevious();
    } else if (!controlHeld && !shiftHeld && !altHeld && (e->key() == Qt::Key_C)) {
        didSomething = _imp->panel->trackNext();
    } else if (!controlHeld && !shiftHeld && !altHeld && (e->key() == Qt::Key_V)) {
        didSomething = _imp->panel->trackForward();
    }
    
    return didSomething;
}

bool TrackerGui::keyUp(double scaleX, double scaleY, QKeyEvent* e)
{
    bool didSomething = false;
    
    if (e->key() == Qt::Key_Control) {
        if (_imp->controlDown > 0) {
            --_imp->controlDown;
        }
    }
    
    Natron::Key natronKey = QtEnumConvert::fromQtKey((Qt::Key)e->key());
    Natron::KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers(e->modifiers());
    const std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >& instances = _imp->panel->getInstances();
    for (std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it!=instances.end(); ++it) {
        if (it->second && !it->first->isNodeDisabled()) {
            Natron::EffectInstance* effect = it->first->getLiveInstance();
            assert(effect);
            effect->setCurrentViewportForOverlays(_imp->viewer->getViewer());
            didSomething = effect->onOverlayKeyUp_public(scaleX, scaleY, natronKey, natronMod);
            if (didSomething) {
                return true;
            }
        }
    }
    
    if (_imp->clickToAddTrackEnabled && (e->key() == Qt::Key_Control || e->key() == Qt::Key_Alt)) {
        _imp->clickToAddTrackEnabled = false;
        _imp->addTrackButton->setDown(false);
        _imp->addTrackButton->setChecked(false);
        didSomething = true;
    }

    return didSomething;
}

bool TrackerGui::loseFocus(double scaleX, double scaleY)
{
    bool didSomething = false;

    _imp->controlDown = 0;

    const std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >& instances = _imp->panel->getInstances();
    for (std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it!=instances.end(); ++it) {
        if (it->second && !it->first->isNodeDisabled()) {
            Natron::EffectInstance* effect = it->first->getLiveInstance();
            assert(effect);
            effect->setCurrentViewportForOverlays(_imp->viewer->getViewer());
            didSomething |= effect->onOverlayFocusLost_public(scaleX, scaleY);
        }
    }
    
    return didSomething;
}

void TrackerGui::updateSelectionFromSelectionRectangle(bool onRelease)
{
    if (!onRelease) {
        return;
    }
    double l,r,b,t;
    _imp->viewer->getViewer()->getSelectionRectangle(l, r, b, t);

    std::list<Natron::Node*> currentSelection;
    
    const std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >& instances = _imp->panel->getInstances();
    for (std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it!=instances.end(); ++it) {
        boost::shared_ptr<KnobI> newInstanceKnob = it->first->getKnobByName("center");
        assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
        Double_Knob* dblKnob = dynamic_cast<Double_Knob*>(newInstanceKnob.get());
        assert(dblKnob);
        double x,y;
        x = dblKnob->getValue(0);
        y = dblKnob->getValue(1);
        if (x >= l && x <= r && y >= b && y <= t) {
            ///assert that the node is really not part of the selection
            assert(std::find(currentSelection.begin(),currentSelection.end(),it->first.get()) == currentSelection.end());
            currentSelection.push_back(it->first.get());
        }
        
    }
    _imp->panel->selectNodes(currentSelection, (_imp->controlDown > 0));
}

void TrackerGui::onSelectionCleared()
{
    _imp->panel->clearSelection();
}

void TrackerGui::onTrackBwClicked()
{
    _imp->panel->trackBackward();
}

void TrackerGui::onTrackPrevClicked()
{
    _imp->panel->trackPrevious();
}

void TrackerGui::onTrackNextClicked()
{
    _imp->panel->trackNext();
}

void TrackerGui::onTrackFwClicked()
{
    _imp->panel->trackForward();
}

void TrackerGui::onUpdateViewerClicked(bool clicked)
{
    _imp->panel->setUpdateViewerOnTracking(clicked);
    _imp->updateViewerButton->setDown(clicked);
    _imp->updateViewerButton->setChecked(clicked);
}

void TrackerGui::onClearAllAnimationClicked()
{
    _imp->panel->clearAllAnimationForSelection();
}

void TrackerGui::onClearBwAnimationClicked()
{
    _imp->panel->clearBackwardAnimationForSelection();
}

void TrackerGui::onClearFwAnimationClicked()
{
    _imp->panel->clearForwardAnimationForSelection();
}
