#ifndef _ofxParamAPI_h_
#define _ofxParamAPI_h_

#include "ofxCore.h"
#include "ofxAttribute.h"
#include "ofxParam.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief String used to label OFX Param manipulation Plug-ins
 *
 * Set the pluginApi member of the OfxPluginHeader inside any OfxParamNodePluginStruct
 * to be this so that the host knows the plugin is an param node.
 */
#define kOfxParamPluginApi "OfxParamPluginAPI"

/**
 * @brief The current version of the Param API
 */
#define kOfxParamPluginApiVersion 1

/** @brief Blind declaration of an OFX param node
 */
typedef struct OfxParamNodeStruct* OfxParamNodeHandle;


/** @brief Used as a value for ::kOfxPropType on param node host handles */
#define kOfxTypeParameterNodeHost "OfxTypeParameterNodeHost"

/** @brief Used as a value for ::kOfxPropType on param node plugin handles */
#define kOfxTypeParameterNode "OfxTypeParameterNode"

/** @brief Used as a value for ::kOfxPropType on image effect instance handles  */
#define kOfxTypeParameterNodeInstance "OfxTypeParameterNodeInstance"



/**
 * @defgroup ActionsParamNode Actions Param Node
 */
///@{

#define kOfxParamNodeActionGetTimeDomain "OfxParamNodeActionGetTimeDomain"
/// GetTimeRangeNeeded is similar to the function GetFramesNeeded in ImageEffectNode
#define kOfxParamNodeActionGetTimeRangeNeeded "OfxParamNodeActionGetTimeRangeNeeded"
#define kOfxParamNodeActionProcess "OfxParamNodeActionProcess"
#define kOfxParamNodeActionBeginSequenceProcess "OfxParamNodeActionBeginSequenceProcess"
#define kOfxParamNodeActionEndSequenceProcess "OfxParamNodeActionEndSequenceProcess"

///@}

/** @brief the string that names image effect suites, passed to OfxHost::fetchSuite */
#define kOfxParamNodeSuite "OfxParamNodeSuite"


/** @brief The OFX suite for param manipulation
 *
 * This suite provides the functions needed by a plugin to defined and use a param manipulation plugin.
 */
typedef struct OfxParamNodeSuiteV1
{
	/** @brief Retrieves the property set for the given param node
	 *
	 * \arg paramNode   param node to get the property set for
	 * \arg propHandle    pointer to a the property set pointer, value is returned here
	 *
	 * The property handle is for the duration of the param node handle.
	 *
	 * @returns
	 * - ::kOfxStatOK       - the property set was found and returned
	 * - ::kOfxStatErrBadHandle  - if the paramter handle was invalid
	 * - ::kOfxStatErrUnknown    - if the type is unknown
	 */
	OfxStatus ( *getPropertySet )( OfxParamNodeHandle  paramNode,
	                               OfxPropertySetHandle* propHandle );

	/** @brief Retrieves the parameter set for the given param node
	 *
	 * \arg paramNode   param node to get the property set for
	 * \arg paramSet     pointer to a the parameter set, value is returned here
	 *
	 * The param set handle is valid for the lifetime of the param node handle.
	 *
	 * @returns
	 * - ::kOfxStatOK       - the property set was found and returned
	 * - ::kOfxStatErrBadHandle  - if the paramter handle was invalid
	 * - ::kOfxStatErrUnknown    - if the type is unknown
	 */
	OfxStatus ( *getParamSet )( OfxParamNodeHandle paramNode,
	                            OfxParamSetHandle*   paramSet );

	/** @brief Returns whether to abort processing or not.
	 *
	 *  \arg paramNode  - instance of the param node
	 *
	 * A host may want to signal to a plugin that it should stop whatever rendering it is doing and start again.
	 * Generally this is done in interactive threads in response to users tweaking some parameter.
	 *
	 * This function indicates whether a plugin should stop whatever processing it is doing.
	 *
	 * @returns
	 * - 0 if the effect should continue whatever processing it is doing
	 * - 1 if the effect should abort whatever processing it is doing
	 */
	int ( *abort )( OfxParamNodeHandle paramNode );


} OfxParamNodeSuiteV1;



#ifdef __cplusplus
}
#endif

#endif

