//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "ViewerTab.h"
#include "ViewerTabPrivate.h"

#include <cassert>

#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QAction>
#include <QVBoxLayout>
#include <QCheckBox>

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/OutputSchedulerThread.h" // RenderEngine
#include "Engine/Project.h"
#include "Engine/ViewerInstance.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/Button.h"
#include "Gui/ChannelsComboBox.h"
#include "Gui/ClickableLabel.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/InfoViewerWidget.h"
#include "Gui/Label.h"
#include "Gui/NodeGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TimeLineGui.h"
#include "Gui/Utils.h"
#include "Gui/ViewerGL.h"


using namespace Natron;


static void makeFullyQualifiedLabel(Natron::Node* node,std::string* ret)
{
    boost::shared_ptr<NodeCollection> parent = node->getGroup();
    NodeGroup* isParentGrp = dynamic_cast<NodeGroup*>(parent.get());
    std::string toPreprend = node->getLabel();
    if (isParentGrp) {
        toPreprend.insert(0, "/");
    }
    ret->insert(0, toPreprend);
    if (isParentGrp) {
        makeFullyQualifiedLabel(isParentGrp->getNode().get(), ret);
    }
}

ViewerTab::ViewerTab(const std::list<NodeGui*> & existingRotoNodes,
                     NodeGui* currentRoto,
                     const std::list<NodeGui*> & existingTrackerNodes,
                     NodeGui* currentTracker,
                     Gui* gui,
                     ViewerInstance* node,
                     QWidget* parent)
    : QWidget(parent)
    , ScriptObject()
      , _imp( new ViewerTabPrivate(gui,node) )
{
    installEventFilter(this);
    
    std::string nodeName =  node->getNode()->getFullyQualifiedName();
    for (std::size_t i = 0; i < nodeName.size(); ++i) {
        if (nodeName[i] == '.') {
            nodeName[i] = '_';
        }
    }
    setScriptName(nodeName);
    std::string label;
    makeFullyQualifiedLabel(node->getNode().get(), &label);
    setLabel(label);
    
    NodePtr internalNode = node->getNode();
    QObject::connect(internalNode.get(), SIGNAL(scriptNameChanged(QString)), this, SLOT(onInternalNodeScriptNameChanged(QString)));
    QObject::connect(internalNode.get(), SIGNAL(labelChanged(QString)), this, SLOT(onInternalNodeLabelChanged(QString)));
    
    _imp->mainLayout = new QVBoxLayout(this);
    setLayout(_imp->mainLayout);
    _imp->mainLayout->setSpacing(0);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);

    /*VIEWER SETTINGS*/

    /*1st row of buttons*/
    _imp->firstSettingsRow = new QWidget(this);
    _imp->firstRowLayout = new QHBoxLayout(_imp->firstSettingsRow);
    _imp->firstSettingsRow->setLayout(_imp->firstRowLayout);
    _imp->firstRowLayout->setContentsMargins(0, 0, 0, 0);
    _imp->firstRowLayout->setSpacing(0);
    _imp->mainLayout->addWidget(_imp->firstSettingsRow);

    _imp->layerChoice = new ComboBox(_imp->firstSettingsRow);
    _imp->layerChoice->setToolTip("<p><b>" + tr("Layer:") + "</b></p><p>"
                                  + tr("The layer that the Viewer node will fetch upstream in the tree. "
                                       "The channels of the layer will be mapped to the RGBA channels of the viewer according to "
                                       "its number of channels. (e.g: UV would be mapped to RG)") + "</p>");
    QObject::connect(_imp->layerChoice,SIGNAL(currentIndexChanged(int)),this,SLOT(onLayerComboChanged(int)));
    _imp->firstRowLayout->addWidget(_imp->layerChoice);
    
    _imp->alphaChannelChoice = new ComboBox(_imp->firstSettingsRow);
    _imp->alphaChannelChoice->setToolTip("<p><b>" + tr("Alpha channel:") + "</b></p><p>"
                                         + tr("Select here a channel of any layer that will be used when displaying the "
                                              "alpha channel with the <b>Channels</b> choice on the right.") + "</p>");
    QObject::connect(_imp->alphaChannelChoice,SIGNAL(currentIndexChanged(int)),this,SLOT(onAlphaChannelComboChanged(int)));
    _imp->firstRowLayout->addWidget(_imp->alphaChannelChoice);

    _imp->viewerChannels = new ChannelsComboBox(_imp->firstSettingsRow);
    _imp->viewerChannels->setToolTip( "<p><b>" + tr("Channels:") + "</b></p><p>"
                                       + tr("The channels to display on the viewer.") + "</p>");
    _imp->firstRowLayout->addWidget(_imp->viewerChannels);
    
    QAction* lumiAction = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDActionLuminance, tr("Luminance"), _imp->viewerChannels);
    QAction* rgbAction = new QAction(QIcon(), tr("RGB"), _imp->viewerChannels);
    QAction* rAction = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDActionRed, tr("Red"), _imp->viewerChannels);
    QAction* gAction = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDActionGreen, tr("Green"), _imp->viewerChannels);
    QAction* bAction = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDActionBlue, tr("Blue"), _imp->viewerChannels);
    QAction* aAction = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDActionAlpha, tr("Alpha"), _imp->viewerChannels);

    _imp->viewerChannels->addAction(lumiAction);
    _imp->viewerChannels->addAction(rgbAction);
    _imp->viewerChannels->addAction(rAction);
    _imp->viewerChannels->addAction(gAction);
    _imp->viewerChannels->addAction(bAction);
    _imp->viewerChannels->addAction(aAction);
    _imp->viewerChannels->setCurrentIndex(1);
    QObject::connect( _imp->viewerChannels, SIGNAL( currentIndexChanged(int) ), this, SLOT( onViewerChannelsChanged(int) ) );

    _imp->zoomCombobox = new ComboBox(_imp->firstSettingsRow);
    _imp->zoomCombobox->setToolTip( "<p><b>" + tr("Zoom:") + "</b></p>"
                                     + tr("The zoom applied to the image on the viewer.") + "</p>");

   
    // Keyboard shortcuts should be made visible to the user, not only in the shortcut editor, but also at logical places in the GUI.

    ActionWithShortcut* fitAction = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDActionFitViewer, "Fit", this);
    ActionWithShortcut* zoomInAction = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDActionZoomIn, "+", this);
    ActionWithShortcut* zoomOutAction = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDActionZoomOut, "-", this);
    ActionWithShortcut* level100Action = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDActionZoomLevel100, "100%", this);
    _imp->zoomCombobox->addAction(fitAction);
    _imp->zoomCombobox->addAction(zoomInAction);
    _imp->zoomCombobox->addAction(zoomOutAction);
    _imp->zoomCombobox->addSeparator();
    _imp->zoomCombobox->addItem("10%");
    _imp->zoomCombobox->addItem("25%");
    _imp->zoomCombobox->addItem("50%");
    _imp->zoomCombobox->addItem("75%");
    _imp->zoomCombobox->addAction(level100Action);
    _imp->zoomCombobox->addItem("125%");
    _imp->zoomCombobox->addItem("150%");
    _imp->zoomCombobox->addItem("200%");
    _imp->zoomCombobox->addItem("400%");
    _imp->zoomCombobox->addItem("800%");
    _imp->zoomCombobox->addItem("1600%");
    _imp->zoomCombobox->addItem("2400%");
    _imp->zoomCombobox->addItem("3200%");
    _imp->zoomCombobox->addItem("6400%");
    _imp->zoomCombobox->setMaximumWidthFromText("100000%");

    _imp->firstRowLayout->addWidget(_imp->zoomCombobox);
    
    QPixmap lockEnabled,lockDisabled;
    appPTR->getIcon(Natron::NATRON_PIXMAP_LOCKED, &lockEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_UNLOCKED, &lockDisabled);
    
    QIcon lockIcon;
    lockIcon.addPixmap(lockEnabled, QIcon::Normal, QIcon::On);
    lockIcon.addPixmap(lockDisabled, QIcon::Normal, QIcon::Off);
    _imp->syncViewerButton = new Button(lockIcon,"",_imp->firstSettingsRow);
    _imp->syncViewerButton->setCheckable(true);
    _imp->syncViewerButton->setToolTip(Natron::convertFromPlainText(tr("When enabled, all viewers will be synchronized to the same portion of the image in the viewport."),Qt::WhiteSpaceNormal));
    _imp->syncViewerButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE);
    _imp->syncViewerButton->setFocusPolicy(Qt::NoFocus);
    QObject::connect(_imp->syncViewerButton, SIGNAL(clicked(bool)), this,SLOT(onSyncViewersButtonPressed(bool)));
    _imp->firstRowLayout->addWidget(_imp->syncViewerButton);

    _imp->centerViewerButton = new Button(_imp->firstSettingsRow);
    _imp->centerViewerButton->setFocusPolicy(Qt::NoFocus);
    _imp->centerViewerButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->firstRowLayout->addWidget(_imp->centerViewerButton);


    _imp->clipToProjectFormatButton = new Button(_imp->firstSettingsRow);
    _imp->clipToProjectFormatButton->setFocusPolicy(Qt::NoFocus);
    _imp->clipToProjectFormatButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->clipToProjectFormatButton->setCheckable(true);
    _imp->clipToProjectFormatButton->setChecked(true);
    _imp->clipToProjectFormatButton->setDown(true);
    _imp->firstRowLayout->addWidget(_imp->clipToProjectFormatButton);

    _imp->enableViewerRoI = new Button(_imp->firstSettingsRow);
    _imp->enableViewerRoI->setFocusPolicy(Qt::NoFocus);
    _imp->enableViewerRoI->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->enableViewerRoI->setCheckable(true);
    _imp->enableViewerRoI->setChecked(false);
    _imp->enableViewerRoI->setDown(false);
    _imp->firstRowLayout->addWidget(_imp->enableViewerRoI);

    _imp->refreshButton = new Button(_imp->firstSettingsRow);
    _imp->refreshButton->setFocusPolicy(Qt::NoFocus);
    _imp->refreshButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    setTooltipWithShortcut(kShortcutGroupViewer, kShortcutIDActionRefresh, "<p>" + tr("Forces a new render of the current frame.") +
                           "</p>", _imp->refreshButton);

    _imp->firstRowLayout->addWidget(_imp->refreshButton);

    _imp->activateRenderScale = new Button(_imp->firstSettingsRow);
    _imp->activateRenderScale->setFocusPolicy(Qt::NoFocus);
    _imp->activateRenderScale->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    setTooltipWithShortcut(kShortcutGroupViewer, kShortcutIDActionProxyEnabled,
                           "<p><b>" + tr("Proxy mode:") + "</b></p><p>" +
                           tr("Activates the downscaling by the amount indicated by the value on the right. "
                              "The rendered images are degraded and as a result of this the whole rendering pipeline "
                            "is much faster.") + "</p>",_imp->activateRenderScale);

    _imp->activateRenderScale->setCheckable(true);
    _imp->activateRenderScale->setChecked(false);
    _imp->activateRenderScale->setDown(false);
    _imp->firstRowLayout->addWidget(_imp->activateRenderScale);

    _imp->renderScaleCombo = new ComboBox(_imp->firstSettingsRow);
    _imp->renderScaleCombo->setFocusPolicy(Qt::NoFocus);
    _imp->renderScaleCombo->setToolTip(Natron::convertFromPlainText(tr("When proxy mode is activated, it scales down the rendered image by this factor "
                                            "to accelerate the rendering."), Qt::WhiteSpaceNormal));
    
    QAction* proxy2 = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDActionProxyLevel2, "2", _imp->renderScaleCombo);
    QAction* proxy4 = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDActionProxyLevel4, "4", _imp->renderScaleCombo);
    QAction* proxy8 = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDActionProxyLevel8, "8", _imp->renderScaleCombo);
    QAction* proxy16 = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDActionProxyLevel16, "16", _imp->renderScaleCombo);
    QAction* proxy32 = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDActionProxyLevel32, "32", _imp->renderScaleCombo);
    _imp->renderScaleCombo->addAction(proxy2);
    _imp->renderScaleCombo->addAction(proxy4);
    _imp->renderScaleCombo->addAction(proxy8);
    _imp->renderScaleCombo->addAction(proxy16);
    _imp->renderScaleCombo->addAction(proxy32);
    _imp->firstRowLayout->addWidget(_imp->renderScaleCombo);

    _imp->firstRowLayout->addStretch();

    _imp->firstInputLabel = new Natron::Label("A:",_imp->firstSettingsRow);
    _imp->firstRowLayout->addWidget(_imp->firstInputLabel);

    _imp->firstInputImage = new ComboBox(_imp->firstSettingsRow);
    _imp->firstInputImage->addItem(" - ");
    QObject::connect( _imp->firstInputImage,SIGNAL( currentIndexChanged(QString) ),this,SLOT( onFirstInputNameChanged(QString) ) );

    _imp->firstRowLayout->addWidget(_imp->firstInputImage);

    _imp->compositingOperator = new ComboBox(_imp->firstSettingsRow);
    QObject::connect( _imp->compositingOperator,SIGNAL( currentIndexChanged(int) ),this,SLOT( onCompositingOperatorIndexChanged(int) ) );
    _imp->compositingOperator->addItem(tr(" - "), QIcon(), QKeySequence(), tr("Only the A input is used."));
    _imp->compositingOperator->addItem(tr("Over"), QIcon(), QKeySequence(), tr("A + B(1 - Aalpha)"));
    _imp->compositingOperator->addItem(tr("Under"), QIcon(), QKeySequence(), tr("A(1 - Balpha) + B"));
    _imp->compositingOperator->addItem(tr("Minus"), QIcon(), QKeySequence(), tr("A - B"));
    ActionWithShortcut* actionWipe = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDToggleWipe, "Wipe", _imp->compositingOperator);
    actionWipe->setToolTip(tr("Wipe between A and B"));
    _imp->compositingOperator->addAction(actionWipe);
    _imp->firstRowLayout->addWidget(_imp->compositingOperator);

    _imp->secondInputLabel = new Natron::Label("B:",_imp->firstSettingsRow);
    _imp->firstRowLayout->addWidget(_imp->secondInputLabel);

    _imp->secondInputImage = new ComboBox(_imp->firstSettingsRow);
    QObject::connect( _imp->secondInputImage,SIGNAL( currentIndexChanged(QString) ),this,SLOT( onSecondInputNameChanged(QString) ) );
    _imp->secondInputImage->addItem(" - ");
    _imp->firstRowLayout->addWidget(_imp->secondInputImage);

    _imp->firstRowLayout->addStretch();

    /*2nd row of buttons*/
    _imp->secondSettingsRow = new QWidget(this);
    _imp->secondRowLayout = new QHBoxLayout(_imp->secondSettingsRow);
    _imp->secondSettingsRow->setLayout(_imp->secondRowLayout);
    _imp->secondRowLayout->setSpacing(0);
    _imp->secondRowLayout->setContentsMargins(0, 0, 0, 0);
    _imp->mainLayout->addWidget(_imp->secondSettingsRow);
    
    QPixmap gainEnabled,gainDisabled;
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_GAIN_ENABLED,&gainEnabled);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_GAIN_DISABLED,&gainDisabled);
    QIcon gainIc;
    gainIc.addPixmap(gainEnabled,QIcon::Normal,QIcon::On);
    gainIc.addPixmap(gainDisabled,QIcon::Normal,QIcon::Off);
    _imp->toggleGainButton = new Button(gainIc,"",_imp->secondSettingsRow);
    _imp->toggleGainButton->setCheckable(true);
    _imp->toggleGainButton->setChecked(false);
    _imp->toggleGainButton->setDown(false);
    _imp->toggleGainButton->setFocusPolicy(Qt::NoFocus);
    _imp->toggleGainButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->toggleGainButton->setToolTip(Natron::convertFromPlainText(tr("Switch between \"neutral\" 1.0 gain f-stop and the previous setting."), Qt::WhiteSpaceNormal));
    _imp->secondRowLayout->addWidget(_imp->toggleGainButton);
    QObject::connect(_imp->toggleGainButton, SIGNAL(clicked(bool)), this, SLOT(onGainToggled(bool)));
    
    _imp->gainBox = new SpinBox(_imp->secondSettingsRow,SpinBox::eSpinBoxTypeDouble);
    QString gainTt =  "<p><b>" + tr("Gain:") + "</b></p><p>" + tr("Gain is shown as f-stops. The image is multipled by pow(2,value) before display.") + "</p>";
    _imp->gainBox->setToolTip(gainTt);
    _imp->gainBox->setIncrement(0.1);
    _imp->gainBox->setValue(0.0);
    _imp->secondRowLayout->addWidget(_imp->gainBox);


    _imp->gainSlider = new ScaleSliderQWidget(-6, 6, 0.0,ScaleSliderQWidget::eDataTypeDouble,_imp->gui,Natron::eScaleTypeLinear,_imp->secondSettingsRow);
    QObject::connect(_imp->gainSlider, SIGNAL(editingFinished(bool)), this, SLOT(onGainSliderEditingFinished(bool)));
    _imp->gainSlider->setToolTip(gainTt);
    _imp->secondRowLayout->addWidget(_imp->gainSlider);

    QString autoContrastToolTip( "<p><b>" + tr("Auto-contrast:") + "</b></p><p>" + tr(
                                     "Automatically adjusts the gain and the offset applied "
                                     "to the colors of the visible image portion on the viewer.") + "</p>");
    _imp->autoConstrastLabel = new ClickableLabel(tr("Auto-contrast:"),_imp->secondSettingsRow);
    _imp->autoConstrastLabel->setToolTip(autoContrastToolTip);
    _imp->secondRowLayout->addWidget(_imp->autoConstrastLabel);

    _imp->autoContrast = new QCheckBox(_imp->secondSettingsRow);
    _imp->autoContrast->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    _imp->autoContrast->setToolTip(autoContrastToolTip);
    _imp->secondRowLayout->addWidget(_imp->autoContrast);
    
    QPixmap gammaEnabled,gammaDisabled;
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_GAMMA_ENABLED,&gammaEnabled);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_GAMMA_DISABLED,&gammaDisabled);
    QIcon gammaIc;
    gammaIc.addPixmap(gammaEnabled,QIcon::Normal,QIcon::On);
    gammaIc.addPixmap(gammaDisabled,QIcon::Normal,QIcon::Off);
    _imp->toggleGammaButton = new Button(gammaIc,"",_imp->secondSettingsRow);
    QObject::connect(_imp->toggleGammaButton, SIGNAL(clicked(bool)), this,SLOT(onGammaToggled(bool)));
    _imp->toggleGammaButton->setCheckable(true);
    _imp->toggleGammaButton->setChecked(false);
    _imp->toggleGammaButton->setDown(false);
    _imp->toggleGammaButton->setFocusPolicy(Qt::NoFocus);
    _imp->toggleGammaButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->toggleGammaButton->setToolTip(Natron::convertFromPlainText(tr("Switch between gamma at 1.0 and the previous setting"), Qt::WhiteSpaceNormal));
    _imp->secondRowLayout->addWidget(_imp->toggleGammaButton);
    
    _imp->gammaBox = new SpinBox(_imp->secondSettingsRow, SpinBox::eSpinBoxTypeDouble);
    QString gammaTt = Natron::convertFromPlainText(tr("Gamma correction. It is applied after gain and before colorspace correction"), Qt::WhiteSpaceNormal);
    _imp->gammaBox->setToolTip(gammaTt);
    QObject::connect(_imp->gammaBox,SIGNAL(valueChanged(double)), this, SLOT(onGammaSpinBoxValueChanged(double)));
    _imp->gammaBox->setValue(1.0);
    _imp->secondRowLayout->addWidget(_imp->gammaBox);
    
    _imp->gammaSlider = new ScaleSliderQWidget(0,4,1.0,ScaleSliderQWidget::eDataTypeDouble,_imp->gui,Natron::eScaleTypeLinear,_imp->secondSettingsRow);
    QObject::connect(_imp->gammaSlider, SIGNAL(editingFinished(bool)), this, SLOT(onGammaSliderEditingFinished(bool)));
    _imp->gammaSlider->setToolTip(gammaTt);
    QObject::connect(_imp->gammaSlider,SIGNAL(positionChanged(double)), this, SLOT(onGammaSliderValueChanged(double)));
    _imp->secondRowLayout->addWidget(_imp->gammaSlider);

    _imp->viewerColorSpace = new ComboBox(_imp->secondSettingsRow);
    _imp->viewerColorSpace->setToolTip( "<p><b>" + tr("Viewer color process:") + "</b></p><p>" + tr(
                                             "The operation applied to the image before it is displayed "
                                             "on screen. All the color pipeline "
                                             "is linear, thus the process converts from linear "
                                             "to your monitor's colorspace.") + "</p>");
    _imp->secondRowLayout->addWidget(_imp->viewerColorSpace);

    _imp->viewerColorSpace->addItem("Linear(None)");
    _imp->viewerColorSpace->addItem("sRGB");
    _imp->viewerColorSpace->addItem("Rec.709");
    _imp->viewerColorSpace->setCurrentIndex(1);
    
    QPixmap pixCheckerboardEnabled,pixCheckerboardDisabld;
    appPTR->getIcon(Natron::NATRON_PIXMAP_VIEWER_CHECKERBOARD_ENABLED, &pixCheckerboardEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_VIEWER_CHECKERBOARD_DISABLED, &pixCheckerboardDisabld);
    QIcon icCk;
    icCk.addPixmap(pixCheckerboardEnabled,QIcon::Normal,QIcon::On);
    icCk.addPixmap(pixCheckerboardDisabld,QIcon::Normal,QIcon::Off);
    _imp->checkerboardButton = new Button(icCk,"",_imp->secondSettingsRow);
    _imp->checkerboardButton->setFocusPolicy(Qt::NoFocus);
    _imp->checkerboardButton->setCheckable(true); 
    _imp->checkerboardButton->setChecked(false);
    _imp->checkerboardButton->setDown(false);
    _imp->checkerboardButton->setToolTip(Natron::convertFromPlainText(tr("If checked, the viewer draws a checkerboard under the image instead of black "
                                                                     "(within the project window only)."), Qt::WhiteSpaceNormal));
    _imp->checkerboardButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QObject::connect(_imp->checkerboardButton,SIGNAL(clicked(bool)),this,SLOT(onCheckerboardButtonClicked()));
    _imp->secondRowLayout->addWidget(_imp->checkerboardButton);

    _imp->viewsComboBox = new ComboBox(_imp->secondSettingsRow);
    _imp->viewsComboBox->setToolTip( "<p><b>" + tr("Active view:") + "</b></p>" + tr(
                                          "Tells the viewer what view should be displayed.") );
    _imp->secondRowLayout->addWidget(_imp->viewsComboBox);
    _imp->viewsComboBox->hide();
    int viewsCount = _imp->app->getProject()->getProjectViewsCount(); //getProjectViewsCount
    updateViewsMenu(viewsCount);

    _imp->secondRowLayout->addStretch();

    /*=============================================*/

    /*OpenGL viewer*/
    _imp->viewerContainer = new QWidget(this);
    _imp->viewerLayout = new QHBoxLayout(_imp->viewerContainer);
    _imp->viewerLayout->setContentsMargins(0, 0, 0, 0);
    _imp->viewerLayout->setSpacing(0);

    _imp->viewerSubContainer = new QWidget(_imp->viewerContainer);
    //_imp->viewerSubContainer->setStyleSheet("background-color:black");
    _imp->viewerSubContainerLayout = new QVBoxLayout(_imp->viewerSubContainer);
    _imp->viewerSubContainerLayout->setContentsMargins(0, 0, 0, 0);
    _imp->viewerSubContainerLayout->setSpacing(1);


    _imp->viewer = new ViewerGL(this);

    _imp->viewerSubContainerLayout->addWidget(_imp->viewer);

    
    /*info bbox & color*/
    QString inputNames[2] = {
        "A:", "B:"
    };
    for (int i = 0; i < 2; ++i) {
        _imp->infoWidget[i] = new InfoViewerWidget(_imp->viewer,inputNames[i],this);
        _imp->viewerSubContainerLayout->addWidget(_imp->infoWidget[i]);
        _imp->viewer->setInfoViewer(_imp->infoWidget[i],i);
        if (i == 1) {
            _imp->infoWidget[i]->hide();
        }
    }

    _imp->viewerLayout->addWidget(_imp->viewerSubContainer);

    _imp->mainLayout->addWidget(_imp->viewerContainer);
    /*=============================================*/

    /*=============================================*/

    /*Player buttons*/
    _imp->playerButtonsContainer = new QWidget(this);
    _imp->playerLayout = new QHBoxLayout(_imp->playerButtonsContainer);
    _imp->playerLayout->setSpacing(0);
    _imp->playerLayout->setContentsMargins(0, 0, 0, 0);
    _imp->playerButtonsContainer->setLayout(_imp->playerLayout);
    _imp->mainLayout->addWidget(_imp->playerButtonsContainer);

    _imp->currentFrameBox = new SpinBox(_imp->playerButtonsContainer,SpinBox::eSpinBoxTypeInt);
    _imp->currentFrameBox->setValue(0);
    _imp->currentFrameBox->setToolTip("<p><b>" + tr("Current frame number") + "</b></p>");
    _imp->playerLayout->addWidget(_imp->currentFrameBox);

    _imp->playerLayout->addStretch();

    _imp->firstFrame_Button = new Button(_imp->playerButtonsContainer);
    _imp->firstFrame_Button->setFocusPolicy(Qt::NoFocus);
    _imp->firstFrame_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    setTooltipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerFirst,"<p>" + tr("First frame") + "</p>", _imp->firstFrame_Button);
    _imp->playerLayout->addWidget(_imp->firstFrame_Button);


    _imp->previousKeyFrame_Button = new Button(_imp->playerButtonsContainer);
    _imp->previousKeyFrame_Button->setFocusPolicy(Qt::NoFocus);
    _imp->previousKeyFrame_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    setTooltipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevKF,"<p>" + tr("Previous Keyframe") + "</p>", _imp->previousKeyFrame_Button);


    _imp->play_Backward_Button = new Button(_imp->playerButtonsContainer);
    _imp->play_Backward_Button->setFocusPolicy(Qt::NoFocus);
    _imp->play_Backward_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    setTooltipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerBackward,"<p>" + tr("Play backward") + "</p>", _imp->play_Backward_Button);
    _imp->play_Backward_Button->setCheckable(true);
    _imp->play_Backward_Button->setDown(false);
    _imp->playerLayout->addWidget(_imp->play_Backward_Button);


    _imp->previousFrame_Button = new Button(_imp->playerButtonsContainer);
    _imp->previousFrame_Button->setFocusPolicy(Qt::NoFocus);
    _imp->previousFrame_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    setTooltipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious,"<p>" + tr("Previous frame") + "</p>", _imp->previousFrame_Button);
    
    _imp->playerLayout->addWidget(_imp->previousFrame_Button);


    _imp->stop_Button = new Button(_imp->playerButtonsContainer);
    _imp->stop_Button->setFocusPolicy(Qt::NoFocus);
    _imp->stop_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    setTooltipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerStop,"<p>" + tr("Stop") + "</p>", _imp->stop_Button);
    _imp->playerLayout->addWidget(_imp->stop_Button);


    _imp->nextFrame_Button = new Button(_imp->playerButtonsContainer);
    _imp->nextFrame_Button->setFocusPolicy(Qt::NoFocus);
    _imp->nextFrame_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    setTooltipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerNext,"<p>" + tr("Next frame") + "</p>", _imp->nextFrame_Button);
    _imp->playerLayout->addWidget(_imp->nextFrame_Button);


    _imp->play_Forward_Button = new Button(_imp->playerButtonsContainer);
    _imp->play_Forward_Button->setFocusPolicy(Qt::NoFocus);
    _imp->play_Forward_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    setTooltipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerForward,"<p>" + tr("Play forward") + "</p>", _imp->play_Forward_Button);
    _imp->play_Forward_Button->setCheckable(true);
    _imp->play_Forward_Button->setDown(false);
    _imp->playerLayout->addWidget(_imp->play_Forward_Button);


    _imp->nextKeyFrame_Button = new Button(_imp->playerButtonsContainer);
    _imp->nextKeyFrame_Button->setFocusPolicy(Qt::NoFocus);
    _imp->nextKeyFrame_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    setTooltipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerNextKF,"<p>" + tr("Next Keyframe") + "</p>", _imp->nextKeyFrame_Button);


    _imp->lastFrame_Button = new Button(_imp->playerButtonsContainer);
    _imp->lastFrame_Button->setFocusPolicy(Qt::NoFocus);
    _imp->lastFrame_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    setTooltipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerLast,"<p>" + tr("Last Frame") + "</p>", _imp->lastFrame_Button);
    _imp->playerLayout->addWidget(_imp->lastFrame_Button);


    _imp->playerLayout->addStretch();

    _imp->playerLayout->addWidget(_imp->previousKeyFrame_Button);
    _imp->playerLayout->addWidget(_imp->nextKeyFrame_Button);

    _imp->playerLayout->addStretch();

    _imp->previousIncrement_Button = new Button(_imp->playerButtonsContainer);
    _imp->previousIncrement_Button->setFocusPolicy(Qt::NoFocus);
    _imp->previousIncrement_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    setTooltipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevIncr,"<p>" + tr("Previous Increment") + "</p>", _imp->previousIncrement_Button);
    _imp->playerLayout->addWidget(_imp->previousIncrement_Button);


    _imp->incrementSpinBox = new SpinBox(_imp->playerButtonsContainer);
    _imp->incrementSpinBox->setValue(10);
    _imp->incrementSpinBox->setToolTip( "<p><b>" + tr("Frame increment:") + "</b></p>" + tr(
                                            "The previous/next increment buttons step"
                                            " with this increment.") );
    _imp->playerLayout->addWidget(_imp->incrementSpinBox);


    _imp->nextIncrement_Button = new Button(_imp->playerButtonsContainer);
    _imp->nextIncrement_Button->setFocusPolicy(Qt::NoFocus);
    _imp->nextIncrement_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    setTooltipWithShortcut(kShortcutGroupPlayer, kShortcutIDActionPlayerNextIncr,"<p>" + tr("Next Increment") + "</p>", _imp->nextIncrement_Button);
    _imp->playerLayout->addWidget(_imp->nextIncrement_Button);

    _imp->playbackMode_Button = new Button(_imp->playerButtonsContainer);
    _imp->playbackMode_Button->setFocusPolicy(Qt::NoFocus);
    _imp->playbackMode_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->playbackMode_Button->setToolTip(Natron::convertFromPlainText(tr("Behaviour to adopt when the playback hit the end of the range: loop,bounce or stop."), Qt::WhiteSpaceNormal));
    _imp->playerLayout->addWidget(_imp->playbackMode_Button);


    _imp->playerLayout->addStretch();
    
    //QFont font(appFont,appFontSize);
    
    _imp->canEditFrameRangeLabel = new ClickableLabel(tr("Frame range"),_imp->playerButtonsContainer);
    //_imp->canEditFrameRangeLabel->setFont(font);
    
    _imp->playerLayout->addWidget(_imp->canEditFrameRangeLabel);

    _imp->frameRangeEdit = new LineEdit(_imp->playerButtonsContainer);
    QObject::connect( _imp->frameRangeEdit,SIGNAL( editingFinished() ),this,SLOT( onFrameRangeEditingFinished() ) );
    _imp->frameRangeEdit->setToolTip( Natron::convertFromPlainText(tr("Define here the timeline bounds in which the cursor will playback. Alternatively"
                                                                  " you can drag the red markers on the timeline. The frame range of the project "
                                                                  "is the part coloured in grey on the timeline."),
                                                               Qt::WhiteSpaceNormal) );
    boost::shared_ptr<TimeLine> timeline = _imp->app->getTimeLine();
    _imp->frameRangeEdit->setMaximumWidth(70);

    _imp->playerLayout->addWidget(_imp->frameRangeEdit);


    _imp->playerLayout->addStretch();

    _imp->canEditFpsBox = new QCheckBox(_imp->playerButtonsContainer);
    
    QString canEditFpsBoxTT = Natron::convertFromPlainText(tr("When unchecked, the frame rate will be automatically set by "
                                                          " the informations of the input stream of the Viewer.  "
                                                          "When checked, you're free to set the frame rate of the Viewer.")
                                                       , Qt::WhiteSpaceNormal);
    
    _imp->canEditFpsBox->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->canEditFpsBox->setToolTip(canEditFpsBoxTT);
    _imp->canEditFpsBox->setChecked(!_imp->fpsLocked);
    QObject::connect( _imp->canEditFpsBox,SIGNAL( clicked(bool) ),this,SLOT( onCanSetFPSClicked(bool) ) );
    
    _imp->canEditFpsLabel = new ClickableLabel(tr("fps"),_imp->playerButtonsContainer);
    QObject::connect(_imp->canEditFpsLabel, SIGNAL(clicked(bool)),this,SLOT(onCanSetFPSLabelClicked(bool)));
    _imp->canEditFpsLabel->setToolTip(canEditFpsBoxTT);
    //_imp->canEditFpsLabel->setFont(font);
    
    _imp->playerLayout->addWidget(_imp->canEditFpsBox);
    _imp->playerLayout->addWidget(_imp->canEditFpsLabel);
    
    _imp->fpsBox = new SpinBox(_imp->playerButtonsContainer,SpinBox::eSpinBoxTypeDouble);
    _imp->fpsBox->setReadOnly(_imp->fpsLocked);
    _imp->fpsBox->decimals(1);
    _imp->fpsBox->setValue(24.0);
    _imp->fpsBox->setIncrement(0.1);
    _imp->fpsBox->setToolTip( "<p><b>" + tr("fps:") + "</b></p>" + tr(
                                  "Enter here the desired playback rate.") );
    
    _imp->playerLayout->addWidget(_imp->fpsBox);
    
    QPixmap pixFreezeEnabled,pixFreezeDisabled;
    appPTR->getIcon(Natron::NATRON_PIXMAP_FREEZE_ENABLED,&pixFreezeEnabled);
    appPTR->getIcon(Natron::NATRON_PIXMAP_FREEZE_DISABLED,&pixFreezeDisabled);
    QIcon icFreeze;
    icFreeze.addPixmap(pixFreezeEnabled,QIcon::Normal,QIcon::On);
    icFreeze.addPixmap(pixFreezeDisabled,QIcon::Normal,QIcon::Off);
    _imp->turboButton = new Button(icFreeze,"",_imp->playerButtonsContainer);
    _imp->turboButton->setCheckable(true);
    _imp->turboButton->setChecked(false);
    _imp->turboButton->setDown(false);
    _imp->turboButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _imp->turboButton->setToolTip("<p><b>" + tr("Turbo mode:") + "</p></b><p>" +
                                  tr("When checked, everything besides the viewer will not be refreshed in the user interface "
                                                                                              "for maximum efficiency during playback.") + "</p>");
    _imp->turboButton->setFocusPolicy(Qt::NoFocus);
    QObject::connect( _imp->turboButton, SIGNAL (clicked(bool)), getGui(), SLOT(onFreezeUIButtonClicked(bool) ) );
    _imp->playerLayout->addWidget(_imp->turboButton);

    QPixmap pixFirst;
    QPixmap pixPrevKF;
    QPixmap pixRewindEnabled;
    QPixmap pixRewindDisabled;
    QPixmap pixBack1;
    QPixmap pixStop;
    QPixmap pixForward1;
    QPixmap pixPlayEnabled;
    QPixmap pixPlayDisabled;
    QPixmap pixNextKF;
    QPixmap pixLast;
    QPixmap pixPrevIncr;
    QPixmap pixNextIncr;
    QPixmap pixRefresh;
    QPixmap pixRefreshActive;
    QPixmap pixCenterViewer;
    QPixmap pixLoopMode;
    QPixmap pixClipToProjectEnabled;
    QPixmap pixClipToProjectDisabled;
    QPixmap pixViewerRoIEnabled;
    QPixmap pixViewerRoIDisabled;
    QPixmap pixViewerRs;
    QPixmap pixViewerRsChecked;

    appPTR->getIcon(NATRON_PIXMAP_PLAYER_FIRST_FRAME,&pixFirst);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PREVIOUS_KEY,&pixPrevKF);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_REWIND_ENABLED,&pixRewindEnabled);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_REWIND_DISABLED,&pixRewindDisabled);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PREVIOUS,&pixBack1);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_STOP,&pixStop);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_NEXT,&pixForward1);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PLAY_ENABLED,&pixPlayEnabled);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PLAY_DISABLED,&pixPlayDisabled);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_NEXT_KEY,&pixNextKF);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_LAST_FRAME,&pixLast);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_PREVIOUS_INCR,&pixPrevIncr);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_NEXT_INCR,&pixNextIncr);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_REFRESH,&pixRefresh);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_REFRESH_ACTIVE,&pixRefreshActive);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_CENTER,&pixCenterViewer);
    appPTR->getIcon(NATRON_PIXMAP_PLAYER_LOOP_MODE,&pixLoopMode);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_ENABLED,&pixClipToProjectEnabled);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_DISABLED,&pixClipToProjectDisabled);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_ROI_ENABLED,&pixViewerRoIEnabled);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_ROI_DISABLED,&pixViewerRoIDisabled);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_RENDER_SCALE,&pixViewerRs);
    appPTR->getIcon(NATRON_PIXMAP_VIEWER_RENDER_SCALE_CHECKED,&pixViewerRsChecked);

    _imp->firstFrame_Button->setIcon( QIcon(pixFirst) );
    _imp->previousKeyFrame_Button->setIcon( QIcon(pixPrevKF) );
    
    QIcon icRewind;
    icRewind.addPixmap(pixRewindEnabled,QIcon::Normal,QIcon::On);
    icRewind.addPixmap(pixRewindDisabled,QIcon::Normal,QIcon::Off);
    _imp->play_Backward_Button->setIcon( icRewind );
    _imp->previousFrame_Button->setIcon( QIcon(pixBack1) );
    _imp->stop_Button->setIcon( QIcon(pixStop) );
    _imp->nextFrame_Button->setIcon( QIcon(pixForward1) );
    
    QIcon icPlay;
    icPlay.addPixmap(pixPlayEnabled,QIcon::Normal,QIcon::On);
    icPlay.addPixmap(pixPlayDisabled,QIcon::Normal,QIcon::Off);
    _imp->play_Forward_Button->setIcon( icPlay );
    _imp->nextKeyFrame_Button->setIcon( QIcon(pixNextKF) );
    _imp->lastFrame_Button->setIcon( QIcon(pixLast) );
    _imp->previousIncrement_Button->setIcon( QIcon(pixPrevIncr) );
    _imp->nextIncrement_Button->setIcon( QIcon(pixNextIncr) );
    _imp->iconRefreshOff = QIcon(pixRefresh);
    _imp->iconRefreshOn = QIcon(pixRefreshActive);
    _imp->refreshButton->setIcon(_imp->iconRefreshOff);
    _imp->centerViewerButton->setIcon( QIcon(pixCenterViewer) );
    _imp->playbackMode_Button->setIcon( QIcon(pixLoopMode) );

    QIcon icClip;
    icClip.addPixmap(pixClipToProjectEnabled,QIcon::Normal,QIcon::On);
    icClip.addPixmap(pixClipToProjectDisabled,QIcon::Normal,QIcon::Off);
    _imp->clipToProjectFormatButton->setIcon(icClip);

    QIcon icRoI;
    icRoI.addPixmap(pixViewerRoIEnabled,QIcon::Normal,QIcon::On);
    icRoI.addPixmap(pixViewerRoIDisabled,QIcon::Normal,QIcon::Off);
    _imp->enableViewerRoI->setIcon(icRoI);

    QIcon icViewerRs;
    icViewerRs.addPixmap(pixViewerRs,QIcon::Normal,QIcon::Off);
    icViewerRs.addPixmap(pixViewerRsChecked,QIcon::Normal,QIcon::On);
    _imp->activateRenderScale->setIcon(icViewerRs);
    
    setTooltipWithShortcut(kShortcutGroupViewer, kShortcutIDActionFitViewer,"<p>" +
                           tr("Scales the image so it doesn't exceed the size of the viewer and centers it.") +"</p>", _imp->centerViewerButton);
    setTooltipWithShortcut(kShortcutGroupViewer, kShortcutIDActionClipEnabled,"<p>" +
                           tr("Clips the portion of the image displayed "
                              "on the viewer to the project format. "
                              "When off, everything in the union of all nodes "
                              "region of definition will be displayed.") +"</p>", _imp->clipToProjectFormatButton);
    setTooltipWithShortcut(kShortcutGroupViewer, kShortcutIDActionROIEnabled,"<p>" +
                           tr("When active, enables the region of interest that will limit"
                              " the portion of the viewer that is kept updated.") +"</p>", _imp->enableViewerRoI);


    /*=================================================*/

    /*frame seeker*/
    _imp->timeLineGui = new TimeLineGui(node,timeline,_imp->gui,this);
    QObject::connect(_imp->timeLineGui, SIGNAL( boundariesChanged(SequenceTime,SequenceTime)),
                     this, SLOT(onTimelineBoundariesChanged(SequenceTime, SequenceTime)));
    QObject::connect(gui->getApp()->getProject().get(), SIGNAL(frameRangeChanged(int,int)), _imp->timeLineGui, SLOT(onProjectFrameRangeChanged(int,int)));
    _imp->timeLineGui->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum);
    _imp->mainLayout->addWidget(_imp->timeLineGui);
    double leftBound,rightBound;
    gui->getApp()->getFrameRange(&leftBound, &rightBound);
    _imp->timeLineGui->setBoundaries(leftBound, rightBound);
    onTimelineBoundariesChanged(leftBound,rightBound);
    _imp->timeLineGui->setFrameRangeEdited(false);
    /*================================================*/


    /*slots & signals*/
    
    manageTimelineSlot(false,timeline);
    
    boost::shared_ptr<Node> wrapperNode = _imp->viewerNode->getNode();
    QObject::connect( _imp->viewerNode, SIGNAL(renderStatsAvailable(int,int,std::map<boost::shared_ptr<Natron::Node>,NodeRenderStats >)),
                     this, SLOT(onRenderStatsAvailable(int,int,std::map<boost::shared_ptr<Natron::Node>,NodeRenderStats >)));
    QObject::connect( wrapperNode.get(),SIGNAL( inputChanged(int) ),this,SLOT( onInputChanged(int) ) );
    QObject::connect( wrapperNode.get(),SIGNAL( inputLabelChanged(int,QString) ),this,SLOT( onInputNameChanged(int,QString) ) );
    QObject::connect( _imp->viewerNode,SIGNAL(clipPreferencesChanged()), this, SLOT(onClipPreferencesChanged()));
    QObject::connect( _imp->viewerNode,SIGNAL( activeInputsChanged() ),this,SLOT( onActiveInputsChanged() ) );
    QObject::connect( _imp->viewerColorSpace, SIGNAL( currentIndexChanged(int) ), this,
                      SLOT( onColorSpaceComboBoxChanged(int) ) );
    QObject::connect( _imp->zoomCombobox, SIGNAL( currentIndexChanged(int) ),this, SLOT( onZoomComboboxCurrentIndexChanged(int)) );
    QObject::connect( _imp->viewer, SIGNAL( zoomChanged(int) ), this, SLOT( updateZoomComboBox(int) ) );
    QObject::connect( _imp->gainBox, SIGNAL( valueChanged(double) ), this,SLOT( onGainSpinBoxValueChanged(double) ) );
    QObject::connect( _imp->gainSlider, SIGNAL( positionChanged(double) ), this, SLOT( onGainSliderChanged(double) ) );
    QObject::connect( _imp->currentFrameBox, SIGNAL( valueChanged(double) ), this, SLOT( onCurrentTimeSpinBoxChanged(double) ) );

    QObject::connect( _imp->play_Forward_Button,SIGNAL( clicked(bool) ),this,SLOT( startPause(bool) ) );
    QObject::connect( _imp->stop_Button,SIGNAL( clicked() ),this,SLOT( abortRendering() ) );
    QObject::connect( _imp->play_Backward_Button,SIGNAL( clicked(bool) ),this,SLOT( startBackward(bool) ) );
    QObject::connect( _imp->previousFrame_Button,SIGNAL( clicked() ),this,SLOT( previousFrame() ) );
    QObject::connect( _imp->nextFrame_Button,SIGNAL( clicked() ),this,SLOT( nextFrame() ) );
    QObject::connect( _imp->previousIncrement_Button,SIGNAL( clicked() ),this,SLOT( previousIncrement() ) );
    QObject::connect( _imp->nextIncrement_Button,SIGNAL( clicked() ),this,SLOT( nextIncrement() ) );
    QObject::connect( _imp->firstFrame_Button,SIGNAL( clicked() ),this,SLOT( firstFrame() ) );
    QObject::connect( _imp->lastFrame_Button,SIGNAL( clicked() ),this,SLOT( lastFrame() ) );
    QObject::connect( _imp->playbackMode_Button, SIGNAL( clicked(bool) ), this, SLOT( togglePlaybackMode() ) );
    

    
    QObject::connect( _imp->refreshButton, SIGNAL( clicked() ), this, SLOT( refresh() ) );
    QObject::connect( _imp->centerViewerButton, SIGNAL( clicked() ), this, SLOT( centerViewer() ) );
    QObject::connect( _imp->viewerNode,SIGNAL( viewerDisconnected() ),this,SLOT( disconnectViewer() ) );
    QObject::connect( _imp->fpsBox, SIGNAL( valueChanged(double) ), this, SLOT( onSpinboxFpsChanged(double) ) );

    QObject::connect( _imp->viewerNode->getRenderEngine(),SIGNAL( renderFinished(int) ),this,SLOT( onEngineStopped() ) );
    QObject::connect( _imp->viewerNode->getRenderEngine(),SIGNAL( renderStarted(bool) ),this,SLOT( onEngineStarted(bool) ) );
    manageSlotsForInfoWidget(0,true);

    QObject::connect( _imp->clipToProjectFormatButton,SIGNAL( clicked(bool) ),this,SLOT( onClipToProjectButtonToggle(bool) ) );
    QObject::connect( _imp->viewsComboBox,SIGNAL( currentIndexChanged(int) ),this,SLOT( showView(int) ) );
    QObject::connect( _imp->enableViewerRoI, SIGNAL( clicked(bool) ), this, SLOT( onEnableViewerRoIButtonToggle(bool) ) );
    QObject::connect( _imp->autoContrast,SIGNAL( clicked(bool) ),this,SLOT( onAutoContrastChanged(bool) ) );
    QObject::connect( _imp->autoConstrastLabel,SIGNAL( clicked(bool) ),this,SLOT( onAutoContrastChanged(bool) ) );
    QObject::connect( _imp->autoConstrastLabel,SIGNAL( clicked(bool) ),_imp->autoContrast,SLOT( setChecked(bool) ) );
    QObject::connect( _imp->renderScaleCombo,SIGNAL( currentIndexChanged(int) ),this,SLOT( onRenderScaleComboIndexChanged(int) ) );
    QObject::connect( _imp->activateRenderScale,SIGNAL( toggled(bool) ),this,SLOT( onRenderScaleButtonClicked(bool) ) );
    
    QObject::connect( _imp->viewerNode, SIGNAL( viewerRenderingStarted() ), this, SLOT( onViewerRenderingStarted() ) );
    QObject::connect( _imp->viewerNode, SIGNAL( viewerRenderingEnded() ), this, SLOT( onViewerRenderingStopped() ) );

    connectToViewerCache();

    for (std::list<NodeGui*>::const_iterator it = existingRotoNodes.begin(); it != existingRotoNodes.end(); ++it) {
        createRotoInterface(*it);
    }
    if ( currentRoto && currentRoto->isSettingsPanelVisible() ) {
        setRotoInterface(currentRoto);
    }

    for (std::list<NodeGui*>::const_iterator it = existingTrackerNodes.begin(); it != existingTrackerNodes.end(); ++it) {
        createTrackerInterface(*it);
    }
    if ( currentTracker && currentTracker->isSettingsPanelVisible() ) {
        setRotoInterface(currentTracker);
    }

    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    
    _imp->viewerNode->setUiContext(getViewer());
    
    refreshLayerAndAlphaChannelComboBox();
    
    QTimer::singleShot(25, _imp->timeLineGui, SLOT(recenterOnBounds()));
    
    
    //Refresh the viewport lock state
    const std::list<ViewerTab*>& viewers = _imp->gui->getViewersList();
    if (!viewers.empty()) {
        ViewerTab* other = viewers.front();
        if (other->isViewersSynchroEnabled()) {
            double left,bottom,factor,par;
            other->getViewer()->getProjection(&left, &bottom, &factor, &par);
            _imp->viewer->setProjection(left, bottom, factor, par);
            _imp->syncViewerButton->setDown(true);
            _imp->syncViewerButton->setChecked(true);
        }
    }

}
