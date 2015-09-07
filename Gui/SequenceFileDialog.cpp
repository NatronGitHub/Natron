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

#include "SequenceFileDialog.h"

#if defined(Q_OS_UNIX)
#include <pwd.h>
#include <unistd.h> // for pathconf() on OS X
#elif defined(Q_OS_WIN)
#  include <QtCore/qt_windows.h>
#endif
#include <cassert>
#include <locale>
#include <algorithm>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QtGui/QPainter>
#include <QListView>
#include <QHeaderView>
#include <QCheckBox>
#include <QFileIconProvider>
#include <QFileSystemModel>
#include <QInputDialog>
#include <QSplitter>
#include <QtGui/QIcon>
#include <QDialog>
#include <QScrollBar>
#include <QFileDialog>
#include <QStyleFactory>
#include <QFileIconProvider>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QtGui/QColor>
#if QT_VERSION < 0x050000
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QStylePainter>
#include <QtGui/QStyleOptionViewItem>
#include <QtGui/QDesktopServices>
#else
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStylePainter>
#include <QtWidgets/QStyleOptionViewItem>
#include <QStandardPaths>
#endif

#include <QtCore/QEvent>
#include <QtCore/QMimeData>
#include <QtConcurrentRun>

#include <QtCore/QSettings>

#include <SequenceParsing.h>

#include "Global/QtCompat.h"
#include "Global/MemoryInfo.h"

#include "Engine/Node.h"
#include "Engine/Settings.h"
#include "Engine/KnobFile.h"
#include "Engine/Project.h"
#include "Engine/ViewerInstance.h"

#include "Gui/Button.h"
#include "Gui/LineEdit.h"
#include "Gui/ComboBox.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/Gui.h"
#include "Gui/NodeGui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"
#include "Gui/TabWidget.h"
#include "Gui/Label.h"
#include "Gui/Menu.h"
#include "Gui/Utils.h"

#include "Global/QtCompat.h" // removeFileExtension

#define FILE_DIALOG_DISABLE_ICONS

using std::make_pair;
using namespace Natron;

#if 0
static inline bool
nocase_equal_char (char c1,
                   char c2)
{
    return toupper(c1) == toupper(c2);
}

static inline bool
nocase_equal_string (const std::string &s1,
                     const std::string &s2)
{
    return s1.size() == s2.size() &&        // ensure same sizes
           std::equal (s1.begin(),s1.end(), // first source string
                       s2.begin(),        // second source string
                       nocase_equal_char);
}
#endif

static inline bool
isCaseSensitiveFileSystem(const QString &path)
{
    Q_UNUSED(path)
#if defined(Q_OS_WIN)
    // Return case insensitive unconditionally, even if someone has a case sensitive

    // file system mounted, wrongly capitalized drive letters will cause mismatches.
    return false;
#elif defined(Q_OS_OSX)

    return pathconf(QFile::encodeName(path).constData(), _PC_CASE_SENSITIVE);
#else

    return true;
#endif
}

#if 0

#ifdef FILE_DIALOG_DISABLE_ICONS
class EmptyIconProvider
    : public QFileIconProvider
{
public:

    EmptyIconProvider()
        : QFileIconProvider()
    {
    }

    virtual QIcon   icon(IconType /*type*/) const
    {
        return QIcon();
    }

    virtual QIcon   icon(const QFileInfo & /*info*/) const
    {
        return QIcon();
    }

    virtual QString type(const QFileInfo & /*info*/) const
    {
        return QString();
    }
};

#endif

#endif // 0

#ifdef __NATRON_WIN32__

static QString mapPathWithDriveLetterToPathWithNetworkShareName(const QString& path)
{
	if (path.size() <= 2) {
		return path;
	}

	QString ret;
	if (path[0].isLetter() && path[1] == QChar(':')) {

		QString driveName = path.mid(0,2);

		TCHAR szDeviceName[512]; 
		DWORD dwResult, cchBuff = sizeof(szDeviceName);
		dwResult = WNetGetConnection(driveName.toStdString().c_str(), szDeviceName, &cchBuff);
		if (dwResult == NO_ERROR) {
			ret = path.mid(2,-1);

			//Replace \\ with / 
			QString qDeviceName(szDeviceName);
			qDeviceName.replace('\\','/');

			//Make sure we remember the mapping
			appPTR->registerUNCPath(qDeviceName, path[0]);

			ret.prepend(qDeviceName);
			return ret;
		} 
	}  
	ret = path;
	return ret;


}


#endif

///////////////////////// SequenceFileDialog

SequenceFileDialog::SequenceFileDialog( QWidget* parent, // necessary to transmit the stylesheet to the dialog
                                       const std::vector<std::string> & filters, // the user accepted file types
                                       bool isSequenceDialog, // true if this dialog can display sequences
                                       FileDialogModeEnum mode, // if it is an open or save dialog
                                       const std::string & currentDirectory,// the directory to show first
                                       Gui* gui,
                                       bool allowRelativePaths)
    : QDialog(parent)
      , _filters()
      , _view(0)
      , _itemDelegate( new SequenceItemDelegate(this) )
      , _model()
      , _favoriteViewModel( new QFileSystemModel() )
      , _lookinViewModel( new QFileSystemModel() )
      , _mainLayout(0)
      , _requestedDir()
      , _lookInLabel(0)
      , _lookInCombobox(0)
      , _previousButton(0)
      , _nextButton(0)
      , _upButton(0)
      , _createDirButton(0)
      , _openButton(0)
      , _cancelButton(0)
      , _addFavoriteButton(0)
      , _removeFavoriteButton(0)
      , _selectionLineEdit(0)
      , _relativeLabel(0)
      , _relativeChoice(0)
      , _sequenceButton(0)
      , _filterLabel(0)
      , _filterLineEdit(0)
      , _filterDropDown(0)
      , _fileExtensionCombo(0)
      , _buttonsLayout(0)
      , _centerLayout(0)
      , _favoriteLayout(0)
      , _favoriteButtonsLayout(0)
      , _selectionLayout(0)
      , _filterLineLayout(0)
      , _filterLayout(0)
      , _buttonsWidget(0)
      , _favoriteWidget(0)
      , _favoriteButtonsWidget(0)
      , _selectionWidget(0)
      , _filterLineWidget(0)
      , _filterWidget(0)
      , _favoriteView(0)
      , _centerSplitter(0)
      , _history()
      , _currentHistoryLocation(-1)
      , _showHiddenAction(0)
      , _newFolderAction(0)
      , _dialogMode(mode)
      , _centerArea(0)
      , _centerAreaLayout(0)
      , _togglePreviewButton(0)
      , _preview()
      , _wasAutosetProjectFormatEnabled(false)
      , _gui(gui)
      , _relativePathsAllowed(allowRelativePaths)
{
    
    for (U32 i = 0; i < filters.size(); ++i) {
        _filters.push_back(filters[i].c_str());
    }
    
    setWindowFlags(Qt::Window);
    _mainLayout = new QVBoxLayout(this);
    setLayout(_mainLayout);
    /*Creating view and setting directory*/
    _view =  new SequenceDialogView(this);

    _model.reset(new FileSystemModel(this));
    
    _view->setSortingEnabled( isCaseSensitiveFileSystem( rootPath() ) );
#ifdef FILE_DIALOG_DISABLE_ICONS
    //EmptyIconProvider* iconProvider = new EmptyIconProvider;
    //_model->setIconProvider(iconProvider);
#endif

    _model->setSequenceModeEnabled(isSequenceDialog ? true : false);
    
    _view->setModel( _model.get() );
    _view->setItemDelegate( _itemDelegate.get() );

    QObject::connect( _model.get(),SIGNAL( directoryLoaded(QString) ),this,SLOT( updateView(QString) ) );
    QObject::connect( _view, SIGNAL( doubleClicked(QModelIndex) ), this, SLOT( doubleClickOpen(QModelIndex) ) );

    /*creating GUI*/
    _buttonsWidget = new QWidget(this);
    _buttonsLayout = new QHBoxLayout(_buttonsWidget);
    _buttonsWidget->setLayout(_buttonsLayout);
    if (mode == eFileDialogModeOpen) {
        _lookInLabel = new Natron::Label(tr("Look in:"),_buttonsWidget);
    } else {
        _lookInLabel = new Natron::Label(tr("Save in:"),_buttonsWidget);
    }
    _buttonsLayout->addWidget(_lookInLabel);

    _lookInCombobox = new FileDialogComboBox(this,_buttonsWidget);
    _buttonsLayout->addWidget(_lookInCombobox);
    QObject::connect( _lookInCombobox, SIGNAL( activated(QString) ), this, SLOT( onLookingComboboxChanged(QString) ) );
    _lookInCombobox->setInsertPolicy(QComboBox::NoInsert);
    _lookInCombobox->setDuplicatesEnabled(false);

   // _lookInCombobox->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Fixed);

    _buttonsLayout->addStretch();


    _previousButton = new Button(style()->standardIcon(QStyle::SP_ArrowBack),"",_buttonsWidget);
    _buttonsLayout->addWidget(_previousButton);
    QObject::connect( _previousButton, SIGNAL( clicked() ), this, SLOT( previousFolder() ) );

    _nextButton = new Button(style()->standardIcon(QStyle::SP_ArrowForward),"",_buttonsWidget);
    _buttonsLayout->addWidget(_nextButton);
    QObject::connect( _nextButton, SIGNAL( clicked() ), this, SLOT( nextFolder() ) );

    _upButton = new Button(style()->standardIcon(QStyle::SP_ArrowUp),"",_buttonsWidget);
    _buttonsLayout->addWidget(_upButton);
    QObject::connect( _upButton, SIGNAL( clicked() ), this, SLOT( parentFolder() ) );

    _createDirButton = new Button(style()->standardIcon(QStyle::SP_FileDialogNewFolder),"",_buttonsWidget);
    _buttonsLayout->addWidget(_createDirButton);
    QObject::connect( _createDirButton, SIGNAL( clicked() ), this, SLOT( createDir() ) );

    _mainLayout->addWidget(_buttonsWidget);

    /*creating center*/
    _centerArea = new QWidget(this);
    _centerAreaLayout = new QHBoxLayout(_centerArea);
    _centerAreaLayout->setContentsMargins(0, 0, 0, 0);
    
    _centerSplitter = new QSplitter(_centerArea);
    _centerSplitter->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    _favoriteWidget = new QWidget(this);
    _favoriteLayout = new QVBoxLayout(_favoriteWidget);
    _favoriteWidget->setLayout(_favoriteLayout);
    _favoriteView = new FavoriteView(_gui,this);
    QObject::connect( _favoriteView,SIGNAL( urlRequested(QUrl) ),this,SLOT( seekUrl(QUrl) ) );
    _favoriteLayout->setSpacing(0);
    _favoriteLayout->setContentsMargins(0, 0, 0, 0);
    _favoriteLayout->addWidget(_favoriteView);

    _favoriteButtonsWidget = new QWidget(_favoriteView);
    _favoriteButtonsLayout = new QHBoxLayout(_favoriteButtonsWidget);
    // _favoriteButtonsLayout->setSpacing(0);
    _favoriteButtonsLayout->setContentsMargins(0,0,0,0);
    _favoriteButtonsWidget->setLayout(_favoriteButtonsLayout);

    _addFavoriteButton = new Button("+",this);
    _addFavoriteButton->setMaximumSize(20,20);
    _favoriteButtonsLayout->addWidget(_addFavoriteButton);
    QObject::connect( _addFavoriteButton, SIGNAL( clicked() ), this, SLOT( addFavorite() ) );

    _removeFavoriteButton = new Button("-",this);
    _removeFavoriteButton->setMaximumSize(20,20);
    _favoriteButtonsLayout->addWidget(_removeFavoriteButton);
    QObject::connect( _removeFavoriteButton, SIGNAL( clicked() ), _favoriteView, SLOT( removeEntry() ) );

    _favoriteButtonsLayout->addStretch();

    _favoriteLayout->addWidget(_favoriteButtonsWidget);


    _centerSplitter->addWidget(_favoriteWidget);
    _centerSplitter->addWidget(_view);
    
    _centerAreaLayout->addWidget(_centerSplitter);
    
    if (mode == eFileDialogModeOpen && isSequenceDialog) {
        QPixmap pixPreviewButtonEnabled,pixPreviewButtonDisabled;
        appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_PLAY_ENABLED, &pixPreviewButtonEnabled);
        appPTR->getIcon(Natron::NATRON_PIXMAP_PLAYER_PLAY_DISABLED, &pixPreviewButtonDisabled);
        QIcon icPreview;
        icPreview.addPixmap(pixPreviewButtonEnabled,QIcon::Normal,QIcon::On);
        icPreview.addPixmap(pixPreviewButtonDisabled,QIcon::Normal,QIcon::Off);
        _togglePreviewButton = new Button(icPreview,"",_centerArea);
        QObject::connect(_togglePreviewButton, SIGNAL(clicked(bool)), this, SLOT(onTogglePreviewButtonClicked(bool) ) );
        _togglePreviewButton->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Expanding);
        _togglePreviewButton->setCheckable(true);
        _togglePreviewButton->setChecked(false);
        _centerAreaLayout->addWidget(_togglePreviewButton);
        assert(gui);
        _preview = gui->getApp()->getPreviewProvider();
        _wasAutosetProjectFormatEnabled = gui->getApp()->getProject()->isAutoSetProjectFormatEnabled();
    }
    
    _mainLayout->addWidget(_centerArea);
    
    /*creating selection widget*/
    _selectionWidget = new QWidget(this);
    _selectionLayout = new QHBoxLayout(_selectionWidget);
    _selectionLayout->setContentsMargins(0, 0, 0, 0);
    _selectionWidget->setLayout(_selectionLayout);

    _relativeLabel = new Natron::Label(tr("Relative to:"),_selectionWidget);
    _selectionLayout->addWidget(_relativeLabel);
    
    _relativeChoice = new ComboBox(_selectionWidget);
    QObject::connect(_relativeChoice,SIGNAL(currentIndexChanged(int)),this,SLOT(onRelativeChoiceChanged(int)));
    _relativeChoice->setToolTip(Natron::convertFromPlainText(tr("This controls how the file-path (absolute/relative) that you choose will be fetched once you have ""chosen a file. The path will be made relative to the selected project path only when OK will be pressed."), Qt::WhiteSpaceNormal));
    _selectionLayout->addWidget(_relativeChoice);
    _relativeChoice->addItem( tr("Absolute") );
    std::map<std::string,std::string> projectPaths;
    gui->getApp()->getProject()->getEnvironmentVariables(projectPaths);
    for (std::map<std::string,std::string>::iterator it = projectPaths.begin(); it != projectPaths.end(); ++it) {
        QString varName;
        varName.append('[');
        varName.append(it->first.c_str());
        varName.append(']');
        _relativeChoice->addItem(varName);
    }
    if (!allowRelativePaths) {
        _relativeLabel->hide();
        _relativeChoice->hide();
    }
    
    _sequenceButton = new ComboBox(_selectionWidget);
    _sequenceButton->addItem( tr("Sequence:") );
    _sequenceButton->addItem( tr("File:") );
   
    
    if (isSequenceDialog) {
        _sequenceButton->setCurrentIndex(0);
    } else {
        _sequenceButton->setCurrentIndex(1);
    }
    QObject::connect( _sequenceButton,SIGNAL( currentIndexChanged(int) ),this,SLOT( sequenceComboBoxSlot(int) ) );
    _selectionLayout->addWidget(_sequenceButton);

    if ( !isSequenceDialog || (_dialogMode == eFileDialogModeDir) ) {
        _sequenceButton->setVisible(false);
    }
    _selectionLineEdit = new LineEdit(_selectionWidget);
    _selectionLayout->addWidget(_selectionLineEdit);

    if ( (mode == SequenceFileDialog::eFileDialogModeOpen) || (mode == SequenceFileDialog::eFileDialogModeDir) ) {
        _openButton = new Button(tr("Open"),_selectionWidget);
    } else {
        _openButton = new Button(tr("Save"),_selectionWidget);
    }
    _openButton->setFocusPolicy(Qt::TabFocus);
    _selectionLayout->addWidget(_openButton);

    if (_dialogMode != eFileDialogModeDir) {
        QObject::connect( _openButton, SIGNAL( clicked() ), this, SLOT( openSelectedFiles() ) );
    } else {
        QObject::connect( _openButton,SIGNAL( clicked() ),this,SLOT( selectDirectory() ) );
    }
    _mainLayout->addWidget(_selectionWidget);

    /*creating filter zone*/
    _filterLineWidget = new QWidget(this);
    _filterLineLayout = new QHBoxLayout(_filterLineWidget);
    _filterLineLayout->setContentsMargins(0, 0, 0, 0);
    _filterLineWidget->setLayout(_filterLineLayout);

    if (_dialogMode == eFileDialogModeOpen) {
        _filterLabel = new Natron::Label(tr("Filter:"),_filterLineWidget);
        _filterLineLayout->addWidget(_filterLabel);
    } else if (_dialogMode == eFileDialogModeSave) {
        _filterLabel = new Natron::Label(tr("File type:"),_filterLineWidget);
        _filterLineLayout->addWidget(_filterLabel);
    }

    _filterWidget = new QWidget(_filterLineWidget);
    _filterLayout = new QHBoxLayout(_filterWidget);
    _filterWidget->setLayout(_filterLayout);
    _filterLayout->setContentsMargins(0,0,0,0);
    _filterLayout->setSpacing(0);

    QString filter = FileSystemModel::generateRegexpFilterFromFileExtensions(_filters);

    if (_dialogMode == eFileDialogModeSave) {
        _fileExtensionCombo = new ComboBox(_filterWidget);
        for (int i = 0; i < _filters.size(); ++i) {
            _fileExtensionCombo->addItem( _filters[i] );
        }
        _filterLineLayout->addWidget(_fileExtensionCombo);
        QObject::connect( _fileExtensionCombo, SIGNAL( currentIndexChanged(int) ), this, SLOT( onFileExtensionComboChanged(int) ) );
        if (isSequenceDialog) {
            int idx = _fileExtensionCombo->itemIndex("jpg");
            if (idx >= 0) {
                _fileExtensionCombo->setCurrentIndex(idx);
            }
        }
    }

    if (_dialogMode != eFileDialogModeDir) {
        _filterLineEdit = new LineEdit(_filterWidget);
        _filterLayout->addWidget(_filterLineEdit);
        _filterLineEdit->setText(filter);
        QSize buttonSize( 15,_filterLineEdit->sizeHint().height() );
        QPixmap pixDropDown;
        appPTR->getIcon(NATRON_PIXMAP_COMBOBOX, &pixDropDown);
        _filterDropDown = new Button(QIcon(pixDropDown),"",_filterWidget);
        _filterDropDown->setFixedSize(buttonSize);
        _filterLayout->addWidget(_filterDropDown);
        QObject::connect( _filterDropDown,SIGNAL( clicked() ),this,SLOT( showFilterMenu() ) );
        QObject::connect( _filterLineEdit,SIGNAL( textEdited(QString) ),this,SLOT( applyFilter(QString) ) );
    } else {
        _filterLineLayout->addStretch();
    }
    
    _model->setRegexpFilters(filter);


    _filterLineLayout->addWidget(_filterWidget);
    _cancelButton = new Button(tr("Cancel"),_filterLineWidget);
    _filterLineLayout->addWidget(_cancelButton);
    QObject::connect( _cancelButton, SIGNAL( clicked() ), this, SLOT( cancelSlot() ) );

    _mainLayout->addWidget(_filterLineWidget);

    resize(900, 400);

    std::vector<QUrl> initialBookmarks;
    initialBookmarks.push_back( QUrl::fromLocalFile( QDir::homePath() ) );
#ifndef __NATRON_WIN32__
    initialBookmarks.push_back( QUrl::fromLocalFile( QLatin1String("/") ) );
#else

    initialBookmarks.push_back( QUrl::fromLocalFile( QLatin1String("C:/") ) );

#endif
    
    _favoriteView->setModelAndUrls(_favoriteViewModel.get(), initialBookmarks);


    QItemSelectionModel *selectionModel = _view->selectionModel();
    QObject::connect( selectionModel, SIGNAL( selectionChanged(QItemSelection,QItemSelection) ),this, SLOT( onSelectionChanged() ) );
    QObject::connect( _selectionLineEdit, SIGNAL( textChanged(QString) ),this, SLOT( autoCompleteFileName(QString) ) );
    if (_dialogMode == eFileDialogModeSave) {
        QObject::connect( _selectionLineEdit,SIGNAL( textEdited(QString) ),this,SLOT( onSelectionLineEditing(QString) ) );
    }
    QObject::connect( _view, SIGNAL( customContextMenuRequested(QPoint) ),
                      this, SLOT( showContextMenu(QPoint) ) );
    QObject::connect( _model.get(), SIGNAL( rootPathChanged(QString) ),
                      this, SLOT( pathChanged(QString) ) );
    QObject::connect( _view->header(), SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)), this,
                     SLOT(onHeaderViewSortIndicatorChanged(int, Qt::SortOrder)));
    
    createMenuActions();

    if (_dialogMode == eFileDialogModeOpen) {
        if (isSequenceDialog) {
            setWindowTitle( tr("Open Sequence") );
        } else {
            setWindowTitle( tr("Open File") );
        }
    } else if (_dialogMode == eFileDialogModeSave) {
        if (isSequenceDialog) {
            setWindowTitle( tr("Save Sequence") );
        } else {
            setWindowTitle( tr("Save File") );
        }
    } else {
        setWindowTitle( tr("Select Directory") );
    }

    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    restoreState( settings.value( QLatin1String("FileDialog") ).toByteArray(),currentDirectory.empty() );

    if ( !currentDirectory.empty() ) {
        setDirectory( currentDirectory.c_str() );
    }


    if (!isSequenceDialog) {
        enableSequenceMode(false);
    }
    _selectionLineEdit->setFocus();
}

SequenceFileDialog::~SequenceFileDialog()
{
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);

    settings.setValue( QLatin1String("FileDialog"), saveState() );
}

void
SequenceFileDialog::onFileExtensionComboChanged(int index)
{
    setFileExtensionOnLineEdit( _fileExtensionCombo->itemText(index) );
}

QByteArray
SequenceFileDialog::saveState() const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    QList<QUrl> urls;
    std::vector<QUrl> stdUrls = _favoriteView->urls();
    for (unsigned int i = 0; i < stdUrls.size(); ++i) {
        urls.push_back(stdUrls[i]);
    }
    stream << _centerSplitter->saveState();
    stream << urls;
    stream << history();
    stream << currentDirectory().path();
    stream << _view->header()->saveState();
    stream << _relativeChoice->itemText(_relativeChoice->activeIndex());
    stream << _sequenceButton->activeIndex();
    return data;
}



bool
SequenceFileDialog::restoreState(const QByteArray & state, bool restoreDirectory)
{
    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);

    if ( stream.atEnd() ) {
        return false;
    }
    QByteArray splitterState;
    QByteArray headerData;
    QList<QUrl> bookmarks;
    QStringList history;
    QString currentDirectory;
    QString relativeChoice;
    int sequenceMode_i;
    stream >> splitterState
    >> bookmarks
    >> history
    >> currentDirectory
    >> headerData
    >> relativeChoice
    >> sequenceMode_i;
    if ( !_centerSplitter->restoreState(splitterState) ) {
        return false;
    }
    QList<int> list = _centerSplitter->sizes();
    if ( (list.count() >= 2) && (list.at(0) == 0) && (list.at(1) == 0) ) {
        for (int i = 0; i < list.count(); ++i) {
            list[i] = _centerSplitter->widget(i)->sizeHint().width();
        }
        _centerSplitter->setSizes(list);
    }
    
    
    for (int i = 0; i < _relativeChoice->count(); ++i) {
        if (_relativeChoice->itemText(i) == relativeChoice) {
            _relativeChoice->setCurrentIndex(i);
            break;
        }
    }
    
    _sequenceButton->setCurrentIndex(sequenceMode_i);
    
    std::map<std::string,std::string> envVar;
    _gui->getApp()->getProject()->getEnvironmentVariables(envVar);
    
    std::vector<QUrl> stdBookMarks;
    
    
    QStringList expandedVars;
    for (std::map<std::string,std::string>::iterator it = envVar.begin(); it != envVar.end(); ++it) {
        QString var(it->second.c_str());
        if (it->first != NATRON_OCIO_ENV_VAR_NAME && !var.isEmpty()) {
            ///The variable may be nested
            Natron::Project::expandVariable(envVar, it->second);
            expandedVars.push_back(var);
            QUrl url = QUrl::fromLocalFile(var);
            
            QDir dir(var);
            if (dir.exists()) {
                stdBookMarks.push_back(url);
            }
        }
    }
    
    for (int i = 0; i < bookmarks.count(); ++i) {
        QString urlPath = bookmarks[i].path();
        
		// On windows url.path() will return something starting with a /
#ifdef __NATRON_WIN32__
		if (urlPath.startsWith("/")) {
			urlPath.remove(0,1);
		}
#endif
        if (!urlPath.isEmpty()) {
            if (urlPath.size() > 1 && (urlPath.endsWith('/') || urlPath.endsWith('\\'))) {
                urlPath = urlPath.remove(urlPath.size() - 1, 1);
            }
            bool alreadyFound = false;
            
            for (U32 j = 0; j < stdBookMarks.size(); ++j) {
                QString otherUrl = stdBookMarks[j].path();
                // On windows url.path() will return something starting with a /
#ifdef __NATRON_WIN32__
                if (otherUrl.startsWith("/")) {
                    otherUrl.remove(0,1);
                }
#endif
                
                if (otherUrl.size() > 1 && (otherUrl.endsWith('/') || otherUrl.endsWith('\\'))) {
                    otherUrl = otherUrl.remove(otherUrl.size() - 1, 1);
                }
                if (otherUrl == urlPath) {
                    alreadyFound = true;
                    break;
                }
            }
            
            QDir dir(urlPath);
            if (!alreadyFound && dir.exists()) {
                stdBookMarks.push_back( bookmarks[i] );
            }
        }
    }
    

    if (!stdBookMarks.empty()) {
        _favoriteView->addUrls(stdBookMarks,_favoriteView->getNUrls());
    }
    while (history.count() > 5) {
        history.pop_front();
    }
    setHistory(history);
    QHeaderView *headerView = _view->header();
    if ( !headerView->restoreState(headerData) ) {
        return false;
    }
    if (restoreDirectory) {
        setDirectory(currentDirectory);
    }

    QList<QAction*> actions = headerView->actions();
    QAbstractItemModel *abstractModel = _model.get();
    int total = qMin(abstractModel->columnCount( QModelIndex() ), actions.count() + 1);
    for (int i = 1; i < total; ++i) {
        actions.at(i - 1)->setChecked( !headerView->isSectionHidden(i) );
    }

    return true;
} // restoreState

void
SequenceFileDialog::setFileExtensionOnLineEdit(const QString & ext)
{
    assert(_dialogMode == eFileDialogModeSave);

    QString str  = _selectionLineEdit->text();
    if ( str.isEmpty() ) {
        return;
    }

    if ( isDirectory(str) ) {
        QString text = _selectionLineEdit->text() + tr("Untitled");
        if ( sequenceModeEnabled() ) {
            text.append('#');
        }
        _selectionLineEdit->setText(text + "." + ext);

        return;
    } else {
        int pos = str.lastIndexOf( QChar('.') );
        if (pos != -1) {
            if ( str.at(pos - 1) == QChar('#') ) {
                --pos;
            }
            str = str.left(pos);
        }
        if ( sequenceModeEnabled() ) {
            //find out if there's already a # character
            std::string unpathed = str.toStdString();
            SequenceParsing::removePath(unpathed);
            pos = unpathed.find_first_of('#');
            if ( (size_t)pos == std::string::npos ) {
                str.append("#");
            }
        }
        str.append(".");
        str.append(ext);
        _selectionLineEdit->setText(str);
    }
}

void
SequenceFileDialog::sequenceComboBoxSlot(int index)
{
    if (index == 1) {
        enableSequenceMode(false);
    } else {
        enableSequenceMode(true);
    }
}

void
SequenceFileDialog::showContextMenu(const QPoint & position)
{
    Natron::Menu menu(_view);

    menu.addAction(_showHiddenAction);
    if ( _createDirButton->isVisible() ) {
        _newFolderAction->setEnabled( _createDirButton->isEnabled() );
        menu.addAction(_newFolderAction);
    }
    menu.exec( _view->viewport()->mapToGlobal(position) );
}

void
SequenceFileDialog::createMenuActions()
{
    QAction *goHomeAction =  new QAction(this);

    goHomeAction->setShortcut(Qt::CTRL + Qt::Key_H + Qt::SHIFT);
    QObject::connect( goHomeAction, SIGNAL( triggered() ), this, SLOT( goHome() ) );
    addAction(goHomeAction);


    QAction *goToParent =  new QAction(this);
    goToParent->setShortcut(Qt::CTRL + Qt::UpArrow);
    QObject::connect( goToParent, SIGNAL( triggered() ), this, SLOT( parentFolder() ) );
    addAction(goToParent);

    _showHiddenAction = new QAction(this);
    _showHiddenAction->setCheckable(true);
    _showHiddenAction->setText( tr("Show hidden fildes") );
    QObject::connect( _showHiddenAction, SIGNAL( triggered() ), this, SLOT( showHidden() ) );

    _newFolderAction = new QAction(this);
    _newFolderAction->setText( tr("New folder") );
    QObject::connect( _newFolderAction, SIGNAL( triggered() ), this, SLOT( createDir() ) );
}

void
SequenceFileDialog::enableSequenceMode(bool b)
{
    _model->setSequenceModeEnabled(b);
}

bool
SequenceFileDialog::getRelativeChoiceProjectPath(std::string& varName,std::string& varValue) const
{
    int choice = _relativeChoice->activeIndex();
    if (choice == 0) {
        return false; //< absolute
    } else {
        --choice; //we skip the absolute choice
        std::map<std::string,std::string> projectPaths;
        _gui->getApp()->getProject()->getEnvironmentVariables(projectPaths);
        std::map<std::string,std::string>::iterator it = projectPaths.begin();
        assert(choice < (int)projectPaths.size());
        std::advance(it, choice);
        varName = it->first;
        varValue = it->second;
        return true;
    }
}

Qt::SortOrder
SequenceFileDialog::sortIndicatorOrder() const
{
    return _view->header()->sortIndicatorOrder();
}

int
SequenceFileDialog::sortIndicatorSection() const
{
    return _view->header()->sortIndicatorSection();
}

void
SequenceFileDialog::onHeaderViewSortIndicatorChanged(int logicalIndex,Qt::SortOrder order)
{
    onSortIndicatorChanged(logicalIndex,order);
}

void
SequenceFileDialog::onSortIndicatorChanged(int logicalIndex,Qt::SortOrder order)
{
    _model->onSortIndicatorChanged(logicalIndex, order);
}

void
SequenceFileDialog::onRelativeChoiceChanged(int /*index*/)
{
    ///Update the line edit text
    std::string text = _selectionLineEdit->text().toStdString();
    std::map<std::string,std::string> env;
    _gui->getApp()->getProject()->getEnvironmentVariables(env);
    Project::expandVariable(env, text);
    
    proxyAndSetLineEditText(text.c_str());
}

void
SequenceFileDialog::onSelectionChanged()
{
    QStringList allFiles;
    QModelIndexList indexes = _view->selectionModel()->selectedRows();

    assert(indexes.size() <= 1);
    ///if a parsed sequence exists with the
    if ( indexes.empty() ) {
        return;
    }
    
    QModelIndex mappedIndex = indexes[0];
    
    FileSystemItem* item = _model->getFileSystemItem(mappedIndex);
    assert(item);
    
    ///if the selected item is a directory, just set the line edit accordingly
    QString absolutePath = item->absoluteFilePath();

    proxyAndSetLineEditText(absolutePath);

    if (!item->isDir()) {
        if (_dialogMode == eFileDialogModeSave) {
            QString extension = item->fileExtension();
            for (int i = 0; i < _fileExtensionCombo->count(); ++i) {
                if ( _fileExtensionCombo->itemText(i) == extension ) {
                    _fileExtensionCombo->setCurrentIndex_no_emit(i);
                    break;
                }
            }
        }
    }
    
    ///refrsh preview if any
    if (_preview && _preview->viewerUI && _preview->viewerUI->isVisible()) {
        refreshPreviewAfterSelectionChange();
    }
    
} // onSelectionChanged

void
SequenceFileDialog::enterDirectory(const QModelIndex & index)
{
    QString path = index.data(FileSystemModel::FilePathRole).toString();

    if ( path.isEmpty() || _model->isDir(index) ) {
        setDirectory(path);
    }
}

void
SequenceFileDialog::setDirectory(const QString &directory)
{
    QDir dir(directory);
    if (!dir.exists()) {
        return;
    }
   
    QString newDirectory = directory;


    _view->selectionModel()->clear();
    _view->verticalScrollBar()->setValue(0);
    //we remove .. and . from the given path if exist
    if ( !directory.isEmpty() ) {
        newDirectory = QDir::cleanPath(directory);
    }

    if ( !directory.isEmpty() && newDirectory.isEmpty() ) {
        return;
    }

	#ifdef __NATRON_WIN32__
	newDirectory = appPTR->mapUNCPathToPathWithDriveLetter(newDirectory);
#endif
	
    _requestedDir = newDirectory;
    _model->setRootPath(newDirectory);
    _createDirButton->setEnabled(_dialogMode != eFileDialogModeOpen);
    if ( !newDirectory.isEmpty() && newDirectory.at(newDirectory.size() - 1) != QChar('/') ) {
        newDirectory.append("/");
    }

    _selectionLineEdit->blockSignals(true);

    if ( (_dialogMode == eFileDialogModeOpen) || (_dialogMode == eFileDialogModeDir) ) {
        proxyAndSetLineEditText(newDirectory);
    } else {
        ///find out if there's already a filename typed by the user
        ///and append it to the new path
        std::string unpathed = _selectionLineEdit->text().toStdString();
        SequenceParsing::removePath(unpathed);
        proxyAndSetLineEditText( newDirectory + unpathed.c_str() );
    }

    _selectionLineEdit->blockSignals(false);
}


void
SequenceFileDialog::proxyAndSetLineEditText(const QString& text)
{
    std::string stdText = text.toStdString();
    if (_relativePathsAllowed) {
        std::string varName,varPath;
        bool relative = getRelativeChoiceProjectPath(varName, varPath);
        if (relative) {
            Natron::Project::makeRelativeToVariable(varName, varPath, stdText);
        }
    }
    _selectionLineEdit->setText(stdText.c_str());
}

/*This function is called when a directory has successfully been loaded (i.e
   when the filesystem finished to load thoroughly the directory requested)*/
void
SequenceFileDialog::updateView(const QString &directory)
{
   

    boost::shared_ptr<FileSystemItem> directoryItem = _model->getFileSystemItem(directory);
    assert(directoryItem);
    
    QModelIndex index = _model->index(directoryItem.get());
    
    /*update the view to show the newly loaded directory*/
    setRootIndex(index);
    
    /*clearselection*/
    _view->selectionModel()->clear();
}

bool
SequenceFileDialog::sequenceModeEnabled() const
{
    return _model->isSequenceModeEnabled();
}


void
SequenceFileDialog::setRootIndex(const QModelIndex & index)
{
    _view->setRootIndex(index);
}

SequenceDialogView::SequenceDialogView(SequenceFileDialog* fd)
    : QTreeView(fd),_fd(fd)
{
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setRootIsDecorated(false);
    setItemsExpandable(false);
    setSortingEnabled(true);
    header()->setSortIndicator(0, Qt::AscendingOrder);
    header()->setStretchLastSection(false);
    setTextElideMode(Qt::ElideMiddle);
    setEditTriggers(QAbstractItemView::EditKeyPressed);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    //setAttribute(Qt::WA_MacShowFocusRect,0);
    setAcceptDrops(true);
}

void
SequenceDialogView::paintEvent(QPaintEvent* e)
{
    QTreeView::paintEvent(e);
}


void
SequenceDialogView::dropEvent(QDropEvent* e)
{
    if ( !e->mimeData()->hasUrls() ) {
        return;
    }

    QStringList filesList;
    QList<QUrl> urls = e->mimeData()->urls();
    QString path;
    if (urls.size() > 0) {
        path = Natron::toLocalFileUrlFixed(urls.at(0)).path();
    }
    if ( !path.isEmpty() ) {
        _fd->setDirectory(path);
    }
}

void
SequenceDialogView::dragEnterEvent(QDragEnterEvent* e)
{
    e->accept();
}

void
SequenceDialogView::dragMoveEvent(QDragMoveEvent* e)
{
    e->accept();
}

void
SequenceDialogView::dragLeaveEvent(QDragLeaveEvent* e)
{
    e->accept();
}

void
SequenceDialogView::resizeEvent(QResizeEvent* e)
{
    expandColumnsToFullWidth( e->size().width() );
    // repaint(); //< seems to make Qt crash on windows
    QTreeView::resizeEvent(e);
}

void
SequenceDialogView::expandColumnsToFullWidth(int w)
{
    setColumnWidth(0,w * 0.65);
    setColumnWidth(1,w * 0.1);
    setColumnWidth(2, w * 0.1);
    setColumnWidth(3, w * 0.15);
}

SequenceItemDelegate::SequenceItemDelegate(SequenceFileDialog* fd)
    : QStyledItemDelegate()
      , _fd(fd)
{
}

void
SequenceItemDelegate::paint(QPainter * painter,
                            const QStyleOptionViewItem &option,
                            const QModelIndex & index) const
{
    assert( index.isValid() );
    
    
    // get the proper subrect from the style
    QStyle *style = QApplication::style();
    QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &option);
    
    
    
    FileSystemModel* model = _fd->getFileSystemModel();
    
    QModelIndex nameColumnIndex;
    if (index.column() == FileSystemModel::Name) {
        nameColumnIndex = index;
    } else {
        ///To get the file-path, first retrieve the item in the column 0
        nameColumnIndex = model->index( index.row(),0,index.parent() );

    }
    
    if ( !index.isValid() ) {
        return QStyledItemDelegate::paint(painter,option,index);
    }
    
    FileSystemItem* item = model->getFileSystemItem(nameColumnIndex);
    if (!item ) {
        return QStyledItemDelegate::paint(painter,option,index);
    }
    
    QString filename = item->fileName();
    //QFont f(appFont,appFontSize);
    //painter->setFont(f);
    if (option.state & QStyle::State_Selected) {
        painter->fillRect( geom, option.palette.highlight() );
    }
    
    QRect r;
    QRect textRect( geom.x() + 5,geom.y(),geom.width() - 5,geom.height() );

    
    if (index.column() == FileSystemModel::Type || index.column() == FileSystemModel::DateModified) {
        ///Draw the item name column
        QString data = index.data().toString();
        if (data.isEmpty()) {
            data = "--";
        }
        painter->drawText(textRect,Qt::TextSingleLine,data,&r);
        return;
    }
    
    bool isDir = item->isDir();
    
    if (index.column() == FileSystemModel::Size) {

        quint64 size = index.data().toULongLong();
        QString itemSizeText;
        if (!isDir) {
            itemSizeText = printAsRAM(size);
        } else {
            itemSizeText = "--";
        }
        painter->drawText(geom,Qt::TextSingleLine | Qt::AlignRight,itemSizeText,&r);
    } else {
#ifdef FILE_DIALOG_DISABLE_ICONS
        if (isDir && !filename.endsWith('/')) {
            filename.append('/');

        }
#endif
        // FIXME: with the default delegate (QStyledItemDelegate), there is a margin
        // of a few more pixels between border and icon, and between icon and text, not with this one
        
#ifdef FILE_DIALOG_DISABLE_ICONS
        QRect textRect( geom.x() + 5,geom.y(),geom.width() - 5,geom.height() );
        QFont f = painter->font();
        if (isDir) {
            //change the font to bold
            f.setBold(true);
        } else {
            f.setBold(false);
        }
        painter->setFont(f);
#else
        int iconWidth = option.decorationSize.width();
        int textSize = totalSize - iconWidth;
        QFileInfo fileInfo( index.data(FileSystemModel::FilePathRole).toString() );
        QIcon icon = _fd->getFileSystemModel()->iconProvider()->icon(fileInfo);
        QRect iconRect( geom.x(),geom.y(),iconWidth,geom.height() );
        QSize iconSize = icon.actualSize( QSize( iconRect.width(),iconRect.height() ) );
        QRect textRect( geom.x() + iconWidth,geom.y(),textSize,geom.height() );
        painter->drawPixmap(iconRect,
                            icon.pixmap(iconSize),
                            r);

#endif

        painter->drawText(textRect,Qt::TextSingleLine,filename,&r);

    }


  
} // paint


bool
SequenceFileDialog::isDirectory(const QString & name) const
{
    QDir dir(name);
	return dir.exists();
}

QString
SequenceFileDialog::rootPath() const
{
    return _model->rootPath();
}

QFileSystemModel*
SequenceFileDialog::getFavoriteSystemModel() const
{
    return _favoriteViewModel.get();
}

QFileSystemModel*
SequenceFileDialog::getLookingFileSystemModel() const
{
    return _lookinViewModel.get();
}

QSize
SequenceItemDelegate::sizeHint(const QStyleOptionViewItem & option,
                               const QModelIndex & index) const
{
    if (index.column() != 0) {
        return QStyledItemDelegate::sizeHint(option,index);
    }
    QString str =  index.data(FileSystemModel::FilePathRole).toString();
    QFontMetrics metric(option.font);

    return QSize( metric.width(str),metric.height() );
}


QString
SequenceFileDialog::getFilePath(const QString & str)
{
    int slashPos = str.lastIndexOf( QDir::separator() );

    if (slashPos != -1) {
        return str.left(slashPos);
    } else {
        return QString(".");
    }
}

void
SequenceFileDialog::previousFolder()
{
    if ( !_history.isEmpty() && (_currentHistoryLocation > 0) ) {
        --_currentHistoryLocation;
        QString previousHistory = _history.at(_currentHistoryLocation);
        setDirectory(previousHistory);
    }
}

void
SequenceFileDialog::nextFolder()
{
    if ( !_history.isEmpty() && (_currentHistoryLocation < _history.size() - 1) ) {
        ++_currentHistoryLocation;
        QString nextHistory = _history.at(_currentHistoryLocation);
        setDirectory(nextHistory);
    }
}

void
SequenceFileDialog::parentFolder()
{
    
    QString newDir ;

    QString rootPath = _model->rootPath();


    QDir dir(rootPath);
    dir.cdUp();
    newDir = dir.absolutePath();

	if (FileSystemModel::isDriveName(rootPath)) {
		newDir = "";
	}

	if (FileSystemModel::isDriveName(newDir)) {
		_upButton->setEnabled(false);
	} else {
		 _upButton->setEnabled(true);
	}

    setDirectory(newDir);
}

void
SequenceFileDialog::showHidden()
{
    QDir::Filters dirFilters = _model->filter();

    if ( _showHiddenAction->isChecked() ) {
        dirFilters |= QDir::Hidden;
    } else {
        dirFilters &= ~QDir::Hidden;
    }
    _model->setFilter(dirFilters);
    // options->setFilter(dirFilters);
    _showHiddenAction->setChecked( (dirFilters & QDir::Hidden) );
}

void
SequenceFileDialog::goHome()
{
    setDirectory( QDir::homePath() );
}

void
SequenceFileDialog::createDir()
{
    _favoriteView->clearSelection();
    QString newFolderString;
    QInputDialog dialog(this);
    dialog.setLabelText( tr("Folder name:") );
    dialog.setWindowTitle( tr("New Folder") );
    if ( dialog.exec() ) {
        newFolderString = dialog.textValue();
        if ( !newFolderString.isEmpty() ) {
            QString folderName = newFolderString;
            QString prefix  = currentDirectory().absolutePath() + QDir::separator();
            if ( QFile::exists(prefix + folderName) ) {
                qlonglong suffix = 2;
                while ( QFile::exists(prefix + folderName) ) {
                    folderName = newFolderString + QString::number(suffix);
                    ++suffix;
                }
            }
            QModelIndex index = _model->mkdir(_view->rootIndex(), folderName);
            if ( !index.isValid() ) {
                return;
            }
            enterDirectory(index);
        }
    }
}

AddFavoriteDialog::AddFavoriteDialog(SequenceFileDialog* fd,
                                     QWidget* parent)
    : QDialog(parent)
      , _fd(fd)
{
    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setSpacing(0);
    _mainLayout->setContentsMargins(5, 5, 0, 0);
    setLayout(_mainLayout);
    setWindowTitle( tr("New Favorite") );

    _descriptionLabel = new Natron::Label("",this);
    _mainLayout->addWidget(_descriptionLabel);

    _secondLine = new QWidget(this);
    _secondLineLayout = new QHBoxLayout(_secondLine);

    _pathLineEdit = new LineEdit(_secondLine);
    _pathLineEdit->setPlaceholderText( tr("path...") );
    _secondLineLayout->addWidget(_pathLineEdit);


    QPixmap pix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE,&pix);

    _openDirButton = new Button(_secondLine);
    _openDirButton->setIcon( QIcon(pix) );
    QObject::connect( _openDirButton, SIGNAL( clicked() ), this, SLOT( openDir() ) );
    _secondLineLayout->addWidget(_openDirButton);

    _mainLayout->addWidget(_secondLine);

    _thirdLine = new QWidget(this);
    _thirdLineLayout = new QHBoxLayout(_thirdLine);
    _thirdLine->setLayout(_thirdLineLayout);

    _cancelButton = new Button(tr("Cancel"),_thirdLine);
    QObject::connect( _cancelButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
    _thirdLineLayout->addWidget(_cancelButton);

    _okButton = new Button(tr("Ok"),_thirdLine);
    QObject::connect( _okButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
    _thirdLineLayout->addWidget(_okButton);

    _mainLayout->addWidget(_thirdLine);
}

void
AddFavoriteDialog::setLabelText(const QString & text)
{
    _descriptionLabel->setText(text);
}

void
AddFavoriteDialog::openDir()
{
    QString str = QFileDialog::getExistingDirectory( this,tr("Select a directory"),_fd->currentDirectory().path() );

    _pathLineEdit->setText(str);
}

QString
AddFavoriteDialog::textValue() const
{
    return _pathLineEdit->text();
}

void
SequenceFileDialog::addFavorite()
{
    AddFavoriteDialog dialog(this,this);

    dialog.setLabelText( tr("Folder path:") );
    QString newFavName,newFavPath;
    if ( dialog.exec() ) {
        newFavName = dialog.textValue();
        newFavPath = dialog.textValue();
        addFavorite(newFavName,newFavPath);
    }
}

void
SequenceFileDialog::addFavorite(const QString & name,
                                const QString & path)
{

    QDir d(path);

    if ( !d.exists() ) {
        return;
    }


    if ( !name.isEmpty() && !path.isEmpty() ) {
        std::vector<QUrl> list;
        list.push_back( QUrl::fromLocalFile(path) );
        _favoriteView->addUrls(list,-1);
    }
}

void
SequenceFileDialog::selectDirectory()
{
    QString str = _selectionLineEdit->text();

    if ( isDirectory(str) ) {
        setDirectory(str);
        QDialog::accept();
    }
}

void
SequenceFileDialog::openSelectedFiles()
{
    QString str = _selectionLineEdit->text();

    if (_dialogMode != eFileDialogModeDir) {
        if ( !isDirectory(str) ) {
            if (_dialogMode == eFileDialogModeOpen) {
                std::string pattern = selectedFiles();
                if ( !pattern.empty() ) {
                    teardownPreview();
                    QDialog::accept();
                }
            } else {
                ///check if str contains already the selected file extension, otherwise append it
                {
                    QString ext = _fileExtensionCombo->getCurrentIndexText();
                    if (ext != "*") {
                        int lastSepPos = str.lastIndexOf("/");
                        if (lastSepPos == -1) {
                            lastSepPos = str.lastIndexOf("//");
                        }
                        int lastDotPos = str.lastIndexOf('.');
                        if (lastDotPos < lastSepPos) {
                            str.append( "." +  ext);
                            _selectionLineEdit->blockSignals(true);
                            _selectionLineEdit->setText(str);
                            _selectionLineEdit->blockSignals(false);
                        }
                    }
                }

                SequenceParsing::SequenceFromPattern sequence;
                SequenceParsing::filesListFromPattern(str.toStdString(), &sequence);
                if (sequence.size() > 0) {
                    QString text;
                    if (sequence.size() == 1) {
                        std::map<int,std::string> & views = sequence.begin()->second;
                        assert( !views.empty() );

                        text = "The file ";
                        text.append( views.begin()->second.c_str() );
                        text.append( tr(" already exists.\n Would you like to replace it ?") );
                    } else {
                        text = tr("The sequence ");
                        text.append(str);
                        text.append( tr(" already exists.\n Would you like to replace it ?") );
                    }
                    QMessageBox::StandardButton ret = QMessageBox::question(this, tr("Existing file"), text,
                                                                            QMessageBox::Yes | QMessageBox::No);
                    if (ret != QMessageBox::Yes) {
                        return;
                    }
                }
                QDialog::accept();
            }
        } else {
            setDirectory(str);
        }
    } else {
        if ( isDirectory(str) ) {
            _selectionLineEdit->setText(str);
            setDirectory(str);
        }
    }
} // openSelectedFiles

void
SequenceFileDialog::cancelSlot()
{
    teardownPreview();
    reject();
}

void
SequenceFileDialog::keyPressEvent(QKeyEvent* e)
{
    if ( (e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter) ) {
        QString str = _selectionLineEdit->text();
        if ( !isDirectory(str) ) {
            QDialog::accept();
        } else {
            setDirectory(str);
        }

        return;
    }
    QDialog::keyPressEvent(e);
}

void
SequenceFileDialog::resizeEvent(QResizeEvent* e)
{
    QDialog::resizeEvent(e);
}

QStringList
SequenceFileDialog::typedFiles() const
{
    QStringList files;
    QString editText = _selectionLineEdit->text();

    if ( !editText.contains( QLatin1Char('"') ) ) {
#ifdef Q_OS_UNIX
        const QString prefix = currentDirectory().absolutePath() + QDir::separator();
        if ( QFile::exists(prefix + editText) ) {
            files << editText;
        } else {
            files << AppManager::qt_tildeExpansion(editText);
        }
#else
        files << editText;
#endif
    } else {
        // " is used to separate files like so: "file1" "file2" "file3" ...
        // ### need escape character for filenames with quotes (")
        QStringList tokens = editText.split( QLatin1Char('\"') );
        for (int i = 0; i < tokens.size(); ++i) {
            if ( (i % 2) == 0 ) {
                continue; // Every even token is a separator
            }
#ifdef Q_OS_UNIX
            const QString token = tokens.at(i);
            const QString prefix = currentDirectory().absolutePath() + QDir::separator();
            if ( QFile::exists(prefix + token) ) {
                files << token;
            } else {
                files << AppManager::qt_tildeExpansion(token);
            }
#else
            files << toInternal( tokens.at(i) );
#endif
        }
    }

    return files;
}

void
SequenceFileDialog::autoCompleteFileName(const QString & text)
{
    if ( text.startsWith( QLatin1String("//") ) || text.startsWith( QLatin1Char('\\') ) ) {
        _view->selectionModel()->clearSelection();

        return;
    }
    QStringList multipleFiles = typedFiles();
    if (multipleFiles.count() > 0) {
        QModelIndexList oldFiles = _view->selectionModel()->selectedRows();
        QModelIndexList newFiles;
        for (int i = 0; i < multipleFiles.count(); ++i) {
            QModelIndex idx = _model->index( multipleFiles.at(i) );
            if ( oldFiles.contains(idx) ) {
                oldFiles.removeAll(idx);
            } else {
                newFiles.append(idx);
            }
        }

        _view->selectionModel()->blockSignals(true);

        if ( _selectionLineEdit->hasFocus() ) {
            for (int i = 0; i < oldFiles.count(); ++i) {
                _view->selectionModel()->select(oldFiles.at(i),QItemSelectionModel::Toggle | QItemSelectionModel::Rows);
            }
        }

        for (int i = 0; i < newFiles.count(); ++i) {
            select( newFiles.at(i) );
        }
        _view->selectionModel()->blockSignals(false);
    }
}

void
SequenceFileDialog::onLookingComboboxChanged(const QString & /*path*/)
{
    QModelIndex index = _lookInCombobox->model()->index( _lookInCombobox->currentIndex(),
                                                         _lookInCombobox->modelColumn(),
                                                         _lookInCombobox->rootModelIndex() );

    if ( !index.isValid() ) {
        return;
    }
    
    QUrl url = index.data(UrlModel::UrlRole).toUrl();
    url = Natron::toLocalFileUrlFixed(url);
    //enterDirectory(index);
	setDirectory(url.path());
}

QString
SequenceFileDialog::getEnvironmentVariable(const QString &string)
{
#ifdef Q_OS_UNIX
    if ( (string.size() > 1) && string.startsWith( QLatin1Char('$') ) ) {
        return QString::fromLocal8Bit( getenv( string.mid(1).toLatin1().constData() ) );
    }
#else
    if ( (string.size() > 2) && string.startsWith( QLatin1Char('%') ) && string.endsWith( QLatin1Char('%') ) ) {
        return QString::fromLocal8Bit( qgetenv( string.mid(1, string.size() - 2).toLatin1().constData() ) );
    }
#endif

    return string;
}

void
SequenceFileDialog::pathChanged(const QString &newPath)
{
    
    if (newPath.isEmpty()) {
        _upButton->setEnabled(false);
    } else {
        _upButton->setEnabled(true);
    }
    
    _favoriteView->selectUrl( QUrl::fromLocalFile(newPath) );
    setHistory( _lookInCombobox->history() );

    if ( (_currentHistoryLocation < 0) || ( _history.value(_currentHistoryLocation) != QDir::toNativeSeparators(newPath) ) ) {
        while ( _currentHistoryLocation >= 0 && _currentHistoryLocation + 1 < _history.count() ) {
            _history.removeLast();
        }
        _history.append( QDir::toNativeSeparators(newPath) );
        ++_currentHistoryLocation;
    }
    _nextButton->setEnabled(_history.size() - _currentHistoryLocation > 1);
    _previousButton->setEnabled(_currentHistoryLocation > 0);
}

void
SequenceFileDialog::setHistory(const QStringList &paths)
{
    _lookInCombobox->setHistory(paths);
}

QStringList
SequenceFileDialog::history() const
{
    QStringList currentHistory = _lookInCombobox->history();
    //On windows the popup display the "C:\", convert to nativeSeparators
    QString newHistory = QDir::toNativeSeparators( _view->rootIndex().data(FileSystemModel::FilePathRole).toString() );

    if ( !currentHistory.contains(newHistory) ) {
        currentHistory << newHistory;
    }

    return currentHistory;
}

std::string
SequenceFileDialog::selectedFiles()
{
    QModelIndexList indexes = _view->selectionModel()->selectedRows();
    if (indexes.count() > 1) {
        return std::string();
    }

    
    std::string selection;
    
   
    
    bool remapSelection = false;
    
    if (indexes.count() == 1) {
        
        FileSystemItem* item = _model->getFileSystemItem(indexes[0]);
        
        if (!item) {
            return selection;
        }
        boost::shared_ptr<SequenceParsing::SequenceFromFiles> sequence = item->getSequence();
        if (sequence) {
            selection = sequence->generateValidSequencePattern();
        } else {
            selection = item->absoluteFilePath().toStdString();
        }
        remapSelection = true;
    } else {
        //if nothing is selected, pick whatever the line edit tells us
        selection =  _selectionLineEdit->text().toStdString();
    }
    
    
    if (remapSelection && _relativePathsAllowed) {
        std::string varName,varPath;
        bool relative = getRelativeChoiceProjectPath(varName, varPath);
        if (relative) {
            Natron::Project::makeRelativeToVariable(varName, varPath, selection);
        }
    }

#ifdef __NATRON_WIN32__
	QString ret = mapPathWithDriveLetterToPathWithNetworkShareName(selection.c_str());
	selection = ret.toStdString();
#endif
    return selection;
}

std::string
SequenceFileDialog::filesToSave()
{
    assert(_dialogMode == eFileDialogModeSave);
    QString text = _selectionLineEdit->text();
    ///Find last dot position and remove everything after the extension which we might added on the line edit
    int lastDotPos = text.lastIndexOf('.');

	QString ret;
    if (lastDotPos != -1) {
        int i = lastDotPos + 1;
        while (i < text.size() && text.at(i) != QChar(' ')) {
            ++i;
        }
        ret =  text.mid(0,i);
        
    } else {
        ret = text;
    }

#ifdef __NATRON_WIN32__
    ret = mapPathWithDriveLetterToPathWithNetworkShareName(ret);
#endif

	return ret.toStdString();
}

QDir
SequenceFileDialog::currentDirectory() const
{
    return _requestedDir;
}

std::string
SequenceFileDialog::selectedDirectory() const
{
    std::string path = _requestedDir.toStdString();
    
    if (_relativePathsAllowed) {
        std::string pathName,pathValue;
        bool relative = getRelativeChoiceProjectPath(pathName, pathValue);
        if (relative) {
            Natron::Project::makeRelativeToVariable(pathName, pathValue, path);
        }
    }


#ifdef __NATRON_WIN32__
	QString ret = mapPathWithDriveLetterToPathWithNetworkShareName(path.c_str());
	path = ret.toStdString();
#endif
	
	return path;
}

QModelIndex
SequenceFileDialog::select(const QModelIndex & index)
{

    if ( index.isValid() && !_view->selectionModel()->isSelected(index) ) {
        _view->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }

    return index;
}

void
SequenceFileDialog::doubleClickOpen(const QModelIndex & /*index*/)
{
    QModelIndexList indexes = _view->selectionModel()->selectedRows();

    for (int i = 0; i < indexes.count(); ++i) {
        QModelIndex sourceIndex = indexes[i];
        if ( _model->isDir(  sourceIndex ) ) {
            _selectionLineEdit->setText(sourceIndex.data(FileSystemModel::FilePathRole).toString() + "/");
            break;
        }
    }

    openSelectedFiles();
}

void
SequenceFileDialog::seekUrl(const QUrl & url)
{
    setDirectory( url.toLocalFile() );
}

void
SequenceFileDialog::showFilterMenu()
{
    assert(_filterLineEdit->parentWidget());
    QPoint position( _filterLineEdit->parentWidget()->mapToGlobal(_filterLineEdit->pos()));

    position.ry() += _filterLineEdit->height();
    QList<QAction *> actions;

    //QFont font(appFont,appFontSize);
    QFontMetrics fm(font());
    
    QString defaultString = FileSystemModel::generateRegexpFilterFromFileExtensions(_filters);
    int w = fm.width(defaultString);
    double percent = _filterLineEdit->width() / (double)w ;
    if (percent < 1.) {
        int nCharsToRemove = (1. - percent) * defaultString.size();
        defaultString.remove(defaultString.size() - 1 - nCharsToRemove, nCharsToRemove);
    }
    
    QAction *defaultFilters = new QAction(defaultString,this);
    QObject::connect( defaultFilters, SIGNAL( triggered() ), this, SLOT( defaultFiltersSlot() ) );
    actions.append(defaultFilters);

    QAction *startSlash = new QAction("*/", this);
    QObject::connect( startSlash, SIGNAL( triggered() ), this, SLOT( starSlashFilterSlot() ) );
    actions.append(startSlash);

    QAction *empty = new QAction("*", this);
    QObject::connect( empty, SIGNAL( triggered() ), this, SLOT( emptyFilterSlot() ) );
    actions.append(empty);

    QAction *dotStar = new QAction(".*", this);
    QObject::connect( dotStar, SIGNAL( triggered() ), this, SLOT( dotStarFilterSlot() ) );
    actions.append(dotStar);


    if (actions.count() > 0) {
        Natron::Menu menu(_filterLineEdit);
        //menu.setFont(font);
        menu.addActions(actions);
      //  menu.setFixedSize( _filterLineEdit->width(),menu.sizeHint().height() );
        menu.exec(position);
    }
}

void
SequenceFileDialog::defaultFiltersSlot()
{
    QString filter = FileSystemModel::generateRegexpFilterFromFileExtensions(_filters);

    _filterLineEdit->setText(filter);
    applyFilter(filter);
}

void
SequenceFileDialog::dotStarFilterSlot()
{
    QString filter(".*");

    _filterLineEdit->setText(filter);
    applyFilter(filter);
}

void
SequenceFileDialog::starSlashFilterSlot()
{
    QString filter("*/");

    _filterLineEdit->setText(filter);
    applyFilter(filter);
}

void
SequenceFileDialog::emptyFilterSlot()
{
    QString filter("*");

    _filterLineEdit->setText(filter);
    applyFilter(filter);
}

void
SequenceFileDialog::applyFilter(QString filter)
{
    _model->setRegexpFilters(filter);
    QString currentDir = _requestedDir;
    setDirectory(currentDir);
}

UrlModel::UrlModel(const std::map<std::string,std::string>& vars, QObject *parent)
    : QStandardItemModel(parent)
      , fileSystemModel(0)
	  , envVars(vars)
{
}

bool
UrlModel::setData(const QModelIndex &index,
                  const QVariant &value,
                  int role)
{
    if (value.type() == QVariant::Url) {
        QUrl url = value.toUrl();
        QModelIndex dirIndex = fileSystemModel->index( url.toLocalFile() );
        QStandardItemModel::setData(index, QDir::toNativeSeparators( fileSystemModel->data(dirIndex, QFileSystemModel::FilePathRole).toString() ), Qt::ToolTipRole);
        //  QStandardItemModel::setData(index, fileSystemModel->data(dirIndex).toString());
        QVariant deco = fileSystemModel->data(dirIndex, Qt::DecorationRole);
        QStandardItemModel::setData(index, deco ,Qt::DecorationRole);
        QStandardItemModel::setData(index, url, UrlRole);

        return true;
    }

    return QStandardItemModel::setData(index, value, role);
}

void
UrlModel::setUrl(const QModelIndex &index,
                 const QUrl &url,
                 const QModelIndex &dirIndex)
{
    setData(index, url, UrlRole);
    if ( url.path().isEmpty() ) {
        setData(index, /*fileSystemModel->myComputer()*/"" );
        setData(index, fileSystemModel->myComputer(Qt::DecorationRole), Qt::DecorationRole);
    } else {
        QString newName;
        newName = QDir::toNativeSeparators( dirIndex.data(QFileSystemModel::FilePathRole).toString() ); //dirIndex.data().toString();
        QIcon newIcon;
        if ( !dirIndex.isValid() ) {

            newIcon = fileSystemModel->iconProvider()->icon(QFileIconProvider::Folder);
            newName = QFileInfo( url.toLocalFile() ).fileName();
            bool invalidUrlFound = false;
            for (unsigned int i = 0; i < invalidUrls.size(); ++i) {
                if (invalidUrls[i] == url) {
                    invalidUrlFound = true;
                    break;
                }
            }
            if (!invalidUrlFound) {
                invalidUrls.push_back(url);
            }
            //The bookmark is invalid then we set to false the EnabledRole
            setData(index, false, EnabledRole);
        } else {
			newIcon = qvariant_cast<QIcon>( dirIndex.data(Qt::DecorationRole) );
            //The bookmark is valid then we set to true the EnabledRole
            setData(index, true, EnabledRole);
        }

        // Make sure that we have at least 32x32 images
        const QSize size = newIcon.actualSize( QSize(32,32) );
        if (size.width() < 32) {
            QPixmap smallPixmap = newIcon.pixmap( QSize(32, 32) );
            newIcon.addPixmap( smallPixmap.scaledToWidth(32, Qt::SmoothTransformation) );
        }
		newName = mapUrlToDisplayName(newName);

        if (index.data().toString() != newName) {
            setData(index, newName);
        }
        QIcon oldIcon = qvariant_cast<QIcon>( index.data(Qt::DecorationRole) );
        if ( oldIcon.cacheKey() != newIcon.cacheKey() ) {
            setData(index, newIcon, Qt::DecorationRole);
        }
    }
}

void
UrlModel::setUrls(const std::vector<QUrl> &urls)
{
    removeRows( 0, rowCount() );
    invalidUrls.clear();
    watching.clear();
    addUrls(urls, 0);
}

void
UrlModel::addUrls(const std::vector<QUrl> &list,
                  int startRow,bool removeExisting)
{
    if (startRow == -1) {
        startRow = rowCount();
    }
    startRow = qMin( startRow,rowCount() );
    
    ///Remove already existant URLS
    ///Result is a pair new Url, clean url path
    std::vector<std::pair<QUrl,QString> > realList;
    for (U32 i = 0; i < list.size(); ++i) {
        
        QUrl url = list[i];
        if ( !url.isValid() || ( url.scheme() != QLatin1String("file") ) ) {
            continue;
        }
        
        const QString cleanUrl = QDir::cleanPath( url.toLocalFile() );
        
        QModelIndex idx = fileSystemModel->index(cleanUrl);
        if ( !fileSystemModel->isDir(idx) ) {
            continue;
        }
        
        bool found = false;
        
        int rc = rowCount();
        for (int j = 0; j < rc ; ++j) {
            if (index(j, 0).data(UrlRole).toUrl().toLocalFile() == cleanUrl) {
                if (removeExisting) {
                    if (j < startRow) {
                        --startRow;
                    }
                    removeRow(j);
                    ///remove it from wacthing too
                    for (U32 k = 0; k < watching.size(); ++k) {
                        if (watching[k].second == cleanUrl) {
                            watching.erase(watching.begin() + k);
                            break;
                        }
                    }
                } else {
                    found = true;
                }
                break;
            }
        }
        if (!found) {
			// If not found, make sure it is not already in the realList 
			for (std::size_t k = 0; k < realList.size(); ++k) {
				if (realList[k].second == cleanUrl) {
					found = true;
					break;
				}
			}
			if (!found) {
				realList.push_back(std::make_pair(QUrl::fromLocalFile(cleanUrl),cleanUrl));
			}
        }
    }
    if (realList.empty()) {
        return;
    }
    
    int rc = rowCount();
    if (startRow < (rc + (int)realList.size())) {
        insertRows(startRow,realList.size());
    }
    
    int row = startRow;
    for (int i = 0; i < (int)realList.size(); ++i) {
        if (row >= rowCount()) {
            insertRow(row);
        }
        QModelIndex idx = fileSystemModel->index(realList[i].second);
        setUrl(index(row, 0), realList[i].first, idx);
        watching.push_back( make_pair(idx, realList[i].second) );
        ++row;

    }
}

std::vector<QUrl>
UrlModel::urls() const
{
    std::vector<QUrl> list;

    for (int i = 0; i < rowCount(); ++i) {
        list.push_back( data(index(i, 0), UrlRole).toUrl() );
    }

    return list;
}

void
UrlModel::setFileSystemModel(QFileSystemModel *model)
{
    if (model == fileSystemModel) {
        return;
    }
    if (fileSystemModel != 0) {
        disconnect( model, SIGNAL( dataChanged(QModelIndex,QModelIndex) ),
                    this, SLOT( dataChanged(QModelIndex,QModelIndex) ) );
        disconnect( model, SIGNAL( layoutChanged() ),
                    this, SLOT( layoutChanged() ) );
        disconnect( model, SIGNAL( rowsRemoved(QModelIndex,int,int) ),
                    this, SLOT( layoutChanged() ) );
    }
    fileSystemModel = model;
    if (fileSystemModel != 0) {
        connect( model, SIGNAL( dataChanged(QModelIndex,QModelIndex) ),
                 this, SLOT( dataChanged(QModelIndex,QModelIndex) ) );
        connect( model, SIGNAL( layoutChanged() ),
                 this, SLOT( layoutChanged() ) );
        connect( model, SIGNAL( rowsRemoved(QModelIndex,int,int) ),
                 this, SLOT( layoutChanged() ) );
    }
    clear();
    insertColumns(0, 1);
}

void
UrlModel::dataChanged(const QModelIndex &topLeft,
                      const QModelIndex &bottomRight)
{
    QModelIndex parent = topLeft.parent();

    for (int i = 0; i < (int)watching.size(); ++i) {
        QModelIndex index = watching[i].first;
        if ( ( index.row() >= topLeft.row() )
             && ( index.row() <= bottomRight.row() )
             && ( index.column() >= topLeft.column() )
             && ( index.column() <= bottomRight.column() )
             && ( index.parent() == parent) ) {
            changed(watching[i].second);
        }
    }
}

void
UrlModel::layoutChanged()
{
    QStringList paths;

    for (int i = 0; i < (int)watching.size(); ++i) {
        paths.append(watching[i].second);
    }
    watching.clear();
    for (int i = 0; i < paths.count(); ++i) {
        QString path = paths.at(i);
        QModelIndex newIndex = fileSystemModel->index(path);
        watching.push_back( std::pair<QModelIndex, QString>(newIndex, path) );
        if ( newIndex.isValid() ) {
            changed(path);
        }
    }
}

void
UrlModel::changed(const QString &path)
{
    for (int i = 0; i < rowCount(); ++i) {
        QModelIndex idx = index(i, 0);
        if (idx.data(UrlRole).toUrl().toLocalFile() == path) {
            setData( idx, idx.data(UrlRole).toUrl() );
        }
    }
}


void
UrlModel::removeRowIndex(const QModelIndex& index)
{
    QString urlPath = index.data(UrlRole).toUrl().path();
    if (!urlPath.isEmpty()) {
        removeRow( index.row() );
        for (U32 j = 0; j < watching.size(); ++j) {
            if (watching[j].second == urlPath) {
                watching.erase(watching.begin() + j);
                break;
            }
        }
    }
}


FavoriteItemDelegate::FavoriteItemDelegate()
: QStyledItemDelegate()
{
    

}

QString
UrlModel::mapUrlToDisplayName(const QString& originalName)
{
	QString str = originalName;

#ifdef __NATRON_WIN32__
	//On Windows strings are stored with backslashes
	str.replace(QChar('\\'),QChar('/'));
#endif

	///if str ends with '/' remove it
	if (str.size() > 1 && (str.endsWith('/') || str.endsWith('\\'))) {
		str = str.remove(str.size() - 1, 1);
	}

	std::map<std::string,std::string>::const_iterator isEnvVar = envVars.end();
	for (std::map<std::string,std::string>::const_iterator it = envVars.begin(); it != envVars.end(); ++it) {
		///if it->second ends with '/' remove it
		std::string stdVar = it->second;
		Natron::Project::expandVariable(envVars, stdVar);
		QString var(stdVar.c_str());
		if (var.size() > 1 && (var.endsWith('/') || var.endsWith('\\'))) {
			var = var.remove(var.size() - 1, 1);
		}
		if (var == str) {
			isEnvVar = it;
			break;
		}
	} 
	if (isEnvVar == envVars.end()) {
		QModelIndex modelIndex = fileSystemModel->index(str);
		if ( !modelIndex.isValid() ) {
			return originalName;
		}
		str = modelIndex.data().toString();
	} else {
		str.clear();
		str.append('[');
		str.append(isEnvVar->first.c_str());
		str.append(']');
	}
	return str;
}

void
FavoriteItemDelegate::paint(QPainter * painter,
                            const QStyleOptionViewItem & option,
                            const QModelIndex & index) const
{
	assert(index.isValid());
	return QStyledItemDelegate::paint(painter,option,index);
	
	/*
	The code below is deactivated because it crashes on Windows when trying to call drawPixmap and drawText
	if the QModelIndex is internally pointing to a UNC path. The only viable solution found was to use the 
	default implementation.
	
	if (index.column() == 0) {
        QString str = index.data().toString();

		QFileInfo fileInfo(str);
		QIcon icon = _model->iconProvider()->icon(fileInfo);
		int totalSize = option.rect.width();
		int iconSize = 0;
		QRect r;
		if (!icon.isNull()) {
			
		    iconSize = option.decorationSize.width();
			QRect iconRect( option.rect.x(),option.rect.y(),iconSize,option.rect.height() );
			
			if (option.state & QStyle::State_Selected) {
				painter->fillRect( option.rect, option.palette.highlight() );
			}

			QPixmap pix = icon.pixmap(icon.actualSize(QSize(iconRect.width(),iconRect.height())));
			if (!pix.isNull()) {
				painter->drawPixmap(iconRect,pix);
			}
		}
		int textSize = totalSize - iconSize;
		QRect textRect( option.rect.x() + iconSize,option.rect.y(),textSize,option.rect.height() );
		painter->drawText(textRect,Qt::TextSingleLine,str,&r);
    } else {
        QStyledItemDelegate::paint(painter,option,index);
    }*/
}


FavoriteView::FavoriteView(Gui* gui,QWidget *parent)
    : QListView(parent)
      , _gui(gui)
      , urlModel(0)
      , _itemDelegate(0)
{
    // setAttribute(Qt::WA_MacShowFocusRect,0);
}

void
FavoriteView::setModelAndUrls(QFileSystemModel *model,
                              const std::vector<QUrl> &newUrls)
{
    setIconSize( QSize(24,24) );
    setUniformItemSizes(true);
    assert(!urlModel);

	std::map<std::string,std::string> envVars;
	_gui->getApp()->getProject()->getEnvironmentVariables(envVars);

    urlModel = new UrlModel(envVars,this);
    urlModel->setFileSystemModel(model);
    assert(!_itemDelegate);
    _itemDelegate = new FavoriteItemDelegate();
    setItemDelegate(_itemDelegate);
    setModel(urlModel);
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    setDragDropMode(QAbstractItemView::DragDrop);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect( this, SIGNAL( customContextMenuRequested(QPoint) ),
             this, SLOT( showMenu(QPoint) ) );
    urlModel->setUrls(newUrls);
    setCurrentIndex( this->model()->index(0,0) );
    connect( selectionModel(), SIGNAL( currentChanged(QModelIndex,QModelIndex) ),
             this, SLOT( clicked(QModelIndex) ) );
}

FavoriteView::~FavoriteView()
{
    delete urlModel;
    delete _itemDelegate;
}

QSize
FavoriteView::sizeHint() const
{
    return QSize( 150,QListView::sizeHint().height() ); //QListView::sizeHint();
}

void
FavoriteView::selectUrl(const QUrl &url)
{
    disconnect( selectionModel(), SIGNAL( currentChanged(QModelIndex,QModelIndex) ),
                this, SLOT( clicked(QModelIndex) ) );

    selectionModel()->clear();
    for (int i = 0; i < model()->rowCount(); ++i) {
        if (model()->index(i, 0).data(UrlModel::UrlRole).toUrl() == url) {
            selectionModel()->select(model()->index(i, 0), QItemSelectionModel::Select);
            break;
        }
    }

    connect( selectionModel(), SIGNAL( currentChanged(QModelIndex,QModelIndex) ),
             this, SLOT( clicked(QModelIndex) ) );
}

void
FavoriteView::removeEntry()
{
    QList<QModelIndex> idxs = selectionModel()->selectedIndexes();
    QList<QPersistentModelIndex> indexes;
    for (int i = 0; i < idxs.count(); ++i) {
        indexes.append( idxs.at(i) );
    }

    for (int i = 0; i < indexes.count(); ++i) {
        urlModel->removeRowIndex(indexes[i]);
    }
}

void
FavoriteView::rename()
{
    QModelIndex index;

    QList<QModelIndex> idxs = selectionModel()->selectedIndexes();
    QList<QPersistentModelIndex> indexes;
    for (int i = 0; i < idxs.count(); ++i) {
        indexes.append( idxs.at(i) );
    }
    for (int i = 0; i < indexes.count(); ++i) {
        if ( !indexes.at(i).data(UrlModel::UrlRole).toUrl().path().isEmpty() ) {
            index = indexes.at(i);
            break;
        }
    }
    QString newName;
    QInputDialog dialog(this);
    dialog.setLabelText( tr("Favorite Name:") );
    dialog.setWindowTitle( tr("Rename Favorite") );
    if ( dialog.exec() ) {
        newName = dialog.textValue();
    }
    rename(index,newName);
}

void
FavoriteView::rename(const QModelIndex & index,
                     const QString & name)
{
    model()->setData(index,name,Qt::EditRole);
}

void
FavoriteView::editUrl()
{
    QModelIndex index;

    QList<QModelIndex> idxs = selectionModel()->selectedIndexes();
    QList<QPersistentModelIndex> indexes;
    for (int i = 0; i < idxs.count(); ++i) {
        indexes.append( idxs.at(i) );
    }
    for (int i = 0; i < indexes.count(); ++i) {
        if ( !indexes.at(i).data(UrlModel::UrlRole).toUrl().path().isEmpty() ) {
            index = indexes.at(i);
            break;
        }
    }
    QString newName;
    QInputDialog dialog(this);
    dialog.setLabelText( tr("Folder Path:") );
    dialog.setWindowTitle( tr("Change Folder Path") );
    if ( dialog.exec() ) {
        newName = dialog.textValue();
    }
    QString cleanpath = QDir::cleanPath(newName);
    QUrl url = QUrl::fromLocalFile(newName);
    UrlModel *myurlModel = dynamic_cast<UrlModel*>( model() );
    assert(myurlModel);
    QFileSystemModel* fileSystemModel = myurlModel->getFileSystemModel();
    assert(fileSystemModel);
    QModelIndex idx = fileSystemModel->index(cleanpath);
    if ( !fileSystemModel->isDir(idx) ) {
        return;
    }
    myurlModel->setUrl(index, url, idx);
}

void
FavoriteView::clicked(const QModelIndex &index)
{
    QUrl url = model()->index(index.row(), 0).data(UrlModel::UrlRole).toUrl();
    Q_EMIT urlRequested(url);

    selectUrl(url);
}

void
FavoriteView::showMenu(const QPoint &position)
{
    QList<QAction *> actions;
    if ( indexAt(position).isValid() ) {
        QAction *removeAction = new QAction(tr("Remove"), this);
        if ( indexAt(position).data(UrlModel::UrlRole).toUrl().path().isEmpty() ) {
            removeAction->setEnabled(false);
        }
        connect( removeAction, SIGNAL( triggered() ), this, SLOT( removeEntry() ) );
        actions.append(removeAction);

        QAction *editAction = new QAction(tr("Edit path"), this);
        if ( indexAt(position).data(UrlModel::UrlRole).toUrl().path().isEmpty() ) {
            editAction->setEnabled(false);
        }
        connect( editAction, SIGNAL( triggered() ), this, SLOT( editUrl() ) );
        actions.append(editAction);
    }
    if (actions.count() > 0) {
        Natron::Menu menu(this);
        //menu.setFont(QFont(appFont,appFontSize));
        menu.addActions(actions);
        menu.exec( mapToGlobal(position) );
    }
}

void
FavoriteView::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Backspace) {
        removeEntry();
    }
    QListView::keyPressEvent(e);
}

QStringList
UrlModel::mimeTypes() const
{
    return QStringList( QLatin1String("text/uri-list") );
}

Qt::ItemFlags
UrlModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QStandardItemModel::flags(index);

    if ( index.isValid() ) {
        flags &= ~Qt::ItemIsEditable;
        // ### some future version could support "moving" urls onto a folder
        flags &= ~Qt::ItemIsDropEnabled;
    }

    if ( index.data(Qt::DecorationRole).isNull() ) {
        flags &= ~Qt::ItemIsEnabled;
    }

    return flags;
}

QMimeData*
UrlModel::mimeData(const QModelIndexList &indexes) const
{
    QList<QUrl> list;
    for (int i = 0; i < indexes.count(); ++i) {
        if (indexes.at(i).column() == 0) {
            list.append( indexes.at(i).data(UrlRole).toUrl() );
        }
    }
    QMimeData *data = new QMimeData();
    data->setUrls(list);

    return data;
}

bool
UrlModel::canDrop(QDragEnterEvent* e)
{
    if ( !e->mimeData()->formats().contains( mimeTypes().first() ) ) {
        return false;
    }

    const QList<QUrl> list = e->mimeData()->urls();
    for (int i = 0; i < list.count(); ++i) {
        QModelIndex idx = fileSystemModel->index( Natron::toLocalFileUrlFixed(list.at(i)).toLocalFile() );
        if ( !fileSystemModel->isDir(idx) ) {
            return false;
        }
    }

    return true;
}

bool
UrlModel::dropMimeData(const QMimeData *data,
                       Qt::DropAction,
                       int row,
                       int,
                       const QModelIndex &)
{
    if ( !data->formats().contains( mimeTypes().first() ) ) {
        return false;
    }
    std::vector<QUrl> urls;
    for (int i = 0; i < data->urls().count(); ++i) {
        urls.push_back( data->urls().at(i) );
    }
    addUrls(urls, row,true);

    return true;
}



void
FavoriteView::dragEnterEvent(QDragEnterEvent* e)
{
    if ( urlModel->canDrop(e) ) {
        QListView::dragEnterEvent(e);
    }
}

FileDialogComboBox::FileDialogComboBox(SequenceFileDialog *p,QWidget *parent)
: QComboBox(parent)
, urlModel(new UrlModel(std::map<std::string,std::string>(),this))
, dialog(p)
, doResize(false)
{
    urlModel->setFileSystemModel( p->getLookingFileSystemModel() );
    setModel(urlModel);
    QObject::connect(this, SIGNAL(currentIndexChanged(int)), this , SLOT(onCurrentIndexChanged(int)));
}

void
FileDialogComboBox::onCurrentIndexChanged(int index)
{
    if (doResize && index >= 0 && index < count()) {
        updateGeometry();
        doResize = false;
    }
}

QSize
FileDialogComboBox::sizeHint() const
{
    int index = currentIndex();
    if (index >= 0 && index < count()) {
        QFontMetrics fm = fontMetrics();
        QString txt = itemText(index);
        int w = fm.width(txt);
        return QSize(w + 10, fm.height() * 1.5);
    }
    return QComboBox::sizeHint();
}

void
FileDialogComboBox::showPopup()
{
    if (model()->rowCount() > 1) {
        QComboBox::showPopup();
    }
    urlModel->setUrls( std::vector<QUrl>() );
    std::vector<QUrl> list;
    QModelIndex idx = dialog->getFileSystemModel()->index( dialog->rootPath() );
    while ( idx.isValid() ) {
        QUrl url = QUrl::fromLocalFile( idx.data(QFileSystemModel::FilePathRole).toString() );
        if ( url.isValid() ) {
            list.push_back(url);
        }
        idx = idx.parent();
    }
    // add "my computer"
    list.push_back( QUrl::fromLocalFile( QLatin1String("") ) );
    urlModel->addUrls(list, 0);
    idx = model()->index(model()->rowCount() - 1, 0);

    // append history
    QList<QUrl> urls;
    for (int i = 0; i < m_history.count(); ++i) {
        QUrl path = QUrl::fromLocalFile( m_history.at(i) );
        if ( !urls.contains(path) ) {
            urls.prepend(path);
        }
    }
    if (urls.count() > 0) {
        model()->insertRow( model()->rowCount() );
        idx = model()->index(model()->rowCount() - 1, 0);
        // ### TODO maybe add a horizontal line before this
        model()->setData( idx,QObject::tr("Recent Places") );
        QStandardItemModel *m = qobject_cast<QStandardItemModel*>( model() );
        if (m) {
            Qt::ItemFlags flags = m->flags(idx);
            flags &= ~Qt::ItemIsEnabled;
            m->item( idx.row(), idx.column() )->setFlags(flags);
        }
        std::vector<QUrl> stdUrls;
        for (int i = 0; i < urls.count(); ++i) {
            stdUrls.push_back( urls.at(i) );
        }
        urlModel->addUrls(stdUrls, -1);
    }
    setCurrentIndex(0);
    
    doResize = true;
    QComboBox::showPopup();
} // showPopup

void
FileDialogComboBox::setHistory(const QStringList &paths)
{
    m_history = paths;
    // Only populate the first item, showPopup will populate the rest if needed
    std::vector<QUrl> list;
    QModelIndex idx = dialog->getFileSystemModel()->index( dialog->rootPath() );
    //On windows the popup display the "C:\", convert to nativeSeparators
    QUrl url = QUrl::fromLocalFile( QDir::toNativeSeparators( idx.data(QFileSystemModel::FilePathRole).toString() ) );
    if ( url.isValid() ) {
        list.push_back(url);
    }
    urlModel->setUrls(list);
}

void
FileDialogComboBox::paintEvent(QPaintEvent* /*e*/)
{
    QStylePainter painter(this);
    QColor c(255,255,255,255);

    painter.setPen(c);
    // draw the combobox frame, focusrect and selected etc.
    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    QRect editRect = style()->subControlRect(QStyle::CC_ComboBox, &opt,
                                             QStyle::SC_ComboBoxEditField, this);
    int size = editRect.width() - opt.iconSize.width() - 4;
    opt.currentText = opt.fontMetrics.elidedText(opt.currentText, Qt::ElideMiddle, size);
    painter.drawComplexControl(QStyle::CC_ComboBox, opt);

    // draw the icon and text
    painter.drawControl(QStyle::CE_ComboBoxLabel, opt);
}

std::vector< boost::shared_ptr<SequenceParsing::SequenceFromFiles> >
SequenceFileDialog::fileSequencesFromFilesList(const QStringList & files,
                                               const QStringList & supportedFileTypes)
{
    std::vector< boost::shared_ptr<SequenceParsing::SequenceFromFiles> > sequences;

    for (int i = 0; i < files.size(); ++i) {
        SequenceParsing::FileNameContent fileContent( files.at(i).toStdString() );

        if ( !supportedFileTypes.contains(fileContent.getExtension().c_str(),Qt::CaseInsensitive) ) {
            continue;
        }

        bool found = false;

        for (U32 j = 0; j < sequences.size(); ++j) {
            if ( sequences[j]->tryInsertFile(fileContent) ) {
                found = true;
                break;
            }
        }
        if (!found) {
            boost::shared_ptr<SequenceParsing::SequenceFromFiles> seq( new SequenceParsing::SequenceFromFiles(fileContent,false) );
            sequences.push_back(seq);
        }
    }

    return sequences;
}

void
SequenceFileDialog::appendFilesFromDirRecursively(QDir* currentDir,
                                                  QStringList* files)
{
    QStringList entries = currentDir->entryList();

    for (int i = 0; i < entries.size(); ++i) {
        const QString & e = entries.at(i);

        //ignore dot and dotdot
        if ( (e == ".") || (e == "..") ) {
            continue;
        }

        QString entryWithPath = currentDir->absoluteFilePath(e);

        //if it is a file, recurse
        QDir d(entryWithPath);
        if ( d.exists() ) {
            appendFilesFromDirRecursively(&d, files);
            continue;
        }

        //else if it is a file, append it
        if ( QFile::exists(entryWithPath) ) {
            files->append(entryWithPath);
        }
    }
}

void
SequenceFileDialog::onSelectionLineEditing(const QString & text)
{
    if (_dialogMode != eFileDialogModeSave) {
        return;
    }
    QString textCpy = text;
    QString extension = Natron::removeFileExtension(textCpy);
    for (int i = 0; i < _fileExtensionCombo->count(); ++i) {
        if (_fileExtensionCombo->itemText(i) == extension) {
            _fileExtensionCombo->setCurrentIndex_no_emit(i);
            break;
        }
    }
}

void
SequenceFileDialog::onTogglePreviewButtonClicked(bool toggled)
{
    _togglePreviewButton->setDown(toggled);
    assert(_preview);
    if (!_preview->viewerNode) {
        createViewerPreviewNode();
    }
    if (toggled) {
        if (!_preview->viewerUI->parentWidget()) {
            _centerAreaLayout->addWidget(_preview->viewerUI);
        }
        if (!_preview->viewerUI->isVisible()) {
            _preview->viewerUI->setVisible(true);
        }
        refreshPreviewAfterSelectionChange();
    } else {
        if (_preview->viewerUI->isVisible()) {
            _preview->viewerUI->setVisible(false);
        }
    }
}

void
SequenceFileDialog::createViewerPreviewNode()
{
    CreateNodeArgs args(PLUGINID_NATRON_VIEWER,
                        "",
                        -1,-1,
                        false,
                        INT_MIN,
                        INT_MIN,
                        false,
                        false,
                        false,
                        NATRON_FILE_DIALOG_PREVIEW_VIEWER_NAME,
                        CreateNodeArgs::DefaultValuesList(),
                        _gui->getApp()->getProject());
    
    _preview->viewerNodeInternal = _gui->getApp()->createNode(args);
    assert(_preview->viewerNodeInternal);
    boost::shared_ptr<NodeGuiI> viewerNodeGui = _preview->viewerNodeInternal->getNodeGui();
    _preview->viewerNode = boost::dynamic_pointer_cast<NodeGui>(viewerNodeGui);
    assert(_preview->viewerNode);
    _preview->viewerNode->hideGui();
    
    ViewerInstance* viewerInstance = dynamic_cast<ViewerInstance*>(_preview->viewerNodeInternal->getLiveInstance());
    assert(viewerInstance);
    if (!viewerInstance) {
        // coverity[dead_error_line]
        return;
    }
    ViewerGL* viewerGL = dynamic_cast<ViewerGL*>(viewerInstance->getUiContext());
    assert(viewerGL);
    if (!viewerGL) {
        // coverity[dead_error_line]
        return;
    }
    _preview->viewerUI = viewerGL->getViewerTab();
    assert(_preview->viewerUI);
    if (!_preview->viewerUI) {
        return;
    }
    _preview->viewerUI->setAsFileDialogViewer();
    
    ///Set a custom timeline so that it is not in synced with the rest of the viewers of the app
    boost::shared_ptr<TimeLine> newTimeline(new TimeLine(NULL));
    _preview->viewerUI->setCustomTimeline(newTimeline);
    _preview->viewerUI->setClipToProject(false);
    _preview->viewerUI->setLeftToolbarVisible(false);
    _preview->viewerUI->setRightToolbarVisible(false);
    _preview->viewerUI->setTopToolbarVisible(false);
    _preview->viewerUI->setInfobarVisible(false);
    _preview->viewerUI->setPlayerVisible(false);
    TabWidget* parent = dynamic_cast<TabWidget*>(_preview->viewerUI->parentWidget());
    if (parent) {
        parent->removeTab(_preview->viewerUI,true);
    }
    _preview->viewerUI->setParent(NULL);
}

boost::shared_ptr<NodeGui>
SequenceFileDialog::findOrCreatePreviewReader(const std::string& filetype)
{
    std::map<std::string,std::string> readersForFormat;
    appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&readersForFormat);
    if ( !filetype.empty() ) {
        std::map<std::string,std::string>::iterator found = readersForFormat.find(filetype);
        if ( found == readersForFormat.end() ) {
            return boost::shared_ptr<NodeGui>();
        }
        std::map<std::string,std::pair< boost::shared_ptr<Natron::Node>, boost::shared_ptr<NodeGui> > >::iterator foundReader = _preview->readerNodes.find(found->second);
        if (foundReader == _preview->readerNodes.end()) {
            
            CreateNodeArgs args(found->second.c_str(),
                                "",
                                -1,-1,
                                false,
                                INT_MIN,
                                INT_MIN,
                                false,
                                false,
                                false,
                                QString(NATRON_FILE_DIALOG_PREVIEW_READER_NAME) +  QString(found->first.c_str()),
                                CreateNodeArgs::DefaultValuesList(),
                                _gui->getApp()->getProject());
            
            boost::shared_ptr<Natron::Node> reader = _gui->getApp()->createNode(args);
            boost::shared_ptr<NodeGuiI> readerGui_i = reader->getNodeGui();
            boost::shared_ptr<NodeGui> readerGui = boost::dynamic_pointer_cast<NodeGui>(readerGui_i);
            assert(readerGui);
            readerGui->hideGui();
            _preview->readerNodes.insert(std::make_pair(found->second,std::make_pair(reader,readerGui)));
            return readerGui;
        } else {
            return foundReader->second.second;
        }
    }
    return  boost::shared_ptr<NodeGui>();
}

void
SequenceFileDialog::refreshPreviewAfterSelectionChange()
{
    if (!_preview->viewerUI->isVisible()) {
        return;
    }
    
    std::string pattern = selectedFiles();
    QString qpattern( pattern.c_str() );
    std::string ext = Natron::removeFileExtension(qpattern).toLower().toStdString();
    assert(_preview->viewerNode);
    
    boost::shared_ptr<Natron::Node> currentInput = _preview->viewerNode->getNode()->getInput(0);
    if (currentInput) {
        _preview->viewerNode->getNode()->disconnectInput(0);
    }
    
    _gui->getApp()->getProject()->setAutoSetProjectFormatEnabled(false);

    boost::shared_ptr<NodeGui> reader = findOrCreatePreviewReader(ext);
    if (reader) {
        const std::vector<boost::shared_ptr<KnobI> > & knobs = reader->getNode()->getKnobs();
        for (U32 i = 0; i < knobs.size(); ++i) {
            KnobFile* fileKnob = dynamic_cast<KnobFile*>(knobs[i].get());
            if ( fileKnob && fileKnob->isInputImageFile() ) {
                fileKnob->setValue(pattern,0);
            }
        }
        _preview->viewerNode->getNode()->connectInput(reader->getNode(), 0);
        
        double firstFrame,lastFrame;
        reader->getNode()->getLiveInstance()->getFrameRange_public(reader->getNode()->getHashValue(), &firstFrame, &lastFrame);
        _preview->viewerUI->setTimelineBounds(firstFrame, lastFrame);
        _preview->viewerUI->centerOn(firstFrame,lastFrame);

    }
    _preview->viewerUI->getInternalNode()->renderCurrentFrame(false);
}

///Reset everything as it was prior to the dialog being opened, also avoid the nodes being deleted
void
SequenceFileDialog::teardownPreview()
{
    if (_preview && _preview->viewerUI) {
        ///Don't delete the viewer when closing the dialog!
        _centerAreaLayout->removeWidget(_preview->viewerUI);
        _preview->viewerUI->setParent(NULL);
        _preview->viewerUI->hide();
        assert(_gui);
        bool autoSetProjectFormatEnabled = _gui->getApp()->getProject()->isAutoSetProjectFormatEnabled();
        if (autoSetProjectFormatEnabled != _wasAutosetProjectFormatEnabled) {
            _gui->getApp()->getProject()->setAutoSetProjectFormatEnabled(_wasAutosetProjectFormatEnabled);
        }
    }
}

void
SequenceFileDialog::closeEvent(QCloseEvent* e)
{
    teardownPreview();
    QDialog::closeEvent(e);
}

void
SequenceFileDialog::done(int r)
{
    teardownPreview();
    QDialog::done(r);
}