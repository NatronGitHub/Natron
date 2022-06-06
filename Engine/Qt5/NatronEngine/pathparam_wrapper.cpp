
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
#include "pathparam_wrapper.h"

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

// Native ---------------------------------------------------------

void PathParamWrapper::pysideInitQtMetaTypes()
{
}

void PathParamWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

PathParamWrapper::~PathParamWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject *Sbk_PathParamFunc_getTable(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PathParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PATHPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PathParam.getTable";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getTable(std::list<std::vector<std::string> >*)const
            // Begin code injection
            std::list<std::vector<std::string> > table;
            cppSelf->getTable(&table);

            std::size_t outListSize = table.size();
            PyObject* outList = PyList_New((int) outListSize);

            std::size_t i = 0;
            for (std::list<std::vector<std::string> >::iterator it = table.begin(); it != table.end(); ++it, ++i) {
            std::size_t subListSize = it->size();
            PyObject* subList = PyList_New((int) subListSize);
            for (std::size_t j = 0; j < subListSize; ++j) {
            std::string cppItem = (*it)[j];
            PyList_SET_ITEM(subList, j, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppItem));
            }
            PyList_SET_ITEM(outList, i, subList);
            }

            return outList;

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

static PyObject *Sbk_PathParamFunc_setAsMultiPathTable(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PathParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PATHPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PathParam.setAsMultiPathTable";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // setAsMultiPathTable()
            cppSelf->setAsMultiPathTable();
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;
}

static PyObject *Sbk_PathParamFunc_setTable(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PathParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PATHPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PathParam.setTable";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PathParam::setTable(std::list<std::vector<std::string> >)
    if (Shiboken::String::checkIterable(pyArg)) {
        overloadId = 0; // setTable(std::list<std::vector<std::string> >)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PathParamFunc_setTable_TypeError;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // setTable(std::list<std::vector<std::string> >)
            // Begin code injection


            if (!PyList_Check(pyArg)) {
                PyErr_SetString(PyExc_TypeError, "table must be a list of list objects.");
                return 0;
            }

            std::list<std::vector<std::string> > table;

            int size = (int)PyList_GET_SIZE(pyArg);
            for (int i = 0; i < size; ++i) {


                PyObject* subList = PyList_GET_ITEM(pyArg,i);
                if (!subList || !PyList_Check(subList)) {
                    PyErr_SetString(PyExc_TypeError, "table must be a list of list objects.");
                    return 0;
                }
                int subSize = (int)PyList_GET_SIZE(subList);
                std::vector<std::string> rowVec(subSize);

                for (int j = 0; j < subSize; ++j) {
                    PyObject* pyString = PyList_GET_ITEM(subList,j);
#if PY_MAJOR_VERSION < 3
                    if ( PyString_Check(pyString) ) {
                        char* buf = PyString_AsString(pyString);
                        if (buf) {
                            std::string ret;
                            ret.append(buf);
                            rowVec[j] = ret;
                            }
                    } else if (PyUnicode_Check(pyString) ) {
#else
                    if (PyUnicode_Check(pyString) ) {
#endif
                        PyObject* utf8pyobj = PyUnicode_AsUTF8String(pyString); // newRef
                        if (utf8pyobj) {
                            char* cstr = PyBytes_AS_STRING(utf8pyobj); // Borrowed pointer
                            std::string ret;
                            ret.append(cstr);
                            Py_DECREF(utf8pyobj);
                            rowVec[j] = ret;
                        }
                    }
                }
                table.push_back(rowVec);
            }

            cppSelf->setTable(table);

            // End of code injection

        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PathParamFunc_setTable_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}


static const char *Sbk_PathParam_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_PathParam_methods[] = {
    {"getTable", reinterpret_cast<PyCFunction>(Sbk_PathParamFunc_getTable), METH_NOARGS},
    {"setAsMultiPathTable", reinterpret_cast<PyCFunction>(Sbk_PathParamFunc_setAsMultiPathTable), METH_NOARGS},
    {"setTable", reinterpret_cast<PyCFunction>(Sbk_PathParamFunc_setTable), METH_O},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_PathParam_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::PathParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PATHPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<PathParamWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_PathParam_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_PathParam_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_PathParam_Type = nullptr;
static SbkObjectType *Sbk_PathParam_TypeF(void)
{
    return _Sbk_PathParam_Type;
}

static PyType_Slot Sbk_PathParam_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_PathParam_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_PathParam_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_PathParam_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_PathParam_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_PathParam_spec = {
    "1:NatronEngine.PathParam",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_PathParam_slots
};

} //extern "C"

static void *Sbk_PathParam_typeDiscovery(void *cptr, SbkObjectType *instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::PathParam *>(reinterpret_cast< ::Param *>(cptr));
    return {};
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void PathParam_PythonToCpp_PathParam_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_PathParam_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_PathParam_PythonToCpp_PathParam_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_PathParam_TypeF())))
        return PathParam_PythonToCpp_PathParam_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *PathParam_PTR_CppToPython_PathParam(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::PathParam *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_PathParam_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *PathParam_SignatureStrings[] = {
    "NatronEngine.PathParam.getTable(self,table:std.list[std.vector[std.string]])",
    "NatronEngine.PathParam.setAsMultiPathTable(self)",
    "NatronEngine.PathParam.setTable(self,table:std.list[std.vector[std.string]])",
    nullptr}; // Sentinel

void init_PathParam(PyObject *module)
{
    _Sbk_PathParam_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "PathParam",
        "PathParam*",
        &Sbk_PathParam_spec,
        &Shiboken::callCppDestructor< ::PathParam >,
        reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX]),
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_PathParam_Type);
    InitSignatureStrings(pyType, PathParam_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_PathParam_Type), Sbk_PathParam_PropertyStrings);
    SbkNatronEngineTypes[SBK_PATHPARAM_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_PathParam_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_PathParam_TypeF(),
        PathParam_PythonToCpp_PathParam_PTR,
        is_PathParam_PythonToCpp_PathParam_PTR_Convertible,
        PathParam_PTR_CppToPython_PathParam);

    Shiboken::Conversions::registerConverterName(converter, "PathParam");
    Shiboken::Conversions::registerConverterName(converter, "PathParam*");
    Shiboken::Conversions::registerConverterName(converter, "PathParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::PathParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::PathParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(Sbk_PathParam_TypeF(), &Sbk_PathParam_typeDiscovery);

    PathParamWrapper::pysideInitQtMetaTypes();
}
