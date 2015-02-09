//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef SCRIPTOBJECT_H
#define SCRIPTOBJECT_H

#include <string>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

struct ScriptObjectPrivate;
class ScriptObject
{
public:
    
    ScriptObject();
    
    virtual ~ScriptObject();
    
    void setLabel(const std::string& label);
    
    std::string getLabel() const;
    
    void setScriptName(const std::string& name);
    
    std::string getScriptName() const;
    
private:
    
    boost::scoped_ptr<ScriptObjectPrivate> _imp;
};

#endif // SCRIPTOBJECT_H
