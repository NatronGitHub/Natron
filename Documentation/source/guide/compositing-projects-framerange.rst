.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Frame ranges
============

The project frame range (**Frame Range** in the Project Settings, which open
with :kbd:`S` in the Node Graph) is the range that Write nodes render by
default.

Each node output also has its own frame range. By default, it is the union of
the frame ranges of the node's inputs, so a Merge or a Switch node covers all
its inputs. Some plug-ins change it: Retime, for example, can make a clip
longer or shorter. This range is only a hint: a node may still render images
outside of it. The **Info** tab of each node's properties shows its frame
range.

Most generators (for example CheckerBoard, ColorBars, ColorWheel, Constant and
Solid) have a **Frame Range** parameter, which is 1 to 1 by default. The
:ref:`FrameRange <net.sf.openfx.FrameRange>` node changes the frame range of a
clip inside the graph.

The frame range of a Read node is the range of the image sequence or video it
reads. Use its **First Frame** and **Last Frame** parameters to trim it, and
its **Before** and **After** parameters to choose what it outputs outside of
that range (see :doc:`thenodes-image-read`).
