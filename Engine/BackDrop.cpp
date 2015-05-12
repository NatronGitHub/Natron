//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "BackDrop.h"
#include "Engine/Transform.h"
#include "Engine/KnobTypes.h"

struct BackDropPrivate
{
    
    boost::weak_ptr<String_Knob> knobLabel;
    
    BackDropPrivate()
    : knobLabel()
    {
    }
    
};

BackDrop::BackDrop(boost::shared_ptr<Natron::Node> node)
: NoOpBase(node)
, _imp(new BackDropPrivate())
{
}

BackDrop::~BackDrop()
{
    
}

std::string
BackDrop::getDescription() const
{
    return QObject::tr("The node backdrop is useful to group nodes and identify them in the node graph. You can also "
              "move all the nodes inside the backdrop.").toStdString() ;
}

void
BackDrop::initializeKnobs()
{
    boost::shared_ptr<Page_Knob> page = Natron::createKnob<Page_Knob>(this, "Controls");
    
    boost::shared_ptr<String_Knob> knobLabel = Natron::createKnob<String_Knob>( this, "Label");
    knobLabel->setAnimationEnabled(false);
    knobLabel->setAsMultiLine();
    knobLabel->setUsesRichText(true);
    knobLabel->setHintToolTip( QObject::tr("Text to display on the backdrop.").toStdString() );
    knobLabel->setEvaluateOnChange(false);
    page->addKnob(knobLabel);
    _imp->knobLabel = knobLabel;
 

}


void
BackDrop::knobChanged(KnobI* k,
                      Natron::ValueChangedReasonEnum /*reason*/,
                      int /*view*/,
                      SequenceTime /*time*/,
                      bool /*originatedFromMainThread*/)
{
    if ( k == _imp->knobLabel.lock().get() ) {
        QString text( _imp->knobLabel.lock()->getValue().c_str() );
        Q_EMIT labelChanged(text);
    } 
}