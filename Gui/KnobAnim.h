/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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


#ifndef NATRON_GUI_KNOBANIM_H
#define NATRON_GUI_KNOBANIM_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <QObject>

#include "Gui/AnimItemBase.h"

NATRON_NAMESPACE_ENTER

class KnobAnimPrivate;
class KnobAnim : public QObject, public AnimItemBase
{
    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON

    struct MakeSharedEnabler;

    // used by make_shared
    KnobAnim(const AnimationModuleBasePtr& model,
             const KnobsHolderAnimBasePtr& holder,
             QTreeWidgetItem *parentItem,
             const KnobIPtr& knob);

public:
    static KnobAnimPtr create(const AnimationModuleBasePtr& model,
                              const KnobsHolderAnimBasePtr& holder,
                              QTreeWidgetItem *parentItem,
                              const KnobIPtr& knob);

    virtual ~KnobAnim();

    virtual AnimatingObjectIPtr getInternalAnimItem() const OVERRIDE FINAL;

    KnobsHolderAnimBasePtr getHolder() const;
    
    /**
     * @brief Returns the associated internal Knob
     **/
    KnobIPtr getInternalKnob() const;

    //// Overriden from AnimItemBase
    virtual QString getViewDimensionLabel(DimIdx dimension, ViewIdx view) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual QTreeWidgetItem * getRootItem() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual QTreeWidgetItem * getTreeItem(DimSpec dimension, ViewSetSpec view) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual CurvePtr getCurve(DimIdx dimension, ViewIdx view) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual CurveGuiPtr getCurveGui(DimIdx dimension, ViewIdx view) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::list<ViewIdx> getViewsList() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getNDimensions() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool hasExpression(DimIdx dimension, ViewIdx view) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual double evaluateCurve(bool useExpressionIfAny, double x, DimIdx dimension, ViewIdx view) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getAllDimensionsVisible(ViewIdx view) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    ////

    void refreshVisibilityConditional(bool refreshHolder);

    void refreshViewLabels(const std::vector<std::string>& projectViewNames);

    void destroyItems();

public Q_SLOTS:

    void emit_s_refreshKnobVisibilityLater();

    void onDoRefreshKnobVisibilityLaterTriggered();

    void onKnobAvailableViewsChanged();

    void onInternalKnobDimensionsVisibilityChanged(ViewSetSpec view);

    void onDoRefreshDimensionsVisibilitylaterTriggered();

    void onCurveAnimationChangedInternally(ViewSetSpec view,
                                           DimSpec dimension);

Q_SIGNALS:

    void s_refreshKnobVisibilityLater();

    void s_refreshDimensionsVisibilityLater();

private:

    void destroyAndRecreate();

    void refreshKnobVisibilityNow();

    void initialize();
    
    boost::scoped_ptr<KnobAnimPrivate> _imp;
};
inline KnobAnimPtr toKnobAnim(const AnimItemBasePtr& p)
{
    return boost::dynamic_pointer_cast<KnobAnim>(p);
}

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_KNOBANIM_H
