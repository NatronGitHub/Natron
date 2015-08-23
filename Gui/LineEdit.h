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

#ifndef NATRON_GUI_LINEEDIT_H_
#define NATRON_GUI_LINEEDIT_H_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QLineEdit>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Macros.h"

class QPaintEvent;
class QDropEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;

class LineEdit
    : public QLineEdit
{
CLANG_DIAG_OFF_36(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON_36(inconsistent-missing-override)

    Q_PROPERTY( int animation READ getAnimation WRITE setAnimation)
    Q_PROPERTY(bool dirty READ getDirty WRITE setDirty)
    Q_PROPERTY(bool altered READ getAltered WRITE setAltered)
    
public:
    explicit LineEdit(QWidget* parent = 0);
    virtual ~LineEdit() OVERRIDE;

    int getAnimation() const
    {
        return animation;
    }

    void setAnimation(int v);

    bool getDirty() const
    {
        return dirty;
    }

    void setDirty(bool b);
    
    void setAltered(bool b);
    bool getAltered() const
    {
        return altered;
    }

Q_SIGNALS:
    
    void textDropped();
    
    void textPasted();
    
public Q_SLOTS:

    void onEditingFinished();

protected:
    
    virtual void paintEvent(QPaintEvent* e) OVERRIDE;

private:
    
    virtual void dropEvent(QDropEvent* e) OVERRIDE FINAL;

    virtual void dragEnterEvent(QDragEnterEvent* e) OVERRIDE FINAL;

    virtual void dragMoveEvent(QDragMoveEvent* e) OVERRIDE FINAL;

    virtual void dragLeaveEvent(QDragLeaveEvent* e) OVERRIDE FINAL;
    
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE;
    
    int animation;
    bool dirty;
    bool altered;
};


#endif // ifndef NATRON_GUI_LINEEDIT_H_
