// Copyright 2011 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/codegen/safepoint-table.h"

#include "src/codegen/assembler-inl.h"
#include "src/codegen/macro-assembler.h"
#include "src/deoptimizer/deoptimizer.h"
#include "src/diagnostics/disasm.h"
#include "src/execution/frames-inl.h"
#include "src/utils/ostreams.h"

#if V8_ENABLE_WEBASSEMBLY
#include "src/wasm/wasm-code-manager.h"
#endif  // V8_ENABLE_WEBASSEMBLY

namespace v8 {
namespace internal {

SafepointTable::SafepointTable(Isolate* isolate, Address pc, Code code)
    : SafepointTable(code.InstructionStart(isolate, pc),
                     code.SafepointTableAddress(), true) {}

#if V8_ENABLE_WEBASSEMBLY
SafepointTable::SafepointTable(const wasm::WasmCode* code)
    : SafepointTable(code->instruction_start(),
                     code->instruction_start() + code->safepoint_table_offset(),
                     false) {}
#endif  // V8_ENABLE_WEBASSEMBLY

SafepointTable::SafepointTable(Address instruction_start,
                               Address safepoint_table_address, bool has_deopt)
    : instruction_start_(instruction_start),
      has_deopt_(has_deopt),
      safepoint_table_address_(safepoint_table_address),
      length_(ReadLength(safepoint_table_address)),
      entry_size_(ReadEntrySize(safepoint_table_address)) {}

unsigned SafepointTable::find_return_pc(unsigned pc_offset) {
  for (unsigned i = 0; i < length(); i++) {
    if (GetTrampolinePcOffset(i) == static_cast<int>(pc_offset)) {
      return GetPcOffset(i);
    } else if (GetPcOffset(i) == pc_offset) {
      return pc_offset;
    }
  }
  UNREACHABLE();
}

SafepointEntry SafepointTable::FindEntry(Address pc) const {
  unsigned pc_offset = static_cast<unsigned>(pc - instruction_start_);
  // We use kMaxUInt32 as sentinel value, so check that we don't hit that.
  DCHECK_NE(kMaxUInt32, pc_offset);
  unsigned len = length();
  CHECK_GT(len, 0);
  // If pc == kMaxUInt32, then this entry covers all call sites in the function.
  if (len == 1 && GetPcOffset(0) == kMaxUInt32) return GetEntry(0);
  for (unsigned i = 0; i < len; i++) {
    // TODO(kasperl): Replace the linear search with binary search.
    if (GetPcOffset(i) == pc_offset ||
        (has_deopt_ &&
         GetTrampolinePcOffset(i) == static_cast<int>(pc_offset))) {
      return GetEntry(i);
    }
  }
  UNREACHABLE();
}

void SafepointTable::PrintEntry(unsigned index, std::ostream& os) const {
  disasm::NameConverter converter;
  SafepointEntry entry = GetEntry(index);
  uint8_t* bits = entry.bits();

  // Print the stack slot bits.
  if (entry_size_ > 0) {
    for (uint32_t i = 0; i < entry_size_; ++i) {
      for (int bit = 0; bit < kBitsPerByte; ++bit) {
        os << ((bits[i] & (1 << bit)) ? "1" : "0");
      }
    }
  }
}

Safepoint SafepointTableBuilder::DefineSafepoint(Assembler* assembler) {
  deoptimization_info_.push_back(
      DeoptimizationInfo(zone_, assembler->pc_offset_for_safepoint()));
  DeoptimizationInfo& new_info = deoptimization_info_.back();
  return Safepoint(new_info.stack_indexes, &new_info.register_indexes);
}

unsigned SafepointTableBuilder::GetCodeOffset() const {
  DCHECK(emitted_);
  return offset_;
}

int SafepointTableBuilder::UpdateDeoptimizationInfo(int pc, int trampoline,
                                                    int start,
                                                    unsigned deopt_index) {
  int index = start;
  for (auto it = deoptimization_info_.Find(start);
       it != deoptimization_info_.end(); it++, index++) {
    if (static_cast<int>(it->pc) == pc) {
      it->trampoline = trampoline;
      it->deopt_index = deopt_index;
      return index;
    }
  }
  UNREACHABLE();
}

void SafepointTableBuilder::Emit(Assembler* assembler, int bits_per_entry) {
  RemoveDuplicates();
  TrimEntries(&bits_per_entry);

  // Make sure the safepoint table is properly aligned. Pad with nops.
  assembler->Align(Code::kMetadataAlignment);
  assembler->RecordComment(";;; Safepoint table.");
  offset_ = assembler->pc_offset();

  // Compute the number of bytes per safepoint entry.
  int bytes_per_entry =
      RoundUp(bits_per_entry, kBitsPerByte) >> kBitsPerByteLog2;

  // Emit the table header.
  STATIC_ASSERT(SafepointTable::kLengthOffset == 0 * kIntSize);
  STATIC_ASSERT(SafepointTable::kEntrySizeOffset == 1 * kIntSize);
  STATIC_ASSERT(SafepointTable::kHeaderSize == 2 * kIntSize);
  int length = static_cast<int>(deoptimization_info_.size());
  assembler->dd(length);
  assembler->dd(bytes_per_entry);

  // Emit sorted table of pc offsets together with additional info (i.e. the
  // deoptimization index or arguments count) and trampoline offsets.
  STATIC_ASSERT(SafepointTable::kPcOffset == 0 * kIntSize);
  STATIC_ASSERT(SafepointTable::kEncodedInfoOffset == 1 * kIntSize);
  STATIC_ASSERT(SafepointTable::kTrampolinePcOffset == 2 * kIntSize);
  STATIC_ASSERT(SafepointTable::kFixedEntrySize == 3 * kIntSize);
  for (const DeoptimizationInfo& info : deoptimization_info_) {
    assembler->dd(info.pc);
    if (info.register_indexes) {
      // We emit the register indexes in the same bits as the deopt_index.
      // Register indexes and deopt_index should not exist at the same time.
      DCHECK_EQ(info.deopt_index,
                static_cast<uint32_t>(Safepoint::kNoDeoptimizationIndex));
      assembler->dd(info.register_indexes);
    } else {
      assembler->dd(info.deopt_index);
    }
    assembler->dd(info.trampoline);
  }

  // Emit table of bitmaps.
  ZoneVector<uint8_t> bits(bytes_per_entry, 0, zone_);
  for (const DeoptimizationInfo& info : deoptimization_info_) {
    ZoneChunkList<int>* indexes = info.stack_indexes;
    std::fill(bits.begin(), bits.end(), 0);

    // Run through the indexes and build a bitmap.
    for (int idx : *indexes) {
      DCHECK_GT(bits_per_entry, idx);
      int index = bits_per_entry - 1 - idx;
      int byte_index = index >> kBitsPerByteLog2;
      int bit_index = index & (kBitsPerByte - 1);
      bits[byte_index] |= (1U << bit_index);
    }

    // Emit the bitmap for the current entry.
    for (int k = 0; k < bytes_per_entry; k++) {
      assembler->db(bits[k]);
    }
  }
  emitted_ = true;
}

void SafepointTableBuilder::RemoveDuplicates() {
  // If the table contains more than one entry, and all entries are identical
  // (except for the pc), replace the whole table by a single entry with pc =
  // kMaxUInt32. This especially compacts the table for wasm code without tagged
  // pointers and without deoptimization info.

  if (deoptimization_info_.size() < 2) return;

  // Check that all entries (1, size] are identical to entry 0.
  const DeoptimizationInfo& first_info = deoptimization_info_.front();
  for (auto it = deoptimization_info_.Find(1); it != deoptimization_info_.end();
       it++) {
    if (!IsIdenticalExceptForPc(first_info, *it)) return;
  }

  // If we get here, all entries were identical. Rewind the list to just one
  // entry, and set the pc to kMaxUInt32.
  deoptimization_info_.Rewind(1);
  deoptimization_info_.front().pc = kMaxUInt32;
}

void SafepointTableBuilder::TrimEntries(int* bits_per_entry) {
  int min_index = *bits_per_entry;
  if (min_index == 0) return;  // Early exit: nothing to trim.

  for (auto& info : deoptimization_info_) {
    for (int idx : *info.stack_indexes) {
      DCHECK_GT(*bits_per_entry, idx);  // Validity check.
      if (idx >= min_index) continue;
      if (idx == 0) return;  // Early exit: nothing to trim.
      min_index = idx;
    }
  }

  DCHECK_LT(0, min_index);
  *bits_per_entry -= min_index;
  for (auto& info : deoptimization_info_) {
    for (int& idx : *info.stack_indexes) {
      idx -= min_index;
    }
  }
}

bool SafepointTableBuilder::IsIdenticalExceptForPc(
    const DeoptimizationInfo& info1, const DeoptimizationInfo& info2) const {
  if (info1.deopt_index != info2.deopt_index) return false;

  ZoneChunkList<int>* indexes1 = info1.stack_indexes;
  ZoneChunkList<int>* indexes2 = info2.stack_indexes;
  if (indexes1->size() != indexes2->size()) return false;
  if (!std::equal(indexes1->begin(), indexes1->end(), indexes2->begin())) {
    return false;
  }

  if (info1.register_indexes != info2.register_indexes) return false;

  return true;
}

}  // namespace internal
}  // namespace v8
