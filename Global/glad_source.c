#include "Global/Macros.h"

GCC_DIAG_OFF(unused-parameter)

#ifdef DEBUG
#include "gladDeb/src/glad.c"
#else
#include "gladRel/src/glad.c"
#endif