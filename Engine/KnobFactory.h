//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_KNOBFACTORY_H_
#define NATRON_ENGINE_KNOBFACTORY_H_

#include <string>
#include <map>

#include "Global/GlobalDefines.h"
#include "Engine/Curve.h"

class Knob;
class AppInstance;

namespace Natron
{
class LibraryBinary;
}

/******************************KNOB_FACTORY**************************************/

class KnobHolder;

class KnobFactory
{
public:
    KnobFactory();

    ~KnobFactory();

    Knob *createKnob(const std::string &id, KnobHolder *holder, const std::string &description, int dimension = 1) const;

private:
    const std::map<std::string, Natron::LibraryBinary *> &getLoadedKnobs() const {
        return _loadedKnobs;
    }

    void loadKnobPlugins();

    void loadBultinKnobs();

private:
    std::map<std::string, Natron::LibraryBinary *> _loadedKnobs;

};


#endif // NATRON_ENGINE_KNOBFACTORY_H_
