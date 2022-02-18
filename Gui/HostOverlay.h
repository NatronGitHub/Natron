/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

#ifndef Gui_HostOverlay_h
#define Gui_HostOverlay_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"

#include "Engine/OfxOverlayInteract.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

// defined below:
struct HostOverlayPrivate;

class DefaultInteractI
{
protected:

    HostOverlay* _overlay;

public:

    DefaultInteractI(HostOverlay* overlay);

    virtual ~DefaultInteractI();

    virtual bool isInteractForKnob(const KnobI* knob) const = 0;
    virtual void draw(double time,
                      const RenderScale& renderScale,
                      ViewIdx view,
                      const OfxPointD& pscale,
                      const QPointF& lastPenPos,
                      const OfxRGBColourD& color,
                      const OfxPointD& shadow,
                      const QFont& font,
                      const QFontMetrics& fm);
    virtual bool penMotion(double time,
                           const RenderScale& renderScale,
                           ViewIdx view,
                           const OfxPointD& pscale,
                           const QPointF& lastPenPos,
                           const QPointF &penPos,
                           const QPoint &penPosViewport,
                           double pressure);
    virtual bool penUp(double time,
                       const RenderScale& renderScale,
                       ViewIdx view,
                       const OfxPointD& pscale,
                       const QPointF& lastPenPos,
                       const QPointF &penPos,
                       const QPoint &penPosViewport,
                       double pressure);
    virtual bool penDown(double time,
                         const RenderScale& renderScale,
                         ViewIdx view,
                         const OfxPointD& pscale,
                         const QPointF& lastPenPos,
                         const QPointF &penPos,
                         const QPoint &penPosViewport,
                         double pressure);
    virtual bool penDoubleClicked(double time,
                                  const RenderScale& renderScale,
                                  ViewIdx view,
                                  const OfxPointD& pscale,
                                  const QPointF& lastPenPos,
                                  const QPointF &penPos,
                                  const QPoint &penPosViewport);
    virtual bool keyDown(double time,
                         const RenderScale& renderScale,
                         ViewIdx view,
                         int key,
                         char*   keyString);
    virtual bool keyUp(double time,
                       const RenderScale& renderScale,
                       ViewIdx view,
                       int key,
                       char*   keyString);
    virtual bool keyRepeat(double time,
                           const RenderScale& renderScale,
                           ViewIdx view,
                           int key,
                           char*   keyString);
    virtual bool gainFocus(double time,
                           const RenderScale& renderScale,
                           ViewIdx view);
    virtual bool loseFocus(double time,
                           const RenderScale& renderScale,
                           ViewIdx view);

protected:

    void renderText(float x,
                    float y,
                    float scalex,
                    float scaley,
                    const QString &text,
                    const QColor &color,
                    const QFont &font,
                    int flags = 0) const; //!< see http://doc.qt.io/qt-4.8/qpainter.html#drawText-10

    void requestRedraw();

    void getPixelScale(double& scaleX, double& scaleY) const;

#ifdef OFX_EXTENSIONS_NATRON
    double getScreenPixelRatio() const;
#endif
};


class HostOverlay
    : public NatronOverlayInteractSupport
{
public:

    HostOverlay(const NodeGuiPtr& node);

    ~HostOverlay();

    NodeGuiPtr getNode() const;

    virtual bool isColorPickerRequired() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    bool addInteract(const HostOverlayKnobsPtr& knobs);


    void draw(double time,
              const RenderScale& renderScale,
              ViewIdx view);


    bool penMotion(double time,
                   const RenderScale& renderScale,
                   ViewIdx view,
                   const QPointF &penPos,
                   const QPoint &penPosViewport,
                   double pressure);


    bool penUp(double time,
               const RenderScale& renderScale,
               ViewIdx view,
               const QPointF &penPos,
               const QPoint &penPosViewport,
               double pressure);


    bool penDown(double time,
                 const RenderScale& renderScale,
                 ViewIdx view,
                 const QPointF &penPos,
                 const QPoint &penPosViewport,
                 double pressure);

    bool penDoubleClicked(double time,
                          const RenderScale& renderScale,
                          ViewIdx view,
                          const QPointF &penPos,
                          const QPoint &penPosViewport);


    bool keyDown(double time,
                 const RenderScale& renderScale,
                 ViewIdx view,
                 int key,
                 char*   keyString);


    bool keyUp(double time,
               const RenderScale& renderScale,
               ViewIdx view,
               int key,
               char*   keyString);


    bool keyRepeat(double time,
                   const RenderScale& renderScale,
                   ViewIdx view,
                   int key,
                   char*   keyString);


    bool gainFocus(double time,
                   const RenderScale& renderScale,
                   ViewIdx view);


    bool loseFocus(double time,
                   const RenderScale& renderScale,
                   ViewIdx view);

    bool hasHostOverlayForParam(const KnobI* param);

    void removePositionHostOverlay(KnobI* knob);

    bool isEmpty() const;

private:

    friend class DefaultInteractI;
    void renderText(float x,
                    float y,
                    float scalex,
                    float scaley,
                    const QString &text,
                    const QColor &color,
                    const QFont &font,
                    int flags = 0) const; //!< see http://doc.qt.io/qt-4.8/qpainter.html#drawText-10

    void requestRedraw();

    boost::scoped_ptr<HostOverlayPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_HostOverlay_h
