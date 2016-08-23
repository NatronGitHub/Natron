/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "ProjectGui.h"

#include <fstream>
#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QtCore/QTimer>
#include <QtCore/QDebug>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)


#include "Engine/Backdrop.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Image.h"
#include "Engine/NodeSerialization.h"
#include "Engine/PyParameter.h" // Param
#include "Engine/Project.h"
#include "Engine/ProjectSerialization.h"
#include "Engine/Settings.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerNode.h"

#include "Gui/BackdropGui.h"
#include "Gui/Button.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveEditor.h"
#include "Gui/DockablePanel.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Histogram.h"
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/ProjectGuiSerialization.h"
#include "Gui/PythonPanels.h"
#include "Gui/RegisteredTabs.h"
#include "Gui/ScriptEditor.h"
#include "Gui/SpinBox.h"
#include "Gui/Splitter.h"
#include "Gui/TabWidget.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

//Remove when serialization is gone from this file
#include "Engine/RectISerialization.h"
#include "Engine/RectDSerialization.h"

NATRON_NAMESPACE_ENTER;

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
}

void
ProjectGui::initializeKnobsGui()
{
    assert(_panel);
    _panel->initializeKnobs();
}

void
ProjectGui::create(const ProjectPtr& projectInternal,
                   QVBoxLayout* container,
                   QWidget* parent)

{
    _project = projectInternal;

    QObject::connect( projectInternal.get(), SIGNAL(mustCreateFormat()), this, SLOT(createNewFormat()) );
    QObject::connect( projectInternal.get(), SIGNAL(knobsInitialized()), this, SLOT(initializeKnobsGui()) );

    _panel = new DockablePanel(_gui,
                               projectInternal,
                               container,
                               DockablePanel::eHeaderModeReadOnlyName,
                               false,
                               boost::shared_ptr<QUndoStack>(),
                               tr("Project Settings"),
                               tr("The settings of the current project."),
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
    _panel->setClosed(!visible);
}

void
ProjectGui::createNewFormat()
{
    ProjectPtr project = _project.lock();
    AddFormatDialog dialog( project.get(), _gui->getApp()->getGui() );

    if ( dialog.exec() ) {
        project->setOrAddProjectFormat( dialog.getFormat() );
    }
}

AddFormatDialog::AddFormatDialog(Project *project,
                                 Gui* gui)
    : QDialog(gui)
    , _gui(gui)
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

    project->getViewers(&_viewers);


    for (std::list<ViewerInstancePtr>::iterator it = _viewers.begin(); it != _viewers.end(); ++it) {
        _copyFromViewerCombo->addItem( QString::fromUtf8( (*it)->getNode()->getLabel().c_str() ) );
    }
    _fromViewerLineLayout->addWidget(_copyFromViewerCombo);

    _copyFromViewerButton = new Button(tr("Copy from"), _fromViewerLine);
    _copyFromViewerButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(
                                           tr("Fill the new format with the currently"
                                              " displayed region of definition of the viewer"
                                              " indicated on the left."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    QObject::connect( _copyFromViewerButton, SIGNAL(clicked()), this, SLOT(onCopyFromViewer()) );
    _mainLayout->addWidget(_fromViewerLine);

    _fromViewerLineLayout->addWidget(_copyFromViewerButton);
    _parametersLine = new QWidget(this);
    _parametersLineLayout = new QHBoxLayout(_parametersLine);
    _mainLayout->addWidget(_parametersLine);

    _widthLabel = new Label(QString::fromUtf8("w:"), _parametersLine);
    _parametersLineLayout->addWidget(_widthLabel);
    _widthSpinBox = new SpinBox(this, SpinBox::eSpinBoxTypeInt);
    _widthSpinBox->setMaximum(99999);
    _widthSpinBox->setMinimum(1);
    _widthSpinBox->setValue(1);
    _parametersLineLayout->addWidget(_widthSpinBox);


    _heightLabel = new Label(QString::fromUtf8("h:"), _parametersLine);
    _parametersLineLayout->addWidget(_heightLabel);
    _heightSpinBox = new SpinBox(this, SpinBox::eSpinBoxTypeInt);
    _heightSpinBox->setMaximum(99999);
    _heightSpinBox->setMinimum(1);
    _heightSpinBox->setValue(1);
    _parametersLineLayout->addWidget(_heightSpinBox);


    _pixelAspectLabel = new Label(tr("pixel aspect:"), _parametersLine);
    _parametersLineLayout->addWidget(_pixelAspectLabel);
    _pixelAspectSpinBox = new SpinBox(this, SpinBox::eSpinBoxTypeDouble);
    _pixelAspectSpinBox->setMinimum(0.);
    _pixelAspectSpinBox->setValue(1.);
    _parametersLineLayout->addWidget(_pixelAspectSpinBox);


    _formatNameLine = new QWidget(this);
    _formatNameLayout = new QHBoxLayout(_formatNameLine);
    _formatNameLine->setLayout(_formatNameLayout);
    _mainLayout->addWidget(_formatNameLine);


    _nameLabel = new Label(tr("Name:"), _formatNameLine);
    _formatNameLayout->addWidget(_nameLabel);
    _nameLineEdit = new LineEdit(_formatNameLine);
    _formatNameLayout->addWidget(_nameLineEdit);

    _buttonsLine = new QWidget(this);
    _buttonsLineLayout = new QHBoxLayout(_buttonsLine);
    _buttonsLine->setLayout(_buttonsLineLayout);
    _mainLayout->addWidget(_buttonsLine);


    _cancelButton = new Button(tr("Cancel"), _buttonsLine);
    QObject::connect( _cancelButton, SIGNAL(clicked()), this, SLOT(reject()) );
    _buttonsLineLayout->addWidget(_cancelButton);

    _okButton = new Button(tr("Ok"), _buttonsLine);
    QObject::connect( _okButton, SIGNAL(clicked()), this, SLOT(accept()) );
    _buttonsLineLayout->addWidget(_okButton);
}

void
AddFormatDialog::onCopyFromViewer()
{
    QString activeText = _copyFromViewerCombo->itemText( _copyFromViewerCombo->activeIndex() );

    for (std::list<ViewerInstancePtr>::iterator it = _viewers.begin(); it != _viewers.end(); ++it) {
        if ( (*it)->getNode()->getLabel() == activeText.toStdString() ) {
            ViewerTab* tab = _gui->getViewerTabForInstance((*it)->getViewerNodeGroup());
            RectD f = tab->getViewer()->getRoD(0);
            Format format = tab->getViewer()->getDisplayWindow();
            _widthSpinBox->setValue( f.width() );
            _heightSpinBox->setValue( f.height() );
            _pixelAspectSpinBox->setValue( format.getPixelAspectRatio() );
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

    return Format(0, 0, w, h, name.toStdString(), pa);
}


static
void
loadNodeGuiSerialization(Gui* gui, const NodeSerializationPtr& serialization)
{
    NodePtr internalNode;
    const std::string & groupName = serialization->getGroupFullyQualifiedName();
    NodeCollectionPtr container;
    if (groupName.empty()) {
        container = gui->getApp()->getProject();
    } else {
        NodePtr groupNode = gui->getApp()->getProject()->getNodeByFullySpecifiedName(groupName);
        if (!groupNode) {
            return;
        }
        NodeGroupPtr isGroupNode = toNodeGroup(groupNode->getEffectInstance());
        if (isGroupNode) {
            container = isGroupNode;
        } else {
            return;
        }
    }
    internalNode = container->getNodeByName(serialization->getNodeScriptName());



    if (!internalNode) {
        return;
    }

    NodeGuiPtr nGui;
    {
        NodeGuiIPtr nGui_i = internalNode->getNodeGui();
        assert(nGui_i);
        nGui = boost::dynamic_pointer_cast<NodeGui>(nGui_i);
    }
    if (!nGui) {
        return;
    }

    double x,y,w,h;
    serialization->getPosition(&x,&y);
    serialization->getSize(&w, &h);


    nGui->refreshPosition( x, y, true );

    EffectInstancePtr iseffect = nGui->getNode()->getEffectInstance();

    double nodeColor[3];
    serialization->getColor(&nodeColor[0], &nodeColor[1], &nodeColor[2]);
    bool hasNodeColor = nodeColor[0] != -1 || nodeColor[1] != -1 || nodeColor[2] != -1;

    BackdropGuiPtr isBd = toBackdropGui( nGui );

    if (hasNodeColor) {
        SettingsPtr settings = appPTR->getCurrentSettings();
        std::list<std::string> grouping;
        nGui->getNode()->getPluginGrouping(&grouping);
        std::string majGroup = grouping.empty() ? "" : grouping.front();
        float defR, defG, defB;

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
        } else if (isBd) {
            settings->getDefaultBackdropColor(&defR, &defG, &defB);
        } else {
            settings->getDefaultNodeColor(&defR, &defG, &defB);
        }


        ///restore color only if different from default.
        if ( (std::abs(nodeColor[0] - defR) > 0.05) || (std::abs(nodeColor[1] - defG) > 0.05) || (std::abs(nodeColor[2] - defB) > 0.05) ) {
            QColor color;
            color.setRgbF(Image::clamp(nodeColor[0], 0., 1.),
                          Image::clamp(nodeColor[1], 0., 1.),
                          Image::clamp(nodeColor[2], 0., 1.));
            nGui->setCurrentColor(color);
        }

    }


    double ovR, ovG, ovB;
    serialization->getOverlayColor(&ovR, &ovG, &ovB);
    bool hasOverlayColor = ovR != -1. || ovG != -1. || ovB != -1.;

    if (hasOverlayColor) {
        QColor c;
        c.setRgbF(ovR, ovG, ovB);
        nGui->setOverlayColor(c);
    }

    if (isBd) {
        isBd->resize(w, h, true);
    }



    bool isNodeSelected = serialization->getSelected();
    if (isNodeSelected) {
        gui->getNodeGraph()->selectNode(nGui, true);
    }

    const std::list<NodeSerializationPtr> & children = serialization->getNodesCollection();
    for (std::list<NodeSerializationPtr>::const_iterator it = children.begin(); it != children.end(); ++it) {
        loadNodeGuiSerialization(gui, *it);
    }

} // loadNodeGuiSerialization

template<>
void
ProjectGui::load<boost::archive::xml_iarchive>(bool isAutosave,  const ProjectSerializationPtr& serialization, const boost::shared_ptr<boost::archive::xml_iarchive> & archive)
{
    bool enableNatron1GuiCompat = false;
    if (serialization->getVersion() < PROJECT_SERIALIZATION_DEPRECATE_PROJECT_GUI) {


        ProjectGuiSerialization deprecatedGuiSerialization;
        (*archive) >> boost::serialization::make_nvp("ProjectGui", deprecatedGuiSerialization);

        enableNatron1GuiCompat = deprecatedGuiSerialization.getVersion() < PROJECT_GUI_SERIALIZATION_MAJOR_OVERHAUL;

        // Restore the old backdrops from old version prior to Natron 1.1
        const std::list<NodeBackdropSerialization> & backdrops = deprecatedGuiSerialization.getBackdrops();
        for (std::list<NodeBackdropSerialization>::const_iterator it = backdrops.begin(); it != backdrops.end(); ++it) {
            double x, y;
            it->getPos(x, y);
            int w, h;
            it->getSize(w, h);

            KnobSerializationPtr labelSerialization = it->getLabelSerialization();
            CreateNodeArgs args( PLUGINID_NATRON_BACKDROP, _project.lock() );
            args.setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
            args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
            args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);

            NodePtr node = getGui()->getApp()->createNode(args);
            NodeGuiIPtr gui_i = node->getNodeGui();
            assert(gui_i);
            BackdropGuiPtr bd = toBackdropGui( gui_i );
            assert(bd);
            if (bd) {
                bd->setPos(x, y);
                bd->resize(w, h);
                if (labelSerialization->_values[0]->_value.type == ValueSerializationStorage::eSerializationValueVariantTypeString) {
                    bd->onLabelChanged( QString::fromUtf8( labelSerialization->_values[0]->_value.value.isString.c_str() ) );
                }
                float r, g, b;
                it->getColor(r, g, b);
                QColor c;
                c.setRgbF(r, g, b);
                bd->setCurrentColor(c);
                node->setLabel( it->getFullySpecifiedName() );
            }
        }

        // Now convert what we can convert to our newer format...
        deprecatedGuiSerialization.convertToProjectSerialization(serialization.get());
    }

    for (std::list<NodeSerializationPtr>::const_iterator it = serialization->_nodes.begin(); it != serialization->_nodes.end(); ++it) {
        loadNodeGuiSerialization(_gui, *it);
    }

    /*const NodesGuiList nodesGui = getVisibleNodes();
    for (NodesGuiList::const_iterator it = nodesGui.begin(); it != nodesGui.end(); ++it) {
        (*it)->refreshEdges();
        (*it)->refreshKnobLinks();
    }*/


    _gui->getApp()->updateProjectLoadStatus( tr("Restoring settings panels") );

    // Now restore opened settings panels
    const std::list<std::string> & openedPanels = serialization->_openedPanelsOrdered;
    //reverse the iterator to fill the layout bottom up
    for (std::list<std::string>::const_reverse_iterator it = openedPanels.rbegin(); it != openedPanels.rend(); ++it) {
        if (*it == kNatronProjectSettingsPanelSerializationName) {
            _gui->setVisibleProjectSettingsPanel();
        } else {
            NodePtr node = getInternalProject()->getNodeByFullySpecifiedName(*it);
            if (node) {
                NodeGuiIPtr nodeGui_i = node->getNodeGui();
                assert(nodeGui_i);
                NodeGui* nodeGui = dynamic_cast<NodeGui*>( nodeGui_i.get() );
                assert(nodeGui);
                if (nodeGui) {
                    nodeGui->setVisibleSettingsPanel(true);
                }
            }
        }
    }

    _gui->getApp()->updateProjectLoadStatus( tr("Restoring layout") );


    

    // For auto-saves, always load the workspace
    bool loadWorkspace = isAutosave || appPTR->getCurrentSettings()->getLoadProjectWorkspce();
    if (loadWorkspace && serialization->_projectWorkspace) {
        _gui->restoreLayout( true, enableNatron1GuiCompat, *serialization->_projectWorkspace );
    }

    const RegisteredTabs& registeredTabs = getGui()->getRegisteredTabs();
    for (std::map<std::string,ViewportData>::const_iterator it = serialization->_viewportsData.begin(); it!=serialization->_viewportsData.end(); ++it) {
        RegisteredTabs::const_iterator found = registeredTabs.find(it->first);
        if (found != registeredTabs.end()) {
            found->second.first->loadProjection(it->second);
        }
    }

    _gui->centerAllNodeGraphsWithTimer();
} // load

NodesGuiList
ProjectGui::getVisibleNodes() const
{
    return _gui->getVisibleNodes_mt_safe();
}

void
ProjectGui::clearColorPickers()
{
    while ( !_colorPickersEnabled.empty() ) {
        _colorPickersEnabled.front()->setPickingEnabled(false);
    }

    _colorPickersEnabled.clear();
}

void
ProjectGui::registerNewColorPicker(KnobColorPtr knob)
{
    clearColorPickers();
    _colorPickersEnabled.push_back(knob);
}

void
ProjectGui::removeColorPicker(KnobColorPtr knob)
{
    std::vector<KnobColorPtr >::iterator found = std::find(_colorPickersEnabled.begin(), _colorPickersEnabled.end(), knob);

    if ( found != _colorPickersEnabled.end() ) {
        _colorPickersEnabled.erase(found);
    }
}

void
ProjectGui::setPickersColor(double r,
                            double g,
                            double b,
                            double a)
{
    if ( _colorPickersEnabled.empty() ) {
        return;
    }
    KnobColorPtr first = _colorPickersEnabled.front();

    for (U32 i = 0; i < _colorPickersEnabled.size(); ++i) {
        if ( !_colorPickersEnabled[i]->areAllDimensionsEnabled() ) {
            _colorPickersEnabled[i]->activateAllDimensions();
        }
        if (_colorPickersEnabled[i]->getDimension() == 3) {
            _colorPickersEnabled[i]->setValues(r, g, b, ViewSpec::all(), eValueChangedReasonNatronGuiEdited);
        } else {
            _colorPickersEnabled[i]->setValues(r, g, b, a, ViewSpec::all(), eValueChangedReasonNatronGuiEdited);
        }
    }
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_ProjectGui.cpp"
