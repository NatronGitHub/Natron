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

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

namespace Natron {
class Node;
}

class Effect
{
    boost::shared_ptr<Natron::Node> _node;
    
public:
    
    Effect(const boost::shared_ptr<Natron::Node>& node);
    
    
};

#endif // NODEWRAPPER_H
