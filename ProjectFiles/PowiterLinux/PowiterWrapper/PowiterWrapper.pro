TEMPLATE = subdirs
CONFIG+=ordered
SUBDIRS += \
    PowiterLib \
    Powiter
Powiter.depends = PowiterLib
