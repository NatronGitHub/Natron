.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Noise removal
=============

Most footage contains noise: sensor noise from digital cameras (stronger in dark
shots and at high ISO), film grain from scanned film, and compression artifacts
that spread the noise into small blocks. Noise gets in the way of many compositing
tasks: keyers produce noisy edges, trackers jitter, and noise is amplified by
color corrections and sharpening.

A common workflow is:

1. Remove the noise from the plates, as early as possible in the node graph.
2. Do the compositing work (keying, tracking, paint, color correction) on the
   clean plates.
3. Add the grain back at the end, so that the composited elements and the plates
   look like they were shot with the same camera.

This page describes the nodes available for each of these steps. Nodes are created
from the node toolbar, or by pressing :kbd:`Tab` in the Node Graph and typing their
name.

Before denoising
----------------

- Look at the noise in each channel: noise is often much stronger in the blue
  channel, and in the chroma than in the luminance. The Viewer can display each
  channel separately.
- Find the areas of the image where the noise is visible without image details:
  flat colors, sky, walls, out-of-focus areas. These are used to measure the noise.
- Denoise the images in linear color, as read by the
  :ref:`Read<fr.inria.built-in.Read>` node with its default settings.

DenoiseSharpen
--------------

:ref:`DenoiseSharpen<net.sf.openfx.DenoiseSharpen>` (in the *Filter* group) is
the main denoising node of Natron. It decomposes the image into frequency bands
(using wavelets), measures the noise level in each band and each channel, and
removes what is below that level. It can also sharpen the image details that are
above the noise level.

Basic use:

1. Connect the footage to the *Source* input, and go to a frame that is
   representative of the shot.
2. Set the analysis rectangle (*Bottom Left* and *Size*, also shown in the Viewer)
   on an area without details, or leave it on most of the image if the image has
   few textured areas. The *AnalysisMask* input can be used to restrict the
   analysis to an area of any shape: pixels where the mask is not zero are used.
3. Press *Analyze Noise Levels*. The noise levels are computed for the current
   frame, and *Lock Analysis and Apply* is checked. As long as the analysis is not
   locked, DenoiseSharpen outputs the source image unchanged.
4. Adjust the amount of denoising with *Noise Level Gain*: lower values keep more
   noise and more details, higher values remove more noise and more details.

The noise levels have to be analyzed again when the *Color Model* is changed.
The default color model, Y'CbCr, works well for most footage: it lets the
chroma channels (where the eye sees less detail) be denoised more than the
luminance, with the *Cb Gain* and *Cr Gain* parameters.

Checking the result:

- Set *Output* to *Noise* to see what is removed from the image. Only noise
  should be visible. If edges or textures of the image are visible, lower
  *Noise Level Gain*, or disable the low-frequency bands (see below).
- To see which details are kept, set *Sharpen Amount* to a high value (10, for
  example) temporarily: only image details should be sharpened, not noise. Reset
  it to 0 afterwards, unless the image should really be sharpened.

The *Frequency Tuning* parameters control each frequency band separately:

- Digital sensor noise is mostly in the high frequencies: *Denoise High
  Frequencies* alone may be enough.
- Film grain and noise from compressed footage are coarser: the medium frequencies
  may also have to be denoised.
- The low and very low frequency bands should only be enabled if the analysis
  area is really flat. Otherwise, part of the image is taken for noise, and the
  result loses contrast and looks blurry.

When the shot has no flat area at all, the noise can be analyzed on another clip
with the same kind of noise (for example, another shot from the same camera and
settings) connected to the *AnalysisSource* input.

DenoiseSharpen assumes that the noise is Gaussian and has the same level in dark
and bright areas. Its description explains how to handle other kinds of noise:
for example, with the *RGB* color model, multiplicative noise can be denoised by
converting the image to a log encoding before denoising and back to linear after.
Isolated bright or dark pixels (salt-and-pepper noise, dead pixels) are better
removed with a median filter.

DenoiseSharpen is slow on large images. When the settings are final, render the
denoised plates to disk and read them back in the rest of the project
(see :doc:`compositing-projects-render`).

Other denoising filters
-----------------------

Several other nodes of the *Filter* group smooth the image while keeping edges.
They have fewer automatic settings than DenoiseSharpen, but may work better on
some kinds of noise:

- :ref:`Median<net.sf.cimg.CImgMedian>` replaces each pixel by the median of its
  neighborhood. It removes isolated bright or dark pixels, dust and scratches,
  but also small details. *Size* sets the neighborhood; with a non-zero
  *Threshold*, only the neighbors that are close in value to the pixel are used.
- :ref:`SmoothPatchBased<net.sf.cimg.CImgDenoise>` averages similar patches of the
  image (non-local means). It is good at keeping repeated textures.
- :ref:`SmoothBilateral<net.sf.cimg.CImgBilateral>` averages neighboring pixels
  that have a similar value, which keeps strong edges sharp.
- :ref:`SmoothAnisotropic<net.sf.cimg.CImgSmooth>` smooths along the edges and
  not across them.
- :ref:`ChromaBlur<net.sf.cimg.CImgChromaBlur>` blurs only the colors, not the
  luminance. It cleans up the chroma noise and the color blocks of compressed and
  chroma-subsampled footage, which helps keying in particular
  (see :doc:`compositing-keying`).
- :ref:`Blur<net.sf.cimg.CImgBlur>` removes the noise and the details alike. It is
  mostly useful on mattes, or on images used only to compute something else (for
  example a key or an edge mask), not on the final image.

All these nodes have a *Mask* input and a *Mix* parameter, which can be used to
denoise only part of the image, or to keep some of the original noise.

G'MIC filters
-------------

The G'MIC plug-ins (in the *GMIC* group of the node toolbar) include many
smoothing and denoising filters, among them:

- :ref:`G'MIC Smooth NL-Means<eu.gmic.SmoothNLMeans>`,
  :ref:`G'MIC Smooth Patch-Based<eu.gmic.SmoothPatchBased>` and
  :ref:`G'MIC Smooth Patch-PCA<eu.gmic.SmoothPatchPCA>`, which average similar
  patches (Patch-PCA is by far the slowest);
- :ref:`G'MIC Smooth Wavelets<eu.gmic.SmoothWavelets>`, a wavelet-based denoiser;
- :ref:`G'MIC Smooth Median<eu.gmic.SmoothMedian>`;
- :ref:`G'MIC Smooth Bilateral<eu.gmic.SmoothBilateral>`,
  :ref:`G'MIC Smooth Anisotropic<eu.gmic.SmoothAnisotropic>`,
  :ref:`G'MIC Smooth Total Variation<eu.gmic.SmoothTotalVariation>`,
  :ref:`G'MIC Smooth Guided<eu.gmic.SmoothGuided>` and
  :ref:`G'MIC Smooth Selective Gaussian<eu.gmic.SmoothSelectiveGaussian>`,
  edge-preserving smoothing filters.

These filters process each frame independently, and most of them are slow on
large images. Try them on a single frame, with the Viewer showing a small area
of the image, before rendering a whole sequence.

Static shots: averaging frames
------------------------------

When the camera and the subject do not move, averaging several frames removes
the noise without losing any detail: averaging N frames divides the noise level
by the square root of N. Use :ref:`FrameBlend<net.sf.openfx.FrameBlend>` with the
*Average* operation, and set *Frame Range* to the frames to average. With the
*Absolute* option and the whole input range (the *Input Range* button), it
produces a single clean plate, which is useful for paint work and clean-plate
keying.

Adding the grain back
---------------------

After compositing on denoised plates, the result looks too clean, and the
composited elements look different from the plate. There are two ways to add the
grain back.

**Using the original grain.** Subtract the denoised plate from the original plate
with a :ref:`Merge<net.sf.openfx.MergePlugin>` node set to the *from* operation
(the original in the B input, the denoised plate in the A input: the result is
B-A). This gives an image of the grain alone. Then add it to the final
composite with a Merge node set to *plus*. This gives exactly the grain of the
plate, but only where the composite has not moved the plate: on elements that
were added, moved or painted, the grain comes from another part of the plate and
may show the edges of the original image.

**Using synthetic grain.** Generate new grain on top of the composite, or only on
the added elements, and adjust it to match the plate:

- :ref:`SeGrain<net.sf.openfx.SeGrain>` (in the *Draw* group) adds film grain,
  with presets for several Kodak film stocks (sized for 2K scans) and for digital
  noise, and separate size, irregularity and intensity controls for each channel.
- :ref:`G'MIC Add Grain<eu.gmic.AddGrain>` adds film grain from a set of presets,
  with controls for the scale, opacity and color of the grain.
- :ref:`Noise<net.sf.cimg.CImgNoise>` (in the *Draw* group) adds random noise
  (Gaussian, uniform, salt-and-pepper, Poisson or Rice), which is closer to digital sensor
  noise than to film grain.

To match the grain, compare the composite and the original plate in flat areas,
channel by channel, at 100% zoom, and on several frames while playing the
sequence: grain that looks right on a still frame may look wrong in motion.
