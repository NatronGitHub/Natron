/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
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

#ifndef Gui_KnobGuiBytes_h
#define Gui_KnobGuiBytes_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/EngineFwd.h"

#include "Gui/KnobGui.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief The interface of a KnobBytes (OFX 1.5 Bytes parameter): opaque data, which is never shown or edited.
 * The knob is secret, and this interface only exists so that the knob gets the undo stack of the settings
 * panel (the changes of the plug-in in a paramEditBegin()/paramEditEnd() block, see Knob<T>::setValue()).
 * It never reads the value: a large value is not copied into a widget.
 **/
class KnobGuiBytes
    : public KnobGui
{
public:
    static KnobGui * BuildKnobGui(KnobIPtr knob,
                                  KnobGuiContainerI *container)
    {
        return new KnobGuiBytes(knob, container);
    }

    KnobGuiBytes(KnobIPtr knob,
                 KnobGuiContainerI *container);

    virtual ~KnobGuiBytes() OVERRIDE;

    virtual void removeSpecificGui() OVERRIDE FINAL;
    virtual KnobIPtr getKnob() const OVERRIDE FINAL;

private:
    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL
    {
    }

    virtual void setReadOnly(bool /*readOnly*/,
                             int /*dimension*/) OVERRIDE FINAL
    {
    }

    virtual void setDirty(bool /*dirty*/) OVERRIDE FINAL
    {
    }

    virtual void updateGUI(int /*dimension*/) OVERRIDE FINAL
    {
    }

private:
    Label *_label;
    KnobBytesWPtr _knob;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_KnobGuiBytes_h
