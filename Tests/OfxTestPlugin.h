/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
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

#ifndef NATRON_TESTS_OFXTESTPLUGIN_H
#define NATRON_TESTS_OFXTESTPLUGIN_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <iostream>
#include <string>

#include <gtest/gtest.h>

#include <ofxhBinary.h>
#include <ofxhParam.h>

#include "Engine/AppManager.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/Plugin.h"

// The OFX plug-ins of Tests/OfxTestPlugin/NatronTestPlugin.cpp, built with the tests (see
// Tests/CMakeLists.txt) and loaded by the host like any other OFX plug-in.
#define kNatronTestPluginParamInteract "net.natron.test.ParamInteract"
#define kNatronTestPluginSingleInstance "net.natron.test.SingleInstance"
#define kNatronTestPluginSyncPrivateData "net.natron.test.SyncPrivateData"
#define kNatronTestPluginParams "net.natron.test.Params"

namespace NatronTestPlugin {
// true if the host loaded the plug-in
inline bool
isAvailable(const char* pluginID)
{
    try {
        NATRON_NAMESPACE::Plugin* p = NATRON_NAMESPACE::appPTR->getPluginBinary(QString::fromUtf8(pluginID), -1, -1, false);

        return p && p->getOfxPlugin();
    } catch (...) {
        return false;
    }
}

// The bundled google-test predates GTEST_SKIP(): the tests return early and are reported as passed,
// so say clearly that they did not run.
inline void
reportSkipped(const char* pluginID)
{
    const testing::TestInfo* info = testing::UnitTest::GetInstance()->current_test_info();

    std::cerr << "[  SKIPPED ] " << info->test_case_name() << '.' << info->name() << ": plug-in " << pluginID
              << " not found (the test plug-in built with the tests, NatronTestPlugin.ofx.bundle)" << std::endl;
}

// The value of a counter of the test plug-ins (NatronTestPluginGetCounter() in NatronTestPlugin.cpp),
// or -1 if the plug-in binary can't be loaded. Loading the binary that the host already loaded gives
// the same library, so these are the counters of the plug-ins the host uses.
inline int
counter(const std::string& name)
{
#ifdef NATRON_TESTS_OFX_PLUGIN_BINARY
    static OFX::Binary binary(NATRON_TESTS_OFX_PLUGIN_BINARY);

    if ( !binary.isLoaded() ) {
        binary.load();
    }
    typedef int (*GetCounterFunc)(const char*);
    GetCounterFunc getCounter = (GetCounterFunc)binary.findSymbol("NatronTestPluginGetCounter");

    return getCounter ? getCounter( name.c_str() ) : -1;
#else
    (void)name;

    return -1;
#endif
}

// The OFX parameter instance of a parameter of an OFX node, whose handle (getHandle()) is what the
// plug-in gets from paramGetHandle(), or NULL.
inline OFX::Host::Param::Instance*
ofxParam(const NATRON_NAMESPACE::NodePtr& node,
         const std::string& name)
{
    NATRON_NAMESPACE::OfxEffectInstance* effect = node ? dynamic_cast<NATRON_NAMESPACE::OfxEffectInstance*>( node->getEffectInstance().get() ) : 0;

    return ( effect && effect->effectInstance() ) ? effect->effectInstance()->getParam(name) : 0;
}

// The OFX suites of the host, as the plug-ins get them from fetchSuite().
inline const OfxParameterSuiteV1*
parameterSuite()
{
    return (const OfxParameterSuiteV1*)OFX::Host::Param::GetSuite(1);
}

inline const OfxPropertySuiteV1*
propertySuite()
{
    return (const OfxPropertySuiteV1*)OFX::Host::Property::GetSuite(1);
}
} // namespace NatronTestPlugin

#endif // NATRON_TESTS_OFXTESTPLUGIN_H
