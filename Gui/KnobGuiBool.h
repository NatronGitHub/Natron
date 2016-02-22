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

#ifndef Gui_KnobGuiBool_h
#define Gui_KnobGuiBool_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector> // KnobGuiInt
#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QStyledItemDelegate>
#include <QTextEdit>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/Singleton.h"
#include "Engine/Knob.h"
#include "Engine/ImageComponents.h"
#include "Engine/EngineFwd.h"

#include "Gui/AnimatedCheckBox.h"
#include "Gui/CurveSelection.h"
#include "Gui/KnobGui.h"
#include "Gui/Label.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;

//================================

class Bool_CheckBox: public AnimatedCheckBox
{
public:
    Bool_CheckBox(KnobGui* knob, int dimension, QWidget* parent = 0);
    
    virtual ~Bool_CheckBox();
    
    void setCustomColor(const QColor& color, bool useCustom)
    {
        useCustomColor = useCustom;
        customColor = color;
    }
    
    virtual void getBackgroundColor(double *r,double *g,double *b) const OVERRIDE FINAL;
    
private:
    
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void keyReleaseEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void dragEnterEvent(QDragEnterEvent* e) OVERRIDE FINAL;
    virtual void dragMoveEvent(QDragMoveEvent* e) OVERRIDE FINAL;
    virtual void dropEvent(QDropEvent* e) OVERRIDE FINAL;
    virtual void focusInEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual void focusOutEvent(QFocusEvent* e) OVERRIDE FINAL;

private:
    bool useCustomColor;
    QColor customColor;
    boost::scoped_ptr<KnobWidgetDnD> _dnd;
};

class KnobGuiBool
    : public KnobGui
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    static KnobGui * BuildKnobGui(KnobPtr knob,
                                  DockablePanel *container)
    {
        return new KnobGuiBool(knob, container);
    }

    KnobGuiBool(KnobPtr knob,
                 DockablePanel *container);

    virtual ~KnobGuiBool() OVERRIDE;
    
    virtual void removeSpecificGui() OVERRIDE FINAL;

    virtual KnobPtr getKnob() const OVERRIDE FINAL;

public Q_SLOTS:

    void onCheckBoxStateChanged(bool);
    void onLabelClicked(bool);
private:

    
    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(int dimension,AnimationLevelEnum level) OVERRIDE FINAL;
    virtual void reflectExpressionState(int dimension,bool hasExpr) OVERRIDE FINAL;
    virtual void updateToolTip() OVERRIDE FINAL;
    virtual void onLabelChangedInternal() OVERRIDE FINAL;
private:

    Bool_CheckBox *_checkBox;
    boost::weak_ptr<KnobBool> _knob;
};

NATRON_NAMESPACE_EXIT;

#endif // Gui_KnobGuiBool_h
