/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef NATRON_GUI_COMBOBOX_H
#define NATRON_GUI_COMBOBOX_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <vector>
#include "Global/Macros.h"
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QFrame>
#include <QtGui/QKeySequence>
#include <QtGui/QIcon>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/GuiFwd.h"


#define DROP_DOWN_ICON_SIZE 6



struct ComboBoxMenuNode
{
    MenuWithToolTips* isMenu;
    QAction* isLeaf;
    QString text;
    std::vector<boost::shared_ptr<ComboBoxMenuNode> > children;
    ComboBoxMenuNode* parent;
    
    ComboBoxMenuNode() : isMenu(0), isLeaf(0), text(), children(), parent(0) {}
};

class ComboBox
    : public QFrame
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON
    
private:
    bool _readOnly;
    bool _enabled;
    int _animation;
    bool _clicked;
    bool _dirty;
    bool _altered;
    bool _cascading;
    int _cascadingIndex;
    
    int _currentIndex;
    QString _currentText;
    std::vector<int> _separators;
    boost::shared_ptr<ComboBoxMenuNode> _rootNode;
    
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
    
    void setCascading(bool cascading)
    {
        _cascading = cascading;
    }

    /*Insert a new item BEFORE the specified index.*/
    void insertItem( int index,const QString &item,QIcon icon = QIcon(),QKeySequence = QKeySequence(),const QString & toolTip = QString() );
    
    void addAction(QAction* action);
    
private:
    
    void addActionPrivate(QAction* action);
    
public:
    
    
    void addItem( const QString &item,QIcon icon = QIcon(),QKeySequence = QKeySequence(),const QString & toolTip = QString() );
    
    void addItemNew();

    /*Appends a separator to the comboBox.*/
    void addSeparator();

    /*Insert a separator after the index specified.*/
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
    
    bool getEnabled_natron() const;
    
    void setAltered(bool b);
    bool getAltered() const;

    virtual QSize minimumSizeHint() const OVERRIDE FINAL WARN_UNUSED_RETURN;

public Q_SLOTS:

    ///Changes the current index AND emits the signal void currentIndexChanged(int)
    void setCurrentIndex(int index);

    ///Same as setCurrentIndex but does not Q_EMIT any signal.
    void setCurrentIndex_no_emit(int index);

    ///Changes the text displayed by the combobox. It doesn't have to match the text
    ///of an entry in the combobox.
    void setCurrentText(const QString & text);

    ///Same as setCurrentText but no signal will be emitted.
    void setCurrentText_no_emit(const QString & text);


    void setEnabled_natron(bool enabled);

Q_SIGNALS:

    void currentIndexChanged(int index);

    void currentIndexChanged(QString);
    
    void itemNewSelected();
    
    void minimumSizeChanged(QSize);

protected:

    virtual void paintEvent(QPaintEvent* e) OVERRIDE;
    
private:
    
    
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void wheelEvent(QWheelEvent *e) OVERRIDE FINAL;
    virtual QSize sizeHint() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void changeEvent(QEvent* e) OVERRIDE FINAL;
    virtual void resizeEvent(QResizeEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    
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

#endif // ifndef NATRON_GUI_COMBOBOX_H
