/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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


class ProjectGui
    : public QObject
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    ProjectGui(Gui* gui);
    virtual ~ProjectGui() OVERRIDE;

    void create(boost::shared_ptr<Natron::Project> projectInternal,QVBoxLayout* container,QWidget* parent = NULL);


    bool isVisible() const;

    DockablePanel* getPanel() const
    {
        return _panel;
    }

    boost::shared_ptr<Natron::Project> getInternalProject() const
    {
        return _project.lock();
    }

    template<class Archive>
    void save(Archive & ar/*,
              const unsigned int version*/) const;

    template<class Archive>
    void load(Archive & ar/*,
              const unsigned int version*/);

    void registerNewColorPicker(boost::shared_ptr<KnobColor> knob);

    void removeColorPicker(boost::shared_ptr<KnobColor> knob);
    
    void clearColorPickers();

    bool hasPickers() const
    {
        return !_colorPickersEnabled.empty();
    }

    void setPickersColor(double r,double g, double b,double a);

    /**
     * @brief Retur
     **/
    std::list<boost::shared_ptr<NodeGui> > getVisibleNodes() const;
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
    boost::weak_ptr<Natron::Project> _project;
    DockablePanel* _panel;
    bool _created;
    std::vector<boost::shared_ptr<KnobColor> > _colorPickersEnabled;
};


class AddFormatDialog
    : public QDialog
{
    Q_OBJECT

public:

    AddFormatDialog(Natron::Project* project,
                    Gui* gui);

    virtual ~AddFormatDialog()
    {
    }

    Format getFormat() const;

public Q_SLOTS:

    void onCopyFromViewer();

private:

    Gui* _gui;
    Natron::Project* _project;
    
    std::list<ViewerInstance*> _viewers;
    
    QVBoxLayout* _mainLayout;
    QWidget* _fromViewerLine;
    QHBoxLayout* _fromViewerLineLayout;
    Button* _copyFromViewerButton;
    ComboBox* _copyFromViewerCombo;
    QWidget* _parametersLine;
    QHBoxLayout* _parametersLineLayout;
    Natron::Label* _widthLabel;
    SpinBox* _widthSpinBox;
    Natron::Label* _heightLabel;
    SpinBox* _heightSpinBox;
    Natron::Label* _pixelAspectLabel;
    SpinBox* _pixelAspectSpinBox;
    QWidget* _formatNameLine;
    QHBoxLayout* _formatNameLayout;
    Natron::Label* _nameLabel;
    LineEdit* _nameLineEdit;
    QWidget* _buttonsLine;
    QHBoxLayout* _buttonsLineLayout;
    Button* _cancelButton;
    Button* _okButton;
};


#endif // PROJECTGUI_H
