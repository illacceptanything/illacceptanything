/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */
YUI().use('addon-rs-config', 'mojito-util', 'mojito-test-extra', 'base', 'oop', 'test', function(Y) {

    var suite = new YUITest.TestSuite('mojito-addon-rs-config-tests'),
        libfs = require('fs'),
        libpath = require('path'),
        mojitoRoot = libpath.join(__dirname, '../../../../../../lib'),
        A = YUITest.Assert,
        OA = YUITest.ObjectAssert,
        AA = YUITest.ArrayAssert;


    function MockRS(config) {
        MockRS.superclass.constructor.apply(this, arguments);
    }
    MockRS.NAME = 'MockResourceStore';
    MockRS.ATTRS = {};
    Y.extend(MockRS, Y.Base, {

        initializer: function(cfg) {
            this._config = cfg || {};
        },

        validateContext: function() {
        },

        cloneObj: function(o) {
            return Y.clone(o);
        },

        getStaticContext: function() {
            return this._config.context || {};
        },

        blendStaticContext: function(ctx) {
            return Y.mojito.util.blend(this._config.context, ctx);
        },

        findResourceVersionByConvention: function(source, mojitType) {
            // no-op
        },

        parseResourceVersion: function(source, type, subtype, mojitType) {
            // no-op
        }

    });


    function readJSON(dir, file) {
        var path = libpath.join(dir, file);
        var contents = libfs.readFileSync(path, 'utf-8');
        return JSON.parse(contents);
    }


    function makeSource(dir, dirType, subdir, file, isFile) {
        var source = {
            fs: {
                fullPath: libpath.join(dir, subdir, file),
                rootDir: dir,
                rootType: dirType,
                subDir: subdir,
                subDirArray: subdir.split('/'),
                isFile: isFile,
                ext: libpath.extname(file)
            },
            pkg: {
                name: 'unittest',
                version: '999.666.999',
                depth: 999
            }
        };
        source.fs.basename = libpath.basename(file, source.fs.ext);
        return source;
    }


    suite.add(new YUITest.TestCase({

        name: 'config rs addon tests',

        'read dimensions': function() {
            // from mojito
            var fixtures = libpath.join(__dirname, '../../../../../fixtures/store');
            var store = new MockRS({ root: fixtures });
            store.plug(Y.mojito.addons.rs.config, { appRoot: fixtures, mojitoRoot: mojitoRoot } );
            var have = store.config.getDimensions();
            var want = readJSON(mojitoRoot, 'dimensions.json');
            Y.TEST_CMP(want, have);

            // app-specified
            fixtures = libpath.join(__dirname, '../../../../../fixtures/ycb');
            store = new MockRS({ root: fixtures });
            store.plug(Y.mojito.addons.rs.config, { appRoot: fixtures, mojitoRoot: mojitoRoot } );
            have = store.config.getDimensions();
            want = readJSON(fixtures, 'dimensions.json');
            Y.TEST_CMP(want, have);
        },


        'find config resources': function() {
            var fixtures = libpath.join(__dirname, '../../../../../fixtures/store');
            var store = new MockRS({ root: fixtures });
            store.plug(Y.mojito.addons.rs.config, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

            // skip non-json files
            var source = makeSource(fixtures, 'app', '.', 'server.js', true);
            var have = store.findResourceVersionByConvention(source, null);
            var want = undefined;
            Y.TEST_CMP(have, want, 'skip non-json files');

            // include all json files in the app
            source = makeSource(fixtures, 'app', '.', 'x.json', true);
            have = store.findResourceVersionByConvention(source, null);
            want = { type: 'config' };
            Y.TEST_CMP(have, want, 'include all json files in the app');

            // ... explicitly including package.json
            source = makeSource(fixtures, 'app', '.', 'package.json', true);
            have = store.findResourceVersionByConvention(source, null);
            want = { type: 'config' };
            Y.TEST_CMP(have, want, 'include package.json in the app');

            // exclude all json files in a bundle
            source = makeSource(fixtures, 'bundle', '.', 'x.json', true);
            have = store.findResourceVersionByConvention(source, null);
            want = undefined;
            Y.TEST_CMP(have, want, 'exclude all json files in a bundle');

            // ... explicitly excluding package.json
            source = makeSource(fixtures, 'bundle', '.', 'package.json', true);
            have = store.findResourceVersionByConvention(source, null);
            want = undefined;
            Y.TEST_CMP(have, want, 'exclude package.json in a bundle');

            // include all json files in a mojit
            source = makeSource(fixtures, 'mojit', '.', 'x.json', true);
            have = store.findResourceVersionByConvention(source, 'foo');
            want = { type: 'config' };
            Y.TEST_CMP(have, want, 'include all json files in a mojit');

            // ... except for the 'shared' mojit
            source = makeSource(fixtures, 'mojit', '.', 'x.json', true);
            have = store.findResourceVersionByConvention(source, 'shared');
            want = undefined;
            Y.TEST_CMP(have, want, 'exclude all json files in the "shared" mojit');

            // ... explicitly including package.json
            source = makeSource(fixtures, 'mojit', '.', 'package.json', true);
            have = store.findResourceVersionByConvention(source, 'shared');
            want = { type: 'config' };
            Y.TEST_CMP(have, want, 'include package.json in the "shared" mojit');
        },


        'parse found resource': function() {
            var fixtures = libpath.join(__dirname, '../../../../../fixtures/store');
            var store = new MockRS({ root: fixtures });
            store.plug(Y.mojito.addons.rs.config, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

            var source = makeSource(fixtures, 'app', '.', 'application.json', true);
            var res = store.parseResourceVersion(source, 'config');
            A.isNotUndefined(res);
            Y.TEST_CMP(res.source, source);
            A.areSame('config', res.type);
            A.areSame('common', res.affinity);
            A.areSame('*', res.selector);
            A.areSame('application', res.name);
            A.areSame('config--application', res.id);
            A.isUndefined(res.mojit);

            source = makeSource(fixtures, 'mojit', '.', 'defaults.json', true);
            res = store.parseResourceVersion(source, 'config', undefined, 'x');
            A.isNotUndefined(res);
            Y.TEST_CMP(res.source, source);
            A.areSame('config', res.type);
            A.areSame('common', res.affinity);
            A.areSame('*', res.selector);
            A.areSame('defaults', res.name);
            A.areSame('config--defaults', res.id);
            A.areSame('x', res.mojit);
        },


        'read YCB files': function() {
            var fixtures = libpath.join(__dirname, '../../../../../fixtures/store');
            var store = new MockRS({ root: fixtures });
            store.plug(Y.mojito.addons.rs.config, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

            var path = libpath.join(fixtures, 'application.json');
            var have = store.config.readConfigYCB(path, { runtime: 'server' });
            var want = {
                "mojitDirs": [
                    "soloMojit"
                ],

                "testKey1": "testVal1-server",
                "testKey2": "testVal2",
                "testKey3": "testVal3",
                "specs": {
                    "test1": {
                        "type": "test_mojit_1"
                    },
                    "single": {
                        "type": "HTMLFrameMojit",
                        "config": {
                            "child": {
                                "type": "page",
                                "config": {
                                    "children": {
                                        "weather": {
                                            "type": "weather",
                                            "action": "index"
                                        },
                                        "stream": {
                                            "type": "stream",
                                            "action": "stream"
                                        }
                                    }
                                }
                            }
                        }
                    },
                    "multiple": {
                        "type": "HTMLFrameMojit",
                        "config": {
                            "child": {
                                "type": "page"
                            }
                        }
                    }

                },
                "selector": "shelves",
                "pathos": "portended"
            };
            Y.TEST_CMP(have, want);
        },


        'malformed JSON for config file': function() {
            var fixtures = libpath.join(__dirname, '../../../../../fixtures/badfiles2');
            var store = new MockRS({ root: fixtures });
            store.plug(Y.mojito.addons.rs.config, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

            var path = libpath.join(fixtures, 'routes.json');
            try {
                store.config.readConfigSimple(path);
                A.fail("should throw an error");
            }
            catch (err) {
                A.areSame('Error parsing file:', err.message.substr(0, 19));
            }
        },


        'JSON config file not YCB': function() {
            var fixtures = libpath.join(__dirname, '../../../../../fixtures/badfiles3');
            var store = new MockRS({ root: fixtures });
            store.plug(Y.mojito.addons.rs.config, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

            var path = libpath.join(fixtures, 'routes.json');
            var have = store.config.readConfigYCB(path, {});
            var want = {};
            Y.TEST_CMP(have, want);
        },


        "readConfigJSON JSON file": function() {
            var fixtures = libpath.join(__dirname, '../../../../../fixtures/gsg5');
            var store = new MockRS({ root: fixtures });
            store.plug(Y.mojito.addons.rs.config, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

            var path = libpath.join(fixtures, 'package.json');
            var have = store.config.readConfigJSON(path);
            var want = readJSON(fixtures, 'package.json');
            Y.TEST_CMP(have, want);
        },


        "readConfigSimple JSON file":  function () {
            var fixtures = libpath.join(__dirname, '../../../../../fixtures'),
                store = new MockRS({ root: fixtures }),
                path = libpath.join(fixtures, "/config/", "json.json"),
                obj;
            store.plug(Y.mojito.addons.rs.config, { appRoot: fixtures, mojitoRoot: mojitoRoot } );
            obj = store.config.readConfigSimple(path);
            A.areSame("val", obj.key);
        },


        "readConfigSimple YAML file":  function () {

            var fixtures = libpath.join(__dirname, '../../../../../fixtures'),
                store = new MockRS({ root: fixtures }),
                path = libpath.join(fixtures, "/config/", "yaml.yaml"),
                obj;

            store.plug(Y.mojito.addons.rs.config, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

            obj = store.config.readConfigSimple(path);

            A.areSame("val", obj.key);
        },


        "readConfigSimple YML file":  function () {

            var fixtures = libpath.join(__dirname, '../../../../../fixtures'),
                store = new MockRS({ root: fixtures }),
                path = libpath.join(fixtures, "/config/", "yml.yml"),
                obj;

            store.plug(Y.mojito.addons.rs.config, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

            obj = store.config.readConfigSimple(path);

            A.areSame("val", obj.key);
        },


        "readConfigSimple no ext file":  function () {

            var fixtures = libpath.join(__dirname, '../../../../../fixtures'),
                store = new MockRS({ root: fixtures }),
                path = libpath.join(fixtures, "/config/", "ext"),
                obj;

            store.plug(Y.mojito.addons.rs.config, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

            obj = store.config.readConfigSimple(path);

            A.areSame("val", obj.key);
        },


        "readConfigSimple YAML file with TAB not space":  function () {

            var fixtures = libpath.join(__dirname, '../../../../../fixtures'),
                store = new MockRS({ root: fixtures }),
                path = libpath.join(fixtures, "/config/", "bad.yaml");

            store.plug(Y.mojito.addons.rs.config, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

            try {
                store.config.readConfigSimple(path);
            }
            catch (err) {
                A.areSame('Error parsing file:', err.message.substr(0, 19));
            }
        },


        'create multipart ycb': function () {
            var fixtures = libpath.join(__dirname, '../../../../../fixtures/app-jsons'),
                store = new MockRS({ root: fixtures });
            store.plug(Y.mojito.addons.rs.config, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

            var paths, ycb, config;
            paths = [
                libpath.join(fixtures, 'application.json'),
                libpath.join(fixtures, 'node_modules', 'devices', 'application.json'),
                libpath.join(fixtures, 'node_modules', 'runtimes', 'application.json')
            ];
            ycb = store.config.createMultipartYCB(paths);
            A.isObject(ycb);
            config = ycb.read({runtime: 'client'});
            A.isObject(config);
            A.areSame('testVal2-client', config.testKey2);
            config = ycb.read({device: 'android'});
            A.isObject(config);
            A.areSame('droid', config.selector);

            // detect non-ycb file
            paths = [
                libpath.join(fixtures, 'application.json'),
                libpath.join(fixtures, 'application-notycb.json')
            ];
            ycb = store.config.createMultipartYCB(paths);
            A.isUndefined(ycb);

            // detect missing settings
            paths = [
                libpath.join(fixtures, 'application.json'),
                libpath.join(fixtures, 'application-nosettings.json')
            ];
            ycb = store.config.createMultipartYCB(paths);
            A.isUndefined(ycb);

            // detect duplicate settings
            paths = [
                libpath.join(fixtures, 'application.json'),
                libpath.join(fixtures, 'application2.json')
            ];
            ycb = store.config.createMultipartYCB(paths);
            A.isObject(ycb);
            config = ycb.read({runtime: 'client'});
            A.isObject(config);
            A.areSame('testVal2-app2', config.testKey2);
            A.areSame('testVal4', config.testKey4);
        },


        'test applicationConfigFiles in _readYcbAppConfig': function () {
            var fixtures = libpath.join(__dirname, '../../../../../fixtures/app-jsons'),
                store = new MockRS({ root: fixtures });
            store.plug(Y.mojito.addons.rs.config, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

            var ycb, config;
            ycb = store.config._readYcbAppConfig();
            A.isObject(ycb);
            config = ycb.read({runtime: 'client'});
            A.isObject(config);
            A.areSame('testVal2-client', config.testKey2);
            config = ycb.read({device: 'android'});
            A.isObject(config);
            A.areSame('droid', config.selector);
        }

    }));

    Y.Test.Runner.add(suite);

});
