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

#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif
#include <QtCore/QThread>
#include <QtCore/QAbstractItemModel>
#include <QtCore/QDir>



#include "Global/GlobalDefines.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

class FileSystemModel;
struct FileSystemItemPrivate;
class FileSystemItem
    : public boost::enable_shared_from_this<FileSystemItem>
{
    struct MakeSharedEnabler;

    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    FileSystemItem( const FileSystemModelPtr& model,
                    bool isDir,
                    const QString& filename,
                    const QString& userFriendlySequenceName,
                    const SequenceParsing::SequenceFromFilesPtr& sequence,
                    const QDateTime& dateModified,
                    quint64 size,
                    const FileSystemItemPtr& parent = FileSystemItemPtr() );
public:
    // public constructors

    // Note: when switching to C++11, we can add variadic templates:
    //template<typename ... T>
    //static FileSystemItemPtr create( T&& ... all ) {
    //    return boost::make_shared<FileSystemItem>( std::forward<T>(all)... );
    //}
    static FileSystemItemPtr create( const FileSystemModelPtr& model,
                                                     bool isDir,
                                                     const QString& filename,
                                                     const QString& userFriendlySequenceName,
                                                     const SequenceParsing::SequenceFromFilesPtr& sequence,
                                                     const QDateTime& dateModified,
                                                     quint64 size,
                                                    const FileSystemItemPtr& parent = FileSystemItemPtr() );

    ~FileSystemItem();

    void resetModelPointer();

    FileSystemItemPtr childAt(int position) const;

    int childCount() const;

    int indexInParent() const;

    FileSystemItemPtr getParentItem() const;

    /**
     * @brief If the item is a file this function will return its absolute file-path.
     * If it is a directory this function will return the directory absolute path.
     * This is faster than fileName()
     **/
    const QString& absoluteFilePath() const;

    bool isDir() const;

    SequenceParsing::SequenceFromFilesPtr getSequence() const;

    /**
     * @brief Returns the fileName without path.
     * If this is a directory this function returns an empty string.
     **/
    const QString& fileName() const;
    const QString& getUserFriendlyFilename() const;
    const QString& fileExtension() const;
    const QDateTime& getLastModified() const;

    quint64 getSize() const;

    /**
     * @brief Add a new child, MT-safe
     **/
    void addChild(const FileSystemItemPtr& child);

    void addChild(const SequenceParsing::SequenceFromFilesPtr& sequence,
                  const QFileInfo& info);

    /**
     * @brief Remove all children, MT-safe
     **/
    void clearChildren();

    /**
     * @brief Tries to find in this item and its children an item with a matching path.
     * @param path The path of the directory/file that has been split by QDir::separator()
     **/
    FileSystemItemPtr matchPath(const QStringList& path,
                                                int startIndex = 0) const;


private:

    boost::scoped_ptr<FileSystemItemPrivate> _imp;
};

class FileSystemModel;
struct FileGathererThreadPrivate;
class FileGathererThread
    : public QThread
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    FileGathererThread(const FileSystemModelPtr& model);

    virtual ~FileGathererThread();

    void abortGathering();

    void quitGatherer();

    void fetchDirectory(const FileSystemItemPtr& item);

    bool isWorking() const;
Q_SIGNALS:

    void directoryLoaded(QString);

private:

    virtual void run() OVERRIDE FINAL;

    void gatheringKernel(const FileSystemItemPtr& item);

    boost::scoped_ptr<FileGathererThreadPrivate> _imp;
};


class SortableViewI
{
public:

    SortableViewI() {}

    virtual ~SortableViewI() {}

    /**
     * @brief Returns the order for the sort indicator. If no section has a sort indicator the return value of this function is undefined.
     **/
    virtual Qt::SortOrder sortIndicatorOrder() const = 0;

    /**
     * @brief Returns the logical index of the section that has a sort indicator. By default this is section 0.
     **/
    virtual int sortIndicatorSection() const = 0;

    /**
     * @brief Called when the section containing the sort indicator or the order indicated is changed.
     * The section's logical index is specified by logicalIndex and the sort order is specified by order.
     **/
    virtual void onSortIndicatorChanged(int logicalIndex, Qt::SortOrder order) = 0;
};

struct FileSystemModelPrivate;
class FileSystemModel
    : public QAbstractItemModel
    , public boost::enable_shared_from_this<FileSystemModel>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    enum Roles
    {
        FilePathRole = Qt::UserRole + 1,
        FileNameRole = Qt::UserRole + 2,
    };

    enum Sections
    {
        Name = 0,
        Size,
        Type,
        DateModified,
        EndSections //do not use
    };


#ifdef __NATRON_WIN32__
    static QString mapPathWithDriveLetterToPathWithNetworkShareName(const QString& path);
    static void initDriveLettersToNetworkShareNamesMapping();
#endif


    FileSystemModel();

    void initialize(SortableViewI* view);

    ////////////////////////////////////// Overridden from QAbstractItemModel ///////////////////////////////

    virtual ~FileSystemModel();

    static bool isDriveName(const QString& name);
    static bool startsWithDriveName(const QString& name);
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int columnCount(const QModelIndex & parent) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int rowCount(const QModelIndex & parent) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual QVariant data(const QModelIndex &index, int role) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual QModelIndex parent(const QModelIndex &index) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual QStringList mimeTypes() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual QMimeData* mimeData(const QModelIndexList & indexes) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /////////////////////////////////////// End overrides ////////////////////////////////////////////////////

    QModelIndex index(const QString& path, int column = 0) const WARN_UNUSED_RETURN;

    FileSystemItemPtr getFileSystemItem(const QString& path) const WARN_UNUSED_RETURN;
    FileSystemItem* getFileSystemItem(const QModelIndex& index) const WARN_UNUSED_RETURN;

    QModelIndex index(FileSystemItem* item, int column = 0) const WARN_UNUSED_RETURN;

    bool isDir(const QModelIndex &index) WARN_UNUSED_RETURN;

    // absoluteFilePath method of QFileInfo class cause the file system
    // query hence causes slower performance.
    QString absolutePath(const QModelIndex &index) const WARN_UNUSED_RETURN;

    QString rootPath() const WARN_UNUSED_RETURN;

    /**
     * @brief Set the root path of the filesystem to the given path. This will force it to load the directory
     * and its content will then be accessible via index(...) and iterating through rowCount().
     * You may only use these methods once the directoryLoaded signal is sent, indicating that the worker thread
     * has gathered all info.
     **/
    bool setRootPath(const QString& path);

    QVariant myComputer(int role = Qt::DisplayRole) const WARN_UNUSED_RETURN;

    void setFilter(const QDir::Filters& filters);

    const QDir::Filters filter() const WARN_UNUSED_RETURN;

    /**
     * @brief Set regexp filters (in the wildcard unix form).
     * @param filters A suite of regexp separated by a space
     **/
    void setRegexpFilters(const QString& filters);

    /**
     * @brief Generates an encoded regexp filter that can be passed to setRegexpFilters from a list of file extensions
     * @extensions A list of file extensions in the form "jpg", "png", "exr" etc...
     **/
    static QString generateRegexpFilterFromFileExtensions(const QStringList& extensions) WARN_UNUSED_RETURN;

    /*
     * @brief Check if the path is accepted by the filter installed by the user
     */
    bool isAcceptedByRegexps(const QString & path) const WARN_UNUSED_RETURN;

    /**
     * @brief Set the file-system in sequence mode: it will try to recnognize file sequences instead of only separate files/directory.
     * By default this is disabled.
     **/
    void setSequenceModeEnabled(bool sequenceMode);

    /**
     * @brief Returns true if sequence mode is enabled
     **/
    bool isSequenceModeEnabled() const;

    /*
     * @brief Make a new directory
     */
    QModelIndex mkdir(const QModelIndex& parent, const QString& name) WARN_UNUSED_RETURN;

    Qt::SortOrder sortIndicatorOrder() const WARN_UNUSED_RETURN;

    int sortIndicatorSection() const WARN_UNUSED_RETURN;

    void onSortIndicatorChanged(int logicalIndex, Qt::SortOrder order);

    void resetCompletly(bool rebuild);

    static bool filesListFromPattern(const std::string& pattern,  std::map<int,std::map<int,std::string> >* sequence);


public Q_SLOTS:

    void onDirectoryLoadedByGatherer(const QString& directory);

    void onWatchedDirectoryChanged(const QString& directory);

    void onWatchedFileChanged(const QString& file);

Q_SIGNALS:

    void rootPathChanged(QString);
    void directoryLoaded(QString);

private:

    void initGatherer();


    FileSystemItemPtr mkPath(const QString& path);
    FileSystemItemPtr mkPathInternal(const FileSystemItemPtr& item, const QStringList& path, int index);
    FileSystemItemPtr getSharedItemPtr(FileSystemItem* item) const;
    FileSystemItemPtr getItem(const QModelIndex &index) const;


    void cleanAndRefreshItem(const FileSystemItemPtr& item);

    friend class FileSystemItem;

    boost::scoped_ptr<FileSystemModelPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // FILESYSTEMMODEL_H
