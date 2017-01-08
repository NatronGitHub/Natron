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
#include <list>

#include <nuke/fnOfxExtensions.h>
#include "Engine/EngineFwd.h"
#include "Engine/ChoiceOption.h"
#include "Serialization/SerializationBase.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

#define kNatronColorPlaneName "Color"
#define kNatronBackwardMotionVectorsPlaneName "Backward"
#define kNatronForwardMotionVectorsPlaneName "Forward"
#define kNatronDisparityLeftPlaneName "DisparityLeft"
#define kNatronDisparityRightPlaneName "DisparityRight"

#define kNatronDisparityComponentsName "Disparity"
#define kNatronMotionComponentsName "Motion"

class ImageComponents : public SERIALIZATION_NAMESPACE::SerializableObjectBase
{
public:


    ImageComponents();

    ImageComponents(const std::string& layerName,
                    const std::string& globalCompName,
                    const std::vector<std::string>& componentsName);

    ImageComponents(const std::string& layerName,
                    const std::string& globalCompName,
                    const char** componentsName,
                    int count);

    ImageComponents(const std::string& layerName,
                    const std::string& pairedLayer,
                    const std::string& globalCompName,
                    const char** componentsName,
                    int count);

    ImageComponents(const ImageComponents& other)
        : _layerName(other._layerName)
        , _pairedLayer(other._pairedLayer)
        , _componentNames(other._componentNames)
        , _globalComponentsName(other._globalComponentsName)
    {
    }

    ImageComponents& operator=(const ImageComponents& other)
    {
        _layerName = other._layerName;
        _pairedLayer = other._pairedLayer;
        _componentNames = other._componentNames;
        _globalComponentsName = other._globalComponentsName;

        return *this;
    }

    ~ImageComponents();

    // Is it Alpha, RGB or RGBA
    bool isColorPlane() const;

    static bool isColorPlane(const std::string& layerName);

    // Only color plane (isColorPlane()) can be convertible
    bool isConvertibleTo(const ImageComponents& other) const;

    int getNumComponents() const;

    ///E.g color
    const std::string& getLayerName() const;

    //True if stereo disparity or motion vectors as per Nuke ofx multi-plane extension
    bool isPairedComponents() const;

    //if isPairedComponents() returns true, return the other layer that is paired to this layer.
    const std::string& getPairedLayerName() const;

    ///E.g ["r","g","b","a"]
    const std::vector<std::string>& getComponentsNames() const;

    ///E.g "rgba"
    const std::string& getComponentsGlobalName() const;

    /**
     * @brief If this layer is paired (i.e: isPairedComponents()) then returns true
     * and set the pairedLayer
     **/
    bool isEqualToPairedPlane(const ImageComponents& other, ImageComponents* pairedLayer) const;

    bool getPlanesPair(ImageComponents* first, ImageComponents* second) const;

    bool operator==(const ImageComponents& other) const;

    bool operator!=(const ImageComponents& other) const
    {
        return !(*this == other);
    }

    //For std::map
    bool operator<(const ImageComponents& other) const;

    operator bool() const
    {
        return getNumComponents() > 0;
    }

    bool operator!() const
    {
        return getNumComponents() == 0;
    }


    ChoiceOption getLayerOption() const;
    ChoiceOption getChannelOption(int channelIndex) const;

    /**
     * @brief Find a layer equivalent to this layer in the other layers container.
     * ITERATOR must be either a std::vector<ImageComponents>::iterator or std::list<ImageComponents>::iterator
     **/
    template <typename ITERATOR>
    static ITERATOR findEquivalentLayer(const ImageComponents& layer, ITERATOR begin, ITERATOR end)
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
    static const ImageComponents& getNoneComponents();
    static const ImageComponents& getRGBAComponents();
    static const ImageComponents& getRGBComponents();
    static const ImageComponents& getAlphaComponents();
    static const ImageComponents& getBackwardMotionComponents();
    static const ImageComponents& getForwardMotionComponents();
    static const ImageComponents& getDisparityLeftComponents();
    static const ImageComponents& getDisparityRightComponents();
    static const ImageComponents& getXYComponents();

    //These are to be compatible with Nuke multi-plane extension and the Furnace plug-ins
    static const ImageComponents& getPairedMotionVectors();
    static const ImageComponents& getPairedStereoDisparity();

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj) OVERRIDE FINAL;

    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase & obj) OVERRIDE FINAL;


private:
    std::string _layerName;
    std::string _pairedLayer;
    std::vector<std::string> _componentNames;
    std::string _globalComponentsName;
};

NATRON_NAMESPACE_EXIT;

#endif // IMAGECOMPONENTS_H
