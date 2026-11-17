.. _maint-knobs:

Knobs: Values, Animation, Links and Expressions
===============================================

Every parameter in Natron — of a node, of the project, of the preferences —
is a *knob*. This chapter explains how a knob stores and returns its value,
how a change propagates, and how links ("slaving") and Python expressions
work. The class list is in :ref:`maint-engine`; the code is mostly in
``Engine/Knob.h``, ``Engine/Knob.cpp`` and ``Engine/KnobImpl.h``.

The classes
-----------

- ``KnobI`` is the interface. ``KnobHelper`` implements everything that does
  not depend on the value type: dimensions, animation curves, expressions,
  links, listeners, and the ``KnobSignalSlotHandler`` that emits the knob's Qt
  signals.
- ``Knob<T>`` stores the values (``T`` is ``int``, ``bool``, ``double`` or
  ``std::string``), and the concrete classes (``KnobInt``, ``KnobDouble``,
  ``KnobColor``, ``KnobChoice``, …) derive from it.
- A knob has one or more **dimensions** (a ``KnobColor`` RGBA has 4) and, for
  multi-view projects, values per **view**. Most functions take a dimension
  index and a ``ViewSpec``.
- Knobs are created with ``AppManager::createKnob<KnobType>(holder, label,
  dimension)`` (see :ref:`maint-design`) and belong to a ``KnobHolder``. They
  are held by ``shared_ptr``; the holder is reached with ``getHolder()``.

Reading a value
---------------

``Knob<T>`` keeps two copies of the values: ``_values``, read by the render
threads, and ``_guiValues``, read by the main thread (``Knob.h``). Both are
set together, except while a value change is queued (see below).

``Knob<T>::getValue(dimension, view, clamp)`` (``KnobImpl.h``) returns, in
this order of precedence:

1. the result of the **expression** of that dimension, if it has one
   (``getValueFromExpression``, which caches the result for each time until
   the expression's inputs change);
2. the value of the **animation curve** at the current time, if the dimension
   is animated (``getValueAtTime``). For a linked dimension, ``getCurve``
   returns the *master's* curve unless it is called with
   ``byPassMaster=true``;
3. the **master's value**, if the dimension is linked to another knob
   (``getValueFromMaster``; only ``int``, ``bool`` and ``double`` masters are
   supported);
4. its own stored value (``_guiValues`` on the main thread, ``_values`` on other
   threads), clamped to the knob's range if ``clamp`` is true.

``getValueAtTime`` follows the same order for a given time. ``getValue``
uses the holder's current time: for an effect, that is the time of the render
in progress on this thread (read from the TLS), or else the timeline's current
frame (``EffectInstance::getCurrentTime``).

Changing a value
----------------

``Knob<T>::setValue`` (``KnobImpl.h``) stores the value (and sets a keyframe
if the dimension is animated and the change comes from the user or a plug-in),
then calls ``KnobHelper::evaluateValueChange``. That brackets the change in
``KnobHolder::beginChanges()`` / ``appendValueChange()`` / ``endChanges()``
(``Knob.cpp``). ``endChanges()`` does the work, once per bracket, for every
knob changed in it:

1. ``KnobHolder::onKnobValueChanged_public`` (main thread only). For an
   effect, ``EffectInstance::onKnobValueChanged_public`` calls the effect's
   ``knobChanged`` — for an OpenFX plug-in, the ``kOfxActionInstanceChanged``
   action (``OfxEffectInstance::knobChanged``) — and then
   ``Node::onEffectKnobValueChanged`` and the Python ``onParamChanged``
   callback.
2. The knob's ``valueChanged`` signal, which refreshes its widget.
3. ``refreshListenersAfterValueChange``, which updates the knobs that depend
   on this one (see the next sections).
4. If the change is *significant* (it affects the image), the holder calls
   ``onSignificantEvaluateAboutToBeCalled`` — for an effect, this aborts the
   renders in progress and increments the node's knobs age, which changes its
   hash (see :ref:`maint-cache`) — then ``evaluate``, which re-renders the
   Viewers that show the node.

Calling ``beginChanges()`` / ``endChanges()`` yourself around several
``setValue`` calls groups them, so the plug-in and the Viewer are notified
only once.

If the holder cannot take a new value right now — for an effect, during an
action where OpenFX forbids ``setValue`` (``KnobHolder::isSetValueCurrentlyPossible``)
— ``setValue`` queues the change (``_setValuesQueue``) and only updates
``_guiValues``. The queue is applied later by ``dequeueValuesSet``, from
``Node::dequeueActions()`` on the main thread.

Links (slave and master)
------------------------

A dimension of a knob can be **linked** to a dimension of another knob of a
compatible type: the linked dimension (the *slave*) then shows and returns
the value of the other one (the *master*). The user creates links in the
parameter panel; the code calls ``KnobI::slaveTo(dimension, master,
masterDimension)`` (``Knob.cpp``), and ``KnobHolder::slaveAllKnobs`` links all
the knobs of a node to another one (for example to clone a node).

``KnobHelper::slaveToInternal`` (``Knob.cpp``):

- stores the master, as a ``weak_ptr``, and the master dimension in
  ``masters[dimension]`` (refused if that dimension already has a master, or
  if it would create a two-knob cycle);
- connects the master's ``keyFrameSet``, ``keyFrameRemoved``,
  ``keyFrameMoved`` and ``animationRemoved`` signals to the slave's
  ``KnobSignalSlotHandler`` — once per (master, slave) pair of knobs, whatever
  the number of linked dimensions (``Qt::UniqueConnection``);
- copies the master dimension into the slave dimension (value, curve), and
  disables the slave dimension in the GUI (except for buttons);
- registers the slave as a **listener** of the master (``addListener`` with
  ``isExpression`` false). The master's listener table maps each listening
  knob to one entry per dimension, with ``isListening``, ``isExpr`` and the
  master dimension it listens to (``targetDim``).

While the link exists, ``getValue`` and ``getCurve`` of the slave dimension
return the master's (see *Reading a value*). The slave also keeps its own copy
up to date, so that its curve can be shown and edited:

- when the master's value changes, ``refreshListenersAfterValueChange`` copies
  the master dimension into each slave dimension that listens to it
  (``clone``), then calls the slave's ``evaluateValueChangeInternal`` with the
  reason ``eValueChangedReasonSlaveRefresh``, and recurses into the slave's own
  listeners;
- when a keyframe of the master changes, the ``KnobSignalSlotHandler::onMaster*``
  slots copy it into the slave dimensions that are linked to *that* master
  dimension, and re-emit the signal with the slave's dimension. The mapping
  from master dimension to slave dimensions (``getDimensionsSlavedTo`` in
  ``Knob.cpp``) matters when the slave has fewer dimensions than the master,
  or when slave dimension *i* is linked to master dimension *j*: before #1198
  the slots copied master dimension *i* into slave dimension *i*, which crashed
  (#1018) or overwrote unlinked dimensions.

Unlinking (``KnobI::unSlave(dimension, copyState)``, which calls
``Knob<T>::unSlaveInternal`` in ``KnobImpl.h``) works on **one dimension**, and
must leave the other linked dimensions of the same knob working (#1201):

- ``resetMaster`` clears ``masters[dimension]`` under the masters lock (render
  threads read it), and resets the knob-wide ``ignoreMasterPersistence`` flag
  only when no dimension is linked any more;
- the master's keyframe signals are disconnected only if no other dimension of
  the knob is still linked to the same master;
- with ``copyState``, the master's value and curve are copied one last time
  into the slave dimension, which becomes editable again;
- ``removeListener`` clears the listener entry of that dimension only, and
  erases the slave from the master's listener table only when none of its
  dimensions listens any more.

Links are saved in the project as ``MasterSerialization`` entries (node name,
knob name, dimension; ``Engine/KnobSerialization.h``) and restored by
``Node::restoreKnobsLinks`` once all the nodes of the project are loaded. A link made with ``ignoreMasterPersistence`` (the
Tracker's internal links) is not saved. ``Tests/KnobSlave_Test.cpp`` covers
these cases on built-in ``Dot`` nodes, without any plug-in; it checks the
slave's own curves and values (``getCurve(…, true)``, ``getRawValue()``).

Expressions
-----------

An expression is a Python script attached to one dimension
(``KnobHelper::setExpression``, which calls ``setExpressionInternal`` in
``Knob.cpp``). ``validateExpression`` wraps the script in a Python function
``expression<dimension>(frame, view)`` (adding ``ret =`` to a one-line
expression), runs it once, and rejects it if it fails or if ``ret`` cannot be
converted to the knob's type. The user-level syntax is described in
:ref:`paramExpr`.

To find out what an expression depends on, ``parseListenersFromExpression``
(``Knob.cpp``) rewrites the calls to ``get``, ``getValue``,
``getValueAtTime``, ``getDerivativeAtTime`` and ``getIntegrateFromTimeToTime``
in the script into calls to ``addAsDependencyOf``, and runs it. Each call lands
in ``Param::_addAsDependencyOf`` (``Engine/PyParameter.cpp``), which registers
the expression's knob as a listener of the knob it reads (``addListener`` with
``isExpression`` true). Dependencies are therefore found by running the
script: a knob that is only read in a branch that did not run is missed.

When a knob that an expression reads changes,
``refreshListenersAfterValueChange`` does not copy anything: it clears the
cached expression results of the listening knob (``clearExpressionsResults``)
and calls its ``evaluateValueChangeInternal``, so the expression is evaluated
again the next time the value is read (``executeExpression``, ``Knob.cpp``).

Expressions and links are both listeners of the knob they read.
``addListener`` also records the link in the listening node
(``KnobHolder::onKnobSlaved``), which is what the NodeGraph draws as an arrow
(``Node::getKnobsLinks``). When a node is deleted, ``Node::deactivate`` goes
through the listeners of its knobs: linked dimensions are unlinked (keeping a
copy of the value), and expressions are marked invalid.
