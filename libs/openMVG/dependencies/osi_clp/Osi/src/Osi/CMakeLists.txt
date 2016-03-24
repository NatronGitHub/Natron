file(
	GLOB
	OSI_OSI_HEADER
	*.hpp
	*.h
)
file(
	GLOB
	OSI_OSI_CPP
	*.cpp
)

ADD_LIBRARY(lib_Osi ${OSI_OSI_HEADER} ${OSI_OSI_CPP})
INSTALL(TARGETS lib_Osi DESTINATION lib EXPORT openMVG-targets)
