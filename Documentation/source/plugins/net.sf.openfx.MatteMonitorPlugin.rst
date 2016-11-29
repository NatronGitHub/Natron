.. _net.sf.openfx.MatteMonitorPlugin:

MatteMonitor
============

*This documentation is for version 1.0 of MatteMonitor.*

A Matte Monitor: make alpha values that are strictly between 0 and 1 more visible.

After applying a Keyer, a scaling operation is usually applied to clean the matte. However, it is difficult to visualize on the output values that are very close to 0 or 1, but not equal. This plugin can be used to better visualize these values: connect it to the output of the scaling operator, then to a viewer, and visualize the alpha channel.

Alpha values lower or equal to 0 and greater or equal to 1 are leaved untouched, and alpha values in between are stretched towards 0.5 (using the slope parameter), making them more visible.

The output of this plugin should not be used for firther processing, but only for viewing.

The Matte Monitor is described in "Digital Compositing for Film and Video" by Steve Wright (Sec. 3.1).

See also the video at http://www.vfxio.com/images/movies/Comp\_Tip\_2.mov

Inputs
------

+----------+---------------+------------+
| Input    | Description   | Optional   |
+==========+===============+============+
| Source   |               | No         |
+----------+---------------+------------+

Controls
--------

+-------------------+---------------+----------+-----------------+------------------------------------------------------------+
| Label (UI Name)   | Script-Name   | Type     | Default-Value   | Function                                                   |
+===================+===============+==========+=================+============================================================+
| Slope             | slope         | Double   | 0.5             | Slope applied to alpha values striuctly between 0 and 1.   |
+-------------------+---------------+----------+-----------------+------------------------------------------------------------+
