//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/

#ifndef KNOBINSTANCE_H
#define KNOBINSTANCE_H

class Knob;
class KnobGui;

/*This class manages the interaction between
a KnobGui object and a Knob object.
*/
class KnobInstance
{
    Knob* _knob;
    KnobGui* _gui;
    
public:
    KnobInstance(Knob* knob,KnobGui* gui):
    _knob(knob),
    _gui(gui)
    {}
};

#endif // KNOBINSTANCE_H
