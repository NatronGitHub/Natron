//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef NATRON_GUI_GROUPBOXLABEL_H_
#define NATRON_GUI_GROUPBOXLABEL_H_

#include <QtGui/QLabel>

#include "Global/Macros.h"

class GroupBoxLabel : public QLabel
{
    Q_OBJECT
public:

    GroupBoxLabel(QWidget *parent = 0);

    virtual ~GroupBoxLabel() OVERRIDE FINAL {}

    bool isChecked() const {
        return _checked;
    }

public slots:

    void setChecked(bool);

private:
    virtual void mousePressEvent(QMouseEvent *) OVERRIDE FINAL {
        emit checked(!_checked);
    }

signals:
    void checked(bool);

private:
    bool _checked;
};

#endif
