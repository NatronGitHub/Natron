#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TEMPLATE = subdirs

# build things in the order we give
CONFIG += ordered

SUBDIRS += \
    HostSupport \
    Engine \
    Gui \
    Tests \
    App

OTHER_FILES += \
    Global/Enums.h \
    Global/GLIncludes.h \
    Global/GlobalDefines.h \
    Global/Macros.h \
    Global/MemoryInfo.h \
    Global/QtCompat.h \
    global.pri \
    config.pri

include(global.pri)
include(config.pri)
