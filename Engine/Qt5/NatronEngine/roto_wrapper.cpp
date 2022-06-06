
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
#include "roto_wrapper.h"

// inner classes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING

// Extra includes
#include <PyRoto.h>


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
static PyObject *Sbk_RotoFunc_createBezier(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Roto *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ROTO_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Roto.createBezier";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "createBezier", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return {};


    // Overloaded function decisor
    // 0: Roto::createBezier(double,double,double)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
        overloadId = 0; // createBezier(double,double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RotoFunc_createBezier_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // createBezier(double,double,double)
            // Begin code injection
            BezierCurve * cppResult = cppSelf->createBezier(cppArg0,cppArg1,cppArg2);
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX]), cppResult);

            // End of code injection


            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_RotoFunc_createBezier_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RotoFunc_createEllipse(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Roto *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ROTO_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Roto.createEllipse";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "createEllipse", 5, 5, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3]), &(pyArgs[4])))
        return {};


    // Overloaded function decisor
    // 0: Roto::createEllipse(double,double,double,bool,double)
    if (numArgs == 5
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[3])))
        && (pythonToCpp[4] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[4])))) {
        overloadId = 0; // createEllipse(double,double,double,bool,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RotoFunc_createEllipse_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);
        bool cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);
        double cppArg4;
        pythonToCpp[4](pyArgs[4], &cppArg4);

        if (!PyErr_Occurred()) {
            // createEllipse(double,double,double,bool,double)
            // Begin code injection
            BezierCurve * cppResult = cppSelf->createEllipse(cppArg0,cppArg1,cppArg2,cppArg3,cppArg4);
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX]), cppResult);

            // End of code injection


            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_RotoFunc_createEllipse_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RotoFunc_createLayer(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Roto *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ROTO_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Roto.createLayer";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // createLayer()
            // Begin code injection
            Layer * cppResult = cppSelf->createLayer();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_LAYER_IDX]), cppResult);

            // End of code injection


            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_RotoFunc_createRectangle(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Roto *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ROTO_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Roto.createRectangle";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "createRectangle", 4, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: Roto::createRectangle(double,double,double,double)
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        overloadId = 0; // createRectangle(double,double,double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RotoFunc_createRectangle_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);
        double cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // createRectangle(double,double,double,double)
            // Begin code injection
            BezierCurve * cppResult = cppSelf->createRectangle(cppArg0,cppArg1,cppArg2,cppArg3);
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_BEZIERCURVE_IDX]), cppResult);

            // End of code injection


            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_RotoFunc_createRectangle_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RotoFunc_getBaseLayer(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Roto *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ROTO_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Roto.getBaseLayer";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getBaseLayer()const
            Layer * cppResult = const_cast<const ::Roto *>(cppSelf)->getBaseLayer();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_LAYER_IDX]), cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_RotoFunc_getItemByName(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Roto *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ROTO_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Roto.getItemByName";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Roto::getItemByName(QString)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // getItemByName(QString)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RotoFunc_getItemByName_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getItemByName(QString)const
            ItemBase * cppResult = const_cast<const ::Roto *>(cppSelf)->getItemByName(cppArg0);
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_ITEMBASE_IDX]), cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_RotoFunc_getItemByName_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}


static const char *Sbk_Roto_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_Roto_methods[] = {
    {"createBezier", reinterpret_cast<PyCFunction>(Sbk_RotoFunc_createBezier), METH_VARARGS},
    {"createEllipse", reinterpret_cast<PyCFunction>(Sbk_RotoFunc_createEllipse), METH_VARARGS},
    {"createLayer", reinterpret_cast<PyCFunction>(Sbk_RotoFunc_createLayer), METH_NOARGS},
    {"createRectangle", reinterpret_cast<PyCFunction>(Sbk_RotoFunc_createRectangle), METH_VARARGS},
    {"getBaseLayer", reinterpret_cast<PyCFunction>(Sbk_RotoFunc_getBaseLayer), METH_NOARGS},
    {"getItemByName", reinterpret_cast<PyCFunction>(Sbk_RotoFunc_getItemByName), METH_O},

    {nullptr, nullptr} // Sentinel
};

} // extern "C"

static int Sbk_Roto_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_Roto_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_Roto_Type = nullptr;
static SbkObjectType *Sbk_Roto_TypeF(void)
{
    return _Sbk_Roto_Type;
}

static PyType_Slot Sbk_Roto_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    nullptr},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_Roto_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_Roto_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_Roto_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_Roto_spec = {
    "1:NatronEngine.Roto",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_Roto_slots
};

} //extern "C"


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void Roto_PythonToCpp_Roto_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_Roto_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_Roto_PythonToCpp_Roto_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_Roto_TypeF())))
        return Roto_PythonToCpp_Roto_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *Roto_PTR_CppToPython_Roto(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::Roto *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_Roto_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *Roto_SignatureStrings[] = {
    "NatronEngine.Roto.createBezier(self,x:double,y:double,time:double)->NatronEngine.BezierCurve",
    "NatronEngine.Roto.createEllipse(self,x:double,y:double,diameter:double,fromCenter:bool,time:double)->NatronEngine.BezierCurve",
    "NatronEngine.Roto.createLayer(self)->NatronEngine.Layer",
    "NatronEngine.Roto.createRectangle(self,x:double,y:double,size:double,time:double)->NatronEngine.BezierCurve",
    "NatronEngine.Roto.getBaseLayer(self)->NatronEngine.Layer",
    "NatronEngine.Roto.getItemByName(self,name:QString)->NatronEngine.ItemBase",
    nullptr}; // Sentinel

void init_Roto(PyObject *module)
{
    _Sbk_Roto_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "Roto",
        "Roto*",
        &Sbk_Roto_spec,
        &Shiboken::callCppDestructor< ::Roto >,
        0,
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_Roto_Type);
    InitSignatureStrings(pyType, Roto_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_Roto_Type), Sbk_Roto_PropertyStrings);
    SbkNatronEngineTypes[SBK_ROTO_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_Roto_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_Roto_TypeF(),
        Roto_PythonToCpp_Roto_PTR,
        is_Roto_PythonToCpp_Roto_PTR_Convertible,
        Roto_PTR_CppToPython_Roto);

    Shiboken::Conversions::registerConverterName(converter, "Roto");
    Shiboken::Conversions::registerConverterName(converter, "Roto*");
    Shiboken::Conversions::registerConverterName(converter, "Roto&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::Roto).name());


}
