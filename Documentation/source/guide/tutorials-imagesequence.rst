.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

How to convert videos to image sequences
========================================

Natron works best when you use image sequences as input.

Video can be used (mp4, mov etc) as input but you may face stability issues.

Converting your video to a sequence of images is recommended.

There are a number of solutions for converting your video to frames:


FFmpeg
~~~~~~

`FFmpeg <https://ffmpeg.org/>`__ provides a convenient command-line solution for converting your video to images.

1. Open a terminal and navigate to the folder containing the video
2. Use this command to extract the video to a png image sequence:

``ffmpeg -i input.mp4 -pix_fmt rgba output_%04d.png``

Replace ``input.mp4`` with the name of your video and ``output_`` with the name you want to call your files.

``%04d`` specifies the position of the characters representing a sequential number in each filename matched by the pattern. Using the above example the output files will be called ``output_0001.png``, ``output_0002.png``, ``output_0002.png`` and so on. For longer videos you will need to use a higher number (``%08d.png``).

If you have a video with an alpha channel, you can extract an image sequence with an alpha channel with the alphaextract option:

``ffmpeg -i input.mov -vf alphaextract output_%04d.png``

To extract TIFF 16 bit image sequence:

``ffmpeg -i input.mov -compression_algo lzw -pix_fmt rgb24 output%04d.tiff``

If alpha is needed:

``ffmpeg -i input.mov -compression_algo lzw -pix_fmt rgba output%04d.tiff``

.. note:: "-compression_algo packbits or raw or lzw or deflate" - is optional. Using it for 4k/+ is recommended. For 4k/+ you can use deflate. For HD you can use lzw to lower the file size.


.. note:: "-pix_fmt rgb24 or rgba" is must to include convert the color space. YUV/YCRB is not ideal for many en/decoders for TIFF.


more info: ffmpeg -v error -h encoder=tiff

FFmpeg GUI
~~~~~~~~~~
If you prefer to use a graphical user interface (GUI) instead of the terminal, try using  `ffmulticonv <https://sourceforge.net/projects/ffmulticonv/>`__ (Linux) or `WinFF <https://github.com/WinFF/winff>`__ (Windows and Mac)


More information of FFmpeg's command line options https://ffmpeg.org/ffmpeg-formats.html


Kdenlive/Shotcut
~~~~~~~~~~~~~~~~
https://kdenlive.org/
(version used here 20.07.70)
https://shotcut.org/

With your video(s) on the timeline go to Project > Render. 
In the render settings choose Images sequence and select your desired image format.

Your sequence will be output with your specified file name and format and using five digits for its numbered sequence (e.g. output_00001.png).

Full instructions on how to use Kdenlive can be found here https://userbase.kde.org/Kdenlive/Manual/Project_Menu/Render

Blender
~~~~~~~
https://www.blender.org/

- Import the movie file in Blender Video Sequencer.
- Go to color management>view transform>standard
- Go to render tab and select PNG/TIFF
- Select RGB/RGBA, 8/16 Color depth, and preferred compression NONE/Any.

Full instructions on how to use the Blender VSE can be found here https://docs.blender.org/manual/en/latest/video_editing/index.html


Adobe Media Encoder
~~~~~~~~~~~~~~~~~~~
- Open media encoder
- Add source video to the queue
- Set the output format to OpenEXR.
- Set compression to "Zip"
- If your source has an alpha channel be sure to scroll down to the bottom of the Video section of the Export Settings and check "Include Alpha Channel"
- Close the Export Settings by clicking Ok and press the Start Queue button.

How To Convert Image Sequences To Video Files
=============================================

FFmpeg
~~~~~~
Converting your images to video follows a similar process to doing the reverse. 

Open a terminal and navigate to the location containing the images. 
In the terminal type:

``ffmpeg -i input_%05d.png output.mp4``

Change ``input_`` to match the name of your files. The number of characters in the quence (%05d) should match the amount in your input files. For example, if your files have four characters in their sequence (e.g. input_0001.png) then you should use %04d. 

For this to work correctly all of your files need to be sequentially numbered and the sequence should start from either 0 or 1.


You can also specify the framerate and the codec, here is an example for framerate 30fps:

ffmpeg -framerate 30 -i input%04d.png -c:v libx264 -r 30 -pix_fmt yuv420p out.mp4



Kdenlive
~~~~~~~~

Shotcut
~~~~~~~

Da Vinci Resolve
~~~~~~~~~~~~~~~~

Blender VSE
~~~~~~~~~~~
For Blender VSE, (go to render tab>file format FFmpeg Video>your preferred codec and container)

Adobe Premiere
~~~~~~~~~~~~~~
Import the image "as sequence" in the timeline and render in your preferred video format. 


Creating Digital Intermediate For Editing Servers
=================================================

For Digital Intermediate, the `PRORES 4444` codec is a nice choice for MOV containers. It supports 12-bit with YUVA and retains alpha with 16-bit precision.

You can do it with ffmpeg or in kdenlive/shotcut importing the TIFF/PNG as sequence.


FFmpeg
~~~~~~
``ffmpeg -framerate 30 -i input%03d.tiff -f mov -acodec pcm_s16le -vcodec prores_ks -vprofile 4444 -vendor ap10 -pix_fmt yuva444p10le out.mov``

Shotcut/Kdenlive
~~~~~~~~~~~~~~~~
You need to create a render profile first with below profile


``f=mov acodec=pcm_s16le vcodec=prores_ks vprofile=4444 vendor=ap10 pix_fmt=yuva444p10le qscale=%quality``


Use TIFF/PNG image as sequence in the timeline.
Then Render with this newly created prores 4444 profile.

A Tutorial on PRORES in LINUX by CGVIRUS:
https://youtu.be/oBiaBYthZSo

It can be done with Adobe Premiere/avid/fcpx/resolve etc as well by importing TIFF/PNG as sequence and render as MOV prores 4444.

DaVinci Resolve, Adobe Premiere etc
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Drag and drop the folder containing the image sequence to a timeline.
Render your timeline in PRORES 4444.

Natron
~~~~~~
The preferred file format to render out composited frames is TIFF.(image attached):

In the write node:

output components can be RGB(no transparency) or RGBA(with transparency)
Use yourfilename_###.tiff (where # is the frame number and padding) 
### will create yourfilename001.tiff and ## will create yourfilename01.tiff
Bit depth can be auto/8i/16i (Don't use float)
compression can be none/lzw (HD). for 4k deflate is ok.

PNG is also a good format:

In the write node:

output components can be RGB(no transparency) or RGBA(with transparency)
Use yourfilename###.png (where # is the frame number and padding) 
### will create yourfilename001.png and ## will create yourfilename01.png
Bit depth can be 8/16bit
compression can be 0 for HD, 6 for 4k is fair enough.




Open Questions for this document:
What format should I use for frames? (esp if the video is 10bit or 12bit) ?

Suggestion:
For muxing audio. But it is usually pointless as it goes to NLE at the end.
