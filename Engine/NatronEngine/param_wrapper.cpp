
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

#include "param_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING
#include <PyParameter.h>


// Native ---------------------------------------------------------

void ParamWrapper::pysideInitQtMetaTypes()
{
}

ParamWrapper::~ParamWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_ParamFunc__addAsDependencyOf(PyObject* self, PyObject* args)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "_addAsDependencyOf", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: _addAsDependencyOf(int,Param*,int)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX], (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
        overloadId = 0; // _addAsDependencyOf(int,Param*,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc__addAsDependencyOf_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return 0;
        ::Param* cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        int cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // _addAsDependencyOf(int,Param*,int)
            ((::ParamWrapper*) cppSelf)->ParamWrapper::_addAsDependencyOf_protected(cppArg0, cppArg1, cppArg2);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc__addAsDependencyOf_TypeError:
        const char* overloads[] = {"int, NatronEngine.Param, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Param._addAsDependencyOf", overloads);
        return 0;
}

static PyObject* Sbk_ParamFunc_copy(PyObject* self, PyObject* args, PyObject* kwds)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.copy(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.copy(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:copy", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: copy(Param*,int)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX], (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // copy(Param*,int)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            overloadId = 0; // copy(Param*,int)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_copy_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "dimension");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.copy(): got multiple values for keyword argument 'dimension'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                    goto Sbk_ParamFunc_copy_TypeError;
            }
        }
        if (!Shiboken::Object::isValid(pyArgs[0]))
            return 0;
        ::Param* cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = -1;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // copy(Param*,int)
            bool cppResult = cppSelf->copy(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ParamFunc_copy_TypeError:
        const char* overloads[] = {"NatronEngine.Param, int = -1", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Param.copy", overloads);
        return 0;
}

static PyObject* Sbk_ParamFunc_curve(PyObject* self, PyObject* args, PyObject* kwds)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.curve(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.curve(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:curve", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: curve(double,int)const
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // curve(double,int)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            overloadId = 0; // curve(double,int)const
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_curve_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "dimension");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.curve(): got multiple values for keyword argument 'dimension'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                    goto Sbk_ParamFunc_curve_TypeError;
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // curve(double,int)const
            double cppResult = const_cast<const ::ParamWrapper*>(cppSelf)->curve(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ParamFunc_curve_TypeError:
        const char* overloads[] = {"float, int = 0", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Param.curve", overloads);
        return 0;
}

static PyObject* Sbk_ParamFunc_getAddNewLine(PyObject* self)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getAddNewLine()
            bool cppResult = cppSelf->getAddNewLine();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ParamFunc_getCanAnimate(PyObject* self)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getCanAnimate()const
            bool cppResult = const_cast<const ::ParamWrapper*>(cppSelf)->getCanAnimate();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ParamFunc_getEvaluateOnChange(PyObject* self)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getEvaluateOnChange()const
            bool cppResult = const_cast<const ::ParamWrapper*>(cppSelf)->getEvaluateOnChange();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ParamFunc_getHelp(PyObject* self)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getHelp()const
            std::string cppResult = const_cast<const ::ParamWrapper*>(cppSelf)->getHelp();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ParamFunc_getIsAnimationEnabled(PyObject* self)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getIsAnimationEnabled()const
            bool cppResult = const_cast<const ::ParamWrapper*>(cppSelf)->getIsAnimationEnabled();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ParamFunc_getIsEnabled(PyObject* self, PyObject* args, PyObject* kwds)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.getIsEnabled(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|O:getIsEnabled", &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: getIsEnabled(int)const
    if (numArgs == 0) {
        overloadId = 0; // getIsEnabled(int)const
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        overloadId = 0; // getIsEnabled(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_getIsEnabled_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "dimension");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.getIsEnabled(): got multiple values for keyword argument 'dimension'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0]))))
                    goto Sbk_ParamFunc_getIsEnabled_TypeError;
            }
        }
        int cppArg0 = 0;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // getIsEnabled(int)const
            bool cppResult = const_cast<const ::ParamWrapper*>(cppSelf)->getIsEnabled(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ParamFunc_getIsEnabled_TypeError:
        const char* overloads[] = {"int = 0", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Param.getIsEnabled", overloads);
        return 0;
}

static PyObject* Sbk_ParamFunc_getIsPersistant(PyObject* self)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getIsPersistant()const
            bool cppResult = const_cast<const ::ParamWrapper*>(cppSelf)->getIsPersistant();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ParamFunc_getIsVisible(PyObject* self)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getIsVisible()const
            bool cppResult = const_cast<const ::ParamWrapper*>(cppSelf)->getIsVisible();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ParamFunc_getLabel(PyObject* self)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getLabel()const
            std::string cppResult = const_cast<const ::ParamWrapper*>(cppSelf)->getLabel();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ParamFunc_getNumDimensions(PyObject* self)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNumDimensions()const
            int cppResult = const_cast<const ::ParamWrapper*>(cppSelf)->getNumDimensions();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ParamFunc_getParent(PyObject* self)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getParent()const
            Param * cppResult = const_cast<const ::ParamWrapper*>(cppSelf)->getParent();
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
}

static PyObject* Sbk_ParamFunc_getScriptName(PyObject* self)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getScriptName()const
            std::string cppResult = const_cast<const ::ParamWrapper*>(cppSelf)->getScriptName();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ParamFunc_getTypeName(PyObject* self)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getTypeName()const
            std::string cppResult = const_cast<const ::ParamWrapper*>(cppSelf)->getTypeName();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ParamFunc_random(PyObject* self, PyObject* args, PyObject* kwds)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.random(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:random", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: random(double,double)const
    // 1: random(uint)const
    if (numArgs == 0) {
        overloadId = 0; // random(double,double)const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<unsigned int>(), (pyArgs[0])))) {
        overloadId = 1; // random(uint)const
    } else if (PyFloat_Check(pyArgs[0]) && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // random(double,double)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
            overloadId = 0; // random(double,double)const
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_random_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // random(double min, double max) const
        {
            if (kwds) {
                PyObject* value = PyDict_GetItemString(kwds, "min");
                if (value && pyArgs[0]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.random(): got multiple values for keyword argument 'min'.");
                    return 0;
                } else if (value) {
                    pyArgs[0] = value;
                    if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0]))))
                        goto Sbk_ParamFunc_random_TypeError;
                }
                value = PyDict_GetItemString(kwds, "max");
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.random(): got multiple values for keyword argument 'max'.");
                    return 0;
                } else if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1]))))
                        goto Sbk_ParamFunc_random_TypeError;
                }
            }
            double cppArg0 = 0.;
            if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1 = 1.;
            if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // random(double,double)const
                double cppResult = const_cast<const ::ParamWrapper*>(cppSelf)->random(cppArg0, cppArg1);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
            }
            break;
        }
        case 1: // random(unsigned int seed) const
        {
            unsigned int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // random(uint)const
                double cppResult = const_cast<const ::ParamWrapper*>(cppSelf)->random(cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ParamFunc_random_TypeError:
        const char* overloads[] = {"float = 0., float = 1.", "unsigned int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Param.random", overloads);
        return 0;
}

static PyObject* Sbk_ParamFunc_randomInt(PyObject* self, PyObject* args)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "randomInt", 1, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: randomInt(int,int)
    // 1: randomInt(uint)const
    if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<unsigned int>(), (pyArgs[0])))) {
        overloadId = 1; // randomInt(uint)const
    } else if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
        overloadId = 0; // randomInt(int,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_randomInt_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // randomInt(int min, int max)
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // randomInt(int,int)
                int cppResult = cppSelf->randomInt(cppArg0, cppArg1);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
            }
            break;
        }
        case 1: // randomInt(unsigned int seed) const
        {
            unsigned int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // randomInt(uint)const
                int cppResult = const_cast<const ::ParamWrapper*>(cppSelf)->randomInt(cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ParamFunc_randomInt_TypeError:
        const char* overloads[] = {"int, int", "unsigned int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Param.randomInt", overloads);
        return 0;
}

static PyObject* Sbk_ParamFunc_setAddNewLine(PyObject* self, PyObject* pyArg)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setAddNewLine(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setAddNewLine(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_setAddNewLine_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setAddNewLine(bool)
            cppSelf->setAddNewLine(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_setAddNewLine_TypeError:
        const char* overloads[] = {"bool", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Param.setAddNewLine", overloads);
        return 0;
}

static PyObject* Sbk_ParamFunc_setAnimationEnabled(PyObject* self, PyObject* pyArg)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setAnimationEnabled(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setAnimationEnabled(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_setAnimationEnabled_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setAnimationEnabled(bool)
            cppSelf->setAnimationEnabled(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_setAnimationEnabled_TypeError:
        const char* overloads[] = {"bool", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Param.setAnimationEnabled", overloads);
        return 0;
}

static PyObject* Sbk_ParamFunc_setAsAlias(PyObject* self, PyObject* pyArg)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setAsAlias(Param*)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX], (pyArg)))) {
        overloadId = 0; // setAsAlias(Param*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_setAsAlias_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArg))
            return 0;
        ::Param* cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setAsAlias(Param*)
            bool cppResult = cppSelf->setAsAlias(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ParamFunc_setAsAlias_TypeError:
        const char* overloads[] = {"NatronEngine.Param", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Param.setAsAlias", overloads);
        return 0;
}

static PyObject* Sbk_ParamFunc_setEnabled(PyObject* self, PyObject* args, PyObject* kwds)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.setEnabled(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.setEnabled(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:setEnabled", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: setEnabled(bool,int)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // setEnabled(bool,int)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            overloadId = 0; // setEnabled(bool,int)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_setEnabled_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "dimension");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.setEnabled(): got multiple values for keyword argument 'dimension'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                    goto Sbk_ParamFunc_setEnabled_TypeError;
            }
        }
        bool cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setEnabled(bool,int)
            cppSelf->setEnabled(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_setEnabled_TypeError:
        const char* overloads[] = {"bool, int = 0", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Param.setEnabled", overloads);
        return 0;
}

static PyObject* Sbk_ParamFunc_setEnabledByDefault(PyObject* self, PyObject* args, PyObject* kwds)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.setEnabledByDefault(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|O:setEnabledByDefault", &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: setEnabledByDefault(bool)
    if (numArgs == 0) {
        overloadId = 0; // setEnabledByDefault(bool)
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[0])))) {
        overloadId = 0; // setEnabledByDefault(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_setEnabledByDefault_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "enabled");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.setEnabledByDefault(): got multiple values for keyword argument 'enabled'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[0]))))
                    goto Sbk_ParamFunc_setEnabledByDefault_TypeError;
            }
        }
        bool cppArg0 = 0;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // setEnabledByDefault(bool)
            cppSelf->setEnabledByDefault(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_setEnabledByDefault_TypeError:
        const char* overloads[] = {"bool = 0", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Param.setEnabledByDefault", overloads);
        return 0;
}

static PyObject* Sbk_ParamFunc_setEvaluateOnChange(PyObject* self, PyObject* pyArg)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setEvaluateOnChange(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setEvaluateOnChange(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_setEvaluateOnChange_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setEvaluateOnChange(bool)
            cppSelf->setEvaluateOnChange(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_setEvaluateOnChange_TypeError:
        const char* overloads[] = {"bool", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Param.setEvaluateOnChange", overloads);
        return 0;
}

static PyObject* Sbk_ParamFunc_setHelp(PyObject* self, PyObject* pyArg)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setHelp(std::string)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArg)))) {
        overloadId = 0; // setHelp(std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_setHelp_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setHelp(std::string)
            cppSelf->setHelp(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_setHelp_TypeError:
        const char* overloads[] = {"std::string", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Param.setHelp", overloads);
        return 0;
}

static PyObject* Sbk_ParamFunc_setPersistant(PyObject* self, PyObject* pyArg)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setPersistant(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setPersistant(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_setPersistant_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setPersistant(bool)
            cppSelf->setPersistant(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_setPersistant_TypeError:
        const char* overloads[] = {"bool", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Param.setPersistant", overloads);
        return 0;
}

static PyObject* Sbk_ParamFunc_setVisible(PyObject* self, PyObject* pyArg)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setVisible(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setVisible(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_setVisible_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setVisible(bool)
            cppSelf->setVisible(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_setVisible_TypeError:
        const char* overloads[] = {"bool", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Param.setVisible", overloads);
        return 0;
}

static PyObject* Sbk_ParamFunc_setVisibleByDefault(PyObject* self, PyObject* pyArg)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setVisibleByDefault(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setVisibleByDefault(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_setVisibleByDefault_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setVisibleByDefault(bool)
            cppSelf->setVisibleByDefault(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_setVisibleByDefault_TypeError:
        const char* overloads[] = {"bool", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Param.setVisibleByDefault", overloads);
        return 0;
}

static PyObject* Sbk_ParamFunc_slaveTo(PyObject* self, PyObject* args)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "slaveTo", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: slaveTo(Param*,int,int)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
        overloadId = 0; // slaveTo(Param*,int,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_slaveTo_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArgs[0]))
            return 0;
        ::Param* cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        int cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // slaveTo(Param*,int,int)
            bool cppResult = cppSelf->slaveTo(cppArg0, cppArg1, cppArg2);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ParamFunc_slaveTo_TypeError:
        const char* overloads[] = {"NatronEngine.Param, int, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Param.slaveTo", overloads);
        return 0;
}

static PyObject* Sbk_ParamFunc_unslave(PyObject* self, PyObject* pyArg)
{
    ParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ParamWrapper*)((::Param*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: unslave(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // unslave(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_unslave_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // unslave(int)
            cppSelf->unslave(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_unslave_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Param.unslave", overloads);
        return 0;
}

static PyMethodDef Sbk_Param_methods[] = {
    {"_addAsDependencyOf", (PyCFunction)Sbk_ParamFunc__addAsDependencyOf, METH_VARARGS},
    {"copy", (PyCFunction)Sbk_ParamFunc_copy, METH_VARARGS|METH_KEYWORDS},
    {"curve", (PyCFunction)Sbk_ParamFunc_curve, METH_VARARGS|METH_KEYWORDS},
    {"getAddNewLine", (PyCFunction)Sbk_ParamFunc_getAddNewLine, METH_NOARGS},
    {"getCanAnimate", (PyCFunction)Sbk_ParamFunc_getCanAnimate, METH_NOARGS},
    {"getEvaluateOnChange", (PyCFunction)Sbk_ParamFunc_getEvaluateOnChange, METH_NOARGS},
    {"getHelp", (PyCFunction)Sbk_ParamFunc_getHelp, METH_NOARGS},
    {"getIsAnimationEnabled", (PyCFunction)Sbk_ParamFunc_getIsAnimationEnabled, METH_NOARGS},
    {"getIsEnabled", (PyCFunction)Sbk_ParamFunc_getIsEnabled, METH_VARARGS|METH_KEYWORDS},
    {"getIsPersistant", (PyCFunction)Sbk_ParamFunc_getIsPersistant, METH_NOARGS},
    {"getIsVisible", (PyCFunction)Sbk_ParamFunc_getIsVisible, METH_NOARGS},
    {"getLabel", (PyCFunction)Sbk_ParamFunc_getLabel, METH_NOARGS},
    {"getNumDimensions", (PyCFunction)Sbk_ParamFunc_getNumDimensions, METH_NOARGS},
    {"getParent", (PyCFunction)Sbk_ParamFunc_getParent, METH_NOARGS},
    {"getScriptName", (PyCFunction)Sbk_ParamFunc_getScriptName, METH_NOARGS},
    {"getTypeName", (PyCFunction)Sbk_ParamFunc_getTypeName, METH_NOARGS},
    {"random", (PyCFunction)Sbk_ParamFunc_random, METH_VARARGS|METH_KEYWORDS},
    {"randomInt", (PyCFunction)Sbk_ParamFunc_randomInt, METH_VARARGS},
    {"setAddNewLine", (PyCFunction)Sbk_ParamFunc_setAddNewLine, METH_O},
    {"setAnimationEnabled", (PyCFunction)Sbk_ParamFunc_setAnimationEnabled, METH_O},
    {"setAsAlias", (PyCFunction)Sbk_ParamFunc_setAsAlias, METH_O},
    {"setEnabled", (PyCFunction)Sbk_ParamFunc_setEnabled, METH_VARARGS|METH_KEYWORDS},
    {"setEnabledByDefault", (PyCFunction)Sbk_ParamFunc_setEnabledByDefault, METH_VARARGS|METH_KEYWORDS},
    {"setEvaluateOnChange", (PyCFunction)Sbk_ParamFunc_setEvaluateOnChange, METH_O},
    {"setHelp", (PyCFunction)Sbk_ParamFunc_setHelp, METH_O},
    {"setPersistant", (PyCFunction)Sbk_ParamFunc_setPersistant, METH_O},
    {"setVisible", (PyCFunction)Sbk_ParamFunc_setVisible, METH_O},
    {"setVisibleByDefault", (PyCFunction)Sbk_ParamFunc_setVisibleByDefault, METH_O},
    {"slaveTo", (PyCFunction)Sbk_ParamFunc_slaveTo, METH_VARARGS},
    {"unslave", (PyCFunction)Sbk_ParamFunc_unslave, METH_O},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_Param_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_Param_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_Param_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.Param",
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
    /*tp_traverse*/         Sbk_Param_traverse,
    /*tp_clear*/            Sbk_Param_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_Param_methods,
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
static void Param_PythonToCpp_Param_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_Param_Type, pyIn, cppOut);
}
static PythonToCppFunc is_Param_PythonToCpp_Param_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_Param_Type))
        return Param_PythonToCpp_Param_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* Param_PTR_CppToPython_Param(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::Param*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_Param_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_Param(PyObject* module)
{
    SbkNatronEngineTypes[SBK_PARAM_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_Param_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "Param", "Param*",
        &Sbk_Param_Type, &Shiboken::callCppDestructor< ::Param >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_Param_Type,
        Param_PythonToCpp_Param_PTR,
        is_Param_PythonToCpp_Param_PTR_Convertible,
        Param_PTR_CppToPython_Param);

    Shiboken::Conversions::registerConverterName(converter, "Param");
    Shiboken::Conversions::registerConverterName(converter, "Param*");
    Shiboken::Conversions::registerConverterName(converter, "Param&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::Param).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::ParamWrapper).name());



    ParamWrapper::pysideInitQtMetaTypes();
}
