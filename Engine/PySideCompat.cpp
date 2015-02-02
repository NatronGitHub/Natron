#ifdef PYSIDE_OLD

// Compatibility function for pyside versions before this commit:
// https://qt.gitorious.org/pyside/pyside/commit/b3669dca4e4321b204d10b06018d35900b1847ee

#include <pyside.h>

#include "Global/Macros.hpp"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
#include <basewrapper.h>
#include <conversions.h>
#include <sbkconverter.h>
#include <gilstate.h>
#include <typeresolver.h>
#include <bindingmanager.h>
CLANG_DIAG_ON(mismatched-tags)
GCC_DIAG_ON(unused-parameter)
GCC_DIAG_ON(missing-field-initializers)

#include <algorithm>
#include <cstring>
#include <cctype>
#include <QStack>
#include <QCoreApplication>
#include <QDebug>
#include <QSharedPointer>

// A QSharedPointer is used with a deletion function to invalidate a pointer
// when the property value is cleared. This should be a QSharedPointer with
// a void* pointer, but that isn't allowed
typedef char any_t;
Q_DECLARE_METATYPE(QSharedPointer<any_t>);

namespace PySide
{

    static void invalidatePtr(any_t* object)
    {
        Shiboken::GilState state;

        SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(object);
        if (wrapper != NULL)
            Shiboken::BindingManager::instance().releaseWrapper(wrapper);
    }

    static const char invalidatePropertyName[] = "_PySideInvalidatePtr";

    PyObject* getWrapperForQObject(QObject* cppSelf, SbkObjectType* sbk_type)
    {
        PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppSelf);
        if (pyOut) {
            Py_INCREF(pyOut);
            return pyOut;
        }

        // Setting the property will trigger an QEvent notification, which may call into
        // code that creates the wrapper so only set the property if it isn't already
        // set and check if it's created after the set call
        QVariant existing = cppSelf->property(invalidatePropertyName);
        if (!existing.isValid()) {
            QSharedPointer<any_t> shared_with_del((any_t*)cppSelf, invalidatePtr);
            cppSelf->setProperty(invalidatePropertyName, QVariant::fromValue(shared_with_del));
            pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppSelf);
            if (pyOut) {
                Py_INCREF(pyOut);
                return pyOut;
            }
        }
        
        const char* typeName = typeid(*cppSelf).name();
        pyOut = Shiboken::Object::newObject(sbk_type, cppSelf, false, false, typeName);
        
        return pyOut;
    }
} //namespace PySide
#endif
