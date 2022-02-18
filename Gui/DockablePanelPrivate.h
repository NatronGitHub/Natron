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

#ifndef Gui_DockablePanelPrivate_h
#define Gui_DockablePanelPrivate_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QMutex>
#include <QFrame>
#include <QTabWidget>
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/DockablePanelI.h"

#include "Gui/DockablePanel.h"
#include "Gui/Button.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER


struct DockablePanelPrivate
{
    DockablePanel* _publicInterface;
    Gui* _gui;
    QVBoxLayout* _container; /*!< ptr to the layout containing this DockablePanel*/

    /*global layout*/
    QVBoxLayout* _mainLayout;

    /*Header related*/
    QFrame* _headerWidget;
    QHBoxLayout *_headerLayout;
    LineEdit* _nameLineEdit; /*!< if the name is editable*/
    Label* _nameLabel; /*!< if the name is read-only*/
    QHBoxLayout* _horizLayout;
    QWidget* _horizContainer;
    VerticalColorBar* _verticalColorBar;

    /*Tab related*/
    QWidget* _rightContainer;
    QVBoxLayout* _rightContainerLayout;
    QTabWidget* _tabWidget;
    Button* _centerNodeButton;
    Button* _enterInGroupButton;
    Button* _helpButton;
    Button* _minimize;
    Button* _hideUnmodifiedButton;
    Button* _floatButton;
    Button* _cross;
    mutable QMutex _currentColorMutex;
    QColor _overlayColor;
    bool _hasOverlayColor;
    Button* _colorButton;
    Button* _overlayButton;
    Button* _undoButton;
    Button* _redoButton;
    Button* _restoreDefaultsButton;
    bool _minimized; /*!< true if the panel is minimized*/
    bool _floating; /*!< true if the panel is floating*/
    FloatingWidget* _floatingWidget;

    ///THe visibility of the knobs before the hide/show unmodified button is clicked
    ///to show only the knobs that need to afterwards
    std::map<KnobGuiWPtr, bool> _knobsVisibilityBeforeHideModif;
    KnobHolder* _holder;
    bool _useScrollAreasForTabs;
    DockablePanel::HeaderModeEnum _mode;
    mutable QMutex _isClosedMutex;
    bool _isClosed; //< accessed by serialization thread too
    QString _helpToolTip;
    QString _pluginID;
    QString _pluginLabel;
    unsigned _pluginVersionMajor, _pluginVersionMinor;
    bool _pagesEnabled;
    TrackerPanel* _trackerPanel;
    Label* _iconLabel;

    DockablePanelPrivate(DockablePanel* publicI,
                         Gui* gui,
                         KnobHolder* holder,
                         QVBoxLayout* container,
                         DockablePanel::HeaderModeEnum headerMode,
                         bool useScrollAreasForTabs,
                         const QString& helpToolTip);
};

class OverlayColorButton
    : public Button
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private:
    DockablePanel* _panel;

public:


    OverlayColorButton(DockablePanel* panel,
                       const QIcon& icon,
                       QWidget* parent);

private:

    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_DockablePanelPrivate_h
