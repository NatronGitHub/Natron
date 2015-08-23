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

#ifndef SCALESLIDERQWIDGET_H
#define SCALESLIDERQWIDGET_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

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
CLANG_DIAG_OFF_36(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON_36(inconsistent-missing-override)

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
