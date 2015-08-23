//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef PROJECTGUI_H
#define PROJECTGUI_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#pragma message WARN("move serialization to a separate header")
CLANG_DIAG_OFF(unused-local-typedef)
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
CLANG_DIAG_ON(unused-local-typedef)
GCC_DIAG_ON(unused-parameter)
#endif

#include "Engine/Format.h"

class Button;
class QWidget;
class QHBoxLayout;
class QVBoxLayout;
class ComboBox;
class SpinBox;
class LineEdit;
class Color_Knob;
class DockablePanel;
class ProjectGuiSerialization;
class Gui;
class NodeGui;
class ViewerInstance;
class NodeGuiSerialization;
namespace boost {
namespace archive {
class xml_archive;
}
}

namespace Natron {
class Project;
class Label;
}

class ProjectGui
    : public QObject
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

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

    void save(boost::archive::xml_oarchive & archive) const;

    void load(boost::archive::xml_iarchive & archive);

    void registerNewColorPicker(boost::shared_ptr<Color_Knob> knob);

    void removeColorPicker(boost::shared_ptr<Color_Knob> knob);

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
    std::vector<boost::shared_ptr<Color_Knob> > _colorPickersEnabled;
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
