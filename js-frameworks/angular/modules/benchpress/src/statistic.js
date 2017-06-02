import { Math } from 'angular2/src/facade/math';
import { ListWrapper } from 'angular2/src/facade/collection';

export class Statistic {
  static calculateCoefficientOfVariation(sample, mean) {
    return Statistic.calculateStandardDeviation(sample, mean) / mean * 100;
  }

  static calculateMean(sample) {
    var total = 0;
    ListWrapper.forEach(sample, (x) => { total += x } );
    return total / sample.length;
  }

  static calculateStandardDeviation(sample, mean) {
    var deviation = 0;
    ListWrapper.forEach(sample, (x) => {
      deviation += Math.pow(x - mean, 2);
    });
    deviation = deviation / (sample.length);
    deviation = Math.sqrt(deviation);
    return deviation;
  }

  static calculateRegressionSlope(xValues, xMean, yValues, yMean) {
    // See http://en.wikipedia.org/wiki/Simple_linear_regression
    var dividendSum = 0;
    var divisorSum = 0;
    for (var i=0; i<xValues.length; i++) {
      dividendSum += (xValues[i] - xMean) * (yValues[i] - yMean);
      divisorSum += Math.pow(xValues[i] - xMean, 2);
    }
    return dividendSum / divisorSum;
  }
}


