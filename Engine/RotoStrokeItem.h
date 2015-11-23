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

#ifndef Engine_RotoStrokeItem_h
#define Engine_RotoStrokeItem_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>
#include <set>
#include <string>

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include "Global/GlobalDefines.h"
#include "Engine/FitCurve.h"
#include "Engine/RotoDrawableItem.h"

CLANG_DIAG_OFF(deprecated-declarations)
#include <QObject>
#include <QMutex>
#include <QMetaType>
CLANG_DIAG_ON(deprecated-declarations)

#define kRotoLayerBaseName "Layer"
#define kRotoBezierBaseName "Bezier"
#define kRotoOpenBezierBaseName "Pencil"
#define kRotoEllipseBaseName "Ellipse"
#define kRotoRectangleBaseName "Rectangle"
#define kRotoPaintBrushBaseName "Brush"
#define kRotoPaintEraserBaseName "Eraser"
#define kRotoPaintBlurBaseName "Blur"
#define kRotoPaintSmearBaseName "Smear"
#define kRotoPaintSharpenBaseName "Sharpen"
#define kRotoPaintCloneBaseName "Clone"
#define kRotoPaintRevealBaseName "Reveal"
#define kRotoPaintDodgeBaseName "Dodge"
#define kRotoPaintBurnBaseName "Burn"

namespace Natron {
class Image;
class ImageComponents;
class Node;
}
namespace boost { namespace serialization { class access; } }

class RectI;
class RectD;
class KnobI;
class KnobBool;
class KnobDouble;
class KnobInt;
class KnobChoice;
class KnobColor;
typedef struct _cairo_pattern cairo_pattern_t;

class Curve;
class Bezier;
struct RotoPoint;
class RotoItemSerialization;
class BezierCP;

/**
 * @class A base class for all items made by the roto context
 **/
class RotoContext;
class RotoLayer;

namespace Transform {
struct Matrix3x3;
}



/**
 * @class Base class for all strokes
 **/
struct RotoStrokeItemPrivate;
class RotoStrokeItem : public RotoDrawableItem
{
public:
    
    RotoStrokeItem(Natron::RotoStrokeType type,
                   const boost::shared_ptr<RotoContext>& context,
                   const std::string & name,
                   const boost::shared_ptr<RotoLayer>& parent);
    
    virtual ~RotoStrokeItem();
    
    
    Natron::RotoStrokeType getBrushType() const;
    
    bool isEmpty() const;
    
    /**
     * @brief Appends to the paint stroke the raw points list.
     * @returns True if the number of points is > 1
     **/
    bool appendPoint(bool newStroke,const RotoPoint& p);
    
    void addStroke(const boost::shared_ptr<Curve>& xCurve,
                  const boost::shared_ptr<Curve>& yCurve,
                  const boost::shared_ptr<Curve>& pCurve);
    
    /**
     * @brief Returns true if the stroke should be removed
     **/
    bool removeLastStroke(boost::shared_ptr<Curve>* xCurve,
                          boost::shared_ptr<Curve>* yCurve,
                          boost::shared_ptr<Curve>* pCurve);
    
    std::vector<cairo_pattern_t*> getPatternCache() const;
    void updatePatternCache(const std::vector<cairo_pattern_t*>& cache);
    
    double renderSingleStroke(const boost::shared_ptr<RotoStrokeItem>& stroke,
                              const RectD& rod,
                              const std::list<std::pair<Natron::Point,double> >& points,
                              unsigned int mipmapLevel,
                              double par,
                              const Natron::ImageComponents& components,
                              Natron::ImageBitDepthEnum depth,
                              double distToNext,
                              boost::shared_ptr<Natron::Image> *wholeStrokeImage);

    
    
    bool getMostRecentStrokeChangesSinceAge(double time,int lastAge, std::list<std::pair<Natron::Point,double> >* points,
                                            RectD* pointsBbox,
                                            RectD* wholeStrokeBbox,
                                            int* newAge,
                                            int* strokeIndex);
    
    RectD getWholeStrokeRoDWhilePainting() const;
    
    void setStrokeFinished();
    
    
    virtual void clone(const RotoItem* other) OVERRIDE FINAL;
    
    /**
     * @brief Must be implemented by the derived class to save the state into
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void save(RotoItemSerialization* obj) const OVERRIDE FINAL;
    
    /**
     * @brief Must be implemented by the derived class to load the state from
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void load(const RotoItemSerialization & obj) OVERRIDE FINAL;
    
    virtual RectD getBoundingBox(double time) const OVERRIDE FINAL;
    
    
    ///bbox is in canonical coords
    void evaluateStroke(unsigned int mipMapLevel, double time,
                        std::list<std::list<std::pair<Natron::Point,double> > >* strokes,
                        RectD* bbox = 0) const;
    
    std::list<boost::shared_ptr<Curve> > getXControlPoints() const;
    std::list<boost::shared_ptr<Curve> > getYControlPoints() const;    
private:
    
    RectD computeBoundingBox(double time) const;
    
    
    boost::scoped_ptr<RotoStrokeItemPrivate> _imp;
};



#endif // Engine_RotoStrokeItem_h
