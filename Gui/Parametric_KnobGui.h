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

#ifndef _Gui_Parametric_KnobGui_h_
#define _Gui_Parametric_KnobGui_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <vector> // Int_KnobGui
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

//Define this if you want the spinbox to clamp to the plugin defined range
//#define SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT

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
class Int_Knob;
class Bool_Knob;
class Double_Knob;
class Button_Knob;
class Separator_Knob;
class Group_Knob;
class Tab_Knob;
class Parametric_Knob;
class Color_Knob;
class Choice_Knob;
class String_Knob;

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

class Parametric_KnobGui
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
        return new Parametric_KnobGui(knob, container);
    }

    Parametric_KnobGui(boost::shared_ptr<KnobI> knob,
                       DockablePanel *container);
    
    virtual void removeSpecificGui() OVERRIDE FINAL;
    
    virtual bool showDescriptionLabel() const OVERRIDE
    {
        return false;
    }

    virtual ~Parametric_KnobGui() OVERRIDE;
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
    boost::weak_ptr<Parametric_Knob> _knob;
};


#endif // _Gui_Parametric_KnobGui_h_
