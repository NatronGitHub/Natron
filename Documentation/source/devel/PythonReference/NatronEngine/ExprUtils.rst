.. module:: NatronEngine
.. _ExprUtils:

ExprUtils
*************

**Inherits** :doc:`Double2DParam`

Synopsis
--------

Various functions useful for expressions. Most noise functions have been taken from the
Walt Disney Animation Studio SeExpr library.

Functions
^^^^^^^^^

- def :meth:`boxstep<NatronEngine.ExprUtils.boxstep>` (x,a)
- def :meth:`linearstep<NatronEngine.ExprUtils.linearstep>` (x,a,b)
- def :meth:`smoothstep<NatronEngine.ExprUtils.boxstep>` (x,a,b)
- def :meth:`gaussstep<NatronEngine.ExprUtils.gaussstep>` (x,a,b)
- def :meth:`remap<NatronEngine.ExprUtils.remap>` (x,source,range,falloff,interp)
- def :meth:`mix<NatronEngine.ExprUtils.mix>` (x,y,alpha)
- def :meth:`hash<NatronEngine.ExprUtils.hash>` (args)
- def :meth:`noise<NatronEngine.ExprUtils.noise>` (x)
- def :meth:`noise<NatronEngine.ExprUtils.noise>` (p)
- def :meth:`noise<NatronEngine.ExprUtils.noise>` (p)
- def :meth:`noise<NatronEngine.ExprUtils.noise>` (p)
- def :meth:`snoise<NatronEngine.ExprUtils.snoise>` (p)
- def :meth:`vnoise<NatronEngine.ExprUtils.vnoise>` (p)
- def :meth:`cnoise<NatronEngine.ExprUtils.cnoise>` (p)
- def :meth:`snoise4<NatronEngine.ExprUtils.snoise4>` (p)
- def :meth:`vnoise4<NatronEngine.ExprUtils.vnoise4>` (p)
- def :meth:`cnoise4<NatronEngine.ExprUtils.cnoise4>` (p)
- def :meth:`turbulence<NatronEngine.ExprUtils.turbulence>` (p[,ocaves=6, lacunarity=2, gain=0.5])
- def :meth:`vturbulence<NatronEngine.ExprUtils.vturbulence>` (p[,ocaves=6, lacunarity=2, gain=0.5])
- def :meth:`cturbulence<NatronEngine.ExprUtils.cturbulence>` (p[,ocaves=6, lacunarity=2, gain=0.5])
- def :meth:`fbm<NatronEngine.ExprUtils.fbm>` (p[,ocaves=6, lacunarity=2, gain=0.5])
- def :meth:`vfbm<NatronEngine.ExprUtils.vfbm>` (p[,ocaves=6, lacunarity=2, gain=0.5])
- def :meth:`fbm4<NatronEngine.ExprUtils.fbm4>` (p[,ocaves=6, lacunarity=2, gain=0.5])
- def :meth:`vfbm4<NatronEngine.ExprUtils.vfbm4>` (p[,ocaves=6, lacunarity=2, gain=0.5])
- def :meth:`cfbm<NatronEngine.ExprUtils.cfbm>` (p[,ocaves=6, lacunarity=2, gain=0.5])
- def :meth:`cfbm4<NatronEngine.ExprUtils.cfbm4>` (p[,ocaves=6, lacunarity=2, gain=0.5])
- def :meth:`cellnoise<NatronEngine.ExprUtils.cellnoise>` (p)
- def :meth:`ccellnoise<NatronEngine.ExprUtils.ccellnoise>` (p)
- def :meth:`pnoise<NatronEngine.ExprUtils.pnoise>` (p, period)

Member functions description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: NatronEngine.ExprUtils.boxstep (x,a)

    :param x: :class:`float<PySide.QtCore.float>`
    :param a: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`float<PySide.QtCore.float>`

     if x < a then 0 otherwise 1

.. method:: NatronEngine.ExprUtils.linearstep (x,a,b)

    :param x: :class:`float<PySide.QtCore.float>`
    :param a: :class:`float<PySide.QtCore.float>`
    :param b: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`float<PySide.QtCore.float>`

     Transitions linearly when a < x < b

.. method:: NatronEngine.ExprUtils.boxstep (x,a,b)

    :param x: :class:`float<PySide.QtCore.float>`
    :param a: :class:`float<PySide.QtCore.float>`
    :param b: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`float<PySide.QtCore.float>`


    Transitions smoothly (cubic) when a < x < b


.. method:: NatronEngine.ExprUtils.gaussstep (x,a,b)

    :param x: :class:`float<PySide.QtCore.float>`
    :param a: :class:`float<PySide.QtCore.float>`
    :param b: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`float<PySide.QtCore.float>`

    Transitions smoothly (exponentially) when a < x < b

.. method:: NatronEngine.ExprUtils.remap (x,source,range,falloff,interp)

    :param x: :class:`float<PySide.QtCore.float>`
    :param source: :class:`float<PySide.QtCore.float>`
    :param range: :class:`float<PySide.QtCore.float>`
    :param falloff: :class:`float<PySide.QtCore.float>`
    :param interp: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`float<PySide.QtCore.float>`

    General remapping function.
    When **x** is within +/- **range** of **source**, the result is 1.
    The result falls to 0 beyond that range over **falloff** distance.
    The falloff shape is controlled by **interp**:
    linear = 0
    smooth = 1
    gaussian = 2

.. method:: NatronEngine.ExprUtils.mix (x,y,alpha)

    :param x: :class:`float<PySide.QtCore.float>`
    :param y: :class:`float<PySide.QtCore.float>`
    :param alpha: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`float<PySide.QtCore.float>`

    Linear interpolation of a and b according to alpha

.. method:: NatronEngine.ExprUtils.hash (args)

    :param args: :class:`Sequence`
    :rtype: :class:`float<PySide.QtCore.float>`

    Like random, but with no internal seeds. Any number of seeds may be given
    and the result will be a random function based on all the seeds.

.. method:: NatronEngine.ExprUtils.noise (x)

    :param x: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`float<PySide.QtCore.float>`

    Original perlin noise at location (C2 interpolant)

.. method:: NatronEngine.ExprUtils.noise (p)

    :param p: :class:`Double2DTuple<NatronEngine.Double2DTuple>`
    :rtype: :class:`float<PySide.QtCore.float>`

    Original perlin noise at location (C2 interpolant)

.. method:: NatronEngine.ExprUtils.noise (p)

    :param p: :class:`Double3DTuple<NatronEngine.Double3DTuple>`
    :rtype: :class:`float<PySide.QtCore.float>`

    Original perlin noise at location (C2 interpolant)


.. method:: NatronEngine.ExprUtils.noise (p)

    :param p: :class:`ColorTuple<NatronEngine.ColorTuple>`
    :rtype: :class:`float<PySide.QtCore.float>`

    Original perlin noise at location (C2 interpolant)


.. method:: NatronEngine.ExprUtils.snoise (p)

    :param p: :class:`Double3DTuple<NatronEngine.Double3DTuple>`
    :rtype: :class:`float<PySide.QtCore.float>`


    Signed noise w/ range -1 to 1 formed with original perlin noise at location (C2 interpolant)


.. method:: NatronEngine.ExprUtils.vnoise (p)

    :param p: :class:`Double3DTuple<NatronEngine.Double3DTuple>`
    :rtype: :class:`Double3DTuple<NatronEngine.Double3DTuple>`

    Vector noise formed with original perlin noise at location (C2 interpolant)

.. method:: NatronEngine.ExprUtils.cnoise (p)

    :param p: :class:`Double3DTuple<NatronEngine.Double3DTuple>`
    :rtype: :class:`Double3DTuple<NatronEngine.Double3DTuple>`

    Color noise formed with original perlin noise at location (C2 interpolant)

.. method:: NatronEngine.ExprUtils.snoise4 (p)

    :param p: :class:`ColorTuple<NatronEngine.ColorTuple>`
    :rtype: :class:`float<PySide.QtCore.float>`

    4D signed noise w/ range -1 to 1 formed with original perlin noise at location (C2 interpolant)

.. method:: NatronEngine.ExprUtils.vnoise4 (p)

    :param p: :class:`ColorTuple<NatronEngine.ColorTuple>`
    :rtype: :class:`Double3DTuple<NatronEngine.Double3DTuple>`

    4D vector noise formed with original perlin noise at location (C2 interpolant)

.. method:: NatronEngine.ExprUtils.cnoise4 (p)

    :param p: :class:`ColorTuple<NatronEngine.ColorTuple>`
    :rtype: :class:`Double3DTuple<NatronEngine.Double3DTuple>`

    4D color noise formed with original perlin noise at location (C2 interpolant)"


.. method:: NatronEngine.ExprUtils.turbulence (p[,ocaves=6, lacunarity=2, gain=0.5])

    :param p: :class:`Double3DTuple<NatronEngine.Double3DTuple>`
    :param octaves: :class:`int<PySide.QtCore.int>`
    :param lacunarity: :class:`float<PySide.QtCore.float>`
    :param gain: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`float<PySide.QtCore.float>`

     FBM (Fractal Brownian Motion) is a multi-frequency noise function.
     The base frequency is the same as the noise function. The total
     number of frequencies is controlled by **octaves**. The **lacunarity** is the
     spacing between the frequencies - A value of 2 means each octave is
     twice the previous frequency. The **gain** controls how much each
     frequency is scaled relative to the previous frequency.

.. method:: NatronEngine.ExprUtils.vturbulence (p[,ocaves=6, lacunarity=2, gain=0.5])

    :param p: :class:`Double3DTuple<NatronEngine.Double3DTuple>`
    :param octaves: :class:`int<PySide.QtCore.int>`
    :param lacunarity: :class:`float<PySide.QtCore.float>`
    :param gain: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`Double3DTuple<NatronEngine.Double3DTuple>`

     FBM (Fractal Brownian Motion) is a multi-frequency noise function.
     The base frequency is the same as the noise function. The total
     number of frequencies is controlled by **octaves**. The **lacunarity** is the
     spacing between the frequencies - A value of 2 means each octave is
     twice the previous frequency. The **gain** controls how much each
     frequency is scaled relative to the previous frequency.

.. method:: NatronEngine.ExprUtils.cturbulence (p[,ocaves=6, lacunarity=2, gain=0.5])

    :param p: :class:`Double3DTuple<NatronEngine.Double3DTuple>`
    :param octaves: :class:`int<PySide.QtCore.int>`
    :param lacunarity: :class:`float<PySide.QtCore.float>`
    :param gain: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`Double3DTuple<NatronEngine.Double3DTuple>`

     FBM (Fractal Brownian Motion) is a multi-frequency noise function.
     The base frequency is the same as the noise function. The total
     number of frequencies is controlled by **octaves**. The **lacunarity** is the
     spacing between the frequencies - A value of 2 means each octave is
     twice the previous frequency. The **gain** controls how much each
     frequency is scaled relative to the previous frequency.

.. method:: NatronEngine.ExprUtils.fbm (p[,ocaves=6, lacunarity=2, gain=0.5])

    :param p: :class:`Double3DTuple<NatronEngine.Double3DTuple>`
    :param octaves: :class:`int<PySide.QtCore.int>`
    :param lacunarity: :class:`float<PySide.QtCore.float>`
    :param gain: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`float<PySide.QtCore.float>`

     FBM (Fractal Brownian Motion) is a multi-frequency noise function.
     The base frequency is the same as the noise function. The total
     number of frequencies is controlled by **octaves**. The **lacunarity** is the
     spacing between the frequencies - A value of 2 means each octave is
     twice the previous frequency. The **gain** controls how much each
     frequency is scaled relative to the previous frequency.

.. method:: NatronEngine.ExprUtils.vfbm (p[,ocaves=6, lacunarity=2, gain=0.5])

    :param p: :class:`Double3DTuple<NatronEngine.Double3DTuple>`
    :param octaves: :class:`int<PySide.QtCore.int>`
    :param lacunarity: :class:`float<PySide.QtCore.float>`
    :param gain: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`Double3DTuple<NatronEngine.Double3DTuple>`

     FBM (Fractal Brownian Motion) is a multi-frequency noise function.
     The base frequency is the same as the noise function. The total
     number of frequencies is controlled by **octaves**. The **lacunarity** is the
     spacing between the frequencies - A value of 2 means each octave is
     twice the previous frequency. The **gain** controls how much each
     frequency is scaled relative to the previous frequency.

.. method:: NatronEngine.ExprUtils.fbm4 (p[,ocaves=6, lacunarity=2, gain=0.5])

    :param p: :class:`Double3DTuple<NatronEngine.Double3DTuple>`
    :param octaves: :class:`int<PySide.QtCore.int>`
    :param lacunarity: :class:`float<PySide.QtCore.float>`
    :param gain: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`float<PySide.QtCore.float>`

     FBM (Fractal Brownian Motion) is a multi-frequency noise function.
     The base frequency is the same as the noise function. The total
     number of frequencies is controlled by **octaves**. The **lacunarity** is the
     spacing between the frequencies - A value of 2 means each octave is
     twice the previous frequency. The **gain** controls how much each
     frequency is scaled relative to the previous frequency.

.. method:: NatronEngine.ExprUtils.vfbm4 (p[,ocaves=6, lacunarity=2, gain=0.5])

    :param p: :class:`Double3DTuple<NatronEngine.Double3DTuple>`
    :param octaves: :class:`int<PySide.QtCore.int>`
    :param lacunarity: :class:`float<PySide.QtCore.float>`
    :param gain: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`Double3DTuple<NatronEngine.Double3DTuple>`

     FBM (Fractal Brownian Motion) is a multi-frequency noise function.
     The base frequency is the same as the noise function. The total
     number of frequencies is controlled by **octaves**. The **lacunarity** is the
     spacing between the frequencies - A value of 2 means each octave is
     twice the previous frequency. The **gain** controls how much each
     frequency is scaled relative to the previous frequency.

.. method:: NatronEngine.ExprUtils.cfbm (p[,ocaves=6, lacunarity=2, gain=0.5])

    :param p: :class:`Double3DTuple<NatronEngine.Double3DTuple>`
    :param octaves: :class:`int<PySide.QtCore.int>`
    :param lacunarity: :class:`float<PySide.QtCore.float>`
    :param gain: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`Double3DTuple<NatronEngine.Double3DTuple>`

     FBM (Fractal Brownian Motion) is a multi-frequency noise function.
     The base frequency is the same as the noise function. The total
     number of frequencies is controlled by **octaves**. The **lacunarity** is the
     spacing between the frequencies - A value of 2 means each octave is
     twice the previous frequency. The **gain** controls how much each
     frequency is scaled relative to the previous frequency.

.. method:: NatronEngine.ExprUtils.cfbm4 (p[,ocaves=6, lacunarity=2, gain=0.5])

    :param p: :class:`Double3DTuple<NatronEngine.Double3DTuple>`
    :param octaves: :class:`int<PySide.QtCore.int>`
    :param lacunarity: :class:`float<PySide.QtCore.float>`
    :param gain: :class:`float<PySide.QtCore.float>`
    :rtype: :class:`Double3DTuple<NatronEngine.Double3DTuple>`

     FBM (Fractal Brownian Motion) is a multi-frequency noise function.
     The base frequency is the same as the noise function. The total
     number of frequencies is controlled by **octaves**. The **lacunarity** is the
     spacing between the frequencies - A value of 2 means each octave is
     twice the previous frequency. The **gain** controls how much each
     frequency is scaled relative to the previous frequency.


.. method:: NatronEngine.ExprUtils.cellnoise (p)

    :param p: :class:`Double3DTuple<NatronEngine.Double3DTuple>`
    :rtype: :class:`float<PySide.QtCore.float>`

     cellnoise generates a field of constant colored cubes based on the integer location
     This is the same as the prman cellnoise function

.. method:: NatronEngine.ExprUtils.ccellnoise (p)

    :param p: :class:`Double3DTuple<NatronEngine.Double3DTuple>`
    :rtype: :class:`Double3DTuple<NatronEngine.Double3DTuple>`

    cellnoise generates a field of constant colored cubes based on the integer location
    This is the same as the prman cellnoise function


.. method:: NatronEngine.ExprUtils.pnoise (p, period)


    :param p: :class:`Double3DTuple<NatronEngine.Double3DTuple>`
    :param period: :class:`Double3DTuple<NatronEngine.Double3DTuple>`
    :rtype: :class:`float<PySide.QtCore.float>`

    Periodic noise


