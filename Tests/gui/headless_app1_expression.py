"""In expressions, app1.<node>.<param> works after a script rebinds the global "app" (#1251).

In background mode, the nodes are declared on "app". A script that rebinds it, e.g. with
app = natron.getInstance(0), gets another App object than app1: app1.<node> was not defined, and an
expression using it, as the GUI writes when linking parameters, failed and read 0 under
"NatronRenderer -t", while thisGroup.<node> and <node> worked.
"""

from NatronEngine import natron

import natronheadless

GROUP = "fr.inria.built-in.Group"


def check_expressions(target, source_name, expected):
    """Checks that the expressions reading the value of source_name from target give expected."""
    for prefix in ("app1.", "app.", "thisGroup.", ""):
        expression = "%s%s.value.get()" % (prefix, source_name)
        ok = target.setExpression(expression, False)
        assert ok, expression
        assert target.getValue() == expected, (expression, target.getValue())


def main(_):
    global app  # pylint: disable=global-statement
    app = natron.getInstance(0)  # rebinds the global: a new App object for the same project, not app1
    assert app is not app1

    source = app.createNode(GROUP)
    source.createDoubleParam("value", "Value").set(33.)
    name = source.getScriptName()
    assert getattr(app1, name) == source, name
    target = app.createNode(GROUP).createDoubleParam("result", "Result")
    check_expressions(target, name, 33.)

    # renaming and deleting the node updates the variables of both
    ok = source.setScriptName("Source")
    assert ok
    assert not hasattr(app1, name) and not hasattr(app, name), name
    check_expressions(target, "Source", 33.)
    source.destroy()
    assert not hasattr(app1, "Source") and not hasattr(app, "Source")


natronheadless.run(main)
