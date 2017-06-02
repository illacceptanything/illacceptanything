/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen:true*/
/*global YUI*/


/**
 * @module ActionContextAddon
 */
YUI.add('mojito-partial-addon', function (Y, NAME) {

    /**
    * <strong>Access point:</strong> <em>ac.partial.*</em>
    * Provides methods for working with "actions" and "views" on the current
    * Mojits.
    * @class Partial.common
    */
    function Addon(command, adapter, ac) {
        this._command = command;
        this._ac = ac;
        this._adapter = adapter;
        this._page = adapter.page;
    }

    Addon.prototype = {

        namespace: 'partial',

        /**
         * This method renders the "data" provided into the "view" specified.
         * The "view" must be the name of one of the files in the current
         * Mojits "views" folder. Returns via the callback.
         * @method render
         * @param {object} data The object to be rendered.
         * @param {string} view The view name to be used for rendering.
         * @param {function} cb callback signature is function(error, result).
         */
        render: function (data, view, cb) {
            var renderer,
                mojitView,
                instance = this._command.instance,
                meta = {view: {}},
                id;

            if (!cb) {
                throw new Error('missing callback when trying to render view: ' + view);
            }

            if (!instance.views[view]) {
                cb('View "' + view + '" not found');
                return;
            }

            mojitView = instance.views[view];
            data = data || {}; // default null data to empty view template
            renderer = new Y.mojito.ViewRenderer(mojitView.engine,
                this._page.staticAppConfig.viewEngine);

            id = NAME + '::' + (instance.id || '@' + instance.type) + '>render:' + view;

            renderer.render(data, instance, mojitView, new Y.mojito.OutputBuffer(id, cb), meta);
        },

        /**
         * This method calls the current mojit's controller with the "action"
         * given and returns its output via the callback.
         *
         * @method invoke
         * @param {string} action name of the action to invoke.
         * @param {object} options a literal object with the configuration
         *
         *      @param {boolean} propagateFailure whether or not errors
         *      invoke should affect the original action by calling adapter.error
         *      @param {object} params optional object to be passed as params in the
         *      invoke command. It defaults to the current action params.
         *
         *          @param {object} route Map of key/value pairs.
         *          @param {object} url Map of key/value pairs.
         *          @param {object} body Map of key/value pairs.
         *          @param {object} file Map of key/value pairs.
         *
         * @param {function} cb callback function to be called on completion.
         */
        invoke: function (action, options, cb) {
            var my = this,
                newCommand,
                newAdapter,
                base = this._command.instance.base,
                type = this._command.instance.type,
                id = NAME + '::' + (base ? '' : '@' + type) + ':' + action;

            // If there are no options use it as the callback
            if ('function' === typeof options) {
                cb = options;
                options = {};
            }

            // the new command baesd on the original command
            newCommand = {
                instance: {
                    base: base,
                    type: type
                },
                action: action,
                context: this._ac.context,
                params: options.params || this._ac.params.getAll()
            };

            // the new adapter that inherit from the original adapter
            newAdapter = new Y.mojito.OutputBuffer(id, function (err, data, meta) {

                // HookSystem::StartBlock
                Y.mojito.hooks.hook('adapterInvoke', my._adapter.hook, 'end', this);
                // HookSystem::EndBlock

                if (err && options.propagateFailure) {
                    my._adapter.error(err);
                    return;
                }

                if (meta) {
                    // Remove whatever "content-type" was set
                    meta.http.headers['content-type'] = undefined;
                    // Remove whatever "view" was set
                    meta.view = undefined;
                }

                cb(err, data, meta);

            });

            // HookSystem::StartBlock
            Y.mojito.hooks.hook('adapterInvoke', this._adapter.hook, 'start', newAdapter);
            // HookSystem::EndBlock

            newAdapter = Y.mix(newAdapter, this._adapter);

            this._ac._dispatch(newCommand, newAdapter);
        }
    };

    Y.namespace('mojito.addons.ac').partial = Addon;

}, '0.1.0', {requires: [
    'mojito-hooks',
    'mojito-output-buffer',
    'mojito-params-addon',
    'mojito-view-renderer'
]});
