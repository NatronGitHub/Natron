//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */
#include "BackDrop.h"
#include "Engine/KnobTypes.h"

struct BackDropPrivate
{
    
    boost::shared_ptr<String_Knob> knobLabel;
    boost::shared_ptr<Int_Knob> headerFontSize;
    
    BackDropPrivate()
    : knobLabel()
    , headerFontSize()
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
    
    _imp->knobLabel = Natron::createKnob<String_Knob>( this, "Label");
    _imp->knobLabel->setAnimationEnabled(false);
    _imp->knobLabel->setAsMultiLine();
    _imp->knobLabel->setUsesRichText(true);
    _imp->knobLabel->setHintToolTip( QObject::tr("Text to display on the backdrop.").toStdString() );
    _imp->knobLabel->setEvaluateOnChange(false);
    page->addKnob(_imp->knobLabel);
 

}


void
BackDrop::knobChanged(KnobI* k,
                      Natron::ValueChangedReasonEnum /*reason*/,
                      int /*view*/,
                      SequenceTime /*time*/,
                      bool /*originatedFromMainThread*/)
{
    if ( k == _imp->knobLabel.get() ) {
        QString text( _imp->knobLabel->getValue().c_str() );
        Q_EMIT labelChanged(text);
    } 
}