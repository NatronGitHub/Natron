
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
#include "tracker_wrapper.h"

// inner classes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING

// Extra includes
#include <PyTracker.h>
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
static PyObject *Sbk_TrackerFunc_createTrack(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Tracker *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_TRACKER_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // createTrack()
            Track * cppResult = cppSelf->createTrack();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_TRACK_IDX]), cppResult);

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

static PyObject *Sbk_TrackerFunc_getAllTracks(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Tracker *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_TRACKER_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getAllTracks(std::list<Track*>*)const
            // Begin code injection
            std::list<Track*> tracks;
            cppSelf->getAllTracks(&tracks);
            PyObject* ret = PyList_New((int) tracks.size());
            int idx = 0;
            for (std::list<Track*>::iterator it = tracks.begin(); it!=tracks.end(); ++it,++idx) {
            PyObject* item = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_TRACK_IDX]), *it);
            // Ownership transferences.
            Shiboken::Object::getOwnership(item);
            PyList_SET_ITEM(ret, idx, item);
            }
            return ret;

            // End of code injection

            pyResult = Py_None;
            Py_INCREF(Py_None);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_TrackerFunc_getSelectedTracks(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Tracker *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_TRACKER_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getSelectedTracks(std::list<Track*>*)const
            // Begin code injection
            std::list<Track*> tracks;
            cppSelf->getSelectedTracks(&tracks);
            PyObject* ret = PyList_New((int) tracks.size());
            int idx = 0;
            for (std::list<Track*>::iterator it = tracks.begin(); it!=tracks.end(); ++it,++idx) {
            PyObject* item = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_TRACK_IDX]), *it);
            // Ownership transferences.
            Shiboken::Object::getOwnership(item);
            PyList_SET_ITEM(ret, idx, item);
            }
            return ret;

            // End of code injection

            pyResult = Py_None;
            Py_INCREF(Py_None);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_TrackerFunc_getTrackByName(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Tracker *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_TRACKER_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Tracker::getTrackByName(QString)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // getTrackByName(QString)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_TrackerFunc_getTrackByName_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getTrackByName(QString)const
            Track * cppResult = const_cast<const ::Tracker *>(cppSelf)->getTrackByName(cppArg0);
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_TRACK_IDX]), cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_TrackerFunc_getTrackByName_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Tracker.getTrackByName");
        return {};
}

static PyObject *Sbk_TrackerFunc_startTracking(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Tracker *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_TRACKER_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "startTracking", 4, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: Tracker::startTracking(std::list<Track*>,int,int,bool)
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_TRACKPTR_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[3])))) {
        overloadId = 0; // startTracking(std::list<Track*>,int,int,bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_TrackerFunc_startTracking_TypeError;

    // Call function/method
    {
        ::std::list<Track* > cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        int cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);
        bool cppArg3;
        pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // startTracking(std::list<Track*>,int,int,bool)
            cppSelf->startTracking(cppArg0, cppArg1, cppArg2, cppArg3);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_TrackerFunc_startTracking_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Tracker.startTracking");
        return {};
}

static PyObject *Sbk_TrackerFunc_stopTracking(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Tracker *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_TRACKER_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // stopTracking()
            cppSelf->stopTracking();
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;
}


static const char *Sbk_Tracker_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_Tracker_methods[] = {
    {"createTrack", reinterpret_cast<PyCFunction>(Sbk_TrackerFunc_createTrack), METH_NOARGS},
    {"getAllTracks", reinterpret_cast<PyCFunction>(Sbk_TrackerFunc_getAllTracks), METH_NOARGS},
    {"getSelectedTracks", reinterpret_cast<PyCFunction>(Sbk_TrackerFunc_getSelectedTracks), METH_NOARGS},
    {"getTrackByName", reinterpret_cast<PyCFunction>(Sbk_TrackerFunc_getTrackByName), METH_O},
    {"startTracking", reinterpret_cast<PyCFunction>(Sbk_TrackerFunc_startTracking), METH_VARARGS},
    {"stopTracking", reinterpret_cast<PyCFunction>(Sbk_TrackerFunc_stopTracking), METH_NOARGS},

    {nullptr, nullptr} // Sentinel
};

} // extern "C"

static int Sbk_Tracker_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_Tracker_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_Tracker_Type = nullptr;
static SbkObjectType *Sbk_Tracker_TypeF(void)
{
    return _Sbk_Tracker_Type;
}

static PyType_Slot Sbk_Tracker_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    nullptr},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_Tracker_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_Tracker_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_Tracker_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_Tracker_spec = {
    "1:NatronEngine.Tracker",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_Tracker_slots
};

} //extern "C"


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void Tracker_PythonToCpp_Tracker_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_Tracker_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_Tracker_PythonToCpp_Tracker_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_Tracker_TypeF())))
        return Tracker_PythonToCpp_Tracker_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *Tracker_PTR_CppToPython_Tracker(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::Tracker *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_Tracker_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *Tracker_SignatureStrings[] = {
    "NatronEngine.Tracker.createTrack(self)->NatronEngine.Track",
    "NatronEngine.Tracker.getAllTracks(self,markers:std.list[NatronEngine.Track])",
    "NatronEngine.Tracker.getSelectedTracks(self,markers:std.list[NatronEngine.Track])",
    "NatronEngine.Tracker.getTrackByName(self,name:QString)->NatronEngine.Track",
    "NatronEngine.Tracker.startTracking(self,marks:std.list[NatronEngine.Track],start:int,end:int,forward:bool)",
    "NatronEngine.Tracker.stopTracking(self)",
    nullptr}; // Sentinel

void init_Tracker(PyObject *module)
{
    _Sbk_Tracker_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "Tracker",
        "Tracker*",
        &Sbk_Tracker_spec,
        &Shiboken::callCppDestructor< ::Tracker >,
        0,
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_Tracker_Type);
    InitSignatureStrings(pyType, Tracker_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_Tracker_Type), Sbk_Tracker_PropertyStrings);
    SbkNatronEngineTypes[SBK_TRACKER_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_Tracker_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_Tracker_TypeF(),
        Tracker_PythonToCpp_Tracker_PTR,
        is_Tracker_PythonToCpp_Tracker_PTR_Convertible,
        Tracker_PTR_CppToPython_Tracker);

    Shiboken::Conversions::registerConverterName(converter, "Tracker");
    Shiboken::Conversions::registerConverterName(converter, "Tracker*");
    Shiboken::Conversions::registerConverterName(converter, "Tracker&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::Tracker).name());



}
