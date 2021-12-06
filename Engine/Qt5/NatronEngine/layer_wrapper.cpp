
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
#include "layer_wrapper.h"

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

void LayerWrapper::pysideInitQtMetaTypes()
{
}

void LayerWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

LayerWrapper::~LayerWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject *Sbk_LayerFunc_addItem(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Layer *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_LAYER_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Layer::addItem(ItemBase*)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_ITEMBASE_IDX]), (pyArg)))) {
        overloadId = 0; // addItem(ItemBase*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_LayerFunc_addItem_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArg))
            return {};
        ::ItemBase *cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // addItem(ItemBase*)
            // Begin code injection
            cppSelf->addItem(cppArg0);

            // End of code injection

        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_LayerFunc_addItem_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Layer.addItem");
        return {};
}

static PyObject *Sbk_LayerFunc_getChildren(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Layer *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_LAYER_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getChildren()const
            // Begin code injection
            std::list<ItemBase*> items = cppSelf->getChildren();
            PyObject* ret = PyList_New((int) items.size());
            int idx = 0;
            for (std::list<ItemBase*>::iterator it = items.begin(); it!=items.end(); ++it,++idx) {
            PyObject* item = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_ITEMBASE_IDX]), *it);
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

static PyObject *Sbk_LayerFunc_insertItem(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Layer *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_LAYER_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "insertItem", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: Layer::insertItem(int,ItemBase*)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_ITEMBASE_IDX]), (pyArgs[1])))) {
        overloadId = 0; // insertItem(int,ItemBase*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_LayerFunc_insertItem_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return {};
        ::ItemBase *cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // insertItem(int,ItemBase*)
            // Begin code injection
            cppSelf->insertItem(cppArg0,cppArg1);

            // End of code injection

        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_LayerFunc_insertItem_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Layer.insertItem");
        return {};
}

static PyObject *Sbk_LayerFunc_removeItem(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Layer *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_LAYER_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Layer::removeItem(ItemBase*)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_ITEMBASE_IDX]), (pyArg)))) {
        overloadId = 0; // removeItem(ItemBase*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_LayerFunc_removeItem_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArg))
            return {};
        ::ItemBase *cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // removeItem(ItemBase*)
            // Begin code injection
            cppSelf->removeItem(cppArg0);

            // End of code injection

        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_LayerFunc_removeItem_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Layer.removeItem");
        return {};
}


static const char *Sbk_Layer_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_Layer_methods[] = {
    {"addItem", reinterpret_cast<PyCFunction>(Sbk_LayerFunc_addItem), METH_O},
    {"getChildren", reinterpret_cast<PyCFunction>(Sbk_LayerFunc_getChildren), METH_NOARGS},
    {"insertItem", reinterpret_cast<PyCFunction>(Sbk_LayerFunc_insertItem), METH_VARARGS},
    {"removeItem", reinterpret_cast<PyCFunction>(Sbk_LayerFunc_removeItem), METH_O},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_Layer_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::Layer *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_LAYER_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<LayerWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_Layer_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_Layer_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_Layer_Type = nullptr;
static SbkObjectType *Sbk_Layer_TypeF(void)
{
    return _Sbk_Layer_Type;
}

static PyType_Slot Sbk_Layer_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_Layer_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_Layer_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_Layer_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_Layer_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_Layer_spec = {
    "1:NatronEngine.Layer",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_Layer_slots
};

} //extern "C"

static void *Sbk_Layer_typeDiscovery(void *cptr, SbkObjectType *instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::ItemBase >()))
        return dynamic_cast< ::Layer *>(reinterpret_cast< ::ItemBase *>(cptr));
    return {};
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void Layer_PythonToCpp_Layer_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_Layer_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_Layer_PythonToCpp_Layer_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_Layer_TypeF())))
        return Layer_PythonToCpp_Layer_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *Layer_PTR_CppToPython_Layer(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::Layer *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_Layer_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *Layer_SignatureStrings[] = {
    "NatronEngine.Layer.addItem(self,item:NatronEngine.ItemBase)",
    "NatronEngine.Layer.getChildren(self)->std.list[NatronEngine.ItemBase]",
    "NatronEngine.Layer.insertItem(self,pos:int,item:NatronEngine.ItemBase)",
    "NatronEngine.Layer.removeItem(self,item:NatronEngine.ItemBase)",
    nullptr}; // Sentinel

void init_Layer(PyObject *module)
{
    _Sbk_Layer_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "Layer",
        "Layer*",
        &Sbk_Layer_spec,
        &Shiboken::callCppDestructor< ::Layer >,
        reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_ITEMBASE_IDX]),
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_Layer_Type);
    InitSignatureStrings(pyType, Layer_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_Layer_Type), Sbk_Layer_PropertyStrings);
    SbkNatronEngineTypes[SBK_LAYER_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_Layer_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_Layer_TypeF(),
        Layer_PythonToCpp_Layer_PTR,
        is_Layer_PythonToCpp_Layer_PTR_Convertible,
        Layer_PTR_CppToPython_Layer);

    Shiboken::Conversions::registerConverterName(converter, "Layer");
    Shiboken::Conversions::registerConverterName(converter, "Layer*");
    Shiboken::Conversions::registerConverterName(converter, "Layer&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::Layer).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::LayerWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(Sbk_Layer_TypeF(), &Sbk_Layer_typeDiscovery);


    LayerWrapper::pysideInitQtMetaTypes();
}
