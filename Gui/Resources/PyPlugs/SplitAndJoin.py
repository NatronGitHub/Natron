# -*- coding: utf-8 -*-
#Split and Join Toolset PyPlug
import NatronEngine

#To access selection in the NodeGraph
from NatronGui import *
import sys

#Specify that this is a toolset and not a regular PyPlug
def getIsToolset():
    return True

def getPluginID():
    return "fr.inria.SplitAndJoin"

def getLabel():
    return "Split and Join"

def getVersion():
    return 1

def getGrouping():
    return "Views"

def getPluginDescription():
    return "Automatically creates a one-view for each view in the project after the selected node and joins them."

def createInstance(app,group):
    #Since this is a toolset, group will be set to None
    
    views = app.getViewNames();
    nbviews = len(views)
    if nbviews <= 1:
        natron.warningDialog("Split and Join","The project must contain at least 2 views")
        return
    
    selectedNodes = app.getSelectedNodes();
    if len(selectedNodes) != 1:
        natron.warningDialog("Split and Join","You must select exactly one node")
        return

    selNode = selectedNodes[0]
    if selNode.isOutputNode():
        natron.warningDialog("Split and Join","The selected node must not be a viewer or an output node")
        return

    selectedNodePosition = selNode.getPosition()
    selectedNodeSize = selNode.getSize()


    totalWidth = nbviews * selectedNodeSize[0] + ((nbviews - 1) * selectedNodeSize[0] / 2)

    yposition = selectedNodePosition[1] + selectedNodeSize[1] * 3.
    xposition = selectedNodePosition[0] + selectedNodeSize[0] / 2. - totalWidth / 2.

    joinViewsNode = app.createNode("fr.inria.built-in.JoinViews")
    joinViewsNode.setPosition(selectedNodePosition[0], yposition + selectedNodeSize[1] * 3.)

    for i, v in enumerate(views):
        oneViewNode = app.createNode("fr.inria.built-in.OneView")
        oneViewNode.setLabel(v)
        oneViewNode.getParam("view").set(i)
        oneViewNode.setPosition(xposition,yposition)
        oneViewNode.connectInput(0,selNode)
        joinViewsNode.connectInput(nbviews - i - 1, oneViewNode)
        xposition += selectedNodeSize[0] * 1.5



