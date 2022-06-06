
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
#include "intparam_wrapper.h"

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

void IntParamWrapper::pysideInitQtMetaTypes()
{
}

void IntParamWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

IntParamWrapper::~IntParamWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject *Sbk_IntParamFunc_addAsDependencyOf(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::IntParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.IntParam.addAsDependencyOf";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "addAsDependencyOf", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return {};


    // Overloaded function decisor
    // 0: IntParam::addAsDependencyOf(int,Param*,int)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PARAM_IDX]), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
        overloadId = 0; // addAsDependencyOf(int,Param*,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_IntParamFunc_addAsDependencyOf_TypeError;

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
            // addAsDependencyOf(int,Param*,int)
            int cppResult = cppSelf->addAsDependencyOf(cppArg0, cppArg1, cppArg2);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_IntParamFunc_addAsDependencyOf_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_IntParamFunc_get(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::IntParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.IntParam.get";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "get", 0, 1, &(pyArgs[0])))
        return {};


    // Overloaded function decisor
    // 0: IntParam::get()const
    // 1: IntParam::get(double)const
    if (numArgs == 0) {
        overloadId = 0; // get()const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        overloadId = 1; // get(double)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_IntParamFunc_get_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // get() const
        {

            if (!PyErr_Occurred()) {
                // get()const
                int cppResult = const_cast<const ::IntParam *>(cppSelf)->get();
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
                int cppResult = const_cast<const ::IntParam *>(cppSelf)->get(cppArg0);
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

    Sbk_IntParamFunc_get_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_IntParamFunc_getDefaultValue(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::IntParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.IntParam.getDefaultValue";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs > 1) {
        static PyObject *const too_many = Shiboken::String::createStaticString(">");
        errInfo = too_many;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_getDefaultValue_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|O:getDefaultValue", &(pyArgs[0])))
        return {};


    // Overloaded function decisor
    // 0: IntParam::getDefaultValue(int)const
    if (numArgs == 0) {
        overloadId = 0; // getDefaultValue(int)const
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        overloadId = 0; // getDefaultValue(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_IntParamFunc_getDefaultValue_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_dimension = Shiboken::String::createStaticString("dimension");
            if (PyDict_Contains(kwds, key_dimension)) {
                value = PyDict_GetItem(kwds, key_dimension);
                if (value && pyArgs[0]) {
                    errInfo = key_dimension;
                    Py_INCREF(errInfo);
                    goto Sbk_IntParamFunc_getDefaultValue_TypeError;
                }
                if (value) {
                    pyArgs[0] = value;
                    if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0]))))
                        goto Sbk_IntParamFunc_getDefaultValue_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_IntParamFunc_getDefaultValue_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        int cppArg0 = 0;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // getDefaultValue(int)const
            int cppResult = const_cast<const ::IntParam *>(cppSelf)->getDefaultValue(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_IntParamFunc_getDefaultValue_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_IntParamFunc_getDisplayMaximum(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::IntParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.IntParam.getDisplayMaximum";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: IntParam::getDisplayMaximum(int)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // getDisplayMaximum(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_IntParamFunc_getDisplayMaximum_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getDisplayMaximum(int)const
            int cppResult = const_cast<const ::IntParam *>(cppSelf)->getDisplayMaximum(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_IntParamFunc_getDisplayMaximum_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_IntParamFunc_getDisplayMinimum(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::IntParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.IntParam.getDisplayMinimum";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: IntParam::getDisplayMinimum(int)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // getDisplayMinimum(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_IntParamFunc_getDisplayMinimum_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getDisplayMinimum(int)const
            int cppResult = const_cast<const ::IntParam *>(cppSelf)->getDisplayMinimum(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_IntParamFunc_getDisplayMinimum_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_IntParamFunc_getMaximum(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::IntParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.IntParam.getMaximum";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs > 1) {
        static PyObject *const too_many = Shiboken::String::createStaticString(">");
        errInfo = too_many;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_getMaximum_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|O:getMaximum", &(pyArgs[0])))
        return {};


    // Overloaded function decisor
    // 0: IntParam::getMaximum(int)const
    if (numArgs == 0) {
        overloadId = 0; // getMaximum(int)const
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        overloadId = 0; // getMaximum(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_IntParamFunc_getMaximum_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_dimension = Shiboken::String::createStaticString("dimension");
            if (PyDict_Contains(kwds, key_dimension)) {
                value = PyDict_GetItem(kwds, key_dimension);
                if (value && pyArgs[0]) {
                    errInfo = key_dimension;
                    Py_INCREF(errInfo);
                    goto Sbk_IntParamFunc_getMaximum_TypeError;
                }
                if (value) {
                    pyArgs[0] = value;
                    if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0]))))
                        goto Sbk_IntParamFunc_getMaximum_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_IntParamFunc_getMaximum_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        int cppArg0 = 0;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // getMaximum(int)const
            int cppResult = const_cast<const ::IntParam *>(cppSelf)->getMaximum(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_IntParamFunc_getMaximum_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_IntParamFunc_getMinimum(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::IntParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.IntParam.getMinimum";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs > 1) {
        static PyObject *const too_many = Shiboken::String::createStaticString(">");
        errInfo = too_many;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_getMinimum_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|O:getMinimum", &(pyArgs[0])))
        return {};


    // Overloaded function decisor
    // 0: IntParam::getMinimum(int)const
    if (numArgs == 0) {
        overloadId = 0; // getMinimum(int)const
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        overloadId = 0; // getMinimum(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_IntParamFunc_getMinimum_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_dimension = Shiboken::String::createStaticString("dimension");
            if (PyDict_Contains(kwds, key_dimension)) {
                value = PyDict_GetItem(kwds, key_dimension);
                if (value && pyArgs[0]) {
                    errInfo = key_dimension;
                    Py_INCREF(errInfo);
                    goto Sbk_IntParamFunc_getMinimum_TypeError;
                }
                if (value) {
                    pyArgs[0] = value;
                    if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0]))))
                        goto Sbk_IntParamFunc_getMinimum_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_IntParamFunc_getMinimum_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        int cppArg0 = 0;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // getMinimum(int)const
            int cppResult = const_cast<const ::IntParam *>(cppSelf)->getMinimum(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_IntParamFunc_getMinimum_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_IntParamFunc_getValue(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::IntParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.IntParam.getValue";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs > 1) {
        static PyObject *const too_many = Shiboken::String::createStaticString(">");
        errInfo = too_many;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_getValue_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|O:getValue", &(pyArgs[0])))
        return {};


    // Overloaded function decisor
    // 0: IntParam::getValue(int)const
    if (numArgs == 0) {
        overloadId = 0; // getValue(int)const
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        overloadId = 0; // getValue(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_IntParamFunc_getValue_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_dimension = Shiboken::String::createStaticString("dimension");
            if (PyDict_Contains(kwds, key_dimension)) {
                value = PyDict_GetItem(kwds, key_dimension);
                if (value && pyArgs[0]) {
                    errInfo = key_dimension;
                    Py_INCREF(errInfo);
                    goto Sbk_IntParamFunc_getValue_TypeError;
                }
                if (value) {
                    pyArgs[0] = value;
                    if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0]))))
                        goto Sbk_IntParamFunc_getValue_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_IntParamFunc_getValue_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        int cppArg0 = 0;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // getValue(int)const
            int cppResult = const_cast<const ::IntParam *>(cppSelf)->getValue(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_IntParamFunc_getValue_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_IntParamFunc_getValueAtTime(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::IntParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.IntParam.getValueAtTime";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs > 2) {
        static PyObject *const too_many = Shiboken::String::createStaticString(">");
        errInfo = too_many;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_getValueAtTime_TypeError;
    } else if (numArgs < 1) {
        static PyObject *const too_few = Shiboken::String::createStaticString("<");
        errInfo = too_few;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_getValueAtTime_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|OO:getValueAtTime", &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: IntParam::getValueAtTime(double,int)const
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // getValueAtTime(double,int)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            overloadId = 0; // getValueAtTime(double,int)const
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_IntParamFunc_getValueAtTime_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_dimension = Shiboken::String::createStaticString("dimension");
            if (PyDict_Contains(kwds, key_dimension)) {
                value = PyDict_GetItem(kwds, key_dimension);
                if (value && pyArgs[1]) {
                    errInfo = key_dimension;
                    Py_INCREF(errInfo);
                    goto Sbk_IntParamFunc_getValueAtTime_TypeError;
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_IntParamFunc_getValueAtTime_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_IntParamFunc_getValueAtTime_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getValueAtTime(double,int)const
            int cppResult = const_cast<const ::IntParam *>(cppSelf)->getValueAtTime(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_IntParamFunc_getValueAtTime_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_IntParamFunc_restoreDefaultValue(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::IntParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.IntParam.restoreDefaultValue";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs > 1) {
        static PyObject *const too_many = Shiboken::String::createStaticString(">");
        errInfo = too_many;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_restoreDefaultValue_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|O:restoreDefaultValue", &(pyArgs[0])))
        return {};


    // Overloaded function decisor
    // 0: IntParam::restoreDefaultValue(int)
    if (numArgs == 0) {
        overloadId = 0; // restoreDefaultValue(int)
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        overloadId = 0; // restoreDefaultValue(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_IntParamFunc_restoreDefaultValue_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_dimension = Shiboken::String::createStaticString("dimension");
            if (PyDict_Contains(kwds, key_dimension)) {
                value = PyDict_GetItem(kwds, key_dimension);
                if (value && pyArgs[0]) {
                    errInfo = key_dimension;
                    Py_INCREF(errInfo);
                    goto Sbk_IntParamFunc_restoreDefaultValue_TypeError;
                }
                if (value) {
                    pyArgs[0] = value;
                    if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0]))))
                        goto Sbk_IntParamFunc_restoreDefaultValue_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_IntParamFunc_restoreDefaultValue_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        int cppArg0 = 0;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // restoreDefaultValue(int)
            // Begin code injection
            cppSelf->restoreDefaultValue(cppArg0);

            // End of code injection

        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_IntParamFunc_restoreDefaultValue_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_IntParamFunc_set(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::IntParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.IntParam.set";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "set", 1, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: IntParam::set(int)
    // 1: IntParam::set(int,double)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // set(int)
        } else if (numArgs == 2
            && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
            overloadId = 1; // set(int,double)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_IntParamFunc_set_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // set(int x)
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // set(int)
                // Begin code injection
                cppSelf->set(cppArg0);

                // End of code injection

            }
            break;
        }
        case 1: // set(int x, double frame)
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // set(int,double)
                // Begin code injection
                cppSelf->set(cppArg0,cppArg1);

                // End of code injection

            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_IntParamFunc_set_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_IntParamFunc_setDefaultValue(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::IntParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.IntParam.setDefaultValue";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs > 2) {
        static PyObject *const too_many = Shiboken::String::createStaticString(">");
        errInfo = too_many;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_setDefaultValue_TypeError;
    } else if (numArgs < 1) {
        static PyObject *const too_few = Shiboken::String::createStaticString("<");
        errInfo = too_few;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_setDefaultValue_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|OO:setDefaultValue", &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: IntParam::setDefaultValue(int,int)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // setDefaultValue(int,int)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            overloadId = 0; // setDefaultValue(int,int)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_IntParamFunc_setDefaultValue_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_dimension = Shiboken::String::createStaticString("dimension");
            if (PyDict_Contains(kwds, key_dimension)) {
                value = PyDict_GetItem(kwds, key_dimension);
                if (value && pyArgs[1]) {
                    errInfo = key_dimension;
                    Py_INCREF(errInfo);
                    goto Sbk_IntParamFunc_setDefaultValue_TypeError;
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_IntParamFunc_setDefaultValue_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_IntParamFunc_setDefaultValue_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setDefaultValue(int,int)
            cppSelf->setDefaultValue(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_IntParamFunc_setDefaultValue_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_IntParamFunc_setDisplayMaximum(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::IntParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.IntParam.setDisplayMaximum";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs > 2) {
        static PyObject *const too_many = Shiboken::String::createStaticString(">");
        errInfo = too_many;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_setDisplayMaximum_TypeError;
    } else if (numArgs < 1) {
        static PyObject *const too_few = Shiboken::String::createStaticString("<");
        errInfo = too_few;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_setDisplayMaximum_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|OO:setDisplayMaximum", &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: IntParam::setDisplayMaximum(int,int)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // setDisplayMaximum(int,int)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            overloadId = 0; // setDisplayMaximum(int,int)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_IntParamFunc_setDisplayMaximum_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_dimension = Shiboken::String::createStaticString("dimension");
            if (PyDict_Contains(kwds, key_dimension)) {
                value = PyDict_GetItem(kwds, key_dimension);
                if (value && pyArgs[1]) {
                    errInfo = key_dimension;
                    Py_INCREF(errInfo);
                    goto Sbk_IntParamFunc_setDisplayMaximum_TypeError;
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_IntParamFunc_setDisplayMaximum_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_IntParamFunc_setDisplayMaximum_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setDisplayMaximum(int,int)
            cppSelf->setDisplayMaximum(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_IntParamFunc_setDisplayMaximum_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_IntParamFunc_setDisplayMinimum(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::IntParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.IntParam.setDisplayMinimum";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs > 2) {
        static PyObject *const too_many = Shiboken::String::createStaticString(">");
        errInfo = too_many;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_setDisplayMinimum_TypeError;
    } else if (numArgs < 1) {
        static PyObject *const too_few = Shiboken::String::createStaticString("<");
        errInfo = too_few;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_setDisplayMinimum_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|OO:setDisplayMinimum", &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: IntParam::setDisplayMinimum(int,int)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // setDisplayMinimum(int,int)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            overloadId = 0; // setDisplayMinimum(int,int)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_IntParamFunc_setDisplayMinimum_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_dimension = Shiboken::String::createStaticString("dimension");
            if (PyDict_Contains(kwds, key_dimension)) {
                value = PyDict_GetItem(kwds, key_dimension);
                if (value && pyArgs[1]) {
                    errInfo = key_dimension;
                    Py_INCREF(errInfo);
                    goto Sbk_IntParamFunc_setDisplayMinimum_TypeError;
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_IntParamFunc_setDisplayMinimum_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_IntParamFunc_setDisplayMinimum_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setDisplayMinimum(int,int)
            cppSelf->setDisplayMinimum(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_IntParamFunc_setDisplayMinimum_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_IntParamFunc_setMaximum(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::IntParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.IntParam.setMaximum";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs > 2) {
        static PyObject *const too_many = Shiboken::String::createStaticString(">");
        errInfo = too_many;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_setMaximum_TypeError;
    } else if (numArgs < 1) {
        static PyObject *const too_few = Shiboken::String::createStaticString("<");
        errInfo = too_few;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_setMaximum_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|OO:setMaximum", &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: IntParam::setMaximum(int,int)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // setMaximum(int,int)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            overloadId = 0; // setMaximum(int,int)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_IntParamFunc_setMaximum_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_dimension = Shiboken::String::createStaticString("dimension");
            if (PyDict_Contains(kwds, key_dimension)) {
                value = PyDict_GetItem(kwds, key_dimension);
                if (value && pyArgs[1]) {
                    errInfo = key_dimension;
                    Py_INCREF(errInfo);
                    goto Sbk_IntParamFunc_setMaximum_TypeError;
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_IntParamFunc_setMaximum_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_IntParamFunc_setMaximum_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setMaximum(int,int)
            cppSelf->setMaximum(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_IntParamFunc_setMaximum_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_IntParamFunc_setMinimum(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::IntParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.IntParam.setMinimum";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs > 2) {
        static PyObject *const too_many = Shiboken::String::createStaticString(">");
        errInfo = too_many;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_setMinimum_TypeError;
    } else if (numArgs < 1) {
        static PyObject *const too_few = Shiboken::String::createStaticString("<");
        errInfo = too_few;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_setMinimum_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|OO:setMinimum", &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: IntParam::setMinimum(int,int)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // setMinimum(int,int)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            overloadId = 0; // setMinimum(int,int)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_IntParamFunc_setMinimum_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_dimension = Shiboken::String::createStaticString("dimension");
            if (PyDict_Contains(kwds, key_dimension)) {
                value = PyDict_GetItem(kwds, key_dimension);
                if (value && pyArgs[1]) {
                    errInfo = key_dimension;
                    Py_INCREF(errInfo);
                    goto Sbk_IntParamFunc_setMinimum_TypeError;
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_IntParamFunc_setMinimum_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_IntParamFunc_setMinimum_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setMinimum(int,int)
            cppSelf->setMinimum(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_IntParamFunc_setMinimum_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_IntParamFunc_setValue(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::IntParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.IntParam.setValue";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs > 2) {
        static PyObject *const too_many = Shiboken::String::createStaticString(">");
        errInfo = too_many;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_setValue_TypeError;
    } else if (numArgs < 1) {
        static PyObject *const too_few = Shiboken::String::createStaticString("<");
        errInfo = too_few;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_setValue_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|OO:setValue", &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: IntParam::setValue(int,int)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // setValue(int,int)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            overloadId = 0; // setValue(int,int)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_IntParamFunc_setValue_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_dimension = Shiboken::String::createStaticString("dimension");
            if (PyDict_Contains(kwds, key_dimension)) {
                value = PyDict_GetItem(kwds, key_dimension);
                if (value && pyArgs[1]) {
                    errInfo = key_dimension;
                    Py_INCREF(errInfo);
                    goto Sbk_IntParamFunc_setValue_TypeError;
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_IntParamFunc_setValue_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_IntParamFunc_setValue_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setValue(int,int)
            // Begin code injection
            cppSelf->setValue(cppArg0,cppArg1);

            // End of code injection

        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_IntParamFunc_setValue_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_IntParamFunc_setValueAtTime(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::IntParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.IntParam.setValueAtTime";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0};

    // invalid argument lengths
    if (numArgs > 3) {
        static PyObject *const too_many = Shiboken::String::createStaticString(">");
        errInfo = too_many;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_setValueAtTime_TypeError;
    } else if (numArgs < 2) {
        static PyObject *const too_few = Shiboken::String::createStaticString("<");
        errInfo = too_few;
        Py_INCREF(errInfo);
        goto Sbk_IntParamFunc_setValueAtTime_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|OOO:setValueAtTime", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return {};


    // Overloaded function decisor
    // 0: IntParam::setValueAtTime(int,double,int)
    if (numArgs >= 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        if (numArgs == 2) {
            overloadId = 0; // setValueAtTime(int,double,int)
        } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
            overloadId = 0; // setValueAtTime(int,double,int)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_IntParamFunc_setValueAtTime_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_dimension = Shiboken::String::createStaticString("dimension");
            if (PyDict_Contains(kwds, key_dimension)) {
                value = PyDict_GetItem(kwds, key_dimension);
                if (value && pyArgs[2]) {
                    errInfo = key_dimension;
                    Py_INCREF(errInfo);
                    goto Sbk_IntParamFunc_setValueAtTime_TypeError;
                }
                if (value) {
                    pyArgs[2] = value;
                    if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2]))))
                        goto Sbk_IntParamFunc_setValueAtTime_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_IntParamFunc_setValueAtTime_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        int cppArg2 = 0;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // setValueAtTime(int,double,int)
            // Begin code injection
            cppSelf->setValueAtTime(cppArg0,cppArg1,cppArg2);

            // End of code injection

        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_IntParamFunc_setValueAtTime_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}


static const char *Sbk_IntParam_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_IntParam_methods[] = {
    {"addAsDependencyOf", reinterpret_cast<PyCFunction>(Sbk_IntParamFunc_addAsDependencyOf), METH_VARARGS},
    {"get", reinterpret_cast<PyCFunction>(Sbk_IntParamFunc_get), METH_VARARGS},
    {"getDefaultValue", reinterpret_cast<PyCFunction>(Sbk_IntParamFunc_getDefaultValue), METH_VARARGS|METH_KEYWORDS},
    {"getDisplayMaximum", reinterpret_cast<PyCFunction>(Sbk_IntParamFunc_getDisplayMaximum), METH_O},
    {"getDisplayMinimum", reinterpret_cast<PyCFunction>(Sbk_IntParamFunc_getDisplayMinimum), METH_O},
    {"getMaximum", reinterpret_cast<PyCFunction>(Sbk_IntParamFunc_getMaximum), METH_VARARGS|METH_KEYWORDS},
    {"getMinimum", reinterpret_cast<PyCFunction>(Sbk_IntParamFunc_getMinimum), METH_VARARGS|METH_KEYWORDS},
    {"getValue", reinterpret_cast<PyCFunction>(Sbk_IntParamFunc_getValue), METH_VARARGS|METH_KEYWORDS},
    {"getValueAtTime", reinterpret_cast<PyCFunction>(Sbk_IntParamFunc_getValueAtTime), METH_VARARGS|METH_KEYWORDS},
    {"restoreDefaultValue", reinterpret_cast<PyCFunction>(Sbk_IntParamFunc_restoreDefaultValue), METH_VARARGS|METH_KEYWORDS},
    {"set", reinterpret_cast<PyCFunction>(Sbk_IntParamFunc_set), METH_VARARGS},
    {"setDefaultValue", reinterpret_cast<PyCFunction>(Sbk_IntParamFunc_setDefaultValue), METH_VARARGS|METH_KEYWORDS},
    {"setDisplayMaximum", reinterpret_cast<PyCFunction>(Sbk_IntParamFunc_setDisplayMaximum), METH_VARARGS|METH_KEYWORDS},
    {"setDisplayMinimum", reinterpret_cast<PyCFunction>(Sbk_IntParamFunc_setDisplayMinimum), METH_VARARGS|METH_KEYWORDS},
    {"setMaximum", reinterpret_cast<PyCFunction>(Sbk_IntParamFunc_setMaximum), METH_VARARGS|METH_KEYWORDS},
    {"setMinimum", reinterpret_cast<PyCFunction>(Sbk_IntParamFunc_setMinimum), METH_VARARGS|METH_KEYWORDS},
    {"setValue", reinterpret_cast<PyCFunction>(Sbk_IntParamFunc_setValue), METH_VARARGS|METH_KEYWORDS},
    {"setValueAtTime", reinterpret_cast<PyCFunction>(Sbk_IntParamFunc_setValueAtTime), METH_VARARGS|METH_KEYWORDS},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_IntParam_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::IntParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_INTPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<IntParamWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_IntParam_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_IntParam_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_IntParam_Type = nullptr;
static SbkObjectType *Sbk_IntParam_TypeF(void)
{
    return _Sbk_IntParam_Type;
}

static PyType_Slot Sbk_IntParam_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_IntParam_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_IntParam_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_IntParam_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_IntParam_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_IntParam_spec = {
    "1:NatronEngine.IntParam",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_IntParam_slots
};

} //extern "C"

static void *Sbk_IntParam_typeDiscovery(void *cptr, SbkObjectType *instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::IntParam *>(reinterpret_cast< ::Param *>(cptr));
    return {};
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void IntParam_PythonToCpp_IntParam_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_IntParam_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_IntParam_PythonToCpp_IntParam_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_IntParam_TypeF())))
        return IntParam_PythonToCpp_IntParam_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *IntParam_PTR_CppToPython_IntParam(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::IntParam *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_IntParam_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *IntParam_SignatureStrings[] = {
    "NatronEngine.IntParam.addAsDependencyOf(self,fromExprDimension:int,param:NatronEngine.Param,thisDimension:int)->int",
    "1:NatronEngine.IntParam.get(self)->int",
    "0:NatronEngine.IntParam.get(self,frame:double)->int",
    "NatronEngine.IntParam.getDefaultValue(self,dimension:int=0)->int",
    "NatronEngine.IntParam.getDisplayMaximum(self,dimension:int)->int",
    "NatronEngine.IntParam.getDisplayMinimum(self,dimension:int)->int",
    "NatronEngine.IntParam.getMaximum(self,dimension:int=0)->int",
    "NatronEngine.IntParam.getMinimum(self,dimension:int=0)->int",
    "NatronEngine.IntParam.getValue(self,dimension:int=0)->int",
    "NatronEngine.IntParam.getValueAtTime(self,time:double,dimension:int=0)->int",
    "NatronEngine.IntParam.restoreDefaultValue(self,dimension:int=0)",
    "1:NatronEngine.IntParam.set(self,x:int)",
    "0:NatronEngine.IntParam.set(self,x:int,frame:double)",
    "NatronEngine.IntParam.setDefaultValue(self,value:int,dimension:int=0)",
    "NatronEngine.IntParam.setDisplayMaximum(self,maximum:int,dimension:int=0)",
    "NatronEngine.IntParam.setDisplayMinimum(self,minimum:int,dimension:int=0)",
    "NatronEngine.IntParam.setMaximum(self,maximum:int,dimension:int=0)",
    "NatronEngine.IntParam.setMinimum(self,minimum:int,dimension:int=0)",
    "NatronEngine.IntParam.setValue(self,value:int,dimension:int=0)",
    "NatronEngine.IntParam.setValueAtTime(self,value:int,time:double,dimension:int=0)",
    nullptr}; // Sentinel

void init_IntParam(PyObject *module)
{
    _Sbk_IntParam_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "IntParam",
        "IntParam*",
        &Sbk_IntParam_spec,
        &Shiboken::callCppDestructor< ::IntParam >,
        reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX]),
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_IntParam_Type);
    InitSignatureStrings(pyType, IntParam_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_IntParam_Type), Sbk_IntParam_PropertyStrings);
    SbkNatronEngineTypes[SBK_INTPARAM_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_IntParam_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_IntParam_TypeF(),
        IntParam_PythonToCpp_IntParam_PTR,
        is_IntParam_PythonToCpp_IntParam_PTR_Convertible,
        IntParam_PTR_CppToPython_IntParam);

    Shiboken::Conversions::registerConverterName(converter, "IntParam");
    Shiboken::Conversions::registerConverterName(converter, "IntParam*");
    Shiboken::Conversions::registerConverterName(converter, "IntParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::IntParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::IntParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(Sbk_IntParam_TypeF(), &Sbk_IntParam_typeDiscovery);

    IntParamWrapper::pysideInitQtMetaTypes();
}
