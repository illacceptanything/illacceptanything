/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true*/
/*global YUI*/


YUI.add('shelf-binder-index', function(Y, NAME) {

    /**
     * Generates a color.
     * @param {String} color The seed color.
     * @return {String} "#nnn234" where nnn are random numbers between 0-9.
     */
    function someColor(color) {
        var somenum = Math.floor(Math.random() * 9);
        color = color || '';
        return color.length > 2 ?
                ('#' + color + '234') :
                someColor(color + somenum);
    }

    /**
     * Applies background color to a node.
     * @param {Y.Node} node The node to colorize.
     */
    function colorize(node) {
        setTimeout(function() {
            node.setStyles({
                'backgroundColor': someColor(),
                'color': '#eee'
            });
        }, Math.floor(Math.random() * 500));
    }

    /**
     * Bind client-side DOM events.
     * @class ShelfIndexBinder
     */
    Y.namespace('mojito.binders')[NAME] = {

        init: function(mojitProxy) {
            this.mp = mojitProxy;
        },

        /**
         * Invoked after ../controller.common.js:index().
         * @param {Y.Node} node The node instance for this mojit's div.
         */
        bind: function(node) {
            node.all('div.toc ul li a').each(function(el) {
                colorize(el);
            });
        }
    };

}, '0.0.1', {requires: [
    'node'
]});
