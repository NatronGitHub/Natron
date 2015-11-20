# History

## Version 2.0 - RC4

- Write nodes now have a Frame Increment parameter which allows to skip frames while rendering.
- The command-line parameters and Python API have been updated in this regard (see Python documentation).
- Fix multiple bugs when rendering a multi-layered exr
- Linux: when crashing, Natron will now print a backtrace if launched from the Terminal. This is temporary and will be soon replaced by a cross-platform crash reports handler.
- RotoPaint: enhancements in the rendering algorithm 
- Color, Double and Integer parameters can now have an expression entered directly in a SpinBox for convenience
- NodeGraph: optimize for speed when the scene contains a lot of nodes and add auto-scrolling when reaching the border of the view

## Version 2.0 - RC3

- The Read node can now read multi-view EXR files. Decoding speed of EXR has been greatly improved.
- The Write node can now write multi-view and multi-layered EXR files. Encoding speed of EXR has been greatly improved.
- Viewer: The channel selected in the "Alpha channel" drop-down can now be overlayed on the image when using the "Matte" display channels
- The RotoPaint/Roto Transform and Clone tabs now have a viewer handle overlay to help manipulating the parameters
- DopeSheet and CureEditor: The scaling interactions with the bounding box of selected keyframes are now correct but may lead to keyframes being located at floating point times. 
- A "P" indicator at the bottom right of a node will indicate that a node is a pass-through. Hovering the mouse over the "P" will indicate from which input this node is a pass-through of.
The pass-through input will also be indicated with a plain arrow whereas all other inputs will have their arrow dashed. 
- Python API: it is now possible to retrieve the layers available on a node with the getAvailableLayers() function, see [Python Reference](http://natron.rtfd.org)


## Version 2.0

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

– New GodRays and DirBlur nodes.
– New RGBToHSI/HSIToRGB YUVToRGB/RGBToYUB YCbCrToRGB/RGBToYCbCr nodes
– Fixed a bug where some TIFF files would not read correctly on Windows versions of Natron
– FIxed a bug where a crash could occur with the Merge node with mix = 0
– Fixed a bug where ReadFFMPEG would sometimes decode incorrectly files with bit depth higher than 8
– Miscellaneous stability fixes

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
    * The node is using several images at different times to compute the result of the output (e.g: a retimer node)
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
- Roto: It is now possible to move a bezier just by dragging a part of the curve where there is no control point.
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

## Version 1.0.0RC3

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
- The project can now be versioned by pressing CTRL + ALT + SHIFT + S (or by the File menu). This will save the project and increment its version, e.g:
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

## Version 1.0.0RC2

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


## Version 1.0.0RC1

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
- Project paths are to be used between brackets, e.g:
  - [MyPath]myImage.png
- You can also express files in a relative manner to a variable, e.g:
  - [MyPath]../../Images/myImage.png
- By default if you don't specify any project path but express the file in a relative manner, it will be understood as being relative to the [Project] path. For example, if [Project] is:
`/Users/Me/Project/`
Then
`../Images/Pictures/img.png`
would in fact expand to: `/Users/Me/Images/Pictures/img.png`
You can also have some project paths depending on other paths, e.g:
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
anything any word , e.g: "Blur" and it will propose all types of Blurs.
- Value parameters can now have their increment based on where the mouse cursor is
- New nodes menu icons were made by Jean Christophe Levet.

Natron 0.96 "Beta"
-----------------

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
- New shortcuts to create basic nodes: – O : Roto – C : ColorCorrect – G : Grade – T : Transform – M : Merge
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

## Bug fixes:

- Fixed preview images rendering
- Fixed a bug where the curves in the RGBLut plug-in would not allow to move the control points in more than 1 direction at a time.
- Fixed a crash that would occur when drag&dropping images within Natron. The same bug would also make Natron crash in some other cases.
- Fixed a bug where inverting alpha channel would not work
- Fixed bugs where timeline keyframe markers wouldn't reflect the actual keyframes
- Fixed the cache green line on the timeline so it is more in-line with what really is in the cache.
- Fixed a bug in the color correct node where the saturation would not be applied correctly
- Fixed a bug in the transform/corner pin nodes that wouldn't work when proxy mode would be activated.
- Fixed the ReadFFMPEG node as a whole, it should now work properly.

We will enhance the WriteFFMPEG node in the future by adding presets so it is
less complicated to handle all the parameters of that node.

