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

#ifndef Gui_KnobGuiFile_h
#define Gui_KnobGuiFile_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QString>
#include <QtCore/QDateTime>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/TimeValue.h"
#include "Gui/KnobGuiTable.h"
#include "Gui/KnobGuiWidgets.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

//================================
class KnobGuiFile
    : public QObject, public KnobGuiWidgets
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    static KnobGuiWidgets * BuildKnobGui(const KnobGuiPtr& knob, ViewIdx view)
    {
        return new KnobGuiFile(knob, view);
    }

    KnobGuiFile(const KnobGuiPtr& knob, ViewIdx view);

    virtual ~KnobGuiFile() OVERRIDE;

    bool checkFileModificationAndWarn(SequenceTime time, bool errorAndAbortRender);

    virtual void reflectLinkedState(DimIdx dimension, bool linked) OVERRIDE;

public Q_SLOTS:

    void onTextEdited();

    void onButtonClicked();

    void onReloadClicked();

    void open_file();

    void onTimelineFrameChanged(SequenceTime time, int reason);


    void onMakeAbsoluteTriggered();

    void onMakeRelativeTriggered();

    void onSimplifyTriggered();

private:

    bool checkFileModificationAndWarnInternal(bool doCheck, TimeValue time, bool errorAndAbortRender);


    virtual void addRightClickMenuEntries(QMenu* menu) OVERRIDE FINAL;
    virtual bool shouldAddStretch() const OVERRIDE FINAL { return false; }

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void setWidgetsVisible(bool visible) OVERRIDE FINAL;
    virtual void setEnabled(const std::vector<bool>& perDimEnabled) OVERRIDE FINAL;
    virtual void reflectMultipleSelection(bool dirty) OVERRIDE FINAL;
    virtual void reflectSelectionState(bool selected) OVERRIDE ;
    virtual void reflectModificationsState() OVERRIDE;
    virtual void updateGUI() OVERRIDE FINAL;
    virtual void reflectAnimationLevel(DimIdx dimension, AnimationLevelEnum level) OVERRIDE FINAL;
    virtual void updateToolTip() OVERRIDE FINAL;

private:

    void updateLastOpened(const QString &str);

    LineEdit *_lineEdit;
    Button *_openFileButton;
    Button* _reloadButton;
    QString _lastOpened;

    //Keep track for each files in the sequence (or video) the modification date
    //of the files
    std::map<std::string, QDateTime> _lastModificationDates;
    KnobFileWPtr _knob;
};



//================================

class KnobGuiPath
    : public KnobGuiTable
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    static KnobGuiWidgets * BuildKnobGui(const KnobGuiPtr& knob, ViewIdx view)
    {
        return new KnobGuiPath(knob, view);
    }

    KnobGuiPath(const KnobGuiPtr& knob, ViewIdx view);

    virtual ~KnobGuiPath() OVERRIDE;

    virtual void reflectLinkedState(DimIdx dimension, bool linked) OVERRIDE;

public Q_SLOTS:

    void onOpenFileButtonClicked();

    void onTextEdited();

    void onMakeAbsoluteTriggered();

    void onMakeRelativeTriggered();

    void onSimplifyTriggered();

private:
    virtual void addRightClickMenuEntries(QMenu* menu) OVERRIDE FINAL;
    virtual void createWidget(QHBoxLayout *layout) OVERRIDE FINAL;
    virtual void setWidgetsVisible(bool visible) OVERRIDE FINAL;
    virtual void setEnabled(const std::vector<bool>& perDimEnabled) OVERRIDE FINAL;
    virtual void reflectMultipleSelection(bool dirty) OVERRIDE FINAL;
    virtual void reflectSelectionState(bool selected) OVERRIDE ;
    virtual void reflectModificationsState() OVERRIDE;
    virtual void updateGUI() OVERRIDE FINAL;
    virtual void reflectAnimationLevel(DimIdx dimension, AnimationLevelEnum level) OVERRIDE FINAL;
    virtual void updateToolTip() OVERRIDE FINAL;
    void updateLastOpened(const QString &str);

    virtual bool addNewUserEntry(QStringList& row) OVERRIDE FINAL;
    // row has been set-up with old value
    virtual bool editUserEntry(QStringList& row) OVERRIDE FINAL;
    virtual void entryRemoved(const QStringList& row) OVERRIDE FINAL;
    virtual void tableChanged(int row, int col, std::string*  newEncodedValue) OVERRIDE FINAL;
    QWidget* _mainContainer;
    LineEdit* _lineEdit;
    Button* _openFileButton;
    QString _lastOpened;
    KnobPathWPtr _knob;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_KnobGuiFile_h

