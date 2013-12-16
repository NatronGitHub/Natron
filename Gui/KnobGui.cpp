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

#include <QKeyEvent>
#include <QColorDialog>
#include <QGroupBox>
#include <QtGui/QVector4D>
#include <QStyleFactory>
#include <QLabel>
#include <QMenu>
#include <QComboBox>

#include "Global/AppManager.h"
#include "Global/LibraryBinary.h"
#include "Global/GlobalDefines.h"

#include "Engine/Node.h"
#include "Engine/VideoEngine.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Settings.h"
#include "Engine/Knob.h"
#include "Engine/KnobFile.h"
#include "Engine/Curve.h"
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
#include "Gui/CurveWidget.h"

#include "Readers/Reader.h"

using namespace Natron;

/////////////// KnobGui

KnobGui::KnobGui(boost::shared_ptr<Knob> knob,DockablePanel* container)
: _knob(knob)
, _triggerNewLine(true)
, _spacingBetweenItems(0)
, _widgetCreated(false)
, _container(container)
, _animationMenu(NULL)
, _animationButton(NULL)
{
    QObject::connect(knob.get(),SIGNAL(valueChanged(int)),this,SLOT(onInternalValueChanged(int)));
    QObject::connect(knob.get(),SIGNAL(keyFrameSet(SequenceTime,int)),this,SLOT(onInternalKeySet(SequenceTime,int)));
    QObject::connect(this,SIGNAL(keyFrameSetByUser(SequenceTime,int)),knob.get(),SLOT(onKeyFrameSet(SequenceTime,int)));
    QObject::connect(knob.get(),SIGNAL(keyFrameRemoved(SequenceTime,int)),this,SLOT(onInternalKeyRemoved(SequenceTime,int)));
    QObject::connect(this,SIGNAL(keyFrameRemovedByUser(SequenceTime,int)),knob.get(),SLOT(onKeyFrameRemoved(SequenceTime,int)));
    QObject::connect(knob.get(),SIGNAL(secretChanged()),this,SLOT(setSecret()));
    QObject::connect(knob.get(),SIGNAL(enabledChanged()),this,SLOT(setEnabledSlot()));
}

KnobGui::~KnobGui(){
    
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
    const std::vector<Variant>& values = _knob->getValueForEachDimension();
    for(U32 i = 0; i < values.size();++i){
        updateGUI(i,values[i]);
        checkAnimationLevel(i);
    }
    setEnabled();
    setSecret();
}

void KnobGui::createAnimationButton(QGridLayout* layout,int row){
    _animationMenu = new QMenu(layout->parentWidget());
    _animationButton = new Button("A",layout->parentWidget());
    _animationButton->setToolTip("Animation menu");
    QObject::connect(_animationButton,SIGNAL(clicked()),this,SLOT(showAnimationMenu()));
    layout->addWidget(_animationButton, row, 3,Qt::AlignLeft);
}

void KnobGui::createAnimationMenu(){
    _animationMenu->clear();
    bool isOnKeyFrame = false;
    for(int i = 0; i < getKnob()->getDimension();++i){
        if(_knob->getAnimationLevel(i) == Natron::ON_KEYFRAME){
            isOnKeyFrame = true;
            break;
        }
    }
    if(!isOnKeyFrame){
        QAction* setKeyAction = new QAction(tr("Set Key"),_animationMenu);
        QObject::connect(setKeyAction,SIGNAL(triggered()),this,SLOT(onSetKeyActionTriggered()));
        _animationMenu->addAction(setKeyAction);
    }else{
        QAction* removeKeyAction = new QAction(tr("Remove Key"),_animationMenu);
        QObject::connect(removeKeyAction,SIGNAL(triggered()),this,SLOT(onRemoveKeyActionTriggered()));
        _animationMenu->addAction(removeKeyAction);
    }
    QAction* showInCurveEditorAction = new QAction(tr("Show in curve editor"),_animationMenu);
    QObject::connect(showInCurveEditorAction,SIGNAL(triggered()),this,SLOT(onShowInCurveEditorActionTriggered()));
    _animationMenu->addAction(showInCurveEditorAction);
    
    QAction* removeAnyAnimationAction = new QAction(tr("Remove animation"),_animationMenu);
    QObject::connect(removeAnyAnimationAction,SIGNAL(triggered()),this,SLOT(onRemoveAnyAnimationActionTriggered()));
    _animationMenu->addAction(removeAnyAnimationAction);
    
    
    
    QMenu* interpolationMenu = new QMenu(_animationMenu);
    interpolationMenu->setTitle("Interpolation");
    _animationMenu->addAction(interpolationMenu->menuAction());
    
    QAction* constantInterpAction = new QAction(tr("Constant"),interpolationMenu);
    QObject::connect(constantInterpAction,SIGNAL(triggered()),this,SLOT(onConstantInterpActionTriggered()));
    interpolationMenu->addAction(constantInterpAction);
    
    QAction* linearInterpAction = new QAction(tr("Linear"),interpolationMenu);
    QObject::connect(linearInterpAction,SIGNAL(triggered()),this,SLOT(onLinearInterpActionTriggered()));
    interpolationMenu->addAction(linearInterpAction);
    
    QAction* smoothInterpAction = new QAction(tr("Smooth"),interpolationMenu);
    QObject::connect(smoothInterpAction,SIGNAL(triggered()),this,SLOT(onSmoothInterpActionTriggered()));
    interpolationMenu->addAction(smoothInterpAction);
    
    QAction* catmullRomInterpAction = new QAction(tr("Catmull-Rom"),interpolationMenu);
    QObject::connect(catmullRomInterpAction,SIGNAL(triggered()),this,SLOT(onCatmullromInterpActionTriggered()));
    interpolationMenu->addAction(catmullRomInterpAction);
    
    QAction* cubicInterpAction = new QAction(tr("Cubic"),interpolationMenu);
    QObject::connect(cubicInterpAction,SIGNAL(triggered()),this,SLOT(onCubicInterpActionTriggered()));
    interpolationMenu->addAction(cubicInterpAction);
    
    QAction* horizInterpAction = new QAction(tr("Horizontal"),interpolationMenu);
    QObject::connect(horizInterpAction,SIGNAL(triggered()),this,SLOT(onHorizontalInterpActionTriggered()));
    interpolationMenu->addAction(horizInterpAction);
    
    
    QMenu* copyMenu = new QMenu(_animationMenu);
    copyMenu->setTitle("Copy");
    _animationMenu->addAction(copyMenu->menuAction());
    
    QAction* copyValuesAction = new QAction(tr("Copy values"),copyMenu);
    QObject::connect(copyValuesAction,SIGNAL(triggered()),this,SLOT(onCopyValuesActionTriggered()));
    copyMenu->addAction(copyValuesAction);
    
    QAction* copyAnimationAction = new QAction(tr("Copy animation"),copyMenu);
    QObject::connect(copyAnimationAction,SIGNAL(triggered()),this,SLOT(onCopyAnimationActionTriggered()));
    copyMenu->addAction(copyAnimationAction);
    
    QAction* pasteAction = new QAction(tr("Paste"),copyMenu);
    QObject::connect(pasteAction,SIGNAL(triggered()),this,SLOT(onPasteActionTriggered()));
    copyMenu->addAction(pasteAction);
    
    
    QAction* linkToAction = new QAction(tr("Link to"),_animationMenu);
    QObject::connect(linkToAction,SIGNAL(triggered()),this,SLOT(onLinkToActionTriggered()));
    _animationMenu->addAction(linkToAction);
    
}

void KnobGui::setSecret() {
    // If the Knob is within a group, only show it if the group is unfolded!
    // To test it:
    // try TuttlePinning: fold all groups, then switch from perspective to affine to perspective.
    //  VISIBILITY is different from SECRETNESS. The code considers that both things are equivalent, which is wrong.
    // Of course, this check has to be *recursive* (in case the group is within a folded group)
    bool showit = !_knob->isSecret();
    boost::shared_ptr<Knob> parentKnob = _knob->getParentKnob();
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
    createAnimationMenu();
    _animationMenu->exec(_animationButton->mapToGlobal(QPoint(0,0)));
}

void KnobGui::onShowInCurveEditorActionTriggered(){
    assert(_knob->getHolder()->getApp());
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

void KnobGui::onRemoveAnyAnimationActionTriggered(){
    assert(_knob->getHolder()->getApp());
#warning "FIXME: we don't want to use the curve editor's undo/redo stack, we want to use the node's panel undo/redo stack instead"
    std::vector<std::pair<CurveGui *, KeyFrame > > toRemove;
    for(int i = 0; i < _knob->getDimension();++i){
        CurveGui* curve = _knob->getHolder()->getApp()->getGui()->_curveEditor->findCurve(this, i);
        const KeyFrameSet& keys = curve->getInternalCurve()->getKeyFrames();
        for(KeyFrameSet::const_iterator it = keys.begin();it!=keys.end();++it){
            toRemove.push_back(std::make_pair(curve,*it));
        }
    }
    _knob->getHolder()->getApp()->getGui()->_curveEditor->removeKeyFrames(toRemove);
    //refresh the gui so it doesn't indicate the parameter is animated anymore
    for(int i = 0; i < _knob->getDimension();++i){
        onInternalValueChanged(i);
    }
}

void KnobGui::setInterpolationForDimensions(const std::vector<int>& dimensions,Natron::KeyframeType interp){
    for(U32 i = 0; i < dimensions.size();++i){
        boost::shared_ptr<Curve> c = _knob->getCurve(dimensions[i]);
        const KeyFrameSet& keyframes = c->getKeyFrames();
        for(KeyFrameSet::const_iterator it = keyframes.begin();it!=keyframes.end();++it){
            c->setKeyFrameInterpolation(interp, it->getTime());
        }
    }
    emit keyInterpolationChanged();
}

void KnobGui::onConstantInterpActionTriggered(){
    std::vector<int> dims;
    for(int i = 0; i < _knob->getDimension();++i){
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims,Natron::KEYFRAME_CONSTANT);
}

void KnobGui::onLinearInterpActionTriggered(){
    std::vector<int> dims;
    for(int i = 0; i < _knob->getDimension();++i){
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims,Natron::KEYFRAME_LINEAR);
}

void KnobGui::onSmoothInterpActionTriggered(){
    std::vector<int> dims;
    for(int i = 0; i < _knob->getDimension();++i){
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims,Natron::KEYFRAME_SMOOTH);
}

void KnobGui::onCatmullromInterpActionTriggered(){
    std::vector<int> dims;
    for(int i = 0; i < _knob->getDimension();++i){
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims,Natron::KEYFRAME_CATMULL_ROM);
}

void KnobGui::onCubicInterpActionTriggered(){
    std::vector<int> dims;
    for(int i = 0; i < _knob->getDimension();++i){
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims,Natron::KEYFRAME_CUBIC);
}

void KnobGui::onHorizontalInterpActionTriggered(){
    std::vector<int> dims;
    for(int i = 0; i < _knob->getDimension();++i){
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims,Natron::KEYFRAME_HORIZONTAL);
}

void KnobGui::setKeyframe(SequenceTime time,int dimension){
    emit keyFrameSetByUser(time,dimension);
    emit keyFrameSet();
}

void KnobGui::onSetKeyActionTriggered(){
    assert(_knob->getHolder()->getApp());
    //get the current time on the global timeline
    SequenceTime time = _knob->getHolder()->getApp()->getTimeLine()->currentFrame();
    for(int i = 0; i < _knob->getDimension();++i){
        setKeyframe(time,i);
    }
    
}

void KnobGui::removeKeyFrame(SequenceTime time,int dimension){
    emit keyFrameRemovedByUser(time,dimension);
    emit keyFrameRemoved();
    updateGUI(dimension,_knob->getValue(dimension));
    checkAnimationLevel(dimension);
}

void KnobGui::onRemoveKeyActionTriggered(){
    assert(_knob->getHolder()->getApp());
    //get the current time on the global timeline
    SequenceTime time = _knob->getHolder()->getApp()->getTimeLine()->currentFrame();
    for(int i = 0; i < _knob->getDimension();++i){
        removeKeyFrame(time,i);
    }
}

void KnobGui::hide(){
    _hide();
    if(_animationButton)
        _animationButton->hide();
    //also  hide the curve from the curve editor if there's any
    if(_knob->getHolder()->getApp()){
        _knob->getHolder()->getApp()->getGui()->_curveEditor->hideCurves(this);
    }
    
}

void KnobGui::show(){
    _show();
    if(_animationButton)
        _animationButton->show();
    //also show the curve from the curve editor if there's any
    if(_knob->getHolder()->getApp()){
        _knob->getHolder()->getApp()->getGui()->_curveEditor->showCurves(this);
    }
}

void KnobGui::setEnabledSlot(){
    setEnabled();
    if(_knob->getHolder()->getApp()){
        if(!_knob->isEnabled()){
            _knob->getHolder()->getApp()->getGui()->_curveEditor->hideCurves(this);
        }else{
            _knob->getHolder()->getApp()->getGui()->_curveEditor->showCurves(this);
        }
    }
}

void KnobGui::onInternalValueChanged(int dimension){
    if(_widgetCreated){
        updateGUI(dimension,_knob->getValue(dimension));
        checkAnimationLevel(dimension);
    }
}

void KnobGui::onInternalKeySet(SequenceTime,int){
    emit keyFrameSet();
}

void KnobGui::onInternalKeyRemoved(SequenceTime,int){
    emit keyFrameRemoved();
}

void KnobGui::onCopyValuesActionTriggered(){
    
}

void KnobGui::onCopyAnimationActionTriggered(){
    
}

void KnobGui::onPasteActionTriggered(){
    
}


LinkToKnobDialog::LinkToKnobDialog(KnobGui* from,QWidget* parent)
: QDialog(parent)
{
    
    
    _mainLayout = new QVBoxLayout(this);
    setLayout(_mainLayout);
    
    _firstLine = new QWidget(this);
    _firstLineLayout = new QHBoxLayout(_firstLine);
    _firstLine->setLayout(_firstLineLayout);
    
    _mainLayout->addWidget(_firstLine);
    
    _buttonsWidget = new QWidget(this);
    _buttonsLayout = new QHBoxLayout(_buttonsWidget);
    _buttonsWidget->setLayout(_buttonsLayout);
    
    _mainLayout->addWidget(_buttonsWidget);
    
    _selectKnobLabel = new QLabel("Target:",_firstLine);
    _firstLineLayout->addWidget(_selectKnobLabel);
    
    _selectionCombo = new QComboBox(_firstLine);
    _firstLineLayout->addWidget(_selectionCombo);
    _selectionCombo->setEditable(true);
    
    QStringList comboItems;
    std::vector<Natron::Node*> allActiveNodes;
    
    assert(from->getKnob()->getHolder()->getApp());

    from->getKnob()->getHolder()->getApp()->getActiveNodes(&allActiveNodes);
    for (U32 i = 0; i < allActiveNodes.size(); ++i) {
        const std::vector< boost::shared_ptr<Knob> >& knobs = allActiveNodes[i]->getKnobs();
        
        for (U32 j = 0; j < knobs.size(); ++j) {
            QString name(allActiveNodes[i]->getName().c_str());
            name.append("/");
            name.append(knobs[j]->getDescription().c_str());
            _allKnobs.insert(std::make_pair(name,knobs[j]));
            comboItems.push_back(name);
        }
    }
    _selectionCombo->addItems(comboItems);
    _selectionCombo->lineEdit()->selectAll();
    _selectionCombo->setFocus();
    
    _cancelButton = new Button("Cancel",_buttonsWidget);
    QObject::connect(_cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    _buttonsLayout->addWidget(_cancelButton);
    _okButton = new Button("Ok",_buttonsWidget);
    QObject::connect(_okButton, SIGNAL(clicked()), this, SLOT(accept()));
    _buttonsLayout->QLayout::addWidget(_okButton);
}

boost::shared_ptr<Knob> LinkToKnobDialog::getSelectedKnobs() const {
    QString str = _selectionCombo->itemText(_selectionCombo->currentIndex());
    std::map<QString,boost::shared_ptr<Knob> >::const_iterator it = _allKnobs.find(str);
    if(it != _allKnobs.end()){
        return it->second;
    }else{
        return boost::shared_ptr<Knob>();
    }
}

void KnobGui::onLinkToActionTriggered(){
    LinkToKnobDialog dialog(this,_animationButton->parentWidget());
    
    if(dialog.exec()){
        boost::shared_ptr<Knob> otherKnob = dialog.getSelectedKnobs();
        if(otherKnob){
            
            if(otherKnob->typeName() != _knob->typeName()){
                std::string err("Cannot link ");
                err.append(_knob->getDescription());
                err.append(" of type ");
                err.append(_knob->typeName());
                err.append(" to ");
                err.append(otherKnob->getDescription());
                err.append(" which is of type ");
                err.append(otherKnob->typeName());
                errorDialog("Knob Link", err);
                return;
            }
            if(otherKnob->getDimension() != _knob->getDimension()){
                std::string err("Cannot link ");
                err.append(_knob->getDescription());
                err.append(" of dimension ");
                err.append(QString::number(_knob->getDimension()).toStdString());
                err.append(" to ");
                err.append(otherKnob->getDescription());
                err.append(" which is of dimension ");
                err.append(QString::number(otherKnob->getDimension()).toStdString());
                errorDialog("Knob Link", err);
                return;
            }
            
            for(int i = 0; i < _knob->getDimension();++i){
                boost::shared_ptr<Knob> existingLink = _knob->isCurveSlave(i);
                if(existingLink){
                    std::string err("Cannot link ");
                    err.append(_knob->getDescription());
                    err.append(" \n because the knob is already linked to ");
                    err.append(existingLink->getDescription());
                    errorDialog("Knob Link", err);
                    return;
                }
                
                _knob->slaveTo(i, otherKnob);
            }
        }
        
    }
    
}

void KnobGui::checkAnimationLevel(int dimension){
    AnimationLevel level = Natron::NO_ANIMATION;
    if(getKnob()->getHolder()->getApp()){
        
        boost::shared_ptr<Curve> c = getKnob()->getCurve(dimension);
        SequenceTime time = getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
        if (c->keyFramesCount() >= 1) {
            const KeyFrameSet &keys = c->getKeyFrames();
            bool found = false;
            for (KeyFrameSet::const_iterator it = keys.begin(); it != keys.end(); ++it) {
                if (it->getTime() == time) {
                    found = true;
                    break;
                }
            }
            if(found){
                level = Natron::ON_KEYFRAME;
            }else{
                level = Natron::INTERPOLATED_VALUE;
            }
        } else {
            level = Natron::NO_ANIMATION;
        }
    }
    _knob->setAnimationLevel(dimension,level);
    reflectAnimationLevel(dimension, level);
}

bool KnobGui::setValue(int dimension,const Variant& variant,KeyFrame* newKey){
    bool ret = _knob->onValueChanged(dimension, variant, newKey);
    updateGUI(dimension,variant);
    checkAnimationLevel(dimension);
    return ret;
}
