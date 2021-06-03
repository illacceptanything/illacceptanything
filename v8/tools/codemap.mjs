// Copyright 2009 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import { SplayTree } from "./splaytree.mjs";

/**
 * Constructs a mapper that maps addresses into code entries.
 *
 * @constructor
 */
export class CodeMap {
  /**
   * Dynamic code entries. Used for JIT compiled code.
   */
  dynamics_ = new SplayTree();

  /**
   * Name generator for entries having duplicate names.
   */
  dynamicsNameGen_ = new NameGenerator();

  /**
   * Static code entries. Used for statically compiled code.
   */
  statics_ = new SplayTree();

  /**
   * Libraries entries. Used for the whole static code libraries.
   */
  libraries_ = new SplayTree();

  /**
   * Map of memory pages occupied with static code.
   */
  pages_ = [];


  /**
   * The number of alignment bits in a page address.
   */
  static PAGE_ALIGNMENT = 12;


  /**
   * Page size in bytes.
   */
  static PAGE_SIZE =  1 << CodeMap.PAGE_ALIGNMENT;


  /**
   * Adds a dynamic (i.e. moveable and discardable) code entry.
   *
   * @param {number} start The starting address.
   * @param {CodeMap.CodeEntry} codeEntry Code entry object.
   */
  addCode(start, codeEntry) {
    this.deleteAllCoveredNodes_(this.dynamics_, start, start + codeEntry.size);
    this.dynamics_.insert(start, codeEntry);
  }

  /**
   * Moves a dynamic code entry. Throws an exception if there is no dynamic
   * code entry with the specified starting address.
   *
   * @param {number} from The starting address of the entry being moved.
   * @param {number} to The destination address.
   */
  moveCode(from, to) {
    const removedNode = this.dynamics_.remove(from);
    this.deleteAllCoveredNodes_(this.dynamics_, to, to + removedNode.value.size);
    this.dynamics_.insert(to, removedNode.value);
  }

  /**
   * Discards a dynamic code entry. Throws an exception if there is no dynamic
   * code entry with the specified starting address.
   *
   * @param {number} start The starting address of the entry being deleted.
   */
  deleteCode(start) {
    const removedNode = this.dynamics_.remove(start);
  }

  /**
   * Adds a library entry.
   *
   * @param {number} start The starting address.
   * @param {CodeMap.CodeEntry} codeEntry Code entry object.
   */
  addLibrary(start, codeEntry) {
    this.markPages_(start, start + codeEntry.size);
    this.libraries_.insert(start, codeEntry);
  }

  /**
   * Adds a static code entry.
   *
   * @param {number} start The starting address.
   * @param {CodeMap.CodeEntry} codeEntry Code entry object.
   */
  addStaticCode(start, codeEntry) {
    this.statics_.insert(start, codeEntry);
  }

  /**
   * @private
   */
  markPages_(start, end) {
    for (let addr = start; addr <= end;
        addr += CodeMap.PAGE_SIZE) {
      this.pages_[(addr / CodeMap.PAGE_SIZE)|0] = 1;
    }
  }

  /**
   * @private
   */
  deleteAllCoveredNodes_(tree, start, end) {
    const to_delete = [];
    let addr = end - 1;
    while (addr >= start) {
      const node = tree.findGreatestLessThan(addr);
      if (!node) break;
      const start2 = node.key, end2 = start2 + node.value.size;
      if (start2 < end && start < end2) to_delete.push(start2);
      addr = start2 - 1;
    }
    for (let i = 0, l = to_delete.length; i < l; ++i) tree.remove(to_delete[i]);
  }

  /**
   * @private
   */
  isAddressBelongsTo_(addr, node) {
    return addr >= node.key && addr < (node.key + node.value.size);
  }

  /**
   * @private
   */
  findInTree_(tree, addr) {
    const node = tree.findGreatestLessThan(addr);
    return node && this.isAddressBelongsTo_(addr, node) ? node : null;
  }

  /**
   * Finds a code entry that contains the specified address. Both static and
   * dynamic code entries are considered. Returns the code entry and the offset
   * within the entry.
   *
   * @param {number} addr Address.
   */
  findAddress(addr) {
    const pageAddr = (addr / CodeMap.PAGE_SIZE)|0;
    if (pageAddr in this.pages_) {
      // Static code entries can contain "holes" of unnamed code.
      // In this case, the whole library is assigned to this address.
      let result = this.findInTree_(this.statics_, addr);
      if (!result) {
        result = this.findInTree_(this.libraries_, addr);
        if (!result) return null;
      }
      return { entry : result.value, offset : addr - result.key };
    }
    const min = this.dynamics_.findMin();
    const max = this.dynamics_.findMax();
    if (max != null && addr < (max.key + max.value.size) && addr >= min.key) {
      const dynaEntry = this.findInTree_(this.dynamics_, addr);
      if (dynaEntry == null) return null;
      // Dedupe entry name.
      const entry = dynaEntry.value;
      if (!entry.nameUpdated_) {
        entry.name = this.dynamicsNameGen_.getName(entry.name);
        entry.nameUpdated_ = true;
      }
      return { entry, offset : addr - dynaEntry.key };
    }
    return null;
  }

  /**
   * Finds a code entry that contains the specified address. Both static and
   * dynamic code entries are considered.
   *
   * @param {number} addr Address.
   */
  findEntry(addr) {
    const result = this.findAddress(addr);
    return result ? result.entry : null;
  }

  /**
   * Returns a dynamic code entry using its starting address.
   *
   * @param {number} addr Address.
   */
  findDynamicEntryByStartAddress(addr) {
    const node = this.dynamics_.find(addr);
    return node ? node.value : null;
  }

  /**
   * Returns an array of all dynamic code entries.
   */
  getAllDynamicEntries() {
    return this.dynamics_.exportValues();
  }

  /**
   * Returns an array of pairs of all dynamic code entries and their addresses.
   */
  getAllDynamicEntriesWithAddresses() {
    return this.dynamics_.exportKeysAndValues();
  }

  /**
   * Returns an array of all static code entries.
   */
  getAllStaticEntries() {
    return this.statics_.exportValues();
  }

  /**
   * Returns an array of pairs of all static code entries and their addresses.
   */
  getAllStaticEntriesWithAddresses() {
    return this.statics_.exportKeysAndValues();
  }

  /**
   * Returns an array of all library entries.
   */
  getAllLibraryEntries() {
    return this.libraries_.exportValues();
  }

  /**
   * Returns an array of pairs of all library entries and their addresses.
   */
  getAllLibraryEntriesWithAddresses() {
    return this.libraries_.exportKeysAndValues();
  }
}


/**
 * Creates a code entry object.
 *
 * @param {number} size Code entry size in bytes.
 * @param {string} opt_name Code entry name.
 * @param {string} opt_type Code entry type, e.g. SHARED_LIB, CPP.
 * @param {object} source Optional source position information
 * @constructor
 */
export class CodeEntry {
  constructor(size, opt_name, opt_type) {
    this.size = size;
    this.name = opt_name || '';
    this.type = opt_type || '';
    this.nameUpdated_ = false;
    this.source = undefined;
  }

  getName() {
    return this.name;
  }

  toString() {
    return this.name + ': ' + this.size.toString(16);
  }

  getSourceCode() {
    return '';
  }
}

class NameGenerator {
  knownNames_ = { __proto__:null }
  getName(name) {
    if (!(name in this.knownNames_)) {
      this.knownNames_[name] = 0;
      return name;
    }
    const count = ++this.knownNames_[name];
    return name + ' {' + count + '}';
  };
}
