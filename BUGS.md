Bugs and feature requests
=========================

Bugs
----

Here is a list of know bugs, ordered by priority from high to low:

- Some plugins crash using the multi-thread suite (mainly Furnace) when the implementation is based on QtConcurrent

- messages (and persistent messages) should be associated with an instance, not with a viewer. The message should appear in the viewer if and only of the given instance is in the the viewer's parents.

- black borders around the viewer when zooming out and multithreading is disabled

- Tab traversal is not set up properly, and sometimes couter-intuitive.
  Many users still have a keyboard and use it, so this should be fixed.
  Refs:
  http://qt-project.org/doc/qt-4.8/qwidget.html#setTabOrder
  http://qt-project.org/doc/qt-4.8/focus.html


Missing and wanted features
---------------------------

Here is a list of non-blocking bugs / wanted features:

- the nodegraph should have scrollbars (or a birds eye view) when the graph is too large.

- implement Fields and Field Rendering:
  http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ImageEffectsFieldRendering

- implement framerate support
  (look for various instances of "temporal coordinates", "framerate" and "frame rate" in the documentation)

- support more image components:
  - kOfxImageComponentAlpha
  - "uk.co.thefoundry.OfxImageComponentMotionVectors" (forward motion?  backward motion? both?)
  - "uk.co.thefoundry.OfxImageComponentStereoDisparity"(left-to-right? right-to-left? both?)

