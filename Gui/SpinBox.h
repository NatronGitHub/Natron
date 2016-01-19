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

#ifndef NATRON_GUI_FEEDBACKSPINBOX_H
#define NATRON_GUI_FEEDBACKSPINBOX_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Gui/LineEdit.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;

struct SpinBoxPrivate;

class SpinBox
    : public LineEdit
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    Q_PROPERTY(int animation READ getAnimation WRITE setAnimation)
    Q_PROPERTY(bool dirty READ getDirty WRITE setDirty)
    
public:

    enum SpinBoxTypeEnum
    {
        eSpinBoxTypeInt = 0,
        eSpinBoxTypeDouble
    };

    explicit SpinBox(QWidget* parent = 0,
                     SpinBoxTypeEnum type = eSpinBoxTypeInt);

    virtual ~SpinBox() OVERRIDE;
    
    void setType(SpinBoxTypeEnum type);

    ///Set the digits after the decimal point.
    void decimals(int d);

    ///Min/Max of the spinbox
    void setMaximum(double t);
    void setMinimum(double b);

    double value()
    {
        return text().toDouble();
    }

    ///If OLD_SPINBOX_INCREMENT is defined in SpinBox.cpp, this function does nothing
    ///as the increments are relative to the position of the cursor in the spinbox.
    void setIncrement(double d);

    ///For the stylesheet: it controls the background color to represent the animated states of parameters
    void setAnimation(int i);
    int getAnimation() const
    {
        return animation;
    }

    ///Get a pointer to the right click menu, this can be used to add extra entries. @see KnobGuiTypes.cpp
    QMenu* getRightClickMenu();

    ///For the stylesheet: it controls whether the background should be paint in black or not.
    void setDirty(bool d);
    bool getDirty() const
    {
        return dirty;
    }

    void setUseLineColor(bool use, const QColor& color);
    
    /**
     * @brief Set an optional validator that will validate numbers instead of the regular double/int validator.
     * The spinbox takes ownership of the validator and will destroy it.
     **/
    void setValidator(SpinBoxValidator* validator);
    
    double getLastValidValueBeforeValidation() const;
    
protected:

    void increment(int delta, int shift);

    virtual void wheelEvent(QWheelEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;

    virtual void focusInEvent(QFocusEvent* e) OVERRIDE;
    virtual void focusOutEvent(QFocusEvent* e) OVERRIDE;
    virtual void paintEvent(QPaintEvent* e) OVERRIDE FINAL;

    bool validateText();
    
    bool validateInternal();
    
    bool validateWithCustomValidator(const QString& txt);

Q_SIGNALS:

    void valueChanged(double d);

public Q_SLOTS:

    void setValue(double d);

    void setValue(int d);

    /*Used internally when the user pressed enter*/
    void interpretReturn();

private:


    void setValue_internal(double d, bool reformat);

    void setText(const QString &str, int cursorPos);

    ///Used by the stylesheet , they are Q_PROPERTIES
    int animation; // 0 = no animation, 1 = interpolated, 2 = equals keyframe value
    bool dirty;
    bool altered;
    boost::scoped_ptr<SpinBoxPrivate> _imp;
};

class KnobSpinBox : public SpinBox
{
    const KnobGui* knob;
    int dimension;
public:
    
    KnobSpinBox(QWidget* parent,
                SpinBoxTypeEnum type,
                const KnobGui* knob,
                int dimension)
    : SpinBox(parent,type)
    , knob(knob)
    , dimension(dimension)
    {
        
    }
    
    virtual ~KnobSpinBox()
    {
        
    }
    
private:
    
    virtual void focusInEvent(QFocusEvent* e) OVERRIDE FINAL;
};

NATRON_NAMESPACE_EXIT;

#endif /* defined(NATRON_GUI_FEEDBACKSPINBOX_H_) */
