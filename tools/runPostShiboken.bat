
:: To be run after shiboken to fix errors

sed "/<destroylistener.h>/d" -i Engine/NatronEngine/*.cpp
sed "/<destroylistener.h>/d" -i Gui/NatronGui/*.cpp
sed -e "/SbkPySide_QtCoreTypes;/d" -i Gui/NatronGui/natrongui_module_wrapper.cpp
sed -e "/SbkPySide_QtCoreTypeConverters;/d" -i  Gui/NatronGui/natrongui_module_wrapper.cpp
sed -e "/SbkNatronEngineTypes;/d" -i Gui/NatronGui/natrongui_module_wrapper.cpp
sed -e "/SbkNatronEngineTypeConverters;/d" -i Gui/NatronGui/natrongui_module_wrapper.cpp
sed -e "s/cleanTypesAttributes/cleanGuiTypesAttributes/g" -i Gui/NatronGui/natrongui_module_wrapper.cpp


rm sed*
rm */sed*