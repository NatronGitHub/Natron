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



#include "Global/Macros.h"
#include <boost/scoped_ptr.hpp>
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

using Natron::ScaleTypeEnum;

struct ScaleSliderQWidgetPrivate;
class QFont;
class ScaleSliderQWidget
    : public QWidget
{
    Q_OBJECT

public:

    ScaleSliderQWidget(double bottom, // the minimum value
                       double top, // the maximum value
                       double initialPos, // the initial value
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

    
signals:
    void editingFinished();
    void positionChanged(double);

public slots:

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
    virtual void paintEvent(QPaintEvent* e) OVERRIDE FINAL;
    virtual void resizeEvent(QResizeEvent* e) OVERRIDE FINAL;

    boost::scoped_ptr<ScaleSliderQWidgetPrivate> _imp;
    
};

#endif // SCALESLIDERQWIDGET_H
