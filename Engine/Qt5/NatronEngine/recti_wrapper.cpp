
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
#include "recti_wrapper.h"

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

void RectIWrapper::pysideInitQtMetaTypes()
{
}

void RectIWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

RectIWrapper::RectIWrapper() : RectI()
{
    resetPyMethodCache();
    // ... middle
}

RectIWrapper::RectIWrapper(int x1_, int y1_, int x2_, int y2_) : RectI(x1_, y1_, x2_, y2_)
{
    resetPyMethodCache();
    // ... middle
}

RectIWrapper::~RectIWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_RectI_Init(PyObject *self, PyObject *args, PyObject *kwds)
{
    SbkObject *sbkSelf = reinterpret_cast<SbkObject *>(self);
PySide::Feature::Select(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::RectI >()))
        return -1;

    ::RectIWrapper *cptr{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.__init__";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectI_Init_TypeError;

    if (!PyArg_UnpackTuple(args, "RectI", 0, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return -1;


    // Overloaded function decisor
    // 0: RectI::RectI()
    // 1: RectI::RectI(RectI)
    // 2: RectI::RectI(int,int,int,int)
    if (numArgs == 0) {
        overloadId = 0; // RectI()
    } else if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[3])))) {
        overloadId = 2; // RectI(int,int,int,int)
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTI_IDX]), (pyArgs[0])))) {
        overloadId = 1; // RectI(RectI)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectI_Init_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // RectI()
        {

            if (!PyErr_Occurred()) {
                // RectI()
                cptr = new ::RectIWrapper();
            }
            break;
        }
        case 1: // RectI(const RectI & b)
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return -1;
            ::RectI *cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // RectI(RectI)
                cptr = new ::RectIWrapper(*cppArg0);
            }
            break;
        }
        case 2: // RectI(int x1_, int y1_, int x2_, int y2_)
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            int cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);
            int cppArg3;
            pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // RectI(int,int,int,int)
                cptr = new ::RectIWrapper(cppArg0, cppArg1, cppArg2, cppArg3);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::RectI >(), cptr)) {
        delete cptr;
        Py_XDECREF(errInfo);
        return -1;
    }
    if (!cptr) goto Sbk_RectI_Init_TypeError;

    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::Object::setHasCppWrapper(sbkSelf, true);
    if (Shiboken::BindingManager::instance().hasWrapper(cptr)) {
        Shiboken::BindingManager::instance().releaseWrapper(Shiboken::BindingManager::instance().retrieveWrapper(cptr));
    }
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;

    Sbk_RectI_Init_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return -1;
}

static PyObject *Sbk_RectIFunc_bottom(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.bottom";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // bottom()const
            int cppResult = const_cast<const ::RectI *>(cppSelf)->bottom();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_RectIFunc_clear(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.clear";
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

static PyObject *Sbk_RectIFunc_contains(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.contains";
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
    // 0: RectI::contains(RectI)const
    // 1: RectI::contains(double,double)const
    // 2: RectI::contains(int,int)const
    if (numArgs == 2
        && PyFloat_Check(pyArgs[0]) && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<double>(), (pyArgs[1])))) {
        overloadId = 1; // contains(double,double)const
    } else if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
        overloadId = 2; // contains(int,int)const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTI_IDX]), (pyArgs[0])))) {
        overloadId = 0; // contains(RectI)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_contains_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // contains(const RectI & other) const
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return {};
            ::RectI *cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // contains(RectI)const
                bool cppResult = const_cast<const ::RectI *>(cppSelf)->contains(*cppArg0);
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
                bool cppResult = const_cast<const ::RectI *>(cppSelf)->contains(cppArg0, cppArg1);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            }
            break;
        }
        case 2: // contains(int x, int y) const
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);

            if (!PyErr_Occurred()) {
                // contains(int,int)const
                bool cppResult = const_cast<const ::RectI *>(cppSelf)->contains(cppArg0, cppArg1);
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

    Sbk_RectIFunc_contains_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectIFunc_height(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.height";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // height()const
            int cppResult = const_cast<const ::RectI *>(cppSelf)->height();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_RectIFunc_intersect(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.intersect";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectIFunc_intersect_TypeError;

    if (!PyArg_UnpackTuple(args, "intersect", 1, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: RectI::intersect(RectI,RectI*)const
    // 1: RectI::intersect(int,int,int,int,RectI*)const
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[3])))) {
        overloadId = 1; // intersect(int,int,int,int,RectI*)const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTI_IDX]), (pyArgs[0])))) {
        overloadId = 0; // intersect(RectI,RectI*)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_intersect_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // intersect(const RectI & r, RectI * intersection) const
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return {};
            ::RectI *cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // intersect(RectI,RectI*)const
                // Begin code injection
                RectI t;
                cppSelf->intersect(*cppArg0,&t);
                pyResult = Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTI_IDX]), &t);
                return pyResult;

                // End of code injection

            }
            break;
        }
        case 1: // intersect(int x1_, int y1_, int x2_, int y2_, RectI * intersection) const
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            int cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);
            int cppArg3;
            pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // intersect(int,int,int,int,RectI*)const
                // Begin code injection
                RectI t;
                cppSelf->intersect(cppArg0,cppArg1,cppArg2,cppArg3,&t);
                pyResult = Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTI_IDX]), &t);
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

    Sbk_RectIFunc_intersect_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectIFunc_intersects(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.intersects";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectIFunc_intersects_TypeError;

    if (!PyArg_UnpackTuple(args, "intersects", 1, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: RectI::intersects(RectI)const
    // 1: RectI::intersects(int,int,int,int)const
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[3])))) {
        overloadId = 1; // intersects(int,int,int,int)const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTI_IDX]), (pyArgs[0])))) {
        overloadId = 0; // intersects(RectI)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_intersects_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // intersects(const RectI & r) const
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return {};
            ::RectI *cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // intersects(RectI)const
                bool cppResult = const_cast<const ::RectI *>(cppSelf)->intersects(*cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            }
            break;
        }
        case 1: // intersects(int x1_, int y1_, int x2_, int y2_) const
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            int cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);
            int cppArg3;
            pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // intersects(int,int,int,int)const
                bool cppResult = const_cast<const ::RectI *>(cppSelf)->intersects(cppArg0, cppArg1, cppArg2, cppArg3);
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

    Sbk_RectIFunc_intersects_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectIFunc_isInfinite(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.isInfinite";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isInfinite()const
            bool cppResult = const_cast<const ::RectI *>(cppSelf)->isInfinite();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_RectIFunc_isNull(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.isNull";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isNull()const
            bool cppResult = const_cast<const ::RectI *>(cppSelf)->isNull();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_RectIFunc_left(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.left";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // left()const
            int cppResult = const_cast<const ::RectI *>(cppSelf)->left();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_RectIFunc_merge(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.merge";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectIFunc_merge_TypeError;

    if (!PyArg_UnpackTuple(args, "merge", 1, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: RectI::merge(RectI)
    // 1: RectI::merge(int,int,int,int)
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[3])))) {
        overloadId = 1; // merge(int,int,int,int)
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTI_IDX]), (pyArgs[0])))) {
        overloadId = 0; // merge(RectI)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_merge_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // merge(const RectI & box)
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return {};
            ::RectI *cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // merge(RectI)
                cppSelf->merge(*cppArg0);
            }
            break;
        }
        case 1: // merge(int x1_, int y1_, int x2_, int y2_)
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            int cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);
            int cppArg3;
            pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // merge(int,int,int,int)
                cppSelf->merge(cppArg0, cppArg1, cppArg2, cppArg3);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_RectIFunc_merge_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectIFunc_right(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.right";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // right()const
            int cppResult = const_cast<const ::RectI *>(cppSelf)->right();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_RectIFunc_set(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.set";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2 || numArgs == 3)
        goto Sbk_RectIFunc_set_TypeError;

    if (!PyArg_UnpackTuple(args, "set", 1, 4, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: RectI::set(RectI)
    // 1: RectI::set(int,int,int,int)
    if (numArgs == 4
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))
        && (pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[3])))) {
        overloadId = 1; // set(int,int,int,int)
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTI_IDX]), (pyArgs[0])))) {
        overloadId = 0; // set(RectI)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_set_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // set(const RectI & b)
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return {};
            ::RectI *cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // set(RectI)
                cppSelf->set(*cppArg0);
            }
            break;
        }
        case 1: // set(int x1_, int y1_, int x2_, int y2_)
        {
            int cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            int cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);
            int cppArg3;
            pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // set(int,int,int,int)
                cppSelf->set(cppArg0, cppArg1, cppArg2, cppArg3);
            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_RectIFunc_set_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectIFunc_set_bottom(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.set_bottom";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: RectI::set_bottom(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // set_bottom(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_set_bottom_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // set_bottom(int)
            cppSelf->set_bottom(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_RectIFunc_set_bottom_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectIFunc_set_left(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.set_left";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: RectI::set_left(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // set_left(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_set_left_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // set_left(int)
            cppSelf->set_left(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_RectIFunc_set_left_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectIFunc_set_right(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.set_right";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: RectI::set_right(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // set_right(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_set_right_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // set_right(int)
            cppSelf->set_right(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_RectIFunc_set_right_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectIFunc_set_top(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.set_top";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: RectI::set_top(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // set_top(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_set_top_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // set_top(int)
            cppSelf->set_top(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_RectIFunc_set_top_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectIFunc_top(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.top";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // top()const
            int cppResult = const_cast<const ::RectI *>(cppSelf)->top();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_RectIFunc_translate(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.translate";
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
    // 0: RectI::translate(int,int)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
        overloadId = 0; // translate(int,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_RectIFunc_translate_TypeError;

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

    Sbk_RectIFunc_translate_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_RectIFunc_width(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.RectI.width";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // width()const
            int cppResult = const_cast<const ::RectI *>(cppSelf)->width();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}


static const char *Sbk_RectI_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_RectI_methods[] = {
    {"bottom", reinterpret_cast<PyCFunction>(Sbk_RectIFunc_bottom), METH_NOARGS},
    {"clear", reinterpret_cast<PyCFunction>(Sbk_RectIFunc_clear), METH_NOARGS},
    {"contains", reinterpret_cast<PyCFunction>(Sbk_RectIFunc_contains), METH_VARARGS},
    {"height", reinterpret_cast<PyCFunction>(Sbk_RectIFunc_height), METH_NOARGS},
    {"intersect", reinterpret_cast<PyCFunction>(Sbk_RectIFunc_intersect), METH_VARARGS},
    {"intersects", reinterpret_cast<PyCFunction>(Sbk_RectIFunc_intersects), METH_VARARGS},
    {"isInfinite", reinterpret_cast<PyCFunction>(Sbk_RectIFunc_isInfinite), METH_NOARGS},
    {"isNull", reinterpret_cast<PyCFunction>(Sbk_RectIFunc_isNull), METH_NOARGS},
    {"left", reinterpret_cast<PyCFunction>(Sbk_RectIFunc_left), METH_NOARGS},
    {"merge", reinterpret_cast<PyCFunction>(Sbk_RectIFunc_merge), METH_VARARGS},
    {"right", reinterpret_cast<PyCFunction>(Sbk_RectIFunc_right), METH_NOARGS},
    {"set", reinterpret_cast<PyCFunction>(Sbk_RectIFunc_set), METH_VARARGS},
    {"set_bottom", reinterpret_cast<PyCFunction>(Sbk_RectIFunc_set_bottom), METH_O},
    {"set_left", reinterpret_cast<PyCFunction>(Sbk_RectIFunc_set_left), METH_O},
    {"set_right", reinterpret_cast<PyCFunction>(Sbk_RectIFunc_set_right), METH_O},
    {"set_top", reinterpret_cast<PyCFunction>(Sbk_RectIFunc_set_top), METH_O},
    {"top", reinterpret_cast<PyCFunction>(Sbk_RectIFunc_top), METH_NOARGS},
    {"translate", reinterpret_cast<PyCFunction>(Sbk_RectIFunc_translate), METH_VARARGS},
    {"width", reinterpret_cast<PyCFunction>(Sbk_RectIFunc_width), METH_NOARGS},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_RectI_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<RectIWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

static int Sbk_RectI___nb_bool(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return -1;
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    return !cppSelf->isNull();
}

// Rich comparison
static PyObject * Sbk_RectI_richcompare(PyObject *self, PyObject *pyArg, int op)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto &cppSelf = *reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    switch (op) {
        case Py_NE:
            if ((pythonToCpp = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTI_IDX]), (pyArg)))) {
                // operator!=(const RectI & b2)
                if (!Shiboken::Object::isValid(pyArg))
                    return {};
                ::RectI *cppArg0;
                pythonToCpp(pyArg, &cppArg0);
                bool cppResult = cppSelf !=(*cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            } else {
                pyResult = Py_True;
                Py_INCREF(pyResult);
            }

            break;
        case Py_EQ:
            if ((pythonToCpp = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_RECTI_IDX]), (pyArg)))) {
                // operator==(const RectI & b2)
                if (!Shiboken::Object::isValid(pyArg))
                    return {};
                ::RectI *cppArg0;
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
            goto Sbk_RectI_RichComparison_TypeError;
    }

    if (pyResult && !PyErr_Occurred())
        return pyResult;
    Sbk_RectI_RichComparison_TypeError:
    PyErr_SetString(PyExc_NotImplementedError, "operator not implemented.");
    return {};

}

static PyObject *Sbk_RectI_get_x1(PyObject *self, void *)
{
    if (!Shiboken::Object::isValid(self))
        return nullptr;
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int cppOut_local = cppSelf->x1;
    PyObject *pyOut = {};
    pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppOut_local);
    return pyOut;
}
static int Sbk_RectI_set_x1(PyObject *self, PyObject *pyIn, void *)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    if (pyIn == nullptr) {
        PyErr_SetString(PyExc_TypeError, "'x1' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp{nullptr};
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'x1', 'int' or convertible type expected");
        return -1;
    }

    int cppOut_local = cppSelf->x1;
    pythonToCpp(pyIn, &cppOut_local);
    cppSelf->x1 = cppOut_local;

    return 0;
}

static PyObject *Sbk_RectI_get_y1(PyObject *self, void *)
{
    if (!Shiboken::Object::isValid(self))
        return nullptr;
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int cppOut_local = cppSelf->y1;
    PyObject *pyOut = {};
    pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppOut_local);
    return pyOut;
}
static int Sbk_RectI_set_y1(PyObject *self, PyObject *pyIn, void *)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    if (pyIn == nullptr) {
        PyErr_SetString(PyExc_TypeError, "'y1' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp{nullptr};
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'y1', 'int' or convertible type expected");
        return -1;
    }

    int cppOut_local = cppSelf->y1;
    pythonToCpp(pyIn, &cppOut_local);
    cppSelf->y1 = cppOut_local;

    return 0;
}

static PyObject *Sbk_RectI_get_x2(PyObject *self, void *)
{
    if (!Shiboken::Object::isValid(self))
        return nullptr;
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int cppOut_local = cppSelf->x2;
    PyObject *pyOut = {};
    pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppOut_local);
    return pyOut;
}
static int Sbk_RectI_set_x2(PyObject *self, PyObject *pyIn, void *)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    if (pyIn == nullptr) {
        PyErr_SetString(PyExc_TypeError, "'x2' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp{nullptr};
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'x2', 'int' or convertible type expected");
        return -1;
    }

    int cppOut_local = cppSelf->x2;
    pythonToCpp(pyIn, &cppOut_local);
    cppSelf->x2 = cppOut_local;

    return 0;
}

static PyObject *Sbk_RectI_get_y2(PyObject *self, void *)
{
    if (!Shiboken::Object::isValid(self))
        return nullptr;
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    int cppOut_local = cppSelf->y2;
    PyObject *pyOut = {};
    pyOut = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppOut_local);
    return pyOut;
}
static int Sbk_RectI_set_y2(PyObject *self, PyObject *pyIn, void *)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    auto cppSelf = reinterpret_cast< ::RectI *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_RECTI_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    if (pyIn == nullptr) {
        PyErr_SetString(PyExc_TypeError, "'y2' may not be deleted");
        return -1;
    }
    PythonToCppFunc pythonToCpp{nullptr};
    if (!(pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyIn)))) {
        PyErr_SetString(PyExc_TypeError, "wrong type attributed to 'y2', 'int' or convertible type expected");
        return -1;
    }

    int cppOut_local = cppSelf->y2;
    pythonToCpp(pyIn, &cppOut_local);
    cppSelf->y2 = cppOut_local;

    return 0;
}

// Getters and Setters for RectI
static PyGetSetDef Sbk_RectI_getsetlist[] = {
    {const_cast<char *>("x1"), Sbk_RectI_get_x1, Sbk_RectI_set_x1},
    {const_cast<char *>("y1"), Sbk_RectI_get_y1, Sbk_RectI_set_y1},
    {const_cast<char *>("x2"), Sbk_RectI_get_x2, Sbk_RectI_set_x2},
    {const_cast<char *>("y2"), Sbk_RectI_get_y2, Sbk_RectI_set_y2},
    {nullptr} // Sentinel
};

} // extern "C"

static int Sbk_RectI_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_RectI_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_RectI_Type = nullptr;
static SbkObjectType *Sbk_RectI_TypeF(void)
{
    return _Sbk_RectI_Type;
}

static PyType_Slot Sbk_RectI_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_RectI_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_RectI_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_RectI_clear)},
    {Py_tp_richcompare, reinterpret_cast<void *>(Sbk_RectI_richcompare)},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_RectI_methods)},
    {Py_tp_getset,      reinterpret_cast<void *>(Sbk_RectI_getsetlist)},
    {Py_tp_init,        reinterpret_cast<void *>(Sbk_RectI_Init)},
    {Py_tp_new,         reinterpret_cast<void *>(SbkObjectTpNew)},
    // type supports number protocol
#ifdef IS_PY3K
    {Py_nb_bool, (void *)Sbk_RectI___nb_bool},
#else
    {Py_nb_nonzero, (void *)Sbk_RectI___nb_bool},
#endif
    {0, nullptr}
};
static PyType_Spec Sbk_RectI_spec = {
    "1:NatronEngine.RectI",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_RectI_slots
};

} //extern "C"


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void RectI_PythonToCpp_RectI_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_RectI_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_RectI_PythonToCpp_RectI_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_RectI_TypeF())))
        return RectI_PythonToCpp_RectI_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *RectI_PTR_CppToPython_RectI(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::RectI *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_RectI_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *RectI_SignatureStrings[] = {
    "2:NatronEngine.RectI(self)",
    "1:NatronEngine.RectI(self,b:NatronEngine.RectI)",
    "0:NatronEngine.RectI(self,x1_:int,y1_:int,x2_:int,y2_:int)",
    "NatronEngine.RectI.bottom(self)->int",
    "NatronEngine.RectI.clear(self)",
    "2:NatronEngine.RectI.contains(self,other:NatronEngine.RectI)->bool",
    "1:NatronEngine.RectI.contains(self,x:double,y:double)->bool",
    "0:NatronEngine.RectI.contains(self,x:int,y:int)->bool",
    "NatronEngine.RectI.height(self)->int",
    "1:NatronEngine.RectI.intersect(self,r:NatronEngine.RectI,intersection:NatronEngine.RectI)->bool",
    "0:NatronEngine.RectI.intersect(self,x1_:int,y1_:int,x2_:int,y2_:int,intersection:NatronEngine.RectI)->bool",
    "1:NatronEngine.RectI.intersects(self,r:NatronEngine.RectI)->bool",
    "0:NatronEngine.RectI.intersects(self,x1_:int,y1_:int,x2_:int,y2_:int)->bool",
    "NatronEngine.RectI.isInfinite(self)->bool",
    "NatronEngine.RectI.isNull(self)->bool",
    "NatronEngine.RectI.left(self)->int",
    "1:NatronEngine.RectI.merge(self,box:NatronEngine.RectI)",
    "0:NatronEngine.RectI.merge(self,x1_:int,y1_:int,x2_:int,y2_:int)",
    "NatronEngine.RectI.right(self)->int",
    "1:NatronEngine.RectI.set(self,b:NatronEngine.RectI)",
    "0:NatronEngine.RectI.set(self,x1_:int,y1_:int,x2_:int,y2_:int)",
    "NatronEngine.RectI.set_bottom(self,v:int)",
    "NatronEngine.RectI.set_left(self,v:int)",
    "NatronEngine.RectI.set_right(self,v:int)",
    "NatronEngine.RectI.set_top(self,v:int)",
    "NatronEngine.RectI.top(self)->int",
    "NatronEngine.RectI.translate(self,dx:int,dy:int)",
    "NatronEngine.RectI.width(self)->int",
    nullptr}; // Sentinel

void init_RectI(PyObject *module)
{
    _Sbk_RectI_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "RectI",
        "RectI*",
        &Sbk_RectI_spec,
        &Shiboken::callCppDestructor< ::RectI >,
        0,
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_RectI_Type);
    InitSignatureStrings(pyType, RectI_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_RectI_Type), Sbk_RectI_PropertyStrings);
    SbkNatronEngineTypes[SBK_RECTI_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_RectI_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_RectI_TypeF(),
        RectI_PythonToCpp_RectI_PTR,
        is_RectI_PythonToCpp_RectI_PTR_Convertible,
        RectI_PTR_CppToPython_RectI);

    Shiboken::Conversions::registerConverterName(converter, "RectI");
    Shiboken::Conversions::registerConverterName(converter, "RectI*");
    Shiboken::Conversions::registerConverterName(converter, "RectI&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::RectI).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::RectIWrapper).name());


    RectIWrapper::pysideInitQtMetaTypes();
}
