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

#ifndef NODEGUII_H
#define NODEGUII_H



class NodeGuiI
{
public :

    NodeGuiI() {}
    
    virtual ~NodeGuiI() {}
    
    /**
     * @brief Returns whether the settings panel of this node is opened or not.
     **/
    virtual bool isSettingsPanelOpened() const = 0;
    
    /**
     * @brief Set the position of the node in the nodegraph.
     **/
    virtual void setPosition(double x,double y) = 0;
    
    /**
     * @brief Get the position of top left corner of the node in the nodegraph.
     * To retrieve the position of the center, you must add w / 2 and h / 2 respectively
     * to x and y. w and h can be retrieved with getSize()
     **/
    virtual void getPosition(double *x, double* y) const = 0;
    
    /**
     * @brief Get the size of the bounding box of the node in the nodegraph
     **/
    virtual void getSize(double* w, double* h) const = 0;
    
    virtual void exportGroupAsPythonScript() = 0;
};

#endif // NODEGUII_H
