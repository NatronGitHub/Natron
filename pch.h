#ifndef PCH_H
#define PCH_H

#include <vector>
#include <string>
#include <map>

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include "Global/Macros.h"

// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
//        static T cast(U u){
CLANG_DIAG_OFF(unused-parameter);
#include <boost/serialization/smart_cast.hpp>
CLANG_DIAG_ON(unused-parameter);

#if QT_VERSION < 0x050000
CLANG_DIAG_OFF(unused-private-field);
#include <QtGui/qmime.h>
CLANG_DIAG_ON(unused-private-field);
#endif

#include <QtCore>

#endif // PCH_H
