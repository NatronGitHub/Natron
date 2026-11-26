# Regression test for #1137: the menu of a Choice parameter shows its options in the plug-in's
# kOfxParamPropChoiceOrder (OFX 1.5), and the value stays the index of the option in the plug-in's list.
# The "ordered" parameter of net.natron.test.ChoiceOrder (Tests/OfxTestPlugin/NatronTestPluginChoice.cpp) has
# the options OptA, OptB, OptC, NewOpt with the order 0, 10, 30, 20: the menu is OptA, OptB, NewOpt, OptC.
# Selecting each menu item (ComboBox::setCurrentIndex(), as a click in the menu does) checks both the menu order
# and the mapping from menu items to values (KnobGuiChoice::onCurrentIndexChanged()).
import NatronGuiTest
from natrontest import *

QtCore, QtGui, QtWidgets = qt_modules()

CHOICE_ORDER = "net.natron.test.ChoiceOrder"
require_plugin(CHOICE_ORDER)


def find_combobox(param_name):
    """The menu of a Choice parameter in the open settings panels, by its paramName property."""
    boxes = [w for w in QtWidgets.QApplication.allWidgets()
             if w.property("paramName") == param_name
             and w.metaObject().className() == "Natron::ComboBox"]  # KnobComboBox has no Q_OBJECT
    check(len(boxes) == 1, "%d menus for %s" % (len(boxes), param_name))
    return boxes[0]


class MenuSelector(QtCore.QObject):
    """Calls the setCurrentIndex(int) slot of a ComboBox, which has no Python bindings."""
    select = QtCore.Signal(int)

    def __init__(self, widget):
        super(MenuSelector, self).__init__()
        check(QtCore.QObject.connect(self, QtCore.SIGNAL("select(int)"), widget, QtCore.SLOT("setCurrentIndex(int)")),
              "cannot connect to ComboBox::setCurrentIndex(int)")


def select_menu_item(selector, index):
    """Select the menu item index, as a click in the menu does (keys need the keyboard focus)."""
    selector.select.emit(index)
    wait_idle()


node = app.createNode(CHOICE_ORDER)
check(node is not None, "createNode(%s) failed" % CHOICE_ORDER)
open_settings_panel(node)

ordered = node.getParam("ordered")
selector = MenuSelector(find_combobox("ordered"))
check(ordered.getValue() == 0, "initial value: %d" % ordered.getValue())

# menu items: OptA, OptB, NewOpt, OptC; values (option indices): 0, 1, 3, 2
for item, value in ((1, 1), (2, 3), (3, 2), (0, 0), (2, 3)):
    select_menu_item(selector, item)
    print("menu item %d: value %d (%s)" % (item, ordered.getValue(), ordered.getOption(ordered.getValue())))
    check(ordered.getValue() == value, "the menu item %d selected the option %d instead of %d" % (item, ordered.getValue(), value))
