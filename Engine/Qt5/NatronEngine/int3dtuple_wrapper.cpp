
// default includes
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
GCC_DIAG_OFF(uninitialized)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <pysidesignal.h>
#include <shiboken.h> // produces many warnings
#ifndef QT_NO_VERSION_TAGGING
#  define QT_NO_VERSION_TAGGING
#endif
#include <QDebug>
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <pysideqenum.h>
#include <feature_select.h>
QT_WARNING_DISABLE_DEPRECATED

#include <typeinfo>
#include <iterator>

// module include
#include "natronengine_python.h"

// main header
#include "int3dtuple_wrapper.h"

// inner classes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING

#include <cctype>
#include <cstring>



template <class T>
static const char *typeNameOf(const T &t)
{
    const char *typeName =  typeid(t).name();
    auto size = std::strlen(typeName);
#if defined(Q_CC_MSVC) // MSVC: "class QPaintDevice * __ptr64"
    if (auto lastStar = strchr(typeName, '*')) {
        // MSVC: "class QPaintDevice * __ptr64"
        while (*--lastStar == ' ') {
        }
        size = lastStar - typeName + 1;
    }
#else // g++, Clang: "QPaintDevice *" -> "P12QPaintDevice"
    if (size > 2 && typeName[0] == 'P' && std::isdigit(typeName[1])) {
        ++typeName;
        --size;
    }
#endif
    char *result = new char[size + 1];
    result[size] = '\0';
    memcpy(result, typeName, size);
    return result;
}


// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_Int3DTuple_Init(PyObject *self, PyObject *args, PyObject *kwds)
{
    SbkObject *sbkSelf = reinterpret_cast<SbkObject *>(self);
PySide::Feature::Select(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::Int3DTuple >()))
        return -1;

    ::Int3DTuple *cptr{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Int3DTuple.__init__";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // Int3DTuple()
            cptr = new ::Int3DTuple();
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::Int3DTuple >(), cptr)) {
        delete cptr;
        return -1;
    }
    Shiboken::Object::setValidCpp(sbkSelf, true);
    if (Shiboken::BindingManager::instance().hasWrapper(cptr)) {
        Shiboken::BindingManager::instance().releaseWrapper(Shiboken::BindingManager::instance().retrieveWrapper(cptr));
    }
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;
}


static const char *Sbk_Int3DTuple_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_Int3DTuple_methods[] = {

    {nullptr, nullptr} // Sentinel
};

PyObject* Sbk_Int3DTupleFunc___getitem__(PyObject *self, Py_ssize_t _i)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Int3DTuple *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    // Begin code injection
    if (_i < 0 || _i >= 3) {
    PyErr_BadArgument();
    return 0;
    } else {
    int ret;
    switch (_i) {
    case 0:
    ret = cppSelf->x;
    break;
    case 1:
    ret = cppSelf->y;
    break;
    case 2:
    ret = cppSelf->z;
    break;
    }
    return  Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &ret);
    }

    // End of code injection

}

static PyObject *Sbk_Int3DTuple_get_x(PyObject *self, void *)
{
    if (!Shiboken::Object::isValid(self))
        return nullptr;
    auto cppSelf = reinterpret_cast< ::Int3DTuple *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int cppOut_local = cppSelf->x;
    PyObject *pyOut = {};
    pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppOut_local);
    return pyOut;
}
static int Sbk_Int3DTuple_set_x(PyObject *self, PyObject *pyIn, void *)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    auto cppSelf = reinterpret_cast< ::Int3DTuple *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    if (pyIn == nullptr) {
        PyErr_SetString(PyExc_TypeError, "'x' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp{nullptr};
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'x', 'int' or convertible type expected");
        return -1;
    }

    int cppOut_local = cppSelf->x;
    pythonToCpp(pyIn, &cppOut_local);
    cppSelf->x = cppOut_local;

    return 0;
}

static PyObject *Sbk_Int3DTuple_get_y(PyObject *self, void *)
{
    if (!Shiboken::Object::isValid(self))
        return nullptr;
    auto cppSelf = reinterpret_cast< ::Int3DTuple *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int cppOut_local = cppSelf->y;
    PyObject *pyOut = {};
    pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppOut_local);
    return pyOut;
}
static int Sbk_Int3DTuple_set_y(PyObject *self, PyObject *pyIn, void *)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    auto cppSelf = reinterpret_cast< ::Int3DTuple *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    if (pyIn == nullptr) {
        PyErr_SetString(PyExc_TypeError, "'y' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp{nullptr};
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'y', 'int' or convertible type expected");
        return -1;
    }

    int cppOut_local = cppSelf->y;
    pythonToCpp(pyIn, &cppOut_local);
    cppSelf->y = cppOut_local;

    return 0;
}

static PyObject *Sbk_Int3DTuple_get_z(PyObject *self, void *)
{
    if (!Shiboken::Object::isValid(self))
        return nullptr;
    auto cppSelf = reinterpret_cast< ::Int3DTuple *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int cppOut_local = cppSelf->z;
    PyObject *pyOut = {};
    pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppOut_local);
    return pyOut;
}
static int Sbk_Int3DTuple_set_z(PyObject *self, PyObject *pyIn, void *)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    auto cppSelf = reinterpret_cast< ::Int3DTuple *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    if (pyIn == nullptr) {
        PyErr_SetString(PyExc_TypeError, "'z' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp{nullptr};
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'z', 'int' or convertible type expected");
        return -1;
    }

    int cppOut_local = cppSelf->z;
    pythonToCpp(pyIn, &cppOut_local);
    cppSelf->z = cppOut_local;

    return 0;
}

// Getters and Setters for Int3DTuple
static PyGetSetDef Sbk_Int3DTuple_getsetlist[] = {
    {const_cast<char *>("x"), Sbk_Int3DTuple_get_x, Sbk_Int3DTuple_set_x},
    {const_cast<char *>("y"), Sbk_Int3DTuple_get_y, Sbk_Int3DTuple_set_y},
    {const_cast<char *>("z"), Sbk_Int3DTuple_get_z, Sbk_Int3DTuple_set_z},
    {nullptr} // Sentinel
};

} // extern "C"

static int Sbk_Int3DTuple_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_Int3DTuple_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_Int3DTuple_Type = nullptr;
static SbkObjectType *Sbk_Int3DTuple_TypeF(void)
{
    return _Sbk_Int3DTuple_Type;
}

static PyType_Slot Sbk_Int3DTuple_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    nullptr},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_Int3DTuple_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_Int3DTuple_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_Int3DTuple_methods)},
    {Py_tp_getset,      reinterpret_cast<void *>(Sbk_Int3DTuple_getsetlist)},
    {Py_tp_init,        reinterpret_cast<void *>(Sbk_Int3DTuple_Init)},
    {Py_tp_new,         reinterpret_cast<void *>(SbkObjectTpNew)},
    // type supports sequence protocol
    {Py_sq_item, (void *)&Sbk_Int3DTupleFunc___getitem__},
    {0, nullptr}
};
static PyType_Spec Sbk_Int3DTuple_spec = {
    "1:NatronEngine.Int3DTuple",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_Int3DTuple_slots
};

} //extern "C"


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void Int3DTuple_PythonToCpp_Int3DTuple_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_Int3DTuple_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_Int3DTuple_PythonToCpp_Int3DTuple_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_Int3DTuple_TypeF())))
        return Int3DTuple_PythonToCpp_Int3DTuple_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *Int3DTuple_PTR_CppToPython_Int3DTuple(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::Int3DTuple *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_Int3DTuple_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *Int3DTuple_SignatureStrings[] = {
    "NatronEngine.Int3DTuple(self)",
    nullptr}; // Sentinel

void init_Int3DTuple(PyObject *module)
{
    _Sbk_Int3DTuple_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "Int3DTuple",
        "Int3DTuple*",
        &Sbk_Int3DTuple_spec,
        &Shiboken::callCppDestructor< ::Int3DTuple >,
        0,
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_Int3DTuple_Type);
    InitSignatureStrings(pyType, Int3DTuple_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_Int3DTuple_Type), Sbk_Int3DTuple_PropertyStrings);
    SbkNatronEngineTypes[SBK_INT3DTUPLE_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_Int3DTuple_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_Int3DTuple_TypeF(),
        Int3DTuple_PythonToCpp_Int3DTuple_PTR,
        is_Int3DTuple_PythonToCpp_Int3DTuple_PTR_Convertible,
        Int3DTuple_PTR_CppToPython_Int3DTuple);

    Shiboken::Conversions::registerConverterName(converter, "Int3DTuple");
    Shiboken::Conversions::registerConverterName(converter, "Int3DTuple*");
    Shiboken::Conversions::registerConverterName(converter, "Int3DTuple&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::Int3DTuple).name());


}
