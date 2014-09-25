//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
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
#include <QTimer>

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
#include <QDialogButtonBox>
#include <QCompleter>

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
#include "Engine/Variant.h"

#include "Gui/AnimationButton.h"
#include "Gui/DockablePanel.h"
#include "Gui/ViewerTab.h"
#include "Gui/TimeLineGui.h"
#include "Gui/MenuWithToolTips.h"
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
#include "Gui/CustomParamInteract.h"
#include "Gui/NodeCreationDialog.h"

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
    CustomParamInteract* customInteract;

    KnobGuiPrivate(DockablePanel* container)
        : triggerNewLine(true)
          , spacingBetweenItems(0)
          , widgetCreated(false)
          , container(container)
          , animationMenu(NULL)
          , animationButton(NULL)
          , copyRightClickMenu( new MenuWithToolTips(container) )
          , fieldLayout(NULL)
          , row(-1)
          , knobsOnSameLine()
          , containerLayout(NULL)
          , field(NULL)
          , descriptionLabel(NULL)
          , isOnNewLine(false)
          , customInteract(NULL)
    {
        copyRightClickMenu->setFont( QFont(NATRON_FONT, NATRON_FONT_SIZE_11) );
    }
};

/////////////// KnobGui
KnobGui::KnobGui(boost::shared_ptr<KnobI> knob,
                 DockablePanel* container)
    : _imp( new KnobGuiPrivate(container) )
{
    knob->setKnobGuiPointer(this);
    KnobHelper* helper = dynamic_cast<KnobHelper*>( knob.get() );
    KnobSignalSlotHandler* handler = helper->getSignalSlotHandler().get();
    QObject::connect( handler,SIGNAL( valueChanged(int,int) ),this,SLOT( onInternalValueChanged(int,int) ) );
    QObject::connect( handler,SIGNAL( keyFrameSet(SequenceTime,int,bool) ),this,SLOT( onInternalKeySet(SequenceTime,int,bool) ) );
    QObject::connect( this,SIGNAL( keyFrameSetByUser(SequenceTime,int) ),handler,SLOT( onKeyFrameSet(SequenceTime,int) ) );
    QObject::connect( handler,SIGNAL( keyFrameRemoved(SequenceTime,int) ),this,SLOT( onInternalKeyRemoved(SequenceTime,int) ) );
    QObject::connect( this,SIGNAL( keyFrameRemovedByUser(SequenceTime,int) ),handler,SLOT( onKeyFrameRemoved(SequenceTime,int) ) );
    QObject::connect( handler,SIGNAL( secretChanged() ),this,SLOT( setSecret() ) );
    QObject::connect( handler,SIGNAL( enabledChanged() ),this,SLOT( setEnabledSlot() ) );
    QObject::connect( handler,SIGNAL( knobSlaved(int,bool) ),this,SLOT( onKnobSlavedChanged(int,bool) ) );
    QObject::connect( handler,SIGNAL( animationAboutToBeRemoved(int) ),this,SLOT( onInternalAnimationAboutToBeRemoved() ) );
    QObject::connect( handler,SIGNAL( animationRemoved(int) ),this,SLOT( onInternalAnimationRemoved() ) );
    QObject::connect( handler,SIGNAL( setValueWithUndoStack(Variant,int) ),this,SLOT( onSetValueUsingUndoStack(Variant,int) ) );
    QObject::connect( handler,SIGNAL( dirty(bool) ),this,SLOT( onSetDirty(bool) ) );
    QObject::connect( handler,SIGNAL( animationLevelChanged(int,int) ),this,SLOT( onAnimationLevelChanged(int,int) ) );
    QObject::connect( handler,SIGNAL( appendParamEditChange(Variant,int,int,bool,bool) ),this,
                      SLOT( onAppendParamEditChanged(Variant,int,int,bool,bool) ) );
    QObject::connect( handler,SIGNAL( frozenChanged(bool) ),this,SLOT( onFrozenChanged(bool) ) );
}

KnobGui::~KnobGui()
{
    delete _imp->animationButton;
    delete _imp->animationMenu;
}

Gui*
KnobGui::getGui() const
{
    return _imp->container->getGui();
}

const QUndoCommand*
KnobGui::getLastUndoCommand() const
{
    return _imp->container->getLastUndoCommand();
}

void
KnobGui::pushUndoCommand(QUndoCommand* cmd)
{
    if ( getKnob()->getCanUndo() && getKnob()->getEvaluateOnChange() ) {
        _imp->container->pushUndoCommand(cmd);
    } else {
        cmd->redo();
    }
}

void
KnobGui::createGUI(QFormLayout* containerLayout,
                   QWidget* fieldContainer,
                   QWidget* label,
                   QHBoxLayout* layout,
                   int row,
                   bool isOnNewLine,
                   const std::vector< boost::shared_ptr< KnobI > > & knobsOnSameLine)
{
    boost::shared_ptr<KnobI> knob = getKnob();

    _imp->containerLayout = containerLayout;
    _imp->fieldLayout = layout;
    _imp->row = row;
    _imp->knobsOnSameLine = knobsOnSameLine;
    _imp->field = fieldContainer;
    _imp->descriptionLabel = label;
    _imp->isOnNewLine = isOnNewLine;
    if (!isOnNewLine) {
        layout->addSpacing(15);
        layout->addWidget(label);
    }

    label->setToolTip( toolTip() );

    boost::shared_ptr<OfxParamOverlayInteract> customInteract = knob->getCustomInteract();
    if (customInteract != 0) {
        _imp->customInteract = new CustomParamInteract(this,knob->getOfxParamHandle(),customInteract);
        layout->addWidget(_imp->customInteract);
    } else {
        createWidget(layout);
    }
    if ( knob->isAnimationEnabled() ) {
        createAnimationButton(layout);
    }
    _imp->widgetCreated = true;

    for (int i = 0; i < knob->getDimension(); ++i) {
        updateGuiInternal(i);
        onAnimationLevelChanged(i, knob->getAnimationLevel(i) );
    }
    

    setEnabledSlot();
    if (isOnNewLine) {
        containerLayout->addRow(label, fieldContainer);
    }
    setSecret();
}

void
KnobGui::updateGuiInternal(int dimension)
{
    if (!_imp->customInteract) {
        updateGUI(dimension);
    } else {
        _imp->customInteract->update();
    }
}

void
KnobGui::createAnimationButton(QHBoxLayout* layout)
{
    _imp->animationMenu = new QMenu( layout->parentWidget() );
    _imp->animationMenu->setFont( QFont(NATRON_FONT, NATRON_FONT_SIZE_11) );
    QPixmap pix;
    appPTR->getIcon(Natron::NATRON_PIXMAP_CURVE, &pix);
    _imp->animationButton = new AnimationButton( this,QIcon(pix),"",layout->parentWidget() );
    _imp->animationButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->animationButton->setToolTip( Qt::convertFromPlainText(tr("Animation menu"), Qt::WhiteSpaceNormal) );
    QObject::connect( _imp->animationButton,SIGNAL( animationMenuRequested() ),this,SLOT( showAnimationMenu() ) );
    layout->addWidget(_imp->animationButton);

    if ( getKnob()->getIsSecret() ) {
        _imp->animationButton->hide();
    }
}

void
KnobGui::onRightClickClicked(const QPoint & pos)
{
    QWidget *widget = qobject_cast<QWidget *>( sender() );

    if (widget) {
        QString objName = widget->objectName();
        objName = objName.remove("dim-");
        showRightClickMenuForDimension( pos, objName.toInt() );
    }
}

void
KnobGui::enableRightClickMenu(QWidget* widget,
                              int dimension)
{
    QString name("dim-");

    name.append( QString::number(dimension) );
    widget->setContextMenuPolicy(Qt::CustomContextMenu);
    widget->setObjectName(name);
    QObject::connect( widget,SIGNAL( customContextMenuRequested(QPoint) ),this,SLOT( onRightClickClicked(QPoint) ) );
}

void
KnobGui::showRightClickMenuForDimension(const QPoint &,
                                        int dimension)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    bool enabled = knob->isEnabled(dimension);

    if ( knob->getIsSecret() ) {
        return;
    }

    createAnimationMenu(_imp->copyRightClickMenu);

    _imp->copyRightClickMenu->addSeparator();

    bool isSlave = knob->isSlave(dimension);
    QAction* copyValuesAction = new QAction(tr("Copy value"),_imp->copyRightClickMenu);
    copyValuesAction->setData( QVariant(dimension) );
    QObject::connect( copyValuesAction,SIGNAL( triggered() ),this,SLOT( onCopyValuesActionTriggered() ) );
    _imp->copyRightClickMenu->addAction(copyValuesAction);

    if (!isSlave) {
        bool isClipBoardEmpty = appPTR->isClipBoardEmpty();
        QAction* pasteAction = new QAction(tr("Paste value"),_imp->copyRightClickMenu);
        pasteAction->setData( QVariant(dimension) );
        QObject::connect( pasteAction,SIGNAL( triggered() ),this,SLOT( onPasteValuesActionTriggered() ) );
        _imp->copyRightClickMenu->addAction(pasteAction);
        if (isClipBoardEmpty || !enabled) {
            pasteAction->setEnabled(false);
        }
    }

    QAction* resetDefaultAction = new QAction(tr("Reset to default"),_imp->copyRightClickMenu);
    resetDefaultAction->setData( QVariant(dimension) );
    QObject::connect( resetDefaultAction,SIGNAL( triggered() ),this,SLOT( onResetDefaultValuesActionTriggered() ) );
    _imp->copyRightClickMenu->addAction(resetDefaultAction);
    if (isSlave || !enabled) {
        resetDefaultAction->setEnabled(false);
    }

    if (!isSlave && enabled) {
        QAction* linkToAction = new QAction(tr("Link to"),_imp->copyRightClickMenu);
        linkToAction->setData( QVariant(dimension) );
        QObject::connect( linkToAction,SIGNAL( triggered() ),this,SLOT( onLinkToActionTriggered() ) );
        _imp->copyRightClickMenu->addAction(linkToAction);
    } else if (isSlave) {
        QAction* unlinkAction = new QAction(tr("Unlink"),_imp->copyRightClickMenu);
        unlinkAction->setData( QVariant(dimension) );
        QObject::connect( unlinkAction,SIGNAL( triggered() ),this,SLOT( onUnlinkActionTriggered() ) );
        _imp->copyRightClickMenu->addAction(unlinkAction);


        ///a stub action just to indicate what is the master knob.
        QAction* masterNameAction = new QAction("",_imp->copyRightClickMenu);
        std::pair<int,boost::shared_ptr<KnobI> > master = knob->getMaster(dimension);
        assert(master.second);

        ///find-out to which node that master knob belongs to
        std::string nodeName("Linked to: ");

        assert( getKnob()->getHolder()->getApp() );
        const std::vector<boost::shared_ptr<Natron::Node> > allNodes = knob->getHolder()->getApp()->getProject()->getCurrentNodes();
        for (U32 i = 0; i < allNodes.size(); ++i) {
            const std::vector< boost::shared_ptr<KnobI> > & knobs = allNodes[i]->getKnobs();
            bool shouldStop = false;
            for (U32 j = 0; j < knobs.size(); ++j) {
                if ( knobs[j].get() == master.second.get() ) {
                    nodeName.append( allNodes[i]->getName() );
                    shouldStop = true;
                    break;
                }
            }
            if (shouldStop) {
                break;
            }
        }
        nodeName.append(".");
        nodeName.append( master.second->getDescription() );
        if (master.second->getDimension() > 1) {
            nodeName.append(".");
            nodeName.append( master.second->getDimensionName(master.first) );
        }
        masterNameAction->setText( nodeName.c_str() );
        masterNameAction->setEnabled(false);
        _imp->copyRightClickMenu->addAction(masterNameAction);
    }

    addRightClickMenuEntries(_imp->copyRightClickMenu);
    _imp->copyRightClickMenu->exec( QCursor::pos() );
} // showRightClickMenuForDimension

void
KnobGui::createAnimationMenu(QMenu* menu)
{
    boost::shared_ptr<KnobI> knob = getKnob();

    menu->clear();
    bool isOnKeyFrame = false;
    for (int i = 0; i < knob->getDimension(); ++i) {
        if (knob->getAnimationLevel(i) == Natron::ON_KEYFRAME) {
            isOnKeyFrame = true;
            break;
        }
    }

    bool isSlave = false;
    bool hasAnimation = false;
    bool isEnabled = true;
    for (int i = 0; i < knob->getDimension(); ++i) {
        if ( knob->isSlave(i) ) {
            isSlave = true;
        }
        if (knob->getKeyFramesCount(i) > 0) {
            hasAnimation = true;
        }
        if (isSlave && hasAnimation) {
            break;
        }
        if ( !knob->isEnabled(i) ) {
            isEnabled = false;
        }
    }
    if ( knob->isAnimationEnabled() ) {
        if (!isSlave) {
            if (!isOnKeyFrame) {
                QAction* setKeyAction = new QAction(tr("Set Key"),menu);
                QObject::connect( setKeyAction,SIGNAL( triggered() ),this,SLOT( onSetKeyActionTriggered() ) );
                menu->addAction(setKeyAction);
                if (!isEnabled) {
                    setKeyAction->setEnabled(false);
                }
            } else {
                QAction* removeKeyAction = new QAction(tr("Remove Key"),menu);
                QObject::connect( removeKeyAction,SIGNAL( triggered() ),this,SLOT( onRemoveKeyActionTriggered() ) );
                menu->addAction(removeKeyAction);
                if (!isEnabled) {
                    removeKeyAction->setEnabled(false);
                }
            }

            QAction* removeAnyAnimationAction = new QAction(tr("Remove animation"),menu);
            QObject::connect( removeAnyAnimationAction,SIGNAL( triggered() ),this,SLOT( onRemoveAnyAnimationActionTriggered() ) );
            menu->addAction(removeAnyAnimationAction);
            if (!hasAnimation || !isEnabled) {
                removeAnyAnimationAction->setEnabled(false);
            }
        }


        if (!isSlave) {
            QAction* showInCurveEditorAction = new QAction(tr("Show in curve editor"),menu);
            QObject::connect( showInCurveEditorAction,SIGNAL( triggered() ),this,SLOT( onShowInCurveEditorActionTriggered() ) );
            menu->addAction(showInCurveEditorAction);
            if (!hasAnimation || !isEnabled) {
                showInCurveEditorAction->setEnabled(false);
            }

            QMenu* interpolationMenu = new QMenu(menu);
            interpolationMenu->setFont( QFont(NATRON_FONT, NATRON_FONT_SIZE_11) );
            interpolationMenu->setTitle("Interpolation");
            menu->addAction( interpolationMenu->menuAction() );
            if (!isEnabled) {
                interpolationMenu->menuAction()->setEnabled(false);
            }

            QAction* constantInterpAction = new QAction(tr("Constant"),interpolationMenu);
            QObject::connect( constantInterpAction,SIGNAL( triggered() ),this,SLOT( onConstantInterpActionTriggered() ) );
            interpolationMenu->addAction(constantInterpAction);

            QAction* linearInterpAction = new QAction(tr("Linear"),interpolationMenu);
            QObject::connect( linearInterpAction,SIGNAL( triggered() ),this,SLOT( onLinearInterpActionTriggered() ) );
            interpolationMenu->addAction(linearInterpAction);

            QAction* smoothInterpAction = new QAction(tr("Smooth"),interpolationMenu);
            QObject::connect( smoothInterpAction,SIGNAL( triggered() ),this,SLOT( onSmoothInterpActionTriggered() ) );
            interpolationMenu->addAction(smoothInterpAction);

            QAction* catmullRomInterpAction = new QAction(tr("Catmull-Rom"),interpolationMenu);
            QObject::connect( catmullRomInterpAction,SIGNAL( triggered() ),this,SLOT( onCatmullromInterpActionTriggered() ) );
            interpolationMenu->addAction(catmullRomInterpAction);

            QAction* cubicInterpAction = new QAction(tr("Cubic"),interpolationMenu);
            QObject::connect( cubicInterpAction,SIGNAL( triggered() ),this,SLOT( onCubicInterpActionTriggered() ) );
            interpolationMenu->addAction(cubicInterpAction);

            QAction* horizInterpAction = new QAction(tr("Horizontal"),interpolationMenu);
            QObject::connect( horizInterpAction,SIGNAL( triggered() ),this,SLOT( onHorizontalInterpActionTriggered() ) );
            interpolationMenu->addAction(horizInterpAction);
        }

        QAction* copyAnimationAction = new QAction(tr("Copy animation"),menu);
        QObject::connect( copyAnimationAction,SIGNAL( triggered() ),this,SLOT( onCopyAnimationActionTriggered() ) );
        menu->addAction(copyAnimationAction);
        if (!hasAnimation) {
            copyAnimationAction->setEnabled(false);
        }

        if (!isSlave) {
            ///If the clipboard is either empty or has no animation, disable the Paste animation action.
            bool isClipBoardEmpty = appPTR->isClipBoardEmpty();
            std::list<Variant> values;
            std::list<boost::shared_ptr<Curve> > curves;
            std::list<boost::shared_ptr<Curve> > parametricCurves;
            std::map<int,std::string> stringAnimation;
            bool copyAnimation;

            appPTR->getKnobClipBoard(&copyAnimation,&values,&curves,&stringAnimation,&parametricCurves);

            QAction* pasteAction = new QAction(tr("Paste animation"),menu);
            QObject::connect( pasteAction,SIGNAL( triggered() ),this,SLOT( onPasteAnimationActionTriggered() ) );
            menu->addAction(pasteAction);
            if (!copyAnimation || isClipBoardEmpty || !isEnabled) {
                pasteAction->setEnabled(false);
            }
        }
    }
} // createAnimationMenu

void
KnobGui::setSecret()
{
    // If the Knob is within a group, only show it if the group is unfolded!
    // To test it:
    // try TuttlePinning: fold all groups, then switch from perspective to affine to perspective.
    //  VISIBILITY is different from SECRETNESS. The code considers that both things are equivalent, which is wrong.
    // Of course, this check has to be *recursive* (in case the group is within a folded group)
    boost::shared_ptr<KnobI> knob = getKnob();
    bool showit = !knob->getIsSecret();
    boost::shared_ptr<KnobI> parentKnob = knob->getParentKnob();

    while (showit && parentKnob && parentKnob->typeName() == "Group") {
        Group_KnobGui* parentGui = dynamic_cast<Group_KnobGui*>( _imp->container->getKnobGui(parentKnob) );
        assert(parentGui);
        // check for secretness and visibility of the group
        if ( parentKnob->getIsSecret() || ( parentGui && !parentGui->isChecked() ) ) {
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

void
KnobGui::showAnimationMenu()
{
    createAnimationMenu(_imp->animationMenu);
    _imp->animationMenu->exec( _imp->animationButton->mapToGlobal( QPoint(0,0) ) );
}

void
KnobGui::onShowInCurveEditorActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();

    assert( knob->getHolder()->getApp() );
    getGui()->setCurveEditorOnTop();
    std::vector<boost::shared_ptr<Curve> > curves;
    for (int i = 0; i < knob->getDimension(); ++i) {
        boost::shared_ptr<Curve> c = knob->getCurve(i);
        if ( c->isAnimated() ) {
            curves.push_back(c);
        }
    }
    if ( !curves.empty() ) {
        getGui()->getCurveEditor()->centerOn(curves);
    }
}

void
KnobGui::onRemoveAnyAnimationActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<std::pair<CurveGui *, KeyFrame > > toRemove;

    for (int i = 0; i < knob->getDimension(); ++i) {
        CurveGui* curve = getGui()->getCurveEditor()->findCurve(this, i);
        KeyFrameSet keys = curve->getInternalCurve()->getKeyFrames_mt_safe();
        for (KeyFrameSet::const_iterator it = keys.begin(); it != keys.end(); ++it) {
            toRemove.push_back( std::make_pair(curve,*it) );
        }
    }
    pushUndoCommand( new RemoveKeysCommand(getGui()->getCurveEditor()->getCurveWidget(),
                                           toRemove) );
    //refresh the gui so it doesn't indicate the parameter is animated anymore
    for (int i = 0; i < knob->getDimension(); ++i) {
        updateGUI(i);
    }
}

void
KnobGui::setInterpolationForDimensions(const std::vector<int> & dimensions,
                                       Natron::KeyframeType interp)
{
    boost::shared_ptr<KnobI> knob = getKnob();

    for (U32 i = 0; i < dimensions.size(); ++i) {
        boost::shared_ptr<Curve> c = knob->getCurve(dimensions[i]);
        int kfCount = c->getKeyFramesCount();
        for (int j = 0; j < kfCount; ++j) {
            c->setKeyFrameInterpolation(interp, j);
        }
    }
    emit keyInterpolationChanged();
}

void
KnobGui::onConstantInterpActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;

    for (int i = 0; i < knob->getDimension(); ++i) {
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims,Natron::KEYFRAME_CONSTANT);
}

void
KnobGui::onLinearInterpActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;

    for (int i = 0; i < knob->getDimension(); ++i) {
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims,Natron::KEYFRAME_LINEAR);
}

void
KnobGui::onSmoothInterpActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;

    for (int i = 0; i < knob->getDimension(); ++i) {
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims,Natron::KEYFRAME_SMOOTH);
}

void
KnobGui::onCatmullromInterpActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;

    for (int i = 0; i < knob->getDimension(); ++i) {
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims,Natron::KEYFRAME_CATMULL_ROM);
}

void
KnobGui::onCubicInterpActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;

    for (int i = 0; i < knob->getDimension(); ++i) {
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims,Natron::KEYFRAME_CUBIC);
}

void
KnobGui::onHorizontalInterpActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;

    for (int i = 0; i < knob->getDimension(); ++i) {
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims,Natron::KEYFRAME_HORIZONTAL);
}

void
KnobGui::setKeyframe(double time,
                     int dimension)
{
    boost::shared_ptr<KnobI> knob = getKnob();

    assert( knob->getHolder()->getApp() );
    emit keyFrameSetByUser(time,dimension);
    emit keyFrameSet();
    if ( !knob->getIsSecret() ) {
        knob->getHolder()->getApp()->getTimeLine()->addKeyframeIndicator(time);
    }
}

void
KnobGui::onSetKeyActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();

    assert( knob->getHolder()->getApp() );
    //get the current time on the global timeline
    SequenceTime time = knob->getHolder()->getApp()->getTimeLine()->currentFrame();

    for (int i = 0; i < knob->getDimension(); ++i) {
        CurveGui* curve = getGui()->getCurveEditor()->findCurve(this, i);
        if (!curve) {
            return;
        }
        std::vector<KeyFrame> kVec;
        KeyFrame kf;
        kf.setTime(time);
        Knob<int>* isInt = dynamic_cast<Knob<int>*>( knob.get() );
        Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( knob.get() );
        AnimatingString_KnobHelper* isString = dynamic_cast<AnimatingString_KnobHelper*>( knob.get() );
        Knob<double>* isDouble = dynamic_cast<Knob<double>*>( knob.get() );

        if (isInt) {
            kf.setValue( isInt->getValue(i) );
        } else if (isBool) {
            kf.setValue( isBool->getValue(i) );
        } else if (isDouble) {
            kf.setValue( isDouble->getValue(i) );
        } else if (isString) {
            std::string v = isString->getValue(i);
            double dv;
            isString->stringToKeyFrameValue(time, v, &dv);
            kf.setValue(dv);
        }

        kVec.push_back(kf);
        pushUndoCommand( new AddKeysCommand(getGui()->getCurveEditor()->getCurveWidget(),
                                            curve,kVec) );
    }
}

void
KnobGui::removeKeyFrame(double time,
                        int dimension)
{
    emit keyFrameRemovedByUser(time,dimension);
    emit keyFrameRemoved();
    boost::shared_ptr<KnobI> knob = getKnob();

    assert( knob->getHolder()->getApp() );
    if ( !knob->getIsSecret() ) {
        knob->getHolder()->getApp()->getTimeLine()->removeKeyFrameIndicator(time);
    }
    updateGUI(dimension);
}

QString
KnobGui::getScriptNameHtml() const
{
    return  QString("<font size = 4><b>%1</b></font>").arg( getKnob()->getName().c_str() );
}

QString
KnobGui::toolTip() const
{
    QString tt = getScriptNameHtml();
    QString realTt( getKnob()->getHintToolTip().c_str() );

    if ( !realTt.isEmpty() ) {
        realTt = Qt::convertFromPlainText(realTt,Qt::WhiteSpaceNormal);
        tt.append(realTt);
    }

    return tt;
}

bool
KnobGui::hasToolTip() const
{
    //Always true now that we display the script name in the tooltip
    return true; //!getKnob()->getHintToolTip().empty();
}

void
KnobGui::onRemoveKeyActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();

    assert( knob->getHolder()->getApp() );
    //get the current time on the global timeline
    SequenceTime time = knob->getHolder()->getApp()->getTimeLine()->currentFrame();
    std::vector<std::pair<CurveGui*,KeyFrame> > toRemove;
    for (int i = 0; i < knob->getDimension(); ++i) {
        CurveGui* curve = getGui()->getCurveEditor()->findCurve(this, i);
        
        KeyFrame kf;
        bool foundKey = knob->getCurve(i)->getKeyFrameWithTime(time, &kf);
        
        if (foundKey) {
            toRemove.push_back( std::make_pair(curve,kf) );
        }
    }
    pushUndoCommand( new RemoveKeysCommand(getGui()->getCurveEditor()->getCurveWidget(),
                                           toRemove) );
}

int
KnobGui::getKnobsCountOnSameLine() const
{
    return _imp->knobsOnSameLine.size();
}

void
KnobGui::hide()
{
    if (!_imp->customInteract) {
        _hide();
    } else {
        _imp->customInteract->hide();
    }
    if (_imp->animationButton) {
        _imp->animationButton->hide();
    }
    //also  hide the curve from the curve editor if there's any
    if ( getKnob()->getHolder()->getApp() ) {
        getGui()->getCurveEditor()->hideCurves(this);
    }

    ////In order to remove the row of the layout we have to make sure ALL the knobs on the row
    ////are hidden.
    bool shouldRemoveWidget = true;
    for (U32 i = 0; i < _imp->knobsOnSameLine.size(); ++i) {
        if ( !_imp->knobsOnSameLine[i]->getIsSecret() ) {
            shouldRemoveWidget = false;
        }
    }

    if (shouldRemoveWidget) {
        _imp->containerLayout->removeWidget(_imp->field);
        _imp->field->setParent(0);
        _imp->field->hide();
    }
    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->hide();
    }
}

void
KnobGui::show(int index)
{
    if (!_imp->customInteract) {
        _show();
    } else {
        _imp->customInteract->show();
    }
    if (_imp->animationButton) {
        _imp->animationButton->show();
    }
    //also show the curve from the curve editor if there's any
    if ( getKnob()->getHolder()->getApp() ) {
        getGui()->getCurveEditor()->showCurves(this);
    }

    if (_imp->isOnNewLine) {
        QLayoutItem* item = _imp->containerLayout->itemAt(_imp->row, QFormLayout::FieldRole);
        if ( ( item && (item->widget() != _imp->field) ) || !item ) {
            int indexToUse = index != -1 ? index : _imp->row;
            _imp->containerLayout->setWidget(indexToUse, QFormLayout::LabelRole,_imp->descriptionLabel);
            _imp->containerLayout->setWidget(indexToUse, QFormLayout::FieldRole, _imp->field);
        }
        _imp->field->setParent( _imp->containerLayout->parentWidget() );
        _imp->field->show();

        if (_imp->descriptionLabel) {
            _imp->descriptionLabel->setParent( _imp->containerLayout->parentWidget() );
            _imp->descriptionLabel->show();
        }
    } else {
        if (_imp->descriptionLabel) {
            _imp->descriptionLabel->show();
        }
    }
}

int
KnobGui::getActualIndexInLayout() const
{
    for (int i = 0; i < _imp->containerLayout->rowCount(); ++i) {
        QLayoutItem* item = _imp->containerLayout->itemAt(i, QFormLayout::FieldRole);
        if ( item && (item->widget() == _imp->field) ) {
            return i;
        }
    }

    return -1;
}

bool
KnobGui::isOnNewLine() const
{
    return _imp->isOnNewLine;
}

void
KnobGui::setEnabledSlot()
{
    if (!getGui()) {
        return;
    }
    if (!_imp->customInteract) {
        setEnabled();
    }
    boost::shared_ptr<KnobI> knob = getKnob();
    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->setEnabled( knob->isEnabled(0) );
    }
    if ( knob->getHolder()->getApp() ) {
        for (int i = 0; i < knob->getDimension(); ++i) {
            if ( !knob->isEnabled(i) ) {
                getGui()->getCurveEditor()->hideCurve(this,i);
            } else {
                getGui()->getCurveEditor()->showCurve(this,i);
            }
        }
    }
}

QWidget*
KnobGui::getFieldContainer() const
{
    return _imp->field;
}

void
KnobGui::onInternalValueChanged(int dimension,
                                int /*reason*/)
{
    if (_imp->widgetCreated) {
        updateGuiInternal(dimension);
    }
}

void
KnobGui::updateCurveEditorKeyframes()
{
    emit keyFrameSet();
}

void
KnobGui::onInternalKeySet(SequenceTime time,
                          int,
                          bool added )
{
    if (added) {
        boost::shared_ptr<KnobI> knob = getKnob();
        if ( !knob->getIsSecret() ) {
            knob->getHolder()->getApp()->getTimeLine()->addKeyframeIndicator(time);
        }
    }

    updateCurveEditorKeyframes();
}

void
KnobGui::onInternalKeyRemoved(SequenceTime time,
                              int)
{
    boost::shared_ptr<KnobI> knob = getKnob();

    knob->getHolder()->getApp()->getTimeLine()->removeKeyFrameIndicator(time);
    emit keyFrameRemoved();
}

void
KnobGui::copyAnimationToClipboard() const
{
    copyToClipBoard(true);
}

void
KnobGui::onCopyValuesActionTriggered()
{
    copyValuesToCliboard();
}

void
KnobGui::copyValuesToCliboard()
{
    copyToClipBoard(false);
}

void
KnobGui::onCopyAnimationActionTriggered()
{
    copyAnimationToClipboard();
}

void
KnobGui::copyToClipBoard(bool copyAnimation) const
{
    std::list<Variant> values;
    std::list<boost::shared_ptr<Curve> > curves;
    std::list<boost::shared_ptr<Curve> > parametricCurves;
    std::map<int,std::string> stringAnimation;
    boost::shared_ptr<KnobI> knob = getKnob();

    Knob<int>* isInt = dynamic_cast<Knob<int>*>( knob.get() );
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( knob.get() );
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>( knob.get() );
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( knob.get() );
    AnimatingString_KnobHelper* isAnimatingString = dynamic_cast<AnimatingString_KnobHelper*>( knob.get() );
    boost::shared_ptr<Parametric_Knob> isParametric = boost::dynamic_pointer_cast<Parametric_Knob>(knob);


    for (int i = 0; i < knob->getDimension(); ++i) {
        if (isInt) {
            values.push_back( Variant( isInt->getValue(i) ) );
        } else if (isBool) {
            values.push_back( Variant( isBool->getValue(i) ) );
        } else if (isDouble) {
            values.push_back( Variant( isDouble->getValue(i) ) );
        } else if (isString) {
            values.push_back( Variant( isString->getValue(i).c_str() ) );
        }
        if (copyAnimation) {
            boost::shared_ptr<Curve> c(new Curve);
            c->clone( *knob->getCurve(i) );
            curves.push_back(c);
        }
    }

    if (isAnimatingString) {
        isAnimatingString->saveAnimation(&stringAnimation);
    }

    if (isParametric) {
        std::list< Curve > tmpCurves;
        isParametric->saveParametricCurves(&tmpCurves);
        for (std::list< Curve >::iterator it = tmpCurves.begin(); it != tmpCurves.end(); ++it) {
            boost::shared_ptr<Curve> c(new Curve);
            c->clone(*it);
            parametricCurves.push_back(c);
        }
    }

    appPTR->setKnobClipBoard(copyAnimation,values,curves,stringAnimation,parametricCurves);
}

void
KnobGui::pasteClipBoard()
{
    if ( appPTR->isClipBoardEmpty() ) {
        return;
    }

    std::list<Variant> values;
    std::list<boost::shared_ptr<Curve> > curves;
    std::list<boost::shared_ptr<Curve> > parametricCurves;
    std::map<int,std::string> stringAnimation;
    bool copyAnimation;

    appPTR->getKnobClipBoard(&copyAnimation,&values,&curves,&stringAnimation,&parametricCurves);

    boost::shared_ptr<KnobI> knob = getKnob();

    Knob<int>* isInt = dynamic_cast<Knob<int>*>( knob.get() );
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( knob.get() );
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>( knob.get() );
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( knob.get() );
    boost::shared_ptr<Parametric_Knob> isParametric = boost::dynamic_pointer_cast<Parametric_Knob>(knob);

    if ( copyAnimation && !curves.empty() && ( (int)curves.size() != knob->getDimension() ) ) {
        Natron::errorDialog( tr("Paste animation").toStdString(), tr("You cannot copy/paste animation from/to parameters with different dimensions.").toStdString() );

        return;
    }

    int i = 0;
    for (std::list<Variant>::iterator it = values.begin(); it != values.end(); ++it) {
        if (isInt) {
            if ( !it->canConvert(QVariant::Int) ) {
                QString err = tr("Cannot paste values from a parameter of type %1 to a parameter of type Integer").arg( it->typeName() );
                Natron::errorDialog( tr("Paste").toStdString(),err.toStdString() );

                return;
            }
        } else if (isBool) {
            if ( !it->canConvert(QVariant::Bool) ) {
                QString err = tr("Cannot paste values from a parameter of type %1 to a parameter of type Boolean").arg( it->typeName() );
                Natron::errorDialog( tr("Paste").toStdString(),err.toStdString() );

                return;
            }
        } else if (isDouble) {
            if ( !it->canConvert(QVariant::Double) ) {
                QString err = tr("Cannot paste values from a parameter of type %1 to a parameter of type Double").arg( it->typeName() );
                Natron::errorDialog( tr("Paste").toStdString(),err.toStdString() );

                return;
            }
        } else if (isString) {
            if ( !it->canConvert(QVariant::String) ) {
                QString err = tr("Cannot paste values from a parameter of type %1 to a parameter of type String").arg( it->typeName() );
                Natron::errorDialog( tr("Paste").toStdString(),err.toStdString() );

                return;
            }
        }

        ++i;
    }

    pushUndoCommand( new PasteUndoCommand(this,copyAnimation,values,curves,parametricCurves,stringAnimation) );
} // pasteClipBoard

void
KnobGui::onPasteAnimationActionTriggered()
{
    pasteClipBoard();
}

void
KnobGui::pasteValuesFromClipboard()
{
    pasteClipBoard();
}

void
KnobGui::onPasteValuesActionTriggered()
{
    pasteValuesFromClipboard();
}

struct LinkToKnobDialogPrivate
{
    KnobGui* fromKnob;
    QVBoxLayout* mainLayout;
    QHBoxLayout* firstLineLayout;
    QWidget* firstLine;
    QLabel* selectNodeLabel;
    CompleterLineEdit* nodeSelectionCombo;
    ComboBox* knobSelectionCombo;
    QDialogButtonBox* buttons;
    std::vector< boost::shared_ptr<Natron::Node> > allNodes;
    std::map<QString,boost::shared_ptr<KnobI > > allKnobs;

    LinkToKnobDialogPrivate(KnobGui* from)
        : fromKnob(from)
    {
    }
};

LinkToKnobDialog::LinkToKnobDialog(KnobGui* from,
                                   QWidget* parent)
    : QDialog(parent)
      , _imp( new LinkToKnobDialogPrivate(from) )
{
    _imp->mainLayout = new QVBoxLayout(this);

    _imp->firstLine = new QWidget(this);
    _imp->firstLineLayout = new QHBoxLayout(_imp->firstLine);

    _imp->mainLayout->addWidget(_imp->firstLine);

    _imp->buttons = new QDialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
                                         Qt::Horizontal,this);
    QObject::connect( _imp->buttons, SIGNAL( accepted() ), this, SLOT( accept() ) );
    QObject::connect( _imp->buttons, SIGNAL( rejected() ), this, SLOT( reject() ) );
    _imp->mainLayout->addWidget(_imp->buttons);

    _imp->selectNodeLabel = new QLabel(tr("Parent:"),_imp->firstLine);
    _imp->firstLineLayout->addWidget(_imp->selectNodeLabel);


    assert( from->getKnob()->getHolder()->getApp() );
    from->getKnob()->getHolder()->getApp()->getActiveNodes(&_imp->allNodes);
    QStringList nodeNames;
    for (U32 i = 0; i < _imp->allNodes.size(); ++i) {
        QString name( _imp->allNodes[i]->getName().c_str() );
        nodeNames.push_back(name);
        //_imp->nodeSelectionCombo->addItem(name);
    }
    nodeNames.sort();
    _imp->nodeSelectionCombo = new CompleterLineEdit(nodeNames,nodeNames,false,this);
    _imp->nodeSelectionCombo->setToolTip( tr("Input the name of a node in the current project.") );
    _imp->firstLineLayout->addWidget(_imp->nodeSelectionCombo);


    _imp->nodeSelectionCombo->setFocus(Qt::PopupFocusReason);
    QTimer::singleShot( 25, _imp->nodeSelectionCombo, SLOT( showCompleter() ) );

    _imp->knobSelectionCombo = new ComboBox(_imp->firstLine);
    _imp->firstLineLayout->addWidget(_imp->knobSelectionCombo);

    QObject::connect( _imp->nodeSelectionCombo,SIGNAL( itemCompletionChosen() ),this,SLOT( onNodeComboEditingFinished() ) );

    _imp->firstLineLayout->addStretch();
}

LinkToKnobDialog::~LinkToKnobDialog()
{
}

void
LinkToKnobDialog::onNodeComboEditingFinished()
{
    QString index = _imp->nodeSelectionCombo->text();

    _imp->knobSelectionCombo->clear();
    boost::shared_ptr<Natron::Node> selectedNode;
    std::string currentNodeName = index.toStdString();
    for (U32 i = 0; i < _imp->allNodes.size(); ++i) {
        if (_imp->allNodes[i]->getName() == currentNodeName) {
            selectedNode = _imp->allNodes[i];
            break;
        }
    }
    if (!selectedNode) {
        return;
    }

    const std::vector< boost::shared_ptr<KnobI> > & knobs = selectedNode->getKnobs();
    boost::shared_ptr<KnobI> from = _imp->fromKnob->getKnob();
    for (U32 j = 0; j < knobs.size(); ++j) {
        if ( !knobs[j]->getIsSecret() && (knobs[j] != from) ) {
            Button_Knob* isButton = dynamic_cast<Button_Knob*>( knobs[j].get() );
            Page_Knob* isPage = dynamic_cast<Page_Knob*>( knobs[j].get() );
            Group_Knob* isGroup = dynamic_cast<Group_Knob*>( knobs[j].get() );
            if (from->isTypeCompatible(knobs[j]) && !isButton && !isPage && !isGroup) {
                QString name( knobs[j]->getDescription().c_str() );

                bool canInsertKnob = true;
                for (int k = 0; k < knobs[j]->getDimension(); ++k) {
                    if ( knobs[j]->isSlave(k) || !knobs[j]->isEnabled(k) || name.isEmpty() ) {
                        canInsertKnob = false;
                    }
                }
                if (canInsertKnob) {
                    _imp->allKnobs.insert( std::make_pair( name, knobs[j]) );
                    _imp->knobSelectionCombo->addItem(name);
                }
            }
        }
    }
}

boost::shared_ptr<KnobI> LinkToKnobDialog::getSelectedKnobs() const
{
    QString str = _imp->knobSelectionCombo->itemText( _imp->knobSelectionCombo->activeIndex() );
    std::map<QString,boost::shared_ptr<KnobI> >::const_iterator it = _imp->allKnobs.find(str);

    if ( it != _imp->allKnobs.end() ) {
        return it->second;
    } else {
        return boost::shared_ptr<KnobI>();
    }
}

void
KnobGui::onKnobSlavedChanged(int dimension,
                             bool b)
{
    if (b) {
        emit keyFrameRemoved();
    } else {
        emit keyFrameSet();
    }
    setReadOnly_(b, dimension);
}

void
KnobGui::linkTo()
{
    LinkToKnobDialog dialog( this,_imp->copyRightClickMenu->parentWidget() );

    if ( dialog.exec() ) {
        boost::shared_ptr<KnobI> thisKnob = getKnob();
        boost::shared_ptr<KnobI>  otherKnob = dialog.getSelectedKnobs();
        if (otherKnob) {
            if ( !thisKnob->isTypeCompatible(otherKnob) ) {
                errorDialog( tr("Knob Link").toStdString(), tr("Types incompatibles!").toStdString() );

                return;
            }

            for (int i = 0; i < thisKnob->getDimension(); ++i) {
                std::pair<int,boost::shared_ptr<KnobI> > existingLink = thisKnob->getMaster(i);
                if (existingLink.second) {
                    std::string err( tr("Cannot link ").toStdString() );
                    err.append( thisKnob->getDescription() );
                    err.append( " \n " + tr("because the knob is already linked to ").toStdString() );
                    err.append( existingLink.second->getDescription() );
                    errorDialog(tr("Knob Link").toStdString(), err);

                    return;
                }
            }

            thisKnob->blockEvaluation();
            int dims = thisKnob->getDimension();
            for (int i = 0; i < dims; ++i) {
                if (i == dims - 1) {
                    thisKnob->unblockEvaluation();
                }
                thisKnob->onKnobSlavedTo(i, otherKnob,i);
                onKnobSlavedChanged(i, true);
            }
            thisKnob->getHolder()->getApp()->triggerAutoSave();
        }
    }
}

void
KnobGui::onLinkToActionTriggered()
{
    linkTo();
}

void
KnobGui::unlink()
{
    boost::shared_ptr<KnobI> thisKnob = getKnob();
    int dims = thisKnob->getDimension();

    thisKnob->blockEvaluation();
    for (int i = 0; i < dims; ++i) {
        std::pair<int,boost::shared_ptr<KnobI> > other = thisKnob->getMaster(i);
        if (i == dims - 1) {
            thisKnob->unblockEvaluation();
        }
        thisKnob->onKnobUnSlaved(i);
        onKnobSlavedChanged(i, false);
    }
    getKnob()->getHolder()->getApp()->triggerAutoSave();
}

void
KnobGui::onUnlinkActionTriggered()
{
    unlink();
}

void
KnobGui::onResetDefaultValuesActionTriggered()
{
    QAction *action = qobject_cast<QAction *>( sender() );

    if (action) {
        resetDefault( action->data().toInt() );
    }
}

void
KnobGui::resetDefault(int /*dimension*/)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    Button_Knob* isBtn = dynamic_cast<Button_Knob*>( knob.get() );
    Page_Knob* isPage = dynamic_cast<Page_Knob*>( knob.get() );
    Group_Knob* isGroup = dynamic_cast<Group_Knob*>( knob.get() );
    Separator_Knob* isSeparator = dynamic_cast<Separator_Knob*>( knob.get() );

    if (!isBtn && !isPage && !isGroup && !isSeparator) {
        std::list<boost::shared_ptr<KnobI> > knobs;
        knobs.push_back(knob);
        pushUndoCommand( new RestoreDefaultsCommand(knobs) );
    }
}

void
KnobGui::setReadOnly_(bool readOnly,
                      int dimension)
{
    if (!_imp->customInteract) {
        setReadOnly(readOnly, dimension);
    }

    ///This code doesn't work since the knob dimensions are still enabled even if readonly
//    bool hasDimensionEnabled = false;
//    for (int i = 0; i < getKnob()->getDimension(); ++i) {
//        if (getKnob()->isEnabled(i)) {
//            hasDimensionEnabled = true;
//        }
//    }
//    _descriptionLabel->setEnabled(hasDimensionEnabled);
}

bool
KnobGui::triggerNewLine() const
{
    return _imp->triggerNewLine;
}

void
KnobGui::turnOffNewLine()
{
    _imp->triggerNewLine = false;
}

/*Set the spacing between items in the layout*/
void
KnobGui::setSpacingBetweenItems(int spacing)
{
    _imp->spacingBetweenItems = spacing;
}

int
KnobGui::getSpacingBetweenItems() const
{
    return _imp->spacingBetweenItems;
}

bool
KnobGui::hasWidgetBeenCreated() const
{
    return _imp->widgetCreated;
}

void
KnobGui::onSetValueUsingUndoStack(const Variant & v,
                                  int dim)
{
    boost::shared_ptr<KnobI> knob = getKnob();

    Knob<int>* isInt = dynamic_cast<Knob<int>*>( knob.get() );
    Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( knob.get() );
    Knob<double>* isDouble = dynamic_cast<Knob<double>*>( knob.get() );
    Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( knob.get() );

    if (isInt) {
        pushUndoCommand( new KnobUndoCommand<int>(this,isInt->getValue(dim),v.toInt(),dim) );
    } else if (isBool) {
        pushUndoCommand( new KnobUndoCommand<bool>(this,isBool->getValue(dim),v.toBool(),dim) );
    } else if (isDouble) {
        pushUndoCommand( new KnobUndoCommand<double>(this,isDouble->getValue(dim),v.toDouble(),dim) );
    } else if (isString) {
        pushUndoCommand( new KnobUndoCommand<std::string>(this,isString->getValue(dim),v.toString().toStdString(),dim) );
    }
}

void
KnobGui::onSetDirty(bool d)
{
    if (!_imp->customInteract) {
        setDirty(d);
    }
}

void
KnobGui::swapOpenGLBuffers()
{
    if (_imp->customInteract) {
        _imp->customInteract->swapOpenGLBuffers();
    }
}

void
KnobGui::redraw()
{
    if (_imp->customInteract) {
        _imp->customInteract->redraw();
    }
}

void
KnobGui::getViewportSize(double &width,
                         double &height) const
{
    if (_imp->customInteract) {
        _imp->customInteract->getViewportSize(width, height);
    }
}

void
KnobGui::getPixelScale(double & xScale,
                       double & yScale) const
{
    if (_imp->customInteract) {
        _imp->customInteract->getPixelScale(xScale, yScale);
    }
}

void
KnobGui::getBackgroundColour(double &r,
                             double &g,
                             double &b) const
{
    if (_imp->customInteract) {
        _imp->customInteract->getBackgroundColour(r, g, b);
    }
}

void
KnobGui::saveOpenGLContext()
{
    if (_imp->customInteract) {
        _imp->customInteract->saveOpenGLContext();
    }
}

void
KnobGui::restoreOpenGLContext()
{
    if (_imp->customInteract) {
        _imp->customInteract->restoreOpenGLContext();
    }
}

///Should set to the underlying knob the gui ptr
void
KnobGui::setKnobGuiPointer()
{
    getKnob()->setKnobGuiPointer(this);
}

void
KnobGui::onInternalAnimationAboutToBeRemoved()
{
    removeAllKeyframeMarkersOnTimeline(-1);
}

void
KnobGui::onInternalAnimationRemoved()
{
    emit keyFrameRemoved();
}

void
KnobGui::removeAllKeyframeMarkersOnTimeline(int dimension)
{
    boost::shared_ptr<KnobI> knob = getKnob();

    if ( knob->getHolder() && knob->getHolder()->getApp() ) {
        boost::shared_ptr<TimeLine> timeline = knob->getHolder()->getApp()->getTimeLine();
        std::list<SequenceTime> times;
        if (dimension == -1) {
            int dim = knob->getDimension();
            for (int i = 0; i < dim; ++i) {
                KeyFrameSet kfs = knob->getCurve(i)->getKeyFrames_mt_safe();
                for (KeyFrameSet::iterator it = kfs.begin(); it != kfs.end(); ++it) {
                    times.push_back( it->getTime() );
                }
            }
        } else {
            KeyFrameSet kfs = knob->getCurve(dimension)->getKeyFrames_mt_safe();
            for (KeyFrameSet::iterator it = kfs.begin(); it != kfs.end(); ++it) {
                times.push_back( it->getTime() );
            }
        }
        timeline->removeMultipleKeyframeIndicator(times,true);
    }
}

void
KnobGui::setAllKeyframeMarkersOnTimeline(int dimension)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    boost::shared_ptr<TimeLine> timeline = knob->getHolder()->getApp()->getTimeLine();
    std::list<SequenceTime> times;

    if (dimension == -1) {
        int dim = knob->getDimension();
        for (int i = 0; i < dim; ++i) {
            KeyFrameSet kfs = knob->getCurve(i)->getKeyFrames_mt_safe();
            for (KeyFrameSet::iterator it = kfs.begin(); it != kfs.end(); ++it) {
                times.push_back( it->getTime() );
            }
        }
    } else {
        KeyFrameSet kfs = knob->getCurve(dimension)->getKeyFrames_mt_safe();
        for (KeyFrameSet::iterator it = kfs.begin(); it != kfs.end(); ++it) {
            times.push_back( it->getTime() );
        }
    }
    timeline->addMultipleKeyframeIndicatorsAdded(times,true);
}

void
KnobGui::setKeyframeMarkerOnTimeline(int time)
{
    boost::shared_ptr<KnobI> knob = getKnob();

    knob->getHolder()->getApp()->getTimeLine()->addKeyframeIndicator(time);
}

void
KnobGui::onKeyFrameMoved(int oldTime,
                         int newTime)
{
    boost::shared_ptr<KnobI> knob = getKnob();

    if ( !knob->isAnimationEnabled() || !knob->canAnimate() ) {
        return;
    }
    boost::shared_ptr<TimeLine> timeline = knob->getHolder()->getApp()->getTimeLine();
    timeline->removeKeyFrameIndicator(oldTime);
    timeline->addKeyframeIndicator(newTime);
}

void
KnobGui::onAnimationLevelChanged(int dim,int level)
{
    if (!_imp->customInteract) {
        reflectAnimationLevel(dim, (Natron::AnimationLevel)level);
    }
}

void
KnobGui::onAppendParamEditChanged(const Variant & v,
                                  int dim,
                                  int time,
                                  bool createNewCommand,
                                  bool setKeyFrame)
{
    pushUndoCommand( new MultipleKnobEditsUndoCommand(this,createNewCommand,setKeyFrame,v,dim,time) );
}

void
KnobGui::onFrozenChanged(bool frozen)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    int dims = knob->getDimension();

    for (int i = 0; i < dims; ++i) {
        ///Do not unset read only if the knob is slaved in this dimension because we are still using it.
        if ( !frozen && knob->isSlave(i) ) {
            continue;
        }
        setReadOnly_(frozen, i);
    }
}

bool
KnobGui::isGuiFrozenForPlayback() const
{
    return getGui() ? getGui()->isGUIFrozen() : false;
}

