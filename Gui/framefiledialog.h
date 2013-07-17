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

#include <QtWidgets/QStyledItemDelegate>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QDialog>
#include <QtCore/QSortFilterProxyModel>
#include <QtWidgets/QFileSystemModel>
#include <QtCore/QStringList>
#include <vector>
#include <string>
#include <iostream>

class Button;
class LineEdit;
class QCheckBox;
class QWidget;
class QLabel;
class QHBoxLayout;
class QVBoxLayout;
class QListView;
class SequenceFileDialog;
class SequenceItemDelegate : public QStyledItemDelegate {
    
    Q_OBJECT
    mutable bool _automaticResize;
    std::vector<std::pair<QString,QString> > _nameMapping;
    SequenceFileDialog* _fd;
public:
    SequenceItemDelegate(SequenceFileDialog* fd) : QStyledItemDelegate(),_automaticResize(false),_fd(fd){}
    void setNameMapping(std::vector<std::pair<QString,QString> > nameMapping){_nameMapping = nameMapping;_automaticResize = true;}
    
signals:
    void contentSizeChanged(QSize) const;
protected:
    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const ;
};

class SequenceDialogView: public QTreeView{
    
    Q_OBJECT
public:
    SequenceDialogView(QWidget* p);
    
    void updateNameMapping(std::vector<std::pair<QString,QString> > nameMapping);
    
    
    public slots:
    
    void adjustSizeToNewContent(QSize);
};


class SequenceDialogProxyModel: public QSortFilterProxyModel{
    /*map of <sequence name, pair< extension name, vector of frame numbers> >
     *Several sequences can have a same name but a different file extension within a same directory.
     */
    mutable std::multimap<std::string, std::pair<std::string,std::vector<int> > > _frameSequences;
    SequenceFileDialog* _fd;
public:
    typedef std::multimap<std::string, std::pair<std::string,std::vector<int> > >::iterator SequenceIterator;
    typedef std::multimap<std::string, std::pair<std::string,std::vector<int> > >::const_iterator ConstSequenceIterator;
    
    SequenceDialogProxyModel(SequenceFileDialog* fd) : QSortFilterProxyModel(),_fd(fd){}
    void clear(){_frameSequences.clear();}
    
    
    std::multimap<std::string, std::pair<std::string,std::vector<int> > > getFrameSequenceCopy() const{return _frameSequences;}
    
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
    
};


class SequenceFileDialog: public QDialog
{
    Q_OBJECT
    
    std::multimap<std::string, std::pair<std::string,std::vector<int> > > _frameSequences;
    std::vector<std::pair<QString,QString> > _nameMapping; // the item whose names must be changed
    
    std::vector<std::string> _filters;
    
    SequenceDialogView* _view;
    SequenceItemDelegate* _itemDelegate;
    SequenceDialogProxyModel* _proxy;
    QFileSystemModel* _model;
    QVBoxLayout* _mainLayout;
    QString _requestedDir;
    
    
    Button* _previousButton;
    Button* _nextButton;
    Button* _upButton;
    Button* _createDirButton;
    Button* _previewButton;
    Button* _openButton;
    Button* _cancelButton;
    Button* _addFavoriteButton;
    
    LineEdit* _selectionLineEdit;
    QLabel* _sequenceLabel;
    QCheckBox* _sequenceCheckbox;
    QLabel* _filterLabel;
    LineEdit* _filterLineEdit;
    
    QHBoxLayout* _buttonsLayout;
    QHBoxLayout* _centerLayout;
    QVBoxLayout* _favoriteLayout;
    QHBoxLayout* _selectionLayout;
    QHBoxLayout* _filterLayout;
    
    QWidget* _buttonsWidget;
    QWidget* _centerWidget;
    QWidget* _favoriteWidget;
    QWidget* _selectionWidget;
    QWidget* _filterWidget;
    
    QListView* _favoriteView;
    
    
    
public:
    SequenceFileDialog(QWidget* parent, std::vector<std::string> filters,std::string directory = std::string());
    virtual ~SequenceFileDialog();
    
    void setRootIndex(const QModelIndex& index);
    void setFrameSequence(std::multimap<std::string, std::pair<std::string,std::vector<int> > > frameSequences);
    const std::vector<int> frameRangesForSequence(std::string sequenceName, std::string extension) const;
    
    bool isASupportedFileExtension(std::string ext) const;
    
    /*Removes the . and the extension from the filename and also
     *returns the extension as a string.*/
    static QString removeFileExtension(QString& filename);
    
    static QString removePath(const QString& str);
    
    static bool checkIfContiguous(const std::vector<int>& v);
    
    
    public slots:
    void enterDirectory(const QModelIndex& index);
    void setDirectory(const QString &directory);
    void updateView(const QString& directory);
    
private:
    
    QModelIndex mapToSource(const QModelIndex& index);
    QModelIndex mapFromSource(const QModelIndex& index);
    
    void itemsToSequence(const QModelIndex &parent);
};




#endif /* defined(__PowiterOsX__filedialog__) */
