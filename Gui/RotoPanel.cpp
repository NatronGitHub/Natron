//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "RotoPanel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QPixmap>

#include "Gui/Button.h"
#include "Gui/SpinBox.h"
#include "Gui/ClickableLabel.h"
#include "Gui/NodeGui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"

#include "Engine/RotoContext.h"
#include "Engine/TimeLine.h"
#include "Engine/Node.h"
#include "Engine/EffectInstance.h"

struct RotoPanelPrivate
{
    NodeGui* node;
    RotoContext* context;
    
    QVBoxLayout* mainLayout;
    
    QWidget* splineContainer;
    QHBoxLayout* splineLayout;
    ClickableLabel* splineLabel;
    SpinBox* currentKeyframe;
    ClickableLabel* ofLabel;
    SpinBox* totalKeyframes;
    
    Button* prevKeyframe;
    Button* nextKeyframe;
    Button* addKeyframe;
    Button* removeKeyframe;
    
    std::list< boost::shared_ptr<Bezier> > selectedCurves;
    
    RotoPanelPrivate(NodeGui*  n)
    : node(n)
    , context(n->getNode()->getRotoContext().get())
    {
        assert(n && context);
        
    }
    
    void updateSplinesInfosGUI(int time);
};

RotoPanel::RotoPanel(NodeGui* n,QWidget* parent)
: QWidget(parent)
, _imp(new RotoPanelPrivate(n))
{
    
    QObject::connect(_imp->context, SIGNAL(selectionChanged()), this, SLOT(onSelectionChanged()));
    QObject::connect(n->getNode()->getApp()->getTimeLine().get(), SIGNAL(frameChanged(SequenceTime,int)), this,
                     SLOT(onTimeChanged(SequenceTime, int)));
    
    _imp->mainLayout = new QVBoxLayout(this);
    
    _imp->splineContainer = new QWidget(this);
    _imp->mainLayout->addWidget(_imp->splineContainer);
    
    _imp->splineLayout = new QHBoxLayout(_imp->splineContainer);
    _imp->splineLayout->setContentsMargins(0, 0, 0, 0);
    _imp->splineLayout->setSpacing(0);
    
    _imp->splineLabel = new ClickableLabel("Spline keyframe:",_imp->splineContainer);
    _imp->splineLabel->setEnabled(false);
    _imp->splineLayout->addWidget(_imp->splineLabel);
    
    _imp->currentKeyframe = new SpinBox(_imp->splineContainer,SpinBox::DOUBLE_SPINBOX);
    _imp->currentKeyframe->setEnabled(false);
    _imp->currentKeyframe->setToolTip("The current keyframe for the selected shape(s)");
    _imp->splineLayout->addWidget(_imp->currentKeyframe);
    
    _imp->ofLabel = new ClickableLabel("of",_imp->splineContainer);
    _imp->ofLabel->setEnabled(false);
    _imp->splineLayout->addWidget(_imp->ofLabel);
    
    _imp->totalKeyframes = new SpinBox(_imp->splineContainer,SpinBox::INT_SPINBOX);
    _imp->totalKeyframes->setEnabled(false);
    _imp->totalKeyframes->setToolTip("The keyframe count for all the shapes.");
    _imp->splineLayout->addWidget(_imp->totalKeyframes);
    
    QPixmap prevPix,nextPix,addPix,removePix;
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_PREVIOUS_KEY, &prevPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_NEXT_KEY, &nextPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_ADD_KEYFRAME, &addPix);
    appPTR->getIcon(Natron::NATRON_PIXMAP_REMOVE_KEYFRAME, &removePix);
    
    _imp->prevKeyframe = new Button(QIcon(prevPix),"",_imp->splineContainer);
    _imp->prevKeyframe->setToolTip("Go to the previous keyframe");
    _imp->prevKeyframe->setEnabled(false);
    QObject::connect(_imp->prevKeyframe, SIGNAL(clicked(bool)), this, SLOT(onGoToPrevKeyframeButtonClicked()));
    _imp->splineLayout->addWidget(_imp->prevKeyframe);
    
    _imp->nextKeyframe = new Button(QIcon(nextPix),"",_imp->splineContainer);
    _imp->nextKeyframe->setToolTip("Go to the next keyframe");
    _imp->nextKeyframe->setEnabled(false);
    QObject::connect(_imp->nextKeyframe, SIGNAL(clicked(bool)), this, SLOT(onGoToNextKeyframeButtonClicked()));
    _imp->splineLayout->addWidget(_imp->nextKeyframe);
    
    _imp->addKeyframe = new Button(QIcon(addPix),"",_imp->splineContainer);
    _imp->addKeyframe->setToolTip("Add keyframe at the current timeline's time");
    _imp->addKeyframe->setEnabled(false);
    QObject::connect(_imp->addKeyframe, SIGNAL(clicked(bool)), this, SLOT(onAddKeyframeButtonClicked()));
    _imp->splineLayout->addWidget(_imp->addKeyframe);
    
    _imp->removeKeyframe = new Button(QIcon(removePix),"",_imp->splineContainer);
    _imp->removeKeyframe->setToolTip("Remove keyframe at the current timeline's time");
    _imp->removeKeyframe->setEnabled(false);
    QObject::connect(_imp->removeKeyframe, SIGNAL(clicked(bool)), this, SLOT(onRemoveKeyframeButtonClicked()));
    _imp->splineLayout->addWidget(_imp->removeKeyframe);
    
}

RotoPanel::~RotoPanel()
{
    
}

void RotoPanel::onGoToPrevKeyframeButtonClicked()
{
    _imp->context->goToPreviousKeyframe();
}

void RotoPanel::onGoToNextKeyframeButtonClicked()
{
    _imp->context->goToNextKeyframe();
}

void RotoPanel::onAddKeyframeButtonClicked()
{
    _imp->context->setKeyframeOnSelectedCurves();
}

void RotoPanel::onRemoveKeyframeButtonClicked()
{
    _imp->context->removeKeyframeOnSelectedCurves();
}

void RotoPanel::onSelectionChanged()
{
    
    ///disconnect previous selection
    for (std::list< boost::shared_ptr<Bezier> >::const_iterator it = _imp->selectedCurves.begin(); it!=_imp->selectedCurves.end(); ++it) {
        QObject::disconnect((*it).get(), SIGNAL(keyframeSet(int)), this, SLOT(onSelectedBezierKeyframeSet(int)));
        QObject::disconnect((*it).get(), SIGNAL(keyframeRemoved(int)), this, SLOT(onSelectedBezierKeyframeRemoved(int)));
    }
    
    ///connect new selection
    _imp->selectedCurves = _imp->context->getSelectedCurves();
    
    bool enabled = !_imp->selectedCurves.empty();

    _imp->splineLabel->setEnabled(enabled);
    _imp->currentKeyframe->setEnabled(enabled);
    _imp->ofLabel->setEnabled(enabled);
    _imp->totalKeyframes->setEnabled(enabled);
    _imp->prevKeyframe->setEnabled(enabled);
    _imp->nextKeyframe->setEnabled(enabled);
    _imp->addKeyframe->setEnabled(enabled);
    _imp->removeKeyframe->setEnabled(enabled);

    
    for (std::list< boost::shared_ptr<Bezier> >::const_iterator it = _imp->selectedCurves.begin(); it!=_imp->selectedCurves.end(); ++it) {
        QObject::connect((*it).get(), SIGNAL(keyframeSet(int)), this, SLOT(onSelectedBezierKeyframeSet(int)));
        QObject::connect((*it).get(), SIGNAL(keyframeRemoved(int)), this, SLOT(onSelectedBezierKeyframeRemoved(int)));
    }
    
    int time = _imp->context->getTimelineCurrentTime();
    
    ///update the splines info GUI
    _imp->updateSplinesInfosGUI(time);
}

void RotoPanel::onSelectedBezierKeyframeSet(int time)
{
    _imp->updateSplinesInfosGUI(time);
}

void RotoPanel::onSelectedBezierKeyframeRemoved(int time)
{
    _imp->updateSplinesInfosGUI(time);
}

void RotoPanelPrivate::updateSplinesInfosGUI(int time)
{
    std::set<int> keyframes;
    for (std::list< boost::shared_ptr<Bezier> >::const_iterator it = selectedCurves.begin(); it!=selectedCurves.end(); ++it) {
        (*it)->getKeyframeTimes(&keyframes);
    }
    totalKeyframes->setValue((double)keyframes.size());
    
    if (keyframes.empty()) {
        currentKeyframe->setValue(1.);
        currentKeyframe->setAnimation(0);
    } else {
        ///get the first time that is equal or greater to the current time
        std::set<int>::iterator lowerBound = keyframes.lower_bound(time);
        int dist = 0;
        if (lowerBound != keyframes.end()) {
            dist = std::distance(keyframes.begin(), lowerBound);
        }
        
        if (lowerBound == keyframes.end()) {
            ///we're after the last keyframe
            currentKeyframe->setValue((double)keyframes.size());
            currentKeyframe->setAnimation(1);
        } else if (*lowerBound == time) {
            currentKeyframe->setValue(dist + 1);
            currentKeyframe->setAnimation(2);
        } else {
            ///we're in-between 2 keyframes, interpolate
            if (lowerBound == keyframes.begin()) {
                currentKeyframe->setValue(1.);
            } else {
                std::set<int>::iterator prev = lowerBound;
                --prev;
                currentKeyframe->setValue((double)(time - *prev) / (double)(*lowerBound - *prev) + dist);
            }
            
            currentKeyframe->setAnimation(1);
        }
    }
}

void RotoPanel::onTimeChanged(SequenceTime time,int /*reason*/)
{
    _imp->updateSplinesInfosGUI(time);
}
