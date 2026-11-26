"""A file parameter refuses a path that uses [Project] before the project is saved (#1539).

Until the project is saved, the [Project] path has no value (a new project has no [Project] row), so
"[Project]/out.exr" is "/out.exr" in the root folder, or "out.exr" in a folder named "[Project]" in
the working directory. Setting such a path on a file parameter (FileParam or OutputFileParam, or
App.createReader()/createWriter()) from Python raises ValueError and keeps the
value of the parameter (no node is created). A PyPlug that sets such a path is still created: Natron
runs its script, as when it loads a project. Once the project is saved, [Project] is its folder and
the path is accepted.
Needs Tests/data/pyplugs in NATRON_PLUGIN_PATH (set by Tests/CMakeLists.txt).
"""
import os
import shutil
import tempfile

from NatronEngine import natron

import natronheadless

PYPLUG = "fr.natron.test.ProjectPath"

SETTERS = (
    ("setValue", lambda param, value: param.setValue(value)),
    ("set", lambda param, value: param.set(value)),
    ("setValueAtTime", lambda param, value: param.setValueAtTime(value, 1)),
    ("set(frame)", lambda param, value: param.set(value, 1)),
    ("setDefaultValue", lambda param, value: param.setDefaultValue(value)),
)


def expect_refused(param, value):
    """Checks that each setter raises ValueError for value and keeps the value of param."""
    before = param.getValue()
    for name, setter in SETTERS:
        try:
            setter(param, value)
        except ValueError as e:
            natronheadless.check("saved" in str(e), "%s(%r): unexpected message %r" % (name, value, str(e)))
        else:
            raise AssertionError("%s(%r) did not raise ValueError" % (name, value))
        natronheadless.check(param.getValue() == before, "%s(%r) changed the value to %r" % (name, value, param.getValue()))
        natronheadless.check(param.getDefaultValue() == "", "%s(%r) changed the default value" % (name, value))


def expect_no_node(app, create, value):
    """Checks that create(value) (App.createReader or createWriter) raises ValueError and creates no node."""
    count = len(app.getChildren())
    try:
        create(value)
    except ValueError as e:
        natronheadless.check("saved" in str(e), "%r: unexpected message %r" % (value, str(e)))
    else:
        raise AssertionError("creating a node for %r did not raise ValueError" % value)
    natronheadless.check(len(app.getChildren()) == count, "a node was created for %r" % value)


def main(app):
    """Checks the refusals in a new project, then that the paths are accepted once it is saved."""
    folder = tempfile.mkdtemp(prefix="natron-test-unsaved-project-path-")
    try:
        check_unsaved_then_saved(app, folder)
    finally:
        shutil.rmtree(folder, ignore_errors=True)


def check_unsaved_then_saved(app, folder):
    """The refusals in the new project, then the same paths accepted once it is saved in folder."""
    group = app.createNode("fr.inria.built-in.Group")
    params = (group.createFileParam("inFile", "In"), group.createOutputFileParam("outFile", "Out"))
    text = group.createStringParam("text", "Text")
    paths = app.getProjectParam("projectPaths")
    paths.setValue(paths.getValue() + "<Name>Shots</Name><Value>[Project]/shots</Value>")

    for param in params:
        # in the root folder, directly or through another project path
        expect_refused(param, "[Project]/out.exr")
        expect_refused(param, "[Shots]/out.####.exr")
        # other paths are accepted
        for value in ("/tmp/out.exr", "out.exr", "[OCIO]/config.ocio", ""):
            param.setValue(value)
            natronheadless.check(param.getValue() == value, "setValue(%r) gave %r" % (value, param.getValue()))
    expect_no_node(app, app.createReader, "[Project]/in.exr")
    expect_no_node(app, app.createWriter, "[Project]/out.####.exr")
    # a PyPlug sets the values it was saved with
    natronheadless.check(PYPLUG in natron.getPluginIDs(), "%s not found: is Tests/data/pyplugs in NATRON_PLUGIN_PATH?" % PYPLUG)
    pyplug = app.createNode(PYPLUG)
    natronheadless.check(pyplug is not None, "the PyPlug was not created")
    natronheadless.check(pyplug.getParam("outFile").getValue() == "[Project]/pyplug.exr", pyplug.getParam("outFile").getValue())
    # not a file parameter: any text
    text.setValue("[Project]/out.exr")
    natronheadless.check(text.getValue() == "[Project]/out.exr", text.getValue())

    # once the project is saved, [Project] is its folder
    natronheadless.check(app.saveProject(os.path.join(folder, "project.ntp")), "saveProject() failed")
    for param in params:
        for value in ("[Project]/out.exr", "[Shots]/out.####.exr"):
            param.setValue(value)
            natronheadless.check(param.getValue() == value, "saved project: setValue(%r) gave %r" % (value, param.getValue()))


natronheadless.run(main)
