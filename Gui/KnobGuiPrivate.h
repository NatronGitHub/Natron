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

#ifndef _Gui_KnobGuiPrivate_h_
#define _Gui_KnobGuiPrivate_h_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Gui/KnobGui.h"

#include <cassert>
#include <climits>
#include <cfloat>
#include <stdexcept>
#include <map>
#include <boost/weak_ptr.hpp>


#include <QtCore/QString>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFormLayout>
#include <QFileDialog>
#include <QTextEdit>
#include <QStyle> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QTimer>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QColorDialog>
#include <QGroupBox>
#include <QtGui/QVector4D>
#include <QStyleFactory>
#include <QDialogButtonBox>
#include <QCompleter>

#include "Global/GlobalDefines.h"

#include "Engine/Curve.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/Variant.h"
#include "Engine/ViewerInstance.h"

#include "Gui/ComboBox.h"
#include "Gui/ClickableLabel.h"
#include "Gui/CurveGui.h"
#include "Gui/CustomParamInteract.h"
#include "Gui/DockablePanel.h"
#include "Gui/EditExpressionDialog.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GroupBoxLabel.h"
#include "Gui/KnobGuiGroup.h"
#include "Gui/KnobGuiWidgets.h"
#include "Gui/LineEdit.h"
#include "Gui/LinkToKnobDialog.h"
#include "Gui/Menu.h"
#include "Gui/Menu.h"
#include "Gui/NodeCreationDialog.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/ScriptTextEdit.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/SpinBox.h"
#include "Gui/TabWidget.h"
#include "Gui/TimeLineGui.h"
#include "Gui/ViewerTab.h"

NATRON_NAMESPACE_ENTER


struct KnobGuiPrivate
{

    KnobGui* _publicInterface;

    // The internal knob
    KnobIWPtr knob;

    // When enabled, only the dimension indicated by singleDimension will be created for the knob
    bool singleDimensionEnabled;

    DimIdx singleDimension;

    // Whether we place the knob on a new line in a vertical layout
    bool isOnNewLine;

    // True when createGui has been called already
    bool widgetCreated;

    // Pointer to the widgets containing this knob
    KnobGuiContainerI*  container;

    // Right click menu
    QMenu* copyRightClickMenu;

    // A pointer to the previous knob on the row
    KnobIWPtr prevKnob;


    struct ViewWidgets
    {
        // A pointer to the widget containing all widgets for a given view
        QWidget* field;

        // the layout of the field widget
        QHBoxLayout* fieldLayout;

        // The view label
        KnobClickableLabel* viewLabel;

        // View name used in the right click menu
        QString viewLongName;

        // The implementation of the knob gui
        KnobGuiWidgetsPtr widgets;

    };

    typedef std::map<ViewIdx, ViewWidgets> PerViewWidgetsMap;

    PerViewWidgetsMap views;

    // A container the label
    QWidget* labelContainer;

    // If multi-view, an arrow to unfold views
    GroupBoxLabel* viewUnfoldArrow;

    //  each view row is contained in this widget
    QWidget* viewsContainer;
    QVBoxLayout* viewsContainerLayout;

    // Contains all knobs in the second column of the grid layout for this knob
    QWidget* mainContainer;
    QHBoxLayout* mainLayout;

    // If non null, this is the stretch object at the end of the layout
    QSpacerItem* endOfLineSpacer;

    // True if endOfLineSpacer is currently added to the layout, false otherwise
    bool spacerAdded;

    // If endOfLineSpacer is non null, specifies if we must add the spacer by default
    bool mustAddSpacerByDefault;

    // If this knob is not on a new line, this is a reference (weak) to the first knob
    // on the layout line.
    KnobGuiWPtr firstKnobOnLine;

    // If this knob is the first knob on the line, potentially it may have other knobs
    // on the same layout line. This is a list, in order, of the knobs that
    // are on the same layout line.
    std::list<KnobGuiWPtr> otherKnobsInMainLayout;

    // The label
    KnobClickableLabel* descriptionLabel;

    // A warning indicator
    Label* warningIndicator;
    std::map<KnobGui::KnobWarningEnum, QString> warningsMapping;

    // If the UI of the knob is handled by a custom interact, this is a pointer to it
    CustomParamInteract* customInteract;

    // True when the gui is no longer valid
    bool guiRemoved;

    // If the knob is a group as tab, this will be the tab widget
    TabGroup* tabGroup;

    // Used to concatenate updateGui() request
    int refreshGuiRequests;

    // Used to concatenate onHasModificationsChanged() request
    int refreshModifStateRequests;
    

    // used to concatenate onInternalKnobDimensionsVisibilityChanged
    std::list<ViewSetSpec> refreshDimensionVisibilityRequests;

    // True if this KnobGui is used in the viewer
    KnobGui::KnobLayoutTypeEnum layoutType;

    KnobGuiPrivate(KnobGui* publicInterface, const KnobIPtr& knob, KnobGui::KnobLayoutTypeEnum layoutType, KnobGuiContainerI* container);

    void refreshIsOnNewLineFlag();

    void createLabel(QWidget* parentWidget);

    void createViewWidgets(KnobGuiPrivate::PerViewWidgetsMap::iterator it);

    void addWidgetsToPreviousKnobLayout();

    void removeViewGui(KnobGuiPrivate::PerViewWidgetsMap::iterator it);
};

NATRON_NAMESPACE_EXIT

#endif // _Gui_KnobGuiPrivate_h_
