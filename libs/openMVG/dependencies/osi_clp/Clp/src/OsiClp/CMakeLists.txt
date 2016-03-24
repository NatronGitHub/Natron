file(
	GLOB_RECURSE
	OSI_HEADERS
	*.hpp
	*.h
)
file(
	GLOB_RECURSE
	OSI_SRCS
	*.cpp
)
ADD_LIBRARY(lib_OsiClpSolver ${OSI_HEADERS} ${OSI_SRCS})
INSTALL(TARGETS lib_OsiClpSolver DESTINATION lib EXPORT openMVG-targets)
