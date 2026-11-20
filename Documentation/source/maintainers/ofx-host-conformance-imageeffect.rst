.. _ofx-host-conformance-imageeffect:

OFX host conformance: image effects
===================================

This page is part of the :ref:`OpenFX 1.5.1 host conformance audit <ofx-host-conformance>`.
It covers the image-effect API: contexts, plug-in and host descriptor properties, clip
preferences, clips and images, the image-effect actions, render thread safety and the
image-effect suite.

**Spec scope.** The headers ``ofxImageEffect.h`` and ``ofxPixels.h`` (which only defines pixel
structs and has no host statements), and the Reference pages ``ofxImageEffectContexts.rst``,
``ofxClipPreferences.rst``, ``ofxImageClip.rst``, ``ofxRendering.rst``,
``ofxThreadSafety.rst``, ``ofxProcessingArch.rst``, the temporal and infinite-RoD parts of
``ofxCoordSystem.rst``, ``ofxImageEffectAPI.rst`` and ``ofxImageEffectActions.rst``.
Plug-in-only obligations are skipped unless the host has to check them. Generic actions (Load,
Describe, CreateInstance, InstanceChanged...) are on the :ref:`core page <ofx-host-conformance-core>`;
colour management and GPU rendering are on the
:ref:`interacts, GPU and packaging page <ofx-host-conformance-interact-gpu-packaging>`.

**Spec reference abbreviations.** ``h:`` (and a bare ``:line`` in the spec column) is
``ofxImageEffect.h``. ``clipprefs.rst`` is ``ofxClipPreferences.rst``, ``contexts.rst`` is
``ofxImageEffectContexts.rst``, ``imageclip.rst`` is ``ofxImageClip.rst``, ``render.rst``
and ``rendering.rst`` are ``ofxRendering.rst``, ``procarch.rst`` is ``ofxProcessingArch.rst``,
``coordsys.rst`` is ``ofxCoordSystem.rst`` and ``threadsafety.rst`` is ``ofxThreadSafety.rst``.

**Code references.** ``HS`` is ``libs/OpenFX/HostSupport/src/`` (a bare ``HS :line`` is in
``ofxhImageEffect.cpp``). Natron files are named without their directory (mostly ``Engine/``).
The rows are numbered per section: row 3.9 is row 9 of section 3.

Verdicts: **OK**; **MISSING** (not implemented); **WRONG** (implemented, but contradicts the spec); **PARTIAL** (partly implemented, or only in some cases); **N/A** (does not apply to Natron); **NEXT** (seen, but out of scope of this audit). The level column gives the strength of the spec statement (MUST, SHOULD, MAY) or DEFAULT for a default value. Spec references link to the upstream ``OFX_Release_1.5.1`` tag. Code references are plain paths with line numbers at the audited commits (Natron RB-2.6 at 7dea437a6, HostSupport at 93c21ed7); they are not updated when the code moves. Each row or problem that is not OK links to the issue that tracks it; items marked **deliberate, won't fix** are known deviations that Natron keeps on purpose.

1. ofxImageEffect.h: contexts, types and host descriptor
--------------------------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec (1.5.1)
     - level
     - Natron + HostSupport
     - verdict
   * - 1
     - `ofxImageEffect.h:55-72 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L55-L72>`__ the six standard contexts; `ofxImageEffectContexts.rst:65-74 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxImageEffectContexts.rst?plain=1#L65-L74>`__ "A host indicates which contexts it supports by setting kOfxImageEffectPropSupportedContexts in the global host descriptor"
     - MUST
     - N: Engine/OfxHost.cpp:286-289 advertises Generator, Filter, General and Transition. Paint and Retimer are not advertised. Reader/Writer are commented out (Furnace bug, :291-293).
     - OK
   * - 2
     - `ofxImageEffect.h:527-530 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L527-L530>`__ "If a host does not support a certain context, then it need not call DescribeInContext for that context"; `ofxImageEffectContexts.rst:56-63 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxImageEffectContexts.rst?plain=1#L56-L63>`__ a tree host should prefer General
     - MAY/SHOULD
     - N: OfxHost.cpp:588-668 ``getContext_internal`` picks one context: Reader > Writer > Tracker > General > Filter > Paint > Generator > Transition. A plug-in whose only context is Paint or Retimer is still described and instantiated in that context, although the host does not advertise it. HS ofxhImageEffectAPI.cpp:149-160 and 228 do no filtering.
     - PARTIAL

       Issue: `#1383 <https://github.com/NatronGitHub/Natron/issues/1383>`__
   * - 3
     - `ofxImageEffect.h:548 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L548>`__ DescribeInContext returns kOfxStatErrMissingHostFeature: "the context will be ignored by the host"
     - MUST
     - HS ofxhImageEffectAPI.cpp:260-264 returns 0. N OfxHost.cpp:706-710 then throws, so the plug-in fails to load. Natron does not fall back to the plug-in's next context.
     - PARTIAL

       Issue: `#1383 <https://github.com/NatronGitHub/Natron/issues/1383>`__
   * - 4
     - Reader/Writer contexts (task list)
     - n/a
     - These are TuttleOFX extensions (``kOfxImageEffectContextReader/Writer``) and are not in 1.5.1. Natron uses them without advertising them (OfxHost.cpp:601-614). Default RoD: HS ofxhImageEffect.cpp:1919-1932.
     - N/A (extension)
   * - 5
     - `ofxImageEffectContexts.rst:93-97 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxImageEffectContexts.rst?plain=1#L93-L97>`__ the instance context never changes and is readable from kOfxImageEffectPropContext
     - MUST
     - HS ofxhImageEffect.cpp:697 (read-only) and :757.
     - OK
   * - 6
     - `ofxImageEffect.h:74-87 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L74-L87>`__ kOfxPropType values: "kOfxTypeImageEffectInstance ... on image effect instance handles"
     - DEFAULT
     - HS: an instance copies the descriptor property set (``Base(other)``, ofxhImageEffect.cpp:724 and :85 ``kOfxTypeImageEffect``). Nothing overrides it: kOfxTypeImageEffectInstance appears nowhere in HS or Natron. The host has kOfxTypeImageEffectHost (HS ofxhHost.cpp:88), clips have kOfxTypeClip and images have kOfxTypeImage (ofxhClip.cpp:64 and :762).
     - WRONG (instance only)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 7
     - `ofxImageEffectContexts.rst:182-187 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxImageEffectContexts.rst?plain=1#L182-L187>`__ and 287-317: Transition/SourceTime are host-driven pseudo-parameters, "not meant to be treated as normal parameters"
     - SHOULD
     - Natron exposes them as ordinary knobs. Nothing drives 'Transition' (no matches for the mandated param names in Engine/Gui). This is an acceptable "explicit curve" UI, but the host does not drive the value.
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 8
     - `ofxImageEffectContexts.rst:113-133 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxImageEffectContexts.rst?plain=1#L113-L133>`__ generator: "the host must choose" output components and depths among the ones the plug-in supports
     - MUST
     - N: EffectInstance.cpp:5495-5509 (``findClosestSupportedComponents``, ``getClosestSupportedBitDepth``), re-checked in checkMetadata at EffectInstance.cpp:5776-5788.
     - OK
   * - 9
     - `ofxImageEffect.h:596-604 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L596-L604>`__ kOfxImageEffectHostPropIsBackground
     - DEFAULT
     - N OfxHost.cpp:267 ``appPTR->isBackground()``.
     - OK
   * - 10
     - `ofxImageEffect.h:752-767 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L752-L767>`__ kOfxImageEffectHostPropNativeOrigin
     - DEFAULT
     - N OfxHost.cpp:322 BottomLeft.
     - OK
   * - 11
     - `ofxImageEffect.h:795-803 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L795-L803>`__ kOfxImageEffectPropSupportsOverlays
     - DEFAULT
     - N OfxHost.cpp:268 = 1.
     - OK
   * - 12
     - `ofxImageEffect.h:995-1025 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L995-L1025>`__ host SupportedPixelDepths and SupportedComponents
     - DEFAULT
     - N OfxHost.cpp:272-284: Float, Short, Byte (no Half); RGBA, Alpha, RGB (setting-dependent) plus extensions.
     - OK
   * - 13
     - `ofxImageEffect.h:649-675 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L649-L675>`__ host kOfxImageEffectPropSupportsMultipleClipDepths / MultipleClipPARs
     - DEFAULT
     - N OfxHost.cpp:295-296 = 1 / 1.
     - OK (see 3.x for honouring)
   * - 14
     - `ofxImageEffect.h:691-715 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L691-L715>`__ host SetableFrameRate / SetableFielding
     - DEFAULT
     - N OfxHost.cpp:297-298 = 1 / 0.
     - OK (see 3.9 and 3.10)
   * - 15
     - `ofxImageEffect.h:829-857 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L829-L857>`__ host SupportsMultiResolution, SupportsTiles; `ofxImageEffect.h:860-873 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L860-L873>`__ TemporalClipAccess
     - DEFAULT
     - N OfxHost.cpp:269-271 = 1, 1, 1.
     - OK
   * - 16
     - `ofxImageEffect.h:717-735 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L717-L735>`__ host kOfxImageEffectInstancePropSequentialRender: "2 ... can sometimes render sequentially, and will have set kOfxImageEffectPropSequentialRenderStatus on the relevant actions"
     - DEFAULT
     - N OfxHost.cpp:311 = 2. See 5.x for how the status is set.
     - OK (value)
   * - 17
     - `ofxImageEffect.h:1167-1179 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1167-L1179>`__ host kOfxImageEffectPropRenderQualityDraft
     - DEFAULT
     - N OfxHost.cpp:321 = 1.
     - OK

2. ofxImageEffect.h: plug-in descriptor properties (defaults, and whether the host honours them)
------------------------------------------------------------------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron + HostSupport
     - verdict
   * - 1
     - `ofxImageEffect.h:606-617 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L606-L617>`__ kOfxImageEffectPluginPropSingleInstance "1 - only one instance can exist at any one time", default 0
     - MUST (host)
     - HS: default 0 (ofxhImageEffect.cpp:95), getter ``Base::isSingleInstance()`` :252. Natron never reads it (no ``SingleInstance`` match in Engine, Gui, App or Renderer, and none in history via ``git log -S isSingleInstance``). Natron also creates hidden extra OFX instances (render clones, N OfxEffectInstance.cpp:641-663) for InstanceSafe plug-ins.
     - MISSING

       Issue: `#1364 <https://github.com/NatronGitHub/Natron/issues/1364>`__
   * - 2
     - `ofxImageEffect.h:619-636 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L619-L636>`__ kOfxImageEffectPluginRenderThreadSafety, default InstanceSafe
     - DEFAULT/MUST
     - HS default InstanceSafe (:96). Honouring: see section 6.
     - OK (default)
   * - 3
     - `ofxImageEffect.h:638-647 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L638-L647>`__ HostFrameThreading, default 1
     - DEFAULT
     - HS default 1 (:97). See 6.4.
     - OK
   * - 4
     - `ofxImageEffect.h:649-675 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L649-L675>`__ plug-in MultipleClipDepths / MultipleClipPARs, default 0
     - DEFAULT
     - HS :104-105.
     - OK
   * - 5
     - `ofxImageEffect.h:678-689 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L678-L689>`__ ClipPreferencesSlaveParam: "A value change in any one of these will trigger a call to the clip preferences action"
     - MUST
     - N OfxImageEffectInstance.cpp:732 ``setIsMetadataSlave``; Knob.cpp:5762 and :5933 count changes and refresh the metadata, which calls GetClipPreferences.
     - OK
   * - 6
     - `ofxImageEffect.h:717-735 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L717-L735>`__ SequentialRender: "plugin descriptor (read/write) or plugin instance (read/write)"
     - MUST
     - N OfxEffectInstance.cpp:414-428 reads the descriptor value once, at creation (``getPlugin()->getDescriptor()``). A value the plug-in sets later on the instance is ignored.
     - PARTIAL

       Issue: `#1375 <https://github.com/NatronGitHub/Natron/issues/1375>`__
   * - 7
     - `ofxImageEffect.h:829-842 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L829-L842>`__ SupportsMultiResolution, default 1 for plugins
     - DEFAULT
     - HS :98. N OfxEffectInstance.cpp:2857-2865 (descriptor AND instance).
     - OK
   * - 8
     - `ofxImageEffect.h:844-857 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L844-L857>`__ SupportsTiles, default 1 for plugin and clip; "V1.4: possible to change in Instance Changed"
     - DEFAULT/MUST
     - HS :99 and ofxhClip.cpp:77. The instance inherits the descriptor value through ``Base(other)``, so the NEXT `#177 <https://github.com/NatronGitHub/Natron/issues/177>`__ HostSupport bug is not present here. N OfxEffectInstance.cpp:2821-2837 reads the instance and the output clip. EffectInstance.cpp:4844 calls ``refreshDynamicProperties()`` after every instanceChanged (Node.cpp:407-427).
     - OK
   * - 9
     - `ofxImageEffect.h:860-873 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L860-L873>`__ TemporalClipAccess, default 0 for plugin and clip
     - DEFAULT
     - HS :101 and ofxhClip.cpp:73.
     - OK
   * - 10
     - `ofxImageEffect.h:1181-1195 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1181-L1195>`__ kOfxImageEffectPropNoSpatialAwareness (1.5.1), plug-in default "false"
     - DEFAULT
     - HS :168 default "false".
     - OK
   * - 11
     - `ofxImageEffect.h:1369-1378 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1369-L1378>`__ FieldRenderTwiceAlways (default 1); `ofxImageEffect.h:1380-1394 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1380-L1394>`__ clip FieldExtraction (default Doubled)
     - DEFAULT
     - HS :103 and ofxhClip.cpp:76 set the defaults. Natron never renders fields (see 4.10).
     - N/A (no fielded footage, `#7 <https://github.com/NatronGitHub/Natron/issues/7>`__)
   * - 12
     - `ofxImageEffect.h:1027-1048 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1027-L1048>`__ clip Optional (default 0), IsMask (default 0)
     - DEFAULT
     - HS ofxhClip.cpp:74-75.
     - OK

3. Clip preferences (`ofxImageEffect.h:286-345 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L286-L345>`__, ofxClipPreferences.rst)
------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Natron does not use HS ``Instance::getClipPreferences()``. It has its own path: N OfxEffectInstance.cpp:1376-1408 → OfxImageEffectInstance.cpp:1201-1430 (``setupClipPreferencesArgsFromMetadata`` / ``getClipPreferences_safe``). The defaults come from EffectInstance.cpp:5338-5532 (``getDefaultMetadata``), the post-check from EffectInstance.cpp:5766-5911 (``checkMetadata``), and the results are pushed back to the clips at OfxEffectInstance.cpp:1328-1373.

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron
     - verdict
   * - 1
     - `ofxImageEffect.h:293-296 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L293-L296>`__ "called once after creation of an instance, each time an input clip is changed, and whenever a [slave] parameter ... has its value changed"
     - SHOULD
     - Done by the metadata refresh on creation, on input change (NodeInputs.cpp:1595-1606 → forceRefreshAllInputRelatedData) and on slave-param change (2.5).
     - OK
   * - 2
     - `ofxClipPreferences.rst:42-43 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxClipPreferences.rst?plain=1#L42-L43>`__ "The clip preferences action is never called until all non-optional clips have been attached"
     - MUST
     - N Node.cpp:6130-6133: ``canCallRefreshMetadata = true; // = !hasMandatoryInputDisconnected();``, disabled on purpose in 90d644baa ("so that layer menus get properly created"). ``areAllNonOptionalClipsConnected()`` (OfxImageEffectInstance.cpp:1189) and HS ``checkClipConnectionStatus()`` (ofxhImageEffect.cpp:1064) have no callers.
     - WRONG (deliberate, won't fix)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 3
     - `ofxImageEffect.h:298-299 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L298-L299>`__ inArgs is NULL
     - DEFAULT
     - OfxImageEffectInstance.cpp:1316-1319 passes 0.
     - OK
   * - 4
     - `ofxImageEffect.h:301-311 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L301-L311>`__ OfxImageClipPropComponents\_/Depth\_ for every attached input clip and the output clip
     - MUST
     - OfxImageEffectInstance.cpp:1237-1286 covers all clips, connected or not.
     - OK
   * - 5
     - `ofxClipPreferences.rst:101-118 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxClipPreferences.rst?plain=1#L101-L118>`__ depth: if host or plug-in sets MultipleClipDepths to 0, all clips get one common depth, "the deepest depth of any input clip mapped to a depth the plugin supports"; "the only case where a plugin may set the output depth" is when both are 1
     - MUST
     - Default: EffectInstance.cpp:5404-5420 and :5482 (deepest connected depth → closest supported). The Depth\_ props are created read-only unless both support it (OfxImageEffectInstance.cpp:1276-1282). With the ``ePluginReadOnlyWarn`` policy (OfxHost.cpp:181-186) a write is still accepted, with a warning. checkMetadata then forces every input to the output depth (EffectInstance.cpp:5826-5829). So a plug-in that does not support multiple depths can still pick the common depth through the output clip.
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 6
     - `ofxClipPreferences.rst:161-170 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxClipPreferences.rst?plain=1#L161-L170>`__ components: "in all contexts, except for the general context, mandated input clips cannot have their component types remapped, nor can the output"; general output default "RGBA if any of the inputs is RGBA, otherwise A if the effect has any inputs, otherwise RGBA"
     - MUST/DEFAULT
     - Default (EffectInstance.cpp:5404-5420 and :5483-5505): the output gets the "most components" of the connected inputs, mapped to the closest supported value. Example: RGB + Alpha inputs give RGB, where the spec says A. The plug-in's component writes are accepted on any clip in any context; nothing enforces the Filter/Transition/Paint/Retimer restrictions. checkMetadata only maps to supported components (:5790-5792).
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 7
     - `ofxClipPreferences.rst:174-177 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxClipPreferences.rst?plain=1#L174-L177>`__ the host "must map the clip to RGBA in a manner transparent to the plugin" for unsupported input components
     - MUST
     - Done by ``getImage(... mapImageToClipPref ...)`` (OfxClipInstance.cpp:1001-1012 → EffectInstance::getImage converts to the clip components).
     - OK
   * - 8
     - `ofxClipPreferences.rst:186-199 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxClipPreferences.rst?plain=1#L186-L199>`__ and `ofxImageClip.rst:232-233 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxImageClip.rst?plain=1#L232-L233>`__ PAR: if the plug-in does not support multiple PARs, "the host must resample all images fed to that effect to agree with the output's PAR"; the common PAR "will be the smallest on all the inputs"
     - MUST
     - Natron never resamples PAR. checkMetadata (EffectInstance.cpp:5833-5838 and :5866-5871) relabels the input PAR to the output PAR and warns that the result is wrong. The GUI refuses such connections: ``Node::canConnectInput`` returns eCanConnectInput_differentPars (NodeInputs.cpp:750-764). The default PAR is the first connected input's PAR, not the smallest (EffectInstance.cpp:5385-5390 and :5453-5462).
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 9
     - `ofxClipPreferences.rst:197-199 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxClipPreferences.rst?plain=1#L197-L199>`__ with both at 1, each clip keeps its own PAR (defaults)
     - DEFAULT
     - EffectInstance.cpp:5456-5462: when ``multipleClipsPAR`` is true and any input is connected, every input gets ``inputPar`` (the first connected input's PAR), not its own. HS does this right (``par = clip->getAspectRatio()``, ofxhImageEffect.cpp setupClipPreferencesArgs). The wrong PAR is then pushed to the clip (OfxEffectInstance.cpp:1366), reported as kOfxImagePropPixelAspectRatio on that input's images (OfxClipInstance.cpp:1026 and :1498), and used for its pixel RoI (EffectInstance.cpp:982, :996).
     - WRONG

       Issue: `#1376 <https://github.com/NatronGitHub/Natron/issues/1376>`__
   * - 10
     - `ofxClipPreferences.rst:204-223 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxClipPreferences.rst?plain=1#L204-L223>`__, `ofxImageEffect.h:705-715 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L705-L715>`__ fielding: plug-in may set kOfxImageClipPropFieldOrder only if host SetableFielding=1
     - MAY (host may refuse)
     - Host says 0 (OfxHost.cpp:298), but the out-arg is writable (OfxImageEffectInstance.cpp:1207) and its value is stored (:1392) and propagated as metadata fielding. Natron renders unfielded anyway.
     - WRONG (spec-only)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 11
     - `ofxClipPreferences.rst:228-263 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxClipPreferences.rst?plain=1#L228-L263>`__ frame rate: SetableFrameRate=1 lets the plug-in change the output FPS, which "is effectively changing the number of frames in the output clip", nFrames' = nFrames\*FPS'/FPS
     - MUST (consequence)
     - The FPS is accepted (OfxImageEffectInstance.cpp:1390), but the output frame range is never remapped (OfxEffectInstance.cpp:1759-1828 has no FPS term). Input clip frame ranges are not converted to output time either (`ofxCoordSystem.rst:287-321 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxCoordSystem.rst?plain=1#L287-L321>`__; OfxClipInstance.cpp:432-447). Differing input FPS are refused at connection (NodeInputs.cpp:766-768) and warned about (EffectInstance.cpp:5851-5860). Either implement the mapping or advertise SetableFrameRate=0.
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 12
     - `ofxClipPreferences.rst:265-280 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxClipPreferences.rst?plain=1#L265-L280>`__ premult: the plug-in cannot change input premult; output default "premultiplied if any input premultiplied, otherwise unpremultiplied if any unpremultiplied, otherwise opaque"
     - DEFAULT
     - Default (EffectInstance.cpp:5393-5403, :5491-5496, :5513-5516): only RGBA inputs count; a disconnected optional input counts as premultiplied; RGBA/Alpha output with no premult input → premultiplied, not opaque. checkMetadata forces RGB→opaque and Alpha→premult (:5794-5801). This is deliberate (comment "else the output of generators may be opaque"), shared with HS ofxhImageEffect.cpp setDefaultClipPreferences. Input premult is not writable (good).
     - PARTIAL (deliberate, won't fix)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 13
     - `ofxImageEffect.h:328-331 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L328-L331>`__, `ofxClipPreferences.rst:52-75 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxClipPreferences.rst?plain=1#L52-L75>`__ ContinuousSamples default false, FrameVarying default 0
     - DEFAULT
     - ContinuousSamples default false (EffectInstance.cpp:5426). The FrameVarying default is ``node->hasAnimatedKnob()`` (:5425), not 0. The value is read back at OfxImageEffectInstance.cpp:1394-1396.
     - PARTIAL (spec-only)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 14
     - `ofxImageEffect.h:315-318 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L315-L318>`__ OfxImageClipPropPreferredColourspaces\_
     - —
     - See the colour audit (`#1104 <https://github.com/NatronGitHub/Natron/issues/1104>`__).
     - N/A here
   * - 15
     - `ofxImageEffect.h:335-343 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L335-L343>`__ return codes: ReplyDefault → defaults; Failed/ErrMemory → error
     - MUST
     - OfxImageEffectInstance.cpp:1328-1346. Invalid values are caught (:1411-1415).
     - OK
   * - 16
     - `ofxClipPreferences.rst:45-49 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxClipPreferences.rst?plain=1#L45-L49>`__ "these properties cannot animate over the duration of an effect"
     - MUST
     - Metadata are per node, not per time.
     - OK

Which preferences can a host refuse, and what Natron does:

- Depth: the plug-in may set it only if both sides support multiple depths. Natron makes the property read-only but accepts writes with a warning (3.5).

- PAR: the plug-in may set it only if both sides support multiple PARs. Natron forces the inputs to the output PAR, but accepts the plug-in's output PAR (3.8).

- Fielding: refusable when SetableFielding=0. Natron advertises 0 but accepts the value (3.10).

- Frame rate: refusable when SetableFrameRate=0. Natron advertises 1 and accepts it, without time mapping (3.11).

- Components of mandated clips and the output in non-General contexts: refusable. Natron accepts them (3.6).

- Input premultiplication: never settable. Natron gives the plug-in no way to set it (OK).

4. Clip and image properties (`ofxImageEffect.h:875-1457 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L875-L1457>`__, ofxImageClip.rst)
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

N: Engine/OfxClipInstance.cpp. HS: ofxhClip.cpp.

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron
     - verdict
   * - 1
     - `ofxImageEffect.h:890-917 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L890-L917>`__ clip PixelDepth / Components = value set by clip preferences
     - MUST
     - OfxEffectInstance.cpp:1361-1365 → ``setComponents``/``setPixelDepth``; OfxClipInstance.cpp:347-354.
     - OK
   * - 2
     - `ofxImageEffect.h:893-917 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L893-L917>`__ and `ofxImageEffect.h:945-970 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L945-L970>`__ the None value "implying a clip is unconnected" (mapped and unmapped)
     - DEFAULT
     - Unconnected clips report real values: unmapped depth = the plug-in's deepest supported depth (OfxClipInstance.cpp:192-206); unmapped components = the first input's components or RGBA (:233-256); mapped depth/components come from the default metadata (RGBA/float). kOfxImageClipPropConnected is correct (4.12), so plug-ins that check it are fine.
     - PARTIAL (spec-only)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 3
     - `ofxImageEffect.h:945-970 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L945-L970>`__ Unmapped depth/components = value before mapping
     - MUST
     - Input node's output depth and components (:158-191, :210-232).
     - OK
   * - 4
     - `ofxImageEffect.h:972-992 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L972-L992>`__ clip/image PreMultiplication
     - MUST
     - Clip: input node premult / own metadata (:266-280). Image: internal image premult (:1495).
     - OK
   * - 5
     - `ofxImageEffect.h:1050-1056 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1050-L1056>`__ clip/image PixelAspectRatio
     - MUST
     - Clip: metadata PAR (:361-369). Image: the clip PAR (:1026, :1498). Wrong for inputs when MultipleClipPARs=1 (3.9).
     - PARTIAL (see 3.9)

       Issue: `#1376 <https://github.com/NatronGitHub/Natron/issues/1376>`__
   * - 6
     - `ofxImageEffect.h:1058-1080 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1058-L1080>`__ FrameRate (input = clip FPS, output = mapped, instance = project) and UnmappedFrameRate
     - MUST
     - Input: nearest non-identity input's FPS (:404-424). Output: metadata FPS. Instance: project FPS (OfxImageEffectInstance.cpp:457-462). Unmapped: input node FPS (:509-520).
     - OK
   * - 7
     - `ofxImageEffect.h:1092-1116 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1092-L1116>`__ FrameRange, UnmappedFrameRange
     - MUST
     - Input node frame range or project range (:432-447, :528-540). No FPS mapping (3.11).
     - PARTIAL (only matters with FPS changes)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 8
     - `ofxImageEffect.h:931-943 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L931-L943>`__, `ofxImageClip.rst:69-73 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxImageClip.rst?plain=1#L69-L73>`__ ContinuousSamples (and "frame rate should then be set to 0")
     - MUST
     - :547-560 ``canRenderContinuously()``. The FPS is not zeroed for continuous clips.
     - PARTIAL (spec-only)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 9
     - `ofxImageEffect.h:1282-1291 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1282-L1291>`__ clip FieldOrder
     - MUST
     - :456-471 metadata fielding (always None in practice).
     - OK
   * - 10
     - `ofxImageEffect.h:1356-1367 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1356-L1367>`__, `ofxRendering.rst:207-340 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxRendering.rst?plain=1#L207-L340>`__ fields: FieldToRender, kOfxImagePropField, FieldExtraction, render twice at t and t+0.5
     - MUST if fielded
     - Natron always passes kOfxImageFieldNone (OfxEffectInstance.cpp:1849, :2052) and never renders fields. Existing issue `#7 <https://github.com/NatronGitHub/Natron/issues/7>`__.
     - N/A (no fielded clips; `#7 <https://github.com/NatronGitHub/Natron/issues/7>`__)
   * - 11
     - `ofxImageEffect.h:1118-1127 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1118-L1127>`__ Connected; "Any clip that is not optional will always be connected during a render action"
     - MUST
     - :477-503. Viewer renders are blocked when a mandatory input upstream is disconnected (ViewerInstance.cpp:705-730 ``checkTreeCanRender_internal``).
     - OK
   * - 12
     - `ofxImageEffect.h:919-929 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L919-L929>`__ kOfxImagePropUniqueIdentifier
     - MUST
     - :1497 = image hash key (hex).
     - OK
   * - 13
     - `ofxImageEffect.h:1293-1306 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1293-L1306>`__ kOfxImagePropData points to the bottom-left of the bounds
     - MUST
     - :1426-1451 ``pixelAt(bounds.left, bounds.bottom)``.
     - OK
   * - 14
     - `ofxImageEffect.h:1308-1322 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1308-L1322>`__ kOfxImagePropBounds "contained by RegionOfDefinition"
     - MUST
     - :1473-1476 = renderWindow ∩ image bounds. Containment is asserted at :1488-1490.
     - OK
   * - 15
     - `ofxImageEffect.h:1324-1338 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1324-L1338>`__ kOfxImagePropRegionOfDefinition in pixel coordinates
     - MUST
     - :1482-1486 ``rod.toPixelEnclosing(mipmapLevel, par)``.
     - OK
   * - 16
     - `ofxImageEffect.h:1340-1353 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1340-L1353>`__ RowBytes ≥ sizeof(pixel)\*width; may be negative
     - MUST/MAY
     - :1413-1454, always positive.
     - OK
   * - 17
     - `ofxImageEffect.h:1142-1165 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1142-L1165>`__ kOfxImageEffectPropRenderScale on images
     - MUST
     - :1374-1375; HS ofxhClip.cpp:766.
     - OK
   * - 18
     - `ofxImageClip.rst:41-49 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxImageClip.rst?plain=1#L41-L49>`__ bottom-to-top, packed pixels; ofxPixels.h structs
     - MUST
     - Natron's Image layout is bottom-up and packed.
     - OK
   * - 19
     - `ofxImageClip.rst:120-122 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxImageClip.rst?plain=1#L120-L122>`__ PAR, FPS, fielding, components and depth "cannot change from frame to frame"
     - MUST
     - Per-node metadata.
     - OK
   * - 20
     - `ofxImageClip.rst:155-161 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxImageClip.rst?plain=1#L155-L161>`__ images may be fetched in Render, and in InstanceChanged/EndInstanceChanged with UserEdited
     - MAY
     - Supported. OfxClipInstance.cpp:922-937 falls back to the RoD outside render; analysis is supported.
     - OK
   * - 21
     - `ofxImageClip.rst:163-176 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxImageClip.rst?plain=1#L163-L176>`__ temporal access: a plug-in must flag it; the host flags support
     - MUST
     - Host = 1. Natron does not police plug-ins that fetch other times without flagging it (not required).
     - OK

5. Actions
----------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron + HostSupport
     - verdict
   * - 5.1
     - `ofxImageEffect.h:100-148 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L100-L148>`__ GetRegionOfDefinition: inArgs Time, RenderScale, ThumbnailRender (opt.); outArgs "initially set by the host to the default RoD"; ReplyDefault → default RoD per context
     - MUST
     - HS ofxhImageEffect.cpp:2030-2117 pre-fills the outArgs with ``calcDefaultRegionOfDefinition`` (:1904-2025). That function covers every context, including General ("non optional + connected Source", :1961-1993) and Retimer (floor/ceil of SourceTime, :1995-2017). N OfxEffectInstance.cpp:1411-1546 calls it, with the RS probe.
     - OK
   * - 5.2
     - `ofxImageEffect.h:108-109 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L108-L109>`__ constant-size hosts need not call it
     - MAY
     - Natron calls it.
     - OK
   * - 5.3
     - `ofxProcessingArch.rst:67-71 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxProcessingArch.rst?plain=1#L67-L71>`__ "Hosts need to clip [infinite] RoDs to an appropriate rectangle, typically the project extent"
     - MUST
     - ``ifInfiniteclipRectToProjectDefault`` (OfxEffectInstance.cpp:1667) for RoIs, and the heuristic in EffectInstance.cpp:642.
     - OK
   * - 5.4
     - `ofxImageEffect.h:151-186 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L151-L186>`__ GetRegionsOfInterest: "The host must initialize all the RoIs in the outArgs ... to [the inArgs RoI]"
     - MUST
     - HS ofxhImageEffect.cpp:2186-2208 does that, except for plug-ins or clips without tile support, where the default is the input clip's RoD (deliberate, comment :2135-2145). N OfxEffectInstance.cpp:1609-1677 uses the results and skips empty RoIs.
     - PARTIAL (deliberate, won't fix)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 5.5
     - `ofxRendering.rst:39-51 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxRendering.rst?plain=1#L39-L51>`__ order RoD → clip → RoI → clip → render inputs → render
     - SHOULD
     - Natron renderRoI follows it. HS does not clamp the RoI to the input RoD (:2253-2258); Natron clamps when it renders the inputs.
     - OK
   * - 5.6
     - `ofxImageEffect.h:223-284 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L223-L284>`__ GetFramesNeeded: "need only ever be called if ... TemporalClipAccess"; every range initialised to time..time; ranges in pairs
     - MUST
     - HS :2546-2669 calls it only when ``temporalAccess()``, initialises each input to (t,t), and fails on an odd dimension (:2645-2646). N OfxEffectInstance.cpp:1680-1757.
     - OK
   * - 5.7
     - `ofxImageEffect.h:189-220 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L189-L220>`__ GetTimeDomain: "Only effects instantiated in the General Context can have this called on them"; default "the union of all the frame ranges of the non optional input clips", infinite if none
     - MUST/DEFAULT
     - N OfxEffectInstance.cpp:1770-1779 also calls it in the Generator context, and in the extension contexts Reader and Writer. The default (:1784-1826) takes the union over *all* connected inputs, optional ones included (deliberate GenArts workaround, comment :1799-1801). Infinite when there are no clips.
     - WRONG (Generator call) / PARTIAL (default)

       **Deliberate, won't fix.**

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 5.8
     - `ofxImageEffect.h:286-345 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L286-L345>`__ GetClipPreferences
     - —
     - See section 3.
     - —
   * - 5.9
     - `ofxImageEffect.h:347-394 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L347-L394>`__ IsIdentity: inArgs Time, FieldToRender, RenderWindow (pixels), RenderScale, ThumbnailRender (opt.); outArgs Name default "", Time default = inArgs time; OK → use that clip/time instead of rendering
     - MUST
     - HS :2675-2763 (outArgs time pre-set at :2739). N OfxEffectInstance.cpp:1830-1925 maps the clip to an input, with "Output" → -2 (time shift), and ignores unknown clip names.
     - OK
   * - 5.10
     - `ofxImageEffect.h:397-437 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L397-L437>`__ Render: inArgs Time, FieldToRender, RenderWindow, RenderScale, SequentialRenderStatus, InteractiveRenderStatus, RenderQualityDraft, NoSpatialAwareness, ThumbnailRender (opt.); pre: BeginSequenceRender called; post: EndSequenceRender called; must not return ReplyDefault
     - MUST
     - HS :1538-1664 sets all of them. N OfxEffectInstance.cpp:2041-2150 treats anything but OK as a failure. Begin/End bracket every renderRoI (EffectInstanceRenderRoI.cpp:1980-1993 and 2073-2083), or the whole sequence for sequential writers (OutputSchedulerThread.cpp:1086-1105 and 1171-1185).
     - OK (values: see 5.12-5.15)
   * - 5.11
     - `ofxImageEffect.h:440-517 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L440-L517>`__ Begin/EndSequenceRender inArgs: FrameRange, FrameStep, IsInteractive, RenderScale, SequentialRenderStatus, InteractiveRenderStatus, ThumbnailRender (opt.); Begin also NoSpatialAwareness (`ofxImageEffect.h:1184 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1184>`__)
     - MUST
     - HS Begin :1437-1536 has all of them. End (:1666-1758) has no kOfxImageEffectPropThumbnailRender, which `ofxImageEffect.h:495 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L495>`__ lists as optional on End. Fixed: End gets kOfxImageEffectPropThumbnailRender too, with the value of ``isThumbnailRender()`` ("false" in Natron).
     - OK (fixed by `NatronGitHub/openfx#203 <https://github.com/NatronGitHub/openfx/pull/203>`__, in libs/OpenFX since `#1507 <https://github.com/NatronGitHub/Natron/pull/1507>`__)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 5.12
     - `ofxImageEffect.h:451 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L451>`__ and `ofxImageEffect.h:490 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L490>`__ kOfxImageEffectPropFrameStep "generally ... 1 ... or 0.5"; `ofxImageEffect.h:1082-1090 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1082-L1090>`__ "can be any positive value"
     - MUST
     - The per-render End passes ``step = time``: EffectInstanceRenderRoI.cpp:2076 ``endSequenceRender_public(time, time, time, false, ...)`` (signature EffectInstance.cpp:4027-4029), while Begin passes 1 (:1988). Frame 0 gives a step of 0, and negative frames give negative steps. Since abff4c933. The scheduler's End for sequential writers passes 1 while its Begin passes ``frameStep`` (OutputSchedulerThread.cpp:1088-1089 vs 1180-1181).
     - WRONG

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 5.13
     - `ofxImageEffect.h:452 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L452>`__ and `ofxImageEffect.h:491 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L491>`__ kOfxPropIsInteractive "is this a single frame render due to user interaction in a GUI, or a proper full sequence render"
     - MUST
     - Per-render Begin: ``!appPTR->isBackground()`` (EffectInstanceRenderRoI.cpp:1988), so 1 even during a GUI disk render. Per-render End: ``false`` (:2076), which does not match the Begin. Scheduler: Begin ``false`` (OutputSchedulerThread.cpp:1090), End ``!isBackground()`` (:1182).
     - WRONG

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 5.14
     - `ofxImageEffect.h:770-783 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L770-L783>`__ InteractiveRenderStatus "1 - the instance is being rendered because a user is modifying parameters"
     - MUST
     - Per frame: correct (ParallelRenderArgs). The scheduler's Begin for sequential writers passes ``true`` for a disk render (OutputSchedulerThread.cpp:1092). Its End passes ``!isBackground()`` (:1184).
     - WRONG (sequential writers only)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 5.15
     - `ofxImageEffect.h:737-750 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L737-L750>`__ SequentialRenderStatus "1 - the host is currently rendering in a way so that it guarantees sequential rendering"; `ofxRendering.rst:177-179 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxRendering.rst?plain=1#L177-L179>`__
     - MUST
     - Disk renders and playback pass ``isSequentialRender = true`` to every node (OutputSchedulerThread.cpp:2269 and 2423). Frames are rendered concurrently (FFA scheduling, DefaultScheduler::getSchedulingPolicy :2528-2536; several render threads, :686-716). Only the writer's own render calls are ordered (:2359). Upstream nodes are told "sequential" while they render out of order and in parallel.
     - WRONG

       Issue: `#1375 <https://github.com/NatronGitHub/Natron/issues/1375>`__
   * - 5.16
     - `ofxRendering.rst:153-166 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxRendering.rst?plain=1#L153-L166>`__ plug-in value 1: "the host must render an instance on a single computer over it's entire frame range, from first to last"; `ofxRendering.rst:186-194 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxRendering.rst?plain=1#L186-L194>`__ "If eventually rendering the sequence, the host *must* ignore all frames rendered out of order and not cache them for use in the final result"
     - MUST
     - Natron honours it for writers only (NodeInputs.cpp:1750-1751 "Just take into account sequentiality for writers"; Node.cpp:5647; the cache lookup is skipped only for sequential writers, EffectInstanceRenderRoI.cpp:964). A non-writer plug-in with SequentialRender=1 is rendered in parallel and out of order, and its interactive (out-of-order) frames are served from the cache during the final render.
     - MISSING (non-writers)

       Issue: `#1375 <https://github.com/NatronGitHub/Natron/issues/1375>`__
   * - 5.17
     - `ofxImageEffect.h:1167-1179 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1167-L1179>`__ RenderQualityDraft in render calls
     - MAY
     - Passed in Begin, Render and End (HS :1478, :1600, :1726; N ``frameArgs->draftMode``).
     - OK
   * - 5.18
     - `ofxImageEffect.h:1181-1195 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1181-L1195>`__ NoSpatialAwareness in Begin/Render, "true" only for plug-ins that declared it
     - MAY
     - HS :653-666 and 1492-1493 / 1604-1605. ``isNoSpatialAwarenessRender()`` is always false: Natron never does such renders.
     - OK
   * - 5.19
     - `ofxImageEffect.h:1197-1222 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1197-L1222>`__ ThumbnailRender, optional, in RoD/RoI/FramesNeeded/ClipPrefs/IsIdentity/Begin/Render/End
     - MAY
     - HS adds it to RoD, RoI, FramesNeeded, IsIdentity, Begin and Render, but not to End or GetClipPreferences (Natron's own clip-prefs path, OfxImageEffectInstance.cpp:1316). It is always "false": Natron never opens a ``ThumbnailRenderScope`` (no match in Engine/Gui), even for node-graph previews. Existing issue `#1105 <https://github.com/NatronGitHub/Natron/issues/1105>`__. End has it since `NatronGitHub/openfx#203 <https://github.com/NatronGitHub/openfx/pull/203>`__. GetClipPreferences still lacks it: the upstream headers are inconsistent about it there. The value is always "false" (`#1105 <https://github.com/NatronGitHub/Natron/issues/1105>`__).
     - PARTIAL (`#1105 <https://github.com/NatronGitHub/Natron/issues/1105>`__): the End part is fixed by `NatronGitHub/openfx#203 <https://github.com/NatronGitHub/openfx/pull/203>`__ (in libs/OpenFX since `#1507 <https://github.com/NatronGitHub/Natron/pull/1507>`__); Natron never marks previews as thumbnail renders

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 5.20
     - `ofxImageEffect.h:1142-1165 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1142-L1165>`__ RenderScale in Render, Begin, End, IsIdentity, RoD, RoI (and InstanceChanged and interacts, other audits)
     - MUST
     - Present in all image-effect actions. Render-scale support is probed with RS 0.5 in RoD (OfxEffectInstance.cpp:1453-1512). A plug-in without RS support is rendered at 1 and downscaled. Related render-scale problems: `#191 <https://github.com/NatronGitHub/Natron/issues/191>`__, `#1123 <https://github.com/NatronGitHub/Natron/issues/1123>`__.
     - OK
   * - 5.21
     - `ofxProcessingArch.rst:196-210 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxProcessingArch.rst?plain=1#L196-L210>`__ a plug-in without tiles/multi-res: the host "can either choose to refuse to load a plugin, or, preferentially, pad images"; `ofxImageEffect.h:853 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L853>`__ "If a clip or plugin does not support tiled images, then the host should supply full RoD images"
     - SHOULD
     - Tiles: an output without tile support is rendered at full RoD (asserted, OfxEffectInstance.cpp:2078-2085). Inputs: the default RoI is the input RoD (5.4), but a plug-in RoI overrides it, so a non-tiled input clip can get less than its RoD. The input-clip flag only feeds the default. Multi-res: Natron refuses the connection (NodeInputs.cpp:743-748 ``checkCanConnectNoMultiRes``) instead of padding.
     - PARTIAL

       **Deliberate, won't fix.**

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__

6. Render thread safety and SMP (`ofxImageEffect.h:619-647 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L619-L647>`__, `ofxRendering.rst:72-124 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxRendering.rst?plain=1#L72-L124>`__, ofxThreadSafety.rst)
-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Mapping: N OfxEffectInstance.cpp:2164-2199 maps the property to ``eRenderSafetyUnsafe / InstanceSafe / FullySafe / FullySafeFrame``. FullySafeFrame means FullySafe plus HostFrameThreading. The value is cached per node (NodeMain.cpp:237, Node.cpp:423) and captured per render (ParallelRenderArgs.cpp:733). Enforcement: EffectInstanceRenderRoI.cpp:1555-1603.

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron
     - verdict
   * - 6.1
     - Unsafe: "only a single 'render' call can be made at any time among all instances"
     - MUST
     - A per-plugin mutex is held around the render (EffectInstanceRenderRoI.cpp:1593-1596, ``Plugin::getPluginLock()``; the mutex is created for Unsafe plug-ins, OfxHost.cpp:1058-1069). **However**, ``Node::getCurrentRenderThreadSafety()`` (Node.cpp:299-306) returns ``eRenderSafetyInstanceSafe`` whenever the user unticks the plug-in's multithreading checkbox in Preferences, even for an Unsafe plug-in. Two instances of an Unsafe plug-in can then render at the same time. Related GUI bug: the MT checkbox starts in the state of ``plugin->isActivated()``, not ``isMultiThreadingEnabled()`` (Gui/PreferencesPanel.cpp:588).
     - PARTIAL (WRONG when MT is disabled)

       Issue: `#1363 <https://github.com/NatronGitHub/Natron/issues/1363>`__
   * - 6.2
     - InstanceSafe: "any instance can have a single 'render' call at any one time"
     - MUST
     - Honoured. Renders run on a render clone (``getOrCreateRenderInstance``, :1583-1589), under a per-node mutex (:1591-1592), so at most one render per node. Note: clones are extra OFX instances (OfxEffectInstance.cpp:641-663). They share the parameter and clip objects with the main instance and get CreateInstance, but never InstanceChanged or SyncPrivateData. Plug-ins that keep private state updated in instanceChanged may render stale state on a clone. This also conflicts with SingleInstance (2.1).
     - OK (with caveat)
   * - 6.3
     - FullySafe: "any instance of a plugin can have multiple renders running simultaneously"
     - MAY
     - No lock (:1603).
     - OK
   * - 6.4
     - HostFrameThreading=1 (FullySafe): "the host will attempt to multithread an effect instance by calling it's render function simultaneously ... different renderWindow ... same frame"; 0: "the host only ever calls the render function once per frame"
     - MAY/MUST
     - FullySafeFrame splits the window into tiles when tiles are supported and threads are free (EffectInstanceRenderRoI.cpp:1305-1316, 1963-2005). With HostFrameThreading=0, Natron does not run concurrent tiles. It may still call render several times per frame, one after the other, with different windows (identity-based splitting ``optimizeRectsToRender`` :1319, and separate viewer requests).
     - PARTIAL (spec-only)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 6.5
     - `ofxThreadSafety.rst:27-35 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxThreadSafety.rst?plain=1#L27-L35>`__ the only actions on render threads are Begin/Render/End, IsIdentity, FramesNeeded, RoD and RoI
     - MUST
     - GetClipPreferences is main-thread only (asserted, OfxEffectInstance.cpp:1386). The render path calls only the listed actions.
     - OK
   * - 6.6
     - `ofxThreadSafety.rst:41-57 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxThreadSafety.rst?plain=1#L41-L57>`__ host locking of API calls: fine-grained, readers do not block readers
     - SHOULD
     - ``preferencesLock`` is a QReadWriteLock, read-locked in render actions and write-locked only when clip prefs are pushed (OfxEffectInstance.cpp:1332).
     - OK
   * - 6.7
     - `ofxThreadSafety.rst:59-62 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxThreadSafety.rst?plain=1#L59-L62>`__ abort the render thread when the user changes a value
     - SHOULD
     - ``abortAnyEvaluation`` (EffectInstance.cpp:5117); ``abort()`` → ``aborted()`` (OfxImageEffectInstance.cpp:1166-1169).
     - OK

7. OfxImageEffectSuiteV1 (`ofxImageEffect.h:1486-1749 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1486-L1749>`__)
---------------------------------------------------------------------------------------------------------------------------------------------------------------------------

HS ofxhImageEffect.cpp:3540-4006. Natron overrides ``getImage`` / ``getRegionOfDefinition`` (OfxClipInstance), ``abort`` and ``newMemoryInstance`` (OfxImageEffectInstance).

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - implementation
     - verdict
   * - 7.1
     - getPropertySet / getParamSet: OK or ErrBadHandle
     - MUST
     - Magic check plus dynamic_cast (around :3540-3616).
     - OK
   * - 7.2
     - clipDefine: pre "inside the describe in context action"
     - MUST
     - Works on descriptors only; returns kOfxStatErrExists for a duplicate name (:3618-3656).
     - OK
   * - 7.3
     - clipGetHandle: "Not a valid call in any of the describe actions"; the handle is valid for the instance's lifetime
     - MUST
     - :3801-3833 casts with ``reinterpret_cast<ImageEffect::Instance*>(effectBase)`` instead of ``dynamic_cast``, so ``if(effectInstance)`` is always true. A descriptor handle from a describe action is then used as an Instance (``getClip`` on the wrong object: undefined behaviour, likely a crash). ``*clip`` is not set to NULL on failure. Only a misbehaving plug-in triggers it.
     - WRONG

       Issue: `#1366 <https://github.com/NatronGitHub/Natron/issues/1366>`__
   * - 7.4
     - clipGetImage: region NULL → "at least the Region of Interest ... clipped to the clip's RoD"; region set → clipped to the RoD, "returned image will be at least as big as this region"
     - MUST
     - Region → RoI (EffectInstance.cpp:904-906); NULL → the RoI stored in TLS or the RoD (:907-948). **But** OfxClipInstance.cpp:942-971 reuses an image already fetched in the same render when components, mipmap, time and view match, **ignoring the region**. A second call with a larger region gets the first, smaller image.
     - WRONG

       Issue: `#1377 <https://github.com/NatronGitHub/Natron/issues/1377>`__
   * - 7.5
     - clipGetImage: "If clipGetImage is called twice with the same parameters, then two separate image handles will be returned, each of which must be release[d]"
     - MUST
     - The same reuse (:955-969) returns the *same* handle with ``addReference()``. Releasing it twice is safe (refcount), but the handles are not separate.
     - WRONG (spec-only)

       Issue: `#1377 <https://github.com/NatronGitHub/Natron/issues/1377>`__
   * - 7.6
     - clipGetImage returns: Failed when the image does not exist (plug-in assumes black), ErrMemory on OOM, ErrBadHandle on a bad clip
     - MUST
     - Null image → Failed (:3720-3725). Bad clip → BadHandle. Every exception, std::bad_alloc included, → kOfxStatErrBadHandle (:3730-3734), never ErrMemory. Fixed: a ``std::bad_alloc`` gives kOfxStatErrMemory, with NULL outputs, in clipGetImage, clipGetImagePlane, clipGetStereoscopicImage, clipGetRegionOfDefinition and imageMemoryLock.
     - OK (fixed by `NatronGitHub/openfx#202 <https://github.com/NatronGitHub/openfx/pull/202>`__, in libs/OpenFX since `#1507 <https://github.com/NatronGitHub/Natron/pull/1507>`__)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 7.7
     - clipReleaseImage: OK / BadHandle
     - MUST
     - :3776-3797.
     - OK
   * - 7.8
     - clipGetRegionOfDefinition: bounds in canonical coordinates; Failed if it cannot be determined
     - MUST
     - :4068-4101 returns Failed for an invalid RoD. N OfxClipInstance.cpp:564-695 returns 0 for a disconnected input or a failed upstream RoD (status OK with an empty RoD).
     - OK
   * - 7.9
     - abort: returns 0/1
     - MUST
     - :3869-3887 → N ``aborted()``. A bad handle returns kOfxStatErrBadHandle (non-zero), which the plug-in reads as "abort".
     - OK
   * - 7.10
     - imageMemoryAlloc: OK, or "kOfxStatErrMemory if there was not enough memory ..., memoryHandle is set to NULL"
     - MUST
     - N OfxImageEffectInstance.cpp:1172-1186 ``newMemoryInstance``: when the allocation fails (RamBuffer throws, caught by OfxMemory::alloc, OfxMemory.cpp), Natron shows a modal ``Dialogs::errorDialog`` (possibly from a render thread) and **still returns the object**. HS :3914-3917 then returns kOfxStatOK with a handle to NULL memory. Only the later imageMemoryLock fails (ErrMemory, :3955).
     - WRONG

       Issue: `#1365 <https://github.com/NatronGitHub/Natron/issues/1365>`__
   * - 7.11
     - imageMemoryFree: frees even with outstanding locks
     - MUST
     - ``freeMem`` clears the data and resets the lock count (PluginMemory.cpp freeMem) and the object is deleted (:3938-3941).
     - OK
   * - 7.12
     - imageMemoryLock: 16-byte aligned; locks nest; contents kept across relocks; ErrMemory → NULL
     - MUST
     - malloc (RamBuffer, CacheEntry.h:120) is 16-byte aligned on the supported 64-bit platforms. The lock count nests (PluginMemory.cpp lock). The buffer never moves. An exception → BadHandle (:3961-3964), not ErrMemory.
     - OK
   * - 7.13
     - imageMemoryUnlock: unlocking an unlocked handle has no effect
     - MUST
     - PluginMemory.cpp unlock clamps at 0.
     - OK

8. Misc instance properties (`ofxImageEffect.h:1224-1280 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L1224-L1280>`__, ofxCoordSystem.rst)
--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron
     - verdict
   * - 8.1
     - ProjectExtent (rooted at 0,0), ProjectSize, ProjectOffset, ProjectPixelAspectRatio, in canonical coordinates
     - MUST
     - OfxImageEffectInstance.cpp:361-433, from the project format.
     - OK
   * - 8.2
     - EffectDuration
     - MUST
     - :435-455, lifetime or project range.
     - OK
   * - 8.3
     - `ofxImageEffect.h:587-594 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L587-L594>`__ kOfxImageEffectPropPluginHandle "The plugin handle passed to the initial 'describe' action"
     - DEFAULT
     - HS :759 sets it to the ``OfxPlugin*`` struct, not to the descriptor handle passed to kOfxActionDescribe. The spec wording is ambiguous; long-standing HS behaviour.
     - PARTIAL (ambiguous)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - 8.4
     - `ofxCoordSystem.rst:282-285 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxCoordSystem.rst?plain=1#L282-L285>`__ a single temporal coordinate system
     - MUST
     - Natron has no per-node time offset.
     - OK

Verdict counts
--------------

.. list-table::
   :header-rows: 1
   :widths: 50 50
   :class: longtable

   * - verdict
     - count
   * - OK
     - 71
   * - PARTIAL
     - 20
   * - WRONG
     - 13
   * - MISSING
     - 2
   * - N/A
     - 4

Counted from the first word of each table row's verdict. Rows with mixed verdicts count once, by their first word: 5.7 counts as WRONG and 6.1 as PARTIAL.

Problems (MISSING / WRONG / PARTIAL), by severity
-------------------------------------------------

Crash or data loss
~~~~~~~~~~~~~~~~~~

1. **SingleInstance not honoured** (2.1, MISSING). Second check: no match for ``SingleInstance`` or ``isSingleInstance`` in Engine, Gui, App or Renderer at RB-2.6, and none in history (``git log -S isSingleInstance``). HS only has the getter (ofxhImageEffect.cpp:252). Natron also creates hidden render clones of InstanceSafe plug-ins (OfxEffectInstance.cpp:641), so a single-instance plug-in can get several live instances. **Issue:** `#1364 <https://github.com/NatronGitHub/Natron/issues/1364>`__.

2. **Unsafe plug-ins become InstanceSafe when the user disables multithreading for them** (6.1, PARTIAL/WRONG). Node.cpp:299-306 returns InstanceSafe whenever ``isMultiThreadingEnabled()`` is false, before the plug-in's own safety is read. Second check: ParallelRenderArgs.cpp:733 and 754, and ViewerInstance.cpp:400, all take the value through this getter. So the per-plugin lock at EffectInstanceRenderRoI.cpp:1593 is skipped, and different instances of a thread-unsafe plug-in render concurrently. Also, Gui/PreferencesPanel.cpp:588 starts the MT checkbox from ``isActivated()``. **Issue:** `#1363 <https://github.com/NatronGitHub/Natron/issues/1363>`__.

3. **clipGetHandle uses reinterpret_cast** (7.3, WRONG). A descriptor handle (misuse in describe) is treated as an Instance: undefined behaviour, likely a crash, instead of kOfxStatErrBadHandle. Second check: the other suite functions (``getParamSet``, ``abort``) use ``dynamic_cast``. ``imageMemoryAlloc`` has the same pattern (:3898-3899), but only after a magic check on the Base. **Issue:** `#1366 <https://github.com/NatronGitHub/Natron/issues/1366>`__.

4. **imageMemoryAlloc reports success when the allocation failed** (7.10, WRONG). Status OK with a NULL-backed handle, plus a modal error dialog that may be raised from a render thread. Second check: OfxMemory::alloc catches std::bad_alloc and returns false (Engine/OfxMemory.cpp). ``newMemoryInstance`` ignores that apart from the dialog (OfxImageEffectInstance.cpp:1178-1185). HS then returns OK (:3914-3917). **Issue:** `#1365 <https://github.com/NatronGitHub/Natron/issues/1365>`__.

Wrong results
~~~~~~~~~~~~~

5. **SequentialRenderStatus=1 while frames render in parallel and out of order** (5.15, WRONG), and **SequentialRender=1 not honoured for non-writer plug-ins** (5.16, MISSING). Interactive out-of-order frames are reused from the cache in the final render. Second check: ``isSequentialRender = true`` at OutputSchedulerThread.cpp:2269 and :2423. The scheduling policy is always FFA (:2528-2536). The sequential checks only look at writers (NodeInputs.cpp:1750, Node.cpp:5647, EffectInstanceRenderRoI.cpp:964). The instance-level SequentialRender value is ignored (2.6, PARTIAL). Related: `#1047 <https://github.com/NatronGitHub/Natron/issues/1047>`__ (ReadFFmpeg) only touches this indirectly. **Issue:** `#1375 <https://github.com/NatronGitHub/Natron/issues/1375>`__.

6. **Default input PAR is wrong when the plug-in supports multiple clip PARs** (3.9, WRONG). Every input gets the first connected input's PAR, which is then reported on its images and used for its pixel RoI. Second check: EffectInstance.cpp:5456-5462. HS ``setupClipPreferencesArgs`` does it right, but Natron bypasses it. Nothing later (checkMetadata :5833-5838) restores the real PAR when ``supportsMultipleClipPARs`` is true. **Issue:** `#1376 <https://github.com/NatronGitHub/Natron/issues/1376>`__.

7. **clipGetImage reuse ignores the requested region** (7.4, WRONG) and returns the same handle for identical calls (7.5, WRONG spec-only). Second check: the match key at OfxClipInstance.cpp:955-958 is components, mipmap, time and view only, and ``optionalBounds`` is not compared. **Issue:** `#1377 <https://github.com/NatronGitHub/Natron/issues/1377>`__.

8. **PAR mismatch is not resampled** (3.8, PARTIAL). Natron refuses the connection or relabels and warns. **Frame-rate change is not mapped to the output frame range** (3.11 and 4.7, PARTIAL) although SetableFrameRate=1. Second check: checkMetadata (EffectInstance.cpp:5826-5871), Node::canConnectInput (NodeInputs.cpp:750-768), and OfxEffectInstance::getFrameRange (no FPS term). No existing issue (`#775 <https://github.com/NatronGitHub/Natron/issues/775>`__ is unrelated). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

9. **Clip-preference context restrictions not enforced, depth write accepted although read-only** (3.5 and 3.6, PARTIAL). Natron accepts component changes on mandated clips and the output in Filter/Transition/Paint/Retimer, and output-depth writes from plug-ins without multiple-depth support (ePluginReadOnlyWarn, OfxHost.cpp:181-186). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

10. **Non-tiled input clips can get less than their RoD; no padding for non-multires plug-ins** (5.21 and 5.4, PARTIAL, deliberate per the HS comment :2135-2145). Related to `#254 <https://github.com/NatronGitHub/Natron/issues/254>`__ (isIdentity vs framesNeeded) only loosely. **Deliberate, won't fix.** Listed in `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__ for completeness.

Spec-only
~~~~~~~~~

11. **GetClipPreferences is called with mandatory clips disconnected** (3.2, WRONG, deliberate since 90d644baa). Second check: Node.cpp:6133. Neither ``areAllNonOptionalClipsConnected`` nor ``checkClipConnectionStatus`` has a caller. **Deliberate, won't fix.** Listed in `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__ for completeness.

12. **Begin/EndSequenceRender argument inconsistencies** (5.12-5.14, WRONG): **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

    - The per-render End passes FrameStep = time (EffectInstanceRenderRoI.cpp:2076, since abff4c933).

    - IsInteractive differs between Begin (``!isBackground``) and End (``false``).

    - For sequential writers, the scheduler's Begin passes InteractiveRenderStatus=1 for a disk render, and FrameStep/IsInteractive differ between its Begin and End (OutputSchedulerThread.cpp:1088-1092 vs 1180-1184).

13. **GetTimeDomain is called in the Generator context; its default includes optional inputs** (5.7, WRONG/PARTIAL). Both are deliberate (OfxEffectInstance.cpp:1767-1779 and 1799-1801). **Deliberate, won't fix.** Listed in `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__ for completeness.

14. **The instance kOfxPropType is kOfxTypeImageEffect instead of kOfxTypeImageEffectInstance** (1.6, WRONG). Second check: no match for kOfxTypeImageEffectInstance in HS src or in Natron. The value comes from the descriptor (ofxhImageEffect.cpp:85, copied at :724). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

15. **Fielding accepted although SetableFielding=0** (3.10, WRONG). Fields are not supported at all (`#7 <https://github.com/NatronGitHub/Natron/issues/7>`__). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__, `#7 <https://github.com/NatronGitHub/Natron/issues/7>`__.

16. **Unconnected clips report real components and depths instead of None** (4.2, PARTIAL). The FPS is not zeroed for continuous clips (4.8, PARTIAL). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

17. **Default output components, premultiplication and FrameVarying differ from the spec defaults** (3.6, 3.12 and 3.13, PARTIAL). Premultiplication is deliberate and shared with HS. The premultiplication default is deliberate, won't fix. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

18. **ThumbnailRender is always "false"**: node previews are not marked, and the property is missing from the End and GetClipPreferences inArgs (5.11 and 5.19, PARTIAL). Existing issue `#1105 <https://github.com/NatronGitHub/Natron/issues/1105>`__. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__, `#1105 <https://github.com/NatronGitHub/Natron/issues/1105>`__.

19. **Unadvertised Paint/Retimer contexts are used; no fallback on kOfxStatErrMissingHostFeature; mandated Transition/SourceTime params are not host-driven** (1.2, 1.3 and 1.7, PARTIAL). **Issue:** `#1383 <https://github.com/NatronGitHub/Natron/issues/1383>`__, `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

20. **clipGetImage and imageMemoryLock map std::bad_alloc to ErrBadHandle instead of ErrMemory** (7.6, PARTIAL). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

21. **HostFrameThreading=0 still allows several sequential render calls per frame** (6.4, PARTIAL). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

22. **kOfxImageEffectPropPluginHandle is the OfxPlugin\*, not the describe handle** (8.3, PARTIAL, ambiguous wording). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

NEXT items met: HostSupport descriptor inheritance of SupportsTiles (#177 upstream). It is already correct in this HostSupport (2.8). kOfxImageEffectPluginPropObsolete: plug-ins that set it are loaded but not offered for new nodes (`#1139 <https://github.com/NatronGitHub/Natron/issues/1139>`__); since `#1491 <https://github.com/NatronGitHub/Natron/issues/1491>`__ it replaces the Natron extension kNatronOfxImageEffectPropDeprecated, which Natron no longer reads.
