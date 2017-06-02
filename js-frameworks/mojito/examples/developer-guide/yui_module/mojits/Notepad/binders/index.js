/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('notepad-binder-index', function (Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        init: function (mojitProxy) {
            this.mp = mojitProxy;
        },

        /**
         * @method bind
         * @param {Node} YUI Node
         */
        bind: function (node) {

            // Based on http://yuilibrary.com/gallery/show/storage-lite
            var keyname = 'storage-lite-example',
                notes = node.one('#notes');

            // Populate the textarea with the stored note text on page load.
            notes.set('value', Y.StorageLite.getItem(keyname) || '');

            // Save the contents of the textarea after each keystroke.
            notes.on('keyup', function () {
                Y.StorageLite.setItem(keyname, notes.get('value'));
            });

            // adding a classname to the notes element to facilitate func tests
            notes.addClass('ready');
        }
    };

}, '0.0.1', {requires: [
    'gallery-storage-lite' //see yui_modules/storage-lite.client.js
]});
