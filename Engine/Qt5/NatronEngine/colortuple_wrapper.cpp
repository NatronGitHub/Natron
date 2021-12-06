
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
#include <qapp_macro.h>

QT_WARNING_DISABLE_DEPRECATED

#include <typeinfo>
#include <iterator>

// module include
#include "natronengine_python.h"

// main header
#include "colortuple_wrapper.h"

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
Sbk_ColorTuple_Init(PyObject *self, PyObject *args, PyObject *kwds)
{
    SbkObject *sbkSelf = reinterpret_cast<SbkObject *>(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::ColorTuple >()))
        return -1;

    ::ColorTuple *cptr{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // ColorTuple()
            cptr = new ::ColorTuple();
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::ColorTuple >(), cptr)) {
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


static const char *Sbk_ColorTuple_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_ColorTuple_methods[] = {

    {nullptr, nullptr} // Sentinel
};

PyObject* Sbk_ColorTupleFunc___getitem__(PyObject *self, Py_ssize_t _i)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::ColorTuple *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    // Begin code injection
    if (_i < 0 || _i >= 4) {
        PyErr_BadArgument();
        return 0;
    } else {
        double ret;
        switch (_i) {
        case 0:
            ret = cppSelf->r;
            break;
        case 1:
            ret = cppSelf->g;
            break;
        case 2:
            ret = cppSelf->b;
            break;
        case 3:
            ret = cppSelf->a;
            break;

        }
        return  Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &ret);
    }

    // End of code injection

}

static PyObject *Sbk_ColorTuple_get_r(PyObject *self, void *)
{
    if (!Shiboken::Object::isValid(self))
        return nullptr;
    auto cppSelf = reinterpret_cast< ::ColorTuple *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyOut = {};
    pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->r);
    return pyOut;
}
static int Sbk_ColorTuple_set_r(PyObject *self, PyObject *pyIn, void *)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    auto cppSelf = reinterpret_cast< ::ColorTuple *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    if (pyIn == nullptr) {
        PyErr_SetString(PyExc_TypeError, "'r' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp{nullptr};
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'r', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->r;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

static PyObject *Sbk_ColorTuple_get_g(PyObject *self, void *)
{
    if (!Shiboken::Object::isValid(self))
        return nullptr;
    auto cppSelf = reinterpret_cast< ::ColorTuple *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyOut = {};
    pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->g);
    return pyOut;
}
static int Sbk_ColorTuple_set_g(PyObject *self, PyObject *pyIn, void *)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    auto cppSelf = reinterpret_cast< ::ColorTuple *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    if (pyIn == nullptr) {
        PyErr_SetString(PyExc_TypeError, "'g' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp{nullptr};
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'g', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->g;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

static PyObject *Sbk_ColorTuple_get_b(PyObject *self, void *)
{
    if (!Shiboken::Object::isValid(self))
        return nullptr;
    auto cppSelf = reinterpret_cast< ::ColorTuple *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyOut = {};
    pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->b);
    return pyOut;
}
static int Sbk_ColorTuple_set_b(PyObject *self, PyObject *pyIn, void *)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    auto cppSelf = reinterpret_cast< ::ColorTuple *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    if (pyIn == nullptr) {
        PyErr_SetString(PyExc_TypeError, "'b' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp{nullptr};
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'b', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->b;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

static PyObject *Sbk_ColorTuple_get_a(PyObject *self, void *)
{
    if (!Shiboken::Object::isValid(self))
        return nullptr;
    auto cppSelf = reinterpret_cast< ::ColorTuple *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyOut = {};
    pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->a);
    return pyOut;
}
static int Sbk_ColorTuple_set_a(PyObject *self, PyObject *pyIn, void *)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    auto cppSelf = reinterpret_cast< ::ColorTuple *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    if (pyIn == nullptr) {
        PyErr_SetString(PyExc_TypeError, "'a' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp{nullptr};
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'a', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->a;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

// Getters and Setters for ColorTuple
static PyGetSetDef Sbk_ColorTuple_getsetlist[] = {
    {const_cast<char *>("r"), Sbk_ColorTuple_get_r, Sbk_ColorTuple_set_r},
    {const_cast<char *>("g"), Sbk_ColorTuple_get_g, Sbk_ColorTuple_set_g},
    {const_cast<char *>("b"), Sbk_ColorTuple_get_b, Sbk_ColorTuple_set_b},
    {const_cast<char *>("a"), Sbk_ColorTuple_get_a, Sbk_ColorTuple_set_a},
    {nullptr} // Sentinel
};

} // extern "C"

static int Sbk_ColorTuple_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_ColorTuple_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_ColorTuple_Type = nullptr;
static SbkObjectType *Sbk_ColorTuple_TypeF(void)
{
    return _Sbk_ColorTuple_Type;
}

static PyType_Slot Sbk_ColorTuple_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    nullptr},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_ColorTuple_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_ColorTuple_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_ColorTuple_methods)},
    {Py_tp_getset,      reinterpret_cast<void *>(Sbk_ColorTuple_getsetlist)},
    {Py_tp_init,        reinterpret_cast<void *>(Sbk_ColorTuple_Init)},
    {Py_tp_new,         reinterpret_cast<void *>(SbkObjectTpNew)},
    // type supports sequence protocol
    {Py_sq_item, (void *)&Sbk_ColorTupleFunc___getitem__},
    {0, nullptr}
};
static PyType_Spec Sbk_ColorTuple_spec = {
    "1:NatronEngine.ColorTuple",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_ColorTuple_slots
};

} //extern "C"


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void ColorTuple_PythonToCpp_ColorTuple_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_ColorTuple_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_ColorTuple_PythonToCpp_ColorTuple_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_ColorTuple_TypeF())))
        return ColorTuple_PythonToCpp_ColorTuple_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *ColorTuple_PTR_CppToPython_ColorTuple(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::ColorTuple *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_ColorTuple_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *ColorTuple_SignatureStrings[] = {
    "NatronEngine.ColorTuple(self)",
    nullptr}; // Sentinel

void init_ColorTuple(PyObject *module)
{
    _Sbk_ColorTuple_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "ColorTuple",
        "ColorTuple*",
        &Sbk_ColorTuple_spec,
        &Shiboken::callCppDestructor< ::ColorTuple >,
        0,
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_ColorTuple_Type);
    InitSignatureStrings(pyType, ColorTuple_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_ColorTuple_Type), Sbk_ColorTuple_PropertyStrings);
    SbkNatronEngineTypes[SBK_COLORTUPLE_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_ColorTuple_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_ColorTuple_TypeF(),
        ColorTuple_PythonToCpp_ColorTuple_PTR,
        is_ColorTuple_PythonToCpp_ColorTuple_PTR_Convertible,
        ColorTuple_PTR_CppToPython_ColorTuple);

    Shiboken::Conversions::registerConverterName(converter, "ColorTuple");
    Shiboken::Conversions::registerConverterName(converter, "ColorTuple*");
    Shiboken::Conversions::registerConverterName(converter, "ColorTuple&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::ColorTuple).name());



}
