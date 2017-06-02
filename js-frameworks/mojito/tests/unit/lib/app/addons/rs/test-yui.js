/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */
YUI().use(
    'base',
    'oop',
    'mojito-resource-store',
    'addon-rs-config',
    'addon-rs-selector',
    'addon-rs-yui',
    'json',
    'test',
    function(Y) {

    var suite = new YUITest.TestSuite('mojito-addon-rs-yui-tests'),
        libasync = require('async'),
        libpath = require('path'),
        libvm = require('vm'),
        mojitoRoot = libpath.join(__dirname, '../../../../../../lib'),
        A = Y.Assert,
        AA = Y.ArrayAssert,
        store;


    function parseConfig(config) {
        var ctx = {
            Y: {
                config: {},
                merge: Y.merge
            },
            x: undefined
        };
        config = 'x = ' + config + ';';
        libvm.runInNewContext(config, ctx, 'config');
        return ctx.x;
    }


    function MockRS(config) {
        MockRS.superclass.constructor.apply(this, arguments);
    }
    MockRS.NAME = 'MockResourceStore';
    MockRS.ATTRS = {};
    Y.extend(MockRS, Y.Base, {

        initializer: function(cfg) {
            this._config = cfg || {};
            this.RVs = {};
            this._mojitRVs = {};  // mojitType: list of resources
            this._appRVs = {};    // list of resources
            this._mojits = {};
            this.publish('getMojitTypeDetails', {emitFacade: true, preventable: false});
            this._appConfig = { yui: {} };
            this.YUI = { add: function () {}, Env: { mods: {} } };
        },

        listAllMojits: function() {
            return Object.keys(this._mojits);
        },

        getStaticAppConfig: function() {
            return Y.clone(this._appConfig, true);
        },

        getResourceVersions: function(filter) {
            var source,
                out = [],
                r,
                res,
                k,
                use;

            source = filter.mojit ? this._mojitRVs[filter.mojit] : this._appRVs;
            if (!source) {
                return [];
            }
            for (r = 0; r < source.length; r += 1) {
                res = source[r];
                use = true;
                for (k in filter) {
                    if (filter.hasOwnProperty(k)) {
                        if (res[k] !== filter[k]) {
                            use = false;
                            break;
                        }
                    }
                }
                if (use) {
                    out.push(res);
                }
            }
            return out;
        },

        getMojitResourceVersions: function (mojit) {
            return this._mojitRVs[mojit] || [];
        },

        findResourceVersionByConvention: function(source, mojitType) {
            // no-op
        },

        parseResourceVersion: function(source, type, subtype, mojitType) {
            // no-op
        },

        addResourceVersion: function(res) {
            this.RVs[[res.affinity, res.selector, res.id].join('/')] = res;
        },

        _makeResource: function(env, ctx, mojit, type, name, yuiName, pkgName) {
            if (mojit && mojit !== 'shared') {
                this._mojits[mojit] = true;
            }
            var res = {
                source: {
                    fs: {
                        fullPath: 'path/for/' + type + '--' + name + '.common.ext',
                        rootDir: 'path/for'
                    },
                    pkg: { name: (pkgName || 'testing') }
                },
                affinity: { affinity: 'common' },
                selector: '*',
                mojit: mojit,
                type: type,
                name: name,
                id: type + '--' + name
            };
            if (yuiName) {
                res.yui = { name: yuiName };
            }
            ctx = Y.JSON.stringify(ctx);
            if (mojit) {
                if (!this._mojitRVs[mojit]) {
                    this._mojitRVs[mojit] = [];
                }
                this._mojitRVs[mojit].push(res);
            } else {
                this._appRVs.push(res);
            }
        }

    });


    function cmp(x, y, msg, path) {
        if (Y.Lang.isArray(x)) {
            A.isArray(x, msg || 'first arg should be an array');
            A.isArray(y, msg || 'second arg should be an array');
            A.areSame(x.length, y.length, msg || 'arrays are different lengths');
            for (var i = 0; i < x.length; i += 1) {
                cmp(x[i], y[i], msg);
            }
            return;
        }
        if (Y.Lang.isObject(x)) {
            A.isObject(x, msg || 'first arg should be an object');
            A.isObject(y, msg || 'second arg should be an object');
            A.areSame(Object.keys(x).length, Object.keys(y).length, msg || 'object keys are different lengths');
            for (var i in x) {
                if (x.hasOwnProperty(i)) {
                    cmp(x[i], y[i], msg);
                }
            }
            return;
        }
        A.areSame(x, y, msg || 'args should be the same');
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

        name: 'yui rs addon tests',

        'find yui resources': function() {
            var fixtures = libpath.join(__dirname, '../../../../../fixtures/store');
            store = new MockRS({ root: fixtures });
            store.plug(Y.mojito.addons.rs.yui, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

            var source = makeSource(fixtures, 'app', 'autoload', 'x.server.txt', true);
            var have = store.findResourceVersionByConvention(source, null);
            var want = undefined;
            cmp(have, want);

            source = makeSource(fixtures, 'app', 'blix', 'x.server.js', true);
            have = store.findResourceVersionByConvention(source, null);
            want = undefined;
            cmp(have, want);

            source = makeSource(fixtures, 'app', 'autoload', 'x.server.js', true);
            have = store.findResourceVersionByConvention(source, null);
            want = { type: 'yui-module', skipSubdirParts: 1 };
            cmp(have, want);

            source = makeSource(fixtures, 'app', 'yui_modules', 'x.server.js', true);
            have = store.findResourceVersionByConvention(source, null);
            want = { type: 'yui-module', skipSubdirParts: 1 };
            cmp(have, want);

            source = makeSource(fixtures, 'app', 'lang', 'x.server.js', true);
            have = store.findResourceVersionByConvention(source, null);
            want = { type: 'yui-lang', skipSubdirParts: 1 };
            cmp(have, want);
        },


        'parse found resource': function() {
            var fixtures = libpath.join(__dirname, '../../../../../fixtures/conventions');
            store = new MockRS({ root: fixtures });
            store.plug(Y.mojito.addons.rs.yui, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

            var source = makeSource(fixtures, 'app', 'autoload', 'm.common.js', true);
            var res = store.parseResourceVersion(source, 'yui-module');
            A.isNotUndefined(res);
            cmp(res.source, source);
            A.areSame('yui-module', res.type);
            A.areSame('common', res.affinity);
            A.areSame('*', res.selector);
            A.areSame('m', res.name);
            A.areSame('yui-module--m', res.id);
            A.isUndefined(res.mojit);

            source = makeSource(fixtures, 'app', 'autoload', 'm.common.iphone.js', true);
            res = store.parseResourceVersion(source, 'yui-module');
            A.isNotUndefined(res);
            cmp(res.source, source);
            A.areSame('yui-module', res.type);
            A.areSame('common', res.affinity);
            A.areSame('iphone', res.selector);
            A.areSame('m', res.name);
            A.areSame('yui-module--m', res.id);
            A.isUndefined(res.mojit);

            source = makeSource(fixtures, 'app', 'yui_modules', 'x.common.js', true);
            res = store.parseResourceVersion(source, 'yui-module');
            A.isNotUndefined(res);
            cmp(res.source, source);
            A.areSame('yui-module', res.type);
            A.areSame('common', res.affinity);
            A.areSame('*', res.selector);
            A.areSame('x', res.name);
            A.areSame('yui-module--x', res.id);
            A.isUndefined(res.mojit);

            source = makeSource(fixtures, 'bundle', 'lang', 'testing.js', true);
            res = store.parseResourceVersion(source, 'yui-lang', undefined, 'testing');
            A.isNotUndefined(res);
            cmp(res.source, source, 'testing.js source');
            A.areSame('yui-lang', res.type, 'testing.js type');
            A.areSame('common', res.affinity, 'testing.js affinity');
            A.areSame('*', res.selector, 'testing.js selector');
            A.areSame('lang/testing', res.name, 'testing.js name');
            A.areSame('yui-lang--lang/testing', res.id, 'testing.js id');
            A.areSame('testing', res.mojit, 'testing.js mojit');

            source = makeSource(fixtures, 'bundle', 'lang', 'testing_de.js', true);
            res = store.parseResourceVersion(source, 'yui-lang', undefined, 'testing');
            A.isNotUndefined(res);
            cmp(res.source, source, 'testing_de.js source');
            A.areSame('yui-lang', res.type, 'testing_de.js type');
            A.areSame('common', res.affinity, 'testing_de.js affinity');
            A.areSame('*', res.selector, 'testing_de.js selector');
            A.areSame('lang/testing_de', res.name, 'testing_de.js name');
            A.areSame('yui-lang--lang/testing_de', res.id, 'testing_de.js id');
            A.areSame('testing', res.mojit, 'testing_de.js mojit');

            source = makeSource(fixtures, 'bundle', 'lang', 'testing_en-US.js', true);
            res = store.parseResourceVersion(source, 'yui-lang', undefined, 'testing');
            A.isNotUndefined(res);
            cmp(res.source, source);
            A.areSame('yui-lang', res.type);
            A.areSame('common', res.affinity);
            A.areSame('*', res.selector);
            A.areSame('lang/testing_en-US', res.name);
            A.areSame('yui-lang--lang/testing_en-US', res.id);
            A.areSame('testing', res.mojit);
        },


        'parse other resources': function() {
            var fixtures = libpath.join(__dirname, '../../../../../fixtures/conventions');
            store = new MockRS({ root: fixtures });
            store.plug(Y.mojito.addons.rs.yui, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

            var source = makeSource(fixtures+'/mojits/X', 'mojit', '.', 'controller.common.js', true);
            var res = {
                source: source,
                mojit: 'X',
                type: 'controller',
                name: 'controller',
                id: 'controller--controller',
                affinity: 'common',
                selector: '*'
            };
            store.addResourceVersion(res);
            res = store.RVs['common/*' + '/controller--controller'];
            cmp(res.source, source);
            A.isNotUndefined(res.yui);
            A.areSame('X', res.yui.name);

            source = makeSource(fixtures+'/mojits/X', 'mojit', 'assets', 'foo.common.js', true);
            res = {
                source: source,
                mojit: 'X',
                type: 'asset',
                name: 'foo',
                id: 'asset-js-foo',
                affinity: 'common',
                selector: '*'
            };
            store.addResourceVersion(res);
            res = store.RVs['common/*' + '/asset-js-foo'];
            cmp(res.source, source);
            A.isUndefined(res.yui);
        },


        'find and parse resources by convention': function() {
            var fixtures = libpath.join(__dirname, '../../../../../fixtures/conventions');
            store = new Y.mojito.ResourceStore({ root: fixtures });

            // fake out some parts of preload(), which we're trying to avoid
            store._fwConfig = store.config.readConfigSimple(libpath.join(mojitoRoot, 'config.json'));
            store._appConfigStatic = store.getStaticAppConfig();
            store.plug(Y.mojito.addons.rs.yui, { appRoot: fixtures, mojitoRoot: mojitoRoot } );
            store.YUI = { add: function () {}, Env: { mods: {} } };

            var pkg = { name: 'test', version: '6.6.6' };
            var mojitType = 'testing';
            var ress = store._findResourcesByConvention(fixtures, 'app', pkg, mojitType);

            var r, res;
            for (r = 0; r < ress.length; r++) {
                res = ress[r];
                A.isNotUndefined(res.id, 'no resource id');
                switch (res.id) {
                    case 'action--x':
                    case 'action--y/z':
                    case 'addon-a-x':
                    case 'archetype-x-y':
                    case 'asset-css-x':
                    case 'asset-css-y/z':
                    case 'binder--x':
                    case 'command--x':
                    case 'config--config':
                    case 'controller--controller':
                    case 'middleware--x':
                    case 'spec--default':
                    case 'spec--x':
                    case 'view--x':
                        break;
                    case 'yui-lang--lang/testing':
                        A.areSame(pkg, res.source.pkg);
                        A.areSame('yui-lang', res.type);
                        A.areSame('lang/testing', res.name);
                        A.areSame('*', res.selector);
                        A.areSame('common', res.affinity);
                        A.areSame('.', res.source.fs.subDir);
                        A.areSame('testing', res.source.fs.basename);
                        A.areSame('.js', res.source.fs.ext);
                        break;
                    case 'yui-lang--lang/testing_de':
                        A.areSame(pkg, res.source.pkg);
                        A.areSame('yui-lang', res.type);
                        A.areSame('lang/testing_de', res.name);
                        A.areSame('*', res.selector);
                        A.areSame('common', res.affinity);
                        A.areSame('.', res.source.fs.subDir);
                        A.areSame('testing_de', res.source.fs.basename);
                        A.areSame('.js', res.source.fs.ext);
                        break;
                    case 'yui-lang--lang/testing_en':
                        A.areSame(pkg, res.source.pkg);
                        A.areSame('yui-lang', res.type);
                        A.areSame('lang/testing_en', res.name);
                        A.areSame('*', res.selector);
                        A.areSame('common', res.affinity);
                        A.areSame('.', res.source.fs.subDir);
                        A.areSame('testing_en', res.source.fs.basename);
                        A.areSame('.js', res.source.fs.ext);
                        break;
                    case 'yui-lang--lang/testing_en-US':
                        A.areSame(pkg, res.source.pkg);
                        A.areSame('yui-lang', res.type);
                        A.areSame('lang/testing_en-US', res.name);
                        A.areSame('*', res.selector);
                        A.areSame('common', res.affinity);
                        A.areSame('.', res.source.fs.subDir);
                        A.areSame('testing_en-US', res.source.fs.basename);
                        A.areSame('.js', res.source.fs.ext);
                        break;
                    case 'yui-module--m':
                        A.areSame(pkg, res.source.pkg);
                        A.areSame('yui-module', res.type);
                        A.areSame('m', res.name);
                        A.areSame('m', res.yui.name);
                        switch (res.source.fs.basename) {
                            case 'm.common':
                                A.areSame('*', res.selector);
                                A.areSame('common', res.affinity);
                                A.areSame('.js', res.source.fs.ext);
                                break;
                            case 'm.common.iphone':
                                A.areSame('iphone', res.selector);
                                A.areSame('common', res.affinity);
                                A.areSame('.js', res.source.fs.ext);
                                break;
                            default:
                                A.fail('unknown resource ' + res.source.fs.fullPath);
                                break;
                        }
                        break;
                    case 'yui-module--x':
                        A.areSame(pkg, res.source.pkg);
                        A.areSame('yui-module', res.type);
                        A.areSame('x', res.name);
                        A.areSame('x', res.yui.name);
                        switch (res.source.fs.basename) {
                            case 'x.common':
                                A.areSame('*', res.selector);
                                A.areSame('common', res.affinity);
                                A.areSame('.js', res.source.fs.ext);
                                break;
                            case 'x.common.iphone':
                                A.areSame('iphone', res.selector);
                                A.areSame('common', res.affinity);
                                A.areSame('.js', res.source.fs.ext);
                                break;
                            default:
                                A.fail('unknown resource ' + res.source.fs.fullPath);
                                break;
                        }
                        break;
                    case 'yui-module--z':
                        A.areSame(pkg, res.source.pkg);
                        A.areSame('yui-module', res.type);
                        A.areSame('z', res.name);
                        A.areSame('z', res.yui.name);
                        A.areSame('y', res.source.fs.subDir);
                        switch (res.source.fs.basename) {
                            case 'z.common':
                                A.areSame('*', res.selector);
                                A.areSame('common', res.affinity);
                                A.areSame('.js', res.source.fs.ext);
                                break;
                            case 'z.common.android':
                                A.areSame('android', res.selector);
                                A.areSame('common', res.affinity);
                                A.areSame('.js', res.source.fs.ext);
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
            A.areSame(31, ress.length, 'wrong number of resources');
        },


        'get config shared': function() {
            var fixtures,
                store,
                config;
            fixtures = libpath.join(__dirname, '../../../../../fixtures/store');
            store = new MockRS({ root: fixtures });
            store.plug(Y.mojito.addons.rs.yui, { appRoot: fixtures, mojitoRoot: mojitoRoot } );

            store._makeResource('server', {}, 'shared', 'binder', 'index', 'FooBinderIndex');
            store._makeResource('server', {}, 'shared', 'binder', 'list', 'FooBinderList', 'mojito');
            store._makeResource('server', {}, 'Foo', 'controller', 'controller', 'FooController');

            config = store.yui.getConfigShared('server');
            A.isNotUndefined(config.modules, 'false config.modules');
            A.isNotUndefined(config.modules.FooBinderIndex, 'false config.modules.FooBinderIndex');
            A.isNotUndefined(config.modules.FooBinderList, 'false config.modules.FooBinderList');
            A.isUndefined(config.modules.FooController, 'false config.modules.FooController');
        },


        'yui meta for loader-app-base-en-US': function() {
            var fixtures = libpath.join(__dirname, '../../../../../fixtures/gsg5'),
                series = [];
            store = new Y.mojito.ResourceStore({ root: fixtures });
            store.preload();

            series.push(function(next) {
                var res, ress;
                ress = store.getResourceVersions({mojit: 'shared', type: 'yui-module', subtype:'synthetic', name:'loader-app-base-en-US' });
                A.isArray(ress);
                A.areSame(1, ress.length, "didn't find yui-module-synthetic-loader-app-base-en-US");
                res = ress[0];
                A.isObject(res, "didn't find res for loader-app-base-en-US");
                store.getResourceContent(res, function(err, buffer, stat) {
                    A.isNull(err, 'error');
                    A.isNotNull(stat, 'stat');
                    meta = buffer.toString();
                    var matches = meta.match(/Y\.applyConfig\(([\s\S]+?)\);/);
                    var config = parseConfig(matches[1]);
                    A.isObject(config);
                    A.isObject(config.groups);
                    A.areSame(1, Object.keys(config.groups).length);
                    A.isObject(config.groups.app);
                    // we'll just spot-check a few things
                    A.isObject(config.groups.app.modules);
                    A.isObject(config.groups.app.modules['lang/PagedFlickr_en-US']);
                    A.isArray(config.groups.app.modules['lang/PagedFlickr_en-US'].requires);
                    AA.itemsAreEqual(['en-US'], config.groups.app.modules['PagedFlickr'].lang);
                    // No longer parsing lang files, so we cant read its yui meta
                    //AA.itemsAreEqual(['intl'], config.groups.app.modules['lang/PagedFlickr_en-US'].requires);
                    AA.itemsAreEqual(['lang/PagedFlickr_en-US.js'], config.groups.app.modules['lang/PagedFlickr_en-US'].path);
                    A.isObject(config.groups.app.modules['mojito-client']);
                    A.isArray(config.groups.app.modules['mojito-client'].requires);
                    A.isUndefined(config.groups.app.modules['mojito-client'].expanded_map);
                    A.isTrue(Object.keys(config.groups.app.modules['mojito-client'].requires).length > 0);
                    A.isObject(config.groups.app.modules['PagedFlickrBinderIndex']);
                    A.isArray(config.groups.app.modules['PagedFlickrBinderIndex'].requires);
                    A.isUndefined(config.groups.app.modules['lang/PagedFlickr_de']);
                    next();
                });
            });
            libasync.series(series, function(err) {
                A.isUndefined(err, 'no errors for all tests');
            });
        },


        'yui meta for loader-app': function() {
            var fixtures = libpath.join(__dirname, '../../../../../fixtures/gsg5'),
                series = [];
            store = new Y.mojito.ResourceStore({ root: fixtures });
            store.preload();

            series.push(function(next) {
                var res, ress;
                ress = store.getResourceVersions({mojit: 'shared', type: 'yui-module', subtype:'synthetic', name:'loader-app' });
                A.isArray(ress);
                A.areSame(1, ress.length);
                res = ress[0];
                A.isObject(res);
                store.getResourceContent(res, function(err, buffer, stat) {
                    A.isNull(err);
                    A.isNotNull(stat);
                    meta = buffer.toString();
                    A.areSame('YUI.add("loader",function(Y){},"",{requires:["loader-base","loader-yui3","loader-app"]});', meta);
                    next();
                });
            });
            libasync.series(series, function(err) {
                A.isUndefined(err, 'no errors for all tests');
            });
        },

        'ignore: gather list of all langs in app': function() {
            // TODO
        }

    }));

    suite.add(new YUITest.TestCase({

        name: 'yui yui addon YUI_config and Seed tests',

        setUp: function() {
            var fixtures;
            fixtures = libpath.join(__dirname, '../../../../../fixtures/store');
            store = new MockRS({ root: fixtures });
            store.plug(Y.mojito.addons.rs.yui, { appRoot: fixtures, mojitoRoot: mojitoRoot } );
        },

        tearDown: function() {
            store = null;
        },

        'test getYUIConfig': function() {
            var config;

            store.getAppConfig = function (ctx) {
                return {
                    foo: {
                        yui: {
                            config: {
                                combine: false
                            }
                        }
                    },
                    bar: {
                        yui: {
                            config: {
                                combine: true
                            }
                        }
                    }
                }[ctx.custom];
            };
            store.yui.langs = {
                'en-US': true
            }; // hack to avoid failures if langs array is undefined

            // testing context custom:foo
            config = store.yui.getYUIConfig({custom: "foo", lang: "es"});
            A.isFalse(config.combine, 'yui->config->combine should be false by default');
            A.isTrue(config.fetchCSS, 'yui->config->fetchCSS should be true by default');
            A.areSame('es', config.lang, 'yui->config->lang should be picked from context');
            A.areSame('http://yui.yahooapis.com/combo?', config.comboBase, 'By default, YUI core modules should come from CDN');

            // testing context custom:bar
            store.yui.langs = {
                'en-US': true
            }; // hack to avoid failures if langs array is undefined
            config = store.yui.getYUIConfig({custom: "bar", lang: "en-US"});
            A.isTrue(config.combine, 'yui->config->combine is not honored');
            A.isTrue(config.fetchCSS, 'yui->config->fetchCSS should be true by default even when combine is true');
            A.isObject(config.groups.app, 'yui->config->groups->app should be created synthetically');

            // testing serveYUIFromAppOrigin flag
            store.yui.staticHandling = {
                serveYUIFromAppOrigin: true
            };
            config = store.yui.getYUIConfig({custom: "bar", lang: "en-US"});
            A.areSame('/combo~', config.comboBase, 'When serving YUI core modules from local, combo should point to local');
            A.areSame(1024, config.maxURLLength, 'When serving YUI core modules from local, we should restrict the size of the url');
            A.areSame('~', config.comboSep, 'When serving YUI core modules from local, comboSep should be ~');
        },

        'test getAppGroupConfig': function() {
            var config;

            store.getAppConfig = function () {
                return {};
            };

            config = store.yui.getAppGroupConfig();
            A.isTrue(config.combine, 'combine should be true by default');
            A.areSame(1024, config.maxURLLength, 'maxURLLength should be 1024 by default');

            store.getAppConfig = function () {
                return {
                    yui: {
                        config: {
                            combine: false,
                            groups: {
                                app: {
                                    maxURLLength: 'maxURLLength',
                                    base: "base",
                                    comboBase: "comboBase",
                                    comboSep: "comboSep",
                                    root: "root"
                                }
                            }
                        }
                    }
                };
            };
            config = store.yui.getAppGroupConfig();
            A.isFalse(config.combine, 'yui->config->combine should be the fallback for yui->config->groups->app->combine');
            A.areSame('maxURLLength', config.maxURLLength, 'yui->config->groups->app->maxURLLength should be honored');
            A.areSame('base', config.base, 'yui->config->groups->app->base should be honored');
            A.areSame('comboBase', config.comboBase, 'yui->config->groups->app->comboBase should be honored');
            A.areSame('comboSep', config.comboSep, 'yui->config->groups->app->comboSep should be honored');
            A.areSame('root', config.root, 'yui->config->groups->app->root should be honored');
        },

        'test getAppSeedFiles': function() {
            var seed;
            store.yui.langs = {
                'en-US': true
            }; // hack to avoid failures if langs array is undefined

            store.yui.appModulesDetails = {
                "app-level-mod": {
                    url: "app/app.js"
                },
                "synthetic_en-US": {
                    url: "app-something/synthetic_en-US.js"
                }
            };

            // basic configuration
            seed = store.yui.getAppSeedFiles({
                lang: 'en-US'
            }, {});
            A.isArray(seed);
            A.areSame(0, seed.length, 'seed should come from second argument, which is empty in this assert');

            // custom seed with lang entries
            seed = store.yui.getAppSeedFiles({
                lang: 'en-US'
            }, {
                root: "root/",
                comboBase: "comboBase?",
                comboSep: "&",
                seed: ['yui-base', 'loader-base', 'app-level-mod', 'synthetic{langPath}'],
                groups: {
                    app: {
                        root: "app-root/",
                        comboBase: "app-comboBase?",
                        comboSep: "~"
                    }
                }
            });
            A.isArray(seed);
            A.areSame(2, seed.length, 'single url with all modules comboded');
            A.areSame('comboBase?root/yui-base/yui-base-min.js&root/loader-base/loader-base-min.js', seed[0], 'yui modules should be honored');
            A.areSame('app-comboBase?app-root/app.js~app-root/synthetic_en-US.js', seed[1], 'synthetic and app level modules should be honored');
        },

        'test getAppSeedFiles with combo off': function() {
            var assets = store.yui.getAppSeedFiles({
                lang: 'en-US'
            }, {
                seed: ['foo', 'bar'],
                comboBase: 'comboBase?',
                comboSep: '~',
                base: 'base/',
                root: 'root/',
                combine: false
            });

            A.areSame(2, assets.length, 'combine: false should be honored');
            A.areSame('base/foo/foo-min.js', assets[0], 'invalid url construction when combo is off');
            A.areSame('base/bar/bar-min.js', assets[1], 'invalid url construction when combo is off for secundary module');
        },

        'test getAppSeedFiles with combo on with external urls entries': function() {
            var assets = store.yui.getAppSeedFiles({
                lang: 'en-US'
            }, {
                seed: ['http://mojito.yahoo/baz', 'foo', 'bar'],
                comboBase: 'comboBase?',
                comboSep: '~',
                base: 'base/',
                root: 'root/',
                combine: true
            });

            A.areSame(2, assets.length, 'external urls should be honored');
            A.areSame('http://mojito.yahoo/baz', assets[0], 'problem with external url detection');
            A.areSame('comboBase?root/foo/foo-min.js~root/bar/bar-min.js', assets[1], 'invalid url construction when combo is on and external urls are also in the mix');
        },

        'test getAppSeedFiles with combo off with external urls entries': function() {
            var assets = store.yui.getAppSeedFiles({
                lang: 'en-US'
            }, {
                seed: ['http://mojito.yahoo/baz', 'foo', 'bar'],
                comboBase: 'comboBase?',
                comboSep: '~',
                base: 'base/',
                root: 'root/',
                combine: false
            });

            A.areSame(3, assets.length, 'external urls should be honored');
            A.areSame('http://mojito.yahoo/baz', assets[0], 'problem with external url detection');
            A.areSame('base/foo/foo-min.js', assets[1], 'invalid url construction when combo is off and external urls are also in the mix');
            A.areSame('base/bar/bar-min.js', assets[2], 'invalid url construction when combo is off and external urls are also in the mix');
        },

        'test getAppSeedFiles flushing order': function() {
            var assets = store.yui.getAppSeedFiles({
                lang: 'en-US'
            }, {
                seed: [
                    'http://mojito.yahoo/baz', 'foo', 'bar', 'http://mojito.yahoo/bar', 'baz'
                ],
                comboBase: 'comboBase?',
                comboSep: '~',
                base: 'base/',
                root: 'root/'
            });

            A.areSame(4, assets.length, 'order should be honored');
            A.areSame('http://mojito.yahoo/baz', assets[0], 'initial external url order not honored');
            A.areSame('comboBase?root/foo/foo-min.js~root/bar/bar-min.js', assets[1], 'combo in second position not honored');
            A.areSame('http://mojito.yahoo/bar', assets[2], 'external url in the middle not honored');
            A.areSame('comboBase?root/baz/baz-min.js', assets[3], 'combo at the end not honored');
        }

    }));

    Y.Test.Runner.add(suite);

});
