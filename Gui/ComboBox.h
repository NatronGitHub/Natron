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

#ifndef NATRON_GUI_COMBOBOX_H_
#define NATRON_GUI_COMBOBOX_H_

#include <vector>
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QFrame>
#include <QtGui/QKeySequence>
#include <QtGui/QIcon>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Macros.h"

class QHBoxLayout;
class QMouseEvent;
class QAction;

class MenuWithToolTips;

class ComboBox
    : public QFrame
{
    
    
private:
    
    Q_OBJECT

    bool _readOnly;
    bool _enabled;
    int _animation;
    bool _clicked;
    bool _dirty;
    
    int _currentIndex;
    QString _currentText;
    std::vector<int> _separators;
    std::vector<QAction*> _actions;
    MenuWithToolTips* _menu;
    
    mutable QSize _sh; ///size hint
    mutable QSize _msh; ///minmum size hint
    mutable QSizePolicy _sizePolicy;
    mutable bool _validHints;
    unsigned short _align;

public:

    explicit ComboBox(QWidget* parent = 0);

    virtual ~ComboBox() OVERRIDE
    {
    }

    /*Insert a new item BEFORE the specified index.*/
    void insertItem( int index,const QString &item,QIcon icon = QIcon(),QKeySequence = QKeySequence(),const QString & toolTip = QString() );
    
    void addAction(QAction* action);
    
    void addItem( const QString &item,QIcon icon = QIcon(),QKeySequence = QKeySequence(),const QString & toolTip = QString() );

    /*Appends a separator to the comboBox.*/
    void addSeparator();

    /*Insert a separator BEFORE the item at the index specified.*/
    void insertSeparator(int index);

    QString itemText(int index) const;

    int count() const;

    int itemIndex(const QString & str) const;

    void removeItem(const QString & item);

    void disableItem(int index);

    void enableItem(int index);

    void setItemText(int index,const QString & item);

    void setItemShortcut(int index,const QKeySequence & sequence);

    void setItemIcon(int index,const QIcon & icon);

    void setMaximumWidthFromText(const QString & str);

    int activeIndex() const;

    void clear();

    QString getCurrentIndexText() const;


    void setDirty(bool b);
    int getAnimation() const;
    void setAnimation(int i);
    void setReadOnly(bool readOnly);

public slots:

    ///Changes the current index AND emits the signal void currentIndexChanged(int)
    void setCurrentIndex(int index);

    ///Same as setCurrentIndex but does not emit any signal.
    void setCurrentIndex_no_emit(int index);

    ///Changes the text displayed by the combobox. It doesn't have to match the text
    ///of an entry in the combobox.
    void setCurrentText(const QString & text);

    ///Same as setCurrentText but no signal will be emitted.
    void setCurrentText_no_emit(const QString & text);


    void setEnabled_natron(bool enabled);

signals:

    void currentIndexChanged(int index);

    void currentIndexChanged(QString);

private:

    virtual void paintEvent(QPaintEvent* e) OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void wheelEvent(QWheelEvent *e) OVERRIDE FINAL;
    virtual QSize sizeHint() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual QSize minimumSizeHint() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void changeEvent(QEvent* e) OVERRIDE FINAL;
    virtual void resizeEvent(QResizeEvent* e) OVERRIDE FINAL;
    
    void growMaximumWidthFromText(const QString & str);
    void createMenu();

    ///changes the current index and returns true if the index really changed, false if it is the same.
    bool setCurrentIndex_internal(int index);

    ///changes the combobox text and returns an entry index if a matching one with the same name was found, -1 otherwise.
    int setCurrentText_internal(const QString & text);
    
    QSize sizeForWidth(int w) const;
    
    QRectF layoutRect() const;
    
    void updateLabel();
};

#endif // ifndef NATRON_GUI_COMBOBOX_H_
