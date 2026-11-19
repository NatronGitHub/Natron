.. _getRotoItemsLabel:

Get items label in a Roto node
==============================

::

  import NatronEngine
  from NatronGui import *

  def getRotoItemsLabel():

    # get current Natron instance running in memory
    app = natron.getGuiInstance(0)

    # get selected nodes
    selectedNodes = app.getSelectedNodes()

    # cycle every selected node #
    for currentNode in selectedNodes:

      # get node class
      currentID = currentNode.getPluginID()

      # check if selected node(s) are of 'Roto' class
      if currentID == "fr.inria.built-in.Roto" or currentID == "fr.inria.built-in.RotoPaint" :

        # get 'Roto' context
        rotoContext = currentNode.getRotoContext()

        # get 'Base layer' (root layer) in 'Roto' context
        baseLayer = rotoContext.getBaseLayer()

        # get all items in 'Base layer'
        allBaseLayerItems = baseLayer.getChildren()

        # cycle every item in 'Base layer'
        for item in allBaseLayerItems:

          # get current item label
          itemName = item.getLabel()

          print(itemName)

This script can now be saved in a .py file and added to Natron using the :func:`addMenuCommand(grouping,function)<NatronGui.PyGuiApplication.addMenuCommand>` function in the initGui.py file.

It can also be executed directly in Natron's script editor by adding::

  getRotoItemsLabel()

at the end of the script.
