/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Global/Macros.h"

#include <vector>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QFrame>
#include <QPixmap>
#include <QtGui/QKeySequence>
#include <QtGui/QIcon>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/StyledKnobWidgetBase.h"


#define DROP_DOWN_ICON_SIZE 6

NATRON_NAMESPACE_ENTER


class ComboBox
    : public QFrame
    , public StyledKnobWidgetBase
{
    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON
    DEFINE_KNOB_GUI_STYLE_PROPERTIES


protected:

    bool ignoreWheelEvent;

public:

    explicit ComboBox(QWidget* parent = 0);

    virtual ~ComboBox() OVERRIDE;

    /**
     * @brief Enable the combobox to support cascading hierarchy of submenus.
     * When enabled the menu does not support some functions of the API
     **/
    void setCascading(bool cascading);
    bool isCascading() const;

    /**
     * @brief Insert a new item BEFORE the specified index. Does not work for cascading menu.
     **/
    void insertItem( int index, const QString &item, QIcon icon = QIcon(), QKeySequence = QKeySequence(), const QString & toolTip = QString() );

    /**
     * @brief Append a new item to the menu
     **/
    void addItem( const QString &item, QIcon icon = QIcon(), QKeySequence = QKeySequence(), const QString & toolTip = QString() );

    /**
     * @brief Add the "New" special action which can be selected by the user to append a new entry.
     * When selected by the user, the itemNewSelected() signal will be emitted and the selected index will
     * return to the original selection.
     * This is not supported for cascading menus
     **/
    void addItemNew();

   /**
    * @brief Add a new action to the combobox menu
    **/
    void addAction(QAction* action);

    /**
    * @brief Set whether the combobox should draw the outter shape. Enabled by default.
    **/
    void setDrawShapeEnabled(bool enabled);

    /**
    * @brief Append a separator in the combobox menu
    **/
    void addSeparator();

    /**
    * @brief Insert a separator in the menu after the specified index
    **/
    void insertSeparator(int index);

    /**
    * @brief Returns the label of the item at the given index in the menu.
    **/
    QString itemText(int index) const;

    /**
    * @brief Returns the number of items in the menu
    **/
    int count() const;

    /**
    * @brief Returns the index of the item matching the given label
    **/
    int itemIndex(const QString& label) const;

    /**
    * @brief Remove an item from the combobox menu by matching its label
    **/
    void removeItem(const QString & label);

   /**
    * @brief Enable/Disable selection of the item at the given index
    **/
    void setItemEnabled(int index, bool enabled);

    /**
    * @brief Set the label associated to the menu entry at the given index
    **/
    void setItemText(int index, const QString & label);

    /**
    * @brief Set the shortcut decoration to add to the menu entry at the given index
    **/
    void setItemShortcut(int index, const QKeySequence & sequence);

    /**
    * @brief Set the icon decoration to add to the menu entry at the given index
    **/
    void setItemIcon(int index, const QIcon & icon);

    /**
    * @brief Set the maximum width of the combobox given a string.
    **/
    void setMaximumWidthFromText(const QString & str);

    /**
     * @brief Enable multiple selection. The user can select 0, 1 or more items in the menu.
     * If disabled (default), only 1 item is always selected.
     **/
    void setMultiSelectionEnabled(bool enabled);
    bool isMultiSelectionEnabled() const;

    /**
     * @brief Can be called only for non multi-selection combobox. Returns the selected index, or -1
     * if the menu is empty for instance.
     **/
    int activeIndex() const;

    /**
    * @brief Returns the selected indices.
    **/
    const std::vector<int> &getSelectedIndices() const;

    /**
    * @brief Clears the menu
    **/
    void clear();

    /**
    * @brief Can be called only for non multi-selection combobox. Returns the label of the currently selected index.
    **/
    QString getCurrentIndexText() const;

    /**
    * @brief Get the combobox enabledness
    **/
    bool getEnabled_natron() const;

    // Reimplemented from QWidget
    virtual QSize minimumSizeHint() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    /**
     * @brief Set the selected indices. Only multi-selection combobox can handle multiple indices.
     * @param emitSignal  emits the signal void currentIndexChanged
     **/
    void setSelectedIndices(const std::vector<int>& indices, bool emitSignal = true);

    /* @brief Set an item from label. If no match were found, this will set the label
    * to be displayed anyway, and make the current index returned to be -1.  Only multi-selection combobox can handle multiple indices.
    * @param emitSignal  emits the signal void currentIndexChanged
    **/
    void setSelectedIndicesFromLabel(const std::vector<QString>& label, bool emitSignal = true);


public Q_SLOTS:

    /**
     * @brief Set the selected index
     * @param emitSignal  emits the signal void currentIndexChanged
     **/
    void setCurrentIndex(int index, bool emitSignal = true);

    /**
     * @brief Set an item from label. If no match were found, this will set the label
     * to be displayed anyway, and make the current index returned to be -1
     * @param emitSignal  emits the signal void currentIndexChanged
    **/
    void setCurrentIndexFromLabel(const QString & label, bool emitSignal = true);


    /**
     * @brief Set the combobox enabledness
     **/
    void setEnabled_natron(bool enabled);

Q_SIGNALS:

    // Emitted if the combobox is not multi-choice enabled
    void currentIndexChanged(int index);
    void currentIndexChanged(QString);

    // Emitted whenever the combobox selection is changed
    void selectionChanged();

    // If the "New" item is present and was triggered by the user, this signal will be emitted
    void itemNewSelected();

    void minimumSizeChanged(QSize);

protected:

    virtual void refreshStylesheet() OVERRIDE;

    virtual void paintEvent(QPaintEvent* e) OVERRIDE;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE;
    virtual void wheelEvent(QWheelEvent *e) OVERRIDE;

private:


    virtual QSize sizeHint() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void changeEvent(QEvent* e) OVERRIDE FINAL;
    virtual void resizeEvent(QResizeEvent* e) OVERRIDE FINAL;

    
    struct Implementation;
    boost::scoped_ptr<Implementation> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // ifndef NATRON_GUI_COMBOBOX_H
