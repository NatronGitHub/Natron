/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef PYTHONPANELS_H
#define PYTHONPANELS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/PyNode.h"
#include "Engine/ScriptObject.h"
#include "Engine/Knob.h"
#include "Engine/ViewIdx.h"

#include "Gui/PanelWidget.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER;
NATRON_PYTHON_NAMESPACE_ENTER;

struct DialogParamHolderPrivate;
class DialogParamHolder
    : public NamedKnobHolder
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    DialogParamHolder(const QString& uniqueID,
                      const AppInstancePtr& app,
                      UserParamHolder* widget);

    virtual ~DialogParamHolder();

    virtual std::string getScriptName_mt_safe() const OVERRIDE FINAL;

    virtual std::string getFullyQualifiedName() const OVERRIDE FINAL;

    void setParamChangedCallback(const QString& callback);

private:

    virtual void initializeKnobs() OVERRIDE FINAL {}

    virtual bool onKnobValueChanged(KnobI* k,
                                    ValueChangedReasonEnum reason,
                                    double time,
                                    ViewSpec view,
                                    bool originatedFromMainThread) OVERRIDE FINAL;
    boost::scoped_ptr<DialogParamHolderPrivate> _imp;
};


struct PyModalDialogPrivate;
class PyModalDialog
    : public QDialog, public UserParamHolder
{
    Q_OBJECT

public:

    PyModalDialog( Gui* gui,
                   StandardButtons defaultButtons = StandardButtons(eStandardButtonOk | eStandardButtonCancel) );

    virtual ~PyModalDialog();

    Param* getParam(const QString& scriptName) const;

    void setParamChangedCallback(const QString& callback);

    void insertWidget(int index, QWidget* widget);

    void addWidget(QWidget* widget);

    DialogParamHolder* getKnobsHolder() const;

private:

    boost::scoped_ptr<PyModalDialogPrivate> _imp;
};


struct PyPanelPrivate;
class PyPanel
    : public QWidget, public UserParamHolder, public PanelWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    PyPanel(const QString& scriptName,
            const QString& label,
            bool useUserParameters,
            GuiApp* app);

    virtual ~PyPanel();

    QString save_serialization_thread() const;

    virtual void restore(const QString& /*data*/) {}

    QString getPanelScriptName() const;

    void setPanelLabel(const QString& label);

    QString getPanelLabel() const;

    Param* getParam(const QString& scriptName) const;
    std::list<Param*> getParams() const;

    void setParamChangedCallback(const QString& callback);

    void insertWidget(int index, QWidget* widget);

    void addWidget(QWidget* widget);

protected:

    virtual QString save() { return QString(); }

    void onUserDataChanged();
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE;
    virtual void enterEvent(QEvent* e) OVERRIDE;
    virtual void leaveEvent(QEvent* e) OVERRIDE;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE;

private:
    boost::scoped_ptr<PyPanelPrivate> _imp;
};


class PyTabWidget
{
    Q_DECLARE_TR_FUNCTIONS(PyTabWidget)

private:
    TabWidget* _tab;

public:

    PyTabWidget(TabWidget* pane);

    ~PyTabWidget();

    TabWidget* getInternalTabWidget() const
    {
        return _tab;
    }

    bool appendTab(PyPanel* tab);

    void insertTab(int index, PyPanel* tab);

    void removeTab(QWidget* tab);

    void removeTab(int index);

    void closeTab(int index);

    QString getTabLabel(int index) const;

    int count();

    QWidget* currentWidget();

    void setCurrentIndex(int index);

    int getCurrentIndex() const;

    PyTabWidget* splitHorizontally();
    PyTabWidget* splitVertically();

    void closePane();

    void floatPane();

    void setNextTabCurrent();

    void floatCurrentTab();

    void closeCurrentTab();

    QString getScriptName() const;
};

NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;

#endif // PYTHONPANELS_H
