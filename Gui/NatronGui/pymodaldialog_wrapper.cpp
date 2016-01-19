
// default includes
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
#include <shiboken.h> // produces many warnings
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <typeresolver.h>
#include <typeinfo>
#include <signalmanager.h>
CLANG_DIAG_OFF(header-guard)
#include <pysidemetafunction.h> // has wrong header guards in pyside 1.2.2
#include <set>
#include "natrongui_python.h"

#include "pymodaldialog_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING
#include <ParameterWrapper.h>
#include <qcoreevent.h>
#include <qdialog.h>
#include <qevent.h>
#include <qobject.h>
#include <qsize.h>
#include <qwidget.h>


// Native ---------------------------------------------------------

void PyModalDialogWrapper::pysideInitQtMetaTypes()
{
}

void PyModalDialogWrapper::accept()
{
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return ;
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, "accept"));
    if (pyOverride.isNull()) {
        gil.release();
        return this->::QDialog::accept();
    }

    Shiboken::AutoDecRef pyArgs(PyTuple_New(0));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, NULL));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return ;
    }
}

void PyModalDialogWrapper::done(int arg__1)
{
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return ;
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, "done"));
    if (pyOverride.isNull()) {
        gil.release();
        return this->::QDialog::done(arg__1);
    }

    Shiboken::AutoDecRef pyArgs(Py_BuildValue("(i)",
        arg__1
    ));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, NULL));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return ;
    }
}

void PyModalDialogWrapper::reject()
{
    Shiboken::GilState gil;
    if (PyErr_Occurred())
        return ;
    Shiboken::AutoDecRef pyOverride(Shiboken::BindingManager::instance().getOverride(this, "reject"));
    if (pyOverride.isNull()) {
        gil.release();
        return this->::QDialog::reject();
    }

    Shiboken::AutoDecRef pyArgs(PyTuple_New(0));

    Shiboken::AutoDecRef pyResult(PyObject_Call(pyOverride, pyArgs, NULL));
    // An error happened in python code!
    if (pyResult.isNull()) {
        PyErr_Print();
        return ;
    }
}

const QMetaObject* PyModalDialogWrapper::metaObject() const
{
    #if QT_VERSION >= 0x040700
    if (QObject::d_ptr->metaObject) return QObject::d_ptr->metaObject;
    #endif
    SbkObject* pySelf = Shiboken::BindingManager::instance().retrieveWrapper(this);
    if (pySelf == NULL)
        return PyModalDialog::metaObject();
    return PySide::SignalManager::retriveMetaObject(reinterpret_cast<PyObject*>(pySelf));
}

int PyModalDialogWrapper::qt_metacall(QMetaObject::Call call, int id, void** args)
{
    int result = PyModalDialog::qt_metacall(call, id, args);
    return result < 0 ? result : PySide::SignalManager::qt_metacall(this, call, id, args);
}

void* PyModalDialogWrapper::qt_metacast(const char* _clname)
{
        if (!_clname) return 0;
        SbkObject* pySelf = Shiboken::BindingManager::instance().retrieveWrapper(this);
        if (pySelf && PySide::inherits(Py_TYPE(pySelf), _clname))
                return static_cast<void*>(const_cast< PyModalDialogWrapper* >(this));
        return PyModalDialog::qt_metacast(_clname);
}

PyModalDialogWrapper::~PyModalDialogWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_PyModalDialogFunc_addWidget(PyObject* self, PyObject* pyArg)
{
    PyModalDialogWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (PyModalDialogWrapper*)((::PyModalDialog*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYMODALDIALOG_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: addWidget(QWidget*)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkPySide_QtGuiTypes[SBK_QWIDGET_IDX], (pyArg)))) {
        overloadId = 0; // addWidget(QWidget*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyModalDialogFunc_addWidget_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArg))
            return 0;
        ::QWidget* cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // addWidget(QWidget*)
            cppSelf->addWidget(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyModalDialogFunc_addWidget_TypeError:
        const char* overloads[] = {"PySide.QtGui.QWidget", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyModalDialog.addWidget", overloads);
        return 0;
}

static PyObject* Sbk_PyModalDialogFunc_getParam(PyObject* self, PyObject* pyArg)
{
    PyModalDialogWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (PyModalDialogWrapper*)((::PyModalDialog*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYMODALDIALOG_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getParam(std::string)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArg)))) {
        overloadId = 0; // getParam(std::string)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyModalDialogFunc_getParam_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getParam(std::string)const
            Param * cppResult = const_cast<const ::PyModalDialogWrapper*>(cppSelf)->getParam(cppArg0);
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_PyModalDialogFunc_getParam_TypeError:
        const char* overloads[] = {"std::string", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyModalDialog.getParam", overloads);
        return 0;
}

static PyObject* Sbk_PyModalDialogFunc_insertWidget(PyObject* self, PyObject* args)
{
    PyModalDialogWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (PyModalDialogWrapper*)((::PyModalDialog*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYMODALDIALOG_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "insertWidget", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: insertWidget(int,QWidget*)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkPySide_QtGuiTypes[SBK_QWIDGET_IDX], (pyArgs[1])))) {
        overloadId = 0; // insertWidget(int,QWidget*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyModalDialogFunc_insertWidget_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return 0;
        ::QWidget* cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // insertWidget(int,QWidget*)
            cppSelf->insertWidget(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyModalDialogFunc_insertWidget_TypeError:
        const char* overloads[] = {"int, PySide.QtGui.QWidget", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronGui.PyModalDialog.insertWidget", overloads);
        return 0;
}

static PyObject* Sbk_PyModalDialogFunc_setParamChangedCallback(PyObject* self, PyObject* pyArg)
{
    PyModalDialogWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (PyModalDialogWrapper*)((::PyModalDialog*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYMODALDIALOG_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setParamChangedCallback(std::string)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArg)))) {
        overloadId = 0; // setParamChangedCallback(std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyModalDialogFunc_setParamChangedCallback_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setParamChangedCallback(std::string)
            cppSelf->setParamChangedCallback(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyModalDialogFunc_setParamChangedCallback_TypeError:
        const char* overloads[] = {"std::string", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyModalDialog.setParamChangedCallback", overloads);
        return 0;
}

static PyMethodDef Sbk_PyModalDialog_methods[] = {
    {"addWidget", (PyCFunction)Sbk_PyModalDialogFunc_addWidget, METH_O},
    {"getParam", (PyCFunction)Sbk_PyModalDialogFunc_getParam, METH_O},
    {"insertWidget", (PyCFunction)Sbk_PyModalDialogFunc_insertWidget, METH_VARARGS},
    {"setParamChangedCallback", (PyCFunction)Sbk_PyModalDialogFunc_setParamChangedCallback, METH_O},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_PyModalDialog_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_PyModalDialog_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
static int mi_offsets[] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
int*
Sbk_PyModalDialog_mi_init(const void* cptr)
{
    if (mi_offsets[0] == -1) {
        std::set<int> offsets;
        std::set<int>::iterator it;
        const PyModalDialog* class_ptr = reinterpret_cast<const PyModalDialog*>(cptr);
        size_t base = (size_t) class_ptr;
        offsets.insert(((size_t) static_cast<const QDialog*>(class_ptr)) - base);
        offsets.insert(((size_t) static_cast<const QDialog*>((PyModalDialog*)((void*)class_ptr))) - base);
        offsets.insert(((size_t) static_cast<const UserParamHolder*>(class_ptr)) - base);
        offsets.insert(((size_t) static_cast<const UserParamHolder*>((PyModalDialog*)((void*)class_ptr))) - base);
        offsets.insert(((size_t) static_cast<const QWidget*>(class_ptr)) - base);
        offsets.insert(((size_t) static_cast<const QWidget*>((QDialog*)((void*)class_ptr))) - base);
        offsets.insert(((size_t) static_cast<const QObject*>(class_ptr)) - base);
        offsets.insert(((size_t) static_cast<const QObject*>((QWidget*)((void*)class_ptr))) - base);
        offsets.insert(((size_t) static_cast<const QPaintDevice*>(class_ptr)) - base);
        offsets.insert(((size_t) static_cast<const QPaintDevice*>((QWidget*)((void*)class_ptr))) - base);

        offsets.erase(0);

        int i = 0;
        for (it = offsets.begin(); it != offsets.end(); it++) {
            mi_offsets[i] = *it;
            i++;
        }
    }
    return mi_offsets;
}
static void* Sbk_PyModalDialogSpecialCastFunction(void* obj, SbkObjectType* desiredType)
{
    PyModalDialog* me = reinterpret_cast< ::PyModalDialog*>(obj);
    if (desiredType == reinterpret_cast<SbkObjectType*>(SbkPySide_QtGuiTypes[SBK_QDIALOG_IDX]))
        return static_cast< ::QDialog*>(me);
    else if (desiredType == reinterpret_cast<SbkObjectType*>(SbkPySide_QtGuiTypes[SBK_QWIDGET_IDX]))
        return static_cast< ::QWidget*>(me);
    else if (desiredType == reinterpret_cast<SbkObjectType*>(SbkPySide_QtCoreTypes[SBK_QOBJECT_IDX]))
        return static_cast< ::QObject*>(me);
    else if (desiredType == reinterpret_cast<SbkObjectType*>(SbkPySide_QtGuiTypes[SBK_QPAINTDEVICE_IDX]))
        return static_cast< ::QPaintDevice*>(me);
    else if (desiredType == reinterpret_cast<SbkObjectType*>(SbkNatronEngineTypes[SBK_USERPARAMHOLDER_IDX]))
        return static_cast< ::UserParamHolder*>(me);
    return me;
}


// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_PyModalDialog_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronGui.PyModalDialog",
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
    /*tp_traverse*/         Sbk_PyModalDialog_traverse,
    /*tp_clear*/            Sbk_PyModalDialog_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_PyModalDialog_methods,
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

static void* Sbk_PyModalDialog_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::QObject >()))
        return dynamic_cast< ::PyModalDialog*>(reinterpret_cast< ::QObject*>(cptr));
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::QPaintDevice >()))
        return dynamic_cast< ::PyModalDialog*>(reinterpret_cast< ::QPaintDevice*>(cptr));
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::UserParamHolder >()))
        return dynamic_cast< ::PyModalDialog*>(reinterpret_cast< ::UserParamHolder*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void PyModalDialog_PythonToCpp_PyModalDialog_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_PyModalDialog_Type, pyIn, cppOut);
}
static PythonToCppFunc is_PyModalDialog_PythonToCpp_PyModalDialog_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_PyModalDialog_Type))
        return PyModalDialog_PythonToCpp_PyModalDialog_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* PyModalDialog_PTR_CppToPython_PyModalDialog(const void* cppIn) {
    return PySide::getWrapperForQObject((::PyModalDialog*)cppIn, &Sbk_PyModalDialog_Type);

}

void init_PyModalDialog(PyObject* module)
{
    SbkNatronGuiTypes[SBK_PYMODALDIALOG_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_PyModalDialog_Type);

    PyObject* Sbk_PyModalDialog_Type_bases = PyTuple_Pack(2,
        (PyObject*)SbkPySide_QtGuiTypes[SBK_QDIALOG_IDX],
        (PyObject*)SbkNatronEngineTypes[SBK_USERPARAMHOLDER_IDX]);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "PyModalDialog", "PyModalDialog*",
        &Sbk_PyModalDialog_Type, &Shiboken::callCppDestructor< ::PyModalDialog >, (SbkObjectType*)SbkNatronEngineTypes[SBK_USERPARAMHOLDER_IDX], Sbk_PyModalDialog_Type_bases)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_PyModalDialog_Type,
        PyModalDialog_PythonToCpp_PyModalDialog_PTR,
        is_PyModalDialog_PythonToCpp_PyModalDialog_PTR_Convertible,
        PyModalDialog_PTR_CppToPython_PyModalDialog);

    Shiboken::Conversions::registerConverterName(converter, "PyModalDialog");
    Shiboken::Conversions::registerConverterName(converter, "PyModalDialog*");
    Shiboken::Conversions::registerConverterName(converter, "PyModalDialog&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyModalDialog).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyModalDialogWrapper).name());


    MultipleInheritanceInitFunction func = Sbk_PyModalDialog_mi_init;
    Shiboken::ObjectType::setMultipleIheritanceFunction(&Sbk_PyModalDialog_Type, func);
    Shiboken::ObjectType::setCastFunction(&Sbk_PyModalDialog_Type, &Sbk_PyModalDialogSpecialCastFunction);
    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_PyModalDialog_Type, &Sbk_PyModalDialog_typeDiscovery);


    PyModalDialogWrapper::pysideInitQtMetaTypes();
    Shiboken::ObjectType::setSubTypeInitHook(&Sbk_PyModalDialog_Type, &PySide::initQObjectSubType);
    PySide::initDynamicMetaObject(&Sbk_PyModalDialog_Type, &::PyModalDialog::staticMetaObject, sizeof(::PyModalDialog));
}
