.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
Frame ranges
============

.. toctree::
   :maxdepth: 2

The project frame range (in the Project Settings, key 's' in the Node Graph') is the range that will be used by default when rendering Writers.

Each clip (input or output of a node in the Node graph) also has its own frame range. This "clip frame range" may be used or even modified by plugins, such as Retime (which may change the frame range), Merge or Switch (which set the frame range to the union of their input frame ranges). The plugin may be able to render images outside of this frame range, and it is just an indication of a valid frame range. This information is available from the "Info" tab of the properties panel of each node.

Most generator plugins (e.g. CheckerBoard, ColorBars, ColorWheel, Constant, Solid) have a "Frame Range" parameter, which is (1,1) by default. The FrameRange plugin may be used to modify this frame range inside the graph.

The default framerange of an image sequence or video is the range of the sequence
