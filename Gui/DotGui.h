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

#ifndef NATRON_GUI_NODEGUI_H
#define NATRON_GUI_NODEGUI_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QRectF>
#include <QtCore/QMutex>
#include <QGraphicsItem>
#include <QDialog>
#include <QtCore/QMutex>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Gui/NodeGui.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

class DotGui
    : public NodeGui
{
public:

    DotGui(QGraphicsItem *parent = 0);

private:


    virtual void createGui() OVERRIDE FINAL;
    virtual NodeSettingsPanel* createPanel(QVBoxLayout* container, const NodeGuiPtr & thisAsShared) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool canMakePreview() OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual void refreshStateIndicator() OVERRIDE FINAL;
    virtual void applyBrush(const QBrush & brush) OVERRIDE FINAL;
    virtual bool canResize() OVERRIDE FINAL WARN_UNUSED_RETURN { return false; }

    virtual QRectF boundingRect() const OVERRIDE FINAL;
    virtual QPainterPath shape() const OVERRIDE FINAL;
    QGraphicsEllipseItem* diskShape;
    QGraphicsEllipseItem* ellipseIndicator;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_GUI_NODEGUI_H
