/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_OFXCLIPINSTANCE_H
#define NATRON_ENGINE_OFXCLIPINSTANCE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <cassert>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QMutex>
CLANG_DIAG_ON(deprecated)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif
//ofx
// ofxhPropertySuite.h:565:37: warning: 'this' pointer cannot be null in well-defined C++ code; comparison may be assumed to always evaluate to true [-Wtautological-undefined-compare]
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare) // appeared in clang 3.5
#include <ofxhImageEffect.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)
#include <ofxPixels.h>

#include "Global/GlobalDefines.h"

#include "Engine/Image.h"
#include "Engine/ThreadStorage.h"
#include "Engine/ImageComponents.h"

class OfxImage;
class OfxEffectInstance;
namespace Transform
{
struct Matrix3x3;
}
namespace Natron {
class EffectInstance;
class GenericAccess;
class OfxImageEffectInstance;
class Image;
class Node;
}

class OfxClipInstance
    : public OFX::Host::ImageEffect::ClipInstance
{
public:
    OfxClipInstance(OfxEffectInstance* node
                    ,
                    Natron::OfxImageEffectInstance* effect
                    ,
                    int index
                    ,
                    OFX::Host::ImageEffect::ClipDescriptor* desc);

    virtual ~OfxClipInstance();
    /// Get the Raw Unmapped Pixel Depth from the host
    ///
    /// \returns
    ///    - kOfxBitDepthNone (implying a clip is unconnected image)
    ///    - kOfxBitDepthByte
    ///    - kOfxBitDepthShort
    ///    - kOfxBitDepthHalf
    ///    - kOfxBitDepthFloat
    const std::string &getUnmappedBitDepth() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /// Get the Raw Unmapped Components from the host
    ///
    /// \returns
    ///     - kOfxImageComponentNone (implying a clip is unconnected, not valid for an image)
    ///     - kOfxImageComponentRGBA
    ///     - kOfxImageComponentAlpha
    virtual const std::string &getUnmappedComponents() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    // PreMultiplication -
    //
    //  kOfxImageOpaque - the image is opaque and so has no premultiplication state
    //  kOfxImagePreMultiplied - the image is premultiplied by it's alpha
    //  kOfxImageUnPreMultiplied - the image is unpremultiplied
    virtual const std::string &getPremult() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    // Pixel Aspect Ratio -
    //
    //  The pixel aspect ratio of a clip or image.
    virtual double getAspectRatio() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    void setAspectRatio(double par);
    
    // Frame Rate -
    //
    //  The frame rate of a clip or instance's project.
    virtual double getFrameRate() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    // Frame Range (startFrame, endFrame) -
    //
    //  The frame range over which a clip has images.
    virtual void getFrameRange(double &startFrame, double &endFrame) const OVERRIDE FINAL;

    /// Field Order - Which spatial field occurs temporally first in a frame.
    /// \returns
    ///  - kOfxImageFieldNone - the clip material is unfielded
    ///  - kOfxImageFieldLower - the clip material is fielded, with image rows 0,2,4.... occuring first in a frame
    ///  - kOfxImageFieldUpper - the clip material is fielded, with image rows line 1,3,5.... occuring first in a frame
    virtual const std::string &getFieldOrder() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    // Connected -
    //
    //  Says whether the clip is actually connected at the moment.
    virtual bool getConnected() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    // Unmapped Frame Rate -
    //
    //  The unmaped frame range over which an output clip has images.
    virtual double getUnmappedFrameRate() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    // Unmapped Frame Range -
    //
    //  The unmaped frame range over which an output clip has images.
    virtual void getUnmappedFrameRange(double &unmappedStartFrame, double &unmappedEndFrame) const OVERRIDE FINAL;

    // Continuous Samples -
    //
    //  0 if the images can only be sampled at discreet times (eg: the clip is a sequence of frames),
    //  1 if the images can only be sampled continuously (eg: the clip is infact an animating roto spline and can be rendered anywhen).
    virtual bool getContinuousSamples() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    /// Returns the components present on the effect. Much like getComponents() it can also
    /// return components from other planes.
    /// Returns a vector since the function getStringPropertyN does not exist. Only getStringProperty
    /// with an index exists.
    virtual const std::vector<std::string>& getComponentsPresent() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    
    virtual int getDimension(const std::string &name) const OFX_EXCEPTION_SPEC OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    
    /// override this to fill in the image at the given time.
    /// The bounds of the image on the image plane should be
    /// 'appropriate', typically the value returned in getRegionsOfInterest
    /// on the effect instance. Outside a render call, the optionalBounds should
    /// be 'appropriate' for the.
    /// If bounds is not null, fetch the indicated section of the canonical image plane.
    virtual OFX::Host::ImageEffect::Image* getImage(OfxTime time, const OfxRectD *optionalBounds) OVERRIDE FINAL WARN_UNUSED_RETURN;

    /// override this to return the rod on the clip
    virtual OfxRectD getRegionOfDefinition(OfxTime time) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /// override this to fill in the image at the given time from a specific view
    /// (using the standard callback gets you the current view being rendered, @see getImage).
    /// The bounds of the image on the image plane should be
    /// 'appropriate', typically the value returned in getRegionsOfInterest
    /// on the effect instance. Outside a render call, the optionalBounds should
    /// be 'appropriate' for the.
    /// If bounds is not null, fetch the indicated section of the canonical image plane.
    /// If view is -1, guess it (e.g. from the last renderargs)
    virtual OFX::Host::ImageEffect::Image* getStereoscopicImage(OfxTime time, int view, const OfxRectD *optionalBounds) OVERRIDE FINAL WARN_UNUSED_RETURN;
#     ifdef OFX_SUPPORTS_OPENGLRENDER
    /// override this to fill in the OpenGL texture at the given time.
    /// The bounds of the image on the image plane should be
    /// 'appropriate', typically the value returned in getRegionsOfInterest
    /// on the effect instance. Outside a render call, the optionalBounds should
    /// be 'appropriate' for the.
    /// If bounds is not null, fetch the indicated section of the canonical image plane.
    virtual OFX::Host::ImageEffect::Texture* loadTexture(OfxTime time, const char *format, const OfxRectD *optionalBounds) OVERRIDE FINAL;
#     endif

    virtual OFX::Host::ImageEffect::Image* getImagePlane(OfxTime time, int view, const std::string& plane,const OfxRectD *optionalBounds) OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    /// override this to return the rod on the clip for the given view
    virtual OfxRectD getRegionOfDefinition(OfxTime time, int view) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /// given the colour component, find the nearest set of supported colour components
    /// override this for extra wierd custom component depths
    virtual const std::string &findSupportedComp(const std::string &s) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual const std::string &getComponents() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /// override this to set the view to be returned by getImage()
    /// This is called by Instance::renderAction() for each clip, before calling
    /// kOfxImageEffectActionRender on the Instance.
    /// The view number has to be stored in the Clip, so this is typically not thread-safe,
    /// except if thread-local storage is used
    //// EDIT: We don't use this function anymore, instead we handle thread storage ourselves in OfxEffectInstance
    //// via the ClipsThreadStorageSetter, this way we can be sure actions are not called recursively and do other checks.
    virtual void setView(int /*view*/) OVERRIDE
    {
    }

    void setRenderedView(int view);

    ///Set the view stored in the thread-local storage to be invalid
    void discardView();

    void setMipMapLevel(unsigned int mipMapLevel);
    
    void discardMipMapLevel();
    
    void clearOfxImagesTLS();
    
    void setClipComponentTLS(bool hasImage,const Natron::ImageComponents& components);
    void clearClipComponentsTLS();

    //returns the index of this clip if it is an input clip, otherwise -1.
    int getInputNb() const WARN_UNUSED_RETURN;

    Natron::EffectInstance* getAssociatedNode() const WARN_UNUSED_RETURN;
    
    Natron::ImageComponents ofxPlaneToNatronPlane(const std::string& plane);
    static std::string natronsPlaneToOfxPlane(const Natron::ImageComponents& plane);
    static std::string natronsComponentsToOfxComponents(const Natron::ImageComponents& comp);
    static std::list<Natron::ImageComponents> ofxComponentsToNatronComponents(const std::string & comp);
    static Natron::ImageBitDepthEnum ofxDepthToNatronDepth(const std::string & depth);
    static std::string natronsDepthToOfxDepth(Natron::ImageBitDepthEnum depth);

    void setTransformAndReRouteInput(const Transform::Matrix3x3& m,Natron::EffectInstance* rerouteInput,int newInputNb);
    void clearTransform();
    
private:

    /**
     * @brief These are datas that are local to an action call but that we need in order to perform the API call like
     * clipGetRegionOfDefinition or clipGetFrameRange, etc...
     * The mipmapLevel and time are NOT stored here since they can be recovered by other means, that is:
     * - the thread-storage of the render args of the associated nodes when the thread is rendering (in a render call)
     * - The current time of the timeline otherwise and 0 for mipMapLevel.
     **/
    struct ActionLocalData {
        
        bool isViewValid;
        int view;
        
        bool isMipmapLevelValid;
        unsigned int mipMapLevel;
        
        bool isTransformDataValid;
        boost::shared_ptr<Transform::Matrix3x3> matrix; //< if the clip is associated to a node that can transform
        Natron::EffectInstance* rerouteNode; //< if the associated node is a concatenated transform, this is the effect from which to fetch images from
        int rerouteInputNb;
        
        std::list<OfxImage*> imagesBeingRendered;
        
        //We keep track of the input images fetch so we do not attempt to take a lock on the image if it has already been fetched
        std::list<boost::weak_ptr<Natron::Image> > inputImagesFetched;
        
        //String indicating what a subsequent call to getComponents should return
        bool clipComponentsValid;
        Natron::ImageComponents clipComponents;
        bool hasImage;
        
        ActionLocalData()
        : isViewValid(false)
        , view(0)
        , isMipmapLevelValid(false)
        , mipMapLevel(false)
        , isTransformDataValid(false)
        , matrix()
        , rerouteNode(0)
        , rerouteInputNb(-1)
        , imagesBeingRendered()
        , inputImagesFetched()
        , clipComponentsValid(false)
        , clipComponents()
        , hasImage(false)
        {
        }
    };

    
    void getRegionOfDefinitionInternal(OfxTime time,int view, unsigned int mipmapLevel,Natron::EffectInstance* associatedNode,
                                       OfxRectD* rod) const;
    
    OFX::Host::ImageEffect::Image* getInputImageInternal(OfxTime time, int view, const OfxRectD *optionalBounds,
                                                    const std::string* ofxPlane);

    OFX::Host::ImageEffect::Image* getOutputImageInternal(const std::string* ofxPlane);

    OFX::Host::ImageEffect::Image* getImagePlaneInternal(OfxTime time, int view, const OfxRectD *optionalBounds, const std::string* ofxPlane);


    OfxEffectInstance* _nodeInstance;
    Natron::OfxImageEffectInstance* const _effect;
    double _aspectRatio;
 
    mutable Natron::ThreadStorage<ActionLocalData> _lastActionData; //< foreach  thread, the args
    
    
   /* struct CompPresent
    {
        ///The component in question
        Natron::ImageComponentsEnum comp;
        
        ///For input clips, the node from which to fetch the components, otherwise NULL for output clips
        boost::weak_ptr<Natron::Node> node;
    };*/
    
    ///  pair< component, ofxcomponent> 
    typedef std::vector<std::string>  ComponentsPresentMap;
    mutable Natron::ThreadStorage<ComponentsPresentMap> _componentsPresent;
    
    mutable Natron::ThreadStorage<std::string> _unmappedComponents;
    
};

class OfxImage
    : public OFX::Host::ImageEffect::Image
{
public:

    /** @brief Enumerates the pixel depths supported */
    enum BitDepthEnum
    {
        eBitDepthNone = 0,                /**< @brief bit depth that indicates no data is present */
        eBitDepthUByte,
        eBitDepthUShort,
        eBitDepthFloat,
        eBitDepthDouble
    };

    /** @brief Enumerates the component types supported */
    enum PixelComponentEnum
    {
        ePixelComponentNone = 0,
        ePixelComponentRGBA,
        ePixelComponentRGB,
        ePixelComponentAlpha
    };


    explicit OfxImage(std::list<OfxImage*>* tlsImages,
                      boost::shared_ptr<Natron::Image> internalImage,
                      bool isSrcImage,
                      const RectI& renderWindow,
                      const boost::shared_ptr<Transform::Matrix3x3>& mat,
                      const std::string& components,
                      int nComps,
                      OfxClipInstance &clip);

    virtual ~OfxImage();

    boost::shared_ptr<Natron::Image> getInternalImage() const
    {
        return _floatImage;
    }

private:

    boost::shared_ptr<Natron::Image> _floatImage;
    boost::shared_ptr<Natron::GenericAccess> _imgAccess;
    std::list<OfxImage*>* tlsImages;
};

#endif // NATRON_ENGINE_OFXCLIPINSTANCE_H
