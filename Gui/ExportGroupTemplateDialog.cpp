/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <stdexcept>

#include <boost/scoped_array.hpp>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QTextStream>
#include <QGridLayout>
#include <QTextEdit>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/NodeGroup.h"
#include "Engine/Node.h"
#include "Engine/Settings.h"
#include "Engine/Utils.h" // convertFromPlainText

#include "Gui/Button.h"
#include "Gui/DialogButtonBox.h"
#include "Gui/Gui.h"
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/SpinBox.h"
#include "Gui/GuiDefines.h"

NATRON_NAMESPACE_ENTER

class PlaceHolderTextEdit
    : public QTextEdit
{
    QString placeHolder;

public:


    PlaceHolderTextEdit(QWidget* parent)
        : QTextEdit(parent)
        , placeHolder()
    {
    }

    void setPlaceHolderText(const QString& ph)
    {
        placeHolder = ph;
    }

    QString getText() const
    {
        QString ret = toPlainText();

        if (ret == placeHolder) {
            return QString();
        }

        return ret;
    }

private:

    virtual void focusInEvent(QFocusEvent *e)
    {
        if ( !placeHolder.isNull() ) {
            QString t = toPlainText();
            if ( t.isEmpty() || (t == placeHolder) ) {
                clear();
            }
        }
        QTextEdit::focusInEvent(e);
    }

    virtual void focusOutEvent(QFocusEvent *e)
    {
        if ( !placeHolder.isEmpty() ) {
            if ( toPlainText().isEmpty() ) {
                setText(placeHolder);
            }
        }
        QTextEdit::focusOutEvent(e);
    }
};


struct ExportGroupTemplateDialogPrivate
{
    Gui* gui;
    NodeCollection* group;
    QGridLayout* mainLayout;
    Label* labelLabel;
    LineEdit* labelEdit;
    Label* idLabel;
    LineEdit* idEdit;
    Label* groupingLabel;
    LineEdit* groupingEdit;
    Label* versionLabel;
    SpinBox* versionSpinbox;
    Label* fileLabel;
    LineEdit* fileEdit;
    Button* openButton;
    Label* iconPathLabel;
    LineEdit* iconPath;
    Label* descriptionLabel;
    PlaceHolderTextEdit* descriptionEdit;
    DialogButtonBox *buttons;

    ExportGroupTemplateDialogPrivate(NodeCollection* group,
                                     Gui* gui)
        : gui(gui)
        , group(group)
        , mainLayout(0)
        , labelLabel(0)
        , labelEdit(0)
        , idLabel(0)
        , idEdit(0)
        , groupingLabel(0)
        , groupingEdit(0)
        , versionLabel(0)
        , versionSpinbox(0)
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

ExportGroupTemplateDialog::ExportGroupTemplateDialog(NodeCollection* group,
                                                     Gui* gui,
                                                     QWidget* parent)
    : QDialog(parent)
    , _imp( new ExportGroupTemplateDialogPrivate(group, gui) )
{
    _imp->mainLayout = new QGridLayout(this);



    _imp->idLabel = new Label(tr("Unique ID"), this);
    QString idTt = NATRON_NAMESPACE::convertFromPlainText(tr("The unique ID is used by %1 to identify the plug-in in various "
                                                     "places in the application.\n"
                                                     "Generally, this contains domain and sub-domains names "
                                                     "such as fr.inria.group.XXX.\n"
                                                     "If two plug-ins or more happen to have the same ID, they will be "
                                                     "gathered by version.\n"
                                                     "If two plug-ins or more have the same ID and version, the first loaded in the"
                                                     " search-paths will take precedence over the other.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ), NATRON_NAMESPACE::WhiteSpaceNormal);
    _imp->idEdit = new LineEdit(this);
    _imp->idEdit->setPlaceholderText( QString::fromUtf8("org.organization.pyplugs.XXX") );
    _imp->idEdit->setToolTip(idTt);


    _imp->labelLabel = new Label(tr("Label"), this);
    QString labelTt = NATRON_NAMESPACE::convertFromPlainText(tr("Set the label of the group as the user will see it in the user interface."), NATRON_NAMESPACE::WhiteSpaceNormal);
    _imp->labelLabel->setToolTip(labelTt);
    _imp->labelEdit = new LineEdit(this);
    _imp->labelEdit->setPlaceholderText( QString::fromUtf8("MyPlugin") );
    QObject::connect( _imp->labelEdit, SIGNAL(editingFinished()), this, SLOT(onLabelEditingFinished()) );
    _imp->labelEdit->setToolTip(labelTt);


    _imp->groupingLabel = new Label(tr("Grouping"), this);
    QString groupingTt = NATRON_NAMESPACE::convertFromPlainText(tr("The grouping of the plug-in specifies where the plug-in will be located in the menus, "
                                                           "e.g. \"Color/Transform\", or \"Draw\". Each sub-level must be separated by a '/'."), NATRON_NAMESPACE::WhiteSpaceNormal);
    _imp->groupingLabel->setToolTip(groupingTt);

    _imp->groupingEdit = new LineEdit(this);
    _imp->groupingEdit->setPlaceholderText( QString::fromUtf8("Color/Transform") );
    _imp->groupingEdit->setToolTip(groupingTt);


    _imp->versionLabel = new Label(tr("Version"), this);
    QString versionTt = NATRON_NAMESPACE::convertFromPlainText(tr("The version can be incremented when changing the behaviour of the plug-in. "
                                                          "If a user is using and old version of this plug-in in a project, if a newer version "
                                                          "of this plug-in is available, upon opening the project a dialog will ask whether "
                                                          "the plug-in should update to the newer version in the project or not."), NATRON_NAMESPACE::WhiteSpaceNormal);
    _imp->versionLabel->setToolTip(versionTt);

    _imp->versionSpinbox = new SpinBox(this, SpinBox::eSpinBoxTypeInt);
    _imp->versionSpinbox->setMinimum(1);
    _imp->versionSpinbox->setToolTip(versionTt);

    _imp->iconPathLabel = new Label(tr("Icon relative path"), this);
    QString iconTt = NATRON_NAMESPACE::convertFromPlainText(tr("Set here the file path of an optional icon to identify the plug-in. "
                                                       "The path is relative to the Python script."), NATRON_NAMESPACE::WhiteSpaceNormal);
    _imp->iconPathLabel->setToolTip(iconTt);
    _imp->iconPath = new LineEdit(this);
    _imp->iconPath->setPlaceholderText( QString::fromUtf8("Label.png") );
    _imp->iconPath->setToolTip(iconTt);

    _imp->descriptionLabel = new Label(tr("Description"), this);
    QString descTt =  NATRON_NAMESPACE::convertFromPlainText(tr("Set here the (optional) plug-in description that the user will see when clicking the "
                                                        "\"?\" button on the settings panel of the node."), NATRON_NAMESPACE::WhiteSpaceNormal);
    _imp->descriptionEdit = new PlaceHolderTextEdit(this);
    _imp->descriptionEdit->setToolTip(descTt);
    _imp->descriptionEdit->setPlaceHolderText( tr("This plug-in can be used to produce XXX effect...") );

    _imp->fileLabel = new Label(tr("Directory"), this);
    QString fileTt  = NATRON_NAMESPACE::convertFromPlainText(tr("Specify here the directory where to export the Python script."), NATRON_NAMESPACE::WhiteSpaceNormal);
    _imp->fileLabel->setToolTip(fileTt);
    _imp->fileEdit = new LineEdit(this);


    _imp->fileEdit->setToolTip(fileTt);


    QPixmap openPix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, NATRON_MEDIUM_BUTTON_ICON_SIZE, &openPix);
    _imp->openButton = new Button(QIcon(openPix), QString(), this);
    _imp->openButton->setFocusPolicy(Qt::NoFocus);
    _imp->openButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QObject::connect( _imp->openButton, SIGNAL(clicked()), this, SLOT(onButtonClicked()) );

    _imp->buttons = new DialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel),
                                         Qt::Horizontal, this);
    QObject::connect( _imp->buttons, SIGNAL(accepted()), this, SLOT(onOkClicked()) );
    QObject::connect( _imp->buttons, SIGNAL(rejected()), this, SLOT(reject()) );


    _imp->mainLayout->addWidget(_imp->idLabel, 0, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->idEdit, 0, 1,  1, 2);
    _imp->mainLayout->addWidget(_imp->labelLabel, 1, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->labelEdit, 1, 1,  1, 2);
    _imp->mainLayout->addWidget(_imp->groupingLabel, 2, 0,  1, 1);
    _imp->mainLayout->addWidget(_imp->groupingEdit, 2, 1,  1, 2);
    _imp->mainLayout->addWidget(_imp->versionLabel, 3, 0,  1, 1);
    _imp->mainLayout->addWidget(_imp->versionSpinbox, 3, 1,  1, 1);
    _imp->mainLayout->addWidget(_imp->iconPathLabel, 4, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->iconPath, 4, 1, 1, 2);
    _imp->mainLayout->addWidget(_imp->descriptionLabel, 5, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->descriptionEdit, 5, 1, 1, 2);
    _imp->mainLayout->addWidget(_imp->fileLabel, 6, 0, 1, 1);
    _imp->mainLayout->addWidget(_imp->fileEdit, 6, 1, 1, 1);
    _imp->mainLayout->addWidget(_imp->openButton, 6, 2, 1, 1);
    _imp->mainLayout->addWidget(_imp->buttons, 7, 0, 1, 3);

    // If this node is already a PyPlug, pre-fill the dialog with existing information
    NodeGroup* isGroupNode = dynamic_cast<NodeGroup*>(group);
    if (isGroupNode) {
        NodePtr pyPlug = isGroupNode->getNode();
        std::string pluginID = pyPlug->getPyPlugID();
        // This is a pyplug for sure
        std::string description = pyPlug->getPyPlugDescription();
        std::string label = pyPlug->getPyPlugLabel();
        std::string iconFilePath = pyPlug->getPyPlugIconFilePath();
        std::string grouping;
        std::list<std::string> groupingList;
        pyPlug->getPyPlugGrouping(&groupingList);
        if (!groupingList.empty()) {
            std::list<std::string>::iterator next = groupingList.begin();
            ++next;
            for (std::list<std::string>::iterator it = groupingList.begin(); it!=groupingList.end(); ++it) {
                grouping.append(*it);

                if (next != groupingList.end()) {
                    grouping += '/';
                    ++next;
                }
            }
        }
        int version = pyPlug->getMajorVersion();
        std::string pluginPath = pyPlug->getPluginPythonModule();
        {
            std::size_t foundLastSlash = pluginPath.find_last_of("/");
            if (foundLastSlash != std::string::npos) {
                pluginPath = pluginPath.substr(0, foundLastSlash);
            }
        }
        {
            std::size_t foundLastSlash = iconFilePath.find_last_of("/");
            if (foundLastSlash != std::string::npos) {
                iconFilePath = iconFilePath.substr(foundLastSlash + 1);
            }
            if (iconFilePath == "group_icon.png") {
                iconFilePath.clear();
            }
        }
        _imp->idEdit->setText(QString::fromUtf8(pluginID.c_str()));
        _imp->labelEdit->setText(QString::fromUtf8(label.c_str()));
        _imp->groupingEdit->setText(QString::fromUtf8(grouping.c_str()));
        _imp->descriptionEdit->setText(QString::fromUtf8(description.c_str()));
        _imp->iconPath->setText(QString::fromUtf8(iconFilePath.c_str()));
        _imp->fileEdit->setText(QString::fromUtf8(pluginPath.c_str()));
        _imp->versionSpinbox->setValue(version);

    }

    resize( 400, sizeHint().height() );
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

    if ( dialog.exec() ) {
        std::string selection = dialog.selectedFiles();
        _imp->fileEdit->setText( QString::fromUtf8( selection.c_str() ) );
        QDir d = dialog.currentDirectory();
        _imp->gui->updateLastPluginDirectory( d.absolutePath() );
    }
}

void
ExportGroupTemplateDialog::onLabelEditingFinished()
{
    if ( _imp->idEdit->text().isEmpty() ) {
        _imp->idEdit->setText( _imp->labelEdit->text() );
    }
}

void
ExportGroupTemplateDialog::onOkClicked()
{
    QString dirPath = _imp->fileEdit->text();

    if ( !dirPath.isEmpty() && ( dirPath[dirPath.size() - 1] == QLatin1Char('/') ) ) {
        dirPath.remove(dirPath.size() - 1, 1);
    }
    QDir d(dirPath);

    if ( !d.exists() ) {
        Dialogs::errorDialog( tr("Error").toStdString(), tr("You must specify a directory to save the script").toStdString() );

        return;
    }
    QString pluginLabel = _imp->labelEdit->text();
    if ( pluginLabel.isEmpty() ) {
        Dialogs::errorDialog( tr("Error").toStdString(), tr("You must specify a label to name the script").toStdString() );

        return;
    } else {
        pluginLabel = QString::fromUtf8( NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly( pluginLabel.toStdString() ).c_str() );
    }

    QString pluginID = _imp->idEdit->text();
    if ( pluginID.isEmpty() ) {
        Dialogs::errorDialog( tr("Error").toStdString(), tr("You must specify a unique ID to identify the script").toStdString() );

        return;
    }

    QString iconPath = _imp->iconPath->text();
    QString grouping = _imp->groupingEdit->text();
    QString description = _imp->descriptionEdit->getText();
    QString filePath = d.absolutePath() + QLatin1Char('/') + pluginLabel + QString::fromUtf8(".py");
    QStringList filters;
    filters.push_back( QString( pluginLabel + QString::fromUtf8(".py") ) );
    if ( !d.entryList(filters, QDir::Files | QDir::NoDotAndDotDot).isEmpty() ) {
        StandardButtonEnum rep = Dialogs::questionDialog(tr("Existing plug-in").toStdString(),
                                                         tr("A group plug-in with the name \"%1\" already exists "
                                                            "would you like to "
                                                            "overwrite it?").arg(pluginLabel).toStdString(), false);
        if  (rep == eStandardButtonNo) {
            return;
        }
    }

    bool foundInPath = false;
    QStringList groupSearchPath = appPTR->getAllNonOFXPluginsPaths();
    for (QStringList::iterator it = groupSearchPath.begin(); it != groupSearchPath.end(); ++it) {
        if ( !it->isEmpty() && ( it->at(it->size() - 1) == QLatin1Char('/') ) ) {
            it->remove(it->size() - 1, 1);
        }
        if (*it == dirPath) {
            foundInPath = true;
        }
    }

    if (!foundInPath) {
        QString message = tr("Directory \"%1\" is not in the group plug-in search path, would you like to add it?").arg(dirPath);
        StandardButtonEnum rep = Dialogs::questionDialog(tr("Plug-in path").toStdString(),
                                                         message.toStdString(), false);

        if  (rep == eStandardButtonYes) {
            appPTR->getCurrentSettings()->appendPythonGroupsPath( dirPath.toStdString() );
        }
    }

    QFile file(filePath);
    if ( !file.open(QIODevice::ReadWrite | QIODevice::Truncate) ) {
        Dialogs::errorDialog( tr("Error").toStdString(), QString(tr("Cannot open ") + filePath).toStdString() );

        return;
    }

    int version = _imp->versionSpinbox->value();

    QTextStream ts(&file);
    QString content;
    _imp->group->exportGroupToPython(pluginID, pluginLabel, description, iconPath, grouping, version, content);
    ts << content;

    accept();
} // ExportGroupTemplateDialog::onOkClicked

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_ExportGroupTemplateDialog.cpp"
