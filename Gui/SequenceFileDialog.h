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
#include <set>

#include <boost/shared_ptr.hpp>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
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
CLANG_DIAG_ON(deprecated)

#include "Global/Macros.h"
#include "Global/QtCompat.h"

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
namespace SequenceParsing {
class SequenceFromFiles;
}
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
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) OVERRIDE FINAL;

    virtual Qt::ItemFlags flags(const QModelIndex &index) const OVERRIDE FINAL;
    
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

private:
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

private:
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
class SequenceDialogView: public QTreeView {
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


/**
 * @brief The SequenceDialogProxyModel class is a proxy that filters image sequences from the QFileSystemModel
 */
class SequenceDialogProxyModel: public QSortFilterProxyModel{
    /*multimap of <sequence name, FileSequence >
     *Several sequences can have a same name but a different file extension within a same directory.
     */
    mutable QMutex _frameSequencesMutex; // protects _frameSequences
    mutable std::vector< boost::shared_ptr<SequenceParsing::SequenceFromFiles> > _frameSequences;
    SequenceFileDialog* _fd;
    QString _filter;

public:

    explicit SequenceDialogProxyModel(SequenceFileDialog* fd) : QSortFilterProxyModel(),_fd(fd){}

    virtual ~SequenceDialogProxyModel(){
        clear();
    }
    
    QString getUserFriendlyFileSequencePatternForFile(const QString& filename,quint64* sequenceSize) const;
    
    QStringList getSequenceFilesForFile(const QString& file) const;
    
    void getSequenceFromFilesForFole(const QString& file,SequenceParsing::SequenceFromFiles* sequence) const;

    void clear(){_frameSequences.clear();}
    
    
    inline void setFilter(QString filter){ _filter = filter;}
    
private:
    
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
    enum FileDialogMode{OPEN_DIALOG = 0,SAVE_DIALOG = 1,DIR_DIALOG = 2} ;

    typedef std::pair<QString,std::pair<qint64,QString> > NameMappingElement;
    typedef std::vector<NameMappingElement> NameMapping;

public:

    
    SequenceFileDialog(QWidget* parent, // necessary to transmit the stylesheet to the dialog
                       const std::vector<std::string>& filters, // the user accepted file types. Empty means it supports everything
                       bool isSequenceDialog = true, // true if this dialog can display sequences
                       FileDialogMode mode = OPEN_DIALOG, // if it is an open or save dialog
                       const std::string& currentDirectory = ""); // the directory to show first

    virtual ~SequenceFileDialog();
    
    ///set the view to show this model index which is a directory
    void setRootIndex(const QModelIndex& index);
    
    ///Returns true if ext belongs to the user filters
    bool isASupportedFileExtension(const std::string& ext) const;
    
    ///Returns the same as SequenceParsing::removePath excepts that str is left untouched.
    static QString getFilePath(const QString& str);
    
    ///Returns the selected sequence.
    ///Works only in OPEN_DIALOG mode.
    QStringList selectedFiles();
    
    ///Same as selectedFiles() but more convenient when the dialog is in sequence mode.
    SequenceParsing::SequenceFromFiles getSelectedFilesAsSequence();
    
    ///Returns  the content of the selection line edit.
    ///Works only in SAVE_DIALOG mode.
    QString filesToSave();
    
    ///Returns the current directory of the dialog.
    ///This can be used for a DIR_DIALOG to retrieve the value selected by the user.
    QDir currentDirectory() const;

    void addFavorite(const QString& name,const QString& path);

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
        _proxy->invalidate();
        return _proxy->mapFromSource(index);
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

    ///same as setDirectory but with a QModelIndex
    void enterDirectory(const QModelIndex& index);

    ///enters a directory and display its content in the file view.
    void setDirectory(const QString &currentDirectory);
    
    ///same as setDirectory but with an url
    void seekUrl(const QUrl& url);
    
    ///same as setDirectory but for the look-in combobox
    void goToDirectory(const QString&);

    ///slot called when the selected directory changed, it updates the view with the (not yet fetched) directory.
    ///Note that the directory will be refreshed once again when fetchSequencesAndRefreshView will be called.
    void updateView(const QString& currentDirectory);
    
    ///slot called when a directory has been fully loaded, it will refresh the view with good names.
    void fetchSequencesAndRefreshView(const QString& currentDirectory);
    
    ////////
    ///////// Buttons slots
    void previousFolder();
    void nextFolder();
    void parentFolder();
    void goHome();
    void createDir();
    void addFavorite();
    //////////////////////////
    
    ///Slot called when the user pressed the "Open" or "Save" button.
    void openSelectedFiles();
    
    ///Slot called when the user pressed the "Open" button in DIR_DIALOG mode
    void selectDirectory();
    
    ///Cancel button slot
    void cancelSlot();
    
    ///Double click on a directory or file. It will select the files if clicked
    ///on a file, or open the directory otherwise.
    void doubleClickOpen(const QModelIndex& index);
    
    ///slot called when the user selection changed
    void selectionChanged();
    
    ///slot called when the sequence mode has changed
    void enableSequenceMode(bool);
    
    ///combobox slot, it calls enableSequenceMode
    void sequenceComboBoxSlot(int index);
    
    ///slot called when the filter  is clicked
    void showFilterMenu();
    
    ///apply a filter
    ///and refreshes the current directory.
    void defaultFiltersSlot();
    void dotStarFilterSlot();
    void starSlashFilterSlot();
    void emptyFilterSlot();
    void applyFilter(QString filter);
    
    ///show hidden files slot
    void showHidden();
    
    ///right click menu
    void showContextMenu(const QPoint &position);
    
    ///updates the history and up/previous buttons
    void pathChanged(const QString &newPath);
    
    ///when the user types, this function tries to automatically select  corresponding
    void autoCompleteFileName(const QString&);
    
    ///if it is a SAVE_DIALOG then it will append the file extension to what the user typed in
    ///when editing is finished.
    void onSelectionLineEditing(const QString&);
    
    ///slot called when the file extension combobox changed
    void onFileExtensionComboChanged(int index);
    
    ///called by onFileExtensionComboChanged 
    void setFileExtensionOnLineEdit(const QString&);

private:
    
    virtual void keyPressEvent(QKeyEvent *e) OVERRIDE FINAL;

    virtual void resizeEvent(QResizeEvent* e) OVERRIDE FINAL;

    void createMenuActions();
    
    /*parent in proxy indexes*/
    void itemsToSequence(const QModelIndex &parent);
    
    QModelIndex select(const QModelIndex& index);
    
    QString generateStringFromFilters();
    
    QByteArray saveState() const;
    
    bool restoreState(const QByteArray& state);

private:
    // FIXME: PIMPL
    
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

private:
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
