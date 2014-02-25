//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef NATRON_GUI_ANIMATEDCHECKBOX_H_
#define NATRON_GUI_ANIMATEDCHECKBOX_H_

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QCheckBox>
CLANG_DIAG_ON(deprecated)

#include "Global/Macros.h"

class AnimatedCheckBox : public QCheckBox
{

    Q_OBJECT
    Q_PROPERTY(int animation READ getAnimation WRITE setAnimation)
    Q_PROPERTY(bool readOnly READ getReadOnly WRITE setReadOnly)

    int animation;
    
    bool readOnly;
public:
    AnimatedCheckBox(QWidget *parent = NULL): QCheckBox(parent), animation(0) , readOnly(false) {}

    virtual ~AnimatedCheckBox() OVERRIDE {}

    void setAnimation(int i) ;

    int getAnimation() const {
        return animation;
    }
    
    void setReadOnly(bool readOnly);
    
    bool getReadOnly() const { return readOnly; }
    
private:
    
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    
};

#endif // NATRON_GUI_ANIMATEDCHECKBOX_H_
