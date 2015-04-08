/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, nomen:true*/
/*global YUI*/

YUI.add('mojito-view-renderer', function (Y) {

    'use strict';

    var cache = {};

    /*
     * Mojito's view renderer abstraction. Will plugin in the specified view
     * plugin to do the rendering, depending on the 'type' specified.
     * @class ViewRenderer
     * @namespace Y.mojito
     * @constructor
     * @param {String} type view engine addon type to use
     * @param {Object} options View engines configuration.
     */
    function Renderer(type, options) {
        this._type = type || 'hb';
        this._options = options;
        if (!cache[this._type]) {
             // caching renderer instance
            cache[this._type] = new (Y.mojito.addons.viewEngines[this._type])(options);
        }
        this._renderer = cache[type];
    }

    Renderer.prototype = {

        /*
         * Renders a view
         * @method render
         * @param {Object} data data to push into the view.
         * @param {string} mojitType name of the mojit type.
         * @param {Object} tmpl some type of template identifier for the view
         *     engine.
         * @param {Object} adapter The output adapter.
         * @param {Object} meta Optional metadata to use.
         * @param {boolean} more Whether there will be more data to render
         *     later. (streaming)
         */
        render: function (data, mojitType, tmpl, adapter, meta, more) {
            // HookSystem::StartBlock
            Y.mojito.hooks.hook('Render', adapter.hook, data, mojitType, tmpl, adapter, meta, more);
            // HookSystem::EndBlock
            this._renderer.render(data, mojitType, tmpl, adapter, meta, more);
        }

    };

    Y.namespace('mojito').ViewRenderer = Renderer;

}, '0.1.0', {requires: [
    'mojito',
    'mojito-hooks'
]});
