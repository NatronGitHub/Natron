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

#ifndef NATRON_WRITERS_WRITEQT_H
#define NATRON_WRITERS_WRITEQT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/OutputEffectInstance.h"

namespace Natron {
namespace Color {
class Lut;
}
}

class KnobOutputFile;
class KnobChoice;
class KnobButton;
class KnobInt;
class KnobBool;

class QtWriter
    : public Natron::OutputEffectInstance
{
public:
    static Natron::EffectInstance* BuildEffect(boost::shared_ptr<Natron::Node> n)
    {
        return new QtWriter(n);
    }

    QtWriter(boost::shared_ptr<Natron::Node> node);

    virtual ~QtWriter();

    virtual bool isWriter() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    static void supportedFileFormats_static(std::vector<std::string>* formats);
    virtual std::vector<std::string> supportedFileFormats() const OVERRIDE FINAL;
    virtual bool isInputOptional(int /*inputNb*/) const OVERRIDE
    {
        return false;
    }

    virtual int getMajorVersion() const OVERRIDE
    {
        return 1;
    }

    virtual int getMinorVersion() const OVERRIDE
    {
        return 0;
    }

    virtual std::string getPluginID() const OVERRIDE;
    virtual std::string getPluginLabel() const OVERRIDE;
    virtual void getPluginGrouping(std::list<std::string>* grouping) const OVERRIDE FINAL;
    virtual std::string getPluginDescription() const OVERRIDE;
    virtual void getFrameRange(double *first,double *last) OVERRIDE;
    virtual int getMaxInputCount() const OVERRIDE
    {
        return 1;
    }

    void knobChanged(KnobI* k, Natron::ValueChangedReasonEnum reason, int view, double time,
                     bool originatedFromMainThread) OVERRIDE FINAL;
    virtual Natron::StatusEnum render(const RenderActionArgs& args) OVERRIDE;
    virtual void addAcceptedComponents(int inputNb,std::list<Natron::ImageComponents>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const OVERRIDE FINAL;

protected:


    virtual void initializeKnobs() OVERRIDE;
    virtual Natron::RenderSafetyEnum renderThreadSafety() const OVERRIDE
    {
        return Natron::eRenderSafetyInstanceSafe;
    }

private:

    const Natron::Color::Lut* _lut;
    boost::shared_ptr<KnobBool> _premultKnob;
    boost::shared_ptr<KnobOutputFile> _fileKnob;
    boost::shared_ptr<KnobChoice> _frameRangeChoosal;
    boost::shared_ptr<KnobInt> _firstFrameKnob;
    boost::shared_ptr<KnobInt> _lastFrameKnob;
    boost::shared_ptr<KnobButton> _renderKnob;
};

#endif /* defined(NATRON_WRITERS_WRITEQT_H_) */
