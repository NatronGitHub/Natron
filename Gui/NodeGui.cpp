/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

#include "NodeGui.h"

#include <cassert>
#include <algorithm> // min, max
#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QDebug>
#include <QLayout>
#include <QAction>
#include <QtCore/QThread>
#include <QFontMetrics>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QGridLayout>
#include <QCursor>
#include <QtCore/QFile>
#include <QApplication>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include <ofxNatron.h>

#include "Engine/Backdrop.h"
#include "Engine/Image.h"
#include "Engine/Knob.h"
#include "Engine/MergingEnum.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/GroupInput.h"
#include "Engine/GroupOutput.h"
#include "Engine/NodeSerialization.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/PyNode.h"
#include "Engine/PyParameter.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/RotoLayer.h"
#include "Engine/Settings.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewerInstance.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/BackdropGui.h"
#include "Gui/Button.h"
#include "Gui/CurveEditor.h"
#include "Gui/HostOverlay.h"
#include "Gui/DockablePanel.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/Edge.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobGuiString.h"
#include "Gui/QtEnumConvert.h"
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/Menu.h"
#include "Gui/NodeClipBoard.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGraphUndoRedo.h"
#include "Gui/NodeGuiSerialization.h"
#include "Gui/NodeGraphTextItem.h"
#include "Gui/NodeGraphRectItem.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/PreviewThread.h"
#include "Gui/PythonPanels.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/SpinBox.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

#define NATRON_STATE_INDICATOR_OFFSET 5

#define NATRON_EDGE_DROP_TOLERANCE 15

#define NATRON_MAGNETIC_GRID_GRIP_TOLERANCE 20

#define NATRON_MAGNETIC_GRID_RELEASE_DISTANCE 30

#define NATRON_ELLIPSE_WARN_DIAMETER 10


#define NATRON_PLUGIN_ICON_SIZE 20
#define PLUGIN_ICON_OFFSET 2

NATRON_NAMESPACE_ENTER

using std::make_pair;

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#define M_PI_2      1.57079632679489661923132169163975144   /* pi/2           */
#endif

static void
replaceLineBreaksWithHtmlParagraph(QString &txt)
{
    txt.replace( QString::fromUtf8("\n"), QString::fromUtf8("<br />") );
}

static void
getPixmapForMergeOperator(const QString& op,
                          QPixmap* pix)
{
    std::string opstd = op.toStdString();

    for (int i = 0; i <= (int)eMergeXOR; ++i) {
        std::string opStr = Merge::getOperatorString( (MergingFunctionEnum)i );
        if (opStr == opstd) {
            PixmapEnum pixEnum = Merge::getOperatorPixmap( (MergingFunctionEnum)i );
            appPTR->getIcon(pixEnum, TO_DPIX(NATRON_PLUGIN_ICON_SIZE), pix);

            return;
        }
    }
}

NodeGui::NodeGui(QGraphicsItem *parent)
    : QObject()
    , QGraphicsItem(parent)
    , _graph(NULL)
    , _internalNode()
    , _selected(false)
    , _settingNameFromGui(false)
    , _panelOpenedBeforeDeactivate(false)
    , _pluginIcon(NULL)
    , _pluginIconFrame(NULL)
    , _presetIcon(NULL)
    , _nameItem(NULL)
    , _nameFrame(NULL)
    , _resizeHandle(NULL)
    , _boundingBox(NULL)
    , _channelsPixmap(NULL)
    , _previewPixmap(NULL)
    , _previewDataMutex()
    , _previewData( NATRON_PREVIEW_HEIGHT * NATRON_PREVIEW_WIDTH * sizeof(unsigned int) )
    , _previewW(NATRON_PREVIEW_WIDTH)
    , _previewH(NATRON_PREVIEW_HEIGHT)
    , _persistentMessage(NULL)
    , _stateIndicator(NULL)
    , _mergeHintActive(false)
    , _streamIssuesWarning()
    , _disabledTopLeftBtmRight(NULL)
    , _disabledBtmLeftTopRight(NULL)
    , _inputEdges()
    , _outputEdge(NULL)
    , _settingsPanel(NULL)
    , _mainInstancePanel(NULL)
    , _panelCreated(false)
    , _currentColorMutex()
    , _currentColor()
    , _clonedColor()
    , _wasBeginEditCalled(false)
    , positionMutex()
    , _slaveMasterLink(NULL)
    , _masterNodeGui()
    , _knobsLinks()
    , _expressionIndicator()
    , _magnecEnabled()
    , _magnecDistance()
    , _updateDistanceSinceLastMagnec()
    , _distanceSinceLastMagnec()
    , _magnecStartingPos()
    , _nodeLabel()
    , _parentMultiInstance()
    , _mtSafeSizeMutex()
    , _mtSafeWidth(0)
    , _mtSafeHeight(0)
    , _hostOverlay()
    , _undoStack( new QUndoStack() )
    , _overlayLocked(false)
    , _availableViewsIndicator()
    , _passThroughIndicator()
    , identityStateSet(false)
{
}

NodeGui::~NodeGui()
{
}

void
NodeGui::initialize(NodeGraph* dag,
                    const NodePtr & internalNode)
{
    _internalNode = internalNode;
    assert(internalNode);
    _graph = dag;

    NodeGuiPtr thisAsShared = shared_from_this();

    internalNode->setNodeGuiPointer(thisAsShared);

    QObject::connect( internalNode.get(), SIGNAL(labelChanged(QString)), this, SLOT(onInternalNameChanged(QString)) );
    QObject::connect( internalNode.get(), SIGNAL(refreshEdgesGUI()), this, SLOT(refreshEdges()) );
    QObject::connect( internalNode.get(), SIGNAL(knobsInitialized()), this, SLOT(initializeKnobs()) );
    QObject::connect( internalNode.get(), SIGNAL(inputsInitialized()), this, SLOT(initializeInputs()) );
    QObject::connect( internalNode.get(), SIGNAL(previewImageChanged(double)), this, SLOT(updatePreviewImage(double)) );
    QObject::connect( internalNode.get(), SIGNAL(previewRefreshRequested(double)), this, SLOT(forceComputePreview(double)) );
    QObject::connect( internalNode.get(), SIGNAL(deactivated(bool)), this, SLOT(deactivate(bool)) );
    QObject::connect( internalNode.get(), SIGNAL(activated(bool)), this, SLOT(activate(bool)) );
    QObject::connect( internalNode.get(), SIGNAL(inputChanged(int)), this, SLOT(connectEdge(int)) );
    QObject::connect( internalNode.get(), SIGNAL(persistentMessageChanged()), this, SLOT(onPersistentMessageChanged()) );
    QObject::connect( internalNode.get(), SIGNAL(allKnobsSlaved(bool)), this, SLOT(onAllKnobsSlaved(bool)) );
    QObject::connect( internalNode.get(), SIGNAL(knobsLinksChanged()), this, SLOT(onKnobsLinksChanged()) );
    QObject::connect( internalNode.get(), SIGNAL(outputsChanged()), this, SLOT(refreshOutputEdgeVisibility()) );
    QObject::connect( internalNode.get(), SIGNAL(previewKnobToggled()), this, SLOT(onPreviewKnobToggled()) );
    QObject::connect( internalNode.get(), SIGNAL(disabledKnobToggled(bool)), this, SLOT(onDisabledKnobToggled(bool)) );
    QObject::connect( internalNode.get(), SIGNAL(streamWarningsChanged()), this, SLOT(onStreamWarningsChanged()) );
    QObject::connect( internalNode.get(), SIGNAL(nodeExtraLabelChanged(QString)), this, SLOT(refreshNodeText(QString)) );
    QObject::connect( internalNode.get(), SIGNAL(outputLayerChanged()), this, SLOT(onOutputLayerChanged()) );
    QObject::connect( internalNode.get(), SIGNAL(hideInputsKnobChanged(bool)), this, SLOT(onHideInputsKnobValueChanged(bool)) );
    QObject::connect( internalNode.get(), SIGNAL(availableViewsChanged()), this, SLOT(onAvailableViewsChanged()) );
    QObject::connect( internalNode.get(), SIGNAL(rightClickMenuKnobPopulated()), this, SLOT(onRightClickMenuKnobPopulated()) );
    QObject::connect( internalNode.get(), SIGNAL(inputEdgeLabelChanged(int, QString)), this, SLOT(onInputLabelChanged(int,QString)) );
    QObject::connect( internalNode.get(), SIGNAL(inputVisibilityChanged(int)), this, SLOT(onInputVisibilityChanged(int)) );
    QObject::connect( this, SIGNAL(previewImageComputed()), this, SLOT(onPreviewImageComputed()) );
    setCacheMode(DeviceCoordinateCache);

    OutputEffectInstance* isOutput = dynamic_cast<OutputEffectInstance*>( internalNode->getEffectInstance().get() );
    if (isOutput) {
        QObject::connect ( isOutput->getRenderEngine().get(), SIGNAL(refreshAllKnobs()), _graph, SLOT(refreshAllKnobsGui()) );
    }

    InspectorNode* isInspector = dynamic_cast<InspectorNode*>( internalNode.get() );
    if (isInspector) {
        QObject::connect( isInspector, SIGNAL(refreshOptionalState()), this, SLOT(refreshDashedStateOfEdges()) );
    }

    createGui();

    NodePtr parent = internalNode->getParentMultiInstance();
    if (parent) {
        NodeGuiIPtr parentNodeGui_i = parent->getNodeGui();
        NodeGui* parentGui = dynamic_cast<NodeGui*>( parentNodeGui_i.get() );
        assert(parentGui);
        if ( parentGui && parentGui->isSettingsPanelVisible() ) {
            ensurePanelCreated();
            MultiInstancePanelPtr panel = parentGui->getMultiInstancePanel();
            assert(panel);
            panel->onChildCreated(internalNode);
        }
    } else if ( internalNode->getEffectInstance()->isBuiltinTrackerNode() ) {
        ensurePanelCreated();
    }

    //Refresh the merge operator icon
    if (internalNode->getPluginID() == PLUGINID_OFX_MERGE) {
        KnobIPtr knob = internalNode->getKnobByName(kNatronOfxParamStringSublabelName);
        assert(knob);
        KnobString* strKnob = dynamic_cast<KnobString*>( knob.get() );
        if (strKnob) {
            refreshNodeText( QString::fromUtf8( strKnob->getValue().c_str() ) );
        }
    }


    if ( internalNode->makePreviewByDefault() ) {
        ///It calls resize
        togglePreview_internal(false);
    } else {
        int w, h;
        getInitialSize(&w, &h);
        resize(w, h);
    }


    _clonedColor.setRgb(200, 70, 100);


    ///Make the output edge
    EffectInstancePtr iseffect = internalNode->getEffectInstance();
    Backdrop* isBd = dynamic_cast<Backdrop*>( iseffect.get() );
    if ( !isBd && !internalNode->isOutputNode() ) {
        _outputEdge = new Edge( thisAsShared, parentItem() );
    }

    restoreStateAfterCreation();

    ///Link the position of the node to the position of the parent multi-instance
    const std::string parentMultiInstanceName = internalNode->getParentMultiInstanceName();
    if ( !parentMultiInstanceName.empty() ) {
        NodePtr parentNode = internalNode->getGroup()->getNodeByName(parentMultiInstanceName);
        NodeGuiIPtr parentNodeGui_I = parentNode->getNodeGui();
        assert(parentNode && parentNodeGui_I);
        NodeGui* parentNodeGui = dynamic_cast<NodeGui*>( parentNodeGui_I.get() );
        assert(parentNodeGui);
        QObject::connect( parentNodeGui, SIGNAL(positionChanged(int,int)), this, SLOT(onParentMultiInstancePositionChanged(int,int)) );
        QPointF p = parentNodeGui->pos();
        refreshPosition(p.x(), p.y(), true);
    }

    if (internalNode) {
        internalNode->initializeHostOverlays();
    }
} // initialize

bool
NodeGui::getColorFromGrouping(QColor* color)
{
    NodePtr internalNode = getNode();
    if (!internalNode) {
        return false;
    }
    EffectInstancePtr iseffect = internalNode->getEffectInstance();
    if (!iseffect) {
        return false;
    }
    SettingsPtr settings = appPTR->getCurrentSettings();
    float r, g, b;
    Backdrop* isBd = dynamic_cast<Backdrop*>( iseffect.get() );
    std::list<std::string> grouping;

    internalNode->getPluginGrouping(&grouping);
    std::string majGroup = grouping.empty() ? "" : grouping.front();

    if ( iseffect->isReader() ) {
        settings->getReaderColor(&r, &g, &b);
    } else if (isBd) {
        settings->getDefaultBackdropColor(&r, &g, &b);
    } else if ( iseffect->isWriter() ) {
        settings->getWriterColor(&r, &g, &b);
    } else if ( iseffect->isGenerator() ) {
        settings->getGeneratorColor(&r, &g, &b);
    } else if (majGroup == PLUGIN_GROUP_COLOR) {
        settings->getColorGroupColor(&r, &g, &b);
    } else if (majGroup == PLUGIN_GROUP_FILTER) {
        settings->getFilterGroupColor(&r, &g, &b);
    } else if (majGroup == PLUGIN_GROUP_CHANNEL) {
        settings->getChannelGroupColor(&r, &g, &b);
    } else if (majGroup == PLUGIN_GROUP_KEYER) {
        settings->getKeyerGroupColor(&r, &g, &b);
    } else if (majGroup == PLUGIN_GROUP_MERGE) {
        settings->getMergeGroupColor(&r, &g, &b);
    } else if (majGroup == PLUGIN_GROUP_PAINT) {
        settings->getDrawGroupColor(&r, &g, &b);
    } else if (majGroup == PLUGIN_GROUP_TIME) {
        settings->getTimeGroupColor(&r, &g, &b);
    } else if (majGroup == PLUGIN_GROUP_TRANSFORM) {
        settings->getTransformGroupColor(&r, &g, &b);
    } else if (majGroup == PLUGIN_GROUP_MULTIVIEW) {
        settings->getViewsGroupColor(&r, &g, &b);
    } else if (majGroup == PLUGIN_GROUP_DEEP) {
        settings->getDeepGroupColor(&r, &g, &b);
    } else {
        settings->getDefaultNodeColor(&r, &g, &b);
    }
    color->setRgbF( Image::clamp<qreal>(r, 0., 1.),
                    Image::clamp<qreal>(g, 0., 1.),
                    Image::clamp<qreal>(b, 0., 1.) );
    return true;
}

void
NodeGui::restoreStateAfterCreation()
{
    NodePtr internalNode = getNode();
    if (!internalNode) {
        return;
    }

    ///Refresh the disabled knob

    QColor color;
    if ( getColorFromGrouping(&color) ) {
        setCurrentColor(color);
    }
    KnobBoolPtr disabledknob = internalNode->getDisabledKnob();
    if ( disabledknob && disabledknob->getValue() ) {
        onDisabledKnobToggled(true);
    }
    if ( !internalNode->isMultiInstance() ) {
        _nodeLabel = QString::fromUtf8( internalNode->getNodeExtraLabel().c_str() );
        replaceLineBreaksWithHtmlParagraph(_nodeLabel);
    }
    ///Refresh the name in the line edit
    onInternalNameChanged( QString::fromUtf8( internalNode->getLabel().c_str() ) );
    onOutputLayerChanged();
    internalNode->refreshIdentityState();
    onPersistentMessageChanged();
    onKnobsLinksChanged();
}

void
NodeGui::ensurePanelCreated(bool minimized, bool hideUnmodified)
{
    if (_panelCreated) {
        return;
    }
    _panelCreated = true;
    Gui* gui = getDagGui()->getGui();
    QVBoxLayout* propsLayout = gui->getPropertiesLayout();
    assert(propsLayout);
    NodeGuiPtr thisShared = shared_from_this();
    _settingsPanel = createPanel(propsLayout, thisShared);

    initializeKnobs();
    beginEditKnobs();

    if (_settingsPanel) {
        QObject::connect( _settingsPanel, SIGNAL(nameChanged(QString)), this, SLOT(setName(QString)) );
        QObject::connect( _settingsPanel, SIGNAL(closeChanged(bool)), this, SLOT(onSettingsPanelClosed(bool)) );
        QObject::connect( _settingsPanel, SIGNAL(minimized()), this, SLOT(onSettingsPanelMinimized()) );
        QObject::connect( _settingsPanel, SIGNAL(maximized()), this, SLOT(onSettingsPanelMaximized()) );
        QObject::connect( _settingsPanel, SIGNAL(colorChanged(QColor)), this, SLOT(onSettingsPanelColorChanged(QColor)) );

        _graph->getGui()->setNodeViewerInterface(thisShared);
    }

    gui->addNodeGuiToCurveEditor(thisShared);
    gui->addNodeGuiToDopeSheetEditor(thisShared);

    //Ensure panel for all children if multi-instance

    MultiInstancePanelPtr panel = getMultiInstancePanel();
    if (_mainInstancePanel && panel) {
        panel->setRedrawOnSelectionChanged(false);

        /*
         * If there are many children, each children may request for a redraw of the viewer which may
         * very well freeze the UI.
         * We just do one redraw when all children are created
         */
        NodesList children;
        NodePtr internalNode = getNode();
        if (internalNode) {
            internalNode->getChildrenMultiInstance(&children);
        }
        for (NodesList::iterator it = children.begin(); it != children.end(); ++it) {
            NodeGuiIPtr gui_i = (*it)->getNodeGui();
            assert(gui_i);
            NodeGui* gui = dynamic_cast<NodeGui*>( gui_i.get() );
            assert(gui);
            gui->ensurePanelCreated();

            panel->onChildCreated(*it);
        }
        panel->setRedrawOnSelectionChanged(true);
    }

    if (hideUnmodified) {
        _settingsPanel->restoreHideUnmodifiedState(true);
    }

    if (minimized) {
        _settingsPanel->restoreMinimizedState(true);
    }

    const std::list<ViewerTab*>& viewers = getDagGui()->getGui()->getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->getViewer()->updatePersistentMessage();
    }
} // NodeGui::ensurePanelCreated

void
NodeGui::onSettingsPanelClosed(bool closed)
{
    NodePtr internalNode = getNode();
    if (internalNode && internalNode->hasAnyPersistentMessage()) {
        const std::list<ViewerTab*>& viewers = getDagGui()->getGui()->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
            (*it)->getViewer()->updatePersistentMessage();
        }
    }
    Q_EMIT settingsPanelClosed(closed);
}

void
NodeGui::onSettingsPanelMinimized()
{
    NodePtr internalNode = getNode();
    if (internalNode && internalNode->hasAnyPersistentMessage()) {
        const std::list<ViewerTab*>& viewers = getDagGui()->getGui()->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
            (*it)->getViewer()->updatePersistentMessage();
        }
    }
    Q_EMIT settingsPanelMinimized();
}

void
NodeGui::onSettingsPanelMaximized()
{
    NodePtr internalNode = getNode();
    if (internalNode && internalNode->hasAnyPersistentMessage()) {
        const std::list<ViewerTab*>& viewers = getDagGui()->getGui()->getViewersList();
        for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
            (*it)->getViewer()->updatePersistentMessage();
        }
    }
    Q_EMIT settingsPanelMaximized();
}

NodeSettingsPanel*
NodeGui::createPanel(QVBoxLayout* container,
                     const NodeGuiPtr& thisAsShared)
{
    NodeSettingsPanel* panel = 0;
    NodePtr node = getNode();

    if ( node && node->getEffectInstance()->getMakeSettingsPanel() ) {
        assert(container);
        MultiInstancePanelPtr multiPanel;
        if ( node->isTrackerNodePlugin() && node->isMultiInstance() && node->getParentMultiInstanceName().empty() ) {
            multiPanel.reset( new TrackerPanelV1(thisAsShared) );

            ///This is valid only if the node is a multi-instance and this is the main instance.
            ///The "real" panel showed on the gui will be the _settingsPanel, but we still need to create
            ///another panel for the main-instance (hidden) knobs to function properly (and also be showed in the CurveEditor)

            _mainInstancePanel = new NodeSettingsPanel( MultiInstancePanelPtr(), _graph->getGui(),
                                                        thisAsShared, container, container->parentWidget() );
            _mainInstancePanel->blockSignals(true);
            _mainInstancePanel->setClosed(true);
            _mainInstancePanel->initializeKnobs();
        }
        panel = new NodeSettingsPanel( multiPanel, _graph->getGui(), thisAsShared, container, container->parentWidget() );

        if (panel) {
            bool isCreatingPythonGroup = getDagGui()->getGui()->getApp()->isCreatingPythonGroup();
            std::string pluginID = node->getPluginID();
            if ( (pluginID == PLUGINID_NATRON_OUTPUT) ||
                 ( isCreatingPythonGroup && ( pluginID != PLUGINID_NATRON_GROUP) ) ||
                 !node->getParentMultiInstanceName().empty() ) {
                panel->setClosed(true);
            } else {
                _graph->getGui()->addVisibleDockablePanel(panel);
            }
        }
    }

    return panel;
}

void
NodeGui::getSizeWithPreview(int *w,
                            int *h) const
{
    getInitialSize(w, h);
    *w = *w -  (TO_DPIX(NODE_WIDTH) / 2.) + TO_DPIX(NATRON_PREVIEW_WIDTH);
    *h = *h + TO_DPIY(NATRON_PREVIEW_HEIGHT) + 10;
}

void
NodeGui::getInitialSize(int *w,
                        int *h) const
{
    NodePtr internalNode = getNode();
    if (!internalNode) {
        *w = TO_DPIX(NODE_WIDTH);
        *h = TO_DPIY(NODE_HEIGHT);
        return;
    }
    const QString& iconFilePath = internalNode->getPlugin()->getIconFilePath();

    if ( !iconFilePath.isEmpty() && QFile::exists(iconFilePath) && appPTR->getCurrentSettings()->isPluginIconActivatedOnNodeGraph() ) {
        *w = TO_DPIX(NODE_WIDTH) + TO_DPIX(NATRON_PLUGIN_ICON_SIZE) + TO_DPIX(PLUGIN_ICON_OFFSET) * 2;
    } else {
        *w = TO_DPIX(NODE_WIDTH);
    }
    *h = TO_DPIY(NODE_HEIGHT);
}

void
NodeGui::createGui()
{
    int depth = getBaseDepth();

    setZValue(depth);
    NodePtr node = getNode();
    if (!node) {
        return; // throw exception instead?
    }
    int cornerRadiusPx = 0;
    _boundingBox = new NodeGraphRectItem(this, cornerRadiusPx);
    _boundingBox->setZValue(depth);

    if ( mustFrameName() ) {
        _nameFrame = new QGraphicsRectItem(this);
        _nameFrame->setZValue(depth + 1);
    }

    if ( mustAddResizeHandle() ) {
        _resizeHandle = new QGraphicsPolygonItem(this);
        _resizeHandle->setZValue(depth + 1);
    }

    const QString& iconFilePath = node->getPlugin()->getIconFilePath();
    BackdropGui* isBd = dynamic_cast<BackdropGui*>(this);

    if ( !isBd && !iconFilePath.isEmpty() && appPTR->getCurrentSettings()->isPluginIconActivatedOnNodeGraph() ) {
        _pluginIcon = new NodeGraphPixmapItem(getDagGui(), this);
        _pluginIcon->setZValue(depth + 1);
        _pluginIconFrame = new QGraphicsRectItem(this);
        _pluginIconFrame->setZValue(depth);
        int r, g, b;
        appPTR->getCurrentSettings()->getPluginIconFrameColor(&r, &g, &b);
        _pluginIconFrame->setBrush( QColor(r, g, b) );


        int size = TO_DPIX(NATRON_PLUGIN_ICON_SIZE);
        QPixmap pix;
        pix.load(iconFilePath);
        if (!pix.isNull()) {
            if (std::max( pix.width(), pix.height() ) != size) {
                pix = pix.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
            _pluginIcon->setPixmap(pix);
        } else {
            _pluginIcon->hide();
        }

    }

    _presetIcon = new NodeGraphPixmapItem(getDagGui(), this);
    _presetIcon->setZValue(depth + 1);
    _presetIcon->hide();


    _nameItem = new NodeGraphTextItem(getDagGui(), this, false);
    _nameItem->setPlainText( QString::fromUtf8( node->getLabel().c_str() ) );
    _nameItem->setDefaultTextColor( QColor(0, 0, 0, 255) );
    //_nameItem->setFont( QFont(appFont,appFontSize) );
    _nameItem->setZValue(depth + 1);

    _persistentMessage = new NodeGraphSimpleTextItem(getDagGui(), this, false);
    _persistentMessage->setZValue(depth + 3);
    QFont f = _persistentMessage->font();
    f.setPointSize(25);
    bool antialias = appPTR->getCurrentSettings()->isNodeGraphAntiAliasingEnabled();
    if (!antialias) {
        f.setStyleStrategy(QFont::NoAntialias);
    }
    _persistentMessage->setFont(f);
    _persistentMessage->hide();

    _stateIndicator = new NodeGraphRectItem(this, cornerRadiusPx);
    _stateIndicator->setZValue(depth - 1);
    _stateIndicator->hide();

    QRectF bbox = boundingRect();
    QGradientStops bitDepthGrad;
    bitDepthGrad.push_back( qMakePair( 0., QColor(Qt::white) ) );
    bitDepthGrad.push_back( qMakePair( 0.3, QColor(Qt::yellow) ) );
    bitDepthGrad.push_back( qMakePair( 1., QColor(243, 137, 0) ) );

    double ellipseDiam = TO_DPIX(NATRON_ELLIPSE_WARN_DIAMETER);

    _streamIssuesWarning.reset( new NodeGuiIndicator(getDagGui(), depth + 2, QString::fromUtf8("C"), QPointF( bbox.x() + bbox.width() / 2, bbox.y() ),
                                                     ellipseDiam, ellipseDiam,
                                                     bitDepthGrad, QColor(0, 0, 0, 255), this) );
    _streamIssuesWarning->setActive(false);


    QGradientStops exprGrad;
    exprGrad.push_back( qMakePair( 0., QColor(Qt::white) ) );
    exprGrad.push_back( qMakePair( 0.3, QColor(Qt::green) ) );
    exprGrad.push_back( qMakePair( 1., QColor(69, 96, 63) ) );
    _expressionIndicator.reset( new NodeGuiIndicator(getDagGui(), depth + 2, QString::fromUtf8("E"), bbox.topRight(), ellipseDiam, ellipseDiam, exprGrad, QColor(255, 255, 255), this) );
    _expressionIndicator->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("This node has one or several expression(s) involving values of parameters of other "
                                                                        "nodes in the project. Hover the mouse on the green connections to see what are the effective links."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _expressionIndicator->setActive(false);

    _availableViewsIndicator.reset( new NodeGuiIndicator(getDagGui(), depth + 2, QString::fromUtf8("V"), bbox.topLeft(), ellipseDiam, ellipseDiam, exprGrad, QColor(255, 255, 255), this) );
    _availableViewsIndicator->setActive(false);

    onAvailableViewsChanged();

    GroupInput* isGroupInput = dynamic_cast<GroupInput*>( node->getEffectInstance().get() );
    GroupOutput* isGroupOutput = dynamic_cast<GroupOutput*>( node->getEffectInstance().get() );

    if (!isGroupInput && !isGroupOutput) {
        QGradientStops ptGrad;
        ptGrad.push_back( qMakePair( 0., QColor(0, 0, 255) ) );
        ptGrad.push_back( qMakePair( 0.5, QColor(0, 50, 200) ) );
        ptGrad.push_back( qMakePair( 1., QColor(0, 100, 150) ) );
        _passThroughIndicator.reset( new NodeGuiIndicator(getDagGui(), depth + 2, QString::fromUtf8("P"), bbox.topRight(), ellipseDiam, ellipseDiam, ptGrad, QColor(255, 255, 255), this) );
        _passThroughIndicator->setActive(false);
    }

    _disabledBtmLeftTopRight = new QGraphicsLineItem(this);
    _disabledBtmLeftTopRight->setZValue(depth + 1);
    _disabledBtmLeftTopRight->hide();
    _disabledTopLeftBtmRight = new QGraphicsLineItem(this);
    _disabledTopLeftBtmRight->hide();
    _disabledTopLeftBtmRight->setZValue(depth + 1);
} // NodeGui::createGui

void
NodeGui::onSettingsPanelColorChanged(const QColor & color)
{
    {
        QMutexLocker k(&_currentColorMutex);
        _currentColor = color;
    }
    Q_EMIT colorChanged(color);

    refreshCurrentBrush();
}

void
NodeGui::beginEditKnobs()
{
    _wasBeginEditCalled = true;
    NodePtr node = getNode();
    if (node) {
        node->beginEditKnobs();
    }
}

void
NodeGui::togglePreview_internal(bool refreshPreview)
{
    if ( !canMakePreview() ) {
        return;
    }
    NodePtr node = getNode();
    if ( node && getNode()->isPreviewEnabled() ) {
        ensurePreviewCreated();
        if (refreshPreview) {
            getNode()->computePreviewImage( _graph->getGui()->getApp()->getTimeLine()->currentFrame() );
        }
    } else {
        if (_previewPixmap) {
            _previewPixmap->setVisible(false);
        }
        int w, h;
        getInitialSize(&w, &h);
        resize(w, h);
    }
}

void
NodeGui::ensurePreviewCreated()
{
    if (!_previewPixmap) {
        QImage prev(NATRON_PREVIEW_WIDTH, NATRON_PREVIEW_HEIGHT, QImage::Format_ARGB32);
        prev.fill(Qt::black);
        QPixmap prev_pixmap = QPixmap::fromImage(prev);
        _previewPixmap = new NodeGraphPixmapItem(getDagGui(), this);
        //Scale the widget according to the DPI of the screen otherwise the pixmap will cover exactly as many pixels
        //as there are in the image
        _previewPixmap->setTransform(QTransform::fromScale( appPTR->getLogicalDPIXRATIO(), appPTR->getLogicalDPIYRATIO() ), true);
        _previewPixmap->setPixmap(prev_pixmap);
        _previewPixmap->setZValue(getBaseDepth() + 1);
    }
    QSize size = getSize();
    int w, h;
    getSizeWithPreview(&w, &h);
    if ( (size.width() < w) ||
         ( size.height() < h) ) {
        resize(w, h);
        _previewPixmap->stackBefore(_nameItem);
    }
}

void
NodeGui::onPreviewKnobToggled()
{
    togglePreview_internal();
}

void
NodeGui::togglePreview()
{
    NodePtr node = getNode();
    if (node) {
        node->togglePreview();
    }
    togglePreview_internal();
}

void
NodeGui::removeUndoStack()
{
    if (_graph && _graph->getGui() && _undoStack) {
        _graph->getGui()->removeUndoStack( _undoStack.get() );
    }
}

void
NodeGui::discardGraphPointer()
{
    _graph = 0;
}

void
NodeGui::removeSettingsPanel()
{
    //called by DockablePanel when it is deleted by Qt's parenting scheme
    _settingsPanel = NULL;
}

void
NodeGui::refreshSize()
{
    QRectF bbox = boundingRect();

    QString& label = _nodeLabel;
    resize( bbox.width(), bbox.height(), false, !label.isEmpty() );
}

int
NodeGui::getFrameNameHeight() const
{
    if ( mustFrameName() ) {
        return _nameFrame->boundingRect().height();
    } else {
        return boundingRect().height();
    }
}

bool
NodeGui::isNearbyNameFrame(const QPointF& pos) const
{
    if ( !mustFrameName() ) {
        return _boundingBox->boundingRect().contains(pos);
    } else {
        QRectF headerBbox = _nameFrame->boundingRect();
        headerBbox.adjust(-5, -5, 5, 5);

        return headerBbox.contains(pos);
    }
}

bool
NodeGui::isNearbyResizeHandle(const QPointF& pos) const
{
    if ( !mustAddResizeHandle() ) {
        return false;
    }

    QPolygonF resizePoly = _resizeHandle->polygon();

    return resizePoly.containsPoint(pos, Qt::OddEvenFill);
}

void
NodeGui::adjustSizeToContent(int* /*w*/,
                             int *h,
                             bool adjustToTextSize)
{
    QRectF labelBbox = _nameItem->boundingRect();

    if (adjustToTextSize) {
        if ( _previewPixmap && _previewPixmap->isVisible() ) {
            int pw, ph;
            getSizeWithPreview(&pw, &ph);
            *h = ph;
        } else {
            *h = labelBbox.height() * 1.2;
        }
    } else {
        *h = std::max( (double)*h, labelBbox.height() * 1.2 );
    }
    if (_pluginIcon && _pluginIcon->isVisible() && _presetIcon && _presetIcon->isVisible()) {
        int iconsHeight = _pluginIcon->boundingRect().height() + _presetIcon->boundingRect().height();
        *h = std::max(*h, iconsHeight);
    }
}

int
NodeGui::getPluginIconWidth() const
{
    return _pluginIcon  && _pluginIcon->isVisible() ? TO_DPIX(NATRON_PLUGIN_ICON_SIZE + PLUGIN_ICON_OFFSET * 2) : 0;
}

double
NodeGui::refreshPreviewAndLabelPosition(const QRectF& bbox)
{
    const QRectF labelBbox = _nameItem->boundingRect();
    const double labelHeight = labelBbox.height();
    int prevW, prevH;
    {
        QMutexLocker k(&_previewDataMutex);
        prevW = _previewW;
        prevH = _previewH;
    }

    if ( !_previewPixmap || !_previewPixmap->isVisible() ) {
        prevW = 0;
        prevH = 0;
    }
    const double textPlusPixHeight = std::min( (double)prevH + 5. + labelHeight, bbox.height() - 5 );
    const int iconWidth = getPluginIconWidth();

    if (_previewPixmap) {
        double pixX = bbox.x() + iconWidth;
        double remainingSpace = bbox.width() - pixX;
        pixX = pixX + remainingSpace / 2. - prevW / 2.;
        double pixY = bbox.y() + bbox.height() / 2 - textPlusPixHeight / 2;
        _previewPixmap->setPos(pixX, pixY);
    }

    double height = bbox.height();

    {
        int nameWidth = labelBbox.width();
        double textX = bbox.x() + iconWidth +  ( (bbox.width() - iconWidth) / 2 ) - (nameWidth / 2);
        //double textY = topLeft.y() + labelHeight * 0.1;
        double textY;
        if ( mustFrameName() ) {
            double frameHeight = 1.5 * labelHeight;
            QRectF nameFrameBox(bbox.x(), bbox.y(), bbox.width(), frameHeight);
            _nameFrame->setRect(nameFrameBox);
            height = std::max( (double)bbox.height(), nameFrameBox.height() );
            textY = bbox.y() + frameHeight / 2 -  labelHeight / 2;
        } else {
            if ( _previewPixmap && _previewPixmap->isVisible() ) {
                textY = bbox.y() + height / 2 - textPlusPixHeight / 2 + prevH;
            } else {
                textY = bbox.y() + height / 2 - labelHeight / 2;
            }
        }

        _nameItem->setPos(textX, textY);
    }

    return height;
} // NodeGui::refreshPreviewAndLabelPosition

void
NodeGui::resize(int width,
                int height,
                bool forceSize,
                bool adjustToTextSize)
{
    if ( !canResize() ) {
        return;
    }
    const bool hasPluginIcon = _pluginIcon && _pluginIcon->isVisible();

    adjustSizeToContent(&width, &height, adjustToTextSize);
    const int iconWidth = getPluginIconWidth();

    {
        QMutexLocker k(&_mtSafeSizeMutex);
        _mtSafeWidth = width;
        _mtSafeHeight = height;
    }
    const QPointF topLeft = mapFromParent( pos() );
    const QPointF bottomRight(topLeft.x() + width, topLeft.y() + height);
    //const QPointF bottomLeft = topLeft + QPointF(0, height);
    const QPointF topRight = topLeft + QPointF(width, 0);

    QRectF bbox(topLeft.x(), topLeft.y(), width, height);

    _boundingBox->setRect(bbox);

    int iconSize = TO_DPIY(NATRON_PLUGIN_ICON_SIZE);
    int iconOffsetX = TO_DPIX(PLUGIN_ICON_OFFSET);
    if (hasPluginIcon) {
        _pluginIcon->setX(topLeft.x() + iconOffsetX);
        int iconsYOffset = _presetIcon  && _presetIcon->isVisible() ? (height - 2 * iconSize) / 3. : (height - iconSize) / 2.;
        _pluginIcon->setY(topLeft.y() + iconsYOffset);
        _pluginIconFrame->setRect(topLeft.x(), topLeft.y(), iconWidth, height);
    }

    if ( _presetIcon  && _presetIcon->isVisible() ) {
        int iconsYOffset =  (height - 2 * iconSize) / 3.;
        _presetIcon->setX(topLeft.x() + iconOffsetX);
        _presetIcon->setY(topLeft.y() + iconsYOffset * 2 + iconSize);
    }

    QFont f(appFont, appFontSize);
    QFontMetrics metrics(f, 0);

    height = refreshPreviewAndLabelPosition(bbox);


    if ( mustAddResizeHandle() ) {
        QPolygonF poly;
        poly.push_back( QPointF( bottomRight.x() - TO_DPIX(20), bottomRight.y() ) );
        poly.push_back(bottomRight);
        poly.push_back( QPointF( bottomRight.x(), bottomRight.y() - TO_DPIY(20) ) );
        _resizeHandle->setPolygon(poly);
    }

    QString persistentMessage = _persistentMessage->text();
    f.setPixelSize(25);
    metrics = QFontMetrics(f, 0);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    int pMWidth = metrics.horizontalAdvance(persistentMessage);
#else
    int pMWidth = metrics.width(persistentMessage);
#endif
    int midNodeX = topLeft.x() + iconWidth + (width - iconWidth) / 2;
    QPointF bitDepthPos(midNodeX, 0);
    _streamIssuesWarning->refreshPosition(bitDepthPos);

    if (_expressionIndicator) {
        _expressionIndicator->refreshPosition(topRight);
    }
    if (_availableViewsIndicator) {
        _availableViewsIndicator->refreshPosition(topLeft);
    }
    if (_passThroughIndicator) {
        _passThroughIndicator->refreshPosition(bottomRight);
    }

    int indicatorOffset = TO_DPIX(NATRON_STATE_INDICATOR_OFFSET);
    _persistentMessage->setPos(midNodeX - (pMWidth / 2), topLeft.y() + height / 2 - metrics.height() / 2);
    _stateIndicator->setRect(topLeft.x() - indicatorOffset, topLeft.y() - indicatorOffset,
                             width + indicatorOffset * 2, height + indicatorOffset * 2);

    _disabledBtmLeftTopRight->setLine( QLineF( bbox.bottomLeft(), bbox.topRight() ) );
    _disabledTopLeftBtmRight->setLine( QLineF( bbox.topLeft(), bbox.bottomRight() ) );

    resizeExtraContent(width, height, forceSize);

    refreshPosition( pos().x(), pos().y(), true );
} // NodeGui::resize

void
NodeGui::refreshPositionEnd(double x,
                            double y)
{
    setPos(x, y);
    if (_graph) {
        QRectF bbox = mapRectToScene( boundingRect() );
        const NodesGuiList & allNodes = _graph->getAllActiveNodes();

        for (NodesGuiList::const_iterator it = allNodes.begin(); it != allNodes.end(); ++it) {
            if ( (*it)->isVisible() && (it->get() != this) && (*it)->intersects(bbox) ) {
                setAboveItem( it->get() );
            }
        }
    }
    refreshEdges();
    NodePtr node = getNode();
    if (node) {
        const NodesWList & outputs = node->getGuiOutputs();

        for (NodesWList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
            NodePtr output = it->lock();
            if (output) {
                output->doRefreshEdgesGUI();
            }
        }
    }
    Q_EMIT positionChanged(x, y);
}

void
NodeGui::refreshPosition(double x,
                         double y,
                         bool skipMagnet,
                         const QPointF & mouseScenePos)
{
    if (appPTR->getCurrentSettings()->isSnapToNodeEnabled() && !skipMagnet) {
        QSize size = getSize();
        ///handle magnetic grid
        QPointF middlePos(x + size.width() / 2, y + size.height() / 2);


        if ( _magnecEnabled.x() || _magnecEnabled.y() ) {
            if ( _magnecEnabled.x() ) {
                _magnecDistance.rx() += ( x - _magnecStartingPos.x() );
                if ( std::abs( _magnecDistance.x() ) >= TO_DPIX(NATRON_MAGNETIC_GRID_RELEASE_DISTANCE) ) {
                    _magnecEnabled.rx() = 0;
                    _updateDistanceSinceLastMagnec.rx() = 1;
                    _distanceSinceLastMagnec.rx() = 0;
                }
            }
            if ( _magnecEnabled.y() ) {
                _magnecDistance.ry() += ( y - _magnecStartingPos.y() );
                if ( std::abs( _magnecDistance.y() ) >= TO_DPIY(NATRON_MAGNETIC_GRID_RELEASE_DISTANCE) ) {
                    _magnecEnabled.ry() = 0;
                    _updateDistanceSinceLastMagnec.ry() = 1;
                    _distanceSinceLastMagnec.ry() = 0;
                }
            }


            if ( !_magnecEnabled.x() && !_magnecEnabled.y() ) {
                ///When releasing the grip, make sure to follow the mouse
                QPointF newPos = ( mapToParent( mapFromScene(mouseScenePos) ) );
                newPos.rx() -= size.width() / 2;
                newPos.ry() -= size.height() / 2;
                refreshPositionEnd( newPos.x(), newPos.y() );

                return;
            } else if ( _magnecEnabled.x() && !_magnecEnabled.y() ) {
                x = pos().x();
            } else if ( !_magnecEnabled.x() && _magnecEnabled.y() ) {
                y = pos().y();
            } else {
                return;
            }
        }

        bool continueMagnet = true;
        if (_updateDistanceSinceLastMagnec.rx() == 1) {
            _distanceSinceLastMagnec.rx() = x - _magnecStartingPos.x();
            if ( std::abs( _distanceSinceLastMagnec.x() ) > ( TO_DPIX(NATRON_MAGNETIC_GRID_GRIP_TOLERANCE) ) ) {
                _updateDistanceSinceLastMagnec.rx() = 0;
            } else {
                continueMagnet = false;
            }
        }
        if (_updateDistanceSinceLastMagnec.ry() == 1) {
            _distanceSinceLastMagnec.ry() = y - _magnecStartingPos.y();
            if ( std::abs( _distanceSinceLastMagnec.y() ) > ( TO_DPIY(NATRON_MAGNETIC_GRID_GRIP_TOLERANCE) ) ) {
                _updateDistanceSinceLastMagnec.ry() = 0;
            } else {
                continueMagnet = false;
            }
        }


        if ( ( !_magnecEnabled.x() || !_magnecEnabled.y() ) && continueMagnet ) {
            for (InputEdges::iterator it = _inputEdges.begin(); it != _inputEdges.end(); ++it) {
                ///For each input try to find if the magnet should be enabled
                NodeGuiPtr inputSource = (*it)->getSource();
                if (inputSource) {
                    QSize inputSize = inputSource->getSize();
                    QPointF inputScenePos = inputSource->scenePos();
                    QPointF inputPos = inputScenePos + QPointF(inputSize.width() / 2, inputSize.height() / 2);
                    QPointF mapped = mapToParent( mapFromScene(inputPos) );
                    if ( !contains(mapped) ) {
                        if ( !_magnecEnabled.x() && ( ( mapped.x() >= ( middlePos.x() - TO_DPIX(NATRON_MAGNETIC_GRID_GRIP_TOLERANCE) ) ) &&
                                                      ( mapped.x() <= ( middlePos.x() + TO_DPIX(NATRON_MAGNETIC_GRID_GRIP_TOLERANCE) ) ) ) ) {
                            _magnecEnabled.rx() = 1;
                            _magnecDistance.rx() = 0;
                            x = mapped.x() - size.width() / 2;
                            _magnecStartingPos.setX(x);
                        } else if ( !_magnecEnabled.y() && ( ( mapped.y() >= ( middlePos.y() - TO_DPIX(NATRON_MAGNETIC_GRID_GRIP_TOLERANCE) ) ) &&
                                                             ( mapped.y() <= ( middlePos.y() + TO_DPIX(NATRON_MAGNETIC_GRID_GRIP_TOLERANCE) ) ) ) ) {
                            _magnecEnabled.ry() = 1;
                            _magnecDistance.ry() = 0;
                            y = mapped.y() - size.height() / 2;
                            _magnecStartingPos.setY(y);
                        }
                    }
                }
            }

            NodePtr node = getNode();
            if ( node && ( !_magnecEnabled.x() || !_magnecEnabled.y() ) ) {
                ///check now the outputs
                const NodesWList & outputs = node->getGuiOutputs();
                for (NodesWList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
                    NodePtr output = it->lock();
                    if (!output) {
                        continue;
                    }
                    NodeGuiIPtr node_gui_i = output->getNodeGui();
                    if (!node_gui_i) {
                        continue;
                    }
                    NodeGui* node = dynamic_cast<NodeGui*>( node_gui_i.get() );
                    assert(node);
                    if (!node) {
                        return;
                    }
                    QSize outputSize = node->getSize();
                    QPointF nodeScenePos = node->scenePos();
                    QPointF outputPos = nodeScenePos  + QPointF(outputSize.width() / 2, outputSize.height() / 2);
                    QPointF mapped = mapToParent( mapFromScene(outputPos) );
                    if ( !contains(mapped) ) {
                        if ( !_magnecEnabled.x() && ( ( mapped.x() >= ( middlePos.x() - TO_DPIX(NATRON_MAGNETIC_GRID_GRIP_TOLERANCE) ) ) &&
                                                      ( mapped.x() <= ( middlePos.x() + TO_DPIX(NATRON_MAGNETIC_GRID_GRIP_TOLERANCE) ) ) ) ) {
                            _magnecEnabled.rx() = 1;
                            _magnecDistance.rx() = 0;
                            x = mapped.x() - size.width() / 2;
                            _magnecStartingPos.setX(x);
                        } else if ( !_magnecEnabled.y() && ( ( mapped.y() >= ( middlePos.y() - TO_DPIY(NATRON_MAGNETIC_GRID_GRIP_TOLERANCE) ) ) &&
                                                             ( mapped.y() <= ( middlePos.y() + TO_DPIY(NATRON_MAGNETIC_GRID_GRIP_TOLERANCE) ) ) ) ) {
                            _magnecEnabled.ry() = 1;
                            _magnecDistance.ry() = 0;
                            y = mapped.y() - size.height() / 2;
                            _magnecStartingPos.setY(y);
                        }
                    }
                }
            }
        }
    }

    refreshPositionEnd(x, y);
} // refreshPosition

void
NodeGui::setAboveItem(QGraphicsItem* item)
{
    if ( !isVisible() || dynamic_cast<BackdropGui*>(this) || dynamic_cast<BackdropGui*>(item) ) {
        return;
    }
    item->stackBefore(this);
    for (InputEdges::iterator it = _inputEdges.begin(); it != _inputEdges.end(); ++it) {
        NodeGuiPtr inputSource = (*it)->getSource();
        if (inputSource.get() != item) {
            item->stackBefore( (*it) );
        }
    }
    if (_outputEdge) {
        item->stackBefore(_outputEdge);
    }
}

void
NodeGui::changePosition(double dx,
                        double dy)
{
    QPointF p = pos();

    refreshPosition(p.x() + dx, p.y() + dy, true);
}

void
NodeGui::refreshDashedStateOfEdges()
{
    NodePtr node = getNode();

    ViewerInstance* viewer = node ? node->isEffectViewer() : NULL;

    if (viewer) {
        int activeInputs[2];
        viewer->getActiveInputs(activeInputs[0], activeInputs[1]);

        int nbInputsConnected = 0;

        for (U32 i = 0; i < _inputEdges.size(); ++i) {
            if ( ( (int)i == activeInputs[0] ) || ( (int)i == activeInputs[1] ) ) {
                _inputEdges[i]->setDashed(false);
            } else {
                _inputEdges[i]->setDashed(true);
            }
            if ( _inputEdges[i]->getSource() ) {
                ++nbInputsConnected;
            }
        }
        if ( (nbInputsConnected == 0) && !_inputEdges.empty() ) {
            if (_inputEdges[0]) {
                _inputEdges[0]->setDashed(false);
            }
        }
    }
}

void
NodeGui::refreshEdges()
{
    NodePtr node = getNode();
    if (!node) {
        return;
    }
    const std::vector<NodeWPtr> & nodeInputs = node->getGuiInputs();

    if ( _inputEdges.size() != nodeInputs.size() ) {
        return;
    }

    for (U32 i = 0; i < _inputEdges.size(); ++i) {
        assert( i < nodeInputs.size() );
        assert(_inputEdges[i]);

        NodePtr input = nodeInputs[i].lock();
        if (input) {
            NodeGuiIPtr nodeInputGui_i = input->getNodeGui();
            if (!nodeInputGui_i) {
                continue;
            }
            NodeGuiPtr node = std::dynamic_pointer_cast<NodeGui>(nodeInputGui_i);
            if (_inputEdges[i]->getSource() != node) {
                _inputEdges[i]->setSource(node);
            } else {
                _inputEdges[i]->initLine();
            }
        } else {
            _inputEdges[i]->initLine();
        }
    }
    if (_outputEdge) {
        _outputEdge->initLine();
    }
}

void
NodeGui::refreshKnobLinks()
{
    for (KnobGuiLinks::iterator it = _knobsLinks.begin(); it != _knobsLinks.end(); ++it) {
        it->second.arrow->refreshPosition();
    }
    if (_slaveMasterLink) {
        _slaveMasterLink->refreshPosition();
    }
}

void
NodeGui::markInputNull(Edge* e)
{
    for (U32 i = 0; i < _inputEdges.size(); ++i) {
        if (_inputEdges[i] == e) {
            _inputEdges[i] = 0;
        }
    }
}

void
NodeGui::updatePreviewImage(double time)
{
    NodePtr node = getNode();

    if ( isVisible() && node && node->isPreviewEnabled() && node->isActivated() && node->getApp()->getProject()->isAutoPreviewEnabled() ) {
        if ( (node->getScriptName().find(NATRON_FILE_DIALOG_PREVIEW_READER_NAME) != std::string::npos) ||
             ( node->getScriptName().find(NATRON_FILE_DIALOG_PREVIEW_VIEWER_NAME) != std::string::npos) ) {
            return;
        }

        ensurePreviewCreated();

        NodeGuiPtr thisShared = shared_from_this();
        assert(thisShared);
        appPTR->appendTaskToPreviewThread(thisShared, time);
    }
}

void
NodeGui::forceComputePreview(double time)
{
    NodePtr node = getNode();

    if ( !node || !node->isActivated() ) {
        return;
    }
    if ( isVisible() && node->isPreviewEnabled() && !node->getApp()->getProject()->isLoadingProject() ) {
        if ( (node->getScriptName().find(NATRON_FILE_DIALOG_PREVIEW_READER_NAME) != std::string::npos) ||
             ( node->getScriptName().find(NATRON_FILE_DIALOG_PREVIEW_VIEWER_NAME) != std::string::npos) ) {
            return;
        }

        ensurePreviewCreated();
        NodeGuiPtr thisShared = shared_from_this();
        assert(thisShared);
        appPTR->appendTaskToPreviewThread(thisShared, time);
    }
}

void
NodeGui::onPreviewImageComputed()
{
    assert( QThread::currentThread() == qApp->thread() );
    assert(_previewPixmap);
    if ( !_previewPixmap->isVisible() ) {
        _previewPixmap->setVisible(true);
    }

    {
        QMutexLocker k(&_previewDataMutex);
        QImage img(reinterpret_cast<const uchar*>( &_previewData.front() ), _previewW, _previewH, QImage::Format_ARGB32_Premultiplied);
        QPixmap pix = QPixmap::fromImage(img);
        _previewPixmap->setPixmap(pix);
    }
    const QPointF topLeft = mapFromParent( pos() );
    int width, height;
    {
        QMutexLocker k(&_mtSafeSizeMutex);
        height = _mtSafeHeight;
        width = _mtSafeWidth;
    }
    QRectF bbox(topLeft.x(), topLeft.y(), width, height);
    refreshPreviewAndLabelPosition(bbox);
}

void
NodeGui::copyPreviewImageBuffer(const std::vector<unsigned int>& data,
                                int width,
                                int height)
{
    {
        QMutexLocker k(&_previewDataMutex);
        _previewData = data;
        _previewW = width;
        _previewH = height;
    }
    Q_EMIT previewImageComputed();
}

bool
NodeGui::getOverlayColor(double* r,
                         double* g,
                         double* b) const
{
    if ( !getSettingPanel() ) {
        return false;
    }
    if (_overlayLocked) {
        *r = 0.5;
        *g = 0.5;
        *b = 0.5;

        return true;
    }
    if ( !getSettingPanel()->hasOverlayColor() ) {
        return false;
    }
    QColor c = getSettingPanel()->getOverlayColor();
    *r = c.redF();
    *g = c.greenF();
    *b = c.blueF();

    return true;
}

void
NodeGui::initializeInputsForInspector()
{
    NodePtr node = getNode();

    assert(node);
    if (!node) {
        return;
    }
    // Count mask inputs first
    std::vector<bool> masksInputs(_inputEdges.size());
    int nMasksInputs = 0;
    for (std::size_t i = 0; i < _inputEdges.size(); ++i) {
        masksInputs[i] = node->getEffectInstance()->isInputMask(i);
        if (masksInputs[i]) {
            ++nMasksInputs;
        }
    }



    ///If the node is a viewer, display 1 input and another one aside and hide all others.
    ///If the node is something else (switch, merge) show 2 inputs and another one aside an hide all others.

    bool isViewer = node->isEffectViewer() != 0;
    int maxInitiallyOnTopVisibleInputs = isViewer ? 1 : 2;
    double angleBetweenInputs = M_PI / (maxInitiallyOnTopVisibleInputs + 1);
    double angle =  angleBetweenInputs;

    double angleBetweenMasks = 0.;
    double maskAngle = 0.;
    if (nMasksInputs > 1) {
        angleBetweenMasks = M_PI / (double)(nMasksInputs + 1);
        maskAngle = -M_PI_2 + angleBetweenMasks;
    }


    for (std::size_t i = 0; i < _inputEdges.size(); ++i) {
        if (masksInputs[i]) {
            _inputEdges[i]->setAngle(maskAngle);
            maskAngle += angleBetweenMasks;
        } else {
            if ( ( (int)i < maxInitiallyOnTopVisibleInputs ) ) {
                _inputEdges[i]->setAngle(angle);
                angle += angleBetweenInputs;
            } else {
                _inputEdges[i]->setAngle(M_PI);
            }
        }


        if ( !_inputEdges[i]->hasSource() ) {
            _inputEdges[i]->initLine();
        }
    }

    refreshEdgesVisility();
}

void
NodeGui::initializeInputs()
{
    ///Also refresh the output position
    if (_outputEdge) {
        _outputEdge->initLine();
    }

    NodePtr node = getNode();

    ///The actual numbers of inputs of the internal node
    const std::vector<NodeWPtr>& inputs = node->getGuiInputs();

    ///Delete all  inputs that may exist
    for (InputEdges::iterator it = _inputEdges.begin(); it != _inputEdges.end(); ++it) {
        delete *it;
    }
    _inputEdges.clear();


    ///Make new edge for all non existing inputs
    NodeGuiPtr thisShared = shared_from_this();
    int inputsCount = 0;
    int emptyInputsCount = 0;
    for (U32 i = 0; i < inputs.size(); ++i) {
        Edge* edge = new Edge( i, 0., thisShared, parentItem() );
        if ( ( node && node->getEffectInstance()->isInputRotoBrush(i) ) || !isVisible() ) {
            edge->setActive(false);
            edge->hide();
        }

        NodePtr input = inputs[i].lock();
        if (input) {
            NodeGuiIPtr gui_i = input->getNodeGui();
            assert(gui_i);
            NodeGuiPtr gui = std::dynamic_pointer_cast<NodeGui>(gui_i);
            assert(gui);
            edge->setSource(gui);
        }
        if ( node &&
             !node->getEffectInstance()->isInputMask(i) &&
             !node->getEffectInstance()->isInputRotoBrush(i) ) {
            if (!input) {
                ++emptyInputsCount;
            }
            ++inputsCount;
        }
        _inputEdges.push_back(edge);
    }


    refreshDashedStateOfEdges();

    InspectorNode* isInspector = dynamic_cast<InspectorNode*>( node.get() );
    if (isInspector) {
        initializeInputsForInspector();
    } else {
        double piDividedbyX = M_PI / (inputsCount + 1);
        double angle =  piDividedbyX;
        int maskIndex = 0;
        for (U32 i = 0; i < _inputEdges.size(); ++i) {
            if ( node && !node->getEffectInstance()->isInputRotoBrush(i) ) {
                double edgeAngle;
                bool incrAngle = true;
                if ( node->getEffectInstance()->isInputMask(i) ) {
                    if (maskIndex == 0) {
                        edgeAngle = 0;
                        incrAngle = false;
                        ++maskIndex;
                    } else if (maskIndex == 1) {
                        edgeAngle = M_PI;
                        incrAngle = false;
                        ++maskIndex;
                    } else {
                        edgeAngle = angle;
                    }
                } else {
                    edgeAngle = angle;
                }
                _inputEdges[i]->setAngle(edgeAngle);
                if (incrAngle) {
                    angle += piDividedbyX;
                }
                if ( !_inputEdges[i]->hasSource() ) {
                    _inputEdges[i]->initLine();
                }
            }
        }
    }
} // initializeInputs

bool
NodeGui::contains(const QPointF &point) const
{
    QRectF bbox = boundingRect();

    bbox.adjust(-5, -5, 5, 5);

    return bbox.contains(point);
}

bool
NodeGui::intersects(const QRectF & rect) const
{
    QRectF mapped = mapRectFromScene(rect);

    return boundingRect().intersects(mapped);
}

QPainterPath
NodeGui::shape() const
{
    return _boundingBox->shape();
}

QRectF
NodeGui::boundingRect() const
{
    QTransform t;
    QRectF bbox = _boundingBox->boundingRect();

    QPointF center = bbox.center();

    t.translate( center.x(), center.y() );
    t.scale( scale(), scale() );
    t.translate( -center.x(), -center.y() );

    return t.mapRect(bbox);
}

void
NodeGui::refreshEdgesVisility()
{
    QPointF mousePos = mapFromScene( _graph->mapToScene( _graph->mapFromGlobal( QCursor::pos() ) ) );
    bool hovered = contains(mousePos);

    refreshEdgesVisibilityInternal(hovered);
}

void
NodeGui::refreshEdgesVisility(bool hovered)
{
    refreshEdgesVisibilityInternal(hovered);
}

void
NodeGui::refreshEdgesVisibilityInternal(bool hovered)
{
    std::vector<bool> edgesVisibility( _inputEdges.size() );

    for (std::size_t i = 0; i < _inputEdges.size(); ++i) {
        edgesVisibility[i] = _inputEdges[i]->computeVisibility(hovered);
    }

    NodePtr node = getNode();
    InspectorNode* isInspector = dynamic_cast<InspectorNode*>( node.get() );
    if (isInspector) {
        bool isViewer = node->isEffectViewer() != 0;
        int maxInitiallyOnTopVisibleInputs = isViewer ? 1 : 2;
        bool inputAsideDisplayed = false;

        /*
         * If optional inputs are displayed, only show one input on th left side of the node
         */
        for (int i = maxInitiallyOnTopVisibleInputs; i < (int)_inputEdges.size(); ++i) {
            if ( !edgesVisibility[i] || _inputEdges[i]->isMask() ) {
                continue;
            }
            if (!inputAsideDisplayed) {
                if ( !_inputEdges[i]->getSource() ) {
                    inputAsideDisplayed = true;
                }
            } else {
                if ( !_inputEdges[i]->getSource() ) {
                    edgesVisibility[i] = false;
                }
            }
        }
    }

    bool hasChanged = false;
    for (std::size_t i = 0; i < _inputEdges.size(); ++i) {
        if (_inputEdges[i]->isVisible() != edgesVisibility[i]) {
            _inputEdges[i]->setVisible(edgesVisibility[i]);
            hasChanged = true;
        }
    }
    if (hasChanged) {
        update();
    }
}

QRectF
NodeGui::boundingRectWithEdges() const
{
    QRectF ret;
    QRectF bbox = boundingRect();

    ret = mapToScene(bbox).boundingRect();
    for (InputEdges::const_iterator it = _inputEdges.begin(); it != _inputEdges.end(); ++it) {
        if ( !(*it)->hasSource() ) {
            ret = ret.united( (*it)->mapToScene( (*it)->boundingRect() ).boundingRect() );
        }
    }

    return ret;
}

bool
NodeGui::isNearby(QPointF &point)
{
    QPointF p = mapFromScene(point);
    QRectF bbox = boundingRect();
    QRectF r( bbox.x() - TO_DPIX(NATRON_EDGE_DROP_TOLERANCE), bbox.y() - TO_DPIY(NATRON_EDGE_DROP_TOLERANCE),
              bbox.width() + TO_DPIX(NATRON_EDGE_DROP_TOLERANCE), bbox.height() + TO_DPIY(NATRON_EDGE_DROP_TOLERANCE) );

    return r.contains(p);
}

Edge*
NodeGui::firstAvailableEdge()
{
    for (U32 i = 0; i < _inputEdges.size(); ++i) {
        Edge* a = _inputEdges[i];
        if ( !a->hasSource() ) {
            NodePtr node;
            if ( node && node->getEffectInstance()->isInputOptional(i) ) {
                continue;
            }
        }

        return a;
    }

    return NULL;
}

void
NodeGui::applyBrush(const QBrush & brush)
{
    _boundingBox->setBrush(brush);
    if ( mustFrameName() ) {
        _nameFrame->setBrush(brush);
    }
    if ( mustAddResizeHandle() ) {
        _resizeHandle->setBrush(brush);
    }
}

void
NodeGui::refreshCurrentBrush()
{
    if (_slaveMasterLink) {
        applyBrush(_clonedColor);
    } else {
        applyBrush(_currentColor);
    }
}

bool
NodeGui::isSelectedInParentMultiInstance(const Node* node) const
{
    MultiInstancePanelPtr multiInstance = getMultiInstancePanel();

    if (!multiInstance) {
        return false;
    }

    const std::list<std::pair<NodeWPtr, bool > >& instances = multiInstance->getInstances();
    for (std::list<std::pair<NodeWPtr, bool > >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        NodePtr instance = it->first.lock();
        if (instance.get() == node) {
            return it->second;
        }
    }

    return false;
}

void
NodeGui::setUserSelected(bool b)
{
    {
        QMutexLocker l(&_selectedMutex);
        _selected = b;
    }
    if (_settingsPanel) {
        _settingsPanel->setSelected(b);
        _settingsPanel->update();
        if ( b && isSettingsPanelVisible() ) {
            NodeGuiPtr thisShared = shared_from_this();
            _graph->getGui()->setNodeViewerInterface(thisShared);
        }
    }

    refreshEdgesVisility();
    refreshStateIndicator();
}

bool
NodeGui::getIsSelected() const
{
    QMutexLocker l(&_selectedMutex);

    return _selected;
}

Edge*
NodeGui::findConnectedEdge(NodeGui* parent)
{
    for (U32 i = 0; i < _inputEdges.size(); ++i) {
        Edge* e = _inputEdges[i];

        if ( e && (e->getSource().get() == parent) ) {
            return e;
        }
    }

    return NULL;
}

bool
NodeGui::connectEdge(int edgeNumber)
{
    NodePtr node = getNode();
    if (!node) {
        return false;
    }
    const std::vector<NodeWPtr> & inputs = node->getGuiInputs();

    if ( (edgeNumber < 0) || ( edgeNumber >= (int)inputs.size() ) || ( _inputEdges.size() != inputs.size() ) ) {
        return false;
    }

    NodeGuiPtr src;
    NodePtr input = inputs[edgeNumber].lock();
    if (input) {
        NodeGuiIPtr ngi = input->getNodeGui();
        src = std::dynamic_pointer_cast<NodeGui>(ngi);
    }

    _inputEdges[edgeNumber]->setSource(src);

    assert(node);
    if ( dynamic_cast<InspectorNode*>( node.get() ) ) {
        initializeInputsForInspector();
    }

    refreshEdgesVisility();

    return true;
}

Edge*
NodeGui::hasEdgeNearbyPoint(const QPointF & pt)
{
    for (InputEdges::const_iterator it = _inputEdges.begin(); it != _inputEdges.end(); ++it) {
        if ( (*it) && (*it)->contains( (*it)->mapFromScene(pt) ) ) {
            return (*it);
        }
    }
    if ( _outputEdge && _outputEdge->contains( _outputEdge->mapFromScene(pt) ) ) {
        return _outputEdge;
    }

    return NULL;
}

Edge*
NodeGui::hasBendPointNearbyPoint(const QPointF & pt)
{
    for (InputEdges::const_iterator it = _inputEdges.begin(); it != _inputEdges.end(); ++it) {
        if ( (*it) && (*it)->hasSource() && (*it)->isBendPointVisible() ) {
            if ( (*it)->isNearbyBendPoint(pt) ) {
                return (*it);
            }
        }
    }

    return NULL;
}

Edge*
NodeGui::hasEdgeNearbyRect(const QRectF & rect)
{
    ///try with all 4 corners

    QLineF rectEdges[4] =
    {
        QLineF( rect.topLeft(), rect.topRight() ),
        QLineF( rect.topRight(), rect.bottomRight() ),
        QLineF( rect.bottomRight(), rect.bottomLeft() ),
        QLineF( rect.bottomLeft(), rect.topLeft() )
    };
    QPointF intersection;
    QPointF middleRect = rect.center();
    Edge* closest = 0;
    double closestSquareDist = 0;

    for (InputEdges::const_iterator it = _inputEdges.begin(); it != _inputEdges.end(); ++it) {
        QLineF edgeLine = (*it)->line();
        edgeLine.setP1( (*it)->mapToScene( edgeLine.p1() ) );
        edgeLine.setP2( (*it)->mapToScene( edgeLine.p2() ) );
        for (int j = 0; j < 4; ++j) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            QLineF::IntersectionType intersectType = edgeLine.intersects(rectEdges[j], &intersection);
#else
            QLineF::IntersectType intersectType = edgeLine.intersect(rectEdges[j], &intersection);
#endif
            if (intersectType == QLineF::BoundedIntersection) {
                if (!closest) {
                    closest = *it;
                    closestSquareDist = ( intersection.x() - middleRect.x() ) * ( intersection.x() - middleRect.x() )
                                        + ( intersection.y() - middleRect.y() ) * ( intersection.y() - middleRect.y() );
                } else {
                    double dist = ( intersection.x() - middleRect.x() ) * ( intersection.x() - middleRect.x() )
                                  + ( intersection.y() - middleRect.y() ) * ( intersection.y() - middleRect.y() );
                    if (dist < closestSquareDist) {
                        closestSquareDist = dist;
                        closest = *it;
                    }
                }
                break;
            }
        }
    }
    if (closest) {
        return closest;
    }

    if (_outputEdge) {
        if ( _outputEdge->isVisible() ) {
            QLineF edgeLine = _outputEdge->line();
            edgeLine.setP1( (_outputEdge)->mapToScene( edgeLine.p1() ) );
            edgeLine.setP2( (_outputEdge)->mapToScene( edgeLine.p2() ) );
            for (int j = 0; j < 4; ++j) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
                QLineF::IntersectionType intersectType = edgeLine.intersects(rectEdges[j], &intersection);
#else
                QLineF::IntersectType intersectType = edgeLine.intersect(rectEdges[j], &intersection);
#endif
                if (intersectType == QLineF::BoundedIntersection) {
                    return _outputEdge;
                }
            }
        }
    }

    return NULL;
} // hasEdgeNearbyRect

void
NodeGui::showGui()
{
    show();
    setActive(true);
    NodePtr node = getNode();
    assert(node);
    if (!node) {
        return;
    }
    for (U32 i = 0; i < _inputEdges.size(); ++i) {
        _graph->scene()->addItem(_inputEdges[i]);
        _inputEdges[i]->setParentItem( parentItem() );
        if ( !node->getEffectInstance()->isInputRotoBrush(i) ) {
            _inputEdges[i]->setActive(true);
        }
    }
    if (_outputEdge) {
        _graph->scene()->addItem(_outputEdge);
        _outputEdge->setParentItem( parentItem() );
        _outputEdge->setActive(true);
    }
    refreshEdges();
    const NodesWList & outputs = node->getGuiOutputs();
    for (NodesWList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        NodePtr output = it->lock();
        if (output) {
            output->doRefreshEdgesGUI();
        }
    }
    ViewerInstance* viewer = node->isEffectViewer();
    if (viewer) {
        _graph->getGui()->activateViewerTab(viewer);
    } else {
        if (_panelOpenedBeforeDeactivate) {
            setVisibleSettingsPanel(true);
        }
        NodeGuiPtr thisShared = shared_from_this();
        _graph->getGui()->setNodeViewerInterface(thisShared);

        OfxEffectInstance* ofxNode = dynamic_cast<OfxEffectInstance*>( node->getEffectInstance().get() );
        if (ofxNode) {
            ofxNode->effectInstance()->beginInstanceEditAction();
        }
    }

    if (_slaveMasterLink) {
        if ( !node->getMasterNode() ) {
            onAllKnobsSlaved(false);
        } else {
            _slaveMasterLink->show();
        }
    }
    for (KnobGuiLinks::iterator it = _knobsLinks.begin(); it != _knobsLinks.end(); ++it) {
        it->second.arrow->show();
    }
} // NodeGui::showGui

void
NodeGui::activate(bool triggerRender)
{
    ///first activate all child instance if any
    NodePtr node = getNode();
    assert(node);
    if (!node) {
        return;
    }
    bool isMultiInstanceChild = !node->getParentMultiInstanceName().empty();

    if (!isMultiInstanceChild) {
        showGui();
    } else {
        ///don't show gui if it is a multi instance child, but still Q_EMIT the begin edit action
        OfxEffectInstance* ofxNode = dynamic_cast<OfxEffectInstance*>( node->getEffectInstance().get() );
        if (ofxNode) {
            ofxNode->effectInstance()->beginInstanceEditAction();
        }
    }
    _graph->restoreFromTrash(this);
    //_graph->getGui()->getCurveEditor()->addNode(shared_from_this());

    if (!isMultiInstanceChild && triggerRender) {
        std::list<ViewerInstance* > viewers;
        node->hasViewersConnected(&viewers);
        for (std::list<ViewerInstance* >::iterator it = viewers.begin(); it != viewers.end(); ++it) {
            (*it)->renderCurrentFrame(true);
        }
    }
}

void
NodeGui::hideGui()
{
    if ( !_graph || !_graph->getGui() ) {
        return;
    }
    hide();
    setActive(false);
    for (InputEdges::const_iterator it = _inputEdges.begin(); it != _inputEdges.end(); ++it) {
        if ( (*it)->scene() ) {
            (*it)->scene()->removeItem( (*it) );
        }
        (*it)->setActive(false);
        (*it)->setSource( NodeGuiPtr() );
    }
    if (_outputEdge) {
        if ( _outputEdge->scene() ) {
            _outputEdge->scene()->removeItem(_outputEdge);
        }
        _outputEdge->setActive(false);
    }

    if (_slaveMasterLink) {
        _slaveMasterLink->hide();
    }
    for (KnobGuiLinks::iterator it = _knobsLinks.begin(); it != _knobsLinks.end(); ++it) {
        it->second.arrow->hide();
    }
    NodePtr node = getNode();
    ViewerInstance* isViewer = node ? node->isEffectViewer() : NULL;
    if (isViewer) {
        ViewerGL* viewerGui = dynamic_cast<ViewerGL*>( isViewer->getUiContext() );
        if (viewerGui) {
            viewerGui->clearLastRenderedTexture();
            _graph->getGui()->deactivateViewerTab(isViewer);
        }
    } else {
        _panelOpenedBeforeDeactivate = isSettingsPanelVisible();
        if (_panelOpenedBeforeDeactivate) {
            setVisibleSettingsPanel(false);
        }

        NodeGuiPtr thisShared = shared_from_this();
        _graph->getGui()->removeNodeViewerInterface(thisShared, false);

        NodeGroup* isGrp = node ? node->isEffectGroup() : NULL;
        if ( isGrp && isGrp->isSubGraphUserVisible() ) {
            NodeGraphI* graph_i = isGrp->getNodeGraph();
            assert(graph_i);
            NodeGraph* graph = dynamic_cast<NodeGraph*>(graph_i);
            assert(graph);
            if (graph) {
                _graph->getGui()->removeGroupGui(graph, false);
            }
        }
    }
} // hideGui

void
NodeGui::deactivate(bool triggerRender)
{
    ///first deactivate all child instance if any
    NodePtr node = getNode();
    bool isMultiInstanceChild = node ? node->getParentMultiInstance().get() != NULL : false;

    if (!isMultiInstanceChild) {
        hideGui();
    }
    OfxEffectInstance* ofxNode = !node ? 0 : dynamic_cast<OfxEffectInstance*>( node->getEffectInstance().get() );
    if (ofxNode) {
        ofxNode->effectInstance()->endInstanceEditAction();
    }
    if (_graph) {
        _graph->moveToTrash(this);
        if ( _graph->getGui() ) {
            _graph->getGui()->getCurveEditor()->removeNode(this);
            _graph->getGui()->getDopeSheetEditor()->removeNode(this);
        }
    }

    if (!isMultiInstanceChild && triggerRender) {
        std::list<ViewerInstance* > viewers;
        if (node) {
            node->hasViewersConnected(&viewers);
        }
        for (std::list<ViewerInstance* >::iterator it = viewers.begin(); it != viewers.end(); ++it) {
            (*it)->renderCurrentFrame(true);
        }
    }
}

void
NodeGui::initializeKnobs()
{
    if (_settingsPanel) {
        _settingsPanel->initializeKnobs();
    }
}

void
NodeGui::setVisibleSettingsPanel(bool b, bool m, bool h)
{
    if (!_panelCreated) {
        ensurePanelCreated(m, h);
    }
    if (_settingsPanel) {
        _settingsPanel->setClosed(!b);
        if (b) {
            // also maximize (but don't minimize when closing)
            _settingsPanel->minimizeOrMaximize(false);
        }
    }
}

bool
NodeGui::isSettingsPanelVisible() const
{
    return _settingsPanel && !_settingsPanel->isClosed() && !_settingsPanel->isMinimized();
}

bool
NodeGui::isSettingsPanelMinimized() const
{
    return _settingsPanel && _settingsPanel->isMinimized();
}

void
NodeGui::onPersistentMessageChanged()
{
    //keep type in synch with this enum:
    //enum MessageTypeEnum{eMessageTypeInfo = 0,eMessageTypeError = 1,eMessageTypeWarning = 2,eMessageTypeQuestion = 3};

    ///don't do anything if the last persistent message is the same
    if ( !_persistentMessage || !_stateIndicator || !_graph || !_graph->getGui() ) {
        return;
    }

    QString message;
    int type = 0;
    NodePtr node = getNode();
    if (node) {
        node->getPersistentMessage(&message, &type);
    }
    _persistentMessage->setVisible( !message.isEmpty() );

    if ( !node || message.isEmpty() ) {
        setToolTip( QString() );
    } else {
        if (type == 1) {
            _persistentMessage->setText( tr("ERROR") );
            QColor errColor(128, 0, 0, 255);
            _persistentMessage->setBrush(errColor);
        } else if (type == 2) {
            _persistentMessage->setText( tr("WARNING") );
            QColor warColor(180, 180, 0, 255);
            _persistentMessage->setBrush(warColor);
        } else {
            return;
        }

        setToolTip(message);

        refreshSize();
    }
    refreshStateIndicator();

    const std::list<ViewerTab*>& viewers = getDagGui()->getGui()->getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->getViewer()->updatePersistentMessage();
    }
}

QVBoxLayout*
NodeGui::getDockContainer() const
{
    return _settingsPanel->getContainer();
}

void
NodeGui::paint(QPainter* /*painter*/,
               const QStyleOptionGraphicsItem* /*options*/,
               QWidget* /*parent*/)
{
    //nothing special
}

const std::list<std::pair<KnobIWPtr, KnobGuiPtr> > &
NodeGui::getKnobs() const
{
    assert(_settingsPanel);
    if (_mainInstancePanel) {
        return _mainInstancePanel->getKnobsMapping();
    }

    return _settingsPanel->getKnobsMapping();
}

void
NodeGui::serialize(NodeGuiSerialization* serializationObject) const
{
    serializationObject->initialize(this);
}

void
NodeGui::serializeInternal(std::list<NodeSerializationPtr>& internalSerialization) const
{
    NodePtr node = getNode();
    NodeSerializationPtr thisSerialization( new NodeSerialization(node, false) );

    internalSerialization.push_back(thisSerialization);

    ///For multi-instancs, serialize children too
    if ( node->isMultiInstance() ) {
        assert(_settingsPanel);
        MultiInstancePanelPtr panel = _settingsPanel->getMultiInstancePanel();
        assert(panel);

        const std::list<std::pair<NodeWPtr, bool> >& instances = panel->getInstances();
        for (std::list<std::pair<NodeWPtr, bool> >::const_iterator it = instances.begin();
             it != instances.end(); ++it) {
            NodeSerializationPtr childSerialization( new NodeSerialization(it->first.lock(), false) );
            internalSerialization.push_back(childSerialization);
        }
    }
}

void
NodeGui::restoreInternal(const NodeGuiPtr& thisShared,
                         const std::list<NodeSerializationPtr>& internalSerialization)
{
    assert(internalSerialization.size() >= 1);

    getSettingPanel()->pushUndoCommand( new LoadNodePresetsCommand(thisShared, internalSerialization) );
}

void
NodeGui::copyFrom(const NodeGuiSerialization & obj)
{
    float r, g, b;
    double overlayR, overlayB, overlayG;
    obj.getColor(&r, &g, &b);
    setCurrentColor( QColor::fromRgbF(r, g, b) );
    if (obj.getOverlayColor(&overlayR, &overlayG, &overlayB)) {
        setOverlayColor( QColor::fromRgbF(overlayR, overlayB, overlayG) );
    }
    setPos_mt_safe( QPointF( obj.getX(), obj.getY() ) );
    NodePtr node = getNode();
    assert(node);
    if ( node && ( node->isPreviewEnabled() != obj.isPreviewEnabled() ) ){
        togglePreview();
    }
    double w, h;
    obj.getSize(&w, &h);
    resize(w, h);
}

QUndoStackPtr
NodeGui::getUndoStack() const
{
    return _undoStack;
}

void
NodeGui::refreshStateIndicator()
{
    if (!_stateIndicator) {
        return;
    }
    QString message;
    int type;
    NodePtr node = getNode();
    if ( !node ) {
        return;
    }
    node->getPersistentMessage(&message, &type);

    bool showIndicator = true;
    int value = node->getIsNodeRenderingCounter();
    bool isSelected = getIsSelected();
    if (value >= 1) {
        _stateIndicator->setBrush(Qt::yellow);
    } else if (_mergeHintActive) {
        _stateIndicator->setBrush(Qt::green);
    } else if (isSelected) {
        _stateIndicator->setBrush(Qt::white);
    } else if ( !message.isEmpty() && ( (type == 1) || (type == 2) ) ) {
        if (type == 1) {
            _stateIndicator->setBrush( QColor(128, 0, 0, 255) ); //< error
        } else if (type == 2) {
            _stateIndicator->setBrush( QColor(80, 180, 0, 255) ); //< warning
        }
    } else {
        showIndicator = false;
    }

    if (_outputEdge) {
        _outputEdge->setUseSelected(isSelected);
    }
    for (std::vector<Edge*>::iterator it = _inputEdges.begin(); it != _inputEdges.end(); ++it) {
        if (*it) {
            (*it)->setUseSelected(isSelected);
        }
    }
    if ( showIndicator && !_stateIndicator->isVisible() ) {
        _stateIndicator->show();
    } else if ( !showIndicator && _stateIndicator->isVisible() ) {
        _stateIndicator->hide();
    } else {
        update();
    }
}

void
NodeGui::setMergeHintActive(bool active)
{
    if (active == _mergeHintActive) {
        return;
    }
    _mergeHintActive = active;
    refreshStateIndicator();
}

void
NodeGui::refreshRenderingIndicator()
{
    NodePtr node = getNode();

    if (!node) {
        return;
    }
    EffectInstancePtr effect = node->getEffectInstance();
    if (!effect) {
        return;
    }
    refreshStateIndicator();
    for (std::size_t i = 0; i < _inputEdges.size(); ++i) {
        int value = node->getIsInputNRenderingCounter(i);
        if (value >= 1) {
            _inputEdges[i]->turnOnRenderingColor();
        } else {
            _inputEdges[i]->turnOffRenderingColor();
        }
    }
    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>( effect.get() );
    if (isViewer) {
        ViewerGL* hasUI = dynamic_cast<ViewerGL*>( isViewer->getUiContext() );
        if (hasUI) {
            hasUI->getViewerTab()->refreshViewerRenderingState();
        }
    }
}

void
NodeGui::moveBelowPositionRecursively(const QRectF & r)
{
    QRectF sceneRect = mapToScene( boundingRect() ).boundingRect();

    NodePtr node = getNode();

    if (!node) {
        return;
    }
    if ( r.intersects(sceneRect) ) {
        changePosition(0, r.height() + NodeGui::DEFAULT_OFFSET_BETWEEN_NODES);
        const NodesWList & outputs = node->getGuiOutputs();
        for (NodesWList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
            NodePtr output = it->lock();
            if (!output) {
                continue;
            }
            NodeGuiIPtr outputGuiI = output->getNodeGui();
            if (!outputGuiI) {
                continue;
            }
            NodeGui* gui = dynamic_cast<NodeGui*>( outputGuiI.get() );
            assert(gui);
            sceneRect = mapToScene( boundingRect() ).boundingRect();
            gui->moveBelowPositionRecursively(sceneRect);
        }
    }
}

void
NodeGui::moveAbovePositionRecursively(const QRectF & r)
{
    QRectF sceneRect = mapToScene( boundingRect() ).boundingRect();

    if ( r.intersects(sceneRect) ) {
        changePosition(0, -r.height() - NodeGui::DEFAULT_OFFSET_BETWEEN_NODES);
        for (U32 i = 0; i < _inputEdges.size(); ++i) {
            if ( _inputEdges[i]->hasSource() ) {
                sceneRect = mapToScene( boundingRect() ).boundingRect();
                _inputEdges[i]->getSource()->moveAbovePositionRecursively(sceneRect);
            }
        }
    }
}

QPointF
NodeGui::getPos_mt_safe() const
{
    QMutexLocker l(&positionMutex);

    return pos();
}

void
NodeGui::setPos_mt_safe(const QPointF & pos)
{
    QMutexLocker l(&positionMutex);

    setPos(pos);
}

void
NodeGui::centerGraphOnIt()
{
    _graph->centerOnItem(this);
}

void
NodeGui::onAllKnobsSlaved(bool b)
{
    NodePtr node = getNode();
    assert(node);
    if (!node) {
        return;
    }

    if (b) {
        NodePtr masterNode = node->getMasterNode();
        assert(masterNode);
        NodeGuiIPtr masterNodeGui_i = masterNode->getNodeGui();
        assert(masterNodeGui_i);
        NodeGuiPtr masterNodeGui = std::dynamic_pointer_cast<NodeGui>(masterNodeGui_i);
        _masterNodeGui = masterNodeGui;
        assert(!_slaveMasterLink);

        if ( masterNode->getGroup() == node->getGroup() ) {
            _slaveMasterLink = new LinkArrow( masterNodeGui, shared_from_this(), parentItem() );
            _slaveMasterLink->setColor( QColor(200, 100, 100) );
            _slaveMasterLink->setArrowHeadColor( QColor(243, 137, 20) );
            _slaveMasterLink->setWidth(3);
        }
        if ( !node->isNodeDisabled() ) {
            if ( !isSelected() ) {
                applyBrush(_clonedColor);
            }
        }
    } else {
        if (_slaveMasterLink) {
            delete _slaveMasterLink;
            _slaveMasterLink = 0;
        }
        _masterNodeGui.reset();
        if ( !node->isNodeDisabled() ) {
            if ( !isSelected() ) {
                applyBrush(_currentColor);
            }
        }
    }
    update();
}

static QString
makeLinkString(Node* masterNode,
               KnobI* master,
               Node* slaveNode,
               KnobI* slave)
{
    QString tt = QString::fromUtf8("<p>");

    tt.append( QString::fromUtf8( masterNode->getLabel().c_str() ) );
    tt.append( QLatin1Char('.') );
    tt.append( QString::fromUtf8( master->getName().c_str() ) );


    tt.append( QString::fromUtf8(" (master) ") );

    tt.append( QString::fromUtf8("------->") );

    tt.append( QString::fromUtf8( slaveNode->getLabel().c_str() ) );
    tt.append( QString::fromUtf8(".") );
    tt.append( QString::fromUtf8( slave->getName().c_str() ) );


    tt.append( QString::fromUtf8(" (slave)</p>") );

    return tt;
}

void
NodeGui::onKnobExpressionChanged(const KnobGui* knob)
{
    KnobIPtr internalKnob = knob->getKnob();

    for (KnobGuiLinks::iterator it = _knobsLinks.begin(); it != _knobsLinks.end(); ++it) {
        int totalLinks = 0;
        int totalInvalid = 0;
        bool isCurrentLink = false;

        for (std::list<LinkedKnob>::iterator it2 = it->second.knobs.begin(); it2 != it->second.knobs.end(); ++it2) {
            KnobIPtr slave = it2->slave.lock();
            if (slave == internalKnob) {
                isCurrentLink = true;
            }
            int ndims = slave->getDimension();
            int invalid = 0;
            for (int i = 0; i < ndims; ++i) {
                if ( !slave->getExpression(i).empty() && !slave->isExpressionValid(i, 0) ) {
                    ++invalid;
                }
            }
            totalLinks += it2->dimensions.size();
            totalInvalid += invalid;

            it2->linkInValid = invalid;
        }
        if (isCurrentLink) {
            if (totalLinks > 0) {
                it->second.arrow->setVisible(totalLinks > totalInvalid);
            }
            break;
        }
    }
}

void
NodeGui::onKnobsLinksChanged()
{
    if (!_expressionIndicator) {
        return;
    }
    NodePtr node = getNode();
    assert(node);
    if (!node) {
        return;
    }

    typedef std::list<Node::KnobLink> InternalLinks;
    InternalLinks links;
    node->getKnobsLinks(links);

    ///1st pass: remove the no longer needed links
    KnobGuiLinks newLinks;
    for (KnobGuiLinks::iterator it = _knobsLinks.begin(); it != _knobsLinks.end(); ++it) {
        bool found = false;
        for (InternalLinks::iterator it2 = links.begin(); it2 != links.end(); ++it2) {
            if ( it2->masterNode.lock() == it->first.lock() ) {
                found = true;
                break;
            }
        }
        if (!found) {
            delete it->second.arrow;
        } else {
            newLinks.insert(*it);
        }
    }
    _knobsLinks = newLinks;

    ///2nd pass: create the new links

    NodeGuiPtr thisShared = shared_from_this();

    for (InternalLinks::iterator it = links.begin(); it != links.end(); ++it) {
        NodePtr masterNode = it->masterNode.lock();
        KnobGuiLinks::iterator foundGuiLink = masterNode ? _knobsLinks.find(it->masterNode) : _knobsLinks.end();
        if ( foundGuiLink != _knobsLinks.end() ) {
            //We already have a link to the master node
            std::list<LinkedKnob>::iterator found = foundGuiLink->second.knobs.end();

            for (std::list<LinkedKnob>::iterator it2 = foundGuiLink->second.knobs.begin(); it2 != foundGuiLink->second.knobs.end(); ++it2) {
                if ( ( it2->slave.lock() == it->slave.lock() ) && ( it2->master.lock() == it->master.lock() ) ) {
                    found = it2;
                    break;
                }
            }
            if ( found == foundGuiLink->second.knobs.end() ) {
                ///There's no link for this knob, add info to the tooltip of the link arrow
                LinkedKnob k;
                k.slave = it->slave;
                k.master = it->master;
                k.dimensions.insert(it->dimension);
                k.linkInValid = 0;
                foundGuiLink->second.knobs.push_back(k);
                QString fullToolTip;
                for (std::list<LinkedKnob>::iterator it2 = foundGuiLink->second.knobs.begin(); it2 != foundGuiLink->second.knobs.end(); ++it2) {
                    QString tt = makeLinkString( masterNode.get(), it2->master.lock().get(), node.get(), it2->slave.lock().get() );
                    fullToolTip.append(tt);
                }
            } else {
                found->dimensions.insert(it->dimension);
            }
        } else {
            ///There's no link to the master node yet
            if ( masterNode && (masterNode->getNodeGui().get() != this) && ( masterNode->getGroup() == node->getGroup() ) ) {
                NodeGuiIPtr master_i = masterNode->getNodeGui();
                NodeGuiPtr master = std::dynamic_pointer_cast<NodeGui>(master_i);
                assert(master);

                LinkArrow* arrow = new LinkArrow( master, thisShared, parentItem() );
                arrow->setWidth(2);
                arrow->setColor( QColor(143, 201, 103) );
                arrow->setArrowHeadColor( QColor(200, 255, 200) );

                QString tt = makeLinkString( masterNode.get(), it->master.lock().get(), node.get(), it->slave.lock().get() );
                arrow->setToolTip(tt);
                if ( !getDagGui()->areKnobLinksVisible() ) {
                    arrow->setVisible(false);
                }
                LinkedDim& guilink = _knobsLinks[it->masterNode];
                guilink.arrow = arrow;
                LinkedKnob k;
                k.slave = it->slave;
                k.master = it->master;
                k.dimensions.insert(it->dimension);
                k.linkInValid = 0;
                guilink.knobs.push_back(k);
            }
        }
    }

    if (links.size() > 0) {
        if ( !_expressionIndicator->isActive() ) {
            _expressionIndicator->setActive(true);
        }
    } else {
        if ( _expressionIndicator->isActive() ) {
            _expressionIndicator->setActive(false);
        }
    }
} // onKnobsLinksChanged

void
NodeGui::refreshOutputEdgeVisibility()
{
    if (_outputEdge) {
        NodePtr node = getNode();
        if ( node && node->getGuiOutputs().empty() ) {
            if ( !_outputEdge->isVisible() ) {
                _outputEdge->setActive(true);
                _outputEdge->show();
            }
        } else {
            if ( _outputEdge->isVisible() ) {
                _outputEdge->setActive(false);
                _outputEdge->hide();
            }
        }
    }
}

void
NodeGui::destroyGui()
{
    Gui* guiObj = getDagGui()->getGui();

    if (!guiObj) {
        return;
    }

    //Remove undo stack
    removeUndoStack();


    NodeGuiPtr thisShared = shared_from_this();

    {
        ///Remove from the nodegraph containers
        _graph->deleteNodePermanantly(thisShared);
    }
    NodePtr internalNode = _internalNode.lock();
    assert(internalNode);

    //Remove viewer UI
    guiObj->removeNodeViewerInterface(thisShared, true);


    //Remove from curve editor
    guiObj->getCurveEditor()->removeNode(this);

    //Remove from dope sheet
    guiObj->getDopeSheetEditor()->removeNode(this);


    //Remove nodegraph if group
    if ( internalNode->getEffectInstance() ) {
        NodeGroup* isGrp = internalNode->isEffectGroup();
        if (isGrp) {
            NodeGraphI* graph_i = isGrp->getNodeGraph();
            if (graph_i) {
                NodeGraph* graph = dynamic_cast<NodeGraph*>(graph_i);
                assert(graph);
                if (graph) {
                    guiObj->removeGroupGui(graph, true);
                }
            }
        }
    }


    // remove from clipboard if existing
    if (internalNode) {
        ///remove the node from the clipboard if it is
        NodeClipBoard &cb = appPTR->getNodeClipBoard();
        for (std::list<NodeSerializationPtr>::iterator it = cb.nodes.begin();
             it != cb.nodes.end(); ++it) {
            if ( (*it)->getNode() == internalNode ) {
                cb.nodes.erase(it);
                break;
            }
        }

        for (std::list<NodeGuiSerializationPtr>::iterator it = cb.nodesUI.begin();
             it != cb.nodesUI.end(); ++it) {
            if ( (*it)->getFullySpecifiedName() == internalNode->getFullyQualifiedName() ) {
                cb.nodesUI.erase(it);
                break;
            }
        }
    }

    //Delete edges
    for (InputEdges::const_iterator it = _inputEdges.begin(); it != _inputEdges.end(); ++it) {
        QGraphicsScene* scene = (*it)->scene();
        if (scene) {
            scene->removeItem( (*it) );
        }
        (*it)->setParentItem(0);
        delete *it;
    }
    _inputEdges.clear();

    if (_outputEdge) {
        QGraphicsScene* scene = _outputEdge->scene();
        if (scene) {
            scene->removeItem(_outputEdge);
        }
        _outputEdge->setParentItem(0);
        delete _outputEdge;
        _outputEdge = 0;
    }

    //Delete settings panel
    delete _settingsPanel;
    _settingsPanel = 0;
} // NodeGui::destroyGui

QSize
NodeGui::getSize() const
{
    if ( QThread::currentThread() == qApp->thread() ) {
        QRectF bbox = boundingRect();

        return QSize( bbox.width(), bbox.height() );
    } else {
        QMutexLocker k(&_mtSafeSizeMutex);

        return QSize(_mtSafeWidth, _mtSafeHeight);
    }
}

void
NodeGui::setSize(double w,
                 double h)
{
    resize(w, h);
}

void
NodeGui::onDisabledKnobToggled(bool disabled)
{
    if (!_nameItem) {
        return;
    }

    NodePtr node = getNode();
    assert(node);
    if (!node) {
        return;
    }
    ///When received whilst the node is under a multi instance, let the MultiInstancePanel call this slot instead.
    if ( ( sender() == node.get() ) && node->isMultiInstance() ) {
        return;
    }

    int firstFrame, lastFrame;
    bool lifetimeEnabled = node->isLifetimeActivated(&firstFrame, &lastFrame);
    int curFrame = node->getApp()->getTimeLine()->currentFrame();
    bool enabled = ( !lifetimeEnabled || (curFrame >= firstFrame && curFrame <= lastFrame) ) && !disabled;

    _disabledTopLeftBtmRight->setVisible(!enabled);
    _disabledBtmLeftTopRight->setVisible(!enabled);
    update();
}

void
NodeGui::onStreamWarningsChanged()
{
    if (!_streamIssuesWarning) {
        return;
    }

    std::map<Node::StreamWarningEnum, QString> warnings;
    NodePtr node = getNode();
    assert(node);
    if (!node) {
        return;
    }
    node->getStreamWarnings(&warnings);
    if ( warnings.empty() ) {
        _streamIssuesWarning->setActive(false);

        return;
    }
    QString tooltip;
    for (std::map<Node::StreamWarningEnum, QString>::iterator it = warnings.begin(); it != warnings.end(); ++it) {
        if ( it->second.isEmpty() ) {
            continue;
        }
        QString tt = QString::fromUtf8("<p><b>") + tr("Stream issue:") + QString::fromUtf8("</b></p>");
        tt += NATRON_NAMESPACE::convertFromPlainText(it->second.trimmed(), NATRON_NAMESPACE::WhiteSpaceNormal);
        tooltip += tt;
    }
    setToolTip(tooltip);
    _streamIssuesWarning->setToolTip(tooltip);
    _streamIssuesWarning->setActive( !tooltip.isEmpty() );
}

////////////////////////////////////////// NodeGuiIndicator ////////////////////////////////////////////////////////

struct NodeGuiIndicatorPrivate
{
    QGraphicsEllipseItem* ellipse;
    QGraphicsSimpleTextItem* textItem;
    QGradientStops gradStops;

    NodeGuiIndicatorPrivate(NodeGraph* graph,
                            int depth,
                            const QString & text,
                            const QPointF & topLeft,
                            int width,
                            int height,
                            const QGradientStops & gradient,
                            const QColor & textColor,
                            QGraphicsItem* parent)
        : ellipse(NULL)
        , textItem(NULL)
        , gradStops(gradient)
    {
        ellipse = new QGraphicsEllipseItem(parent);
        int ellipseRad = width / 2;
        QPoint ellipsePos(topLeft.x() + (width / 2) - ellipseRad, -ellipseRad);
        QRectF ellipseRect(ellipsePos.x(), ellipsePos.y(), width, height);
        ellipse->setRect(ellipseRect);
        ellipse->setZValue(depth);

        QPointF ellipseCenter = ellipseRect.center();
        QRadialGradient radialGrad(ellipseCenter, ellipseRad);
        radialGrad.setStops(gradStops);
        ellipse->setBrush(radialGrad);


        textItem = new NodeGraphSimpleTextItem(graph, parent, false);
        textItem->setText(text);


        QPointF sceneCenter = ellipse->mapToScene( ellipse->mapFromParent(ellipseCenter) );
        QRectF textBr = textItem->mapToScene( textItem->boundingRect() ).boundingRect();
        sceneCenter.ry() -= (textBr.height() / 2.);
        sceneCenter.rx() -= (textBr.width() / 2.);
        QPointF textPos = textItem->mapToParent( textItem->mapFromScene(sceneCenter) );
        textItem->setPos(textPos);


        textItem->setBrush(textColor);
        textItem->setZValue(depth);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
        textItem->scale(0.8, 0.8);
#else
        textItem->setScale(0.8);
#endif
    }
};

NodeGuiIndicator::NodeGuiIndicator(NodeGraph* graph,
                                   int depth,
                                   const QString & text,
                                   const QPointF & topLeft,
                                   int width,
                                   int height,
                                   const QGradientStops & gradient,
                                   const QColor & textColor,
                                   QGraphicsItem* parent)
    : _imp( new NodeGuiIndicatorPrivate(graph, depth, text, topLeft, width, height, gradient, textColor, parent) )
{
}

NodeGuiIndicator::~NodeGuiIndicator()
{
}

void
NodeGuiIndicator::setToolTip(const QString & tooltip)
{
    _imp->ellipse->setToolTip(tooltip);
}

void
NodeGuiIndicator::setActive(bool active)
{
    _imp->ellipse->setActive(active);
    _imp->textItem->setActive(active);
    _imp->ellipse->setVisible(active);
    _imp->textItem->setVisible(active);
}

bool
NodeGuiIndicator::isActive() const
{
    return _imp->ellipse->isVisible();
}

void
NodeGuiIndicator::refreshPosition(const QPointF & center)
{
    QRectF r = _imp->ellipse->rect();
    int ellipseRad = r.width() / 2;
    QPoint ellipsePos(center.x() - ellipseRad, center.y() - ellipseRad);
    QRectF ellipseRect( ellipsePos.x(), ellipsePos.y(), r.width(), r.height() );

    _imp->ellipse->setRect(ellipseRect);

    QRadialGradient radialGrad(ellipseRect.center(), ellipseRad);
    radialGrad.setStops(_imp->gradStops);
    _imp->ellipse->setBrush(radialGrad);
    QPointF ellipseCenter = ellipseRect.center();
    QPointF sceneCenter = _imp->ellipse->mapToScene( _imp->ellipse->mapFromParent(ellipseCenter) );
    QRectF textBr = _imp->textItem->mapToScene( _imp->textItem->boundingRect() ).boundingRect();
    sceneCenter.ry() -= (textBr.height() / 2.);
    sceneCenter.rx() -= (textBr.width() / 2.);
    QPointF textPos = _imp->textItem->mapToParent( _imp->textItem->mapFromScene(sceneCenter) );
    _imp->textItem->setPos(textPos);
}

///////////////////

void
NodeGui::setScale_natron(double scale)
{
    setScale(scale);
    for (InputEdges::iterator it = _inputEdges.begin(); it != _inputEdges.end(); ++it) {
        (*it)->setScale(scale);
    }

    if (_outputEdge) {
        _outputEdge->setScale(scale);
    }
    refreshEdges();
    NodePtr node = getNode();
    assert(node);
    if (!node) {
        update();
        return;
    }
    const NodesWList & outputs = node->getGuiOutputs();
    for (NodesWList::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        NodePtr output = it->lock();
        if (output) {
            output->doRefreshEdgesGUI();
        }
    }
    update();
}

void
NodeGui::removeHighlightOnAllEdges()
{
    for (InputEdges::iterator it = _inputEdges.begin(); it != _inputEdges.end(); ++it) {
        (*it)->setUseHighlight(false);
    }
    if (_outputEdge) {
        _outputEdge->setUseHighlight(false);
    }
}

Edge*
NodeGui::getInputArrow(int inputNb) const
{
    if (inputNb == -1) {
        return _outputEdge;
    }
    if ( inputNb >= (int)_inputEdges.size() ) {
        return 0;
    }

    return _inputEdges[inputNb];
}

Edge*
NodeGui::getOutputArrow() const
{
    return _outputEdge;
}

void
NodeGui::setNameItemHtml(const QString & name,
                         const QString & label)
{
    if ( !_graph->getGui() || !_nameItem) {
        return;
    }
    QString textLabel;

    if ( !label.isEmpty() ) {
        QString labelCopy = label;

        ///remove any custom data tag natron might have added
        QString startCustomTag = QString::fromUtf8(NATRON_CUSTOM_HTML_TAG_START);
        int startCustomData = labelCopy.indexOf(startCustomTag);
        if (startCustomData != -1) {
            labelCopy.remove( startCustomData, startCustomTag.size() );

            QString endCustomTag = QString::fromUtf8(NATRON_CUSTOM_HTML_TAG_END);
            int endCustomData = labelCopy.indexOf(endCustomTag, startCustomData);
            assert(endCustomData != -1);
            labelCopy.remove( endCustomData, endCustomTag.size() );
            //labelCopy.insert( endCustomData, QLatin1Char('\n') );
        }

        ///add the node name into the html encoded label
        int startFontTag = labelCopy.indexOf( QString::fromUtf8("<font size=") );
        if (startFontTag != -1) {
            QString toFind = QString::fromUtf8("\">");
            int endFontTag = labelCopy.indexOf(toFind, startFontTag);
            if (endFontTag != -1) {
                endFontTag += toFind.size();
            }
            QString toInsert = name.trimmed();
            int labelEndPos = -1;
            // is there really something before the closing font tag? if yes, insert a newline
            if (endFontTag != -1) {
                labelEndPos = labelCopy.indexOf(QString::fromUtf8("</font>"), endFontTag);
                if (labelEndPos != endFontTag) {
                    toInsert += QLatin1Char('\n');
                }
            }
            if (!_channelsExtraLabel.isEmpty()) {
                QString extra = QLatin1Char('\n') + _channelsExtraLabel;
                if (labelEndPos != -1) {
                    labelCopy.insert(labelEndPos, extra);
                } else {
                    toInsert += extra;
                }
            }
            labelCopy.insert(endFontTag == -1 ? 0 : endFontTag, toInsert);
        } else {
            labelCopy.prepend( name.trimmed() + QLatin1Char('\n') );
            if ( !_channelsExtraLabel.isEmpty() ) {
                labelCopy.append( QLatin1Char('\n') + _channelsExtraLabel.trimmed() );
            }
            ///Default to something not too bad
            /*QString fontTag = (QString("<font size=\"%1\" color=\"%2\" face=\"%3\">")
                               .arg(6)
                               .arg( QColor(Qt::black).name() )
                               .arg(QApplication::font().family()));
               labelCopy.prepend(fontTag);
               labelCopy.append("</font>");*/
        }
        textLabel.append(labelCopy);
    } else {
        ///Default to something not too bad
        /*QString fontTag = (QString("<font size=\"%1\" color=\"%2\" face=\"%3\">")
                           .arg(6)
                           .arg( QColor(Qt::black).name() )
                           .arg(QApplication::font().family()));
           textLabel.append(fontTag);*/
        textLabel.append(name + QLatin1Char('\n') + _channelsExtraLabel);
        //textLabel.append("</font>");
    }
    QString finalText;
    finalText += textLabel.trimmed();

    replaceLineBreaksWithHtmlParagraph(finalText);

    finalText.prepend( QString::fromUtf8("<div align=\"center\">") );
    finalText.append( QString::fromUtf8("</div>") );

    int startFontTag = finalText.indexOf( QString::fromUtf8("<font size=") );
    int endFontTag = -1;
    if (startFontTag != -1) {
        startFontTag = finalText.indexOf(QString::fromUtf8("\">"), startFontTag);
    }

    QString oldText = _nameItem->toHtml();
    if (finalText == oldText) {
        // Nothing changed
        return;
    }

    QFont f;
    QColor color = Qt::black;
    if (startFontTag != -1) {
        KnobGuiString::parseFont(finalText, &f, &color);
        //Remove font from the HTML
        finalText.remove(startFontTag, endFontTag - startFontTag);
    } else {
        f = QApplication::font();
    }
    bool antialias = appPTR->getCurrentSettings()->isNodeGraphAntiAliasingEnabled();
    if (!antialias) {
        f.setStyleStrategy(QFont::NoAntialias);
    }
    _nameItem->setDefaultTextColor(color);
    _nameItem->setFont(f);
    _nameItem->setHtml(finalText);
    _nameItem->adjustSize();


    QRectF bbox = boundingRect();
    resize( bbox.width(), bbox.height(), false, !label.isEmpty() );
} // setNameItemHtml

void
NodeGui::onOutputLayerChanged()
{
    NodePtr internalNode = getNode();
    if (!internalNode) {
        return;
    }

    QString extraLayerStr;
    KnobBoolPtr processAllKnob = internalNode->getProcessAllLayersKnob();
    bool processAll = false;
    if (processAllKnob && processAllKnob->hasModifications()) {
        processAll = processAllKnob->getValue();
        if (processAll) {
            //extraLayerStr.append( QString::fromUtf8("<br />") );
            extraLayerStr += tr("(All)");
        }
    }
    KnobChoicePtr layerKnob = internalNode->getChannelSelectorKnob(-1);
    ImagePlaneDesc outputLayer;
    {
        bool isAll;
        std::list<ImagePlaneDesc> availableLayers;
        internalNode->getEffectInstance()->getAvailableLayers(internalNode->getApp()->getTimeLine()->currentFrame(), ViewIdx(0), -1,  &availableLayers);


        internalNode->getSelectedLayer(-1, availableLayers, 0, &isAll, &outputLayer);
    }
    if (!processAll && outputLayer.getNumComponents() > 0) {
        if (!outputLayer.isColorPlane()) {
            if (!extraLayerStr.isEmpty()) {
                extraLayerStr.append( QString::fromUtf8("<br />") );
            }
            extraLayerStr.push_back( QLatin1Char('(') );
            extraLayerStr.append( QString::fromUtf8( outputLayer.getPlaneLabel().c_str() ) );
            extraLayerStr.push_back( QLatin1Char(')') );
        }
    }
    if (extraLayerStr == _channelsExtraLabel) {
        return;
    }
    _channelsExtraLabel = extraLayerStr;
    setNameItemHtml(QString::fromUtf8( internalNode->getLabel().c_str() ), _nodeLabel);
}

void
NodeGui::refreshNodeText(const QString & label)
{
    if ( !_graph->getGui() ) {
        return;
    }
    NodePtr node = getNode();
    assert(node);
    if (!node) {
        return;
    }
    _nodeLabel = label;
    if ( node->isMultiInstance() ) {
        ///The multi-instances store in the kNatronOfxParamStringSublabelName knob the name of the instance
        ///Since the "main-instance" is the one displayed on the node-graph we don't want it to display its name
        ///hence we remove it
        _nodeLabel = KnobGuiString::removeNatronHtmlTag(_nodeLabel);
    }
    replaceLineBreaksWithHtmlParagraph(_nodeLabel); ///< maybe we should do this in the knob itself when the user writes ?
    setNameItemHtml(QString::fromUtf8( node->getLabel().c_str() ), _nodeLabel);

    //For the merge node, set its operator icon
    if ( node->getPlugin()->getPluginID() == QString::fromUtf8(PLUGINID_OFX_MERGE) ) {
        QString subLabelContent = KnobGuiString::getNatronHtmlTagContent(label);
        if  ( !subLabelContent.isEmpty() ) {
            //Remove surrounding parenthesis
            if ( subLabelContent[0] == QLatin1Char('(') ) {
                subLabelContent.remove(0, 1);
            }
            if ( subLabelContent[subLabelContent.size() - 1] == QLatin1Char(')') ) {
                subLabelContent.remove(subLabelContent.size() - 1, 1);
            }
        }
        assert(_presetIcon);
        if (_presetIcon) {
            QPixmap pix;
            getPixmapForMergeOperator(subLabelContent, &pix);
            if ( pix.isNull() ) {
                _presetIcon->setVisible(false);
            } else {
                _presetIcon->setVisible(true);
                _presetIcon->setPixmap(pix);
            }
        }
        refreshSize();
    }
}

QColor
NodeGui::getCurrentColor() const
{
    QMutexLocker k(&_currentColorMutex);

    return _currentColor;
}

void
NodeGui::setCurrentColor(const QColor & c)
{
    onSettingsPanelColorChanged(c);
    if (_settingsPanel) {
        _settingsPanel->setCurrentColor(c);
    }
}

void
NodeGui::setOverlayColor(const QColor& c)
{
    if (_settingsPanel) {
        _settingsPanel->setOverlayColor(c);
    }
}

void
NodeGui::onSwitchInputActionTriggered()
{
    NodePtr node = getNode();

    if (node && node->getNInputs() >= 2) {
        node->switchInput0And1();
        std::list<ViewerInstance* > viewers;
        node->hasViewersConnected(&viewers);
        for (std::list<ViewerInstance* >::iterator it = viewers.begin(); it != viewers.end(); ++it) {
            (*it)->renderCurrentFrame(true);
        }
        update();
        node->getApp()->triggerAutoSave();
    }
}

///////////////////

TextItem::TextItem(QGraphicsItem* parent )
    : QGraphicsTextItem(parent)
    , _alignement(Qt::AlignCenter)
{
    init();
}

TextItem::TextItem(const QString & text,
                   QGraphicsItem* parent)
    : QGraphicsTextItem(text, parent)
    , _alignement(Qt::AlignCenter)
{
    init();
}

void
TextItem::setAlignment(Qt::Alignment alignment)
{
    _alignement = alignment;
    QTextBlockFormat format;
    format.setAlignment(alignment);
    QTextCursor cursor = textCursor();      // save cursor position
    int position = textCursor().position();
    cursor.select(QTextCursor::Document);
    cursor.mergeBlockFormat(format);
    cursor.clearSelection();
    cursor.setPosition(position);           // restore cursor position
    setTextCursor(cursor);
}

int
TextItem::type() const
{
    return Type;
}

void
TextItem::updateGeometry(int,
                         int,
                         int)
{
    updateGeometry();
}

void
TextItem::updateGeometry()
{
    QPointF topRightPrev = boundingRect().topRight();

    setTextWidth(-1);
    setTextWidth( boundingRect().width() );
    setAlignment(_alignement);
    QPointF topRight = boundingRect().topRight();

    if (_alignement & Qt::AlignRight) {
        setPos( pos() + (topRightPrev - topRight) );
    }
}

void
TextItem::init()
{
    updateGeometry();
    connect( document(), SIGNAL(contentsChange(int,int,int)),
             this, SLOT(updateGeometry(int,int,int)) );
}

void
NodeGui::refreshKnobsAfterTimeChange(bool onlyTimeEvaluationKnobs,
                                     SequenceTime time)
{
    NodePtr node = getNode();
    assert(node);
    if (!node) {
        return;
    }
    if ( ( _settingsPanel && !_settingsPanel->isClosed() ) ) {
        if (onlyTimeEvaluationKnobs) {
            node->getEffectInstance()->refreshAfterTimeChangeOnlyKnobsWithTimeEvaluation(time);
        } else {
            node->getEffectInstance()->refreshAfterTimeChange(false, time);
        }
    } else if ( !node->getParentMultiInstanceName().empty() ) {
        node->getEffectInstance()->refreshInstanceSpecificKnobsOnly(false, time);
    }
}

void
NodeGui::onSettingsPanelClosedChanged(bool closed)
{
    if (!_settingsPanel) {
        return;
    }

    DockablePanel* panel = dynamic_cast<DockablePanel*>( sender() );
    assert(panel);
    if (panel == _settingsPanel) {
        ///if it is a multiinstance, notify the multi instance panel
        if (_mainInstancePanel) {
            _settingsPanel->getMultiInstancePanel()->onSettingsPanelClosed(closed);
        } else {
            NodePtr node = getNode();
            assert(node);
            if (node && !closed) {
                SequenceTime time = node->getApp()->getTimeLine()->currentFrame();
                node->getEffectInstance()->refreshAfterTimeChange(false, time);
            }
        }
    }
}

MultiInstancePanelPtr NodeGui::getMultiInstancePanel() const
{
    if (_settingsPanel) {
        return _settingsPanel->getMultiInstancePanel();
    } else {
        return MultiInstancePanelPtr();
    }
}

TrackerPanel*
NodeGui::getTrackerPanel() const
{
    return _settingsPanel ? _settingsPanel->getTrackerPanel() : 0;
}

void
NodeGui::setParentMultiInstance(const NodeGuiPtr & node)
{
    _parentMultiInstance = node;
}

void
NodeGui::setKnobLinksVisible(bool visible)
{
    for (KnobGuiLinks::iterator it = _knobsLinks.begin(); it != _knobsLinks.end(); ++it) {
        it->second.arrow->setVisible(visible);
    }
}

void
NodeGui::onParentMultiInstancePositionChanged(int x,
                                              int y)
{
    refreshPosition(x, y, true);
}

void
NodeGui::onInternalNameChanged(const QString & s)
{
    if (_settingNameFromGui) {
        return;
    }

    setNameItemHtml(s, _nodeLabel);

    if (_settingsPanel) {
        _settingsPanel->setName(s);
    }
    scene()->update();
}

void
NodeGui::setName(const QString & newName)
{
    std::string stdName = newName.toStdString();

    stdName = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(stdName);

    NodePtr node = getNode();
    assert(node);
    if (!node) {
        return;
    }
    std::string oldScriptName = node->getScriptName();
    try {
        node->setScriptName(stdName);
    } catch (const std::exception& e) {
        Dialogs::errorDialog(tr("Rename").toStdString(), tr("Could not set node script-name to ").toStdString() + stdName + ": " + e.what());
        return;
    }

    _settingNameFromGui = true;
    node->setLabel( newName.toStdString() );
    _settingNameFromGui = false;

    onInternalNameChanged(newName);
}

void
NodeGui::setPosition(double x,
                     double y)
{
    refreshPosition(x, y, true);
}

void
NodeGui::getPosition(double *x,
                     double* y) const
{
    QPointF pos = getPos_mt_safe();

    *x = pos.x();
    *y = pos.y();
}

void
NodeGui::getSize(double* w,
                 double* h) const
{
    QSize s = getSize();

    *w = s.width();
    *h = s.height();
}

void
NodeGui::exportGroupAsPythonScript()
{
    NodePtr node = getNode();

    if (!node) {
        return;
    }
    NodeGroup* isGroup = node->isEffectGroup();
    if (!isGroup) {
        qDebug() << "Attempting to export a non-group as a python script.";

        return;
    }
    getDagGui()->getGui()->exportGroupAsPythonScript(isGroup);
}

void
NodeGui::getColor(double* r,
                  double *g,
                  double* b) const
{
    QColor c = getCurrentColor();

    *r = c.redF();
    *g = c.greenF();
    *b = c.blueF();
}

void
NodeGui::setColor(double r,
                  double g,
                  double b)
{
    QColor c;

    c.setRgbF(r, g, b);
    setCurrentColor(c);
}

void
NodeGui::addDefaultInteract(const HostOverlayKnobsPtr& knobs)
{
    assert( QThread::currentThread() == qApp->thread() );
    if (!_hostOverlay) {
        _hostOverlay.reset( new HostOverlay( shared_from_this() ) );
    }

    if ( _hostOverlay->addInteract(knobs) ) {
        getDagGui()->getGui()->redrawAllViewers();
    }
}

HostOverlayPtr
NodeGui::getHostOverlay() const
{
    return _hostOverlay;
}

bool
NodeGui::hasHostOverlay() const
{
    if (_hostOverlay) {
        return true;
    }

    return false;
}

void
NodeGui::setCurrentViewportForHostOverlays(OverlaySupport* viewPort)
{
    if (_hostOverlay) {
        _hostOverlay->setCallingViewport(viewPort);
    }
}

void
NodeGui::drawHostOverlay(double time,
                         const RenderScale& renderScale,
                         ViewIdx view)
{
    if (_hostOverlay) {
        OverlaySupport* overlaySupport = _hostOverlay->getLastCallingViewport();
        NatronOverlayInteractSupport::OGLContextSaver s( overlaySupport );
        _hostOverlay->draw(time, renderScale, view);
    }
}

bool
NodeGui::onOverlayPenDownDefault(double time,
                                 const RenderScale& renderScale,
                                 ViewIdx view,
                                 const QPointF & viewportPos,
                                 const QPointF & pos,
                                 double pressure)
{
    if (_hostOverlay) {
        return _hostOverlay->penDown(time, renderScale, view, pos, viewportPos.toPoint(), pressure);
    }

    return false;
}

bool
NodeGui::onOverlayPenDoubleClickedDefault(double time,
                                          const RenderScale& renderScale,
                                          ViewIdx view,
                                          const QPointF & viewportPos,
                                          const QPointF & pos)
{
    if (_hostOverlay) {
        return _hostOverlay->penDoubleClicked( time, renderScale, view, pos, viewportPos.toPoint() );
    }

    return false;
}

bool
NodeGui::onOverlayPenMotionDefault(double time,
                                   const RenderScale& renderScale,
                                   ViewIdx view,
                                   const QPointF & viewportPos,
                                   const QPointF & pos,
                                   double pressure)
{
    if (_hostOverlay) {
        return _hostOverlay->penMotion(time, renderScale, view, pos, viewportPos.toPoint(), pressure);
    }

    return false;
}

bool
NodeGui::onOverlayPenUpDefault(double time,
                               const RenderScale& renderScale,
                               ViewIdx view,
                               const QPointF & viewportPos,
                               const QPointF & pos,
                               double pressure)
{
    if (_hostOverlay) {
        return _hostOverlay->penUp(time, renderScale, view, pos, viewportPos.toPoint(), pressure);
    }

    return false;
}

bool
NodeGui::onOverlayKeyDownDefault(double time,
                                 const RenderScale& renderScale,
                                 ViewIdx view,
                                 Key key,
                                 KeyboardModifiers /*modifiers*/)
{
    if (_hostOverlay) {
        QByteArray keyStr;

        return _hostOverlay->keyDown( time, renderScale, view, (int)key, keyStr.data() );
    }

    return false;
}

bool
NodeGui::onOverlayKeyUpDefault(double time,
                               const RenderScale& renderScale,
                               ViewIdx view,
                               Key key,
                               KeyboardModifiers /*modifiers*/)
{
    if (_hostOverlay) {
        QByteArray keyStr;

        return _hostOverlay->keyUp( time, renderScale, view, (int)key, keyStr.data() );
    }

    return false;
}

bool
NodeGui::onOverlayKeyRepeatDefault(double time,
                                   const RenderScale& renderScale,
                                   ViewIdx view,
                                   Key key,
                                   KeyboardModifiers /*modifiers*/)
{
    if (_hostOverlay) {
        QByteArray keyStr;

        return _hostOverlay->keyRepeat( time, renderScale, view, (int)key, keyStr.data() );
    }

    return false;
}

bool
NodeGui::onOverlayFocusGainedDefault(double time,
                                     const RenderScale& renderScale,
                                     ViewIdx view)
{
    if (_hostOverlay) {
        QByteArray keyStr;

        return _hostOverlay->gainFocus(time, renderScale, view);
    }

    return false;
}

bool
NodeGui::onOverlayFocusLostDefault(double time,
                                   const RenderScale& renderScale,
                                   ViewIdx view)
{
    if (_hostOverlay) {
        QByteArray keyStr;

        return _hostOverlay->loseFocus(time, renderScale, view);
    }

    return false;
}

bool
NodeGui::hasHostOverlayForParam(const KnobI* param)
{
    if (_hostOverlay) {
        return _hostOverlay->hasHostOverlayForParam(param);
    }

    return false;
}

void
NodeGui::removePositionHostOverlay(KnobI* knob)
{
    if (_hostOverlay) {
        _hostOverlay->removePositionHostOverlay(knob);
        if ( _hostOverlay->isEmpty() ) {
            _hostOverlay.reset();
        }
    }
}


void
NodeGui::setPluginIDAndVersion(const std::list<std::string>& /*grouping*/,
                               const std::string& pluginLabel,
                               const std::string& pluginID,
                               const std::string& pluginDesc,
                               const std::string& pluginIconFilePath,
                               unsigned int version)
{
    QColor color;
    if ( getColorFromGrouping(&color) ) {
        setCurrentColor(color);
    }
    if ( getSettingPanel() ) {
        getSettingPanel()->setPluginIDAndVersion(pluginLabel, pluginID, pluginDesc, version);
    }

    SettingsPtr currentSettings = appPTR->getCurrentSettings();
    QPixmap p( QString::fromUtf8( pluginIconFilePath.c_str() ) );

    if ( p.isNull() || !currentSettings->isPluginIconActivatedOnNodeGraph() ) {
        return;
    }
    int size = TO_DPIX(NATRON_PLUGIN_ICON_SIZE);
    if (std::max( p.width(), p.height() ) != size) {
        p = p.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    if ( getSettingPanel() ) {
        getSettingPanel()->setPluginIcon(p);
    }

    if (!_pluginIcon) {
        _pluginIcon = new NodeGraphPixmapItem(getDagGui(), this);
        _pluginIcon->setZValue(getBaseDepth() + 1);
        _pluginIconFrame = new QGraphicsRectItem(this);
        _pluginIconFrame->setZValue( getBaseDepth() );

        int r, g, b;
        currentSettings->getPluginIconFrameColor(&r, &g, &b);
        _pluginIconFrame->setBrush( QColor(r, g, b) );
    }

    if (_pluginIcon) {
        _pluginIcon->setPixmap(p);
        if ( !_pluginIcon->isVisible() ) {
            _pluginIcon->show();
            _pluginIconFrame->show();
        }
        double w, h;
        getSize(&w, &h);
        w = TO_DPIX(NODE_WIDTH) + TO_DPIX(NATRON_PLUGIN_ICON_SIZE) + TO_DPIX(PLUGIN_ICON_OFFSET) * 2;
        resize(w, h);

        double x, y;
        getPosition(&x, &y);
        x -= TO_DPIX(NATRON_PLUGIN_ICON_SIZE) / 2. + TO_DPIX(PLUGIN_ICON_OFFSET);
        setPosition(x, y);
    }
}


void
NodeGui::setOverlayLocked(bool locked)
{
    assert( QThread::currentThread() == qApp->thread() );
    _overlayLocked = locked;
}

void
NodeGui::onAvailableViewsChanged()
{
    NodePtr node = getNode();
    assert(node);
    if (!node) {
        return;
    }
    const std::vector<std::string>& views = node->getCreatedViews();

    if ( views.empty() || (views.size() == 1) ) {
        if ( _availableViewsIndicator->isActive() ) {
            _availableViewsIndicator->setActive(false);
        }

        return;
    }

    const std::vector<std::string>& projectViews = getDagGui()->getGui()->getApp()->getProject()->getProjectViewNames();
    QStringList qProjectViews;
    for (std::size_t i = 0; i < projectViews.size(); ++i) {
        qProjectViews.push_back( QString::fromUtf8( projectViews[i].c_str() ) );
    }

    QString toolTip;
    toolTip.append( tr("The following views are available in this node:") );
    toolTip.append( QString::fromUtf8(" ") );
    for (std::size_t i = 0; i < views.size(); ++i) {
        ///Try to find a match in the project views to have the same case sensitivity
        QString qView = QString::fromUtf8( views[i].c_str() );
        for (QStringList::Iterator it = qProjectViews.begin(); it != qProjectViews.end(); ++it) {
            if ( ( it->size() == qView.size() ) && it->startsWith(qView, Qt::CaseInsensitive) ) {
                qView = *it;
                break;
            }
        }
        toolTip.append(qView);
        if (i < views.size() - 1) {
            toolTip.push_back( QLatin1Char(',') );
        }
    }
    _availableViewsIndicator->setToolTip(toolTip);
    _availableViewsIndicator->setActive(true);
}

void
NodeGui::onIdentityStateChanged(int inputNb)
{
    if (!_passThroughIndicator) {
        return;
    }
    NodePtr ptInput;
    NodePtr node = getNode();
    assert(node);
    if (!node) {
        return;
    }
    bool disabled = node->isNodeDisabled();
    int firstFrame, lastFrame;
    bool lifetimeEnabled = node->isLifetimeActivated(&firstFrame, &lastFrame);
    int curFrame = node->getApp()->getTimeLine()->currentFrame();
    bool enabled = ( !lifetimeEnabled || (curFrame >= firstFrame && curFrame <= lastFrame) ) && !disabled;
    if (enabled) {
        if ( _disabledBtmLeftTopRight->isVisible() ) {
            _disabledBtmLeftTopRight->setVisible(false);
        }
        if ( _disabledTopLeftBtmRight->isVisible() ) {
            _disabledTopLeftBtmRight->setVisible(false);
        }
    } else {
        if ( !_disabledBtmLeftTopRight->isVisible() ) {
            _disabledBtmLeftTopRight->setVisible(true);
        }
        if ( !_disabledTopLeftBtmRight->isVisible() ) {
            _disabledTopLeftBtmRight->setVisible(true);
        }
    }

    if (inputNb >= 0) {
        ptInput = node->getInput(inputNb);
    }
    NodePtr prevIdentityInput = _identityInput.lock();
    if ( identityStateSet && ( ( ptInput && (ptInput == prevIdentityInput) ) ||
                               (!ptInput && !prevIdentityInput) ) ) {
        return;
    }

    identityStateSet = true;
    _identityInput = ptInput;


    _passThroughIndicator->setActive(ptInput.get() != 0);
    if (ptInput) {
        QString tooltip = tr("This node is a pass-through and produces the same results as %1.").arg( QString::fromUtf8( ptInput->getLabel().c_str() ) );
        _passThroughIndicator->setToolTip(tooltip);
        for (std::size_t i = 0; i < _inputEdges.size(); ++i) {
            if ( (int)i != inputNb ) {
                _inputEdges[i]->setDashed(true);
            } else {
                _inputEdges[i]->setDashed(false);
            }
        }
    } else {
        for (std::size_t i = 0; i < _inputEdges.size(); ++i) {
            _inputEdges[i]->setDashed( node->getEffectInstance()->isInputMask(i) );
        }
    }
    getDagGui()->update();
} // NodeGui::onIdentityStateChanged

void
NodeGui::onHideInputsKnobValueChanged(bool /*hidden*/)
{
    refreshEdgesVisility();
}

class NodeUndoRedoCommand
    : public QUndoCommand
{
    UndoCommandPtr _command;

public:

    NodeUndoRedoCommand(const UndoCommandPtr& command)
        : QUndoCommand()
        , _command(command)
    {
        setText( QString::fromUtf8( command->getText().c_str() ) );
    }

    virtual ~NodeUndoRedoCommand()
    {
    }

    virtual void redo() OVERRIDE FINAL
    {
        _command->redo();
    }

    virtual void undo() OVERRIDE FINAL
    {
        _command->undo();
    }

    virtual int id() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return kNodeUndoChangeCommandCompressionID;
    }

    virtual bool mergeWith(const QUndoCommand* other) OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        const NodeUndoRedoCommand* o = dynamic_cast<const NodeUndoRedoCommand*>(other);

        if (!o) {
            return false;
        }

        return _command->mergeWith(o->_command);
    }
};

void
NodeGui::pushUndoCommand(const UndoCommandPtr& command)
{
    NodeSettingsPanel* panel = getSettingPanel();

    if (!panel) {
        command->redo();
    } else {
        panel->pushUndoCommand( new NodeUndoRedoCommand(command) );
    }
}

void
NodeGui::setCurrentCursor(CursorEnum defaultCursor)
{
    NodePtr node = getNode();

    if (!node) {
        return;
    }
    OverlaySupport* overlayInteract = node->getEffectInstance()->getCurrentViewportForOverlays();
    if (!overlayInteract) {
        return;
    }
    ViewerGL* isViewer = dynamic_cast<ViewerGL*>(overlayInteract);
    if (!isViewer) {
        return;
    }

    if (defaultCursor == eCursorDefault) {
        isViewer->unsetCursor();
    }
    Qt::CursorShape qtCursor;
    bool ok = QtEnumConvert::toQtCursor(defaultCursor, &qtCursor);
    if (!ok) {
        return;
    }
    isViewer->setCursor(qtCursor);
}

bool
NodeGui::setCurrentCursor(const QString& customCursorFilePath)
{
    NodePtr node = getNode();

    if (!node) {
        return false;
    }
    OverlaySupport* overlayInteract = node->getEffectInstance()->getCurrentViewportForOverlays();
    if (!overlayInteract) {
        return false;
    }
    ViewerGL* isViewer = dynamic_cast<ViewerGL*>(overlayInteract);
    if (!isViewer) {
        return false;
    }
    if ( customCursorFilePath.isEmpty() ) {
        return false;
    }

    QString cursorFilePath = customCursorFilePath;

    if ( !QFile::exists(customCursorFilePath) ) {
        QString resourcesPath = QString::fromUtf8( node->getPluginResourcesPath().c_str() );
        if ( !resourcesPath.endsWith( QLatin1Char('/') ) ) {
            resourcesPath += QLatin1Char('/');
        }
        cursorFilePath.prepend(resourcesPath);
    }

    if ( !QFile::exists(cursorFilePath) ) {
        return false;
    }

    QPixmap pix(cursorFilePath);
    if ( pix.isNull() ) {
        return false;
    }
    QCursor c(pix);
    isViewer->setCursor(c);

    return true;
}

class GroupKnobDialog
    : public NATRON_PYTHON_NAMESPACE::PyModalDialog
{
public:


    GroupKnobDialog(Gui* gui,
                    const KnobGroup* group);

    virtual ~GroupKnobDialog()
    {
    }
};

GroupKnobDialog::GroupKnobDialog(Gui* gui,
                                 const KnobGroup* group)
    : NATRON_PYTHON_NAMESPACE::PyModalDialog(gui, eStandardButtonNoButton)
{
    setWindowTitle( QString::fromUtf8( group->getLabel().c_str() ) );
    std::vector<KnobIPtr> children = group->getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        KnobIPtr duplicate = children[i]->createDuplicateOnHolder(getKnobsHolder(), KnobPagePtr(), KnobGroupPtr(), i, true, children[i]->getName(), children[i]->getLabel(), children[i]->getHintToolTip(), false, true);
        duplicate->setAddNewLine( children[i]->isNewLineActivated() );
    }

    refreshUserParamsGUI();
}

void
NodeGui::showGroupKnobAsDialog(KnobGroup* group)
{
    assert( QThread::currentThread() == qApp->thread() );
    assert(group);
    bool showDialog = group->getValue();
    if (showDialog) {
        _activeNodeCustomModalDialog = new GroupKnobDialog(getDagGui()->getGui(), group);
        _activeNodeCustomModalDialog->move( QCursor::pos() );
        int accepted = _activeNodeCustomModalDialog->exec();
        Q_UNUSED(accepted);
        // Notify dialog closed
        group->onValueChanged(false, ViewSpec::all(), 0, eValueChangedReasonUserEdited, 0);
        _activeNodeCustomModalDialog->deleteLater();
        _activeNodeCustomModalDialog = 0;
    } else if (_activeNodeCustomModalDialog) {
        _activeNodeCustomModalDialog->close();
        _activeNodeCustomModalDialog->deleteLater();
        _activeNodeCustomModalDialog = 0;
    }
}

void
NodeGui::onRightClickMenuKnobPopulated()
{
    NodePtr node = getNode();

    if (!node) {
        return;
    }
    OverlaySupport* overlayInteract = node->getEffectInstance()->getCurrentViewportForOverlays();
    if (!overlayInteract) {
        return;
    }
    ViewerGL* isViewer = dynamic_cast<ViewerGL*>(overlayInteract);
    if (!isViewer) {
        return;
    }

    KnobIPtr rightClickKnob = node->getKnobByName(kNatronOfxParamRightClickMenu);
    if (!rightClickKnob) {
        return;
    }
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>( rightClickKnob.get() );
    if (!isChoice) {
        return;
    }
    std::vector<ChoiceOption> entries = isChoice->getEntries_mt_safe();
    if ( entries.empty() ) {
        return;
    }

    Menu m(isViewer);
    for (std::vector<ChoiceOption>::iterator it = entries.begin(); it != entries.end(); ++it) {
        KnobIPtr knob = node->getKnobByName(it->id);
        if (!knob) {
            // Plug-in specified invalid knob name in the menu
            continue;
        }
        KnobButton* button = dynamic_cast<KnobButton*>( knob.get() );
        if (!button) {
            // Plug-in must only use buttons inside menu
            continue;
        }
        bool checkable = button->getIsCheckable();
        ActionWithShortcut* action = new ActionWithShortcut(node->getPlugin()->getPluginShortcutGroup().toStdString(),
                                                            button->getName(),
                                                            button->getLabel(),
                                                            &m);
        if (checkable) {
            action->setCheckable(true);
            action->setChecked( button->getValue() );
        }
        QObject::connect( action, SIGNAL(triggered()), this, SLOT(onRightClickActionTriggered()) );
        m.addAction(action);
    }

    m.exec( QCursor::pos() );
} // NodeGui::onRightClickMenuKnobPopulated

void
NodeGui::onRightClickActionTriggered()
{
    ActionWithShortcut* action = dynamic_cast<ActionWithShortcut*>( sender() );

    if (!action) {
        return;
    }
    const std::vector<std::pair<QString, QKeySequence> >& shortcuts = action->getShortcuts();
    assert( !shortcuts.empty() );
    std::string knobName = shortcuts.front().first.toStdString();
    NodePtr node = getNode();
    assert(node);
    if (!node) {
        return;
    }
    KnobIPtr knob = node->getKnobByName(knobName);
    if (!knob) {
        // Plug-in specified invalid knob name in the menu
        return;
    }
    KnobButton* button = dynamic_cast<KnobButton*>( knob.get() );
    if (!button) {
        // Plug-in must only use buttons inside menu
        return;
    }
    if ( button->getIsCheckable() ) {
        button->setValue( !button->getValue() );
    } else {
        button->trigger();
    }
}

void
NodeGui::onInputLabelChanged(int inputNb,const QString& label)
{
    if (inputNb < 0 || inputNb >= (int)_inputEdges.size()) {
        return;
    }
    assert(_inputEdges[inputNb]);
    if (_inputEdges[inputNb]) {
        _inputEdges[inputNb]->setLabel(label);
    }
}

void
NodeGui::onInputVisibilityChanged(int /*inputNb*/)
{
    refreshEdgesVisility();
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_NodeGui.cpp"
