//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef NATRON_GUI_CLICKABLELABEL_H_
#define NATRON_GUI_CLICKABLELABEL_H_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Macros.h"

#include "Label.h"

namespace Natron {
 
class ClickableLabel
    : public Label
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

    Q_PROPERTY(bool dirty READ getDirty WRITE setDirty)
    Q_PROPERTY( int animation READ getAnimation WRITE setAnimation)
    Q_PROPERTY( bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY( bool sunkenStyle READ isSunken WRITE setSunken)

public:

    ClickableLabel(const QString &text,
                   QWidget *parent);

    virtual ~ClickableLabel() OVERRIDE
    {
    }

    void setClicked(bool b)
    {
        _toggled = b;
    }

    void setDirty(bool b);

    bool getDirty() const
    {
        return dirty;
    }

    ///Updates the text as setText does but also keeps the current color info
    void setText_overload(const QString & str);

    int getAnimation() const
    {
        return animation;
    }

    void setAnimation(int i);

    bool isReadOnly() const
    {
        return readOnly;
    }

    void setReadOnly(bool readOnly);

    bool isSunken() const
    {
        return sunkenStyle;
    }

    void setSunken(bool s);

Q_SIGNALS:
    void clicked(bool);

private:
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void changeEvent(QEvent* e) OVERRIDE FINAL;

private:
    bool _toggled;
    bool dirty;
    bool readOnly;
    int animation;
    bool sunkenStyle;
};

} // namespace Natron
#endif // NATRON_GUI_CLICKABLELABEL_H_
