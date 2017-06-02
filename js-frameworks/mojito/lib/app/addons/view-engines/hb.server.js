/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen:true, node:true*/
/*global YUI*/


YUI.add('mojito-hb', function (Y, NAME) {

    'use strict';

    var fs = require('fs'),
        HB = require('yui/handlebars').Handlebars,
        cache = YUI.namespace('Env.Mojito.Handlebars');

    /**
     * HandlerBars Adapter for the server runtime.
     * @class HandleBarsAdapterServer
     * @constructor
     * @param {object} options View engine configuration.
     * @private
     */
    function HandleBarsAdapter(options) {
        this.options = options || {};
    }

    HandleBarsAdapter.prototype = {

        /**
         * Renders the handlebars template using the data provided.
         * @param {object} data The data to render.
         * @param {object} instance The expanded mojit instance.
         * @param {object} template The view object from RS to render with format:
         * {'content-path': 'path to view', content: 'cached string'}.
         * @param {object} adapter The output adapter to use.
         * @param {object} meta Optional metadata.
         * @param {boolean} more Whether there is more to be rendered
         */
        render: function (data, instance, template, adapter, meta, more) {
            var cacheTemplates = (this.options.cacheTemplates === false ? false : true),
                handler = function (err, obj) {
                    var output,
                        helpers = HB.helpers;

                    if (err) {
                        adapter.error(err);
                        return;
                    }

                    // making sure we preserve the original helpers
                    // from Y.Handlebars.helpers but letting the custom
                    // helpers per mojit instance to override any of them.
                    if (instance && instance.helpers) {
                        helpers = Y.merge(HB.helpers, instance.helpers);
                    }

                    output = obj.compiled(data, {
                        partials: obj.partials,
                        helpers: helpers
                    });

                    // HookSystem::StartBlock
                    Y.mojito.hooks.hook('hb', adapter.hook, 'end', template);
                    // HookSystem::EndBlock

                    if (more) {
                        adapter.flush(output, meta);
                    } else {
                        adapter.done(output, meta);
                    }
                },
                stack,
                cacheKey,
                fn,
                partial,
                partials;

            // HookSystem::StartBlock
            Y.mojito.hooks.hook('hb', adapter.hook, 'start', template);
            // HookSystem::EndBlock

            // support for legacy url instead of a view object
            if (Y.Lang.isString(template)) {
                Y.log('[view] argument in [render] method should be an object', 'warn', NAME);
                template = {
                    'content-path': template
                };
            }

            cacheKey = template['content-path'];

            if (cacheTemplates && cache[cacheKey]) {
                handler(null, cache[cacheKey]);
                return;
            }

            stack = new Y.Parallel();
            partials = {};

            // first item in the asyc queue is the actual view
            this._getTemplateObj(template, stack.add(function (err, obj) {
                if (err) {
                    Y.log('Error trying to compile view ' + cacheKey, 'error', NAME);
                    Y.log(err, 'error', NAME);
                    return;
                }
                cache[cacheKey] = obj;
            }));

            // after the first item, we just add any partial
            if (instance && instance.partials && Y.Object.keys(instance.partials).length > 0) {
                fn = function (partial, err, obj) {
                    if (err) {
                        Y.log('Error trying to compile partial [' + partial + '] on view ' +
                            cacheKey, 'error', NAME);
                        Y.log(err, 'error', NAME);
                        return;
                    }
                    partials[partial] = obj.compiled;
                };
                for (partial in instance.partials) {
                    if (instance.partials.hasOwnProperty(partial)) {
                        this._getTemplateObj(instance.partials[partial],
                            stack.add(Y.bind(fn, this, partial)));
                    }
                }
            }

            // finally, let's just put the compiled view and partials together
            stack.done(function () {
                if (!cache[cacheKey]) {
                    handler(new Error("Error trying to render view " + cacheKey));
                    return;
                }
                cache[cacheKey].partials = partials;
                handler(null, cache[cacheKey]);
            });

        },

        /**
         * Stringify the handlebars template.
         * @param {string} tmpl The name of the template to render.
         * @return {string} the string representation of the template
         * that can be sent to the client side.
         */
        compiler: function (tmpl, callback) {
            this._getTemplateObj(tmpl, function (err, obj) {
                callback(err, JSON.stringify(obj.raw));
            });
        },

        /**
         * Precompiles the handlebars template as a string of javascript.
         * @param {string} tmpl The name of the template to render.
         * @return {string} the precompiled template that can be sent to the client side as Javascript code.
         */
        precompile: function (tmpl, callback) {
            this._getTemplateObj(tmpl, function (err, obj) {
                callback(err, HB.precompile(obj.raw));
            });
        },

        /**
         * Build a compiled handlebar template, plus
         * a raw string representation of the template.
         * @private
         * @param {object|string} template The view object from RS to render with format:
         * {'content-path': 'path to view', content: 'cached string'} or a string with the
         * fullpath of the template to be loaded.
         * @param {function} callback The function that is called with the compiled template
         */
        _getTemplateObj: function (template, callback) {
            var fn = function (err, str) {
                    if (err) {
                        callback(err);
                        return;
                    }
                    callback(null, {
                        raw: str,
                        compiled: HB.compile(str)
                    });
                };
            if (template.content) {
                fn(null, template.content);
            } else {
                this._loadTemplate((typeof template === 'string' ? template : template['content-path']), fn);
            }
        },

        /**
         * Loads a template from the file system
         * @param {string} tmpl The location of the template file
         * @param {function} callback The callback to call with the template contents
         * @private
         */
        _loadTemplate: function (tmpl, callback) {
            fs.readFile(tmpl, 'utf8', function (err, data) {
                callback(err, data);
            });
        }
    };

    Y.namespace('mojito.addons.viewEngines').hb = HandleBarsAdapter;

}, '0.1.0', {requires: [
    'mojito',
    'parallel',
    'mojito-hooks'
]});
