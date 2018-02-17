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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "HostOverlaySupport.h"

#include <map>

#include "Engine/KnobTypes.h"


NATRON_NAMESPACE_ENTER

struct HostOverlayKnobDescription
{
    int enumID;
    std::string typeName;
    bool isOptional;
    int nDims;

    HostOverlayKnobDescription(int enumID,
                               const std::string& t,
                               int nDims,
                               bool optional = false)
        : enumID(enumID)
        , typeName(t)
        , isOptional(optional)
        , nDims(nDims)
    {
    }
};

struct HostOverlayKnobsPrivate
{
    std::map<int, KnobPtr> knobs;
    std::vector<HostOverlayKnobDescription> knobsDescription;
    bool describeCall;

    HostOverlayKnobsPrivate()
        : knobs()
        , knobsDescription()
        , describeCall(false)
    {
    }
};

HostOverlayKnobs::HostOverlayKnobs()
    : _imp( new HostOverlayKnobsPrivate() )
{
}

HostOverlayKnobs::~HostOverlayKnobs()
{
}

KnobPtr
HostOverlayKnobs::getFirstKnob() const
{
    if ( _imp->knobs.empty() ) {
        return KnobPtr();
    }

    return _imp->knobs.begin()->second;
}

/**
 * @brief Add an overlay interact slave knob. The role name is the key that will be used to determine
 * if the knob is present when calling checkHostOverlayValid(). If empty, the roleName is the name of the knob
 * otherwise it is expected to match the name filled in the descriveOverlayKnobs function
 **/
void
HostOverlayKnobs::addKnob(const KnobPtr& knob,
                          int enumID)
{
    std::map<int, KnobPtr>::const_iterator found = _imp->knobs.find(enumID);

    assert( found == _imp->knobs.end() );
    if ( found != _imp->knobs.end() ) {
        return;
    }
    _imp->knobs.insert( std::make_pair(enumID, knob) );
}

KnobPtr
HostOverlayKnobs::getKnob(int enumID) const
{
    std::map<int, KnobPtr>::const_iterator found = _imp->knobs.find(enumID);

    if ( found == _imp->knobs.end() ) {
        return KnobPtr();
    }

    return found->second;
}

void
HostOverlayKnobs::describeKnob(int enumID,
                               const std::string& type,
                               int nDims,
                               bool optional)
{
    _imp->knobsDescription.push_back( HostOverlayKnobDescription(enumID, type, nDims, optional) );
}

bool
HostOverlayKnobs::checkHostOverlayValid()
{
    if (!_imp->describeCall) {
        describeOverlayKnobs();
        _imp->describeCall = true;
    }
    for (std::size_t i = 0; i < _imp->knobsDescription.size(); ++i) {
        KnobPtr knob = getKnob(_imp->knobsDescription[i].enumID);

        // Mandatory knob is not present
        if (!knob) {
            if (!_imp->knobsDescription[i].isOptional) {
                return false;
            } else {
                continue;
            }
        }

        // Type mismatch
        if (knob->typeName() != _imp->knobsDescription[i].typeName) {
            return false;
        }

        // Dimension mismatch
        if (knob->getDimension() != _imp->knobsDescription[i].nDims) {
            return false;
        }
    }

    return true;
}

void
HostOverlayKnobsTransform::describeOverlayKnobs()
{
    describeKnob(eKnobsEnumerationTranslate, KnobDouble::typeNameStatic(), 2);
    describeKnob(eKnobsEnumerationScale, KnobDouble::typeNameStatic(), 2);
    describeKnob(eKnobsEnumerationUniform, KnobBool::typeNameStatic(), 1);
    describeKnob(eKnobsEnumerationRotate, KnobDouble::typeNameStatic(), 1);
    describeKnob(eKnobsEnumerationSkewx, KnobDouble::typeNameStatic(), 1);
    describeKnob(eKnobsEnumerationSkewy, KnobDouble::typeNameStatic(), 1);
    describeKnob(eKnobsEnumerationSkewOrder, KnobChoice::typeNameStatic(), 1);
    describeKnob(eKnobsEnumerationCenter, KnobDouble::typeNameStatic(), 2);
    describeKnob(eKnobsEnumerationInvert, KnobBool::typeNameStatic(), 1, true);
    describeKnob(eKnobsEnumerationInteractive, KnobBool::typeNameStatic(), 1, true);
}

void
HostOverlayKnobsCornerPin::describeOverlayKnobs()
{
    describeKnob(eKnobsEnumerationFrom1, KnobDouble::typeNameStatic(), 2);
    describeKnob(eKnobsEnumerationFrom2, KnobDouble::typeNameStatic(), 2);
    describeKnob(eKnobsEnumerationFrom3, KnobDouble::typeNameStatic(), 2);
    describeKnob(eKnobsEnumerationFrom4, KnobDouble::typeNameStatic(), 2);
    describeKnob(eKnobsEnumerationTo1, KnobDouble::typeNameStatic(), 2);
    describeKnob(eKnobsEnumerationTo2, KnobDouble::typeNameStatic(), 2);
    describeKnob(eKnobsEnumerationTo3, KnobDouble::typeNameStatic(), 2);
    describeKnob(eKnobsEnumerationTo4, KnobDouble::typeNameStatic(), 2);
    describeKnob(eKnobsEnumerationEnable1, KnobBool::typeNameStatic(), 1);
    describeKnob(eKnobsEnumerationEnable2, KnobBool::typeNameStatic(), 1);
    describeKnob(eKnobsEnumerationEnable3, KnobBool::typeNameStatic(), 1);
    describeKnob(eKnobsEnumerationEnable4, KnobBool::typeNameStatic(), 1);
    describeKnob(eKnobsEnumerationOverlayPoints, KnobChoice::typeNameStatic(), 1);
    describeKnob(eKnobsEnumerationInvert, KnobBool::typeNameStatic(), 1, true);
    describeKnob(eKnobsEnumerationInteractive, KnobBool::typeNameStatic(), 1, true);
}

void
HostOverlayKnobsPosition::describeOverlayKnobs()
{
    describeKnob(eKnobsEnumerationPosition, KnobDouble::typeNameStatic(), 2);
    describeKnob(eKnobsEnumerationInteractive, KnobBool::typeNameStatic(), 1, true);
}

NATRON_NAMESPACE_EXIT
