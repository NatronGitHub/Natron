
// default includes
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
GCC_DIAG_OFF(uninitialized)
#include <shiboken.h> // produces many warnings
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "group_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING
#include <PyNode.h>
#include <list>


// Native ---------------------------------------------------------

void GroupWrapper::pysideInitQtMetaTypes()
{
}

GroupWrapper::GroupWrapper() : Group() {
    // ... middle
}

GroupWrapper::~GroupWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_Group_Init(PyObject* self, PyObject* args, PyObject* kwds)
{
    SbkObject* sbkSelf = reinterpret_cast<SbkObject*>(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::Group >()))
        return -1;

    ::GroupWrapper* cptr = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // Group()
            cptr = new ::GroupWrapper();
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::Group >(), cptr)) {
        delete cptr;
        return -1;
    }
    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::Object::setHasCppWrapper(sbkSelf, true);
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;
}

static PyObject* Sbk_GroupFunc_getChildren(PyObject* self)
{
    ::Group* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Group*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_GROUP_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getChildren()const
            // Begin code injection

            std::list<Effect*> effects = cppSelf->getChildren();
            PyObject* ret = PyList_New((int) effects.size());
            int idx = 0;
            for (std::list<Effect*>::iterator it = effects.begin(); it!=effects.end(); ++it,++idx) {
            PyObject* item = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], *it);
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
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_GroupFunc_getNode(PyObject* self, PyObject* pyArg)
{
    ::Group* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::Group*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_GROUP_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getNode(std::string)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArg)))) {
        overloadId = 0; // getNode(std::string)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GroupFunc_getNode_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getNode(std::string)const
            Effect * cppResult = const_cast<const ::Group*>(cppSelf)->getNode(cppArg0);
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_GroupFunc_getNode_TypeError:
        const char* overloads[] = {"std::string", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Group.getNode", overloads);
        return 0;
}

static PyMethodDef Sbk_Group_methods[] = {
    {"getChildren", (PyCFunction)Sbk_GroupFunc_getChildren, METH_NOARGS},
    {"getNode", (PyCFunction)Sbk_GroupFunc_getNode, METH_O},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_Group_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_Group_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_Group_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.Group",
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
    /*tp_flags*/            Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    /*tp_doc*/              0,
    /*tp_traverse*/         Sbk_Group_traverse,
    /*tp_clear*/            Sbk_Group_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_Group_methods,
    /*tp_members*/          0,
    /*tp_getset*/           0,
    /*tp_base*/             reinterpret_cast<PyTypeObject*>(&SbkObject_Type),
    /*tp_dict*/             0,
    /*tp_descr_get*/        0,
    /*tp_descr_set*/        0,
    /*tp_dictoffset*/       0,
    /*tp_init*/             Sbk_Group_Init,
    /*tp_alloc*/            0,
    /*tp_new*/              SbkObjectTpNew,
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
static void Group_PythonToCpp_Group_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_Group_Type, pyIn, cppOut);
}
static PythonToCppFunc is_Group_PythonToCpp_Group_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_Group_Type))
        return Group_PythonToCpp_Group_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* Group_PTR_CppToPython_Group(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::Group*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_Group_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_Group(PyObject* module)
{
    SbkNatronEngineTypes[SBK_GROUP_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_Group_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "Group", "Group*",
        &Sbk_Group_Type, &Shiboken::callCppDestructor< ::Group >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_Group_Type,
        Group_PythonToCpp_Group_PTR,
        is_Group_PythonToCpp_Group_PTR_Convertible,
        Group_PTR_CppToPython_Group);

    Shiboken::Conversions::registerConverterName(converter, "Group");
    Shiboken::Conversions::registerConverterName(converter, "Group*");
    Shiboken::Conversions::registerConverterName(converter, "Group&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::Group).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::GroupWrapper).name());



    GroupWrapper::pysideInitQtMetaTypes();
}
