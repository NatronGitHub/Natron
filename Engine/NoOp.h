//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef NOOP_H
#define NOOP_H

#include <boost/shared_ptr.hpp>
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"

/**
 * @brief A NoOp is an effect that doesn't do anything. It is useful for scripting (adding custom parameters)
 * and it is also used to implement the "Dot" node.
 **/
class NoOpBase
    : public Natron::EffectInstance
{
public:

    NoOpBase(boost::shared_ptr<Natron::Node> n);

    virtual int getMajorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 1;
    }

    virtual int getMinorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 0;
    }

    virtual int getMaxInputCount() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 1;
    }

    virtual std::string getPluginID() const WARN_UNUSED_RETURN = 0;
    virtual std::string getPluginLabel() const WARN_UNUSED_RETURN = 0;
    virtual std::string getDescription() const WARN_UNUSED_RETURN = 0;
    virtual void getPluginGrouping(std::list<std::string>* grouping) const OVERRIDE FINAL
    {
        grouping->push_back(PLUGIN_GROUP_OTHER);
    }

    virtual bool isInputOptional(int /*inputNb*/) const
    {
        return false;
    }

    virtual void addAcceptedComponents(int inputNb,std::list<Natron::ImageComponentsEnum>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const OVERRIDE FINAL;

    ///Doesn't really matter here since it won't be used (this effect is always an identity)
    virtual EffectInstance::RenderSafetyEnum renderThreadSafety() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return EffectInstance::eRenderSafetyFullySafeFrame;
    }

private:

    /**
     * @brief A NoOp is always an identity on its input.
     **/
    virtual bool isIdentity(SequenceTime time,
                            const RenderScale & scale,
                            const RectD & rod, //!< image rod in canonical coordinates
                            const double par,
                            int view,
                            SequenceTime* inputTime,
                            int* inputNb) OVERRIDE FINAL WARN_UNUSED_RETURN;
};

class Dot
    : public NoOpBase
{
public:

    static Natron::EffectInstance* BuildEffect(boost::shared_ptr<Natron::Node> n)
    {
        return new Dot(n);
    }

    Dot(boost::shared_ptr<Natron::Node> n)
        : NoOpBase(n)
    {
    }

    virtual std::string getPluginID() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return "Dot";
    }

    virtual std::string getPluginLabel() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return "Dot";
    }

    virtual std::string getDescription() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::string getInputLabel(int /*inputNb*/) const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return "";
    }
};

#endif // NOOP_H
