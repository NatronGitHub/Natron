"""Undo groups do nothing in background mode (App.undoGroup(), #1455)."""

import natronheadless


def main(app):
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


natronheadless.run(main)
