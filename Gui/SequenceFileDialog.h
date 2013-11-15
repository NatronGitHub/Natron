//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef NATRON_GUI_SEQUENCEFILEDIALOG_H_
#define NATRON_GUI_SEQUENCEFILEDIALOG_H_

#include <vector>
#include <string>
#include <map>
#include <utility>

#include <boost/shared_ptr.hpp>

#include <QStyledItemDelegate>
#include <QTreeView>
#include <QDialog>
#include <QSortFilterProxyModel>
#include <QFileSystemModel>
#include <QtCore/QByteArray>
#include <QtGui/QStandardItemModel>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QDir>
#include <QtCore/QMutex>
#include <QtCore/QReadWriteLock>
#include <QtCore/QUrl>
#include <QtCore/QLatin1Char>
#include <QComboBox>
#include <QListView>

#include "Global/Macros.h"

class LineEdit;
class Button;
class QCheckBox;
class ComboBox;
class QWidget;
class QLabel;
class QHBoxLayout;
class QVBoxLayout;
class QSplitter;
class QAction;
class SequenceFileDialog;
class SequenceItemDelegate;


#if QT_VERSION < 0x050000
namespace Natron{
    inline bool removeRecursively(const QString& dirName){
        bool result = false;
        QDir dir(dirName);
        
        if (dir.exists(dirName)) {
            Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
                if (info.isDir()) {
                    result = removeRecursively(info.absoluteFilePath());
                }
                else {
                    result = QFile::remove(info.absoluteFilePath());
                }
                
                if (!result) {
                    return result;
                }
            }
            result = dir.rmdir(dirName);
        }
        return result;
    }
}
#endif

/**
 * @brief The UrlModel class is the model used by the favorite view in the file dialog. It serves as a connexion between
 *the file system and some urls.
 */
class  UrlModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum Roles {
        UrlRole = Qt::UserRole + 1,
        EnabledRole = Qt::UserRole + 2
    };
    
    explicit UrlModel(QObject *parent = 0);
    
    QStringList mimeTypes() const;
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool canDrop(QDragEnterEvent *event);
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    
    bool setData(const QModelIndex &index, const QVariant &value, int role=Qt::EditRole);

    
    void setUrls(const std::vector<QUrl> &urls);
    void addUrls(const std::vector<QUrl> &urls, int row = -1, bool move = true);
    std::vector<QUrl> urls() const;
    void setFileSystemModel(QFileSystemModel *model);
    QFileSystemModel* getFileSystemModel() const {return fileSystemModel;}
    void setUrl(const QModelIndex &index, const QUrl &url, const QModelIndex &dirIndex);

public slots:
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void layoutChanged();
    
private:
    void changed(const QString &path);
    void addIndexToWatch(const QString &path, const QModelIndex &index);
    QFileSystemModel *fileSystemModel;
    std::vector<std::pair<QModelIndex, QString> > watching;
    std::vector<QUrl> invalidUrls;
};

class FavoriteItemDelegate : public QStyledItemDelegate {
    QFileSystemModel *_model;
public:
    FavoriteItemDelegate(QFileSystemModel *model):QStyledItemDelegate(),_model(model){}

protected:
    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
};

/**
 * @brief The FavoriteView class is the favorite list seen in the file dialog.
 */
class FavoriteView : public QListView
{
    Q_OBJECT
    
signals:
    void urlRequested(const QUrl &url);
    
public:
    explicit FavoriteView(QWidget *parent = 0);
    void setModelAndUrls(QFileSystemModel *model, const std::vector<QUrl> &newUrls);
    ~FavoriteView();
    
    QSize sizeHint() const;
    
    void setUrls(const std::vector<QUrl> &list) { urlModel->setUrls(list); }
    void addUrls(const std::vector<QUrl> &list, int row) { urlModel->addUrls(list, row); }
    std::vector<QUrl> urls() const { return urlModel->urls(); }
    
    void selectUrl(const QUrl &url);

    void rename(const QModelIndex& index,const QString& name);
    
    
    
public slots:
    void clicked(const QModelIndex &index);
    void showMenu(const QPoint &position);
    void removeEntry();
    void rename();
    void editUrl();
protected:
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void dragEnterEvent(QDragEnterEvent *event);
    
    virtual void focusInEvent(QFocusEvent *event)
    {
        QAbstractScrollArea::focusInEvent(event);
        viewport()->update();
    }
private:
    UrlModel *urlModel;
    FavoriteItemDelegate *_itemDelegate;
};


/**
 * @brief The SequenceDialogView class is the view of the filesystem within the dialog.
 */
class SequenceDialogView: public QTreeView{
    SequenceFileDialog* _fd;
public:
    explicit SequenceDialogView(SequenceFileDialog* fd);
    
    void updateNameMapping(const std::vector<std::pair<QString,std::pair<qint64,QString> > >& nameMapping);

    inline void expandColumnsToFullWidth(){
        int size = width() - columnWidth(1) - columnWidth(2) - columnWidth(3);
        setColumnWidth(0,size);
    }
    
    void dropEvent(QDropEvent* event);
    
    void dragEnterEvent(QDragEnterEvent *ev);
    
    void dragMoveEvent(QDragMoveEvent* e);
    
    void dragLeaveEvent(QDragLeaveEvent* e);

};

class FileSequence{
    
public:
    
    class FrameIndexes{
    public:
        FrameIndexes():_isEmpty(true),_firstFrame(999999),_lastFrame(-1),_size(0){}
        
        ~FrameIndexes(){}
        
        bool isEmpty() const {return _isEmpty;}
        
        int firstFrame() const {return _firstFrame;}
        
        int lastFrame() const {return _lastFrame;}
        
        int size() const {return _size;}
        
        /*frame index is a frame index as read in the file name.*/
        bool isInSequence(int frameIndex) const;
        
        /*frame index is a frame index as read in the file name.*/
        bool addToSequence(int frameIndex);
        
        
    private:
        
        bool _isEmpty;
        int _firstFrame,_lastFrame;
        int _size;
        std::vector<quint64> _bits; /// each bit of a quint64 represents a frame index. The storage is a vector if there're more than 64
                                    /// files in the sequence. The first frame and the last frame are not counted in these bits.
    };
    
    
    FileSequence(const std::string& filetype):
     _fileType(filetype)
    ,_totalSize(0)
    ,_lock()
    {}

    FileSequence(const FileSequence& other):
        _fileType(other._fileType)
      , _frameIndexes(other._frameIndexes)
      , _totalSize(other._totalSize)
      , _lock()
    {
    }

    ~FileSequence(){}
    
    /*Returns true if the frameIndex didn't exist when adding it*/
    void addToSequence(int frameIndex,const QString& path);
    
    bool isInSequence(int frameIndex) const { QMutexLocker locker(&_lock); return _frameIndexes.isInSequence(frameIndex); }

    int size() const { return _frameIndexes.size(); }

    int firstFrame() const { return _frameIndexes.firstFrame(); }

    int lastFrame() const { return _frameIndexes.lastFrame(); }

    int isEmpty() const { return _frameIndexes.isEmpty(); }

    const std::string& getFileType() const { return _fileType; }

    qint64 getTotalSize() const { return _totalSize; }
    
    void lock() const { _lock.lock(); }

    void unlock() const { assert(!_lock.tryLock()); _lock.unlock(); }

 private:

    std::string _fileType;
    FrameIndexes _frameIndexes;
    qint64 _totalSize;
    mutable QMutex _lock;
};

namespace Natron{
    typedef std::multimap<std::string, boost::shared_ptr<FileSequence> > FrameSequences;
    typedef FrameSequences::iterator SequenceIterator;
    typedef FrameSequences::const_iterator ConstSequenceIterator;

}

/**
 * @brief The SequenceDialogProxyModel class is a proxy that filters image sequences from the QFileSystemModel
 */
class SequenceDialogProxyModel: public QSortFilterProxyModel{
    /*multimap of <sequence name, FileSequence >
     *Several sequences can have a same name but a different file extension within a same directory.
     */
    mutable QMutex _frameSequencesMutex; // protects _frameSequences
    mutable Natron::FrameSequences _frameSequences;
    SequenceFileDialog* _fd;
    QString _filter;

public:

    explicit SequenceDialogProxyModel(SequenceFileDialog* fd) : QSortFilterProxyModel(),_fd(fd){}

    virtual ~SequenceDialogProxyModel(){
        clear();
    }

    void clear(){_frameSequences.clear();}
    
    
    const Natron::FrameSequences& getFrameSequence() const{return _frameSequences;}
    
    
    inline void setFilter(QString filter){ _filter = filter;}

    /**@brief Returns in path the name of the file with the extension truncated and
     *the frame number truncated.
     *It returns in framenumber the number of the frame, or -1 if it had no frame number
     *and in extension the name of file type
     **/
    static void parseFilename(QString &path, int* frameNumber, QString &extension);

    
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
private:
       /*
    *Check if the path is accepted by the filter installed by the user
    */
    bool isAcceptedByUser(const QString& path) const;
};


class FileDialogComboBox : public QComboBox
{
public:
    explicit FileDialogComboBox(QWidget *parent = 0) : QComboBox(parent), urlModel(0) {}
    void setFileDialogPointer(SequenceFileDialog *p);
    void showPopup();
    void setHistory(const QStringList &paths);
    QStringList history() const { return m_history; }
    void paintEvent(QPaintEvent *);

private:
    UrlModel *urlModel;
    SequenceFileDialog *dialog;
    QStringList m_history;
};

/**
 * @brief The SequenceFileDialog class is the main dialog, containing the GUI and with whom the end user can interact.
 */
class SequenceFileDialog: public QDialog
{
    Q_OBJECT
    
public:
    enum FileDialogMode{OPEN_DIALOG = 0,SAVE_DIALOG = 1} ;

    typedef std::pair<QString,std::pair<qint64,QString> > NameMappingElement;
    typedef std::vector<NameMappingElement> NameMapping;
    
private:
    
    Natron::FrameSequences _frameSequences;
    mutable QReadWriteLock _nameMappingMutex; // protects _nameMapping
    NameMapping _nameMapping; // the item whose names must be changed
    
    std::vector<std::string> _filters;
    
    SequenceDialogView* _view;
    SequenceItemDelegate* _itemDelegate;
    SequenceDialogProxyModel* _proxy;
    QFileSystemModel* _model;
    QVBoxLayout* _mainLayout;
    QString _requestedDir;
    
    QLabel* _lookInLabel;
    FileDialogComboBox* _lookInCombobox;

    Button* _previousButton;
    Button* _nextButton;
    Button* _upButton;
    Button* _createDirButton;
    Button* _previewButton;
    Button* _openButton;
    Button* _cancelButton;
    Button* _addFavoriteButton;
    Button* _removeFavoriteButton;
    
    LineEdit* _selectionLineEdit;
    ComboBox* _sequenceButton;
    QLabel* _filterLabel;
    LineEdit* _filterLineEdit;
    Button* _filterDropDown;
    ComboBox* _fileExtensionCombo;

    
    QHBoxLayout* _buttonsLayout;
    QHBoxLayout* _centerLayout;
    QVBoxLayout* _favoriteLayout;
    QHBoxLayout* _favoriteButtonsLayout;
    QHBoxLayout* _selectionLayout;
    QHBoxLayout* _filterLineLayout;
    QHBoxLayout* _filterLayout;
    
    QWidget* _buttonsWidget;
    QWidget* _favoriteWidget;
    QWidget* _favoriteButtonsWidget;
    QWidget* _selectionWidget;
    QWidget* _filterLineWidget;
    QWidget* _filterWidget;
    
    FavoriteView* _favoriteView;

    QSplitter* _centerSplitter;
    
    QStringList _history;
    int _currentHistoryLocation;

    QAction* _showHiddenAction;
    QAction* _newFolderAction;
    
    FileDialogMode _dialogMode;
    
public:

    
    SequenceFileDialog(QWidget* parent, // necessary to transmit the stylesheet to the dialog
                       const std::vector<std::string>& filters, // the user accepted file types. Empty means it supports everything
                       bool isSequenceDialog = true, // true if this dialog can display sequences
                       FileDialogMode mode = OPEN_DIALOG, // if it is an open or save dialog
                       const std::string& currentDirectory = ""); // the directory to show first

    virtual ~SequenceFileDialog();
    
    void setRootIndex(const QModelIndex& index);

    void setFrameSequence(const Natron::FrameSequences& frameSequences);

    boost::shared_ptr<FileSequence> frameRangesForSequence(const std::string& sequenceName, const std::string& extension) const;
    
    bool isASupportedFileExtension(const std::string& ext) const;
    
    /*Removes the . and the extension from the filename and also
     *returns the extension as a string.*/
    static QString removeFileExtension(QString& filename);
    
    /**
     * @brief Tries to remove the digits prepending the file extension if the file is part of
     * a sequence. The digits MUST absolutely be placed before the last '.' of the file name.
     * If no such digits could be found, this function returns false.
     **/
    static bool removeSequenceDigits(QString& file,int* frameNumber);
    
    static QString removePath(const QString& str);
    
    static bool checkIfContiguous(const std::vector<int>& v);
    
    QStringList selectedFiles();
    
    /*Returns the pattern of the sequence, e.g:
     mySequence#.jpg*/
    QString getSequencePatternFromLineEdit();
    
    static QStringList filesListFromPattern(const QString& pattern);
    
    /*files must have the same extension and a digit placed before the . character prepending the extension*/
    static QString patternFromFilesList(const QStringList& files);
    
    QString filesToSave();
    
    QDir currentDirectory() const;

    void addFavorite(const QString& name,const QString& path);

    QByteArray saveState() const;

    bool restoreState(const QByteArray& state);

    bool sequenceModeEnabled() const;

    bool isDirectory(const QString& name) const;

    inline QString rootPath() const {
        return _model->rootPath();
    }

    QFileSystemModel* getFileSystemModel() const {return _model;}

    SequenceDialogView* getSequenceView() const {return _view;}

    inline QModelIndex mapToSource(const QModelIndex& index){
         return _proxy->mapToSource(index);
    }

    inline QModelIndex mapFromSource(const QModelIndex& index){
        QModelIndex ret =  _proxy->mapFromSource(index);
        setFrameSequence(_proxy->getFrameSequence());
        return ret;
    }

    static inline QString toInternal(const QString &path){
#if defined(Q_OS_WIN)
        QString n(path);
        n.replace(QLatin1Char('\\'),QLatin1Char('/'));
#if defined(Q_OS_WINCE)
        if((n.size() > 1) && n.startsWith(QLatin1String("//"))) {
            n = n.mid(1);
        }
#endif
        return n;
#else
        return path;
#endif
    }

    void setHistory(const QStringList &paths);
    QStringList history() const;

    QStringList typedFiles() const;

    QString getEnvironmentVariable(const QString &string);
    
    FileDialogMode getDialogMode() const {return _dialogMode;}
    
    /**
     * @brief Returns a vector of the different image file sequences found in the files given in parameter. 
     * Any file which has an extension not contained in the supportedFileType will be ignored.
     **/
    static std::vector<QStringList> fileSequencesFromFilesList(const QStringList& files,const QStringList& supportedFileTypes);
    
    /**
     * @brief Append all files in the current directory and all its sub-directories recursively.
     **/
    static void appendFilesFromDirRecursively(QDir* currentDir,QStringList* files);
public slots:

    void enterDirectory(const QModelIndex& index);

    void setDirectory(const QString &currentDirectory);
    void updateView(const QString& currentDirectory);
    
    void previousFolder();
    void nextFolder();
    void parentFolder();
    void goHome();
    void createDir();
    void addFavorite();
    void openSelectedFiles();
    void cancelSlot();
    void doubleClickOpen(const QModelIndex& index);
    void seekUrl(const QUrl& url);
    void selectionChanged();
    void enableSequenceMode(bool);
    void sequenceComboBoxSlot(const QString&);
    void showFilterMenu();
    void defaultFiltersSlot();
    void dotStarFilterSlot();
    void starSlashFilterSlot();
    void emptyFilterSlot();
    void applyFilter(QString filter);
    void showHidden();
    void showContextMenu(const QPoint &position);
    void pathChanged(const QString &newPath);
    void autoCompleteFileName(const QString&);
    void goToDirectory(const QString&);
    void setFileExtensionOnLineEdit(const QString&);
protected:
    virtual void keyPressEvent(QKeyEvent *e);

    virtual void resizeEvent(QResizeEvent* e);
private:
    
    void createMenuActions();
    
    /*parent in proxy indexes*/
    void itemsToSequence(const QModelIndex &parent);
    
    QModelIndex select(const QModelIndex& index);
    
    QString generateStringFromFilters();
    
    
};

/**
 * @brief The SequenceItemDelegate class is used to alterate the rendering of the cells in the filesystem view
 *within the file dialog. Mainly it transforms the text to draw for an item and also the size.
 */
class SequenceItemDelegate : public QStyledItemDelegate {

    int _maxW;
    mutable QReadWriteLock _nameMappingMutex; // protects _nameMapping
    SequenceFileDialog::NameMapping _nameMapping;
    SequenceFileDialog* _fd;
public:
    explicit SequenceItemDelegate(SequenceFileDialog* fd);

    void setNameMapping(const SequenceFileDialog::NameMapping& nameMapping);

protected:
    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const ;
};



class AddFavoriteDialog : public QDialog{
    
    Q_OBJECT
    
    QVBoxLayout* _mainLayout;
    
    QLabel* _descriptionLabel;
    
    QWidget* _secondLine;
    QHBoxLayout* _secondLineLayout;
    LineEdit* _pathLineEdit;
    Button* _openDirButton;
    SequenceFileDialog* _fd;
    
    QWidget* _thirdLine;
    QHBoxLayout* _thirdLineLayout;
    Button* _cancelButton;
    Button* _okButton;
public:
    
    AddFavoriteDialog(SequenceFileDialog* fd,QWidget* parent = 0);
    
    
    void setLabelText(const QString& text);
    
    QString textValue() const;
    
    virtual ~AddFavoriteDialog(){}
    
    public slots:
    
    void openDir();
    
};
#endif /* defined(NATRON_GUI_SEQUENCEFILEDIALOG_H_) */
