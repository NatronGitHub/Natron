//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */


#ifndef DOCKABLEPANELI_H
#define DOCKABLEPANELI_H


class DockablePanelI
{
    
public:
    
    DockablePanelI() {}
    
    virtual ~DockablePanelI() {}
    
    /**
     * @brief Must scan for new knobs and rebuild the panel accordingly
     **/
    virtual void scanForNewKnobs() = 0;
};
#endif // DOCKABLEPANELI_H
