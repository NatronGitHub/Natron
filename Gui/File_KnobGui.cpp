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

#include "File_KnobGui.h"

#include <QFormLayout> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QHBoxLayout> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QDebug>
#include <QItemSelectionModel>
#include <QHeaderView>
#include <QPainter>
#include <QStyle>
#include <QApplication>
#include <QStyledItemDelegate>
#include <QAction>
#include <QFileSystemWatcher>

#include "Engine/EffectInstance.h"
#include "Engine/KnobFile.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"

#include "Gui/Button.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/LineEdit.h"
#include "Gui/Menu.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/TableModelView.h"
#include "Gui/Utils.h"

#include <SequenceParsing.h>

using namespace Natron;


//===========================FILE_KNOB_GUI=====================================
File_KnobGui::File_KnobGui(boost::shared_ptr<KnobI> knob,
                           DockablePanel *container)
    : KnobGui(knob, container)
    , _lineEdit(0)
    , _openFileButton(0)
    , _reloadButton(0)
    , _lastOpened()
    , _watcher(new QFileSystemWatcher)
    , _fileBeingWatched()
{
    boost::shared_ptr<File_Knob> k = boost::dynamic_pointer_cast<File_Knob>(knob);
    assert(k);
    QObject::connect( k.get(), SIGNAL( openFile() ), this, SLOT( open_file() ) );
    QObject::connect(_watcher, SIGNAL(fileChanged(QString)), this, SLOT(watchedFileChanged()));
    _knob = k;

}

File_KnobGui::~File_KnobGui()
{
}

void File_KnobGui::removeSpecificGui()
{
    delete _lineEdit;
    delete _openFileButton;
    delete _watcher;
}

void
File_KnobGui::createWidget(QHBoxLayout* layout)
{
    
    boost::shared_ptr<File_Knob> knob = _knob.lock();
    if (knob->getHolder() && knob->getEvaluateOnChange()) {
        boost::shared_ptr<TimeLine> timeline = getGui()->getApp()->getTimeLine();
        QObject::connect(timeline.get(), SIGNAL(frameChanged(SequenceTime,int)), this, SLOT(onTimelineFrameChanged(SequenceTime, int)));
    }
    
    QWidget *container = new QWidget( layout->parentWidget() );
    QHBoxLayout *containerLayout = new QHBoxLayout(container);
    container->setLayout(containerLayout);
    containerLayout->setSpacing(0);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    
    _lineEdit = new LineEdit(container);
    //layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    _lineEdit->setPlaceholderText( tr("File path...") );
    _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_lineEdit, 0);

    QObject::connect( _lineEdit, SIGNAL( editingFinished() ), this, SLOT( onTextEdited() ) );


    _openFileButton = new Button(container);
    _openFileButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QPixmap pix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
    _openFileButton->setIcon( QIcon(pix) );
    _openFileButton->setToolTip(toolTip());
    _openFileButton->setFocusPolicy(Qt::NoFocus); // exclude from tab focus
    QObject::connect( _openFileButton, SIGNAL( clicked() ), this, SLOT( onButtonClicked() ) );
    
    containerLayout->addWidget(_lineEdit);
    containerLayout->addWidget(_openFileButton);
    
    if (knob->getHolder() && knob->isInputImageFile()) {
        _reloadButton = new Button(container);
        _reloadButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _reloadButton->setFocusPolicy(Qt::NoFocus);
        QPixmap pixRefresh;
        appPTR->getIcon(NATRON_PIXMAP_VIEWER_REFRESH, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixRefresh);
        _reloadButton->setIcon(QIcon(pixRefresh));
        _reloadButton->setToolTip(Natron::convertFromPlainText(tr("Reload the file."), Qt::WhiteSpaceNormal));
        QObject::connect( _reloadButton, SIGNAL( clicked() ), this, SLOT( onReloadClicked() ) );
        containerLayout->addWidget(_reloadButton);
    }
    layout->addWidget(container);
    
}

void
File_KnobGui::onButtonClicked()
{
    open_file();
}

void
File_KnobGui::onReloadClicked()
{
    
    if (_reloadButton) {
        boost::shared_ptr<File_Knob> knob = _knob.lock();
        assert(knob->getHolder());
        EffectInstance* effect = dynamic_cast<EffectInstance*>(knob->getHolder());
        if (effect) {
            effect->purgeCaches();
            effect->clearPersistentMessage(false);
        }
        knob->evaluateValueChange(0, Natron::eValueChangedReasonNatronInternalEdited);
    }
}


void
File_KnobGui::open_file()
{
    std::vector<std::string> filters;
    boost::shared_ptr<File_Knob> knob = _knob.lock();
    if ( !knob->isInputImageFile() ) {
        filters.push_back("*");
    } else {
        Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>( knob->getHolder() );
        if (effect) {
            filters = effect->supportedFileFormats();
        }
    }
    std::string oldPattern = knob->getGuiValue();
    std::string currentPattern = oldPattern;
    std::string path = SequenceParsing::removePath(currentPattern);
    QString pathWhereToOpen;
    if ( path.empty() ) {
        pathWhereToOpen = _lastOpened;
    } else {
        pathWhereToOpen = path.c_str();
    }

    SequenceFileDialog dialog( _lineEdit->parentWidget(), filters, knob->isInputImageFile(),
                               SequenceFileDialog::eFileDialogModeOpen, pathWhereToOpen.toStdString(), getGui(),true);
    
    if ( dialog.exec() ) {
        std::string selectedFile = dialog.selectedFiles();
        
        std::string originalSelectedFile = selectedFile;
        path = SequenceParsing::removePath(selectedFile);
        updateLastOpened( path.c_str() );
        
        pushUndoCommand( new KnobUndoCommand<std::string>(this,oldPattern,originalSelectedFile) );
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
    boost::shared_ptr<File_Knob> knob = _knob.lock();
    _lineEdit->setText(knob->getGuiValue().c_str());
    
    bool useNotifications = appPTR->getCurrentSettings()->notifyOnFileChange();
    if (useNotifications && knob->getHolder() && knob->getEvaluateOnChange() ) {
        if (!_fileBeingWatched.empty()) {
            _watcher->removePath(_fileBeingWatched.c_str());
            _fileBeingWatched.clear();
        }
        
        std::string newValue = knob->getFileName(knob->getCurrentTime());
        if (knob->getHolder()->getApp()) {
            knob->getHolder()->getApp()->getProject()->canonicalizePath(newValue);
        }
        QString file(newValue.c_str());
        
        if (QFile::exists(file)) {
            _watcher->addPath(file);
            QFileInfo info(file);
            _lastModified = info.lastModified();
            
            QString tt = toolTip();
            tt.append("\n\nLast modified: ");
            tt.append(_lastModified.toString(Qt::SystemLocaleShortDate));
            _lineEdit->setToolTip(tt);
            _fileBeingWatched = newValue;
        }
    }
}

void
File_KnobGui::onTimelineFrameChanged(SequenceTime time,int /*reason*/)
{
    
    bool useNotifications = appPTR->getCurrentSettings()->notifyOnFileChange();
    if (!useNotifications) {
        return;
    }
    boost::shared_ptr<File_Knob> knob = _knob.lock();
    ///Get the current file, if it exists, add the file path to the file system watcher
    ///to get notified if the file changes.
    std::string filepath = knob->getFileName(time);
    if (!filepath.empty() && knob->getHolder() && knob->getHolder()->getApp()) {
        knob->getHolder()->getApp()->getProject()->canonicalizePath(filepath);
    }
    if (filepath != _fileBeingWatched  && knob->getHolder() && knob->getEvaluateOnChange() ) {
        
        
        if (!_fileBeingWatched.empty()) {
            _watcher->removePath(_fileBeingWatched.c_str());
            _fileBeingWatched.clear();
        }
        
        QString qfilePath(filepath.c_str());
        
        if (QFile::exists(qfilePath)) {
            _watcher->addPath(qfilePath);
            _fileBeingWatched = filepath;
            QFileInfo info(qfilePath);
            _lastModified = info.lastModified();
        }
    }
   
    

}

void
File_KnobGui::watchedFileChanged()
{
    ///The file has changed, trigger a new render.
    
    ///Make sure the node doesn't hold any cache
    boost::shared_ptr<File_Knob> knob = _knob.lock();
    if (knob->getHolder()) {
        EffectInstance* effect = dynamic_cast<EffectInstance*>(knob->getHolder());
        if (effect) {
            effect->purgeCaches();
            
            if (_reloadButton) {
                QFileInfo fileMonitored(_fileBeingWatched.c_str());
                if (fileMonitored.lastModified() != _lastModified) {
                    QString warn = tr("The file ") + _lineEdit->text() + tr(" has changed on disk. Press reload file to load the new version of the file");
                    effect->setPersistentMessage(Natron::eMessageTypeWarning, warn.toStdString());
                }
                
            } else {
                 knob->evaluateValueChange(0, Natron::eValueChangedReasonNatronInternalEdited);
            }
        }
        
    }
    
    
}

void File_KnobGui::onTextEdited()
{
    std::string str = _lineEdit->text().toStdString();
    
    ///don't do antyhing if the pattern is the same
    std::string oldValue = _knob.lock()->getGuiValue();
    
    if ( str == oldValue ) {
        return;
    }
    
//    
//    if (_knob->getHolder() && _knob->getHolder()->getApp()) {
//        std::map<std::string,std::string> envvar;
//        _knob->getHolder()->getApp()->getProject()->getEnvironmentVariables(envvar);
//        Natron::Project::findReplaceVariable(envvar,str);
//    }
    
    
    
    pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,str ) );
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
    return _knob.lock();
}

void
File_KnobGui::addRightClickMenuEntries(QMenu* menu)
{
    QAction* makeAbsoluteAction = new QAction(QObject::tr("Make absolute"),menu);
    QObject::connect(makeAbsoluteAction,SIGNAL(triggered()),this,SLOT(onMakeAbsoluteTriggered()));
    makeAbsoluteAction->setToolTip(QObject::tr("Make the file-path absolute if it was previously relative to any project path"));
    menu->addAction(makeAbsoluteAction);
    QAction* makeRelativeToProject = new QAction(QObject::tr("Make relative to project"),menu);
    makeRelativeToProject->setToolTip(QObject::tr("Make the file-path relative to the [Project] path"));
    QObject::connect(makeRelativeToProject,SIGNAL(triggered()),this,SLOT(onMakeRelativeTriggered()));
    menu->addAction(makeRelativeToProject);
    QAction* simplify = new QAction(QObject::tr("Simplify"),menu);
    QObject::connect(simplify,SIGNAL(triggered()),this,SLOT(onSimplifyTriggered()));
    simplify->setToolTip(QObject::tr("Same as make relative but will pick the longest project variable to simplify"));
    menu->addAction(simplify);

    menu->addSeparator();
    QMenu* qtMenu = _lineEdit->createStandardContextMenu();
    qtMenu->setFont(QApplication::font()); // necessary
    qtMenu->setTitle(tr("Edit"));
    menu->addMenu(qtMenu);
}

void
File_KnobGui::onMakeAbsoluteTriggered()
{
    boost::shared_ptr<File_Knob> knob = _knob.lock();
    if (knob->getHolder() && knob->getHolder()->getApp()) {
        std::string oldValue = knob->getGuiValue();
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->canonicalizePath(newValue);
        pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,newValue ) );
    }
}

void
File_KnobGui::onMakeRelativeTriggered()
{
    boost::shared_ptr<File_Knob> knob = _knob.lock();
    if (knob->getHolder() && knob->getHolder()->getApp()) {
        std::string oldValue = knob->getGuiValue();
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->makeRelativeToProject(newValue);
        pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,newValue ) );
    }

}

void
File_KnobGui::onSimplifyTriggered()
{
    boost::shared_ptr<File_Knob> knob = _knob.lock();
    if (knob->getHolder() && knob->getHolder()->getApp()) {
        std::string oldValue = knob->getGuiValue();
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->simplifyPath(newValue);
        pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,newValue ) );
    }
}

void
File_KnobGui::reflectAnimationLevel(int /*dimension*/,Natron::AnimationLevelEnum /*level*/)
{
    _lineEdit->setAnimation(0);
}

void
File_KnobGui::reflectExpressionState(int /*dimension*/,bool hasExpr)
{
    bool isSlaved = _knob.lock()->isSlave(0);
    _lineEdit->setAnimation(3);
    _lineEdit->setReadOnly(hasExpr || isSlaved);
    _openFileButton->setEnabled(!hasExpr && !isSlaved);
}

void
File_KnobGui::updateToolTip()
{
    if (hasToolTip()) {
        QString tt = toolTip();
        _lineEdit->setToolTip(tt);
    }
}

//============================OUTPUT_FILE_KNOB_GUI====================================
OutputFile_KnobGui::OutputFile_KnobGui(boost::shared_ptr<KnobI> knob,
                                       DockablePanel *container)
    : KnobGui(knob, container)
    , _lineEdit(0)
    , _openFileButton(0)
{
    _knob = boost::dynamic_pointer_cast<OutputFile_Knob>(knob);
    assert(_knob.lock());
    QObject::connect( _knob.lock().get(), SIGNAL( openFile(bool) ), this, SLOT( open_file(bool) ) );
}

OutputFile_KnobGui::~OutputFile_KnobGui()
{

}

void OutputFile_KnobGui::removeSpecificGui()
{
    delete _lineEdit;
    delete _openFileButton;
}


void
OutputFile_KnobGui::createWidget(QHBoxLayout* layout)
{
    _lineEdit = new LineEdit( layout->parentWidget() );
    layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QObject::connect( _lineEdit, SIGNAL( editingFinished() ), this, SLOT( onTextEdited() ) );

    _lineEdit->setPlaceholderText( tr("File path...") );

    _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_lineEdit, 0);


    _openFileButton = new Button( layout->parentWidget() );
    _openFileButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QPixmap pix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
    _openFileButton->setIcon( QIcon(pix) );
    _openFileButton->setToolTip(Natron::convertFromPlainText(tr("Browse file..."), Qt::WhiteSpaceNormal));
    _openFileButton->setFocusPolicy(Qt::NoFocus); // exclude from tab focus
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
    open_file( _knob.lock()->isSequencesDialogEnabled() );
}

void
OutputFile_KnobGui::open_file(bool openSequence)
{
    std::vector<std::string> filters;

    if ( !_knob.lock()->isOutputImageFile() ) {
        filters.push_back("*");
    } else {
        Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>( getKnob()->getHolder() );
        if (effect) {
            filters = effect->supportedFileFormats();
        }
    }

    SequenceFileDialog dialog( _lineEdit->parentWidget(), filters, openSequence, SequenceFileDialog::eFileDialogModeSave, _lastOpened.toStdString(), getGui(),true);
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
    _lineEdit->setText( _knob.lock()->getGuiValue().c_str() );
}

void
OutputFile_KnobGui::onTextEdited()
{
    std::string newPattern = _lineEdit->text().toStdString();

//    if (_knob->getHolder() && _knob->getHolder()->getApp()) {
//        std::map<std::string,std::string> envvar;
//        _knob->getHolder()->getApp()->getProject()->getEnvironmentVariables(envvar);
//        Natron::Project::findReplaceVariable(envvar,newPattern);
//    }
//
//    
    pushUndoCommand( new KnobUndoCommand<std::string>( this,_knob.lock()->getGuiValue(),newPattern ) );
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
    return _knob.lock();
}


void
OutputFile_KnobGui::addRightClickMenuEntries(QMenu* menu)
{
    QAction* makeAbsoluteAction = new QAction(QObject::tr("Make absolute"),menu);
    QObject::connect(makeAbsoluteAction,SIGNAL(triggered()),this,SLOT(onMakeAbsoluteTriggered()));
    makeAbsoluteAction->setToolTip(QObject::tr("Make the file-path absolute if it was previously relative to any project path"));
    menu->addAction(makeAbsoluteAction);
    QAction* makeRelativeToProject = new QAction(QObject::tr("Make relative to project"),menu);
    makeRelativeToProject->setToolTip(QObject::tr("Make the file-path relative to the [Project] path"));
    QObject::connect(makeRelativeToProject,SIGNAL(triggered()),this,SLOT(onMakeRelativeTriggered()));
    menu->addAction(makeRelativeToProject);
    QAction* simplify = new QAction(QObject::tr("Simplify"),menu);
    QObject::connect(simplify,SIGNAL(triggered()),this,SLOT(onSimplifyTriggered()));
    simplify->setToolTip(QObject::tr("Same as make relative but will pick the longest project variable to simplify"));
    menu->addAction(simplify);
    
    menu->addSeparator();
    QMenu* qtMenu = _lineEdit->createStandardContextMenu();
    qtMenu->setFont(QApplication::font()); // necessary
    qtMenu->setTitle(tr("Edit"));
    menu->addMenu(qtMenu);
}

void
OutputFile_KnobGui::onMakeAbsoluteTriggered()
{
    boost::shared_ptr<OutputFile_Knob> knob = _knob.lock();
    if (knob->getHolder() && knob->getHolder()->getApp()) {
        std::string oldValue = knob->getGuiValue();
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->canonicalizePath(newValue);
        pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,newValue ) );
    }
}

void
OutputFile_KnobGui::onMakeRelativeTriggered()
{
    boost::shared_ptr<OutputFile_Knob> knob = _knob.lock();
    if (knob->getHolder() && knob->getHolder()->getApp()) {
        std::string oldValue = knob->getGuiValue();
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->makeRelativeToProject(newValue);
        pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,newValue ) );
    }
    
}

void
OutputFile_KnobGui::onSimplifyTriggered()
{
    boost::shared_ptr<OutputFile_Knob> knob = _knob.lock();
    if (knob->getHolder() && knob->getHolder()->getApp()) {
        std::string oldValue = knob->getGuiValue();
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->simplifyPath(newValue);
        pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,newValue ) );
    }
}

void
OutputFile_KnobGui::reflectExpressionState(int /*dimension*/,bool hasExpr)
{
    bool isSlaved = _knob.lock()->isSlave(0);
    _lineEdit->setAnimation(3);
    _lineEdit->setReadOnly(hasExpr || isSlaved);
    _openFileButton->setEnabled(!hasExpr && !isSlaved);
}


void
OutputFile_KnobGui::reflectAnimationLevel(int /*dimension*/,Natron::AnimationLevelEnum /*level*/)
{
    _lineEdit->setAnimation(0);
}

void
OutputFile_KnobGui::updateToolTip()
{
    if (hasToolTip()) {
        QString tt = toolTip();
        _lineEdit->setToolTip(tt);
    }
}

//============================PATH_KNOB_GUI====================================
Path_KnobGui::Path_KnobGui(boost::shared_ptr<KnobI> knob,
                           DockablePanel *container)
    : KnobGui(knob, container)
    , _mainContainer(0)
    , _lineEdit(0)
    , _openFileButton(0)
    , _table(0)
    , _model(0)
    , _addPathButton(0)
    , _removePathButton(0)
    , _editPathButton(0)
    , _isInsertingItem(false)
{
    _knob = boost::dynamic_pointer_cast<Path_Knob>(knob);
    assert(_knob.lock());
}

Path_KnobGui::~Path_KnobGui()
{
}

void Path_KnobGui::removeSpecificGui()
{
    delete _mainContainer;
}

////////////// TableView delegate

class PathKnobTableItemDelegate
: public QStyledItemDelegate
{
    TableView* _view;
    
public:
    
    explicit PathKnobTableItemDelegate(TableView* view);
    
private:
    
    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const OVERRIDE FINAL;
};

PathKnobTableItemDelegate::PathKnobTableItemDelegate(TableView* view)
: QStyledItemDelegate(view)
, _view(view)
{
}

void
PathKnobTableItemDelegate::paint(QPainter * painter,
                         const QStyleOptionViewItem & option,
                         const QModelIndex & index) const
{
    
    if (!index.isValid() || option.state & QStyle::State_Selected) {
        QStyledItemDelegate::paint(painter,option,index);
        
        return;
    }
    TableModel* model = dynamic_cast<TableModel*>( _view->model() );
    assert(model);
    if (!model) {
        // coverity[dead_error_begin]
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }
    TableItem* item = model->item(index);
    if (!item) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }
    QPen pen;
    
    if (!item->flags().testFlag(Qt::ItemIsEnabled)) {
        pen.setColor(Qt::black);
    } else {
        pen.setColor( QColor(200,200,200) );
        
    }
    painter->setPen(pen);
    
    // get the proper subrect from the style
    QStyle *style = QApplication::style();
    QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &option);
    
    ///Draw the item name column
    if (option.state & QStyle::State_Selected) {
        painter->fillRect( geom, option.palette.highlight() );
    }
    QRect r;
    QString str = item->data(Qt::DisplayRole).toString();
    if (index.column() == 0) {
        ///Env vars are used between brackets
        str.prepend('[');
        str.append(']');
    }
    painter->drawText(geom,Qt::TextSingleLine,str,&r);
}


void
Path_KnobGui::createWidget(QHBoxLayout* layout)
{
    _mainContainer = new QWidget(layout->parentWidget());
    boost::shared_ptr<Path_Knob> knob = _knob.lock();
    if (knob->isMultiPath()) {
        QVBoxLayout* mainLayout = new QVBoxLayout(_mainContainer);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        
        _table = new TableView( _mainContainer );
        layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        //    QObject::connect( _table, SIGNAL( editingFinished() ), this, SLOT( onReturnPressed() ) );
  
        _table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        _table->setAttribute(Qt::WA_MacShowFocusRect,0);

#if QT_VERSION < 0x050000
        _table->header()->setResizeMode(QHeaderView::ResizeToContents);
#else
        _table->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
        _table->header()->setStretchLastSection(true);
        _table->setItemDelegate(new PathKnobTableItemDelegate(_table));
        
        _model = new TableModel(0,0,_table);
        QObject::connect( _model,SIGNAL( s_itemChanged(TableItem*) ),this,SLOT( onItemDataChanged(TableItem*) ) );
        
        _table->setTableModel(_model);
        
        _table->setColumnCount(2);
        QStringList headers;
        headers << tr("Variable name") << tr("Value");
        _table->setHorizontalHeaderLabels(headers);
        
        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_table, 0);
        
        QWidget* buttonsContainer = new QWidget(_mainContainer);
        QHBoxLayout* buttonsLayout = new QHBoxLayout(buttonsContainer);
        buttonsLayout->setContentsMargins(0, 0, 0, 0);
        
        _addPathButton = new Button( tr("Add"),buttonsContainer );
        _addPathButton->setToolTip(Natron::convertFromPlainText(tr("Click to add a new project path."), Qt::WhiteSpaceNormal));
        QObject::connect( _addPathButton, SIGNAL( clicked() ), this, SLOT( onAddButtonClicked() ) );
        
        _removePathButton = new Button( tr("Remove"),buttonsContainer);
        QObject::connect( _removePathButton, SIGNAL( clicked() ), this, SLOT( onRemoveButtonClicked() ) );
        _removePathButton->setToolTip(Natron::convertFromPlainText(tr("Click to remove selected project path."), Qt::WhiteSpaceNormal));
        
        _editPathButton = new Button( tr("Edit"), buttonsContainer);
        QObject::connect( _editPathButton, SIGNAL( clicked() ), this, SLOT( onEditButtonClicked() ) );
        _editPathButton->setToolTip(Natron::convertFromPlainText(tr("Click to change the path of the selected project path."), Qt::WhiteSpaceNormal));
        
        buttonsLayout->addWidget(_addPathButton);
        buttonsLayout->addWidget(_removePathButton);
        buttonsLayout->addWidget(_editPathButton);
        buttonsLayout->addStretch();
        
        mainLayout->addWidget(_table);
        mainLayout->addWidget(buttonsContainer);
        
    } else { // _knob->isMultiPath()
        QHBoxLayout* mainLayout = new QHBoxLayout(_mainContainer);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        _lineEdit = new LineEdit(_mainContainer);
        _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QObject::connect( _lineEdit, SIGNAL( editingFinished() ), this, SLOT( onTextEdited() ) );

        enableRightClickMenu(_lineEdit, 0);
        _openFileButton = new Button( layout->parentWidget() );
        _openFileButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _openFileButton->setToolTip(Natron::convertFromPlainText(tr("Click to select a path to append to/replace this variable."), Qt::WhiteSpaceNormal));
        QPixmap pix;
        appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
        _openFileButton->setIcon( QIcon(pix) );
        QObject::connect( _openFileButton, SIGNAL( clicked() ), this, SLOT( onOpenFileButtonClicked() ) );
        
        mainLayout->addWidget(_lineEdit);
        mainLayout->addWidget(_openFileButton);
    }
    
    layout->addWidget(_mainContainer);
}

void
Path_KnobGui::onAddButtonClicked()
{
    std::vector<std::string> filters;
    SequenceFileDialog dialog( _mainContainer, filters, false, SequenceFileDialog::eFileDialogModeDir, _lastOpened.toStdString(),getGui(),true );
    
    if ( dialog.exec() ) {
        std::string dirPath = dialog.selectedDirectory();
        if (!dirPath.empty() && dirPath[dirPath.size() - 1] == '/') {
            dirPath.erase(dirPath.size() - 1, 1);
        }
        updateLastOpened(dirPath.c_str());
        
        
        std::string oldValue = _knob.lock()->getGuiValue();
        
        int rowCount = (int)_items.size();
        
        QString varName = QString(tr("Path") + "%1").arg(rowCount);
        createItem(rowCount, dirPath.c_str(), varName);
        std::string newPath = rebuildPath();
        
       
        
        pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,newPath ) );
    }

}

void
Path_KnobGui::onEditButtonClicked()
{
    std::string oldValue = _knob.lock()->getGuiValue();
    QModelIndexList selection = _table->selectionModel()->selectedRows();
    
    if (selection.size() != 1) {
        return;
    }
    
    Variables::iterator found = _items.find(selection[0].row());
    if (found != _items.end()) {
        std::vector<std::string> filters;
        
        SequenceFileDialog dialog( _mainContainer, filters, false, SequenceFileDialog::eFileDialogModeDir, found->second.value->text().toStdString(),getGui(),true );
        if (dialog.exec()) {
            
            std::string dirPath = dialog.selectedDirectory();
            if (!dirPath.empty() && dirPath[dirPath.size() - 1] == '/') {
                dirPath.erase(dirPath.size() - 1, 1);
            }
            updateLastOpened(dirPath.c_str());
            
            found->second.value->setText(dirPath.c_str());
            std::string newPath = rebuildPath();
            
            
            
            pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,newPath ) );
        }
    }

    
    
}

void
Path_KnobGui::onOpenFileButtonClicked()
{
    std::vector<std::string> filters;
    SequenceFileDialog dialog( _mainContainer, filters, false, SequenceFileDialog::eFileDialogModeDir, _lastOpened.toStdString(),getGui(),true );
    
    if ( dialog.exec() ) {
        std::string dirPath = dialog.selectedDirectory();
        updateLastOpened(dirPath.c_str());
        
        std::string oldValue = _knob.lock()->getGuiValue();
        
        pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,dirPath ) );
    }

}

void
Path_KnobGui::onRemoveButtonClicked()
{
    boost::shared_ptr<Path_Knob> knob = _knob.lock();
    std::string oldValue = knob->getGuiValue();
    QModelIndexList selection = _table->selectionModel()->selectedRows();

	if (selection.isEmpty()) {
		return;
	}
    
    std::list<std::string> removeVars;
    for (int i = 0; i < selection.size(); ++i) {
        Variables::iterator found = _items.find(selection[i].row());
        if (found != _items.end()) {
            removeVars.push_back(found->second.varName->text().toStdString());
            _items.erase(found);
        }
    }
    
    
    _model->removeRows(selection.front().row(),selection.size());

    
    ///Fix all variables if needed
    if (knob->getHolder() && knob->getHolder() == getGui()->getApp()->getProject().get() &&
        appPTR->getCurrentSettings()->isAutoFixRelativeFilePathEnabled()) {
        for (std::list<std::string>::iterator it = removeVars.begin(); it != removeVars.end(); ++it) {
            getGui()->getApp()->getProject()->fixRelativeFilePaths(*it, "",false);
        }
        
    }
    
    std::string newPath = rebuildPath();
    
    pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,newPath ) );
}


void
Path_KnobGui::onTextEdited()
{
    std::string dirPath = _lineEdit->text().toStdString();
    if (!dirPath.empty() && dirPath[dirPath.size() - 1] == '/') {
        dirPath.erase(dirPath.size() - 1, 1);
    }
    updateLastOpened(dirPath.c_str());
    
    
    
//    if (allowSimplification && _knob->getHolder() && _knob->getHolder()->getApp()) {
//        std::map<std::string,std::string> envvar;
//        _knob->getHolder()->getApp()->getProject()->getEnvironmentVariables(envvar);
//        Natron::Project::findReplaceVariable(envvar,dirPath);
//    }

    
    std::string oldValue = _knob.lock()->getGuiValue();
    
    pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,dirPath ) );
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
    boost::shared_ptr<Path_Knob> knob = _knob.lock();
    QString path(_knob.lock()->getGuiValue().c_str());
    
    if (_knob.lock()->isMultiPath()) {
        std::map<std::string,std::string> variables;
        Natron::Project::makeEnvMap(path.toStdString(), variables);
        
        
        _model->clear();
        _items.clear();
        int i = 0;
        for (std::map<std::string,std::string> ::const_iterator it = variables.begin(); it != variables.end(); ++it, ++i) {
            createItem(i, it->second.c_str(), it->first.c_str());
        }
    } else {
        _lineEdit->setText(path);
    }
}

void
Path_KnobGui::createItem(int row,const QString& value,const QString& varName)
{
    
    
    Qt::ItemFlags flags;
    
    ///Project env var is disabled and uneditable and set automatically by the project
    if (varName != NATRON_PROJECT_ENV_VAR_NAME && varName != NATRON_OCIO_ENV_VAR_NAME) {
        flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }
    TableItem* cell0 = new TableItem;
    cell0->setText(varName);
    cell0->setFlags(flags);
    
    TableItem* cell1 = new TableItem;
    cell1->setText(value);
    cell1->setFlags(flags);
    
    Row r;
    r.varName = cell0;
    r.value = cell1;
    
    _items.insert(std::make_pair(row, r));
    int modelRowCount = _model->rowCount();
    if (row >= modelRowCount) {
        _model->insertRow(row);
    }
    _isInsertingItem = true;
    _table->setItem(row, 0, cell0);
    _table->setItem(row, 1, cell1);
    _isInsertingItem = false;
    _table->resizeColumnToContents(0);
    _table->resizeColumnToContents(1);
}

void
Path_KnobGui::_hide()
{
    _mainContainer->hide();
}

void
Path_KnobGui::_show()
{
    _mainContainer->show();
}

void
Path_KnobGui::setEnabled()
{
    bool enabled = getKnob()->isEnabled(0);
    if (_knob.lock()->isMultiPath()) {
        _table->setEnabled(enabled);
        _addPathButton->setEnabled(enabled);
        _removePathButton->setEnabled(enabled);
    } else {
        _lineEdit->setEnabled(enabled);
        _openFileButton->setEnabled(enabled);
    }
}

void
Path_KnobGui::setReadOnly(bool readOnly,
                          int /*dimension*/)
{
    if (_knob.lock()->isMultiPath()) {
        _table->setEnabled(!readOnly);
        _addPathButton->setEnabled(!readOnly);
        _removePathButton->setEnabled(!readOnly);
    } else {
        _lineEdit->setReadOnly(readOnly);
        _openFileButton->setEnabled(!readOnly);
    }
}

void
Path_KnobGui::setDirty(bool /*dirty*/)
{
    
}

boost::shared_ptr<KnobI> Path_KnobGui::getKnob() const
{
    return _knob.lock();
}

void
Path_KnobGui::onItemDataChanged(TableItem* /*item*/)
{
    if (_isInsertingItem) {
        return;
    }
    boost::shared_ptr<Path_Knob> knob = _knob.lock();
    std::string newPath = rebuildPath();
    std::string oldPath = knob->getGuiValue();
    
    if (oldPath != newPath) {
        
        if (knob->getHolder() && knob->getHolder()->isProject() &&
            appPTR->getCurrentSettings()->isAutoFixRelativeFilePathEnabled()) {
            std::map<std::string,std::string> oldEnv,newEnv;
            
            Natron::Project::makeEnvMap(oldPath,oldEnv);
            Natron::Project::makeEnvMap(newPath, newEnv);
            
            ///Compare the 2 maps to find-out if a path has changed or just a name
            if (oldEnv.size() == newEnv.size()) {
                std::map<std::string,std::string>::iterator itOld = oldEnv.begin();
                for (std::map<std::string,std::string>::iterator itNew = newEnv.begin(); itNew != newEnv.end(); ++itNew, ++itOld) {
                    
                    if (itOld->first != itNew->first) {
                        ///a name has changed
                        getGui()->getApp()->getProject()->fixPathName(itOld->first, itNew->first);
                        break;
                    } else if (itOld->second != itOld->second) {
                        getGui()->getApp()->getProject()->fixRelativeFilePaths(itOld->first, itNew->second,false);
                        break;
                    }
                }
            }
        }
        
        pushUndoCommand( new KnobUndoCommand<std::string>( this,oldPath,newPath ) );
    }
}

/**
 * @brief A Path knob could also be called Environment_variable_Knob.
 * The string is encoded the following way:
 * [VariableName1]:[Value1];[VariableName2]:[Value2] etc...
 * Split all the ';' characters to get all different variables
 * then for each variable split the ':' to get the name and the value of the variable.
 **/
std::string
Path_KnobGui::rebuildPath() const
{
    std::string path;
    
    Variables::const_iterator next = _items.begin();
    if (next != _items.end()) {
        ++next;
    }
    for (Variables::const_iterator it = _items.begin(); it != _items.end(); ++it) {
        // In order to use XML tags, the text inside the tags has to be escaped.
        path += NATRON_ENV_VAR_NAME_START_TAG;
        path += Project::escapeXML(it->second.varName->text().toStdString());
        path += NATRON_ENV_VAR_NAME_END_TAG;
        path += NATRON_ENV_VAR_VALUE_START_TAG;
        path += Project::escapeXML(it->second.value->text().toStdString());
        path += NATRON_ENV_VAR_VALUE_END_TAG;

        // increment for next iteration
        if (next != _items.end()) {
            ++next;
        }
    } // for(it)
    return path;
}



void
Path_KnobGui::addRightClickMenuEntries(QMenu* menu)
{
    if (!_knob.lock()->isMultiPath()) {
        QAction* makeAbsoluteAction = new QAction(QObject::tr("Make absolute"),menu);
        QObject::connect(makeAbsoluteAction,SIGNAL(triggered()),this,SLOT(onMakeAbsoluteTriggered()));
        makeAbsoluteAction->setToolTip(QObject::tr("Make the file-path absolute if it was previously relative to any project path"));
        menu->addAction(makeAbsoluteAction);
        QAction* makeRelativeToProject = new QAction(QObject::tr("Make relative to project"),menu);
        makeRelativeToProject->setToolTip(QObject::tr("Make the file-path relative to the [Project] path"));
        QObject::connect(makeRelativeToProject,SIGNAL(triggered()),this,SLOT(onMakeRelativeTriggered()));
        menu->addAction(makeRelativeToProject);
        QAction* simplify = new QAction(QObject::tr("Simplify"),menu);
        QObject::connect(simplify,SIGNAL(triggered()),this,SLOT(onSimplifyTriggered()));
        simplify->setToolTip(QObject::tr("Same as make relative but will pick the longest project variable to simplify"));
        menu->addAction(simplify);
        
        menu->addSeparator();
        QMenu* qtMenu = _lineEdit->createStandardContextMenu();
        qtMenu->setFont(QApplication::font()); // necessary
        qtMenu->setTitle(tr("Edit"));
        menu->addMenu(qtMenu);
    }
}

void
Path_KnobGui::onMakeAbsoluteTriggered()
{
    boost::shared_ptr<Path_Knob> knob = _knob.lock();
    if (knob->getHolder() && knob->getHolder()->getApp()) {
        std::string oldValue = knob->getGuiValue();
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->canonicalizePath(newValue);
        pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,newValue ) );
    }
}

void
Path_KnobGui::onMakeRelativeTriggered()
{
    boost::shared_ptr<Path_Knob> knob = _knob.lock();
    if (knob->getHolder() && knob->getHolder()->getApp()) {
        std::string oldValue = knob->getGuiValue();
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->makeRelativeToProject(newValue);
        pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,newValue ) );
    }
    
}

void
Path_KnobGui::onSimplifyTriggered()
{
    boost::shared_ptr<Path_Knob> knob = _knob.lock();
    if (knob->getHolder() && knob->getHolder()->getApp()) {
        std::string oldValue = knob->getGuiValue();
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->simplifyPath(newValue);
        pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,newValue ) );
    }
}

void
Path_KnobGui::reflectAnimationLevel(int /*dimension*/,Natron::AnimationLevelEnum /*level*/)
{
    if (!_knob.lock()->isMultiPath()) {
        _lineEdit->setAnimation(0);
    }
}


void
Path_KnobGui::reflectExpressionState(int /*dimension*/,bool hasExpr)
{
    boost::shared_ptr<Path_Knob> knob = _knob.lock();
    if (!knob->isMultiPath()) {
        bool isSlaved = knob->isSlave(0);
        _lineEdit->setAnimation(3);
        _lineEdit->setReadOnly(hasExpr || isSlaved);
        _openFileButton->setEnabled(!hasExpr && !isSlaved);
    }
}

void
Path_KnobGui::updateToolTip()
{
    if (hasToolTip()) {
        QString tt = toolTip();
        if (!_knob.lock()->isMultiPath()) {
            _lineEdit->setToolTip(tt);
        } else {
            _table->setToolTip(tt);
        }
    }
}