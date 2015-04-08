import {describe, ddescribe, it, iit, xit, expect, beforeEach, afterEach} from 'angular2/test_lib';
import { Date, DateWrapper } from 'angular2/src/facade/lang';
import { ListWrapper } from 'angular2/src/facade/collection';

import {
  Validator, RegressionSlopeValidator, Injector, bind, MeasureValues
} from 'benchpress/common';

export function main() {
  describe('regression slope validator', () => {
    var validator;

    function createValidator({size, metric}) {
      validator = new Injector([
        RegressionSlopeValidator.BINDINGS,
        bind(RegressionSlopeValidator.METRIC).toValue(metric),
        bind(RegressionSlopeValidator.SAMPLE_SIZE).toValue(size)
      ]).get(RegressionSlopeValidator);
    }

    it('should return sampleSize and metric as description', () => {
      createValidator({size: 2, metric: 'script'});
      expect(validator.describe()).toEqual({
        'sampleSize': 2,
        'regressionSlopeMetric': 'script'
      });
    });

    it('should return null while the completeSample is smaller than the given size', () => {
      createValidator({size: 2, metric: 'script'});
      expect(validator.validate([])).toBe(null);
      expect(validator.validate([mv(0,0,{})])).toBe(null);
    });

    it('should return null while the regression slope is < 0', () => {
      createValidator({size: 2, metric: 'script'});
      expect(validator.validate([mv(0,0,{'script':2}), mv(1,1,{'script':1})])).toBe(null);
    });

    it('should return the last sampleSize runs when the regression slope is ==0', () => {
      createValidator({size: 2, metric: 'script'});
      var sample = [mv(0,0,{'script':1}), mv(1,1,{'script':1}), mv(2,2,{'script':1})];
      expect(validator.validate(ListWrapper.slice(sample,0,2))).toEqual(ListWrapper.slice(sample,0,2));
      expect(validator.validate(sample)).toEqual(ListWrapper.slice(sample,1,3));
    });

    it('should return the last sampleSize runs when the regression slope is >0', () => {
      createValidator({size: 2, metric: 'script'});
      var sample = [mv(0,0,{'script':1}), mv(1,1,{'script':2}), mv(2,2,{'script':3})];
      expect(validator.validate(ListWrapper.slice(sample,0,2))).toEqual(ListWrapper.slice(sample,0,2));
      expect(validator.validate(sample)).toEqual(ListWrapper.slice(sample,1,3));
    });

  });
}

function mv(runIndex, time, values) {
  return new MeasureValues(runIndex, DateWrapper.fromMillis(time), values);
}
