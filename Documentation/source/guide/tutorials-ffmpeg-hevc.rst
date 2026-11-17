.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

.. _tutorials-ffmpeg-hevc:

Encoding HEVC
=============

HEVC (High Efficiency Video Coding, also called H.265) is a video codec for
distribution: at the same visual quality, its files are much smaller than
H.264 files. It supports 10-bit and 12-bit images, which avoids banding in
gradients, and it is the usual codec for HDR video.

HEVC is not a good format for intermediate files: most frames are predicted
from other frames, so decoding a given frame is slow, and the compression is
lossy. Render your compositing to an image sequence or to a digital
intermediate (see :doc:`tutorials-ffmpeg-di`), and encode the HEVC file from
it as the last step.

There are two ways to encode HEVC:

- with the WriteFFmpeg node in Natron, which is simple, but only exposes a few
  settings;
- by rendering an image sequence from Natron, then encoding it with the
  ``ffmpeg`` command-line tool, which gives access to all the encoder
  settings and can add audio. This is the recommended way.

Both use the x265 encoder (``libx265`` in FFmpeg).


Availability of x265
--------------------

x265 is distributed under the GNU GPL. Natron's GPL builds include FFmpeg
built with x265 (and x264 for H.264). Builds made with the ``COMMERCIAL``
license option leave out the GPL components, so they cannot write HEVC (or
H.264): use the ``ffmpeg`` command-line tool instead.

x265 can be built with 8-bit, 10-bit and 12-bit support, or with only some of
them. To see which pixel formats your copy supports, run:

.. code-block:: sh

   ffmpeg -h encoder=libx265

and look at the "Supported pixel formats" line: ``yuv420p10le`` means 10-bit
4:2:0 is available, ``yuv420p12le`` means 12-bit 4:2:0 is available. In
WriteFFmpeg, the **Show Avail.** button gives the same information.


Writing HEVC from Natron
------------------------

Use a Write node with a ``.mov``, ``.mp4`` or ``.mkv`` file name, select the
:ref:`WriteFFmpeg <fr.inria.openfx.WriteFFmpeg>` writer, and choose
``hev1 H.265 / HEVC (High Efficiency Video Coding) [libx265]`` in the
**Codec** menu. Then set:

- **Output Components**: ``RGB``. HEVC files for distribution have no alpha
  channel.
- **File Colorspace**: the display encoding of the video, usually Rec. 709
  (``nuke_rec709`` in the default OpenColorIO config).
- **Pref. Pixel Coding**: ``YUV420``. This is what players and web sites
  expect. The default, ``YUV422``, makes files that many players cannot
  decode.
- **Bit Depth**: ``10``, if your x265 supports it. Check the result in
  **Selected Pixel Coding**, which should read ``YUV420/10``. If it shows
  another format, such as ``YUVA420`` (with alpha), your x265 has no 10-bit
  support: set **Bit Depth** to ``8``.
- **Output Quality**: the quality target (x265's constant rate factor, CRF).
  ``High Quality`` (CRF 20) or ``Perceptually Lossless`` (CRF 17) are good
  choices for delivery. If you select ``None``, the **Bitrate** parameter is
  used instead.
- **Encoding Speed**: x265's preset. Slower presets make smaller files at the
  same quality. It is only used when **Output Quality** is not ``None``.

The **Keyframe Interval** and **Max B-Frames** parameters are not used with
x265.

The **Lossless** choice of **Output Quality** sets CRF 0, which is *not*
lossless with x265: it only gives the highest quality. For true lossless
encoding, also add ``x265-params=lossless=1`` in **Codec Options** (in the
**Advanced** group). **Codec Options** passes other encoder options too, for
example ``tune=grain`` or ``x265-params='keyint=48:bframes=4'`` (values that
contain a colon must be quoted).

The file is tagged ``hev1``. Apple's players (QuickTime Player, iOS, Safari)
only play HEVC tagged ``hvc1``. Change the tag without re-encoding with:

.. code-block:: sh

   ffmpeg -i input.mp4 -c copy -tag:v hvc1 output.mp4

WriteFFmpeg writes the color primaries and the transfer function that match
the **File Colorspace** (Rec. 709 for ``nuke_rec709``), but not the Y'CbCr
matrix. It converts RGB to Y'CbCr with the Rec. 709 matrix when the image
height is 720 or more, and with the Rec. 601 matrix below that. Players
usually guess the matrix right for HD images, but you can write it in the
file without re-encoding (this command also sets the ``hvc1`` tag):

.. code-block:: sh

   ffmpeg -i input.mp4 -c copy -bsf:v hevc_metadata=colour_primaries=1:transfer_characteristics=1:matrix_coefficients=1 -tag:v hvc1 output.mp4

The value 1 means Rec. 709 for all three. WriteFFmpeg does not write audio;
to add it, see :doc:`tutorials-ffmpeg-audio`.


Encoding an image sequence with ffmpeg
--------------------------------------

Render the images from Natron with a Write node (for example to 16-bit TIFF
or PNG files, with **File Colorspace** set to Rec. 709), then encode them.
The commands below read ``render.0001.tif``, ``render.0002.tif``, … at
24 fps. Replace ``-framerate 24`` with your project's frame rate (for example
``-framerate 24000/1001`` for 23.976 fps), and add ``-start_number 1001`` if
the first frame is 1001.

10-bit HEVC in MP4, for delivery:

.. code-block:: sh

   ffmpeg -framerate 24 -i render.%04d.tif -c:v libx265 -preset slow -crf 20 -pix_fmt yuv420p10le -vf "setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709" -tag:v hvc1 -movflags +faststart output.mp4

The options are:

- ``-c:v libx265``: encode with x265.
- ``-preset slow``: the speed/compression trade-off, from ``ultrafast`` to
  ``veryslow``. The default is ``medium``.
- ``-crf 20``: the quality. Lower values give better quality and larger
  files; 18 to 24 is the usual range. The default is 28.
- ``-pix_fmt yuv420p10le``: 10-bit Y'CbCr 4:2:0 (the "Main 10" profile). Use
  ``yuv420p`` for 8-bit ("Main" profile) if the file must play on old
  devices. ``yuv422p10le``, ``yuv444p10le`` and the 12-bit formats are also
  available, but few players support them.
- ``-vf "setparams=..."``: the color metadata, see below.
- ``-tag:v hvc1``: the tag that Apple's players need.
- ``-movflags +faststart``: put the index at the start of the file, so that
  playback over the network can start before the download ends.

To reach a given file size rather than a given quality, encode in two passes
with a target bit rate (here 8 Mbit/s):

.. code-block:: sh

   ffmpeg -framerate 24 -i render.%04d.tif -c:v libx265 -preset slow -b:v 8M -x265-params pass=1 -pix_fmt yuv420p10le -vf "setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709" -an -f null /dev/null

.. code-block:: sh

   ffmpeg -framerate 24 -i render.%04d.tif -c:v libx265 -preset slow -b:v 8M -x265-params pass=2 -pix_fmt yuv420p10le -vf "setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709" -tag:v hvc1 -movflags +faststart output.mp4

On Windows, replace ``/dev/null`` with ``NUL``. The first pass writes
``x265_2pass.log`` files, which you can delete afterwards.

Lossless HEVC (lossless with respect to the 10-bit Y'CbCr 4:4:4 images; the
conversion from RGB is not lossless):

.. code-block:: sh

   ffmpeg -framerate 24 -i render.%04d.tif -c:v libx265 -x265-params lossless=1 -pix_fmt yuv444p10le -vf "setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709" output.mkv

Other x265 settings can be given with ``-x265-params``, as a list of
``key=value`` pairs separated by colons. See the
`x265 documentation <https://x265.readthedocs.io/en/master/cli.html>`_ for
the list.

To add a sound track, add the audio file as a second input and encode it
too, for example ``-i audio.wav -c:a aac -b:a 192k`` after the first
``-i`` option. See :doc:`tutorials-ffmpeg-audio` for more.


Color metadata
--------------

Video files store three values that tell players how to display the images:
the color primaries, the transfer function (the "gamma"), and the matrix used
to convert between RGB and Y'CbCr. If they are missing, players guess, and
the colors or the contrast may look different from Natron's viewer.

The ``setparams`` filter sets them. For Rec. 709 video:

.. code-block:: sh

   -vf "setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709"

ffmpeg uses the ``colorspace`` value to convert the RGB images to Y'CbCr, and
writes all three values in the file. Without it, ffmpeg converts with the
Rec. 601 matrix and writes no metadata. Use ``setparams`` rather than the
``-color_primaries``, ``-color_trc`` and ``-colorspace`` output options: with
FFmpeg 9, ``-color_primaries`` and ``-color_trc`` were ignored when the input
was an image sequence.

These values only describe the images: they do not convert them. The images
must already be encoded the way the metadata says, which is why the
**File Colorspace** of the Write node that renders them matters.

To check the result:

.. code-block:: sh

   ffprobe -v error -select_streams v:0 -show_entries stream=codec_name,codec_tag_string,profile,pix_fmt,color_range,color_space,color_primaries,color_transfer -of default=noprint_wrappers=1 output.mp4

For the first command above, it prints:

.. code-block:: none

   codec_name=hevc
   profile=Main 10
   codec_tag_string=hvc1
   pix_fmt=yuv420p10le
   color_range=tv
   color_space=bt709
   color_transfer=bt709
   color_primaries=bt709

``unknown`` means that a value is missing. To fix the metadata of an existing
HEVC file without re-encoding, use the ``hevc_metadata`` bitstream filter, as
shown in the WriteFFmpeg section above. Its values are the numbers defined in
the HEVC standard: 1 is Rec. 709, 9 is Rec. 2020 (primaries), 16 is PQ
(transfer function) and 9 is Rec. 2020 non-constant luminance (matrix).


HDR10
-----

HDR10 files use 10-bit Y'CbCr 4:2:0, Rec. 2020 primaries, and the PQ (SMPTE
ST 2084) transfer function, plus metadata about the mastering display and the
brightness of the content. The images must be rendered from Natron already
encoded with PQ and Rec. 2020 primaries, which requires an OpenColorIO
configuration that has such a color space (the default configuration does
not). Then encode them with:

.. code-block:: sh

   ffmpeg -framerate 24 -i render.%04d.tif -c:v libx265 -preset slow -crf 18 -pix_fmt yuv420p10le -vf "setparams=color_primaries=bt2020:color_trc=smpte2084:colorspace=bt2020nc" -x265-params "hdr10=1:repeat-headers=1:master-display=G(13250,34500)B(7500,3000)R(34000,16000)WP(15635,16450)L(10000000,1):max-cll=1000,400" -tag:v hvc1 output.mp4

``master-display`` describes the display used for grading (here a display
with P3 primaries, a D65 white point and a 1000 cd/m² peak), and ``max-cll``
gives the maximum and the maximum frame-average brightness of the content, in
cd/m². Replace them with the values for your content.


Hardware encoders
-----------------

The ``ffmpeg`` command-line tool may also offer hardware HEVC encoders, which
are much faster than x265 but usually make larger files at the same quality:
``hevc_videotoolbox`` on macOS, ``hevc_nvenc`` with NVIDIA GPUs,
``hevc_qsv`` with Intel GPUs, and ``hevc_amf`` with AMD GPUs. Run
``ffmpeg -encoders`` to see which are available. For example, on macOS:

.. code-block:: sh

   ffmpeg -framerate 24 -i render.%04d.tif -c:v hevc_videotoolbox -profile:v main10 -q:v 65 -pix_fmt p010le -vf "setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709" -tag:v hvc1 output.mp4

``-q:v`` sets the quality, from 1 to 100 (higher is better). WriteFFmpeg
does not use hardware encoders.
