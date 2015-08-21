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

#include <cassert>
#include <QDebug>
#include <QApplication>
#include <QSlider>
#include <QComboBox>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QGridLayout>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QAbstractItemView>
#include <QCheckBox>
#include <QTimer>
#include <QCoreApplication>
#include <QToolBar>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QKeyEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QtGui/QKeySequence>
#include <QTextDocument>

#include <boost/weak_ptr.hpp>

#include "Engine/ViewerInstance.h"
#include "Engine/Settings.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"
#include "Engine/Node.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Transform.h"

#include "Gui/ViewerGL.h"
#include "Gui/InfoViewerWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TimeLineGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/ComboBox.h"
#include "Gui/Button.h"
#include "Gui/Gui.h"
#include "Gui/TabWidget.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/ClickableLabel.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/RotoGui.h"
#include "Gui/TrackerGui.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/GuiMacros.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/Label.h"
#include "Gui/Utils.h"
#include "Gui/DopeSheet.h"

#ifndef M_LN2
#define M_LN2       0.693147180559945309417232121458176568  /* loge(2)        */
#endif

#define NATRON_TRANSFORM_AFFECTS_OVERLAYS


using namespace Natron;

namespace {
struct InputName
{
    QString name;
    boost::weak_ptr<Natron::Node> input;
};

typedef std::map<int,InputName> InputNamesMap;
}

class ChannelsComboBox : public ComboBox
{
    
public:
    
    ChannelsComboBox(QWidget* parent) : ComboBox(parent) {}
    
private:
    
    virtual void paintEvent(QPaintEvent* event) OVERRIDE FINAL
    {
        ComboBox::paintEvent(event);
        
        int idx = activeIndex();
        if (idx != 1) {
            QColor color;
            
            QPainter p(this);
            QPen pen;
            
            switch (idx) {
                case 0:
                    //luminance
                    color.setRgbF(0.5, 0.5, 0.5);
                    break;
                case 2:
                    //r
                    color.setRgbF(1., 0, 0);
                    break;
                case 3:
                    //g
                    color.setRgbF(0., 1., 0.);
                    break;
                case 4:
                    //b
                    color.setRgbF(0., 0. , 1.);
                    break;
                case 5:
                    //a
                    color.setRgbF(1.,1.,1.);
                    break;
            }
            
            pen.setColor(color);
            p.setPen(pen);
            
            
            QRectF bRect = rect();
            QRectF roundedRect = bRect.adjusted(1., 1., -2., -2.);
            
            double roundPixels = 3;
            
            
            QPainterPath path;
            path.addRoundedRect(roundedRect, roundPixels, roundPixels);
            p.drawPath(path);
        }
    }
};

struct ViewerTabPrivate
{
    /*OpenGL viewer*/
    ViewerGL* viewer;
    GuiAppInstance* app;
    QWidget* viewerContainer;
    QHBoxLayout* viewerLayout;
    QWidget* viewerSubContainer;
    QVBoxLayout* viewerSubContainerLayout;
    QVBoxLayout* mainLayout;

    /*Viewer Settings*/
    QWidget* firstSettingsRow, *secondSettingsRow;
    QHBoxLayout* firstRowLayout, *secondRowLayout;

    /*1st row*/
    //ComboBox* viewerLayers;
    ComboBox* layerChoice;
    ComboBox* alphaChannelChoice;
    ChannelsComboBox* viewerChannels;
    ComboBox* zoomCombobox;
    Button* syncViewerButton;
    Button* centerViewerButton;
    Button* clipToProjectFormatButton;
    Button* enableViewerRoI;
    Button* refreshButton;
    QIcon iconRefreshOff, iconRefreshOn;
    int ongoingRenderCount;
    
    Button* activateRenderScale;
    bool renderScaleActive;
    ComboBox* renderScaleCombo;
    Natron::Label* firstInputLabel;
    ComboBox* firstInputImage;
    ComboBox* compositingOperator;
    Natron::Label* secondInputLabel;
    ComboBox* secondInputImage;

    /*2nd row*/
    Button* toggleGainButton;
    SpinBox* gainBox;
    ScaleSliderQWidget* gainSlider;
    double lastFstopValue;
    ClickableLabel* autoConstrastLabel;
    QCheckBox* autoContrast;
    SpinBox* gammaBox;
    double lastGammaValue;
    Button* toggleGammaButton;
    ScaleSliderQWidget* gammaSlider;
    ComboBox* viewerColorSpace;
    Button* checkerboardButton;
    ComboBox* viewsComboBox;
    int currentViewIndex;
    QMutex currentViewMutex;
    /*Info*/
    InfoViewerWidget* infoWidget[2];


    /*TimeLine buttons*/
    QWidget* playerButtonsContainer;
    QHBoxLayout* playerLayout;
    SpinBox* currentFrameBox;
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
    Button* playbackMode_Button;
    
    mutable QMutex playbackModeMutex;
    Natron::PlaybackModeEnum playbackMode;
    
    LineEdit* frameRangeEdit;

    ClickableLabel* canEditFrameRangeLabel;

    QCheckBox* canEditFpsBox;
    ClickableLabel* canEditFpsLabel;
    mutable QMutex fpsLockedMutex;
    bool fpsLocked;
    SpinBox* fpsBox;
    Button* turboButton;

    /*frame seeker*/
    TimeLineGui* timeLineGui;
    std::map<NodeGui*,RotoGui*> rotoNodes;
    std::pair<NodeGui*,RotoGui*> currentRoto;
    std::map<NodeGui*,TrackerGui*> trackerNodes;
    std::pair<NodeGui*,TrackerGui*> currentTracker;
    InputNamesMap inputNamesMap;
    mutable QMutex compOperatorMutex;
    ViewerCompositingOperatorEnum compOperator;
    Gui* gui;
    ViewerInstance* viewerNode; // < pointer to the internal node
    
    mutable QMutex visibleToolbarsMutex; //< protects the 4 bool below
    bool infobarVisible;
    bool playerVisible;
    bool timelineVisible;
    bool leftToolbarVisible;
    bool rightToolbarVisible;
    bool topToolbarVisible;
    
    bool isFileDialogViewer;
    
    mutable QMutex checkerboardMutex;
    bool checkerboardEnabled;

    mutable QMutex fpsMutex;
    double fps;
    
    //The last node that took the penDown/motion/keyDown/keyRelease etc...
    boost::weak_ptr<Natron::Node> lastOverlayNode;
    
    ViewerTabPrivate(Gui* gui,
                     ViewerInstance* node)
    : viewer(NULL)
    , app( gui->getApp() )
    , viewerContainer(NULL)
    , viewerLayout(NULL)
    , viewerSubContainer(NULL)
    , viewerSubContainerLayout(NULL)
    , mainLayout(NULL)
    , firstSettingsRow(NULL)
    , secondSettingsRow(NULL)
    , firstRowLayout(NULL)
    , secondRowLayout(NULL)
    , layerChoice(NULL)
    , alphaChannelChoice(NULL)
    , viewerChannels(NULL)
    , zoomCombobox(NULL)
    , syncViewerButton(NULL)
    , centerViewerButton(NULL)
    , clipToProjectFormatButton(NULL)
    , enableViewerRoI(NULL)
    , refreshButton(NULL)
    , iconRefreshOff()
    , iconRefreshOn()
    , ongoingRenderCount(0)
    , activateRenderScale(NULL)
    , renderScaleActive(false)
    , renderScaleCombo(NULL)
    , firstInputLabel(NULL)
    , firstInputImage(NULL)
    , compositingOperator(NULL)
    , secondInputLabel(NULL)
    , secondInputImage(NULL)
    , toggleGainButton(NULL)
    , gainBox(NULL)
    , gainSlider(NULL)
    , lastFstopValue(0.)
    , autoConstrastLabel(NULL)
    , autoContrast(NULL)
    , gammaBox(NULL)
    , lastGammaValue(1.)
    , toggleGammaButton(NULL)
    , gammaSlider(NULL)
    , viewerColorSpace(NULL)
    , checkerboardButton(NULL)
    , viewsComboBox(NULL)
    , currentViewIndex(0)
    , currentViewMutex()
    , infoWidget()
    , playerButtonsContainer(0)
    , playerLayout(NULL)
    , currentFrameBox(NULL)
    , firstFrame_Button(NULL)
    , previousKeyFrame_Button(NULL)
    , play_Backward_Button(NULL)
    , previousFrame_Button(NULL)
    , stop_Button(NULL)
    , nextFrame_Button(NULL)
    , play_Forward_Button(NULL)
    , nextKeyFrame_Button(NULL)
    , lastFrame_Button(NULL)
    , previousIncrement_Button(NULL)
    , incrementSpinBox(NULL)
    , nextIncrement_Button(NULL)
    , playbackMode_Button(NULL)
    , playbackModeMutex()
    , playbackMode(Natron::ePlaybackModeLoop)
    , frameRangeEdit(NULL)
    , canEditFrameRangeLabel(NULL)
    , canEditFpsBox(NULL)
    , canEditFpsLabel(NULL)
    , fpsLockedMutex()
    , fpsLocked(true)
    , fpsBox(NULL)
    , turboButton(NULL)
    , timeLineGui(NULL)
    , rotoNodes()
    , currentRoto()
    , trackerNodes()
    , currentTracker()
    , inputNamesMap()
    , compOperatorMutex()
    , compOperator(eViewerCompositingOperatorNone)
    , gui(gui)
    , viewerNode(node)
    , visibleToolbarsMutex()
    , infobarVisible(true)
    , playerVisible(true)
    , timelineVisible(true)
    , leftToolbarVisible(true)
    , rightToolbarVisible(true)
    , topToolbarVisible(true)
    , isFileDialogViewer(false)
    , checkerboardMutex()
    , checkerboardEnabled(false)
    , fpsMutex()
    , fps(24.)
    , lastOverlayNode()
    {
        infoWidget[0] = infoWidget[1] = NULL;
        currentRoto.first = NULL;
        currentRoto.second = NULL;
    }
    
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
    // return the tronsform to apply to the overlay as a 3x3 homography in canonical coordinates
    bool getOverlayTransform(double time,
                             int view,
                             const boost::shared_ptr<Natron::Node>& target,
                             Natron::EffectInstance* currentNode,
                             Transform::Matrix3x3* transform) const;
    
    bool getTimeTransform(double time,
                          int view,
                          const boost::shared_ptr<Natron::Node>& target,
                          Natron::EffectInstance* currentNode,
                          double *newTime) const;

#endif
    
    void getComponentsAvailabel(std::set<ImageComponents>* comps) const;
    
    };

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

#pragma message WARN("TODO: add zoom in/zoom out/fit to viewer zoom menu")
    // Unfortunately, this require a bit of work, because zoomSlot(QString) *parses* the menu entry and thus expects all entries to have the form "xx%".
    // Keyboard shortcuts should be made visible to the user, not only in the shortcut editor, but also at logical places in the GUI.

    //ActionWithShortcut* zoomInAction = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionZoomIn, kShortcutDescActionZoomIn, this);
    //_imp->zoomCombobox->addAction(zoomInAction);
    //ActionWithShortcut* zoomOutAction = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionZoomOut, kShortcutDescActionZoomOut, this);
    //_imp->zoomCombobox->addAction(zoomOutAction);
    //ActionWithShortcut* zoomFitAction = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDActionFitViewer, kShortcutDescActionFitViewer, this);
    //_imp->zoomCombobox->addAction(zoomFitAction);
    _imp->zoomCombobox->addItem("10%");
    _imp->zoomCombobox->addItem("25%");
    _imp->zoomCombobox->addItem("50%");
    _imp->zoomCombobox->addItem("75%");
    ActionWithShortcut* level100Action = new ActionWithShortcut(kShortcutGroupViewer, kShortcutIDActionZoomLevel100, "100%", this);
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
    _imp->refreshButton->setToolTip("<p>" + tr("Forces a new render of the current frame.") +
                                     "</p><p><b>" + tr("Keyboard shortcut:") + " U</b></p>");
    _imp->firstRowLayout->addWidget(_imp->refreshButton);

    _imp->activateRenderScale = new Button(_imp->firstSettingsRow);
    _imp->activateRenderScale->setFocusPolicy(Qt::NoFocus);
    _imp->activateRenderScale->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence rsKs(Qt::CTRL + Qt::Key_P);
    _imp->activateRenderScale->setToolTip("<p><b>" + tr("Proxy mode:") + "</b></p><p>" + tr(
                                               "Activates the downscaling by the amount indicated by the value on the right. "
                                               "The rendered images are degraded and as a result of this the whole rendering pipeline "
                                               "is much faster.") +
                                           "</p><p><b>" + tr("Keyboard shortcut:") + " " + rsKs.toString(QKeySequence::NativeText) + "</b></p>");
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
    _imp->compositingOperator->addItem(tr("Wipe"), QIcon(), QKeySequence(), tr("Wipe between A and B"));
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
    QKeySequence firstFrameKey(Qt::CTRL + Qt::Key_Left);
    QString tooltip = "<p>" + tr("First frame") + "</p>";
    tooltip.append("<p><b>" + tr("Keyboard shortcut:") + " ");
    tooltip.append( firstFrameKey.toString(QKeySequence::NativeText) );
    tooltip.append("</b></p>");
    _imp->firstFrame_Button->setToolTip(tooltip);
    _imp->playerLayout->addWidget(_imp->firstFrame_Button);


    _imp->previousKeyFrame_Button = new Button(_imp->playerButtonsContainer);
    _imp->previousKeyFrame_Button->setFocusPolicy(Qt::NoFocus);
    _imp->previousKeyFrame_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence previousKeyFrameKey(Qt::CTRL + Qt::SHIFT +  Qt::Key_Left);
    tooltip = "<p>" + tr("Previous keyframe") + "</p>";
    tooltip.append( "<p><b>" + tr("Keyboard shortcut:") + " " );
    tooltip.append( previousKeyFrameKey.toString(QKeySequence::NativeText) );
    tooltip.append("</b></p>");
    _imp->previousKeyFrame_Button->setToolTip(tooltip);


    _imp->play_Backward_Button = new Button(_imp->playerButtonsContainer);
    _imp->play_Backward_Button->setFocusPolicy(Qt::NoFocus);
    _imp->play_Backward_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence playbackFrameKey(Qt::Key_J);
    tooltip = "<p>" + tr("Play backward") + "</p>";
    tooltip.append( "<p><b>" + tr("Keyboard shortcut:") + " " );
    tooltip.append( playbackFrameKey.toString(QKeySequence::NativeText) );
    tooltip.append("</b></p>");
    _imp->play_Backward_Button->setToolTip(tooltip);
    _imp->play_Backward_Button->setCheckable(true);
    _imp->play_Backward_Button->setDown(false);
    _imp->playerLayout->addWidget(_imp->play_Backward_Button);


    _imp->previousFrame_Button = new Button(_imp->playerButtonsContainer);
    _imp->previousFrame_Button->setFocusPolicy(Qt::NoFocus);
    _imp->previousFrame_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence previousFrameKey(Qt::Key_Left);
    tooltip = "<p>" + tr("Previous frame") + "</p>";
    tooltip.append( "<p><b>" + tr("Keyboard shortcut:") + " " );
    tooltip.append( previousFrameKey.toString(QKeySequence::NativeText) );
    tooltip.append("</b></p>");
    _imp->previousFrame_Button->setToolTip(tooltip);
    _imp->playerLayout->addWidget(_imp->previousFrame_Button);


    _imp->stop_Button = new Button(_imp->playerButtonsContainer);
    _imp->stop_Button->setFocusPolicy(Qt::NoFocus);
    _imp->stop_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence stopKey(Qt::Key_K);
    tooltip = "<p>" + tr("Stop") + "</p>";
    tooltip.append( "<p><b>" + tr("Keyboard shortcut:") + " " );
    tooltip.append( stopKey.toString(QKeySequence::NativeText) );
    tooltip.append("</b></p>");
    _imp->stop_Button->setToolTip(tooltip);
    _imp->playerLayout->addWidget(_imp->stop_Button);


    _imp->nextFrame_Button = new Button(_imp->playerButtonsContainer);
    _imp->nextFrame_Button->setFocusPolicy(Qt::NoFocus);
    _imp->nextFrame_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence nextFrameKey(Qt::Key_Right);
    tooltip = "<p>" + tr("Next frame") + "</p>";
    tooltip.append( "<p><b>" + tr("Keyboard shortcut:") + " " );
    tooltip.append( nextFrameKey.toString(QKeySequence::NativeText) );
    tooltip.append("</b></p>");
    _imp->nextFrame_Button->setToolTip(tooltip);
    _imp->playerLayout->addWidget(_imp->nextFrame_Button);


    _imp->play_Forward_Button = new Button(_imp->playerButtonsContainer);
    _imp->play_Forward_Button->setFocusPolicy(Qt::NoFocus);
    _imp->play_Forward_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence playKey(Qt::Key_L);
    tooltip = "<p>" + tr("Play forward") + "</p>";
    tooltip.append( "<p><b>" + tr("Keyboard shortcut:") + " " );
    tooltip.append( playKey.toString(QKeySequence::NativeText) );
    tooltip.append("</b></p>");
    _imp->play_Forward_Button->setToolTip(tooltip);
    _imp->play_Forward_Button->setCheckable(true);
    _imp->play_Forward_Button->setDown(false);
    _imp->playerLayout->addWidget(_imp->play_Forward_Button);


    _imp->nextKeyFrame_Button = new Button(_imp->playerButtonsContainer);
    _imp->nextKeyFrame_Button->setFocusPolicy(Qt::NoFocus);
    _imp->nextKeyFrame_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence nextKeyFrameKey(Qt::CTRL + Qt::SHIFT +  Qt::Key_Right);
    tooltip = "<p>" + tr("Next keyframe") + "</p>";
    tooltip.append( "<p><b>" + tr("Keyboard shortcut:") + " " );
    tooltip.append( nextKeyFrameKey.toString(QKeySequence::NativeText) );
    tooltip.append("</b></p>");
    _imp->nextKeyFrame_Button->setToolTip(tooltip);


    _imp->lastFrame_Button = new Button(_imp->playerButtonsContainer);
    _imp->lastFrame_Button->setFocusPolicy(Qt::NoFocus);
    _imp->lastFrame_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence lastFrameKey(Qt::CTRL + Qt::Key_Right);
    tooltip = "<p>" + tr("Last frame") + "</p>";
    tooltip.append( "<p><b>" + tr("Keyboard shortcut:") + " " );
    tooltip.append( lastFrameKey.toString(QKeySequence::NativeText) );
    tooltip.append("</b></p>");
    _imp->lastFrame_Button->setToolTip(tooltip);
    _imp->playerLayout->addWidget(_imp->lastFrame_Button);


    _imp->playerLayout->addStretch();

    _imp->playerLayout->addWidget(_imp->previousKeyFrame_Button);
    _imp->playerLayout->addWidget(_imp->nextKeyFrame_Button);

    _imp->playerLayout->addStretch();

    _imp->previousIncrement_Button = new Button(_imp->playerButtonsContainer);
    _imp->previousIncrement_Button->setFocusPolicy(Qt::NoFocus);
    _imp->previousIncrement_Button->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QKeySequence previousIncrFrameKey(Qt::SHIFT + Qt::Key_Left);
    tooltip = "<p>" + tr("Previous increment") + "</p>";
    tooltip.append( "<p><b>" + tr("Keyboard shortcut:") + " " );
    tooltip.append( previousIncrFrameKey.toString(QKeySequence::NativeText) );
    tooltip.append("</b></p>");
    _imp->previousIncrement_Button->setToolTip(tooltip);
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
    QKeySequence nextIncrFrameKey(Qt::SHIFT + Qt::Key_Right);
    tooltip = "<p>" + tr("Next increment") + "</p>";
    tooltip.append( "<p><b>" + tr("Keyboard shortcut:") + " " );
    tooltip.append( nextIncrFrameKey.toString(QKeySequence::NativeText) );
    tooltip.append("</b></p>");
    _imp->nextIncrement_Button->setToolTip(tooltip);
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


    _imp->centerViewerButton->setToolTip("<p>" + tr("Scales the image so it doesn't exceed the size of the viewer and centers it.") +
                                          "</p><p><b>" + tr("Keyboard shortcut:") + " F</b></p>");

    _imp->clipToProjectFormatButton->setToolTip("<p>" + tr("Clips the portion of the image displayed "
                                                            "on the viewer to the project format. "
                                                            "When off, everything in the union of all nodes "
                                                            "region of definition will be displayed.") +
                                                 "</p><p><b>" + tr("Keyboard shortcut:") + " " + QKeySequence(Qt::SHIFT + Qt::Key_C).toString() +
                                                 "</b></p>");

    QKeySequence enableViewerKey(Qt::SHIFT + Qt::Key_W);
    _imp->enableViewerRoI->setToolTip("<p>" + tr("When active, enables the region of interest that will limit"
                                                  " the portion of the viewer that is kept updated.") +
                                       "</p><p><b>" + tr("Keyboard shortcut:") + " " + enableViewerKey.toString() + "</b></p>");
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
    QObject::connect( wrapperNode.get(),SIGNAL( inputChanged(int) ),this,SLOT( onInputChanged(int) ) );
    QObject::connect( wrapperNode.get(),SIGNAL( inputLabelChanged(int,QString) ),this,SLOT( onInputNameChanged(int,QString) ) );
    QObject::connect( _imp->viewerNode,SIGNAL(clipPreferencesChanged()), this, SLOT(onClipPreferencesChanged()));
    QObject::connect( _imp->viewerNode,SIGNAL( activeInputsChanged() ),this,SLOT( onActiveInputsChanged() ) );
    QObject::connect( _imp->viewerColorSpace, SIGNAL( currentIndexChanged(int) ), this,
                      SLOT( onColorSpaceComboBoxChanged(int) ) );
    QObject::connect( _imp->zoomCombobox, SIGNAL( currentIndexChanged(QString) ),_imp->viewer, SLOT( zoomSlot(QString) ) );
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

void
ViewerTab::onColorSpaceComboBoxChanged(int v)
{
    Natron::ViewerColorSpaceEnum colorspace;

    if (v == 0) {
        colorspace = Natron::eViewerColorSpaceLinear;
    } else if (v == 1) {
        colorspace = Natron::eViewerColorSpaceSRGB;
    } else if (v == 2) {
        colorspace = Natron::eViewerColorSpaceRec709;
    } else {
        assert(false);
    }
    _imp->viewer->setLut( (int)colorspace );
    _imp->viewerNode->onColorSpaceChanged(colorspace);
}

void
ViewerTab::onEnableViewerRoIButtonToggle(bool b)
{
    _imp->enableViewerRoI->setDown(b);
    _imp->viewer->setUserRoIEnabled(b);
}

void
ViewerTab::updateViewsMenu(int count)
{
    int currentIndex = _imp->viewsComboBox->activeIndex();

    _imp->viewsComboBox->clear();
    if (count == 1) {
        _imp->viewsComboBox->hide();
        _imp->viewsComboBox->addItem( tr("Main") );
    } else if (count == 2) {
        _imp->viewsComboBox->show();
        _imp->viewsComboBox->addItem( tr("Left"),QIcon(),QKeySequence(Qt::CTRL + Qt::Key_1) );
        _imp->viewsComboBox->addItem( tr("Right"),QIcon(),QKeySequence(Qt::CTRL + Qt::Key_2) );
    } else {
        _imp->viewsComboBox->show();
        for (int i = 0; i < count; ++i) {
            _imp->viewsComboBox->addItem( QString( tr("View ") ) + QString::number(i + 1),QIcon(),Gui::keySequenceForView(i) );
        }
    }
    if ( ( currentIndex < _imp->viewsComboBox->count() ) && (currentIndex != -1) ) {
        _imp->viewsComboBox->setCurrentIndex(currentIndex);
    } else {
        _imp->viewsComboBox->setCurrentIndex(0);
    }
    _imp->gui->updateViewsActions(count);
}

void
ViewerTab::setCurrentView(int view)
{
    _imp->viewsComboBox->setCurrentIndex(view);
}

int
ViewerTab::getCurrentView() const
{
    QMutexLocker l(&_imp->currentViewMutex);

    return _imp->currentViewIndex;
}

void
ViewerTab::setPlaybackMode(Natron::PlaybackModeEnum mode)
{
    QPixmap pix;
    switch (mode) {
        case Natron::ePlaybackModeLoop:
            appPTR->getIcon(NATRON_PIXMAP_PLAYER_LOOP_MODE, &pix);
            break;
        case Natron::ePlaybackModeBounce:
            appPTR->getIcon(NATRON_PIXMAP_PLAYER_BOUNCE, &pix);
            break;
        case Natron::ePlaybackModeOnce:
            appPTR->getIcon(NATRON_PIXMAP_PLAYER_PLAY_ONCE, &pix);
            break;
        default:
            break;
    }
    {
        QMutexLocker k(&_imp->playbackModeMutex);
        _imp->playbackMode = mode;
    }
    _imp->playbackMode_Button->setIcon(QIcon(pix));
    _imp->viewerNode->getRenderEngine()->setPlaybackMode(mode);

}

Natron::PlaybackModeEnum
ViewerTab::getPlaybackMode() const
{
    QMutexLocker k(&_imp->playbackModeMutex);
    return _imp->playbackMode;
}

void
ViewerTab::togglePlaybackMode()
{
    Natron::PlaybackModeEnum mode = _imp->viewerNode->getRenderEngine()->getPlaybackMode();
    mode = (Natron::PlaybackModeEnum)(((int)mode + 1) % 3);
    QPixmap pix;
    switch (mode) {
        case Natron::ePlaybackModeLoop:
            appPTR->getIcon(NATRON_PIXMAP_PLAYER_LOOP_MODE, &pix);
            break;
        case Natron::ePlaybackModeBounce:
            appPTR->getIcon(NATRON_PIXMAP_PLAYER_BOUNCE, &pix);
            break;
        case Natron::ePlaybackModeOnce:
            appPTR->getIcon(NATRON_PIXMAP_PLAYER_PLAY_ONCE, &pix);
            break;
        default:
            break;
    }
    {
        QMutexLocker k(&_imp->playbackModeMutex);
        _imp->playbackMode = mode;
    }
    _imp->playbackMode_Button->setIcon(QIcon(pix));
    _imp->viewerNode->getRenderEngine()->setPlaybackMode(mode);
}

void
ViewerTab::onClipToProjectButtonToggle(bool b)
{
    _imp->clipToProjectFormatButton->setDown(b);
    _imp->viewer->setClipToDisplayWindow(b);
}


void
ViewerTab::updateZoomComboBox(int value)
{
    assert(value > 0);
    QString str = QString::number(value);
    str.append( QChar('%') );
    str.prepend("  ");
    str.append("  ");
    _imp->zoomCombobox->setCurrentText_no_emit(str);
}

/*In case they're several viewer around, we need to reset the dag and tell it
   explicitly we want to use this viewer and not another one.*/
void
ViewerTab::startPause(bool b)
{
    abortRendering();
    if (b) {
        _imp->gui->getApp()->setLastViewerUsingTimeline(_imp->viewerNode->getNode());
        _imp->viewerNode->getRenderEngine()->renderFromCurrentFrame(OutputSchedulerThread::eRenderDirectionForward);
    }
}

void
ViewerTab::abortRendering()
{
    if (_imp->play_Forward_Button) {
        _imp->play_Forward_Button->setDown(false);
        _imp->play_Forward_Button->setChecked(false);
    }
    if (_imp->play_Backward_Button) {
        _imp->play_Backward_Button->setDown(false);
        _imp->play_Backward_Button->setChecked(false);
    }
    if (_imp->gui && _imp->gui->isGUIFrozen() && appPTR->getCurrentSettings()->isAutoTurboEnabled()) {
        _imp->gui->onFreezeUIButtonClicked(false);
    }
    if (_imp->gui) {
        ///Abort all viewers because they are all synchronised.
        const std::list<ViewerTab*> & activeNodes = _imp->gui->getViewersList();

        for (std::list<ViewerTab*>::const_iterator it = activeNodes.begin(); it != activeNodes.end(); ++it) {
            ViewerInstance* viewer = (*it)->getInternalNode();
            if (viewer) {
                viewer->getRenderEngine()->abortRendering(true);
            }
        }
    }
}

void
ViewerTab::onEngineStarted(bool forward)
{
    if (!_imp->gui) {
        return;
    }
    
    if (_imp->play_Forward_Button) {
        _imp->play_Forward_Button->setDown(forward);
        _imp->play_Forward_Button->setChecked(forward);
    }

    if (_imp->play_Backward_Button) {
        _imp->play_Backward_Button->setDown(!forward);
        _imp->play_Backward_Button->setChecked(!forward);
    }

    if (_imp->gui && !_imp->gui->isGUIFrozen() && appPTR->getCurrentSettings()->isAutoTurboEnabled()) {
        _imp->gui->onFreezeUIButtonClicked(true);
    }
}

void
ViewerTab::onEngineStopped()
{
    if (!_imp->gui) {
        return;
    }
    
    if (_imp->play_Forward_Button && _imp->play_Forward_Button->isDown()) {
        _imp->play_Forward_Button->setDown(false);
        _imp->play_Forward_Button->setChecked(false);
    }
    
    if (_imp->play_Backward_Button && _imp->play_Backward_Button->isDown()) {
        _imp->play_Backward_Button->setDown(false);
        _imp->play_Backward_Button->setChecked(false);
    }
    if (_imp->gui && _imp->gui->isGUIFrozen() && appPTR->getCurrentSettings()->isAutoTurboEnabled()) {
        _imp->gui->onFreezeUIButtonClicked(false);
    }
}

void
ViewerTab::startBackward(bool b)
{
    abortRendering();
    if (b) {
        _imp->gui->getApp()->setLastViewerUsingTimeline(_imp->viewerNode->getNode());
        _imp->viewerNode->getRenderEngine()->renderFromCurrentFrame(OutputSchedulerThread::eRenderDirectionBackward);

    }
}

void
ViewerTab::seek(SequenceTime time)
{
    _imp->currentFrameBox->setValue(time);
    _imp->timeLineGui->seek(time);
    
}

void
ViewerTab::previousFrame()
{
    int prevFrame = _imp->timeLineGui->currentFrame() -1 ;
    if (prevFrame  < _imp->timeLineGui->leftBound()) {
        prevFrame = _imp->timeLineGui->rightBound();
    }
    seek(prevFrame);
}

void
ViewerTab::nextFrame()
{
    int nextFrame = _imp->timeLineGui->currentFrame() + 1;
    if (nextFrame  > _imp->timeLineGui->rightBound()) {
        nextFrame = _imp->timeLineGui->leftBound();
    }
    seek(nextFrame);
}

void
ViewerTab::previousIncrement()
{
    seek( _imp->timeLineGui->currentFrame() - _imp->incrementSpinBox->value() );
}

void
ViewerTab::nextIncrement()
{
    seek( _imp->timeLineGui->currentFrame() + _imp->incrementSpinBox->value() );
}

void
ViewerTab::firstFrame()
{
    seek( _imp->timeLineGui->leftBound() );
}

void
ViewerTab::lastFrame()
{
    seek( _imp->timeLineGui->rightBound() );
}

void
ViewerTab::onTimeLineTimeChanged(SequenceTime time,
                                 int /*reason*/)
{
    if (!_imp->gui) {
        return;
    }
    _imp->currentFrameBox->setValue(time);
    
    if (_imp->timeLineGui->getTimeline() != _imp->gui->getApp()->getTimeLine()) {
        _imp->viewerNode->renderCurrentFrame(true);
    }
}

void
ViewerTab::onCurrentTimeSpinBoxChanged(double time)
{
    _imp->timeLineGui->seek(time);
}

void
ViewerTab::centerViewer()
{
    _imp->viewer->fitImageToFormat();
    if ( _imp->viewer->displayingImage() ) {
        _imp->viewerNode->renderCurrentFrame(false);
    } else {
        _imp->viewer->updateGL();
    }
}

void
ViewerTab::refresh()
{
    _imp->viewerNode->forceFullComputationOnNextFrame();
    _imp->viewerNode->renderCurrentFrame(false);
}

ViewerTab::~ViewerTab()
{
    if (_imp->gui) {
        NodeGraph* graph = 0;
        if (_imp->viewerNode) {
            boost::shared_ptr<NodeCollection> collection = _imp->viewerNode->getNode()->getGroup();
            if (collection) {
                NodeGroup* isGrp = dynamic_cast<NodeGroup*>(collection.get());
                if (isGrp) {
                    NodeGraphI* graph_i = isGrp->getNodeGraph();
                    if (graph_i) {
                        graph = dynamic_cast<NodeGraph*>(graph_i);
                        assert(graph);
                    }
                } else {
                    graph = _imp->gui->getNodeGraph();
                }
            }
            _imp->viewerNode->invalidateUiContext();
        } else {
            graph = _imp->gui->getNodeGraph();
        }
        assert(graph);
        if ( _imp->app && !_imp->app->isClosing() && (graph->getLastSelectedViewer() == this) ) {
            graph->setLastSelectedViewer(0);
        }
    }
    for (std::map<NodeGui*,RotoGui*>::iterator it = _imp->rotoNodes.begin(); it != _imp->rotoNodes.end(); ++it) {
        delete it->second;
    }
    for (std::map<NodeGui*,TrackerGui*>::iterator it = _imp->trackerNodes.begin(); it != _imp->trackerNodes.end(); ++it) {
        delete it->second;
    }
}

void
ViewerTab::keyPressEvent(QKeyEvent* e)
{
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();

    if ( isKeybind(kShortcutGroupGlobal, kShortcutIDActionShowPaneFullScreen, modifiers, key) ) { //< this shortcut is global
        if ( parentWidget() ) {
            QKeyEvent* ev = new QKeyEvent(QEvent::KeyPress,key, modifiers);
            QCoreApplication::postEvent(parentWidget(),ev);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionLuminance, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 0) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, true);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(0);
            setDisplayChannels(0, true);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionRed, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 2) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, true);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(2);
            setDisplayChannels(2, true);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionGreen, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 3) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, true);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(3);
            setDisplayChannels(3, true);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionBlue, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 4) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, true);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(4);
            setDisplayChannels(4, true);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionAlpha, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 5) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, true);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(5);
            setDisplayChannels(5, true);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionLuminanceA, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 0) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, false);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(0);
            setDisplayChannels(0, false);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionRedA, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 2) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, false);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(2);
            setDisplayChannels(2, false);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionGreenA, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 3) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, false);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(3);
            setDisplayChannels(3, false);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionBlueA, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 4) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, false);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(4);
            setDisplayChannels(4, false);
        }
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionAlphaA, modifiers, key) ) {
        int currentIndex = _imp->viewerChannels->activeIndex();
        if (currentIndex == 5) {
            _imp->viewerChannels->setCurrentIndex_no_emit(1);
            setDisplayChannels(1, false);
        } else {
            _imp->viewerChannels->setCurrentIndex_no_emit(5);
            setDisplayChannels(5, false);
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, modifiers, key) ) {
        previousFrame();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerBackward, modifiers, key) ) {
        startBackward( !_imp->play_Backward_Button->isDown() );
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerStop, modifiers, key) ) {
        abortRendering();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerForward, modifiers, key) ) {
        startPause( !_imp->play_Forward_Button->isDown() );
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNext, modifiers, key) ) {
        nextFrame();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevIncr, modifiers, key) ) {
        //prev incr
        previousIncrement();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextIncr, modifiers, key) ) {
        //next incr
        nextIncrement();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerFirst, modifiers, key) ) {
        //first frame
        firstFrame();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerLast, modifiers, key) ) {
        //last frame
        lastFrame();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevKF, modifiers, key) ) {
        //prev key
        _imp->app->getTimeLine()->goToPreviousKeyframe();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextKF, modifiers, key) ) {
        //next key
        _imp->app->getTimeLine()->goToNextKeyframe();
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionFitViewer, modifiers, key) ) {
        centerViewer();
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionClipEnabled, modifiers, key) ) {
        onClipToProjectButtonToggle( !_imp->clipToProjectFormatButton->isDown() );
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionRefresh, modifiers, key) ) {
        refresh();
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionROIEnabled, modifiers, key) ) {
        onEnableViewerRoIButtonToggle( !_imp->enableViewerRoI->isDown() );
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionProxyEnabled, modifiers, key) ) {
        onRenderScaleButtonClicked(!_imp->renderScaleActive);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel2, modifiers, key) ) {
        _imp->renderScaleCombo->setCurrentIndex(0);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel4, modifiers, key) ) {
        _imp->renderScaleCombo->setCurrentIndex(1);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel8, modifiers, key) ) {
        _imp->renderScaleCombo->setCurrentIndex(2);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel16, modifiers, key) ) {
        _imp->renderScaleCombo->setCurrentIndex(3);
    } else if ( isKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel32, modifiers, key) ) {
        _imp->renderScaleCombo->setCurrentIndex(4);
    } else if (isKeybind(kShortcutGroupViewer, kShortcutIDActionZoomLevel100, modifiers, key) ) {
        _imp->viewer->zoomSlot(100);
        _imp->zoomCombobox->setCurrentIndex_no_emit(4);
    } else {
        QWidget::keyPressEvent(e);
    }
} // keyPressEvent

void
ViewerTab::setDisplayChannels(int i, bool setBothInputs)
{
    Natron::DisplayChannelsEnum channels;
    
    switch (i) {
        case 0:
            channels = Natron::eDisplayChannelsY;
            break;
        case 1:
            channels = Natron::eDisplayChannelsRGB;
            break;
        case 2:
            channels = Natron::eDisplayChannelsR;
            break;
        case 3:
            channels = Natron::eDisplayChannelsG;
            break;
        case 4:
            channels = Natron::eDisplayChannelsB;
            break;
        case 5:
            channels = Natron::eDisplayChannelsA;
            break;
        default:
            channels = Natron::eDisplayChannelsRGB;
            break;
    }
    _imp->viewerNode->setDisplayChannels(channels, setBothInputs);
}

void
ViewerTab::onViewerChannelsChanged(int i)
{
    setDisplayChannels(i, false);
}

bool
ViewerTab::eventFilter(QObject *target,
                       QEvent* e)
{
    if (e->type() == QEvent::MouseButtonPress) {
        if (_imp->gui && _imp->app) {
            boost::shared_ptr<NodeGuiI> gui_i = _imp->viewerNode->getNode()->getNodeGui();
            assert(gui_i);
            boost::shared_ptr<NodeGui> gui = boost::dynamic_pointer_cast<NodeGui>(gui_i);
            _imp->gui->selectNode(gui);
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
    if (!_imp->playerButtonsContainer->isVisible()) {
        return QSize(500,200);
    }
    return QWidget::minimumSizeHint();
}

QSize
ViewerTab::sizeHint() const
{
    if (!_imp->playerButtonsContainer->isVisible()) {
        return QSize(500,200);
    }

    return QWidget::sizeHint();
}

void
ViewerTab::showView(int view)
{
    {
        QMutexLocker l(&_imp->currentViewMutex);
        
        _imp->currentViewIndex = view;
    }
    abortRendering();
    _imp->viewerNode->renderCurrentFrame(true);
}

#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
//OpenGL is column-major for matrixes
static void transformToOpenGLMatrix(const Transform::Matrix3x3& mat,GLdouble* oglMat)
{
    oglMat[0] = mat.a; oglMat[4] = mat.b; oglMat[8]  = 0; oglMat[12] = mat.c;
    oglMat[1] = mat.d; oglMat[5] = mat.e; oglMat[9]  = 0; oglMat[13] = mat.f;
    oglMat[2] = 0;     oglMat[6] = 0;     oglMat[10] = 1; oglMat[14] = 0;
    oglMat[3] = mat.g; oglMat[7] = mat.h; oglMat[11] = 0; oglMat[15] = mat.i;
}
#endif

void
ViewerTab::drawOverlays(double time,
                        double scaleX,
                        double scaleY) const
{

    if ( !_imp->app ||
        !_imp->viewer ||
        _imp->app->isClosing() ||
        isFileDialogViewer() ||
        !_imp->gui ||
        (_imp->gui->isGUIFrozen() && !_imp->app->getIsUserPainting())) {
        return;
    }
    
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
    int view = getCurrentView();
#endif
    
    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    
    ///Draw overlays in reverse order of appearance so that the first (top) panel is drawn on top of everything else
    for (std::list<boost::shared_ptr<Natron::Node> >::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        double transformedTime;
        bool ok = _imp->getTimeTransform(time, view, *it, getInternalNode(), &transformedTime);
        
        boost::shared_ptr<NodeGui> nodeUi = boost::dynamic_pointer_cast<NodeGui>((*it)->getNodeGui());
        if (!nodeUi) {
            continue;
        }
        bool overlayDeemed = false;
        if (ok) {
            if (time != transformedTime) {
                //when retimed, modify the overlay color so it looks deemed to indicate that the user
                //cannot modify it
                overlayDeemed = true;
            }
            time = transformedTime;
        }
        nodeUi->setOverlayLocked(overlayDeemed);
        
        Transform::Matrix3x3 mat(1,0,0,0,1,0,0,0,1);
        ok = _imp->getOverlayTransform(time, view, *it, getInternalNode(), &mat);
        GLfloat oldMat[16];
        if (ok) {
            //Ok we've got a transform here, apply it to the OpenGL model view matrix
            
            GLdouble oglMat[16];
            transformToOpenGLMatrix(mat,oglMat);
            glMatrixMode(GL_MODELVIEW);
            glGetFloatv(GL_MODELVIEW_MATRIX, oldMat);
            glMultMatrixd(oglMat);
        }
        
#endif
        
        if (_imp->currentRoto.first && (*it) == _imp->currentRoto.first->getNode()) {
            if ( _imp->currentRoto.second && _imp->currentRoto.first->isSettingsPanelVisible() ) {
                _imp->currentRoto.second->drawOverlays(time, scaleX, scaleY);
            }
        } else if (_imp->currentTracker.first && (*it) == _imp->currentTracker.first->getNode()) {
            if ( _imp->currentTracker.second && _imp->currentTracker.first->isSettingsPanelVisible() ) {
                _imp->currentTracker.second->drawOverlays(time, scaleX, scaleY);
            }
        } else {
            
            Natron::EffectInstance* effect = (*it)->getLiveInstance();
            assert(effect);
            effect->setCurrentViewportForOverlays_public(_imp->viewer);
            effect->drawOverlay_public(time, scaleX,scaleY);
        }
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        if (ok) {
            glMatrixMode(GL_MODELVIEW);
            glLoadMatrixf(oldMat);
        }
#endif
    }
}

bool
ViewerTab::notifyOverlaysPenDown_internal(const boost::shared_ptr<Natron::Node>& node,
                                          double scaleX,
                                          double scaleY,
                                          Natron::PenType pen,
                                          bool isTabletEvent,
                                          const QPointF & viewportPos,
                                          const QPointF & pos,
                                          double pressure,
                                          double timestamp,
                                          QMouseEvent* e)
{

    QPointF transformViewportPos;
    QPointF transformPos;
    double time = _imp->app->getTimeLine()->currentFrame();
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
    
    
    int view = getCurrentView();
    
    double transformedTime;
    bool ok = _imp->getTimeTransform(time, view, node, getInternalNode(), &transformedTime);
    if (ok) {
        
        /*
         * Do not allow interaction with retimed interacts otherwise the user may end up modifying keyframes at unexpected 
         * (or invalid for floating point) frames, which may be confusing. Rather we indicate with the overlay color hint
         * that interact is not editable when it is retimed.
         */
        if (time != transformedTime) {
            return false;
        }
        time = transformedTime;
    }
    
    Transform::Matrix3x3 mat(1,0,0,0,1,0,0,0,1);
    ok = _imp->getOverlayTransform(time, view, node, getInternalNode(), &mat);
    if (!ok) {
        transformViewportPos = viewportPos;
        transformPos = pos;
    } else {
        mat = Transform::matInverse(mat);
        {
            Transform::Point3D p;
            p.x = viewportPos.x();
            p.y = viewportPos.y();
            p.z = 1;
            p = Transform::matApply(mat, p);
            transformViewportPos.rx() = p.x / p.z;
            transformViewportPos.ry() = p.y / p.z;
        }
        {
            Transform::Point3D p;
            p.x = pos.x();
            p.y = pos.y();
            p.z = 1;
            p = Transform::matApply(mat, p);
            transformPos.rx() = p.x / p.z;
            transformPos.ry() = p.y / p.z;
        }
    }
    
   
#else
    transformViewportPos = viewportPos;
    transformPos = pos;
#endif
    
    if (_imp->currentRoto.first && node == _imp->currentRoto.first->getNode()) {
        if ( _imp->currentRoto.second && _imp->currentRoto.first->isSettingsPanelVisible() ) {
            if ( _imp->currentRoto.second->penDown(time, scaleX, scaleY, pen, isTabletEvent, transformViewportPos, transformPos, pressure, timestamp, e) ) {
                _imp->lastOverlayNode = node;
                return true;
            }
        }
    } else if (_imp->currentTracker.first && node == _imp->currentTracker.first->getNode()) {
        if ( _imp->currentTracker.second && _imp->currentTracker.first->isSettingsPanelVisible() ) {
            if ( _imp->currentTracker.second->penDown(time, scaleX, scaleY, transformViewportPos, transformPos, pressure, e) ) {
                _imp->lastOverlayNode = node;
                return true;
            }
        }
    } else {
        
        Natron::EffectInstance* effect = node->getLiveInstance();
        assert(effect);
        effect->setCurrentViewportForOverlays_public(_imp->viewer);
        bool didSmthing = effect->onOverlayPenDown_public(time, scaleX, scaleY, transformViewportPos, transformPos, pressure);
        if (didSmthing) {
            //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html
            // if the instance returns kOfxStatOK, the host should not pass the pen motion
            
            // to any other interactive object it may own that shares the same view.
            _imp->lastOverlayNode = node;
            return true;
        }
    }
    
    return false;
}

bool
ViewerTab::notifyOverlaysPenDown(double scaleX,
                                 double scaleY,
                                 Natron::PenType pen,
                                 bool isTabletEvent,
                                 const QPointF & viewportPos,
                                 const QPointF & pos,
                                 double pressure,
                                 double timestamp,
                                 QMouseEvent* e)
{

    if ( !_imp->app || _imp->app->isClosing() ) {
        return false;
    }

    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    
    
    boost::shared_ptr<Natron::Node> lastOverlay = _imp->lastOverlayNode.lock();
    if (lastOverlay) {
        for (std::list<boost::shared_ptr<Natron::Node> >::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if (*it == lastOverlay) {
                
                if (notifyOverlaysPenDown_internal(*it, scaleX, scaleY, pen, isTabletEvent, viewportPos, pos, pressure, timestamp, e)) {
                    return true;
                } else {
                    nodes.erase(it);
                    break;
                }
            }
        }
    }
    
    for (std::list<boost::shared_ptr<Natron::Node> >::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        if (notifyOverlaysPenDown_internal(*it, scaleX, scaleY, pen, isTabletEvent, viewportPos, pos, pressure, timestamp, e)) {
            return true;
        }
    }

    if (getGui()->getApp()->getOverlayRedrawRequestsCount() > 0) {
        getGui()->getApp()->redrawAllViewers();
    }
    getGui()->getApp()->clearOverlayRedrawRequests();
    return false;
}


bool
ViewerTab::notifyOverlaysPenDoubleClick(double scaleX,
                                        double scaleY,
                                        const QPointF & viewportPos,
                                        const QPointF & pos,
                                        QMouseEvent* e)
{
    if ( !_imp->app || _imp->app->isClosing() ) {
        return false;
    }
    
    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    

    for (std::list<boost::shared_ptr<Natron::Node> >::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        QPointF transformViewportPos;
        QPointF transformPos;
        double time = _imp->app->getTimeLine()->currentFrame();
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        int view = getCurrentView();
        
        double transformedTime;
        bool ok = _imp->getTimeTransform(time, view, *it, getInternalNode(), &transformedTime);
        if (ok) {
            time = transformedTime;
        }
        
        Transform::Matrix3x3 mat(1,0,0,0,1,0,0,0,1);
        ok = _imp->getOverlayTransform(time, view, *it, getInternalNode(), &mat);
        if (!ok) {
            transformViewportPos = viewportPos;
            transformPos = pos;
        } else {
            mat = Transform::matInverse(mat);
            {
                Transform::Point3D p;
                p.x = viewportPos.x();
                p.y = viewportPos.y();
                p.z = 1;
                p = Transform::matApply(mat, p);
                transformViewportPos.rx() = p.x / p.z;
                transformViewportPos.ry() = p.y / p.z;
            }
            {
                Transform::Point3D p;
                p.x = pos.x();
                p.y = pos.y();
                p.z = 1;
                p = Transform::matApply(mat, p);
                transformPos.rx() = p.x / p.z;
                transformPos.ry() = p.y / p.z;
            }
        }
        
        

#else
        transformViewportPos = viewportPos;
        transformPos = pos;
#endif
        
        if ( _imp->currentRoto.second && _imp->currentRoto.first->isSettingsPanelVisible() ) {
            if ( _imp->currentRoto.second->penDoubleClicked(time, scaleX, scaleY, transformViewportPos, transformPos, e) ) {
                _imp->lastOverlayNode = _imp->currentRoto.first->getNode();
                return true;
            }
        }
        
        if ( _imp->currentTracker.second && _imp->currentTracker.first->isSettingsPanelVisible() ) {
            if ( _imp->currentTracker.second->penDoubleClicked(time, scaleX, scaleY, transformViewportPos, transformPos, e) ) {
                _imp->lastOverlayNode = _imp->currentRoto.first->getNode();
                return true;
            }
        }
    }

    return false;
}

bool
ViewerTab::notifyOverlaysPenMotion_internal(const boost::shared_ptr<Natron::Node>& node,
                                            double scaleX,
                                            double scaleY,
                                            const QPointF & viewportPos,
                                            const QPointF & pos,
                                            double pressure,
                                            double timestamp,
                                            QInputEvent* e)
{
    
    QPointF transformViewportPos;
    QPointF transformPos;
    double time = _imp->app->getTimeLine()->currentFrame();
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
    int view = getCurrentView();
    
    double transformedTime;
    bool ok = _imp->getTimeTransform(time, view, node, getInternalNode(), &transformedTime);
    if (ok) {
        /*
         * Do not allow interaction with retimed interacts otherwise the user may end up modifying keyframes at unexpected
         * (or invalid for floating point) frames, which may be confusing. Rather we indicate with the overlay color hint
         * that interact is not editable when it is retimed.
         */
        if (time != transformedTime) {
            return false;
        }
        time = transformedTime;
    }
    

    
    Transform::Matrix3x3 mat(1,0,0,0,1,0,0,0,1);
    ok = _imp->getOverlayTransform(time, view, node, getInternalNode(), &mat);
    if (!ok) {
        transformViewportPos = viewportPos;
        transformPos = pos;
    } else {
        mat = Transform::matInverse(mat);
        {
            Transform::Point3D p;
            p.x = viewportPos.x();
            p.y = viewportPos.y();
            p.z = 1;
            p = Transform::matApply(mat, p);
            transformViewportPos.rx() = p.x / p.z;
            transformViewportPos.ry() = p.y / p.z;
        }
        {
            Transform::Point3D p;
            p.x = pos.x();
            p.y = pos.y();
            p.z = 1;
            p = Transform::matApply(mat, p);
            transformPos.rx() = p.x / p.z;
            transformPos.ry() = p.y / p.z;
        }
    }
    
#else
    transformViewportPos = viewportPos;
    transformPos = pos;
#endif
    
    if (_imp->currentRoto.first && node == _imp->currentRoto.first->getNode()) {
        if ( _imp->currentRoto.second && _imp->currentRoto.first->isSettingsPanelVisible() ) {
            if ( _imp->currentRoto.second->penMotion(time, scaleX, scaleY, transformViewportPos, transformPos, pressure, timestamp, e) ) {
                _imp->lastOverlayNode = node;
                return true;
            }
        }
    } else if (_imp->currentTracker.first && node == _imp->currentTracker.first->getNode()) {
        if ( _imp->currentTracker.second && _imp->currentTracker.first->isSettingsPanelVisible() ) {
            if ( _imp->currentTracker.second->penMotion(time, scaleX, scaleY, transformViewportPos, transformPos, pressure, e) ) {
                _imp->lastOverlayNode = node;
                return true;
            }
        }
    } else {
        
        Natron::EffectInstance* effect = node->getLiveInstance();
        assert(effect);
        effect->setCurrentViewportForOverlays_public(_imp->viewer);
        bool didSmthing = effect->onOverlayPenMotion_public(time, scaleX, scaleY, transformViewportPos, transformPos, pressure);
        if (didSmthing) {
            //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html
            // if the instance returns kOfxStatOK, the host should not pass the pen motion
            
            // to any other interactive object it may own that shares the same view.
            _imp->lastOverlayNode = node;
            return true;
        }
    }
    return false;
}

bool
ViewerTab::notifyOverlaysPenMotion(double scaleX,
                                   double scaleY,
                                   const QPointF & viewportPos,
                                   const QPointF & pos,
                                   double pressure,
                                   double timestamp,
                                   QInputEvent* e)
{
    bool didSomething = false;

    if ( !_imp->app || _imp->app->isClosing() ) {
        return false;
    }
    
    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    
    
    boost::shared_ptr<Natron::Node> lastOverlay = _imp->lastOverlayNode.lock();
    if (lastOverlay) {
        for (std::list<boost::shared_ptr<Natron::Node> >::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if (*it == lastOverlay) {
                if (notifyOverlaysPenMotion_internal(*it, scaleX, scaleY, viewportPos, pos, pressure, timestamp, e)) {
                    return true;
                } else {
                    nodes.erase(it);
                    break;
                }
            }
        }
    }

    
    for (std::list<boost::shared_ptr<Natron::Node> >::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        if (notifyOverlaysPenMotion_internal(*it, scaleX, scaleY, viewportPos, pos, pressure, timestamp, e)) {
            return true;
        }
    }

   
    if (getGui()->getApp()->getOverlayRedrawRequestsCount() > 0) {
        getGui()->getApp()->redrawAllViewers();
    }
    getGui()->getApp()->clearOverlayRedrawRequests();
   

    return didSomething;
}

bool
ViewerTab::notifyOverlaysPenUp(double scaleX,
                               double scaleY,
                               const QPointF & viewportPos,
                               const QPointF & pos,
                               double pressure,
                               double timestamp,
                               QMouseEvent* e)
{
    bool didSomething = false;

    if ( !_imp->app || _imp->app->isClosing() ) {
        return false;
    }
    
    _imp->lastOverlayNode.reset();
    
    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        
        
        QPointF transformViewportPos;
        QPointF transformPos;
        double time = _imp->app->getTimeLine()->currentFrame();
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        int view = getCurrentView();
        
        
        double transformedTime;
        bool ok = _imp->getTimeTransform(time, view, *it, getInternalNode(), &transformedTime);
        if (ok) {
            /*
             * Do not allow interaction with retimed interacts otherwise the user may end up modifying keyframes at unexpected
             * (or invalid for floating point) frames, which may be confusing. Rather we indicate with the overlay color hint
             * that interact is not editable when it is retimed.
             */
            if (time != transformedTime) {
                return false;
            }
            time = transformedTime;
        }
        
        Transform::Matrix3x3 mat(1,0,0,0,1,0,0,0,1);
        ok = _imp->getOverlayTransform(time, view, *it, getInternalNode(), &mat);
        if (!ok) {
            transformViewportPos = viewportPos;
            transformPos = pos;
        } else {
            mat = Transform::matInverse(mat);
            {
                Transform::Point3D p;
                p.x = viewportPos.x();
                p.y = viewportPos.y();
                p.z = 1;
                p = Transform::matApply(mat, p);
                transformViewportPos.rx() = p.x / p.z;
                transformViewportPos.ry() = p.y / p.z;
            }
            {
                Transform::Point3D p;
                p.x = pos.x();
                p.y = pos.y();
                p.z = 1;
                p = Transform::matApply(mat, p);
                transformPos.rx() = p.x / p.z;
                transformPos.ry() = p.y / p.z;
            }
        }
        
        

#else
        transformViewportPos = viewportPos;
        transformPos = pos;
#endif
        
        
        if (_imp->currentRoto.first && (*it) == _imp->currentRoto.first->getNode()) {
            
            if ( _imp->currentRoto.second && _imp->currentRoto.first->isSettingsPanelVisible() ) {
                didSomething |= _imp->currentRoto.second->penUp(time, scaleX, scaleY, transformViewportPos, transformPos, pressure, timestamp, e);
            }
        }
        if (_imp->currentTracker.first && (*it) == _imp->currentTracker.first->getNode()) {
            if ( _imp->currentTracker.second && _imp->currentTracker.first->isSettingsPanelVisible() ) {
                didSomething |=  _imp->currentTracker.second->penUp(time, scaleX, scaleY, transformViewportPos, transformPos, pressure, e)  ;
            }
        }
        
        Natron::EffectInstance* effect = (*it)->getLiveInstance();
        assert(effect);
        effect->setCurrentViewportForOverlays_public(_imp->viewer);
        didSomething |= effect->onOverlayPenUp_public(time, scaleX, scaleY, transformViewportPos, transformPos, pressure);
        
        
    }

   
    if (!didSomething && getGui()->getApp()->getOverlayRedrawRequestsCount() > 0) {
        getGui()->getApp()->redrawAllViewers();
    }
    getGui()->getApp()->clearOverlayRedrawRequests();
    

    return didSomething;
}

bool
ViewerTab::notifyOverlaysKeyDown_internal(const boost::shared_ptr<Natron::Node>& node,double scaleX,double scaleY,QKeyEvent* e,
                                          Natron::Key k,
                                          Natron::KeyboardModifiers km)
{
    
    double time = _imp->app->getTimeLine()->currentFrame();
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
    double transformedTime;
    bool ok = _imp->getTimeTransform(time, 0, node, getInternalNode(), &transformedTime);
    if (ok) {
        /*
         * Do not allow interaction with retimed interacts otherwise the user may end up modifying keyframes at unexpected
         * (or invalid for floating point) frames, which may be confusing. Rather we indicate with the overlay color hint
         * that interact is not editable when it is retimed.
         */
        if (time != transformedTime) {
            return false;
        }
        time = transformedTime;
    }
#endif
    
    
    if (_imp->currentRoto.first && node == _imp->currentRoto.first->getNode()) {
        
        if ( _imp->currentRoto.second && _imp->currentRoto.first->isSettingsPanelVisible() ) {
            if ( _imp->currentRoto.second->keyDown(time, scaleX, scaleY, e) ) {
                _imp->lastOverlayNode = node;
                return true;
            }
        }
    } else if (_imp->currentTracker.first && node == _imp->currentTracker.first->getNode()) {
        if ( _imp->currentTracker.second && _imp->currentTracker.first->isSettingsPanelVisible() ) {
            if ( _imp->currentTracker.second->keyDown(time, scaleX, scaleY, e) ) {
                _imp->lastOverlayNode = node;
                return true;
            }
        }
        
    } else {
   
        Natron::EffectInstance* effect = node->getLiveInstance();
        assert(effect);
        effect->setCurrentViewportForOverlays_public(_imp->viewer);
        bool didSmthing = effect->onOverlayKeyDown_public(time, scaleX,scaleY,k,km);
        if (didSmthing) {
            //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html
            // if the instance returns kOfxStatOK, the host should not pass the pen motion
            
            // to any other interactive object it may own that shares the same view.
            _imp->lastOverlayNode = node;
            return true;
        }
    }
    return false;
}

bool
ViewerTab::notifyOverlaysKeyDown(double scaleX,
                                 double scaleY,
                                 QKeyEvent* e)
{
    bool didSomething = false;

    if ( !_imp->app || _imp->app->isClosing() ) {
        return false;
    }

    Natron::Key natronKey = QtEnumConvert::fromQtKey( (Qt::Key)e->key() );
    Natron::KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers( e->modifiers() );
    
    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    
    boost::shared_ptr<Natron::Node> lastOverlay = _imp->lastOverlayNode.lock();
    if (lastOverlay) {
        for (std::list<boost::shared_ptr<Natron::Node> >::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if (*it == lastOverlay) {
                if (notifyOverlaysKeyDown_internal(*it, scaleX, scaleY, e, natronKey, natronMod)) {
                    return true;
                } else {
                    nodes.erase(it);
                    break;
                }
            }
        }
    }

    
    for (std::list<boost::shared_ptr<Natron::Node> >::reverse_iterator it = nodes.rbegin();
         it != nodes.rend();
         ++it) {
        if (notifyOverlaysKeyDown_internal(*it, scaleX, scaleY, e, natronKey, natronMod)) {
            return true;
        }
    }

    if (getGui()->getApp()->getOverlayRedrawRequestsCount() > 0) {
        getGui()->getApp()->redrawAllViewers();
    }
    getGui()->getApp()->clearOverlayRedrawRequests();

    return didSomething;
}

bool
ViewerTab::notifyOverlaysKeyUp(double scaleX,
                               double scaleY,
                               QKeyEvent* e)
{
    bool didSomething = false;

    if ( !_imp->app || _imp->app->isClosing() ) {
        return false;
    }
    
    _imp->lastOverlayNode.reset();


    double time = _imp->app->getTimeLine()->currentFrame();

    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        Natron::EffectInstance* effect = (*it)->getLiveInstance();
        assert(effect);
        
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        double transformedTime;
        bool ok = _imp->getTimeTransform(time, 0, *it, getInternalNode(), &transformedTime);
        if (ok) {
            /*
             * Do not allow interaction with retimed interacts otherwise the user may end up modifying keyframes at unexpected
             * (or invalid for floating point) frames, which may be confusing. Rather we indicate with the overlay color hint
             * that interact is not editable when it is retimed.
             */
            if (time != transformedTime) {
                return false;
            }
            time = transformedTime;
        }
#endif
        
        if (_imp->currentRoto.first && (*it) == _imp->currentRoto.first->getNode()) {
            if ( _imp->currentRoto.second && _imp->currentRoto.first->isSettingsPanelVisible() ) {
                didSomething |= _imp->currentRoto.second->keyUp(time, scaleX, scaleY, e);
            }
        }
        if (_imp->currentTracker.first && (*it) == _imp->currentTracker.first->getNode()) {
            if ( _imp->currentTracker.second && _imp->currentTracker.first->isSettingsPanelVisible() ) {
                didSomething |= _imp->currentTracker.second->keyUp(time, scaleX, scaleY, e);
            }
        }
        
        effect->setCurrentViewportForOverlays_public(_imp->viewer);
        didSomething |= effect->onOverlayKeyUp_public( time, scaleX,scaleY,
                                            QtEnumConvert::fromQtKey( (Qt::Key)e->key() ),QtEnumConvert::fromQtModifiers( e->modifiers() ) );
        
    }
    
   
    if (!didSomething && getGui()->getApp()->getOverlayRedrawRequestsCount() > 0) {
        getGui()->getApp()->redrawAllViewers();
    }
    getGui()->getApp()->clearOverlayRedrawRequests();
    

    return didSomething;
}

bool
ViewerTab::notifyOverlaysKeyRepeat_internal(const boost::shared_ptr<Natron::Node>& node,double scaleX,double scaleY,QKeyEvent* e,Natron::Key k,
                                      Natron::KeyboardModifiers km)
{
    
    double time = _imp->app->getTimeLine()->currentFrame();
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
    double transformedTime;
    bool ok = _imp->getTimeTransform(time, 0, node, getInternalNode(), &transformedTime);
    if (ok) {
        /*
         * Do not allow interaction with retimed interacts otherwise the user may end up modifying keyframes at unexpected
         * (or invalid for floating point) frames, which may be confusing. Rather we indicate with the overlay color hint
         * that interact is not editable when it is retimed.
         */
        if (time != transformedTime) {
            return false;
        }
        time = transformedTime;
    }
#endif
    
    if (_imp->currentRoto.first && node == _imp->currentRoto.first->getNode()) {
        
        if ( _imp->currentRoto.second && _imp->currentRoto.first->isSettingsPanelVisible() ) {
            if ( _imp->currentRoto.second->keyRepeat(time, scaleX, scaleY, e) ) {
                _imp->lastOverlayNode = node;
                return true;
            }
        }
    } else {
        //if (_imp->currentTracker.second && _imp->currentTracker.first->isSettingsPanelVisible()) {
        //    if (_imp->currentTracker.second->loseFocus(scaleX, scaleY,e)) {
        //        return true;
        //    }
        //}
        Natron::EffectInstance* effect = node->getLiveInstance();
        assert(effect);
        effect->setCurrentViewportForOverlays_public(_imp->viewer);
        bool didSmthing = effect->onOverlayKeyRepeat_public(time, scaleX,scaleY,k,km);
        if (didSmthing) {
            //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html
            // if the instance returns kOfxStatOK, the host should not pass the pen motion
            
            // to any other interactive object it may own that shares the same view.
            _imp->lastOverlayNode = node;
            return true;
        }
    }
    return false;
}

bool
ViewerTab::notifyOverlaysKeyRepeat(double scaleX,
                                   double scaleY,
                                   QKeyEvent* e)
{
    if ( !_imp->app || _imp->app->isClosing() ) {
        return false;
    }
    
    Natron::Key natronKey = QtEnumConvert::fromQtKey( (Qt::Key)e->key() );
    Natron::KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers( e->modifiers() );
    
    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    
    boost::shared_ptr<Natron::Node> lastOverlay = _imp->lastOverlayNode.lock();
    if (lastOverlay) {
        for (std::list<boost::shared_ptr<Natron::Node> >::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if (*it == lastOverlay) {
                if (notifyOverlaysKeyRepeat_internal(*it, scaleX, scaleY, e, natronKey, natronMod)) {
                    return true;
                } else {
                    nodes.erase(it);
                    break;
                }
            }
        }
    }
    

    
    for (std::list<boost::shared_ptr<Natron::Node> >::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        if (notifyOverlaysKeyRepeat_internal(*it, scaleX, scaleY, e, natronKey, natronMod)) {
            return true;
        }
    }

    if (getGui()->getApp()->getOverlayRedrawRequestsCount() > 0) {
        getGui()->getApp()->redrawAllViewers();
    }
    getGui()->getApp()->clearOverlayRedrawRequests();


    return false;
}

bool
ViewerTab::notifyOverlaysFocusGained(double scaleX,
                                     double scaleY)
{
    if ( !_imp->app || _imp->app->isClosing() ) {
        return false;
    }
    
    double time = _imp->app->getTimeLine()->currentFrame();

    
    bool ret = false;
    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        Natron::EffectInstance* effect = (*it)->getLiveInstance();
        assert(effect);
        
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        double transformedTime;
        bool ok = _imp->getTimeTransform(time, 0, *it, getInternalNode(), &transformedTime);
        if (ok) {
            time = transformedTime;
        }
#endif
        
        effect->setCurrentViewportForOverlays_public(_imp->viewer);
        bool didSmthing = effect->onOverlayFocusGained_public(time, scaleX,scaleY);
        if (didSmthing) {
            ret = true;
        }
        
    }
    
    if (!ret && getGui()->getApp()->getOverlayRedrawRequestsCount() > 0) {
        getGui()->getApp()->redrawAllViewers();
    }
    getGui()->getApp()->clearOverlayRedrawRequests();

    return ret;
}

bool
ViewerTab::notifyOverlaysFocusLost(double scaleX,
                                   double scaleY)
{
    if ( !_imp->app || _imp->app->isClosing() ) {
        return false;
    }
    
    double time = _imp->app->getTimeLine()->currentFrame();
    
    bool ret = false;
    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        double transformedTime;
        bool ok = _imp->getTimeTransform(time, 0, *it, getInternalNode(), &transformedTime);
        if (ok) {
            time = transformedTime;
        }
#endif
        
        if (_imp->currentRoto.first && (*it) == _imp->currentRoto.first->getNode()) {
            
            if ( _imp->currentRoto.second && _imp->currentRoto.first->isSettingsPanelVisible() ) {
                _imp->currentRoto.second->focusOut(time);
            }
        } else if (_imp->currentTracker.first && (*it) == _imp->currentTracker.first->getNode()) {
            if ( _imp->currentTracker.second && _imp->currentTracker.first->isSettingsPanelVisible() ) {
                if ( _imp->currentTracker.second->loseFocus(time, scaleX, scaleY) ) {
                    return true;
                }
            }
        }
        
        Natron::EffectInstance* effect = (*it)->getLiveInstance();
        assert(effect);
        
        effect->setCurrentViewportForOverlays_public(_imp->viewer);
        bool didSmthing = effect->onOverlayFocusLost_public(time, scaleX,scaleY);
        if (didSmthing) {
            ret = true;
        }
    }
    
    
    if (!ret && getGui()->getApp()->getOverlayRedrawRequestsCount() > 0) {
        getGui()->getApp()->redrawAllViewers();
    }
    getGui()->getApp()->clearOverlayRedrawRequests();

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
    Natron::ViewerColorSpaceEnum lut = (Natron::ViewerColorSpaceEnum)_imp->viewerNode->getLutType();

    switch (lut) {
    case Natron::eViewerColorSpaceLinear:

        return "Linear(None)";
        break;
    case Natron::eViewerColorSpaceSRGB:

        return "sRGB";
        break;
    case Natron::eViewerColorSpaceRec709:

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
    return _imp->viewerNode->isAutoContrastEnabled();
}

void
ViewerTab::setAutoContrastEnabled(bool b)
{
    _imp->autoContrast->setChecked(b);
    _imp->gainSlider->setEnabled(!b);
    _imp->gainBox->setEnabled(!b);
    _imp->toggleGainButton->setEnabled(!b);
    _imp->gammaSlider->setEnabled(!b);
    _imp->gammaBox->setEnabled(!b);
    _imp->toggleGammaButton->setEnabled(!b);
    _imp->viewerNode->onAutoContrastChanged(b,true);
}

void
ViewerTab::setUserRoI(const RectD & r)
{
    _imp->viewer->setUserRoI(r);
}

void
ViewerTab::setClipToProject(bool b)
{
    onClipToProjectButtonToggle(b);
}

void
ViewerTab::setColorSpace(const std::string & colorSpaceName)
{
    int index = _imp->viewerColorSpace->itemIndex( colorSpaceName.c_str() );

    if (index != -1) {
        _imp->viewerColorSpace->setCurrentIndex(index);
    }
}


void
ViewerTab::setGain(double d)
{
    double fstop = std::log(d) / M_LN2;
    _imp->gainBox->setValue(fstop);
    _imp->gainSlider->seekScalePosition(fstop);
    _imp->viewer->setGain(d);
    _imp->viewerNode->onGainChanged(d);
    _imp->toggleGainButton->setDown(d != 1.);
    _imp->toggleGainButton->setChecked(d != 1.);
    _imp->lastFstopValue = fstop;
}

double
ViewerTab::getGain() const
{
    return _imp->viewerNode->getGain();
}

void
ViewerTab::setGamma(double gamma)
{
    _imp->gammaBox->setValue(gamma);
    _imp->gammaSlider->seekScalePosition(gamma);
    _imp->viewerNode->onGammaChanged(gamma);
    _imp->viewer->setGamma(gamma);
    _imp->toggleGammaButton->setDown(gamma != 1.);
    _imp->toggleGammaButton->setChecked(gamma != 1.);
    _imp->lastGammaValue = gamma;
}

double
ViewerTab::getGamma() const
{
    return _imp->viewerNode->getGamma();
}

void
ViewerTab::setMipMapLevel(int level)
{
    if (level > 0) {
        _imp->renderScaleCombo->setCurrentIndex(level - 1);
    }

    _imp->viewerNode->onMipMapLevelChanged(level);
}

int
ViewerTab::getMipMapLevel() const
{
    return _imp->viewerNode->getMipMapLevel();
}

void
ViewerTab::setRenderScaleActivated(bool act)
{
    onRenderScaleButtonClicked(act);
}

bool
ViewerTab::getRenderScaleActivated() const
{
    return _imp->viewerNode->getMipMapLevel() != 0;
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

Natron::DisplayChannelsEnum
ViewerTab::getChannels() const
{
    return _imp->viewerNode->getChannels(0);
}

std::string
ViewerTab::getChannelsString(Natron::DisplayChannelsEnum c)
{
    switch (c) {
        case Natron::eDisplayChannelsRGB:
            
            return "RGB";
        case Natron::eDisplayChannelsR:
            
            return "R";
        case Natron::eDisplayChannelsG:
            
            return "G";
        case Natron::eDisplayChannelsB:
            
            return "B";
        case Natron::eDisplayChannelsA:
            
            return "A";
        case Natron::eDisplayChannelsY:
            
            return "Luminance";
            break;
        default:
            
            return "";
    }
}

std::string
ViewerTab::getChannelsString() const
{
    Natron::DisplayChannelsEnum c = _imp->viewerNode->getChannels(0);
    return getChannelsString(c);
}

void
ViewerTab::setChannels(const std::string & channelsStr)
{
    int index = _imp->viewerChannels->itemIndex( channelsStr.c_str() );

    if (index != -1) {
        _imp->viewerChannels->setCurrentIndex_no_emit(index);
        setDisplayChannels(index, true);
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
    return _imp->viewerNode;
}

void
ViewerTab::discardInternalNodePointer()
{
    _imp->viewerNode = 0;
}

Gui*
ViewerTab::getGui() const
{
    return _imp->gui;
}


void
ViewerTab::onAutoContrastChanged(bool b)
{
    _imp->gainSlider->setEnabled(!b);
    _imp->gainBox->setEnabled(!b);
    _imp->toggleGainButton->setEnabled(!b);
    _imp->viewerNode->onAutoContrastChanged(b,b);
    _imp->gammaBox->setEnabled(!b);
    _imp->gammaSlider->setEnabled(!b);
    _imp->toggleGammaButton->setEnabled(!b);
    if (!b) {
        _imp->viewerNode->onGainChanged( std::pow(2,_imp->gainBox->value()) );
        _imp->viewerNode->onGammaChanged(_imp->gammaBox->value());
    }
}

void
ViewerTab::onRenderScaleComboIndexChanged(int index)
{
    int level;

    if (_imp->renderScaleActive) {
        level = index + 1;
    } else {
        level = 0;
    }
    _imp->viewerNode->onMipMapLevelChanged(level);
}

void
ViewerTab::onRenderScaleButtonClicked(bool checked)
{
    _imp->activateRenderScale->blockSignals(true);
    _imp->renderScaleActive = checked;
    _imp->activateRenderScale->setDown(checked);
    _imp->activateRenderScale->setChecked(checked);
    _imp->activateRenderScale->blockSignals(false);
    onRenderScaleComboIndexChanged( _imp->renderScaleCombo->activeIndex() );
}

void
ViewerTab::setInfoBarResolution(const Format & f)
{
    _imp->infoWidget[0]->setResolution(f);
    _imp->infoWidget[1]->setResolution(f);
}

void
ViewerTab::createTrackerInterface(NodeGui* n)
{
    boost::shared_ptr<MultiInstancePanel> multiPanel = n->getMultiInstancePanel();
    if (!multiPanel) {
        return;
    }
    std::map<NodeGui*,TrackerGui*>::iterator found = _imp->trackerNodes.find(n);
    if (found != _imp->trackerNodes.end()) {
        return;
    }
    
    boost::shared_ptr<TrackerPanel> trackPanel = boost::dynamic_pointer_cast<TrackerPanel>(multiPanel);

    assert(trackPanel);
    TrackerGui* tracker = new TrackerGui(trackPanel,this);
    std::pair<std::map<NodeGui*,TrackerGui*>::iterator,bool> ret = _imp->trackerNodes.insert( std::make_pair(n,tracker) );
    assert(ret.second);
    if (!ret.second) {
        qDebug() << "ViewerTab::createTrackerInterface() failed";
        delete tracker;

        return;
    }
    QObject::connect( n,SIGNAL( settingsPanelClosed(bool) ),this,SLOT( onTrackerNodeGuiSettingsPanelClosed(bool) ) );
    if ( n->isSettingsPanelVisible() ) {
        setTrackerInterface(n);
    } else {
        tracker->getButtonsBar()->hide();
    }
}

void
ViewerTab::setTrackerInterface(NodeGui* n)
{
    assert(n);
    std::map<NodeGui*,TrackerGui*>::iterator it = _imp->trackerNodes.find(n);
    if ( it != _imp->trackerNodes.end() ) {
        if (_imp->currentTracker.first == n) {
            return;
        }

        ///remove any existing tracker gui
        if (_imp->currentTracker.first != NULL) {
            removeTrackerInterface(_imp->currentTracker.first, false,true);
        }

        ///Add the widgets

        ///if there's a current roto add it before it
        int index;
        if (_imp->currentRoto.second) {
            index = _imp->mainLayout->indexOf( _imp->currentRoto.second->getCurrentButtonsBar() );
            assert(index != -1);
        } else {
            index = _imp->mainLayout->indexOf(_imp->viewerContainer);
        }

        assert(index >= 0);
        QWidget* buttonsBar = it->second->getButtonsBar();
        _imp->mainLayout->insertWidget(index,buttonsBar);
        
        {
            QMutexLocker l(&_imp->visibleToolbarsMutex);
            if (_imp->topToolbarVisible) {
                buttonsBar->show();
            }
        }

        _imp->currentTracker.first = n;
        _imp->currentTracker.second = it->second;
        _imp->viewer->redraw();
    }
}

void
ViewerTab::removeTrackerInterface(NodeGui* n,
                                  bool permanently,
                                  bool removeAndDontSetAnother)
{
    std::map<NodeGui*,TrackerGui*>::iterator it = _imp->trackerNodes.find(n);

    if ( it != _imp->trackerNodes.end() ) {
        if (!_imp->gui) {
            if (permanently) {
                delete it->second;
            }

            return;
        }

        if (_imp->currentTracker.first == n) {
            ///Remove the widgets of the current tracker node

            int buttonsBarIndex = _imp->mainLayout->indexOf( _imp->currentTracker.second->getButtonsBar() );
            assert(buttonsBarIndex >= 0);
            QLayoutItem* buttonsBar = _imp->mainLayout->itemAt(buttonsBarIndex);
            assert(buttonsBar);
            _imp->mainLayout->removeItem(buttonsBar);
            buttonsBar->widget()->hide();

            if (!removeAndDontSetAnother) {
                ///If theres another tracker node, set it as the current tracker interface
                std::map<NodeGui*,TrackerGui*>::iterator newTracker = _imp->trackerNodes.end();
                for (std::map<NodeGui*,TrackerGui*>::iterator it2 = _imp->trackerNodes.begin(); it2 != _imp->trackerNodes.end(); ++it2) {
                    if ( (it2->second != it->second) && it2->first->isSettingsPanelVisible() ) {
                        newTracker = it2;
                        break;
                    }
                }

                _imp->currentTracker.first = 0;
                _imp->currentTracker.second = 0;

                if ( newTracker != _imp->trackerNodes.end() ) {
                    setTrackerInterface(newTracker->first);
                }
            }
        }

        if (permanently) {
            delete it->second;
            _imp->trackerNodes.erase(it);
        }
    }
}

void
ViewerTab::createRotoInterface(NodeGui* n)
{
    RotoGui* roto = new RotoGui( n,this,getRotoGuiSharedData(n) );
    QObject::connect( roto,SIGNAL( selectedToolChanged(int) ),_imp->gui,SLOT( onRotoSelectedToolChanged(int) ) );
    std::pair<std::map<NodeGui*,RotoGui*>::iterator,bool> ret = _imp->rotoNodes.insert( std::make_pair(n,roto) );

    assert(ret.second);
    if (!ret.second) {
        qDebug() << "ViewerTab::createRotoInterface() failed";
        delete roto;

        return;
    }
    QObject::connect( n,SIGNAL( settingsPanelClosed(bool) ),this,SLOT( onRotoNodeGuiSettingsPanelClosed(bool) ) );
    if ( n->isSettingsPanelVisible() ) {
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
    std::map<NodeGui*,RotoGui*>::iterator it = _imp->rotoNodes.find(n);
    if ( it != _imp->rotoNodes.end() ) {
        if (_imp->currentRoto.first == n) {
            return;
        }

        ///remove any existing roto gui
        if (_imp->currentRoto.first != NULL) {
            removeRotoInterface(_imp->currentRoto.first, false,true);
        }

        ///Add the widgets
        QToolBar* toolBar = it->second->getToolBar();
        _imp->viewerLayout->insertWidget(0, toolBar);
        
        {
            QMutexLocker l(&_imp->visibleToolbarsMutex);
            if (_imp->leftToolbarVisible) {
                toolBar->show();
            }
        }

        ///If there's a tracker add it right after the tracker
        int index;
        if (_imp->currentTracker.second) {
            index = _imp->mainLayout->indexOf( _imp->currentTracker.second->getButtonsBar() );
            assert(index != -1);
            ++index;
        } else {
            index = _imp->mainLayout->indexOf(_imp->viewerContainer);
        }
        assert(index >= 0);
        QWidget* buttonsBar = it->second->getCurrentButtonsBar();
        _imp->mainLayout->insertWidget(index,buttonsBar);
        
        {
            QMutexLocker l(&_imp->visibleToolbarsMutex);
            if (_imp->topToolbarVisible) {
                buttonsBar->show();
            }
        }

        QObject::connect( it->second,SIGNAL( roleChanged(int,int) ),this,SLOT( onRotoRoleChanged(int,int) ) );
        _imp->currentRoto.first = n;
        _imp->currentRoto.second = it->second;
        _imp->viewer->redraw();
    }
}

void
ViewerTab::removeRotoInterface(NodeGui* n,
                               bool permanently,
                               bool removeAndDontSetAnother)
{
    std::map<NodeGui*,RotoGui*>::iterator it = _imp->rotoNodes.find(n);

    if ( it != _imp->rotoNodes.end() ) {
        if (_imp->currentRoto.first == n) {
            QObject::disconnect( _imp->currentRoto.second,SIGNAL( roleChanged(int,int) ),this,SLOT( onRotoRoleChanged(int,int) ) );
            ///Remove the widgets of the current roto node
            assert(_imp->viewerLayout->count() > 1);
            QLayoutItem* currentToolBarItem = _imp->viewerLayout->itemAt(0);
            QToolBar* currentToolBar = qobject_cast<QToolBar*>( currentToolBarItem->widget() );
            currentToolBar->hide();
            assert( currentToolBar == _imp->currentRoto.second->getToolBar() );
            _imp->viewerLayout->removeItem(currentToolBarItem);
            int buttonsBarIndex = _imp->mainLayout->indexOf( _imp->currentRoto.second->getCurrentButtonsBar() );
            assert(buttonsBarIndex >= 0);
            QLayoutItem* buttonsBar = _imp->mainLayout->itemAt(buttonsBarIndex);
            assert(buttonsBar);
            _imp->mainLayout->removeItem(buttonsBar);
            buttonsBar->widget()->hide();

            if (!removeAndDontSetAnother) {
                ///If theres another roto node, set it as the current roto interface
                std::map<NodeGui*,RotoGui*>::iterator newRoto = _imp->rotoNodes.end();
                for (std::map<NodeGui*,RotoGui*>::iterator it2 = _imp->rotoNodes.begin(); it2 != _imp->rotoNodes.end(); ++it2) {
                    if ( (it2->second != it->second) && it2->first->isSettingsPanelVisible() ) {
                        newRoto = it2;
                        break;
                    }
                }

                _imp->currentRoto.first = 0;
                _imp->currentRoto.second = 0;

                if ( newRoto != _imp->rotoNodes.end() ) {
                    setRotoInterface(newRoto->first);
                }
            }
        }

        if (permanently) {
            delete it->second;
            _imp->rotoNodes.erase(it);
        }
    }
}

void
ViewerTab::getRotoContext(std::map<NodeGui*,RotoGui*>* rotoNodes,
                          std::pair<NodeGui*,RotoGui*>* currentRoto) const
{
    *rotoNodes = _imp->rotoNodes;
    *currentRoto = _imp->currentRoto;
}

void
ViewerTab::getTrackerContext(std::map<NodeGui*,TrackerGui*>* trackerNodes,
                             std::pair<NodeGui*,TrackerGui*>* currentTracker) const
{
    *trackerNodes = _imp->trackerNodes;
    *currentTracker = _imp->currentTracker;
}

void
ViewerTab::onRotoRoleChanged(int previousRole,
                             int newRole)
{
    RotoGui* roto = qobject_cast<RotoGui*>( sender() );

    if (roto) {
        assert(roto == _imp->currentRoto.second);

        ///Remove the previous buttons bar
        QWidget* previousBar = _imp->currentRoto.second->getButtonsBar( (RotoGui::RotoRoleEnum)previousRole ) ;
        assert(previousBar);
        int buttonsBarIndex = _imp->mainLayout->indexOf(previousBar);
        assert(buttonsBarIndex >= 0);
        _imp->mainLayout->removeItem( _imp->mainLayout->itemAt(buttonsBarIndex) );
        previousBar->hide();


        ///Set the new buttons bar
        int viewerIndex = _imp->mainLayout->indexOf(_imp->viewerContainer);
        assert(viewerIndex >= 0);
        QWidget* currentBar = _imp->currentRoto.second->getButtonsBar( (RotoGui::RotoRoleEnum)newRole );
        assert(currentBar);
        _imp->mainLayout->insertWidget( viewerIndex, currentBar);
        currentBar->show();
        assert(_imp->mainLayout->itemAt(viewerIndex)->widget() == currentBar);
    }
}

void
ViewerTab::updateRotoSelectedTool(int tool,
                                  RotoGui* sender)
{
    if ( _imp->currentRoto.second && (_imp->currentRoto.second != sender) ) {
        _imp->currentRoto.second->setCurrentTool( (RotoGui::RotoToolEnum)tool,false );
    }
}

boost::shared_ptr<RotoGuiSharedData>
ViewerTab::getRotoGuiSharedData(NodeGui* node) const
{
    std::map<NodeGui*,RotoGui*>::const_iterator found = _imp->rotoNodes.find(node);

    if ( found == _imp->rotoNodes.end() ) {
        return boost::shared_ptr<RotoGuiSharedData>();
    } else {
        return found->second->getRotoGuiSharedData();
    }
}

void
ViewerTab::onRotoEvaluatedForThisViewer()
{
    _imp->gui->onViewerRotoEvaluated(this);
}

void
ViewerTab::onRotoNodeGuiSettingsPanelClosed(bool closed)
{
    NodeGui* n = qobject_cast<NodeGui*>( sender() );

    if (n) {
        if (closed) {
            removeRotoInterface(n, false,false);
        } else {
            if (n != _imp->currentRoto.first) {
                setRotoInterface(n);
            }
        }
    }
}

void
ViewerTab::onTrackerNodeGuiSettingsPanelClosed(bool closed)
{
    NodeGui* n = qobject_cast<NodeGui*>( sender() );

    if (n) {
        if (closed) {
            removeTrackerInterface(n, false,false);
        } else {
            if (n != _imp->currentTracker.first) {
                setTrackerInterface(n);
            }
        }
    }
}

void
ViewerTab::notifyAppClosing()
{
    _imp->gui = 0;
    _imp->timeLineGui->discardGuiPointer();
    _imp->app = 0;
    
    for (std::map<NodeGui*,RotoGui*>::iterator it = _imp->rotoNodes.begin() ; it!=_imp->rotoNodes.end(); ++it) {
        it->second->notifyGuiClosing();
    }
}

void
ViewerTab::onCompositingOperatorChangedInternal(Natron::ViewerCompositingOperatorEnum oldOp,Natron::ViewerCompositingOperatorEnum newOp)
{
    if ( (oldOp == eViewerCompositingOperatorNone) && (newOp != eViewerCompositingOperatorNone) ) {
        _imp->viewer->resetWipeControls();
    }
    
    _imp->secondInputImage->setEnabled_natron(newOp != eViewerCompositingOperatorNone);
    if (newOp == eViewerCompositingOperatorNone) {
        _imp->secondInputImage->setCurrentIndex_no_emit(0);
        _imp->viewerNode->setInputB(-1);
    }
    
    if (newOp == eViewerCompositingOperatorNone || !_imp->secondInputImage->getEnabled_natron()  || _imp->secondInputImage->activeIndex() == 0) {
        manageSlotsForInfoWidget(1, false);
        _imp->infoWidget[1]->hide();
    } else if (newOp != eViewerCompositingOperatorNone) {
        manageSlotsForInfoWidget(1, true);
        _imp->infoWidget[1]->show();
    }
    
    _imp->viewer->updateGL();
}

void
ViewerTab::onCompositingOperatorIndexChanged(int index)
{
    ViewerCompositingOperatorEnum newOp,oldOp;
    {
        QMutexLocker l(&_imp->compOperatorMutex);
        oldOp = _imp->compOperator;
        switch (index) {
        case 0:
            _imp->compOperator = eViewerCompositingOperatorNone;
            _imp->secondInputImage->setEnabled_natron(false);
            manageSlotsForInfoWidget(1, false);
            _imp->infoWidget[1]->hide();
            break;
        case 1:
            _imp->compOperator = eViewerCompositingOperatorOver;
            break;
        case 2:
            _imp->compOperator = eViewerCompositingOperatorUnder;
            break;
        case 3:
            _imp->compOperator = eViewerCompositingOperatorMinus;
            break;
        case 4:
            _imp->compOperator = eViewerCompositingOperatorWipe;
            break;
        default:
            break;
        }
        newOp = _imp->compOperator;
    }

    onCompositingOperatorChangedInternal(oldOp, newOp);
}

void
ViewerTab::setCompositingOperator(Natron::ViewerCompositingOperatorEnum op)
{
    int comboIndex;

    switch (op) {
    case Natron::eViewerCompositingOperatorNone:
        comboIndex = 0;
        break;
    case Natron::eViewerCompositingOperatorOver:
        comboIndex = 1;
        break;
    case Natron::eViewerCompositingOperatorUnder:
        comboIndex = 2;
        break;
    case Natron::eViewerCompositingOperatorMinus:
        comboIndex = 3;
        break;
    case Natron::eViewerCompositingOperatorWipe:
        comboIndex = 4;
        break;
    default:
        break;
    }
    Natron::ViewerCompositingOperatorEnum oldOp;
    {
        QMutexLocker l(&_imp->compOperatorMutex);
        oldOp = _imp->compOperator;
        _imp->compOperator = op;
        
    }
    _imp->compositingOperator->setCurrentIndex_no_emit(comboIndex);
    onCompositingOperatorChangedInternal(oldOp, op);
    
    

}

ViewerCompositingOperatorEnum
ViewerTab::getCompositingOperator() const
{
    QMutexLocker l(&_imp->compOperatorMutex);

    return _imp->compOperator;
}

void
ViewerTab::setInputA(int index)
{
    InputNamesMap::iterator found = _imp->inputNamesMap.find(index);
    if (found == _imp->inputNamesMap.end()) {
        return;
    }
    
    int comboboxIndex = _imp->firstInputImage->itemIndex(found->second.name);
    if (comboboxIndex == -1) {
        return;
    }
    _imp->firstInputImage->setCurrentIndex(comboboxIndex);
    _imp->viewerNode->setInputA(index);
    _imp->viewerNode->renderCurrentFrame(true);
    
}

void
ViewerTab::setInputB(int index)
{
    InputNamesMap::iterator found = _imp->inputNamesMap.find(index);
    if (found == _imp->inputNamesMap.end()) {
        return;
    }
    
    int comboboxIndex = _imp->secondInputImage->itemIndex(found->second.name);
    if (comboboxIndex == -1) {
        return;
    }
    _imp->secondInputImage->setCurrentIndex(comboboxIndex);
    _imp->viewerNode->setInputB(index);
    _imp->viewerNode->renderCurrentFrame(true);
}

///Called when the user change the combobox choice
void
ViewerTab::onFirstInputNameChanged(const QString & text)
{
    int inputIndex = -1;

    for (InputNamesMap::iterator it = _imp->inputNamesMap.begin(); it != _imp->inputNamesMap.end(); ++it) {
        if (it->second.name == text) {
            inputIndex = it->first;
            break;
        }
    }
    _imp->viewerNode->setInputA(inputIndex);
    _imp->viewerNode->renderCurrentFrame(true);
}

///Called when the user change the combobox choice
void
ViewerTab::onSecondInputNameChanged(const QString & text)
{
    int inputIndex = -1;

    for (InputNamesMap::iterator it = _imp->inputNamesMap.begin(); it != _imp->inputNamesMap.end(); ++it) {
        if (it->second.name == text) {
            inputIndex = it->first;
            break;
        }
    }
    _imp->viewerNode->setInputB(inputIndex);
    if (inputIndex == -1) {
        manageSlotsForInfoWidget(1, false);
        //setCompositingOperator(Natron::eViewerCompositingOperatorNone);
        _imp->infoWidget[1]->hide();
    } else {
        if ( !_imp->infoWidget[1]->isVisible() ) {
            _imp->infoWidget[1]->show();
            manageSlotsForInfoWidget(1, true);
            _imp->secondInputImage->setEnabled_natron(true);
            if (_imp->compOperator == Natron::eViewerCompositingOperatorNone) {
                _imp->viewer->resetWipeControls();
                setCompositingOperator(Natron::eViewerCompositingOperatorWipe);
            }
        }
    }
    _imp->viewerNode->renderCurrentFrame(true);
}

///This function is called only when the user changed inputs on the node graph
void
ViewerTab::onActiveInputsChanged()
{
    int activeInputs[2];

    _imp->viewerNode->getActiveInputs(activeInputs[0], activeInputs[1]);
    InputNamesMap::iterator foundA = _imp->inputNamesMap.find(activeInputs[0]);
    if ( foundA != _imp->inputNamesMap.end() ) {
        int indexInA = _imp->firstInputImage->itemIndex(foundA->second.name);
        assert(indexInA != -1);
        if (indexInA != -1) {
            _imp->firstInputImage->setCurrentIndex_no_emit(indexInA);
        }
    } else {
        _imp->firstInputImage->setCurrentIndex_no_emit(0);
    }

    Natron::ViewerCompositingOperatorEnum op = getCompositingOperator();
    _imp->secondInputImage->setEnabled_natron(op != Natron::eViewerCompositingOperatorNone);

    InputNamesMap::iterator foundB = _imp->inputNamesMap.find(activeInputs[1]);
    if ( foundB != _imp->inputNamesMap.end() ) {
        int indexInB = _imp->secondInputImage->itemIndex(foundB->second.name);

        assert(indexInB != -1);
        _imp->secondInputImage->setCurrentIndex_no_emit(indexInB);
    } else {
        _imp->secondInputImage->setCurrentIndex_no_emit(0);
    }

    if (op == eViewerCompositingOperatorNone || !_imp->secondInputImage->getEnabled_natron()  || _imp->secondInputImage->activeIndex() == 0) {
        manageSlotsForInfoWidget(1, false);
        _imp->infoWidget[1]->hide();
    } else if (op != eViewerCompositingOperatorNone) {
        manageSlotsForInfoWidget(1, true);
        _imp->infoWidget[1]->show();
    }
    
    bool autoWipe = appPTR->getCurrentSettings()->isAutoWipeEnabled();
    
    /*if ( ( (activeInputs[0] == -1) || (activeInputs[1] == -1) ) //only 1 input is valid
         && ( op != eViewerCompositingOperatorNone) ) {
        //setCompositingOperator(eViewerCompositingOperatorNone);
        _imp->infoWidget[1]->hide();
        manageSlotsForInfoWidget(1, false);
        // _imp->secondInputImage->setEnabled_natron(false);
    }
    else*/ if ( autoWipe && (activeInputs[0] != -1) && (activeInputs[1] != -1) && (activeInputs[0] != activeInputs[1])
                && (op == eViewerCompositingOperatorNone) ) {
        _imp->viewer->resetWipeControls();
        setCompositingOperator(Natron::eViewerCompositingOperatorWipe);
    }
    
}

void
ViewerTab::onClipPreferencesChanged()
{
    //Try to set auto-fps if it is enabled
    if (_imp->fpsLocked) {
        
        int activeInputs[2];
        
        _imp->viewerNode->getActiveInputs(activeInputs[0], activeInputs[1]);
        EffectInstance* input0 = activeInputs[0] != - 1 ? _imp->viewerNode->getInput(activeInputs[0]) : 0;
        if (input0) {
            _imp->fpsBox->setValue(input0->getPreferredFrameRate());
        } else {
            EffectInstance* input1 = activeInputs[1] != - 1 ? _imp->viewerNode->getInput(activeInputs[1]) : 0;
            if (input1) {
                _imp->fpsBox->setValue(input1->getPreferredFrameRate());
            } else {
                _imp->fpsBox->setValue(getGui()->getApp()->getProjectFrameRate());
            }
        }
        onSpinboxFpsChanged(_imp->fpsBox->value());
    }
    refreshLayerAndAlphaChannelComboBox();
}

void
ViewerTab::onInputChanged(int inputNb)
{
    ///rebuild the name maps
    NodePtr inp;
    const std::vector<boost::shared_ptr<Natron::Node> > &inputs  = _imp->viewerNode->getNode()->getGuiInputs();
    if (inputNb >= 0 && inputNb < (int)inputs.size()) {
        if (inputs[inputNb]) {
            inp = inputs[inputNb];
        }
    }
    

    if (inp) {
        InputNamesMap::iterator found = _imp->inputNamesMap.find(inputNb);
        if ( found != _imp->inputNamesMap.end() ) {
            NodePtr input = found->second.input.lock();
            if (!input) {
                return;
            }
            const std::string & curInputName = input->getLabel();
            found->second.input = inp;
            int indexInA = _imp->firstInputImage->itemIndex( curInputName.c_str() );
            int indexInB = _imp->secondInputImage->itemIndex( curInputName.c_str() );
            assert(indexInA != -1 && indexInB != -1);
            found->second.name = inp->getLabel().c_str();
            _imp->firstInputImage->setItemText(indexInA, found->second.name);
            _imp->secondInputImage->setItemText(indexInB, found->second.name);
        } else {
            InputName inpName;
            inpName.input = inp;
            inpName.name = inp->getLabel().c_str();
            _imp->inputNamesMap.insert( std::make_pair(inputNb,inpName) );
            _imp->firstInputImage->addItem(inpName.name);
            _imp->secondInputImage->addItem(inpName.name);
        }
    } else {
        InputNamesMap::iterator found = _imp->inputNamesMap.find(inputNb);

        ///The input has been disconnected
        if ( found != _imp->inputNamesMap.end() ) {
            
            NodePtr input = found->second.input.lock();
            if (!input) {
                return;
            }
            const std::string & curInputName = input->getLabel();
            _imp->firstInputImage->blockSignals(true);
            _imp->secondInputImage->blockSignals(true);
            _imp->firstInputImage->removeItem( curInputName.c_str() );
            _imp->secondInputImage->removeItem( curInputName.c_str() );
            _imp->firstInputImage->blockSignals(false);
            _imp->secondInputImage->blockSignals(false);
            _imp->inputNamesMap.erase(found);
        }
    }
    
    //refreshLayerAndAlphaChannelComboBox();
}

void
ViewerTab::onInputNameChanged(int inputNb,
                              const QString & name)
{
    InputNamesMap::iterator found = _imp->inputNamesMap.find(inputNb);

    assert( found != _imp->inputNamesMap.end() );
    int indexInA = _imp->firstInputImage->itemIndex(found->second.name);
    int indexInB = _imp->secondInputImage->itemIndex(found->second.name);
    assert(indexInA != -1 && indexInB != -1);
    _imp->firstInputImage->setItemText(indexInA, name);
    _imp->secondInputImage->setItemText(indexInB, name);
    found->second.name = name;
}

void
ViewerTab::manageSlotsForInfoWidget(int textureIndex,
                                    bool connect)
{
    RenderEngine* engine = _imp->viewerNode->getRenderEngine();
    assert(engine);
    if (connect) {
        QObject::connect( engine, SIGNAL( fpsChanged(double,double) ), _imp->infoWidget[textureIndex], SLOT( setFps(double,double) ) );
        QObject::connect( engine,SIGNAL( renderFinished(int) ),_imp->infoWidget[textureIndex],SLOT( hideFps() ) );
    } else {
        QObject::disconnect( engine, SIGNAL( fpsChanged(double,double) ), _imp->infoWidget[textureIndex],
                            SLOT( setFps(double,double) ) );
        QObject::disconnect( engine,SIGNAL( renderFinished(int) ),_imp->infoWidget[textureIndex],SLOT( hideFps() ) );
    }
}

void
ViewerTab::setImageFormat(int textureIndex,const Natron::ImageComponents& components,Natron::ImageBitDepthEnum depth)
{
    _imp->infoWidget[textureIndex]->setImageFormat(components,depth);
}

void
ViewerTab::onFrameRangeEditingFinished()
{
    QString text = _imp->frameRangeEdit->text();
    ///try to parse the frame range, if failed set it back to what the timeline currently is
    int i = 0;
    QString firstStr;

    while ( i < text.size() && text.at(i).isDigit() ) {
        firstStr.push_back( text.at(i) );
        ++i;
    }

    ///advance the marker to the second digit if any
    while ( i < text.size() && !text.at(i).isDigit() ) {
        ++i;
    }
    
    int curLeft,curRight;
    getTimelineBounds(&curLeft, &curRight);

    bool ok;
    int first = firstStr.toInt(&ok);
    if (!ok) {
        QString text = QString("%1 - %2").arg( curLeft ).arg( curRight );
        _imp->frameRangeEdit->setText(text);
        _imp->frameRangeEdit->adjustSize();

        return;
    }

    if ( i == text.size() ) {
        ///there's no second marker, set the timeline's boundaries to be the same frame
        setTimelineBounds(first, first);
    } else {
        QString secondStr;
        while ( i < text.size() && text.at(i).isDigit() ) {
            secondStr.push_back( text.at(i) );
            ++i;
        }
        int second = secondStr.toInt(&ok);
        if (!ok) {
            ///there's no second marker, set the timeline's boundaries to be the same frame
            setTimelineBounds(first, first);
        } else {
            setTimelineBounds(first, second);
        }
    }
    _imp->frameRangeEdit->adjustSize();
}


void
ViewerTab::onCanSetFPSLabelClicked(bool toggled)
{
    _imp->canEditFpsBox->setChecked(toggled);
    onCanSetFPSClicked(toggled);
}

void
ViewerTab::onCanSetFPSClicked(bool toggled)
{
    _imp->fpsBox->setReadOnly(!toggled);
    {
        QMutexLocker l(&_imp->fpsLockedMutex);
        _imp->fpsLocked = !toggled;
    }

}

void
ViewerTab::setFPSLocked(bool fpsLocked)
{
    _imp->canEditFpsBox->setChecked(!fpsLocked);
    onCanSetFPSClicked(!fpsLocked);
}

void
ViewerTab::onTimelineBoundariesChanged(SequenceTime first,
                                       SequenceTime second)
{
    QString text = QString("%1 - %2").arg(first).arg(second);

    _imp->frameRangeEdit->setText(text);
    _imp->frameRangeEdit->adjustSize();
}


bool
ViewerTab::isFPSLocked() const
{
    QMutexLocker k(&_imp->fpsLockedMutex);
    
    return _imp->fpsLocked;
}

void
ViewerTab::connectToViewerCache()
{
    _imp->timeLineGui->connectSlotsToViewerCache();
}

void
ViewerTab::disconnectFromViewerCache()
{
    _imp->timeLineGui->disconnectSlotsFromViewerCache();
}

void
ViewerTab::clearTimelineCacheLine()
{
    if (_imp->timeLineGui) {
        _imp->timeLineGui->clearCachedFrames();
    }
}

void
ViewerTab::toggleInfobarVisbility()
{
    bool visible;
    {
        QMutexLocker l(&_imp->visibleToolbarsMutex);
        visible = !_imp->infobarVisible;
    }
    setInfobarVisible(visible);
}

void
ViewerTab::togglePlayerVisibility()
{
    bool visible;
    {
        QMutexLocker l(&_imp->visibleToolbarsMutex);
        visible = !_imp->playerVisible;
    }
    setPlayerVisible(visible);
}

void
ViewerTab::toggleTimelineVisibility()
{
    bool visible;
    {
        QMutexLocker l(&_imp->visibleToolbarsMutex);
        visible = !_imp->timelineVisible;
    }
    setTimelineVisible(visible);
}

void
ViewerTab::toggleLeftToolbarVisiblity()
{
    bool visible;
    {
        QMutexLocker l(&_imp->visibleToolbarsMutex);
        visible = !_imp->leftToolbarVisible;
    }
    setLeftToolbarVisible(visible);
}

void
ViewerTab::toggleRightToolbarVisibility()
{
    bool visible;
    {
        QMutexLocker l(&_imp->visibleToolbarsMutex);
        visible =  !_imp->rightToolbarVisible;
    }
    setRightToolbarVisible(visible);
}

void
ViewerTab::toggleTopToolbarVisibility()
{
    bool visible;
    {
        QMutexLocker l(&_imp->visibleToolbarsMutex);
        visible = !_imp->topToolbarVisible;
    }
    setTopToolbarVisible(visible);
}

void
ViewerTab::setLeftToolbarVisible(bool visible)
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    _imp->leftToolbarVisible = visible;
    if (_imp->currentRoto.second) {
        _imp->currentRoto.second->getToolBar()->setVisible(_imp->leftToolbarVisible);
    }
}

void
ViewerTab::setRightToolbarVisible(bool visible)
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    _imp->rightToolbarVisible = visible;
}

void
ViewerTab::setTopToolbarVisible(bool visible)
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    _imp->topToolbarVisible = visible;
    _imp->firstSettingsRow->setVisible(_imp->topToolbarVisible);
    _imp->secondSettingsRow->setVisible(_imp->topToolbarVisible);
    if (_imp->currentRoto.second) {
        _imp->currentRoto.second->getCurrentButtonsBar()->setVisible(_imp->topToolbarVisible);
    }
    if (_imp->currentTracker.second) {
        _imp->currentTracker.second->getButtonsBar()->setVisible(_imp->topToolbarVisible);
    }
}

void
ViewerTab::setPlayerVisible(bool visible)
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    _imp->playerVisible = visible;
    _imp->playerButtonsContainer->setVisible(_imp->playerVisible);

}

void
ViewerTab::setTimelineVisible(bool visible)
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    _imp->timelineVisible = visible;
    _imp->timeLineGui->setVisible(_imp->timelineVisible);

}

void
ViewerTab::setInfobarVisible(bool visible)
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    _imp->infobarVisible = visible;
    for (int i = 0; i < 2; ++i) {
        if (i == 1) {
            int inputIndex = -1;
            
            for (InputNamesMap::iterator it = _imp->inputNamesMap.begin(); it != _imp->inputNamesMap.end(); ++it) {
                if (it->second.name == _imp->secondInputImage->getCurrentIndexText()) {
                    inputIndex = it->first;
                    break;
                }
            }
            if (getCompositingOperator() == eViewerCompositingOperatorNone || inputIndex == -1) {
                continue;
            }
        }
        
        _imp->infoWidget[i]->setVisible(_imp->infobarVisible);
    }

}

void
ViewerTab::showAllToolbars()
{
    if (!isTopToolbarVisible()) {
        toggleTopToolbarVisibility();
    }
    if (!isRightToolbarVisible()) {
        toggleRightToolbarVisibility();
    }
    if (!isLeftToolbarVisible()) {
        toggleLeftToolbarVisiblity();
    }
    if (!isInfobarVisible()) {
        toggleInfobarVisbility();
    }
    if (!isPlayerVisible()) {
        togglePlayerVisibility();
    }
    if (!isTimelineVisible()) {
        toggleTimelineVisibility();
    }
}

void
ViewerTab::hideAllToolbars()
{
    if (isTopToolbarVisible()) {
        toggleTopToolbarVisibility();
    }
    if (isRightToolbarVisible()) {
        toggleRightToolbarVisibility();
    }
    if (isLeftToolbarVisible()) {
        toggleLeftToolbarVisiblity();
    }
    if (isInfobarVisible()) {
        toggleInfobarVisbility();
    }
    if (isPlayerVisible()) {
        togglePlayerVisibility();
    }
    if (isTimelineVisible()) {
        toggleTimelineVisibility();
    }
}

bool
ViewerTab::isInfobarVisible() const
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    return _imp->infobarVisible;
}

bool
ViewerTab::isTopToolbarVisible() const
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    return _imp->topToolbarVisible;
}

bool
ViewerTab::isPlayerVisible() const
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    return _imp->playerVisible;
}

bool
ViewerTab::isTimelineVisible() const
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    return _imp->timelineVisible;
}

bool
ViewerTab::isLeftToolbarVisible() const
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    return _imp->leftToolbarVisible;
}

bool
ViewerTab::isRightToolbarVisible() const
{
    QMutexLocker l(&_imp->visibleToolbarsMutex);
    return _imp->rightToolbarVisible;
}

void
ViewerTab::setAsFileDialogViewer()
{
    _imp->isFileDialogViewer = true;
}

bool
ViewerTab::isFileDialogViewer() const
{
    return _imp->isFileDialogViewer;
}

void
ViewerTab::setCustomTimeline(const boost::shared_ptr<TimeLine>& timeline)
{
    _imp->timeLineGui->setTimeline(timeline);
    manageTimelineSlot(true,timeline);
}

void
ViewerTab::manageTimelineSlot(bool disconnectPrevious,const boost::shared_ptr<TimeLine>& timeline)
{
    if (disconnectPrevious) {
        boost::shared_ptr<TimeLine> previous = _imp->timeLineGui->getTimeline();
        QObject::disconnect( _imp->nextKeyFrame_Button,SIGNAL( clicked(bool) ),previous.get(), SLOT( goToNextKeyframe() ) );
        QObject::disconnect( _imp->previousKeyFrame_Button,SIGNAL( clicked(bool) ),previous.get(), SLOT( goToPreviousKeyframe() ) );
        QObject::disconnect( previous.get(),SIGNAL( frameChanged(SequenceTime,int) ),
                         this, SLOT( onTimeLineTimeChanged(SequenceTime,int) ) );
        

    }
    
    QObject::connect( _imp->nextKeyFrame_Button,SIGNAL( clicked(bool) ),timeline.get(), SLOT( goToNextKeyframe() ) );
    QObject::connect( _imp->previousKeyFrame_Button,SIGNAL( clicked(bool) ),timeline.get(), SLOT( goToPreviousKeyframe() ) );
    QObject::connect( timeline.get(),SIGNAL( frameChanged(SequenceTime,int) ),
                     this, SLOT( onTimeLineTimeChanged(SequenceTime,int) ) );


}

boost::shared_ptr<TimeLine>
ViewerTab::getTimeLine() const
{
    return _imp->timeLineGui->getTimeline();
}

void
ViewerTab::onVideoEngineStopped()
{
    ///Refresh knobs
    if (_imp->gui && _imp->gui->isGUIFrozen()) {
        NodeGraph* graph = _imp->gui->getNodeGraph();
        if (graph && _imp->timeLineGui) {
            boost::shared_ptr<TimeLine> timeline = _imp->timeLineGui->getTimeline();
            graph->refreshNodesKnobsAtTime(timeline->currentFrame());
        }
    }
}

void
ViewerTab::onCheckerboardButtonClicked()
{
    {
        QMutexLocker l(&_imp->checkerboardMutex);
        _imp->checkerboardEnabled = !_imp->checkerboardEnabled;
    }
    _imp->checkerboardButton->setDown(_imp->checkerboardEnabled);
    _imp->viewer->redraw();
}

bool ViewerTab::isCheckerboardEnabled() const
{
    QMutexLocker l(&_imp->checkerboardMutex);
    return _imp->checkerboardEnabled;
}

void ViewerTab::setCheckerboardEnabled(bool enabled)
{
    {
        QMutexLocker l(&_imp->checkerboardMutex);
        _imp->checkerboardEnabled = enabled;
    }
    _imp->checkerboardButton->setDown(enabled);
    _imp->checkerboardButton->setChecked(enabled);

}

void
ViewerTab::onSpinboxFpsChanged(double fps)
{
    _imp->viewerNode->getRenderEngine()->setDesiredFPS(fps);
    QMutexLocker k(&_imp->fpsMutex);
    _imp->fps = fps;
}

double
ViewerTab::getDesiredFps() const
{
    QMutexLocker l(&_imp->fpsMutex);
    return _imp->fps;
}

void
ViewerTab::setDesiredFps(double fps)
{
    {
        QMutexLocker l(&_imp->fpsMutex);
        _imp->fps = fps;
    }
    _imp->fpsBox->setValue(fps);
    _imp->viewerNode->getRenderEngine()->setDesiredFPS(fps);
}

void
ViewerTab::setProjection(double zoomLeft, double zoomBottom, double zoomFactor, double zoomAspectRatio)
{
    
    _imp->viewer->setProjection(zoomLeft, zoomBottom, zoomFactor, zoomAspectRatio);
    QString str = QString::number(std::floor(zoomFactor * 100 + 0.5));
    str.append( QChar('%') );
    str.prepend("  ");
    str.append("  ");
    _imp->zoomCombobox->setCurrentText_no_emit(str);
}

void
ViewerTab::onViewerRenderingStarted()
{
    
    if (!_imp->ongoingRenderCount) {
        _imp->refreshButton->setIcon(_imp->iconRefreshOn);
    }
    ++_imp->ongoingRenderCount;
}

void
ViewerTab::onViewerRenderingStopped()
{
    --_imp->ongoingRenderCount;
    if (!_imp->ongoingRenderCount) {
        _imp->refreshButton->setIcon(_imp->iconRefreshOff);
    }
}

void
ViewerTab::setTurboButtonDown(bool down)
{
    _imp->turboButton->setDown(down);
    _imp->turboButton->setChecked(down);
}

void 
ViewerTab::redrawGLWidgets()
{
	_imp->viewer->updateGL();
	_imp->timeLineGui->updateGL();
}

void
ViewerTab::getTimelineBounds(int* left,int* right) const
{
    return _imp->timeLineGui->getBounds(left, right);
}

void
ViewerTab::setTimelineBounds(int left,int right)
{
    _imp->timeLineGui->setBoundaries(left, right);
}

void ViewerTab::centerOn(SequenceTime left, SequenceTime right)
{
    _imp->timeLineGui->centerOn(left, right);
}

void ViewerTab::centerOn_tripleSync(SequenceTime left, SequenceTime right)
{
    _imp->timeLineGui->centerOn_tripleSync(left, right);
}

void
ViewerTab::setFrameRangeEdited(bool edited)
{
    _imp->timeLineGui->setFrameRangeEdited(edited);
}


void
ViewerTab::setFrameRange(int left,int right)
{
    setTimelineBounds(left, right);
    onTimelineBoundariesChanged(left, right);
    _imp->timeLineGui->recenterOnBounds();
}

void
ViewerTab::onInternalNodeLabelChanged(const QString& name)
{
    TabWidget* parent = dynamic_cast<TabWidget*>(parentWidget() );
    if (parent) {
        setLabel(name.toStdString());
        parent->setTabLabel(this, name);
    }
}

void
ViewerTab::onInternalNodeScriptNameChanged(const QString& /*name*/)
{
    // always running in the main thread
    std::string newName = _imp->viewerNode->getNode()->getFullyQualifiedName();
    std::string oldName = getScriptName();
    
    for (std::size_t i = 0; i < newName.size(); ++i) {
        if (newName[i] == '.') {
            newName[i] = '_';
        }
    }

    
    assert( qApp && qApp->thread() == QThread::currentThread() );
    getGui()->unregisterTab(this);
    setScriptName(newName);
    getGui()->registerTab(this,this);
    TabWidget* parent = dynamic_cast<TabWidget*>(parentWidget() );
    if (parent) {
        parent->onTabScriptNameChanged(this, oldName, newName);
    }
}

#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
bool
ViewerTabPrivate::getOverlayTransform(double time,
                                      int view,
                                      const boost::shared_ptr<Natron::Node>& target,
                                      Natron::EffectInstance* currentNode,
                                      Transform::Matrix3x3* transform) const
{
    if (currentNode == target->getLiveInstance()) {
        return true;
    }
    RenderScale s;
    s.x = s.y = 1.;
    Natron::EffectInstance* input = 0;
    Natron::StatusEnum stat = eStatusReplyDefault;
    Transform::Matrix3x3 mat;
    if (!currentNode->getNode()->isNodeDisabled() && currentNode->getCanTransform()) {
        stat = currentNode->getTransform_public(time, s, view, &input, &mat);
    }
    if (stat == eStatusFailed) {
        return false;
    } else if (stat == eStatusReplyDefault) {
        //No transfo matrix found, pass to the input...
        
        ///Test all inputs recursively, going from last to first, preferring non optional inputs.
        std::list<Natron::EffectInstance*> nonOptionalInputs;
        std::list<Natron::EffectInstance*> optionalInputs;
        int maxInp = currentNode->getMaxInputCount();
        
        ///We cycle in reverse by default. It should be a setting of the application.
        ///In this case it will return input B instead of input A of a merge for example.
        for (int i = maxInp - 1; i >= 0; --i) {
            Natron::EffectInstance* inp = currentNode->getInput(i);
            bool optional = currentNode->isInputOptional(i);
            if (inp) {
                if (optional) {
                    optionalInputs.push_back(inp);
                } else {
                    nonOptionalInputs.push_back(inp);
                }
            }
        }
        
        if (nonOptionalInputs.empty() && optionalInputs.empty()) {
            return false;
        }
        
        ///Cycle through all non optional inputs first
        for (std::list<Natron::EffectInstance*> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
            mat = Transform::Matrix3x3(1,0,0,0,1,0,0,0,1);
            bool isOk = getOverlayTransform(time, view, target, *it, &mat);
            if (isOk) {
                *transform = Transform::matMul(*transform, mat);
                return true;
            }
        }
        
        ///Cycle through optional inputs...
        for (std::list<Natron::EffectInstance*> ::iterator it = optionalInputs.begin(); it != optionalInputs.end(); ++it) {
            mat = Transform::Matrix3x3(1,0,0,0,1,0,0,0,1);
            bool isOk = getOverlayTransform(time, view, target, *it, &mat);
            if (isOk) {
                *transform = Transform::matMul(*transform, mat);
                return true;
            }
            
        }
        return false;
    } else {
        
        assert(input);
        double par = input->getPreferredAspectRatio();
        
        //The mat is in pixel coordinates, though
        mat = Transform::matMul(Transform::matPixelToCanonical(par, 1, 1, false),mat);
        mat = Transform::matMul(mat,Transform::matCanonicalToPixel(par, 1, 1, false));
        *transform = Transform::matMul(*transform, mat);
        bool isOk = getOverlayTransform(time, view, target, input, transform);
        return isOk;
    }
    return false;
    
}

static double transformTimeForNode(Natron::EffectInstance* currentNode, double inTime)
{
    U64 nodeHash = currentNode->getHash();
    EffectInstance::FramesNeededMap framesNeeded = currentNode->getFramesNeeded_public(nodeHash, inTime, 0, 0);
    EffectInstance::FramesNeededMap::iterator foundInput0 = framesNeeded.find(0);
    if (foundInput0 == framesNeeded.end()) {
        return inTime;
    }
    
    std::map<int,std::vector<OfxRangeD> >::iterator foundView0 = foundInput0->second.find(0);
    if (foundView0 == foundInput0->second.end()) {
        return inTime;
    }
    
    if (foundView0->second.empty()) {
        return inTime;
    } else {
        return (foundView0->second.front().min);
    }
}

bool
ViewerTabPrivate::getTimeTransform(double time,
                                   int view,
                                   const boost::shared_ptr<Natron::Node>& target,
                                   Natron::EffectInstance* currentNode,
                                   double *newTime) const
{
    if (currentNode == target->getLiveInstance()) {
        *newTime = time;
        return true;
    }
    
    if (!currentNode->getNode()->isNodeDisabled()) {
        *newTime = transformTimeForNode(currentNode, time);
    } else {
        *newTime = time;
    }
    
    ///Test all inputs recursively, going from last to first, preferring non optional inputs.
    std::list<Natron::EffectInstance*> nonOptionalInputs;
    std::list<Natron::EffectInstance*> optionalInputs;
    int maxInp = currentNode->getMaxInputCount();
    
    ///We cycle in reverse by default. It should be a setting of the application.
    ///In this case it will return input B instead of input A of a merge for example.
    for (int i = maxInp - 1; i >= 0; --i) {
        Natron::EffectInstance* inp = currentNode->getInput(i);
        bool optional = currentNode->isInputOptional(i);
        if (inp) {
            if (optional) {
                optionalInputs.push_back(inp);
            } else {
                nonOptionalInputs.push_back(inp);
            }
        }
    }
    
    if (nonOptionalInputs.empty() && optionalInputs.empty()) {
        return false;
    }
    
    ///Cycle through all non optional inputs first
    for (std::list<Natron::EffectInstance*> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
        double inputTime;
        bool isOk = getTimeTransform(*newTime, view, target, *it, &inputTime);
        if (isOk) {
            *newTime = inputTime;
            return true;
        }
    }
    
    ///Cycle through optional inputs...
    for (std::list<Natron::EffectInstance*> ::iterator it = optionalInputs.begin(); it != optionalInputs.end(); ++it) {
        double inputTime;
        bool isOk = getTimeTransform(*newTime, view, target, *it, &inputTime);
        if (isOk) {
            *newTime = inputTime;
            return true;
        }
        
    }
    return false;
}

#endif

void
ViewerTabPrivate::getComponentsAvailabel(std::set<ImageComponents>* comps) const
{
    int activeInputIdx[2];
    viewerNode->getActiveInputs(activeInputIdx[0], activeInputIdx[1]);
    EffectInstance* activeInput[2] = {0, 0};
    for (int i = 0; i < 2; ++i) {
        activeInput[i] = viewerNode->getInput(activeInputIdx[i]);
        if (activeInput[i]) {
            EffectInstance::ComponentsAvailableMap compsAvailable;
            activeInput[i]->getComponentsAvailable(gui->getApp()->getTimeLine()->currentFrame(), &compsAvailable);
            for (EffectInstance::ComponentsAvailableMap::iterator it = compsAvailable.begin(); it != compsAvailable.end(); ++it) {
                if (it->second.lock()) {
                    comps->insert(it->first);
                }
            }
        }
    }

}

void
ViewerTab::refreshLayerAndAlphaChannelComboBox()
{
    if (!_imp->viewerNode) {
        return;
    }
    
    QString layerCurChoice = _imp->layerChoice->getCurrentIndexText();
    QString alphaCurChoice = _imp->alphaChannelChoice->getCurrentIndexText();
    
    std::set<ImageComponents> components;
    _imp->getComponentsAvailabel(&components);
    
    _imp->layerChoice->clear();
    _imp->alphaChannelChoice->clear();
    
    _imp->layerChoice->addItem("-");
    _imp->alphaChannelChoice->addItem("-");
    
    std::set<ImageComponents>::iterator foundColorIt = components.end();
    std::set<ImageComponents>::iterator foundOtherIt = components.end();
    std::set<ImageComponents>::iterator foundCurIt = components.end();
    std::set<ImageComponents>::iterator foundCurAlphaIt = components.end();
    std::string foundAlphaChannel;
    
    for (std::set<ImageComponents>::iterator it = components.begin(); it != components.end(); ++it) {
        QString layerName(it->getLayerName().c_str());
        QString itemName = layerName + '.' + QString(it->getComponentsGlobalName().c_str());
        _imp->layerChoice->addItem(itemName);
        
        if (itemName == layerCurChoice) {
            foundCurIt = it;
        }
        
        if (layerName == kNatronColorPlaneName) {
            foundColorIt = it;
        } else {
            foundOtherIt = it;
        }
        
        const std::vector<std::string>& channels = it->getComponentsNames();
        for (U32 i = 0; i < channels.size(); ++i) {
            QString itemName = layerName + '.' + QString(channels[i].c_str());
            if (itemName == alphaCurChoice) {
                foundCurAlphaIt = it;
                foundAlphaChannel = channels[i];
            }
            _imp->alphaChannelChoice->addItem(itemName);
        }
        
        if (layerName == kNatronColorPlaneName) {
            //There's RGBA or alpha, set it to A
            std::string alphaChoice;
            if (channels.size() == 4) {
                alphaChoice = channels[3];
            } else if (channels.size() == 1) {
                alphaChoice = channels[0];
            }
            if (!alphaChoice.empty()) {
                _imp->alphaChannelChoice->setCurrentIndex_no_emit(_imp->alphaChannelChoice->count() - 1);
            } else {
                alphaCurChoice = _imp->alphaChannelChoice->itemText(0);
            }
            
            _imp->viewerNode->setAlphaChannel(*it, alphaChoice, false);
        }
        
        
    }
    
    int layerIdx;
    if (foundCurIt == components.end()) {
        layerCurChoice = "-";
    }

    
    
    if (layerCurChoice == "-") {
        
        ///Try to find color plane, otherwise fallback on any other layer
        if (foundColorIt != components.end()) {
            layerCurChoice = QString(foundColorIt->getLayerName().c_str())
            + '.' + QString(foundColorIt->getComponentsGlobalName().c_str());
            foundCurIt = foundColorIt;
            
        } else if (foundOtherIt != components.end()) {
            layerCurChoice = QString(foundOtherIt->getLayerName().c_str())
            + '.' + QString(foundOtherIt->getComponentsGlobalName().c_str());
            foundCurIt = foundOtherIt;
        } else {
            layerCurChoice = "-";
            foundCurIt = components.end();
        }
        
        
    }
    layerIdx = _imp->layerChoice->itemIndex(layerCurChoice);
    assert(layerIdx != -1);

    
    
    _imp->layerChoice->setCurrentIndex_no_emit(layerIdx);
    if (foundCurIt == components.end()) {
        _imp->viewerNode->setActiveLayer(ImageComponents::getNoneComponents(), false);
    } else {
        _imp->viewerNode->setActiveLayer(*foundCurIt, false);
    }
    
    int alphaIdx;
    if (foundCurAlphaIt == components.end() || foundAlphaChannel.empty()) {
        alphaCurChoice = "-";
    }
    
    if (alphaCurChoice == "-") {
        
        ///Try to find color plane, otherwise fallback on any other layer
        if (foundColorIt != components.end() && foundColorIt->getComponentsNames().size() == 4) {
            alphaCurChoice = QString(foundColorIt->getLayerName().c_str())
            + '.' + QString(foundColorIt->getComponentsNames()[3].c_str());
            foundAlphaChannel = foundColorIt->getComponentsNames()[3];
            foundCurAlphaIt = foundColorIt;
            
            
        } else {
            alphaCurChoice = "-";
            foundCurAlphaIt = components.end();
        }
        
        
    }
    
    alphaIdx = _imp->alphaChannelChoice->itemIndex(alphaCurChoice);
    if (alphaIdx == -1) {
        alphaIdx = 0;
    }
    
    _imp->alphaChannelChoice->setCurrentIndex_no_emit(alphaIdx);
    if (foundCurAlphaIt != components.end()) {
        _imp->viewerNode->setAlphaChannel(*foundCurAlphaIt, foundAlphaChannel, false);
    } else {
        _imp->viewerNode->setAlphaChannel(ImageComponents::getNoneComponents(), std::string(), false);
    }
}

void
ViewerTab::onAlphaChannelComboChanged(int index)
{
    std::set<ImageComponents> components;
    _imp->getComponentsAvailabel(&components);
    int i = 1; // because of the "-" choice
    for (std::set<ImageComponents>::iterator it = components.begin(); it != components.end(); ++it) {
        
        const std::vector<std::string>& channels = it->getComponentsNames();
        if (index >= ((int)channels.size() + i)) {
            i += channels.size();
        } else {
            for (U32 j = 0; j < channels.size(); ++j, ++i) {
                if (i == index) {
                    _imp->viewerNode->setAlphaChannel(*it, channels[j], true);
                    return;
                }
            }
            
        }
    }
    _imp->viewerNode->setAlphaChannel(ImageComponents::getNoneComponents(), std::string(), true);
}

void
ViewerTab::onLayerComboChanged(int index)
{
    std::set<ImageComponents> components;
    _imp->getComponentsAvailabel(&components);
    if (index >= (int)(components.size() + 1) || index < 0) {
        qDebug() << "ViewerTab::onLayerComboChanged: invalid index";
        return;
    }
    int i = 1; // because of the "-" choice
    int chanCount = 1; // because of the "-" choice
    for (std::set<ImageComponents>::iterator it = components.begin(); it != components.end(); ++it, ++i) {
        
        chanCount += it->getComponentsNames().size();
        if (i == index) {
            _imp->viewerNode->setActiveLayer(*it, true);
            
            ///If it has an alpha channel, set it
            if (it->getComponentsNames().size() == 4) {
                _imp->alphaChannelChoice->setCurrentIndex_no_emit(chanCount - 1);
                _imp->viewerNode->setAlphaChannel(*it, it->getComponentsNames()[3], true);
            }
            return;
        }
    }
    
    _imp->alphaChannelChoice->setCurrentIndex_no_emit(0);
    _imp->viewerNode->setAlphaChannel(ImageComponents::getNoneComponents(), std::string(), false);
    _imp->viewerNode->setActiveLayer(ImageComponents::getNoneComponents(), true);
    
}

void
ViewerTab::onGainToggled(bool clicked)
{
    double value;
    if (clicked) {
        value = _imp->lastFstopValue;
    } else {
        value = 0;
    }
    _imp->toggleGainButton->setDown(clicked);
    _imp->gainBox->setValue(value);
    _imp->gainSlider->seekScalePosition(value);
    
    double gain = std::pow(2,value);
    _imp->viewer->setGain(gain);
    _imp->viewerNode->onGainChanged(gain);
}


void
ViewerTab::onGainSliderChanged(double v)
{
    if (!_imp->toggleGainButton->isChecked()) {
        _imp->toggleGainButton->setChecked(true);
        _imp->toggleGainButton->setDown(true);
    }
    _imp->gainBox->setValue(v);
    double gain = std::pow(2,v);
    _imp->viewer->setGain(gain);
    _imp->viewerNode->onGainChanged(gain);
    _imp->lastFstopValue = v;
}

void
ViewerTab::onGainSpinBoxValueChanged(double v)
{
    if (!_imp->toggleGainButton->isChecked()) {
        _imp->toggleGainButton->setChecked(true);
        _imp->toggleGainButton->setDown(true);
    }
    _imp->gainSlider->seekScalePosition(v);
    double gain = std::pow(2,v);
    _imp->viewer->setGain(gain);
    _imp->viewerNode->onGainChanged(gain);
    _imp->lastFstopValue = v;
}

void
ViewerTab::onGammaToggled(bool clicked)
{
    double value;
    if (clicked) {
        value = _imp->lastGammaValue;
    } else {
        value = 1.;
    }
    _imp->toggleGammaButton->setDown(clicked);
    _imp->gammaBox->setValue(value);
    _imp->gammaSlider->seekScalePosition(value);
    _imp->viewer->setGamma(value);
    _imp->viewerNode->onGammaChanged(value);
}

void
ViewerTab::onGammaSliderValueChanged(double value)
{
    if (!_imp->toggleGammaButton->isChecked()) {
        _imp->toggleGammaButton->setChecked(true);
        _imp->toggleGammaButton->setDown(true);
    }
    _imp->gammaBox->setValue(value);
    _imp->viewer->setGamma(value);
    _imp->viewerNode->onGammaChanged(value);
    _imp->lastGammaValue = value;
}

void
ViewerTab::onGammaSpinBoxValueChanged(double value)
{
    if (!_imp->toggleGammaButton->isChecked()) {
        _imp->toggleGammaButton->setChecked(true);
        _imp->toggleGammaButton->setDown(true);
    }
    _imp->gammaSlider->seekScalePosition(value);
    _imp->viewer->setGamma(value);
    _imp->viewerNode->onGammaChanged(value);
    _imp->lastGammaValue = value;
}

void
ViewerTab::onGammaSliderEditingFinished(bool hasMovedOnce)
{
    bool autoProxyEnabled = appPTR->getCurrentSettings()->isAutoProxyEnabled();
    if (autoProxyEnabled && hasMovedOnce) {
        getGui()->renderAllViewers();
    }
}

void
ViewerTab::onGainSliderEditingFinished(bool hasMovedOnce)
{
    bool autoProxyEnabled = appPTR->getCurrentSettings()->isAutoProxyEnabled();
    if (autoProxyEnabled && hasMovedOnce) {
        getGui()->renderAllViewers();
    }
}

bool
ViewerTab::isViewersSynchroEnabled() const
{
    return _imp->syncViewerButton->isDown();
}

void
ViewerTab::synchronizeOtherViewersProjection()
{
    assert(_imp->gui);
    _imp->gui->setMasterSyncViewer(this);
    double left,bottom,factor,par;
    _imp->viewer->getProjection(&left, &bottom, &factor, &par);
    const std::list<ViewerTab*>& viewers = _imp->gui->getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        if ((*it) != this) {
            (*it)->getViewer()->setProjection(left, bottom, factor, par);
            (*it)->getInternalNode()->renderCurrentFrame(false);
            
        }
    }
    
}

void
ViewerTab::onSyncViewersButtonPressed(bool clicked)
{

    const std::list<ViewerTab*>& viewers = _imp->gui->getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->_imp->syncViewerButton->setDown(clicked);
        (*it)->_imp->syncViewerButton->setChecked(clicked);
    }
    if (clicked) {
        synchronizeOtherViewersProjection();
    }
}
