.. _tutorials-hugin:

Using Hugin (PanoTools) projects in Natron
==========================================

`Hugin <https://hugin.sourceforge.io/>`__ aligns overlapping photos and
saves the result as a PanoTools project (a ``.pto`` file). This page shows
how to rebuild a Hugin panorama in Natron from the values in that file,
using a :ref:`LensDistortion <net.sf.openfx.LensDistortion>` node and a
:ref:`Card3D <net.sf.openfx.Card3D>` node per image. You can then animate
the camera, use videos instead of still images, and paint the seams with
Roto masks.

This method only handles rectilinear (normal, non-fisheye) source images
and a rectilinear output projection. See :ref:`tutorials-hugin-limits`.

Creating a PanoTools project
----------------------------

Installing Hugin
~~~~~~~~~~~~~~~~

- **macOS:** install it with `MacPorts <https://www.macports.org/>`__:
  ``sudo port install hugin-app``. The applications (Hugin, PTBatcherGUI,
  HuginStitchProject, calibrate_lens_gui) go to
  ``/Applications/MacPorts/Hugin``, and the command-line tools, including
  ``enblend``, go to ``/opt/local/bin``. The official Hugin download for
  macOS is an old version (2019.0) that does not run on Apple Silicon Macs.
- **Linux:** install the ``hugin`` package of your distribution (on Debian
  and Ubuntu, the command-line tools are in ``hugin-tools``, and
  ``enblend`` is a separate package), or the Flathub Flatpak.
- **Windows:** use the 64-bit installer from the
  `Hugin download page <https://hugin.sourceforge.io/download/>`__.

Using the Hugin GUI
~~~~~~~~~~~~~~~~~~~

Read the `Hugin documentation <https://wiki.panotools.org/Hugin>`__ or
follow a `Hugin tutorial <https://hugin.sourceforge.io/tutorials/overview/en.shtml>`__,
make your panorama, and save the project. Choose the **Rectilinear**
projection before saving (see :ref:`tutorials-hugin-limits`).

Using the command-line tools
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The `Panorama scripting in a nutshell
<https://wiki.panotools.org/Panorama_scripting_in_a_nutshell>`__ page
describes the command-line tools in detail. The following commands make a
project from the JPEG images in the current directory (file names are case
sensitive: use ``*.JPG`` if your files end with ``.JPG``):

.. code-block:: bash

    pto_gen -o project.pto *.jpg
    cpfind --multirow -o project.pto project.pto
    celeste_standalone -i project.pto -o project.pto
    cpclean -v --output project.pto project.pto
    autooptimiser -a -l -s -m -o project.pto project.pto
    pano_modify -p 0 --fov=AUTO --canvas=AUTO -o project.pto project.pto

- ``pto_gen`` creates the project, ``cpfind`` finds control points
  between the images, ``celeste_standalone`` removes control points on
  clouds, and ``cpclean`` removes outliers.
- ``autooptimiser -a`` aligns the images, ``-l`` levels the horizon,
  ``-s`` picks an output projection and size, and ``-m`` optimizes the
  photometric parameters (exposure, white balance, vignetting).
- ``pano_modify -p 0`` sets the output projection to rectilinear, and
  ``--fov=AUTO --canvas=AUTO`` recompute the field of view and the size of
  the panorama.

To get Hugin's own stitched result, for comparison:

.. code-block:: bash

    nona -m TIFF_m -o project project.pto
    enblend --save-masks -o panorama.tif project*.tif

``nona`` writes one remapped image per input (``project0000.tif``,
``project0001.tif``, …), and ``enblend`` blends them into ``panorama.tif``
and saves its seam masks as ``mask-1.tif``, ``mask-2.tif``, …

Reading the project file
------------------------

A ``.pto`` file is a text file. Its syntax is described in the
`PTOptimizer <https://wiki.panotools.org/PTOptimizer>`__ and
`PTStitcher <https://wiki.panotools.org/PTStitcher>`__ documentation.
The lines used here are:

- the ``p`` line (near the top of the file), which describes the output
  panorama: ``w`` and ``h`` are its size in pixels, ``v`` is its
  horizontal field of view in degrees, and ``f`` is its projection
  (``f0`` is rectilinear);
- one ``i`` line per input image, in the same order as the images: ``v``
  is the horizontal field of view of the image, ``y``, ``p`` and ``r`` are
  its yaw, pitch and roll in degrees, and ``a``, ``b``, ``c``, ``d``,
  ``e``, ``g``, ``t`` are its lens distortion parameters.

A value written as ``name=N``, such as ``v=0`` or ``a=0``, is linked to
image ``N``: use the value from the ``i`` line of image ``N`` (images are
numbered from 0).

Building the panorama in Natron
-------------------------------

Project format
~~~~~~~~~~~~~~

In the project settings (see :ref:`compositing-projects-setup`), set the
output format to the size of the panorama, given by ``w`` and ``h`` on the
``p`` line.

Reading the images
~~~~~~~~~~~~~~~~~~

Add a :ref:`Read <fr.inria.built-in.Read>` node for each image (or video),
and set its **Output Components** to **RGBA**, so that the area outside each
image is transparent after the projection.

Distortion correction
~~~~~~~~~~~~~~~~~~~~~

After each Read node, add a :ref:`LensDistortion <net.sf.openfx.LensDistortion>`
node. Set **Model** to **PanoTools** and **Direction** to **Undistort**, then
copy the ``a``, ``b``, ``c``, ``d``, ``e``, ``g`` and ``t`` values from the
image's ``i`` line into the **a**, **b**, **c**, **d**, **e**, **g** and
**t** parameters (script names ``pt_a`` … ``pt_t``).

If all images share the same lens (the ``i`` lines of the other images
contain ``a=0 b=0 c=0 d=0 e=0 g=0 t=0``), set up the first LensDistortion
node, then clone it: select it, right-click in the Node Graph and choose
**Edit > Clone Nodes** (:kbd:`Alt+K`). Clones share their parameter values.

Projection
~~~~~~~~~~

After each LensDistortion node, add a :ref:`Card3D <net.sf.openfx.Card3D>`
node, which places the image on a card in front of a camera, as the
``nona`` remapping tool does. In each Card3D node:

- Set the card's **Transform Order** (``cardXformOrder``) to **STR**, so
  that the card rotates around the camera. Leave the card's **Rotation
  Order** (``cardRotOrder``) at its default, **ZXY**, which matches the
  PanoTools order (roll, then pitch, then yaw). These are the card
  parameters, not those of the **Axis** and **Cam** groups at the top of
  the parameters.
- Leave **Output Format** at its default, **Project**.
- Set the card's **Rotate** (``cardRotate``) from the ``i`` line:
  ``cardRotate.x = p``, ``cardRotate.y = -y`` and ``cardRotate.z = -r``.
  In PanoTools, a positive yaw moves the image to the right and a positive
  roll turns it clockwise, the opposite of the Card3D rotations.
- Set **Lens-In H.Aperture** (``lensInHAperture``) to
  ``2*tan(v*pi/360)``, where ``v`` is the field of view from the ``i``
  line, and leave **Lens-In Focal** at 1. You can type the expression
  directly in the value field, with ``v`` replaced by its value.

Camera
~~~~~~

The camera defines the view of the panorama. In the first Card3D node, open
the **Cam** group:

- Check **Enable Camera**. When it is unchecked, Card3D uses a default
  camera with a horizontal field of view of about 53°.
- In the **Cam Projection** group, set **Focal Length** (``camfocal``) to 1
  and **Horiz. Aperture** (``camhaperture``) to ``2*tan(v*pi/360)``, where
  ``v`` is the field of view from the ``p`` line.

All Card3D nodes must use the same camera. Natron's Card3D node has no
camera input, so link the camera parameters instead: right-click
**Enable Camera** in the first Card3D node, choose **Copy Link**, then
right-click the same parameter in each other Card3D node and choose
**Paste Link**. Do the same for **Focal Length** and **Horiz. Aperture**,
and for **Cam Rotate** (``camRotate``) if you want to change the view
direction later.

Overlapping the images
~~~~~~~~~~~~~~~~~~~~~~

Connect the first Card3D node to the **B** input of a
:ref:`Merge <net.sf.openfx.MergePlugin>` node, the second one to the
**A** input, the third one to **A2**, and so on. The Merge node now shows a
first version of the panorama, with the images stacked on top of each other.

Drawing the masks
~~~~~~~~~~~~~~~~~

To hide the seams, add a :ref:`Roto <fr.inria.built-in.Roto>` node between
each LensDistortion node and its Card3D node, and draw a mask for each
image:

1. Connect only the first two Card3D nodes to the Merge node (inputs B and
   A), and view the Merge output.
2. In the Roto node of the second image, check **Premultiply**, and uncheck
   **R**, **G** and **B**, so that only the alpha channel is affected.
3. Draw a shape on the second image, and set its **Operator** to **min**
   (the default is **over**), so that the shape masks the image's own alpha.
   Feather the shape, especially in the overlap area.
4. Connect the third Card3D node to the **A2** input, and draw its mask the
   same way. Repeat for each image.

.. _tutorials-hugin-limits:

Limitations
-----------

- **Projections:** Card3D is a pinhole camera, so the source images must be
  rectilinear (``f0`` on the ``i`` lines) and the output must be rectilinear
  (``f0`` on the ``p`` line), with a field of view well under 180°.
  Fisheye images and cylindrical or equirectangular panoramas are not
  supported.
- **Camera translation:** the ``TrX``, ``TrY`` and ``TrZ`` parameters
  (mosaic mode) are not used by this method.
- **Photometric parameters:** this method ignores the camera response
  (``Ra`` … ``Re``, see
  `Camera response curve <https://wiki.panotools.org/Camera_response_curve>`__),
  the exposure (``Eev``), the white balance multipliers (``Er`` and ``Eb``),
  and the vignetting correction (``Va`` … ``Vd``, center shift ``Vx`` and
  ``Vy``, see `Vignetting <https://wiki.panotools.org/Vignetting>`__). If
  the images differ in brightness or color, correct them with a Grade or
  ColorCorrect node before the Card3D node.
- **Blending:** the ``enblend`` seams are not imported. You can read the
  ``mask-*.tif`` files that ``enblend --save-masks`` writes and use them as
  masks, but they are in the panorama space, after the Card3D projection.
