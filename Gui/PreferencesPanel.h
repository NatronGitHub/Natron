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

#ifndef PREFERENCESPANEL_H
#define PREFERENCESPANEL_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Gui/KnobGuiContainerHelper.h"
#include "Gui/LineEdit.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER


struct PreferencesPanelPrivate;
class PreferencesPanel
    : public QWidget
      , public KnobGuiContainerHelper
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    PreferencesPanel(Gui* parent);

    virtual ~PreferencesPanel();

    void createGui();

    virtual Gui* getGui() const OVERRIDE FINAL;
    virtual bool useScrollAreaForTabs() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    void addShortcut(BoundAction* action);

public Q_SLOTS:

    void restoreDefaults();

    void cancelChanges();

    void saveChangesAndClose();

    void onSettingChanged(KnobI* knob);

    void openHelp();

    void onItemSelectionChanged();

    void onItemEnabledCheckBoxChecked(bool);
    void onRSEnabledCheckBoxChecked(bool);
    void onMTEnabledCheckBoxChecked(bool);
    void onGLEnabledCheckBoxChecked(bool);

    void filterPlugins(const QString & txt);


    void onShortcutsSelectionChanged();

    void onValidateShortcutButtonClicked();

    void onClearShortcutButtonClicked();

    void onResetShortcutButtonClicked();

    void onRestoreDefaultShortcutsButtonClicked();

private:

    void createPluginsView(QGridLayout* pluginsFrameLayout);

    void createShortcutEditor(QTreeWidgetItem* uiPageTreeItem);

    virtual void showEvent(QShowEvent* e) OVERRIDE;
    virtual void closeEvent(QCloseEvent* e) OVERRIDE;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void onPageActivated(const KnobPageGuiPtr& page) OVERRIDE FINAL;
    virtual void refreshCurrentPage() OVERRIDE FINAL;
    virtual QWidget* getPagesContainer() const OVERRIDE FINAL;
    virtual QWidget* createPageMainWidget(QWidget* parent) const OVERRIDE FINAL;
    virtual void addPageToPagesContainer(const KnobPageGuiPtr& page) OVERRIDE FINAL;
    virtual void removePageFromContainer(const KnobPageGuiPtr& page) OVERRIDE FINAL;
    virtual void refreshUndoRedoButtonsEnabledNess(bool canUndo, bool canRedo) OVERRIDE FINAL;
    virtual void setPagesOrder(const std::list<KnobPageGuiPtr>& order, const KnobPageGuiPtr& curPage, bool restorePageIndex) OVERRIDE FINAL;
    virtual void onPageLabelChanged(const KnobPageGuiPtr& page) OVERRIDE FINAL;
    boost::scoped_ptr<PreferencesPanelPrivate> _imp;
};


class KeybindRecorder
    : public LineEdit
{
public:

    KeybindRecorder(QWidget* parent);

    virtual ~KeybindRecorder();

private:

    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
};

NATRON_NAMESPACE_EXIT

#endif // PREFERENCESPANEL_H
