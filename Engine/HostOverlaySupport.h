/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef HOSTOVERLAYSUPPORT_H
#define HOSTOVERLAYSUPPORT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER


struct HostOverlayKnobsPrivate;
class HostOverlayKnobs
{
public:

    HostOverlayKnobs();

    virtual ~HostOverlayKnobs();


    KnobPtr getFirstKnob() const;

    /**
     * @brief Add an overlay interact slave knob. The role name is the key that will be used to determine
     * if the knob is present when calling checkHostOverlayValid(). If empty, the roleName is the name of the knob
     * otherwise it is expected to match the name filled in the descriveOverlayKnobs function
     **/
    void addKnob(const KnobPtr& knob, int enumID);

    KnobPtr getKnob(int enumID) const;

    template <typename T>
    boost::shared_ptr<T> getKnob(int enumID) const
    {
        KnobPtr knob = getKnob(enumID);

        if (!knob) {
            return boost::shared_ptr<T>();
        }

        return boost::dynamic_pointer_cast<T>(knob);
    }

    /**
     * @brief Must check if all necessary knobs are present for the interact
     **/
    bool checkHostOverlayValid();

protected:

    virtual void describeOverlayKnobs() = 0;

    void describeKnob(int enumID, const std::string& type, int nDims, bool optional = false);

private:


    boost::scoped_ptr<HostOverlayKnobsPrivate> _imp;
};

class HostOverlayKnobsTransform
    : public HostOverlayKnobs
{
public:

    enum KnobsEnumeration
    {
        eKnobsEnumerationTranslate,
        eKnobsEnumerationScale,
        eKnobsEnumerationUniform,
        eKnobsEnumerationRotate,
        eKnobsEnumerationSkewx,
        eKnobsEnumerationSkewy,
        eKnobsEnumerationCenter,
        eKnobsEnumerationSkewOrder,
        eKnobsEnumerationInvert,
        eKnobsEnumerationInteractive
    };

    HostOverlayKnobsTransform()
        : HostOverlayKnobs() {}

    virtual ~HostOverlayKnobsTransform() {}

private:

    virtual void describeOverlayKnobs() OVERRIDE FINAL;
};

class HostOverlayKnobsCornerPin
    : public HostOverlayKnobs
{
public:

    enum KnobsEnumeration
    {
        eKnobsEnumerationFrom1,
        eKnobsEnumerationFrom2,
        eKnobsEnumerationFrom3,
        eKnobsEnumerationFrom4,
        eKnobsEnumerationTo1,
        eKnobsEnumerationTo2,
        eKnobsEnumerationTo3,
        eKnobsEnumerationTo4,
        eKnobsEnumerationEnable1,
        eKnobsEnumerationEnable2,
        eKnobsEnumerationEnable3,
        eKnobsEnumerationEnable4,
        eKnobsEnumerationOverlayPoints,
        eKnobsEnumerationInvert,
        eKnobsEnumerationInteractive
    };

    HostOverlayKnobsCornerPin()
        : HostOverlayKnobs() {}

    virtual ~HostOverlayKnobsCornerPin() {}

private:

    virtual void describeOverlayKnobs() OVERRIDE FINAL;
};


class HostOverlayKnobsPosition
    : public HostOverlayKnobs
{
public:

    enum KnobsEnumeration
    {
        eKnobsEnumerationPosition,
        eKnobsEnumerationInteractive
    };

    HostOverlayKnobsPosition()
        : HostOverlayKnobs() {}

    virtual ~HostOverlayKnobsPosition() {}

private:

    virtual void describeOverlayKnobs() OVERRIDE FINAL;
};


NATRON_NAMESPACE_EXIT

#endif // HOSTOVERLAYSUPPORT_H
