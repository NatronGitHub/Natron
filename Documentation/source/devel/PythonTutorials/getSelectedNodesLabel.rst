.. _getSelectedNodesLabel:

Get selected nodes label
========================

::

  import os
  import NatronEngine
  from NatronGui import *

  def getSelectedNodesLabel():

    # get current Natron instance running in memory
    app = natron.getGuiInstance(0)

    # get selected nodes
    selectedNodes = app.getSelectedNodes()

    # cycle through every selected node
    for currentNode in selectedNodes:

      # get current node label
      currentLabel = currentNode.getLabel()

      # write node label in console
      os.write(1,'\n' + str(currentLabel) + '\n')

This script can now be saved in a .py file and added to Natron using the :func:`addMenuCommand(grouping,function)<NatronGui.PyGuiApplication.addMenuCommand>` function in the initGuy.py file.

It can also be can executed directly in Natron's script editor by adding::

  getSelectedNodesLabel()

at the end of the script.
