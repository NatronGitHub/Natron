//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */
#ifndef BACKDROP_H
#define BACKDROP_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Engine/NoOpBase.h"

struct BackDropPrivate;
class BackDrop : public NoOpBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON
    
public:
    
    static Natron::EffectInstance* BuildEffect(boost::shared_ptr<Natron::Node> n)
    {
        return new BackDrop(n);
    }
    
    BackDrop(boost::shared_ptr<Natron::Node> node);
    
    virtual ~BackDrop();
  
    virtual std::string getPluginID() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return PLUGINID_NATRON_BACKDROP;
    }
    
    virtual std::string getPluginLabel() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return "BackDrop";
    }
    
    virtual std::string getDescription() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual std::string getInputLabel(int /*inputNb*/) const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return "";
    }
    
    virtual int getMaxInputCount() const OVERRIDE FINAL  WARN_UNUSED_RETURN
    {
        return 0;
    }
    
Q_SIGNALS:
    
    void labelChanged(QString);
    
private:
    
    virtual void knobChanged(KnobI* k,
                             Natron::ValueChangedReasonEnum /*reason*/,
                             int /*view*/,
                             SequenceTime /*time*/,
                             bool /*originatedFromMainThread*/) OVERRIDE FINAL;

    virtual void initializeKnobs() OVERRIDE FINAL;
    
    boost::scoped_ptr<BackDropPrivate> _imp;
};

#endif // BACKDROP_H
