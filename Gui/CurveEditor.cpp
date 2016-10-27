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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "CurveEditor.h"

#include <utility>
#include <stdexcept>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include <QApplication>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QSplitter>
#include <QHeaderView>
#include <QUndoStack> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QAction>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QMouseEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON

#include "Engine/Bezier.h"
#include "Engine/Knob.h"
#include "Engine/Curve.h"
#include "Engine/Node.h"
#include "Engine/KnobFile.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/EffectInstance.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewIdx.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/CurveGui.h"
#include "Gui/NodeGui.h"
#include "Gui/KnobGui.h"
#include "Gui/LineEdit.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"
#include "Gui/Gui.h"
#include "Gui/DockablePanel.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/Label.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/TabWidget.h"

#include "Serialization/WorkspaceSerialization.h"


#if 0
CurveEditor::CurveEditor(const std::string& scriptName,
                         Gui* gui,
                         const TimeLinePtr& timeline,
                         QWidget *parent)
    : QWidget(parent)
    , CurveSelection()
    , PanelWidget(scriptName, this, gui)
    , _imp( new CurveEditorPrivate() )
{
    setObjectName( QString::fromUtf8("CurveEditor") );
    _imp->undoAction = _imp->undoStack->createUndoAction( this, tr("&Undo") );
    _imp->undoAction->setShortcuts(QKeySequence::Undo);
    _imp->redoAction = _imp->undoStack->createRedoAction( this, tr("&Redo") );
    _imp->redoAction->setShortcuts(QKeySequence::Redo);

    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    _imp->mainLayout->setSpacing(0);

    _imp->splitter = new QSplitter(Qt::Horizontal, this);
    _imp->splitter->setObjectName( QString::fromUtf8("CurveEditorSplitter") );

    _imp->curveWidget = new CurveWidget(gui, this, timeline, _imp->splitter);
    _imp->curveWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _imp->leftPaneContainer = new QWidget(_imp->splitter);
    _imp->leftPaneLayout = new QVBoxLayout(_imp->leftPaneContainer);
    _imp->leftPaneLayout->setContentsMargins(0, 0, 0, 0);
    _imp->filterContainer = new QWidget(_imp->leftPaneContainer);

    _imp->filterLayout = new QHBoxLayout(_imp->filterContainer);
    _imp->filterLayout->setContentsMargins(0, 0, 0, 0);

    QString filterTt = tr("Show in the curve editor only nodes containing the following filter");
    _imp->filterLabel = new Label(tr("Filter:"), _imp->filterContainer);
    _imp->filterLabel->setToolTip(filterTt);
    _imp->filterEdit = new LineEdit(_imp->filterContainer);
    _imp->filterEdit->setToolTip(filterTt);
    QObject::connect( _imp->filterEdit, SIGNAL(textChanged(QString)), this, SLOT(onFilterTextChanged(QString)) );
    _imp->filterLayout->addWidget(_imp->filterLabel);
    _imp->filterLayout->addWidget(_imp->filterEdit);

    _imp->leftPaneLayout->addWidget(_imp->filterContainer);

    _imp->tree = new CurveEditorTreeWidget(gui, _imp->leftPaneContainer);
    _imp->tree->setObjectName( QString::fromUtf8("tree") );
    _imp->tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    _imp->tree->setColumnCount(1);
    _imp->tree->header()->close();
    _imp->tree->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    _imp->tree->setAttribute(Qt::WA_MacShowFocusRect, 0);

    _imp->leftPaneLayout->addWidget(_imp->tree);


    _imp->splitter->addWidget(_imp->leftPaneContainer);
    _imp->splitter->addWidget(_imp->curveWidget);


    _imp->mainLayout->addWidget(_imp->splitter);

    _imp->expressionContainer = new QWidget(this);
    _imp->expressionLayout = new QHBoxLayout(_imp->expressionContainer);
    _imp->expressionLayout->setContentsMargins(0, 0, 0, 0);

    _imp->knobLabel = new Label(_imp->expressionContainer);
    _imp->knobLabel->setAltered(true);
    _imp->knobLabel->setText( tr("No curve selected") );
    _imp->knobLineEdit = new LineEdit(_imp->expressionContainer);
    QObject::connect( _imp->knobLineEdit, SIGNAL(editingFinished()), this, SLOT(onExprLineEditFinished()) );
    _imp->resultLabel = new Label(_imp->expressionContainer);
    _imp->resultLabel->setAltered(true);
    _imp->resultLabel->setText( QString::fromUtf8("= ") );
    _imp->knobLineEdit->setReadOnly_NoFocusRect(true);

    _imp->expressionLayout->addWidget(_imp->knobLabel);
    _imp->expressionLayout->addWidget(_imp->knobLineEdit);
    _imp->expressionLayout->addWidget(_imp->resultLabel);

    _imp->mainLayout->addWidget(_imp->expressionContainer);

    QObject::connect( _imp->tree, SIGNAL(itemSelectionChanged()),
                      this, SLOT(onItemSelectionChanged()) );
    QObject::connect( _imp->tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
                      this, SLOT(onItemDoubleClicked(QTreeWidgetItem*,int)) );
}


void
CurveEditor::refreshCurrentExpression()
{
    KnobCurveGuiPtr curve = _imp->selectedKnobCurve.lock();

    if (!curve) {
        return;
    }
    KnobIPtr knob = curve->getInternalKnob();
    std::string expr = knob->getExpression( curve->getDimension() );
    double v = knob->getValueAtWithExpression( getGui()->getApp()->getTimeLine()->currentFrame(), ViewIdx(0), curve->getDimension() );
    _imp->knobLineEdit->setText( QString::fromUtf8( expr.c_str() ) );
    _imp->resultLabel->setText( QString::fromUtf8("= ") + QString::number(v) );
}

void
CurveEditor::onExprLineEditFinished()
{
    setSelectedCurveExpression( _imp->knobLineEdit->text() );
}
#endif
