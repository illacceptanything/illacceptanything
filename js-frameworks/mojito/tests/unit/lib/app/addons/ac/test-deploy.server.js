/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


YUI().use('mojito-deploy-addon', 'test', 'json-parse', function(Y) {
    var suite = new Y.Test.Suite('mojito-deploy-addon tests'),
        cases = {},
        A = Y.Assert,
        AA = Y.ArrayAssert,
        OA = Y.ObjectAssert,

        addon;

    cases = {
        name: 'mojito-deploy-addon tests',

        setUp: function() {
            addon = new Y.mojito.addons.ac.deploy(
                {instance: {}}, // command
                {page: { clientRoutes: { 'get': { } } } } // adapter
            );
            addon.ac = {
                http: {
                    getHeader: function(h) {
                        return null;
                    }
                },
                context: {}
            };
        },

        tearDown: function() {
            addon = null;
        },


        'YUI.applyConfig() should use application.json yui.config': function() {
            var blobs = [];
            var assetHandler = {
                    addCss: function(path, location) {
                        // not testing this
                        return;
                    },
                    addAssets: function(type, location, content) {
                        // not testing this
                        return;
                    },
                    addAsset: function(type, location, content) {
                        if ('blob' === type) {
                            blobs.push(content);
                        }
                    }
                };

            var binderMap = {};

            addon.setStore({
                getAppConfig: function() {
                    return {};
                },
                _updateLoader: function () {
                    return true;
                },
                yui: {
                    getYUIConfig: function() {
                        return {
                            lang: 'klingon',
                            foo:'bar',
                            seed: ['/static/seed.js']
                        };
                    }
                }
            });

            addon.constructMojitoClientRuntime(assetHandler, binderMap);

            A.areSame(1, blobs.length, 'wrong number of blobs');
            var matches = blobs[0].match(/YUI\.applyConfig\((.+?)\);/);
            A.isNotUndefined(matches[1], 'failed to find YUI.applyConfig() in blob');
            var config = Y.JSON.parse(matches[1]);
            A.isObject(config, 'failed to parse YUI.applyConfig()');
            A.areSame('bar', config.foo, 'failed to base YUI.applyConfig() on application.yui.config');
            A.areSame('klingon', config.lang, 'wrong lang used');
        },


        'test constructMojitoClientRuntime w/ a binderMap': function() {
            var blobs = [],
                assetHandler = {
                    addCss: function(path, location) {
                        // not testing this
                        return;
                    },
                    addAssets: function(type, location, content) {
                        // not testing this
                        return;
                    },
                    addAsset: function(type, location, content) {
                        if ('blob' === type) {
                            blobs.push(content);
                        }
                    }
                },
                binderMap = {
                    'viewId1': {
                        needs: 'a drink'
                    },
                    'viewId2': {
                        needs: 'another drink'
                    }
                };

            addon.setStore({
                getAppConfig: function() {
                    return {yui: {config: {discarded: true}}};
                },
                getRoutes: function() {
                    return ['routes'];
                },
                _updateLoader: function () {
                    return true;
                },
                yui: {
                    getYUIConfig: function() {
                        return {
                            lang: 'klingon',
                            foo:'bar',
                            seed: ['/static/seed.js']
                        };
                    }
                }
            });

            addon.constructMojitoClientRuntime(assetHandler, binderMap);

            A.isArray(blobs);
            A.areSame(1, blobs.length, 'wrong number of blobs');

            var matches = blobs[0].match(/YUI\.applyConfig\((.+?)\);/);
            A.isNotUndefined(matches[1], 'failed to find YUI.applyConfig() in blob');
            var config = Y.JSON.parse(matches[1]);
            A.isObject(config, 'failed to parse YUI.applyConfig()');
            A.areSame('bar', config.foo, 'failed to base YUI.applyConfig() on application.yui.config');
            A.areSame('klingon', config.lang, 'wrong lang used');

            // window.YMojito = { client: new Y.mojito.Client({...}) };
            matches = blobs[0].match(/window\.YMojito = { client: new Y\.mojito\.Client\((.+?)\) };/);
            A.isTrue(!!matches, 'failed to find new Y.mojito.Client() in blob');
            A.isNotUndefined(matches[1], 'failed to find new Y.mojito.Client() in blob');
            config = Y.JSON.parse(matches[1]);
            A.isObject(config, 'failed to parse Y.mojito.Client() config');
            A.isObject(config.context, 'config.context should be an object');
            A.areSame('client', config.context.runtime, 'config.context.runtime should be "client"');
            A.isObject(config.binderMap, 'config.binderMap should be an object');
            A.isObject(config.binderMap.viewId1, 'config.binderMap.viewId1 should be an object');
            A.areSame('a drink', config.binderMap.viewId1.needs, 'config.binderMap.viewId1.needs should be "a drink"');
            A.isObject(config.binderMap.viewId2, 'config.binderMap.viewId2 should be an object');
            A.areSame('another drink', config.binderMap.viewId2.needs, 'config.binderMap.viewId2.needs should be "another drink"');
            A.isObject(config.appConfig, 'config.appConfig should be an object');
            A.isObject(config.appConfig.yui, 'config.appConfig.yui should be an object');
            A.isObject(config.routes, 'config.routes should be an object');
            A.isObject(config.routes.get, 'config.routes.get should be an object');
            A.isObject(config.page, 'config.page should be an object');
        },


        'test application.json should honor yui.config.fetchCSS=false': function() {
            addon.setStore({
                getAppConfig: function() {
                    return {};
                },
                getRoutes: function() {
                    return ['routes'];
                },
                _updateLoader: function () {
                    return true;
                },
                yui: {
                    getYUIConfig: function() {
                        return {
                            lang: 'klingon',
                            fetchCSS:false,
                            seed: ['/static/seed.js']
                        };
                    }
                }
            });

            var counts = {};
            var assetHandler = {
                    addCss: function(path, location) {
                        counts['css ' + location] = counts['css ' + location] || 0;
                        counts['css ' + location]++;
                        return;
                    },
                    addAssets: function(args) {
                        for (var location in args) {
                            if (args.hasOwnProperty(location)) {
                                for (var type in args[location]) {
                                    if (args[location].hasOwnProperty(type)) {
                                        var contents = args[location][type];
                                        counts[type + ' ' + location] = counts[type + ' ' + location] || 0;
                                        counts[type + ' ' + location] += contents.length;
                                    }
                                }
                            }
                        }
                        return;
                    },
                    addAsset: function(type, location, content) {
                        counts[type + ' ' + location] = counts[type + ' ' + location] || 0;
                        counts[type + ' ' + location]++;
                    }
                };
            var binderMap = {};

            addon.constructMojitoClientRuntime(assetHandler, binderMap);

            A.areSame(2, Object.keys(counts).length, 'too many type:location pairs');
            A.areSame(1, counts['js top'], 'wrong number of js:top');
            A.areSame(1, counts['blob bottom'], 'wrong number of blob:bottom');
        },


        'test constructMojitoClientRuntime processes yui config correctly': function() {
            addon.setStore({
                getAppConfig: function() {
                    return {};
                },
                getRoutes: function() {
                    return ['routes'];
                },
                _updateLoader: function () {
                    return true;
                },
                yui: {
                    getYUIConfig: function() {
                        return {
                            lang: 'klingon',
                            comboSep: '&',
                            groups: {
                                app: {
                                    comboSep: '&'
                                }
                            },
                            seed: ['/static/seed.js']
                        };
                    }
                }
            });

            var blobs = [];
            var assetHandler = {
                    addCss: function(path, location) {
                        // not testing this
                        return;
                    },
                    addAssets: function(type, location, content) {
                        // not testing this
                        return;
                    },
                    addAsset: function(type, location, content) {
                        if ('blob' === type) {
                            blobs.push(content);
                        }
                    }
                };
            var binderMap = {};
            addon.constructMojitoClientRuntime(assetHandler, binderMap);

            var matches = blobs[0].match(/YUI\.applyConfig\((.+?)\);/);
            A.isNotUndefined(matches[1], 'failed to find YUI.applyConfig() in blob');
            var config = Y.JSON.parse(matches[1]);
            A.areSame('&', config.comboSep, 'comboSep got mangled');
            A.areSame('&', config.groups.app.comboSep, 'groups.app.comboSep got mangled');
        }

    };

    suite.add(new Y.Test.Case(cases));
    Y.Test.Runner.add(suite);
});
