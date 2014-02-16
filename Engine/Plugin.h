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

#ifndef PLUGIN_H
#define PLUGIN_H

#include <vector>
#include <QString>

class QMutex;
namespace Natron {
    class LibraryBinary;
}

class PluginGroupNode{
    
    QString _id;
    QString _label;
    QString _iconPath;
    std::vector<PluginGroupNode*> _children;
    PluginGroupNode* _parent;
    
public:
    PluginGroupNode(const QString& pluginID,
                    const QString& pluginLabel,
                    const QString& iconPath)
    :
    _id(pluginID)
    , _label(pluginLabel)
    , _iconPath(iconPath)
    , _children()
    , _parent(NULL)
    {
        
    }
    
    const QString& getID() const {return _id;}
    
    const QString& getLabel() const {return _label;}
    
    void setLabel(const QString& label) {_label = label;}
    
    const QString& getIconPath() const {return _iconPath;}
    
    void setIconPath(const QString& iconPath) {_iconPath = iconPath;}
    
    const std::vector<PluginGroupNode*>& getChildren() const {return _children;}
    
    void tryAddChild(PluginGroupNode* plugin);
    
    PluginGroupNode* getParent() const  {return _parent;}
    
    void setParent(PluginGroupNode* parent) {_parent = parent;}
    
    bool hasParent() const {return _parent != NULL;}
    
};

namespace Natron{
    class Plugin {
        
        Natron::LibraryBinary* _binary;
        QString _id;
        QString _label;
        QMutex* _lock;
        int _majorVersion;
        int _minorVersion;
        
    public:
        
        Plugin():
        _binary(NULL)
        , _id()
        , _label()
        , _lock()
        , _majorVersion(0)
        , _minorVersion(0)
        {}
        
        Plugin(Natron::LibraryBinary* binary,
               const QString& id,
               const QString& label,
               QMutex* lock,
               int majorVersion,
               int minorVersion
               ):
        _binary(binary)
        , _id(id)
        , _label(label)
        , _lock(lock)
        , _majorVersion(majorVersion)
        , _minorVersion(minorVersion)
        {
            
        }
        
        ~Plugin();
        
        void setPluginID(const QString& id) { _id = id; }
        
        const QString& getPluginID() const { return _id; }
        
        void setPluginLabel(const QString& label){ _label = label; }
        
        const QString& getPluginLabel() const { return _label; }
        
        QMutex* getPluginLock() const  { return _lock; }
        
        Natron::LibraryBinary* getLibraryBinary() const { return _binary; }
        
        int getMajorVersion() const { return _majorVersion; }
        
        int getMinorVersion() const { return _minorVersion; }
        
    };
}

#endif // PLUGIN_H
