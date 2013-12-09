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


#include "Global/AppManager.h"

#include "Engine/Project.h"
#include "Engine/ViewerInstance.h"

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

ProjectGui::ProjectGui()
: _project()
, _panel(NULL)
, _created(false)
{
    
}

void ProjectGui::create(boost::shared_ptr<Natron::Project> projectInternal,QVBoxLayout* container,QWidget* parent)

{
    _project = projectInternal;
    
    QObject::connect(projectInternal.get(),SIGNAL(mustCreateFormat()),this,SLOT(createNewFormat()));
    
    _panel = new DockablePanel(projectInternal.get(),
                               container,
                               true,
                               "Project Settings",
                               "The settings of the current project.",
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
    _copyFromViewerButton->setToolTip("Fill the new format with the currently"
                                      " displayed region of definition of the viewer"
                                      " indicated on the left.");
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


}
