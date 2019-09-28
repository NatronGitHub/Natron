/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Engine_Backdrop_h
#define Engine_Backdrop_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/NoOpBase.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

struct BackdropPrivate;

class Backdrop
    : public NoOpBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private: // derives from EffectInstance
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    Backdrop(const NodePtr& node);

    Backdrop(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key);

public:
    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new Backdrop(node) );
    }

    static EffectInstancePtr createRenderClone(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new Backdrop(mainInstance, key) );
    }

    static PluginPtr createPlugin();

    virtual ~Backdrop();

    KnobStringPtr getTextAreaKnob() const;

Q_SIGNALS:

    void labelChanged(QString);

private:

    virtual bool knobChanged(const KnobIPtr&,
                             ValueChangedReasonEnum /*reason*/,
                             ViewSetSpec /*view*/,
                             TimeValue /*time*/) OVERRIDE FINAL;
    virtual void initializeKnobs() OVERRIDE FINAL;
    boost::scoped_ptr<BackdropPrivate> _imp;
};


inline BackdropPtr
toBackdrop(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<Backdrop>(effect);
}


NATRON_NAMESPACE_EXIT

#endif // Engine_Backdrop_h
