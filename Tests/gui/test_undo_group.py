# Undo groups (#1455): the Python edits made in "with app.undoGroup(text):" are one undo step on the
# node graph's undo stack; outside a group, Python edits stay non-undoable.
import NatronGuiTest
from natrontest import *

require_plugin(GENERATOR)

viewer = viewer_node(app)
history0 = app.getUndoHistory()

# outside a group: not undoable
gen0 = app.createNode(GENERATOR)
gen0.getParam("par").setValue(2.0)
check(app.getUndoHistory() == history0, "Python edits outside a group are not undoable: %r" % app.getUndoHistory())

# createNode / connect / setValue / setValueAtTime / delete in one (nested) group
with app.undoGroup("Bake"):
    gen = app.createNode(GENERATOR)
    name = gen.getScriptName()
    with app.undoGroup("Inner"):  # nested: no undo step of its own
        viewer.connectInput(0, gen)
        par = gen.getParam("par")
        par.setValue(1.5)
        par.setValueAtTime(3.0, 10)
    gen0.destroy()
wait_idle()

history = app.getUndoHistory()
check(len(history) == len(history0) + 1 and history[-1] == "Bake", "one undo step 'Bake': %r" % history)
check(app.getUndoIndex() == len(history), "the group is done")
check(app.getNode(gen0.getScriptName()) is None, "gen0 is deleted")

app.undo()
wait_idle()
check(app.getNode(name) is None, "undo removed the created node")
restored = app.getNode(gen0.getScriptName())
check(restored is not None, "undo restored the deleted node")
check(restored is not None and abs(restored.getParam("par").getValue() - 2.0) < 1e-9, "the deleted node has its value")
check(viewer.getInput(0) is None, "undo disconnected the viewer from the created node")

app.redo()
wait_idle()
gen = app.getNode(name)
check(gen is not None, "redo restored the created node")
check(same_node(viewer.getInput(0), gen), "redo reconnected the viewer")
par = gen.getParam("par")
check(abs(par.getValueAtTime(10) - 3.0) < 1e-9, "redo restored the keyframe at 10: %r" % par.getValueAtTime(10))
check(app.getNode(gen0.getScriptName()) is None, "redo deleted gen0 again")

# an exception inside the group ends it, and its edits are still one undo step
before = (par.getValueAtTime(1), par.getValueAtTime(10))
try:
    with app.undoGroup("Failing"):
        par.setValue(4.0)
        raise ValueError("expected")
except ValueError:
    pass
check(app.getUndoHistory()[-1] == "Failing", "the group ended on the exception: %r" % app.getUndoHistory())
app.undo()
wait_idle()
after = (par.getValueAtTime(1), par.getValueAtTime(10))
check(after == before, "undo restored the values before 'Failing': %r, expected %r" % (after, before))

# endUndoGroup() without a group raises
try:
    app.endUndoGroup()
    raise AssertionError("endUndoGroup() without a group did not raise")
except RuntimeError:
    pass
