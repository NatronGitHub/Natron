/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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

#ifndef NATRON_ENGINE_OFXNODE_H
#define NATRON_ENGINE_OFXNODE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>
#include <string>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
#include <QtCore/QStringList>

#include "Engine/EffectInstance.h"
#include "Engine/ViewIdx.h"

#include "Engine/EngineFwd.h"


#define kNatronPersistentErrorOpenFXPlugin "NatronPersistentErrorOpenFXPlugin"


NATRON_NAMESPACE_ENTER


class AbstractOfxEffectInstance
    : public EffectInstance
{
protected: // derives from EffectInstance, parent of OfxEffectInstance
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    AbstractOfxEffectInstance(const NodePtr& node)
        : EffectInstance(node)
    {
    }

    AbstractOfxEffectInstance(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key)
    : EffectInstance(mainInstance, key)
    {
    }

public:

    virtual ~AbstractOfxEffectInstance()
    {
    }

    static std::vector<std::string> makePluginGrouping(const std::string & pluginIdentifier,
                                          int versionMajor, int versionMinor,
                                          const std::string & pluginLabel,
                                          const std::string & grouping) WARN_UNUSED_RETURN;
    static std::string makePluginLabel(const std::string & shortLabel,
                                       const std::string & label,
                                       const std::string & longLabel) WARN_UNUSED_RETURN;
};

class ThreadIsActionCaller_RAII
{

public:

    ThreadIsActionCaller_RAII(const OfxEffectInstancePtr& effect);

    ~ThreadIsActionCaller_RAII();
};

struct OfxEffectInstancePrivate;
class OfxEffectInstance
    : public AbstractOfxEffectInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private: // derives from EffectInstance
    // TODO: enable_shared_from_this
    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>
    OfxEffectInstance(const NodePtr& node);

    OfxEffectInstance(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key);
public:
    static EffectInstancePtr create(const NodePtr& node) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr( new OfxEffectInstance(node) );
    }

    static EffectInstancePtr createRenderClone(const EffectInstancePtr& mainInstance, const FrameViewRenderKey& key) WARN_UNUSED_RETURN
    {
        return EffectInstancePtr(new  OfxEffectInstance(mainInstance, key));
    }

    virtual ~OfxEffectInstance();


    OfxImageEffectInstance* effectInstance() WARN_UNUSED_RETURN;
    const OfxImageEffectInstance* effectInstance() const WARN_UNUSED_RETURN;
    const std::string & getShortLabel() const WARN_UNUSED_RETURN;

    typedef std::vector<OFX::Host::ImageEffect::ClipDescriptor*> MappedInputV;
    MappedInputV inputClipsCopyWithoutOutput() const WARN_UNUSED_RETURN;

    bool isInitialized() const;

    /**
     * @brief Calls syncPrivateDataAction from another thread than the main thread. The actual
     * call of the action will take place in the main-thread.
     **/
    void syncPrivateData_other_thread();

public:
    /********OVERRIDEN FROM EFFECT INSTANCE*************/
    virtual void describePlugin() OVERRIDE FINAL;
    virtual void createInstanceAction() OVERRIDE FINAL;
    virtual bool isGenerator() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isReader() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isVideoReader() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isWriter() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isVideoWriter() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isOutput() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isFilter() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void onScriptNameChanged(const std::string& fullyQualifiedName) OVERRIDE FINAL;
    virtual ActionRetCodeEnum getRegionOfDefinition(TimeValue time, const RenderScale & scale, ViewIdx view, RectD* rod) OVERRIDE WARN_UNUSED_RETURN;

    /// calculate the default rod for this effect instance
    virtual void calcDefaultRegionOfDefinition(TimeValue time, const RenderScale & scale, ViewIdx view,  RectD *rod)  OVERRIDE;
    virtual ActionRetCodeEnum getRegionsOfInterest(TimeValue time,
                                      const RenderScale & scale,
                                      const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                      ViewIdx view,
                                      RoIMap* ret) OVERRIDE FINAL;
    virtual ActionRetCodeEnum getFramesNeeded(TimeValue time, ViewIdx view,FramesNeededMap* framesNeeded) OVERRIDE WARN_UNUSED_RETURN;
    virtual ActionRetCodeEnum getFrameRange(double *first, double *last) OVERRIDE FINAL;
    virtual void initializeOverlayInteract() OVERRIDE FINAL;

    virtual bool canHandleRenderScaleForOverlays() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void beginKnobsValuesChanged(ValueChangedReasonEnum reason) OVERRIDE;
    virtual void endKnobsValuesChanged(ValueChangedReasonEnum reason) OVERRIDE;
    virtual bool knobChanged(const KnobIPtr& k,
                             ValueChangedReasonEnum reason,
                             ViewSetSpec view,
                             TimeValue time) OVERRIDE;
    virtual void beginEditKnobs() OVERRIDE;
    virtual ActionRetCodeEnum render(const RenderActionArgs& args) OVERRIDE WARN_UNUSED_RETURN;
    virtual ActionRetCodeEnum isIdentity(TimeValue time,
                                         const RenderScale & scale,
                                         const RectI & renderWindow, //!< render window in pixel coords
                                         ViewIdx view,
                                         const ImagePlaneDesc& plane,
                                         TimeValue* inputTime,
                                         ViewIdx* inputView,
                                         int* inputNb,
                                         ImagePlaneDesc* identityPlane) OVERRIDE;
    virtual void purgeCaches() OVERRIDE;

    virtual void onInputChanged(int inputNo) OVERRIDE FINAL;
    virtual ActionRetCodeEnum beginSequenceRender(double first,
                                           double last,
                                           double step,
                                           bool interactive,
                                           const RenderScale & scale,
                                           bool isSequentialRender,
                                           bool isRenderResponseToUserInteraction,
                                           bool draftMode,
                                           ViewIdx view,
                                           RenderBackendTypeEnum backendType,
                                           const EffectOpenGLContextDataPtr& glContextData) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual ActionRetCodeEnum endSequenceRender(double first,
                                         double last,
                                         double step,
                                         bool interactive,
                                         const RenderScale & scale,
                                         bool isSequentialRender,
                                         bool isRenderResponseToUserInteraction,
                                         bool draftMode,
                                         ViewIdx view,
                                         RenderBackendTypeEnum backendType,
                                         const EffectOpenGLContextDataPtr& glContextData) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual ActionRetCodeEnum getTimeInvariantMetadata(NodeMetadata& metadata) OVERRIDE FINAL;
    virtual void onMetadataChanged(const NodeMetadata& metadata) OVERRIDE FINAL;
    virtual ActionRetCodeEnum getLayersProducedAndNeeded(TimeValue time, ViewIdx view, std::map<int, std::list<ImagePlaneDesc> >* inputLayersNeeded, std::list<ImagePlaneDesc>* layersProduced, TimeValue* passThroughTime, ViewIdx* passThroughView, int* passThroughInputNb) OVERRIDE;
    virtual bool supportsConcurrentOpenGLRenders() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual ActionRetCodeEnum attachOpenGLContext(TimeValue time, ViewIdx view, const RenderScale& scale, const OSGLContextPtr& glContext, EffectOpenGLContextDataPtr* data) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual ActionRetCodeEnum dettachOpenGLContext(const OSGLContextPtr& glContext, const EffectOpenGLContextDataPtr& data) OVERRIDE FINAL;

    virtual EffectInstanceTLSDataPtr getTLSObject() const OVERRIDE FINAL;
    virtual EffectInstanceTLSDataPtr getTLSObjectForThread(QThread* thread) const OVERRIDE FINAL;
    virtual EffectInstanceTLSDataPtr getOrCreateTLSObject() const OVERRIDE FINAL;

private:

    virtual PluginMemoryPtr createPluginMemory() OVERRIDE FINAL WARN_UNUSED_RETURN;

public:
#ifdef DEBUG
    virtual void checkCanSetValueAndWarn() OVERRIDE FINAL;
#endif

public:


    virtual ActionRetCodeEnum getInverseDistortion(TimeValue time,
                                            const RenderScale & renderScale,
                                            bool draftRender,
                                            ViewIdx view,
                                            DistortionFunction2D* distortion) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void onPropertiesChanged(const EffectDescription& description) OVERRIDE FINAL;
    virtual void refreshDynamicProperties() OVERRIDE FINAL;

    /********OVERRIDDEN FROM EFFECT INSTANCE: END*************/
    OfxClipInstance* getClipCorrespondingToInput(int inputNo) const;
    static ContextEnum mapToContextEnum(const std::string &s);
    static std::string mapContextToString(ContextEnum ctx);
    static std::string natronValueChangedReasonToOfxValueChangedReason(ValueChangedReasonEnum reason);

    int getClipInputNumber(const OfxClipInstance* clip) const;

    void onClipLabelChanged(int inputNb, const std::string& label);
    void onClipHintChanged(int inputNb, const std::string& hint);
    void onClipSecretChanged(int inputNb, bool isSecret);

    const std::vector<std::string>& getUserPlanes() const;


public Q_SLOTS:

    void onSyncPrivateDataRequested();


Q_SIGNALS:

    void syncPrivateDataRequested();

private:


    void tryInitializeOverlayInteracts();

private:

    boost::scoped_ptr<OfxEffectInstancePrivate> _imp;
};


inline AbstractOfxEffectInstancePtr
toAbstractOfxEffectInstance(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<AbstractOfxEffectInstance>(effect);
}

inline OfxEffectInstancePtr
toOfxEffectInstance(const EffectInstancePtr& effect)
{
    return boost::dynamic_pointer_cast<OfxEffectInstance>(effect);
}


NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_OFXNODE_H
