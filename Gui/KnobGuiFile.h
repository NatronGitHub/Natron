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

#ifndef NATRON_GUI_KNOBGUIFILE_H_
#define NATRON_GUI_KNOBGUIFILE_H_

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QString>
CLANG_DIAG_ON(deprecated)

#include "Global/GlobalDefines.h"

#include "Gui/KnobGui.h"

// Qt
class QHBoxLayout;

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

//================================
class File_KnobGui: public KnobGui
{
    Q_OBJECT

    friend class File_Knob_UndoCommand;
public:

    static KnobGui *BuildKnobGui(boost::shared_ptr<KnobI> knob, DockablePanel *container) {
        return new File_KnobGui(knob, container);
    }

    File_KnobGui(boost::shared_ptr<KnobI> knob, DockablePanel *container);

    virtual ~File_KnobGui() OVERRIDE;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;
public slots:

    void onReturnPressed();
    
    void onButtonClicked();
    
    void open_file(bool);

private:
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
    boost::shared_ptr<File_Knob> _knob;
};


//================================
class OutputFile_KnobGui : public KnobGui
{
    Q_OBJECT
public:

    static KnobGui *BuildKnobGui(boost::shared_ptr<KnobI> knob, DockablePanel *container) {
        return new OutputFile_KnobGui(knob, container);
    }

    OutputFile_KnobGui(boost::shared_ptr<KnobI> knob, DockablePanel *container);

    virtual ~OutputFile_KnobGui() OVERRIDE;
    
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public slots:

    void onReturnPressed();
    
    void onButtonClicked();

    void open_file(bool);
    
private:
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

class Path_KnobGui : public KnobGui
{
    Q_OBJECT
public:
    
    static KnobGui *BuildKnobGui(boost::shared_ptr<KnobI> knob, DockablePanel *container) {
        return new Path_KnobGui(knob, container);
    }
    
    Path_KnobGui(boost::shared_ptr<KnobI> knob, DockablePanel *container);
    
    virtual ~Path_KnobGui() OVERRIDE;
    
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;
public slots:
    
    
    void onReturnPressed();

    void onButtonClicked();
    
    void open_file();
    
private:
    
    
    virtual void createWidget(QHBoxLayout *layout) OVERRIDE FINAL;
    
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
    boost::shared_ptr<Path_Knob> _knob;
};

#endif // NATRON_GUI_KNOBGUIFILE_H_

