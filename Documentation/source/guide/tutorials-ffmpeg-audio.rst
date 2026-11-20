.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

.. _tutorials-ffmpeg-audio:

Muxing audio
============

Natron does not handle audio: :ref:`WriteFFmpeg <fr.inria.openfx.WriteFFmpeg>`
writes video files without sound. To add sound to a video rendered by Natron,
*mux* it with ``ffmpeg``: muxing puts the audio and video streams into one
file without re-encoding the video, so it is fast and keeps the video quality.

In the commands below, ``video.mp4`` is the video rendered by Natron and
``audio.wav`` is the sound track.

Adding audio to a video
-----------------------

.. code-block:: bash

   ffmpeg -i video.mp4 -i audio.wav -c:v copy -c:a aac output.mp4

``-c:v copy`` copies the video stream as it is, and ``-c:a aac`` encodes the
audio to AAC. Recent FFmpeg versions can store uncompressed (PCM) audio in
MP4, but many players can't play it, so AAC is the safe choice for MP4. See the
`FFmpeg AAC encoding guide <https://trac.ffmpeg.org/wiki/Encode/AAC>`__ for
the quality options.

If the audio and the video have different durations, the output is as long as
the longest one. Add ``-shortest`` to stop when the shortest stream ends:

.. code-block:: bash

   ffmpeg -i video.mp4 -i audio.wav -c:v copy -c:a aac -shortest output.mp4

Copying the audio without re-encoding
-------------------------------------

Containers such as Matroska (``.mkv``) or QuickTime (``.mov``) can store PCM
audio, so both streams can be copied:

.. code-block:: bash

   ffmpeg -i video.mp4 -i audio.wav -c copy output.mkv

Replacing the audio of a video
------------------------------

If the video already has an audio stream, use ``-map`` to choose the streams
of the output: here, the first video stream of the first input (``0:v:0``) and
the first audio stream of the second input (``1:a:0``). Without ``-map``,
FFmpeg picks one video and one audio stream by itself, which may be the old
audio.

.. code-block:: bash

   ffmpeg -i video.mp4 -i audio.wav -c:v copy -c:a aac -map 0:v:0 -map 1:a:0 output.mp4

Combining six mono files into 5.1 audio
---------------------------------------

The ``join`` audio filter merges mono inputs into one multichannel stream.
With ``channel_layout=5.1``, the inputs are used in the order of that layout:
front left, front right, front center, LFE, back left, back right.

.. code-block:: bash

   ffmpeg -i front_left.wav -i front_right.wav -i front_center.wav \
     -i lfe.wav -i back_left.wav -i back_right.wav \
     -filter_complex "[0:a][1:a][2:a][3:a][4:a][5:a]join=inputs=6:channel_layout=5.1[a]" \
     -map "[a]" output.wav

The ``map`` option of ``join`` sets which input goes to which output channel
(``input.channel-OUTPUT``). This example uses the ``5.1(side)`` layout, with
side left and side right surround channels instead of back left and back
right:

.. code-block:: bash

   ffmpeg -i front_left.wav -i front_right.wav -i front_center.wav \
     -i lfe.wav -i side_left.wav -i side_right.wav \
     -filter_complex "[0:a][1:a][2:a][3:a][4:a][5:a]join=inputs=6:channel_layout=5.1(side):map=0.0-FL|1.0-FR|2.0-FC|3.0-LFE|4.0-SL|5.0-SR[a]" \
     -map "[a]" output.wav

``ffmpeg -layouts`` lists the channel names and layouts. More examples are in
the FFmpeg wiki page
`Manipulating audio channels <https://trac.ffmpeg.org/wiki/AudioChannelManipulation>`__.

See also
--------

- :doc:`thenodes-image-write`: writing the video with the Write node.
- :doc:`compositing-time`: editing time in Natron, which the sound track must
  then match.
