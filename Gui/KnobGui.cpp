//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "Gui/KnobUndoCommand.h"
#include "Gui/KnobGui.h"

#include <cassert>
#include <climits>
#include <cfloat>
#include <stdexcept>

#include <QtCore/QString>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFormLayout>
#include <QFileDialog>
#include <QTextEdit>
#include <QStyle> // in QtGui on Qt4, in QtWidgets on Qt5

CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QColorDialog>
#include <QGroupBox>
#include <QtGui/QVector4D>
#include <QStyleFactory>
#include <QLabel>
#include <QMenu>
#include <QComboBox>

#include "Engine/LibraryBinary.h"
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
#include "Engine/KnobSerialization.h"

#include "Gui/AnimationButton.h"
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
#include "Gui/KnobGuiTypes.h"
#include "Gui/CurveWidget.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiAppInstance.h"

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
, _copyRightClickMenu(new QMenu(container))
, _fieldLayout(NULL)
, _row(-1)
, _containerLayout(NULL)
, _field(NULL)
, _descriptionLabel(NULL)
{
    QObject::connect(knob.get(),SIGNAL(valueChanged(int)),this,SLOT(onInternalValueChanged(int)));
    QObject::connect(knob.get(),SIGNAL(keyFrameSet(SequenceTime,int)),this,SLOT(onInternalKeySet(SequenceTime,int)));
    QObject::connect(this,SIGNAL(keyFrameSetByUser(SequenceTime,int)),knob.get(),SLOT(onKeyFrameSet(SequenceTime,int)));
    QObject::connect(knob.get(),SIGNAL(keyFrameRemoved(SequenceTime,int)),this,SLOT(onInternalKeyRemoved(SequenceTime,int)));
    QObject::connect(this,SIGNAL(keyFrameRemovedByUser(SequenceTime,int)),knob.get(),SLOT(onKeyFrameRemoved(SequenceTime,int)));
    QObject::connect(knob.get(),SIGNAL(secretChanged()),this,SLOT(setSecret()));
    QObject::connect(knob.get(),SIGNAL(enabledChanged()),this,SLOT(setEnabledSlot()));
    QObject::connect(knob.get(), SIGNAL(restorationComplete()), this, SLOT(onRestorationComplete()));
    QObject::connect(knob.get(), SIGNAL(readOnlyChanged(bool,int)),this,SLOT(onReadOnlyChanged(bool,int)));
}

KnobGui::~KnobGui(){
    
    delete _animationButton;
    delete _animationMenu;
}

Gui* KnobGui::getGui() const {
    return _container->getGui();
}

const QUndoCommand* KnobGui::getLastUndoCommand() const{
    return _container->getLastUndoCommand();
}

void KnobGui::pushUndoCommand(QUndoCommand* cmd){
    if(_knob->canBeUndone() && !_knob->isInsignificant()){
        _container->pushUndoCommand(cmd);
    }else{
        cmd->redo();
    }
}


void KnobGui::createGUI(QFormLayout* containerLayout,QWidget* fieldContainer,QWidget* label,QHBoxLayout* layout,int row) {
    _fieldLayout = layout;
    _row = row;
    _containerLayout = containerLayout;
    _field = fieldContainer;
    _descriptionLabel = label;
    createWidget(layout);
    if(_knob->isAnimationEnabled() && _knob->typeName() != File_Knob::typeNameStatic()){
        createAnimationButton(layout);
    }
    _widgetCreated = true;
    const std::vector<Variant>& values = _knob->getValueForEachDimension();
    for(U32 i = 0; i < values.size();++i){
        updateGUI(i,values[i]);
        checkAnimationLevel(i);
    }
    setEnabled();

}

void KnobGui::createAnimationButton(QHBoxLayout* layout) {
    _animationMenu = new QMenu(layout->parentWidget());
    _animationButton = new AnimationButton(this,"A",layout->parentWidget());
    _animationButton->setToolTip(Qt::convertFromPlainText("Animation menu", Qt::WhiteSpaceNormal));
    QObject::connect(_animationButton,SIGNAL(clicked()),this,SLOT(showAnimationMenu()));
    layout->addWidget(_animationButton);
    
    if (getKnob()->isSecret()) {
        _animationButton->hide();
    }
}

void KnobGui::onRightClickClicked(const QPoint& pos) {
    QWidget *widget = qobject_cast<QWidget *>(sender());
    if (widget) {
        QString objName = widget->objectName();
        objName = objName.remove("dim-");
        showRightClickMenuForDimension(pos, objName.toInt());
    }
}

void KnobGui::enableRightClickMenu(QWidget* widget,int dimension) {
    QString name("dim-");
    name.append(QString::number(dimension));
    widget->setContextMenuPolicy(Qt::CustomContextMenu);
    widget->setObjectName(name);
    QObject::connect(widget,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(onRightClickClicked(QPoint)));
    
}

void KnobGui::showRightClickMenuForDimension(const QPoint&,int dimension) {
    
    bool enabled = getKnob()->isEnabled(dimension);
    
    if (getKnob()->isSecret()) {
        return;
    }
    
    _copyRightClickMenu->clear();
    
    bool isSlave = getKnob()->isSlave(dimension);
    
    QAction* copyValuesAction = new QAction(tr("Copy value"),_copyRightClickMenu);
    copyValuesAction->setData(QVariant(dimension));
    QObject::connect(copyValuesAction,SIGNAL(triggered()),this,SLOT(onCopyValuesActionTriggered()));
    _copyRightClickMenu->addAction(copyValuesAction);
    copyValuesAction->setEnabled(false);

    if(!isSlave) {
        
        bool isClipBoardEmpty = appPTR->isClipBoardEmpty();
        
        QAction* pasteAction = new QAction(tr("Paste value"),_copyRightClickMenu);
        pasteAction->setData(QVariant(dimension));
        QObject::connect(pasteAction,SIGNAL(triggered()),this,SLOT(onPasteValuesActionTriggered()));
        _copyRightClickMenu->addAction(pasteAction);
        if (isClipBoardEmpty || !enabled) {
            pasteAction->setEnabled(false);
        }
        
    }
    
    QAction* resetDefaultAction = new QAction(tr("Set default value"),_copyRightClickMenu);
    resetDefaultAction->setData(QVariant(dimension));
    QObject::connect(resetDefaultAction,SIGNAL(triggered()),this,SLOT(onResetDefaultValuesActionTriggered()));
    _copyRightClickMenu->addAction(resetDefaultAction);
    if (isSlave || !enabled) {
        resetDefaultAction->setEnabled(false);
    }
    
    if(!isSlave && enabled) {
        QAction* linkToAction = new QAction(tr("Link to"),_copyRightClickMenu);
        linkToAction->setData(QVariant(dimension));
        QObject::connect(linkToAction,SIGNAL(triggered()),this,SLOT(onLinkToActionTriggered()));
        _copyRightClickMenu->addAction(linkToAction);
    } else if(isSlave) {
        QAction* unlinkAction = new QAction(tr("Unlink"),_copyRightClickMenu);
        unlinkAction->setData(QVariant(dimension));
        QObject::connect(unlinkAction,SIGNAL(triggered()),this,SLOT(onUnlinkActionTriggered()));
        _copyRightClickMenu->addAction(unlinkAction);
    }

    _copyRightClickMenu->exec(QCursor::pos());

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
    
    bool isSlave = false;
    bool hasAnimation = false;
    for (int i = 0; i < getKnob()->getDimension(); ++i) {
        if (getKnob()->isSlave(i)) {
            isSlave = true;
        }
        if (getKnob()->getKeyFramesCount(i) > 0) {
            hasAnimation = true;
        }
        if (isSlave && hasAnimation) {
            break;
        }
    }
    
    if(!isSlave) {
        if(!isOnKeyFrame){
            QAction* setKeyAction = new QAction(tr("Set Key"),_animationMenu);
            QObject::connect(setKeyAction,SIGNAL(triggered()),this,SLOT(onSetKeyActionTriggered()));
            _animationMenu->addAction(setKeyAction);
        }else{
            QAction* removeKeyAction = new QAction(tr("Remove Key"),_animationMenu);
            QObject::connect(removeKeyAction,SIGNAL(triggered()),this,SLOT(onRemoveKeyActionTriggered()));
            _animationMenu->addAction(removeKeyAction);
        }
        
        QAction* removeAnyAnimationAction = new QAction(tr("Remove animation"),_animationMenu);
        QObject::connect(removeAnyAnimationAction,SIGNAL(triggered()),this,SLOT(onRemoveAnyAnimationActionTriggered()));
        _animationMenu->addAction(removeAnyAnimationAction);
        if (!hasAnimation) {
            removeAnyAnimationAction->setEnabled(false);
        }
        
        
        
        
    }
    QAction* showInCurveEditorAction = new QAction(tr("Show in curve editor"),_animationMenu);
    QObject::connect(showInCurveEditorAction,SIGNAL(triggered()),this,SLOT(onShowInCurveEditorActionTriggered()));
    _animationMenu->addAction(showInCurveEditorAction);
    
    if(!isSlave) {
        
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
        
    }

    QAction* copyAnimationAction = new QAction(tr("Copy animation"),_animationMenu);
    QObject::connect(copyAnimationAction,SIGNAL(triggered()),this,SLOT(onCopyAnimationActionTriggered()));
    _animationMenu->addAction(copyAnimationAction);
    
    if(!isSlave) {
        
        bool isClipBoardEmpty = appPTR->isClipBoardEmpty();
        
        QAction* pasteAction = new QAction(tr("Paste animation"),_animationMenu);
        QObject::connect(pasteAction,SIGNAL(triggered()),this,SLOT(onPasteAnimationActionTriggered()));
        _animationMenu->addAction(pasteAction);
        if (isClipBoardEmpty) {
            pasteAction->setEnabled(false);
        }
    }
    
    
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
    getGui()->setCurveEditorOnTop();
    std::vector<boost::shared_ptr<Curve> > curves;
    for(int i = 0; i < _knob->getDimension();++i){
        boost::shared_ptr<Curve> c = _knob->getCurve(i);
        if(c->isAnimated()){
            curves.push_back(c);
        }
    }
    if(!curves.empty()){
        getGui()->getCurveEditor()->centerOn(curves);
    }
    
    
}

void KnobGui::onRemoveAnyAnimationActionTriggered(){
    assert(_knob->getHolder()->getApp());
    std::vector<std::pair<CurveGui *, KeyFrame > > toRemove;
    for(int i = 0; i < _knob->getDimension();++i){
        CurveGui* curve = getGui()->getCurveEditor()->findCurve(this, i);
        const KeyFrameSet& keys = curve->getInternalCurve()->getKeyFrames();
        for(KeyFrameSet::const_iterator it = keys.begin();it!=keys.end();++it){
            toRemove.push_back(std::make_pair(curve,*it));
        }
    }
    pushUndoCommand(new RemoveKeysCommand(getGui()->getCurveEditor()->getCurveWidget(),
                                          toRemove));
    //refresh the gui so it doesn't indicate the parameter is animated anymore
    for(int i = 0; i < _knob->getDimension();++i){
        onInternalValueChanged(i);
    }
}

void KnobGui::setInterpolationForDimensions(const std::vector<int>& dimensions,Natron::KeyframeType interp){
    for(U32 i = 0; i < dimensions.size();++i){
        boost::shared_ptr<Curve> c = _knob->getCurve(dimensions[i]);
        int kfCount = c->keyFramesCount();
        for(int j = 0;j < kfCount;++j){
            c->setKeyFrameInterpolation(interp, j);
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
        CurveGui* curve = getGui()->getCurveEditor()->findCurve(this, i);
        assert(curve);
        std::vector<KeyFrame> kVec;
        kVec.push_back(KeyFrame((double)time,_knob->getValue<double>(i)));
        pushUndoCommand(new AddKeysCommand(getGui()->getCurveEditor()->getCurveWidget(),
                                           curve,kVec));
    }
    
}

void KnobGui::removeKeyFrame(SequenceTime time,int dimension){
    emit keyFrameRemovedByUser(time,dimension);
    emit keyFrameRemoved();
    updateGUI(dimension,_knob->getValue(dimension));
    checkAnimationLevel(dimension);
}

QString KnobGui::toolTip() const
{
    return Qt::convertFromPlainText(getKnob()->getHintToolTip().c_str(), Qt::WhiteSpaceNormal);
}

bool KnobGui::hasToolTip() const {
    return !getKnob()->getHintToolTip().empty();
}

void KnobGui::onRemoveKeyActionTriggered(){
    assert(_knob->getHolder()->getApp());
    //get the current time on the global timeline
    SequenceTime time = _knob->getHolder()->getApp()->getTimeLine()->currentFrame();
    std::vector<std::pair<CurveGui*,KeyFrame> > toRemove;
    for(int i = 0; i < _knob->getDimension();++i){
        CurveGui* curve = getGui()->getCurveEditor()->findCurve(this, i);
        toRemove.push_back(std::make_pair(curve,KeyFrame(time,_knob->getValue<double>(i))));
    }
    pushUndoCommand(new RemoveKeysCommand(getGui()->getCurveEditor()->getCurveWidget(),
                                          toRemove));
}


void KnobGui::hide(){
    _hide();
    if(_animationButton)
        _animationButton->hide();
    //also  hide the curve from the curve editor if there's any
    if(_knob->getHolder()->getApp()){
       getGui()->getCurveEditor()->hideCurves(this);
    }
    
    if (!_knob->isNewLineTurnedOff() && _field->objectName() != "multi-line") {
        _containerLayout->removeWidget(_field);
        _containerLayout->removeWidget(_descriptionLabel);
        _field->setParent(0);
        _descriptionLabel->setParent(0);
        _field->hide();
    }
    _descriptionLabel->hide();
}
void KnobGui::show(){
    _show();
    if(_animationButton)
        _animationButton->show();
    //also show the curve from the curve editor if there's any
    if(_knob->getHolder()->getApp()){
        getGui()->getCurveEditor()->showCurves(this);
    }
    
    if (!_knob->isNewLineTurnedOff() && _field->objectName() != "multi-line") {
        _containerLayout->insertRow(_row, _descriptionLabel, _field);
        _field->setParent(_containerLayout->parentWidget());
        _descriptionLabel->setParent(_containerLayout->parentWidget());
        _field->show();
    } 
    _descriptionLabel->show();

}

void KnobGui::setEnabledSlot(){
    setEnabled();
    if(_knob->getHolder()->getApp()){
        for (int i = 0; i < _knob->getDimension(); ++i) {
            if(!_knob->isEnabled(i)){
                getGui()->getCurveEditor()->hideCurve(this,i);
            }else{
                getGui()->getCurveEditor()->showCurve(this,i);
            }
        }
    }
}

void KnobGui::onInternalValueChanged(int dimension) {
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
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        copyValues(action->data().toInt());
    }
}

void KnobGui::copyValues(int dimension) {
    KnobSerialization k;
    k.initialize(_knob.get());
    appPTR->setKnobClipBoard(k,false,dimension);

}

void KnobGui::onCopyAnimationActionTriggered(){
    KnobSerialization k;
    k.initialize(_knob.get());
    appPTR->setKnobClipBoard(k,true,-1);
}

void KnobGui::onPasteAnimationActionTriggered() {
    if (appPTR->isClipBoardEmpty()) {
        return;
    }

    KnobSerialization k;
    bool copyAnimation;
    int dimension; //<unused here since we copy all dimensions
    appPTR->getKnobClipBoard(&k, &copyAnimation,&dimension);
    
    if (!copyAnimation) {
        return;
    }
    
    _knob->beginValueChange(Natron::PLUGIN_EDITED);
    
    const std::vector<Variant>& values =  k.getValues();
    
    const Variant& thisValue = _knob->getValue();
    if (thisValue.type() != values[0].type()) {
        QString err = QString("Cannot paste values of a %1 parameter to a %2 parameter")
        .arg(values[0].typeName()).arg(thisValue.typeName());
        
        Natron::errorDialog("Paste",err.toStdString());
        return;
    }
    
    if ((int)values.size() == _knob->getDimension()) {
        if(!copyAnimation) {
            pushValueChangedCommand(values);
        }else {
            for (U32 i = 0; i < values.size(); ++i) {
                _knob->setValue(values[i], i,true);
            }
        }
    }else{
        Natron::errorDialog("Paste value", "You cannot copy/paste values from/to parameters with different dimensions.");
    }


    const  std::vector< boost::shared_ptr<Curve> >& curves = k.getCurves();
    if((int)curves.size() == _knob->getDimension()){
        for (U32 i = 0; i < curves.size(); ++i) {
            _knob->getCurve(i)->clone(*curves[i]);
        }
        emit keyFrameSet();
    }else{
        Natron::errorDialog("Paste animation", "You cannot copy/paste animation from/to parameters with different dimensions.");
    }
    
    _knob->endValueChange();


}

void KnobGui::pasteValues(int dimension) {
    
    if (appPTR->isClipBoardEmpty()) {
        return;
    }
    
    KnobSerialization k;
    bool copyAnimation;
    int dimensionToCopyFrom;
    appPTR->getKnobClipBoard(&k, &copyAnimation,&dimensionToCopyFrom);
    
    
    const std::vector<Variant>& values =  k.getValues();
    
    std::vector<Variant> thisValues = _knob->getValueForEachDimension();
    assert(!thisValues.empty());
    if (!values[0].canConvert(thisValues[0].type())) {
        QString err = QString("Cannot paste values of a %1 parameter to a %2 parameter")
        .arg(values[0].typeName()).arg(thisValues[0].typeName());
        
        Natron::errorDialog("Paste",err.toStdString());
        return;
    }
    
    thisValues[dimension] = values[dimensionToCopyFrom];
    pushValueChangedCommand(thisValues);
    
}

void KnobGui::onPasteValuesActionTriggered(){
    
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        pasteValues(action->data().toInt());
    }
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
    
    _selectionCombo = new ComboBox(_firstLine);
    _firstLineLayout->addWidget(_selectionCombo);
    
    std::vector<Natron::Node*> allActiveNodes;
    
    assert(from->getKnob()->getHolder()->getApp());
    
    from->getKnob()->getHolder()->getApp()->getActiveNodes(&allActiveNodes);
    for (U32 i = 0; i < allActiveNodes.size(); ++i) {
        const std::vector< boost::shared_ptr<Knob> >& knobs = allActiveNodes[i]->getKnobs();
        
        for (U32 j = 0; j < knobs.size(); ++j) {
            if(!knobs[j]->isSecret() && knobs[j] != from->getKnob()) {
                if (from->getKnob()->isTypeCompatible(*knobs[j])) {
                    for (int k = 0; k < knobs[j]->getDimension(); ++k) {
                        if (!knobs[j]->isSlave(k) && knobs[j]->isEnabled(k)) {
                            QString name(allActiveNodes[i]->getName().c_str());
                            name.append('.');
                            name.append(knobs[j]->getDescription().c_str());
                            QString dimensionName = knobs[j]->getDimensionName(k).c_str();
                            if (!dimensionName.isEmpty()) {
                                name.append('.');
                                name.append(dimensionName);
                            }
                            _allKnobs.insert(std::make_pair(name, std::make_pair(k,knobs[j])));
                            _selectionCombo->addItem(name);
                        }
                    }
                }
                
            }
        }
    }
    _selectionCombo->setFocus();
    
    _cancelButton = new Button("Cancel",_buttonsWidget);
    QObject::connect(_cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    _buttonsLayout->addWidget(_cancelButton);
    _okButton = new Button("Ok",_buttonsWidget);
    QObject::connect(_okButton, SIGNAL(clicked()), this, SLOT(accept()));
    _buttonsLayout->QLayout::addWidget(_okButton);
}

std::pair<int,boost::shared_ptr<Knob> > LinkToKnobDialog::getSelectedKnobs() const {
    QString str = _selectionCombo->itemText(_selectionCombo->activeIndex());
    std::map<QString,std::pair<int,boost::shared_ptr<Knob > > >::const_iterator it = _allKnobs.find(str);
    if(it != _allKnobs.end()){
        return it->second;
    }else{
        return std::make_pair(-1,boost::shared_ptr<Knob>());
    }
}

void KnobGui::linkTo(int dimension) {
    LinkToKnobDialog dialog(this,_copyRightClickMenu->parentWidget());
    
    if(dialog.exec()){
        std::pair<int,boost::shared_ptr<Knob> > otherKnob = dialog.getSelectedKnobs();
        if(otherKnob.second){
            
            Variant thisValue = getKnob()->getValue();
            Variant otherKnobValue = otherKnob.second->getValue();
         
            
            if (!getKnob()->isTypeCompatible(*otherKnob.second)) {
                std::string err("Cannot link ");
                err.append(_knob->getDescription());
                err.append(" of type ");
                err.append(thisValue.typeName());
                err.append(" to ");
                err.append(otherKnob.second->getDescription());
                err.append(" which is of type ");
                err.append(otherKnobValue.typeName());
                errorDialog("Knob Link", err);
                return;
            }
            
            assert(otherKnob.first < otherKnob.second->getDimension());
            
            std::pair<int,boost::shared_ptr<Knob> > existingLink = _knob->getMaster(otherKnob.first);
            if(existingLink.second){
                std::string err("Cannot link ");
                err.append(_knob->getDescription());
                err.append(" \n because the knob is already linked to ");
                err.append(existingLink.second->getDescription());
                errorDialog("Knob Link", err);
                return;
            }
            
            _knob->slaveTo(dimension, otherKnob.second,otherKnob.first);
            updateGUI(dimension,_knob->getValue(otherKnob.first));
            checkAnimationLevel(otherKnob.first);
            emit keyFrameRemoved();
            QObject::connect(otherKnob.second.get(), SIGNAL(updateSlaves(int)), _knob.get(), SLOT(onMasterChanged(int)));
            
            setReadOnly(true, dimension);
        }
        
    }

}

void KnobGui::onLinkToActionTriggered() {
    
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        linkTo(action->data().toInt());
    }
    
}

void KnobGui::unlink(int dimension) {
    std::pair<int,boost::shared_ptr<Knob> > other = _knob->getMaster(dimension);
    _knob->unSlave(dimension);
    updateGUI(dimension,_knob->getValue(dimension));
    checkAnimationLevel(dimension);
    emit keyFrameSet();
    QObject::disconnect(other.second.get(), SIGNAL(updateSlaves(int)), _knob.get(), SLOT(onMasterChanged(int)));
    setReadOnly(false,dimension);
}

void KnobGui::onUnlinkActionTriggered() {
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        unlink(action->data().toInt());
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

int KnobGui::setValue(int dimension,const Variant& variant,KeyFrame* newKey){
    
    Knob::ValueChangedReturnCode ret = _knob->onValueChanged(dimension, variant, newKey);
    if(ret > 0){
        emit keyFrameSet();
    }
    updateGUI(dimension,variant);
    checkAnimationLevel(dimension);
    return (int)ret;
}

void KnobGui::pushValueChangedCommand(const std::vector<Variant>& newValues){
    
    pushUndoCommand(new KnobUndoCommand(this, getKnob()->getValueForEachDimension(),newValues));
}

void KnobGui::pushValueChangedCommand(const Variant& v, int dimension){
    std::vector<Variant> vec(getKnob()->getDimension());
    for (int i = 0; i < getKnob()->getDimension(); ++i) {
        if(i != dimension){
            vec[i] = getKnob()->getValue(i);
        }else{
            vec[i] = v;
        }
    }
    pushUndoCommand(new KnobUndoCommand(this, getKnob()->getValueForEachDimension(),vec));
}

void KnobGui::onRestorationComplete() {
    for (int i = 0; i < _knob->getDimension(); ++i) {
        std::pair<int,boost::shared_ptr<Knob> > other = _knob->getMaster(i);
        if(other.second) {
            QObject::connect(other.second.get(), SIGNAL(updateSlaves(int)), _knob.get(), SLOT(onMasterChanged(int)));
        }
    }
}

void KnobGui::onResetDefaultValuesActionTriggered() {
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        resetDefault(action->data().toInt());
    }
}

void KnobGui::resetDefault(int dimension) {
    ///this cannot be undone for now. it's kinda lots of effort to do it and frankly not much necessary.
    getKnob()->resetToDefaultValue(dimension);
}

void KnobGui::onReadOnlyChanged(bool b,int d) {
    setReadOnly(b,d);
}