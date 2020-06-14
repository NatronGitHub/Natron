/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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

#ifndef KNOBWIDGETDND_H
#define KNOBWIDGETDND_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
#endif

#include <QtCore/QCoreApplication>

#include "Gui/GuiFwd.h"

#define KNOB_DND_MIME_DATA_KEY "KnobLink"


NATRON_NAMESPACE_ENTER

struct KnobWidgetDnDPrivate;
class KnobWidgetDnD : public boost::enable_shared_from_this<KnobWidgetDnD>
{
    Q_DECLARE_TR_FUNCTIONS(KnobWidgetDnD)

    struct MakeSharedEnabler;

    // used by boost::make_shared
    KnobWidgetDnD(const KnobGuiPtr& knob,
                  int dimension,
                  QWidget* widget);

public:


    static KnobWidgetDnDPtr create(const KnobGuiPtr& knob,
                                            int dimension,
                                                   QWidget* widget);

    ~KnobWidgetDnD();

    QWidget* getWidget() const;

    void keyPress(QKeyEvent* e);
    void keyRelease(QKeyEvent* e);
    bool mousePress(QMouseEvent* e);
    bool mouseMove(QMouseEvent* e);
    void mouseRelease(QMouseEvent* e);
    bool mouseWheel(QWheelEvent* e);
    bool dragEnter(QDragEnterEvent* e);
    bool dragMove(QDragMoveEvent* e);
    bool drop(QDropEvent* e);
    void mouseEnter(QEvent* e);
    void mouseLeave(QEvent* e);
    void focusOut();
    void focusIn();

private:
    void startDrag();

    boost::scoped_ptr<KnobWidgetDnDPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // KNOBWIDGETDND_H
