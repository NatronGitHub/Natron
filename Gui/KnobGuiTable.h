/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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
#include "Gui/KnobGui.h"

NATRON_NAMESPACE_ENTER

struct KnobGuiTablePrivate;
class KnobGuiTable
    : public KnobGui
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    KnobGuiTable(KnobIPtr knob,
                 KnobGuiContainerI *container);

    virtual ~KnobGuiTable() OVERRIDE;

    virtual void removeSpecificGui() OVERRIDE;

    int rowCount() const;

public Q_SLOTS:


    void onAddButtonClicked();

    void onRemoveButtonClicked();

    void onEditButtonClicked();

    void onItemDataChanged(TableItem* item);

    void onItemAboutToDrop();

    void onItemDropped();

    void onItemDoubleClicked(TableItem* item);

protected:

    virtual void setDirty(bool /*dirty*/) OVERRIDE
    {
    }

    virtual void createWidget(QHBoxLayout *layout) OVERRIDE;
    virtual void _hide() OVERRIDE;
    virtual void _show() OVERRIDE;
    virtual void setEnabled() OVERRIDE;
    virtual void setReadOnly(bool readOnly, int dimension) OVERRIDE;
    virtual void updateGUI(int dimension) OVERRIDE;
    virtual void reflectAnimationLevel(int /*dimension*/,
                                       AnimationLevelEnum /*level*/) OVERRIDE
    {
    }

    virtual void reflectExpressionState(int /*dimension*/,
                                        bool /*hasExpr*/) OVERRIDE
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

    static KnobGui * BuildKnobGui(KnobIPtr knob,
                                  KnobGuiContainerI *container)
    {
        return new KnobGuiLayers(knob, container);
    }

    KnobGuiLayers(KnobIPtr knob,
                  KnobGuiContainerI *container);

    virtual ~KnobGuiLayers() OVERRIDE;


    virtual KnobIPtr getKnob() const OVERRIDE FINAL;

private:


    virtual bool addNewUserEntry(QStringList& row) OVERRIDE FINAL WARN_UNUSED_RETURN;

    // row has been set-up with old value
    virtual bool editUserEntry(QStringList& row) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void entryRemoved(const QStringList& /*row*/)  OVERRIDE {}

    virtual void tableChanged(int row, int col, std::string* newEncodedValue) OVERRIDE FINAL;
    KnobLayersWPtr _knob;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_KnobGuiLayer_h
