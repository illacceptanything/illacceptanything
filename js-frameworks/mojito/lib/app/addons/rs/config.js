/*
 * Copyright (c) 2012, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true, stupid:true, node:true */
/*global YUI*/


/**
 * The <strong>Resource Store</strong> is a Y.Base -- a host for Y.Plugins.
 * Each Addon provides additional functions through a namespace that is
 * attached directly to the resource store.
 * @module ResourceStoreAddon
 */


/**
 * @class RSAddonConfig
 * @extension ResourceStore.server
 */
YUI.add('addon-rs-config', function(Y, NAME) {

    'use strict';

    var libfs = require('fs'),
        libpath = require('path'),
        existsSync = libfs.existsSync || libpath.existsSync,
        libycb = require('ycb'),
        libyaml = require('js-yaml');

    function RSAddonConfig() {
        RSAddonConfig.superclass.constructor.apply(this, arguments);
    }
    RSAddonConfig.NS = 'config';

    Y.extend(RSAddonConfig, Y.Plugin.Base, {

        /**
         * This methods is part of Y.Plugin.Base.  See documentation for that for details.
         * @method initializer
         * @param {object} config Configuration object as per Y.Plugin.Base
         * @return {nothing}
         */
        initializer: function(config) {
            this.appRoot = config.appRoot;
            this.mojitoRoot = config.mojitoRoot;
            this.afterHostMethod('findResourceVersionByConvention', this.findResourceVersionByConvention, this);
            this.beforeHostMethod('parseResourceVersion', this.parseResourceVersion, this);

            this._jsonCache = {};   // fullPath: contents as JSON object
            this._simpleCache = {}; // fullPath: contents as JSON object
            this._ycbCache = {};    // fullPath: context: YCB config object
            this._ycbDims = this._readYcbDimensions();
            this._ycbAppConfig = this._readYcbAppConfig();
        },


        /**
         * Returns the YCB dimensions for the application.
         * @method getDimensions
         * @return {object} the YCB dimensions structure for the app
         */
        getDimensions: function() {
            return Y.mojito.util.copy(this._ycbDims);
        },


        /**
         * Returns the YCB library object for the application config.
         * @method getAppConfigYCB
         * @return {YCB} YCB library object for the application config
         */
        getAppConfigYCB: function() {
            return this._ycbAppConfig;
        },


        /**
         * Reads a JSON file.  In mojito, this should generally only be used for
         * package.json files, and all other mojito config files should instead
         * be read using readConfigSimple() or readConfigYCB().
         * @method readConfigJSON
         * @param {string} fullPath path to JSON file
         * @return {user-defined} contents of file as an object
         */
        readConfigJSON: function(fullPath) {
            var json,
                contents;
            if (!existsSync(fullPath)) {
                return {};
            }
            json = this._jsonCache[fullPath];
            if (!json) {
                try {
                    contents = libfs.readFileSync(fullPath, 'utf-8');
                    json = JSON.parse(contents);
                } catch (e) {
                    throw new Error('Error parsing JSON file: ' + fullPath);
                }
                this._jsonCache[fullPath] = json;
            }
            return Y.mojito.util.copy(json);
        },


        /**
         * Reads and parses a JSON or YAML structured file.
         * @method readConfigSimple
         * @param {string} fullPath path to JSON or YAML file
         * @return {user-defined} contents of file as an object
         */
        readConfigSimple: function(fullPath) {
            var extensions = ['.yml', '.yaml', '.json'],
                basename,   // everything except the extension
                i,
                json = false,
                raw,
                obj;

            obj = this._simpleCache[fullPath];
            if (!obj) {
                basename = fullPath;
                if (libpath.extname(fullPath)) {
                    basename = fullPath.slice(0, libpath.extname(fullPath).length * -1);
                }
                for (i = extensions.length - 1; i >= 0; i -= 1) {
                    try {
                        fullPath = basename + extensions[i];
                        raw = libfs.readFileSync(fullPath, 'utf8');
                        try {
                            if (i === 2) { // json
                                obj = JSON.parse(raw);
                                json = true;
                            } else { // yaml or yml
                                obj = libyaml.load(raw);
                                if (json) {
                                    Y.log(basename + extensions[2] + ' exists but ' + extensions[i] + ' file will be used instead', 'warn', NAME);
                                }
                            }
                            // TODO: what happen when one of them exists?
                            //       and what then more than one exists?
                        } catch (parseErr) {
                            throw new Error(parseErr);
                        }
                    } catch (err) {
                        if (err.errno !== 34) { // if the error was not "no such file or directory" report it
                            throw new Error("Error parsing file: " + fullPath + "\n" + err);
                        }
                    }
                }
                if (!obj) {
                    obj = {};
                }
                this._simpleCache[fullPath] = obj;
            }
            return Y.mojito.util.copy(obj);
        },


        /**
         * Reads a configuration file that is in YCB format.
         * @method readConfigYCB
         * @param {object} ctx runtime context
         * @param {string} fullPath path to the YCB file
         * @return {object} the contextualized configuration
         */
        // TODO:  async interface
        readConfigYCB: function(fullPath, ctx) {
            var store = this.get('host'),
                cacheKey,
                json,
                ycb;

            ctx = store.blendStaticContext(ctx);

            if (!this._ycbCache[fullPath]) {
                this._ycbCache[fullPath] = {};
            }

            cacheKey = JSON.stringify(ctx);
            ycb = this._ycbCache[fullPath][cacheKey];
            if (!ycb) {
                json = this.readConfigSimple(fullPath);
                json = this._ycbDims.concat(json);
                ycb = libycb.read(json, ctx);
                this._ycbCache[fullPath][cacheKey] = ycb;
            }
            return Y.mojito.util.copy(ycb);
        },


        /**
         * Creates a YCB configuration bundle using contents from multiple files.
         * The appropriate dimensions.json file will be mixed in, and doesn't need
         * to be part of the list of files. This method is tolerant to errors, and
         * will fallback to `{}` if a file does not exists or fails to load.
         * @method createMultipartYCB
         * @param {array} paths list of files to load
         * @return {YCB} return a YCB library object
         */
        createMultipartYCB: function(paths) {
            var p,
                path,
                config,
                s,
                section,
                settings = {},
                bundle = [];
            bundle.push(this.getDimensions()[0]);
            for (p = 0; p < paths.length; p += 1) {
                path = paths[p];
                config = this.readConfigSimple(path);
                if (!Y.Lang.isArray(config)) {
                    Y.log('not a YCB file: ' + path, 'error', NAME);
                    return;
                }
                for (s = 0; s < config.length; s += 1) {
                    section = config[s];
                    if (!Y.Lang.isArray(section.settings)) {
                        Y.log('missing "settings" in YCB file: ' + path, 'error', NAME);
                        return;
                    }
                    section.__ycb_source__ = path;
                    bundle.push(section);
                }
            }
            return new libycb.Ycb(bundle);
        },


        /**
         * Using AOP, this is called after the ResourceStore's version.
         * @method findResourceVersionByConvention
         * @param {object} source metadata about where the resource is located
         * @param {string} mojitType name of mojit to which the resource likely belongs
         * @return {object||null} for config file resources, returns metadata signifying that
         */
        findResourceVersionByConvention: function(source, mojitType) {
            var fs = source.fs,
                use = false;

            // we only care about files
            if (!fs.isFile) {
                return;
            }
            // we don't care about files in subdirectories
            if ('.' !== fs.subDir) {
                return;
            }
            // we only care about json or yaml files
            if ('.json' !== fs.ext && '.yaml' !== fs.ext && '.yml' !== fs.ext) {
                return;
            }
            // use package.json for the app and the mojit
            if ('package' === fs.basename && 'bundle' !== fs.rootType) {
                use = true;
            }
            // use all configs in the application
            if ('app' === fs.rootType) {
                use = true;
            }
            // use configs from non-shared mojit resources
            if (mojitType && 'shared' !== mojitType) {
                use = true;
            }
            if (!use) {
                return;
            }

            return new Y.Do.AlterReturn(null, {
                type: 'config'
            });
        },


        /**
         * Using AOP, this is called before the ResourceStore's version.
         * @method parseResourceVersion
         * @param {object} source metadata about where the resource is located
         * @param {string} type type of the resource
         * @param {string} subtype subtype of the resource
         * @param {string} mojitType name of mojit to which the resource likely belongs
         * @return {object||null} for config file resources, returns the resource metadata
         */
        parseResourceVersion: function(source, type, subtype, mojitType) {
            var baseParts,
                res;

            if ('config' !== type) {
                return;
            }

            baseParts = source.fs.basename.split('.');
            if (baseParts.length !== 1) {
                Y.log('invalid config filename. skipping ' + source.fs.fullPath, 'warn', NAME);
                return;
            }
            res = {
                source: source,
                type: 'config',
                affinity: 'common',
                selector: '*'
            };
            if ('app' !== source.fs.rootType) {
                res.mojit = mojitType;
            }
            res.name = libpath.join(source.fs.subDir, baseParts.join('.'));
            res.id = [res.type, res.subtype, res.name].join('-');
            return new Y.Do.Halt(null, res);
        },


        /**
         * Read the application's dimensions.json file for YCB processing. If not
         * available, fall back to the framework's default dimensions.json.
         * @private
         * @method _readYcbDimensions
         * @return {array} contents of the dimensions.json file
         */
        _readYcbDimensions: function() {
            var path = libpath.join(this.appRoot, 'dimensions.json');
            if (!existsSync(path)) {
                path = libpath.join(this.mojitoRoot, 'dimensions.json');
            }
            return this.readConfigSimple(path);
        },

        /**
         * Initializes the special multi-file YCB library for all the application
         * files. By default, we try to load `application.json`, then mix any other
         * relative config file specified in the master section under the
         * `applicationConfigFiles` array, which is optional.
         * @private
         * @method _readYcbAppConfig
         * @return {object} libycb object
         */
        _readYcbAppConfig: function() {
            var ycb,
                i,
                rootAppJSON = libpath.join(this.appRoot, 'application.json'),
                paths = [],
                relativePaths;

            // since application.json is optional, we should be careful
            // in any case there is a low-level cache mechanism going on here.
            if (Y.Lang.isArray(this.readConfigSimple(rootAppJSON))) {
                // ensuring the context runtime:server to read applicationConfigFiles,
                // it does not matter if runtime is not a dimension, it works just fine
                // since the purpose of this block is to read applicationConfigFiles
                // from /application.json, and the rest is part of this.createMultipartYCB
                ycb = this.readConfigYCB(rootAppJSON, {
                    runtime: 'server'
                });
                // adding the master application.json as the top level
                paths.push(rootAppJSON);
                // optional applicationConfigFiles to mix in more configs
                relativePaths = ycb.applicationConfigFiles || [];
                for (i = 0; i < relativePaths.length; i += 1) {
                    paths.push(libpath.resolve(this.appRoot, relativePaths[i]));
                }
            }

            // Note: it doesn't matter if we try to reach the same file multiple
            // times (application.json) because readConfigSimple will cache it anyway
            ycb = this.createMultipartYCB(paths);
            if (!ycb) {
                throw new Error("failed to create a YCB config from the following files:\n  " + paths.join("\n  "));
            }
            return ycb;
        }

    });
    Y.namespace('mojito.addons.rs');
    Y.mojito.addons.rs.config = RSAddonConfig;

}, '0.0.1', { requires: ['plugin', 'oop', 'mojito-util']});
