
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
#include "app_wrapper.h"

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

void AppWrapper::pysideInitQtMetaTypes()
{
}

void AppWrapper::resetPyMethodCache()
{
    std::fill_n(m_PyMethodCache, sizeof(m_PyMethodCache) / sizeof(m_PyMethodCache[0]), false);
}

AppWrapper::~AppWrapper()
{
    SbkObject *wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject *Sbk_AppFunc_addFormat(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.addFormat";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: App::addFormat(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // addFormat(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AppFunc_addFormat_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // addFormat(QString)
            cppSelf->addFormat(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_AppFunc_addFormat_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AppFunc_addProjectLayer(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.addProjectLayer";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: App::addProjectLayer(ImageLayer)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), (pyArg)))) {
        overloadId = 0; // addProjectLayer(ImageLayer)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AppFunc_addProjectLayer_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArg))
            return {};
        ::ImageLayer cppArg0_local = ::ImageLayer(::QString(), ::QString(), ::QStringList());
        ::ImageLayer *cppArg0 = &cppArg0_local;
        if (Shiboken::Conversions::isImplicitConversion(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), pythonToCpp))
            pythonToCpp(pyArg, &cppArg0_local);
        else
            pythonToCpp(pyArg, &cppArg0);


        if (!PyErr_Occurred()) {
            // addProjectLayer(ImageLayer)
            cppSelf->addProjectLayer(*cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_AppFunc_addProjectLayer_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AppFunc_closeProject(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.closeProject";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // closeProject()
            bool cppResult = cppSelf->closeProject();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_AppFunc_createNode(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.createNode";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs > 4) {
        static PyObject *const too_many = Shiboken::String::createStaticString(">");
        errInfo = too_many;
        Py_INCREF(errInfo);
        goto Sbk_AppFunc_createNode_TypeError;
    } else if (numArgs < 1) {
        static PyObject *const too_few = Shiboken::String::createStaticString("<");
        errInfo = too_few;
        Py_INCREF(errInfo);
        goto Sbk_AppFunc_createNode_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|OOOO:createNode", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: App::createNode(QString,int,Group*,std::map<QString,NodeCreationProperty*>)const
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // createNode(QString,int,Group*,std::map<QString,NodeCreationProperty*>)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
            if (numArgs == 2) {
                overloadId = 0; // createNode(QString,int,Group*,std::map<QString,NodeCreationProperty*>)const
            } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_GROUP_IDX]), (pyArgs[2])))) {
                if (numArgs == 3) {
                    overloadId = 0; // createNode(QString,int,Group*,std::map<QString,NodeCreationProperty*>)const
                } else if ((pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_QSTRING_NODECREATIONPROPERTYPTR_IDX], (pyArgs[3])))) {
                    overloadId = 0; // createNode(QString,int,Group*,std::map<QString,NodeCreationProperty*>)const
                }
            }
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AppFunc_createNode_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_majorVersion = Shiboken::String::createStaticString("majorVersion");
            if (PyDict_Contains(kwds, key_majorVersion)) {
                value = PyDict_GetItem(kwds, key_majorVersion);
                if (value && pyArgs[1]) {
                    errInfo = key_majorVersion;
                    Py_INCREF(errInfo);
                    goto Sbk_AppFunc_createNode_TypeError;
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1]))))
                        goto Sbk_AppFunc_createNode_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_majorVersion);
            }
            static PyObject *const key_group = Shiboken::String::createStaticString("group");
            if (PyDict_Contains(kwds, key_group)) {
                value = PyDict_GetItem(kwds, key_group);
                if (value && pyArgs[2]) {
                    errInfo = key_group;
                    Py_INCREF(errInfo);
                    goto Sbk_AppFunc_createNode_TypeError;
                }
                if (value) {
                    pyArgs[2] = value;
                    if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_GROUP_IDX]), (pyArgs[2]))))
                        goto Sbk_AppFunc_createNode_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_group);
            }
            static PyObject *const key_props = Shiboken::String::createStaticString("props");
            if (PyDict_Contains(kwds, key_props)) {
                value = PyDict_GetItem(kwds, key_props);
                if (value && pyArgs[3]) {
                    errInfo = key_props;
                    Py_INCREF(errInfo);
                    goto Sbk_AppFunc_createNode_TypeError;
                }
                if (value) {
                    pyArgs[3] = value;
                    if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_QSTRING_NODECREATIONPROPERTYPTR_IDX], (pyArgs[3]))))
                        goto Sbk_AppFunc_createNode_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_props);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_AppFunc_createNode_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        ::QString cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1 = -1;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);
        if (!Shiboken::Object::isValid(pyArgs[2]))
            return {};
        ::Group *cppArg2 = 0;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);
        ::std::map<QString,NodeCreationProperty* > cppArg3;
        if (pythonToCpp[3]) pythonToCpp[3](pyArgs[3], &cppArg3);

        if (!PyErr_Occurred()) {
            // createNode(QString,int,Group*,std::map<QString,NodeCreationProperty*>)const
            // Begin code injection
            Effect * cppResult = cppSelf->createNode(cppArg0,cppArg1,cppArg2, cppArg3);
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_EFFECT_IDX]), cppResult);

            // End of code injection


            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_AppFunc_createNode_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AppFunc_createReader(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.createReader";
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
        goto Sbk_AppFunc_createReader_TypeError;
    } else if (numArgs < 1) {
        static PyObject *const too_few = Shiboken::String::createStaticString("<");
        errInfo = too_few;
        Py_INCREF(errInfo);
        goto Sbk_AppFunc_createReader_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|OOO:createReader", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return {};


    // Overloaded function decisor
    // 0: App::createReader(QString,Group*,std::map<QString,NodeCreationProperty*>)const
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // createReader(QString,Group*,std::map<QString,NodeCreationProperty*>)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_GROUP_IDX]), (pyArgs[1])))) {
            if (numArgs == 2) {
                overloadId = 0; // createReader(QString,Group*,std::map<QString,NodeCreationProperty*>)const
            } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_QSTRING_NODECREATIONPROPERTYPTR_IDX], (pyArgs[2])))) {
                overloadId = 0; // createReader(QString,Group*,std::map<QString,NodeCreationProperty*>)const
            }
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AppFunc_createReader_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_group = Shiboken::String::createStaticString("group");
            if (PyDict_Contains(kwds, key_group)) {
                value = PyDict_GetItem(kwds, key_group);
                if (value && pyArgs[1]) {
                    errInfo = key_group;
                    Py_INCREF(errInfo);
                    goto Sbk_AppFunc_createReader_TypeError;
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_GROUP_IDX]), (pyArgs[1]))))
                        goto Sbk_AppFunc_createReader_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_group);
            }
            static PyObject *const key_props = Shiboken::String::createStaticString("props");
            if (PyDict_Contains(kwds, key_props)) {
                value = PyDict_GetItem(kwds, key_props);
                if (value && pyArgs[2]) {
                    errInfo = key_props;
                    Py_INCREF(errInfo);
                    goto Sbk_AppFunc_createReader_TypeError;
                }
                if (value) {
                    pyArgs[2] = value;
                    if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_QSTRING_NODECREATIONPROPERTYPTR_IDX], (pyArgs[2]))))
                        goto Sbk_AppFunc_createReader_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_props);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_AppFunc_createReader_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        ::QString cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return {};
        ::Group *cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);
        ::std::map<QString,NodeCreationProperty* > cppArg2;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // createReader(QString,Group*,std::map<QString,NodeCreationProperty*>)const
            // Begin code injection
            Effect * cppResult = cppSelf->createReader(cppArg0,cppArg1, cppArg2);
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_EFFECT_IDX]), cppResult);

            // End of code injection


            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_AppFunc_createReader_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AppFunc_createWriter(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.createWriter";
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
        goto Sbk_AppFunc_createWriter_TypeError;
    } else if (numArgs < 1) {
        static PyObject *const too_few = Shiboken::String::createStaticString("<");
        errInfo = too_few;
        Py_INCREF(errInfo);
        goto Sbk_AppFunc_createWriter_TypeError;
    }

    if (!PyArg_ParseTuple(args, "|OOO:createWriter", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return {};


    // Overloaded function decisor
    // 0: App::createWriter(QString,Group*,std::map<QString,NodeCreationProperty*>)const
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // createWriter(QString,Group*,std::map<QString,NodeCreationProperty*>)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_GROUP_IDX]), (pyArgs[1])))) {
            if (numArgs == 2) {
                overloadId = 0; // createWriter(QString,Group*,std::map<QString,NodeCreationProperty*>)const
            } else if ((pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_QSTRING_NODECREATIONPROPERTYPTR_IDX], (pyArgs[2])))) {
                overloadId = 0; // createWriter(QString,Group*,std::map<QString,NodeCreationProperty*>)const
            }
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AppFunc_createWriter_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject *value{};
            PyObject *kwds_dup = PyDict_Copy(kwds);
            static PyObject *const key_group = Shiboken::String::createStaticString("group");
            if (PyDict_Contains(kwds, key_group)) {
                value = PyDict_GetItem(kwds, key_group);
                if (value && pyArgs[1]) {
                    errInfo = key_group;
                    Py_INCREF(errInfo);
                    goto Sbk_AppFunc_createWriter_TypeError;
                }
                if (value) {
                    pyArgs[1] = value;
                    if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_GROUP_IDX]), (pyArgs[1]))))
                        goto Sbk_AppFunc_createWriter_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_group);
            }
            static PyObject *const key_props = Shiboken::String::createStaticString("props");
            if (PyDict_Contains(kwds, key_props)) {
                value = PyDict_GetItem(kwds, key_props);
                if (value && pyArgs[2]) {
                    errInfo = key_props;
                    Py_INCREF(errInfo);
                    goto Sbk_AppFunc_createWriter_TypeError;
                }
                if (value) {
                    pyArgs[2] = value;
                    if (!(pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_MAP_QSTRING_NODECREATIONPROPERTYPTR_IDX], (pyArgs[2]))))
                        goto Sbk_AppFunc_createWriter_TypeError;
                }
                PyDict_DelItem(kwds_dup, key_props);
            }
            if (PyDict_Size(kwds_dup) > 0) {
                errInfo = kwds_dup;
                goto Sbk_AppFunc_createWriter_TypeError;
            } else {
                Py_DECREF(kwds_dup);
            }
        }
        ::QString cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return {};
        ::Group *cppArg1 = 0;
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);
        ::std::map<QString,NodeCreationProperty* > cppArg2;
        if (pythonToCpp[2]) pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // createWriter(QString,Group*,std::map<QString,NodeCreationProperty*>)const
            // Begin code injection
            Effect * cppResult = cppSelf->createWriter(cppArg0,cppArg1, cppArg2);
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_EFFECT_IDX]), cppResult);

            // End of code injection


            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_AppFunc_createWriter_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AppFunc_getAppID(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.getAppID";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getAppID()const
            int cppResult = const_cast<const ::AppWrapper *>(cppSelf)->getAppID();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_AppFunc_getProjectParam(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.getProjectParam";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: App::getProjectParam(QString)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // getProjectParam(QString)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AppFunc_getProjectParam_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getProjectParam(QString)const
            Param * cppResult = const_cast<const ::AppWrapper *>(cppSelf)->getProjectParam(cppArg0);
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

    Sbk_AppFunc_getProjectParam_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AppFunc_getViewNames(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.getViewNames";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getViewNames()const
            std::list<QString > cppResult = const_cast<const ::AppWrapper *>(cppSelf)->getViewNames();
            pyResult = Shiboken::Conversions::copyToPython(SbkNatronEngineTypeConverters[SBK_NATRONENGINE_STD_LIST_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_AppFunc_loadProject(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.loadProject";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: App::loadProject(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // loadProject(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AppFunc_loadProject_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // loadProject(QString)
            // Begin code injection
            App * cppResult = cppSelf->loadProject(cppArg0);
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_APP_IDX]), cppResult);

            // End of code injection


            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_AppFunc_loadProject_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AppFunc_newProject(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.newProject";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // newProject()
            // Begin code injection
            App * cppResult = cppSelf->newProject();
            pyResult = Shiboken::Conversions::pointerToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_APP_IDX]), cppResult);

            // End of code injection


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

static PyObject *Sbk_AppFunc_render(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.render";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0, 0};

    // invalid argument lengths
    if (numArgs > 4) {
        static PyObject *const too_many = Shiboken::String::createStaticString(">");
        errInfo = too_many;
        Py_INCREF(errInfo);
        goto Sbk_AppFunc_render_TypeError;
    } else if (numArgs < 1) {
        static PyObject *const too_few = Shiboken::String::createStaticString("<");
        errInfo = too_few;
        Py_INCREF(errInfo);
        goto Sbk_AppFunc_render_TypeError;
    } else if (numArgs == 2)
        goto Sbk_AppFunc_render_TypeError;

    if (!PyArg_ParseTuple(args, "|OOOO:render", &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2]), &(pyArgs[3])))
        return {};


    // Overloaded function decisor
    // 0: App::render(Effect*,int,int,int)
    // 1: App::render(std::list<Effect*>,std::list<int>,std::list<int>,std::list<int>)
    if (numArgs >= 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_EFFECT_IDX]), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
        if (numArgs == 3) {
            overloadId = 0; // render(Effect*,int,int,int)
        } else if ((pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[3])))) {
            overloadId = 0; // render(Effect*,int,int,int)
        }
    } else if (numArgs == 1
        && PyList_Check(pyArgs[0])) {
        overloadId = 1; // render(std::list<Effect*>,std::list<int>,std::list<int>,std::list<int>)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AppFunc_render_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // render(Effect * writeNode, int firstFrame, int lastFrame, int frameStep)
        {
            if (kwds) {
                PyObject *value{};
                PyObject *kwds_dup = PyDict_Copy(kwds);
                static PyObject *const key_frameStep = Shiboken::String::createStaticString("frameStep");
                if (PyDict_Contains(kwds, key_frameStep)) {
                    value = PyDict_GetItem(kwds, key_frameStep);
                    if (value && pyArgs[3]) {
                        errInfo = key_frameStep;
                        Py_INCREF(errInfo);
                        goto Sbk_AppFunc_render_TypeError;
                    }
                    if (value) {
                        pyArgs[3] = value;
                        if (!(pythonToCpp[3] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[3]))))
                            goto Sbk_AppFunc_render_TypeError;
                    }
                    PyDict_DelItem(kwds_dup, key_frameStep);
                }
                if (PyDict_Size(kwds_dup) > 0) {
                    errInfo = kwds_dup;
                    goto Sbk_AppFunc_render_TypeError;
                } else {
                    Py_DECREF(kwds_dup);
                }
            }
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return {};
            ::Effect *cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            int cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);
            int cppArg3 = 1;
            if (pythonToCpp[3]) pythonToCpp[3](pyArgs[3], &cppArg3);

            if (!PyErr_Occurred()) {
                // render(Effect*,int,int,int)
                cppSelf->render(cppArg0, cppArg1, cppArg2, cppArg3);
            }
            break;
        }
        case 1: // render(const std::list<Effect* > & effects, const std::list<int > & firstFrames, const std::list<int > & lastFrames, const std::list<int > & frameSteps)
        {
            if (kwds) {
                errInfo = kwds;
                Py_INCREF(errInfo);
                goto Sbk_AppFunc_render_TypeError;
            }

            if (!PyErr_Occurred()) {
                // render(std::list<Effect*>,std::list<int>,std::list<int>,std::list<int>)
                // Begin code injection
                if (!PyList_Check(pyArgs[1-1])) {
                    PyErr_SetString(PyExc_TypeError, "tasks must be a list of tuple objects.");
                    return 0;
                }
                std::list<Effect*> effects;

                std::list<int> firstFrames;

                std::list<int> lastFrames;

                std::list<int> frameSteps;

                int size = (int)PyList_GET_SIZE(pyArgs[1-1]);
                for (int i = 0; i < size; ++i) {
                    PyObject* tuple = PyList_GET_ITEM(pyArgs[1-1],i);
                    if (!tuple) {
                        PyErr_SetString(PyExc_TypeError, "tasks must be a list of tuple objects.");
                        return 0;
                    }

                    int tupleSize = PyTuple_GET_SIZE(tuple);
                    if (tupleSize != 4 && tupleSize != 3) {
                        PyErr_SetString(PyExc_TypeError, "the tuple must have 3 or 4 items.");
                        return 0;
                    }
                    ::Effect* writeNode{nullptr};
                    Shiboken::Conversions::pythonToCppPointer(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_EFFECT_IDX]), PyTuple_GET_ITEM(tuple, 0), &(writeNode));
                    int firstFrame;
                    Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<int>(), PyTuple_GET_ITEM(tuple, 1), &(firstFrame));
                    int lastFrame;
                    Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<int>(), PyTuple_GET_ITEM(tuple, 2), &(lastFrame));
                    int frameStep;
                    if (tupleSize == 4) {
                        Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<int>(), PyTuple_GET_ITEM(tuple, 3), &(frameStep));
                    } else {
                        frameStep = INT_MIN;
                    }
                    effects.push_back(writeNode);
                    firstFrames.push_back(firstFrame);
                    lastFrames.push_back(lastFrame);
                    frameSteps.push_back(frameStep);
                }

                cppSelf->render(effects,firstFrames,lastFrames, frameSteps);

                // End of code injection

            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_AppFunc_render_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AppFunc_resetProject(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.resetProject";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // resetProject()
            bool cppResult = cppSelf->resetProject();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_AppFunc_saveProject(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.saveProject";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: App::saveProject(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // saveProject(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AppFunc_saveProject_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // saveProject(QString)
            bool cppResult = cppSelf->saveProject(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_AppFunc_saveProject_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AppFunc_saveProjectAs(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.saveProjectAs";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: App::saveProjectAs(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // saveProjectAs(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AppFunc_saveProjectAs_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // saveProjectAs(QString)
            bool cppResult = cppSelf->saveProjectAs(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_AppFunc_saveProjectAs_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AppFunc_saveTempProject(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.saveTempProject";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: App::saveTempProject(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // saveTempProject(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AppFunc_saveTempProject_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // saveTempProject(QString)
            bool cppResult = cppSelf->saveTempProject(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_AppFunc_saveTempProject_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_AppFunc_timelineGetLeftBound(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.timelineGetLeftBound";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // timelineGetLeftBound()const
            int cppResult = const_cast<const ::AppWrapper *>(cppSelf)->timelineGetLeftBound();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_AppFunc_timelineGetRightBound(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.timelineGetRightBound";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // timelineGetRightBound()const
            int cppResult = const_cast<const ::AppWrapper *>(cppSelf)->timelineGetRightBound();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_AppFunc_timelineGetTime(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.timelineGetTime";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // timelineGetTime()const
            int cppResult = const_cast<const ::AppWrapper *>(cppSelf)->timelineGetTime();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_AppFunc_writeToScriptEditor(PyObject *self, PyObject *pyArg)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = static_cast<AppWrapper *>(reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self))));
    SBK_UNUSED(cppSelf)
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.App.writeToScriptEditor";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: App::writeToScriptEditor(QString)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArg)))) {
        overloadId = 0; // writeToScriptEditor(QString)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_AppFunc_writeToScriptEditor_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // writeToScriptEditor(QString)
            cppSelf->writeToScriptEditor(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return {};
    }
    Py_RETURN_NONE;

    Sbk_AppFunc_writeToScriptEditor_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}


static const char *Sbk_App_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_App_methods[] = {
    {"addFormat", reinterpret_cast<PyCFunction>(Sbk_AppFunc_addFormat), METH_O},
    {"addProjectLayer", reinterpret_cast<PyCFunction>(Sbk_AppFunc_addProjectLayer), METH_O},
    {"closeProject", reinterpret_cast<PyCFunction>(Sbk_AppFunc_closeProject), METH_NOARGS},
    {"createNode", reinterpret_cast<PyCFunction>(Sbk_AppFunc_createNode), METH_VARARGS|METH_KEYWORDS},
    {"createReader", reinterpret_cast<PyCFunction>(Sbk_AppFunc_createReader), METH_VARARGS|METH_KEYWORDS},
    {"createWriter", reinterpret_cast<PyCFunction>(Sbk_AppFunc_createWriter), METH_VARARGS|METH_KEYWORDS},
    {"getAppID", reinterpret_cast<PyCFunction>(Sbk_AppFunc_getAppID), METH_NOARGS},
    {"getProjectParam", reinterpret_cast<PyCFunction>(Sbk_AppFunc_getProjectParam), METH_O},
    {"getViewNames", reinterpret_cast<PyCFunction>(Sbk_AppFunc_getViewNames), METH_NOARGS},
    {"loadProject", reinterpret_cast<PyCFunction>(Sbk_AppFunc_loadProject), METH_O},
    {"newProject", reinterpret_cast<PyCFunction>(Sbk_AppFunc_newProject), METH_NOARGS},
    {"render", reinterpret_cast<PyCFunction>(Sbk_AppFunc_render), METH_VARARGS|METH_KEYWORDS},
    {"resetProject", reinterpret_cast<PyCFunction>(Sbk_AppFunc_resetProject), METH_NOARGS},
    {"saveProject", reinterpret_cast<PyCFunction>(Sbk_AppFunc_saveProject), METH_O},
    {"saveProjectAs", reinterpret_cast<PyCFunction>(Sbk_AppFunc_saveProjectAs), METH_O},
    {"saveTempProject", reinterpret_cast<PyCFunction>(Sbk_AppFunc_saveTempProject), METH_O},
    {"timelineGetLeftBound", reinterpret_cast<PyCFunction>(Sbk_AppFunc_timelineGetLeftBound), METH_NOARGS},
    {"timelineGetRightBound", reinterpret_cast<PyCFunction>(Sbk_AppFunc_timelineGetRightBound), METH_NOARGS},
    {"timelineGetTime", reinterpret_cast<PyCFunction>(Sbk_AppFunc_timelineGetTime), METH_NOARGS},
    {"writeToScriptEditor", reinterpret_cast<PyCFunction>(Sbk_AppFunc_writeToScriptEditor), METH_O},

    {nullptr, nullptr} // Sentinel
};

static int Sbk_App_setattro(PyObject *self, PyObject *name, PyObject *value)
{
    PySide::Feature::Select(self);
    if (value && PyCallable_Check(value)) {
        auto plain_inst = reinterpret_cast< ::App *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_APP_IDX], reinterpret_cast<SbkObject *>(self)));
        auto inst = dynamic_cast<AppWrapper *>(plain_inst);
        if (inst)
            inst->resetPyMethodCache();
    }
    return PyObject_GenericSetAttr(self, name, value);
}

} // extern "C"

static int Sbk_App_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_App_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_App_Type = nullptr;
static SbkObjectType *Sbk_App_TypeF(void)
{
    return _Sbk_App_Type;
}

static PyType_Slot Sbk_App_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        nullptr},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    reinterpret_cast<void *>(Sbk_App_setattro)},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_App_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_App_clear)},
    {Py_tp_richcompare, nullptr},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_App_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        nullptr},
    {Py_tp_new,         reinterpret_cast<void *>(SbkDummyNew /* PYSIDE-595: Prevent replacement of "0" with base->tp_new. */)},
    {0, nullptr}
};
static PyType_Spec Sbk_App_spec = {
    "1:NatronEngine.App",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_App_slots
};

} //extern "C"

static void *Sbk_App_typeDiscovery(void *cptr, SbkObjectType *instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType *>(Shiboken::SbkType< ::Group >()))
        return dynamic_cast< ::App *>(reinterpret_cast< ::Group *>(cptr));
    return {};
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void App_PythonToCpp_App_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_App_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_App_PythonToCpp_App_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_App_TypeF())))
        return App_PythonToCpp_App_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *App_PTR_CppToPython_App(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::App *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_App_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *App_SignatureStrings[] = {
    "NatronEngine.App.addFormat(self,formatSpec:QString)",
    "NatronEngine.App.addProjectLayer(self,layer:NatronEngine.ImageLayer)",
    "NatronEngine.App.closeProject(self)->bool",
    "NatronEngine.App.createNode(self,pluginID:QString,majorVersion:int=-1,group:NatronEngine.Group=0,props:std.map[QString, NatronEngine.NodeCreationProperty]=NodeCreationPropertyMap())->NatronEngine.Effect",
    "NatronEngine.App.createReader(self,filename:QString,group:NatronEngine.Group=0,props:std.map[QString, NatronEngine.NodeCreationProperty]=NodeCreationPropertyMap())->NatronEngine.Effect",
    "NatronEngine.App.createWriter(self,filename:QString,group:NatronEngine.Group=0,props:std.map[QString, NatronEngine.NodeCreationProperty]=NodeCreationPropertyMap())->NatronEngine.Effect",
    "NatronEngine.App.getAppID(self)->int",
    "NatronEngine.App.getProjectParam(self,name:QString)->NatronEngine.Param",
    "NatronEngine.App.getViewNames(self)->std.list[QString]",
    "NatronEngine.App.loadProject(self,filename:QString)->NatronEngine.App",
    "NatronEngine.App.newProject(self)->NatronEngine.App",
    "1:NatronEngine.App.render(self,writeNode:NatronEngine.Effect,firstFrame:int,lastFrame:int,frameStep:int=1)",
    "0:NatronEngine.App.render(self,effects:std.list[NatronEngine.Effect],firstFrames:std.list[int],lastFrames:std.list[int],frameSteps:std.list[int])",
    "NatronEngine.App.resetProject(self)->bool",
    "NatronEngine.App.saveProject(self,filename:QString)->bool",
    "NatronEngine.App.saveProjectAs(self,filename:QString)->bool",
    "NatronEngine.App.saveTempProject(self,filename:QString)->bool",
    "NatronEngine.App.timelineGetLeftBound(self)->int",
    "NatronEngine.App.timelineGetRightBound(self)->int",
    "NatronEngine.App.timelineGetTime(self)->int",
    "NatronEngine.App.writeToScriptEditor(self,message:QString)",
    nullptr}; // Sentinel

void init_App(PyObject *module)
{
    _Sbk_App_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "App",
        "App*",
        &Sbk_App_spec,
        &Shiboken::callCppDestructor< ::App >,
        reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_GROUP_IDX]),
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_App_Type);
    InitSignatureStrings(pyType, App_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_App_Type), Sbk_App_PropertyStrings);
    SbkNatronEngineTypes[SBK_APP_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_App_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_App_TypeF(),
        App_PythonToCpp_App_PTR,
        is_App_PythonToCpp_App_PTR_Convertible,
        App_PTR_CppToPython_App);

    Shiboken::Conversions::registerConverterName(converter, "App");
    Shiboken::Conversions::registerConverterName(converter, "App*");
    Shiboken::Conversions::registerConverterName(converter, "App&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::App).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::AppWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(Sbk_App_TypeF(), &Sbk_App_typeDiscovery);

    AppWrapper::pysideInitQtMetaTypes();
}
