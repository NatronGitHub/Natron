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

//Defined to avoid including some headers when running shiboken which may crash shiboken (particularily boost headers)
#define SBK_RUN

#include <pyside_global.h>

//Global
#include <GlobalDefines.h>
#include <Enums.h>

//Engine
#include <AppManager.h>

#endif // PYSIDE_ENGINE_PYTHON_H
