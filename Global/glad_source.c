#include "Global/Macros.h"

GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-prototypes)
GCC_DIAG_OFF(unused-variable)

#ifdef DEBUG
#include "gladDeb/src/glad.c"
#else
#include "gladRel/src/glad.c"
#endif
