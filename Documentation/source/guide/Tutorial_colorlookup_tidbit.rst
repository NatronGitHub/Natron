Keyer Node or Chromakey Node: Colorlookup node tidbit
======================================================
**Using the ColorLookup node for better screen matte control,**

This is concerning pulling a key using the Keyer Node or Chromakey Node. I have been practicing using the **Colorlookup node** curves to adjust my keys instead of using the *Key Lift* & *Key Gain* when pulling my keys. I find sometimes on certain material you have to crank the acceptance angle higher than I want to and trying to find the best center position in the *Keyer node* to get that good high contrast matte. Sometime the edges may suffer based on the hue you have to pull.

The **Colorlookup node** curves offers you more control over crushing your blacks and expanding your white to get that perfect contrast. You can create anchors and pivot points in the curve to limit which range you want to adjust, rather it be *shadows*, *mid-tones* or *highlights*. The attached image shows a very basic screen shot of what it looks like in action.

..figure:: _images/keyergraph.jpg

The Key Lift & Key Gain performs great but you don’t have that extra control on the mid tones that may effect your edges. Just remember to insert the Colorlookup node after your keyer/chromakey node and use the Alpha Channel curve to finesse your matte.

..Note:: Also use your *gamma* switch control at the top of the viewer to do a final check and pass on all your keys. This process is called slamming your *gamma*. Most times to the necked eye we can’t see our *mattes* properly if our monitors are not calibrated for the right *gamma* display. This is most useful when using the curves from the **ColorLookup** node for final adjustments.
