#ifndef _ofxNatron_h_
#define _ofxNatron_h_

/** @brief The kOfxPropName property of the Natron Host */
#define kNatronOfxHostName          "fr.inria.Natron"

/** @brief int x 1 property used on the image effect host descriptor. 
 It indicates whether the host is Natron as another mean than just the host name.
 This was added because it may be required to activate certain plug-ins by changing the host-name 
 (since some plug-ins don't allow to be loaded on any host)
 but to keep some other functionnalities specific to the Natron extensions working.
 
Valid values:
 - 0: Indicates that the host is not Natron
 - 1: Indicates that the host is Natron and can support all extensions provided by this header
 
 Default value:
 - 0
 */
#define kNatronOfxHostIsNatron "NatronOfxHostIsNatron"

/** @brief int x 1 property used on the image effect host descriptor.
 It indicates whether the host supports dynamic choice param entries changes or not.
 
 Reasoning:
 By default most of hosts expect the entries of a choice param to be statically defined in the describeInContext action.
 However this might often be a need to re-populate the entries depending on other dynamic parameters, such as the pixel
 components. 
 You would then have to call propReset on the kOfxParamPropChoiceOption property via the param suite and then call
 propSetString on the kOfxParamPropChoiceOption property to re-build the entries list.
 
 Valid values:
 - 0: Indicates that the host does not support dynamic choices
 - 1: Indicates that the host supports dynamic choices
 
 Default value:
 - 0
 */
#define kNatronOfxParamHostPropSupportsDynamicChoices "NatronOfxParamHostPropSupportsDynamicChoices"

/** @brief The name of a string parameter that maps to a sublabel in the GUI,
    e.g. the name of the file in a Reader node, or the name of the operation
    in a Merge node, or the name of the track in a Tracker node.

    Whenever the host sets the value of this parameter, 
    kOfxActionInstanceChanged is called (as with any other parameter).
    It may be kOfxParamPropSecret=1 if it should not appear in the instance UI.
    It may be kOfxParamPropEnabled=0 if only the host should set its value via
    a special GUI.

    This parameter should have the properties
    - kOfxParamPropPersistent=1,
    - kOfxParamPropEvaluateOnChange=0.
    You should set its default value during the kOfxImageEffectActionDescribeInContext action.
 */
#define kNatronOfxParamStringSublabelName "NatronOfxParamStringSublabelName"


/*
  ----------------------------------------------- -----------------------------------------------------------------
 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 Natron multi-plane definitions and extensions brought to Nuke multi-plane extensions defined in fnOfxExtensions.h:
 ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 ----------------------------------------------- -----------------------------------------------------------------
 
 Definitions:
 ------------
 
 - Layer: Corresponds to 1 image plane and has a unique name
 
 - Components: The "type" of data (i.e: the number of channels) contained by a Layer. This may be equal to any of the default
 components defined by OpenFX (e.g: kOfxImageComponentRGBA) but also to the one added by fnOfxExtensions.h (namely 
 kFnOfxImagePlaneForwardMotionVector or kFnOfxImagePlaneBackwardMotionVector) and finally to the custom planes extension
 defined by Natron.
 
 ----------------------------------------------- -----------------------------------------------------------------

The Foundry multi-plane suite:
------------------------------
 
 - In The Foundry specification, some layers are paired and can be requested at the same time:
 this is the (kFnOfxImagePlaneBackwardMotionVector, kFnOfxImagePlaneForwardMotionVector) and (kFnOfxImagePlaneStereoDisparityLeft, kFnOfxImagePlaneStereoDisparityRight) layers. A pair means both layers have the same components type and are generally rendered together.
 These layers are the only one added by this extension.
 
 - The color layer (kFnOfxImagePlaneColour) can have the default OpenFX types.
 
 - The only plug-ins to support The Foundry multi-plane suite are the Furnace ones (among potentially others) and the only (known) hosts to support it are Natron and Nuke.
 
 - In The Foundry multi-plane suite, the plug-in specify that it wants to operate on a motion vector plane or disparity plane by setting kFnOfxImageComponentMotionVectors or kFnOfxImageComponentStereoDisparity on the clip components during the getClipPreferences action. They do not support other planes.

 - The getClipComponents action is unused.
 
 - If the clip components are set to kFnOfxImageComponentMotionVectors or kFnOfxImageComponentStereoDisparity it is expected that the following render actions are called on both paired planes (the plug-in will attempt to fetch both from the render action).
 
Natron modifications:
---------------------
 
 - Some file formats (OpenEXR, TIFF) allow multiple arbitrary image layers (= planes) to be embedded in a single file.
 In the same way, a host application may want/need to use multiple arbitrary image layers into the same image stream
 coming from a clip.
 The multi-plane extension defined in fnOfxExtensions.h by The Foundry has nothing set in this regard and we had to come-up
 with one.
 
 A custom Layer (a.k.a plane) is defined as follows:
 
 - A unique name, e.g: "fr.myRenderer.RenderLayer"
 - An optional label, e.g: "RenderLayer". If empty, this will be set to the name.
 - A set of 1 to 4 channels represented by strings, e.g: ["R","G","B","A"]
 - An optional string to better illustrate the channels type, e.g: "MotionVectors" would better represent "XY". If empty, this will be set to the concatenation of all channel strings.
 
 Typically it would be presented like this to the user in a choice parameter:
 
 RenderLayer.RGBA
 DisparityLeft.Disparity
 
 Internally instead of passing this string and parsing it, we encode the layer as such:
 
 kNatronOfxImageComponentsPlaneName + planeName +
 <optional> kNatronOfxImageComponentsPlaneLabel + planeLabel +
 <optional> kNatronOfxImageComponentsPlaneChannelsLabel + channelsLabel +
 kNatronOfxImageComponentsPlaneChannel + channel1Name +
 kNatronOfxImageComponentsPlaneChannel + channel2Name +
 kNatronOfxImageComponentsPlaneChannel + channel3Name

 Examples:

 kNatronOfxImageComponentsPlaneName + "fr.unique.id.position" + kNatronOfxImageComponentsPlaneLabel + "Position" + kNatronOfxImageComponentsPlaneChannel + "X" + kNatronOfxImageComponentsPlaneChannel + "Y" + kNatronOfxImageComponentsPlaneChannel + "Z"


 kNatronOfxImageComponentsPlaneName + "DisparityLeft" + kNatronOfxImageComponentsPlaneChannelsLabel + "Disparity" + kNatronOfxImageComponentsPlaneChannel + "X" + kNatronOfxImageComponentsPlaneChannel + "Y"

 



 Natron custom layers can be passed wherever layers are used (clipGetImage,render action) or components are used: getClipComponents. They may not be used
 in getClipPreferences.
 
 - Multi-plane effects (kFnOfxImageEffectPropMultiPlanar=1) request their layers via the getClipComponents action
 
 - getClipPreferences has not changed and may only be used to specify the type of components onto which the color layer (kFnOfxImagePlaneColour) will be mapped
 
 - Multi-plane effects (kFnOfxImageEffectPropMultiPlanar=1) are expected to support arbitrary component types and should not rely on the components set during getClipPreferences except for the kFnOfxImagePlaneColour layer.

 - OpenFX didn't allow to pass 2-channel image natively through a clip, even for plug-ins that are not multi-plane.
 The color layer (kFnOfxImagePlaneColour) can now have the default OpenFX types of components as well as kNatronOfxImageComponentXY to specify 2-channel images.
 kNatronOfxImageComponentXY may be specified wherever default OpenFX components types are used. 
 

 
 
Notes:
------
 
 - Layers are what is passed to the render actionn getClipComponents action and clipGetImage function whereas components are what is used for getClipPreferences

 - The kFnOfxImageEffectPropComponentsPresent property on clip instances must return a list of planes available on that clip (not components).
 
 */


/** @brief string property to indicate the presence of custom components on a clip in or out.
  The string should be formed as such:

  kNatronOfxImageComponentsPlaneName + 
  planeName +
  <optional> kNatronOfxImageComponentsPlaneLabel + planeLabel +
  <optional> kNatronOfxImageComponentsPlaneChannelsLabel + channelsLabel +
  kNatronOfxImageComponentsPlaneChannel + channel1Name +
  kNatronOfxImageComponentsPlaneChannel + channel2Name +
  kNatronOfxImageComponentsPlaneChannel + channel3Name
 
  Examples:
  
  kNatronOfxImageComponentsPlaneName + "fr.unique.id.position" + kNatronOfxImageComponentsPlaneLabel + "Position" + kNatronOfxImageComponentsPlaneChannel + "X" + kNatronOfxImageComponentsPlaneChannel + "Y" + kNatronOfxImageComponentsPlaneChannel + "Z"
    
  kNatronOfxImageComponentsPlaneName + "DisparityLeft" + kNatronOfxImageComponentsPlaneChannelsLabel + "Disparity" + kNatronOfxImageComponentsPlaneChannel + "X" + kNatronOfxImageComponentsPlaneChannel + "Y"
 
  This indicates to the host in which plane should the given components appear and how many pixel channels it contains.
  It can be used at any place where kOfxImageComponentAlpha, kOfxImageComponentRGB, kOfxImageComponentRGBA, kOfxImageComponentNone (etc...) is
  given.
 */
#define kNatronOfxImageComponentsPlaneName  "NatronOfxImageComponentsPlaneName_"

#define kNatronOfxImageComponentsPlaneLabel  "_PlaneLabel_"

#define kNatronOfxImageComponentsPlaneChannelsLabel  "_ChannelsLabel_"

#define kNatronOfxImageComponentsPlaneChannel   "_Channel_"


/** @brief String to label images with 2 components. If the plug-in supports this then the host can attempt to send to the plug-in
 planes with 2 components.*/
#define kNatronOfxImageComponentXY "NatronOfxImageComponentXY"

/** @brief Whether the menu should be hierarchical/cascading, and each option contains a slash-separated path to the item.

    - Type - int X 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read only), and on the image effect host descriptor (read only)
    - Default - 0
    - Valid Values - 0 or 1

This is a property on parameters of type ::kOfxParamTypeChoice, and tells the choice whether menu should be hierarchical/cascading, and each option contains a slash-separated path to the item.

*/
#define kNatronOfxParamPropChoiceCascading "NatronOfxParamPropChoiceCascading"

/** @brief
 DEPRECATED: No longer used as of Natron 3
 int x1 property on a choice parameter descriptor (read/write) and choice parameter instance (read-only) to indicate whether
 the host can add a new choice on its own (probably via a GUI specific to this parameter).
 The plugin may then retrieve the option enum whenever a choice value is out of its initial range.

 This property primarily targets image plane choices, where the host should be able to create a new plane and add it to the menu.
 
 Valid values:
 - 0: Indicates that the parameter does not support/want that the host adds new options
 - 1: Indicates that the parameter wants to have the choice "New" so that the host can create a new option
 Default value:
 - 0
 */
#define kNatronOfxParamPropChoiceHostCanAddOptions "NatronOfxParamPropChoiceHostCanAddOptions"

/** @brief 
 DEPRECATED: No longer used as of Natron 3
 The standard parameter for setting output channels of a plugin (used by Shuffle)
 */
#define kNatronOfxParamOutputChannels "outputChannels"

/** @brief String xN property, similar to kFnOfxImageEffectPropComponentsPresent, except that this indicates for an effect
 a set of extraneous planes that were created by the user though some sort of interface which should be made available
 in output of this effect.

 Property Set - image effect instance (read only)
 Valid values - A list of one or multiple planes as described in the multi-plane definition 
 Default value - None
 */
#define kNatronOfxExtraCreatedPlanes "NatronOfxExtraCreatedPlanes"

/** @brief Indicates if the host may add a channel selector, and which components should be selected by default.

 - Type - string X 1
 - Property Set - image effect descriptor (read/write), host descriptor (read only)
 - Valid Values - This must be one of
 - kOfxImageComponentNone (channel selector is disabled)
 - kOfxImageComponentRGBA (enabled, with RGBA selected by default)
 - kOfxImageComponentRGB (enabled, with RGB selected by default)
 - kOfxImageComponentAlpha (enabled, with Alpha selected by default)

 This string indicates if the host may add a channel selector, and which components should be selected by default.

 If the property is not present the the host descriptor, or its value is not kOfxImageComponentRGBA, then the host does not have a channel selector, and the plugin may propose its own solution (e.g. a checkbox for each channel).
 
 The default for an image effect descriptor is kOfxImageComponentRGBA.
 */
#define kNatronOfxImageEffectPropChannelSelector "NatronOfxImageEffectPropChannelSelector"


/** @brief Indicates if the host may add a mask that will be handled automatically.
 
 - Type - int X 1
 - Property Set - image effect descriptor (read/write), host descriptor (read only)
 - Valid Values - 0 or 1
 - Default value: 0
 
 When set to 1, the plug-in should be able to call clipGetHandle on the clip created by the host.
 */

#define kNatronOfxImageEffectPropHostMasking "kNatronOfxImageEffectPropHostMasking"

/** @brief Indicates if the host may add a "Mix" double parameter that will dissolve
  between the source image at 0 and the full effect at 1.
 
 - Type - int X 1
 - Property Set - image effect descriptor (read/write), host descriptor (read only)
 - Valid Values - 0 or 1
 - Default value: 0
*/
#define kNatronOfxImageEffectPropHostMixing "kNatronOfxImageEffectPropHostMixing"


/** @brief Generic parameter name for a channel selector. If the plugin doesn't define these, and
    kNatronOfxImageEffectPropChannelSelector is not set by the plugin, the host may add its own channel selector.
 **/
#define kNatronOfxParamProcessR      "NatronOfxParamProcessR"
#define kNatronOfxParamProcessRLabel "R"
#define kNatronOfxParamProcessRHint  "Process red component."
#define kNatronOfxParamProcessG      "NatronOfxParamProcessG"
#define kNatronOfxParamProcessGLabel "G"
#define kNatronOfxParamProcessGHint  "Process green component."
#define kNatronOfxParamProcessB      "NatronOfxParamProcessB"
#define kNatronOfxParamProcessBLabel "B"
#define kNatronOfxParamProcessBHint  "Process blue component."
#define kNatronOfxParamProcessA      "NatronOfxParamProcessA"
#define kNatronOfxParamProcessALabel "A"
#define kNatronOfxParamProcessAHint  "Process alpha component."

/** @brief Used to define the tracker effect context. 
 In this context the effect instance will be exactly 1 track. 
 It  will define 4 buttons parameters, namely:
- kNatronParamTrackingPrevious
- kNatronParamTrackingNext
- kNatronParamTrackingBackward
- kNatronParamTrackingForward
 and 1 string parameter containing the name of the track:
 - kNatronOfxParamStringSublabelName
 
 The instance changed action on the 4 push-buttons parameters can be called on a thread different than the main-thread, allowing multiple instance changed action
 to be called on the same parameter concurrently but with a different 'time' parameter. It is up to the Host application to schedule correctly the 
 multi-threading of the tracks.
 
 In this context, typically the host would provide a general interface under which multiple instances of the plug-in in this context would co-exist. 
 The name parameter is here to identity the track.
 This could be a table in which each instance would have a separate row on its own.
 Such instances would probably have shared parameters, such as parameters of the tracking algorithm. On the other hand the instances have "specific" parameters
 that could not be shared among instances, e.g: the resulting position of a point tracking would be unique for each separate track instance.
 The host could propose in its user interface to display instance-specific parameters in each row of the table, but could display the shared parameter as a global
 parameter for all instances. To flag that a parameter is instance-specific, a new property on the parameter descriptor has been introduced, kNatronOfxImageEffectContextTracker. This property should be set to 1 for the kNatronOfxParamStringSublabelName parameter.
 */
#define kNatronOfxImageEffectContextTracker "NatronOfxImageEffectContextTracker"

/** @brief Button param that must be present on a plug-in in the kNatronOfxImageEffectContextTracker context. When the instance changed action
 is called on this parameter, it should apply the analysis on the frame preceding the current frame on the timeline.*/
#define kNatronParamTrackingPrevious "trackPrevious"

/** @brief Button param that must be present on a plug-in in the kNatronOfxImageEffectContextTracker context. When the instance changed action
 is called on this parameter, it should apply the analysis on the frame following the current frame on the timeline.*/
#define kNatronParamTrackingNext "trackNext"

/** @brief Button param that must be present on a plug-in in the kNatronOfxImageEffectContextTracker context. When the instance changed action
 is called on this parameter, it should apply the analysis on all the frames from the preceding frame until the left
 bound of the timeline..*/
#define kNatronParamTrackingBackward "trackBackward"

/** @brief Button param that must be present on a plug-in in the kNatronOfxImageEffectContextTracker context. When the instance changed action
 is called on this parameter, it should apply the analysis on all the frames from the following frame until the right
 bound of the timeline..*/
#define kNatronParamTrackingForward "trackForward"


/** @brief int  property to indicate whether a double 3D parameter is associated to a matrix 3x3
 - Type - int x 1
 - Property Set - plugin parameter descriptor (read/write) and instance (read/write only)
 - Valid values - 0, 1, 2, 3
 0: The parameter is a regular Double3D and the host should use it as any other parameter
 1: The parameter is the first row of the 3x3 matrix
 2: The parameter is the second row of the 3x3 matrix
 3: The parameter is the third row of the 3x3 matrix
 
 The 3 parameters are identified by the host by their name: the share the same name, but they end with "_rowX", with X being replaced
 by the 1-based index row of the matrix, e.g:
 
 transform_row1
 transform_row2
 transform_row3
 
 would be recognized by the host as a parameter of type Matrix3x3 with the name "transform".
 In this case the properties of the host rectangle parameter are taken from the row1 parameter, except
 for the kOfxParamPropDimensionLabel, kOfxParamPropDefault
 */
#define kNatronOfxParamPropDoubleTypeMatrix3x3 "NatronOfxParamTypeMatrix3x3"


/** @brief int  property to indicate whether a double or int 2D parameter is associated to a rectangle [(x,y),(w,h)]
 - Type - int x 1
 - Property Set - plugin parameter descriptor (read/write) and instance (read/write only)
 - Valid values - 0, 1, or 2
  0: The parameter is a regular Int2D/Double2D and the host should use its double type property
  1: The parameter is the position component of the rectangle (x,y)
  2: The parameter is the size component of the rectangle (w,h)
 
 The 2 parameters are identified by the host by their name: the share both the same name, but one end with "_position"
 and the other with "_size", e.g:
 
 rectangle_position
 rectangle_size
 
 would be recognized by the host as a parameter of type rectangle with the name "rectangle".
 In this case the properties of the host rectangle parameter are taken from the position component parameter, except
 for the kOfxParamPropMin, kOfxParamPropMax, kOfxParamPropDisplayMin, kOfxParamPropDisplayMax, kOfxParamPropDimensionLabel, 
 kOfxParamPropDefault
 */
#define kNatronOfxParamPropTypeRectangle  "NatronOfxParamPropTypeRectangle"



/** @brief int  property to indicate whether a parameter is instance-specific or not.
 - Type - int x 1
 - Property Set - plugin parameter descriptor (read/write) and instance (read/write only)
 - Default - 0
 - Valid Values - 0 or 1
 When set to 1, the parameter is specific to an effect instance of the plug-in and should have a
 unique representation for each instance. See descripton of kNatronOfxImageEffectContextTracker for more details
 on multiple instances and difference between shared and specific parameters.
 */
#define kNatronOfxParamPropIsInstanceSpecific "NatronOfxParamPropIsInstanceSpecific"

/** @brief bool property to indicate that an effect should be considered deprecated
 - Type - int x 1
 - Property Set - image effect descriptor (read/write) image effect (read only)
 - Default - 0
 - Valid Values - 0 or 1
 When set to 1, the host may disable using this plugin in new projects.
*/
#define kNatronOfxImageEffectPropDeprecated "NatronOfxImageEffectPropDeprecated"

/** @brief This extension is to allow the Host application to add extra formats 
  (which are a set of 3 values: width, height, pixel aspect ratio) to a choice parameter.
  If the host application finds the following 3 parameters:
  - kNatronParamFormatChoice
  - kNatronParamFormatSize
  - kNatronParamFormatPar
  The host will then control the choice parameter itself, including:
  - Refreshing the list of available formats in the choice parameter
  - Handling the instanceChanged action calls instead of the plug-in on the parameter kNatronParamFormatChoice by setting
  the values of kNatronParamFormatSize and kNatronParamFormatPar accordingly. The handler will not be called on the plug-in.
  
  The plug-in should still handle the case where the instanceChanged action is called for kNatronParamFormatChoice
  which would indicate that the host does not handle the parameter itself.

 */

/** @brief Choice parameter used to choose the target format*/
#define kNatronParamFormatChoice "NatronParamFormatChoice"

/** @brief Int2D parameter used to store the width,height of the format. Should be secret and evaluateOnChange=false
 The values returned are in pixel coordinates.
 */
#define kNatronParamFormatSize "NatronParamFormatSize"

/** @brief Double parameter used to store the pixel aspect ratio of the format. Should be secret and evaluateOnChange=false*/
#define kNatronParamFormatPar "NatronParamFormatPar"

/** @brief string property that uniquely identifies the project which holds the instance within the host
 - Type - string x 1
 - Property Set - image effect descriptor (read only) image effect (read only)
 - Default - ""
 - Valid Values - any
 */
#define kNatronOfxImageEffectPropProjectId "NatronOfxImageEffectPropProjectId"

/** @brief string property that uniquely identifies the group (if any) containing the instance within the current project
 If the instance os within nested groups, this should be something like "Group1.Group2" with each group level separated by a dot.
 - Type - string x 1
 - Property Set - image effect descriptor (read only) image effect (read only)
 - Default - ""
 - Valid Values - any
 */
#define kNatronOfxImageEffectPropGroupId "NatronOfxImageEffectPropGroupId"

/** @brief string property that uniquely identifies the instance within the group (if any) or within the current project
 - Type - string x 1
 - Property Set - image effect descriptor (read only) image effect (read only)
 - Default - ""
 - Valid Values - any
 */
#define kNatronOfxImageEffectPropInstanceId "NatronOfxImageEffectPropInstanceId"

/** @brief Indicates if a plug-in can make use of multiple CPU threads within a single call of the kOfxImageEffectActionRender action, for instance
 by using the multi-thread suite.
 The host should only account this as a hint to better organize its multi-threading priority queue.
 Unlike the kOfxImageEffectPluginRenderThreadSafety property, it does not inform in any way about the multi-thread safety of the plug-in.

 - Type - int X 1
 - Property Set - plugin descriptor (read/write)
 - Default - 0
 - Valid Values -
 - 0: indicating that only the thread calling the kOfxImageEffectActionRender action is the only thread expected to be doing processing
 - 1: indicating that the plug-in can launch multiple processing threads during a single call to the kOfxImageEffectActionRender action.
 The plug-in should try to use at most the multi-thread suite and especially the multiThreadNumCPUs function to help the host application 
 schedule threads over concurrent tasks.
 */
#define kNatronOfxImageEffectPluginUsesMultipleThread "NatronOfxImageEffectPluginUsesMultipleThread"


////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// Natron in-viewer interface extension /////////////////////////////////
/*
 The need for this extension was to be able to create parameters within the viewport of the host application when the user
 would be engaged in a fullscreen task and to get better support with the viewport.
 
 Several things are covered in this extension:
 
    1) In-viewer parameters that behave like normal parameters but have their interface directly next to the viewport.
    2) Toggable buttons, implemented as Boolean parameters.
    3) Buttons and toggable buttons can be assigned shortcuts
    4) Ability to add toolbars which can let the user define the "state" of the interaction (selecting, editing, etc...) Implemented using page group and boolean parameters.
    5) Ability to pop a menu during overlay actions that uses the native software menu interface (typically a right click menu)
    6) Addition of the kOfxInteractPropPenPosition and kOfxInteractPropPenViewportPosition properties for ALL interact actions (even draw, focus in/out) so the plug-in can now at any moment where the position of the cursor is
    7) A lot of overlay interacts require some sort of selection where the user can draw a rectangle to select specific items. Rather than re-create a new custom-looking selection rectangle, let the host make one for us through its own interact actions and notify us if needed.
        A new parameter with a special name can be used to let the host know we are interested in knowing the state of the selection.
 
    8) Let the plug-in a way to control the host application cursor.
    9) Add a way to show a custom dialog with fields represented as parameters. This is implemented as a property on a group parameter whether its content can be represented as a non-modal dialog or not.
    10) Plug-ins description (kOfxPropPluginDescription) and parameters hint (kOfxParamPropHint) may be written in markdown.
    11) A plug-in might want to be able to access the host undo/redo actions of the Edit menu when doing special actions from within the overlay interact actions not involving parameters.
    12) Deprecate kOfxParamPropUseHostOverlayHandle, instead add a property on the image effect descriptor indicating group of parameters which altogether control
 more complex overlays. A host can then provide default overlay interact handling.

1) In the host viewport, parameters can be arranged by widget rows controlled by the kNatronOfxImageEffectPropInViewerContextParamsOrder property.
 This index indicates the order of the parameter in the rows.
 New rows are created if the parameter also have the property kNatronOfxParamPropInViewerContextLayoutHint set to kNatronOfxParamPropInViewerContextLayoutHintAddNewLine.
 Note that the host should try to make the parameters UI concise.
 
 Several properties are a duplicate of existing properties on the parameter descriptor, because there was a need to differentiate
 the state of the interface of the parameter on the viewer and at the same time in its standard page. 
 The Host should carefully ensure that the state of the interface of a parameter in the viewer and in the page are synchronized.
 
 2) Boolean parameters now have a property kNatronOfxBooleanParamPropIsToggableButton that indicate whether it should be 
 displayed as a toggable button instead of a checkbox.

 3) PushButton parameters, Boolean Parameters with property kNatronOfxBooleanParamPropIsToggableButton, and Group parameters with the property kNatronOfxParamPropInViewerContextIsInToolbar can have shortcuts. Instead of responding to specific key strokes in the overlay interact actions, the plug-in can then just inform the host about the shortcut associated to this parameter. This enables the host to represent the shortcut in some sort of shortcut editor where the user could modify the shortcut.
 The plug-in informs to the host via the property on the image effect kNatronOfxImageEffectPropInViewerContextDefaultShortcuts the default shortcuts for a parameter.
 Each shortcut, should be specified with a symbol kNatronOfxImageEffectPropInViewerContextShortcutSymbol, as well and indicate its modifiers via
 kNatronOfxImageEffectPropInViewerContextShortcutHasControlModifier, kNatronOfxImageEffectPropInViewerContextShortcutHasShiftModifier, kNatronOfxImageEffectPropInViewerContextShortcutHasAltModifier, kNatronOfxImageEffectPropInViewerContextShortcutHasMetaModifier and kNatronOfxImageEffectPropInViewerContextShortcutHasKeypadModifier.
 All these properties should have the same dimension as kNatronOfxImageEffectPropInViewerContextDefaultShortcuts.
 
 When a shortcut is triggered, the host will not call the associated keydown action but instead will call the action kOfxActionInstanceChanged.
 
 4) It is also possible to add a toolbar to the host viewport, to indicate "states" in the plug-in, e.g: "Selecting", "Drawing", "Cloning", etc...
 A toolbar is defined simply by defining a Page parameter with property kNatronOfxParamPropInViewerContextIsInToolbar set to 1.
 To add toolbuttons, add Group parameters to the page, with the property kNatronOfxParamPropInViewerContextIsInToolbar set to 1.
 To add an action to a toolbutton, add a BooleanParam to a group with the property kNatronOfxParamPropInViewerContextIsInToolbar set to 1.
 A shortcut can be set on the group parameters which are represented as toolbuttons so that the user can cycle through the actions with the keyboard.
 
 The selected toolbutton in the toolbar will have the property kOfxParamPropGroupOpen set to 1. 
 The selected tool action in a toolbutton will have its parameter value set to true.
 To find out the active tool, retrieve the active action of the selected toolbutton.
 The host should always make sure that there is a selected toolbutton and that it has an active action.
 Similarly, the plug-in should by default ensure that one and only one toolbutton is selected and that for each toolbutton, one action is at least active.
 
 5) A menu can be popped on the viewer under the position of the cursor when requested during overlay interacts actions.
   The menu itself is described via a secret Choice parameter with the name kNatronOfxParamRightClickMenu
 which kOfxParamPropChoiceOption are the script-name of parameters which have been defined as in-viewer parameters with the kNatronOfxParamPropInViewerContextIndex set. Only Boolean parameters and  PushButton parameters can be specified in the menu, typically to represent a checkable state or that an action can be triggered.
    The menu must be constructed by the plug-in during the action it wants to use it, hence the host must support kNatronOfxParamHostPropSupportsDynamicChoices

 6) The kOfxInteractPropPenPosition and kOfxInteractPropPenViewportPosition properties have been added to all overlay interacts actions as they are sometimes needed to know the position of the cursor during the draw action, for example to draw something only if the cursor is in the window.

 7) Add a way to use the host native selection rectangle to define the selection of items. To advertise to the host that we are interested in the selection state of the rectangle, we declare a Int parameter with the name kNatronOfxParamSelectionRectangleState. The host notifies the plug-in of the selection state change via the kOfxActionInstanceChanged action.
     The value of the parameter can be 3-fold:
         0 meaning that the selection has been cleared by the user
         1 meaning that the selection is being actively edited by the user (i.e: during pen motion)
         2 meaning that the selection is finilized by the user (i.e: during pen up)
    The property kNatronOfxImageEffectSelectionRectangle will be updated prior to calling the kOfxActionInstanceChanged action for this parameter by the host, indicating the region covered by the selection rectangle. 

 8) The host application cursor can be controled by the plug-in via a secret String parameter with the name kNatronOfxParamCursorName. If this parameter is found, the host should display a cursor depending on the value of this parameter.  The value of the parameter should be the name of the cursor. Several default cursor can be made available by the host as advertised by the kNatronOfxImageEffectPropDefaultCursors property. The plug-in may also embed custom cursors as PNG files located in the Resources directory of the plug-in bundle. To specify one the image in the plug-in bundle as an icon, you may set the value of this parameter to the filename of the image without any leading path. This is up to the host to figure out the path to the plug-in bundle Resources directory.
     Whenever an event is sent to overlay interacts (PenDown, PenMotion, PenUp, KeyDown, KeyUp, KeyRepeat, GainFocus, LoseFocus, the OFX Host should set the cursor to the one of the first interact which catches the event by returning kOfxStatOK to the corresponding action. If no overlay interact returns kOfxStatOK, the cursor should be set to the default cursor.
 
 9) It may be useful to show a dialog to a user to request a little information before doing a task such as an analysis.
 Instead of embedding the parameters in the settings of an image effect, the host could display a temporary dialog with the parameters requested. If the property kNatronOfxGroupParamPropIsDialog is set to 1, then whenever the property kOfxParamPropGroupOpen is set to 1, the dialog should be displayed by the host.

 10) If the property kNatronOfxPropDescriptionIsMarkdown is set to 1 on a parameter descriptor or on an image effect descriptor, then their corresponding description (kOfxParamPropHint for a parameter descriptor and kOfxPropPluginDescription for an image effect descriptor) may be interpreted as markdown by the host.

 11) To support undo/redo, the plug-in must define a string parameter named kNatronOfxParamUndoRedoText and a boolean parameter named kNatronOfxParamUndoRedoState. The parameters should have the following properties settings:
     - kOfxParamPropEvaluateOnChange = 0
     - kOfxParamPropPersistent = 0
     - kOfxParamPropSecret = 1
    The string parameter value represents the text that should be displayed by the host in the undo/redo action of the Edit menu.
    The boolean parameter value is used to notify the plug-in about undo/Redo stack changes. If the value is set to false, the user just pressed Undo, otherwise he/she pressed Redo.
    To notify the host that a new action has been pushed, call paramSetValue on the string parameter with the name of the action that was done. The host notifies the plug-in in-turn that an action has been redone/undone by calling the  kOfxActionInstanceChanged action with kOfxPropChangeReason = kOfxChangeUserEdited.
    It is up to the plug-in to maintain the actual undo/redo stack of actions. Also note the following:
    - The first time an action is push, the host should not call kOfxActionInstanceChanged on the plug-in since the action has already been redone
    - It is up to the plug-in to merge similar undo/redo actions (e.g: imagine each action being typing a letter, the plug-in could merge them under a single one to delimit words instead of letters whenever the user wants to undo something). The first action of such a merge should always call paramSetValue but if a merge is successful, it should not call it.
 
  12) The new property kNatronOfxPropNativeOverlays is defined on the plug-in image effect descriptor and on the host descriptor. 
 It adds a way for a plug-in to let the host handle overlay interacts.
 kOfxParamPropUseHostOverlayHandle is no longer needed with this new property.
 See property description for more infos.
 
 */

/**
  string property xN that indicates for a host the supported native overlay interacts and for a plug-in the native overlay interacts requested to the host.
 
 Property set - image effect descriptor (read/write) and host descriptor (read only).

 On the host side, each string describes one type of native overlay interact. Each native overlay interact is of the following form:

 kNatronNativeOverlayType + '_' + "UniqueOverlayName" + '_'

 Followed by an undefined number of strings representing parameters. Each parameter is defined of the following form:
 
 kNatronNativeOverlayParameterHint + '_' + "RoleHint" + kNatronNativeOverlayParameterType + "ParamType"
 
 The type of a parameter is as defined in ofxParam.h such as kOfxParamTypeInteger or kOfxParamTypeDouble2D.
 Each parameter string is separated by a '_'.
 
 For example:

 "NatronNativeOverlayType_Point2D_kNatronNativeOverlayParameterHint_Position_NatronNativeOverlayParameterType_OfxParamTypeDouble2D"
 
 Or

"NatronNativeOverlayType_CornerPin2D_NatronNativeOverlayParameterHint_TopLeft_NatronNativeOverlayParameterType_OfxParamTypeDouble2D_NatronNativeOverlayParameterHint_TopRight_NatronNativeOverlayParameterType_OfxParamTypeDouble2D_NatronNativeOverlayParameterHint_BottomRIght_NatronNativeOverlayParameterType_OfxParamTypeDouble2D_NatronNativeOverlayParameterHint_BottomLeft_NatronNativeOverlayParameterType_OfxParamTypeDouble2D"

 
 On the plug-in side, each string describes which overlay type of the host to use and a number of parameter strings.
 Each parameter string is of the form:
 
 "NatronNativeOverlayParameterName" + '_' + "parameterName"

 
 For example:
 
 "NatronNativeOverlayType_Point2D_NatronNativeOverlayParameterName_myPositionParameter"

 "NatronNativeOverlayType_CornerPin2D_NatronNativeOverlayParameterName_topLeft_NatronNativeOverlayParameterName_topRight_NatronNativeOverlayParameterName_bottomRight_NatronNativeOverlayParameterName_bottomLeft"
 

 **/
#define kNatronOfxPropNativeOverlays "NatronOfxPropNativeOverlays"

/**
 String used to define native overlay interacts in the kNatronOfxPropNativeOverlays property
 **/
#define kNatronNativeOverlayType "NatronNativeOverlayType"
#define kNatronNativeOverlayParameterHint "NatronNativeOverlayParameterHint"
#define kNatronNativeOverlayParameterType "NatronNativeOverlayParameterType"
#define kNatronNativeOverlayParameterName "NatronNativeOverlayParameterName"

/**
 int property that tells if a hint or description is written in markdown instead of plain text.
 This can be applied either to the content of the kOfxPropPluginDescription property for plug-in descripion or the kOfxParamPropHint property for parameter description.
 For a host, this indicates whether markdown is supported or not
 - Type - int x 1
 - Property Set - plugin parameter descriptor (read/write) and instance (read only) or image effect descriptor (read/write) and instance (read only) or host descriptor (read only)
 - Default - 0
 - Valid Values - 1 or 0
 If the value is 1, the hint is written in markdown
 If the value is 0, the hint is written in plain-text
 */
#define kNatronOfxPropDescriptionIsMarkdown "NatronOfxPropDescriptionIsMarkdown"

/**
 int property that tells if a group parameter can be displayed as a dialog by the host when its kOfxParamPropGroupOpen
property is set to 1.
 When a dialog is requested, all parameters within this group should be represented in the dialog. The dialog itself could be titled with the label of the group parameter.
 - Type - int x 1
 - Property Set - group parameter descriptor (read/write) and instance (read only)
 - Default - 1
 - Valid Values - 1 or 0
 If the value is 1, this parameter cannot have an interface in the viewport.
 If the value is 0, this corresponds to the index on the viewport at which the host should place the parameter.
 */
#define kNatronOfxGroupParamPropIsDialog "NatronOfxGroupParamPropIsDialog"

/**
 The name of the string parameter that should be used to implement host undo/redo. See 11) of the extension description.
 */
#define kNatronOfxParamUndoRedoText "NatronOfxParamUndoRedoText"

/**
 The name of the boolean parameter that should be used to implement host undo/redo. See 11) of the extension description.
 */
#define kNatronOfxParamUndoRedoState "NatronOfxParamUndoRedoState"

/**
 The name of the choice parameter that should be used to implement host right click menus. See 5) of the extension description.
 */
#define kNatronOfxParamRightClickMenu "NatronOfxParamRightClickMenu"


/**
 The name of the int parameter that should be used to implement host selection rectangle. See 7) of the extension description.
 */
#define kNatronOfxParamSelectionRectangleState "NatronOfxParamSelectionRectangleState"

/**
 The name of the string parameter that should be used to implement host cursors. See 8) of the extension description.
 */
#define kNatronOfxParamCursorName "NatronOfxParamCursorName"

/**
 double property that defines the current selection rectangle drawn by the user on the host viewport.
 See 7) for the associated extension. This value is refreshed whenever calling the kOfxActionInstanceChanged action for the parameter kNatronOfxParamSelectionRectangleState to let the plug-in a change to correctly synchronized its selection.

 - Type - double x 4
 - Property Set - plugin parameter descriptor (read/write) and instance (read/write)
 - Default - (0,0,0,0)
 - Valid Values - (x1,y1,x2,y2) quadruplet such as x1 < x2 and y1 < y2
 */
#define kNatronOfxImageEffectSelectionRectangle "NatronOfxImageEffectSelectionRectangle"

/** 
 string property indicating for a host, which are the default cursors provided to the plug-in and for a plug-in which are the cursor that it provides a drawing for.
 - Type - string x N
 - Property Set -  host descriptor (read only)
 - Default - kNatronOfxDefaultCursor
 - Valid Values: Any of the cursor defined below by the properties kNatronOfx*Cursor
    For a plug-in, any cursor defined by the properties kNatronOfx*Cursor
 
 The special value of kNatronOfxDefaultCursor means that the host should keep the default cursor.
 The special value of kNatronOfxBlankCursor means that the host should not draw any cursor at all.
 
 Whenever an event is sent to overlay interacts (PenDown, PenMotion, PenUp, KeyDown, KeyUp, KeyRepeat, GainFocus, LoseFocus, the OFX Host should set the cursor to the one of the first interact which catches the event by returning kOfxStatOK to the corresponding action. If no overlay interact returns kOfxStatOK, the cursor should be set to the default cursor.

 */
#define kNatronOfxImageEffectPropDefaultCursors "NatronOfxImageEffectPropDefaultCursors"

// Default cursors that can be defined by a host, see http://doc.qt.io/qt-4.8/qcursor.html for an illustration to how they could look
#define kNatronOfxDefaultCursor "NatronOfxDefaultCursor"
#define kNatronOfxBlankCursor "kNatronOfxBlankCursor"
#define kNatronOfxArrowCursor "NatronOfxArrowCursor"
#define kNatronOfxUpArrowCursor "NatronOfxUpArrowCursor"
#define kNatronOfxCrossCursor "NatronOfxCrossCursor"
#define kNatronOfxIBeamCursor "NatronOfxIBeamCursor"
#define kNatronOfxWaitCursor "NatronOfxWaitCursor"
#define kNatronOfxBusyCursor "NatronOfxBusyCursor"
#define kNatronOfxForbiddenCursor "NatronOfxForbiddenCursor"
#define kNatronOfxPointingHandCursor "NatronOfxPointingHandCursor"
#define kNatronOfxWhatsThisCursor "NatronOfxWhatsThisCursor"
#define kNatronOfxSizeVerCursor "NatronOfxSizeVerCursor"
#define kNatronOfxSizeHorCursor "NatronOfxSizeHorCursor"
#define kNatronOfxSizeBDiagCursor "NatronOfxSizeBDiagCursor"
#define kNatronOfxSizeFDiagCursor "NatronOfxSizeFDiagCursor"
#define kNatronOfxSizeAllCursor "NatronOfxSizeAllCursor"
#define kNatronOfxSplitVCursor "NatronOfxSplitVCursor"
#define kNatronOfxSplitHCursor "NatronOfxSplitHCursor"
#define kNatronOfxOpenHandCursor "NatronOfxOpenHandCursor"
#define kNatronOfxClosedHandCursor "NatronOfxClosedHandCursor"

/**
 string property on the image effect descriptor that defines which parameters should have an interface on the host viewport

 - Type - string x N
 - Property Set - image effect descriptor (read/write) and instance (read only)
 - Default -

 This property should contain a list of all parameters name that should be visible in the viewer. The ordering on which they appear
 on the viewport is defined by their order in this property, with index 0 defining the top-left parameter and last index the bottom-right
 parameter.
 */
#define kNatronOfxImageEffectPropInViewerContextParamsOrder "NatronOfxParamPropInViewerContextParamsOrder"

/**
 int property that controls the host viewer parameter layout.
 - Type - int x 1
 - Property Set - plugin parameter descriptor (read/write) and instance (read/write only)
 - Default - 0
 - Valid Values - kNatronOfxParamPropInViewerContextLayoutHintNormal, kNatronOfxParamPropInViewerContextLayoutHintNormalDivider, kNatronOfxParamPropInViewerContextLayoutHintAddNewLine

 kNatronOfxParamPropInViewerContextLayoutHintNormal: the host will not create a new row in the viewer interface before creating the next parameter
 kNatronOfxParamPropInViewerContextLayoutHintNormalDivider: the host will not create a new row in the viewer interface before creating the next parameter and will add a vertical separator before this parameter and the next
 kNatronOfxParamPropInViewerContextLayoutHintAddNewLine: the host will create a new row after this parameter
 */
#define kNatronOfxParamPropInViewerContextLayoutHint "NatronOfxParamPropInViewerContextLayoutHint"

//   lay out as normal, no need line added
#define kNatronOfxParamPropInViewerContextLayoutHintNormal 0

//   put a divider after parameter and don't add a new line
#define kNatronOfxParamPropInViewerContextLayoutHintNormalDivider 1

//   have the next parameter start on a new line after this parameter
#define kNatronOfxParamPropInViewerContextLayoutHintAddNewLine 2

/** @brief Layout padding between this parameter and the next in the viewport

 - Type - int X 1
 - Property Set - plugin parameter descriptor (read/write) and instance (read only)
 - Default - 3
 - Valid Values - any positive integer value

 It tells the host how much space (in pixels) to leave between the current parameter and the next parameter in a viewport row
 Note that this is only relevant if kNatronOfxParamPropInViewerContextLayoutHint is set to kNatronOfxParamPropInViewerContextLayoutHintNormal
 */
#define kNatronOfxParamPropInViewerContextLayoutPadWidth "NatronOfxParamPropInViewerContextLayoutPadWidth"


/** @brief Label for the parameter in the viewer. This is distinct from the parameter label, because for layout efficiency we might want to display the label
 only in the page and not on the viewer.

 - Type - UTF8 C string X 1
 - Property Set - plugin parameter descriptor (read/write) and instance (read only),
 - Default -
 - Valid Values - any
 */
#define kNatronOfxParamPropInViewerContextLabel "NatronOfxParamPropInViewerContextLabel"

/** @brief Flags whether the viewer interface of a parameter should be exposed to a user

 - Type - int x 1
 - Property Set - plugin parameter descriptor (read/write) and instance (read/write)
 - Default - 0
 - Valid Values - 0 or 1

 If secret, a parameter is not exposed to a user in the viewer interface, but should otherwise behave as a normal parameter.
 */
#define kNatronOfxParamPropInViewerContextSecret "NatronOfxParamPropInViewerContextSecret"


/** @brief Flags whether a Boolean parameter should be represented as a toggable button instead of a checkbox.
 - Type - int x 1
 - Property Set - plugin boolean parameter descriptor (read/write) and instance (read/write)
 - Default - 0
 - Valid Values - 0 or 1
 If 0, the boolean parameter should be a checkbox. If 1, it should be a toggable button.
 */
#define kNatronOfxBooleanParamPropIsToggableButton "NatronOfxBooleanParamPropIsToggableButton"


/** @brief Valid for Page, Group and Boolean parameters:
    For a page , flags whether a Page parameter should be represented as a tool bar inside the viewer
    For a group , flags whether a Group parameter should be represented as a tool button inside a toolbar of the viewer
    For a boolean param , flags whether a Boolean parameter should be represented as a tool button action inside a toolbar of the viewer
 - Type - int x 1
 - Property Set - plugin parameter descriptor (read/write) and instance (read/write)
 - Default - 0
 - Valid Values - 0 or 1
 */
#define kNatronOfxParamPropInViewerContextIsInToolbar "NatronOfxPageParamPropInViewerContextIsToolbar"


/** @brief Valid for Group parameters with the property kNatronOfxParamPropInViewerContextIsInToolbar set to 1, Boolean parameters with the property kNatronOfxBooleanParamPropIsToggableButton set to 1 and PushButton parameters:
 Determines whether a parameter may be assigned a shortcut by the host or not.
 - Type - int x 1
 - Property Set - plugin parameter descriptor (read/write) and instance (read/write)
 - Default - 0
 - Valid Values - 0 or 1
  If set to 1 then the host could display this parameter in some sort of shortcut editor where the user may modify it.
 */
#define kNatronOfxParamPropInViewerContextCanHaveShortcut "NatronOfxParamPropInViewerContextCanHaveShortcut"


/** @brief Property defining the default shortcuts for parameters that have the property kNatronOfxParamPropInViewerContextCanHaveShortcut set to 1. This property is a list
 of the parameter names that have a default shortcut assigned. Parameter that do not appear here but have
 the property kNatronOfxParamPropInViewerContextCanHaveShortcut set to 1 are assumed to have no default shortcut
 assigned, but the user could modify it via the host appropriate interface.

 Each parameter name should be accompanied by the following properties of the same dimension:
 - kNatronOfxParamPropInViewerContextShortcutSymbol
 - kNatronOfxParamPropInViewerContextShortcutHasControlModifier
 - kNatronOfxParamPropInViewerContextShortcutHasShiftModifier
 - kNatronOfxParamPropInViewerContextShortcutHasAltModifier
 - kNatronOfxParamPropInViewerContextShortcutHasMetaModifier
 - kNatronOfxParamPropInViewerContextShortcutHasKeypadModifier

 - Type - string x N
 - Property Set - image effect descriptor (read/write) and instance (read/write)
 - Default -
 - Valid Values -
 */
#define kNatronOfxImageEffectPropInViewerContextDefaultShortcuts "NatronOfxImageEffectPropInViewerContextDefaultShortcuts"

/** @brief Property indicating the symbol of a shortcut as defined in ofxKeySyms.h for a parameter with a shortcut. This should not include any modifier (that is kOfxKey_Control_L, kOfxKey_Control_R, kOfxKey_Shift_L, kOfxKey_Shift_R, kOfxKey_Alt_L, kOfxKey_Alt_R, kOfxKey_Meta_L, kOfxKey_Meta_R).
 The shortcut is associated to kNatronOfxImageEffectPropInViewerContextDefaultShortcuts and should thus have the same dimension.

 - Type - int x N
 - Property Set - image effect descriptor (read/write) and instance (read/write)
 - Default - 0
 - Valid Values - Any symbol defined in ofxKeySyms.h
 */
#define kNatronOfxImageEffectPropInViewerContextShortcutSymbol "NatronOfxImageEffectPropInViewerContextShortcutSymbol"

/** @brief Property indicating whether the control modifier (kOfxKey_Control_L or kOfxKey_Control_R) must be held for a parameter shortcut to trigger.
 The shortcut is associated to kNatronOfxImageEffectPropInViewerContextDefaultShortcuts and should thus have the same dimension.

 - Type - int x N
 - Property Set - image effect descriptor (read/write) and instance (read/write)
 - Default - 0
 - Valid Values - 0 or 1
 */
#define kNatronOfxImageEffectPropInViewerContextShortcutHasControlModifier "NatronOfxImageEffectPropInViewerContextShortcutHasControlModifier"


/** @brief Property indicating whether the shit modifier (kOfxKey_Shift_L or kOfxKey_Shift_R) must be held for a parameter shortcut to trigger.
 The shortcut is associated to kNatronOfxImageEffectPropInViewerContextDefaultShortcuts and should thus have the same dimension.

 - Type - int x N
 - Property Set - image effect descriptor (read/write) and instance (read/write)
 - Default - 0
 - Valid Values - 0 or 1
 */
#define kNatronOfxImageEffectPropInViewerContextShortcutHasShiftModifier "NatronOfxImageEffectPropInViewerContextShortcutHasShiftModifier"


/** @brief Property indicating whether the control modifier (kOfxKey_Alt_L orkOfxKey_Alt_R) must be held for a parameter shortcut to trigger.
 The shortcut is associated to kNatronOfxImageEffectPropInViewerContextDefaultShortcuts and should thus have the same dimension.

 - Type - int x N
 - Property Set - image effect descriptor (read/write) and instance (read/write)
 - Default - 0
 - Valid Values - 0 or 1
 */
#define kNatronOfxImageEffectPropInViewerContextShortcutHasAltModifier "NatronOfxImageEffectPropInViewerContextShortcutHasAltModifier"


/** @brief Property indicating whether the control modifier (kOfxKey_Meta_L or kOfxKey_Meta_R) must be held for a parameter shortcut to trigger.
 The shortcut is associated to kNatronOfxImageEffectPropInViewerContextDefaultShortcuts and should thus have the same dimension.

 - Type - int x N
 - Property Set - image effect descriptor (read/write) and instance (read/write)
 - Default - 0
 - Valid Values - 0 or 1
 */
#define kNatronOfxImageEffectPropInViewerContextShortcutHasMetaModifier "NatronOfxImageEffectPropInViewerContextShortcutHasMetaModifier"


/** @brief Property indicating whether the key should be pressed on the keypad for a parameter shortcut to trigger.
 The shortcut is associated to kNatronOfxImageEffectPropInViewerContextDefaultShortcuts and should thus have the same dimension.

 - Type - int x N
 - Property Set - image effect descriptor (read/write) and instance (read/write)
 - Default - 0
 - Valid Values - 0 or 1
 */
#define kNatronOfxImageEffectPropInViewerContextShortcutHasKeypadModifier "NatronOfxImageEffectPropInViewerContextShortcutHasKeypadModifier"


/** @brief A pointer property used to hold OpenGL context-specific data, such as texture or program IDs.

   - Type - pointer X 1
   - Property Set - inArgs property set of the following actions...
      - ::kOfxImageEffectActionRender
      - ::kOfxImageEffectActionBeginSequenceRender
      - ::kOfxImageEffectActionEndSequenceRender
      - ::kOfxActionOpenGLContextDetached
     - outArgs property set of the following actions...
      - ::kOfxActionOpenGLContextAttached
   - Default - NULL

This pointer is a handle to plugin-owned data holding OpenGL context-specific data
(e.g. texture or program IDs). This is returned by the plugin when the context is first
attached, and then passed as inArgs to each render call in that context. This permits running
several parallel renders on different OpenGL contexts, but it violates the OpenFX spec, which
says:

 "A host cannot call ::kOfxActionOpenGLContextAttached on the same instance
 without an intervening ::kOfxActionOpenGLContextDetached. A host can have a
 plugin swap OpenGL contexts by issuing a attach/detach for the first context
 then another attach for the next context."

 If the plugin returns NULL as outArgs of kOfxActionOpenGLContextDetached, it is supposed
 to follow the OpenFX spec, and will have to be detached/attached when switching the render context.

 */
#define kNatronOfxImageEffectPropOpenGLContextData "NatronOfxImageEffectPropOpenGLContextData"

/** @brief A int property that tells whether an interact wants colour picking information from the host.

 When set to 1, the picked colour is passed as an additional kNatronOfxPropPickerColour property
 on the inArgs of every interact action, and the draw action is called every time the picked colour
 changes.
 This property must be set to 1 on the interact descriptor if the descriptor may use colour picking.
 Then, it can be set to 0 or 1 on the interact instance in order to enable colour picking and
 to call the draw action each time the picked colour changes.
 This can also be set on the interact of parameter interacts (kOfxParamPropInteractV1)
 and on the interact for the background of parametric parameters
 (kOfxParamPropParametricInteractBackground).

 - Type - int x1
 - Property Set - interact descriptor and instance (read/write)
 - Value values - 0 or 1
 - Default value - 0
 */
#define kNatronOfxInteractColourPicking "NatronOfxInteractColourPicking"

/** @brief Double x4 property indicating the RGBA colour of the image visualized in the host
  viewport under the mouse. This is passed in the inargs of the draw action of parametric
  parameters interact (i.e: with property kOfxParamPropParametricInteractBackground).
  The special value -1,-1,-1,-1 means no color.
 - Type - Double x4
 - Property Set - inArgs property set of the action kOfxInteractActionDraw called on the parametric parameter 
 - Default - -1,-1,-1,-1
 */
#define kNatronOfxPropPickerColour "NatronOfxPropPickerColour"

/** @brief Property to indicate the whether the curves in the parametric parametric
 are considered to be periodic or not. If periodic the first and last control point
 should be considered equal in the curve and cannot be moved from their parametric position.
 The host should always evaluate the curve in-between the first and last keyframes.

 - Type - int X 1
 - Property Set - parametric parameter descriptor (read/write only), and instance (read only)
 - Default Value - (0)
 - Valid Values - 1 or 0
 If 1 the curves are considered to be periodic.
 */
#define kNatronOfxParamPropParametricIsPeriodic "OfxParamPropParametricIsPeriodic"

/** Used to indicate the format size (in pixel coordinates) of the stream that should be displayed in output of this plug-in. It is not necessarily the same as the region of definition.

 Example: User has a project size 2K. The user draws a rectangle which does not cover the full project size, the region of definition is thus smaller than the project size. Think now of a Reformat plug-in appended after the rectangle to perform a scale down to 1K: the region of definition in output of the plug-in should be half the region of the original rectangle and the format should become 1K. Without this property, the Reformat plug-in would be forced to have a region of definition of 1K and render many un-needed pixels.

 - Type - int X4
 - Property Set - a read/write out argument property of the kOfxImageEffectActionGetClipPreferences and a read only property of a clip instance
 - Default - Host by default sets the format to the "Source" clip format. If the effect is in the General context, the format is initialized to the first non-optional input clip.
 If there are no input clips, the format is initialized to the project extent/size.

 The order of the values is x1, y1, x2, y2 where x1 <= x2 and y1 <= y2.

 **/
#define kOfxImageClipPropFormat "OfxImageClipPropFormat"



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Extensions for image effects that can return a distortion function rather
// than an image. A distortion function gost from a 2D distorted position in
// canonical coordinates in the output distorted image to the undistorted
// position in canonical coordinates in the source image.

// This implemented the standard change originally proposed at http://openeffects.org/standard_changes/243

/** @brief Property to indicate that a plugin, clip or host can handle distortions

 - Type - int X 1
 - Property Set - host descriptor (read only), plugin instance (read/write), clip descriptor (read/write)
 - Default - 0
 - Valid Values - This must be one of
 - 0 if the host or plugin cannot make use of the kOfxImageEffectActionGetInverseDistortion
 - 1 if the host or plugin can use the kOfxImageEffectActionGetInverseDistortion, or the clip
 can return images with a distortion function attached (@see kOfxPropInverseDistortionFunction)

 This is a property on the descriptor.
 */
#define kOfxImageEffectPropCanDistort "OfxImageEffectPropCanDistort"

/** @brief Action called in place of a render to recover a distortion from an effect.

 Some effects do a simple matrix transform or distortion on their images, the cleverness in the effect
 is how that transform is arrived at. For example a stabilisation effect which analyses
 it's input clip in user interface and writes key frames to a set of params which represent
 a transform. The render action of such an effect is just a transform by a matrix.

 Often, such effects are chained together, and you are incurring both an extra compute cost
 and a degradation in the quality of the output image by having the effects in question
 perform multiple image transforms and filtering actions.

 In such a situation it would be much better if you could recover the transforms from each
 of the effects and compose them together and transform the image once. This improves
 both performance and quality. This action allows you to do such a thing and serves in
 place of a standard render call.

 To be able to successfully reproduce the same result as the render action with this
 action, any effects that implement it must adhere to several conditions...
 The effect does not need the following called to determine the correct transform,
 - The Get Region of Definition Action
 - The Get Regions Of Interest Action
 - The Get Frames Needed Action
 - The Is Identity Action
 - The Render Action
 - The Begin Sequence Render Action
 - The End Sequence Render Action
 The effect only needs a single image frome the clip named in the out args of this action


 For maximum flexibility, a plug-in must be allowed to fetch images inside this action, so as to be
 able to perform on the fly analysis to calculate the required transform. Ideally this should be
 discouraged in favour of simply returning pre-analysed values.

 Note that the render action can still be called if a host so chooses, this action does not
 completely replace the render action of such an effect.

 When the plug-in needs to sample a source clip, if the plug-in has flagged the source clip kOfxImageEffectCanDistort to 1
 then an image fetched from this clip *may* receive a pointer to such function.
 In this case, this is a pointer of a function of the host that will call in turn the distortion function of all concatenated source effects
 and return the final undistorted position. 
 Note that even if this effect supports returning a distortion,
 the concatenation will not include the distortion provided by this effect, it will have to be applied on top of the concatenation function.

 This action has the following properties on its arguments....
 inargs -
 - kOfxPropTime (double x1)- the time at which to test for identity
 - kOfxImageEffectPropFieldToRender (string x1)- the field being transformed
 - kOfxImageEffectPropRenderScale (double x2)- the scale factor being applied to the images being transformed
 - kFnOfxImageEffectPropView (int x1) (only if view aware)

 outargs -
 - kOfxPropName (string x1) - this to the name of the input clip that would be transformed by the effect during render
 defaults to "Source". The input clip image will be fetched at the time and view given in the inArgs.

 - kOfxPropMatrix3x3 - (double x9) If the output distortion can be represented as a 3x3 matrix, then prefer this as the
host may optimize the concatenation of 3x3 matrix into a single matrix. This is in canonical coordinates space,
 going from the source image to the destination, defaults to the identity matrix.
 
 If however the distortion cannot be a simple 3x3 matrix, then the following properties should be set in the outArgs:
 
 -  kOfxPropInverseDistortionFunction (pointer x1): A pointer to the distortion function itself that the host should call. 
 The function has the signature described below (OfxDistortionFunctionV1)

 -  kOfxPropInverseDistortionFunctionData (pointer x1) : Pointer to datas owned by the plug-in that should be passed back as customData
 to the OfxDistortionFunctionV1 function. This can contain parameter values or STMaps or anything custom to the plug-in.
 These datas will be freed by the kOfxPropInverseDistortionFunctionFreeData speficied below once the host does not need them anymore.
 Note that the host may cache these datas away and it is important to hint the host about the size these datas holds in the host cache.

 - kOfxPropInverseDistortionFunctionDataSize (int x1): An estimated size in bytes of the memory held by the datas pointed to by kOfxPropInverseDistortionFunctionData.
 The host may cache these datas away and it is important to hint the host about the size these datas holds in the host cache.
 This should not overflow an integer size.

 -  kOfxPropInverseDistortionDataFreeFunction (pointer x1): Pointer to a function called by the host when the concatenation is done to free
the datas pointed to by kOfxPropInverseDistortionFunctionData
 


 @returns
 - ::kOfxStatDefault - don't attempt to any of the out args, but render the image as per normal,
 - ::kOfxStatOK - the transfrom and clip name were set and can be used to modify the named image appropriately,

 */

#define kOfxImageEffectActionGetInverseDistortion "OfxImageEffectActionGetInverseDistortion"

/*
 @brief Property that holds a pointer to a function of type OfxDistortionFunctionV1
 - Type - Pointer x1
 - Property Set - outArgs of kOfxImageEffectActionGetInverseDistortion and image instance (read only)
 - Default - 0
 
 The plug-in returns a pointer of such as function in the kOfxImageEffectActionGetInverseDistortion action.
 */
#define kOfxPropInverseDistortionFunction "OfxPropInverseDistortionFunction"

/*
 @brief Property that holds a pointer to data used in the function pointed to by kOfxPropInverseDistortionFunction.
 - Type - Pointer x1
 - Property Set - outArgs of kOfxImageEffectActionGetInverseDistortion and image instance (read only)
 - Default - 0
 
 When calling kOfxImageEffectActionGetInverseDistortion, the plug-in returns these data that should be used by the kOfxPropInverseDistortionFunction function
 passed in the outArgs of the action.
 
 When the property is on the image instance, these are host data the should be passed to the function pointed to by the kOfxPropInverseDistortionFunction property
 on the same image.
 */
#define kOfxPropInverseDistortionFunctionData "OfxPropInverseDistortionFunctionData"


/*
@brief Property that hints the host about the total size of the datas pointed to by kOfxPropInverseDistortionFunctionData. If these datas contain buffers or images
 they should be summed up.
 The host may cache these datas away and it is important to hint the host about the size these datas holds in the host cache.
 This should not overflow an integer size.
 
 - Type - Int x1
 - Property Set - outArgs of kOfxImageEffectActionGetInverseDistortion action
 - Default - 0

 */
#define kOfxPropInverseDistortionFunctionDataSize "OfxPropInverseDistortionFunctionDataSize"

/*
@brief Property that holds a pointer to a function that can be called to free the data pointed to by kOfxPropInverseDistortionFunctionData that were passed back
 by a plug-in in the kOfxImageEffectActionGetInverseDistortion action
 
 - Type - Pointer x1
 - Property Set - outArgs of the kOfxImageEffectActionGetInverseDistortion action
 - Default - 0
 */
#define kOfxPropInverseDistortionDataFreeFunction "OfxPropInverseDistortionDataFreeFunction"

/**
 @brief Prototype of the distortion passed to the kOfxPropInverseDistortionFunction property. It takes in input the distorted position and should output the
 undistorted position, both in canonical coordinates. Optionnally the Jacobian may be output.
 @param customData These are custom datas that were returned by the plug-in in the kOfxImageEffectActionGetInverseDistortion action in kOfxPropInverseDistortionFunctionData
 @param wantsJacobian True if the caller would like the function to return a Jacobian, if possible. Note that the function may not return a Jacobian
 even if it was asked for, in which case the caller will have to compute it using for example finite differences.
 @param gotJaboian True if the jacobian was computed and set in output. If wantsJacobian is set to false, this parameter may be NULL.
 @param jacobian The 4 partial derivatives of the function: [dFx/dx, dFx/dy, dFy/dx, dFy/dy].
 If wantsJacobian is set to false, this parameter may be NULL.
 **/
typedef void (*OfxInverseDistortionFunctionV1)(const void* customData,
                                               double distortedX, double distortedY,
                                               bool wantsJacobian,
                                               double *undistortedX, double *undistortedY,
                                               bool* gotJabobian,
                                               double jacobian[4]);

/*
 * @brief Prototype of the function passed to the kOfxPropInverseDistortionDataFreeFunction property. It takes in input the kOfxPropInverseDistortionFunctionData data that
 were returned by the kOfxImageEffectActionGetInverseDistortion action and should free them.
 */
typedef void (*OfxInverseDistortionDataFreeFunctionV1)(void* customData);

/** @brief Property that represents a 2D matrix

 - Type - double X 9
 - Property Set - out args of kOfxImageEffectActionGetInverseDistortion, or on an image instance (read only)
 - Default - the identity matrix
 - Valid Values - any matrix value

 The 9 values of this property represent a 2D 3 by 3 matrix. The matrix is in row/column format.
 **/

#define kOfxPropMatrix3x3 "OfxPropMatrix3x3"

/** @brief The pixel ratio of the screen. On Retina screens, this would typically return 2.
 This is needed because typically when displayed on a Retina screen, the OpenGL framebuffer drawn to
 would be bigger, hence all functions taking pixels need to be scaled accordingly. Namely:
 - glPointSize
 - glLineWidth
 - glScissor,
 - etc...

 - Type - double X 1
 - Property Set - read only in argument to the ::kOfxInteractActionPenMotion, ::kOfxInteractActionPenDown and ::kOfxInteractActionPenUp actions
 - Default value - 1.
 */
#define kOfxInteractPropScreenPixelRatio "OfxInteractPropScreenPixelRatio"

#endif // #ifndef _ofxNatron_h_
