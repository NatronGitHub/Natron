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

#ifndef Gui_KnobGuiParametric_h
#define Gui_KnobGuiParametric_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <vector> // KnobGuiInt
#include <list>
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QStyledItemDelegate>
#include <QTextEdit>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"

#include "Engine/Singleton.h"
#include "Engine/Knob.h"
#include "Engine/ImageComponents.h"

#include "Gui/CurveSelection.h"
#include "Gui/KnobGui.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/Label.h"

// Qt
class QString;
class QFrame;
class QHBoxLayout;
class QTreeWidget;
class QTreeWidgetItem;
class QScrollArea;
class QFontComboBox;

// Engine
class KnobI;
class KnobInt;
class KnobBool;
class KnobDouble;
class KnobButton;
class KnobSeparator;
class KnobGroup;
class KnobParametric;
class KnobColor;
class KnobChoice;
class KnobString;

// Gui
class DockablePanel;
class LineEdit;
class Button;
class SpinBox;
class ComboBox;
class ScaleSliderQWidget;
class CurveWidget;
class KnobCurveGui;
class TabGroup;

// private classes, defined in KnobGuiTypes.cpp
namespace Natron {
class GroupBoxLabel;
class ClickableLabel;
}
class AnimatedCheckBox;

namespace Natron {
class Node;
}

class KnobGuiParametric
    : public KnobGui
    , public CurveSelection
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new KnobGuiParametric(knob, container);
    }

    KnobGuiParametric(boost::shared_ptr<KnobI> knob,
                       DockablePanel *container);
    
    virtual void removeSpecificGui() OVERRIDE FINAL;
    
    virtual bool showDescriptionLabel() const OVERRIDE
    {
        return false;
    }

    virtual ~KnobGuiParametric() OVERRIDE;
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

    virtual void getSelectedCurves(std::vector<boost::shared_ptr<CurveGui> >* selection) OVERRIDE FINAL;

    
public Q_SLOTS:


    void onCurveChanged(int dimension);

    void onItemsSelectionChanged();

    void resetSelectedCurves();

    void onColorChanged(int dimension);

private:

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool /*dirty*/) OVERRIDE FINAL
    {
    }

    virtual void setReadOnly(bool /*readOnly*/,
                             int /*dimension*/) OVERRIDE FINAL
    {
    }

private:
    // TODO: PIMPL
    QWidget* treeColumn;
    CurveWidget* _curveWidget;
    QTreeWidget* _tree;
    Button* _resetButton;
    struct CurveDescriptor
    {
        boost::shared_ptr<KnobCurveGui> curve;
        QTreeWidgetItem* treeItem;
    };

    typedef std::map<int,CurveDescriptor> CurveGuis;
    CurveGuis _curves;
    boost::weak_ptr<KnobParametric> _knob;
};


#endif // Gui_KnobGuiParametric_h
