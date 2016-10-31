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

#ifndef Gui_KnobGuiParametric_h
#define Gui_KnobGuiParametric_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector> // KnobGuiInt
#include <list>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QStyledItemDelegate>
#include <QTextEdit>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/Singleton.h"
#include "Engine/Knob.h"
#include "Engine/ImageComponents.h"
#include "Engine/EngineFwd.h"

#include "Gui/AnimationModuleBase.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/KnobGuiWidgets.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/Label.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;

class KnobGuiParametric
    : public QObject, public KnobGuiWidgets, public OverlaySupport, public AnimationModuleBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    static KnobGuiWidgets * BuildKnobGui(const KnobGuiPtr& knob, ViewIdx view)
    {
        return new KnobGuiParametric(knob, view);
    }

    KnobGuiParametric(const KnobGuiPtr& knob, ViewIdx view);
    virtual void removeSpecificGui() OVERRIDE FINAL;
    virtual bool mustCreateLabelWidget() const OVERRIDE
    {
        return false;
    }

    virtual ~KnobGuiParametric() OVERRIDE;


    // Overriden from OverlaySupport
    virtual void swapOpenGLBuffers() OVERRIDE FINAL;
    virtual void redraw() OVERRIDE FINAL;
    virtual void getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const OVERRIDE FINAL;
    virtual void getViewportSize(double &width, double &height) const OVERRIDE FINAL;
    virtual void getPixelScale(double & xScale, double & yScale) const OVERRIDE FINAL;
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE FINAL;
    virtual void saveOpenGLContext() OVERRIDE FINAL;
    virtual void restoreOpenGLContext() OVERRIDE FINAL;

    // Overriden from AnimationModuleBase
    virtual TimeLinePtr getTimeline() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void getTopLevelKnobs(std::vector<KnobAnimPtr>* knobs) const OVERRIDE FINAL;
    virtual void refreshSelectionBboxAndUpdateView() OVERRIDE FINAL;
    virtual CurveWidget* getCurveWidget() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual AnimationModuleSelectionModelPtr getSelectionModel() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool findItem(QTreeWidgetItem* treeItem, AnimatedItemTypeEnum *type, KnobAnimPtr* isKnob, TableItemAnimPtr* isTableItem, NodeAnimPtr* isNodeItem, ViewSetSpec* view, DimSpec* dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;

public Q_SLOTS:


    void onCurveChanged(DimSpec dimension);

    void onItemsSelectionChanged();

    void resetSelectedCurves();

    void onColorChanged(DimSpec dimension);

private:

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void setWidgetsVisible(bool visible) OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void updateGUI(DimSpec dimension) OVERRIDE FINAL;
    virtual void setDirty(bool /*dirty*/) OVERRIDE FINAL
    {
    }

    virtual void setReadOnly(bool /*readOnly*/,
                             DimSpec /*dimension*/) OVERRIDE FINAL
    {
    }

    virtual void refreshDimensionName(DimIdx dim) OVERRIDE FINAL;

private:
    // TODO: PIMPL
    QWidget* treeColumn;
    CurveWidget* _curveWidget;
    QTreeWidget* _tree;
    AnimationModuleSelectionModelPtr _selectionModel;
    KnobAnimPtr _animRoot;
    Button* _resetButton;
    struct CurveDescriptor
    {
        CurveGuiPtr curve;
        QTreeWidgetItem* treeItem;
    };

    typedef std::vector<CurveDescriptor> CurveGuis;
    CurveGuis _curves;
    KnobParametricWPtr _knob;
};

NATRON_NAMESPACE_EXIT;

#endif // Gui_KnobGuiParametric_h
