//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */


#ifndef NODEGROUPWRAPPER_H
#define NODEGROUPWRAPPER_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <list>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

class NodeCollection;
class Effect;
class Group
{
    
    boost::weak_ptr<NodeCollection> _collection;
    
public:
    
    Group();
    
    void init(const boost::shared_ptr<NodeCollection>& collection);
    
    virtual ~Group();
    
    /**
     * @brief Get a pointer to a node. The node is identified by its fully qualified name, e.g:
     * Group1.Blur1 if it belongs to the Group1, or just Blur1 if it belongs to the project's root.
     * This function is called recursively on subgroups until a match is found. 
     * It is meant to be called only on the project's root.
     **/
    Effect* getNode(const std::string& fullySpecifiedName) const;
    
    /**
     * @brief Get all nodes in the project's root.
     **/
    std::list<Effect*> getChildren() const;

};

#endif // NODEGROUPWRAPPER_H
