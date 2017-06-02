/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint nomen:true*/
/*global YUI*/

/**
 * @module ActionContextAddon
 */
YUI.add('mojito-models-addon', function (Y, NAME) {

    'use strict';

    /**
     * <strong>Access point:</strong> <em>ac.models.*</em>
     * Addon that provides access to the models collection
     * @class Models.common
     */
    function Addon(command, adapter) {
        this._models = {};
        this._page = adapter.page;
        this._instance = command.instance;
    }

    Addon.prototype = {

        namespace: 'models',

        /**
         * Gets model instance
         * @method get
         * @param {string} modelName The model's logical name (filename minus the affinity and context parts).
         * @return {object} contextualized model instance, or null.
         */
        get: function (modelName) {

            var cache = this._page.models || {},
                model = this._models[modelName] || cache[modelName],
                yuiName = this._instance.models[modelName];

            // instantanting the model once during the lifetime of
            // the ac object, this acts like an internal cache.
            if (!model && Y.mojito.models[yuiName]) {

                // We have to heir() otherwise this.something in the model
                // will pollute other instances of the model.
                model = Y.mojito.util.heir(Y.mojito.models[yuiName]);

                if (Y.Lang.isFunction(model.init)) {
                    // NOTE that we use the same config here that we use to
                    // config the controller
                    model.init(this._instance.config);
                }
                this._models[modelName] = model;

            }

            // returning from cache if exists
            return model;

        },

        /**
         * set a model instance at the mojit instance.
         * @method set
         * @param {string} modelName The model name.
         * @param {object} model The model instance.
         */
        set: function (modelName, model) {
            if (!modelName || !model) {
                Y.log('Invalid model name or model instance ' +
                    'when calling `ac.models.set()`: ' + modelName, 'error', NAME);
                return;
            }
            if (this._models[modelName]) {
                Y.log('Overiding an existing model instance with name: ' +
                    modelName, 'warn', NAME);
            }
            this._models[modelName] = model;
        },

        /**
         * Expose a model instance as global. On the server side
         * this means any mojit instance under a particular request
         * will have access to the model. On the client, any
         * mojit instance on the page will have access to
         * the model as well.
         * @method expose
         * @param {string} modelName The model name.
         * @param {object} model Optional model instance, if not
         * present, the instance will be lookup by modelName.
         */
        expose: function (modelName, model) {
            this._page.models = this._page.models || {};
            // you might want to expose an existing local model
            model = model || this.get(modelName);

            // NOTE: models on the server will be destroyed
            //       with the request lifecycle.
            // NOTE: for models on the client, there is no way
            //       to destroy them at the moment, it is tied
            //       to the page life cycle.
            this._page.models[modelName] = model;
            Y.log('Exposing a global model: ' + modelName, 'info', NAME);
        }

    };

    Y.namespace('mojito.addons.ac').models = Addon;

}, '0.1.0', {requires: [
    'mojito',
    'mojito-util'
]});
