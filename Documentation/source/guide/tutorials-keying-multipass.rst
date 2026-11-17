.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Multipass Keying
================

Most problems when pulling a key come from the lighting and from noise in the
image. Noise is usually strongest in the red and blue channels. Before keying,
clean up the plate:

- A screen-cleaning node such as **lp_CleanScreen** or **PxF_ScreenClean**
  (community PyPlugs from the `natron-plugins
  <https://github.com/NatronGitHub/natron-plugins>`_ collection) evens out
  the colour of the green screen.
- If the screen is noisy because of poor lighting,
  :ref:`DenoiseSharpen <net.sf.openfx.DenoiseSharpen>` smooths the channels
  enough to get a decent key.

Then key with several :ref:`ChromaKeyer <net.sf.openfx.ChromaKeyerPlugin>`
nodes in a chain, each one refining the matte of the previous one. The example
below uses a screen-cleaning node, DenoiseSharpen and three ChromaKeyer nodes.

.. figure:: _images/tutorials-keying/multipass-keying.jpg
   :width: 650px
   :align: center
   :alt: Natron window with three keyed mattes in the Viewers and a node graph with three chained ChromaKeyer nodes

   Multipass keying with three ChromaKeyer nodes. The properties of the last
   one are shown on the right.

Setting up the chain
--------------------

The first keyer makes the core matte (the hard, solid inside of the
foreground). Each following keyer adds to it, and the last one makes the final
image. Set the **Output Mode** and **Source Alpha** parameters of each
ChromaKeyer as follows:

+-----------------------+---------------------+-------------------------+
| ChromaKeyer           | Output Mode         | Source Alpha            |
+=======================+=====================+=========================+
| First (core matte)    | Intermediate        | Ignore                  |
+-----------------------+---------------------+-------------------------+
| Each following one    | Intermediate        | Add to Inside Mask      |
+-----------------------+---------------------+-------------------------+
| Last                  | Premultiplied       | Add to Inside Mask      |
+-----------------------+---------------------+-------------------------+

With **Intermediate**, the colour is passed through unchanged and the alpha
holds the key, which is what the next keyer needs. **Add to Inside Mask** adds
the alpha of the previous keyer to the inside mask (holdout matte) of this
one. Only the last keyer of the chain should output a **Premultiplied** image.
With more than three keyers the chain gets harder to follow, so keep it short.
