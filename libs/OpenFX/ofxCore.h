#ifndef _ofxCore_h_
#define _ofxCore_h_

/*
Software License :

Copyright (c) 2003-2009, The Open Effects Association Ltd. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name The Open Effects Association Ltd, nor the names of its 
      contributors may be used to endorse or promote products derived from this
      software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "stddef.h" // for size_t

#ifdef __cplusplus
extern "C" {
#endif

/** @file ofxCore.h
Contains the core OFX architectural struct and function definitions. For more details on the basic OFX architecture, see \ref Architecture.
*/


/** @brief Platform independent export macro.
 *
 * This macro is to be used before any symbol that is to be
 * exported from a plug-in. This is OS/compiler dependent.
 */
#if defined(WIN32) || defined(WIN64)
	#define OfxExport extern __declspec(dllexport)
#else
	#define OfxExport extern
#endif

/** @brief Blind data structure to manipulate sets of properties through */
typedef struct OfxPropertySetStruct *OfxPropertySetHandle;

/** @brief OFX status return type */
typedef int OfxStatus;

/** @brief Generic host structure passed to OfxPlugin::setHost function

    This structure contains what is needed by a plug-in to bootstrap its connection
    to the host.
*/
typedef struct OfxHost {
  /** @brief Global handle to the host. Extract relevant host properties from this.
      This pointer will be valid while the binary containing the plug-in is loaded.
   */
  OfxPropertySetHandle host;

  /** @brief The function which the plug-in uses to fetch suites from the host.

      \arg \e host          - the host the suite is being fetched from this \em must be the \e host member of the OfxHost struct containing fetchSuite.
      \arg \e suiteName     - ASCII string labelling the host supplied API
      \arg \e suiteVersion  - version of that suite to fetch

      Any API fetched will be valid while the binary containing the plug-in is loaded.

      Repeated calls to fetchSuite with the same parameters will return the same pointer.

      returns
         - NULL if the API is unknown (either the api or the version requested),
	 - pointer to the relevant API if it was found
  */
  void *(*fetchSuite)(OfxPropertySetHandle host, const char *suiteName, int suiteVersion);
} OfxHost;


/** @brief Entry point for plug-ins

  \arg \e action   - ASCII c string indicating which action to take 
  \arg \e instance - object to which action should be applied, this will need to be cast to the appropriate blind data type depending on the \e action
  \arg \e inData   - handle that contains action specific properties
  \arg \e outData  - handle where the plug-in should set various action specific properties

  This is how the host generally communicates with a plug-in. Entry points are used to pass messages
  to various objects used within OFX. The main use is within the OfxPlugin struct.

  The exact set of actions is determined by the plug-in API that is being implemented, however all plug-ins
  can perform several actions. For the list of actions consult \ref ActionsAll.
 */
typedef  OfxStatus (OfxPluginEntryPoint)(const char *action, const void *handle, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs);

/** @brief The structure that defines a plug-in to a host.
 *
 * This structure is the first element in any plug-in structure
 * using the OFX plug-in architecture. By examining its members
 * a host can determine the API that the plug-in implements,
 * the version of that API, its name and version.
 *
 * For details see \ref Architecture.
 *
 */
typedef struct OfxPlugin {
  /** Defines the type of the plug-in, this will tell the host what the plug-in does. e.g.: an image 
      effects plug-in would be a "OfxImageEffectPlugin"
   */
  const char		*pluginApi;

  /** Defines the version of the pluginApi that this plug-in implements */
  int            apiVersion;

  /** String that uniquely labels the plug-in among all plug-ins that implement an API.
      It need not necessarily be human sensible, however the preference is to use reverse
      internet domain name of the developer, followed by a '.' then by a name that represents
      the plug-in.. It must be a legal ASCII string and have no whitespace in the
      name and no non printing chars.
      For example "uk.co.somesoftwarehouse.myPlugin"
  */
  const char 		*pluginIdentifier; 
  
  /** Major version of this plug-in, this gets incremented when backwards compatibility is broken. */
  unsigned int 	 pluginVersionMajor;
  
  /**  Major version of this plug-in, this gets incremented when software is changed,
       but does not break backwards compatibility. */
  unsigned int   pluginVersionMinor;

  /** @brief Function the host uses to connect the plug-in to the host's api fetcher
      
      \arg \e fetchApi - pointer to host's API fetcher

      Mandatory function. 

      The very first function called in a plug-in. The plug-in \em must \em not call any OFX functions within this, it must only set its local copy of the host pointer.

      \pre
        - nothing else has been called

      \post
        - the pointer suite is valid until the plug-in is unloaded
  */
  void     (*setHost)(OfxHost *host);
 
  /** @brief Main entry point for plug-ins

  Mandatory function.

  The exact set of actions is determined by the plug-in API that is being implemented, however all plug-ins
  can perform several actions. For the list of actions consult \ref ActionsAll.

   Preconditions
      - setHost has been called
   */
  OfxPluginEntryPoint *mainEntry;
} OfxPlugin;

/**
   \defgroup ActionsAll OFX Actions

These are the actions passed to a plug-in's 'main' function
*/
/*@{*/

/** @brief Action called just after a plug-in has been loaded, for more details see \ref ArchitectureMainFunction and \ref ActionsGeneralLoad */
#define  kOfxActionLoad "OfxActionLoad"

/** @brief Action called to have a plug-in describe itself to the host, for more details see \ref ArchitectureMainFunction and \ref ActionsGeneralDescribe */
#define kOfxActionDescribe "OfxActionDescribe"

/** @brief Action called just before a plug-in is unloaded, for more details see \ref ArchitectureMainFunction and \ref ActionsGeneralUnload */
#define kOfxActionUnload "OfxActionUnload"

/** @brief Action called to have a plug-in purge any temporary caches it may have allocated \ref ArchitectureMainFunction and \ref ActionsGeneralPurgeCaches */
#define kOfxActionPurgeCaches                 "OfxActionPurgeCaches"

/** @brief Action called to have a plug-in sync any internal data structures into custom parameters */
#define kOfxActionSyncPrivateData                 "OfxActionSyncPrivateData"

/** @brief Action called just after an instance has been created \ref ArchitectureMainFunction and \ref ActionsGeneralCreateInstance  */
#define kOfxActionCreateInstance        "OfxActionCreateInstance"

/** @brief Action called just before an instance is destroyed and \ref ActionsGeneralDestroyInstance */
#define kOfxActionDestroyInstance       "OfxActionDestroyInstance"

/** @brief Action indicating something in the instance has been changed, see \ref ActionsGeneralInstanceChanged */
#define kOfxActionInstanceChanged "OfxActionInstanceChanged"

/** @brief Action called before the start of a set of kOfxActionEndInstanceChanged actions, used with ::kOfxActionEndInstanceChanged to bracket a grouped set of changes, see \ref ActionsGeneralInstanceChangedBeginEnd */
#define kOfxActionBeginInstanceChanged "OfxActionBeginInstanceChanged"

/** @brief Action called after the end of a set of kOfxActionEndInstanceChanged actions, used with ::kOfxActionBeginInstanceChanged to bracket a grouped set of changes,  see \ref ActionsGeneralInstanceChangedBeginEnd*/
#define kOfxActionEndInstanceChanged "OfxActionEndInstanceChanged"

/** @brief Action called when an instance has the first editor opened for it */
#define kOfxActionBeginInstanceEdit "OfxActionBeginInstanceEdit"

/** @brief Action called when an instance has the last editor closed */
#define kOfxActionEndInstanceEdit "OfxActionEndInstanceEdit"

/*@}*/

/** @brief Returns the 'nth' plug-in implemented inside a binary
 *
 * Returns a pointer to the 'nth' plug-in implemented in the binary. A function of this type
 * must be implemented in and exported from each plug-in binary.
 */
OfxExport OfxPlugin *OfxGetPlugin(int nth);

/** @brief Defines the number of plug-ins implemented inside a binary
 *
 * A host calls this to determine how many plug-ins there are inside
 * a binary it has loaded. A function of this type
 * must be implemented in and exported from each plug-in binary.
 */
OfxExport int OfxGetNumberOfPlugins(void);

/**
   \defgroup PropertiesAll Ofx Properties 

These strings are used to identify properties within OFX, they are broken up by the host suite or API they relate to.
*/
/*@{*/

/**
   \defgroup PropertiesGeneral General Properties 

These properties are general properties and  apply to may objects across OFX
*/
/*@{*/

/** @brief Property on the host descriptor, saying what API version of the API is being implemented

    - Type - int X N
    - Property Set - host descriptor.

This is a version string that will specify which version of the API is being implemented by a host. It
can have multiple values. For example "1.0", "1.2.4" etc.....

If this is not present, it is safe to assume that the version of the API is "1.0".
*/
#define kOfxPropAPIVersion "OfxPropAPIVersion"

/** @brief General property used to get/set the time of something.

    - Type - double X 1
    - Default - 0, if a setable property
    - Property Set - commonly used as an argument to actions, input and output.
*/
#define kOfxPropTime "OfxPropTime"

/** @brief Indicates if a host is actively editing the effect with some GUI.

    - Type - int X 1
    - Property Set - effect instance (read only)
    - Valid Values - 0 or 1

If false the effect currently has no interface, however this may be because the effect is loaded in a background render host, or it may be loaded on an interactive host that has not yet opened an editor for the effect.

The output of an effect should only ever depend on the state of its parameters, not on the interactive flag. The interactive flag is more a courtesy flag to let a plugin know that it has an interace. If a plugin want's to have its behaviour dependant on the interactive flag, it can always make a secret parameter which shadows the state if the flag.
*/
#define kOfxPropIsInteractive "OfxPropIsInteractive"

/** @brief The file path to the plugin.

    - Type - C string X 1
    - Property Set - effect descriptor (read only)

This is a string that indicates the file path where the plug-in was found by the host. The path is in the native
path format for the host OS (eg:  UNIX directory separators are forward slashes, Windows ones are backslashes).

The path is to the bundle location, see \ref InstallationLocation. 
eg:  '/usr/OFX/Plugins/AcmePlugins/AcmeFantasticPlugin.ofx.bundle'
*/
#define kOfxPluginPropFilePath "OfxPluginPropFilePath"

/** @brief  A private data pointer that the plug-in can store its own data behind.

    - Type - pointer X 1
    - Property Set - plugin instance (read/write),
    - Default - NULL

This data pointer is unique to each plug-in instance, so two instances of the same plug-in do not share the same data pointer. Use it to hang any needed private data structures.
*/
#define kOfxPropInstanceData "OfxPropInstanceData"

/** @brief General property, used to identify the kind of an object behind a handle

    - Type - ASCII C string X 1
    - Property Set - any object handle (read only)
    - Valid Values - currently this can be...
       - ::kOfxTypeImageEffectHost
       - ::kOfxTypeImageEffect
       - ::kOfxTypeImageEffectInstance
       - ::kOfxTypeParameter
       - ::kOfxTypeParameterInstance
       - ::kOfxTypeClip
       - ::kOfxTypeImage
*/
#define kOfxPropType "OfxPropType"

/** @brief Unique name of an object.

    - Type - ASCII C string X 1
    - Property Set - on many objects (descriptors and instances), see \ref PropertiesByObject (read only)

This property is used to label objects uniquely amoung objects of that type. It is typically set when a plugin creates a new object with a function that takes a name.
*/
#define kOfxPropName "OfxPropName"

/** @brief Identifies a specific version of a host or plugin.

    - Type - int X N
    - Property Set - host descriptor (read only), plugin descriptor (read/write)
    - Default - "0"
    - Valid Values - positive integers

This is a multi dimensional integer property that represents the version of a host (host descriptor), or plugin (plugin descriptor). These represent a version number of the form '1.2.3.4', with each dimension adding another 'dot' on the right.

A version is considered to be more recent than another if its ordered set of values is lexicographically greater than another, reading left to right. (ie: 1.2.4 is smaller than 1.2.6). Also, if the number of dimensions is different, then the values of the missing dimensions are considered to be zero (so 1.2.4 is greater than 1.2).
*/
#define kOfxPropVersion "OfxPropVersion"

/** @brief Unique user readable version string of a plugin or host.

    - Type - string X 1
    - Property Set - host descriptor (read only), plugin descriptor (read/write)
    - Default - none, the host needs to set this
    - Valid Values - ASCII string

This is purely for user feedback, a plugin or host should use ::kOfxPropVersion if they need
to check for specific versions.
*/
#define kOfxPropVersionLabel "OfxPropVersionLabel"

/** @brief Description of the plug-in to a user.

    - Type - string X 1
    - Property Set - plugin descriptor (read/write) and instance (read only)
    - Default - ""
    - Valid Values - UTF8 string

This is a string giving a potentially verbose description of the effect.
*/
#define kOfxPropPluginDescription "OfxPropPluginDescription"

/** @brief User visible name of an object.

    - Type - UTF8 C string X 1
    - Property Set - on many objects (descriptors and instances), see \ref PropertiesByObject. Typically readable and writable in most cases.
    - Default - the ::kOfxPropName the object was created with.

The label is what a user sees on any interface in place of the object's name. 

Note that resetting this will also reset ::kOfxPropShortLabel and ::kOfxPropLongLabel.
*/
#define kOfxPropLabel "OfxPropLabel"

/** @brief If set this tells the host to use an icon instead of a label for some object in the interface.

    - Type - string X 2
    - Property Set - various descriptors in the API
    - Default - ""
    - Valid Values - ASCII string

The value is a path is defined relative to the Resource folder that points to an SVG or PNG file containing the icon.

The first dimension, if set, will the name of and SVG file, the second a PNG file.
*/
#define kOfxPropIcon "OfxPropIcon"

/** @brief Short user visible name of an object.

    - Type - UTF8 C string X 1
    - Property Set - on many objects (descriptors and instances), see \ref PropertiesByObject. Typically readable and writable in most cases.
    - Default - initially ::kOfxPropName, but will be reset if ::kOfxPropLabel is changed.

This is a shorter version of the label, typically 13 character glyphs or less. Hosts should use this if they have limitted display space for their object labels.
*/
#define kOfxPropShortLabel "OfxPropShortLabel"

/** @brief Long user visible name of an object.

    - Type - UTF8 C string X 1
    - Property Set - on many objects (descriptors and instances), see \ref PropertiesByObject. Typically readable and writable in most cases.
    - Default - initially ::kOfxPropName, but will be reset if ::kOfxPropLabel is changed.

This is a longer version of the label, typically 32 character glyphs or so. Hosts should use this if they have mucg display space for their object labels.
*/
#define kOfxPropLongLabel "OfxPropLongLabel"

/** @brief Indicates why a plug-in changed.

    - Type - ASCII C string X 1
    - Property Set - the inArgs parameter on the ::kOfxActionInstanceChanged action.
    - Valid Values - this can be...
       - ::kOfxChangeUserEdited - the user directly edited the instance somehow and caused a change to something, this includes undo/redos and resets
       - ::kOfxChangePluginEdited - the plug-in itself has changed the value of the object in some action
       - ::kOfxChangeTime - the time has changed and this has affected the value of the object because it varies over time

Argument property for the ::kOfxActionInstanceChanged action.
*/
#define kOfxPropChangeReason "OfxPropChangeReason"

/** @brief A pointer to an effect instance.

    - Type - pointer X 1
    - Property Set - on an interact instance (read only)

This property is used to link an object to the effect. For example if the plug-in supplies an openGL overlay for an image effect, 
the interact instance will have one of these so that the plug-in can connect back to the effect the GUI links to.
*/
#define kOfxPropEffectInstance "OfxPropEffectInstance"

/** @brief A pointer to an operating system specific application handle.

    - Type - pointer X 1
    - Property Set - host descriptor.

Some plug-in vendor want raw OS specific handles back from the host so they can do interesting things with host OS APIs. Typically this is to control windowing properly on Microsoft Windows. This property returns the appropriate 'root' window handle on the current operating system. So on Windows this would be the hWnd of the application main window.
*/
#define kOfxPropHostOSHandle "OfxPropHostOSHandle"

/*@}*/

/*@}*/

/** @brief String used as a value to ::kOfxPropChangeReason to indicate a user has changed something */
#define kOfxChangeUserEdited "OfxChangeUserEdited"

/** @brief String used as a value to ::kOfxPropChangeReason to indicate the plug-in itself has changed something */
#define kOfxChangePluginEdited "OfxChangePluginEdited"

/** @brief String used as a value to ::kOfxPropChangeReason to a time varying object has changed due to a time change */
#define kOfxChangeTime "OfxChangeTime"

/** @brief How time is specified within the OFX API */
typedef double OfxTime;

/** @brief Defines one dimensional integer bounds */
typedef struct OfxRangeI {
  int min, max;
} OfxRangeI;

/** @brief Defines one dimensional double bounds */
typedef struct OfxRangeD {
  double min, max;
} OfxRangeD;

/** @brief Defines two dimensional integer point */
typedef struct OfxPointI {
  int x, y;
} OfxPointI;

/** @brief Defines two dimensional double point */
typedef struct OfxPointD {
  double x, y;
} OfxPointD;

/** @brief Used to flag infinite rects. Set minimums to this to indicate infinite

This is effectively INT_MAX. 
 */
#define kOfxFlagInfiniteMax ((int)((1 << (sizeof(int)*8 - 1)) - 1))

/** @brief Used to flag infinite rects. Set minimums to this to indicate infinite.

This is effectively INT_MIN
 */
#define kOfxFlagInfiniteMin ((int)(-kOfxFlagInfiniteMax - 1))

/** @brief Defines two dimensional integer region

Regions are x1 <= x < x2

Infinite regions are flagged by setting
- x1 = kOfxFlagInfiniteMin
- y1 = kOfxFlagInfiniteMin
- x2 = kOfxFlagInfiniteMax
- y2 = kOfxFlagInfiniteMax

 */
typedef struct OfxRectI {
  int x1, y1, x2, y2;
} OfxRectI;

/** @brief Defines two dimensional double region

Regions are x1 <= x < x2

Infinite regions are flagged by setting
- x1 = kOfxFlagInfiniteMin
- y1 = kOfxFlagInfiniteMin
- x2 = kOfxFlagInfiniteMax
- y2 = kOfxFlagInfiniteMax

 */
typedef struct OfxRectD {
  double x1, y1, x2, y2;
} OfxRectD;

/** @brief String used to label unset bitdepths */
#define kOfxBitDepthNone "OfxBitDepthNone"

/** @brief String used to label unsigned 8 bit integer samples */
#define kOfxBitDepthByte "OfxBitDepthByte"

/** @brief String used to label unsigned 16 bit integer samples */
#define kOfxBitDepthShort "OfxBitDepthShort"

/** @brief String used to label signed 32 bit floating point samples */
#define kOfxBitDepthFloat "OfxBitDepthFloat"

/**
   \defgroup StatusCodes Status Codes 

These strings are used to identify error states within ofx, they are returned
by various host suite functions, as well as plug-in functions. The valid return codes 
for each function are documented with that function.
*/
/*@{*/

/**
   \defgroup StatusCodesGeneral General Status Codes 

General status codes start at 1 and continue until 999

*/
/*@{*/

/** @brief Status code indicating all was fine */
#define kOfxStatOK 0

/** @brief Status error code for a failed operation */
#define kOfxStatFailed  ((int)1)

/** @brief Status error code for a fatal error

  Only returned in the case where the plug-in or host cannot continue to function and needs to be restarted. 
 */
#define kOfxStatErrFatal ((int)2)

/** @brief Status error code for an operation on or request for an unknown object */
#define kOfxStatErrUnknown ((int)3)

/** @brief Status error code returned by plug-ins when they are missing host functionality, either an API or some optional functionality (eg: custom params).

    Plug-Ins returning this should post an appropriate error message stating what they are missing.
 */
#define kOfxStatErrMissingHostFeature ((int) 4)

/** @brief Status error code for an unsupported feature/operation */
#define kOfxStatErrUnsupported ((int) 5)

/** @brief Status error code for an operation attempting to create something that exists */
#define kOfxStatErrExists  ((int) 6)

/** @brief Status error code for an incorrect format */
#define kOfxStatErrFormat ((int) 7)

/** @brief Status error code indicating that something failed due to memory shortage */
#define kOfxStatErrMemory  ((int) 8)

/** @brief Status error code for an operation on a bad handle */
#define kOfxStatErrBadHandle ((int) 9)

/** @brief Status error code indicating that a given index was invalid or unavailable */
#define kOfxStatErrBadIndex ((int)10)

/** @brief Status error code indicating that something failed due an illegal value */
#define kOfxStatErrValue ((int) 11)

/** @brief OfxStatus returned indicating a 'yes' */
#define kOfxStatReplyYes ((int) 12)

/** @brief OfxStatus returned indicating a 'no' */
#define kOfxStatReplyNo ((int) 13)

/** @brief OfxStatus returned indicating that a default action should be performed */
#define kOfxStatReplyDefault ((int) 14)

/*@}*/

/*@}*/

#ifdef __cplusplus
}
#endif

/** @mainpage OFX : Open Plug-Ins For Special Effects

This page represents the automatically extracted HTML documentation of the source headers for the OFX Image Effect API. The documentation was extracted by doxygen (http://www.doxygen.org). It breaks documentation into sets of pages, use the links at the top of this page (marked 'Modules', 'Compound List' and especially 'File List' etcc) to browse through the OFX doc.

A more complete reference manual is http://openfx.sourceforge.net .

*/

#endif
