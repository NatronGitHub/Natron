# Regression test for #1722: the Reload button of a File parameter (and the Rewrite button of an Output File
# parameter) used the old path when a new one was typed in the line edit but not committed yet (no Return):
# the buttons take no focus, so the line edit doesn't emit editingFinished() when they are clicked.
from natrontest import *

QtCore, QtGui, QtWidgets = qt_modules()


def param_widgets(param_name, class_name):
    """The widgets of a parameter in the open settings panels, by their paramName property and class."""
    return [w for w in QtWidgets.QApplication.allWidgets()
            if w.property("paramName") == param_name and w.metaObject().className() == class_name]


def line_edit(param_name):
    edits = param_widgets(param_name, "Natron::LineEdit")
    check(len(edits) == 1, "%d line edits for %s" % (len(edits), param_name))
    return edits[0]


def button(param_name, tooltip_word):
    # Natron::Button has no Q_OBJECT: its metaObject() is the one of QPushButton
    buttons = [b for b in param_widgets(param_name, "QPushButton") if tooltip_word in b.toolTip()]
    check(len(buttons) == 1, "%d %s buttons for %s" % (len(buttons), tooltip_word, param_name))
    return buttons[0]


def type_and_click(param, param_name, tooltip_word, new_path):
    """Type new_path in the line edit without committing it, then click the button."""
    old = param.getValue()
    edit = line_edit(param_name)
    edit.setText(new_path)  # no editingFinished(): the path is not committed
    wait_idle()
    check(param.getValue() == old, "%s: setText() committed the path" % param_name)
    button(param_name, tooltip_word).click()
    wait_idle()
    check(param.getValue() == new_path,
          "%s: %s used %r, not the typed path %r" % (param_name, tooltip_word, param.getValue(), new_path))


g = app.createNode("fr.inria.built-in.Group")
check(g is not None, "createNode(Group) failed")
input_file = g.createFileParam("natronTestInputFile", "Input file")
input_file.setValue("/tmp/natron-test-old-input.png")
output_file = g.createOutputFileParam("natronTestOutputFile", "Output file")
output_file.setValue("/tmp/natron-test-old-output.png")
g.refreshUserParamsGUI()
open_settings_panel(g)

type_and_click(input_file, "natronTestInputFile", "Reload", "/tmp/natron-test-new-input.png")
type_and_click(output_file, "natronTestOutputFile", "Rewrite", "/tmp/natron-test-new-output.png")

# clicking again without typing keeps the path
button("natronTestInputFile", "Reload").click()
button("natronTestOutputFile", "Rewrite").click()
wait_idle()
check(input_file.getValue() == "/tmp/natron-test-new-input.png", "Reload changed the path: %r" % input_file.getValue())
check(output_file.getValue() == "/tmp/natron-test-new-output.png", "Rewrite changed the path: %r" % output_file.getValue())
