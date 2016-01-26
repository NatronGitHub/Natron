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

#ifndef NATRON_ENGINE_OFXCLIPINSTANCE_H
#define NATRON_ENGINE_OFXCLIPINSTANCE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <cassert>
#include <list>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QMutex>
CLANG_DIAG_ON(deprecated)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/scoped_ptr.hpp>
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
#include "Engine/ImageComponents.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

struct OfxClipInstancePrivate;
class OfxClipInstance
    : public OFX::Host::ImageEffect::ClipInstance
{
public:
    OfxClipInstance(const boost::shared_ptr<OfxEffectInstance>& node,
                    OfxImageEffectInstance* effect,
                    int index,
                    OFX::Host::ImageEffect::ClipDescriptor* desc);

    virtual ~OfxClipInstance();
    
    bool getIsOptional() const;
    bool getIsMask() const;
    
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

    void setClipTLS(int view,
                    unsigned int mipmapLevel,
                    const ImageComponents& components);
    void invalidateClipTLS();

    //returns the index of this clip if it is an input clip, otherwise -1.
    int getInputNb() const WARN_UNUSED_RETURN;

    EffectInstPtr getAssociatedNode() const WARN_UNUSED_RETURN;
    
    ImageComponents ofxPlaneToNatronPlane(const std::string& plane);
    static std::string natronsPlaneToOfxPlane(const ImageComponents& plane);
    static std::string natronsComponentsToOfxComponents(const ImageComponents& comp);
    static std::list<ImageComponents> ofxComponentsToNatronComponents(const std::string & comp);
    static ImageBitDepthEnum ofxDepthToNatronDepth(const std::string & depth);
    static std::string natronsDepthToOfxDepth(ImageBitDepthEnum depth);




    struct RenderActionData
    {
    //We keep track of the images being rendered (on the output clip) so that we return the same pointer
        //if this is the same image
        std::list<OfxImage*> imagesBeingRendered;
        
        //Used to determine the plane to render in a call to getOutputImageInternal()
        ImageComponents clipComponents;
        
        RenderActionData()
        : imagesBeingRendered()
        , clipComponents()
        {
            
        }
    
    };

    //These are per-clip thread-local data
    struct ClipTLSData
    {
        //////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////
        ///////These data are valid only throughout a recursive action
        
        //View may be involved in a recursive action
        std::list<int> view;
        //mipmaplevel may be involved in a recursive action
        std::list<unsigned int> mipMapLevel;
        
        //////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////
        /// Valid only throughought a render action
        std::list< boost::shared_ptr<RenderActionData> > renderData;
        //////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////
        
        
        //////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////
        //The following data are always valid and do not necessitate a valid flag
        std::vector<std::string> componentsPresent;
        std::string unmappedComponents;
        
        ClipTLSData()
        : view()
        , mipMapLevel()
        , componentsPresent()
        , unmappedComponents()
        {
        }
        
        ClipTLSData(const ClipTLSData& other)
        : view(other.view)
        , mipMapLevel(other.mipMapLevel)
        , renderData()
        , componentsPresent(other.componentsPresent)
        , unmappedComponents(other.unmappedComponents)
        {
            for (std::list< boost::shared_ptr<RenderActionData> >::const_iterator it = other.renderData.begin();
                 it!= other.renderData.end(); ++it) {
                boost::shared_ptr<RenderActionData> d(new RenderActionData(**it));
                renderData.push_back(d);
            }
        }
    };

    typedef boost::shared_ptr<ClipTLSData> ClipDataTLSPtr;

private:


    
    void getRegionOfDefinitionInternal(OfxTime time,int view, unsigned int mipmapLevel,EffectInstance* associatedNode,
                                       OfxRectD* rod) const;
    
    OFX::Host::ImageEffect::Image* getInputImageInternal(OfxTime time, int view, const OfxRectD *optionalBounds,
                                                    const std::string* ofxPlane);

    OFX::Host::ImageEffect::Image* getOutputImageInternal(const std::string* ofxPlane);

    OFX::Host::ImageEffect::Image* getImagePlaneInternal(OfxTime time, int view, const OfxRectD *optionalBounds, const std::string* ofxPlane);



    boost::scoped_ptr<OfxClipInstancePrivate> _imp;
    
};


struct OfxImagePrivate;
class OfxImage
    : public OFX::Host::ImageEffect::Image
{
public:


    explicit OfxImage(const boost::shared_ptr<OfxClipInstance::RenderActionData>& renderData,
                      const boost::shared_ptr<NATRON_NAMESPACE::Image>& internalImage,
                      bool isSrcImage,
                      const RectI& renderWindow,
                      const boost::shared_ptr<Transform::Matrix3x3>& mat,
                      const std::string& components,
                      int nComps,
                      OfxClipInstance &clip);

    virtual ~OfxImage();

    boost::shared_ptr<NATRON_NAMESPACE::Image> getInternalImage() const;

private:
    
    boost::scoped_ptr<OfxImagePrivate> _imp;

};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_OFXCLIPINSTANCE_H
