import {ddescribe, describe, it, iit, xit, expect, beforeEach, afterEach} from 'angular2/test_lib';

import {coalesce} from 'angular2/src/change_detection/coalesce';
import {RECORD_TYPE_SELF, ProtoRecord} from 'angular2/src/change_detection/proto_record';

export function main() {
  function r(funcOrValue, args, contextIndex, selfIndex, lastInBinding = false) {
    return new ProtoRecord(99,  "name", funcOrValue, args, null, contextIndex, selfIndex,
      null, null, null, lastInBinding, false);
  }

  describe("change detection - coalesce", () => {
    it("should work with an empty list", () => {
      expect(coalesce([])).toEqual([]);
    });

    it("should remove non-terminal duplicate records" +
      " and update the context indices referencing them", () => {
      var rs = coalesce([
        r("user",   [], 0, 1),
        r("first",  [], 1, 2),
        r("user",   [], 0, 3),
        r("last",   [], 3, 4)
      ]);

      expect(rs).toEqual([
        r("user",   [], 0, 1),
        r("first",  [], 1, 2),
        r("last",   [], 1, 3)
      ]);
    });

    it("should update indices of other records", () => {
      var rs = coalesce([
        r("dup",   [],  0, 1),
        r("dup",   [],  0, 2),
        r("user",  [],  0, 3),
        r("first", [3], 3, 4)
      ]);

      expect(rs).toEqual([
        r("dup",   [],  0, 1),
        r("user",  [],  0, 2),
        r("first", [2], 2, 3)
      ]);
    });

    it("should remove non-terminal duplicate records" +
      " and update the args indices referencing them", () => {
      var rs = coalesce([
        r("user1",  [],  0, 1),
        r("user2",  [],  0, 2),
        r("hi",     [1], 0, 3),
        r("hi",     [1], 0, 4),
        r("hi",     [2], 0, 5)
      ]);

      expect(rs).toEqual([
        r("user1",  [],  0, 1),
        r("user2",  [],  0, 2),
        r("hi",     [1], 0, 3),
        r("hi",     [2], 0, 4)
      ]);
    });

    it("should replace duplicate terminal records with" +
      " self records", () => {

      var rs = coalesce([
        r("user",  [],  0, 1, true),
        r("user",  [],  0, 2, true)
      ]);

      expect(rs[1]).toEqual(new ProtoRecord(
        RECORD_TYPE_SELF, "self", null,
        [], null, 1, 2,
        null, null, null,
        true, false)
      );
    });
  });
}
