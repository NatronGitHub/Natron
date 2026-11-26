# -*- coding: utf-8 -*-
# Natron Project
# Saved by Natron 2.6 (Python project format version 1).
# Open it with File > Open, or with NatronRenderer or app.loadProject(), as a .ntp project.

import NatronEngine


def createProject(app):
    # the nodes of the project are in app, its top-level group
    group = app

    # Project settings

    # Create all nodes in the group

    # Viewer "Viewer1"
    groupViewer1 = app.createNode("fr.inria.built-in.Viewer", 1, group)
    if groupViewer1 is not None:
        groupViewer1.setScriptName("Viewer1")
        groupViewer1.setPosition(100, 450)

    # Start of node "Dot1"
    lastNode = app.createNode("fr.inria.built-in.Dot", 1, group)
    lastNode.setScriptName("Dot1")
    lastNode.setLabel("Dot1")
    lastNode.setPosition(100, 200)
    lastNode.setSize(16, 16)
    lastNode.setColor(0.8, 0.2, 0.1)
    groupDot1 = lastNode

    del lastNode
    # End of node "Dot1"

    # Start of node "Dot2"
    lastNode = app.createNode("fr.inria.built-in.Dot", 1, group)
    lastNode.setScriptName("Dot2")
    lastNode.setLabel("Dot2")
    lastNode.setPosition(100, 320)
    lastNode.setSize(16, 16)
    lastNode.setColor(0.7, 0.7, 0.7)
    groupDot2 = lastNode

    del lastNode
    # End of node "Dot2"

    # Start of node "Backdrop1"
    lastNode = app.createNode("fr.inria.built-in.BackDrop", 1, group)
    lastNode.setScriptName("Backdrop1")
    lastNode.setLabel("Backdrop1")
    lastNode.setPosition(40, 150)
    lastNode.setSize(300, 260)
    lastNode.setColor(0.2, 0.4, 0.6)
    groupBackdrop1 = lastNode

    param = lastNode.getParam("Label")
    if param is not None:
        param.setValue("a backdrop")
        del param

    del lastNode
    # End of node "Backdrop1"

    # Start of node "Group1"
    lastNode = app.createNode("fr.inria.built-in.Group", 1, group)
    lastNode.setScriptName("Group1")
    lastNode.setLabel("Group1")
    lastNode.setPosition(420, 200)
    lastNode.setSize(104, 30)
    lastNode.setColor(0.5, 0.3, 0.7)
    groupGroup1 = lastNode

    del lastNode
    # End of node "Group1"

    groupgroup = groupGroup1
    # Create all nodes in the group

    # Create the parameters of the group node the same way we did for all internal nodes
    lastNode = groupgroup
    lastNode.setColor(0.5, 0.3, 0.7)
    del lastNode

    # Start of node "Output1"
    lastNode = app.createNode("fr.inria.built-in.Output", 1, groupgroup)
    lastNode.setLabel("Output")
    lastNode.setPosition(0, 0)
    lastNode.setSize(104, 30)
    lastNode.setColor(0.7, 0.7, 0.7)
    groupgroupOutput1 = lastNode

    del lastNode
    # End of node "Output1"

    # Start of node "Input1"
    lastNode = app.createNode("fr.inria.built-in.Input", 1, groupgroup)
    lastNode.setScriptName("Input1")
    lastNode.setLabel("Input1")
    lastNode.setPosition(0, -100)
    lastNode.setSize(104, 30)
    lastNode.setColor(0.3, 0.5, 0.2)
    groupgroupInput1 = lastNode

    del lastNode
    # End of node "Input1"

    # Start of node "Dot3"
    lastNode = app.createNode("fr.inria.built-in.Dot", 1, groupgroup)
    lastNode.setScriptName("Dot3")
    lastNode.setLabel("Dot1")
    lastNode.setPosition(10, 20)
    lastNode.setSize(16, 16)
    lastNode.setColor(0.1, 0.9, 0.3)
    groupgroupDot3 = lastNode

    del lastNode
    # End of node "Dot3"

    # Now that all nodes are created we can connect them together, restore expressions
    groupgroupOutput1.connectInput(0, groupgroupInput1)


    # Now that all nodes are created we can connect them together, restore expressions
    if groupViewer1 is not None:
        groupViewer1.connectInput(0, groupDot2)
    groupDot2.connectInput(0, groupDot1)

    # Project frame range
    param = app.getProjectParam("frameRange")
    if param is not None:
        param.setValue(1, 0)
        param.setValue(250, 1)
        del param


if __name__ == "__main__":
    createProject(app)
