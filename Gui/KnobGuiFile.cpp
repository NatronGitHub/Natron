//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "Gui/KnobGuiFile.h"

#include <QLabel> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QFormLayout> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QHBoxLayout> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QDebug>

#include "Engine/Settings.h"
#include "Engine/KnobFile.h"
#include <SequenceParsing.h>
#include "Engine/EffectInstance.h"
#include "Engine/Project.h"

#include "Gui/GuiApplicationManager.h"
#include "Gui/Button.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/LineEdit.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"


using namespace Natron;

//===========================FILE_KNOB_GUI=====================================
File_KnobGui::File_KnobGui(boost::shared_ptr<KnobI> knob,
                           DockablePanel *container)
    : KnobGui(knob, container)
{
    _knob = boost::dynamic_pointer_cast<File_Knob>(knob);
    assert(_knob);
    QObject::connect( _knob.get(), SIGNAL( openFile() ), this, SLOT( open_file() ) );
}

File_KnobGui::~File_KnobGui()
{
    delete _lineEdit;
    delete _openFileButton;
}

void
File_KnobGui::createWidget(QHBoxLayout* layout)
{
    _lineEdit = new LineEdit( layout->parentWidget() );
    layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    _lineEdit->setPlaceholderText( tr("File path...") );
    _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_lineEdit, 0);


    if ( hasToolTip() ) {
        _lineEdit->setToolTip( toolTip() );
    }
    QObject::connect( _lineEdit, SIGNAL( editingFinished() ), this, SLOT( onReturnPressed() ) );

    _openFileButton = new Button( layout->parentWidget() );
    _openFileButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QPixmap pix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, &pix);
    _openFileButton->setIcon( QIcon(pix) );
    QObject::connect( _openFileButton, SIGNAL( clicked() ), this, SLOT( onButtonClicked() ) );
    QWidget *container = new QWidget( layout->parentWidget() );
    QHBoxLayout *containerLayout = new QHBoxLayout(container);
    container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(_lineEdit);
    containerLayout->addWidget(_openFileButton);

    layout->addWidget(container);
}

void
File_KnobGui::onButtonClicked()
{
    open_file();
}

void
File_KnobGui::open_file()
{
    std::vector<std::string> filters;

    if ( !_knob->isInputImageFile() ) {
        filters.push_back("*");
    } else {
        Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>( _knob->getHolder() );
        if (effect) {
            filters = effect->supportedFileFormats();
        }
    }
    std::string currentPattern = _knob->getValue();
    std::string path = SequenceParsing::removePath(currentPattern);
    QString pathWhereToOpen;
    if ( path.empty() ) {
        pathWhereToOpen = _lastOpened;
    } else {
        pathWhereToOpen = path.c_str();
    }

    SequenceFileDialog dialog( _lineEdit->parentWidget(), filters, _knob->isInputImageFile(),
                               SequenceFileDialog::OPEN_DIALOG, pathWhereToOpen.toStdString(), getGui());
    if ( dialog.exec() ) {
        std::string selectedFile = dialog.selectedFiles();
        std::string originalSelectedFile = selectedFile;
        path = SequenceParsing::removePath(selectedFile);
        updateLastOpened( path.c_str() );
        pushUndoCommand( new KnobUndoCommand<std::string>(this,currentPattern,originalSelectedFile) );
    }
}

void
File_KnobGui::updateLastOpened(const QString &str)
{
    std::string unpathed = str.toStdString();

    _lastOpened = SequenceParsing::removePath(unpathed).c_str();
    getGui()->updateLastSequenceOpenedPath(_lastOpened);
}

void
File_KnobGui::updateGUI(int /*dimension*/)
{
    _lineEdit->setText( _knob->getValue().c_str() );
}

void
File_KnobGui::onReturnPressed()
{
    QString str = _lineEdit->text();

    ///don't do antyhing if the pattern is the same
    std::string oldValue = _knob->getValue();

    if ( str == oldValue.c_str() ) {
        return;
    }
    pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,str.toStdString() ) );
}

void
File_KnobGui::_hide()
{
    _openFileButton->hide();
    _lineEdit->hide();
}

void
File_KnobGui::_show()
{
    _openFileButton->show();
    _lineEdit->show();
}

void
File_KnobGui::setEnabled()
{
    bool enabled = getKnob()->isEnabled(0);

    _openFileButton->setEnabled(enabled);
    _lineEdit->setEnabled(enabled);
}

void
File_KnobGui::setReadOnly(bool readOnly,
                          int /*dimension*/)
{
    _openFileButton->setEnabled(!readOnly);
    _lineEdit->setReadOnly(readOnly);
}

void
File_KnobGui::setDirty(bool dirty)
{
    _lineEdit->setDirty(dirty);
}

boost::shared_ptr<KnobI> File_KnobGui::getKnob() const
{
    return _knob;
}

//============================OUTPUT_FILE_KNOB_GUI====================================
OutputFile_KnobGui::OutputFile_KnobGui(boost::shared_ptr<KnobI> knob,
                                       DockablePanel *container)
    : KnobGui(knob, container)
{
    _knob = boost::dynamic_pointer_cast<OutputFile_Knob>(knob);
    assert(_knob);
    QObject::connect( _knob.get(), SIGNAL( openFile(bool) ), this, SLOT( open_file(bool) ) );
}

OutputFile_KnobGui::~OutputFile_KnobGui()
{
    delete _lineEdit;
    delete _openFileButton;
}

void
OutputFile_KnobGui::createWidget(QHBoxLayout* layout)
{
    _lineEdit = new LineEdit( layout->parentWidget() );
    layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QObject::connect( _lineEdit, SIGNAL( editingFinished() ), this, SLOT( onReturnPressed() ) );
    _lineEdit->setPlaceholderText( tr("File path...") );
    if ( hasToolTip() ) {
        _lineEdit->setToolTip( toolTip() );
    }
    _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_lineEdit, 0);


    _openFileButton = new Button( layout->parentWidget() );
    _openFileButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QPixmap pix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, &pix);
    _openFileButton->setIcon( QIcon(pix) );
    QObject::connect( _openFileButton, SIGNAL( clicked() ), this, SLOT( onButtonClicked() ) );
    QWidget *container = new QWidget( layout->parentWidget() );
    QHBoxLayout *containerLayout = new QHBoxLayout(container);
    container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);

    containerLayout->addWidget(_lineEdit);
    containerLayout->addWidget(_openFileButton);

    layout->addWidget(container);
}

void
OutputFile_KnobGui::onButtonClicked()
{
    open_file( _knob->isSequencesDialogEnabled() );
}

void
OutputFile_KnobGui::open_file(bool openSequence)
{
    std::vector<std::string> filters;

    if ( !_knob->isOutputImageFile() ) {
        filters.push_back("*");
    } else {
        Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>( getKnob()->getHolder() );
        if (effect) {
            filters = effect->supportedFileFormats();
        }
    }

    SequenceFileDialog dialog( _lineEdit->parentWidget(), filters, openSequence, SequenceFileDialog::SAVE_DIALOG, _lastOpened.toStdString(),
                              getGui());
    if ( dialog.exec() ) {
        std::string oldPattern = _lineEdit->text().toStdString();
        std::string newPattern = dialog.filesToSave();
        updateLastOpened( SequenceParsing::removePath(oldPattern).c_str() );

        pushUndoCommand( new KnobUndoCommand<std::string>(this,oldPattern,newPattern) );
    }
}

void
OutputFile_KnobGui::updateLastOpened(const QString &str)
{
    std::string withoutPath = str.toStdString();

    _lastOpened = SequenceParsing::removePath(withoutPath).c_str();
    getGui()->updateLastSequenceSavedPath(_lastOpened);
}

void
OutputFile_KnobGui::updateGUI(int /*dimension*/)
{
    _lineEdit->setText( _knob->getValue().c_str() );
}

void
OutputFile_KnobGui::onReturnPressed()
{
    QString newPattern = _lineEdit->text();

    pushUndoCommand( new KnobUndoCommand<std::string>( this,_knob->getValue(),newPattern.toStdString() ) );
}

void
OutputFile_KnobGui::_hide()
{
    _openFileButton->hide();
    _lineEdit->hide();
}

void
OutputFile_KnobGui::_show()
{
    _openFileButton->show();
    _lineEdit->show();
}

void
OutputFile_KnobGui::setEnabled()
{
    bool enabled = getKnob()->isEnabled(0);

    _openFileButton->setEnabled(enabled);
    _lineEdit->setEnabled(enabled);
}

void
OutputFile_KnobGui::setReadOnly(bool readOnly,
                                int /*dimension*/)
{
    _openFileButton->setEnabled(!readOnly);
    _lineEdit->setReadOnly(readOnly);
}

void
OutputFile_KnobGui::setDirty(bool dirty)
{
    _lineEdit->setDirty(dirty);
}

boost::shared_ptr<KnobI> OutputFile_KnobGui::getKnob() const
{
    return _knob;
}

//============================PATH_KNOB_GUI====================================
Path_KnobGui::Path_KnobGui(boost::shared_ptr<KnobI> knob,
                           DockablePanel *container)
    : KnobGui(knob, container)
{
    _knob = boost::dynamic_pointer_cast<Path_Knob>(knob);
    assert(_knob);
    QObject::connect( _knob.get(), SIGNAL( openFile() ), this, SLOT( open_file() ) );
}

Path_KnobGui::~Path_KnobGui()
{
    delete _lineEdit;
    delete _openFileButton;
}

void
Path_KnobGui::createWidget(QHBoxLayout* layout)
{
    _lineEdit = new LineEdit( layout->parentWidget() );
    layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QObject::connect( _lineEdit, SIGNAL( editingFinished() ), this, SLOT( onReturnPressed() ) );
    if ( hasToolTip() ) {
        _lineEdit->setToolTip( toolTip() );
    }
    _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_lineEdit, 0);

    _openFileButton = new Button( layout->parentWidget() );
    _openFileButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    _openFileButton->setToolTip( tr("Click to select a path to append to/replace this variable.") );
    QPixmap pix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, &pix);
    _openFileButton->setIcon( QIcon(pix) );
    QObject::connect( _openFileButton, SIGNAL( clicked() ), this, SLOT( onButtonClicked() ) );
    QWidget *container = new QWidget( layout->parentWidget() );
    QHBoxLayout *containerLayout = new QHBoxLayout(container);
    container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);

    containerLayout->addWidget(_lineEdit);
    containerLayout->addWidget(_openFileButton);

    layout->addWidget(container);
}

void
Path_KnobGui::onButtonClicked()
{
    open_file();
}

void
Path_KnobGui::open_file()
{
    std::vector<std::string> filters;
    SequenceFileDialog dialog( _lineEdit->parentWidget(), filters, false, SequenceFileDialog::DIR_DIALOG, _lastOpened.toStdString() );

    if ( dialog.exec() ) {
        QString dirPath = dialog.currentDirectory().absolutePath();
        updateLastOpened(dirPath);

        if ( _knob->isMultiPath() ) {
            std::string existingPath = _knob->getValue();
            if ( !existingPath.empty() ) {
                existingPath += ';';
                dirPath.prepend( existingPath.c_str() );
            }
        }
        pushUndoCommand( new KnobUndoCommand<std::string>( this,_knob->getValue(),dirPath.toStdString() ) );
    }
}

void
Path_KnobGui::updateLastOpened(const QString &str)
{
    std::string withoutPath = str.toStdString();

    _lastOpened = SequenceParsing::removePath(withoutPath).c_str();
}

void
Path_KnobGui::updateGUI(int /*dimension*/)
{
    _lineEdit->setText( _knob->getValue().c_str() );
}

void
Path_KnobGui::_hide()
{
    _openFileButton->hide();
    _lineEdit->hide();
}

void
Path_KnobGui::_show()
{
    _openFileButton->show();
    _lineEdit->show();
}

void
Path_KnobGui::onReturnPressed()
{
    QString dirPath = _lineEdit->text();
    boost::shared_ptr<Path_Knob> fk = boost::dynamic_pointer_cast<Path_Knob>( getKnob() );

    if ( fk->isMultiPath() ) {
        std::string existingPath = fk->getValue();
        if ( !existingPath.empty() && !dirPath.isEmpty() ) {
            existingPath += ';';
            dirPath.prepend( existingPath.c_str() );
        }
    }
    pushUndoCommand( new KnobUndoCommand<std::string>( this,fk->getValue(),dirPath.toStdString() ) );
}

void
Path_KnobGui::setEnabled()
{
    bool enabled = getKnob()->isEnabled(0);

    _openFileButton->setEnabled(enabled);
    _lineEdit->setEnabled(enabled);
}

void
Path_KnobGui::setReadOnly(bool readOnly,
                          int /*dimension*/)
{
    _openFileButton->setEnabled(!readOnly);
    _lineEdit->setReadOnly(readOnly);
}

void
Path_KnobGui::setDirty(bool dirty)
{
    _lineEdit->setDirty(dirty);
}

boost::shared_ptr<KnobI> Path_KnobGui::getKnob() const
{
    return _knob;
}

