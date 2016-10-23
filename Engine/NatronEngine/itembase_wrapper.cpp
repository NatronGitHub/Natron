
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

#include "itembase_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING
#include <PyItemsTable.h>
#include <PyParameter.h>
#include <list>


// Native ---------------------------------------------------------

void ItemBaseWrapper::pysideInitQtMetaTypes()
{
}

ItemBaseWrapper::~ItemBaseWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_ItemBaseFunc_deleteUserKeyframe(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ItemBase.deleteUserKeyframe(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ItemBase.deleteUserKeyframe(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:deleteUserKeyframe", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: deleteUserKeyframe(double,QString)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // deleteUserKeyframe(double,QString)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
            overloadId = 0; // deleteUserKeyframe(double,QString)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ItemBaseFunc_deleteUserKeyframe_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ItemBase.deleteUserKeyframe(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                    goto Sbk_ItemBaseFunc_deleteUserKeyframe_TypeError;
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1 = QLatin1String(kPyParamViewSetSpecAll);
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // deleteUserKeyframe(double,QString)
            cppSelf->deleteUserKeyframe(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ItemBaseFunc_deleteUserKeyframe_TypeError:
        const char* overloads[] = {"float, unicode = QLatin1String(kPyParamViewSetSpecAll)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ItemBase.deleteUserKeyframe", overloads);
        return 0;
}

static PyObject* Sbk_ItemBaseFunc_getChildren(PyObject* self)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getChildren()const
            // Begin code injection

            std::list<ItemBase*> items = cppSelf->getChildren();
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

static PyObject* Sbk_ItemBaseFunc_getIconFilePath(PyObject* self)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getIconFilePath()const
            QString cppResult = const_cast<const ::ItemBase*>(cppSelf)->getIconFilePath();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ItemBaseFunc_getIndexInParent(PyObject* self)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getIndexInParent()const
            int cppResult = const_cast<const ::ItemBase*>(cppSelf)->getIndexInParent();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ItemBaseFunc_getLabel(PyObject* self)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getLabel()const
            QString cppResult = const_cast<const ::ItemBase*>(cppSelf)->getLabel();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ItemBaseFunc_getParam(PyObject* self, PyObject* pyArg)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getParam(QString)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // getParam(QString)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ItemBaseFunc_getParam_TypeError;

    // Call function/method
    {
        ::QString cppArg0 = ::QString();
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getParam(QString)const
            Param * cppResult = const_cast<const ::ItemBase*>(cppSelf)->getParam(cppArg0);
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

    Sbk_ItemBaseFunc_getParam_TypeError:
        const char* overloads[] = {"unicode", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ItemBase.getParam", overloads);
        return 0;
}

static PyObject* Sbk_ItemBaseFunc_getParams(PyObject* self)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getParams()const
            // Begin code injection

            std::list<Param*> params = cppSelf->getParams();
            PyObject* ret = PyList_New((int) params.size());
            int idx = 0;
            for (std::list<Param*>::iterator it = params.begin(); it!=params.end(); ++it,++idx) {
            PyObject* item = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX], *it);
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

static PyObject* Sbk_ItemBaseFunc_getParent(PyObject* self)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getParent()const
            ItemBase * cppResult = const_cast<const ::ItemBase*>(cppSelf)->getParent();
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
}

static PyObject* Sbk_ItemBaseFunc_getScriptName(PyObject* self)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getScriptName()const
            QString cppResult = const_cast<const ::ItemBase*>(cppSelf)->getScriptName();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ItemBaseFunc_getUserKeyframes(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ItemBase.getUserKeyframes(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|O:getUserKeyframes", &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: getUserKeyframes(std::list<double>*,QString)const
    if (numArgs == 0) {
        overloadId = 0; // getUserKeyframes(std::list<double>*,QString)const
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))) {
        overloadId = 0; // getUserKeyframes(std::list<double>*,QString)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ItemBaseFunc_getUserKeyframes_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ItemBase.getUserKeyframes(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0]))))
                    goto Sbk_ItemBaseFunc_getUserKeyframes_TypeError;
            }
        }
        ::QString cppArg0 = QLatin1String(kPyParamViewIdxMain);
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // getUserKeyframes(std::list<double>*,QString)const
            // Begin code injection

            std::list<double> keys;
            cppSelf->getUserKeyframes(&keys,cppArg0);
            PyObject* ret = PyList_New((int) keys.size());
            int idx = 0;
            for (std::list<double>::iterator it = keys.begin(); it!=keys.end(); ++it,++idx) {
            PyList_SET_ITEM(ret, idx, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &*it));
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

    Sbk_ItemBaseFunc_getUserKeyframes_TypeError:
        const char* overloads[] = {"list, unicode = QLatin1String(kPyParamViewIdxMain)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ItemBase.getUserKeyframes", overloads);
        return 0;
}

static PyObject* Sbk_ItemBaseFunc_setIconFilePath(PyObject* self, PyObject* pyArg)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setIconFilePath(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // setIconFilePath(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ItemBaseFunc_setIconFilePath_TypeError;

    // Call function/method
    {
        ::QString cppArg0 = ::QString();
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setIconFilePath(QString)
            cppSelf->setIconFilePath(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ItemBaseFunc_setIconFilePath_TypeError:
        const char* overloads[] = {"unicode", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ItemBase.setIconFilePath", overloads);
        return 0;
}

static PyObject* Sbk_ItemBaseFunc_setLabel(PyObject* self, PyObject* pyArg)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setLabel(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // setLabel(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ItemBaseFunc_setLabel_TypeError;

    // Call function/method
    {
        ::QString cppArg0 = ::QString();
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setLabel(QString)
            cppSelf->setLabel(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ItemBaseFunc_setLabel_TypeError:
        const char* overloads[] = {"unicode", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ItemBase.setLabel", overloads);
        return 0;
}

static PyObject* Sbk_ItemBaseFunc_setUserKeyframe(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::ItemBase* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ItemBase.setUserKeyframe(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ItemBase.setUserKeyframe(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:setUserKeyframe", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: setUserKeyframe(double,QString)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // setUserKeyframe(double,QString)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
            overloadId = 0; // setUserKeyframe(double,QString)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ItemBaseFunc_setUserKeyframe_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ItemBase.setUserKeyframe(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                    goto Sbk_ItemBaseFunc_setUserKeyframe_TypeError;
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1 = QLatin1String(kPyParamViewSetSpecAll);
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setUserKeyframe(double,QString)
            cppSelf->setUserKeyframe(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ItemBaseFunc_setUserKeyframe_TypeError:
        const char* overloads[] = {"float, unicode = QLatin1String(kPyParamViewSetSpecAll)", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ItemBase.setUserKeyframe", overloads);
        return 0;
}

static PyMethodDef Sbk_ItemBase_methods[] = {
    {"deleteUserKeyframe", (PyCFunction)Sbk_ItemBaseFunc_deleteUserKeyframe, METH_VARARGS|METH_KEYWORDS},
    {"getChildren", (PyCFunction)Sbk_ItemBaseFunc_getChildren, METH_NOARGS},
    {"getIconFilePath", (PyCFunction)Sbk_ItemBaseFunc_getIconFilePath, METH_NOARGS},
    {"getIndexInParent", (PyCFunction)Sbk_ItemBaseFunc_getIndexInParent, METH_NOARGS},
    {"getLabel", (PyCFunction)Sbk_ItemBaseFunc_getLabel, METH_NOARGS},
    {"getParam", (PyCFunction)Sbk_ItemBaseFunc_getParam, METH_O},
    {"getParams", (PyCFunction)Sbk_ItemBaseFunc_getParams, METH_NOARGS},
    {"getParent", (PyCFunction)Sbk_ItemBaseFunc_getParent, METH_NOARGS},
    {"getScriptName", (PyCFunction)Sbk_ItemBaseFunc_getScriptName, METH_NOARGS},
    {"getUserKeyframes", (PyCFunction)Sbk_ItemBaseFunc_getUserKeyframes, METH_VARARGS|METH_KEYWORDS},
    {"setIconFilePath", (PyCFunction)Sbk_ItemBaseFunc_setIconFilePath, METH_O},
    {"setLabel", (PyCFunction)Sbk_ItemBaseFunc_setLabel, METH_O},
    {"setUserKeyframe", (PyCFunction)Sbk_ItemBaseFunc_setUserKeyframe, METH_VARARGS|METH_KEYWORDS},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_ItemBase_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_ItemBase_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_ItemBase_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.ItemBase",
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
    /*tp_traverse*/         Sbk_ItemBase_traverse,
    /*tp_clear*/            Sbk_ItemBase_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_ItemBase_methods,
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
static void ItemBase_PythonToCpp_ItemBase_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_ItemBase_Type, pyIn, cppOut);
}
static PythonToCppFunc is_ItemBase_PythonToCpp_ItemBase_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_ItemBase_Type))
        return ItemBase_PythonToCpp_ItemBase_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* ItemBase_PTR_CppToPython_ItemBase(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::ItemBase*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_ItemBase_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_ItemBase(PyObject* module)
{
    SbkNatronEngineTypes[SBK_ITEMBASE_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_ItemBase_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "ItemBase", "ItemBase*",
        &Sbk_ItemBase_Type, &Shiboken::callCppDestructor< ::ItemBase >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_ItemBase_Type,
        ItemBase_PythonToCpp_ItemBase_PTR,
        is_ItemBase_PythonToCpp_ItemBase_PTR_Convertible,
        ItemBase_PTR_CppToPython_ItemBase);

    Shiboken::Conversions::registerConverterName(converter, "ItemBase");
    Shiboken::Conversions::registerConverterName(converter, "ItemBase*");
    Shiboken::Conversions::registerConverterName(converter, "ItemBase&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::ItemBase).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::ItemBaseWrapper).name());



    ItemBaseWrapper::pysideInitQtMetaTypes();
}
