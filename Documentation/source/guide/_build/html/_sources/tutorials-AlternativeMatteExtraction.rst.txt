Alternative Matte Extraction Tutorial
=====================================

.. figure:: _images/AlternativeMatteExtraction/ArticleCoverPage.jpg
    :width: 650px
    :align: center

In the world of vfx in current films these days, it is hard to even phathom that **pulling keys (aka chromakeying)** or generally just creating mattes from images is not common place. Today I want to share some features in a few nodes that are in **Natron VFX Digital Compositor**. The nodes that I want to discuss are **Despill**, **Ip_ChillSpill** and **ColorSuppression**. You can guess by the name the functions that they perform. Basically, they **subtract any blue or green screen spillage that happens to contaminate your foreground objects during the production process**. These types of functions are common place in every post-production facilities in the industry. It doesn't matter if you are a beginner wanting to produce your own short films or a professional working on block buster films. The needs are the same. The attached image is a greenscreen image that I pulled off google to demonstrate what the Natron developers had implemented to take these despillers to another level or just added functionality.

.. figure:: _images/AlternativeMatteExtraction/HeBeGreeen.jpg
    :width: 200px
    :align: center

The added functionality is having the ability to use the suppressed or despill color information and convert it to a matte or alpha. I am unaware if any other compositing applications has these abilities. Natron is my main compositing app and from time to time I use these **despilling node algorithms** to help generate masks, general mattes and scaled alphas. This is Natron's node graph pipeline for each node that I will be discussing. The pipeline for each node is really simple. You just connect the green/blue screen footage to the input of the nodes, adjust whatever you have to adjust, click on the very simple knob that says "Spillmap to Alpha" if you are using the Despill node, "Shuffle Spillmatte to Alpha" if you are using the community openfx plugin called **Ip_ChillSpill Node**, and **"Output: Image, Alpha & Image and Alpha"** if you are using the ColorSuppression node.

.. figure:: _images/AlternativeMatteExtraction/ArticleCoverPage.jpg
    :width: 200px
    :align: center

The first screenshot demonstration if for the node **Ip_ChillSpill**. This despilling node has the most of features and functions that I can tell that exist amongst all the despilling nodes. in the image below you will see the spill suppression on the left and the alpha channel from selecting **"Shuffle Spillmatte to Alpha"** on the right.

.. figure:: _images/AlternativeMatteExtraction/Ip_ChillSpill-FullComp.jpg
    :width: 200px
    :align: center

Now just selecting the **Shuffle Spillmatte to Alpha** feature is not some magic trick and you get a perfect matte, not by a long shot. For a matter a fact its not for any of them. Attached are example of the nodes in their default state before the scaling process begins. The first image is the **Ip_ChillSpill** default matte output and the second in the ColorSuppression default matte output. It looks like if I was trying to use the **HSVTool Node** to pull a **Saturation or Brightness Key**. You can read more about that in my HSVTool node `tutorial <https://colorgrade13.wordpress.com/2015/05/07/natron-digital-compositor-hsvtool-node-tutorial/>`_. I used a very underated and under used node amongst beginners called the **ColorLookup Node**. You can be very familiar with the node if you have used Photoshop or Gimp's curve tool.

.. figure::  _images/AlternativeMatteExtraction/Ip_ChillSpill-default.jpg
    :width: 200px
    :align: center

Here are the nodes and their settings to show what I had to do to get it to scale my suppression mattes. The key tool is to use the **ColorLookup Node** connected after the despilling nodes. The **ColorLookup Node** four color channels and the channel that you use the scale your mattes is the **"alpha curve channel"**. The bottom left of the **alpha curve is used the to crush your blacks/shadows** and the top right is used to **extend your white/highlights**. In the **ColorLookup Node** you will also see a feature called "Luminance Math". This feature will yield its full benefits based off the resolution and color spaces of your footage. The ColorLookup node is very powerful in a sense because the channel curves can have multiple points to limit its effects.

.. figure::  _images/AlternativeMatteExtraction/ColorSuppression-default.jpg
    :width: 200px
    :align: center

As you can see these nodes all perform the same functions but some has different parameters to accomplishes the same thing and well as providing other color processing effects. Here are some screen captures of the effects using the **ColorLookup Node**.

.. figure::  _images/AlternativeMatteExtraction/Ip_ChillSpill.jpg
    :width: 200px
    :align: center

.. figure::  _images/AlternativeMatteExtraction/AllColorLookup.jpg
    :width: 200px
    :align: center

**Now this by node means a primary replacement for powerful keying node in Natron. The extended functionality should only be considered as compliment to Chromakeyer, PIK/PIK Color and Keyer nodes**. Just remember that the Here are some screen captures of the effects using the **ColorLookup node is needed to scaled that matte**. Also this process doesn't treat your edges with a choking or eroding effect. You would have to experiment with some of the filters to process them. Now you can try and cheat by using the **Shuffle Node** to convert to this matte output to an real alpha channel and maybe you can process your edges directly as if you were coming out of a keyer. You will need to do some serious testing. After you have done that, please feel free to talk about it and join `NatronNation <https://www.facebook.com/groups/NatronNation/>`_ and read my `blog <https://colorgrade13.wordpress.com/>`_.

Despill and Color Suppression Pipeline
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure::  _images/AlternativeMatteExtraction/Ip_ChillSpill-FullComp.jpg
    :width: 200px
    :align: center


.. figure::  _images/AlternativeMatteExtraction/Despill.jpg
    :width: 200px
    :align: center


.. figure::  _images/AlternativeMatteExtraction/ColorSuppression.jpg
    :width: 200px
    :align: center


.. figure::  _images/AlternativeMatteExtraction/ReversedMatte.jpg
    :width: 200px
    :align: center
