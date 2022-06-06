
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
#include "animatedparam_wrapper.h"

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

void AnimatedParamWrapper::pysideInitQtMetaTypes()
{
}

void AnimatedParamWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

AnimatedParamWrapper::~AnimatedParamWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject *Sbk_AnimatedParamFunc_deleteValueAtTime(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AnimatedParamWrapper *>(reinterpret_cast< ::AnimatedParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.AnimatedParam.deleteValueAtTime";
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
        goto Sbk_AnimatedParamFunc_deleteValueAtTime_TypeError;
    } else if (numArgs < 1) {
        static PyObject *const too_few = Shiboken::String::createStaticString("<");
        errInfo = too_few;
        Py_INCREF(errInfo);
        goto Sbk_AnimatedParamFunc_deleteValueAtTime_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|OO:deleteValueAtTime", &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: AnimatedParam::deleteValueAtTime(double,int)
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // deleteValueAtTime(double,int)
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            overloadId = 0; // deleteValueAtTime(double,int)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AnimatedParamFunc_deleteValueAtTime_TypeError;

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
                    goto Sbk_AnimatedParamFunc_deleteValueAtTime_TypeError;
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_AnimatedParamFunc_deleteValueAtTime_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_AnimatedParamFunc_deleteValueAtTime_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // deleteValueAtTime(double,int)
            cppSelf->deleteValueAtTime(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_AnimatedParamFunc_deleteValueAtTime_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AnimatedParamFunc_getCurrentTime(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AnimatedParamWrapper *>(reinterpret_cast< ::AnimatedParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.AnimatedParam.getCurrentTime";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getCurrentTime()const
            int cppResult = const_cast<const ::AnimatedParamWrapper *>(cppSelf)->getCurrentTime();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_AnimatedParamFunc_getDerivativeAtTime(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AnimatedParamWrapper *>(reinterpret_cast< ::AnimatedParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.AnimatedParam.getDerivativeAtTime";
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
        goto Sbk_AnimatedParamFunc_getDerivativeAtTime_TypeError;
    } else if (numArgs < 1) {
        static PyObject *const too_few = Shiboken::String::createStaticString("<");
        errInfo = too_few;
        Py_INCREF(errInfo);
        goto Sbk_AnimatedParamFunc_getDerivativeAtTime_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|OO:getDerivativeAtTime", &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: AnimatedParam::getDerivativeAtTime(double,int)const
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
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_dimension = Shiboken::String::createStaticString("dimension");
            if (PyDict_Contains(kwds, key_dimension)) {
                value = PyDict_GetItem(kwds, key_dimension);
                if (value && pyArgs[1]) {
                    errInfo = key_dimension;
                    Py_INCREF(errInfo);
                    goto Sbk_AnimatedParamFunc_getDerivativeAtTime_TypeError;
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_AnimatedParamFunc_getDerivativeAtTime_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_AnimatedParamFunc_getDerivativeAtTime_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getDerivativeAtTime(double,int)const
            double cppResult = const_cast<const ::AnimatedParamWrapper *>(cppSelf)->getDerivativeAtTime(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_AnimatedParamFunc_getDerivativeAtTime_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AnimatedParamFunc_getExpression(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AnimatedParamWrapper *>(reinterpret_cast< ::AnimatedParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.AnimatedParam.getExpression";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: AnimatedParam::getExpression(int,bool*)const
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
            QString cppResult = cppSelf->getExpression(cppArg0,&hasRetVar);
            pyResult = PyTuple_New(2);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &hasRetVar));
            return pyResult;

            // End of code injection

        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_AnimatedParamFunc_getExpression_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AnimatedParamFunc_getIntegrateFromTimeToTime(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AnimatedParamWrapper *>(reinterpret_cast< ::AnimatedParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.AnimatedParam.getIntegrateFromTimeToTime";
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
        goto Sbk_AnimatedParamFunc_getIntegrateFromTimeToTime_TypeError;
    } else if (numArgs < 2) {
        static PyObject *const too_few = Shiboken::String::createStaticString("<");
        errInfo = too_few;
        Py_INCREF(errInfo);
        goto Sbk_AnimatedParamFunc_getIntegrateFromTimeToTime_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|OOO:getIntegrateFromTimeToTime", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return {};


    // Overloaded function decisor
    // 0: AnimatedParam::getIntegrateFromTimeToTime(double,double,int)const
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
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_dimension = Shiboken::String::createStaticString("dimension");
            if (PyDict_Contains(kwds, key_dimension)) {
                value = PyDict_GetItem(kwds, key_dimension);
                if (value && pyArgs[2]) {
                    errInfo = key_dimension;
                    Py_INCREF(errInfo);
                    goto Sbk_AnimatedParamFunc_getIntegrateFromTimeToTime_TypeError;
                }
                if (value) {
                    pyArgs[2] = value;
                    if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2]))))
                        goto Sbk_AnimatedParamFunc_getIntegrateFromTimeToTime_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_AnimatedParamFunc_getIntegrateFromTimeToTime_TypeError;
            } else {
                Py_DECREF(kwds_dup);
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
            double cppResult = const_cast<const ::AnimatedParamWrapper *>(cppSelf)->getIntegrateFromTimeToTime(cppArg0, cppArg1, cppArg2);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_AnimatedParamFunc_getIntegrateFromTimeToTime_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AnimatedParamFunc_getIsAnimated(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AnimatedParamWrapper *>(reinterpret_cast< ::AnimatedParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.AnimatedParam.getIsAnimated";
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
        goto Sbk_AnimatedParamFunc_getIsAnimated_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|O:getIsAnimated", &(pyArgs[0])))
        return {};


    // Overloaded function decisor
    // 0: AnimatedParam::getIsAnimated(int)const
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
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_dimension = Shiboken::String::createStaticString("dimension");
            if (PyDict_Contains(kwds, key_dimension)) {
                value = PyDict_GetItem(kwds, key_dimension);
                if (value && pyArgs[0]) {
                    errInfo = key_dimension;
                    Py_INCREF(errInfo);
                    goto Sbk_AnimatedParamFunc_getIsAnimated_TypeError;
                }
                if (value) {
                    pyArgs[0] = value;
                    if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0]))))
                        goto Sbk_AnimatedParamFunc_getIsAnimated_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_AnimatedParamFunc_getIsAnimated_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        int cppArg0 = 0;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // getIsAnimated(int)const
            bool cppResult = const_cast<const ::AnimatedParamWrapper *>(cppSelf)->getIsAnimated(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_AnimatedParamFunc_getIsAnimated_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AnimatedParamFunc_getKeyIndex(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AnimatedParamWrapper *>(reinterpret_cast< ::AnimatedParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.AnimatedParam.getKeyIndex";
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
        goto Sbk_AnimatedParamFunc_getKeyIndex_TypeError;
    } else if (numArgs < 1) {
        static PyObject *const too_few = Shiboken::String::createStaticString("<");
        errInfo = too_few;
        Py_INCREF(errInfo);
        goto Sbk_AnimatedParamFunc_getKeyIndex_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|OO:getKeyIndex", &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: AnimatedParam::getKeyIndex(double,int)const
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // getKeyIndex(double,int)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            overloadId = 0; // getKeyIndex(double,int)const
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AnimatedParamFunc_getKeyIndex_TypeError;

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
                    goto Sbk_AnimatedParamFunc_getKeyIndex_TypeError;
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_AnimatedParamFunc_getKeyIndex_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_AnimatedParamFunc_getKeyIndex_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getKeyIndex(double,int)const
            int cppResult = const_cast<const ::AnimatedParamWrapper *>(cppSelf)->getKeyIndex(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_AnimatedParamFunc_getKeyIndex_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AnimatedParamFunc_getKeyTime(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AnimatedParamWrapper *>(reinterpret_cast< ::AnimatedParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.AnimatedParam.getKeyTime";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "getKeyTime", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: AnimatedParam::getKeyTime(int,int,double*)const
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
        return {};
    }
    return pyResult;

    Sbk_AnimatedParamFunc_getKeyTime_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AnimatedParamFunc_getNumKeys(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AnimatedParamWrapper *>(reinterpret_cast< ::AnimatedParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.AnimatedParam.getNumKeys";
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
        goto Sbk_AnimatedParamFunc_getNumKeys_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|O:getNumKeys", &(pyArgs[0])))
        return {};


    // Overloaded function decisor
    // 0: AnimatedParam::getNumKeys(int)const
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
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_dimension = Shiboken::String::createStaticString("dimension");
            if (PyDict_Contains(kwds, key_dimension)) {
                value = PyDict_GetItem(kwds, key_dimension);
                if (value && pyArgs[0]) {
                    errInfo = key_dimension;
                    Py_INCREF(errInfo);
                    goto Sbk_AnimatedParamFunc_getNumKeys_TypeError;
                }
                if (value) {
                    pyArgs[0] = value;
                    if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0]))))
                        goto Sbk_AnimatedParamFunc_getNumKeys_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_AnimatedParamFunc_getNumKeys_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        int cppArg0 = 0;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // getNumKeys(int)const
            int cppResult = const_cast<const ::AnimatedParamWrapper *>(cppSelf)->getNumKeys(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_AnimatedParamFunc_getNumKeys_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AnimatedParamFunc_removeAnimation(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AnimatedParamWrapper *>(reinterpret_cast< ::AnimatedParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.AnimatedParam.removeAnimation";
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
        goto Sbk_AnimatedParamFunc_removeAnimation_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|O:removeAnimation", &(pyArgs[0])))
        return {};


    // Overloaded function decisor
    // 0: AnimatedParam::removeAnimation(int)
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
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_dimension = Shiboken::String::createStaticString("dimension");
            if (PyDict_Contains(kwds, key_dimension)) {
                value = PyDict_GetItem(kwds, key_dimension);
                if (value && pyArgs[0]) {
                    errInfo = key_dimension;
                    Py_INCREF(errInfo);
                    goto Sbk_AnimatedParamFunc_removeAnimation_TypeError;
                }
                if (value) {
                    pyArgs[0] = value;
                    if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0]))))
                        goto Sbk_AnimatedParamFunc_removeAnimation_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_AnimatedParamFunc_removeAnimation_TypeError;
            } else {
                Py_DECREF(kwds_dup);
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
        return {};
    }
    Py_RETURN_NONE;

    Sbk_AnimatedParamFunc_removeAnimation_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AnimatedParamFunc_setExpression(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AnimatedParamWrapper *>(reinterpret_cast< ::AnimatedParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.AnimatedParam.setExpression";
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
        goto Sbk_AnimatedParamFunc_setExpression_TypeError;
    } else if (numArgs < 2) {
        static PyObject *const too_few = Shiboken::String::createStaticString("<");
        errInfo = too_few;
        Py_INCREF(errInfo);
        goto Sbk_AnimatedParamFunc_setExpression_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|OOO:setExpression", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return {};


    // Overloaded function decisor
    // 0: AnimatedParam::setExpression(QString,bool,int)
    if (numArgs >= 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[1])))) {
        if (numArgs == 2) {
            overloadId = 0; // setExpression(QString,bool,int)
        } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
            overloadId = 0; // setExpression(QString,bool,int)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AnimatedParamFunc_setExpression_TypeError;

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
                    goto Sbk_AnimatedParamFunc_setExpression_TypeError;
                }
                if (value) {
                    pyArgs[2] = value;
                    if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2]))))
                        goto Sbk_AnimatedParamFunc_setExpression_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_AnimatedParamFunc_setExpression_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        ::QString cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        bool cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        int cppArg2 = 0;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // setExpression(QString,bool,int)
            // Begin code injection
            bool cppResult = cppSelf->setExpression(cppArg0,cppArg1,cppArg2);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);

            // End of code injection

        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_AnimatedParamFunc_setExpression_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AnimatedParamFunc_setInterpolationAtTime(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AnimatedParamWrapper *>(reinterpret_cast< ::AnimatedParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.AnimatedParam.setInterpolationAtTime";
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
        goto Sbk_AnimatedParamFunc_setInterpolationAtTime_TypeError;
    } else if (numArgs < 2) {
        static PyObject *const too_few = Shiboken::String::createStaticString("<");
        errInfo = too_few;
        Py_INCREF(errInfo);
        goto Sbk_AnimatedParamFunc_setInterpolationAtTime_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|OOO:setInterpolationAtTime", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return {};


    // Overloaded function decisor
    // 0: AnimatedParam::setInterpolationAtTime(double,NATRON_ENUM::KeyframeTypeEnum,int)
    if (numArgs >= 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(*PepType_SGTP(SbkNatronEngineTypes[SBK_NATRON_ENUM_KEYFRAMETYPEENUM_IDX])->converter, (pyArgs[1])))) {
        if (numArgs == 2) {
            overloadId = 0; // setInterpolationAtTime(double,NATRON_ENUM::KeyframeTypeEnum,int)
        } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
            overloadId = 0; // setInterpolationAtTime(double,NATRON_ENUM::KeyframeTypeEnum,int)
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AnimatedParamFunc_setInterpolationAtTime_TypeError;

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
                    goto Sbk_AnimatedParamFunc_setInterpolationAtTime_TypeError;
                }
                if (value) {
                    pyArgs[2] = value;
                    if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2]))))
                        goto Sbk_AnimatedParamFunc_setInterpolationAtTime_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_dimension);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_AnimatedParamFunc_setInterpolationAtTime_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::NATRON_ENUM::KeyframeTypeEnum cppArg1{NATRON_ENUM::eKeyframeTypeConstant};
        pythonToCpp[1](pyArgs[1], &cppArg1);
        int cppArg2 = 0;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // setInterpolationAtTime(double,NATRON_ENUM::KeyframeTypeEnum,int)
            bool cppResult = cppSelf->setInterpolationAtTime(cppArg0, cppArg1, cppArg2);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_AnimatedParamFunc_setInterpolationAtTime_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}


static const char *Sbk_AnimatedParam_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_AnimatedParam_methods[] = {
    {"deleteValueAtTime", reinterpret_cast<PyCFunction>(Sbk_AnimatedParamFunc_deleteValueAtTime), METH_VARARGS|METH_KEYWORDS},
    {"getCurrentTime", reinterpret_cast<PyCFunction>(Sbk_AnimatedParamFunc_getCurrentTime), METH_NOARGS},
    {"getDerivativeAtTime", reinterpret_cast<PyCFunction>(Sbk_AnimatedParamFunc_getDerivativeAtTime), METH_VARARGS|METH_KEYWORDS},
    {"getExpression", reinterpret_cast<PyCFunction>(Sbk_AnimatedParamFunc_getExpression), METH_O},
    {"getIntegrateFromTimeToTime", reinterpret_cast<PyCFunction>(Sbk_AnimatedParamFunc_getIntegrateFromTimeToTime), METH_VARARGS|METH_KEYWORDS},
    {"getIsAnimated", reinterpret_cast<PyCFunction>(Sbk_AnimatedParamFunc_getIsAnimated), METH_VARARGS|METH_KEYWORDS},
    {"getKeyIndex", reinterpret_cast<PyCFunction>(Sbk_AnimatedParamFunc_getKeyIndex), METH_VARARGS|METH_KEYWORDS},
    {"getKeyTime", reinterpret_cast<PyCFunction>(Sbk_AnimatedParamFunc_getKeyTime), METH_VARARGS},
    {"getNumKeys", reinterpret_cast<PyCFunction>(Sbk_AnimatedParamFunc_getNumKeys), METH_VARARGS|METH_KEYWORDS},
    {"removeAnimation", reinterpret_cast<PyCFunction>(Sbk_AnimatedParamFunc_removeAnimation), METH_VARARGS|METH_KEYWORDS},
    {"setExpression", reinterpret_cast<PyCFunction>(Sbk_AnimatedParamFunc_setExpression), METH_VARARGS|METH_KEYWORDS},
    {"setInterpolationAtTime", reinterpret_cast<PyCFunction>(Sbk_AnimatedParamFunc_setInterpolationAtTime), METH_VARARGS|METH_KEYWORDS},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_AnimatedParam_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::AnimatedParam *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<AnimatedParamWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_AnimatedParam_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_AnimatedParam_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_AnimatedParam_Type = nullptr;
static SbkObjectType *Sbk_AnimatedParam_TypeF(void)
{
    return _Sbk_AnimatedParam_Type;
}

static PyType_Slot Sbk_AnimatedParam_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_AnimatedParam_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_AnimatedParam_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_AnimatedParam_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_AnimatedParam_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_AnimatedParam_spec = {
    "1:NatronEngine.AnimatedParam",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_AnimatedParam_slots
};

} //extern "C"

static void *Sbk_AnimatedParam_typeDiscovery(void *cptr, SbkObjectType *instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::Param >()))
        return dynamic_cast< ::AnimatedParam *>(reinterpret_cast< ::Param *>(cptr));
    return {};
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void AnimatedParam_PythonToCpp_AnimatedParam_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_AnimatedParam_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_AnimatedParam_PythonToCpp_AnimatedParam_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_AnimatedParam_TypeF())))
        return AnimatedParam_PythonToCpp_AnimatedParam_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *AnimatedParam_PTR_CppToPython_AnimatedParam(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::AnimatedParam *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_AnimatedParam_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *AnimatedParam_SignatureStrings[] = {
    "NatronEngine.AnimatedParam.deleteValueAtTime(self,time:double,dimension:int=0)",
    "NatronEngine.AnimatedParam.getCurrentTime(self)->int",
    "NatronEngine.AnimatedParam.getDerivativeAtTime(self,time:double,dimension:int=0)->double",
    "NatronEngine.AnimatedParam.getExpression(self,dimension:int,hasRetVariable:bool*)->QString",
    "NatronEngine.AnimatedParam.getIntegrateFromTimeToTime(self,time1:double,time2:double,dimension:int=0)->double",
    "NatronEngine.AnimatedParam.getIsAnimated(self,dimension:int=0)->bool",
    "NatronEngine.AnimatedParam.getKeyIndex(self,time:double,dimension:int=0)->int",
    "NatronEngine.AnimatedParam.getKeyTime(self,index:int,dimension:int,time:double*)->bool",
    "NatronEngine.AnimatedParam.getNumKeys(self,dimension:int=0)->int",
    "NatronEngine.AnimatedParam.removeAnimation(self,dimension:int=0)",
    "NatronEngine.AnimatedParam.setExpression(self,expr:QString,hasRetVariable:bool,dimension:int=0)->bool",
    "NatronEngine.AnimatedParam.setInterpolationAtTime(self,time:double,interpolation:NatronEngine.NATRON_ENUM.KeyframeTypeEnum,dimension:int=0)->bool",
    nullptr}; // Sentinel

void init_AnimatedParam(PyObject *module)
{
    _Sbk_AnimatedParam_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "AnimatedParam",
        "AnimatedParam*",
        &Sbk_AnimatedParam_spec,
        &Shiboken::callCppDestructor< ::AnimatedParam >,
        reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PARAM_IDX]),
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_AnimatedParam_Type);
    InitSignatureStrings(pyType, AnimatedParam_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_AnimatedParam_Type), Sbk_AnimatedParam_PropertyStrings);
    SbkNatronEngineTypes[SBK_ANIMATEDPARAM_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_AnimatedParam_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_AnimatedParam_TypeF(),
        AnimatedParam_PythonToCpp_AnimatedParam_PTR,
        is_AnimatedParam_PythonToCpp_AnimatedParam_PTR_Convertible,
        AnimatedParam_PTR_CppToPython_AnimatedParam);

    Shiboken::Conversions::registerConverterName(converter, "AnimatedParam");
    Shiboken::Conversions::registerConverterName(converter, "AnimatedParam*");
    Shiboken::Conversions::registerConverterName(converter, "AnimatedParam&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::AnimatedParam).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::AnimatedParamWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(Sbk_AnimatedParam_TypeF(), &Sbk_AnimatedParam_typeDiscovery);

    AnimatedParamWrapper::pysideInitQtMetaTypes();
}
