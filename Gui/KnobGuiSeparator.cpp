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

#include "KnobGuiSeparator.h"

#include <cfloat>
#include <algorithm> // min, max
#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QGridLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QColorDialog>
#include <QToolTip>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QApplication>
#include <QScrollArea>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QtCore/QDebug>
#include <QFontComboBox>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Lut.h"
#include "Engine/KnobUndoCommand.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"

#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveGui.h"
#include "Gui/DockablePanel.h"
#include "Gui/GroupBoxLabel.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"
#include "Gui/Label.h"
#include "Gui/KnobGui.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/TabGroup.h"

#include <ofxNatron.h>


NATRON_NAMESPACE_ENTER
using std::make_pair;

//=============================SEPARATOR_KNOB_GUI===================================

KnobGuiSeparator::KnobGuiSeparator(const KnobGuiPtr& knob, ViewIdx view)
    : KnobGuiWidgets(knob, view)
    , _line(0)
{
    _knob = toKnobSeparator(knob->getKnob());
}

void
KnobGuiSeparator::createWidget(QHBoxLayout* layout)
{
    layout->parentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    _line = new QFrame( layout->parentWidget() );
    _line->setFixedHeight( TO_DPIY(2) );
    _line->setGeometry( 0, 0, TO_DPIX(300), TO_DPIY(2) );
    _line->setFrameShape(QFrame::HLine);
    _line->setFrameShadow(QFrame::Sunken);

    //Without that the frame won't showup
    _line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(_line);
}

KnobGuiSeparator::~KnobGuiSeparator()
{
}

bool
KnobGuiSeparator::isLabelOnSameColumn() const
{
    return true;
}

bool
KnobGuiSeparator::isLabelBold() const
{
    return true;
}


void
KnobGuiSeparator::setWidgetsVisible(bool visible)
{
    _line->setVisible(visible);
}


NATRON_NAMESPACE_EXIT
