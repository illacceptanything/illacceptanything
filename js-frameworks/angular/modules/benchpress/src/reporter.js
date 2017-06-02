import { bind } from 'angular2/di';
import {
  Promise, PromiseWrapper
} from 'angular2/src/facade/async';
import {
  ABSTRACT, BaseException
} from 'angular2/src/facade/lang';

import { MeasureValues } from './measure_values';

/**
 * A reporter reports measure values and the valid sample.
 */
@ABSTRACT()
export class Reporter {
  static bindTo(delegateToken) {
    return [
      bind(Reporter).toFactory(
        (delegate) => delegate, [delegateToken]
      )
    ];
  }

  reportMeasureValues(values:MeasureValues):Promise {
    throw new BaseException('NYI');
  }

  reportSample(completeSample:List<MeasureValues>, validSample:List<MeasureValues>):Promise {
    throw new BaseException('NYI');
  }
}
