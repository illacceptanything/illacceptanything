// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_SNAPSHOT_SNAPSHOT_H_
#define V8_SNAPSHOT_SNAPSHOT_H_

#include "include/v8.h"  // For StartupData.
#include "src/common/assert-scope.h"
#include "src/common/globals.h"

namespace v8 {
namespace internal {

class Context;
class Isolate;
class SnapshotData;
class JSGlobalProxy;

class Snapshot : public AllStatic {
 public:
  // ---------------- Serialization -------------------------------------------

  enum SerializerFlag {
    // If set, serializes unknown external references as verbatim data. This
    // usually leads to invalid state if the snapshot is deserialized in a
    // different isolate or a different process.
    // If unset, all external references must be known to the encoder.
    kAllowUnknownExternalReferencesForTesting = 1 << 0,
    // If set, the serializer enters a more permissive mode which allows
    // serialization of a currently active, running isolate. This has multiple
    // effects; for example, open handles are allowed, microtasks may exist,
    // etc. Note that in this mode, the serializer is allowed to skip
    // visitation of certain problematic areas even if they are non-empty. The
    // resulting snapshot is not guaranteed to result in a runnable context
    // after deserialization.
    // If unset, we assert that these previously mentioned areas are empty.
    kAllowActiveIsolateForTesting = 1 << 1,
    // If set, the ReadOnlySerializer reconstructs the read-only object cache
    // from the existing ReadOnlyHeap's read-only object cache so the same
    // mapping is used.  This mode is used for testing deserialization of a
    // snapshot from a live isolate that's using a shared
    // ReadOnlyHeap. Otherwise during deserialization the indices will mismatch,
    // causing deserialization crashes when e.g. types mismatch.
    // If unset, the read-only object cache is populated as read-only objects
    // are serialized.
    kReconstructReadOnlyObjectCacheForTesting = 1 << 2,
  };
  using SerializerFlags = base::Flags<SerializerFlag>;
  V8_EXPORT_PRIVATE static constexpr SerializerFlags kDefaultSerializerFlags =
      {};

  // In preparation for serialization, clear data from the given isolate's heap
  // that 1. can be reconstructed and 2. is not suitable for serialization. The
  // `clear_recompilable_data` flag controls whether compiled objects are
  // cleared from shared function infos and regexp objects.
  V8_EXPORT_PRIVATE static void ClearReconstructableDataForSerialization(
      Isolate* isolate, bool clear_recompilable_data);

  // Serializes the given isolate and contexts. Each context may have an
  // associated callback to serialize internal fields. The default context must
  // be passed at index 0.
  static v8::StartupData Create(
      Isolate* isolate, std::vector<Context>* contexts,
      const std::vector<SerializeInternalFieldsCallback>&
          embedder_fields_serializers,
      const DisallowGarbageCollection& no_gc,
      SerializerFlags flags = kDefaultSerializerFlags);

  // Convenience helper for the above when only serializing a single context.
  static v8::StartupData Create(
      Isolate* isolate, Context default_context,
      const DisallowGarbageCollection& no_gc,
      SerializerFlags flags = kDefaultSerializerFlags);

  // ---------------- Deserialization -----------------------------------------

  // Initialize the Isolate from the internal snapshot. Returns false if no
  // snapshot could be found.
  static bool Initialize(Isolate* isolate);

  // Create a new context using the internal context snapshot.
  static MaybeHandle<Context> NewContextFromSnapshot(
      Isolate* isolate, Handle<JSGlobalProxy> global_proxy,
      size_t context_index,
      v8::DeserializeEmbedderFieldsCallback embedder_fields_deserializer);

  // ---------------- Testing -------------------------------------------------

  // This function is used to stress the snapshot component. It serializes the
  // current isolate and context into a snapshot, deserializes the snapshot into
  // a new isolate and context, and finally runs VerifyHeap on the fresh
  // isolate.
  V8_EXPORT_PRIVATE static void SerializeDeserializeAndVerifyForTesting(
      Isolate* isolate, Handle<Context> default_context);

  // ---------------- Helper methods ------------------------------------------

  static bool HasContextSnapshot(Isolate* isolate, size_t index);
  static bool EmbedsScript(Isolate* isolate);
  V8_EXPORT_PRIVATE static bool VerifyChecksum(const v8::StartupData* data);
  static bool ExtractRehashability(const v8::StartupData* data);
  static bool VersionIsValid(const v8::StartupData* data);

  // To be implemented by the snapshot source.
  static const v8::StartupData* DefaultSnapshotBlob();

#ifdef DEBUG
  static bool SnapshotIsValid(const v8::StartupData* snapshot_blob);
#endif  // DEBUG
};

// Convenience wrapper around snapshot data blob creation used e.g. by tests and
// mksnapshot.
V8_EXPORT_PRIVATE v8::StartupData CreateSnapshotDataBlobInternal(
    v8::SnapshotCreator::FunctionCodeHandling function_code_handling,
    const char* embedded_source, v8::Isolate* isolate = nullptr);

// Convenience wrapper around snapshot data blob warmup used e.g. by tests and
// mksnapshot.
V8_EXPORT_PRIVATE v8::StartupData WarmUpSnapshotDataBlobInternal(
    v8::StartupData cold_snapshot_blob, const char* warmup_source);

#ifdef V8_USE_EXTERNAL_STARTUP_DATA
void SetSnapshotFromFile(StartupData* snapshot_blob);
#endif

}  // namespace internal
}  // namespace v8

#endif  // V8_SNAPSHOT_SNAPSHOT_H_
