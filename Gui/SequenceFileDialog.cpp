//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */


#include "SequenceFileDialog.h"

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
#include <QLabel>
#include <QFileIconProvider>
#include <QInputDialog>
#include <QSplitter>
#include <QtGui/QIcon>
#include <QDialog>
#include <QFileDialog>
#include <QtCore/QRegExp>
#include <QtGui/QKeyEvent>
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
#include <QMenu>

#include <QtCore/QEvent>
#include <QtCore/QMimeData>
#include <QtConcurrentRun>

#include <QtCore/QSettings>

#include "Global/AppManager.h"
#include "Global/QtCompat.h"
#include "Gui/Button.h"
#include "Gui/LineEdit.h"
#include "Gui/ComboBox.h"
#include "Global/MemoryInfo.h"

using std::make_pair;
using namespace Natron;

static inline bool nocase_equal_char (char c1, char c2)
{
    return toupper(c1) == toupper(c2);
}

static inline bool nocase_equal_string (const std::string &s1, const std::string &s2)
{
    return s1.size() == s2.size() &&        // ensure same sizes
    std::equal (s1.begin(),s1.end(),      // first source string
                s2.begin(),               // second source string
                nocase_equal_char);
}



namespace {
    struct NameMappingCompareFirst {
        NameMappingCompareFirst(const QString &val) : val_(val) {}
      bool operator()(const SequenceFileDialog::NameMappingElement& elem) const {
            return val_ == elem.first;
        }
    private:
        QString val_;
    };

    struct NameMappingCompareFirstNoPath {
        NameMappingCompareFirstNoPath(const QString &val) : val_(val) {}
        bool operator()(const SequenceFileDialog::NameMappingElement& elem) const {
            return val_ == SequenceFileDialog::removePath(elem.first);
        }
    private:
        QString val_;
    };
}

SequenceFileDialog::SequenceFileDialog(QWidget* parent, // necessary to transmit the stylesheet to the dialog
                                       const std::vector<std::string>& filters, // the user accepted file types
                                       bool isSequenceDialog, // true if this dialog can display sequences
                                       FileDialogMode mode, // if it is an open or save dialog
                                       const std::string& currentDirectory) // the directory to show first
: QDialog(parent)
, _frameSequences()
, _nameMappingMutex()
, _nameMapping()
, _filters(filters)
, _view(0)
, _itemDelegate(0)
, _proxy(0)
, _model(0)
, _mainLayout(0)
, _requestedDir()
, _lookInLabel(0)
, _lookInCombobox(0)
, _previousButton(0)
, _nextButton(0)
, _upButton(0)
, _createDirButton(0)
, _previewButton(0)
, _openButton(0)
, _cancelButton(0)
, _addFavoriteButton(0)
, _removeFavoriteButton(0)
, _selectionLineEdit(0)
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
{
    setWindowFlags(Qt::Window);
    _mainLayout = new QVBoxLayout(this);
    setLayout(_mainLayout);
    /*Creating view and setting directory*/
    _view =  new SequenceDialogView(this);
    _itemDelegate = new SequenceItemDelegate(this);
    _proxy = new SequenceDialogProxyModel(this);
    _model = new QFileSystemModel();
    _proxy->setSourceModel(_model);
    _view->setModel(_proxy);
    _view->setItemDelegate(_itemDelegate);
    QObject::connect(_model,SIGNAL(directoryLoaded(QString)),this,SLOT(updateView(QString)));
    QObject::connect(_view, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(doubleClickOpen(QModelIndex)));
    
    /*creating GUI*/
    _buttonsWidget = new QWidget(this);
    _buttonsLayout = new QHBoxLayout(_buttonsWidget);
    _buttonsWidget->setLayout(_buttonsLayout);
    if(mode == OPEN_DIALOG)
        _lookInLabel = new QLabel("Look in:",_buttonsWidget);
    else
        _lookInLabel = new QLabel("Save in:",_buttonsWidget);
    _buttonsLayout->addWidget(_lookInLabel);
    
    _lookInCombobox = new FileDialogComboBox(_buttonsWidget);
    _buttonsLayout->addWidget(_lookInCombobox);
    _lookInCombobox->setFileDialogPointer(this);
    QObject::connect(_lookInCombobox, SIGNAL(activated(QString)), this, SLOT(goToDirectory(QString)));
    _lookInCombobox->setInsertPolicy(QComboBox::NoInsert);
    _lookInCombobox->setDuplicatesEnabled(false);
    
    _lookInCombobox->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Fixed);
    
    _buttonsLayout->addStretch();
    
    
    _previousButton = new Button(style()->standardIcon(QStyle::SP_ArrowBack),"",_buttonsWidget);
    _buttonsLayout->addWidget(_previousButton);
    QObject::connect(_previousButton, SIGNAL(clicked()), this, SLOT(previousFolder()));
    
    _nextButton = new Button(style()->standardIcon(QStyle::SP_ArrowForward),"",_buttonsWidget);
    _buttonsLayout->addWidget(_nextButton);
    QObject::connect(_nextButton, SIGNAL(clicked()), this, SLOT(nextFolder()));
    
    _upButton = new Button(style()->standardIcon(QStyle::SP_ArrowUp),"",_buttonsWidget);
    _buttonsLayout->addWidget(_upButton);
    QObject::connect(_upButton, SIGNAL(clicked()), this, SLOT(parentFolder()));
    
    _createDirButton = new Button(style()->standardIcon(QStyle::SP_FileDialogNewFolder),"",_buttonsWidget);
    _buttonsLayout->addWidget(_createDirButton);
    QObject::connect(_createDirButton, SIGNAL(clicked()), this, SLOT(createDir()));
    
    
    _previewButton = new Button("preview",_buttonsWidget);
    _previewButton->setVisible(false);/// @todo Implement preview mode for the file dialog
    _buttonsLayout->addWidget(_previewButton);
    
    
    _mainLayout->addWidget(_buttonsWidget);
    
    /*creating center*/
    _centerSplitter = new QSplitter(this);
    _centerSplitter->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    
    _favoriteWidget = new QWidget(this);
    _favoriteLayout = new QVBoxLayout(_favoriteWidget);
    _favoriteWidget->setLayout(_favoriteLayout);
    _favoriteView = new FavoriteView(this);
    QObject::connect(_favoriteView,SIGNAL(urlRequested(QUrl)),this,SLOT(seekUrl(QUrl)));
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
    QObject::connect(_addFavoriteButton, SIGNAL(clicked()), this, SLOT(addFavorite()));
    
    _removeFavoriteButton = new Button("-",this);
    _removeFavoriteButton->setMaximumSize(20,20);
    _favoriteButtonsLayout->addWidget(_removeFavoriteButton);
    QObject::connect(_removeFavoriteButton, SIGNAL(clicked()), _favoriteView, SLOT(removeEntry()));
    
    _favoriteButtonsLayout->addStretch();
    
    _favoriteLayout->addWidget(_favoriteButtonsWidget);
    
    
    _centerSplitter->addWidget(_favoriteWidget);
    _centerSplitter->addWidget(_view);
    
    _mainLayout->addWidget(_centerSplitter);
    
    /*creating selection widget*/
    _selectionWidget = new QWidget(this);
    _selectionLayout = new QHBoxLayout(_selectionWidget);
    _selectionLayout->setContentsMargins(0, 0, 0, 0);
    _selectionWidget->setLayout(_selectionLayout);
    
    _sequenceButton = new ComboBox(_buttonsWidget);
    _sequenceButton->addItem("Sequence:");
    _sequenceButton->addItem("File:");
    if(isSequenceDialog){
        _sequenceButton->setCurrentIndex(0);
    }else{
        _sequenceButton->setCurrentIndex(1);
    }
    QObject::connect(_sequenceButton,SIGNAL(currentIndexChanged(int)),this,SLOT(sequenceComboBoxSlot(int)));
    _selectionLayout->addWidget(_sequenceButton);
    
    if(!isSequenceDialog){
        _sequenceButton->setVisible(false);
    }
    _selectionLineEdit = new LineEdit(_selectionWidget);
    _selectionLayout->addWidget(_selectionLineEdit);
    
    if(mode==SequenceFileDialog::OPEN_DIALOG)
        _openButton = new Button("Open",_selectionWidget);
    else
        _openButton = new Button("Save",_selectionWidget);
    _selectionLayout->addWidget(_openButton);
    QObject::connect(_openButton, SIGNAL(pressed()), this, SLOT(openSelectedFiles()));
    
    _mainLayout->addWidget(_selectionWidget);
    
    /*creating filter zone*/
    _filterLineWidget = new QWidget(this);
    _filterLineLayout = new QHBoxLayout(_filterLineWidget);
    _filterLineLayout->setContentsMargins(0, 0, 0, 0);
    _filterLineWidget->setLayout(_filterLineLayout);
    
    if(_dialogMode == OPEN_DIALOG){
        _filterLabel = new QLabel("Filter:",_filterLineWidget);
    }else{
        _filterLabel = new QLabel("File type:",_filterLineWidget);
    }
    _filterLineLayout->addWidget(_filterLabel);
    
    _filterWidget = new QWidget(_filterLineWidget);
    _filterLayout = new QHBoxLayout(_filterWidget);
    _filterWidget->setLayout(_filterLayout);
    _filterLayout->setContentsMargins(0,0,0,0);
    _filterLayout->setSpacing(0);
    
    QString filter = generateStringFromFilters();
    if(_dialogMode == OPEN_DIALOG){
        _filterLineEdit = new LineEdit(_filterWidget);
        _filterLayout->addWidget(_filterLineEdit);
        _filterLineEdit->setText(filter);
        QSize buttonSize(15,_filterLineEdit->sizeHint().height());
        QPixmap pixDropDown;
        appPTR->getIcon(NATRON_PIXMAP_COMBOBOX, &pixDropDown);
        _filterDropDown = new Button(QIcon(pixDropDown),"",_filterWidget);
        _filterDropDown->setFixedSize(buttonSize);
        _filterLayout->addWidget(_filterDropDown);
        QObject::connect(_filterDropDown,SIGNAL(clicked()),this,SLOT(showFilterMenu()));
        QObject::connect(_filterLineEdit,SIGNAL(textEdited(QString)),this,SLOT(applyFilter(QString)));
        
        
    }else{
        _fileExtensionCombo = new ComboBox(_filterWidget);
        for (U32 i = 0; i < _filters.size(); ++i) {
            _fileExtensionCombo->addItem(_filters[i].c_str());
        }
        _filterLineLayout->addWidget(_fileExtensionCombo);
        QObject::connect(_fileExtensionCombo, SIGNAL(currentIndexChanged(QString)), this, SLOT(setFileExtensionOnLineEdit(QString)));
        if(isSequenceDialog)
            _fileExtensionCombo->setCurrentIndex(_fileExtensionCombo->itemIndex("jpg"));
        _filterLineLayout->addStretch();
    }
    _proxy->setFilter(filter);

    
    _filterLineLayout->addWidget(_filterWidget);
    _cancelButton = new Button("Cancel",_filterLineWidget);
    _filterLineLayout->addWidget(_cancelButton);
    QObject::connect(_cancelButton, SIGNAL(clicked()), this, SLOT(cancelSlot()));
    
    _mainLayout->addWidget(_filterLineWidget);
    
    resize(900, 400);
    
    std::vector<QUrl> initialBookmarks;
#ifndef __NATRON_WIN32__
    initialBookmarks.push_back(QUrl::fromLocalFile(QLatin1String("/")));
#else
    
    initialBookmarks.push_back(QUrl::fromLocalFile(QLatin1String("C:")));
    
#endif
    initialBookmarks.push_back(QUrl::fromLocalFile(QDir::homePath()));
    _favoriteView->setModelAndUrls(_model, initialBookmarks);
    
    
    QItemSelectionModel *selectionModel = _view->selectionModel();
    QObject::connect(selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),this, SLOT(selectionChanged()));
    QObject::connect(_selectionLineEdit, SIGNAL(textChanged(QString)),this, SLOT(autoCompleteFileName(QString)));
    QObject::connect(_view, SIGNAL(customContextMenuRequested(QPoint)),
                     this, SLOT(showContextMenu(QPoint)));
    QObject::connect(_model, SIGNAL(rootPathChanged(QString)),
                     this, SLOT(pathChanged(QString)));
    createMenuActions();
    
    if(_dialogMode == OPEN_DIALOG){
        if(isSequenceDialog)
            setWindowTitle("Open Sequence");
        else
            setWindowTitle("Open File");
    }else{
        if(isSequenceDialog)
            setWindowTitle("Save Sequence");
        else
            setWindowTitle("Save File");
    }
    
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    restoreState(settings.value(QLatin1String("FileDialog")).toByteArray());
    
    if(!currentDirectory.empty())
        setDirectory(currentDirectory.c_str());
    
    
    if(!isSequenceDialog){
        enableSequenceMode(false);
    }
    
    
}
SequenceFileDialog::~SequenceFileDialog(){
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    settings.setValue(QLatin1String("FileDialog"), saveState());
      
    delete _model;
    delete _itemDelegate;
    delete _proxy;
    
    
    
}

QByteArray SequenceFileDialog::saveState() const{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    QList<QUrl> urls;
    std::vector<QUrl> stdUrls = _favoriteView->urls();
    for(unsigned int i = 0 ; i < stdUrls.size(); ++i) {
        urls.push_back(stdUrls[i]);
    }
    stream << _centerSplitter->saveState();
    stream << urls;
    stream << history();
    stream << currentDirectory().path();
    stream << _view->header()->saveState();
    return data;
}

bool SequenceFileDialog::restoreState(const QByteArray& state){
    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);
    if (stream.atEnd())
        return false;
    QByteArray splitterState;
    QByteArray headerData;
    QList<QUrl> bookmarks;
    QStringList history;
    QString currentDirectory;
    stream >> splitterState
    >> bookmarks
    >> history
    >> currentDirectory
    >> headerData;
    if (!_centerSplitter->restoreState(splitterState))
        return false;
    QList<int> list = _centerSplitter->sizes();
    if (list.count() >= 2 && list.at(0) == 0 && list.at(1) == 0) {
        for (int i = 0; i < list.count(); ++i)
            list[i] = _centerSplitter->widget(i)->sizeHint().width();
        _centerSplitter->setSizes(list);
    }
    std::vector<QUrl> stdBookMarks;
    for(int i = 0 ; i < bookmarks.count() ; ++i) {
        stdBookMarks.push_back(bookmarks.at(i));
    }
    _favoriteView->setUrls(stdBookMarks);
    while (history.count() > 5)
        history.pop_front();
    setHistory(history);
    setDirectory(currentDirectory);
    QHeaderView *headerView = _view->header();
    if (!headerView->restoreState(headerData))
        return false;
    
    QList<QAction*> actions = headerView->actions();
    QAbstractItemModel *abstractModel = _model;
    if (_proxy)
        abstractModel = _proxy;
    int total = qMin(abstractModel->columnCount(QModelIndex()), actions.count() + 1);
    for (int i = 1; i < total; ++i)
        actions.at(i - 1)->setChecked(!headerView->isSectionHidden(i));
    
    return true;
}



void SequenceFileDialog::setFileExtensionOnLineEdit(const QString& ext){
    QString str  = _selectionLineEdit->text();
    if(str.isEmpty()) return;
    if(isDirectory(str)){
        QString text = _selectionLineEdit->text() + "Untitled";
        if(sequenceModeEnabled()){
            text.append('#');
        }
        _selectionLineEdit->setText(text + "." + ext);
        return;
    }
    //from now on this is a filename or sequence name. It might not exist though
    
    QString pattern = getSequencePatternFromLineEdit();
    if(!pattern.isEmpty())
        str = pattern;
    
    int pos = str.lastIndexOf(QChar('.'));
    if(pos != -1){
        if(str.at(pos-1) == QChar('#')){
            --pos;
        }
        str = str.left(pos);
    }
    if (sequenceModeEnabled()) {
        //find out if there's already a # character
        QString unpathed = removePath(str);
        pos = unpathed.indexOf(QChar('#'));
        if(pos == -1){
            str.append("#");
        }
    }
    str.append(".");
    str.append(ext);
    _selectionLineEdit->setText(str);
}

void SequenceFileDialog::sequenceComboBoxSlot(int index){
    if(index == 1){
        enableSequenceMode(false);
    }else{
        enableSequenceMode(true);
    }
}

void SequenceFileDialog::showContextMenu(const QPoint& position){
    QMenu menu(_view);
    menu.addAction(_showHiddenAction);
    if (_createDirButton->isVisible()) {
        _newFolderAction->setEnabled(_createDirButton->isEnabled());
        menu.addAction(_newFolderAction);
    }
    menu.exec(_view->viewport()->mapToGlobal(position));
}

void SequenceFileDialog::createMenuActions(){
    QAction *goHomeAction =  new QAction(this);
    goHomeAction->setShortcut(Qt::CTRL + Qt::Key_H + Qt::SHIFT);
    QObject::connect(goHomeAction, SIGNAL(triggered()), this, SLOT(goHome()));
    addAction(goHomeAction);
    
    
    QAction *goToParent =  new QAction(this);
    goToParent->setShortcut(Qt::CTRL + Qt::UpArrow);
    QObject::connect(goToParent, SIGNAL(triggered()), this, SLOT(parentFolder()));
    addAction(goToParent);
    
    _showHiddenAction = new QAction(this);
    _showHiddenAction->setCheckable(true);
    _showHiddenAction->setText("Show hidden fildes");
    QObject::connect(_showHiddenAction, SIGNAL(triggered()), this, SLOT(showHidden()));
    
    _newFolderAction = new QAction(this);
    _newFolderAction->setText("New folder");
    QObject::connect(_newFolderAction, SIGNAL(triggered()), this, SLOT(createDir()));
}

void SequenceFileDialog::enableSequenceMode(bool b){
    _frameSequences.clear();
    _proxy->clear();
    if(!b){
        QWriteLocker locker(&_nameMappingMutex);
        _nameMapping.clear();
        _view->updateNameMapping(_nameMapping);
    }
    QString currentDir = _requestedDir;
    setDirectory(_model->myComputer().toString());
    setDirectory(currentDir);
    _view->viewport()->update();
}

void SequenceFileDialog::selectionChanged(){
    QStringList allFiles;
    QModelIndexList indexes = _view->selectionModel()->selectedRows();
    for (int i = 0; i < indexes.count(); ++i) {
        if (_model->isDir(mapToSource(indexes.at(i)))){
            _selectionLineEdit->setText(indexes.at(i).data(QFileSystemModel::FilePathRole).toString()+QDir::separator());
            continue;
        }
        allFiles.append(indexes.at(i).data(QFileSystemModel::FilePathRole).toString());
    }
    if (allFiles.count() > 1)
        for (int i = 0; i < allFiles.count(); ++i) {
            allFiles.replace(i, QString(QLatin1Char('"') + allFiles.at(i) + QLatin1Char('"')));
        }
    
    QString finalFiles = allFiles.join(QString(QLatin1Char(' ')));
    {
        QReadLocker locker(&_nameMappingMutex);
        NameMapping::const_iterator it = std::find_if(_nameMapping.begin(), _nameMapping.end(), NameMappingCompareFirst(finalFiles));
        if (it != _nameMapping.end()) {
            finalFiles =  it->second.second;
        }
    }
    if (!finalFiles.isEmpty())
        _selectionLineEdit->setText(finalFiles);
    if(_dialogMode == SAVE_DIALOG){
        setFileExtensionOnLineEdit(_fileExtensionCombo->itemText(_fileExtensionCombo->activeIndex()));
    }
    
}

void SequenceFileDialog::enterDirectory(const QModelIndex& index){
    
    QModelIndex sourceIndex = index.model() == _proxy ? mapToSource(index) : index;
    QString path = sourceIndex.data(QFileSystemModel::FilePathRole).toString();
    if (path.isEmpty() || _model->isDir(sourceIndex)) {
        setDirectory(path);
        
    }
}
void SequenceFileDialog::setDirectory(const QString &directory){
    if(directory.isEmpty()) return;
    QString newDirectory = directory;
    _view->selectionModel()->clear();
    
    //we remove .. and . from the given path if exist
    if (!directory.isEmpty())
        newDirectory = QDir::cleanPath(directory);
    
    if (!directory.isEmpty() && newDirectory.isEmpty())
        return;
    _requestedDir = newDirectory;
    QModelIndex root = _model->setRootPath(newDirectory); // < calls filterAcceptsRow
    _createDirButton->setEnabled(_dialogMode == SAVE_DIALOG);
    if(newDirectory.at(newDirectory.size()-1) != QChar('/')){
        newDirectory.append("/");
    }
    
    _selectionLineEdit->blockSignals(true);
    _selectionLineEdit->setText(newDirectory);
    _selectionLineEdit->blockSignals(false);


}

/*This function is called when a directory has successfully been loaded (i.e
 when the filesystem finished to load thoroughly the directory requested)*/
void SequenceFileDialog::updateView(const QString &directory){
    /*if the directory is the same as the last requested directory,just return*/
    if(directory != _requestedDir) return;
    
    QDir dir(_model->rootDirectory());
    /*update the to_parent button*/
    _upButton->setEnabled(dir.exists());
    
    QModelIndex root = _model->index(directory);
    QModelIndex proxyIndex = mapFromSource(root); // < calls filterAcceptsRow
    
    /*update the name mapping to display on the view according to the new
     datas that have been fetched and filtered by the file system.
     Not that it runs in another thread and updates the name mapping on the
     view when the computations are done.*/
    //QtConcurrent::run(this,&SequenceFileDialog::itemsToSequence,proxyIndex);
    itemsToSequence(proxyIndex);
    /*update the view to show the newly loaded directory*/
    setRootIndex(proxyIndex);
    _view->expandColumnsToFullWidth();
    /*clearing selection*/
    _view->selectionModel()->clear();
}

bool SequenceFileDialog::sequenceModeEnabled() const{
    return _sequenceButton->activeIndex() == 0;
}


bool SequenceDialogProxyModel::isAcceptedByUser(const QString &path) const{
    if(_filter.isEmpty()) return true;
    std::vector<QString> regExps;
    int i = 0;
    while (i < _filter.size()) {
        QString regExp;
        while( i < _filter.size() && _filter.at(i) != QChar(' ')){
            regExp.append(_filter.at(i));
            ++i;
        }
        ++i;
        regExps.push_back(regExp);
    }
    bool recognized = false;
    bool noRegExpValid = true;
    for (U32 j = 0; j < regExps.size(); ++j) {
        QRegExp rx(regExps[j],Qt::CaseInsensitive,QRegExp::Wildcard);
        if(!rx.isValid()){
            continue;
        }else{
            noRegExpValid = false;
            if(rx.exactMatch(path)){
                recognized = true;
                break;
            }
        }
        
    }
    if(!noRegExpValid && recognized){
        return true;
    }else{
        return false;
    }
}
void FileSequence::addToSequence(int frameIndex,const QString& path){
    QMutexLocker locker(&_lock);
    if(_frameIndexes.addToSequence(frameIndex)){
        QFile f(path);
        _totalSize += f.size();
        
    }
}

/*Complex filter to actually extract the sequences from the file system*/
bool SequenceDialogProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
    
    
    /*the item to filter*/
    QModelIndex item = sourceModel()->index(source_row , 0 , source_parent);
    
    /*the full path of the item*/
    QString path = item.data(QFileSystemModel::FilePathRole).toString();
    
    /*if it is a directory, we accept it*/
    if(qobject_cast<QFileSystemModel*>(sourceModel())->isDir(item)){
        return true;
    }
    
    /*if the item does not match the filter set by the user, discard it*/
    if(!isAcceptedByUser(item.data().toString())){
        return false;
    }
    
    /*if we reach here, this is a valid file and we need to take actions*/
    int frameNumber;
    QString pathCpy = path;
    QString extension;
    /*extracting the frame number if any and the file type.
     */
    parseFilename(pathCpy,&frameNumber,extension);
    
    /*If file sequence fetching is disabled, just call the base class version*/
    if(!_fd->sequenceModeEnabled() || frameNumber == -1){
        return QSortFilterProxyModel::filterAcceptsRow(source_row,source_parent);
    }
    
    /*Locking the access to the frame sequences multi-map*/
    QMutexLocker locker(&_frameSequencesMutex);
    std::pair<Natron::SequenceIterator,Natron::SequenceIterator> it = _frameSequences.equal_range(pathCpy.toStdString());
    if(it.first != it.second){
        /*we found a matching sequence name, we need to figure out if it has the same file type*/
        for(Natron::SequenceIterator it2 = it.first; it2!=it.second; ++it2) {
            if(it2->second->getFileType() == extension.toStdString()){
                it2->second->addToSequence(frameNumber,path);
                return frameNumber == it2->second->firstFrame();
            }
        }
        /*if it reaches here it means there's already a sequence with the same name but
         *with a different file type
         */
        boost::shared_ptr<FileSequence> _frames(new FileSequence(extension.toStdString()));
        _frames->addToSequence(frameNumber,path);
        _frameSequences.insert(make_pair(pathCpy.toStdString(),_frames));
        return true;
        
    }else{ /*couldn't find a sequence with the same name, we create one*/
        boost::shared_ptr<FileSequence>  _frames(new FileSequence(extension.toStdString()));
        _frames->addToSequence(frameNumber,path);
        _frameSequences.insert(make_pair(pathCpy.toStdString(),_frames));
        return true;
    }
    return true;
}

bool SequenceFileDialog::removeSequenceDigits(QString& file,int* frameNumber){
    int i = file.size() - 1;
    QString fnumberStr;
    while(i >= 0 && file.at(i).isDigit()) {
        fnumberStr.prepend(file.at(i));
        --i;
    }
    //if the whole file name is composed of digits or the file name doesn't contain any digit, return false.
    if(i == 0 || i == file.size()-1){
        return false;
    }
    file = file.left(i+1);
    *frameNumber = fnumberStr.toInt();
    return true;
}
void SequenceFileDialog::itemsToSequence(const QModelIndex& parent){
    if(!sequenceModeEnabled()){
        return;
    }
    /*Iterating over all the content of the parent directory.
     *Note that only 1 item is left per sequence already,
     *We just need to change its name to reflect the number
     *of elements in the sequence.
     */
    // for(SequenceIterator it = _frameSequences.begin() ; it!= _frameSequences.end(); ++it) {
    //     cout << it->first << " = " << it->second.second.size() << "x " << it->second.first << endl;
    // }
    for(int c = 0 ; c < _proxy->rowCount(parent) ; ++c) {
        QModelIndex item = _proxy->index(c,0,parent);
        /*We skip directories*/
        if(!item.isValid() || _model->isDir(_proxy->mapToSource(item))){
            continue;
        }
        QString name = item.data(QFileSystemModel::FilePathRole).toString();
        QString originalName = name;
        QString extension;
        int frameNumber;
        SequenceDialogProxyModel::parseFilename(name, &frameNumber, extension);
        if(frameNumber == -1){
            continue;
        }
        boost::shared_ptr<FileSequence> frameRanges = frameRangesForSequence(name.toStdString(),extension.toStdString());

        if(!frameRanges){
            continue;
        }

        /*we don't display sequences with no frame contiguous like 10-13-15.
         *This is corner case and is not useful anyway, we rather display it as several files
         */
        if(!frameRanges->isEmpty()){
            std::vector< std::pair<int,int> > chunks;
            int first = frameRanges->firstFrame();
            while(first <= frameRanges->lastFrame()){
                
                while (!(frameRanges->isInSequence(first))) {
                    ++first;
                }
                
                chunks.push_back(std::make_pair(first, frameRanges->lastFrame()));
                int next = first + 1;
                int prev = first;
                int count = 1;
                while((next <= frameRanges->lastFrame())
                      && frameRanges->isInSequence(next)
                      && (next == prev + 1) ){
                    prev = next;
                    ++next;
                    ++count;
                }
                --next;
                chunks.back().second = next;
                first += count;
            }

            name.append("#.");
            name.append(extension);
            
            if(chunks.size() == 1){
                name.append(QString(" %1-%2").arg(QString::number(chunks[0].first)).arg(QString::number(chunks[0].second)));
            }else{
                name.append(" ( ");
                for(unsigned int i = 0 ; i < chunks.size() ; ++i) {
                    if(chunks[i].first != chunks[i].second){
                        name.append(QString(" %1-%2").arg(QString::number(chunks[i].first)).arg(QString::number(chunks[i].second)));
                    }else{
                        name.append(QString(" %1").arg(QString::number(chunks[i].first)));
                    }
                    if(i < chunks.size() -1) name.append(" /");
                }
                name.append(" ) ");
            }
            {
                QWriteLocker locker(&_nameMappingMutex);
                NameMapping::const_iterator it = std::find_if(_nameMapping.begin(), _nameMapping.end(), NameMappingCompareFirst(originalName));
                if (it == _nameMapping.end()) {
                    //        cout << "mapping: " << originalName.toStdString() << " TO " << name.toStdString() << endl;
                    _nameMapping.push_back(make_pair(originalName,make_pair(frameRanges->getTotalSize(),name)));
                }
            }
        }
    }
    
    {
        QReadLocker locker(&_nameMappingMutex);
        _view->updateNameMapping(_nameMapping);
    }
}
void SequenceFileDialog::setRootIndex(const QModelIndex& index){
    _view->setRootIndex(index);
}


void SequenceFileDialog::setFrameSequence(const Natron::FrameSequences &frameSequences){
    /*Removing from the sequence any element with a sequence of 1 element*/
    _frameSequences = frameSequences;
}
boost::shared_ptr<FileSequence>  SequenceFileDialog::frameRangesForSequence(const std::string& sequenceName, const std::string& extension) const{
    std::pair<Natron::ConstSequenceIterator,Natron::ConstSequenceIterator> found =  _frameSequences.equal_range(sequenceName);
    for(Natron::ConstSequenceIterator it = found.first ;it!=found.second;++it) {
        if(it->second->getFileType() == extension){
            return it->second;
            break;
        }
    }
    return boost::shared_ptr<FileSequence>();
}


void SequenceDialogProxyModel::parseFilename(QString& path,int* frameNumber,QString &extension){
    /*removing the file extension if any*/
    extension = Natron::removeFileExtension(path);
    /*removing the frame number if any*/
    if (!SequenceFileDialog::removeSequenceDigits(path, frameNumber)) {
        *frameNumber = -1;
    }
}

SequenceDialogView::SequenceDialogView(SequenceFileDialog* fd):QTreeView(fd),_fd(fd){
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setRootIsDecorated(false);
    setItemsExpandable(false);
    setSortingEnabled(true);
    header()->setSortIndicator(0, Qt::AscendingOrder);
    header()->setStretchLastSection(false);
    setTextElideMode(Qt::ElideMiddle);
    setEditTriggers(QAbstractItemView::EditKeyPressed);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setDragDropMode(QAbstractItemView::InternalMove);
    //setAttribute(Qt::WA_MacShowFocusRect,0);
    setAcceptDrops(true);
}

void SequenceDialogView::dropEvent(QDropEvent* event){
    if(!event->mimeData()->hasUrls())
        return;

    QStringList filesList;
    QList<QUrl> urls = event->mimeData()->urls();
    QString path;
    if(urls.size() > 0){
        path = urls.at(0).path();
    }
    if(!path.isEmpty()){
        _fd->setDirectory(path);
    }
}

void SequenceDialogView::dragEnterEvent(QDragEnterEvent *ev){
    ev->accept();
}

void SequenceDialogView::dragMoveEvent(QDragMoveEvent* e){
    e->accept();
}

void SequenceDialogView::dragLeaveEvent(QDragLeaveEvent* e){
    e->accept();
}

void SequenceDialogView::updateNameMapping(const std::vector<std::pair<QString, std::pair<qint64, QString> > >& nameMapping){
    dynamic_cast<SequenceItemDelegate*>(itemDelegate())->setNameMapping(nameMapping);
}


SequenceItemDelegate::SequenceItemDelegate(SequenceFileDialog* fd) : QStyledItemDelegate(),_maxW(200),_fd(fd){}

void SequenceItemDelegate::setNameMapping(const std::vector<std::pair<QString, std::pair<qint64, QString> > >& nameMapping) {
    _maxW = 200;
    QFont f(NATRON_FONT_ALT, NATRON_FONT_SIZE_6);
    QFontMetrics metric(f);
    {
        QWriteLocker locker(&_nameMappingMutex);
        _nameMapping.clear();
        for(unsigned int i = 0 ; i < nameMapping.size() ; ++i) {
            const SequenceFileDialog::NameMappingElement& p = nameMapping[i];
            _nameMapping.push_back(p);
            int w = metric.width(p.second.second);
            if(w > _maxW) _maxW = w;
        }
    }
}



void SequenceItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem &option, const QModelIndex & index) const {
    if(index.column() != 0 && index.column() != 1) {
        return QStyledItemDelegate::paint(painter,option,index);
    }
    QString str;
    if (index.column() == 0) {
        str = index.data().toString();
    } else if (index.column() == 1) {
        QFileSystemModel* model = _fd->getFileSystemModel();
        QModelIndex modelIndex = _fd->mapToSource(index);
        QModelIndex idx = model->index(modelIndex.row(),0,modelIndex.parent());
        str = idx.data().toString();
    }
    std::pair<qint64,QString> found_item;
    {
        QReadLocker locker(&_nameMappingMutex);
        SequenceFileDialog::NameMapping::const_iterator it = std::find_if(_nameMapping.begin(), _nameMapping.end(), NameMappingCompareFirstNoPath(str));
        if (it == _nameMapping.end()) { // probably a directory or a single image file
            return QStyledItemDelegate::paint(painter,option,index);
        }
        found_item = it->second;
    }
    // get the proper subrect from the style
    QStyle *style = QApplication::style();
    QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &option);
    // QRect geom = option.rect;

    // FIXME: with the default delegate (QStyledItemDelegate), there is a margin
    // of a few more pixels between border and icon, and between icon and text, not with this one
    if (index.column() == 0) {
        QRect r;
        if (option.state & QStyle::State_Selected){
            painter->fillRect(geom, option.palette.highlight());
        }
        QString nameToPaint = SequenceFileDialog::removePath(found_item.second);
        int totalSize = geom.width();
        int iconWidth = option.decorationSize.width();
        int textSize = totalSize - iconWidth;

        QFileInfo fileInfo(index.data(QFileSystemModel::FilePathRole).toString());
        QIcon icon = _fd->getFileSystemModel()->iconProvider()->icon(fileInfo);
        QRect iconRect(geom.x(),geom.y(),iconWidth,geom.height());
        QSize iconSize = icon.actualSize(QSize(iconRect.width(),iconRect.height()));

        QRect textRect(geom.x()+iconWidth,geom.y(),textSize,geom.height());

        painter->drawPixmap(iconRect,
                            icon.pixmap(iconSize),
                            r);
        painter->drawText(textRect,Qt::TextSingleLine,nameToPaint,&r);
    } else if (index.column() == 1) {
        QRect r;
        if (option.state & QStyle::State_Selected){
            painter->fillRect(geom, option.palette.highlight());
        }
        QString nameToPaint(printAsRAM(found_item.first));
        painter->drawText(geom,Qt::TextSingleLine|Qt::AlignRight,nameToPaint,&r);
    }


}

void FavoriteItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const{
    if(index.column() == 0){
        QString str = index.data().toString();
        QFileInfo fileInfo(str);
        str = _model->index(str).data().toString();
        QIcon icon = _model->iconProvider()->icon(fileInfo);
        int totalSize = option.rect.width();
        int iconSize = option.decorationSize.width();
        int textSize = totalSize - iconSize;

        QRect iconRect(option.rect.x(),option.rect.y(),iconSize,option.rect.height());
        QRect textRect(option.rect.x()+iconSize,option.rect.y(),textSize,option.rect.height());
        QRect r;
        if (option.state & QStyle::State_Selected){
            painter->fillRect(option.rect, option.palette.highlight());
        }
        painter->drawPixmap(iconRect,
                            icon.pixmap(icon.actualSize(QSize(iconRect.width(),iconRect.height()))),
                            r);
        painter->drawText(textRect,Qt::TextSingleLine,str,&r);

    }else{
        QStyledItemDelegate::paint(painter,option,index);
    }
}
bool SequenceFileDialog::isDirectory(const QString& name) const{
    QModelIndex index = _model->index(name);
    return index.isValid() && _model->isDir(index);
}
QSize SequenceItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const {
    if(index.column() != 0){
        return QStyledItemDelegate::sizeHint(option,index);
    }
    QString str =  index.data().toString();
    QFontMetrics metric(option.font);
    return QSize(metric.width(str),metric.height());
}
bool SequenceFileDialog::isASupportedFileExtension(const std::string& ext) const{
    if(!_filters.size())
        return true;
    for(unsigned int i = 0 ; i < _filters.size() ; ++i) {
        if(nocase_equal_string(ext, _filters[i]))
            return true;
    }
    return false;
}

QString SequenceFileDialog::removePath(const QString& str){
    int i = str.lastIndexOf(".");
    if(i!=-1){
        //if the file has an extension
        --i;
        while(i >=0 && str.at(i) != QChar('/') && str.at(i) != QChar('\\')){
            --i;
        }
        ++i;
        std::string stdStr = str.toStdString();
        if(str.isEmpty()){
            return "";
        }
        return stdStr.substr(i).c_str();
    }else{
        i = str.lastIndexOf(QChar('/'));
        if(i == -1){
            //try out \\ char
            i = str.lastIndexOf(QChar('\\'));
        }
        if(i == -1){
            return str;
        }else{
            ++i;
            std::string stdStr = str.toStdString();
            return stdStr.substr(i).c_str();
        }
    }
}

bool SequenceFileDialog::checkIfContiguous(const std::vector<int>& v){
    for(unsigned int i = 0 ; i < v.size() ;++i) {
        if(i < v.size()-1 &&  v[i]+1 == v[i+1])
            return true;
    }
    return false;
}

void SequenceFileDialog::previousFolder(){
    if(!_history.isEmpty() && _currentHistoryLocation > 0){
        --_currentHistoryLocation;
        QString previousHistory = _history.at(_currentHistoryLocation);
        setDirectory(previousHistory);
    }

}
void SequenceFileDialog::nextFolder(){
    if(!_history.isEmpty() && _currentHistoryLocation < _history.size()-1){
        ++_currentHistoryLocation;
        QString nextHistory = _history.at(_currentHistoryLocation);
        setDirectory(nextHistory);
    }
}
void SequenceFileDialog::parentFolder(){
    QDir dir(_model->rootDirectory());
    QString newDir;
    if(dir.isRoot()){
        newDir = _model->myComputer().toString();
    }else{
        dir.cdUp();
        newDir = dir.absolutePath();
    }
    setDirectory(newDir);
}

void SequenceFileDialog::showHidden(){
    QDir::Filters dirFilters = _model->filter();
    if (_showHiddenAction->isChecked())
        dirFilters |= QDir::Hidden;
    else
        dirFilters &= ~QDir::Hidden;
    _model->setFilter(dirFilters);
    // options->setFilter(dirFilters);
    _showHiddenAction->setChecked((dirFilters & QDir::Hidden));
}

void SequenceFileDialog::goHome(){
    setDirectory(QDir::homePath());
}

void SequenceFileDialog::createDir(){
    _favoriteView->clearSelection();
    QString newFolderString;
    QInputDialog dialog(this);
    dialog.setLabelText("Folder name:");
    dialog.setWindowTitle("New folder");
    if(dialog.exec()){
        newFolderString = dialog.textValue();
        if(!newFolderString.isEmpty()){
            QString folderName = newFolderString;
            QString prefix  = currentDirectory().absolutePath()+QDir::separator();
            if (QFile::exists(prefix + folderName)) {
                qlonglong suffix = 2;
                while (QFile::exists(prefix + folderName)) {
                    folderName = newFolderString + QString::number(suffix);
                    ++suffix;
                }

            }
            QModelIndex parent = mapToSource(_view->rootIndex());
            QModelIndex index = _model->mkdir(parent, folderName);
            if (!index.isValid())
                return;
            enterDirectory(index);
        }
    }
}
AddFavoriteDialog::AddFavoriteDialog(SequenceFileDialog* fd,QWidget* parent):QDialog(parent),_fd(fd){
    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setSpacing(0);
    _mainLayout->setContentsMargins(5, 5, 0, 0);
    setLayout(_mainLayout);
    setWindowTitle("New favorite");

    _descriptionLabel = new QLabel("",this);
    _mainLayout->addWidget(_descriptionLabel);

    _secondLine = new QWidget(this);
    _secondLineLayout = new QHBoxLayout(_secondLine);

    _pathLineEdit = new LineEdit(_secondLine);
    _pathLineEdit->setPlaceholderText("path...");
    _secondLineLayout->addWidget(_pathLineEdit);

    
    QPixmap pix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE,&pix);

    _openDirButton = new Button(_secondLine);
    _openDirButton->setIcon(QIcon(pix));
    _openDirButton->setFixedSize(20, 20);
    QObject::connect(_openDirButton, SIGNAL(clicked()), this, SLOT(openDir()));
    _secondLineLayout->addWidget(_openDirButton);

    _mainLayout->addWidget(_secondLine);

    _thirdLine = new QWidget(this);
    _thirdLineLayout = new QHBoxLayout(_thirdLine);
    _thirdLine->setLayout(_thirdLineLayout);

    _cancelButton = new Button("Cancel",_thirdLine);
    QObject::connect(_cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    _thirdLineLayout->addWidget(_cancelButton);

    _okButton = new Button("Ok",_thirdLine);
    QObject::connect(_okButton, SIGNAL(clicked()), this, SLOT(accept()));
    _thirdLineLayout->addWidget(_okButton);

    _mainLayout->addWidget(_thirdLine);

}

void AddFavoriteDialog::setLabelText(const QString& text){
    _descriptionLabel->setText(text);
}

void AddFavoriteDialog::openDir(){
    QString str = QFileDialog::getExistingDirectory(this,"Select a directory",_fd->currentDirectory().path());
    _pathLineEdit->setText(str);
}

QString AddFavoriteDialog::textValue() const{
    return _pathLineEdit->text();
}
void SequenceFileDialog::addFavorite(){
    AddFavoriteDialog dialog(this,this);
    dialog.setLabelText("Folder path:");
    QString newFavName,newFavPath;
    if(dialog.exec()){
        newFavName = dialog.textValue();
        newFavPath = dialog.textValue();
        addFavorite(newFavName,newFavPath);
    }

}

void SequenceFileDialog::addFavorite(const QString& name,const QString& path){
    QDir d(path);
    if(!d.exists()){
        return;
    }
    if(!name.isEmpty() && !path.isEmpty()){
        std::vector<QUrl> list;
        list.push_back(QUrl::fromLocalFile(path));
        _favoriteView->addUrls(list,-1);
    }
}

void SequenceFileDialog::openSelectedFiles(){
    QString str = _selectionLineEdit->text();
    if(!isDirectory(str)){
        if(_dialogMode == OPEN_DIALOG){
            QStringList files = selectedFiles();
            if(!files.isEmpty()){
                QDialog::accept();
            }
        }else{
            QString pattern = getSequencePatternFromLineEdit();
            if(!pattern.isEmpty())
                str = pattern;
            QString unpathed = removePath(str);
            int pos;
            if(sequenceModeEnabled()){
                pos = unpathed.indexOf("#");
                if(pos == -1){
                    pos = unpathed.lastIndexOf(".");
                    if (pos == -1) {
                        str.append("#");
                    }else{
                        str.insert(pos, "#");
                    }
                }else{
                    int dotPos = unpathed.lastIndexOf(".");
                    if(dotPos != pos+1){
                        //natron only supports padding character # before the . marking the file extension
                        QMessageBox::critical(this, "Filename error", QCoreApplication::applicationName() + " only supports padding character"
                                              " (#) before the '.' marking the file extension.");
                        return;
                    }
                }

            }
            pos = unpathed.lastIndexOf(".");
            if(pos == -1){
                str.append(".");
                str.append(_fileExtensionCombo->itemText(_fileExtensionCombo->activeIndex()));
            }
            _selectionLineEdit->setText(str);
            QStringList files = filesListFromPattern(str);
            if(files.size() > 0){
                QString text;
                if(files.size() == 1){
                    text = "The file ";
                    text.append(files.at(0));
                    text.append(" already exists.\n Would you like to replace it ?");
                }else{
                    text = "The sequence ";
                    text.append(removePath(str));
                    text.append(" already exists.\n Would you like to replace it ?");
                }
                QMessageBox::StandardButton ret = QMessageBox::question(this, "Existing file", text,
                                                                        QMessageBox::Yes | QMessageBox::No);
                if(ret != QMessageBox::Yes){
                    return;
                }

            }
            QDialog::accept();
        }
    }else{
        setDirectory(str);
    }
}
void SequenceFileDialog::cancelSlot(){
    QDialog::reject();
}
void SequenceFileDialog::keyPressEvent(QKeyEvent *e){
    if(e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter){
        QString str = _selectionLineEdit->text();
        if(!isDirectory(str)){
            QStringList files = selectedFiles();
            if(!files.isEmpty()){
                QDialog::accept();
            }
        }else{
            setDirectory(str);
        }
        return;
    }
    QDialog::keyPressEvent(e);
}
void SequenceFileDialog::resizeEvent(QResizeEvent* e){
    QDialog::resizeEvent(e);
    _view->expandColumnsToFullWidth();
    _view->repaint();
}

#ifdef Q_OS_UNIX
static QString qt_tildeExpansion(const QString &path, bool *expanded = 0)
{
    if (expanded != 0)
        *expanded = false;
    if (!path.startsWith(QLatin1Char('~')))
        return path;
    QString ret = path;
    QStringList tokens = ret.split(QDir::separator());
    if (tokens.first() == QLatin1String("~")) {
        ret.replace(0, 1, QDir::homePath());
    } /*else {
       QString userName = tokens.first();
       userName.remove(0, 1);

       const QString homePath = QString::fro#if defined(Q_OS_VXWORKS)
       const QString homePath = QDir::homePath();
       #elif defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(Q_OS_OPENBSD)
       passwd pw;
       passwd *tmpPw;
       char buf[200];
       const int bufSize = sizeof(buf);
       int err = 0;
       #if defined(Q_OS_SOLARIS) && (_POSIX_C_SOURCE - 0 < 199506L)
       tmpPw = getpwnam_r(userName.toLocal8Bit().constData(), &pw, buf, bufSize);
       #else
       err = getpwnam_r(userName.toLocal8Bit().constData(), &pw, buf, bufSize, &tmpPw);
       #endif
       if (err || !tmpPw)
       return ret;mLocal8Bit(pw.pw_dir);
       #else
       passwd *pw = getpwnam(userName.toLocal8Bit().constData());
       if (!pw)
       return ret;
       const QString homePath = QString::fromLocal8Bit(pw->pw_dir);
       #endif
       ret.replace(0, tokens.first().length(), homePath);
       }*/
    if (expanded != 0)
        *expanded = true;
    return ret;
}
#endif

QStringList SequenceFileDialog::typedFiles() const{
    QStringList files;
    QString editText = _selectionLineEdit->text();
    if (!editText.contains(QLatin1Char('"'))) {
#ifdef Q_OS_UNIX
        const QString prefix = currentDirectory().absolutePath() + QDir::separator();
        if (QFile::exists(prefix + editText))
            files << editText;
        else
            files << qt_tildeExpansion(editText);
#else
        files << editText;
#endif
    } else {
        // " is used to separate files like so: "file1" "file2" "file3" ...
        // ### need escape character for filenames with quotes (")
        QStringList tokens = editText.split(QLatin1Char('\"'));
        for (int i=0; i<tokens.size(); ++i) {
            if ((i % 2) == 0)
                continue; // Every even token is a separator
#ifdef Q_OS_UNIX
            const QString token = tokens.at(i);
            const QString prefix = currentDirectory().absolutePath() + QDir::separator();
            if (QFile::exists(prefix + token))
                files << token;
            else
                files << qt_tildeExpansion(token);
#else
            files << toInternal(tokens.at(i));
#endif
        }
    }
    return files;
}
void SequenceFileDialog::autoCompleteFileName(const QString& text){
    if (text.startsWith(QLatin1String("//")) || text.startsWith(QLatin1Char('\\'))) {
        _view->selectionModel()->clearSelection();
        return;
    }
    QStringList multipleFiles = typedFiles();
    if (multipleFiles.count() > 0) {
        QModelIndexList oldFiles = _view->selectionModel()->selectedRows();
        QModelIndexList newFiles;
        for (int i = 0; i < multipleFiles.count(); ++i) {
            QModelIndex idx = _model->index(multipleFiles.at(i));
            if (oldFiles.contains(idx))
                oldFiles.removeAll(idx);
            else
                newFiles.append(idx);
        }
        for (int i = 0; i < newFiles.count(); ++i)
            select(newFiles.at(i));
        if (_selectionLineEdit->hasFocus())
            for (int i = 0; i < oldFiles.count(); ++i)
                _view->selectionModel()->select(oldFiles.at(i),
                                                QItemSelectionModel::Toggle | QItemSelectionModel::Rows);
    }
}
void SequenceFileDialog::goToDirectory(const QString& path){
    QModelIndex index = _lookInCombobox->model()->index(_lookInCombobox->currentIndex(),
                                                        _lookInCombobox->modelColumn(),
                                                        _lookInCombobox->rootModelIndex());
    QString path2 = path;
    if (!index.isValid())
        index = mapFromSource(_model->index(getEnvironmentVariable(path)));
    else {
        path2 = index.data(QFileSystemModel::FilePathRole).toUrl().toLocalFile();
        index = mapFromSource(_model->index(path2));
    }
    QDir dir(path2);
    if (!dir.exists())
        dir = getEnvironmentVariable(path2);

    if (dir.exists() || path2.isEmpty() || path2 == _model->myComputer().toString()) {
        enterDirectory(index);
    }
}
QString SequenceFileDialog::getEnvironmentVariable(const QString &string)
{
#ifdef Q_OS_UNIX
    if (string.size() > 1 && string.startsWith(QLatin1Char('$'))) {
        return QString::fromLocal8Bit(getenv(string.mid(1).toLatin1().constData()));
    }
#else
    if (string.size() > 2 && string.startsWith(QLatin1Char('%')) && string.endsWith(QLatin1Char('%'))) {
        return QString::fromLocal8Bit(qgetenv(string.mid(1, string.size() - 2).toLatin1().constData()));
    }
#endif
    return string;
}


void SequenceFileDialog::pathChanged(const QString &newPath)
{
    QDir dir(_model->rootDirectory());
    _upButton->setEnabled(dir.exists());
    _favoriteView->selectUrl(QUrl::fromLocalFile(newPath));
    setHistory(_lookInCombobox->history());

    if (_currentHistoryLocation < 0 || _history.value(_currentHistoryLocation) != QDir::toNativeSeparators(newPath)) {
        while (_currentHistoryLocation >= 0 && _currentHistoryLocation + 1 < _history.count()) {
            _history.removeLast();
        }
        _history.append(QDir::toNativeSeparators(newPath));
        ++_currentHistoryLocation;
    }
    _nextButton->setEnabled(_history.size() - _currentHistoryLocation > 1);
    _previousButton->setEnabled(_currentHistoryLocation > 0);
}

void SequenceFileDialog::setHistory(const QStringList &paths){
    _lookInCombobox->setHistory(paths);
}

QStringList SequenceFileDialog::history() const{
    QStringList currentHistory = _lookInCombobox->history();
    //On windows the popup display the "C:\", convert to nativeSeparators
    QString newHistory = QDir::toNativeSeparators(_view->rootIndex().data(QFileSystemModel::FilePathRole).toString());
    if (!currentHistory.contains(newHistory))
        currentHistory << newHistory;
    return currentHistory;
}

QStringList SequenceFileDialog::selectedFiles(){
    QStringList out;
    QModelIndexList indexes = _view->selectionModel()->selectedRows();
    if(indexes.count() > 0){
        QDir dir = currentDirectory();
        QString prefix = dir.absolutePath()+"/";
        QModelIndex sequenceIndex = indexes.at(0);
        QString path = sequenceIndex.data().toString();
        path = prefix+path;
        QString originalPath = path;
        QString ext = Natron::removeFileExtension(path);
        int i  = path.size() -1;
        while(i >= 0 && path.at(i).isDigit()) {
            --i;
        }
        ++i;
        path = path.left(i);
        // FileSequence *sequence = 0;
        std::pair<Natron::SequenceIterator,Natron::SequenceIterator> range = _frameSequences.equal_range(path.toStdString());

        /*if this is not a registered sequence. i.e: this is a single image file*/
        if(range.first == range.second){
            //  cout << originalPath.toStdString() << endl;
            out.append(originalPath);
        }else{
            //            for(SequenceIterator it = range.first ; it!=range.second; ++it) {
            //                if(it->second._fileType == ext.toStdString()){
            //                    sequence = &(it->second);
            //                    break;
            //                }
            //            }
            /*now that we now how many frames there are,their extension and the common name, we retrieve them*/
            i = 0;
            QStringList dirEntries = dir.entryList();
            for(int j = 0 ; j < dirEntries.size(); ++j) {
                QString s = dirEntries.at(j);
                s = prefix+s;

                if(QFile::exists(s) && s.contains(path) && s.contains(ext)){
                    out.append(s);
                    ++i;
                }
                //if(out.size() > (int)(sequence->_frameIndexes.size())) break; //number of files exceeding the sequence size...something is wrong
            }

        }
    }else{
        //if nothing is selected, pick whatever the line edit tells us
        QString lineEditTxt = _selectionLineEdit->text();
        out.append(lineEditTxt);
    }
    return out;
}
QString SequenceFileDialog::getSequencePatternFromLineEdit(){
    QStringList selected = selectedFiles();
    /*get the extension of the first selected file*/
    if(selected.size() > 0){
        QString firstInSequence = selected.at(0);
        QString ext = Natron::removeFileExtension(firstInSequence);
        ext.prepend(".");

        QString lineEditText = _selectionLineEdit->text();
        int extPos = lineEditText.lastIndexOf(ext);
        if(extPos!=-1){
            int rightMostChar = extPos+ext.size();
            return lineEditText.left(rightMostChar);
        }else{
            return "";
        }

    }else{
        return "";
    }
}
QStringList SequenceFileDialog::filesListFromPattern(const QString& pattern){
    QStringList ret;
    QString unpathed = removePath(pattern);
    int indexOfCommonPart = pattern.indexOf(unpathed);
    QString path = pattern.left(indexOfCommonPart);
    assert(pattern == (path + unpathed));

    QString commonPart;
    int i = 0;
    while (i < unpathed.size() && unpathed.at(i) != QChar('#') && unpathed.at(i) != QChar('.')) {
        commonPart.append(unpathed.at(i));
        ++i;
    }
    if(i == unpathed.size()) return ret;


    QDir d(path);
    if(d.isReadable()) {
        QStringList files = d.entryList();
        for (int j = 0; j < files.size() ; ++j) {
            if (files.at(j).contains(commonPart)) {
                ret << QString(path + files.at(j));
            }
        }
    }
    return ret;

}
QString SequenceFileDialog::patternFromFilesList(const QStringList& files){
    if(files.size() == 0)
        return "";
    QString firstFile = files.at(0);
    int pos = firstFile.lastIndexOf(QChar('.'));
    int extensionPos = pos+1;
    --pos;
    while (pos >=0 && firstFile.at(pos).isDigit()) {
        --pos;
    }
    ++pos;
    QString commonPart = firstFile.left(pos);
    QString ext;
    while(extensionPos < firstFile.size()){
        ext.append(firstFile.at(extensionPos));
        ++extensionPos;
    }
    if(files.size() > 1)
        return QString(commonPart + "#." +  ext);
    else
        return QString(commonPart + "." + ext);
}
QString SequenceFileDialog::filesToSave(){
    QString pattern = getSequencePatternFromLineEdit();
    if(filesListFromPattern(pattern).size() > 0){
        return pattern;
    }
    return _selectionLineEdit->text();
}

QDir SequenceFileDialog::currentDirectory() const{
    return _requestedDir;
}
QModelIndex SequenceFileDialog::select(const QModelIndex& index){
    QModelIndex ret = mapFromSource(index);
    if(ret.isValid() && !_view->selectionModel()->isSelected(ret)){
        _view->selectionModel()->select(ret, QItemSelectionModel::Select |QItemSelectionModel::Rows);
    }
    return ret;
}

void SequenceFileDialog::doubleClickOpen(const QModelIndex& index){
    QModelIndexList indexes = _view->selectionModel()->selectedRows();
    for (int i = 0; i < indexes.count(); ++i) {
        if (_model->isDir(mapToSource(indexes.at(i)))){
            _selectionLineEdit->setText(indexes.at(i).data(QFileSystemModel::FilePathRole).toString()+QDir::separator());
            break;
        }
    }

    openSelectedFiles();
    enterDirectory(index);
}
void SequenceFileDialog::seekUrl(const QUrl& url){
    setDirectory(url.toLocalFile());
}

void SequenceFileDialog::showFilterMenu(){
    QPoint position(_filterLineEdit->mapToGlobal(_filterLineEdit->pos()));
    position.ry() += _filterLineEdit->height();
    QList<QAction *> actions;


    QAction *defaultFilters = new QAction(generateStringFromFilters(),this);
    QObject::connect(defaultFilters, SIGNAL(triggered()), this, SLOT(defaultFiltersSlot()));
    actions.append(defaultFilters);

    QAction *startSlash = new QAction("*/", this);
    QObject::connect(startSlash, SIGNAL(triggered()), this, SLOT(starSlashFilterSlot()));
    actions.append(startSlash);

    QAction *empty = new QAction("*", this);
    QObject::connect(empty, SIGNAL(triggered()), this, SLOT(emptyFilterSlot()));
    actions.append(empty);

    QAction *dotStar = new QAction(".*", this);
    QObject::connect(dotStar, SIGNAL(triggered()), this, SLOT(dotStarFilterSlot()));
    actions.append(dotStar);


    if (actions.count() > 0){
        QMenu menu(_filterLineEdit);
        menu.addActions(actions);
        menu.setFixedSize(_filterLineEdit->width(),menu.sizeHint().height());
        menu.exec(position);
    }
}
QString SequenceFileDialog::generateStringFromFilters(){
    QString ret;
    for (U32 i = 0 ; i < _filters.size(); ++i) {
        ret.append("*");
        ret.append(".");
        ret.append(_filters[i].c_str());
        if(i < _filters.size() -1)
            ret.append(" ");
    }
    return ret;
}
void SequenceFileDialog::defaultFiltersSlot(){
    QString filter = generateStringFromFilters();
    _filterLineEdit->setText(filter);
    applyFilter(filter);
}
void SequenceFileDialog::dotStarFilterSlot(){
    QString filter(".*");
    _filterLineEdit->setText(filter) ;
    applyFilter(filter);
}

void SequenceFileDialog::starSlashFilterSlot(){
    QString filter("*/");
    _filterLineEdit->setText(filter) ;
    applyFilter(filter);
}

void SequenceFileDialog::emptyFilterSlot(){
    QString filter("*");
    _filterLineEdit->setText(filter) ;
    applyFilter(filter);
}

void SequenceFileDialog::applyFilter(QString filter){
    _proxy->setFilter(filter);
    QString currentDir = _requestedDir;
    setDirectory(_model->myComputer().toString());
    setDirectory(currentDir);
}

UrlModel::UrlModel(QObject *parent) : QStandardItemModel(parent), fileSystemModel(0)
{

}


bool UrlModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (value.type() == QVariant::Url) {
        QUrl url = value.toUrl();
        QModelIndex dirIndex = fileSystemModel->index(url.toLocalFile());

        QStandardItemModel::setData(index, QDir::toNativeSeparators(fileSystemModel->data(dirIndex, QFileSystemModel::FilePathRole).toString()), Qt::ToolTipRole);
        //  QStandardItemModel::setData(index, fileSystemModel->data(dirIndex).toString());
        QStandardItemModel::setData(index, fileSystemModel->data(dirIndex, Qt::DecorationRole),Qt::DecorationRole);
        QStandardItemModel::setData(index, url, UrlRole);
        return true;
    }
    return QStandardItemModel::setData(index, value, role);
}

void UrlModel::setUrl(const QModelIndex &index, const QUrl &url, const QModelIndex &dirIndex)
{
    setData(index, url, UrlRole);
    if (url.path().isEmpty()) {
        setData(index, fileSystemModel->myComputer());
        setData(index, fileSystemModel->myComputer(Qt::DecorationRole), Qt::DecorationRole);
    } else {
        QString newName;
        newName = QDir::toNativeSeparators(dirIndex.data(QFileSystemModel::FilePathRole).toString());//dirIndex.data().toString();
        QIcon newIcon = qvariant_cast<QIcon>(dirIndex.data(Qt::DecorationRole));
        if (!dirIndex.isValid()) {
            newIcon = fileSystemModel->iconProvider()->icon(QFileIconProvider::Folder);
            newName = QFileInfo(url.toLocalFile()).fileName();
            bool invalidUrlFound = false;
            for(unsigned int i = 0 ; i < invalidUrls.size(); ++i) {
                if(invalidUrls[i]==url){
                    invalidUrlFound = true;
                    break;
                }
            }
            if (!invalidUrlFound)
                invalidUrls.push_back(url);
            //The bookmark is invalid then we set to false the EnabledRole
            setData(index, false, EnabledRole);
        } else {
            //The bookmark is valid then we set to true the EnabledRole
            setData(index, true, EnabledRole);
        }

        // Make sure that we have at least 32x32 images
        const QSize size = newIcon.actualSize(QSize(32,32));
        if (size.width() < 32) {
            QPixmap smallPixmap = newIcon.pixmap(QSize(32, 32));
            newIcon.addPixmap(smallPixmap.scaledToWidth(32, Qt::SmoothTransformation));
        }

        if (index.data().toString() != newName)
            setData(index, newName);
        QIcon oldIcon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        if (oldIcon.cacheKey() != newIcon.cacheKey())
            setData(index, newIcon, Qt::DecorationRole);
    }
}

void UrlModel::setUrls(const std::vector<QUrl> &urls)
{
    removeRows(0, rowCount());
    invalidUrls.clear();
    watching.clear();
    addUrls(urls, 0);
}
void UrlModel::addUrls(const std::vector<QUrl> &list, int row, bool move)
{
    if (row == -1)
        row = rowCount();
    row = qMin(row, rowCount());
    for (int i = (int)list.size() - 1; i >= 0; --i) {
        QUrl url = list[i];
        if (!url.isValid() || url.scheme() != QLatin1String("file"))
            continue;
        //this makes sure the url is clean
        const QString cleanUrl = QDir::cleanPath(url.toLocalFile());
        url = QUrl::fromLocalFile(cleanUrl);

        for (int j = 0; move && j < rowCount(); ++j) {
            //QString local = index(j, 0).data(UrlRole).toUrl().toLocalFile();
            //#if defined(__NATRON_WIN32__)
            //if (index(j, 0).data(UrlRole).toUrl().toLocalFile().toLower() == cleanUrl.toLower()) {
            //#else
            if (index(j, 0).data(UrlRole).toUrl().toLocalFile() == cleanUrl) {
                //#endif
                removeRow(j);
                if (j <= row) {
                    --row;
                }
                break;
            }
        }
        row = qMax(row, 0);
        QModelIndex idx = fileSystemModel->index(cleanUrl);
        if (!fileSystemModel->isDir(idx))
            continue;
        insertRows(row, 1);
        setUrl(index(row, 0), url, idx);
        watching.push_back(make_pair(idx, cleanUrl));
    }
}


std::vector<QUrl> UrlModel::urls() const
{
    std::vector<QUrl> list;
    for (int i = 0; i < rowCount(); ++i)
        list.push_back(data(index(i, 0), UrlRole).toUrl());
    return list;
}

void UrlModel::setFileSystemModel(QFileSystemModel *model)
{
    if (model == fileSystemModel)
        return;
    if (fileSystemModel != 0) {
        disconnect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                   this, SLOT(dataChanged(QModelIndex,QModelIndex)));
        disconnect(model, SIGNAL(layoutChanged()),
                   this, SLOT(layoutChanged()));
        disconnect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                   this, SLOT(layoutChanged()));
    }
    fileSystemModel = model;
    if (fileSystemModel != 0) {
        connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                this, SLOT(dataChanged(QModelIndex,QModelIndex)));
        connect(model, SIGNAL(layoutChanged()),
                this, SLOT(layoutChanged()));
        connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                this, SLOT(layoutChanged()));
    }
    clear();
    insertColumns(0, 1);
}

void UrlModel::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    QModelIndex parent = topLeft.parent();
    for (int i = 0; i < (int)watching.size(); ++i) {
        QModelIndex index = watching[i].first;
        if (index.row() >= topLeft.row()
            && index.row() <= bottomRight.row()
            && index.column() >= topLeft.column()
            && index.column() <= bottomRight.column()
            && index.parent() == parent) {
            changed(watching[i].second);
        }
    }
}

void UrlModel::layoutChanged()
{
    QStringList paths;
    for (int i = 0; i < (int)watching.size(); ++i)
        paths.append(watching[i].second);
    watching.clear();
    for (int i = 0; i < paths.count(); ++i) {
        QString path = paths.at(i);
        QModelIndex newIndex = fileSystemModel->index(path);
        watching.push_back(std::pair<QModelIndex, QString>(newIndex, path));
        if (newIndex.isValid())
            changed(path);
    }
}
void UrlModel::changed(const QString &path)
{
    for (int i = 0; i < rowCount(); ++i) {
        QModelIndex idx = index(i, 0);
        if (idx.data(UrlRole).toUrl().toLocalFile() == path) {
            setData(idx, idx.data(UrlRole).toUrl());
        }
    }
}

FavoriteView::FavoriteView(QWidget *parent)
: QListView(parent)
, urlModel(0)
, _itemDelegate(0)
{
    // setAttribute(Qt::WA_MacShowFocusRect,0);
}

void FavoriteView::setModelAndUrls(QFileSystemModel *model, const std::vector<QUrl> &newUrls)
{
    setIconSize(QSize(24,24));
    setUniformItemSizes(true);
    assert(!urlModel);
    urlModel = new UrlModel(this);
    urlModel->setFileSystemModel(model);
    assert(!_itemDelegate);
    _itemDelegate = new FavoriteItemDelegate(model);
    setItemDelegate(_itemDelegate);
    setModel(urlModel);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(clicked(QModelIndex)));
    setDragDropMode(QAbstractItemView::DragDrop);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(showMenu(QPoint)));
    urlModel->setUrls(newUrls);
    setCurrentIndex(this->model()->index(0,0));

}

FavoriteView::~FavoriteView()
{
    delete urlModel;
    delete _itemDelegate;
}


QSize FavoriteView::sizeHint() const
{
    return QSize(150,QListView::sizeHint().height());//QListView::sizeHint();
}

void FavoriteView::selectUrl(const QUrl &url)
{
    disconnect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
               this, SLOT(clicked(QModelIndex)));

    selectionModel()->clear();
    for (int i = 0; i < model()->rowCount(); ++i) {
        if (model()->index(i, 0).data(UrlModel::UrlRole).toUrl() == url) {
            selectionModel()->select(model()->index(i, 0), QItemSelectionModel::Select);
            break;
        }
    }

    connect(selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(clicked(QModelIndex)));
}
void FavoriteView::removeEntry()
{
    QList<QModelIndex> idxs = selectionModel()->selectedIndexes();
    QList<QPersistentModelIndex> indexes;
    for (int i = 0; i < idxs.count(); ++i)
        indexes.append(idxs.at(i));

    for (int i = 0; i < indexes.count(); ++i)
        if (!indexes.at(i).data(UrlModel::UrlRole).toUrl().path().isEmpty())
            model()->removeRow(indexes.at(i).row());
}

void FavoriteView::rename(){
    QModelIndex index;
    QList<QModelIndex> idxs = selectionModel()->selectedIndexes();
    QList<QPersistentModelIndex> indexes;
    for (int i = 0; i < idxs.count(); ++i)  indexes.append(idxs.at(i));
    for (int i = 0; i < indexes.count(); ++i){
        if (!indexes.at(i).data(UrlModel::UrlRole).toUrl().path().isEmpty()){
            index = indexes.at(i);
            break;
        }
    }
    QString newName;
    QInputDialog dialog(this);
    dialog.setLabelText("Favorite name:");
    dialog.setWindowTitle("Rename favorite");
    if(dialog.exec()){
        newName = dialog.textValue();
    }
    rename(index,newName);
}

void FavoriteView::rename(const QModelIndex& index,const QString& name){
    model()->setData(index,name,Qt::EditRole);
}

void FavoriteView::editUrl(){
    QModelIndex index;
    QList<QModelIndex> idxs = selectionModel()->selectedIndexes();
    QList<QPersistentModelIndex> indexes;
    for (int i = 0; i < idxs.count(); ++i)  indexes.append(idxs.at(i));
    for (int i = 0; i < indexes.count(); ++i){
        if (!indexes.at(i).data(UrlModel::UrlRole).toUrl().path().isEmpty()){
            index = indexes.at(i);
            break;
        }
    }
    QString newName;
    QInputDialog dialog(this);
    dialog.setLabelText("Folder path:");
    dialog.setWindowTitle("Change folder path");
    if(dialog.exec()){
        newName = dialog.textValue();
    }
    QString cleanpath = QDir::cleanPath(newName);
    QUrl url = QUrl::fromLocalFile(newName);
    UrlModel *myurlModel = dynamic_cast<UrlModel*>(model());
    assert(myurlModel);
    QFileSystemModel* fileSystemModel = myurlModel->getFileSystemModel();
    assert(fileSystemModel);
    QModelIndex idx = fileSystemModel->index(cleanpath);
    if (!fileSystemModel->isDir(idx))
        return;
    myurlModel->setUrl(index, url, idx);
}

void FavoriteView::clicked(const QModelIndex &index)
{
    QUrl url = model()->index(index.row(), 0).data(UrlModel::UrlRole).toUrl();
    emit urlRequested(url);
    selectUrl(url);
}

void FavoriteView::showMenu(const QPoint &position)
{
    QList<QAction *> actions;
    if (indexAt(position).isValid()) {
        QAction *removeAction = new QAction("Remove", this);
        if (indexAt(position).data(UrlModel::UrlRole).toUrl().path().isEmpty())
            removeAction->setEnabled(false);
        connect(removeAction, SIGNAL(triggered()), this, SLOT(removeEntry()));
        actions.append(removeAction);

        QAction *editAction = new QAction("Edit path", this);
        if (indexAt(position).data(UrlModel::UrlRole).toUrl().path().isEmpty())
            editAction->setEnabled(false);
        connect(editAction, SIGNAL(triggered()), this, SLOT(editUrl()));
        actions.append(editAction);

    }
    if (actions.count() > 0){
        QMenu menu(this);
        menu.addActions(actions);
        menu.exec(mapToGlobal(position));
    }
}
void FavoriteView::keyPressEvent(QKeyEvent *event){
    if(event->key() == Qt::Key_Backspace){
        removeEntry();
    }
    QListView::keyPressEvent(event);
}

QStringList UrlModel::mimeTypes() const{
    return QStringList(QLatin1String("text/uri-list"));
}

Qt::ItemFlags UrlModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QStandardItemModel::flags(index);
    if (index.isValid()) {
        flags &= ~Qt::ItemIsEditable;
        // ### some future version could support "moving" urls onto a folder
        flags &= ~Qt::ItemIsDropEnabled;
    }

    if (index.data(Qt::DecorationRole).isNull())
        flags &= ~Qt::ItemIsEnabled;

    return flags;
}

QMimeData *UrlModel::mimeData(const QModelIndexList &indexes) const{
    QList<QUrl> list;
    for (int i = 0; i < indexes.count(); ++i) {
        if (indexes.at(i).column() == 0)
            list.append(indexes.at(i).data(UrlRole).toUrl());
    }
    QMimeData *data = new QMimeData();
    data->setUrls(list);
    return data;
}
bool UrlModel::canDrop(QDragEnterEvent *event){
    if (!event->mimeData()->formats().contains(mimeTypes().first()))
        return false;

    const QList<QUrl> list = event->mimeData()->urls();
    for (int i = 0; i < list.count(); ++i) {
        QModelIndex idx = fileSystemModel->index(list.at(0).toLocalFile());
        if (!fileSystemModel->isDir(idx))
            return false;
    }
    return true;
}

bool UrlModel::dropMimeData(const QMimeData *data, Qt::DropAction , int row, int , const QModelIndex &){
    if (!data->formats().contains(mimeTypes().first()))
        return false;
    std::vector<QUrl> urls;
    for(int i =0 ; i < data->urls().count() ; ++i) {
        urls.push_back(data->urls().at(i));
    }
    addUrls(urls, row);
    return true;
}

void FavoriteView::dragEnterEvent(QDragEnterEvent *event){
    if (urlModel->canDrop(event))
        QListView::dragEnterEvent(event);
}

void FileDialogComboBox::setFileDialogPointer(SequenceFileDialog *p){
    dialog = p;
    urlModel = new UrlModel(this);
    urlModel->setFileSystemModel(p->getFileSystemModel());
    setModel(urlModel);
}

void FileDialogComboBox::showPopup(){
    if (model()->rowCount() > 1)
        QComboBox::showPopup();

    urlModel->setUrls(std::vector<QUrl>());
    std::vector<QUrl> list;
    QModelIndex idx = dialog->getFileSystemModel()->index(dialog->rootPath());
    while (idx.isValid()) {
        QUrl url = QUrl::fromLocalFile(idx.data(QFileSystemModel::FilePathRole).toString());
        if (url.isValid())
            list.push_back(url);
        idx = idx.parent();
    }
    // add "my computer"
    list.push_back(QUrl::fromLocalFile(QLatin1String("")));
    urlModel->addUrls(list, 0);
    idx = model()->index(model()->rowCount() - 1, 0);

    // append history
    QList<QUrl> urls;
    for (int i = 0; i < m_history.count(); ++i) {
        QUrl path = QUrl::fromLocalFile(m_history.at(i));
        if (!urls.contains(path))
            urls.prepend(path);
    }
    if (urls.count() > 0) {
        model()->insertRow(model()->rowCount());
        idx = model()->index(model()->rowCount()-1, 0);
        // ### TODO maybe add a horizontal line before this
        model()->setData(idx,"Recent Places");
        QStandardItemModel *m = qobject_cast<QStandardItemModel*>(model());
        if (m) {
            Qt::ItemFlags flags = m->flags(idx);
            flags &= ~Qt::ItemIsEnabled;
            m->item(idx.row(), idx.column())->setFlags(flags);
        }
        std::vector<QUrl> stdUrls;
        for(int i = 0; i < urls.count() ; ++i) {
            stdUrls.push_back(urls.at(i));
        }
        urlModel->addUrls(stdUrls, -1, false);
    }
    setCurrentIndex(0);

    QComboBox::showPopup();
}

void FileDialogComboBox::setHistory(const QStringList &paths){
    m_history = paths;
    // Only populate the first item, showPopup will populate the rest if needed
    std::vector<QUrl> list;
    QModelIndex idx = dialog->getFileSystemModel()->index(dialog->rootPath());
    //On windows the popup display the "C:\", convert to nativeSeparators
    QUrl url = QUrl::fromLocalFile(QDir::toNativeSeparators(idx.data(QFileSystemModel::FilePathRole).toString()));
    if (url.isValid())
        list.push_back(url);
    urlModel->setUrls(list);
}
void FileDialogComboBox::paintEvent(QPaintEvent *){
    QStylePainter painter(this);
    painter.setPen(palette().color(QPalette::Text));

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


std::vector<QStringList> SequenceFileDialog::fileSequencesFromFilesList(const QStringList& files,const QStringList& supportedFileTypes){
    std::vector<QStringList> ret;
    typedef std::map<QString,std::map<QString,QStringList> >  Sequences; //<common name, <extention,files list> >
    Sequences sequences;
    for (int i = 0; i < files.size(); ++i) {
        QString file = files.at(i);
        QString rawFile = file;
        QString extension;
        int frameNumber;
        SequenceDialogProxyModel::parseFilename(file, &frameNumber, extension);

        //if the extension is not supported, ignore the file
        bool foundMatch = false;
        for(int i = 0 ; i < supportedFileTypes.size() ; ++i) {
            if(nocase_equal_string(extension.toStdString(), supportedFileTypes[i].toStdString())){
                foundMatch = true;
                break;
            }
        }
        if(!foundMatch){
            continue;
        }


        //try to find an existing sequence with the same common name
        Sequences::iterator it = sequences.find(file);
        if(it != sequences.end()){
            // try to find an existing sequence with this extension
            std::map<QString,QStringList>::iterator it2 = it->second.find(extension);
            if(it2 != it->second.end()){
                it2->second.append(rawFile);
            }else{
                //make a new sequence for this other extension
                QStringList list;
                list << rawFile;
                it->second.insert(std::make_pair(extension, list));
            }

        }else{
            //insert a new sequence
            QStringList list;
            list << rawFile;
            std::map<QString,QStringList> newMap;
            newMap.insert(std::make_pair(extension, list));
            sequences.insert(std::make_pair(file, newMap));
        }
    }

    //run through all the sequences and insert it into ret
    for (Sequences::iterator it = sequences.begin(); it!=sequences.end(); ++it) {
        QStringList list;
        for (std::map<QString,QStringList>::iterator it2 = it->second.begin(); it2!=it->second.end(); ++it2) {
            for (int i = 0; i < it2->second.size(); ++i) {
                list << it2->second.at(i);
            }
        }
        ret.push_back(list);
    }
    return ret;
}

void SequenceFileDialog::appendFilesFromDirRecursively(QDir* currentDir,QStringList* files){
    QStringList entries = currentDir->entryList();
    for (int i = 0; i < entries.size(); ++i) {
        const QString& e = entries.at(i);

        //ignore dot and dotdot
        if(e == "." || e == "..")
            continue;

        QString entryWithPath = currentDir->absoluteFilePath(e);

        //if it is a file, recurse
        QDir d(entryWithPath);
        if(d.exists()){
            appendFilesFromDirRecursively(&d, files);
            continue;
        }

        //else if it is a file, append it
        if(QFile::exists(entryWithPath)){
            files->append(entryWithPath);
        }
    }
}
