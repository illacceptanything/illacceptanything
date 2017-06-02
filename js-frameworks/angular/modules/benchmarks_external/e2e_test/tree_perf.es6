var perfUtil = require('angular2/src/test_lib/perf_util');

describe('ng1.x tree benchmark', function () {

  var URL = 'benchmarks_external/src/tree/tree_benchmark.html';

  afterEach(perfUtil.verifyNoBrowserErrors);

  it('should log the stats', function(done) {
    perfUtil.runClickBenchmark({
      url: URL,
      buttons: ['#destroyDom', '#createDom'],
      id: 'ng1.tree',
      params: [{
        name: 'depth', value: 9, scale: 'log2'
      }]
    }).then(done, done.fail);
  });

});
