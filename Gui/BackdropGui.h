/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#ifndef Gui_BackdropGui_h
#define Gui_BackdropGui_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Gui/NodeGui.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

struct BackdropGuiPrivate;
class BackdropGui
    : public NodeGui
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    BackdropGui(QGraphicsItem* parent = 0);

    virtual ~BackdropGui();

    void refreshTextLabelFromKnob();

public Q_SLOTS:

    void onLabelChanged(const QString& label);

private:

    virtual int getBaseDepth() const OVERRIDE FINAL { return 10; }

    virtual void createGui() OVERRIDE FINAL;
    virtual bool canMakePreview() OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual void resizeExtraContent(int w, int h, bool forceResize) OVERRIDE FINAL;
    virtual bool mustFrameName() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool mustAddResizeHandle() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual void adjustSizeToContent(int *w, int *h, bool adjustToTextSize) OVERRIDE FINAL;
    virtual void getInitialSize(int *w, int *h) const OVERRIDE FINAL;

private:
    boost::scoped_ptr<BackdropGuiPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_BackdropGui_h
