.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

.. _tutorials-ffmpeg-di:

Producing digital intermediates
===============================

A *digital intermediate* is a high-quality file made to be processed again:
edited, graded, composited or archived. It is not meant for viewers: it keeps
as much image quality as possible, every frame can be decoded on its own (so
editing and grading applications can seek quickly), and the files are large.
To make a small file for distribution, encode the final version from the
intermediate (see :doc:`tutorials-ffmpeg-hevc`).

There are two families of digital intermediates:

- **Image sequences** (one file per frame), such as OpenEXR, DPX or TIFF. This
  is the usual choice in visual effects and for grading hand-offs. If a render
  stops, only the missing frames have to be rendered again.
- **Intra-frame video codecs**, such as Apple ProRes and Avid DNxHR/DNxHD.
  Editing applications handle them well, and a shot is a single file.

The usual workflow is to render an image sequence from Natron, then make the
video files from it with the ``ffmpeg`` command-line tool. You can also write
ProRes and DNxHR directly from Natron with the WriteFFmpeg node. See also
:doc:`getstarted-troubleshooting` for why image sequences are preferred as
inputs and outputs.


Choosing a format
-----------------

.. list-table::
   :header-rows: 1
   :widths: 22 24 12 42

   * - Format
     - Bit depth and sampling
     - Alpha
     - Typical use
   * - OpenEXR (``.exr``)
     - 16-bit (half) or 32-bit float, RGB
     - Yes
     - VFX hand-offs, scene-linear images, multiple layers and views.
   * - DPX (``.dpx``)
     - 10-bit (also 8, 12, 16) integer, RGB
     - Yes
     - Grading and film pipelines, usually log or Rec. 709 encoded.
   * - TIFF (``.tif``)
     - 8- or 16-bit integer, or float, RGB
     - Yes
     - Interchange with photo and paint applications.
   * - ProRes 422 Proxy, LT, 422, HQ
     - 10-bit, Y'CbCr 4:2:2
     - No
     - Editing. HQ is a good default intermediate; Proxy and LT are for
       offline editing.
   * - ProRes 4444, 4444 XQ
     - 10- or 12-bit, Y'CbCr 4:4:4
     - Yes
     - Graphics with alpha, grading, high-quality masters.
   * - DNxHR LB, SQ, HQ
     - 8-bit, Y'CbCr 4:2:2
     - No
     - Editing in Avid Media Composer and other applications.
   * - DNxHR HQX, 444
     - 10-bit, Y'CbCr 4:2:2 (HQX), Y'CbCr or RGB 4:4:4 (444)
     - No
     - Grading and finishing.
   * - DNxHD
     - 8- or 10-bit, Y'CbCr 4:2:2
     - No
     - Older Avid workflows. Mainly 1920x1080 and 1280x720, at a few
       frame rates.
   * - FFV1 (in ``.mkv``)
     - 8 to 16-bit, Y'CbCr or RGB
     - Yes
     - Lossless archiving. Few editing applications read it.

The ProRes, DNxHR/DNxHD and FFV1 encoders are part of FFmpeg itself, so they
are available in every Natron build.


Color space and bit depth
-------------------------

Natron processes images in floating point, in the scene-linear working space
of the project. The Write node converts the images to the color space chosen
in its **File Colorspace** parameter (an OpenColorIO color space) before
writing them. A default is chosen from the file type, but check it and choose
what the next application expects:

- **OpenEXR**: keep the images scene-linear (``Linear`` in the default
  OpenColorIO config). 16-bit half float, the default for EXR, is enough for
  almost all images and is half the size of 32-bit float.
- **DPX**: use a log encoding (for example ``Cineon``) when the grading
  application expects log images, or a display encoding (for example
  ``nuke_rec709``) when it expects video images. Write 10 bits or more.
- **ProRes, DNxHR and DNxHD**: these are video formats. The images are
  converted to a display encoding (usually Rec. 709, ``nuke_rec709`` in the
  default config), then from RGB to Y'CbCr with limited ("video") range
  (except DNxHR 444, which stays RGB).
  Use at least 10 bits per component: 8-bit intermediates may show banding
  after grading.

Y'CbCr 4:2:2 codecs keep the color information at half the horizontal
resolution. Prefer a 4:4:4 codec (ProRes 4444, DNxHR 444) for green-screen
keys, graphics and text with sharp colored edges.

Among these video codecs, only ProRes 4444/4444 XQ and FFV1 store an alpha
channel. DNxHR and the other ProRes profiles drop it. Image sequences (EXR,
DPX, TIFF) can store alpha.

Video files also carry color *metadata*: the color primaries, the transfer
function and the Y'CbCr matrix. These values do not change the pixels, but
they tell the next application how to interpret them. Wrong or missing values
are a common cause of shifted colors or contrast after an export.


Writing image sequences from Natron
-----------------------------------

Use a Write node with a file name that contains a frame number, such as
``shot_010.####.exr``. The node that does the writing is
:ref:`WriteOIIO <fr.inria.openfx.WriteOIIO>`. Its main parameters for
intermediates are:

- **File Colorspace**: see above.
- **Bit Depth**: ``auto`` writes 16-bit half float for EXR and 16-bit integer
  for DPX and TIFF. Choose ``10i`` for 10-bit DPX, or ``32f`` for full-float
  EXR.
- **Compression**: for EXR, ``zip`` and ``piz`` are lossless. ``dwaa`` and
  ``dwab`` are lossy, but give much smaller files.
- **Output Components**: choose ``RGBA`` to write the alpha channel, ``RGB``
  otherwise.


Writing ProRes or DNxHR from Natron
-----------------------------------

Use a Write node with a ``.mov`` file name, and select the
:ref:`WriteFFmpeg <fr.inria.openfx.WriteFFmpeg>` writer. In its **Codec**
menu, the ProRes profiles are listed by their four-letter codes:

- ``ap4h`` Apple ProRes 4444
- ``apch`` Apple ProRes 422 HQ
- ``apcn`` Apple ProRes 422
- ``apcs`` Apple ProRes 422 LT
- ``apco`` Apple ProRes 422 Proxy
- ``ap4x`` Apple ProRes 4444 XQ

For Avid codecs, choose ``AVdn Avid DNxHD / DNxHR / SMPTE VC-3``, then
select the profile in **DNxHD Codec Profile**. The DNxHR profiles work at
any resolution. In WriteFFmpeg, the DNxHD profiles only exist for 1920x1080
and 1280x720 at some frame rates: the profile descriptions in the menu list
the bit rate used for each frame rate. **DNxHD Output Range** selects between
limited ("video") range, the default, and full range: the pixel values are
written in that range, and the file is tagged with it. Avid tools may expect
limited range. DNxHR 444 is written as RGB, which is always full range.

WriteFFmpeg picks the pixel format from the profile: 10-bit 4:2:2 for
ProRes 422 and DNxHR HQX, 10-bit 4:4:4 for ProRes 4444 and 4444 XQ, 10-bit
RGB for DNxHR 444, and 8-bit 4:2:2 for DNxHR LB, SQ and HQ. It shows the
result in **Selected Pixel Coding** and **Bit Depth**. To keep the alpha
channel with ProRes 4444 or 4444 XQ, check **Alpha** (the input must be
RGBA).

For all the ProRes profiles and for DNxHR/DNxHD (except DNxHR 444),
WriteFFmpeg writes Rec. 709 color metadata when the image height is 720 or
more, and Rec. 601 metadata (with PAL or NTSC primaries) for smaller images,
with the Rec. 709 transfer function in both cases. **Write NCLC**, checked by
default, also writes them in the QuickTime ``colr`` atom. For ProRes, they
are also written in the header of each frame. For DNxHR 444, it only writes
the primaries and the transfer function that match the **File Colorspace**.

WriteFFmpeg cannot write MXF files, and it does not write audio. Use
``ffmpeg`` for those, as shown below.


Encoding with ffmpeg
--------------------

The commands below read a 16-bit TIFF sequence named ``render.0001.tif``,
``render.0002.tif``, … rendered by Natron with a Rec. 709 **File
Colorspace**, and write a 24 fps movie. Replace ``-framerate 24`` with the
frame rate of your project (for example ``-framerate 24000/1001`` for 23.976
fps), and add ``-start_number 1001`` if the first frame is 1001.

All the commands use the same video filter to write the color metadata:

.. code-block:: sh

   -vf "setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709"

This sets Rec. 709 primaries, transfer function and Y'CbCr matrix. ffmpeg
uses the matrix to convert RGB to Y'CbCr, then writes all three values in the
file. Without it, ffmpeg converts with the Rec. 601 matrix and leaves the
metadata empty. Use ``setparams`` rather than the ``-color_primaries`` and
``-color_trc`` output options: with FFmpeg 9, these two options are ignored
when the input is an image sequence, and the file gets no primaries or
transfer function.

Apple ProRes 422 HQ:

.. code-block:: sh

   ffmpeg -framerate 24 -i render.%04d.tif -c:v prores_ks -profile:v hq -vendor apl0 -pix_fmt yuv422p10le -vf "setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709" output.mov

The ``-profile:v`` value can be ``proxy``, ``lt``, ``standard``, ``hq``,
``4444`` or ``4444xq``. ``-vendor apl0`` marks the frames as made by Apple's
encoder, which some applications check for. WriteFFmpeg does the same.

Apple ProRes 4444 with alpha, from a 16-bit RGBA PNG sequence:

.. code-block:: sh

   ffmpeg -framerate 24 -i render.%04d.png -c:v prores_ks -profile:v 4444 -vendor apl0 -pix_fmt yuva444p10le -alpha_bits 16 -vf "setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709" output.mov

Apple ProRes 4444 XQ, without alpha:

.. code-block:: sh

   ffmpeg -framerate 24 -i render.%04d.tif -c:v prores_ks -profile:v 4444xq -vendor apl0 -pix_fmt yuv444p10le -vf "setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709" output.mov

Avid DNxHR HQX (10-bit, 4:2:2):

.. code-block:: sh

   ffmpeg -framerate 24 -i render.%04d.tif -c:v dnxhd -profile:v dnxhr_hqx -pix_fmt yuv422p10le -vf "setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709" output.mov

The ``-profile:v`` value can be ``dnxhr_lb``, ``dnxhr_sq`` or ``dnxhr_hq``
with ``-pix_fmt yuv422p`` (8-bit), ``dnxhr_hqx`` with ``-pix_fmt
yuv422p10le``, or ``dnxhr_444`` with ``-pix_fmt yuv444p10le``.

To write an MXF file instead, as used by Avid Media Composer, change the
output file name to ``output.mxf``. An existing QuickTime file can be
rewrapped as MXF without re-encoding:

.. code-block:: sh

   ffmpeg -i output.mov -c copy output.mxf

Avid DNxHD 175x, at 1920x1080, 23.976 fps and 10 bits. DNxHD has no
profile names: the encoder picks the variant from the frame size, frame rate, pixel format
and bit rate, and fails if the combination does not exist. The ``scale``
filter resizes the images to 1920x1080 if needed:

.. code-block:: sh

   ffmpeg -framerate 24000/1001 -i render.%04d.tif -c:v dnxhd -b:v 175M -pix_fmt yuv422p10le -vf "scale=1920:1080,setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709" output.mov

Lossless FFV1 in Matroska, for archiving:

.. code-block:: sh

   ffmpeg -framerate 24 -i render.%04d.tif -c:v ffv1 -level 3 -g 1 -slicecrc 1 -pix_fmt yuv422p10le -vf "setparams=color_primaries=bt709:color_trc=bt709:colorspace=bt709" output.mkv

``-level 3`` selects FFV1 version 3, ``-g 1`` makes every frame a keyframe
(so that any frame can be decoded on its own), and ``-slicecrc 1`` adds a
checksum to each slice, so that a damaged file can be detected.

FFV1 is lossless, but the conversion to 10-bit Y'CbCr 4:2:2 is not. To keep
the RGB values of the 16-bit images exactly, use ``-pix_fmt gbrp16le`` and
remove ``:colorspace=bt709`` from the filter.

To add a sound track to any of these files, see
:doc:`tutorials-ffmpeg-audio`.


Checking the result
-------------------

``ffprobe`` shows the codec, profile, pixel format and color metadata of the
video stream:

.. code-block:: sh

   ffprobe -v error -select_streams v:0 -show_entries stream=codec_name,profile,pix_fmt,color_range,color_space,color_primaries,color_transfer -of default=noprint_wrappers=1 output.mov

For the ProRes 422 HQ command above, it prints:

.. code-block:: none

   codec_name=prores
   profile=HQ
   pix_fmt=yuv422p10le
   color_range=tv
   color_space=bt709
   color_transfer=bt709
   color_primaries=bt709

``color_range=tv`` means limited ("video") range. ``unknown`` means that the
value is missing, and the application that reads the file will guess it.


Converting intermediates back to images
---------------------------------------

To bring a ProRes or DNxHR file back into Natron as an image sequence, see
:doc:`tutorials-imagesequence`. For example, to make a 10-bit DPX sequence:

.. code-block:: sh

   ffmpeg -i input.mov -c:v dpx -pix_fmt gbrp10le output.%04d.dpx

ffmpeg uses the color metadata of the input file to convert from Y'CbCr to
RGB, so check it with ``ffprobe`` first. Read the DPX files in Natron with
the matching **File Colorspace** (for example ``nuke_rec709``).
