//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

/**
* @brief Simple wrap for the EffectInstance and Node class that is the API we want to expose to the Python
* Engine module.
**/

#ifndef NODEWRAPPER_H
#define NODEWRAPPER_H

#include <list>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include "Engine/ParameterWrapper.h"

namespace Natron {
class Node;
}

class Param;

class Effect
{
    boost::shared_ptr<Natron::Node> _node;
    
public:
    
    Effect(const boost::shared_ptr<Natron::Node>& node);
    
    ~Effect();
    
    bool isNull() const
    {
        if (_node) {
            return false;
        } else {
            return true;
        }
    }
    
    /**
     * @brief Returns the maximum number of inputs that can be connected to the node.
     **/
    int getMaxInputCount() const;
    
    /**
     * @brief Determines whether a connection is possible for the given node at the given input number.
     **/
    bool canSetInput(int inputNumber,const Effect* node) const;
    
    /**
     * @brief Attempts to connect the Effect 'input' to the given inputNumber.
     * This function uses canSetInput(int,Effect) to determine whether a connection is possible.
     * There's no auto-magic behind this function: you must explicitely disconnect any already connected Effect
     * to the given inputNumber otherwise this function will return false.
     **/
    bool connectInput(int inputNumber,const Effect* input);

    /**
     * @brief Disconnects any Effect connected to the given inputNumber.
     **/
    void disconnectInput(int inputNumber);
    
    /**
     * @brief Returns the Effect connected to the given inputNumber
     * @returns Pointer to an Effect, the caller is responsible for freeing it.
     **/
    Effect* getInput(int inputNumber) const;
    
    /**
     * @brief Returns the name of the Effect as displayed on the GUI.
     **/
    std::string getName() const;
    
    /**
     * @brief Returns the ID of the plug-in embedded into the Effect
     **/
    std::string getPluginID() const;
    
    /**
     * @brief Returns a list of all parameters for the Effect. These are the parameters located in the settings panel
     * on the GUI.
     **/
    std::list<Param*> getParameters() const;
    
    /**
     * @brief Returns a pointer to the Param named after the given name or NULL if no parameter with the given name could be found.
     **/
    Param* getParamByName(const std::string& name) const;
    
};

#endif // NODEWRAPPER_H
