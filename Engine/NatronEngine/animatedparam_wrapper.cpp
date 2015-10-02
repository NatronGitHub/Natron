
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

#include "animatedparam_wrapper.h"

// Extra includes
#include <ParameterWrapper.h>


// Native ---------------------------------------------------------

void AnimatedParamWrapper::pysideInitQtMetaTypes()
{
}

AnimatedParamWrapper::~AnimatedParamWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_AnimatedParamFunc_deleteValueAtTime(PyObject* self, PyObject* args, PyObject* kwds)
{
    AnimatedParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (AnimatedParamWrapper*)((::AnimatedParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.deleteValueAtTime(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.deleteValueAtTime(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:deleteValueAtTime", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: deleteValueAtTime(int,int)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // deleteValueAtTime(int,int)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            overloadId = 0; // deleteValueAtTime(int,int)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AnimatedParamFunc_deleteValueAtTime_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "dimension");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.deleteValueAtTime(): got multiple values for keyword argument 'dimension'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                    goto Sbk_AnimatedParamFunc_deleteValueAtTime_TypeError;
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // deleteValueAtTime(int,int)
            cppSelf->deleteValueAtTime(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_AnimatedParamFunc_deleteValueAtTime_TypeError:
        const char* overloads[] = {"int, int = 0", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.AnimatedParam.deleteValueAtTime", overloads);
        return 0;
}

static PyObject* Sbk_AnimatedParamFunc_getCurrentTime(PyObject* self)
{
    AnimatedParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (AnimatedParamWrapper*)((::AnimatedParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getCurrentTime()const
            int cppResult = const_cast<const ::AnimatedParamWrapper*>(cppSelf)->getCurrentTime();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_AnimatedParamFunc_getDerivativeAtTime(PyObject* self, PyObject* args, PyObject* kwds)
{
    AnimatedParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (AnimatedParamWrapper*)((::AnimatedParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getDerivativeAtTime(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getDerivativeAtTime(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:getDerivativeAtTime", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: getDerivativeAtTime(double,int)const
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // getDerivativeAtTime(double,int)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            overloadId = 0; // getDerivativeAtTime(double,int)const
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AnimatedParamFunc_getDerivativeAtTime_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "dimension");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getDerivativeAtTime(): got multiple values for keyword argument 'dimension'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                    goto Sbk_AnimatedParamFunc_getDerivativeAtTime_TypeError;
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getDerivativeAtTime(double,int)const
            double cppResult = const_cast<const ::AnimatedParamWrapper*>(cppSelf)->getDerivativeAtTime(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_AnimatedParamFunc_getDerivativeAtTime_TypeError:
        const char* overloads[] = {"float, int = 0", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.AnimatedParam.getDerivativeAtTime", overloads);
        return 0;
}

static PyObject* Sbk_AnimatedParamFunc_getExpression(PyObject* self, PyObject* pyArg)
{
    AnimatedParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (AnimatedParamWrapper*)((::AnimatedParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getExpression(int,bool*)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // getExpression(int,bool*)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AnimatedParamFunc_getExpression_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getExpression(int,bool*)const
            // Begin code injection

            bool hasRetVar;
            std::string cppResult = cppSelf->getExpression(cppArg0,&hasRetVar);
            pyResult = PyTuple_New(2);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &hasRetVar));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_AnimatedParamFunc_getExpression_TypeError:
        const char* overloads[] = {"int, PySide.QtCore.bool", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.AnimatedParam.getExpression", overloads);
        return 0;
}

static PyObject* Sbk_AnimatedParamFunc_getIntegrateFromTimeToTime(PyObject* self, PyObject* args, PyObject* kwds)
{
    AnimatedParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (AnimatedParamWrapper*)((::AnimatedParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 3) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getIntegrateFromTimeToTime(): too many arguments");
        return 0;
    } else if (numArgs < 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getIntegrateFromTimeToTime(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOO:getIntegrateFromTimeToTime", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: getIntegrateFromTimeToTime(double,double,int)const
    if (numArgs >= 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        if (numArgs == 2) {
            overloadId = 0; // getIntegrateFromTimeToTime(double,double,int)const
        } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
            overloadId = 0; // getIntegrateFromTimeToTime(double,double,int)const
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AnimatedParamFunc_getIntegrateFromTimeToTime_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "dimension");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getIntegrateFromTimeToTime(): got multiple values for keyword argument 'dimension'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2]))))
                    goto Sbk_AnimatedParamFunc_getIntegrateFromTimeToTime_TypeError;
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        int cppArg2 = 0;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // getIntegrateFromTimeToTime(double,double,int)const
            double cppResult = const_cast<const ::AnimatedParamWrapper*>(cppSelf)->getIntegrateFromTimeToTime(cppArg0, cppArg1, cppArg2);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_AnimatedParamFunc_getIntegrateFromTimeToTime_TypeError:
        const char* overloads[] = {"float, float, int = 0", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.AnimatedParam.getIntegrateFromTimeToTime", overloads);
        return 0;
}

static PyObject* Sbk_AnimatedParamFunc_getIsAnimated(PyObject* self, PyObject* args, PyObject* kwds)
{
    AnimatedParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (AnimatedParamWrapper*)((::AnimatedParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getIsAnimated(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|O:getIsAnimated", &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: getIsAnimated(int)const
    if (numArgs == 0) {
        overloadId = 0; // getIsAnimated(int)const
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        overloadId = 0; // getIsAnimated(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AnimatedParamFunc_getIsAnimated_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "dimension");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getIsAnimated(): got multiple values for keyword argument 'dimension'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0]))))
                    goto Sbk_AnimatedParamFunc_getIsAnimated_TypeError;
            }
        }
        int cppArg0 = 0;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // getIsAnimated(int)const
            bool cppResult = const_cast<const ::AnimatedParamWrapper*>(cppSelf)->getIsAnimated(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_AnimatedParamFunc_getIsAnimated_TypeError:
        const char* overloads[] = {"int = 0", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.AnimatedParam.getIsAnimated", overloads);
        return 0;
}

static PyObject* Sbk_AnimatedParamFunc_getKeyIndex(PyObject* self, PyObject* args, PyObject* kwds)
{
    AnimatedParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (AnimatedParamWrapper*)((::AnimatedParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getKeyIndex(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getKeyIndex(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:getKeyIndex", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: getKeyIndex(int,int)const
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // getKeyIndex(int,int)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            overloadId = 0; // getKeyIndex(int,int)const
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AnimatedParamFunc_getKeyIndex_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "dimension");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getKeyIndex(): got multiple values for keyword argument 'dimension'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                    goto Sbk_AnimatedParamFunc_getKeyIndex_TypeError;
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getKeyIndex(int,int)const
            int cppResult = const_cast<const ::AnimatedParamWrapper*>(cppSelf)->getKeyIndex(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_AnimatedParamFunc_getKeyIndex_TypeError:
        const char* overloads[] = {"int, int = 0", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.AnimatedParam.getKeyIndex", overloads);
        return 0;
}

static PyObject* Sbk_AnimatedParamFunc_getKeyTime(PyObject* self, PyObject* args)
{
    AnimatedParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (AnimatedParamWrapper*)((::AnimatedParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "getKeyTime", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: getKeyTime(int,int,double*)const
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
        overloadId = 0; // getKeyTime(int,int,double*)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AnimatedParamFunc_getKeyTime_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getKeyTime(int,int,double*)const
            // Begin code injection

            double time;
            bool cppResult = cppSelf->getKeyTime(cppArg0, cppArg1,&time);
            pyResult = PyTuple_New(2);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &time));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_AnimatedParamFunc_getKeyTime_TypeError:
        const char* overloads[] = {"int, int, PySide.QtCore.double", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.AnimatedParam.getKeyTime", overloads);
        return 0;
}

static PyObject* Sbk_AnimatedParamFunc_getNumKeys(PyObject* self, PyObject* args, PyObject* kwds)
{
    AnimatedParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (AnimatedParamWrapper*)((::AnimatedParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getNumKeys(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|O:getNumKeys", &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: getNumKeys(int)const
    if (numArgs == 0) {
        overloadId = 0; // getNumKeys(int)const
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        overloadId = 0; // getNumKeys(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AnimatedParamFunc_getNumKeys_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "dimension");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getNumKeys(): got multiple values for keyword argument 'dimension'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0]))))
                    goto Sbk_AnimatedParamFunc_getNumKeys_TypeError;
            }
        }
        int cppArg0 = 0;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // getNumKeys(int)const
            int cppResult = const_cast<const ::AnimatedParamWrapper*>(cppSelf)->getNumKeys(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_AnimatedParamFunc_getNumKeys_TypeError:
        const char* overloads[] = {"int = 0", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.AnimatedParam.getNumKeys", overloads);
        return 0;
}

static PyObject* Sbk_AnimatedParamFunc_removeAnimation(PyObject* self, PyObject* args, PyObject* kwds)
{
    AnimatedParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (AnimatedParamWrapper*)((::AnimatedParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.removeAnimation(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|O:removeAnimation", &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: removeAnimation(int)
    if (numArgs == 0) {
        overloadId = 0; // removeAnimation(int)
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        overloadId = 0; // removeAnimation(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AnimatedParamFunc_removeAnimation_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "dimension");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.removeAnimation(): got multiple values for keyword argument 'dimension'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0]))))
                    goto Sbk_AnimatedParamFunc_removeAnimation_TypeError;
            }
        }
        int cppArg0 = 0;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // removeAnimation(int)
            cppSelf->removeAnimation(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_AnimatedParamFunc_removeAnimation_TypeError:
        const char* overloads[] = {"int = 0", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.AnimatedParam.removeAnimation", overloads);
        return 0;
}

static PyObject* Sbk_AnimatedParamFunc_setExpression(PyObject* self, PyObject* args, PyObject* kwds)
{
    AnimatedParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (AnimatedParamWrapper*)((::AnimatedParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 3) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.setExpression(): too many arguments");
        return 0;
    } else if (numArgs < 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.setExpression(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOO:setExpression", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: setExpression(std::string,bool,int)
    if (numArgs >= 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[1])))) {
        if (numArgs == 2) {
            overloadId = 0; // setExpression(std::string,bool,int)
        } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
            overloadId = 0; // setExpression(std::string,bool,int)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AnimatedParamFunc_setExpression_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "dimension");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.setExpression(): got multiple values for keyword argument 'dimension'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2]))))
                    goto Sbk_AnimatedParamFunc_setExpression_TypeError;
            }
        }
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        bool cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        int cppArg2 = 0;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // setExpression(std::string,bool,int)
            // Begin code injection

            bool cppResult = cppSelf->setExpression(cppArg0,cppArg1,cppArg2);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_AnimatedParamFunc_setExpression_TypeError:
        const char* overloads[] = {"std::string, bool, int = 0", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.AnimatedParam.setExpression", overloads);
        return 0;
}

static PyMethodDef Sbk_AnimatedParam_methods[] = {
    {"deleteValueAtTime", (PyCFunction)Sbk_AnimatedParamFunc_deleteValueAtTime, METH_VARARGS|METH_KEYWORDS},
    {"getCurrentTime", (PyCFunction)Sbk_AnimatedParamFunc_getCurrentTime, METH_NOARGS},
    {"getDerivativeAtTime", (PyCFunction)Sbk_AnimatedParamFunc_getDerivativeAtTime, METH_VARARGS|METH_KEYWORDS},
    {"getExpression", (PyCFunction)Sbk_AnimatedParamFunc_getExpression, METH_O},
    {"getIntegrateFromTimeToTime", (PyCFunction)Sbk_AnimatedParamFunc_getIntegrateFromTimeToTime, METH_VARARGS|METH_KEYWORDS},
    {"getIsAnimated", (PyCFunction)Sbk_AnimatedParamFunc_getIsAnimated, METH_VARARGS|METH_KEYWORDS},
    {"getKeyIndex", (PyCFunction)Sbk_AnimatedParamFunc_getKeyIndex, METH_VARARGS|METH_KEYWORDS},
    {"getKeyTime", (PyCFunction)Sbk_AnimatedParamFunc_getKeyTime, METH_VARARGS},
    {"getNumKeys", (PyCFunction)Sbk_AnimatedParamFunc_getNumKeys, METH_VARARGS|METH_KEYWORDS},
    {"removeAnimation", (PyCFunction)Sbk_AnimatedParamFunc_removeAnimation, METH_VARARGS|METH_KEYWORDS},
    {"setExpression", (PyCFunction)Sbk_AnimatedParamFunc_setExpression, METH_VARARGS|METH_KEYWORDS},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_AnimatedParam_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_AnimatedParam_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_AnimatedParam_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.AnimatedParam",
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
    /*tp_traverse*/         Sbk_AnimatedParam_traverse,
    /*tp_clear*/            Sbk_AnimatedParam_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_AnimatedParam_methods,
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

static void* Sbk_AnimatedParam_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::AnimatedParam*>(reinterpret_cast< ::Param*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void AnimatedParam_PythonToCpp_AnimatedParam_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_AnimatedParam_Type, pyIn, cppOut);
}
static PythonToCppFunc is_AnimatedParam_PythonToCpp_AnimatedParam_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_AnimatedParam_Type))
        return AnimatedParam_PythonToCpp_AnimatedParam_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* AnimatedParam_PTR_CppToPython_AnimatedParam(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::AnimatedParam*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_AnimatedParam_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_AnimatedParam(PyObject* module)
{
    SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_AnimatedParam_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "AnimatedParam", "AnimatedParam*",
        &Sbk_AnimatedParam_Type, &Shiboken::callCppDestructor< ::AnimatedParam >, (SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX])) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_AnimatedParam_Type,
        AnimatedParam_PythonToCpp_AnimatedParam_PTR,
        is_AnimatedParam_PythonToCpp_AnimatedParam_PTR_Convertible,
        AnimatedParam_PTR_CppToPython_AnimatedParam);

    Shiboken::Conversions::registerConverterName(converter, "AnimatedParam");
    Shiboken::Conversions::registerConverterName(converter, "AnimatedParam*");
    Shiboken::Conversions::registerConverterName(converter, "AnimatedParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::AnimatedParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::AnimatedParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_AnimatedParam_Type, &Sbk_AnimatedParam_typeDiscovery);


    AnimatedParamWrapper::pysideInitQtMetaTypes();
}
