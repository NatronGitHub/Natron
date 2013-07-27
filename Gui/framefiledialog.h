//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/








#ifndef __PowiterOsX__filedialog__
#define __PowiterOsX__filedialog__

#include <QStyledItemDelegate>
#include <QTreeView>
#include <QDialog>
#include <QSortFilterProxyModel>
#include <QFileSystemModel>
#include <QDialogButtonBox>
#include <QtCore/QByteArray>
#include <QtGui/QStandardItemModel>
#include <QtCore/QStringList>
#include <QtCore/QDir>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QUrl>
#include <QComboBox>
#include <QListView>
#include <vector>
#include <string>
#include <iostream>

class LineEdit;
class Button;
class QCheckBox;
class QWidget;
class QLabel;
class QHBoxLayout;
class QVBoxLayout;
class QSplitter;
class QAction;
class SequenceFileDialog;


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
    
    UrlModel(QObject *parent = 0);
    bool setData(const QModelIndex &index, const QVariant &value, int role=Qt::EditRole);
    
    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool canDrop(QDragEnterEvent *event);
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

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

/**
 * @brief The FavoriteView class is the favorite list seen in the file dialog.
 */
class FavoriteView : public QListView
{
    Q_OBJECT
    
signals:
    void urlRequested(const QUrl &url);
    
public:
    FavoriteView(QWidget *parent = 0);
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
private:
    UrlModel *urlModel;
};

/**
 * @brief The SequenceItemDelegate class is used to alterate the rendering of the cells in the filesystem view
 *within the file dialog. Mainly it transforms the text to draw for an item and also the size.
 */
class SequenceItemDelegate : public QStyledItemDelegate {
    
    int _maxW;
    mutable bool _automaticResize;
    std::vector<std::pair<QString,std::pair<qint64,QString> > > _nameMapping;
    SequenceFileDialog* _fd;
public:
    SequenceItemDelegate(SequenceFileDialog* fd);

    void setNameMapping(std::vector<std::pair<QString,std::pair<qint64,QString> > > nameMapping);

protected:
    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const ;
};

/**
 * @brief The SequenceDialogView class is the view of the filesystem within the dialog.
 */
class SequenceDialogView: public QTreeView{
    
    Q_OBJECT
public:
    SequenceDialogView(QWidget* parent);
    
    void updateNameMapping(std::vector<std::pair<QString,std::pair<qint64,QString> > > nameMapping);

    inline void expandColumnsToFullWidth(){
        int size = width() - columnWidth(1) - columnWidth(2) - columnWidth(3);
        setColumnWidth(0,size);
    }
};

class FileSequence{
    
public:
    
    class FrameIndexes{
    public:
        FrameIndexes():_isEmpty(true),_firstFrame(-1),_lastFrame(-1),_size(0){}
        
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
    
    
    FileSequence(std::string filetype):_fileType(filetype),_totalSize(0){}
    
    ~FileSequence(){}
    
    /*Returns true if the frameIndex didn't exist when adding it*/
    void addToSequence(int frameIndex,int frameSize);
    
    
    std::string _fileType;
    FrameIndexes _frameIndexes;
    qint64 _totalSize;
};

/**
 * @brief The SequenceDialogProxyModel class is a proxy that filters image sequences from the QFileSystemModel
 */
class SequenceDialogProxyModel: public QSortFilterProxyModel{
    /*multimap of <sequence name, FileSequence >
     *Several sequences can have a same name but a different file extension within a same directory.
     */
    mutable std::multimap<std::string, FileSequence > _frameSequences;
    SequenceFileDialog* _fd;
    mutable QMutex _lock;
    QString _filter;

public:
    typedef std::multimap<std::string, FileSequence >::iterator SequenceIterator;
    typedef std::multimap<std::string, FileSequence >::const_iterator ConstSequenceIterator;
    
    SequenceDialogProxyModel(SequenceFileDialog* fd) : QSortFilterProxyModel(),_fd(fd){}
    void clear(){_frameSequences.clear();}
    
    
    std::multimap<std::string, FileSequence > getFrameSequenceCopy() const{return _frameSequences;}
    
    inline void setFilter(QString filter){ _filter = filter;}

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
private:
    /**@brief Returns in path the name of the file with the extension truncated and
     *and the frame number truncated.
     *It returns in framenumber the number of the frame
     *and in extension the name of file type
     *This function returns false if it if it couldn't detect
     *it as part of a sequence or if the file extension is not supported by the filters
     *of the file dialog.
     **/
    bool parseFilename(QString &path, int* frameNumber, QString &extension) const;
    
    /*
    *Check if the path is accepted by the filter installed by the user
    */
    bool isAcceptedByUser(const QString& path) const;
};


class FileDialogComboBox : public QComboBox
{
public:
    FileDialogComboBox(QWidget *parent = 0) : QComboBox(parent), urlModel(0) {}
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
    typedef std::multimap<std::string, FileSequence > FrameSequences;
    typedef std::vector<std::pair<QString,std::pair<qint64,QString> > > NameMapping;
    FrameSequences _frameSequences;
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
    Button* _sequenceButton;
    QLabel* _filterLabel;
    LineEdit* _filterLineEdit;
    Button* _filterDropDown;

    
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
    
public:
    typedef SequenceDialogProxyModel::SequenceIterator SequenceIterator;
    typedef SequenceDialogProxyModel::ConstSequenceIterator ConstSequenceIterator;
    
    
    SequenceFileDialog(QWidget* parent, std::vector<std::string> filters,std::string currentDirectory = std::string());

    virtual ~SequenceFileDialog();
    
    void setRootIndex(const QModelIndex& index);

    void setFrameSequence(FrameSequences frameSequences);

    const FileSequence frameRangesForSequence(std::string sequenceName, std::string extension) const;
    
    bool isASupportedFileExtension(std::string ext) const;
    
    /*Removes the . and the extension from the filename and also
     *returns the extension as a string.*/
    static QString removeFileExtension(QString& filename);
    
    static QString removePath(const QString& str);
    
    static bool checkIfContiguous(const std::vector<int>& v);
    
    QStringList selectedFiles();
    
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
        setFrameSequence(_proxy->getFrameSequenceCopy());
        return ret;
    }
    void setHistory(const QStringList &paths);
    QStringList history() const;

    QStringList typedFiles() const;

    QString getEnvironmentVariable(const QString &string);
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
    void showFilterMenu();
    void dotStarFilterSlot();
    void starSlashFilterSlot();
    void emptyFilterSlot();
    void applyFilter(QString filter);
    void showHidden();
    void showContextMenu(const QPoint &position);
    void pathChanged(const QString &newPath);
    void autoCompleteFileName(const QString&);
    void goToDirectory(const QString&);
protected:
    virtual void keyPressEvent(QKeyEvent *e);

    virtual void resizeEvent(QResizeEvent* e);
private:
    
    void createMenuActions();
    
    /*parent in proxy indexes*/
    void itemsToSequence(const QModelIndex &parent);
    
    QModelIndex select(const QModelIndex& index);
};


#endif /* defined(__PowiterOsX__filedialog__) */
