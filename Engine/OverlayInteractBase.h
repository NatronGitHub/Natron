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

#ifndef NATRON_ENGINE_OVERLAYINTERACTBASE_H
#define NATRON_ENGINE_OVERLAYINTERACTBASE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <stdexcept>
#include <map>


#include <QCoreApplication>
#include <QPointF>

#include "Global/GlobalDefines.h"
#include "Global/KeySymbols.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/EngineFwd.h"

#include "Engine/Color.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/TimeValue.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief Base class for any overlay interact. All functions can only be called on the UI main-thread.
 **/
struct OverlayInteractBasePrivate;
class OverlayInteractBase : public QObject
{
    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    struct KnobDesc
    {
        // The Knob::typeName()
        std::string type;

        // The number of dimensions of the knob
        int nDims;

        // Is this knob optional or does this overlay requires this knobs to work properly
        bool optional;

        KnobDesc()
        : type()
        , nDims(0)
        , optional(false)
        {

        }
    };

    typedef std::map<std::string, KnobDesc> KnobsDescMap;

    /**
     * @brief Make a new interact. To actually use it call EffectInstance::registerOverlay
     **/
    OverlayInteractBase();

    /**
     * @brief Make a new interact for a parameter that replaces the current interface to allow custom OpenGL widgets
     * For parametric parameters, this does not completely replace the interface but instead allows to draw as an overlay
     * on top of the drawing.
     **/
    OverlayInteractBase(const KnobIPtr& knob);

    virtual ~OverlayInteractBase();

    /**
     * @brief Returns described knobs that are needed by an overlay.
     * This enable an overlay to be used by different node classes that meet parameters requirements.
     * Internally calls describeKnobs()
     **/
    const KnobsDescMap& getDescription() const;

    void fetchKnobs_public(const std::map<std::string, std::string>& knobs);

protected:

    /**
     * @brief To be implemented to describe knobs that are needed by an overlay.
     * This enable an overlay to be used by different node classes that meet parameters requirements.
     **/
    virtual void describeKnobs(KnobsDescMap* knobs) const;

    /**
     * @brief To be implemented to fetch all knobs as given by the description returned by describeKnobs
     * on the effect returned by getEffect().
     * This function should throw a std::invalid_argument() if a knob cannot be found
     **/
    virtual void fetchKnobs(const std::map<std::string, std::string>& knobs);

public:

    template <typename T>
    boost::shared_ptr<T> getEffectKnobByRole(const std::map<std::string, std::string>& knobs,
                                             const std::string& roleName,
                                             int nDims,
                                             bool optional) const
    {
        std::string knobName = getKnobName(knobs, roleName, optional);
        if (knobName.empty()) {
            return boost::shared_ptr<T>();
        }
        return getEffectKnob<T>(knobName, nDims, optional);
    }


    std::string getKnobName(const std::map<std::string, std::string>& knobs,
                            const std::string& knobRole,
                            bool optional) const;


    template <typename T>
    boost::shared_ptr<T> getEffectKnob(const std::string& name, int nDims, bool optional) const
    {
        KnobIPtr knob = getEffectKnobByName(name, nDims, optional);
        if (!knob) {
            return boost::shared_ptr<T>();
        }

        boost::shared_ptr<T> ret = boost::dynamic_pointer_cast<T>(knob);
        if (!ret) {
            if (optional) {
                return ret;
            }
            std::string m = tr("Parameter %1 does not match required type (%2)").arg(QString::fromUtf8(name.c_str())).arg(QString::fromUtf8(T::typeNameStatic().c_str())).toStdString();
            throw std::invalid_argument(m);
        }
        return ret;
    }

    KnobIPtr getEffectKnobByName(const std::string& name, int nDims, bool optional) const;

private:
    
    /**
     * @brief Set the effect holding this interact. Called internally by EffectInstance::registerOverlay
     **/
    void setEffect(const EffectInstancePtr& effect);

public:

    /**
     * @brief Returns the effect holding this overlay
     **/
    EffectInstancePtr getEffect() const;

    /**
     * @brief Set the interact enabled or disabled. When disabled the user can no longer interact with it
     * and it is no longer displayed in the viewport.
     * By default an interact is always enabled.
     **/
    void setInteractEnabled(bool enabled);
    bool isInteractEnabled() const;

    /**
     * @brief Return true if the color picker is required for this interact
     * By default returns false.
     **/
    virtual bool isColorPickerRequired() const;

    /**
     * @brief Implement to respond to native viewport selection rectangle
     **/
    virtual void onViewportSelectionCleared();

    /**
     * @brief Implement to respond to native viewport selection rectangle
     **/
    virtual void onViewportSelectionUpdated(const RectD& rectangle, bool onRelease);

    /**
     * @brief Called by the host viewport to update the color picker if this interact requires color-picking
     **/
    void setInteractColourPicker(const ColorRgba<double>& color, bool setColor, bool hasColor);

    /**
     * @brief Return true if the picker has a valid color
     **/
    bool hasColorPicker() const;

    /**
     * @brief Return the current picker color
     **/
    const ColorRgba<double>& getLastColorPickerColor() const;

    /**
     * @brief Get a pointer to the last viewport set in setCallingViewport(). Note that the pointer is only guaranteed
     * to be valid throughout the lifetime of the current action.
     **/
    OverlaySupport* getLastCallingViewport() const;

    /**
     * @brief Swaps the buffer of the attached viewer, this shouldn't be needed if the host viewport is double buffered
     **/
    void swapOpenGLBuffers();

    /**
     * @brief Get properties on the OpenGL context used for drawing the interact
     **/
    void getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const;

    /**
     * @brief Return the size in pixels of the viewport widget
     **/
    void getViewportSize(double &width, double &height) const;

    /**
     * @brief Return the scale from the current OpenGL projection mapping to the viewport coordinates
     **/
    void getPixelScale(double & xScale, double & yScale) const;

#ifdef OFX_EXTENSIONS_NATRON
    double getScreenPixelRatio() const;
#endif

    /**
     * @brief Return the viewport background color
     **/
    void getBackgroundColour(double &r, double &g, double &b) const;

    /**
     * @brief Hints about a suggested drawing color.
     * @returns True if the color hint is valid, false otherwise.
     **/
    virtual bool getOverlayColor(double &r, double &g, double &b) const;

    /**
     * @brief Redraw the calling viewport
     **/
    void redraw();

    //// Interacts for parameters support

    /**
     * @brief Only valid for parameters interact,
     * Implement to return the minimum size in pixels of the widget
     **/
    virtual void getMinimumSize(double & minW, double & minH) const;

    /**
     * @brief Only valid for parameters interact,
     * Implement to return the preferred size in pixels of the widget
     **/
    virtual void getPreferredSize(int & pW, int & pH) const;

    /**
     * @brief Only valid for parameters interact,
     * Implement to return the actual size in pixels of the widget
     **/
    virtual void getSize(int &w, int &h) const;

    /**
     * @brief Only valid for parameters interact,
     * Implement to set the size in pixels of the widget
     **/
    virtual void setSize(int w, int h);

    /**
     * @brief Only valid for parameters interact,
     * Implement to return the pixel aspect ratio of the widget
     **/
    virtual void getPixelAspectRatio(double & par) const;

public:

    /// Overlay actions public functions

    void drawOverlay_public(OverlaySupport* viewport, TimeValue time, const RenderScale & renderScale, ViewIdx view);

    bool onOverlayPenDown_public(OverlaySupport* viewport, TimeValue time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp, PenType pen) WARN_UNUSED_RETURN;

    bool onOverlayPenMotion_public(OverlaySupport* viewport, TimeValue time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp) WARN_UNUSED_RETURN;

    bool onOverlayPenDoubleClicked_public(OverlaySupport* viewport, TimeValue time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos) WARN_UNUSED_RETURN;

    bool onOverlayPenUp_public(OverlaySupport* viewport, TimeValue time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp) WARN_UNUSED_RETURN;

    bool onOverlayKeyDown_public(OverlaySupport* viewport, TimeValue time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    bool onOverlayKeyUp_public(OverlaySupport* viewport, TimeValue time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    bool onOverlayKeyRepeat_public(OverlaySupport* viewport, TimeValue time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    bool onOverlayFocusGained_public(OverlaySupport* viewport, TimeValue time, const RenderScale & renderScale, ViewIdx view) WARN_UNUSED_RETURN;

    bool onOverlayFocusLost_public(OverlaySupport* viewport, TimeValue time, const RenderScale & renderScale, ViewIdx view) WARN_UNUSED_RETURN;

protected:

    //// Interact actions to implement

    virtual void drawOverlay(TimeValue time,
                             const RenderScale & renderScale,
                             ViewIdx view);

    virtual bool onOverlayPenDown(TimeValue time,
                                  const RenderScale & renderScale,
                                  ViewIdx view,
                                  const QPointF & viewportPos,
                                  const QPointF & pos,
                                  double pressure,
                                  TimeValue timestamp,
                                  PenType pen) WARN_UNUSED_RETURN;

    virtual bool onOverlayPenDoubleClicked(TimeValue time,
                                           const RenderScale & renderScale,
                                           ViewIdx view,
                                           const QPointF & viewportPos,
                                           const QPointF & pos) WARN_UNUSED_RETURN;

    virtual bool onOverlayPenMotion(TimeValue time,
                                    const RenderScale & renderScale,
                                    ViewIdx view,
                                    const QPointF & viewportPos,
                                    const QPointF & pos,
                                    double pressure,
                                    TimeValue timestamp) WARN_UNUSED_RETURN;

    virtual bool onOverlayPenUp(TimeValue time,
                                const RenderScale & renderScale,
                                ViewIdx view,
                                const QPointF & viewportPos,
                                const QPointF & pos,
                                double pressure,
                                TimeValue timestamp) WARN_UNUSED_RETURN;

    virtual bool onOverlayKeyDown(TimeValue time,
                                  const RenderScale & renderScale,
                                  ViewIdx view,
                                  Key key,
                                  KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    virtual bool onOverlayKeyUp(TimeValue time,
                                const RenderScale & renderScale,
                                ViewIdx view,
                                Key key,
                                KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    virtual bool onOverlayKeyRepeat(TimeValue time,
                                    const RenderScale & renderScale,
                                    ViewIdx view,
                                    Key key,
                                    KeyboardModifiers modifiers) WARN_UNUSED_RETURN;

    virtual bool onOverlayFocusGained(TimeValue time,
                                      const RenderScale & renderScale,
                                      ViewIdx view) WARN_UNUSED_RETURN;

    virtual bool onOverlayFocusLost(TimeValue time,
                                    const RenderScale & renderScale,
                                    ViewIdx view) WARN_UNUSED_RETURN;

    //// End actions

Q_SIGNALS:

    void mustRedrawOnMainThread();

private Q_SLOTS:

    void doRedrawOnMainThread();

private:

    friend class EffectInstance;
    boost::scoped_ptr<OverlayInteractBasePrivate> _imp;
};


NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_OVERLAYINTERACTBASE_H
