import {
  afterEach,
  AsyncTestCompleter,
  beforeEach,
  ddescribe,
  describe,
  expect,
  iit,
  inject,
  it,
  xit,
} from 'angular2/test_lib';

import { List, ListWrapper, StringMapWrapper } from 'angular2/src/facade/collection';
import { PromiseWrapper, Promise } from 'angular2/src/facade/async';
import { isPresent, isBlank } from 'angular2/src/facade/lang';

import {
  Metric, PerflogMetric, WebDriverExtension,
  PerfLogFeatures,
  bind, Injector, Options
} from 'benchpress/common';

import { TraceEventFactory } from '../trace_event_factory';

export function main() {
  var commandLog;
  var eventFactory = new TraceEventFactory('timeline', 'pid0');

  function createMetric(perfLogs, microMetrics = null, perfLogFeatures = null) {
    commandLog = [];
    if (isBlank(perfLogFeatures)) {
      perfLogFeatures = new PerfLogFeatures({render: true, gc: true});
    }
    if (isBlank(microMetrics)) {
      microMetrics = StringMapWrapper.create();
    }
    var bindings = [
      Options.DEFAULT_BINDINGS,
      PerflogMetric.BINDINGS,
      bind(Options.MICRO_METRICS).toValue(microMetrics),
      bind(PerflogMetric.SET_TIMEOUT).toValue( (fn, millis) => {
        ListWrapper.push(commandLog, ['setTimeout', millis]);
        fn();
      }),
      bind(WebDriverExtension).toValue(new MockDriverExtension(perfLogs, commandLog, perfLogFeatures))
    ];
    return new Injector(bindings).get(PerflogMetric);
  }

  describe('perflog metric', () => {

    it('should describe itself based on the perfLogFeatrues', () => {
      expect(createMetric([[]], null, new PerfLogFeatures()).describe()).toEqual({
        'scriptTime': 'script execution time in ms, including gc and render',
        'pureScriptTime': 'script execution time in ms, without gc nor render'
      });

      expect(createMetric([[]], null, new PerfLogFeatures({
        render: true,
        gc: false
      })).describe()).toEqual({
        'scriptTime': 'script execution time in ms, including gc and render',
        'pureScriptTime': 'script execution time in ms, without gc nor render',
        'renderTime': 'render time in and ouside of script in ms',
      });

      expect(createMetric([[]]).describe()).toEqual({
        'scriptTime': 'script execution time in ms, including gc and render',
        'pureScriptTime': 'script execution time in ms, without gc nor render',
        'renderTime': 'render time in and ouside of script in ms',
        'gcTime': 'gc time in and ouside of script in ms',
        'gcAmount': 'gc amount in kbytes',
        'majorGcTime': 'time of major gcs in ms'
      });
    });

    it('should describe itself based on micro metrics', () => {
      var description = createMetric([[]], {
        'myMicroMetric': 'someDesc'
      }).describe();
      expect(description['myMicroMetric']).toEqual('someDesc');
    });

    describe('beginMeasure', () => {

      it('should mark the timeline', inject([AsyncTestCompleter], (async) => {
        var metric = createMetric([[]]);
        metric.beginMeasure().then((_) => {
          expect(commandLog).toEqual([['timeBegin', 'benchpress0']]);

          async.done();
        });
      }));

    });

    describe('endMeasure', () => {

      it('should mark and aggregate events in between the marks', inject([AsyncTestCompleter], (async) => {
        var events = [
          [
            eventFactory.markStart('benchpress0', 0),
            eventFactory.start('script', 4),
            eventFactory.end('script', 6),
            eventFactory.markEnd('benchpress0', 10)
          ]
        ];
        var metric = createMetric(events);
        metric.beginMeasure()
          .then( (_) => metric.endMeasure(false) )
          .then( (data) => {
            expect(commandLog).toEqual([
              ['timeBegin', 'benchpress0'],
              ['timeEnd', 'benchpress0', null],
              'readPerfLog'
            ]);
            expect(data['scriptTime']).toBe(2);

            async.done();
        });
      }));

      it('should restart timing', inject([AsyncTestCompleter], (async) => {
        var events = [
          [
            eventFactory.markStart('benchpress0', 0),
            eventFactory.markEnd('benchpress0', 1),
            eventFactory.markStart('benchpress1', 2),
          ], [
            eventFactory.markEnd('benchpress1', 3)
          ]
        ];
        var metric = createMetric(events);
        metric.beginMeasure()
          .then( (_) => metric.endMeasure(true) )
          .then( (_) => metric.endMeasure(true) )
          .then( (_) => {
            expect(commandLog).toEqual([
              ['timeBegin', 'benchpress0'],
              ['timeEnd', 'benchpress0', 'benchpress1'],
              'readPerfLog',
              ['timeEnd', 'benchpress1', 'benchpress2'],
              'readPerfLog'
            ]);

            async.done();
        });
      }));

      it('should loop and aggregate until the end mark is present', inject([AsyncTestCompleter], (async) => {
        var events = [
          [ eventFactory.markStart('benchpress0', 0), eventFactory.start('script', 1) ],
          [ eventFactory.end('script', 2) ],
          [ eventFactory.start('script', 3), eventFactory.end('script', 5), eventFactory.markEnd('benchpress0', 10) ]
        ];
        var metric = createMetric(events);
        metric.beginMeasure()
          .then( (_) => metric.endMeasure(false) )
          .then( (data) => {
            expect(commandLog).toEqual([
              ['timeBegin', 'benchpress0'],
              ['timeEnd', 'benchpress0', null],
              'readPerfLog',
              [ 'setTimeout', 100 ],
              'readPerfLog',
              [ 'setTimeout', 100 ],
              'readPerfLog'
            ]);
            expect(data['scriptTime']).toBe(3);

            async.done();
        });
      }));

      it('should store events after the end mark for the next call', inject([AsyncTestCompleter], (async) => {
        var events = [
          [ eventFactory.markStart('benchpress0', 0), eventFactory.markEnd('benchpress0', 1), eventFactory.markStart('benchpress1', 1),
            eventFactory.start('script', 1), eventFactory.end('script', 2) ],
          [ eventFactory.start('script', 3), eventFactory.end('script', 5), eventFactory.markEnd('benchpress1', 6) ]
        ];
        var metric = createMetric(events);
        metric.beginMeasure()
          .then( (_) => metric.endMeasure(true) )
          .then( (data) => {
            expect(data['scriptTime']).toBe(0);
            return metric.endMeasure(true)
          })
          .then( (data) => {
            expect(commandLog).toEqual([
              ['timeBegin', 'benchpress0'],
              ['timeEnd', 'benchpress0', 'benchpress1'],
              'readPerfLog',
              ['timeEnd', 'benchpress1', 'benchpress2'],
              'readPerfLog'
            ]);
            expect(data['scriptTime']).toBe(3);

            async.done();
        });
      }));

    });

    describe('aggregation', () => {

      function aggregate(events, microMetrics = null) {
        ListWrapper.insert(events, 0, eventFactory.markStart('benchpress0', 0));
        ListWrapper.push(events, eventFactory.markEnd('benchpress0', 10));
        var metric = createMetric([events], microMetrics);
        return metric
          .beginMeasure().then( (_) => metric.endMeasure(false) );
      }


      it('should report a single interval', inject([AsyncTestCompleter], (async) => {
        aggregate([
          eventFactory.start('script', 0),
          eventFactory.end('script', 5)
        ]).then((data) => {
          expect(data['scriptTime']).toBe(5);
          async.done();
        });
      }));

      it('should sum up multiple intervals', inject([AsyncTestCompleter], (async) => {
        aggregate([
          eventFactory.start('script', 0),
          eventFactory.end('script', 5),
          eventFactory.start('script', 10),
          eventFactory.end('script', 17)
        ]).then((data) => {
          expect(data['scriptTime']).toBe(12);
          async.done();
        });
      }));

      it('should ignore not started intervals', inject([AsyncTestCompleter], (async) => {
        aggregate([
          eventFactory.end('script', 10)
        ]).then((data) => {
          expect(data['scriptTime']).toBe(0);
          async.done();
        });
      }));

      it('should ignore not ended intervals', inject([AsyncTestCompleter], (async) => {
        aggregate([
          eventFactory.start('script', 10)
        ]).then((data) => {
          expect(data['scriptTime']).toBe(0);
          async.done();
        });
      }));

      it('should ignore events from different processed as the start mark', inject([AsyncTestCompleter], (async) => {
        var otherProcessEventFactory = new TraceEventFactory('timeline', 'pid1');
        var metric = createMetric([[
          eventFactory.markStart('benchpress0', 0),
          eventFactory.start('script', 0, null),
          eventFactory.end('script', 5, null),
          otherProcessEventFactory.start('script', 10, null),
          otherProcessEventFactory.end('script', 17, null),
          eventFactory.markEnd('benchpress0', 20)
        ]]);
        metric.beginMeasure()
          .then( (_) => metric.endMeasure(false) )
          .then((data) => {
            expect(data['scriptTime']).toBe(5);
            async.done();
          });
      }));

      it('should support scriptTime metric', inject([AsyncTestCompleter], (async) => {
        aggregate([
          eventFactory.start('script', 0),
          eventFactory.end('script', 5)
        ]).then((data) => {
          expect(data['scriptTime']).toBe(5);
          async.done();
        });
      }));

      it('should support renderTime metric', inject([AsyncTestCompleter], (async) => {
        aggregate([
          eventFactory.start('render', 0),
          eventFactory.end('render', 5)
        ]).then((data) => {
          expect(data['renderTime']).toBe(5);
          async.done();
        });
      }));

      it('should support gcTime/gcAmount metric', inject([AsyncTestCompleter], (async) => {
        aggregate([
          eventFactory.start('gc', 0, {'usedHeapSize': 2500}),
          eventFactory.end('gc', 5, {'usedHeapSize': 1000})
        ]).then((data) => {
          expect(data['gcTime']).toBe(5);
          expect(data['gcAmount']).toBe(1.5);
          expect(data['majorGcTime']).toBe(0);
          async.done();
        });
      }));

      it('should support majorGcTime metric', inject([AsyncTestCompleter], (async) => {
        aggregate([
          eventFactory.start('gc', 0, {'usedHeapSize': 2500}),
          eventFactory.end('gc', 5, {'usedHeapSize': 1000, 'majorGc': true})
        ]).then((data) => {
          expect(data['gcTime']).toBe(5);
          expect(data['majorGcTime']).toBe(5);
          async.done();
        });
      }));

      it('should support pureScriptTime = scriptTime-gcTime-renderTime', inject([AsyncTestCompleter], (async) => {
        aggregate([
          eventFactory.start('script', 0),
          eventFactory.start('gc', 1, {'usedHeapSize': 1000}),
          eventFactory.end('gc', 4, {'usedHeapSize': 0}),
          eventFactory.start('render', 4),
          eventFactory.end('render', 5),
          eventFactory.end('script', 6)
        ]).then((data) => {
          expect(data['scriptTime']).toBe(6);
          expect(data['pureScriptTime']).toBe(2);
          async.done();
        });
      }));

      describe('microMetrics', () => {

        it('should report micro metrics', inject([AsyncTestCompleter], (async) => {
          aggregate([
            eventFactory.markStart('mm1', 0),
            eventFactory.markEnd('mm1', 5),
          ], {'mm1': 'micro metric 1'}).then((data) => {
            expect(data['mm1']).toBe(5.0);
            async.done();
          });
        }));

        it('should ignore micro metrics that were not specified', inject([AsyncTestCompleter], (async) => {
          aggregate([
            eventFactory.markStart('mm1', 0),
            eventFactory.markEnd('mm1', 5),
          ]).then((data) => {
            expect(data['mm1']).toBeFalsy();
            async.done();
          });
        }));

        it('should report micro metric averages', inject([AsyncTestCompleter], (async) => {
          aggregate([
            eventFactory.markStart('mm1*20', 0),
            eventFactory.markEnd('mm1*20', 5),
          ], {'mm1': 'micro metric 1'}).then((data) => {
            expect(data['mm1']).toBe(5/20);
            async.done();
          });
        }));

      });

    });

  });
}

class MockDriverExtension extends WebDriverExtension {
  _perfLogs:List;
  _commandLog:List;
  _perfLogFeatures:PerfLogFeatures;
  constructor(perfLogs, commandLog, perfLogFeatures) {
    super();
    this._perfLogs = perfLogs;
    this._commandLog = commandLog;
    this._perfLogFeatures = perfLogFeatures;
  }

  timeBegin(name):Promise {
    ListWrapper.push(this._commandLog, ['timeBegin', name]);
    return PromiseWrapper.resolve(null);
  }

  timeEnd(name, restartName):Promise {
    ListWrapper.push(this._commandLog, ['timeEnd', name, restartName]);
    return PromiseWrapper.resolve(null);
  }

  perfLogFeatures():PerfLogFeatures {
    return this._perfLogFeatures;
  }

  readPerfLog():Promise {
    ListWrapper.push(this._commandLog, 'readPerfLog');
    if (this._perfLogs.length > 0) {
      var next = this._perfLogs[0];
      ListWrapper.removeAt(this._perfLogs, 0);
      return PromiseWrapper.resolve(next);
    } else {
      return PromiseWrapper.resolve([]);
    }
  }
}
