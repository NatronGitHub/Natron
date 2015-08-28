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

#ifndef SCALESLIDERQWIDGET_H
#define SCALESLIDERQWIDGET_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

using Natron::ScaleTypeEnum;

struct ScaleSliderQWidgetPrivate;
class QColor;
class QFont;
class Gui;

class ScaleSliderQWidget
    : public QWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    
    enum DataTypeEnum
    {
        eDataTypeInt,
        eDataTypeDouble
    };
    
    ScaleSliderQWidget(double bottom, // the minimum value
                       double top, // the maximum value
                       double initialPos, // the initial value
                       DataTypeEnum dataType,
                       Gui* gui,
                       Natron::ScaleTypeEnum type = Natron::eScaleTypeLinear, // the type of scale
                       QWidget* parent = 0);
    

    virtual ~ScaleSliderQWidget() OVERRIDE;

    void setMinimumAndMaximum(double min,double max);


    Natron::ScaleTypeEnum type() const;

    double minimum() const;

    double maximum() const;

    double getPosition() const;

    bool isReadOnly() const;
    
    void setReadOnly(bool ro);

    // the size of a pixel increment (used to round the value)
    double increment();
    
    void setAltered(bool b);
    bool getAltered() const;
    
    void setUseLineColor(bool use, const QColor& color);
    
Q_SIGNALS:
    void editingFinished(bool hasMovedOnce);
    void positionChanged(double);

public Q_SLOTS:

    void seekScalePosition(double v);

private:

    void seekInternal(double v);


    void centerOn(double left,double right);

    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void keyReleaseEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual QSize sizeHint() const OVERRIDE FINAL;
    virtual QSize minimumSizeHint() const OVERRIDE FINAL;
    virtual void paintEvent(QPaintEvent* e) OVERRIDE FINAL;
    virtual void resizeEvent(QResizeEvent* e) OVERRIDE FINAL;

    boost::scoped_ptr<ScaleSliderQWidgetPrivate> _imp;
    
};

#endif // SCALESLIDERQWIDGET_H
