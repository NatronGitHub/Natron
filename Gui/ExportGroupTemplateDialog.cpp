/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include "ExportGroupTemplateDialog.h"

#include <cassert>
#include <algorithm> // min, max
#include <boost/scoped_array.hpp>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QTextStream>
#include <QDialogButtonBox>
#include <QGridLayout>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/NodeGroup.h"
#include "Engine/Settings.h"

#include "Gui/Button.h"
#include "Gui/Gui.h"
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Utils.h" // convertFromPlainText
#include "Gui/GuiDefines.h"

struct ExportGroupTemplateDialogPrivate
{
    Gui* gui;
    NodeCollection* group;
    QGridLayout* mainLayout;

    Natron::Label* labelLabel;
    LineEdit* labelEdit;

    Natron::Label* idLabel;
    LineEdit* idEdit;

    Natron::Label* groupingLabel;
    LineEdit* groupingEdit;

    Natron::Label* fileLabel;
    LineEdit* fileEdit;
    Button* openButton;

    Natron::Label* iconPathLabel;
    LineEdit* iconPath;

    Natron::Label* descriptionLabel;
    LineEdit* descriptionEdit;

    QDialogButtonBox *buttons;

    ExportGroupTemplateDialogPrivate(NodeCollection* group,Gui* gui)
    : gui(gui)
    , group(group)
    , mainLayout(0)
    , labelLabel(0)
    , labelEdit(0)
    , idLabel(0)
    , idEdit(0)
    , groupingLabel(0)
    , groupingEdit(0)
    , fileLabel(0)
    , fileEdit(0)
    , openButton(0)
    , iconPathLabel(0)
    , iconPath(0)
    , descriptionLabel(0)
    , descriptionEdit(0)
    , buttons(0)
    {

    }
};

ExportGroupTemplateDialog::ExportGroupTemplateDialog(NodeCollection* group,Gui* gui,QWidget* parent)
: QDialog(parent)
, _imp(new ExportGroupTemplateDialogPrivate(group,gui))
{
    _imp->mainLayout = new QGridLayout(this);


    _imp->idLabel = new Natron::Label(tr("Unique ID"),this);
    QString idTt = Natron::convertFromPlainText(tr("The unique ID is used by " NATRON_APPLICATION_NAME "to identify the plug-in in various "
                                               "places in the application. Generally this contains domain and sub-domains names "
                                               "such as fr.inria.group.XXX. If 2 plug-ins happen to have the same ID they will be "
                                               "gathered by version. If 2 plug-ins have the same ID and version, the first loaded in the"
                                               " search-paths will take precedence over the other."), Qt::WhiteSpaceNormal);
    _imp->idEdit = new LineEdit(this);
    _imp->idEdit->setPlaceholderText("org.organization.pyplugs.XXX");
    _imp->idEdit->setToolTip(idTt);


    _imp->labelLabel = new Natron::Label(tr("Label"),this);
    QString labelTt = Natron::convertFromPlainText(tr("Set the label of the group as the user will see it in the user interface."), Qt::WhiteSpaceNormal);
    _imp->labelLabel->setToolTip(labelTt);
    _imp->labelEdit = new LineEdit(this);
    _imp->labelEdit->setPlaceholderText("MyPlugin");
    QObject::connect(_imp->labelEdit,SIGNAL(editingFinished()), this , SLOT(onLabelEditingFinished()));
    _imp->labelEdit->setToolTip(labelTt);


    _imp->groupingLabel = new Natron::Label(tr("Grouping"),this);
    QString groupingTt = Natron::convertFromPlainText(tr("The grouping of the plug-in specifies where the plug-in will be located in the menus. "
                                                     "E.g: Color/Transform, or Draw. Each sub-level must be separated by a '/'."), Qt::WhiteSpaceNormal);
    _imp->groupingLabel->setToolTip(groupingTt);

    _imp->groupingEdit = new LineEdit(this);
    _imp->groupingEdit->setPlaceholderText("Color/Transform");
    _imp->groupingEdit->setToolTip(groupingTt);


    _imp->iconPathLabel = new Natron::Label(tr("Icon relative path"),this);
    QString iconTt = Natron::convertFromPlainText(tr("Set here the file path of an optional icon to identify the plug-in. "
                                                 "The path is relative to the Python script."), Qt::WhiteSpaceNormal);
    _imp->iconPathLabel->setToolTip(iconTt);
    _imp->iconPath = new LineEdit(this);
    _imp->iconPath->setPlaceholderText("Label.png");
    _imp->iconPath->setToolTip(iconTt);

    _imp->descriptionLabel = new Natron::Label(tr("Description"),this);
    QString descTt =  Natron::convertFromPlainText(tr("Set here the (optional) plug-in description that the user will see when clicking the "
                                                  " \"?\" button on the settings panel of the node."), Qt::WhiteSpaceNormal);
    _imp->descriptionEdit = new LineEdit(this);
    _imp->descriptionEdit->setToolTip(descTt);
    _imp->descriptionEdit->setPlaceholderText(tr("This plug-in can be used to produce XXX effect..."));

    _imp->fileLabel = new Natron::Label(tr("Directory"),this);
    QString fileTt  = Natron::convertFromPlainText(tr("Specify here the directory where to export the Python script."), Qt::WhiteSpaceNormal);
    _imp->fileLabel->setToolTip(fileTt);
    _imp->fileEdit = new LineEdit(this);


    _imp->fileEdit->setToolTip(fileTt);


    QPixmap openPix;
    appPTR->getIcon(Natron::NATRON_PIXMAP_OPEN_FILE, NATRON_MEDIUM_BUTTON_ICON_SIZE, &openPix);
    _imp->openButton = new Button(QIcon(openPix),"",this);
    _imp->openButton->setFocusPolicy(Qt::NoFocus);
    _imp->openButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QObject::connect( _imp->openButton, SIGNAL( clicked() ), this, SLOT( onButtonClicked() ) );

    _imp->buttons = new QDialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
                                         Qt::Horizontal,this);
    QObject::connect(_imp->buttons, SIGNAL(accepted()), this, SLOT(onOkClicked()));
    QObject::connect(_imp->buttons, SIGNAL(rejected()), this, SLOT(reject()));


    _imp->mainLayout->addWidget(_imp->idLabel, 0, 0 , 1 , 1);
    _imp->mainLayout->addWidget(_imp->idEdit, 0, 1,  1 , 2);
    _imp->mainLayout->addWidget(_imp->labelLabel, 1, 0 , 1 , 1);
    _imp->mainLayout->addWidget(_imp->labelEdit, 1, 1,  1 , 2);
    _imp->mainLayout->addWidget(_imp->groupingLabel, 2, 0,  1 , 1);
    _imp->mainLayout->addWidget(_imp->groupingEdit, 2, 1,  1 , 2);
    _imp->mainLayout->addWidget(_imp->iconPathLabel, 3, 0 , 1 , 1);
    _imp->mainLayout->addWidget(_imp->iconPath, 3, 1 , 1 , 2);
    _imp->mainLayout->addWidget(_imp->descriptionLabel, 4, 0 , 1 , 1);
    _imp->mainLayout->addWidget(_imp->descriptionEdit, 4, 1 , 1 , 2);
    _imp->mainLayout->addWidget(_imp->fileLabel, 5, 0 , 1 , 1);
    _imp->mainLayout->addWidget(_imp->fileEdit, 5, 1, 1 , 1);
    _imp->mainLayout->addWidget(_imp->openButton, 5, 2, 1, 1);
    _imp->mainLayout->addWidget(_imp->buttons, 6, 0, 1, 3);

    resize(400,sizeHint().height());


}

ExportGroupTemplateDialog::~ExportGroupTemplateDialog()
{

}

void
ExportGroupTemplateDialog::onButtonClicked()
{
    std::vector<std::string> filters;

    const QString& path = _imp->gui->getLastPluginDirectory();
    SequenceFileDialog dialog(this, filters, false, SequenceFileDialog::eFileDialogModeDir, path.toStdString(), _imp->gui, false);
    if (dialog.exec()) {
        std::string selection = dialog.selectedFiles();
        _imp->fileEdit->setText(selection.c_str());
        QDir d = dialog.currentDirectory();
        _imp->gui->updateLastPluginDirectory(d.absolutePath());
    }
}

void
ExportGroupTemplateDialog::onLabelEditingFinished()
{
    if (_imp->idEdit->text().isEmpty()) {
        _imp->idEdit->setText(_imp->labelEdit->text());
    }
}

void
ExportGroupTemplateDialog::onOkClicked()
{
    QString dirPath = _imp->fileEdit->text();

    if (!dirPath.isEmpty() && dirPath[dirPath.size() - 1] == QChar('/')) {
        dirPath.remove(dirPath.size() - 1, 1);
    }
    QDir d(dirPath);

    if (!d.exists()) {
        Natron::errorDialog(tr("Error").toStdString(), tr("You must specify a directory to save the script").toStdString());
        return;
    }
    QString pluginLabel = _imp->labelEdit->text();
    if (pluginLabel.isEmpty()) {
        Natron::errorDialog(tr("Error").toStdString(), tr("You must specify a label to name the script").toStdString());
        return;
    } else {
        pluginLabel = Natron::makeNameScriptFriendly(pluginLabel.toStdString()).c_str();
    }

    QString pluginID = _imp->idEdit->text();
    if (pluginID.isEmpty()) {
        Natron::errorDialog(tr("Error").toStdString(), tr("You must specify a unique ID to identify the script").toStdString());
        return;
    }

    QString iconPath = _imp->iconPath->text();
    QString grouping = _imp->groupingEdit->text();
    QString description = _imp->descriptionEdit->text();

    QString filePath = d.absolutePath() + "/" + pluginLabel + ".py";

    QStringList filters;
    filters.push_back(QString(pluginLabel + ".py"));
    if (!d.entryList(filters,QDir::Files | QDir::NoDotAndDotDot).isEmpty()) {
        Natron::StandardButtonEnum rep = Natron::questionDialog(tr("Existing plug-in").toStdString(),
                                                                tr("A group plug-in with the same name already exists "
                                                                   "would you like to "
                                                                   "override it?").toStdString(), false);
        if  (rep == Natron::eStandardButtonNo) {
            return;
        }
    }

    bool foundInPath = false;
    QStringList groupSearchPath = appPTR->getAllNonOFXPluginsPaths();
    for (QStringList::iterator it = groupSearchPath.begin(); it != groupSearchPath.end(); ++it) {
        if (!it->isEmpty() && it->at(it->size() - 1) == QChar('/')) {
            it->remove(it->size() - 1, 1);
        }
        if (*it == dirPath) {
            foundInPath = true;
        }
    }

    if (!foundInPath) {

        QString message = dirPath + tr(" does not exist in the group plug-in search path, would you like to add it?");
        Natron::StandardButtonEnum rep = Natron::questionDialog(tr("Plug-in path").toStdString(),
                                                                message.toStdString(), false);

        if  (rep == Natron::eStandardButtonYes) {
            appPTR->getCurrentSettings()->appendPythonGroupsPath(dirPath.toStdString());
        }

    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        Natron::errorDialog(tr("Error").toStdString(), QString(tr("Cannot open ") + filePath).toStdString());
        return;
    }

    QTextStream ts(&file);
    QString content;
    _imp->group->exportGroupToPython(pluginID, pluginLabel, description, iconPath, grouping, content);
    ts << content;

    accept();
}
