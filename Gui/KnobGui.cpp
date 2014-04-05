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
#include "Engine/KnobFile.h"
#include "Engine/Curve.h"
#include "Engine/KnobTypes.h"
#include "Engine/TimeLine.h"
#include "Engine/KnobSerialization.h"
#include "Engine/Project.h"

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


struct KnobGui::KnobGuiPrivate
{
    bool triggerNewLine;
    int spacingBetweenItems;
    bool widgetCreated;
    DockablePanel* const container;
    QMenu* animationMenu;
    AnimationButton* animationButton;
    QMenu* copyRightClickMenu;
    QHBoxLayout* fieldLayout; //< the layout containing the widgets of the knob
    int row;
    
    ////A vector of all other knobs on the same line.
    std::vector< boost::shared_ptr< KnobI > > knobsOnSameLine;
    
    QFormLayout* containerLayout;
    QWidget* field;
    QWidget* descriptionLabel;
    bool isOnNewLine;
    
    KnobGuiPrivate(DockablePanel* container)
    : triggerNewLine(true)
    , spacingBetweenItems(0)
    , widgetCreated(false)
    , container(container)
    , animationMenu(NULL)
    , animationButton(NULL)
    , copyRightClickMenu(new QMenu(container))
    , fieldLayout(NULL)
    , row(-1)
    , knobsOnSameLine()
    , containerLayout(NULL)
    , field(NULL)
    , descriptionLabel(NULL)
    , isOnNewLine(false)
    {
        
    }

};

/////////////// KnobGui

KnobGui::KnobGui(boost::shared_ptr<KnobI> knob,DockablePanel* container)
: _imp(new KnobGuiPrivate(container))
{
    KnobHelper* helper = dynamic_cast<KnobHelper*>(knob.get());
    KnobSignalSlotHandler* handler = helper->getSignalSlotHandler().get();
    
    QObject::connect(handler,SIGNAL(valueChanged(int)),this,SLOT(onInternalValueChanged(int)));
    QObject::connect(handler,SIGNAL(keyFrameSet(SequenceTime,int)),this,SLOT(onInternalKeySet(SequenceTime,int)));
    QObject::connect(this,SIGNAL(keyFrameSetByUser(SequenceTime,int)),handler,SLOT(onKeyFrameSet(SequenceTime,int)));
    QObject::connect(handler,SIGNAL(keyFrameRemoved(SequenceTime,int)),this,SLOT(onInternalKeyRemoved(SequenceTime,int)));
    QObject::connect(this,SIGNAL(keyFrameRemovedByUser(SequenceTime,int)),handler,SLOT(onKeyFrameRemoved(SequenceTime,int)));
    QObject::connect(handler,SIGNAL(secretChanged()),this,SLOT(setSecret()));
    QObject::connect(handler,SIGNAL(enabledChanged()),this,SLOT(setEnabledSlot()));
    QObject::connect(handler,SIGNAL(knobSlaved(int,bool)),this,SLOT(onKnobSlavedChanged(int,bool)));
    QObject::connect(handler,SIGNAL(animationRemoved(int)),this,SIGNAL(keyFrameRemoved()));

}

KnobGui::~KnobGui(){
    
    delete _imp->animationButton;
    delete _imp->animationMenu;
}

Gui* KnobGui::getGui() const {
    return _imp->container->getGui();
}

const QUndoCommand* KnobGui::getLastUndoCommand() const{
    return _imp->container->getLastUndoCommand();
}

void KnobGui::pushUndoCommand(QUndoCommand* cmd){
    if(getKnob()->getCanUndo() && getKnob()->getEvaluateOnChange()){
        _imp->container->pushUndoCommand(cmd);
    }else{
        cmd->redo();
    }
}


void KnobGui::createGUI(QFormLayout* containerLayout,QWidget* fieldContainer,QWidget* label,QHBoxLayout* layout,int row,bool isOnNewLine,
                        const std::vector< boost::shared_ptr< KnobI > >& knobsOnSameLine) {
    
    boost::shared_ptr<KnobI> knob = getKnob();
    
    _imp->fieldLayout = layout;
    _imp->row = row;
    _imp->knobsOnSameLine = knobsOnSameLine;
    _imp->containerLayout = containerLayout;
    _imp->field = fieldContainer;
    _imp->descriptionLabel = label;
    _imp->isOnNewLine = isOnNewLine;
    createWidget(layout);
    if(knob->isAnimationEnabled() && knob->typeName() != File_Knob::typeNameStatic()){
        createAnimationButton(layout);
    }
    _imp->widgetCreated = true;

    for(int i = 0; i < knob->getDimension();++i){
        updateGUI(i);
        checkAnimationLevel(i);
    }
    setEnabledSlot();
    
    if (isOnNewLine) {
        containerLayout->addRow(label, fieldContainer);
    }
    setSecret();

}

void KnobGui::createAnimationButton(QHBoxLayout* layout) {
    _imp->animationMenu = new QMenu(layout->parentWidget());
    _imp->animationButton = new AnimationButton(this,"A",layout->parentWidget());
    _imp->animationButton->setToolTip(Qt::convertFromPlainText("Animation menu", Qt::WhiteSpaceNormal));
    QObject::connect(_imp->animationButton,SIGNAL(clicked()),this,SLOT(showAnimationMenu()));
    layout->addWidget(_imp->animationButton);
    
    if (getKnob()->getIsSecret()) {
        _imp->animationButton->hide();
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
    
    boost::shared_ptr<KnobI> knob = getKnob();
    bool enabled = knob->isEnabled(dimension);
    
    if (knob->getIsSecret()) {
        return;
    }
    
    _imp->copyRightClickMenu->clear();
    
    bool isSlave = knob->isSlave(dimension);
    
    QAction* copyValuesAction = new QAction(tr("Copy value"),_imp->copyRightClickMenu);
    copyValuesAction->setData(QVariant(dimension));
    QObject::connect(copyValuesAction,SIGNAL(triggered()),this,SLOT(onCopyValuesActionTriggered()));
    _imp->copyRightClickMenu->addAction(copyValuesAction);

    if(!isSlave) {
        
        bool isClipBoardEmpty = appPTR->isClipBoardEmpty();
        
        QAction* pasteAction = new QAction(tr("Paste value"),_imp->copyRightClickMenu);
        pasteAction->setData(QVariant(dimension));
        QObject::connect(pasteAction,SIGNAL(triggered()),this,SLOT(onPasteValuesActionTriggered()));
        _imp->copyRightClickMenu->addAction(pasteAction);
        if (isClipBoardEmpty || !enabled) {
            pasteAction->setEnabled(false);
        }
        
    }
    
    QAction* resetDefaultAction = new QAction(tr("Set default value"),_imp->copyRightClickMenu);
    resetDefaultAction->setData(QVariant(dimension));
    QObject::connect(resetDefaultAction,SIGNAL(triggered()),this,SLOT(onResetDefaultValuesActionTriggered()));
    _imp->copyRightClickMenu->addAction(resetDefaultAction);
    if (isSlave || !enabled) {
        resetDefaultAction->setEnabled(false);
    }
    
    if(!isSlave && enabled) {
        QAction* linkToAction = new QAction(tr("Link to"),_imp->copyRightClickMenu);
        linkToAction->setData(QVariant(dimension));
        QObject::connect(linkToAction,SIGNAL(triggered()),this,SLOT(onLinkToActionTriggered()));
        _imp->copyRightClickMenu->addAction(linkToAction);
    } else if(isSlave) {
        QAction* unlinkAction = new QAction(tr("Unlink"),_imp->copyRightClickMenu);
        unlinkAction->setData(QVariant(dimension));
        QObject::connect(unlinkAction,SIGNAL(triggered()),this,SLOT(onUnlinkActionTriggered()));
        _imp->copyRightClickMenu->addAction(unlinkAction);
        
        
        ///a stub action just to indicate what is the master knob.
        QAction* masterNameAction = new QAction("",_imp->copyRightClickMenu);
        std::pair<int,boost::shared_ptr<KnobI> > master = knob->getMaster(dimension);
        assert(master.second);
        
        ///find-out to which node that master knob belongs to
        std::string nodeName("Linked to: ");
        
        assert(getKnob()->getHolder()->getApp());
        const std::vector<boost::shared_ptr<Natron::Node> > allNodes = knob->getHolder()->getApp()->getProject()->getCurrentNodes();
        for (U32 i = 0; i < allNodes.size(); ++i) {
            const std::vector< boost::shared_ptr<KnobI> >& knobs = allNodes[i]->getKnobs();
            bool shouldStop = false;
            for (U32 j = 0; j < knobs.size(); ++j) {
                if (knobs[j].get() == master.second.get()) {
                    nodeName.append(allNodes[i]->getName());
                    shouldStop = true;
                    break;
                }
            }
            if (shouldStop) {
                break;
            }
        }
        nodeName.append(".");
        nodeName.append(master.second->getDescription());
        if (master.second->getDimension() > 1) {
            nodeName.append(".");
            nodeName.append(master.second->getDimensionName(master.first));
        }
        masterNameAction->setText(nodeName.c_str());
        masterNameAction->setEnabled(false);
        _imp->copyRightClickMenu->addAction(masterNameAction);
        
    }
    
    
    
    _imp->copyRightClickMenu->exec(QCursor::pos());

}

void KnobGui::createAnimationMenu(){
    boost::shared_ptr<KnobI> knob = getKnob();
    _imp->animationMenu->clear();
    bool isOnKeyFrame = false;
    for(int i = 0; i < knob->getDimension();++i){
        if(knob->getAnimationLevel(i) == Natron::ON_KEYFRAME){
            isOnKeyFrame = true;
            break;
        }
    }
    
    bool isSlave = false;
    bool hasAnimation = false;
    bool isEnabled = true;
    for (int i = 0; i < knob->getDimension(); ++i) {
        if (knob->isSlave(i)) {
            isSlave = true;
        }
        if (knob->getKeyFramesCount(i) > 0) {
            hasAnimation = true;
        }
        if (isSlave && hasAnimation) {
            break;
        }
        if (!knob->isEnabled(i)) {
            isEnabled = false;
        }
    }
    
    if(!isSlave) {
        if (!isOnKeyFrame) {
            QAction* setKeyAction = new QAction(tr("Set Key"),_imp->animationMenu);
            QObject::connect(setKeyAction,SIGNAL(triggered()),this,SLOT(onSetKeyActionTriggered()));
            _imp->animationMenu->addAction(setKeyAction);
            if (!isEnabled) {
                setKeyAction->setEnabled(false);
            }
        } else {
            QAction* removeKeyAction = new QAction(tr("Remove Key"),_imp->animationMenu);
            QObject::connect(removeKeyAction,SIGNAL(triggered()),this,SLOT(onRemoveKeyActionTriggered()));
            _imp->animationMenu->addAction(removeKeyAction);
            if (!isEnabled) {
                removeKeyAction->setEnabled(false);
            }
        }
        
        QAction* removeAnyAnimationAction = new QAction(tr("Remove animation"),_imp->animationMenu);
        QObject::connect(removeAnyAnimationAction,SIGNAL(triggered()),this,SLOT(onRemoveAnyAnimationActionTriggered()));
        _imp->animationMenu->addAction(removeAnyAnimationAction);
        if (!hasAnimation || !isEnabled) {
            removeAnyAnimationAction->setEnabled(false);
        }
        
        
        
        
    }
    
    
    if(!isSlave) {
        
        QAction* showInCurveEditorAction = new QAction(tr("Show in curve editor"),_imp->animationMenu);
        QObject::connect(showInCurveEditorAction,SIGNAL(triggered()),this,SLOT(onShowInCurveEditorActionTriggered()));
        _imp->animationMenu->addAction(showInCurveEditorAction);
        if (!hasAnimation || !isEnabled) {
            showInCurveEditorAction->setEnabled(false);
        }
        
        QMenu* interpolationMenu = new QMenu(_imp->animationMenu);
        interpolationMenu->setTitle("Interpolation");
        _imp->animationMenu->addAction(interpolationMenu->menuAction());
        if (!isEnabled) {
            interpolationMenu->menuAction()->setEnabled(false);
        }
        
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

    QAction* copyAnimationAction = new QAction(tr("Copy animation"),_imp->animationMenu);
    QObject::connect(copyAnimationAction,SIGNAL(triggered()),this,SLOT(onCopyAnimationActionTriggered()));
    _imp->animationMenu->addAction(copyAnimationAction);
    if (!hasAnimation) {
        copyAnimationAction->setEnabled(false);
    }
    
    if(!isSlave) {
        
        bool isClipBoardEmpty = appPTR->isClipBoardEmpty();
        
        QAction* pasteAction = new QAction(tr("Paste animation"),_imp->animationMenu);
        QObject::connect(pasteAction,SIGNAL(triggered()),this,SLOT(onPasteAnimationActionTriggered()));
        _imp->animationMenu->addAction(pasteAction);
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
    boost::shared_ptr<KnobI> knob = getKnob();
    bool showit = !knob->getIsSecret();
    boost::shared_ptr<KnobI> parentKnob = knob->getParentKnob();
    while (showit && parentKnob && parentKnob->typeName() == "Group") {
        Group_KnobGui* parentGui = dynamic_cast<Group_KnobGui*>(_imp->container->findKnobGuiOrCreate(parentKnob,true,NULL));
        assert(parentGui);
        // check for secretness and visibility of the group
        if (parentKnob->getIsSecret() || !parentGui->isChecked()) {
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
    _imp->animationMenu->exec(_imp->animationButton->mapToGlobal(QPoint(0,0)));
}

void KnobGui::onShowInCurveEditorActionTriggered(){
    boost::shared_ptr<KnobI> knob = getKnob();
    assert(knob->getHolder()->getApp());
    getGui()->setCurveEditorOnTop();
    std::vector<boost::shared_ptr<Curve> > curves;
    for(int i = 0; i < knob->getDimension();++i){
        boost::shared_ptr<Curve> c = knob->getCurve(i);
        if(c->isAnimated()){
            curves.push_back(c);
        }
    }
    if(!curves.empty()){
        getGui()->getCurveEditor()->centerOn(curves);
    }
    
    
}

void KnobGui::onRemoveAnyAnimationActionTriggered(){
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<std::pair<CurveGui *, KeyFrame > > toRemove;
    for(int i = 0; i < knob->getDimension();++i){
        CurveGui* curve = getGui()->getCurveEditor()->findCurve(this, i);
        KeyFrameSet keys = curve->getInternalCurve()->getKeyFrames_mt_safe();
        for(KeyFrameSet::const_iterator it = keys.begin();it!=keys.end();++it){
            toRemove.push_back(std::make_pair(curve,*it));
        }
    }
    pushUndoCommand(new RemoveKeysCommand(getGui()->getCurveEditor()->getCurveWidget(),
                                          toRemove));
    //refresh the gui so it doesn't indicate the parameter is animated anymore
    for(int i = 0; i < knob->getDimension();++i){
        updateGUI(i);
    }
}

void KnobGui::setInterpolationForDimensions(const std::vector<int>& dimensions,Natron::KeyframeType interp)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    for (U32 i = 0; i < dimensions.size();++i) {
        boost::shared_ptr<Curve> c = knob->getCurve(dimensions[i]);
        int kfCount = c->getKeyFramesCount();
        for (int j = 0; j < kfCount; ++j) {
            c->setKeyFrameInterpolation(interp, j);
        }
    }
    emit keyInterpolationChanged();
}

void KnobGui::onConstantInterpActionTriggered(){
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;
    for(int i = 0; i < knob->getDimension();++i){
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims,Natron::KEYFRAME_CONSTANT);
}

void KnobGui::onLinearInterpActionTriggered(){
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;
    for(int i = 0; i < knob->getDimension();++i){
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims,Natron::KEYFRAME_LINEAR);
}

void KnobGui::onSmoothInterpActionTriggered(){
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;
    for(int i = 0; i < knob->getDimension();++i){
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims,Natron::KEYFRAME_SMOOTH);
}

void KnobGui::onCatmullromInterpActionTriggered(){
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;
    for(int i = 0; i < knob->getDimension();++i){
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims,Natron::KEYFRAME_CATMULL_ROM);
}

void KnobGui::onCubicInterpActionTriggered(){
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;
    for(int i = 0; i < knob->getDimension();++i){
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims,Natron::KEYFRAME_CUBIC);
}

void KnobGui::onHorizontalInterpActionTriggered(){
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;
    for(int i = 0; i < knob->getDimension();++i){
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims,Natron::KEYFRAME_HORIZONTAL);
}

void KnobGui::setKeyframe(double time,int dimension){
    emit keyFrameSetByUser(time,dimension);
    emit keyFrameSet();
}

void KnobGui::onSetKeyActionTriggered(){
    boost::shared_ptr<KnobI> knob = getKnob();
    assert(knob->getHolder()->getApp());
    //get the current time on the global timeline
    SequenceTime time = knob->getHolder()->getApp()->getTimeLine()->currentFrame();
    for(int i = 0; i < knob->getDimension();++i){
        CurveGui* curve = getGui()->getCurveEditor()->findCurve(this, i);
        assert(curve);
        std::vector<KeyFrame> kVec;
        KeyFrame kf;
        kf.setTime(time);
        Knob<int>* isInt = dynamic_cast<Knob<int>*>(knob.get());
        Knob<bool>* isBool = dynamic_cast<Knob<bool>*>(knob.get());
        AnimatingString_KnobHelper* isString = dynamic_cast<AnimatingString_KnobHelper*>(knob.get());
        Knob<double>* isDouble = dynamic_cast<Knob<double>*>(knob.get());
        
        if (isInt) {
            kf.setValue(isInt->getValue(i));
        } else if (isBool) {
            kf.setValue(isBool->getValue(i));
        } else if (isDouble) {
            kf.setValue(isDouble->getValue(i));
        } else if (isString) {
            std::string v = isString->getValue(i);
            double dv;
            isString->stringToKeyFrameValue(time, v, &dv);
            kf.setValue(dv);
        }

        kVec.push_back(kf);
        pushUndoCommand(new AddKeysCommand(getGui()->getCurveEditor()->getCurveWidget(),
                                           curve,kVec));
    }
    
}

void KnobGui::removeKeyFrame(double time,int dimension){
    emit keyFrameRemovedByUser(time,dimension);
    emit keyFrameRemoved();
    updateGUI(dimension);
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
    boost::shared_ptr<KnobI> knob = getKnob();
    assert(knob->getHolder()->getApp());
    //get the current time on the global timeline
    SequenceTime time = knob->getHolder()->getApp()->getTimeLine()->currentFrame();
    std::vector<std::pair<CurveGui*,KeyFrame> > toRemove;
    for(int i = 0; i < knob->getDimension();++i){
        CurveGui* curve = getGui()->getCurveEditor()->findCurve(this, i);
        
        KeyFrame kf;
        kf.setTime(time);
        Knob<int>* isInt = dynamic_cast<Knob<int>*>(knob.get());
        Knob<bool>* isBool = dynamic_cast<Knob<bool>*>(knob.get());
        AnimatingString_KnobHelper* isString = dynamic_cast<AnimatingString_KnobHelper*>(knob.get());
        Knob<double>* isDouble = dynamic_cast<Knob<double>*>(knob.get());
        
        if (isInt) {
            kf.setValue(isInt->getValue(i));
        } else if (isBool) {
            kf.setValue(isBool->getValue(i));
        } else if (isDouble) {
            kf.setValue(isDouble->getValue(i));
        } else if (isString) {
            std::string v = isString->getValue(i);
            double dv;
            isString->stringToKeyFrameValue(time, v, &dv);
            kf.setValue(dv);
        }
        

        toRemove.push_back(std::make_pair(curve,kf));
    }
    pushUndoCommand(new RemoveKeysCommand(getGui()->getCurveEditor()->getCurveWidget(),
                                          toRemove));
}

int KnobGui::getKnobsCountOnSameLine() const {
    return _imp->knobsOnSameLine.size();
}

void KnobGui::hide(){
    _hide();
    if(_imp->animationButton)
        _imp->animationButton->hide();
    //also  hide the curve from the curve editor if there's any
    if(getKnob()->getHolder()->getApp()){
       getGui()->getCurveEditor()->hideCurves(this);
    }
    
    ////In order to remove the row of the layout we have to make sure ALL the knobs on the row
    ////are hidden.
    bool shouldRemoveWidget = true;
    for (U32 i = 0; i < _imp->knobsOnSameLine.size(); ++i) {
        if (!_imp->knobsOnSameLine[i]->getIsSecret()) {
            shouldRemoveWidget = false;
        }
    }
    
    if (shouldRemoveWidget) {
        _imp->containerLayout->removeWidget(_imp->field);
        if (_imp->descriptionLabel) {
            _imp->containerLayout->removeWidget(_imp->descriptionLabel);
            _imp->descriptionLabel->setParent(0);
        }
        _imp->field->setParent(0);
        _imp->field->hide();
    }
    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->hide();
    }
}
void KnobGui::show(int index){
    _show();
    if(_imp->animationButton)
        _imp->animationButton->show();
    //also show the curve from the curve editor if there's any
    if(getKnob()->getHolder()->getApp()){
        getGui()->getCurveEditor()->showCurves(this);
    }
    
    if (_imp->isOnNewLine) {
        QLayoutItem* item = _imp->containerLayout->itemAt(_imp->row, QFormLayout::FieldRole);
        if ((item && item->widget() != _imp->field) || !item) {
            int indexToUse = index != -1 ? index : _imp->row;
            _imp->containerLayout->insertRow(indexToUse, _imp->descriptionLabel, _imp->field);
        }
        _imp->field->setParent(_imp->containerLayout->parentWidget());
        if (_imp->descriptionLabel) {
            _imp->descriptionLabel->setParent(_imp->containerLayout->parentWidget());
        }
        _imp->field->show();
    }
    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->show();
    }

}

int KnobGui::getActualIndexInLayout() const {
    for (int i = 0; i < _imp->containerLayout->rowCount(); ++i) {
        QLayoutItem* item = _imp->containerLayout->itemAt(i, QFormLayout::FieldRole);
        if (item && item->widget() == _imp->field) {
            return i;
        }
    }
    return -1;
}

bool KnobGui::isOnNewLine() const {
    return _imp->isOnNewLine;
}

void KnobGui::setEnabledSlot(){
    setEnabled();
    boost::shared_ptr<KnobI> knob = getKnob();
    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->setEnabled(knob->isEnabled(0));
    }
    if(knob->getHolder()->getApp()){
        for (int i = 0; i < knob->getDimension(); ++i) {
            if(!knob->isEnabled(i)){
                getGui()->getCurveEditor()->hideCurve(this,i);
            }else{
                getGui()->getCurveEditor()->showCurve(this,i);
            }
        }
    }
}

QWidget* KnobGui::getFieldContainer() const {
    return _imp->field;
}

void KnobGui::onInternalValueChanged(int dimension) {
    if(_imp->widgetCreated){
        updateGUI(dimension);
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
    copyToClipBoard(dimension,false);
}

void KnobGui::copyToClipBoard(int dimension,bool copyAnimation)
{
    std::list<Variant> values;
    std::list<boost::shared_ptr<Curve> > curves;
    std::list<boost::shared_ptr<Curve> > parametricCurves;
    std::map<int,std::string> stringAnimation;
    
    boost::shared_ptr<KnobI> knob = getKnob();
    Knob<int>* isInt = dynamic_cast<Knob<int>*>(knob.get());
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>(knob.get());
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>(knob.get());
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(knob.get());
    AnimatingString_KnobHelper* isAnimatingString = dynamic_cast<AnimatingString_KnobHelper*>(knob.get());
    boost::shared_ptr<Parametric_Knob> isParametric = boost::dynamic_pointer_cast<Parametric_Knob>(knob);
    
    
    for (int i = 0; i < knob->getDimension(); ++i) {
        if (isInt) {
            values.push_back(Variant(isInt->getValue(i)));
        } else if (isBool) {
            values.push_back(Variant(isBool->getValue(i)));
        } else if (isDouble) {
            values.push_back(Variant(isDouble->getValue(i)));
        } else if (isString) {
            values.push_back(Variant(isString->getValue(i).c_str()));
        }
        if (copyAnimation) {
            boost::shared_ptr<Curve> c(new Curve);
            c->clone(*knob->getCurve(i));
            curves.push_back(c);
        }
    }
    
    if (isAnimatingString) {
        isAnimatingString->saveAnimation(&stringAnimation);
    }
    
    if (isParametric) {
        std::list< Curve > tmpCurves;
        isParametric->saveParametricCurves(&tmpCurves);
        for (std::list< Curve >::iterator it = tmpCurves.begin(); it!=tmpCurves.end(); ++it) {
            boost::shared_ptr<Curve> c(new Curve);
            c->clone(*it);
            parametricCurves.push_back(c);
        }
    }
    
    appPTR->setKnobClipBoard(copyAnimation,dimension,values,curves,stringAnimation,parametricCurves);
}

void KnobGui::onCopyAnimationActionTriggered(){
    copyToClipBoard(-1, true);
}

void KnobGui::pasteClipBoard(int targetDimension)
{
    if (appPTR->isClipBoardEmpty()) {
        return;
    }
    
    std::list<Variant> values;
    std::list<boost::shared_ptr<Curve> > curves;
    std::list<boost::shared_ptr<Curve> > parametricCurves;
    std::map<int,std::string> stringAnimation;
    
    bool copyAnimation;
    int dimension;
    boost::shared_ptr<KnobI> knob = getKnob();
    Knob<int>* isInt = dynamic_cast<Knob<int>*>(knob.get());
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>(knob.get());
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>(knob.get());
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>(knob.get());
    AnimatingString_KnobHelper* isAnimatingString = dynamic_cast<AnimatingString_KnobHelper*>(knob.get());
    boost::shared_ptr<Parametric_Knob> isParametric = boost::dynamic_pointer_cast<Parametric_Knob>(knob);
    appPTR->getKnobClipBoard(&copyAnimation,&dimension,&values,&curves,&stringAnimation,&parametricCurves);

    ///If we are interested in just copying a value but the dimension is it out of range, return
    if (!copyAnimation && targetDimension >= knob->getDimension()) {
        return;
    }
    
    knob->beginValueChange(Natron::PLUGIN_EDITED);
    
    bool stop = false;
    int i = 0;
    for (std::list<Variant>::iterator it = values.begin(); it!=values.end();++it) {
        if (i == dimension) {
            if (isInt) {
                if (!it->canConvert(QVariant::Int)) {
                    QString err = QString("Cannot paste values from a parameter of type %1 to a parameter of type Integer").arg(it->typeName());
                    Natron::errorDialog("Paste",err.toStdString());
                    stop = true;
                    break;
                }
                isInt->setValue(it->toInt(), i);
            } else if (isBool) {
                if (!it->canConvert(QVariant::Bool)) {
                    QString err = QString("Cannot paste values from a parameter of type %1 to a parameter of type Boolean").arg(it->typeName());
                    Natron::errorDialog("Paste",err.toStdString());
                    stop = true;
                    break;
                }
                isBool->setValue(it->toBool(), i);
            } else if (isDouble) {
                if (!it->canConvert(QVariant::Double)) {
                    QString err = QString("Cannot paste values from a parameter of type %1 to a parameter of type Double").arg(it->typeName());
                    Natron::errorDialog("Paste",err.toStdString());
                    stop = true;
                    break;
                }
                isDouble->setValue(it->toDouble(), i);
            } else if (isString) {
                if (!it->canConvert(QVariant::String)) {
                    QString err = QString("Cannot paste values from a parameter of type %1 to a parameter of type String").arg(it->typeName());
                    Natron::errorDialog("Paste",err.toStdString());
                    stop = true;
                    break;
                }
                isString->setValue(it->toString().toStdString(), i);
            }
        }
        
        ++i;
    }
    if (stop) {
        return;
    }
    
    if (copyAnimation && !curves.empty() && ((int)curves.size() != knob->getDimension())) {
        Natron::errorDialog("Paste animation", "You cannot copy/paste animation from/to parameters with different dimensions.");
    } else {
        i = 0;
        for (std::list<boost::shared_ptr<Curve> >::iterator it = curves.begin(); it!=curves.end(); ++it) {
            knob->getCurve(i)->clone(*(*it));
            ++i;
        }
        if (!curves.empty()) {
            emit keyFrameSet();
        }
    }
    
    
    if (isAnimatingString) {
        isAnimatingString->loadAnimation(stringAnimation);
    }
    
    if (isParametric) {
        std::list<Curve> tmpCurves;
        for(std::list<boost::shared_ptr<Curve> >::iterator it = parametricCurves.begin();it!=parametricCurves.end();++it) {
            Curve c;
            c.clone(*(*it));
            tmpCurves.push_back(c);
        }
        isParametric->loadParametricCurves(tmpCurves);
    }
    
    knob->endValueChange();
}

void KnobGui::onPasteAnimationActionTriggered() {
    pasteClipBoard(0);
}

void KnobGui::pasteValues(int dimension) {
    pasteClipBoard(dimension);
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
    
    std::vector<boost::shared_ptr<Natron::Node> > allActiveNodes;
    
    assert(from->getKnob()->getHolder()->getApp());
    
    from->getKnob()->getHolder()->getApp()->getActiveNodes(&allActiveNodes);
    for (U32 i = 0; i < allActiveNodes.size(); ++i) {
        const std::vector< boost::shared_ptr<KnobI> >& knobs = allActiveNodes[i]->getKnobs();
        
        for (U32 j = 0; j < knobs.size(); ++j) {
            if(!knobs[j]->getIsSecret() && knobs[j] != from->getKnob()) {
                if (from->getKnob()->isTypeCompatible(knobs[j])) {
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

std::pair<int,boost::shared_ptr<KnobI> > LinkToKnobDialog::getSelectedKnobs() const {
    QString str = _selectionCombo->itemText(_selectionCombo->activeIndex());
    std::map<QString,std::pair<int,boost::shared_ptr<KnobI > > >::const_iterator it = _allKnobs.find(str);
    if(it != _allKnobs.end()){
        return it->second;
    }else{
        return std::make_pair(-1,boost::shared_ptr<KnobI>());
    }
}

void KnobGui::onKnobSlavedChanged(int dimension,bool b) {
    checkAnimationLevel(dimension);
    if (b) {
        emit keyFrameRemoved();
    } else {
        emit keyFrameSet();
    }
    setReadOnly_(b, dimension);
}

void KnobGui::linkTo(int dimension) {
    LinkToKnobDialog dialog(this,_imp->copyRightClickMenu->parentWidget());
    
    if(dialog.exec()){
        boost::shared_ptr<KnobI> thisKnob = getKnob();
        std::pair<int,boost::shared_ptr<KnobI> > otherKnob = dialog.getSelectedKnobs();
        if(otherKnob.second){
            
            if (!thisKnob->isTypeCompatible(otherKnob.second)) {
                errorDialog("Knob Link", "Types incompatibles!");
                return;
            }
            
            assert(otherKnob.first < otherKnob.second->getDimension());
            
            std::pair<int,boost::shared_ptr<KnobI> > existingLink = thisKnob->getMaster(otherKnob.first);
            if(existingLink.second){
                std::string err("Cannot link ");
                err.append(thisKnob->getDescription());
                err.append(" \n because the knob is already linked to ");
                err.append(existingLink.second->getDescription());
                errorDialog("Knob Link", err);
                return;
            }
            
            thisKnob->onKnobSlavedTo(dimension, otherKnob.second,otherKnob.first);
            onKnobSlavedChanged(dimension, true);
            thisKnob->getHolder()->getApp()->triggerAutoSave();
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
    boost::shared_ptr<KnobI> thisKnob = getKnob();
    std::pair<int,boost::shared_ptr<KnobI> > other = thisKnob->getMaster(dimension);
    thisKnob->onKnobUnSlaved(dimension);
    onKnobSlavedChanged(dimension, false);
    getKnob()->getHolder()->getApp()->triggerAutoSave();
}

void KnobGui::onUnlinkActionTriggered() {
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        unlink(action->data().toInt());
    }
}

void KnobGui::checkAnimationLevel(int dimension)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    AnimationLevel level = Natron::NO_ANIMATION;
    if (knob->getHolder()->getApp()) {
        
        boost::shared_ptr<Curve> c = getKnob()->getCurve(dimension);
        SequenceTime time = getKnob()->getHolder()->getApp()->getTimeLine()->currentFrame();
        if (c->getKeyFramesCount() > 0) {
            KeyFrame kf;
            bool found = c->getKeyFrameWithTime(time, &kf);;
            if (found) {
                level = Natron::ON_KEYFRAME;
            } else {
                level = Natron::INTERPOLATED_VALUE;
            }
        } else {
            level = Natron::NO_ANIMATION;
        }
    }
    knob->setAnimationLevel(dimension,level);
    reflectAnimationLevel(dimension, level);
}


void KnobGui::onResetDefaultValuesActionTriggered() {
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        resetDefault(action->data().toInt());
    }
}

void KnobGui::resetDefault(int dimension) {
    ///this cannot be undone for now. it's kinda lots of effort to do it and frankly not much necessary.
    if (getKnob()->typeName() != Button_Knob::typeNameStatic()) {
        getKnob()->resetToDefaultValue(dimension);
    }
}


void KnobGui::setReadOnly_(bool readOnly,int dimension) {
    
    setReadOnly(readOnly, dimension);
    
    ///This code doesn't work since the knob dimensions are still enabled even if readonly
//    bool hasDimensionEnabled = false;
//    for (int i = 0; i < getKnob()->getDimension(); ++i) {
//        if (getKnob()->isEnabled(i)) {
//            hasDimensionEnabled = true;
//        }
//    }
//    _descriptionLabel->setEnabled(hasDimensionEnabled);
}

bool KnobGui::triggerNewLine() const { return _imp->triggerNewLine; }

void KnobGui::turnOffNewLine() { _imp->triggerNewLine = false; }

/*Set the spacing between items in the layout*/
void KnobGui::setSpacingBetweenItems(int spacing){ _imp->spacingBetweenItems = spacing; }

int KnobGui::getSpacingBetweenItems() const { return _imp->spacingBetweenItems; }

bool KnobGui::hasWidgetBeenCreated() const {return _imp->widgetCreated;}
