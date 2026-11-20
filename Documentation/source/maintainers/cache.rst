.. _maint-cache:

The Cache
=========

The cache is what makes real-time playback possible: once a frame (or an
intermediate result) is computed, it is kept so the next request is instant.
This chapter explains where images are cached, how they are found again, and
how the cache stays within its size limits. The render side (when
``renderRoI`` looks up and fills the cache) is in :ref:`maint-rendering`.

.. mermaid::
   :alt: renderRoI looks up the cache: first the RAM part, then the disk part,
         whose entries are mapped again and moved back to RAM. On a hit, the
         entry with the right mip-map level is picked, or one of a higher
         resolution is downscaled. On a miss, the output image is allocated:
         outside the cache if shouldCacheOutput says no, otherwise with
         getOrCreate, which checks the free memory and evicts least-recently-
         used entries while the cache is more than 90 percent full. When a node
         hash changes, the CacheCleanerThread removes the node entries that
         have another hash.
   :caption: Looking up and filling an image cache (Cache.h,
             EffectInstance.cpp).

   flowchart TD
       A["renderRoI: getImageFromCacheAndConvertIfNeeded()"] --> L["Cache::get(key)"]
       L --> D{"hash in _memoryCache?"}
       D -- yes --> H["entries with this hash:<br/>pick the mip-map level,<br/>downscale or convert if needed"]
       D -- no --> E{"hash in _diskCache?"}
       E -- yes --> F["map the file again,<br/>move the entry back to RAM"] --> H
       E -- no --> M["miss: allocate the output image"]
       M --> B{"shouldCacheOutput()?"}
       B -- no --> X["new Image, outside the cache"]
       B -- yes --> C["Cache::getOrCreate(key, params)<br/>→ createInternal()"]
       C --> G1["checkCacheFreeMemoryIsGoodEnough()"]
       G1 --> G2{"cache more than 90% full?"}
       G2 -- yes --> G3["evict the LRU entry: to the disk part,<br/>or delete it on the DeleterThread"] --> G2
       G2 -- no --> G4["new entry in _memoryCache"]
       N["node hash changed"] -.-> CL["CacheCleanerThread removes the node's<br/>entries that have another hash"]

The three caches
----------------

``AppManager`` owns three caches (``Engine/AppManagerPrivate.h``), created
in ``AppManager::loadInternalAfterInitGui()`` with sizes taken from the
preferences:

.. list-table::
   :header-rows: 1
   :widths: 18 22 60

   * - Member
     - Type and name
     - Contents
   * - ``_nodeCache``
     - ``Cache<Image>``, ``"NodeCache"``
     - The images rendered by the nodes. RAM only: its size is the
       *Maximum amount of RAM memory used for caching* preference (a
       percentage of the total RAM).
   * - ``_diskCache``
     - ``Cache<Image>``, ``"DiskCache"``
     - The images of the ``DiskCache`` node, on disk (memory-mapped files),
       up to *Maximum DiskCache node disk usage*.
   * - ``_viewerCache``
     - ``Cache<FrameEntry>``, ``"ViewerCache"``
     - The textures shown by the Viewer (8-bit or 32-bit float, already
       converted for display), on disk, up to *Maximum playback disk cache
       size*. This cache is *tiled*: its entries are tiles in a few large
       files (``TileCacheFile``, 2 GB each, ``NATRON_TILE_CACHE_FILE_SIZE_BYTES``
       in ``Cache.h``).

The preferences are documented in :doc:`../_prefs` ("Caching"), and the
setting objects are in ``Engine/Settings.cpp`` (``_maxRAMPercent``,
``_maxDiskCacheNodeGB``, ``_maxViewerDiskCacheGB``, ``_diskCachePath``).

The two disk caches survive a restart: ``AppManagerPrivate::saveCaches()``
writes their table of contents when Natron quits (the Viewer cache only in GUI
mode), and ``restoreCaches()`` reads it at startup. When
``NATRON_CACHE_VERSION`` (``Global/GlobalDefines.h``) differs from the version
stored in the settings, or when Natron is started with ``--clear-cache``, the
disk caches are wiped instead. Bump ``NATRON_CACHE_VERSION``
whenever you change the layout of a cache entry on disk.

The ``Cache`` template
----------------------

``Cache<EntryType>`` (``Engine/Cache.h``) is a thread-safe least-recently-used
cache with two parts: ``_memoryCache`` (entries in RAM) and ``_diskCache``
(entries backed by a memory-mapped file, ``MemoryFile``, whose RAM has been
released). Both are LRU hash tables (``LRUHashTable.h``) from a 64-bit hash
to a *list* of entries: several entries can share a hash and differ by their
parameters.

``get(key, &entries)``
    Looks the key up in RAM, then on disk. An entry found on disk is mapped
    into memory again and moved back to the RAM part (which may push other
    entries out of RAM).

``getOrCreate(key, params, locker, &entry)``
    Returns the entry whose parameters match, or creates one. Before creating
    it, the cache makes room (``createInternal``): it checks that enough
    physical RAM is free (``AppManager::checkCacheFreeMemoryIsGoodEnough``,
    which evicts node-cache images while the free RAM is below the *System RAM
    to keep free (% of total RAM)* preference), then evicts least-recently-used entries while
    the cache is more than 90% full (``NATRON_CACHE_LIMIT_PERCENT``). An entry
    evicted from RAM that has a backing file goes to the disk part; others are
    deleted. The deletion itself happens on a background ``DeleterThread``,
    because freeing large buffers is slow.

An entry that is still used somewhere (another ``shared_ptr`` holds it) cannot
be evicted, so the cache can temporarily exceed its limit while a render holds
many images.

Keys, hashes and parameters
---------------------------

An image entry has a key (``ImageKey``, ``Engine/ImageKey.cpp``), which
gives the hash, and parameters (``ImageParams``), which are compared only
among the entries with the same hash.

The key hash (``ImageKey::fillHash``) combines:

- the **node hash** (see below);
- the time, but only if the node's output varies with time or is animated
  (otherwise one image serves all frames);
- the view, the pixel aspect ratio, the draft-mode flag, and whether the image
  was rendered at full scale from downscaled inputs.

The parameters (``ImageParams``) hold the region of definition, the
components, the bit depth, the mip-map level, the premultiplication state and
the fielding. ``EffectInstance::getImageFromCacheAndConvertIfNeeded`` takes the
entries with the right hash and picks one with the right mip-map level, or
downscales one of a higher resolution, and converts components and bit depth
if needed. The region of interest is not part of the key: an image covers its
region of definition, and its **bitmap** records which pixels have been
rendered (see the worked example in :ref:`maint-rendering`).

The node hash (``Node::computeHashInternal`` in ``Engine/Node.cpp``) is
computed from the node's *knobs age*, the hashes of its inputs, its script
name, and the project creation time. The knobs age is a counter, not a hash of
the knob values: every significant change of a knob increments it
(``Node::incrementKnobsAge``), so the node hash changes and every downstream
hash changes with it. This is how invalidation stays correct without explicit
dirty-tracking. A consequence: undoing a change does not bring back the old
hash, so the old image is not reused.

The knobs age is saved in the project file, together with a per-node cache
ID, so that the disk caches still match after the project is reopened. After a
load, the first change replaces the age with a random 64-bit value
(``advanceKnobsAge``), so that two sessions that edit the same saved project
don't produce the same hashes for different images.

Removing stale entries
----------------------

When a node's hash changes, its old images can never be requested again.
``Node::computeHash`` then calls
``AppManager::removeAllImagesFromCacheWithMatchingIDAndDifferentKey``, which
queues a request on the cache's ``CacheCleanerThread`` (``Cache.h``): the
cleaner thread removes, in the background, the entries of that node (same
cache ID) whose hash differs from the new one.

Which node outputs are cached
-----------------------------

Not every node output goes to the cache: caching everything would fill the RAM
with images that are used once. ``renderRoI`` asks
``EffectInstance::shouldCacheOutput``, which calls ``Node::shouldCacheOutput``.
The comment at the top of that function lists the cases where the output is
cached: the node has several outputs, its single output has its settings panel
open, *Force caching* is checked on the node (or on its group), the
*Aggressive caching* preference is on, the node is in a recursive action
(analysis), the plug-in accesses other frames, the node shows a preview, it is
a direct input of a Viewer, its output does not vary with time, it is a Roto
node being edited, or it does not support tiles. Images that are not cached
are still allocated through the same ``Image`` class, but outside the cache.

The Viewer cache is separate: ``ViewerInstance`` looks up its ``FrameEntry``
textures before it asks its input for an image
(``getRenderViewerArgsAndCheckCache``), which is why playing back a frame
range that was already viewed doesn't render anything.

Concurrency
-----------

Several threads can ask for the same image at once. ``getOrCreate`` holds the
cache lock while it looks up and creates the entry, and an ``ImageLocker``
keeps a freshly created image locked until its memory is allocated. Inside the
image, the bitmap is a *trimap* (``NATRON_ENABLE_TRIMAP`` in
``Global/Macros.h``): a pixel can be "not rendered", "rendered" or "being
rendered", so that a thread waits for another one instead of rendering the
same pixels again (``markImageAsBeingRendered``,
``waitForImageBeingRenderedElsewhere``; see :ref:`maint-rendering`).

Known problems
--------------

- `#192 <https://github.com/NatronGitHub/Natron/issues/192>`_: Natron does not
  respect the disk cache size limits and crashes when the disk is full.
- `#248 <https://github.com/NatronGitHub/Natron/issues/248>`_: rendering can
  stall after a number of frames; the wait for pixels "being rendered
  elsewhere" is a suspect (see :ref:`maint-issue-triage`).
