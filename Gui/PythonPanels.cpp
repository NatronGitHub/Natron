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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "PythonPanels.h"

#include <stdexcept>
#include <sstream> // stringstream

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QMutex>
#include <QVBoxLayout>
#include <QHBoxLayout>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/PyNode.h"
#include "Engine/ViewIdx.h"

#include "Gui/DialogButtonBox.h"
#include "Gui/Gui.h"
#include "Gui/TabWidget.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/DockablePanel.h"
#include "Gui/PyGuiApp.h"

#include "Serialization/WorkspaceSerialization.h"


NATRON_NAMESPACE_ENTER


NATRON_PYTHON_NAMESPACE_ENTER


struct DialogParamHolderPrivate
{
    std::string uniqueID;
    QMutex paramChangedCBMutex;
    std::string paramChangedCB;
    UserParamHolder* widget;

    DialogParamHolderPrivate(UserParamHolder* widget,
                             const QString& uniqueID)
        : uniqueID( uniqueID.toStdString() )
        , paramChangedCBMutex()
        , paramChangedCB()
        , widget(widget)
    {
    }
};

DialogParamHolder::DialogParamHolder(const QString& uniqueID,
                                     const AppInstancePtr& app,
                                     UserParamHolder* widget)
    : NamedKnobHolder(app)
    , _imp( new DialogParamHolderPrivate(widget, uniqueID) )
{
}

DialogParamHolder::~DialogParamHolder()
{
}

std::string
DialogParamHolder::getScriptName_mt_safe() const
{
    return _imp->uniqueID;
}

void
DialogParamHolder::setParamChangedCallback(const QString& callback)
{
    QMutexLocker k(&_imp->paramChangedCBMutex);

    _imp->paramChangedCB = callback.toStdString();
}

bool
DialogParamHolder::onKnobValueChanged(const KnobIPtr& k,
                                      ValueChangedReasonEnum reason,
                                      TimeValue time,
                                      ViewSetSpec view)
{
    std::string callback;
    {
        QMutexLocker l(&_imp->paramChangedCBMutex);
        callback = _imp->paramChangedCB;
    }

    if ( !callback.empty() ) {
        bool userEdited = reason == eValueChangedReasonUserEdited ||
                          reason == eValueChangedReasonUserEdited;
        std::vector<std::string> args;
        std::string error;
        try {
            NATRON_PYTHON_NAMESPACE::getFunctionArguments(callback, &error, &args);
        } catch (const std::exception& e) {
            getApp()->appendToScriptEditor( std::string("Failed to get signature of onParamChanged callback: ")
                                            + e.what() );

            return false;
        }

        if ( !error.empty() ) {
            getApp()->appendToScriptEditor("Failed to get signature of onParamChanged callback: " + error);

            return false;
        }

        std::string signatureError;
        signatureError.append("The param changed callback supports the following signature(s):\n");
        signatureError.append("- callback(paramName, app, userEdited)");
        if (args.size() != 3) {
            getApp()->appendToScriptEditor("Wrong signature of onParamChanged callback: " + signatureError);

            return false;
        }

        if ( ( (args[0] != "paramName") || (args[1] != "app") || (args[2] != "userEdited") ) ) {
            getApp()->appendToScriptEditor("Wrong signature of onParamChanged callback: " + signatureError);

            return false;
        }

        std::string appID =  getApp()->getAppIDString();
        std::stringstream ss;
        ss << callback << "(\"" << k->getName() << "\"," << appID << ",";
        if (userEdited) {
            ss << "True";
        } else {
            ss << "False";
        }
        ss << ")\n";

        std::string script = ss.str();
        std::string err;
        std::string output;
        if ( !NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, &err, &output) ) {
            getApp()->appendToScriptEditor( tr("Failed to execute callback: %1").arg( QString::fromUtf8( err.c_str() ) ).toStdString() );
        } else if ( !output.empty() ) {
            getApp()->appendToScriptEditor(output);
        }

        return true;
    }
    _imp->widget->onKnobValueChanged(k, reason, time, view);

    return false;
} // DialogParamHolder::onKnobValueChanged

struct PyModalDialogPrivate
{
    Gui* gui;
    DialogParamHolderPtr holder;
    QVBoxLayout* mainLayout;
    DockablePanel* panel;
    QWidget* centerContainer;
    QVBoxLayout* centerLayout;
    DialogButtonBox* buttons;

    PyModalDialogPrivate(Gui* gui)
        : gui(gui)
        , holder()
        , mainLayout(0)
        , panel(0)
        , centerContainer(0)
        , centerLayout(0)
        , buttons(0)
    {
    }
};

PyModalDialog::PyModalDialog(Gui* gui,
                             StandardButtons defaultButtons)
    : QDialog(gui)
    , UserParamHolder()
    , _imp( new PyModalDialogPrivate(gui) )
{
    _imp->holder = DialogParamHolder::create( QString(), gui->getApp(), this );
    setHolder(_imp->holder);
    _imp->holder->initializeKnobsPublic();
    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);

    _imp->centerContainer = new QWidget(this);
    _imp->centerLayout = new QVBoxLayout(_imp->centerContainer);
    _imp->centerLayout->setContentsMargins(0, 0, 0, 0);


    _imp->panel = new DockablePanel(gui,
                                    _imp->holder,
                                    _imp->mainLayout,
                                    DockablePanel::eHeaderModeNoHeader,
                                    false,
                                    QUndoStackPtr(),
                                    QString(), QString(),
                                    _imp->centerContainer);
    _imp->panel->turnOffPages();
    _imp->holder->setPanelPointer(_imp->panel);
    _imp->panel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    _imp->centerLayout->insertWidget(0, _imp->panel);

    _imp->mainLayout->addWidget(_imp->centerContainer);

    if ( (defaultButtons & eStandardButtonNoButton) == 0 ) {
        QDialogButtonBox::StandardButtons qbuttons;
        if (defaultButtons & eStandardButtonOk) {
            qbuttons |= QDialogButtonBox::Ok;
        }
        if (defaultButtons & eStandardButtonSave) {
            qbuttons |= QDialogButtonBox::Save;
        }
        if (defaultButtons & eStandardButtonSaveAll) {
            qbuttons |= QDialogButtonBox::SaveAll;
        }
        if (defaultButtons & eStandardButtonOpen) {
            qbuttons |= QDialogButtonBox::Open;
        }
        if (defaultButtons & eStandardButtonYes) {
            qbuttons |= QDialogButtonBox::Yes;
        }
        if (defaultButtons & eStandardButtonYesToAll) {
            qbuttons |= QDialogButtonBox::YesToAll;
        }
        if (defaultButtons & eStandardButtonNo) {
            qbuttons |= QDialogButtonBox::No;
        }
        if (defaultButtons & eStandardButtonNoToAll) {
            qbuttons |= QDialogButtonBox::NoToAll;
        }
        if (defaultButtons & eStandardButtonAbort) {
            qbuttons |= QDialogButtonBox::Abort;
        }
        if (defaultButtons & eStandardButtonRetry) {
            qbuttons |= QDialogButtonBox::Retry;
        }
        if (defaultButtons & eStandardButtonIgnore) {
            qbuttons |= QDialogButtonBox::Ignore;
        }
        if (defaultButtons & eStandardButtonClose) {
            qbuttons |= QDialogButtonBox::Close;
        }
        if (defaultButtons & eStandardButtonCancel) {
            qbuttons |= QDialogButtonBox::Cancel;
        }
        if (defaultButtons & eStandardButtonDiscard) {
            qbuttons |= QDialogButtonBox::Discard;
        }
        if (defaultButtons & eStandardButtonHelp) {
            qbuttons |= QDialogButtonBox::Help;
        }
        if (defaultButtons & eStandardButtonApply) {
            qbuttons |= QDialogButtonBox::Apply;
        }
        if (defaultButtons & eStandardButtonReset) {
            qbuttons |= QDialogButtonBox::Reset;
        }
        if (defaultButtons & eStandardButtonRestoreDefaults) {
            qbuttons |= QDialogButtonBox::RestoreDefaults;
        }


        _imp->buttons = new DialogButtonBox(qbuttons, Qt::Horizontal, this);
        _imp->buttons->setFocusPolicy(Qt::TabFocus);
        QObject::connect( _imp->buttons, SIGNAL(accepted()), this, SLOT(accept()) );
        QObject::connect( _imp->buttons, SIGNAL(rejected()), this, SLOT(reject()) );
        _imp->mainLayout->addWidget(_imp->buttons);
    }
}

PyModalDialog::~PyModalDialog()
{
}

DialogParamHolderPtr
PyModalDialog::getKnobsHolder() const
{
    return _imp->holder;
}

void
PyModalDialog::insertWidget(int index,
                            QWidget* widget)
{
    _imp->centerLayout->insertWidget(index, widget);
}

void
PyModalDialog::addWidget(QWidget* widget)
{
    _imp->centerLayout->addWidget(widget);
}

void
PyModalDialog::setParamChangedCallback(const QString& callback)
{
    _imp->holder->setParamChangedCallback(callback);
}

Param*
PyModalDialog::getParam(const QString& scriptName) const
{
    KnobIPtr knob =  _imp->holder->getKnobByName( scriptName.toStdString() );

    if (!knob) {
        return 0;
    }

    return Effect::createParamWrapperForKnob(knob);
}

struct PyPanelPrivate
{
    DialogParamHolderPtr holder;
    QVBoxLayout* mainLayout;
    DockablePanel* panel;
    QWidget* centerContainer;
    QVBoxLayout* centerLayout;
    mutable QMutex serializationMutex;
    QString serialization;
    QString pythonFunction;

    PyPanelPrivate()
        : holder()
        , mainLayout(0)
        , panel(0)
        , centerContainer(0)
        , centerLayout(0)
        , serializationMutex()
        , serialization()
        , pythonFunction()
    {
    }
};

PyPanel::PyPanel(const QString& scriptName,
                 const QString& label,
                 bool useUserParameters,
                 GuiApp* app)
    : QWidget( app->getGui() )
    , UserParamHolder()
    , PanelWidget(scriptName.toStdString(), this, app->getGui() )
    , _imp( new PyPanelPrivate() )
{
    setLabel( label.toStdString() );


    int idx = 1;
    std::string name = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly( scriptName.toStdString() );
    PanelWidget* existing = 0;
    existing = getGui()->findExistingTab(name);
    while (existing) {
        std::stringstream ss;
        ss << name << idx;
        existing = getGui()->findExistingTab( ss.str() );
        if (!existing) {
            name = ss.str();
        }
        ++idx;
    }

    setScriptName(name);


    if (useUserParameters) {
        _imp->holder = DialogParamHolder::create( QString::fromUtf8( name.c_str() ), getGui()->getApp(), this );
        setHolder(_imp->holder);
        _imp->holder->initializeKnobsPublic();
        _imp->mainLayout = new QVBoxLayout(this);
        _imp->mainLayout->setContentsMargins(0, 0, 0, 0);

        _imp->centerContainer = new QWidget(this);
        _imp->centerLayout = new QVBoxLayout(_imp->centerContainer);
        _imp->centerLayout->setContentsMargins(0, 0, 0, 0);

        _imp->panel = new DockablePanel(getGui(),
                                        _imp->holder,
                                        _imp->mainLayout,
                                        DockablePanel::eHeaderModeNoHeader,
                                        false,
                                        QUndoStackPtr(),
                                        QString(), QString(),
                                        _imp->centerContainer);
        _imp->holder->setPanelPointer(_imp->panel);
        _imp->panel->turnOffPages();
        _imp->centerLayout->insertWidget(0, _imp->panel);

        _imp->mainLayout->addWidget(_imp->centerContainer);
        _imp->mainLayout->addStretch();
    }
}

PyPanel::~PyPanel()
{
    getGui()->getApp()->unregisterPyPanel(this);
}

void
PyPanel::setPythonFunction(const QString& function)
{
    QMutexLocker k(&_imp->serializationMutex);
    _imp->pythonFunction = function;
}

QString
PyPanel::getPythonFunction() const
{
    QMutexLocker k(&_imp->serializationMutex);
    return _imp->pythonFunction;
}

QString
PyPanel::getPanelScriptName() const
{
    return QString::fromUtf8( getScriptName().c_str() );
}

void
PyPanel::setPanelLabel(const QString& label)
{
    setLabel( label.toStdString() );
    TabWidget* parent = dynamic_cast<TabWidget*>( parentWidget() );
    if (parent) {
        parent->setTabLabel(this, label);
    }
}

QString
PyPanel::getPanelLabel() const
{
    return QString::fromUtf8( getLabel().c_str() );
}

Param*
PyPanel::getParam(const QString& scriptName) const
{
    if (!_imp->holder) {
        return 0;
    }
    KnobIPtr knob =  _imp->holder->getKnobByName( scriptName.toStdString() );
    if (!knob) {
        return 0;
    }

    return Effect::createParamWrapperForKnob(knob);
}

std::list<Param*>
PyPanel::getParams() const
{
    std::list<Param*> ret;

    if (!_imp->holder) {
        return ret;
    }
    KnobsVec knobs = _imp->holder->getKnobs_mt_safe();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        Param* p = Effect::createParamWrapperForKnob(*it);
        if (p) {
            ret.push_back(p);
        }
    }

    return ret;
}

void
PyPanel::setParamChangedCallback(const QString& callback)
{
    if (_imp->holder) {
        _imp->holder->setParamChangedCallback(callback);
    }
}

void
PyPanel::insertWidget(int index,
                      QWidget* widget)
{
    if (!_imp->centerLayout) {
        return;
    }
    _imp->centerLayout->insertWidget(index, widget);
}

void
PyPanel::addWidget(QWidget* widget)
{
    if (!_imp->centerLayout) {
        return;
    }
    _imp->centerLayout->addWidget(widget);
}

void
PyPanel::onUserDataChanged()
{
    QMutexLocker k(&_imp->serializationMutex);

    _imp->serialization = save();
}

QString
PyPanel::save_serialization_thread() const
{
    QMutexLocker k(&_imp->serializationMutex);

    return _imp->serialization;
}

void
PyPanel::mousePressEvent(QMouseEvent* e)
{
    takeClickFocus();
    QWidget::mousePressEvent(e);
}

void
PyPanel::enterEvent(QEvent* e)
{
    enterEventBase();
    QWidget::enterEvent(e);
}

void
PyPanel::leaveEvent(QEvent* e)
{
    leaveEventBase();
    QWidget::leaveEvent(e);
}

void
PyPanel::keyPressEvent(QKeyEvent* e)
{
    QWidget::keyPressEvent(e);
}

KnobsVec
PyPanel::getKnobs() const
{
    return _imp->holder->getKnobs_mt_safe();
}


PyTabWidget::PyTabWidget(TabWidget* pane)
    : _tab(pane)
{
}

PyTabWidget::~PyTabWidget()
{
}

bool
PyTabWidget::appendTab(PyPanel* tab)
{
    return _tab->appendTab(tab, tab);
}

void
PyTabWidget::insertTab(int index,
                       PyPanel* tab)
{
    _tab->insertTab(index, tab, tab);
}

void
PyTabWidget::removeTab(QWidget* tab)
{
    PyPanel* isPanel = dynamic_cast<PyPanel*>(tab);

    if (!isPanel) {
        return;
    }
    _tab->removeTab(isPanel, false);
}

void
PyTabWidget::removeTab(int index)
{
    _tab->removeTab(index, false);
}

void
PyTabWidget::closeTab(int index)
{
    _tab->removeTab(index, true);
}

QString
PyTabWidget::getTabLabel(int index) const
{
    return QString::fromUtf8( _tab->getTabLabel(index).toStdString().c_str() );
}

int
PyTabWidget::count()
{
    return _tab->count();
}

QWidget*
PyTabWidget::currentWidget()
{
    PanelWidget* w =  _tab->currentWidget();

    if (!w) {
        return 0;
    }

    return w->getWidget();
}

void
PyTabWidget::setCurrentIndex(int index)
{
    _tab->setCurrentIndex(index);
}

int
PyTabWidget::getCurrentIndex() const
{
    return _tab->activeIndex();
}

PyTabWidget*
PyTabWidget::splitHorizontally()
{
    TabWidget* ret = _tab->splitHorizontally();

    if (ret) {
        return new PyTabWidget(ret);
    } else {
        return 0;
    }
}

PyTabWidget*
PyTabWidget::splitVertically()
{
    TabWidget* ret = _tab->splitVertically();

    if (ret) {
        return new PyTabWidget(ret);
    } else {
        return 0;
    }
}

void
PyTabWidget::closePane()
{
    if (_tab->getGui()->getApp()->getTabWidgetsSerialization().size() == 1) {
        _tab->getGui()->getApp()->appendToScriptEditor( tr("Cannot close pane when this is the last one remaining.").toStdString() );

        return;
    }
    _tab->closePane();
    _tab->close();
}

void
PyTabWidget::floatPane()
{
    _tab->floatPane();
}

void
PyTabWidget::setNextTabCurrent()
{
    _tab->moveToNextTab();
}

void
PyTabWidget::floatCurrentTab()
{
    _tab->floatCurrentWidget();
}

void
PyTabWidget::closeCurrentTab()
{
    _tab->closeCurrentWidget();
}

QString
PyTabWidget::getScriptName() const
{
    return QString::fromUtf8(_tab->getScriptName().c_str());
}

NATRON_PYTHON_NAMESPACE_EXIT
NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
NATRON_PYTHON_NAMESPACE_USING
#include "moc_PythonPanels.cpp"
