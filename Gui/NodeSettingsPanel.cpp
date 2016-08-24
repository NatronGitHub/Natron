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

#include "NodeSettingsPanel.h"

#include <vector>
#include <list>
#include <string>
#include <exception>
#include <fstream>
#include <stdexcept>

#if QT_VERSION >= 0x050000
#include <QtWidgets/QStyle>
#else
#include <QtGui/QStyle>
#endif
#include <QGridLayout>
#include <QDialogButtonBox>

#include "Engine/EffectInstance.h"
#include "Engine/FStreamsSupport.h"
#include "Engine/Knob.h" // KnobHolder
#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/RotoLayer.h"
#include "Engine/Utils.h" // convertFromPlainText

#include "Gui/ActionShortcuts.h"
#include "Gui/Button.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/GuiDefines.h"
#include "Gui/Menu.h"
#include "Gui/Label.h"
#include "Gui/Button.h"
#include "Gui/LineEdit.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/QtEnumConvert.h"
#include "Gui/PreferencesPanel.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/TrackerPanel.h"
#include "Gui/RotoPanel.h"


using std::make_pair;
NATRON_NAMESPACE_ENTER;


NodeSettingsPanel::NodeSettingsPanel(const MultiInstancePanelPtr & multiPanel,
                                     Gui* gui,
                                     const NodeGuiPtr &NodeUi,
                                     QVBoxLayout* container,
                                     QWidget *parent)
    : DockablePanel(gui,
                    multiPanel ? boost::dynamic_pointer_cast<KnobHolder>( multiPanel ) : NodeUi->getNode()->getEffectInstance(),
                    container,
                    DockablePanel::eHeaderModeFullyFeatured,
                    false,
                    NodeUi->getUndoStack(),
                    QString::fromUtf8( NodeUi->getNode()->getLabel().c_str() ),
                    QString::fromUtf8( NodeUi->getNode()->getPluginDescription().c_str() ),
                    parent)
    , _nodeGUI(NodeUi)
    , _selected(false)
    , _settingsButton(0)
    , _multiPanel(multiPanel)
{
    if (multiPanel) {
        multiPanel->initializeKnobsPublic();
    }


    QObject::connect( this, SIGNAL(closeChanged(bool)), NodeUi.get(), SLOT(onSettingsPanelClosedChanged(bool)) );
    const QSize mediumBSize( TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE) );
    const QSize mediumIconSize( TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE) );
    QPixmap pixSettings;
    appPTR->getIcon(NATRON_PIXMAP_SETTINGS, TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), &pixSettings);
    _settingsButton = new Button( QIcon(pixSettings), QString(), getHeaderWidget() );
    _settingsButton->setFixedSize(mediumBSize);
    _settingsButton->setIconSize(mediumIconSize);
    _settingsButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Settings and presets."), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _settingsButton->setFocusPolicy(Qt::NoFocus);
    QObject::connect( _settingsButton, SIGNAL(clicked()), this, SLOT(onSettingsButtonClicked()) );
    insertHeaderWidget(1, _settingsButton);
}

NodeSettingsPanel::~NodeSettingsPanel()
{
    NodeGuiPtr node = getNode();

    if (node) {
        node->removeSettingsPanel();
    }
}

void
NodeSettingsPanel::setSelected(bool s)
{
    if (s != _selected) {
        _selected = s;
        style()->unpolish(this);
        style()->polish(this);
    }
}

void
NodeSettingsPanel::centerOnItem()
{
    getNode()->centerGraphOnIt();
}

RotoPanel*
NodeSettingsPanel::initializeRotoPanel()
{
    if ( getNode()->getNode()->isRotoPaintingNode() ) {
        return new RotoPanel(_nodeGUI.lock(), this);
    } else {
        return NULL;
    }
}

TrackerPanel*
NodeSettingsPanel::initializeTrackerPanel()
{
    if ( getNode()->getNode()->getEffectInstance()->isBuiltinTrackerNode() ) {
        return new TrackerPanel(_nodeGUI.lock(), this);
    } else {
        return NULL;
    }
}

QColor
NodeSettingsPanel::getCurrentColor() const
{
    return getNode()->getCurrentColor();
}

void
NodeSettingsPanel::initializeExtraGui(QVBoxLayout* layout)
{
    if ( _multiPanel && !_multiPanel->isGuiCreated() ) {
        _multiPanel->createMultiInstanceGui(layout);
    }
}

void
NodeSettingsPanel::onSettingsButtonClicked()
{
    Menu menu(this);
    //menu.setFont(QFont(appFont,appFontSize));
    NodeGuiPtr node = getNode();
    NodePtr master = node->getNode()->getMasterNode();
    QAction* importPresets = new QAction(tr("Import presets"), &menu);
    QObject::connect( importPresets, SIGNAL(triggered()), this, SLOT(onImportPresetsActionTriggered()) );
    QAction* exportAsPresets = new QAction(tr("Export as presets"), &menu);
    QObject::connect( exportAsPresets, SIGNAL(triggered()), this, SLOT(onExportPresetsActionTriggered()) );

    menu.addAction(importPresets);
    menu.addAction(exportAsPresets);
    menu.addSeparator();

    QAction* manageUserParams = new QAction(tr("Manage user parameters..."), &menu);
    QObject::connect( manageUserParams, SIGNAL(triggered()), this, SLOT(onManageUserParametersActionTriggered()) );
    menu.addAction(manageUserParams);

    menu.addSeparator();


    QAction* setKeyOnAll = new QAction(tr("Set key on all parameters"), &menu);
    QObject::connect( setKeyOnAll, SIGNAL(triggered()), this, SLOT(setKeyOnAllParameters()) );
    QAction* removeAnimationOnAll = new QAction(tr("Remove animation on all parameters"), &menu);
    QObject::connect( removeAnimationOnAll, SIGNAL(triggered()), this, SLOT(removeAnimationOnAllParameters()) );
    menu.addAction(setKeyOnAll);
    menu.addAction(removeAnimationOnAll);

    if ( master || !node->getDagGui() || !node->getDagGui()->getGui() || node->getDagGui()->getGui()->isGUIFrozen() ) {
        importPresets->setEnabled(false);
        exportAsPresets->setEnabled(false);
        setKeyOnAll->setEnabled(false);
        removeAnimationOnAll->setEnabled(false);
    }

    menu.exec( _settingsButton->mapToGlobal( _settingsButton->pos() ) );
}

void
NodeSettingsPanel::onImportPresetsActionTriggered()
{
    std::vector<std::string> filters;

    filters.push_back(NATRON_PRESETS_FILE_EXT);
    std::string filename = getGui()->popOpenFileDialog(false, filters, getGui()->getLastLoadProjectDirectory().toStdString(), false);
    if ( filename.empty() ) {
        return;
    }


    FStreamsSupport::ifstream ifile;
    FStreamsSupport::open(&ifile, filename);
    if (!ifile) {
        Dialogs::errorDialog( tr("Presets").toStdString(), tr("Failed to open file: ").toStdString() + filename, false );

        return;
    }

    std::list<NodeSerializationPtr > nodeSerialization;
    try {
        int nNodes;
        boost::archive::xml_iarchive iArchive(ifile);
        iArchive >> boost::serialization::make_nvp("NodesCount", nNodes);
        for (int i = 0; i < nNodes; ++i) {
            NodeSerializationPtr node( new NodeSerialization() );
            iArchive >> boost::serialization::make_nvp("Node", *node);
            nodeSerialization.push_back(node);
        }
    } catch (const std::exception & e) {
        Dialogs::errorDialog( "Presets", e.what() );

        return;
    }

    NodeGuiPtr node = getNode();
    if ( nodeSerialization.front()->getPluginID() != node->getNode()->getPluginID() ) {
        QString err = tr("You cannot load %1 which are presets for the plug-in %2 on the plug-in %3.")
                      .arg( QString::fromUtf8( filename.c_str() ) )
                      .arg( QString::fromUtf8( nodeSerialization.front()->getPluginID().c_str() ) )
                      .arg( QString::fromUtf8( node->getNode()->getPluginID().c_str() ) );
        Dialogs::errorDialog( tr("Presets").toStdString(), err.toStdString() );

        return;
    }

    node->restoreInternal(node, nodeSerialization);
}

/*static bool
endsWith(const std::string &str,
         const std::string &suffix)
{
    return ( ( str.size() >= suffix.size() ) &&
             (str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0) );
}*/


SavePresetsDialog::SavePresetsDialog(Gui* gui, QWidget* parent)
: QDialog(parent)
, _gui(gui)
{
    mainLayout = new QGridLayout(this);
    
    QWidget* row1 = new QWidget(this);
    QHBoxLayout* row1Layout = new QHBoxLayout(row1);
    row1Layout->setContentsMargins(0, 0, 0, 0);
    QWidget* row2 = new QWidget(this);
    QHBoxLayout* row2Layout = new QHBoxLayout(row2);
    row2Layout->setContentsMargins(0, 0, 0, 0);
    QWidget* row3 = new QWidget(this);
    QHBoxLayout* row3Layout = new QHBoxLayout(row3);
    row3Layout->setContentsMargins(0, 0, 0, 0);
    QWidget* row4 = new QWidget(this);
    QHBoxLayout* row4Layout = new QHBoxLayout(row4);
    row4Layout->setContentsMargins(0, 0, 0, 0);
    
    presetNameLabel = new Label(tr("Preset Name:"), row1);
    presetNameEdit = new LineEdit(row1);
    row1Layout->addWidget(presetNameEdit);
    
    presetIconLabel = new Label(tr("Preset Icon File:"), row2);
    presetIconEdit = new LineEdit(row2);
    presetIconEdit->setPlaceholderText(tr("Icon file without path"));
    row2Layout->addWidget(presetIconEdit);
    
    presetShortcutKeyLabel = new Label(tr("Shortcut:"), row3);
    presetShortcutKeyEditor = new KeybindRecorder(row3);
    row3Layout->addWidget(presetShortcutKeyEditor);
    
    filePathLabel = new Label(tr("Preset File:"), row4);
    filePathEdit = new LineEdit(row4);
    QPixmap openPix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, NATRON_MEDIUM_BUTTON_ICON_SIZE, &openPix);
    filePathOpenButton = new Button(QIcon(openPix), QString(), row4);
    QObject::connect( filePathOpenButton, SIGNAL(clicked(bool)), this, SLOT(onOpenFileButtonClicked()) );
    row4Layout->addWidget(filePathEdit);
    row4Layout->addWidget(filePathOpenButton);
    
    mainLayout->addWidget(presetNameLabel, 0, 0);
    mainLayout->addWidget(row1, 0, 1);
    
    mainLayout->addWidget(presetIconLabel, 1, 0);
    mainLayout->addWidget(row2, 1, 1);
    
    mainLayout->addWidget(presetShortcutKeyLabel, 2, 0);
    mainLayout->addWidget(row3, 2, 1);
    
    mainLayout->addWidget(filePathLabel, 3, 0);
    mainLayout->addWidget(row4, 3, 1);
    
    buttonBox = new QDialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
                                     Qt::Horizontal, this);
    QObject::connect( buttonBox, SIGNAL(accepted()), this, SLOT(accept()) );
    QObject::connect( buttonBox, SIGNAL(rejected()), this, SLOT(reject()) );
    
    mainLayout->addWidget(buttonBox, 4, 0, 1, 2, Qt::AlignRight | Qt::AlignVCenter);
}

void
SavePresetsDialog::onOpenFileButtonClicked()
{
    std::vector<std::string> filters;
    const QString& path = _gui->getLastPluginDirectory();
    SequenceFileDialog dialog(this, filters, false, SequenceFileDialog::eFileDialogModeDir, path.toStdString(), _gui, false);
    
    if ( dialog.exec() ) {
        std::string selection = dialog.selectedFiles();
        filePathEdit->setText( QString::fromUtf8( selection.c_str() ) );
        QDir d = dialog.currentDirectory();
        _gui->updateLastPluginDirectory( d.absolutePath() );
    }
}

QString
SavePresetsDialog::getPresetName() const
{
    return presetNameEdit->text();
}

QString
SavePresetsDialog::getPresetIconFile() const
{
    return presetIconEdit->text();
}

QString
SavePresetsDialog::getPresetShortcut()
{
    return presetShortcutKeyEditor->text();
}

QString
SavePresetsDialog::getPresetPath() const
{
    return filePathEdit->text();
}


void
NodeSettingsPanel::onExportPresetsActionTriggered()
{
    
    SavePresetsDialog dialog(getGui());
    if (!dialog.exec()) {
        return;
    }
    
    QString presetName = dialog.getPresetName();
    QString presetIconFile = dialog.getPresetIconFile();
    QString presetShortcut = dialog.getPresetShortcut();
    QString presetPath = dialog.getPresetPath();
    
    QString presetFilePath = presetPath;
    if (!presetFilePath.endsWith(QChar::fromAscii('/'))) {
        presetFilePath += QChar::fromAscii('/');
    }
    presetFilePath += presetName;
    presetFilePath += QChar::fromAscii('.');
    presetFilePath += QLatin1String(NATRON_PRESETS_FILE_EXT);
    
    Qt::KeyboardModifiers qtMods;
    Qt::Key qtKey;
    
    QKeySequence keySeq(presetShortcut, QKeySequence::NativeText);
    extractKeySequence(keySeq, qtMods, qtKey);
    
    try {
        getNode()->getNode()->saveNodeToPresets(presetFilePath.toStdString(),
                                                presetName.toStdString(),
                                                presetIconFile.toStdString(),
                                                QtEnumConvert::fromQtKey(qtKey),
                                                QtEnumConvert::fromQtModifiers(qtMods));
    } catch (const std::exception &e) {
        Dialogs::errorDialog( tr("Export Presets").toStdString(), e.what(), false );
    }

    
    

}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_NodeSettingsPanel.cpp"
