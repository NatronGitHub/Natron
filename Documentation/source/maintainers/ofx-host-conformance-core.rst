.. _ofx-host-conformance-core:

OFX host conformance: core API
==============================

This page is part of the :ref:`OpenFX 1.5.1 host conformance audit <ofx-host-conformance>`.
It covers the actions, properties and suites that every OFX API shares.

**Spec scope.** The headers ``ofxCore.h``, ``ofxProperty.h``, ``ofxMemory.h``,
``ofxMultiThread.h``, ``ofxMessage.h``, ``ofxProgress.h``, ``ofxTimeLine.h`` and
``ofxDialog.h``; the Reference pages ``ofxCoreAPI.rst``, ``ofxHostStruct.rst``,
``ofxLoadingSequence.rst``, ``ofxPluginStruct.rst``, ``ofxStatusCodes.rst``,
``ofxStructure.rst``, ``ofxThreadSafety.rst``, ``ofxImageEffectActions.rst``, the host
section of ``ofxPropertiesByObject.rst`` and the suite reference pages; and the Guide page
``ofxExample1_Basics.rst``.

**Code references.** ``HS/...`` is ``libs/OpenFX/HostSupport/src/...``. Other paths are in the
Natron tree.

Verdicts: **OK**; **MISSING** (not implemented); **WRONG** (implemented, but contradicts the spec); **PARTIAL** (partly implemented, or only in some cases); **N/A** (does not apply to Natron); **NEXT** (seen, but out of scope of this audit). The level column gives the strength of the spec statement (MUST, SHOULD, MAY) or DEFAULT for a default value. Spec references link to the upstream ``OFX_Release_1.5.1`` tag. Code references are plain paths with line numbers at the audited commits (Natron RB-2.6 at 7dea437a6, HostSupport at 93c21ed7); they are not updated when the code moves. Each row or problem that is not OK links to the issue that tracks it; items marked **deliberate, won't fix** are known deviations that Natron keeps on purpose.

1. ofxCore.h: OfxHost, OfxPlugin, entry points, loading sequence
----------------------------------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron + HostSupport
     - verdict
   * - C1
     - `ofxCore.h:568-577 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L568-L577>`__ ``OfxSetHost``: "First thing host should call… host must check if the plugin implemented it"; ofxLoadingSequence.rst step 2 "(if implemented by plugin and host)"
     - MAY (optional for the host; if the host does it, it must call it first and treat kOfxStatFailed as "skip silently")
     - Never looked up: no ``OfxSetHost`` anywhere in HS or Engine/Gui. ``PluginBinary::loadPluginInfo`` (HS/ofxhPluginCache.cpp:326-380) only resolves ``OfxGetNumberOfPlugins`` and ``OfxGetPlugin``.
     - MISSING

       Issues: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__, `#1107 <https://github.com/NatronGitHub/Natron/issues/1107>`__
   * - C2
     - ofxLoadingSequence.rst 3-4.1: call OfxGetNumberOfPlugins, then OfxGetPlugin(i)
     - MUST
     - HS/ofxhPluginCache.cpp:345-366. A negative count is clamped to 0; NULL or incomplete structs are skipped.
     - OK
   * - C3
     - ofxLoadingSequence.rst 4.2-4.3, ofxPluginStruct.rst "If a host does not support the given plug-in type, or … version it should simply ignore that plug-in"
     - SHOULD
     - ``findApiHandler(pluginApi, apiVersion)`` (HS/ofxhPluginCache.cpp:373). Only ``OfxImageEffectPluginAPI`` v1 is accepted (HS/ofxhImageEffectAPI.cpp:317).
     - OK
   * - C4
     - ofxLoadingSequence.rst 4.5, `ofxCore.h:118-137 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L118-L137>`__ ``setHost``: "The very first function called… \\pre nothing else has been called"
     - MUST
     - Called lazily in the ``PluginHandle`` ctor (HS/ofxhPluginCache.cpp:418), right before Load. The Guide allows this ("If the host decides it wants to use one of the plugins… call setHost"). But a second ``PluginHandle`` calls setHost again after Load/Describe/Unload already ran (see C9).
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - C5
     - `ofxCore.h:56-59 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L56-L59>`__ fetchSuite: same pointer on repeated calls; "recommended… same host and suite pointers to all plugins in the same shared lib"
     - MUST / SHOULD
     - The suites are static structs. A single ``OfxHost`` gives the same ``&_host`` to every plug-in (HS/ofxhHost.cpp:110-123).
     - OK
   * - C6
     - `ofxCore.h:60-63 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L60-L63>`__, ofxCoreAPI.rst "If the host does not implement a requested suite, or version… return NULL"
     - SHOULD
     - Engine/OfxHost.cpp:1193-1201 → HS/ofxhImageEffect.cpp:4798-4912 → HS/ofxhHost.cpp:126-137, which return NULL.
     - OK
   * - C7
     - ofxHostStruct.rst "host… a property set handle that holds a set of properties which describe the host" (read only, see §8)
     - MUST
     - The host property set exists. Its base properties are not read-only (see H1).
     - PARTIAL (see H1)

       Issue: `#1367 <https://github.com/NatronGitHub/Natron/issues/1367>`__
   * - C8
     - ofxPluginStruct.rst plug-in versioning: new instances use the greatest major.minor; loading a setup uses the same major with the greatest minor
     - SHOULD
     - ``confirmPlugin`` keeps one plug-in per (id, major) with the highest minor (HS/ofxhImageEffectAPI.cpp:589-640). New nodes use the highest version; projects use the same major, else the closest higher major (Engine/AppManager.cpp:2315-2400).
     - OK
   * - C9
     - `ofxCore.h:174 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L174>`__ kOfxActionLoad \\post "This action will not be called again while the binary containing the plug-in remains loaded"; `ofxCore.h:251 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L251>`__ Unload \\post "No other actions will be called"; `ofxCore.h:212 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L212>`__ Describe "will not be called again, unless it fails"
     - MUST (header). The Guide (`ofxExample1_Basics.rst:446 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Guide/ofxExample1_Basics.rst?plain=1#L446>`__, 779-783) instead allows Load again after Unload "even if it doesn't dynamically unload the binary".
     - When the cache is dirty (first run, or the binary changed), the scan runs Load → Describe → Unload (``PluginCache::loadFromPlugin``, HS/ofxhImageEffectAPI.cpp:413-490). The binary stays dlopen'ed: ``loadPluginInfo`` keeps a ref "once per session" (HS/ofxhPluginCache.cpp:334-340). The first instance creation then runs setHost → Load → Describe again on the same loaded binary (``getPluginHandle``, HS/ofxhImageEffectAPI.cpp:164-216).
     - PARTIAL (the header is violated, the Guide is satisfied)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - C10
     - `ofxCore.h:179-183 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L179-L183>`__ Load kOfxStatFailed: "no further actions… Do not create an entry in the host's UI"
     - MUST / SHOULD
     - On failure, no further actions are sent (HS/ofxhImageEffectAPI.cpp:183-186, 439-442). Describe never runs, so there are no contexts and the plug-in is not registered (Engine/OfxHost.cpp:990).
     - OK
   * - C11
     - `ofxCore.h:183 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L183>`__ the plug-in may return 0 from OfxGetNumberOfPlugins, or kOfxStatFailed from OfxSetHost
     - MUST (host tolerates it)
     - 0 is handled. OfxSetHost does not apply (C1).
     - OK / N/A
   * - C12
     - `ofxCore.h:157-160 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L157-L160>`__ Load: handle, inArgs and outArgs "should be set to NULL"; same for Unload (`ofxCore.h:241-242 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L241-L242>`__)
     - MUST
     - ``mainEntry(kOfxActionLoad, 0, 0, 0)`` and Unload likewise (HS/ofxhImageEffectAPI.cpp:181, 307, 434, 480).
     - OK
   * - C13
     - `ofxCore.h:203-205 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L203-L205>`__ Describe: inArgs and outArgs NULL; handle is the descriptor
     - MUST
     - HS/ofxhImageEffectAPI.cpp:196, 449.
     - OK
   * - C14
     - `ofxCore.h:212-216 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L212-L216>`__ Describe \\post: the descriptor handle is valid until the end of Unload
     - MUST
     - ``_baseDescriptor`` is deleted after ``unload()`` (HS/ofxhImageEffectAPI.cpp:99-107).
     - OK
   * - C15
     - `ofxCore.h:226 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L226>`__ Describe kOfxStatErrMissingHostFeature: "the plugin will be unloaded and ignored"
     - MUST
     - Unload is sent and the handle dropped (HS/ofxhImageEffectAPI.cpp:200-213, 455-463). The plug-in is not registered (no contexts).
     - OK
   * - C16
     - `ofxCore.h:228 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L228>`__ Describe kOfxStatErrMemory: "may be called again after a memory purge"
     - MAY
     - Not retried.
     - OK (MAY)
   * - C17
     - `ofxCore.h:219-222 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L219-L222>`__ DescribeInContext "will be called once for each context that the host and plug-in mutually support. If a plug-in does not report to support any context supported by host, host should not enable the plug-in."
     - MUST / SHOULD
     - Called lazily and for **one** context only (``getContext_internal``, Engine/OfxHost.cpp:585-668, 671-711). The plug-in's contexts are never intersected with the host's ``kOfxImageEffectPropSupportedContexts`` (Generator, Filter, General, Transition; Engine/OfxHost.cpp:286-293). Natron uses Reader/Writer/Paint/Tracker contexts it doesn't advertise. A plug-in whose only contexts the host doesn't support (e.g. Retimer only) is still listed (Engine/OfxHost.cpp:990 only skips *zero* contexts), and creating it then fails.
     - PARTIAL

       Issue: `#1383 <https://github.com/NatronGitHub/Natron/issues/1383>`__
   * - C18
     - `ofxCore.h:245-249 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L245-L249>`__ Unload \\pre "all instances of a plugin have been destroyed"
     - MUST
     - Unload runs in ``~ImageEffectPlugin`` → ``~PluginBinary`` → ``clearPluginCache`` (Engine/OfxHost.cpp:190-198), from ``~AppManagerPrivate``, after ``~AppManager`` has quit every AppInstance (Engine/AppManager.cpp:469-540).
     - OK
   * - C19
     - `ofxCore.h:256 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L256>`__ Unload kOfxStatErrFatal "the program will be forced to quit"
     - (describes consequence)
     - Ignored.
     - N/A
   * - C20
     - ofxCoreAPI.rst "Label strings… A host must be aware that when it unloads a plug-in all such pointers will be invalid"; Guide `ofxCore.h:80-83 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L80-L83>`__ copy strings out of OfxPlugin
     - MUST / SHOULD
     - HS copies identifiers and properties into std::string.
     - OK
   * - C21
     - ofxCoreAPI.rst value strings "scope… until the next OFX API function is called" (host-returned strings)
     - MUST
     - Properties return ``c_str()`` of stored std::strings, valid at least until the next set.
     - OK
   * - C22
     - ofxCoreAPI.rst / ofxStatusCodes.rst: actions return kOfxStatReplyDefault if not trapped
     - (host must accept it)
     - ReplyDefault is accepted as success for Load, Describe, DescribeInContext and CreateInstance (HS/ofxhImageEffectAPI.cpp:183, 198, 259; HS/ofxhImageEffect.cpp:1173-1176; Engine/OfxImageEffectInstance.cpp:214-232).
     - OK

2. ofxCore.h: instance actions, ordering and pre/post conditions
----------------------------------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron + HostSupport
     - verdict
   * - A1
     - `ofxCore.h:330-338 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L330-L338>`__ CreateInstance \\pre "the instance is fully constructed… if the values are being loaded from an old setup, that load should have taken place before the create instance action"
     - MUST
     - ``populate()``, knobs, ``loadKnobs(serialization)`` and ``setValuesFromSerialization`` all run before ``createInstanceAction()`` (Engine/OfxEffectInstance.cpp:450-553). ``knobChanged`` is suppressed until ``_imp->initialized`` (Engine/OfxEffectInstance.cpp:2680-2682).
     - OK
   * - A2
     - `ofxCore.h:326-327 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L326-L327>`__ CreateInstance inArgs and outArgs NULL
     - MUST
     - Engine/OfxImageEffectInstance.cpp:221.
     - OK
   * - A3
     - `ofxCore.h:349-352 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L349-L352>`__ CreateInstance kOfxStatFailed: "the host should destroy the instance handle and not attempt to proceed further"; ErrMemory: may retry
     - SHOULD / MAY
     - Throws, ``_imp->effect.reset()``. ``_created`` is false, so no DestroyInstance is sent (Engine/OfxEffectInstance.cpp:563-640). No retry.
     - OK
   * - A4
     - `ofxCore.h:340-342 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L340-L342>`__ instance pointer valid until DestroyInstance
     - MUST
     - Yes. Render clones each get their own CreateInstance/DestroyInstance (Engine/OfxEffectInstance.cpp:643-660).
     - OK
   * - A5
     - `ofxCore.h:369-372 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L369-L372>`__ DestroyInstance \\pre "CreateInstance has been called… the instance has not had any of its members destroyed yet"
     - MUST
     - ``~OfxEffectInstance`` → ``destroyInstanceAction()`` (Engine/OfxEffectInstance.cpp:632-638). This runs before the KnobHolder base destructor (knobs alive) and before HS deletes the clips (HS/ofxhImageEffect.cpp:1029-1045). It is sent only if ``_created`` (HS/ofxhImageEffect.cpp:1181-1199).
     - OK
   * - A6
     - `ofxCore.h:378-381 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L378-L381>`__ "the host should continue destruction of the instance regardless"
     - SHOULD
     - ``_created=false`` whatever the status (HS/ofxhImageEffect.cpp:1193-1196).
     - OK
   * - A7
     - `ofxCore.h:396-398 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L396-L398>`__ InstanceChanged "All change actions are bracketed by a pair of BeginInstanceChanged and EndInstanceChanged"; `ofxCore.h:429-432 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L429-L432>`__ \\pre Begin called, \\post End will be called; `ofxCore.h:475-481 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L475-L481>`__
     - MUST
     - **Clip changes:** bracketed (Engine/OfxEffectInstance.cpp:1247-1249). **Param changes (user, undo/redo, reset, Python, slave refresh, and plug-in edits):** unbracketed. ``knobChanged`` → ``paramInstanceChangedAction`` only (Engine/OfxEffectInstance.cpp:2720, 2726). ``beginKnobsValuesChanged_public``/``endKnobsValuesChanged_public`` (Engine/Knob.cpp:6211-6228) have no callers anywhere in the tree. The plug-in-edit path in HS (``Instance::paramChangedByPlugin``, which brackets, HS/ofxhImageEffect.cpp:3497-3511) is overridden by a no-op in Natron (Engine/OfxImageEffectInstance.cpp:1548-1556).
     - **WRONG (**\ `#909 <https://github.com/NatronGitHub/Natron/issues/909>`__\ **, re-verified)**
   * - A8
     - `ofxCore.h:405-424 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L405-L424>`__ InstanceChanged inArgs: kOfxPropType, kOfxPropName, kOfxPropChangeReason, kOfxPropTime, kOfxImageEffectPropRenderScale
     - MUST
     - Params: HS/ofxhImageEffect.cpp:1238-1258. Clips: HS/ofxhClip.cpp:669-685 (scale 1,1).
     - OK
   * - A9
     - `ofxCore.h:409-419 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L409-L419>`__ / `ofxCore.h:753-762 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L753-L762>`__ kOfxPropChangeReason values: UserEdited "includes undo/redos, resets and loading values from files or presets"; PluginEdited; Time
     - MUST
     - Mapped in ``natronValueChangedReasonToOfxValueChangedReason`` (Engine/OfxEffectInstance.cpp:2593-2614). Values loaded with a project are set before CreateInstance, so no InstanceChanged is sent. Sending it after project load is a NEXT item, out of scope.
     - OK
   * - A10
     - `ofxCore.h:428-429 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L428-L429>`__ InstanceChanged \\pre CreateInstance has been called
     - MUST
     - ``!_imp->initialized → return`` (Engine/OfxEffectInstance.cpp:2680). HS skips plug-in edits made during CreateInstance (HS/ofxhImageEffect.cpp:3499-3504).
     - OK
   * - A11
     - `ofxCore.h:451-481 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L451-L481>`__ Begin/EndInstanceChanged inArgs: kOfxPropChangeReason; \\post "For BeginInstanceChanged, InstanceChanged will be called at least once"
     - MUST
     - The clip bracket has the reason (HS/ofxhImageEffect.cpp:1203-1222, 1288-1306) and exactly one InstanceChanged. For params, see A7.
     - OK (clips) / see A7

       Issue: `#909 <https://github.com/NatronGitHub/Natron/issues/909>`__
   * - A12
     - ofxThreadSafety.rst "Recursive Actions": the host may issue InstanceChanged recursively for a param set inside InstanceChanged
     - MAY
     - Yes: main thread, recursion level tracked (Engine/OfxEffectInstance.cpp:2702-2727). InstanceChanged runs only on the main thread (Engine/Knob.cpp:6237-6238).
     - OK
   * - A13
     - `ofxCore.h:262-273 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L262-L273>`__ PurgeCaches: "may be passed… in low memory situations… should be called after EndSequenceRender"
     - MAY / SHOULD
     - Sent on the user "Clear all caches" (Engine/AppInstance.cpp:2064), and on file param changes and reloads (Engine/KnobFile.cpp:75, 139; Gui/KnobGuiFile.cpp:289; Gui/KnobUndoCommand.cpp:788). Not on low memory and not after EndSequenceRender. Only the main instance gets it, not the render clones. ``OfxEffectInstance::purgeCaches`` also sends SyncPrivateData right after (Engine/OfxEffectInstance.cpp:2768-2810).
     - OK (MAY)
   * - A14
     - `ofxCore.h:283-291 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L283-L291>`__ PurgeCaches returns may include kOfxStatErrFatal and kOfxStatFailed
     - (host must tolerate them)
     - ``assert(stat == kOfxStatOK || stat == kOfxStatReplyDefault)`` after both PurgeCaches and SyncPrivateData (Engine/OfxEffectInstance.cpp:2781, 2808). A legal kOfxStatFailed aborts a debug build. Release builds define NDEBUG (CMakeLists.txt:353, global.pri).
     - WRONG (debug builds only)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - A15
     - `ofxCore.h:297-301 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L297-L301>`__ SyncPrivateData "generally occurs when an effect is about to be saved or copied"; \\post private state can be reconstructed from the params
     - SHOULD
     - ``NodeSerialization`` ctor → ``syncPrivateData_other_thread()`` (Engine/NodeSerialization.cpp:66-70). That emits a Qt signal connected with AutoConnection (Engine/OfxEffectInstance.cpp:312, 319). A main-thread save or copy runs the sync synchronously. **Autosave** runs in ``QtConcurrent::run`` (Engine/Project.cpp:768): the signal is queued, so the node is serialized *before* the sync runs on the main thread. The sync is also skipped entirely while a dialog is shown (Engine/OfxEffectInstance.cpp:2876-2889). ``kOfxPropParamSetNeedsSyncing`` is honoured (HS/ofxhImageEffect.cpp:1320-1341).
     - PARTIAL

       Issue: `#1368 <https://github.com/NatronGitHub/Natron/issues/1368>`__
   * - A16
     - `ofxCore.h:496-519 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L496-L519>`__ BeginInstanceEdit "called when an instance is *first* actively edited by a user, ie: an interface is open"; \\post "EndInstanceEdit will be called when the last editor is closed"
     - MUST
     - Begin: sent once when the settings panel is first created (Gui/NodeGui.cpp:416-430 → 734-741 → Engine/Node.cpp:2650 → Engine/OfxEffectInstance.cpp:2867-2873). It is guarded by ``_wasBeginEditCalled`` at the other callers (Gui/CurveEditor.cpp:888, Gui/DopeSheetHierarchyView.cpp:1117, Gui/NodeGraph25.cpp:124). End: only in ``NodeGui::deactivate`` (node deleted; Gui/NodeGui.cpp:2099-2101). It is not sent when the panel is closed, nor before DestroyInstance on project close or quit.
     - PARTIAL

       Issue: `#1381 <https://github.com/NatronGitHub/Natron/issues/1381>`__
   * - A17
     - `ofxCore.h:536-540 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L536-L540>`__ EndInstanceEdit \\pre "BeginInstanceEdit has been called on the instance handle"
     - MUST
     - ``deactivate()`` calls End unconditionally: for a node whose panel was never opened (Begin never sent), and again after undo-delete then delete. ``_wasBeginEditCalled`` is never reset, so Begin is not re-sent on reactivation.
     - WRONG

       Issue: `#1381 <https://github.com/NatronGitHub/Natron/issues/1381>`__
   * - A18
     - `ofxCore.h:613-621 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L613-L621>`__ kOfxPropIsInteractive "Indicates if a host is actively editing the effect with some GUI", instance, read only
     - MUST (valid value)
     - Always 0. ``OfxHost::newInstance`` passes ``interactive=false`` (Engine/OfxHost.cpp:495). Nothing updates it on BeginInstanceEdit, even in the GUI app (no other ``kOfxPropIsInteractive`` in Engine/Gui).
     - WRONG

       Issue: `#1382 <https://github.com/NatronGitHub/Natron/issues/1382>`__

3. ofxCore.h: general properties
--------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron + HostSupport
     - verdict
   * - P1
     - `ofxCore.h:593-603 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L593-L603>`__ kOfxPropAPIVersion "int X N… version… implemented by a host"
     - DEFAULT / MUST
     - {1,4} (Engine/OfxHost.cpp:261-262). Natron honestly claims 1.4, not 1.5.
     - OK (tracked as `#1107 <https://github.com/NatronGitHub/Natron/issues/1107>`__ "Advertise OFX API 1.5")
   * - P2
     - `ofxCore.h:648-661 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L648-L661>`__ kOfxPropType valid values incl. kOfxTypeImageEffectHost, kOfxTypeImageEffectInstance, kOfxTypeParameterInstance (`ofxImageEffect.h:80-81 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L80-L81>`__, `ofxParam.h:20-21 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L20-L21>`__)
     - DEFAULT
     - Host: kOfxTypeImageEffectHost (HS/ofxhHost.cpp:88). Effect **instances** copy the descriptor set (``Base(other)``, HS/ofxhImageEffect.cpp:742) and keep "OfxTypeImageEffect". Param **instances** keep "OfxTypeParameter" (HS/ofxhParam.cpp:277, 706-708). Nothing sets the ``*Instance`` values (grep in HS and Engine).
     - WRONG

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - P3
     - `ofxCore.h:625-636 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L625-L636>`__ kOfxPluginPropFilePath, effect descriptor (read only): bundle path
     - MUST
     - ``getBundlePath()`` (HS/ofxhImageEffect.cpp:525, 533, 541), read only (HS/ofxhImageEffect.cpp:109).
     - OK
   * - P4
     - `ofxCore.h:638-646 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L638-L646>`__ kOfxPropInstanceData, Default NULL, "unique to each plug-in instance, so two instances … do not share the same data pointer"
     - DEFAULT / MUST
     - Main instances start at NULL. **Render clones** are copy-constructed (``Instance(const Instance&)`` → ``Base(other)`` copies the property set, HS/ofxhImageEffect.cpp:780-797; HS/ofxhPropertySuite.cpp:578-592). So before their CreateInstance, clones carry the main instance's pointer (Engine/OfxEffectInstance.cpp:643-653).
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - P5
     - `ofxCore.h:663-670 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L663-L670>`__ kOfxPropName (host): "globally unique name of the application" (`ofxPropertiesByObject.rst:19-20 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxPropertiesByObject.rst?plain=1#L19-L20>`__)
     - MUST
     - It is the user setting ``Settings::getHostName()``, and it is overridden per plug-in to spoof Nuke or Vegas for known vendors (Engine/OfxHost.cpp:258-259, 391-482). This is a deliberate compatibility measure. Note: ``getPluginContextAndDescribe`` sets ``loadingPluginVersionMinor = getVersionMajor()`` (Engine/OfxHost.cpp:676, typo), which is harmless because the proxy ignores versions.
     - PARTIAL (deliberate, won't fix)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - P6
     - `ofxCore.h:672-695 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L672-L695>`__ kOfxPropVersion and kOfxPropVersionLabel (host, read only); "Default - none, the host needs to set this"
     - MUST
     - {MAJOR,MINOR,REVISION} and NATRON_VERSION_STRING (Engine/OfxHost.cpp:263-266).
     - OK (read-only: see H1)
   * - P7
     - `ofxCore.h:708-751 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L708-L751>`__ kOfxPropLabel default = kOfxPropName; "resetting this will also reset kOfxPropShortLabel and kOfxPropLongLabel"; Short/Long "initially kOfxPropName, but will be reset if kOfxPropLabel is changed"
     - DEFAULT / MUST
     - Effect descriptor labels default to "" (HS/ofxhImageEffect.cpp:86-89). Reading them falls back to the label on the host side (HS/ofxhImageEffect.cpp:206, 229), but a plug-in reading Short/Long gets "". ``propReset(kOfxPropLabel)`` and setting kOfxPropLabel do not touch Short/Long (no hook links them). Params are the params area.
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - P8
     - `ofxCore.h:776-783 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L776-L783>`__ kOfxPropHostOSHandle, host descriptor: root window handle
     - SHOULD
     - NULL at startup. It is set to the main window when the GUI is created (Gui/Gui05.cpp:174 → Engine/AppManager.cpp:2202-2205, 2195). It is still NULL during the Load/Describe scan.
     - OK
   * - P9
     - `ofxCore.h:766-774 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L766-L774>`__ kOfxPropEffectInstance on interacts
     - MUST
     - HS/ofxhInteract.cpp:117, 181 (interact area).
     - OK
   * - P10
     - ofxStatusCodes.rst / `ofxCore.h:880-946 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L880-L946>`__ status code values
     - DEFAULT
     - Same header values.
     - OK

4. ofxProperty.h: OfxPropertySuiteV1
------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron + HostSupport
     - verdict
   * - R1
     - `ofxProperty.h:31-36 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxProperty.h#L31-L36>`__ (all setters) returns OK / ErrBadHandle / ErrUnknown / ErrBadIndex / ErrValue
     - MUST
     - ``propSet``/``propSetN`` (HS/ofxhPropertySuite.cpp:844-940): NULL or bad magic → BadHandle; unknown name or **type mismatch** → ErrUnknown; index < 0 or past a fixed dimension → BadIndex; read-only → ErrValue (when enforced).
     - OK
   * - R2
     - (setters) a property marked "read only" (for plug-ins) cannot be set
     - MUST
     - ``refusePluginWrite`` (HS/ofxhPropertySuite.cpp:231-252). Natron selects ``ePluginReadOnlyWarn`` (Engine/OfxHost.cpp:186): every plug-in write to a read-only property **succeeds**, with a one-time warning. This is deliberate, for old third-party plug-ins (openfx #91).
     - PARTIAL (deliberate, won't fix)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - R3
     - propSetN: "count number of values we are setting (ie: indices 0..count-1)"
     - MUST
     - A fixed dimension rejects count > dim. A variable dimension is resized to exactly ``count`` (HS/ofxhPropertySuite.cpp:430-451).
     - OK
   * - R4
     - `ofxProperty.h:154-276 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxProperty.h#L154-L276>`__ getters: OK / BadHandle / Unknown / BadIndex
     - MUST
     - ``propGet``/``propGetN`` (HS/ofxhPropertySuite.cpp:944-1030). Out-of-range → BadIndex (HS/ofxhPropertySuite.cpp:369-397). They follow the chained (descriptor) set. **Exception:** the host's kOfxPropName get-hook throws **kOfxStatErrValue** for index ≠ 0 (Engine/OfxHost.cpp:483-484) instead of ErrBadIndex.
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - R5
     - `ofxProperty.h:278-288 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxProperty.h#L278-L288>`__ propReset "Resets all dimensions of a property to its default value"; returns OK / BadHandle / Unknown
     - MUST
     - HS/ofxhPropertySuite.cpp:1033-1074, 474-498: a fixed dimension goes back to the default and a variable one to empty (its default), or the reset goes through the get-hook. It also returns ErrValue for read-only properties (not in the listed codes, but consistent with the setters).
     - OK
   * - R6
     - `ofxProperty.h:290-301 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxProperty.h#L290-L301>`__ propGetDimension: OK / BadHandle / Unknown
     - MUST
     - Checks only ``properties != NULL``, **not** ``verifyMagic()``, unlike every other suite function (HS/ofxhPropertySuite.cpp:1077-1110). A stale or non-property handle is dereferenced instead of returning BadHandle.
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - R7
     - ofxPropertySuiteReference.rst "number and types of properties… strictly defined by the API… no scope to add new properties"
     - MUST
     - Setting an undefined name returns ErrUnknown.
     - OK
   * - R8
     - value strings (ofxCoreAPI.rst Strings): the suite does not take ownership; the receiver doesn't free
     - MUST
     - Values are copied into std::string. Note: ``propSetString(…, NULL)`` builds a ``std::string`` from NULL (UB), but that is a plug-in error.
     - OK

5. ofxMemory.h: OfxMemorySuiteV1
--------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron + HostSupport
     - verdict
   * - M1
     - `ofxMemory.h:23-35 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMemory.h#L23-L35>`__ memoryAlloc: "Allocated memory will be aligned for any use"; OK or ErrMemory, "no memory was allocated"
     - MUST
     - ``malloc`` (HS/ofxhHost.cpp:53-63). Natron doesn't override it (Engine/OfxHost.cpp:1193-1201). ``malloc(0)`` may return NULL, giving ErrMemory.
     - OK
   * - M2
     - `ofxMemory.h:25 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMemory.h#L25>`__ handle "effect instance to associate with this memory allocation, or NULL"
     - MAY
     - Ignored.
     - OK
   * - M3
     - `ofxMemory.h:41-50 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMemory.h#L41-L50>`__ memoryFree: kOfxStatErrBadHandle "allocatedData was not a valid pointer returned by memoryAlloc"
     - MUST
     - Plain ``free()`` (HS/ofxhHost.cpp:65-69), always OK. An invalid pointer is UB or a crash, not BadHandle.
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__

6. ofxMultiThread.h: OfxMultiThreadSuiteV1 (Natron implementation, Engine/OfxHost.cpp:1222-1732)
------------------------------------------------------------------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron
     - verdict
   * - T1
     - `ofxMultiThread.h:46-48 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMultiThread.h#L46-L48>`__ spawn nThreads, each ``func(index, nThreads, arg)``
     - MUST
     - Thread pool or ``OfxThread`` paths (Engine/OfxHost.cpp:1393-1460).
     - OK
   * - T2
     - `ofxMultiThread.h:50-51 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMultiThread.h#L50-L51>`__ "multiThread will not return until all the spawned threads have returned"
     - MUST
     - ``waitForFinished`` / ``wait()`` on every thread.
     - OK
   * - T3
     - `ofxMultiThread.h:53-54 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMultiThread.h#L53-L54>`__ "threads will be limited to the number of CPUs returned by multiThreadNumCPUs"
     - MUST
     - Non-pool path: at most ``maxConcurrentThread`` threads at once. Pool path: QThreadPool limits.
     - OK
   * - T4
     - `ofxMultiThread.h:56-61 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMultiThread.h#L56-L61>`__ "This function cannot be called recursively"; kOfxStatErrExists on a recursive attempt
     - MUST
     - No check: a nested call from a spawned thread is executed (pool or sequential) and returns OK.
     - WRONG (more permissive)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - T5
     - `ofxMultiThread.h:84-86 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMultiThread.h#L84-L86>`__ multiThreadIndex "same as the threadIndex argument… If there are no threads currently spawned… 0"
     - MUST
     - Correct on the threaded paths (TLS stack). On the **sequential path** (``nThreads==1 || maxConcurrentThread<=1 || single-thread setting``, Engine/OfxHost.cpp:1380-1390), ``func(i,…)`` runs without pushing ``i``, so multiThreadIndex returns 0 for i>0, and multiThreadIsSpawnedThread returns 0 inside func.
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - T6
     - `ofxMultiThread.h:68-77 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMultiThread.h#L68-L77>`__ multiThreadNumCPUs "may be less than the actual number of CPUs"
     - MAY
     - Settings- and load-dependent, ≥1 (Engine/OfxHost.cpp:1472-1523).
     - OK
   * - T7
     - `ofxMultiThread.h:102-157 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMultiThread.h#L102-L157>`__ mutexCreate(lockCount) / Destroy / Lock / UnLock / TryLock: a counted (recursive) mutex
     - MUST
     - ``mutexCreate`` allocates a **QRecursiveMutex** (Engine/OfxHost.cpp:1571), but Destroy, Lock, UnLock and TryLock ``reinterpret_cast`` the handle to **QMutex\*** (Engine/OfxHost.cpp:1622, 1653, 1682, 1712). In Qt5 this works by accident: QRecursiveMutex privately derives from QMutex, as Global/QtCompat.h:134 notes. In **Qt6** the two classes are unrelated. QRecursiveMutex starts with ``owner``, ``count``, then ``QMutex mutex``, so ``QMutex::lock()`` operates on the ``owner`` field: re-locking from the same thread deadlocks, and a mutex created with lockCount>0 (really locked through QRecursiveMutex) is then "unlocked" as a QMutex with a thread pointer as d_ptr, which crashes. ``delete`` also goes through the wrong type. RB-2.7 (the Qt6 branch) has the same code.
     - **WRONG (Qt6 builds: deadlock/crash)**

       Issue: `#1361 <https://github.com/NatronGitHub/Natron/issues/1361>`__
   * - T8
     - `ofxMultiThread.h:112 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMultiThread.h#L112>`__ mutexCreate lockCount "can be negative"
     - DEFAULT
     - A negative count does nothing (loop skipped).
     - OK
   * - T9
     - `ofxMultiThread.h:120 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMultiThread.h#L120>`__,132,142,155 kOfxStatErrBadHandle "if the handle was bad"
     - MUST
     - Only NULL is detected.
     - PARTIAL (spec-only)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - T10
     - ofxThreadSafety.rst list of render-thread actions; "a main thread should generally issue an abort to any linked render thread when a parameter… changed"
     - MUST / SHOULD
     - InstanceChanged and the edit actions run only on the main thread. Render-thread use of the other image-effect actions belongs to the image-effect area.
     - OK (core part)

7. ofxMessage.h: OfxMessageSuiteV1/V2
-------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron + HostSupport
     - verdict
   * - G1
     - `ofxMessage.h:93-95 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMessage.h#L93-L95>`__ "a host can return this struct for both V1 and V2"
     - MAY
     - One V2 struct for both (HS/ofxhImageEffect.cpp:4809-4815, 4365-4369).
     - OK
   * - G2
     - `ofxMessage.h:68 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMessage.h#L68>`__, `ofxMessage.h:100 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMessage.h#L100>`__ message handle "effect handle (descriptor or instance)… may be NULL"
     - MUST
     - ``messageHandleToInstance`` checks the magic and falls back to the host for NULL or a descriptor (HS/ofxhImageEffect.cpp:4289-4325).
     - OK
   * - G3
     - `ofxMessage.h:74-78 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMessage.h#L74-L78>`__, `ofxMessage.h:106-110 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMessage.h#L106-L110>`__ message returns "kOfxStatOK - if the message was successfully posted"
     - MUST
     - Both ``OfxImageEffectInstance::vmessage`` (Engine/OfxImageEffectInstance.cpp:327) and ``OfxHost::vmessage`` (Engine/OfxHost.cpp:566) return **kOfxStatReplyDefault** for Log, Error, Fatal, Warning and Message, and for unknown types.
     - WRONG

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - G4
     - `ofxMessage.h:48-57 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMessage.h#L48-L57>`__ kOfxMessageQuestion: "The host is to enter a modal state which waits for the user to respond yes or no… only return after the user responds"; "It is an error to post a question message if the plugin is not in an interactive session"
     - MUST
     - On the main thread with a GUI: modal question (Engine/Node.cpp:3823-3825). In three cases the user is not asked and the answer is wrong. (a) Posted from a **non-main thread**: logged and ``return true`` → kOfxStatReplyYes (Engine/Node.cpp:3782-3793). (b) Node inside a Read/Write **IO container**: ``ioContainer->message(...)`` is asked but its answer is discarded, and ``return true`` gives Yes, even if the user answered No (Engine/Node.cpp:3796-3800). (c) **Background/NatronRenderer**: ``AppInstance::questionDialog`` prints and returns Yes (Engine/AppInstance.cpp:1717-1726) instead of an error. Also, an aborted node returns No silently (Engine/Node.cpp:3775-3778).
     - WRONG

       Issue: `#1374 <https://github.com/NatronGitHub/Natron/issues/1374>`__
   * - G5
     - `ofxMessage.h:21 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMessage.h#L21>`__,28,34,40,46 type semantics: Fatal/Error "recoverable by user intervention", Warning, Message "directly to the user", Log "to a log and not to the end user"
     - SHOULD
     - Instance: Error and Fatal → error dialog, Warning → warning dialog, Message → info dialog, Log → log (Engine/OfxImageEffectInstance.cpp:306-318). Host-level (NULL or descriptor handle): Error, Fatal and Warning go **only to the log**, on purpose because of Sapphire (Engine/OfxHost.cpp:541-556). From non-main threads everything is logged (Engine/Node.cpp:3782-3793).
     - PARTIAL (deliberate, won't fix)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - G6
     - `ofxMessage.h:118-137 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMessage.h#L118-L137>`__ setPersistentMessage: handle "may NOT be null"; ErrBadHandle "the handle was rubbish"; error/warning "associated error states should be flagged"
     - MUST / SHOULD
     - A NULL or bad handle gives BadHandle (HS/ofxhImageEffect.cpp:4327-4347). Error, Warning and Message are set on the node and shown on it (Engine/OfxImageEffectInstance.cpp:256-278). Other types are silently ignored but return OK.
     - OK
   * - G7
     - `ofxMessage.h:134 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMessage.h#L134>`__ "Persistent messages are associated with an effect handle until explicitly cleared by an effect"
     - MUST
     - Natron also clears them itself: on node deactivate (Engine/Node.cpp:2815) and at the start of ``Node::makePreviewImage`` (Engine/Node.cpp:3352).
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - G8
     - `ofxMessage.h:145-157 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxMessage.h#L145-L157>`__ clearPersistentMessage: OK / ErrBadHandle; "Clearing a message will clear any associated error state"
     - MUST
     - HS/ofxhImageEffect.cpp:4349-4362 → Engine/OfxImageEffectInstance.cpp:280-290.
     - OK

8. ofxPropertiesByObject.rst: host descriptor (core items; image-effect host properties belong to the image-effect area)
------------------------------------------------------------------------------------------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron + HostSupport
     - verdict
   * - H1
     - `ofxPropertiesByObject.rst:15-26 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxPropertiesByObject.rst?plain=1#L15-L26>`__ and `ofxCore.h:675 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L675>`__, 688: kOfxPropAPIVersion, kOfxPropType, kOfxPropName, kOfxPropLabel, kOfxPropVersion and kOfxPropVersionLabel are **(read only)** on the host
     - MUST
     - The base ``hostStuffs`` declares all six with ``readonly=false`` (HS/ofxhHost.cpp:86-94), so they are writable by plug-ins even under ePluginReadOnlyEnforce. The image-effect host properties are read-only (HS/ofxhImageEffect.cpp:4660-4720). Also, the private ``sf.openfx.net.OfxHostSupportHostPointer`` (HS/ofxhHost.cpp:93, 116), which ``fetchSuite`` dereferences (HS/ofxhHost.cpp:98-108), is plug-in writable: overwriting it makes every later fetchSuite call through a garbage pointer. One plug-in can also rename the host for all the others (the kOfxPropName raw value, Engine/OfxHost.cpp:480).
     - WRONG

       Issue: `#1367 <https://github.com/NatronGitHub/Natron/issues/1367>`__
   * - H2
     - kOfxPropType = kOfxTypeImageEffectHost
     - DEFAULT
     - HS/ofxhHost.cpp:88.
     - OK
   * - H3
     - kOfxPropLabel, the user-visible application name
     - MUST
     - "Natron" (Engine/OfxHost.cpp:260).
     - OK
   * - H4
     - kOfxPropHostOSHandle
     - SHOULD
     - See P8.
     - OK

9. ofxProgress.h: OfxProgressSuiteV1/V2
---------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron + HostSupport
     - verdict
   * - S1
     - V1 and V2 both provided (best practice: plug-in tries 2 then 1)
     - MAY
     - HS/ofxhImageEffect.cpp:4837-4844.
     - OK
   * - S2
     - `ofxProgress.h:46-57 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxProgress.h#L46-L57>`__ progressStart: effectInstance "cannot be NULL"; ErrBadHandle "effectInstance was invalid"; OK or Failed
     - MUST
     - Only NULL is checked. Any other pointer is ``reinterpret_cast`` to ``Instance*`` without a magic check (HS/ofxhImageEffect.cpp:4430-4450). It always returns OK. In background mode it is a no-op that returns OK (Engine/AppInstance.h:251-260).
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - S3
     - `ofxProgress.h:17-27 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxProgress.h#L17-L27>`__ (API 1.4) "expected that it can be raised in a modal manner and have a 'cancel' button when invoked in instanceChanged… kOfxStatReplyNo should be returned… when the user presses cancel"
     - SHOULD
     - The progress panel task has Cancel. ``wasCanceled()`` gives ReplyNo (Gui/ProgressPanel.cpp:539-576). It is **not modal**: ``processEvents()`` lets the user keep editing while the plug-in analyses in instanceChanged.
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - S4
     - `ofxProgress.h:65-70 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxProgress.h#L65-L70>`__ progressUpdate: OK, ReplyNo (abort), or ErrBadHandle "the progress handle was invalid"
     - MUST
     - With no started task, ``updateTask`` returns false → **ReplyNo** (Gui/ProgressPanel.cpp:549-551), not BadHandle. Background: always OK.
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - S5
     - `ofxProgress.h:51 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxProgress.h#L51>`__ / `ofxProgress.h:110 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxProgress.h#L110>`__ progressStart \\pre "no currently ongoing progress display for this instance"
     - (plug-in)
     - A second Start cancels and replaces the task (Gui/ProgressPanel.cpp:451-459).
     - OK
   * - S6
     - `ofxProgress.h:82 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxProgress.h#L82>`__ progressEnd \\post "you can no longer call progressUpdate"
     - (plug-in)
     - Enforced indirectly (S4).
     - OK

10. ofxTimeLine.h: OfxTimeLineSuiteV1
-------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron + HostSupport
     - verdict
   * - L1
     - `ofxTimeLine.h:18-29 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxTimeLine.h#L18-L29>`__ getTime "current time value of the timeline associated with the effect"
     - MUST
     - ``currentFrame()`` (Engine/OfxImageEffectInstance.cpp:1129-1135).
     - OK
   * - L2
     - `ofxTimeLine.h:32-46 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxTimeLine.h#L32-L46>`__ gotoTime "moves the timeline… Any side effects of the timeline change are also triggered and completed before this returns (… instance changed actions and renders…)"; ErrValue "the time was an illegal value"; time in the effect's temporal coordinates (double)
     - MUST
     - ``seekFrame((int)t, …)`` (Engine/OfxImageEffectInstance.cpp:1138-1150): it **truncates** fractional times, re-renders asynchronously (renders are not completed on return), always returns OK, and never ErrValue.
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - L3
     - `ofxTimeLine.h:49-61 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxTimeLine.h#L49-L61>`__ getTimeBounds
     - MUST
     - The project frame range (Engine/OfxImageEffectInstance.cpp:1153-1162).
     - OK
   * - L4
     - ErrBadHandle "the effect handle was invalid"
     - MUST
     - Only NULL is checked; no magic check (HS/ofxhImageEffect.cpp:4490-4520).
     - PARTIAL (spec-only)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__

11. ofxDialog.h: OfxDialogSuiteV1
---------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron + HostSupport
     - verdict
   * - D1
     - `ofxDialog.h:45-66 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxDialog.h#L45-L66>`__ OfxDialogSuiteV1 {RequestDialog(user_data), NotifyRedrawPending()}, an optional suite
     - MAY
     - ``OFX_SUPPORTS_DIALOG`` is on, but ``OFX_SUPPORTS_DIALOG_V1`` is not (global.pri:28-30; CMakeLists.txt:472), so ``fetchSuite(kOfxDialogSuite,1)`` returns NULL (HS/ofxhImageEffect.cpp:4816-4828). Only a fork-specific **V2** (instance and inArgs arguments, NatronGitHub/openfx include/ofxDialog.h) is returned, under the unprefixed standard suite name. Returning NULL is allowed.
     - N/A (not provided). The V2 name could collide with a future standard V2.
   * - D2
     - `ofxDialog.h:33-42 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxDialog.h#L33-L42>`__ kOfxActionDialog "runs in the host's UI thread… host will not send any other messages asynchronous to this one"
     - MUST (if provided)
     - V2 → ``requestOFXDIalogOnMainThread`` (Engine/OfxHost.cpp:1738-1748).
     - N/A for V1
   * - D3
     - `ofxDialog.h:57-64 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxDialog.h#L57-L64>`__ NotifyRedrawPending returns kOfxStatReplyDefault
     - MUST
     - Engine/OfxHost.cpp:1752-1759 (V2).
     - N/A for V1

12. ofxStructure.rst, ofxThreadSafety.rst: other host statements
----------------------------------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron
     - verdict
   * - X1
     - ofxStructure.rst "descriptors are typically passed… during Describe, whilst all other actions are passed an instance"
     - MUST
     - Yes.
     - OK
   * - X2
     - ofxThreadSafety.rst "For a given effect instance, there can be only one main thread and zero or more render threads"; the host may use shadow instances for rendering
     - MAY
     - Render clones (Engine/OfxEffectInstance.cpp:643-660).
     - OK
   * - X3
     - ofxThreadSafety.rst param locking "block write/read, not block read/read, fine grained"
     - SHOULD
     - Params area.
     - N/A here

Problems (MISSING / WRONG / PARTIAL), by severity
-------------------------------------------------

Crash or data loss
~~~~~~~~~~~~~~~~~~

1. **T7: multithread-suite mutexes are created as QRecursiveMutex but used as QMutex.** Under Qt6 this deadlocks on recursive locking, and crashes when lockCount>0. The code is Engine/OfxHost.cpp:1571 (create) versus 1622/1653/1682/1712 (destroy, lock, unlock, trylock). Second check: Global/QtCompat.h:134-139 itself says "Qt6 made the two types unrelated". ``MULTI_THREAD_SUITE_USES_THREAD_SAFE_MUTEX_ALLOCATION`` is not defined, so the ``#else`` path is live. ``git log -L`` shows that 583860c89 (`#720 <https://github.com/NatronGitHub/Natron/issues/720>`__) switched creation to QRecursiveMutex without touching the other four functions, and 6e28f2a16 removed the Qt<5.14 branch. origin/RB-2.7 (the Qt6 branch) has the same code. Qt5 builds work only because Qt5's QRecursiveMutex privately derives from QMutex at offset 0. **Issue:** `#1361 <https://github.com/NatronGitHub/Natron/issues/1361>`__.

2. **A15: SyncPrivateData is not done before an autosave.** Autosave serializes in ``QtConcurrent::run`` (Engine/Project.cpp:768). ``NodeSerialization`` (Engine/NodeSerialization.cpp:66-70) emits ``syncPrivateDataRequested``, which is queued to the main thread (AutoConnection, Engine/OfxEffectInstance.cpp:312, 319), so the node is saved before the plug-in syncs. The sync is also skipped while a dialog is shown (Engine/OfxEffectInstance.cpp:2878-2887). Effect: autosaves (crash recovery) can contain stale plug-in private data. Second check: the only other sync entry points are ``onSyncPrivateDataRequested`` (main thread, Engine/EffectInstance.cpp:5297) and ``purgeCaches`` (Engine/OfxEffectInstance.cpp:2768-2810). Neither runs on the autosave path. **Issue:** `#1368 <https://github.com/NatronGitHub/Natron/issues/1368>`__.

Wrong results or behaviour visible to plug-ins
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

3. **A7: Begin/EndInstanceChanged never bracket parameter InstanceChanged.** This is `#909 <https://github.com/NatronGitHub/Natron/issues/909>`__ (OPEN), re-verified. ``KnobHolder::beginKnobsValuesChanged_public``/``endKnobsValuesChanged_public`` (Engine/Knob.cpp:6211-6228) have no callers in the tree (``git grep``). ``git log -S`` shows the last references were in 3c2a4956a and 11b023533. ``paramChangedByPlugin``, the bracketed HS path (HS/ofxhImageEffect.cpp:3497-3511), is overridden by a no-op in Engine/OfxImageEffectInstance.cpp:1548-1556. Only clip changes are bracketed (Engine/OfxEffectInstance.cpp:1247-1249). Note that `#1107 <https://github.com/NatronGitHub/Natron/issues/1107>`__ requires this before advertising 1.5. **Issue:** `#909 <https://github.com/NatronGitHub/Natron/issues/909>`__.

4. **G4: a question message can get an answer the user did not give.** A Question returns kOfxStatReplyYes without asking in three cases: from a non-main thread (Engine/Node.cpp:3782-3793); in background mode (Engine/AppInstance.cpp:1717-1726, instead of an error); and for a node inside an IO container, where the user's real answer is discarded (Engine/Node.cpp:3796-3800). Second check: ``OfxImageEffectInstance::vmessage`` maps the bool straight to Yes/No (Engine/OfxImageEffectInstance.cpp:319-324), and ``Node::message`` is the only implementation. **Issue:** `#1374 <https://github.com/NatronGitHub/Natron/issues/1374>`__.

5. **A17/A16: EndInstanceEdit is sent without BeginInstanceEdit, and is not sent at the right times.** End is sent without Begin for a node whose panel was never opened, and again after undo-delete then delete. It is never sent on panel close, project close or quit. Begin is not re-sent after reactivation. The code is Gui/NodeGui.cpp:2099-2101 (the only caller of ``endInstanceEditAction``) and ``_wasBeginEditCalled`` (Gui/NodeGui.cpp:185, 736), which is never reset. Second check: ``git grep endInstanceEdit`` gives only that call; ``beginEditKnobs`` callers are listed in A16. **Issue:** `#1381 <https://github.com/NatronGitHub/Natron/issues/1381>`__.

6. **A18: kOfxPropIsInteractive is always 0**, even while the GUI is editing the effect. Every instance is created with ``interactive=false`` (Engine/OfxHost.cpp:495), and nothing else writes it (``git grep`` in Engine/Gui; HS sets it only in the ctor, HS/ofxhImageEffect.cpp:755). **Issue:** `#1382 <https://github.com/NatronGitHub/Natron/issues/1382>`__.

7. **C17: DescribeInContext is called for one context only, and plug-ins are not filtered by mutually supported contexts.** Plug-ins whose contexts are all unsupported are still listed, and creating them then fails. The code is Engine/OfxHost.cpp:585-711 and 990. Second check: there is no other ``getContext()`` call (``git grep``). Natron uses Reader/Writer/Paint/Tracker contexts without advertising them, and Reader/Writer are deliberately not advertised because of Furnace (Engine/OfxHost.cpp:291-293). **Issue:** `#1383 <https://github.com/NatronGitHub/Natron/issues/1383>`__.

8. **C9/C4: Load, Describe and setHost are sent again on a binary that was never unloaded, after Unload.** This happens on the first launch or after a plug-in binary changes. The sequence is Load→Describe→Unload during the scan (HS/ofxhImageEffectAPI.cpp:413-490), then setHost→Load→Describe at the first instance (HS/ofxhImageEffectAPI.cpp:164-216). The binary is kept loaded by ``loadPluginInfo``'s session reference (HS/ofxhPluginCache.cpp:334-340). This contradicts `ofxCore.h:174 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L174>`__, 212 and 251, but the Guide (`ofxExample1_Basics.rst:446 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Guide/ofxExample1_Basics.rst?plain=1#L446>`__, 779-783) explicitly allows it. Second check: Natron has no override of ``loadFromPlugin`` or ``PluginHandle``. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

9. **P2: kOfxPropType is wrong on instances.** Effect instances report "OfxTypeImageEffect" and parameter instances report "OfxTypeParameter", instead of the ``*Instance`` values (HS/ofxhImageEffect.cpp:742; HS/ofxhParam.cpp:277, 706-708). Second check: ``grep kOfxTypeImageEffectInstance|kOfxTypeParameterInstance`` finds nothing in HS or Engine. (Also relevant to the params area.) **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

10. **H1: six host-descriptor properties and the private host pointer are plug-in writable.** They are kOfxPropAPIVersion, Type, Name, Label, Version and VersionLabel, plus the private ``OfxHostSupportHostPointer`` (HS/ofxhHost.cpp:86-94). A plug-in could rename the host for everyone, or corrupt ``fetchSuite``. Second check: the ImageEffect ``hostStuffs`` doesn't redeclare them (``createProperty`` ignores duplicates anyway), and Natron's ``setProperties`` only sets values. **Issue:** `#1367 <https://github.com/NatronGitHub/Natron/issues/1367>`__.

Spec-only
~~~~~~~~~

11. **G3:** ``message()`` returns kOfxStatReplyDefault instead of kOfxStatOK on success (Engine/OfxImageEffectInstance.cpp:327; Engine/OfxHost.cpp:566). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

12. **C1:** ``OfxSetHost`` is not supported, which is optional for the host. It is related to `#1107 <https://github.com/NatronGitHub/Natron/issues/1107>`__, which mentions only the API version. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__, `#1107 <https://github.com/NatronGitHub/Natron/issues/1107>`__.

13. **R2:** read-only properties are writable with a warning (``ePluginReadOnlyWarn``, Engine/OfxHost.cpp:186). This is deliberate (openfx #91). **Deliberate, won't fix.** Listed in `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__ for completeness.

14. **T4:** recursive ``multiThread`` is allowed instead of returning kOfxStatErrExists (Engine/OfxHost.cpp:1362-1465). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

15. **T5:** on the sequential multiThread path, multiThreadIndex and IsSpawnedThread return 0 (Engine/OfxHost.cpp:1380-1390). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

16. **L2:** ``gotoTime`` truncates to int, does not wait for side effects, and never returns ErrValue (Engine/OfxImageEffectInstance.cpp:1138-1150). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

17. **S3/S4:** progress is not modal, and ``progressUpdate`` without a start returns ReplyNo instead of BadHandle (Gui/ProgressPanel.cpp:539-576). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

18. **S2/L4/T9/R6/M3:** handles are not validated. The progress and timeline suites ``reinterpret_cast`` without a magic check (HS/ofxhImageEffect.cpp:4430-4520). ``propGetDimension`` lacks ``verifyMagic`` (HS/ofxhPropertySuite.cpp:1077-1110). The mutex functions check only NULL. ``memoryFree`` calls ``free()`` on anything (HS/ofxhHost.cpp:65-69). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

19. **R4:** the host kOfxPropName get-hook returns ErrValue instead of ErrBadIndex for index≠0 (Engine/OfxHost.cpp:483-484). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

20. **P4:** render clones start with the main instance's kOfxPropInstanceData instead of NULL (HS/ofxhImageEffect.cpp:780-797). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

21. **P5:** the host kOfxPropName is user-set and proxied per plug-in (deliberate). There is also a typo: ``loadingPluginVersionMinor = getVersionMajor()`` (Engine/OfxHost.cpp:676), harmless. The proxied name is deliberate, won't fix; the typo is in `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

22. **P7:** effect descriptor Short/Long labels default to "", and resetting or setting kOfxPropLabel does not reset them. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

23. **G5/G7:** host-level Error and Warning messages go only to the log (deliberate, because of Sapphire). Natron also clears persistent messages itself (Engine/Node.cpp:2815, 3352). Host-level messages going to the log is deliberate, won't fix. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

24. **A14:** asserts on legal PurgeCaches and SyncPrivateData return codes (Engine/OfxEffectInstance.cpp:2781, 2808). This affects debug builds only. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

25. **D1:** the standard OfxDialogSuiteV1 is not provided; a fork-only V2 is served under the standard name. This is allowed (optional suite). Allowed by the spec: no issue.

NEXT items met: InstanceChanged after project load (`#1107 <https://github.com/NatronGitHub/Natron/issues/1107>`__ mentions it). It is out of scope.
