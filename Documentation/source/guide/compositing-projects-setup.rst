.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Project setup
=============

Although Natron will automatically adjust the project settings when user first imports a media using a Read node, one should first define the settings for it.

1. The Project Settings panel can be accessed manually from Display > Show Project Settings or by pressing "S" on the keyboard. From here one can adjust the values to the needs.
2. The writer never used Project Paths. Feel free to contribute.

.. image:: _images/project_setup_Settings.png
 :width: 600px

Output Format
~~~~~~~~~~~~~
1. On the Settings tab, select the resolution for the canvas on the viewer from the Output Format dropdown menu. If the desired format is not in the menu, select New Format.
2. User can copy format from any viewer by selecting the viewer and choose Copy From.
3. Define the width and height of the format in the w and h fields.
4. User can also define pixel aspect ratio.
5. In the name field, enter a name for the new format.
6. Click OK to save the format. It now appears in the dropdown menu where user can select it.

In the Node Graph, some nodes have preview images appear on them. The images can be automatically refreshed if user leaves the the Auto Previews checked.
.. image:: _images/compositing-Merging_images_00

Frame Range and Frame Rate
~~~~~~~~~~~~~~~~~~~~~~~~~~
1. Type the numbers of the first and last frames in the Frame Range fields to define length of time for your project.
2. In the Frame Rate field, enter the rate in frames per second (fps) at which you want your viewers to play back footage.

GPU Rendering
~~~~~~~~~~~~~
User can select when to activate GPU rendering for plug-ins. Note that if the OpenGL Rendering parameter in the Preferences/GPU Rendering is set to disabled then GPU rendering will not be activated regardless of that value.
Enabled: Enable GPU rendering if required resources are available and the plugin supports it.
Disabled: Disable GPU rendering for all plug-ins.
Disabled if background: Disable GPU rendering when rendering with NatronRenderer but not in GUI mode.

.. toctree::
   :maxdepth: 2
