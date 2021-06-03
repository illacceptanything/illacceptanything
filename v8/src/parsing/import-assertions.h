// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_PARSING_IMPORT_ASSERTIONS_H_
#define V8_PARSING_IMPORT_ASSERTIONS_H_

#include "src/parsing/scanner.h"  // Only for Scanner::Location.
#include "src/zone/zone-containers.h"

namespace v8 {
namespace internal {

struct V8_EXPORT_PRIVATE ImportAssertionsKeyComparer {
  bool operator()(const AstRawString* lhs, const AstRawString* rhs) const;
};

class ImportAssertions
    : public ZoneMap<const AstRawString*,
                     std::pair<const AstRawString*, Scanner::Location>,
                     ImportAssertionsKeyComparer> {
 public:
  explicit ImportAssertions(Zone* zone)
      : ZoneMap<const AstRawString*,
                std::pair<const AstRawString*, Scanner::Location>,
                ImportAssertionsKeyComparer>(zone) {}
};

}  // namespace internal
}  // namespace v8

#endif  // V8_PARSING_IMPORT_ASSERTIONS_H_
