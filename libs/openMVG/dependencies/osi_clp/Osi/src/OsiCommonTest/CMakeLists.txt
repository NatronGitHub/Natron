file(
	GLOB
	OSI_TEST_CPP
	*.cpp
)

file(
	GLOB
	OSI_TEST_HEADER
	*.hpp
	*.h
)

ADD_LIBRARY(lib_OsiCommonTest ${OSI_TEST_CPP} ${OSI_TEST_HEADER})
TARGET_LINK_LIBRARIES(lib_OsiCommonTest lib_Osi lib_CoinUtils)
