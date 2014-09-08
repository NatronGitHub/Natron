//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "Gui/KnobGuiFile.h"

#include <QLabel> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QFormLayout> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QHBoxLayout> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QDebug>
#include <QItemSelectionModel>
#include <QHeaderView>
#include <QPainter>
#include <QStyle>
#include <QApplication>
#include <QStyledItemDelegate>
#include <QFileSystemWatcher>

#include "Engine/Settings.h"
#include "Engine/KnobFile.h"
#include <SequenceParsing.h>
#include "Engine/EffectInstance.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"

#include "Gui/GuiApplicationManager.h"
#include "Gui/Button.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/LineEdit.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/TableModelView.h"

using namespace Natron;

//===========================FILE_KNOB_GUI=====================================
File_KnobGui::File_KnobGui(boost::shared_ptr<KnobI> knob,
                           DockablePanel *container)
    : KnobGui(knob, container)
    , _lineEdit(0)
    , _openFileButton(0)
    , _lastOpened()
    , _watcher(0)
{
    _knob = boost::dynamic_pointer_cast<File_Knob>(knob);
    assert(_knob);
    QObject::connect( _knob.get(), SIGNAL( openFile() ), this, SLOT( open_file() ) );
}

File_KnobGui::~File_KnobGui()
{
    delete _lineEdit;
    delete _openFileButton;
    delete _watcher;
}

void
File_KnobGui::createWidget(QHBoxLayout* layout)
{
    
    if (_knob->getHolder() && _knob->getEvaluateOnChange()) {
        boost::shared_ptr<TimeLine> timeline = getGui()->getApp()->getTimeLine();
        QObject::connect(timeline.get(), SIGNAL(frameChanged(SequenceTime,int)), this, SLOT(onTimelineFrameChanged(SequenceTime, int)));
    }
    _lineEdit = new LineEdit( layout->parentWidget() );
    layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    _lineEdit->setPlaceholderText( tr("File path...") );
    _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_lineEdit, 0);


    if ( hasToolTip() ) {
        _lineEdit->setToolTip( toolTip() );
    }
    QObject::connect( _lineEdit, SIGNAL( editingFinished() ), this, SLOT( onReturnPressed() ) );

    _openFileButton = new Button( layout->parentWidget() );
    _openFileButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QPixmap pix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, &pix);
    _openFileButton->setIcon( QIcon(pix) );
    QObject::connect( _openFileButton, SIGNAL( clicked() ), this, SLOT( onButtonClicked() ) );
    QWidget *container = new QWidget( layout->parentWidget() );
    QHBoxLayout *containerLayout = new QHBoxLayout(container);
    container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(_lineEdit);
    containerLayout->addWidget(_openFileButton);

    layout->addWidget(container);
    
}

void
File_KnobGui::onButtonClicked()
{
    open_file();
}


void
File_KnobGui::open_file()
{
    std::vector<std::string> filters;

    if ( !_knob->isInputImageFile() ) {
        filters.push_back("*");
    } else {
        Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>( _knob->getHolder() );
        if (effect) {
            filters = effect->supportedFileFormats();
        }
    }
    std::string currentPattern = _knob->getValue();
    std::string path = SequenceParsing::removePath(currentPattern);
    QString pathWhereToOpen;
    if ( path.empty() ) {
        pathWhereToOpen = _lastOpened;
    } else {
        pathWhereToOpen = path.c_str();
    }

    SequenceFileDialog dialog( _lineEdit->parentWidget(), filters, _knob->isInputImageFile(),
                               SequenceFileDialog::OPEN_DIALOG, pathWhereToOpen.toStdString(), getGui(),true);
    
    if ( dialog.exec() ) {
        std::string selectedFile = dialog.selectedFiles();
        
        std::string originalSelectedFile = selectedFile;
        path = SequenceParsing::removePath(selectedFile);
        updateLastOpened( path.c_str() );
        
        pushUndoCommand( new KnobUndoCommand<std::string>(this,currentPattern,originalSelectedFile) );
    }
}

void
File_KnobGui::updateLastOpened(const QString &str)
{
    std::string unpathed = str.toStdString();

    _lastOpened = SequenceParsing::removePath(unpathed).c_str();
    getGui()->updateLastSequenceOpenedPath(_lastOpened);
}

void
File_KnobGui::updateGUI(int /*dimension*/)
{
    QString file(_knob->getValue().c_str());
    _lineEdit->setText(file);
    if (_knob->getHolder() && _knob->getEvaluateOnChange() && QFile::exists(file)) {
        if (_watcher && _watcher->files().contains(file)) {
            return;
        }
        delete _watcher;
        _watcher = new QFileSystemWatcher;
        _watcher->addPath(file);
        QObject::connect(_watcher, SIGNAL(fileChanged(QString)), this, SLOT(watchedFileChanged()));

    }
}

void
File_KnobGui::onTimelineFrameChanged(SequenceTime time,int /*reason*/)
{
    ///Get the current file, if it exists, add the file path to the file system watcher
    ///to get notified if the file changes.
    std::string filepath = _knob->getFileName(time, 0);
    QString qfilePath(filepath.c_str());
    if (!QFile::exists(qfilePath)) {
        return;
    }
    
    delete _watcher;
    _watcher = new QFileSystemWatcher;
    _watcher->addPath(qfilePath);
    QObject::connect(_watcher, SIGNAL(fileChanged(QString)), this, SLOT(watchedFileChanged()));
}

void
File_KnobGui::watchedFileChanged()
{
    ///The file has changed, trigger a new render.
    
    ///Make sure the node doesn't hold any cache
    if (_knob->getHolder()) {
        EffectInstance* effect = dynamic_cast<EffectInstance*>(_knob->getHolder());
        if (effect) {
            effect->purgeCaches();
        }
        _knob->getHolder()->evaluate_public(_knob.get(), true, Natron::USER_EDITED);
    }
    
    
}

void
File_KnobGui::onReturnPressed()
{
    QString str = _lineEdit->text();

    ///don't do antyhing if the pattern is the same
    std::string oldValue = _knob->getValue();

    if ( str == oldValue.c_str() ) {
        return;
    }
    pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,str.toStdString() ) );
}

void
File_KnobGui::_hide()
{
    _openFileButton->hide();
    _lineEdit->hide();
}

void
File_KnobGui::_show()
{
    _openFileButton->show();
    _lineEdit->show();
}

void
File_KnobGui::setEnabled()
{
    bool enabled = getKnob()->isEnabled(0);

    _openFileButton->setEnabled(enabled);
    _lineEdit->setEnabled(enabled);
}

void
File_KnobGui::setReadOnly(bool readOnly,
                          int /*dimension*/)
{
    _openFileButton->setEnabled(!readOnly);
    _lineEdit->setReadOnly(readOnly);
}

void
File_KnobGui::setDirty(bool dirty)
{
    _lineEdit->setDirty(dirty);
}

boost::shared_ptr<KnobI> File_KnobGui::getKnob() const
{
    return _knob;
}

//============================OUTPUT_FILE_KNOB_GUI====================================
OutputFile_KnobGui::OutputFile_KnobGui(boost::shared_ptr<KnobI> knob,
                                       DockablePanel *container)
    : KnobGui(knob, container)
{
    _knob = boost::dynamic_pointer_cast<OutputFile_Knob>(knob);
    assert(_knob);
    QObject::connect( _knob.get(), SIGNAL( openFile(bool) ), this, SLOT( open_file(bool) ) );
}

OutputFile_KnobGui::~OutputFile_KnobGui()
{
    delete _lineEdit;
    delete _openFileButton;
}

void
OutputFile_KnobGui::createWidget(QHBoxLayout* layout)
{
    _lineEdit = new LineEdit( layout->parentWidget() );
    layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QObject::connect( _lineEdit, SIGNAL( editingFinished() ), this, SLOT( onReturnPressed() ) );
    _lineEdit->setPlaceholderText( tr("File path...") );
    if ( hasToolTip() ) {
        _lineEdit->setToolTip( toolTip() );
    }
    _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    ///set the copy/link actions in the right click menu
    enableRightClickMenu(_lineEdit, 0);


    _openFileButton = new Button( layout->parentWidget() );
    _openFileButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
    QPixmap pix;
    appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, &pix);
    _openFileButton->setIcon( QIcon(pix) );
    QObject::connect( _openFileButton, SIGNAL( clicked() ), this, SLOT( onButtonClicked() ) );
    QWidget *container = new QWidget( layout->parentWidget() );
    QHBoxLayout *containerLayout = new QHBoxLayout(container);
    container->setLayout(containerLayout);
    containerLayout->setContentsMargins(0, 0, 0, 0);

    containerLayout->addWidget(_lineEdit);
    containerLayout->addWidget(_openFileButton);

    layout->addWidget(container);
}

void
OutputFile_KnobGui::onButtonClicked()
{
    open_file( _knob->isSequencesDialogEnabled() );
}

void
OutputFile_KnobGui::open_file(bool openSequence)
{
    std::vector<std::string> filters;

    if ( !_knob->isOutputImageFile() ) {
        filters.push_back("*");
    } else {
        Natron::EffectInstance* effect = dynamic_cast<Natron::EffectInstance*>( getKnob()->getHolder() );
        if (effect) {
            filters = effect->supportedFileFormats();
        }
    }

    SequenceFileDialog dialog( _lineEdit->parentWidget(), filters, openSequence, SequenceFileDialog::SAVE_DIALOG, _lastOpened.toStdString(), getGui(),true);
    if ( dialog.exec() ) {
        std::string oldPattern = _lineEdit->text().toStdString();
        
        std::string newPattern = dialog.filesToSave();
        updateLastOpened( SequenceParsing::removePath(oldPattern).c_str() );

        pushUndoCommand( new KnobUndoCommand<std::string>(this,oldPattern,newPattern) );
    }
}

void
OutputFile_KnobGui::updateLastOpened(const QString &str)
{
    std::string withoutPath = str.toStdString();

    _lastOpened = SequenceParsing::removePath(withoutPath).c_str();
    getGui()->updateLastSequenceSavedPath(_lastOpened);
}

void
OutputFile_KnobGui::updateGUI(int /*dimension*/)
{
    _lineEdit->setText( _knob->getValue().c_str() );
}

void
OutputFile_KnobGui::onReturnPressed()
{
    QString newPattern = _lineEdit->text();

    pushUndoCommand( new KnobUndoCommand<std::string>( this,_knob->getValue(),newPattern.toStdString() ) );
}

void
OutputFile_KnobGui::_hide()
{
    _openFileButton->hide();
    _lineEdit->hide();
}

void
OutputFile_KnobGui::_show()
{
    _openFileButton->show();
    _lineEdit->show();
}

void
OutputFile_KnobGui::setEnabled()
{
    bool enabled = getKnob()->isEnabled(0);

    _openFileButton->setEnabled(enabled);
    _lineEdit->setEnabled(enabled);
}

void
OutputFile_KnobGui::setReadOnly(bool readOnly,
                                int /*dimension*/)
{
    _openFileButton->setEnabled(!readOnly);
    _lineEdit->setReadOnly(readOnly);
}

void
OutputFile_KnobGui::setDirty(bool dirty)
{
    _lineEdit->setDirty(dirty);
}

boost::shared_ptr<KnobI> OutputFile_KnobGui::getKnob() const
{
    return _knob;
}

//============================PATH_KNOB_GUI====================================
Path_KnobGui::Path_KnobGui(boost::shared_ptr<KnobI> knob,
                           DockablePanel *container)
    : KnobGui(knob, container)
    , _mainContainer(0)
    , _isInsertingItem(false)
{
    _knob = boost::dynamic_pointer_cast<Path_Knob>(knob);
    assert(_knob);
}

Path_KnobGui::~Path_KnobGui()
{
    delete _mainContainer;
}

////////////// TableView delegate

class PathKnobTableItemDelegate
: public QStyledItemDelegate
{
    TableView* _view;
    
public:
    
    explicit PathKnobTableItemDelegate(TableView* view);
    
private:
    
    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const OVERRIDE FINAL;
};

PathKnobTableItemDelegate::PathKnobTableItemDelegate(TableView* view)
: QStyledItemDelegate(view)
, _view(view)
{
}

void
PathKnobTableItemDelegate::paint(QPainter * painter,
                         const QStyleOptionViewItem & option,
                         const QModelIndex & index) const
{
    
    if (!index.isValid() || option.state & QStyle::State_Selected) {
        QStyledItemDelegate::paint(painter,option,index);
        
        return;
    }
    TableItem* item = dynamic_cast<TableModel*>( _view->model() )->item(index);
    if (!item) {
        QStyledItemDelegate::paint(painter,option,index);
        return;
    }
    QPen pen;
    
    if (!item->flags().testFlag(Qt::ItemIsEnabled)) {
        pen.setColor(Qt::black);
    } else {
        pen.setColor( QColor(200,200,200) );
        
    }
    painter->setPen(pen);
    
    // get the proper subrect from the style
    QStyle *style = QApplication::style();
    QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &option);
    
    ///Draw the item name column
    if (option.state & QStyle::State_Selected) {
        painter->fillRect( geom, option.palette.highlight() );
    }
    QRect r;
    QString str = item->data(Qt::DisplayRole).toString();
    if (index.column() == 0) {
        ///Env vars are used between brackets
        str.prepend('[');
        str.append(']');
    }
    painter->drawText(geom,Qt::TextSingleLine,str,&r);
}


void
Path_KnobGui::createWidget(QHBoxLayout* layout)
{
    _mainContainer = new QWidget(layout->parentWidget());
    
    if (_knob->isMultiPath()) {
        QVBoxLayout* mainLayout = new QVBoxLayout(_mainContainer);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        
        _table = new TableView( _mainContainer );
        layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        //    QObject::connect( _table, SIGNAL( editingFinished() ), this, SLOT( onReturnPressed() ) );
        if ( hasToolTip() ) {
            _table->setToolTip( toolTip() );
        }
        _table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        _table->setAttribute(Qt::WA_MacShowFocusRect,0);

#if QT_VERSION < 0x050000
        _table->header()->setResizeMode(QHeaderView::ResizeToContents);
#else
        _table->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
        _table->header()->setStretchLastSection(true);
        _table->setItemDelegate(new PathKnobTableItemDelegate(_table));
        
        _model = new TableModel(0,0,_table);
        QObject::connect( _model,SIGNAL( s_itemChanged(TableItem*) ),this,SLOT( onItemDataChanged(TableItem*) ) );
        
        _table->setTableModel(_model);
        
        _table->setColumnCount(2);
        QStringList headers;
        headers << tr("Variable name") << tr("Value");
        _table->setHorizontalHeaderLabels(headers);
        
        ///set the copy/link actions in the right click menu
        enableRightClickMenu(_table, 0);
        
        QWidget* buttonsContainer = new QWidget(_mainContainer);
        QHBoxLayout* buttonsLayout = new QHBoxLayout(buttonsContainer);
        buttonsLayout->setContentsMargins(0, 0, 0, 0);
        
        _addPathButton = new Button( "+",buttonsContainer );
        _addPathButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _addPathButton->setToolTip( tr("Click to add a new project path") );
        QObject::connect( _addPathButton, SIGNAL( clicked() ), this, SLOT( onAddButtonClicked() ) );
        
        _removePathButton = new Button( "-",buttonsContainer);
        _removePathButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE,NATRON_MEDIUM_BUTTON_SIZE);
        QObject::connect( _removePathButton, SIGNAL( clicked() ), this, SLOT( onRemoveButtonClicked() ) );
        _removePathButton->setToolTip(tr("Click to remove selected project path"));
        
        
        buttonsLayout->addWidget(_addPathButton);
        buttonsLayout->addWidget(_removePathButton);
        buttonsLayout->addStretch();
        
        mainLayout->addWidget(_table);
        mainLayout->addWidget(buttonsContainer);
        
    } else { // _knob->isMultiPath()
        QHBoxLayout* mainLayout = new QHBoxLayout(_mainContainer);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        _lineEdit = new LineEdit(_mainContainer);
        _lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        if ( hasToolTip() ) {
            _lineEdit->setToolTip( toolTip() );
        }
        enableRightClickMenu(_lineEdit, 0);
        _openFileButton = new Button( layout->parentWidget() );
        _openFileButton->setFixedSize(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE);
        _openFileButton->setToolTip( tr("Click to select a path to append to/replace this variable.") );
        QPixmap pix;
        appPTR->getIcon(NATRON_PIXMAP_OPEN_FILE, &pix);
        _openFileButton->setIcon( QIcon(pix) );
        QObject::connect( _openFileButton, SIGNAL( clicked() ), this, SLOT( onOpenFileButtonClicked() ) );
        
        mainLayout->addWidget(_lineEdit);
        mainLayout->addWidget(_openFileButton);
    }
    
    layout->addWidget(_mainContainer);
}

void
Path_KnobGui::onAddButtonClicked()
{
    std::vector<std::string> filters;
    SequenceFileDialog dialog( _mainContainer, filters, false, SequenceFileDialog::DIR_DIALOG, _lastOpened.toStdString(),getGui(),true );
    
    if ( dialog.exec() ) {
        std::string dirPath = dialog.selectedDirectory();
        if (dirPath[dirPath.size() - 1] != '/') {
            dirPath += '/';
        }
        updateLastOpened(dirPath.c_str());
        
        
        std::string oldValue = _knob->getValue();
        
        int rowCount = (int)_items.size();
        
        QString varName = QString(tr("Path") + "%1").arg(rowCount);
        createItem(rowCount, dirPath.c_str(), varName);
        std::string newPath = rebuildPath();
        
       
        
        pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,newPath ) );
    }

}

void
Path_KnobGui::onOpenFileButtonClicked()
{
    std::vector<std::string> filters;
    SequenceFileDialog dialog( _mainContainer, filters, false, SequenceFileDialog::DIR_DIALOG, _lastOpened.toStdString(),getGui(),true );
    
    if ( dialog.exec() ) {
        std::string dirPath = dialog.currentDirectory().absolutePath().toStdString();
        updateLastOpened(dirPath.c_str());
        std::string varName,varPath;
        bool relative = dialog.getRelativeChoiceProjectPath(varName, varPath);
        if (relative) {
            Natron::Project::makeRelativeToVariable(varName, varPath, dirPath);
        }
        
        std::string oldValue = _knob->getValue();
        
        pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,dirPath ) );
    }

}

void
Path_KnobGui::onRemoveButtonClicked()
{
    std::string oldValue = _knob->getValue();
    QModelIndexList selection = _table->selectionModel()->selectedRows();
    _model->removeRows(selection.front().row(),selection.size());
    
    std::list<std::string> removeVars;
    for (int i = 0; i < selection.size(); ++i) {
        Variables::iterator found = _items.find(selection[i].row());
        if (found != _items.end()) {
            removeVars.push_back(found->second.varName->text().toStdString());
            _items.erase(found);
        }
    }
    
    
    ///Fix all variables if needed
    if (_knob->getHolder() && _knob->getHolder() == getGui()->getApp()->getProject().get() &&
        appPTR->getCurrentSettings()->isAutoFixRelativeFilePathEnabled()) {
        for (std::list<std::string>::iterator it = removeVars.begin(); it != removeVars.end(); ++it) {
            getGui()->getApp()->getProject()->fixRelativeFilePaths(*it, "");
        }
        
    }
    
    std::string newPath = rebuildPath();
    
    pushUndoCommand( new KnobUndoCommand<std::string>( this,oldValue,newPath ) );
}



void
Path_KnobGui::updateLastOpened(const QString &str)
{
    std::string withoutPath = str.toStdString();

    _lastOpened = SequenceParsing::removePath(withoutPath).c_str();
}

void
Path_KnobGui::updateGUI(int /*dimension*/)
{
    QString path(_knob->getValue().c_str());
    
    if (_knob->isMultiPath()) {
        QStringList variables = path.split(QChar(';'));
        
        _model->clear();
        _items.clear();
        for (int i = 0; i < variables.size(); ++i) {
            QStringList cols = variables[i].split(QChar(':'));
            if (cols.size() < 2) {
                continue;
            }
            
            createItem(i, cols[1], cols[0]);
        }
    } else {
        _lineEdit->setText(path);
    }
}

void
Path_KnobGui::createItem(int row,const QString& value,const QString& varName)
{
    
    
    Qt::ItemFlags flags;
    
    ///Project env var is disabled and uneditable and set automatically by the project
    if (varName != NATRON_PROJECT_ENV_VAR_NAME) {
        flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    }
    TableItem* cell0 = new TableItem;
    cell0->setText(varName);
    cell0->setFlags(flags);
    
    TableItem* cell1 = new TableItem;
    cell1->setText(value);
    cell1->setFlags(flags);
    
    Row r;
    r.varName = cell0;
    r.value = cell1;
    
    _items.insert(std::make_pair(row, r));
    int modelRowCount = _model->rowCount();
    if (row >= modelRowCount) {
        _model->insertRow(row);
    }
    _isInsertingItem = true;
    _table->setItem(row, 0, cell0);
    _table->setItem(row, 1, cell1);
    _isInsertingItem = false;
    _table->resizeColumnToContents(0);
    _table->resizeColumnToContents(1);
}

void
Path_KnobGui::_hide()
{
    _mainContainer->hide();
}

void
Path_KnobGui::_show()
{
    _mainContainer->show();
}

void
Path_KnobGui::setEnabled()
{
    bool enabled = getKnob()->isEnabled(0);
    if (_knob->isMultiPath()) {
        _table->setEnabled(enabled);
        _addPathButton->setEnabled(enabled);
        _removePathButton->setEnabled(enabled);
    } else {
        _lineEdit->setEnabled(enabled);
        _openFileButton->setEnabled(enabled);
    }
}

void
Path_KnobGui::setReadOnly(bool readOnly,
                          int /*dimension*/)
{
    if (_knob->isMultiPath()) {
        _table->setEnabled(!readOnly);
        _addPathButton->setEnabled(!readOnly);
        _removePathButton->setEnabled(!readOnly);
    } else {
        _lineEdit->setReadOnly(readOnly);
        _openFileButton->setEnabled(!readOnly);
    }
}

void
Path_KnobGui::setDirty(bool /*dirty*/)
{
    
}

boost::shared_ptr<KnobI> Path_KnobGui::getKnob() const
{
    return _knob;
}

void
Path_KnobGui::onItemDataChanged(TableItem* /*item*/)
{
    if (_isInsertingItem) {
        return;
    }
    
    std::string newPath = rebuildPath();
    std::string oldPath = _knob->getValue();
    
    if (oldPath != newPath) {
        
        if (_knob->getHolder() && _knob->getHolder() == _knob->getHolder()->getApp()->getProject().get() &&
            appPTR->getCurrentSettings()->isAutoFixRelativeFilePathEnabled()) {
            std::map<std::string,std::string> oldEnv,newEnv;
            
            Natron::Project::makeEnvMap(oldPath,oldEnv);
            Natron::Project::makeEnvMap(newPath, newEnv);
            
            ///Compare the 2 maps to find-out if a path has changed or just a name
            if (oldEnv.size() == newEnv.size()) {
                std::map<std::string,std::string>::iterator itOld = oldEnv.begin();
                for (std::map<std::string,std::string>::iterator itNew = newEnv.begin(); itNew != newEnv.end(); ++itNew, ++itOld) {
                    
                    if (itOld->first != itNew->first) {
                        ///a name has changed
                        getGui()->getApp()->getProject()->fixPathName(itOld->first, itNew->first);
                        break;
                    } else if (itOld->second != itOld->second) {
                        getGui()->getApp()->getProject()->fixRelativeFilePaths(itOld->first, itNew->second);
                        break;
                    }
                }
            }
        }
        
        pushUndoCommand( new KnobUndoCommand<std::string>( this,oldPath,newPath ) );
    }
}

/**
 * @brief A Path knob could also be called Environment_variable_Knob.
 * The string is encoded the following way:
 * [VariableName1]:[Value1];[VariableName2]:[Value2] etc...
 * Split all the ';' characters to get all different variables
 * then for each variable split the ':' to get the name and the value of the variable.
 **/
std::string
Path_KnobGui::rebuildPath() const
{
    std::string path;
    
    Variables::const_iterator next = _items.begin();
    if (!_items.empty()) {
        ++next;
    }
    for (Variables::const_iterator it = _items.begin(); it!= _items.end(); ++it,++next) {
            path += it->second.varName->text().toStdString();
            path += ':';
            path += it->second.value->text().toStdString();
        if (next == _items.end()) {
            --next;
        } else {
            path += ';';
        }
        
    }
    return path;
}