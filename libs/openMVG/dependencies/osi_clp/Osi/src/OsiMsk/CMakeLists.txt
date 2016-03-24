file(
	GLOB
	OsiMskSolverInterface_CPP
	*.cpp
)

file(
	GLOB
	OsiMskSolverInterface_HEADER
	*.hpp
	*.h
)

IF (EXISTS "${MOSEK_INCLUDE}" AND EXISTS "${MOSEK_LIB}")
  INCLUDE_DIRECTORIES( ${MOSEK_INCLUDE} )
  ADD_LIBRARY(lib_Osi_Msk ${OsiMskSolverInterface_CPP}  ${OsiMskSolverInterface_HEADER})
ENDIF (EXISTS "${MOSEK_INCLUDE}" AND EXISTS "${MOSEK_LIB}")
