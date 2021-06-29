Green Screen Despilling Tips
============================

This tutorial project was filmed in the One Button Studios at University at Buffalo, NY. I am sure that I have posted this technique or workflow before but I wanted to show more screen shots in a little more detail as to how I use Natron’s green screen Dispeller nodes. Thanks to Frederick and other open source community plugin developers, we have about 3 Dispeller nodes. We all know what the dispeller node does but each one of these nodes has features called " Spillmap to Alpha , Shuffle Spillmatte to Alpha and Output Spill Matte in Alpha ." These are from 3 different nodes but they all do the same thing. These nodes are named Despill, lp_ChillSpill_smp and DespillMadness.

In the image below I had connected all three despillers in the node graph to create a mini pipeline for basic visual references.
..figure:: _images/dispell1.png

As you can see each despiller gives you a different suppression result but the matte outcome will be the same, but it’s contingent upon the Algorithm used(Avg, Max, Min). The next image is the spillmatte before using a Colorlookup node to crush and expand the black and white using the ChillSpill node.

..figure:: _images/dispell2.jpg

The next image is with the Colorlookup node crushing and expanding the black and white using the ChillSpill node.
..figure:: _images/dispell3.png

..note:: NOTE: When using the Colorlookup to crush and expand your matte, be sure to have the black clamp and white clamp activated or use a Clamp node. Some of you can push to far and your matte values can generate negative values and etc.

The image below is all three Depiller nodes with their spillmap mattes crushed and expanding with the Colorlookup nodes. Also the far left viewer node shows the result of a node processing my Color.RGBA image. You will have to use an invert node to reverse the matte so that it shows the green as the black and the talent (Me) in white. Now I am using the Despillers as an alternative keyer processors. I still have to add another Depiller node because the first spillmap processor performed an Avg algorithm and because this footage was converted to a highly compressed .MP4 format, artifacts on my edges were evident. Follow my node graph pipeline and you can see how powerful Natron’s spillmap matte generators can be having to use any color difference keyers, chromakeyers or color keyers. This workflow does not replace those colorspace designed keyers but it can be added or used as an addition to the professional keyers to help keep your edges and edge mattes smoother.
..figure:: _images/dispell4.png

Then you can apply color corrections and etc to finish your shot.
..figure:: _images/dispell5.png

These are just some of my workflows that I use Natron for in my daily green screen productions.
