/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef PROJECTGUI_H
#define PROJECTGUI_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Format.h"

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

class ProjectGui
    : public QObject
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    ProjectGui(Gui* gui);
    virtual ~ProjectGui() OVERRIDE;

    void create(ProjectPtr projectInternal, QVBoxLayout* container, QWidget* parent = NULL);


    bool isVisible() const;

    DockablePanel* getPanel() const
    {
        return _panel;
    }

    ProjectPtr getInternalProject() const
    {
        return _project.lock();
    }

    template<class Archive>
    void save(Archive & ar) const;

    template<class Archive>
    void load(bool isAutosave, Archive & ar);

    void registerNewColorPicker(KnobColorPtr knob);

    void removeColorPicker(KnobColorPtr knob);

    void clearColorPickers();

    bool hasPickers() const
    {
        return !_colorPickersEnabled.empty();
    }

    void setPickersColor(double r, double g, double b, double a);

    /**
     * @brief Return
     **/
    NodesGuiList getVisibleNodes() const;
    Gui* getGui() const
    {
        return _gui;
    }

public Q_SLOTS:

    void createNewFormat();

    void setVisible(bool visible);

    void initializeKnobsGui();

private:


    Gui* _gui;
    ProjectWPtr _project;
    DockablePanel* _panel;
    bool _created;
    std::vector<KnobColorPtr> _colorPickersEnabled;
};


class AddFormatDialog
    : public QDialog
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    AddFormatDialog(Project* project,
                    Gui* gui);

    virtual ~AddFormatDialog()
    {
    }

    Format getFormat() const;

public Q_SLOTS:

    void onCopyFromViewer();

private:

    Gui* _gui;
    std::list<ViewerInstance*> _viewers;
    QVBoxLayout* _mainLayout;
    QWidget* _fromViewerLine;
    QHBoxLayout* _fromViewerLineLayout;
    Button* _copyFromViewerButton;
    ComboBox* _copyFromViewerCombo;
    QWidget* _parametersLine;
    QHBoxLayout* _parametersLineLayout;
    Label* _widthLabel;
    SpinBox* _widthSpinBox;
    Label* _heightLabel;
    SpinBox* _heightSpinBox;
    Label* _pixelAspectLabel;
    SpinBox* _pixelAspectSpinBox;
    QWidget* _formatNameLine;
    QHBoxLayout* _formatNameLayout;
    Label* _nameLabel;
    LineEdit* _nameLineEdit;
    DialogButtonBox* _buttonBox;
};

NATRON_NAMESPACE_EXIT


#endif // PROJECTGUI_H
