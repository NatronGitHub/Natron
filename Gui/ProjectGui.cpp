//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */
#include "ProjectGui.h"

#include <fstream>



#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QDebug>
#include <QTextDocument> // for Qt::convertFromPlainText

#include "Engine/Project.h"
#include "Engine/ViewerInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/EffectInstance.h"
#include "Engine/VideoEngine.h"
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

ProjectGui::~ProjectGui(){
    const std::vector<boost::shared_ptr<Natron::Node> >& nodes = _project->getCurrentNodes();
    for (U32 i = 0; i < nodes.size(); ++i) {
        nodes[i]->quitAnyProcessing();
    }
}

void ProjectGui::initializeKnobsGui() {
    assert(_panel);
    _panel->initializeKnobs();
}

void ProjectGui::create(boost::shared_ptr<Natron::Project> projectInternal,QVBoxLayout* container,QWidget* parent)

{
    _project = projectInternal;
    
    QObject::connect(projectInternal.get(),SIGNAL(mustCreateFormat()),this,SLOT(createNewFormat()));
    QObject::connect(projectInternal.get(),SIGNAL(knobsInitialized()),this,SLOT(initializeKnobsGui()));
    
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

bool ProjectGui::isVisible() const{
    return _panel->isVisible();
}

void ProjectGui::setVisible(bool visible){
    _panel->setVisible(visible);
}

void ProjectGui::createNewFormat(){
    AddFormatDialog dialog(_project.get(),_gui->getApp()->getGui());
    if(dialog.exec()){
        _project->setOrAddProjectFormat(dialog.getFormat());
    }
}



AddFormatDialog::AddFormatDialog(Natron::Project *project,Gui* gui):QDialog(gui),
_gui(gui),
_project(project)
{
    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setSpacing(0);
    _mainLayout->setContentsMargins(5, 5, 0, 0);
    setLayout(_mainLayout);
    setWindowTitle(tr("New Format"));
    
    _fromViewerLine = new QWidget(this);
    _fromViewerLineLayout = new QHBoxLayout(_fromViewerLine);
    _fromViewerLine->setLayout(_fromViewerLineLayout);
    
    _copyFromViewerCombo = new ComboBox(_fromViewerLine);
    const std::vector<boost::shared_ptr<Natron::Node> >& nodes = project->getCurrentNodes();
    
    for(U32 i = 0 ; i < nodes.size(); ++i){
        if(nodes[i]->pluginID() == "Viewer"){
            _copyFromViewerCombo->addItem(nodes[i]->getName().c_str());
        }
    }
    _fromViewerLineLayout->addWidget(_copyFromViewerCombo);
    
    _copyFromViewerButton = new Button(tr("Copy from"),_fromViewerLine);
    _copyFromViewerButton->setToolTip(Qt::convertFromPlainText(
                                      tr("Fill the new format with the currently"
                                      " displayed region of definition of the viewer"
                                      " indicated on the left."), Qt::WhiteSpaceNormal));
    QObject::connect(_copyFromViewerButton,SIGNAL(clicked()),this,SLOT(onCopyFromViewer()));
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
    QObject::connect(_cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    _buttonsLineLayout->addWidget(_cancelButton);
    
    _okButton = new Button(tr("Ok"),_buttonsLine);
    QObject::connect(_okButton, SIGNAL(clicked()), this, SLOT(accept()));
    _buttonsLineLayout->addWidget(_okButton);
    
}

void AddFormatDialog::onCopyFromViewer(){
    const std::vector<boost::shared_ptr<Natron::Node> >& nodes = _project->getCurrentNodes();
    
    QString activeText = _copyFromViewerCombo->itemText(_copyFromViewerCombo->activeIndex());
    for(U32 i = 0 ; i < nodes.size(); ++i){
        if(nodes[i]->getName() == activeText.toStdString()){
            ViewerInstance* v = dynamic_cast<ViewerInstance*>(nodes[i]->getLiveInstance());
            ViewerTab* tab = _gui->getViewerTabForInstance(v);
            RectI f = tab->getViewer()->getRoD(0);
            Format format = tab->getViewer()->getDisplayWindow();
            _widthSpinBox->setValue(f.width());
            _heightSpinBox->setValue(f.height());
            _pixelAspectSpinBox->setValue(format.getPixelAspect());
        }
    }
}

Format AddFormatDialog::getFormat() const{
    int w = (int)_widthSpinBox->value();
    int h = (int)_heightSpinBox->value();
    double pa = _pixelAspectSpinBox->value();
    QString name = _nameLineEdit->text();
    return Format(0,0,w,h,name.toStdString(),pa);
}



void ProjectGui::save(boost::archive::xml_oarchive& archive) const {
    ProjectGuiSerialization projectGuiSerializationObj;
    projectGuiSerializationObj.initialize(this);
    archive << boost::serialization::make_nvp("ProjectGui",projectGuiSerializationObj);
}

void restoreTabWidgetLayoutRecursively(Gui* gui,const std::map<std::string,PaneLayout>& guiLayout,
                                       std::map<std::string,PaneLayout>::const_iterator layout,unsigned int projectVersion)
{
    const std::map<std::string,QWidget*>& registeredTabs = gui->getRegisteredTabs();
    const std::list<TabWidget*>& registeredPanes = gui->getPanes();
    
    QString serializedTabName(layout->first.c_str());
    ///for older projects before the layout change, map the old defaut tab names to new defaultLayout1 tab names
    if (projectVersion < PROJECT_GUI_CHANGES_SPLITTERS) {
        if (serializedTabName == "PropertiesPane") {
            serializedTabName = "ViewerPane" + TabWidget::splitHorizontallyTag + QString::number(0);
        } else if (serializedTabName == "WorkshopPane") {
            serializedTabName = "ViewerPane" + TabWidget::splitVerticallyTag + QString::number(0);
        }
    }
    
    TabWidget* pane = 0;
    for (std::list<TabWidget*>::const_iterator it = registeredPanes.begin(); it!=registeredPanes.end(); ++it) {
        if ((*it)->objectName() == serializedTabName) {
            ///For splits we should pass by here
            pane = *it;
        }
    }

    if (!pane) {
        pane = new TabWidget(gui,gui);
        gui->registerPane(pane);
        pane->setObjectName_mt_safe(serializedTabName);
    }
    
    
    
    //we found the pane, restore it!
    for (std::list<bool>::const_iterator it2 = layout->second.splits.begin();it2!=layout->second.splits.end();++it2) {
        if (*it2) {
            pane->splitVertically();
        } else {
            pane->splitHorizontally();
        }
    }
    if(layout->second.floating){
        pane->floatPane();
        pane->move(layout->second.posx, layout->second.posy);
    }
    
    ///find all the tabs and move them to this widget
    for (std::list<std::string>::const_iterator it2 = layout->second.tabs.begin();it2!=layout->second.tabs.end();++it2) {
        std::map<std::string,QWidget*>::const_iterator foundTab = registeredTabs.find(*it2);
        if (foundTab != registeredTabs.end()) {
            TabWidget::moveTab(foundTab->second,pane);
        } else if (*it2 == gui->getCurveEditor()->objectName().toStdString()) {
            TabWidget::moveTab(gui->getCurveEditor(),pane);
        } else if (*it2 == gui->getPropertiesScrollArea()->objectName().toStdString()) {
            TabWidget::moveTab(gui->getPropertiesScrollArea(), pane);
        } else if (*it2 == gui->getNodeGraph()->objectName().toStdString()) {
            TabWidget::moveTab(gui->getNodeGraph(), pane);
        }
    }
    
    pane->makeCurrentTab(layout->second.currentIndex);
    
    ///now call this recursively on the freshly new splits
    for (std::list<std::string>::const_iterator it2 = layout->second.splitsNames.begin();it2!=layout->second.splitsNames.end();++it2) {
        //find in the guiLayout map the PaneLayout corresponding to the split
        std::map<std::string,PaneLayout>::const_iterator splitIt = guiLayout.find(*it2);
        if (splitIt != guiLayout.end()) {
            
            restoreTabWidgetLayoutRecursively(gui, guiLayout, splitIt,projectVersion);
        }
    }
    
    
    
    
}

void ProjectGui::load(boost::archive::xml_iarchive& archive){
    
    ProjectGuiSerialization obj;
    archive >> boost::serialization::make_nvp("ProjectGui",obj);
 
    const std::map<std::string, ViewerData >& viewersProjections = obj.getViewersProjections();
    
    
    ///now restore the backdrops
    const std::list<NodeBackDropSerialization>& backdrops = obj.getBackdrops();
    for (std::list<NodeBackDropSerialization>::const_iterator it = backdrops.begin(); it != backdrops.end(); ++it) {
        NodeBackDrop* bd = _gui->createBackDrop(true,*it);
                
        if (it->isSelected()) {
            _gui->getNodeGraph()->selectBackDrop(bd, true);
        }
    }
    
    ///now restore backdrops slave/master links
    std::list<NodeBackDrop*> newBDs = _gui->getNodeGraph()->getBackDrops();
    for (std::list<NodeBackDrop*>::iterator it = newBDs.begin(); it!=newBDs.end(); ++it) {
        ///find its serialization
        for (std::list<NodeBackDropSerialization>::const_iterator it2 = backdrops.begin(); it2!=backdrops.end(); ++it2) {
            if (it2->getName() == (*it)->getName_mt_safe()) {
                
                std::string masterName = it2->getMasterBackdropName();
                if (!masterName.empty()) {
                    ///search the master backdrop by name
                    for (std::list<NodeBackDrop*>::iterator it3 = newBDs.begin(); it3!=newBDs.end(); ++it3) {
                        if ((*it3)->getName_mt_safe() == masterName) {
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
    const std::list<NodeGuiSerialization>& nodesGuiSerialization = obj.getSerializedNodesGui();
    for (std::list<NodeGuiSerialization>::const_iterator it = nodesGuiSerialization.begin();it!=nodesGuiSerialization.end();++it) {
        const std::string& name = it->getName();
        boost::shared_ptr<NodeGui> nGui = _gui->getApp()->getNodeGui(name);
        if (!nGui) {
            continue;
        }
        nGui->setPos(it->getX(),it->getY());
        
        if ((it->isPreviewEnabled() && !nGui->getNode()->isPreviewEnabled()) ||
           (!it->isPreviewEnabled() && nGui->getNode()->isPreviewEnabled())) {
            nGui->togglePreview();
        }
        
        Natron::EffectInstance* iseffect = nGui->getNode()->getLiveInstance();
        
        if (it->colorWasFound()) {
            std::list<std::string> grouping;
            iseffect->pluginGrouping(&grouping);
            std::string majGroup = grouping.empty() ? "" : grouping.front();
            
            if (iseffect->isReader()) {
                settings->getReaderColor(&defR, &defG, &defB);
            } else if (iseffect->isWriter()) {
                settings->getWriterColor(&defR, &defG, &defB);
            } else if (iseffect->isGenerator()) {
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
            if (std::abs(r - defR) > 0.05 || std::abs(g - defG) > 0.05 || std::abs(b - defB) > 0.05) {
                QColor color;
                color.setRgbF(r, g, b);
                nGui->setDefaultGradientColor(color);
                nGui->setCurrentColor(color);
            }
        }
        
        if (nGui->getNode()->pluginID() == "Viewer") {
            std::map<std::string, ViewerData >::const_iterator found = viewersProjections.find(name);
            if (found != viewersProjections.end()) {
                ViewerInstance* viewer = dynamic_cast<ViewerInstance*>(nGui->getNode()->getLiveInstance());
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
                tab->setCompositingOperator((Natron::ViewerCompositingOperator)found->second.wipeCompositingOp);
                tab->setZoomOrPannedSinceLastFit(found->second.zoomOrPanSinceLastFit);
                tab->setFrameRangeLocked(found->second.frameRangeLocked);
            }
        }
        
        if (it->isSelected()) {
            _gui->getNodeGraph()->selectNode(nGui, true);
        }
        
    }
    
    
    const std::list<boost::shared_ptr<NodeGui> > nodesGui = getVisibleNodes();
    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = nodesGui.begin();it!=nodesGui.end();++it) {
        (*it)->refreshEdges();
        (*it)->refreshSlaveMasterLinkPosition();
        
        ///restore the multi-instance panels now that all nodes are restored
        std::string parentName = (*it)->getNode()->getParentMultiInstanceName();
        
        if (!parentName.empty()) {
            boost::shared_ptr<NodeGui> parent;
            for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it2 = nodesGui.begin();it2!=nodesGui.end();++it2) {
                if ((*it2)->getNode()->getName() == parentName) {
                    parent = *it2;
                    break;
                }
            }
            
            ///The parent must have been restored already.
            assert(parent);
            
            boost::shared_ptr<MultiInstancePanel> panel = parent->getMultiInstancePanel();
            ///the main instance must have a panel!
            assert(panel);
            panel->addRow((*it)->getNode());
        }
    }
    
    ///Wipe the current layout
    _gui->wipeLayout();
    
    ///For older projects prior to the layout change, try to load panes
    if (obj.getVersion() < PROJECT_GUI_CHANGES_SPLITTERS) {
        _gui->createDefaultLayout1();
    }
    
   
    
    ///now restore the gui layout

    const std::map<std::string,PaneLayout>& guiLayout = obj.getGuiLayout();
    for (std::map<std::string,PaneLayout>::const_iterator it = guiLayout.begin(); it!=guiLayout.end(); ++it) {
        
        ///if it is a top level tab (i.e: the original tabs)
        ///this will recursively restore all their splits
        if(it->second.parentName.empty()){
            restoreTabWidgetLayoutRecursively(_gui->getApp()->getGui(), guiLayout, it,obj.getVersion());
        }
    }
    
    ///now restore the splitters
    const std::map<std::string,std::string>& splitters = obj.getSplittersStates();
    std::list<Splitter*> appSplitters = _gui->getApp()->getGui()->getSplitters();
    for (std::map<std::string,std::string>::const_iterator it = splitters.begin();it!=splitters.end();++it) {
        //find the splitter by name
        for (std::list<Splitter*>::const_iterator it2 = appSplitters.begin(); it2!=appSplitters.end(); ++it2) {
            
            if ((*it2)->objectName().toStdString() == it->first) {
                //found a matching splitter, restore its state
                QString splitterGeometry(it->second.c_str());
                if (!splitterGeometry.isEmpty()) {
                    (*it2)->restoreNatron(splitterGeometry);
                }
                break;
            }
        }
        
    }
    
    ///restore the histograms
    const std::list<std::string>& histograms = obj.getHistograms();
    for (std::list<std::string>::const_iterator it = histograms.begin();it!=histograms.end();++it) {
        Histogram* h = _gui->addNewHistogram();
        h->setObjectName((*it).c_str());
        //move it by default to the viewer pane, before restoring the layout anyway which
        ///will relocate it correctly
        _gui->appendTabToDefaultViewerPane(h);
    }
    
    
    ///now restore opened settings panels
    const std::list<std::string>& openedPanels = obj.getOpenedPanels();
    //reverse the iterator to fill the layout bottom up
    for (std::list<std::string>::const_reverse_iterator it = openedPanels.rbegin(); it!=openedPanels.rend(); ++it) {
        if (*it == "Natron_Project_Settings_Panel") {
            _gui->setVisibleProjectSettingsPanel();
        } else {
            for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it2 = nodesGui.begin(); it2 != nodesGui.end();++it2) {
                if ((*it2)->getNode()->getName() == *it) {
                    NodeSettingsPanel* panel = (*it2)->getSettingPanel();
                    if (panel) {
                        (*it2)->setVisibleSettingsPanel(true);
                        _gui->putSettingsPanelFirst(panel);
                    }
                }
            }
        }
    }
}

std::list<boost::shared_ptr<NodeGui> > ProjectGui::getVisibleNodes() const {
    return _gui->getVisibleNodes_mt_safe();
}


void ProjectGui::registerNewColorPicker(boost::shared_ptr<Color_Knob> knob) {
    for (std::vector<boost::shared_ptr<Color_Knob> >::iterator it = _colorPickersEnabled.begin();it!=_colorPickersEnabled.end();++it) {
        (*it)->setPickingEnabled(false);
    }
    _colorPickersEnabled.clear();
    _colorPickersEnabled.push_back(knob);
    
}

void ProjectGui::removeColorPicker(boost::shared_ptr<Color_Knob> knob){
    std::vector<boost::shared_ptr<Color_Knob> >::iterator found = std::find(_colorPickersEnabled.begin(), _colorPickersEnabled.end(), knob);
    if(found != _colorPickersEnabled.end()){
        _colorPickersEnabled.erase(found);
    }
}

void ProjectGui::setPickersColor(const QColor& color){
    if(_colorPickersEnabled.empty()){
        return;
    }
    boost::shared_ptr<Color_Knob> first = _colorPickersEnabled.front();

    for(U32 i = 0; i < _colorPickersEnabled.size();++i){
        double r,g,b,a;
        r = color.redF();
        g = color.greenF();
        b = color.blueF();
        a = color.alphaF();
        if (!_colorPickersEnabled[i]->areAllDimensionsEnabled()) {
            _colorPickersEnabled[i]->activateAllDimensions();
        }
        if (_colorPickersEnabled[i]->getDimension() == 3) {
            _colorPickersEnabled[i]->setValues(r, g, b);
        } else {
            _colorPickersEnabled[i]->setValues(r, g, b,a);
        }
    }
}
