Multipass Keying Pipeline
=========================


This response may be an overkill of information but here goes. Most chromakeying or pulling keys issues will have to do with lighting and channel noise. Noise is most apparent in the Red and Blue channels. Like I had posted before, using nodes like lp_CleanScreen1 or PxF_ScreenClean1 can help flatten the color range of the green screen. Now there are times when your green screen surface is noisy because of bad lighting period. Then the DenoiseSharpen1 node will come in handy the smooth out the channels to get a decent key. After you have done all of that, applying a multipass Chromakeying workflow should follow in the pipeline. See attached image for visual reference. I had used the screen clean node, DenoiseSharpen1 node and 3 ChromaKeyer nodes in a multipass piping.

..figure:: _images/MPKeying.jpg

The Chromakey nodes needs to generate inner core mattes and then additional keys. Each time you need to apply an additional ChromaKeyer node, the Output Mode needs the "Intermediate" option selected Source Alpha "None" selected. This generates the Core Matte (AKA Hard Edges). The second ChromaKeyer Output Mode can be premultiplied if it is the last keyer in the chain or Intermediate for more multipass keying. Rather ChromaKeyer node is the last keyer or use for more multipass keying, the Source Alpha "Add to inside mask" must be selected. Every ChromaKeyer node after the first ChromaKeyer in the pipeline for multipass keying must have these options selected: Output Mode needs the "Intermediate" option selected Source Alpha "Add to inside mask" selected. Only the last ChromaKeyer node in the chain Output Mode should be premultiplied.

Sorry for the redundancy. Multipass keying gets a little complex once you need to go pass 3 layers.
