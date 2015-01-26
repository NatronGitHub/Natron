#!/bin/bash

# To be run after shiboken to fix errors

sed -e '/<destroylistener.h>/d' -i .bak Engine/NatronEngine/*.cpp  || exit 1
sed -e '/<destroylistener.h>/d' -i .bak Engine/NatronGui/*.cpp  || exit 1
sed -e '/SbkPySide_QtCoreTypes;/d' -i .bak Gui/NatronGui/natrongui_module_wrapper.cpp || exit 1
sed -e '/SbkPySide_QtCoreTypeConverters;/d' -i .bak Gui/NatronGui/natrongui_module_wrapper.cpp  || exit 1
sed -e '/SbkNatronEngineTypes;/d' -i .bak Gui/NatronGui/natrongui_module_wrapper.cpp  || exit 1
sed -e '/SbkNatronEngineTypeConverters;/d' -i .bak Gui/NatronGui/natrongui_module_wrapper.cpp  || exit 1
sed -e 's/cleanTypesAttributes/cleanGuiTypesAttributes/g' -i .bak Gui/NatronGui/natrongui_module_wrapper.cpp  || exit 1
cd Gui/NatronGui && rm *.bak && cd ../.. && cd Engine/NatronEngine && rm *.bak && cd ../..  || exit 1
