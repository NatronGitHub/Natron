.. for help on writing/extending this file, see the reStructuredText cheatsheet
   http://github.com/ralsina/rst-cheatsheet/raw/master/rst-cheatsheet.pdf
   
Tracking and stabilizing
========================

.. toctree::
   :maxdepth: 2

Workflow Summary
----------------

In order to track a planar shape and move a Roto mask or a texture corresponding to that shape:

- Track some points inside your mask (shape)
- In the Transform tab, set the transform to CornerPin and to match-move
- Disable the CornerPin and set the from points of the corner pin at the reference frame where you want your object to move in (basically the bounding box of the shape to track)
- Export to CornerPin
- Append your CornerPin to the Roto node 

In a future version we will have a planar tracker that will do that automatically for you in a single click. 

Detailed Usage
--------------

To link parameters in Natron, it is the same as in Nuke except that you drag and drop the widget of a parameter onto another one by holding the control key (or cmd on macOS).

The tracker works differently than the Nuke tracker regarding the "Transform" part. For the tracking itself, almost everything is the same. Basically, in Nuke they can only output a CornerPin with exactly 4 points, and they map 1 track to each corner of the CornerPin. For the Transform node they may use 1 (translation only), 2, or N points to find the final transformation, however that will never be something other than a `similarity <https://en.wikipedia.org/wiki/Similarity_(geometry)>`_, which means that it cannot handle perspective deformation.

In Natron, we offer the possibility to compute a CornerPin with N points, that is an `homography <https://en.wikipedia.org/wiki/Homography>`_, which encompasses all distortion-free perspective transforms.

This is much better, because the more tracks you use to compute that CornerPin, the more robust it will be.

An homography is typically used to contain information about a perspective deformation, whereas a similarity is more constrained: a similarity is translation, rotation and uniform scale.

In The Transform tab, this is what we call "the model". 
Basically, the problem we are trying to solve is to fit a model (i.e. similarity or homography) so it is the closest to the N point correspondences. Each correspondence is the position of a track at the reference frame and its position at the tracked time. 

Hence the more correspondences you have (i.e. the more tracks), the more robust the homography is in the region where you tracked features.

The *Fitting error* parameter (in the Transform tab) is an indication of how much difference there is in pixels between the reference point on which we applied the computed transformation and the original tracked point. This is the RMS (root mean square) error across all tracks, and gives an estimate of the quality of the model found in pixel units. 

For each tracked frame, the *correspondences* we use to compute the CornerPin are the tracks that are *enabled* at this frame (i.e. the Enabled parameter is checked at this time) and that have a keyframe on the center (i.e. they successfully tracked). 

When you press *Compute*, it computes the model (CornerPin/Transform) with all the tracks that meet the aforementioned requirements *over all keyframes*.

When *Compute Transform Automatically* is checked, whenever a parameter that has an effect on the output model is changed, this will recompute the Corner/Pin transform *over all keyframes* again.
 
The parameters that have an effect on the output model are:

- The motion type
- The Transform Type (i.e. Similarity or Homography)
- The Reference Frame 
- Jitter Period
- Smooth: this can be used to smooth the resulting curve to remove some of the noise in the high frequencies of the CornerPin/Transform. Note that in "Add Jitter" mode, you can increase High frequencies to simulate a camera shake that follows the original camera movements.
- Robust model: this is quite complicated, but in short: When trying to find a model that *best fits* all correspondences, you may have correspondences that are just wrong (bad tracking for example). These bad correspondences are called *outliers*, and this parameter when checked tells  we should not take into account those outliers to compute the final model. In most cases this should be checked. However sometimes, the user may have for example required to compute an homography (i.e. CornerPin), but the given tracked points (correspondences) just cannot make-up an homography. In this case, if the parameter were to be checked, it would fail to compute a model. If you uncheck this, it will take into account all the points and compute a model that averages the motion of all correspondences.

Also when *Compute Transform Automatically* is checked, the model will be computed automatically when the tracking ends.

We cannot compute the model after each track step (i.e. during tracking) because the model at each frame depends on the model at other frames since we may smooth the curve or add jitter. 

So all in all it works differently than Nuke, the whole transformation computation can be more robust and happens as a second pass after the tracking is actually done.

One last thing: to compute the CornerPin in the "Transform" tab of the tracker, the **to** points are computed using the **from** points as reference. 

Basically what happens is that the tracking outputs a transformation matrix at each frame. Then when computing the model, this matrix is applied to the **from** points at each frame in order to obtain the **to** points.

So if you were to change the reference points (i.e. the **from** points) with the *Set to input RoD* for example, then you would need to recompute the model at all frames, because the **to** points would just not be the same. 

The work is usually done in two steps:

- First, disable the CornerPin so that even if the viewer is connected to the Tracker there is no deformation going on, and set the **from** points to be the RoD (bounding box) of the Roto shape at the reference frame.
- Then, export the CornerPin. It just links the parameters of the CornerPin to the ones in the tracker, so if you change something in the tracker transform tab the changes will reflect onto the CornerPin.

Basically what the Planar tracker will do in the future is automatically do all the steps for you: it will place markers inside the mask for you, track them and output a CornerPin from the bounding box of the roto shape.
