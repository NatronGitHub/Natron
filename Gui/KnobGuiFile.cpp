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
#include "Engine/KnobUndoCommand.h"
#include "Engine/Settings.h"
#include "Engine/Image.h"
#include "Engine/TimeLine.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewIdx.h"

#include "Gui/Button.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/LineEdit.h"
#include "Gui/KnobGui.h"
#include "Gui/Menu.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/TableModelView.h"

#include <SequenceParsing.h> // for SequenceParsing::removePath

#define kNatronPersistentWarningFileOutOfDate "NatronPersistentWarningFileOutOfDate"

NATRON_NAMESPACE_ENTER


KnobGuiFile::KnobGuiFile(const KnobGuiPtr& knob, ViewIdx view)
    : KnobGuiWidgets(knob, view)
    , _lineEdit(0)
    , _openFileButton(0)
    , _reloadButton(0)
    , _lastOpened()
    , _lastModificationDates()
{
    KnobFilePtr k = toKnobFile(knob->getKnob());

    assert(k);
    QObject::connect( k.get(), SIGNAL(openFile()), this, SLOT(open_file()) );
    _knob = k;
}

KnobGuiFile::~KnobGuiFile()
{
}


void
KnobGuiFile::createWidget(QHBoxLayout* layout)
{
    KnobGuiPtr knobUI = getKnobGui();
    if (!knobUI) {
        return;
    }
    Gui* gui = knobUI->getGui();
    if (!gui) {
        return;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return;
    }
    KnobFilePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    EffectInstancePtr holderIsEffect = toEffectInstance( knob->getHolder() );


    if ( holderIsEffect && holderIsEffect->isReader() && (knob->getName() == kOfxImageEffectFileParamName) ) {

        TimeLinePtr timeline = app->getTimeLine();
        QObject::connect( timeline.get(), SIGNAL(frameChanged(SequenceTime,int)), this, SLOT(onTimelineFrameChanged(SequenceTime,int)) );
    }


    _lineEdit = new LineEdit( layout->parentWidget() );
    //layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    _lineEdit->setPlaceholderText( tr("File path...") );
    _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    ///set the copy/link actions in the right click menu
    KnobGuiWidgets::enableRightClickMenu(knobUI, _lineEdit, DimIdx(0), getView());

    QObject::connect( _lineEdit, SIGNAL(editingFinished()), this, SLOT(onTextEdited()) );


    _openFileButton = new Button( layout->parentWidget() );
    _openFileButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QPixmap pix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, NATRON_MEDIUM_BUTTON_ICON_SIZE, &pix);
    _openFileButton->setIcon( QIcon(pix) );
    knobUI->toolTip(_openFileButton, getView());
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
        KnobFile::KnobFileDialogTypeEnum type = knob->getDialogType();
        bool existing = ( (type == KnobFile::eKnobFileDialogTypeOpenFile) ||
                          (type == KnobFile::eKnobFileDialogTypeOpenFileSequences) );
        _reloadButton->setToolTip( existing ?
                                   NATRON_NAMESPACE::convertFromPlainText(tr("Reload the file."), NATRON_NAMESPACE::WhiteSpaceNormal) :
                                   NATRON_NAMESPACE::convertFromPlainText(tr("Rewrite the file."), NATRON_NAMESPACE::WhiteSpaceNormal) );
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
        if (!knob) {
            return;
        }
        if (knob) {
            knob->reloadFile();
        }
    }
}

void
KnobGuiFile::open_file()
{
    KnobFilePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    std::string oldPattern = knob->getFileNameWithoutVariablesExpension(DimIdx(0), getView());
    std::string currentPattern = oldPattern;
    std::string path = SequenceParsing::removePath(currentPattern);
    QString pathWhereToOpen;
    if ( path.empty() ) {
        pathWhereToOpen = _lastOpened;
    } else {
        pathWhereToOpen = QString::fromUtf8( path.c_str() );
    }

    KnobFile::KnobFileDialogTypeEnum type = knob->getDialogType();

    bool useSequence = ( (type == KnobFile::eKnobFileDialogTypeOpenFileSequences) ||
                        (type == KnobFile::eKnobFileDialogTypeSaveFileSequences) );

    bool existing = ( (type == KnobFile::eKnobFileDialogTypeOpenFile) ||
                     (type == KnobFile::eKnobFileDialogTypeOpenFileSequences) );

    std::vector<std::string> filters = knob->getDialogFilters();
    if (filters.empty()) {
        filters.push_back("*");
    }

    KnobGuiPtr knobUI = getKnobGui();

    SequenceFileDialog dialog( _lineEdit->parentWidget(), filters, useSequence,
                              existing ? SequenceFileDialog::eFileDialogModeOpen : SequenceFileDialog::eFileDialogModeSave, pathWhereToOpen.toStdString(), knobUI->getGui(), true);

    if ( dialog.exec() ) {
        std::string selectedFile = dialog.selectedFiles();
        std::string originalSelectedFile = selectedFile;
        path = SequenceParsing::removePath(selectedFile);
        updateLastOpened( QString::fromUtf8( path.c_str() ) );

        knobUI->pushUndoCommand( new KnobUndoCommand<std::string>(knob, oldPattern, originalSelectedFile, DimIdx(0), getView()) );
    }
}

void
KnobGuiFile::updateLastOpened(const QString &str)
{
    std::string unpathed = str.toStdString();

    _lastOpened = QString::fromUtf8( SequenceParsing::removePath(unpathed).c_str() );

    KnobGuiPtr knobUI = getKnobGui();
    knobUI->getGui()->updateLastSequenceOpenedPath(_lastOpened);
}

void
KnobGuiFile::updateGUI()
{
    KnobFilePtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    QString filePath = QString::fromUtf8( knob->getFileNameWithoutVariablesExpension().c_str() );
    if (_lineEdit->text() == filePath) {
        return;
    }
    _lineEdit->setText(filePath);

    bool useNotifications = appPTR->getCurrentSettings()->notifyOnFileChange();
    if ( useNotifications && knob->getHolder() && knob->getEvaluateOnChange() ) {
        std::string newValue = knob->getValueAtTime( knob->getHolder()->getTimelineCurrentTime(), DimIdx(0), getView() );
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

            QString tt = getKnobGui()->toolTip(0,getView());
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
    checkFileModificationAndWarnInternal(false, TimeValue(time), false);
}

bool
KnobGuiFile::checkFileModificationAndWarnInternal(bool doCheck,
                                                  TimeValue time,
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
    EffectInstancePtr effect = toEffectInstance( knob->getHolder() );
    assert(effect);
    if (!effect ) {
        return false;
    }

    ///Get the current file, if it exists, add the file path to the file system watcher
    ///to get notified if the file changes.
    std::string filepath = knob->getValueAtTime( time, DimIdx(0), ViewIdx(0) );
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
    bool warningDisplayed = false;
    if ( foundModificationDate != _lastModificationDates.end() ) {
        if ( doCheck && (date != foundModificationDate->second) ) {
            if (errorAndAbortRender) {
                QString warn = tr("The file \"%1\" has changed on disk.\n"
                                  "Press reload file to load the new version of the file").arg(qfilePath);
                effect->getNode()->setPersistentMessage( eMessageTypeError, kNatronPersistentWarningFileOutOfDate, warn.toStdString() );
                effect->getNode()->abortAnyProcessing_non_blocking();
                warningDisplayed = true;
            }
            effect->purgeCaches_public();
            _lastModificationDates.clear();
            ret = true;
        } else {
            return false;
        }
    }
    if (!warningDisplayed) {
        effect->getNode()->clearPersistentMessage(kNatronPersistentWarningFileOutOfDate);
    }

    _lastModificationDates.insert( std::make_pair(filepath, date) );

    return ret;
} // KnobGuiFile::checkFileModificationAndWarnInternal

bool
KnobGuiFile::checkFileModificationAndWarn(SequenceTime time,
                                          bool errorAndAbortRender)
{
    return checkFileModificationAndWarnInternal(true, TimeValue(time), errorAndAbortRender);
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
    std::string oldValue = knob->getFileNameWithoutVariablesExpension(DimIdx(0), getView());

    if (str == oldValue) {
        return;
    }

//
//    if (_knob->getHolder() && _knob->getHolder()->getApp()) {
//        std::map<std::string,std::string> envvar;
//        _knob->getHolder()->getApp()->getProject()->getEnvironmentVariables(envvar);
//        Project::findReplaceVariable(envvar,str);
//    }


    KnobGuiPtr knobUI = getKnobGui();
    knobUI->pushUndoCommand( new KnobUndoCommand<std::string>( knob, oldValue, str, DimIdx(0), getView() ) );
}

void
KnobGuiFile::setWidgetsVisible(bool visible)
{
    _openFileButton->setVisible(visible);
    _lineEdit->setVisible(visible);
}

void
KnobGuiFile::setEnabled(const std::vector<bool>& perDimEnabled)
{

    _openFileButton->setEnabled(perDimEnabled[0]);
    _lineEdit->setReadOnly_NoFocusRect(!perDimEnabled[0]);
}


void
KnobGuiFile::reflectMultipleSelection(bool dirty)
{
    _lineEdit->setIsSelectedMultipleTimes(dirty);
}

void
KnobGuiFile::reflectSelectionState(bool selected)
{
    _lineEdit->setIsSelected(selected);
}

void
KnobGuiFile::reflectModificationsState()
{
    KnobFilePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    bool hasModif = knob->hasModifications();

    if (_lineEdit) {
        _lineEdit->setIsModified(hasModif);
    }
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
    if (!knob) {
        return;
    }

    if ( knob->getHolder() && knob->getHolder()->getApp() ) {
        std::string oldValue = knob->getValue(DimIdx(0), getView());
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->canonicalizePath(newValue);
        boost::shared_ptr<FileKnobDimView> data =  boost::dynamic_pointer_cast<FileKnobDimView>(knob->getDataForDimView(DimIdx(0), ViewIdx(0)));
        assert(data);
        data->setVariablesExpansionEnabled(false);
        KnobGuiPtr knobUI = getKnobGui();
        knobUI->pushUndoCommand( new KnobUndoCommand<std::string>( knob, oldValue, newValue, DimIdx(0), getView() ) );
        data->setVariablesExpansionEnabled(true);
    }
}

void
KnobGuiFile::onMakeRelativeTriggered()
{
    KnobFilePtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    if ( knob->getHolder() && knob->getHolder()->getApp() ) {
        std::string oldValue = knob->getValue(DimIdx(0), getView());
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->makeRelativeToProject(newValue);
        KnobGuiPtr knobUI = getKnobGui();
        knobUI->pushUndoCommand( new KnobUndoCommand<std::string>( knob, oldValue, newValue, DimIdx(0), getView() ) );
    }
}

void
KnobGuiFile::onSimplifyTriggered()
{
    KnobFilePtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    if ( knob->getHolder() && knob->getHolder()->getApp() ) {
        std::string oldValue = knob->getValue(DimIdx(0), getView());
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->simplifyPath(newValue);
        KnobGuiPtr knobUI = getKnobGui();
        knobUI->pushUndoCommand( new KnobUndoCommand<std::string>( knob, oldValue, newValue, DimIdx(0), getView() ) );
    }
}

void
KnobGuiFile::reflectLinkedState(DimIdx /*dimension*/, bool linked)
{
    QColor c;
    if (linked) {
        double r,g,b;
        appPTR->getCurrentSettings()->getExprColor(&r, &g, &b);
        c.setRgbF(Image::clamp(r, 0., 1.),
                  Image::clamp(g, 0., 1.),
                  Image::clamp(b, 0., 1.));
        _lineEdit->setAdditionalDecorationTypeEnabled(LineEdit::eAdditionalDecorationColoredFrame, true, c);
    } else {
        _lineEdit->setAdditionalDecorationTypeEnabled(LineEdit::eAdditionalDecorationColoredFrame, false);
    }
}

void
KnobGuiFile::reflectAnimationLevel(DimIdx /*dimension*/,
                                   AnimationLevelEnum level)
{
    KnobFilePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    bool isEnabled = knob->isEnabled();
    _lineEdit->setReadOnly_NoFocusRect(level == eAnimationLevelExpression || !isEnabled);
    _openFileButton->setEnabled(level != eAnimationLevelExpression && isEnabled);
    _lineEdit->setAnimation(level);
}

void
KnobGuiFile::updateToolTip()
{

    KnobGuiPtr knobUI = getKnobGui();
    if ( knobUI->hasToolTip() ) {
        knobUI->toolTip(_lineEdit, getView());
    }
}


KnobGuiPath::KnobGuiPath(const KnobGuiPtr& knob, ViewIdx view)
    : KnobGuiTable(knob, view)
    , _mainContainer(0)
    , _lineEdit(0)
    , _openFileButton(0)
{
    _knob = toKnobPath(knob->getKnob());
    assert( _knob.lock() );
}

KnobGuiPath::~KnobGuiPath()
{
}


void
KnobGuiPath::reflectLinkedState(DimIdx /*dimension*/, bool linked)
{
    if (!_lineEdit) {
        return;
    }
    QColor c;
    if (linked) {
        double r,g,b;
        appPTR->getCurrentSettings()->getExprColor(&r, &g, &b);
        c.setRgbF(Image::clamp(r, 0., 1.),
                  Image::clamp(g, 0., 1.),
                  Image::clamp(b, 0., 1.));
        _lineEdit->setAdditionalDecorationTypeEnabled(LineEdit::eAdditionalDecorationColoredFrame, true, c);
    } else {
        _lineEdit->setAdditionalDecorationTypeEnabled(LineEdit::eAdditionalDecorationColoredFrame, false);
    }
}

void
KnobGuiPath::createWidget(QHBoxLayout* layout)
{
    KnobPathPtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    KnobGuiPtr knobUI = getKnobGui();
    if ( knob->isMultiPath() ) {
        KnobGuiTable::createWidget(layout);
    } else { // _knob->isMultiPath()
        _mainContainer = new QWidget( layout->parentWidget() );
        QHBoxLayout* mainLayout = new QHBoxLayout(_mainContainer);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        _lineEdit = new LineEdit(_mainContainer);
        _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QObject::connect( _lineEdit, SIGNAL(editingFinished()), this, SLOT(onTextEdited()) );

        KnobGuiWidgets::enableRightClickMenu(knobUI, _lineEdit, DimIdx(0), getView());
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
    KnobPathPtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    std::string oldPath = knob->getFileNameWithoutVariablesExpension(DimIdx(0), getView());
    QString pathWhereToOpen;
    if ( oldPath.empty() ) {
        pathWhereToOpen = _lastOpened;
    } else {
        pathWhereToOpen = QString::fromUtf8( oldPath.c_str() );
    }


    KnobGuiPtr knobUI = getKnobGui();
    std::vector<std::string> filters;
    SequenceFileDialog dialog( _mainContainer, filters, false, SequenceFileDialog::eFileDialogModeDir, pathWhereToOpen.toStdString(), knobUI->getGui(), true );

    if ( dialog.exec() ) {
        std::string dirPath = dialog.selectedDirectory();
        updateLastOpened( QString::fromUtf8( dirPath.c_str() ) );

        KnobPathPtr knob = _knob.lock();
        if (!knob) {
            return;
        }
        std::string oldValue = knob->getFileNameWithoutVariablesExpension(DimIdx(0), getView());

        knobUI->pushUndoCommand( new KnobUndoCommand<std::string>( knob, oldValue, dirPath, DimIdx(0), getView() ) );
    }
}

bool
KnobGuiPath::addNewUserEntry(QStringList& row)
{
    KnobPathPtr knob = _knob.lock();
    if (!knob) {
        return false;
    }

    std::vector<std::string> filters;

    std::string oldPath = knob->getFileNameWithoutVariablesExpension(DimIdx(0), getView());
    QString pathWhereToOpen;
    if ( oldPath.empty() ) {
        pathWhereToOpen = _lastOpened;
    } else {
        pathWhereToOpen = QString::fromUtf8( oldPath.c_str() );
    }

    KnobGuiPtr knobUI = getKnobGui();
    SequenceFileDialog dialog( knobUI->getGui(), filters, false, SequenceFileDialog::eFileDialogModeDir, pathWhereToOpen.toStdString(), knobUI->getGui(), true);

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
    KnobGuiPtr knobUI = getKnobGui();
    SequenceFileDialog dialog(knobUI->getGui(), filters, false, SequenceFileDialog::eFileDialogModeDir, row[1].toStdString(), knobUI->getGui(), true );

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
    KnobGuiPtr knobUI = getKnobGui();
    if (!knobUI) {
        return;
    }
    Gui* gui = knobUI->getGui();
    if (!gui) {
        return;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return;
    }
    KnobPathPtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    ///Fix all variables if needed
    if ( knob && knob->getHolder() && ( knob->getHolder() == app->getProject() ) &&
         appPTR->getCurrentSettings()->isAutoFixRelativeFilePathEnabled() ) {
        app->getProject()->fixRelativeFilePaths(row[0].toStdString(), std::string(), false);
    }
}

void
KnobGuiPath::tableChanged(int row,
                          int col,
                          std::string* newEncodedValue)
{

    KnobGuiPtr knobUI = getKnobGui();
    if (!knobUI) {
        return;
    }
    Gui* gui = knobUI->getGui();
    if (!gui) {
        return;
    }    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return;
    }
    boost::shared_ptr<KnobTable> knob = boost::dynamic_pointer_cast<KnobTable>( _knob.lock() );

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
                    app->getProject()->fixPathName( (*itOld)[0], (*itNew)[0] );
                } else if ( (*itOld)[1] != (*itNew)[1] ) {
                    app->getProject()->fixRelativeFilePaths( (*itOld)[0], (*itNew)[1], false );

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
    std::string oldValue = knob->getFileNameWithoutVariablesExpension(DimIdx(0), getView());

    KnobGuiPtr knobUI = getKnobGui();
    knobUI->pushUndoCommand( new KnobUndoCommand<std::string>( knob, oldValue, dirPath, DimIdx(0), getView() ) );
}

void
KnobGuiPath::updateLastOpened(const QString &str)
{
    std::string withoutPath = str.toStdString();

    _lastOpened = QString::fromUtf8( SequenceParsing::removePath(withoutPath).c_str() );
}

void
KnobGuiPath::updateGUI()
{
    KnobPathPtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    if ( !knob->isMultiPath() ) {
        QString value =  QString::fromUtf8( knob->getFileNameWithoutVariablesExpension(DimIdx(0), getView()).c_str() );
        if (_lineEdit->text() == value) {
            return;
        }
        _lineEdit->setText(value);
    } else {
        KnobGuiTable::updateGUI();
    }
}

void
KnobGuiPath::setWidgetsVisible(bool visible)
{
    if (_mainContainer) {
        _mainContainer->setVisible(visible);
    }
    KnobGuiTable::setWidgetsVisible(visible);
}

void
KnobGuiPath::setEnabled(const std::vector<bool>& perDimEnabled)
{
    KnobPathPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    if ( knob->isMultiPath() ) {
        KnobGuiTable::setEnabled(perDimEnabled);
    } else {
        _lineEdit->setReadOnly_NoFocusRect(!perDimEnabled[0]);
        _openFileButton->setEnabled(perDimEnabled[0]);
    }
}


void
KnobGuiPath::reflectMultipleSelection(bool dirty)
{
    if (_lineEdit) {
        _lineEdit->setIsSelectedMultipleTimes(dirty);
    }
}


void
KnobGuiPath::reflectSelectionState(bool selected)
{
    if (_lineEdit) {
        _lineEdit->setIsSelected(selected);
    }
}

void
KnobGuiPath::reflectModificationsState()
{
    KnobPathPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    bool hasModif = knob->hasModifications();

    if (_lineEdit) {
        _lineEdit->setIsModified(hasModif);
    }
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
    if (!knob) {
        return;
    }

    if ( knob->getHolder() && knob->getHolder()->getApp() ) {
        std::string oldValue = knob->getFileNameWithoutVariablesExpension(DimIdx(0), getView());
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->canonicalizePath(newValue);
        KnobGuiPtr knobUI = getKnobGui();
        knobUI->pushUndoCommand( new KnobUndoCommand<std::string>( knob, oldValue, newValue, DimIdx(0), getView() ) );
    }
}

void
KnobGuiPath::onMakeRelativeTriggered()
{
    KnobPathPtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    if ( knob->getHolder() && knob->getHolder()->getApp() ) {
        std::string oldValue = knob->getFileNameWithoutVariablesExpension(DimIdx(0), getView());
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->makeRelativeToProject(newValue);
        KnobGuiPtr knobUI = getKnobGui();
        knobUI->pushUndoCommand( new KnobUndoCommand<std::string>( knob, oldValue, newValue, DimIdx(0), getView() ) );
    }
}

void
KnobGuiPath::onSimplifyTriggered()
{
    KnobPathPtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    if ( knob->getHolder() && knob->getHolder()->getApp() ) {
        std::string oldValue = knob->getFileNameWithoutVariablesExpension(DimIdx(0), getView());
        std::string newValue = oldValue;
        knob->getHolder()->getApp()->getProject()->simplifyPath(newValue);
        KnobGuiPtr knobUI = getKnobGui();
        knobUI->pushUndoCommand( new KnobUndoCommand<std::string>( knob, oldValue, newValue, DimIdx(0), getView() ) );
    }
}

void
KnobGuiPath::reflectAnimationLevel(DimIdx /*dimension*/,
                                   AnimationLevelEnum level)
{
    KnobPathPtr knob = _knob.lock();
    if (!knob) {
        return;
    }

    if ( !knob->isMultiPath() ) {
        _lineEdit->setAnimation(level);
        bool isEnabled = knob->isEnabled();
        _lineEdit->setReadOnly_NoFocusRect(level == eAnimationLevelExpression || !isEnabled);
        _openFileButton->setEnabled(level != eAnimationLevelExpression && isEnabled);
    }

}



void
KnobGuiPath::updateToolTip()
{
    KnobGuiPtr knobUI = getKnobGui();
    if ( knobUI && knobUI->hasToolTip() ) {
        KnobPathPtr knob = _knob.lock();
        if (!knob) {
            return;
        }
        if ( !knob->isMultiPath() ) {
            knobUI->toolTip(_lineEdit, getView());

        } else {
            KnobGuiTable::updateToolTip();
        }
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_KnobGuiFile.cpp"
