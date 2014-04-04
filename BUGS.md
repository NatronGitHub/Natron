Bugs and feature requests
=========================

Bugs
----

Here is a list of know bugs, ordered by priority from high to low:

- Support OpenFX >= 1.2 double parameter types (normalized types are deprecated but can be supported):
  http://openeffects.org/dokuwiki/doku.php?id=standards:draft:ofx_image_processing_api_2:minor:non_normalised_spatial_parameters

- Some plugins crash using the multi-thread suite (mainly Furnace)

- messages (and persistent messages) should be associated with an instance, not with a viewer. The message should appear in the viewer if and only of the given instance is in the the viewer's parents.

- black borders around the viewer when zooming out and multithreading is disabled

- String_KnobGui: kOfxParamStringIsRichTextFormat  is not supported (it should use a QTextEdit and not convert the string to plain text)

- Tab traversal is not set up properly, and sometimes couter-intuitive.
  Many users still have a keyboard and use it, so this should be fixed.
  Refs:
  http://qt-project.org/doc/qt-4.8/qwidget.html#setTabOrder
  http://qt-project.org/doc/qt-4.8/focus.html


Missing and wanted features
---------------------------

Here is a list of non-blocking bugs / wanted features:

- the nodegraph should have a "f" shortcut to recenter the graph, and scrollbars (or a birds eye view) when the graph is too large.

- Progress suite is not implemented (GUI is blocked until the operation has ended)

- a log window with OFX and Natron log messages (see OfxImageEffectInstance::vmessage, OfxHost::vmessage)

- The OFX keyframe interface is missing some unimplemented functions yet:
  - OfxParameterSuiteV1::paramEditBegin — Used to group any parameter changes for undo/redo purposes
  - OfxParameterSuiteV1::paramEditEnd — Used to group any parameter changes for undo/redo purposes
  - OfxParameterSuiteV1::paramCopy — Copies one parameter to another, including any animation etc...

- support Parameter Interacts, which are OpenGL "widgets" integrated in the parameters panel
http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ParametersInteracts
  An example can be found in the "Prop Tester" plugin from the Support
  plugins ("OFX Test" plugin group). The "RGB Custom" and "RGB Custom
  2" parameters have parameter interacts.

- implement Fields and Field Rendering:
  http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ImageEffectsFieldRendering

- implement framerate support
  (look for various instances of "temporal coordinates", "framerate" and "frame rate" in the documentation)

- support more image components:
  - kOfxImageComponentAlpha
  - "uk.co.thefoundry.OfxImageComponentMotionVectors" (forward motion?  backward motion? both?)
  - "uk.co.thefoundry.OfxImageComponentStereoDisparity"(left-to-right? right-to-left? both?)

