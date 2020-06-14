/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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
#include "Engine/PyParameter.h" // Param
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerNode.h"

#include "Gui/BackdropGui.h"
#include "Gui/Button.h"
#include "Gui/ComboBox.h"
#include "Gui/DialogButtonBox.h"
#include "Gui/DockablePanel.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Histogram.h"
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/NodeGui.h"
#include "Gui/PythonPanels.h"
#include "Gui/RegisteredTabs.h"
#include "Gui/ScriptEditor.h"
#include "Gui/SpinBox.h"
#include "Gui/Splitter.h"
#include "Gui/TabWidget.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"

#include "Serialization/ProjectSerialization.h"
#include "Serialization/NodeSerialization.h"
#include "Serialization/WorkspaceSerialization.h"
#include "Serialization/RectISerialization.h"
#include "Serialization/RectDSerialization.h"

NATRON_NAMESPACE_ENTER

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
                               QUndoStackPtr(),
                               tr("Project Settings"),
                               tr("The settings of the current project."),
                               parent);
    projectInternal->setPanelPointer(_panel);

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

    _copyFromViewerButton = new Button(tr("Copy From"), _fromViewerLine);
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

    _widthLabel = new Label(tr("w:"), _parametersLine);
    _parametersLineLayout->addWidget(_widthLabel);
    _widthSpinBox = new SpinBox(this, SpinBox::eSpinBoxTypeInt);
    _widthSpinBox->setMaximum(99999);
    _widthSpinBox->setMinimum(1);
    _widthSpinBox->setValue(1);
    _parametersLineLayout->addWidget(_widthSpinBox);


    _heightLabel = new Label(tr("h:"), _parametersLine);
    _parametersLineLayout->addWidget(_heightLabel);
    _heightSpinBox = new SpinBox(this, SpinBox::eSpinBoxTypeInt);
    _heightSpinBox->setMaximum(99999);
    _heightSpinBox->setMinimum(1);
    _heightSpinBox->setValue(1);
    _parametersLineLayout->addWidget(_heightSpinBox);


    _pixelAspectLabel = new Label(tr("Pixel aspect:"), _parametersLine);
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

    _buttonBox = new DialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel), Qt::Horizontal, this);
    QObject::connect( _buttonBox, SIGNAL(rejected()), this, SLOT(reject()) );
    QObject::connect( _buttonBox, SIGNAL(accepted()), this, SLOT(accept()) );
    _mainLayout->addWidget(_buttonBox);
}

void
AddFormatDialog::onCopyFromViewer()
{
    QString activeText = _copyFromViewerCombo->itemText( _copyFromViewerCombo->activeIndex() );

    for (std::list<ViewerInstancePtr>::iterator it = _viewers.begin(); it != _viewers.end(); ++it) {
        if ( (*it)->getNode()->getLabel() == activeText.toStdString() ) {
            ViewerTab* tab = _gui->getViewerTabForInstance((*it)->getViewerNodeGroup());
            RectD f = tab->getViewer()->getRoD(0);
            RectD canonicalFormat = tab->getViewer()->getCanonicalFormat(0);
            double par = tab->getViewer()->getPAR(0);
            RectI format;
            canonicalFormat.toPixelEnclosing(0, par, &format);
            _widthSpinBox->setValue( format.width() );
            _heightSpinBox->setValue( format.height() );
            _pixelAspectSpinBox->setValue(par);
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
loadNodeGuiSerialization(Gui* gui, const SERIALIZATION_NAMESPACE::NodeSerializationPtr& serialization)
{
    NodePtr internalNode;
    const std::string & groupName = serialization->_groupFullyQualifiedScriptName;
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
    internalNode = container->getNodeByName(serialization->_nodeScriptName);



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


    nGui->refreshPosition(serialization->_nodePositionCoords[0], serialization->_nodePositionCoords[1], true );

    EffectInstancePtr iseffect = nGui->getNode()->getEffectInstance();

    bool hasNodeColor = serialization->_nodeColor[0] != -1 || serialization->_nodeColor[1] != -1 || serialization->_nodeColor[2] != -1;

    BackdropGuiPtr isBd = toBackdropGui( nGui );

    if (hasNodeColor) {
        double defR, defG, defB;
        iseffect->getNode()->getDefaultColor(&defR, &defG, &defB);

        ///restore color only if different from default.
        if ( (std::abs(serialization->_nodeColor[0] - defR) > 0.05) || (std::abs(serialization->_nodeColor[1] - defG) > 0.05) || (std::abs(serialization->_nodeColor[2] - defB) > 0.05) ) {
            QColor color;
            color.setRgbF(Image::clamp(serialization->_nodeColor[0], 0., 1.),
                          Image::clamp(serialization->_nodeColor[1], 0., 1.),
                          Image::clamp(serialization->_nodeColor[2], 0., 1.));
            nGui->setCurrentColor(color);
        }

    }


    bool hasOverlayColor = serialization->_overlayColor[0] != -1. || serialization->_overlayColor[1] != -1. || serialization->_overlayColor[2] != -1.;

    if (hasOverlayColor) {
        QColor c;
        c.setRgbF(serialization->_overlayColor[0], serialization->_overlayColor[1], serialization->_overlayColor[2]);
        nGui->setOverlayColor(c);
    }

    if (isBd) {
        isBd->resize(serialization->_nodeSize[0], serialization->_nodeSize[1], true);
    }

    for (SERIALIZATION_NAMESPACE::NodeSerializationList::const_iterator it = serialization->_children.begin(); it != serialization->_children.end(); ++it) {
        loadNodeGuiSerialization(gui, *it);
    }

} // loadNodeGuiSerialization

void
ProjectGui::load(bool isAutosave,  const SERIALIZATION_NAMESPACE::ProjectSerializationPtr& serialization)
{

    for (std::list<SERIALIZATION_NAMESPACE::NodeSerializationPtr>::const_iterator it = serialization->_nodes.begin(); it != serialization->_nodes.end(); ++it) {
        loadNodeGuiSerialization(_gui, *it);
    }

    // we have to give the tr() context explicitly due to a bug in lupdate
    _gui->getApp()->updateProjectLoadStatus( QObject::tr("Restoring settings panels", "ProjectGui") );

    // Now restore opened settings panels
    const std::list<std::string> & openedPanels = serialization->_openedPanelsOrdered;
    //reverse the iterator to fill the layout bottom up
    bool hasProjectSettings = false;
    for (std::list<std::string>::const_reverse_iterator it = openedPanels.rbegin(); it != openedPanels.rend(); ++it) {
        if (*it == kNatronProjectSettingsPanelSerializationNameOld || *it == kNatronProjectSettingsPanelSerializationNameNew) {
            _gui->setVisibleProjectSettingsPanel();
            hasProjectSettings = true;
        } else {
            NodePtr node = getInternalProject()->getNodeByFullySpecifiedName(*it);
            if (node) {
                NodeGuiIPtr nodeGui_i = node->getNodeGui();
                assert(nodeGui_i);
                NodeGui* nodeGui = dynamic_cast<NodeGui*>( nodeGui_i.get() );
                assert(nodeGui);
                if (nodeGui) {
                    nodeGui->setVisibleSettingsPanel(true);
                    if (nodeGui->getSettingPanel()) {
                        _gui->putSettingsPanelFirst(nodeGui->getSettingPanel());
                    }
                }
            }
        }
    }
    if (!hasProjectSettings) {
        _panel->setClosed(true);
    }

    // we have to give the tr() context explicitly due to a bug in lupdate
    _gui->getApp()->updateProjectLoadStatus( QObject::tr("Restoring layout", "ProjectGui") );




    // For auto-saves, always load the workspace
    bool loadWorkspace = isAutosave || appPTR->getCurrentSettings()->getLoadProjectWorkspce();
    if (loadWorkspace && serialization->_projectWorkspace) {
        _gui->restoreLayout( true, false, *serialization->_projectWorkspace );
    }

    const RegisteredTabs& registeredTabs = getGui()->getRegisteredTabs();
    for (RegisteredTabs::const_iterator it = registeredTabs.begin(); it != registeredTabs.end(); ++it) {
        std::map<std::string,SERIALIZATION_NAMESPACE::ViewportData>::const_iterator found = serialization->_viewportsData.find(it->first);
        if (found != serialization->_viewportsData.end()) {
            it->second.first->loadProjection(found->second);
        } else {
            // If we could not find a projection for a viewer, reset to image fitted in the viewport
            ViewerTab* isViewer = dynamic_cast<ViewerTab*>(it->second.first);
            if (isViewer) {
                isViewer->getViewer()->fitImageToFormat();
            }
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
        PickerKnobSet::iterator it = _colorPickersEnabled.begin();
        it->knob->setPickingEnabled(it->view, false);
    }

    _colorPickersEnabled.clear();
}

void
ProjectGui::registerNewColorPicker(KnobColorPtr knob, ViewIdx view)
{
    clearColorPickers();

    PickerKnob p(knob,view);
    _colorPickersEnabled.insert(p);
}

void
ProjectGui::removeColorPicker(KnobColorPtr knob, ViewIdx view)
{
    PickerKnob p(knob,view);
    PickerKnobSet::iterator found = _colorPickersEnabled.find(p);

    if ( found != _colorPickersEnabled.end() ) {
        _colorPickersEnabled.erase(found);
    }
}

void
ProjectGui::setPickersColor(ViewIdx view,
                            double r,
                            double g,
                            double b,
                            double a)
{
    if ( _colorPickersEnabled.empty() ) {
        return;
    }
    KnobColorPtr first = _colorPickersEnabled.begin()->knob;



    for (PickerKnobSet::iterator it = _colorPickersEnabled.begin(); it != _colorPickersEnabled.end(); ++it) {
        if ( !it->knob->getAllDimensionsVisible(view) ) {
            it->knob->setAllDimensionsVisible(view, true);
        }
        if (it->knob->getNDimensions() == 3) {
            std::vector<double> values(3);
            values[0] = r;
            values[1] = g;
            values[2] = b;
            it->knob->setValueAcrossDimensions(values, DimIdx(0), view, eValueChangedReasonUserEdited);
        } else {
            std::vector<double> values(4);
            values[0] = r;
            values[1] = g;
            values[2] = b;
            values[3] = a;
            it->knob->setValueAcrossDimensions(values, DimIdx(0), view, eValueChangedReasonUserEdited);
        }
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_ProjectGui.cpp"
