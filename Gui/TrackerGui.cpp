//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */



#include "TrackerGui.h"

#include <QHBoxLayout>
#include <QTextEdit>
#include <QKeyEvent>

#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"

#include "Gui/Button.h"
#include "Gui/FromQtEnums.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"

using namespace Natron;

struct TrackerGuiPrivate
{
    
    boost::shared_ptr<TrackerPanel> panel;
    ViewerTab* viewer;
    QWidget* buttonsBar;
    QHBoxLayout* buttonsLayout;
    
    Button* addTrackButton;
    
    bool clickToAddTrackEnabled;
    QPointF lastMousePos;
    
    QRectF selectionRectangle;
    
    bool controlDown;
    
    TrackerGuiPrivate(const boost::shared_ptr<TrackerPanel>& panel,ViewerTab* parent)
    : panel(panel)
    , viewer(parent)
    , buttonsBar(NULL)
    , buttonsLayout(NULL)
    , addTrackButton(NULL)
    , clickToAddTrackEnabled(false)
    , lastMousePos()
    , selectionRectangle()
    , controlDown(false)
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
    _imp->addTrackButton->setCheckable(true);
    _imp->addTrackButton->setChecked(false);
    _imp->addTrackButton->setToolTip(tr(Qt::convertFromPlainText("When enabled you can add new tracks by clicking on the Viewer. "
                                                                 "Holding the Control + Alt keys is the same as pressing this button."
                                                                 ,Qt::WhiteSpaceNormal).toStdString().c_str()));
    _imp->buttonsLayout->addWidget(_imp->addTrackButton);
    QObject::connect(_imp->addTrackButton, SIGNAL(clicked(bool)), this, SLOT(onAddTrackClicked(bool)));
    
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
    std::pair<double,double> pixelScale;
    _imp->viewer->getViewer()->getPixelScale(pixelScale.first, pixelScale.second);
    
    ///For each instance: <pointer,selected ? >
    const std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >& instances = _imp->panel->getInstances();
    for (std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it!=instances.end(); ++it) {
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
                    glTranslated(pixelScale.first, - pixelScale.second, 0);
                    glColor4d(0., 0., 0., 1.);

                } else {
                    glColor4f(1., 1., 1., 1.);
                }
                
                double x = dblKnob->getValue(0);
                double y = dblKnob->getValue(1);
                glPointSize(5.);
                glBegin(GL_POINTS);
                glVertex2d(x,y);
                glEnd();
                
                double crossSize = pixelScale.first * 5;
                glBegin(GL_LINES);
                glVertex2d(x - crossSize, y);
                glVertex2d(x + crossSize, y);
                
                glVertex2d(x, y - crossSize);
                glVertex2d(x, y + crossSize);
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
                glTranslated(pixelScale.first, - pixelScale.second, 0);
                glColor4d(0., 0., 0., 0.8);
            } else {
                glColor4d(0., 1., 0.,0.8);
            }
            
            double offset = pixelScale.first * 10;
            glBegin(GL_LINE_STRIP);
            glVertex2d(_imp->lastMousePos.x() - offset, _imp->lastMousePos.y() - offset);
            glVertex2d(_imp->lastMousePos.x() - offset, _imp->lastMousePos.y() + offset);
            glVertex2d(_imp->lastMousePos.x() + offset, _imp->lastMousePos.y() + offset);
            glVertex2d(_imp->lastMousePos.x() + offset, _imp->lastMousePos.y() - offset);
            glVertex2d(_imp->lastMousePos.x() - offset, _imp->lastMousePos.y() - offset);
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
        if (it->second) {
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
        dblKnob->beginValueChange(Natron::PLUGIN_EDITED);
        dblKnob->setValue(pos.x(), 0,false);
        dblKnob->setValue(pos.y(), 1);
        dblKnob->endValueChange();
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
        if (it->second) {
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
        if (it->second) {
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
        _imp->controlDown = true;
    }
    bool controlHeld = e->modifiers().testFlag(Qt::ControlModifier);
    bool shiftHeld = e->modifiers().testFlag(Qt::ShiftModifier);
    bool altHeld = e->modifiers().testFlag(Qt::AltModifier);
    
    Natron::Key natronKey = QtEnumConvert::fromQtKey((Qt::Key)e->key());
    Natron::KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers(e->modifiers());
    const std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >& instances = _imp->panel->getInstances();
    for (std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it!=instances.end(); ++it) {
        if (it->second) {
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
        didSomething = true;
    } else if (!controlHeld && !shiftHeld && !altHeld && (e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete)) {
        _imp->panel->onDeleteKeyPressed();
        didSomething = true;
    }
    
    return didSomething;

}

bool TrackerGui::keyUp(double scaleX,double scaleY,QKeyEvent* e)
{
    bool didSomething = false;
    
    if (e->key() == Qt::Key_Control) {
        _imp->controlDown = false;
    }
    
    Natron::Key natronKey = QtEnumConvert::fromQtKey((Qt::Key)e->key());
    Natron::KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers(e->modifiers());
    const std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >& instances = _imp->panel->getInstances();
    for (std::list<std::pair<boost::shared_ptr<Natron::Node>,bool> >::const_iterator it = instances.begin(); it!=instances.end(); ++it) {
        if (it->second) {
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
    _imp->panel->selectNodes(currentSelection,_imp->controlDown);
}

void TrackerGui::onSelectionCleared()
{
    _imp->panel->clearSelection();
}