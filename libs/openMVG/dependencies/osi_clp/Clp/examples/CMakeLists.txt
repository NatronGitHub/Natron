
file(
	GLOB
	CLP_EXAMPLES_HEADER
	*.hpp
	*.h
)
file(
	GLOB
	CLP_EXAMPLES_CPP
	*.cpp
	*.c
)


file(
     GLOB
     REMOVE_CPP
      ClpSolve.cpp
      useVolume.cpp
)   

#file(
#     GLOB
#    REMOVE_HEADER#
#
#)   

#list(REMOVE_ITEM CLP_SRC_HEADER ${REMOVE_HEADER})
list(REMOVE_ITEM CLP_EXAMPLES_CPP ${REMOVE_CPP})

#ADD_LIBRARY(lib_examples ${CLP_EXAMPLES_HEADER} ${CLP_EXAMPLES_CPP})
#TARGET_LINK_LIBRARIES( )

#file( GLOB APP_SOURCES app/*.cxx )
#foreach( appsourcefile ${APP_SOURCES} )
#    string( REPLACE ".cpp" "" testname ${testsourcefile} )
#    add_executable( ${testname} ${testsourcefile} )
#    target_link_libraries( ${testname} lib_examples )
#endforeach( appsourcefile ${APP_SOURCES} )



add_executable( defaults defaults.cpp )
TARGET_LINK_LIBRARIES( defaults lib_Osi lib_clp lib_CoinUtils)

add_executable( driver2 driver2.cpp )
TARGET_LINK_LIBRARIES( driver2 lib_Osi lib_clp lib_CoinUtils)

add_executable( sprint2 sprint2.cpp )
TARGET_LINK_LIBRARIES(  sprint2 lib_Osi lib_clp lib_CoinUtils)

add_executable( testGub testGub.cpp )
TARGET_LINK_LIBRARIES( testGub lib_Osi lib_clp lib_CoinUtils)

add_executable( network network.cpp )
TARGET_LINK_LIBRARIES( network lib_Osi lib_clp lib_CoinUtils)

#add_executable( useVolume useVolume.cpp )
#TARGET_LINK_LIBRARIES( useVolume lib_Osi lib_clp lib_CoinUtils)

add_executable( testBasis testBasis.cpp )
TARGET_LINK_LIBRARIES(  testBasis lib_Osi lib_clp lib_CoinUtils)

add_executable( minimum minimum.cpp )
TARGET_LINK_LIBRARIES( minimum lib_Osi lib_clp lib_CoinUtils)

add_executable( driver driver.cpp )
TARGET_LINK_LIBRARIES( driver lib_Osi lib_clp lib_CoinUtils)

#add_executable( testQP testQP.cpp )
#TARGET_LINK_LIBRARIES( testQP lib_Osi lib_clp lib_CoinUtils)

add_executable( decomp2 decomp2.cpp )
TARGET_LINK_LIBRARIES( decomp2 lib_Osi lib_clp lib_CoinUtils)

add_executable( decompose decompose.cpp )
TARGET_LINK_LIBRARIES( decompose lib_Osi lib_clp lib_CoinUtils)

add_executable( makeDual makeDual.cpp )
TARGET_LINK_LIBRARIES( makeDual lib_Osi lib_clp lib_CoinUtils)

add_executable( piece piece.cpp )
TARGET_LINK_LIBRARIES( piece lib_Osi lib_clp lib_CoinUtils)

add_executable( modify modify.cpp )
TARGET_LINK_LIBRARIES( modify lib_Osi lib_clp lib_CoinUtils)

add_executable( sprint sprint.cpp )
TARGET_LINK_LIBRARIES( sprint lib_Osi lib_clp lib_CoinUtils)

add_executable( decomp3 decomp3.cpp )
TARGET_LINK_LIBRARIES( decomp3 lib_Osi lib_clp lib_CoinUtils)

add_executable( testGub2 testGub2.cpp )
TARGET_LINK_LIBRARIES( testGub2 lib_Osi lib_clp lib_CoinUtils)

add_executable( addBits addBits.cpp )
TARGET_LINK_LIBRARIES( addBits lib_Osi lib_clp lib_CoinUtils)

add_executable( dualCuts dualCuts.cpp )
TARGET_LINK_LIBRARIES( dualCuts lib_Osi lib_clp lib_CoinUtils)

add_executable( testBarrier testBarrier.cpp )
TARGET_LINK_LIBRARIES( testBarrier lib_Osi lib_clp lib_CoinUtils)

add_executable( driverC driverC.c )
TARGET_LINK_LIBRARIES( driverC lib_Osi lib_clp lib_CoinUtils)

add_executable( pdco pdco.cpp )
TARGET_LINK_LIBRARIES( pdco lib_Osi lib_clp lib_CoinUtils)

add_executable( hello hello.cpp )
TARGET_LINK_LIBRARIES( hello lib_Osi lib_clp lib_CoinUtils)

add_executable( addRows addRows.cpp )
TARGET_LINK_LIBRARIES( addRows lib_Osi lib_clp lib_CoinUtils)

add_executable( addColumns addColumns.cpp )
TARGET_LINK_LIBRARIES( addColumns lib_Osi lib_clp lib_CoinUtils)

add_executable( rowColumn rowColumn.cpp )
TARGET_LINK_LIBRARIES( rowColumn lib_Osi lib_clp lib_CoinUtils)

#add_executable( ekk ekk.cpp )
#TARGET_LINK_LIBRARIES( ekk lib_Osi lib_clp lib_CoinUtils)

