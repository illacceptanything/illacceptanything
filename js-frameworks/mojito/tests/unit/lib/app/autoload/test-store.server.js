/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */
YUI().use(
    'oop',
    'mojito-test-extra',
    'mojito-resource-store',
    'addon-rs-config',
    'addon-rs-selector',
    'addon-rs-url',
    'addon-rs-yui',
    'test',
    function(Y) {

        var suite = new Y.Test.Suite('mojito-store-server-tests'),
            libpath = require('path'),
            mojitoRoot = libpath.join(__dirname, '../../../lib'),
            store,
            Mock = Y.Mock,
            A = Y.Assert,
            AA = Y.ArrayAssert,
            OA = Y.ObjectAssert;


        suite.add(new Y.Test.Case({

            name: 'Store tests -- preload fixture "store"',

            init: function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/store');
                store = new Y.mojito.ResourceStore({ root: fixtures });
                store.preload();
            },

            'pre load': function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/store');
                //Y.log(Y.JSON.stringify(store,null,4));
                A.isTrue(store._config.root === fixtures);
            },

            'store is not lazy by default': function () {
                A.isFalse(Object.keys(store._mojitDetailsCache).length === 0);
            },

            'valid context': function() {
                var success;

                try {
                    store.validateContext({});
                } catch(e) {
                    A.fail('{} should be valid');
                }

                try {
                    store.validateContext({device:'iphone'});
                } catch(e) {
                    A.fail('{device:iphone} should be valid');
                }

                try {
                    store.validateContext({device:'iphone',lang:'en'});
                } catch(e) {
                    A.fail('{device:iphone,lang:en} should be valid');
                }

                try {
                    store.validateContext({device:'iphone',runtime:'common'});
                } catch(e) {
                    A.fail('{device:iphone,runtime:common} should be valid');
                }

                try {
                    success = undefined;
                    store.validateContext({device:'blender'});
                    success = true;
                } catch(e) {
                    success = false;
                }
                A.isFalse(success, '{device:blender} should be invalid');

                try {
                    success = undefined;
                    store.validateContext({device:'iphone',texture:'corrugated'});
                    success = true;
                } catch(e) {
                    success = false;
                }
                A.isFalse(success, '{device:iphone,texture:corrugated} should be invalid');

                try {
                    success = undefined;
                    store.validateContext({device:'iphone',runtime:'kite'});
                    success = true;
                } catch(e) {
                    success = false;
                }
                A.isFalse(success, '{device:iphone,runtime:kite} should be invalid');
            },

            'server app config value': function() {
                var config = store.getAppConfig(null);
                A.isTrue(config.testKey1 === 'testVal1');
            },

            'server mojit config value': function() {
                var instance = {base:'test1'};
                store.expandInstance(instance, {}, function(err, instance){
                    A.isNull(err);
                    A.isTrue(instance.id === 'test1', 'wrong ID');
                    A.isTrue(instance.type === 'test_mojit_1', 'wrong type');
                    A.isTrue(instance.config.testKey4 === 'testVal4', 'missing key from definition.json');
                });
            },

            'server mojit config value via type': function() {
                var instance = {type:'test_mojit_1'};
                store.expandInstance(instance, {}, function(err, instance){
                    A.isTrue(instance.type === 'test_mojit_1', 'wrong ID');
                    A.isTrue(instance.config.testKey4 === 'testVal4', 'missing config from definition.json');
                    A.isTrue(instance.config.testKey6.testKey7 === 'testVal7', 'missing deep config from definition.json');
                });
            },

            'server mojit config value via type and override': function() {
                var instance = {
                    type:'test_mojit_1',
                    config:{testKey4: 'other'}
                };
                store.expandInstance(instance, {}, function(err, instance){
                    A.isTrue(instance.type === 'test_mojit_1', 'wrong ID');
                    A.areSame('other', instance.config.testKey4, 'missing config from definition.json');
                    A.areSame('testVal5', instance.config.testKey5, 'missing deep config from defaults.json');
                });
            },

            'server mojit instance assets': function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/store');
                var instance = {type:'test_mojit_1'};
                store.expandInstance(instance, {}, function(err, instance) {
                    // we'll skip the favicon.ico that ships with Mojito
                    // (it's not availble when running --coverage anyway)
                    A.areSame(libpath.join(fixtures, 'mojits/test_mojit_1/assets/css/main.css'), instance.assets['css/main.css']);
                    A.areSame(libpath.join(fixtures, 'mojits/test_mojit_1/assets/js/main.js'), instance.assets['js/main.js']);
                });
            },

            'server mojit instance views and binders': function() {
                var instance = {type:'test_mojit_1'};
                store.expandInstanceForEnv('client', instance, {}, function(err, instance) {
                    A.areSame(4, Y.Object.keys(instance.views).length);

                    A.isObject(instance.views['test_1']);
                    A.areSame('/static/test_mojit_1/views/test_1.hb.html', instance.views['test_1']['content-path']);
                    A.areSame('hb', instance.views['test_1']['engine']);

                    A.areSame('test_mojit_1Bindertest_1', instance.binders['test_1']);
                    A.areSame('test_mojit_1Bindersubdir/test_1', instance.binders['subdir/test_1']);

                    A.isObject(instance.views['test_1']);
                    A.areSame('/static/test_mojit_1/views/test_1.hb.html', instance.views['test_1']['content-path']);
                    A.areSame('hb', instance.views['test_1']['engine']);

                    A.isObject(instance.views['test_2']);
                    A.areSame('/static/test_mojit_1/views/test_2.hb.html', instance.views['test_2']['content-path']);
                    A.areSame('hb', instance.views['test_2']['engine']);

                    A.isObject(instance.views['subdir/test_1']);
                    A.areSame('/static/test_mojit_1/views/subdir/test_1.hb.html', instance.views['subdir/test_1']['content-path']);
                    A.areSame('hb', instance.views['subdir/test_1']['engine']);

                    A.isObject(instance.partials['test_3']);
                    A.areSame('/static/test_mojit_1/views/partials/test_3.hb.html', instance.partials['test_3']['content-path']);
                    A.areSame('hb', instance.partials['test_3']['engine']);
                });
            },

            'server mojit instance models': function() {
                var instance = {type:'test_mojit_1'};
                store.expandInstance(instance, {}, function(err, instance) {
                    A.areSame(4, Y.Object.keys(instance.models).length);
                    A.areSame('ModelFlickr', instance.models['flickr']);
                    A.areSame('test_applevelModel', instance.models['test_applevel']);
                    A.areSame('test_mojit_1_model_test_1', instance.models['test_1']);
                    A.areSame('test_mojit_1_model_test_2', instance.models['test_2']);
                });
            },

            'server mojit type name can come from package.json': function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/store');
                var instance = {type:'TestMojit2'};
                store.expandInstance(instance, {}, function(err, instance){
                    A.isNotUndefined(instance.controller);
                    A.areSame('TestMojit2', instance.type);
                    A.areSame(libpath.join(fixtures, 'mojits/test_mojit_2/views/index.hb.html'), instance.views.index['content-path']);
                });
            },

            'server mojit is NOT loaded because of package mojito version mismatch': function(){
                var urls = store.getAllURLs();
                A.isUndefined(urls['/static/test_mojit_4/package.json']);
                A.isUndefined(urls['/static/TestMojit4/package.json']);
            },

            'server mojit is loaded because of package mojito version match': function(){
                var instance = {type:'TestMojit2'};
                store.expandInstance(instance, {}, function(err, instance){
                    A.areSame('TestMojit2', instance.type);
                });
            },

            'server a mojits package.json file is available as appropriate': function() {
                var urls = store.getAllURLs();
                A.isUndefined(urls['/static/TestMojit2/package.json']);
                A.isNotUndefined(urls['/static/TestMojit3/package.json']);
                A.isUndefined(urls['/static/TestMojit5/package.json']);
            },

            'server mojit view index.hb.html is loaded correctly': function() {
                var instance = {type:'TestMojit3'};
                store.expandInstance(instance, {}, function(err, instance){
                    A.areSame('index.hb.html', instance.views.index['content-path'].split(libpath.sep).pop());
                });
            },

            'server mojit view index.iphone.hb.html is loaded correctly': function(){
                var instance = {type:'TestMojit3'};
                store.expandInstance(instance, {device:'iphone'}, function(err, instance){
                    A.areSame('index.iphone.hb.html', instance.views.index['content-path'].split(libpath.sep).pop());
                });
            },

            'app-level mojits': function() {
                var instance = { type: 'test_mojit_1' };
                store.expandInstance(instance, {}, function(err, instance) {
                    A.isNotUndefined(instance.models.test_applevel);
                });
            },

            'mojitDirs setting': function() {
                var instance = { type: 'soloMojit' };
                store.expandInstance(instance, {}, function(err, instance) {
                    A.areSame('soloMojit', instance.controller);
                });
            },

            'getMojitTypeDetails caching': function() {
                var key = Y.JSON.stringify(['server', ['*'], 'en', 'x']);
                store._getMojitTypeDetailsCache[key] = { x: 'y' };
                var details = store.getMojitTypeDetails('server', {lang: 'en'}, 'x');
                A.isObject(details);
                A.areEqual(1, Object.keys(details).length);
                A.areEqual('y', details.x);
            },

            'expandInstanceForEnv preserves instanceId': function() {
                var inInstance = {
                    type: 'test_mojit_1',
                    instanceId: 'foo'
                };
                store.expandInstanceForEnv('server', inInstance, {}, function(err, outInstance) {
                    A.areSame('foo', outInstance.instanceId);
                });
            },

            'multi preload': function() {
                var pre = {
                    appRVs: Y.clone(store._appRVs, true),
                    mojitRVs: Y.clone(store._mojitRVs, true),
                    appResources: Y.clone(store._appResources, true),
                    mojitResources: Y.clone(store._mojitResources, true)
                };

                store.preload();
                var post = {
                    appRVs: Y.clone(store._appRVs, true),
                    mojitRVs: Y.clone(store._mojitRVs, true),
                    appResources: Y.clone(store._appResources, true),
                    mojitResources: Y.clone(store._mojitResources, true)
                };

                Y.TEST_CMP(post, pre);
            },

            'instance with base pointing to non-existant spec': function() {
                var spec = { base: 'nonexistant' };
                store.expandInstance(spec, {}, function(err, instance) {
                    A.isNotUndefined(err);
                    A.areSame('Unknown base "nonexistant". You should have configured "nonexistant" in application.json under specs or used "@nonexistant" if you wanted to specify a mojit name.', err.message);
                    A.isUndefined(instance);
                });
            },

            'instance with default spec': function() {
                // should use tests/fixtures/store/mojits/test_mojit_2/specs/default.json
                var spec = { base: 'TestMojit2' };
                store.expandInstance(spec, {}, function(err, instance) {
                    A.areSame('testVal1', instance.config.testKey1);
                });
            },

            'getAppConfig() returns contextualized info': function() {
                var context = { runtime: 'server' },
                    config;
                config = store.getAppConfig(context);
                A.isObject(config);
                A.areSame('testVal1-server', config.testKey1, 'testKey1 wasnt contextualized to the server');
                A.areSame('testVal2', config.testKey2, 'testKey2 gotten from the wrong context');
                A.areSame('portended', config.pathos, 'missing contextualized config');
                A.isUndefined(config.testKey4, 'testKey4 gotten from the wrong context');
            },

            'call getRoutes()': function() {
                var routes = store.getRoutes({});
                A.isObject(routes, 'no routes at all');
                A.isObject(routes.flickr_by_page, 'missing route flickr_by_page');
                A.isObject(routes.flickr_base, 'missing route flickr_base');
            },

            'call listAllMojits()': function() {
                var list = store.listAllMojits('server');
                A.areSame(11, list.length, 'found the wrong number of mojits');
                AA.contains('TunnelProxy', list);
                AA.contains('HTMLFrameMojit', list);
                AA.contains('LazyLoad', list);
                AA.contains('inlinecss', list);
                AA.contains('rollups', list);
                AA.contains('test_mojit_1', list);
                AA.contains('TestMojit2', list);
                AA.contains('TestMojit3', list);
                AA.contains('TestMojit5', list);
                AA.contains('soloMojit', list);
                AA.contains('page', list);
            },

            // TODO -- do we still need rollups?
            'ignore: app with rollups': function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/store');
                var spec = { type: 'rollups' };
                store.expandInstanceForEnv('client', spec, {}, function(err, instance) {
                    A.areSame('/static/rollups/rollup.client.js', instance.yui.config.modules['rollups'].fullpath, 'main rollup');
                    var urls = store.getAllURLs();
                    A.areSame(libpath.join(fixtures, 'mojits/rollups/rollup.client.js'), urls['/static/rollups/rollup.client.js']);
                });
            },

            'app resource overrides framework resource': function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/store'),
                    details = store.getMojitTypeDetails('server', {}, 'HTMLFrameMojit');
                A.areSame(libpath.join(fixtures, 'mojits/HTMLFrameMojit'), details.fullPath);
            },

            'ignore: getAllURLResources()': function() {
                // TODO
            },

            'ignore: makeResourceVersions()': function() {
                // TODO
            },

            'ignore: getResourceContent()': function() {
                // TODO
            },

            'ignore: processResourceContent()': function() {
                // TODO
            },

            'ignore: getAppPkgMeta()': function() {
                // TODO
            },

            'ignore: makeResourceFSMeta()': function() {
                // TODO
            }

        }));

        suite.add(new Y.Test.Case({

            name: 'Store tests -- preload fixture "gsg5"',

            init: function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/gsg5');
                store = new Y.mojito.ResourceStore({ root: fixtures });
                store.preload();
            },

            'controller with selector': function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/gsg5');
                var spec = { type: 'PagedFlickr' };
                var ctx = { device: 'iphone' };
                store.expandInstance(spec, ctx, function(err, instance) {
                    A.areSame('PagedFlickr', instance.controller);
                });
            },

            'binder with selector': function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/gsg5');
                var spec = { type: 'PagedFlickr' };
                var ctx = { device: 'iphone' };
                store.expandInstance(spec, ctx, function(err, instance) {
                    A.areSame(libpath.join(fixtures, 'mojits/PagedFlickr/views/index.iphone.hb.html'), instance.views.index['content-path']);
                });
            }

        }));


        suite.add(new Y.Test.Case({

            name: 'Store tests -- preload fixture "gsg5-appConfig"',

            init: function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/gsg5-appConfig');
                store = new Y.mojito.ResourceStore({ root: fixtures });
                store.preload();
            },

            'appConfig staticHandling.prefix': function() {
                var spec = { type: 'PagedFlickr' };
                store.expandInstanceForEnv('client', spec, {}, function(err, instance) {
                    A.areSame('/static/PagedFlickr/assets', instance.assetsRoot);
                });
            }

        }));

        suite.add(new Y.Test.Case({
            name: 'Store tests -- preload fixture "lazy-resolve"',

            init: function () {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/lazy-resolve');
                store = new Y.mojito.ResourceStore({ root: fixtures });
                store.preload();
            },

            'store is actually lazy': function () {
                A.isTrue(Object.keys(store._mojitDetailsCache).length === 0);
            }
        }));


        suite.add(new Y.Test.Case({

            name: 'Store tests -- misc',

            'static context is really static': function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/store'),
                    context = { runtime: 'server' },
                    store = new Y.mojito.ResourceStore({ root: fixtures, context: context }),
                    config;
                store.preload();
                config = store.getAppConfig();
                A.isObject(config);
                A.areSame('testVal1-server', config.testKey1, 'testKey1 wasnt contextualized to the server');
                A.areSame('testVal2', config.testKey2, 'testKey2 gotten from the wrong context');
                A.areSame('portended', config.pathos, 'missing contextualized config');
                A.isUndefined(config.testKey4, 'testKey4 gotten from the wrong context');
            },

            'pre load no application.json file': function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/store_no_app_config'),
                    store = new Y.mojito.ResourceStore({ root: fixtures });
                store.preload();

                //Y.log(Y.JSON.stringify(store,null,4));
                A.isTrue(store._config.root === fixtures);
            },

            'default routes': function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/store_no_app_config'),
                    store = new Y.mojito.ResourceStore({ root: fixtures });
                store.preload();

                var have = store.getRoutes();
                A.isObject(have._default_path);
            },

            'bad files': function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/badfiles'),
                    store = new Y.mojito.ResourceStore({ root: fixtures });
                store.preload();
                var spec = { type: 'M' };
                store.expandInstance(spec, {}, function(err, instance) {
                    A.isUndefined(instance.models['MModelNot']);
                    A.isUndefined(instance.binders.not);
                });
            },

            'sortedReaddirSync() sorts the result of fs.readdirSync()': function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/store');
                var mockfs = Mock();

                Mock.expect(mockfs, {
                    method: 'readdirSync',
                    args: ['dir'],
                    returns: ['d', 'c', 'a', 'b']
                });

                var store = new Y.mojito.ResourceStore({ root: fixtures });
                store._mockLib('fs', mockfs);
                var files = store._sortedReaddirSync('dir');

                AA.itemsAreSame(['a', 'b', 'c', 'd'], files);
                Mock.verify(mockfs);
            },

            '_skipBadPath() does just that': function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/store');
                var store = new Y.mojito.ResourceStore({ root: fixtures });
                A.isTrue(store._skipBadPath({ isFile: true, ext: '.js~' }), 'need to skip bad file naems');
                A.isFalse(store._skipBadPath({ isFile: false, ext: '.js~' }), 'need to not-skip bad directory names');
                A.isFalse(store._skipBadPath({ isFile: true, ext: '.js' }), 'need to not-skip good file names');
            },

            'load node_modules': function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/packages'),
                    store = new Y.mojito.ResourceStore({ root: fixtures });

                if (!store._mojitRVs.a && !store._mojitRVs.aa && !store._mojitRVs.ba) {
                    // This happens when mojito is installed via npm, since npm
                    // won't install the node_modules/ directories in
                    // tests/fixtures/packages.
                    A.isTrue(true);
                    return;
                }
                var config = store.yui.getConfigShared('server');
                A.isObject(config.modules.b, 'b');
                A.isObject(config.modules.ab, 'ab');
                A.isObject(config.modules.bb, 'bb');
                A.isObject(config.modules.cb, 'cb');

                var details = store.getMojitTypeDetails('server', {}, 'a');
                A.areSame('a', details.controller);
            },


            'skip loaded packages': function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/packages'),
                    store = new Y.mojito.ResourceStore({ root: fixtures });
                store.preload();
                var oldlog = Y.log;
                var logged = false;
                Y.log = function(msg, lvl, src) {
                    if ('debug' === lvl && 'mojito-resource-store' === src && msg.match(/^skipping duplicate package a/)) {
                        logged = true;
                    }
                };
                try {
                    store.preload();
                } finally {
                    Y.log = oldlog;
                }
                A.isTrue(logged, 'info logged');
            },

            'find and parse resources by convention': function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/conventions'),
                    store = new Y.mojito.ResourceStore({ root: fixtures });

                // fake out some parts of preload(), which we're trying to avoid
                store._fwConfig = store.config.readConfigSimple(libpath.join(mojitoRoot, 'config.json'));
                store._appConfigStatic = store.getStaticAppConfig();

                var dir = libpath.join(__dirname, '../../../../fixtures/conventions');
                var pkg = { name: 'test', version: '6.6.6' };
                var mojitType = 'testing';
                var ress = store._findResourcesByConvention(dir, 'app', pkg, mojitType);

                var r, res;
                for (r = 0; r < ress.length; r++) {
                    res = ress[r];
                    A.isNotUndefined(res.id, 'no resource id');
                    switch (res.id) {
                        case 'action--x':
                            A.areSame(pkg, res.source.pkg);
                            A.areSame('action', res.type);
                            A.areSame('x', res.name);
                            switch (res.source.fs.basename) {
                                case 'x.common':
                                    A.areSame('*', res.selector);
                                    A.areSame('common', res.affinity);
                                    A.areSame('.js', res.source.fs.ext);
                                    A.areSame('x', res.name);
                                    break;
                                case 'x.common.iphone':
                                    A.areSame('iphone', res.selector);
                                    A.areSame('common', res.affinity);
                                    A.areSame('.js', res.source.fs.ext);
                                    A.areSame('x', res.name);
                                    break;
                                default:
                                    A.fail('unknown resource ' + res.source.fs.fullPath);
                                    break;
                            }
                            break;
                        case 'action--y/z':
                            A.areSame(pkg, res.source.pkg);
                            A.areSame('action', res.type);
                            A.areSame('y/z', res.name);
                            A.areSame('*', res.selector);
                            A.areSame('common', res.affinity);
                            A.areSame('.js', res.source.fs.ext);
                            A.areSame('z.common', res.source.fs.basename);
                            break;
                        case 'addon-a-x':
                            A.areSame(pkg, res.source.pkg);
                            A.areSame('addon', res.type);
                            A.areSame('a', res.subtype);
                            A.areSame('x', res.name);
                            switch (res.source.fs.basename) {
                                case 'x.common':
                                    A.areSame('*', res.selector);
                                    A.areSame('common', res.affinity);
                                    A.areSame('.js', res.source.fs.ext);
                                    A.areSame('x', res.name);
                                    break;
                                case 'x.common.iphone':
                                    A.areSame('iphone', res.selector);
                                    A.areSame('common', res.affinity);
                                    A.areSame('.js', res.source.fs.ext);
                                    A.areSame('x', res.name);
                                    break;
                                default:
                                    A.fail('unknown resource ' + res.source.fs.fullPath);
                                    break;
                            }
                            break;
                        case 'archetype-x-y':
                            A.areSame(pkg, res.source.pkg);
                            A.areSame('archetype', res.type);
                            A.areSame('x', res.subtype);
                            A.areSame('y', res.name);
                            A.areSame('y', res.source.fs.basename);
                            break;
                        case 'asset-css-x':
                            A.areSame(pkg, res.source.pkg);
                            A.areSame('asset', res.type);
                            A.areSame('css', res.subtype);
                            A.areSame('x', res.name);
                            switch (res.source.fs.basename) {
                                case 'x':
                                    A.areSame('*', res.selector);
                                    A.areSame('common', res.affinity);
                                    A.areSame('.css', res.source.fs.ext);
                                    break;
                                case 'x.iphone':
                                    A.areSame('iphone', res.selector);
                                    A.areSame('common', res.affinity);
                                    A.areSame('.css', res.source.fs.ext);
                                    break;
                                default:
                                    A.fail('unknown resource ' + res.source.fs.fullPath);
                                    break;
                            }
                            break;
                        case 'asset-css-y/z':
                            A.areSame(pkg, res.source.pkg);
                            A.areSame('asset', res.type);
                            A.areSame('css', res.subtype);
                            A.areSame('y/z', res.name);
                            switch (res.source.fs.basename) {
                                case 'z':
                                    A.areSame('*', res.selector);
                                    A.areSame('common', res.affinity);
                                    A.areSame('.css', res.source.fs.ext);
                                    break;
                                case 'z.android':
                                    A.areSame('android', res.selector);
                                    A.areSame('common', res.affinity);
                                    A.areSame('.css', res.source.fs.ext);
                                    break;
                                default:
                                    A.fail('unknown resource ' + res.source.fs.fullPath);
                                    break;
                            }
                            break;
                        case 'binder--x':
                            A.areSame(pkg, res.source.pkg);
                            A.areSame('binder', res.type);
                            A.areSame('x', res.name);
                            switch (res.source.fs.basename) {
                                case 'x':
                                    A.areSame('*', res.selector);
                                    A.areSame('client', res.affinity);
                                    A.areSame('.js', res.source.fs.ext);
                                    break;
                                case 'x.iphone':
                                    A.areSame('iphone', res.selector);
                                    A.areSame('client', res.affinity);
                                    A.areSame('.js', res.source.fs.ext);
                                    break;
                                default:
                                    A.fail('unknown resource ' + res.source.fs.fullPath);
                                    break;
                            }
                            break;
                        case 'command--x':
                            A.areSame(pkg, res.source.pkg);
                            A.areSame('command', res.type);
                            A.areSame('x', res.name);
                            A.areSame('x', res.source.fs.basename);
                            break;
                        case 'config--config':
                            A.areSame(pkg, res.source.pkg);
                            A.areSame('config', res.type);
                            A.areSame('config', res.name);
                            A.areSame('config', res.source.fs.basename);
                            A.areSame('.json', res.source.fs.ext);
                            break;
                        case 'controller--controller':
                            A.areSame(pkg, res.source.pkg);
                            A.areSame('controller', res.type);
                            A.areSame('controller', res.name);
                            switch (res.source.fs.basename) {
                                case 'controller.common':
                                    A.areSame('*', res.selector);
                                    A.areSame('common', res.affinity);
                                    A.areSame('.js', res.source.fs.ext);
                                    break;
                                case 'controller.server.iphone':
                                    A.areSame('iphone', res.selector);
                                    A.areSame('server', res.affinity);
                                    A.areSame('.js', res.source.fs.ext);
                                    break;
                                default:
                                    A.fail('unknown resource ' + res.source.fs.fullPath);
                                    break;
                            }
                            break;
                        case 'middleware--x':
                            A.areSame(pkg, res.source.pkg);
                            A.areSame('middleware', res.type);
                            A.areSame('x', res.name);
                            A.areSame('x', res.source.fs.basename);
                            A.areSame('.js', res.source.fs.ext);
                            break;
                        case 'spec--default':
                            A.areSame(pkg, res.source.pkg);
                            A.areSame('spec', res.type);
                            A.areSame('default', res.name);
                            A.areSame('default', res.source.fs.basename);
                            A.areSame('.json', res.source.fs.ext);
                            break;
                        case 'spec--x':
                            A.areSame(pkg, res.source.pkg);
                            A.areSame('spec', res.type);
                            A.areSame('testing', res.mojit);
                            A.areSame('x', res.name);
                            A.areSame('x', res.source.fs.basename);
                            A.areSame('.json', res.source.fs.ext);
                            break;
                        case 'view--x':
                            A.areSame(pkg, res.source.pkg);
                            A.areSame('view', res.type);
                            A.areSame('x', res.name);
                            A.areSame('html', res.view.outputFormat);
                            A.areSame('hb', res.view.engine);
                            switch (res.source.fs.basename) {
                                case 'x.hb':
                                    A.areSame('*', res.selector);
                                    A.areSame('common', res.affinity);
                                    A.areSame('.html', res.source.fs.ext);
                                    break;
                                case 'x.iphone.hb':
                                    A.areSame('iphone', res.selector);
                                    A.areSame('common', res.affinity);
                                    A.areSame('.html', res.source.fs.ext);
                                    break;
                                default:
                                    A.fail('unknown resource ' + res.source.fs.fullPath);
                                    break;
                            }
                            break;
                        default:
                            A.fail('unknown resource ' + res.id);
                            break;
                    }
                }
                A.areSame(21, ress.length, 'wrong number of resources');
            }

        }));

        suite.add(new Y.Test.Case({

            name: 'Store tests -- "bleeding"',

            init: function() {
                var fixtures = libpath.join(__dirname, '../../../../fixtures/store');

                store = new Y.mojito.ResourceStore({ root: fixtures });
                store.preload();
            },

            'test bleeding spec with no config': function() {
                var instance = { type: "page" },
                    ctx = {};

                store.expandInstanceForEnv('server', instance, ctx, function(err, expanded) {
                    A.isNotUndefined(expanded, 'expanded should not be undefined');
                    OA.areEqual({}, expanded.config, 'config should be empty');
                });
            },

            'test bleeding spec with config': function() {
                var instance,
                    ctx = {};

                instance = {
                    type: "page",
                    config: {
                        children: {
                            weather: { type: "weather", action: "index" },
                            stream: { type: "stream", action: "index" }
                        }
                    }
                };
                store.expandInstanceForEnv('server', instance, ctx, function(err, expanded) {

                    A.isNotUndefined(expanded, 'expanded should not be undefined');
                    OA.areEqual(instance.config.children.weather,
                                expanded.config.children.weather,
                                'config missing children.weather');
                    OA.areEqual(instance.config.children.stream,
                                expanded.config.children.stream,
                                'config missing children.stream');
                });
            },

            'test bleeding spec with mixed config': function() {
                var instance1,
                    instance2,
                    ctx = {};

                instance1 = {
                    type: "page",
                    config: {
                        children: {
                            weather: { type: "weather", action: "index" },
                            stream: { type: "stream", action: "index" }
                        }
                    }
                };
                instance2 = {
                    type: "page"
                };

                store.expandInstanceForEnv('server', instance1, ctx, function(err, expanded1) {

                    // test 1
                    A.isNotUndefined(expanded1, 'expanded1 should not be undefined');
                    OA.areEqual(instance1.config.children.weather,
                                expanded1.config.children.weather,
                                'config missing children.weather');
                    OA.areEqual(instance1.config.children.stream,
                                expanded1.config.children.stream,
                                'config missing children.stream');

                    // test 2
                    store.expandInstanceForEnv('server', instance2, ctx, function(err, expanded2) {
                        A.isNotUndefined(expanded2, 'expanded2 should not be undefined');
                        OA.areEqual({}, expanded2.config, 'expanded2 instance config should be empty!');
                    });
                });
            }
        }));

    Y.Test.Runner.add(suite);

});
