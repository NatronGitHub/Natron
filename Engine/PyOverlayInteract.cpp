/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "PyOverlayInteract.h"

#include <QString>

#include "Engine/AppInstance.h"
#include "Engine/CornerPinOverlayInteract.h"
#include "Engine/TransformOverlayInteract.h"
#include "Engine/PyAppInstance.h"
#include "Engine/EffectInstance.h"
#include "Engine/PyNode.h"
#include "Engine/OverlaySupport.h"
#include "Engine/PointOverlayInteract.h"
#include "Engine/OverlayInteractBase.h"
#include "Engine/QtEnumConvert.h"
#include "Engine/Project.h"

NATRON_NAMESPACE_ENTER
NATRON_PYTHON_NAMESPACE_ENTER

struct PyOverlayInteractPrivate
{
    PyOverlayInteract* _publicInterface;
    EffectInstanceWPtr effect;
    bool pickingEnabled;

    OverlayInteractBasePtr ui;

    PyOverlayInteractPrivate(PyOverlayInteract* publicInterface)
    : _publicInterface(publicInterface)
    , effect()
    , pickingEnabled(false)
    , ui()
    {

    }

    QString viewToString(ViewIdx view) const
    {
        EffectInstancePtr e = effect.lock();
        if (!e) {
            return QString::fromUtf8(kPyParamViewIdxMain);
        }
        return QString::fromUtf8(e->getApp()->getProject()->getViewName(view).c_str());
    }
};


class PyOverlayInteractImpl : public OverlayInteractBase
{
    PyOverlayInteractPrivate* _imp;
public:

    PyOverlayInteractImpl(PyOverlayInteractPrivate* imp)
    : OverlayInteractBase()
    , _imp(imp)
    {

    }

    virtual ~PyOverlayInteractImpl()
    {
        
    }

    virtual bool isColorPickerRequired() const OVERRIDE FINAL;
    
private:

    virtual void describeKnobs(KnobsDescMap* knobs) const OVERRIDE FINAL;

    virtual void fetchKnobs(const std::map<std::string, std::string>& knobs) OVERRIDE FINAL;

    virtual void drawOverlay(TimeValue time,
                             const RenderScale & renderScale,
                             ViewIdx view) OVERRIDE ;

    virtual bool onOverlayPenDown(TimeValue time,
                                  const RenderScale & renderScale,
                                  ViewIdx view,
                                  const QPointF & viewportPos,
                                  const QPointF & pos,
                                  double pressure,
                                  TimeValue timestamp,
                                  PenType pen) OVERRIDE WARN_UNUSED_RETURN;

    virtual bool onOverlayPenDoubleClicked(TimeValue time,
                                           const RenderScale & renderScale,
                                           ViewIdx view,
                                           const QPointF & viewportPos,
                                           const QPointF & pos) OVERRIDE WARN_UNUSED_RETURN;

    virtual bool onOverlayPenMotion(TimeValue time,
                                    const RenderScale & renderScale,
                                    ViewIdx view,
                                    const QPointF & viewportPos,
                                    const QPointF & pos,
                                    double pressure,
                                    TimeValue timestamp) OVERRIDE  WARN_UNUSED_RETURN;

    virtual bool onOverlayPenUp(TimeValue time,
                                const RenderScale & renderScale,
                                ViewIdx view,
                                const QPointF & viewportPos,
                                const QPointF & pos,
                                double pressure,
                                TimeValue timestamp) OVERRIDE WARN_UNUSED_RETURN;

    virtual bool onOverlayKeyDown(TimeValue time,
                                  const RenderScale & renderScale,
                                  ViewIdx view,
                                  Key key,
                                  KeyboardModifiers modifiers) OVERRIDE WARN_UNUSED_RETURN;

    virtual bool onOverlayKeyUp(TimeValue time,
                                const RenderScale & renderScale,
                                ViewIdx view,
                                Key key,
                                KeyboardModifiers modifiers) OVERRIDE WARN_UNUSED_RETURN;

    virtual bool onOverlayKeyRepeat(TimeValue time,
                                    const RenderScale & renderScale,
                                    ViewIdx view,
                                    Key key,
                                    KeyboardModifiers modifiers) OVERRIDE WARN_UNUSED_RETURN;

    virtual bool onOverlayFocusGained(TimeValue time,
                                      const RenderScale & renderScale,
                                      ViewIdx view) OVERRIDE WARN_UNUSED_RETURN;

    virtual bool onOverlayFocusLost(TimeValue time,
                                    const RenderScale & renderScale,
                                    ViewIdx view) OVERRIDE WARN_UNUSED_RETURN;
};


PyOverlayInteract::PyOverlayInteract()
: _imp(new PyOverlayInteractPrivate(this))
{
}

PyOverlayInteract::~PyOverlayInteract()
{
    
}

void
PyOverlayInteract::initInternalInteract()
{
    _imp->ui = createInternalInteract();
}

OverlayInteractBasePtr
PyOverlayInteract::createInternalInteract()
{
    _imp->ui.reset(new PyOverlayInteractImpl(_imp.get()));
    return _imp->ui;
}

OverlayInteractBasePtr
PyOverlayInteract::getInternalInteract() const
{
    return _imp->ui;
}

Effect*
PyOverlayInteract::getHoldingEffect() const
{
    EffectInstancePtr effect = _imp->ui->getEffect();
    if (!effect) {
        return 0;
    }
    return App::createEffectFromNodeWrapper(effect->getNode());
}

void
PyOverlayInteract::setColorPickerEnabled(bool enabled)
{
    _imp->pickingEnabled = enabled;
}

bool
PyOverlayInteractImpl::isColorPickerRequired() const
{
    return _imp->_publicInterface->isColorPickerRequired();
}

bool
PyOverlayInteract::isColorPickerRequired() const
{
    return _imp->pickingEnabled;
}

static Double2DTuple renderScale2Tuple(const RenderScale& renderScale) {
    Double2DTuple scale;
    scale.x = renderScale.x;
    scale.y = renderScale.y;
    return scale;
}

bool
PyOverlayInteract::isColorPickerValid() const
{
    return _imp->ui->hasColorPicker();
}

Double2DTuple
PyOverlayInteract::getPixelScale() const
{
    Double2DTuple ret;
    _imp->ui->getPixelScale(ret.x, ret.y);
    return ret;
}

#ifdef OFX_EXTENSIONS_NATRON
double
PyOverlayInteract::getScreenPixelRatio() const
{
    return _imp->ui->getScreenPixelRatio();
}
#endif

ColorTuple
PyOverlayInteract::getColorPicker() const
{
    ColorTuple ret;
    const ColorRgba<double> &color = _imp->ui->getLastColorPickerColor();
    ret.r = color.r;
    ret.g = color.g;
    ret.b = color.b;
    ret.a = color.a;
    return ret;
}

Int2DTuple
PyOverlayInteract::getViewportSize() const
{
    Int2DTuple ret;
    double x,y;
    _imp->ui->getViewportSize(x,y);
    ret.x = x;
    ret.y = y;
    return ret;
}

bool
PyOverlayInteract::getSuggestedColor(ColorTuple* color) const
{
    if (!color) {
        return false;
    }
    return _imp->ui->getOverlayColor(color->r, color->g, color->b);
}

void
PyOverlayInteract::redraw()
{
    _imp->ui->redraw();
}

void
PyOverlayInteractImpl::describeKnobs(KnobsDescMap* knobs) const
{
    std::map<QString,PyOverlayParamDesc> tmp = _imp->_publicInterface->describeParameters();
    for (std::map<QString,PyOverlayParamDesc>::const_iterator it = tmp.begin(); it != tmp.end(); ++it) {
        KnobDesc& k = (*knobs)[it->first.toStdString()];
        k.nDims = it->second.nDims;
        k.type = it->second.type.toStdString();
        k.optional = it->second.optional;
    }
}

std::map<QString, PyOverlayParamDesc>
PyOverlayInteract::describeParameters() const
{
    return std::map<QString,PyOverlayParamDesc>();
}

void
PyOverlayInteractImpl::fetchKnobs(const std::map<std::string, std::string>& knobs)
{
    std::map<QString, QString> pyknobs;
    for (std::map<std::string, std::string>::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        pyknobs.insert(std::make_pair(QString::fromUtf8(it->first.c_str()), QString::fromUtf8(it->second.c_str())));
    }
    _imp->_publicInterface->fetchParameters(pyknobs);
}

void
PyOverlayInteract::fetchParameters(const std::map<QString, QString>& /*params*/)
{

}

Param*
PyOverlayInteract::fetchParameter(const std::map<QString, QString>& params,
                                  const QString& role,
                                  const QString& type,
                                  int nDims,
                                  bool optional) const
{

    std::map<std::string, std::string> knobsMap;
    for (std::map<QString, QString>::const_iterator it = params.begin(); it != params.end(); ++it) {
        knobsMap.insert(std::make_pair(it->first.toStdString(), it->second.toStdString()));
    }
    std::string knobName;
    try {
        knobName = _imp->ui->getKnobName(knobsMap, role.toStdString(), optional);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return 0;
    }
    if (knobName.empty()) {
        return 0;
    }
    KnobIPtr knob;
    try {
        knob = _imp->ui->getEffectKnobByName(knobName, nDims, optional);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return 0;
    }
    if (!knob) {
        return 0;
    }
    if (knob->typeName() != type.toStdString()) {
        PyErr_SetString(PyExc_RuntimeError, OverlayInteractBase::tr("Invalid parameter type %1").arg(type).toStdString().c_str());
        return 0;
    }
    return Effect::createParamWrapperForKnob(knob);
}

void
PyOverlayInteract::renderText(double x,
                              double y,
                              const QString &string,
                              double r,
                              double g,
                              double b,
                              double a)
{
    OverlaySupport* overlay = _imp->ui->getLastCallingViewport();
    if (!overlay) {
        return;
    }
    overlay->renderText(x, y, string.toStdString(), r, g, b, a);
}

void
PyOverlayInteractImpl::drawOverlay(TimeValue time,
                         const RenderScale & renderScale,
                         ViewIdx view)
{

    Double2DTuple scale = renderScale2Tuple(renderScale);
    QString viewStr = _imp->viewToString(view);
    _imp->_publicInterface->draw(time, scale, viewStr);
}

bool
PyOverlayInteractImpl::onOverlayPenDown(TimeValue time,
                              const RenderScale & renderScale,
                              ViewIdx view,
                              const QPointF & viewportPos,
                              const QPointF & pos,
                              double pressure,
                              TimeValue timestamp,
                              PenType pen)
{
    Double2DTuple scale = renderScale2Tuple(renderScale);
    QString viewStr = _imp->viewToString(view);

    Double2DTuple viewportPosTuple = {viewportPos.x(), viewportPos.y()};
    Double2DTuple penPosTuple = {pos.x(), pos.y()};
    return _imp->_publicInterface->penDown(time, scale, viewStr, viewportPosTuple, penPosTuple, pressure, timestamp, pen);
}

bool
PyOverlayInteractImpl::onOverlayPenDoubleClicked(TimeValue time,
                                       const RenderScale & renderScale,
                                       ViewIdx view,
                                       const QPointF & viewportPos,
                                       const QPointF & pos)
{
    Double2DTuple scale = renderScale2Tuple(renderScale);
    QString viewStr = _imp->viewToString(view);

    Double2DTuple viewportPosTuple = {viewportPos.x(), viewportPos.y()};
    Double2DTuple penPosTuple = {pos.x(), pos.y()};
    return _imp->_publicInterface->penDoubleClicked(time, scale, viewStr, viewportPosTuple, penPosTuple);
}

bool
PyOverlayInteractImpl::onOverlayPenMotion(TimeValue time,
                                const RenderScale & renderScale,
                                ViewIdx view,
                                const QPointF & viewportPos,
                                const QPointF & pos,
                                double pressure,
                                TimeValue timestamp)
{
    Double2DTuple scale = renderScale2Tuple(renderScale);
    QString viewStr = _imp->viewToString(view);

    Double2DTuple viewportPosTuple = {viewportPos.x(), viewportPos.y()};
    Double2DTuple penPosTuple = {pos.x(), pos.y()};
    return _imp->_publicInterface->penMotion(time, scale, viewStr, viewportPosTuple, penPosTuple, pressure, timestamp);

}

bool
PyOverlayInteractImpl::onOverlayPenUp(TimeValue time,
                            const RenderScale & renderScale,
                            ViewIdx view,
                            const QPointF & viewportPos,
                            const QPointF & pos,
                            double pressure,
                            TimeValue timestamp)
{
    Double2DTuple scale = renderScale2Tuple(renderScale);
    QString viewStr = _imp->viewToString(view);

    Double2DTuple viewportPosTuple = {viewportPos.x(), viewportPos.y()};
    Double2DTuple penPosTuple = {pos.x(), pos.y()};
    return _imp->_publicInterface->penUp(time, scale, viewStr, viewportPosTuple, penPosTuple, pressure, timestamp);

}

bool
PyOverlayInteractImpl::onOverlayKeyDown(TimeValue time,
                              const RenderScale & renderScale,
                              ViewIdx view,
                              Key key,
                              KeyboardModifiers modifiers)
{
    Double2DTuple scale = renderScale2Tuple(renderScale);
    QString viewStr = _imp->viewToString(view);

    Qt::Key qkey = QtEnumConvert::toQtKey(key);
    Qt::KeyboardModifiers qmods = QtEnumConvert::toQtModifiers(modifiers);
    return _imp->_publicInterface->keyDown(time, scale, viewStr, qkey, qmods);

}

bool
PyOverlayInteractImpl::onOverlayKeyUp(TimeValue time,
                            const RenderScale & renderScale,
                            ViewIdx view,
                            Key key,
                            KeyboardModifiers modifiers)
{
    Double2DTuple scale = renderScale2Tuple(renderScale);
    QString viewStr = _imp->viewToString(view);

    Qt::Key qkey = QtEnumConvert::toQtKey(key);
    Qt::KeyboardModifiers qmods = QtEnumConvert::toQtModifiers(modifiers);
    return _imp->_publicInterface->keyUp(time, scale, viewStr, qkey, qmods);
}

bool
PyOverlayInteractImpl::onOverlayKeyRepeat(TimeValue time,
                                const RenderScale & renderScale,
                                ViewIdx view,
                                Key key,
                                KeyboardModifiers modifiers)
{
    Double2DTuple scale = renderScale2Tuple(renderScale);
    QString viewStr = _imp->viewToString(view);

    Qt::Key qkey = QtEnumConvert::toQtKey(key);
    Qt::KeyboardModifiers qmods = QtEnumConvert::toQtModifiers(modifiers);
    return _imp->_publicInterface->keyRepeat(time, scale, viewStr, qkey, qmods);
}

bool
PyOverlayInteractImpl::onOverlayFocusGained(TimeValue time,
                                  const RenderScale & renderScale,
                                  ViewIdx view)
{
    Double2DTuple scale = renderScale2Tuple(renderScale);
    QString viewStr = _imp->viewToString(view);
    return _imp->_publicInterface->focusGained(time, scale, viewStr);
}

bool
PyOverlayInteractImpl::onOverlayFocusLost(TimeValue time,
                                const RenderScale & renderScale,
                                ViewIdx view)
{
    Double2DTuple scale = renderScale2Tuple(renderScale);
    QString viewStr = _imp->viewToString(view);
    return _imp->_publicInterface->focusLost(time, scale, viewStr);
}


void
PyOverlayInteract::draw(double /*time*/, const Double2DTuple& /*renderScale*/, QString /*view*/)
{

}

bool
PyOverlayInteract::penDown(double /*time*/,
                     const Double2DTuple& /*renderScale*/,
                           const QString& /*view*/,
                           const Double2DTuple & /*viewportPos*/,
                           const Double2DTuple & /*pos*/,
                           double /*pressure*/,
                           double /*timestamp*/,
                           PenType /*pen*/)
{
    return false;
}

bool
PyOverlayInteract::penDoubleClicked(double /*time*/,
                                    const Double2DTuple & /*renderScale*/,
                                    const QString& /*view*/,
                                    const Double2DTuple & /*viewportPos*/,
                                    const Double2DTuple & /*pos*/)
{
    return false;
}

bool
PyOverlayInteract::penMotion(double /*time*/,
                             const Double2DTuple & /*renderScale*/,
                             const QString& /*view*/,
                             const Double2DTuple & /*viewportPos*/,
                             const Double2DTuple & /*pos*/,
                             double /*pressure*/,
                             double /*timestamp*/)
{
    return false;
}

bool
PyOverlayInteract::penUp(double /*time*/,
                         const Double2DTuple & /*renderScale*/,
                         const QString& /*view*/,
                         const Double2DTuple & /*viewportPos*/,
                         const Double2DTuple & /*pos*/,
                         double /*pressure*/,
                         double /*timestamp*/)
{
    return false;
}

bool
PyOverlayInteract::keyDown(double /*time*/,
                           const Double2DTuple & /*renderScale*/,
                           const QString& /*view*/,
                           Qt::Key /*key*/,
                           const Qt::KeyboardModifiers& /*modifiers*/)
{
    return false;
}

bool
PyOverlayInteract::keyUp(double /*time*/,
                         const Double2DTuple & /*renderScale*/,
                         const QString& /*view*/,
                         Qt::Key /*key*/,
                         const Qt::KeyboardModifiers& /*modifiers*/)
{
    return false;
}

bool
PyOverlayInteract::keyRepeat(double /*time*/,
                             const Double2DTuple & /*renderScale*/,
                             const QString& /*view*/,
                             Qt::Key /*key*/,
                             const Qt::KeyboardModifiers& /*modifiers*/)
{
    return false;
}

bool
PyOverlayInteract::focusGained(double /*time*/,
                               const Double2DTuple & /*renderScale*/,
                               const QString& /*view*/)
{
    return false;
}

bool
PyOverlayInteract::focusLost(double /*time*/,
                             const Double2DTuple & /*renderScale*/,
                             const QString& /*view*/)
{
    return false;
}

OverlayInteractBasePtr
PyPointOverlayInteract::createInternalInteract()
{
    OverlayInteractBasePtr ret(new PointOverlayInteract());
    return ret;
}

PyPointOverlayInteract::PyPointOverlayInteract()
: PyOverlayInteract()
{

}

PyTransformOverlayInteract::PyTransformOverlayInteract()
: PyOverlayInteract()
{

}

OverlayInteractBasePtr
PyTransformOverlayInteract::createInternalInteract()
{
    OverlayInteractBasePtr ret(new TransformOverlayInteract());
    return ret;
}

PyCornerPinOverlayInteract::PyCornerPinOverlayInteract()
: PyOverlayInteract()
{

}

OverlayInteractBasePtr
PyCornerPinOverlayInteract::createInternalInteract()
{
    OverlayInteractBasePtr ret(new CornerPinOverlayInteract());
    return ret;
}

NATRON_PYTHON_NAMESPACE_EXIT
NATRON_NAMESPACE_EXIT
