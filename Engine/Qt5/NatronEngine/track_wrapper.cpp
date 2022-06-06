
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
#include "track_wrapper.h"

// inner classes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING

// Extra includes
#include <PyParameter.h>
#include <list>


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
static PyObject *Sbk_TrackFunc_getParam(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Track *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_TRACK_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Track.getParam";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Track::getParam(QString)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // getParam(QString)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_TrackFunc_getParam_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getParam(QString)const
            Param * cppResult = const_cast<const ::Track *>(cppSelf)->getParam(cppArg0);
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PARAM_IDX]), cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_TrackFunc_getParam_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_TrackFunc_getParams(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Track *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_TRACK_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Track.getParams";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getParams()const
            // Begin code injection
            std::list<Param*> params = cppSelf->getParams();
            PyObject* ret = PyList_New((int) params.size());
            int idx = 0;
            for (std::list<Param*>::iterator it = params.begin(); it!=params.end(); ++it,++idx) {
            PyObject* item = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PARAM_IDX]), *it);
            // Ownership transferences.
            Shiboken::Object::getOwnership(item);
            PyList_SET_ITEM(ret, idx, item);
            }
            return ret;

            // End of code injection

        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_TrackFunc_getScriptName(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Track *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_TRACK_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Track.getScriptName";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getScriptName()const
            QString cppResult = const_cast<const ::Track *>(cppSelf)->getScriptName();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_TrackFunc_reset(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Track *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_TRACK_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Track.reset";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // reset()
            cppSelf->reset();
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;
}

static PyObject *Sbk_TrackFunc_setScriptName(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Track *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_TRACK_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Track.setScriptName";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Track::setScriptName(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // setScriptName(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_TrackFunc_setScriptName_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setScriptName(QString)
            cppSelf->setScriptName(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_TrackFunc_setScriptName_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}


static const char *Sbk_Track_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_Track_methods[] = {
    {"getParam", reinterpret_cast<PyCFunction>(Sbk_TrackFunc_getParam), METH_O},
    {"getParams", reinterpret_cast<PyCFunction>(Sbk_TrackFunc_getParams), METH_NOARGS},
    {"getScriptName", reinterpret_cast<PyCFunction>(Sbk_TrackFunc_getScriptName), METH_NOARGS},
    {"reset", reinterpret_cast<PyCFunction>(Sbk_TrackFunc_reset), METH_NOARGS},
    {"setScriptName", reinterpret_cast<PyCFunction>(Sbk_TrackFunc_setScriptName), METH_O},

    {nullptr, nullptr} // Sentinel
};

} // extern "C"

static int Sbk_Track_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_Track_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_Track_Type = nullptr;
static SbkObjectType *Sbk_Track_TypeF(void)
{
    return _Sbk_Track_Type;
}

static PyType_Slot Sbk_Track_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    nullptr},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_Track_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_Track_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_Track_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_Track_spec = {
    "1:NatronEngine.Track",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_Track_slots
};

} //extern "C"


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void Track_PythonToCpp_Track_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_Track_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_Track_PythonToCpp_Track_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_Track_TypeF())))
        return Track_PythonToCpp_Track_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *Track_PTR_CppToPython_Track(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::Track *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_Track_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *Track_SignatureStrings[] = {
    "NatronEngine.Track.getParam(self,scriptName:QString)->NatronEngine.Param",
    "NatronEngine.Track.getParams(self)->std.list[NatronEngine.Param]",
    "NatronEngine.Track.getScriptName(self)->QString",
    "NatronEngine.Track.reset(self)",
    "NatronEngine.Track.setScriptName(self,scriptName:QString)",
    nullptr}; // Sentinel

void init_Track(PyObject *module)
{
    _Sbk_Track_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "Track",
        "Track*",
        &Sbk_Track_spec,
        &Shiboken::callCppDestructor< ::Track >,
        0,
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_Track_Type);
    InitSignatureStrings(pyType, Track_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_Track_Type), Sbk_Track_PropertyStrings);
    SbkNatronEngineTypes[SBK_TRACK_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_Track_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_Track_TypeF(),
        Track_PythonToCpp_Track_PTR,
        is_Track_PythonToCpp_Track_PTR_Convertible,
        Track_PTR_CppToPython_Track);

    Shiboken::Conversions::registerConverterName(converter, "Track");
    Shiboken::Conversions::registerConverterName(converter, "Track*");
    Shiboken::Conversions::registerConverterName(converter, "Track&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::Track).name());


}
