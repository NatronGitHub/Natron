//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef NATRON_GUI_ANIMATEDCHECKBOX_H_
#define NATRON_GUI_ANIMATEDCHECKBOX_H_

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QCheckBox>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Macros.h"

class AnimatedCheckBox
    : public QCheckBox
{
    Q_OBJECT Q_PROPERTY(int animation READ getAnimation WRITE setAnimation)
    Q_PROPERTY(bool readOnly READ getReadOnly WRITE setReadOnly)
    Q_PROPERTY(bool dirty READ getDirty WRITE setDirty)

    int animation;
    bool readOnly;
    bool dirty;

public:

    AnimatedCheckBox(QWidget *parent = NULL)
        : QCheckBox(parent), animation(0), readOnly(false), dirty(false)
    {
        setFocusPolicy(Qt::StrongFocus);
    }

    virtual ~AnimatedCheckBox() OVERRIDE
    {
    }

    void setAnimation(int i);

    int getAnimation() const
    {
        return animation;
    }

    void setReadOnly(bool readOnly);

    bool getReadOnly() const
    {
        return readOnly;
    }

    bool getDirty() const
    {
        return dirty;
    }

    void setDirty(bool b);

    virtual QSize minimumSizeHint() const OVERRIDE FINAL
    {
        return QSize(25,25);
    }

    virtual QSize sizeHint() const OVERRIDE FINAL
    {
        return QSize(25,25);
    }

private:

    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
};

#endif // NATRON_GUI_ANIMATEDCHECKBOX_H_
