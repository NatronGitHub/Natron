.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
What is compositing?
====================

Compositing is the combining of visual elements from separate sources into single images, often to create the illusion that all those elements are parts of the same scene [Wikipedia]_.

Typical examples of compositing are, for example:

- The superimposition of a character filmed on a green background over a scene shot in another place, at another time, or a computer-generated scene;
- The manual detouring (also called rotoscopy) of an element in a video to embed it in another video, possibly with a different motion;
- Artistic modifications of a video, after shooting a live-action scene or rendering a CGI scene, in order to modify its lighting, colors, depth of field, camera motion, or to remove noise or add film grain.

A video compositing software is not a 3D computer graphics software, like Blender or Maya, but it is perfectly suited for combining computer-generated elements produced by other software with live-action video or 2D animation. Rather than rendering a full 3D scene with the 3D software, which may cost many hours of computation, the video compositing software can assemble the elements produced separately with a much more reactive interface and an almost instantaneous visual feedback.


Theory
******

The math behind compositing was formalized by Porter & Duff [PorterDuff1984]_ after the preliminary work by Wallace [Wallace1981]_. More informating about the theory behind compositing can be found in the works of Jim Blinn [Blinn1994a]_ [Blinn1994b]_ and Alvy Ray Smith [Smith1995]_.

The compositing theory also introduces the notion of "premultipled" RGB values, or "associated alpha", and there is still a lot of `debate <https://groups.google.com/forum/#!topic/ocio-dev/ZehKhUFqhjc>`_ about `premultiplying or not <http://lists.openimageio.org/pipermail/oiio-dev-openimageio.org/2011-December/004709.html>`_.

Natron made the choice of using premultiplied alpha by default in the compositing workflow, like all modern compositing software, because images are stored internally with floating-point values.

Practice
********

There are excellent books that introduce how to do compositing in practice, and using compositing software: [Wright2010]_, [Brinkmann2008]_, [Lanier2009]_, [VES2014]_.

Most of what is described in these books also apply to Natron. It is thus strongly recommended to become familiar with the techniques and workflows described in these books before starting to use Natron.

There are also video tutorials available on video streaming platforms (youtube, vimeo) for Natron or other reference compositing software, such as Nuke of Fusion. These tutorials can be used to get acquainted with compositing.


.. [Wikipedia] `Compositing <https://en.wikipedia.org/wiki/Compositing>`_, in Wikipedia, retrieved Sep. 14, 2016 from https://en.wikipedia.org/wiki/Compositing

.. [PorterDuff1984] Porter, Thomas; Tom Duff (1984). `"Compositing Digital Images" <https://keithp.com/~keithp/porterduff/p253-porter.pdf>`_. Computer Graphics. 18 (3): 253–259. `doi:10.1145/800031.808606 <https://dx.doi.org/10.1145%2F800031.808606>`_

.. [Wallace1981] Wallace,  Bruce  A., `Merging  and  Transformation  of  Raster  Images  for Cartoon  Animation <https://graphics.stanford.edu/papers/merging-sig81/>`_, Computer  Graphics,  Vol  15,  No  3,  Aug  1981, 253-262. SIGGRAPH’81 Conference Proceedings, `doi:10.1145/800224.806813 <http://dx.doi.org/10.1145/800224.806813>`_.

.. [Blinn1994a] Blinn, James F., Jim Blinn's Corner: Compositing Part 1: Theory, IEEE Computer Graphics & Applications, Sep 1994, 83-87, `doi:10.1109/38.310740 <http://dx.doi.org/10.1109/38.310740>`_.

.. [Blinn1994b] Blinn,  James  F., Jim Blinn's Corner: Compositing Part 2: Practice, IEEE Computer Graphics & Applications, Nov 1994, 78-82, `doi:10.1109/38.329100 <http://dx.doi.org/10.1109/38.329100>`_.

.. [Smith1995]  Alvy Ray Smith, `Image Compositing Fundamentals <http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.27.5956>`_, 1995.

.. [Brinkmann2008] Ron Brinkmann, The Art and Science of Digital Compositing, 2nd Edition, 2008 (ISBN  0123706386)

.. [Lanier2009] Lee Lanier, Professional Digital Compositing: Essential Tools and Techniques, 2009 (ISBN 0470452617)

.. [Wright2010] Steve Wright, Digital Compositing for Film and Video, Third Edition, 2010 (ISBN 78-0-240-81309-7)

.. [VES2014] The VES Handbook of Visual Effects: Industry Standard VFX Practices and Procedures, 2nd Edition (ISBM 0240825187)

