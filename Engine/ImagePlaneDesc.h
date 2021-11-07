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

#ifndef IMAGECOMPONENTS_H
#define IMAGECOMPONENTS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>
#include <vector>

#if (!defined(Q_MOC_RUN) && !defined(SBK_RUN)) || defined(SBK2_GEN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
#endif

#define IMAGEPLANEDESC_SERIALIZATION_INTRODUCES_ID 2
#define IMAGEPLANEDESC_SERIALIZATION_VERSION IMAGEPLANEDESC_SERIALIZATION_INTRODUCES_ID

#include <nuke/fnOfxExtensions.h>
#include "Engine/EngineFwd.h"
#include "Engine/ChoiceOption.h"

#define kNatronColorPlaneID kFnOfxImagePlaneColour
#define kNatronColorPlaneLabel "Color"

#define kNatronBackwardMotionVectorsPlaneID kFnOfxImagePlaneBackwardMotionVector
#define kNatronBackwardMotionVectorsPlaneLabel "Backward"

#define kNatronForwardMotionVectorsPlaneID kFnOfxImagePlaneForwardMotionVector
#define kNatronForwardMotionVectorsPlaneLabel "Forward"

#define kNatronDisparityLeftPlaneID kFnOfxImagePlaneStereoDisparityLeft
#define kNatronDisparityLeftPlaneLabel "DisparityLeft"

#define kNatronDisparityRightPlaneID kFnOfxImagePlaneStereoDisparityRight
#define kNatronDisparityRightPlaneLabel "DisparityRight"

#define kNatronDisparityComponentsLabel "Disparity"
#define kNatronMotionComponentsLabel "Motion"

NATRON_NAMESPACE_ENTER

class ImagePlaneDesc
{
public:

    ImagePlaneDesc();

    ImagePlaneDesc(const std::string& planeID,
                   const std::string& planeLabel,
                   const std::string& channelsLabel,
                   const std::vector<std::string>& channels);

    ImagePlaneDesc(const std::string& planeID,
                   const std::string& planeLabel,
                   const std::string& channelsLabel,
                   const char** chanels,
                   int count);


    ImagePlaneDesc(const ImagePlaneDesc& other);

    ImagePlaneDesc& operator=(const ImagePlaneDesc& other);

    ~ImagePlaneDesc();

    // Is it Alpha, RGB or RGBA
    bool isColorPlane() const;

    static bool isColorPlane(const std::string& layerID);

    /**
     * @brief Returns the number of channels in this plane.
     **/
    int getNumComponents() const;

    /**
     * @brief Returns the plane unique identifier. This should be used to compare ImagePlaneDesc together.
     * This is not supposed to be used for display purpose, use getPlaneLabel() instead.
     **/
    const std::string& getPlaneID() const;

    /**
     * @brief Returns the plane label.
     * This is what is used to display to the user.
     **/
    const std::string& getPlaneLabel() const;

    /**
     * @brief Returns the channels composing this plane.
     **/
    const std::vector<std::string>& getChannels() const;

    /**
     * @brief Returns a label used to better represent the type of components used by this plane.
     * e.g: "Motion" can be used to better label "XY" component types.
     **/
    const std::string& getChannelsLabel() const;


    bool operator==(const ImagePlaneDesc& other) const;

    bool operator!=(const ImagePlaneDesc& other) const
    {
        return !(*this == other);
    }

    // For std::map
    bool operator<(const ImagePlaneDesc& other) const;

    operator bool() const
    {
        return getNumComponents() > 0;
    }

    bool operator!() const
    {
        return getNumComponents() == 0;
    }


    ChoiceOption getPlaneOption() const;
    ChoiceOption getChannelOption(int channelIndex) const;

    /**
     * @brief Maps the given nComps to the color plane
     **/
    static const ImagePlaneDesc& mapNCompsToColorPlane(int nComps);

    /**
     * @brief Maps the given OpenFX plane to a ImagePlaneDesc.
     * @param ofxPlane Can be

     *  kFnOfxImagePlaneBackwardMotionVector
     *  kFnOfxImagePlaneForwardMotionVector
     *  kFnOfxImagePlaneStereoDisparityLeft
     *  kFnOfxImagePlaneStereoDisparityRight
     *  Or any plane encoded in the format specified by the Natron multi-plane extension.
     * This function CANNOT be used to map the color plane, instead use mapNCompsToColorPlane.
     *
     * This function returns an empty plane desc upon failure.
     **/
    static ImagePlaneDesc mapOFXPlaneStringToPlane(const std::string& ofxPlane);

    /**
     * @brief Maps OpenFX components string to a plane, optionnally also to a paired plane in the case of disparity/motion vectors.
     * @param ofxComponents Must be a string between
     * kOfxImageComponentRGBA, kOfxImageComponentRGB, kOfxImageComponentAlpha, kNatronOfxImageComponentXY, kOfxImageComponentNone
     * or kFnOfxImageComponentStereoDisparity or kFnOfxImageComponentMotionVectors
     * Or any plane encoded in the format specified by the Natron multi-plane extension.
     **/
    static void mapOFXComponentsTypeStringToPlanes(const std::string& ofxComponents, ImagePlaneDesc* plane, ImagePlaneDesc* pairedPlane);

    /**
     * @brief Does the inverse of mapOFXPlaneStringToPlane, except that it can also be used for
     * the color plane.
     **/
    static std::string mapPlaneToOFXPlaneString(const ImagePlaneDesc& plane);

    /**
     * @brief Returns an OpenFX encoded string representing the components type of the plane.
     * @returns One of the following strings:
     * kOfxImageComponentRGBA, kOfxImageComponentRGB, kOfxImageComponentAlpha, kNatronOfxImageComponentXY, kOfxImageComponentNone
     * or kFnOfxImageComponentStereoDisparity or kFnOfxImageComponentMotionVectors
     * Or any plane encoded in the format specified by the Natron multi-plane extension.
     **/
    static std::string mapPlaneToOFXComponentsTypeString(const ImagePlaneDesc& plane);

    /**
     * @brief Find a layer equivalent to this layer in the other layers container.
     * ITERATOR must be either a std::vector<ImagePlaneDesc>::iterator or std::list<ImagePlaneDesc>::iterator
     **/
    template <typename ITERATOR>
    static ITERATOR findEquivalentLayer(const ImagePlaneDesc& layer, ITERATOR begin, ITERATOR end)
    {
        bool isColor = layer.isColorPlane();

        ITERATOR foundExistingColorMatch = end;
        ITERATOR foundExistingComponents = end;

        for (ITERATOR it = begin; it != end; ++it) {
            if (it->isColorPlane() && isColor) {
                foundExistingColorMatch = it;
            } else {
                if (*it == layer) {
                    foundExistingComponents = it;
                    break;
                }
            }
        } // for each output components

        if (foundExistingComponents != end) {
            return foundExistingComponents;
        } else if (foundExistingColorMatch != end) {
            return foundExistingColorMatch;
        } else {
            return end;
        }
    } // findEquivalentLayer

    /*
     * These are default presets image components
     */
    static const ImagePlaneDesc& getNoneComponents();
    static const ImagePlaneDesc& getRGBAComponents();
    static const ImagePlaneDesc& getRGBComponents();
    static const ImagePlaneDesc& getAlphaComponents();
    static const ImagePlaneDesc& getBackwardMotionComponents();
    static const ImagePlaneDesc& getForwardMotionComponents();
    static const ImagePlaneDesc& getDisparityLeftComponents();
    static const ImagePlaneDesc& getDisparityRightComponents();
    static const ImagePlaneDesc& getXYComponents();


    template<class Archive>
    void save(Archive & ar, const unsigned int version) const;

    template<class Archive>
    void load(Archive & ar, const unsigned int version);

private:
    std::string _planeID, _planeLabel;
    std::vector<std::string> _channels;
    std::string _channelsLabel;
    
    friend class boost::serialization::access;

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

NATRON_NAMESPACE_EXIT

BOOST_CLASS_VERSION(NATRON_NAMESPACE::ImagePlaneDesc, IMAGEPLANEDESC_SERIALIZATION_VERSION)


#endif // IMAGECOMPONENTS_H
