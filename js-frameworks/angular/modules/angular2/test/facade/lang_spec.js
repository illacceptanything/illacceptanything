import {describe, it, expect, beforeEach, ddescribe, iit, xit, el} from 'angular2/test_lib';

import {ListWrapper} from 'angular2/src/facade/collection';
import {isPresent, RegExpWrapper, RegExpMatcherWrapper} from 'angular2/src/facade/lang';

export function main() {
  describe('RegExp', () => {
    it('should expose the index for each match', () => {
      var re = RegExpWrapper.create('(!)');
      var matcher = RegExpWrapper.matcher(re, '0!23!567!!');
      var indexes = [];
      var m;

      while (isPresent(m = RegExpMatcherWrapper.next(matcher))) {
        ListWrapper.push(indexes, m.index);
        expect(m[0]).toEqual('!');
        expect(m[1]).toEqual('!');
        expect(m.length).toBe(2);
      }

      expect(indexes).toEqual([1, 4, 8, 9]);
    })
 });
}
