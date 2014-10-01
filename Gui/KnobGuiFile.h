//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_GUI_KNOBGUIFILE_H_
#define NATRON_GUI_KNOBGUIFILE_H_
#include <map>
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QString>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Gui/KnobGui.h"

// Qt
class QHBoxLayout;
class QMenu;
class QFileSystemWatcher;

// Engine
class KnobI;
class File_Knob;
class OutputFile_Knob;
class Path_Knob;

// Gui
class LineEdit;
class DockablePanel;
class Button;
class File_Knob_UndoCommand;
class TableView;
class TableModel;
class TableItem;
//================================
class File_KnobGui
    : public KnobGui
{
    Q_OBJECT

    friend class File_Knob_UndoCommand;

public:

    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new File_KnobGui(knob, container);
    }

    File_KnobGui(boost::shared_ptr<KnobI> knob,
                 DockablePanel *container);

    virtual ~File_KnobGui() OVERRIDE;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public slots:

    void onTextEdited() ;
    
    void onButtonClicked();

    void open_file();

    void onTimelineFrameChanged(SequenceTime time,int reason);
    
    void watchedFileChanged();
    
    void onMakeAbsoluteTriggered();
    
    void onMakeRelativeTriggered();
    
    void onSimplifyTriggered();
private:
    
    virtual void addRightClickMenuEntries(QMenu* menu) OVERRIDE FINAL;

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;

private:

    void updateLastOpened(const QString &str);

    LineEdit *_lineEdit;
    Button *_openFileButton;
    QString _lastOpened;
    QFileSystemWatcher* _watcher;
    std::string _fileBeingWatched;
    boost::shared_ptr<File_Knob> _knob;
};


//================================
class OutputFile_KnobGui
    : public KnobGui
{
    Q_OBJECT

public:

    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new OutputFile_KnobGui(knob, container);
    }

    OutputFile_KnobGui(boost::shared_ptr<KnobI> knob,
                       DockablePanel *container);

    virtual ~OutputFile_KnobGui() OVERRIDE;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public slots:

    void onTextEdited() ;

    void onButtonClicked();

    void open_file(bool);

    void onMakeAbsoluteTriggered();
    
    void onMakeRelativeTriggered();
    
    void onSimplifyTriggered();

private:
    
    virtual void addRightClickMenuEntries(QMenu* menu) OVERRIDE FINAL;
    
    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;

    void updateLastOpened(const QString &str);

private:
    LineEdit *_lineEdit;
    Button *_openFileButton;
    QString _lastOpened;
    boost::shared_ptr<OutputFile_Knob> _knob;
};


//================================

class Path_KnobGui
    : public KnobGui
{
    Q_OBJECT

public:

    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new Path_KnobGui(knob, container);
    }

    Path_KnobGui(boost::shared_ptr<KnobI> knob,
                 DockablePanel *container);

    virtual ~Path_KnobGui() OVERRIDE;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public slots:



    void onAddButtonClicked();
    
    void onRemoveButtonClicked();
    
    void onOpenFileButtonClicked();
    
    void onItemDataChanged(TableItem* item);
    
    void onTextEdited();

    void onMakeAbsoluteTriggered();
    
    void onMakeRelativeTriggered();
    
    void onSimplifyTriggered();

private:
    virtual void addRightClickMenuEntries(QMenu* menu) OVERRIDE FINAL;

    virtual void createWidget(QHBoxLayout *layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;

    void updateLastOpened(const QString &str);

    /**
     * @brief A Path knob could also be called Environment_variable_Knob.
     * The string is encoded the following way:
     * [VariableName1]:[Value1];[VariableName2]:[Value2] etc...
     * Split all the ';' characters to get all different variables
     * then for each variable split the ':' to get the name and the value of the variable.
     **/
    std::string rebuildPath() const;
    
    void createItem(int row,const QString& value,const QString& varName);
    
    struct Row
    {
        TableItem* varName;
        TableItem* value;
    };
    typedef std::map<int,Row> Variables;
    
 
    QWidget* _mainContainer;
    
    LineEdit* _lineEdit;
    Button* _openFileButton;
    
    
    TableView *_table;
    TableModel* _model;
    Button *_addPathButton;
    Button* _removePathButton;
    QString _lastOpened;
    boost::shared_ptr<Path_Knob> _knob;
    bool _isInsertingItem;
    Variables _items;
};

#endif // NATRON_GUI_KNOBGUIFILE_H_

