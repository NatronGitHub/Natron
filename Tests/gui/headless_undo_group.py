# Undo groups are no-ops in background mode (#1455): run by NatronRenderer -t (see Tests/CMakeLists.txt).
import NatronEngine
from NatronEngine import natron

status = 1
try:
    app = natron.getInstance(0)
    with app.undoGroup("Headless"):
        with app.undoGroup("Nested"):
            n = app.createNode("fr.inria.built-in.Dot")
    assert n is not None
    assert app.getUndoHistory() == [], app.getUndoHistory()
    assert not app.canUndo() and not app.canRedo()
    app.undo()  # does nothing
    assert app.getNode(n.getScriptName()) is not None
    assert app.waitForRenders(10)
    try:
        app.endUndoGroup()
        raise AssertionError("endUndoGroup() without a group did not raise")
    except RuntimeError:
        pass
    status = 0
except BaseException:
    import traceback
    traceback.print_exc()
natron.quit(status)
