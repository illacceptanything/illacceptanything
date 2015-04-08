/*
 * Copyright (c) 2012, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, nomen:true, stupid:true, continue:true, node:true*/
/*global YUI*/


/**
 * @module ResourceStoreAddon
 */


/**
 * @class RSAddonYUI
 * @extension ResourceStore.server
 */
YUI.add('addon-rs-yui', function(Y, NAME) {

    'use strict';

    var libfs   = require('fs'),
        libpath = require('path'),
        libvm   = require('vm'),
        libmime = require('mime'),
        liburl  = require('url'),
        libutil = require('../../../util.js'),
        Module = require('module'),
        serialize = require('express-state/lib/serialize'),

        WARN_SERVER_MODULES = /\b(dom-[\w\-]+|node-[\w\-]+|io-upload-iframe)/ig,
        MODULE_SUBDIRS = {
            autoload: true,
            tests: true,
            yui_modules: true
        },

        resourceSortByDepthTest = function (a, b) {
            return a.source.pkg.depth - b.source.pkg.depth;
        },

        yuiSandboxFactory = require(libpath.join(__dirname, '..', '..', '..', 'yui-sandbox.js')),
        syntheticStat = null,

        MODULE_META_ENTRIES = ['path', 'requires', 'use', 'optional', 'skinnable', 'after',
            'condition', 'lang', 'langPack', 'test', 'templates', 'langBundles', 'optionalRequires'],

        REGEX_LANG_TOKEN = /\"\{langToken\}\"/g,
        REGEX_LANG_PATH  = /\{langPath\}/g,
        REGEX_LOCALE     = /\_([a-zA-Z0-9]+(-[a-zA-Z0-9]+)*)$/,

        MODULE_PER_LANG  = ['loader-app-base'],
        MODULE_TEMPLATES = {
            /*
             * This is a replacement of the original loader to include loader-app
             * module, which represents the meta of the app.
             */
            'loader-app':
                'YUI.add("loader",function(Y){' +
                '},"",{requires:["loader-base","loader-yui3","loader-app"]});',

            /*
             * Use this module when you want to rely on the loader to do recursive
             * computations to resolve combo urls for app yui modules in the client
             * runtime.
             * Note: This is the default config used by YUI.
             */
            'loader-app-base':
                'YUI.add("loader-app",function(Y){' +
                    'Y.applyConfig({groups:{app:Y.merge(' +
                        '((Y.config.groups&&Y.config.groups.app)||{}),' +
                        '{modules:{app-base}}' +
                    ')}});' +
                '},"",{requires:["loader-base"]});'

        };

    function RSAddonYUI() {
        RSAddonYUI.superclass.constructor.apply(this, arguments);
    }
    RSAddonYUI.NS = 'yui';

    Y.extend(RSAddonYUI, Y.Plugin.Base, {

        /**
         * This methods is part of Y.Plugin.Base.  See documentation for that for details.
         * @method initializer
         * @param {object} config Configuration object as per Y.Plugin.Base
         * @return {nothing}
         */
        initializer: function(config) {
            this.host = config.host;
            this.appRoot = config.appRoot;
            this.mojitoRoot = config.mojitoRoot;

            // for all synthetic files, since we don't have an actual file, we need to
            // create a stat object, in this case we use the mojito folder stat as
            // a replacement. We make it syncronous since it is meant to be executed
            // once during the preload process.
            syntheticStat = libfs.statSync(libpath.join(__dirname, '../../../..'));

            this.afterHostMethod('preloadResourceVersions', this.preloadResourceVersions, this);
            this.afterHostMethod('findResourceVersionByConvention', this.findResourceVersionByConvention, this);
            this.beforeHostMethod('parseResourceVersion', this.parseResourceVersion, this);
            this.beforeHostMethod('addResourceVersion', this.addResourceVersion, this);
            this.beforeHostMethod('makeResourceVersions', this.makeResourceVersions, this);
            this.afterHostMethod('resolveResourceVersions', this.resolveResourceVersions, this);
            this.beforeHostMethod('getResourceContent', this.getResourceContent, this);
            this.onHostEvent('loadConfigs', this.loadConfigs, this);

            this.loadConfigs();

            this.langs = {};            // keys are list of languages in the app, values are simply "true"
            this.resContents = {};      // res.id: contents
            this.appModulesDetails = {};    // res.yui.name: static handler details
            this.yuiModulesDetails = {};    // res.yui.name: static handler details

            this._langLoaderCreated = {};   // lang mapping indicating whether a lang specific loader has been created.
        },

        preloadResourceVersions: function () {
            this._langLoaderCreated = {};
        },

        loadConfigs: function () {
            this.staticAppConfig = this.host.getStaticAppConfig() || {};
            this.staticHandling = this.staticAppConfig.staticHandling || {};
            this.staticPrefix = libutil.webpath('/', (this.staticHandling.prefix || 'static'), "/");
            this.yuiConfig = (this.staticAppConfig.yui && this.staticAppConfig.yui.config) || {};
        },

        /**
         * Returns a datastructure which tells a YUI instance where to find
         * the YUI modules that are shared among all mojits.
         * @method getConfigShared
         * @param {string} env runtime environment (either `client`, or `server`)
         * @return {object} datastructure for configuring YUI
         */
        getConfigShared: function(env) {
            var r,
                res,
                ress,
                modules = {};
            ress = this.get('host').getMojitResourceVersions('shared');
            for (r = 0; r < ress.length; r += 1) {
                res = ress[r];
                if (!res.yui || !res.yui.name) {
                    continue;
                }
                if (res.affinity.affinity !== env && res.affinity.affinity !== 'common') {
                    continue;
                }
                modules[res.yui.name] = this._makeYUIModuleConfig(env, res);
            }
            return { modules: modules };
        },


        /**
         * Returns a datastructure which tells a YUI instance where to find
         * the YUI modules in the app.
         * @method getModulesConfig
         * @param {string} env runtime environment (either `client`, or `server`)
         * @param {boolean} justApp Indicates whether to include the YUI
         *      modules just found in the application (true), or also include
         *      those found in mojito (false).
         * @return {object} datastructure for configuring YUI
         */
        getModulesConfig: function(env, justApp, lang) {
            var store = this.get('host'),
                m,
                mojit,
                mojits,

                modules = {};

            mojits = store.listAllMojits();
            mojits.push('shared');
            for (m = 0; m < mojits.length; m += 1) {
                mojit = mojits[m];
                this.getMojitModulesConfig(mojit, modules, env, justApp, lang);
            }
            return { modules: modules };
        },

        getMojitModulesConfig: function (mojit, modules, env, justApp, lang) {
            var store = this.get('host'),
                r,
                res,
                ress;

            ress = store.getMojitResourceVersions(mojit);
            ress.sort(resourceSortByDepthTest);
            for (r = 0; r < ress.length; r += 1) {
                res = ress[r];
                if (!res.yui || !res.yui.name) {
                    continue;
                }
                if (res.affinity.affinity !== env && res.affinity.affinity !== 'common') {
                    continue;
                }
                if (justApp && ('mojito' === res.source.pkg.name)) {
                    continue;
                }

                if (lang && res.type === 'yui-lang' && res.yui.lang !== lang && res.yui.lang !== '') {
                    continue;
                }
                // don't overwrite resource if it's there already
                modules[res.yui.name] = modules[res.yui.name] || this._makeYUIModuleConfig(env, res);
            }
        },


        /**
         * Hook to allow other RS addons to control the yui
         * configuration. By default, the `yui.config` will
         * allow customization of the combo handler when needed
         * from `application.json`.
         * @method getYUIConfig
         * @param {object} ctx the context
         * @return {object} yui configuration
         */
        getYUIConfig: function(ctx) {
            var version = Y.version,
                yuiPrefix = libutil.webpath(this.staticPrefix, 'yui/'),
                appConfig = this.get('host').getAppConfig(ctx),
                yuiConfig;

            if (this.staticHandling.serveYUIFromAppOrigin) {

                // by default, we want to serve YUI from CDN
                yuiConfig = {

                    maxURLLength: 1024,
                    base: yuiPrefix,
                    comboBase: "/combo~",
                    comboSep: "~",
                    root: yuiPrefix

                };

            } else {

                yuiConfig = {

                    // the base path for non-combo paths
                    base: 'http://yui.yahooapis.com/' + version + '/',
                    // the path to the combo service
                    comboBase: 'http://yui.yahooapis.com/combo?',
                    comboSep: '&',
                    // a fragment to prepend to the path attribute when
                    // when building combo urls
                    root: version + '/'

                };

            }

            yuiConfig = Y.merge(yuiConfig, {
                fetchCSS: true,
                combine: true
            }, (appConfig.yui && appConfig.yui.config) || {});

            // to boot the app in the client with the proper lang
            yuiConfig.lang = ctx.lang;

            yuiConfig.groups = yuiConfig.groups || {};
            yuiConfig.groups.app = this.getAppGroupConfig(ctx, yuiConfig);
            yuiConfig.seed = this.getAppSeedFiles(ctx, yuiConfig);

            return yuiConfig;
        },


        /**
         * Hook to allow other RS addons to control the combo
         * handler configuration for group "app". By default,
         * the `yui.config.groups.app` will allow customization
         * of the combo handler when needed from `application.json`
         * @method getAppGroupConfig
         * @param {object} ctx the context
         * @return {object} yui configuration for group "app"
         */
        getAppGroupConfig: function(ctx) {
            var appConfig = this.get('host').getAppConfig(ctx),
                yuiConfig = (appConfig.yui && appConfig.yui.config) || {};

            return Y.merge({
                combine: (yuiConfig.combine === false) ? false : true,
                maxURLLength: 1024,
                base: this.staticPrefix,
                comboBase: "/combo~",
                comboSep: "~",
                root: this.staticPrefix
            }, ((yuiConfig.groups && yuiConfig.groups.app) || {}));
        },


        /**
         * Produce the YUI seed files. This can be controlled through
         * application.json->yui->config->seed in a form of
         * a array with the list of full paths for all seed files.
         * @method getAppSeedFiles
         * @param {object} ctx the context
         * @param {object} yuiConfig the config that is sent to client
         * @return {array} list of seed files
         */
        getAppSeedFiles: function(ctx, yuiConfig) {

            yuiConfig = yuiConfig || {}; // to support legacy

            var files = [],
                seed = Y.Array(yuiConfig.seed || []),
                appGroupConfig = (yuiConfig.groups && yuiConfig.groups.app) || {},
                hash = {},
                appModules = [],
                yuiModules = [],
                filter = yuiConfig.filter || 'min',
                file,
                lang,
                i;

            // picking up the closest language based on yui config
            // of from the yui bundles
            lang = Y.mojito.util.findClosestLang(ctx.lang, this.langs);

            function newEntry(f) {
                // flushing any pending combo for yui core modules
                if (yuiModules.length > 0) {
                    files.push(yuiConfig.comboBase + yuiModules.join(yuiConfig.comboSep));
                    yuiModules = [];
                }
                // flushing any pending combo for app modules
                if (appModules.length > 0) {
                    files.push(appGroupConfig.comboBase + appModules.join(appGroupConfig.comboSep));
                    appModules = [];
                }
                if (f) {
                    files.push(f);
                }
            }

            // adjusting filter to be url friendly
            filter = filter === 'raw' ? '' : '-' + filter;

            // adjusting lang just to be url friendly
            lang = lang ? '_' + lang : '';

            // The seed files collection is lang aware, hence we should adjust
            // is on runtime.
            for (i = 0; i < seed.length; i += 1) {

                // adjusting the seed based on {langToken} to facilitate
                // the customization of the seed file url per lang.
                seed[i] = seed[i].replace(REGEX_LANG_PATH, lang);

                if (hash.hasOwnProperty(seed[i])) {

                    Y.log('Skiping duplicated entry in yui.config.seed: ' + seed[i], 'warn', NAME);

                } else if (liburl.parse(seed[i]).protocol) {

                    newEntry(seed[i]);

                } else if (this.appModulesDetails.hasOwnProperty(seed[i])) {

                    // app module
                    file = this.appModulesDetails[seed[i]].url.split('/').pop();
                    // default app module
                    if (appGroupConfig.combine === false) {
                        // if the combo is disabled, then we need to insert one by one
                        // this is useful for offline and hybrid apps where the combo
                        // does not work.
                        newEntry(appGroupConfig.base + file);
                    } else {
                        // the item is a module and should be combined
                        appModules.push(appGroupConfig.root + file);
                    }

                } else {

                    // assume yui core module
                    file = seed[i] + '/' + seed[i] + filter + '.js';
                    // the module is a yui core module, treat is accordingly
                    if (yuiConfig.combine === false) {
                        // if the combo is disabled, then we need to insert one by one
                        // this is useful for offline and hybrid apps where the combo
                        // does not work.
                        newEntry(yuiConfig.base + file);
                    } else {
                        // the item is a module and should be combined
                        yuiModules.push(yuiConfig.root + file);
                    }

                }
                // hash table to avoid duplicated entries in the seed
                hash[seed[i]] = true;
            }

            newEntry(); // just to flush any remaining entry

            return files;
        },


        /**
         * Aggregate all yui core files
         * using the path of as the hash.
         *
         * @private
         * @method getYUIURLDetails
         * @return {object} yui core resources by url
         */
        getYUIURLDetails: function () {
            var name,
                urls = {};

            for (name in this.yuiModulesDetails) {
                if (this.yuiModulesDetails.hasOwnProperty(name)) {
                    urls[this.yuiModulesDetails[name].url] = this.yuiModulesDetails[name];
                }
            }
            return urls;
        },


        /**
         * Using AOP, this is called after the ResourceStore's version.
         * @method findResourceVersionByConvention
         * @param {object} source metadata about where the resource is located
         * @param {string} mojitType name of mojit to which the resource likely belongs
         * @return {object||null} for yui modules or lang bundles, returns metadata signifying that
         */
        findResourceVersionByConvention: function(source, mojitType) {
            var fs = source.fs;

            if (!fs.isFile) {
                return;
            }
            if ('.js' !== fs.ext) {
                return;
            }

            if (fs.subDirArray.length >= 1 && MODULE_SUBDIRS[fs.subDirArray[0]]) {
                return new Y.Do.AlterReturn(null, {
                    type: 'yui-module',
                    skipSubdirParts: 1
                });
            }

            if (fs.subDirArray.length >= 1 && 'lang' === fs.subDirArray[0]) {
                return new Y.Do.AlterReturn(null, {
                    type: 'yui-lang',
                    skipSubdirParts: 1
                });
            }
        },


        /**
         * Using AOP, this is called before the ResourceStore's version.
         * @method parseResourceVersion
         * @param {object} source metadata about where the resource is located
         * @param {string} type type of the resource
         * @param {string} subtype subtype of the resource
         * @param {string} mojitType name of mojit to which the resource likely belongs
         * @return {object||null} for yui modules or lang bundles, returns the resource metadata
         */
        parseResourceVersion: function(source, type, subtype, mojitType) {
            var store = this.get('host'),
                fs = source.fs,
                baseParts,
                res,
                sandbox,
                m;

            // If lazyLangs is on then process yui lang files just
            // by reading the filename, instead of executing them.
            if ('yui-lang' === type && store.lazyLangs) {
                res = {
                    source: source,
                    mojit: mojitType,
                    type: 'yui-lang',
                    affinity: 'common',
                    selector: '*'
                };
                if (!res.yui) {
                    res.yui = {};
                }

                if (source.fs.basename.indexOf(mojitType) === 0) {
                    m = source.fs.basename.substring(mojitType.length).match(REGEX_LOCALE);
                    res.yui.lang = (m && m[1]) || '';
                    res.yui.name = 'lang/' + mojitType + (res.yui.lang ? '_' + res.yui.lang : '');
                    res.name = res.yui.name;
                    res.id = [res.type, res.subtype, res.name].join('-');
                    if (!store.lazyLangs) {
                        this.langs[res.yui.lang] = true;
                    }
                } else {
                    Y.log('Unexpected lang filename "' + source.fs.basename + '". The filename should start with "' + mojitType + '"', 'warn');
                }
                return new Y.Do.Halt(null, res);
            }

            if ('yui-lang' === type) {
                res = {
                    source: source,
                    mojit: mojitType,
                    type: 'yui-lang',
                    affinity: 'common',
                    selector: '*'
                };
                if (!res.yui) {
                    res.yui = {};
                }
                sandbox = {
                    Intl: {
                        add: function(langFor, lang) {
                            res.yui.langFor = langFor;
                            res.yui.lang = lang;
                        }
                    }
                };
                this._captureYUIModuleDetails(res, sandbox);

                if (!res.yui) {
                    // This resource is not a valid YUI module and should not be added.
                    return new Y.Do.Halt();
                }

                res.name = res.yui.name;
                res.id = [res.type, res.subtype, res.name].join('-');
                this.langs[res.yui.lang] = true;
                if (res.yui.name === 'lang/' + res.yui.langFor) {
                    res.yui.isRootLang = true;
                }
                return new Y.Do.Halt(null, res);
            }

            if ('yui-module' === type) {
                baseParts = fs.basename.split('.');
                res = {
                    source: source,
                    mojit: mojitType,
                    type: 'yui-module',
                    affinity: 'server',
                    selector: '*'
                };
                if (baseParts.length >= 3) {
                    res.selector = baseParts.pop();
                }
                if (baseParts.length >= 2) {
                    res.affinity = baseParts.pop();
                }
                if (baseParts.length !== 1) {
                    Y.log('invalid yui-module filename. skipping ' + fs.fullPath, 'warn', NAME);
                    return;
                }
                this._captureYUIModuleDetails(res);

                if (!res.yui) {
                    // This resource is not a valid YUI module and should not be added.
                    return new Y.Do.Halt();
                }
                res.name = res.yui.name;
                res.id = [res.type, res.subtype, res.name].join('-');
                return new Y.Do.Halt(null, res);
            }
        },


        /**
         * Using AOP, this is called before the ResourceStore's version.
         * If the resource is a YUI module, augments the metadata with details
         * about the YUI module.
         * @method addResourceVersion
         * @param {object} res resource version metadata
         * @return {nothing}
         */
        addResourceVersion: function(res) {
            if ('.js' !== res.source.fs.ext) {
                return;
            }
            if (res.yui && res.yui.name) {
                // work done already
                return;
            }
            // ASSUMPTION:  no app-level resources are YUI modules
            if (!res.mojit) {
                return;
            }
            if ('asset' === res.type) {
                return;
            }

            var store = this.get('host');

            this._captureYUIModuleDetails(res);

            if (!res.yui) {
                // Do not add YUI resources that failed while capturing details.
                return new Y.Do.Halt();
            }
        },


        /**
         * Using AOP, this is called before the ResourceStore's version.
         * We register some fake resource versions that represent the YUI
         * configurations.
         * @method addResourceVersion
         * @param {object} res resource version metadata
         * @return {nothing}
         */
        makeResourceVersions: function() {
            var store = this.get('host'),
                res,
                l,
                langs = Object.keys(this.langs);

            // we always want to make the no-lang version
            if (!this.langs['']) {
                langs.push('');
            }

            res = {
                source: {},
                mojit: 'shared',
                type: 'yui-module',
                subtype: 'synthetic',
                name: 'loader-app',
                affinity: 'client',
                selector: '*',
                yui: {
                    name: 'loader-app'
                }
            };
            res.id = [res.type, res.subtype, res.name].join('-');
            res.source.pkg = store.getAppPkgMeta();
            res.source.fs = store.makeResourceFSMeta(this.appRoot, 'app', '.', 'loader-app.js', true);
            store.addResourceVersion(res);

            for (l = 0; l < langs.length; l += 1) {
                this._makeLoaderResourceVersion(langs[l]);
            }

            // we can also make some fake resources for all yui
            // modules that we might want to serve.
            this._precalcYUIResources();
        },

        _makeLoaderResourceVersion: function (lang) {
            if (this._langLoaderCreated[lang]) {
                return [];
            }

            var store = this.get('host'),
                i,
                name,
                res,
                ress = [],
                langExt = lang ? '_' + lang : '';

            for (i = 0; i < MODULE_PER_LANG.length; i += 1) {

                name = MODULE_PER_LANG[i];

                res = {
                    source: {},
                    mojit: 'shared',
                    type: 'yui-module',
                    subtype: 'synthetic',
                    name: [name, lang].join('-'),
                    affinity: 'client',
                    selector: '*',
                    yui: {
                        name: name + langExt
                    }
                };
                res.id = [res.type, res.subtype, res.name].join('-');
                res.source.pkg = store.getAppPkgMeta();
                res.source.fs = store.makeResourceFSMeta(this.appRoot, 'app', '.',
                    name + langExt + '.js', true);
                store.addResourceVersion(res);
                ress.push(res);
            }
            this._langLoaderCreated[lang] = true;
            this.langs[lang] = true;
            return ress;
        },


        /**
         * Using AOP, this is called after the ResourceStore's version.
         * We precompute the YUI configurations.
         * @method resolveResourceVersions
         * @return {nothing}
         */
        resolveResourceVersions: function() {
            var me = this,
                store = this.get('host'),
                m,
                mojit,
                mojits;

            me._processResources(store.getAppResourceVersions());
            mojits = store.listAllMojits();
            mojits.push('shared');
            for (m = 0; m < mojits.length; m += 1) {
                mojit = mojits[m];
                me._processResources(store.getMojitResourceVersions(mojit));
            }

            this._precalcLoaderMeta();
        },

        _processResources: function (ress) {
            var r,
                store = this.get('host'),
                res;
            for (r = 0; r < ress.length; r += 1) {
                res = ress[r];
                if ('client' !== res.affinity.affinity) {
                    // appModulesDetails is used by getAppSeedFiles, which only matters for the client
                    continue;
                }
                if (!res.yui || !res.yui.name) {
                    continue;
                }
                if (this.appModulesDetails[res.yui.name]) {
                    if (this.appModulesDetails[res.yui.name].path !== res.source.fs.fullPath) {
                        Y.log('YUI module collision for name=' + res.yui.name +
                              '. Choosing:\n' + this.appModulesDetails[res.yui.name].path +
                                      ' over\n' + res.source.fs.fullPath, 'debug', NAME);
                    }
                } else {
                    this.appModulesDetails[res.yui.name] = store.makeStaticHandlerDetails(res);
                }
            }
        },

        /**
         * Return the content for resources we make in makeResourceVersions().
         *
         * @method getResourceContent
         * @param {object} details static handling details
         * @param {function} callback callback used to return the resource content (or error)
         * @param {Error|undefined} callback.err Error that occurred, if any.
         *      If an error is given that the other two arguments will be undefined.
         * @param {Buffer} callback.content the contents of the resource
         * @param {Stat||null} callback.stat Stat object with details about the file on the filesystem
         *          Can be null if the resource doesn't have a direct representation on the filesystem.
         * @return {undefined} nothing is returned, the results are returned via the callback
         */
        getResourceContent: function(res, callback) {
            var contents = res.name && this.resContents[res.name];
            if (contents) {
                callback(null, new Buffer(contents, 'utf8'), syntheticStat);
                return new Y.Do.Halt(null, null);
            }
        },


        /**
         * Precomputes YUI modules resources, so that we don't have to at runtime.
         * @private
         * @method _precalcYUIResources
         * @return {nothing}
         */
        _precalcYUIResources: function() {
            var store = this.get('host'),
                name,
                modules,
                mimetype,
                charset,
                fullpath,
                Ysandbox;

            if (!this.staticHandling.serveYUIFromAppOrigin) {
                // this should helps with the memory consumption
                // by avoiding serving YUI Core modules, and instead
                // getting those modules from CDN.
                return;
            }

            Ysandbox = yuiSandboxFactory
                .getYUI(this.yuiConfig.filter)(Y.merge(this.yuiConfig));

            // used to find the the modules in YUI itself
            Ysandbox.use('loader');
            modules = (new Ysandbox.Loader(Ysandbox.config)).moduleInfo || {};

            for (name in modules) {
                if (modules.hasOwnProperty(name)) {
                    // faking a RS object for the sake of simplicity
                    fullpath = libpath.join(__dirname,
                        '../../../../node_modules/yui', modules[name].path);
                    mimetype = libmime.lookup(fullpath);
                    charset  = libmime.charsets.lookup(mimetype);

                    modules[name] = store.makeStaticHandlerDetails({
                        type: 'yui-module',
                        name: name,
                        url: libutil.webpath(this.staticPrefix, 'yui', modules[name].path),
                        path: modules[name].path,
                        source: {
                            fs: {
                                fullPath: fullpath
                            }
                        },
                        mime: {
                            type: mimetype,
                            charset: charset
                        }
                    });
                }
            }
            this.yuiModulesDetails = modules;
        },

        /**
         * Precomputes YUI loader metadata, so that we don't have to at runtime.
         * @private
         * @method _precalcLoaderMeta
         * @param {array} langs array of languages for which to compute YUI loader metadata
         * @return {nothing}
         */
        _precalcLoaderMeta: function(lang) {
            var store = this.get('host'),
                langs,
                Ysandbox,
                modules_config,
                Ysanbdox,
                loader,
                resolved,
                appMetaData = {
                    base: {}
                },
                modules = {}, // regular meta  (a la loader-yui3)
                name,
                i,
                l;

            if (lang) {
                langs = ['', lang];
            } else {
                langs = Object.keys(this.langs);

                // we always want to make the no-lang version
                if (!this.langs['']) {
                    langs.push('');
                }
            }

            Ysandbox = yuiSandboxFactory
                .getYUI(this.yuiConfig.filter)(Y.merge(this.yuiConfig));

            modules_config = this.getModulesConfig('client', false, lang).modules;
            Ysandbox.applyConfig({
                modules: Ysandbox.merge({}, modules_config),
                useSync: true
            });
            Ysandbox.use('loader');

            // using the loader at the server side to compute the loader metadata
            // to avoid loading the whole thing on demand.
            loader = new Ysandbox.Loader(Ysandbox.merge(Ysandbox.config, {
                require: Ysandbox.Object.keys(modules_config)
            }));
            resolved = loader.resolve(true);

            // we need to copy, otherwise the datastructures that Y.loader holds
            // onto get mixed with our changes, and Y.loader gets confused
            resolved = Y.mojito.util.copy(resolved);

            this._processMeta(resolved.jsMods,  modules, modules_config);
            this._processMeta(resolved.cssMods, modules, modules_config);

            for (i = 0; i < langs.length; i += 1) {
                lang = langs[i] || '*';

                appMetaData.base[lang] = {};

                for (name in modules) {
                    if (modules.hasOwnProperty(name)) {
                        if (modules[name].owner &&
                                !modules[modules[name].owner]) {
                            // if there is not a module corresponding with the lang pack
                            // that means the controller doesn't have client affinity,
                            // in that case, we don't need to ship it.
                            continue;
                        }
                        if ((lang === '*') ||
                                (modules[name].langPack === '*') ||
                                    (!modules[name].langPack) ||
                                            (lang === modules[name].langPack)) {

                            // we want to separate modules into different buckets
                            // to be able to support groups in loader config
                            if (modules_config[name]) {
                                appMetaData.base[lang][name] = modules[name];
                            }
                        }
                    }
                }

                appMetaData.base[lang] = serialize(appMetaData.base[lang]);

            } // for each lang

            this.resContents['loader-app'] = MODULE_TEMPLATES['loader-app'];

            for (l = 0; l < langs.length; l += 1) {
                lang = langs[l] || '';

                for (i = 0; i < MODULE_PER_LANG.length; i += 1) {

                    name = MODULE_PER_LANG[i];
                    // populating the internal cache using name+lang as the key
                    this.resContents[([name, lang].join('-'))] =
                        this._produceMeta(name, lang || '*', appMetaData);

                }

            }
        },


        /**
         * @private
         * @method _processMeta
         * @param {object} resolvedMods resolved module metadata, from Y.Loader.resolve()
         * @param {object} modules regular YUI module metadata (ala loader-yui3)
         * @param {object} appModules a hash table with the modules that are part of the app, use to correct paths when needed.
         * @return {nothing}
         */
        _processMeta: function(resolvedMods, modules, appModules) {
            var m,
                l,
                i,
                module,
                name,
                mod,
                mod1,
                lang,
                bundle,
                intlmodules = [];

            for (m in resolvedMods) {
                if (resolvedMods.hasOwnProperty(m) && appModules.hasOwnProperty(resolvedMods[m].name)) {
                    module = resolvedMods[m];

                    mod = name = module.name;

                    bundle = name.indexOf('lang/') === 0;
                    lang = bundle && REGEX_LOCALE.exec(name);
                    if (lang) {
                        mod = mod.slice(0, lang.index); // eg. lang/foo_en-US -> lang/foo
                        lang = lang[1];
                        mod1 = mod.split("/");
                        if (intlmodules.indexOf(mod1[1]) === -1) {
                            intlmodules.push(mod1[1]);
                        }
                        // TODO: validate lang
                    }
                    mod = bundle ? mod.slice(5) : mod; // eg. lang/foo -> foo

                    // language manipulation
                    // TODO: this routine is very restrictive, and we might want to
                    // make it optional later on.
                    if (module.lang) {
                        module.lang = ['{langToken}'];
                    }
                    if (bundle) {
                        module.owner = mod;
                        // applying some extra optimizations
                        module.langPack = lang || '*';
                        module.intl = true;
                        module.expanded_map = undefined;
                    }

                    // getting the last portion of the url which
                    // is the important part for loader to make
                    // combo urls shorter
                    module.path = module.path.split('/').pop();

                    modules[module.name] = {};
                    if (module.type === 'css') {
                        modules[module.name].type = 'css';
                    }
                    for (i = 0; i < MODULE_META_ENTRIES.length; i += 1) {

                        if (MODULE_META_ENTRIES[i] === 'path' && module.intl) {
                            module[MODULE_META_ENTRIES[i]] =
                                'lang/' + module[MODULE_META_ENTRIES[i]];
                        }
                        if (module[MODULE_META_ENTRIES[i]]) {
                            modules[module.name][MODULE_META_ENTRIES[i]] =
                                module[MODULE_META_ENTRIES[i]];
                        }
                    }
                }
            }

            //scan modules in resolvedMods, if a module is also listed in intlmodules,
            //lang holder will be added to the module's meta data.
            //lang info will be plugged in later when _precalcLoaderMeta is called
            for (m in resolvedMods) {
                if (resolvedMods.hasOwnProperty(m) && appModules.hasOwnProperty(resolvedMods[m].name)) {
                    module = resolvedMods[m];
                    if (intlmodules.indexOf(module.name) > -1) {
                        modules[module.name].lang = ['{langToken}'];
                    }
                }
            }
        },


        /**
         * Generates the final YUI metadata.
         * @private
         * @method _produceMeta
         * @param {string} name type of YUI metadata to return
         * @param {string} lang which language the metadata should be customized for
         * @param {object} appMetaData gathered YUI metadata for the application
         * @return {string} the requested YUI metadata
         */
        _produceMeta: function(name, lang, appMetaData) {
            var token = '',
                path  = '';

            if (lang) {
                token = '"' + lang + '"';
                path  = '_' + lang;
            } else {
                lang = '*';
            }

            // module definition definitions
            return MODULE_TEMPLATES[name]
                .replace('{app-base}', appMetaData.base[lang] || appMetaData.base['*'])
                .replace(REGEX_LANG_TOKEN, token)
                .replace(REGEX_LANG_PATH, path);
        },


        /**
         * Precomputes a set of dependencies.
         * @private
         * @method _precomputeYUIDependencies
         * @param {string} lang YUI language code
         * @param {string} env runtime environment (either `client`, or `server`)
         * @param {string} mojit name of the mojit
         * @param {object} modules YUI module metadata
         * @param {object} required lookup hash of YUI module names that are required
         * @param {boolean} forceYLoader whether to force the use of Y.Loader
         * @return {object} precomputed (and sorted) module dependencies
         */
        _precomputeYUIDependencies: function(lang, env, mojit, modules, required, forceYLoader) {
            var loader,
                m,
                module,
                originalYUAnodejs,
                info,
                warn,
                sortedPaths = {};

            // We don't actually need the full list, just the required modules.
            // YUI.Loader() will do the rest at runtime.
            if (!forceYLoader) {
                for (module in required) {
                    if (required.hasOwnProperty(module) && modules[module]) {
                        sortedPaths[module] = modules[module].fullpath;
                    }
                }
                return {
                    sorted: Object.keys(sortedPaths),
                    paths: sortedPaths
                };
            }

            // HACK
            // We need to clear YUI's cached dependencies, since there's no
            // guarantee that the previously calculated dependencies have been done
            // using the same context as this calculation.
            YUI.Env._renderedMods = undefined;

            // Trick the loader into thinking it's -not- running on nodejs.
            // This is the official way to do it.
            originalYUAnodejs = Y.UA.nodejs;
            Y.UA.nodejs = ('server' === env);

            // Use ignoreRegistered here instead of the old `YUI.Env._renderedMods = undefined;` hack
            loader = new Y.Loader({ ignoreRegistered: true });
            // Only override the default if it's required
            if (this.yuiConfig.base) {
                loader.base = this.yuiConfig.base;
            }

            loader.addGroup({modules: modules}, mojit);
            loader.calculate({required: required});

            Y.UA.nodejs = originalYUAnodejs;

            for (m = 0; m < loader.sorted.length; m += 1) {
                module = loader.sorted[m];
                info = loader.moduleInfo[module];
                if (info) {
                    // modules with "nodejs" in their name are tweaks on other modules
                    if ('client' === env && module.indexOf('nodejs') !== -1) {
                        continue;
                    }
                    sortedPaths[module] = info.fullpath || loader._url(info.path);
                }
            }

            // log warning if server mojit has dom dependency
            if ('server' === env) {
                warn = Y.Object.keys(sortedPaths).join(' ').match(WARN_SERVER_MODULES);
                if (warn) {
                    Y.log('your mojit "' + mojit + '" has a server affinity and these client-related deps: ' + warn.join(', '), 'WARN', NAME);
                    Y.log('Mojito may be unable to start, unless you have provided server-side DOM/host-object suppport', 'WARN', NAME);
                }
            }

            return {
                sorted: loader.sorted,
                paths: sortedPaths
            };
        },


        /**
         * Generates the YUI configuration for the resource.
         * @private
         * @method _makeYUIModuleConfig
         * @param {string} env runtime environment (either `client`, or `server`)
         * @param {object} res the resource metadata
         * @return {object} the YUI configuration for the module
         */
        _makeYUIModuleConfig: function(env, res) {
            var config = {
                requires: (res.yui.meta && res.yui.meta.requires) || []
            };
            if ('client' === env) {
                // using relative path since the loader will do the rest
                config.path = res.url;
            } else {
                config.fullpath = res.source.fs.fullPath;
            }
            return config;
        },


        /**
         * If the resource is a YUI module, augments its metadata with metadata
         * about the YUI module and execute the module if it has a server/common affinity.
         * @private
         * @method _captureYUIModuleDetails
         * @param {object} res resource metadata
         * @param {object} runSandbox if passed, the function in the module
         *      will be called using this parameter as the YUI sandbox
         * @return {nothing}
         */
        _captureYUIModuleDetails: function(res, runSandbox) {

            var file = libfs.readFileSync(res.source.fs.fullPath, 'utf8'),
                yui = res.yui || {},
                store = this.get('host'),
                originalAdd = store.YUI.add,
                mod = new Module(res.source.fs.fullPath, module);

            mod.filename =  res.source.fs.fullPath;
            mod.paths = Module._nodeModulePaths(libpath.dirname(res.source.fs.fullPath));

            try {
                mod._compile('module.exports = function (YUI) {' +
                    'return (function () {' + file + '\n;}).apply(global);' +
                    '};', res.source.fs.fullPath);
            } catch (e1) {
                Y.log('Error compiling ' + mod.filename +  ': ' + e1.message, 'error', NAME);
                return;
            }

            // Hook into YUI.add in order to capture the module's YUI metadata.
            store.YUI.add = function(name, fn, version, meta) {
                // Check YUI.Env.mods to make sure the same module is not added multiple times.
                // This is important because if multiple modules have the same name, Mojito uses the first one,
                // so it should not be overwritten.
                if (res.affinity !== 'client' && !store.YUI.Env.mods[name]) {
                    // Client side modules are never used on the server so only add
                    // modules with server or common affinity.
                    originalAdd.apply(store.YUI, arguments);
                }

                yui.name = name;
                yui.version = version;
                yui.meta = meta || {};
                if (!yui.meta.requires) {
                    yui.meta.requires = [];
                }

                if (runSandbox) {
                    try {
                        fn(runSandbox, yui.name);
                    } catch (e) {
                        Y.log('failed to run javascript file ' + res.source.fs.fullPath + '\n' + e.message, 'error', NAME);
                    }
                }
            };

            try {
                mod.exports(store.YUI);
                res.yui = yui;
            } catch (e2) {
                Y.log('Error running ' + mod.filename + '\n' + e2.stack, 'error', NAME);
            }

            store.YUI.add = originalAdd;
        }
    });
    Y.namespace('mojito.addons.rs');
    Y.mojito.addons.rs.yui = RSAddonYUI;

}, '0.0.1', { requires: ['plugin', 'oop', 'loader-base', 'mojito-util']});
