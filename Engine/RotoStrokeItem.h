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

#ifndef Engine_RotoStrokeItem_h
#define Engine_RotoStrokeItem_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <set>
#include <string>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated-declarations)
#include <QtCore/QObject>
#include <QtCore/QMutex>
#include <QtCore/QMetaType>
CLANG_DIAG_ON(deprecated-declarations)

#include "Global/GlobalDefines.h"
#include "Engine/FitCurve.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/EngineFwd.h"


// The number of pressure levels is 256 on an old Wacom Graphire 4, and 512 on an entry-level Wacom Bamboo
// 512 should be OK, see:
// http://www.davidrevoy.com/article182/calibrating-wacom-stylus-pressure-on-krita
#define ROTO_PRESSURE_LEVELS 512


NATRON_NAMESPACE_ENTER;

/**
 * @class A base class for all items made by the roto context
 **/


/**
 * @class Base class for all strokes
 **/
struct RotoStrokeItemPrivate;
class RotoStrokeItem
    : public RotoDrawableItem
{
public:

    RotoStrokeItem(RotoStrokeType type,
                   const RotoContextPtr& context,
                   const std::string & name,
                   const RotoLayerPtr& parent);

    virtual ~RotoStrokeItem();


    RotoStrokeType getBrushType() const;

    bool isEmpty() const;

    /**
     * @brief Appends to the paint stroke the raw points list.
     * @returns True if the number of points is > 1
     **/
    bool appendPoint(bool newStroke, const RotoPoint& p);

    void addStroke(const CurvePtr& xCurve,
                   const CurvePtr& yCurve,
                   const CurvePtr& pCurve);

    /**
     * @brief Returns true if the stroke should be removed
     **/
    bool removeLastStroke(CurvePtr* xCurve,
                          CurvePtr* yCurve,
                          CurvePtr* pCurve);

    std::vector<cairo_pattern_t*> getPatternCache() const;
    void updatePatternCache(const std::vector<cairo_pattern_t*>& cache);

    void setDrawingGLContext(const OSGLContextPtr& gpuContext, const OSGLContextPtr& cpuContext);
    void getDrawingGLContext(OSGLContextPtr* gpuContext, OSGLContextPtr* cpuContext) const;

    bool getMostRecentStrokeChangesSinceAge(double time,
                                            int lastAge,
                                            int lastMultiStrokeIndex,
                                            std::list<std::pair<Point, double> >* points,
                                            RectD* pointsBbox,
                                            RectD* wholeStrokeBbox,
                                            bool *isStrokeFirstTick,
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
    virtual void toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj) OVERRIDE FINAL;

    /**
     * @brief Must be implemented by the derived class to load the state from
     * the serialization object.
     * Derived implementations must call the parent class implementation.
     **/
    virtual void fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase & obj) OVERRIDE FINAL;

    static RotoStrokeType strokeTypeFromSerializationString(const std::string& s);
    
    virtual RectD getBoundingBox(double time) const OVERRIDE FINAL;


    ///bbox is in canonical coords
    void evaluateStroke(unsigned int mipMapLevel, double time,
                        std::list<std::list<std::pair<Point, double> > >* strokes,
                        RectD* bbox = 0) const;

    std::list<CurvePtr > getXControlPoints() const;
    std::list<CurvePtr > getYControlPoints() const;

    virtual void appendToHash(double time, ViewIdx view, Hash64* hash) OVERRIDE FINAL;

private:

    RectD computeBoundingBox(double time) const;

    RectD computeBoundingBoxInternal(double time) const;

    boost::scoped_ptr<RotoStrokeItemPrivate> _imp;
};

inline RotoStrokeItemPtr
toRotoStrokeItem(const KnobHolderPtr& item)
{
    return boost::dynamic_pointer_cast<RotoStrokeItem>(item);
}

NATRON_NAMESPACE_EXIT;

#endif // Engine_RotoStrokeItem_h
