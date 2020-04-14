#ifndef _ofxSonyVegas_h_
#define _ofxSonyVegas_h_
/******************************************************************************
 *  Copyright (C) 2010 Sony Creative Software Inc. All rights reserved. 
 *
 *
 *  Description:
 *      Sony-added messages to the OFX standard
 ******************************************************************************/


#define kOfxSonyVegasHostName                    "com.sonycreativesoftware.vegas"
#define kOfxSonyVegasMovieStudioHostName         "com.sonycreativesoftware.vegas.moviestudio.hd"
#define kOfxSonyVegasMovieStudioPlatinumHostName "com.sonycreativesoftware.vegas.moviestudio.pe"


/** @brief string property on a Vegas host for the appropriate app data directory (for logs and caches) */
#define kOfxPropVegasHostAppDataDirectory "OfxPropVegasHostAppDataDirectory"

/** @brief Double value on the OfxParamPropDoubleType property of a double 2d parameter (Vegas extension) */
#define kOfxParamDoubleTypePolar "OfxParamDoubleTypePolar"

/** @brief Double value on the OfxParamPropDoubleType property of a double 2d parameter (Vegas extension) */
#define kOfxParamDoubleTypeChrominance "OfxParamDoubleTypeChrominance"

/** @brief Double value on the OfxParamPropDoubleType property of a double 2d parameter (Vegas extension) */
#define kOfxParamPropColorWheelLevel "OfxParamPropColorWheelLevel"

/** @brief String property of a RGB or RGBA parameter for the default UI colorspace (Vegas extension) */
#define kOfxParamColorDefaultColorspace "OfxParamColorDefaultColorspace"

/** @brief String values on the kOfxParamColorDefaultColorspace property of a RGB or RGBA parameter (Vegas extension) */
#define kOfxParamColorColorspaceRGB "OfxParamColorColorspaceRGB"
#define kOfxParamColorColorspaceHSL "OfxParamColorColorspaceHSL"
#define kOfxParamColorColorspaceHSV "OfxParamColorColorspaceHSV"
#define kOfxParamColorColorspaceLab "OfxParamColorColorspaceLab"

/** @brief Integer property indicates the default open/closed state of a UI item defined 
  * (Vegas extension, only double 3d, 2d, int 3d, 2d, rgb, rgba)
  */
#define kOfxParamPropParameterExpanded "OfxParamPropParameterExpanded"

/** @brief String property on plug-ins for preset thumb hinting */
#define kOfxProbPluginVegasPresetThumbnail "OfxProbPluginVegasPresetThumbnail"

#define kOfxProbPluginVegasPresetThumbnailDefault        "OfxProbPluginVegasPresetThumbnailDefault"
#define kOfxProbPluginVegasPresetThumbnailSolidImage     "OfxProbPluginVegasPresetThumbnailSolidImage"
#define kOfxProbPluginVegasPresetThumbnailImageWithAlpha "OfxProbPluginVegasPresetThumbnailImageWithAlpha"


/** @brief Name of the vegas progress suite (extended from the ofx 1.0 progress suite to support
           localization) 
*/
#define kOfxVegasProgressSuite "OfxVegasProgressSuite"

/** @brief Name of the vegas stereoscopic image effect suite (used to get a clip image from the
           other view (otherwise getClipImage by default returns the current image)
*/
#define kOfxVegasStereoscopicImageEffectSuite "OfxVegasStereoscopicImageEffectSuite"



/** @brief String property (array of length 1) indicates the filename of the plug-in help file 
        defaults to the "pluginname.chm" in the resource directory of the plug-in
*/
#define kOfxImageEffectPropHelpFile "OfxImageEffectPropHelpFile"

/** @brief Integer property (array of length 1) indicates the context id to use in the help file */
#define kOfxImageEffectPropHelpContextID "OfxImageEffectPropHelpContextID"

/** @brief Action to plug-in to invoke help
      Arguments
        - handle  - handle to the image effect
        - inArgs  - is set to NULL
        - outArgs - is set to NULL
      Returns
        kOfxStatOK           - the plug-in has handled the help do nothing
        kOfxStatReplyDefault - plug-in has done nothing; check the help file and context ID
*/
#define kOfxImageEffectActionInvokeHelp  "OfxImageEffectActionInvokeHelp"

/** @brief Action to plug-in to invoke about dialog
      Arguments
        - handle  - handle to the image effect
        - inArgs  - is set to NULL
        - outArgs - is set to NULL
      Returns
        kOfxStatOK           - the plug-in has handled the about do nothing
        kOfxStatReplyDefault - plug-in has done nothing; vegas will show default about dialog
*/
#define kOfxImageEffectActionInvokeAbout  "OfxImageEffectActionInvokeAbout"


/** @brief String property (array of any length) indicates the guid that a plug-in can uplift from */
#define kOfxImageEffectPropVegasUpliftGUID "OfxImageEffectPropVegasUpliftGUID"

/** @brief Action to plug-in to uplift the keyframe data 
      Arguments
        - handle - handle to the image effect
        - inArgs - has the following properties
              - kOfxImageEffectPropVegasUpliftGUID    - the guid of the data being uplifted
              - kOfxPropVegasUpliftKeyframeData       - pointer array of all the keyframe data
              - kOfxPropVegasUpliftKeyframeDataLength - integer array of all the keyframe data lengths
              - kOfxPropVegasUpliftKeyframeTime       - double array of all the keyframe times
        - outArgs - is set to NULL
*/
#define kOfxImageEffectActionVegasKeyframeUplift  "OfxImageEffectActionVegasKeyframeUplift"

/** @brief Pointer property (array of any length) to the original keyframe data plug-in can uplift from */
#define kOfxPropVegasUpliftKeyframeData "OfxPropVegasUpliftKeyframeData"

/** @brief Integer property (array of any length) to the original keyframe data length plug-in can uplift from */
#define kOfxPropVegasUpliftKeyframeDataLength "OfxPropVegasUpliftKeyframeDataLength"

/** @brief Double property (array of any length) to the keyframe time for the keyframe data plug-in can uplift from */
#define kOfxPropVegasUpliftKeyframeTime "OfxPropVegasUpliftKeyframeTime"

/** @brief String property (array of any length) to the original keyframe interpolation type */
#define kOfxPropVegasUpliftKeyframeInterpolation "OfxPropVegasUpliftKeyframeInterpolation"

/** @brief Pointer property (array of length 1) to the original plug-in data */
#define kOfxPropVegasUpliftData "OfxPropVegasUpliftData"

/** @brief Integer property (array of length 1) to the original plug-in data length */
#define kOfxPropVegasUpliftDataLength "OfxPropVegasUpliftDataLength"



/** @brief String used to label unsigned 8 bit integer samples in BGR order */
#define kOfxBitDepthByteBGR  "OfxBitDepthByteBGR"

/** @brief String used to label unsigned 16 bit integer samples in BGR order */
#define kOfxBitDepthShortBGR "OfxBitDepthShortBGR"

/** @brief String used to label signed 32 bit floating point samples in BGR order */
#define kOfxBitDepthFloatBGR "OfxBitDepthFloatBGR"



/** @brief String to label images with RGBA pixel order */
#define kOfxImagePixelOrderRGBA "OfxImagePixelOrderRGBA"

/** @brief String to label images with BGRA pixel order */
#define kOfxImagePixelOrderBGRA "OfxImagePixelOrderBGRA"


/** @brief Indicates the components pixel order in an image,

   - Type - string x 1
   - Property Set - clip (read only)
   - Valid Values - This must be one of
     - kOfxImagePixelOrderRGBA
     - kOfxImagePixelOrderBGRA

*/
#define kOfxImagePropPixelOrder "OfxImageEffectPropPixelOrder"


/** @brief Indicates the number of total views to render

   - Type - int x 1
   - Property Set - Render In Args (read only)
   - Valid Values - 1 (standard), 2 (stereoscopic processing)

*/
#define kOfxImageEffectPropViewsToRender "OfxImageEffectPropViewsToRender"

/** @brief Indicates the number of the view to render

   - Type - int x 1
   - Property Set - Render In Args (read only)
   - Valid Values - 0 (standard), 0 (left stereoscopic processing), 1 (right stereoscopic processing)

*/
#define kOfxImageEffectPropRenderView "OfxImageEffectPropRenderView"





/** @brief Indicates the quality level of the render

   - Type - string x 1
   - Property Set - Render In Args (read only)
   - Valid Values - see below... (VeryBad, Bad, Good, Great)

*/
#define kOfxImageEffectPropRenderQuality "OfxImageEffectPropRenderQuality"

#define kOfxImageEffectPropRenderQualityDraft   "OfxImageEffectPropRenderQualityDraft"
#define kOfxImageEffectPropRenderQualityPreview "OfxImageEffectPropRenderQualityPreview"
#define kOfxImageEffectPropRenderQualityGood    "OfxImageEffectPropRenderQualityGood"
#define kOfxImageEffectPropRenderQualityBest    "OfxImageEffectPropRenderQualityBest"


/** @brief Indicates the context a plug-in instance exists in vegas

   - Type - string x 1
   - Property Set - Effect Instance (read only)
   - Valid Values - see below... (Media, Track, Event, Project, Generator, ...)

*/
#define kOfxImageEffectPropVegasContext "OfxImageEffectPropVegasContext"

#define kOfxImageEffectPropVegasContextUnknown      "OfxImageEffectPropVegasContextUnknown"
#define kOfxImageEffectPropVegasContextMedia        "OfxImageEffectPropVegasContextMedia"
#define kOfxImageEffectPropVegasContextTrack        "OfxImageEffectPropVegasContextTrack"        
#define kOfxImageEffectPropVegasContextEvent        "OfxImageEffectPropVegasContextEvent"
#define kOfxImageEffectPropVegasContextEventFadeIn  "OfxImageEffectPropVegasContextEventFadeIn"
#define kOfxImageEffectPropVegasContextEventFadeOut "OfxImageEffectPropVegasContextEventFadeOut"
#define kOfxImageEffectPropVegasContextProject      "OfxImageEffectPropVegasContextProject"
#define kOfxImageEffectPropVegasContextGenerator    "OfxImageEffectPropVegasContextGenerator"


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#define kOfxVegasKeyframeSuite "OfxVegasKeyframeSuite"

#define kOfxVegasKeyframeInterpolationUnknown      "OfxVegasKeyframeInterpolationUnknown"
#define kOfxVegasKeyframeInterpolationLinear       "OfxVegasKeyframeInterpolationLinear"
#define kOfxVegasKeyframeInterpolationFast         "OfxVegasKeyframeInterpolationFast"
#define kOfxVegasKeyframeInterpolationSlow         "OfxVegasKeyframeInterpolationSlow"
#define kOfxVegasKeyframeInterpolationSmooth       "OfxVegasKeyframeInterpolationSmooth"
#define kOfxVegasKeyframeInterpolationSharp        "OfxVegasKeyframeInterpolationSharp"
#define kOfxVegasKeyframeInterpolationHold         "OfxVegasKeyframeInterpolationHold"
#define kOfxVegasKeyframeInterpolationManual       "OfxVegasKeyframeInterpolationManual"
#define kOfxVegasKeyframeInterpolationSplit        "OfxVegasKeyframeInterpolationSplit"

// in calling the paramGetKeySlopes and paramSetKeySlopes order of parameters is...
//  if type Manual keyframe is set on a double...
//    double m1;
//    keyframeSuite->paramGetSlopes(doubleParam, keyframetime, &m1);
//    keyframeSuite->paramSetSlopes(doubleParam, keyframetime, m1);
//  if type Split is set on a double param...
//    double m_in, m_out;
//    keyframeSuite->paramGetSlopes(doubleParam, keyframetime, &m_in, &m_out);
//    keyframeSuite->paramSetSlopes(doubleParam, keyframetime, m_in, m_out);
//  if type Manual keyframe is set on a double2d...
//    double mx, my;
//    keyframeSuite->paramGetSlopes(double2dParam, keyframetime, &mx, &my);
//    keyframeSuite->paramSetSlopes(double2dParam, keyframetime, mx, my);
//  if type Split is set on a double2d param...
//    double mx_in, mx_out, my_in, my_out;
//    keyframeSuite->paramGetSlopes(double2dParam, keyframetime, &mx_in, &mx_out, &my_in, &my_out);
//    keyframeSuite->paramSetSlopes(double2dParam, keyframetime, mx_in, mx_out, my_in, my_out);
//  if type Manual keyframe is set on a double3d...
//    double mx, my, mz;
//    keyframeSuite->paramGetSlopes(double3dParam, keyframetime, &mx, &my, &mz);
//    keyframeSuite->paramSetSlopes(double3dParam, keyframetime, mx, my, mz);
//  if type Split is set on a double3d param...
//    double mx_in, mx_out, my_in, my_out, mz_in, mz_out;
//    keyframeSuite->paramGetSlopes(double3dParam, keyframetime, &mx_in, &mx_out, &my_in, &my_out, &mz_in, &mz_out);
//    keyframeSuite->paramSetSlopes(double3dParam, keyframetime, mx_in, mx_out, my_in, my_out, mz_in, mz_out);
//    

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#define kOfxHWndInteractSuite "OfxHWndInteractSuite"

// HWnd interact functions

/** @brief Pointer property (array of length 1) to the HWnd interact function */
#define kOfxImageEffectPluginPropHWndInteractV1 "OfxImageEffectPluginPropHWndInteractV1"

/** @brief Pointer property (array of length 1) to the HWnd parent window */
#define kOfxHWndInteractPropParent "OfxHWndInteractPropParent"

/** @brief Int property (array of length 2) to the minimum size (width, height) */
#define kOfxHWndInteractPropMinSize "OfxHWndInteractPropMinSize"

/** @brief Int property (array of length 2) to the preferred size (width, height) */
#define kOfxHWndInteractPropPreferredSize "OfxHWndInteractPropPrefferedSize"

/** @brief Action to plug-in to create an interaction hwnd
      Arguments
        - handle - handle to the image effect
        - inArgs - has the following properties
              - kOfxHWndInteractPropParent        - the HWND parent to create a window in
        - outArgs - has the following properties
              - kOfxHWndInteractPropMinSize       - the minimum size of the window
              - kOfxHWndInteractPropPreferredSize - the preferred size of the window
*/
#define kOfxHWndInteractActionCreateWindow  "OfxHWndInteractActionCreateWindow"

/** @brief Int property (array of length 4) to the rect to move to (left, top, width, height) */
#define kOfxHWndInteractPropLocation "OfxHWndInteractPropLocation"

/** @brief Action to plug-in to move the interact child window
      Arguments
        - handle - handle to the image effect
        - inArgs - has the following properties
              - kOfxHWndInteractPropLocation    - the rect to move to
        - outArgs - is set to NULL
*/
#define kOfxHWndInteractActionMoveWindow  "OfxHWndInteractActionMoveWindow"

/** @brief Action to plug-in to dispose the interact child window
      Arguments
        - handle - handle to the image effect
        - inArgs - is set to NULL
        - outArgs - is set to NULL
*/
#define kOfxHWndInteractActionDisposeWindow  "OfxHWndInteractActionDisposeWindow"

/** @brief Action to plug-in to show the interact child window
      Arguments
        - handle - handle to the image effect
        - inArgs - is set to NULL
        - outArgs - is set to NULL
*/
#define kOfxHWndInteractActionShowWindow  "OfxHWndInteractActionShowWindow"



/** @brief OFX suite that allows an effect to interact with an openGL window so as to provide custom interfaces.

*/
typedef struct OfxHWNDInteractSuiteV1 {	
  /** @brief Gets the property set handle for this interact handle */
  OfxStatus (*interactGetPropertySet)(OfxInteractHandle interactInstance,
				      OfxPropertySetHandle *property);
  OfxStatus (*interactUpdate)(OfxInteractHandle interactInstance);
} OfxHWNDInteractSuiteV1;


typedef struct OfxVegasProgressSuiteV1 {  
  OfxStatus (*progressStart)(void *effectInstance,
                             const char *message,
                             const char *messageid);
  OfxStatus (*progressUpdate)(void *effectInstance, double progress);
  OfxStatus (*progressEnd)(void *effectInstance);
} OfxVegasProgressSuiteV1;

typedef struct OfxVegasProgressSuiteV2 {  
  OfxStatus (*progressStart)(void *effectInstance,
                             const char *message,
                             const char *messageid,
                             int showTimeWindows);
  OfxStatus (*progressUpdate)(void *effectInstance, double progress);
  OfxStatus (*progressEnd)(void *effectInstance);
} OfxVegasProgressSuiteV2;

typedef struct OfxVegasStereoscopicImageEffectSuiteV1 {  
  OfxStatus (*clipGetStereoscopicImage)(OfxImageClipHandle clip,
			    OfxTime       time,
                int           iview,
			    OfxRectD     *region,
			    OfxPropertySetHandle   *imageHandle);
} OfxVegasStereoscopicImageSuiteV1;

typedef struct OfxVegasKeyframeSuiteV1 {
  OfxStatus (*paramGetKeyInterpolation)(OfxParamHandle  paramHandle,
                   OfxTime time,
                   char** interpolationType);
  OfxStatus (*paramGetKeySlopes)(OfxParamHandle  paramHandle,
                   OfxTime time,
                   ...);
  OfxStatus (*paramSetKeyInterpolation)(OfxParamHandle  paramHandle,
                   OfxTime time,
                   const char* interpolationType);
  OfxStatus (*paramSetKeySlopes)(OfxParamHandle  paramHandle,
                   OfxTime time,
                   ...);
 } OfxVegasKeyframeSuiteV1;
#endif // #ifndef _ofxSonyVegas_h_
