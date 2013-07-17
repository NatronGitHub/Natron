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
#include <QtCore/QDir>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtGui/QPainter>
#include <QtWidgets/QListView>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QLabel>
#include "Gui/button.h"
#include "Gui/lineEdit.h"
#include <algorithm>

using namespace std;

SequenceFileDialog::SequenceFileDialog(QWidget* parent, std::vector<std::string> filters,std::string directory ):
QDialog(parent),_filters(filters)
{
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
    QObject::connect(_itemDelegate,SIGNAL(contentSizeChanged(QSize)),_view,SLOT(adjustSizeToNewContent(QSize)));
    QObject::connect(_model,SIGNAL(directoryLoaded(QString)),this,SLOT(updateView(QString)));
    QObject::connect(_view,SIGNAL(clicked(QModelIndex)),this,SLOT(enterDirectory(QModelIndex)));
    if(directory.empty()){
        directory = QDir::currentPath().toStdString();
    }
    setDirectory(directory.c_str());
    
    /*creating GUI*/
    _buttonsWidget = new QWidget(this);
    _buttonsLayout = new QHBoxLayout(_buttonsWidget);
    _buttonsLayout->setContentsMargins(0, 0, 0, 0);
    _buttonsWidget->setLayout(_buttonsLayout);
    
    _buttonsLayout->addStretch();
    
    _previousButton = new Button(style()->standardIcon(QStyle::SP_ArrowBack),"",_buttonsWidget);
    _buttonsLayout->addWidget(_previousButton);
    
    _nextButton = new Button(style()->standardIcon(QStyle::SP_ArrowForward),"",_buttonsWidget);
    _buttonsLayout->addWidget(_nextButton);
    
    _upButton = new Button(style()->standardIcon(QStyle::SP_ArrowUp),"",_buttonsWidget);
    _buttonsLayout->addWidget(_upButton);
    
    _createDirButton = new Button(style()->standardIcon(QStyle::SP_FileDialogNewFolder),"",_buttonsWidget);
    _buttonsLayout->addWidget(_createDirButton);
    
    _previewButton = new Button("preview",_buttonsWidget);
    _buttonsLayout->addWidget(_previewButton);
    
    
    _mainLayout->addWidget(_buttonsWidget);
    
    /*creating center*/
    _centerWidget = new QWidget(this);
    _favoriteWidget = new QWidget(this);
    _favoriteLayout = new QVBoxLayout(_favoriteWidget);
    _favoriteWidget->setLayout(_favoriteLayout);
    _favoriteView = new QListView(this);
    _favoriteLayout->setContentsMargins(0, 0, 0, 0);
    _favoriteLayout->addWidget(_favoriteView);
    
    _addFavoriteButton = new Button("+",this);
    _favoriteLayout->addWidget(_addFavoriteButton);
    
    
    _centerLayout = new QHBoxLayout(_centerWidget);
    _centerLayout->setContentsMargins(0, 0, 0, 0);
    _centerWidget->setLayout(_centerLayout);
    
    _centerLayout->addWidget(_favoriteWidget);
    _centerLayout->addWidget(_view);
    
    _mainLayout->addWidget(_centerWidget);
    
    /*creating selection widget*/
    _selectionWidget = new QWidget(this);
    _selectionLayout = new QHBoxLayout(_selectionWidget);
    _selectionLayout->setContentsMargins(0, 0, 0, 0);
    _selectionWidget->setLayout(_selectionLayout);
    
    _selectionLineEdit = new LineEdit(_selectionWidget);
    _selectionLayout->addWidget(_selectionLineEdit);
    
    _openButton = new Button("Open",_selectionWidget);
    _selectionLayout->addWidget(_openButton);
    
    _mainLayout->addWidget(_selectionWidget);
    
    /*creating filter zone*/
    _filterWidget = new QWidget(this);
    _filterLayout = new QHBoxLayout(_filterWidget);
    _filterLayout->setContentsMargins(0, 0, 0, 0);
    _filterWidget->setLayout(_filterLayout);
    
    _sequenceLabel = new QLabel("sequences",_filterWidget);
    _filterLayout->addWidget(_sequenceLabel);
    
    _sequenceCheckbox = new QCheckBox(_filterWidget);
    _filterLayout->addWidget(_sequenceCheckbox);
    
    _filterLabel = new QLabel("Filter",_filterWidget);
    _filterLayout->addWidget(_filterLabel);
    
    _filterLineEdit = new LineEdit(_filterWidget);
    _filterLayout->addWidget(_filterLineEdit);
    
    _cancelButton = new Button("Cancel",_filterWidget);
    _filterLayout->addWidget(_cancelButton);
    
    _mainLayout->addWidget(_filterWidget);

    
}
SequenceFileDialog::~SequenceFileDialog(){
    delete _model;
    delete _itemDelegate;
    delete _proxy;
}
void SequenceFileDialog::enterDirectory(const QModelIndex& index){
    
    QModelIndex sourceIndex = index.model() == _proxy ? mapToSource(index) : index;
    QString path = sourceIndex.data(QFileSystemModel::FilePathRole).toString();
    if (path.isEmpty() || _model->isDir(sourceIndex)) {
        
        setDirectory(path);
    }
}

void SequenceFileDialog::updateView(const QString &directory){
    if(directory != _requestedDir) return;
    QModelIndex root = _model->index(directory);
    QModelIndex proxyIndex = mapFromSource(root);
    itemsToSequence(proxyIndex);
    setRootIndex(proxyIndex);
    _view->selectionModel()->clear();
    _view->resizeColumnToContents(0);
}
void SequenceFileDialog::itemsToSequence(const QModelIndex& parent){
    /*Iterating over all the content of the parent directory.
     *Note that only 1 item is left per sequence already,
     *We just need to change its name to reflect the number
     *of elements in the sequence.
     */
    _nameMapping.clear();
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
        
        const vector<int> frameRanges = frameRangesForSequence(name.toStdString(),extension.toStdString());
        /*we don't display sequences with no frame contiguous like 10-13-15.
         *This is corner case and is not useful anyway, we rather display it as several files
         */
        
        
        if(frameRanges.size() > 0){
            vector< pair<int,int> > chunks;
            unsigned int k = 0;
            
            while(k < frameRanges.size()){
                chunks.push_back(make_pair(frameRanges[k],-1));
                unsigned int j = k+1;
                int prev = frameRanges[k];
                int count = 1;
                while(j < frameRanges.size() && frameRanges[j] == prev+1){ prev = frameRanges[j];j++; count++;}
                j--;
                chunks.back().second = frameRanges[j];
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
            _nameMapping.push_back(make_pair(SequenceFileDialog::removePath(originalName),SequenceFileDialog::removePath(name)));
        }
    }
    _view->updateNameMapping(_nameMapping);
}
void SequenceFileDialog::setRootIndex(const QModelIndex& index){
    _view->setRootIndex(index);
}

void SequenceFileDialog::setDirectory(const QString &directory){
    QString newDirectory = directory;
    //we remove .. and . from the given path if exist
    if (!directory.isEmpty())
        newDirectory = QDir::cleanPath(directory);
    
    if (!directory.isEmpty() && newDirectory.isEmpty())
        return;
    _requestedDir = newDirectory;
    _model->setRootPath(newDirectory);
    _proxy->clear();
}



QModelIndex SequenceFileDialog::mapToSource(const QModelIndex& index){
    return _proxy->mapToSource(index);
    
}
QModelIndex SequenceFileDialog::mapFromSource(const QModelIndex& index){
    QModelIndex ret =  _proxy->mapFromSource(index);
    setFrameSequence(_proxy->getFrameSequenceCopy());
    _proxy->clear();
    return ret;
}
void SequenceFileDialog::setFrameSequence(std::multimap<string, std::pair<string, std::vector<int> > > frameSequences){
    /*Removing from the sequence any element with a sequence of 1 element*/
    _frameSequences.clear();
    for(SequenceDialogProxyModel::ConstSequenceIterator it = frameSequences.begin(); it!=frameSequences.end();it++){
        if(it->second.second.size() > 1){
            _frameSequences.insert(*it);
        }
    }
    
}
const std::vector<int> SequenceFileDialog::frameRangesForSequence(std::string sequenceName,std::string extension) const{
    vector<int> ret;
    pair<SequenceDialogProxyModel::ConstSequenceIterator,SequenceDialogProxyModel::ConstSequenceIterator> found =  _frameSequences.equal_range(sequenceName);
    for(SequenceDialogProxyModel::ConstSequenceIterator it = found.first ;it!=found.second;it++){
        if(it->second.first == extension){
            ret = it->second.second;
            break;
        }
    }
    std::sort(ret.begin(),ret.end());
    return ret;
}

bool SequenceDialogProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const{
    QModelIndex item = sourceModel()->index(source_row,0,source_parent);
    QString path = item.data(QFileSystemModel::FilePathRole).toString();
    if(qobject_cast<QFileSystemModel*>(sourceModel())->isDir(item)){
        return true;
    }
    int frameNumber;
    QString pathCpy = path;
    QString extension;
    if(!parseFilename(pathCpy,&frameNumber,extension))
        return false;
    pair<SequenceIterator,SequenceIterator> it = _frameSequences.equal_range(pathCpy.toStdString());
    if(it.first != it.second){
        /*we found a matching sequence name, we need to figure out if it has the same file type*/
        for(SequenceIterator it2 = it.first ;it2!=it.second;it2++){
            if(it2->second.first == extension.toStdString()){
                it2->second.second.push_back(frameNumber);
                return false;
            }
        }
        /*if it reaches here it means there's already a sequence with the same name but
         *with a different file type
         */
        vector<int> _frames;
        _frames.push_back(frameNumber);
        _frameSequences.insert(make_pair(pathCpy.toStdString(),make_pair(extension.toStdString(),_frames)));
        return true;
        
    }else{ /*couldn't find a sequence with the same name, we create one*/
        vector<int> _frames;
        _frames.push_back(frameNumber);
        _frameSequences.insert(make_pair(pathCpy.toStdString(),make_pair(extension.toStdString(),_frames)));
        return true;
    }
    return true;
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
        return false;
    }
    *frameNumber = fNumber.toInt();
    return true;
    
}

SequenceDialogView::SequenceDialogView(QWidget* p):QTreeView(p){
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setRootIsDecorated(false);
    setItemsExpandable(false);
    setSortingEnabled(true);
    header()->setSortIndicator(0, Qt::AscendingOrder);
    header()->setStretchLastSection(false);
    setTextElideMode(Qt::ElideMiddle);
    setEditTriggers(QAbstractItemView::EditKeyPressed);
    setContextMenuPolicy(Qt::CustomContextMenu);
    
}
void SequenceDialogView::updateNameMapping(std::vector<std::pair<QString,QString> > nameMapping){
    dynamic_cast<SequenceItemDelegate*>(itemDelegate())->setNameMapping(nameMapping);
}
void SequenceDialogView::adjustSizeToNewContent(QSize size){
    setColumnWidth(0,size.width());
}

void SequenceItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const{
    if(index.column() != 0){
        QStyledItemDelegate::paint(painter,option,index);
        return;
    }
    QString str = index.data().toString();
    bool found = false;
    for(unsigned int i =0 ;i < _nameMapping.size() ;i++){
        if(_nameMapping[i].first == str){
            QRect r;
            if (option.state & QStyle::State_Selected)
                painter->fillRect(option.rect, option.palette.highlight());
            painter->drawText(option.rect,Qt::TextSingleLine,_nameMapping[i].second,&r);
            found = true;
            if(_automaticResize){
                emit contentSizeChanged(painter->window().size());
                _automaticResize = false;
            }
            return;
        }
    }
    if(!found){ // must be a directory or a single image file
        
        QRect r;
        QString extension = SequenceFileDialog::removeFileExtension(str);
        QFont f = option.font;
        if(!_fd->isASupportedFileExtension(extension.toStdString())){
            if (option.state & QStyle::State_Selected)
                painter->fillRect(option.rect, option.palette.highlight());
            f.setBold(true);
            painter->setFont(f);
            str.append("/");
        }else{
            str.append(".");
            str.append(extension);
        }
        painter->drawText(option.rect,Qt::TextSingleLine,str,&r);
        f.setBold(false);
        painter->setFont(f);
        if(_automaticResize){
            emit contentSizeChanged(painter->window().size());
            _automaticResize = false;
        }
    }
    
}
QSize SequenceItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const {
    if(index.column() != 0){
        return QStyledItemDelegate::sizeHint(option,index);
    }
    QString str =  index.data().toString();
    QFontMetrics metric(option.font);
    
    return QSize(metric.width(str),metric.height());
}
bool SequenceFileDialog::isASupportedFileExtension(std::string ext) const{
    for(unsigned int i = 0 ; i < _filters.size() ; i++){
        if(ext == _filters[i])
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
