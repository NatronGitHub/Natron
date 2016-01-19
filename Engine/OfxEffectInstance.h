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
//ofx
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare)
#include <ofxhImageEffect.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)

#include "Engine/OutputEffectInstance.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

class AbstractOfxEffectInstance
    : public OutputEffectInstance
{
public:

    AbstractOfxEffectInstance(boost::shared_ptr<Node> node)
        : OutputEffectInstance(node)
    {
    }

    virtual ~AbstractOfxEffectInstance()
    {
    }

    virtual void createOfxImageEffectInstance(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                              OFX::Host::ImageEffect::Descriptor* desc,
                                              Natron::ContextEnum context,
                                              const NodeSerialization* serialization,
                                               const std::list<boost::shared_ptr<KnobSerialization> >& paramValues,
                                              bool allowFileDialogs,
                                              bool disableRenderScaleSupport,
                                              bool *hasUsedFileDialog) = 0;
    static QStringList makePluginGrouping(const std::string & pluginIdentifier,
                                          int versionMajor, int versionMinor,
                                          const std::string & pluginLabel,
                                          const std::string & grouping) WARN_UNUSED_RETURN;
    static std::string makePluginLabel(const std::string & shortLabel,
                                       const std::string & label,
                                       const std::string & longLabel) WARN_UNUSED_RETURN;
    
};

struct OfxEffectInstancePrivate;
class OfxEffectInstance
    : public AbstractOfxEffectInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    OfxEffectInstance(boost::shared_ptr<Node> node);

    virtual ~OfxEffectInstance();

    void createOfxImageEffectInstance(OFX::Host::ImageEffect::ImageEffectPlugin* plugin,
                                      OFX::Host::ImageEffect::Descriptor* desc,
                                      Natron::ContextEnum context,
                                      const NodeSerialization* serialization,
                                      const std::list<boost::shared_ptr<KnobSerialization> >& paramValues,
                                      bool allowFileDialogs,
                                      bool disableRenderScaleSupport,
                                      bool *hasUsedFileDialog) OVERRIDE FINAL;

    OfxImageEffectInstance* effectInstance() WARN_UNUSED_RETURN;
    
    const OfxImageEffectInstance* effectInstance() const WARN_UNUSED_RETURN;
    
    const std::string & getShortLabel() const WARN_UNUSED_RETURN;

    typedef std::vector<OFX::Host::ImageEffect::ClipDescriptor*> MappedInputV;
    MappedInputV inputClipsCopyWithoutOutput() const WARN_UNUSED_RETURN;

    bool isCreated() const;
    bool isInitialized() const;

    const std::string & ofxGetOutputPremultiplication() const;

    /**
     * @brief Calls syncPrivateDataAction from another thread than the main thread. The actual
     * call of the action will take place in the main-thread.
     **/
    void syncPrivateData_other_thread()
    {
        Q_EMIT syncPrivateDataRequested();
    }

public:
    /********OVERRIDEN FROM EFFECT INSTANCE*************/
    virtual int getMajorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getMinorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isGenerator() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isReader() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isWriter() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isOutput() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isGeneratorAndFilter() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isTrackerNode() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isOpenFX() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }
    virtual bool isEffectCreated() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool makePreviewByDefault() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getMaxInputCount() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::string getPluginID() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::string getPluginLabel() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void getPluginGrouping(std::list<std::string>* grouping) const OVERRIDE FINAL;
    virtual std::string getPluginDescription() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::string getInputLabel (int inputNb) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isInputOptional(int inputNb) const OVERRIDE WARN_UNUSED_RETURN;
    virtual bool isInputMask(int inputNb) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isInputRotoBrush(int inputNb) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual int getRotoBrushInputIndex() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual StatusEnum getRegionOfDefinition(U64 hash, double time, const RenderScale & scale, int view, RectD* rod) OVERRIDE WARN_UNUSED_RETURN;

    /// calculate the default rod for this effect instance
    virtual void calcDefaultRegionOfDefinition(U64 hash, double time, const RenderScale & scale, int view, RectD *rod)  OVERRIDE;
    virtual void getRegionsOfInterest(double time,
                                      const RenderScale & scale,
                                      const RectD & outputRoD, //!< full RoD in canonical coordinates
                                      const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                      int view,
                                      RoIMap* ret) OVERRIDE FINAL;

    virtual FramesNeededMap getFramesNeeded(double time,int view) OVERRIDE WARN_UNUSED_RETURN;
    virtual void getFrameRange(double *first,double *last) OVERRIDE;
    virtual void initializeOverlayInteract() OVERRIDE FINAL;
    virtual bool hasOverlay() const OVERRIDE FINAL;
    virtual void redrawOverlayInteract() OVERRIDE FINAL;
    virtual RenderScale getOverlayInteractRenderScale() const OVERRIDE FINAL;
    virtual void drawOverlay(double time, const RenderScale & renderScale, int view) OVERRIDE FINAL;
    virtual bool onOverlayPenDown(double time, const RenderScale & renderScale, int view, const QPointF & viewportPos, const QPointF & pos, double pressure) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayPenMotion(double time, const RenderScale & renderScale, int view,
                                    const QPointF & viewportPos, const QPointF & pos, double pressure) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayPenUp(double time, const RenderScale & renderScale, int view, const QPointF & viewportPos, const QPointF & pos, double pressure) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayKeyDown(double time, const RenderScale & renderScale, int view, Natron::Key key, Natron::KeyboardModifiers modifiers) OVERRIDE FINAL;
    virtual bool onOverlayKeyUp(double time, const RenderScale & renderScale, int view, Natron::Key key,Natron::KeyboardModifiers modifiers) OVERRIDE FINAL;
    virtual bool onOverlayKeyRepeat(double time, const RenderScale & renderScale, int view, Natron::Key key,Natron::KeyboardModifiers modifiers) OVERRIDE FINAL;
    virtual bool onOverlayFocusGained(double time, const RenderScale & renderScale, int view) OVERRIDE FINAL;
    virtual bool onOverlayFocusLost(double time, const RenderScale & renderScale, int view) OVERRIDE FINAL;
    virtual void setCurrentViewportForOverlays(OverlaySupport* viewport) OVERRIDE FINAL;
    virtual void beginKnobsValuesChanged(ValueChangedReasonEnum reason) OVERRIDE;
    virtual void endKnobsValuesChanged(ValueChangedReasonEnum reason) OVERRIDE;
    virtual void knobChanged(KnobI* k, ValueChangedReasonEnum reason, int view, double time,
                             bool originatedFromMainThread) OVERRIDE;
    virtual void beginEditKnobs() OVERRIDE;
    virtual StatusEnum render(const RenderActionArgs& args) OVERRIDE WARN_UNUSED_RETURN;
    virtual bool isIdentity(double time,
                            const RenderScale & scale,
                            const RectI & renderWindow, //!< render window in pixel coords
                            int view,
                            double* inputTime,
                            int* inputNb) OVERRIDE;
    virtual Natron::RenderSafetyEnum renderThreadSafety() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void purgeCaches() OVERRIDE;

    /**
     * @brief Does this effect supports tiling ?
     * http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsTiles
     * If a clip or plugin does not support tiled images, then the host should supply
     * full RoD images to the effect whenever it fetches one.
     **/
    virtual bool supportsTiles() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool supportsRenderQuality() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual PluginOpenGLRenderSupport supportsOpenGLRender() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool doesTemporalClipAccess() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Does this effect supports multiresolution ?
     * http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsMultiResolution
     * Multiple resolution images mean...
     * input and output images can be of any size
     * input and output images can be offset from the origin
     **/
    virtual bool supportsMultiResolution() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool supportsMultipleClipsPAR() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isHostChannelSelectorSupported(bool* defaultR,bool* defaultG, bool* defaultB, bool* defaultA) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void onInputChanged(int inputNo) OVERRIDE FINAL;
    virtual std::vector<std::string> supportedFileFormats() const OVERRIDE FINAL;
    virtual StatusEnum beginSequenceRender(double first,
                                                   double last,
                                                   double step,
                                                   bool interactive,
                                                   const RenderScale & scale,
                                                   bool isSequentialRender,
                                                   bool isRenderResponseToUserInteraction,
                                                   bool draftMode,
                                                   int view) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual StatusEnum endSequenceRender(double first,
                                                 double last,
                                                 double step,
                                                 bool interactive,
                                                 const RenderScale & scale,
                                                 bool isSequentialRender,
                                                 bool isRenderResponseToUserInteraction,
                                                 bool draftMode,
                                                 int view) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void addAcceptedComponents(int inputNb, std::list<ImageComponents>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const OVERRIDE FINAL;
    virtual void getPreferredDepthAndComponents(int inputNb, std::list<ImageComponents>* comp, ImageBitDepthEnum* depth) const OVERRIDE FINAL;
    virtual Natron::SequentialPreferenceEnum getSequentialPreference() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual ImagePremultiplicationEnum getOutputPremultiplication() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool refreshClipPreferences(double time,
                                        const RenderScale & scale,
                                        ValueChangedReasonEnum reason,
                                        bool forceGetClipPrefAction) OVERRIDE FINAL;
    
    virtual void getComponentsNeededAndProduced(double time, int view,
                                               EffectInstance::ComponentsNeededMap* comps,
                                                SequenceTime* passThroughTime,
                                                int* passThroughView,
                                                boost::shared_ptr<Node>* passThroughInput) OVERRIDE;


    virtual bool isMultiPlanar() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual EffectInstance::PassThroughEnum isPassThroughForNonRenderedPlanes() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isViewAware() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual EffectInstance::ViewInvarianceLevel isViewInvariant() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
public:

    virtual double getPreferredAspectRatio() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual double getPreferredFrameRate() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getCanTransform() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool getInputsHoldingTransform(std::list<int>* inputs) const  OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual StatusEnum getTransform(double time,
                                            const RenderScale & renderScale,
                                            int view,
                                            EffectInstance** inputToTransform,
                                            Transform::Matrix3x3* transform) OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool isFrameVarying() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual bool isHostMaskingEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isHostMixingEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN;


    /********OVERRIDEN FROM EFFECT INSTANCE: END*************/

    OfxClipInstance* getClipCorrespondingToInput(int inputNo) const;

    
    static Natron::ContextEnum mapToContextEnum(const std::string &s);
    static std::string mapContextToString(Natron::ContextEnum ctx);
    
    static std::string natronValueChangedReasonToOfxValueChangedReason(ValueChangedReasonEnum reason);

    int getClipInputNumber(const OfxClipInstance* clip) const;
    
public Q_SLOTS:

    void onSyncPrivateDataRequested();


Q_SIGNALS:

    void syncPrivateDataRequested();

private:
 


    void tryInitializeOverlayInteracts();

    void initializeContextDependentParams();


    
private:

    boost::scoped_ptr<OfxEffectInstancePrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_OFXNODE_H
