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

#ifndef CORNERPINOVERLAYINTERACT_H
#define CORNERPINOVERLAYINTERACT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/OverlayInteractBase.h"

NATRON_NAMESPACE_ENTER


struct CornerPinOverlayInteractPrivate;
class CornerPinOverlayInteract : public OverlayInteractBase
{
public:

    CornerPinOverlayInteract();

    virtual ~CornerPinOverlayInteract();

    /**
     * @brief Enables auto-keying of the to points upon interaction.
     * Defaults to false
     **/
    void setAutoKeyEnabledOnToPoints(bool enabled);

    /**
     * @brief If disabled, the user cannot interact with the to points and they will not
     * be visible.
     * Defaults to true
     **/
    void setToPointsEnabledIfNotAnimated(bool enabled);

private:

    virtual void describeKnobs(OverlayInteractBase::KnobsDescMap* desc) const OVERRIDE FINAL;

    virtual void fetchKnobs(const std::map<std::string, std::string>& knobs) OVERRIDE FINAL;

    virtual void drawOverlay(TimeValue time,
                             const RenderScale & renderScale,
                             ViewIdx view) OVERRIDE FINAL;

    virtual bool onOverlayPenDown(TimeValue time,
                                  const RenderScale & renderScale,
                                  ViewIdx view,
                                  const QPointF & viewportPos,
                                  const QPointF & pos,
                                  double pressure,
                                  TimeValue timestamp,
                                  PenType pen) OVERRIDE FINAL WARN_UNUSED_RETURN;


    virtual bool onOverlayPenMotion(TimeValue time,
                                    const RenderScale & renderScale,
                                    ViewIdx view,
                                    const QPointF & viewportPos,
                                    const QPointF & pos,
                                    double pressure,
                                    TimeValue timestamp) OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool onOverlayPenUp(TimeValue time,
                                const RenderScale & renderScale,
                                ViewIdx view,
                                const QPointF & viewportPos,
                                const QPointF & pos,
                                double pressure,
                                TimeValue timestamp) OVERRIDE FINAL WARN_UNUSED_RETURN;

      virtual bool onOverlayFocusLost(TimeValue time,
                                    const RenderScale & renderScale,
                                    ViewIdx view) OVERRIDE FINAL WARN_UNUSED_RETURN;

private:

    boost::scoped_ptr<CornerPinOverlayInteractPrivate> _imp;
};



NATRON_NAMESPACE_EXIT

#endif // CORNERPINOVERLAYINTERACT_H
