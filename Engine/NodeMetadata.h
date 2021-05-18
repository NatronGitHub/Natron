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

#ifndef Engine_NodeMetadata_h
#define Engine_NodeMetadata_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif


#include "Global/GlobalDefines.h"
#include "Engine/ImagePlaneDesc.h"


NATRON_NAMESPACE_ENTER

/**
 * @brief These meta-data are represents what's flowing through a node. They change
 * everytimes refreshMetadata() is called.
 * The bitdepth and inputs components must have been validated against the plug-ins supported components/bitdepth
 *
 **/
struct NodeMetadataPrivate;
class NodeMetadata
{
public:
    NodeMetadata();

    NodeMetadata(const NodeMetadata& other);

    ~NodeMetadata();

    void clearAndResize(int inputCount);

    void operator=(const NodeMetadata& other);
    bool operator==(const NodeMetadata& other) const;
    bool operator!=(const NodeMetadata& other) const
    {
        return !(*this == other);
    }

    void setOutputPremult(ImagePremultiplicationEnum premult);

    ImagePremultiplicationEnum getOutputPremult() const;

    void setOutputFrameRate(double fps);

    double getOutputFrameRate() const;

    void setOutputFielding(ImageFieldingOrderEnum fielding);

    ImageFieldingOrderEnum getOutputFielding() const;

    void setIsContinuous(bool continuous);

    bool getIsContinuous() const;

    void setIsFrameVarying(bool varying);

    bool getIsFrameVarying() const;

    void setPixelAspectRatio(int inputNb, double par);

    double getPixelAspectRatio(int inputNb) const;

    void setBitDepth(int inputNb, ImageBitDepthEnum depth);

    ImageBitDepthEnum getBitDepth(int inputNb) const;

    void setNComps(int inputNb, int nComps);

    int getNComps(int inputNb) const;

    void setComponentsType(int inputNb,const std::string& componentsType);

    std::string getComponentsType(int inputNb) const;

    void setOutputFormat(const RectI& format);

    const RectI& getOutputFormat() const;

private:

    boost::scoped_ptr<NodeMetadataPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // Engine_NodeMetadata_h
