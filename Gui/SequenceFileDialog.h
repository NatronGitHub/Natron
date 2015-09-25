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

#ifndef NATRON_GUI_SEQUENCEFILEDIALOG_H
#define NATRON_GUI_SEQUENCEFILEDIALOG_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <vector>
#include <string>
#include <map>
#include <utility>
#include <set>
#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QDialog>
#include <QtCore/QByteArray>
#include <QtGui/QStandardItemModel>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QDir>
#include <QtCore/QUrl>
#include <QtCore/QRegExp>
#include <QtCore/QLatin1Char>
#include <QComboBox>
#include <QListView>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Macros.h"
#include "Global/QtCompat.h"
#include "Engine/FileSystemModel.h"


class LineEdit;
class Button;
class QCheckBox;
class ComboBox;
class QWidget;
namespace Natron {
class Label;
}
class QFileInfo;
class QHBoxLayout;
class QVBoxLayout;
class QSplitter;
class QAction;
class SequenceFileDialog;
class QFileSystemModel;
class SequenceItemDelegate;
class Gui;
class NodeGui;
namespace SequenceParsing {
class SequenceFromFiles;
}
struct FileDialogPreviewProvider;




/**
 * @brief The UrlModel class is the model used by the favorite view in the file dialog. It serves as a connexion between
 * the file system and some urls.
 */
class UrlModel
    : public QStandardItemModel
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    enum Roles
    {
        UrlRole = Qt::UserRole + 1,
        EnabledRole = Qt::UserRole + 2
    };

    explicit UrlModel(const std::map<std::string,std::string>& envVars, QObject *parent = 0);

    QStringList mimeTypes() const OVERRIDE;
    virtual QMimeData * mimeData(const QModelIndexList &indexes) const OVERRIDE;
    bool canDrop(QDragEnterEvent* e);
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) OVERRIDE FINAL;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const OVERRIDE FINAL;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) OVERRIDE;


    void setUrls(const std::vector<QUrl> &urls);
    void addUrls(const std::vector<QUrl> &urls, int row = -1,bool removeExisting = false);
    std::vector<QUrl> urls() const;
    void setFileSystemModel(QFileSystemModel *model);
    QFileSystemModel* getFileSystemModel() const
    {
        return fileSystemModel;
    }

    void setUrl(const QModelIndex &index, const QUrl &url, const QModelIndex &dirIndex);

    int getNUrls() const {
        return watching.size();
    }
    
    void removeRowIndex(const QModelIndex& index);

public Q_SLOTS:
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void layoutChanged();

private:
    void changed(const QString &path);
    void addIndexToWatch(const QString &path, const QModelIndex &index);
    
    QString mapUrlToDisplayName(const QString& originalName);

    QFileSystemModel *fileSystemModel;
    std::vector<std::pair<QModelIndex, QString> > watching;
    std::vector<QUrl> invalidUrls;
	std::map<std::string,std::string> envVars;
};

class FavoriteItemDelegate
    : public QStyledItemDelegate
{
    

public:
    FavoriteItemDelegate();

private:
    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;

	
};

/**
 * @brief The FavoriteView class is the favorite list seen in the file dialog.
 */
class FavoriteView
    : public QListView
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

Q_SIGNALS:
    void urlRequested(const QUrl &url);

public:
    explicit FavoriteView(Gui* gui,QWidget *parent = 0);
    void setModelAndUrls(QFileSystemModel *model, const std::vector<QUrl> &newUrls);
    ~FavoriteView();

    virtual QSize sizeHint() const OVERRIDE FINAL;

    void setUrls(const std::vector<QUrl> &list)
    {
        urlModel->setUrls(list);
    }

    void addUrls(const std::vector<QUrl> &list,
                 int row)
    {
        urlModel->addUrls(list, row);
    }
    
    int getNUrls() const {
        return urlModel->getNUrls();
    }

    std::vector<QUrl> urls() const
    {
        return urlModel->urls();
    }

    void selectUrl(const QUrl &url);

    void rename(const QModelIndex & index,const QString & name);

    
public Q_SLOTS:
    void clicked(const QModelIndex &index);
    void showMenu(const QPoint &position);
    void removeEntry();
    void rename();
    void editUrl();

private:
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void dragEnterEvent(QDragEnterEvent* e) OVERRIDE FINAL;
    virtual void focusInEvent(QFocusEvent* e) OVERRIDE FINAL
    {
        QAbstractScrollArea::focusInEvent(e);

        viewport()->update();
    }

private:
    Gui* _gui;
    UrlModel *urlModel;
    FavoriteItemDelegate *_itemDelegate;
};


/**
 * @brief The SequenceDialogView class is the view of the filesystem within the dialog.
 */
class SequenceDialogView
    : public QTreeView
{
    SequenceFileDialog* _fd;

public:
    explicit SequenceDialogView(SequenceFileDialog* fd);

    void updateNameMapping(const std::vector<std::pair<QString,std::pair<qint64,QString> > > & nameMapping);

    void expandColumnsToFullWidth(int w);

    void dropEvent(QDropEvent* e) OVERRIDE FINAL;

    virtual void dragEnterEvent(QDragEnterEvent* e) OVERRIDE FINAL;
    virtual void dragMoveEvent(QDragMoveEvent* e) OVERRIDE FINAL;
    virtual void dragLeaveEvent(QDragLeaveEvent* e) OVERRIDE FINAL;
    virtual void resizeEvent(QResizeEvent* e) OVERRIDE FINAL;
    virtual void paintEvent(QPaintEvent* e) OVERRIDE FINAL;
};


class FileDialogComboBox
    : public QComboBox
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    
    
    FileDialogComboBox(SequenceFileDialog *p,QWidget *parent = 0);
    
    void showPopup() OVERRIDE;
    void setHistory(const QStringList &paths);
    QStringList history() const
    {
        return m_history;
    }

    
    
public Q_SLOTS:
    
    void onCurrentIndexChanged(int index);
private:
    
    virtual void paintEvent(QPaintEvent* e) OVERRIDE FINAL;
    virtual QSize sizeHint() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    UrlModel *urlModel;
    SequenceFileDialog *dialog;
    QStringList m_history;
    bool doResize;
};

/**
 * @brief The SequenceFileDialog class is the main dialog, containing the GUI and with whom the end user can interact.
 */
class SequenceFileDialog
: public QDialog, public SortableViewI
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    enum FileDialogModeEnum
    {
        eFileDialogModeOpen,
        eFileDialogModeSave,
        eFileDialogModeDir,
    };


public:
    


    SequenceFileDialog(QWidget* parent, // necessary to transmit the stylesheet to the dialog
                       const std::vector<std::string> & filters, // the user accepted file types. Empty means it supports everything
                       bool isSequenceDialog, // true if this dialog can display sequences
                       FileDialogModeEnum mode, // if it is an open or save dialog
                       const std::string & currentDirectory,  // the directory to show first
                       Gui* gui,
                       bool allowRelativePaths);

    virtual ~SequenceFileDialog();

    ///set the view to show this model index which is a directory
    void setRootIndex(const QModelIndex & index);

    ///Returns the same as SequenceParsing::removePath excepts that str is left untouched.
    static QString getFilePath(const QString & str);

    ///Returns the selected pattern sequence or file name.
    ///Works only in eFileDialogModeOpen mode.
    std::string selectedFiles();

    ///Returns  the content of the selection line edit.
    ///Works only in eFileDialogModeSave mode.
    std::string filesToSave();

    ///Returns the path of the directory returned by currentDirectory() but whose path has been made
    ///relative to the selected user project path.
    std::string selectedDirectory() const;
    
    ///Returns the current directory of the dialog.
    ///This can be used for a eFileDialogModeDir to retrieve the value selected by the user.
    QDir currentDirectory() const;

    void addFavorite(const QString & name,const QString & path);

    bool sequenceModeEnabled() const;

    bool isDirectory(const QString & name) const;

    inline QString rootPath() const;
    
    QFileSystemModel* getFavoriteSystemModel() const;
    
    QFileSystemModel* getLookingFileSystemModel() const;

    FileSystemModel* getFileSystemModel() const
    {
        return _model.get();
    }

    SequenceDialogView* getSequenceView() const
    {
        return _view;
    }


    static inline QString toInternal(const QString &path)
    {
#if defined(Q_OS_WIN)
        QString n(path);
        n.replace( QLatin1Char('\\'),QLatin1Char('/') );
#if defined(Q_OS_WINCE)
        if ( (n.size() > 1) && n.startsWith( QLatin1String("//") ) ) {
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

    FileDialogModeEnum getDialogMode() const
    {
        return _dialogMode;
    }


    /**
     * @brief Append all files in the current directory and all its sub-directories recursively.
     **/
    static void appendFilesFromDirRecursively(QDir* currentDir,QStringList* files);

    static std::vector< boost::shared_ptr<SequenceParsing::SequenceFromFiles> >
    fileSequencesFromFilesList(const QStringList & files,const QStringList & supportedFileTypes);

    /**
     * @brief Get the user preference regarding how the file should be fetched.
     * @returns False if the file path should be absolute. When true the varName and varValue
     * will be set to the project path desired.
     **/
    bool getRelativeChoiceProjectPath(std::string& varName,std::string& varValue) const;
    
    /**
     * @brief Returns the order for the sort indicator. If no section has a sort indicator the return value of this function is undefined.
     **/
    virtual Qt::SortOrder sortIndicatorOrder() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    /**
     * @brief Returns the logical index of the section that has a sort indicator. By default this is section 0.
     **/
    virtual int	sortIndicatorSection() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    /**
     * @brief Called when the section containing the sort indicator or the order indicated is changed.
     * The section's logical index is specified by logicalIndex and the sort order is specified by order.
     **/
    virtual void onSortIndicatorChanged(int logicalIndex,Qt::SortOrder order) OVERRIDE FINAL;
    
public Q_SLOTS:

    ///same as setDirectory but with a QModelIndex
    void enterDirectory(const QModelIndex & index);

    ///enters a directory and display its content in the file view.
    void setDirectory(const QString &currentDirectory);

    ///same as setDirectory but with an url
    void seekUrl(const QUrl & url);

    ///same as setDirectory but for the look-in combobox
    void onLookingComboboxChanged(const QString &);

    ///slot called when the selected directory changed, it updates the view with the (not yet fetched) directory.
    void updateView(const QString & currentDirectory);
    
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

    ///Slot called when the user pressed the "Open" button in eFileDialogModeDir mode
    void selectDirectory();

    ///Cancel button slot
    void cancelSlot();

    ///Double click on a directory or file. It will select the files if clicked
    ///on a file, or open the directory otherwise.
    void doubleClickOpen(const QModelIndex & index);

    ///slot called when the user selection changed
    void onSelectionChanged();

    ///slot called when the sequence mode has changed
    void enableSequenceMode(bool);

    ///combobox slot, it calls enableSequenceMode
    void sequenceComboBoxSlot(int index);
    
    
    void onRelativeChoiceChanged(int index);

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
    void autoCompleteFileName(const QString &);

    ///if it is a eFileDialogModeSave then it will append the file extension to what the user typed in
    ///when editing is finished.
    void onSelectionLineEditing(const QString &);

    ///slot called when the file extension combobox changed
    void onFileExtensionComboChanged(int index);

    ///called by onFileExtensionComboChanged
    void setFileExtensionOnLineEdit(const QString &);
    
    void onTogglePreviewButtonClicked(bool toggled);

    void onHeaderViewSortIndicatorChanged(int logicalIndex,Qt::SortOrder order);
    
    virtual void done(int r) OVERRIDE FINAL;
private:

    /**
     * @brief Tries to find if text starts with a project path and if so replaces it,
     * The line edit text will be set to the resulting text
     **/
    void proxyAndSetLineEditText(const QString& text);
    
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void resizeEvent(QResizeEvent* e) OVERRIDE FINAL;
    virtual void closeEvent(QCloseEvent* e) OVERRIDE FINAL;
    
    void createMenuActions();

    QModelIndex select(const QModelIndex & index);

    QByteArray saveState() const;

    bool restoreState(const QByteArray & state, bool restoreDirectory);

    void createViewerPreviewNode();
    
    void teardownPreview();
    
    boost::shared_ptr<NodeGui> findOrCreatePreviewReader(const std::string& filetype);
    
    void refreshPreviewAfterSelectionChange();
    
    
    QString getUserFriendlyFileSequencePatternForFile(const QString & filename,quint64* sequenceSize) const;
    
    void getSequenceFromFilesForFole(const QString & file,SequenceParsing::SequenceFromFiles* sequence) const;
    
    
private:
    // FIXME: PIMPL

    QStringList _filters;
    SequenceDialogView* _view;
    boost::scoped_ptr<SequenceItemDelegate> _itemDelegate;
    boost::scoped_ptr<FileSystemModel> _model;
    
    ///The favorite view and the dialog view don't share the same model as they don't have
    ///the same icon provider
    boost::scoped_ptr<QFileSystemModel> _favoriteViewModel;
    boost::scoped_ptr<QFileSystemModel> _lookinViewModel;
    QVBoxLayout* _mainLayout;
    QString _requestedDir;
    Natron::Label* _lookInLabel;
    FileDialogComboBox* _lookInCombobox;
    Button* _previousButton;
    Button* _nextButton;
    Button* _upButton;
    Button* _createDirButton;
    Button* _openButton;
    Button* _cancelButton;
    Button* _addFavoriteButton;
    Button* _removeFavoriteButton;
    LineEdit* _selectionLineEdit;
    Natron::Label* _relativeLabel;
    ComboBox* _relativeChoice;
    ComboBox* _sequenceButton;
    Natron::Label* _filterLabel;
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
    FileDialogModeEnum _dialogMode;
    QWidget* _centerArea;
    QHBoxLayout* _centerAreaLayout;
    Button* _togglePreviewButton;
    
   
    
    boost::shared_ptr<FileDialogPreviewProvider> _preview;
    
    ///Remember  autoSetProjectFormat  state before opening the dialog
    bool _wasAutosetProjectFormatEnabled;
    
    Gui* _gui;
    
    bool _relativePathsAllowed;
    
};

/**
 * @brief The SequenceItemDelegate class is used to alterate the rendering of the cells in the filesystem view
 * within the file dialog. Mainly it transforms the text to draw for an item and also the size.
 */
class SequenceItemDelegate
    : public QStyledItemDelegate
{
    SequenceFileDialog* _fd;

public:
    explicit SequenceItemDelegate(SequenceFileDialog* fd);

private:
    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const;
};


class AddFavoriteDialog
    : public QDialog
{
    Q_OBJECT

    QVBoxLayout* _mainLayout;
    Natron::Label* _descriptionLabel;
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

    AddFavoriteDialog(SequenceFileDialog* fd,
                      QWidget* parent = 0);


    void setLabelText(const QString & text);

    QString textValue() const;

    virtual ~AddFavoriteDialog()
    {
    }

public Q_SLOTS:

    void openDir();
};

#endif /* defined(NATRON_GUI_SEQUENCEFILEDIALOG_H_) */
