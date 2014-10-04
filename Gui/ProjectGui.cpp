//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */
#include "ProjectGui.h"

#include <fstream>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QDebug>
#include <QTextDocument> // for Qt::convertFromPlainText
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Project.h"
#include "Engine/ViewerInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/Settings.h"

#include "Gui/GuiApplicationManager.h"
#include "Gui/Gui.h"
#include "Gui/ComboBox.h"
#include "Gui/Button.h"
#include "Gui/LineEdit.h"
#include "Gui/SpinBox.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"
#include "Gui/DockablePanel.h"
#include "Gui/NodeGui.h"
#include "Gui/TabWidget.h"
#include "Gui/ProjectGuiSerialization.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/NodeGraph.h"
#include "Gui/Splitter.h"
#include "Gui/Histogram.h"
#include "Gui/NodeBackDrop.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/CurveEditor.h"

ProjectGui::ProjectGui(Gui* gui)
    : _gui(gui)
      , _project()
      , _panel(NULL)
      , _created(false)
      , _colorPickersEnabled()
{
}

ProjectGui::~ProjectGui()
{
    const std::vector<boost::shared_ptr<Natron::Node> > & nodes = _project->getCurrentNodes();

    for (U32 i = 0; i < nodes.size(); ++i) {
        nodes[i]->quitAnyProcessing();
    }
}

void
ProjectGui::initializeKnobsGui()
{
    assert(_panel);
    _panel->initializeKnobs();
}

void
ProjectGui::create(boost::shared_ptr<Natron::Project> projectInternal,
                   QVBoxLayout* container,
                   QWidget* parent)

{
    _project = projectInternal;

    QObject::connect( projectInternal.get(),SIGNAL( mustCreateFormat() ),this,SLOT( createNewFormat() ) );
    QObject::connect( projectInternal.get(),SIGNAL( knobsInitialized() ),this,SLOT( initializeKnobsGui() ) );

    _panel = new DockablePanel(_gui,
                               projectInternal.get(),
                               container,
                               DockablePanel::READ_ONLY_NAME,
                               false,
                               tr("Project Settings"),
                               tr("The settings of the current project."),
                               false,
                               tr("Settings"),
                               parent);


    _created = true;
}

bool
ProjectGui::isVisible() const
{
    return _panel->isVisible();
}

void
ProjectGui::setVisible(bool visible)
{
    _panel->setVisible(visible);
}

void
ProjectGui::createNewFormat()
{
    AddFormatDialog dialog( _project.get(),_gui->getApp()->getGui() );

    if ( dialog.exec() ) {
        _project->setOrAddProjectFormat( dialog.getFormat() );
    }
}

AddFormatDialog::AddFormatDialog(Natron::Project *project,
                                 Gui* gui)
    : QDialog(gui),
      _gui(gui),
      _project(project)
{
    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setSpacing(0);
    _mainLayout->setContentsMargins(5, 5, 0, 0);
    setLayout(_mainLayout);
    setWindowTitle( tr("New Format") );

    _fromViewerLine = new QWidget(this);
    _fromViewerLineLayout = new QHBoxLayout(_fromViewerLine);
    _fromViewerLine->setLayout(_fromViewerLineLayout);

    _copyFromViewerCombo = new ComboBox(_fromViewerLine);
    const std::vector<boost::shared_ptr<Natron::Node> > & nodes = project->getCurrentNodes();

    for (U32 i = 0; i < nodes.size(); ++i) {
        if (nodes[i]->getPluginID() == "Viewer") {
            _copyFromViewerCombo->addItem( nodes[i]->getName().c_str() );
        }
    }
    _fromViewerLineLayout->addWidget(_copyFromViewerCombo);

    _copyFromViewerButton = new Button(tr("Copy from"),_fromViewerLine);
    _copyFromViewerButton->setToolTip( Qt::convertFromPlainText(
                                           tr("Fill the new format with the currently"
                                              " displayed region of definition of the viewer"
                                              " indicated on the left."), Qt::WhiteSpaceNormal) );
    QObject::connect( _copyFromViewerButton,SIGNAL( clicked() ),this,SLOT( onCopyFromViewer() ) );
    _mainLayout->addWidget(_fromViewerLine);

    _fromViewerLineLayout->addWidget(_copyFromViewerButton);
    _parametersLine = new QWidget(this);
    _parametersLineLayout = new QHBoxLayout(_parametersLine);
    _mainLayout->addWidget(_parametersLine);

    _widthLabel = new QLabel("w:",_parametersLine);
    _parametersLineLayout->addWidget(_widthLabel);
    _widthSpinBox = new SpinBox(this,SpinBox::INT_SPINBOX);
    _widthSpinBox->setMaximum(99999);
    _widthSpinBox->setMinimum(1);
    _widthSpinBox->setValue(1);
    _parametersLineLayout->addWidget(_widthSpinBox);


    _heightLabel = new QLabel("h:",_parametersLine);
    _parametersLineLayout->addWidget(_heightLabel);
    _heightSpinBox = new SpinBox(this,SpinBox::INT_SPINBOX);
    _heightSpinBox->setMaximum(99999);
    _heightSpinBox->setMinimum(1);
    _heightSpinBox->setValue(1);
    _parametersLineLayout->addWidget(_heightSpinBox);


    _pixelAspectLabel = new QLabel(tr("pixel aspect:"),_parametersLine);
    _parametersLineLayout->addWidget(_pixelAspectLabel);
    _pixelAspectSpinBox = new SpinBox(this,SpinBox::DOUBLE_SPINBOX);
    _pixelAspectSpinBox->setMinimum(0.);
    _pixelAspectSpinBox->setValue(1.);
    _parametersLineLayout->addWidget(_pixelAspectSpinBox);


    _formatNameLine = new QWidget(this);
    _formatNameLayout = new QHBoxLayout(_formatNameLine);
    _formatNameLine->setLayout(_formatNameLayout);
    _mainLayout->addWidget(_formatNameLine);


    _nameLabel = new QLabel(tr("Name:"),_formatNameLine);
    _formatNameLayout->addWidget(_nameLabel);
    _nameLineEdit = new LineEdit(_formatNameLine);
    _formatNameLayout->addWidget(_nameLineEdit);

    _buttonsLine = new QWidget(this);
    _buttonsLineLayout = new QHBoxLayout(_buttonsLine);
    _buttonsLine->setLayout(_buttonsLineLayout);
    _mainLayout->addWidget(_buttonsLine);


    _cancelButton = new Button(tr("Cancel"),_buttonsLine);
    QObject::connect( _cancelButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
    _buttonsLineLayout->addWidget(_cancelButton);

    _okButton = new Button(tr("Ok"),_buttonsLine);
    QObject::connect( _okButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
    _buttonsLineLayout->addWidget(_okButton);
}

void
AddFormatDialog::onCopyFromViewer()
{
    const std::vector<boost::shared_ptr<Natron::Node> > & nodes = _project->getCurrentNodes();
    QString activeText = _copyFromViewerCombo->itemText( _copyFromViewerCombo->activeIndex() );

    for (U32 i = 0; i < nodes.size(); ++i) {
        if ( nodes[i]->getName() == activeText.toStdString() ) {
            ViewerInstance* v = dynamic_cast<ViewerInstance*>( nodes[i]->getLiveInstance() );
            ViewerTab* tab = _gui->getViewerTabForInstance(v);
            RectD f = tab->getViewer()->getRoD(0);
            Format format = tab->getViewer()->getDisplayWindow();
            _widthSpinBox->setValue( f.width() );
            _heightSpinBox->setValue( f.height() );
            _pixelAspectSpinBox->setValue( format.getPixelAspect() );
        }
    }
}

Format
AddFormatDialog::getFormat() const
{
    int w = (int)_widthSpinBox->value();
    int h = (int)_heightSpinBox->value();
    double pa = _pixelAspectSpinBox->value();
    QString name = _nameLineEdit->text();

    return Format(0,0,w,h,name.toStdString(),pa);
}

void
ProjectGui::save(boost::archive::xml_oarchive & archive) const
{
    ProjectGuiSerialization projectGuiSerializationObj;

    projectGuiSerializationObj.initialize(this);
    archive << boost::serialization::make_nvp("ProjectGui",projectGuiSerializationObj);
}

void
ProjectGui::load(boost::archive::xml_iarchive & archive)
{
    ProjectGuiSerialization obj;

    archive >> boost::serialization::make_nvp("ProjectGui",obj);

    const std::map<std::string, ViewerData > & viewersProjections = obj.getViewersProjections();


    ///now restore the backdrops
    const std::list<NodeBackDropSerialization> & backdrops = obj.getBackdrops();
    for (std::list<NodeBackDropSerialization>::const_iterator it = backdrops.begin(); it != backdrops.end(); ++it) {
        NodeBackDrop* bd = _gui->createBackDrop(true,*it);

        if ( it->isSelected() ) {
            _gui->getNodeGraph()->selectBackDrop(bd, true);
        }
    }

    ///now restore backdrops slave/master links
    std::list<NodeBackDrop*> newBDs = _gui->getNodeGraph()->getBackDrops();
    for (std::list<NodeBackDrop*>::iterator it = newBDs.begin(); it != newBDs.end(); ++it) {
        ///find its serialization
        for (std::list<NodeBackDropSerialization>::const_iterator it2 = backdrops.begin(); it2 != backdrops.end(); ++it2) {
            if ( it2->getName() == (*it)->getName_mt_safe() ) {
                std::string masterName = it2->getMasterBackdropName();
                if ( !masterName.empty() ) {
                    ///search the master backdrop by name
                    for (std::list<NodeBackDrop*>::iterator it3 = newBDs.begin(); it3 != newBDs.end(); ++it3) {
                        if ( (*it3)->getName_mt_safe() == masterName ) {
                            (*it)->slaveTo(*it3);
                            break;
                        }
                    }
                }
                break;
            }
        }
    }


    ///default color for nodes
    float defR,defG,defB;
    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();
    const std::list<NodeGuiSerialization> & nodesGuiSerialization = obj.getSerializedNodesGui();
    for (std::list<NodeGuiSerialization>::const_iterator it = nodesGuiSerialization.begin(); it != nodesGuiSerialization.end(); ++it) {
        const std::string & name = it->getName();
        boost::shared_ptr<NodeGui> nGui = _gui->getApp()->getNodeGui(name);
        if (!nGui) {
            continue;
        }
        nGui->refreshPosition( it->getX(),it->getY(), true );

        if ( ( it->isPreviewEnabled() && !nGui->getNode()->isPreviewEnabled() ) ||
             ( !it->isPreviewEnabled() && nGui->getNode()->isPreviewEnabled() ) ) {
            nGui->togglePreview();
        }

        Natron::EffectInstance* iseffect = nGui->getNode()->getLiveInstance();

        if ( it->colorWasFound() ) {
            std::list<std::string> grouping;
            iseffect->getPluginGrouping(&grouping);
            std::string majGroup = grouping.empty() ? "" : grouping.front();

            if ( iseffect->isReader() ) {
                settings->getReaderColor(&defR, &defG, &defB);
            } else if ( iseffect->isWriter() ) {
                settings->getWriterColor(&defR, &defG, &defB);
            } else if ( iseffect->isGenerator() ) {
                settings->getGeneratorColor(&defR, &defG, &defB);
            } else if (majGroup == PLUGIN_GROUP_COLOR) {
                settings->getColorGroupColor(&defR, &defG, &defB);
            } else if (majGroup == PLUGIN_GROUP_FILTER) {
                settings->getFilterGroupColor(&defR, &defG, &defB);
            } else if (majGroup == PLUGIN_GROUP_CHANNEL) {
                settings->getChannelGroupColor(&defR, &defG, &defB);
            } else if (majGroup == PLUGIN_GROUP_KEYER) {
                settings->getKeyerGroupColor(&defR, &defG, &defB);
            } else if (majGroup == PLUGIN_GROUP_MERGE) {
                settings->getMergeGroupColor(&defR, &defG, &defB);
            } else if (majGroup == PLUGIN_GROUP_PAINT) {
                settings->getDrawGroupColor(&defR, &defG, &defB);
            } else if (majGroup == PLUGIN_GROUP_TIME) {
                settings->getTimeGroupColor(&defR, &defG, &defB);
            } else if (majGroup == PLUGIN_GROUP_TRANSFORM) {
                settings->getTransformGroupColor(&defR, &defG, &defB);
            } else if (majGroup == PLUGIN_GROUP_MULTIVIEW) {
                settings->getViewsGroupColor(&defR, &defG, &defB);
            } else if (majGroup == PLUGIN_GROUP_DEEP) {
                settings->getDeepGroupColor(&defR, &defG, &defB);
            } else {
                settings->getDefaultNodeColor(&defR, &defG, &defB);
            }


            float r,g,b;
            it->getColor(&r, &g, &b);
            ///restore color only if different from default.
            if ( (std::abs(r - defR) > 0.05) || (std::abs(g - defG) > 0.05) || (std::abs(b - defB) > 0.05) ) {
                QColor color;
                color.setRgbF(r, g, b);
                nGui->setDefaultGradientColor(color);
                nGui->setCurrentColor(color);
            }
        }

        ViewerInstance* viewer = dynamic_cast<ViewerInstance*>( nGui->getNode()->getLiveInstance() );
        if (viewer) {
            std::map<std::string, ViewerData >::const_iterator found = viewersProjections.find(name);
            if ( found != viewersProjections.end() ) {
                ViewerTab* tab = _gui->getApp()->getGui()->getViewerTabForInstance(viewer);
                tab->getViewer()->setProjection(found->second.zoomLeft, found->second.zoomBottom, found->second.zoomFactor, found->second.zoomPAR);
                tab->setChannels(found->second.channels);
                tab->setColorSpace(found->second.colorSpace);
                tab->setGain(found->second.gain);
                tab->setUserRoIEnabled(found->second.userRoIenabled);
                tab->setAutoContrastEnabled(found->second.autoContrastEnabled);
                tab->setUserRoI(found->second.userRoI);
                tab->setClipToProject(found->second.isClippedToProject);
                tab->setRenderScaleActivated(found->second.renderScaleActivated);
                tab->setMipMapLevel(found->second.mipMapLevel);
                tab->setCompositingOperator( (Natron::ViewerCompositingOperator)found->second.wipeCompositingOp );
                tab->setZoomOrPannedSinceLastFit(found->second.zoomOrPanSinceLastFit);
                tab->setFrameRangeLocked(found->second.frameRangeLocked);
                tab->setTopToolbarVisible(found->second.topToolbarVisible);
                tab->setLeftToolbarVisible(found->second.leftToolbarVisible);
                tab->setRightToolbarVisible(found->second.rightToolbarVisible);
                tab->setPlayerVisible(found->second.playerVisible);
                tab->setInfobarVisible(found->second.infobarVisible);
                tab->setTimelineVisible(found->second.timelineVisible);
                tab->setCheckerboardEnabled(found->second.checkerboardEnabled);
                tab->setDesiredFps(found->second.fps);
            }
        }

        if ( it->isSelected() ) {
            _gui->getNodeGraph()->selectNode(nGui, true);
        }
    }


    const std::list<boost::shared_ptr<NodeGui> > nodesGui = getVisibleNodes();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodesGui.begin(); it != nodesGui.end(); ++it) {
        (*it)->refreshEdges();
        (*it)->refreshKnobLinks();
        ///restore the multi-instance panels now that all nodes are restored
        std::string parentName = (*it)->getNode()->getParentMultiInstanceName();

        if ( !parentName.empty() ) {
            boost::shared_ptr<NodeGui> parent;
            for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it2 = nodesGui.begin(); it2 != nodesGui.end(); ++it2) {
                if ( (*it2)->getNode()->getName() == parentName ) {
                    parent = *it2;
                    break;
                }
            }

            ///The parent must have been restored already.
            assert(parent);

            boost::shared_ptr<MultiInstancePanel> panel = parent->getMultiInstancePanel();
            ///the main instance must have a panel!
            assert(panel);
            panel->addRow( (*it)->getNode() );
        }
    }


    ///now restore opened settings panels
    const std::list<std::string> & openedPanels = obj.getOpenedPanels();
    //reverse the iterator to fill the layout bottom up
    for (std::list<std::string>::const_reverse_iterator it = openedPanels.rbegin(); it != openedPanels.rend(); ++it) {
        if (*it == kNatronProjectSettingsPanelSerializationName) {
            _gui->setVisibleProjectSettingsPanel();
        } else {
            for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it2 = nodesGui.begin(); it2 != nodesGui.end(); ++it2) {
                if ( (*it2)->getNode()->getName() == *it ) {
                    NodeSettingsPanel* panel = (*it2)->getSettingPanel();
                    if (panel) {
                        (*it2)->setVisibleSettingsPanel(true);
                    }
                }
            }
        }
    }

    _gui->restoreLayout( true,obj.getVersion() < PROJECT_GUI_SERIALIZATION_MAJOR_OVERHAUL,obj.getGuiLayout() );

    ///restore the histograms
    const std::list<std::string> & histograms = obj.getHistograms();
    for (std::list<std::string>::const_iterator it = histograms.begin(); it != histograms.end(); ++it) {
        Histogram* h = _gui->addNewHistogram();
        h->setObjectName( (*it).c_str() );
        //move it by default to the viewer pane, before restoring the layout anyway which
        ///will relocate it correctly
        _gui->appendTabToDefaultViewerPane(h);
    }
} // load

std::list<boost::shared_ptr<NodeGui> > ProjectGui::getVisibleNodes() const
{
    return _gui->getVisibleNodes_mt_safe();
}

void
ProjectGui::registerNewColorPicker(boost::shared_ptr<Color_Knob> knob)
{
    for (std::vector<boost::shared_ptr<Color_Knob> >::iterator it = _colorPickersEnabled.begin(); it != _colorPickersEnabled.end(); ++it) {
        (*it)->setPickingEnabled(false);
    }
    _colorPickersEnabled.clear();
    _colorPickersEnabled.push_back(knob);
}

void
ProjectGui::removeColorPicker(boost::shared_ptr<Color_Knob> knob)
{
    std::vector<boost::shared_ptr<Color_Knob> >::iterator found = std::find(_colorPickersEnabled.begin(), _colorPickersEnabled.end(), knob);

    if ( found != _colorPickersEnabled.end() ) {
        _colorPickersEnabled.erase(found);
    }
}

void
ProjectGui::setPickersColor(const QColor & color)
{
    if ( _colorPickersEnabled.empty() ) {
        return;
    }
    boost::shared_ptr<Color_Knob> first = _colorPickersEnabled.front();

    for (U32 i = 0; i < _colorPickersEnabled.size(); ++i) {
        double r,g,b,a;
        r = color.redF();
        g = color.greenF();
        b = color.blueF();
        a = color.alphaF();
        if ( !_colorPickersEnabled[i]->areAllDimensionsEnabled() ) {
            _colorPickersEnabled[i]->activateAllDimensions();
        }
        if (_colorPickersEnabled[i]->getDimension() == 3) {
            _colorPickersEnabled[i]->setValues(r, g, b);
        } else {
            _colorPickersEnabled[i]->setValues(r, g, b,a);
        }
    }
}

