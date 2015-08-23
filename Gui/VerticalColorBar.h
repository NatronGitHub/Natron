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

#ifndef _Gui_VerticalColorBar_h_
#define _Gui_VerticalColorBar_h_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>


#include "Global/Macros.h"

#include <QtCore/QSize>
#include <QtGui/QWidget>
#include <QtGui/QColor>

class QPaintEvent;

class VerticalColorBar : public QWidget
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

private:
    QColor _color;
    
public:
    
    VerticalColorBar(QWidget* parent);
    
public Q_SLOTS:
    
    void setColor(const QColor& color);
    
private:
    
    virtual QSize sizeHint() const OVERRIDE FINAL;
    virtual void paintEvent(QPaintEvent* e) OVERRIDE FINAL;
};
#endif // _Gui_VerticalColorBar_h_
