.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Merge Node
==========

The Merge node combines two or more images. By default, it puts input **A**
over input **B** with the **over** operation. See :doc:`compositing-merge`
for a step-by-step guide, and the :ref:`Merge node reference
<net.sf.openfx.MergePlugin>` for all the parameters.

Usage
-----

Natron, like Nuke, works with premultiplied images: a pixel with a zero alpha
must also have zero red, green and blue values. Do not expect non-black RGB
values to be transparent just because their alpha is zero. That works in a
compositor that uses unpremultiplied images, such as After Effects, but not in
Natron.

To ignore the alpha channel of an input, uncheck the **A** checkbox on that
input's row (**A Channels** or **B Channels**) in the Merge node's properties.
