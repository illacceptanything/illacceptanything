library angular.core.facade.math;

import 'dart:core' show double, num;
import 'dart:math' as math;

const NaN = double.NAN;

class Math {
  static num pow(num x, num exponent) {
    return math.pow(x, exponent);
  }

  static num min(num a, num b) => math.min(a, b);

  static num floor(num a) => a.floor();

  static num ceil(num a) => a.ceil();

  static num sqrt(num x) => math.sqrt(x);
}
