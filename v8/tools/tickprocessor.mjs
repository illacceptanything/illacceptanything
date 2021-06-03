// Copyright 2012 the V8 project authors. All rights reserved.
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

import { LogReader, parseString, parseVarArgs } from "./logreader.mjs";
import { BaseArgumentsProcessor, parseBool } from "./arguments.mjs";
import { Profile, JsonProfile } from "./profile.mjs";
import { ViewBuilder } from "./profile_view.mjs";


class V8Profile extends Profile {
  static IC_RE =
      /^(LoadGlobalIC: )|(Handler: )|(?:CallIC|LoadIC|StoreIC)|(?:Builtin: (?:Keyed)?(?:Load|Store)IC_)/;
  static BYTECODES_RE = /^(BytecodeHandler: )/;
  static BASELINE_HANDLERS_RE = /^(Builtin: .*Baseline.*)/;
  static BUILTINS_RE = /^(Builtin: )/;
  static STUBS_RE = /^(Stub: )/;

  constructor(separateIc, separateBytecodes, separateBuiltins, separateStubs,
        separateBaselineHandlers) {
    super();
    const regexps = [];
    if (!separateIc) regexps.push(V8Profile.IC_RE);
    if (!separateBytecodes) regexps.push(V8Profile.BYTECODES_RE);
    if (!separateBuiltins) regexps.push(V8Profile.BUILTINS_RE);
    if (!separateStubs) regexps.push(V8Profile.STUBS_RE);
    if (regexps.length > 0) {
      this.skipThisFunction = function(name) {
        for (let i = 0; i < regexps.length; i++) {
          if (regexps[i].test(name)) return true;
        }
        return false;
      };
    }
  }
}


/**
 * A thin wrapper around shell's 'read' function showing a file name on error.
 */
export function readFile(fileName) {
  try {
    return read(fileName);
  } catch (e) {
    printErr(`file="${fileName}": ${e.message || e}`);
    throw e;
  }
}


export class TickProcessor extends LogReader {
  constructor(
    cppEntriesProvider,
    separateIc,
    separateBytecodes,
    separateBuiltins,
    separateStubs,
    separateBaselineHandlers,
    callGraphSize,
    ignoreUnknown,
    stateFilter,
    distortion,
    range,
    sourceMap,
    timedRange,
    pairwiseTimedRange,
    onlySummary,
    runtimeTimerFilter,
    preprocessJson) {
    super({},
      timedRange,
      pairwiseTimedRange);
    this.dispatchTable_ = {
      'shared-library': {
        parsers: [parseString, parseInt, parseInt, parseInt],
        processor: this.processSharedLibrary
      },
      'code-creation': {
        parsers: [parseString, parseInt, parseInt, parseInt, parseInt,
          parseString, parseVarArgs],
        processor: this.processCodeCreation
      },
      'code-deopt': {
        parsers: [parseInt, parseInt, parseInt, parseInt, parseInt,
          parseString, parseString, parseString],
        processor: this.processCodeDeopt
      },
      'code-move': {
        parsers: [parseInt, parseInt,],
        processor: this.processCodeMove
      },
      'code-delete': {
        parsers: [parseInt],
        processor: this.processCodeDelete
      },
      'code-source-info': {
        parsers: [parseInt, parseInt, parseInt, parseInt, parseString,
          parseString, parseString],
        processor: this.processCodeSourceInfo
      },
      'script-source': {
        parsers: [parseInt, parseString, parseString],
        processor: this.processScriptSource
      },
      'sfi-move': {
        parsers: [parseInt, parseInt],
        processor: this.processFunctionMove
      },
      'active-runtime-timer': {
        parsers: [parseString],
        processor: this.processRuntimeTimerEvent
      },
      'tick': {
        parsers: [parseInt, parseInt, parseInt,
          parseInt, parseInt, parseVarArgs],
        processor: this.processTick
      },
      'heap-sample-begin': {
        parsers: [parseString, parseString, parseInt],
        processor: this.processHeapSampleBegin
      },
      'heap-sample-end': {
        parsers: [parseString, parseString],
        processor: this.processHeapSampleEnd
      },
      'timer-event-start': {
        parsers: [parseString, parseString, parseString],
        processor: this.advanceDistortion
      },
      'timer-event-end': {
        parsers: [parseString, parseString, parseString],
        processor: this.advanceDistortion
      },
      // Ignored events.
      'profiler': null,
      'function-creation': null,
      'function-move': null,
      'function-delete': null,
      'heap-sample-item': null,
      'current-time': null,  // Handled specially, not parsed.
      // Obsolete row types.
      'code-allocate': null,
      'begin-code-region': null,
      'end-code-region': null
    };

    this.preprocessJson = preprocessJson;
    this.cppEntriesProvider_ = cppEntriesProvider;
    this.callGraphSize_ = callGraphSize;
    this.ignoreUnknown_ = ignoreUnknown;
    this.stateFilter_ = stateFilter;
    this.runtimeTimerFilter_ = runtimeTimerFilter;
    this.sourceMap = sourceMap;
    const ticks = this.ticks_ =
      { total: 0, unaccounted: 0, excluded: 0, gc: 0 };

    distortion = parseInt(distortion);
    // Convert picoseconds to nanoseconds.
    this.distortion_per_entry = isNaN(distortion) ? 0 : (distortion / 1000);
    this.distortion = 0;
    const rangelimits = range ? range.split(",") : [];
    const range_start = parseInt(rangelimits[0]);
    const range_end = parseInt(rangelimits[1]);
    // Convert milliseconds to nanoseconds.
    this.range_start = isNaN(range_start) ? -Infinity : (range_start * 1000);
    this.range_end = isNaN(range_end) ? Infinity : (range_end * 1000)

    V8Profile.prototype.handleUnknownCode = function (
      operation, addr, opt_stackPos) {
      const op = Profile.Operation;
      switch (operation) {
        case op.MOVE:
          printErr(`Code move event for unknown code: 0x${addr.toString(16)}`);
          break;
        case op.DELETE:
          printErr(`Code delete event for unknown code: 0x${addr.toString(16)}`);
          break;
        case op.TICK:
          // Only unknown PCs (the first frame) are reported as unaccounted,
          // otherwise tick balance will be corrupted (this behavior is compatible
          // with the original tickprocessor.py script.)
          if (opt_stackPos == 0) {
            ticks.unaccounted++;
          }
          break;
      }
    };

    if (preprocessJson) {
      this.profile_ = new JsonProfile();
    } else {
      this.profile_ = new V8Profile(separateIc, separateBytecodes,
        separateBuiltins, separateStubs, separateBaselineHandlers);
    }
    this.codeTypes_ = {};
    // Count each tick as a time unit.
    this.viewBuilder_ = new ViewBuilder(1);
    this.lastLogFileName_ = null;

    this.generation_ = 1;
    this.currentProducerProfile_ = null;
    this.onlySummary_ = onlySummary;
  }

  static VmStates = {
    JS: 0,
    GC: 1,
    PARSER: 2,
    BYTECODE_COMPILER: 3,
    // TODO(cbruni): add BASELINE_COMPILER
    COMPILER: 4,
    OTHER: 5,
    EXTERNAL: 6,
    IDLE: 7,
  };

  static CodeTypes = {
    CPP: 0,
    SHARED_LIB: 1
  };
  // Otherwise, this is JS-related code. We are not adding it to
  // codeTypes_ map because there can be zillions of them.

  static CALL_PROFILE_CUTOFF_PCT = 1.0;
  static CALL_GRAPH_SIZE = 5;

  /**
   * @override
   */
  printError(str) {
    printErr(str);
  }

  setCodeType(name, type) {
    this.codeTypes_[name] = TickProcessor.CodeTypes[type];
  }

  isSharedLibrary(name) {
    return this.codeTypes_[name] == TickProcessor.CodeTypes.SHARED_LIB;
  }

  isCppCode(name) {
    return this.codeTypes_[name] == TickProcessor.CodeTypes.CPP;
  }

  isJsCode(name) {
    return name !== "UNKNOWN" && !(name in this.codeTypes_);
  }

  processLogFile(fileName) {
    this.lastLogFileName_ = fileName;
    let line;
    while (line = readline()) {
      this.processLogLine(line);
    }
  }

  processLogFileInTest(fileName) {
    // Hack file name to avoid dealing with platform specifics.
    this.lastLogFileName_ = 'v8.log';
    const contents = readFile(fileName);
    this.processLogChunk(contents);
  }

  processSharedLibrary(name, startAddr, endAddr, aslrSlide) {
    const entry = this.profile_.addLibrary(name, startAddr, endAddr, aslrSlide);
    this.setCodeType(entry.getName(), 'SHARED_LIB');
    this.cppEntriesProvider_.parseVmSymbols(
      name, startAddr, endAddr, aslrSlide, (fName, fStart, fEnd) => {
        this.profile_.addStaticCode(fName, fStart, fEnd);
        this.setCodeType(fName, 'CPP');
      });
  }

  processCodeCreation(type, kind, timestamp, start, size, name, maybe_func) {
    if (type != 'RegExp' && maybe_func.length) {
      const funcAddr = parseInt(maybe_func[0]);
      const state = Profile.parseState(maybe_func[1]);
      this.profile_.addFuncCode(type, name, timestamp, start, size, funcAddr, state);
    } else {
      this.profile_.addCode(type, name, timestamp, start, size);
    }
  }

  processCodeDeopt(
      timestamp, size, code, inliningId, scriptOffset, bailoutType,
      sourcePositionText, deoptReasonText) {
    this.profile_.deoptCode(timestamp, code, inliningId, scriptOffset,
      bailoutType, sourcePositionText, deoptReasonText);
  }

  processCodeMove(from, to) {
    this.profile_.moveCode(from, to);
  }

  processCodeDelete(start) {
    this.profile_.deleteCode(start);
  }

  processCodeSourceInfo(
      start, script, startPos, endPos, sourcePositions, inliningPositions,
      inlinedFunctions) {
    this.profile_.addSourcePositions(start, script, startPos,
      endPos, sourcePositions, inliningPositions, inlinedFunctions);
  }

  processScriptSource(script, url, source) {
    this.profile_.addScriptSource(script, url, source);
  }

  processFunctionMove(from, to) {
    this.profile_.moveFunc(from, to);
  }

  includeTick(vmState) {
    if (this.stateFilter_ !== null) {
      return this.stateFilter_ == vmState;
    } else if (this.runtimeTimerFilter_ !== null) {
      return this.currentRuntimeTimer == this.runtimeTimerFilter_;
    }
    return true;
  }

  processRuntimeTimerEvent(name) {
    this.currentRuntimeTimer = name;
  }

  processTick(pc,
    ns_since_start,
    is_external_callback,
    tos_or_external_callback,
    vmState,
    stack) {
    this.distortion += this.distortion_per_entry;
    ns_since_start -= this.distortion;
    if (ns_since_start < this.range_start || ns_since_start > this.range_end) {
      return;
    }
    this.ticks_.total++;
    if (vmState == TickProcessor.VmStates.GC) this.ticks_.gc++;
    if (!this.includeTick(vmState)) {
      this.ticks_.excluded++;
      return;
    }
    if (is_external_callback) {
      // Don't use PC when in external callback code, as it can point
      // inside callback's code, and we will erroneously report
      // that a callback calls itself. Instead we use tos_or_external_callback,
      // as simply resetting PC will produce unaccounted ticks.
      pc = tos_or_external_callback;
      tos_or_external_callback = 0;
    } else if (tos_or_external_callback) {
      // Find out, if top of stack was pointing inside a JS function
      // meaning that we have encountered a frameless invocation.
      const funcEntry = this.profile_.findEntry(tos_or_external_callback);
      if (!funcEntry || !funcEntry.isJSFunction || !funcEntry.isJSFunction()) {
        tos_or_external_callback = 0;
      }
    }

    this.profile_.recordTick(
      ns_since_start, vmState,
      this.processStack(pc, tos_or_external_callback, stack));
  }

  advanceDistortion() {
    this.distortion += this.distortion_per_entry;
  }

  processHeapSampleBegin(space, state, ticks) {
    if (space != 'Heap') return;
    this.currentProducerProfile_ = new CallTree();
  }

  processHeapSampleEnd(space, state) {
    if (space != 'Heap' || !this.currentProducerProfile_) return;

    print(`Generation ${this.generation_}:`);
    const tree = this.currentProducerProfile_;
    tree.computeTotalWeights();
    const producersView = this.viewBuilder_.buildView(tree);
    // Sort by total time, desc, then by name, desc.
    producersView.sort((rec1, rec2) =>
      rec2.totalTime - rec1.totalTime ||
      (rec2.internalFuncName < rec1.internalFuncName ? -1 : 1));
    this.printHeavyProfile(producersView.head.children);

    this.currentProducerProfile_ = null;
    this.generation_++;
  }

  printVMSymbols() {
    console.log(
      JSON.stringify(this.profile_.serializeVMSymbols()));
  }

  printStatistics() {
    if (this.preprocessJson) {
      this.profile_.writeJson();
      return;
    }

    print(`Statistical profiling result from ${this.lastLogFileName_}` +
      `, (${this.ticks_.total} ticks, ${this.ticks_.unaccounted} unaccounted, ` +
      `${this.ticks_.excluded} excluded).`);


    if (this.ticks_.total == 0) return;

    const flatProfile = this.profile_.getFlatProfile();
    const flatView = this.viewBuilder_.buildView(flatProfile);
    // Sort by self time, desc, then by name, desc.
    flatView.sort((rec1, rec2) =>
      rec2.selfTime - rec1.selfTime ||
      (rec2.internalFuncName < rec1.internalFuncName ? -1 : 1));
    let totalTicks = this.ticks_.total;
    if (this.ignoreUnknown_) {
      totalTicks -= this.ticks_.unaccounted;
    }
    const printAllTicks = !this.onlySummary_;

    // Count library ticks
    const flatViewNodes = flatView.head.children;

    let libraryTicks = 0;
    if (printAllTicks) this.printHeader('Shared libraries');
    this.printEntries(flatViewNodes, totalTicks, null,
      name => this.isSharedLibrary(name),
      (rec) => { libraryTicks += rec.selfTime; }, printAllTicks);
    const nonLibraryTicks = totalTicks - libraryTicks;

    let jsTicks = 0;
    if (printAllTicks) this.printHeader('JavaScript');
    this.printEntries(flatViewNodes, totalTicks, nonLibraryTicks,
      name => this.isJsCode(name),
      (rec) => { jsTicks += rec.selfTime; }, printAllTicks);

    let cppTicks = 0;
    if (printAllTicks) this.printHeader('C++');
    this.printEntries(flatViewNodes, totalTicks, nonLibraryTicks,
      name => this.isCppCode(name),
      (rec) => { cppTicks += rec.selfTime; }, printAllTicks);

    this.printHeader('Summary');
    this.printLine('JavaScript', jsTicks, totalTicks, nonLibraryTicks);
    this.printLine('C++', cppTicks, totalTicks, nonLibraryTicks);
    this.printLine('GC', this.ticks_.gc, totalTicks, nonLibraryTicks);
    this.printLine('Shared libraries', libraryTicks, totalTicks, null);
    if (!this.ignoreUnknown_ && this.ticks_.unaccounted > 0) {
      this.printLine('Unaccounted', this.ticks_.unaccounted,
        this.ticks_.total, null);
    }

    if (printAllTicks) {
      print('\n [C++ entry points]:');
      print('   ticks    cpp   total   name');
      const c_entry_functions = this.profile_.getCEntryProfile();
      const total_c_entry = c_entry_functions[0].ticks;
      for (let i = 1; i < c_entry_functions.length; i++) {
        const c = c_entry_functions[i];
        this.printLine(c.name, c.ticks, total_c_entry, totalTicks);
      }

      this.printHeavyProfHeader();
      const heavyProfile = this.profile_.getBottomUpProfile();
      const heavyView = this.viewBuilder_.buildView(heavyProfile);
      // To show the same percentages as in the flat profile.
      heavyView.head.totalTime = totalTicks;
      // Sort by total time, desc, then by name, desc.
      heavyView.sort((rec1, rec2) =>
        rec2.totalTime - rec1.totalTime ||
        (rec2.internalFuncName < rec1.internalFuncName ? -1 : 1));
      this.printHeavyProfile(heavyView.head.children);
    }
  }

  printHeader(headerTitle) {
    print(`\n [${headerTitle}]:`);
    print('   ticks  total  nonlib   name');
  }

  printLine(
    entry, ticks, totalTicks, nonLibTicks) {
    const pct = ticks * 100 / totalTicks;
    const nonLibPct = nonLibTicks != null
      ? `${(ticks * 100 / nonLibTicks).toFixed(1).toString().padStart(5)}%  `
      : '        ';
    print(`${`  ${ticks.toString().padStart(5)}  ` +
      pct.toFixed(1).toString().padStart(5)}%  ${nonLibPct}${entry}`);
  }

  printHeavyProfHeader() {
    print('\n [Bottom up (heavy) profile]:');
    print('  Note: percentage shows a share of a particular caller in the ' +
      'total\n' +
      '  amount of its parent calls.');
    print(`  Callers occupying less than ${TickProcessor.CALL_PROFILE_CUTOFF_PCT.toFixed(1)}% are not shown.\n`);
    print('   ticks parent  name');
  }

  processProfile(profile, filterP, func) {
    for (let i = 0, n = profile.length; i < n; ++i) {
      const rec = profile[i];
      if (!filterP(rec.internalFuncName)) {
        continue;
      }
      func(rec);
    }
  }

  getLineAndColumn(name) {
    const re = /:([0-9]+):([0-9]+)$/;
    const array = re.exec(name);
    if (!array) {
      return null;
    }
    return { line: array[1], column: array[2] };
  }

  hasSourceMap() {
    return this.sourceMap != null;
  }

  formatFunctionName(funcName) {
    if (!this.hasSourceMap()) {
      return funcName;
    }
    const lc = this.getLineAndColumn(funcName);
    if (lc == null) {
      return funcName;
    }
    // in source maps lines and columns are zero based
    const lineNumber = lc.line - 1;
    const column = lc.column - 1;
    const entry = this.sourceMap.findEntry(lineNumber, column);
    const sourceFile = entry[2];
    const sourceLine = entry[3] + 1;
    const sourceColumn = entry[4] + 1;

    return `${sourceFile}:${sourceLine}:${sourceColumn} -> ${funcName}`;
  }

  printEntries(
        profile, totalTicks, nonLibTicks, filterP, callback, printAllTicks) {
    this.processProfile(profile, filterP, (rec) => {
      if (rec.selfTime == 0) return;
      callback(rec);
      const funcName = this.formatFunctionName(rec.internalFuncName);
      if (printAllTicks) {
        this.printLine(funcName, rec.selfTime, totalTicks, nonLibTicks);
      }
    });
  }

  printHeavyProfile(profile, opt_indent) {
    const indent = opt_indent || 0;
    const indentStr = ''.padStart(indent);
    this.processProfile(profile, () => true, (rec) => {
      // Cut off too infrequent callers.
      if (rec.parentTotalPercent < TickProcessor.CALL_PROFILE_CUTOFF_PCT) return;
      const funcName = this.formatFunctionName(rec.internalFuncName);
      print(`${`  ${rec.totalTime.toString().padStart(5)}  ` +
        rec.parentTotalPercent.toFixed(1).toString().padStart(5)}%  ${indentStr}${funcName}`);
      // Limit backtrace depth.
      if (indent < 2 * this.callGraphSize_) {
        this.printHeavyProfile(rec.children, indent + 2);
      }
      // Delimit top-level functions.
      if (indent == 0) print('');
    });
  }
}


class CppEntriesProvider {
  inRange(funcInfo, start, end) {
    return funcInfo.start >= start && funcInfo.end <= end;
  }

  parseVmSymbols(libName, libStart, libEnd, libASLRSlide, processorFunc) {
    this.loadSymbols(libName);

    let lastUnknownSize;
    let lastAdded;

    let addEntry = (funcInfo) => {
      // Several functions can be mapped onto the same address. To avoid
      // creating zero-sized entries, skip such duplicates.
      // Also double-check that function belongs to the library address space.

      if (lastUnknownSize &&
        lastUnknownSize.start < funcInfo.start) {
        // Try to update lastUnknownSize based on new entries start position.
        lastUnknownSize.end = funcInfo.start;
        if ((!lastAdded ||
            !this.inRange(lastUnknownSize, lastAdded.start, lastAdded.end)) &&
            this.inRange(lastUnknownSize, libStart, libEnd)) {
          processorFunc(
              lastUnknownSize.name, lastUnknownSize.start, lastUnknownSize.end);
          lastAdded = lastUnknownSize;
        }
      }
      lastUnknownSize = undefined;

      if (funcInfo.end) {
        // Skip duplicates that have the same start address as the last added.
        if ((!lastAdded || lastAdded.start != funcInfo.start) &&
          this.inRange(funcInfo, libStart, libEnd)) {
          processorFunc(funcInfo.name, funcInfo.start, funcInfo.end);
          lastAdded = funcInfo;
        }
      } else {
        // If a funcInfo doesn't have an end, try to match it up with then next
        // entry.
        lastUnknownSize = funcInfo;
      }
    }

    while (true) {
      const funcInfo = this.parseNextLine();
      if (funcInfo === null) continue;
      if (funcInfo === false) break;
      if (funcInfo.start < libStart - libASLRSlide &&
        funcInfo.start < libEnd - libStart) {
        funcInfo.start += libStart;
      } else {
        funcInfo.start += libASLRSlide;
      }
      if (funcInfo.size) {
        funcInfo.end = funcInfo.start + funcInfo.size;
      }
      addEntry(funcInfo);
    }
    addEntry({ name: '', start: libEnd });
  }

  loadSymbols(libName) {}

  parseNextLine() { return false }
}


export class UnixCppEntriesProvider extends CppEntriesProvider {
  constructor(nmExec, objdumpExec, targetRootFS, apkEmbeddedLibrary) {
    super();
    this.symbols = [];
    // File offset of a symbol minus the virtual address of a symbol found in
    // the symbol table.
    this.fileOffsetMinusVma = 0;
    this.parsePos = 0;
    this.nmExec = nmExec;
    this.objdumpExec = objdumpExec;
    this.targetRootFS = targetRootFS;
    this.apkEmbeddedLibrary = apkEmbeddedLibrary;
    this.FUNC_RE = /^([0-9a-fA-F]{8,16}) ([0-9a-fA-F]{8,16} )?[tTwW] (.*)$/;
  }


  loadSymbols(libName) {
    this.parsePos = 0;
    if (this.apkEmbeddedLibrary && libName.endsWith('.apk')) {
      libName = this.apkEmbeddedLibrary;
    }
    if (this.targetRootFS) {
      libName = libName.substring(libName.lastIndexOf('/') + 1);
      libName = this.targetRootFS + libName;
    }
    try {
      this.symbols = [
        os.system(this.nmExec, ['-C', '-n', '-S', libName], -1, -1),
        os.system(this.nmExec, ['-C', '-n', '-S', '-D', libName], -1, -1)
      ];

      const objdumpOutput = os.system(this.objdumpExec, ['-h', libName], -1, -1);
      for (const line of objdumpOutput.split('\n')) {
        const [, sectionName, , vma, , fileOffset] = line.trim().split(/\s+/);
        if (sectionName === ".text") {
          this.fileOffsetMinusVma = parseInt(fileOffset, 16) - parseInt(vma, 16);
        }
      }
    } catch (e) {
      // If the library cannot be found on this system let's not panic.
      this.symbols = ['', ''];
    }
  }

  parseNextLine() {
    if (this.symbols.length == 0) {
      return false;
    }
    const lineEndPos = this.symbols[0].indexOf('\n', this.parsePos);
    if (lineEndPos == -1) {
      this.symbols.shift();
      this.parsePos = 0;
      return this.parseNextLine();
    }

    const line = this.symbols[0].substring(this.parsePos, lineEndPos);
    this.parsePos = lineEndPos + 1;
    const fields = line.match(this.FUNC_RE);
    let funcInfo = null;
    if (fields) {
      funcInfo = { name: fields[3], start: parseInt(fields[1], 16) + this.fileOffsetMinusVma };
      if (fields[2]) {
        funcInfo.size = parseInt(fields[2], 16);
      }
    }
    return funcInfo;
  }
}

export class MacCppEntriesProvider extends UnixCppEntriesProvider {
  constructor(nmExec, objdumpExec, targetRootFS, apkEmbeddedLibrary) {
    super(nmExec, objdumpExec, targetRootFS, apkEmbeddedLibrary);
    // Note an empty group. It is required, as UnixCppEntriesProvider expects 3 groups.
    this.FUNC_RE = /^([0-9a-fA-F]{8,16})() (.*)$/;
  }

  loadSymbols(libName) {
    this.parsePos = 0;
    libName = this.targetRootFS + libName;

    // It seems that in OS X `nm` thinks that `-f` is a format option, not a
    // "flat" display option flag.
    try {
      this.symbols = [os.system(this.nmExec, ['-n', libName], -1, -1), ''];
    } catch (e) {
      // If the library cannot be found on this system let's not panic.
      this.symbols = '';
    }
  }
}


export class WindowsCppEntriesProvider extends CppEntriesProvider {
  constructor(_ignored_nmExec, _ignored_objdumpExec, targetRootFS,
    _ignored_apkEmbeddedLibrary) {
    super();
    this.targetRootFS = targetRootFS;
    this.symbols = '';
    this.parsePos = 0;
  }

  static FILENAME_RE = /^(.*)\.([^.]+)$/;
  static FUNC_RE =
    /^\s+0001:[0-9a-fA-F]{8}\s+([_\?@$0-9a-zA-Z]+)\s+([0-9a-fA-F]{8}).*$/;
  static IMAGE_BASE_RE =
    /^\s+0000:00000000\s+___ImageBase\s+([0-9a-fA-F]{8}).*$/;
  // This is almost a constant on Windows.
  static EXE_IMAGE_BASE = 0x00400000;

  loadSymbols(libName) {
    libName = this.targetRootFS + libName;
    const fileNameFields = libName.match(WindowsCppEntriesProvider.FILENAME_RE);
    if (!fileNameFields) return;
    const mapFileName = `${fileNameFields[1]}.map`;
    this.moduleType_ = fileNameFields[2].toLowerCase();
    try {
      this.symbols = read(mapFileName);
    } catch (e) {
      // If .map file cannot be found let's not panic.
      this.symbols = '';
    }
  }

  parseNextLine() {
    const lineEndPos = this.symbols.indexOf('\r\n', this.parsePos);
    if (lineEndPos == -1) {
      return false;
    }

    const line = this.symbols.substring(this.parsePos, lineEndPos);
    this.parsePos = lineEndPos + 2;

    // Image base entry is above all other symbols, so we can just
    // terminate parsing.
    const imageBaseFields = line.match(WindowsCppEntriesProvider.IMAGE_BASE_RE);
    if (imageBaseFields) {
      const imageBase = parseInt(imageBaseFields[1], 16);
      if ((this.moduleType_ == 'exe') !=
        (imageBase == WindowsCppEntriesProvider.EXE_IMAGE_BASE)) {
        return false;
      }
    }

    const fields = line.match(WindowsCppEntriesProvider.FUNC_RE);
    return fields ?
      { name: this.unmangleName(fields[1]), start: parseInt(fields[2], 16) } :
      null;
  }

  /**
   * Performs very simple unmangling of C++ names.
   *
   * Does not handle arguments and template arguments. The mangled names have
   * the form:
   *
   *   ?LookupInDescriptor@JSObject@internal@v8@@...arguments info...
   */
  unmangleName(name) {
    // Empty or non-mangled name.
    if (name.length < 1 || name.charAt(0) != '?') return name;
    const nameEndPos = name.indexOf('@@');
    const components = name.substring(1, nameEndPos).split('@');
    components.reverse();
    return components.join('::');
  }
}


export class ArgumentsProcessor extends BaseArgumentsProcessor {
  getArgsDispatch() {
    let dispatch = {
      '-j': ['stateFilter', TickProcessor.VmStates.JS,
        'Show only ticks from JS VM state'],
      '-g': ['stateFilter', TickProcessor.VmStates.GC,
        'Show only ticks from GC VM state'],
      '-p': ['stateFilter', TickProcessor.VmStates.PARSER,
        'Show only ticks from PARSER VM state'],
      '-b': ['stateFilter', TickProcessor.VmStates.BYTECODE_COMPILER,
        'Show only ticks from BYTECODE_COMPILER VM state'],
      '-c': ['stateFilter', TickProcessor.VmStates.COMPILER,
        'Show only ticks from COMPILER VM state'],
      '-o': ['stateFilter', TickProcessor.VmStates.OTHER,
        'Show only ticks from OTHER VM state'],
      '-e': ['stateFilter', TickProcessor.VmStates.EXTERNAL,
        'Show only ticks from EXTERNAL VM state'],
      '--filter-runtime-timer': ['runtimeTimerFilter', null,
        'Show only ticks matching the given runtime timer scope'],
      '--call-graph-size': ['callGraphSize', TickProcessor.CALL_GRAPH_SIZE,
        'Set the call graph size'],
      '--ignore-unknown': ['ignoreUnknown', true,
        'Exclude ticks of unknown code entries from processing'],
      '--separate-ic': ['separateIc', parseBool,
        'Separate IC entries'],
      '--separate-bytecodes': ['separateBytecodes', parseBool,
        'Separate Bytecode entries'],
      '--separate-builtins': ['separateBuiltins', parseBool,
        'Separate Builtin entries'],
      '--separate-stubs': ['separateStubs', parseBool,
        'Separate Stub entries'],
      '--separate-baseline-handlers': ['separateBaselineHandlers', parseBool,
        'Separate Baseline Handler entries'],
      '--unix': ['platform', 'unix',
        'Specify that we are running on *nix platform'],
      '--windows': ['platform', 'windows',
        'Specify that we are running on Windows platform'],
      '--mac': ['platform', 'mac',
        'Specify that we are running on Mac OS X platform'],
      '--nm': ['nm', 'nm',
        'Specify the \'nm\' executable to use (e.g. --nm=/my_dir/nm)'],
      '--objdump': ['objdump', 'objdump',
        'Specify the \'objdump\' executable to use (e.g. --objdump=/my_dir/objdump)'],
      '--target': ['targetRootFS', '',
        'Specify the target root directory for cross environment'],
      '--apk-embedded-library': ['apkEmbeddedLibrary', '',
        'Specify the path of the embedded library for Android traces'],
      '--range': ['range', 'auto,auto',
        'Specify the range limit as [start],[end]'],
      '--distortion': ['distortion', 0,
        'Specify the logging overhead in picoseconds'],
      '--source-map': ['sourceMap', null,
        'Specify the source map that should be used for output'],
      '--timed-range': ['timedRange', true,
        'Ignore ticks before first and after last Date.now() call'],
      '--pairwise-timed-range': ['pairwiseTimedRange', true,
        'Ignore ticks outside pairs of Date.now() calls'],
      '--only-summary': ['onlySummary', true,
        'Print only tick summary, exclude other information'],
      '--serialize-vm-symbols': ['serializeVMSymbols', true,
        'Print all C++ symbols and library addresses as JSON data'],
      '--preprocess': ['preprocessJson', true,
        'Preprocess for consumption with web interface']
    };
    dispatch['--js'] = dispatch['-j'];
    dispatch['--gc'] = dispatch['-g'];
    dispatch['--compiler'] = dispatch['-c'];
    dispatch['--other'] = dispatch['-o'];
    dispatch['--external'] = dispatch['-e'];
    dispatch['--ptr'] = dispatch['--pairwise-timed-range'];
    return dispatch;
  }

  getDefaultResults() {
    return {
      logFileName: 'v8.log',
      platform: 'unix',
      stateFilter: null,
      callGraphSize: 5,
      ignoreUnknown: false,
      separateIc: true,
      separateBytecodes: false,
      separateBuiltins: true,
      separateStubs: true,
      separateBaselineHandlers: false,
      preprocessJson: null,
      targetRootFS: '',
      nm: 'nm',
      objdump: 'objdump',
      range: 'auto,auto',
      distortion: 0,
      timedRange: false,
      pairwiseTimedRange: false,
      onlySummary: false,
      runtimeTimerFilter: null,
      serializeVMSymbols: false,
    };
  }
}
