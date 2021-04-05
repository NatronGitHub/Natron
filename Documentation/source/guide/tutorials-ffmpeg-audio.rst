.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
Muxing Audio
============

Muxing audio is a process to add audio to a video without re-rendering the whole video again.

Muxing is less time consuming and keeps the video/audio quality of the original files.



Merging video and audio, with audio re-encoding
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``ffmpeg -i video.mp4 -i audio.wav -c:v copy -c:a aac output.mp4``

We assume that the video file does not contain any audio stream yet and that the output format stays the same as the input format.

The above command transcodes the audio, since MP4s cannot carry PCM audio streams. You can use any other desired audio codec if you want.
See the FFmpeg Wiki: AAC Encoding Guide for more info.

If your audio or video stream is longer, you can add the ``-shortest`` option so that ffmpeg will stop encoding once one file ends.



Copying the audio without re-encoding
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If your output container can handle any codec (e.g. .mkv) then you can simply copy both audio and video streams:

``ffmpeg -i video.mp4 -i audio.wav -c copy output.mkv``


Replacing audio stream
~~~~~~~~~~~~~~~~~~~~~~

If your input video already contains an audio stream and you want to replace it, you need to tell ffmpeg which audio stream to take:

``ffmpeg -i video.mp4 -i audio.wav -c:v copy -c:a aac -map 0:v:0 -map 1:a:0 output.mp4``


The -map option makes ffmpeg only use the first video stream from the first input and the first audio stream from the second input for the output file.


Combine 6 mono inputs into one 5.1 (6 channel) audio output
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``ffmpeg -i front_left.wav -i front_right.wav -i front_center.wav -i lfe.wav -i back_left.wav -i back_right.wav \
-filter_complex "[0:a][1:a][2:a][3:a][4:a][5:a]join=inputs=6:channel_layout=5.1[a]" -map "[a]" output.wav``

The join audio filter also allows you to manually choose the layout:

``ffmpeg -i front_left.wav -i front_right.wav -i front_center.wav -i lfe.wav -i back_left.wav -i back_right.wav \
-filter_complex "[0:a][1:a][2:a][3:a][4:a][5:a]join=inputs=6:channel_layout=5.1:map=0.0-FL|1.0-FR|2.0-FC|3.0-LFE|4.0-BL|5.0-BR[a]" -map "[a]" output.wav``





.. toctree::
   :maxdepth: 2

