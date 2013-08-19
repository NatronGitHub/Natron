#ifndef _tuttle_extension_ofxParam_h_
#define _tuttle_extension_ofxParam_h_

#ifdef	__cplusplus
extern "C" {
#endif


/** @brief Set an label option in a choice parameter.
 *
 *  - Type - UTF8 C string X N
 *  - Property Set - plugin parameter descriptor (read/write) and instance (read/write),
 *  - Default - the property is empty with no options set.
 *
 * This property contains the set of options that will be presented to a user from a choice parameter. See @ref ParametersChoice for more details.
 */
#define kOfxParamPropChoiceLabelOption "OfxParamPropChoiceLabelOption"


#ifdef	__cplusplus
}
#endif

#endif	/* OFXPARAM_H */

