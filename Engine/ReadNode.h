/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#ifndef READNODE_H
#define READNODE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/NodeGroup.h"

#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER


#define NATRON_READER_INPUT_NAME "Sync"

#define kNatronReadNodeParamDecodingPluginChoice "decodingPluginChoice"

#define kNatronReadNodeOCIOParamInputSpace "ocioInputSpace"


/**
 * @brief A wrapper around all OpenFX Readers nodes so that to the user they all appear under a single Read node that has a dynamic
 * settings panel.
 * Every method forwards to the corresponding OpenFX plug-in.
 **/
struct ReadNodePrivate;
class ReadNode
    : public NodeGroup
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    /**
     * @brief Returns if the given plug-in is compatible with this ReadNode container.
     * by default all nodes which inherits GenericReader in OpenFX are.
     **/
    static bool isBundledReader(const std::string& pluginID);

private: // derives from EffectInstance
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    ReadNode(const NodePtr& n);
public:
    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new ReadNode(node) );
    }



    static PluginPtr createPlugin();

    virtual ~ReadNode();

    NodePtr getEmbeddedReader() const;

    void setEmbeddedReader(const NodePtr& node);
    static bool isVideoReader(const std::string& pluginID);

    virtual bool isReader() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isVideoReader() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isGenerator() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual void onEffectCreated(const CreateNodeArgs& defaultParamValues) OVERRIDE FINAL;

    virtual bool isSubGraphUserVisible() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool isSubGraphPersistent() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual void setupInitialSubGraphState() OVERRIDE FINAL;

    virtual void refreshDynamicProperties() OVERRIDE FINAL;
private:


    virtual void initializeKnobs() OVERRIDE FINAL;
    virtual void onKnobsAboutToBeLoaded(const SERIALIZATION_NAMESPACE::NodeSerialization& serialization) OVERRIDE FINAL;
    virtual bool knobChanged(const KnobIPtr& k,
                             ValueChangedReasonEnum reason,
                             ViewSetSpec view,
                             TimeValue time) OVERRIDE FINAL;
    virtual ActionRetCodeEnum getFrameRange(double *first, double *last) OVERRIDE FINAL;

    boost::scoped_ptr<ReadNodePrivate> _imp;
};


inline ReadNodePtr
toReadNode(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<ReadNode>(effect);
}


NATRON_NAMESPACE_EXIT

#endif // READNODE_H
