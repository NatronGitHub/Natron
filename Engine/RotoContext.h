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

#ifndef Engine_RotoContext_h
#define Engine_RotoContext_h

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

CLANG_DIAG_OFF(deprecated-declarations)
#include <QObject>
#include <QMutex>
#include <QMetaType>
CLANG_DIAG_ON(deprecated-declarations)


#include "Global/GlobalDefines.h"

#include "Engine/FitCurve.h"
#include "Engine/RotoItem.h"
#include "Engine/EngineFwd.h"


//#define NATRON_ROTO_ENABLE_MOTION_BLUR

NATRON_NAMESPACE_ENTER;

/**
 * @class A base class for all items made by the roto context
 **/



/**
 * @class This class is a member of all effects instantiated in the context "paint". It describes internally
 * all the splines data structures and their state.
 * This class is MT-safe.
 **/
class RotoContextSerialization;
struct RotoContextPrivate;
class RotoContext
: public QObject, public boost::enable_shared_from_this<RotoContext>
{
    Q_OBJECT

public:

    
    RotoContext(const boost::shared_ptr<Node>& node);

    virtual ~RotoContext();
    
    /**
     * @brief We have chosen to disable rotopainting and roto shapes from the same RotoContext because the rendering techniques are
     * very much differents. The rotopainting systems requires an entire compositing tree held inside whereas the rotoshapes
     * are rendered and optimized by Cairo internally.
     **/
    bool isRotoPaint() const;
    
    void createBaseLayer();
    
    boost::shared_ptr<RotoLayer> getOrCreateBaseLayer() ;

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
    boost::shared_ptr<RotoLayer> addLayer();
private:
    
    boost::shared_ptr<RotoLayer> addLayerInternal(bool declarePython);
public:
    
    
    /**
     * @brief Add an existing layer to the layers
     **/
    void addLayer(const boost::shared_ptr<RotoLayer> & layer);
    

    /**
     * @brief Make a new bezier curve and append it into the currently selected layer.
     * @param baseName A hint to name the item. It can be something like "Bezier", "Ellipse", "Rectangle" , etc...
     **/
    boost::shared_ptr<Bezier> makeBezier(double x,double y,const std::string & baseName,double time, bool isOpenBezier);
    boost::shared_ptr<Bezier> makeEllipse(double x,double y,double diameter,bool fromCenter,double time);
    boost::shared_ptr<Bezier> makeSquare(double x,double y,double initialSize,double time);
    
    
    boost::shared_ptr<RotoStrokeItem> makeStroke(Natron::RotoStrokeType type,
                                                 const std::string& baseName,
                                                 bool clearSel);
    
    std::string generateUniqueName(const std::string& baseName);
    
    /**
     * @brief Removes the given item from the context. This also removes the item from the selection
     * if it was selected. If the item has children, this will also remove all the children.
     **/
    void removeItem(const boost::shared_ptr<RotoItem>& item, RotoItem::SelectionReasonEnum reason = RotoItem::eSelectionReasonOther);

    ///This is here for undo/redo purpose. Do not call this
    void addItem(const boost::shared_ptr<RotoLayer>& layer, int indexInLayer, const boost::shared_ptr<RotoItem> & item, RotoItem::SelectionReasonEnum reason);
    /**
     * @brief Returns a const ref to the layers list. This can only be called from
     * the main thread.
     **/
    const std::list< boost::shared_ptr<RotoLayer> > & getLayers() const;

    /**
     * @brief Returns a bezier curves nearby the point (x,y) and the parametric value
     * which would be used to find the exact bezier point lying on the curve.
     **/
    boost::shared_ptr<Bezier> isNearbyBezier(double x,double y,double acceptance,int* index,double* t,bool *feather) const;


    /**
     * @brief Returns the region of definition of the shape unioned to the region of definition of the node
     * or the project format.
     **/
    void getMaskRegionOfDefinition(double time,
                                   int view,
                                   RectD* rod) const; //!< rod in canonical coordinates
    
    /**
     * @brief Returns true if  all items have the same compositing operator and there are only strokes or bezier (because they
     * are not masked)
     **/
    bool isRotoPaintTreeConcatenatable() const;
    
    static bool isRotoPaintTreeConcatenatableInternal(const std::list<boost::shared_ptr<RotoDrawableItem> >& items);
    
    void getGlobalMotionBlurSettings(const double time,
                               double* startTime,
                               double* endTime,
                               double* timeStep) const;


private:
    
    
    void getItemsRegionOfDefinition(const std::list<boost::shared_ptr<RotoItem> >& items, double time, int view, RectD* rod) const;
    
public:
    
    
   
    boost::shared_ptr<Image> renderMaskFromStroke(const boost::shared_ptr<RotoDrawableItem>& stroke,
                                                          const RectI& roi,
                                                          const ImageComponents& components,
                                                          const double time,
                                                          const int view,
                                                          const ImageBitDepthEnum depth,
                                                          const unsigned int mipmapLevel);
    
    double renderSingleStroke(const boost::shared_ptr<RotoStrokeItem>& stroke,
                            const RectD& rod,
                            const std::list<std::pair<Point,double> >& points,
                            unsigned int mipmapLevel,
                            double par,
                            const ImageComponents& components,
                            ImageBitDepthEnum depth,
                            double distToNext,
                            boost::shared_ptr<Image> *wholeStrokeImage);
    
private:
    
    boost::shared_ptr<Image> renderMaskInternal(const boost::shared_ptr<RotoDrawableItem>& stroke,
                                                        const RectI & roi,
                                                        const ImageComponents& components,
                                                        const double startTime,
                                                        const double endTime,
                                                        const double timeStep,
                                                        const double time,
                                                        const ImageBitDepthEnum depth,
                                                        const unsigned int mipmapLevel,
                                                        const std::list<std::list<std::pair<Point,double> > >& strokes,
                                                        const boost::shared_ptr<Image> &image);
    
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
    void select(const boost::shared_ptr<RotoItem> & b, RotoItem::SelectionReasonEnum reason);

    ///for convenience
    void select(const std::list<boost::shared_ptr<RotoDrawableItem> > & beziers, RotoItem::SelectionReasonEnum reason);
    void select(const std::list<boost::shared_ptr<RotoItem> > & items, RotoItem::SelectionReasonEnum reason);

    /**
     * @brief This must be called by the GUI whenever an item is deselected. This is recursive for layers.
     **/
    void deselect(const boost::shared_ptr<RotoItem> & b, RotoItem::SelectionReasonEnum reason);

    ///for convenience
    void deselect(const std::list<boost::shared_ptr<Bezier> > & beziers, RotoItem::SelectionReasonEnum reason);
    void deselect(const std::list<boost::shared_ptr<RotoItem> > & items, RotoItem::SelectionReasonEnum reason);

    void clearAndSelectPreviousItem(const boost::shared_ptr<RotoItem>& item,RotoItem::SelectionReasonEnum reason);
    
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
    std::list< boost::shared_ptr<RotoDrawableItem> > getSelectedCurves() const;


    /**
     * @brief Returns a const ref to the selected items. This can only be called on the main thread.
     **/
    const std::list< boost::shared_ptr<RotoItem> > & getSelectedItems() const;

    /**
     * @brief Returns a list of all the curves in the order in which they should be rendered.
     * Non-active curves will not be inserted into the list.
     * MT-safe
     **/
    std::list< boost::shared_ptr<RotoDrawableItem> > getCurvesByRenderOrder(bool onlyActivated = true) const;
    
    int getNCurves() const;
    
    boost::shared_ptr<Node> getNode() const;
    
    boost::shared_ptr<RotoLayer> getLayerByName(const std::string & n) const;
    boost::shared_ptr<RotoItem> getItemByName(const std::string & n) const;
    boost::shared_ptr<RotoItem> getLastInsertedItem() const;

#ifdef NATRON_ROTO_INVERTIBLE
    boost::shared_ptr<KnobBool> getInvertedKnob() const;
#endif
    boost::shared_ptr<KnobColor> getColorKnob() const;

    void resetTransformCenter();
    
    void resetCloneTransformCenter();
    
    void resetTransformsCenter(bool doClone, bool doTransform);
    
    void resetTransform();
    
    void resetCloneTransform();
    
private:
    
    void resetTransformInternal(const boost::shared_ptr<KnobDouble>& translate,
                                const boost::shared_ptr<KnobDouble>& scale,
                                const boost::shared_ptr<KnobDouble>& center,
                                const boost::shared_ptr<KnobDouble>& rotate,
                                const boost::shared_ptr<KnobDouble>& skewX,
                                const boost::shared_ptr<KnobDouble>& skewY,
                                const boost::shared_ptr<KnobBool>& scaleUniform,
                                const boost::shared_ptr<KnobChoice>& skewOrder);
    
public:
    
    boost::shared_ptr<KnobChoice> getMotionBlurTypeKnob() const;
    

    boost::shared_ptr<RotoItem> getLastItemLocked() const;
    boost::shared_ptr<RotoLayer> getDeepestSelectedLayer() const;

    void onItemLockedChanged(const boost::shared_ptr<RotoItem>& item,RotoItem::SelectionReasonEnum reason);

    void emitRefreshViewerOverlays();

    void getBeziersKeyframeTimes(std::list<double> *times) const;

    /**
     * @brief Returns the name of the node holding this item
     **/
    std::string getRotoNodeName() const;
    
    void onItemScriptNameChanged(const boost::shared_ptr<RotoItem>& item);
    void onItemLabelChanged(const boost::shared_ptr<RotoItem>& item);
    
    void onItemKnobChanged();

    void declarePythonFields();
    
    void changeItemScriptName(const std::string& oldFullyQualifiedName,const std::string& newFullyQUalifiedName);
    
    void declareItemAsPythonField(const boost::shared_ptr<RotoItem>& item);
    void removeItemAsPythonField(const boost::shared_ptr<RotoItem>& item);
    
    /**
     * @brief Rebuilds the connection between nodes used internally by the rotopaint tree
     * To be called whenever changes position in the hierarchy or when one gets removed/inserted
     **/
    void refreshRotoPaintTree();
    
    void onRotoPaintInputChanged(const boost::shared_ptr<Node>& node);
        
    void getRotoPaintTreeNodes(std::list<boost::shared_ptr<Node> >* nodes) const;
    
    boost::shared_ptr<Node> getRotoPaintBottomMergeNode() const;
        
    void setWhileCreatingPaintStrokeOnMergeNodes(bool b);
    
    /**
     * @brief First searches through the selected layer which one is the deepest in the hierarchy.
     * If nothing is found, it searches through the selected items and find the deepest selected item's layer
     **/
    boost::shared_ptr<RotoLayer> findDeepestSelectedLayer() const;
    
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
    
    void knobChanged(KnobI* k,
                     ValueChangedReasonEnum reason,
                     int view,
                     double time,
                     bool originatedFromMainThread);
    
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

    void itemInserted(int,int);

    void itemRemoved(const boost::shared_ptr<RotoItem>&,int);

    void refreshViewerOverlays();

    void itemLockedChanged(int reason);
    
    void itemScriptNameChanged(const boost::shared_ptr<RotoItem>&);
    void itemLabelChanged(const boost::shared_ptr<RotoItem>&);

public Q_SLOTS:

    void onAutoKeyingChanged(bool enabled);

    void onFeatherLinkChanged(bool enabled);

    void onRippleEditChanged(bool enabled);
    
    void onSelectedKnobCurveChanged();
    
    void onLifeTimeKnobValueChanged(int, int);

private:
    
    
    boost::shared_ptr<Node> getOrCreateGlobalMergeNode(int *availableInputIndex);
    
    void selectInternal(const boost::shared_ptr<RotoItem>& b);
    void deselectInternal(boost::shared_ptr<RotoItem> b);

    void removeItemRecursively(const boost::shared_ptr<RotoItem>& item,RotoItem::SelectionReasonEnum reason);

   
    boost::scoped_ptr<RotoContextPrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // Engine_RotoContext_h
