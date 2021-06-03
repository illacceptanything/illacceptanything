// Copyright 2010 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_PROFILER_PROFILE_GENERATOR_INL_H_
#define V8_PROFILER_PROFILE_GENERATOR_INL_H_

#include "src/profiler/profile-generator.h"

#include <memory>

namespace v8 {
namespace internal {

CodeEntry::CodeEntry(CodeEventListener::LogEventsAndTags tag, const char* name,
                     const char* resource_name, int line_number,
                     int column_number,
                     std::unique_ptr<SourcePositionTable> line_info,
                     bool is_shared_cross_origin, CodeType code_type)
    : bit_field_(TagField::encode(tag) |
                 BuiltinIdField::encode(Builtins::builtin_count) |
                 CodeTypeField::encode(code_type) |
                 SharedCrossOriginField::encode(is_shared_cross_origin)),
      name_(name),
      resource_name_(resource_name),
      line_number_(line_number),
      column_number_(column_number),
      script_id_(v8::UnboundScript::kNoScriptId),
      position_(0),
      line_info_(std::move(line_info)) {}

ProfileNode::ProfileNode(ProfileTree* tree, CodeEntry* entry,
                         ProfileNode* parent, int line_number)
    : tree_(tree),
      entry_(entry),
      self_ticks_(0),
      line_number_(line_number),
      parent_(parent),
      id_(tree->next_node_id()) {
  tree_->EnqueueNode(this);
}

inline Isolate* ProfileNode::isolate() const { return tree_->isolate(); }

}  // namespace internal
}  // namespace v8

#endif  // V8_PROFILER_PROFILE_GENERATOR_INL_H_
