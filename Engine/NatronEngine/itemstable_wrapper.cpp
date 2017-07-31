
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

#include "itemstable_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING
#include <PyItemsTable.h>
#include <list>


// Native ---------------------------------------------------------

void ItemsTableWrapper::pysideInitQtMetaTypes()
{
}

ItemsTableWrapper::~ItemsTableWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_ItemsTableFunc_getAttributeName(PyObject* self)
{
    ::ItemsTable* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemsTable*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMSTABLE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getAttributeName()const
            QString cppResult = const_cast<const ::ItemsTable*>(cppSelf)->getAttributeName();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ItemsTableFunc_getItemByFullyQualifiedScriptName(PyObject* self, PyObject* pyArg)
{
    ::ItemsTable* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemsTable*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMSTABLE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getItemByFullyQualifiedScriptName(QString)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // getItemByFullyQualifiedScriptName(QString)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ItemsTableFunc_getItemByFullyQualifiedScriptName_TypeError;

    // Call function/method
    {
        ::QString cppArg0 = ::QString();
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getItemByFullyQualifiedScriptName(QString)const
            ItemBase * cppResult = const_cast<const ::ItemsTable*>(cppSelf)->getItemByFullyQualifiedScriptName(cppArg0);
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_ITEMBASE_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ItemsTableFunc_getItemByFullyQualifiedScriptName_TypeError:
        const char* overloads[] = {"unicode", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ItemsTable.getItemByFullyQualifiedScriptName", overloads);
        return 0;
}

static PyObject* Sbk_ItemsTableFunc_getSelectedItems(PyObject* self)
{
    ::ItemsTable* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemsTable*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMSTABLE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getSelectedItems()const
            // Begin code injection

            std::list<ItemBase*> items = cppSelf->getSelectedItems();
            PyObject* ret = PyList_New((int) items.size());
            int idx = 0;
            for (std::list<ItemBase*>::iterator it = items.begin(); it!=items.end(); ++it,++idx) {
            PyObject* item = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_ITEMBASE_IDX], *it);
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

static PyObject* Sbk_ItemsTableFunc_getTableName(PyObject* self)
{
    ::ItemsTable* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemsTable*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMSTABLE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getTableName()const
            QString cppResult = const_cast<const ::ItemsTable*>(cppSelf)->getTableName();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ItemsTableFunc_getTopLevelItems(PyObject* self)
{
    ::ItemsTable* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemsTable*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMSTABLE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getTopLevelItems()const
            // Begin code injection

            std::list<ItemBase*> items = cppSelf->getTopLevelItems();
            PyObject* ret = PyList_New((int) items.size());
            int idx = 0;
            for (std::list<ItemBase*>::iterator it = items.begin(); it!=items.end(); ++it,++idx) {
            PyObject* item = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_ITEMBASE_IDX], *it);
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

static PyObject* Sbk_ItemsTableFunc_insertItem(PyObject* self, PyObject* args)
{
    ::ItemsTable* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemsTable*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMSTABLE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "insertItem", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: insertItem(int,const ItemBase*,const ItemBase*)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (pyArgs[2])))) {
        overloadId = 0; // insertItem(int,const ItemBase*,const ItemBase*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ItemsTableFunc_insertItem_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return 0;
        ::ItemBase* cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        if (!Shiboken::Object::isValid(pyArgs[2]))
            return 0;
        ::ItemBase* cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // insertItem(int,const ItemBase*,const ItemBase*)
            cppSelf->insertItem(cppArg0, cppArg1, cppArg2);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ItemsTableFunc_insertItem_TypeError:
        const char* overloads[] = {"int, NatronEngine.ItemBase, NatronEngine.ItemBase", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ItemsTable.insertItem", overloads);
        return 0;
}

static PyObject* Sbk_ItemsTableFunc_isModelParentingEnabled(PyObject* self)
{
    ::ItemsTable* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemsTable*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMSTABLE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isModelParentingEnabled()const
            bool cppResult = const_cast<const ::ItemsTable*>(cppSelf)->isModelParentingEnabled();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ItemsTableFunc_removeItem(PyObject* self, PyObject* pyArg)
{
    ::ItemsTable* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemsTable*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMSTABLE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: removeItem(const ItemBase*)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (pyArg)))) {
        overloadId = 0; // removeItem(const ItemBase*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ItemsTableFunc_removeItem_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArg))
            return 0;
        ::ItemBase* cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // removeItem(const ItemBase*)
            cppSelf->removeItem(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ItemsTableFunc_removeItem_TypeError:
        const char* overloads[] = {"NatronEngine.ItemBase", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ItemsTable.removeItem", overloads);
        return 0;
}

static PyMethodDef Sbk_ItemsTable_methods[] = {
    {"getAttributeName", (PyCFunction)Sbk_ItemsTableFunc_getAttributeName, METH_NOARGS},
    {"getItemByFullyQualifiedScriptName", (PyCFunction)Sbk_ItemsTableFunc_getItemByFullyQualifiedScriptName, METH_O},
    {"getSelectedItems", (PyCFunction)Sbk_ItemsTableFunc_getSelectedItems, METH_NOARGS},
    {"getTableName", (PyCFunction)Sbk_ItemsTableFunc_getTableName, METH_NOARGS},
    {"getTopLevelItems", (PyCFunction)Sbk_ItemsTableFunc_getTopLevelItems, METH_NOARGS},
    {"insertItem", (PyCFunction)Sbk_ItemsTableFunc_insertItem, METH_VARARGS},
    {"isModelParentingEnabled", (PyCFunction)Sbk_ItemsTableFunc_isModelParentingEnabled, METH_NOARGS},
    {"removeItem", (PyCFunction)Sbk_ItemsTableFunc_removeItem, METH_O},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_ItemsTable_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_ItemsTable_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_ItemsTable_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.ItemsTable",
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
    /*tp_traverse*/         Sbk_ItemsTable_traverse,
    /*tp_clear*/            Sbk_ItemsTable_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_ItemsTable_methods,
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
static void ItemsTable_PythonToCpp_ItemsTable_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_ItemsTable_Type, pyIn, cppOut);
}
static PythonToCppFunc is_ItemsTable_PythonToCpp_ItemsTable_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_ItemsTable_Type))
        return ItemsTable_PythonToCpp_ItemsTable_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* ItemsTable_PTR_CppToPython_ItemsTable(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::ItemsTable*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_ItemsTable_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_ItemsTable(PyObject* module)
{
    SbkNatronEngineTypes[SBK_ITEMSTABLE_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_ItemsTable_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "ItemsTable", "ItemsTable*",
        &Sbk_ItemsTable_Type, &Shiboken::callCppDestructor< ::ItemsTable >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_ItemsTable_Type,
        ItemsTable_PythonToCpp_ItemsTable_PTR,
        is_ItemsTable_PythonToCpp_ItemsTable_PTR_Convertible,
        ItemsTable_PTR_CppToPython_ItemsTable);

    Shiboken::Conversions::registerConverterName(converter, "ItemsTable");
    Shiboken::Conversions::registerConverterName(converter, "ItemsTable*");
    Shiboken::Conversions::registerConverterName(converter, "ItemsTable&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::ItemsTable).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::ItemsTableWrapper).name());



    ItemsTableWrapper::pysideInitQtMetaTypes();
}
