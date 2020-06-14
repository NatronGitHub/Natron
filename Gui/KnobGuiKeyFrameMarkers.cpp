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



#include "KnobGuiKeyFrameMarkers.h"

#include <QHBoxLayout>
#include <QColor>

#include "Gui/AnimItemBase.h"
#include "Gui/AnimationModuleEditor.h"
#include "Gui/AnimationModuleUndoRedo.h"
#include "Gui/AnimationModule.h"
#include "Gui/ClickableLabel.h"
#include "Gui/KnobGui.h"
#include "Gui/GuiMacros.h"
#include "Gui/GuiDefines.h"
#include "Gui/KnobAnim.h"
#include "Gui/Gui.h"
#include "Gui/SpinBox.h"
#include "Gui/Button.h"
#include "Gui/GuiApplicationManager.h"


#include "Engine/KnobTypes.h"
#include "Engine/Project.h"
#include "Engine/Color.h"
#include "Engine/KnobUndoCommand.h"
#include "Engine/AppInstance.h"
#include "Engine/Image.h"
#include "Engine/TimeLine.h"
#include "Engine/Utils.h"
#include "Engine/Settings.h"

NATRON_NAMESPACE_ENTER


struct KnobGuiKeyFrameMarkers::Implementation
{

    KnobKeyFrameMarkersWPtr knob;

    SpinBox* currentKeyFrameSpinBox;
    Label* ofTotalKeyFramesLabel;
    SpinBox* numKeyFramesSpinBox;
    Button* goToPrevKeyframeButton;
    Button* goToNextKeyframeButton;
    Button* addKeyFrameButton;
    Button* removeKeyFrameButton;
    Button* clearKeyFramesButton;
    
    Implementation()
    : knob()
    , currentKeyFrameSpinBox(0)
    , ofTotalKeyFramesLabel(0)
    , numKeyFramesSpinBox(0)
    , goToPrevKeyframeButton(0)
    , goToNextKeyframeButton(0)
    , addKeyFrameButton(0)
    , removeKeyFrameButton(0)
    , clearKeyFramesButton(0)
    {

    }
};


KnobGuiKeyFrameMarkers::KnobGuiKeyFrameMarkers(const KnobGuiPtr& knob, ViewIdx view)
: KnobGuiWidgets(knob, view)
, _imp(new Implementation())
{
    _imp->knob = toKnobKeyFrameMarkers(knob->getKnob());
}

void
KnobGuiKeyFrameMarkers::createWidget(QHBoxLayout* layout)
{
    KnobGuiPtr knobUI = getKnobGui();

    _imp->currentKeyFrameSpinBox = new SpinBox(layout->widget(), SpinBox::eSpinBoxTypeDouble);
    _imp->currentKeyFrameSpinBox->setEnabled(false);
    _imp->currentKeyFrameSpinBox->setReadOnly_NoFocusRect(true);
    _imp->currentKeyFrameSpinBox->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Index of the current keyframe"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    layout->addWidget(_imp->currentKeyFrameSpinBox);

    _imp->ofTotalKeyFramesLabel = new ClickableLabel(QString::fromUtf8("of"), layout->widget());
    _imp->ofTotalKeyFramesLabel->setEnabled(false);
    layout->addWidget(_imp->ofTotalKeyFramesLabel);

    _imp->numKeyFramesSpinBox = new SpinBox(layout->widget(), SpinBox::eSpinBoxTypeInt);
    _imp->numKeyFramesSpinBox->setEnabled(false);
    _imp->numKeyFramesSpinBox->setReadOnly_NoFocusRect(true);
    _imp->numKeyFramesSpinBox->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("The total number of keyframes"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    layout->addWidget(_imp->numKeyFramesSpinBox);

    const QSize medButtonSize( TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE) );
    const QSize medButtonIconSize( TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE) );

    int medIconSize = TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE);
    QPixmap prevPix, nextPix, addPix, removePix, clearAnimPix;
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PREVIOUS_KEY, medIconSize, &prevPix);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_NEXT_KEY, medIconSize, &nextPix);
    appPTR->getIcon(NATRON_PIXMAP_ADD_KEYFRAME, medIconSize, &addPix);
    appPTR->getIcon(NATRON_PIXMAP_REMOVE_KEYFRAME, medIconSize, &removePix);
    appPTR->getIcon(NATRON_PIXMAP_CLEAR_ALL_ANIMATION, medIconSize, &clearAnimPix);

    _imp->goToPrevKeyframeButton = new Button(QIcon(prevPix), QString(), layout->widget());
    _imp->goToPrevKeyframeButton->setFixedSize(medButtonSize);
    _imp->goToPrevKeyframeButton->setIconSize(medButtonIconSize);
    _imp->goToPrevKeyframeButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Go to the previous keyframe"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->goToPrevKeyframeButton->setEnabled(false);
    QObject::connect( _imp->goToPrevKeyframeButton, SIGNAL(clicked(bool)), this, SLOT(onGoToPrevKeyframeButtonClicked()) );
    layout->addWidget(_imp->goToPrevKeyframeButton);

    _imp->goToNextKeyframeButton = new Button(QIcon(nextPix), QString(), layout->widget());
    _imp->goToNextKeyframeButton->setFixedSize(medButtonSize);
    _imp->goToNextKeyframeButton->setIconSize(medButtonIconSize);
    _imp->goToNextKeyframeButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Go to the next keyframe"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->goToNextKeyframeButton->setEnabled(false);
    QObject::connect( _imp->goToNextKeyframeButton, SIGNAL(clicked(bool)), this, SLOT(onGoToNextKeyframeButtonClicked()) );
    layout->addWidget(_imp->goToNextKeyframeButton);

    _imp->addKeyFrameButton = new Button(QIcon(addPix), QString(), layout->widget());
    _imp->addKeyFrameButton->setFixedSize(medButtonSize);
    _imp->addKeyFrameButton->setIconSize(medButtonIconSize);
    _imp->addKeyFrameButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Add a keyframe at the current timeline's time"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->addKeyFrameButton->setEnabled(false);
    QObject::connect( _imp->addKeyFrameButton, SIGNAL(clicked(bool)), this, SLOT(onAddKeyframeButtonClicked()) );
    layout->addWidget(_imp->addKeyFrameButton);

    _imp->removeKeyFrameButton = new Button(QIcon(removePix), QString(), layout->widget());
    _imp->removeKeyFrameButton->setFixedSize(medButtonSize);
    _imp->removeKeyFrameButton->setIconSize(medButtonIconSize);
    _imp->removeKeyFrameButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Remove a keyframe at the current timeline's time"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->removeKeyFrameButton->setEnabled(false);
    QObject::connect( _imp->removeKeyFrameButton, SIGNAL(clicked(bool)), this, SLOT(onRemoveKeyframeButtonClicked()) );
    layout->addWidget(_imp->removeKeyFrameButton);

    _imp->clearKeyFramesButton = new Button(QIcon(clearAnimPix), QString(), layout->widget());
    _imp->clearKeyFramesButton->setFixedSize(medButtonSize);
    _imp->clearKeyFramesButton->setIconSize(medButtonIconSize);
    _imp->clearKeyFramesButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Remove all animation"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _imp->clearKeyFramesButton->setEnabled(false);
    QObject::connect( _imp->clearKeyFramesButton, SIGNAL(clicked(bool)), this, SLOT(onRemoveAnimationButtonClicked()) );
    layout->addWidget(_imp->clearKeyFramesButton);
} // createWidget

KnobGuiKeyFrameMarkers::~KnobGuiKeyFrameMarkers()
{
}

void
KnobGuiKeyFrameMarkers::updateGUI()
{
    KnobKeyFrameMarkersPtr knob = _imp->knob.lock();
    if (!knob) {
        return;
    }

    CurvePtr curve = knob->getAnimationCurve(getView(), DimIdx(0));
    KeyFrameSet keys = curve->getKeyFrames_mt_safe();

    _imp->numKeyFramesSpinBox->setValue( (double)keys.size() );

    TimeValue currentTime = knob->getCurrentRenderTime();

    if ( keys.empty() ) {
        _imp->currentKeyFrameSpinBox->setValue(0.);
        _imp->currentKeyFrameSpinBox->setAnimation(0);
    } else {
        // Get the first time that is equal or greater to the current time
        KeyFrame tmp(currentTime, 0);
        KeyFrameSet::iterator lowerBound = keys.lower_bound(tmp);
        int dist = 0;
        if ( lowerBound != keys.end() ) {
            dist = std::distance(keys.begin(), lowerBound);
        }

        if ( lowerBound == keys.end() ) {
            ///we're after the last keyframe
            _imp->currentKeyFrameSpinBox->setValue( (double)keys.size() );
            _imp->currentKeyFrameSpinBox->setAnimation(1);
        } else if (lowerBound->getTime() == currentTime) {
            _imp->currentKeyFrameSpinBox->setValue(dist + 1);
            _imp->currentKeyFrameSpinBox->setAnimation(2);
        } else {
            ///we're in-between 2 keyframes, interpolate
            if ( lowerBound == keys.begin() ) {
                _imp->currentKeyFrameSpinBox->setValue(1.);
            } else {
                KeyFrameSet::iterator prev = lowerBound;
                if ( prev != keys.begin() ) {
                    --prev;
                }
                _imp->currentKeyFrameSpinBox->setValue( (double)(currentTime - prev->getTime()) / (double)(lowerBound->getTime() - prev->getTime()) + dist );
            }

            _imp->currentKeyFrameSpinBox->setAnimation(1);
        }
    }
} // updateGUI

void
KnobGuiKeyFrameMarkers::onGoToPrevKeyframeButtonClicked()
{
    KnobKeyFrameMarkersPtr knob = _imp->knob.lock();
    if (!knob) {
        return;
    }
    CurvePtr curve = knob->getAnimationCurve(getView(), DimIdx(0));
    KeyFrameSet keys = curve->getKeyFrames_mt_safe();

    if (keys.empty()) {
        return;
    }

    TimeValue currentTime = knob->getCurrentRenderTime();

    TimeValue prevKeyTime;
    KeyFrame tmp(currentTime, 0);
    KeyFrameSet::iterator lb = keys.lower_bound(tmp);
    if (lb == keys.end()) {
        // Check if the last keyframe is prior to the current time
        KeyFrameSet::reverse_iterator last = keys.rbegin();
        if (last->getTime() >= currentTime) {
            return;
        } else {
            prevKeyTime = last->getTime();
        }
    } else if (lb == keys.begin()) {
        // No keyframe before
        return;
    } else {
        --lb;
        prevKeyTime = lb->getTime();
    }

    knob->getHolder()->getApp()->getTimeLine()->seekFrame(prevKeyTime, false, EffectInstancePtr(), eTimelineChangeReasonOtherSeek);


}

void
KnobGuiKeyFrameMarkers::onGoToNextKeyframeButtonClicked()
{
    KnobKeyFrameMarkersPtr knob = _imp->knob.lock();
    if (!knob) {
        return;
    }
    CurvePtr curve = knob->getAnimationCurve(getView(), DimIdx(0));
    KeyFrameSet keys = curve->getKeyFrames_mt_safe();

    if (keys.empty()) {
        return;
    }

    TimeValue currentTime = knob->getCurrentRenderTime();

    TimeValue nextKeyTime;
    KeyFrame tmp(currentTime, 0.);
    KeyFrameSet::iterator ub = keys.upper_bound(tmp);
    if (ub == keys.end()) {
        return;
    } else {
        nextKeyTime = ub->getTime();
    }

    knob->getHolder()->getApp()->getTimeLine()->seekFrame(nextKeyTime, false, EffectInstancePtr(), eTimelineChangeReasonOtherSeek);
}

void
KnobGuiKeyFrameMarkers::onAddKeyframeButtonClicked()
{
    KnobKeyFrameMarkersPtr knob = _imp->knob.lock();
    TimeValue time = knob->getCurrentRenderTime();
    KnobGuiPtr knobGui = getKnobGui();

    AnimationModulePtr model = knobGui->getGui()->getAnimationModuleEditor()->getModel();

    KnobAnimPtr knobAnim = model->findKnobAnim(knobGui);
    if (!knobAnim) {
        return;
    }

    AnimItemDimViewIndexID id;
    id.item = knobAnim;
    id.dim = DimIdx(0);
    id.view = getView();

    AnimItemDimViewKeyFramesMap keysMap;
    KeyFrameSet& keys = keysMap[id];

    // Insert a raw keyframe with just time
    KeyFrame k;
    k.setTime(time);
    keys.insert(k);



    knobGui->pushUndoCommand(new AddKeysCommand(keysMap, model, false /*replaceCurve*/));

}

void
KnobGuiKeyFrameMarkers::onRemoveKeyframeButtonClicked()
{


    KnobGuiPtr knobGui = getKnobGui();
    knobGui->removeKeyAtCurrentTime(DimSpec(0), getView());
}

void
KnobGuiKeyFrameMarkers::onRemoveAnimationButtonClicked()
{
    KnobGuiPtr knobGui = getKnobGui();
    knobGui->removeAnimation(DimSpec(0), getView());
}


void
KnobGuiKeyFrameMarkers::reflectAnimationLevel(DimIdx /*dimension*/,
                                   AnimationLevelEnum level)
{
    int value = (int)level;
    if ( value != _imp->currentKeyFrameSpinBox->getAnimation() ) {
        KnobKeyFrameMarkersPtr knob = _imp->knob.lock();
        if (!knob) {
            return;
        }
        bool isEnabled = knob->isEnabled();
        _imp->currentKeyFrameSpinBox->setReadOnly_NoFocusRect(level == eAnimationLevelExpression || !isEnabled);
        _imp->currentKeyFrameSpinBox->setAnimation(value);
    }
}

void
KnobGuiKeyFrameMarkers::onLabelChanged()
{
}

void
KnobGuiKeyFrameMarkers::setWidgetsVisible(bool visible)
{
    _imp->currentKeyFrameSpinBox->setVisible(visible);
    _imp->ofTotalKeyFramesLabel->setVisible(visible);
    _imp->numKeyFramesSpinBox->setVisible(visible);
    _imp->goToPrevKeyframeButton->setVisible(visible);
    _imp->goToNextKeyframeButton->setVisible(visible);
    _imp->addKeyFrameButton->setVisible(visible);
    _imp->removeKeyFrameButton->setVisible(visible);
    _imp->clearKeyFramesButton->setVisible(visible);

}

void
KnobGuiKeyFrameMarkers::setEnabled(const std::vector<bool>& perDimEnabled)
{
    bool enabled = perDimEnabled[0];

    _imp->currentKeyFrameSpinBox->setEnabled(enabled);
    _imp->ofTotalKeyFramesLabel->setEnabled(enabled);
    _imp->numKeyFramesSpinBox->setEnabled(enabled);
    _imp->goToPrevKeyframeButton->setEnabled(enabled);
    _imp->goToNextKeyframeButton->setEnabled(enabled);
    _imp->addKeyFrameButton->setEnabled(enabled);
    _imp->removeKeyFrameButton->setEnabled(enabled);
    _imp->clearKeyFramesButton->setEnabled(enabled);
}

void
KnobGuiKeyFrameMarkers::reflectMultipleSelection(bool dirty)
{
    _imp->currentKeyFrameSpinBox->setIsSelectedMultipleTimes(dirty);
    _imp->numKeyFramesSpinBox->setIsSelectedMultipleTimes(dirty);
}

void
KnobGuiKeyFrameMarkers::reflectSelectionState(bool selected)
{
    _imp->currentKeyFrameSpinBox->setIsSelected(selected);
    _imp->numKeyFramesSpinBox->setIsSelected(selected);
}

void
KnobGuiKeyFrameMarkers::reflectLinkedState(DimIdx /*dimension*/, bool linked)
{
    QColor c;
    if (linked) {
        double r,g,b;
        appPTR->getCurrentSettings()->getExprColor(&r, &g, &b);
        c.setRgbF(Image::clamp(r, 0., 1.),
                  Image::clamp(g, 0., 1.),
                  Image::clamp(b, 0., 1.));
        _imp->currentKeyFrameSpinBox->setAdditionalDecorationTypeEnabled(LineEdit::eAdditionalDecorationColoredFrame, true, c);
        _imp->numKeyFramesSpinBox->setAdditionalDecorationTypeEnabled(LineEdit::eAdditionalDecorationColoredFrame, true, c);
    } else {
        _imp->currentKeyFrameSpinBox->setAdditionalDecorationTypeEnabled(LineEdit::eAdditionalDecorationColoredFrame, false);
        _imp->numKeyFramesSpinBox->setAdditionalDecorationTypeEnabled(LineEdit::eAdditionalDecorationColoredFrame, false);
    }

}

void
KnobGuiKeyFrameMarkers::updateToolTip()
{

}


NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING
#include "moc_KnobGuiKeyFrameMarkers.cpp"
