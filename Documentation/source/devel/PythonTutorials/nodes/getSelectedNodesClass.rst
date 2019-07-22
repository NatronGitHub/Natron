.. _getSelectedNodesClass:

Get selected nodes class
========================

::

    import os
    import NatronEngine
    from NatronGui import *

    def getSelectedNodesClass():

        # get current Natron instance running in memory
        app = natron.getGuiInstance(0)

        # get selected nodes
        selectedNodes = app.getSelectedNodes()

        # cycle through every selected node
        for currentNode in selectedNodes:

            # get current node class
            currentID = currentNode.getPluginID()

            # write node class in console
            os.write(1,'\n' + str(currentID) + '\n')

This script can now be saved in a .py file and added to Natron using the :func:`addMenuCommand(grouping,function)<NatronGui.PyGuiApplication.addMenuCommand>` function in the initGuy.py file.

It can also be can executed directly in Natron's script editor by adding::

  getSelectedNodesClass()

at the end of the script.
