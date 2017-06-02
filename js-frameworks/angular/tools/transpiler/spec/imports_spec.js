import {describe, it, expect, IS_DARTIUM} from 'angular2/test_lib';

import {Foo, Bar} from './foo';
// TODO: Does not work, as dart does not support renaming imports
// import {Foo as F} from './fixtures/foo';
import * as fooModule from './foo';

import * as exportModule from './export';

import {Type} from 'angular2/src/facade/lang';

import {Baz} from './reexport';

export function main() {
  describe('imports', function() {
    it('should re-export imported vars', function() {
      expect(Baz).toBe('BAZ');
    });

    it('should work', function() {
      expect(Foo).toBe('FOO');
      expect(Bar).toBe('BAR');
      // TODO: Does not work
      // assert(F == 'FOO');
      expect(fooModule.Foo).toBe('FOO');
      expect(fooModule.Bar).toBe('BAR');

      expect(exportModule.Foo).toBe('FOO');
      expect(exportModule.Bar).toBe('BAR');
      expect(exportModule.Bar1).toBe('BAR1');
      expect(exportModule.Bar2).toBe('BAR2');

      // Make sure Bar3 is not re-exported.
      expect(function() {
          exportModule.Bar3();
        }).toThrowError(IS_DARTIUM ?
          // Dart
          "No top-level method 'exportModule.Bar3' declared.":
          // JavaScript
          new RegExp('.*is not a function')
        );

      expect(Type).toBeTruthy();
    });
  });
}
