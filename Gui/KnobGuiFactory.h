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

#ifndef NATRON_GUI_KNOBGUIFACTORY_H_
#define NATRON_GUI_KNOBGUIFACTORY_H_

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

class KnobI;
class DockablePanel;

namespace Natron {
class LibraryBinary;
}

/******************************KNOB_FACTORY**************************************/
//Maybe the factory should move to a separate file since it is used to create KnobGui aswell
class KnobGui;
class KnobHolder;

class KnobGuiFactory
{
    std::map<std::string, Natron::LibraryBinary *> _loadedKnobs;

public:
    KnobGuiFactory();

    ~KnobGuiFactory();


    KnobGui * createGuiForKnob(boost::shared_ptr<KnobI> knob, DockablePanel *container) const;

private:
    const std::map<std::string, Natron::LibraryBinary *> &getLoadedKnobs() const
    {
        return _loadedKnobs;
    }

    void loadKnobPlugins();

    void loadBultinKnobs();
};


#endif // NATRON_GUI_KNOBGUIFACTORY_H_
