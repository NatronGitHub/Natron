
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

#include "choiceparam_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING
#include <PyAppInstance.h>
#include <PyItemsTable.h>
#include <PyNode.h>
#include <PyParameter.h>
#include <list>
#include <utility>


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
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_addAsDependencyOf_TypeError;

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
            int cppResult = cppSelf->addAsDependencyOf(cppArg0, cppArg1, cppArg2, cppArg3, cppArg4);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ChoiceParamFunc_addAsDependencyOf_TypeError:
        const char* overloads[] = {"NatronEngine.Param, int, int, unicode, unicode", 0};
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
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "addOption", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: addOption(QString,QString,QString)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2])))) {
        overloadId = 0; // addOption(QString,QString,QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_addOption_TypeError;

    // Call function/method
    {
        ::QString cppArg0 = ::QString();
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1 = ::QString();
        pythonToCpp[1](pyArgs[1], &cppArg1);
        ::QString cppArg2 = ::QString();
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // addOption(QString,QString,QString)
            cppSelf->addOption(cppArg0, cppArg1, cppArg2);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ChoiceParamFunc_addOption_TypeError:
        const char* overloads[] = {"unicode, unicode, unicode", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ChoiceParam.addOption", overloads);
        return 0;
}

static PyObject* Sbk_ChoiceParamFunc_get(PyObject* self, PyObject* args, PyObject* kwds)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.get(): too many arguments");
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
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_get_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // get(const QString & view) const
        {
            if (kwds) {
                PyObject* value = PyDict_GetItemString(kwds, "view");
                if (value && pyArgs[0]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.get(): got multiple values for keyword argument 'view'.");
                    return 0;
                } else if (value) {
                    pyArgs[0] = value;
                    if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0]))))
                        goto Sbk_ChoiceParamFunc_get_TypeError;
                }
            }
            ::QString cppArg0 = QLatin1String("Main");
            if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // get(QString)const
                int cppResult = const_cast<const ::ChoiceParamWrapper*>(cppSelf)->get(cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
            }
            break;
        }
        case 1: // get(double frame, const QString & view) const
        {
            if (kwds) {
                PyObject* value = PyDict_GetItemString(kwds, "view");
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.get(): got multiple values for keyword argument 'view'.");
                    return 0;
                } else if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                        goto Sbk_ChoiceParamFunc_get_TypeError;
                }
            }
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            ::QString cppArg1 = QLatin1String("Main");
            if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // get(double,QString)const
                int cppResult = const_cast<const ::ChoiceParamWrapper*>(cppSelf)->get(cppArg0, cppArg1);
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
        const char* overloads[] = {"unicode = QLatin1String(\"Main\")", "float, unicode = QLatin1String(\"Main\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ChoiceParam.get", overloads);
        return 0;
}

static PyObject* Sbk_ChoiceParamFunc_getActiveOption(PyObject* self, PyObject* args, PyObject* kwds)
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
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.getActiveOption(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|O:getActiveOption", &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: getActiveOption(QString*,QString*,QString*,QString)const
    if (numArgs == 0) {
        overloadId = 0; // getActiveOption(QString*,QString*,QString*,QString)const
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))) {
        overloadId = 0; // getActiveOption(QString*,QString*,QString*,QString)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_getActiveOption_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.getActiveOption(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0]))))
                    goto Sbk_ChoiceParamFunc_getActiveOption_TypeError;
            }
        }
        ::QString cppArg0 = QLatin1String("Main");
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // getActiveOption(QString*,QString*,QString*,QString)const
            // Begin code injection

            QString optionID, optionLabel, optionHint;
            cppSelf->getActiveOption(&optionID,&optionLabel, &optionHint, cppArg0);
            pyResult = PyTuple_New(3);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &optionID));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &optionLabel));
            PyTuple_SET_ITEM(pyResult, 2, Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &optionHint));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ChoiceParamFunc_getActiveOption_TypeError:
        const char* overloads[] = {"PySide.QtCore.QString, PySide.QtCore.QString, PySide.QtCore.QString, unicode = QLatin1String(\"Main\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ChoiceParam.getActiveOption", overloads);
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
    // 0: getOption(int,QString*,QString*,QString*)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // getOption(int,QString*,QString*,QString*)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_getOption_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getOption(int,QString*,QString*,QString*)const
            // Begin code injection

            QString optionID, optionLabel, optionHint;
            bool ok = cppSelf->getOption(cppArg0, &optionID,&optionLabel, &optionHint);
            pyResult = PyTuple_New(4);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &ok));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &optionID));
            PyTuple_SET_ITEM(pyResult, 2, Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &optionLabel));
            PyTuple_SET_ITEM(pyResult, 3, Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &optionHint));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ChoiceParamFunc_getOption_TypeError:
        const char* overloads[] = {"int, PySide.QtCore.QString, PySide.QtCore.QString, PySide.QtCore.QString", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ChoiceParam.getOption", overloads);
        return 0;
}

static PyObject* Sbk_ChoiceParamFunc_getOptions(PyObject* self, PyObject* args)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "getOptions", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: getOptions(std::list<QString>*,std::list<QString>*,std::list<QString>*)const
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_QSTRING_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_QSTRING_IDX], (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_QSTRING_IDX], (pyArgs[2])))) {
        overloadId = 0; // getOptions(std::list<QString>*,std::list<QString>*,std::list<QString>*)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_getOptions_TypeError;

    // Call function/method
    {
        ::std::list<QString >* cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::list<QString >* cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        ::std::list<QString >* cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // getOptions(std::list<QString>*,std::list<QString>*,std::list<QString>*)const
            const_cast<const ::ChoiceParamWrapper*>(cppSelf)->getOptions(cppArg0, cppArg1, cppArg2);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ChoiceParamFunc_getOptions_TypeError:
        const char* overloads[] = {"list, list, list", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ChoiceParam.getOptions", overloads);
        return 0;
}

static PyObject* Sbk_ChoiceParamFunc_getValue(PyObject* self, PyObject* args, PyObject* kwds)
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
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.getValue(): too many arguments");
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
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_getValue_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.getValue(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0]))))
                    goto Sbk_ChoiceParamFunc_getValue_TypeError;
            }
        }
        ::QString cppArg0 = QLatin1String("Main");
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // getValue(QString)const
            int cppResult = const_cast<const ::ChoiceParamWrapper*>(cppSelf)->getValue(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ChoiceParamFunc_getValue_TypeError:
        const char* overloads[] = {"unicode = QLatin1String(\"Main\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ChoiceParam.getValue", overloads);
        return 0;
}

static PyObject* Sbk_ChoiceParamFunc_getValueAtTime(PyObject* self, PyObject* args, PyObject* kwds)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.getValueAtTime(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.getValueAtTime(): not enough arguments");
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
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_getValueAtTime_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.getValueAtTime(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                    goto Sbk_ChoiceParamFunc_getValueAtTime_TypeError;
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1 = QLatin1String("Main");
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getValueAtTime(double,QString)const
            int cppResult = const_cast<const ::ChoiceParamWrapper*>(cppSelf)->getValueAtTime(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_ChoiceParamFunc_getValueAtTime_TypeError:
        const char* overloads[] = {"float, unicode = QLatin1String(\"Main\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ChoiceParam.getValueAtTime", overloads);
        return 0;
}

static PyObject* Sbk_ChoiceParamFunc_restoreDefaultValue(PyObject* self, PyObject* args, PyObject* kwds)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.restoreDefaultValue(): too many arguments");
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
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_restoreDefaultValue_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.restoreDefaultValue(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0]))))
                    goto Sbk_ChoiceParamFunc_restoreDefaultValue_TypeError;
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

    Sbk_ChoiceParamFunc_restoreDefaultValue_TypeError:
        const char* overloads[] = {"unicode = QLatin1String(\"All\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ChoiceParam.restoreDefaultValue", overloads);
        return 0;
}

static PyObject* Sbk_ChoiceParamFunc_set(PyObject* self, PyObject* args, PyObject* kwds)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 3) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.set(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.set(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOO:set", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: set(QString,QString)
    // 1: set(int,QString)
    // 2: set(int,double,QString)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 1; // set(int,QString)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
            if (numArgs == 2) {
                overloadId = 2; // set(int,double,QString)
            } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2])))) {
                overloadId = 2; // set(int,double,QString)
            }
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
            overloadId = 1; // set(int,QString)
        }
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // set(QString,QString)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
            overloadId = 0; // set(QString,QString)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_set_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // set(const QString & label, const QString & view)
        {
            if (kwds) {
                PyObject* value = PyDict_GetItemString(kwds, "view");
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.set(): got multiple values for keyword argument 'view'.");
                    return 0;
                } else if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                        goto Sbk_ChoiceParamFunc_set_TypeError;
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
        case 1: // set(int x, const QString & view)
        {
            if (kwds) {
                PyObject* value = PyDict_GetItemString(kwds, "view");
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.set(): got multiple values for keyword argument 'view'.");
                    return 0;
                } else if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                        goto Sbk_ChoiceParamFunc_set_TypeError;
                }
            }
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            ::QString cppArg1 = QLatin1String("All");
            if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // set(int,QString)
                cppSelf->set(cppArg0, cppArg1);
            }
            break;
        }
        case 2: // set(int x, double frame, const QString & view)
        {
            if (kwds) {
                PyObject* value = PyDict_GetItemString(kwds, "view");
                if (value && pyArgs[2]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.set(): got multiple values for keyword argument 'view'.");
                    return 0;
                } else if (value) {
                    pyArgs[2] = value;
                    if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2]))))
                        goto Sbk_ChoiceParamFunc_set_TypeError;
                }
            }
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            ::QString cppArg2 = QLatin1String("All");
            if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);

            if (!PyErr_Occurred()) {
                // set(int,double,QString)
                cppSelf->set(cppArg0, cppArg1, cppArg2);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ChoiceParamFunc_set_TypeError:
        const char* overloads[] = {"unicode, unicode = QLatin1String(\"All\")", "int, unicode = QLatin1String(\"All\")", "int, float, unicode = QLatin1String(\"All\")", 0};
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
    // 0: setDefaultValue(QString)
    // 1: setDefaultValue(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 1; // setDefaultValue(int)
    } else if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // setDefaultValue(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_setDefaultValue_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // setDefaultValue(const QString & value)
        {
            ::QString cppArg0 = ::QString();
            pythonToCpp(pyArg, &cppArg0);

            if (!PyErr_Occurred()) {
                // setDefaultValue(QString)
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
        const char* overloads[] = {"unicode", "int", 0};
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
    // 0: setOptions(std::list<QString>,std::list<QString>,std::list<QString>)
    // 1: setOptions(std::list<std::pair<QString,QString> >)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_STD_PAIR_QSTRING_QSTRING_IDX], (pyArg)))) {
        overloadId = 1; // setOptions(std::list<std::pair<QString,QString> >)
    } else if (PySequence_Check(pyArg)) {
        overloadId = 0; // setOptions(std::list<QString>,std::list<QString>,std::list<QString>)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_setOptions_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // setOptions(const std::list<QString > & optionIDs, const std::list<QString > & optionLabels, const std::list<QString > & optionHelps)
        {

            if (!PyErr_Occurred()) {
                // setOptions(std::list<QString>,std::list<QString>,std::list<QString>)
                // Begin code injection


                if (!PyList_Check(pyArg)) {
                    PyErr_SetString(PyExc_ValueError, "Argument must be a list of tuples");
                    return 0;
                }

                int listSize = PyList_Size(pyArg);
                if (listSize == 0) {
                    PyErr_SetString(PyExc_ValueError, "Argument must be a list of tuples");
                    return 0;
                }

                std::list<QString> optionIDs, optionLabels, optionHelps;
                for (int i = 0; i < listSize; ++i) {
                    PyObject* obj = PyList_GetItem(pyArg, i);
                    if (!PyTuple_Check(obj)) {
                        PyErr_SetString(PyExc_ValueError, "Argument must be a list of tuples");
                        return 0;
                    }
                    int tupleSize = PyTuple_Size(obj);
                    if (tupleSize != 3) {
                        PyErr_SetString(PyExc_ValueError, "Each option must be a tuple of 3 strings");
                        return 0;
                    }
                    PyObject* pyOptionId = PyTuple_GetItem(obj, 0);
                    PyObject* pyOptionLabel = PyTuple_GetItem(obj, 1);
                    PyObject* pyOptionHnt = PyTuple_GetItem(obj, 2);
                    ::QString optionId = ::QString();
                    Shiboken::Conversions::pythonToCppCopy(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], pyOptionId, &(optionId));
                    ::QString optionLabel = ::QString();
                    Shiboken::Conversions::pythonToCppCopy(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], pyOptionLabel, &(optionLabel));
                    ::QString optionHint = ::QString();
                    Shiboken::Conversions::pythonToCppCopy(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], pyOptionHnt, &(optionHint));
                    optionIDs.push_back(optionId);
                    optionLabels.push_back(optionLabel);
                    optionHelps.push_back(optionHint);
                }

                cppSelf->setOptions(optionIDs, optionLabels, optionHelps);


                // End of code injection


            }
            break;
        }
        case 1: // setOptions(const std::list<std::pair<QString, QString > > & options)
        {
            ::std::list<std::pair<QString, QString > > cppArg0;
            pythonToCpp(pyArg, &cppArg0);

            if (!PyErr_Occurred()) {
                // setOptions(std::list<std::pair<QString,QString> >)
                cppSelf->setOptions(cppArg0);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ChoiceParamFunc_setOptions_TypeError:
        const char* overloads[] = {"list, list, list", "list", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ChoiceParam.setOptions", overloads);
        return 0;
}

static PyObject* Sbk_ChoiceParamFunc_setValue(PyObject* self, PyObject* args, PyObject* kwds)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.setValue(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.setValue(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:setValue", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: setValue(QString,QString)
    // 1: setValue(int,QString)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 1; // setValue(int,QString)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
            overloadId = 1; // setValue(int,QString)
        }
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // setValue(QString,QString)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))) {
            overloadId = 0; // setValue(QString,QString)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_setValue_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // setValue(const QString & label, const QString & view)
        {
            if (kwds) {
                PyObject* value = PyDict_GetItemString(kwds, "view");
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.setValue(): got multiple values for keyword argument 'view'.");
                    return 0;
                } else if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                        goto Sbk_ChoiceParamFunc_setValue_TypeError;
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
            break;
        }
        case 1: // setValue(int value, const QString & view)
        {
            if (kwds) {
                PyObject* value = PyDict_GetItemString(kwds, "view");
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.setValue(): got multiple values for keyword argument 'view'.");
                    return 0;
                } else if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1]))))
                        goto Sbk_ChoiceParamFunc_setValue_TypeError;
                }
            }
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            ::QString cppArg1 = QLatin1String("All");
            if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // setValue(int,QString)
                cppSelf->setValue(cppArg0, cppArg1);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ChoiceParamFunc_setValue_TypeError:
        const char* overloads[] = {"unicode, unicode = QLatin1String(\"All\")", "int, unicode = QLatin1String(\"All\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ChoiceParam.setValue", overloads);
        return 0;
}

static PyObject* Sbk_ChoiceParamFunc_setValueAtTime(PyObject* self, PyObject* args, PyObject* kwds)
{
    ChoiceParamWrapper* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = (ChoiceParamWrapper*)((::ChoiceParam*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_CHOICEPARAM_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 3) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.setValueAtTime(): too many arguments");
        return 0;
    } else if (numArgs < 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.setValueAtTime(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OOO:setValueAtTime", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: setValueAtTime(int,double,QString)
    if (numArgs >= 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        if (numArgs == 2) {
            overloadId = 0; // setValueAtTime(int,double,QString)
        } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2])))) {
            overloadId = 0; // setValueAtTime(int,double,QString)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ChoiceParamFunc_setValueAtTime_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "view");
            if (value && pyArgs[2]) {
                PyErr_SetString(PyExc_TypeError, "NatronEngine.ChoiceParam.setValueAtTime(): got multiple values for keyword argument 'view'.");
                return 0;
            } else if (value) {
                pyArgs[2] = value;
                if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[2]))))
                    goto Sbk_ChoiceParamFunc_setValueAtTime_TypeError;
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        ::QString cppArg2 = QLatin1String("All");
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // setValueAtTime(int,double,QString)
            cppSelf->setValueAtTime(cppArg0, cppArg1, cppArg2);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_ChoiceParamFunc_setValueAtTime_TypeError:
        const char* overloads[] = {"int, float, unicode = QLatin1String(\"All\")", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ChoiceParam.setValueAtTime", overloads);
        return 0;
}

static PyMethodDef Sbk_ChoiceParam_methods[] = {
    {"addAsDependencyOf", (PyCFunction)Sbk_ChoiceParamFunc_addAsDependencyOf, METH_VARARGS},
    {"addOption", (PyCFunction)Sbk_ChoiceParamFunc_addOption, METH_VARARGS},
    {"get", (PyCFunction)Sbk_ChoiceParamFunc_get, METH_VARARGS|METH_KEYWORDS},
    {"getActiveOption", (PyCFunction)Sbk_ChoiceParamFunc_getActiveOption, METH_VARARGS|METH_KEYWORDS},
    {"getDefaultValue", (PyCFunction)Sbk_ChoiceParamFunc_getDefaultValue, METH_NOARGS},
    {"getNumOptions", (PyCFunction)Sbk_ChoiceParamFunc_getNumOptions, METH_NOARGS},
    {"getOption", (PyCFunction)Sbk_ChoiceParamFunc_getOption, METH_O},
    {"getOptions", (PyCFunction)Sbk_ChoiceParamFunc_getOptions, METH_VARARGS},
    {"getValue", (PyCFunction)Sbk_ChoiceParamFunc_getValue, METH_VARARGS|METH_KEYWORDS},
    {"getValueAtTime", (PyCFunction)Sbk_ChoiceParamFunc_getValueAtTime, METH_VARARGS|METH_KEYWORDS},
    {"restoreDefaultValue", (PyCFunction)Sbk_ChoiceParamFunc_restoreDefaultValue, METH_VARARGS|METH_KEYWORDS},
    {"set", (PyCFunction)Sbk_ChoiceParamFunc_set, METH_VARARGS|METH_KEYWORDS},
    {"setDefaultValue", (PyCFunction)Sbk_ChoiceParamFunc_setDefaultValue, METH_O},
    {"setOptions", (PyCFunction)Sbk_ChoiceParamFunc_setOptions, METH_O},
    {"setValue", (PyCFunction)Sbk_ChoiceParamFunc_setValue, METH_VARARGS|METH_KEYWORDS},
    {"setValueAtTime", (PyCFunction)Sbk_ChoiceParamFunc_setValueAtTime, METH_VARARGS|METH_KEYWORDS},

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
