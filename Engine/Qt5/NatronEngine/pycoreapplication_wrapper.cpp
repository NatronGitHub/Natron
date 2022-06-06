
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
#include "pycoreapplication_wrapper.h"

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

void PyCoreApplicationWrapper::pysideInitQtMetaTypes()
{
}

void PyCoreApplicationWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

PyCoreApplicationWrapper::PyCoreApplicationWrapper() : PyCoreApplication()
{
    resetPyMethodCache();
    // ... middle
}

PyCoreApplicationWrapper::~PyCoreApplicationWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_PyCoreApplication_Init(PyObject *self, PyObject *args, PyObject *kwds)
{
    SbkObject *sbkSelf = reinterpret_cast<SbkObject *>(self);
PySide::Feature::Select(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::PyCoreApplication >()))
        return -1;

    ::PyCoreApplicationWrapper *cptr{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.__init__";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // PyCoreApplication()
            cptr = new ::PyCoreApplicationWrapper();
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::PyCoreApplication >(), cptr)) {
        delete cptr;
        return -1;
    }
    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::Object::setHasCppWrapper(sbkSelf, true);
    if (Shiboken::BindingManager::instance().hasWrapper(cptr)) {
        Shiboken::BindingManager::instance().releaseWrapper(Shiboken::BindingManager::instance().retrieveWrapper(cptr));
    }
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;
}

static PyObject *Sbk_PyCoreApplicationFunc_appendToNatronPath(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.appendToNatronPath";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PyCoreApplication::appendToNatronPath(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // appendToNatronPath(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyCoreApplicationFunc_appendToNatronPath_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // appendToNatronPath(QString)
            cppSelf->appendToNatronPath(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyCoreApplicationFunc_appendToNatronPath_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyCoreApplicationFunc_getActiveInstance(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.getActiveInstance";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getActiveInstance()const
            App * cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->getActiveInstance();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_APP_IDX]), cppResult);

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

static PyObject *Sbk_PyCoreApplicationFunc_getBuildNumber(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.getBuildNumber";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getBuildNumber()const
            int cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->getBuildNumber();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyCoreApplicationFunc_getInstance(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.getInstance";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PyCoreApplication::getInstance(int)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // getInstance(int)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyCoreApplicationFunc_getInstance_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getInstance(int)const
            App * cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->getInstance(cppArg0);
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_APP_IDX]), cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_PyCoreApplicationFunc_getInstance_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyCoreApplicationFunc_getNatronDevelopmentStatus(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.getNatronDevelopmentStatus";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNatronDevelopmentStatus()const
            QString cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->getNatronDevelopmentStatus();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyCoreApplicationFunc_getNatronPath(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.getNatronPath";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNatronPath()const
            QStringList cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->getNatronPath();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRINGLIST_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyCoreApplicationFunc_getNatronVersionEncoded(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.getNatronVersionEncoded";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNatronVersionEncoded()const
            int cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->getNatronVersionEncoded();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyCoreApplicationFunc_getNatronVersionMajor(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.getNatronVersionMajor";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNatronVersionMajor()const
            int cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->getNatronVersionMajor();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyCoreApplicationFunc_getNatronVersionMinor(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.getNatronVersionMinor";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNatronVersionMinor()const
            int cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->getNatronVersionMinor();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyCoreApplicationFunc_getNatronVersionRevision(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.getNatronVersionRevision";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNatronVersionRevision()const
            int cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->getNatronVersionRevision();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyCoreApplicationFunc_getNatronVersionString(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.getNatronVersionString";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNatronVersionString()const
            QString cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->getNatronVersionString();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyCoreApplicationFunc_getNumCpus(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.getNumCpus";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNumCpus()const
            int cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->getNumCpus();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyCoreApplicationFunc_getNumInstances(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.getNumInstances";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNumInstances()const
            int cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->getNumInstances();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyCoreApplicationFunc_getPluginIDs(PyObject *self, PyObject *args)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.getPluginIDs";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "getPluginIDs", 0, 1, &(pyArgs[0])))
        return {};


    // Overloaded function decisor
    // 0: PyCoreApplication::getPluginIDs()const
    // 1: PyCoreApplication::getPluginIDs(QString)const
    if (numArgs == 0) {
        overloadId = 0; // getPluginIDs()const
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))) {
        overloadId = 1; // getPluginIDs(QString)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyCoreApplicationFunc_getPluginIDs_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // getPluginIDs() const
        {

            if (!PyErr_Occurred()) {
                // getPluginIDs()const
                QStringList cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->getPluginIDs();
                pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRINGLIST_IDX], &cppResult);
            }
            break;
        }
        case 1: // getPluginIDs(const QString & filter) const
        {
            ::QString cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);

            if (!PyErr_Occurred()) {
                // getPluginIDs(QString)const
                QStringList cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->getPluginIDs(cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRINGLIST_IDX], &cppResult);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_PyCoreApplicationFunc_getPluginIDs_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyCoreApplicationFunc_getSettings(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.getSettings";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getSettings()const
            AppSettings * cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->getSettings();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_APPSETTINGS_IDX]), cppResult);

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

static PyObject *Sbk_PyCoreApplicationFunc_is64Bit(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.is64Bit";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // is64Bit()const
            bool cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->is64Bit();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyCoreApplicationFunc_isBackground(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.isBackground";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isBackground()const
            bool cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->isBackground();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyCoreApplicationFunc_isLinux(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.isLinux";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isLinux()const
            bool cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->isLinux();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyCoreApplicationFunc_isMacOSX(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.isMacOSX";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isMacOSX()const
            bool cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->isMacOSX();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyCoreApplicationFunc_isUnix(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.isUnix";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isUnix()const
            bool cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->isUnix();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyCoreApplicationFunc_isWindows(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.isWindows";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isWindows()const
            bool cppResult = const_cast<const ::PyCoreApplication *>(cppSelf)->isWindows();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_PyCoreApplicationFunc_setOnProjectCreatedCallback(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.setOnProjectCreatedCallback";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PyCoreApplication::setOnProjectCreatedCallback(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // setOnProjectCreatedCallback(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyCoreApplicationFunc_setOnProjectCreatedCallback_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setOnProjectCreatedCallback(QString)
            cppSelf->setOnProjectCreatedCallback(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyCoreApplicationFunc_setOnProjectCreatedCallback_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_PyCoreApplicationFunc_setOnProjectLoadedCallback(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.PyCoreApplication.setOnProjectLoadedCallback";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: PyCoreApplication::setOnProjectLoadedCallback(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // setOnProjectLoadedCallback(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyCoreApplicationFunc_setOnProjectLoadedCallback_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setOnProjectLoadedCallback(QString)
            cppSelf->setOnProjectLoadedCallback(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_PyCoreApplicationFunc_setOnProjectLoadedCallback_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}


static const char *Sbk_PyCoreApplication_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_PyCoreApplication_methods[] = {
    {"appendToNatronPath", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_appendToNatronPath), METH_O},
    {"getActiveInstance", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_getActiveInstance), METH_NOARGS},
    {"getBuildNumber", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_getBuildNumber), METH_NOARGS},
    {"getInstance", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_getInstance), METH_O},
    {"getNatronDevelopmentStatus", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_getNatronDevelopmentStatus), METH_NOARGS},
    {"getNatronPath", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_getNatronPath), METH_NOARGS},
    {"getNatronVersionEncoded", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_getNatronVersionEncoded), METH_NOARGS},
    {"getNatronVersionMajor", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_getNatronVersionMajor), METH_NOARGS},
    {"getNatronVersionMinor", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_getNatronVersionMinor), METH_NOARGS},
    {"getNatronVersionRevision", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_getNatronVersionRevision), METH_NOARGS},
    {"getNatronVersionString", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_getNatronVersionString), METH_NOARGS},
    {"getNumCpus", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_getNumCpus), METH_NOARGS},
    {"getNumInstances", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_getNumInstances), METH_NOARGS},
    {"getPluginIDs", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_getPluginIDs), METH_VARARGS},
    {"getSettings", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_getSettings), METH_NOARGS},
    {"is64Bit", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_is64Bit), METH_NOARGS},
    {"isBackground", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_isBackground), METH_NOARGS},
    {"isLinux", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_isLinux), METH_NOARGS},
    {"isMacOSX", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_isMacOSX), METH_NOARGS},
    {"isUnix", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_isUnix), METH_NOARGS},
    {"isWindows", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_isWindows), METH_NOARGS},
    {"setOnProjectCreatedCallback", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_setOnProjectCreatedCallback), METH_O},
    {"setOnProjectLoadedCallback", reinterpret_cast<PyCFunction>(Sbk_PyCoreApplicationFunc_setOnProjectLoadedCallback), METH_O},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_PyCoreApplication_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::PyCoreApplication *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<PyCoreApplicationWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_PyCoreApplication_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_PyCoreApplication_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_PyCoreApplication_Type = nullptr;
static SbkObjectType *Sbk_PyCoreApplication_TypeF(void)
{
    return _Sbk_PyCoreApplication_Type;
}

static PyType_Slot Sbk_PyCoreApplication_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_PyCoreApplication_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_PyCoreApplication_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_PyCoreApplication_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_PyCoreApplication_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        reinterpret_cast<void *>(Sbk_PyCoreApplication_Init)},
    {Py_tp_new,         reinterpret_cast<void *>(SbkObjectTpNew)},
    {0, nullptr}
};
static PyType_Spec Sbk_PyCoreApplication_spec = {
    "1:NatronEngine.PyCoreApplication",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_PyCoreApplication_slots
};

} //extern "C"


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void PyCoreApplication_PythonToCpp_PyCoreApplication_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_PyCoreApplication_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_PyCoreApplication_PythonToCpp_PyCoreApplication_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_PyCoreApplication_TypeF())))
        return PyCoreApplication_PythonToCpp_PyCoreApplication_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *PyCoreApplication_PTR_CppToPython_PyCoreApplication(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::PyCoreApplication *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_PyCoreApplication_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *PyCoreApplication_SignatureStrings[] = {
    "NatronEngine.PyCoreApplication(self)",
    "NatronEngine.PyCoreApplication.appendToNatronPath(self,path:QString)",
    "NatronEngine.PyCoreApplication.getActiveInstance(self)->NatronEngine.App",
    "NatronEngine.PyCoreApplication.getBuildNumber(self)->int",
    "NatronEngine.PyCoreApplication.getInstance(self,idx:int)->NatronEngine.App",
    "NatronEngine.PyCoreApplication.getNatronDevelopmentStatus(self)->QString",
    "NatronEngine.PyCoreApplication.getNatronPath(self)->QStringList",
    "NatronEngine.PyCoreApplication.getNatronVersionEncoded(self)->int",
    "NatronEngine.PyCoreApplication.getNatronVersionMajor(self)->int",
    "NatronEngine.PyCoreApplication.getNatronVersionMinor(self)->int",
    "NatronEngine.PyCoreApplication.getNatronVersionRevision(self)->int",
    "NatronEngine.PyCoreApplication.getNatronVersionString(self)->QString",
    "NatronEngine.PyCoreApplication.getNumCpus(self)->int",
    "NatronEngine.PyCoreApplication.getNumInstances(self)->int",
    "1:NatronEngine.PyCoreApplication.getPluginIDs(self)->QStringList",
    "0:NatronEngine.PyCoreApplication.getPluginIDs(self,filter:QString)->QStringList",
    "NatronEngine.PyCoreApplication.getSettings(self)->NatronEngine.AppSettings",
    "NatronEngine.PyCoreApplication.is64Bit(self)->bool",
    "NatronEngine.PyCoreApplication.isBackground(self)->bool",
    "NatronEngine.PyCoreApplication.isLinux(self)->bool",
    "NatronEngine.PyCoreApplication.isMacOSX(self)->bool",
    "NatronEngine.PyCoreApplication.isUnix(self)->bool",
    "NatronEngine.PyCoreApplication.isWindows(self)->bool",
    "NatronEngine.PyCoreApplication.setOnProjectCreatedCallback(self,pythonFunctionName:QString)",
    "NatronEngine.PyCoreApplication.setOnProjectLoadedCallback(self,pythonFunctionName:QString)",
    nullptr}; // Sentinel

void init_PyCoreApplication(PyObject *module)
{
    _Sbk_PyCoreApplication_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "PyCoreApplication",
        "PyCoreApplication*",
        &Sbk_PyCoreApplication_spec,
        &Shiboken::callCppDestructor< ::PyCoreApplication >,
        0,
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_PyCoreApplication_Type);
    InitSignatureStrings(pyType, PyCoreApplication_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_PyCoreApplication_Type), Sbk_PyCoreApplication_PropertyStrings);
    SbkNatronEngineTypes[SBK_PYCOREAPPLICATION_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_PyCoreApplication_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_PyCoreApplication_TypeF(),
        PyCoreApplication_PythonToCpp_PyCoreApplication_PTR,
        is_PyCoreApplication_PythonToCpp_PyCoreApplication_PTR_Convertible,
        PyCoreApplication_PTR_CppToPython_PyCoreApplication);

    Shiboken::Conversions::registerConverterName(converter, "PyCoreApplication");
    Shiboken::Conversions::registerConverterName(converter, "PyCoreApplication*");
    Shiboken::Conversions::registerConverterName(converter, "PyCoreApplication&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyCoreApplication).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyCoreApplicationWrapper).name());


    PyCoreApplicationWrapper::pysideInitQtMetaTypes();
}
