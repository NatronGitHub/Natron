.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf


.. _tutorials-imagesequence:

Converting between videos and image sequences
=============================================

Natron works best with image sequences: one file per frame, such as
``shot_0001.png``, ``shot_0002.png``... It can read videos (MP4, MOV...), but
decoding them is slower and less reliable, especially with codecs that
compress across frames, such as H.264 and H.265 (see
:ref:`compositing-projects-readers-sequences-first`). The usual workflow is:

1. convert the video to an image sequence;
2. composite the sequence in Natron, and write the result as another image
   sequence;
3. encode that sequence to a video, and add the sound back.

Video to image sequence
-----------------------

With FFmpeg
~~~~~~~~~~~

`FFmpeg <https://ffmpeg.org/>`__ converts a video to images from the command
line. Open a terminal in the folder of the video, and run:

.. code-block:: bash

   ffmpeg -i input.mp4 -pix_fmt rgb24 output_%04d.png

Replace ``input.mp4`` with the name of the video, and ``output_`` with the
name of the images. ``%04d`` is the frame number, padded with zeroes to four
digits: the files are ``output_0001.png``, ``output_0002.png``,
``output_0003.png``, and so on. Four digits are enough for 9999 frames (more
than six minutes at 25 frames per second), and larger numbers simply get more
digits.

``-pix_fmt`` chooses the pixel format, i.e. the channels and the bit depth of
the images. Videos are usually stored as Y'CbCr (YUV), often with fewer
chroma samples than pixels, and ``-pix_fmt`` converts them to RGB. For TIFF it
is required: without it, FFmpeg writes a Y'CbCr TIFF (4:2:0 for most videos),
which most applications can't read.

=====================  =======================  ========================
Format                 8 bits per channel       16 bits per channel
=====================  =======================  ========================
PNG, RGB               ``-pix_fmt rgb24``       ``-pix_fmt rgb48be``
PNG, RGBA              ``-pix_fmt rgba``        ``-pix_fmt rgba64be``
TIFF, RGB              ``-pix_fmt rgb24``       ``-pix_fmt rgb48le``
TIFF, RGBA             ``-pix_fmt rgba``        ``-pix_fmt rgba64le``
=====================  =======================  ========================

Use 16 bits for videos with more than 8 bits per channel (10-bit or 12-bit
ProRes, DNxHR, HEVC...). The alpha channel (RGBA) is only useful if the video
has one, as some ProRes 4444 files do.

PNG and TIFF are both lossless: their compression only changes the size of
the files and the time it takes to write and read them, never the image. For
TIFF, ``-compression_algo`` chooses the compression (``packbits``, ``raw``,
``lzw`` or ``deflate``), for example:

.. code-block:: bash

   ffmpeg -i input.mov -pix_fmt rgb48le -compression_algo deflate output_%04d.tif

For the pixel formats and options of each encoder, run
``ffmpeg -h encoder=png`` or ``ffmpeg -h encoder=tiff``, or see the
`FFmpeg codecs documentation <https://ffmpeg.org/ffmpeg-codecs.html>`__.

With an editing or 3D application
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Most video applications can also render image sequences. Their menus change
from one version to the next: check the manual of your version.

- **Kdenlive:** put the video on the timeline, choose **File** > **Render**,
  then an image sequence preset, such as PNG (see the
  `Kdenlive manual <https://docs.kdenlive.org/en/exporting/render.html>`__).
- **Shotcut:** put the video on the timeline, open the **Export** panel, and
  choose a preset of the **Stills** category, such as PNG or TIFF.
- **Blender:** in the Video Sequencer, add the movie, and set the frame range,
  resolution and frame rate of the scene to those of the movie. In the
  **Output** properties, set the output path and the **File Format** (PNG, TIFF or
  OpenEXR), with RGB or RGBA and the color depth. In the **Render**
  properties, set **Color Management** > **View Transform** to **Standard**,
  so that the colors of the video are not changed (the default is AgX in
  Blender 4, Filmic in Blender 2.8 to 3.6). Then choose **Render** >
  **Render Animation**.
- **DaVinci Resolve:** on the **Deliver** page, choose a custom render with
  the TIFF, DPX or OpenEXR format, add it to the render queue, and render.
- **Adobe Media Encoder:** add the video to the queue, set the format to
  OpenEXR (or TIFF, PNG), and check **Include Alpha Channel** at the bottom of
  the Video settings if the video has an alpha channel. Then start the queue.

Writing image sequences from Natron
-----------------------------------

The Write node writes one file per frame when its file name contains a frame
number pattern:

- ``#`` characters give the padding: ``shot_###.tif`` writes
  ``shot_009.tif`` for frame 9, and ``shot_####.tif`` writes
  ``shot_0009.tif``;
- a single ``#`` means no padding: ``shot_#.tif`` writes ``shot_9.tif``;
- the ``printf`` syntax also works: ``shot_%04d.tif`` writes
  ``shot_0009.tif``.

TIFF and PNG are good choices for 8-bit and 16-bit images, and OpenEXR for
floating-point images (values below 0 or above 1, linear light):

- **TIFF** (:ref:`WriteOIIO <fr.inria.openfx.WriteOIIO>`): **Output
  Components** RGB, or RGBA to keep the alpha; **Bit Depth** 8i or 16i
  (``auto`` writes 16 bits for TIFF); and a lossless **Compression**:
  ``default`` (``zip``), ``none``, ``lzw`` or ``packbits``. Avoid ``jpeg``,
  which is lossy.
- **PNG** (:ref:`WritePNG <fr.inria.openfx.WritePNG>`): **Output Components**
  RGB or RGBA; **Depth** 8-bit or 16-bit; and a **Compression Level** from 0
  (none, fastest) to 9 (smallest files), 6 by default. Every level gives the
  same image.

See :doc:`compositing-projects-render` for rendering from the GUI and from
the command line.

Image sequence to video
-----------------------

FFmpeg encodes an image sequence back to a video:

.. code-block:: bash

   ffmpeg -framerate 24 -i output_%04d.png -c:v libx264 -pix_fmt yuv420p output.mp4

- ``-framerate`` gives the frame rate of the sequence: use that of the
  original video (``ffprobe input.mp4`` shows it; write 23.976 as
  ``24000/1001``), or the sound won't stay in sync. Without it, FFmpeg uses 25
  frames per second. It must come before ``-i``.
- The pattern after ``-i`` must match the file names, with the same number of
  digits: ``%04d`` for ``output_0001.png``, ``%05d`` for ``output_00001.png``.
- The frames must be numbered without gaps. FFmpeg looks for the first frame
  between 0 and 4: for a sequence that starts at another number, such as
  1001, add ``-start_number 1001`` before ``-i``.
- ``-pix_fmt yuv420p`` makes an H.264 video that every player can play; the
  width and height must then be even. Without it, FFmpeg keeps the full color
  resolution of RGB images (4:4:4), which many players can't play.

To encode a digital intermediate (ProRes, DNxHR, FFV1) for editing, see
:doc:`tutorials-ffmpeg-di`. To add the sound of the original video, see
:doc:`tutorials-ffmpeg-audio`.

Editing applications (Kdenlive, Shotcut, DaVinci Resolve, Adobe Premiere)
can also import an image sequence as a clip, usually with an "image
sequence" option in their import dialog, and render it to any video format.

See also
--------

- :doc:`thenodes-image-read`: the parameters of the Read node.
- :ref:`compositing-projects-readers-sequences-first`: why Natron works better
  with image sequences.
- :doc:`compositing-time`: trimming, shifting and retiming clips in Natron.
- `FFmpeg formats documentation <https://ffmpeg.org/ffmpeg-formats.html>`__:
  the ``image2`` demuxer and muxer, which read and write image sequences.
