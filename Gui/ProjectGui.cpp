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

#include "Global/AppManager.h"

#include "Engine/Project.h"
#include "Engine/ViewerInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/EffectInstance.h"
#include "Engine/VideoEngine.h"

#include "Gui/Gui.h"
#include "Gui/ComboBox.h"
#include "Gui/Button.h"
#include "Gui/LineEdit.h"
#include "Gui/SpinBox.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"
#include "Gui/DockablePanel.h"
#include "Gui/NodeGui.h"
#include "Gui/ProjectGuiSerialization.h"
#include "Gui/TabWidget.h"

ProjectGui::ProjectGui()
: _project()
, _panel(NULL)
, _created(false)
, _colorPickersEnabled()
{
    
}

ProjectGui::~ProjectGui(){
    const std::vector<Natron::Node*>& nodes = _project->getCurrentNodes();
    for (U32 i = 0; i < nodes.size(); ++i) {
        nodes[i]->quitAnyProcessing();
    }
}

void ProjectGui::create(boost::shared_ptr<Natron::Project> projectInternal,QVBoxLayout* container,QWidget* parent)

{
    _project = projectInternal;
    
    QObject::connect(projectInternal.get(),SIGNAL(mustCreateFormat()),this,SLOT(createNewFormat()));
    
    _panel = new DockablePanel(projectInternal.get(),
                               container,
                               DockablePanel::READ_ONLY_NAME,
                               "Project Settings",
                               "The settings of the current project.",
                               true,
                               "Rendering",
                               parent);
    _panel->initializeKnobs();
    
    
    _created = true;
}

bool ProjectGui::isVisible() const{
    return _panel->isVisible();
}

void ProjectGui::setVisible(bool visible){
    _panel->setVisible(visible);
}

void ProjectGui::createNewFormat(){
    AddFormatDialog dialog(_project.get(),_project->getApp()->getGui());
    if(dialog.exec()){
        _project->tryAddProjectFormat(dialog.getFormat());
    }
}



AddFormatDialog::AddFormatDialog(Natron::Project *project, QWidget* parent):QDialog(parent),
_project(project)
{
    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setSpacing(0);
    _mainLayout->setContentsMargins(5, 5, 0, 0);
    setLayout(_mainLayout);
    setWindowTitle("New Format");
    
    _fromViewerLine = new QWidget(this);
    _fromViewerLineLayout = new QHBoxLayout(_fromViewerLine);
    _fromViewerLine->setLayout(_fromViewerLineLayout);
    
    _copyFromViewerCombo = new ComboBox(_fromViewerLine);
    const std::vector<Natron::Node*>& nodes = project->getCurrentNodes();
    
    for(U32 i = 0 ; i < nodes.size(); ++i){
        if(nodes[i]->pluginID() == "Viewer"){
            _copyFromViewerCombo->addItem(nodes[i]->getName().c_str());
        }
    }
    _fromViewerLineLayout->addWidget(_copyFromViewerCombo);
    
    _copyFromViewerButton = new Button("Copy from",_fromViewerLine);
    _copyFromViewerButton->setToolTip(Qt::convertFromPlainText(
                                      "Fill the new format with the currently"
                                      " displayed region of definition of the viewer"
                                      " indicated on the left.", Qt::WhiteSpaceNormal));
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
    
    
    _pixelAspectLabel = new QLabel("pixel aspect:",_parametersLine);
    _parametersLineLayout->addWidget(_pixelAspectLabel);
    _pixelAspectSpinBox = new SpinBox(this,SpinBox::DOUBLE_SPINBOX);
    _pixelAspectSpinBox->setMinimum(0.);
    _pixelAspectSpinBox->setValue(1.);
    _parametersLineLayout->addWidget(_pixelAspectSpinBox);
    
    
    _formatNameLine = new QWidget(this);
    _formatNameLayout = new QHBoxLayout(_formatNameLine);
    _formatNameLine->setLayout(_formatNameLayout);
    _mainLayout->addWidget(_formatNameLine);
    
    
    _nameLabel = new QLabel("Name:",_formatNameLine);
    _formatNameLayout->addWidget(_nameLabel);
    _nameLineEdit = new LineEdit(_formatNameLine);
    _formatNameLayout->addWidget(_nameLineEdit);
    
    _buttonsLine = new QWidget(this);
    _buttonsLineLayout = new QHBoxLayout(_buttonsLine);
    _buttonsLine->setLayout(_buttonsLineLayout);
    _mainLayout->addWidget(_buttonsLine);
    
    
    _cancelButton = new Button("Cancel",_buttonsLine);
    QObject::connect(_cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    _buttonsLineLayout->addWidget(_cancelButton);
    
    _okButton = new Button("Ok",_buttonsLine);
    QObject::connect(_okButton, SIGNAL(clicked()), this, SLOT(accept()));
    _buttonsLineLayout->addWidget(_okButton);
    
}

void AddFormatDialog::onCopyFromViewer(){
    const std::vector<Natron::Node*>& nodes = _project->getCurrentNodes();
    
    QString activeText = _copyFromViewerCombo->itemText(_copyFromViewerCombo->activeIndex());
    for(U32 i = 0 ; i < nodes.size(); ++i){
        if(nodes[i]->getName() == activeText.toStdString()){
            ViewerInstance* v = dynamic_cast<ViewerInstance*>(nodes[i]->getLiveInstance());
            const RectI& f = v->getUiContext()->viewer->getCurrentViewerInfos().getRoD();
            const Format& format = v->getUiContext()->viewer->getCurrentViewerInfos().getDisplayWindow();
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



void ProjectGui::save(ProjectGuiSerialization* serializationObject) const{
    serializationObject->initialize(this);
}

void restoreTabWidgetLayoutRecursively(Gui* gui,const std::map<std::string,PaneLayout>& guiLayout,
                                       std::map<std::string,PaneLayout>::const_iterator layout){
    const std::list<TabWidget*>& initialWidgets = gui->getPanes();
    const std::map<std::string,QWidget*>& registeredTabs = gui->_registeredTabs;
    for(std::list<TabWidget*>::const_iterator it = initialWidgets.begin();it!=initialWidgets.end();++it){
        
        if((*it)->objectName().toStdString() == layout->first){
            //we found the pane, restore it!
            for (U32 i = 0; i < layout->second.splits.size(); ++i) {
                if(layout->second.splits[i]){
                    (*it)->splitVertically();
                }else{
                    (*it)->splitHorizontally();
                }
            }
            if(layout->second.floating){
                (*it)->floatPane();
                (*it)->move(layout->second.posx, layout->second.posy);
            }
            
            ///find all the tabs and move them to this widget
            for (U32 i = 0; i < layout->second.tabs.size(); ++i) {
                std::map<std::string,QWidget*>::const_iterator foundTab = registeredTabs.find(layout->second.tabs[i]);
                assert(foundTab != registeredTabs.end());
                TabWidget::moveTab(foundTab->second, *it);
            }
            
            ///now call this recursively on the freshly new splits
            for (U32 i = 0; i < layout->second.splitsNames.size(); ++i) {
                //find in the guiLayout map the PaneLayout corresponding to the split
                std::map<std::string,PaneLayout>::const_iterator splitIt = guiLayout.find(layout->second.splitsNames[i]);
                assert(splitIt != guiLayout.end());
                
                restoreTabWidgetLayoutRecursively(gui, guiLayout, splitIt);
            }
            
            break;
        }
    }
    
}

void ProjectGui::load(const ProjectGuiSerialization& obj){
    const std::vector< boost::shared_ptr<NodeGuiSerialization> >& nodesGuiSerialization = obj.getSerializedNodesGui();
    for (U32 i = 0; i < nodesGuiSerialization.size(); ++i) {
        const std::string& name = nodesGuiSerialization[i]->getName();
        NodeGui* nGui = _project->getApp()->getNodeGui(name);
        assert(nGui);
        nGui->setPos(nodesGuiSerialization[i]->getX(),nodesGuiSerialization[i]->getY());
        _project->getApp()->deselectAllNodes();
        if(nodesGuiSerialization[i]->isPreviewEnabled() && !nGui->getNode()->isPreviewEnabled()){
            nGui->togglePreview();
        }
        
    }
    const std::vector<NodeGui*> nodesGui = _project->getApp()->getVisibleNodes();
    for(U32 i = 0 ; i < nodesGui.size();++i){
        nodesGui[i]->refreshEdges();
    }

    ///now restore the gui layout
    
    const std::map<std::string,PaneLayout>& guiLayout = obj.getGuiLayout();
    for (std::map<std::string,PaneLayout>::const_iterator it = guiLayout.begin(); it!=guiLayout.end(); ++it) {
        
        ///if it is a top level tab (i.e: the original tabs)
        ///this will recursively restore all their splits
        if(it->second.parentName.empty()){
            restoreTabWidgetLayoutRecursively(_project->getApp()->getGui(), guiLayout, it);
        }
    }
    
    ///now restore the splitters
    const std::map<std::string,std::string>& splitters = obj.getSplittersStates();
    const std::list<QSplitter*>& appSplitters = _project->getApp()->getGui()->getSplitters();
    for (std::map<std::string,std::string>::const_iterator it = splitters.begin();it!=splitters.end();++it) {
        //find the splitter by name
        for (std::list<QSplitter*>::const_iterator it2 = appSplitters.begin(); it2!=appSplitters.end(); ++it2) {
            
            if ((*it2)->objectName().toStdString() == it->first) {
                //found a matching splitter, restore its state
                QByteArray splitterGeometry(it->second.c_str());
                splitterGeometry = QByteArray::fromBase64(splitterGeometry);
                if(!(*it2)->restoreState(splitterGeometry)){
                    qDebug() << "Failed to restore " << (*it2)->objectName() << "'s splitter state.";
                }
                break;
            }
        }
        
    }
}




void ProjectGui::registerNewColorPicker(boost::shared_ptr<Color_Knob> knob){
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
    first->beginValueChange(Natron::USER_EDITED);
    for(U32 i = 0; i < _colorPickersEnabled.size();++i){
        _colorPickersEnabled[i]->beginValueChange(Natron::USER_EDITED);
        _colorPickersEnabled[i]->setValue(color.redF(), 0);
        if(_colorPickersEnabled[i]->getDimension() >= 3){
            _colorPickersEnabled[i]->setValue(color.greenF(), 1);
            _colorPickersEnabled[i]->setValue(color.blueF(), 2);
        }
        if(_colorPickersEnabled[i]->getDimension() >= 4){
            _colorPickersEnabled[i]->setValue(color.alphaF(), 3);
        }
        _colorPickersEnabled[i]->endValueChange();
    }
    first->endValueChange();
}