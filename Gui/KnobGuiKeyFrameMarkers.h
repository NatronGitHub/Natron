/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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

#ifndef KNOBGUIKEYFRAMEMARKERS_H
#define KNOBGUIKEYFRAMEMARKERS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****


#include "Gui/KnobGuiWidgets.h"

NATRON_NAMESPACE_ENTER

class KnobGuiKeyFrameMarkers
: public QObject, public KnobGuiWidgets
{


    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON

    struct Implementation;

public:

    static KnobGuiWidgets * BuildKnobGui(const KnobGuiPtr& knob, ViewIdx view)
    {
        return new KnobGuiKeyFrameMarkers(knob, view);
    }

    KnobGuiKeyFrameMarkers(const KnobGuiPtr& knob, ViewIdx view);

    virtual ~KnobGuiKeyFrameMarkers() OVERRIDE;

    virtual void reflectLinkedState(DimIdx dimension, bool linked) OVERRIDE;


public Q_SLOTS:

    void onGoToPrevKeyframeButtonClicked();

    void onGoToNextKeyframeButtonClicked();

    void onAddKeyframeButtonClicked();

    void onRemoveKeyframeButtonClicked();

    void onRemoveAnimationButtonClicked();

private:


    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void setWidgetsVisible(bool visible) OVERRIDE FINAL;
    virtual void setEnabled(const std::vector<bool>& perDimEnabled) OVERRIDE FINAL;
    virtual void reflectMultipleSelection(bool dirty) OVERRIDE FINAL;
    virtual void reflectSelectionState(bool selected) OVERRIDE FINAL;
    virtual void updateGUI() OVERRIDE FINAL;
    virtual void reflectAnimationLevel(DimIdx dimension, AnimationLevelEnum level) OVERRIDE FINAL;
    virtual void updateToolTip() OVERRIDE FINAL;
    virtual void onLabelChanged() OVERRIDE FINAL;

private:

    boost::scoped_ptr<Implementation> _imp;

};


NATRON_NAMESPACE_EXIT

#endif // KNOBGUIKEYFRAMEMARKERS_H
