.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

.. _tutorials-ffmpeg:

FFmpeg
======

`FFmpeg <https://ffmpeg.org/>`__ is a free, cross-platform command-line tool
to convert, record and stream audio and video. Its full documentation is at
https://ffmpeg.org/documentation.html.

Natron reads and writes video files with FFmpeg, through the
:ref:`ReadFFmpeg <fr.inria.openfx.ReadFFmpeg>` and
:ref:`WriteFFmpeg <fr.inria.openfx.WriteFFmpeg>` nodes, but some tasks are
easier with the ``ffmpeg`` command-line tool, before or after compositing in
Natron. These tutorials show a few of them. See also
:ref:`tutorials-imagesequence`.

.. toctree::
   :maxdepth: 2

   tutorials-ffmpeg-di
   tutorials-ffmpeg-audio
   tutorials-ffmpeg-hevc
