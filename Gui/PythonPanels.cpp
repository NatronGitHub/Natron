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


#include "PythonPanels.h"

#include <QMutex>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>

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
        
        std::string script = callback;
        script.append("()\n");
        KnobI::declareCurrentKnobVariable_Python(k, -1, script);
        script.append("userEdited = ");
        if (userEdited) {
            script.append("True\n");
        } else {
            script.append("False\n");
        }
        std::string err;
        if (!Natron::interpretPythonScript(script, &err,NULL)) {
            getApp()->appendToScriptEditor(QObject::tr("Failed to execute callback: ").toStdString() + err);
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
    
    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    
    _imp->centerContainer = new QWidget(this);
    _imp->centerLayout = new QVBoxLayout(_imp->centerContainer);
    _imp->centerLayout->setContentsMargins(0, 0, 0, 0);
    
    
    _imp->panel = new DockablePanel(_imp->gui,
                                    _imp->holder,
                                    _imp->mainLayout,
                                    DockablePanel::eHeaderModeNoHeader,
                                    false,
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
    
    Gui* gui;
    
    mutable QMutex labelMutex;
    std::string label;
    
    DialogParamHolder* holder;
    
    QVBoxLayout* mainLayout;
    DockablePanel* panel;
    
    QWidget* centerContainer;
    QVBoxLayout* centerLayout;
    
    QMutex paramChangedCBMutex;
    std::string paramChangedCB;
    
    mutable QMutex serializationMutex;
    std::string serialization;

    
    PyPanelPrivate(const std::string& label)
    : gui(0)
    , labelMutex()
    , label(label)
    , holder(0)
    , mainLayout(0)
    , panel(0)
    , centerContainer(0)
    , centerLayout(0)
    , paramChangedCBMutex()
    , paramChangedCB()
    , serializationMutex()
    , serialization()
    {
        
    }
};



PyPanel::PyPanel(const std::string& label,bool useUserParameters,GuiApp* app)
: QWidget(app->getGui())
, UserParamHolder()
, _imp(new PyPanelPrivate(label))
{
    _imp->gui = app->getGui();
    setObjectName(label.c_str());
    _imp->gui->registerTab(this);
   
    
    if (useUserParameters) {
        _imp->holder = new DialogParamHolder(_imp->label,_imp->gui->getApp());
        
        _imp->mainLayout = new QVBoxLayout(this);
        _imp->mainLayout->setContentsMargins(0, 0, 0, 0);
        
        _imp->centerContainer = new QWidget(this);
        _imp->centerLayout = new QVBoxLayout(_imp->centerContainer);
        _imp->centerLayout->setContentsMargins(0, 0, 0, 0);
        
        _imp->panel = new DockablePanel(_imp->gui,
                                        _imp->holder,
                                        _imp->mainLayout,
                                        DockablePanel::eHeaderModeNoHeader,
                                        false,
                                        QString(),QString(),
                                        false,
                                        "Default",
                                        _imp->centerContainer);
        _imp->panel->turnOffPages();
        _imp->panel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        _imp->centerLayout->insertWidget(0,_imp->panel);
        
        _imp->mainLayout->addWidget(_imp->centerContainer);
    }
}

PyPanel::~PyPanel()
{
    
}

void
PyPanel::setLabel(const std::string& label)
{
    {
        QMutexLocker k(&_imp->labelMutex);
        _imp->label = label;
    }
    QString name(label.c_str());
    setObjectName(name);
    TabWidget* parent = dynamic_cast<TabWidget*>(parentWidget());
    if (parent) {
        parent->setTabName(this, name);
    }
}

std::string
PyPanel::getLabel() const
{
    QMutexLocker k(&_imp->labelMutex);
    return _imp->label;
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
    const std::vector<boost::shared_ptr<KnobI> >& knobs = _imp->holder->getKnobs();
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
    QMutexLocker k(&_imp->paramChangedCBMutex);
    _imp->paramChangedCB = callback;
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

PyTabWidget::PyTabWidget(TabWidget* pane)
: _tab(pane)
{
    
}

PyTabWidget::~PyTabWidget()
{
    
}

bool
PyTabWidget::appendTab(QWidget* tab)
{
    return _tab->appendTab(tab);
}

void
PyTabWidget::insertTab(int index,QWidget* tab)
{
    _tab->insertTab(index, QIcon(), tab);
}

void
PyTabWidget::removeTab(QWidget* tab)
{
    _tab->removeTab(tab, false);
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
PyTabWidget::getTabName(int index) const
{
    return _tab->getTabName(index).toStdString();
}

int
PyTabWidget::count()
{
    return _tab->count();
}

QWidget*
PyTabWidget::currentWidget()
{
    return _tab->currentWidget();
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
    _tab->closePane();
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