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

#ifndef STUBNODE_H
#define STUBNODE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/NoOpBase.h"
#include "Engine/EngineFwd.h"

#define kStubNodeParamSerialization "content"
#define kStubNodeParamSerializationLabel "Content"
#define kStubNodeParamSerializationHint "The content of the node that was saved in the project"

NATRON_NAMESPACE_ENTER;

struct StubNodePrivate;
class StubNode
: public NoOpBase
{
private: // derives from EffectInstance
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    StubNode(const NodePtr& n);

public:

    static PluginPtr createPlugin();

    virtual ~StubNode();

    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new StubNode(node) );
    }


    virtual int getMaxInputCount() const OVERRIDE WARN_UNUSED_RETURN;

    virtual std::string getInputLabel(int /*inputNb*/) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    void refreshInputsFromSerialization();
    
    SERIALIZATION_NAMESPACE::NodeSerializationPtr getNodeSerialization () const;

    virtual void onEffectCreated(bool mayCreateFileDialog, const CreateNodeArgs& defaultParamValues) OVERRIDE FINAL;
    
private:

    virtual void initializeKnobs() OVERRIDE FINAL;
    
    virtual bool knobChanged(const KnobIPtr& k,
                             ValueChangedReasonEnum reason,
                             ViewSpec view,
                             double time,
                             bool originatedFromMainThread) OVERRIDE FINAL;

private:

    boost::scoped_ptr<StubNodePrivate> _imp;
};

inline StubNodePtr
toStubNode(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<StubNode>(effect);
}

NATRON_NAMESPACE_EXIT;


#endif // STUBNODE_H
