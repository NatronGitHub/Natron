.. module:: NatronEngine
.. _Natron:

Natron
******


Detailed Description
--------------------

This class contains enumerations that are used by some functions of the API to return status
that are more complicated than a simple boolean value.


.. attribute:: NatronEngine.Natron.ViewerContextLayoutTypeEnum

    * eViewerContextLayoutTypeSpacing	// Add spacing in pixels after the item
    * eViewerContextLayoutTypeSeparator  // Add a vertical line after the item
    * eViewerContextLayoutTypeStretchAfter // Add layout stretch after the item
    * eViewerContextLayoutTypeAddNewLine // Add a new line after the item


.. attribute:: NatronEngine.Natron.StandardButtonEnum

	Can have the following values:
	
		* eStandardButtonNoButton           = 0x00000000,
    	* eStandardButtonEscape             = 0x00000200,            // obsolete
    	* eStandardButtonOk                 = 0x00000400,
    	* eStandardButtonSave               = 0x00000800,
    	* eStandardButtonSaveAll            = 0x00001000,
    	* eStandardButtonOpen               = 0x00002000,
    	* eStandardButtonYes                = 0x00004000,
    	* eStandardButtonYesToAll           = 0x00008000,
    	* eStandardButtonNo                 = 0x00010000,
    	* eStandardButtonNoToAll            = 0x00020000,
    	* eStandardButtonAbort              = 0x00040000,
    	* eStandardButtonRetry              = 0x00080000,
    	* eStandardButtonIgnore             = 0x00100000,
    	* eStandardButtonClose              = 0x00200000,
    	* eStandardButtonCancel             = 0x00400000,
    	* eStandardButtonDiscard            = 0x00800000,
    	* eStandardButtonHelp               = 0x01000000,
    	* eStandardButtonApply              = 0x02000000,
    	* eStandardButtonReset              = 0x04000000,
    	* eStandardButtonRestoreDefaults    = 0x08000000

.. attribute:: NatronEngine.Natron.ImageBitDepthEnum

	Can have the following values:
	
		* eImageBitDepthNone = 0,
    	* eImageBitDepthByte,
    	* eImageBitDepthShort,
    	* eImageBitDepthFloat
	
.. attribute:: NatronEngine.Natron.KeyframeTypeEnum

	Can have the following values:

		* eKeyframeTypeConstant = 0,
    	* eKeyframeTypeLinear = 1,
    	* eKeyframeTypeSmooth = 2,
    	* eKeyframeTypeCatmullRom = 3,
    	* eKeyframeTypeCubic = 4,
    	* eKeyframeTypeHorizontal = 5,
    	* eKeyframeTypeFree = 6,
    	* eKeyframeTypeBroken = 7,
    	* eKeyframeTypeNone = 8


.. attribute:: NatronEngine.Natron.AnimationLevelEnum

	Can have the following values:

		* eAnimationLevelNone = 0,
    	* eAnimationLevelInterpolatedValue = 1,
    	* eAnimationLevelOnKeyframe = 2

.. attribute:: NatronEngine.Natron.OrientationEnum

	Can have the following values:
	
		* eOrientationHorizontal = 0x1,
    	* eOrientationVertical = 0x2

.. attribute:: NatronEngine.Natron.ImagePremultiplicationEnum
	
	Can have the following values:
	
		* eImagePremultiplicationOpaque = 0,
    	* eImagePremultiplicationPremultiplied,
    	* eImagePremultiplicationUnPremultiplied,

.. attribute:: NatronEngine.Natron.StatusEnum

	Can have the following values:
	
		* eStatusOK = 0,
    	* eStatusFailed = 1,
    	* eStatusReplyDefault = 14

.. attribute:: NatronEngine.Natron.PixmapEnum

	See `here <https://github.com/MrKepzie/Natron/blob/master/Global/Enums.h>`_	for
	potential values of this enumeration. 
	
