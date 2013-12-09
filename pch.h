// -*-c++-*-
#ifndef PCH_H
#define PCH_H

#include <vector>
#include <string>
#include <map>

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

#include "Global/Macros.h"

// /usr/include/qt5/QtCore/qgenericatomic.h:177:13: warning: 'register' storage class specifier is deprecated [-Wdeprecated]
//             register T tmp = load(_q_value);
//             ^~~~~~~~~
// /usr/include/qt5/QtCore/qmetatype.h:688:5: warning: 'register' storage class specifier is deprecated [-Wdeprecated]
//     register int id = qMetaTypeId<T>();
//     ^~~~~~~~~
// /usr/include/qt5/QtCore/qsharedpointer_impl.h:481:13: warning: 'register' storage class specifier is deprecated [-Wdeprecated]
//            register int tmp = o->strongref.load();
//            ^~~~~~~~~
#if QT_VERSION >= 0x050000
CLANG_DIAG_OFF(deprecated);
#include <QtCore/qgenericatomic.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qsharedpointer_impl.h>
CLANG_DIAG_ON(deprecated);
#endif

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
