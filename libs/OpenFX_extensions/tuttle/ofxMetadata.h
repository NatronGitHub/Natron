#ifndef _ofxMetadata_h_
#define _ofxMetadata_h_

// TUTTLE_TODO

#include "ofxCore.h"
#include "ofxParam.h"
#include "ofxClip.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OfxClipMetaDataSuiteV1
{
	OfxStatus clipMetaDataGetParameterSet( const OfxClipHandle* clip, OfxParamSetHandle* paramSet );
} OfxClipMetaDataSuiteV1;

/*

// Need to exist ?
typedef struct OfxImageMetaDataSuiteV1 {

 OfxStatus imageMetaDataGetParameterSet(const OfxImageHandle* image, OfxParamSetHandle *paramSet);

} OfxImageMetaDataSuiteV1;

*/

#ifdef __cplusplus
}
#endif

#endif

