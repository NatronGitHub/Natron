.. _ofx-host-conformance-interact-gpu-packaging:

OFX host conformance: interacts, GPU, colour and packaging
==========================================================

This page is part of the :ref:`OpenFX 1.5.1 host conformance audit <ofx-host-conformance>`.

**Spec scope.** The headers ``ofxInteract.h``, ``ofxKeySyms.h``, ``ofxDrawSuite.h``,
``ofxGPURender.h`` and ``ofxColour.h``; the Reference pages ``ofxInteracts.rst``,
``ofxInteractActions.rst``, ``suites/ofxDrawSuiteReference.rst`` and the interact sections of
``ofxPropertiesByObject.rst``; and, for binary loading, the plug-in cache and plug-in versions,
``ofxPackaging.rst``, ``ofxPluginStruct.rst`` and ``ofxLoadingSequence.rst``.
Section 3 describes pull request `#1349 <https://github.com/NatronGitHub/Natron/pull/1349>`__ (DrawSuite support, branch ``ofx15-draw-suite``), which
was open and not yet on RB-2.6 at the time of the audit. It has since been merged into RB-2.6.

**Spec reference abbreviations.** A bare ``:line`` in the spec column is in the header of that
section (``ofxInteract.h``, ``ofxDrawSuite.h``, ``ofxGPURender.h`` or ``ofxColour.h``).

**Code reference abbreviations.** HS is HostSupport (``libs/OpenFX/HostSupport/src/``); ofxhI,
ofxhIE, ofxhPC and ofxhAPI are its ``ofxhInteract.cpp``, ``ofxhImageEffect.cpp``,
``ofxhPluginCache.cpp`` and ``ofxhImageEffectAPI.cpp``. Natron paths are relative to the
repository root, or named without their directory.

Verdicts: **OK**; **MISSING** (not implemented); **WRONG** (implemented, but contradicts the spec); **PARTIAL** (partly implemented, or only in some cases); **N/A** (does not apply to Natron); **NEXT** (seen, but out of scope of this audit). The level column gives the strength of the spec statement (MUST, SHOULD, MAY) or DEFAULT for a default value. Spec references link to the upstream ``OFX_Release_1.5.1`` tag. Code references are plain paths with line numbers at the audited commits (Natron RB-2.6 at 7dea437a6, HostSupport at 93c21ed7); they are not updated when the code moves. Each row or problem that is not OK links to the issue that tracks it; items marked **deliberate, won't fix** are known deviations that Natron keeps on purpose.

1. ofxInteract.h / ofxInteracts.rst / ofxInteractActions.rst
------------------------------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron + HostSupport
     - verdict
   * - I1
     - `ofxInteract.h:35-48 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L35-L48>`__ kOfxInteractPropSlaveToParam, "interact instance property (read/write)", default none; "will need to be redrawn if any of those parameters' values change"
     - DEFAULT / SHOULD
     - HS ofxhI:124 declares it RW, dim 0. Overlay: ``OfxEffectInstance::tryInitializeOverlayInteracts`` (Engine/OfxEffectInstance.cpp:731-746) reads it once, after createInstance; ``EffectInstance.cpp:4822`` redraws when a slave knob changes. Natron also redraws on any param change of an effect with an overlay (EffectInstance.cpp:4813-4820). Param interacts: the property is never read; KnobGui.cpp:261-264 only redraws on the interact's own param. Later changes of the property are not followed.
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - I2
     - `ofxInteract.h:50-56 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L50-L56>`__ kOfxInteractPropPixelScale, double X2, "interact instance and actions (read only)": "size of a real screen pixel under the interact's canonical projection"
     - MUST (valid value)
     - Hooked on instance and args (ofxhI:119,151,183,200). Viewer: ``ViewerGL::getPixelScale`` (Gui/ViewerGL.cpp:3630) = 1/zoom, in Qt logical pixels, not device pixels (the Natron extension kOfxInteractPropScreenPixelRatio gives the ratio). Param interact: 1,1 (Gui/CustomParamInteract.cpp:192), consistent with its pixel projection.
     - PARTIAL (HiDPI: logical pixels, see problem 21)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - I3
     - `ofxInteract.h:60-69 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L60-L69>`__ kOfxInteractPropBackgroundColour, double X3, 0..1, RO on instance and in Draw args
     - MUST
     - Hooked (ofxhI:120,147,184,201). Viewer: clear colour (ViewerGL.cpp:3660). Param interact: 0,0,0 (CustomParamInteract.cpp:208).
     - OK
   * - I4
     - `ofxInteract.h:71-83 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L71-L83>`__ kOfxInteractPropSuggestedColour, RO, default 1.0; "If a host does not support such a colour, it should return kOfxStatReplyDefault"
     - SHOULD
     - HS ofxhI:125 dim 0/3 via getDimension hook (ofxhI:235-247); getters throw ``Property::Exception(kOfxStatReplyDefault)`` when unsupported (ofxhI:300,338). Overlay: node overlay colour (Engine/OfxOverlayInteract.cpp:229-239); param interact: none (OfxOverlayInteract.cpp:349-355).
     - OK
   * - I5
     - `ofxInteract.h:86-93 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L86-L93>`__ kOfxInteractPropPenPosition, canonical coords, in Pen\* args
     - MUST
     - ofxhI:422-429; viewer passes canonical pos (transformed for NATRON_TRANSFORM_AFFECTS_OVERLAYS); param interact: widget pixels, y flipped (CustomParamInteract.cpp:323-330), which is its canonical space.
     - OK
   * - I6
     - `ofxInteract.h:95-101 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L95-L101>`__ kOfxInteractPropPenViewportPosition, int X2, "openGL viewport coordinates, with 0,0 being at the bottom left"
     - MUST (valid value)
     - Viewer overlays: ``ViewerGL`` passes ``e->localPos()`` (ViewerGL.cpp:1745,1868,2071,2503), i.e. Qt widget coordinates, origin top-left, y down, logical pixels; ``ViewerTab::notifyOverlaysPen*`` and ``EffectInstance::onOverlayPenDown_public`` (EffectInstance.cpp:3324-3354) and ``OfxEffectInstance::onOverlayPenDown`` (OfxEffectInstance.cpp:2259-2262) copy it unchanged. Param interacts: y flipped (CustomParamInteract.cpp:327-329).
     - WRONG (viewer)

       Issue: `#1378 <https://github.com/NatronGitHub/Natron/issues/1378>`__
   * - I7
     - `ofxInteract.h:104-112 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L104-L112>`__ kOfxInteractPropPenPressure, 0..1; "If a pen has only two states (eg: a mouse button), these should map to 0.0 and 1.0"
     - SHOULD
     - Viewer: mouse move always 1.0, also with no button down (ViewerGL.cpp:2088, mouse tracking on, :129); mouse release 1.0 (``pressureOnRelease = 1.`` ViewerGL.cpp:1759); tablets OK (ViewerGL.cpp:2106-2131). Param interact: 1.0 for down, motion and up (CustomParamInteract.cpp:330,349,368).
     - WRONG

       Issues: `#1378 <https://github.com/NatronGitHub/Natron/issues/1378>`__, `#1379 <https://github.com/NatronGitHub/Natron/issues/1379>`__
   * - I8
     - `ofxInteract.h:114-130 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L114-L130>`__ kOfxInteractPropBitDepth / HasAlpha, descriptor+instance RO
     - MUST
     - HS ofxhI:61-63, set in ``Descriptor::describe`` (ofxhI:82-97), instance chains to descriptor (ofxhI:182). Overlay: 8, alpha (OfxEffectInstance.cpp:714); param: 8, no alpha (OfxEffectInstance.cpp:775), matching "double buffered 24 bit RGB" (`ofxInteracts.rst:140 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxInteracts.rst?plain=1#L140>`__).
     - OK
   * - I9
     - `ofxInteract.h:146-169 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L146-L169>`__ Describe: inArgs/outArgs NULL; kOfxStatFailed "the host should ignore the interact"; ErrMemory "describe may be called again after a memory purge"
     - MUST / MAY
     - ofxhI:88 passes NULL,NULL; failure sets eFailed and every later callEntry returns kOfxStatFailed (ofxhI:93,105-109,176). No retry after ErrMemory (MAY).
     - OK
   * - I10
     - `ofxInteracts.rst:184 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxInteracts.rst?plain=1#L184>`__ "The host must first call the effect's Describe action before calling its interact Describe action"
     - MUST
     - Overlay descriptor is described lazily from the described effect descriptor (ofxhIE:568-576); param interacts described in tryInitializeOverlayInteracts after instance creation (OfxEffectInstance.cpp:770-775).
     - OK
   * - I11
     - `ofxInteract.h:172-195 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L172-L195>`__ CreateInstance: NULL args, "kOfxStatFailed in which case the host should ignore this interact"
     - MUST
     - ofxhI:438-448: OK/ReplyDefault -> eCreated else eFailed, later calls return Failed. Natron ignores the return value but the state guards it.
     - OK
   * - I12
     - `ofxInteract.h:198-224 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L198-L224>`__ DestroyInstance: pre "kOfxActionCreateInstance has been called"; host "should continue destruction regardless"
     - MUST
     - ofxhI:207-214 only if eCreated, return ignored.
     - OK
   * - I13
     - `ofxInteracts.rst:186-187 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxInteracts.rst?plain=1#L186-L187>`__ "An interact instance must always be associated with an effect instance, so it gets created after the effect and destroyed before the effect."
     - MUST
     - Overlay: ``~OfxEffectInstance`` resets the overlay before ``destroyInstanceAction`` (OfxEffectInstance.cpp:632-638): OK. Param interacts: owned by ``shared_ptr`` in the knob (``KnobHelper::setCustomInteract``, Engine/Knob.cpp:3463), in ``CustomParamInteractPrivate::entryPoint`` and in ``CurveWidget`` (Gui/CurveWidget.cpp:2322). They are destroyed with the knobs, i.e. after ``~OfxEffectInstance`` has sent the effect's kOfxActionDestroyInstance and deleted ``_imp->effect`` (knobs live in the KnobHolder base, Knob.cpp:5019), or later if the GUI still holds them. Their kOfxActionDestroyInstance then runs on an interact whose kOfxPropEffectInstance points to a deleted instance.
     - WRONG

       Issue: `#1362 <https://github.com/NatronGitHub/Natron/issues/1362>`__
   * - I14
     - `ofxInteract.h:227-264 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L227-L264>`__ Draw inArgs: kOfxPropEffectInstance, PixelScale, BackgroundColour, kOfxPropTime, RenderScale
     - MUST
     - ``Instance::initArgProp`` (ofxhI:368-420) sets all of them (+ Nuke view, Natron picker colour, screen ratio, key modifiers, kOfxPropInstanceData).
     - OK
   * - I15
     - `ofxInteract.h:254-257 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L254-L257>`__ Draw pre: "the openGL context for this interact has been set", "projection matrix will correspond to the interact's canonical view"; `ofxInteracts.rst:92-95 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxInteracts.rst?plain=1#L92-L95>`__ overlay GL_PROJECTION canonical, GL_MODELVIEW identity; `ofxInteracts.rst:134-138 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxInteracts.rst?plain=1#L134-L138>`__ param interact ortho -0.5..w-0.5
     - MUST
     - Overlay draw runs in ViewerGL::paintGL with the viewer projection; ``OGLContextSaver`` saves/restores state (OfxOverlayInteract.cpp:44-54,121-130). Param: glOrtho(-0.5, w-0.5, -0.5, h-0.5), modelview identity (CustomParamInteract.cpp:120-128).
     - OK
   * - I16
     - `ofxInteract.h:263 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L263>`__ Draw returns kOfxStatFailed: "the host should ignore this interact in future"
     - SHOULD
     - HS drawAction (ofxhI:450-489) does not change ``_state``; ``OfxEffectInstance::drawOverlay`` (OfxEffectInstance.cpp:2220-2232) and ``CustomParamInteract::paintGL`` (:133) ignore the status, so the interact keeps being drawn and receiving events.
     - MISSING

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - I17
     - `ofxInteract.h:236-238 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L236-L238>`__ / `ofxInteracts.rst:85 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxInteracts.rst?plain=1#L85>`__ "If this is called via ...OverlayInteractV2, drawing MUST use DrawSuite"; V1 SHOULD use OpenGL
     - MUST (host side: choose entry)
     - HS ``Base::getOverlayInteractMainEntry`` uses V2 only if ``gImageEffectHost->supportsDrawSuite()`` (ofxhIE:278-301). RB-2.6 Natron does not override ``supportsDrawSuite`` (default false, ofxhIE:4792-4795), so V1 is used, a V2-only plug-in gets no overlay, and fetchSuite(kOfxDrawSuite) returns NULL (ofxhIE:4833-4836). Consistent: Natron does not claim DrawSuite. See section 3 for `#1349 <https://github.com/NatronGitHub/Natron/pull/1349>`__. Since #1349 was merged, ``supportsDrawSuite()`` returns true: a V2-only plug-in gets its V2 overlay, and a plug-in with both gets V1 (``preferOverlayInteractV1()``, Engine/OfxHost.h).
     - OK (DrawSuite not supported; `#1103 <https://github.com/NatronGitHub/Natron/issues/1103>`__/#1136)
   * - I18
     - `ofxInteract.h:240-242 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L240-L242>`__, `ofxInteracts.rst:97-100 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxInteracts.rst?plain=1#L97-L100>`__ overlays must not swap buffers; `ofxInteracts.rst:142-144 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxInteracts.rst?plain=1#L142-L144>`__ "A parameter's interact draw function will have full responsibility for ... swapping buffers"
     - MUST (host provides swap for param interacts)
     - interactSwapBuffers -> ``n_swapBuffers`` -> ``CustomParamInteract::swapOpenGLBuffers`` = ``update()`` (CustomParamInteract.cpp:172-175). QOpenGLWidget swaps itself after paintGL; calling ``update()`` from inside paintGL (where a conforming param interact calls swapBuffers) schedules another paint, i.e. a continuous repaint loop. No known plug-in calls interactSwapBuffers (grep of openfx Examples, openfx-misc, openfx-io, openfx-arena), so this is not observed.
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - I19
     - `ofxInteract.h:267-302 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L267-L302>`__ PenMotion: "issued whenever the pen moves an the interact's has focus ... whether the pen is currently up or down"; inArgs incl. PenPosition, PenViewportPosition, PenPressure; pre: GainFocus called
     - MUST (args) / SHOULD (focus)
     - Args: ofxhI:522-553 (see I6, I7 for the values). Focus: viewer overlays get GainFocus when the ViewerGL gets Qt focus (ViewerGL.cpp:3112-3124, ``setFocus()`` on press :1770; focus policy commented out :128), but PenMotion is sent on every hover with mouse tracking, before any click, and interacts created while the viewer already has focus never get GainFocus. Param interacts never get focus (I24).
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - I20
     - `ofxInteract.h:293-300 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L293-L300>`__, 331-338, 367-374 post: "if the instance returns kOfxStatOK the host should not pass the pen motion to any other interactive object" (PenMotion/Down/Up)
     - SHOULD
     - PenDown/PenMotion/KeyDown/KeyRepeat stop at the first overlay that traps (``return true``, Gui/ViewerTab20.cpp:236,284,446,483,820,996). PenUp and KeyUp are sent to all overlays (``didSomething |= caught``, ViewerTab20.cpp:688-690, 920-923), deliberately, so that every overlay sees the release.
     - PARTIAL (deliberate, won't fix)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - I21
     - `ofxInteract.h:378-476 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L378-L476>`__ KeyDown/Up/Repeat inArgs: kOfxPropKeySym, kOfxPropKeyString ("UTF8 string representing a character key that was pressed ... in which case this is \\"\\""), Time, RenderScale
     - MUST
     - kOfxPropKeyString is always "": every caller passes ``keyStr.data()`` of an empty ``QByteArray`` (OfxEffectInstance.cpp:2414-2417, 2443-2446, 2474-2478; CustomParamInteract.cpp:407-412, 426-427). The Engine API (``onOverlayKeyDown(time, scale, view, Key, modifiers)``) has no text argument, so the GUI can't pass ``QKeyEvent::text()``. Unchanged since e1a5140c5 ("Gui/Engine separation").
     - WRONG

       Issues: `#1378 <https://github.com/NatronGitHub/Natron/issues/1378>`__, `#1379 <https://github.com/NatronGitHub/Natron/issues/1379>`__
   * - I22
     - `ofxKeySyms.h:13-29 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxKeySyms.h#L13-L29>`__, 108-118 kOfxPropKeySym: "raw key press", values from ofxKeySyms.h; kOfxKey_Unknown when the key has a UTF8 value but no keysym
     - MUST (valid values)
     - ``QtEnumConvert::fromQtKey`` (Gui/QtEnumConvert.cpp:35-708) maps Qt keys to the kOfxKey\_\* values of Global/KeySymbols.h; unmapped keys -> Key_Unknown (0). Letters are always the uppercase keysym (Qt::Key_A -> kOfxKey_A) regardless of Shift, which fits "raw key press".
     - OK
   * - I23
     - `ofxInteract.h:478-526 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L478-L526>`__ GainFocus/LoseFocus inArgs (EffectInstance, PixelScale, BackgroundColour, Time, RenderScale); `ofxInteracts.rst:189-191 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxInteracts.rst?plain=1#L189-L191>`__ "should be issued a gain focus action before any key or pen actions ... and a lose focus action when it goes"
     - SHOULD
     - Args: ofxhI:717-777. Viewer: see I19.
     - PARTIAL (see I19)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - I24
     - `ofxInteracts.rst:161-182 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxInteracts.rst?plain=1#L161-L182>`__ param interacts receive pen/key/focus actions "whenever the interact has the input focus"
     - SHOULD
     - ``CustomParamInteract`` (QOpenGLWidget) never sets a focus policy (no ``setFocusPolicy`` in Gui/CustomParamInteract.cpp or Gui/KnobGui\*.cpp; Qt default is Qt::NoFocus), and does not call ``setFocus()`` on press. So focusInEvent/keyPressEvent (CustomParamInteract.cpp:375-430) never run: param interacts never get GainFocus/LoseFocus or any key action; pen actions arrive without GainFocus.
     - MISSING

       Issue: `#1379 <https://github.com/NatronGitHub/Natron/issues/1379>`__
   * - I25
     - `ofxInteracts.rst:63-70 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxInteracts.rst?plain=1#L63-L70>`__ one interact instance used in several views, "the relevant properties controlling the view being changed before any action"
     - MUST
     - ``setCurrentViewportForOverlays`` before each viewer's actions (OfxEffectInstance.cpp:2235-2240, ViewerTab20.cpp:687); hooks read the calling viewport.
     - OK
   * - I26
     - `ofxInteract.h:531-543 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxInteract.h#L531-L543>`__ OfxInteractSuiteV1: swapBuffers, redraw, getPropertySet
     - MUST
     - ofxhI:784-829, version 1 only (ofxhI:977-981). redraw: overlay -> redraw/queue all viewers (OfxOverlayInteract.cpp:242-259); param -> ``update()`` (CustomParamInteract.cpp:178). getPropertySet returns the instance (or descriptor) property set.
     - OK
   * - I27
     - `ofxParam.h:130-139 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L130-L139>`__ kOfxParamHostPropSupportsCustomInteract; `ofxInteracts.rst:112-119 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxInteracts.rst?plain=1#L112-L119>`__ the interact "will completely replace the parameters default user interface in the 'paged' and hierarchical interfaces, but ... not ... in any animation sheet"
     - MUST (if advertised)
     - Host sets 1 (Engine/OfxHost.cpp:299). KnobGui replaces the widget by CustomParamInteract (Gui/KnobGui.cpp:231-241), the curve editor keeps the param.
     - OK
   * - I28
     - `ofxParam.h:207 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L207>`__ kOfxParamPropInteractSize "set by a host to indicate the current size of a custom interface ... plugin parameter instance (read only)", double x2
     - MUST
     - Natron stores the size as an int property on the *interact descriptor* (``OfxParamOverlayInteract::setSize``, Engine/OfxOverlayInteract.cpp:381-387; spec added by ``getOfxParamOverlayInteractDescProps``, Engine/OfxImageEffectInstance.cpp:523-535). The param instance's kOfxParamPropInteractSize (HS ofxhParam.cpp:462) stays (0,0).
     - WRONG

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - I29
     - `ofxParam.h:218-240 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxParam.h#L218-L240>`__ kOfxParamPropInteractSizeAspect ("the custom interface ... will be of a size with this aspect ratio"), MinimumSize ("Any custom interface will not be less than this size"), PreferedSize ("A host should attempt to set ... to be this size") — param descriptor properties
     - MUST / SHOULD
     - The plug-in sets these on the *param* descriptor (HS ofxhParam.cpp:463-465). Natron reads them from the interact descriptor (OfxOverlayInteract.cpp:357-393; CustomParamInteract.cpp:76,93-94,166-169), where ``getOfxParamOverlayInteractDescProps`` declared them with defaults (1, 10x10, 10x10) and nothing copies the plug-in values (only writer of these names in Engine/Gui). So the plug-in's minimum/preferred size and aspect are ignored; the aspect is also read with getIntProperty on a double property (returns 0) and never applied.
     - WRONG

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__

2. ofxDrawSuite.h (RB-2.6)
--------------------------

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
     - `ofxDrawSuite.h:21-22 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxDrawSuite.h#L21-L22>`__, ofxCore "If the host does not implement a requested suite ... it should return NULL"
     - MUST
     - fetchSuite(kOfxDrawSuite) returns NULL unless ``supportsDrawSuite()`` (ofxhIE:4833-4836); RB-2.6 Natron doesn't override it. Since #1349 was merged, ``supportsDrawSuite()`` returns true and fetchSuite(kOfxDrawSuite, 1) succeeds.
     - OK (not supported)
   * - D2
     - `ofxDrawSuite.h:28-34 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxDrawSuite.h#L28-L34>`__ kOfxInteractPropDrawContext, pointer, RO in Draw inArgs
     - MUST (if supported)
     - HS sets it to the interact instance during Draw only (ofxhI:138, 473-487). Declared even when the suite is unavailable (harmless).
     - N/A
   * - D3
     - `ofxDrawSuite.h:86-175 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxDrawSuite.h#L86-L175>`__ getColour/setColour/setLineWidth/setLineStipple/draw/drawText return codes (OK, ErrValue, Failed outside Draw)
     - MUST (if supported)
     - HS wrappers check the context and Draw (ofxhI:837-951), validate std_colour, pattern, primitive/point counts (Lines even, Rectangle/Ellipse exactly 2, Polygon >= 3), text/pos; default hooks return kOfxStatErrMissingHostFeature (ofxhI:491-520). Unreachable in RB-2.6 (D1).
     - N/A
   * - D4
     - `ofxDrawSuite.h:82-84 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxDrawSuite.h#L82-L84>`__ "To use this, the plugin must use kOfxImageEffectPluginPropOverlayInteractV2"
     - —
     - V2 not used in RB-2.6 (I17).
     - N/A

3. What PR `#1349 <https://github.com/NatronGitHub/Natron/pull/1349>`__ changes (open, head ``ofx15-draw-suite``)
-------------------------------------------------------------------------------------------------------------------

#1349 has since been merged into RB-2.6. As merged, a plug-in that has both a V1 and a V2 overlay gets the V1 (OpenGL) one (``OfxHost::preferOverlayInteractV1()``).

- ``OfxHost::supportsDrawSuite()`` returns true (Engine/OfxHost.h), so fetchSuite(kOfxDrawSuite,1) succeeds and V2 overlays are used, V2 preferred when a plug-in has both (Tests/OfxDrawSuite_Test.cpp ``OverlayEntryPoint``). Closes I17/D1 (`#1103 <https://github.com/NatronGitHub/Natron/issues/1103>`__, `#1136 <https://github.com/NatronGitHub/Natron/issues/1136>`__).

- New ``OfxDrawSuiteGL`` (Engine/OfxDrawSuiteGL.cpp) draws in the calling viewport's GL context, for overlay interacts and (beyond the spec, harmless) param interacts; state reset to opaque white, width 0, solid at each Draw (``beginDraw``).

  - getColour: Background black, Active green, Selected/MarqueeFG = selection colour, MarqueeBG = selection colour at 0.25 alpha, Deselected/Text white; invalid -> ErrValue. OK.

  - setColour; draw with "over" blending (GL_SRC_ALPHA, ONE_MINUS_SRC_ALPHA) for lines and fills. OK.

  - setLineWidth: 0 -> 1 device pixel, non-zero -> GL_LINE_SMOOTH with width x screen pixel ratio ("The host should adjust for screen density"). Negative/NaN/inf -> kOfxStatErrValue (not in the spec's return list; harmless). OK.

  - setLineStipple: glLineStipple patterns scaled by the pixel ratio. OK.

  - draw: Lines/LineStrip/LineLoop/Polygon as the GL primitives, Rectangle as a filled quad from 2 corners, Ellipse as an unfilled line loop inside the 2-corner box. OK.

  - drawText: host font, alignment flags incl. CenterH (Left\|Right) and CenterV (Top\|Baseline). **The colour's alpha is ignored** (``renderText(..., r, g, b, 0)``), although "Sets the colour for future drawing operations (lines, filled shapes and text) ... The host should use "over" compositing when using a non-opaque colour" (`ofxDrawSuite.h:99-104 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxDrawSuite.h#L99-L104>`__). PARTIAL. Returns kOfxStatFailed if the viewport can't draw text.

- Not changed by `#1349 <https://github.com/NatronGitHub/Natron/pull/1349>`__: I6, I7, I13, I16, I18-I24, I28, I29 (it only adds DrawSuite hooks and the descent helper).

4. ofxGPURender.h
-----------------

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
     - `ofxGPURender.h:41-59 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxGPURender.h#L41-L59>`__ kOfxImageEffectPropOpenGLRenderSupported, host descriptor RO; plug-in default "false"; V1.4 instance may switch
     - MUST / DEFAULT
     - Host "true" (Engine/OfxHost.cpp:312-320, static so the cache stays valid, even when OpenGL is disabled in the preferences or unavailable); plug-in default "false" (ofxhIE:111); instance value read at render time (``OfxEffectInstance::supportsOpenGLRender``, OfxEffectInstance.cpp:2839-2853; any value other than false/true is treated as "needed", assert in debug). A "needed" plug-in on a host without usable GL describes fine and fails at render (EffectInstanceRenderRoI.cpp:815-818).
     - PARTIAL

       Issue: `#1384 <https://github.com/NatronGitHub/Natron/issues/1384>`__
   * - G2
     - `ofxGPURender.h:63-87 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxGPURender.h#L63-L87>`__ kOfxOpenGLPropPixelDepth: "the host will try to provide buffers/textures in one of the supported formats"
     - SHOULD
     - Declared (ofxhIE:113); HS ``isOpenGLPixelDepthSupported`` (ofxhIE:329-334) is never called by Natron. Output textures are always float (OfxClipInstance.cpp:1046 "ignore requested texture depth"); input textures get the ``format`` argument if given.
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - G3
     - `ofxGPURender.h:90-114 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxGPURender.h#L90-L114>`__ kOfxImageEffectPropOpenGLEnabled in Render/BeginSequence/EndSequence inArgs
     - MUST
     - ofxhIE:1470,1508,1576,1614,1699,1729; Natron passes args.useOpenGL / isOpenGLRender.
     - OK
   * - G4
     - `ofxGPURender.h:118-150 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxGPURender.h#L118-L150>`__ texture handle props TextureIndex/Target (RO) and the clipGetImage set
     - MUST
     - OfxClipInstance.cpp:1379-1380 + the image property set.
     - OK
   * - G5
     - `ofxGPURender.h:153-167 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxGPURender.h#L153-L167>`__ kOfxImageEffectPropCPURenderSupported (1.5.1), host RO, default "true"
     - DEFAULT
     - Host "true", plug-in default "true" (ofxhIE:165,4745). A plug-in with CPU "false" and OpenGL "true" is treated as GL-needed (``OfxHost::applyCPURenderSupport``, OfxHost.cpp:359-372). With CPU "false" and OpenGL "false" (CUDA/Metal/OpenCL-only plug-in), Natron still renders it on the CPU.
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - G6
     - `ofxGPURender.h:171-191 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxGPURender.h#L171-L191>`__ "If a plug-in returns kOfxStatGLRenderFailed, the host should retry the render with OpenGL rendering disabled"; GLOutOfMemory: "may ... free resources on the GPU and retry"
     - SHOULD / MAY
     - ``OfxEffectInstance::render`` maps every non-OK status to eStatusFailed (OfxEffectInstance.cpp:2126-2147); only an attach returning kOfxStatErrMemory leads to the CPU retry (EffectInstanceRenderRoI.cpp:1626-1636, 1720-1729). 1001/1002 appear nowhere in Engine. Re-verified: still open.
     - MISSING (`#1359 <https://github.com/NatronGitHub/Natron/issues/1359>`__)
   * - G7
     - `ofxGPURender.h:199-282 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxGPURender.h#L199-L282>`__ clipLoadTexture: format honoured, Output clip binds the render target (may be done before Render), region clipped to RoD / default RoI; returns Failed (black), ErrBadHandle, ErrMemory
     - MUST
     - ofxhIE:4182-4223 -> ``OfxClipInstance::loadTexture`` (OfxClipInstance.cpp:703-719) via the clipGetImage path; output FBO bound before render (EffectInstance.cpp:2650). Every failure returns kOfxStatFailed (ofxhIE:4205-4209): kOfxStatErrMemory is never returned.
     - PARTIAL

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - G8
     - `ofxGPURender.h:284-305 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxGPURender.h#L284-L305>`__ clipFreeTexture
     - MUST
     - ofxhIE:4225-4246 releases the handle (cache-backed textures; host keeps output).
     - OK
   * - G9
     - `ofxGPURender.h:308-324 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxGPURender.h#L308-L324>`__ flushResources: kOfxStatOK "the host has actually released some resources", kOfxStatReplyDefault "nothing the host could do"
     - MUST (return semantics)
     - ``OfxHost::flushOpenGLResources`` returns kOfxStatOK and releases nothing (Engine/OfxHost.cpp:1766-1770); the only override. Fixed: ``flushOpenGLResources()`` returns kOfxStatReplyDefault.
     - OK (fixed by `#1483 <https://github.com/NatronGitHub/Natron/pull/1483>`__)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - G10
     - `ofxGPURender.h:329-360 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxGPURender.h#L329-L360>`__ ContextAttached: inArgs/outArgs NULL; no second attach without detach; kOfxStatFailed -> "the host should not attempt to run the plug-in in OpenGL render mode"
     - MUST / SHOULD
     - Attach/detach bracket per render or per context (EffectInstanceRenderRoI.cpp:1626-1656). Natron passes a Natron-extension outArgs (kNatronOfxImageEffectPropOpenGLContextData, ofxhIE:1373-1386), and inArgs on Detached (ofxhIE:1409-1429): documented extension, harmless. On kOfxStatFailed, the render fails with "Rendering Failed" (EffectInstanceRenderRoI.cpp:1632-1634, 1715-1719) and the next render tries OpenGL again; no CPU fallback, GL not disabled for the instance.
     - PARTIAL

       Issue: `#1384 <https://github.com/NatronGitHub/Natron/issues/1384>`__
   * - G11
     - `ofxGPURender.h:363-387 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxGPURender.h#L363-L387>`__ ContextDetached "with the same OpenGL context active"
     - MUST
     - dettachOpenGLContext_public with the same glContext (EffectInstanceRenderRoI.cpp:1653-1656).
     - OK
   * - G12
     - `ofxGPURender.h:520-534 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxGPURender.h#L520-L534>`__ GL context current during Render/Begin/EndSequenceRender (if OpenGLEnabled) and Attach/Detach
     - MUST
     - Natron makes its OSGLContext current around GL renders.
     - OK
   * - G13
     - `ofxGPURender.h:546-617 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxGPURender.h#L546-L617>`__ CUDA: host descriptor kOfxImageEffectPropCudaRenderSupported / CudaStreamSupported RO, "false" when unsupported; CudaEnabled MAY; CudaStream "will only be set if the host and plug-in both support CUDA streams"
     - DEFAULT / MUST
     - Host "false" for both (ofxhIE:4716-4717; Natron never sets them). CudaEnabled = 0 in render args. kOfxImageEffectPropCudaStream is declared (NULL) in every Render/Begin/End inArgs (ofxhIE:1480 and the Begin/End lists) although streams are unsupported.
     - PARTIAL (spec-only)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - G14
     - `ofxGPURender.h:626-676 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxGPURender.h#L626-L676>`__ Metal: host kOfxImageEffectPropMetalRenderSupported "false"; MetalEnabled MAY
     - DEFAULT
     - Host "false" (ofxhIE:4718); MetalEnabled 0, MetalCommandQueue NULL declared in inArgs (ofxhIE:1478,1481). kOfxImageEffectPropMetalTextureSupported (ofxhIE:166,4746) is NEXT.
     - OK
   * - G15
     - `ofxGPURender.h:684-766 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxGPURender.h#L684-L766>`__ OpenCL: host kOfxImageEffectPropOpenCLRenderSupported (Buffers) / kOfxImageEffectPropOpenCLSupported (Images) "false"; OpenCLEnabled; OpenCLImage
     - DEFAULT
     - Host "false" for both (ofxhIE:4715,4747); OpenCLEnabled = 0 (note: the header's valid-values text at :725-729 is inverted, "0 ... SHOULD use"; the overview :824 says 0 = must not use, which Natron follows).
     - OK
   * - G16
     - `ofxGPURender.h:769-784 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxGPURender.h#L769-L784>`__ OfxOpenCLProgramSuiteV1 "optional suite the host can provide"
     - MAY
     - fetchSuite returns NULL unless ``supportsOpenCLProgramSuite()`` (ofxhIE:4876-4881); Natron doesn't override.
     - OK (N/A)

5. ofxColour.h (Natron advertises no colour management)
-------------------------------------------------------

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
     - `ofxColour.h:23-57 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxColour.h#L23-L57>`__ kOfxImageEffectPropColourManagementStyle: host descriptor RO, plug-in RW, instance RO, default None; "The chosen style must be set by the host ... on an image effect instance"
     - DEFAULT / MUST
     - Host None (ofxhIE:4748), plug-in default None (ofxhIE:169); instance gets its own RO property = None (ofxhIE:743-749), not the plug-in's capability. Natron never calls ``setColourManagementStyle``/``negotiateColourManagementStyle``.
     - OK
   * - C2
     - `ofxColour.h:71-83 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxColour.h#L71-L83>`__ kOfxImageEffectPropColourManagementAvailableConfigs "must be set by both plug-ins and hosts"
     - MUST
     - Host: dim 0 (ofxhIE:4749). With style None there is no native config to list.
     - OK (N/A with None)
   * - C3
     - `ofxColour.h:85-108 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxColour.h#L85-L108>`__ ColourManagementConfig / OCIOConfig "host must set" when negotiated
     - MUST if negotiated
     - Declared empty RO on the instance (ofxhIE:3250-3261); never negotiated.
     - N/A
   * - C4
     - `ofxColour.h:110-199 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxColour.h#L110-L199>`__ clip Colourspace / PreferredColourspaces, cross-references
     - MUST if negotiated
     - Declared (ofxhClip.cpp:287-295; Colourspace RO, though the spec says clip instance read/write) and HS has cross-reference helpers; unused with None.
     - N/A
   * - C5
     - `ofxColour.h:201-239 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxColour.h#L201-L239>`__ Display/OCIODisplay/OCIOView
     - SHOULD if negotiated
     - Declared empty.
     - N/A
   * - C6
     - `ofxColour.h:241-281 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxColour.h#L241-L281>`__ kOfxImageEffectActionGetOutputColourspace: host "must call this action whenever the effect instance is changed" if it relies on the output colourspace
     - MUST if used
     - HS ``getOutputColourspaceAction`` exists; Natron never calls it (no colour management).
     - N/A (`#1104 <https://github.com/NatronGitHub/Natron/issues/1104>`__)

6. Packaging, binary loading, plug-in cache, versions
-----------------------------------------------------

.. list-table::
   :header-rows: 1
   :widths: 6 26 10 44 14
   :class: longtable

   * - #
     - spec
     - level
     - Natron + HostSupport
     - verdict
   * - K1
     - `ofxPackaging.rst:23-58 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxPackaging.rst?plain=1#L23-L58>`__ NAME.ofx.bundle/Contents/ARCH/NAME.ofx; "All the binaries must end with .ofx"
     - SHOULD (recommended scheme)
     - ofxhPC:595-640: bundle entries ``*.ofx.bundle``, binary Contents/<arch>/<NAME>.ofx. The bundle test is a substring match (``name.find(".ofx.bundle")``, ofxhPC:596), so e.g. ``X.ofx.bundle.old`` is treated as a bundle (recorded as an invalid binary).
     - OK
   * - K2
     - `ofxPackaging.rst:78-92 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxPackaging.rst?plain=1#L78-L92>`__, 97-100 architectures: MacOS (universal), MacOS-x86-64 deprecated, Win32, Win64, Linux-x86, Linux-x86-64; "There will not be a specific arm64 folder"
     - MUST (per OS)
     - ``archDirsFor`` (HostSupport/include/ofxhArchDirs.h): MacOS, then MacOS-x86-64 only on x86-64 hosts; Win32/Win64 (+ lowercase); Linux-x86/Linux-x86-64. First folder whose binary matches the host word size/CPU (ELF/Mach-O/PE header check, ofxhPC:197-260, 600-626).
     - OK
   * - K3
     - `ofxPackaging.rst:110-126 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxPackaging.rst?plain=1#L110-L126>`__ (informative) Win-arm64, Linux-${uname -m} (Linux-aarch64)
     - MAY
     - Linux: Linux-$(uname -m), compiled-arch name, legacy Linux-x86-64 fallback; Windows Arm64: Win-arm64 (+ Win-arm64x). Win-arm64ec is NEXT.
     - OK
   * - K4
     - `ofxPackaging.rst:150-159 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxPackaging.rst?plain=1#L150-L159>`__ "All such directories are examined for plug-in bundles and sub directories are also recursively examined"; OFX_PLUGIN_PATH "examined, first to last, ... then the default location"
     - SHOULD
     - ``PluginCache()`` (ofxhPC:504-541): OFX_PLUGIN_PATH entries in order, then the default location; scanPluginFiles walks ``_pluginPath`` in order, recursively (ofxhPC:700-709). Natron then adds /…/OFX/Natron and /…/OFX/Nuke, the preference paths, and the bundled Plugins/OFX/Natron (appended, or prepended if "prefer bundled plug-ins", Engine/OfxHost.cpp:910-938); "use standard OFX location" can be turned off (OfxHost.cpp:896-901). Host choices the spec leaves open ("entirely up to it").
     - OK
   * - K5
     - `ofxPackaging.rst:161-187 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxPackaging.rst?plain=1#L161-L187>`__ Windows: ';' list, ``getStdOFXPluginPath`` (CSIDL_PROGRAM_FILES_COMMON\\OFX\\Plugins), and the deprecated ``C:\Program Files\Common Files\OFX\Plugins`` "should still be examined"
     - SHOULD
     - ofxhPC:76 (';'), 430-447, 528-535: both.
     - OK
   * - K6
     - `ofxPackaging.rst:189-200 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxPackaging.rst?plain=1#L189-L200>`__ macOS ';' list + /Library/OFX/Plugins; Unix ':' list + /usr/OFX/Plugins
     - SHOULD
     - ofxhPC:62,70 (Unix and macOS accept both ':' and ';'), 536-541.
     - OK
   * - K7
     - `ofxPackaging.rst:202-203 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxPackaging.rst?plain=1#L202-L203>`__ "Any bundle or sub-directory name starting with the character '@' is to be ignored. Such directories or bundles must be skipped by the host."
     - MUST
     - Only plain directories are checked (``name[0] != '@'``, ofxhPC:663). The bundle branch (ofxhPC:596-659) runs first and has no '@' test, so ``@Foo.ofx.bundle`` is loaded. No other filter in HostSupport or Natron.
     - WRONG

       Issue: `#1380 <https://github.com/NatronGitHub/Natron/issues/1380>`__
   * - K8
     - `ofxPackaging.rst:205-237 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxPackaging.rst?plain=1#L205-L237>`__ icons: Resources/<pluginIdentifier>.png (and optional .svg); host resizes keeping the aspect and shows the name next to the icon
     - MAY / SHOULD
     - Natron uses Resources/<id>.png (or kOfxPropIcon dim 1 if a plug-in managed to set it), OfxHost.cpp:1014-1028; NodeGui scales with Qt::KeepAspectRatio (Gui/NodeGui.cpp:629-637); label shown next to it. SVG not used (MAY).
     - OK
   * - K9
     - `ofxPackaging.rst:239-254 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxPackaging.rst?plain=1#L239-L254>`__ Resources/NAME.xml external resource file
     - MAY
     - Not read.
     - N/A
   * - K10
     - `ofxPluginStruct.rst:62-70 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxPluginStruct.rst?plain=1#L62-L70>`__, `ofxLoadingSequence.rst:4 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxLoadingSequence.rst?plain=1#L4>`__.2-4.3 "If a host does not support the given plug-in type, or ... version it should simply ignore that plug-in"
     - SHOULD
     - ofxhPC:363-376: invalid OfxPlugin structs and unsupported pluginApi/apiVersion skipped.
     - OK
   * - K11
     - `ofxLoadingSequence.rst:2 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxLoadingSequence.rst?plain=1#L2>`__, `ofxCore.h:571-577 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L571-L577>`__ OfxSetHost "if implemented by plugin and host ... host must check if the plugin implemented it"; plug-in may return kOfxStatFailed to be skipped
     - MAY
     - HS never looks up or calls OfxSetHost (only OfxGetNumberOfPlugins/OfxGetPlugin, ofxhPC:345-346, 413); none in Natron. Plug-ins that only list their effects after OfxSetHost show their default set.
     - MISSING (MAY; `#1107 <https://github.com/NatronGitHub/Natron/issues/1107>`__)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - K12
     - `ofxCoreAPI.rst:57 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxCoreAPI.rst?plain=1#L57>`__ "The returned pointer to OfxGetPlugin and pointers in the struct do not need to be freed"
     - MUST
     - Not freed.
     - OK
   * - K13
     - `ofxPluginStruct.rst:95-100 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxPluginStruct.rst?plain=1#L95-L100>`__ multiple versions: new instance -> greatest major and minor; loading a setup -> same major, greatest minor
     - SHOULD
     - Natron registers one plug-in per (id, major), the greatest minor (``getPluginsByIDMajor``, OfxHost.cpp:983; ``PluginCache::confirmPlugin``, ofxhAPI:589-643). ``AppManager::getPluginBinary`` (Engine/AppManager.cpp:2315-2385): major -1 (new node) -> highest (major, minor); a saved major -> that major. If the saved major is missing, Natron loads the next higher major, else the highest (beyond the spec).
     - OK
   * - K14
     - `ofxPluginStruct.rst:72-78 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxPluginStruct.rst?plain=1#L72-L78>`__ pluginIdentifier "should be unique among all plug-ins"; duplicates with the same version
     - (no host rule)
     - Same id+major+minor in two folders: HS keeps the one in the earlier plug-in path entry (ofxhAPI:611-637). The rank is the *last* path entry that is a string prefix of the bundle path (no separator check), so nested or prefix-sharing path entries can pick the wrong copy. Plug-ins from the bundled path and a system path with the same version: the earlier path in the list wins (see K4 for the order).
     - PARTIAL (low)

       Issue: `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__
   * - K15
     - `ofxCore.h:683-695 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L683-L695>`__ kOfxPropVersion / VersionLabel on the plug-in descriptor; `ofxImageEffect.h:785-793 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxImageEffect.h#L785-L793>`__ kOfxImageEffectPluginPropGrouping ("purely a user interface hint ... group related effects on any menus")
     - DEFAULT
     - Declared (ofxhIE:90-94); Natron uses the OfxPlugin struct versions and splits the grouping on '/' (``ofxExtractAllPartsOfGrouping``, OfxEffectInstance.cpp:1000-1053), with group icons from Resources/<group>.png.
     - OK
   * - K16
     - `ofxCore.h:706-751 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/include/ofxCore.h#L706-L751>`__ kOfxPropLabel default kOfxPropName; Short/LongLabel "initially kOfxPropName, but will be reset if kOfxPropLabel is changed"
     - DEFAULT
     - Descriptor defaults are "" (ofxhIE:87-89); getters fall back. Natron's menu label = label, else short, else long (``makePluginLabel``, OfxEffectInstance.cpp:1057-1071), and ``Base::getLabel`` falls back to the name. Latent HS bug: ``ImageEffect::Base::getShortLabel/getLongLabel`` (ofxhIE:204-237) return "" when the short/long label is unset but kOfxPropLabel is set (the inner ``s2`` is never returned); its only Natron caller ``OfxEffectInstance::getShortLabel`` (OfxEffectInstance.cpp:2202) has no callers, so no visible effect.
     - OK
   * - K17
     - Plug-in cache (host-specific, not in the spec)
     - —
     - Cache file per user, versioned ``NatronOFXCachev2``, entries checked against mtime/size; host properties kept static so the cached descriptors stay valid (OfxHost.cpp:906, 313).
     - OK

Problems (MISSING / WRONG / PARTIAL), by severity
-------------------------------------------------

Crash or data loss
~~~~~~~~~~~~~~~~~~

1. **I13 WRONG: parameter interacts are destroyed after their effect.** `ofxInteracts.rst:186-187 <https://github.com/AcademySoftwareFoundation/openfx/blob/OFX_Release_1.5.1/Documentation/sources/Reference/ofxInteracts.rst?plain=1#L186-L187>`__ "created after the effect and destroyed before the effect". ``OfxParamOverlayInteract`` is a ``shared_ptr`` held by the knob (Knob.cpp:3463), ``CustomParamInteract`` and ``CurveWidget``; ``~OfxEffectInstance`` (OfxEffectInstance.cpp:632-638) sends the effect's kOfxActionDestroyInstance and deletes the HostSupport instance first, the knobs die later in ``~KnobHolder`` (Knob.cpp:5019) or with the GUI. The interact's kOfxActionDestroyInstance (ofxhI:207-214) then runs with kOfxPropEffectInstance pointing to freed memory: a crash for any plug-in whose param-interact destructor touches its effect. Second check: only one ``setCustomInteract`` call (OfxEffectInstance.cpp:777), no reset of ``customInteract`` anywhere, ``KnobHolder::~KnobHolder`` only clears the holder pointer; the overlay interact is ordered correctly (reset before destroyInstanceAction). **Issue:** `#1362 <https://github.com/NatronGitHub/Natron/issues/1362>`__.

Wrong results / wrong behaviour
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

2. **G6 MISSING: no CPU retry on kOfxStatGLRenderFailed / GLOutOfMemory** (`#1359 <https://github.com/NatronGitHub/Natron/issues/1359>`__, re-verified open). ``OfxEffectInstance::render`` collapses every failure to eStatusFailed (OfxEffectInstance.cpp:2126-2147); the CPU retry (EffectInstanceRenderRoI.cpp:1720-1729) only follows an attach that returned kOfxStatErrMemory. Second check: no 1001/1002/kOfxStatGPU\* in Engine; HS renderAction returns the plug-in status unchanged. **Issue:** `#1359 <https://github.com/NatronGitHub/Natron/issues/1359>`__.

3. **G10 PARTIAL: kOfxActionOpenGLContextAttached returning kOfxStatFailed fails the render** instead of "the host should not attempt to run the plug-in in OpenGL render mode": EffectInstanceRenderRoI.cpp:1632-1634 -> throw "Rendering Failed" (:1715-1719), and nothing disables GL for that instance, so every render repeats it. Second check: ``attachOpenGLContext`` maps Failed -> eStatusFailed (OfxEffectInstance.cpp:3256-3263); no other caller. Related to `#1359 <https://github.com/NatronGitHub/Natron/issues/1359>`__. **Issue:** `#1384 <https://github.com/NatronGitHub/Natron/issues/1384>`__.

4. **I28 + I29 WRONG: custom param interact sizing ignores the plug-in.** Natron reads kOfxParamPropInteractSizeAspect / MinimumSize / PreferedSize from the interact descriptor, which only has Natron's defaults (OfxImageEffectInstance.cpp:523-535; OfxOverlayInteract.cpp:357-393; CustomParamInteract.cpp:76,93-94), and writes the current size as int on that descriptor instead of the param instance's double kOfxParamPropInteractSize (which stays 0,0). Second check: these property names are only used in those two Engine files and CustomParamInteract.cpp; nothing copies the param descriptor values (HS ofxhParam.cpp:462-465); the interact descriptor's property set is not chained to the param; the aspect is read with getIntProperty on a double (returns 0) and is unused. Since 32fb56d1b. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

5. **I6 WRONG: kOfxInteractPropPenViewportPosition in viewer overlays has a top-left origin** (Qt ``localPos()``, ViewerGL.cpp:1745/1868/2071/2503, passed unchanged by ViewerTab20.cpp:229, EffectInstance.cpp:3324-3354, OfxEffectInstance.cpp:2259-2262), in logical pixels; the spec requires GL viewport coordinates with 0,0 at the bottom left. Second check: no y flip anywhere on that path (param interacts do flip, CustomParamInteract.cpp:327). **Issue:** `#1378 <https://github.com/NatronGitHub/Natron/issues/1378>`__.

6. **I21 WRONG: kOfxPropKeyString is always "".** All key actions pass an empty ``QByteArray`` (OfxEffectInstance.cpp:2414,2443,2474; CustomParamInteract.cpp:407,426); the Engine key API carries only the Key enum. Plug-ins that read the typed character (non-English layouts, where the keysym is kOfxKey_Unknown) get nothing. Second check: HS ``setKeyArgProps`` just copies the argument (ofxhI:431-436); empty since e1a5140c5. **Issue:** `#1378 <https://github.com/NatronGitHub/Natron/issues/1378>`__, `#1379 <https://github.com/NatronGitHub/Natron/issues/1379>`__.

7. **I24 MISSING: custom param interacts never get focus**, hence never GainFocus/LoseFocus and never any KeyDown/KeyUp/KeyRepeat. Second check: no ``setFocusPolicy``/``setFocus`` in Gui/CustomParamInteract.cpp or Gui/KnobGui\*.cpp; QOpenGLWidget defaults to Qt::NoFocus; ``git log -S setFocusPolicy`` on the file: nothing. Related: `#1283 <https://github.com/NatronGitHub/Natron/issues/1283>`__. **Issue:** `#1379 <https://github.com/NatronGitHub/Natron/issues/1379>`__.

8. **I7 WRONG: pen pressure is 1.0 for button-up states.** Viewer: hover motion 1.0 (ViewerGL.cpp:2088), mouse release 1.0 (ViewerGL.cpp:1759); param interacts: 1.0 for down, motion and up (CustomParamInteract.cpp:330,349,368). Spec (SHOULD): a two-state pen maps to 0.0 and 1.0. Second check: only tablet events set real values (ViewerGL.cpp:2106-2131); introduced with 4cc28890a. **Issue:** `#1378 <https://github.com/NatronGitHub/Natron/issues/1378>`__, `#1379 <https://github.com/NatronGitHub/Natron/issues/1379>`__.

9. **K7 WRONG (MUST): bundles whose name starts with '@' are not skipped.** ofxhPC:596 handles ``*.ofx.bundle`` before the '@' test, which only guards recursion into plain directories (ofxhPC:663). Second check: no other name filter in HostSupport (``scanDirectory``, ``scanPluginFiles``) or Natron (``OfxHost::loadOFXPlugins``). **Issue:** `#1380 <https://github.com/NatronGitHub/Natron/issues/1380>`__.

10. **G9 WRONG: flushResources always returns kOfxStatOK without releasing anything** (OfxHost.cpp:1766-1770), telling the plug-in to retry its GPU allocation; should be kOfxStatReplyDefault, or actually purge the GL texture cache. Second check: the only override of ``flushOpenGLResources``. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__. **Fixed** by `#1483 <https://github.com/NatronGitHub/Natron/pull/1483>`__.

11. **I16 MISSING: an interact whose Draw returns kOfxStatFailed is not ignored afterwards.** HS drawAction keeps ``_state`` (ofxhI:450-489); Natron ignores the status (OfxEffectInstance.cpp:2230, CustomParamInteract.cpp:133). Second check: ``_state = eFailed`` is only set by describe/createInstance. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

Spec-only
~~~~~~~~~

12. **I19/I23 PARTIAL: GainFocus is not guaranteed before pen actions** in viewers (hover PenMotion before the first click; overlays created while the viewer already has focus never get GainFocus). ViewerGL.cpp:128-129, 1770, 3112-3139. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

13. **I20 PARTIAL (deliberate): PenUp and KeyUp are sent to every overlay** even after one returned kOfxStatOK (ViewerTab20.cpp:688-690, 920-923). **Deliberate, won't fix.** Listed in `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__ for completeness.

14. **I18 PARTIAL: param interact swapBuffers calls** ``update()``, which from inside paintGL schedules a new paint: a conforming plug-in that swaps in Draw would repaint continuously. Not observed (no known plug-in calls interactSwapBuffers); ``swapOpenGLBuffers`` should be a no-op during paintGL. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

15. **I1 PARTIAL: kOfxInteractPropSlaveToParam** is read once after createInstance for overlays and never for param interacts. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

16. **G1 PARTIAL: OpenGL render advertised "true" even when GL is disabled or unavailable** (static for the cache, OfxHost.cpp:312-320); "needed" plug-ins then fail at render rather than in describe. Related closed issue `#779 <https://github.com/NatronGitHub/Natron/issues/779>`__. **Issue:** `#1384 <https://github.com/NatronGitHub/Natron/issues/1384>`__.

17. **G2 PARTIAL: kOfxOpenGLPropPixelDepth is never consulted** (``isOpenGLPixelDepthSupported`` unused; output textures always float, OfxClipInstance.cpp:1046). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

18. **G7 PARTIAL: clipLoadTexture never returns kOfxStatErrMemory** (all failures -> kOfxStatFailed, ofxhIE:4205-4209). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

19. **G5 PARTIAL: a plug-in with kOfxImageEffectPropCPURenderSupported "false" and no OpenGL** (CUDA/Metal/OpenCL only) is still rendered on the CPU (OfxHost.cpp:359-372). `#1106 <https://github.com/NatronGitHub/Natron/issues/1106>`__. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

20. **G13 PARTIAL: kOfxImageEffectPropCudaStream is present (NULL) in Render/Begin/EndSequenceRender inArgs** although "This property will only be set if the host and plug-in both support CUDA streams" (ofxhIE:1480 and the Begin/End lists). `#1145 <https://github.com/NatronGitHub/Natron/issues/1145>`__. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

21. **I2 PARTIAL (low, ambiguous): kOfxInteractPropPixelScale is per logical pixel**, not per "real screen pixel" on HiDPI screens (ViewerGL.cpp:3630-3637); Natron offers kOfxInteractPropScreenPixelRatio as an extension. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

22. **K14 PARTIAL (low): duplicate-version path ranking uses the last matching string prefix** of the plug-in path list (ofxhAPI:619-633). **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

23. **K11 MISSING (MAY): OfxSetHost is never called** by HostSupport/Natron. `#1107 <https://github.com/NatronGitHub/Natron/issues/1107>`__. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

24. `#1349 <https://github.com/NatronGitHub/Natron/pull/1349>`__ **PARTIAL: DrawSuite drawText ignores the colour's alpha** (``renderText(..., 0)`` in Engine/OfxDrawSuiteGL.cpp, PR only). Tracked in `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__ until `#1349 <https://github.com/NatronGitHub/Natron/pull/1349>`__ is merged. **Issue:** `#1385 <https://github.com/NatronGitHub/Natron/issues/1385>`__.

NEXT items met: kOfxImageEffectPropMetalTextureSupported, Win-arm64ec folder, kOfxInteractPropKeyModifiers (OFX 1.6, under ``#ifdef``), the interact cursor extension (NatronGitHub/openfx#4, Natron extension).

Verdict counts
--------------

Over the 72 table rows (sections 1, 2, 4, 5, 6; the `#1349 <https://github.com/NatronGitHub/Natron/pull/1349>`__ section is counted separately):

.. list-table::
   :header-rows: 1
   :widths: 50 50
   :class: longtable

   * - verdict
     - count
   * - OK
     - 40
   * - PARTIAL
     - 13
   * - WRONG
     - 7
   * - MISSING
     - 4
   * - N/A
     - 8

`#1349 <https://github.com/NatronGitHub/Natron/pull/1349>`__: 1 PARTIAL (drawText alpha); it resolves I17/D1 (DrawSuite and OverlayInteractV2 supported), and fixes none of the other problems.
