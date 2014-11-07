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

#ifndef NATRON_WRITERS_WRITEQT_H_
#define NATRON_WRITERS_WRITEQT_H_


#include "Engine/EffectInstance.h"

namespace Natron {
namespace Color {
class Lut;
}
}

class OutputFile_Knob;
class Choice_Knob;
class Button_Knob;
class Int_Knob;
class Bool_Knob;

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
    virtual std::string getDescription() const OVERRIDE;
    virtual void getFrameRange(SequenceTime *first,SequenceTime *last) OVERRIDE;
    virtual int getMaxInputCount() const OVERRIDE
    {
        return 1;
    }

    void knobChanged(KnobI* k, Natron::ValueChangedReasonEnum reason, int view, SequenceTime time) OVERRIDE FINAL;
    virtual Natron::StatusEnum render(SequenceTime time,
                                  const RenderScale & scale,
                                  const RectI & roi,
                                  int view,
                                  bool isSequentialRender,
                                  bool isRenderResponseToUserInteraction,
                                  boost::shared_ptr<Natron::Image> output) OVERRIDE;
    virtual void addAcceptedComponents(int inputNb,std::list<Natron::ImageComponentsEnum>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const OVERRIDE FINAL;

protected:


    virtual void initializeKnobs() OVERRIDE;
    virtual Natron::EffectInstance::RenderSafetyEnum renderThreadSafety() const OVERRIDE
    {
        return Natron::EffectInstance::eRenderSafetyInstanceSafe;
    }

private:

    const Natron::Color::Lut* _lut;
    boost::shared_ptr<Bool_Knob> _premultKnob;
    boost::shared_ptr<OutputFile_Knob> _fileKnob;
    boost::shared_ptr<Choice_Knob> _frameRangeChoosal;
    boost::shared_ptr<Int_Knob> _firstFrameKnob;
    boost::shared_ptr<Int_Knob> _lastFrameKnob;
    boost::shared_ptr<Button_Knob> _renderKnob;
};

#endif /* defined(NATRON_WRITERS_WRITEQT_H_) */
