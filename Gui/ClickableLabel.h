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
    Q_PROPERTY(bool dirty READ getDirty WRITE setDirty)
    Q_PROPERTY( int animation READ getAnimation WRITE setAnimation)
    Q_PROPERTY( bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY( bool sunkenStyle READ isSunken WRITE setSunken)
public:

    ClickableLabel(const QString &text, QWidget *parent):
    QLabel(text, parent),
    _toggled(false) ,
    dirty(false) ,
    readOnly(false),
    animation(0),
    sunkenStyle(false)
    {}

    virtual ~ClickableLabel() OVERRIDE {}

    void setClicked(bool b) {
        _toggled = b;
    }
    
    void setDirty(bool b);
    
    bool getDirty() const { return dirty; }
    
    ///Updates the text as setText does but also keeps the current color info
    void setText_overload(const QString& str);
    
    int getAnimation() const { return animation; }

    void setAnimation(int i);

    bool isReadOnly() const { return readOnly; }
    
    void setReadOnly(bool readOnly) ;
    
    bool isSunken() const { return sunkenStyle; }
    
    void setSunken(bool s);

signals:
    void clicked(bool);

private:
    virtual void mousePressEvent(QMouseEvent *) OVERRIDE FINAL;
    
    virtual void changeEvent(QEvent* e) OVERRIDE FINAL;
    
private:
    bool _toggled;
    bool dirty;
    bool readOnly;
    int animation;
    bool sunkenStyle;
};

#endif // NATRON_GUI_CLICKABLELABEL_H_
