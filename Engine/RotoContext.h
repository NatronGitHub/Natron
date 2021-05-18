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

#ifndef Engine_RotoContext_h
#define Engine_RotoContext_h

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
#include <boost/enable_shared_from_this.hpp>
#endif

CLANG_DIAG_OFF(deprecated-declarations)
#include <QtCore/QObject>
#include <QtCore/QMutex>
#include <QtCore/QMetaType>
CLANG_DIAG_ON(deprecated-declarations)


#include "Global/GlobalDefines.h"

#include "Engine/FitCurve.h"
#include "Engine/RotoItem.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"


//#define NATRON_ROTO_INVERTIBLE
//#define NATRON_ROTO_ENABLE_MOTION_BLUR

NATRON_NAMESPACE_ENTER

//Small RAII class that properly destroys the cairo image upon destruction
class CairoImageWrapper
{
public:

    CairoImageWrapper()
        : cairoImg(0)
        , ctx(0)
    {
    }

    CairoImageWrapper(cairo_surface_t* img,
                      cairo_t* context)
        : cairoImg(img)
        , ctx(context)
    {
    }

    ~CairoImageWrapper();

    cairo_surface_t* cairoImg;
    cairo_t* ctx;
};

/**
 * @class This class is a member of all effects instantiated in the context "paint". It describes internally
 * all the splines data structures and their state.
 * This class is MT-safe.
 **/
class RotoContextSerialization;
struct RotoContextPrivate;

class RotoContext
    : public QObject
    , public boost::enable_shared_from_this<RotoContext>
{
    Q_OBJECT

    struct MakeSharedEnabler;

    // constructors should be privatized in any class that derives from boost::enable_shared_from_this<>

    RotoContext(const NodePtr& node);

public:
    static RotoContextPtr create(const NodePtr& node);

    virtual ~RotoContext();

    /**
     * @brief We have chosen to disable rotopainting and roto shapes from the same RotoContext because the rendering techniques are
     * very much different. The rotopainting systems requires an entire compositing tree held inside whereas the rotoshapes
     * are rendered and optimized by Cairo internally.
     **/
    bool isRotoPaint() const;

    void createBaseLayer();

    RotoLayerPtr getOrCreateBaseLayer();

    /**
     * @brief Returns true when the context is empty (it has no shapes)
     **/
    bool isEmpty() const;

    /**
     * @brief Turn on/off auto-keying
     **/
    void setAutoKeyingEnabled(bool enabled);
    bool isAutoKeyingEnabled() const;

    /**
     * @brief Turn on/off feather link. When enabled the feather point will move by the same delta as the corresponding spline point
     * is moved by.
     **/
    void setFeatherLinkEnabled(bool enabled);
    bool isFeatherLinkEnabled() const;

    /**
     * @brief Turn on/off ripple edit. When enabled moving a point at a given keyframe will move it to the same position
     * at all keyframes.
     **/
    void setRippleEditEnabled(bool enabled);
    bool isRippleEditEnabled() const;

    int getTimelineCurrentTime() const;

    /**
     * @brief Create a new layer to the currently selected layer.
     **/
    RotoLayerPtr addLayer();

private:

    RotoLayerPtr addLayerInternal(bool declarePython);

public:


    /**
     * @brief Add an existing layer to the layers
     **/
    void addLayer(const RotoLayerPtr & layer);


    /**
     * @brief Make a new bezier curve and append it into the currently selected layer.
     * @param baseName A hint to name the item. It can be something like "Bezier", "Ellipse", "Rectangle" , etc...
     **/
    BezierPtr makeBezier(double x, double y, const std::string & baseName, double time, bool isOpenBezier);
    BezierPtr makeEllipse(double x, double y, double diameter, bool fromCenter, double time);
    BezierPtr makeSquare(double x, double y, double initialSize, double time);
    RotoStrokeItemPtr makeStroke(RotoStrokeType type,
                                                 const std::string& baseName,
                                                 bool clearSel);
    std::string generateUniqueName(const std::string& baseName);

    /**
     * @brief Removes the given item from the context. This also removes the item from the selection
     * if it was selected. If the item has children, this will also remove all the children.
     **/
    void removeItem(const RotoItemPtr& item, RotoItem::SelectionReasonEnum reason = RotoItem::eSelectionReasonOther);

    ///This is here for undo/redo purpose. Do not call this
    void addItem(const RotoLayerPtr& layer, int indexInLayer, const RotoItemPtr & item, RotoItem::SelectionReasonEnum reason);
    /**
     * @brief Returns a const ref to the layers list. This can only be called from
     * the main thread.
     **/
    const std::list<RotoLayerPtr> & getLayers() const;

    /**
     * @brief Returns a bezier curves nearby the point (x,y) and the parametric value
     * which would be used to find the exact Bezier point lying on the curve.
     **/
    BezierPtr isNearbyBezier(double x, double y, double acceptance, int* index, double* t, bool *feather) const;


    /**
     * @brief Returns the region of definition of the shape unioned to the region of definition of the node
     * or the project format.
     **/
    void getMaskRegionOfDefinition(double time,
                                   ViewIdx view,
                                   RectD* rod) const; //!< rod in canonical coordinates

    /**
     * @brief Returns true if  all items have the same compositing operator and there are only strokes or bezier (because they
     * are not masked)
     **/
    bool isRotoPaintTreeConcatenatable() const;

    static bool isRotoPaintTreeConcatenatableInternal(const std::list<RotoDrawableItemPtr>& items, int* blendingMode);

    void getGlobalMotionBlurSettings(const double time,
                                     double* startTime,
                                     double* endTime,
                                     double* timeStep) const;

private:


    void getItemsRegionOfDefinition(const std::list<RotoItemPtr>& items, double time, ViewIdx view, RectD* rod) const;

public:


    /**
     * @brief To be called when a change was made to trigger a new render.
     **/
    void evaluateChange();
    void evaluateChange_noIncrement();

    void incrementAge();

    void clearViewersLastRenderedStrokes();

    /**
     *@brief Returns the age of the roto context
     **/
    U64 getAge();

    ///Serialization
    void save(RotoContextSerialization* obj) const;

    ///Deserialization
    void load(const RotoContextSerialization & obj);


    /**
     * @brief This must be called by the GUI whenever an item is selected. This is recursive for layers.
     **/
    void select(const RotoItemPtr & b, RotoItem::SelectionReasonEnum reason);
    void selectFromLayer(const RotoItemPtr & b, RotoItem::SelectionReasonEnum reason);

    ///for convenience
    void select(const std::list<RotoDrawableItemPtr> & beziers, RotoItem::SelectionReasonEnum reason);
    void select(const std::list<RotoItemPtr> & items, RotoItem::SelectionReasonEnum reason);

    /**
     * @brief This must be called by the GUI whenever an item is deselected. This is recursive for layers.
     **/
    void deselect(const RotoItemPtr & b, RotoItem::SelectionReasonEnum reason);

    ///for convenience
    void deselect(const std::list<BezierPtr> & beziers, RotoItem::SelectionReasonEnum reason);
    void deselect(const std::list<RotoItemPtr> & items, RotoItem::SelectionReasonEnum reason);

    void clearAndSelectPreviousItem(const RotoItemPtr& item, RotoItem::SelectionReasonEnum reason);

    void clearSelection(RotoItem::SelectionReasonEnum reason);

    ///only callable on main-thread
    void setKeyframeOnSelectedCurves();

    ///only callable on main-thread
    void removeKeyframeOnSelectedCurves();

    void removeAnimationOnSelectedCurves();

    ///only callable on main-thread
    void goToPreviousKeyframe();

    ///only callable on main-thread
    void goToNextKeyframe();

    /**
     * @brief Returns a list of the currently selected curves. Can only be called on the main-thread.
     **/
    std::list<RotoDrawableItemPtr> getSelectedCurves() const;


    /**
     * @brief Returns a const ref to the selected items. This can only be called on the main thread.
     **/
    const std::list<RotoItemPtr> & getSelectedItems() const;

    /**
     * @brief Returns a list of all the curves in the order in which they should be rendered.
     * Non-active curves will not be inserted into the list.
     * MT-safe
     **/
    std::list<RotoDrawableItemPtr> getCurvesByRenderOrder(bool onlyActivated = true) const;

    int getNCurves() const;

    NodePtr getNode() const;

    RotoLayerPtr getLayerByName(const std::string & n) const;
    RotoItemPtr getItemByName(const std::string & n) const;
    RotoItemPtr getLastInsertedItem() const;

#ifdef NATRON_ROTO_INVERTIBLE
    KnobBoolPtr getInvertedKnob() const;
#endif

    KnobColorPtr getColorKnob() const;

    void resetTransformCenter();

    void resetCloneTransformCenter();

    void resetTransformsCenter(bool doClone, bool doTransform);

    void resetTransform();

    void resetCloneTransform();

private:

    void resetTransformInternal(const KnobDoublePtr& translate,
                                const KnobDoublePtr& scale,
                                const KnobDoublePtr& center,
                                const KnobDoublePtr& rotate,
                                const KnobDoublePtr& skewX,
                                const KnobDoublePtr& skewY,
                                const KnobBoolPtr& scaleUniform,
                                const KnobChoicePtr& skewOrder,
                                const KnobDoublePtr& extraMatrix);

public:

    KnobChoicePtr getMotionBlurTypeKnob() const;
    RotoItemPtr getLastItemLocked() const;
    RotoLayerPtr getDeepestSelectedLayer() const;

    void onItemLockedChanged(const RotoItemPtr& item, RotoItem::SelectionReasonEnum reason);

    void emitRefreshViewerOverlays();

    void getBeziersKeyframeTimes(std::list<double> *times) const;

    /**
     * @brief Returns the name of the node holding this item
     **/
    std::string getRotoNodeName() const;

    void onItemGloballyActivatedChanged(const RotoItemPtr& item);
    void onItemScriptNameChanged(const RotoItemPtr& item);
    void onItemLabelChanged(const RotoItemPtr& item);

    void onItemKnobChanged();

    void declarePythonFields();

    void changeItemScriptName(const std::string& oldFullyQualifiedName, const std::string& newFullyQUalifiedName);

    void declareItemAsPythonField(const RotoItemPtr& item);
    void removeItemAsPythonField(const RotoItemPtr& item);

    /**
     * @brief Rebuilds the connection between nodes used internally by the rotopaint tree
     * To be called whenever changes position in the hierarchy or when one gets removed/inserted
     **/
    void refreshRotoPaintTree();

    void onRotoPaintInputChanged(const NodePtr& node);

    void getRotoPaintTreeNodes(NodesList* nodes) const;

    NodePtr getRotoPaintBottomMergeNode() const;

    void setWhileCreatingPaintStrokeOnMergeNodes(bool b);

    /**
     * @brief First searches through the selected layer which one is the deepest in the hierarchy.
     * If nothing is found, it searches through the selected items and find the deepest selected item's layer
     **/
    RotoLayerPtr findDeepestSelectedLayer() const;

    void dequeueGuiActions();

    /**
     * @brief When finishing a stroke with the paint brush, we need to re-render it because the interpolation of the curve
     * will be much smoother with more points than what it was during painting.
     * We explicitly freeze the UI while waiting for the image to be drawn, otherwise the user might attempt to do a
     * multiple stroke on top of it which could make some artifacts.
     **/
    void evaluateNeatStrokeRender();


    bool mustDoNeatRender() const;

    void setIsDoingNeatRender(bool doing);

    bool isDoingNeatRender() const;

    void s_breakMultiStroke() { Q_EMIT breakMultiStroke(); }

    bool knobChanged(KnobI* k,
                     ValueChangedReasonEnum reason,
                     ViewSpec view,
                     double time,
                     bool originatedFromMainThread);

    static bool allocateAndRenderSingleDotStroke(int brushSizePixel, double brushHardness, double alpha, CairoImageWrapper& wrapper);

Q_SIGNALS:

    /**
     * @brief Signalled when the gui should force a new stroke to be added
     **/
    void breakMultiStroke();

    /**
     * Emitted when the selection is changed. The integer corresponds to the
     * RotoItem::SelectionReasonEnum enum.
     **/
    void selectionChanged(int);

    void restorationComplete();

    void itemInserted(int, int);

    void itemRemoved(const RotoItemPtr&, int);

    void refreshViewerOverlays();

    void itemLockedChanged(int reason);

    void itemGloballyActivatedChanged(const RotoItemPtr&);
    void itemScriptNameChanged(const RotoItemPtr&);
    void itemLabelChanged(const RotoItemPtr&);

public Q_SLOTS:

    void onAutoKeyingChanged(bool enabled);

    void onFeatherLinkChanged(bool enabled);

    void onRippleEditChanged(bool enabled);

    void onSelectedKnobCurveChanged();

    void onLifeTimeKnobValueChanged(ViewSpec, int, int);

private:


    NodePtr getOrCreateGlobalMergeNode(int *availableInputIndex);

    void selectInternal(const RotoItemPtr& b, bool slaveKnobs = true);
    void deselectInternal(RotoItemPtr b);

    void removeItemRecursively(const RotoItemPtr& item, RotoItem::SelectionReasonEnum reason);


    boost::scoped_ptr<RotoContextPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // Engine_RotoContext_h
