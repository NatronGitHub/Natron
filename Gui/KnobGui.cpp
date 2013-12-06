//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

// TODO: split into KnobGui.cpp, KnobGuiFile.cpp and KnobGuiTypes.cpp

#include "Gui/KnobUndoCommand.h"
#include "Gui/KnobGui.h"

#include <cassert>
#include <climits>
#include <cfloat>
#include <stdexcept>

#include <QtCore/QString>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QTextEdit>
#include <QtGui/QStyle>

#if QT_VERSION < 0x050000
#include "Global/Macros.h"
CLANG_DIAG_OFF(unused-private-field);
#include <QtGui/qmime.h>
CLANG_DIAG_ON(unused-private-field);
#endif
#include <QKeyEvent>
#include <QColorDialog>
#include <QGroupBox>
#include <QtGui/QVector4D>
#include <QStyleFactory>
#include <QMenu>

#include "Global/AppManager.h"
#include "Global/LibraryBinary.h"
#include "Global/GlobalDefines.h"

#include "Engine/Node.h"
#include "Engine/VideoEngine.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Settings.h"
#include "Engine/Knob.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/TimeLine.h"

#include "Gui/Button.h"
#include "Gui/DockablePanel.h"
#include "Gui/ViewerTab.h"
#include "Gui/TimeLineGui.h"
#include "Gui/Gui.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/TabWidget.h"
#include "Gui/NodeGui.h"
#include "Gui/SpinBox.h"
#include "Gui/ComboBox.h"
#include "Gui/LineEdit.h"
#include "Gui/CurveEditor.h"
#include "Gui/ScaleSlider.h"
#include "Gui/KnobGuiTypes.h"

#include "Readers/Reader.h"

using namespace Natron;

/////////////// KnobGui

KnobGui::KnobGui(Knob* knob,DockablePanel* container)
: _knob(knob)
, _triggerNewLine(true)
, _spacingBetweenItems(0)
, _widgetCreated(false)
, _container(container)
, _animationMenu(NULL)
, _setKeyAction(NULL)
, _animationButton(NULL)
{
    QObject::connect(knob,SIGNAL(valueChanged(int)),this,SLOT(onInternalValueChanged(int)));
    QObject::connect(this,SIGNAL(valueChanged(int,const Variant&)),knob,SLOT(onValueChanged(int,const Variant&)));
    QObject::connect(knob,SIGNAL(secretChanged()),this,SLOT(setSecret()));
    QObject::connect(knob,SIGNAL(enabledChanged()),this,SLOT(setEnabled()));
    QObject::connect(knob,SIGNAL(deleted()),this,SLOT(deleteKnob()));
}

KnobGui::~KnobGui(){
    
    emit deleted(this);
    delete _animationButton;
    delete _animationMenu;
}

void KnobGui::pushUndoCommand(QUndoCommand* cmd){
    if(_knob->canBeUndone() && !_knob->isInsignificant()){
        _container->pushUndoCommand(cmd);
    }else{
        cmd->redo();
    }
}



void KnobGui::moveToLayout(QVBoxLayout* layout){
    QWidget* container = new QWidget(layout->parentWidget());
    QHBoxLayout* containerLayout = new QHBoxLayout(container);
    container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    addToLayout(containerLayout);
    layout->addWidget(container);
}

void KnobGui::createGUI(QGridLayout* layout,int row){
    createWidget(layout, row);
    if(_knob->isAnimationEnabled() && !_knob->isSecret()){
        createAnimationButton(layout,row);
    }
    _widgetCreated = true;
    const std::map<int,Variant>& values = _knob->getValueForEachDimension();
    for(std::map<int,Variant>::const_iterator it = values.begin();it!=values.end();++it){
        updateGUI(it->first,it->second);
    }
    setEnabled();
    setSecret();
}

void KnobGui::createAnimationButton(QGridLayout* layout,int row){
    createAnimationMenu(layout->parentWidget());
    _animationButton = new Button("A",layout->parentWidget());
    _animationButton->setToolTip("Animation menu");
    QObject::connect(_animationButton,SIGNAL(clicked()),this,SLOT(showAnimationMenu()));
    layout->addWidget(_animationButton, row, 3,Qt::AlignLeft);
}

void KnobGui::createAnimationMenu(QWidget* parent){
    _animationMenu = new QMenu(parent);
    _setKeyAction = new QAction(tr("Set Key"),_animationMenu);
    QObject::connect(_setKeyAction,SIGNAL(triggered()),this,SLOT(onSetKeyActionTriggered()));
    _animationMenu->addAction(_setKeyAction);
    
    QAction* showInCurveEditorAction = new QAction(tr("Show in curve editor"),_animationMenu);
    QObject::connect(showInCurveEditorAction,SIGNAL(triggered()),this,SLOT(onShowInCurveEditorActionTriggered()));
    _animationMenu->addAction(showInCurveEditorAction);
    
}

void KnobGui::setSetKeyActionEnabled(bool e){
    if(_setKeyAction){
        _setKeyAction->setEnabled(e);
    }
}

void KnobGui::setSecret() {
    // If the Knob is within a group, only show it if the group is unfolded!
    // To test it:
    // try TuttlePinning: fold all groups, then switch from perspective to affine to perspective.
    //  VISIBILITY is different from SECRETNESS. The code considers that both things are equivalent, which is wrong.
    // Of course, this check has to be *recursive* (in case the group is within a folded group)
    bool showit = !_knob->isSecret();
    Knob* parentKnob = _knob->getParentKnob();
    while (showit && parentKnob && parentKnob->typeName() == "Group") {
        Group_KnobGui* parentGui = dynamic_cast<Group_KnobGui*>(_container->findKnobGuiOrCreate(parentKnob));
        assert(parentGui);
        // check for secretness and visibility of the group
        if (parentKnob->isSecret() || !parentGui->isChecked()) {
            showit = false; // one of the including groups is folder, so this item is hidden
        }
        // prepare for next loop iteration
        parentKnob = parentKnob->getParentKnob();
    }
    if (showit) {
        show(); //
    } else {
        hide();
    }
}

void KnobGui::showAnimationMenu(){
    _animationMenu->exec(_animationButton->mapToGlobal(QPoint(0,0)));
}

void KnobGui::onShowInCurveEditorActionTriggered(){
    _knob->getHolder()->getApp()->getGui()->setCurveEditorOnTop();
    std::vector<boost::shared_ptr<Curve> > curves;
    for(int i = 0; i < _knob->getDimension();++i){
        boost::shared_ptr<Curve> c = _knob->getCurve(i);
        if(c->isAnimated()){
            curves.push_back(c);
        }
    }
    if(!curves.empty()){
        _knob->getHolder()->getApp()->getGui()->_curveEditor->centerOn(curves);
    }
    
}

void KnobGui::setKeyframe(SequenceTime time,int dimension){
     _knob->getHolder()->getApp()->getGui()->_curveEditor->addKeyFrame(this,time,dimension);
}

void KnobGui::onSetKeyActionTriggered(){
    
    //get the current time on the global timeline
    SequenceTime time = _knob->getHolder()->getApp()->getTimeLine()->currentFrame();
    for(int i = 0; i < _knob->getDimension();++i){
        setKeyframe(time,i);
    }
    
}

void KnobGui::hide(){
    _hide();
    if(_animationButton)
        _animationButton->hide();
}

void KnobGui::show(){
    _show();
    if(_animationButton)
        _animationButton->show();
}

void KnobGui::onInternalValueChanged(int dimension){
    if(_widgetCreated)
        updateGUI(dimension,_knob->getValue(dimension));
}

