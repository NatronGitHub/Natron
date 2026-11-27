"""Using a Param whose parameter was deleted raises RuntimeError (#1615).

It returned 0, "" or False, or did nothing, so the script went on with wrong values; copy() and slaveTo() with a
deleted parameter crashed. The parameter is deleted by removeParam(), or with its node by Effect.destroy().
"""

import natronheadless

GROUP = "fr.inria.built-in.Group"


def check_raises(call, what):
    """Checks that call() raises the RuntimeError of a deleted parameter (not a SystemError, nor no error)."""
    try:
        call()
    except RuntimeError as e:
        assert "deleted" in str(e), (what, str(e))
        return
    raise AssertionError("%s did not raise RuntimeError" % what)


def check_deleted(param, other):
    """Checks that the functions of the deleted param raise; other is a live parameter of the same type."""
    checks = [
        ("getScriptName()", param.getScriptName),
        ("getNumDimensions()", param.getNumDimensions),
        ("getIsVisible()", param.getIsVisible),
        ("setVisible(False)", lambda: param.setVisible(False)),
        ("copy(other)", lambda: param.copy(other)),
        ("other.copy(param)", lambda: other.copy(param)),
        ("slaveTo(other, 0, 0)", lambda: param.slaveTo(other, 0, 0)),
    ]
    for what, call in checks:
        check_raises(call, what)


def main(app):
    group = app.createNode(GROUP)
    value = group.createIntParam("value", "Value")
    other = group.createIntParam("other", "Other")
    value.set(7)
    assert value.getValue() == 7  # a live parameter works as before
    removed = group.removeParam(value)
    assert removed
    check_deleted(value, other)
    check_raises(value.getValue, "getValue()")
    check_raises(lambda: value.setValue(5), "setValue(5)")
    check_raises(lambda: value.get(), "get()")
    check_raises(lambda: value.getDefaultValue(), "getDefaultValue()")
    # functions whose binding has its own code (typesystem_engine.xml)
    check_raises(lambda: value.getKeyTime(0, 0), "getKeyTime(0, 0)")
    check_raises(lambda: value.getExpression(0), "getExpression(0)")
    check_raises(lambda: value.setExpression("1", False), "setExpression()")
    # removing it again does nothing, as documented
    assert not group.removeParam(value)
    assert other.getValue() == 0  # the other parameters are still there

    path = group.createPathParam("path", "Path")
    parametric = group.createParametricParam("curves", "Curves", 2)
    assert group.removeParam(path) and group.removeParam(parametric)
    check_raises(path.getTable, "PathParam.getTable()")
    check_raises(lambda: path.setTable([["a"]]), "PathParam.setTable()")
    check_raises(lambda: parametric.getCurveColor(0), "ParametricParam.getCurveColor(0)")
    check_raises(lambda: parametric.getNthControlPoint(0, 0), "ParametricParam.getNthControlPoint(0, 0)")

    # the parameters of a destroyed node
    size = group.createDoubleParam("size", "Size")
    color = group.createColorParam("color", "Color", True)
    group.destroy()
    check_raises(size.getValue, "DoubleParam.getValue()")
    check_raises(lambda: size.setValue(1.), "DoubleParam.setValue(1.)")
    check_raises(color.get, "ColorParam.get()")


natronheadless.run(main)
