/*
 * Copyright (c) 2012, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/
/*global YUI*/


/**
 * @module ResourceStoreAddon
 */

/**
 * @class RSAddonSelector
 * @extension ResourceStore.server
 */
YUI.add('addon-rs-selector', function(Y, NAME) {

    function RSAddonSelector() {
        RSAddonSelector.superclass.constructor.apply(this, arguments);
    }
    RSAddonSelector.NS = 'selector';
    RSAddonSelector.DEPS = ['config'];

    Y.extend(RSAddonSelector, Y.Plugin.Base, {

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

            this.loadConfigs();

            this._poslCache = {};   // context: POSL

            this.onHostEvent('loadConfigs', this.loadConfigs, this);
        },

        loadConfigs: function () {
            this._appConfigYCB = this.host.config.getAppConfigYCB();
        },

        /**
         * Returns a list of all priority-ordered selector lists (POSLs).
         * @method getAllPOSLs
         * @return {array} list of priority-ordered selector lists
         */
        getAllPOSLs: function() {
            var c,
                ctx,
                ctxs,
                posl,
                posls = {};
            ctxs = this._listUsedContexts();
            for (c = 0; c < ctxs.length; c += 1) {
                ctx = ctxs[c];
                posl = this.getPOSLFromContext(ctx);
                posls[JSON.stringify(posl)] = posl;
            }
            ctxs = null; // free a bunch of memory
            return Y.Object.values(posls);
        },


        /**
         * Returns the priority-ordered selector list (POSL) for the context.
         * @method getPOSLFromContext
         * @param {object} ctx runtime context
         * @param {boolean} update whether to delete the cache entry and recompute the posl
         * @return {array} priority-ordered selector list
         */
        getPOSLFromContext: function(ctx, update) {
            var store = this.get('host'),
                cacheKey,
                posl,
                p,
                part,
                parts;

            this._appConfigYCB = this.host.config.getAppConfigYCB();

            cacheKey = JSON.stringify(ctx);

            if (update) {
                delete this._poslCache[cacheKey];
            } else {
                posl = this._poslCache[cacheKey];
            }

            if (!posl) {
                posl = ['*'];
                // TODO:  use rs.config for this too
                parts = this._appConfigYCB.readNoMerge(ctx, {});
                for (p = 0; p < parts.length; p += 1) {
                    part = parts[p];
                    if (part.selector && store.selectors[part.selector]) {
                        posl.unshift(part.selector);
                    }
                }
                this._poslCache[cacheKey] = posl;
            }
            // NOTE:  We used to copy() here. Research suggested that it was safe to drop.
            return posl;
        },


        /**
         * Returns the a list of dimensions that are actually used in the
         * application.json file.
         * @private
         * @method _listUsedDimensions
         * @return {array} list of dimensions and values
         *     (values have no structure)
         */
        _listUsedDimensions: function() {
            var ctxs = [],
                ctxValues = {}; // dimName: value: true

            this._appConfigYCB = this.host.config.getAppConfigYCB();
            this._appConfigYCB.walkSettings(function(settings, config) {
                Y.Object.each(settings, function(val, name) {
                    if (!ctxValues[name]) {
                        ctxValues[name] = {};
                    }
                    ctxValues[name][val] = true;
                });
                return true;
            });
            Y.Object.each(ctxValues, function(vals, name) {
                ctxs[name] = Object.keys(vals);
            });
            return ctxs;
        },


        /**
         * Generates a list of contexts to which application.json is sensitive.
         * @private
         * @method _listUsedContexts
         * @return {array of objects} all contexts in application.json
         */
        _listUsedContexts: function() {
            var dims = this._listUsedDimensions(),
                nctxs,
                c,
                ctxs = [],
                dn,
                dname,
                dnames = Object.keys(dims),
                dv,
                dval,
                dvals,
                e,
                each,
                mod;

            nctxs = 1;
            for (dn = 0; dn < dnames.length; dn += 1) {
                dname = dnames[dn];
                dvals = dims[dname];
                if (dname !== 'runtime') {
                    // we never have indeterminant runtime
                    dvals.push('*');
                }
                nctxs *= dvals.length;
            }

            for (c = 0; c < nctxs; c += 1) {
                ctxs[c] = {};
            }
            mod = 1;
            for (dn = 0; dn < dnames.length; dn += 1) {
                dname = dnames[dn];
                dvals = dims[dname];
                mod *= dvals.length;
                each = nctxs / mod;

                e = each;
                dv = 0;
                for (c = 0; c < nctxs; e -= 1, c += 1) {
                    if (0 === e) {
                        e = each;
                        dv += 1;
                        dv = dv % dvals.length;
                    }
                    dval = dvals[dv];
                    if ('*' !== dval) {
                        ctxs[c][dname] = dval;
                    }
                }
            }

            return ctxs;
        }


    });
    Y.namespace('mojito.addons.rs');
    Y.mojito.addons.rs.selector = RSAddonSelector;

}, '0.0.1', { requires: ['plugin', 'oop', 'mojito-util']});
