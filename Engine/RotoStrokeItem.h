/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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
#include <boost/noncopyable.hpp>
#endif

CLANG_DIAG_OFF(deprecated-declarations)
#include <QtCore/QObject>
#include <QtCore/QMutex>
#include <QtCore/QMetaType>
CLANG_DIAG_ON(deprecated-declarations)

#include "Global/GlobalDefines.h"
#include "Engine/FitCurve.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/RotoPoint.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

// The number of pressure levels is 256 on an old Wacom Graphire 4, and 512 on an entry-level Wacom Bamboo
// 512 should be OK, see:
// http://www.davidrevoy.com/article182/calibrating-wacom-stylus-pressure-on-krita
#define ROTO_PRESSURE_LEVELS 512

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
                   const KnobItemsTablePtr& model);

private:


    RotoStrokeItem(const RotoStrokeItemPtr& other, const FrameViewRenderKey& key);

public:

    virtual ~RotoStrokeItem();

    /**
     * @brief Returns the brush type of the stroke
     **/
    virtual RotoStrokeType getBrushType() const OVERRIDE FINAL;

    /**
     * @brief If empty there's no stroke
     **/
    bool isEmpty() const;

    /**
     * @brief Set the stroke in drawing mode. Internally all nodes get a instance safe thread-safety.
     * Each call to appendPoint will trigger a render. For each render we mark the number of points we
     * rendered so that we don't re-render them again.
     * When finished drawing a stroke, call endSubStroke.
     **/
    void beginSubStroke();

    /**
     * @brief Add a point to the current stroke. This can only be called between a begin/endSubStroke bracket.
     **/
    void appendPoint(const RotoPoint& p);

    /**
     * @brief Notifies that the sub-stroke drawing is finished. This call must match another call to beginSubStroke.
     * After that, the stroke is no longer in drawing mode, meaning the subsequent render will always render the full stroke.
     **/
    void endSubStroke();

    /**
     * @brief Clears all strokes and set them to the given points
     **/
    void setStrokes(const std::list<std::list<RotoPoint> >& strokes);

    /**
     * @brief Add one sub-stroke at once by giving the 3 internal curves
     **/
    void addStroke(const CurvePtr& xCurve,
                   const CurvePtr& yCurve,
                   const CurvePtr& pCurve);

    /**
     * @brief Returns true if the stroke should be removed
     **/
    bool removeLastStroke(CurvePtr* xCurve,
                          CurvePtr* yCurve,
                          CurvePtr* pCurve);

    /**
     * @brief When drawing, each step need to store some data in order
     * to the next step to pick-up from the same state.
     **/
    void updateStrokeData(const Point& lastCenter, double distNextOut, int lastSubStrokePointIndex);

    /**
     * @brief Get the state of the stroke rendering algorithm at the previous draw step.
     **/
    void getStrokeState(Point* lastCenterIn, double* distNextIn) const;

    /**
     * @brief Should only be called on the render clone: this is used to determine
     * if we are rendering the first sub-stroke or not.
     **/
    int getRenderCloneCurrentStrokeIndex() const;

    /**
     * @brief Should only be called on the render clone: this is used to determine
     * if we are rendering the first drawing tick for a sub-stroke.
     **/
    int getRenderCloneCurrentStrokeStartPointIndex() const;
    int getRenderCloneCurrentStrokeEndPointIndex() const;


    /**
     * @brief Internal to the cairo implementation of the stroke rendering. Do not call.
     **/
    std::vector<cairo_pattern_t*> getPatternCache() const;
    void updatePatternCache(const std::vector<cairo_pattern_t*>& cache);

    /**
     * @brief When rendering a stroke that is being drawn, we re-use previous buffers and textures.
     * We must ensure that we keep the same OpenGL context!
     **/
    void setDrawingGLContext(const OSGLContextPtr& gpuContext, const OSGLContextPtr& cpuContext);
    void getDrawingGLContext(OSGLContextPtr* gpuContext, OSGLContextPtr* cpuContext) const;

    /**
     * @brief Can only be called on a render clone: returns the bounding box of the points that are to be rendered
     * and not the bounding box of the whole stroke.
     **/
    RectD getLastStrokeMovementBbox() const;

    /**
     * @brief Returns true if we currently are drawing. In this case the paint-buffers should be the same and only the portion
     * of the un-rendereed points should be rendered.
     **/
    bool isCurrentlyDrawing() const;


    virtual void copyItem(const KnobTableItem& other) OVERRIDE FINAL;

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
    
    virtual RectD getBoundingBox(TimeValue time, ViewIdx view) const OVERRIDE FINAL;


    ///bbox is in canonical coords
    void evaluateStroke(const RenderScale& scale,
                        TimeValue time,
                        ViewIdx view,
                        std::list<std::list<std::pair<Point, double> > >* strokes) const;

    std::list<CurvePtr> getXControlPoints() const;
    std::list<CurvePtr> getYControlPoints() const;

    int getNumControlPoints(int strokeIndex) const;


    KnobDoublePtr getBrushEffectKnob() const;
    KnobBoolPtr getPressureOpacityKnob() const;
    KnobBoolPtr getPressureSizeKnob() const;
    KnobBoolPtr getPressureHardnessKnob() const;
    KnobBoolPtr getBuildupKnob() const;

    KnobDoublePtr getBrushCloneTranslateKnob() const;
    KnobDoublePtr getBrushCloneRotateKnob() const;
    KnobDoublePtr getBrushCloneScaleKnob() const;
    KnobBoolPtr getBrushCloneScaleUniformKnob() const;
    KnobDoublePtr getBrushCloneSkewXKnob() const;
    KnobDoublePtr getBrushCloneSkewYKnob() const;
    KnobChoicePtr getBrushCloneSkewOrderKnob() const;
    KnobDoublePtr getBrushCloneCenterKnob() const;
    KnobChoicePtr getBrushCloneFilterKnob() const;
    KnobBoolPtr getBrushCloneBlackOutsideKnob() const;

    virtual void appendToHash(const ComputeHashArgs& args, Hash64* hash) OVERRIDE FINAL;

    virtual std::string getBaseItemName() const OVERRIDE FINAL;

    virtual std::string getSerializationClassName() const OVERRIDE FINAL;

    
private:

    virtual KnobHolderPtr createRenderCopy(const FrameViewRenderKey& render) const OVERRIDE FINAL;

    virtual bool isRenderCloneNeeded() const OVERRIDE FINAL
    {
        return true;
    }

    virtual void fetchRenderCloneKnobs() OVERRIDE;

    virtual void initializeKnobs() OVERRIDE;

    boost::scoped_ptr<RotoStrokeItemPrivate> _imp;
};


inline RotoStrokeItemPtr
toRotoStrokeItem(const KnobHolderPtr& item)
{
    return boost::dynamic_pointer_cast<RotoStrokeItem>(item);
}


NATRON_NAMESPACE_EXIT

#endif // Engine_RotoStrokeItem_h
