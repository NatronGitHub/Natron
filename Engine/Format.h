//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_FORMAT_H_
#define NATRON_ENGINE_FORMAT_H_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <string>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
CLANG_DIAG_OFF(unused-local-typedef)
GCC_DIAG_OFF(unused-parameter)
#pragma message WARN("move serialization to a separate header")
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/version.hpp>
CLANG_DIAG_ON(unused-local-typedef)
GCC_DIAG_ON(unused-parameter)
#endif
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QMetaType>
CLANG_DIAG_ON(deprecated)

#include "Engine/RectD.h"
#include "Engine/RectI.h"

#define FORMAT_SERIALIZATION_CHANGES_TO_RECTD 2
#define FORMAT_SERIALIZATION_CHANGES_TO_RECTI 3
#define FORMAT_SERIALIZATION_VERSION FORMAT_SERIALIZATION_CHANGES_TO_RECTI

/*This class is used to hold the format of a frame (its resolution).
 * Some formats have a name , e.g : 1920*1080 is full HD, etc...
 * It also holds a pixel aspect ratio so the viewer can display the
 * frame accordingly*/
class Format
    : public RectI            //!< project format is in pixel coordinates

{
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int version)
    {
        if (version < FORMAT_SERIALIZATION_CHANGES_TO_RECTD) {
            RectI r;
            ar & boost::serialization::make_nvp("RectI",r);
            x1 = r.x1;
            x2 = r.x2;
            y1 = r.y1;
            y2 = r.y2;
        } else if (version < FORMAT_SERIALIZATION_CHANGES_TO_RECTI) {
            
            RectD r;
            ar & boost::serialization::make_nvp("RectD",r);
            x1 = r.x1;
            x2 = r.x2;
            y1 = r.y1;
            y2 = r.y2;

        } else {
            boost::serialization::void_cast_register<Format,RectI>( static_cast<Format *>(NULL),
                                                                    static_cast<RectI *>(NULL) );
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RectI);
        }
        ar & boost::serialization::make_nvp("Pixel_aspect_ratio",_par);
        ar & boost::serialization::make_nvp("Name",_name);
    }

public:
    Format(int l,
           int b,
           int r,
           int t,
           const std::string & name,
           double par)
        : RectI(l,b,r,t)
          , _par(par)
          , _name(name)
    {
    }

    Format(const RectI & rect)
        : RectI(rect)
          , _par(1.)
          , _name()
    {
    }
    
    Format(const RectI& rect, const double par)
    : RectI(rect)
    , _par(par)
    , _name()
    {
        
    }

    Format(const Format & other)
        : RectI(other.left(), other.bottom(), other.right(), other.top())
          , _par(other.getPixelAspectRatio())
          , _name(other.getName())
    {
    }

    Format()
        : RectI()
          , _par(1.0)
          , _name()
    {
    }

    virtual ~Format()
    {
    }

    const std::string & getName() const
    {
        return _name;
    }

    void setName(const std::string & n)
    {
        _name = n;
    }

    double getPixelAspectRatio() const
    {
        return _par;
    }

    void setPixelAspectRatio(double p)
    {
        _par = p;
    }
    
    RectD toCanonicalFormat() const
    {
        RectD ret;
        toCanonical_noClipping(0, _par, &ret);
        return ret;
    }

    Format & operator=(const Format & other)
    {
        set(other.left(), other.bottom(), other.right(), other.top());
        setName(other.getName());
        setPixelAspectRatio(other.getPixelAspectRatio());

        return *this;
    }

    bool operator==(const Format & other) const
    {
        return (_par == other.getPixelAspectRatio() &&
                left() == other.left() &&
                bottom() == other.bottom() &&
                right() == other.right() &&
                top() == other.top());
    }

private:
    double _par;
    std::string _name;
};

BOOST_CLASS_VERSION(Format, FORMAT_SERIALIZATION_VERSION)

Q_DECLARE_METATYPE(Format);


#endif // NATRON_ENGINE_FORMAT_H_
