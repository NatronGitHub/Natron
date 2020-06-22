.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Project setup
=============

Although Natron will automatically adjust the project settings when you first import a media using a Read node, you should first define the settings for it.

1. When you first opened Natron, the Project Settings panel will appear. The Project Settings panel can be accessed manuallt from Display > Show Project Settings or by pressing "S" on the keyboard. From here you can adjust the the values to your needs.
2. (Project Paths).

.. image:: _images/project_setup_Settings.png
 :width: 600px

Output Format
~~~~~~~~~~~~~
1. On the Settings tab, select the resolution for the canvas on the viewer from the Output Format dropdown menu. If the format you want to use is not in the menu, select New Format.
2. You can copy format from any viewer by selecting the viewer and choose Copy From.
3. Define the width and height of the format in the w and h fields.
4. You can also define pixel aspect ratio.
5. In the name field, enter a name for the new format.
6. Click OK to save the format. It now appears in the dropdown menu where you can select it.

Frame Range and Frame Rate
~~~~~~~~~~~~~~~~~~~~~~~~~~
1. Type the numbers of the first and last frames in the Frame Range fields to define length of time for your project.
2. In the Frame Rate field, enter the rate in frames per second (fps) at which you want your viewers to play back footage.

(Need help with project paths content).

.. toctree::
   :maxdepth: 2