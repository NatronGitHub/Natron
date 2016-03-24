
file(
	GLOB
	CLP_TEST_CPP
	*.cpp
)

ADD_EXECUTABLE( osiUnitTest ${CLP_TEST_CPP})
TARGET_LINK_LIBRARIES( osiUnitTest 
		       lib_CoinUtils 
		       lib_OsiCommonTest 
		       lib_Osi
		       lib_OsiClpSolver 
		       lib_clp)