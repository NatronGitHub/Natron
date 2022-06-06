
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
#include "nodecreationproperty_wrapper.h"

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

void NodeCreationPropertyWrapper::pysideInitQtMetaTypes()
{
}

void NodeCreationPropertyWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

NodeCreationPropertyWrapper::NodeCreationPropertyWrapper() : NodeCreationProperty()
{
    resetPyMethodCache();
    // ... middle
}

NodeCreationPropertyWrapper::~NodeCreationPropertyWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_NodeCreationProperty_Init(PyObject *self, PyObject *args, PyObject *kwds)
{
    SbkObject *sbkSelf = reinterpret_cast<SbkObject *>(self);
PySide::Feature::Select(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::NodeCreationProperty >()))
        return -1;

    ::NodeCreationPropertyWrapper *cptr{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.NodeCreationProperty.__init__";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // NodeCreationProperty()
            cptr = new ::NodeCreationPropertyWrapper();
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::NodeCreationProperty >(), cptr)) {
        delete cptr;
        return -1;
    }
    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::Object::setHasCppWrapper(sbkSelf, true);
    if (Shiboken::BindingManager::instance().hasWrapper(cptr)) {
        Shiboken::BindingManager::instance().releaseWrapper(Shiboken::BindingManager::instance().retrieveWrapper(cptr));
    }
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;
}


static const char *Sbk_NodeCreationProperty_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_NodeCreationProperty_methods[] = {

    {nullptr, nullptr} // Sentinel
};

static int Sbk_NodeCreationProperty_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::NodeCreationProperty *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_NODECREATIONPROPERTY_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<NodeCreationPropertyWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_NodeCreationProperty_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_NodeCreationProperty_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_NodeCreationProperty_Type = nullptr;
static SbkObjectType *Sbk_NodeCreationProperty_TypeF(void)
{
    return _Sbk_NodeCreationProperty_Type;
}

static PyType_Slot Sbk_NodeCreationProperty_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_NodeCreationProperty_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_NodeCreationProperty_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_NodeCreationProperty_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_NodeCreationProperty_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        reinterpret_cast<void *>(Sbk_NodeCreationProperty_Init)},
    {Py_tp_new,         reinterpret_cast<void *>(SbkObjectTpNew)},
    {0, nullptr}
};
static PyType_Spec Sbk_NodeCreationProperty_spec = {
    "1:NatronEngine.NodeCreationProperty",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_NodeCreationProperty_slots
};

} //extern "C"


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void NodeCreationProperty_PythonToCpp_NodeCreationProperty_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_NodeCreationProperty_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_NodeCreationProperty_PythonToCpp_NodeCreationProperty_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_NodeCreationProperty_TypeF())))
        return NodeCreationProperty_PythonToCpp_NodeCreationProperty_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *NodeCreationProperty_PTR_CppToPython_NodeCreationProperty(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::NodeCreationProperty *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_NodeCreationProperty_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *NodeCreationProperty_SignatureStrings[] = {
    "NatronEngine.NodeCreationProperty(self)",
    nullptr}; // Sentinel

void init_NodeCreationProperty(PyObject *module)
{
    _Sbk_NodeCreationProperty_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "NodeCreationProperty",
        "NodeCreationProperty*",
        &Sbk_NodeCreationProperty_spec,
        &Shiboken::callCppDestructor< ::NodeCreationProperty >,
        0,
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_NodeCreationProperty_Type);
    InitSignatureStrings(pyType, NodeCreationProperty_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_NodeCreationProperty_Type), Sbk_NodeCreationProperty_PropertyStrings);
    SbkNatronEngineTypes[SBK_NODECREATIONPROPERTY_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_NodeCreationProperty_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_NodeCreationProperty_TypeF(),
        NodeCreationProperty_PythonToCpp_NodeCreationProperty_PTR,
        is_NodeCreationProperty_PythonToCpp_NodeCreationProperty_PTR_Convertible,
        NodeCreationProperty_PTR_CppToPython_NodeCreationProperty);

    Shiboken::Conversions::registerConverterName(converter, "NodeCreationProperty");
    Shiboken::Conversions::registerConverterName(converter, "NodeCreationProperty*");
    Shiboken::Conversions::registerConverterName(converter, "NodeCreationProperty&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::NodeCreationProperty).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::NodeCreationPropertyWrapper).name());


    NodeCreationPropertyWrapper::pysideInitQtMetaTypes();
}
