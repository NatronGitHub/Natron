/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef DISKCACHENODE_H
#define DISKCACHENODE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/OutputEffectInstance.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

struct DiskCacheNodePrivate;

class DiskCacheNode
    : public OutputEffectInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    static EffectInstance* BuildEffect(NodePtr n)
    {
        return new DiskCacheNode(n);
    }

    DiskCacheNode(NodePtr node);

    virtual ~DiskCacheNode();

    virtual int getMajorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 1;
    }

    virtual int getMinorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 0;
    }

    virtual int getNInputs() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 1;
    }

    virtual bool getCanTransform() const OVERRIDE FINAL WARN_UNUSED_RETURN { return false; }

    virtual std::string getPluginID() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return PLUGINID_NATRON_DISKCACHE;
    }

    virtual std::string getPluginLabel() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return "DiskCache";
    }

    virtual std::string getPluginDescription() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return tr("This node caches all images of the connected input node onto the disk with full 32bit floating point raw data. "
                  "When an image is found in the cache, %1 will then not request the input branch to render out that image. "
                  "The DiskCache node only caches full images and does not split up the images in chunks.  "
                  "The DiskCache node is useful if working with a large and complex node tree: this allows to break the tree into smaller "
                  "branches and cache any branch that you're no longer working on. The cached images are saved by default in the same directory that is used "
                  "for the viewer cache but you can set its location and size in the preferences. A solid state drive disk is recommended for efficiency of this node. "
                  "By default all images that pass into the node are cached but they depend on the zoom-level of the viewer. For convenience you can cache "
                  "a specific frame range at scale 100% much like a writer node would do.\n"
                  "WARNING: The DiskCache node must be part of the tree when you want to read cached data from it.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString();
    }

    virtual void getPluginGrouping(std::list<std::string>* grouping) const OVERRIDE FINAL
    {
        grouping->push_back(PLUGIN_GROUP_OTHER);
    }

    virtual std::string getInputLabel (int /*inputNb*/) const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return "Source";
    }

    virtual bool isInputOptional(int /*inputNb*/) const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual void addAcceptedComponents(int inputNb, std::list<ImagePlaneDesc>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const OVERRIDE FINAL;

    ///Doesn't really matter here since it won't be used (this effect is always an identity)
    virtual RenderSafetyEnum renderThreadSafety() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return eRenderSafetyFullySafeFrame;
    }

    virtual bool supportsTiles() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool supportsMultiResolution() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool isOutput() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual void initializeKnobs() OVERRIDE FINAL;
    virtual void getFrameRange(double *first, double *last) OVERRIDE FINAL;
    virtual bool getCreateChannelSelectorKnob() const OVERRIDE FINAL WARN_UNUSED_RETURN { return false; }

    virtual bool isHostChannelSelectorSupported(bool* defaultR, bool* defaultG, bool* defaultB, bool* defaultA) const OVERRIDE WARN_UNUSED_RETURN;

private:

    virtual bool knobChanged(KnobI* k,
                             ValueChangedReasonEnum reason,
                             ViewSpec view,
                             double time,
                             bool originatedFromMainThread) OVERRIDE FINAL;
    virtual StatusEnum render(const RenderActionArgs& args) OVERRIDE WARN_UNUSED_RETURN;
    virtual bool shouldCacheOutput(bool isFrameVaryingOrAnimated, double time, ViewIdx view, int visitsCount) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    boost::scoped_ptr<DiskCacheNodePrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // DISKCACHENODE_H
