/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#ifndef Gui_CurveWidgetPrivate_h
#define Gui_CurveWidgetPrivate_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <QMutex>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/OfxOverlayInteract.h"


#include "Gui/AnimationModuleBase.h"
#include "Gui/AnimationModuleViewPrivate.h"
#include "Gui/AnimationModuleSelectionModel.h"
#include "Gui/AnimationModuleUndoRedo.h"
#include "Gui/CurveGui.h"
#include "Gui/Menu.h"
#include "Gui/ZoomContext.h"

#include <QtCore/QSize>

#define CURVEWIDGET_DERIVATIVE_ROUND_PRECISION 3.

NATRON_NAMESPACE_ENTER


/*****************************CURVE WIDGET***********************************************/



// although all members are public, CurveWidgetPrivate is really a class because it has lots of member functions
// (in fact, many members could probably be made private)
class CurveWidgetPrivate : public AnimationModuleViewPrivate
{
    Q_DECLARE_TR_FUNCTIONS(CurveWidget)

public:
    CurveWidgetPrivate(Gui* gui,
                       const AnimationModuleBasePtr& model,
                       AnimationModuleView* publicInterface);

    virtual ~CurveWidgetPrivate();







public:



};

NATRON_NAMESPACE_EXIT

#endif // Gui_CurveWidgetPrivate_h
