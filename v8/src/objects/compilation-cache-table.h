// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_COMPILATION_CACHE_TABLE_H_
#define V8_OBJECTS_COMPILATION_CACHE_TABLE_H_

#include "src/objects/feedback-cell.h"
#include "src/objects/hash-table.h"
#include "src/objects/js-regexp.h"
#include "src/objects/shared-function-info.h"
#include "src/roots/roots.h"

// Has to be the last include (doesn't have include guards):
#include "src/objects/object-macros.h"

namespace v8 {
namespace internal {

class CompilationCacheShape : public BaseShape<HashTableKey*> {
 public:
  static inline bool IsMatch(HashTableKey* key, Object value) {
    return key->IsMatch(value);
  }

  static inline uint32_t Hash(ReadOnlyRoots roots, HashTableKey* key) {
    return key->Hash();
  }

  static inline uint32_t RegExpHash(String string, Smi flags);

  static inline uint32_t StringSharedHash(String source,
                                          SharedFunctionInfo shared,
                                          LanguageMode language_mode,
                                          int position);

  static inline uint32_t StringSharedHash(String source,
                                          LanguageMode language_mode);

  static inline uint32_t HashForObject(ReadOnlyRoots roots, Object object);

  static const int kPrefixSize = 0;
  // An 'entry' is essentially a grouped collection of slots. Entries are used
  // in various ways by the different caches; most store the actual key in the
  // first entry slot, but it may also be used differently.
  // Why 3 slots? Because of the eval cache.
  static const int kEntrySize = 3;
  static const bool kMatchNeedsHoleCheck = true;
};

class InfoCellPair {
 public:
  InfoCellPair() = default;
  inline InfoCellPair(Isolate* isolate, SharedFunctionInfo shared,
                      FeedbackCell feedback_cell);

  FeedbackCell feedback_cell() const {
    DCHECK(is_compiled_scope_.is_compiled());
    return feedback_cell_;
  }
  SharedFunctionInfo shared() const {
    DCHECK(is_compiled_scope_.is_compiled());
    return shared_;
  }

  bool has_feedback_cell() const {
    return !feedback_cell_.is_null() && is_compiled_scope_.is_compiled();
  }
  bool has_shared() const {
    // Only return true if SFI is compiled - the bytecode could have been
    // flushed while it's in the compilation cache, and not yet have been
    // removed form the compilation cache.
    return !shared_.is_null() && is_compiled_scope_.is_compiled();
  }

 private:
  IsCompiledScope is_compiled_scope_;
  SharedFunctionInfo shared_;
  FeedbackCell feedback_cell_;
};

EXTERN_DECLARE_HASH_TABLE(CompilationCacheTable, CompilationCacheShape)

class CompilationCacheTable
    : public HashTable<CompilationCacheTable, CompilationCacheShape> {
 public:
  NEVER_READ_ONLY_SPACE

  // The 'script' cache contains SharedFunctionInfos.
  static MaybeHandle<SharedFunctionInfo> LookupScript(
      Handle<CompilationCacheTable> table, Handle<String> src,
      LanguageMode language_mode, Isolate* isolate);
  static Handle<CompilationCacheTable> PutScript(
      Handle<CompilationCacheTable> cache, Handle<String> src,
      LanguageMode language_mode, Handle<SharedFunctionInfo> value,
      Isolate* isolate);

  // Eval code only gets cached after a second probe for the
  // code object. To do so, on first "put" only a hash identifying the
  // source is entered into the cache, mapping it to a lifetime count of
  // the hash. On each call to Age all such lifetimes get reduced, and
  // removed once they reach zero. If a second put is called while such
  // a hash is live in the cache, the hash gets replaced by an actual
  // cache entry. Age also removes stale live entries from the cache.
  // Such entries are identified by SharedFunctionInfos pointing to
  // either the recompilation stub, or to "old" code. This avoids memory
  // leaks due to premature caching of eval strings that are
  // never needed later.
  static InfoCellPair LookupEval(Handle<CompilationCacheTable> table,
                                 Handle<String> src,
                                 Handle<SharedFunctionInfo> shared,
                                 Handle<Context> native_context,
                                 LanguageMode language_mode, int position);
  static Handle<CompilationCacheTable> PutEval(
      Handle<CompilationCacheTable> cache, Handle<String> src,
      Handle<SharedFunctionInfo> outer_info, Handle<SharedFunctionInfo> value,
      Handle<Context> native_context, Handle<FeedbackCell> feedback_cell,
      int position);

  // The RegExp cache contains JSRegExp::data fixed arrays.
  Handle<Object> LookupRegExp(Handle<String> source, JSRegExp::Flags flags);
  static Handle<CompilationCacheTable> PutRegExp(
      Isolate* isolate, Handle<CompilationCacheTable> cache, Handle<String> src,
      JSRegExp::Flags flags, Handle<FixedArray> value);

  void Remove(Object value);
  void Age(Isolate* isolate);

  DECL_CAST(CompilationCacheTable)

 private:
  void RemoveEntry(int entry_index);

  OBJECT_CONSTRUCTORS(CompilationCacheTable,
                      HashTable<CompilationCacheTable, CompilationCacheShape>);
};

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"

#endif  // V8_OBJECTS_COMPILATION_CACHE_TABLE_H_
