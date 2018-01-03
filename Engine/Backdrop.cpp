/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Backdrop.h"

#include <cassert>
#include <stdexcept>

#include "Engine/Transform.h"
#include "Engine/KnobTypes.h"

NATRON_NAMESPACE_ENTER

struct BackdropPrivate
{
    KnobStringWPtr knobLabel;

    BackdropPrivate()
        : knobLabel()
    {
    }
};

PluginPtr
Backdrop::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_OTHER);
    PluginPtr ret = Plugin::create(Backdrop::create, Backdrop::createRenderClone, PLUGINID_NATRON_BACKDROP, "Backdrop", 1, 0, grouping);

    QString desc =  tr("The Backdrop node is useful to group nodes and identify them in the node graph.\n"
                       "You can also move all the nodes inside the backdrop.");
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    ret->setProperty<std::string>(kNatronPluginPropIconFilePath,  "Images/backdrop_icon.png");
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthFloat, 0);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthShort, 1);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthByte, 2);
    ret->setProperty<std::bitset<4> >(kNatronPluginPropOutputSupportedComponents, std::bitset<4>(std::string("1111")));

    return ret;
}


Backdrop::Backdrop(const NodePtr& node)
: NoOpBase(node)
, _imp( new BackdropPrivate() )
{
}

Backdrop::Backdrop(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key)
: NoOpBase(mainInstance, key)
, _imp( new BackdropPrivate() )
{

}

Backdrop::~Backdrop()
{
}

KnobStringPtr
Backdrop::getTextAreaKnob() const
{
    return _imp->knobLabel.lock();
}

void
Backdrop::initializeKnobs()
{
    KnobPagePtr page = createKnob<KnobPage>("controlsPage");
    page->setLabel(tr("Controls"));
    {
        KnobStringPtr knobLabel = createKnob<KnobString>("label");
        knobLabel->setLabel(tr("Label"));
        knobLabel->setAnimationEnabled(false);
        knobLabel->setAsMultiLine();
        knobLabel->setUsesRichText(true);
        knobLabel->setHintToolTip( tr("Text to display on the backdrop.") );
        knobLabel->setEvaluateOnChange(false);
        page->addKnob(knobLabel);

        _imp->knobLabel = knobLabel;
    }
}

bool
Backdrop::knobChanged(const KnobIPtr& k,
                      ValueChangedReasonEnum /*reason*/,
                      ViewSetSpec /*view*/,
                      TimeValue /*time*/)
{
    if ( k == _imp->knobLabel.lock() ) {
        QString text = QString::fromUtf8( _imp->knobLabel.lock()->getValue().c_str() );
        Q_EMIT labelChanged(text);

        return true;
    }

    return false;
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_Backdrop.cpp"
