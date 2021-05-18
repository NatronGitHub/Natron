/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#include "FileSystemModel.h"

#include <vector>
#include <cassert>
#include <stdexcept>

#include <boost/make_shared.hpp>

#ifdef __NATRON_WIN32__
#include <windows.h>
#endif

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QtGlobal> // for Q_OS_*
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

#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif

NATRON_NAMESPACE_ENTER

static QStringList
getSplitPath(const QString& path)
{
    if ( path.isEmpty() ) {
        return QStringList();
    }
    QString pathCpy = path;
    bool isdriveOrRoot;
#ifdef __NATRON_WIN32__
    QString startPath = pathCpy.mid(0, 3);
    isdriveOrRoot = FileSystemModel::isDriveName(startPath);
    if (isdriveOrRoot) {
        pathCpy = pathCpy.remove(0, 3);
    }
    if ( (pathCpy.size() > 0) && ( pathCpy[pathCpy.size() - 1] == QChar::fromLatin1('/') ) ) {
        pathCpy = pathCpy.mid(0, pathCpy.size() - 1);
    }
    QStringList splitPath = pathCpy.split( QChar::fromLatin1('/') );
    if (isdriveOrRoot) {
        splitPath.prepend( startPath.mid(0, 3) );
    }

#else
    isdriveOrRoot = pathCpy.startsWith( QChar::fromLatin1('/') );
    if (isdriveOrRoot) {
        pathCpy = pathCpy.remove(0, 1);
    }
    if ( (pathCpy.size() > 0) && ( pathCpy[pathCpy.size() - 1] == QChar::fromLatin1('/') ) ) {
        pathCpy = pathCpy.mid(0, pathCpy.size() - 1);
    }
    QStringList splitPath = pathCpy.split( QChar::fromLatin1('/') );
    if (isdriveOrRoot) {
        splitPath.prepend( QChar::fromLatin1('/') );
    }

#endif

    return splitPath;
}

struct FileSystemModelPrivate
{
    FileSystemModel* _publicInterface;
    SortableViewI* view;

    ///A background thread that fetches info about the file-system and reports when done
    boost::scoped_ptr<FileGathererThread> gatherer;

    ///The watcher watches the current root path
    boost::scoped_ptr<QFileSystemWatcher> watcher;

    ///The root of the file-system: "MyComputer" for Windows or '/' for Unix
    FileSystemItemPtr rootItem;
    QString currentRootPath;
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
    mutable QMutex mappingMutex;
    std::map<FileSystemItem*, FileSystemItemWPtr> itemsMap;


    FileSystemModelPrivate(FileSystemModel* model)
        : _publicInterface(model)
        , view(0)
        , gatherer()
        , watcher()
        , rootItem()
        , currentRootPath()
        , headers()
        , filters(QDir::AllEntries | QDir::NoDotAndDotDot)
        , encodedRegexps()
        , regexps()
        , filtersMutex()
        , sequenceModeEnabledMutex()
        , sequenceModeEnabled(false)
        , ordering(Qt::AscendingOrder)
        , sortSection(0)
        , sortMutex()
        , mappingMutex()
        , itemsMap()
    {
    }

    void registerItem(const FileSystemItemPtr& item);
    void unregisterItem(FileSystemItem* item);


    FileSystemItemPtr getItemFromPath(const QString &path) const;

    void populateItem(const FileSystemItemPtr& item);
};

/////////FileSystemItem
static QString
generateChildAbsoluteName(FileSystemItem* parent,
                          const QString& name)
{
    QString childName = parent->absoluteFilePath();

    if ( !childName.endsWith( QChar::fromLatin1('/') ) ) {
        childName.append( QChar::fromLatin1('/') );
    }
    childName.append(name);

    return childName;
}

struct FileSystemItemPrivate
{
    FileSystemModelWPtr model;
    FileSystemItemWPtr parent;
    std::vector<FileSystemItemPtr> children; ///vector for random access
    QMutex childrenMutex;
    bool isDir;
    QString filename;
    QString userFriendlySequenceName;

    ///This will be set when the file system model is in sequence mode and this is a file
    SequenceParsing::SequenceFromFilesPtr sequence;
    QDateTime dateModified;
    quint64 size;
    QString fileExtension;
    QString absoluteFilePath;

    FileSystemItemPrivate(const FileSystemModelPtr& model,
                          bool isDir,
                          const QString& filename,
                          const QString& userFriendlySequenceName,
                          const SequenceParsing::SequenceFromFilesPtr& sequence,
                          const QDateTime& dateModified,
                          quint64 size,
                          const FileSystemItemPtr &parent)
        : model(model)
        , parent(parent)
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
            int lastDotPos = filename.lastIndexOf( QChar::fromLatin1('.') );
            if (lastDotPos != -1) {
                fileExtension = filename.mid(lastDotPos + 1);
            }
        }

        if (parent) {
            // absoluteFilePath method of QFileInfo class cause the file system
            // query hence causes slower performance
            if ( !parent->getParentItem() ) {
                // for drives, there is no filename
                absoluteFilePath = filename;
            } else {
                absoluteFilePath = generateChildAbsoluteName(parent.get(), filename);
            }
        }
    }

    FileSystemModelPtr getModel() const
    {
        return model.lock();
    }
};

FileSystemItem::FileSystemItem(const FileSystemModelPtr& model,
                               bool isDir,
                               const QString& filename,
                               const QString& userFriendlySequenceName,
                               const SequenceParsing::SequenceFromFilesPtr& sequence,
                               const QDateTime& dateModified,
                               quint64 size,
                               const FileSystemItemPtr& parent)
    : _imp( new FileSystemItemPrivate(model, isDir, filename, userFriendlySequenceName, sequence, dateModified, size, parent) )
{
}


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct FileSystemItem::MakeSharedEnabler: public FileSystemItem
{
    MakeSharedEnabler(const FileSystemModelPtr& model,
                      bool isDir,
                      const QString& filename,
                      const QString& userFriendlySequenceName,
                      const SequenceParsing::SequenceFromFilesPtr& sequence,
                      const QDateTime& dateModified,
                      quint64 size,
                      const FileSystemItemPtr& parent) : FileSystemItem(model, isDir, filename, userFriendlySequenceName, sequence, dateModified, size, parent) {
    }
};


FileSystemItemPtr
FileSystemItem::create( const FileSystemModelPtr& model,
                                         bool isDir,
                                         const QString& filename,
                                         const QString& userFriendlySequenceName,
                                         const SequenceParsing::SequenceFromFilesPtr& sequence,
                                         const QDateTime& dateModified,
                                         quint64 size,
                                         const FileSystemItemPtr& parent)
{
    return boost::make_shared<FileSystemItem::MakeSharedEnabler>(model, isDir, filename, userFriendlySequenceName, sequence, dateModified, size, parent);
}
FileSystemItem::~FileSystemItem()
{
    FileSystemModelPtr model = _imp->model.lock();

    if (model) {
        model->_imp->unregisterItem(this);
    }
}

void
FileSystemItem::resetModelPointer()
{
    _imp->model.reset();
}

FileSystemItemPtr
FileSystemItem::childAt(int position) const
{
    QMutexLocker l(&_imp->childrenMutex);

    if ( (position >= 0) && ( position < (int)_imp->children.size() ) ) {
        return _imp->children[position];
    } else {
        return FileSystemItemPtr();
    }
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
    FileSystemItemPtr parent = _imp->parent.lock();

    if (parent) {
        for (U32 i = 0; i < parent->_imp->children.size(); ++i) {
            if (parent->_imp->children[i].get() == this) {
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

FileSystemItemPtr
FileSystemItem::getParentItem() const
{
    return _imp->parent.lock();
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

SequenceParsing::SequenceFromFilesPtr
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
FileSystemItem::addChild(const FileSystemItemPtr& child)
{
    QMutexLocker l(&_imp->childrenMutex);

    _imp->children.push_back(child);
}

void
FileSystemItem::addChild(const SequenceParsing::SequenceFromFilesPtr& sequence,
                         const QFileInfo& info)
{
    FileSystemModelPtr model = _imp->getModel();

    if (!model) {
        return;
    }
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
        filename = QString::fromUtf8( pattern.c_str() );
        if ( !sequence->isSingleFile() ) {
            pattern = sequence->generateUserFriendlySequencePatternFromValidPattern(pattern);
        }
        userFriendlyFilename = QString::fromUtf8( pattern.c_str() );
    }


    for (std::vector<FileSystemItemPtr>::iterator it = _imp->children.begin(); it != _imp->children.end(); ++it) {
        if ( (*it)->fileName() == filename ) {
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
    FileSystemItemPtr child = boost::make_shared<FileSystemItem::MakeSharedEnabler>( model,
                                                                                                    isDir,
                                                                                                    filename,
                                                                                                    userFriendlyFilename,
                                                                                                    sequence,
                                                                                                    info.lastModified(),
                                                                                                    size,
                                                                                                    shared_from_this() );
    model->_imp->registerItem(child);
    _imp->children.push_back(child);
} // FileSystemItem::addChild

void
FileSystemItem::clearChildren()
{
    QMutexLocker l(&_imp->childrenMutex);

    _imp->children.clear();
}

// This is a recursive method which tries to match a path to a specifiq
// FileSystemItem item which has the path;
// Here startIndex is the position of the separator
FileSystemItemPtr
FileSystemItem::matchPath(const QStringList& path,
                          int startIndex) const
{
    const QString& pathBit = path.at(startIndex);

    for (U32 i = 0; i < _imp->children.size(); ++i) {
        const FileSystemItemPtr& child = _imp->children[i];

        if ( child && (child->fileName() == pathBit) ) {
            if (startIndex == path.size() - 1) {
                return child;
            } else {
                return child->matchPath(path, startIndex + 1);
            }
        }
    }

    return FileSystemItemPtr();
}

//////////////FileSystemModel


FileSystemModel::FileSystemModel()
    : QAbstractItemModel()
    , _imp( new FileSystemModelPrivate(this) )
{
}

void
FileSystemModel::initialize(SortableViewI* view)
{
    assert(view);
    assert( _imp->headers.isEmpty() );
    _imp->headers << tr("Name") << tr("Size") << tr("Type") << tr("Date Modified");

    _imp->ordering = view->sortIndicatorOrder();
    _imp->sortSection = view->sortIndicatorSection();


    initGatherer();


    // scoped_ptr
    _imp->watcher.reset(new QFileSystemWatcher);
    assert(_imp->watcher);
    QObject::connect( _imp->watcher.get(), SIGNAL(directoryChanged(QString)), this, SLOT(onWatchedDirectoryChanged(QString)) );
    QObject::connect( _imp->watcher.get(), SIGNAL(fileChanged(QString)), this, SLOT(onWatchedFileChanged(QString)) );
    _imp->watcher->addPath( QDir::rootPath() );

    resetCompletly(true);
}

//////////////////////////////////////Overridden from QAbstractItemModel///////////////////////////////////////


#ifdef __NATRON_WIN32__

void
FileSystemModel::initDriveLettersToNetworkShareNamesMapping()
{
    QFileInfoList drives = QDir::drives();

    Q_FOREACH(const QFileInfo &drive, drives) {
        // for drives, there is no filename
        QString driveName  = drive.canonicalPath();
        QString uncPath = mapPathWithDriveLetterToPathWithNetworkShareName(driveName);

#ifdef DEBUG
        qDebug() << "Filesystem: " << driveName << " = " << uncPath;
#endif
    }
}

QString
FileSystemModel::mapPathWithDriveLetterToPathWithNetworkShareName(const QString& path)
{
    if (path.size() <= 2) {
        return path;
    }

    QString ret;
    if ( path[0].isLetter() && ( path[1] == QLatin1Char(':') ) ) {
        QString driveName = path.mid(0, 2);
        wchar_t szDeviceName[512];
        DWORD dwResult, cchBuff = sizeof(szDeviceName);

        dwResult = WNetGetConnectionW(driveName.toStdWString().c_str(), szDeviceName, &cchBuff);
        if (dwResult == NO_ERROR) {
            ret = path.mid(2, -1);

            //Replace \\ with /
            std::wstring wstr(szDeviceName);
            QString qDeviceName = QString::fromStdWString(wstr);

            qDeviceName.replace( QLatin1Char('\\'), QLatin1Char('/') );

            //Make sure we remember the mapping
            appPTR->registerUNCPath(qDeviceName, path[0]);

            ret.prepend(qDeviceName);

            return ret;
        }
    }
    ret = path;

    return ret;
}

#endif // ifdef __NATRON_WIN32__

FileSystemModel::~FileSystemModel()
{
    if (_imp->gatherer) {
        _imp->gatherer->quitGatherer();
    }
}

void
FileSystemModelPrivate::registerItem(const FileSystemItemPtr& item)
{
    QMutexLocker k(&mappingMutex);

    itemsMap[item.get()] = item;
}

void
FileSystemModelPrivate::unregisterItem(FileSystemItem* item)
{
    QMutexLocker k(&mappingMutex);
    std::map<FileSystemItem*, FileSystemItemWPtr>::iterator found = itemsMap.find(item);

    if ( found != itemsMap.end() ) {
        itemsMap.erase(found);
    }
}

FileSystemItemPtr
FileSystemModel::getSharedItemPtr(FileSystemItem* item) const
{
    QMutexLocker k(&_imp->mappingMutex);
    std::map<FileSystemItem*, FileSystemItemWPtr>::const_iterator found = _imp->itemsMap.find(item);

    if ( found != _imp->itemsMap.end() ) {
        return found->second.lock();
    }

    return FileSystemItemPtr();
}

bool
FileSystemModel::isDriveName(const QString& name)
{
#ifdef __NATRON_WIN32__

    return name.size() == 3 && name.at(0).isLetter() && name.at(1) == QLatin1Char(':') && ( name.at(2) == QLatin1Char('/') || name.at(2) == QLatin1Char('\\') );
#else

    return name == QDir::rootPath();
#endif
}

bool
FileSystemModel::startsWithDriveName(const QString& name)
{
#ifdef __NATRON_WIN32__

    return name.size() >= 3 && name.at(0).isLetter() && name.at(1) == QLatin1Char(':') && ( name.at(2) == QLatin1Char('/') || name.at(2) == QLatin1Char('\\') );
#else

    return name.startsWith( QDir::rootPath() );
#endif
}

QVariant
FileSystemModel::headerData(int section,
                            Qt::Orientation orientation,
                            int role) const
{
    if (orientation == Qt::Horizontal) {
        switch (role) {
        // in case of DisplayRole, just return the header text
        case Qt::DisplayRole:

            return _imp->headers[section];
            break;
        // in case of TextAlignmentRole, only Size column will be right align,
        // others will be left align
        case Qt::TextAlignmentRole:

            return (Sections)section == Size ? Qt::AlignRight : Qt::AlignLeft;
            break;
        }
    }

    return QVariant();
}

Qt::ItemFlags
FileSystemModel::flags(const QModelIndex &index) const
{
    if ( !index.isValid() ) {
        return 0;
    }

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
    FileSystemItemPtr item = getItem(parent);

    if (item) {
        return item->childCount();
    } else {
        return 0;
    }
}

QVariant
FileSystemModel::data(const QModelIndex &index,
                      int role) const
{
    if ( !index.isValid() ) {
        return QVariant();
    }

    Sections col = (Sections)index.column();


    // in case of TextAlignmentRole, only Size column will be right align,
    // others will be left align
    if ( (role == Qt::TextAlignmentRole) && (col == Size) ) {
        return Qt::AlignRight;
    }


    FileSystemItemPtr item = getItem(index);

    if (!item) {
        return QVariant();
    }

    if ( (role == FilePathRole) && (col == Name) ) {
        return item->absoluteFilePath();
    } else if ( (role == FileNameRole) && (col == Name) ) {
        return item->fileName();
    }

    if ( (role == Qt::DecorationRole) && (col == Name) ) {
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
} // FileSystemModel::data

QModelIndex
FileSystemModel::index(int row,
                       int column,
                       const QModelIndex &parent) const
{
    // As the Name column is a tree, we will only create index whose parent is in the Name column
    if ( parent.isValid() && ( (Sections)parent.column() != Name ) ) {
        return QModelIndex();
    }

    FileSystemItemPtr parentItem = getItem(parent);


    if (parentItem) {
        FileSystemItemPtr childItem = parentItem->childAt(row);

        if (childItem) {
            return createIndex( row, column, childItem.get() );
        }
    }

    return QModelIndex();
}

QModelIndex
FileSystemModel::parent(const QModelIndex &index) const
{
    if ( !index.isValid() ) {
        return QModelIndex();
    }

    FileSystemItemPtr childItem = getItem(index);

    if (!childItem) {
        return QModelIndex();
    }

    FileSystemItemPtr parentItem = childItem->getParentItem();

    // if there is no parent or parent is invisible, there is no index
    if ( !parentItem || (parentItem == _imp->rootItem) ) {
        return QModelIndex();
    }
    assert(parentItem);

    return createIndex( parentItem->indexInParent(), (int)Name, parentItem.get() );
}

QStringList
FileSystemModel::mimeTypes() const
{
    return QStringList( QLatin1String("text/uri-list") );
}

QMimeData*
FileSystemModel::mimeData(const QModelIndexList & indexes) const
{
    QList<QUrl> urls;
    for (QList<QModelIndex>::const_iterator it = indexes.begin(); it != indexes.end(); ++it) {
        if ( (*it).column() == 0 ) {
            urls << QUrl::fromLocalFile( absolutePath(*it) );
        }
    }
    QMimeData *data = new QMimeData();
    data->setUrls(urls);

    return data;
}

/////////////////////////////////////// End overrides ////////////////////////////////////////////////////

QModelIndex
FileSystemModel::index(const QString& path,
                       int column) const
{
    FileSystemItemPtr item = getFileSystemItem(path);

    if (item) {
        return index(item.get(), column);
    }

    return QModelIndex();
}

FileSystemItemPtr
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
FileSystemModel::index(FileSystemItem* item,
                       int column) const
{
    assert(item);
    if (!item) {
        return QModelIndex();
    }

    return createIndex(item->indexInParent(), column, item);
}

bool
FileSystemModel::isDir(const QModelIndex &index)
{
    if ( !index.isValid() ) {
        return false;
    }
    FileSystemItem *item = getFileSystemItem(index);

    if ( item && ( item != _imp->rootItem.get() ) ) {
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

    if ( item && ( item != _imp->rootItem.get() ) ) {
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
    FileSystemItemPtr parent = _imp->getItemFromPath( info.absolutePath() );

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
            while ( i < filters.size() && filters.at(i) != QLatin1Char(' ') ) {
                regExp.append( filters.at(i) );
                ++i;
            }
            if ( regExp != QString( QLatin1Char('*') ) ) {
                QRegExp rx(regExp, Qt::CaseInsensitive, QRegExp::Wildcard);
                if ( rx.isValid() ) {
                    _imp->regexps.push_back(rx);
                }
            }
            ++i;
        }
    }
    resetCompletly(true);
}

QString
FileSystemModel::generateRegexpFilterFromFileExtensions(const QStringList& extensions)
{
    if ( extensions.empty() ) {
        return QString();
    }

    QString ret;

    Q_FOREACH(const QString &ext, extensions) {
        if ( ext != QString::fromUtf8("*") ) {
            ret.append( QString::fromUtf8("*.") );
        }
        ret.append( ext );
        ret.append( QChar::fromLatin1(' ') );
    }
    ret.chop(1); // remove trailing space

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

    resetCompletly(true);

    ///Now re-populate the current root path
    setRootPath(_imp->currentRootPath);
}

void
FileSystemModel::resetCompletly(bool rebuild)
{
    {
        QMutexLocker k(&_imp->mappingMutex);
        _imp->itemsMap.clear();
    }

    if (_imp->rootItem) {
        beginResetModel();

        ///Wipe all the file-system loaded by clearing all children of drives
        for (int i = 0; i < _imp->rootItem->childCount(); ++i) {
            FileSystemItemPtr child = _imp->rootItem->childAt(i);
            if (child) {
                child->clearChildren();
            }
        }
        _imp->rootItem->clearChildren();
        _imp->rootItem.reset();
        endResetModel();
    }


    FileSystemModelPtr model = shared_from_this();

    if (rebuild) {
        _imp->rootItem = FileSystemItem::create(model, true, QString(), QString(), SequenceParsing::SequenceFromFilesPtr(), QDateTime(), 0);
        _imp->registerItem(_imp->rootItem);

        QFileInfoList drives = QDir::drives();

        ///Fetch all drives by default
        Q_FOREACH(const QFileInfo &drive, drives) {
            QString driveName;

#ifdef __NATRON_WIN32__
            // for drives, there is no filename
            driveName = drive.canonicalPath();
#else
            driveName = generateChildAbsoluteName( _imp->rootItem.get(), drive.fileName() );
#endif

            FileSystemItemPtr child = FileSystemItem::create(model, true, //isDir
                                                                        driveName, //drives have canonical path
                                                                        driveName,
                                                                        SequenceParsing::SequenceFromFilesPtr(),
                                                                        drive.lastModified(),
                                                                        drive.size(),
                                                                        _imp->rootItem);
            _imp->registerItem(child);
            _imp->rootItem->addChild(child);
        }
    }
} // FileSystemModel::resetCompletly

bool
FileSystemModel::isSequenceModeEnabled() const
{
    QMutexLocker l(&_imp->sequenceModeEnabledMutex);

    return _imp->sequenceModeEnabled;
}

FileSystemItemPtr
FileSystemModel::mkPathInternal(const FileSystemItemPtr& item,
                                const QStringList& path,
                                int index)
{
    assert(index < path.size() && index >= 0);

    FileSystemItemPtr child;
    for (int i = 0; i < item->childCount(); ++i) {
        FileSystemItemPtr c = item->childAt(i);
        if ( c && (c->fileName() == path[index]) ) {
            child = c;
            break;
        }
    }

    if (!child) {
        QFileInfo info( generateChildAbsoluteName(item.get(), path[index]) );
        ///The child doesn't exist already, create it without populating it
        child = FileSystemItem::create(shared_from_this(), true, //isDir
                                        path[index], //name
                                        path[index], //name
                                        SequenceParsing::SequenceFromFilesPtr(),
                                        info.lastModified(),
                                        0, //0 for directories
                                        item);
        _imp->registerItem(child);
        item->addChild(child);
    }
    if (index == path.size() - 1) {
        return child;
    } else {
        return mkPathInternal(child, path, index + 1);
    }
}

FileSystemItemPtr
FileSystemModel::mkPath(const QString& path)
{
    if ( path.isEmpty() || !_imp->rootItem) {
        return _imp->rootItem;
    }

    QStringList splitPath = getSplitPath(path);

    return mkPathInternal(_imp->rootItem, splitPath, 0);
}

QModelIndex
FileSystemModel::mkdir(const QModelIndex& parent,
                       const QString& name)
{
    if ( !parent.isValid() ) {
        return QModelIndex();
    }

    FileSystemItem *item = getFileSystemItem(parent);

    if (item) {
        QDir dir( item->absoluteFilePath() );
        dir.mkpath(name);
        FileSystemItemPtr newDir = mkPath( generateChildAbsoluteName(item, name) );
        if (!newDir) {
            return QModelIndex();
        }
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
FileSystemModel::onSortIndicatorChanged(int logicalIndex,
                                        Qt::SortOrder order)
{
    {
        QMutexLocker l(&_imp->sortMutex);
        _imp->sortSection = logicalIndex;
        _imp->ordering = order;
    }
    FileSystemItemPtr item = _imp->getItemFromPath(_imp->currentRootPath);

    if (item) {
        cleanAndRefreshItem(item);
    }
}

FileSystemItemPtr
FileSystemModelPrivate::getItemFromPath(const QString &path) const
{
    if ( path.isEmpty() || !rootItem ) {
        return rootItem;
    }
    QStringList splitPath = getSplitPath(path);

    return rootItem->matchPath( splitPath, 0 );
}

bool
FileSystemModel::setRootPath(const QString& path)
{
    assert( QThread::currentThread() == qApp->thread() );


    ///Check if the path exists
    if ( !path.isEmpty() ) {
        QDir dir(path);
        if ( !dir.exists() ) {
            return false;
        }
    }

    _imp->currentRootPath = path;


    ///Make sure the path exist
    FileSystemItemPtr item = mkPath(path);
    assert(item);
    if (!item) {
        return false;
    }
    if (item != _imp->rootItem) {
        // scoped_ptr
        _imp->watcher.reset(new QFileSystemWatcher);
        assert(_imp->watcher);
        QObject::connect( _imp->watcher.get(), SIGNAL(directoryChanged(QString)), this, SLOT(onWatchedDirectoryChanged(QString)) );
        QObject::connect( _imp->watcher.get(), SIGNAL(fileChanged(QString)), this, SLOT(onWatchedFileChanged(QString)) );
        _imp->watcher->removePath(_imp->currentRootPath);
        _imp->watcher->addPath( item->absoluteFilePath() );

        ///Since we are about to kill some FileSystemItem's we must force a reset of the QAbstractItemModel to clear the persistent
        ///QModelIndex left in the model that may hold raw pointers to bad FileSystemItem's
        beginResetModel();
        endResetModel();

        _imp->populateItem(item);
    } else {
        Q_EMIT directoryLoaded(path);
    }

    Q_EMIT rootPathChanged(path);

    return true;
}

void
FileSystemModel::initGatherer()
{
    if (!_imp->gatherer) {
        _imp->gatherer.reset( new FileGathererThread( shared_from_this() ) );
        assert(_imp->gatherer);
        QObject::connect( _imp->gatherer.get(), SIGNAL(directoryLoaded(QString)), this, SLOT(onDirectoryLoadedByGatherer(QString)) );
    }
}

void
FileSystemModelPrivate::populateItem(const FileSystemItemPtr &item)
{
    ///We do it in a separate thread because it might be expensive,
    ///the directoryLoaded signal will be emitted when it is finished
    assert(gatherer);
    gatherer->fetchDirectory(item);
}

void
FileSystemModel::onDirectoryLoadedByGatherer(const QString& directory)
{
    ///Get the item corresponding to the directory
    FileSystemItemPtr item = _imp->getItemFromPath(directory);

    if (!item) {
        qDebug() << "FileSystemModel failed to load the following requested directory: " << directory;

        return;
    }

    if (directory != _imp->currentRootPath) {
        return;
    }


    ///Finally notify the client that the directory is ready for use
    Q_EMIT directoryLoaded(directory);
}

void
FileSystemModel::onWatchedDirectoryChanged(const QString& directory)
{
    FileSystemItemPtr item = _imp->getItemFromPath(directory);

    if (item) {
        if (directory == _imp->currentRootPath) {
            cleanAndRefreshItem(item);
        } else {
            ///This is a sub-directory
            ///Clear the parent of the corresponding item
            FileSystemItemPtr parent = item->getParentItem();
            if (parent) {
                QModelIndex idx = index( item.get() );
                assert( idx.isValid() );
                int count = parent->childCount();
                if (count > 0) {
                    beginRemoveRows(idx.parent(), 0, count - 1);
                    parent->clearChildren();
                    endRemoveRows();
                }
            }
        }
    }

    QDir dir(_imp->currentRootPath);
    if ( !dir.exists() ) {
        ///The current directory has changed its name or was deleted.. just fallback the filesystem to the root-path
        setRootPath( QDir::rootPath() );
    } else {
        setRootPath(_imp->currentRootPath);
    }
}

void
FileSystemModel::onWatchedFileChanged(const QString& file)
{
    ///Get the item corresponding to the current directory
    QFileInfo info(file);
    FileSystemItemPtr parent = _imp->getItemFromPath( info.absolutePath() );

    if (parent) {
        cleanAndRefreshItem(parent);
    }
}

void
FileSystemModel::cleanAndRefreshItem(const FileSystemItemPtr& item)
{
    if (!item) {
        return;
    }
    QModelIndex idx = index(item.get(), 0);
    if ( idx.isValid() ) {
        int count = item->childCount();
        if (count > 0) {
            beginRemoveRows(idx, 0, count - 1);
            item->clearChildren();
            endRemoveRows();
        }

        _imp->populateItem(item);
    }
}

FileSystemItemPtr
FileSystemModel::getItem(const QModelIndex &index) const
{
    // just return the internal pointer we set at creating index if the index is valid

    if ( index.isValid() ) {
        FileSystemItem *item = static_cast<FileSystemItem*>( index.internalPointer() );
        if (item) {
            return getSharedItemPtr(item);
        }
    }

    return _imp->rootItem;
}

///////////////////////// FileGathererThread

struct FileGathererThreadPrivate
{
    FileSystemModelWPtr model;
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
    FileSystemItemPtr requestedItem, itemBeingFetched;
    QMutex requestedDirMutex;

    FileGathererThreadPrivate(const FileSystemModelPtr& model)
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

    FileSystemModelPtr getModel() const
    {
        return model.lock();
    }
};

FileGathererThread::FileGathererThread(const FileSystemModelPtr& model)
    : QThread()
    , _imp( new FileGathererThreadPrivate(model) )
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
    if ( !isRunning() ) {
        return;
    }
    abortGathering();
    {
        QMutexLocker k(&_imp->mustQuitMutex);
        assert(!_imp->mustQuit);
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

    ~WorkingSetter()
    {
        QMutexLocker k(&_imp->workingMutex);

        _imp->working = false;
    }
};

void
FileGathererThread::run()
{
#ifdef DEBUG
    boost_adaptbx::floating_point::exception_trapping trap(boost_adaptbx::floating_point::exception_trapping::division_by_zero |
                                                           boost_adaptbx::floating_point::exception_trapping::invalid |
                                                           boost_adaptbx::floating_point::exception_trapping::overflow);
#endif
    for (;; ) {
        {
            WorkingSetter working( _imp.get() );

            if ( _imp->checkForExit() ) {
                return;
            }

            {
                QMutexLocker k(&_imp->requestedDirMutex);
                _imp->itemBeingFetched = _imp->requestedItem;
                _imp->requestedItem.reset();
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

static bool
isVideoFileExtension(const std::string& ext)
{
    if ( (ext == "mov") ||
         ( ext == "avi") ||
         ( ext == "mp4") ||
         ( ext == "mpeg") ||
         ( ext == "flv") ||
         ( ext == "mkv") ||
         ( ext == "mxf") ||
         ( ext == "mpg") ||
         ( ext == "mts") ||
         ( ext == "m2ts") ||
         ( ext == "ts") ||
         ( ext == "webm") ||
         ( ext == "ogg") ||
         ( ext == "ogv") ) {
        return true;
    }

    return false;
}

typedef std::list<std::pair<SequenceParsing::SequenceFromFilesPtr, QFileInfo > > FileSequences;

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
FileGathererThread::gatheringKernel(const FileSystemItemPtr& item)
{
    if (!item) {
        return;
    }
    QDir dir( item->absoluteFilePath() );
    FileSystemModelPtr model = _imp->getModel();
    if (!model) {
        return;
    }

    Qt::SortOrder viewOrder = model->sortIndicatorOrder();
    FileSystemModel::Sections sortSection = (FileSystemModel::Sections)model->sortIndicatorSection();
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
    QFileInfoList all = dir.entryInfoList(model->filter(), sort);

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
            sequences.push_back( std::make_pair(SequenceParsing::SequenceFromFilesPtr(), all[i]) );
        } else {
            QString filename = all[i].fileName();
            /// If the item does not match the filter regexp set by the user, discard it
            if ( !model->isAcceptedByRegexps(filename) ) {
                KERNEL_INCR();
                continue;
            }

            /// If file sequence fetching is disabled, accept it
            if ( !model->isSequenceModeEnabled() ) {
                sequences.push_back( std::make_pair(SequenceParsing::SequenceFromFilesPtr(), all[i]) );
                KERNEL_INCR();
                continue;
            }

            std::string absoluteFilePath = generateChildAbsoluteName(item.get(), filename).toStdString();
            bool foundMatchingSequence = false;

            /// If we reach here, this is a valid file and we need to determine if it belongs to another sequence or we need
            /// to create a new one
            SequenceParsing::FileNameContent fileContent(absoluteFilePath);

            if ( !isVideoFileExtension( fileContent.getExtension() ) ) {
                ///Note that we use a reverse iterator because we have more chance to find a match in the last recently added entries
                for (FileSequences::reverse_iterator it = sequences.rbegin(); it != sequences.rend(); ++it) {
                    if ( it->first && it->first->tryInsertFile(fileContent, false) ) {
                        foundMatchingSequence = true;
                        break;
                    }
                }
            }

            if (!foundMatchingSequence) {
                SequenceParsing::SequenceFromFilesPtr newSequence = boost::make_shared<SequenceParsing::SequenceFromFiles>(fileContent, true);
                sequences.push_back( std::make_pair(newSequence, all[i]) );
            }
        }
        KERNEL_INCR();
    }

    ///Now iterate through the sequences and create the children as necessary
    for (FileSequences::iterator it = sequences.begin(); it != sequences.end(); ++it) {
        item->addChild(it->first, it->second);
    }

    Q_EMIT directoryLoaded( item->absoluteFilePath() );
} // FileGathererThread::gatheringKernel

void
FileGathererThread::fetchDirectory(const FileSystemItemPtr& item)
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

bool
FileSystemModel::filesListFromPattern(const std::string& pattern, SequenceParsing::SequenceFromPattern* sequence)
{
    std::string patternCpy = pattern;
    std::string patternPath = SequenceParsing::removePath(patternCpy);

    QDir dir(QString::fromUtf8(patternPath.c_str()));
    if (!dir.exists()) {
        return false;
    }

    QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);

    SequenceParsing::StringList filesList;
    for (QStringList::iterator it = files.begin(); it!=files.end(); ++it) {
        filesList.push_back(it->toStdString());
    }
    return SequenceParsing::filesListFromPattern_fast(pattern, filesList, sequence);
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_FileSystemModel.cpp"
