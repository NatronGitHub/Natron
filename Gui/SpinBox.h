//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_GUI_FEEDBACKSPINBOX_H_
#define NATRON_GUI_FEEDBACKSPINBOX_H_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif
#include "Gui/LineEdit.h"

class QColor;
class QMenu;

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
    
private:

    void increment(int delta, int shift);

    virtual void wheelEvent(QWheelEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;

    //virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void focusInEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual void focusOutEvent(QFocusEvent* e) OVERRIDE FINAL;
    virtual void paintEvent(QPaintEvent* e) OVERRIDE FINAL;

    bool validateText();

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

#endif /* defined(NATRON_GUI_FEEDBACKSPINBOX_H_) */
