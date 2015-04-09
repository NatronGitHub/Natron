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

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/KeySymbols.h"
#include "Global/Enums.h"

class QPointF;
class Double_Knob;
class KnobI;
class OverlaySupport;
namespace Natron {
class Node;
}

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
     * @brief Returns whether the node should draw an overlay (if it has any) on the viewer or not.
     **/
    virtual bool shouldDrawOverlay() const = 0;
    
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
    
    /**
     * @brief Set the size of the bounding box of the node in the nodegraph
     **/
    virtual void setSize(double w, double h) = 0;
    
    virtual void exportGroupAsPythonScript() = 0;
    
    /**
     * @brief Get the colour of the node as it appears on the nodegraph.
     **/
    virtual void getColor(double* r,double *g, double* b) const = 0;
    
    /**
     * @brief Set the colour of the node as it appears on the nodegraph.
     **/
    virtual void setColor(double r, double g, double b) = 0;
    
    /**
     * @brief Get the suggested overlay colour
     **/
    virtual bool getOverlayColor(double* r, double* g, double* b) const = 0;
    
    /**
     * @brief Add a default viewer overlay for the given point parameter
     **/
    virtual void addDefaultPositionInteract(const boost::shared_ptr<Double_Knob>& point) = 0;
    
    virtual void drawDefaultOverlay(double scaleX,double scaleY)  = 0;
    
    virtual bool onOverlayPenDownDefault(double scaleX,double scaleY,const QPointF & viewportPos, const QPointF & pos)  = 0;
    
    virtual bool onOverlayPenMotionDefault(double scaleX,double scaleY,const QPointF & viewportPos, const QPointF & pos)  = 0;
    
    virtual bool onOverlayPenUpDefault(double scaleX,double scaleY,const QPointF & viewportPos, const QPointF & pos)  = 0;
    
    virtual bool onOverlayKeyDownDefault(double scaleX,double scaleY,Natron::Key key,Natron::KeyboardModifiers modifiers)  = 0;
    
    virtual bool onOverlayKeyUpDefault(double scaleX,double scaleY,Natron::Key key,Natron::KeyboardModifiers modifiers)  = 0;
    
    virtual bool onOverlayKeyRepeatDefault(double scaleX,double scaleY,Natron::Key key,Natron::KeyboardModifiers modifiers) = 0;
    
    virtual bool onOverlayFocusGainedDefault(double scaleX,double scaleY) = 0;
    
    virtual bool onOverlayFocusLostDefault(double scaleX,double scaleY) = 0;
    
    virtual bool hasDefaultOverlay() const = 0;
    
    virtual void setCurrentViewportForDefaultOverlays(OverlaySupport* viewPort) = 0;
    
    virtual bool hasDefaultOverlayForParam(const KnobI* param) = 0;
    
    virtual void removeDefaultOverlay(KnobI* knob) = 0;
    
    virtual void setPluginIconFilePath(const std::string& filePath) = 0;
    
    virtual void setPluginDescription(const std::string& description) = 0;

    virtual void setPluginIDAndVersion(const std::string& pluginLabel,const std::string& pluginID,unsigned int version) = 0;
};

#endif // NODEGUII_H
