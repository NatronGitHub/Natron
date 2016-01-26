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

#ifndef NODEGUII_H
#define NODEGUII_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/KeySymbols.h"
#include "Global/Enums.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

class NodeGuiI
{
public :

    NodeGuiI() {}
    
    virtual ~NodeGuiI() {}
    
    /**
     * @brief Should destroy any GUI associated to the internal node. Memory should be recycled and widgets no longer accessible from 
     * anywhere. Upon returning this function, the caller should have a shared_ptr use_count() of 1
     **/
    virtual void destroyGui() = 0;
    
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
    virtual void addDefaultPositionInteract(const boost::shared_ptr<KnobDouble>& point) = 0;
    
    /**
     * @brief Add a default viewer transform overlay for this set of parameters
     **/
    virtual void addTransformInteract(const boost::shared_ptr<KnobDouble>& translate,
                                      const boost::shared_ptr<KnobDouble>& scale,
                                      const boost::shared_ptr<KnobBool>& scaleUniform,
                                      const boost::shared_ptr<KnobDouble>& rotate,
                                      const boost::shared_ptr<KnobDouble>& skewX,
                                      const boost::shared_ptr<KnobDouble>& skewY,
                                      const boost::shared_ptr<KnobChoice>& skewOrder,
                                      const boost::shared_ptr<KnobDouble>& center) = 0;
    
    virtual void drawHostOverlay(double time, const RenderScale & renderScale)  = 0;
    
    virtual bool onOverlayPenDownDefault(const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos, double pressure)  = 0;
    
    virtual bool onOverlayPenMotionDefault(const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos, double pressure)  = 0;
    
    virtual bool onOverlayPenUpDefault(const RenderScale & renderScale, const QPointF & viewportPos, const QPointF & pos, double pressure)  = 0;
    
    virtual bool onOverlayKeyDownDefault(const RenderScale & renderScale, Key key, KeyboardModifiers modifiers)  = 0;
    
    virtual bool onOverlayKeyUpDefault(const RenderScale & renderScale, Key key, KeyboardModifiers modifiers)  = 0;
    
    virtual bool onOverlayKeyRepeatDefault(const RenderScale & renderScale, Key key, KeyboardModifiers modifiers) = 0;
    
    virtual bool onOverlayFocusGainedDefault(const RenderScale & renderScale) = 0;
    
    virtual bool onOverlayFocusLostDefault(const RenderScale & renderScale) = 0;
    
    virtual bool hasHostOverlay() const = 0;
    
    virtual void setCurrentViewportForHostOverlays(OverlaySupport* viewPort) = 0;
    
    virtual bool hasHostOverlayForParam(const KnobI* param) = 0;
    
    virtual void removePositionHostOverlay(KnobI* knob) = 0;
    
    virtual void setPluginIconFilePath(const std::string& filePath) = 0;
    
    virtual void setPluginDescription(const std::string& description) = 0;

    virtual void setPluginIDAndVersion(const std::string& pluginLabel, const std::string& pluginID, unsigned int version) = 0;
    
    virtual bool isUserSelected() const = 0;
    
    virtual void restoreStateAfterCreation() = 0;
    
    virtual void onIdentityStateChanged(int inputNb) = 0;
};

NATRON_NAMESPACE_EXIT;

#endif // NODEGUII_H
