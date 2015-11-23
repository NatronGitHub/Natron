/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef Gui_TabGroup_h
#define Gui_TabGroup_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QFrame>
#include <QTabWidget>
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/DockablePanelI.h"

class KnobI;
class KnobGui;
class KnobHolder;
class NodeGui;
class Gui;
class KnobPage;
class QVBoxLayout;
class Button;
class QUndoStack;
class QUndoCommand;
class QGridLayout;
class RotoPanel;
class MultiInstancePanel;
class QTabWidget;
class KnobGroup;

/**
 * @brief Used when group are using the kFnOfxParamPropGroupIsTab extension
 **/
class TabGroup : public QFrame
{
    QTabWidget* _tabWidget;
    std::vector<boost::weak_ptr<KnobGroup> > _tabs;
    
public:
    
    TabGroup(QWidget* parent);
    
    QGridLayout* addTab(const boost::shared_ptr<KnobGroup>& group, const QString &label);
    
    void removeTab(KnobGroup* group);
    
    bool isEmpty() const
    {
        return _tabs.empty();
    }
};

#endif // Gui_TabGroup_h
