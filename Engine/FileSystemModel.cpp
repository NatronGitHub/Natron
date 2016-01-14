/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "FileSystemModel.h"

#include <vector>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QFileInfo>
#include <QtCore/QDateTime>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QUrl>
#include <QtCore/QMimeData>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include <SequenceParsing.h>



static QStringList getSplitPath(const QString& path)
{
	if (path.isEmpty()) {
		return QStringList();
	}
	QString pathCpy = path;

	bool isdriveOrRoot;
#ifdef __NATRON_WIN32__
	QString startPath = pathCpy.mid(0,3);
	 isdriveOrRoot = FileSystemModel::isDriveName(startPath);
	if (isdriveOrRoot) {
		pathCpy = pathCpy.remove(0,3);
	}

	QStringList splitPath = pathCpy.split('/');
	if (isdriveOrRoot) {
		splitPath.prepend(startPath.mid(0,3));
	}

#else
	 isdriveOrRoot = pathCpy.startsWith("/");
	if (isdriveOrRoot) {
		pathCpy = pathCpy.remove(0,1);
	}
	QStringList splitPath = pathCpy.split('/');
	if (isdriveOrRoot) {
		splitPath.prepend("/");
	}

#endif
	if (!isdriveOrRoot && pathCpy[pathCpy.size() - 1] == QChar('/')) {
		pathCpy = pathCpy.mid(0,pathCpy.size() - 1);
	} 
	return splitPath;
}

/////////FileSystemItem

static QString generateChildAbsoluteName(FileSystemItem* parent,const QString& name)
{
    QString childName = parent->absoluteFilePath();
    if (!childName.endsWith('/')) {
        childName.append('/');
    }
    childName.append(name);
    return childName;
}

struct FileSystemItemPrivate
{
    
    FileSystemItem* parent;
    std::vector< boost::shared_ptr<FileSystemItem> > children; ///vector for random access
    QMutex childrenMutex;

    bool isDir;
    QString filename;
    QString userFriendlySequenceName;
    
    ///This will be set when the file system model is in sequence mode and this is a file
    boost::shared_ptr<SequenceParsing::SequenceFromFiles> sequence;
    
    QDateTime dateModified;
    quint64 size;
    QString fileExtension;
    QString absoluteFilePath;
    
    FileSystemItemPrivate(bool isDir,const QString& filename,const QString& userFriendlySequenceName,const boost::shared_ptr<SequenceParsing::SequenceFromFiles>& sequence,
                          const QDateTime& dateModified,quint64 size,FileSystemItem* parent)
    : parent(parent)
    , children()
    , childrenMutex()
    , isDir(isDir)
    , filename(filename)
    , userFriendlySequenceName(userFriendlySequenceName)
    , sequence(sequence)
    , dateModified(dateModified)
    , size(size)
    , fileExtension()
    , absoluteFilePath()
    {
        if (!isDir) {
            int lastDotPos = filename.lastIndexOf(QChar('.'));
            if (lastDotPos != - 1) {
                fileExtension = filename.mid(lastDotPos + 1);
            }
        }
        
        if (parent) {
            
            // absoluteFilePath method of QFileInfo class cause the file system
            // query hence causes slower performance
            if ( !parent->parent() ) {
                // for drives, there is no filename
                absoluteFilePath = filename;
            } else {
                absoluteFilePath = generateChildAbsoluteName(parent,filename);
            }
            
        }
    }
   
};

FileSystemItem::FileSystemItem(bool isDir,
                               const QString& filename,
                               const QString& userFriendlySequenceName,
                               const boost::shared_ptr<SequenceParsing::SequenceFromFiles>& sequence,
                               const QDateTime& dateModified,
                               quint64 size,
                               FileSystemItem* parent)
: _imp(new FileSystemItemPrivate(isDir,filename,userFriendlySequenceName, sequence,dateModified,size,parent))
{
    
}

FileSystemItem::~FileSystemItem()
{
}

boost::shared_ptr<FileSystemItem>
FileSystemItem::childAt(int position) const
{
    QMutexLocker l(&_imp->childrenMutex);
    return _imp->children[position];
}

int
FileSystemItem::childCount() const
{
    QMutexLocker l(&_imp->childrenMutex);
    return (int)_imp->children.size();
}

int
FileSystemItem::indexInParent() const
{
    if (_imp->parent) {
        for (U32 i = 0; i < _imp->parent->_imp->children.size(); ++i) {
            if (_imp->parent->_imp->children[i].get() == this) {
                return i;
            }
        }
    }
    return -1;
}

bool
FileSystemItem::isDir() const
{
    return _imp->isDir;
}

FileSystemItem*
FileSystemItem::parent() const
{
    return _imp->parent;
}


const QString&
FileSystemItem::absoluteFilePath() const
{
    return _imp->absoluteFilePath;
}

const QString&
FileSystemItem::fileName() const
{
    return _imp->filename;
}

const QString&
FileSystemItem::getUserFriendlyFilename() const
{
    return _imp->userFriendlySequenceName;
}

boost::shared_ptr<SequenceParsing::SequenceFromFiles>
FileSystemItem::getSequence() const
{
    return _imp->sequence;
}

const QString&
FileSystemItem::fileExtension() const
{
    return _imp->fileExtension;
}

const QDateTime&
FileSystemItem::getLastModified() const
{
    return _imp->dateModified;
}

quint64
FileSystemItem::getSize() const
{
    return _imp->size;
}

void
FileSystemItem::addChild(const boost::shared_ptr<FileSystemItem>& child)
{
    QMutexLocker l(&_imp->childrenMutex);
    _imp->children.push_back(child);
    
}

void
FileSystemItem::addChild(const boost::shared_ptr<SequenceParsing::SequenceFromFiles>& sequence,
              const QFileInfo& info)
{
    QMutexLocker l(&_imp->childrenMutex);
    ///Does the child exist already ?
    
    QString filename;
    QString userFriendlyFilename;
    if (!sequence) {
        filename = info.fileName();
        userFriendlyFilename = filename;
    } else {
        std::string pattern = sequence->generateValidSequencePattern();
        SequenceParsing::removePath(pattern);
        filename = pattern.c_str();
        if (!sequence->isSingleFile()) {
            pattern = sequence->generateUserFriendlySequencePatternFromValidPattern(pattern);
        }
        userFriendlyFilename = pattern.c_str();
    }
    
    
    for (std::vector<boost::shared_ptr<FileSystemItem> >::iterator it = _imp->children.begin(); it!=_imp->children.end();++it) {
        if ((*it)->fileName() == filename) {
            _imp->children.erase(it);
            break;
        }
    }
    
    
    bool isDir = sequence ? false : info.isDir();
    qint64 size;
    if (sequence) {
        size = sequence->getEstimatedTotalSize();
    } else {
        size = isDir ? 0 : info.size();
    }
    
    
    ///Create the child
    boost::shared_ptr<FileSystemItem> child( new FileSystemItem(isDir,
                                                                filename,
                                                                userFriendlyFilename,
                                                                sequence,
                                                                info.lastModified(),
                                                                size,
                                                                this) );
    _imp->children.push_back(child);
    
}

void
FileSystemItem::clearChildren()
{
    QMutexLocker l(&_imp->childrenMutex);
    _imp->children.clear();
}

// This is a recursive method which tries to match a path to a specifiq
// FileSystemItem item which has the path;
// Here startIndex is the position of the separator
boost::shared_ptr<FileSystemItem>
FileSystemItem::matchPath(const QStringList& path, int startIndex) const
{
    const QString& pathBit = path.at(startIndex);

    for (U32 i = 0; i < _imp->children.size(); ++i) {
        
        const boost::shared_ptr<FileSystemItem>& child = _imp->children[i];
        
        if ( child->fileName() == pathBit ) {
            
            if ( startIndex == path.size() -1 ) {
                return child;
            } else {
                return child->matchPath(path,startIndex + 1);
            }
        }
    }
    return boost::shared_ptr<FileSystemItem>();
}

//////////////FileSystemModel

struct FileSystemModelPrivate
{
    SortableViewI* view;
    
    ///A background thread that fetches info about the file-system and reports when done
    FileGathererThread gatherer;
    
    ///The watcher watches the current root path
    QFileSystemWatcher* watcher;
    
    ///The root of the file-system: "MyComputer" for Windows or '/' for Unix
    boost::shared_ptr<FileSystemItem> rootItem;
    
    QString currentRootPath;
    bool rootPathWatched;
    QStringList headers;
    
    QDir::Filters filters;
    
    QString encodedRegexps;
    std::list<QRegExp> regexps;
    mutable QMutex filtersMutex;

    mutable QMutex sequenceModeEnabledMutex;
    bool sequenceModeEnabled;
    
    Qt::SortOrder ordering;
    int sortSection;
    mutable QMutex sortMutex;
    
    FileSystemModelPrivate(FileSystemModel* model,SortableViewI* view)
    : view(view)
    , gatherer(model)
    , watcher(0)
    , rootItem()
    , currentRootPath()
    , rootPathWatched(false)
    , headers()
    , filters(QDir::AllEntries | QDir::NoDotAndDotDot)
    , encodedRegexps()
    , regexps()
    , filtersMutex()
    , sequenceModeEnabledMutex()
    , sequenceModeEnabled(false)
    , ordering(view->sortIndicatorOrder())
    , sortSection(view->sortIndicatorSection())
    , sortMutex()
    {
        assert(view);
    }
    
    
    boost::shared_ptr<FileSystemItem> getItemFromPath(const QString &path) const;
    
    void populateItem(const boost::shared_ptr<FileSystemItem>& item);
    
    FileSystemItem *getItem(const QModelIndex &index) const;
    
    boost::shared_ptr<FileSystemItem> mkPath(const QString& path);
};


FileSystemModel::FileSystemModel(SortableViewI* view)
: QAbstractItemModel()
, _imp(new FileSystemModelPrivate(this,view))
{
    QObject::connect(&_imp->gatherer, SIGNAL(directoryLoaded(QString)), this, SLOT(onDirectoryLoadedByGatherer(QString)));
    
    
    _imp->headers << tr("Name") << tr("Size") << tr("Type") << tr("Date Modified");
    
    _imp->rootItem.reset(new FileSystemItem(true,QString(),QString(),boost::shared_ptr<SequenceParsing::SequenceFromFiles>(),QDateTime(),0));
    
    _imp->watcher = new QFileSystemWatcher;
    QObject::connect(_imp->watcher, SIGNAL(directoryChanged(QString)), this, SLOT(onWatchedDirectoryChanged(QString)));
    QObject::connect(_imp->watcher, SIGNAL(fileChanged(QString)), this, SLOT(onWatchedFileChanged(QString)));
    _imp->watcher->addPath(QDir::rootPath());
    
    QFileInfoList drives = QDir::drives();
    
    ///Fetch all drives by default
    for (int i = 0; i < drives.size(); ++i) {
      
        QString driveName;
#ifdef __NATRON_WIN32__
        // for drives, there is no filename
        driveName = drives[i].canonicalPath();
#else
        driveName = generateChildAbsoluteName(_imp->rootItem.get(),drives[i].fileName());
#endif
        
        boost::shared_ptr<FileSystemItem> child(new FileSystemItem(true, //isDir
                                                                   driveName, //drives have canonical path
                                                                   driveName,
                                                                   boost::shared_ptr<SequenceParsing::SequenceFromFiles>(),
                                                                   drives[i].lastModified(),
                                                                   drives[i].size(),
                                                                   _imp->rootItem.get()));
        _imp->rootItem->addChild(child);
    }
    
    
}

///////////////////////////////////////Overriden from QAbstractItemModel///////////////////////////////////////

FileSystemModel::~FileSystemModel()
{
    _imp->gatherer.quitGatherer();
    delete _imp->watcher;
}


bool
FileSystemModel::isDriveName(const QString& name)
{
#ifdef __NATRON_WIN32__
	return name.size() == 3 && name.at(0).isLetter() && name.at(1) == QLatin1Char(':') && (name.at(2) == QLatin1Char('/') || name.at(2) == QLatin1Char('\\'));
#else
	return name == "/";
#endif
}

bool
FileSystemModel::startsWithDriveName(const QString& name)
{
#ifdef __NATRON_WIN32__
    return name.size() >= 3 && name.at(0).isLetter() && name.at(1) == QLatin1Char(':') && (name.at(2) == QLatin1Char('/') || name.at(2) == QLatin1Char('\\'));
#else
    return name.startsWith("/");
#endif
}

QVariant
FileSystemModel::headerData(int section, Qt::Orientation orientation,int role) const
{
    if (orientation == Qt::Horizontal) {
        switch(role) {
                // in case of DisplayRole, just return the header text
            case Qt::DisplayRole:
                return _imp->headers[section];
                break;
                // in case of TextAlignmentRole, only Size column will be right align,
                // others will be left align
            case Qt::TextAlignmentRole:
                return  (Sections)section == Size ? Qt::AlignRight : Qt::AlignLeft;
                break;
        }
    }
    
    return QVariant();
}

Qt::ItemFlags
FileSystemModel::flags(const QModelIndex &index) const
{
    if ( !index.isValid() )
        return 0;
    
    // Our model is read only.
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

int
FileSystemModel::columnCount(const QModelIndex & /*parent*/) const
{
    return (int)EndSections;
}

int
FileSystemModel::rowCount(const QModelIndex & parent) const
{
    FileSystemItem *item = _imp->getItem(parent);
    return item->childCount();
}

QVariant
FileSystemModel::data(const QModelIndex &index, int role) const
{
    if ( !index.isValid() ) {
        return QVariant();
    }
    
    Sections col = (Sections)index.column();

    
    // in case of TextAlignmentRole, only Size column will be right align,
    // others will be left align
    if( role == Qt::TextAlignmentRole && col == Size ){
        return Qt::AlignRight;
    }
    

    FileSystemItem *item = _imp->getItem(index);
    
    if (!item) {
        return QVariant();
    }
    
    if (role == FilePathRole && col == Name) {
        return item->absoluteFilePath();
    } else if (role == FileNameRole && col == Name) {
        return item->fileName();
    }
    
    if (role == Qt::DecorationRole && col == Name ) {
        return item->fileName();
    }
    
    QVariant data;
    
    
    switch (col) {
        case Name:
            data = item->fileName();
            break;
        case Size:
            data = item->getSize();
            break;
        case Type:
            data = item->fileExtension();
            break;
        case DateModified:
            data = item->getLastModified().toString(Qt::LocalDate);
            break;  
        default:
            break;
    }
    
    return data;
}

QModelIndex
FileSystemModel::index(int row, int column, const QModelIndex &parent) const
{
    // As the Name column is a tree, we will only create index whose parent is in the Name column
    if (parent.isValid() && (Sections)parent.column() != Name)
        return QModelIndex();
    
    FileSystemItem *parentItem = _imp->getItem(parent);
    

    if (parentItem) {
        
        boost::shared_ptr<FileSystemItem> childItem = parentItem->childAt(row);
        
        if (childItem) {
            return createIndex(row, column, childItem.get());
        }
    }
    
    return QModelIndex();
}

QModelIndex
FileSystemModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }
    
    FileSystemItem* childItem = _imp->getItem(index);
    
    if (!childItem) {
        return QModelIndex();
    }
    
    FileSystemItem *parentItem = childItem->parent();
    
    // if there is no parent or parent is invisible, there is no index
    if (!parentItem || parentItem == _imp->rootItem.get()) {
        return QModelIndex();
    }
    
    return createIndex(parentItem->indexInParent(), (int)Name, parentItem);
}

QStringList
FileSystemModel::mimeTypes() const
{
    return QStringList(QLatin1String("text/uri-list"));
}

QMimeData*
FileSystemModel::mimeData(const QModelIndexList & indexes) const
{
    QList<QUrl> urls;
    for (QList<QModelIndex>::const_iterator it = indexes.begin(); it != indexes.end(); ++it) {
        
        if ((*it).column() == 0) {
            urls << QUrl::fromLocalFile(absolutePath(*it));
        }
    }
    QMimeData *data = new QMimeData();
    data->setUrls(urls);
    return data;
}

/////////////////////////////////////// End overrides ////////////////////////////////////////////////////

QModelIndex
FileSystemModel::index(const QString& path, int column) const
{
    boost::shared_ptr<FileSystemItem> item = getFileSystemItem(path);
    if (item) {
        return index(item.get(),column);
    }
    return QModelIndex();
}

boost::shared_ptr<FileSystemItem>
FileSystemModel::getFileSystemItem(const QString& path) const
{
    if ( !path.isEmpty() ) {
        return _imp->getItemFromPath(path);
    }
    return _imp->rootItem;
}

FileSystemItem*
FileSystemModel::getFileSystemItem(const QModelIndex& index) const
{
    return static_cast<FileSystemItem*>( index.internalPointer() );
}

QModelIndex
FileSystemModel::index(FileSystemItem* item,int column) const
{
    assert(item);
    return createIndex(item->indexInParent(), column, item);
}

bool
FileSystemModel::isDir(const QModelIndex &index)
{
    if ( !index.isValid() ) {
        return false;
    }
    FileSystemItem *item = getFileSystemItem(index);
    
    if (item && item != _imp->rootItem.get()) {
        return item->isDir();
    }
    
    return false;
}

QString
FileSystemModel::absolutePath(const QModelIndex &index) const
{
    if ( !index.isValid() ) {
        return QString();
    }
    FileSystemItem *item = getFileSystemItem(index);
    
    if (item && item != _imp->rootItem.get()) {
        return item->absoluteFilePath();
    }
    
    return QString();
}

QString
FileSystemModel::rootPath() const
{
    return _imp->currentRootPath;
}

QVariant
FileSystemModel::myComputer(int /*role*/) const
{
//    switch (role) {
//        case Qt::DisplayRole:
#ifdef Q_OS_WIN
            return tr("Computer");
#else
            return tr("Computer");
#endif
//        case Qt::DecorationRole:
//            return d->fileInfoGatherer.iconProvider()->icon(QFileIconProvider::Computer);
//    }
//    return QVariant();

}

void
FileSystemModel::setFilter(const QDir::Filters& filters)
{
    {
        QMutexLocker l(&_imp->filtersMutex);
        _imp->filters = filters;
    }
    
    ///Refresh the current directory
    
    ///Get the item corresponding to the current directory
    QFileInfo info(_imp->currentRootPath);
    
    boost::shared_ptr<FileSystemItem> parent = _imp->getItemFromPath( info.absolutePath() );
    assert(parent);
    cleanAndRefreshItem(parent);
}

const QDir::Filters
FileSystemModel::filter() const
{
    QMutexLocker l(&_imp->filtersMutex);
    return _imp->filters;
}

void
FileSystemModel::setRegexpFilters(const QString& filters)
{
    {
        QMutexLocker l(&_imp->filtersMutex);
        
        _imp->encodedRegexps = filters;
        _imp->regexps.clear();
        int i = 0;
        while ( i < filters.size() ) {
            QString regExp;
            while ( i < filters.size() && filters.at(i) != QChar(' ') ) {
                regExp.append( filters.at(i) );
                ++i;
            }
            ++i;
            QRegExp rx(regExp,Qt::CaseInsensitive,QRegExp::Wildcard);
            if ( rx.isValid() ) {
                _imp->regexps.push_back(rx);
            }
        }
    }
    resetCompletly();
}

QString
FileSystemModel::generateRegexpFilterFromFileExtensions(const QStringList& extensions)
{
    QString ret;
    
    for (int i = 0; i < extensions.size(); ++i) {
        if (extensions[i] != "*") {
            ret.append("*.");
        }
        ret.append( extensions[i] );
        if (i < extensions.size() - 1) {
            ret.append(" ");
        }
    }
    return ret;
}

bool
FileSystemModel::isAcceptedByRegexps(const QString & path) const
{
    
    QMutexLocker l(&_imp->filtersMutex);
    
    if ( _imp->encodedRegexps.isEmpty() ) {
        return true;
    }
    
    for (std::list<QRegExp>::const_iterator it = _imp->regexps.begin(); it != _imp->regexps.end(); ++it) {
        if ( it->exactMatch(path) ) {
            return true;
        }
    }
    
    return false;
}

void
FileSystemModel::setSequenceModeEnabled(bool sequenceMode)
{
    {
        QMutexLocker l(&_imp->sequenceModeEnabledMutex);
        _imp->sequenceModeEnabled = sequenceMode;
    }
    
    resetCompletly();
    
    ///Now re-populate the current root path
    setRootPath(_imp->currentRootPath);
}

void
FileSystemModel::resetCompletly()
{
    beginResetModel();
    
    ///Wipe all the file-system loaded by clearing all children of drives
    for (int i = 0; i < _imp->rootItem->childCount(); ++i) {
        _imp->rootItem->childAt(i)->clearChildren();
    }
    
    endResetModel();
    
}


bool
FileSystemModel::isSequenceModeEnabled() const
{
    QMutexLocker l(&_imp->sequenceModeEnabledMutex);
    return _imp->sequenceModeEnabled;
}

static boost::shared_ptr<FileSystemItem> mkPathInternal(FileSystemItem* item,const QStringList& path,int index)
{
    assert(index < path.size() && index >= 0);
    
    boost::shared_ptr<FileSystemItem> child;
    for (int i = 0; i < item->childCount(); ++i) {
        boost::shared_ptr<FileSystemItem> c = item->childAt(i);
        if ( c->fileName() == path[index] ) {
            child = c;
            break;
        }
    }

    if (!child) {
        
        QFileInfo info(generateChildAbsoluteName(item, path[index]));
        ///The child doesn't exist already, create it without populating it
        child.reset(new FileSystemItem(true, //isDir
                                       path[index], //name
                                       path[index], //name
                                       boost::shared_ptr<SequenceParsing::SequenceFromFiles>(),
                                       info.lastModified(),
                                       0, //0 for directories
                                       item));
        item->addChild(child);
    }
    if (index == path.size() - 1) {
        return child;
    } else {
        return mkPathInternal(child.get(), path, index + 1);
    }
}

boost::shared_ptr<FileSystemItem>
FileSystemModelPrivate::mkPath(const QString& path)
{
	
    if (path.isEmpty()) {
        return rootItem;
    }
    
	QStringList splitPath = getSplitPath(path);
    return mkPathInternal(rootItem.get(), splitPath,0);
}

QModelIndex
FileSystemModel::mkdir(const QModelIndex& parent,const QString& name)
{
    if (!parent.isValid()) {
        return QModelIndex();
    }
    
    FileSystemItem *item = getFileSystemItem(parent);
    
    if (item) {
        QDir dir(item->absoluteFilePath());
        dir.mkpath(name);
        boost::shared_ptr<FileSystemItem> newDir = _imp->mkPath(generateChildAbsoluteName(item,name));
        return createIndex(newDir->indexInParent(), 0, item);
    }
    return QModelIndex();
}

Qt::SortOrder
FileSystemModel::sortIndicatorOrder() const
{
    QMutexLocker l(&_imp->sortMutex);
    return _imp->ordering;
}

int
FileSystemModel::sortIndicatorSection() const
{
    QMutexLocker l(&_imp->sortMutex);
    return _imp->sortSection;
}

void
FileSystemModel::onSortIndicatorChanged(int logicalIndex,Qt::SortOrder order)
{
    {
        QMutexLocker l(&_imp->sortMutex);
        _imp->sortSection = logicalIndex;
        _imp->ordering = order;
    }
    boost::shared_ptr<FileSystemItem> item = _imp->getItemFromPath(_imp->currentRootPath);
    if (item) {
        cleanAndRefreshItem(item);
    }
}

boost::shared_ptr<FileSystemItem>
FileSystemModelPrivate::getItemFromPath(const QString &path) const
{
    if (path.isEmpty()) {
        return rootItem;
    }
	QStringList splitPath = getSplitPath(path);

    return rootItem->matchPath( splitPath, 0 );
}

void
FileSystemModel::setRootPath(const QString& path)
{
    assert(QThread::currentThread() == qApp->thread());
    
    ///Check if the path exists
    {
        QDir dir(path);
        if ( !dir.exists() ) {
            return;
        }
    }
    
    _imp->currentRootPath = path;
    
    ///Set it to false so that onDirectoryLoadedByGatherer will watch the content of the directory
    _imp->rootPathWatched = false;
    
    ///Make sure the path exist
    boost::shared_ptr<FileSystemItem> item = _imp->mkPath(path);
    
    if (item && item != _imp->rootItem) {
        
        delete _imp->watcher;
        _imp->watcher = new QFileSystemWatcher;
        QObject::connect(_imp->watcher, SIGNAL(directoryChanged(QString)), this, SLOT(onWatchedDirectoryChanged(QString)));
        QObject::connect(_imp->watcher, SIGNAL(fileChanged(QString)), this, SLOT(onWatchedFileChanged(QString)));
        _imp->watcher->removePath(_imp->currentRootPath);
        _imp->watcher->addPath(item->absoluteFilePath());

        ///Since we are about to kill some FileSystemItem's we must force a reset of the QAbstractItemModel to clear the persistent
        ///QModelIndex left in the model that may hold raw pointers to bad FileSystemItem's
        beginResetModel();
        endResetModel();
        
        _imp->populateItem(item);
    } else {
        Q_EMIT directoryLoaded(path);
    }
    
    Q_EMIT rootPathChanged(path);
}


void
FileSystemModelPrivate::populateItem(const boost::shared_ptr<FileSystemItem> &item)
{
    ///We do it in a separate thread because it might be expensive,
    ///the directoryLoaded signal will be emitted when it is finished
    gatherer.fetchDirectory(item);
}

void
FileSystemModel::onDirectoryLoadedByGatherer(const QString& directory)
{
    
    ///Get the item corresponding to the directory
    boost::shared_ptr<FileSystemItem> item = _imp->getItemFromPath(directory);
    if (!item) {
        qDebug() << "FileSystemModel failed to load the following requested directory: " << directory;
        return;
    }
    
    if (directory != _imp->currentRootPath) {
        return;
    }
    
    if (!_imp->rootPathWatched) {
        assert(_imp->watcher);
        
        ///Watch all files in the directory and track changes
        for (int i = 0; i < item->childCount(); ++i) {
            boost::shared_ptr<FileSystemItem> child = item->childAt(i);
            boost::shared_ptr<SequenceParsing::SequenceFromFiles> sequence = child->getSequence();
            
            if (sequence) {
                ///Add all items in the sequence
                if (sequence->isSingleFile()) {
                    _imp->watcher->addPath(sequence->generateValidSequencePattern().c_str());
                } else {
                    const std::map<int,SequenceParsing::FileNameContent>& indexes = sequence->getFrameIndexes();
                    for (std::map<int,SequenceParsing::FileNameContent>::const_iterator it = indexes.begin();
                         it != indexes.end(); ++it) {
                        _imp->watcher->addPath(it->second.absoluteFileName().c_str());
                    }
                }
    
            } else {
                const QString& absolutePath = child->absoluteFilePath();
                _imp->watcher->addPath(absolutePath);
            }
            
        }
        
        ///Set it to true to prevent it from being re-watched
        _imp->rootPathWatched = true;
    }
    
    ///Finally notify the client that the directory is ready for use
    Q_EMIT directoryLoaded(directory);
}

void
FileSystemModel::onWatchedDirectoryChanged(const QString& directory)
{
    boost::shared_ptr<FileSystemItem> item = _imp->getItemFromPath(directory);
    if (item) {
        if (directory == _imp->currentRootPath) {
            cleanAndRefreshItem(item);
        } else {
            ///This is a sub-directory
            ///Clear the parent of the corresponding item
            if (item->parent()) {
                
                QModelIndex idx = index(item.get());
				assert(idx.isValid());
				int count = item->parent()->childCount();
				if (count > 0) {
					beginRemoveRows(idx.parent(), 0, count - 1);
					item->parent()->clearChildren();
					endRemoveRows();
				}
            }
        }
    }
    
    QDir dir(_imp->currentRootPath);
    if (!dir.exists()) {
        ///The current directory has changed its name or was deleted.. just fallback the filesystem to the root-path
        setRootPath(QDir::rootPath());
    } else {
        setRootPath(_imp->currentRootPath);
    }
}

void
FileSystemModel::onWatchedFileChanged(const QString& file)
{
    ///Get the item corresponding to the current directory
    QFileInfo info(file);
    
    boost::shared_ptr<FileSystemItem> parent = _imp->getItemFromPath( info.absolutePath() );
    if (parent) {
        cleanAndRefreshItem(parent);
    }
}

void
FileSystemModel::cleanAndRefreshItem(const boost::shared_ptr<FileSystemItem>& item)
{
    QModelIndex idx = index(item.get(),0);
    if (idx.isValid()) {
        int count = item->childCount();
        if (count > 0) {
            beginRemoveRows(idx, 0, count - 1);
            item->clearChildren();
            endRemoveRows();
        }
        
        _imp->populateItem(item);
    }
}

FileSystemItem *
FileSystemModelPrivate::getItem(const QModelIndex &index) const
{
    // just return the internal pointer we set at creating index if the index is valid
    if (index.isValid() ) {
        FileSystemItem *item = static_cast<FileSystemItem*>(index.internalPointer());
        
        if (item) {
            return item;
        }
    }
    
    return rootItem.get();
}

///////////////////////// FileGathererThread

struct FileGathererThreadPrivate
{
    FileSystemModel* model;
    
    bool mustQuit;
    mutable QMutex mustQuitMutex;
    QWaitCondition mustQuitCond;
    
    bool working;
    mutable QMutex workingMutex;
    
    int abortRequests;
    mutable QMutex abortRequestsMutex;
    QWaitCondition abortRequestsCond;
    
    int startCount;
    mutable QMutex startCountMutex;
    QWaitCondition startCountCond;
    
    boost::shared_ptr<FileSystemItem> requestedItem,itemBeingFetched;
    QMutex requestedDirMutex;
    
    FileGathererThreadPrivate(FileSystemModel* model)
    : model(model)
    , mustQuit(false)
    , mustQuitMutex()
    , mustQuitCond()
    , working(false)
    , workingMutex()
    , abortRequests(0)
    , abortRequestsMutex()
    , abortRequestsCond()
    , startCount(0)
    , startCountMutex()
    , startCountCond()
    , requestedItem()
    , itemBeingFetched()
    , requestedDirMutex()
    {
        
    }
    
    bool checkForExit()
    {
        QMutexLocker l(&mustQuitMutex);
        if (mustQuit) {
            mustQuit = false;
            mustQuitCond.wakeOne();
            return true;
        }
        return false;
    }
    
    
    bool checkForAbort()
    {
        
        QMutexLocker k(&abortRequestsMutex);
        if (abortRequests > 0) {
            abortRequests = 0;
            abortRequestsCond.wakeOne();
            return true;
        }
        return false;
        
    }
};

FileGathererThread::FileGathererThread(FileSystemModel* model)
: QThread()
, _imp(new FileGathererThreadPrivate(model))
{
    
}

FileGathererThread::~FileGathererThread()
{
    quitGatherer();
}

bool
FileGathererThread::isWorking() const
{
    QMutexLocker k(&_imp->workingMutex);
    return _imp->working;
}

void
FileGathererThread::abortGathering()
{
    if ( !isRunning() || !isWorking() ) {
        return;
    }
    
    QMutexLocker k(&_imp->abortRequestsMutex);
    ++_imp->abortRequests;
    while (_imp->abortRequests > 0) {
        _imp->abortRequestsCond.wait(&_imp->abortRequestsMutex);
    }
}

void
FileGathererThread::quitGatherer()
{
    if (!isRunning()) {
        return;
    }
    abortGathering();
    {
        QMutexLocker k(&_imp->mustQuitMutex);
        _imp->mustQuit = true;
    }
    {
        QMutexLocker l(&_imp->startCountMutex);
        ++_imp->startCount;
        _imp->startCountCond.wakeOne();
    }
    {
        QMutexLocker k(&_imp->mustQuitMutex);
        while (_imp->mustQuit) {
            _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
        }
    }
}

struct WorkingSetter
{
    FileGathererThreadPrivate* _imp;
    
    WorkingSetter(FileGathererThreadPrivate* imp)
    : _imp(imp)
    {
        QMutexLocker k(&imp->workingMutex);
        imp->working = true;
    }
    
    ~WorkingSetter() {
        QMutexLocker k(&_imp->workingMutex);
        _imp->working = false;
    }
};


void
FileGathererThread::run()
{
    for (;;) {
        
        {
            WorkingSetter working( _imp.get() );
            
            if ( _imp->checkForExit() ) {
                return;
            }
            
            {
                QMutexLocker k(&_imp->requestedDirMutex);
                _imp->itemBeingFetched = _imp->requestedItem;
            }
            
            ///Doesn't need to be protected under requestedDirMutex since it is written to only by this thread
            gatheringKernel(_imp->itemBeingFetched);
            _imp->itemBeingFetched.reset();
            
        } //WorkingSetter
        
        _imp->checkForAbort();
        
        {
            QMutexLocker l(&_imp->startCountMutex);
            while (_imp->startCount <= 0) {
                _imp->startCountCond.wait(&_imp->startCountMutex);
            }
            _imp->startCount = 0;
        }
        
    }
}


static bool isVideoFileExtension(const std::string& ext)
{
    if (ext == "mov" ||
        ext == "avi" ||
        ext == "mp4" ||
        ext == "mpeg" ||
        ext == "flv" ||
        ext == "mkv") {
        return true;
    }
    return false;
}

typedef std::list< std::pair< boost::shared_ptr<SequenceParsing::SequenceFromFiles> ,QFileInfo > > FileSequences;

#define KERNEL_INCR() \
    switch (viewOrder) \
    { \
        case Qt::AscendingOrder: \
            ++i; \
            break; \
        case Qt::DescendingOrder: \
            --i; \
            break; \
    }

void
FileGathererThread::gatheringKernel(const boost::shared_ptr<FileSystemItem>& item)
{

    QDir dir( item->absoluteFilePath() );
    
    Qt::SortOrder viewOrder = _imp->model->sortIndicatorOrder();
    FileSystemModel::Sections sortSection = (FileSystemModel::Sections)_imp->model->sortIndicatorSection();
    QDir::SortFlags sort;
    switch (sortSection) {
        case FileSystemModel::Name:
            sort = QDir::Name;
            break;
        case FileSystemModel::Size:
            sort = QDir::Size;
            break;
        case FileSystemModel::Type:
            sort = QDir::Type;
            break;
        case FileSystemModel::DateModified:
            sort = QDir::Time;
            break;
        default:
            break;
    }
    sort |= QDir::IgnoreCase;
    sort |= QDir::DirsFirst;
    
    ///All entries in the directory
    QFileInfoList all = dir.entryInfoList(_imp->model->filter(), sort);
    
    ///List of all possible file sequences in the directory or directories
    FileSequences sequences;
    
    int start = 0;
    int end = 0;
    switch (viewOrder) {
        case Qt::AscendingOrder:
            start = 0;
            end = all.size();
            break;
        case Qt::DescendingOrder:
            start = all.size() - 1;
            end = -1;
            break;
    }
    
    int i = start;
    while (i != end) {
        
        ///If we must abort we do it now
        if ( _imp->checkForAbort() ) {
            return;
        }
                
        if ( all[i].isDir() ) {
            ///This is a directory
            sequences.push_back(std::make_pair(boost::shared_ptr<SequenceParsing::SequenceFromFiles>(), all[i]));
        } else {
            

            QString filename = all[i].fileName();

            /// If the item does not match the filter regexp set by the user, discard it
            if ( !_imp->model->isAcceptedByRegexps(filename) ) {
                KERNEL_INCR();
                continue;
            }
            
            /// If file sequence fetching is disabled, accept it
            if ( !_imp->model->isSequenceModeEnabled() ) {
                sequences.push_back(std::make_pair(boost::shared_ptr<SequenceParsing::SequenceFromFiles>(), all[i]));
                KERNEL_INCR();
                continue;
            }

            std::string absoluteFilePath = generateChildAbsoluteName(item.get(), filename).toStdString();
            
            
            bool foundMatchingSequence = false;
            
            /// If we reach here, this is a valid file and we need to determine if it belongs to another sequence or we need
            /// to create a new one
            SequenceParsing::FileNameContent fileContent(absoluteFilePath);
            
            if (!isVideoFileExtension(fileContent.getExtension())) {
                ///Note that we use a reverse iterator because we have more chance to find a match in the last recently added entries
                for (FileSequences::reverse_iterator it = sequences.rbegin(); it != sequences.rend(); ++it) {
                    
                    if ( it->first && it->first->tryInsertFile(fileContent,false) ) {
                        
                        foundMatchingSequence = true;
                        break;
                    }
                    
                }
            }
            
            if (!foundMatchingSequence) {
                boost::shared_ptr<SequenceParsing::SequenceFromFiles> newSequence( new SequenceParsing::SequenceFromFiles(fileContent,true) );
                sequences.push_back(std::make_pair(newSequence, all[i]));

            }
            
        }
        KERNEL_INCR();
    }
    
    ///Now iterate through the sequences and create the children as necessary
    for (FileSequences::iterator it = sequences.begin(); it != sequences.end(); ++it) {
        item->addChild(it->first, it->second);
    }
    
    Q_EMIT directoryLoaded( item->absoluteFilePath() );
}

void
FileGathererThread::fetchDirectory(const boost::shared_ptr<FileSystemItem>& item)
{
    abortGathering();
    {
        QMutexLocker l(&_imp->requestedDirMutex);
        _imp->requestedItem = item;
    }
    
    if ( isRunning() ) {
        QMutexLocker k(&_imp->startCountMutex);
        ++_imp->startCount;
        _imp->startCountCond.wakeOne();
    } else {
        start();
    }
}
