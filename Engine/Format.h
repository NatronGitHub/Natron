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

#ifndef Engine_Format_h
#define Engine_Format_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <string>

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QMetaType>
CLANG_DIAG_ON(deprecated)

#include "Engine/RectD.h"
#include "Engine/RectI.h"

/*This class is used to hold the format of a frame (its resolution).
 * Some formats have a name , e.g : 1920*1080 is full HD, etc...
 * It also holds a pixel aspect ratio so the viewer can display the
 * frame accordingly*/
class Format
    : public RectI            //!< project format is in pixel coordinates

{
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

    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int version);

private:
    double _par;
    std::string _name;
};

Q_DECLARE_METATYPE(Format);


#endif // Engine_Format_h
