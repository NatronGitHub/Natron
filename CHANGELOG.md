# Known Bugs

- Windows: If Natron is not installed in Program Files using the installer, the Text node does not work because fontconfig does not initialize properly.

# History


## Version 2.3.15

- Inputs of the selected nodes are now always visible

### Plugins

- Fix bugs in DenoiseSharpen that caused crashes #300
- Add support for chromatic aberration correction when reading RAW files #309 


## Version 2.3.14

- Error messages are not cleared anymore at each rendered frame. They can be cleared explicitely using the "Refresh" button in the viewer.
- ReadSVG: Fix reading layers of SVG files #292
- Fix many G'MIC plugins. Changes are not backward compatible and existing graphs may need to be reworked. Note that G'MIC plugins are still beta. #295
- LensDistortion: fix loading PFBarrel files #296
- Label parameters now use both columns in the parameters panel.


## Version 2.3.13

- Fix default value for file premult in ReadSVG (should be premultiplied).
- HSV values in the viewer info lines are now computed from linear RGB #286.
- RGBToHSV, HSVToRGB, RGBToHSL, HSLToRGB, RGBToHSI, HSIToRGB: Use linear RGB values in computation #286.
- Tracker: fix bug where Transform tracking was wrong when using more than 1 point #289.


## Version 2.3.12

*Note*: all github issues were moved to https://github.com/NatronGitHub/Natron/issues , and issue numbers in the change log now refer to this github repository.
Issue numbers up to version 2.3.10 refer to archived issues in https://github.com/MrKepzie/Natron/issues .

- Fix font selection in the plugins that render text (Text and Polaroid from openfx-arena) #269.
- Python: add Effect.getOutputFormat() method.
- GCC 8.1 (used for the 2.3.11 binaries) breaks the timeline GUI, revert to GCC 7.3 for Linux builds #279.
- Disable crash reporter in official binaries (requires maintenance to get it working again), and add crash reporter code to the source tree.


## Version 2.3.11

- User Guide: Add documentation on tracking and stabilizing.
- FrameBlend: Add Over operation, add decay parameter, and fix bugs.
- Fix Python Pseudo-random number generators random(min,max,time,seed) and randomInt(min,max,time,seed)
- Shadertoy: Document the process to adapt a shader from shadertoy.com to the Shadertoy plugin.
- Support FFmpeg 4.0 in the ReadFFmpeg and WriteFFmpeg plugins.


## Version 2.3.10

- Add "Overwrite" checkbox to writers to avoid re-rendering the whole sequence #1683.
- Writers do not render the whole RoD on their input anymore (this may result in a huge speed improvement on some projects).
- Fix bug where effects could be marked as pass-through if their RoD was larger than the project format #1578
- Add python method setCanAutoFoldDimensions(enable) to Double2DParam and Double3DParam.
- Fix several plugins when included in a PyPlug: ColorSuppress, HSVTool, ImageStatistics, Ramp, Shuffle, Reformat #1749.
- Fix Matrix3x3 and Matrix5x5 GUI (y was reversed).


## Version 2.3.9

- Numerical text fields (aka SpinBoxes) are now auto-reselected when validated #1737.
- When a node is selected, make its inputs/outputs more visible.
- Node shape is now thinner in the node graph.
- In the node graph, the cursor now really reflects what a mouse click would do.
- All nodes inside a backdrop can be enabled/disabled using the "D" key #1720.
- Fix the "." shortcut for creating a Dot in the nodegraph on non-French keyboards.
- Only auto-connect a newly-created node to the input of the selected node if it cannot have an input itself.
- Made more clear what the "LUT" tab in the project settings is for #1744.
- Better "hide all parameters without modifications" behaviour #1625
- Only the keypad number keys should be used to nudge Bezier/tracker, regular number keys switch the viewer #1582


## Version 2.3.8

- Fix crash when Viewer/maxNodeUiOpened=1 in the preferences
- Fix bug where an exported PyPlug would not have a correct Python syntax if a string had a backslash followed by newline (as may be seen in Shadertoy sources).
- Fix behaviour of loop/bounce in the Read node when there is a time offset #1734.
- G'MIC plugins by Tobias Fleischer are now bundled with the binary distributions (beta).
- Fix bug where the "&" character was not displayed in the plugin creation menus.


## Version 2.3.7

- The viewer timeline can now display information as timecode instead of frames (see TC/TF choice next to fps below the timeline)
- Disabled Merge nodes (and other nodes with input A and B) now always pass-thru the B input. Preferences should never affect the render results.
- Shuffle now passes through B by default, and has a new toggle parameter "setGBAFromR" to disable automatically setting G B and A from R.
- Grade: Add "Reverse" option to apply the inverse function. Usage: clone or copy-paste a Grade node, insert it downstream of the original node, and check "Reverse" in the downstream Grade.
- Fix RunScript (the bugs were in ReadOIIO and Natron), and better document it.
- GIF format is now read and written by ReadFFmpeg (most GIFs these days are animated).


## Version 2.3.6

- Fix bug when using PyPlugs containing Shadertoy (and possibly other plugins too) #1726 #1637
- Fix bug when creating a group from a plugin with invisible inputs (e.g. Shadertoy)
- Fix bug where Natron would crash when the "clear all panels" button is pressed #1729
- Fix bug where Roto and RotoPaint lifetime would change randomly #1707

### Plugins

- ReadOIIO/ReadPNG: "Image Info..." gives a more explicit message, containing the filename and more info.
- Fix bug in all OCIO plugins where GPU render is wrong if (un)premult is checked (disable GPU render in this case)


## Version 2.3.5

### Plugins

- ReadOIIO: fix a bug where the Read nodes are not properly restored when timeOffset is used #1711
- ReadOIIO: add options for reading RAW files: rawUserSat, rawHighlightMode, rawHighlightRebuildLevel
- DenoiseSharpen: fix a bug where the plugin would not abort processing when required.
- ColorCorrect: fix luminance computation when applying saturation #1706
- Reformat: add a checkbox to use the input RoD instead of the input format (useful when preceded by a crop)
- ContactSheet/LayerContactSheet: correctly set the format when the output size is changed


## Version 2.3.4

- Binaries distributed through Natron's web site are now built with 8-bit x264. 10-bit x264 (introduced with 2.2.6) causes too many compatibility issues. There are other codecs that support 10-bit output (especially ProRes, vc2, libopenjpeg, libvpx-vp9, and x265 on some systems). In order to get 10-bit x264, it is recommended to encode a quasi-lossless using one of these codecs, and then transcode with a ffmpeg binary capable of encoding 10-bit x264.
- Work around a possible Qt/Linux  bug where tablet events have a negative pressure on Wacom Intuos tablet #1697
- Binaries: add 10-bit HEVC encoding, Cisco openh264 encoding, VidVox Hap encoding.

### Plugins

- Upgrade SeExpr to version 2.11.
- Grade: add a "Normalize" button to automatically compute the clack and white points.
- Matrix3x3, Matrix5x5: new plugins, apply a 3x3 or 5x5 custom filter.
- ColorCorrect: Fix wrong render for input values outside the [0-1] range #1703
- ReadOIIO: Adjust Maximum Thr. (used when reading RAW camera files) should defaut to 0.0 #1705


## Version 2.3.3

- Fix computation of remaining time when rendering.
- Fix loading third-party plugins on Linux #1682

### Plugins

- ColorLookup: add "Master Curve Mode" parameter, which enable selecting different algorithms to apply the tone curve with more or less color shifting. These curve modes are inspired by [RawTherapee](http://rawpedia.rawtherapee.com/Exposure#Curve_Mode). Also add the possibility to compute and display the RGB histogram of the source input.
- WriteFFmpeg: support 10-bit DNxHR 444 and DNxHR HQX (and fix a related FFmpeg bug).
- Shadertoy: rework many presets so that they work better with Natron, especially the effects in the Blur, Effect, and Source categories. Most blur effects can also be modulated per-pixel using the "Modulate" input to get Z-dependent blur.
- Shadertoy: new presets: Blur/Bokeh Disc, Blur/Mipmap Blur, Effect/Anaglyphic, Effect/Bloom Paint, Effect/Sawnbringer 4bit, Effect/Sharpen, Effect/CRT, Effect/Fisheye, Effect/Image Cel Shade, Effect/Kaleidoscope, Effect/Noisy Mirror, Effect/Quad Mirror, Effect/Q*Bert-ify, Effect/Stripes, Effect/Vignette, Source/Cloud, Source/Cloudy Sky, Source/Disks, Source/Fireball, Source/Flash, Source/Seascape, Source/Star Nest, Source/Voronoi
- Shadertoy: add iChannelOffset extension.
- LensDistortion: add cropToFormat parameter (true by default), to avoid computing areas outside of the project frame.


## Version 2.3.2

- Merge: fix bug #1648, where the alpha from the B input would be used for all consecutive merge operations, rather than updated at each operation.


## Version 2.3.1

- Python: Fix the setTable() function of the PathParam class
- Windows: Fix a bug where most image formats would not read and write correctly


## Version 2.3.0

- Fix lots of bugs when loading older Natron project files.
- Internal changes: implement a new OpenFX multiplane suite.


## Version 2.2.9

- Fix a bug where the channel selector would be wrong when loading older projects (e.g. Spaceship).

### Plugins

- SpriteSheet: convert a sprite sheet to an animation.


## Version 2.2.8

- OpenGL interact is now always affected by transforms even if there is motion blur.

### Plugins

- Radial, Rectangle and generators: when the Extent is set to Size, and Reformat is not checked, Bottom Left and Size can be animated.
- add Box filter to Transform, CornerPin, GodRays, and all Distortion nodes.
- ContactSheet, LayerContactSheet: Make a contact sheet from several inputs, frames or layers (beta)


## Version 2.2.7

- Reduce noise on the roto shape borders #1594

### Plugins

- ReadOIIO: fix bug when reading EXRs with datawindow different from displaywindow.
- (beta feature) Inpaint: New plugin. Inpaint the areas indicated by the Mask input using patch-based inpainting. Please read the plugin documentation.


## Version 2.2.6

- Multi-dimensional parameters don't automatically fold into a single dimension, except for scale and size params.
- Binaries distributed through Natron's web site are now built with 10-bit x264 and 10/12-bit libvpx-vp9 support. That means the produced video files may be unplayable on some hardware (e.g. phones or TVs), but Natron should really be used to produce digital intermediates with the highest possible fidelity, which can then be transcoded to more suitable distribution codecs.
- Better cache usage for Readers, ReadOIIO does not use the OIIO cache anymore (partly due to https://github.com/OpenImageIO/oiio/issues/1632).
- Fix a bug where custom OpenFX plugins directories would be ignored #1584
- Add a documentation chapter on importing camera data from PanoTools-based software (PTGui, PTAssembler, Hugin).

### Plugins

- WriteFFmpeg: the preferred pixel coding and bit depth can now be selected, which is very useful for codecs that propose multiple pixel formats (e.g. ffv1, ffvhuff, huffyuv, jpeg2000, mjpeg, mpeg2video, vc2, libopenjpeg, png, qtrle, targa, tiff, libschroedinger, libtheora, libvpx, libvpx-vp9, libx264, libx265).
- WriteFFmpeg: Bitrate-based (CBR) encoding was removed from the following codecs in favor of quality-based (VBR) encoding, mainly because CBR should be used in 2-pass mode (using handbrake or the ffmpeg command-line tool): mpeg4, mpeg2video, mpeg1video, flv.
- ColorCorrect: changed the Contrast formula to make adjustments more intuitive, see https://compositormathematic.wordpress.com/2013/07/06/gamma-contrast/ - this may affect existing projects that used the previously buggy Contrast parameter.
- LensDistortion: add PanoTools/PTGui/PTAssembler/Hugin model.
- Card3D can now import/export chan files from Natron, Nuke, 3D-Equalizer, Maya, etc., and txt files from Boujou.
- Card3D and CornerPin: only show things that are in front of the camera. In cornerPin, point 1 is always considered to be in front.
- Noise, Plasma: result is now reproductible (a given render always gives the same result).
- ReadOIIO: add advanced options for reading RAW files.
- STMap: Use the format of the Source input rather than its RoD to determine the texture size (useful when using an STMap written by LensDistortion).
- SmoothBilateral, SmoothBilateralGuided, SmoothRollingGuidance: The Value Std Dev. was clamped by CImg to a rather high value (0.1), making the filters almost useless. Fixed.


## Version 2.2.5

- Fix undo when manipulating 2D points in the viewer interact #1576
- Fix manipulating the interact plugin on non-retiming inputs of retiming effects (e.g. the Retime Map in Slitscan) #1577
- Fix exporting curves to ascii (eg ColorLookup), and values for xstart, xincr and xend can now be simple expressions.

### Plugins

- DenoiseSharpen: the output is now the noisy source when Noise Analysis is not locked. It is easier to see where there is noise that way, and it also makes the plugin usable in Resolve (which calls render even if non-significant parameters are changed)
- Generators now may set the output format when Extent=Size (as they do when Extent=Format or Extent=Project).
- Rework many plugins so that they work better in DaVinci Resolve (all generators including Radial and Rectangle, CopyRectangle, DenoiseSharpen, LensDistortion, HSVTool, ImageStatistics, Position)
- Card3D (beta): Transform and image as if it were projected on a 3D card.


## Version 2.2.4

- Write: remove Python page, add Info page
- ReadFFmpeg: fix "Image Info..." button (which calls ffprobe) when file path is relative to a project variable
- fix high-resolution application icons

### Plugins

- Readers and writers now only get the colorspace from the filename if it is before the extension and preceded by a delimiter
- BilateralGuided: bug fix
- Natron can now use plugins compiled with the DaVinci Resolve OpenFX SDK (which assumes that the host is Resolve), such as the Paul Dore plugins (see https://github.com/baldavenger/). These plugins may require CUDA runtime library to run. It can be found in the Resolve installation. On macOS, execute `cp "/Applications/DaVinci Resolve/DaVinci Resolve.app/Contents/Libraries/libcudart.dylib /Applications/Natron.app/Contents/Frameworks/libcudart.7.5.dylib`


## Version 2.2.3

- add proper quality options to WriteFFmpeg, including CRF-based encoding for libx264.
- the online documentation for Read and Write now show the documentation of the underlying plugin.
- fix bug in the recent files menu #1560
- fix reading of transparent PNG #1567
- fix clearing error message for readers/writers.
- disable MXF writing (too many constraints, use an external transcoder)
- fix MKV writing


## Version 2.2.2

This is a bug-fix release

- fix hue display in the viewer #1554
- EdgeBlur now has an optional Matte input used to compute the edges #1553
- fix reading image sequences that do not have frame number 1 in them #1556
- better/simpler GUI and documentation for the file dialog.


## Version 2.2.1

This is a bug-fix release.

- fix a bug with the file dialog when network drives are detached on Windows
- the "Open Recent" menu now shows the directory name if there are several files with the same name.

### Plugins

- STMap: was broken in 2.2, fix it.
- WritePNG: fix writing 16 bit PNG


## Version 2.2

- OpenGL rendering is enabled by default for interactive editing in plugins that support it (but still disabled for background rendering)
- Roto & RotoPaint: ellipses and circles are more accurate #1524
- When a plugin is not available with the right major version, use the smallest major version above for better compatibility (before that change, the highest major version was returned)
- Natron can now be launched in 32-bits mode on macOS
- Documentation is now licensed under CC BY-SA 4.0, and external contributions are welcome
- Organize nodes documentation
- New project formats: HD_720, UHD_4K, 2K_DCP, 4K_DCP

### Plugins

- The plugins that were made available as beta features in the 2.1 releases are now considered stable:
    - DenoiseSharpen: new wavelet-based denoising plugin
    - EdgeBlur: Blur the image where there are edges in the alpha/matte channel.
    - EdgeDetect: Perform edge detection by computing the image gradient magnitude.
    - EdgeExtend: Fill a matte (i.e. a non-opaque color image with an alpha channel) by extending the edges of the matte.
    - ErodeBlur: Erode or dilate a mask by smoothing.
    - HueCorrect: Apply hue-dependent color adjustments using lookup curves.
    - HueKeyer: Compute a key depending on hue value.
    - KeyMix: Copies A to B only where Mask is non-zero.
    - Log2Lin: Convert from/to the logarithmic space used by Cineon files.
    - PIK: A per-pixel color difference keyer that uses a mix operation instead of a max operation to combine the non-backing screen channels.
    - PIKColor: Generate a clean plate from each frame for keying with PIK.
    - PLogLin: Convert between linear and log representations using the Josh Pines log conversion.
    - Quantize: Reduce the number of color levels with posterization or dithering
    - SeExprSimple: new simple expression plugin with one expression per channel
    - Sharpen & Soften: new plugins.
    - SlitScan: Per-pixel retiming.
- SeNoise: fix bugs in the Transform parameters #1527
- PIKColor: do not expand region of definition
- Shadertoy: support iDate, add presets, fix CPU rendering #1526
- Transform & CornerPin: additional "Amount" parameter to control the amount of transform to apply.
- ColorLookup: fix a bug where output was always clamped to (0,1) #1533
- Grade: fix a bug where negative values were clamped even when gamma=1 #1533
- STMap, IDistort, LensDistortion, Transform, CornerPin: reduce supersampling to avoid artifacts
- LensDistortion: add STMap output mode, add undistort output, add PFBarrel and 3DEqualizer distortions model, add proper region of definition support.
- RotoMerge: a merge plugin that takes an external mask as the alpha for the A input.
- WriteFFmpeg: DNxHD codec now supports DNxHR HQ, DNxHR SQ and DNxHR LB profiles.


## Version 2.1.9

- Read/Write: Do not automatically set parameters when changing the filename (#1492). Creating a new Reader/Writer still sets those automatically.
- ctrl-click on a node in the nodegraph (cmd-click on Mac) now opens its control-panel (same as double-click)
- Curves with a single keyframe can now have a slope.
- Fix gamma=0 in the viewer
- Fix a bug where removing all control points from ColorLookup would crash the application
- Fix bugs in curves drawing.
- Fix potential crashes when using KDE on Linux
- Fix a bug where Roto strokes drawn at a different frame than the current frame would disappear
- (macOS) Fix a bug where the interface would become very slow after a String parameter is disabled or enabled

### Plugins

- ColorWheel: antialiased rendering
- Rectangle: add rounded corners
- PIKColor: fix black rims issue #1502, rework & optimize
- PIKColor: remove hard limits on parameters b85f558
- Grade & Gamma: fix behaviour when gamma=0
- Shadertoy: support iChannelResolution


## Version 2.1.8

- Fix a bug where several images with file names that contain two numbers would be be considered as a sequence even if both numbers differed.
- Fix a bug where a disabled Merge node would pas input A if the preferences say Merge should auto-connect to B (#1484)
- Node Graph: hints for possible links between nodes are now only active when holding the Control key (Command on macOS) (#1488)
- Roto: The default shortcuts to nudge Control Points has been changed to the num-pad 2,4,6 and 8 keys instead of the arrow keys
which were conflicting with the timeline shortcuts. (#1408)
- Fix a bug where the output channels of the Shuffle node could forget their link or expression (#1480)
- Fix a bug where the focus on parameter would jump randomly (#1471)

### Plugins

- Rectangle: antialiased rendering + remove the "black outside" param
- Radial: antialiased rendering (can render an antialiased disc or ellipse when softness=0)
- Merge: fix a bug where the "A" checkbox of the B input would always be turned off when an RGB clip is connected. This is now done only if the user never toggled that checkbox.
- (beta feature) EdgeDetect: Perform edge detection by computing the image gradient magnitude.
- (beta feature) EdgeBlur: Blur the image where there are edges in the alpha/matte channel.


## Version 2.1.7

- Fix a bug where PyPlugs would not load if the PYTHONPATH environment variable was set

### Plugins

- (beta feature) SlitScan: Per-pixel retiming.


## Version 2.1.6

- Fix a bug where Natron would freeze or be extremely slow when using big node graphs

### Plugins

- DenoiseSharpen: Use a more intuitive "Sharpen Size" parameter instead of "Sharpen Radius", and do not sharpen the noise. Add "Denoise Amount" parameter, which can be set to 0 to sharpen only.
- ErodeBlur: fix the "Crop To Format" parameter (which did not work)
- (beta feature) HueCorrect: Apply hue-dependent color adjustments using lookup curves.
- (beta feature) HueKeyer: Compute a key depending on hue value.


## Version 2.1.5

- Introduce the notion of "Format", which is basically the area of the image to be displayed (similar to the display window in OpenEXR). Each clip has a format attached, so a project can contain images of different sizes.
- Fix a bug where deprecated plugins would not be loaded from project file 2561778
- macOS: clicking the dock icon now raises all windows
- macOS: fix ColorLookUp curves parameter display
- PyPlug: fix a bug where removing a node inside a Group would break any expression on its siblings
- Reader: fix a bug where copy/pasting a node would display a "Bad Image Format" error
- The whole user interface now uses the same font, and dialogs were cleaned up to use standard buttons
- Roto: fix a bug where the selected tool in the viewer would not refresh properly
- Fix a bug where Natron would not work properly when installed in a directory containing unicode characters 
- OpenEXR: fix a bug where auto-crop files would not have their origin placed correctly

### Plugins

- Crop: add the "Extent" choice, to chose either a predefined format or a custom size
- Blur: add the "Crop To Format" option.
- Reformat: if input has a format, use it to compute the reformated output.
- NoOp: can also set the format.
- Shuffle: re-enable the "Output Components" choice"
- Premult/UnPremult: don't try to check processed channel when rewiring the input
- (beta feature) ErodeBlur: Erode or dilate a mask by smoothing.
- (beta feature) KeyMix: Copies A to B only where Mask is non-zero.
- (beta feature) PIK: A per-pixel color difference keyer that uses a mix operation instead of a max operation to combine the non-backing screen channels.
- (beta feature) PIKColor: Generate a clean plate from each frame for keying with PIK.
- (beta feature) Sharpen & Soften: new plugins.
- (beta feature) EdgeExtend: Fill a matte (i.e. a non-opaque color image with an alpha channel) by extending the edges of the matte.


## Version 2.1.4

- Windows: Fix a bug where the UI would freeze for a long time when reading files over a network share
- Python: Add ExprUtils class that adds helpers for FBM and Perlin noise (taken from Walt Disney Animation SeExpr library)
- Tracker: fix add/remove jitter motion types
- Fix creation of SeNoise node
- Fix a bug where the data-window would not be read correctly with auto-crop EXR image sequences
- Fix a bug where the group expand/fold feature would not work correctly


## Version 2.1.3

- Gui: sliders have a cleaner look with less ticks and a round handle 231c7f7
- fix bug where the OFX plugin cache could be wrong if OpenGL was disabled 32c1532
- fix dynamic kOfxSupportsTiles handling
- add more properties to the "Info" node panel
- the log window now becomes visible whenever a message is sent
- Fix a bug where entering a Group node could crash Natron
- Fix a bug where copy pasting a Group could loose expressions/links used nodes within the Group
- Fix a bug when reading auto-crop EXR image sequences 
- Fix a bug where a project could use all the RAM available plus hit the swap when reading scan-line based multi-layered EXR files

### Plugins

- (beta feature) SeExprSimple: new simple expression plugin with one expression per channel
- (beta feature) DenoiseSharpen: new wavelet-based denoising plugin
- (beta feature) Log2Lin: Convert from/to the logarithmic space used by Cineon files.
- (beta feature) PLogLin: Convert between linear and log representations using the Josh Pines log conversion.
- (beta feature) Quantize: Reduce the number of color levels with posterization or dithering
- ImageStatistics: can now extract the pixels with the maximum and minimum luminance, as well as their values


## Version 2.1.2

- Viewer: A new button can now force full-frame rendering instead of the visible portion. This may be useful to remove borders artifacts when panning/zooming during playback
- Fix a bug where the Glow node would not work correctly
- Windows: Fix a bug where the 32-bit version would crash on launch with AMD Graphic Cards
- Fix a bug where the Read node would sometimes show an error dialog but everything was in fact fine
- Fix a bug where changing the output filename in a Write node would reset encoder specific parameters
- Tracker: when clicking "Set Input RoD" in the From points of the corner pin, automatically re-compute the To points over all keyframes
- Tracker: A bug was found in the internal algorithm when tracking with a rotation/affine model. This was fixed in co-operation with Blender developers
- Fix a bug where the .lock file of a project would not go away even if closing Natron correctly
- Fix a bug where expressions would not work in some circumstances


## Version 2.1.1

- Fix a bug where enabling GPU rendering from the settings would not be taken into account correctly by the ShaderToy node
- Fix a bug where panning the viewport during playback could show banding artifacts
- Fix a bug where the Reformat node would not work properly when loading a project and then switching the Type parameter
- Fix a crash when adding a control point to a roto shape with CTRL + ALT + LMB 
- Fix a bug where the writing of the OpenFX plug-ins loading cache was not thread-safe. Multiple Natron processes on a same node of a render farm could corrupt the cache thus failing some renders because plug-ins could not load.
- The Dilate/Erode nodes now have the Alpha checkbox checked by default and also have a parameter to expand the bounding box
- The Auto-Turbo mode is now also enabled when writing out on disk


## Version 2.1.0

- The point tracker was completely reworked. It now uses libmv (from Blender). It now allows tracking different motion types (translation, rotation, scaling, perspective) and can be used for anything that needs match-moving or stabilizing.
- Readers/Writers are now all under the same Read/Write node. If 2 readers/writers can decode/encode the same format, a drop-down allows to choose between them.
- Python: it is now possible to pass extra properties to the createNode function to control how nodes are created, see (Python reference)[http://natron.readthedocs.io/en/master/devel/PythonReference/NatronEngine/App.html#NatronEngine.NatronEngine.App.createNode].
- (beta feature) New documentation for Natron: Whenever pressing the "?" button in the properties panel of a node, Natron will open-up a page in your web-browser with documentation for this node. Natron also has a user-guide that is work in progress. 
You may contribute to this user guide, follow (these)[http://natron.readthedocs.io/en/master/guide/writedoc.html] instructions to enrich the documentation.
- (beta feature) Shadertoy is a new plugin that allows writing plugins using GLSL fragment shaders. It supports both GPU rendering using OpenGL and CPU rendering using Mesa.
- (beta feature) OpenGL rendering is now supported (enable it in Preferences/GPU rendering). Tested with the Shadertoy plugin and the HitFilm Ignite plugins.
-  ColorLookup now has a background that makes it easier to use as a color ramp.
- (beta feature) TextFX is a new text plugin with more features than the original text node(s).
- (beta feature) ReadCDR is a new reader plugin that supports CorelDRAW(R) documents.
- (beta feature) ReadPDF is a new reader plugin that supports PDF documents.


## Version 2.0.5

- Viewer: The number keys now always switch input A when not shifted, and B when shifted, even on keyboards where numbers should be shifted (such as French AZERTY).
- Viewer: Reworked the wipe modes. added Onion Skin and stack modes.


## Version 2.0.3

- Fixed a bug where some image sequences would fail to read when a first frame would be missing
- Fixed a bug where a PyPlug containing another PyPlug would fail to load in some cases
- Fixed a bug where the play button would not be pressed on all viewers during playback
- Python: add a function to File parameters to reload the file 
- Fixed a bug where the gui could stop giving feedback when rendering
- CTRL + left mouse button click can now be used on sliders to reset to their default value
- Fixed Python call to the onProjectSave callback which would not work


## Version 2.0.2

- Expressions are now persistent: it can become invalid if a condition is not met currently to successfully run the expression, but can become valid again automatically.
- Layers/Channels are now persistent: when copy pasting small "graphlets" or even creating PyPlugs, they will be remembered for convenience. An error may occur next to the corresponding parameter if the channel/layer does not exist any longer. You may right click the drop-down parameter to synchronize the menu to the actual state of the node to see the layers available.
- Fix a bug where expressions would not be evaluated at the correct time for nodes outside of the tree currently being rendered
- Fix a bug where the Format parameter would be reseted automatically for the Reformat node
- Fix a bug with QGroupBox stylesheet for PySide 
- Fix a bug with keyframes for Rotoshapes where some control points would not get keyframed
- Fix a bug where a size parameter could get ignored from the project if with=1 and height=1
- Fix a bug with PNG reading: if a sequence of PNG would contain grayscale/RGB images then it would be assumed that the format of the first image in the sequence would be the format of the whole sequence. 
- It is now possible to turn off the file dialog prompt when creating a Write node from the Preferences->General tab
- Write nodes now default their frame range to the project range
- Fix a bug where the Write node channels checkboxes would not be taken into account correctly
- Python API: add app.getActiveViewer() and app.getActiveTabWidget(), see Python reference for documentation
- Viewer: Whenever the B input is not set, set it automatically to the A input for convenience
- Drag and Drop: It is now possible to drag&drop images to anywhere in the Natron UI, not necessarily in the Node Graph. It is also possible to drop Natron projects or Python scripts. 
- Copy/Paste: It is now possible to paste python code directly into Natron UI, which will be run in the Script Editor automatically if valid 
- Node graph: improve performances
- Script Editor font and font size can now be customized from the Preferences-->Appearance tab
- Fix a bug where rendering in a separate process would always fail on Windows.
- WriteFFMPEG: fix a bug with multi-threading that was introduced with enhancements in 2.0.1 for certain codecs that do not support it


## Version 2.0.1

- Major performance improvement for ReadFFMPEG and WriteFFMPEG
- Fix crash received from crash reporter
- Roto: fix interaction with feather 
- Roto: It is now possible to add points to a shape by pressing CTRL+ALT+click
- Roto: fix a bug where animating any parameter would crash Natron
- Python: add ability to query the *active project* (i.e: the top level window) with the NatronEngine.natron.getActiveInstance() function
- Python: fix issue where the argument of *saveProject(filename)* would be ignored 


## Version 2.0

- Python: PyPlugs can now be used to create toolsets, such as Split And Join in the views menu
- New Glow PyPlug 
- New bloom node
- The HSVTool node now has an analysis button to set the src color
- The file dialog performances have been greatly improved 
- It is now possible to list all available drives in the file dialog on Windows
- New progress panel to manage renders to cancel, queue, pause or restart them
- EXR format reading/writing speed has been greatly improved
- ReadFFMPEG/WriteFFMPEG performances have been improved
- WriteFFMPEG has now a better interface to display possible format/codec combination
- The viewer now has the ability to pause updates so that no render is triggered upon any change that would modify the displayed image
- User interface for parameters has been improved and requires less mouse clicks
- Undo/Redo shortcuts (CTRL-Z or CMD-Z on Mac) now follow the mouse over focus indicated on the interface tabs
- The curve editor has received major speed enhancements when manipulating thousands of keyframes
- The dope sheet zoom is now always appropriate, even for still images
- Fixed issues on Linux/Windows where Natron interface would not display correctly on screens with high DPI
- Fixed issues on Windows where file paths with non ASCII characters would not be read/written correctly
- Fixed issues on Windows where reading from/writing to a network share would not work correctly
- Value parameters can now have Python expressions written directly in their spinbox and can be used as calculators
- A new demo project from Francois Grassard is available at [sourceforge.net/projects/natron](https://sourceforge.net/projects/natron/files/Examples/).
  It contains a fully-featured project demonstrating the use of a complex node-graph including usage of PyPlugs.


## Version 2.0rc4

- Write nodes now have a Frame Increment parameter which allows to skip frames while rendering.
- The command-line parameters and Python API have been updated in this regard (see Python documentation).
- Fix multiple bugs when rendering a multi-layered exr
- Linux: when crashing, Natron will now print a backtrace if launched from the Terminal. This is temporary and will be soon replaced by a cross-platform crash reports handler.
- RotoPaint: enhancements in the rendering algorithm 
- Color, Double and Integer parameters can now have an expression entered directly in a SpinBox for convenience
- NodeGraph: optimize for speed when the scene contains a lot of nodes and add auto-scrolling when reaching the border of the view


## Version 2.0rc3

- The Read node can now read multi-view EXR files. Decoding speed of EXR has been greatly improved.
- The Write node can now write multi-view and multi-layered EXR files. Encoding speed of EXR has been greatly improved.
- Viewer: The channel selected in the "Alpha channel" drop-down can now be overlayed on the image when using the "Matte" display channels
- The RotoPaint/Roto Transform and Clone tabs now have a viewer handle overlay to help manipulating the parameters
- DopeSheet and CureEditor: The scaling interactions with the bounding box of selected keyframes are now correct but may lead to keyframes being located at floating point times. 
- A "P" indicator at the bottom right of a node will indicate that a node is a pass-through. Hovering the mouse over the "P" will indicate from which input this node is a pass-through of.
The pass-through input will also be indicated with a plain arrow whereas all other inputs will have their arrow dashed. 
- Python API: it is now possible to retrieve the layers available on a node with the getAvailableLayers() function, see [Python Reference](http://natron.rtfd.org)


## Version 2.0rc2

- Python 2.7 API: 
  - Parameters expressions
  - User-defined parameters
  - Script editor to control the application overall
  - User-defined python callbacks
  - Integration of Pyside to the GUI so that the interface is extensible with new menus and windows
  - Python API is available at http://natron.readthedocs.org
- Nodes group to have cleaner graphs with hidden sub-nodegraphs
- PyPlug: You can export a  group as a Python plug-in and it be re-used in any other project as a single node as you would use any other plug-in
- SeExpr integration within a node: http://www.disneyanimation.com/technology/seexpr.html
- New SeNoise and SeGrain nodes based on SeExpr 
- RotoPaint node with Wacom tablets support
- DopeSheet editor: This is where you can control easily keyframes and clips in time for motion graphics purposes
- Render statistics: Available in the Render menu, use this to debug complex compositions
- New Text plug-in with much more controls than the previous Text node
- New TextPango node based on the Pango library to directly input Pango Markup Language (html-like),  see https://github.com/olear/openfx-arena/wiki/Pango
- Many new nodes, based on the ImageMagick library: ReadPSD, ReadSVG, Charcoal, Oilpaint, Sketch, Arc, Polar, Polaroid, Reflection, Roll, Swirl, Tile, Texture
- New nodes:
  - STMap
  - IDistort
  - LensDistortion
  - TimeBlur
  - OCIODisplay node
  - GmicEXPR, to enter GMIC expressions
- Rewrote entirely the WriteFFMPEG and ReadFFMPEG nodes. Most widely used codecs are supported
- New merge operators: grain-extract, grain-merge, color, hue, luminosity
- New gamma and gain controllers on the viewer
- Multiple viewers can now have their projection synchronised to enhance image comparison
- Support for multi-layered workflows for cleaner graphs
- Better support for multi-view workflows
- Various performance and stability enhancements across the board 


## Version 1.2.1

- New GodRays and DirBlur nodes.
- New RGBToHSI/HSIToRGB YUVToRGB/RGBToYUB YCbCrToRGB/RGBToYCbCr nodes
- Fixed a bug where some TIFF files would not read correctly on Windows versions of Natron
- FIxed a bug where a crash could occur with the Merge node with mix = 0
- Fixed a bug where ReadFFMPEG would sometimes decode incorrectly files with bit depth higher than 8
- Miscellaneous stability fixes


## Version 1.2.0

- Overlays on the viewer are now transformed by all Transform effects such as CornerPin,Transform 
- The user interface colors can now be changed in the "Appearance" tab of the Preferences of Natron
- Bezier curves motion now have a constant or linear interpolation 
- The dialog that pops up when trying to merge 2 clips that do not have a matching FPS is now a bit clearer regarding possible solutions
- Wacom support is now more intuitive and supported on all widgets that can zoom or pan.
- New "Image statistics" node to analyse various significant values from an image
- It is now possible to connect any node to a Writer node

### Bug fixes

- ReadFFMPEG would crash when reading video files with a videostream bitdepth > 8bit
- ReadFFMPEG would crash when reading image sequences
- The viewer would not redraw correctly
- The nodegraph would not redraw correctly, hence producing a latency effect
- Readers would not recognize some image sequences
- Some TIFF files would not be read correctly by the ReadOIIO node

### Known issues

- Stereo workflows are broken and will be re-introduced with a more
  clever approach in the next version. The only way for now is to use
  a Switch node.


## Version 1.1.0

- Fix stability of the software as a whole. This build has been reported to be robust to several hours of work.
- The tracker node can now be offset to track elements that go beyond the bounds of the image
- The roto node interaction has been improved:
- The composing of selection is now easier:
    - A new button can now toggle on/off the click capture of the bounding box of selected points
    - The smooth/cusp options are now applied with smaller steps and depending of the zoom factor
    - A simple left click is required to cycle through tool buttons and a right click to open the menu
    - A new button in the settings panel allows to clear the animation of the selected Beziers
    - Only the keyframes of the selected shapes are now displayed on the timeline instead of all shapes keyframes
- CurveEditor: added a filter to display only nodes containing the filter
- CurveEditor: double-clicking the item of a node will open the settings panel much like a double-click in the NodeGraph does
- The interpolation submenu of the animation menu of the parameter is working again
- A simple click on an arrow of the node graph no longer disconnects nodes to prevent mistakes, instead now, to disconnect only with a simple click it is required to hold the control modifier.
- Playback: The behaviour when using multiple viewers is now much more logical: all viewers are kept in sync and follow the frame range of the viewer that started the playback. The frame range is now per viewer.
- The frame range is now a property of the project: Each time a Reader node the frame range of the project is unioned with the frame range of the sequence read (unless the Lock range parameter is checked). New viewers that have their frame range untouched will use the project frame range by default.
- New nodes: Add, Multiply, Gamma, ColorConvert, SharpenInvDiff, SharpenShock


## Version 1.0.0

- Transform effects (such as Transform, CornerPin, Dot, Switch) now concatenates: the filtering
is now only applied by the "bottom" node of the transform chain. For example if 2
Transform nodes are setup one after another, the first with a scale of 0.1 and the second
with a scale of 10 then only the downstream node will be rendered with a scale of 1.
- The font of the application and its size are now customizable in the preferences, however a change to these settings requires a restart of Natron. The new default font is "Muli": https://www.google.com/fonts/specimen/Muli
- We slightly adjusted how caching works in Natron which should globally make the software much faster. Previous versions of Natron had what's called an "Agressive caching" behaviour: every image of every node was cached in RAM, resulting in heavy memory usage and sometimes degraded performances. The new default behaviour is to cache the output of a node only if:
    * Several outputs are connected to this node
    * The node has a single output, but that output has its settings panel opened (Meaning the user is heavily editing the output effect and would like the input branch being in the cache)
    * The node has its preview image enabled (in interactive session only)
    * The node is using several images at different times to compute the result of the output (e.g. a retimer node)
    * The parameter "Force caching" in the "Node" tab of the settings panel is checked
Aggressive caching can however make the interactivity of Natron slightly faster (when using it in GUI mode) but would not be any useful in background render mode, so make sure it is checked off when rendering on disk. 
You can by-pass this behaviour and come-back to the original "Aggressive caching" solution by turning it on in the Preferences of Natron. At least 8GiB of RAM are required to use aggressive caching, 16GiB  are recommended. 
- New HSVTool node to adjust hue, saturation and brightnes, or perform color replacement.
- New HSVToRGB & RGBToHSV nodes to convert between these 2 color-spaces
- New Saturation node to modify the color saturation of an image.
- New DiskCache node. This node allows to cache a branch of the compositing tree to re-use it later on without re-computing the images. This cache is persistent on disk and is saved between 2 runs of Natron. You can configure the location and maximum size of the cache in the preferences of Natron; in the Cache tab. 
- A new progress bar will display the progression while loading a project
- When zooming out of the node-graph, all texts on nodes / arrows will be hidden to increase performances when handling huge compositions.
- Tracker: all tracks are now multi-threaded for better performances. Also fixed a bug where the overlay displayed while tracking wasn't matching the underlying displayed image.
- Roto: Selected points can now be dragged from everywhere within the bounding box instead of only the cross-hair.
- Roto: It is now possible to move a Bezier just by dragging a part of the curve where there is no control point.
- Roto: Holding shift while dragging a scale handle of the bounding box will now scale only the half of the shape on the side of the handle
- Improved parameters alignment and spacing in the settings panel 
- A new tab in the preferences is now dedicated to plug-ins management. You can now choose to enable/disable a plug-in. This can be seen as a blacklist of the plug-ins you don't want to use. By default most TuttleOFX nodes that are redundant with the bundled nodes will be disabled. 
- Also another per plug-in control has been added to regulate whether the a plug-in should be aware of zoom levels or not. Zoom level aware means that a plug-in will attempt to render images at lower resolution if the viewer is zoomed-out or if proxy mode is enabled. This setting is set by the plug-in internally, but some plug-ins are known to be bugged (they flag that do support zoom levels but in fact they don't). 
- A new changelog tab in the About window is now available
- Roto: When restoring a project, the default tool will be "Select All" instead of "Bezier"
to avoid creating new beziers by mistake
- Timeline: when pressing the left and right arrows of the keyboard, the cursor will no longer cross the bounds of the timeline but loop over the range instead.
- Viewer: the drop-down to select the currently visualized channels now reflects the current choice with a specific border color for each options. 
- A new Auto-turbo setting has been added: when enabled, the Turbo-mode (originally toggable with the button on the right of the media player) will be enabled/disabled automatically when playback is started/finished. You can turn on/off this preference in the settings (NodeGraph tab) or in the right click menu of the node-graph.
- Transform: When holding down the SHIFT modifier and controling the translate handle on the viewer, the direction will be constrained to either the X or  Y axis.
- Fixed a crash on windows when connecting nodes
- Fixed a bug on windows where the properties pane would overlap the viewer if placed below it
- Fixed a bug where the locale won't be taken into account and files with accents wouldn't be correctly displayed in the file dialog
- Viewer: fixed a bug when the "Auto-wipe" preferences in the settings was disabled. The wipe would still show up automatically. 
- Fixed a bug where the extra OpenFX plug-ins search paths would be ignored
- Backdrop: only move nodes which are initially within the backdrop, not the ones that are crossed when moving it.
- Nodegraph: zooming is now done under the mouse cursor
- Readers: when a file changes externally, don't reload it automatically, instead a warning is displayed on the viewer and it is up to the user to reload it with the button created specifically
for the occasion. The warning notification can be disabled in the preferences of Natron. The tooltip of the field with the filename now indicates the last modification date of the file.


## Version 1.0.0rc3

- The internal renderer now estimates the amount of RAM needed by a compositing tree to render out a single frame and will thus limit the number of parallel renders so that all parallel renders can fit in RAM if possible. Before this change the renderer would only take into account CPU load which would seriously slow down computers without enough RAM to support all the CPU power available.
- Focus has been re-worked through all the interface so the focus switches correctly among the widgets when pressing Tab. Drop-downs can now get the focus so the user can
scroll the items by pressing the up/down arrows. Checkbox can now also have focus and you can check/uncheck them by pressing the space bar. Left toolbar buttons also get the focus and you can expand them by pressing the right arrow.
- Added possibility to move the timeline from anywhere in the interface, assuming the widget your mouse is currently hovering doesn't interpret those key-binds for another functionality.
- Focus is now indicated by a orange border around the widget that currently owns it.
- The node-graph navigator has been speeded-up, there was a bug where everything was rendered twice.
- A new button in the settings panel of a node can now hide all parameters that do not have any modification.
- It is now possible to close a settings panel by pressing the Escape key when hovering it
- When double-clicking a backdrop node and holding down the control key, this will clear all settings panel currently opened in the properties bin, and open the panels of all the nodes within the backdrop.
- It is now possible to zoom the range of a slider around the current position by holding certain modifiers: Hold CTRL to zoom-in x10, hold CTRL + SHIFT to zoom-in x100. This can help adjusting precisely a parameter value
- It is now possible to extract one or more nodes from the graph by pressing CTRL + SHIFT + X or via the right click menu. This will keep the connections between all extracted nodes but remove them from the surrounding tree and shift them by 200 pixels on the right.
- It is now possible to merge 2 nodes (with a MergeOFX node) by holding CTRL + SHIFT and moving a node close to another one. The bounding box of both nodes should be green, indicating that a merge is possible.
- A new Info tab on each node now contains informations about the format of images coming from each input and out-going from the node. This can help understand what data is really processed under the hood.
- New shortcut to set the zoom level of the viewer to 100% (CTRL + 1)
- New shortcut to toggle preview images for selected nodes: ALT + P
- Added the possibility to edit node names in the node graph by pressing the key N
- The backdrop node now has its name in the header affected by the font family and color. However the size of the name is controlled by a different parameter so the name and the content can have different font sizes.
- Roto: when selecting points with the selection rectangle, only points that belong to selected curves will be eligible for selection, unless no curve is selected, in which case all the points are eligible.
- Roto : when selecting points with the selection rectangle, if the SHIFT modifier is held down, it will not clear the previous selection. Also when holding down SHIFT, clicking on a selected point will remove it from the selection. Similarly, if selecting points with the selection rectangle but while holding down both SHIFT and CTRL will now allow to keep the previous selection but remove from the selection the newly selected points.
- Roto : the beziers animation can now be controlled in the curve editor, as well as the per-shape parameters.
- The ColorCorrect and Grade nodes can now choose on which channels to operate on, including the alpha channel
- The viewer info bar font has been changed to the same font of the rest of the application and a line of 1 pixel now separates the Viewer from the informations.
- The viewer refresh button will now be red when it is actively rendering (not using the cache).
- The turbo-mode button is now next to the FPS box of the viewer’s player bar instead of being on top of the properties bin.
- The paste/duplicate/clone nodes operations now position new nodes under the mouse cursor.
- The D (for disabling nodes) and CTRL+F (for finding nodes) shortcuts are now available across the UI. The same thing was applied for timeline interaction (such as seek next/previous/start/end,etc…) wherever it was possible.
- New Blender OpenColorIO-Config which is now the default one instead of nuke-default. Warning dialogs will show up if the current configuration is not the default one.
- New OCIOLook node to apply a look to the image. This node uses the 67 looks provided by the Blender OpenColorIO config.
- New luma Keyer node
- New ramp node
- New BlurGuided node using the CImg library. This is a faster bilateral blur
- New AdjustRoD node to enlarge/shrink the region of definition (bounding box) of an image
- A new button on the Transform node allows to recenter the handle to the center of the input image.
- The project can now be versioned by pressing CTRL + ALT + SHIFT + S (or by the File menu). This will save the project and increment its version, e.g.:
  - MyProject_001.ntp
  - MyProject_002.ntp
  - ...
- The Tab dialog to create quickly a node now remembers the name of the last node that was created
- A BlurCImg node can now be created using the B key
- The right click menu of parameters now proposes 2 options for keyframes operations such as set key, remove key or remove animation. It can operate on either 1 dimension at a time or all dimensions at the same time.

### Bug fixes

- The tracker node now once again works on Windows.
- Fixed a bug where color dialogs wouldn't refresh the color until the OK button was pressed
- Fixed a bug where the bounding box of the image wasn't correct when using motion blur of the Transform node.
- Fixed a bug where the bounding box of roto beziers wouldn't take the feather distance into account.
- Fixed a bug where changing the number of panels in the property bin wouldn't have any effect
- Fixed a bug where CImg channels parameter wouldn't be saved into the project
- The ColorLookUp node’s curve editor now properly gets keyboard focus


## Version 1.0.0rc2

- New nodes using the CImg library :
  - Blur
  - Dilate
  - Erode
  - Bilateral
  - Noise
  - Denoise
  - Plasma
  - Smooth
  - Equalize
  - New Clamp node
- Re-written internal rendering engine. It now uses CPU at almost 100%. The default settings
in the preferences are set to tune the render engine for basic usage and are optimal 
for a station with 8 cores. Some tuning may be needed for stations with huge number of cores
(24, 32...).
- Here's a description of the new settings controlling the multi-threading in Natron, use it wisely:
  - Number of render threads: This will limit the number of threads used by any rendering globally. By default this will use the number of cores of your CPU, indicated by the value 0.
  - Number of parallel renders: New in RC2, Natron now parallelise the rendering of frames by rendering ahead during playback or rendering on disk.  By default this is adjusted automatically and synchronised with the Number of render threads parameter so that it never exceeds the CPU maximum number of cores. Setting a value different than 0, will set the number of frames that will be computed at the same time.
  - Effect use thread-pool: New in RC2, Natron uses internally what is called a thread-pool to manage all the multi-threading and to ensure we never overload the CPU. This is a very efficient technology to avoid the system to spend to much time in scheduling threads by recycling threads. Unfortunately, some plug-ins expect threads to be fresh and new before they start rendering and using the application's thread-pool will make them unstable. In particular all plug-ins of The Foundry Furnace product seems to crash if used in a thread-pool environment. We recommend to uncheck this setting before using any of The Foundry Furnace plug-in. Natron will warn you if the setting is checked and you attempt to use any of The Foundry Furnace plug-in.
  - Max threads usable per effect: New in RC2, this controls how many threads a single node can use to render an image. By default the value internally is set to 4 if you select 0. Beyond that value we observed degraded performance because the system spends most of its time scheduling the threads rather than doing the actual processing. You may however set the value as you wish for convenience.
- The background renderer can now take an optional frame range in parameter with the following syntax:
  - NatronRenderer —w MyFirstWriter 10-40 -w MySecondWriter MyProject.ntp
  - If no frame range is specified, the whole frame range specified by the node will be rendered.
- The setting "Render in a separate process" is now off by default as it seems that rendering live is always a little bit faster due to the setup time of the second process.
- RGBLut plug-in has been renamed to ColorLookUp and now allows arbitrary input values range (instead of clamping input values to 0-1) and can now clamp output values with specific parameters (Clamp black/Clamp white). The curves can now be constrained to move either in X or Y direction by holding the control key.
- The keyframes in the curve editor and in the ColorLookUp plug-in can now have their value set by double clicking on their text. This also applies to the derivatives.
- Fixed the file dialog by changing radically its implementation making it faster and more reliable.
- Added a new Find node dialog that is accessible by pressing CTRL-F in the node graph or by the right click menu
- Natron will read correctly the pixel aspect ratio of the images and display them correctly. Due to an OpenFX limitation, if several images in a sequence don't have the same pixel aspect ratio, only the pixel aspect ratio of the first image in the sequence will be taken into account. This cannot be dynamic.
- When reading RGB opaque images, and using them as RGBA, Natron will set the alpha channel to 0 to match what other compositing softwares do. In some rare cases there might be issues with premultiplication state in some filter plug-ins. In particular the unpremult parameter of the filter nodes (such as Grade) will be checked and can cause a black image. If you really do not want to have support for RGB images, you can disable RGB support in the General preferences of Natron.
- Roto node will now output RGBA data by default. You can switch it to Alpha only when editing masks for maximum efficiency.
- Interface clean-up: We adjusted all drop-down menus so that they look more consistent  with the rest of the interface and on Windows the menu bar has also been redesigned.
- You can now click in the navigator window of the node graph to move the center the node graph on a position
- MacOSX retina displays are not supported yet (icons needs to be all reshaped) hence we disabled high DPI support when Natron is used on a mac.
- Adjusted slightly the ReadFFMPEG node to be more tolerant with files that may have failed to be read in the past.
- Fixed a few bugs with project paths
- The user can now simplify a project path or make it relative or absolute by right clicking any 
filepath parameter.
- Fixed a few bugs with the render progression report
- Copy/Paste of a node will use an index-based copy differentiation instead of an incremental number of "- copy" appended to the original node name. For example: "Blur" "Blur.1" "Blur.2", etc...
- The project now contains more informations visible in the Infos tab of the project settings. In particular, we added the following:
  - Original author name (the person who created the project) (read-only)
  - Last author name (the person who last saved this project) (read-only)
  - Creation date (read-only)
  - Last save date (read-only)
- Added a comments area where you can write anything the user might want to write. For example it can be used to add the license of the project, give a description of what it does, etc...
- The viewer FPS is now properly saved into the project file.
- Natron is now more flexible for plug-ins that do not respect entirely the OpenFX
specification so that more plug-ins can be used in Natron. In particular you can now
use the TuttleHistogramKeyer node.


## Version 1.0.0rc1

- New ResizeOIIO node to reformat footage. It internally uses the OpenImageIO library and offers numerous optimized filters. One should prefer Lanczos filter for downscaling and Blackman-harris filter for upscaling.
- New ColorMatrix node to apply a matrix to colors
- New TextOIIO node to render some text. The node is not finished and you currently have to set the name of a system font yourself. We will try to improve the node later on and propose more options.
- Rework of all the alpha premultiplication state of the workflow in Natron:
  1. All readers now have a "Premultiplied" choice, that is automatically set to what's found in the image file header. However some images are badly encoded and say they are not premultiplied but in fact they are and vice versa. To overcome the issue, the user can still modify the parameter if it is obviously wrong. You can assume that *readers always output alpha premultiplied images* (or opaque)!
  2. Writers have the same parameter and work kind of the same way: It detects what's the pre-multiplication state of the input-stream and set automatically the value it has found. However, again it can be wrong, so the user is free to modify the parameter.
  3. Readers output pixel components can now be specified. They will be set by default when loading and image, but the user can control it.
- Writers can now choose the format to render
- New node-graph bird view, though you cannot interact with it yet.
- New "turbo-mode": when enabled nothing but the viewer will be refreshed during playback for maximum efficiency. Mainly this overcomes the issue that all parameters animation refresh can be expensive during playback.
- New checkerboard mode for the viewer: when checked, everything transparent in the image will have a checkerboard behind. This can enable you to better appreciate transparency. You can control the color of the checkerboard and its size in Natron's preferences.
- Project paths: The project can now have path variables that allows you to express file paths relative to a variable instead of always using absolute file paths. This is great for sharing work with others and/or moving image files safely.By default there is always the variable [Project] that will be set to the location of your project file (.ntp).
- Project paths are to be used between brackets, e.g.:
  - [MyPath]myImage.png
- You can also express files in a relative manner to a variable, e.g.:
  - [MyPath]../../Images/myImage.png
- By default if you don't specify any project path but express the file in a relative manner, it will be understood as being relative to the [Project] path. For example, if [Project] is:
`/Users/Me/Project/`
Then
`../Images/Pictures/img.png`
would in fact expand to: `/Users/Me/Images/Pictures/img.png`
You can also have some project paths depending on other paths, e.g.:
```
[Project] = /Users/Me/Project/
[Scene1] = [Project]Footage/Scene1/
[Shot1] = [Scene1]001/
```
If you're using project paths, be sure to use them at the start of the file-path and no where else. For example
`C:\Lalala[Images]Images.png` would not be interpreted correctly.
Lastly, in the preferences of Natron there is an option in the General tab (disabled by default) to auto-fix file paths when you change the content of a project path or the variable name. 
For example if you change
`[Project] = /Users/Me/Project/`
to 
`[Project] = /Users/Me/MySuperProject/`
Then all file-paths that depended on this variable will be updated to point to the correct location of the file.
- File-paths are now "watched" by Natron: When a file changes outside of Natron, for example because another software have overwritten it or modified it, Natron will be notified and will reload the images.
- New shortcuts editor: you can now customise the entire keyboard layout for the application.
- Caching has been improved and should now be more aware of your system capabilities. It should strive for being as close possible to what the settings in the preferences are set to. Note that it might still need some tweeking on some platforms though because we're using system-dependents informations to make the Cache work.
- You can now add control points/keyframes to curves by double-clicking on them. However, it doesn't apply for the Roto beziers.
- File dialog speed has been increased drastically
- ReadFFMPEG: improve stability a lot
- Parameters links and expressions are now displayed in the nodegraph (you can disable them in the right click menu).
- File Dialog: New preview viewer that allows you to visualise quickly what the selected image file/sequence corresponds to.
- Saving application's layout: You can now import/export the application's layout in the Layout menu. You can also change the default layout by setting the parameter in the preferences to point to a valid natron's layout file (.nl).
- Node presets: You can now import/export node presets. You can then re-use any configuration on any other project and share it with others.
- The command line renderer is back! You can pass it a project filename in arguments and render with it. For the final release we will also add the possibility to set a frame range for a writer.
- TrackerNode: more export modes: basically we added the possibility to export to a CornerPin where the "to" points would be linked against the tracks. We also added a stabilise mode which is in fact just a regular export to CornerPin with the "Invert" parameter of the CornerPin checked.
- Undo/Redo support was added for restoring default parameters
- The "link to..." dialog for parameters have been re-designed for a better user interface
- The "Tab" menu to quickly create a node has been slightly adjusted: you can now type in
anything any word , e.g. "Blur" and it will propose all types of Blurs.
- Value parameters can now have their increment based on where the mouse cursor is
- New nodes menu icons were made by Jean Christophe Levet.


## Version 0.96

- Roto can now output RGBA colors and all beziers have a blending mode. It is now easier to layer your work within the same roto node.
- New Dot node to break connections and make cleaner graphs
- New OCIOCDLTransform to apply an ASC Color decision list, it also supports the .ccc (Color correction collection) or .cc (Color correction) file formats.
- New OCIOFileTransform to apply a transform from a given file. Supported formats:
  - .3dl (flame or lustre)
  - .ccc (color correction collection)
  - .cc (color correction)
  - .csp (cinespace)
  - .lut (houdini)
  - .itx (iridas_itx)
  - .cube (iridas_cube)
  - .look (iridas_look)
  - .mga (pandora_mga)
  - .m3d (pandora_m3d)
  - .spi1d (spi1d)
  - .spi3d (spi3d)
  - .spimtx (spimtx)
  - .cub (truelight)
  - .vf (nukevf)
- Natron now includes Aces 0.7.1 OpenColorIO profiles
- Can now read multilayer/multiview images using ReadOIIO (the standard image reader plugin in Natron) for formats that support it (such as OpenEXR or Photoshop PSD). ReadOIIO now lets you select the layer/view/channel for each output component.
- Multiple nodes can now be selected/edited in the nodegraph at the same time. Hold CTRL-A to select all nodes, CTRL-SHIFT-A to select only visible nodes in the visible portion
- The “Tab” shortcut menu is now more intuitive and easy to use
- New shortcuts to create basic nodes: - O : Roto - C : ColorCorrect - G : Grade - T : Transform - M : Merge
- In addition the “L” shortcut allows you to rearrange automatically the layout of the selected nodes.
- New Tracker node: 4 different algorithms can be used to track points. Tracks can be created and controlled directly from the Viewer. You can then export the results in a CornerPin node. (Transform export will be included later).
- In addition for convenience you can also link the control points of the Roto splines directly to a given track. The tracker nodes accepts a mask in input: When tracking points inside a roto shape, plug that same shape as a mask of the tracker to help the tracker giving better results.
- The cache has been greatly improved so it is more clever with the usage of the memory and can be cleared totally using the menu option.
- The green-line on the timeline now has 2 different colours:
- Dark green meaning that the frame is cached on the disk
- Bright green meaning that the frame is present in memory (RAM) New tools icons in the interface (thanks to Jean Christophe Levet) Combobox (Choice) parameters now display in their tooltip the help for all the choices so it is easier to understand the parameter than to cycle through all the list 1 by 1
- Fixed multiple bugs in the curve editor that prevented to select and move several keyframes
- Fixed a bug where some image sequences wouldn’t be recognised correctly
- Fixed stability of the software across the board
- Improved the efficiency of the rendering engine across the board, making it faster and more robust to some rare configurations
- Linux builds now offer a complete installer and maintenance tools allowing to install/uninstall/update the software easily (thanks to Ole Andre)
- Linux 32 bit platforms are now also officially supported.


## Version 0.95

- This was shortly after 0.94 but we needed to deploy various fixes to the major bugs that were mainly impairing usability of the software on Linux distributions.
- This is a minor patch but it makes Natron much more stable than in 0.94.
- New setting in the preferences to allow Natron to only render once you finish editing a parameter/slider/curve
- New right click menu for roto splines/control points
- The RGBLut node now has a master curve and an alpha curve
- New shift-X shortcut on nodes to quickly switch their input A and B
- The project loader is now more flexible and will still try load a project even if it detects some incompatibilities or corrupted file.
- Fixed a bug in the render engine which would cause a freeze of the application.
- Fixed a bug where images from the cache wouldn't go away
- Fixed multiple crashs on exit
- Fixed compatibility with Qt4 on Linux (this bug was not Linux specific but only noticed on this platform).
- Fixed a bug with the colour parameters that would not provide to the plug-in the correct values.
- Fixed a bug in the Transform interact that would arise when the user would be zoomed in
- Fixed a bug where the background renderer would crash
- Improved performance in the refreshing of the graphical user interface of the parameters, it should increase the performances of Natron across the board.


## Version 0.94

- Windows users are now proposed a regular installer and double clicking .ntp projects icons will now open them within Natron.
- New premult and unpremult nodes.
- The cache received some enhancements, as a result of it Natron should consume less memory. This should also speed up the rendering engine as a whole. Rendering complex graphs of 4K sequences should now almost always fit in the cache size limit and be faster.
- Nodes now have a color according to their role (Channel processing, Color processing, Filter, Merge, etc...)
- The roto points can now be moved with the keyboard arrows by a constant size depending on the viewport scale factor.
- The color picker on the viewer will now show pixel components according to the components present in the image being viewed.
- Added a setting in the preferences panel allowing to change the application's name as seen by the OpenFX plug-ins. This can help loading plug-ins that wouldn't load otherwise because they restrict their usage to specific applications.
- Invert/Transform/CornerPin/Merge nodes can now invert their mask
- Holding control while moving a backdrop now allows to move only the backdrop without moving the nodes within it.
- Connecting a viewer to the selected node using the 1,2,3,4... shortcuts will now select back the viewer afterwards so that the user doesn't have to deselect the node to switch quickly between inputs.

### Bug fixes

- Fixed preview images rendering
- Fixed a bug where the curves in the RGBLut plug-in would not allow to move the control points in more than 1 direction at a time.
- Fixed a crash that would occur when drag&dropping images within Natron. The same bug would also make Natron crash in some other cases.
- Fixed a bug where inverting alpha channel would not work
- Fixed bugs where timeline keyframe markers wouldn't reflect the actual keyframes
- Fixed the cache green line on the timeline so it is more in-line with what really is in the cache.
- Fixed a bug in the color correct node where the saturation would not be applied correctly
- Fixed a bug in the transform/corner pin nodes that wouldn't work when proxy mode would be activated.
- Fixed the ReadFFMPEG node as a whole, it should now work properly.
