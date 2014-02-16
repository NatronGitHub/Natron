//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "Plugin.h"

#include <QMutex>

#include "Engine/LibraryBinary.h"

using namespace Natron;

Plugin::~Plugin(){
    if(_lock){
        delete _lock;
    }
    if(_binary){
        delete _binary;
    }
}


void PluginGroupNode::tryAddChild(PluginGroupNode* plugin){
    for(unsigned int i = 0; i < _children.size() ; ++i){
        if(_children[i] == plugin)
            return;
    }
    _children.push_back(plugin);
}