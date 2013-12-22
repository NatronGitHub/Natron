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

#include <QtCore/QString>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"

#include "Gui/KnobGui.h"

// Qt
class QLabel;
class QGridLayout;

// Engine
class Knob;
class Variant;

// Gui
class LineEdit;
class DockablePanel;
class Button;

//================================
class File_KnobGui: public KnobGui
{
    Q_OBJECT

public:

    static KnobGui *BuildKnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container) {
        return new File_KnobGui(knob, container);
    }

    File_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container);

    virtual ~File_KnobGui() OVERRIDE;

public slots:
    void open_file();

    void onReturnPressed();

private:
    virtual void createWidget(QGridLayout *layout, int row) OVERRIDE FINAL;

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL {}

    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;

    void updateLastOpened(const QString &str);

private:
    LineEdit *_lineEdit;
    QLabel *_descriptionLabel;
    Button *_openFileButton;
    QString _lastOpened;
};


//================================
class OutputFile_KnobGui : public KnobGui
{
    Q_OBJECT
public:

    static KnobGui *BuildKnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container) {
        return new OutputFile_KnobGui(knob, container);
    }

    OutputFile_KnobGui(boost::shared_ptr<Knob> knob, DockablePanel *container);

    virtual ~OutputFile_KnobGui() OVERRIDE;

public slots:

    void open_file();
    void onReturnPressed();

private:
    virtual void createWidget(QGridLayout *layout, int row) OVERRIDE FINAL;

    virtual void _hide() OVERRIDE FINAL;

    virtual void _show() OVERRIDE FINAL;

    virtual void setEnabled() OVERRIDE FINAL {}

    virtual void updateGUI(int dimension, const Variant &variant) OVERRIDE FINAL;

    void updateLastOpened(const QString &str);

private:
    LineEdit *_lineEdit;
    QLabel *_descriptionLabel;
    Button *_openFileButton;
    QString _lastOpened;
};

#endif // NATRON_GUI_KNOBGUIFILE_H_

