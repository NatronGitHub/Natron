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


#ifndef PYTHONPANELS_H
#define PYTHONPANELS_H

#include <QDialog>

#include "Global/Macros.h"

#include "Engine/NodeWrapper.h"
#include "Engine/Knob.h"

class Gui;

struct DialogParamHolderPrivate;
class DialogParamHolder : public NamedKnobHolder
{
    
public:
    
    DialogParamHolder(const std::string& uniqueID,AppInstance* app);
    
    virtual ~DialogParamHolder();
    
    virtual std::string getScriptName_mt_safe() const OVERRIDE FINAL;
    
    void setParamChangedCallback(const std::string& callback);
    
private:
    
    virtual void initializeKnobs() OVERRIDE FINAL {}
    
    virtual void evaluate(KnobI* /*knob*/,bool /*isSignificant*/,Natron::ValueChangedReasonEnum /*reason*/) OVERRIDE FINAL {}
    
    virtual void onKnobValueChanged(KnobI* /*k*/,
                                    Natron::ValueChangedReasonEnum /*reason*/,
                                    SequenceTime /*time*/,
                                    bool /*originatedFromMainThread*/) OVERRIDE FINAL;

    boost::scoped_ptr<DialogParamHolderPrivate> _imp;
};




struct PyModalDialogPrivate;
class PyModalDialog : public QDialog, public UserParamHolder
{
    Q_OBJECT
    
public:
    
    PyModalDialog(Gui* gui);
    
    virtual ~PyModalDialog();
    
    void setParamChangedCallback(const std::string& callback);
    
private:
    
    boost::scoped_ptr<PyModalDialogPrivate> _imp;

    
};

#endif // PYTHONPANELS_H
