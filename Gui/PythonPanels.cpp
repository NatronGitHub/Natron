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

#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/DockablePanel.h"



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
    QDialogButtonBox* buttons;
    
    PyModalDialogPrivate(Gui* gui)
    : gui(gui)
    , holder(0)
    , mainLayout(0)
    , panel(0)
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
    
    _imp->mainLayout = new QVBoxLayout(this);
    
    _imp->panel = new DockablePanel(gui,
                                    _imp->holder,
                                    _imp->mainLayout,
                                    DockablePanel::eHeaderModeNoHeader,
                                    false,
                                    QString(),QString(),
                                    false,
                                    "Default",
                                    this);
    
    _imp->mainLayout->addWidget(_imp->panel);
    
    _imp->buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal,this);
    QObject::connect(_imp->buttons, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(_imp->buttons, SIGNAL(rejected()), this, SLOT(reject()));
    _imp->mainLayout->addWidget(_imp->buttons);
}

void
PyModalDialog::setParamChangedCallback(const std::string& callback)
{
    _imp->holder->setParamChangedCallback(callback);
}


