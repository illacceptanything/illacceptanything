// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef V8_OBJECTS_JS_SEGMENT_ITERATOR_H_
#define V8_OBJECTS_JS_SEGMENT_ITERATOR_H_

#ifndef V8_INTL_SUPPORT
#error Internationalization is expected to be enabled.
#endif  // V8_INTL_SUPPORT

#include "src/base/bit-field.h"
#include "src/execution/isolate.h"
#include "src/heap/factory.h"
#include "src/objects/js-segmenter.h"
#include "src/objects/managed.h"
#include "src/objects/objects.h"
#include "unicode/uversion.h"

// Has to be the last include (doesn't have include guards):
#include "src/objects/object-macros.h"

namespace U_ICU_NAMESPACE {
class BreakIterator;
class UnicodeString;
}  // namespace U_ICU_NAMESPACE

namespace v8 {
namespace internal {

#include "torque-generated/src/objects/js-segment-iterator-tq.inc"

class JSSegmentIterator
    : public TorqueGeneratedJSSegmentIterator<JSSegmentIterator, JSObject> {
 public:
  // ecma402 #sec-CreateSegmentIterator
  V8_WARN_UNUSED_RESULT static MaybeHandle<JSSegmentIterator> Create(
      Isolate* isolate, icu::BreakIterator* icu_break_iterator,
      JSSegmenter::Granularity granularity);

  // ecma402 #sec-segment-iterator-prototype-next
  V8_WARN_UNUSED_RESULT static MaybeHandle<JSReceiver> Next(
      Isolate* isolate, Handle<JSSegmentIterator> segment_iterator_holder);

  Handle<String> GranularityAsString(Isolate* isolate) const;

  // SegmentIterator accessors.
  DECL_ACCESSORS(icu_break_iterator, Managed<icu::BreakIterator>)
  DECL_ACCESSORS(unicode_string, Managed<icu::UnicodeString>)

  DECL_PRINTER(JSSegmentIterator)

  inline void set_granularity(JSSegmenter::Granularity granularity);
  inline JSSegmenter::Granularity granularity() const;

  // Bit positions in |flags|.
  DEFINE_TORQUE_GENERATED_JS_SEGMENT_ITERATOR_FLAGS()

  STATIC_ASSERT(JSSegmenter::Granularity::GRAPHEME <= GranularityBits::kMax);
  STATIC_ASSERT(JSSegmenter::Granularity::WORD <= GranularityBits::kMax);
  STATIC_ASSERT(JSSegmenter::Granularity::SENTENCE <= GranularityBits::kMax);

  TQ_OBJECT_CONSTRUCTORS(JSSegmentIterator)
};

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"

#endif  // V8_OBJECTS_JS_SEGMENT_ITERATOR_H_
