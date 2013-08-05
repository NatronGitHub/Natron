//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */


#include "framefiledialog.h"

#include <locale>
#include <algorithm>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QtGui/QPainter>
#include <QListView>
#include <QHeaderView>
#include <QCheckBox>
#include <QLabel>
#include <QFileIconProvider>
#include <QInputDialog>
#include <QSplitter>
#include <QtGui/QIcon>
#include <QtCore/QRegExp>
#include <QtGui/QKeyEvent>
#include <QtGui/QColor>
#include <QtGui/QAction>
#include <QMenu>
#include <QtGui/QApplication>
#include <QtGui/QStylePainter>
#include <QtCore/QEvent>
#include <QtCore/QMimeData>
#include <QtConcurrentRun>
#include <QtGui/QStyleOptionViewItem>
#include <QtCore/QSettings>

#include "Gui/button.h"
#include "Gui/lineEdit.h"
#include "Superviser/powiterFn.h"
#include "Superviser/MemoryInfo.h"

using namespace std;

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

SequenceFileDialog::SequenceFileDialog(QWidget* parent, // necessary to transmit the stylesheet to the dialog
                   const std::vector<std::string>& filters, // the user accepted file types
                   FileDialogMode mode, // if it is an open or save dialog
                   const std::string& currentDirectory): // the directory to show first
QDialog(parent),
_filters(filters),
_currentHistoryLocation(-1),
_dialogMode(mode)
{
    setWindowFlags(Qt::Window);
    _mainLayout = new QVBoxLayout(this);
   // _mainLayout->setSpacing(0);
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
    
    _lookInLabel = new QLabel("Look in :",_buttonsWidget);
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
    
    
    _sequenceButton = new QCheckBox("Sequences",_buttonsWidget);
    _sequenceButton->setChecked(true);
    _buttonsLayout->addWidget(_sequenceButton);
    QObject::connect(_sequenceButton,SIGNAL(clicked(bool)),this,SLOT(enableSequenceMode(bool)));
    
    
    _previewButton = new Button("preview",_buttonsWidget);
    _previewButton->setVisible(false);//!@todo Implement preview mode for the file dialog
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
    
    _selectionLineEdit = new LineEdit(_selectionWidget);
    _selectionLayout->addWidget(_selectionLineEdit);
    
    _openButton = new Button("Open",_selectionWidget);
    _selectionLayout->addWidget(_openButton);
    QObject::connect(_openButton, SIGNAL(pressed()), this, SLOT(openSelectedFiles()));
    
    _mainLayout->addWidget(_selectionWidget);
    
    /*creating filter zone*/
    _filterLineWidget = new QWidget(this);
    _filterLineLayout = new QHBoxLayout(_filterLineWidget);
    _filterLineLayout->setContentsMargins(0, 0, 0, 0);
    _filterLineWidget->setLayout(_filterLineLayout);
    
    
    _filterLabel = new QLabel("Filter",_filterLineWidget);
    _filterLineLayout->addWidget(_filterLabel);
    
    _filterWidget = new QWidget(_filterLineWidget);
    _filterLayout = new QHBoxLayout(_filterWidget);
    _filterWidget->setLayout(_filterLayout);
    _filterLayout->setContentsMargins(0,0,0,0);
    _filterLayout->setSpacing(0);
    
    _filterLineEdit = new LineEdit(_filterWidget);
    _filterLayout->addWidget(_filterLineEdit);
    
    QImage dropDownImg(IMAGES_PATH"combobox.png");
    QPixmap pixDropDown = QPixmap::fromImage(dropDownImg);
    QSize buttonSize(15,_filterLineEdit->sizeHint().height());
    pixDropDown = pixDropDown.scaled(buttonSize);
    _filterDropDown = new Button(QIcon(pixDropDown),"",_filterWidget);
    _filterDropDown->setFixedSize(buttonSize);
    _filterLayout->addWidget(_filterDropDown);
    QObject::connect(_filterDropDown,SIGNAL(clicked()),this,SLOT(showFilterMenu()));
    _filterLineLayout->addWidget(_filterWidget);
    
    _cancelButton = new Button("Cancel",_filterLineWidget);
    _filterLineLayout->addWidget(_cancelButton);
    QObject::connect(_cancelButton, SIGNAL(clicked()), this, SLOT(cancelSlot()));
    
    _mainLayout->addWidget(_filterLineWidget);
    
    resize(900, 400);
    
        std::vector<QUrl> initialBookmarks;
#ifndef __POWITER_WIN32__
    initialBookmarks.push_back(QUrl::fromLocalFile(QLatin1String("/")));
#else
    
    initialBookmarks.push_back(QUrl::fromLocalFile(QLatin1String("C:")));
    
#endif
    initialBookmarks.push_back(QUrl::fromLocalFile(QDir::homePath()));
    _favoriteView->setModelAndUrls(_model, initialBookmarks);
    
    if(!currentDirectory.empty())
        setDirectory(currentDirectory.c_str());
    
    QItemSelectionModel *selectionModel = _view->selectionModel();
    QObject::connect(selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),this, SLOT(selectionChanged()));
    QObject::connect(_filterLineEdit,SIGNAL(textEdited(QString)),this,SLOT(applyFilter(QString)));
    QObject::connect(_selectionLineEdit, SIGNAL(textChanged(QString)),this, SLOT(autoCompleteFileName(QString)));
    QObject::connect(_view, SIGNAL(customContextMenuRequested(QPoint)),
                     this, SLOT(showContextMenu(QPoint)));
    QObject::connect(_model, SIGNAL(rootPathChanged(QString)),
                     this, SLOT(pathChanged(QString)));
    createMenuActions();
    
    if(_dialogMode == OPEN_DIALOG){
        setWindowTitle("Open Sequence");
    }else{
        setWindowTitle("Save Sequence");
    }
    
     QSettings settings(QSettings::UserScope, QLatin1String("Powiter"));
     settings.beginGroup(QLatin1String("Powiter"));
     restoreState(settings.value(QLatin1String("filedialog")).toByteArray());

    
}
SequenceFileDialog::~SequenceFileDialog(){
    QSettings settings(QSettings::UserScope, QLatin1String("Powiter"));
    settings.beginGroup(QLatin1String("Powiter"));
    settings.setValue(QLatin1String("filedialog"), saveState());
    
    delete _model;
    delete _itemDelegate;
    delete _proxy;
    
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
    _sequenceButton->setChecked(b);
    if(!b){
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
        if (_model->isDir(mapToSource(indexes.at(i))))
            continue;
        allFiles.append(indexes.at(i).data(QFileSystemModel::FilePathRole).toString());
    }
    if (allFiles.count() > 1)
        for (int i = 0; i < allFiles.count(); ++i) {
            allFiles.replace(i, QString(QLatin1Char('"') + allFiles.at(i) + QLatin1Char('"')));
        }
    
    QString finalFiles = allFiles.join(QString(QLatin1Char(' ')));
    for(unsigned int i = 0 ; i < _nameMapping.size(); i++){
        if(finalFiles == _nameMapping[i].first){
            finalFiles =  _nameMapping[i].second.second;
            break;
        }
    }
    if (!finalFiles.isEmpty())
        _selectionLineEdit->setText(finalFiles);
    
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
    _createDirButton->setEnabled(_model->flags(root) & Qt::ItemIsDropEnabled);
    if(newDirectory.at(newDirectory.size()-1) != QChar('/')){
        newDirectory.append("/");
    }
    
    _selectionLineEdit->setText(newDirectory);
    
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
    QtConcurrent::run(this,&SequenceFileDialog::itemsToSequence,proxyIndex);
    /*update the view to show the newly loaded directory*/
    setRootIndex(proxyIndex);
    _view->expandColumnsToFullWidth();
    /*clearing selection*/
    _view->selectionModel()->clear();
}

bool SequenceFileDialog::sequenceModeEnabled() const{
    return _sequenceButton->isChecked();
}


bool SequenceDialogProxyModel::isAcceptedByUser(const QString &path) const{
    if(_filter.isEmpty()) return true;
    QRegExp rx(_filter,Qt::CaseInsensitive,QRegExp::Wildcard);
    if(!rx.isValid())
        return true;
    return rx.exactMatch(path);
}
void FileSequence::addToSequence(int frameIndex,const QString& path){
    if(_frameIndexes.addToSequence(frameIndex)){
        QFile f(path);
        _totalSize += f.size();
        
    }
}

/*Complex filter to actually extract the sequences from the file system*/
bool SequenceDialogProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const{
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
     This function returns false if the file type is not supported by
     the filters set at the creation of the dialog.*/
    if(!parseFilename(pathCpy,&frameNumber,extension))
        return false;
    
    /*If file sequence fetching is disabled, just call the base class version*/
    if(!_fd->sequenceModeEnabled() || frameNumber == -1){
        return QSortFilterProxyModel::filterAcceptsRow(source_row,source_parent);
    }
    
    /*Locking the access to the frame sequences multi-map*/
    QMutexLocker g(&_lock);
    pair<SequenceIterator,SequenceIterator> it = _frameSequences.equal_range(pathCpy.toStdString());
    if(it.first != it.second){
        /*we found a matching sequence name, we need to figure out if it has the same file type*/
        for(SequenceIterator it2 = it.first ;it2!=it.second;it2++){
            if(it2->second._fileType == extension.toStdString()){
                it2->second.addToSequence(frameNumber,path);
                if(frameNumber == it2->second._frameIndexes.firstFrame())
                    return true;
                return false;
            }
        }
        /*if it reaches here it means there's already a sequence with the same name but
         *with a different file type
         */
        FileSequence _frames(extension.toStdString());
        _frames.addToSequence(frameNumber,path);
        _frameSequences.insert(make_pair(pathCpy.toStdString(),_frames));
        return true;
        
    }else{ /*couldn't find a sequence with the same name, we create one*/
        FileSequence _frames(extension.toStdString());
        _frames.addToSequence(frameNumber,path);
        _frameSequences.insert(make_pair(pathCpy.toStdString(),_frames));
        return true;
    }
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
    // for(SequenceIterator it = _frameSequences.begin() ; it!= _frameSequences.end(); it++){
    //     cout << it->first << " = " << it->second.second.size() << "x " << it->second.first << endl;
    // }
    for(int c = 0 ; c < _proxy->rowCount(parent) ; c++){
        QModelIndex item = _proxy->index(c,0,parent);
        /*We skip directories*/
        if(_model->isDir(_proxy->mapToSource(item))){
            continue;
        }
        QString name = item.data(QFileSystemModel::FilePathRole).toString();
        QString originalName = name;
        QString extension = SequenceFileDialog::removeFileExtension(name);
        int i = name.size() - 1;
        QString fNumber ;
        while(i>=0 && name.at(i).isDigit()){fNumber.prepend(name.at(i)); i--;}
        if(i == 0 || i == name.size()-1){
            continue;
        }
        name = name.left(i+1);
        const FileSequence frameRanges = frameRangesForSequence(name.toStdString(),extension.toStdString());
        /*we don't display sequences with no frame contiguous like 10-13-15.
         *This is corner case and is not useful anyway, we rather display it as several files
         */
        if(!frameRanges._frameIndexes.isEmpty()){
            vector< pair<int,int> > chunks;
            int k = frameRanges._frameIndexes.firstFrame();
            
            while(k <= frameRanges._frameIndexes.lastFrame()){
                chunks.push_back(make_pair(k,-1));
                int j = k+1;
                int prev = k;
                int count = 1;
                while(j <= frameRanges._frameIndexes.lastFrame() && j == prev+1){ prev = j;j++; count++;}
                j--;
                chunks.back().second = j;
                k+=count;
            }
            
            name.append("#.");
            name.append(extension);
            
            if(chunks.size() == 1){
                name.append(QString(" %1-%2").arg(QString::number(chunks[0].first)).arg(QString::number(chunks[0].second)));
            }else{
                name.append(" ( ");
                for(unsigned int i = 0 ; i < chunks.size() ; i++){
                    if(chunks[i].first != chunks[i].second){
                        name.append(QString(" %1-%2").arg(QString::number(chunks[i].first)).arg(QString::number(chunks[i].second)));
                    }else{
                        name.append(QString(" %1").arg(QString::number(chunks[i].first)));
                    }
                    if(i < chunks.size() -1) name.append(" /");
                }
                name.append(" ) ");
            }
            bool foundExistingMapping = false;
            for(unsigned int j = 0 ; j < _nameMapping.size();j++){
                if(_nameMapping[j].first == originalName){
                    foundExistingMapping = true;
                    break;
                }
            }
            if(!foundExistingMapping){
                //        cout << "mapping: " << originalName.toStdString() << " TO " << name.toStdString() << endl;
                _nameMapping.push_back(make_pair(originalName,make_pair(frameRanges._totalSize,name)));
            }
        }
    }
    
    _view->updateNameMapping(_nameMapping);
}
void SequenceFileDialog::setRootIndex(const QModelIndex& index){
    _view->setRootIndex(index);
}


void SequenceFileDialog::setFrameSequence(FrameSequences frameSequences){
    /*Removing from the sequence any element with a sequence of 1 element*/
    _frameSequences.clear();
    for(SequenceDialogProxyModel::ConstSequenceIterator it = frameSequences.begin(); it!=frameSequences.end();it++){
        if(it->second._frameIndexes.size() > 1){
            _frameSequences.insert(*it);
        }
    }
    
}
const FileSequence SequenceFileDialog::frameRangesForSequence(const std::string& sequenceName, const std::string& extension) const{
    pair<ConstSequenceIterator,ConstSequenceIterator> found =  _frameSequences.equal_range(sequenceName);
    for(SequenceDialogProxyModel::ConstSequenceIterator it = found.first ;it!=found.second;it++){
        if(it->second._fileType == extension){
            return it->second;
            break;
        }
    }
    return FileSequence("empty");
}


bool SequenceDialogProxyModel::parseFilename(QString& path,int* frameNumber,QString &extension) const{
    
    /*removing the file extension if any*/
    
    extension = SequenceFileDialog::removeFileExtension(path);
    if(!_fd->isASupportedFileExtension(extension.toStdString())){
        return false;
    }
    /*removing the frame number if any*/
    int i = path.size() - 1;
    QString fNumber ;
    int digitCount = 0;
    while(i>=0 && path.at(i).isDigit()){fNumber.prepend(path.at(i)); i--; digitCount++;}
    path = path.left(i+1);
    if((i == 0 && path.size() > 1)|| !digitCount){
        *frameNumber = -1;
        path = "";
    }else{
        *frameNumber = fNumber.toInt();
    }
    return true;
    
}

SequenceDialogView::SequenceDialogView(QWidget* parent):QTreeView(parent){
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
}
void SequenceDialogView::updateNameMapping(std::vector<std::pair<QString, std::pair<qint64, QString> > > nameMapping){
    dynamic_cast<SequenceItemDelegate*>(itemDelegate())->setNameMapping(nameMapping);
}


SequenceItemDelegate::SequenceItemDelegate(SequenceFileDialog* fd) : QStyledItemDelegate(),_maxW(200),_fd(fd){}

void SequenceItemDelegate::setNameMapping(std::vector<std::pair<QString, std::pair<qint64, QString> > > nameMapping){
    _nameMapping.clear();
    _maxW = 200;
    QFont f("Times",6);
    QFontMetrics metric(f);
    for(unsigned int i = 0 ; i < nameMapping.size() ; i++){
        pair<QString,pair<qint64,QString> >& p = nameMapping[i];
        _nameMapping.push_back(p);
        int w = metric.width(p.second.second);
        if(w > _maxW) _maxW = w;
    }
}

void SequenceItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem &option, const QModelIndex & index) const{
    if(index.column() == 0){
        QString str = index.data().toString();
        bool found = false;
        for(unsigned int i =0 ;i < _nameMapping.size() ;i++){
            if(SequenceFileDialog::removePath(_nameMapping[i].first) == str){
                QRect r;
                if (option.state & QStyle::State_Selected){
                    painter->fillRect(option.rect, option.palette.highlight());
                }
                QString nameToPaint = SequenceFileDialog::removePath(_nameMapping[i].second.second);
                int totalSize = option.rect.width();
                int iconSize = option.decorationSize.width();
                int textSize = totalSize - iconSize;
                
                QFileInfo fileInfo(index.data(QFileSystemModel::FilePathRole).toString());
                QIcon icon = _fd->getFileSystemModel()->iconProvider()->icon(fileInfo);
                
                QRect iconRect(option.rect.x(),option.rect.y(),iconSize,option.rect.height());
                QRect textRect(option.rect.x()+iconSize,option.rect.y(),textSize,option.rect.height());
                
                painter->drawPixmap(iconRect,
                                    icon.pixmap(icon.actualSize(QSize(iconRect.width(),iconRect.height()))),
                                    r);
                painter->drawText(textRect,Qt::TextSingleLine,nameToPaint,&r);
                
                found = true;
                
                break;
            }
        }
        if(!found){ // must be a directory or a single image file
            QStyledItemDelegate::paint(painter,option,index);
            return;
            
        }
    }else if(index.column() == 1){
        QFileSystemModel* model = _fd->getFileSystemModel();
        QModelIndex modelIndex = _fd->mapToSource(index);
        QModelIndex idx = model->index(modelIndex.row(),0,modelIndex.parent());
        QString str = idx.data().toString();
        bool found = false;
        for(unsigned int i =0 ;i < _nameMapping.size() ;i++){
            if(SequenceFileDialog::removePath(_nameMapping[i].first) == str){
                QRect r;
                if (option.state & QStyle::State_Selected){
                    painter->fillRect(option.rect, option.palette.highlight());
                }
                string memString = printAsRAM(_nameMapping[i].second.first,2);
                QString nameToPaint(memString.c_str());
                painter->drawText(option.rect,Qt::TextSingleLine,nameToPaint,&r);
                found = true;
                break;
            }
        }
        if(!found){
            if(index.column() == 3){
                _fd->getSequenceView()->setColumnWidth(3,_fd->getSequenceView()->width()/4);
            }
            QStyledItemDelegate::paint(painter,option,index);
        }
    }else{
        QStyledItemDelegate::paint(painter,option,index);
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
    for(unsigned int i = 0 ; i < _filters.size() ; i++){
        if(nocase_equal_string(ext, _filters[i]))
            return true;
    }
    return false;
}
QString SequenceFileDialog::removeFileExtension(QString& filename){
    int i = filename.size() -1;
    QString extension;
    while(i>=0 && filename.at(i) != QChar('.')){extension.prepend(filename.at(i)); i--;}
    filename = filename.left(i);
    return extension;
}
QString SequenceFileDialog::removePath(const QString& str){
    int i = str.size() - 1;
    int count = 0;
    bool hasHitExtension = false;
    while(i >= 0 ){
        if(str.at(i) == QChar('.'))
            hasHitExtension = true;
        if(str.at(i) == QChar('/') && hasHitExtension){
            break;
        }
        i--;
        count++;
    }
    return str.right(count);
}

bool SequenceFileDialog::checkIfContiguous(const std::vector<int>& v){
    for(unsigned int i = 0 ; i < v.size() ;i++){
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
                    folderName = newFolderString + QString::number(suffix++);
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
void SequenceFileDialog::addFavorite(){
    QInputDialog dialog(this);
    dialog.setLabelText("Folder path:");
    dialog.setWindowTitle("New favorite");
    QString newFavName,newFavPath;
    if(dialog.exec()){
        newFavName = dialog.textValue();
        newFavPath = dialog.textValue();
        addFavorite(newFavName,newFavPath);
    }
    
}

void SequenceFileDialog::addFavorite(const QString& name,const QString& path){
    if(!name.isEmpty() && !path.isEmpty()){
        std::vector<QUrl> list;
        list.push_back(QUrl::fromLocalFile(path));
        _favoriteView->addUrls(list,-1);
    }
}

void SequenceFileDialog::openSelectedFiles(){
    QString str = _selectionLineEdit->text();
    if(!isDirectory(str)){
        QStringList files = selectedFiles();
        if(!files.isEmpty()){
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
        Q_UNUSED(q)
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
        QString prefix = dir.absolutePath()+QDir::separator();
        QModelIndex sequenceIndex = indexes.at(0);
        QString path = sequenceIndex.data().toString();
        path = prefix+path;
        QString originalPath = path;
        QString ext = SequenceFileDialog::removeFileExtension(path);
        int i  = path.size() -1;
        while(i >= 0 && path.at(i).isDigit()){i--;}
        i++;
        path = path.left(i);
        // FileSequence *sequence = 0;
        pair<SequenceIterator,SequenceIterator> range = _frameSequences.equal_range(path.toStdString());
        
        /*if this is not a registered sequence. i.e: this is a single image file*/
        if(range.first == range.second){
            //  cout << originalPath.toStdString() << endl;
            out.append(originalPath);
        }else{
//            for(SequenceIterator it = range.first ; it!=range.second; it++){
//                if(it->second._fileType == ext.toStdString()){
//                    sequence = &(it->second);
//                    break;
//                }
//            }
            /*now that we now how many frames there are,their extension and the common name, we retrieve them*/
            i = 0;
            QStringList dirEntries = dir.entryList();
            for(int j = 0 ; j < dirEntries.size(); j++){
                QString s = dirEntries.at(j);
                s = prefix+s;
                
                if(QFile::exists(s) && s.contains(path) && s.contains(ext)){
                    out.append(s);
                    i++;
                }
                //if(out.size() > (int)(sequence->_frameIndexes.size())) break; //number of files exceeding the sequence size...something is wrong
            }
            
        }
    }
    return out;
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
            for(unsigned int i = 0 ; i < invalidUrls.size(); i++){
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
            //#if defined(__POWITER_WIN32__)
            //if (index(j, 0).data(UrlRole).toUrl().toLocalFile().toLower() == cleanUrl.toLower()) {
            //#else
            if (index(j, 0).data(UrlRole).toUrl().toLocalFile() == cleanUrl) {
                //#endif
                removeRow(j);
                if (j <= row)
                    row--;
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

FavoriteView::FavoriteView(QWidget *parent) : QListView(parent)
{
   // setAttribute(Qt::WA_MacShowFocusRect,0);
}

void FavoriteView::setModelAndUrls(QFileSystemModel *model, const std::vector<QUrl> &newUrls)
{
    setIconSize(QSize(24,24));
    setUniformItemSizes(true);
    urlModel = new UrlModel(this);
    urlModel->setFileSystemModel(model);
    setItemDelegate(new FavoriteItemDelegate(model));
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
    for (int i = 0; i < idxs.count(); i++)
        indexes.append(idxs.at(i));
    
    for (int i = 0; i < indexes.count(); ++i)
        if (!indexes.at(i).data(UrlModel::UrlRole).toUrl().path().isEmpty())
            model()->removeRow(indexes.at(i).row());
}

void FavoriteView::rename(){
    QModelIndex index;
    QList<QModelIndex> idxs = selectionModel()->selectedIndexes();
    QList<QPersistentModelIndex> indexes;
    for (int i = 0; i < idxs.count(); i++)  indexes.append(idxs.at(i));
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
    for (int i = 0; i < idxs.count(); i++)  indexes.append(idxs.at(i));
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
    UrlModel *urlModel = dynamic_cast<UrlModel*>(model());
    QFileSystemModel* fileSystemModel = urlModel->getFileSystemModel();
    QModelIndex idx = fileSystemModel->index(cleanpath);
    if (!fileSystemModel->isDir(idx))
        return;
    urlModel->setUrl(index, url, idx);
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
QByteArray SequenceFileDialog::saveState() const{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    QList<QUrl> urls;
    std::vector<QUrl> stdUrls = _favoriteView->urls();
    for(unsigned int i = 0 ; i < stdUrls.size(); i++){
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
    for(int i = 0 ; i < bookmarks.count() ; i++){
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
    for(int i =0 ; i < data->urls().count() ; i++){
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
        for(int i = 0; i < urls.count() ; i++){
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


bool FileSequence::FrameIndexes::isInSequence(int frameIndex) const{
    if(_firstFrame == 999999) return false;
    if (frameIndex == _firstFrame || frameIndex == _lastFrame) {
        return true;
    }
    // assuming frameIndex is positive,offset is the index in the bits to look for (starting at 0)
    int offset = frameIndex - _firstFrame - 1;
    int bitsIndex = offset / 64; //accessing the proper variable containing the bit
    int bitsOffset = (offset+1) % (64+1);
    quint64 frames = _bits[bitsIndex];
    quint64 base = 1;
    return frames & ( base << bitsOffset);
    
}
bool FileSequence::FrameIndexes::addToSequence(int frameIndex){
    if (frameIndex == _firstFrame || frameIndex == _lastFrame) {
        return false;
    }
    if(_firstFrame == 999999){
        _firstFrame = frameIndex;
        _lastFrame = _firstFrame;
        _isEmpty = false;
        _size++;
        return true;
    }
    int offset = frameIndex - _firstFrame -1; // assuming frameIndex is positive,offset is the index in the bits to look for (starting at 0)
    int bitsIndex = offset / 64; //accessing the proper variable containing the bit
    int bitsOffset = (offset+1) % (64+1);
    quint64 frames = 0;
    if(bitsIndex < (int)_bits.size()){
        frames = _bits[bitsIndex];
    }
    quint64 base = 1;
    if(!(frames & ( base << bitsOffset))){
        frames |= (base << bitsOffset);
        if(bitsIndex >= (int)_bits.size()){
            _bits.push_back(frames);
        }
        _size++;
        _isEmpty = false;
        if(frameIndex < _firstFrame)
            _firstFrame = frameIndex;
        if(frameIndex > _lastFrame)
            _lastFrame = frameIndex;
        return true;
    }
    return false;
    
}
