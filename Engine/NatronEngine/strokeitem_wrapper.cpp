
// default includes
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
GCC_DIAG_OFF(uninitialized)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <shiboken.h> // produces many warnings
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "strokeitem_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING
#include <PyItemsTable.h>
#include <PyParameter.h>
#include <PyRoto.h>
#include <RectD.h>
#include <list>


// Native ---------------------------------------------------------

void StrokeItemWrapper::pysideInitQtMetaTypes()
{
}

StrokeItemWrapper::~StrokeItemWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_StrokeItemFunc_getBoundingBox(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::StrokeItem* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::StrokeItem*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STROKEITEM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.StrokeItem.getBoundingBox(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.StrokeItem.getBoundingBox(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:getBoundingBox", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: getBoundingBox(double,QString)const
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // getBoundingBox(double,QString)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
            overloadId = 0; // getBoundingBox(double,QString)const
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StrokeItemFunc_getBoundingBox_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.StrokeItem.getBoundingBox(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                    goto Sbk_StrokeItemFunc_getBoundingBox_TypeError;
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1 = QLatin1String(kPyParamViewIdxMain);
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getBoundingBox(double,QString)const
            RectD* cppResult = new RectD(const_cast<const ::StrokeItem*>(cppSelf)->getBoundingBox(cppArg0, cppArg1));
            pyResult = Shiboken::Object::newObject((SbkObjectType*)SbkNatronEngineTypes[SBK_RECTD_IDX], cppResult, true, true);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_StrokeItemFunc_getBoundingBox_TypeError:
        const char* overloads[] = {"float, unicode = QLatin1String(kPyParamViewIdxMain)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.StrokeItem.getBoundingBox", overloads);
        return 0;
}

static PyObject* Sbk_StrokeItemFunc_getBrushType(PyObject* self)
{
    ::StrokeItem* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::StrokeItem*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STROKEITEM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getBrushType()const
            NATRON_NAMESPACE::RotoStrokeType cppResult = NATRON_NAMESPACE::RotoStrokeType(const_cast<const ::StrokeItem*>(cppSelf)->getBrushType());
            pyResult = Shiboken::Conversions::copyToPython(SBK_CONVERTER(SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_ROTOSTROKETYPE_IDX]), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_StrokeItemFunc_getPoints(PyObject* self)
{
    ::StrokeItem* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::StrokeItem*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STROKEITEM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getPoints()const
            // Begin code injection

            std::list<std::list<StrokePoint> > points = cppSelf->getPoints();
            PyObject* ret = PyList_New((int) points.size());
            int i = 0;
            for (std::list<std::list<StrokePoint> >::iterator it = points.begin(); it!=points.end(); ++it, ++i) {


                PyObject* subStrokeItemList = PyList_New((int) it->size());

                int idx = 0;
                for (std::list<StrokePoint>::iterator it2 = it->begin(); it2!=it->end(); ++it2,++idx) {
                    PyObject* item = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_STROKEPOINT_IDX], &*it);
                    // Ownership transferences.
                    PyList_SET_ITEM(subStrokeItemList, idx, item);
                }
                PyList_SET_ITEM(ret, i, subStrokeItemList);
            }
            return ret;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_StrokeItemFunc_setPoints(PyObject* self, PyObject* pyArg)
{
    ::StrokeItem* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::StrokeItem*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STROKEITEM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setPoints(std::list<std::list<StrokePoint> >)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_LIST_STROKEPOINT_IDX], (pyArg)))) {
        overloadId = 0; // setPoints(std::list<std::list<StrokePoint> >)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StrokeItemFunc_setPoints_TypeError;

    // Call function/method
    {
        ::std::list<std::list<StrokePoint > > cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setPoints(std::list<std::list<StrokePoint> >)
            cppSelf->setPoints(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_StrokeItemFunc_setPoints_TypeError:
        const char* overloads[] = {"list", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.StrokeItem.setPoints", overloads);
        return 0;
}

static PyMethodDef Sbk_StrokeItem_methods[] = {
    {"getBoundingBox", (PyCFunction)Sbk_StrokeItemFunc_getBoundingBox, METH_VARARGS|METH_KEYWORDS},
    {"getBrushType", (PyCFunction)Sbk_StrokeItemFunc_getBrushType, METH_NOARGS},
    {"getPoints", (PyCFunction)Sbk_StrokeItemFunc_getPoints, METH_NOARGS},
    {"setPoints", (PyCFunction)Sbk_StrokeItemFunc_setPoints, METH_O},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_StrokeItem_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_StrokeItem_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_StrokeItem_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.StrokeItem",
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
    /*tp_traverse*/         Sbk_StrokeItem_traverse,
    /*tp_clear*/            Sbk_StrokeItem_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_StrokeItem_methods,
    /*tp_members*/          0,
    /*tp_getset*/           0,
    /*tp_base*/             0,
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

static void* Sbk_StrokeItem_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::ItemBase >()))
        return dynamic_cast< ::StrokeItem*>(reinterpret_cast< ::ItemBase*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void StrokeItem_PythonToCpp_StrokeItem_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_StrokeItem_Type, pyIn, cppOut);
}
static PythonToCppFunc is_StrokeItem_PythonToCpp_StrokeItem_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_StrokeItem_Type))
        return StrokeItem_PythonToCpp_StrokeItem_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* StrokeItem_PTR_CppToPython_StrokeItem(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::StrokeItem*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_StrokeItem_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_StrokeItem(PyObject* module)
{
    SbkNatronEngineTypes[SBK_STROKEITEM_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_StrokeItem_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "StrokeItem", "StrokeItem*",
        &Sbk_StrokeItem_Type, &Shiboken::callCppDestructor< ::StrokeItem >, (SbkObjectType*)SbkNatronEngineTypes[SBK_ITEMBASE_IDX])) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_StrokeItem_Type,
        StrokeItem_PythonToCpp_StrokeItem_PTR,
        is_StrokeItem_PythonToCpp_StrokeItem_PTR_Convertible,
        StrokeItem_PTR_CppToPython_StrokeItem);

    Shiboken::Conversions::registerConverterName(converter, "StrokeItem");
    Shiboken::Conversions::registerConverterName(converter, "StrokeItem*");
    Shiboken::Conversions::registerConverterName(converter, "StrokeItem&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::StrokeItem).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::StrokeItemWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_StrokeItem_Type, &Sbk_StrokeItem_typeDiscovery);


    StrokeItemWrapper::pysideInitQtMetaTypes();
}
