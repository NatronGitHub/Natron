/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#include "KnobGuiFile.h"

#include <stdexcept>

#include <QFormLayout> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QHBoxLayout> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QtCore/QDebug>
#include <QItemSelectionModel>
#include <QHeaderView>
#include <QPainter>
#include <QStyle>
#include <QApplication>
#include <QStyledItemDelegate>
#include <QAction>

#include "Engine/EffectInstance.h"
#include "Engine/KnobFile.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewIdx.h"

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

#include <SequenceParsing.h>

NATRON_NAMESPACE_ENTER


//===========================FILE_KNOB_GUI=====================================
KnobGuiFile::KnobGuiFile(KnobIPtr knob,
                         KnobGuiContainerI *container)
    : KnobGui(knob, container)
    , _lineEdit(0)
    , _openFileButton(0)
    , _reloadButton(0)
    , _lastOpened()
    , _lastModificationDates()
{
    KnobFilePtr k = boost::dynamic_pointer_cast<KnobFile>(knob);

    assert(k);
    QObject::connect( k.get(), SIGNAL(openFile()), this, SLOT(open_file()) );
    _knob = k;
}

KnobGuiFile::~KnobGuiFile()
{
}

void
KnobGuiFile::removeSpecificGui()
{
    _lineEdit->deleteLater();
    _openFileButton->deleteLater();
}

void
KnobGuiFile::createWidget(QHBoxLayout* layout)
{
    KnobFilePtr knob = _knob.lock();
    EffectInstance* holderIsEffect = dynamic_cast<EffectInstance*>( knob->getHolder() );

    if ( holderIsEffect && holderIsEffect->isReader() && (knob->getName() == kOfxImageEffectFileParamName) ) {
        TimeLinePtr timeline = getGui()->getApp()->getTimeLine();
        QObject::connect( timeline.get(), SIGNAL(frameChanged(SequenceTime,int)), this, SLOT(onTimelineFrameChanged(SequenceTime,int)) );
    }


    _lineEdit = new LineEdit( layout->parentWidget() );
    //layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    _lineEdit->setPlaceholderText( tr("File path...") );
    _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_lineEdit, 0);

    QObject::connect( _lineEdit, SIGNAL(editingFinished()), this, SLOT(onTextEdited()) );


    _openFileButton = new Button( layout->parentWidget() );
    _openFileButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QPixmap pix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
    _openFileButton->setIcon( QIcon(pix) );
    _openFileButton->setToolTip( toolTip() );
    _openFileButton->setFocusPolicy(Qt::NoFocus); // exclude from tab focus
    QObject::connect( _openFileButton, SIGNAL(clicked()), this, SLOT(onButtonClicked()) );

    layout->addWidget(_lineEdit);
    layout->addWidget(_openFileButton);

    if ( knob && knob->getHolder() ) {
        _reloadButton = new Button( layout->parentWidget() );
        _reloadButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _reloadButton->setFocusPolicy(Qt::NoFocus);
        QPixmap pixRefresh;
        appPTR->getIcon(NATRON_PIXMAP_VIEWER_REFRESH, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixRefresh);
        _reloadButton->setIcon( QIcon(pixRefresh) );
        _reloadButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Reload the file."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        QObject::connect( _reloadButton, SIGNAL(clicked()), this, SLOT(onReloadClicked()) );
        layout->addWidget(_reloadButton);
    }
}

void
KnobGuiFile::onButtonClicked()
{
    open_file();
}

void
KnobGuiFile::onReloadClicked()
{
    if (_reloadButton) {
        KnobFilePtr knob = _knob.lock();
        if (knob) {
            knob->reloadFile();
        }
    }
}

void
KnobGuiFile::open_file()
{
    std::vector<std::string> filters;
    KnobFilePtr knob = _knob.lock();

    if ( !knob->isInputImageFile() ) {
        filters.push_back("*");
    } else {
        appPTR->getSupportedReaderFileFormats(&filters);
    }
    std::string oldPattern = knob->getValue();
    std::string currentPattern = oldPattern;
    std::string path = SequenceParsing::removePath(currentPattern);
    QString pathWhereToOpen;
    if ( path.empty() ) {
        pathWhereToOpen = _lastOpened;
    } else {
        pathWhereToOpen = QString::fromUtf8( path.c_str() );
    }

    SequenceFileDialog dialog( _lineEdit->parentWidget(), filters, knob->isInputImageFile(),
                               SequenceFileDialog::eFileDialogModeOpen, pathWhereToOpen.toStdString(), getGui(), true);

    if ( dialog.exec() ) {
        std::string selectedFile = dialog.selectedFiles();
        std::string originalSelectedFile = selectedFile;
        path = SequenceParsing::removePath(selectedFile);
        updateLastOpened( QString::fromUtf8( path.c_str() ) );

        pushUndoCommand( new KnobUndoCommand<std::string>(shared_from_this(), oldPattern, originalSelectedFile) );
    }
}

void
KnobGuiFile::updateLastOpened(const QString &str)
{
    std::string unpathed = str.toStdString();

    _lastOpened = QString::fromUtf8( SequenceParsing::removePath(unpathed).c_str() );
    getGui()->updateLastSequenceOpenedPath(_lastOpened);
}

void
KnobGuiFile::updateGUI(int /*dimension*/)
{
    KnobFilePtr knob = _knob.lock();

    _lineEdit->setText( QString::fromUtf8( knob->getValue().c_str() ) );

    bool useNotifications = appPTR->getCurrentSettings()->notifyOnFileChange();
    if ( useNotifications && knob->getHolder() && knob->getEvaluateOnChange() ) {
        std::string newValue = knob->getFileName( knob->getCurrentTime(), ViewIdx(0) );
        if ( knob->getHolder()->getApp() ) {
            knob->getHolder()->getApp()->getProject()->canonicalizePath(newValue);
        }
        QString file( QString::fromUtf8( newValue.c_str() ) );

        //The sequence probably changed, clear modification dates
        _lastModificationDates.clear();

        if ( QFile::exists(file) ) {
            //If the file exists at the current time, set the modification date in the tooltip
            QFileInfo info(file);
            QDateTime dateTime = info.lastModified();

            _lastModificationDates[newValue] = dateTime;

            QString tt = toolTip();
            tt.append( QString::fromUtf8("\n\nLast modified: ") );
            tt.append( dateTime.toString(Qt::SystemLocaleShortDate) );
            _lineEdit->setToolTip(tt);
        }
    }
}

void
KnobGuiFile::onTimelineFrameChanged(SequenceTime time,
                                    int /*reason*/)
{
    checkFileModificationAndWarnInternal(false, time, false);
}

bool
KnobGuiFile::checkFileModificationAndWarnInternal(bool doCheck,
                                                  SequenceTime time,
                                                  bool errorAndAbortRender)
{
    bool useNotifications = appPTR->getCurrentSettings()->notifyOnFileChange();

    if (!useNotifications) {
        return false;
    }
    KnobFilePtr knob = _knob.lock();
    if (!knob) {
        return false;
    }
    EffectInstance* effect = dynamic_cast<EffectInstance*>( knob->getHolder() );
    assert(effect);
    if ( !effect || !effect->getNode()->isActivated() ) {
        return false;
    }

    ///Get the current file, if it exists, add the file path to the file system watcher
    ///to get notified if the file changes.
    std::string filepath = knob->getFileName( time, knob->getCurrentView() );
    if ( !filepath.empty() && knob->getHolder() && knob->getHolder()->getApp() ) {
        knob->getHolder()->getApp()->getProject()->canonicalizePath(filepath);
    }

    QString qfilePath = QString::fromUtf8( filepath.c_str() );
    if ( !QFile::exists(qfilePath) ) {
        return false;
    }

    QDateTime date;
    std::map<std::string, QDateTime>::iterator foundModificationDate = _lastModificationDates.find(filepath);
    QFileInfo info(qfilePath);
    date = info.lastModified();

    //We already have a modification date
    bool ret = false;
    if ( foundModificationDate != _lastModificationDates.end() ) {
        if ( doCheck && (date != foundModificationDate->second) ) {
            if (errorAndAbortRender) {
                QString warn = tr("The file \"%1\" has changed on disk.\n"
                                  "Press reload file to load the new version of the file").arg(qfilePath);
                effect->setPersistentMessage( eMessageTypeError, warn.toStdString() );
                effect->abortAnyEvaluation();
            }
            effect->purgeCaches();
            effect->getNode()->removeAllImagesFromCache(true);
            _lastModificationDates.clear();
            ret = true;
        } else {
            return false;
        }
    }
    _lastModificationDates.insert( std::make_pair(filepath, date) );

    return ret;
} // KnobGuiFile::checkFileModificationAndWarnInternal

bool
KnobGuiFile::checkFileModificationAndWarn(SequenceTime time,
                                          bool errorAndAbortRender)
{
    return checkFileModificationAndWarnInternal(true, time, errorAndAbortRender);
}

void
KnobGuiFile::onTextEdited()
{
    if (!_lineEdit) {
        return;
    }
    std::string str = _lineEdit->text().toStdString();

    ///don't do antyhing if the pattern is the same
    KnobFilePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    std::string oldValue = knob->getValue();

    if (str == oldValue) {
        return;
    }

//
//    if (_knob->getHolder() && _knob->getHolder()->getApp()) {
//        std::map<std::string,std::string> envvar;
//        _knob->getHolder()->getApp()->getProject()->getEnvironmentVariables(envvar);
//        Project::findReplaceVariable(envvar,str);
//    }


    pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), oldValue, str ) );
}

void
KnobGuiFile::_hide()
{
    _openFileButton->hide();
    _lineEdit->hide();
}

void
KnobGuiFile::_show()
{
    _openFileButton->show();
    _lineEdit->show();
}

void
KnobGuiFile::setEnabled()
{
    bool enabled = getKnob()->isEnabled(0);

    _openFileButton->setEnabled(enabled);
    _lineEdit->setReadOnly_NoFocusRect(!enabled);
}

void
KnobGuiFile::setReadOnly(bool readOnly,
                         int /*dimension*/)
{
    _openFileButton->setEnabled(!readOnly);
    _lineEdit->setReadOnly_NoFocusRect(readOnly);
}

void
KnobGuiFile::setDirty(bool dirty)
{
    _lineEdit->setDirty(dirty);
}

KnobIPtr
KnobGuiFile::getKnob() const
{
    return _knob.lock();
}

void
KnobGuiFile::addRightClickMenuEntries(QMenu* menu)
{
    QAction* makeAbsoluteAction = new QAction(tr("Make absolute"), menu);
    QObject::connect( makeAbsoluteAction, SIGNAL(triggered()), this, SLOT(onMakeAbsoluteTriggered()) );

    makeAbsoluteAction->setToolTip( tr("Make the file-path absolute if it was previously relative to any project path") );
    menu->addAction(makeAbsoluteAction);
    QAction* makeRelativeToProject = new QAction(tr("Make relative to project"), menu);
    makeRelativeToProject->setToolTip( tr("Make the file-path relative to the [Project] path") );
    QObject::connect( makeRelativeToProject, SIGNAL(triggered()), this, SLOT(onMakeRelativeTriggered()) );
    menu->addAction(makeRelativeToProject);
    QAction* simplify = new QAction(tr("Simplify"), menu);
    QObject::connect( simplify, SIGNAL(triggered()), this, SLOT(onSimplifyTriggered()) );
    simplify->setToolTip( tr("Same as make relative but will pick the longest project variable to simplify") );
    menu->addAction(simplify);

    menu->addSeparator();
    QMenu* qtMenu = _lineEdit->createStandardContextMenu();
    qtMenu->setFont( QApplication::font() ); // necessary
    qtMenu->setTitle( tr("Edit") );
    menu->addMenu(qtMenu);
}

void
KnobGuiFile::onMakeAbsoluteTriggered()
{
    KnobFilePtr knob = _knob.lock();

    if ( knob->getHolder() && knob->getHolder()->getApp() ) {
        std::string oldValue = knob->getValue();
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->canonicalizePath(newValue);
        pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), oldValue, newValue ) );
    }
}

void
KnobGuiFile::onMakeRelativeTriggered()
{
    KnobFilePtr knob = _knob.lock();

    if ( knob->getHolder() && knob->getHolder()->getApp() ) {
        std::string oldValue = knob->getValue();
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->makeRelativeToProject(newValue);
        pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), oldValue, newValue ) );
    }
}

void
KnobGuiFile::onSimplifyTriggered()
{
    KnobFilePtr knob = _knob.lock();

    if ( knob->getHolder() && knob->getHolder()->getApp() ) {
        std::string oldValue = knob->getValue();
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->simplifyPath(newValue);
        pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), oldValue, newValue ) );
    }
}

void
KnobGuiFile::reflectAnimationLevel(int /*dimension*/,
                                   AnimationLevelEnum /*level*/)
{
    _lineEdit->setAnimation(0);
}

void
KnobGuiFile::reflectExpressionState(int /*dimension*/,
                                    bool hasExpr)
{
    KnobFilePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    bool isEnabled = knob->isEnabled(0);

    _lineEdit->setAnimation(3);
    _lineEdit->setReadOnly_NoFocusRect(hasExpr || !isEnabled);
    _openFileButton->setEnabled(!hasExpr || isEnabled);
}

void
KnobGuiFile::updateToolTip()
{
    if ( hasToolTip() ) {
        QString tt = toolTip();
        _lineEdit->setToolTip(tt);
    }
}

//============================OUTPUT_FILE_KNOB_GUI====================================
KnobGuiOutputFile::KnobGuiOutputFile(KnobIPtr knob,
                                     KnobGuiContainerI *container)
    : KnobGui(knob, container)
    , _lineEdit(0)
    , _openFileButton(0)
    , _rewriteButton(0)
{
    KnobOutputFilePtr k = boost::dynamic_pointer_cast<KnobOutputFile>(knob);

    assert(k);
    QObject::connect( k.get(), SIGNAL(openFile(bool)), this, SLOT(open_file(bool)) );
    _knob = k;
}

KnobGuiOutputFile::~KnobGuiOutputFile()
{
}

void
KnobGuiOutputFile::removeSpecificGui()
{
    _lineEdit->deleteLater();
    _openFileButton->deleteLater();
}

void
KnobGuiOutputFile::createWidget(QHBoxLayout* layout)
{
    KnobOutputFilePtr knob = _knob.lock();

    _lineEdit = new LineEdit( layout->parentWidget() );
    //layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    _lineEdit->setPlaceholderText( tr("File path...") );
    _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_lineEdit, 0);

    QObject::connect( _lineEdit, SIGNAL(editingFinished()), this, SLOT(onTextEdited()) );


    _openFileButton = new Button( layout->parentWidget() );
    _openFileButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QPixmap pix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
    _openFileButton->setIcon( QIcon(pix) );
    _openFileButton->setToolTip( toolTip() );
    _openFileButton->setFocusPolicy(Qt::NoFocus); // exclude from tab focus
    QObject::connect( _openFileButton, SIGNAL(clicked()), this, SLOT(onButtonClicked()) );

    layout->addWidget(_lineEdit);
    layout->addWidget(_openFileButton);

    if ( knob && knob->getHolder() ) {
        _rewriteButton = new Button( layout->parentWidget() );
        _rewriteButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _rewriteButton->setFocusPolicy(Qt::NoFocus);
        QPixmap pixRefresh;
        appPTR->getIcon(NATRON_PIXMAP_VIEWER_REFRESH, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pixRefresh);
        _rewriteButton->setIcon( QIcon(pixRefresh) );
        _rewriteButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Rewrite the file."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        QObject::connect( _rewriteButton, SIGNAL(clicked()), this, SLOT(onRewriteClicked()) );
        layout->addWidget(_rewriteButton);
    }
}

void
KnobGuiOutputFile::onButtonClicked()
{
    KnobOutputFilePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    open_file( knob->isSequencesDialogEnabled() );
}

void
KnobGuiOutputFile::onRewriteClicked()
{
    if (_rewriteButton) {
        KnobOutputFilePtr knob = _knob.lock();
        if (knob) {
            knob->rewriteFile();
        }
    }
}

void
KnobGuiOutputFile::open_file(bool openSequence)
{
    KnobOutputFilePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    std::vector<std::string> filters;

    if ( !knob->isOutputImageFile() ) {
        filters.push_back("*");
    } else {
        appPTR->getSupportedWriterFileFormats(&filters);
    }

    SequenceFileDialog dialog( _lineEdit->parentWidget(), filters, openSequence, SequenceFileDialog::eFileDialogModeSave, _lastOpened.toStdString(), getGui(), true);
    if ( dialog.exec() ) {
        std::string oldPattern = _lineEdit->text().toStdString();
        std::string newPattern = dialog.filesToSave();
        updateLastOpened( QString::fromUtf8( SequenceParsing::removePath(oldPattern).c_str() ) );

        pushUndoCommand( new KnobUndoCommand<std::string>(shared_from_this(), oldPattern, newPattern) );
    }
}

void
KnobGuiOutputFile::updateLastOpened(const QString &str)
{
    std::string withoutPath = str.toStdString();

    _lastOpened = QString::fromUtf8( SequenceParsing::removePath(withoutPath).c_str() );
    getGui()->updateLastSequenceSavedPath(_lastOpened);
}

void
KnobGuiOutputFile::updateGUI(int /*dimension*/)
{
    KnobOutputFilePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    _lineEdit->setText( QString::fromUtf8( knob->getValue().c_str() ) );
}

void
KnobGuiOutputFile::onTextEdited()
{
    if (!_lineEdit) {
        return;
    }
    std::string newPattern = _lineEdit->text().toStdString();

//    if (_knob->getHolder() && _knob->getHolder()->getApp()) {
//        std::map<std::string,std::string> envvar;
//        _knob->getHolder()->getApp()->getProject()->getEnvironmentVariables(envvar);
//        Project::findReplaceVariable(envvar,newPattern);
//    }
//
//
    KnobOutputFilePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), knob->getValue(), newPattern ) );
}

void
KnobGuiOutputFile::_hide()
{
    _openFileButton->hide();
    _lineEdit->hide();
}

void
KnobGuiOutputFile::_show()
{
    _openFileButton->show();
    _lineEdit->show();
}

void
KnobGuiOutputFile::setEnabled()
{
    bool enabled = getKnob()->isEnabled(0);

    _openFileButton->setEnabled(enabled);
    _lineEdit->setReadOnly_NoFocusRect(!enabled);
}

void
KnobGuiOutputFile::setReadOnly(bool readOnly,
                               int /*dimension*/)
{
    _openFileButton->setEnabled(!readOnly);
    _lineEdit->setReadOnly_NoFocusRect(readOnly);
}

void
KnobGuiOutputFile::setDirty(bool dirty)
{
    _lineEdit->setDirty(dirty);
}

KnobIPtr
KnobGuiOutputFile::getKnob() const
{
    return _knob.lock();
}

void
KnobGuiOutputFile::addRightClickMenuEntries(QMenu* menu)
{
    QAction* makeAbsoluteAction = new QAction(tr("Make absolute"), menu);
    QObject::connect( makeAbsoluteAction, SIGNAL(triggered()), this, SLOT(onMakeAbsoluteTriggered()) );

    makeAbsoluteAction->setToolTip( tr("Make the file-path absolute if it was previously relative to any project path") );
    menu->addAction(makeAbsoluteAction);
    QAction* makeRelativeToProject = new QAction(tr("Make relative to project"), menu);
    makeRelativeToProject->setToolTip( tr("Make the file-path relative to the [Project] path") );
    QObject::connect( makeRelativeToProject, SIGNAL(triggered()), this, SLOT(onMakeRelativeTriggered()) );
    menu->addAction(makeRelativeToProject);
    QAction* simplify = new QAction(tr("Simplify"), menu);
    QObject::connect( simplify, SIGNAL(triggered()), this, SLOT(onSimplifyTriggered()) );
    simplify->setToolTip( tr("Same as make relative but will pick the longest project variable to simplify") );
    menu->addAction(simplify);

    menu->addSeparator();
    QMenu* qtMenu = _lineEdit->createStandardContextMenu();
    qtMenu->setFont( QApplication::font() ); // necessary
    qtMenu->setTitle( tr("Edit") );
    menu->addMenu(qtMenu);
}

void
KnobGuiOutputFile::onMakeAbsoluteTriggered()
{
    KnobOutputFilePtr knob = _knob.lock();

    if ( knob->getHolder() && knob->getHolder()->getApp() ) {
        std::string oldValue = knob->getValue();
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->canonicalizePath(newValue);
        pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), oldValue, newValue ) );
    }
}

void
KnobGuiOutputFile::onMakeRelativeTriggered()
{
    KnobOutputFilePtr knob = _knob.lock();

    if ( knob->getHolder() && knob->getHolder()->getApp() ) {
        std::string oldValue = knob->getValue();
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->makeRelativeToProject(newValue);
        pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), oldValue, newValue ) );
    }
}

void
KnobGuiOutputFile::onSimplifyTriggered()
{
    KnobOutputFilePtr knob = _knob.lock();

    if ( knob->getHolder() && knob->getHolder()->getApp() ) {
        std::string oldValue = knob->getValue();
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->simplifyPath(newValue);
        pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), oldValue, newValue ) );
    }
}

void
KnobGuiOutputFile::reflectExpressionState(int /*dimension*/,
                                          bool hasExpr)
{
    KnobOutputFilePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    bool isEnabled = knob->isEnabled(0);

    _lineEdit->setAnimation(3);
    _lineEdit->setReadOnly_NoFocusRect(hasExpr || !isEnabled);
    _openFileButton->setEnabled(!hasExpr || isEnabled);
}

void
KnobGuiOutputFile::reflectAnimationLevel(int /*dimension*/,
                                         AnimationLevelEnum /*level*/)
{
    _lineEdit->setAnimation(0);
}

void
KnobGuiOutputFile::updateToolTip()
{
    if ( hasToolTip() ) {
        QString tt = toolTip();
        if (_lineEdit) {
            _lineEdit->setToolTip(tt);
        }
    }
}

//============================PATH_KNOB_GUI====================================
KnobGuiPath::KnobGuiPath(KnobIPtr knob,
                         KnobGuiContainerI *container)
    : KnobGuiTable(knob, container)
    , _mainContainer(0)
    , _lineEdit(0)
    , _openFileButton(0)
{
    _knob = boost::dynamic_pointer_cast<KnobPath>(knob);
    assert( _knob.lock() );
}

KnobGuiPath::~KnobGuiPath()
{
}

void
KnobGuiPath::removeSpecificGui()
{
    if (_mainContainer) {
        _mainContainer->deleteLater();
    }
    KnobGuiTable::removeSpecificGui();
}

void
KnobGuiPath::createWidget(QHBoxLayout* layout)
{
    KnobPathPtr knob = _knob.lock();

    if ( knob->isMultiPath() ) {
        KnobGuiTable::createWidget(layout);
    } else { // _knob->isMultiPath()
        _mainContainer = new QWidget( layout->parentWidget() );
        QHBoxLayout* mainLayout = new QHBoxLayout(_mainContainer);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        _lineEdit = new LineEdit(_mainContainer);
        _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QObject::connect( _lineEdit, SIGNAL(editingFinished()), this, SLOT(onTextEdited()) );

        enableRightClickMenu(_lineEdit, 0);
        _openFileButton = new Button( layout->parentWidget() );
        _openFileButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _openFileButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Click to select a path to append to/replace this variable."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        QPixmap pix;
        appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
        _openFileButton->setIcon( QIcon(pix) );
        QObject::connect( _openFileButton, SIGNAL(clicked()), this, SLOT(onOpenFileButtonClicked()) );

        mainLayout->addWidget(_lineEdit);
        mainLayout->addWidget(_openFileButton);
        layout->addWidget(_mainContainer);
    }
}

void
KnobGuiPath::onOpenFileButtonClicked()
{
    std::vector<std::string> filters;
    SequenceFileDialog dialog( _mainContainer, filters, false, SequenceFileDialog::eFileDialogModeDir, _lastOpened.toStdString(), getGui(), true );

    if ( dialog.exec() ) {
        std::string dirPath = dialog.selectedDirectory();
        updateLastOpened( QString::fromUtf8( dirPath.c_str() ) );

        KnobPathPtr knob = _knob.lock();
        if (!knob) {
            return;
        }
        std::string oldValue = knob->getValue();

        pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), oldValue, dirPath ) );
    }
}

bool
KnobGuiPath::addNewUserEntry(QStringList& row)
{
    std::vector<std::string> filters;
    SequenceFileDialog dialog( getGui(), filters, false, SequenceFileDialog::eFileDialogModeDir, _lastOpened.toStdString(), getGui(), true);

    if ( dialog.exec() ) {
        std::string dirPath = dialog.selectedDirectory();
        if ( !dirPath.empty() && (dirPath[dirPath.size() - 1] == '/') ) {
            dirPath.erase(dirPath.size() - 1, 1);
        }
        QString path = QString::fromUtf8( dirPath.c_str() );
        updateLastOpened(path);

        int rc = rowCount();
        QString varName = QString( tr("Path") + QString::fromUtf8("%1") ).arg(rc);
        row.push_back(varName);
        row.push_back(path);

        return true;
    }

    return false;
}

bool
KnobGuiPath::editUserEntry(QStringList& row)
{
    std::vector<std::string> filters;
    SequenceFileDialog dialog(getGui(), filters, false, SequenceFileDialog::eFileDialogModeDir, row[1].toStdString(), getGui(), true );

    if ( dialog.exec() ) {
        std::string dirPath = dialog.selectedDirectory();
        if ( !dirPath.empty() && (dirPath[dirPath.size() - 1] == '/') ) {
            dirPath.erase(dirPath.size() - 1, 1);
        }
        QString path = QString::fromUtf8( dirPath.c_str() );
        updateLastOpened(path);
        row[1] = path;

        return true;
    }

    return false;
}

void
KnobGuiPath::entryRemoved(const QStringList& row)
{
    KnobPathPtr knob = _knob.lock();

    ///Fix all variables if needed
    if ( knob && knob->getHolder() && ( knob->getHolder() == getGui()->getApp()->getProject().get() ) &&
         appPTR->getCurrentSettings()->isAutoFixRelativeFilePathEnabled() ) {
        getGui()->getApp()->getProject()->fixRelativeFilePaths(row[0].toStdString(), std::string(), false);
    }
}

void
KnobGuiPath::tableChanged(int row,
                          int col,
                          std::string* newEncodedValue)
{
    KnobTablePtr knob = boost::dynamic_pointer_cast<KnobTable>( getKnob() );

    assert(knob);

    if ( knob->getHolder() && knob->getHolder()->isProject() &&
         appPTR->getCurrentSettings()->isAutoFixRelativeFilePathEnabled() ) {
        if (col == 0) {
            std::list<std::vector<std::string> > oldTable, newTable;
            knob->decodeFromKnobTableFormat(knob->getValue(), &oldTable);
            knob->decodeFromKnobTableFormat(*newEncodedValue, &newTable);


            ///Compare the 2 maps to find-out if a path has changed or just a name
            if ( ( oldTable.size() == newTable.size() ) && ( row < (int)oldTable.size() ) ) {
                std::list<std::vector<std::string> >::iterator itOld = oldTable.begin();
                std::list<std::vector<std::string> >::iterator itNew = newTable.begin();
                std::advance(itOld, row);
                std::advance(itNew, row);


                if ( (*itOld)[0] != (*itNew)[0] ) {
                    ///a name has changed
                    getGui()->getApp()->getProject()->fixPathName( (*itOld)[0], (*itNew)[0] );
                } else if ( (*itOld)[1] != (*itNew)[1] ) {
                    getGui()->getApp()->getProject()->fixRelativeFilePaths( (*itOld)[0], (*itNew)[1], false );
                }
            }
        }
    }
}

void
KnobGuiPath::onTextEdited()
{
    if (!_lineEdit) {
        return;
    }
    std::string dirPath = _lineEdit->text().toStdString();

    if ( !dirPath.empty() && (dirPath[dirPath.size() - 1] == '/') ) {
        dirPath.erase(dirPath.size() - 1, 1);
    }
    updateLastOpened( QString::fromUtf8( dirPath.c_str() ) );

    KnobPathPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    std::string oldValue = knob->getValue();

    pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), oldValue, dirPath ) );
}

void
KnobGuiPath::updateLastOpened(const QString &str)
{
    std::string withoutPath = str.toStdString();

    _lastOpened = QString::fromUtf8( SequenceParsing::removePath(withoutPath).c_str() );
}

void
KnobGuiPath::updateGUI(int dimension)
{
    KnobPathPtr knob = _knob.lock();

    if ( !knob->isMultiPath() ) {
        std::string value = knob->getValue();
        _lineEdit->setText( QString::fromUtf8( value.c_str() ) );
    } else {
        KnobGuiTable::updateGUI(dimension);
    }
}

void
KnobGuiPath::_hide()
{
    if (_mainContainer) {
        _mainContainer->hide();
    }
    KnobGuiTable::_hide();
}

void
KnobGuiPath::_show()
{
    if (_mainContainer) {
        _mainContainer->show();
    }
    KnobGuiTable::_show();
}

void
KnobGuiPath::setEnabled()
{
    KnobPathPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    if ( knob->isMultiPath() ) {
        KnobGuiTable::setEnabled();
    } else {
        bool enabled = getKnob()->isEnabled(0);
        _lineEdit->setReadOnly_NoFocusRect(!enabled);
        _openFileButton->setEnabled(enabled);
    }
}

void
KnobGuiPath::setReadOnly(bool readOnly,
                         int dimension)
{
    KnobPathPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    if ( knob->isMultiPath() ) {
        KnobGuiTable::setReadOnly(readOnly, dimension);
    } else {
        _lineEdit->setReadOnly_NoFocusRect(readOnly);
        _openFileButton->setEnabled(!readOnly);
    }
}

void
KnobGuiPath::setDirty(bool /*dirty*/)
{
}

KnobIPtr
KnobGuiPath::getKnob() const
{
    return _knob.lock();
}

void
KnobGuiPath::addRightClickMenuEntries(QMenu* menu)
{
    KnobPathPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    if ( !knob->isMultiPath() ) {
        QAction* makeAbsoluteAction = new QAction(tr("Make absolute"), menu);
        QObject::connect( makeAbsoluteAction, SIGNAL(triggered()), this, SLOT(onMakeAbsoluteTriggered()) );
        makeAbsoluteAction->setToolTip( tr("Make the file-path absolute if it was previously relative to any project path") );
        menu->addAction(makeAbsoluteAction);
        QAction* makeRelativeToProject = new QAction(tr("Make relative to project"), menu);
        makeRelativeToProject->setToolTip( tr("Make the file-path relative to the [Project] path") );
        QObject::connect( makeRelativeToProject, SIGNAL(triggered()), this, SLOT(onMakeRelativeTriggered()) );
        menu->addAction(makeRelativeToProject);
        QAction* simplify = new QAction(tr("Simplify"), menu);
        QObject::connect( simplify, SIGNAL(triggered()), this, SLOT(onSimplifyTriggered()) );
        simplify->setToolTip( tr("Same as make relative but will pick the longest project variable to simplify") );
        menu->addAction(simplify);

        menu->addSeparator();
        QMenu* qtMenu = _lineEdit->createStandardContextMenu();
        qtMenu->setFont( QApplication::font() ); // necessary
        qtMenu->setTitle( tr("Edit") );
        menu->addMenu(qtMenu);
    }
}

void
KnobGuiPath::onMakeAbsoluteTriggered()
{
    KnobPathPtr knob = _knob.lock();

    if ( knob->getHolder() && knob->getHolder()->getApp() ) {
        std::string oldValue = knob->getValue();
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->canonicalizePath(newValue);
        pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), oldValue, newValue ) );
    }
}

void
KnobGuiPath::onMakeRelativeTriggered()
{
    KnobPathPtr knob = _knob.lock();

    if ( knob->getHolder() && knob->getHolder()->getApp() ) {
        std::string oldValue = knob->getValue();
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->makeRelativeToProject(newValue);
        pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), oldValue, newValue ) );
    }
}

void
KnobGuiPath::onSimplifyTriggered()
{
    KnobPathPtr knob = _knob.lock();

    if ( knob->getHolder() && knob->getHolder()->getApp() ) {
        std::string oldValue = knob->getValue();
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->simplifyPath(newValue);
        pushUndoCommand( new KnobUndoCommand<std::string>( shared_from_this(), oldValue, newValue ) );
    }
}

void
KnobGuiPath::reflectAnimationLevel(int /*dimension*/,
                                   AnimationLevelEnum /*level*/)
{
    KnobPathPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    if ( !knob->isMultiPath() ) {
        _lineEdit->setAnimation(0);
    }
}

void
KnobGuiPath::reflectExpressionState(int /*dimension*/,
                                    bool hasExpr)
{
    KnobPathPtr knob = _knob.lock();

    if ( !knob->isMultiPath() ) {
        bool isEnabled = knob->isEnabled(0);
        _lineEdit->setAnimation(3);
        _lineEdit->setReadOnly_NoFocusRect(hasExpr || !isEnabled);
        _openFileButton->setEnabled(!hasExpr || isEnabled);
    }
}

void
KnobGuiPath::updateToolTip()
{
    if ( hasToolTip() ) {
        QString tt = toolTip();
        KnobPathPtr knob = _knob.lock();
        if (!knob) {
            return;
        }
        if ( !knob->isMultiPath() ) {
            _lineEdit->setToolTip(tt);
        } else {
            KnobGuiTable::updateToolTip();
        }
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_KnobGuiFile.cpp"
