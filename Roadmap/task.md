# Natron Feature Implementation Tasks

## Feature 1: Multi-layer EXR / AOV Ergonomics

### Step 1.1 — MultiPlaneImage Wrapper (Engine)
- [ ] Create `Engine/MultiPlaneImage.h`
- [ ] Create `Engine/MultiPlaneImage.cpp`
- [ ] Add to build system (`Engine/CMakeLists.txt`)
- [ ] Unit test `Tests/TestMultiPlaneImage.cpp`

### Step 1.2 — ReadNode Multi-layer Enhancement
- [ ] Analyze current `ReadNode.cpp` plane handling
- [ ] Add `_readAllLayersKnob` to ReadNode
- [ ] Modify `clipGetOutputComponents()` for multi-plane output
- [ ] Signal when available layers change

### Step 1.3 — Viewer Layer Quick-Switch
- [ ] Add layer dropdown to ViewerTab
- [ ] Connect to `ViewerInstance::setActiveLayer()`
- [ ] Add Page Up/Down keyboard shortcuts for layer cycling
- [ ] Add 'L' hotkey for searchable layer popup

### Step 1.4 — Layer Inspector Panel (Gui)
- [ ] Create `Gui/LayerInspectorWidget.h`
- [ ] Create `Gui/LayerInspectorWidget.cpp`
- [ ] Register as dockable panel
- [ ] Tree view with layer → channels hierarchy
- [ ] Thumbnail preview per layer

### Step 1.5 — Auto-Shuffle Insertion
- [ ] Detect multi-plane → single-plane connection
- [ ] Prompt layer selection
- [ ] Auto-insert Shuffle node

---

## Feature 2: Cache System Hardening

### Step 2.1 — CacheManager Singleton
- [ ] Create `Engine/CacheManager.h`
- [ ] Create `Engine/CacheManager.cpp`
- [ ] Separate viewer and node cache instances
- [ ] Backward-compatible accessors in AppManager

### Step 2.2 — Lock Contention Reduction
- [ ] Replace triple-mutex with QReadWriteLock + atomics
- [ ] Verify thread safety with stress tests

### Step 2.3 — Priority-Based Eviction
- [ ] Add CachePriority enum
- [ ] Modify LRUHashTable for priority-aware eviction
- [ ] Viewer frames = HIGH, intermediate = LOW

### Step 2.4 — Tile Cache Lifecycle
- [ ] Startup orphan file detection
- [ ] Reference counting on TileCacheFile
- [ ] Configurable file cap

### Step 2.5 — Cache Status Widget (Gui)
- [ ] Create `Gui/CacheStatusWidget.h`
- [ ] Create `Gui/CacheStatusWidget.cpp`
- [ ] RAM/disk gauges in status bar

---

## Feature 3: Channel Management Modernization
- [ ] LayerPickerKnob
- [ ] Shuffle2 core node
- [ ] Channel name normalization
- [ ] Utility nodes (RemoveLayer, RenameLayer, LayerCopy)
