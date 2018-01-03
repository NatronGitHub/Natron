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

#ifndef Gui_KnobGuiLayer_h
#define Gui_KnobGuiLayer_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Gui/GuiFwd.h"
#include "Gui/KnobGuiWidgets.h"

NATRON_NAMESPACE_ENTER

struct KnobGuiTablePrivate;
class KnobGuiTable
    : public QObject, public KnobGuiWidgets
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    KnobGuiTable(const KnobGuiPtr& knob, ViewIdx view);

    virtual ~KnobGuiTable() OVERRIDE;

    int rowCount() const;

public Q_SLOTS:


    void onAddButtonClicked();

    void onRemoveButtonClicked();

    void onEditButtonClicked();

    void onItemDataChanged(const TableItemPtr& item, int col, int role);

    void onItemAboutToDrop();

    void onItemDropped();

    void onItemDoubleClicked(const TableItemPtr& item);

protected:

    virtual void reflectMultipleSelection(bool /*dirty*/) OVERRIDE
    {
    }
    virtual void reflectSelectionState(bool /*selected*/) OVERRIDE
    {

    }
    virtual void createWidget(QHBoxLayout *layout) OVERRIDE;
    virtual void setWidgetsVisible(bool visible) OVERRIDE;
    virtual void setEnabled(const std::vector<bool>& perDimEnabled) OVERRIDE;
    virtual void updateGUI() OVERRIDE;
    virtual void reflectAnimationLevel(DimIdx /*dimension*/,
                                       AnimationLevelEnum /*level*/) OVERRIDE
    {
    }

    virtual void updateToolTip() OVERRIDE;
    virtual bool addNewUserEntry(QStringList& row) = 0;

    // row has been set-up with old value
    virtual bool editUserEntry(QStringList& row) = 0;
    virtual void entryRemoved(const QStringList& /*row*/)  {}

    virtual void tableChanged(int /*row*/,
                              int /*col*/,
                              std::string* /*newEncodedValue*/)
    {
    }

private:

    virtual bool shouldAddStretch() const OVERRIDE FINAL { return false; }

    boost::scoped_ptr<KnobGuiTablePrivate> _imp;
};

class KnobGuiLayers
    : public KnobGuiTable
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    static KnobGuiWidgets * BuildKnobGui(const KnobGuiPtr& knob, ViewIdx view)
    {
        return new KnobGuiLayers(knob, view);
    }

    KnobGuiLayers(const KnobGuiPtr& knob, ViewIdx view);

    virtual ~KnobGuiLayers() OVERRIDE;

private:


    virtual bool addNewUserEntry(QStringList& row) OVERRIDE FINAL WARN_UNUSED_RETURN;

    // row has been set-up with old value
    virtual bool editUserEntry(QStringList& row) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void entryRemoved(const QStringList& /*row*/)  OVERRIDE {}

    virtual void tableChanged(int row, int col, std::string* newEncodedValue) OVERRIDE FINAL;
    boost::weak_ptr<KnobLayers> _knob;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_KnobGuiLayer_h
