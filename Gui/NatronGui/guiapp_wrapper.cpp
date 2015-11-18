
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

#include "guiapp_wrapper.h"

// Extra includes
#include <AppInstanceWrapper.h>
#include <GuiAppWrapper.h>
#include <NodeGroupWrapper.h>
#include <NodeWrapper.h>
#include <ParameterWrapper.h>
#include <PythonPanels.h>
#include <list>
#include <vector>


// Native ---------------------------------------------------------

void GuiAppWrapper::pysideInitQtMetaTypes()
{
}

GuiAppWrapper::~GuiAppWrapper()
{
    SbkObject* wrapper = Shiboken::BindingManager::instance().retrieveWrapper(this);
    Shiboken::Object::destroy(wrapper, this);
}

// Target ---------------------------------------------------------

extern "C" {
static PyObject* Sbk_GuiAppFunc_clearSelection(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.clearSelection(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|O:clearSelection", &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: clearSelection(Group*)
    if (numArgs == 0) {
        overloadId = 0; // clearSelection(Group*)
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_GROUP_IDX], (pyArgs[0])))) {
        overloadId = 0; // clearSelection(Group*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GuiAppFunc_clearSelection_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "group");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.clearSelection(): got multiple values for keyword argument 'group'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_GROUP_IDX], (pyArgs[0]))))
                    goto Sbk_GuiAppFunc_clearSelection_TypeError;
            }
        }
        if (!Shiboken::Object::isValid(pyArgs[0]))
            return 0;
        ::Group* cppArg0 = 0;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // clearSelection(Group*)
            cppSelf->clearSelection(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_GuiAppFunc_clearSelection_TypeError:
        const char* overloads[] = {"NatronEngine.Group = None", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronGui.GuiApp.clearSelection", overloads);
        return 0;
}

static PyObject* Sbk_GuiAppFunc_createModalDialog(PyObject* self)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // createModalDialog()
            PyModalDialog * cppResult = cppSelf->createModalDialog();
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronGuiTypes[SBK_PYMODALDIALOG_IDX], cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_GuiAppFunc_deselectNode(PyObject* self, PyObject* pyArg)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: deselectNode(Effect*)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], (pyArg)))) {
        overloadId = 0; // deselectNode(Effect*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GuiAppFunc_deselectNode_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArg))
            return 0;
        ::Effect* cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // deselectNode(Effect*)
            cppSelf->deselectNode(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_GuiAppFunc_deselectNode_TypeError:
        const char* overloads[] = {"NatronEngine.Effect", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.GuiApp.deselectNode", overloads);
        return 0;
}

static PyObject* Sbk_GuiAppFunc_getDirectoryDialog(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.getDirectoryDialog(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|O:getDirectoryDialog", &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: getDirectoryDialog(std::string)const
    if (numArgs == 0) {
        overloadId = 0; // getDirectoryDialog(std::string)const
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))) {
        overloadId = 0; // getDirectoryDialog(std::string)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GuiAppFunc_getDirectoryDialog_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "location");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.getDirectoryDialog(): got multiple values for keyword argument 'location'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0]))))
                    goto Sbk_GuiAppFunc_getDirectoryDialog_TypeError;
            }
        }
        ::std::string cppArg0 = std::string();
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // getDirectoryDialog(std::string)const
            std::string cppResult = const_cast<const ::GuiApp*>(cppSelf)->getDirectoryDialog(cppArg0);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_GuiAppFunc_getDirectoryDialog_TypeError:
        const char* overloads[] = {"std::string = std.string()", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronGui.GuiApp.getDirectoryDialog", overloads);
        return 0;
}

static PyObject* Sbk_GuiAppFunc_getFilenameDialog(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.getFilenameDialog(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.getFilenameDialog(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:getFilenameDialog", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: getFilenameDialog(std::vector<std::string>,std::string)const
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_VECTOR_STD_STRING_IDX], (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // getFilenameDialog(std::vector<std::string>,std::string)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
            overloadId = 0; // getFilenameDialog(std::vector<std::string>,std::string)const
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GuiAppFunc_getFilenameDialog_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "location");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.getFilenameDialog(): got multiple values for keyword argument 'location'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1]))))
                    goto Sbk_GuiAppFunc_getFilenameDialog_TypeError;
            }
        }
        ::std::vector<std::string > cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1 = std::string();
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getFilenameDialog(std::vector<std::string>,std::string)const
            std::string cppResult = const_cast<const ::GuiApp*>(cppSelf)->getFilenameDialog(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_GuiAppFunc_getFilenameDialog_TypeError:
        const char* overloads[] = {"list, std::string = std.string()", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronGui.GuiApp.getFilenameDialog", overloads);
        return 0;
}

static PyObject* Sbk_GuiAppFunc_getRGBColorDialog(PyObject* self)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;

    // Call function/method
    {

        if (!PyErr_Occurred()) {
            // getRGBColorDialog()const
            ColorTuple* cppResult = new ColorTuple(const_cast<const ::GuiApp*>(cppSelf)->getRGBColorDialog());
            pyResult = Shiboken::Object::newObject((SbkObjectType*)SbkNatronEngineTypes[SBK_COLORTUPLE_IDX], cppResult, true, true);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;
}

static PyObject* Sbk_GuiAppFunc_getSelectedNodes(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.getSelectedNodes(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|O:getSelectedNodes", &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: getSelectedNodes(Group*)const
    if (numArgs == 0) {
        overloadId = 0; // getSelectedNodes(Group*)const
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_GROUP_IDX], (pyArgs[0])))) {
        overloadId = 0; // getSelectedNodes(Group*)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GuiAppFunc_getSelectedNodes_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "group");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.getSelectedNodes(): got multiple values for keyword argument 'group'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_GROUP_IDX], (pyArgs[0]))))
                    goto Sbk_GuiAppFunc_getSelectedNodes_TypeError;
            }
        }
        if (!Shiboken::Object::isValid(pyArgs[0]))
            return 0;
        ::Group* cppArg0 = 0;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // getSelectedNodes(Group*)const
            // Begin code injection

            std::list<Effect*> effects = cppSelf->getSelectedNodes(cppArg0);
            PyObject* ret = PyList_New((int) effects.size());
            int idx = 0;
            for (std::list<Effect*>::iterator it = effects.begin(); it!=effects.end(); ++it,++idx) {
            PyObject* item = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], *it);
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
        return 0;
    }
    return pyResult;

    Sbk_GuiAppFunc_getSelectedNodes_TypeError:
        const char* overloads[] = {"NatronEngine.Group = None", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronGui.GuiApp.getSelectedNodes", overloads);
        return 0;
}

static PyObject* Sbk_GuiAppFunc_getSequenceDialog(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.getSequenceDialog(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.getSequenceDialog(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:getSequenceDialog", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: getSequenceDialog(std::vector<std::string>,std::string)const
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_VECTOR_STD_STRING_IDX], (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // getSequenceDialog(std::vector<std::string>,std::string)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
            overloadId = 0; // getSequenceDialog(std::vector<std::string>,std::string)const
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GuiAppFunc_getSequenceDialog_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "location");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.getSequenceDialog(): got multiple values for keyword argument 'location'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1]))))
                    goto Sbk_GuiAppFunc_getSequenceDialog_TypeError;
            }
        }
        ::std::vector<std::string > cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1 = std::string();
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // getSequenceDialog(std::vector<std::string>,std::string)const
            std::string cppResult = const_cast<const ::GuiApp*>(cppSelf)->getSequenceDialog(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_GuiAppFunc_getSequenceDialog_TypeError:
        const char* overloads[] = {"list, std::string = std.string()", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronGui.GuiApp.getSequenceDialog", overloads);
        return 0;
}

static PyObject* Sbk_GuiAppFunc_getTabWidget(PyObject* self, PyObject* pyArg)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getTabWidget(std::string)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArg)))) {
        overloadId = 0; // getTabWidget(std::string)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GuiAppFunc_getTabWidget_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getTabWidget(std::string)const
            PyTabWidget * cppResult = const_cast<const ::GuiApp*>(cppSelf)->getTabWidget(cppArg0);
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_GuiAppFunc_getTabWidget_TypeError:
        const char* overloads[] = {"std::string", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.GuiApp.getTabWidget", overloads);
        return 0;
}

static PyObject* Sbk_GuiAppFunc_getUserPanel(PyObject* self, PyObject* pyArg)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getUserPanel(std::string)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArg)))) {
        overloadId = 0; // getUserPanel(std::string)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GuiAppFunc_getUserPanel_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getUserPanel(std::string)const
            PyPanel * cppResult = const_cast<const ::GuiApp*>(cppSelf)->getUserPanel(cppArg0);
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronGuiTypes[SBK_PYPANEL_IDX], cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_GuiAppFunc_getUserPanel_TypeError:
        const char* overloads[] = {"std::string", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.GuiApp.getUserPanel", overloads);
        return 0;
}

static PyObject* Sbk_GuiAppFunc_getViewer(PyObject* self, PyObject* pyArg)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: getViewer(std::string)const
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArg)))) {
        overloadId = 0; // getViewer(std::string)const
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GuiAppFunc_getViewer_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // getViewer(std::string)const
            PyViewer * cppResult = const_cast<const ::GuiApp*>(cppSelf)->getViewer(cppArg0);
            pyResult = Shiboken::Conversions::pointerToPython((SbkObjectType*)SbkNatronGuiTypes[SBK_PYVIEWER_IDX], cppResult);

            // Ownership transferences.
            Shiboken::Object::getOwnership(pyResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_GuiAppFunc_getViewer_TypeError:
        const char* overloads[] = {"std::string", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.GuiApp.getViewer", overloads);
        return 0;
}

static PyObject* Sbk_GuiAppFunc_moveTab(PyObject* self, PyObject* args)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "moveTab", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: moveTab(std::string,PyTabWidget*)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronGuiTypes[SBK_PYTABWIDGET_IDX], (pyArgs[1])))) {
        overloadId = 0; // moveTab(std::string,PyTabWidget*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GuiAppFunc_moveTab_TypeError;

    // Call function/method
    {
        ::std::string cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        if (!Shiboken::Object::isValid(pyArgs[1]))
            return 0;
        ::PyTabWidget* cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // moveTab(std::string,PyTabWidget*)
            bool cppResult = cppSelf->moveTab(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_GuiAppFunc_moveTab_TypeError:
        const char* overloads[] = {"std::string, NatronGui.PyTabWidget", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronGui.GuiApp.moveTab", overloads);
        return 0;
}

static PyObject* Sbk_GuiAppFunc_registerPythonPanel(PyObject* self, PyObject* args)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "registerPythonPanel", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: registerPythonPanel(PyPanel*,std::string)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronGuiTypes[SBK_PYPANEL_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
        overloadId = 0; // registerPythonPanel(PyPanel*,std::string)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GuiAppFunc_registerPythonPanel_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArgs[0]))
            return 0;
        ::PyPanel* cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // registerPythonPanel(PyPanel*,std::string)
            cppSelf->registerPythonPanel(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_GuiAppFunc_registerPythonPanel_TypeError:
        const char* overloads[] = {"NatronGui.PyPanel, std::string", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronGui.GuiApp.registerPythonPanel", overloads);
        return 0;
}

static PyObject* Sbk_GuiAppFunc_renderBlocking(PyObject* self, PyObject* args)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0, 0};

    // invalid argument lengths
    if (numArgs == 2)
        goto Sbk_GuiAppFunc_renderBlocking_TypeError;

    if (!PyArg_UnpackTuple(args, "renderBlocking", 1, 3, &(pyArgs[0]), &(pyArgs[1]), &(pyArgs[2])))
        return 0;


    // Overloaded function decisor
    // 0: renderBlocking(Effect*,int,int)
    // 1: renderBlocking(std::list<Effect*>,std::list<int>,std::list<int>)
    if (numArgs == 3
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[1])))
        && (pythonToCpp[2] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<int>(), (pyArgs[2])))) {
        overloadId = 0; // renderBlocking(Effect*,int,int)
    } else if (numArgs == 1
        && PyList_Check(pyArgs[0])) {
        overloadId = 1; // renderBlocking(std::list<Effect*>,std::list<int>,std::list<int>)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GuiAppFunc_renderBlocking_TypeError;

    // Call function/method
    switch (overloadId) {
        case 0: // renderBlocking(Effect * writeNode, int firstFrame, int lastFrame)
        {
            if (!Shiboken::Object::isValid(pyArgs[0]))
                return 0;
            ::Effect* cppArg0;
            pythonToCpp[0](pyArgs[0], &cppArg0);
            int cppArg1;
            pythonToCpp[1](pyArgs[1], &cppArg1);
            int cppArg2;
            pythonToCpp[2](pyArgs[2], &cppArg2);

            if (!PyErr_Occurred()) {
                // renderBlocking(Effect*,int,int)
                cppSelf->renderBlocking(cppArg0, cppArg1, cppArg2);
            }
            break;
        }
        case 1: // renderBlocking(const std::list<Effect * > & effects, const std::list<int > & firstFrames, const std::list<int > & lastFrames)
        {

            if (!PyErr_Occurred()) {
                // renderBlocking(std::list<Effect*>,std::list<int>,std::list<int>)
                // Begin code injection

                if (!PyList_Check(pyArgs[1-1])) {
                PyErr_SetString(PyExc_TypeError, "tasks must be a list of tuple objects.");
                return 0;
                }
                std::list<Effect*> effects;

                std::list<int> firstFrames;

                std::list<int> lastFrames;

                int size = (int)PyList_GET_SIZE(pyArgs[1-1]);
                for (int i = 0; i < size; ++i) {
                PyObject* tuple = PyList_GET_ITEM(pyArgs[1-1],i);
                if (!tuple) {
                PyErr_SetString(PyExc_TypeError, "tasks must be a list of tuple objects.");
                return 0;
                }
                if (PyTuple_GET_SIZE(pyArgs[1-1]) != 3) {
                PyErr_SetString(PyExc_TypeError, "the tuple must have exactly 3 items.");
                return 0;
                }
                ::Effect* writeNode = ((::Effect*)0);
                    Shiboken::Conversions::pythonToCppPointer((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], PyTuple_GET_ITEM(tuple, 0), &(writeNode));
                int firstFrame;
                    Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<int>(), PyTuple_GET_ITEM(tuple, 1), &(firstFrame));
                int lastFrame;
                    Shiboken::Conversions::pythonToCppCopy(Shiboken::Conversions::PrimitiveTypeConverter<int>(), PyTuple_GET_ITEM(tuple, 2), &(lastFrame));
                effects.push_back(writeNode);
                firstFrames.push_back(firstFrame);
                lastFrames.push_back(lastFrame);
                }

                cppSelf->renderBlocking(effects,firstFrames,lastFrames);

                // End of code injection


            }
            break;
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_GuiAppFunc_renderBlocking_TypeError:
        const char* overloads[] = {"NatronEngine.Effect, int, int", "list, list, list", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronGui.GuiApp.renderBlocking", overloads);
        return 0;
}

static PyObject* Sbk_GuiAppFunc_saveFilenameDialog(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.saveFilenameDialog(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.saveFilenameDialog(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:saveFilenameDialog", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: saveFilenameDialog(std::vector<std::string>,std::string)const
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_VECTOR_STD_STRING_IDX], (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // saveFilenameDialog(std::vector<std::string>,std::string)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
            overloadId = 0; // saveFilenameDialog(std::vector<std::string>,std::string)const
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GuiAppFunc_saveFilenameDialog_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "location");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.saveFilenameDialog(): got multiple values for keyword argument 'location'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1]))))
                    goto Sbk_GuiAppFunc_saveFilenameDialog_TypeError;
            }
        }
        ::std::vector<std::string > cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1 = std::string();
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // saveFilenameDialog(std::vector<std::string>,std::string)const
            std::string cppResult = const_cast<const ::GuiApp*>(cppSelf)->saveFilenameDialog(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_GuiAppFunc_saveFilenameDialog_TypeError:
        const char* overloads[] = {"list, std::string = std.string()", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronGui.GuiApp.saveFilenameDialog", overloads);
        return 0;
}

static PyObject* Sbk_GuiAppFunc_saveSequenceDialog(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    PyObject* pyResult = 0;
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 2) {
        PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.saveSequenceDialog(): too many arguments");
        return 0;
    } else if (numArgs < 1) {
        PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.saveSequenceDialog(): not enough arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|OO:saveSequenceDialog", &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: saveSequenceDialog(std::vector<std::string>,std::string)const
    if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_VECTOR_STD_STRING_IDX], (pyArgs[0])))) {
        if (numArgs == 1) {
            overloadId = 0; // saveSequenceDialog(std::vector<std::string>,std::string)const
        } else if ((pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1])))) {
            overloadId = 0; // saveSequenceDialog(std::vector<std::string>,std::string)const
        }
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GuiAppFunc_saveSequenceDialog_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "location");
            if (value && pyArgs[1]) {
                PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.saveSequenceDialog(): got multiple values for keyword argument 'location'.");
                return 0;
            } else if (value) {
                pyArgs[1] = value;
                if (!(pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), (pyArgs[1]))))
                    goto Sbk_GuiAppFunc_saveSequenceDialog_TypeError;
            }
        }
        ::std::vector<std::string > cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        ::std::string cppArg1 = std::string();
        if (pythonToCpp[1]) pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // saveSequenceDialog(std::vector<std::string>,std::string)const
            std::string cppResult = const_cast<const ::GuiApp*>(cppSelf)->saveSequenceDialog(cppArg0, cppArg1);
            pyResult = Shiboken::Conversions::copyToPython(Shiboken::Conversions::PrimitiveTypeConverter<std::string>(), &cppResult);
        }
    }

    if (PyErr_Occurred() || !pyResult) {
        Py_XDECREF(pyResult);
        return 0;
    }
    return pyResult;

    Sbk_GuiAppFunc_saveSequenceDialog_TypeError:
        const char* overloads[] = {"list, std::string = std.string()", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronGui.GuiApp.saveSequenceDialog", overloads);
        return 0;
}

static PyObject* Sbk_GuiAppFunc_selectAllNodes(PyObject* self, PyObject* args, PyObject* kwds)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0 };
    SBK_UNUSED(pythonToCpp)
    int numNamedArgs = (kwds ? PyDict_Size(kwds) : 0);
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0};

    // invalid argument lengths
    if (numArgs + numNamedArgs > 1) {
        PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.selectAllNodes(): too many arguments");
        return 0;
    }

    if (!PyArg_ParseTuple(args, "|O:selectAllNodes", &(pyArgs[0])))
        return 0;


    // Overloaded function decisor
    // 0: selectAllNodes(Group*)
    if (numArgs == 0) {
        overloadId = 0; // selectAllNodes(Group*)
    } else if ((pythonToCpp[0] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_GROUP_IDX], (pyArgs[0])))) {
        overloadId = 0; // selectAllNodes(Group*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GuiAppFunc_selectAllNodes_TypeError;

    // Call function/method
    {
        if (kwds) {
            PyObject* value = PyDict_GetItemString(kwds, "group");
            if (value && pyArgs[0]) {
                PyErr_SetString(PyExc_TypeError, "NatronGui.GuiApp.selectAllNodes(): got multiple values for keyword argument 'group'.");
                return 0;
            } else if (value) {
                pyArgs[0] = value;
                if (!(pythonToCpp[0] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_GROUP_IDX], (pyArgs[0]))))
                    goto Sbk_GuiAppFunc_selectAllNodes_TypeError;
            }
        }
        if (!Shiboken::Object::isValid(pyArgs[0]))
            return 0;
        ::Group* cppArg0 = 0;
        if (pythonToCpp[0]) pythonToCpp[0](pyArgs[0], &cppArg0);

        if (!PyErr_Occurred()) {
            // selectAllNodes(Group*)
            cppSelf->selectAllNodes(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_GuiAppFunc_selectAllNodes_TypeError:
        const char* overloads[] = {"NatronEngine.Group = None", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronGui.GuiApp.selectAllNodes", overloads);
        return 0;
}

static PyObject* Sbk_GuiAppFunc_selectNode(PyObject* self, PyObject* args)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp[] = { 0, 0 };
    SBK_UNUSED(pythonToCpp)
    int numArgs = PyTuple_GET_SIZE(args);
    PyObject* pyArgs[] = {0, 0};

    // invalid argument lengths


    if (!PyArg_UnpackTuple(args, "selectNode", 2, 2, &(pyArgs[0]), &(pyArgs[1])))
        return 0;


    // Overloaded function decisor
    // 0: selectNode(Effect*,bool)
    if (numArgs == 2
        && (pythonToCpp[0] = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronEngineTypes[SBK_EFFECT_IDX], (pyArgs[0])))
        && (pythonToCpp[1] = Shiboken::Conversions::isPythonToCppConvertible(Shiboken::Conversions::PrimitiveTypeConverter<bool>(), (pyArgs[1])))) {
        overloadId = 0; // selectNode(Effect*,bool)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GuiAppFunc_selectNode_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArgs[0]))
            return 0;
        ::Effect* cppArg0;
        pythonToCpp[0](pyArgs[0], &cppArg0);
        bool cppArg1;
        pythonToCpp[1](pyArgs[1], &cppArg1);

        if (!PyErr_Occurred()) {
            // selectNode(Effect*,bool)
            cppSelf->selectNode(cppArg0, cppArg1);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_GuiAppFunc_selectNode_TypeError:
        const char* overloads[] = {"NatronEngine.Effect, bool", 0};
        Shiboken::setErrorAboutWrongArguments(args, "NatronGui.GuiApp.selectNode", overloads);
        return 0;
}

static PyObject* Sbk_GuiAppFunc_setSelection(PyObject* self, PyObject* pyArg)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: setSelection(std::list<Effect*>)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppConvertible(SbkNatronGuiTypeConverters[SBK_NATRONGUI_STD_LIST_EFFECTPTR_IDX], (pyArg)))) {
        overloadId = 0; // setSelection(std::list<Effect*>)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GuiAppFunc_setSelection_TypeError;

    // Call function/method
    {
        ::std::list<Effect * > cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // setSelection(std::list<Effect*>)
            cppSelf->setSelection(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_GuiAppFunc_setSelection_TypeError:
        const char* overloads[] = {"list", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.GuiApp.setSelection", overloads);
        return 0;
}

static PyObject* Sbk_GuiAppFunc_unregisterPythonPanel(PyObject* self, PyObject* pyArg)
{
    ::GuiApp* cppSelf = 0;
    SBK_UNUSED(cppSelf)
    if (!Shiboken::Object::isValid(self))
        return 0;
    cppSelf = ((::GuiApp*)Shiboken::Conversions::cppPointer(SbkNatronGuiTypes[SBK_GUIAPP_IDX], (SbkObject*)self));
    int overloadId = -1;
    PythonToCppFunc pythonToCpp;
    SBK_UNUSED(pythonToCpp)

    // Overloaded function decisor
    // 0: unregisterPythonPanel(PyPanel*)
    if ((pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible((SbkObjectType*)SbkNatronGuiTypes[SBK_PYPANEL_IDX], (pyArg)))) {
        overloadId = 0; // unregisterPythonPanel(PyPanel*)
    }

    // Function signature not found.
    if (overloadId == -1) goto Sbk_GuiAppFunc_unregisterPythonPanel_TypeError;

    // Call function/method
    {
        if (!Shiboken::Object::isValid(pyArg))
            return 0;
        ::PyPanel* cppArg0;
        pythonToCpp(pyArg, &cppArg0);

        if (!PyErr_Occurred()) {
            // unregisterPythonPanel(PyPanel*)
            cppSelf->unregisterPythonPanel(cppArg0);
        }
    }

    if (PyErr_Occurred()) {
        return 0;
    }
    Py_RETURN_NONE;

    Sbk_GuiAppFunc_unregisterPythonPanel_TypeError:
        const char* overloads[] = {"NatronGui.PyPanel", 0};
        Shiboken::setErrorAboutWrongArguments(pyArg, "NatronGui.GuiApp.unregisterPythonPanel", overloads);
        return 0;
}

static PyMethodDef Sbk_GuiApp_methods[] = {
    {"clearSelection", (PyCFunction)Sbk_GuiAppFunc_clearSelection, METH_VARARGS|METH_KEYWORDS},
    {"createModalDialog", (PyCFunction)Sbk_GuiAppFunc_createModalDialog, METH_NOARGS},
    {"deselectNode", (PyCFunction)Sbk_GuiAppFunc_deselectNode, METH_O},
    {"getDirectoryDialog", (PyCFunction)Sbk_GuiAppFunc_getDirectoryDialog, METH_VARARGS|METH_KEYWORDS},
    {"getFilenameDialog", (PyCFunction)Sbk_GuiAppFunc_getFilenameDialog, METH_VARARGS|METH_KEYWORDS},
    {"getRGBColorDialog", (PyCFunction)Sbk_GuiAppFunc_getRGBColorDialog, METH_NOARGS},
    {"getSelectedNodes", (PyCFunction)Sbk_GuiAppFunc_getSelectedNodes, METH_VARARGS|METH_KEYWORDS},
    {"getSequenceDialog", (PyCFunction)Sbk_GuiAppFunc_getSequenceDialog, METH_VARARGS|METH_KEYWORDS},
    {"getTabWidget", (PyCFunction)Sbk_GuiAppFunc_getTabWidget, METH_O},
    {"getUserPanel", (PyCFunction)Sbk_GuiAppFunc_getUserPanel, METH_O},
    {"getViewer", (PyCFunction)Sbk_GuiAppFunc_getViewer, METH_O},
    {"moveTab", (PyCFunction)Sbk_GuiAppFunc_moveTab, METH_VARARGS},
    {"registerPythonPanel", (PyCFunction)Sbk_GuiAppFunc_registerPythonPanel, METH_VARARGS},
    {"renderBlocking", (PyCFunction)Sbk_GuiAppFunc_renderBlocking, METH_VARARGS},
    {"saveFilenameDialog", (PyCFunction)Sbk_GuiAppFunc_saveFilenameDialog, METH_VARARGS|METH_KEYWORDS},
    {"saveSequenceDialog", (PyCFunction)Sbk_GuiAppFunc_saveSequenceDialog, METH_VARARGS|METH_KEYWORDS},
    {"selectAllNodes", (PyCFunction)Sbk_GuiAppFunc_selectAllNodes, METH_VARARGS|METH_KEYWORDS},
    {"selectNode", (PyCFunction)Sbk_GuiAppFunc_selectNode, METH_VARARGS},
    {"setSelection", (PyCFunction)Sbk_GuiAppFunc_setSelection, METH_O},
    {"unregisterPythonPanel", (PyCFunction)Sbk_GuiAppFunc_unregisterPythonPanel, METH_O},

    {0} // Sentinel
};

} // extern "C"

static int Sbk_GuiApp_traverse(PyObject* self, visitproc visit, void* arg)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_traverse(self, visit, arg);
}
static int Sbk_GuiApp_clear(PyObject* self)
{
    return reinterpret_cast<PyTypeObject*>(&SbkObject_Type)->tp_clear(self);
}
// Class Definition -----------------------------------------------
extern "C" {
static SbkObjectType Sbk_GuiApp_Type = { { {
    PyVarObject_HEAD_INIT(&SbkObjectType_Type, 0)
    /*tp_name*/             "NatronGui.GuiApp",
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
    /*tp_traverse*/         Sbk_GuiApp_traverse,
    /*tp_clear*/            Sbk_GuiApp_clear,
    /*tp_richcompare*/      0,
    /*tp_weaklistoffset*/   0,
    /*tp_iter*/             0,
    /*tp_iternext*/         0,
    /*tp_methods*/          Sbk_GuiApp_methods,
    /*tp_members*/          0,
    /*tp_getset*/           0,
    /*tp_base*/             0,
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

static void* Sbk_GuiApp_typeDiscovery(void* cptr, SbkObjectType* instanceType)
{
    if (instanceType == reinterpret_cast<SbkObjectType*>(Shiboken::SbkType< ::Group >()))
        return dynamic_cast< ::GuiApp*>(reinterpret_cast< ::Group*>(cptr));
    return 0;
}


// Type conversion functions.

// Python to C++ pointer conversion - returns the C++ object of the Python wrapper (keeps object identity).
static void GuiApp_PythonToCpp_GuiApp_PTR(PyObject* pyIn, void* cppOut) {
    Shiboken::Conversions::pythonToCppPointer(&Sbk_GuiApp_Type, pyIn, cppOut);
}
static PythonToCppFunc is_GuiApp_PythonToCpp_GuiApp_PTR_Convertible(PyObject* pyIn) {
    if (pyIn == Py_None)
        return Shiboken::Conversions::nonePythonToCppNullPtr;
    if (PyObject_TypeCheck(pyIn, (PyTypeObject*)&Sbk_GuiApp_Type))
        return GuiApp_PythonToCpp_GuiApp_PTR;
    return 0;
}

// C++ to Python pointer conversion - tries to find the Python wrapper for the C++ object (keeps object identity).
static PyObject* GuiApp_PTR_CppToPython_GuiApp(const void* cppIn) {
    PyObject* pyOut = (PyObject*)Shiboken::BindingManager::instance().retrieveWrapper(cppIn);
    if (pyOut) {
        Py_INCREF(pyOut);
        return pyOut;
    }
    const char* typeName = typeid(*((::GuiApp*)cppIn)).name();
    return Shiboken::Object::newObject(&Sbk_GuiApp_Type, const_cast<void*>(cppIn), false, false, typeName);
}

void init_GuiApp(PyObject* module)
{
    SbkNatronGuiTypes[SBK_GUIAPP_IDX] = reinterpret_cast<PyTypeObject*>(&Sbk_GuiApp_Type);

    if (!Shiboken::ObjectType::introduceWrapperType(module, "GuiApp", "GuiApp*",
        &Sbk_GuiApp_Type, &Shiboken::callCppDestructor< ::GuiApp >, (SbkObjectType*)SbkNatronEngineTypes[SBK_APP_IDX])) {
        return;
    }

    // Register Converter
    SbkConverter* converter = Shiboken::Conversions::createConverter(&Sbk_GuiApp_Type,
        GuiApp_PythonToCpp_GuiApp_PTR,
        is_GuiApp_PythonToCpp_GuiApp_PTR_Convertible,
        GuiApp_PTR_CppToPython_GuiApp);

    Shiboken::Conversions::registerConverterName(converter, "GuiApp");
    Shiboken::Conversions::registerConverterName(converter, "GuiApp*");
    Shiboken::Conversions::registerConverterName(converter, "GuiApp&");
    Shiboken::Conversions::registerConverterName(converter, typeid(::GuiApp).name());
    Shiboken::Conversions::registerConverterName(converter, typeid(::GuiAppWrapper).name());


    Shiboken::ObjectType::setTypeDiscoveryFunctionV2(&Sbk_GuiApp_Type, &Sbk_GuiApp_typeDiscovery);


    GuiAppWrapper::pysideInitQtMetaTypes();
}
