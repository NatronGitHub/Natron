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

#ifndef PRECOMPNODE_H
#define PRECOMPNODE_H

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

#include "Engine/NodeGroup.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

struct PrecompNodePrivate;
class PrecompNode
    : public NodeGroup
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private: // derives from EffectInstance
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    PrecompNode(const NodePtr& n);
public:
    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new PrecompNode(node) );
    }

    static PluginPtr createPlugin();

    virtual ~PrecompNode();


    virtual bool isOutput() const OVERRIDE WARN_UNUSED_RETURN
    {
        return false;
    }

    NodePtr getOutputNode() const;

    AppInstancePtr getPrecompApp() const;

    virtual bool isSubGraphUserVisible() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool isSubGraphPersistent() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual void setupInitialSubGraphState() OVERRIDE FINAL;

public Q_SLOTS:

    void onPreRenderFinished();

    void onReadNodePersistentMessageChanged();

private:

    virtual void initializeKnobs() OVERRIDE FINAL;
    virtual void onKnobsLoaded() OVERRIDE FINAL;
    virtual bool knobChanged(const KnobIPtr& k,
                             ValueChangedReasonEnum reason,
                             ViewSetSpec view,
                             TimeValue time) OVERRIDE FINAL;
    boost::scoped_ptr<PrecompNodePrivate> _imp;
};


inline PrecompNodePtr
toPrecompNode(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<PrecompNode>(effect);
}


NATRON_NAMESPACE_EXIT

#endif // PRECOMPNODE_H
