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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "PythonPanels.h"

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QMutex>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/NodeWrapper.h"

#include "Gui/Gui.h"
#include "Gui/TabWidget.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/DockablePanel.h"
#include "Gui/GuiAppWrapper.h"


struct DialogParamHolderPrivate
{
    std::string uniqueID;
    
    QMutex paramChangedCBMutex;
    std::string paramChangedCB;
    
    DialogParamHolderPrivate(const std::string& uniqueID)
    : uniqueID(uniqueID)
    , paramChangedCBMutex()
    , paramChangedCB()
    {
        
    }
};

DialogParamHolder::DialogParamHolder(const std::string& uniqueID,AppInstance* app)
: NamedKnobHolder(app)
, _imp(new DialogParamHolderPrivate(uniqueID))
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
DialogParamHolder::setParamChangedCallback(const std::string& callback)
{
    QMutexLocker k(&_imp->paramChangedCBMutex);
    _imp->paramChangedCB = callback;
}

void
DialogParamHolder::onKnobValueChanged(KnobI* k,
                        Natron::ValueChangedReasonEnum reason,
                        SequenceTime /*time*/,
                        bool /*originatedFromMainThread*/)
{
    std::string callback;
    {
        QMutexLocker k(&_imp->paramChangedCBMutex);
        callback = _imp->paramChangedCB;
    }
    if (!callback.empty()) {
        bool userEdited = reason == Natron::eValueChangedReasonNatronGuiEdited ||
        reason == Natron::eValueChangedReasonUserEdited;

        
        std::vector<std::string> args;
        std::string error;
        try {
            Natron::getFunctionArguments(callback, &error, &args);
        } catch (const std::exception& e) {
            getApp()->appendToScriptEditor(std::string("Failed to run onParamChanged callback: ")
                                                             + e.what());
            return;
        }
        if (!error.empty()) {
            getApp()->appendToScriptEditor("Failed to run onParamChanged callback: " + error);
            return;
        }
        
        std::string signatureError;
        signatureError.append("The param changed callback supports the following signature(s):\n");
        signatureError.append("- callback(paramName,app,userEdited)");
        if (args.size() != 3) {
            getApp()->appendToScriptEditor("Failed to run onParamChanged callback: " + signatureError);
            return;
        }
        
        if ((args[0] != "paramName" || args[1] != "app" || args[2] != "userEdited")) {
            getApp()->appendToScriptEditor("Failed to run onParamChanged callback: " + signatureError);
            return;
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
        if (!Natron::interpretPythonScript(script, &err,&output)) {
            getApp()->appendToScriptEditor(QObject::tr("Failed to execute callback: ").toStdString() + err);
        } else if (!output.empty()) {
            getApp()->appendToScriptEditor(output);
        }

    }
}

struct PyModalDialogPrivate
{
    Gui* gui;
    DialogParamHolder* holder;
    
    QVBoxLayout* mainLayout;
    DockablePanel* panel;
    
    QWidget* centerContainer;
    QVBoxLayout* centerLayout;
    
    QDialogButtonBox* buttons;
        
    PyModalDialogPrivate(Gui* gui)
    : gui(gui)
    , holder(0)
    , mainLayout(0)
    , panel(0)
    , centerContainer(0)
    , centerLayout(0)
    , buttons(0)
    {
        
    }
};

PyModalDialog::PyModalDialog(Gui* gui)
: QDialog(gui)
, UserParamHolder()
, _imp(new PyModalDialogPrivate(gui))
{
    _imp->holder = new DialogParamHolder(std::string(),gui->getApp());
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
                                    boost::shared_ptr<QUndoStack>(),
                                    QString(),QString(),
                                    false,
                                    "Default",
                                    _imp->centerContainer);
    _imp->panel->turnOffPages();
    _imp->panel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    _imp->centerLayout->insertWidget(0,_imp->panel);
    
    _imp->mainLayout->addWidget(_imp->centerContainer);
    
    _imp->buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal,this);
    QObject::connect(_imp->buttons, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(_imp->buttons, SIGNAL(rejected()), this, SLOT(reject()));
    _imp->mainLayout->addWidget(_imp->buttons);
}

PyModalDialog::~PyModalDialog()
{
    
}

void
PyModalDialog::insertWidget(int index, QWidget* widget)
{
    _imp->centerLayout->insertWidget(index, widget);
}

void
PyModalDialog::addWidget(QWidget* widget)
{
    _imp->centerLayout->addWidget(widget);
}



void
PyModalDialog::setParamChangedCallback(const std::string& callback)
{
    _imp->holder->setParamChangedCallback(callback);
}

Param*
PyModalDialog::getParam(const std::string& scriptName) const
{
    boost::shared_ptr<KnobI> knob =  _imp->holder->getKnobByName(scriptName);
    if (!knob) {
        return 0;
    }
    return Effect::createParamWrapperForKnob(knob);
}



struct PyPanelPrivate
{
    
    
    DialogParamHolder* holder;
    
    QVBoxLayout* mainLayout;
    DockablePanel* panel;
    
    QWidget* centerContainer;
    QVBoxLayout* centerLayout;

    mutable QMutex serializationMutex;
    std::string serialization;

    
    PyPanelPrivate()
    : holder(0)
    , mainLayout(0)
    , panel(0)
    , centerContainer(0)
    , centerLayout(0)
    , serializationMutex()
    , serialization()
    {
        
    }
};



PyPanel::PyPanel(const std::string& scriptName,const std::string& label,bool useUserParameters,GuiApp* app)
: QWidget(app->getGui())
, UserParamHolder()
, PanelWidget(this,app->getGui())
, _imp(new PyPanelPrivate())
{
    setLabel(label.c_str());

    
    int idx = 1;
    std::string name = Natron::makeNameScriptFriendly(scriptName);
    PanelWidget* existing = 0;
    existing = getGui()->findExistingTab(name);
    while (existing) {
        std::stringstream ss;
        ss << name << idx;
        existing = getGui()->findExistingTab(ss.str());
        if (!existing) {
            name = ss.str();
        }
        ++idx;
    }
    
    setScriptName(name);
    getGui()->registerTab(this,this);

    
    if (useUserParameters) {
        _imp->holder = new DialogParamHolder(name,getGui()->getApp());
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
                                        boost::shared_ptr<QUndoStack>(),
                                        QString(),QString(),
                                        false,
                                        "Default",
                                        _imp->centerContainer);
        _imp->panel->turnOffPages();
        _imp->centerLayout->insertWidget(0,_imp->panel);
        
        _imp->mainLayout->addWidget(_imp->centerContainer);
        _imp->mainLayout->addStretch();
    }
}

PyPanel::~PyPanel()
{
    getGui()->unregisterPyPanel(this);
}

std::string
PyPanel::getPanelScriptName() const
{
    return getScriptName();
}

void
PyPanel::setPanelLabel(const std::string& label)
{
    setLabel(label);
    QString name(label.c_str());
    TabWidget* parent = dynamic_cast<TabWidget*>(parentWidget());
    if (parent) {
        parent->setTabLabel(this, name);
    }
}

std::string
PyPanel::getPanelLabel() const
{
    return getLabel();
}

Param*
PyPanel::getParam(const std::string& scriptName) const
{
    if (!_imp->holder) {
        return 0;
    }
    boost::shared_ptr<KnobI> knob =  _imp->holder->getKnobByName(scriptName);
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
    std::vector<boost::shared_ptr<KnobI> > knobs = _imp->holder->getKnobs_mt_safe();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        Param* p = Effect::createParamWrapperForKnob(*it);
        if (p) {
            ret.push_back(p);
        }
    }
    return ret;
}


void
PyPanel::setParamChangedCallback(const std::string& callback)
{
    if (_imp->holder) {
        _imp->holder->setParamChangedCallback(callback);
    }
}

void
PyPanel::insertWidget(int index, QWidget* widget)
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

std::string
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
    return _tab->appendTab(tab,tab);
}

void
PyTabWidget::insertTab(int index,PyPanel* tab)
{
    _tab->insertTab(index, QIcon(), tab, tab);
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

std::string
PyTabWidget::getTabLabel(int index) const
{
    return _tab->getTabLabel(index).toStdString();
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
    _tab->makeCurrentTab(index);
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
    if (_tab->getGui()->getPanes().size() == 1) {
        _tab->getGui()->getApp()->appendToScriptEditor(QObject::tr("Cannot close pane when this is the last one remaining.").toStdString());
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

std::string
PyTabWidget::getScriptName() const
{
    return _tab->objectName_mt_safe().toStdString();
}