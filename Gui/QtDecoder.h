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

#ifndef NATRON_READERS_READQT_H
#define NATRON_READERS_READQT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <vector>
#include <string>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QMutex>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/EffectInstance.h"
#include "Engine/EngineFwd.h"


class QtReader
    : public Natron::EffectInstance
{
public:
    static Natron::EffectInstance* BuildEffect(boost::shared_ptr<Natron::Node> n)
    {
        return new QtReader(n);
    }

    QtReader(boost::shared_ptr<Natron::Node> node);

    virtual ~QtReader();

    static void supportedFileFormats_static(std::vector<std::string>* formats);
    virtual std::vector<std::string> supportedFileFormats() const OVERRIDE FINAL;
    virtual bool makePreviewByDefault() const OVERRIDE
    {
        return true;
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
    virtual Natron::StatusEnum getRegionOfDefinition(U64 hash,double time,
                                                 const RenderScale & scale,
                                                 int view,
                                                 RectD* rod) OVERRIDE; //!< rod is in canonical coordinates
    virtual void getFrameRange(double *first,double *last) OVERRIDE;
    virtual int getMaxInputCount() const OVERRIDE
    {
        return 0;
    }

    virtual bool isGenerator() const OVERRIDE
    {
        return true;
    }

    virtual bool isReader() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool isInputOptional(int /*inputNb*/) const OVERRIDE
    {
        return false;
    }

    virtual Natron::StatusEnum render(const RenderActionArgs& args) OVERRIDE;
    virtual void knobChanged(KnobI* k, Natron::ValueChangedReasonEnum reason, int view, double time,
                             bool originatedFromMainThread) OVERRIDE FINAL;
    virtual Natron::RenderSafetyEnum renderThreadSafety() const OVERRIDE
    {
        return Natron::eRenderSafetyInstanceSafe;
    }

    virtual void addAcceptedComponents(int inputNb,std::list<Natron::ImageComponents>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<Natron::ImageBitDepthEnum>* depths) const OVERRIDE FINAL;

    virtual bool isFrameVarying() const OVERRIDE FINAL WARN_UNUSED_RETURN { return true; }
private:

    virtual void initializeKnobs() OVERRIDE;

    void getSequenceTimeDomain(SequenceTime & first,SequenceTime & last);

    void timeDomainFromSequenceTimeDomain(SequenceTime & first,SequenceTime & last,bool mustSetFrameRange);

    SequenceTime getSequenceTime(SequenceTime t);

    void getFilenameAtSequenceTime(SequenceTime time, std::string &filename);


    const Natron::Color::Lut* _lut;
    std::string _filename;
    QImage* _img;
    QMutex _lock;
    boost::shared_ptr<KnobFile> _fileKnob;
    boost::shared_ptr<KnobInt> _firstFrame;
    boost::shared_ptr<KnobChoice> _before;
    boost::shared_ptr<KnobInt> _lastFrame;
    boost::shared_ptr<KnobChoice> _after;
    boost::shared_ptr<KnobChoice> _missingFrameChoice;
    boost::shared_ptr<KnobChoice> _frameMode;
    boost::shared_ptr<KnobInt> _startingFrame;
    boost::shared_ptr<KnobInt> _timeOffset;
    bool _settingFrameRange;
};

#endif /* defined(NATRON_READERS_READQT_H_) */
