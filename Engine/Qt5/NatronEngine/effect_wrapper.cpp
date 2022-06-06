
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
#include <algorithm>
#include <set>
#include <iterator>

// module include
#include "natronengine_python.h"

// main header
#include "effect_wrapper.h"

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

void EffectWrapper::pysideInitQtMetaTypes()
{
}

void EffectWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

EffectWrapper::~EffectWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject *Sbk_EffectFunc_addUserPlane(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.addUserPlane";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "addUserPlane", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: Effect::addUserPlane(QString,QStringList)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRINGLIST_IDX], (pyArgs[1])))) {
        overloadId = 0; // addUserPlane(QString,QStringList)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_addUserPlane_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QStringList cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // addUserPlane(QString,QStringList)
            bool cppResult = cppSelf->addUserPlane(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_EffectFunc_addUserPlane_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_EffectFunc_beginChanges(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.beginChanges";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // beginChanges()
            cppSelf->beginChanges();
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;
}

static PyObject *Sbk_EffectFunc_canConnectInput(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.canConnectInput";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "canConnectInput", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: Effect::canConnectInput(int,const Effect*)const
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_EFFECT_IDX]), (pyArgs[1])))) {
        overloadId = 0; // canConnectInput(int,const Effect*)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_canConnectInput_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return {};
        ::Effect *cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // canConnectInput(int,const Effect*)const
            bool cppResult = const_cast<const ::Effect *>(cppSelf)->canConnectInput(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_EffectFunc_canConnectInput_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_EffectFunc_connectInput(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.connectInput";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "connectInput", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: Effect::connectInput(int,const Effect*)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_EFFECT_IDX]), (pyArgs[1])))) {
        overloadId = 0; // connectInput(int,const Effect*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_connectInput_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return {};
        ::Effect *cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // connectInput(int,const Effect*)
            // Begin code injection
            bool cppResult = cppSelf->connectInput(cppArg0,cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);

            // End of code injection

        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_EffectFunc_connectInput_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_EffectFunc_destroy(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.destroy";
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
        goto Sbk_EffectFunc_destroy_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|O:destroy", &(pyArgs[0])))
        return {};


    // Overloaded function decisor
    // 0: Effect::destroy(bool)
    if (numArgs == 0) {
        overloadId = 0; // destroy(bool)
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[0])))) {
        overloadId = 0; // destroy(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_destroy_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_autoReconnect = Shiboken::String::createStaticString("autoReconnect");
            if (PyDict_Contains(kwds, key_autoReconnect)) {
                value = PyDict_GetItem(kwds, key_autoReconnect);
                if (value && pyArgs[0]) {
                    errInfo = key_autoReconnect;
                    Py_INCREF(errInfo);
                    goto Sbk_EffectFunc_destroy_TypeError;
                }
                if (value) {
                    pyArgs[0] = value;
                    if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[0]))))
                        goto Sbk_EffectFunc_destroy_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_autoReconnect);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_EffectFunc_destroy_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        bool cppArg0 = true;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // destroy(bool)
            // Begin code injection
            cppSelf->destroy(cppArg0);

            // End of code injection

        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_EffectFunc_destroy_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_EffectFunc_disconnectInput(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.disconnectInput";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Effect::disconnectInput(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // disconnectInput(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_disconnectInput_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // disconnectInput(int)
            // Begin code injection
            cppSelf->disconnectInput(cppArg0);

            // End of code injection

        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_EffectFunc_disconnectInput_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_EffectFunc_endChanges(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.endChanges";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // endChanges()
            cppSelf->endChanges();
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;
}

static PyObject *Sbk_EffectFunc_getAvailableLayers(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getAvailableLayers";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Effect::getAvailableLayers(int)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // getAvailableLayers(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_getAvailableLayers_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getAvailableLayers(int)const
            std::list<ImageLayer > cppResult = const_cast<const ::Effect *>(cppSelf)->getAvailableLayers(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_IMAGELAYER_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_EffectFunc_getAvailableLayers_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_EffectFunc_getBitDepth(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getBitDepth";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getBitDepth()const
            NATRON_ENUM::ImageBitDepthEnum cppResult = NATRON_ENUM::ImageBitDepthEnum(const_cast<const ::Effect *>(cppSelf)->getBitDepth());
            pyResult = Shiboken::Conversions::copyToPython(*PepType_SGTP(SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEBITDEPTHENUM_IDX])->converter, &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_EffectFunc_getColor(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getColor";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getColor(double*,double*,double*)const
            // Begin code injection
            double r,g,b;
            cppSelf->getColor(&r,&g,&b);
            pyResult = PyTuple_New(3);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &r));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &g));
            PyTuple_SET_ITEM(pyResult, 2, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &b));
            return pyResult;

            // End of code injection

        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_EffectFunc_getCurrentTime(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getCurrentTime";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getCurrentTime()const
            int cppResult = const_cast<const ::Effect *>(cppSelf)->getCurrentTime();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_EffectFunc_getFrameRate(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getFrameRate";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getFrameRate()const
            double cppResult = const_cast<const ::Effect *>(cppSelf)->getFrameRate();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_EffectFunc_getInput(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getInput";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Effect::getInput(QString)const
    // 1: Effect::getInput(int)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 1; // getInput(int)const
    } else if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // getInput(QString)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_getInput_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // getInput(const QString & inputLabel) const
        {
            ::QString cppArg0;
            pythonToCpp(pyArg, &cppArg0);

            if (!PyErr_Occurred()) {
                // getInput(QString)const
                Effect * cppResult = const_cast<const ::Effect *>(cppSelf)->getInput(cppArg0);
                pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_EFFECT_IDX]), cppResult);

                // Ownership transferences.
                Shiboken::Object::getOwnership(pyResult);
            }
            break;
        }
        case 1: // getInput(int inputNumber) const
        {
            int cppArg0;
            pythonToCpp(pyArg, &cppArg0);

            if (!PyErr_Occurred()) {
                // getInput(int)const
                Effect * cppResult = const_cast<const ::Effect *>(cppSelf)->getInput(cppArg0);
                pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_EFFECT_IDX]), cppResult);

                // Ownership transferences.
                Shiboken::Object::getOwnership(pyResult);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_EffectFunc_getInput_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_EffectFunc_getInputLabel(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getInputLabel";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Effect::getInputLabel(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // getInputLabel(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_getInputLabel_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getInputLabel(int)
            QString cppResult = cppSelf->getInputLabel(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_EffectFunc_getInputLabel_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_EffectFunc_getLabel(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getLabel";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getLabel()const
            QString cppResult = const_cast<const ::Effect *>(cppSelf)->getLabel();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_EffectFunc_getMaxInputCount(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getMaxInputCount";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getMaxInputCount()const
            int cppResult = const_cast<const ::Effect *>(cppSelf)->getMaxInputCount();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_EffectFunc_getOutputFormat(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getOutputFormat";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getOutputFormat()const
            RectI* cppResult = new RectI(const_cast<const ::Effect *>(cppSelf)->getOutputFormat());
            pyResult = Shiboken::Object::newObject(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTI_IDX]), cppResult, true, true);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_EffectFunc_getParam(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getParam";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Effect::getParam(QString)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // getParam(QString)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_getParam_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getParam(QString)const
            Param * cppResult = const_cast<const ::Effect *>(cppSelf)->getParam(cppArg0);
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

    Sbk_EffectFunc_getParam_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_EffectFunc_getParams(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getParams";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getParams()const
            // Begin code injection
            std::list<Param*> params = cppSelf->getParams();
            PyObject* ret = PyList_New((int) params.size());
            int idx = 0;
            for (std::list<Param*>::iterator it = params.begin(); it!=params.end(); ++it,++idx) {
                PyObject* item = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PARAM_IDX]), *it);
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
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_EffectFunc_getPixelAspectRatio(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getPixelAspectRatio";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getPixelAspectRatio()const
            double cppResult = const_cast<const ::Effect *>(cppSelf)->getPixelAspectRatio();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_EffectFunc_getPluginID(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getPluginID";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getPluginID()const
            QString cppResult = const_cast<const ::Effect *>(cppSelf)->getPluginID();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_EffectFunc_getPosition(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getPosition";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getPosition(double*,double*)const
            // Begin code injection
            double x,y;
            cppSelf->getPosition(&x,&y);
            pyResult = PyTuple_New(2);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &x));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &y));
            return pyResult;

            // End of code injection

        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_EffectFunc_getPremult(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getPremult";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getPremult()const
            NATRON_ENUM::ImagePremultiplicationEnum cppResult = NATRON_ENUM::ImagePremultiplicationEnum(const_cast<const ::Effect *>(cppSelf)->getPremult());
            pyResult = Shiboken::Conversions::copyToPython(*PepType_SGTP(SbkNatronEngineTypes[SBK_NATRON_ENUM_IMAGEPREMULTIPLICATIONENUM_IDX])->converter, &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_EffectFunc_getRegionOfDefinition(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getRegionOfDefinition";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "getRegionOfDefinition", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: Effect::getRegionOfDefinition(double,int)const
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
        overloadId = 0; // getRegionOfDefinition(double,int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_getRegionOfDefinition_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getRegionOfDefinition(double,int)const
            RectD* cppResult = new RectD(const_cast<const ::Effect *>(cppSelf)->getRegionOfDefinition(cppArg0, cppArg1));
            pyResult = Shiboken::Object::newObject(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTD_IDX]), cppResult, true, true);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_EffectFunc_getRegionOfDefinition_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_EffectFunc_getRotoContext(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getRotoContext";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getRotoContext()const
            Roto * cppResult = const_cast<const ::Effect *>(cppSelf)->getRotoContext();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_ROTO_IDX]), cppResult);

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

static PyObject *Sbk_EffectFunc_getScriptName(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getScriptName";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getScriptName()const
            QString cppResult = const_cast<const ::Effect *>(cppSelf)->getScriptName();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_EffectFunc_getSize(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getSize";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getSize(double*,double*)const
            // Begin code injection
            double x,y;
            cppSelf->getSize(&x,&y);
            pyResult = PyTuple_New(2);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &x));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &y));
            return pyResult;

            // End of code injection

        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_EffectFunc_getTrackerContext(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getTrackerContext";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getTrackerContext()const
            Tracker * cppResult = const_cast<const ::Effect *>(cppSelf)->getTrackerContext();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_TRACKER_IDX]), cppResult);

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

static PyObject *Sbk_EffectFunc_getUserPageParam(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.getUserPageParam";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getUserPageParam()const
            PageParam * cppResult = const_cast<const ::Effect *>(cppSelf)->getUserPageParam();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_PAGEPARAM_IDX]), cppResult);

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

static PyObject *Sbk_EffectFunc_isNodeSelected(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.isNodeSelected";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isNodeSelected()const
            bool cppResult = const_cast<const ::Effect *>(cppSelf)->isNodeSelected();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_EffectFunc_isOutputNode(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.isOutputNode";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isOutputNode()
            bool cppResult = cppSelf->isOutputNode();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_EffectFunc_isReaderNode(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.isReaderNode";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isReaderNode()
            bool cppResult = cppSelf->isReaderNode();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_EffectFunc_isWriterNode(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.isWriterNode";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isWriterNode()
            bool cppResult = cppSelf->isWriterNode();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_EffectFunc_setColor(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.setColor";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setColor", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return {};


    // Overloaded function decisor
    // 0: Effect::setColor(double,double,double)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))) {
        overloadId = 0; // setColor(double,double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_setColor_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        double cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // setColor(double,double,double)
            cppSelf->setColor(cppArg0, cppArg1, cppArg2);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_EffectFunc_setColor_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_EffectFunc_setLabel(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.setLabel";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Effect::setLabel(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // setLabel(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_setLabel_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setLabel(QString)
            // Begin code injection
            cppSelf->setLabel(cppArg0);

            // End of code injection

        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_EffectFunc_setLabel_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_EffectFunc_setPagesOrder(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.setPagesOrder";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Effect::setPagesOrder(QStringList)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRINGLIST_IDX], (pyArg)))) {
        overloadId = 0; // setPagesOrder(QStringList)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_setPagesOrder_TypeError;

    // Call function/method
    {
        ::QStringList cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setPagesOrder(QStringList)
            cppSelf->setPagesOrder(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_EffectFunc_setPagesOrder_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_EffectFunc_setPosition(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.setPosition";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setPosition", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: Effect::setPosition(double,double)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        overloadId = 0; // setPosition(double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_setPosition_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setPosition(double,double)
            cppSelf->setPosition(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_EffectFunc_setPosition_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_EffectFunc_setScriptName(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.setScriptName";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Effect::setScriptName(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // setScriptName(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_setScriptName_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setScriptName(QString)
            // Begin code injection
            bool cppResult = cppSelf->setScriptName(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);

            // End of code injection

        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_EffectFunc_setScriptName_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_EffectFunc_setSize(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.setSize";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setSize", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: Effect::setSize(double,double)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        overloadId = 0; // setSize(double,double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_setSize_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        double cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setSize(double,double)
            cppSelf->setSize(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_EffectFunc_setSize_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_EffectFunc_setSubGraphEditable(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.Effect.setSubGraphEditable";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: Effect::setSubGraphEditable(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setSubGraphEditable(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_EffectFunc_setSubGraphEditable_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setSubGraphEditable(bool)
            cppSelf->setSubGraphEditable(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_EffectFunc_setSubGraphEditable_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}


static const char *Sbk_Effect_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_Effect_methods[] = {
    {"addUserPlane", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_addUserPlane), METH_VARARGS},
    {"beginChanges", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_beginChanges), METH_NOARGS},
    {"canConnectInput", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_canConnectInput), METH_VARARGS},
    {"connectInput", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_connectInput), METH_VARARGS},
    {"destroy", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_destroy), METH_VARARGS|METH_KEYWORDS},
    {"disconnectInput", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_disconnectInput), METH_O},
    {"endChanges", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_endChanges), METH_NOARGS},
    {"getAvailableLayers", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getAvailableLayers), METH_O},
    {"getBitDepth", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getBitDepth), METH_NOARGS},
    {"getColor", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getColor), METH_NOARGS},
    {"getCurrentTime", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getCurrentTime), METH_NOARGS},
    {"getFrameRate", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getFrameRate), METH_NOARGS},
    {"getInput", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getInput), METH_O},
    {"getInputLabel", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getInputLabel), METH_O},
    {"getLabel", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getLabel), METH_NOARGS},
    {"getMaxInputCount", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getMaxInputCount), METH_NOARGS},
    {"getOutputFormat", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getOutputFormat), METH_NOARGS},
    {"getParam", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getParam), METH_O},
    {"getParams", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getParams), METH_NOARGS},
    {"getPixelAspectRatio", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getPixelAspectRatio), METH_NOARGS},
    {"getPluginID", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getPluginID), METH_NOARGS},
    {"getPosition", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getPosition), METH_NOARGS},
    {"getPremult", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getPremult), METH_NOARGS},
    {"getRegionOfDefinition", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getRegionOfDefinition), METH_VARARGS},
    {"getRotoContext", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getRotoContext), METH_NOARGS},
    {"getScriptName", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getScriptName), METH_NOARGS},
    {"getSize", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getSize), METH_NOARGS},
    {"getTrackerContext", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getTrackerContext), METH_NOARGS},
    {"getUserPageParam", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_getUserPageParam), METH_NOARGS},
    {"isNodeSelected", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_isNodeSelected), METH_NOARGS},
    {"isOutputNode", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_isOutputNode), METH_NOARGS},
    {"isReaderNode", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_isReaderNode), METH_NOARGS},
    {"isWriterNode", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_isWriterNode), METH_NOARGS},
    {"setColor", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_setColor), METH_VARARGS},
    {"setLabel", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_setLabel), METH_O},
    {"setPagesOrder", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_setPagesOrder), METH_O},
    {"setPosition", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_setPosition), METH_VARARGS},
    {"setScriptName", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_setScriptName), METH_O},
    {"setSize", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_setSize), METH_VARARGS},
    {"setSubGraphEditable", reinterpret_cast<PyCFunction>(Sbk_EffectFunc_setSubGraphEditable), METH_O},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_Effect_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::Effect *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_EFFECT_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<EffectWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_Effect_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_Effect_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
static int mi_offsets[] = { -1, -1, -1, -1, -1 };
int *
Sbk_Effect_mi_init(const void *cptr)
{
    if (mi_offsets[0] == -1) {
        std::set<int> offsets;
        const auto *class_ptr = reinterpret_cast<const Effect *>(cptr);
        const auto base = reinterpret_cast<uintptr_t>(class_ptr);
        offsets.insert(int(reinterpret_cast<uintptr_t>(static_cast<const Group *>(class_ptr)) - base));
        offsets.insert(int(reinterpret_cast<uintptr_t>(static_cast<const Group *>(static_cast<const Effect *>(static_cast<const void *>(class_ptr)))) - base));
        offsets.insert(int(reinterpret_cast<uintptr_t>(static_cast<const UserParamHolder *>(class_ptr)) - base));
        offsets.insert(int(reinterpret_cast<uintptr_t>(static_cast<const UserParamHolder *>(static_cast<const Effect *>(static_cast<const void *>(class_ptr)))) - base));

        offsets.erase(0);

        std::copy(offsets.cbegin(), offsets.cend(), mi_offsets);
    }
    return mi_offsets;
}
static void * Sbk_EffectSpecialCastFunction(void *obj, SbkObjectType *desiredType)
{
    auto me = reinterpret_cast< ::Effect *>(obj);
    if (desiredType == reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_GROUP_IDX]))
        return static_cast< ::Group *>(me);
    else if (desiredType == reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_USERPARAMHOLDER_IDX]))
        return static_cast< ::UserParamHolder *>(me);
    return me;
}


// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_Effect_Type = nullptr;
static SbkObjectType *Sbk_Effect_TypeF(void)
{
    return _Sbk_Effect_Type;
}

static PyType_Slot Sbk_Effect_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_Effect_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_Effect_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_Effect_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_Effect_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_Effect_spec = {
    "1:NatronEngine.Effect",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_Effect_slots
};

} //extern "C"

static void *Sbk_Effect_typeDiscovery(void *cptr, SbkObjectType *instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::Group >()))
        return dynamic_cast< ::Effect *>(reinterpret_cast< ::Group *>(cptr));
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::UserParamHolder >()))
        return dynamic_cast< ::Effect *>(reinterpret_cast< ::UserParamHolder *>(cptr));
    return {};
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void Effect_PythonToCpp_Effect_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_Effect_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_Effect_PythonToCpp_Effect_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_Effect_TypeF())))
        return Effect_PythonToCpp_Effect_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *Effect_PTR_CppToPython_Effect(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::Effect *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_Effect_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *Effect_SignatureStrings[] = {
    "NatronEngine.Effect.addUserPlane(self,planeName:QString,channels:QStringList)->bool",
    "NatronEngine.Effect.beginChanges(self)",
    "NatronEngine.Effect.canConnectInput(self,inputNumber:int,node:NatronEngine.Effect)->bool",
    "NatronEngine.Effect.connectInput(self,inputNumber:int,input:NatronEngine.Effect)->bool",
    "NatronEngine.Effect.destroy(self,autoReconnect:bool=true)",
    "NatronEngine.Effect.disconnectInput(self,inputNumber:int)",
    "NatronEngine.Effect.endChanges(self)",
    "NatronEngine.Effect.getAvailableLayers(self,inputNb:int)->std.list[NatronEngine.ImageLayer]",
    "NatronEngine.Effect.getBitDepth(self)->NatronEngine.NATRON_ENUM.ImageBitDepthEnum",
    "NatronEngine.Effect.getColor(self,r:double*,g:double*,b:double*)",
    "NatronEngine.Effect.getCurrentTime(self)->int",
    "NatronEngine.Effect.getFrameRate(self)->double",
    "1:NatronEngine.Effect.getInput(self,inputLabel:QString)->NatronEngine.Effect",
    "0:NatronEngine.Effect.getInput(self,inputNumber:int)->NatronEngine.Effect",
    "NatronEngine.Effect.getInputLabel(self,inputNumber:int)->QString",
    "NatronEngine.Effect.getLabel(self)->QString",
    "NatronEngine.Effect.getMaxInputCount(self)->int",
    "NatronEngine.Effect.getOutputFormat(self)->NatronEngine.RectI",
    "NatronEngine.Effect.getParam(self,name:QString)->NatronEngine.Param",
    "NatronEngine.Effect.getParams(self)->std.list[NatronEngine.Param]",
    "NatronEngine.Effect.getPixelAspectRatio(self)->double",
    "NatronEngine.Effect.getPluginID(self)->QString",
    "NatronEngine.Effect.getPosition(self,x:double*,y:double*)",
    "NatronEngine.Effect.getPremult(self)->NatronEngine.NATRON_ENUM.ImagePremultiplicationEnum",
    "NatronEngine.Effect.getRegionOfDefinition(self,time:double,view:int)->NatronEngine.RectD",
    "NatronEngine.Effect.getRotoContext(self)->NatronEngine.Roto",
    "NatronEngine.Effect.getScriptName(self)->QString",
    "NatronEngine.Effect.getSize(self,w:double*,h:double*)",
    "NatronEngine.Effect.getTrackerContext(self)->NatronEngine.Tracker",
    "NatronEngine.Effect.getUserPageParam(self)->NatronEngine.PageParam",
    "NatronEngine.Effect.isNodeSelected(self)->bool",
    "NatronEngine.Effect.isOutputNode(self)->bool",
    "NatronEngine.Effect.isReaderNode(self)->bool",
    "NatronEngine.Effect.isWriterNode(self)->bool",
    "NatronEngine.Effect.setColor(self,r:double,g:double,b:double)",
    "NatronEngine.Effect.setLabel(self,name:QString)",
    "NatronEngine.Effect.setPagesOrder(self,pages:QStringList)",
    "NatronEngine.Effect.setPosition(self,x:double,y:double)",
    "NatronEngine.Effect.setScriptName(self,scriptName:QString)->bool",
    "NatronEngine.Effect.setSize(self,w:double,h:double)",
    "NatronEngine.Effect.setSubGraphEditable(self,editable:bool)",
    nullptr}; // Sentinel

void init_Effect(PyObject *module)
{
    PyObject *Sbk_Effect_Type_bases = PyTuple_Pack(2,
        reinterpret_cast<PyObject *>(SbkNatronEngineTypes[SBK_GROUP_IDX]),
        reinterpret_cast<PyObject *>(SbkNatronEngineTypes[SBK_USERPARAMHOLDER_IDX]));

    _Sbk_Effect_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "Effect",
        "Effect*",
        &Sbk_Effect_spec,
        &Shiboken::callCppDestructor< ::Effect >,
        reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_GROUP_IDX]),
        Sbk_Effect_Type_bases,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_Effect_Type);
    InitSignatureStrings(pyType, Effect_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_Effect_Type), Sbk_Effect_PropertyStrings);
    SbkNatronEngineTypes[SBK_EFFECT_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_Effect_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_Effect_TypeF(),
        Effect_PythonToCpp_Effect_PTR,
        is_Effect_PythonToCpp_Effect_PTR_Convertible,
        Effect_PTR_CppToPython_Effect);

    Shiboken::Conversions::registerConverterName(converter, "Effect");
    Shiboken::Conversions::registerConverterName(converter, "Effect*");
    Shiboken::Conversions::registerConverterName(converter, "Effect&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::Effect).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::EffectWrapper).name());


    MultipleInheritanceInitFunction func = Sbk_Effect_mi_init;
    Shiboken::ObjectType::setMultipleInheritanceFunction(Sbk_Effect_TypeF(), func);
    Shiboken::ObjectType::setCastFunction(Sbk_Effect_TypeF(), &Sbk_EffectSpecialCastFunction);
    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(Sbk_Effect_TypeF(), &Sbk_Effect_typeDiscovery);

    EffectWrapper::pysideInitQtMetaTypes();
}
