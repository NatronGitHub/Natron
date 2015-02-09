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

/**
* @brief This is the global header used by Shiboken to generate bindings for the Engine library.
**/

#ifndef PYSIDE_ENGINE_PYTHON_H
#define PYSIDE_ENGINE_PYTHON_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

//Defined to avoid including some headers when running shiboken which may crash shiboken (particularily boost headers)
#define SBK_RUN

#include <pyside_global.h>

//Global
#include <GlobalDefines.h>
#include <Enums.h>

//Engine
#include "GlobalFunctionsWrapper.h"
#include "NodeGroupWrapper.h"
#include "AppInstanceWrapper.h"
#include "RotoWrapper.h"
#include "NodeWrapper.h"
#include "ParameterWrapper.h"

#endif // PYSIDE_ENGINE_PYTHON_H
