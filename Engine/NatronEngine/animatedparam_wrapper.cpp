
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

#include "animatedparam_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING
#include <PyAppInstance.h>
#include <PyItemsTable.h>
#include <PyNode.h>
#include <PyParameter.h>


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
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 3) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.deleteValueAtTime(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.deleteValueAtTime(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOO:deleteValueAtTime", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: deleteValueAtTime(double,int,QString)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // deleteValueAtTime(double,int,QString)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            if (numArgs == 2) {
                overloadId = 0; // deleteValueAtTime(double,int,QString)
            } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2])))) {
                overloadId = 0; // deleteValueAtTime(double,int,QString)
            }
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
            value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.deleteValueAtTime(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2]))))
                    goto Sbk_AnimatedParamFunc_deleteValueAtTime_TypeError;
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);
        ::QString cppArg2 = QLatin1String("All");
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // deleteValueAtTime(double,int,QString)
            cppSelf->deleteValueAtTime(cppArg0, cppArg1, cppArg2);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_AnimatedParamFunc_deleteValueAtTime_TypeError:
        const char* overloads[] = {"float, int = 0, unicode = QLatin1String(\"All\")", 0};
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
            double cppResult = const_cast<const ::AnimatedParamWrapper*>(cppSelf)->getCurrentTime();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
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
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 3) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getDerivativeAtTime(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getDerivativeAtTime(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOO:getDerivativeAtTime", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: getDerivativeAtTime(double,int,QString)const
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // getDerivativeAtTime(double,int,QString)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            if (numArgs == 2) {
                overloadId = 0; // getDerivativeAtTime(double,int,QString)const
            } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2])))) {
                overloadId = 0; // getDerivativeAtTime(double,int,QString)const
            }
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
            value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getDerivativeAtTime(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2]))))
                    goto Sbk_AnimatedParamFunc_getDerivativeAtTime_TypeError;
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);
        ::QString cppArg2 = QLatin1String("Main");
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // getDerivativeAtTime(double,int,QString)const
            double cppResult = const_cast<const ::AnimatedParamWrapper*>(cppSelf)->getDerivativeAtTime(cppArg0, cppArg1, cppArg2);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_AnimatedParamFunc_getDerivativeAtTime_TypeError:
        const char* overloads[] = {"float, int = 0, unicode = QLatin1String(\"Main\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.AnimatedParam.getDerivativeAtTime", overloads);
        return 0;
}

static PyObject* Sbk_AnimatedParamFunc_getExpression(PyObject* self, PyObject* args, PyObject* kwds)
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
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getExpression(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getExpression(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:getExpression", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: getExpression(int,bool*,QString)const
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // getExpression(int,bool*,QString)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
            overloadId = 0; // getExpression(int,bool*,QString)const
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AnimatedParamFunc_getExpression_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getExpression(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                    goto Sbk_AnimatedParamFunc_getExpression_TypeError;
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // getExpression(int,bool*,QString)const
            // Begin code injection

            bool hasRetVar;
            QString cppResult = cppSelf->getExpression(cppArg0,&hasRetVar);
            pyResult = PyTuple_New(2);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult));
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
        const char* overloads[] = {"int, PySide.QtCore.bool, unicode = QLatin1String(\"Main\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.AnimatedParam.getExpression", overloads);
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
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getIntegrateFromTimeToTime(): too many arguments");
        return 0;
    } else if (numArgs < 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getIntegrateFromTimeToTime(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOOO:getIntegrateFromTimeToTime", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: getIntegrateFromTimeToTime(double,double,int,QString)const
    if (numArgs >= 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        if (numArgs == 2) {
            overloadId = 0; // getIntegrateFromTimeToTime(double,double,int,QString)const
        } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
            if (numArgs == 3) {
                overloadId = 0; // getIntegrateFromTimeToTime(double,double,int,QString)const
            } else if ((pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[3])))) {
                overloadId = 0; // getIntegrateFromTimeToTime(double,double,int,QString)const
            }
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
            value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[3]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getIntegrateFromTimeToTime(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[3] = value;
                if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[3]))))
                    goto Sbk_AnimatedParamFunc_getIntegrateFromTimeToTime_TypeError;
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        int cppArg2 = 0;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);
        ::QString cppArg3 = QLatin1String("Main");
        if (pythonToCpp[3]) pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // getIntegrateFromTimeToTime(double,double,int,QString)const
            double cppResult = const_cast<const ::AnimatedParamWrapper*>(cppSelf)->getIntegrateFromTimeToTime(cppArg0, cppArg1, cppArg2, cppArg3);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_AnimatedParamFunc_getIntegrateFromTimeToTime_TypeError:
        const char* overloads[] = {"float, float, int = 0, unicode = QLatin1String(\"Main\")", 0};
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
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getIsAnimated(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:getIsAnimated", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: getIsAnimated(int,QString)const
    if (numArgs == 0) {
        overloadId = 0; // getIsAnimated(int,QString)const
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // getIsAnimated(int,QString)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
            overloadId = 0; // getIsAnimated(int,QString)const
        }
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
            value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getIsAnimated(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                    goto Sbk_AnimatedParamFunc_getIsAnimated_TypeError;
            }
        }
        int cppArg0 = 0;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1 = QLatin1String("Main");
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getIsAnimated(int,QString)const
            bool cppResult = const_cast<const ::AnimatedParamWrapper*>(cppSelf)->getIsAnimated(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_AnimatedParamFunc_getIsAnimated_TypeError:
        const char* overloads[] = {"int = 0, unicode = QLatin1String(\"Main\")", 0};
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
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 3) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getKeyIndex(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getKeyIndex(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOO:getKeyIndex", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: getKeyIndex(double,int,QString)const
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // getKeyIndex(double,int,QString)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            if (numArgs == 2) {
                overloadId = 0; // getKeyIndex(double,int,QString)const
            } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2])))) {
                overloadId = 0; // getKeyIndex(double,int,QString)const
            }
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
            value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getKeyIndex(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2]))))
                    goto Sbk_AnimatedParamFunc_getKeyIndex_TypeError;
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);
        ::QString cppArg2 = QLatin1String("Main");
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // getKeyIndex(double,int,QString)const
            int cppResult = const_cast<const ::AnimatedParamWrapper*>(cppSelf)->getKeyIndex(cppArg0, cppArg1, cppArg2);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_AnimatedParamFunc_getKeyIndex_TypeError:
        const char* overloads[] = {"float, int = 0, unicode = QLatin1String(\"Main\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.AnimatedParam.getKeyIndex", overloads);
        return 0;
}

static PyObject* Sbk_AnimatedParamFunc_getKeyTime(PyObject* self, PyObject* args, PyObject* kwds)
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
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getKeyTime(): too many arguments");
        return 0;
    } else if (numArgs < 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getKeyTime(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOO:getKeyTime", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: getKeyTime(int,int,double*,QString)const
    if (numArgs >= 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
        if (numArgs == 2) {
            overloadId = 0; // getKeyTime(int,int,double*,QString)const
        } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2])))) {
            overloadId = 0; // getKeyTime(int,int,double*,QString)const
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AnimatedParamFunc_getKeyTime_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getKeyTime(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2]))))
                    goto Sbk_AnimatedParamFunc_getKeyTime_TypeError;
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getKeyTime(int,int,double*,QString)const
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
        const char* overloads[] = {"int, int, PySide.QtCore.double, unicode = QLatin1String(\"Main\")", 0};
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
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getNumKeys(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:getNumKeys", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: getNumKeys(int,QString)const
    if (numArgs == 0) {
        overloadId = 0; // getNumKeys(int,QString)const
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // getNumKeys(int,QString)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
            overloadId = 0; // getNumKeys(int,QString)const
        }
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
            value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.getNumKeys(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                    goto Sbk_AnimatedParamFunc_getNumKeys_TypeError;
            }
        }
        int cppArg0 = 0;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1 = QLatin1String("Main");
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getNumKeys(int,QString)const
            int cppResult = const_cast<const ::AnimatedParamWrapper*>(cppSelf)->getNumKeys(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_AnimatedParamFunc_getNumKeys_TypeError:
        const char* overloads[] = {"int = 0, unicode = QLatin1String(\"Main\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.AnimatedParam.getNumKeys", overloads);
        return 0;
}

static PyObject* Sbk_AnimatedParamFunc_getViewsList(PyObject* self)
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
            // getViewsList()const
            QStringList cppResult = const_cast<const ::AnimatedParamWrapper*>(cppSelf)->getViewsList();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRINGLIST_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_AnimatedParamFunc_removeAnimation(PyObject* self, PyObject* args, PyObject* kwds)
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
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.removeAnimation(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:removeAnimation", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: removeAnimation(int,QString)
    if (numArgs == 0) {
        overloadId = 0; // removeAnimation(int,QString)
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // removeAnimation(int,QString)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
            overloadId = 0; // removeAnimation(int,QString)
        }
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
            value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.removeAnimation(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                    goto Sbk_AnimatedParamFunc_removeAnimation_TypeError;
            }
        }
        int cppArg0 = -1;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1 = QLatin1String("All");
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // removeAnimation(int,QString)
            cppSelf->removeAnimation(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_AnimatedParamFunc_removeAnimation_TypeError:
        const char* overloads[] = {"int = -1, unicode = QLatin1String(\"All\")", 0};
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
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.setExpression(): too many arguments");
        return 0;
    } else if (numArgs < 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.setExpression(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOOO:setExpression", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: setExpression(QString,bool,int,QString)
    if (numArgs >= 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[1])))) {
        if (numArgs == 2) {
            overloadId = 0; // setExpression(QString,bool,int,QString)
        } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
            if (numArgs == 3) {
                overloadId = 0; // setExpression(QString,bool,int,QString)
            } else if ((pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[3])))) {
                overloadId = 0; // setExpression(QString,bool,int,QString)
            }
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
            value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[3]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.setExpression(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[3] = value;
                if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[3]))))
                    goto Sbk_AnimatedParamFunc_setExpression_TypeError;
            }
        }
        ::QString cppArg0 = ::QString();
        pythonToCpp[0](pyArgs[0], &cppArg0);
        bool cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        int cppArg2 = -1;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // setExpression(QString,bool,int,QString)
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
        const char* overloads[] = {"unicode, bool, int = -1, unicode = QLatin1String(\"All\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.AnimatedParam.setExpression", overloads);
        return 0;
}

static PyObject* Sbk_AnimatedParamFunc_setInterpolationAtTime(PyObject* self, PyObject* args, PyObject* kwds)
{
    AnimatedParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (AnimatedParamWrapper*)((::AnimatedParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.setInterpolationAtTime(): too many arguments");
        return 0;
    } else if (numArgs < 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.setInterpolationAtTime(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOOO:setInterpolationAtTime", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return 0;


    // Overloaded function decisor
    // 0: setInterpolationAtTime(double,NATRON_NAMESPACE::KeyframeTypeEnum,int,QString)
    if (numArgs >= 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SBK_CONVERTER(SbkNatronEngineTypes[SBK_NATRON_NAMESPACE_KEYFRAMETYPEENUM_IDX]), (pyArgs[1])))) {
        if (numArgs == 2) {
            overloadId = 0; // setInterpolationAtTime(double,NATRON_NAMESPACE::KeyframeTypeEnum,int,QString)
        } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
            if (numArgs == 3) {
                overloadId = 0; // setInterpolationAtTime(double,NATRON_NAMESPACE::KeyframeTypeEnum,int,QString)
            } else if ((pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[3])))) {
                overloadId = 0; // setInterpolationAtTime(double,NATRON_NAMESPACE::KeyframeTypeEnum,int,QString)
            }
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AnimatedParamFunc_setInterpolationAtTime_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "dimension");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.setInterpolationAtTime(): got multiple values for keyword argument 'dimension'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2]))))
                    goto Sbk_AnimatedParamFunc_setInterpolationAtTime_TypeError;
            }
            value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[3]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.AnimatedParam.setInterpolationAtTime(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[3] = value;
                if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[3]))))
                    goto Sbk_AnimatedParamFunc_setInterpolationAtTime_TypeError;
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::NATRON_NAMESPACE::KeyframeTypeEnum cppArg1 = ((::NATRON_NAMESPACE::KeyframeTypeEnum)0);
        pythonToCpp[1](pyArgs[1], &cppArg1);
        int cppArg2 = -1;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);
        ::QString cppArg3 = QLatin1String("All");
        if (pythonToCpp[3]) pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // setInterpolationAtTime(double,NATRON_NAMESPACE::KeyframeTypeEnum,int,QString)
            bool cppResult = cppSelf->setInterpolationAtTime(cppArg0, cppArg1, cppArg2, cppArg3);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_AnimatedParamFunc_setInterpolationAtTime_TypeError:
        const char* overloads[] = {"float, NatronEngine.NATRON_NAMESPACE.KeyframeTypeEnum, int = -1, unicode = QLatin1String(\"All\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.AnimatedParam.setInterpolationAtTime", overloads);
        return 0;
}

static PyObject* Sbk_AnimatedParamFunc_splitView(PyObject* self, PyObject* pyArg)
{
    AnimatedParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (AnimatedParamWrapper*)((::AnimatedParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: splitView(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // splitView(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AnimatedParamFunc_splitView_TypeError;

    // Call function/method
    {
        ::QString cppArg0 = ::QString();
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // splitView(QString)
            cppSelf->splitView(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_AnimatedParamFunc_splitView_TypeError:
        const char* overloads[] = {"unicode", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.AnimatedParam.splitView", overloads);
        return 0;
}

static PyObject* Sbk_AnimatedParamFunc_unSplitView(PyObject* self, PyObject* pyArg)
{
    AnimatedParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (AnimatedParamWrapper*)((::AnimatedParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: unSplitView(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // unSplitView(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AnimatedParamFunc_unSplitView_TypeError;

    // Call function/method
    {
        ::QString cppArg0 = ::QString();
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // unSplitView(QString)
            cppSelf->unSplitView(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_AnimatedParamFunc_unSplitView_TypeError:
        const char* overloads[] = {"unicode", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.AnimatedParam.unSplitView", overloads);
        return 0;
}

static PyMethodDef Sbk_AnimatedParam_methods[] = {
    {"deleteValueAtTime", (PyCFunction)Sbk_AnimatedParamFunc_deleteValueAtTime, METH_VARARGS|METH_KEYWORDS},
    {"getCurrentTime", (PyCFunction)Sbk_AnimatedParamFunc_getCurrentTime, METH_NOARGS},
    {"getDerivativeAtTime", (PyCFunction)Sbk_AnimatedParamFunc_getDerivativeAtTime, METH_VARARGS|METH_KEYWORDS},
    {"getExpression", (PyCFunction)Sbk_AnimatedParamFunc_getExpression, METH_VARARGS|METH_KEYWORDS},
    {"getIntegrateFromTimeToTime", (PyCFunction)Sbk_AnimatedParamFunc_getIntegrateFromTimeToTime, METH_VARARGS|METH_KEYWORDS},
    {"getIsAnimated", (PyCFunction)Sbk_AnimatedParamFunc_getIsAnimated, METH_VARARGS|METH_KEYWORDS},
    {"getKeyIndex", (PyCFunction)Sbk_AnimatedParamFunc_getKeyIndex, METH_VARARGS|METH_KEYWORDS},
    {"getKeyTime", (PyCFunction)Sbk_AnimatedParamFunc_getKeyTime, METH_VARARGS|METH_KEYWORDS},
    {"getNumKeys", (PyCFunction)Sbk_AnimatedParamFunc_getNumKeys, METH_VARARGS|METH_KEYWORDS},
    {"getViewsList", (PyCFunction)Sbk_AnimatedParamFunc_getViewsList, METH_NOARGS},
    {"removeAnimation", (PyCFunction)Sbk_AnimatedParamFunc_removeAnimation, METH_VARARGS|METH_KEYWORDS},
    {"setExpression", (PyCFunction)Sbk_AnimatedParamFunc_setExpression, METH_VARARGS|METH_KEYWORDS},
    {"setInterpolationAtTime", (PyCFunction)Sbk_AnimatedParamFunc_setInterpolationAtTime, METH_VARARGS|METH_KEYWORDS},
    {"splitView", (PyCFunction)Sbk_AnimatedParamFunc_splitView, METH_O},
    {"unSplitView", (PyCFunction)Sbk_AnimatedParamFunc_unSplitView, METH_O},

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
