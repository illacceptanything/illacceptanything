console.warn('Make sure you have: npm install benchmark -g');

var Benchmark = require('benchmark'),
    YUI = require('yui').YUI;

var Y = YUI({
    useSync: true,
    modules: {
        'model-base': {
            fullpath: './model-base.common.js'
        },
        'model-vanilla': {
            fullpath: '../../lib/app/autoload/model-vanilla.common.js'
        }
    }
}).use('model', 'model-base', 'model-vanilla');

var data = { miami: 'florida' };

// warm up process
console.log((new Y.Model.Base(data)).toJSON());

console.log((new Y.Model.Vanilla(data)).toJSON());

console.log((new Y.Model(data)).toJSON());

var suite = Benchmark.Suite('Models');

suite.on('start', function () {
    console.log('Starting benchmarks.');
});

suite.on('cycle', function (event) {
    if (!event.target.error) {
        console.log(String(event.target));
    }
});

suite.on('error', function (event) {
    console.error(String(event.target) + String(event.target.error));
});

suite.on('complete', function (event) {
    console.warn('Fastest is ' + this.filter('fastest').pluck('name'));
});

// add tests
suite.add('new Y.Model.Vanilla()', function() {

        (new Y.Model.Vanilla(data)).toJSON();

    })
    .add('new Y.Model.Base()', function() {

        (new Y.Model.Base(data)).toJSON();

    })
    .add('new Y.Model()', function() {

        (new Y.Model(data)).toJSON();

    })
    // run async
    .run({ 'async': true });
