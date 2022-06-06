
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
#include "rectd_wrapper.h"

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

void RectDWrapper::pysideInitQtMetaTypes()
{
}

void RectDWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

RectDWrapper::RectDWrapper() : RectD()
{
    resetPyMethodCache();
    // ... middle
}

RectDWrapper::RectDWrapper(double l, double b, double r, double t) : RectD(l, b, r, t)
{
    resetPyMethodCache();
    // ... middle
}

RectDWrapper::~RectDWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_RectD_Init(PyObject *self, PyObject *args, PyObject *kwds)
{
    SbkObject *sbkSelf = reinterpret_cast<SbkObject *>(self);
PySide::Feature::Select(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::RectD >()))
        return -1;

    ::RectDWrapper *cptr{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.__init__";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectD_Init_TypeError;

    if (!PyArg_UnpackTuple(args, "RectD", 0, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return -1;


    // Overloaded function decisor
    // 0: RectD::RectD()
    // 1: RectD::RectD(RectD)
    // 2: RectD::RectD(double,double,double,double)
    if (numArgs == 0) {
        overloadId = 0; // RectD()
    } else if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        overloadId = 2; // RectD(double,double,double,double)
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTD_IDX]), (pyArgs[0])))) {
        overloadId = 1; // RectD(RectD)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectD_Init_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // RectD()
        {

            if (!PyErr_Occurred()) {
                // RectD()
                cptr = new ::RectDWrapper();
            }
            break;
        }
        case 1: // RectD(const RectD & b)
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return -1;
            ::RectD *cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // RectD(RectD)
                cptr = new ::RectDWrapper(*cppArg0);
            }
            break;
        }
        case 2: // RectD(double l, double b, double r, double t)
        {
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            double cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);
            double cppArg3;
            pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // RectD(double,double,double,double)
                cptr = new ::RectDWrapper(cppArg0, cppArg1, cppArg2, cppArg3);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::RectD >(), cptr)) {
        delete cptr;
        Py_XDECREF(errInfo);
        return -1;
    }
    if (!cptr) goto Sbk_RectD_Init_TypeError;

    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::Object::setHasCppWrapper(sbkSelf, true);
    if (Shiboken::BindingManager::instance().hasWrapper(cptr)) {
        Shiboken::BindingManager::instance().releaseWrapper(Shiboken::BindingManager::instance().retrieveWrapper(cptr));
    }
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;

    Sbk_RectD_Init_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return -1;
}

static PyObject *Sbk_RectDFunc_area(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.area";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // area()const
            double cppResult = const_cast<const ::RectD *>(cppSelf)->area();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_RectDFunc_bottom(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.bottom";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // bottom()const
            double cppResult = const_cast<const ::RectD *>(cppSelf)->bottom();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_RectDFunc_clear(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.clear";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // clear()
            cppSelf->clear();
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;
}

static PyObject *Sbk_RectDFunc_contains(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.contains";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "contains", 1, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: RectD::contains(RectD)const
    // 1: RectD::contains(double,double)const
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        overloadId = 1; // contains(double,double)const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTD_IDX]), (pyArgs[0])))) {
        overloadId = 0; // contains(RectD)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_contains_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // contains(const RectD & other) const
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return {};
            ::RectD *cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // contains(RectD)const
                bool cppResult = const_cast<const ::RectD *>(cppSelf)->contains(*cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            }
            break;
        }
        case 1: // contains(double x, double y) const
        {
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // contains(double,double)const
                bool cppResult = const_cast<const ::RectD *>(cppSelf)->contains(cppArg0, cppArg1);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_RectDFunc_contains_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectDFunc_height(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.height";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // height()const
            double cppResult = const_cast<const ::RectD *>(cppSelf)->height();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_RectDFunc_intersect(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.intersect";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectDFunc_intersect_TypeError;

    if (!PyArg_UnpackTuple(args, "intersect", 1, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: RectD::intersect(RectD,RectD*)const
    // 1: RectD::intersect(double,double,double,double,RectD*)const
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        overloadId = 1; // intersect(double,double,double,double,RectD*)const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTD_IDX]), (pyArgs[0])))) {
        overloadId = 0; // intersect(RectD,RectD*)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_intersect_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // intersect(const RectD & r, RectD * intersection) const
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return {};
            ::RectD *cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // intersect(RectD,RectD*)const
                // Begin code injection
                RectD t;
                cppSelf->intersect(*cppArg0,&t);
                pyResult = Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTD_IDX]), &t);
                return pyResult;

                // End of code injection

            }
            break;
        }
        case 1: // intersect(double l, double b, double r, double t, RectD * intersection) const
        {
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            double cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);
            double cppArg3;
            pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // intersect(double,double,double,double,RectD*)const
                // Begin code injection
                RectD t;
                cppSelf->intersect(cppArg0,cppArg1,cppArg2,cppArg3,&t);
                pyResult = Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTD_IDX]), &t);
                return pyResult;

                // End of code injection

            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_RectDFunc_intersect_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectDFunc_intersects(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.intersects";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectDFunc_intersects_TypeError;

    if (!PyArg_UnpackTuple(args, "intersects", 1, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: RectD::intersects(RectD)const
    // 1: RectD::intersects(double,double,double,double)const
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        overloadId = 1; // intersects(double,double,double,double)const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTD_IDX]), (pyArgs[0])))) {
        overloadId = 0; // intersects(RectD)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_intersects_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // intersects(const RectD & r) const
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return {};
            ::RectD *cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // intersects(RectD)const
                bool cppResult = const_cast<const ::RectD *>(cppSelf)->intersects(*cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            }
            break;
        }
        case 1: // intersects(double l, double b, double r, double t) const
        {
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            double cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);
            double cppArg3;
            pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // intersects(double,double,double,double)const
                bool cppResult = const_cast<const ::RectD *>(cppSelf)->intersects(cppArg0, cppArg1, cppArg2, cppArg3);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_RectDFunc_intersects_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectDFunc_isInfinite(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.isInfinite";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isInfinite()const
            bool cppResult = const_cast<const ::RectD *>(cppSelf)->isInfinite();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_RectDFunc_isNull(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.isNull";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isNull()const
            bool cppResult = const_cast<const ::RectD *>(cppSelf)->isNull();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_RectDFunc_left(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.left";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // left()const
            double cppResult = const_cast<const ::RectD *>(cppSelf)->left();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_RectDFunc_merge(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.merge";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectDFunc_merge_TypeError;

    if (!PyArg_UnpackTuple(args, "merge", 1, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: RectD::merge(RectD)
    // 1: RectD::merge(double,double,double,double)
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        overloadId = 1; // merge(double,double,double,double)
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTD_IDX]), (pyArgs[0])))) {
        overloadId = 0; // merge(RectD)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_merge_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // merge(const RectD & box)
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return {};
            ::RectD *cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // merge(RectD)
                cppSelf->merge(*cppArg0);
            }
            break;
        }
        case 1: // merge(double l, double b, double r, double t)
        {
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            double cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);
            double cppArg3;
            pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // merge(double,double,double,double)
                cppSelf->merge(cppArg0, cppArg1, cppArg2, cppArg3);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_RectDFunc_merge_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectDFunc_right(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.right";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // right()const
            double cppResult = const_cast<const ::RectD *>(cppSelf)->right();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_RectDFunc_set(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.set";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectDFunc_set_TypeError;

    if (!PyArg_UnpackTuple(args, "set", 1, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: RectD::set(RectD)
    // 1: RectD::set(double,double,double,double)
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[3])))) {
        overloadId = 1; // set(double,double,double,double)
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTD_IDX]), (pyArgs[0])))) {
        overloadId = 0; // set(RectD)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_set_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // set(const RectD & b)
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return {};
            ::RectD *cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // set(RectD)
                cppSelf->set(*cppArg0);
            }
            break;
        }
        case 1: // set(double l, double b, double r, double t)
        {
            double cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            double cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            double cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);
            double cppArg3;
            pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // set(double,double,double,double)
                cppSelf->set(cppArg0, cppArg1, cppArg2, cppArg3);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_RectDFunc_set_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectDFunc_set_bottom(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.set_bottom";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: RectD::set_bottom(double)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArg)))) {
        overloadId = 0; // set_bottom(double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_set_bottom_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // set_bottom(double)
            cppSelf->set_bottom(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_RectDFunc_set_bottom_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectDFunc_set_left(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.set_left";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: RectD::set_left(double)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArg)))) {
        overloadId = 0; // set_left(double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_set_left_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // set_left(double)
            cppSelf->set_left(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_RectDFunc_set_left_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectDFunc_set_right(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.set_right";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: RectD::set_right(double)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArg)))) {
        overloadId = 0; // set_right(double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_set_right_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // set_right(double)
            cppSelf->set_right(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_RectDFunc_set_right_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectDFunc_set_top(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.set_top";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: RectD::set_top(double)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArg)))) {
        overloadId = 0; // set_top(double)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_set_top_TypeError;

    // Call function/method
    {
        double cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // set_top(double)
            cppSelf->set_top(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_RectDFunc_set_top_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectDFunc_setupInfinity(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.setupInfinity";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // setupInfinity()
            cppSelf->setupInfinity();
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;
}

static PyObject *Sbk_RectDFunc_top(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.top";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // top()const
            double cppResult = const_cast<const ::RectD *>(cppSelf)->top();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_RectDFunc_translate(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.translate";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "translate", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return {};


    // Overloaded function decisor
    // 0: RectD::translate(int,int)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
        overloadId = 0; // translate(int,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectDFunc_translate_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // translate(int,int)
            cppSelf->translate(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_RectDFunc_translate_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectDFunc_width(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectD.width";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // width()const
            double cppResult = const_cast<const ::RectD *>(cppSelf)->width();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}


static const char *Sbk_RectD_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_RectD_methods[] = {
    {"area", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_area), METH_NOARGS},
    {"bottom", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_bottom), METH_NOARGS},
    {"clear", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_clear), METH_NOARGS},
    {"contains", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_contains), METH_VARARGS},
    {"height", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_height), METH_NOARGS},
    {"intersect", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_intersect), METH_VARARGS},
    {"intersects", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_intersects), METH_VARARGS},
    {"isInfinite", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_isInfinite), METH_NOARGS},
    {"isNull", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_isNull), METH_NOARGS},
    {"left", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_left), METH_NOARGS},
    {"merge", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_merge), METH_VARARGS},
    {"right", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_right), METH_NOARGS},
    {"set", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_set), METH_VARARGS},
    {"set_bottom", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_set_bottom), METH_O},
    {"set_left", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_set_left), METH_O},
    {"set_right", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_set_right), METH_O},
    {"set_top", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_set_top), METH_O},
    {"setupInfinity", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_setupInfinity), METH_NOARGS},
    {"top", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_top), METH_NOARGS},
    {"translate", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_translate), METH_VARARGS},
    {"width", reinterpret_cast<PyCFunction>(Sbk_RectDFunc_width), METH_NOARGS},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_RectD_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<RectDWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

static int Sbk_RectD___nb_bool(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return -1;
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    return !cppSelf->isNull();
}

// Rich comparison
static PyObject * Sbk_RectD_richcompare(PyObject *self, PyObject *pyArg, int op)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto &cppSelf = *reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    switch (op) {
        case Py_NE:
            if ((pythonToCpp = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTD_IDX]), (pyArg)))) {
                // operator!=(const RectD & b2)
                if (!Shiboken::Object::isValid(pyArg))
                    return {};
                ::RectD *cppArg0;
                pythonToCpp(pyArg, &cppArg0);
                bool cppResult = cppSelf !=(*cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            } else {
                pyResult = Py_True;
                Py_INCREF(pyResult);
            }

            break;
        case Py_EQ:
            if ((pythonToCpp = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTD_IDX]), (pyArg)))) {
                // operator==(const RectD & b2)
                if (!Shiboken::Object::isValid(pyArg))
                    return {};
                ::RectD *cppArg0;
                pythonToCpp(pyArg, &cppArg0);
                bool cppResult = cppSelf ==(*cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            } else {
                pyResult = Py_False;
                Py_INCREF(pyResult);
            }

            break;
        default:
            // PYSIDE-74: By default, we redirect to object's tp_richcompare (which is `==`, `!=`).
            return FallbackRichCompare(self, pyArg, op);
            goto Sbk_RectD_RichComparison_TypeError;
    }

    if (pyResult && !PyErr_Occurred())
        return pyResult;
    Sbk_RectD_RichComparison_TypeError:
    PyErr_SetString(PyExc_NotImplementedError, "operator not implemented.");
    return {};

}

static PyObject *Sbk_RectD_get_x1(PyObject *self, void *)
{
    if (!Shiboken::Object::isValid(self))
        return nullptr;
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyOut = {};
    pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->x1);
    return pyOut;
}
static int Sbk_RectD_set_x1(PyObject *self, PyObject *pyIn, void *)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    if (pyIn == nullptr) {
        PyErr_SetString(PyExc_TypeError, "'x1' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp{nullptr};
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'x1', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->x1;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

static PyObject *Sbk_RectD_get_y1(PyObject *self, void *)
{
    if (!Shiboken::Object::isValid(self))
        return nullptr;
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyOut = {};
    pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->y1);
    return pyOut;
}
static int Sbk_RectD_set_y1(PyObject *self, PyObject *pyIn, void *)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    if (pyIn == nullptr) {
        PyErr_SetString(PyExc_TypeError, "'y1' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp{nullptr};
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'y1', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->y1;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

static PyObject *Sbk_RectD_get_x2(PyObject *self, void *)
{
    if (!Shiboken::Object::isValid(self))
        return nullptr;
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyOut = {};
    pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->x2);
    return pyOut;
}
static int Sbk_RectD_set_x2(PyObject *self, PyObject *pyIn, void *)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    if (pyIn == nullptr) {
        PyErr_SetString(PyExc_TypeError, "'x2' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp{nullptr};
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'x2', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->x2;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

static PyObject *Sbk_RectD_get_y2(PyObject *self, void *)
{
    if (!Shiboken::Object::isValid(self))
        return nullptr;
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyOut = {};
    pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<double>(), &cppSelf->y2);
    return pyOut;
}
static int Sbk_RectD_set_y2(PyObject *self, PyObject *pyIn, void *)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    auto cppSelf = reinterpret_cast< ::RectD *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTD_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    if (pyIn == nullptr) {
        PyErr_SetString(PyExc_TypeError, "'y2' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp{nullptr};
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'y2', 'double' or convertible type expected");
        return -1;
    }

    double& cppOut_ptr = cppSelf->y2;
    pythonToCpp(pyIn, &cppOut_ptr);

    return 0;
}

// Getters and Setters for RectD
static PyGetSetDef Sbk_RectD_getsetlist[] = {
    {const_cast<char *>("x1"), Sbk_RectD_get_x1, Sbk_RectD_set_x1},
    {const_cast<char *>("y1"), Sbk_RectD_get_y1, Sbk_RectD_set_y1},
    {const_cast<char *>("x2"), Sbk_RectD_get_x2, Sbk_RectD_set_x2},
    {const_cast<char *>("y2"), Sbk_RectD_get_y2, Sbk_RectD_set_y2},
    {nullptr} // Sentinel
};

} // extern "C"

static int Sbk_RectD_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_RectD_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_RectD_Type = nullptr;
static SbkObjectType *Sbk_RectD_TypeF(void)
{
    return _Sbk_RectD_Type;
}

static PyType_Slot Sbk_RectD_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_RectD_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_RectD_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_RectD_clear)},
    {Py_tp_richcompare, reinterpret_cast<void *>(Sbk_RectD_richcompare)},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_RectD_methods)},
    {Py_tp_getset,      reinterpret_cast<void *>(Sbk_RectD_getsetlist)},
    {Py_tp_init,        reinterpret_cast<void *>(Sbk_RectD_Init)},
    {Py_tp_new,         reinterpret_cast<void *>(SbkObjectTpNew)},
    // type supports number protocol
#ifdef IS_PY3K
    {Py_nb_bool, (void *)Sbk_RectD___nb_bool},
#else
    {Py_nb_nonzero, (void *)Sbk_RectD___nb_bool},
#endif
    {0, nullptr}
};
static PyType_Spec Sbk_RectD_spec = {
    "1:NatronEngine.RectD",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_RectD_slots
};

} //extern "C"


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void RectD_PythonToCpp_RectD_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_RectD_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_RectD_PythonToCpp_RectD_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_RectD_TypeF())))
        return RectD_PythonToCpp_RectD_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *RectD_PTR_CppToPython_RectD(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::RectD *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_RectD_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *RectD_SignatureStrings[] = {
    "2:NatronEngine.RectD(self)",
    "1:NatronEngine.RectD(self,b:NatronEngine.RectD)",
    "0:NatronEngine.RectD(self,l:double,b:double,r:double,t:double)",
    "NatronEngine.RectD.area(self)->double",
    "NatronEngine.RectD.bottom(self)->double",
    "NatronEngine.RectD.clear(self)",
    "1:NatronEngine.RectD.contains(self,other:NatronEngine.RectD)->bool",
    "0:NatronEngine.RectD.contains(self,x:double,y:double)->bool",
    "NatronEngine.RectD.height(self)->double",
    "1:NatronEngine.RectD.intersect(self,r:NatronEngine.RectD,intersection:NatronEngine.RectD)->bool",
    "0:NatronEngine.RectD.intersect(self,l:double,b:double,r:double,t:double,intersection:NatronEngine.RectD)->bool",
    "1:NatronEngine.RectD.intersects(self,r:NatronEngine.RectD)->bool",
    "0:NatronEngine.RectD.intersects(self,l:double,b:double,r:double,t:double)->bool",
    "NatronEngine.RectD.isInfinite(self)->bool",
    "NatronEngine.RectD.isNull(self)->bool",
    "NatronEngine.RectD.left(self)->double",
    "1:NatronEngine.RectD.merge(self,box:NatronEngine.RectD)",
    "0:NatronEngine.RectD.merge(self,l:double,b:double,r:double,t:double)",
    "NatronEngine.RectD.right(self)->double",
    "1:NatronEngine.RectD.set(self,b:NatronEngine.RectD)",
    "0:NatronEngine.RectD.set(self,l:double,b:double,r:double,t:double)",
    "NatronEngine.RectD.set_bottom(self,v:double)",
    "NatronEngine.RectD.set_left(self,v:double)",
    "NatronEngine.RectD.set_right(self,v:double)",
    "NatronEngine.RectD.set_top(self,v:double)",
    "NatronEngine.RectD.setupInfinity(self)",
    "NatronEngine.RectD.top(self)->double",
    "NatronEngine.RectD.translate(self,dx:int,dy:int)",
    "NatronEngine.RectD.width(self)->double",
    nullptr}; // Sentinel

void init_RectD(PyObject *module)
{
    _Sbk_RectD_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "RectD",
        "RectD*",
        &Sbk_RectD_spec,
        &Shiboken::callCppDestructor< ::RectD >,
        0,
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_RectD_Type);
    InitSignatureStrings(pyType, RectD_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_RectD_Type), Sbk_RectD_PropertyStrings);
    SbkNatronEngineTypes[SBK_RECTD_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_RectD_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_RectD_TypeF(),
        RectD_PythonToCpp_RectD_PTR,
        is_RectD_PythonToCpp_RectD_PTR_Convertible,
        RectD_PTR_CppToPython_RectD);

    Shiboken::Conversions::registerConverterName(converter, "RectD");
    Shiboken::Conversions::registerConverterName(converter, "RectD*");
    Shiboken::Conversions::registerConverterName(converter, "RectD&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::RectD).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::RectDWrapper).name());


    RectDWrapper::pysideInitQtMetaTypes();
}
