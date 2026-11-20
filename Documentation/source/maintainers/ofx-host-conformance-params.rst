.. _ofx-host-conformance-params:

OFX host conformance: parameters
================================

This page is part of the :ref:`OpenFX 1.5.1 host conformance audit <ofx-host-conformance>`.
It covers parameters and the parameter suites.

**Spec scope.** The headers ``ofxParam.h`` and ``ofxParametricParam.h``, and the Reference page
``ofxParameter.rst`` (the suite reference pages ``ofxParametersSuiteReference.rst`` and
``ofxParametricParamReference.rst`` only wrap the header structs).

**Spec reference abbreviations.** ``rst:`` is ``ofxParameter.rst``. A bare ``:line`` in the
spec column is in ``ofxParam.h``, or in ``ofxParametricParam.h`` in Table 6.

**Code reference abbreviations.** OPI is ``Engine/OfxParamInstance.cpp``, OIEI is
``Engine/OfxImageEffectInstance.cpp``, OEI is ``Engine/OfxEffectInstance.cpp``, oPP is
``libs/OpenFX_extensions/ofxhParametricParam.cpp`` (Natron's own parametric-param host code).
hP, hIE and hPS are ``ofxhParam.cpp``, ``ofxhImageEffect.cpp`` and ``ofxhPropertySuite.cpp``
in ``libs/OpenFX/HostSupport/src/``. Other Natron files are named without their directory
(``Engine/`` or ``Gui/``).

The table rows have their own identifiers (H, T, P, S, L, R); the problems at the end of the page
are numbered "Problem 1" to "Problem 20", and the verdict column refers to them as "problem N".

Verdicts: **OK**; **MISSING** (not implemented); **WRONG** (implemented, but contradicts the spec); **PARTIAL** (partly implemented, or only in some cases); **N/A** (does not apply to Natron); **NEXT** (seen, but out of scope of this audit). The level column gives the strength of the spec statement (MUST, SHOULD, MAY) or DEFAULT for a default value. Spec references link to the upstream ``OFX_Release_1.5.1`` tag. Code references are plain paths with line numbers at the audited commits (Natron RB-2.6 at 7dea437a6, HostSupport at 93c21ed7); they are not updated when the code moves. Each row or problem that is not OK links to the issue that tracks it; items marked **deliberate, won't fix** are known deviations that Natron keeps on purpose.

Table 1: ofxParam.h, host properties
------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - Spec (1.5.1)
     - Level
     - Natron + HostSupport
     - Verdict
   * - H1
     - `ofxParam.h:98-104 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L98-L104>`__ ``kOfxParamHostPropSupportsCustomAnimation`` host, ro, 0/1
     - DEFAULT
     - OfxHost.cpp:303 sets ``KnobString::canAnimateStatic()`` = 1 (KnobTypes.h:760). The interpolation callback is passed the wrong handle and its status is ignored (see P3), so animated custom params get "" between keys
     - PARTIAL

       Issue: `#1370 <https://github.com/NatronGitHub/Natron/issues/1370>`__
   * - H2
     - `ofxParam.h:106-112 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L106-L112>`__ SupportsStringAnimation
     - DEFAULT
     - OfxHost.cpp:300 → 1; string keys are step-interpolated (Knob.cpp:6449-6482)
     - OK
   * - H3
     - `ofxParam.h:114-120 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L114-L120>`__ SupportsBooleanAnimation
     - DEFAULT
     - OfxHost.cpp:302 → 1 (KnobTypes.h:173)
     - OK
   * - H4
     - `ofxParam.h:122-128 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L122-L128>`__ SupportsChoiceAnimation
     - DEFAULT
     - OfxHost.cpp:301 → 1 (KnobTypes.h:527)
     - OK
   * - H5
     - `ofxParam.h:130-139 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L130-L139>`__ SupportsCustomInteract
     - DEFAULT
     - OfxHost.cpp:299 → 1; interacts created in OEI.cpp:755-778
     - OK
   * - H6
     - `ofxParam.h:141-148 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L141-L148>`__ MaxParameters, -1 = unlimited
     - DEFAULT
     - OfxHost.cpp:307 → -1
     - OK
   * - H7
     - `ofxParam.h:150-159 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L150-L159>`__ MaxPages; "hosts that do not support paged layout should set this to zero"
     - SHOULD
     - OfxHost.cpp:308 → 0. Natron shows Page params as tabs of a hierarchical panel, not as a row/column paged layout
     - OK
   * - H8
     - `ofxParam.h:161-168 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L161-L168>`__ PageRowColumnCount int x2
     - DEFAULT
     - OfxHost.cpp:309-310 → (0,0)
     - OK
   * - H9
     - `ofxParam.h:684-692 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L684-L692>`__ SupportsStrChoiceAnimation (1.5)
     - DEFAULT
     - hIE.cpp:4692 default 0, not overridden by Natron
     - OK
   * - H10
     - `ofxParam.h:694-702 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L694-L702>`__ SupportsStrChoice (1.5)
     - DEFAULT
     - hIE.cpp:4694 default 0, not set by OfxHost.cpp, so the host correctly says "unsupported" (issue `#1101 <https://github.com/NatronGitHub/Natron/issues/1101>`__)
     - OK (feature N/A)
   * - H11
     - `ofxParametricParam.h:88-96 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParametricParam.h#L88-L96>`__ SupportsParametricAnimation
     - DEFAULT
     - OfxHost.cpp:305 → 0; parametric getValue ignores time (OPI.cpp:4728-4743)
     - OK

Table 2: ofxParam.h, parameter types
------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - Spec
     - Level
     - Natron + HostSupport
     - Verdict
   * - T1
     - `ofxParam.h:41-74 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L41-L74>`__ the 17 types + Parametric; `ofxParameter.rst:79-97 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L79-L97>`__
     - MUST (paramDefine)
     - hP.cpp:218-237 type map; OIEI.cpp:538-693 ``newParam`` builds a knob for every type except StrChoice and Bytes
     - OK
   * - T2
     - `ofxParam.h:49 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L49>`__ StrChoice (1.5)
     - MAY
     - hP.cpp:2252-2260: paramDefine returns kOfxStatErrUnsupported because Natron leaves SupportsStrChoice=0. OIEI ``newParam`` would throw "unknown OFX type" (OIEI.cpp:698-700), but that point is never reached. Issue `#1101 <https://github.com/NatronGitHub/Natron/issues/1101>`__
     - N/A (not advertised)
   * - T3
     - `ofxParam.h:67-85 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L67-L85>`__ Bytes + OfxBytes
     - MAY
     - hP.cpp:2264-2272: ErrUnsupported unless ``kOfxHostSupportHostPropSupportsBytes``, which Natron doesn't set. The spec has no host property for it: the status is the only way for a plug-in to find out. Issue `#1102 <https://github.com/NatronGitHub/Natron/issues/1102>`__
     - N/A
   * - T4
     - `ofxParameter.rst:120-124 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L120-L124>`__ multi-dim params are atomic, "this applies to keyframes as well"; X/Y/Z labels
     - SHOULD
     - get/set of all dimensions together (e.g. OPI.cpp:2460-2506). Natron curves are per dimension and the user can key one dimension only; getNumKeys/KeyTime return the union over all dimensions (OPI.cpp:81-120), so the plug-in always sees one consistent key list
     - OK
   * - T5
     - `ofxParameter.rst:133 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L133>`__ Integer between INT_MIN and INT_MAX
     - DEFAULT
     - hP.cpp:533-545
     - OK
   * - T6
     - `ofxParameter.rst:151-154 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L151-L154>`__ colour values normalised 0..1
     - DEFAULT
     - Display range 0..1 by default (hP.cpp:541-542); values are not clamped (allowed)
     - OK
   * - T7
     - `ofxParameter.rst:162 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L162>`__ Boolean 0/1
     - DEFAULT
     - KnobBool (OPI.cpp:1244-1283)
     - OK
   * - T8
     - `ofxParameter.rst:171-195 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L171-L195>`__ Choice 0..N-1; "if no default... host should use index 0"; host stores the index
     - SHOULD
     - Default "0" (hP.cpp:510); set() rejects out-of-range with kOfxStatErrBadIndex (OPI.cpp:1497-1531). Natron stores the option *string* plus index and restores by string (Node.cpp:1176-1192), which is more robust than the spec requires
     - OK
   * - T9
     - `ofxParameter.rst:197-261 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L197-L261>`__, `ofxParam.h:630-666 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L630-L666>`__ ChoiceOrder (1.5): "If the host supports this property, it should sort the options... display in increasing order"; "To query whether a host supports this, a plugin should attempt to set the property and check the return status"
     - SHOULD
     - HostSupport declares the property on Choice and StrChoice descriptors (hP.cpp:334,342), so setting it **succeeds**, but nothing in Engine/Gui reads it (``git grep ChoiceOrder`` in Engine/Gui finds nothing). The host signals "supported" and then ignores it. Project storage is safe because Natron restores by option string (T8); only the UI order is wrong. Issues `#1137 <https://github.com/NatronGitHub/Natron/issues/1137>`__, `#1106 <https://github.com/NatronGitHub/Natron/issues/1106>`__
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - T10
     - `ofxParameter.rst:264-309 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L264-L309>`__ StrChoice behaviour (store the enum, restore by enum, unknown enum → default)
     - SHOULD
     - Not supported (T2)
     - N/A
   * - T11
     - `ofxParameter.rst:311-331 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L311-L331>`__, `ofxParameter.rst:798-847 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L798-L847>`__ String modes: SingleLine/MultiLine/FilePath/DirectoryPath/Label/RichText
     - DEFAULT
     - OPI.cpp:3790-3830 maps every mode (FilePath→KnobFile/KnobOutputFile by FilePathExists, Dir→KnobPath, Label→disabled label, RichText→multi-line rich text)
     - OK
   * - T12
     - `ofxParam.h:814-823 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L814-L823>`__ StringFilePathExists default 1; 0 lets the user name a new file
     - DEFAULT
     - hP.cpp:315; OPI.cpp:3797 picks an output-file knob when 0
     - OK
   * - T13
     - `ofxParameter.rst:336-353 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L336-L353>`__ Group: tree hierarchy, Parent "" = root, ordering by addition
     - SHOULD
     - Group knobs (OIEI.cpp:623-642); Parent default "" (hP.cpp:278)
     - OK
   * - T14
     - `ofxParameter.rst:363-394 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L363-L394>`__ Custom: no interface by default; "if they animate, the host's animation sheet should present a keyframe/curve representation"; "if the custom param sets kOfxParamPropInteractV1, this should be used by the host in any normal interface"
     - SHOULD
     - OIEI.cpp:618-619 forces every Custom param secret and disabled, and OPI.cpp custom setSecret/setEnabled are no-ops (comment: "Always make custom parameters secret"). So a custom param with an InteractV1 is never shown, and an animated custom param shows no key representation
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - T15
     - `ofxParameter.rst:373-379 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L373-L379>`__, `ofxParameter.rst:787-796 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L787-L796>`__, `ofxParameter.rst:879-902 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L879-L902>`__ Custom interp callback: called with ``(OfxParamSetHandle instance, inArgs{Name,Time,CustomValue x2,InterpolationTime x2,InterpolationAmount}, outArgs{CustomValue})``
     - MUST (host calls it as typed)
     - inArgs/outArgs are built correctly (StringAnimationManager.cpp:158-182). **The first argument is the param handle** (``(void*)getHandle()`` of the Param::Instance, OPI.cpp:4352), not the param set handle. The return status is ignored, and the result is read from outArgs, whose default is "" (StringAnimationManager.cpp:184-185)
     - WRONG (P3)

       Issue: `#1370 <https://github.com/NatronGitHub/Natron/issues/1370>`__
   * - T16
     - `ofxParameter.rst:401-408 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L401-L408>`__ PushButton: "if pressed, kOfxActionInstanceChanged... with kOfxChangeUserEdited"
     - MUST
     - KnobButton → knobChanged with UserEdited → kOfxChangeUserEdited (OEI.cpp:2594-2606)
     - OK
   * - T17
     - `ofxParameter.rst:413-438 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L413-L438>`__ animation defaults: Int/Double/Colour animate; Group/Page/PushButton cannot; Custom/String/Bool/Choice/StrChoice "do not animate by default"
     - DEFAULT
     - hP.cpp:497-517: Animates=1 for numeric/colour, 0 for Custom/String/Boolean/Choice/StrChoice/Bytes. Group/Page/PushButton have no value properties (eNone in type map)
     - OK
   * - T18
     - `ofxParameter.rst:529-547 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L529-L547>`__ PageOrder, PageChild, SkipRow/SkipColumn
     - MAY
     - PageOrder read (OEI.cpp); PageChild read (OIEI.cpp:643-662); SkipRow/SkipColumn ignored (MaxPages=0, so no grid layout)
     - OK

Table 3: ofxParam.h, parameter properties
-----------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - Spec
     - Level
     - Natron + HostSupport
     - Verdict
   * - P-Int1
     - `ofxParam.h:186-198 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L186-L198>`__ InteractV1, desc rw / inst ro, default NULL
     - DEFAULT
     - hP.cpp:461; used by OEI.cpp:764-777
     - OK
   * - P-Int2
     - `ofxParam.h:200-207 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L200-L207>`__ InteractSize double x2 "set by a host to indicate the current size of a custom interface" on the param **instance**
     - MUST (host sets)
     - Natron writes the size (as int) to the *interact descriptor* (OfxOverlayInteract.cpp:382-387, from CustomParamInteract.cpp:162), never to the param instance, where it stays (0,0) (hP.cpp:462) Fixed: ``OfxParamOverlayInteract::setSize()`` also sets the double kOfxParamPropInteractSize of the parameter instance.
     - OK (fixed by `#1484 <https://github.com/NatronGitHub/Natron/pull/1484>`__)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - P-Int3
     - `ofxParam.h:209-240 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L209-L240>`__ InteractSizeAspect (default 1), MinimumSize (10,10), PreferedSize (10,10) on the **param descriptor**; "A host should attempt to set ... to be this size"
     - SHOULD
     - Defaults OK (hP.cpp:463-465). Natron reads them from the *interact descriptor*, filled with its own defaults (OEI.cpp:769-771, OIEI.cpp:522-533), so values the plug-in set on the param descriptor are ignored (the Support library sets them on the interact descriptor, which hides this). Aspect is read with ``getIntProperty`` on a double property (CustomParamInteract.cpp:76), which returns 0, so the aspect is always ignored
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - P1
     - `ofxParam.h:242-249 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L242-L249>`__ ParamPropType, ro, "set to the type the parameter was created with"
     - MUST
     - hP.cpp:281 (read-only)
     - OK
   * - P2
     - `ofxParam.h:251-260 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L251-L260>`__ Animates default 1 (see T17), desc rw / inst ro
     - DEFAULT
     - hP.cpp:509; OIEI.cpp:735 ``setAnimationEnabled``
     - OK
   * - P3
     - `ofxParam.h:262-269 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L262-L269>`__ CanUndo default 1: "whether changes to a parameter should be put on the undo/redo stack"
     - DEFAULT/MUST
     - Default OK (hP.cpp:487); OIEI.cpp:738 ``setCanUndo``. But ``KnobGui::pushUndoCommand`` (Gui/KnobGui.cpp:169-177) pushes only if ``getCanUndo() && getEvaluateOnChange()``, so **params with EvaluateOnChange=0 are never undoable**, even for user edits. A CanUndo=0 param that is the first write of an edit block makes the following writes merge into the previous undo entry (see P7)
     - WRONG (P5)

       Issue: `#1372 <https://github.com/NatronGitHub/Natron/issues/1372>`__
   * - P4
     - `ofxParam.h:271-295 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L271-L295>`__ kOfxPropParamSetNeedsSyncing on the param set: the host "will examine this property each time it does a copy or save"; if unset, "will always call SyncPrivateData"
     - MUST
     - hIE.cpp:1319-1340 honours it if present. But neither HostSupport nor Natron declares it on the instance properties (``git grep`` finds only hIE.cpp:1327), and propSet on an undeclared property returns kOfxStatErrUnknown (hPS.cpp:860-864). The plug-in can't set it, so the host always syncs: allowed by the "not set" branch. Sync before save and copy: NodeSerialization.cpp:64-70
     - PARTIAL (P13)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - P5
     - `ofxParam.h:297-307 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L297-L307>`__ IsAnimating, inst ro: "set by a host... non-constant value... animation, scripting or expression"
     - MUST (host sets)
     - Only writer: OPI.cpp:360-374 (``onKnobAnimationLevelChanged``), from the animation level of **dimension 0 only**. The ``animationLevelChanged`` signal is emitted only if the knob has a GUI and the GUI isn't frozen for playback (Knob.cpp:3856-3860), and the slot is connected only if animation is enabled (OIEI.cpp:742-747). So with NatronRenderer, or before a panel exists, IsAnimating stays 0 for animated params; for 2D/3D params animated only in Y/Z it is 0
     - WRONG (P4)

       Issue: `#1371 <https://github.com/NatronGitHub/Natron/issues/1371>`__
   * - P6
     - `ofxParam.h:309-321 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L309-L321>`__ PluginMayWrite (deprecated in 1.4) default 0
     - DEFAULT
     - hP.cpp:485 declared, unused
     - OK
   * - P7
     - `ofxParam.h:323-332 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L323-L332>`__ Persistant default 1: "should be saved"
     - MUST
     - hP.cpp:484 (the fork names it kOfxParamPropPersistent, same string); OIEI.cpp:716-733 ``setIsPersistent`` (forced 0 for Group/Page); NodeSerialization.cpp:105-112 skips non-persistent knobs (unless they have an expression or link)
     - OK
   * - P8
     - `ofxParam.h:334-343 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L334-L343>`__ EvaluateOnChange default 1, inst rw
     - DEFAULT
     - hP.cpp:486; notify hook hP.cpp:720/843 → ``setEvaluateOnChange`` in each OPI class; OIEI.cpp:712
     - OK (but see P3)

       Issue: `#1372 <https://github.com/NatronGitHub/Natron/issues/1372>`__
   * - P9
     - `ofxParam.h:345-356 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L345-L356>`__ Secret default 0, inst rw; hidden "but should otherwise behave as a normal parameter"
     - MUST
     - hP.cpp:274; notify hook → setSecret (e.g. OPI.cpp:762). Custom params ignore it (T14). On project load Natron also restores Secret/Enabled from the project (Node.cpp:1216-1222): not a spec requirement, but plug-in logic that toggles visibility stays consistent only because of it (no instanceChanged after load: NEXT)
     - OK
   * - P10
     - `ofxParam.h:358-367 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L358-L367>`__ ScriptName default = name: "the name of a parameter in [host] scripts"
     - MAY
     - hP.cpp:277 default; Natron's Python API uses the param name, and ScriptName is never read (``git grep kOfxParamPropScriptName`` in Engine/Gui finds nothing)
     - PARTIAL (low)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - P11
     - `ofxParam.h:369-394 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L369-L394>`__ CacheInvalidation default ValueChange
     - MAY
     - hP.cpp:488 declared; Natron ignores it, but its node hash covers the whole curve, so any key change invalidates every frame of the node (as with kOfxParamInvalidateAll): conservative
     - OK
   * - P12
     - `ofxParam.h:396-402 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L396-L402>`__ Hint default "", inst rw
     - DEFAULT
     - hP.cpp:275; notify → setHint
     - OK
   * - P13
     - `ofxParam.h:404-429 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L404-L429>`__ Default per type (0 / "" / nullptr for Bytes; none for Group/Page/PushButton), inst rw
     - DEFAULT
     - hP.cpp:218-237, 507-512; notify → setDefault (e.g. OPI.cpp:808). On project load, ``cloneDefaultValues`` replaces the plug-in default with the default saved in the project (Node.cpp:1174) for every knob in the project: "reset to default" then gives the old plug-in default (spec `ofxParameter.rst:629-632 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L629-L632>`__ step 4 says the plug-in defaults)
     - PARTIAL (low)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - P14
     - `ofxParam.h:431-487 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L431-L487>`__ DoubleType default Plain; Angle in degrees, Scale, Time/AbsoluteTime in frames, X/Y/XY(Absolute) canonical
     - DEFAULT
     - hP.cpp:571-575. Values are passed through unchanged (no unit conversion: OK). Normalised types are de/normalised (OPI.cpp:935-972). XYAbsolute 2D get a host overlay handle (OIEI.cpp:1531-1541)
     - OK
   * - P15
     - `ofxParam.h:489-512 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L489-L512>`__ DefaultCoordinateSystem default Canonical; Normalised default means a fraction of the project size
     - MUST
     - hP.cpp:574; OPI.cpp:963-969 ``setDefaultValuesAreNormalized``; denormalised at reset/apply (KnobImpl.h:2450-2456, 2537-2543)
     - OK
   * - P16
     - `ofxParam.h:514-533 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L514-L533>`__ HasHostOverlayHandle (desc ro, host sets) / UseHostOverlayHandle default 0
     - MAY
     - hP.cpp:466-467 (HasHost read-only); set to 1 for 2D XYAbsolute/NormalisedXYAbsolute (OIEI.cpp:1524-1543); UseHost honoured in OPI.cpp
     - OK
   * - P17
     - `ofxParam.h:536-547 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L536-L547>`__ ShowTimeMarker default 0, for AbsoluteTime doubles
     - MAY
     - hP.cpp:579-583 declared; Natron never reads it
     - PARTIAL (low)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - P18
     - `ofxParam.h:549-558 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L549-L558>`__ PluginPropParamPageOrder, default declaration order
     - SHOULD
     - Read in OEI.cpp
     - OK
   * - P19
     - `ofxParam.h:560-571 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L560-L571>`__ PageChild
     - MAY
     - OIEI.cpp:643-662
     - OK
   * - P20
     - `ofxParam.h:573-584 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L573-L584>`__ Parent, default "" = root
     - SHOULD
     - hP.cpp:278
     - OK
   * - P21
     - `ofxParam.h:586-596 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L586-L596>`__ GroupOpen default 1
     - DEFAULT
     - hP.cpp:360; read in OPI.cpp (OfxGroupInstance)
     - OK
   * - P22
     - `ofxParam.h:598-607 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L598-L607>`__ Enabled default 1, inst rw; "the plug-in itself can still change the value of a disabled parameter"
     - MUST
     - hP.cpp:279; notify → setAllDimensionsEnabled; plug-in setValue is not blocked by Enabled
     - OK
   * - P23
     - `ofxParam.h:609-617 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L609-L617>`__ DataPtr, inst rw, default NULL, unique per instance
     - MUST
     - hP.cpp:280. Instances copy the descriptor's properties (hP.cpp:706-708), so the default is NULL and each instance has its own storage
     - OK
   * - P24
     - `ofxParam.h:619-628 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L619-L628>`__ ChoiceOption, desc/inst rw
     - MUST
     - Instance options re-read on change: ``ChoiceInstance::notify`` → ``OfxChoiceInstance::setOption`` (OPI.cpp:1627-1679)
     - OK
   * - P25
     - `ofxParam.h:630-666 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L630-L666>`__ ChoiceOrder
     - SHOULD
     - See T9
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - P26
     - `ofxParam.h:668-682 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L668-L682>`__ ChoiceEnum (StrChoice)
     - MAY
     - Declared for StrChoice (hP.cpp:339). Natron also accepts it on plain Choice as a Natron extension (hP.cpp:328, OPI.cpp:1447-1451)
     - N/A
   * - P27
     - `ofxParam.h:704-742 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L704-L742>`__ Min/Max/DisplayMin/DisplayMax defaults INT_MIN/MAX, ±DBL_MAX; inst rw; "Setting [Min] will also reset kOfxParamPropDisplayMin" (same for Max)
     - DEFAULT/MUST
     - Defaults OK (hP.cpp:533-545; colour display range 0..1). Notify hooks → setRange/setDisplayRange (hP.cpp:714-719, 837-841). Neither HostSupport nor Natron resets DisplayMin/Max when Min/Max is set (no such code in hP.cpp or OPI.cpp setRange, e.g. OPI.cpp:905-920)
     - PARTIAL (low)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - P28
     - `ofxParam.h:744-753 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L744-L753>`__ Increment default 1, >0
     - DEFAULT
     - hP.cpp:563; OPI.cpp:946-950 (applied only if >0). Not re-read after creation (no notify hook)
     - OK
   * - P29
     - `ofxParam.h:755-764 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L755-L764>`__ Digits default 2
     - DEFAULT
     - hP.cpp:564; OPI.cpp:951-953
     - OK
   * - P30
     - `ofxParam.h:766-775 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L766-L775>`__ DimensionLabel default "x","y","z"
     - DEFAULT
     - hP.cpp:614-619 (colour: r,g,b,a); OPI.cpp sets dimension names
     - OK
   * - P31
     - `ofxParam.h:777-785 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L777-L785>`__ IsAutoKeying, inst ro: "set by the host simply to indicate the state"
     - MUST (host sets)
     - OPI.cpp:373: set to ``level == eAnimationLevelInterpolatedValue``, i.e. 1 only between keys, 0 on a key. In Natron a change to an animated knob always keys it. Same signal problems as P5 (GUI-only, dimension 0)
     - PARTIAL

       Issue: `#1371 <https://github.com/NatronGitHub/Natron/issues/1371>`__
   * - P32
     - `ofxParam.h:787-796 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L787-L796>`__ CustomInterpCallbackV1
     - MUST
     - See T15
     - WRONG

       Issue: `#1370 <https://github.com/NatronGitHub/Natron/issues/1370>`__
   * - P33
     - Read-only properties on instances ("instance (read only)": Animates, CanUndo, Persistant, ScriptName, DoubleType, DimensionLabel, StringMode, ...)
     - MUST
     - Instances copy the descriptor PropSpecs with the same "host only" flag, which is false for these (hP.cpp:272-300, 481-488). A plug-in can write them on an instance with kOfxStatOK and nothing happens. Only Type is enforced (hP.cpp:281)
     - PARTIAL (spec-only)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - P34
     - kOfxParamInterpType (HostSupport hP.cpp:629-635)
     -
     - NEXT, out of scope (`#1138 <https://github.com/NatronGitHub/Natron/issues/1138>`__)
     - NEXT

Table 4: OfxParameterSuiteV1 (`ofxParam.h:905-1278 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L905-L1278>`__)
-----------------------------------------------------------------------------------------------------------------------------------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - Spec
     - Level
     - Natron + HostSupport
     - Verdict
   * - S1
     - paramDefine `ofxParam.h:908-932 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L908-L932>`__: OK / ErrBadHandle / ErrExists / **ErrUnknown if the type is unknown** / ErrUnsupported if known but unsupported
     - MUST
     - hP.cpp:2217-2290. ErrBadHandle (2226) OK, ErrExists (2234-2246) OK, ErrUnsupported for StrChoice/Bytes (2252-2272) OK. **An unknown type** gets ``SetDescriptor::paramDefine`` → NULL (hP.cpp:688-689) → **kOfxStatErrUnsupported** (hP.cpp:2282-2288), not ErrUnknown. Natron's override (OIEI.cpp:1524-1529) passes through. A NULL paramType reaches ``std::string(paramType)`` / ``strcmp`` (crash), but a NULL type is invalid anyway Re-checked at HostSupport bdfb48d4: an unknown type gives kOfxStatErrUnknown, and a NULL type kOfxStatErrUnknown instead of a crash.
     - OK (fixed by `NatronGitHub/openfx#187 <https://github.com/NatronGitHub/openfx/pull/187>`__, in libs/OpenFX since `#1483 <https://github.com/NatronGitHub/Natron/pull/1483>`__)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - S2
     - paramDefine "Parameters cannot be defined outside describe"
     - MAY enforce
     - Not checked: a SetInstance handle is reinterpret_cast to SetDescriptor (hP.cpp:2224)
     - OK (plug-in obligation)
   * - S3
     - paramGetHandle `ofxParam.h:934-951 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L934-L951>`__: OK / ErrBadHandle / ErrUnknown; "you cannot call this in a plugin's describe function"
     - MUST
     - hP.cpp:2306-2400: unknown name → ErrUnknown, bad set → ErrBadHandle. Also works on descriptors, which the parametric example in `ofxParameter.rst:1074-1087 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L1074-L1087>`__ relies on
     - OK
   * - S4
     - paramSetGetPropertySet `ofxParam.h:953-966 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L953-L966>`__: same handle as the plug-in instance's
     - MUST
     - hP.cpp:2415-2440 → ``getParamSetProps()``, which is the image effect's properties (hIE.cpp:801)
     - OK
   * - S5
     - paramGetPropertySet `ofxParam.h:968-981 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L968-L981>`__
     - MUST
     - hP.cpp:2455-2483 (magic check)
     - OK
   * - S6
     - paramGetValue / AtTime `ofxParam.h:983-1028 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L983-L1028>`__: OK / ErrBadHandle
     - MUST
     - hP.cpp:2499-2565; NaN time → ErrValue (extra); types without getV (Group/Page/PushButton/Parametric) → ErrUnsupported, which matches `ofxParameter.rst:971-982 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L971-L982>`__ for Parametric
     - OK
   * - S7
     - paramGetDerivative `ofxParam.h:1030-1050 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L1030-L1050>`__, paramGetIntegral `ofxParam.h:1052-1073 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L1052-L1073>`__: "Only double and colour params"
     - MUST
     - Double 1/2/3D and RGB/RGBA implemented (OPI.cpp:1025-1047, 1823-1857, 2133-2162, 2508-2533, 3130-3159) via the curve derivative and integral (KnobImpl.h:2353-2420, Simpson's rule for expressions). Integer → ErrUnsupported (hP.cpp:1133-1139)
     - OK
   * - S8
     - paramSetValue / AtTime `ofxParam.h:1075-1117 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L1075-L1117>`__; only in InstanceChanged / interact actions
     - MUST
     - hP.cpp:2639-2711 → ``setValueFromPlugin`` / ``setValueAtTimeFromPlugin``. Natron doesn't reject calls from other actions: writes made where a set isn't possible are queued (KnobImpl.h:1112-1120), and there is a DEBUG-only warning (KnobImpl.h:1037-1044). ``paramChangedByPlugin`` is a no-op, instanceChanged is sent by ``OfxEffectInstance::knobChanged`` (OIEI.cpp:1548-1556). Choice out of range → ErrBadIndex
     - OK
   * - S9
     - Keyframe ordering `ofxParam.h:1120-1131 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L1120-L1131>`__: "time(kf[i]) < time(kf[i+1])", indices stable within an action
     - MUST
     - std::set with epsilon compare (OPI.cpp:66-79)
     - OK
   * - S10
     - paramGetNumKeys `ofxParam.h:1134-1149 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L1134-L1149>`__
     - MUST
     - OPI.cpp:125-136. **An expression on dimension 0 reports exactly 2 fake keys at t=0 and t=1** (OPI.cpp:97-108, deliberate, for GeneratorPlugin isIdentity); paramGetKeyTime then returns these fake times. NULL numberOfKeys is dereferenced (hP.cpp:2729)
     - PARTIAL (P12, deliberate, won't fix; the NULL dereference is fixed by `NatronGitHub/openfx#187 <https://github.com/NatronGitHub/openfx/pull/187>`__, in libs/OpenFX since `#1483 <https://github.com/NatronGitHub/Natron/pull/1483>`__)

       **Deliberate, won't fix.**

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - S11
     - paramGetKeyTime `ofxParam.h:1151-1164 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L1151-L1164>`__: ErrBadIndex if nthKey does not exist
     - MUST
     - OPI.cpp:139-156; unsigned→int cast handled (nth<0 → BadIndex)
     - OK
   * - S12
     - paramGetKeyIndex `ofxParam.h:1167-1185 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L1167-L1185>`__: direction 0/±; **index "set to -1 if no key was found"**; kOfxStatFailed if none
     - MUST
     - OPI.cpp:158-224. (a) **direction<0 fails when a key exists exactly at** ``time``: with keys {1,5} and time=5 it returns kOfxStatFailed instead of index 0 (the loop returns Failed as soon as it reaches the key at ``time``, OPI.cpp:200-201). (b) index is never set to -1 on failure Fixed: the last key before ``time`` is found (a key within 1e-6 of ``time`` is at ``time``), and ``index`` is set to -1 when no key is found.
     - OK (fixed by `#1484 <https://github.com/NatronGitHub/Natron/pull/1484>`__)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - S13
     - paramDeleteKey `ofxParam.h:1187-1198 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L1187-L1198>`__: **ErrBadIndex if no key at the given time**
     - MUST
     - OPI.cpp:226-245 always returns kOfxStatOK (parametric returns BadIndex, ofxhParametricParam.cpp:269-272) Fixed: kOfxStatErrBadIndex when no key is at ``time``.
     - OK (fixed by `#1484 <https://github.com/NatronGitHub/Natron/pull/1484>`__)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - S14
     - paramDeleteAllKeys `ofxParam.h:1200-1212 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L1200-L1212>`__
     - MUST
     - OPI.cpp:247-265 ``removeAnimation`` on each dimension
     - OK
   * - S15
     - paramCopy `ofxParam.h:1216-1237 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L1216-L1237>`__: "copies the value of paramFrom to paramTo, including any animation... All the previous values in paramTo will be lost"; frameRange NULL or **[0,0] = all**; dstOffset
     - MUST
     - OPI.cpp:268-293 loops ``for (i = startDim; i < endDim; ++i)`` with the default startDim=endDim=-1, so **nothing is copied for Integer, Double, Boolean, Choice, RGB, RGBA, Integer3D, String, Custom and Parametric** (OPI.cpp:886, 1223, 1423, 1726, 2043, 2349, 3604, 4297, 4537, 4890), and it returns kOfxStatOK. Only Double2D, Integer2D and Double3D pass explicit dimensions (OPI.cpp:2722, 3002, 3344). Regression from 441a6d3d0 (2016-04-14), which replaced ``to->clone(from, offset, range)`` with this loop. Also: [0,0] is deliberately treated as the range [0,0], not "all" (Curve.cpp:448-450), dstOffset is truncated to an int (``KnobHelper::cloneCurves(..., int offset, ...)``, Knob.cpp:4207), and different types → silent kOfxStatOK
     - WRONG (P1)

       Issue: `#1369 <https://github.com/NatronGitHub/Natron/issues/1369>`__
   * - S16
     - paramEditBegin `ofxParam.h:1240-1258 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L1240-L1258>`__ "start of a set of parameter changes that should be considered part of a single undo/redo block"; ``name`` = "label to attach to any undo/redo string"
     - MUST
     - hP.cpp:2980-2998 → OIEI.cpp:1049-1061 ``setMultipleParamsEditLevel(OnCreateNewCommand)``. Plug-in writes in the block → one ``MultipleKnobEditsUndoCommand`` (KnobImpl.h:1046-1090, Gui/KnobGui20.cpp:750-758). The block survives nested actions since `#1335 <https://github.com/NatronGitHub/Natron/issues/1335>`__ (OIEI.cpp:189-208). ``name`` **is ignored**: the label is always "Multiple edits of <node>" (KnobUndoCommand.cpp:257-264). Also: ``std::string(name)`` with name==NULL (hP.cpp:2992), and the param set handle is reinterpret_cast without checking that it is an instance
     - PARTIAL (P6)

       Issue: `#1373 <https://github.com/NatronGitHub/Natron/issues/1373>`__
   * - S17
     - paramEditEnd `ofxParam.h:1260-1277 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L1260-L1277>`__
     - MUST
     - OIEI.cpp:1066-1077 → EditOff; an unmatched Begin is closed when the outermost main-thread action returns (OIEI.cpp:189-208, `#708 <https://github.com/NatronGitHub/Natron/issues/708>`__)
     - OK
   * - S18
     - `ofxParameter.rst:575-583 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L575-L583>`__: writes outside an edit block "will ordinarily be put on the undo stack, one event per parameter"
     - SHOULD
     - Outside a block, plug-in writes are undoable only during interact actions (KnobImpl.h:1095-1106 ``isDoingInteractAction`` → one ``KnobUndoCommand`` per dimension). Writes made from kOfxActionInstanceChanged (e.g. one param's handler setting others) are never on the undo stack. Undoing the user's edit replays instanceChanged, which usually recomputes them
     - PARTIAL (P8)

       Issue: `#1373 <https://github.com/NatronGitHub/Natron/issues/1373>`__
   * - S19
     - `ofxParameter.rst:572-573 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L572-L573>`__, `ofxParameter.rst:262-269 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L262-L269>`__: params with CanUndo=0 are "excluded from being undone/redone"; the sky-preset example (`ofxParameter.rst:585-600 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L585-L600>`__) mixes a CanUndo=0 preset with undoable params in one block
     - MUST
     - Writes to CanUndo=0 knobs are applied without an undo entry (KnobGui.cpp:169-177): OK. But if the **first** write of a block is to a CanUndo=0 knob, the ``createNew=true`` command is dropped (applied and deleted), and the next writes of the block (``createNew=false``) are merged by ``QUndoStack::push`` into whatever ``MultipleKnobEditsUndoCommand`` is on top of the node's undo stack (KnobUndoCommand.cpp:515-560 merges whenever createNew is false), e.g. a previous edit block of the same node: two undo steps become one
     - WRONG (P7)

       Issue: `#1373 <https://github.com/NatronGitHub/Natron/issues/1373>`__

Table 5: Parameter persistence / project load (`ofxParameter.rst:612-640 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L612-L640>`__)
----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - Spec
     - Level
     - Natron
     - Verdict
   * - L1
     - `ofxParameter.rst:615-621 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L615-L621>`__ persistent params persist; save the plug-in's major version and identifier
     - MUST
     - NodeSerialization.cpp:105-118 (persistent and modified knobs; plug-in ID and version saved by the node). Major-version matching belongs to the loading/plug-in cache area
     - OK
   * - L2
     - `ofxParameter.rst:623-633 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L623-L633>`__ load order: create the instance with its params → set plug-in defaults → set persistent values from the setup → call CreateInstance
     - MUST
     - OEI.cpp:436 ``populate()`` (knobs with plug-in defaults) → :473 ``initializeKnobs`` → :517 ``loadKnobs(serialization)`` → :553 ``createInstanceAction()``
     - OK
   * - L3
     - `ofxParameter.rst:635-640 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L635-L640>`__ "not an error for a parameter to exist in the plugin but not the setup, and vice versa"
     - MUST
     - Unmatched knobs skipped (Node.cpp:1141-1156); unmodified knobs aren't saved, so a new plug-in default applies. Type changes are skipped (Node.cpp:1162-1165)
     - OK
   * - L4
     - Choice restore by index (`ofxParameter.rst:190-193 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L190-L193>`__, 238-242)
     - MUST
     - By option string, then index, with compatibility filters (Node.cpp:1176-1192): stricter than required
     - OK
   * - L5
     - Saved setup → plug-in defaults (step 4)
     - MUST
     - Defaults overwritten by the defaults saved in the project (Node.cpp:1174): see P13
     - PARTIAL (low)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - L6
     - SyncPrivateData before save/copy (`ofxParam.h:283-293 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L283-L293>`__, `ofxParameter.rst:20-25 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L20-L25>`__)
     - MUST
     - NodeSerialization.cpp:64-70 calls ``syncPrivateData_other_thread()`` for every OFX node before serialising (save, copy/paste, PyPlug)
     - OK
   * - L7
     - instanceChanged after project load
     -
     - NEXT, out of scope
     - NEXT

Table 6: ofxParametricParam.h (+ `ofxParameter.rst:929-1117 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L929-L1117>`__)
----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - Spec
     - Level
     - Natron (libs/OpenFX_extensions/ofxhParametricParam.cpp = oPP, OPI)
     - Verdict
   * - R1
     - `ofxParametricParam.h:47-56 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParametricParam.h#L47-L56>`__ ParametricDimension default 1, >0
     - DEFAULT
     - hP.cpp:367
     - OK
   * - R2
     - `ofxParametricParam.h:58-71 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParametricParam.h#L58-L71>`__ ParametricUIColour, default unset, 3 doubles per dimension; "**If not set, the host should generally draw these in white**"
     - SHOULD
     - OPI.cpp:4557-4566 reads 3\*dim values into a zero-filled vector (``Set::getPropertyN`` swallows the BadIndex, hPS.cpp:653-661), so unset colours give **black** (0,0,0) curves, and partly set colours fill the rest with 0. Worse, oPP.cpp:177-187 (the "default colour" code, run for each curve after the constructor) writes its default colour vector **into kOfxParamPropDisplayMax** instead of UIColour, with dim\*3 values (``getDoublePropertyN(kOfxParamPropDisplayMax...)`` / ``setDoublePropertyN(kOfxParamPropDisplayMax, &v[0], dim*3)``). With UIColour unset, DisplayMax becomes [1,0,0,0,1,0,...]: DisplayMax 0 for curves 1 and 2 of a 3D parametric, and the DisplayMax notify hook passes it to the knob (OPI.cpp:4683-4698). Since f2e952a00 (2016-10-15)
     - WRONG (P11)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - R3
     - `ofxParametricParam.h:73-86 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParametricParam.h#L73-L86>`__ ParametricInteractBackground default NULL; host draws the default background; ortho transform in parametric space; no pen/key actions
     - MAY
     - OPI.cpp:4570-4576 → overlay interact on the curve widget
     - OK (transform not checked: interacts area)
   * - R4
     - `ofxParametricParam.h:98-107 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParametricParam.h#L98-L107>`__ ParametricRange default (0,1), first < second
     - DEFAULT
     - hP.cpp:432-435; OPI.cpp:4567-4571 applied only if max>min
     - OK
   * - R5
     - `ofxParametricParam.h:136-139 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParametricParam.h#L136-L139>`__, `ofxParameter.rst:1027-1031 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L1027-L1031>`__ defaults: identity curve unless the plug-in sets control points on the **descriptor**; instances get that curve
     - MUST
     - Descriptor control points stored as ``kOfxParamPropControlPoints_<i>`` and replayed into the instance (oPP.cpp:189-201, OIEI.cpp:680-691 ``defaultInitializeAllCurves`` + ``setDefaultCurvesFromCurves``). Not checked: whether an empty descriptor gives an identity curve rather than an empty one (KnobParametric code)
     - OK (partly checked)
   * - R6
     - parametricParamGetValue `ofxParametricParam.h:143-160 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParametricParam.h#L143-L160>`__: OK / ErrBadHandle / **ErrBadIndex for a bad curve index**
     - MUST
     - oPP.cpp:371-405 → OPI.cpp:4728-4743: a bad curveIndex → ``knob->getValue`` fails → **kOfxStatFailed**, not ErrBadIndex Fixed: every parametric suite function of ``OfxParametricInstance`` returns kOfxStatErrBadIndex for a bad curve index, and for a bad control point index.
     - OK (fixed by `#1484 <https://github.com/NatronGitHub/Natron/pull/1484>`__)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - R7
     - GetNControlPoints `ofxParametricParam.h:163-178 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParametricParam.h#L163-L178>`__ OK/ErrBadHandle/ErrBadIndex
     - MUST
     - oPP.cpp:418-470 checks the index → ErrBadIndex
     - OK
   * - R8
     - GetNthControlPoint `ofxParametricParam.h:180-199 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParametricParam.h#L180-L199>`__ OK/ErrBadHandle/ErrUnknown
     - MUST
     - oPP.cpp:494-560 (ErrBadIndex for a bad curve or point: more precise)
     - OK
   * - R9
     - SetNthControlPoint `ofxParametricParam.h:201-229 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParametricParam.h#L201-L229>`__ (addAnimationKey meaningless without parametric animation)
     - MUST
     - oPP.cpp:619-690 → OPI.cpp:4760-4777
     - OK
   * - R10
     - AddControlPoint `ofxParametricParam.h:231-256 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParametricParam.h#L231-L256>`__: "If a key exists sufficiently close to 'key', then it will be set"
     - MUST
     - OPI.cpp:4779-4815 → ``KnobParametric::addControlPoint`` (replacing a close key not checked); rejects NaN/inf; per-plug-in interpolation hack
     - OK (partly checked)
   * - R11
     - DeleteControlPoint / DeleteAllControlPoints `ofxParametricParam.h:258-274 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParametricParam.h#L258-L274>`__
     - MUST
     - oPP.cpp:997-1130 → OPI.cpp:4817-4850
     - OK
   * - R12
     - `ofxParameter.rst:971-982 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L971-L982>`__: paramGetValue/AtTime/Derivative/Integral/SetValue/AtTime are invalid on parametric params
     - MUST
     - ParametricInstance has no getV/setV → kOfxStatErrUnsupported (hP.cpp:784-816)
     - OK
   * - R13
     - `ofxParameter.rst:973-975 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L973-L975>`__ "All [other] functions in the standard parameter suite are valid" (keys: none without animation; paramCopy)
     - MUST
     - Key functions return "no keys" (oPP.cpp:252-276). paramCopy goes to ``OfxParametricInstance::copyFrom`` → no-op (S15)
     - WRONG (via P1)

       Issue: `#1369 <https://github.com/NatronGitHub/Natron/issues/1369>`__

Problems (MISSING / WRONG / PARTIAL), by severity
-------------------------------------------------

Severity order: crash or data loss > wrong results > spec-only. Each problem lists the second check and the related issues.

Wrong results / data loss
~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem 1.** ``paramCopy`` **copies nothing for 10 of the 13 value types and still returns kOfxStatOK (WRONG).** Spec `ofxParam.h:1216-1237 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L1216-L1237>`__. ``OfxKeyFrame::copyFrom`` (Engine/OfxParamInstance.cpp:268-293) loops ``for (int i = startDim; i < endDim; ++i)`` with the defaults startDim=endDim=-1. Integer, Double, Boolean, Choice, RGB, RGBA, Integer3D, String, Custom and Parametric call it without dimensions (OPI.cpp:886, 1223, 1423, 1726, 2043, 2349, 3604, 4297, 4537, 4890), so neither ``clone()`` nor ``evaluateValueChange()`` runs. Also: frameRange [0,0] is treated as the range [0,0], not "all" (Curve.cpp:445-450, with a comment calling the spec a bug); dstOffset is truncated to an int (Knob.cpp:4207 ``int offset``); different types return OK without copying. **Issue:** `#1369 <https://github.com/NatronGitHub/Natron/issues/1369>`__.

*Second check:* no other override (each OfxXxxInstance::copyFrom calls this helper; HostSupport's base ``Instance::copyFrom`` returns ErrMissingHostFeature, hP.cpp:857, and is overridden). ``KnobI::clone(..., dimension=-1)`` would mean "all dimensions" (Knob.cpp:4214-4219), but the loop never reaches it. ``git show 441a6d3d0`` (2016-04-14, "KnobGuiValue: add support for matrices...") replaced ``to->clone(from, offset, range)`` with the loop: a regression.

**Problem 2.** ``paramGetKeyIndex`` **with direction<0 fails when a key exists at** ``time``\ **, and never sets index=-1 (WRONG).** Spec `ofxParam.h:1167-1185 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L1167-L1185>`__. OPI.cpp:181-208: with keys {1,5} and time=5 the loop doesn't return at i=0 (because ``*next > time`` is false), then sees ``*it == time``, which is not ``< time``, and returns kOfxStatFailed. The correct answer is index 0. This breaks "previous keyframe" navigation from a key, the most common call. On failure ``index`` is left unchanged (the spec says -1). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__. **Fixed** by `#1484 <https://github.com/NatronGitHub/Natron/pull/1484>`__.

*Second check:* all param types funnel into this one helper (OPI.cpp getKeyIndex methods), and HostSupport only forwards to it (hP.cpp:2805-2838). No other implementation.

**Problem 3. Custom param interpolation gets the param handle instead of the param set handle, and its status is ignored (WRONG).** Spec `ofxParam.h:879-902 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L879-L902>`__. OPI.cpp:4352 passes ``(void*)getHandle()`` (an OfxParamHandle) as the ``OfxParamSetHandle instance`` argument. StringAnimationManager.cpp:182-185 ignores the returned status and reads outArgs' CustomValue, whose default is "". With the Support library (ofxsImageEffect.cpp:5951-5968 at 93c21ed7), the handle is passed to ``retrieveImageEffectPointer`` → getPropertySet magic check fails (hIE.cpp:3531-3534) → the plug-in throws internally and returns an error, so between keys the custom param reads "". Meanwhile the host advertises ``kOfxParamHostPropSupportsCustomAnimation=1`` (OfxHost.cpp:303). **Issue:** `#1370 <https://github.com/NatronGitHub/Natron/issues/1370>`__.

*Second check:* ``setCustomInterpolation`` has one caller (OPI.cpp:4352); ``AnimatingKnobStringHelper::getStringFromCurve`` (Knob.cpp:6464-6476) catches only C++ exceptions, not error statuses.

**Problem 4.** ``kOfxParamPropIsAnimating`` **(and IsAutoKeying) is updated only when the knob has a GUI, and only from dimension 0 (WRONG).** Spec `ofxParam.h:297-307 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L297-L307>`__. The only writer is OPI.cpp:360-374, a slot on ``animationLevelChanged``. That signal is emitted only ``if (changed && _signalSlotHandler && hasGui && !hasGui->isGuiFrozenForPlayback())`` (Knob.cpp:3856-3860), and the slot reads ``getAnimationLevel(0)`` only. In NatronRenderer, or before a node's panel exists, IsAnimating stays 0 for animated params; for multi-dimensional params animated only in Y/Z it is 0. A plug-in that uses it to skip per-frame work (e.g. an isIdentity shortcut) gives wrong results in headless renders. **Issue:** `#1371 <https://github.com/NatronGitHub/Natron/issues/1371>`__.

*Second check:* ``git grep kOfxParamPropIsAnimating`` in Engine/Gui/libs/OpenFX_extensions finds only OPI.cpp:372; HostSupport only declares it (hP.cpp:481); there's no GetHook. The GUI gating dates from 2016 (5903f000a/31f8f2580).

**Problem 5. Params with EvaluateOnChange=0 are never undoable (WRONG).** Spec `ofxParam.h:262-269 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L262-L269>`__ (CanUndo governs undo); `ofxParameter.rst:572-573 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L572-L573>`__. ``KnobGui::pushUndoCommand`` (Gui/KnobGui.cpp:169-177) pushes only if ``getCanUndo() && getEvaluateOnChange()``, otherwise it applies and deletes the command. User edits of UI-only OFX params (EvaluateOnChange=0, CanUndo=1 by default) can't be undone. **Issue:** `#1372 <https://github.com/NatronGitHub/Natron/issues/1372>`__.

*Second check:* this is the only push path for KnobGui commands; ``setCanUndo``/``getCanUndo`` have no other users (OIEI.cpp:738, Knob.cpp:3404-3413). The condition dates from at least 24d9073da (``git log -S``).

Wrong undo grouping (no data loss, undo history wrong)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem 7. An edit block whose first write is to a CanUndo=0 param merges into the previous undo entry (WRONG).** Spec `ofxParameter.rst:585-600 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L585-L600>`__ (the sky-preset example mixes these on purpose). KnobImpl.h:1052-1070 emits the block's first write with ``createNew=true``; KnobGui.cpp:169-177 drops it (CanUndo=0); the next writes have ``createNew=false``, and ``MultipleKnobEditsUndoCommand::mergeWith`` (KnobUndoCommand.cpp:515-560) merges them into any ``MultipleKnobEditsUndoCommand`` on top of the node's undo stack, e.g. a previous edit block of the same node. **Issue:** `#1373 <https://github.com/NatronGitHub/Natron/issues/1373>`__.

*Second check:* QUndoStack merges whenever ``id()`` matches (kMultipleKnobsUndoChangeCommandCompressionID) and ``mergeWith`` returns true; nothing resets the "new command" state after the first command is dropped.

*Issues:* related to `#1335 <https://github.com/NatronGitHub/Natron/issues/1335>`__ (closed: the edit-block nesting fix), not the same bug.

**Problem 6. The paramEditBegin label is ignored (PARTIAL).** Spec `ofxParam.h:1240-1258 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L1240-L1258>`__ (``name``: "label to attach to any undo/redo string"). OIEI.cpp:1049 drops ``name``; the undo text is always "Multiple edits of <node>" (KnobUndoCommand.cpp:264). Also, hP.cpp:2992 builds ``std::string(name)`` without a NULL check, and casts the param set handle without checking it. **Issue:** `#1373 <https://github.com/NatronGitHub/Natron/issues/1373>`__.

*Second check:* ``editBegin`` has one override (OIEI.cpp:1049); ``MultipleKnobEditsUndoCommand`` has no other ``setText``.

**Problem 8. Plug-in writes outside an edit block are undoable only in interact actions (PARTIAL).** Spec `ofxParameter.rst:575-578 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L575-L578>`__ ("will ordinarily be put on the undo stack, one event per parameter"). KnobImpl.h:1046-1109: without an edit block, plug-in writes get an undo command only if ``holder->isDoingInteractAction()``. Writes from kOfxActionInstanceChanged are applied without undo; undoing the user's triggering edit replays instanceChanged, which usually recomputes them. **Issue:** `#1373 <https://github.com/NatronGitHub/Natron/issues/1373>`__.

*Second check:* no other path pushes undo commands for eValueChangedReasonPluginEdited outside an edit level.

Spec-only / UI / return codes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem 9.** ``paramDeleteKey`` **returns kOfxStatOK when there is no key at** ``time`` **(WRONG, low).** Spec `ofxParam.h:1192-1196 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L1192-L1196>`__ (ErrBadIndex). OPI.cpp:226-245 always returns OK. *Second check:* one helper for all types; parametric returns BadIndex (ofxhParametricParam.cpp:269-272). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__. **Fixed** by `#1484 <https://github.com/NatronGitHub/Natron/pull/1484>`__.

**Problem 10.** ``paramDefine`` **with an unknown type returns kOfxStatErrUnsupported, not kOfxStatErrUnknown (WRONG, low).** Spec `ofxParam.h:922-927 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L922-L927>`__. hP.cpp:688-689 returns NULL, then hP.cpp:2282-2288 returns Unsupported. *Second check:* Natron's override OIEI.cpp:1524-1543 calls the base class first and doesn't change the status. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__. **Fixed** by `NatronGitHub/openfx#187 <https://github.com/NatronGitHub/openfx/pull/187>`__ (in libs/OpenFX since `#1483 <https://github.com/NatronGitHub/Natron/pull/1483>`__).

**Problem 11. Parametric UI colour: black when unset, and the default-colour code clobbers DisplayMax (WRONG, UI).** Spec `ofxParametricParam.h:58-71 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParametricParam.h#L58-L71>`__. OPI.cpp:4557-4566 reads a zero-filled vector, so unset colours give black curves. ofxhParametricParam.cpp:177-187 reads and writes **kOfxParamPropDisplayMax** where it means kOfxParamPropParametricUIColour (dim\*3 values, pattern [1,0,0,0,1,0,...]), which can set a 0 display max for curves 1 and 2 through the DisplayMax notify hook (OPI.cpp:4683-4698). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

*Second check:* no other code sets UIColour; the block was written in f2e952a00 (2016-10-15, "ofxhParametricParam: many fixes"). Plug-ins that set UIColour for every curve (most openfx-misc ones) are not affected.

**Problem 12. paramGetNumKeys/KeyTime report 2 fake keys (t=0, t=1) when dimension 0 has an expression (PARTIAL, deliberate).** Spec `ofxParam.h:1134-1164 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L1134-L1164>`__. OPI.cpp:97-108 (documented workaround so that GeneratorPlugin isIdentity sees animation). The expression on dimension 0 is used for every dimension. A NULL ``numberOfKeys`` is dereferenced (hP.cpp:2729). The fake keys are deliberate, won't fix; the NULL dereference is in `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

**Problem 13. kOfxPropParamSetNeedsSyncing can't be set by the plug-in (PARTIAL).** Spec `ofxParam.h:271-295 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L271-L295>`__. Not declared on instance properties, so propSet returns ErrUnknown (hPS.cpp:860-864); hIE.cpp:1327 never finds it, so the host always calls SyncPrivateData (allowed by the "not set" branch). *Second check:* ``git grep`` in HostSupport and Engine. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

**Problem 14. kOfxParamPropChoiceOrder is accepted but ignored (PARTIAL).** Spec `ofxParam.h:630-666 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L630-L666>`__, `ofxParameter.rst:197-261 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L197-L261>`__ (plug-ins detect support by setting the property). HostSupport declares it (hP.cpp:334,342), so the set succeeds, but Natron never reads it. No project risk (Natron restores choices by option string, Node.cpp:1176-1192); the menu order is wrong. *Issues:* `#1137 <https://github.com/NatronGitHub/Natron/issues/1137>`__, `#1106 <https://github.com/NatronGitHub/Natron/issues/1106>`__. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__, `#1137 <https://github.com/NatronGitHub/Natron/issues/1137>`__, `#1106 <https://github.com/NatronGitHub/Natron/issues/1106>`__.

**Problem 15. Custom params are always hidden and disabled, so their InteractV1 is never shown (PARTIAL).** Spec `ofxParameter.rst:381-394 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L381-L394>`__. OIEI.cpp:618-619 forces it; OPI.cpp custom setSecret/setEnabled are no-ops. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

**Problem 16. Param interact size properties are read from and written to the interact descriptor, not the param (PARTIAL).** Spec `ofxParam.h:200-240 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L200-L240>`__. Values the plug-in sets on the param descriptor (Aspect, MinimumSize, PreferedSize) are ignored; the instance's InteractSize is never updated; the aspect is read with ``getIntProperty`` on a double, which returns 0 (CustomParamInteract.cpp:76, OfxOverlayInteract.cpp:358-394, OEI.cpp:769-771). Hidden by the Support library, which sets them on the interact descriptor. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__. The sizes are read from the parameter, and the current size is set on it, since `#1484 <https://github.com/NatronGitHub/Natron/pull/1484>`__; the aspect is still not applied.

**Problem 17. IsAutoKeying means "between keys" (PARTIAL).** Spec `ofxParam.h:777-785 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L777-L785>`__. OPI.cpp:373; same GUI/dimension-0 limits as problem 4. **Issue:** `#1371 <https://github.com/NatronGitHub/Natron/issues/1371>`__.

**Problem 18. Setting Min/Max doesn't reset DisplayMin/DisplayMax (PARTIAL, low).** Spec `ofxParam.h:710 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L710>`__, 720. No such code in hP.cpp notify or in the OPI setRange methods. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

**Problem 19. Project load replaces plug-in defaults with the defaults saved in the project (PARTIAL, low).** Spec `ofxParameter.rst:630 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxParameter.rst?plain=1#L630>`__ (step 4). Node.cpp:1174 ``cloneDefaultValues``; this only affects "reset to default" for knobs saved in the project. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

**Problem 20. Small ones (PARTIAL, low):** ScriptName ignored (Python uses the param name); ShowTimeMarker ignored; parametricParamGetValue returns kOfxStatFailed instead of ErrBadIndex for a bad curve index (OPI.cpp:4728-4743); "instance (read only)" properties are writable on instances with no effect (hP.cpp:272-300, 481-488). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__. The parametricParamGetValue bad curve index is fixed by `#1484 <https://github.com/NatronGitHub/Natron/pull/1484>`__.

NEXT items seen (out of scope): kOfxParamInterpType (HostSupport hP.cpp:629-635, `#1138 <https://github.com/NatronGitHub/Natron/issues/1138>`__); instanceChanged after project load.

Counts
------

Table rows (105): OK 75, PARTIAL 16, WRONG 8, N/A 4, NEXT 2. Some bugs appear in more than one row: paramCopy in S15 and R13, custom interpolation in T15 and P32, ChoiceOrder in T9 and P25.

Distinct problems (1-20): 9 WRONG (1, 2, 3, 4, 5, 7, 9, 10, 11), 11 PARTIAL (6, 8, 12-20).
