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

#ifndef NATRON_ENGINE_PYOVERLAYINTERACT_H
#define NATRON_ENGINE_PYOVERLAYINTERACT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <map>
#include <QString>
#include "Global/Macros.h"

#include "Engine/PyParameter.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;
NATRON_PYTHON_NAMESPACE_ENTER;


struct PyOverlayParamDesc
{
    QString type;
    int nDims;
    bool optional;
};

struct PyOverlayInteractPrivate;
class Effect;
class PyOverlayInteract 
{
public:


    PyOverlayInteract();

    virtual ~PyOverlayInteract();

    Effect* getHoldingEffect() const;

    void setColorPickerEnabled(bool enabled);

    bool isColorPickerValid() const;

    ColorTuple getColorPicker() const;

    Double2DTuple getPixelScale() const;

#ifdef OFX_EXTENSIONS_NATRON
    double getScreenPixelRatio() const;
#endif

    Int2DTuple getViewportSize() const;

    bool getSuggestedColor(ColorTuple* color) const;

    void redraw();

    bool isColorPickerRequired() const;

    virtual std::map<QString, PyOverlayParamDesc> describeParameters() const;

    virtual void fetchParameters(const std::map<QString, QString>& params);

    void renderText(double x,
                    double y,
                    const QString &string,
                    double r,
                    double g,
                    double b,
                    double a);

protected:

    Param* fetchParameter(const std::map<QString, QString>& params,
                          const QString& role,
                          const QString& type,
                          int nDims,
                          bool optional) const;

public:

    virtual void draw(double time, const Double2DTuple& renderScale, QString view);

    virtual bool penDown(double time,
                         const Double2DTuple& renderScale,
                         const QString& view,
                         const Double2DTuple & viewportPos,
                         const Double2DTuple & pos,
                         double pressure,
                         double timestamp,
                         PenType pen) WARN_UNUSED_RETURN;

    virtual bool penDoubleClicked(double time,
                                  const Double2DTuple & renderScale,
                                  const QString& view,
                                  const Double2DTuple & viewportPos,
                                  const Double2DTuple & pos) WARN_UNUSED_RETURN;

    virtual bool penMotion(double time,
                           const Double2DTuple & renderScale,
                           const QString& view,
                           const Double2DTuple & viewportPos,
                           const Double2DTuple & pos,
                           double pressure,
                           double timestamp) WARN_UNUSED_RETURN;

    virtual bool penUp(double time,
                       const Double2DTuple & renderScale,
                       const QString& view,
                       const Double2DTuple & viewportPos,
                       const Double2DTuple & pos,
                       double pressure,
                       double timestamp) WARN_UNUSED_RETURN;

    virtual bool keyDown(double time,
                         const Double2DTuple & renderScale,
                         const QString& view,
                         Qt::Key key,
                         const Qt::KeyboardModifiers& modifiers) WARN_UNUSED_RETURN;

    virtual bool keyUp(double time,
                       const Double2DTuple & renderScale,
                       const QString& view,
                       Qt::Key key,
                       const Qt::KeyboardModifiers& modifiers) WARN_UNUSED_RETURN;

    virtual bool keyRepeat(double time,
                           const Double2DTuple & renderScale,
                           const QString& view,
                           Qt::Key key,
                           const Qt::KeyboardModifiers& modifiers) WARN_UNUSED_RETURN;

    virtual bool focusGained(double time,
                             const Double2DTuple & renderScale,
                             const QString& view) WARN_UNUSED_RETURN;

    virtual bool focusLost(double time,
                           const Double2DTuple & renderScale,
                           const QString& view) WARN_UNUSED_RETURN;

    OverlayInteractBasePtr getInternalInteract() const;

protected:

    void initInternalInteract();

    virtual OverlayInteractBasePtr createInternalInteract();

private:

    friend class Effect;

    boost::scoped_ptr<PyOverlayInteractPrivate> _imp;
};

class PyPointOverlayInteract : public PyOverlayInteract
{
public:

    PyPointOverlayInteract();

    virtual ~PyPointOverlayInteract()
    {

    }

private:

    virtual OverlayInteractBasePtr createInternalInteract() OVERRIDE FINAL;
};

class PyTransformOverlayInteract : public PyOverlayInteract
{
public:

    PyTransformOverlayInteract();

    virtual ~PyTransformOverlayInteract()
    {
    }

private:

    virtual OverlayInteractBasePtr createInternalInteract() OVERRIDE FINAL;

};

class PyCornerPinOverlayInteract : public PyOverlayInteract
{
public:

    PyCornerPinOverlayInteract();

    virtual ~PyCornerPinOverlayInteract()
    {
        
    }

private:

    virtual OverlayInteractBasePtr createInternalInteract() OVERRIDE FINAL;
};
NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;

#endif // PYOVERLAYINTERACT_H
