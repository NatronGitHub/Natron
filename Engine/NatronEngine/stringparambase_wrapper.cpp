
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

#include "stringparambase_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING
#include <PyAppInstance.h>
#include <PyItemsTable.h>
#include <PyNode.h>
#include <PyParameter.h>


// Native ---------------------------------------------------------

void StringParamBaseWrapper::pysideInitQtMetaTypes()
{
}

StringParamBaseWrapper::~StringParamBaseWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_StringParamBaseFunc_addAsDependencyOf(PyObject* self, PyObject* args)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "addAsDependencyOf", 5, 5, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3]), &(pyArgs[4])))
        return 0;


    // Overloaded function decisor
    // 0: addAsDependencyOf(Param*,int,int,QString,QString)
    if (numArgs == 5
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_PARAM_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[3])))
        && (pythonToCpp[4] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[4])))) {
        overloadId = 0; // addAsDependencyOf(Param*,int,int,QString,QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StringParamBaseFunc_addAsDependencyOf_TypeError;

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
        ::QString cppArg3 = ::QString();
        pythonToCpp[3](pyArgs[3], &cppArg3);
        ::QString cppArg4 = ::QString();
        pythonToCpp[4](pyArgs[4], &cppArg4);

        if (!PyErr_Occurred()) {
            // addAsDependencyOf(Param*,int,int,QString,QString)
            QString cppResult = cppSelf->addAsDependencyOf(cppArg0, cppArg1, cppArg2, cppArg3, cppArg4);
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_StringParamBaseFunc_addAsDependencyOf_TypeError:
        const char* overloads[] = {"NatronEngine.Param, int, int, unicode, unicode", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.StringParamBase.addAsDependencyOf", overloads);
        return 0;
}

static PyObject* Sbk_StringParamBaseFunc_get(PyObject* self, PyObject* args, PyObject* kwds)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.get(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:get", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: get(QString)const
    // 1: get(double,QString)const
    if (numArgs == 0) {
        overloadId = 0; // get(QString)const
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 1; // get(double,QString)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
            overloadId = 1; // get(double,QString)const
        }
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))) {
        overloadId = 0; // get(QString)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StringParamBaseFunc_get_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // get(const QString & view) const
        {
            if (kwds) {
                PyObject* value = PyDict_GetItemString(kwds, "view");
                if (value && pyArgs[0]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.get(): got multiple values for keyword argument 'view'.");
                    return 0;
                } else if (value) {
                    pyArgs[0] = value;
                    if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0]))))
                        goto Sbk_StringParamBaseFunc_get_TypeError;
                }
            }
            ::QString cppArg0 = QLatin1String("Main");
            if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // get(QString)const
                QString cppResult = const_cast<const ::StringParamBaseWrapper*>(cppSelf)->get(cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
            }
            break;
        }
        case 1: // get(double frame, const QString & view) const
        {
            if (kwds) {
                PyObject* value = PyDict_GetItemString(kwds, "view");
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.get(): got multiple values for keyword argument 'view'.");
                    return 0;
                } else if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                        goto Sbk_StringParamBaseFunc_get_TypeError;
                }
            }
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            ::QString cppArg1 = QLatin1String("Main");
            if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // get(double,QString)const
                QString cppResult = const_cast<const ::StringParamBaseWrapper*>(cppSelf)->get(cppArg0, cppArg1);
                pyResult = Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_StringParamBaseFunc_get_TypeError:
        const char* overloads[] = {"unicode = QLatin1String(\"Main\")", "float, unicode = QLatin1String(\"Main\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.StringParamBase.get", overloads);
        return 0;
}

static PyObject* Sbk_StringParamBaseFunc_getDefaultValue(PyObject* self)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getDefaultValue()const
            QString cppResult = const_cast<const ::StringParamBaseWrapper*>(cppSelf)->getDefaultValue();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_StringParamBaseFunc_getValue(PyObject* self, PyObject* args, PyObject* kwds)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.getValue(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|O:getValue", &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: getValue(QString)const
    if (numArgs == 0) {
        overloadId = 0; // getValue(QString)const
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))) {
        overloadId = 0; // getValue(QString)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StringParamBaseFunc_getValue_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.getValue(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0]))))
                    goto Sbk_StringParamBaseFunc_getValue_TypeError;
            }
        }
        ::QString cppArg0 = QLatin1String("Main");
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // getValue(QString)const
            QString cppResult = const_cast<const ::StringParamBaseWrapper*>(cppSelf)->getValue(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_StringParamBaseFunc_getValue_TypeError:
        const char* overloads[] = {"unicode = QLatin1String(\"Main\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.StringParamBase.getValue", overloads);
        return 0;
}

static PyObject* Sbk_StringParamBaseFunc_getValueAtTime(PyObject* self, PyObject* args, PyObject* kwds)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.getValueAtTime(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.getValueAtTime(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:getValueAtTime", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: getValueAtTime(double,QString)const
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // getValueAtTime(double,QString)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
            overloadId = 0; // getValueAtTime(double,QString)const
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StringParamBaseFunc_getValueAtTime_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.getValueAtTime(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                    goto Sbk_StringParamBaseFunc_getValueAtTime_TypeError;
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1 = QLatin1String("Main");
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getValueAtTime(double,QString)const
            QString cppResult = const_cast<const ::StringParamBaseWrapper*>(cppSelf)->getValueAtTime(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_StringParamBaseFunc_getValueAtTime_TypeError:
        const char* overloads[] = {"float, unicode = QLatin1String(\"Main\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.StringParamBase.getValueAtTime", overloads);
        return 0;
}

static PyObject* Sbk_StringParamBaseFunc_restoreDefaultValue(PyObject* self, PyObject* args, PyObject* kwds)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.restoreDefaultValue(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|O:restoreDefaultValue", &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: restoreDefaultValue(QString)
    if (numArgs == 0) {
        overloadId = 0; // restoreDefaultValue(QString)
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))) {
        overloadId = 0; // restoreDefaultValue(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StringParamBaseFunc_restoreDefaultValue_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.restoreDefaultValue(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0]))))
                    goto Sbk_StringParamBaseFunc_restoreDefaultValue_TypeError;
            }
        }
        ::QString cppArg0 = QLatin1String("All");
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // restoreDefaultValue(QString)
            cppSelf->restoreDefaultValue(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_StringParamBaseFunc_restoreDefaultValue_TypeError:
        const char* overloads[] = {"unicode = QLatin1String(\"All\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.StringParamBase.restoreDefaultValue", overloads);
        return 0;
}

static PyObject* Sbk_StringParamBaseFunc_set(PyObject* self, PyObject* args, PyObject* kwds)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 3) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.set(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.set(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOO:set", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: set(QString,QString)
    // 1: set(QString,double,QString)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // set(QString,QString)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
            if (numArgs == 2) {
                overloadId = 1; // set(QString,double,QString)
            } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2])))) {
                overloadId = 1; // set(QString,double,QString)
            }
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
            overloadId = 0; // set(QString,QString)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StringParamBaseFunc_set_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // set(const QString & x, const QString & view)
        {
            if (kwds) {
                PyObject* value = PyDict_GetItemString(kwds, "view");
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.set(): got multiple values for keyword argument 'view'.");
                    return 0;
                } else if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                        goto Sbk_StringParamBaseFunc_set_TypeError;
                }
            }
            ::QString cppArg0 = ::QString();
            pythonToCpp[0](pyArgs[0], &cppArg0);
            ::QString cppArg1 = QLatin1String("All");
            if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // set(QString,QString)
                cppSelf->set(cppArg0, cppArg1);
            }
            break;
        }
        case 1: // set(const QString & x, double frame, const QString & view)
        {
            if (kwds) {
                PyObject* value = PyDict_GetItemString(kwds, "view");
                if (value && pyArgs[2]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.set(): got multiple values for keyword argument 'view'.");
                    return 0;
                } else if (value) {
                    pyArgs[2] = value;
                    if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2]))))
                        goto Sbk_StringParamBaseFunc_set_TypeError;
                }
            }
            ::QString cppArg0 = ::QString();
            pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            ::QString cppArg2 = QLatin1String("All");
            if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);

            if (!PyErr_Occurred()) {
                // set(QString,double,QString)
                cppSelf->set(cppArg0, cppArg1, cppArg2);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_StringParamBaseFunc_set_TypeError:
        const char* overloads[] = {"unicode, unicode = QLatin1String(\"All\")", "unicode, float, unicode = QLatin1String(\"All\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.StringParamBase.set", overloads);
        return 0;
}

static PyObject* Sbk_StringParamBaseFunc_setDefaultValue(PyObject* self, PyObject* pyArg)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setDefaultValue(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // setDefaultValue(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StringParamBaseFunc_setDefaultValue_TypeError;

    // Call function/method
    {
        ::QString cppArg0 = ::QString();
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setDefaultValue(QString)
            cppSelf->setDefaultValue(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_StringParamBaseFunc_setDefaultValue_TypeError:
        const char* overloads[] = {"unicode", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.StringParamBase.setDefaultValue", overloads);
        return 0;
}

static PyObject* Sbk_StringParamBaseFunc_setValue(PyObject* self, PyObject* args, PyObject* kwds)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.setValue(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.setValue(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:setValue", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: setValue(QString,QString)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // setValue(QString,QString)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
            overloadId = 0; // setValue(QString,QString)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StringParamBaseFunc_setValue_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.setValue(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                    goto Sbk_StringParamBaseFunc_setValue_TypeError;
            }
        }
        ::QString cppArg0 = ::QString();
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1 = QLatin1String("All");
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setValue(QString,QString)
            cppSelf->setValue(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_StringParamBaseFunc_setValue_TypeError:
        const char* overloads[] = {"unicode, unicode = QLatin1String(\"All\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.StringParamBase.setValue", overloads);
        return 0;
}

static PyObject* Sbk_StringParamBaseFunc_setValueAtTime(PyObject* self, PyObject* args, PyObject* kwds)
{
    StringParamBaseWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (StringParamBaseWrapper*)((::StringParamBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 3) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.setValueAtTime(): too many arguments");
        return 0;
    } else if (numArgs < 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.setValueAtTime(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOO:setValueAtTime", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: setValueAtTime(QString,double,QString)
    if (numArgs >= 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        if (numArgs == 2) {
            overloadId = 0; // setValueAtTime(QString,double,QString)
        } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2])))) {
            overloadId = 0; // setValueAtTime(QString,double,QString)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_StringParamBaseFunc_setValueAtTime_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.StringParamBase.setValueAtTime(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2]))))
                    goto Sbk_StringParamBaseFunc_setValueAtTime_TypeError;
            }
        }
        ::QString cppArg0 = ::QString();
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        ::QString cppArg2 = QLatin1String("All");
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // setValueAtTime(QString,double,QString)
            cppSelf->setValueAtTime(cppArg0, cppArg1, cppArg2);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_StringParamBaseFunc_setValueAtTime_TypeError:
        const char* overloads[] = {"unicode, float, unicode = QLatin1String(\"All\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.StringParamBase.setValueAtTime", overloads);
        return 0;
}

static PyMethodDef Sbk_StringParamBase_methods[] = {
    {"addAsDependencyOf", (PyCFunction)Sbk_StringParamBaseFunc_addAsDependencyOf, METH_VARARGS},
    {"get", (PyCFunction)Sbk_StringParamBaseFunc_get, METH_VARARGS|METH_KEYWORDS},
    {"getDefaultValue", (PyCFunction)Sbk_StringParamBaseFunc_getDefaultValue, METH_NOARGS},
    {"getValue", (PyCFunction)Sbk_StringParamBaseFunc_getValue, METH_VARARGS|METH_KEYWORDS},
    {"getValueAtTime", (PyCFunction)Sbk_StringParamBaseFunc_getValueAtTime, METH_VARARGS|METH_KEYWORDS},
    {"restoreDefaultValue", (PyCFunction)Sbk_StringParamBaseFunc_restoreDefaultValue, METH_VARARGS|METH_KEYWORDS},
    {"set", (PyCFunction)Sbk_StringParamBaseFunc_set, METH_VARARGS|METH_KEYWORDS},
    {"setDefaultValue", (PyCFunction)Sbk_StringParamBaseFunc_setDefaultValue, METH_O},
    {"setValue", (PyCFunction)Sbk_StringParamBaseFunc_setValue, METH_VARARGS|METH_KEYWORDS},
    {"setValueAtTime", (PyCFunction)Sbk_StringParamBaseFunc_setValueAtTime, METH_VARARGS|METH_KEYWORDS},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_StringParamBase_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_StringParamBase_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_StringParamBase_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.StringParamBase",
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
    /*tp_traverse*/         Sbk_StringParamBase_traverse,
    /*tp_clear*/            Sbk_StringParamBase_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_StringParamBase_methods,
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

static void* Sbk_StringParamBase_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::StringParamBase*>(reinterpret_cast< ::Param*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void StringParamBase_PythonToCpp_StringParamBase_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_StringParamBase_Type, pyIn, cppOut);
}
static PythonToCppFunc is_StringParamBase_PythonToCpp_StringParamBase_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_StringParamBase_Type))
        return StringParamBase_PythonToCpp_StringParamBase_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* StringParamBase_PTR_CppToPython_StringParamBase(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::StringParamBase*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_StringParamBase_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_StringParamBase(PyObject* module)
{
    SbkNatronEngineTypes[SBK_STRINGPARAMBASE_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_StringParamBase_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "StringParamBase", "StringParamBase*",
        &Sbk_StringParamBase_Type, &Shiboken::callCppDestructor< ::StringParamBase >, (SbkObjectType*)SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX])) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_StringParamBase_Type,
        StringParamBase_PythonToCpp_StringParamBase_PTR,
        is_StringParamBase_PythonToCpp_StringParamBase_PTR_Convertible,
        StringParamBase_PTR_CppToPython_StringParamBase);

    Shiboken::Conversions::registerConverterName(converter, "StringParamBase");
    Shiboken::Conversions::registerConverterName(converter, "StringParamBase*");
    Shiboken::Conversions::registerConverterName(converter, "StringParamBase&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::StringParamBase).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::StringParamBaseWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_StringParamBase_Type, &Sbk_StringParamBase_typeDiscovery);


    StringParamBaseWrapper::pysideInitQtMetaTypes();
}
