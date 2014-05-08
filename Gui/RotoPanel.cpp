//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "RotoPanel.h"

struct RotoPanelPrivate
{
    NodeGui* node;
    
    RotoPanelPrivate(NodeGui*  n)
    : node(n)
    {
        
    }
};

RotoPanel::RotoPanel(NodeGui* n,QWidget* parent)
: QWidget(parent)
, _imp(new RotoPanelPrivate(n))
{
    
}

RotoPanel::~RotoPanel()
{
    
}