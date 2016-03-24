#==============================================================================                                                                                                                                                                                             

PROJECT(COIN_OR_OSI_CLP C CXX)

# CMake version control
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# static libraries only
SET(BUILD_SHARED_LIBS OFF)

# use PIC code for link into shared lib
IF(UNIX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
ENDIF(UNIX)

################
INCLUDE_DIRECTORIES(.
  ./CoinUtils/src/
  ./Osi/src/Osi/
  ./Clp/src/
  ./Clp/src/OsiClp/
  #Clp/test/
  #Osi/src/OsiCommonTest/
)

# optimise gcc
IF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  ADD_DEFINITIONS(-O3 -Wall -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -ffast-math)
ENDIF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)

IF(MSVC)
  ADD_DEFINITIONS(
    -D_CRT_SECURE_NO_WARNINGS
    -D_CRT_SECURE_NO_DEPRECATE
    #-DSAMPLEDIR="..\..\..\..\Data\Sample\\"
  )
ENDIF(MSVC)

ADD_SUBDIRECTORY(CoinUtils)
SET_PROPERTY(TARGET lib_CoinUtils PROPERTY FOLDER OpenMVG/3rdParty/osi_clp)

ADD_SUBDIRECTORY(Osi)
SET_PROPERTY(TARGET lib_Osi PROPERTY FOLDER OpenMVG/3rdParty/osi_clp)

ADD_SUBDIRECTORY(Clp)
SET_PROPERTY(TARGET lib_OsiClpSolver PROPERTY FOLDER OpenMVG/3rdParty/osi_clp)
SET_PROPERTY(TARGET lib_clp PROPERTY FOLDER OpenMVG/3rdParty/osi_clp)

