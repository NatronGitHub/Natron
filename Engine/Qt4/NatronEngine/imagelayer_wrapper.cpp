
// default includes
#include "Global/Macros.h"
// clang-format off
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
GCC_DIAG_OFF(uninitialized)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// clang-format on
#include <shiboken.h> // produces many warnings
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natronengine_python.h"

#include "imagelayer_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING NATRON_PYTHON_NAMESPACE_USING
#include <PyNode.h>



// Target ---------------------------------------------------------

extern "C" {
static int
Sbk_ImageLayer_Init(PyObject* self, PyObject* args, PyObject* kwds)
{
    SbkObject* sbkSelf = reinterpret_cast<SbkObject*>(self);
    if (Shiboken::Object::isUserType(self) && !Shiboken::ObjectType::canCallConstructor(self->ob_type, Shiboken::SbkType< ::ImageLayer >()))
        return -1;

    ::ImageLayer* cptr = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2)
        goto Sbk_ImageLayer_Init_TypeError;

    if (!PyArg_UnpackTuple(args, "ImageLayer", 1, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return -1;


    // Overloaded function decisor
    // 0: ImageLayer(ImageLayer)
    // 1: ImageLayer(QString,QString,QStringList)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(SbkPySide_QtCoreTypeConverters[SBK_QSTRINGLIST_IDX], (pyArgs[2])))) {
        overloadId = 1; // ImageLayer(QString,QString,QStringList)
    } else if (numArgs == 1
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], (pyArgs[0])))) {
        overloadId = 0; // ImageLayer(ImageLayer)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ImageLayer_Init_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // ImageLayer(const ImageLayer & ImageLayer)
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return -1;
            ::ImageLayer cppArg0_local = ::ImageLayer(::QString(), ::QString(), ::QStringList());
            ::ImageLayer* cppArg0 = &cppArg0_local;
            if (Shiboken::Conversions::isImplicitConversion((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], pythonToCpp[0]))
                pythonToCpp[0](pyArgs[0], &cppArg0_local);
            else
                pythonToCpp[0](pyArgs[0], &cppArg0);


            if (!PyErr_Occurred()) {
                // ImageLayer(ImageLayer)
                cptr = new ::ImageLayer(*cppArg0);
            }
            break;
        }
        case 1: // ImageLayer(const QString & layerName, const QString & componentsPrettyName, const QStringList & componentsName)
        {
            ::QString cppArg0 = ::QString();
            pythonToCpp[0](pyArgs[0], &cppArg0);
            ::QString cppArg1 = ::QString();
            pythonToCpp[1](pyArgs[1], &cppArg1);
            ::QStringList cppArg2 = ::QStringList();
            pythonToCpp[2](pyArgs[2], &cppArg2);

            if (!PyErr_Occurred()) {
                // ImageLayer(QString,QString,QStringList)
                cptr = new ::ImageLayer(cppArg0, cppArg1, cppArg2);
            }
            break;
        }
    }

    if (PyErr_Occurred() || !Shiboken::Object::setCppPointer(sbkSelf, Shiboken::SbkType< ::ImageLayer >(), cptr)) {
        delete cptr;
        return -1;
    }
    if (!cptr) goto Sbk_ImageLayer_Init_TypeError;

    Shiboken::Object::setValidCpp(sbkSelf, true);
    Shiboken::BindingManager::instance().registerWrapper(sbkSelf, cptr);


    return 1;

    Sbk_ImageLayer_Init_TypeError:
        const char* overloads[] = {"NatronEngine.ImageLayer", "unicode, unicode, QStringList", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronEngine.ImageLayer", overloads);
        return -1;
}

static PyObject* Sbk_ImageLayerFunc_getAlphaComponents(PyObject* self)
{
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getAlphaComponents()
            ImageLayer cppResult = ::ImageLayer::getAlphaComponents();
            pyResult = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ImageLayerFunc_getBackwardMotionComponents(PyObject* self)
{
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getBackwardMotionComponents()
            ImageLayer cppResult = ::ImageLayer::getBackwardMotionComponents();
            pyResult = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ImageLayerFunc_getComponentsNames(PyObject* self)
{
    ::ImageLayer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ImageLayer*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getComponentsNames()const
            const QStringList & cppResult = const_cast<const ::ImageLayer*>(cppSelf)->getComponentsNames();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRINGLIST_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ImageLayerFunc_getComponentsPrettyName(PyObject* self)
{
    ::ImageLayer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ImageLayer*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getComponentsPrettyName()const
            const QString & cppResult = const_cast<const ::ImageLayer*>(cppSelf)->getComponentsPrettyName();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ImageLayerFunc_getDisparityLeftComponents(PyObject* self)
{
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getDisparityLeftComponents()
            ImageLayer cppResult = ::ImageLayer::getDisparityLeftComponents();
            pyResult = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ImageLayerFunc_getDisparityRightComponents(PyObject* self)
{
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getDisparityRightComponents()
            ImageLayer cppResult = ::ImageLayer::getDisparityRightComponents();
            pyResult = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ImageLayerFunc_getForwardMotionComponents(PyObject* self)
{
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getForwardMotionComponents()
            ImageLayer cppResult = ::ImageLayer::getForwardMotionComponents();
            pyResult = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ImageLayerFunc_getHash(PyObject* self, PyObject* pyArg)
{
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getHash(ImageLayer)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], (pyArg)))) {
        overloadId = 0; // getHash(ImageLayer)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_ImageLayerFunc_getHash_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArg))
            return 0;
        ::ImageLayer cppArg0_local = ::ImageLayer(::QString(), ::QString(), ::QStringList());
        ::ImageLayer* cppArg0 = &cppArg0_local;
        if (Shiboken::Conversions::isImplicitConversion((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], pythonToCpp))
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
        return 0;
    }
    return pyResult;

    Sbk_ImageLayerFunc_getHash_TypeError:
        const char* overloads[] = {"NatronEngine.ImageLayer", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronEngine.ImageLayer.getHash", overloads);
        return 0;
}

static PyObject* Sbk_ImageLayerFunc_getLayerName(PyObject* self)
{
    ::ImageLayer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ImageLayer*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getLayerName()const
            const QString & cppResult = const_cast<const ::ImageLayer*>(cppSelf)->getLayerName();
            pyResult = Shiboken::Conversions::copyToPython(SbkPySide_QtCoreTypeConverters[SBK_QSTRING_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ImageLayerFunc_getNoneComponents(PyObject* self)
{
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNoneComponents()
            ImageLayer cppResult = ::ImageLayer::getNoneComponents();
            pyResult = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ImageLayerFunc_getNumComponents(PyObject* self)
{
    ::ImageLayer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ImageLayer*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getNumComponents()const
            int cppResult = const_cast<const ::ImageLayer*>(cppSelf)->getNumComponents();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ImageLayerFunc_getRGBAComponents(PyObject* self)
{
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getRGBAComponents()
            ImageLayer cppResult = ::ImageLayer::getRGBAComponents();
            pyResult = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ImageLayerFunc_getRGBComponents(PyObject* self)
{
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getRGBComponents()
            ImageLayer cppResult = ::ImageLayer::getRGBComponents();
            pyResult = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ImageLayerFunc_isColorPlane(PyObject* self)
{
    ::ImageLayer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ImageLayer*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isColorPlane()const
            bool cppResult = const_cast<const ::ImageLayer*>(cppSelf)->isColorPlane();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_ImageLayer___copy__(PyObject* self)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    ::ImageLayer& cppSelf = *(((::ImageLayer*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], (SbkObject*)self)));
    PyObject* pyResult = Shiboken::Conversions::copyToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], &cppSelf);
    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyMethodDef Sbk_ImageLayer_methods[] = {
    {"getAlphaComponents", (PyCFunction)Sbk_ImageLayerFunc_getAlphaComponents, METH_NOARGS|METH_STATIC},
    {"getBackwardMotionComponents", (PyCFunction)Sbk_ImageLayerFunc_getBackwardMotionComponents, METH_NOARGS|METH_STATIC},
    {"getComponentsNames", (PyCFunction)Sbk_ImageLayerFunc_getComponentsNames, METH_NOARGS},
    {"getComponentsPrettyName", (PyCFunction)Sbk_ImageLayerFunc_getComponentsPrettyName, METH_NOARGS},
    {"getDisparityLeftComponents", (PyCFunction)Sbk_ImageLayerFunc_getDisparityLeftComponents, METH_NOARGS|METH_STATIC},
    {"getDisparityRightComponents", (PyCFunction)Sbk_ImageLayerFunc_getDisparityRightComponents, METH_NOARGS|METH_STATIC},
    {"getForwardMotionComponents", (PyCFunction)Sbk_ImageLayerFunc_getForwardMotionComponents, METH_NOARGS|METH_STATIC},
    {"getHash", (PyCFunction)Sbk_ImageLayerFunc_getHash, METH_O|METH_STATIC},
    {"getLayerName", (PyCFunction)Sbk_ImageLayerFunc_getLayerName, METH_NOARGS},
    {"getNoneComponents", (PyCFunction)Sbk_ImageLayerFunc_getNoneComponents, METH_NOARGS|METH_STATIC},
    {"getNumComponents", (PyCFunction)Sbk_ImageLayerFunc_getNumComponents, METH_NOARGS},
    {"getRGBAComponents", (PyCFunction)Sbk_ImageLayerFunc_getRGBAComponents, METH_NOARGS|METH_STATIC},
    {"getRGBComponents", (PyCFunction)Sbk_ImageLayerFunc_getRGBComponents, METH_NOARGS|METH_STATIC},
    {"isColorPlane", (PyCFunction)Sbk_ImageLayerFunc_isColorPlane, METH_NOARGS},

    {"__copy__", (PyCFunction)Sbk_ImageLayer___copy__, METH_NOARGS},
    {0} // Sentinel
};

// Rich comparison
static PyObject* Sbk_ImageLayer_richcompare(PyObject* self, PyObject* pyArg, int op)
{
    if (!Shiboken::Object::isValid(self))
        return 0;
    ::ImageLayer& cppSelf = *(((::ImageLayer*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], (SbkObject*)self)));
    SBK_UNUSED(cppSelf)
    PyObject* pyResult = 0;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    switch (op) {
        case Py_NE:
            if ((pythonToCpp = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], (pyArg)))) {
                // operator!=(const ImageLayer & other) const
                if (!Shiboken::Object::isValid(pyArg))
                    return 0;
                ::ImageLayer cppArg0_local = ::ImageLayer(::QString(), ::QString(), ::QStringList());
                ::ImageLayer* cppArg0 = &cppArg0_local;
                if (Shiboken::Conversions::isImplicitConversion((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], pythonToCpp))
                    pythonToCpp(pyArg, &cppArg0_local);
                else
                    pythonToCpp(pyArg, &cppArg0);

                bool cppResult = cppSelf != (*cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            } else {
                pyResult = Py_True;
                Py_INCREF(pyResult);
            }

            break;
        case Py_LT:
            if ((pythonToCpp = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], (pyArg)))) {
                // operator<(const ImageLayer & other) const
                if (!Shiboken::Object::isValid(pyArg))
                    return 0;
                ::ImageLayer cppArg0_local = ::ImageLayer(::QString(), ::QString(), ::QStringList());
                ::ImageLayer* cppArg0 = &cppArg0_local;
                if (Shiboken::Conversions::isImplicitConversion((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], pythonToCpp))
                    pythonToCpp(pyArg, &cppArg0_local);
                else
                    pythonToCpp(pyArg, &cppArg0);

                bool cppResult = cppSelf < (*cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            } else {
                goto Sbk_ImageLayer_RichComparison_TypeError;
            }

            break;
        case Py_EQ:
            if ((pythonToCpp = Shiboken::Conversions::isPythonToCppReferenceConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], (pyArg)))) {
                // operator==(const ImageLayer & other) const
                if (!Shiboken::Object::isValid(pyArg))
                    return 0;
                ::ImageLayer cppArg0_local = ::ImageLayer(::QString(), ::QString(), ::QStringList());
                ::ImageLayer* cppArg0 = &cppArg0_local;
                if (Shiboken::Conversions::isImplicitConversion((SbkObjectType*)SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], pythonToCpp))
                    pythonToCpp(pyArg, &cppArg0_local);
                else
                    pythonToCpp(pyArg, &cppArg0);

                bool cppResult = cppSelf == (*cppArg0);
                pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
            } else {
                pyResult = Py_False;
                Py_INCREF(pyResult);
            }

            break;
        default:
            goto Sbk_ImageLayer_RichComparison_TypeError;
    }

    if (pyResult && !PyErr_Occurred())
        return pyResult;
    Sbk_ImageLayer_RichComparison_TypeError:
    PyErr_SetString(PyExc_NotImplementedError, "operator not implemented.");
    return 0;

}

} // extern "C"

static Py_hash_t Sbk_ImageLayer_HashFunc(PyObject* self) {
    ::ImageLayer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::ImageLayer*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], (SbkObject*)self));
    return ImageLayer::getHash(*cppSelf);
}

static int Sbk_ImageLayer_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_ImageLayer_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_ImageLayer_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronEngine.ImageLayer",
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
    /*tp_hash*/             &Sbk_ImageLayer_HashFunc,
    /*tp_call*/             0,
    /*tp_str*/              0,
    /*tp_getattro*/         0,
    /*tp_setattro*/         0,
    /*tp_as_buffer*/        0,
    /*tp_flags*/            Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    /*tp_doc*/              0,
    /*tp_traverse*/         Sbk_ImageLayer_traverse,
    /*tp_clear*/            Sbk_ImageLayer_clear,
    /*tp_richcompare*/      Sbk_ImageLayer_richcompare,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_ImageLayer_methods,
    /*tp_members*/          0,
    /*tp_getset*/           0,
    /*tp_base*/             reinterpret_cast<PyTypeObject*>(&SbkObject_Type),
    /*tp_dict*/             0,
    /*tp_descr_get*/        0,
    /*tp_descr_set*/        0,
    /*tp_dictoffset*/       0,
    /*tp_init*/             Sbk_ImageLayer_Init,
    /*tp_alloc*/            0,
    /*tp_new*/              SbkObjectTpNew,
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


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void ImageLayer_PythonToCpp_ImageLayer_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_ImageLayer_Type, pyIn, cppOut);
}
static PythonToCppFunc is_ImageLayer_PythonToCpp_ImageLayer_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_ImageLayer_Type))
        return ImageLayer_PythonToCpp_ImageLayer_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* ImageLayer_PTR_CppToPython_ImageLayer(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::ImageLayer*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_ImageLayer_Type, const_cast<void*>(cppIn), false, false, typeName);
}

// C++ to Python copy conversion.
static PyObject* ImageLayer_COPY_CppToPython_ImageLayer(const void* cppIn) {
    return Shiboken::Object::newObject(&Sbk_ImageLayer_Type, new ::ImageLayer(*((::ImageLayer*)cppIn)), true, true);
}

// Python to C++ copy conversion.
static void ImageLayer_PythonToCpp_ImageLayer_COPY(PyObject* pyIn, void* cppOut) {
    *((::ImageLayer*)cppOut) = *((::ImageLayer*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_IMAGELAYER_IDX], (SbkObject*)pyIn));
}
static PythonToCppFunc is_ImageLayer_PythonToCpp_ImageLayer_COPY_Convertible(PyObject* pyIn) {
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_ImageLayer_Type))
        return ImageLayer_PythonToCpp_ImageLayer_COPY;
    return 0;
}

void init_ImageLayer(PyObject* module)
{
    SbkNatronEngineTypes[SBK_IMAGELAYER_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_ImageLayer_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "ImageLayer", "ImageLayer",
        &Sbk_ImageLayer_Type, &Shiboken::callCppDestructor< ::ImageLayer >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_ImageLayer_Type,
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
