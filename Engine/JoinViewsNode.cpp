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

#include "JoinViewsNode.h"

#include <QMutex>

#include "Engine/AppInstance.h"
#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/NodeMetadata.h"

NATRON_NAMESPACE_ENTER;

struct JoinViewsNodePrivate
{
    QMutex inputsMutex;
    std::vector<std::string> inputs;
    
    JoinViewsNodePrivate()
    : inputs()
    {
        
    }
};

JoinViewsNode::JoinViewsNode(NodePtr node)
: EffectInstance(node)
, _imp(new JoinViewsNodePrivate())
{
    setSupportsRenderScaleMaybe(eSupportsYes);
    if (node) {
        boost::shared_ptr<Project> project = node->getApp()->getProject();
        QObject::connect(project.get(), SIGNAL(projectViewsChanged()), this, SLOT(onProjectViewsChanged()));
    }
    
}

JoinViewsNode::~JoinViewsNode()
{
}

int
JoinViewsNode::getMaxInputCount() const
{
    QMutexLocker k(&_imp->inputsMutex);
    return (int)_imp->inputs.size();
}

std::string
JoinViewsNode::getInputLabel (int inputNb) const
{
    QMutexLocker k(&_imp->inputsMutex);
    assert(inputNb >= 0 && inputNb < (int)_imp->inputs.size());
    return _imp->inputs[inputNb];
}


void
JoinViewsNode::addAcceptedComponents(int /*inputNb*/,std::list<ImageComponents>* comps)
{
    comps->push_back(ImageComponents::getRGBAComponents());
    comps->push_back(ImageComponents::getRGBComponents());
    comps->push_back(ImageComponents::getAlphaComponents());
}
void
JoinViewsNode::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthFloat);
    depths->push_back(eImageBitDepthShort);
    depths->push_back(eImageBitDepthByte);
}

void
JoinViewsNode::initializeKnobs()
{
    onProjectViewsChanged();
}

bool
JoinViewsNode::isHostChannelSelectorSupported(bool* /*defaultR*/,bool* /*defaultG*/, bool* /*defaultB*/, bool* /*defaultA*/) const
{
    return false;
}

bool
JoinViewsNode::isIdentity(double time,
                          const RenderScale & /*scale*/,
                          const RectI & /*roi*/,
                          ViewIdx view,
                          double* inputTime,
                          int* inputNb)
{
    *inputTime = time;
    *inputNb = getMaxInputCount() - 1 - view.value();
    return true;
}

void
JoinViewsNode::onProjectViewsChanged()
{
    std::size_t nInputs,oldNInputs;
    {
        QMutexLocker k(&_imp->inputsMutex);
        const std::vector<std::string>& views = getApp()->getProject()->getProjectViewNames();
        
        //Reverse names
        oldNInputs = _imp->inputs.size();
        nInputs = views.size();
        _imp->inputs.resize(nInputs);
        for (std::size_t i = 0; i < nInputs; ++i) {
            _imp->inputs[i] = views[nInputs - 1 - i];
        }
        
    }
    std::vector<NodePtr> inputs(oldNInputs);
    NodePtr node = getNode();
    for (std::size_t i = 0; i < oldNInputs; ++i) {
        inputs[i] = node->getInput(i);
    }
    node->initializeInputs();
    
    //Reconnect the inputs
    int index = oldNInputs - 1;
    if (index >= 0) {
        node->beginInputEdition();
        
        for (int i = (int)nInputs - 1; i  >=0; --i,--index) {
            node->disconnectInput(i);
            if (index >= 0) {
                if (inputs[index]) {
                    node->connectInput(inputs[index], i);
                }
            }
        }
        
        node->endInputEdition(true);
    }
}


NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_JoinViewsNode.cpp"