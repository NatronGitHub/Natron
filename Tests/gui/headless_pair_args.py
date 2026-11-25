"""A std::pair argument may be any sequence of 2 items of the right types (#1811).

ChoiceParam.setOptions() takes a list of (option, help) pairs. A pair that was
neither a tuple nor a list (a str, or any other sequence) crashed Natron. A str
or bytes is now rejected with TypeError: "de" is a mistake for ("de", "").
Run by NatronRenderer -t (see Tests/CMakeLists.txt).
"""
import sys

import NatronEngine
from NatronEngine import natron


class Pair(object):
    """A sequence of 2 items that is neither a tuple nor a list."""

    def __init__(self, first, second):
        self._items = (first, second)

    def __len__(self):
        return 2

    def __getitem__(self, index):
        return self._items[index]


def work_around_shiboken2_on_python_3_14():
    """Lets shiboken2 raise TypeError for wrong argument types on Python >= 3.14.

    Shiboken builds the message of that TypeError from the function signatures.
    shiboken2 5.15 replaces inspect.formatannotation with a function that doesn't
    take the quote_annotation_strings argument passed by Python 3.14 (MSYS2
    UCRT64), so formatting fails and, for any Shiboken function called with
    arguments of the wrong type, the TypeError becomes a fatal Python error
    ("seterror_argument did not receive a result") before Natron's code runs.
    That is a separate bug; here, wrap the replacement so that the TypeError
    cases below run on every platform.
    """
    import inspect

    NatronEngine.ChoiceParam.setOptions.__signature__  # loads Shiboken's signature support

    def probe(value: int):
        pass

    try:
        str(inspect.signature(probe))
        return
    except TypeError:
        pass
    shiboken_formatannotation = inspect.formatannotation

    def formatannotation(annotation, base_module=None, **unused_kwargs):
        return shiboken_formatannotation(annotation, base_module)

    inspect.formatannotation = formatannotation
    str(inspect.signature(probe))
    print("Worked around shiboken2's inspect.formatannotation on Python %d.%d" % sys.version_info[:2])


def expect_type_error(choice, options):
    """Checks that choice.setOptions(options) raises TypeError."""
    try:
        choice.setOptions(options)
    except TypeError:
        return
    raise AssertionError("setOptions(%r) did not raise TypeError" % (options,))


status = 1
try:
    app = natron.getInstance(0)
    group = app.createNode("fr.inria.built-in.Group")
    choice = group.createChoiceParam("choice", "Choice")

    choice.setOptions([("a", "help a"), ["b", "help b"], Pair("c", "help c")])
    assert choice.getOptions() == ["a", "b", "c"], choice.getOptions()

    # Raised by Natron's pair conversion, not by Shiboken's argument check:
    # no workaround needed on Python >= 3.14.
    for options in (["de"], [("a", "help a"), "de"]):
        expect_type_error(choice, options)
    assert choice.getOptions() == ["a", "b", "c"], choice.getOptions()

    work_around_shiboken2_on_python_3_14()
    # Raised by Shiboken's argument check (b"de" is a sequence of 2 ints).
    for options in ([(1, 2)], [("a", 2)], [("a",)], [("a", "b", "c")], [None], [Pair("a", 2)], [b"de"]):
        expect_type_error(choice, options)
    assert choice.getOptions() == ["a", "b", "c"], choice.getOptions()
    status = 0
except BaseException:
    import traceback
    traceback.print_exc()
natron.quit(status)
