import {describe, ddescribe, it, iit, xit, expect, beforeEach, afterEach} from 'angular2/test_lib';

import { isBlank, isPresent, Date, DateWrapper } from 'angular2/src/facade/lang';
import { List, ListWrapper } from 'angular2/src/facade/collection';

import {
  SampleState, Reporter, bind, Injector,
  ConsoleReporter, SampleDescription, MeasureValues
} from 'benchpress/common';

export function main() {
  describe('console reporter', () => {
    var reporter;
    var log;

    function createReporter({columnWidth, sampleId, descriptions, metrics}) {
      log = [];
      if (isBlank(descriptions)) {
        descriptions = [];
      }
      if (isBlank(sampleId)) {
        sampleId = 'null';
      }
      var bindings = [
        ConsoleReporter.BINDINGS,
        bind(SampleDescription).toValue(new SampleDescription(sampleId, descriptions, metrics)),
        bind(ConsoleReporter.PRINT).toValue((line) => ListWrapper.push(log, line))
      ];
      if (isPresent(columnWidth)) {
        ListWrapper.push(bindings, bind(ConsoleReporter.COLUMN_WIDTH).toValue(columnWidth));
      }
      reporter = new Injector(bindings).get(ConsoleReporter);
    }

    it('should print the sample id, description and table header', () => {
      createReporter({
        columnWidth: 8,
        sampleId: 'someSample',
        descriptions: [{
          'a': 1,
          'b': 2
        }],
        metrics: {
          'm1': 'some desc',
          'm2': 'some other desc'
        }
      });
      expect(log).toEqual([
        'BENCHMARK someSample',
        'Description:',
        '- a: 1',
        '- b: 2',
        'Metrics:',
        '- m1: some desc',
        '- m2: some other desc',
        '',
        '      m1 |       m2',
        '-------- | --------',
      ]);
    });

    it('should print a table row', () => {
      createReporter({
        columnWidth: 8,
        metrics: {
          'a': '',
          'b': ''
        }
      });
      log = [];
      reporter.reportMeasureValues(mv(0, 0, {
        'a': 1.23, 'b': 2
      }));
      expect(log).toEqual([
        '    1.23 |     2.00'
      ]);
    });

    it('should print the table footer and stats when there is a valid sample', () => {
      createReporter({
        columnWidth: 8,
        metrics: {
          'a': '',
          'b': ''
        }
      });
      log = [];
      reporter.reportSample([], [mv(0,0,{
        'a': 3, 'b': 6
      }), mv(1,1,{
        'a': 5, 'b': 9
      })]);
      expect(log).toEqual([
        '======== | ========',
        '4.00+-25% | 7.50+-20%'
      ]);
    });

  });
}

function mv(runIndex, time, values) {
  return new MeasureValues(runIndex, DateWrapper.fromMillis(time), values);
}
