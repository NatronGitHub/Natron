/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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

#ifndef _Gui_File_KnobGui_h_
#define _Gui_File_KnobGui_h_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <map>
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QString>
#include <QtCore/QDateTime>
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
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

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
    
    virtual void removeSpecificGui() OVERRIDE FINAL;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public Q_SLOTS:

    void onTextEdited() ;
    
    void onButtonClicked();
    
    void onReloadClicked();

    void open_file();

    void onTimelineFrameChanged(SequenceTime time,int reason);
    
    void watchedFileChanged();
    
    void onMakeAbsoluteTriggered();
    
    void onMakeRelativeTriggered();
    
    void onSimplifyTriggered();
private:
    
    virtual void addRightClickMenuEntries(QMenu* menu) OVERRIDE FINAL;
    virtual bool shouldAddStretch() const OVERRIDE FINAL { return false; }
    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevelEnum level) OVERRIDE FINAL;
    virtual void reflectExpressionState(int dimension,bool hasExpr) OVERRIDE FINAL;
    virtual void updateToolTip() OVERRIDE FINAL;
private:

    void updateLastOpened(const QString &str);

    LineEdit *_lineEdit;
    Button *_openFileButton;
    Button* _reloadButton;
    QString _lastOpened;
    QDateTime _lastModified;
    QFileSystemWatcher* _watcher;
    std::string _fileBeingWatched;
    boost::weak_ptr<File_Knob> _knob;
};


//================================
class OutputFile_KnobGui
    : public KnobGui
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new OutputFile_KnobGui(knob, container);
    }

    OutputFile_KnobGui(boost::shared_ptr<KnobI> knob,
                       DockablePanel *container);

    virtual ~OutputFile_KnobGui() OVERRIDE;

    virtual void removeSpecificGui() OVERRIDE FINAL;
    
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public Q_SLOTS:

    void onTextEdited() ;

    void onButtonClicked();

    void open_file(bool);

    void onMakeAbsoluteTriggered();
    
    void onMakeRelativeTriggered();
    
    void onSimplifyTriggered();

private:
    
    virtual void addRightClickMenuEntries(QMenu* menu) OVERRIDE FINAL;
    virtual bool shouldAddStretch() const OVERRIDE FINAL { return false; }
    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevelEnum level) OVERRIDE FINAL;
    virtual void reflectExpressionState(int dimension,bool hasExpr) OVERRIDE FINAL;
    virtual void updateToolTip() OVERRIDE FINAL;
    void updateLastOpened(const QString &str);

private:
    LineEdit *_lineEdit;
    Button *_openFileButton;
    QString _lastOpened;
    boost::weak_ptr<OutputFile_Knob> _knob;
};


//================================

class Path_KnobGui
    : public KnobGui
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new Path_KnobGui(knob, container);
    }

    Path_KnobGui(boost::shared_ptr<KnobI> knob,
                 DockablePanel *container);

    virtual ~Path_KnobGui() OVERRIDE;
    
    virtual void removeSpecificGui() OVERRIDE FINAL;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public Q_SLOTS:



    void onAddButtonClicked();
    
    void onRemoveButtonClicked();
    
    void onEditButtonClicked();
    
    void onOpenFileButtonClicked();
    
    void onItemDataChanged(TableItem* item);
    
    void onTextEdited();

    void onMakeAbsoluteTriggered();
    
    void onMakeRelativeTriggered();
    
    void onSimplifyTriggered();

private:
    virtual void addRightClickMenuEntries(QMenu* menu) OVERRIDE FINAL;
    virtual bool shouldAddStretch() const OVERRIDE FINAL { return false; }
    virtual void createWidget(QHBoxLayout *layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevelEnum level) OVERRIDE FINAL;
    virtual void reflectExpressionState(int dimension,bool hasExpr) OVERRIDE FINAL;
    virtual void updateToolTip() OVERRIDE FINAL;
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
    Button* _editPathButton;
    QString _lastOpened;
    boost::weak_ptr<Path_Knob> _knob;
    bool _isInsertingItem;
    Variables _items;
};

#endif // _Gui_File_KnobGui_h_

