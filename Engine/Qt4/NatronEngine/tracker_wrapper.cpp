
// default includes
#include "Global/Macros.h"
// clang-format off
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
GCC_DIAG_OFF(uninitialized)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// clang-format on
#include <shiboken.h> // produces many warnings
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "tracker_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING
#include <PyTracker.h>
#include <list>



// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_TrackerFunc_createTrack(PyObject* self)
{
    ::Tracker* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Tracker*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_TRACKER_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // createTrack()
            Track * cppResult = cppSelf->createTrack();
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_TRACK_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_TrackerFunc_getAllTracks(PyObject* self)
{
    ::Tracker* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Tracker*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_TRACKER_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

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
            PyObject* item = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_TRACK_IDX], *it);
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
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_TrackerFunc_getSelectedTracks(PyObject* self)
{
    ::Tracker* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Tracker*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_TRACKER_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

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
            PyObject* item = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_TRACK_IDX], *it);
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
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_TrackerFunc_getTrackByName(PyObject* self, PyObject* pyArg)
{
    ::Tracker* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Tracker*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_TRACKER_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getTrackByName(QString)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // getTrackByName(QString)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_TrackerFunc_getTrackByName_TypeError;

    // Call function/method
    {
        ::QString cppArg0 = ::QString();
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getTrackByName(QString)const
            Track * cppResult = const_cast<const ::Tracker*>(cppSelf)->getTrackByName(cppArg0);
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_TRACK_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_TrackerFunc_getTrackByName_TypeError:
        const char* overloads[] = {"unicode", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Tracker.getTrackByName", overloads);
        return 0;
}

static PyObject* Sbk_TrackerFunc_startTracking(PyObject* self, PyObject* args)
{
    ::Tracker* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Tracker*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_TRACKER_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "startTracking", 4, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: startTracking(std::list<Track*>,int,int,bool)
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
        ::std::list<Track * > cppArg0;
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
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_TrackerFunc_startTracking_TypeError:
        const char* overloads[] = {"list, int, int, bool", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Tracker.startTracking", overloads);
        return 0;
}

static PyObject* Sbk_TrackerFunc_stopTracking(PyObject* self)
{
    ::Tracker* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Tracker*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_TRACKER_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // stopTracking()
            cppSelf->stopTracking();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyMethodDef Sbk_Tracker_methods[] = {
    {"createTrack", (PyCFunction)Sbk_TrackerFunc_createTrack, METH_NOARGS},
    {"getAllTracks", (PyCFunction)Sbk_TrackerFunc_getAllTracks, METH_NOARGS},
    {"getSelectedTracks", (PyCFunction)Sbk_TrackerFunc_getSelectedTracks, METH_NOARGS},
    {"getTrackByName", (PyCFunction)Sbk_TrackerFunc_getTrackByName, METH_O},
    {"startTracking", (PyCFunction)Sbk_TrackerFunc_startTracking, METH_VARARGS},
    {"stopTracking", (PyCFunction)Sbk_TrackerFunc_stopTracking, METH_NOARGS},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_Tracker_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_Tracker_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_Tracker_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.Tracker",
    /*tp_basicsize*/        sizeof(SbkObject),
    /*tp_itemsize*/         0,
    /*tp_dealloc*/          &SbkDeallocWrapper,
    /*tp_print*/            0,
    /*tp_getattr*/          0,
    /*tp_setattr*/          0,
    /*tp_compare*/          0,
    /*tp_repr*/             0,
    /*tp_as_number*/        0,
    /*tp_as_sequence*/      0,
    /*tp_as_mapping*/       0,
    /*tp_hash*/             0,
    /*tp_call*/             0,
    /*tp_str*/              0,
    /*tp_getattro*/         0,
    /*tp_setattro*/         0,
    /*tp_as_buffer*/        0,
    /*tp_flags*/            Py_TPFLAGS_DEFAULT|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    /*tp_doc*/              0,
    /*tp_traverse*/         Sbk_Tracker_traverse,
    /*tp_clear*/            Sbk_Tracker_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_Tracker_methods,
    /*tp_members*/          0,
    /*tp_getset*/           0,
    /*tp_base*/             reinterpret_cast<PyTypeObject*>(&SbkObject_Type),
    /*tp_dict*/             0,
    /*tp_descr_get*/        0,
    /*tp_descr_set*/        0,
    /*tp_dictoffset*/       0,
    /*tp_init*/             0,
    /*tp_alloc*/            0,
    /*tp_new*/              0,
    /*tp_free*/             0,
    /*tp_is_gc*/            0,
    /*tp_bases*/            0,
    /*tp_mro*/              0,
    /*tp_cache*/            0,
    /*tp_subclasses*/       0,
    /*tp_weaklist*/         0
}, },
    /*priv_data*/           0
};
} //extern


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void Tracker_PythonToCpp_Tracker_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_Tracker_Type, pyIn, cppOut);
}
static PythonToCppFunc is_Tracker_PythonToCpp_Tracker_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_Tracker_Type))
        return Tracker_PythonToCpp_Tracker_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* Tracker_PTR_CppToPython_Tracker(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::Tracker*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_Tracker_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_Tracker(PyObject* module)
{
    SbkNatronEngineTypes[SBK_TRACKER_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_Tracker_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "Tracker", "Tracker*",
        &Sbk_Tracker_Type, &Shiboken::callCppDestructor< ::Tracker >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_Tracker_Type,
        Tracker_PythonToCpp_Tracker_PTR,
        is_Tracker_PythonToCpp_Tracker_PTR_Convertible,
        Tracker_PTR_CppToPython_Tracker);

    Shiboken::Conversions::registerConverterName(converter, "Tracker");
    Shiboken::Conversions::registerConverterName(converter, "Tracker*");
    Shiboken::Conversions::registerConverterName(converter, "Tracker&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::Tracker).name());



}
