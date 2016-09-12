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


// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****



#include "StubNode.h"

#include "Engine/AppManager.h"
#include "Engine/KnobTypes.h"

NATRON_NAMESPACE_ENTER

struct StubNodePrivate
{

    KnobStringWPtr serializationKnob;

    StubNodePrivate()
    {
        
    }
};

StubNode::StubNode(const NodePtr& n)
: NoOpBase(n)
, _imp(new StubNodePrivate())
{
}

StubNode::~StubNode()
{

}

std::string
StubNode::getPluginDescription() const
{
    return tr("This plug-in is used as a temporary replacement for another plug-in when loading a project with a plug-in which cannot be found.").toStdString();
}

std::string
StubNode::getInputLabel(int inputNb) const
{

}

std::string
StubNode::getPluginLabel() const
{
    return "Stub";
}

void
StubNode::onEffectCreated(bool mayCreateFileDialog, const CreateNodeArgs& defaultParamValues)
{

}

void
StubNode::initializeKnobs()
{
    EffectInstancePtr thisShared = shared_from_this();
    {
        KnobStringPtr param = AppManager::createKnob<KnobString>(thisShared, tr(kStubNodeParamSerializationLabel));
        param->setName(kStubNodeParamSerialization);
        param->setHintToolTip(tr(kStubNodeParamSerializationHint));
        param->setAsMultiLine();
        _imp->serializationKnob = param;
    }
}

void
StubNode::onKnobsLoaded()
{

}
NATRON_NAMESPACE_EXIT
