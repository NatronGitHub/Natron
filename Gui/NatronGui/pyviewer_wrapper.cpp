
// default includes
#include "Global/Macros.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(missing-field-initializers)
GCC_DIAG_OFF(missing-declarations)
#include <shiboken.h> // produces many warnings
#include <pysidesignal.h>
#include <pysideproperty.h>
#include <pyside.h>
#include <typeresolver.h>
#include <typeinfo>
#include "natrongui_python.h"

#include "pyviewer_wrapper.h"

// Extra includes
NATRON_NAMESPACE_USING



// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_PyViewerFunc_getAInput(PyObject* self)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getAInput()const
            int cppResult = const_cast<const ::PyViewer*>(cppSelf)->getAInput();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyViewerFunc_getBInput(PyObject* self)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getBInput()const
            int cppResult = const_cast<const ::PyViewer*>(cppSelf)->getBInput();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyViewerFunc_getChannels(PyObject* self)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getChannels()const
            DisplayChannelsEnum cppResult = const_cast<const ::PyViewer*>(cppSelf)->getChannels();
            pyResult = Shiboken::Conversions::copyToPython(SBK_CONVERTER(SbkNatronEngineTypes[SBK_DISPLAYCHANNELSENUM_IDX]), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyViewerFunc_getCompositingOperator(PyObject* self)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getCompositingOperator()const
            ViewerCompositingOperatorEnum cppResult = const_cast<const ::PyViewer*>(cppSelf)->getCompositingOperator();
            pyResult = Shiboken::Conversions::copyToPython(SBK_CONVERTER(SbkNatronEngineTypes[SBK_VIEWERCOMPOSITINGOPERATORENUM_IDX]), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyViewerFunc_getCurrentFrame(PyObject* self)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getCurrentFrame()
            int cppResult = cppSelf->getCurrentFrame();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyViewerFunc_getCurrentView(PyObject* self)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getCurrentView()const
            int cppResult = const_cast<const ::PyViewer*>(cppSelf)->getCurrentView();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyViewerFunc_getFrameRange(PyObject* self)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getFrameRange(int*,int*)const
            // Begin code injection

            int x,y;
            cppSelf->getFrameRange(&x,&y);
            pyResult = PyTuple_New(2);
            PyTuple_SET_ITEM(pyResult, 0, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &x));
            PyTuple_SET_ITEM(pyResult, 1, Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &y));
            return pyResult;

            // End of code injection


        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyViewerFunc_getPlaybackMode(PyObject* self)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getPlaybackMode()const
            PlaybackModeEnum cppResult = const_cast<const ::PyViewer*>(cppSelf)->getPlaybackMode();
            pyResult = Shiboken::Conversions::copyToPython(SBK_CONVERTER(SbkNatronEngineTypes[SBK_PLAYBACKMODEENUM_IDX]), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyViewerFunc_getProxyIndex(PyObject* self)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getProxyIndex()const
            int cppResult = const_cast<const ::PyViewer*>(cppSelf)->getProxyIndex();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<int>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyViewerFunc_isProxyModeEnabled(PyObject* self)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // isProxyModeEnabled()const
            bool cppResult = const_cast<const ::PyViewer*>(cppSelf)->isProxyModeEnabled();
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_PyViewerFunc_pause(PyObject* self)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // pause()
            cppSelf->pause();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_PyViewerFunc_redraw(PyObject* self)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // redraw()
            cppSelf->redraw();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_PyViewerFunc_renderCurrentFrame(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronGui.PyViewer.renderCurrentFrame(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|O:renderCurrentFrame", &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: renderCurrentFrame(bool)
    if (numArgs == 0) {
        overloadId = 0; // renderCurrentFrame(bool)
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[0])))) {
        overloadId = 0; // renderCurrentFrame(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyViewerFunc_renderCurrentFrame_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "useCache");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronGui.PyViewer.renderCurrentFrame(): got multiple values for keyword argument 'useCache'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[0]))))
                    goto Sbk_PyViewerFunc_renderCurrentFrame_TypeError;
            }
        }
        bool cppArg0 = true;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // renderCurrentFrame(bool)
            cppSelf->renderCurrentFrame(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyViewerFunc_renderCurrentFrame_TypeError:
        const char* overloads[] = {"bool = true", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronGui.PyViewer.renderCurrentFrame", overloads);
        return 0;
}

static PyObject* Sbk_PyViewerFunc_seek(PyObject* self, PyObject* pyArg)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: seek(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // seek(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyViewerFunc_seek_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // seek(int)
            cppSelf->seek(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyViewerFunc_seek_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyViewer.seek", overloads);
        return 0;
}

static PyObject* Sbk_PyViewerFunc_setAInput(PyObject* self, PyObject* pyArg)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setAInput(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // setAInput(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyViewerFunc_setAInput_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setAInput(int)
            cppSelf->setAInput(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyViewerFunc_setAInput_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyViewer.setAInput", overloads);
        return 0;
}

static PyObject* Sbk_PyViewerFunc_setBInput(PyObject* self, PyObject* pyArg)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setBInput(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // setBInput(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyViewerFunc_setBInput_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setBInput(int)
            cppSelf->setBInput(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyViewerFunc_setBInput_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyViewer.setBInput", overloads);
        return 0;
}

static PyObject* Sbk_PyViewerFunc_setChannels(PyObject* self, PyObject* pyArg)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setChannels(DisplayChannelsEnum)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SBK_CONVERTER(SbkNatronEngineTypes[SBK_DISPLAYCHANNELSENUM_IDX]), (pyArg)))) {
        overloadId = 0; // setChannels(DisplayChannelsEnum)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyViewerFunc_setChannels_TypeError;

    // Call function/method
    {
        ::DisplayChannelsEnum cppArg0 = ((::DisplayChannelsEnum)0);
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setChannels(DisplayChannelsEnum)
            cppSelf->setChannels(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyViewerFunc_setChannels_TypeError:
        const char* overloads[] = {"NatronEngine.DisplayChannelsEnum", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyViewer.setChannels", overloads);
        return 0;
}

static PyObject* Sbk_PyViewerFunc_setCompositingOperator(PyObject* self, PyObject* pyArg)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setCompositingOperator(ViewerCompositingOperatorEnum)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SBK_CONVERTER(SbkNatronEngineTypes[SBK_VIEWERCOMPOSITINGOPERATORENUM_IDX]), (pyArg)))) {
        overloadId = 0; // setCompositingOperator(ViewerCompositingOperatorEnum)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyViewerFunc_setCompositingOperator_TypeError;

    // Call function/method
    {
        ::ViewerCompositingOperatorEnum cppArg0 = ((::ViewerCompositingOperatorEnum)0);
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setCompositingOperator(ViewerCompositingOperatorEnum)
            cppSelf->setCompositingOperator(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyViewerFunc_setCompositingOperator_TypeError:
        const char* overloads[] = {"NatronEngine.ViewerCompositingOperatorEnum", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyViewer.setCompositingOperator", overloads);
        return 0;
}

static PyObject* Sbk_PyViewerFunc_setCurrentView(PyObject* self, PyObject* pyArg)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setCurrentView(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // setCurrentView(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyViewerFunc_setCurrentView_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setCurrentView(int)
            cppSelf->setCurrentView(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyViewerFunc_setCurrentView_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyViewer.setCurrentView", overloads);
        return 0;
}

static PyObject* Sbk_PyViewerFunc_setFrameRange(PyObject* self, PyObject* args)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "setFrameRange", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: setFrameRange(int,int)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))) {
        overloadId = 0; // setFrameRange(int,int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyViewerFunc_setFrameRange_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        int cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // setFrameRange(int,int)
            cppSelf->setFrameRange(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyViewerFunc_setFrameRange_TypeError:
        const char* overloads[] = {"int, int", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronGui.PyViewer.setFrameRange", overloads);
        return 0;
}

static PyObject* Sbk_PyViewerFunc_setPlaybackMode(PyObject* self, PyObject* pyArg)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setPlaybackMode(PlaybackModeEnum)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SBK_CONVERTER(SbkNatronEngineTypes[SBK_PLAYBACKMODEENUM_IDX]), (pyArg)))) {
        overloadId = 0; // setPlaybackMode(PlaybackModeEnum)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyViewerFunc_setPlaybackMode_TypeError;

    // Call function/method
    {
        ::PlaybackModeEnum cppArg0 = ((::PlaybackModeEnum)0);
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setPlaybackMode(PlaybackModeEnum)
            cppSelf->setPlaybackMode(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyViewerFunc_setPlaybackMode_TypeError:
        const char* overloads[] = {"NatronEngine.PlaybackModeEnum", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyViewer.setPlaybackMode", overloads);
        return 0;
}

static PyObject* Sbk_PyViewerFunc_setProxyIndex(PyObject* self, PyObject* pyArg)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setProxyIndex(int)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArg)))) {
        overloadId = 0; // setProxyIndex(int)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyViewerFunc_setProxyIndex_TypeError;

    // Call function/method
    {
        int cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setProxyIndex(int)
            cppSelf->setProxyIndex(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyViewerFunc_setProxyIndex_TypeError:
        const char* overloads[] = {"int", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyViewer.setProxyIndex", overloads);
        return 0;
}

static PyObject* Sbk_PyViewerFunc_setProxyModeEnabled(PyObject* self, PyObject* pyArg)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setProxyModeEnabled(bool)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArg)))) {
        overloadId = 0; // setProxyModeEnabled(bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_PyViewerFunc_setProxyModeEnabled_TypeError;

    // Call function/method
    {
        bool cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setProxyModeEnabled(bool)
            cppSelf->setProxyModeEnabled(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_PyViewerFunc_setProxyModeEnabled_TypeError:
        const char* overloads[] = {"bool", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.PyViewer.setProxyModeEnabled", overloads);
        return 0;
}

static PyObject* Sbk_PyViewerFunc_startBackward(PyObject* self)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // startBackward()
            cppSelf->startBackward();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyObject* Sbk_PyViewerFunc_startForward(PyObject* self)
{
    ::PyViewer* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::PyViewer*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_PYVIEWER_IDX], (SbkObject*)self));

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // startForward()
            cppSelf->startForward();
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;
}

static PyMethodDef Sbk_PyViewer_methods[] = {
    {"getAInput", (PyCFunction)Sbk_PyViewerFunc_getAInput, METH_NOARGS},
    {"getBInput", (PyCFunction)Sbk_PyViewerFunc_getBInput, METH_NOARGS},
    {"getChannels", (PyCFunction)Sbk_PyViewerFunc_getChannels, METH_NOARGS},
    {"getCompositingOperator", (PyCFunction)Sbk_PyViewerFunc_getCompositingOperator, METH_NOARGS},
    {"getCurrentFrame", (PyCFunction)Sbk_PyViewerFunc_getCurrentFrame, METH_NOARGS},
    {"getCurrentView", (PyCFunction)Sbk_PyViewerFunc_getCurrentView, METH_NOARGS},
    {"getFrameRange", (PyCFunction)Sbk_PyViewerFunc_getFrameRange, METH_NOARGS},
    {"getPlaybackMode", (PyCFunction)Sbk_PyViewerFunc_getPlaybackMode, METH_NOARGS},
    {"getProxyIndex", (PyCFunction)Sbk_PyViewerFunc_getProxyIndex, METH_NOARGS},
    {"isProxyModeEnabled", (PyCFunction)Sbk_PyViewerFunc_isProxyModeEnabled, METH_NOARGS},
    {"pause", (PyCFunction)Sbk_PyViewerFunc_pause, METH_NOARGS},
    {"redraw", (PyCFunction)Sbk_PyViewerFunc_redraw, METH_NOARGS},
    {"renderCurrentFrame", (PyCFunction)Sbk_PyViewerFunc_renderCurrentFrame, METH_VARARGS|METH_KEYWORDS},
    {"seek", (PyCFunction)Sbk_PyViewerFunc_seek, METH_O},
    {"setAInput", (PyCFunction)Sbk_PyViewerFunc_setAInput, METH_O},
    {"setBInput", (PyCFunction)Sbk_PyViewerFunc_setBInput, METH_O},
    {"setChannels", (PyCFunction)Sbk_PyViewerFunc_setChannels, METH_O},
    {"setCompositingOperator", (PyCFunction)Sbk_PyViewerFunc_setCompositingOperator, METH_O},
    {"setCurrentView", (PyCFunction)Sbk_PyViewerFunc_setCurrentView, METH_O},
    {"setFrameRange", (PyCFunction)Sbk_PyViewerFunc_setFrameRange, METH_VARARGS},
    {"setPlaybackMode", (PyCFunction)Sbk_PyViewerFunc_setPlaybackMode, METH_O},
    {"setProxyIndex", (PyCFunction)Sbk_PyViewerFunc_setProxyIndex, METH_O},
    {"setProxyModeEnabled", (PyCFunction)Sbk_PyViewerFunc_setProxyModeEnabled, METH_O},
    {"startBackward", (PyCFunction)Sbk_PyViewerFunc_startBackward, METH_NOARGS},
    {"startForward", (PyCFunction)Sbk_PyViewerFunc_startForward, METH_NOARGS},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_PyViewer_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_PyViewer_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_PyViewer_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronGui.PyViewer",
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
    /*tp_hash*/             0,
    /*tp_call*/             0,
    /*tp_str*/              0,
    /*tp_getattro*/         0,
    /*tp_setattro*/         0,
    /*tp_as_buffer*/        0,
    /*tp_flags*/            Py_TPFLAGS_DEFAULT|Py_TPFLAGS_CHECKTYPES|Py_TPFLAGS_HAVE_GC,
    /*tp_doc*/              0,
    /*tp_traverse*/         Sbk_PyViewer_traverse,
    /*tp_clear*/            Sbk_PyViewer_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_PyViewer_methods,
    /*tp_members*/          0,
    /*tp_getset*/           0,
    /*tp_base*/             reinterpret_cast<PyTypeObject*>(&SbkObject_Type),
    /*tp_dict*/             0,
    /*tp_descr_get*/        0,
    /*tp_descr_set*/        0,
    /*tp_dictoffset*/       0,
    /*tp_init*/             0,
    /*tp_alloc*/            0,
    /*tp_new*/              0,
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
static void PyViewer_PythonToCpp_PyViewer_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_PyViewer_Type, pyIn, cppOut);
}
static PythonToCppFunc is_PyViewer_PythonToCpp_PyViewer_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_PyViewer_Type))
        return PyViewer_PythonToCpp_PyViewer_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* PyViewer_PTR_CppToPython_PyViewer(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::PyViewer*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_PyViewer_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_PyViewer(PyObject* module)
{
    SbkNatronGuiTypes[SBK_PYVIEWER_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_PyViewer_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "PyViewer", "PyViewer*",
        &Sbk_PyViewer_Type, &Shiboken::callCppDestructor< ::PyViewer >)) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_PyViewer_Type,
        PyViewer_PythonToCpp_PyViewer_PTR,
        is_PyViewer_PythonToCpp_PyViewer_PTR_Convertible,
        PyViewer_PTR_CppToPython_PyViewer);

    Shiboken::Conversions::registerConverterName(converter, "PyViewer");
    Shiboken::Conversions::registerConverterName(converter, "PyViewer*");
    Shiboken::Conversions::registerConverterName(converter, "PyViewer&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::PyViewer).name());



}
