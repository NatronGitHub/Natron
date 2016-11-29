.. _net.sf.openfx.Deinterlace:

DeinterlaceOFX
==============

.. figure:: net.sf.openfx.Deinterlace.png
   :alt: 

*This documentation is for version 1.0 of DeinterlaceOFX.*

Deinterlace input stream.

The following deinterlacing algorithms are supported:

-  Weave: This is what 100fps.com calls "do nothing". Other names: "disabled" or "no deinterlacing". Should be used for PsF content.

-  Blend: Blender (full resolution). Each line of the picture is created as the average of a line from the odd and a line from the even half-pictures. This ignores the fact that they are supposed to be displayed at different times.

-  Bob: Doubler. Display each half-picture like a full picture, by simply displaying each line twice. Preserves temporal resolution of interlaced video.

-  Discard: Only display one of the half-pictures, discard the other. Other name: "single field". Both temporal and vertical spatial resolutions are halved. Can be used for slower computers or to give interlaced video movie-like look with characteristic judder.

-  Linear: Doubler. Bob with linear interpolation: instead of displaying each line twice, line 2 is created as the average of line 1 and 3, etc.

-  Mean: Blender (half resolution). Display a half-picture that is created as the average of the two original half-pictures.

-  Yadif: Interpolator (Yet Another DeInterlacing Filter) from MPlayer by Michael Niedermayer (http://www.mplayerhq.hu). It checks pixels of previous, current and next frames to re-create the missed field by some local adaptive method (edge-directed interpolation) and uses spatial check to prevent most artifacts.

Inputs
------

+----------+---------------+------------+
| Input    | Description   | Optional   |
+==========+===============+============+
| Source   |               | No         |
+----------+---------------+------------+

Controls
--------

+-------------------------+---------------+----------+----------------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------+
| Label (UI Name)         | Script-Name   | Type     | Default-Value        | Function                                                                                                                                                  |
+=========================+===============+==========+======================+===========================================================================================================================================================+
| Field Order             | fieldOrder    | Choice   | HD=upper,SD=lower    | Interlaced field order                                                                                                                                    |
+-------------------------+---------------+----------+----------------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------+
| Parity                  | parity        | Choice   | Lower                | Interpolate which field                                                                                                                                   |
+-------------------------+---------------+----------+----------------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------+
| Yadif Processing Mode   | yadifMode     | Choice   | Temporal & spatial   | Mode of checking fields\ **Temporal & spatial**: temporal and spatial interlacing check (default).\ **Temporal only**: skips spatial interlacing check.   |
+-------------------------+---------------+----------+----------------------+-----------------------------------------------------------------------------------------------------------------------------------------------------------+
