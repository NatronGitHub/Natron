.. for help on writing/extending this file, see the reStructuredText cheatsheet
    http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf

Plasma Node
=========

Creates cloudy  noise. Brightness of the result can be modulated by the source image
    
.. image:: _images/TheNodes-Draw/nodes_draw_plasma01.jpg


Usage
--------

.. image:: _images/TheNodes-Draw/nodes_draw_plasma02.jpg

The "Scale" parameter changes the size of the clouds pattern

check "Static Seed" for a freeze frame of the effect

.. image:: _images/TheNodes-Draw/nodes_draw_plasma03.jpg

Above:
    - high alpha/low beta gives clean clouds

    - low alpha / high beta gives noisy clouds

This node alone is not suitable for image regrain. but with a scale of 1 it can partly simulate the splotchy behavior of high speed film stocks


.. toctree::
    :maxdepth: 2
