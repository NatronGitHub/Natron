
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
#include "natronengine_python.h"

#include "choiceparam_wrapper.h"

// Extra includes
#include <ParameterWrapper.h>
#include <list>
#include <utility>
#include <vector>


// Native ---------------------------------------------------------

void ChoiceParamWrapper::pysideInitQtMetaTypes()
{
}

ChoiceParamWrapper::~ChoiceParamWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_ChoiceParamFunc_addAsDependencyOf(PyObject* self, PyObject* args)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "addAsDependencyOf", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: addAsDependencyOf(int,Param*,int)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX], (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
        overloadId = 0; // addAsDependencyOf(int,Param*,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_addAsDependencyOf_TypeError;

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
            // addAsDependencyOf(int,Param*,int)
            int cppResult = cppSelf->addAsDependencyOf(cppArg0, cppArg1, cppArg2);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ChoiceParamFunc_addAsDependencyOf_TypeError:
        const char* overloads[] = {"int, NatronEngine.Param, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ChoiceParam.addAsDependencyOf", overloads);
        return 0;
}

static PyObject* Sbk_ChoiceParamFunc_addOption(PyObject* self, PyObject* args)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "addOption", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: addOption(std::string,std::string)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
        overloadId = 0; // addOption(std::string,std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_addOption_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // addOption(std::string,std::string)
            cppSelf->addOption(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ChoiceParamFunc_addOption_TypeError:
        const char* overloads[] = {"std::string, std::string", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ChoiceParam.addOption", overloads);
        return 0;
}

static PyObject* Sbk_ChoiceParamFunc_get(PyObject* self, PyObject* args)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "get", 0, 1, &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: get()const
    // 1: get(double)const
    if (numArgs == 0) {
        overloadId = 0; // get()const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        overloadId = 1; // get(double)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_get_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // get() const
        {

            if (!PyErr_Occurred()) {
                // get()const
                int cppResult = const_cast<const ::ChoiceParamWrapper*>(cppSelf)->get();
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
            }
            break;
        }
        case 1: // get(double frame) const
        {
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // get(double)const
                int cppResult = const_cast<const ::ChoiceParamWrapper*>(cppSelf)->get(cppArg0);
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

    Sbk_ChoiceParamFunc_get_TypeError:
        const char* overloads[] = {"", "float", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ChoiceParam.get", overloads);
        return 0;
}

static PyObject* Sbk_ChoiceParamFunc_getDefaultValue(PyObject* self)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getDefaultValue()const
            int cppResult = const_cast<const ::ChoiceParamWrapper*>(cppSelf)->getDefaultValue();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ChoiceParamFunc_getNumOptions(PyObject* self)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNumOptions()const
            int cppResult = const_cast<const ::ChoiceParamWrapper*>(cppSelf)->getNumOptions();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ChoiceParamFunc_getOption(PyObject* self, PyObject* pyArg)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getOption(int)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // getOption(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_getOption_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getOption(int)const
            std::string cppResult = const_cast<const ::ChoiceParamWrapper*>(cppSelf)->getOption(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ChoiceParamFunc_getOption_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ChoiceParam.getOption", overloads);
        return 0;
}

static PyObject* Sbk_ChoiceParamFunc_getOptions(PyObject* self)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getOptions()const
            std::vector<std::string > cppResult = const_cast<const ::ChoiceParamWrapper*>(cppSelf)->getOptions();
            pyResult = Shiboken::Conversions::copyToPython(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_VECTOR_STD_STRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ChoiceParamFunc_getValue(PyObject* self)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getValue()const
            int cppResult = const_cast<const ::ChoiceParamWrapper*>(cppSelf)->getValue();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ChoiceParamFunc_getValueAtTime(PyObject* self, PyObject* pyArg)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getValueAtTime(double)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArg)))) {
        overloadId = 0; // getValueAtTime(double)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_getValueAtTime_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getValueAtTime(double)const
            int cppResult = const_cast<const ::ChoiceParamWrapper*>(cppSelf)->getValueAtTime(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ChoiceParamFunc_getValueAtTime_TypeError:
        const char* overloads[] = {"float", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ChoiceParam.getValueAtTime", overloads);
        return 0;
}

static PyObject* Sbk_ChoiceParamFunc_restoreDefaultValue(PyObject* self)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // restoreDefaultValue()
            cppSelf->restoreDefaultValue();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_ChoiceParamFunc_set(PyObject* self, PyObject* args)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "set", 1, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: set(std::string)
    // 1: set(int)
    // 2: set(int,int)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 1; // set(int)
        } else if (numArgs == 2
            && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            overloadId = 2; // set(int,int)
        }
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))) {
        overloadId = 0; // set(std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_set_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // set(const std::string & label)
        {
            ::std::string cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // set(std::string)
                cppSelf->set(cppArg0);
            }
            break;
        }
        case 1: // set(int x)
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // set(int)
                cppSelf->set(cppArg0);
            }
            break;
        }
        case 2: // set(int x, int frame)
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // set(int,int)
                cppSelf->set(cppArg0, cppArg1);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ChoiceParamFunc_set_TypeError:
        const char* overloads[] = {"std::string", "int", "int, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ChoiceParam.set", overloads);
        return 0;
}

static PyObject* Sbk_ChoiceParamFunc_setDefaultValue(PyObject* self, PyObject* pyArg)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setDefaultValue(std::string)
    // 1: setDefaultValue(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 1; // setDefaultValue(int)
    } else if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArg)))) {
        overloadId = 0; // setDefaultValue(std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_setDefaultValue_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // setDefaultValue(const std::string & value)
        {
            ::std::string cppArg0;
            pythonToCpp(pyArg, &cppArg0);

            if (!PyErr_Occurred()) {
                // setDefaultValue(std::string)
                cppSelf->setDefaultValue(cppArg0);
            }
            break;
        }
        case 1: // setDefaultValue(int value)
        {
            int cppArg0;
            pythonToCpp(pyArg, &cppArg0);

            if (!PyErr_Occurred()) {
                // setDefaultValue(int)
                cppSelf->setDefaultValue(cppArg0);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ChoiceParamFunc_setDefaultValue_TypeError:
        const char* overloads[] = {"std::string", "int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ChoiceParam.setDefaultValue", overloads);
        return 0;
}

static PyObject* Sbk_ChoiceParamFunc_setOptions(PyObject* self, PyObject* pyArg)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setOptions(std::list<std::pair<std::string,std::string> >)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_PAIR_STD_STRING_STD_STRING_IDX], (pyArg)))) {
        overloadId = 0; // setOptions(std::list<std::pair<std::string,std::string> >)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_setOptions_TypeError;

    // Call function/method
    {
        ::std::list<std::pair<std::string, std::string > > cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setOptions(std::list<std::pair<std::string,std::string> >)
            cppSelf->setOptions(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ChoiceParamFunc_setOptions_TypeError:
        const char* overloads[] = {"list", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ChoiceParam.setOptions", overloads);
        return 0;
}

static PyObject* Sbk_ChoiceParamFunc_setValue(PyObject* self, PyObject* pyArg)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setValue(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // setValue(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_setValue_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setValue(int)
            cppSelf->setValue(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ChoiceParamFunc_setValue_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ChoiceParam.setValue", overloads);
        return 0;
}

static PyObject* Sbk_ChoiceParamFunc_setValueAtTime(PyObject* self, PyObject* args)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setValueAtTime", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: setValueAtTime(int,int)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
        overloadId = 0; // setValueAtTime(int,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_setValueAtTime_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setValueAtTime(int,int)
            cppSelf->setValueAtTime(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ChoiceParamFunc_setValueAtTime_TypeError:
        const char* overloads[] = {"int, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ChoiceParam.setValueAtTime", overloads);
        return 0;
}

static PyMethodDef Sbk_ChoiceParam_methods[] = {
    {"addAsDependencyOf", (PyCFunction)Sbk_ChoiceParamFunc_addAsDependencyOf, METH_VARARGS},
    {"addOption", (PyCFunction)Sbk_ChoiceParamFunc_addOption, METH_VARARGS},
    {"get", (PyCFunction)Sbk_ChoiceParamFunc_get, METH_VARARGS},
    {"getDefaultValue", (PyCFunction)Sbk_ChoiceParamFunc_getDefaultValue, METH_NOARGS},
    {"getNumOptions", (PyCFunction)Sbk_ChoiceParamFunc_getNumOptions, METH_NOARGS},
    {"getOption", (PyCFunction)Sbk_ChoiceParamFunc_getOption, METH_O},
    {"getOptions", (PyCFunction)Sbk_ChoiceParamFunc_getOptions, METH_NOARGS},
    {"getValue", (PyCFunction)Sbk_ChoiceParamFunc_getValue, METH_NOARGS},
    {"getValueAtTime", (PyCFunction)Sbk_ChoiceParamFunc_getValueAtTime, METH_O},
    {"restoreDefaultValue", (PyCFunction)Sbk_ChoiceParamFunc_restoreDefaultValue, METH_NOARGS},
    {"set", (PyCFunction)Sbk_ChoiceParamFunc_set, METH_VARARGS},
    {"setDefaultValue", (PyCFunction)Sbk_ChoiceParamFunc_setDefaultValue, METH_O},
    {"setOptions", (PyCFunction)Sbk_ChoiceParamFunc_setOptions, METH_O},
    {"setValue", (PyCFunction)Sbk_ChoiceParamFunc_setValue, METH_O},
    {"setValueAtTime", (PyCFunction)Sbk_ChoiceParamFunc_setValueAtTime, METH_VARARGS},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_ChoiceParam_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_ChoiceParam_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_ChoiceParam_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.ChoiceParam",
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
    /*tp_traverse*/         Sbk_ChoiceParam_traverse,
    /*tp_clear*/            Sbk_ChoiceParam_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_ChoiceParam_methods,
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

static void* Sbk_ChoiceParam_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::ChoiceParam*>(reinterpret_cast< ::Param*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void ChoiceParam_PythonToCpp_ChoiceParam_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_ChoiceParam_Type, pyIn, cppOut);
}
static PythonToCppFunc is_ChoiceParam_PythonToCpp_ChoiceParam_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_ChoiceParam_Type))
        return ChoiceParam_PythonToCpp_ChoiceParam_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* ChoiceParam_PTR_CppToPython_ChoiceParam(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::ChoiceParam*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_ChoiceParam_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_ChoiceParam(PyObject* module)
{
    SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_ChoiceParam_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "ChoiceParam", "ChoiceParam*",
        &Sbk_ChoiceParam_Type, &Shiboken::callCppDestructor< ::ChoiceParam >, (SbkObjectType*)SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX])) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_ChoiceParam_Type,
        ChoiceParam_PythonToCpp_ChoiceParam_PTR,
        is_ChoiceParam_PythonToCpp_ChoiceParam_PTR_Convertible,
        ChoiceParam_PTR_CppToPython_ChoiceParam);

    Shiboken::Conversions::registerConverterName(converter, "ChoiceParam");
    Shiboken::Conversions::registerConverterName(converter, "ChoiceParam*");
    Shiboken::Conversions::registerConverterName(converter, "ChoiceParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::ChoiceParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::ChoiceParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_ChoiceParam_Type, &Sbk_ChoiceParam_typeDiscovery);


    ChoiceParamWrapper::pysideInitQtMetaTypes();
}
