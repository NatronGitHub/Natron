#ifndef QKEYCHAIN_EXPORT_H
#define QKEYCHAIN_EXPORT_H

#include <qglobal.h>

# ifdef QKEYCHAIN_STATICLIB
#  undef QKEYCHAIN_SHAREDLIB
#  define QKEYCHAIN_EXPORT
# else
#  ifdef QKEYCHAIN_BUILD_QKEYCHAIN_LIB
#   define QKEYCHAIN_EXPORT Q_DECL_EXPORT
#  else
#   define QKEYCHAIN_EXPORT Q_DECL_IMPORT
#  endif
# endif

#endif
