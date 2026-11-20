.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
What is compositing?
====================

Compositing is the combining of visual elements from separate sources into single images, often to create the illusion that all those elements are parts of the same scene [Wikipedia]_.

Typical examples of compositing are:

- The superimposition of a character filmed on a green background over a scene shot in another place, at another time, or a computer-generated scene;
- Rotoscoping: manually cutting out an element of a video to put it in another video, possibly with a different motion;
- Artistic modifications of a video, after shooting a live-action scene or rendering a CGI scene, in order to modify its lighting, colors, depth of field, camera motion, or to remove noise or add film grain.

A video compositor is not a 3D application like Blender or Maya, but it is well suited to combining computer-generated elements made with other software with live-action video or 2D animation. Rather than rendering a full 3D scene again, which may take hours, the compositor assembles separately rendered elements, with a responsive interface and almost instant visual feedback.


Theory
******

The math behind compositing was formalized by Porter & Duff [PorterDuff1984]_ after the preliminary work by Wallace [Wallace1981]_. More information about the theory behind compositing can be found in the works of Jim Blinn [Blinn1994a]_ [Blinn1994b]_ and Alvy Ray Smith [Smith1995]_.

Compositing theory also introduces the notion of "premultiplied" RGB values, or "associated alpha", and there is still a lot of `debate <https://groups.google.com/forum/#!topic/ocio-dev/ZehKhUFqhjc>`_ about `premultiplying or not <https://web.archive.org/web/20161228231715/http://lists.openimageio.org/pipermail/oiio-dev-openimageio.org/2011-December/004709.html>`_.

Like other modern compositors, Natron uses premultiplied alpha by default, because images are stored internally as floating-point values.

Practice
********

There are excellent books that introduce how to do compositing in practice, and how to use compositing software: [Wright2010]_, [Brinkmann2008]_, [Lanier2009]_, [VES2014]_.

Most of what these books describe also applies to Natron, and it is a good idea to get familiar with their techniques and workflows.

Video tutorials for Natron, and for other compositors such as Nuke or Fusion, are available on YouTube and Vimeo.


.. [Wikipedia] `Compositing <https://en.wikipedia.org/wiki/Compositing>`_, in Wikipedia, retrieved Sep. 14, 2016 from https://en.wikipedia.org/wiki/Compositing

.. [PorterDuff1984] Porter, Thomas; Tom Duff (1984). `"Compositing Digital Images" <https://keithp.com/~keithp/porterduff/p253-porter.pdf>`_. Computer Graphics. 18 (3): 253–259. `doi:10.1145/800031.808606 <https://doi.org/10.1145/800031.808606>`_

.. [Wallace1981] Wallace,  Bruce  A., `Merging  and  Transformation  of  Raster  Images  for Cartoon  Animation <https://graphics.stanford.edu/papers/merging-sig81/>`_, Computer  Graphics,  Vol  15,  No  3,  Aug  1981, 253-262. SIGGRAPH’81 Conference Proceedings, `doi:10.1145/800224.806813 <https://doi.org/10.1145/800224.806813>`_.

.. [Blinn1994a] Blinn, James F., Jim Blinn's Corner: Compositing Part 1: Theory, IEEE Computer Graphics & Applications, Sep 1994, 83-87, `doi:10.1109/38.310740 <https://doi.org/10.1109/38.310740>`_.

.. [Blinn1994b] Blinn,  James  F., Jim Blinn's Corner: Compositing Part 2: Practice, IEEE Computer Graphics & Applications, Nov 1994, 78-82, `doi:10.1109/38.329100 <https://doi.org/10.1109/38.329100>`_.

.. [Smith1995]  Alvy Ray Smith, `Image Compositing Fundamentals <https://alvyray.com/Memos/CG/Microsoft/4_comp.pdf>`_, Microsoft Technical Memo 4, 1995.

.. [Brinkmann2008] Ron Brinkmann, The Art and Science of Digital Compositing, 2nd Edition, 2008 (ISBN  0123706386)

.. [Lanier2009] Lee Lanier, Professional Digital Compositing: Essential Tools and Techniques, 2009 (ISBN 0470452617)

.. [Wright2010] Steve Wright, Digital Compositing for Film and Video, Third Edition, 2010 (ISBN 978-0-240-81309-7)

.. [VES2014] The VES Handbook of Visual Effects: Industry Standard VFX Practices and Procedures, 2nd Edition (ISBN 0240825187)

