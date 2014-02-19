Bugs and feature requests
=========================

Bugs
----

Here is a list of know bugs, ordered by priority from high to low:

- File dialog: the file list should be a few pixels wider, so that the horizontal scrollbar does not appear.

- String_KnobGui: kOfxParamStringIsRichTextFormat  is not supported (it should use a QTextEdit and not convert the string to plain text)

- Tab traversal is not set up properly, and sometimes couter-intuitive.
  Many users still have a keyboard and use it, so this should be fixed.
  Refs:
  http://qt-project.org/doc/qt-4.8/qwidget.html#setTabOrder
  http://qt-project.org/doc/qt-4.8/focus.html

- On Linux the property dock's scrollbar is broken and doesn't allow the user to span through
the whole content of the widget.

- On Ubuntu 13.10 with Qt5, Natron is non-functional (transparent widgets, extra windows...)

Missing and wanted features
---------------------------

Here is a list of non-blocking bugs / wanted features:

- a log window with OFX and Natron log messages (see OfxImageEffectInstance::vmessage, OfxHost::vmessage)

- derive and integrate are not implemented for animated Double,
  Double2D, Double3D, RGBA, RGB parameters (derive returns 0,
  integrate returns (t2-t1)*getValue())

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

- zoomable and accurate histogram

  An infinitely-zoomable and accurate histogram is useful to examine the
  effects of conversions, sampling, and to find threasholds.

  For example, when you load a 12-bits file, does it really have 12
  bits, or is it just an upgraded 8-bit file?
  For examples of 10-bit and 12-bit files, see:
  http://www.sounddevices.com/products/pix240i/sample-files/
  http://samples.mplayerhq.hu/V-codecs/R210/

  The histogram must be rebuilt each time the histogram window is panned
  or zoomed in, and the "bin" size must be equal to 1 pixel in the
  histogram view.

  Each time the histogram is zoomed or panned:
  - let vmin and vmax be the minimum and maximum x-values visible in the
    histogram (in intensity units)
  - let width be the histogram widget width
  - we have to build a histogram of the image or the image ROI from vmin
    to vmax with a bin_size of (vmax-vmin)/width (there's no other
    solution than scanning the image at each pan/zoom step to build this
    new histogram)
  - the bin count (the "y" value on the histogram) has to be normalized,
    so that it doesn't depend on the image ROI or on the histogram
    pan/zoom. Just divide the pixel count for a bin by the image ROI size
    in pixels, and by the bin_size. The result is a probability density
    function for each value (its integral from -infinity to +infinity is
    exactly 1).


- Autocontrast

  Autocontrast would compute the minimum and maximum values of the image
  in the intersection of the viewer area and the viewer ROI, and adjust
  the response curve accordingly. That's great for examining noise,
  seeing in the shadows, or examining non-intensity values (such as
  depth or optical flow).

- support more image components:
  - kOfxImageComponentAlpha
  - "uk.co.thefoundry.OfxImageComponentMotionVectors" (forward motion?  backward motion? both?)
  - "uk.co.thefoundry.OfxImageComponentStereoDisparity"(left-to-right? right-to-left? both?)

