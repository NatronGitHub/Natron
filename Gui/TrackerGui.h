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

#ifndef TRACKERGUI_H
#define TRACKERGUI_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/GlobalDefines.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QObject>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;

struct TrackerGuiPrivate;

class TrackerGui
    : public QObject
{
    Q_OBJECT

public:

    TrackerGui(const boost::shared_ptr<TrackerPanel> & panel,
               ViewerTab* parent);

    virtual ~TrackerGui();

    /**
     * @brief Return the horizontal buttons bar.
     **/
    QWidget* getButtonsBar() const;


    void drawOverlays(double time, const RenderScale & renderScale, int view) const;

    bool penDown(double time, const RenderScale & renderScale, int view, const QPointF & viewportPos, const QPointF & pos, double pressure, QMouseEvent* e);

    bool penDoubleClicked(double time, const RenderScale & renderScale, int view, const QPointF & viewportPos, const QPointF & pos, QMouseEvent* e);

    bool penMotion(double time, const RenderScale & renderScale, int view, const QPointF & viewportPos, const QPointF & pos, double pressure, QInputEvent* e);

    bool penUp(double time, const RenderScale & renderScale, int view, const QPointF & viewportPos, const QPointF & pos, double pressure, QMouseEvent* e);

    bool keyDown(double time, const RenderScale & renderScale, int view, QKeyEvent* e);

    bool keyUp(double time, const RenderScale & renderScale, int view, QKeyEvent* e);

    bool loseFocus(double time, const RenderScale & renderScale, int view);

public Q_SLOTS:

    void onAddTrackClicked(bool clicked);

    void onTrackBwClicked();

    void onTrackPrevClicked();

    void onStopButtonClicked();

    void onTrackNextClicked();

    void onTrackFwClicked();

    void onUpdateViewerClicked(bool clicked);

    void onClearAllAnimationClicked();

    void onClearBwAnimationClicked();

    void onClearFwAnimationClicked();

    void updateSelectionFromSelectionRectangle(bool onRelease);

    void onSelectionCleared();

    void onTrackingEnded();
private:

    boost::scoped_ptr<TrackerGuiPrivate> _imp;
};

NATRON_NAMESPACE_EXIT;

#endif // TRACKERGUI_H
