//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef NATRON_GUI_ANIMATEDCHECKBOX_H_
#define NATRON_GUI_ANIMATEDCHECKBOX_H_

#include <QCheckBox>

class AnimatedCheckBox : public QCheckBox
{

    Q_OBJECT
    Q_PROPERTY(int animation READ getAnimation WRITE setAnimation)

    int animation;
public:
    AnimatedCheckBox(QWidget *parent = NULL): QCheckBox(parent), animation(0) {}

    virtual ~AnimatedCheckBox() {}

    void setAnimation(int i) ;

    int getAnimation() const {
        return animation;
    }
};

#endif // NATRON_GUI_ANIMATEDCHECKBOX_H_
