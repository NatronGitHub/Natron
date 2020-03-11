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

#include "OverlayInteractBase.h"

#include <QThread>
#include <QCoreApplication>

#include <sstream>

#include "Engine/AppInstance.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/OverlaySupport.h"

NATRON_NAMESPACE_ENTER

// an RAII class to save OpenGL context
class OGLContextSaver
{
public:
    OGLContextSaver(OverlaySupport* viewport);

    ~OGLContextSaver();

private:
    OverlaySupport* currentViewport;
};

OGLContextSaver::OGLContextSaver(OverlaySupport* viewport)
: currentViewport(viewport)
{
    assert(currentViewport);
    currentViewport->saveOpenGLContext();
}

OGLContextSaver::~OGLContextSaver()
{
    currentViewport->restoreOpenGLContext();
}


struct OverlayInteractBasePrivate
{
    // The effect on which the overlay sits
    EffectInstanceWPtr effect;

    // The knobs description
    mutable boost::scoped_ptr<OverlayInteractBase::KnobsDescMap> description;

    // Set if knob custom interact
    KnobIWPtr knob;

    // Color picker data
    bool hasColorPicker;
    ColorRgba<double> pickerColor;

    // Ptr to the current viewport, valid only during an action
    OverlaySupport* currentViewport;

    bool fetchKnobCalledOnce;

    // Should the overlay be active
    bool overlayEnabled;

    OverlayInteractBasePrivate()
    : effect()
    , description()
    , knob()
    , hasColorPicker(false)
    , pickerColor()
    , currentViewport(0)
    , fetchKnobCalledOnce(false)
    , overlayEnabled(true)
    {
        
    }
};


class ViewportSetter_RAII
{
    OverlayInteractBasePrivate* _imp;
public:

    ViewportSetter_RAII(OverlayInteractBasePrivate* imp, OverlaySupport* viewport)
    : _imp(imp)
    {
        _imp->currentViewport = viewport;
    }

    ~ViewportSetter_RAII()
    {
        // Do not uncomment
        //_imp->currentViewport = 0;
    }

};

OverlayInteractBase::OverlayInteractBase()
: _imp(new OverlayInteractBasePrivate())
{
    assert(QThread::currentThread() == qApp->thread());
}

OverlayInteractBase::OverlayInteractBase(const KnobIPtr& knob)
: _imp(new OverlayInteractBasePrivate())
{
    _imp->knob = knob;
    _imp->effect = toEffectInstance(knob->getHolder());
    assert(QThread::currentThread() == qApp->thread());
    connect(this, SIGNAL(mustRedrawOnMainThread()), this, SLOT(doRedrawOnMainThread()));
}

OverlayInteractBase::~OverlayInteractBase()
{

}

const OverlayInteractBase::KnobsDescMap&
OverlayInteractBase::getDescription() const
{
    if (_imp->description) {
        return *_imp->description;
    }
    _imp->description.reset(new OverlayInteractBase::KnobsDescMap);
    describeKnobs(_imp->description.get());
    return *_imp->description;
}

void
OverlayInteractBase::describeKnobs(KnobsDescMap* /*knobs*/) const
{

}

void
OverlayInteractBase::fetchKnobs_public(const std::map<std::string, std::string>& knobs)
{
    if (_imp->fetchKnobCalledOnce) {
        return;
    }
    fetchKnobs(knobs);
    _imp->fetchKnobCalledOnce = true;
}

void
OverlayInteractBase::fetchKnobs(const std::map<std::string, std::string>& /*knobs*/) 
{

}

std::string
OverlayInteractBase::getKnobName(const std::map<std::string, std::string>& knobs, const std::string& knobRole, bool optional) const
{
    std::map<std::string, std::string>::const_iterator found = knobs.find(knobRole);
    if (found == knobs.end()) {
        if (!optional) {
            std::string m = tr("Missing non optional parameter role %1").arg(QString::fromUtf8(knobRole.c_str())).toStdString();
            throw std::invalid_argument(m);
        }
        return std::string();
    }
    return found->second;
}

KnobIPtr
OverlayInteractBase::getEffectKnobByName(const std::string& name, int nDims, bool optional) const
{
    EffectInstancePtr effect = getEffect();
    if (!effect) {
        assert(false);
        return KnobIPtr();
    }
    
    KnobIPtr knob = effect->getKnobByName(name);
    if (!knob) {
        if (!optional) {
            std::string m = tr("Could not find non optional parameter %1").arg(QString::fromUtf8(name.c_str())).toStdString();
            throw std::invalid_argument(m);
        }
        return knob;
    }
    if (knob->getNDimensions() != nDims) {
        std::string m = tr("Parameter %1 does not match required number of dimensions (%2)").arg(QString::fromUtf8(name.c_str())).arg(nDims).toStdString();
        throw std::invalid_argument(m);
        return KnobIPtr();
    }
    return knob;
}

void
OverlayInteractBase::setEffect(const EffectInstancePtr& effect)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->effect = effect;
}

bool
OverlayInteractBase::isColorPickerRequired() const
{
    return false;
}


void
OverlayInteractBase::onViewportSelectionCleared()
{

}

void
OverlayInteractBase::onViewportSelectionUpdated(const RectD& /*rectangle*/, bool /*onRelease*/)
{

}


void
OverlayInteractBase::setInteractColourPicker(const ColorRgba<double>& color, bool setColor, bool hasColor)
{
    assert(QThread::currentThread() == qApp->thread());
    if (!isColorPickerRequired()) {
        return;
    }
    if (!hasColor) {
        _imp->hasColorPicker = false;
    } else {
        _imp->hasColorPicker = true;
        if (setColor) {
            _imp->pickerColor = color;
        }
    }

    redraw();
}

bool
OverlayInteractBase::hasColorPicker() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->hasColorPicker;
}


const ColorRgba<double>&
OverlayInteractBase::getLastColorPickerColor() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->pickerColor;
}

OverlaySupport*
OverlayInteractBase::getLastCallingViewport() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->currentViewport;
}

void
OverlayInteractBase::swapOpenGLBuffers()
{
    assert(QThread::currentThread() == qApp->thread());
    if (_imp->currentViewport) {
        _imp->currentViewport->swapOpenGLBuffers();
    }
}

void
OverlayInteractBase::getOpenGLContextFormat(int* depthPerComponents, bool* hasAlpha) const
{
    assert(QThread::currentThread() == qApp->thread());
    if (_imp->currentViewport) {
        _imp->currentViewport->getOpenGLContextFormat(depthPerComponents, hasAlpha);
    }
}

void
OverlayInteractBase::getViewportSize(double &width, double &height) const
{
    assert(QThread::currentThread() == qApp->thread());
    if (_imp->currentViewport) {
        _imp->currentViewport->getViewportSize(width, height);
    }
}

void
OverlayInteractBase::getPixelScale(double & xScale, double & yScale) const
{
    assert(QThread::currentThread() == qApp->thread());
    if (_imp->currentViewport) {
        _imp->currentViewport->getPixelScale(xScale, yScale);
    }
}

#ifdef OFX_EXTENSIONS_NATRON
double
OverlayInteractBase::getScreenPixelRatio() const
{
    assert(QThread::currentThread() == qApp->thread());
    if (_imp->currentViewport) {
        return _imp->currentViewport->getScreenPixelRatio();
    }
    return 1.;
}
#endif

void
OverlayInteractBase::getBackgroundColour(double &r, double &g, double &b) const
{
    assert(QThread::currentThread() == qApp->thread());
    if (_imp->currentViewport) {
        _imp->currentViewport->getBackgroundColour(r, g, b);
    }

}

EffectInstancePtr
OverlayInteractBase::getEffect() const
{
    return _imp->effect.lock();
}

void
OverlayInteractBase::setInteractEnabled(bool enabled)
{
    if (enabled != _imp->overlayEnabled) {
        _imp->overlayEnabled = enabled;
        redraw();
    }
}

bool
OverlayInteractBase::isInteractEnabled() const
{
    return _imp->overlayEnabled;
}

bool
OverlayInteractBase::getOverlayColor(double &r, double &g, double &b) const
{
    assert(QThread::currentThread() == qApp->thread());
    EffectInstancePtr effect = getEffect();
    if (!effect) {
        return false;
    }
    return effect->getNode()->getOverlayColor(&r, &g, &b);
}

void
OverlayInteractBase::redraw()
{
    if (QThread::currentThread() != qApp->thread()) {
        Q_EMIT mustRedrawOnMainThread();
        return;
    }

    KnobIPtr isKnobInteract = _imp->knob.lock();
    if (isKnobInteract) {
        if (!_imp->currentViewport) {
            return;
        }
        _imp->currentViewport->redraw();
    } else {
        // For effects interacts, we don't just want to redraw only the calling viewer but all viewers displaying this interact.
        EffectInstancePtr effect = getEffect();
        if (!effect) {
            return;
        }
        effect->getApp()->redrawAllViewers();
    }
}

void
OverlayInteractBase::doRedrawOnMainThread()
{
    assert(QThread::currentThread() == qApp->thread());
    redraw();
}

void
OverlayInteractBase::drawOverlay_public(OverlaySupport* viewport,
                                        TimeValue time,
                                        const RenderScale & renderScale,
                                        ViewIdx view)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );

    if (!isInteractEnabled()) {
        return;
    }

    ViewportSetter_RAII viewportSetter(_imp.get(), viewport);
    {

        EffectInstancePtr effect = getEffect();
        if (!effect) {
            return;
        }
        RenderScale actualScale;
        if ( !effect->canHandleRenderScaleForOverlays() ) {
            actualScale.x = actualScale.y = 1.;
        } else {
            actualScale = renderScale;
        }

        OGLContextSaver saver(_imp->currentViewport);
        DuringInteractActionSetter_RAII _setter(effect->getNode());
        drawOverlay(time, actualScale, view);
    }

} // drawOverlay_public

bool
OverlayInteractBase::onOverlayPenDown_public(OverlaySupport* viewport,
                                             TimeValue time,
                                             const RenderScale & renderScale,
                                             ViewIdx view,
                                             const QPointF & viewportPos,
                                             const QPointF & pos,
                                             double pressure,
                                             TimeValue timestamp,
                                             PenType pen)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );

    if (!isInteractEnabled()) {
        return false;
    }

    ViewportSetter_RAII viewportSetter(_imp.get(), viewport);
    
    EffectInstancePtr effect = getEffect();
    if (!effect) {
        return false;
    }
    
    RenderScale actualScale;
    if ( !effect->canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    bool ret;
    {
        DuringInteractActionSetter_RAII _setter(effect->getNode());
        ret = onOverlayPenDown(time, actualScale, view, viewportPos, pos, pressure, timestamp, pen);
    }

    return ret;
} // onOverlayPenDown_public

bool
OverlayInteractBase::onOverlayPenDoubleClicked_public(OverlaySupport* viewport,
                                                      TimeValue time,
                                                      const RenderScale & renderScale,
                                                      ViewIdx view,
                                                      const QPointF & viewportPos,
                                                      const QPointF & pos)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );

    if (!isInteractEnabled()) {
        return false;
    }

    ViewportSetter_RAII viewportSetter(_imp.get(), viewport);

    EffectInstancePtr effect = getEffect();
    if (!effect) {
        return false;
    }

    RenderScale actualScale;
    if ( !effect->canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    bool ret;
    {
        DuringInteractActionSetter_RAII _setter(effect->getNode());
        ret = onOverlayPenDoubleClicked(time, actualScale, view, viewportPos, pos);
    }

    return ret;
} // onOverlayPenDoubleClicked_public

bool
OverlayInteractBase::onOverlayPenMotion_public(OverlaySupport* viewport,
                                               TimeValue time,
                                               const RenderScale & renderScale,
                                               ViewIdx view,
                                               const QPointF & viewportPos,
                                               const QPointF & pos,
                                               double pressure,
                                               TimeValue timestamp)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );

    if (!isInteractEnabled()) {
        return false;
    }

    ViewportSetter_RAII viewportSetter(_imp.get(), viewport);

    EffectInstancePtr effect = getEffect();
    if (!effect) {
        return false;
    }

    RenderScale actualScale;
    if ( !effect->canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    DuringInteractActionSetter_RAII _setter(effect->getNode());
    bool ret = onOverlayPenMotion(time, actualScale, view, viewportPos, pos, pressure, timestamp);
    return ret;
} // onOverlayPenMotion_public

bool
OverlayInteractBase::onOverlayPenUp_public(OverlaySupport* viewport,
                                           TimeValue time,
                                           const RenderScale & renderScale,
                                           ViewIdx view,
                                           const QPointF & viewportPos,
                                           const QPointF & pos,
                                           double pressure,
                                           TimeValue timestamp)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );

    if (!isInteractEnabled()) {
        return false;
    }

    ViewportSetter_RAII viewportSetter(_imp.get(), viewport);

    EffectInstancePtr effect = getEffect();
    if (!effect) {
        return false;
    }


    RenderScale actualScale;
    if ( !effect->canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    bool ret;
    {
        DuringInteractActionSetter_RAII _setter(effect->getNode());
        ret = onOverlayPenUp(time, actualScale, view, viewportPos, pos, pressure, timestamp);
    }

    return ret;
} // onOverlayPenUp_public

bool
OverlayInteractBase::onOverlayKeyDown_public(OverlaySupport* viewport,
                                             TimeValue time,
                                             const RenderScale & renderScale,
                                             ViewIdx view,
                                             Key key,
                                             KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );

    if (!isInteractEnabled()) {
        return false;
    }

    ViewportSetter_RAII viewportSetter(_imp.get(), viewport);

    EffectInstancePtr effect = getEffect();
    if (!effect) {
        return false;
    }
    

    RenderScale actualScale;
    if ( !effect->canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }


    bool ret;
    {
        DuringInteractActionSetter_RAII _setter(effect->getNode());
        ret = onOverlayKeyDown(time, actualScale, view, key, modifiers);
    }

    return ret;
} // onOverlayKeyDown_public

bool
OverlayInteractBase::onOverlayKeyUp_public(OverlaySupport* viewport,
                                           TimeValue time,
                                           const RenderScale & renderScale,
                                           ViewIdx view,
                                           Key key,
                                           KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );

    if (!isInteractEnabled()) {
        return false;
    }

    ViewportSetter_RAII viewportSetter(_imp.get(), viewport);

    EffectInstancePtr effect = getEffect();
    if (!effect) {
        return false;
    }


    RenderScale actualScale;
    if ( !effect->canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    bool ret;
    {
        DuringInteractActionSetter_RAII _setter(effect->getNode());
        ret = onOverlayKeyUp(time, actualScale, view, key, modifiers);
    }

    return ret;
} // onOverlayKeyUp_public

bool
OverlayInteractBase::onOverlayKeyRepeat_public(OverlaySupport* viewport,
                                               TimeValue time,
                                          const RenderScale & renderScale,
                                          ViewIdx view,
                                          Key key,
                                          KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );

    if (!isInteractEnabled()) {
        return false;
    }

    ViewportSetter_RAII viewportSetter(_imp.get(), viewport);

    EffectInstancePtr effect = getEffect();
    if (!effect) {
        return false;
    }


    RenderScale actualScale;
    if ( !effect->canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    bool ret;
    {

        DuringInteractActionSetter_RAII _setter(effect->getNode());
        ret = onOverlayKeyRepeat(time, actualScale, view, key, modifiers);
    }

    return ret;
} // onOverlayKeyRepeat_public

bool
OverlayInteractBase::onOverlayFocusGained_public(OverlaySupport* viewport,
                                                 TimeValue time,
                                                 const RenderScale & renderScale,
                                                 ViewIdx view)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );

    if (!isInteractEnabled()) {
        return false;
    }

    ViewportSetter_RAII viewportSetter(_imp.get(), viewport);

    EffectInstancePtr effect = getEffect();
    if (!effect) {
        return false;
    }

    RenderScale actualScale;
    if ( !effect->canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }

    bool ret;
    {
        DuringInteractActionSetter_RAII _setter(effect->getNode());
        ret = onOverlayFocusGained(time, actualScale, view);

    }

    return ret;
} // onOverlayFocusGained_public

bool
OverlayInteractBase::onOverlayFocusLost_public(OverlaySupport* viewport,
                                               TimeValue time,
                                               const RenderScale & renderScale,
                                               ViewIdx view)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );

    if (!isInteractEnabled()) {
        return false;
    }

    ViewportSetter_RAII viewportSetter(_imp.get(), viewport);

    EffectInstancePtr effect = getEffect();
    if (!effect) {
        return false;
    }

    RenderScale actualScale;
    if ( !effect->canHandleRenderScaleForOverlays() ) {
        actualScale.x = actualScale.y = 1.;
    } else {
        actualScale = renderScale;
    }


    bool ret;
    {

        DuringInteractActionSetter_RAII _setter(effect->getNode());
        ret = onOverlayFocusLost(time, actualScale, view);
    }
    
    return ret;
} // onOverlayFocusLost_public


void
OverlayInteractBase::drawOverlay(TimeValue /*time*/,
                                 const RenderScale & /*renderScale*/,
                                 ViewIdx /*view*/)
{
}

bool
OverlayInteractBase::onOverlayPenDown(TimeValue /*time*/,
                                      const RenderScale & /*renderScale*/,
                                      ViewIdx /*view*/,
                                      const QPointF & /*viewportPos*/,
                                      const QPointF & /*pos*/,
                                      double /*pressure*/,
                                      TimeValue /*timestamp*/,
                                      PenType /*pen*/)
{
    return false;
}

bool
OverlayInteractBase::onOverlayPenDoubleClicked(TimeValue /*time*/,
                                               const RenderScale & /*renderScale*/,
                                               ViewIdx /*view*/,
                                               const QPointF & /*viewportPos*/,
                                               const QPointF & /*pos*/)
{
    return false;
}

bool
OverlayInteractBase::onOverlayPenMotion(TimeValue /*time*/,
                                        const RenderScale & /*renderScale*/,
                                        ViewIdx /*view*/,
                                        const QPointF & /*viewportPos*/,
                                        const QPointF & /*pos*/,
                                        double /*pressure*/,
                                        TimeValue /*timestamp*/)
{
    return false;
}

bool
OverlayInteractBase::onOverlayPenUp(TimeValue /*time*/,
                                    const RenderScale & /*renderScale*/,
                                    ViewIdx /*view*/,
                                    const QPointF & /*viewportPos*/,
                                    const QPointF & /*pos*/,
                                    double /*pressure*/,
                                    TimeValue /*timestamp*/)
{
    return false;
}

bool
OverlayInteractBase::onOverlayKeyDown(TimeValue /*time*/,
                                      const RenderScale & /*renderScale*/,
                                      ViewIdx /*view*/,
                                      Key /*key*/,
                                      KeyboardModifiers /*modifiers*/)
{
    return false;
}

bool
OverlayInteractBase::onOverlayKeyUp(TimeValue /*time*/,
                                    const RenderScale & /*renderScale*/,
                                    ViewIdx /*view*/,
                                    Key /*key*/,
                                    KeyboardModifiers /*modifiers*/)
{
    return false;
}

bool
OverlayInteractBase::onOverlayKeyRepeat(TimeValue /*time*/,
                                        const RenderScale & /*renderScale*/,
                                        ViewIdx /*view*/,
                                        Key /*key*/,
                                        KeyboardModifiers /*modifiers*/)
{
    return false;
}

bool
OverlayInteractBase::onOverlayFocusGained(TimeValue /*time*/,
                                          const RenderScale & /*renderScale*/,
                                          ViewIdx /*view*/)
{
    return false;
}

bool
OverlayInteractBase::onOverlayFocusLost(TimeValue /*time*/,
                                        const RenderScale & /*renderScale*/,
                                        ViewIdx /*view*/)
{
    return false;
}


void
OverlayInteractBase::getMinimumSize(double & /*minW*/, double & /*minH*/) const
{

}


void
OverlayInteractBase::getPreferredSize(int & /*pW*/, int & /*pH*/) const
{

}


void
OverlayInteractBase::getSize(int &/*w*/, int &/*h*/) const
{

}

void
OverlayInteractBase::setSize(int /*w*/, int /*h*/)
{

}

void
OverlayInteractBase::getPixelAspectRatio(double & /*par*/) const
{

}

NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING
#include "moc_OverlayInteractBase.cpp"
