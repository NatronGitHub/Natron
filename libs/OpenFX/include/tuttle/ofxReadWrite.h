#ifndef _ofxReadWrite_h_
#define _ofxReadWrite_h_

#ifdef __cplusplus
extern "C" {
#endif

/** @brief String value used to know the supported extensions of a plugin (specific to i/o).
 *
 * - Type - String X N
 * - Property Set - plugin descriptor (read/write)
 * - Default - empty string X 0
 * - Values - A array of extensions (jpeg, jpg...).
 * 
 */
#define kTuttleOfxImageEffectPropSupportedExtensions "TuttleOfxImageEffectPropSupportedExtensions"

/** @brief Double value used to evaluate the implementation of a plugin (currently specific to i/o).
 *
 * - Type - Double X 1
 * - Property Set - plugin descriptor (read/write)
 * - Default - -1 for a plugin
 * - Valid Values - The value should be between 0 and 100.
 *   - 0 - means bad implementation
 *   - 50 - means good implementation without specific optimization
 *   - 100 - means good implementation with specific optimizations
 *   - -1 - means the plugin is not evaluated
 * 
 */
#define kTuttleOfxImageEffectPropEvaluation "TuttleOfxImageEffectPropEvaluation"

#ifdef __cplusplus
}
#endif

#endif
