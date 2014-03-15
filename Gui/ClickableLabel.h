//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef NATRON_GUI_CLICKABLELABEL_H_
#define NATRON_GUI_CLICKABLELABEL_H_

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QObject>
#include <QLabel> // in QtGui on Qt4, in QtWidgets on Qt5
CLANG_DIAG_ON(deprecated)

#include "Global/Macros.h"

class ClickableLabel : public QLabel
{
    Q_OBJECT
public:

    ClickableLabel(const QString &text, QWidget *parent): QLabel(text, parent), _toggled(false) {}

    virtual ~ClickableLabel() OVERRIDE {}

    void setClicked(bool b) {
        _toggled = b;
    }
    
    ///Updates the text as setText does but also keeps the current color info
    void setText_overload(const QString& str);

signals:
    void clicked(bool);

private:
    virtual void mousePressEvent(QMouseEvent *) OVERRIDE FINAL;
    
    virtual void changeEvent(QEvent* e) OVERRIDE FINAL;
    
private:
    bool _toggled;
};

#endif // NATRON_GUI_CLICKABLELABEL_H_
