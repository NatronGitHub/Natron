//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "Gui/KnobGuiFile.h"

#include <QLabel> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QFormLayout> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QHBoxLayout> // in QtGui on Qt4, in QtWidgets on Qt5

#include "Engine/Settings.h"
#include "Engine/KnobFile.h"

#include "Gui/GuiApplicationManager.h"
#include "Gui/Button.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/LineEdit.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/Gui.h"

using namespace Natron;

//===========================FILE_KNOB_GUI=====================================
File_KnobGui::File_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container)
: KnobGui(knob, container)
{
    boost::shared_ptr<File_Knob> fk = boost::dynamic_pointer_cast<File_Knob>(knob);
    assert(fk);
    QObject::connect(fk.get(), SIGNAL(openFile(bool)), this, SLOT(open_file(bool)));
}

File_KnobGui::~File_KnobGui()
{
    delete _lineEdit;
    delete _openFileButton;
}

void File_KnobGui::createWidget(QHBoxLayout* layout)
{


    _lineEdit = new LineEdit(layout->parentWidget());
    layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    _lineEdit->setPlaceholderText("File path...");
    _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_lineEdit, 0);

    
    if(hasToolTip()) {
        _lineEdit->setToolTip(toolTip());
    }
    QObject::connect(_lineEdit, SIGNAL(returnPressed()), this, SLOT(onReturnPressed()));

    _openFileButton = new Button(layout->parentWidget());

    QPixmap pix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, &pix);
    _openFileButton->setIcon(QIcon(pix));
    _openFileButton->setFixedSize(20, 20);
    QObject::connect(_openFileButton, SIGNAL(clicked()), this, SLOT(onButtonClicked()));


    QWidget *container = new QWidget(layout->parentWidget());
    QHBoxLayout *containerLayout = new QHBoxLayout(container);
    container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(_lineEdit);
    containerLayout->addWidget(_openFileButton);

    layout->addWidget(container);
}

void File_KnobGui::onButtonClicked() {
    boost::shared_ptr<File_Knob> fk = boost::dynamic_pointer_cast<File_Knob>(getKnob());
    assert(fk);
    open_file(fk->isAnimationEnabled());
}

class File_Knob_UndoCommand : public QUndoCommand {
    
    File_KnobGui* _knob;
    QStringList _oldFiles;
    QStringList _newFiles;
    
public:
    
    File_Knob_UndoCommand(File_KnobGui *knob,
                          const QStringList& oldFiles,
                          const QStringList& newFiles,
                          QUndoCommand *parent = 0)
    : QUndoCommand(parent)
    , _knob(knob)
    , _oldFiles(oldFiles)
    , _newFiles(newFiles)
    {
        
    }
    
private:
    
    virtual void undo() OVERRIDE FINAL {
        boost::shared_ptr<File_Knob> fk = boost::dynamic_pointer_cast<File_Knob>(_knob->getKnob());
        fk->setFiles(_oldFiles);
        QString value;
        if (_oldFiles.size() > 0) {
            value = _oldFiles.at(0);
        }
        _knob->updateGUI(0, Variant(value));
    }
    
    virtual void redo() OVERRIDE FINAL {
        boost::shared_ptr<File_Knob> fk = boost::dynamic_pointer_cast<File_Knob>(_knob->getKnob());
        fk->setFiles(_newFiles);
        QString value;
        if (_newFiles.size() > 0) {
            value = _newFiles.at(0);
        }
        _knob->updateGUI(0, Variant(value));
    }
    
};


void File_KnobGui::open_file(bool openSequence)
{
    
    std::vector<std::string> filters;
    
    boost::shared_ptr<File_Knob> fk = boost::dynamic_pointer_cast<File_Knob>(getKnob());
    if (!fk->isInputImageFile()) {
        filters.push_back("*");
    } else {
        std::map<std::string,std::string> readersForFormat;
        appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&readersForFormat);
        for (std::map<std::string,std::string>::const_iterator it = readersForFormat.begin(); it!=readersForFormat.end(); ++it) {
            filters.push_back(it->first);
        }
    }
    QStringList files;
    
    QString pathWhereToOpen;
    QStringList currentFiles = fk->getValue<QStringList>();
    if (currentFiles.isEmpty()) {
        pathWhereToOpen = _lastOpened;
    } else {
        QString first = currentFiles.at(0);
        pathWhereToOpen = SequenceFileDialog::getFilePath(first);
    }
    
    SequenceFileDialog dialog(_lineEdit->parentWidget(), filters, openSequence, SequenceFileDialog::OPEN_DIALOG, pathWhereToOpen.toStdString());
    if (dialog.exec()) {
        files = dialog.selectedFiles();
    }
    if (!files.isEmpty()) {
        updateLastOpened(files.at(0));
        QStringList oldFiles;
        fk->getFiles(&oldFiles);
        pushUndoCommand(new File_Knob_UndoCommand(this,oldFiles,files));
    }
}

void File_KnobGui::updateLastOpened(const QString &str)
{
    QString unpathed = str;
    _lastOpened = File_Knob::removePath(unpathed);
    getGui()->updateLastSequenceOpenedPath(_lastOpened);

}

void File_KnobGui::updateGUI(int /*dimension*/, const Variant &variant)
{
    QString file = variant.toString();
    if (getKnob()->isAnimationEnabled() && getKnob()->getKeyFramesCount(0) > 1) {
        file = SequenceFileDialog::patternFromFilesList(file);
        _lineEdit->setText(file);
    } else {
        _lineEdit->setText(file);
    }
}

void File_KnobGui::onReturnPressed()
{
    QString str = _lineEdit->text();
    if (str.isEmpty()) {
        return;
    }
    QStringList newList = SequenceFileDialog::filesListFromPattern(str);
    if (newList.isEmpty()) {
        return;
    }
    
    boost::shared_ptr<File_Knob> fk = boost::dynamic_pointer_cast<File_Knob>(getKnob());

    QStringList oldFiles;
    fk->getFiles(&oldFiles);
    pushUndoCommand(new File_Knob_UndoCommand(this,oldFiles,newList));
}


void File_KnobGui::_hide()
{
    _openFileButton->hide();
    _lineEdit->hide();
}

void File_KnobGui::_show()
{
    _openFileButton->show();
    _lineEdit->show();
}

void File_KnobGui::setEnabled() {
    bool enabled = getKnob()->isEnabled(0);
    _openFileButton->setEnabled(enabled);
    _lineEdit->setEnabled(enabled);

}
void File_KnobGui::setReadOnly(bool readOnly,int /*dimension*/) {
    _openFileButton->setEnabled(!readOnly);
    _lineEdit->setReadOnly(readOnly);
}

//============================OUTPUT_FILE_KNOB_GUI====================================
OutputFile_KnobGui::OutputFile_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container)
: KnobGui(knob, container)
{
    boost::shared_ptr<OutputFile_Knob> fk = boost::dynamic_pointer_cast<OutputFile_Knob>(knob);
    assert(fk);
    QObject::connect(fk.get(), SIGNAL(openFile(bool)), this, SLOT(open_file(bool)));
}

OutputFile_KnobGui::~OutputFile_KnobGui()
{
    delete _lineEdit;
    delete _openFileButton;
}

void OutputFile_KnobGui::createWidget(QHBoxLayout* layout)
{


    _lineEdit = new LineEdit(layout->parentWidget());
    QObject::connect(_lineEdit, SIGNAL(returnPressed()), this, SLOT(onReturnPressed()));
    _lineEdit->setPlaceholderText(QString("File path..."));
    if(hasToolTip()) {
        _lineEdit->setToolTip(toolTip());
    }
    _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_lineEdit, 0);

    
    _openFileButton = new Button(layout->parentWidget());

    QPixmap pix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, &pix);
    _openFileButton->setIcon(QIcon(pix));
    _openFileButton->setFixedSize(20, 20);
    QObject::connect(_openFileButton, SIGNAL(clicked()), this, SLOT(onButtonClicked()));


    QWidget *container = new QWidget(layout->parentWidget());
    QHBoxLayout *containerLayout = new QHBoxLayout(container);
    container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);

    containerLayout->addWidget(_lineEdit);
    containerLayout->addWidget(_openFileButton);

    layout->addWidget(container);
}

void OutputFile_KnobGui::onButtonClicked() {
    boost::shared_ptr<OutputFile_Knob> fk = boost::dynamic_pointer_cast<OutputFile_Knob>(getKnob());
    assert(fk);
    open_file(fk->isSequencesDialogEnabled());
}

void OutputFile_KnobGui::open_file(bool openSequence)
{
    
    std::vector<std::string> filters;
    
    boost::shared_ptr<OutputFile_Knob> fk = boost::dynamic_pointer_cast<OutputFile_Knob>(getKnob());
    if (!fk->isOutputImageFile()) {
        filters.push_back("*");
    } else {
        std::map<std::string,std::string> writersForFormat;
        appPTR->getCurrentSettings()->getFileFormatsForWritingAndWriter(&writersForFormat);
        for (std::map<std::string,std::string>::const_iterator it = writersForFormat.begin(); it!=writersForFormat.end(); ++it) {
            filters.push_back(it->first);
        }

    }
    
    SequenceFileDialog dialog(_lineEdit->parentWidget(), filters, openSequence, SequenceFileDialog::SAVE_DIALOG, _lastOpened.toStdString());
    if (dialog.exec()) {
        QString oldPattern = _lineEdit->text();
        QString newPattern = dialog.filesToSave();
        updateLastOpened(File_Knob::removePath(oldPattern));
        
        pushValueChangedCommand(Variant(newPattern));
    }
}

void OutputFile_KnobGui::updateLastOpened(const QString &str)
{
    QString withoutPath = str;
    _lastOpened = File_Knob::removePath(withoutPath);
    getGui()->updateLastSequenceSavedPath(_lastOpened);

}

void OutputFile_KnobGui::updateGUI(int /*dimension*/, const Variant &variant)
{
    _lineEdit->setText(variant.toString());
}



void OutputFile_KnobGui::onReturnPressed()
{
    QString newPattern = _lineEdit->text();

    pushValueChangedCommand(Variant(newPattern));
}


void OutputFile_KnobGui::_hide()
{
    _openFileButton->hide();
    _lineEdit->hide();
}

void OutputFile_KnobGui::_show()
{
    _openFileButton->show();
    _lineEdit->show();
}

void OutputFile_KnobGui::setEnabled() {
    bool enabled = getKnob()->isEnabled(0);
    _openFileButton->setEnabled(enabled);
    _lineEdit->setEnabled(enabled);
    
}

void OutputFile_KnobGui::setReadOnly(bool readOnly,int /*dimension*/) {
    _openFileButton->setEnabled(!readOnly);
    _lineEdit->setReadOnly(readOnly);
}
//============================PATH_KNOB_GUI====================================
Path_KnobGui::Path_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container)
: KnobGui(knob, container)
{
    boost::shared_ptr<Path_Knob> fk = boost::dynamic_pointer_cast<Path_Knob>(knob);
    assert(fk);
    QObject::connect(fk.get(), SIGNAL(openFile()), this, SLOT(open_file()));
}

Path_KnobGui::~Path_KnobGui()
{
    delete _lineEdit;
    delete _openFileButton;
}

void Path_KnobGui::createWidget(QHBoxLayout* layout)
{
   
    _lineEdit = new LineEdit(layout->parentWidget());
    QObject::connect(_lineEdit, SIGNAL(returnPressed()), this, SLOT(onReturnPressed()));
    if(hasToolTip()) {
        _lineEdit->setToolTip(toolTip());
    }
    _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_lineEdit, 0);
    
    _openFileButton = new Button(layout->parentWidget());
    _openFileButton->setToolTip("Click to select a path to append to/replace this variable.");
    QPixmap pix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, &pix);
    _openFileButton->setIcon(QIcon(pix));
    _openFileButton->setFixedSize(20, 20);
    QObject::connect(_openFileButton, SIGNAL(clicked()), this, SLOT(onButtonClicked()));
    
    
    QWidget *container = new QWidget(layout->parentWidget());
    QHBoxLayout *containerLayout = new QHBoxLayout(container);
    container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    
    containerLayout->addWidget(_lineEdit);
    containerLayout->addWidget(_openFileButton);
    
    layout->addWidget(container);
}

void Path_KnobGui::onButtonClicked() {
    boost::shared_ptr<Path_Knob> fk = boost::dynamic_pointer_cast<Path_Knob>(getKnob());
    assert(fk);
    open_file();
}

void Path_KnobGui::open_file()
{
    
    std::vector<std::string> filters;
    
    SequenceFileDialog dialog(_lineEdit->parentWidget(), filters, false, SequenceFileDialog::DIR_DIALOG, _lastOpened.toStdString());
    if (dialog.exec()) {
        QString dirPath = dialog.currentDirectory().absolutePath();
        updateLastOpened(dirPath);
        boost::shared_ptr<Path_Knob> fk = boost::dynamic_pointer_cast<Path_Knob>(getKnob());

        if (fk->isMultiPath()) {
            QString existingPath = fk->getValue<QString>();
            if (!existingPath.isEmpty()) {
                existingPath.append(QChar(';'));
                dirPath.prepend(existingPath);
            }
            
        }
        pushValueChangedCommand(Variant(dirPath));
    }
}

void Path_KnobGui::updateLastOpened(const QString &str)
{
    QString withoutPath = str;
    _lastOpened = File_Knob::removePath(withoutPath);
}

void Path_KnobGui::updateGUI(int /*dimension*/, const Variant &variant)
{
    _lineEdit->setText(variant.toString());
}



void Path_KnobGui::_hide()
{
    _openFileButton->hide();
    _lineEdit->hide();
}

void Path_KnobGui::_show()
{
    _openFileButton->show();
    _lineEdit->show();
}

void Path_KnobGui::onReturnPressed()
{
    QString dirPath = _lineEdit->text();
    boost::shared_ptr<Path_Knob> fk = boost::dynamic_pointer_cast<Path_Knob>(getKnob());
    
    if (fk->isMultiPath()) {
        QString existingPath = fk->getValue<QString>();
        if (!existingPath.isEmpty() && !dirPath.isEmpty()) {
            existingPath.append(QChar(';'));
            dirPath.prepend(existingPath);
        }
    }
    pushValueChangedCommand(Variant(dirPath));
}


void Path_KnobGui::setEnabled() {
    bool enabled = getKnob()->isEnabled(0);
    _openFileButton->setEnabled(enabled);
    _lineEdit->setEnabled(enabled);
    
}

void Path_KnobGui::setReadOnly(bool readOnly,int /*dimension*/) {
    _openFileButton->setEnabled(!readOnly);
    _lineEdit->setReadOnly(readOnly);
}
