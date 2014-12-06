Known bugs and limitations
=========================

Bugs
----

Here is a list of know bugs, ordered by priority from high to low:

- Resampling nodes (Transform, CornerPin) do not implement texture
  filtering, see <https://github.com/MrKepzie/Natron/issues/258>. When
  downsizing an image with a factor larger than 1.5, the image should
  be blurred before being downscaled to avoid aliasing
  artifacts. Mipmapping and anisotropic texture filtering, which solve
  this problem, will be implemented in the future.

- black borders around the viewer when zooming out and multithreading
  is disabled
  
- Tab traversal is not set up properly, and sometimes couter-intuitive.
  Many users still have a keyboard and use it, so this should be fixed.
  Refs:
  <http://qt-project.org/doc/qt-4.8/qwidget.html#setTabOrder>
  <http://qt-project.org/doc/qt-4.8/focus.html>

Limitations
-----------

- Resampling nodes (Transform, CornerPin) do not implement texture
  filtering, see <https://github.com/MrKepzie/Natron/issues/258>. When
  downsizing an image with a factor larger than 1.5, the image should
  be blurred before being downscaled to avoid aliasing
  artifacts. Mipmapping and anisotropic texture filtering, which solve
  this problem, will be implemented in the future.

Missing and wanted features
---------------------------

Here is a list of non-blocking bugs / wanted features:


- implement Fields and Field Rendering:
  <http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ImageEffectsFieldRendering>

- support deep images, via the multiplane suite
  <http://openeffects.org/standard_changes/240>
  <http://openeffects.org/standard_changes/224>
