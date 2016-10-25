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

#ifndef CURVEEDITOR_H
#define CURVEEDITOR_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/EngineFwd.h"

#include "Gui/PanelWidget.h"
#include "Gui/CurveSelection.h"
#include "Gui/AnimationModuleEditorUndoRedo.h"
#include "Gui/GuiFwd.h"


NATRON_NAMESPACE_ENTER;


struct CurveEditorPrivate;
class CurveEditor
    : public QWidget
      , public CurveSelection
      , public PanelWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    CurveEditor(const std::string& scriptName,
                Gui* gui,
                const TimeLinePtr& timeline,
                QWidget* parent = 0);

    virtual ~CurveEditor() OVERRIDE;

    /**
     * @brief Creates a new NodeCurveEditorContext and stores it until the CurveEditor is destroyed.
     **/
    void addNode(NodeGuiPtr node);

    void removeNode(const NodeGuiPtr& node);

    void setTreeWidgetWidth(int width);

    void centerOn(const std::vector<CurvePtr > & curves);

    std::pair<QAction*, QAction*> getUndoRedoActions() const WARN_UNUSED_RETURN;
    std::list<CurveGuiPtr > findCurve(const KnobGuiPtr& knob, int dimension) const WARN_UNUSED_RETURN;

    void hideCurves(const KnobGuiPtr& knob);

    void hideCurve(const KnobGuiPtr& knob, int dimension);

    void showCurves(const KnobGuiPtr& knob);

    void showCurve(const KnobGuiPtr& knob, int dimension);

    CurveWidget* getCurveWidget() const WARN_UNUSED_RETURN;
    virtual void getSelectedCurves(std::vector<CurveGuiPtr >* selection) OVERRIDE FINAL;

    void setSelectedCurve(const CurveGuiPtr& curve);

    CurveGuiPtr getSelectedCurve() const;

    void refreshCurrentExpression();

    void setSelectedCurveExpression(const QString& expression);

    void onInputEventCalled();

    virtual bool saveProjection(SERIALIZATION_NAMESPACE::ViewportData* data) OVERRIDE FINAL;

    virtual bool loadProjection(const SERIALIZATION_NAMESPACE::ViewportData& data) OVERRIDE FINAL;

public Q_SLOTS:

    void onFilterTextChanged(const QString& filter);

    void onItemSelectionChanged();

    void onItemDoubleClicked(QTreeWidgetItem* item, int);

    void onExprLineEditFinished();

private:

    virtual QUndoStack* getUndoStack() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void keyReleaseEvent(QKeyEvent* e) OVERRIDE FINAL;


    void recursiveSelect(QTreeWidgetItem* cur, std::vector<CurveGuiPtr > *curves, bool inspectRotos = true);

    boost::scoped_ptr<CurveEditorPrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // CURVEEDITOR_H
