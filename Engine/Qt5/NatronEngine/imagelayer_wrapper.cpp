
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
#include "imagelayer_wrapper.h"

// inner classes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING

// Extra includes
#include <PyNode.h>


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


// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_ImageLayer_Init(PyObject *self, PyObject *args, PyObject *kwds)
{
    SbkObject *sbkSelf = reinterpret_cast<SbkObject *>(self);
PySide::Feature::Select(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::ImageLayer >()))
        return -1;

    ::ImageLayer *cptr{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ImageLayer.__init__";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { nullptr, nullptr, nullptr };
    SBK_UNUSED(pythonToCpp)
    const Py_ssize_t numArgs = PyTuple_GET_SIZE(args);
    SBK_UNUSED(numArgs)
    PyObject *pyArgs[] = {0, 0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "ImageLayer", 3, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return -1;


    // Overloaded function decisor
    // 0: ImageLayer::ImageLayer(QString,QString,QStringList)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide2_QtCoreTypeConverters[SBK_QSTRINGLIST_IDX], (pyArgs[2])))) {
        overloadId = 0; // ImageLayer(QString,QString,QStringList)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ImageLayer_Init_TypeError;

    // Call function/method
    {
        ::QString cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::QString cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);
        ::QStringList cppArg2;
        pythonToCpp[2](pyArgs[2], &cppArg2);

        if (!PyErr_Occurred()) {
            // ImageLayer(QString,QString,QStringList)
            cptr = new ::ImageLayer(cppArg0, cppArg1, cppArg2);
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::ImageLayer >(), cptr)) {
        delete cptr;
        Py_XDECREF(errInfo);
        return -1;
    }
    if (!cptr) goto Sbk_ImageLayer_Init_TypeError;

    Shiboken::Object::setValidCpp(sbkSelf, true);
    if (Shiboken::BindingManager::instance().hasWrapper(cptr)) {
        Shiboken::BindingManager::instance().releaseWrapper(Shiboken::BindingManager::instance().retrieveWrapper(cptr));
    }
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;

    Sbk_ImageLayer_Init_TypeError:
        Shiboken::setErrorAboutWrongArguments(args, fullName, errInfo);
        Py_XDECREF(errInfo);
        return -1;
}

static PyObject *Sbk_ImageLayerFunc_getAlphaComponents(PyObject *self)
{
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ImageLayer.getAlphaComponents";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getAlphaComponents()
            ImageLayer cppResult = ::ImageLayer::getAlphaComponents();
            pyResult = Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ImageLayerFunc_getBackwardMotionComponents(PyObject *self)
{
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ImageLayer.getBackwardMotionComponents";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getBackwardMotionComponents()
            ImageLayer cppResult = ::ImageLayer::getBackwardMotionComponents();
            pyResult = Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ImageLayerFunc_getComponentsNames(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::ImageLayer *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ImageLayer.getComponentsNames";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getComponentsNames()const
            const QStringList & cppResult = const_cast<const ::ImageLayer *>(cppSelf)->getComponentsNames();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRINGLIST_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ImageLayerFunc_getComponentsPrettyName(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::ImageLayer *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ImageLayer.getComponentsPrettyName";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getComponentsPrettyName()const
            const QString & cppResult = const_cast<const ::ImageLayer *>(cppSelf)->getComponentsPrettyName();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ImageLayerFunc_getDisparityLeftComponents(PyObject *self)
{
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ImageLayer.getDisparityLeftComponents";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getDisparityLeftComponents()
            ImageLayer cppResult = ::ImageLayer::getDisparityLeftComponents();
            pyResult = Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ImageLayerFunc_getDisparityRightComponents(PyObject *self)
{
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ImageLayer.getDisparityRightComponents";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getDisparityRightComponents()
            ImageLayer cppResult = ::ImageLayer::getDisparityRightComponents();
            pyResult = Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ImageLayerFunc_getForwardMotionComponents(PyObject *self)
{
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ImageLayer.getForwardMotionComponents";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getForwardMotionComponents()
            ImageLayer cppResult = ::ImageLayer::getForwardMotionComponents();
            pyResult = Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ImageLayerFunc_getHash(PyObject *self, PyObject *pyArg)
{
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ImageLayer.getHash";
    SBK_UNUSED(fullName)
    int overloadId = -1;
    PythonToCppFunc pythonToCpp{};
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: static ImageLayer::getHash(ImageLayer)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), (pyArg)))) {
        overloadId = 0; // getHash(ImageLayer)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ImageLayerFunc_getHash_TypeError;

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
            // getHash(ImageLayer)
            int cppResult = ::ImageLayer::getHash(*cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;

    Sbk_ImageLayerFunc_getHash_TypeError:
        Shiboken::setErrorAboutWrongArguments(pyArg, fullName, errInfo);
        Py_XDECREF(errInfo);
        return {};
}

static PyObject *Sbk_ImageLayerFunc_getLayerName(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::ImageLayer *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ImageLayer.getLayerName";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getLayerName()const
            const QString & cppResult = const_cast<const ::ImageLayer *>(cppSelf)->getLayerName();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide2_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ImageLayerFunc_getNoneComponents(PyObject *self)
{
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ImageLayer.getNoneComponents";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNoneComponents()
            ImageLayer cppResult = ::ImageLayer::getNoneComponents();
            pyResult = Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ImageLayerFunc_getNumComponents(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::ImageLayer *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ImageLayer.getNumComponents";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNumComponents()const
            int cppResult = const_cast<const ::ImageLayer *>(cppSelf)->getNumComponents();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ImageLayerFunc_getRGBAComponents(PyObject *self)
{
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ImageLayer.getRGBAComponents";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getRGBAComponents()
            ImageLayer cppResult = ::ImageLayer::getRGBAComponents();
            pyResult = Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ImageLayerFunc_getRGBComponents(PyObject *self)
{
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ImageLayer.getRGBComponents";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getRGBComponents()
            ImageLayer cppResult = ::ImageLayer::getRGBComponents();
            pyResult = Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ImageLayerFunc_isColorPlane(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::ImageLayer *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PyObject *errInfo{};
    SBK_UNUSED(errInfo)
    static const char *fullName = "NatronEngine.ImageLayer.isColorPlane";
    SBK_UNUSED(fullName)

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isColorPlane()const
            bool cppResult = const_cast<const ::ImageLayer *>(cppSelf)->isColorPlane();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}

static PyObject *Sbk_ImageLayer___copy__(PyObject *self)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto &cppSelf = *reinterpret_cast< ::ImageLayer *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], reinterpret_cast<SbkObject *>(self)));
    PyObject *pyResult = Shiboken::Conversions::copyToPython(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), &cppSelf);
    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return {};
    }
    return pyResult;
}


static const char *Sbk_ImageLayer_PropertyStrings[] = {
    nullptr // Sentinel
};

static PyMethodDef Sbk_ImageLayer_methods[] = {
    {"getAlphaComponents", reinterpret_cast<PyCFunction>(Sbk_ImageLayerFunc_getAlphaComponents), METH_NOARGS|METH_STATIC},
    {"getBackwardMotionComponents", reinterpret_cast<PyCFunction>(Sbk_ImageLayerFunc_getBackwardMotionComponents), METH_NOARGS|METH_STATIC},
    {"getComponentsNames", reinterpret_cast<PyCFunction>(Sbk_ImageLayerFunc_getComponentsNames), METH_NOARGS},
    {"getComponentsPrettyName", reinterpret_cast<PyCFunction>(Sbk_ImageLayerFunc_getComponentsPrettyName), METH_NOARGS},
    {"getDisparityLeftComponents", reinterpret_cast<PyCFunction>(Sbk_ImageLayerFunc_getDisparityLeftComponents), METH_NOARGS|METH_STATIC},
    {"getDisparityRightComponents", reinterpret_cast<PyCFunction>(Sbk_ImageLayerFunc_getDisparityRightComponents), METH_NOARGS|METH_STATIC},
    {"getForwardMotionComponents", reinterpret_cast<PyCFunction>(Sbk_ImageLayerFunc_getForwardMotionComponents), METH_NOARGS|METH_STATIC},
    {"getHash", reinterpret_cast<PyCFunction>(Sbk_ImageLayerFunc_getHash), METH_O|METH_STATIC},
    {"getLayerName", reinterpret_cast<PyCFunction>(Sbk_ImageLayerFunc_getLayerName), METH_NOARGS},
    {"getNoneComponents", reinterpret_cast<PyCFunction>(Sbk_ImageLayerFunc_getNoneComponents), METH_NOARGS|METH_STATIC},
    {"getNumComponents", reinterpret_cast<PyCFunction>(Sbk_ImageLayerFunc_getNumComponents), METH_NOARGS},
    {"getRGBAComponents", reinterpret_cast<PyCFunction>(Sbk_ImageLayerFunc_getRGBAComponents), METH_NOARGS|METH_STATIC},
    {"getRGBComponents", reinterpret_cast<PyCFunction>(Sbk_ImageLayerFunc_getRGBComponents), METH_NOARGS|METH_STATIC},
    {"isColorPlane", reinterpret_cast<PyCFunction>(Sbk_ImageLayerFunc_isColorPlane), METH_NOARGS},

    {"__copy__", reinterpret_cast<PyCFunction>(Sbk_ImageLayer___copy__), METH_NOARGS},
    {nullptr, nullptr} // Sentinel
};

// Rich comparison
static PyObject * Sbk_ImageLayer_richcompare(PyObject *self, PyObject *pyArg, int op)
{
    if (!Shiboken::Object::isValid(self))
        return {};
    auto &cppSelf = *reinterpret_cast< ::ImageLayer *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    PyObject *pyResult{};
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    switch (op) {
        case Py_NE:
            if ((pythonToCpp = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), (pyArg)))) {
                // operator!=(const ImageLayer & other) const
                if (!Shiboken::Object::isValid(pyArg))
                    return {};
                ::ImageLayer cppArg0_local = ::ImageLayer(::QString(), ::QString(), ::QStringList());
                ::ImageLayer *cppArg0 = &cppArg0_local;
                if (Shiboken::Conversions::isImplicitConversion(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), pythonToCpp))
                    pythonToCpp(pyArg, &cppArg0_local);
                else
                    pythonToCpp(pyArg, &cppArg0);

                bool cppResult = cppSelf !=(*cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            } else {
                pyResult = Py_True;
                Py_INCREF(pyResult);
            }

            break;
        case Py_LT:
            if ((pythonToCpp = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), (pyArg)))) {
                // operator<(const ImageLayer & other) const
                if (!Shiboken::Object::isValid(pyArg))
                    return {};
                ::ImageLayer cppArg0_local = ::ImageLayer(::QString(), ::QString(), ::QStringList());
                ::ImageLayer *cppArg0 = &cppArg0_local;
                if (Shiboken::Conversions::isImplicitConversion(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), pythonToCpp))
                    pythonToCpp(pyArg, &cppArg0_local);
                else
                    pythonToCpp(pyArg, &cppArg0);

                bool cppResult = cppSelf <(*cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            } else {
                goto Sbk_ImageLayer_RichComparison_TypeError;
            }

            break;
        case Py_EQ:
            if ((pythonToCpp = Shiboken::Conversions::isPythonToCppReferenceConvertible(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), (pyArg)))) {
                // operator==(const ImageLayer & other) const
                if (!Shiboken::Object::isValid(pyArg))
                    return {};
                ::ImageLayer cppArg0_local = ::ImageLayer(::QString(), ::QString(), ::QStringList());
                ::ImageLayer *cppArg0 = &cppArg0_local;
                if (Shiboken::Conversions::isImplicitConversion(reinterpret_cast<SbkObjectType *>(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]), pythonToCpp))
                    pythonToCpp(pyArg, &cppArg0_local);
                else
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
            goto Sbk_ImageLayer_RichComparison_TypeError;
    }

    if (pyResult && !PyErr_Occurred())
        return pyResult;
    Sbk_ImageLayer_RichComparison_TypeError:
    PyErr_SetString(PyExc_NotImplementedError, "operator not implemented.");
    return {};

}

} // extern "C"

static Py_hash_t Sbk_ImageLayer_HashFunc(PyObject *self) {
    if (!Shiboken::Object::isValid(self))
        return {};
    auto cppSelf = reinterpret_cast< ::ImageLayer *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], reinterpret_cast<SbkObject *>(self)));
    SBK_UNUSED(cppSelf)
    return Py_hash_t(ImageLayer::getHash(*cppSelf));
}

static int Sbk_ImageLayer_traverse(PyObject *self, visitproc visit, void *arg)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_traverse(self, visit, arg);
}
static int Sbk_ImageLayer_clear(PyObject *self)
{
    return reinterpret_cast<PyTypeObject *>(SbkObject_TypeF())->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType *_Sbk_ImageLayer_Type = nullptr;
static SbkObjectType *Sbk_ImageLayer_TypeF(void)
{
    return _Sbk_ImageLayer_Type;
}

static PyType_Slot Sbk_ImageLayer_slots[] = {
    {Py_tp_base,        nullptr}, // inserted by introduceWrapperType
    {Py_tp_dealloc,     reinterpret_cast<void *>(&SbkDeallocWrapper)},
    {Py_tp_repr,        nullptr},
    {Py_tp_hash,        reinterpret_cast<void *>(&Sbk_ImageLayer_HashFunc)},
    {Py_tp_call,        nullptr},
    {Py_tp_str,         nullptr},
    {Py_tp_getattro,    nullptr},
    {Py_tp_setattro,    nullptr},
    {Py_tp_traverse,    reinterpret_cast<void *>(Sbk_ImageLayer_traverse)},
    {Py_tp_clear,       reinterpret_cast<void *>(Sbk_ImageLayer_clear)},
    {Py_tp_richcompare, reinterpret_cast<void *>(Sbk_ImageLayer_richcompare)},
    {Py_tp_iter,        nullptr},
    {Py_tp_iternext,    nullptr},
    {Py_tp_methods,     reinterpret_cast<void *>(Sbk_ImageLayer_methods)},
    {Py_tp_getset,      nullptr},
    {Py_tp_init,        reinterpret_cast<void *>(Sbk_ImageLayer_Init)},
    {Py_tp_new,         reinterpret_cast<void *>(SbkObjectTpNew)},
    {0, nullptr}
};
static PyType_Spec Sbk_ImageLayer_spec = {
    "1:NatronEngine.ImageLayer",
    sizeof(SbkObject),
    0,
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    Sbk_ImageLayer_slots
};

} //extern "C"


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void ImageLayer_PythonToCpp_ImageLayer_PTR(PyObject *pyIn, void *cppOut) {
    Shiboken::Conversions::pythonToCppPointer(Sbk_ImageLayer_TypeF(), pyIn, cppOut);
}
static PythonToCppFunc is_ImageLayer_PythonToCpp_ImageLayer_PTR_Convertible(PyObject *pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_ImageLayer_TypeF())))
        return ImageLayer_PythonToCpp_ImageLayer_PTR;
    return {};
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject *ImageLayer_PTR_CppToPython_ImageLayer(const void *cppIn) {
    auto pyOut = reinterpret_cast<PyObject *>(Shiboken::BindingManager::instance().retrieveWrapper(cppIn));
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    bool changedTypeName = false;
    auto tCppIn = reinterpret_cast<const ::ImageLayer *>(cppIn);
    const char *typeName = typeid(*tCppIn).name();
    auto sbkType = Shiboken::ObjectType::typeForTypeName(typeName);
    if (sbkType && Shiboken::ObjectType::hasSpecialCastFunction(sbkType)) {
        typeName = typeNameOf(tCppIn);
        changedTypeName = true;
    }
    PyObject *result = Shiboken::Object::newObject(Sbk_ImageLayer_TypeF(), const_cast<void *>(cppIn), false, /* exactType */ changedTypeName, typeName);
    if (changedTypeName)
        delete [] typeName;
    return result;
}

// C++ to Python copy conversion.
static PyObject *ImageLayer_COPY_CppToPython_ImageLayer(const void *cppIn) {
    return Shiboken::Object::newObject(Sbk_ImageLayer_TypeF(), new ::ImageLayer(*reinterpret_cast<const ::ImageLayer *>(cppIn)), true, true);
}

// Python to C++ copy conversion.
static void ImageLayer_PythonToCpp_ImageLayer_COPY(PyObject *pyIn, void *cppOut) {
    *reinterpret_cast<::ImageLayer *>(cppOut) = *reinterpret_cast< ::ImageLayer *>(Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], reinterpret_cast<SbkObject *>(pyIn)));
}
static PythonToCppFunc is_ImageLayer_PythonToCpp_ImageLayer_COPY_Convertible(PyObject *pyIn) {
    if (PyObject_TypeCheck(pyIn, reinterpret_cast<PyTypeObject *>(Sbk_ImageLayer_TypeF())))
        return ImageLayer_PythonToCpp_ImageLayer_COPY;
    return {};
}

// The signatures string for the functions.
// Multiple signatures have their index "n:" in front.
static const char *ImageLayer_SignatureStrings[] = {
    "NatronEngine.ImageLayer(self,layerName:QString,componentsPrettyName:QString,componentsName:QStringList)",
    "NatronEngine.ImageLayer.getAlphaComponents()->NatronEngine.ImageLayer",
    "NatronEngine.ImageLayer.getBackwardMotionComponents()->NatronEngine.ImageLayer",
    "NatronEngine.ImageLayer.getComponentsNames(self)->QStringList",
    "NatronEngine.ImageLayer.getComponentsPrettyName(self)->QString",
    "NatronEngine.ImageLayer.getDisparityLeftComponents()->NatronEngine.ImageLayer",
    "NatronEngine.ImageLayer.getDisparityRightComponents()->NatronEngine.ImageLayer",
    "NatronEngine.ImageLayer.getForwardMotionComponents()->NatronEngine.ImageLayer",
    "NatronEngine.ImageLayer.getHash(layer:NatronEngine.ImageLayer)->int",
    "NatronEngine.ImageLayer.getLayerName(self)->QString",
    "NatronEngine.ImageLayer.getNoneComponents()->NatronEngine.ImageLayer",
    "NatronEngine.ImageLayer.getNumComponents(self)->int",
    "NatronEngine.ImageLayer.getRGBAComponents()->NatronEngine.ImageLayer",
    "NatronEngine.ImageLayer.getRGBComponents()->NatronEngine.ImageLayer",
    "NatronEngine.ImageLayer.isColorPlane(self)->bool",
    "NatronEngine.ImageLayer.__copy__()",
    nullptr}; // Sentinel

void init_ImageLayer(PyObject *module)
{
    _Sbk_ImageLayer_Type = Shiboken::ObjectType::introduceWrapperType(
        module,
        "ImageLayer",
        "ImageLayer",
        &Sbk_ImageLayer_spec,
        &Shiboken::callCppDestructor< ::ImageLayer >,
        0,
        0,
        0    );
    
    auto pyType = reinterpret_cast<PyTypeObject *>(_Sbk_ImageLayer_Type);
    InitSignatureStrings(pyType, ImageLayer_SignatureStrings);
    SbkObjectType_SetPropertyStrings(reinterpret_cast<PyTypeObject *>(_Sbk_ImageLayer_Type), Sbk_ImageLayer_PropertyStrings);
    SbkNatronEngineTypes[SBK_IMAGELAYER_IDX]
        = reinterpret_cast<PyTypeObject *>(Sbk_ImageLayer_TypeF());

    // Register Converter
    SbkConverter *converter = Shiboken::Conversions::createConverter(Sbk_ImageLayer_TypeF(),
        ImageLayer_PythonToCpp_ImageLayer_PTR,
        is_ImageLayer_PythonToCpp_ImageLayer_PTR_Convertible,
        ImageLayer_PTR_CppToPython_ImageLayer,
        ImageLayer_COPY_CppToPython_ImageLayer);

    Shiboken::Conversions::registerConverterName(converter, "ImageLayer");
    Shiboken::Conversions::registerConverterName(converter, "ImageLayer*");
    Shiboken::Conversions::registerConverterName(converter, "ImageLayer&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::ImageLayer).name());

    // Add Python to C++ copy (value, not pointer neither reference) conversion to type converter.
    Shiboken::Conversions::addPythonToCppValueConversion(converter,
        ImageLayer_PythonToCpp_ImageLayer_COPY,
        is_ImageLayer_PythonToCpp_ImageLayer_COPY_Convertible);

}
