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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "DockablePanelPrivate.h"

#include <vector>
#include <utility>
#include <stdexcept>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

#include <QtCore/QDebug>
#include <QGridLayout>
#include <QUndoStack>
#include <QScrollArea>
#include <QApplication>
#include <QStyle>
#include <QMessageBox>
#include <QMouseEvent>

#include "Engine/KnobTypes.h"
#include "Engine/Node.h" // NATRON_PARAMETER_PAGE_NAME_INFO
#include "Engine/ViewIdx.h"

#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/GuiMacros.h"
#include "Gui/KnobGui.h"
#include "Gui/KnobGuiGroup.h" // for KnobGuiGroup
#include "Gui/KnobWidgetDnD.h"
#include "Gui/Label.h"
#include "Gui/TabGroup.h"


using std::make_pair;
NATRON_NAMESPACE_ENTER


DockablePanelPrivate::DockablePanelPrivate(DockablePanel* publicI,
                                           Gui* gui,
                                           const KnobHolderPtr& holder,
                                           QVBoxLayout* container,
                                           DockablePanel::HeaderModeEnum headerMode,
                                           bool useScrollAreasForTabs,
                                           const QString& helpToolTip)
    : _publicInterface(publicI)
    , _gui(gui)
    , _container(container)
    , _mainLayout(NULL)
    , _headerWidget(NULL)
    , _headerLayout(NULL)
    , _nameLineEdit(NULL)
    , _nameLabel(NULL)
    , _horizLayout(0)
    , _horizContainer(0)
    , _verticalColorBar(0)
    , _rightContainer(0)
    , _rightContainerLayout(0)
    , _tabWidget(NULL)
    , _centerNodeButton(NULL)
    , _enterInGroupButton(NULL)
    , _helpButton(NULL)
    , _minimize(NULL)
    , _hideUnmodifiedButton(NULL)
    , _floatButton(NULL)
    , _cross(NULL)
    , _colorButton(NULL)
    , _overlayButton(NULL)
    , _undoButton(NULL)
    , _redoButton(NULL)
    , _restoreDefaultsButton(NULL)
    , _minimized(false)
    , _floating(false)
    , _floatingWidget(NULL)
    , _knobsVisibilityBeforeHideModif()
    , _holder(holder)
    , _useScrollAreasForTabs(useScrollAreasForTabs)
    , _mode(headerMode)
    , _isClosedMutex()
    , _isClosed(false)
    , _helpToolTip(helpToolTip)
    , _pluginID()
    , _pluginVersionMajor(0)
    , _pluginVersionMinor(0)
    , _iconLabel(0)
{
}

////////////////////////////////////////////////////////////////////////////////////////

OverlayColorButton::OverlayColorButton(DockablePanel* panel,
                                       const QIcon& icon,
                                       QWidget* parent)
    : Button(icon, QString(), parent)
    , _panel(panel)
{
}

void
OverlayColorButton::mousePressEvent(QMouseEvent* e)
{
    if ( triggerButtonIsRight(e) ) {
        StandardButtonEnum rep = Dialogs::questionDialog(tr("Warning").toStdString(),
                                                         tr("Are you sure you want to reset the overlay color?").toStdString(),
                                                         false);
        if (rep == eStandardButtonYes) {
            _panel->resetHostOverlayColor();
        }
    } else {
        Button::mousePressEvent(e);
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_DockablePanelPrivate.cpp"
