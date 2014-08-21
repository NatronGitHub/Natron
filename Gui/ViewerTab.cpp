//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "ViewerTab.h"

#include <cassert>
#include <QDebug>
#include <QApplication>
#include <QSlider>
#include <QComboBox>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QAbstractItemView>
#include <QCheckBox>
#include <QCoreApplication>
#include <QToolBar>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QKeyEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QtGui/QKeySequence>
#include <QTextDocument>

#include "Engine/ViewerInstance.h"
#include "Engine/VideoEngine.h"
#include "Engine/Settings.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"
#include "Engine/Node.h"

#include "Gui/ViewerGL.h"
#include "Gui/InfoViewerWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TimeLineGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/ComboBox.h"
#include "Gui/Button.h"
#include "Gui/Gui.h"
#include "Gui/TabWidget.h"
#include "Gui/FromQtEnums.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/ClickableLabel.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/RotoGui.h"
#include "Gui/TrackerGui.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/GuiMacros.h"

using namespace Natron;

namespace {
struct InputName
{
    QString name;
    EffectInstance* input;
};
typedef std::map<int,InputName> InputNamesMap;
    

}

struct ViewerTabPrivate {
    
    /*OpenGL viewer*/
	ViewerGL* viewer;
    
    GuiAppInstance* app;
    
    QWidget* _viewerContainer;
    QHBoxLayout* _viewerLayout;
    
    QWidget* _viewerSubContainer;
    QVBoxLayout* _viewerSubContainerLayout;
    
    QVBoxLayout* _mainLayout;
    
	/*Viewer Settings*/
    QWidget* _firstSettingsRow,*_secondSettingsRow;
    QHBoxLayout* _firstRowLayout,*_secondRowLayout;
    
    /*1st row*/
	//ComboBox* _viewerLayers;
	ComboBox* _viewerChannels;
    ComboBox* _zoomCombobox;
    Button* _centerViewerButton;
    
    Button* _clipToProjectFormatButton;
    Button* _enableViewerRoI;
    Button* _refreshButton;
    Button* _activateRenderScale;
    bool _renderScaleActive;
    ComboBox* _renderScaleCombo;
    
    QLabel* _firstInputLabel;
    ComboBox* _firstInputImage;
    ComboBox* _compositingOperator;
    QLabel* _secondInputLabel;
    ComboBox* _secondInputImage;
    
    /*2nd row*/
    SpinBox* _gainBox;
    ScaleSliderQWidget* _gainSlider;
    ClickableLabel* _autoConstrastLabel;
    QCheckBox* _autoContrast;
    ComboBox* _viewerColorSpace;
    ComboBox* _viewsComboBox;
    int _currentViewIndex;
    QMutex _currentViewMutex;
    /*Infos*/
    InfoViewerWidget* _infosWidget[2];
    
    
	/*TimeLine buttons*/
    QWidget* _playerButtonsContainer;
	QHBoxLayout* _playerLayout;
	SpinBox* _currentFrameBox;
	Button* firstFrame_Button;
    Button* previousKeyFrame_Button;
    Button* play_Backward_Button;
	Button* previousFrame_Button;
    Button* stop_Button;
    Button* nextFrame_Button;
	Button* play_Forward_Button;
    Button* nextKeyFrame_Button;
	Button* lastFrame_Button;
    Button* previousIncrement_Button;
    SpinBox* incrementSpinBox;
    Button* nextIncrement_Button;
    Button* loopMode_Button;
    
    LineEdit* frameRangeEdit;
    Button* lockFrameRangeButton;
    mutable QMutex frameRangeLockedMutex;
    bool frameRangeLocked;
    
    QLabel* fpsName;
    SpinBox* fpsBox;
    
	/*frame seeker*/
    TimeLineGui* _timeLineGui;
    
    std::map<NodeGui*,RotoGui*> _rotoNodes;
    std::pair<NodeGui*,RotoGui*> _currentRoto;
    
    std::map<NodeGui*,TrackerGui*> _trackerNodes;
    std::pair<NodeGui*,TrackerGui*> _currentTracker;

    InputNamesMap _inputNamesMap;
    mutable QMutex compOperatorMutex;
    ViewerCompositingOperator _compOperator;
    
    Gui* _gui;
    
    ViewerInstance* _viewerNode;// < pointer to the internal node

    ViewerTabPrivate(Gui* gui,ViewerInstance* node)
    : app(gui->getApp())
    , _renderScaleActive(false)
    , _currentViewIndex(0)
    , frameRangeLocked(true)
    , _timeLineGui(NULL)
    , _compOperator(OPERATOR_NONE)
    , _gui(gui)
    , _viewerNode(node)
    {
        _currentRoto.first = NULL;
        _currentRoto.second = NULL;
    }
};

ViewerTab::ViewerTab(const std::list<NodeGui*>& existingRotoNodes,
                     NodeGui* currentRoto,
                     const std::list<NodeGui*>& existingTrackerNodes,
                     NodeGui* currentTracker,
                     Gui* gui,
                     ViewerInstance* node,
                     QWidget* parent)
: QWidget(parent)
, _imp(new ViewerTabPrivate(gui,node))
{
    
    installEventFilter(this);
    setObjectName(node->getName().c_str());
    _imp->_mainLayout=new QVBoxLayout(this);
    setLayout(_imp->_mainLayout);
    _imp->_mainLayout->setSpacing(0);
    _imp->_mainLayout->setContentsMargins(0, 0, 0, 0);
    
    /*VIEWER SETTINGS*/
    
    /*1st row of buttons*/
    _imp->_firstSettingsRow = new QWidget(this);
    _imp->_firstRowLayout = new QHBoxLayout(_imp->_firstSettingsRow);
    _imp->_firstSettingsRow->setLayout(_imp->_firstRowLayout);
    _imp->_firstRowLayout->setContentsMargins(0, 0, 0, 0);
    _imp->_firstRowLayout->setSpacing(0);
    _imp->_mainLayout->addWidget(_imp->_firstSettingsRow);
    
    // _viewerLayers = new ComboBox(_firstSettingsRow);
    //_firstRowLayout->addWidget(_viewerLayers);
    
    _imp->_viewerChannels = new ComboBox(_imp->_firstSettingsRow);
    _imp->_viewerChannels->setToolTip("<p><b>" + tr("Channels") + ": \n</b></p>"
                                +tr("The channels to display on the viewer."));
    _imp->_firstRowLayout->addWidget(_imp->_viewerChannels);
    
    _imp->_viewerChannels->addItem("Luminance",QIcon(),QKeySequence(Qt::Key_Y));
    _imp->_viewerChannels->addItem("RGB");
    _imp->_viewerChannels->addItem("R",QIcon(),QKeySequence(Qt::Key_R));
    _imp->_viewerChannels->addItem("G",QIcon(),QKeySequence(Qt::Key_G));
    _imp->_viewerChannels->addItem("B",QIcon(),QKeySequence(Qt::Key_B));
    _imp->_viewerChannels->addItem("A",QIcon(),QKeySequence(Qt::Key_A));
    _imp->_viewerChannels->setCurrentIndex(1);
    QObject::connect(_imp->_viewerChannels, SIGNAL(currentIndexChanged(int)), this, SLOT(onViewerChannelsChanged(int)));
    
    _imp->_zoomCombobox = new ComboBox(_imp->_firstSettingsRow);
    _imp->_zoomCombobox->setToolTip("<p><b>" + tr("Zoom") + ": \n</b></p>"
                              +tr("The zoom applied to the image on the viewer."));
    _imp->_zoomCombobox->addItem("10%");
    _imp->_zoomCombobox->addItem("25%");
    _imp->_zoomCombobox->addItem("50%");
    _imp->_zoomCombobox->addItem("75%");
    _imp->_zoomCombobox->addItem("100%");
    _imp->_zoomCombobox->addItem("125%");
    _imp->_zoomCombobox->addItem("150%");
    _imp->_zoomCombobox->addItem("200%");
    _imp->_zoomCombobox->addItem("400%");
    _imp->_zoomCombobox->addItem("800%");
    _imp->_zoomCombobox->addItem("1600%");
    _imp->_zoomCombobox->addItem("2400%");
    _imp->_zoomCombobox->addItem("3200%");
    _imp->_zoomCombobox->addItem("6400%");
    _imp->_zoomCombobox->setMaximumWidthFromText("100000%");

    _imp->_firstRowLayout->addWidget(_imp->_zoomCombobox);
    
    _imp->_centerViewerButton = new Button(_imp->_firstSettingsRow);
    _imp->_centerViewerButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->_firstRowLayout->addWidget(_imp->_centerViewerButton);
    
    
    _imp->_clipToProjectFormatButton = new Button(_imp->_firstSettingsRow);
    _imp->_clipToProjectFormatButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->_clipToProjectFormatButton->setCheckable(true);
    _imp->_clipToProjectFormatButton->setChecked(true);
    _imp->_clipToProjectFormatButton->setDown(true);
    _imp->_firstRowLayout->addWidget(_imp->_clipToProjectFormatButton);
    
    _imp->_enableViewerRoI = new Button(_imp->_firstSettingsRow);
    _imp->_enableViewerRoI->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->_enableViewerRoI->setCheckable(true);
    _imp->_enableViewerRoI->setChecked(false);
    _imp->_enableViewerRoI->setDown(false);
    _imp->_firstRowLayout->addWidget(_imp->_enableViewerRoI);
    
    _imp->_refreshButton = new Button(_imp->_firstSettingsRow);
    _imp->_refreshButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->_refreshButton->setToolTip(tr("Forces a new render of the current frame.")+
                                     "<p><b>" + tr("Keyboard shortcut") + ": U</b></p>");
    _imp->_firstRowLayout->addWidget(_imp->_refreshButton);
    
    _imp->_activateRenderScale = new Button(_imp->_firstSettingsRow);
    _imp->_activateRenderScale->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence rsKs(Qt::CTRL + Qt::Key_P);
    _imp->_activateRenderScale->setToolTip("<p><b>" + tr("Proxy mode") + "</b></p>" + tr(
                                           "Activates the downscaling by the amount indicated by the value on the right. \n"
                                           "The rendered images are degraded and as a result of this the whole rendering pipeline \n"
                                           "is much faster.")+
                                           "<p><b>" + tr("Keyboard shortcut") + ": " + rsKs.toString(QKeySequence::NativeText) +"</b></p>");
    _imp->_activateRenderScale->setCheckable(true);
    _imp->_activateRenderScale->setChecked(false);
    _imp->_activateRenderScale->setDown(false);
    _imp->_firstRowLayout->addWidget(_imp->_activateRenderScale);
    
    _imp->_renderScaleCombo = new ComboBox(_imp->_firstSettingsRow);
    _imp->_renderScaleCombo->setToolTip(tr("When proxy mode is activated, it scales down the rendered image by this factor \n"
                                        "to accelerate the rendering."));
    _imp->_renderScaleCombo->addItem("2",QIcon(),QKeySequence(Qt::ALT + Qt::Key_1));
    _imp->_renderScaleCombo->addItem("4",QIcon(),QKeySequence(Qt::ALT + Qt::Key_2));
    _imp->_renderScaleCombo->addItem("8",QIcon(),QKeySequence(Qt::ALT + Qt::Key_3));
    _imp->_renderScaleCombo->addItem("16",QIcon(),QKeySequence(Qt::ALT + Qt::Key_4));
    _imp->_renderScaleCombo->addItem("32",QIcon(),QKeySequence(Qt::ALT + Qt::Key_5));
    _imp->_firstRowLayout->addWidget(_imp->_renderScaleCombo);
    
    _imp->_firstRowLayout->addStretch();
    
    _imp->_firstInputLabel = new QLabel("A:",_imp->_firstSettingsRow);
    _imp->_firstRowLayout->addWidget(_imp->_firstInputLabel);
    
    _imp->_firstInputImage = new ComboBox(_imp->_firstSettingsRow);
    _imp->_firstInputImage->addItem(" - ");
    QObject::connect(_imp->_firstInputImage,SIGNAL(currentIndexChanged(QString)),this,SLOT(onFirstInputNameChanged(QString)));
    
    _imp->_firstRowLayout->addWidget(_imp->_firstInputImage);
    
    _imp->_compositingOperator = new ComboBox(_imp->_firstSettingsRow);
    QObject::connect(_imp->_compositingOperator,SIGNAL(currentIndexChanged(int)),this,SLOT(onCompositingOperatorIndexChanged(int)));
    _imp->_compositingOperator->addItem(" - ",QIcon(),QKeySequence(),"Only the A input is used.");
    _imp->_compositingOperator->addItem("Over",QIcon(),QKeySequence(),"A + B(1 - Aalpha)");
    _imp->_compositingOperator->addItem("Under",QIcon(),QKeySequence(),"A(1 - Balpha) + B");
    _imp->_compositingOperator->addItem("Minus",QIcon(),QKeySequence(),"A - B");
    _imp->_compositingOperator->addItem("Wipe",QIcon(),QKeySequence(),"Wipe betweens A and B");
    _imp->_firstRowLayout->addWidget(_imp->_compositingOperator);
    
    _imp->_secondInputLabel = new QLabel("B:",_imp->_firstSettingsRow);
    _imp->_firstRowLayout->addWidget(_imp->_secondInputLabel);
    
    _imp->_secondInputImage = new ComboBox(_imp->_firstSettingsRow);
    QObject::connect(_imp->_secondInputImage,SIGNAL(currentIndexChanged(QString)),this,SLOT(onSecondInputNameChanged(QString)));
    _imp->_secondInputImage->addItem(" - ");
    _imp->_firstRowLayout->addWidget(_imp->_secondInputImage);
    
    _imp->_firstRowLayout->addStretch();
    
    /*2nd row of buttons*/
    _imp->_secondSettingsRow = new QWidget(this);
    _imp->_secondRowLayout = new QHBoxLayout(_imp->_secondSettingsRow);
    _imp->_secondSettingsRow->setLayout(_imp->_secondRowLayout);
    _imp->_secondRowLayout->setSpacing(0);
    _imp->_secondRowLayout->setContentsMargins(0, 0, 0, 0);
    _imp->_mainLayout->addWidget(_imp->_secondSettingsRow);
    
    _imp->_gainBox = new SpinBox(_imp->_secondSettingsRow,SpinBox::DOUBLE_SPINBOX);
    _imp->_gainBox->setToolTip("<p><b>" + tr("Gain") + ": \n</b></p>" + tr(
                         "Multiplies the image by \nthis amount before display."));
    _imp->_gainBox->setIncrement(0.1);
    _imp->_gainBox->setValue(1.0);
    _imp->_gainBox->setMinimum(0.0);
    _imp->_secondRowLayout->addWidget(_imp->_gainBox);
    
    
    _imp->_gainSlider=new ScaleSliderQWidget(0, 64,1.0,Natron::LINEAR_SCALE,_imp->_secondSettingsRow);
    _imp->_gainSlider->setToolTip("<p><b>" + tr("Gain") + ": \n</b></p>" + tr(
                            "Multiplies the image by \nthis amount before display."));
    _imp->_secondRowLayout->addWidget(_imp->_gainSlider);
    
    QString autoContrastToolTip("<p><b>" + tr("Auto-contrast") + ": \n</b></p>" + tr(
                                "Automatically adjusts the gain and the offset applied \n"
                                "to the colors of the visible image portion on the viewer."));
    _imp->_autoConstrastLabel = new ClickableLabel(tr("Auto-contrast:"),_imp->_secondSettingsRow);
    _imp->_autoConstrastLabel->setToolTip(autoContrastToolTip);
    _imp->_secondRowLayout->addWidget(_imp->_autoConstrastLabel);
    
    _imp->_autoContrast = new QCheckBox(_imp->_secondSettingsRow);
    _imp->_autoContrast->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    _imp->_autoContrast->setToolTip(autoContrastToolTip);
    _imp->_secondRowLayout->addWidget(_imp->_autoContrast);
    
    _imp->_viewerColorSpace=new ComboBox(_imp->_secondSettingsRow);
    _imp->_viewerColorSpace->setToolTip("<p><b>" + tr("Viewer color process") + ": \n</b></p>" + tr(
                                  "The operation applied to the image before it is displayed\n"
                                  "on screen. All the color pipeline \n"
                                  "is linear,thus the process converts from linear\n"
                                  "to your monitor's colorspace."));
    _imp->_secondRowLayout->addWidget(_imp->_viewerColorSpace);
    
    _imp->_viewerColorSpace->addItem("Linear(None)");
    _imp->_viewerColorSpace->addItem("sRGB");
    _imp->_viewerColorSpace->addItem("Rec.709");
    _imp->_viewerColorSpace->setCurrentIndex(1);
    
    _imp->_viewsComboBox = new ComboBox(_imp->_secondSettingsRow);
    _imp->_viewsComboBox->setToolTip("<p><b>" + tr("Active view") + ": \n</b></p>" + tr(
                               "Tells the viewer what view should be displayed."));
    _imp->_secondRowLayout->addWidget(_imp->_viewsComboBox);
    _imp->_viewsComboBox->hide();
    int viewsCount = _imp->app->getProject()->getProjectViewsCount(); //getProjectViewsCount
    updateViewsMenu(viewsCount);
    
    _imp->_secondRowLayout->addStretch();
    
    /*=============================================*/
    
    /*OpenGL viewer*/
    _imp->_viewerContainer = new QWidget(this);
    _imp->_viewerLayout = new QHBoxLayout(_imp->_viewerContainer);
    _imp->_viewerLayout->setContentsMargins(0, 0, 0, 0);
    _imp->_viewerLayout->setSpacing(0);
    
    _imp->_viewerSubContainer = new QWidget(_imp->_viewerContainer);
    _imp->_viewerSubContainerLayout = new QVBoxLayout(_imp->_viewerSubContainer);
    _imp->_viewerSubContainerLayout->setContentsMargins(0, 0, 0, 0);
    _imp->_viewerSubContainerLayout->setSpacing(0);
    
    _imp->viewer = new ViewerGL(this);
    
    _imp->_viewerSubContainerLayout->addWidget(_imp->viewer);
    
    /*info bbox & color*/
    QString inputNames[2] = { "A:" , "B:" };
    for (int i = 0; i < 2 ; ++i) {
        _imp->_infosWidget[i] = new InfoViewerWidget(_imp->viewer,inputNames[i],this);
        _imp->_viewerSubContainerLayout->addWidget(_imp->_infosWidget[i]);
        _imp->viewer->setInfoViewer(_imp->_infosWidget[i],i);
        if (i == 1) {
            _imp->_infosWidget[i]->hide();
        }
    }
    
    _imp->_viewerLayout->addWidget(_imp->_viewerSubContainer);
    
    _imp->_mainLayout->addWidget(_imp->_viewerContainer);
    /*=============================================*/
    
    /*=============================================*/
    
    /*Player buttons*/
    _imp->_playerButtonsContainer = new QWidget(this);
    _imp->_playerLayout=new QHBoxLayout(_imp->_playerButtonsContainer);
    _imp->_playerLayout->setSpacing(0);
    _imp->_playerLayout->setContentsMargins(0, 0, 0, 0);
    _imp->_playerButtonsContainer->setLayout(_imp->_playerLayout);
    _imp->_mainLayout->addWidget(_imp->_playerButtonsContainer);
    
    _imp->_currentFrameBox=new SpinBox(_imp->_playerButtonsContainer,SpinBox::INT_SPINBOX);
    _imp->_currentFrameBox->setValue(0);
    _imp->_currentFrameBox->setToolTip("<p><b>" + tr("Current frame number") + "</b></p>");
    _imp->_playerLayout->addWidget(_imp->_currentFrameBox);
    
    _imp->_playerLayout->addStretch();
    
    _imp->firstFrame_Button = new Button(_imp->_playerButtonsContainer);
    _imp->firstFrame_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence firstFrameKey(Qt::CTRL + Qt::Key_Left);
    QString tooltip = "First frame";
    tooltip.append("<p><b>" + tr("Keyboard shortcut: "));
    tooltip.append(firstFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->firstFrame_Button->setToolTip(tooltip);
    _imp->_playerLayout->addWidget(_imp->firstFrame_Button);
    
    
    _imp->previousKeyFrame_Button=new Button(_imp->_playerButtonsContainer);
    _imp->previousKeyFrame_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence previousKeyFrameKey(Qt::CTRL + Qt::SHIFT +  Qt::Key_Left);
    tooltip = tr("Previous keyframe");
    tooltip.append(tr("<p><b>Keyboard shortcut: "));
    tooltip.append(previousKeyFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->previousKeyFrame_Button->setToolTip(tooltip);
    
    
    _imp->play_Backward_Button=new Button(_imp->_playerButtonsContainer);
    _imp->play_Backward_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence playbackFrameKey(Qt::Key_J);
    tooltip = tr("Play backward");
    tooltip.append("<p><b>" + tr("Keyboard shortcut: "));
    tooltip.append(playbackFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->play_Backward_Button->setToolTip(tooltip);
    _imp->play_Backward_Button->setCheckable(true);
    _imp->_playerLayout->addWidget(_imp->play_Backward_Button);
    
    
    _imp->previousFrame_Button = new Button(_imp->_playerButtonsContainer);
    _imp->previousFrame_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence previousFrameKey(Qt::Key_Left);
    tooltip = tr("Previous frame");
    tooltip.append("<p><b>" + tr("Keyboard shortcut: "));
    tooltip.append(previousFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->previousFrame_Button->setToolTip(tooltip);
    _imp->_playerLayout->addWidget(_imp->previousFrame_Button);
    
    
    _imp->stop_Button = new Button(_imp->_playerButtonsContainer);
    _imp->stop_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence stopKey(Qt::Key_K);
    tooltip = tr("Stop");
    tooltip.append("<p><b>" + tr("Keyboard shortcut: "));
    tooltip.append(stopKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->stop_Button->setToolTip(tooltip);
    _imp->_playerLayout->addWidget(_imp->stop_Button);
    
    
    _imp->nextFrame_Button = new Button(_imp->_playerButtonsContainer);
    _imp->nextFrame_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence nextFrameKey(Qt::Key_Right);
    tooltip = tr("Next frame");
    tooltip.append("<p><b>" + tr("Keyboard shortcut: "));
    tooltip.append(nextFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->nextFrame_Button->setToolTip(tooltip);
    _imp->_playerLayout->addWidget(_imp->nextFrame_Button);
    
    
    _imp->play_Forward_Button = new Button(_imp->_playerButtonsContainer);
    _imp->play_Forward_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence playKey(Qt::Key_L);
    tooltip = tr("Play forward");
    tooltip.append("<p><b>" + tr("Keyboard shortcut: "));
    tooltip.append(playKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->play_Forward_Button->setToolTip(tooltip);
    _imp->play_Forward_Button->setCheckable(true);
    _imp->_playerLayout->addWidget(_imp->play_Forward_Button);
    
    
    _imp->nextKeyFrame_Button = new Button(_imp->_playerButtonsContainer);
    _imp->nextKeyFrame_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence nextKeyFrameKey(Qt::CTRL + Qt::SHIFT +  Qt::Key_Right);
    tooltip = tr("Next keyframe");
    tooltip.append("<p><b>" + tr("Keyboard shortcut: "));
    tooltip.append(nextKeyFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->nextKeyFrame_Button->setToolTip(tooltip);
    
    
    _imp->lastFrame_Button = new Button(_imp->_playerButtonsContainer);
    _imp->lastFrame_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence lastFrameKey(Qt::CTRL + Qt::Key_Right);
    tooltip = tr("Last frame");
    tooltip.append("<p><b>" + tr("Keyboard shortcut: "));
    tooltip.append(lastFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->lastFrame_Button->setToolTip(tooltip);
    _imp->_playerLayout->addWidget(_imp->lastFrame_Button);
    
    
    _imp->_playerLayout->addStretch();
    
    _imp->_playerLayout->addWidget(_imp->previousKeyFrame_Button);
    _imp->_playerLayout->addWidget(_imp->nextKeyFrame_Button);

    _imp->_playerLayout->addStretch();
    
    _imp->previousIncrement_Button = new Button(_imp->_playerButtonsContainer);
    _imp->previousIncrement_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence previousIncrFrameKey(Qt::SHIFT + Qt::Key_Left);
    tooltip = tr("Previous increment");
    tooltip.append("<p><b>" + tr("Keyboard shortcut: "));
    tooltip.append(previousIncrFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->previousIncrement_Button->setToolTip(tooltip);
    _imp->_playerLayout->addWidget(_imp->previousIncrement_Button);
    
    
    _imp->incrementSpinBox=new SpinBox(_imp->_playerButtonsContainer);
    _imp->incrementSpinBox->setValue(10);
    _imp->incrementSpinBox->setToolTip("<p><b>" + tr("Frame increment") + ": \n</b></p>" + tr(
                                 "The previous/next increment buttons step"
                                 " with this increment."));
    _imp->_playerLayout->addWidget(_imp->incrementSpinBox);
    
    
    _imp->nextIncrement_Button = new Button(_imp->_playerButtonsContainer);
    _imp->nextIncrement_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence nextIncrFrameKey(Qt::SHIFT + Qt::Key_Right);
    tooltip = tr("Next increment");
    tooltip.append("<p><b>" + tr("Keyboard shortcut: "));
    tooltip.append(nextIncrFrameKey.toString(QKeySequence::NativeText));
    tooltip.append("</b></p>");
    _imp->nextIncrement_Button->setToolTip(tooltip);
    _imp->_playerLayout->addWidget(_imp->nextIncrement_Button);
    
    _imp->loopMode_Button = new Button(_imp->_playerButtonsContainer);
    _imp->loopMode_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->loopMode_Button->setCheckable(true);
    _imp->loopMode_Button->setChecked(true);
    _imp->loopMode_Button->setDown(true);
    _imp->loopMode_Button->setToolTip(tr("Behaviour to adopt when the playback\n hit the end of the range: loop or stop."));
    _imp->_playerLayout->addWidget(_imp->loopMode_Button);
    
    
    _imp->_playerLayout->addStretch();
    
    _imp->frameRangeEdit = new LineEdit(_imp->_playerButtonsContainer);
    QObject::connect(_imp->frameRangeEdit,SIGNAL(editingFinished()),this,SLOT(onFrameRangeEditingFinished()));
    _imp->frameRangeEdit->setReadOnly(true);
    _imp->frameRangeEdit->setToolTip(Qt::convertFromPlainText(tr("Define here the timeline bounds in which the cursor will playback. Alternatively"
                                                              " you can drag the red markers on the timeline. To activate editing, unlock the"
                                                              " button on the right."),
                                                              Qt::WhiteSpaceNormal));
    boost::shared_ptr<TimeLine> timeline = _imp->app->getTimeLine();
    _imp->frameRangeEdit->setMaximumWidth(70);
    onTimelineBoundariesChanged(timeline->leftBound(), timeline->rightBound(), 0);
    
    _imp->_playerLayout->addWidget(_imp->frameRangeEdit);
    
    QPixmap pixRangeLocked,pixRangeUnlocked;
    appPTR->getIcon(NATRON_PIXMAP_LOCKED, &pixRangeLocked);
    appPTR->getIcon(NATRON_PIXMAP_UNLOCKED, &pixRangeUnlocked);
    QIcon rangeLockedIC;
    rangeLockedIC.addPixmap(pixRangeLocked,QIcon::Normal,QIcon::On);
    rangeLockedIC.addPixmap(pixRangeUnlocked,QIcon::Normal,QIcon::Off);
    _imp->lockFrameRangeButton = new Button(rangeLockedIC,"",_imp->_playerButtonsContainer);
    _imp->lockFrameRangeButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->lockFrameRangeButton->setCheckable(true);
    _imp->lockFrameRangeButton->setChecked(true);
    _imp->lockFrameRangeButton->setDown(true);
    _imp->lockFrameRangeButton->setToolTip(Qt::convertFromPlainText(tr("When locked, the timeline bounds will be automatically set by "
                                                                    "%1 so it defines the real frame range as "
                                                                    "informed by the Readers. When unchecked, the bounds will no longer "
                                                                    "be automatically set, and you're free to set them in the edit line "
                                                                    "on the left or by dragging the timeline markers.").arg(NATRON_APPLICATION_NAME)
                                                                    ,Qt::WhiteSpaceNormal));
    QObject::connect(_imp->lockFrameRangeButton,SIGNAL(clicked(bool)),this,SLOT(onLockFrameRangeButtonClicked(bool)));
    _imp->_playerLayout->addWidget(_imp->lockFrameRangeButton);
    
    _imp->_playerLayout->addStretch();
    
    _imp->fpsName = new QLabel(tr("fps"),_imp->_playerButtonsContainer);
    _imp->_playerLayout->addWidget(_imp->fpsName);
    _imp->fpsBox = new SpinBox(_imp->_playerButtonsContainer,SpinBox::DOUBLE_SPINBOX);
    _imp->fpsBox->decimals(1);
    _imp->fpsBox->setValue(24.0);
    _imp->fpsBox->setIncrement(0.1);
    _imp->fpsBox->setToolTip("<p><b>" + tr("fps") + ": \n</b></p>" + tr(
                       "Enter here the desired playback rate."));
    _imp->_playerLayout->addWidget(_imp->fpsBox);
    
    
    QPixmap pixFirst;
    QPixmap pixPrevKF;
    QPixmap pixRewind;
    QPixmap pixBack1;
    QPixmap pixStop;
    QPixmap pixForward1;
    QPixmap pixPlay;
    QPixmap pixNextKF;
    QPixmap pixLast;
    QPixmap pixPrevIncr;
    QPixmap pixNextIncr;
    QPixmap pixRefresh ;
    QPixmap pixCenterViewer ;
    QPixmap pixLoopMode;
    QPixmap pixClipToProjectEnabled ;
    QPixmap pixClipToProjectDisabled ;
    QPixmap pixViewerRoIEnabled;
    QPixmap pixViewerRoIDisabled;
    QPixmap pixViewerRs;
    QPixmap pixViewerRsChecked;
    
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_FIRST_FRAME,&pixFirst);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PREVIOUS_KEY,&pixPrevKF);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_REWIND,&pixRewind);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PREVIOUS,&pixBack1);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_STOP,&pixStop);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_NEXT,&pixForward1);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PLAY,&pixPlay);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_NEXT_KEY,&pixNextKF);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_LAST_FRAME,&pixLast);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PREVIOUS_INCR,&pixPrevIncr);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_NEXT_INCR,&pixNextIncr);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_REFRESH,&pixRefresh);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_CENTER,&pixCenterViewer);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_LOOP_MODE,&pixLoopMode);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_ENABLED,&pixClipToProjectEnabled);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_DISABLED,&pixClipToProjectDisabled);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_ROI_ENABLED,&pixViewerRoIEnabled);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_ROI_DISABLED,&pixViewerRoIDisabled);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_RENDER_SCALE,&pixViewerRs);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_RENDER_SCALE_CHECKED,&pixViewerRsChecked);
    
    _imp->firstFrame_Button->setIcon(QIcon(pixFirst));
    _imp->previousKeyFrame_Button->setIcon(QIcon(pixPrevKF));
    _imp->play_Backward_Button->setIcon(QIcon(pixRewind));
    _imp->previousFrame_Button->setIcon(QIcon(pixBack1));
    _imp->stop_Button->setIcon(QIcon(pixStop));
    _imp->nextFrame_Button->setIcon(QIcon(pixForward1));
    _imp->play_Forward_Button->setIcon(QIcon(pixPlay));
    _imp->nextKeyFrame_Button->setIcon(QIcon(pixNextKF));
    _imp->lastFrame_Button->setIcon(QIcon(pixLast));
    _imp->previousIncrement_Button->setIcon(QIcon(pixPrevIncr));
    _imp->nextIncrement_Button->setIcon(QIcon(pixNextIncr));
    _imp->_refreshButton->setIcon(QIcon(pixRefresh));
    _imp->_centerViewerButton->setIcon(QIcon(pixCenterViewer));
    _imp->loopMode_Button->setIcon(QIcon(pixLoopMode));
    
    QIcon icClip;
    icClip.addPixmap(pixClipToProjectEnabled,QIcon::Normal,QIcon::On);
    icClip.addPixmap(pixClipToProjectDisabled,QIcon::Normal,QIcon::Off);
    _imp->_clipToProjectFormatButton->setIcon(icClip);
    
    QIcon icRoI;
    icRoI.addPixmap(pixViewerRoIEnabled,QIcon::Normal,QIcon::On);
    icRoI.addPixmap(pixViewerRoIDisabled,QIcon::Normal,QIcon::Off);
    _imp->_enableViewerRoI->setIcon(icRoI);
    
    QIcon icViewerRs;
    icViewerRs.addPixmap(pixViewerRs,QIcon::Normal,QIcon::Off);
    icViewerRs.addPixmap(pixViewerRsChecked,QIcon::Normal,QIcon::On);
    _imp->_activateRenderScale->setIcon(icViewerRs);
    
    
    _imp->_centerViewerButton->setToolTip(tr("Scales the image so it doesn't exceed the size of the viewer and centers it.")+
                                    "<p><b>" + tr("Keyboard shortcut") + ": F</b></p>");
    
    _imp->_clipToProjectFormatButton->setToolTip("<p>" + tr("Clips the portion of the image displayed "
                                           "on the viewer to the project format. "
                                           "When off, everything in the union of all nodes "
                                           "region of definition will be displayed.") + "</p>"
                                                 "<p><b>" + tr("Keyboard shortcut") + ": " + QKeySequence(Qt::SHIFT + Qt::Key_C).toString()+
                                                 "</b></p>");

    QKeySequence enableViewerKey(Qt::SHIFT + Qt::Key_W);
    _imp->_enableViewerRoI->setToolTip("<p>" + tr("When active, enables the region of interest that will limit"
                                 " the portion of the viewer that is kept updated.") + "</p>"
                                 "<p><b>" + tr("Keyboard shortcut:") + enableViewerKey.toString() + "</b></p>");
    /*=================================================*/
    
    /*frame seeker*/
    _imp->_timeLineGui = new TimeLineGui(_imp->app->getTimeLine(),_imp->_gui,this);
    _imp->_timeLineGui->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    _imp->_mainLayout->addWidget(_imp->_timeLineGui);
    /*================================================*/
    
    
    /*slots & signals*/
    boost::shared_ptr<Node> wrapperNode = _imp->_viewerNode->getNode();
    QObject::connect(wrapperNode.get(),SIGNAL(inputChanged(int)),this,SLOT(onInputChanged(int)));
    QObject::connect(wrapperNode.get(),SIGNAL(inputNameChanged(int,QString)),this,SLOT(onInputNameChanged(int,QString)));
    QObject::connect(_imp->_viewerNode,SIGNAL(activeInputsChanged()),this,SLOT(onActiveInputsChanged()));
    QObject::connect(_imp->_viewerNode,SIGNAL(imageFormatChanged(int,int,int)),this,SLOT(onImageFormatChanged(int,int,int)));
    
    QObject::connect(_imp->_viewerColorSpace, SIGNAL(currentIndexChanged(int)), this,
                     SLOT(onColorSpaceComboBoxChanged(int)));
    QObject::connect(_imp->_zoomCombobox, SIGNAL(currentIndexChanged(QString)),_imp->viewer, SLOT(zoomSlot(QString)));
    QObject::connect(_imp->viewer, SIGNAL(zoomChanged(int)), this, SLOT(updateZoomComboBox(int)));
    QObject::connect(_imp->_gainBox, SIGNAL(valueChanged(double)), this,SLOT(onGainSliderChanged(double)));
    QObject::connect(_imp->_gainSlider, SIGNAL(positionChanged(double)), _imp->_gainBox, SLOT(setValue(double)));
    QObject::connect(_imp->_gainSlider, SIGNAL(positionChanged(double)), this, SLOT(onGainSliderChanged(double)));
    QObject::connect(_imp->_gainBox, SIGNAL(valueChanged(double)), _imp->_gainSlider, SLOT(seekScalePosition(double)));
    QObject::connect(_imp->_currentFrameBox, SIGNAL(valueChanged(double)), this, SLOT(onCurrentTimeSpinBoxChanged(double)));
    
    VideoEngine* vengine = _imp->_viewerNode->getVideoEngine().get();
    assert(vengine);
    
    QObject::connect(_imp->play_Forward_Button,SIGNAL(clicked(bool)),this,SLOT(startPause(bool)));
    QObject::connect(_imp->stop_Button,SIGNAL(clicked()),this,SLOT(abortRendering()));
    QObject::connect(_imp->play_Backward_Button,SIGNAL(clicked(bool)),this,SLOT(startBackward(bool)));
    QObject::connect(_imp->previousFrame_Button,SIGNAL(clicked()),this,SLOT(previousFrame()));
    QObject::connect(_imp->nextFrame_Button,SIGNAL(clicked()),this,SLOT(nextFrame()));
    QObject::connect(_imp->previousIncrement_Button,SIGNAL(clicked()),this,SLOT(previousIncrement()));
    QObject::connect(_imp->nextIncrement_Button,SIGNAL(clicked()),this,SLOT(nextIncrement()));
    QObject::connect(_imp->firstFrame_Button,SIGNAL(clicked()),this,SLOT(firstFrame()));
    QObject::connect(_imp->lastFrame_Button,SIGNAL(clicked()),this,SLOT(lastFrame()));
    QObject::connect(_imp->loopMode_Button, SIGNAL(clicked(bool)), this, SLOT(toggleLoopMode(bool)));
    QObject::connect(_imp->nextKeyFrame_Button,SIGNAL(clicked(bool)),_imp->app->getTimeLine().get(), SLOT(goToNextKeyframe()));
    QObject::connect(_imp->previousKeyFrame_Button,SIGNAL(clicked(bool)),_imp->app->getTimeLine().get(), SLOT(goToPreviousKeyframe()));
    
    QObject::connect(timeline.get(),SIGNAL(frameChanged(SequenceTime,int)),
                     this, SLOT(onTimeLineTimeChanged(SequenceTime,int)));
    QObject::connect(timeline.get(),SIGNAL(boundariesChanged(SequenceTime,SequenceTime,int)),this,
                     SLOT(onTimelineBoundariesChanged(SequenceTime,SequenceTime,int)));

    QObject::connect(_imp->_refreshButton, SIGNAL(clicked()), this, SLOT(refresh()));
    
    QObject::connect(_imp->_centerViewerButton, SIGNAL(clicked()), this, SLOT(centerViewer()));
    QObject::connect(_imp->_viewerNode,SIGNAL(viewerDisconnected()),this,SLOT(disconnectViewer()));
    QObject::connect(_imp->fpsBox, SIGNAL(valueChanged(double)), vengine, SLOT(setDesiredFPS(double)));
    
    manageSlotsForInfoWidget(0,true);
    
    QObject::connect(vengine, SIGNAL(engineStarted(bool,int)), this, SLOT(onEngineStarted(bool,int)));
    QObject::connect(vengine, SIGNAL(engineStopped(int)), this, SLOT(onEngineStopped()));
    
    QObject::connect(_imp->_clipToProjectFormatButton,SIGNAL(clicked(bool)),this,SLOT(onClipToProjectButtonToggle(bool)));
    
    QObject::connect(_imp->_viewsComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(showView(int)));
    
    QObject::connect(_imp->_enableViewerRoI, SIGNAL(clicked(bool)), this, SLOT(onEnableViewerRoIButtonToggle(bool)));
    QObject::connect(_imp->_autoContrast,SIGNAL(clicked(bool)),this,SLOT(onAutoContrastChanged(bool)));
    QObject::connect(_imp->_autoConstrastLabel,SIGNAL(clicked(bool)),this,SLOT(onAutoContrastChanged(bool)));
    QObject::connect(_imp->_autoConstrastLabel,SIGNAL(clicked(bool)),_imp->_autoContrast,SLOT(setChecked(bool)));
    QObject::connect(_imp->_renderScaleCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(onRenderScaleComboIndexChanged(int)));
    QObject::connect(_imp->_activateRenderScale,SIGNAL(toggled(bool)),this,SLOT(onRenderScaleButtonClicked(bool)));
    
    connectToViewerCache();
    
    for (std::list<NodeGui*>::const_iterator it = existingRotoNodes.begin(); it!=existingRotoNodes.end(); ++it) {
        createRotoInterface(*it);
    }
    if (currentRoto && currentRoto->isSettingsPanelVisible()) {
        setRotoInterface(currentRoto);
    }
    
    for (std::list<NodeGui*>::const_iterator it = existingTrackerNodes.begin(); it!=existingTrackerNodes.end(); ++it) {
        createTrackerInterface(*it);
    }
    if (currentTracker && currentTracker->isSettingsPanelVisible()) {
        setRotoInterface(currentTracker);
    }
    
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

void
ViewerTab::onColorSpaceComboBoxChanged(int v)
{
    Natron::ViewerColorSpace colorspace;
    if (v == 0) {
        colorspace = Natron::Linear;
    } else if (1) {
        colorspace = Natron::sRGB;
    } else if (2) {
       colorspace = Natron::Rec709;
    } else {
        assert(false);
    }
    _imp->viewer->setLut((int)colorspace);
    _imp->_viewerNode->onColorSpaceChanged(colorspace);
}

void
ViewerTab::onEnableViewerRoIButtonToggle(bool b)
{
    _imp->_enableViewerRoI->setDown(b);
    _imp->viewer->setUserRoIEnabled(b);
}

void
ViewerTab::updateViewsMenu(int count)
{
    int currentIndex = _imp->_viewsComboBox->activeIndex();
    _imp->_viewsComboBox->clear();
    if(count == 1){
        _imp->_viewsComboBox->hide();
        _imp->_viewsComboBox->addItem(tr("Main"));
        
    }else if(count == 2){
        _imp->_viewsComboBox->show();
        _imp->_viewsComboBox->addItem(tr("Left"),QIcon(),QKeySequence(Qt::CTRL + Qt::Key_1));
        _imp->_viewsComboBox->addItem(tr("Right"),QIcon(),QKeySequence(Qt::CTRL + Qt::Key_2));
    }else{
        _imp->_viewsComboBox->show();
        for(int i = 0 ; i < count;++i){
            _imp->_viewsComboBox->addItem(QString(tr("View "))+QString::number(i+1),QIcon(),Gui::keySequenceForView(i));
        }
    }
    if(currentIndex < _imp->_viewsComboBox->count() && currentIndex != -1){
        _imp->_viewsComboBox->setCurrentIndex(currentIndex);
    }else{
        _imp->_viewsComboBox->setCurrentIndex(0);
    }
    _imp->_gui->updateViewsActions(count);
}

void
ViewerTab::setCurrentView(int view)
{
    _imp->_viewsComboBox->setCurrentIndex(view);
}
int
ViewerTab::getCurrentView() const
{
    QMutexLocker l(&_imp->_currentViewMutex);
    return _imp->_currentViewIndex;
}

void
ViewerTab::toggleLoopMode(bool b)
{
    _imp->loopMode_Button->setDown(b);
    _imp->_viewerNode->getVideoEngine()->toggleLoopMode(b);
}

void
ViewerTab::onClipToProjectButtonToggle(bool b)
{
    _imp->_clipToProjectFormatButton->setDown(b);
    _imp->viewer->setClipToDisplayWindow(b);
}

void
ViewerTab::onEngineStarted(bool forward,int frameCount)
{
    if (frameCount > 1) {
        _imp->play_Forward_Button->setChecked(forward);
        _imp->play_Forward_Button->setDown(forward);
        _imp->play_Backward_Button->setChecked(!forward);
        _imp->play_Backward_Button->setDown(!forward);
    }
}

void
ViewerTab::onEngineStopped()
{
    _imp->play_Forward_Button->setChecked(false);
    _imp->play_Forward_Button->setDown(false);
    _imp->play_Backward_Button->setChecked(false);
    _imp->play_Backward_Button->setDown(false);
}

void
ViewerTab::updateZoomComboBox(int value)
{
    assert(value > 0);
    QString str = QString::number(value);
    str.append(QChar('%'));
    str.prepend("  ");
    str.append("  ");
    _imp->_zoomCombobox->setCurrentText_no_emit(str);
}

/*In case they're several viewer around, we need to reset the dag and tell it
 explicitly we want to use this viewer and not another one.*/
void
ViewerTab::startPause(bool b)
{
    abortRendering();
    if(b){
        _imp->_viewerNode->getVideoEngine()->render(-1, /*frame count*/
                                                    true,/*seek timeline ?*/
                                                    true,/*rebuild tree?*/
                                                    true, /*forward ?*/
                                                    false,/*same frame ?*/
                                                    false);/*force preview?*/
    }
}

void
ViewerTab::abortRendering()
{
    ///Abort all viewers because they are all synchronised.
    const std::list<boost::shared_ptr<NodeGui> >& activeNodes = _imp->_gui->getNodeGraph()->getAllActiveNodes();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = activeNodes.begin(); it!=activeNodes.end(); ++it) {
        ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>((*it)->getNode()->getLiveInstance());
        if (isViewer) {
            isViewer->getVideoEngine()->abortRendering(false);
        }
    }
}

void
ViewerTab::startBackward(bool b)
{
    abortRendering();
    if(b){
        _imp->_viewerNode->getVideoEngine()->render(-1, /*frame count*/
                                                    true,/*seek timeline ?*/
                                                    true,/*rebuild tree?*/
                                                    false,/*forward?*/
                                                    false,/*same frame ?*/
                                                    false);/*force preview?*/
    }
}

void
ViewerTab::seek(SequenceTime time)
{
    _imp->_currentFrameBox->setValue(time);
    _imp->_timeLineGui->seek(time);
}

void
ViewerTab::previousFrame()
{
    seek(_imp->_timeLineGui->currentFrame()-1);
}

void
ViewerTab::nextFrame()
{
    seek(_imp->_timeLineGui->currentFrame()+1);
}

void
ViewerTab::previousIncrement()
{
    seek(_imp->_timeLineGui->currentFrame()-_imp->incrementSpinBox->value());
}

void
ViewerTab::nextIncrement()
{
    seek(_imp->_timeLineGui->currentFrame()+_imp->incrementSpinBox->value());
}

void
ViewerTab::firstFrame()
{
    seek(_imp->_timeLineGui->leftBound());
}

void
ViewerTab::lastFrame()
{
    seek(_imp->_timeLineGui->rightBound());
}

void
ViewerTab::onTimeLineTimeChanged(SequenceTime time,
                                 int /*reason*/)
{
    _imp->_currentFrameBox->setValue(time);
}

void
ViewerTab::onCurrentTimeSpinBoxChanged(double time)
{
    _imp->_timeLineGui->seek(time);
}

void
ViewerTab::centerViewer()
{
    _imp->viewer->fitImageToFormat();
    if(_imp->viewer->displayingImage()){
        _imp->_viewerNode->refreshAndContinueRender(false,true);
        
    }else{
        _imp->viewer->updateGL();
    }
}

void
ViewerTab::refresh()
{
    _imp->_viewerNode->forceFullComputationOnNextFrame();
    _imp->_viewerNode->updateTreeAndRender();
}

ViewerTab::~ViewerTab()
{
	if (_imp->_gui) {
		_imp->_viewerNode->invalidateUiContext();
		if (_imp->app && !_imp->app->isClosing() && _imp->_gui->getLastSelectedViewer() == this) {
			assert(_imp->_gui);
			_imp->_gui->setLastSelectedViewer(NULL);
		}
	}
    for (std::map<NodeGui*,RotoGui*>::iterator it = _imp->_rotoNodes.begin(); it!=_imp->_rotoNodes.end(); ++it) {
        delete it->second;
    }
    for (std::map<NodeGui*,TrackerGui*>::iterator it = _imp->_trackerNodes.begin(); it!=_imp->_trackerNodes.end(); ++it) {
        delete it->second;
    }
}

bool
ViewerTab::isPlayingForward() const
{
    return _imp->play_Forward_Button->isDown();
}

bool
ViewerTab::isPlayingBackward() const
{
    return _imp->play_Backward_Button->isDown();
}

void
ViewerTab::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Space && modCASIsNone(e)) {
        if (parentWidget()) {
            QKeyEvent* ev = new QKeyEvent(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier);
            QCoreApplication::postEvent(parentWidget(),ev);
        }

    } else if (e->key() == Qt::Key_Y && modCASIsNone(e)) {
        int currentIndex = _imp->_viewerChannels->activeIndex();
        if (currentIndex == 0) {
            _imp->_viewerChannels->setCurrentIndex(1);
        } else {
            _imp->_viewerChannels->setCurrentIndex(0);
        }

    } else if (e->key() == Qt::Key_R && modCASIsNone(e)) {
        int currentIndex = _imp->_viewerChannels->activeIndex();
        if (currentIndex == 2) {
            _imp->_viewerChannels->setCurrentIndex(1);
        } else {
            _imp->_viewerChannels->setCurrentIndex(2);
        }

    } else if (e->key() == Qt::Key_G && modCASIsNone(e)) {
        int currentIndex = _imp->_viewerChannels->activeIndex();
        if (currentIndex == 3) {
            _imp->_viewerChannels->setCurrentIndex(1);
        } else {
            _imp->_viewerChannels->setCurrentIndex(3);
        }

    } else if (e->key() == Qt::Key_B  && modCASIsNone(e)) {
        int currentIndex = _imp->_viewerChannels->activeIndex();
        if (currentIndex == 4) {
            _imp->_viewerChannels->setCurrentIndex(1);
        } else {
            _imp->_viewerChannels->setCurrentIndex(4);
        }

    } else if (e->key() == Qt::Key_A && modCASIsNone(e)) {
        int currentIndex = _imp->_viewerChannels->activeIndex();
        if (currentIndex == 5) {
            _imp->_viewerChannels->setCurrentIndex(1);
        } else {
            _imp->_viewerChannels->setCurrentIndex(5);
        }

    } else if (e->key() == Qt::Key_Left && modCASIsNone(e)) {
        previousFrame();

    } else if (e->key() == Qt::Key_J && modCASIsNone(e)) {
        startBackward(!_imp->play_Backward_Button->isDown());

    } else if (e->key() == Qt::Key_K && modCASIsNone(e)) {
        abortRendering();

    } else if (e->key() == Qt::Key_L && modCASIsNone(e)) {
        startPause(!_imp->play_Forward_Button->isDown());
        
    } else if (e->key() == Qt::Key_Right && modCASIsNone(e)) {
        nextFrame();
    } else if (e->key() == Qt::Key_Left && modCASIsShift(e)) {
        //prev incr
        previousIncrement();

    } else if (e->key() == Qt::Key_Right && modCASIsShift(e)) {
        //next incr
        nextIncrement();

    } else if (e->key() == Qt::Key_Left && modCASIsControl(e)) {
        //first frame
        firstFrame();

    } else if (e->key() == Qt::Key_Right && modCASIsControl(e)) {
        //last frame
        lastFrame();

    } else if (e->key() == Qt::Key_Left && modCASIsControlShift(e)) {
        //prev key
        _imp->app->getTimeLine()->goToPreviousKeyframe();

    } else if (e->key() == Qt::Key_Right && modCASIsControlShift(e)) {
        //next key
        _imp->app->getTimeLine()->goToNextKeyframe();

    } else if(e->key() == Qt::Key_F && modCASIsNone(e)) {
        centerViewer();
        
    } else if(e->key() == Qt::Key_C && modCASIsShift(e)) {
        onClipToProjectButtonToggle(!_imp->_clipToProjectFormatButton->isDown());

    } else if(e->key() == Qt::Key_U && modCASIsNone(e)) {
        refresh();

    } else if(e->key() == Qt::Key_W && modCASIsShift(e)) {
        onEnableViewerRoIButtonToggle(!_imp->_enableViewerRoI->isDown());

    } else if (e->key() == Qt::Key_P && modCASIsControl(e)) {
        onRenderScaleButtonClicked(!_imp->_renderScaleActive);

    } else if (e->key() == Qt::Key_1 && (modCASIsAlt(e) || modCASIsAltShift(e))) {
        // On some keyboards (e.g. French AZERTY), the number keys are shifted
        _imp->_renderScaleCombo->setCurrentIndex(0);

    } else if (e->key() == Qt::Key_2 && (modCASIsAlt(e) || modCASIsAltShift(e))) {
        // On some keyboards (e.g. French AZERTY), the number keys are shifted
        _imp->_renderScaleCombo->setCurrentIndex(1);

    } else if (e->key() == Qt::Key_3 && (modCASIsAlt(e) || modCASIsAltShift(e))) {
        // On some keyboards (e.g. French AZERTY), the number keys are shifted
        _imp->_renderScaleCombo->setCurrentIndex(2);

    } else if (e->key() == Qt::Key_4 && (modCASIsAlt(e) || modCASIsAltShift(e))) {
        // On some keyboards (e.g. French AZERTY), the number keys are shifted
      _imp->_renderScaleCombo->setCurrentIndex(3);

    } else if (e->key() == Qt::Key_5 && (modCASIsAlt(e) || modCASIsAltShift(e))) {
        // On some keyboards (e.g. French AZERTY), the number keys are shifted
       _imp->_renderScaleCombo->setCurrentIndex(4);
    }
}


bool
ViewerTab::isPlayForwardButtonDown() const
{
    return _imp->play_Forward_Button->isDown();
}

bool
ViewerTab::isPlayBackwardButtonDown() const
{
    return _imp->play_Backward_Button->isDown();
}

void
ViewerTab::onGainSliderChanged(double v)
{
    _imp->viewer->setGain(v);
    _imp->_viewerNode->onGainChanged(v);
}

void
ViewerTab::onViewerChannelsChanged(int i)
{
    ViewerInstance::DisplayChannels channels;
    switch (i) {
        case 0:
            channels = ViewerInstance::LUMINANCE;
            break;
        case 1:
            channels = ViewerInstance::RGB;
            break;
        case 2:
            channels = ViewerInstance::R;
            break;
        case 3:
            channels = ViewerInstance::G;
            break;
        case 4:
            channels = ViewerInstance::B;
            break;
        case 5:
            channels = ViewerInstance::A;
            break;
        default:
            channels = ViewerInstance::RGB;
            break;
    }
    _imp->_viewerNode->setDisplayChannels(channels);
}

bool
ViewerTab::eventFilter(QObject *target, QEvent* e)
{
    if (e->type() == QEvent::MouseButtonPress) {
        if (_imp->_gui && _imp->app) {
            _imp->_gui->selectNode(_imp->app->getNodeGui(_imp->_viewerNode->getNode()));
        }
        
    }
    return QWidget::eventFilter(target, e);
}

void
ViewerTab::disconnectViewer()
{
    _imp->viewer->disconnectViewer();
}

QSize
ViewerTab::minimumSizeHint() const
{
    return QWidget::minimumSizeHint();
}

QSize ViewerTab::sizeHint() const
{
    return QWidget::sizeHint();
}

void
ViewerTab::showView(int view)
{
    QMutexLocker l(&_imp->_currentViewMutex);
    _imp->_currentViewIndex = view;
    abortRendering();
    bool isAutoPreview = _imp->app->getProject()->isAutoPreviewEnabled();
    _imp->_viewerNode->refreshAndContinueRender(isAutoPreview,true);
}


void
ViewerTab::drawOverlays(double scaleX,double scaleY) const
{
    if (!_imp->app || _imp->app->isClosing()) {
        return;
    }

    if (_imp->_currentRoto.second && _imp->_currentRoto.first->isSettingsPanelVisible()) {
        _imp->_currentRoto.second->drawOverlays(scaleX, scaleY);
    }
    
    if (_imp->_currentTracker.second && _imp->_currentTracker.first->isSettingsPanelVisible()) {
        _imp->_currentTracker.second->drawOverlays(scaleX, scaleY);
    }
    
    const std::list<boost::shared_ptr<NodeGui> >& nodes = getGui()->getNodeGraph()->getAllActiveNodes();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin();it!=nodes.end();++it) {
        if ((*it)->shouldDrawOverlay()) {
            Natron::EffectInstance* effect = (*it)->getNode()->getLiveInstance();
            assert(effect);
            effect->setCurrentViewportForOverlays(_imp->viewer);
            effect->drawOverlay_public(scaleX,scaleY);
        }
    }
}

bool
ViewerTab::notifyOverlaysPenDown(double scaleX,
                                 double scaleY,
                                 const QPointF& viewportPos,
                                 const QPointF& pos,
                                 QMouseEvent* e)
{
    bool didSomething = false;
    if (!_imp->app || _imp->app->isClosing()) {
        return false;
    }
    
    const std::list<boost::shared_ptr<NodeGui> >& nodes = getGui()->getNodeGraph()->getAllActiveNodes();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin();it!=nodes.end();++it) {
        if ((*it)->shouldDrawOverlay()) {
            Natron::EffectInstance* effect = (*it)->getNode()->getLiveInstance();
            assert(effect);
            
            effect->setCurrentViewportForOverlays(_imp->viewer);
            bool didSmthing = effect->onOverlayPenDown_public(scaleX,scaleY,viewportPos, pos);
            if (didSmthing) {
                //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html
                // if the instance returns kOfxStatOK, the host should not pass the pen motion
                // to any other interactive object it may own that shares the same view.
                return true;
            }
        }
    }
    
    if (_imp->_currentTracker.second && _imp->_currentTracker.first->isSettingsPanelVisible()) {
        if (_imp->_currentTracker.second->penDown(scaleX, scaleY,viewportPos,pos,e)) {
            return true;
        }
    }

    if (_imp->_currentRoto.second && _imp->_currentRoto.first->isSettingsPanelVisible()) {
        if (_imp->_currentRoto.second->penDown(scaleX, scaleY,viewportPos,pos,e)) {
            didSomething  = true;
        }
    }

    return didSomething;
}

bool
ViewerTab::notifyOverlaysPenDoubleClick(double scaleX,
                                        double scaleY,
                                        const QPointF& viewportPos,
                                        const QPointF& pos,
                                        QMouseEvent* e)
{
    if (!_imp->app || _imp->app->isClosing()) {
        return false;
    }
    
    if (_imp->_currentRoto.second && _imp->_currentRoto.first->isSettingsPanelVisible()) {
        if (_imp->_currentRoto.second->penDoubleClicked(scaleX, scaleY, viewportPos, pos, e)) {
            return true;
        }
    }
    
    if (_imp->_currentTracker.second && _imp->_currentTracker.first->isSettingsPanelVisible()) {
        if (_imp->_currentTracker.second->penDoubleClicked(scaleX, scaleY, viewportPos, pos, e)) {
            return true;
        }
    }
    
    return false;
}

bool
ViewerTab::notifyOverlaysPenMotion(double scaleX,
                                   double scaleY,
                                   const QPointF& viewportPos,
                                   const QPointF& pos,
                                   QMouseEvent* e)
{
    bool didSomething = false;
    if (!_imp->app || _imp->app->isClosing()) {
        return false;
    }
   
    const std::list<boost::shared_ptr<NodeGui> >& nodes = getGui()->getNodeGraph()->getAllActiveNodes();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin();it!=nodes.end();++it) {
        if ((*it)->shouldDrawOverlay()) {
            Natron::EffectInstance* effect = (*it)->getNode()->getLiveInstance();
            assert(effect);
            effect->setCurrentViewportForOverlays(_imp->viewer);
            bool didSmthing = effect->onOverlayPenMotion_public(scaleX,scaleY,viewportPos, pos);
            if (didSmthing) {
                //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html
                // if the instance returns kOfxStatOK, the host should not pass the pen motion
                // to any other interactive object it may own that shares the same view.
                return true;
            }
        }
    }
    
    if (_imp->_currentTracker.second && _imp->_currentTracker.first->isSettingsPanelVisible()) {
        if (_imp->_currentTracker.second->penMotion(scaleX, scaleY, viewportPos, pos, e)) {
            return true;
        }
    }
    
    if (_imp->_currentRoto.second && _imp->_currentRoto.first->isSettingsPanelVisible()) {
        if (_imp->_currentRoto.second->penMotion(scaleX, scaleY, viewportPos, pos, e)) {
            didSomething = true;
        }
    }

    return didSomething;
}

bool
ViewerTab::notifyOverlaysPenUp(double scaleX,
                               double scaleY,
                               const QPointF& viewportPos,
                               const QPointF& pos,
                               QMouseEvent* e)
{
    bool didSomething = false;
    if (!_imp->app || _imp->app->isClosing()) {
        return false;
    }
    
    const std::list<boost::shared_ptr<NodeGui> >& nodes = getGui()->getNodeGraph()->getAllActiveNodes();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin();it!=nodes.end();++it) {
        if ((*it)->shouldDrawOverlay()) {
            Natron::EffectInstance* effect = (*it)->getNode()->getLiveInstance();
            assert(effect);
            effect->setCurrentViewportForOverlays(_imp->viewer);
            bool didSmthing = effect->onOverlayPenUp_public(scaleX,scaleY,viewportPos, pos);
            if (didSmthing) {
                //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html
                // if the instance returns kOfxStatOK, the host should not pass the pen motion
                // to any other interactive object it may own that shares the same view.
                return true;
            }
        }
    }
    
    if (_imp->_currentTracker.second && _imp->_currentTracker.first->isSettingsPanelVisible()) {
        if (_imp->_currentTracker.second->penUp(scaleX, scaleY, viewportPos, pos, e)) {
            return true;
        }
    }
    
    if (_imp->_currentRoto.second && _imp->_currentRoto.first->isSettingsPanelVisible()) {
        if (_imp->_currentRoto.second->penUp(scaleX, scaleY, viewportPos, pos, e)) {
            didSomething  =  true;
        }
    }

    return didSomething ;
}

bool
ViewerTab::notifyOverlaysKeyDown(double scaleX,
                                 double scaleY,
                                 QKeyEvent* e)
{
    bool didSomething = false;
    if (!_imp->app || _imp->app->isClosing()) {
        return false;
    }
    
    Natron::Key natronKey = QtEnumConvert::fromQtKey((Qt::Key)e->key());
    Natron::KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers(e->modifiers());
    const std::list<boost::shared_ptr<NodeGui> >& nodes = getGui()->getNodeGraph()->getAllActiveNodes();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin();
         it!=nodes.end();
         ++it) {
        if ((*it)->shouldDrawOverlay()) {
            Natron::EffectInstance* effect = (*it)->getNode()->getLiveInstance();
            assert(effect);
            effect->setCurrentViewportForOverlays(_imp->viewer);
            bool didSmthing = effect->onOverlayKeyDown_public(scaleX,scaleY,natronKey,natronMod);
            if (didSmthing) {
                //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html
                // if the instance returns kOfxStatOK, the host should not pass the pen motion
                // to any other interactive object it may own that shares the same view.
                return true;
            }
        }
    }
    
    if (_imp->_currentTracker.second && _imp->_currentTracker.first->isSettingsPanelVisible()) {
        if (_imp->_currentTracker.second->keyDown(scaleX, scaleY, e)) {
            return true;
        }
    }

    
    if (_imp->_currentRoto.second && _imp->_currentRoto.first->isSettingsPanelVisible()) {
        if (_imp->_currentRoto.second->keyDown(scaleX, scaleY, e)) {
            didSomething = true;
        }
    }
    
   
    return didSomething;
}

bool
ViewerTab::notifyOverlaysKeyUp(double scaleX,
                               double scaleY,
                               QKeyEvent* e)
{
    bool didSomething = false;
    if (!_imp->app || _imp->app->isClosing()) {
        return false;
    }
    
   
    const std::list<boost::shared_ptr<NodeGui> >& nodes = getGui()->getNodeGraph()->getAllActiveNodes();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin();it!=nodes.end();++it) {
        if ((*it)->shouldDrawOverlay()) {
            Natron::EffectInstance* effect = (*it)->getNode()->getLiveInstance();
            assert(effect);
            
            effect->setCurrentViewportForOverlays(_imp->viewer);
            bool didSmthing = effect->onOverlayKeyUp_public(scaleX,scaleY,
                                                            QtEnumConvert::fromQtKey((Qt::Key)e->key()),QtEnumConvert::fromQtModifiers(e->modifiers()));
            if (didSmthing) {
                //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html
                // if the instance returns kOfxStatOK, the host should not pass the pen motion
                // to any other interactive object it may own that shares the same view.
                return true;
            }
        }
    }
    
    if (_imp->_currentTracker.second && _imp->_currentTracker.first->isSettingsPanelVisible()) {
        if (_imp->_currentTracker.second->keyUp(scaleX, scaleY, e)) {
            return true;
        }
    }
    
    if (_imp->_currentRoto.second && _imp->_currentRoto.first->isSettingsPanelVisible()) {
        if (_imp->_currentRoto.second->keyUp(scaleX, scaleY, e)) {
            didSomething = true;
        }
    }

    return didSomething;
}

bool
ViewerTab::notifyOverlaysKeyRepeat(double scaleX,
                                   double scaleY,
                                   QKeyEvent* e)
{
    if (!_imp->app || _imp->app->isClosing()) {
        return false;
    }
    const std::list<boost::shared_ptr<NodeGui> >& nodes = getGui()->getNodeGraph()->getAllActiveNodes();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin();it!=nodes.end();++it) {
        if ((*it)->shouldDrawOverlay()) {
            Natron::EffectInstance* effect = (*it)->getNode()->getLiveInstance();
            assert(effect);
            
            effect->setCurrentViewportForOverlays(_imp->viewer);
            bool didSmthing = effect->onOverlayKeyRepeat_public(scaleX,scaleY,
                                        QtEnumConvert::fromQtKey((Qt::Key)e->key()),QtEnumConvert::fromQtModifiers(e->modifiers()));
            if (didSmthing) {
                //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html
                // if the instance returns kOfxStatOK, the host should not pass the pen motion
                // to any other interactive object it may own that shares the same view.
                return true;
            }
        }
    }

    //if (_imp->_currentTracker.second && _imp->_currentTracker.first->isSettingsPanelVisible()) {
    //    if (_imp->_currentTracker.second->loseFocus(scaleX, scaleY,e)) {
    //        return true;
    //    }
    //}

    if (_imp->_currentRoto.second && _imp->_currentRoto.first->isSettingsPanelVisible()) {
        if (_imp->_currentRoto.second->keyRepeat(scaleX, scaleY, e)) {
            return  true;
        }
    }

    return false;
}

bool
ViewerTab::notifyOverlaysFocusGained(double scaleX,
                                     double scaleY)
{
    if (!_imp->app || _imp->app->isClosing()) {
        return false;
    }
    bool ret = false;
    const std::list<boost::shared_ptr<NodeGui> >& nodes = getGui()->getNodeGraph()->getAllActiveNodes();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin();it!=nodes.end();++it) {
        if ((*it)->shouldDrawOverlay()) {
            Natron::EffectInstance* effect = (*it)->getNode()->getLiveInstance();
            assert(effect);
            
            effect->setCurrentViewportForOverlays(_imp->viewer);
            bool didSmthing = effect->onOverlayFocusGained_public(scaleX,scaleY);
            if (didSmthing) {
                ret = true;
            }
        }
    }

    //if (_imp->_currentTracker.second && _imp->_currentTracker.first->isSettingsPanelVisible()) {
    //    if (_imp->_currentTracker.second->gainFocus(scaleX, scaleY)) {
    //        ret = true;
    //    }
    //}

    //if (_imp->_currentRoto.second && _imp->_currentRoto.first->isSettingsPanelVisible()) {
    //    if (_imp->_currentRoto.second->gainFocus(scaleX, scaleY)) {
    //        ret = true;
    //    }
    //}

    return ret;
}

bool
ViewerTab::notifyOverlaysFocusLost(double scaleX,
                                   double scaleY)
{
    if (!_imp->app || _imp->app->isClosing()) {
        return false;
    }
    bool ret = false;
    const std::list<boost::shared_ptr<NodeGui> >& nodes = getGui()->getNodeGraph()->getAllActiveNodes();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodes.begin();it!=nodes.end();++it) {
        if ((*it)->shouldDrawOverlay()) {
            Natron::EffectInstance* effect = (*it)->getNode()->getLiveInstance();
            assert(effect);
            
            effect->setCurrentViewportForOverlays(_imp->viewer);
            bool didSmthing = effect->onOverlayFocusLost_public(scaleX,scaleY);
            if (didSmthing) {
                ret = true;
            }
        }
    }

    if (_imp->_currentTracker.second && _imp->_currentTracker.first->isSettingsPanelVisible()) {
        if (_imp->_currentTracker.second->loseFocus(scaleX, scaleY)) {
            return true;
        }
    }

    //if (_imp->_currentRoto.second && _imp->_currentRoto.first->isSettingsPanelVisible()) {
    //    if (_imp->_currentRoto.second->loseFocus(scaleX, scaleY)) {
    //        didSomething = true;
    //    }
    //}

    return ret;
}

bool
ViewerTab::isClippedToProject() const
{
    return _imp->viewer->isClippingImageToProjectWindow();
}

std::string
ViewerTab::getColorSpace() const
{
    Natron::ViewerColorSpace lut = (Natron::ViewerColorSpace)_imp->_viewerNode->getLutType();
    switch (lut) {
        case Natron::Linear:
            return "Linear(None)";
            break;
        case Natron::sRGB:
            return "sRGB";
            break;
        case Natron::Rec709:
            return "Rec.709";
            break;
        default:
            return "";
            break;
    }
}

void
ViewerTab::setUserRoIEnabled(bool b)
{
    onEnableViewerRoIButtonToggle(b);
}

bool
ViewerTab::isAutoContrastEnabled() const
{
    return _imp->_viewerNode->isAutoContrastEnabled();
}

void
ViewerTab::setAutoContrastEnabled(bool b)
{
    _imp->_autoContrast->setChecked(b);
    _imp->_gainSlider->setEnabled(!b);
    _imp->_gainBox->setEnabled(!b);
    _imp->_viewerNode->onAutoContrastChanged(b,true);
}

void
ViewerTab::setUserRoI(const RectD& r)
{
    _imp->viewer->setUserRoI(r);
}

void
ViewerTab::setClipToProject(bool b)
{
    onClipToProjectButtonToggle(b);
}

void
ViewerTab::setColorSpace(const std::string& colorSpaceName)
{
    int index = _imp->_viewerColorSpace->itemIndex(colorSpaceName.c_str());
    if (index != -1) {
        _imp->_viewerColorSpace->setCurrentIndex(index);
    }
}

void
ViewerTab::setGain(double d)
{
    _imp->_gainBox->setValue(d);
    _imp->_gainSlider->seekScalePosition(d);
    _imp->_viewerNode->onGainChanged(d);
}

double
ViewerTab::getGain() const
{
    return _imp->_viewerNode->getGain();
}

void
ViewerTab::setMipMapLevel(int level)
{
    if (level > 0) {
        _imp->_renderScaleCombo->setCurrentIndex(level - 1);
    }

    _imp->_viewerNode->onMipMapLevelChanged(level);
}

int
ViewerTab::getMipMapLevel() const
{
    return _imp->_viewerNode->getMipMapLevel();
}

void
ViewerTab::setRenderScaleActivated(bool act)
{
    onRenderScaleButtonClicked(act);
}

bool
ViewerTab::getRenderScaleActivated() const
{
    return _imp->_viewerNode->getMipMapLevel() != 0;
}

void
ViewerTab::setZoomOrPannedSinceLastFit(bool enabled)
{
    _imp->viewer->setZoomOrPannedSinceLastFit(enabled);
}

bool
ViewerTab::getZoomOrPannedSinceLastFit() const
{
    return _imp->viewer->getZoomOrPannedSinceLastFit();
}

std::string
ViewerTab::getChannelsString() const
{
    ViewerInstance::DisplayChannels c = _imp->_viewerNode->getChannels();
    switch (c) {
        case ViewerInstance::RGB:
            return "RGB";
        case ViewerInstance::R:
            return "R";
        case ViewerInstance::G:
            return "G";
        case ViewerInstance::B:
            return "B";
        case ViewerInstance::A:
            return "A";
        case ViewerInstance::LUMINANCE:
            return "Luminance";
            break;
        default:
            return "";
    }
}

void
ViewerTab::setChannels(const std::string& channelsStr)
{
    int index = _imp->_viewerChannels->itemIndex(channelsStr.c_str());
    if (index != -1) {
        _imp->_viewerChannels->setCurrentIndex(index);
    }
}

ViewerGL*
ViewerTab::getViewer() const
{
    return _imp->viewer;
}

ViewerInstance*
ViewerTab::getInternalNode() const
{
    return _imp->_viewerNode;
}

Gui*
ViewerTab::getGui() const
{
    return _imp->_gui;
}

void
ViewerTab::enterEvent(QEvent*)
{
    setFocus();
}

void
ViewerTab::onAutoContrastChanged(bool b)
{
    _imp->_gainSlider->setEnabled(!b);
    _imp->_gainBox->setEnabled(!b);
    _imp->_viewerNode->onAutoContrastChanged(b,b);
    if (!b) {
        _imp->_viewerNode->onGainChanged(_imp->_gainBox->value()) ;
    }
}

void
ViewerTab::onRenderScaleComboIndexChanged(int index)
{
    int level;
    if (_imp->_renderScaleActive) {
        level = index + 1;
    } else {
        level = 0;
    }
    _imp->_viewerNode->onMipMapLevelChanged(level);
}

void
ViewerTab::onRenderScaleButtonClicked(bool checked)
{
    _imp->_activateRenderScale->blockSignals(true);
    _imp->_renderScaleActive = checked;
    _imp->_activateRenderScale->setDown(checked);
    _imp->_activateRenderScale->setChecked(checked);
    _imp->_activateRenderScale->blockSignals(false);
    onRenderScaleComboIndexChanged(_imp->_renderScaleCombo->activeIndex());
}

void
ViewerTab::setInfoBarResolution(const Format& f)
{
    _imp->_infosWidget[0]->setResolution(f);
    _imp->_infosWidget[1]->setResolution(f);
}

void
ViewerTab::createTrackerInterface(NodeGui* n)
{
    boost::shared_ptr<MultiInstancePanel> multiPanel = n->getMultiInstancePanel();
    boost::shared_ptr<TrackerPanel> trackPanel = boost::dynamic_pointer_cast<TrackerPanel>(multiPanel);
    assert(trackPanel);
    TrackerGui* tracker = new TrackerGui(trackPanel,this);
    std::pair<std::map<NodeGui*,TrackerGui*>::iterator,bool> ret = _imp->_trackerNodes.insert(std::make_pair(n,tracker));
    assert(ret.second);
    if (!ret.second) {
        qDebug() << "ViewerTab::createTrackerInterface() failed";
        delete tracker;
        return;
    }
    QObject::connect(n,SIGNAL(settingsPanelClosed(bool)),this,SLOT(onTrackerNodeGuiSettingsPanelClosed(bool)));
    if (n->isSettingsPanelVisible()) {
        setTrackerInterface(n);
    } else {
        tracker->getButtonsBar()->hide();
    }
}

void
ViewerTab::setTrackerInterface(NodeGui* n)
{
    assert(n);
    std::map<NodeGui*,TrackerGui*>::iterator it = _imp->_trackerNodes.find(n);
    if (it != _imp->_trackerNodes.end()) {
        if (_imp->_currentTracker.first == n) {
            return;
        }
        
        ///remove any existing tracker gui
        if (_imp->_currentTracker.first != NULL) {
            removeTrackerInterface(_imp->_currentTracker.first, false,true);
        }
        
        ///Add the widgets
        
        ///if there's a current roto add it before it
        int index;
        if (_imp->_currentRoto.second) {
            index = _imp->_mainLayout->indexOf(_imp->_currentRoto.second->getCurrentButtonsBar());
            assert(index != -1);
        } else {
            index = _imp->_mainLayout->indexOf(_imp->_viewerContainer);
        }
        
        assert(index >= 0);
        QWidget* buttonsBar = it->second->getButtonsBar();
        _imp->_mainLayout->insertWidget(index,buttonsBar);
        buttonsBar->show();
        
        _imp->_currentTracker.first = n;
        _imp->_currentTracker.second = it->second;
        _imp->viewer->redraw();
    }

}

void
ViewerTab::removeTrackerInterface(NodeGui* n,bool permanently,bool removeAndDontSetAnother)
{
    
    std::map<NodeGui*,TrackerGui*>::iterator it = _imp->_trackerNodes.find(n);
    if (it != _imp->_trackerNodes.end()) {
        if (!_imp->_gui) {
            if (permanently) {
                delete it->second;
            }
            return;
        }
        
        if (_imp->_currentTracker.first == n) {
            ///Remove the widgets of the current tracker node
            
            int buttonsBarIndex = _imp->_mainLayout->indexOf(_imp->_currentTracker.second->getButtonsBar());
            assert(buttonsBarIndex >= 0);
            QLayoutItem* buttonsBar = _imp->_mainLayout->itemAt(buttonsBarIndex);
            assert(buttonsBar);
            _imp->_mainLayout->removeItem(buttonsBar);
            buttonsBar->widget()->hide();
            
            if (!removeAndDontSetAnother) {
                ///If theres another tracker node, set it as the current tracker interface
                std::map<NodeGui*,TrackerGui*>::iterator newTracker = _imp->_trackerNodes.end();
                for (std::map<NodeGui*,TrackerGui*>::iterator it2 = _imp->_trackerNodes.begin(); it2 != _imp->_trackerNodes.end(); ++it2) {
                    if (it2->second != it->second && it2->first->isSettingsPanelVisible()) {
                        newTracker = it2;
                        break;
                    }
                }
                
                _imp->_currentTracker.first = 0;
                _imp->_currentTracker.second = 0;
                
                if (newTracker != _imp->_trackerNodes.end()) {
                    setTrackerInterface(newTracker->first);
                }
            }
        }
        
        if (permanently) {
            delete it->second;
            _imp->_trackerNodes.erase(it);
        }
    }

}

void
ViewerTab::createRotoInterface(NodeGui* n)
{
    RotoGui* roto = new RotoGui(n,this,getRotoGuiSharedData(n));
    QObject::connect(roto,SIGNAL(selectedToolChanged(int)),_imp->_gui,SLOT(onRotoSelectedToolChanged(int)));
    std::pair<std::map<NodeGui*,RotoGui*>::iterator,bool> ret = _imp->_rotoNodes.insert(std::make_pair(n,roto));
    assert(ret.second);
    if (!ret.second) {
        qDebug() << "ViewerTab::createRotoInterface() failed";
        delete roto;
        return;
    }
    QObject::connect(n,SIGNAL(settingsPanelClosed(bool)),this,SLOT(onRotoNodeGuiSettingsPanelClosed(bool)));
    if (n->isSettingsPanelVisible()) {
        setRotoInterface(n);
    } else {
        roto->getToolBar()->hide();
        roto->getCurrentButtonsBar()->hide();
    }
}


void
ViewerTab::setRotoInterface(NodeGui* n)
{
    assert(n);
    std::map<NodeGui*,RotoGui*>::iterator it = _imp->_rotoNodes.find(n);
    if (it != _imp->_rotoNodes.end()) {
        if (_imp->_currentRoto .first == n) {
            return;
        }
        
        ///remove any existing roto gui
        if (_imp->_currentRoto.first != NULL) {
            removeRotoInterface(_imp->_currentRoto.first, false,true);
        }
        
        ///Add the widgets
        QToolBar* toolBar = it->second->getToolBar();
        _imp->_viewerLayout->insertWidget(0, toolBar);
        toolBar->show();
        
        ///If there's a tracker add it right after the tracker
        int index;
        if (_imp->_currentTracker.second) {
            index = _imp->_mainLayout->indexOf(_imp->_currentTracker.second->getButtonsBar());
            assert(index != -1);
            ++index;
        } else {
            index = _imp->_mainLayout->indexOf(_imp->_viewerContainer);
        }
        assert(index >= 0);
        QWidget* buttonsBar = it->second->getCurrentButtonsBar();
        _imp->_mainLayout->insertWidget(index,buttonsBar);
        buttonsBar->show();
        
        QObject::connect(it->second,SIGNAL(roleChanged(int,int)),this,SLOT(onRotoRoleChanged(int,int)));
        _imp->_currentRoto.first = n;
        _imp->_currentRoto.second = it->second;
        _imp->viewer->redraw();
    }
    
}

void
ViewerTab::removeRotoInterface(NodeGui* n,
                               bool permanently,
                               bool removeAndDontSetAnother)
{
    std::map<NodeGui*,RotoGui*>::iterator it = _imp->_rotoNodes.find(n);
    if (it != _imp->_rotoNodes.end()) {
        
        if (_imp->_currentRoto.first == n) {
            QObject::disconnect(_imp->_currentRoto.second,SIGNAL(roleChanged(int,int)),this,SLOT(onRotoRoleChanged(int,int)));
            ///Remove the widgets of the current roto node
            assert(_imp->_viewerLayout->count() > 1);
            QLayoutItem* currentToolBarItem = _imp->_viewerLayout->itemAt(0);
            QToolBar* currentToolBar = qobject_cast<QToolBar*>(currentToolBarItem->widget());
            currentToolBar->hide();
            assert(currentToolBar == _imp->_currentRoto.second->getToolBar());
            _imp->_viewerLayout->removeItem(currentToolBarItem);
            int buttonsBarIndex = _imp->_mainLayout->indexOf(_imp->_currentRoto.second->getCurrentButtonsBar());
            assert(buttonsBarIndex >= 0);
            QLayoutItem* buttonsBar = _imp->_mainLayout->itemAt(buttonsBarIndex);
            assert(buttonsBar);
            _imp->_mainLayout->removeItem(buttonsBar);
            buttonsBar->widget()->hide();
            
            if (!removeAndDontSetAnother) {
                ///If theres another roto node, set it as the current roto interface
                std::map<NodeGui*,RotoGui*>::iterator newRoto = _imp->_rotoNodes.end();
                for (std::map<NodeGui*,RotoGui*>::iterator it2 = _imp->_rotoNodes.begin(); it2 != _imp->_rotoNodes.end(); ++it2) {
                    if (it2->second != it->second && it2->first->isSettingsPanelVisible()) {
                        newRoto = it2;
                        break;
                    }
                }
                
                _imp->_currentRoto.first = 0;
                _imp->_currentRoto.second = 0;
                
                if (newRoto != _imp->_rotoNodes.end()) {
                    setRotoInterface(newRoto->first);
                }
            }

        }
    
        if (permanently) {
            delete it->second;
            _imp->_rotoNodes.erase(it);
        }
    }
}

void
ViewerTab::getRotoContext(std::map<NodeGui*,RotoGui*>* rotoNodes,
                          std::pair<NodeGui*,RotoGui*>* currentRoto) const
{
    *rotoNodes = _imp->_rotoNodes;
    *currentRoto = _imp->_currentRoto;
}

void
ViewerTab::getTrackerContext(std::map<NodeGui*,TrackerGui*>* trackerNodes,
                             std::pair<NodeGui*,TrackerGui*>* currentTracker) const
{
    *trackerNodes = _imp->_trackerNodes;
    *currentTracker = _imp->_currentTracker;
}

void
ViewerTab::onRotoRoleChanged(int previousRole,
                             int newRole)
{
    RotoGui* roto = qobject_cast<RotoGui*>(sender());
    if (roto) {
        assert(roto == _imp->_currentRoto.second);
        
        ///Remove the previous buttons bar
        int buttonsBarIndex = _imp->_mainLayout->indexOf(_imp->_currentRoto.second->getButtonsBar((RotoGui::Roto_Role)previousRole));
        assert(buttonsBarIndex >= 0);
        _imp->_mainLayout->removeItem(_imp->_mainLayout->itemAt(buttonsBarIndex));
        
        
        ///Set the new buttons bar
        int viewerIndex = _imp->_mainLayout->indexOf(_imp->_viewerContainer);
        assert(viewerIndex >= 0);
        _imp->_mainLayout->insertWidget(viewerIndex, _imp->_currentRoto.second->getButtonsBar((RotoGui::Roto_Role)newRole));
    }
}

void
ViewerTab::updateRotoSelectedTool(int tool,
                                  RotoGui* sender)
{
    if (_imp->_currentRoto.second && _imp->_currentRoto.second != sender) {
        _imp->_currentRoto.second->setCurrentTool((RotoGui::Roto_Tool)tool,false);
    }
}

boost::shared_ptr<RotoGuiSharedData>
ViewerTab::getRotoGuiSharedData(NodeGui* node) const
{
    std::map<NodeGui*,RotoGui*>::const_iterator found = _imp->_rotoNodes.find(node);
    if (found == _imp->_rotoNodes.end()) {
        return boost::shared_ptr<RotoGuiSharedData>();
    } else {
        return found->second->getRotoGuiSharedData();
    }
}

void
ViewerTab::onRotoEvaluatedForThisViewer()
{
    _imp->_gui->onViewerRotoEvaluated(this);
}

void
ViewerTab::onRotoNodeGuiSettingsPanelClosed(bool closed)
{
    NodeGui* n = qobject_cast<NodeGui*>(sender());
    if (n) {
        if (closed) {
            removeRotoInterface(n, false,false);
        } else {
            if (n != _imp->_currentRoto.first) {
                setRotoInterface(n);
            }
        }
    }
}

void
ViewerTab::onTrackerNodeGuiSettingsPanelClosed(bool closed)
{
    NodeGui* n = qobject_cast<NodeGui*>(sender());
    if (n) {
        if (closed) {
            removeTrackerInterface(n, false,false);
        } else {
            if (n != _imp->_currentTracker.first) {
                setTrackerInterface(n);
            }
        }
    }
}

void
ViewerTab::notifyAppClosing()
{
    _imp->_gui = 0;
    _imp->app = 0;
}

void
ViewerTab::onCompositingOperatorIndexChanged(int index)
{
    ViewerCompositingOperator newOp,oldOp;
    {
        QMutexLocker l(&_imp->compOperatorMutex);
        oldOp = _imp->_compOperator;
        switch (index) {
            case 0:
                _imp->_compOperator = OPERATOR_NONE;
                _imp->_secondInputImage->setEnabled_natron(false);
                manageSlotsForInfoWidget(1, false);
                _imp->_infosWidget[1]->hide();
                break;
            case 1:
                _imp->_compOperator = OPERATOR_OVER;
                break;
            case 2:
                _imp->_compOperator = OPERATOR_UNDER;
                break;
            case 3:
                _imp->_compOperator = OPERATOR_MINUS;
                break;
            case 4:
                _imp->_compOperator = OPERATOR_WIPE;
                break;
            default:
                break;
        }
        newOp = _imp->_compOperator;

    }
    if (oldOp == OPERATOR_NONE && newOp != OPERATOR_NONE) {
        _imp->viewer->resetWipeControls();
    }

    if (_imp->_compOperator != OPERATOR_NONE && !_imp->_secondInputImage->isEnabled()) {
        _imp->_secondInputImage->setEnabled_natron(true);
        manageSlotsForInfoWidget(1, true);
        _imp->_infosWidget[1]->show();
    } else if (_imp->_compOperator == OPERATOR_NONE) {
        _imp->_secondInputImage->setEnabled_natron(false);
        manageSlotsForInfoWidget(1, false);
        _imp->_infosWidget[1]->hide();
    }
    

    _imp->viewer->updateGL();
}

void
ViewerTab::setCompositingOperator(Natron::ViewerCompositingOperator op)
{
    int comboIndex ;
    switch (op) {
        case Natron::OPERATOR_NONE:
            comboIndex = 0;
            break;
        case Natron::OPERATOR_OVER:
            comboIndex = 1;
            break;
        case Natron::OPERATOR_UNDER:
            comboIndex = 2;
            break;
        case Natron::OPERATOR_MINUS:
            comboIndex = 3;
            break;
        case Natron::OPERATOR_WIPE:
            comboIndex = 4;
            break;
        default:
            break;
    }
    {
        QMutexLocker l(&_imp->compOperatorMutex);
        _imp->_compOperator = op;
    }
    _imp->_compositingOperator->setCurrentIndex_no_emit(comboIndex);
    _imp->viewer->updateGL();
}

ViewerCompositingOperator
ViewerTab::getCompositingOperator() const
{
    QMutexLocker l(&_imp->compOperatorMutex);
    return _imp->_compOperator;
}

///Called when the user change the combobox choice
void
ViewerTab::onFirstInputNameChanged(const QString& text)
{
    int inputIndex = -1;
    for (InputNamesMap::iterator it = _imp->_inputNamesMap.begin(); it!= _imp->_inputNamesMap.end(); ++it) {
        if (it->second.name == text) {
            inputIndex = it->first;
            break;
        }
    }
    _imp->_viewerNode->setInputA(inputIndex);
    _imp->_viewerNode->refreshAndContinueRender(false,true);
}

///Called when the user change the combobox choice
void
ViewerTab::onSecondInputNameChanged(const QString& text)
{
    int inputIndex = -1;
    for (InputNamesMap::iterator it = _imp->_inputNamesMap.begin(); it!= _imp->_inputNamesMap.end(); ++it) {
        if (it->second.name == text) {
            inputIndex = it->first;
            break;
        }
    }
    _imp->_viewerNode->setInputB(inputIndex);
    if (inputIndex == -1) {
        manageSlotsForInfoWidget(1, false);
        //setCompositingOperator(Natron::OPERATOR_NONE);
        _imp->_infosWidget[1]->hide();
    } else {
        if (!_imp->_infosWidget[1]->isVisible()) {
            _imp->_infosWidget[1]->show();
            manageSlotsForInfoWidget(1, true);
            _imp->_secondInputImage->setEnabled_natron(true);
            if (_imp->_compOperator == Natron::OPERATOR_NONE) {
                _imp->viewer->resetWipeControls();
                setCompositingOperator(Natron::OPERATOR_WIPE);
            }
        }
    }
    _imp->_viewerNode->refreshAndContinueRender(false,true);
}

///This function is called only when the user changed inputs on the node graph
void
ViewerTab::onActiveInputsChanged()
{
    int activeInputs[2];
    _imp->_viewerNode->getActiveInputs(activeInputs[0], activeInputs[1]);
    InputNamesMap::iterator foundA = _imp->_inputNamesMap.find(activeInputs[0]);
    if (foundA != _imp->_inputNamesMap.end()) {
        int indexInA = _imp->_firstInputImage->itemIndex(foundA->second.name);
        assert(indexInA != -1);
        _imp->_firstInputImage->setCurrentIndex_no_emit(indexInA);
    } else {
        _imp->_firstInputImage->setCurrentIndex_no_emit(0);
    }
    
    InputNamesMap::iterator foundB = _imp->_inputNamesMap.find(activeInputs[1]);
    if (foundB != _imp->_inputNamesMap.end()) {
        int indexInB = _imp->_secondInputImage->itemIndex(foundB->second.name);

        assert(indexInB != -1);
        _imp->_secondInputImage->setCurrentIndex_no_emit(indexInB);
        if (!_imp->_infosWidget[1]->isVisible()) {
            _imp->_infosWidget[1]->show();
            _imp->_secondInputImage->setEnabled_natron(true);
            manageSlotsForInfoWidget(1, true);
        }

    } else {
        _imp->_secondInputImage->setCurrentIndex_no_emit(0);
        //setCompositingOperator(Natron::OPERATOR_NONE);
        manageSlotsForInfoWidget(1, false);
        _imp->_infosWidget[1]->hide();
        //_imp->_secondInputImage->setEnabled_natron(false);
    }
    
    if ((activeInputs[0] == -1 || activeInputs[1] == -1) //only 1 input is valid
        && getCompositingOperator() != OPERATOR_NONE) {
        //setCompositingOperator(OPERATOR_NONE);
        _imp->_infosWidget[1]->hide();
        manageSlotsForInfoWidget(1, false);
        // _imp->_secondInputImage->setEnabled_natron(false);
    } else if (activeInputs[0] != -1 && activeInputs[1] != -1 && activeInputs[0] != activeInputs[1]
               && getCompositingOperator() == OPERATOR_NONE) {
        _imp->viewer->resetWipeControls();
        setCompositingOperator(Natron::OPERATOR_WIPE);
        
    }
}

void
ViewerTab::onInputChanged(int inputNb)
{
    ///rebuild the name maps
    EffectInstance* inp = _imp->_viewerNode->input(inputNb);
    if (inp) {
        InputNamesMap::iterator found = _imp->_inputNamesMap.find(inputNb);
        if (found != _imp->_inputNamesMap.end()) {
            const std::string& curInputName = found->second.input->getName();
            found->second.input = inp;
            int indexInA = _imp->_firstInputImage->itemIndex(curInputName.c_str());
            int indexInB = _imp->_secondInputImage->itemIndex(curInputName.c_str());
            assert(indexInA != -1 && indexInB != -1);
            found->second.name = inp->getName().c_str();
            _imp->_firstInputImage->setItemText(indexInA, found->second.name);
            _imp->_secondInputImage->setItemText(indexInB, found->second.name);
        } else {
            InputName inpName;
            inpName.input = inp;
            inpName.name = inp->getName().c_str();
            _imp->_inputNamesMap.insert(std::make_pair(inputNb,inpName));
            _imp->_firstInputImage->addItem(inpName.name);
            _imp->_secondInputImage->addItem(inpName.name);
        }
    } else {
        InputNamesMap::iterator found = _imp->_inputNamesMap.find(inputNb);
        
        ///The input has been disconnected
        if (found != _imp->_inputNamesMap.end()) {
            const std::string& curInputName = found->second.input->getName();
            _imp->_firstInputImage->blockSignals(true);
            _imp->_secondInputImage->blockSignals(true);
            _imp->_firstInputImage->removeItem(curInputName.c_str());
            _imp->_secondInputImage->removeItem(curInputName.c_str());
            _imp->_firstInputImage->blockSignals(false);
            _imp->_secondInputImage->blockSignals(false);
            _imp->_inputNamesMap.erase(found);
        }
    }
    _imp->viewer->clearPersistentMessage();
}

void
ViewerTab::onInputNameChanged(int inputNb,
                              const QString& name)
{
    InputNamesMap::iterator found = _imp->_inputNamesMap.find(inputNb);
    assert(found != _imp->_inputNamesMap.end());
    int indexInA = _imp->_firstInputImage->itemIndex(found->second.name);
    int indexInB = _imp->_secondInputImage->itemIndex(found->second.name);
    assert(indexInA != -1 && indexInB != -1);
    _imp->_firstInputImage->setItemText(indexInA, name);
    _imp->_secondInputImage->setItemText(indexInB, name);
    found->second.name = name;
    
}

void
ViewerTab::manageSlotsForInfoWidget(int textureIndex,
                                    bool connect)
{
    VideoEngine* vengine = _imp->_viewerNode->getVideoEngine().get();
    if (connect) {
        QObject::connect(vengine, SIGNAL(fpsChanged(double,double)), _imp->_infosWidget[textureIndex], SLOT(setFps(double,double)));
        QObject::connect(vengine,SIGNAL(engineStopped(int)),_imp->_infosWidget[textureIndex],SLOT(hideFps()));

    } else {
        QObject::disconnect(vengine, SIGNAL(fpsChanged(double,double)), _imp->_infosWidget[textureIndex], SLOT(setFps(double,double)));
        QObject::disconnect(vengine,SIGNAL(engineStopped(int)),_imp->_infosWidget[textureIndex],SLOT(hideFps()));
    }
}

void
ViewerTab::onImageFormatChanged(int texIndex,
                                int components,
                                int bitdepth)
{
    _imp->_infosWidget[texIndex]->setImageFormat((ImageComponents)components, (ImageBitDepth)bitdepth);
}

void
ViewerTab::onFrameRangeEditingFinished()
{
    QString text = _imp->frameRangeEdit->text();
    ///try to parse the frame range, if failed set it back to what the timeline currently is
    int i = 0;
    QString firstStr;
    while (i < text.size() && text.at(i).isDigit()) {
        firstStr.push_back(text.at(i));
        ++i;
    }
    
    ///advance the marker to the second digit if any
    while (i < text.size() && !text.at(i).isDigit()) {
        ++i;
    }
    
    boost::shared_ptr<TimeLine> timeline = _imp->app->getTimeLine();
    
    bool ok;
    int first = firstStr.toInt(&ok);
    if (!ok) {
        QString text = QString("%1 - %2").arg(timeline->leftBound()).arg(timeline->rightBound());
        _imp->frameRangeEdit->setText(text);
        _imp->frameRangeEdit->adjustSize();
        return;
    }
    
    if (i == text.size()) {
        ///there's no second marker, set the timeline's boundaries to be the same frame
        timeline->setFrameRange(first, first);
    } else {
        QString secondStr;
        while (i < text.size() && text.at(i).isDigit()) {
            secondStr.push_back(text.at(i));
            ++i;
        }
        int second = secondStr.toInt(&ok);
        if (!ok) {
            ///there's no second marker, set the timeline's boundaries to be the same frame
            timeline->setFrameRange(first, first);
        } else {
            timeline->setFrameRange(first,second);
        }
    }
    _imp->frameRangeEdit->adjustSize();
}

void
ViewerTab::onLockFrameRangeButtonClicked(bool toggled)
{
    _imp->lockFrameRangeButton->setDown(toggled);
    _imp->frameRangeEdit->setReadOnly(toggled);
    {
        QMutexLocker l(&_imp->frameRangeLockedMutex);
        _imp->frameRangeLocked = toggled;
    }
}

void
ViewerTab::setFrameRangeLocked(bool toggled)
{
    _imp->lockFrameRangeButton->setChecked(toggled);
    onLockFrameRangeButtonClicked(toggled);
}

void
ViewerTab::onTimelineBoundariesChanged(SequenceTime first,
                                       SequenceTime second,
                                       int /*reason*/)
{
    QString text = QString("%1 - %2").arg(first).arg(second);
    _imp->frameRangeEdit->setText(text);
    _imp->frameRangeEdit->adjustSize();
}

bool
ViewerTab::isFrameRangeLocked() const
{
    QMutexLocker l(&_imp->frameRangeLockedMutex);
    return _imp->frameRangeLocked;
}

void
ViewerTab::connectToViewerCache()
{
    _imp->_timeLineGui->connectSlotsToViewerCache();
}

void
ViewerTab::disconnectFromViewerCache()
{
    _imp->_timeLineGui->disconnectSlotsFromViewerCache();
}

void
ViewerTab::clearTimelineCacheLine()
{
    if (_imp->_timeLineGui) {
        _imp->_timeLineGui->clearCachedFrames();
    }
}
