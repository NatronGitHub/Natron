/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2018-2023 The Natron developers
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

#include <iostream>
#include <string>
#include <exception>

#include "CallbacksManager.h"


#ifdef Q_OS_WIN
// g++ knows nothing about wmain
// https://sourceforge.net/p/mingw-w64/wiki2/Unicode%20apps/
extern "C" {
int wmain(int argc, wchar_t** argv)
#else
int main(int argc, char *argv[])
#endif
{
    CallbacksManager manager;

    try {
#ifdef Q_OS_WIN
         manager.initW(argc, argv);
#else
        manager.init(argc, argv);
#endif
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;

        return 1;
    }

    if ( !manager.hasInit() ) {
        return 1;
    }

    return manager.exec();
} // main
#ifdef Q_OS_WIN
} // extern "C"
#endif
