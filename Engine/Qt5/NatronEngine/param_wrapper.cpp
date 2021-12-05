
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
#include "param_wrapper.h"

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

void ParamWrapper::pysideInitQtMetaTypes()
{
}

void ParamWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

ParamWrapper::~ParamWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject *Sbk_ParamFunc__addAsDependencyOf(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "_addAsDependencyOf", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return {};


    // Overloaded function decisor
    // 0: Param::_addAsDependencyOf(int,Param*,int)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PARAM_IDX]), (pyArgs[1])))
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
            return {};
        ::Param *cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        int cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // _addAsDependencyOf(int,Param*,int)
            static_cast<::ParamWrapper *>(cppSelf)->ParamWrapper::_addAsDependencyOf_protected(cppArg0, cppArg1, cppArg2);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc__addAsDependencyOf_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Param._addAsDependencyOf");
        return {};
}

static PyObject *Sbk_ParamFunc_copy(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.copy(): too many arguments");
        return {};
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.copy(): not enough arguments");
        return {};
    }

    if (!PyArg_ParseTuple(args, "|OO:copy", &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: Param::copy(Param*,int)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PARAM_IDX]), (pyArgs[0])))) {
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
            PyObject *keyName = nullptr;
            PyObject *value = nullptr;
            keyName = Py_BuildValue("s","dimension");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.copy(): got multiple values for keyword argument 'dimension'.");
                    return {};
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_ParamFunc_copy_TypeError;
                }
            }
        }
        if (!Shiboken::Object::isValid(pyArgs[0]))
            return {};
        ::Param *cppArg0;
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
        return {};
    }
    return pyResult;

    Sbk_ParamFunc_copy_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Param.copy");
        return {};
}

static PyObject *Sbk_ParamFunc_curve(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.curve(): too many arguments");
        return {};
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.curve(): not enough arguments");
        return {};
    }

    if (!PyArg_ParseTuple(args, "|OO:curve", &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: Param::curve(double,int)const
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
            PyObject *keyName = nullptr;
            PyObject *value = nullptr;
            keyName = Py_BuildValue("s","dimension");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.curve(): got multiple values for keyword argument 'dimension'.");
                    return {};
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_ParamFunc_curve_TypeError;
                }
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // curve(double,int)const
            double cppResult = const_cast<const ::ParamWrapper *>(cppSelf)->curve(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_ParamFunc_curve_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Param.curve");
        return {};
}

static PyObject *Sbk_ParamFunc_getAddNewLine(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

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
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ParamFunc_getCanAnimate(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getCanAnimate()const
            bool cppResult = const_cast<const ::ParamWrapper *>(cppSelf)->getCanAnimate();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ParamFunc_getEvaluateOnChange(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getEvaluateOnChange()const
            bool cppResult = const_cast<const ::ParamWrapper *>(cppSelf)->getEvaluateOnChange();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ParamFunc_getHelp(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getHelp()const
            QString cppResult = const_cast<const ::ParamWrapper *>(cppSelf)->getHelp();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ParamFunc_getIsAnimationEnabled(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getIsAnimationEnabled()const
            bool cppResult = const_cast<const ::ParamWrapper *>(cppSelf)->getIsAnimationEnabled();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ParamFunc_getIsEnabled(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.getIsEnabled(): too many arguments");
        return {};
    }

    if (!PyArg_ParseTuple(args, "|O:getIsEnabled", &(pyArgs[0])))
        return {};


    // Overloaded function decisor
    // 0: Param::getIsEnabled(int)const
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
            PyObject *keyName = nullptr;
            PyObject *value = nullptr;
            keyName = Py_BuildValue("s","dimension");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[0]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.getIsEnabled(): got multiple values for keyword argument 'dimension'.");
                    return {};
                }
                if (value) {
                    pyArgs[0] = value;
                    if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0]))))
                        goto Sbk_ParamFunc_getIsEnabled_TypeError;
                }
            }
        }
        int cppArg0 = 0;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // getIsEnabled(int)const
            bool cppResult = const_cast<const ::ParamWrapper *>(cppSelf)->getIsEnabled(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_ParamFunc_getIsEnabled_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Param.getIsEnabled");
        return {};
}

static PyObject *Sbk_ParamFunc_getIsPersistent(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getIsPersistent()const
            bool cppResult = const_cast<const ::ParamWrapper *>(cppSelf)->getIsPersistent();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ParamFunc_getIsVisible(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getIsVisible()const
            bool cppResult = const_cast<const ::ParamWrapper *>(cppSelf)->getIsVisible();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ParamFunc_getLabel(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getLabel()const
            QString cppResult = const_cast<const ::ParamWrapper *>(cppSelf)->getLabel();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ParamFunc_getNumDimensions(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNumDimensions()const
            int cppResult = const_cast<const ::ParamWrapper *>(cppSelf)->getNumDimensions();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ParamFunc_getParent(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getParent()const
            Param * cppResult = const_cast<const ::ParamWrapper *>(cppSelf)->getParent();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PARAM_IDX]), cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ParamFunc_getScriptName(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getScriptName()const
            QString cppResult = const_cast<const ::ParamWrapper *>(cppSelf)->getScriptName();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ParamFunc_getTypeName(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getTypeName()const
            QString cppResult = const_cast<const ::ParamWrapper *>(cppSelf)->getTypeName();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ParamFunc_random(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.random(): too many arguments");
        return {};
    }

    if (!PyArg_ParseTuple(args, "|OOOO:random", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: Param::random(double,double)const
    // 1: Param::random(double,double,double,uint)const
    if (numArgs == 0) {
        overloadId = 0; // random(double,double)const
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // random(double,double)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
            if (numArgs == 2) {
                overloadId = 0; // random(double,double)const
            } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
                if (numArgs == 3) {
                    overloadId = 1; // random(double,double,double,uint)const
                } else if ((pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<unsigned int>(), (pyArgs[3])))) {
                    overloadId = 1; // random(double,double,double,uint)const
                }
            }
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_random_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // random(double min, double max) const
        {
            if (kwds) {
                PyObject *keyName = nullptr;
                PyObject *value = nullptr;
                keyName = Py_BuildValue("s","min");
                if (PyDict_Contains(kwds, keyName)) {
                    value = PyDict_GetItem(kwds, keyName);
                    if (value && pyArgs[0]) {
                        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.random(): got multiple values for keyword argument 'min'.");
                        return {};
                    }
                    if (value) {
                        pyArgs[0] = value;
                        if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0]))))
                            goto Sbk_ParamFunc_random_TypeError;
                    }
                }
                keyName = Py_BuildValue("s","max");
                if (PyDict_Contains(kwds, keyName)) {
                    value = PyDict_GetItem(kwds, keyName);
                    if (value && pyArgs[1]) {
                        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.random(): got multiple values for keyword argument 'max'.");
                        return {};
                    }
                    if (value) {
                        pyArgs[1] = value;
                        if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1]))))
                            goto Sbk_ParamFunc_random_TypeError;
                    }
                }
            }
            double cppArg0 = 0.;
            if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1 = 1.;
            if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // random(double,double)const
                double cppResult = const_cast<const ::ParamWrapper *>(cppSelf)->random(cppArg0, cppArg1);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
            }
            break;
        }
        case 1: // random(double min, double max, double time, unsigned int seed) const
        {
            if (kwds) {
                PyObject *keyName = nullptr;
                PyObject *value = nullptr;
                keyName = Py_BuildValue("s","seed");
                if (PyDict_Contains(kwds, keyName)) {
                    value = PyDict_GetItem(kwds, keyName);
                    if (value && pyArgs[3]) {
                        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.random(): got multiple values for keyword argument 'seed'.");
                        return {};
                    }
                    if (value) {
                        pyArgs[3] = value;
                        if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<unsigned int>(), (pyArgs[3]))))
                            goto Sbk_ParamFunc_random_TypeError;
                    }
                }
            }
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            double cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);
            unsigned int cppArg3 = 0;
            if (pythonToCpp[3]) pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // random(double,double,double,uint)const
                double cppResult = const_cast<const ::ParamWrapper *>(cppSelf)->random(cppArg0, cppArg1, cppArg2, cppArg3);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_ParamFunc_random_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Param.random");
        return {};
}

static PyObject *Sbk_ParamFunc_randomInt(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 4) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.randomInt(): too many arguments");
        return {};
    } else if (numArgs < 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.randomInt(): not enough arguments");
        return {};
    }

    if (!PyArg_ParseTuple(args, "|OOOO:randomInt", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: Param::randomInt(int,int)
    // 1: Param::randomInt(int,int,double,uint)const
    if (numArgs >= 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
        if (numArgs == 2) {
            overloadId = 0; // randomInt(int,int)
        } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
            if (numArgs == 3) {
                overloadId = 1; // randomInt(int,int,double,uint)const
            } else if ((pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<unsigned int>(), (pyArgs[3])))) {
                overloadId = 1; // randomInt(int,int,double,uint)const
            }
        }
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
        case 1: // randomInt(int min, int max, double time, unsigned int seed) const
        {
            if (kwds) {
                PyObject *keyName = nullptr;
                PyObject *value = nullptr;
                keyName = Py_BuildValue("s","seed");
                if (PyDict_Contains(kwds, keyName)) {
                    value = PyDict_GetItem(kwds, keyName);
                    if (value && pyArgs[3]) {
                        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.randomInt(): got multiple values for keyword argument 'seed'.");
                        return {};
                    }
                    if (value) {
                        pyArgs[3] = value;
                        if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<unsigned int>(), (pyArgs[3]))))
                            goto Sbk_ParamFunc_randomInt_TypeError;
                    }
                }
            }
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            double cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);
            unsigned int cppArg3 = 0;
            if (pythonToCpp[3]) pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // randomInt(int,int,double,uint)const
                int cppResult = const_cast<const ::ParamWrapper *>(cppSelf)->randomInt(cppArg0, cppArg1, cppArg2, cppArg3);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_ParamFunc_randomInt_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Param.randomInt");
        return {};
}

static PyObject *Sbk_ParamFunc_setAddNewLine(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Param::setAddNewLine(bool)
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
        return {};
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_setAddNewLine_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Param.setAddNewLine");
        return {};
}

static PyObject *Sbk_ParamFunc_setAnimationEnabled(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Param::setAnimationEnabled(bool)
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
        return {};
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_setAnimationEnabled_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Param.setAnimationEnabled");
        return {};
}

static PyObject *Sbk_ParamFunc_setAsAlias(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Param::setAsAlias(Param*)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PARAM_IDX]), (pyArg)))) {
        overloadId = 0; // setAsAlias(Param*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_setAsAlias_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArg))
            return {};
        ::Param *cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setAsAlias(Param*)
            bool cppResult = cppSelf->setAsAlias(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_ParamFunc_setAsAlias_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Param.setAsAlias");
        return {};
}

static PyObject *Sbk_ParamFunc_setEnabled(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.setEnabled(): too many arguments");
        return {};
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.setEnabled(): not enough arguments");
        return {};
    }

    if (!PyArg_ParseTuple(args, "|OO:setEnabled", &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: Param::setEnabled(bool,int)
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
            PyObject *keyName = nullptr;
            PyObject *value = nullptr;
            keyName = Py_BuildValue("s","dimension");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[1]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.setEnabled(): got multiple values for keyword argument 'dimension'.");
                    return {};
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_ParamFunc_setEnabled_TypeError;
                }
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
        return {};
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_setEnabled_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Param.setEnabled");
        return {};
}

static PyObject *Sbk_ParamFunc_setEnabledByDefault(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.setEnabledByDefault(): too many arguments");
        return {};
    }

    if (!PyArg_ParseTuple(args, "|O:setEnabledByDefault", &(pyArgs[0])))
        return {};


    // Overloaded function decisor
    // 0: Param::setEnabledByDefault(bool)
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
            PyObject *keyName = nullptr;
            PyObject *value = nullptr;
            keyName = Py_BuildValue("s","enabled");
            if (PyDict_Contains(kwds, keyName)) {
                value = PyDict_GetItem(kwds, keyName);
                if (value && pyArgs[0]) {
                    PyErr_SetString(PyExc_TypeError, "NatronEngine.Param.setEnabledByDefault(): got multiple values for keyword argument 'enabled'.");
                    return {};
                }
                if (value) {
                    pyArgs[0] = value;
                    if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[0]))))
                        goto Sbk_ParamFunc_setEnabledByDefault_TypeError;
                }
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
        return {};
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_setEnabledByDefault_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Param.setEnabledByDefault");
        return {};
}

static PyObject *Sbk_ParamFunc_setEvaluateOnChange(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Param::setEvaluateOnChange(bool)
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
        return {};
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_setEvaluateOnChange_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Param.setEvaluateOnChange");
        return {};
}

static PyObject *Sbk_ParamFunc_setHelp(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Param::setHelp(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // setHelp(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_setHelp_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setHelp(QString)
            cppSelf->setHelp(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_setHelp_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Param.setHelp");
        return {};
}

static PyObject *Sbk_ParamFunc_setIconFilePath(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Param::setIconFilePath(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // setIconFilePath(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_setIconFilePath_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setIconFilePath(QString)
            cppSelf->setIconFilePath(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_setIconFilePath_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Param.setIconFilePath");
        return {};
}

static PyObject *Sbk_ParamFunc_setPersistent(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Param::setPersistent(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setPersistent(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_setPersistent_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setPersistent(bool)
            cppSelf->setPersistent(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_setPersistent_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Param.setPersistent");
        return {};
}

static PyObject *Sbk_ParamFunc_setVisible(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Param::setVisible(bool)
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
        return {};
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_setVisible_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Param.setVisible");
        return {};
}

static PyObject *Sbk_ParamFunc_setVisibleByDefault(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Param::setVisibleByDefault(bool)
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
        return {};
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_setVisibleByDefault_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Param.setVisibleByDefault");
        return {};
}

static PyObject *Sbk_ParamFunc_slaveTo(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "slaveTo", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return {};


    // Overloaded function decisor
    // 0: Param::slaveTo(Param*,int,int)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PARAM_IDX]), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
        overloadId = 0; // slaveTo(Param*,int,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ParamFunc_slaveTo_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArgs[0]))
            return {};
        ::Param *cppArg0;
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
        return {};
    }
    return pyResult;

    Sbk_ParamFunc_slaveTo_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.Param.slaveTo");
        return {};
}

static PyObject *Sbk_ParamFunc_unslave(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<ParamWrapper *>(reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Param::unslave(int)
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
        return {};
    }
    Py_RETURN_NONE;

    Sbk_ParamFunc_unslave_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.Param.unslave");
        return {};
}


static const char *Sbk_Param_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_Param_methods[] = {
    {"_addAsDependencyOf", reinterpret_cast<PyCFunction>(Sbk_ParamFunc__addAsDependencyOf), METH_VARARGS},
    {"copy", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_copy), METH_VARARGS|METH_KEYWORDS},
    {"curve", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_curve), METH_VARARGS|METH_KEYWORDS},
    {"getAddNewLine", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_getAddNewLine), METH_NOARGS},
    {"getCanAnimate", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_getCanAnimate), METH_NOARGS},
    {"getEvaluateOnChange", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_getEvaluateOnChange), METH_NOARGS},
    {"getHelp", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_getHelp), METH_NOARGS},
    {"getIsAnimationEnabled", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_getIsAnimationEnabled), METH_NOARGS},
    {"getIsEnabled", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_getIsEnabled), METH_VARARGS|METH_KEYWORDS},
    {"getIsPersistent", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_getIsPersistent), METH_NOARGS},
    {"getIsVisible", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_getIsVisible), METH_NOARGS},
    {"getLabel", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_getLabel), METH_NOARGS},
    {"getNumDimensions", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_getNumDimensions), METH_NOARGS},
    {"getParent", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_getParent), METH_NOARGS},
    {"getScriptName", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_getScriptName), METH_NOARGS},
    {"getTypeName", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_getTypeName), METH_NOARGS},
    {"random", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_random), METH_VARARGS|METH_KEYWORDS},
    {"randomInt", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_randomInt), METH_VARARGS|METH_KEYWORDS},
    {"setAddNewLine", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_setAddNewLine), METH_O},
    {"setAnimationEnabled", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_setAnimationEnabled), METH_O},
    {"setAsAlias", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_setAsAlias), METH_O},
    {"setEnabled", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_setEnabled), METH_VARARGS|METH_KEYWORDS},
    {"setEnabledByDefault", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_setEnabledByDefault), METH_VARARGS|METH_KEYWORDS},
    {"setEvaluateOnChange", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_setEvaluateOnChange), METH_O},
    {"setHelp", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_setHelp), METH_O},
    {"setIconFilePath", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_setIconFilePath), METH_O},
    {"setPersistent", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_setPersistent), METH_O},
    {"setVisible", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_setVisible), METH_O},
    {"setVisibleByDefault", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_setVisibleByDefault), METH_O},
    {"slaveTo", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_slaveTo), METH_VARARGS},
    {"unslave", reinterpret_cast<PyCFunction>(Sbk_ParamFunc_unslave), METH_O},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_Param_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::Param *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PARAM_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<ParamWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_Param_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_Param_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_Param_Type = nullptr;
static SbkObjectType *Sbk_Param_TypeF(void)
{
    return _Sbk_Param_Type;
}

static PyType_Slot Sbk_Param_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_Param_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_Param_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_Param_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_Param_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_Param_spec = {
    "1:NatronEngine.Param",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_Param_slots
};

} //extern "C"


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void Param_PythonToCpp_Param_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_Param_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_Param_PythonToCpp_Param_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_Param_TypeF())))
        return Param_PythonToCpp_Param_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *Param_PTR_CppToPython_Param(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::Param *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_Param_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *Param_SignatureStrings[] = {
    "NatronEngine.Param._addAsDependencyOf(self,fromExprDimension:int,param:NatronEngine.Param,thisDimension:int)",
    "NatronEngine.Param.copy(self,other:NatronEngine.Param,dimension:int=-1)->bool",
    "NatronEngine.Param.curve(self,time:double,dimension:int=0)->double",
    "NatronEngine.Param.getAddNewLine(self)->bool",
    "NatronEngine.Param.getCanAnimate(self)->bool",
    "NatronEngine.Param.getEvaluateOnChange(self)->bool",
    "NatronEngine.Param.getHelp(self)->QString",
    "NatronEngine.Param.getIsAnimationEnabled(self)->bool",
    "NatronEngine.Param.getIsEnabled(self,dimension:int=0)->bool",
    "NatronEngine.Param.getIsPersistent(self)->bool",
    "NatronEngine.Param.getIsVisible(self)->bool",
    "NatronEngine.Param.getLabel(self)->QString",
    "NatronEngine.Param.getNumDimensions(self)->int",
    "NatronEngine.Param.getParent(self)->NatronEngine.Param",
    "NatronEngine.Param.getScriptName(self)->QString",
    "NatronEngine.Param.getTypeName(self)->QString",
    "1:NatronEngine.Param.random(self,min:double=0.,max:double=1.)->double",
    "0:NatronEngine.Param.random(self,min:double,max:double,time:double,seed:unsigned int=0)->double",
    "1:NatronEngine.Param.randomInt(self,min:int,max:int)->int",
    "0:NatronEngine.Param.randomInt(self,min:int,max:int,time:double,seed:unsigned int=0)->int",
    "NatronEngine.Param.setAddNewLine(self,a:bool)",
    "NatronEngine.Param.setAnimationEnabled(self,e:bool)",
    "NatronEngine.Param.setAsAlias(self,other:NatronEngine.Param)->bool",
    "NatronEngine.Param.setEnabled(self,enabled:bool,dimension:int=0)",
    "NatronEngine.Param.setEnabledByDefault(self,enabled:bool=0)",
    "NatronEngine.Param.setEvaluateOnChange(self,eval:bool)",
    "NatronEngine.Param.setHelp(self,help:QString)",
    "NatronEngine.Param.setIconFilePath(self,icon:QString)",
    "NatronEngine.Param.setPersistent(self,persistent:bool)",
    "NatronEngine.Param.setVisible(self,visible:bool)",
    "NatronEngine.Param.setVisibleByDefault(self,visible:bool)",
    "NatronEngine.Param.slaveTo(self,other:NatronEngine.Param,thisDimension:int,otherDimension:int)->bool",
    "NatronEngine.Param.unslave(self,dimension:int)",
    nullptr}; // Sentinel

void init_Param(PyObject *module)
{
    _Sbk_Param_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "Param",
        "Param*",
        &Sbk_Param_spec,
        &Shiboken::callCppDestructor< ::Param >,
        0,
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_Param_Type);
    InitSignatureStrings(pyType, Param_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_Param_Type), Sbk_Param_PropertyStrings);
    SbkNatronEngineTypes[SBK_PARAM_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_Param_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_Param_TypeF(),
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
