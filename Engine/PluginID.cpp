//
//  PluginID.cpp
//  Powiter
//
//  Created by Frédéric Devernay on 01/09/13.
//
//

#include "PluginID.h"

#ifdef __POWITER_UNIX__
#include <dlfcn.h>
#endif

#ifdef __POWITER_WIN32__
#elif defined(__POWITER_UNIX__)
PluginID::~PluginID() {
    dlclose(first);
}
#endif
