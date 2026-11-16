--- src/3rdparty/chromium/third_party/blink/renderer/platform/wtf/hash_table.h.orig	2024-08-13 14:59:47 UTC
+++ src/3rdparty/chromium/third_party/blink/renderer/platform/wtf/hash_table.h
@@ -1786,7 +1786,7 @@ HashTable<Key, Value, Extractor, HashFunctions, Traits
     }
   }
   table_ = temporary_table;
-  Allocator::template BackingWriteBarrier(&table_);
+  Allocator::template BackingWriteBarrier<>(&table_);
 
   if (Traits::kEmptyValueIsZero) {
     memset(original_table, 0, new_table_size * sizeof(ValueType));
@@ -1844,7 +1844,7 @@ HashTable<Key, Value, Extractor, HashFunctions, Traits
   // This swaps the newly allocated buffer with the current one. The store to
   // the current table has to be atomic to prevent races with concurrent marker.
   AsAtomicPtr(&table_)->store(new_hash_table.table_, std::memory_order_relaxed);
-  Allocator::template BackingWriteBarrier(&table_);
+  Allocator::template BackingWriteBarrier<>(&table_);
   table_size_ = new_table_size;
 
   new_hash_table.table_ = old_table;
@@ -2012,8 +2012,8 @@ void HashTable<Key,
   // on the mutator thread, which is also the only one that writes to them, so
   // there is *no* risk of data races when reading.
   AtomicWriteSwap(table_, other.table_);
-  Allocator::template BackingWriteBarrier(&table_);
-  Allocator::template BackingWriteBarrier(&other.table_);
+  Allocator::template BackingWriteBarrier<>(&table_);
+  Allocator::template BackingWriteBarrier<>(&other.table_);
   if (IsWeak<ValueType>::value) {
     // Weak processing is omitted when no backing store is present. In case such
     // an empty table is later on used it needs to be strongified.
